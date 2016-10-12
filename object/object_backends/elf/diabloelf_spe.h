/*
 * Copyright (C) 2007 Lluis Vilanova <xscript@gmx.net> {{{
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

#ifndef DIABLOELF_SPE_DEFINES
#define DIABLOELF_SPE_DEFINES

#include <diabloelf.h>

/* SPU relocations defined by the ABI */
enum t_reloc_spu {
    R_SPU_NONE      =  0,
    R_SPU_ADDR10    =  1,
    R_SPU_ADDR16    =  2,
    R_SPU_ADDR16_HI =  3,
    R_SPU_ADDR16_LO =  4,
    R_SPU_ADDR18    =  5,
    R_SPU_GLOB_DAT  =  6,
    R_SPU_REL16     =  7,
    R_SPU_ADDR7     =  8,
    R_SPU_REL9      =  9,
    R_SPU_REL9I     = 10,
    R_SPU_ADDR10I   = 11,
};

#ifndef DIABLOELF_SPE_FUNCTIONS
#define DIABLOELF_SPE_FUNCTIONS

t_bool IsElfSpeSameEndian(FILE * fp);
t_bool IsElfSpeSwitchedEndian(FILE * fp);
void ElfReadSpeSameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadSpeSwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWriteSpeSameEndian (FILE * fp, t_object * obj);
void ElfWriteSpeSwitchedEndian (FILE * fp, t_object * obj);

void *SpeAddOverlayStubs(t_ast_successors *succ, void *data);
#endif

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab: */
