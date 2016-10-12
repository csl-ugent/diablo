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

t_uint16 spe_reg_field_start[SPE_REGFIELD_MAX] = {4, 11, 18, 25};

/* SpeDisassembleRegisters {{{ */
/*! Sets the used register in an SPE instruction, extracted from the raw
 * instruction, and returns it */
static t_reg
SpeDisassembleSetRegister (t_uint32 raw, t_spe_ins *ins, t_spe_reg_field rfield, t_spe_slot slot, t_bool special)
{
    t_reg res;

    res = Bitfield32Get (raw, spe_reg_field_start[rfield], SPE_REGFIELD_SIZE);
    if (special)
    {
        res += SPE_REG_NGPR;
    }
    ASSERT (0 <= res && res < SPE_REG_NONE, ("Invalid register identifier: %d", res));
    switch (rfield)
    {
        case SPE_REGFIELD_A:
            SPE_INS_SET_REGA (ins, res);
            break;
        case SPE_REGFIELD_C:
            SPE_INS_SET_REGC (ins, res);
            break;
        case SPE_REGFIELD_B:
            SPE_INS_SET_REGB (ins, res);
            break;
        case SPE_REGFIELD_T:
            SPE_INS_SET_REGT (ins, res);
            break;
        default:
            FATAL(("Register field out of bounds (this should _never_ happen ;))"));
            break;
    }

    return res;
}

/*! Set the register usage for an instruction */
void
SpeDisassembleRegisters (t_uint32 raw, t_spe_ins *ins, t_spe_opcode_info *info)
{
    char *su;
    t_spe_reg_field rfield = SPE_REGFIELD_FIRST;
    t_reg reg;
    t_regset used, defined;
    t_bool special;

    SPE_INS_SET_REGT (ins, SPE_REG_NONE);
    SPE_INS_SET_REGB (ins, SPE_REG_NONE);
    SPE_INS_SET_REGA (ins, SPE_REG_NONE);
    SPE_INS_SET_REGC (ins, SPE_REG_NONE);
    RegsetSetEmpty (used);
    RegsetSetEmpty (defined);

    if (info->slots)
    {
        special = FALSE;

        for (su = info->slots; *su; su++)
        {
            switch (*su)
            {
                case '-':
                    special = FALSE;
                    rfield++;
                    ASSERT(rfield < SPE_REGFIELD_MAX, ("Had more register slot definitions than expected for instruction %s", info->name));
                    break;
                case 's':
                    special = TRUE;
                    break;
                case 'b':
                case 'B':
                    reg = SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_B, special);
                    if (*su == 'b')
                        RegsetSetAddReg (used, reg);
                    else
                        RegsetSetAddReg (defined, reg);
                    break;
                case 'h':
                case 'H':
                    reg = SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_H, special);
                    if (*su == 'h')
                        RegsetSetAddReg (used, reg);
                    else
                        RegsetSetAddReg (defined, reg);
                    break;
                case 'a':
                case 'A':
                case 'w':
                case 'W':
                    reg = SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_W, special);
                    if (*su == 'w' || *su == 'a')
                        RegsetSetAddReg (used, reg);
                    else
                        RegsetSetAddReg (defined, reg);
                    break;
                case 'd':
                case 'D':
                    reg = SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_D, special);
                    if (*su == 'd')
                        RegsetSetAddReg (used, reg);
                    else
                        RegsetSetAddReg (defined, reg);
                    break;
                case 'q':
                case 'Q':
                    reg = SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_Q, special);
                    if (*su == 'q')
                        RegsetSetAddReg (used, reg);
                    else
                        RegsetSetAddReg (defined, reg);
                    break;
                case 'f':
                    SpeDisassembleSetRegister (raw, ins, rfield, SPE_SLOT_F, special);
                    break;
                case '_':
                    break;
                default:
                    FATAL(("Unknown register field character: %c", *su));
                    break;
            }
        }
    }

    /* REGC is only used when REGT is also */
    if (SPE_INS_REGT (ins) == SPE_REG_NONE && SPE_INS_REGC (ins) != SPE_REG_NONE)
    {
        SPE_INS_SET_REGT (ins, SPE_INS_REGC (ins));
        SPE_INS_SET_REGC (ins, SPE_REG_NONE);
    }

    SPE_INS_SET_REGS_USE (ins, used);
    SPE_INS_SET_REGS_DEF (ins, defined);
}
/* }}} */

/* SpeAssembleRegisters {{{ */
/*! Sets the used register into the final instruction */
static void
SpeAssembleSetRegister (t_uint32 *raw, t_spe_ins *ins, t_spe_reg_field rfield, t_spe_opcode_info *info, t_bool hadT, t_bool special)
{
    t_reg reg;

    switch (rfield)
    {
        case SPE_REGFIELD_A:
            reg = SPE_INS_REGA (ins);
            break;
        case SPE_REGFIELD_C:
            if (!hadT)
            {
                reg = SPE_INS_REGT (ins);
            }
            else
            {
                reg = SPE_INS_REGC (ins);
            }
            break;
        case SPE_REGFIELD_B:
            reg = SPE_INS_REGB (ins);
            break;
        case SPE_REGFIELD_T:
            reg = SPE_INS_REGT (ins);
            break;
        default:
            FATAL(("Register field out of bounds (this should _never_ happen ;))"));
            break;
    }

    if (special)
    {
        reg -= SPE_REG_NGPR;
    }
    ASSERT (0 <= reg && reg < SPE_REG_NONE, ("Invalid register identifier %d in @I for register field %d", reg, ins, rfield));
    *raw = Bitfield32Set (*raw, reg, spe_reg_field_start[rfield], SPE_REGFIELD_SIZE);
}

/*! Encode the used registers into the final instruction */
void
SpeAssembleRegisters (t_uint32 *raw, t_spe_ins *ins, t_spe_opcode_info *info)
{
    char *su;
    t_spe_reg_field rfield = SPE_REGFIELD_FIRST;
    t_bool hadT = FALSE;
    t_bool special = FALSE;

    for (su = info->slots; *su; su++)
    {
        switch (*su)
        {
            case '-':
                special = FALSE;
                rfield++;
                ASSERT(rfield < SPE_REGFIELD_MAX, ("Had more register slot definitions than expected for instruction %s", info->name));
                break;
            /* Prefixed modifiers */
            case 's':
                special = FALSE;
                break;
            /* Slot identifiers */
            case 'b':
            case 'B':
            case 'h':
            case 'H':
            case 'a':
            case 'A':
            case 'w':
            case 'W':
            case 'd':
            case 'D':
            case 'q':
            case 'Q':
            case 'f':
                if (rfield == SPE_REGFIELD_T)
                {
                    hadT = TRUE;
                }
                SpeAssembleSetRegister (raw, ins, rfield, info, hadT, special);
                break;
            case '_':
                break;
            default:
                FATAL(("Unknown register field character: %c", *su));
                break;
        }
    }
}
/* }}} */

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
