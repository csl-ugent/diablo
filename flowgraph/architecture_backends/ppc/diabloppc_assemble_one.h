/*
 * Copyright (C) 2005 {{{
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
#ifdef DIABLOPPC_FUNCTIONS
#ifndef PPC_ASSEMBLE_ONE_H
#define PPC_ASSEMBLE_ONE_H
void PpcAssembleD(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleUnknown(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleData(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleX(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleI(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleB(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleSC(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleD(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleDS(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleXL(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleXFX(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleXFL(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleXS(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleXO(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleA(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleM(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleMD(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleMDS(t_ppc_ins * ins, t_uint32 * instr);
#ifdef PPC_ALTIVEC_SUPPORT
void PpcAssembleAltivecVXA(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleAltivecVX(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleAltivecVXR(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleAltivecX(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleAltivecXDSS(t_ppc_ins * ins, t_uint32 * instr);
void PpcAssembleAltivecUnknown(t_ppc_ins * ins, t_uint32 * instr);
#endif
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
