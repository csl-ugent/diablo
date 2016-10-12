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
#ifndef PPC_DISASSEMBLE_ONE_H
#define PPC_DISASSEMBLE_ONE_H
void PpcDisassembleD(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleUnknown(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleData(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleI(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleB(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleSC(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleD(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleDS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleXL(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleXFX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleXFL(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleXS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleXO(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleA(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleM(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleMD(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleMDS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
#ifdef PPC_ALTIVEC_SUPPORT
void PpcDisassembleAltivecVXA(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleAltivecVX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleAltivecVXR(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleAltivecX(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleAltivecXDSS(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
void PpcDisassembleAltivecUnknown(t_ppc_ins * ins, t_uint32 instr, t_uint16 opc);
#endif
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
