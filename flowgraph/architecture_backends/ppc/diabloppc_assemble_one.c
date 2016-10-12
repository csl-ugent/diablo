/*
 * Copyright (C) 2005 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
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
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

/* TODO: check L flag implementation */ 

#include <diabloppc.h>

#define PPC_IMM(instr) PPC_BITFIELD(instr, 16, 16)

/*!
 * Assemble ppc instructions
 *
 * All the operations defined have the same parameters.
 * The Assemble operations are split by the
 * instruction format defined by the PPC architecture.
 *
 * \param ins The ppc instruction to disassemble
 * \param instr The encoded ppc instruction buffer
 *
 * \return void
 */


/* PpcAssembleD {{{ */
void
PpcAssembleD(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_ADDIC_DOT:
    case PPC_ADDIC:
    case PPC_SUBFIC:
    case PPC_ADDI:
    case PPC_MULLI:
      /* Instruction format :
         OPCODE | RT | RA | Signed integer
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_ADDIS:
      /* Instruction format :
         OPCODE | RT | RA | Signed integer
         Shifted form
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins))>>16);
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_ANDI_DOT:
    case PPC_ORI:
    case PPC_XORI:
      /* Instruction format :
         OPCODE | RA | RT | Unsigned integer
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      break;
    case PPC_ANDIS_DOT:
    case PPC_ORIS:
    case PPC_XORIS:
      /* Instruction format :
         OPCODE | RA | RT | Unsigned integer
         Shifted form
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins))>>16);
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      break;
    case PPC_LFD:
    case PPC_LFDU:
    case PPC_LFS:
    case PPC_LFSU:
      /* Instruction format :
         OPCODE | RT | RA | C2-integer-sign-extended
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_FPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_LBZ:
    case PPC_LBZU:
    case PPC_LHA:
    case PPC_LHAU:
    case PPC_LHZ:
    case PPC_LHZU:
    case PPC_LWZ:
    case PPC_LWA:
    case PPC_LWZU:
    case PPC_LD:
    case PPC_LDU:
      /* Instruction format :
         OPCODE | RT | RA | C2-integer-sign-extended
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      if(PPC_INS_OPCODE(ins) == PPC_LDU)
      {
        PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) | 0x0001);
      } 
      else if(PPC_INS_OPCODE(ins) == PPC_LWA)
      {
        PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) | 0x0002);
      } 
      break;
    case PPC_STB:
    case PPC_STBU:
    case PPC_STH:
    case PPC_STHU:
    case PPC_STW:
    case PPC_STWU:
    case PPC_STD:
    case PPC_STDU:
      /* Instruction format :
         OPCODE | RA | RB | C2-integer-sign-extended
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins));
      if(PPC_INS_OPCODE(ins) == PPC_STDU)
      {
        PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) | 0x0001);
      }
      break;
    case PPC_STFD:
    case PPC_STFDU:
    case PPC_STFS:
    case PPC_STFSU:
      /* Instruction format :
         OPCODE | RA | RB | C2-integer-sign-extended
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_FPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins));
      break;
    case PPC_TWI:
    case PPC_TDI:
      /* Instruction format :
         OPCODE | TO | RA | signed-integer
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_CT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_CMPI:
    case PPC_CMPLI:
      /* Instruction format :
         OPCODE | BF | / | L | RA | integer (signed or unsigned)
       */
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_CR(*instr, 6, PPC_INS_REGT(ins));
      PPC_BITFIELD_SET_VAL(*instr, 10, 1, (PPC_INS_FLAGS(ins)&PPC_FL_L)!=0);
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_LMW:
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr,6,PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr,11,PPC_INS_REGA(ins));
      break; 
    case PPC_STMW:
      PPC_ASM_SET_IMM(*instr, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr,6,PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr,11,PPC_INS_REGA(ins));
      break; 
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleX{{{*/
void
PpcAssembleX(t_ppc_ins * ins, t_uint32 * instr)
{

  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_LFDUX:
    case PPC_LFSUX:
    case PPC_LFDX:
    case PPC_LFSX:
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      goto encode_1;
    case PPC_ECIWX:
    case PPC_LBZUX:
    case PPC_LHAUX:
    case PPC_LWAUX:
    case PPC_LWAX:
    case PPC_LHZUX:
    case PPC_LWZUX:
    case PPC_LDUX:
    case PPC_LBZX:
    case PPC_LHAX:
    case PPC_LHBRX:
    case PPC_LHZX:
    case PPC_LWARX:
    case PPC_LDARX:
    case PPC_LWBRX:
    case PPC_LWZX:
    case PPC_LDX:
      /* Instruction format:
         OPCODE | RT | RA | RB
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGT(ins));
encode_1:
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_FRSP:
    case PPC_FCTIW:
    case PPC_FCTID:
    case PPC_FCFID:
    case PPC_FCTIWZ:
    case PPC_FCTIDZ:
    case PPC_FMR:
    case PPC_FABS:
    case PPC_FNABS:
    case PPC_FNEG:
      /* Instruction format:
         OPCODE | RT  | zeros | RA | Rc
       */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGA(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_MFFS:
      /* Instruction format:
         OPCODE | RT  | zeros | zeros | Rc
      */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_MFCR:
      /* Instruction format:
         OPCODE | RT | zeros | zeros | / 
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGT(ins));
      break;
    case PPC_AND:
    case PPC_ANDC:
    case PPC_EQV:
    case PPC_NAND:
    case PPC_NOR:
    case PPC_XOR:
    case PPC_OR:
    case PPC_ORC:
      /* Instruction format:
         OPCODE | RA | RT | RB | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_STWCX_DOT:
    case PPC_STDCX_DOT:
      PPC_BITFIELD_SET(*instr,31);
    case PPC_STBUX:
    case PPC_STHUX:
    case PPC_STWUX:
    case PPC_STDUX:
    case PPC_STBX:
    case PPC_STHX:
    case PPC_STWX:
    case PPC_STDX:
    case PPC_STHBRX:
    case PPC_STWBRX:
    case PPC_STSWX:
      /* Instruction format:
         OPCODE | RA | RB | RC | \
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGC(ins));
      break;
    case PPC_SLW:
    case PPC_SLD:
    case PPC_SRAW:
    case PPC_SRAD:
    case PPC_SRW:
    case PPC_SRD:
      /* Instruction format:
         OPCODE | RA | RT | RB | Rc
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_STFDUX:
    case PPC_STFSUX:
    case PPC_STFDX:
    case PPC_STFSX:
    case PPC_STFIWX:  
      /* Instruction format:
         OPCODE | RA | RT | RB
       */
      PPC_ASM_SET_FPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGC(ins));
      break;
    case PPC_ECOWX:
      /* Instruction format:
         OPCODE | RA | RB | RC
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGC(ins));
      break;
    case PPC_CNTLZW:
    case PPC_CNTLZD:
    case PPC_EXTSH:
    case PPC_EXTSB:
    case PPC_EXTSW:
      /* Instruction format:
         OPCODE | RA | RT | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_SRAWI:
      /* Instruction format:
         OPCODE | RA | RT | Shift amount | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_SRADI:
      /* Instruction format:
         OPCODE | RA | RT | Shift amount | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) & 0x0000001f);
      PPC_BITFIELD_SET_VAL(*instr, 30, 1, (AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) & 0x00000020)>>5);
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_CMP:
    case PPC_CMPL:
      /* Instruction format:
         OPCODE | CR | L | RA | RB
       */
      PPC_ASM_SET_CR (*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      PPC_BITFIELD_SET_VAL(*instr, 10, 1, (PPC_INS_FLAGS(ins)&PPC_FL_L)!=0);
      break;
    case PPC_FCMPO:
    case PPC_FCMPU:
      /* Instruction format:
         OPCODE | CR | RA | RB
       */
      PPC_ASM_SET_CR (*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_MCRFS:
      /* Instruction format:
         OPCODE | CR-dest | CR-orig
       */
      PPC_ASM_SET_CR (*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_CR (*instr, 11, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    case PPC_MTFSFI:
      /* Instruction format:
         OPCODE | BF | Immediate | Rc
       */
      PPC_ASM_SET_CR (*instr,  6, PPC_INS_REGA(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 4, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_EIEIO:
      /* Instruction format:
         OPCODE
       */
      break;
    case PPC_SYNC:
      /* Instruction format:
         OPCODE | L
       */
      PPC_BITFIELD_SET_VAL(*instr, 9, 2, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    case PPC_DCBF:
    case PPC_DCBST:
    case PPC_DCBT:
    case PPC_DCBTST:
    case PPC_DCBZ:
    case PPC_ICBI:
      /* Instruction format:
         OPCODE | RA | RB
       */
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_MTFSB1:
    case PPC_MTFSB0:
      /* Instruction format:
         OPCODE | bit | Rc
       */
      PPC_BITFIELD_SET_VAL(*instr, 6, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_TW:
    case PPC_TD:
      /* Instruction format:
         OPCODE | TO | RA | RB
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_CT  (ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_LSWI:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins))); 
      break;
    case PPC_LSWX:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_STSWI:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins))); 
      break;
    case PPC_MFSRIN:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGA(ins));
    case PPC_MTMSR:
    case PPC_MTMSRD:
      PPC_ASM_SET_GPR(*instr,6, PPC_INS_REGA(ins));
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        PPC_BITFIELD_SET_VAL(*instr,15,1,1);
      }
      break;
    case PPC_MFSR:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_SRR(*instr, 12, PPC_INS_REGA(ins));
      break;
    case PPC_MTSR:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_SRR(*instr, 12, PPC_INS_REGT(ins));
      break;
    case PPC_MTSRIN:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_SLBIA:
    case PPC_TLBIA:
    case PPC_TLBSYNC:
      break;
    case PPC_TLBIE:
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        PPC_BITFIELD_SET_VAL(*instr,10,1,1);
      }
    case PPC_SLBIE:
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGA(ins));
      break;
    case PPC_SLBMFEE:
    case PPC_SLBMFEV:
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      break;
    case PPC_SLBMTE:
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      break;
    case PPC_MFMSR:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
}
/*}}}*/

/* PpcAssembleI{{{*/
void
PpcAssembleI(t_ppc_ins * ins, t_uint32 * instr)
{
  t_int64 soffset;

  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_B:
      /* Instruction format:
         OPCODE | LI | AA | LK
         where LI is signed integer in c2
       */

      /* encode branch offset */
      soffset = AddressExtractInt64SignExtend(PPC_INS_IMMEDIATE(ins));
      ASSERT(soffset < (1 << 25) && soffset >= -(1 << 25),
             ("offset %lld cannot be encoded in this instruction", soffset));
      PPC_BITFIELD_SET_VAL(*instr, 6, 24, soffset >> 2);
      if (PPC_INS_FLAGS(ins) & PPC_FL_ABSOLUTE)
      {
        PPC_BITFIELD_SET(*instr, 30);
      }
      if (PPC_INS_FLAGS(ins) & PPC_FL_LINK)
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleB{{{*/
void
PpcAssembleB(t_ppc_ins * ins, t_uint32 * instr)
{
  t_int64 soffset;

  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_BC:
      /* Instruction format:
         OPCODE | BO | BI | BD | AA | LK
       */
      PPC_BITFIELD_SET_VAL(*instr,  6, 5, PPC_INS_BO(ins));
      PPC_BITFIELD_SET_VAL(*instr, 11, 5, PPC_INS_CB(ins));

      /* encode branch offset */
      soffset = AddressExtractInt64SignExtend(PPC_INS_IMMEDIATE(ins));
      ASSERT(soffset < (1 << 15) && soffset >= -(1 << 15),
             ("offset %lld cannot be encoded in this instruction", soffset));
      PPC_BITFIELD_SET_VAL(*instr, 16, 14, soffset >> 2);

      if (PPC_INS_FLAGS(ins) & PPC_FL_ABSOLUTE)
      {
        PPC_BITFIELD_SET(*instr, 30);
      }
      if (PPC_INS_FLAGS(ins) & PPC_FL_LINK)
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleSC{{{*/
void
PpcAssembleSC(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_SC:
      /* Instruction format:
         OPCODE
       */
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleDS{{{*/
void
PpcAssembleDS(t_ppc_ins * ins, t_uint32 * instr)
{
  FATAL(("%s : Implement @I", __func__, ins));
}
/*}}}*/

/* PpcAssembleXL{{{*/
void
PpcAssembleXL(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_BCCTR:
    case PPC_BCLR:
      /* Instruction format:
         OPCODE | BO | BI | BH | LK
       */
      PPC_BITFIELD_SET_VAL(*instr,  6, 5, PPC_INS_BO(ins));
      PPC_BITFIELD_SET_VAL(*instr, 11, 5, PPC_INS_CB(ins));
      PPC_BITFIELD_SET_VAL(*instr, 19, 2, PPC_INS_BH(ins));
      if (PPC_INS_FLAGS(ins) & PPC_FL_LINK)
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    case PPC_CRAND:
    case PPC_CRANDC:
    case PPC_CREQV:
    case PPC_CRNAND:
    case PPC_CRNOR:
    case PPC_CROR:
    case PPC_CRORC:
    case PPC_CRXOR:
      /* Instruction format:
         OPCODE | CR bit  T | CR bit A | CR bit B
       */
      PPC_ASM_SET_BIT_CR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_BIT_CR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_BIT_CR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_MCRF:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_ASM_SET_CR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_CR(*instr, 11, PPC_INS_REGA(ins));
      break;
    case PPC_ISYNC:
      /* Instruction format:
         OPCODE
       */
      break;
    case PPC_RFID:
      /* Instruction format:
         OPCODE
       */
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleXFX{{{*/
void
PpcAssembleXFX(t_ppc_ins * ins, t_uint32 * instr )
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_MTSPR:
      /* Instruction format:
         OPCODE | RA | RT
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_SPR(*instr,    PPC_INS_REGT(ins));
      break;
    case PPC_MFSPR:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_SPR(*instr,    PPC_INS_REGA(ins));
      break;
    case PPC_MFTB:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGT(ins));
      PPC_ASM_SET_TBR(*instr,    PPC_INS_REGA(ins));
      break;
    case PPC_MTCRF:
    case PPC_MTOCRF:
      /* Instruction format:
         OPCODE | RA | MASK
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_BITFIELD_SET_VAL(*instr, 12, 8, PPC_INS_MASK(ins));
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleXFL{{{*/
void
PpcAssembleXFL(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_MTFSF:
      /* Instruction format:
         OPCODE | MASK | RA | Rc
       */
      PPC_BITFIELD_SET_VAL(*instr, 7, 8, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGA(ins));
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
      {
        PPC_BITFIELD_SET(*instr, 31);
      }
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleXS{{{*/
void
PpcAssembleXS(t_ppc_ins * ins, t_uint32 * instr)
{
  FATAL(("%s : Implement @I", __func__, ins));
}
/*}}}*/

/* PpcAssembleXO{{{*/
void
PpcAssembleXO(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
  {
    PPC_BITFIELD_SET(*instr, 31);
  }
  if (PPC_SR_MARKED(PPC_INS_SREGS(ins), XER, OV))
  {
    PPC_BITFIELD_SET(*instr, 21);
  }
  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_ADD:
    case PPC_ADDC:
    case PPC_ADDE:
    case PPC_DIVD:
    case PPC_DIVDU:
    case PPC_DIVW:
    case PPC_DIVWU:
    case PPC_MULHD:
    case PPC_MULHDU:
    case PPC_MULHW:
    case PPC_MULHWU:
    case PPC_MULLD:
    case PPC_MULLW:
    case PPC_SUBF:
    case PPC_SUBFC:
    case PPC_SUBFE:
      /* Instruction format:
         OPCODE | D | RA | RB | Oe | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_ADDME:
    case PPC_ADDZE:
    case PPC_NEG:
    case PPC_SUBFME:
    case PPC_SUBFZE:
      /* Instruction format:
         OPCODE | D | RA | Oe | Rc
       */
      PPC_ASM_SET_GPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins));
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleA{{{*/
void
PpcAssembleA(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
  {
    PPC_BITFIELD_SET(*instr, 31);
  }
  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_FADD:
    case PPC_FADDS:
    case PPC_FSUB:
    case PPC_FSUBS:
    case PPC_FDIV:
    case PPC_FDIVS:
      /* Instruction format :
       * OPCODE | RT | RA | RB | constant
       */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGB(ins));
      break;
    case PPC_FMADD:
    case PPC_FMADDS:
    case PPC_FMSUB:
    case PPC_FMSUBS:
    case PPC_FNMADD:
    case PPC_FNMADDS:
    case PPC_FNMSUB:
    case PPC_FNMSUBS:
    case PPC_FSEL:
      /* Instruction format :
       * OPCODE | RT | RA | RB | RC | constant
       */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGB(ins));
      PPC_ASM_SET_FPR(*instr, 21, PPC_INS_REGC(ins));
      break;
    case PPC_FMUL:
    case PPC_FMULS:
      /* Instruction format :
       * OPCODE | RT | RA | constant | RB | constant
       */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 11, PPC_INS_REGA(ins));
      PPC_ASM_SET_FPR(*instr, 21, PPC_INS_REGB(ins));
      break;
    case PPC_FRES:
    case PPC_FRSQRTE:
    case PPC_FSQRT:
    case PPC_FSQRTS:
      /* Instruction format :
       * OPCODE | RT | constant | RA | constant
       */
      PPC_ASM_SET_FPR(*instr,  6, PPC_INS_REGT(ins));
      PPC_ASM_SET_FPR(*instr, 16, PPC_INS_REGA(ins));
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleM{{{*/
void
PpcAssembleM(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
  {
    PPC_BITFIELD_SET(*instr, 31);
  }
  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_RLWIMI:
    case PPC_RLWINM:
      /* Instruction format:
         OPCODE | RA  | RT | shift - mask bit - mask bit | Rc
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 15, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    case PPC_RLWNM:
      /* Instruction format:
         OPCODE | RA  | RT | RB |  mask bit - mask bit | Rc
       */
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      PPC_BITFIELD_SET_VAL(*instr, 21, 10, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    default:
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleMD{{{*/
void
PpcAssembleMD(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR0))
  {
    PPC_BITFIELD_SET(*instr, 31);
  }
  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_RLDIC:
    case PPC_RLDICL:
    case PPC_RLDICR:
    case PPC_RLDIMI:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_BITFIELD_SET_VAL(*instr, 16, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) & 0x0000001f);
      PPC_BITFIELD_SET_VAL(*instr, 30, 1, (AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)) & 0x00000020)>>5);
      PPC_BITFIELD_SET_VAL(*instr, 21, 5, PPC_INS_MASK(ins) & 0x0000001f);
      PPC_BITFIELD_SET_VAL(*instr, 26, 1, (PPC_INS_MASK(ins) & 0x00000020)>>5);
      break;
    default:
    FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleMDS{{{*/
void
PpcAssembleMDS(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR1))
  {
    PPC_BITFIELD_SET(*instr, 31);
  }
  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_RLDCL:
    case PPC_RLDCR:
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGA(ins));
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGT(ins));
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins));
      PPC_BITFIELD_SET_VAL(*instr, 21, 6, PPC_INS_MASK(ins));
    break;
    default:
    FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleUnknown{{{*/
void
PpcAssembleUnknown(t_ppc_ins * ins, t_uint32 * instr)
{
  FATAL(("%s : Implement @I", __func__, ins));
}
/*}}}*/

/* PpcAssembleData {{{ */
/*! Assemble a piece of data. */
void
PpcAssembleData (t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = (t_uint32)(AddressExtractUint64 (PPC_INS_IMMEDIATE (ins)) & 0xffffffff);
}
/* }}} */

#ifdef PPC_ALTIVEC_SUPPORT
/* PpcAssembleAltivecVXA{{{*/
void
PpcAssembleAltivecVXA(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_VMHADDSHS:
    case PPC_VMHRADDSHS:
    case PPC_VMSUMUHS:
    case PPC_VMSUMSHS:
    case PPC_VMLADDUHM:
    case PPC_VMSUMUBM:
    case PPC_VMSUMMBM:
    case PPC_VMSUMUHM:
    case PPC_VMSUMSHM:
    case PPC_VSEL:
    case PPC_VPERM:
    case PPC_VMADDFP:
    case PPC_VNMSUBFP:
      /* Instruction format :
         OPCODE | RT | RA | RB | RC 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_VR(*instr, 11, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGB(ins)); 
      PPC_ASM_SET_VR(*instr, 21, PPC_INS_REGC(ins )); 
      break;
    case PPC_VSLDOI:
      /* Instruction format :
         OPCODE | RT | RA | RB | shift 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_VR(*instr, 11, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGB(ins)); 
      PPC_BITFIELD_SET_VAL(*instr, 22, 4, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    default:   
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleAltivecVX{{{*/
void
PpcAssembleAltivecVX(t_ppc_ins * ins, t_uint32 * instr)
{

  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_VADDUBS:
    case PPC_VADDUHS:
    case PPC_VADDUWS:
    case PPC_VADDSBS:
    case PPC_VADDSHS:
    case PPC_VADDSWS:
    case PPC_VSUBUBS:
    case PPC_VSUBUHS:
    case PPC_VSUBUWS:
    case PPC_VSUBSBS:
    case PPC_VSUBSHS:
    case PPC_VSUBSWS:
    case PPC_VSUM4UBS:
    case PPC_VSUM4SBS:
    case PPC_VSUM4SHS:
    case PPC_VSUM2SWS:
    case PPC_VSUMSWS:
    case PPC_VPKUHUS:
    case PPC_VPKUWUS:
    case PPC_VPKSHUS:
    case PPC_VPKSWUS:
    case PPC_VPKSHSS:
    case PPC_VPKSWSS:
    case PPC_VADDUBM:
    case PPC_VADDUHM:
    case PPC_VADDUWM:
    case PPC_VADDCUW:
    case PPC_VSUBUBM:
    case PPC_VSUBUHM:
    case PPC_VSUBUWM:
    case PPC_VSUBCUW:
    case PPC_VMAXUB:
    case PPC_VMAXUH:
    case PPC_VMAXUW:
    case PPC_VMAXSB:
    case PPC_VMAXSH:
    case PPC_VMAXSW:
    case PPC_VMINUB:
    case PPC_VMINUH:
    case PPC_VMINUW:
    case PPC_VMINSB:
    case PPC_VMINSH:
    case PPC_VMINSW:
    case PPC_VAVGUB:
    case PPC_VAVGUH:
    case PPC_VAVGUW:
    case PPC_VAVGSB:
    case PPC_VAVGSH:
    case PPC_VAVGSW:
    case PPC_VRLB:
    case PPC_VRLH:
    case PPC_VRLW:
    case PPC_VSLB:
    case PPC_VSLH:
    case PPC_VSLW:
    case PPC_VSL:
    case PPC_VSRB:
    case PPC_VSRH:
    case PPC_VSRW:
    case PPC_VSR:
    case PPC_VSRAB:
    case PPC_VSRAH:
    case PPC_VSRAW:
    case PPC_VAND:
    case PPC_VANDC:
    case PPC_VOR:
    case PPC_VNOR:
    case PPC_VMULOUB:
    case PPC_VMULOUH:
    case PPC_VMULOSB:
    case PPC_VMULOSH:
    case PPC_VMULEUB:
    case PPC_VMULEUH:
    case PPC_VMULESB:
    case PPC_VMULESH:
    case PPC_VADDFP:
    case PPC_VSUBFP:
    case PPC_VMAXFP:
    case PPC_VMINFP:
    case PPC_VMRGHB:
    case PPC_VMRGHH:
    case PPC_VMRGHW:
    case PPC_VMRGLB:
    case PPC_VMRGLH:
    case PPC_VMRGLW:
    case PPC_VSLO:
    case PPC_VSRO:
    case PPC_VPKUHUM:
    case PPC_VPKUWUM:
    case PPC_VPKPX:
    case PPC_VXOR:
      /* Instruction format :
         OPCODE | RT | RA | RB 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_VR(*instr, 11, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGB(ins)); 
      break;
    case PPC_MFVSCR:
      /* Instruction format :
         OPCODE | RT | 0 | 0
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      break;
    case PPC_MTVSCR:
      /* Instruction format :
         OPCODE | 0 | 0 | RA 
       */
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGA(ins)); 
      break;
    case PPC_VREFP:
    case PPC_VRSQRTEFP:
    case PPC_VEXPTEFP:
    case PPC_VLOGEFP:
    case PPC_VRFIN:
    case PPC_VRFIZ:
    case PPC_VRFIP:
    case PPC_VRFIM:
    case PPC_VUPKHSB:
    case PPC_VUPKHSH:
    case PPC_VUPKLSB:
    case PPC_VUPKLSH:
    case PPC_VUPKHPX:
    case PPC_VUPKLPX:
      /* Instruction format :
         OPCODE | RT | 0 | RA 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGA(ins)); 
      break;
    case PPC_VCTUXS:
    case PPC_VCTSXS:
    case PPC_VCFUX:
    case PPC_VCFSX:
    case PPC_VSPLTH:
    case PPC_VSPLTW:
    case PPC_VSPLTB:
      /* Instruction format :
         OPCODE | RT | unsigned immediate | RA 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_BITFIELD_SET_VAL(*instr, 11, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGA(ins)); 
      break;
    case PPC_VSPLTISB:
    case PPC_VSPLTISH:
    case PPC_VSPLTISW:
      /* Instruction format :
         OPCODE | RT | signed immediate | 0  
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_BITFIELD_SET_VAL(*instr, 11, 5, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      break;
    default:   
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleAltivecVXR{{{*/
void
PpcAssembleAltivecVXR(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_VCMPBFP:
    case PPC_VCMPEQFP:
    case PPC_VCMPEQUB:
    case PPC_VCMPEQUH:
    case PPC_VCMPEQUW:
    case PPC_VCMPGEFP:
    case PPC_VCMPGTFP:
    case PPC_VCMPGTSB:
    case PPC_VCMPGTSH:
    case PPC_VCMPGTSW:
    case PPC_VCMPGTUB:
    case PPC_VCMPGTUH:
    case PPC_VCMPGTUW:
      /* Instruction format :
         OPCODE | RT | RA | RB | Rc 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_VR(*instr, 11, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_VR(*instr, 16, PPC_INS_REGB(ins)); 
      if (PPC_SR_MARKED_VAL(PPC_INS_SREGS(ins), CR, CR6))
      {
        PPC_BITFIELD_SET(*instr, 21);
      }
      break;
    default:   
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement @I", __func__, ins));
  }
}

/*}}}*/

/* PpcAssembleAltivecX{{{*/
void
PpcAssembleAltivecX(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_LVEBX:
    case PPC_LVEHX:
    case PPC_LVEWX:
    case PPC_LVSL:
    case PPC_LVSR:
    case PPC_LVX:
    case PPC_LVXL:
      /* Instruction format :
         OPCODE | Vector - RT | General Purpose RA | General Purpose RB 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGT(ins));   
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGB(ins)); 
      break;
    case PPC_STVEBX:
    case PPC_STVEHX:
    case PPC_STVEWX:
    case PPC_STVX:
    case PPC_STVXL:
      /* Instruction format :
         OPCODE | Vector - RA | General Purpose RB | General Purpose RC 
       */
      PPC_ASM_SET_VR(*instr, 6, PPC_INS_REGA(ins));   
      PPC_ASM_SET_GPR(*instr, 11, PPC_INS_REGB(ins)); 
      PPC_ASM_SET_GPR(*instr, 16, PPC_INS_REGC(ins)); 
      break;
    default:   
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement @I", __func__, ins));
  }
}
/*}}}*/

/* PpcAssembleAltivecXDSS{{{*/
void
PpcAssembleAltivecXDSS(t_ppc_ins * ins, t_uint32 * instr)
{
  *instr = ppc_opcode_table[PPC_INS_OPCODE(ins)].opcode;

  switch(PPC_INS_OPCODE(ins))
  {
    case PPC_DST:
    case PPC_DSTT:
    case PPC_DSTST:
    case PPC_DSTSTT:
    case PPC_DSS:
    case PPC_DSSALL:
      /* Instruction format :
         OPCODE | Strm | General Purpose RA | General Purpose RB 
       */
      PPC_BITFIELD_SET_VAL(*instr, 9, 2, AddressExtractUint64 (PPC_INS_IMMEDIATE(ins)));
      PPC_ASM_SET_GPR(*instr, 1, PPC_INS_REGA(ins)); 
      PPC_ASM_SET_GPR(*instr, 6, PPC_INS_REGB(ins)); 
      break;
    default:   
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement @I", __func__, ins));
  }
}

/*}}}*/

/* PpcAssembleAltivecUnknown{{{*/
void
PpcAssembleAltivecUnknown(t_ppc_ins * ins, t_uint32 * instr)
{
  FATAL(("%s : Implement @I", __func__, ins));
}
/*}}}*/
#endif
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
