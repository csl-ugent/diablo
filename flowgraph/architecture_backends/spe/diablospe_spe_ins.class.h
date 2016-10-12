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

#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS spe_ins
#define spe_ins_field_select_prefix SPE_INS
#define spe_ins_function_prefix SpeIns
#endif

#ifdef SPE_INS_NEXT
#undef SPE_INS_NEXT
#endif

#ifdef SPE_INS_PREV
#undef SPE_INS_PREV
#endif


/*! \addtogroup SPE_BACKEND @{ */

/*! \brief This class is used to represent SPE instructions. 
 *
 * SPE specific code uses this representation, generic code uses the generic
 * representation */
DIABLO_CLASS_BEGIN

/*! The generic instruction */
EXTENDS(t_ins)

/*! The represented opcode */
MEMBER(t_spe_opcode, opcode, OPCODE)
/*! Instruction register field T (if any) */
MEMBER(t_reg, regt, REGT)
/*! Instruction register field B (if any) */
MEMBER(t_reg, regb, REGB)
/*! Instruction register field A (if any) */
MEMBER(t_reg, rega, REGA)
/*! Instruction register field C (if any) */
MEMBER(t_reg, regc, REGC)
/*! Original immediate operand (if any) */
MEMBER(t_address, immediate_orig, IMMEDIATE_ORIG)
/*! Transformed immediate operand (if any) */
MEMBER(t_address, immediate, IMMEDIATE)
/*! Original address referenced (only in 'hbr*') */
MEMBER(t_address, address_orig, ADDRESS_ORIG)
/*! Address referenced (calculated from immediate, if any, except in 'hbr*') */
MEMBER(t_address, address, ADDRESS)
/*! Instruction assembler function */
MEMBER(SpeAssembleFunction, assembler, ASSEMBLER)
/*! Special instruction flags */
MEMBER(t_spe_ins_flags, flags, FLAGS)
/*! Associated branch hint */
MEMBER(t_spe_opcode, bhint, BHINT)

DIABLO_CLASS_END

/* }@ */

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define SPE_INS_NEXT(x) ({ FATAL(("Do not use SPE_INS_NEXT: Use SPE_INS_INEXT instead")); NULL; })
#define SPE_INS_PREV(x) ({ FATAL(("Do not use SPE_INS_PREV: Use SPE_INS_IPREV instead")); NULL; })
/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
