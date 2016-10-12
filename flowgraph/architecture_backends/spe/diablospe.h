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
 *
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

/*! \defgroup SPE_BACKEND Synergestic Processor Element (SPE) Backend @{ */

#ifndef DIABLOSPE_H
#define DIABLOSPE_H

#ifdef GENERATE_CLASS_CODE
#  undef GENERATE_CLASS_CODE
#  include <diabloflowgraph.h>
#  define GENERATE_CLASS_CODE
#else /* !GENERATE_CLASS_CODE */
#  include <diabloflowgraph.h>
#endif

/* Class generation {{{ */
#ifndef DIABLOSPE_FUNCTIONS
#  ifndef DIABLOSPE_TYPES
#    define TYPEDEFS
#    include "diablospe_spe_ins.class.h"
#    undef TYPEDEFS 
#  else
#    define TYPES
#    include "diablospe_spe_ins.class.h"
#    undef TYPES
#  endif
#else
#  define DEFINES
#  include "diablospe_spe_ins.class.h"
#  undef DEFINES
#  define DEFINES2
#  include "diablospe_spe_ins.class.h"
#  undef DEFINES2
#  define FUNCTIONS
#  include "diablospe_spe_ins.class.h"
#  undef FUNCTIONS
#  define CONSTRUCTORS
#  include "diablospe_spe_ins.class.h"
#  undef CONSTRUCTORS
#endif
/* }}} */

/* Backend includes {{{ */
#include "diablospe_cmdline.h"
#include "diablospe_description.h"
#include "diablospe_registers.h"
#include "diablospe_opcodes.h"
#include "diablospe_print.h"
#include "diablospe_disassemble.h"
#include "diablospe_disassemble_one.h"
#include "diablospe_assemble.h"
#include "diablospe_assemble_one.h"
#include "diablospe_instructions.h"
#include "diablospe_flowgraph.h"
#include "diablospe_deflowgraph.h"
/* }}} */

#ifndef DIABLOSPE_TYPES
#  define DIABLOSPE_TYPES
#  undef DIABLOSPE_H
#  include <diablospe.h>
#endif

#ifndef DIABLOSPE_FUNCTIONS
#  define DIABLOSPE_FUNCTIONS
#  undef DIABLOSPE_H
#  include <diablospe.h>
void DiabloSpeInit (int, char **);
void DiabloSpeFini ();
#endif

#endif

/* }@ */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
