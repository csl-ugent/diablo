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

#include <diabloppc64.h>

/* Ppc64DisassembleSwitch {{{ */
/*! Disassemble a piece of data of a switch table.
 *
 * \param ins
 * \param data0
 *
 * \return void
 */
void
Ppc64DisassembleSwitch (t_ppc_ins *ins, t_uint32 data0)
{
  PpcInsSet (T_PPC_INS (ins), PPC_DATA);
  PPC_INS_SET_TYPE (ins, IT_DATA);
  /* \TODO: what about IF_SWITCHJUMP? */
  PPC_INS_SET_ATTRIB (ins, IF_DATA | IF_SWITCHTABLE);
  PPC_INS_SET_IMMEDIATE (ins, AddressSignExtend (AddressNewForIns (T_INS(ins), data0), 31));
  PPC_INS_SET_ATTRIB (ins, IF_SWITCHTABLE);
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
