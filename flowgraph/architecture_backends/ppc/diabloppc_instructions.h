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

#ifndef PPC_INSTRUCTION_H
#define PPC_INSTRUCTION_H

/* some convenience macros */
#define PPC_INS_OBJECT(ins) 		CFG_OBJECT(PPC_INS_CFG(ins))
#define PPC_INS_IS_CONDITIONAL(x)	(PPC_INS_ATTRIB(x) & IF_CONDITIONAL)
#define BBL_FOREACH_PPC_INS(bbl,ins) 	for(ins=T_PPC_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=PPC_INS_INEXT(ins))
#define BBL_FOREACH_PPC_INS_R(bbl,ins)	for(ins=T_PPC_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=PPC_INS_IPREV(ins))
#define BBL_FOREACH_PPC_INS_SAFE(bbl,ins,tmp)   for(ins=T_PPC_INS(BBL_INS_FIRST(bbl)), tmp=ins?PPC_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?PPC_INS_INEXT(ins):0)
#define SECTION_FOREACH_PPC_INS(code,ins) for(ins=T_PPC_INS(SECTION_DATA(code)); ins!=NULL; ins=PPC_INS_INEXT(ins))

#define T_PPC_INS(ppc_ins)            ((t_ppc_ins *) ppc_ins)

/* Bitfield manipulators (32 bit) {{{ */
/*! Get a field form a bitfield, given a mask to filter the value */
#define PPC_BITFIELD2(bf, start, mask, size)            (((bf) & ( (mask) << (32 - (start) - (size)) )) >> (32 - (start) - (size)))
/*! Get a field form a bitfield */
#define PPC_BITFIELD(bf, start, size)                   (PPC_BITFIELD2(bf, start, ((0x1 << (size))- 1), size))

#define __PPC_BITFIELD_POS(val, start, size)            ((val) << (32 - (start) - (size)))
/*! Set the value of a field on a bitfield */
#define PPC_BITFIELD_SET_VAL(bf, start, size, val)                              \
  do {                                                                          \
    /* reset field to zero */                                                   \
    bf &= ~__PPC_BITFIELD_POS(((t_uint64)0x1 << (size)) - 1, start, size);                  \
    /* put the new value into the field */                                      \
    bf |= __PPC_BITFIELD_POS(( (((t_uint64)0x1 << size) - 1) & (val) ), start, size);       \
  } while(0)
/*! Set a bit on a bitfield */
#define PPC_BITFIELD_SET(bf, start)                     PPC_BITFIELD_SET_VAL(bf, start, 1, 1)

#define PPC_INS_SH(x)         (PPC_BITFIELD(PPC_INS_IMMEDIATE(x),0,5)) 
#define PPC_INS_MB(x)         (PPC_BITFIELD(PPC_INS_IMMEDIATE(x),5,5)) 
#define PPC_INS_ME(x)         (PPC_BITFIELD(PPC_INS_IMMEDIATE(x),10,5)) 
                               
/* }}} */

/* Addition macros for special encodings {{{*/
#define PPC_IMM_SH(ins)         ((AddressExtractUint32(PPC_INS_IMMEDIATE(ins))>>10)&0x0000001f)
#define PPC_IMM_MB(ins)         ((AddressExtractUint32(PPC_INS_IMMEDIATE(ins))>>5)&0x0000001f)
#define PPC_IMM_ME(ins)         (AddressExtractUint32(PPC_INS_IMMEDIATE(ins))&0x0000001f)
/*}}}*/

#define PPC_FL_SIGNED       0x1     /* to know if the inmediate operant is signed */
#define PPC_FL_ABSOLUTE     0x2     /* to know if the address provided is absolute */
#define PPC_FL_LINK         0x4     /* writes to Link Register */
#define PPC_FL_L            0x8     /* L bit - field set */
#define PPC_FL_CTR          0x10    /* Some branches update de Counter Register */ 
#define PPC_FL_IMM_SHIFT16  0x20    /* to know the immediate is shifted 16 bits */
#define PPC_FL_WITH_UPDATE  0x40    /* to know if the load or store is with update. Some 
                                        instructions allow to load/store a value from/to
                                        an effective address (EA) calculated and also write
                                        this EA to another one. */
#define PPC_FL_RELOCATED    0x80    /* To know if the immediate value is relocated */
#define PPC_FL_GOT_PRODUCER 0x100   /* These instructions branch and link to the got section 
                                        to get the address of the got table */

#define PPC_FL_CR_LOGIC     0x200   /* Conditional Register Logical Instructions 
                                        These instruccions have bits of the CR as
                                        sources. */
#define PPC_FL_EXTENDED     0x400   /* Extended arithmetic instructions use the carry
                                        as a source operand */
#define PPC_FL_RC           0x800   /* Bit Rc set */
#define PPC_FL_OE           0x1000  /* Bit Oe set */
#define PPC_FL_MULTIPLE     0x2000  /* Some operations use multiple registers as
                                        source or target */
#define PPC_FL_IMM          0x4000  /* Instruction has an immediate operand */

#endif


#ifdef DIABLOPPC_FUNCTIONS
#ifndef PPC_INSTRUCTION_FUNCTIONS
#define PPC_INSTRUCTION_FUNCTIONS
/* function declarations for the corresponding .c file go here */
t_bool PpcInsHasSideEffect(t_ins * ins);
t_bool PpcInsIsLoad(t_ins * ins);
t_bool PpcInsIsStore(t_ins * ins);
t_bool PpcInsIsProcedureCall(t_ins * ins);
t_bool PpcInsIsUnconditionalBranch(t_ins * ins);
t_bool PpcInsIsControlTransfer(t_ins * ins);
t_bool PpcInsIsSystemInstruction(t_ins * ins);
t_tristate PpcInsIsSyscallExit(t_ins * ins);
t_bool PpcInsAreIdentical(t_ins * ins1, t_ins * ins2);
t_uint32 PpcInsGetSyscallNumber(t_ins * ins);
t_bool PpcInsUnconditionalize(t_ins * ins);
t_bool PpcInsIsIndirectCall(t_ins * ins);
void PpcInsMakeNop (t_ins *ins);
void PpcInsMakeUncondBranch(t_ins *gins);
void PpcInsMakeBlr(t_ppc_ins *ins);
void PpcInsMakeMtspr(t_ppc_ins *ins, t_reg spr, t_reg src);
void PpcInsMakeMfspr(t_ppc_ins *ins, t_reg dest, t_reg spr);
void PpcInsMakeCall(t_ppc_ins *ins);
void PpcInsMakeLi(t_ppc_ins *ins, t_reg dst, t_int16 value);
void PpcInsMakeAddi(t_ppc_ins *ins, t_reg dst, t_reg src, t_int16 value);
void PpcInsMakeAddis(t_ppc_ins *ins, t_reg dst, t_reg src, t_int16 value);
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
