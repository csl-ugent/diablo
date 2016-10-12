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

#ifndef PPC_PROCESSORS_TYPEDEFS
#define PPC_PROCESSORS_TYPEDEFS

typedef enum _t_ppc_opcode
{
  PPC_TDI,
  PPC_TWI,
  PPC_MULLI,
  PPC_SUBFIC,
  PPC_CMPLI,
  PPC_CMPI,
  PPC_ADDIC,
  PPC_ADDIC_DOT,
  PPC_ADDI,
  PPC_ADDIS,
  PPC_BC,
  PPC_SC,
  PPC_B,
  PPC_MCRF,
  PPC_BCLR,
  PPC_RFID,
  PPC_CRNOR,
  PPC_CRANDC,
  PPC_ISYNC,
  PPC_CRXOR,
  PPC_CRNAND,
  PPC_CRAND,
  PPC_CREQV,
  PPC_CRORC,
  PPC_CROR,
  PPC_BCCTR,
  PPC_RLWIMI,
  PPC_RLWINM,
  PPC_RLWNM,
  PPC_ORI,
  PPC_ORIS,
  PPC_XORI,
  PPC_XORIS,
  PPC_ANDI_DOT,
  PPC_ANDIS_DOT,
  PPC_RLDICL,
  PPC_RLDICR,
  PPC_RLDIC,
  PPC_RLDIMI,
  PPC_RLDCL,
  PPC_RLDCR,
  PPC_CMP,
  PPC_TW,
  PPC_SUBFC,
  PPC_MULHDU,
  PPC_ADDC,
  PPC_MULHWU,
  PPC_MFCR,
  PPC_LWARX,
  PPC_LDX,
  PPC_LWZX,
  PPC_SLW,
  PPC_CNTLZW,
  PPC_SLD,
  PPC_AND,
  PPC_CMPL,
  PPC_SUBF,
  PPC_LDUX,
  PPC_DCBST,
  PPC_LWZUX,
  PPC_CNTLZD,
  PPC_ANDC,
  PPC_TD,
  PPC_MULHD,
  PPC_MULHW,
  PPC_MFMSR,
  PPC_LDARX,
  PPC_DCBF,
  PPC_LBZX,
  PPC_NEG,
  PPC_LBZUX,
  PPC_NOR,
  PPC_SUBFE,
  PPC_ADDE,
  PPC_MTCRF,
  PPC_MTOCRF,
  PPC_MTMSR,
  PPC_STDX,
  PPC_STWCX_DOT,
  PPC_STWX,
  PPC_MTMSRD,
  PPC_STDUX,
  PPC_STWUX,
  PPC_SUBFZE,
  PPC_ADDZE,
  PPC_MTSR,
  PPC_STDCX_DOT,
  PPC_STBX,
  PPC_SUBFME,
  PPC_MULLD,
  PPC_ADDME,
  PPC_MULLW,
  PPC_MTSRIN,
  PPC_DCBTST,
  PPC_STBUX,
  PPC_ADD,
  PPC_DCBT,
  PPC_LHZX,
  PPC_EQV,
  PPC_TLBIE,
  PPC_ECIWX,
  PPC_LHZUX,
  PPC_XOR,
  PPC_MFSPR,
  PPC_LWAX,
  PPC_LHAX,
  PPC_TLBIA,
  PPC_MFTB,
  PPC_LWAUX,
  PPC_LHAUX,
  PPC_SLBMTE,
  PPC_STHX,
  PPC_ORC,
  PPC_SRADI,
  PPC_SLBIE,
  PPC_ECOWX,
  PPC_STHUX,
  PPC_OR,
  PPC_DIVDU,
  PPC_DIVWU,
  PPC_MTSPR,
  PPC_NAND,
  PPC_DIVD,
  PPC_DIVW,
  PPC_SLBIA,
  PPC_MCRXR,
  PPC_LSWX,
  PPC_LWBRX,
  PPC_LFSX,
  PPC_SRW,
  PPC_SRD,
  PPC_TLBSYNC,
  PPC_LFSUX,
  PPC_MFSR,
  PPC_LSWI,
  PPC_SYNC,
  PPC_LFDX,
  PPC_LFDUX,
  PPC_MFSRIN,
  PPC_STSWX,
  PPC_STWBRX,
  PPC_STFSX,
  PPC_STFSUX,
  PPC_STSWI,
  PPC_STFDX,
  PPC_STFDUX,
  PPC_LHBRX,
  PPC_SRAW,
  PPC_SRAD,
  PPC_SRAWI,
  PPC_SLBMFEV,
  PPC_EIEIO,
  PPC_SLBMFEE,
  PPC_STHBRX,
  PPC_EXTSH,
  PPC_EXTSB,
  PPC_ICBI,
  PPC_STFIWX,
  PPC_EXTSW,
  PPC_DCBZ,
  PPC_LWZ,
  PPC_LWZU,
  PPC_LBZ,
  PPC_LBZU,
  PPC_STW,
  PPC_STWU,
  PPC_STB,
  PPC_STBU,
  PPC_LHZ,
  PPC_LHZU,
  PPC_LHA,
  PPC_LHAU,
  PPC_STH,
  PPC_STHU,
  PPC_LMW,
  PPC_STMW,
  PPC_LFS,
  PPC_LFSU,
  PPC_LFD,
  PPC_LFDU,
  PPC_STFS,
  PPC_STFSU,
  PPC_STFD,
  PPC_STFDU,
  PPC_LD,
  PPC_LDU,
  PPC_LWA,
  PPC_FDIVS,
  PPC_FSUBS,
  PPC_FADDS,
  PPC_FSQRTS,
  PPC_FRES,
  PPC_FMULS,
  PPC_FMSUBS,
  PPC_FMADDS,
  PPC_FNMSUBS,
  PPC_FNMADDS,
  PPC_STD,
  PPC_STDU,
  PPC_FCMPU,
  PPC_FRSP,
  PPC_FCTIW,
  PPC_FCTIWZ,
  PPC_FDIV,
  PPC_FSUB,
  PPC_FADD,
  PPC_FSQRT,
  PPC_FSEL,
  PPC_FMUL,
  PPC_FRSQRTE,
  PPC_FMSUB,
  PPC_FMADD,
  PPC_FNMSUB,
  PPC_FNMADD,
  PPC_FCMPO,
  PPC_MTFSB1,
  PPC_FNEG,
  PPC_MCRFS,
  PPC_MTFSB0,
  PPC_FMR,
  PPC_MTFSFI,
  PPC_FNABS,
  PPC_FABS,
  PPC_MFFS,
  PPC_MTFSF,
  PPC_FCTID,
  PPC_FCTIDZ,
  PPC_FCFID,
#ifdef PPC_ALTIVEC_SUPPORT
  PPC_VMHADDSHS,
  PPC_VMHRADDSHS,
  PPC_VMLADDUHM,
  PPC_VMSUMUBM,
  PPC_VMSUMMBM,
  PPC_VMSUMUHM,
  PPC_VMSUMUHS,
  PPC_VMSUMSHM,
  PPC_VMSUMSHS,
  PPC_VSEL,
  PPC_VPERM,
  PPC_VSLDOI,
  PPC_VMADDFP,
  PPC_VNMSUBFP,
  PPC_VADDUBM,
  PPC_VADDUHM,
  PPC_VADDUWM,
  PPC_VADDCUW,
  PPC_VADDUBS,
  PPC_VADDUHS,
  PPC_VADDUWS,
  PPC_VADDSBS,
  PPC_VADDSHS,
  PPC_VADDSWS,
  PPC_VSUBUBM,
  PPC_VSUBUHM,
  PPC_VSUBUWM,
  PPC_VSUBCUW,
  PPC_VSUBUBS,
  PPC_VSUBUHS,
  PPC_VSUBUWS,
  PPC_VSUBSBS,
  PPC_VSUBSHS,
  PPC_VSUBSWS,
  PPC_VMAXUB,
  PPC_VMAXUH,
  PPC_VMAXUW,
  PPC_VMAXSB,
  PPC_VMAXSH,
  PPC_VMAXSW,
  PPC_VMINUB,
  PPC_VMINUH,
  PPC_VMINUW,
  PPC_VMINSB,
  PPC_VMINSH,
  PPC_VMINSW,
  PPC_VAVGUB,
  PPC_VAVGUH,
  PPC_VAVGUW,
  PPC_VAVGSB,
  PPC_VAVGSH,
  PPC_VAVGSW,
  PPC_VRLB,
  PPC_VRLH,
  PPC_VRLW,
  PPC_VSLB,
  PPC_VSLH,
  PPC_VSLW,
  PPC_VSL,
  PPC_VSRB,
  PPC_VSRH,
  PPC_VSRW,
  PPC_VSR,
  PPC_VSRAB,
  PPC_VSRAH,
  PPC_VSRAW,
  PPC_VAND,
  PPC_VANDC,
  PPC_VOR,
  PPC_VXOR,
  PPC_VNOR,
  PPC_MFVSCR,
  PPC_MTVSCR,
  PPC_VCMPEQUB,
  PPC_VCMPEQUH,
  PPC_VCMPEQUW,
  PPC_VCMPEQFP,
  PPC_VCMPGEFP,
  PPC_VCMPGTUB,
  PPC_VCMPGTUH,
  PPC_VCMPGTUW,
  PPC_VCMPGTFP,
  PPC_VCMPGTSB,
  PPC_VCMPGTSH,
  PPC_VCMPGTSW,
  PPC_VCMPBFP,
  PPC_VMULOUB,
  PPC_VMULOUH,
  PPC_VMULOSB,
  PPC_VMULOSH,
  PPC_VMULEUB,
  PPC_VMULEUH,
  PPC_VMULESB,
  PPC_VMULESH,
  PPC_VSUM4UBS,
  PPC_VSUM4SBS,
  PPC_VSUM4SHS,
  PPC_VSUM2SWS,
  PPC_VSUMSWS,
  PPC_VADDFP,
  PPC_VSUBFP,
  PPC_VREFP,
  PPC_VRSQRTEFP,
  PPC_VEXPTEFP,
  PPC_VLOGEFP,
  PPC_VRFIN,
  PPC_VRFIZ,
  PPC_VRFIP,
  PPC_VRFIM,
  PPC_VCFUX,
  PPC_VCFSX,
  PPC_VCTUXS,
  PPC_VCTSXS,
  PPC_VMAXFP,
  PPC_VMINFP,
  PPC_VMRGHB,
  PPC_VMRGHH,
  PPC_VMRGHW,
  PPC_VMRGLB,
  PPC_VMRGLH,
  PPC_VMRGLW,
  PPC_VSPLTB,
  PPC_VSPLTH,
  PPC_VSPLTW,
  PPC_VSPLTISB,
  PPC_VSPLTISH,
  PPC_VSPLTISW,
  PPC_VSLO,
  PPC_VSRO,
  PPC_VPKUHUM,
  PPC_VPKUWUM,
  PPC_VPKUHUS,
  PPC_VPKUWUS,
  PPC_VPKSHUS,
  PPC_VPKSWUS,
  PPC_VPKSHSS,
  PPC_VPKSWSS,
  PPC_VUPKHSB,
  PPC_VUPKHSH,
  PPC_VUPKLSB,
  PPC_VUPKLSH,
  PPC_VPKPX,
  PPC_VUPKHPX,
  PPC_VUPKLPX,
  PPC_LVSL,
  PPC_LVSR,
  PPC_DST,
  PPC_DSTT,
  PPC_DSTST,
  PPC_DSTSTT,
  PPC_DSS,
  PPC_DSSALL,
  PPC_LVEBX,
  PPC_LVEHX,
  PPC_LVEWX,
  PPC_LVX,
  PPC_LVXL,
  PPC_STVEBX,
  PPC_STVEHX,
  PPC_STVEWX,
  PPC_STVX,
  PPC_STVXL,
  PPC_ALTIVEC,
  PPC_ALTIVEC2,
#endif
  PPC_UNKNOWN,
  PPC_DATA,
  /* update this value to "the last in line" */
  PPC_LAST = PPC_DATA
} t_ppc_opcode, renamed_t_ppc_opcode;
#endif

#ifndef PPC_PROCESSOR_SUPPORT_FLAGS
#define PPC_PROCESSOR_SUPPORT_FLAGS

/* Opcode is defined for the PowerPC architecture.  */
#define PPC_PROCESSOR_PPC			 1

/* Opcode is defined for the POWER (RS/6000) architecture.  */
#define PPC_PROCESSOR_POWER		 2

/* Opcode is defined for the POWER2 (Rios 2) architecture.  */
#define PPC_PROCESSOR_POWER2		 4

/* Opcode is only defined on 32 bit architectures.  */
#define PPC_PROCESSOR_32			 8

/* Opcode is only defined on 64 bit architectures.  */
#define PPC_PROCESSOR_64		      0x10

/* Opcode is supported by the Motorola PowerPC 601 processor.  The 601
   is assumed to support all PowerPC (PPC_PROCESSOR_PPC) instructions,
   but it also supports many additional POWER instructions.  */
#define PPC_PROCESSOR_601		      0x20

/* Opcode is supported in both the Power and PowerPC architectures
   (ie, compiler's -mcpu=common or assembler's -mcom).  */
#define PPC_PROCESSOR_COMMON	      0x40

/* Opcode is supported for any Power or PowerPC platform (this is
   for the assembler's -many option, and it eliminates duplicates).  */
#define PPC_PROCESSOR_ANY		      0x80

/* Opcode is supported as part of the 64-bit bridge.  */
#define PPC_PROCESSOR_64_BRIDGE	     0x100

/* Opcode is supported by Altivec Vector Unit */
#define PPC_PROCESSOR_ALTIVEC	     0x200

/* Opcode is supported by PowerPC 403 processor.  */
#define PPC_PROCESSOR_403		     0x400

/* Opcode is supported by PowerPC BookE processor.  */
#define PPC_PROCESSOR_BOOKE	     0x800

/* Opcode is only supported by 64-bit PowerPC BookE processor.  */
#define PPC_PROCESSOR_BOOKE64	    0x1000

/* Opcode is supported by PowerPC 440 processor.  */
#define PPC_PROCESSOR_440		    0x2000

/* Opcode is only supported by Power4 architecture.  */
#define PPC_PROCESSOR_POWER4	    0x4000

/* Opcode isn't supported by Power4 architecture.  */
#define PPC_PROCESSOR_NOPOWER4	    0x8000

/* Opcode is only supported by POWERPC Classic architecture.  */
#define PPC_PROCESSOR_CLASSIC	   0x10000

/* Opcode is only supported by e500x2 Core.  */
#define PPC_PROCESSOR_SPE		   0x20000

/* Opcode is supported by e500x2 Integer select APU.  */
#define PPC_PROCESSOR_ISEL		   0x40000

/* Opcode is an e500 SPE floating point instruction.  */
#define PPC_PROCESSOR_EFS		   0x80000

/* Opcode is supported by branch locking APU.  */
#define PPC_PROCESSOR_BRLOCK	  0x100000

/* Opcode is supported by performance monitor APU.  */
#define PPC_PROCESSOR_PMR		  0x200000

/* Opcode is supported by cache locking APU.  */
#define PPC_PROCESSOR_CACHELCK	  0x400000

/* Opcode is supported by machine check APU.  */
#define PPC_PROCESSOR_RFMCI	  0x800000

/* Macros used to form opcodes.  */

#define OPC(val, st, sz)        ((((t_uint32)(val)) & ((0x1 << sz) - 1)) << st)
#define OPC_MASK(st, sz)        (((0x1 << sz) - 1) << st)

/* The main opcode.  */
#define OP(x) (((( t_uint32)(x)) & 0X3f) << 26)
#define OP_MASK OP (0x3f)

/* The main opcode combined with a trap code in the TO field of a D
   form instruction.  Used for extended mnemonics for the trap
   instructions.  */
#define OPTO(x,to) (OP (x) | ((((unsigned long)(to)) & 0x1f) << 21))
#define OPTO_MASK (OP_MASK | TO_MASK)

/* The main opcode combined with a comparison size bit in the L field
   of a D form or X form instruction.  Used for extended mnemonics for
   the comparison instructions.  */
#define OPL(x,l) (OP (x) | ((((unsigned long)(l)) & 1) << 21))
#define OPL_MASK OPL (0x3f,1)

/* An A form instruction.  */
#define A(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1))
#define A_MASK A (0x3f, 0x1f)

/* An A_MASK with the FRB field fixed.  */
#define AFRB_MASK (A_MASK | FRB_MASK)

/* An A_MASK with the FRC field fixed.  */
#define AFRC_MASK (A_MASK | FRC_MASK)

/* An A_MASK with the FRA and FRC fields fixed.  */
#define AFRAFRC_MASK (A_MASK | FRA_MASK | FRC_MASK)

/* A B form instruction.  */
#define B(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 1) << 1) | ((lk) & 1))
#define B_MASK B (0x3f, 1, 1)

/* A B form instruction setting the BO field.  */
#define BBO(op, bo, aa, lk) (B ((op), (aa), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define BBO_MASK BBO (0x3f, 0x1f, 1, 1)

/* A BBO_MASK with the y bit of the BO field removed.  This permits
   matching a conditional branch regardless of the setting of the y
   bit.  Similarly for the 'at' bits used for power4 branch hints.  */
#define Y_MASK   (((unsigned long) 1) << 21)
#define AT1_MASK (((unsigned long) 3) << 21)
#define AT2_MASK (((unsigned long) 9) << 21)
#define BBOY_MASK  (BBO_MASK &~ Y_MASK)
#define BBOAT_MASK (BBO_MASK &~ AT1_MASK)

/* A B form instruction setting the BO field and the condition bits of
   the BI field.  */
#define BBOCB(op, bo, cb, aa, lk) \
(BBO ((op), (bo), (aa), (lk)) | ((((unsigned long)(cb)) & 0x3) << 16))
#define BBOCB_MASK BBOCB (0x3f, 0x1f, 0x3, 1, 1)

     /* A BBOCB_MASK with the y bit of the BO field removed.  */
#define BBOYCB_MASK (BBOCB_MASK &~ Y_MASK)
#define BBOATCB_MASK (BBOCB_MASK &~ AT1_MASK)
#define BBOAT2CB_MASK (BBOCB_MASK &~ AT2_MASK)

     /* A BBOYCB_MASK in which the BI field is fixed.  */
#define BBOYBI_MASK (BBOYCB_MASK | BI_MASK)
#define BBOATBI_MASK (BBOAT2CB_MASK | BI_MASK)

     /* An Context form instruction.  */
#define CTX(op, xop)   (OP (op) | (((unsigned long)(xop)) & 0x7))
#define CTX_MASK CTX(0x3f, 0x7)

     /* An User Context form instruction.  */
#define UCTX(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define UCTX_MASK UCTX(0x3f, 0x1f)

     /* The main opcode mask with the RA field clear.  */
#define DRA_MASK (OP_MASK | RA_MASK)

     /* A DS form instruction.  */
#define DSO(op, xop) (OP (op) | ((xop) & 0x3))
#define DS_MASK DSO (0x3f, 3)

     /* A DE form instruction.  */
#define DEO(op, xop) (OP (op) | ((xop) & 0xf))
#define DE_MASK DEO (0x3e, 0xf)

     /* An EVSEL form instruction.  */
#define EVSEL(op, xop) (OP (op) | (((unsigned long)(xop)) & 0xff) << 3)
#define EVSEL_MASK EVSEL(0x3f, 0xff)

     /* An M form instruction.  */
#define M(op, rc) (OP (op) | ((rc) & 1))
#define M_MASK M (0x3f, 1)

     /* An M form instruction with the ME field specified.  */
#define MME(op, me, rc) (M ((op), (rc)) | ((((unsigned long)(me)) & 0x1f) << 1))

     /* An M_MASK with the MB and ME fields fixed.  */
#define MMBME_MASK (M_MASK | MB_MASK | ME_MASK)

     /* An M_MASK with the SH and ME fields fixed.  */
#define MSHME_MASK (M_MASK | SH_MASK | ME_MASK)

     /* An MD form instruction.  */
#define MD(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x7) << 2))
#define MD_MASK MD (0x3f, 0x7)

     /* An MD_MASK with the MB field fixed.  */
#define MDMB_MASK (MD_MASK | MB6_MASK)

     /* An MD_MASK with the SH field fixed.  */
#define MDSH_MASK (MD_MASK | SH6_MASK)

     /* An MDS form instruction.  */
#define MDS(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0xf) << 1))
#define MDS_MASK MDS (0x3f, 0xf)

     /* An MDS_MASK with the MB field fixed.  */
#define MDSMB_MASK (MDS_MASK | MB6_MASK)

     /* An SC form instruction.  */
#define SC(op) ((OP (op)) | 0x2)
#define SC_MASK (OP_MASK | 0x2)

     /* An VX form instruction.  */
#define VX(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x7ff))

     /* The mask for an VX form instruction.  */
#define VX_MASK	VX(0x3f, 0x7ff)

     /* An VA form instruction.  */
#define VXA(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x03f))

     /* The mask for an VA form instruction.  */
#define VXA_MASK VXA(0x3f, 0x3f)

     /* An VXR form instruction.  */
#define VXR(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x3ff))

     /* The mask for a VXR form instruction.  */
#define VXR_MASK VXR(0x3f, 0x3ff)

     /* An X form instruction.  */
#define X(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

     /* An X form instruction with the RC bit specified.  */
#define XRC(op, xop, rc) (X ((op), (xop)) | ((rc) & 1))

     /* The mask for an X form instruction.  */
#define X_MASK XRC (0x3f, 0x3ff, 0)

     /* An X_MASK with the RA field fixed.  */
#define XRA_MASK (X_MASK | RA_MASK)

     /* An X_MASK with the RB field fixed.  */
#define XRB_MASK (X_MASK | RB_MASK)

     /* An X_MASK with the RT field fixed.  */
#define XRT_MASK (X_MASK | RT_MASK)

     /* An X_MASK with the RA and RB fields fixed.  */
#define XRARB_MASK (X_MASK | RA_MASK | RB_MASK)

     /* An XRARB_MASK, but with the L bit clear.  */
#define XRLARB_MASK (XRARB_MASK & ~((unsigned long) 1 << 16))

     /* An X_MASK with the RT and RA fields fixed.  */
#define XRTRA_MASK (X_MASK | RT_MASK | RA_MASK)

     /* An XRTRA_MASK, but with L bit clear.  */
#define XRTLRA_MASK (XRTRA_MASK & ~((unsigned long) 1 << 21))

     /* An X form instruction with the L bit specified.  */
#define XOPL(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 1) << 21))

     /* The mask for an X form comparison instruction.  */
#define XCMP_MASK (X_MASK | (((unsigned long)1) << 22))

     /* The mask for an X form comparison instruction with the L field
        fixed.  */
#define XCMPL_MASK (XCMP_MASK | (((unsigned long)1) << 21))

     /* An X form trap instruction with the TO field specified.  */
#define XTO(op, xop, to) (X ((op), (xop)) | ((((unsigned long)(to)) & 0x1f) << 21))
#define XTO_MASK (X_MASK | TO_MASK)

     /* An X form tlb instruction with the SH field specified.  */
#define XTLB(op, xop, sh) (X ((op), (xop)) | ((((unsigned long)(sh)) & 0x1f) << 11))
#define XTLB_MASK (X_MASK | SH_MASK)

     /* An X form sync instruction.  */
#define XSYNC(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 3) << 21))

     /* An X form sync instruction with everything filled in except the LS field.  */
#define XSYNC_MASK (0xff9fffff)

     /* An X form AltiVec dss instruction.  */
#define XDSS(op, xop, a) (X ((op), (xop)) | ((((unsigned long)(a)) & 1) << 25))
#define XDSS_MASK XDSS(0x3f, 0x3ff, 1)

     /* An XFL form instruction.  */
#define XFL(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1) | (((unsigned long)(rc)) & 1))
#define XFL_MASK (XFL (0x3f, 0x3ff, 1) | (((unsigned long)1) << 25) | (((unsigned long)1) << 16))

     /* An X form isel instruction.  */
#define XISEL(op, xop)  (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1))
#define XISEL_MASK      XISEL(0x3f, 0x1f)

     /* An XL form instruction with the LK field set to 0.  */
#define XL(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

     /* An XL form instruction which uses the LK field.  */
#define XLLK(op, xop, lk) (XL ((op), (xop)) | ((lk) & 1))

     /* The mask for an XL form instruction.  */
#define XL_MASK XLLK (0x3f, 0x3ff, 0)

     /* An XL form instruction which explicitly sets the BO field.  */
#define XLO(op, bo, xop, lk) \
(XLLK ((op), (xop), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define XLO_MASK (XL_MASK | BO_MASK)

     /* An XL form instruction which explicitly sets the y bit of the BO
        field.  */
#define XLYLK(op, xop, y, lk) (XLLK ((op), (xop), (lk)) | ((((unsigned long)(y)) & 1) << 21))
#define XLYLK_MASK (XL_MASK | Y_MASK)

     /* An XL form instruction which sets the BO field and the condition
        bits of the BI field.  */
#define XLOCB(op, bo, cb, xop, lk) \
(XLO ((op), (bo), (xop), (lk)) | ((((unsigned long)(cb)) & 3) << 16))
#define XLOCB_MASK XLOCB (0x3f, 0x1f, 0x3, 0x3ff, 1)

     /* An XL_MASK or XLYLK_MASK or XLOCB_MASK with the BB field fixed.  */
#define XLBB_MASK (XL_MASK | BB_MASK)
#define XLYBB_MASK (XLYLK_MASK | BB_MASK)
#define XLBOCBBB_MASK (XLOCB_MASK | BB_MASK)

     /* A mask for branch instructions using the BH field.  */
#define XLBH_MASK (XL_MASK | (0x1c << 11))

     /* An XL_MASK with the BO and BB fields fixed.  */
#define XLBOBB_MASK (XL_MASK | BO_MASK | BB_MASK)

     /* An XL_MASK with the BO, BI and BB fields fixed.  */
#define XLBOBIBB_MASK (XL_MASK | BO_MASK | BI_MASK | BB_MASK)

     /* An XO form instruction.  */
#define XO(op, xop, oe, rc) \
(OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1) | ((((unsigned long)(oe)) & 1) << 10) | (((unsigned long)(rc)) & 1))
#define XO_MASK XO (0x3f, 0x1ff, 0, 0)

     /* An XO_MASK with the RB field fixed.  */
#define XORB_MASK (XO_MASK | RB_MASK)

     /* An XS form instruction.  */
#define XS(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 2) )
#define XS_MASK XS (0x3f, 0x1ff)

     /* A mask for the FXM version of an XFX form instruction.  */
#define XFXFXM_MASK (X_MASK | (1 << 11) | (1 << 20))

     /* An XFX form instruction with the FXM field filled in.  */
#define XFXM(op, xop, fxm, p4) \
(X ((op), (xop)) | ((((unsigned long)(fxm)) & 0xff) << 12) \
 | ((unsigned long)(p4) << 20))

     /* An XFX form instruction with the SPR field filled in.  */
#define XSPR(op, xop, spr) \
(X ((op), (xop)) | ((((unsigned long)(spr)) & 0x1f) << 16) | ((((unsigned long)(spr)) & 0x3e0) << 6))
#define XSPR_MASK (X_MASK | SPR_MASK)

     /* An XFX form instruction with the SPR field filled in except for the
        SPRBAT field.  */
#define XSPRBAT_MASK (XSPR_MASK &~ SPRBAT_MASK)

     /* An XFX form instruction with the SPR field filled in except for the
        SPRG field.  */
#define XSPRG_MASK (XSPR_MASK &~ SPRG_MASK)

     /* An X form instruction with everything filled in except the E field.  */
#define XE_MASK (0xffff7fff)

     /* An X form user context instruction.  */
#define XUC(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define XUC_MASK      XUC(0x3f, 0x1f)

#define I_AA(x)         ((x) & 0x00000002)
#define I_LK(x)         ((x) & 0x00000001)
#define I_RC(x)         ((x) & 0x00000001)
#define I_ALTIVEC_RC(x) ((x) & 0x00000400)
#define I_OE(x)         ((x) & 0x00000400)
#define I_CTR(x)        (((x) & 0x04) == 0 )

     /* The BO encodings used in extended conditional branch mnemonics.  */
#define PPC_BODNZF	(0x0)
#define PPC_BODNZFP	(0x1)
#define PPC_BODZF	(0x2)
#define PPC_BODZFP	(0x3)
#define PPC_BODNZT	(0x8)
#define PPC_BODNZTP	(0x9)
#define PPC_BODZT	(0xa)
#define PPC_BODZTP	(0xb)

#define PPC_BOF	(0x4)
#define PPC_BOFP	(0x5)
#define PPC_BOFM4	(0x6)
#define PPC_BOFP4	(0x7)
#define PPC_BOT	(0xc)
#define PPC_BOTP	(0xd)
#define PPC_BOTM4	(0xe)
#define PPC_BOTP4	(0xf)

#define PPC_BODNZ	(0x10)
#define PPC_BODNZP	(0x11)
#define PPC_BODZ	(0x12)
#define PPC_BODZP	(0x13)
#define PPC_BODNZM4 (0x18)
#define PPC_BODNZP4 (0x19)
#define PPC_BODZM4	(0x1a)
#define PPC_BODZP4	(0x1b)

#define PPC_BOU	(0x14)

     typedef enum _t_ppc_condition_bo
{
  PPC_BO_DNZF,
  PPC_BO_DNZFP,
  PPC_BO_DZF,
  PPC_BO_DZFP,
  PPC_BO_DNZT,
  PPC_BO_DNZTP,
  PPC_BO_DZT,
  PPC_BO_DZTP,
  PPC_BO_F,
  PPC_BO_FP,
  PPC_BO_FM4,
  PPC_BO_FP4,
  PPC_BO_T,
  PPC_BO_TP,
  PPC_BO_TM4,
  PPC_BO_TP4,
  PPC_BO_DNZ,
  PPC_BO_DNZP,
  PPC_BO_DZ,
  PPC_BO_DZP,
  PPC_BO_DNZM4,
  PPC_BO_DNZP4,
  PPC_BO_DZM4,
  PPC_BO_DZP4,
  PPC_BO_U
} t_ppc_condition_bo, renamed_t_ppc_condition_bo;

/* The BI condition bit encodings used in extended conditional branch
   mnemonics.  */

#define PPC_CBLT	(0)
#define PPC_CBGT	(1)
#define PPC_CBEQ	(2)
#define PPC_CBSO	(3)
#define PPC_CBUN    (3)
#define PPC_CBINV   (-1)

#define PPC_IS_CBLT(x) ((PPC_INS_CB(x)%4)==PPC_CBLT)
#define PPC_IS_CBGT(x) ((PPC_INS_CB(x)%4)==PPC_CBGT)
#define PPC_IS_CBEQ(x) ((PPC_INS_CB(x)%4)==PPC_CBEQ)
#define PPC_IS_CBSO(x) ((PPC_INS_CB(x)%4)==PPC_CBSO)
#define PPC_IS_CBUN(x) ((PPC_INS_CB(x)%4)==PPC_CBUN)

#define PPC_COND_REG(x) ((PPC_INS_CB(x)/4)+PPC_REG_CR0)

typedef enum _t_ppc_condition_bit
{
  PPC_CB_INVALID = -1,
  PPC_CB_LT = 0,
  PPC_CB_GT = 1,
  PPC_CB_EQ = 2,
  PPC_CB_SO = 3,
  PPC_CB_UN = 3,
} t_ppc_condition_bit, renamed_t_ppc_condition_bit;

/* The TO encodings used in extended trap mnemonics.  */
#define PPC_TOLGT	(0x1)
#define PPC_TOLLT	(0x2)
#define PPC_TOEQ	(0x4)
#define PPC_TOLGE	(0x5)
#define PPC_TOLNL	(0x5)
#define PPC_TOLLE	(0x6)
#define PPC_TOLNG	(0x6)
#define PPC_TOGT	(0x8)
#define PPC_TOGE	(0xc)
#define PPC_TONL	(0xc)
#define PPC_TOLT	(0x10)
#define PPC_TOLE	(0x14)
#define PPC_TONG	(0x14)
#define PPC_TONE	(0x18)
#define PPC_TOU	(0x1f)

typedef enum _t_ppc_condition_trap
{
  PPC_CT_OLGT,	
  PPC_CT_OLLT,	
  PPC_CT_OEQ,	
  PPC_CT_OLGE,	
  PPC_CT_OLNL,	
  PPC_CT_OLLE,	
  PPC_CT_OLNG,	
  PPC_CT_OGT,	
  PPC_CT_OGE,	
  PPC_CT_ONL,	
  PPC_CT_OLT,	
  PPC_CT_OLE,	
  PPC_CT_ONG,	
  PPC_CT_ONE,	
  PPC_CT_OU	
} t_ppc_condition_trap, renamed_t_ppc_condition_trap;

/* The BH encondings used in branch to LR and CTR instructions */

#define PPC_BH_LR_SR  (0x00)
#define PPC_BH_CTR_NSR  (0x00)
#define PPC_BH_LR_NSR  (0x01)
#define PPC_BH_NP  (0x11)

typedef enum _t_ppc_branch_hint
{
  PPC_FBH_LR_SR,	
  PPC_FBH_CTR_NSR,	
  PPC_FBH_LR_NSR,	
  PPC_FBH_NP	
} t_ppc_branch_hint, renamed_t_ppc_branch_hint;


/* Smaller names for the flags so each entry in the opcodes table will
   fit on a single line.  */
#undef	PPC
#define PPC     PPC_PROCESSOR_PPC
#define PPCCOM	PPC_PROCESSOR_PPC | PPC_PROCESSOR_COMMON
#define NOPOWER4 PPC_PROCESSOR_NOPOWER4 | PPCCOM
#define POWER4	PPC_PROCESSOR_POWER4
#define PPC32   PPC_PROCESSOR_32 | PPC_PROCESSOR_PPC
#define PPC64   PPC_PROCESSOR_64 | PPC_PROCESSOR_PPC
#define PPC403	PPC_PROCESSOR_403
#define PPC405	PPC403
#define PPC440	PPC_PROCESSOR_440
#define PPC750	PPC
#define PPC860	PPC
#define PPCVEC	PPC_PROCESSOR_ALTIVEC
#define	POWER   PPC_PROCESSOR_POWER
#define	POWER2	PPC_PROCESSOR_POWER | PPC_PROCESSOR_POWER2
#define PPCPWR2	PPC_PROCESSOR_PPC | PPC_PROCESSOR_POWER | PPC_PROCESSOR_POWER2
#define	POWER32	PPC_PROCESSOR_POWER | PPC_PROCESSOR_32
#define	COM     PPC_PROCESSOR_POWER | PPC_PROCESSOR_PPC | PPC_PROCESSOR_COMMON
#define	COM32   PPC_PROCESSOR_POWER | PPC_PROCESSOR_PPC | PPC_PROCESSOR_COMMON | PPC_PROCESSOR_32
#define	M601    PPC_PROCESSOR_POWER | PPC_PROCESSOR_601
#define PWRCOM	PPC_PROCESSOR_POWER | PPC_PROCESSOR_601 | PPC_PROCESSOR_COMMON
#define	MFDEC1	PPC_PROCESSOR_POWER
#define	MFDEC2	PPC_PROCESSOR_PPC | PPC_PROCESSOR_601 | PPC_PROCESSOR_BOOKE
#define BOOKE	PPC_PROCESSOR_BOOKE
#define BOOKE64	PPC_PROCESSOR_BOOKE64
#define CLASSIC	PPC_PROCESSOR_CLASSIC
#define PPCSPE	PPC_PROCESSOR_SPE
#define PPCISEL	PPC_PROCESSOR_ISEL
#define PPCEFS	PPC_PROCESSOR_EFS
#define PPCBRLK	PPC_PROCESSOR_BRLOCK
#define PPCPMR	PPC_PROCESSOR_PMR
#define PPCCHLK	PPC_PROCESSOR_CACHELCK
#define PPCCHLK64	PPC_PROCESSOR_CACHELCK | PPC_PROCESSOR_BOOKE64
#define PPCRFMCI	PPC_PROCESSOR_RFMCI

#endif

#ifdef DIABLOPPC_TYPES
#ifndef PPC_PROCESSORS_TYPES
#define PPC_PROCESSORS_TYPES

/*! The structure of the different disassemble functions */
typedef void (*PpcDisassembleFunction) (t_ppc_ins *, t_uint32 instr, t_uint16 opc);
/*! The structure of the different assemble functions */
typedef void (*PpcAssembleFunction) (t_ppc_ins *, t_uint32 *);

/*! The opcode structure, used in the opcode table to specify what disassembly
 * function corresponds to what bitpattern */
typedef struct _t_ppc_opcode_entry
{
  /*! Textual name of the opcode */
  t_string name;
  /*! Opcode's extracted value */
  t_ppc_opcode opcode;
  /*! Opcode's extractor mask */
  t_uint32 mask;
  /*! Disassembly function */
  PpcDisassembleFunction Disassemble;
  /*! Assembly function */
  PpcAssembleFunction Assemble;
  /*! Print format */
  char *print;
} t_ppc_opcode_entry;

/*! The opcode table */
extern const t_ppc_opcode_entry *ppc_opcode_table;

/* Condition aliases */
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
