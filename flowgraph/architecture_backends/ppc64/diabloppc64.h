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

/*! \defgroup PPC64_BACKEND PowerPc 64bit Backend
 * \ingroup PPC_BACKEND
 * @{ */

#ifndef DIABLOPPC64_H
#define DIABLOPPC64_H

#include <diabloppc.h>
#include <diabloflowgraph.h>

void DiabloPpc64Init(int, char **);
void DiabloPpc64Fini();

/* Backend includes {{{ */
#include "diabloppc64_cmdline.h"
#include "diabloppc64_description.h"
#if 0
#include "diabloppc_registers.h"
#include "diabloppc64_instructions.h"
#endif
#include "diabloppc64_opcodes.h"
#include "diabloppc64_disassemble.h"
#include "diabloppc64_disassemble_one.h"
#if 0
#include "diabloppc_assemble.h"
#endif
#include "diabloppc64_assemble_one.h"
#if 0
#include "diabloppc_utils.h"
#endif
#include "diabloppc64_flowgraph.h"
#if 0
#include "diabloppc_print.h"
#endif
#include "diabloppc64_deflowgraph.h"
#if 0
#include <diabloflowgraph.h>
#endif
/* }}} */

#endif

/* }@ */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab: */
