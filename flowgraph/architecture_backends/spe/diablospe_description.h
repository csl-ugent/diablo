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

/*! \addtogroup SPE_BACKEND @{ */

#ifndef SPE_DESCRIPTION_H
#define SPE_DESCRIPTION_H

#include <diablospe.h>

/*! Default size for the Local Store */
#define SPE_LSLR_DEFAULT 0x0003FFFF

/*! Current configured size for Local Store */
extern t_address SPE_LSLR;

extern t_architecture_description spe_description;

void SpeDescriptionInit ();

#endif

/* }@ */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
