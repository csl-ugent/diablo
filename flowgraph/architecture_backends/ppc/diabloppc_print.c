/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
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
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diabloppc.h>
#include <string.h>

/* PpcInstructionPrint {{{ */
void PpcPrintParse (char *, t_ppc_ins *, char *);

/*!
 * Prints an instruction, either by printing its mnemotecnic if defined, or by
 * indicating the name of its opcode.
 *
 * \param data          The instruction to print
 * \param outputstring  The output string
 *
 * \return void 
*/
void
PpcInstructionPrint (t_ins *data, t_string outputstring) 
{
  /* assume outputstring always has enough room - say at least 80 characters
   * (this is way too much but it's safe) */
  t_ppc_ins *ins = T_PPC_INS(data);
  const t_ppc_opcode_entry *entry;

   entry = &ppc_opcode_table[PPC_INS_OPCODE(ins)];

  if (entry->print[0] == '\0')
  {
    
    sprintf (outputstring, "%-7s %-69s", entry->name, "...");
  }
  else
  {
    char instr[100];
    char *operands;

    PpcPrintParse (entry->print, ins, instr);

    operands = index (instr, ' ');
    if (operands)
    {
      operands[0] = '\0';
      sprintf (outputstring, "%-11s%-69s", instr, operands + 1);
    }
    else
    {
      sprintf (outputstring, "%s", instr);
    }
  }
  StringTrim (outputstring);
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
