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


#ifndef PPC_REGISTERS_H
#define PPC_REGISTERS_H

#include <diabloflowgraph.h> 

#define PPC_REG_INVALID   254
#define PPC_REG_NONE      255

#define PPC_GPR(i, x)     ((PPC_BITFIELD(i, x, 5)) + PPC_REG_R0 )
#define PPC_FPR(i, x)     ((PPC_BITFIELD(i, x, 5)) + PPC_REG_F0 )
#define PPC_CR(i, x)      ((PPC_BITFIELD(i, x, 3)) + PPC_REG_CR0 )
#define PPC_VR(i, x)      ((PPC_BITFIELD(i, x, 5)) + PPC_REG_ALTIVEC_VR0 )
#define PPC_SRR(i, x)      ((PPC_BITFIELD(i, x, 4)) + PPC_REG_SR0 )
#define PPC_SPR(i)        PpcRegisterSpecial(PPC_BITFIELD(i, 11, 10)) 
#define PPC_TBR(i)        PpcRegisterTimeBase(PPC_BITFIELD(i, 11, 10))
#define PPC_BIT_CR(i, x)  (PPC_BITFIELD(i, x, 5))

#define PPC_REG2NUM(x)    PpcRegisterToNumber(x)
/**
#define PPC_REG2NUM(x) ({               \
        int aux;                        \
        if (x >= PPC_REG_ALTIVEC_VR0)   \
          aux = x - PPC_REG_ALTIVEC_VR0;\
        else if (x >= PPC_REG_CR0)      \
          aux = x - PPC_REG_CR0;        \
        else if (x >= PPC_REG_F0)       \
          aux = x - PPC_REG_F0;         \
        else                            \
          aux = x - PPC_REG_R0;         \
        aux;                            \
      })
*/

#define PPC_ASM_SET_GPR(i, x, value)        PPC_BITFIELD_SET_VAL(i,  x,  5, (value - PPC_REG_R0 ))
#define PPC_ASM_SET_FPR(i, x, value)        PPC_BITFIELD_SET_VAL(i,  x,  5, (value - PPC_REG_F0))
#define PPC_ASM_SET_CR(i, x, value)         PPC_BITFIELD_SET_VAL(i,  x,  3, (value - PPC_REG_CR0))
#define PPC_ASM_SET_VR(i, x, value)         PPC_BITFIELD_SET_VAL(i,  x,  5, (value - PPC_REG_ALTIVEC_VR0))
#define PPC_ASM_SET_SRR(i, x, value)         PPC_BITFIELD_SET_VAL(i,  x,  4, (value - PPC_REG_SR0))
#define PPC_ASM_SET_SPR(i, value)           PPC_BITFIELD_SET_VAL(i, 11, 10, PpcSetRegisterSpecial((value)))
#define PPC_ASM_SET_TBR(i, value)           PPC_BITFIELD_SET_VAL(i, 11, 10, PpcSetRegisterTimeBase((value)))
#define PPC_ASM_SET_IMM(i, value)           PPC_BITFIELD_SET_VAL(i, 16, 16, (value))
#define PPC_ASM_SET_BIT_CR(i, x, value)     PPC_BITFIELD_SET_VAL(i,  x,  5, (value))

/* general purpose registers */

#define PPC_REG_R0		0
#define PPC_REG_R1		1
#define PPC_REG_R2		2
#define PPC_REG_R3		3
#define PPC_REG_R4		4
#define PPC_REG_R5		5
#define PPC_REG_R6		6
#define PPC_REG_R7		7
#define PPC_REG_R8		8
#define PPC_REG_R9		9
#define PPC_REG_R10		10
#define PPC_REG_R11		11
#define PPC_REG_R12		12
#define PPC_REG_R13		13
#define PPC_REG_R14		14
#define PPC_REG_R15		15
#define PPC_REG_R16		16
#define PPC_REG_R17		17
#define PPC_REG_R18		18
#define PPC_REG_R19		19
#define PPC_REG_R20		20
#define PPC_REG_R21		21
#define PPC_REG_R22		22
#define PPC_REG_R23		23
#define PPC_REG_R24		24
#define PPC_REG_R25		25
#define PPC_REG_R26		26
#define PPC_REG_R27		27
#define PPC_REG_R28		28
#define PPC_REG_R29		29
#define PPC_REG_R30		30
#define PPC_REG_R31		31
#define PPC_REG_F0		32
#define PPC_REG_F1		33
#define PPC_REG_F2		34
#define PPC_REG_F3		35
#define PPC_REG_F4		36
#define PPC_REG_F5		37
#define PPC_REG_F6		38
#define PPC_REG_F7		39
#define PPC_REG_F8		40
#define PPC_REG_F9		41
#define PPC_REG_F10		42
#define PPC_REG_F11		43
#define PPC_REG_F12		44
#define PPC_REG_F13		45
#define PPC_REG_F14		46
#define PPC_REG_F15		47
#define PPC_REG_F16		48
#define PPC_REG_F17		49
#define PPC_REG_F18		50
#define PPC_REG_F19		51
#define PPC_REG_F20		52
#define PPC_REG_F21		53
#define PPC_REG_F22		54
#define PPC_REG_F23		55
#define PPC_REG_F24		56
#define PPC_REG_F25		57
#define PPC_REG_F26		58
#define PPC_REG_F27		59
#define PPC_REG_F28		60
#define PPC_REG_F29		61
#define PPC_REG_F30		62
#define PPC_REG_F31		63
#define PPC_REG_CR0		64
#define PPC_REG_CR1		65
#define PPC_REG_CR2		66
#define PPC_REG_CR3		67
#define PPC_REG_CR4		68
#define PPC_REG_CR5		69
#define PPC_REG_CR6		70
#define PPC_REG_CR7		71
#define PPC_REG_LR		72
#define PPC_REG_CTR		73
#define PPC_REG_XER		74
#define PPC_REG_FPSCR		75

/* Virtual environment architecture */
#define PPC_REG_TBL             76 
#define PPC_REG_TBU              77 

/* Operating environment architecture */
#define PPC_REG_MSR             78 
#define PPC_REG_PVR             79 
#define PPC_REG_IBAT0U          80 
#define PPC_REG_IBAT0L          81 
#define PPC_REG_IBAT1U          82 
#define PPC_REG_IBAT1L          83 
#define PPC_REG_IBAT2U          84 
#define PPC_REG_IBAT2L          85 
#define PPC_REG_IBAT3U          86 
#define PPC_REG_IBAT3L          87 
#define PPC_REG_DBAT0U          88 
#define PPC_REG_DBAT0L          89 
#define PPC_REG_DBAT1U          90 
#define PPC_REG_DBAT1L          91 
#define PPC_REG_DBAT2U          92 
#define PPC_REG_DBAT2L          93 
#define PPC_REG_DBAT3U          94 
#define PPC_REG_DBAT3L          95 
#define PPC_REG_SDR1            96 
#define PPC_REG_SR0             97 
#define PPC_REG_SR1             98 
#define PPC_REG_SR2             99 
#define PPC_REG_SR3             90 
#define PPC_REG_SR4             101 
#define PPC_REG_SR5             102 
#define PPC_REG_SR6             103 
#define PPC_REG_SR7             104 
#define PPC_REG_SR8             105 
#define PPC_REG_SR9             106 
#define PPC_REG_SR10            107 
#define PPC_REG_SR11            108 
#define PPC_REG_SR12            109 
#define PPC_REG_SR13            110 
#define PPC_REG_SR14            111 
#define PPC_REG_SR15            112 
#define PPC_REG_DAR             113 
#define PPC_REG_DSISR           114 
#define PPC_REG_SPRG0           115 
#define PPC_REG_SPRG1           116 
#define PPC_REG_SPRG2           117 
#define PPC_REG_SPRG3           118 
#define PPC_REG_SRR0            119 
#define PPC_REG_SRR1            120 

#ifdef PPC_ALTIVEC_SUPPORT
/* Altivec */
#define PPC_REG_ALTIVEC_VRSAVE  121
#define PPC_REG_ALTIVEC_VSCR    122
#define PPC_REG_ALTIVEC_VR0     123
#define PPC_REG_ALTIVEC_VR1     124
#define PPC_REG_ALTIVEC_VR2     125
#define PPC_REG_ALTIVEC_VR3     126
#define PPC_REG_ALTIVEC_VR4     127
#define PPC_REG_ALTIVEC_VR5     128
#define PPC_REG_ALTIVEC_VR6     129
#define PPC_REG_ALTIVEC_VR7     130
#define PPC_REG_ALTIVEC_VR8     131
#define PPC_REG_ALTIVEC_VR9     132
#define PPC_REG_ALTIVEC_VR10     133
#define PPC_REG_ALTIVEC_VR11     134
#define PPC_REG_ALTIVEC_VR12     135
#define PPC_REG_ALTIVEC_VR13     136
#define PPC_REG_ALTIVEC_VR14     137
#define PPC_REG_ALTIVEC_VR15     138
#define PPC_REG_ALTIVEC_VR16     139
#define PPC_REG_ALTIVEC_VR17     140
#define PPC_REG_ALTIVEC_VR18     141
#define PPC_REG_ALTIVEC_VR19     142
#define PPC_REG_ALTIVEC_VR20     143
#define PPC_REG_ALTIVEC_VR21     144
#define PPC_REG_ALTIVEC_VR22     145
#define PPC_REG_ALTIVEC_VR23     146
#define PPC_REG_ALTIVEC_VR24     147
#define PPC_REG_ALTIVEC_VR25     148
#define PPC_REG_ALTIVEC_VR26     149
#define PPC_REG_ALTIVEC_VR27     150
#define PPC_REG_ALTIVEC_VR28     151
#define PPC_REG_ALTIVEC_VR29     152
#define PPC_REG_ALTIVEC_VR30     153
#define PPC_REG_ALTIVEC_VR31     154
#endif

/* function prototypes */
t_string PpcRegisterName(t_reg reg);

t_uint32 PpcRegisterSpecial(t_uint32 reg);

t_uint32 PpcRegisterTimeBase(t_uint32 reg);

t_uint32 PpcSetRegisterSpecial(t_uint32 reg);

t_uint32 PpcSetRegisterTimeBase(t_uint32 reg);

int PpcRegisterToNumber(int x);
/* macros */

/* Special Registers handling {{{ */
#define __PPC_SR_GET_REG(srs, srname)                                   \
  (srs[ PPC_SR_ ## srname ])

/* Getters {{{ */
/*! Check if a field has been modified */
#define PPC_SR_MARKED_VAL(srs, srname, fldname)                         \
  (PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname ## _MARK),                 \
                PPC_SR_ ## srname ## _ ## fldname,                      \
                PPC_SR_ ## srname ## _ ## fldname ## _SIZE))
/*! Check if a field has been modified */
#define PPC_SR_MARKED(srs, srname, fldname)                             \
  (PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname ## _MARK),                 \
                PPC_SR_ ## srname ## _ ## fldname, 1))

/*! Check if a field's value has been set */
#define PPC_SR_HAS_VAL(srs, srname, fldname)                            \
  (if (!PPC_SR_MARKED_VAL(srs, srname, fldname))                        \
     FATAL(("Special register %s:%s has not been marked", srname, fldname)); \
   PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname ## _SET),                  \
                PPC_SR_ ## srname ## _ ## fldname,                      \
                PPC_SR_ ## srname ## _ ## fldname ## _SIZE))
/*! Check if a field's value has been set */
#define PPC_SR_HAS(srs, srname, fldname)                                \
  (if (!PPC_SR_MARKED(srs, srname, fldname))                            \
     FATAL(("Special register %s:%s has not been marked", srname, fldname)); \
   PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname ## _SET),                  \
                PPC_SR_ ## srname ## _ ## fldname, 1))

/*! Get a field's multi-bit value */
#define PPC_SR_VAL(srs, srname, fldname)                                \
  (if (!PPC_SR_HAS_VAL(srs, srname, fldname))                           \
     FATAL(("Special register %s:%s has not been set", srname, fldname)); \
   PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname),                          \
                PPC_SR_ ## srname ## _ ## fldname,                      \
                PPC_SR_ ## srname ## _ ## fldname ## _SIZE))
/*! Get a field's single-bit value bit */
#define PPC_SR(srs, srname, fldname)                                    \
  (if (!PPC_SR_HAS(srs, srname, fldname))                               \
     FATAL(("Special register %s:%s has not been set", srname, fldname)); \
   PPC_BITFIELD(__PPC_SR_GET_REG(srs, srname),                          \
                PPC_SR_ ## srname ## _ ## fldname, 1))
/* }}} */

/* Markers {{{ */
#define __PPC_SR_MARK_VAL(srs, srname, fldname, size)                   \
  do {                                                                  \
    PPC_BITFIELD_SET_VAL(__PPC_SR_GET_REG(srs, srname ## _MARK),        \
                     PPC_SR_ ## srname ## _ ## fldname, size,           \
                     ((0x1 << size) - 1));                              \
  } while(0)
#define __PPC_SR_UNMARK_VAL(srs, srname, fldname, size)                 \
  do {                                                                  \
    PPC_BITFIELD_SET_VAL(__PPC_SR_GET_REG(srs, srname ## _MARK),        \
                     PPC_SR_ ## srname ## _ ## fldname, size,           \
                     0);                                                \
  } while(0)
/*! Mark an unknown multi-bit field as modified */
#define PPC_SR_MARK_VAL_UNK(srs, srname, start, size)                   \
  do {                                                                  \
    PPC_BITFIELD_SET_VAL(__PPC_SR_GET_REG(srs, srname ## _MARK),        \
                         (start), size, (((t_uint64)0x1 << size) - 1));           \
  } while(0)
/*! Mark a multi-bit field as modified */
#define PPC_SR_MARK_VAL(srs, srname, fldname)                           \
  __PPC_SR_MARK_VAL(srs, srname, fldname,                               \
                    PPC_SR_ ## srname ## _ ## fldname ## _SIZE)
/*! Mark a single-bit field as modified */
#define PPC_SR_MARK(srs, srname, fldname)                               \
  __PPC_SR_MARK_VAL(srs, srname, fldname, 1)
/*! Unmark a multi-bit field as modified */
#define PPC_SR_UNMARK_VAL(srs, srname, fldname)                           \
  __PPC_SR_UNMARK_VAL(srs, srname, fldname,                               \
                    PPC_SR_ ## srname ## _ ## fldname ## _SIZE)
/*! Unmark a single-bit field as modified */
#define PPC_SR_UNMARK(srs, srname, fldname)                               \
  __PPC_SR_UNMARK_VAL(srs, srname, fldname, 1)
/* }}} */

/* Setters {{{ */
#define __PPC_SR_SET_VAL_UNK(srs, srname, start, size, val)             \
  do {                                                                  \
    PPC_BITFIELD_SET_VAL(__PPC_SR_GET_REG(srs, srname),                 \
                         start, size, val);                             \
    PPC_BITFIELD_SET_VAL(__PPC_SR_GET_REG(srs, srname ## _SET),         \
                         start, size, (0x1 << size));                   \
    PPC_SR_MARK_VAL_UNK(srs, srname, start, size);                      \
  } while(0)
#define __PPC_SR_SET_VAL(srs, srname, fldname, size, val)               \
  __PPC_SR_SET_VAL_UNK(srs, srname, PPC_SR_ ## srname ## _ ## fldname, size, val);
/*! Set an unknown multi-bit field */
#define PPC_SR_SET_VAL_UNK(srs, srname, start, size, val)               \
  __PPC_SR_SET_VAL_UNK(srs, srname, start, size, val);
/*! Set a value on a multi-bit field */
#define PPC_SR_SET_VAL(srs, srname, fldname, val)                       \
  __PPC_SR_SET_VAL(srs, srname, fldname,                                \
                   PPC_SR_ ## srname ## _ ## fldname ## _SIZE, val)
/*! Set an unknown bit on a single-bit field */
#define PPC_SR_SET_UNK(srs, srname, start)                              \
  __PPC_SR_SET_VAL_UNK(srs, srname, start, 1, 1)
/*! Set a bit on a single-bit field */
#define PPC_SR_SET(srs, srname, fldname)                                \
  __PPC_SR_SET_VAL(srs, srname, fldname, 1, 1)
/*! Unset an unknown bit on a single-bit field */
#define PPC_SR_UNSET_UNK(srs, srname, start)                            \
  __PPC_SR_SET_VAL_UNK(srs, srname, start, 1, 0)
/*! Unset a bit on a single-bit field */
#define PPC_SR_UNSET(srs, srname, fldname)                              \
  __PPC_SR_SET_VAL(srs, srname, fldname, 1, 0)
/* }}} */

/*! Special register index */
#define PPC_SR_DECLARE(name) PPC_SR_ ## name, PPC_SR_ ## name ## _SET, PPC_SR_ ## name ## _MARK
enum _t_ppc_sreg {
  PPC_SR_DECLARE(CR),
  PPC_SR_DECLARE(XER),
  PPC_SR_DECLARE(FPSCR),
  PPC_SR_DECLARE(VSCR),
  /* total count */
  PPC_SR_NUM
};

/* CR (Condition Register) {{{ */
enum _t_ppc_sreg_cr {
  /* single-bit flags */
  PPC_SR_CR_LT,
  PPC_SR_CR_GT,
  PPC_SR_CR_EQ,
  PPC_SR_CR_SO,
  PPC_SR_CR_FX,
  PPC_SR_CR_FEX,
  PPC_SR_CR_VX,
  PPC_SR_CR_OX,
  /* multi-bit flags */
  PPC_SR_CR_CR0 = 0,
  PPC_SR_CR_CR0_SIZE = 4,
  PPC_SR_CR_CR1 = 4,
  PPC_SR_CR_CR1_SIZE = 4,
  PPC_SR_CR_CR2 = 8,
  PPC_SR_CR_CR2_SIZE = 4,
  PPC_SR_CR_CR3 = 12,
  PPC_SR_CR_CR3_SIZE = 4,
  PPC_SR_CR_CR4 = 16,
  PPC_SR_CR_CR4_SIZE = 4,
  PPC_SR_CR_CR5 = 20,
  PPC_SR_CR_CR5_SIZE = 4,
  PPC_SR_CR_CR6 = 24,
  PPC_SR_CR_CR6_SIZE = 4,
  PPC_SR_CR_CR7 = 28,
  PPC_SR_CR_CR7_SIZE = 4,
};
/* }}} */

/* XER (Fixed-Point Exception Register) {{{ */
/* XXX: this register is treated as a 32-bit register because the lower 32 bits
 *      are reserved */
enum _t_ppc_sreg_xer {
  /* single-bit flags */
  PPC_SR_XER_SO,
  PPC_SR_XER_OV,
  PPC_SR_XER_CA,
};
/* }}} */

/* FPSCR (Floating-Point Status and Control Register) {{{ */
enum _t_ppc_sreg_fpscr {
  /* single-bit flags */
  PPC_SR_FPSCR_FX,
  PPC_SR_FPSCR_FEX,
  PPC_SR_FPSCR_VX,
  PPC_SR_FPSCR_OX,
  PPC_SR_FPSCR_UX,
  PPC_SR_FPSCR_ZX,
  PPC_SR_FPSCR_XX,
  PPC_SR_FPSCR_VXSNAN,
  PPC_SR_FPSCR_VXISI,
  PPC_SR_FPSCR_VXIDI,
  PPC_SR_FPSCR_VXZDZ,
  PPC_SR_FPSCR_VXIMZ,
  PPC_SR_FPSCR_VXVC,
  PPC_SR_FPSCR_FR,
  PPC_SR_FPSCR_FI,
  PPC_SR_FPSCR_C,
  PPC_SR_FPSCR_FL,
  PPC_SR_FPSCR_FG,
  PPC_SR_FPSCR_FE,
  PPC_SR_FPSCR_FU,
  PPC_SR_FPSCR_VXSOFT = 21,
  PPC_SR_FPSCR_VXSQRT,
  PPC_SR_FPSCR_VXCVI,
  PPC_SR_FPSCR_VE,
  PPC_SR_FPSCR_OE,
  PPC_SR_FPSCR_UE,
  PPC_SR_FPSCR_ZE,
  PPC_SR_FPSCR_XE,
  PPC_SR_FPSCR_NI,
  /* multi-bit flags */
  PPC_SR_FPSCR_FPRF = 15,
  PPC_SR_FPSCR_FPRF_SIZE = 5,
  PPC_SR_FPSCR_FPCC = 16,
  PPC_SR_FPSCR_FPCC_SIZE = 4,
  PPC_SR_FPSCR_RN = 30,
  PPC_SR_FPSCR_RN_SIZE = 2
};
/* }}} */

/* VSCR (Vector Status and Control Register - Altivec) {{{ */
enum _t_ppc_sreg_vscr {
  /* single-bit flags */
  PPC_SR_VSCR_NJ = 15,
  PPC_SR_VSCR_SAT = 31,
};
/* }}} */

/* }}} */

#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
