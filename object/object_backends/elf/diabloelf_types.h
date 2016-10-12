/*
 * Copyright 2001,2002: Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef MY_ELF_TYPES
#define MY_ELF_TYPES
#include <diablosupport.h>
typedef t_uint32 Elf32_Addr;   /* 4 bytes unsigned */
typedef t_uint16 Elf32_Half; /* 2 bytes unsigned */
typedef t_uint32  Elf32_Off;    /* 4 bytes unsigned file offset*/
typedef t_int32 Elf32_Sword;           /* 4 bytes signed large integer */
typedef t_uint32 Elf32_Word;   /* 4 bytes unsigned large integer */
typedef unsigned char Elf32_Byte;  /* 1 bytes unsigned small integer */

typedef t_uint64 Elf64_Addr;   /* 64 bits */
typedef t_uint16 Elf64_Half;   /* 16 bits */
typedef t_uint16 Elf64_Section;   /* 16 bits */
typedef t_uint64 Elf64_Off;
typedef t_uint32 Elf64_Word;   /* 32 bits */
typedef t_uint64 Elf64_Xword;  /* 64 bits */
typedef t_int64  Elf64_Sxword; /* 64 bits */
typedef unsigned char Elf64_Byte;  /* 1 bytes unsigned small integer */
#endif
