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

void SpePrintParse (char *, t_spe_ins *, char *);

/*! Print an instruction.
 *
 * \param ins  The instruction to print
 * \param out  The output string
 * \return void
 */
void
SpeInstructionPrint (t_ins *ins, t_string out)
{
    /* assume outputstring always has enough room - say at least 80 characters
     * (this is way too much but it's safe) */
    t_spe_ins *s_ins = T_SPE_INS (ins);
    t_spe_opcode_info *info;

    info = &spe_opcode_table[SPE_INS_OPCODE (s_ins)];

    if (info->print[0] == '\0')
    {
        sprintf (out, "%-7s %-69s", info->name, "...");
    }
    else
    {
        char instr[100];
        char *operands;

        SpePrintParse (info->print, s_ins, instr);

        operands = index (instr, ' ');
        if (operands)
        {
            operands[0] = '\0';
            sprintf (out, "%-11s %-69s", instr, operands + 1);
        }
        else
        {
            sprintf (out, "%s", instr);
        }
    }

    StringTrim (out);
}

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
