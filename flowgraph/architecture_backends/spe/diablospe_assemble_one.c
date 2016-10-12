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
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diablospe.h>

void SpeAssembleData (t_uint32 *raw, t_spe_ins *ins)
{
    *raw = AddressExtractUint32(SPE_INS_IMMEDIATE(ins));
}

/*! Special treatment for Hint-for-Branch instructions */
void
SpeAssembleHfB (t_uint32 *raw, t_spe_ins * ins)
{
    t_address RO, ROH, ROL;

    RO = AddressSub (SPE_INS_ADDRESS (ins), SPE_INS_CADDRESS (ins));
    ROL = AddressShr (RO, AddressNewForIns (T_INS(ins), 2));
    ROH = AddressShr (ROL, AddressNewForIns (T_INS(ins), 7));

    switch (SPE_INS_OPCODE (ins))
    {
        case SPE_HBR:
            if (SPE_INS_FLAGS (ins) & IF_SPE_INLINE_PREFETCHING)
            {
                *raw = Bitfield32Set (*raw, 1, 11, 1);
                *raw = Bitfield32Set (*raw, AddressExtractUint32 (SPE_INS_IMMEDIATE (ins)), 18, SPE_REGFIELD_SIZE);
            }
            else
            {
                *raw = Bitfield32Set (*raw, AddressExtractUint32 (SPE_INS_IMMEDIATE (ins)), 18, SPE_REGFIELD_SIZE);
                *raw = Bitfield32Set (*raw, AddressExtractUint32 (ROH), 16, 2);
                *raw = Bitfield32Set (*raw, AddressExtractUint32 (ROL), 25, SPE_REGFIELD_SIZE);
            }
            break;
        case SPE_HBRA:
        case SPE_HBRR:
            *raw = Bitfield32Set (*raw, AddressExtractUint32 (ROH), 7, 2);
            *raw = Bitfield32Set (*raw, AddressExtractUint32 (ROL), 25, SPE_REGFIELD_SIZE);
            break;
        default:
            FATAL(("Unknown opcode for %d hint-for-branch instruction %x", SPE_INS_OPCODE (ins), raw));
    }
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
