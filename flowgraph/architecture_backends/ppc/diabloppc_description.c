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
/* names {{{ */
static char * names[] =
{
	"r0",  	"r1", 	"r2",   "r3",   "r4",   "r5",   "r6",   "r7",  "r8",  /*GPR registers*/
        "r9",   "r10",  "r11",  "r12",  "r13",  "r14",  "r15",  "r16",   
        "r17",  "r18",  "r19",  "r20",  "r21",  "r22",  "r23",  "r24", 
	"r25",  "r26",  "r27",  "r28",  "r29",  "r30",  "r31",  /* 32 */
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",  "f8", /*FP registers*/ 
        "f9",	"f10",  "f11",  "f12",  "f13",  "f14",  "f15",  "f16",   
        "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",  "f24", 
	"f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",  /* 64 */
	"cr0",	"cr1",	"cr2",	"cr3",	"cr4",	"cr5",	"cr6",	"cr7", /*Condition registers */
	"LR",	"CTR",	/* Special registers */
        "XER",	
        "FPSCR",
	/* Add PowerPC Privileged Registers */
        "TBL","TBU", /* User mode - VEA - Time Base Register*/
        "MSR",/* Machine state register */
        "PVR",/* Processor Version Register */
        "IBAT0U","IBAT0L","IBAT1U","IBAT1L","IBAT2U","IBAT2L",/* Instruction BAT
                                                                 registers */
        "IBAT3U","IBAT3L",
        "DBAT0U","DBAT0L","DBAT1U","DBAT1L","DBAT2U","DBAT2L",/* Data BAT
                                                                 registers */
        "DBAT3U","DBAT3L", /* 96 */
        "SDR1", /* SDR1 register */
        "SR0","SR1","SR2","SR3","SR4","SR5","SR6","SR7", /* Segment Registers */
        "SR8","SR9","SR10","SR11","SR12","SR13","SR14","SR15",
        "DAR", /* Data Address Register */
        "DSISR", /* DSISR Register */
        "SPRG0","SPRG1","SPRG2","SPRG3", /* SPRGs */  
        "SRR0","SRR1" /* Save and Restore Registers */ /* 121 */
#ifdef PPC_ALTIVEC_SUPPORT
        /* Altivec Registers */
        ,"VRSAVE","VSCR", /* 123 */
        "VR0","VR1","VR2","VR3","VR4","VR5","VR6","VR7","VR8","VR9","VR10",
        "VR11","VR12","VR13","VR14","VR15","VR16","VR17","VR18","VR19","VR20",
        "VR21","VR22","VR23","VR24","VR25","VR26","VR27","VR28","VR29","VR30","VR31" /* 155 */
#endif  
};
/* }}} */

t_architecture_description ppc_description =
{
  /*! Size of an instruction: encoded minimal size */
   32, /* encoded minimal size [1 byte] */
  /*! Size of an instruction: encoded maximum size */
   32, /* encoded maximum size [15 byte] */
   32, /* mod size */
   1, /* bundle size */
   0, /* bundle template size */
   sizeof(t_ppc_ins), /* size of an instruction */
   ADDRSIZE32, /* ADDRSIZE64 for 64-bit architectures */
  /*! The number of general purpose integer registers */
   32,  /* int regs */
  /*! The number of general purpose floating point registers */
   32,  /* float regs */
  /*! The number of predicate registers */
   8,  /* predicate regs */
  /*! The number of branch registers */
   0,  /* branch regs */
  /*! The number of special registers */
   36, /* special regs */

/* register sets: this is an ugly mess that needs to be cleaned up ASAP
 * the values filled in here are those of the i386, they just serve as
 * an example */
#if MAX_REG_ITERATOR > 64
/* more than 64 registers: regsets are represented as a structure with an array of t_uint32 values */
/* all registers            */   {MAX_REG_ITERATOR, {0xffffffff,0xffffffff,0xffffffff,0x00000fff}},
/* int registers            */   {MAX_REG_ITERATOR, {0xffffffff}},
/* float registers          */   {MAX_REG_ITERATOR, {0x00000000,0xffffffff}},
/* predicate registers      */   {MAX_REG_ITERATOR, {0x00000000,0x00000000,0x000000ff}},
/* callee saved             */   {MAX_REG_ITERATOR, {0xffffc002,0xffffc000,0x0000001c}},

/* next is the set of registers at the entry of the callee (i.e., including the
 * the changed link register) which can be used by a "normal" function
 *
 * This includes the argument registers, but also the TOC pointer, stack pointer and link register,
 * So: r1 (stack pointer), r2 (TOC pointer), r3-r10 (arg registers), r11 (local link), f1-f8 (fp arg
 * registers ppc32) or f1-f13 (fp arg registers ppc64), lr, bit 6 of cr (= cr1[eq], set when passing
 * arguments in floating point registers to a varargs function). Also include all system registers.
 */
/* callee may use           */   {MAX_REG_ITERATOR, {0x00000ffe,0x000001fe,0xfffff902,0x00000fff}},  

/* next is the set of registers which can be changed by a "normal" function
 *
 * These are the volatile registers, and the TOC pointer (restored after the call if needed).
 * Also include all system registers.
 */
/* callee may change        */   {MAX_REG_ITERATOR, {0x00001ff9,0x00003fff,0xffffffe3,0x00000fff}},
/* callee may return        */   {MAX_REG_ITERATOR, {0x00000018,0x0000001e}}, /* r3-r4 for 64 bit int, long long double may use f1-f4 */
/* always live              */   {MAX_REG_ITERATOR, {0x00002006}}, /* r13 = tls ptr, r2 = gp, r1 = sp */
/* registers prop over hell */   {MAX_REG_ITERATOR, {0x00000000}}, /* TODO*/
/* const registers          */   {MAX_REG_ITERATOR, {0x00000000}},
/* dead over call           */   {MAX_REG_ITERATOR, {0x00001001}}, /* r0 & r12 may be overwritten by trampoline */
/* link registers           */   {MAX_REG_ITERATOR, {0x00000000,0x00000000,0x00000100}},
/* argument registers       */   {MAX_REG_ITERATOR, {0x000007f8,0x000001fe,0x00000002}}, /* r3-r11 (r11: local link for nested functions), f1-f8 (on ppc64: f1-f13), cr1[eq] (varargs use float registers) */
/* return registers         */   {MAX_REG_ITERATOR, {0x00000018,0x0000001e}},
/* dyncall use registers    */   {MAX_REG_ITERATOR, {0}},
#elif MAX_REG_ITERATOR > 32
/* less than 64 registers: regsets are represented as t_uint64 */
#error ppc has more than 64 registers!!!
#else
/* less than 32 registers: regsets are represented as t_uint32 */
#error ppc has more than 32 registers!!!
#endif
   /*! The program counter */
   REG_NONE, /* REG_NONE if the program counter is not user-visible,
		the pc's register number otherwise */
   /*! An array containing the name of each register */
   names,
   /*! Callback to disassemble a section */
   PpcDisassembleSection,
   /*! Callback to assemble a section */
   PpcAssembleSection,
   /*! Callback to create a flowgraph for an object */
   PpcFlowgraph,
   /*! Callback to deflowgraph a section */
   PpcDeflowgraph,
   /*! Callback to make addressproducers for a section */
   PpcMakeAddressProducers,
   NULL,
   PpcInsMakeUncondBranch,
   NULL,
   PpcInsHasSideEffect,
   /*! Returns true when instruction is load */
   PpcInsIsLoad,
   /*! Returns true when instruction is a store */
   PpcInsIsStore,
   PpcInsIsProcedureCall,
   PpcInsIsIndirectCall,
   NULL,
   PpcInsIsUnconditionalBranch,
   PpcInsIsControlTransfer,
   PpcInsIsSystemInstruction,
   /*! Returns true when instruction is program exit */
   PpcInsIsSyscallExit,
   PpcInstructionPrint,
   PpcInsMakeNop,
   PpcInsAreIdentical,
   PpcInsUnconditionalize,
   NULL /*PpcParseFromStringAndInsertAt*/,
   NULL,
//   PpcFunComputeStackSavedRegisters,
   PpcFunIsGlobal,
   /*! An array with names of unbehaved functions */
   NULL,   
   NULL,
   NULL,
   NULL
};

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
