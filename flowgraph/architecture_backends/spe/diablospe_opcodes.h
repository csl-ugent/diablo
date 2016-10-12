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

/*! \addtogroup SPE_BACKEND @{ */

#ifndef SPE_OPCODES_H
#define SPE_OPCODES_H

#include <diablospe.h>

typedef enum
{
    /* Memory load/store */
    SPE_LQD,
    SPE_LQX,
    SPE_LQA,
    SPE_LQR,
    SPE_STQD,
    SPE_STQX,
    SPE_STQA,
    SPE_STQR,
    SPE_CBD,
    SPE_CBX,
    SPE_CHD,
    SPE_CHX,
    SPE_CWD,
    SPE_CWX,
    SPE_CDD,
    SPE_CDX,
    /* Constant-formation */
    SPE_ILH,
    SPE_ILHU,
    SPE_IL,
    SPE_ILA,
    SPE_IOHL,
    SPE_FSMBI,
    /* Integer and logical */
    SPE_AH,
    SPE_AHI,
    SPE_A,
    SPE_AI,
    SPE_SFH,
    SPE_SFHI,
    SPE_SF,
    SPE_SFI,
    SPE_ADDX,
    SPE_CG,
    SPE_CGX,
    SPE_SFX,
    SPE_BG,
    SPE_BGX,
    SPE_MPY,
    SPE_MPYU,
    SPE_MPYI,
    SPE_MPYUI,
    SPE_MPYA,
    SPE_MPYH,
    SPE_MPYS,
    SPE_MPYHH,
    SPE_MPYHHA,
    SPE_MPYHHU,
    SPE_MPYHHAU,
    SPE_CLZ,
    SPE_CNTB,
    SPE_FSMB,
    SPE_FSMH,
    SPE_FSM,
    SPE_GBB,
    SPE_GBH,
    SPE_GB,
    SPE_AVGB,
    SPE_ABSDB,
    SPE_SUMB,
    SPE_XSBH,
    SPE_XSHW,
    SPE_XSWD,
    SPE_AND,
    SPE_ANDC,
    SPE_ANDBI,
    SPE_ANDHI,
    SPE_ANDI,
    SPE_OR,
    SPE_ORC,
    SPE_ORBI,
    SPE_ORHI,
    SPE_ORI,
    SPE_ORX,
    SPE_XOR,
    SPE_XORBI,
    SPE_XORHI,
    SPE_XORI,
    SPE_NAND,
    SPE_NOR,
    SPE_EQV,
    SPE_SELB,
    SPE_SHUFB,
    /* Shift and rotate */
    SPE_SHLH,
    SPE_SHLHI,
    SPE_SHL,
    SPE_SHLI,
    SPE_SHLQBI,
    SPE_SHLQBII,
    SPE_SHLQBY,
    SPE_SHLQBYI,
    SPE_SHLQBYBI,
    SPE_ROTH,
    SPE_ROTHI,
    SPE_ROT,
    SPE_ROTI,
    SPE_ROTQBY,
    SPE_ROTQBYI,
    SPE_ROTQBYBI,
    SPE_ROTQBI,
    SPE_ROTQBII,
    SPE_ROTHM,
    SPE_ROTHMI,
    SPE_ROTM,
    SPE_ROTMI,
    SPE_ROTQMBY,
    SPE_ROTQMBYI,
    SPE_ROTQMBYBI,
    SPE_ROTQMBI,
    SPE_ROTQMBII,
    SPE_ROTMAH,
    SPE_ROTMAHI,
    SPE_ROTMA,
    SPE_ROTMAI,
    /* Compare, branch and halt */
    SPE_HEQ,
    SPE_HEQI,
    SPE_HGT,
    SPE_HGTI,
    SPE_HLGT,
    SPE_HLGTI,
    SPE_CEQB,
    SPE_CEQBI,
    SPE_CEQH,
    SPE_CEQHI,
    SPE_CEQ,
    SPE_CEQI,
    SPE_CGTB,
    SPE_CGTBI,
    SPE_CGTH,
    SPE_CGTHI,
    SPE_CGT,
    SPE_CGTI,
    SPE_CLGTB,
    SPE_CLGTBI,
    SPE_CLGTH,
    SPE_CLGTHI,
    SPE_CLGT,
    SPE_CLGTI,
    SPE_BR,
    SPE_BRA,
    SPE_BRSL,
    SPE_BRASL,
    SPE_BI,
    SPE_IRET,
    SPE_BISLED,
    SPE_BISL,
    SPE_BRNZ,
    SPE_BRZ,
    SPE_BRHNZ,
    SPE_BRHZ,
    SPE_BIZ,
    SPE_BINZ,
    SPE_BIHZ,
    SPE_BIHNZ,
    /* Hint-for-branch */
    SPE_HBR,
    SPE_HBRA,
    SPE_HBRR,
    /* Floating-point */
    SPE_FA,
    SPE_DFA,
    SPE_FS,
    SPE_DFS,
    SPE_FM,
    SPE_DFM,
    SPE_FMA,
    SPE_DFMA,
    SPE_FNMS,
    SPE_DFNMS,
    SPE_FMS,
    SPE_DFMS,
    SPE_DFNMA,
    SPE_FREST,
    SPE_FRSQEST,
    SPE_FI,
    SPE_CSFLT,
    SPE_CFLTS,
    SPE_CUFLT,
    SPE_CFLTU,
    SPE_FRDS,
    SPE_FESD,
    SPE_FCEQ,
    SPE_FCMEQ,
    SPE_FCGT,
    SPE_FCMGT,
    SPE_FSCRWR,
    SPE_FSCRRD,
    /* Control */
    SPE_STOP,
    SPE_STOPD,
    SPE_LNOP,
    SPE_NOP,
    SPE_SYNC,
    SPE_DSYNC,
    SPE_MFSPR,
    SPE_MTSPR,
    /* Channel */
    SPE_RDCH,
    SPE_RCHCNT,
    SPE_WRCH,

    /* Special entries */
    SPE_DATA,
    SPE_UNKNOWN,
    /* update this value to "the last in line" */
    SPE_LAST_OPCODE = SPE_UNKNOWN
} t_spe_opcode;

/*! Special additional flags for instructions */
typedef t_uint16 t_spe_ins_flags;
#define IF_SPE_NONE                 0x0
#define IF_SPE_INLINE_PREFETCHING   0x1
#define IF_SPE_UPDATES_LINK_REG     0x2

/*! Types of transformations when reading an immediate.
 *
 * This is a string defining some simple transformations (one after the other):
 *      - s      : Sign extension (repeat the immediate field's leftmost bit)
 *      - <'num' : Shift 'num' bits left (add 'num' zeroes)
 */
typedef t_string t_spe_imm_trans;

/*! The structure of the different disassemble functions */
typedef void (*SpeDisassembleFunction) (t_uint32, t_spe_ins *, t_uint16 opc);
/*! The structure of the different assemble functions */
typedef void (*SpeAssembleFunction) (t_uint32 *, t_spe_ins *);

/*! The opcode information structure. */
typedef struct _t_spe_opcode_info
{
    /*! Textual name of the opcode */
    t_string name;
    /*! Opcode's extracted value */
    t_uint32 opcode;
    /*! Opcode's extractor mask */
    t_uint32 mask;
    /*! Instruction type */
    t_uint16 type;
    /*! Immediate starting bit */
    t_uint16 immstart;
    /*! Immediate size */
    t_uint16 immsize;
    /*! Immediate transformation */
    t_spe_imm_trans immtrans;
    /*! Affected slots */
    t_spe_slot_usage slots;
    /*! Disassembly function */
    SpeDisassembleFunction Disassemble;
    /*! Assembly function */
    SpeAssembleFunction Assemble;
    /*! Print format */
    char *print;
    /*! Opcode attributes */
    t_instruction_flags attribs;
    /*! Spe-specific opcode flags */
    t_spe_ins_flags flags;
} t_spe_opcode_info;

/*! The opcode table */
extern t_spe_opcode_info *spe_opcode_table;

void SpeOpcodesInit ();
void SpeOpcodesFini ();

#endif

/* }@ */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
