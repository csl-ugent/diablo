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

#ifndef SPE_REGISTERS_H
#define SPE_REGISTERS_H

#include <diablospe.h>

/*! \addtogroup SPE_BACKEND @{ */

/* Register fields inside a raw instruction {{{ */

/*! Indicates a register field inside a raw instruction */
typedef enum
{
    SPE_REGFIELD_T,
    SPE_REGFIELD_FIRST = SPE_REGFIELD_T,
    SPE_REGFIELD_B,
    SPE_REGFIELD_A,
    SPE_REGFIELD_C,
    SPE_REGFIELD_MAX
} t_spe_reg_field;

/*! Starting bit of a register field inside a raw instruction */
extern t_uint16 spe_reg_field_start[SPE_REGFIELD_MAX];

/*! Size of a single register field inside a raw instruction */
#define SPE_REGFIELD_SIZE 7

/* }}} */

/* Register slots {{{ */

/*! Register slot usage definition.
 *
 * This type is a string with 4 fields, each separated by '-'.
 *
 * The fields are, in this order: RT (only RRR format), RB, RA, RT/RC (stored in REGC).
 * 
 * Each field can be:
 *  '_'         No slot is affected
 *  '<char>'    <char> slot is affected on its default position
 *
 * Where <char> can be:
 *  'b'         Byte slot (3)
 *  'h'         Halfword slot (2:3)
 *  'a'         Address slot (0:3)
 *  'w'         Word slot (0:3)
 *  'd'         Doubleword slot (0:7)
 *  'q'         Quadword slot (0:15)
 *
 * If <char> is lowercase, it means the slot is read, uppercase means the slot
 * is written into.
 *
 * Multiple <char> definitions can be concatenated in a single field (wW: a word
 * slot is read and then written into).
 *
 * There is a special slot 'f' (foo) which is used to indicate that the register
 * field must be retrieved from the raw instruction, but it's not referring a ny
 * real register (a bitfield, etc.).
 *
 * Each slot definition can have some prefixed modifiers:
 *  's'         This is a special-purpose register
 *
 * \TODO: Should provide means for definition of slot position?
 */
typedef t_string t_spe_slot_usage;

/*! Identify a slot inside a register. */
typedef enum
{
    SPE_SLOT_NONE,
    SPE_SLOT_B,
    SPE_SLOT_H,
    SPE_SLOT_W,
    SPE_SLOT_A = SPE_SLOT_W,
    SPE_SLOT_D,
    SPE_SLOT_Q,
    SPE_SLOT_F
} t_spe_slot;

/* }}} */

/* Registers {{{ */

/*! Number of general-purpose registers */
#define SPE_REG_NGPR 128
/*! Number of special-purpose registers */
#define SPE_REG_NSPR 128
/*! Number of registers */
#define SPE_REG_MAX (SPE_REG_NGPR + SPE_REG_NSPR + 1)

/*! LR identifier */
#define SPE_REG_LR 0

/*! SP identifier */
#define SPE_REG_SP 1

/*! FPSCR identifier */
#define SPE_REG_FPSCR SPE_REG_NGPR

/*! No-register identifier */
#define SPE_REG_NONE SPE_REG_MAX

/* }}} */


struct _t_spe_opcode_info;
void SpeDisassembleRegisters (t_uint32 raw, t_spe_ins *ins, struct _t_spe_opcode_info *info);
void SpeAssembleRegisters (t_uint32 *raw, t_spe_ins *ins, struct _t_spe_opcode_info *info);

/* }@ */

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
