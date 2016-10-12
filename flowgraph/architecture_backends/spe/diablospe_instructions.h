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

#ifndef SPE_INSTRUCTIONS_H
#define SPE_INSTRUCTIONS_H

#include <diablospe.h>

/*! \addtogroup SPE_BACKEND @{ */

#define SPE_INS_IS_CONDITIONAL(i) (SPE_INS_ATTRIB(i) & IF_CONDITIONAL)

#define SPE_INS_UPDATES_LINK_REG(i) (SPE_INS_FLAGS(i) & IF_SPE_UPDATES_LINK_REG)

#define BBL_FOREACH_SPE_INS(bbl,ins) 	for(ins=T_SPE_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=SPE_INS_INEXT(ins))
#define BBL_FOREACH_SPE_INS_R(bbl,ins)	for(ins=T_SPE_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=SPE_INS_IPREV(ins))
#define BBL_FOREACH_SPE_INS_SAFE(bbl,ins,tmp)   for(ins=T_SPE_INS(BBL_INS_FIRST(bbl)), tmp=ins?SPE_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?SPE_INS_INEXT(ins):0)
#define SECTION_FOREACH_SPE_INS(code,ins) for(ins=T_SPE_INS(SECTION_DATA(code)); ins!=NULL; ins=SPE_INS_INEXT(ins))

#define T_SPE_INS(spe_ins)            ((t_spe_ins *) spe_ins)

#define SPE_INS_SIZE 32

t_bool SpeInsHasSideEffect (t_spe_ins *ins);
t_bool SpeInsIsLoad (t_spe_ins *ins);
t_bool SpeInsIsStore (t_spe_ins *ins);
t_bool SpeInsIsProcedureCall (t_spe_ins *ins);
t_bool SpeInsIsIndirectCall (t_spe_ins *ins);
t_bool SpeInsIsUnconditionalJump (t_spe_ins *ins);
t_bool SpeInsIsControlFlow (t_spe_ins *ins);
t_bool SpeInsIsSystem (t_spe_ins *ins);
t_bool SpeInsIsRegularControlFlow (t_spe_ins *ins);
t_bool SpeInsIsControlFlowWithDisplacement (t_spe_ins *ins);
t_tristate SpeInsIsSyscallExit (t_spe_ins *ins);

void SpeInsMakeNop (t_ins *ins);
void SpeInsMakeLnop (t_ins *ins);
void SpeInsMakeHintForBranch (t_spe_ins *ins, t_spe_ins *branch);

/* }@ */

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
