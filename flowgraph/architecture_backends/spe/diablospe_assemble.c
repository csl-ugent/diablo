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

/* SpeAssembleImmediate {{{ */
/*! Encode and unapply transformations into the final instruction's immediate */
static void
SpeAssembleImmediate (t_uint32 *raw, t_spe_ins *ins, t_spe_opcode_info *info)
{
  int num;
  t_address imm;
  t_spe_imm_trans immtr, immtr2;

  imm = SPE_INS_IMMEDIATE (ins);

  for (immtr = info->immtrans + strlen(info->immtrans) - 1; immtr >= info->immtrans; immtr--)
  {
    switch (*immtr)
    {
      case 's':
        break;
      case '<':
        immtr2 = immtr + 1;
        ASSERT('0' <= *immtr2 && *immtr2 <= '9', ("Unknown immediate left shift '%s'", immtr2));
        for (num = 0; '0' <= *immtr2 && *immtr2 <= '9'; immtr2++)
        {
          num = (num * 10) + (*immtr2 - '0');
        }
        imm = AddressShr (imm, AddressNewForIns (T_INS(ins), num));
        num = 0;
        break;
      case 'p':
        imm = AddressSub (imm, SPE_INS_CADDRESS (ins));
        break;
      default:
        if (*immtr < '0' || '9' < *immtr)
        {
          FATAL(("Unknown immediate transformation '%s'", immtr));
        }
    }
  }

  *raw = Bitfield32Set (*raw, AddressExtractUint32 (imm), info->immstart, info->immsize);
}
/* }}} */

/* SpeAssembleSection {{{ */
#define DEBUG_RAW 0
t_uint32 SpeAssembleOne(t_spe_ins *ins)
{
  t_uint32 data;
  t_spe_opcode_info *o_info = &spe_opcode_table[SPE_INS_OPCODE (ins)];

  data = o_info->opcode;
  if (o_info->immsize)
  {
    SpeAssembleImmediate (&data, ins, o_info);
  }
  SpeAssembleRegisters (&data, ins, o_info);
  if (o_info->Assemble) o_info->Assemble (&data, ins);

  if (OBJECT_SWITCHED_ENDIAN(INS_OBJECT(T_INS(ins))))
  {
    data = Uint32SwapEndian(data);
  }

#if DEBUG_RAW
  DEBUG(("@G: %#x", SPE_INS_CADDRESS(ins), data));
#endif
  return data;
}

/*! Assemble an entire section
 *
 * The assembled instructions are written into the temporary buffer of the
 * section (allocated by DiabloAction),
 *
 * \param sec The section to assemble
 *
 * \return void
 */
void
SpeAssembleSection (t_section *sec)
{
  t_spe_ins *i_ins;
  char *data;
  int nins;

  nins = 0;
  i_ins = T_SPE_INS (SECTION_DATA (sec));
  data = SECTION_TMP_BUF (sec);

  while (i_ins)
  {
    *((t_uint32*)data) = SpeAssembleOne(i_ins);
    nins++;
    data += 4;
    i_ins = SPE_INS_INEXT (i_ins);
  }

  VERBOSE(0, ("Assembled %d instructions", nins));
}
/* }}} */

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
