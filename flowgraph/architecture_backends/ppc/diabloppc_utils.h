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

#ifndef PPC_UTILS_H
#define PPC_UTILS_H
#define PpcInsUpdatesLinkRegister(x)       (PPC_INS_FLAGS(x) & PPC_FL_LINK)
#define PpcInsUpdatesCounterRegister(x)    (PPC_INS_FLAGS(x) & PPC_FL_CTR)
#define PpcInsAbsoluteAddress(x)            (PPC_INS_FLAGS(x) & PPC_FL_ABSOLUTE)
#define PpcInsIsNOOP(x)             ((PPC_INS_OPCODE(x) == PPC_ORI) && \
                                    (PPC_INS_REGA(x) == PPC_REG_R0 ) && \
                                    (PPC_INS_REGT(x) == PPC_REG_R0 ) && \
                                    (AddressIsEq (PPC_INS_IMMEDIATE(x), 0) ))

#define PpcInsMakeNOOP(x)           do { \
                                    PPC_INS_SET_OPCODE(x, PPC_ORI); \
                                    PPC_INS_SET_REGA(x, PPC_REG_R0); \
                                    PPC_INS_SET_REGT(x, PPC_REG_R0); \
                                    x->Assemble = ppc_opcode_table[PPC_ORI].Assemble; \
                                    PPC_INS_SET_IMMEDIATE(x, AddressNullForIns(T_INS(x)));\
                                     } while(0)

#define PpcInsInit(x)           do{ PPC_INS_SET_OPCODE(x, PPC_UNKNOWN); \
                                    PPC_INS_SET_REGT(x, PPC_REG_NONE); \
                                    PPC_INS_SET_REGA(x, PPC_REG_NONE); \
                                    PPC_INS_SET_REGB(x, PPC_REG_NONE); \
                                    PPC_INS_SET_REGC(x, PPC_REG_NONE); \
                                    PPC_INS_SET_BO(x, 0); \
                                    PPC_INS_SET_CB(x, PPC_CB_INVALID); \
                                    PPC_INS_SET_CT(x, 0); \
                                    PPC_INS_SET_MASK(x, 0); \
                                    PPC_INS_SET_BH(x, 0); \
                                    PPC_INS_SET_FLAGS(x,0); \
                                    x->Assemble = ppc_opcode_table[PPC_UNKNOWN].Assemble; \
                                    PPC_INS_SET_IMMEDIATE(x, AddressNullForIns(T_INS(x))); } while (0)

#define RealPpcInsNewForSec(s)  do { t_ppc_ins *ret = T_PPC_INS (InsNewForSec (s)); \
                                     PpcInsInit (ret); \
                                     (ret) \
                                   } while (0)

#define PpcInsSet(x,y)          do{ PPC_INS_SET_OPCODE(x,y); \
                                    x->Assemble = ppc_opcode_table[y].Assemble;\
                                    } while (0)
                                     
#endif

#ifdef DIABLOPPC_FUNCTIONS
#ifndef PPC_UTILS_FUNCTIONS
#define PPC_UTILS_FUNCTIONS
t_regset PpcUsedRegisters(t_ppc_ins * ins);
t_regset PpcDefinedRegisters(t_ppc_ins * ins);
t_bool PpcInsIsConditional(t_ppc_ins * ins);	
t_bool PpcInsIsNop(t_ppc_ins * ins);
t_bool PpcInsIsPicBcl(t_ppc_ins * ins);
t_ppc_ins * PpcFindInstructionThatDefinesRegister(t_ppc_ins * start,
                                                       t_regset reg);
/* t_address_generic PpcFindTargetAddressOfIndirectBranch(t_ppc_ins * ins); */
t_address PpcComputeRelocatedAddressValue(t_ppc_ins ** ins, t_reg reg, t_bool *found);
/* t_uint32 PpcLoadValueFromAddress(t_object *, t_address_generic value);  */
t_bool PpcEdgeKillAndUpdateBblRecursive(t_cfg_edge * edge);
t_bool PpcFunIsGlobal(t_function * fun);
#endif
#endif

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
