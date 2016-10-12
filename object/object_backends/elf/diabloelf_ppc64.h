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

#ifndef DIABLOELF_PPC64_DEFINES
#define DIABLOELF_PPC64_DEFINES

#include <diabloelf.h>

/* PowerPC relocations defined by the ABI */
enum t_reloc_ppc64 {
    R_PPC64_NONE               =   0,
    R_PPC64_ADDR32             =   1,
    R_PPC64_ADDR24             =   2,
    R_PPC64_ADDR16             =   3,
    R_PPC64_ADDR16_LO          =   4,
    R_PPC64_ADDR16_HI          =   5,
    R_PPC64_ADDR16_HA          =   6,
    R_PPC64_ADDR14             =   7,
    R_PPC64_ADDR14_BRTAKEN     =   8,
    R_PPC64_ADDR14_BRNTAKEN    =   9,
    R_PPC64_REL24              =  10,
    R_PPC64_REL14              =  11,
    R_PPC64_REL14_BRTAKEN      =  12,
    R_PPC64_REL14_BRNTAKEN     =  13,
    R_PPC64_GOT16              =  14,
    R_PPC64_GOT16_LO           =  15,
    R_PPC64_GOT16_HI           =  16,
    R_PPC64_GOT16_HA           =  17,
    R_PPC64_COPY               =  19,
    R_PPC64_GLOB_DAT           =  20,
    R_PPC64_JMP_SLOT           =  21,
    R_PPC64_RELATIVE           =  22,
    R_PPC64_UADDR32            =  24,
    R_PPC64_UADDR16            =  25,
    R_PPC64_REL32              =  26,
    R_PPC64_PLT32              =  27,
    R_PPC64_PLTREL32           =  28,
    R_PPC64_PLT16_LO           =  29,
    R_PPC64_PLT16_HI           =  30,
    R_PPC64_PLT16_HA           =  31,
    R_PPC64_SECTOFF            =  33,
    R_PPC64_SECTOFF_LO         =  34,
    R_PPC64_SECTOFF_HI         =  35,
    R_PPC64_SECTOFF_HA         =  36,
    R_PPC64_ADDR30             =  37,
    R_PPC64_ADDR64             =  38,
    R_PPC64_ADDR16_HIGHER      =  39,
    R_PPC64_ADDR16_HIGHERA     =  40,
    R_PPC64_ADDR16_HIGHEST     =  41,
    R_PPC64_ADDR16_HIGHESTA    =  42,
    R_PPC64_UADDR64            =  43,
    R_PPC64_REL64              =  44,
    R_PPC64_PLT64              =  45,
    R_PPC64_PLTREL64           =  46,
    R_PPC64_TOC16              =  47,
    R_PPC64_TOC16_LO           =  48,
    R_PPC64_TOC16_HI           =  49,
    R_PPC64_TOC16_HA           =  50,
    R_PPC64_TOC                =  51,
    R_PPC64_PLTGOT16           =  52,
    R_PPC64_PLTGOT16_LO        =  53,
    R_PPC64_PLTGOT16_HI        =  54,
    R_PPC64_PLTGOT16_HA        =  55,
    R_PPC64_ADDR16_DS          =  56,
    R_PPC64_ADDR16_LO_DS       =  57,
    R_PPC64_GOT16_DS           =  58,
    R_PPC64_GOT16_LO_DS        =  59,
    R_PPC64_PLT16_LO_DS        =  60,
    R_PPC64_SECTOFF_DS         =  61,
    R_PPC64_SECTOFF_LO_DS      =  62,
    R_PPC64_TOC16_DS           =  63,
    R_PPC64_TOC16_LO_DS        =  64,
    R_PPC64_PLTGOT16_DS        =  65,
    R_PPC64_PLTGOT16_LO_DS     =  66,
    R_PPC64_TLS                =  67,
    R_PPC64_DTPMOD64           =  68,
    R_PPC64_TPREL16            =  69,
    R_PPC64_TPREL16_LO         =  70,
    R_PPC64_TPREL16_HI         =  71,
    R_PPC64_TPREL16_HA         =  72,
    R_PPC64_TPREL64            =  73,
    R_PPC64_DTPREL16           =  74,
    R_PPC64_DTPREL16_LO        =  75,
    R_PPC64_DTPREL16_HI        =  76,
    R_PPC64_DTPREL16_HA        =  77,
    R_PPC64_DTPREL64           =  78,
    R_PPC64_GOT_TLSGD16        =  79,
    R_PPC64_GOT_TLSGD16_LO     =  80,
    R_PPC64_GOT_TLSGD16_HI     =  81,
    R_PPC64_GOT_TLSGD16_HA     =  82,
    R_PPC64_GOT_TLSLD16        =  83,
    R_PPC64_GOT_TLSLD16_LO     =  84,
    R_PPC64_GOT_TLSLD16_HI     =  85,
    R_PPC64_GOT_TLSLD16_HA     =  86,
    R_PPC64_GOT_TPREL16_DS     =  87,
    R_PPC64_GOT_TPREL16_LO_DS  =  88,
    R_PPC64_GOT_TPREL16_HI     =  89,
    R_PPC64_GOT_TPREL16_HA     =  90,
    R_PPC64_GOT_DTPREL16_DS    =  91,
    R_PPC64_GOT_DTPREL16_LO_DS =  92,
    R_PPC64_GOT_DTPREL16_HI    =  93,
    R_PPC64_GOT_DTPREL16_HA    =  94,
    R_PPC64_TPREL16_DS         =  95,
    R_PPC64_TPREL16_LO_DS      =  96,
    R_PPC64_TPREL16_HIGHER     =  97,
    R_PPC64_TPREL16_HIGHERA    =  98,
    R_PPC64_TPREL16_HIGHEST    =  99,
    R_PPC64_TPREL16_HIGHESTA   = 100,
    R_PPC64_DTPREL16_DS        = 101,
    R_PPC64_DTPREL16_LO_DS     = 102,
    R_PPC64_DTPREL16_HIGHER    = 103,
    R_PPC64_DTPREL16_HIGHERA   = 104,
    R_PPC64_DTPREL16_HIGHEST   = 105,
    R_PPC64_DTPREL16_HIGHESTA  = 106,
};


#ifndef DIABLOELF_PPC64_FUNCTIONS
#define DIABLOELF_PPC64_FUNCTIONS

t_bool IsElfPpc64SameEndian(FILE * fp);
void ElfReadPpc64SameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWritePpc64SameEndian (FILE * fp, t_object * obj);
void *Ppc64ResolveTocsAndAddLinkerStubs(t_ast_successors *succ, void *data);

#endif

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab: */
