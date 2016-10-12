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

/*! Special treatment for Hint-for-Branch instructions
 *
 * \TODO: mark inline prefetching bit in 'hbr' ?
 */
void
SpeDisassembleHfB (t_uint32 raw, t_spe_ins * ins, t_uint16 opc)
{
    t_spe_ins_flags flags;
    t_address RO, ROH, ROL;

    switch (opc)
    {
        case SPE_HBR:
            flags = SPE_INS_FLAGS (ins);
            if (Bitfield32Get (raw, 11, 1))
                flags |= IF_SPE_INLINE_PREFETCHING;
            SPE_INS_SET_FLAGS (ins, flags);

            SPE_INS_SET_IMMEDIATE_ORIG (ins, AddressNewForIns (T_INS(ins), SPE_INS_REGA (ins)));
            SPE_INS_SET_IMMEDIATE (ins, AddressNewForIns (T_INS(ins), SPE_INS_REGA (ins)));

            ROH = AddressNewForIns (T_INS(ins), Bitfield32Get (raw, 16, 2));
            break;
        case SPE_HBRA:
        case SPE_HBRR:
            ROH = AddressNewForIns (T_INS(ins), Bitfield32Get (raw, 7, 2));
            break;
        default:
            FATAL(("Unknown opcode for %d hint-for-branch instruction %x", opc, raw));
    }

    ROH = AddressSignExtend (ROH, 1);
    ROL = AddressNewForIns (T_INS(ins), Bitfield32Get (raw, 25, 7));
    RO = AddressShl (AddressOr (AddressShl (ROH,
                                            AddressNewForIns (T_INS(ins), 7)),
                                ROL),
                     AddressNewForIns (T_INS(ins), 2));
    SPE_INS_SET_ADDRESS_ORIG (ins, RO);
    SPE_INS_SET_ADDRESS (ins, AddressAdd (RO, SPE_INS_CADDRESS (ins)));
}

/*! Special treatment for FPSCR accessor instructions */
void
SpeDisassembleFPSCR (t_uint32 raw, t_spe_ins * ins, t_uint16 opc)
{
    t_regset used, defined;

    used = SPE_INS_REGS_USE (ins);
    defined = SPE_INS_REGS_DEF (ins);

    switch (opc)
    {
        case SPE_FSCRWR:
            RegsetSetAddReg (used, SPE_REG_FPSCR);
            break;
        case SPE_FSCRRD:
            RegsetSetAddReg (used, SPE_REG_FPSCR);
            break;
        default:
            FATAL(("Unknown opcode for %d FPSCR accessor instruction %x", opc, raw));
    }

    SPE_INS_SET_REGS_USE (ins, used);
    SPE_INS_SET_REGS_DEF (ins, defined);
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
