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


#include <diabloppc.h>

#define PPC_IMM(instr) PPC_BITFIELD(instr, 16, 16)

/* TODO: some instructions if Ra or Rb is 0 it means value zero, not
   the register, so we could put R0 as PPC_REG_NONE, o better catch these
   cases in the RegsUse function. Put a flag here, and then check at 
   the REG_USE...*/

/* TODO: check generic t_ins ATTRIB and TYPE values */

/* TODO: check L flag */ 

/* TODO: check all SR_MARK and change to SR_SET */

/*!
 * Disassemble ppc instructions
 *
 * All the opetarions defined have the same parameters.
 * The disassemble operations are splitted by the
 * instruction format defined by the PPC architecture.
 *
 * \param ins The ppc instruction that gets filled in
 * \param instr The encoded ppc instruction
 * \param opc The opcode
 *
 * \return void
 */


/* PpcDisassembleD {{{ */
void
PpcDisassembleD(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_ADDIC_DOT:
      PPC_SR_MARK_VAL(sregs, CR, CR0);
    case PPC_ADDIC:
    case PPC_SUBFIC:
      PPC_SR_MARK(sregs, XER, CA);
    case PPC_ADDI:
    case PPC_MULLI:
      if(opc!=PPC_MULLI)
      {
        PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      }
      else
      {
        PPC_INS_SET_TYPE(ins, IT_MUL);
      }
      /* Instruction format :
         OPCODE | RT | RA | Signed integer
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_IMM(instr)), 15));
      flags |= PPC_FL_SIGNED|PPC_FL_IMM;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      break;
    case PPC_ADDIS:
      /* Instruction format :
         OPCODE | RT | RA | Signed integer
         Shifted form
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)<<16));
      flags |= PPC_FL_SIGNED | PPC_FL_IMM_SHIFT16 | PPC_FL_IMM;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      break;
    case PPC_ANDI_DOT:
      PPC_SR_MARK_VAL(sregs, CR, CR0);
    case PPC_ORI:
    case PPC_XORI:
      /* Instruction format :
         OPCODE | RA | RT | Unsigned integer
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      break;
    case PPC_ANDIS_DOT:
      PPC_SR_MARK_VAL(sregs, CR, CR0);
    case PPC_ORIS:
    case PPC_XORIS:
      /* Instruction format :
         OPCODE | RA | RT | Unsigned integer
         Shifted form
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)<<16));
      flags |= PPC_FL_IMM_SHIFT16 | PPC_FL_IMM;
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      break;
    case PPC_LFDU:
    case PPC_LFSU:
      flags |= PPC_FL_WITH_UPDATE;
    case PPC_LFD:
    case PPC_LFS:
      /* Instruction format :
         OPCODE | RT | RA | C2-integer-sign-extended
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_IMM(instr)), 15));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGT(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      flags |= PPC_FL_SIGNED;
      break;
    case PPC_LHAU:
    case PPC_LHZU:
    case PPC_LWZU:
    case PPC_LBZU:
    case PPC_LDU:
      flags |= PPC_FL_WITH_UPDATE;
    case PPC_LBZ:
    case PPC_LD:
    case PPC_LHA:
    case PPC_LHZ:
    case PPC_LWA:
    case PPC_LWZ:
      /* Instruction format :
         OPCODE | RT | RA | C2-integer-sign-extended
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_IMM(instr)), 15));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      if(opc == PPC_LWA || opc == PPC_LD || opc == PPC_LDU)
      {
        PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend(AddressAnd (PPC_INS_IMMEDIATE(ins), AddressNewForIns (T_INS(ins), 0xfffc)),15));
      }
      flags |= PPC_FL_SIGNED | PPC_FL_IMM;
      break;
    case PPC_STBU:
    case PPC_STHU:
    case PPC_STWU:
    case PPC_STDU:
      flags |= PPC_FL_WITH_UPDATE;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
    case PPC_STB:
    case PPC_STH:
    case PPC_STW:
    case PPC_STD:
      /* Instruction format :
         OPCODE | RA | RB | C2-integer-sign-extended
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_IMM(instr)), 15));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_STORE);
      flags |= PPC_FL_SIGNED | PPC_FL_IMM;
      if(opc == PPC_STD || opc == PPC_STDU)
      {
        PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend(AddressAnd (PPC_INS_IMMEDIATE(ins), AddressNewForIns (T_INS(ins), 0xfffc)),15));
      }
      break;
    case PPC_STFDU:
    case PPC_STFSU:
      flags |= PPC_FL_WITH_UPDATE;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
    case PPC_STFD:
    case PPC_STFS:
      /* Instruction format :
         OPCODE | RA | RB | C2-integer-sign-extended
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_IMM(instr)), 15));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_STORE);
      flags |= PPC_FL_SIGNED | PPC_FL_IMM;
      break;
    case PPC_TWI:
    case PPC_TDI:
      /* Instruction format :
         OPCODE | TO | RA | signed-integer
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)));
      flags |= PPC_FL_SIGNED | PPC_FL_IMM;
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_CT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_ATTRIB(ins,(PPC_INS_ATTRIB(ins)|IF_CONDITIONAL));
      PPC_INS_SET_TYPE(ins,IT_SWI);
      break;
    case PPC_CMPI:
      flags |= PPC_FL_SIGNED;
    case PPC_CMPLI:
      /* Instruction format :
         OPCODE | BF | / | L | RA | integer (signed or unsigned)
         The L field is not used in 32 bits pcc
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGT(ins, PPC_CR(instr, 6));
      if (PPC_BITFIELD(instr, 10, 1))
        flags|=PPC_FL_L;
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * PPC_CR(instr, 6), 4);
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      if(PPC_BITFIELD(instr,10,1))
      {
        flags |= PPC_FL_L;
      }
      break;
    case PPC_LMW:

      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)));
      PPC_INS_SET_TYPE(ins,IT_LOAD);
      flags |= PPC_FL_MULTIPLE | PPC_FL_IMM;
      break;
    case PPC_STMW:

      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_IMM(instr)));
      PPC_INS_SET_TYPE(ins,IT_STORE);
      flags |= PPC_FL_MULTIPLE | PPC_FL_IMM;
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleX{{{*/
void
PpcDisassembleX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_ECIWX:
    case PPC_LBZUX:
    case PPC_LHAUX:
    case PPC_LWAUX:
    case PPC_LHZUX:
    case PPC_LWZUX:
    case PPC_LDUX:
      flags |= PPC_FL_WITH_UPDATE;
    case PPC_LBZX:
    case PPC_LHAX:
    case PPC_LHBRX:
    case PPC_LHZX:
    case PPC_LWARX:
    case PPC_LWAX:
    case PPC_LDARX:
    case PPC_LWBRX:
    case PPC_LWZX:
    case PPC_LDX:
      /* Instruction format:
         OPCODE | RT | RA | RB
       */
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      break;
    case PPC_LFDUX:
    case PPC_LFSUX:
      flags |= PPC_FL_WITH_UPDATE;
    case PPC_LFDX:
    case PPC_LFSX:
      /* Instruction format:
         OPCODE | RT  | RA | RB
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr,6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_FLT_LOAD);
      break;
    case PPC_FRSP:
      PPC_SR_MARK(sregs, FPSCR, OX);
      PPC_SR_MARK(sregs, FPSCR, UX);
    case PPC_FCTIW:
    case PPC_FCTID:
    case PPC_FCTIWZ:
    case PPC_FCTIDZ:
      PPC_SR_MARK(sregs, FPSCR, VXSNAN);
    case PPC_FCFID:
      PPC_SR_MARK_VAL(sregs, FPSCR, FPRF);
      PPC_SR_MARK(sregs, FPSCR, FR);
      PPC_SR_MARK(sregs, FPSCR, FI);
      PPC_SR_MARK(sregs, FPSCR, FX);
      PPC_SR_MARK(sregs, FPSCR, XX);
      if(opc != PPC_FRSP && opc != PPC_FCFID)
      {
        PPC_SR_MARK(sregs, FPSCR, VXCVI);
      }
    case PPC_FMR:
    case PPC_FABS:
    case PPC_FNABS:
    case PPC_FNEG:
      /* Instruction format:
         OPCODE | RT  | zeros | RA | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr,6));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr,16));
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR1);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins, IT_FLT_ALU);
      break;
    case PPC_MFFS:
      /* Instruction format:
         OPCODE | RT  | zeros | zeros | Rc
       */
      PPC_INS_SET_TYPE(ins, IT_FLT_STATUS);
      PPC_INS_SET_REGT(ins, PPC_FPR(instr,6));
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR1);
        flags |= PPC_FL_RC;
      }
      break;  
    case PPC_MFCR:
      /* Instruction format:
         OPCODE | RT | zeros | zeros | Rc
       */
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
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
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    case PPC_STBUX:
    case PPC_STHUX:
    case PPC_STWUX:
    case PPC_STDUX:
      flags |= PPC_FL_WITH_UPDATE;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
    case PPC_STWCX_DOT:
    case PPC_STDCX_DOT:
      if ((opc == PPC_STWCX_DOT) ||
          (opc == PPC_STDCX_DOT))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
      }
    case PPC_STBX:
    case PPC_STHX:
    case PPC_STWX:
    case PPC_STDX:
    case PPC_STHBRX:
    case PPC_STWBRX:
    case PPC_STSWX:
      /* Instruction format:
         OPCODE | RA | RB | RC | /
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGC(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_STORE);
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
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    case PPC_STFDUX:
    case PPC_STFSUX:
      flags |= PPC_FL_WITH_UPDATE;
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
    case PPC_STFDX:
    case PPC_STFSX:
    case PPC_STFIWX:   
      /* Instruction format:
         OPCODE | RA | RB | RC
       */
      PPC_INS_SET_REGA(ins, PPC_FPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGC(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_FLT_STORE);
      break;
    case PPC_ECOWX:
      /* Instruction format:
         OPCODE | RA | RB | RC
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGC(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_STORE);
      break;
    case PPC_CNTLZD:
    case PPC_CNTLZW:
    case PPC_EXTSH:
    case PPC_EXTSB:
    case PPC_EXTSW:
      /* Instruction format:
         OPCODE | RA | RT | Rc
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      break;
    case PPC_SRAWI:
      /* Instruction format:
         OPCODE | RA | RT | Shift amount | Rc
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,16,5)));
      flags |= PPC_FL_IMM;
      PPC_SR_MARK(sregs, XER, CA);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_SRADI:
      /* Instruction format:
         OPCODE | RA | RT | Shift amount | Rc
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,11));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), (PPC_BITFIELD(instr,30,1)<<5)|PPC_BITFIELD(instr,16,5)));
      flags |= PPC_FL_IMM;
      PPC_SR_MARK(sregs, XER, CA);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_CMP:
    case PPC_CMPL:
      /* Instruction format:
         OPCODE | CR | L | RA | RB
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_REGT(ins, PPC_CR(instr,6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 10, 1)));
      flags |= PPC_FL_IMM;
      if (PPC_BITFIELD(instr, 10, 1))
        flags |= PPC_FL_L;
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * PPC_CR(instr, 6), 4);
      break;
    case PPC_FCMPO:
      PPC_SR_MARK(sregs, FPSCR, VXVC);
    case PPC_FCMPU:
      /* Instruction format:
         OPCODE | CR | RA | RB
       */
      PPC_INS_SET_REGA(ins, PPC_FPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_FPR(instr,16));
      PPC_INS_SET_REGT(ins, PPC_CR(instr,6));
      PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * PPC_CR(instr, 6), 4);
      PPC_SR_MARK_VAL(sregs, FPSCR, FPCC);
      PPC_SR_MARK(sregs, FPSCR, FX);
      PPC_SR_MARK(sregs, FPSCR, VXSNAN);
      PPC_INS_SET_TYPE(ins,IT_FLT_ALU);
      break;
    case PPC_MCRFS:
      /* Instruction format:
         OPCODE | CR-dest | FSCR-orig-field
       */
      PPC_INS_SET_REGA(ins, PPC_REG_FPSCR);
      PPC_INS_SET_REGT(ins, PPC_CR(instr, 6));
      PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * PPC_CR(instr, 6), 4);
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_CR(instr, 11)));
      flags |= PPC_FL_IMM;
      switch (AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)))
      {
        case 0:
          PPC_SR_MARK(sregs, FPSCR, FX);
          PPC_SR_MARK(sregs, FPSCR, OX);
          break;
        case 1:
          PPC_SR_MARK(sregs, FPSCR, UX);
          PPC_SR_MARK(sregs, FPSCR, ZX);
          PPC_SR_MARK(sregs, FPSCR, XX);
          PPC_SR_MARK(sregs, FPSCR, VXSNAN);
          break;
        case 2:
          PPC_SR_MARK(sregs, FPSCR, VXISI);
          PPC_SR_MARK(sregs, FPSCR, VXIDI);
          PPC_SR_MARK(sregs, FPSCR, VXZDZ);
          PPC_SR_MARK(sregs, FPSCR, VXIMZ);
          break;
        case 3:
          PPC_SR_MARK(sregs, FPSCR, VXVC);
          break;
        case 5:
          PPC_SR_MARK(sregs, FPSCR, VXSOFT);
          PPC_SR_MARK(sregs, FPSCR, VXSQRT);
          PPC_SR_MARK(sregs, FPSCR, VXCVI);
          break;
        default:
          FATAL(("Interpreted an unknown CR field for %x (intended field %d)",
                 instr, PPC_BITFIELD(instr, 11, 3)));
          break;
      }
      PPC_INS_SET_TYPE(ins, IT_FLT_STATUS);
      break;
    case PPC_MTFSFI:
      /* Instruction format:
         OPCODE | BF | Immediate | Rc
       */
      {
        t_uint32 U = PPC_BITFIELD(instr, 16, 4);
        PPC_INS_SET_REGT(ins, PPC_REG_FPSCR);
        PPC_INS_SET_REGA(ins, PPC_CR(instr, 6));
        PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), U));
        flags |= PPC_FL_IMM;
        if (PPC_INS_REGA(ins) == 0)
        {
          PPC_SR_MARK_VAL_UNK(sregs, FPSCR, 1, 1);
          PPC_SR_MARK_VAL_UNK(sregs, FPSCR, 2, 1);
          PPC_SR_SET_VAL_UNK(sregs, FPSCR, PPC_INS_REGA(ins), 4, (U & 0x9));
        }
        else
        {
          PPC_SR_SET_VAL_UNK(sregs, FPSCR, PPC_INS_REGA(ins), 4, U);
        }
      }
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR1);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins, IT_FLT_STATUS);
      break;
    case PPC_EIEIO:
      /* Instruction format:
         OPCODE
       */
      PPC_INS_SET_TYPE(ins, IT_SYNC);
      break;
    case PPC_SYNC:
      /* Instruction format:
         OPCODE | L
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,9,2)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_TYPE(ins, IT_SYNC);
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
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_CACHE);
      break;
    case PPC_MTFSB1:
      PPC_SR_MARK(sregs, FPSCR, FX);
    case PPC_MTFSB0:
      /* Instruction format:
         OPCODE | bit | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_REG_FPSCR);
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 6, 5)));
      flags |= PPC_FL_IMM;
      PPC_SR_UNSET_UNK(sregs, FPSCR, PPC_BITFIELD(instr, 6, 5));
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR1);
        flags |= PPC_FL_RC;
      }
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_TW:
    case PPC_TD:
      /* Instruction format:
         OPCODE | TO | RA | RB
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_CT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_TYPE(ins, IT_SWI);
      break;

    case PPC_LSWI:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,16,5)));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      flags|=PPC_FL_MULTIPLE | PPC_FL_IMM;
      break;
    case PPC_LSWX:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      flags|=PPC_FL_MULTIPLE;
      break;

    case PPC_STSWI:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,16,5)));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      flags|=PPC_FL_MULTIPLE | PPC_FL_IMM;
      break;
    case PPC_MFSRIN:
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      break;
    case PPC_MTMSR:
    case PPC_MTMSRD:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_REG_MSR);
      if(PPC_BITFIELD(instr,15,1))
      {
        flags |= PPC_FL_L;
      }
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_MFSR:
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGA(ins, PPC_SRR(instr,12));
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_MTSR:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGT(ins, PPC_SRR(instr,12));
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_MTSRIN:
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_SLBIA:
    case PPC_TLBIA:
    case PPC_TLBSYNC:
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    case PPC_TLBIE:
      if(PPC_BITFIELD(instr,10,1))
      {
        flags |= PPC_FL_L;
      }
    case PPC_SLBIE:
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,16));
      break;
    case PPC_SLBMFEE:
    case PPC_SLBMFEV:
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,16));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      break;
    case PPC_SLBMTE:
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      PPC_INS_SET_REGA(ins, PPC_GPR(instr,6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr,16));
      break;
    case PPC_MFMSR:
      PPC_INS_SET_REGT(ins, PPC_GPR(instr,6));
      PPC_INS_SET_TYPE(ins, IT_STATUS);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleI{{{*/
void
PpcDisassembleI(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_B:
      /* Instruction format:
         OPCODE | LI | AA | LK
         where LI is signed integer in c2
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 6, 24)<<2),25));
      flags |= PPC_FL_IMM;
      if(I_AA(instr))
      {
        flags |= PPC_FL_ABSOLUTE;
      }
      if(I_LK(instr))
      {
        flags |= PPC_FL_LINK;
      }
      PPC_INS_SET_TYPE(ins,IT_BRANCH);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags );
}
/*}}}*/

/* PpcDisassembleB{{{*/
void
PpcDisassembleB(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_BC:
      /* Instruction format:
         OPCODE | BO | BI | BD | AA | LK
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 16, 14)<<2),15));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_BO(ins, PPC_BITFIELD(instr, 6, 5));
      PPC_INS_SET_CB(ins, PPC_BITFIELD(instr, 11, 5));
      /* BO = PPC_BOU means "branch always" */
      if (PPC_INS_BO(ins)!=PPC_BOU)
        PPC_INS_SET_ATTRIB(ins,(PPC_INS_ATTRIB(ins)|IF_CONDITIONAL));
      if(I_AA(instr))
      {
        flags |= PPC_FL_ABSOLUTE;
      }
      if(I_LK(instr))
      {
        flags |= PPC_FL_LINK;
      }
      if(I_CTR(PPC_INS_BO(ins)))
      {
        flags |= PPC_FL_CTR;
      }
      /* need to be set now already for PpcInsIsPicBcl to work */
      PPC_INS_SET_FLAGS(ins, flags );
      if (!PpcInsIsPicBcl(ins))
      {
        PPC_INS_SET_TYPE(ins,IT_BRANCH);
      }
      else
      {
        PPC_INS_SET_TYPE(ins,IT_CONSTS);
      }
     break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  /* uncomment if more cases are added above
   * PPC_INS_SET_FLAGS(ins, flags );
  */
}
/*}}}*/

/* PpcDisassembleSC{{{*/
void
PpcDisassembleSC(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_SC:
      /* Instruction format:
         OPCODE
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,20,6)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_TYPE(ins, IT_SWI);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleDS{{{*/
void
PpcDisassembleDS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  VERBOSE(0,("64 bits DS-From instruction"));
  FATAL(("%s : Implement %x\n", __func__, instr));
}
/*}}}*/

/* PpcDisassembleXL{{{*/
void
PpcDisassembleXL(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_BCCTR:
      PPC_INS_SET_REGA(ins,PPC_REG_CTR);
    case PPC_BCLR:
      if(opc!=PPC_BCCTR)
      {
        PPC_INS_SET_REGA(ins,PPC_REG_LR);
      }
      /* Instruction format:
         OPCODE | BO | BI | BH | LK
       */
      PPC_INS_SET_BO(ins, PPC_BITFIELD(instr, 6, 5));
      PPC_INS_SET_CB(ins, PPC_BITFIELD(instr, 11, 5));
      PPC_INS_SET_BH(ins, PPC_BITFIELD(instr, 19, 2));
      /* BO = PPC_BOU means "branch always" */
      if (PPC_INS_BO(ins)!=PPC_BOU)
        PPC_INS_SET_ATTRIB(ins,PPC_INS_ATTRIB(ins)|IF_CONDITIONAL);
      PPC_INS_SET_TYPE(ins,IT_BRANCH);
      if(I_LK(instr))
      {
        flags |= PPC_FL_LINK;
      }
      if(I_CTR(PPC_INS_BO(ins)))
      {
        flags |= PPC_FL_CTR;
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
      flags |= PPC_FL_CR_LOGIC;
      PPC_INS_SET_REGT(ins, PPC_BIT_CR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_BIT_CR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_BIT_CR(instr, 16));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_MCRF:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_INS_SET_REGA(ins, PPC_CR(instr, 11));
      PPC_INS_SET_REGT(ins, PPC_CR(instr, 6));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_ISYNC:
      /* Instruction format:
         OPCODE
       */
      PPC_INS_SET_TYPE(ins,IT_SYNC);
      break;
    case PPC_RFID:
      /* Instruction format:
         OPCODE
       */
      PPC_INS_SET_TYPE(ins,IT_SWI);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags );
}
/*}}}*/

/* PpcDisassembleXFX{{{*/
void
PpcDisassembleXFX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);

  switch(opc)
  {
    case PPC_MTSPR:
      /* Instruction format:
         OPCODE | RA | RT
       */
      PPC_INS_SET_REGT(ins, PPC_SPR(instr));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_MFSPR:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_INS_SET_REGA(ins, PPC_SPR(instr));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_MFTB:
      /* Instruction format:
         OPCODE | RT | RA
       */
      PPC_INS_SET_REGA(ins, PPC_TBR(instr));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      break;
    case PPC_MTCRF:
    case PPC_MTOCRF:
      /* Instruction format:
         OPCODE | RA | MASK
       */
      PPC_INS_SET_MASK(ins, PPC_BITFIELD(instr, 12, 8));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_TYPE(ins,IT_DATAPROC);
      {
        int i, n, count;
        n = 0;
        for(count = 0, i = 0;
            i < 8;
            i++)
        {
          /* bits are counted from left to right on the PowerPC ISA -> 7-i
           */
          if ((PPC_INS_MASK(ins) >> (7-i)) & 0x1)
          {
            n = i;
            count++;
            if (opc == PPC_MTCRF)
            {
              PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * n, 4);
            }
          }
        }

        if (opc == PPC_MTOCRF)
        {
          if (count == 1)
          {
            PPC_SR_MARK_VAL_UNK(sregs, CR, 4 * n, 4);
          }
          else
          {
            PPC_SR_MARK_VAL_UNK(sregs, CR, 0, 32);
          }
        }
      }
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
}
/*}}}*/

/* PpcDisassembleXFL{{{*/
void
PpcDisassembleXFL (t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_MTFSF:
      /* Instruction format:
         OPCODE | MASK | RA | Rc
       */
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 7, 8)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGT(ins, PPC_REG_FPSCR);
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 16));
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR1);
        flags |= PPC_FL_RC;
      }
      {
        t_uint32 aux;
        int i;
        for (aux = AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)) , i = 7;
             aux;
             aux >>= 1 , i--)
        {
          if (aux & 0x1)
          {
            PPC_SR_MARK_VAL_UNK(sregs, FPSCR, 4 * i, 4);
          }
        }
      }
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins,flags);
}
/*}}}*/

/* PpcDisassembleXS{{{*/
void
PpcDisassembleXS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  VERBOSE(0,("64 bits XS-From instruction"));
  FATAL(("%s : Implement %x\n", __func__, instr));
}
/*}}}*/

/* PpcDisassembleXO{{{*/
void
PpcDisassembleXO(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags = 0x00000000;
  t_uint32 *sregs = PPC_INS_SREGS(ins);

  switch(opc)
  {
    case PPC_DIVD:
    case PPC_DIVDU:
    case PPC_DIVW:
    case PPC_DIVWU:
      /* Instruction format:
         OPCODE | D | RA | RB | Oe | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_DIV);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      if(I_OE(instr))
      {
        PPC_SR_MARK(sregs, XER, OV);
        PPC_SR_MARK(sregs, XER, SO);
        flags |= PPC_FL_OE;  
      }
      break;
    case PPC_MULLD:
    case PPC_MULLW:
      if(I_OE(instr))
      {
        PPC_SR_MARK(sregs, XER, OV);
        PPC_SR_MARK(sregs, XER, SO);
        flags |= PPC_FL_OE;  
      }
    case PPC_MULHD:
    case PPC_MULHDU:
    case PPC_MULHW:
    case PPC_MULHWU:
      /* Instruction format:
         OPCODE | D | RA | RB | // | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_MUL);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    case PPC_SUBFE:
    case PPC_ADDE:
      flags |= PPC_FL_EXTENDED;
    case PPC_ADDC:
    case PPC_SUBFC:
      PPC_SR_MARK(sregs,XER,CA);
    case PPC_ADD:
    case PPC_SUBF:
      /* Instruction format:
         OPCODE | D | RA | RB | Oe | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      if(I_OE(instr))
      {
        PPC_SR_MARK(sregs, XER, OV);
        PPC_SR_MARK(sregs, XER, SO);
        flags |= PPC_FL_OE;  
      }
      break;
    case PPC_ADDME:
    case PPC_ADDZE:
    case PPC_SUBFME:
    case PPC_SUBFZE:
      flags |= PPC_FL_EXTENDED;
      PPC_SR_MARK(sregs, XER, CA);
    case PPC_NEG:
      /* Instruction format:
         OPCODE | D | RA | Oe | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      if(I_OE(instr))
      {
        PPC_SR_MARK(sregs, XER, OV);
        PPC_SR_MARK(sregs, XER, SO);
        flags |= PPC_FL_OE;  
      }
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleA{{{*/
void
PpcDisassembleA(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  /* set flags on special registers for everybody */
  if(I_RC(instr))
  {
    PPC_SR_MARK_VAL(sregs, CR, CR1);
    flags |= PPC_FL_RC;
  }
  if (opc == PPC_FSEL)
  {
    /* ugly, but it's an unstandard case */
    goto decode_fsel;
  }
  PPC_SR_MARK_VAL(sregs, FPSCR, FPRF);
  PPC_SR_MARK(sregs, FPSCR, FR);
  PPC_SR_MARK(sregs, FPSCR, FI);
  PPC_SR_MARK(sregs, FPSCR, FX);
  PPC_SR_MARK(sregs, FPSCR, OX);
  PPC_SR_MARK(sregs, FPSCR, UX);
  PPC_SR_MARK(sregs, FPSCR, VXSNAN);

  switch (opc)
  {
    case PPC_FADD:
    case PPC_FADDS:
    case PPC_FSUB:
    case PPC_FSUBS:
      PPC_SR_SET(sregs, FPSCR, VXISI);
      goto decode1;
    case PPC_FDIV:
    case PPC_FDIVS:
      PPC_SR_MARK(sregs, FPSCR, ZX);
      PPC_SR_MARK(sregs, FPSCR, VXIDI);
      PPC_SR_MARK(sregs, FPSCR, VXZDZ);
decode1:
      /* Instruction format :
       * OPCODE | RT | RA | RB | constant
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_FPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_FLT_ALU);
      break;
    case PPC_FMADD:
    case PPC_FMADDS:
    case PPC_FMSUB:
    case PPC_FMSUBS:
    case PPC_FNMADD:
    case PPC_FNMADDS:
    case PPC_FNMSUB:
    case PPC_FNMSUBS:
      PPC_SR_MARK(sregs, FPSCR, VXIMZ);
decode_fsel:
      /* Instruction format :
       * OPCODE | RT | RA | RB | RC | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_FPR(instr, 16));
      PPC_INS_SET_REGC(ins, PPC_FPR(instr, 21));
      PPC_INS_SET_TYPE(ins, IT_FLT_ALU);
      break;
    case PPC_FMUL:
    case PPC_FMULS:
      PPC_SR_MARK(sregs, FPSCR, VXIMZ);
      /* Instruction format :
       * OPCODE | RT | RA | /// | RB | Rc
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_FPR(instr, 21));
      PPC_INS_SET_TYPE(ins, IT_FLT_ALU);
      break;
    case PPC_FRES:
      PPC_SR_MARK(sregs, FPSCR, ZX);
    case PPC_FRSQRTE:
      goto decode2;
    case PPC_FSQRT:
    case PPC_FSQRTS:
      PPC_SR_MARK(sregs, FPSCR, XX);
decode2:
      PPC_SR_MARK(sregs, FPSCR, VXSQRT);
      /* undo default flags */
      PPC_SR_UNMARK(sregs, FPSCR, OX);
      PPC_SR_UNMARK(sregs, FPSCR, UX);
      /* Instruction format :
       * OPCODE | RT | constant | RA | constant
       */
      PPC_INS_SET_REGT(ins, PPC_FPR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_FPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_FLT_ALU);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins,flags);
}
/*}}}*/

/* PpcDisassembleM{{{*/
void
PpcDisassembleM(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_RLWIMI:
    case PPC_RLWINM:
      /* Instruction format:
         OPCODE | RA  | RT | shift - mask bit - mask bit | Rc
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 16, 15)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    case PPC_RLWNM:
      /* Instruction format:
         OPCODE | RA  | RT | RB |  mask bit - mask bit | Rc
       */
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,21 , 10)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags );
}
/*}}}*/

/* PpcDisassembleMD{{{*/
void
PpcDisassembleMD(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_RLDIC:
    case PPC_RLDICL:
    case PPC_RLDICR:
    case PPC_RLDIMI:
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), (PPC_BITFIELD(instr, 30, 1)<<5)|PPC_BITFIELD(instr, 16, 5)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_MASK(ins,(PPC_BITFIELD(instr, 26, 1)<<5)|PPC_BITFIELD(instr, 21, 5));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      } 
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags );
}
/*}}}*/

/* PpcDisassembleMDS{{{*/
void
PpcDisassembleMDS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_RLDCL:
    case PPC_RLDCR:
      PPC_INS_SET_REGT(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_MASK(ins,PPC_BITFIELD(instr, 21, 6));
      PPC_INS_SET_TYPE(ins, IT_DATAPROC);
      if(I_RC(instr))
      {
        PPC_SR_MARK_VAL(sregs, CR, CR0);
        flags |= PPC_FL_RC;
      }
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags );
}
/*}}}*/

/* PpcDisassembleData {{{ */
/*! Disassemble a piece of data intermixed in a code section.
 *
 * \param ins   The t_ins where the data is being disassembled into.
 * \param data0 The data itself.
 *
 * \return void
 */
void
PpcDisassembleData (t_ppc_ins *ins, t_uint32 data0, t_uint16 opc)
{
  PpcInsSet (ins, PPC_DATA);
  PPC_INS_SET_TYPE (ins, IT_DATA);
  PPC_INS_SET_ATTRIB (ins, IF_DATA);
  PPC_INS_SET_IMMEDIATE (ins, AddressSignExtend (AddressNewForIns (T_INS(ins), data0), 31));
}
/* }}} */

/* PpcDisassembleUnknown{{{*/
void
PpcDisassembleUnknown(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  FATAL(("%s : Implement %x at @G\n", __func__, instr, PPC_INS_CADDRESS(ins)));
}
/*}}}*/

#ifdef PPC_ALTIVEC_SUPPORT
/* PpcDisassembleAltivecVXA{{{*/
void
PpcDisassembleAltivecVXA(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
  {
    case PPC_VMHADDSHS:
    case PPC_VMHRADDSHS:
    case PPC_VMSUMUHS:
    case PPC_VMSUMSHS:
      PPC_SR_MARK(sregs,VSCR,SAT);
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
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_VR(instr, 16));
      PPC_INS_SET_REGC(ins, PPC_VR(instr, 21));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    case PPC_VSLDOI:
      /* Instruction format :
         OPCODE | RT | RA | RB | shift
       */
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_VR(instr, 16));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr,22,4)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleAltivecVX{{{*/
void
PpcDisassembleAltivecVX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
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
      PPC_SR_MARK(sregs, VSCR, SAT);
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
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_VR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    case PPC_MFVSCR:
      /* Instruction format :
         OPCODE | RT | 0 | 0
       */
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    case PPC_MTVSCR:
      /* Instruction format :
         OPCODE | 0 | 0 | RA
       */
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
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
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    case PPC_VCTUXS:
    case PPC_VCTSXS:
      PPC_SR_MARK(sregs, VSCR, SAT);
    case PPC_VCFUX:
    case PPC_VCFSX:
    case PPC_VSPLTH:
    case PPC_VSPLTW:
    case PPC_VSPLTB:
      /* Instruction format :
         OPCODE | RT | unsigned immediate | RA
       */
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 11,5)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      break;
    case PPC_VSPLTISB:
    case PPC_VSPLTISH:
    case PPC_VSPLTISW:
      /* Instruction format :
         OPCODE | RT | signed immediate | 0
       */
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 11,5)), 4));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      flags |= PPC_FL_IMM;
      break;
    default:
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}
/*}}}*/

/* PpcDisassembleAltivecVXR{{{*/
void
PpcDisassembleAltivecVXR(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 *sregs = PPC_INS_SREGS(ins);
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
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
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_VR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_SIMD);
      if(I_ALTIVEC_RC(instr)){
        PPC_SR_MARK_VAL(sregs, CR, CR6);
        flags |= PPC_FL_RC;
      }
      break;
    default:
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
}

/*}}}*/

/* PpcDisassembleAltivecX{{{*/
void
PpcDisassembleAltivecX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
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
      PPC_INS_SET_REGT(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_LOAD);
      break;
    case PPC_STVEBX:
    case PPC_STVEHX:
    case PPC_STVEWX:
    case PPC_STVX:
    case PPC_STVXL:
      /* Instruction format :
         OPCODE | Vector - RA | General Purpose RB | General Purpose RC
       */
      PPC_INS_SET_REGA(ins, PPC_VR(instr, 6));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGC(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_STORE);
      break;
    default:
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
}
/*}}}*/

/* PpcDisassembleAltivecXDSS{{{*/
void
PpcDisassembleAltivecXDSS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 flags;
  flags = 0x00000000;

  switch(opc)
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
      PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns (T_INS(ins), PPC_BITFIELD(instr, 9, 2)));
      flags |= PPC_FL_IMM;
      PPC_INS_SET_REGA(ins, PPC_GPR(instr, 11));
      PPC_INS_SET_REGB(ins, PPC_GPR(instr, 16));
      PPC_INS_SET_TYPE(ins, IT_PREF);
      break;
    default:
      printf("Is an Altivec Function!");
      FATAL(("%s : Implement %x\n", __func__, instr));
  }
  PPC_INS_SET_FLAGS(ins, flags);
}

/*}}}*/

/* PpcDisassembleAltivecUnknown{{{*/
void
PpcDisassembleAltivecUnknown(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc)
{
  // FATAL(("%s : Implement %x\n", __func__, instr));
  printf("Is an Altivec Function!");
}
/*}}}*/
#endif
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
