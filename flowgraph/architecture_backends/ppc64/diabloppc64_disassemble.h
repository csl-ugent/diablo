/*
 * Copyright (C) 2006 Lluis Vilanova <xscript@gmx.net> {{{
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
 */

/*!\addtogroup PPC64_BACKEND @{ */

#include <diabloobject.h>

#ifndef PPC64_DISASSEMBLE_H
#define PPC64_DISASSEMBLE_H
/* Includes {{{ */
#include <diablosupport.h>
/* }}} */

//void * PpcDisassembleOneInstruction(t_object * obj, t_address start, int * size_ret);
//void * PpcDisassembleOneInstructionForSection(t_section * sec, t_uint32 offset, int * size_ret);
void DiabloFlowgraphPpc64CfgCreated (t_object *obj, t_cfg *cfg);
void Ppc64DisassembleSection (t_section *code);
#endif

/* @} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
