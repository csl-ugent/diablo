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


#ifndef DIABLOPPC_H
#define DIABLOPPC_H

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#endif

#include "diabloppc_cmdline.h"
#include "diabloppc_description.h"
#include "diabloppc_registers.h"
#include "diabloppc_instructions.h"
#include "diabloppc_opcodes.h"
#include "diabloppc_disassemble.h"
#include "diabloppc_disassemble_one.h"
#include "diabloppc_assemble.h"
#include "diabloppc_assemble_one.h"
#include "diabloppc_utils.h"
#include "diabloppc_flowgraph.h"
#include "diabloppc_print.h"
#include "diabloppc_deflowgraph.h"


#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#ifndef DIABLOPPC_FUNCTIONS
#ifndef DIABLOPPC_TYPES
#define TYPEDEFS
#include "diabloppc_ppc_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloppc_ppc_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloppc_ppc_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloppc_ppc_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloppc_ppc_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloppc_ppc_ins.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOPPC_TYPES
#define DIABLOPPC_TYPES
#undef DIABLOPPC_H
#include <diabloppc.h>
#endif

#ifndef DIABLOPPC_FUNCTIONS
#define DIABLOPPC_FUNCTIONS
#undef DIABLOPPC_H
#include <diabloppc.h>

void DiabloPpcInit(int, char **);
void DiabloPpcFini();
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
