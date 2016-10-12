/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Jonas Maebe <Jonas.Maebe@elis.ugent.be>
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

#include <diabloanoptppc.h>

/* PpcFunComputeStackSavedRegisters {{{*/
/* This function computes the registers that a procedure saved on the
 * call/stack frame. We shoud be conservative because assemble code can
 * easily modify the values on the on the frame. */
void PpcFunComputeStackSavedRegisters(t_cfg * cfg, t_function * ifun)
{

  /** TODO: Implement **/
  FUNCTION_SET_REGS_SAVED(ifun, NullRegs);
  return;

}
/*}}}*/

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
