/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

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

#ifndef I386_REGISTERS_H
#define I386_REGISTERS_H

#include <diabloflowgraph.h> 

#define I386_REG_INVALID   254
#define I386_REG_NONE      255

/* general registers */
#define I386_REG_EAX	0
#define I386_REG_EBX	1
#define I386_REG_ECX	2
#define I386_REG_EDX	3
#define I386_REG_ESI	4
#define I386_REG_EDI	5
#define I386_REG_EBP	6
#define I386_REG_ESP	7

/* FPU data registers */
#define I386_REG_ST0    8
#define I386_REG_ST1    9
#define I386_REG_ST2    10
#define I386_REG_ST3    11 
#define I386_REG_ST4    12
#define I386_REG_ST5    13
#define I386_REG_ST6    14
#define I386_REG_ST7    15

/* segment registers */
#define I386_REG_CS	16
#define I386_REG_DS	17
#define I386_REG_ES	18
#define I386_REG_FS	19
#define I386_REG_GS	20
#define I386_REG_SS	21
                        
/* control registers */
#define I386_REG_CR0	22
#define I386_REG_CR1	23
#define I386_REG_CR2	24
#define I386_REG_CR3	25
#define I386_REG_CR4	26

/* debug registers */
#define I386_REG_DR0	27
#define I386_REG_DR1	28
#define I386_REG_DR2	29
#define I386_REG_DR3	30
#define I386_REG_DR4	31
#define I386_REG_DR5	32
#define I386_REG_DR6	33
#define I386_REG_DR7	34

/* XMM   registers */
#define I386_REG_XMM0	35
#define I386_REG_XMM1	36
#define I386_REG_XMM2	37
#define I386_REG_XMM3	38
#define I386_REG_XMM4	39
#define I386_REG_XMM5	40
#define I386_REG_XMM6	41
#define I386_REG_XMM7	42

/* status bits in the %eflags register */
#define I386_CONDREG_AF  43
#define I386_CONDREG_CF  44
#define I386_CONDREG_DF  45
#define I386_CONDREG_IF  46
#define I386_CONDREG_NT  47
#define I386_CONDREG_OF  48
#define I386_CONDREG_PF  49
#define I386_CONDREG_RF  50
#define I386_CONDREG_SF  51
#define I386_CONDREG_TF  52
#define I386_CONDREG_ZF  53

/* status bits in fp status register */
#define I386_CONDREG_C0  54
#define I386_CONDREG_C1  55 
#define I386_CONDREG_C2  56
#define I386_CONDREG_C3  57

#define I386_REG_FPCW 58

/* function prototypes */
t_string I386RegisterName(t_reg reg);

/* macros */
#define I386RegisterGetType(r) RegisterGetType(i386_description, (r))
#define I386IsGeneralPurposeReg(reg)	((reg >= I386_REG_EAX) && (reg <= I386_REG_ESP))
#define I386IsFloatingPointReg(reg)	((reg >= I386_REG_ST0) && (reg <= I386_REG_ST7))
#define I386IsControlReg(reg)		((reg >= I386_REG_CR0) && (reg <= I386_REG_CR4))
#define I386IsDebugReg(reg)		((reg >= I386_REG_DR0) && (reg <= I386_REG_DR7))
#define I386IsSegmentReg(reg)		((reg >= I386_REG_CS ) && (reg <= I386_REG_SS ))
#endif
