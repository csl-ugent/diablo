/*
 * Copyright 2001,2002: Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef AMD64_REGISTERS_H
#define AMD64_REGISTERS_H

#include <diabloflowgraph.h> 

#define AMD64_REG_INVALID   254
#define AMD64_REG_NONE      255

/* general purpose registers */
#define AMD64_REG_RAX	0
#define AMD64_REG_RBX	1
#define AMD64_REG_RCX	2
#define AMD64_REG_RDX	3
#define AMD64_REG_RSI	4
#define AMD64_REG_RDI	5
#define AMD64_REG_RBP	6
#define AMD64_REG_RSP	7
#define AMD64_REG_R8	8
#define AMD64_REG_R9    9
#define AMD64_REG_R10   10
#define AMD64_REG_R11   11
#define AMD64_REG_R12   12
#define AMD64_REG_R13   13
#define AMD64_REG_R14   14
#define AMD64_REG_R15   15

/* FPU data registers - MMX registers*/
#define AMD64_REG_ST0   16
#define AMD64_REG_ST1   17
#define AMD64_REG_ST2   18
#define AMD64_REG_ST3   19 
#define AMD64_REG_ST4   20
#define AMD64_REG_ST5   21
#define AMD64_REG_ST6   22
#define AMD64_REG_ST7   23

/*SSE registers - XMM   registers */
#define AMD64_REG_XMM0  24
#define AMD64_REG_XMM1  25
#define AMD64_REG_XMM2  26
#define AMD64_REG_XMM3  27
#define AMD64_REG_XMM4  28
#define AMD64_REG_XMM5  29
#define AMD64_REG_XMM6  30
#define AMD64_REG_XMM7  31
#define AMD64_REG_XMM8  32
#define AMD64_REG_XMM9  33
#define AMD64_REG_XMM10 34
#define AMD64_REG_XMM11 35
#define AMD64_REG_XMM12 36
#define AMD64_REG_XMM13 37
#define AMD64_REG_XMM14 38
#define AMD64_REG_XMM15 39

/* segment registers */
#define AMD64_REG_CS	40
#define AMD64_REG_DS	41
#define AMD64_REG_ES	42
#define AMD64_REG_FS	43
#define AMD64_REG_GS	44
#define AMD64_REG_SS	45
                        
/* control registers */
#define AMD64_REG_CR0	46
#define AMD64_REG_CR1	47
#define AMD64_REG_CR2	48
#define AMD64_REG_CR3	49
#define AMD64_REG_CR4	50
#define AMD64_REG_CR5   51
#define AMD64_REG_CR6   52
#define AMD64_REG_CR7   53
#define AMD64_REG_CR8   54

/* debug registers */
#define AMD64_REG_DR0	55
#define AMD64_REG_DR1	56
#define AMD64_REG_DR2	57
#define AMD64_REG_DR3	58
#define AMD64_REG_DR4	59
#define AMD64_REG_DR5	60
#define AMD64_REG_DR6	61
#define AMD64_REG_DR7	62

/* status bits in the %eflags register */
#define AMD64_CONDREG_AF  63
#define AMD64_CONDREG_CF  64
#define AMD64_CONDREG_DF  65
#define AMD64_CONDREG_IF  66
#define AMD64_CONDREG_NT  67
#define AMD64_CONDREG_OF  68
#define AMD64_CONDREG_PF  69
#define AMD64_CONDREG_RF  70
#define AMD64_CONDREG_SF  71
#define AMD64_CONDREG_TF  72
#define AMD64_CONDREG_ZF  73

/* status bits in fp status register */
#define AMD64_CONDREG_C0  74
#define AMD64_CONDREG_C1  75 
#define AMD64_CONDREG_C2  76
#define AMD64_CONDREG_C3  77

#define AMD64_REG_RIP     78
/* function prototypes */
t_string Amd64RegisterName(t_reg reg);

/* macros */
#define Amd64IsGeneralPurposeReg(reg)	((reg >= AMD64_REG_RAX) && (reg <= AMD64_REG_R15))
#define Amd64IsFloatingPointReg(reg)	((reg >= AMD64_REG_ST0) && (reg <= AMD64_REG_ST7))
#define Amd64IsSSEReg(reg)		((reg >= AMD64_REG_XMM0)&& (reg <= AMD64_REG_XMM15))
#define Amd64IsSegmentReg(reg)    	((reg >= AMD64_REG_CS)&& (reg <= AMD64_REG_SS))
#define Amd64IsControlReg(reg)		((reg >= AMD64_REG_CR0) && (reg <= AMD64_REG_CR8))
#define Amd64IsDebugReg(reg)		((reg >= AMD64_REG_DR0) && (reg <= AMD64_REG_DR7))
#define Amd64IsStatusReg(reg)		((reg >= AMD64_REG_AF ) && (reg <= AMD64_REG_ZF ))
#define Amd64IsFPStatusReg(reg)         ((reg >= AMD64_REG_C0 ) && (reg <= AMD64_REG_C3 ))

#endif
