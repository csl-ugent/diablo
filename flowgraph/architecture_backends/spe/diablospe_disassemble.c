/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * }}}
 *
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diablospe.h>

/* SpeDisassembleImmediate {{{ */
/*! Decode and apply transformations to the encoded immediate */
static void
SpeDisassembleImmediate (t_uint32 raw, t_spe_ins *ins, t_spe_opcode_info *info)
{
  t_address imm;
  t_spe_imm_trans immtr;
  int num;

  imm = AddressNewForIns (T_INS(ins), Bitfield32Get (raw, info->immstart, info->immsize));
  SPE_INS_SET_IMMEDIATE_ORIG (ins, imm);

  ASSERT(info->immtrans,
         ("no immediate transformation for @G (%s)",
          SPE_INS_OLD_ADDRESS(ins), info->name));

  for (immtr = info->immtrans; *immtr; immtr++)
  {
    switch (*immtr)
    {
      case 's':
        imm = AddressSignExtend (imm, info->immsize - 1);
        break;
      case '<':
        immtr++;
        ASSERT('0' <= *immtr && *immtr <= '9', ("Unknown immediate left shift '%s'", immtr));
        for (num = 0; '0' <= *immtr && *immtr <= '9'; immtr++)
        {
          num = (num * 10) + (*immtr - '0');
        }
        immtr--;
        imm = AddressBitMove (imm, AddressSize (imm), num);
        break;
      case 'p':
        imm = AddressAdd (imm, SPE_INS_CADDRESS (ins));
        break;
      default:
        FATAL(("Unknown immediate transformation '%s' in '%s'", immtr, info->immtrans));
    }
  }

  SPE_INS_SET_IMMEDIATE (ins, imm);
}
/* }}} */

/* SpeDisassembleSection {{{ */
#define DEBUG_TYPE 0 /* Debug decoded types */

void SpeDisassembleData(t_uint32 raw, t_spe_ins *ins)
{
  SPE_INS_SET_CSIZE (ins, AddressNew32 (4));
  SPE_INS_SET_OLD_SIZE (ins, AddressNew32 (4));

  SPE_INS_SET_OPCODE(ins, SPE_DATA);
  SPE_INS_SET_ASSEMBLER(ins, SpeAssembleData);
  SPE_INS_SET_TYPE(ins, IT_DATA);
  SPE_INS_SET_ATTRIB(ins, 0x0);
  SPE_INS_SET_FLAGS(ins, 0x0);
  SPE_INS_SET_BHINT(ins, SPE_UNKNOWN);
  SPE_INS_SET_IMMEDIATE(ins, AddressNew32(raw));
}

void SpeDisassembleOne(t_uint32 raw, t_spe_ins *ins)
{
  t_spe_opcode_info *o_info;
  t_spe_opcode i = -1; /* Must search for the instruction's opcode */

  SPE_INS_SET_CSIZE (ins, AddressNew32 (4));
  SPE_INS_SET_OLD_SIZE (ins, AddressNew32 (4));

  /* Lookup the opcode in the opcode table */
  if (i == -1)
  {
    for (i = 0; ((raw) & spe_opcode_table[i].mask) != spe_opcode_table[i].opcode; i++) ;
    ASSERT (i <= SPE_LAST_OPCODE, ("Unknown opcode in instruction %#x", raw));
  }
  o_info = &spe_opcode_table[i];

  SPE_INS_SET_OPCODE (ins, i);
  SPE_INS_SET_ASSEMBLER(ins, o_info->Assemble);
  SPE_INS_SET_TYPE (ins, o_info->type);
  SPE_INS_SET_ATTRIB (ins, o_info->attribs);
  SPE_INS_SET_FLAGS (ins, o_info->flags);
  SPE_INS_SET_BHINT (ins, SPE_UNKNOWN);
  SpeDisassembleImmediate (raw, ins, o_info);
  SpeDisassembleRegisters (raw, ins, o_info);
  if (o_info->Disassemble)
  {
    o_info->Disassemble (raw, ins, i);
  }

#if DEBUG_TYPE
  DEBUG(("Code %#x: @I", raw, ins));
#endif
}
/*! Disassemble an entire section.
 *
 * This disassembly creates an array of t_spe_ins's.
 *
 * \param sec The section to disassemble
 * \return void
 */
void
SpeDisassembleSection (t_section *sec)
{
  t_object *obj = SECTION_OBJECT (sec);
  t_spe_ins *i_sec = NULL;
  t_spe_ins *i_prev = NULL;
  int nins = 0;
  t_address a_offset, a_current;
  int teller = 0;
  t_uint32 data;

  a_offset = AddressNullForObject (obj);

  while (AddressIsLt (a_offset, SECTION_CSIZE (sec)))
  {
    nins++;
    i_sec = SpeInsNewForSec (sec);
    if (!i_sec) FATAL (("No instructions!"));
    a_current = AddressAdd (SECTION_CADDRESS (sec), a_offset);
    ASSERT (AddressIsNull (AddressAnd (a_current, AddressNewForObject (obj, 0x3))),
            ("Instruction in SPE mode not aligned"));

    data = SectionGetData32 (sec, a_offset);

    SPE_INS_SET_CADDRESS (i_sec, a_current);
    SPE_INS_SET_OLD_ADDRESS (i_sec, AddressAdd(SECTION_OLD_ADDRESS(sec), a_offset));

    if (SymbolTableGetDataType(obj, a_current) == ADDRESS_IS_CODE)
    {
      SpeDisassembleOne(data, i_sec);
    }
    else
    {
      SpeDisassembleData(data, i_sec);
    }

    SECTION_ADDRESS_TO_INS_MAP (sec)[teller] = T_INS (i_sec);

    SPE_INS_SET_IPREV (i_sec, i_prev);
    if (i_prev) SPE_INS_SET_INEXT (i_prev, i_sec);
    i_prev = i_sec;

    a_offset = AddressAddUint32 (a_offset, 4);
    teller += 4;
  }

  SPE_INS_SET_INEXT (i_sec, NULL);

  VERBOSE(0, ("Disassembled %d SPE instructions from %s", nins, SECTION_NAME (sec)));
}
/* }}} */

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
