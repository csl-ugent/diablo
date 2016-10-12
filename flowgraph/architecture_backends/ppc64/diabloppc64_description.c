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

t_architecture_description ppc64_description;

/* Ppc64DescriptionInit {{{ */
/*! Initialize the architecutre description.
 * This uses an inheritance-like way with the Ppc description. */
void
Ppc64DescriptionInit ()
{
    memcpy (&ppc64_description, &ppc_description, sizeof (ppc64_description));
    /* ppc32 may use r3-r4 to return 64 bit ints, ppc64 only uses r3 */
    ppc64_description.callee_may_return.regset[0] = 0x00000008;
    ppc64_description.return_regs.regset[1] = 0x00000008;
    /* elf/ppc32 uses f1-f8 for floating parameters, elf/ppc64 uses f1-f13 */
    ppc64_description.callee_may_use.regset[1] = 0x00003ffe;
    ppc64_description.argument_regs.regset[1] = 0x00003ffe;
    ppc64_description.DisassembleSec = Ppc64DisassembleSection;
    ppc64_description.Flowgraph = Ppc64Flowgraph;
    ppc64_description.Deflowgraph = Ppc64Deflowgraph;
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
