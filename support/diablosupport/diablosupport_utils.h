/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

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
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 */

#ifndef DIABLOSUPPORT_UTILS_H
#define DIABLOSUPPORT_UTILS_H

#include <diablosupport.h>

/* Bit manipulators (bitfields & bitmasks) {{{ */

/*! Create a bitmask of 'size' bits
 *
 * This is declared as a function to avoid some noisy warnings.
 */
static inline t_uint32 Bitmask(int size)
{
  if (size >= 32)
    return 0xffffffff;
  else
    return (0x1 << size) - 1;
}
/* #define Bitmask(size) ((0x1 << (size)) - 1) */
/*! Create a bitmask of 'size' bits, with a right padding offset of 'off' bits */
#define BitmaskOffset(size, off) (Bitmask((size)+(off)) & ~Bitmask(off))

/* Bitfield manipulators {{{ */
/*! Move a bitfiled of size 'size' from 'bf' 'offset' bits left */
#define BitfieldMove(bf, size, offset) (((bf) << (offset)) & (BitmaskOffset((size),(offset))))
/*! Reset the last 'size' bits (offseted 'offset' bits to the left) from 'bf' */
#define BitfieldReset(bf, size, offset) ((bf) & ~(BitmaskOffset(size, offset)))
/* }}} */

/* Bitfield getters {{{ */
/*! Get the trailing size (right-hand) of a bitfiled of size 'total', starting
 * at 'start' with a length of 'size' bits */
#define BitfieldRSize(start, size, total) ((total) - (start) - (size))
/*! Get a bitfield from 'bf' ('total' bits) of 'size' bits, starting at 'start'
 *
 * The "last" bit is moved to the rightmost position.
 */
#define BitfieldGet(bf, start, size, total) (( (bf) & BitmaskOffset(size, BitfieldRSize(start, size, total)) ) >> BitfieldRSize(start, size, total))
/*! Get a bitfield from 'bf' (32 bits) of 'size' bits, starting at 'start'
 *
 * 'start' is meant to be bit 'start' from the leftmost side.
 */
#define Bitfield32Get(bf, start, size)      BitfieldGet(bf, start, size, 32)
/* }}} */

/* Bitfield setters {{{ */
/*! Set bitfield 'bf' ('total' bits) with the lower 'size' bits of 'data', replacing from 'start'*/
#define BitfieldSet(bf, data, start, size, total) (BitfieldReset(bf, size, BitfieldRSize(start,size,total)) | BitfieldMove(data, size, BitfieldRSize(start,size,total)))
/*! Set bitfield 'bf' (32 bits) with the lower 'size' bits of 'data', replacing from 'start'*/
#define Bitfield32Set(bf, data, start, size) BitfieldSet(bf, data, start, size, 32)
/* }}} */

/* }}} */

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
