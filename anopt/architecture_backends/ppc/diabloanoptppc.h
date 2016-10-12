/*
 * Copyright (C) 2005, 2006, 2007 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Jonas Maebe <Jonas.Maebe@elis.ugent.be>
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

#ifndef DIABLOANOPT_PPC_H
#define DIABLOANOPT_PPC_H

#include <diabloanopt.h>
#include <diabloppc.h>
#include "diabloanoptppc_cmdline.h"
#include "diabloanoptppc_stack.h"
#include "diabloanoptppc_factor.h"

#ifndef DIABLOANOPT_PPC_FUNCTIONS
#ifndef DIABLOANOPT_PPC_TYPES
#define TYPEDEFS
#undef TYPEDEFS 
#else
#define TYPES
#undef TYPES
#endif
#else
#define DEFINES
#undef DEFINES
#define DEFINES2
#undef DEFINES2
#define FUNCTIONS
#undef FUNCTIONS
#define CONSTRUCTORS
#undef CONSTRUCTORS
#endif


#ifndef DIABLOANOPT_PPC_TYPES
#define DIABLOANOPT_PPC_TYPES
#undef DIABLOANOPT_PPC_H
#include <diabloanoptppc.h>
#endif

#ifndef DIABLOANOPT_PPC_FUNCTIONS
#define DIABLOANOPT_PPC_FUNCTIONS
#undef DIABLOANOPT_PPC_H
#include <diabloanoptppc.h>

void DiabloAnoptPpcInit(int, char **);
void DiabloAnoptPpcFini();

#endif
#endif
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
