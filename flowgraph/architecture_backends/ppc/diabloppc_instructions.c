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

/*PpcInsHasSideEffect{{{*/
t_bool 
PpcInsHasSideEffect(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins);

  /*TODO: Check if writes to condition registers are
    considered 'side effect' */
  /* they are not in case of ppc, since they are modelled explicitly here (JM) */

  /*TODO: Check for side effect of privileded instruc-
    tions */
  /*TODO: Check for side effect of ALTIVEC instructions */
  
  if (PPC_INS_TYPE(ppc_ins) == IT_STORE) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_FLT_STORE)
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_STORE_MULTIPLE) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_SWI) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_BRANCH) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_SYNC) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_CACHE) 
  {
    ret = TRUE;
  }   
  /* these (can) set exception flags */
  else if ((PPC_INS_OPCODE(ppc_ins)==PPC_MTFSF) ||
           (PPC_INS_OPCODE(ppc_ins)==PPC_MTFSFI) ||
           (PPC_INS_OPCODE(ppc_ins)==PPC_MTFSB0) ||
           (PPC_INS_OPCODE(ppc_ins)==PPC_MTFSB1) ||
  /* these set the store reservation flag */
           (PPC_INS_OPCODE(ppc_ins)==PPC_LWARX) ||
           (PPC_INS_OPCODE(ppc_ins)==PPC_LDARX))
  {
    ret = TRUE;
  }
 return ret;
}
/*}}}*/

/*PpcInsIsLoad{{{*/
t_bool 
PpcInsIsLoad(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins); 

  /*TODO: Check for loads in ALTIVEC ISA */
  
  if (PPC_INS_TYPE(ppc_ins) == IT_LOAD) 
  {
    ret = TRUE;
  }
  else if(PPC_INS_TYPE(ppc_ins) == IT_LOAD_MULTIPLE) 
  {
    ret = TRUE;
  }
  else if(PPC_INS_TYPE(ppc_ins) == IT_FLT_LOAD) 
  {
    ret = TRUE;
  }
  return ret;
}
/*}}}*/

/*PpcInsIsStore{{{*/
t_bool 
PpcInsIsStore(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins); 
  
  /*TODO: Check for stores in ALTIVEC ISA */
  
  if (PPC_INS_TYPE(ppc_ins) == IT_STORE) 
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_FLT_STORE)
  {
    ret = TRUE;
  }
  else if (PPC_INS_TYPE(ppc_ins) == IT_STORE_MULTIPLE) 
  {
    ret = TRUE;
  }
  return ret;
}
/*}}}*/

/*PpcInsIsProcedureCall{{{*/
t_bool 
PpcInsIsProcedureCall(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins); 
 
  if (PpcInsUpdatesLinkRegister(ppc_ins) &&
       ((PPC_INS_OPCODE(ppc_ins) == PPC_B) ||
       (PPC_INS_OPCODE(ppc_ins) == PPC_BC)))
  {
    ret = TRUE; 
  }    
  return ret;
}
/*}}}*/

/*PpcInsIsUnconditionalBranch{{{*/
t_bool 
PpcInsIsUnconditionalBranch(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins); 
  if (PPC_INS_TYPE(ppc_ins) == IT_BRANCH &&
      !PpcInsIsConditional(ppc_ins) &&
      PPC_INS_REGA(ppc_ins) == PPC_REG_NONE &&
      !(PPC_INS_REFERS_TO(ppc_ins)))
  {
    ret = TRUE;
  }
  return ret;
}
/*}}}*/

/*PpcInsIsControlTransfer{{{*/
t_bool 
PpcInsIsControlTransfer(t_ins * ins)
{
  t_bool ret = FALSE;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins);

  /*TODO:Check if there is any privileged 
    instruction that modifies the control */
  
  if (PPC_INS_TYPE(ppc_ins) == IT_BRANCH) 
  {
    ret = TRUE;
  }
  else if(PPC_INS_TYPE(ppc_ins) == IT_SWI) 
  {
    ret = TRUE;
  }
  return ret;
}
/*}}}*/

/*PpcInsIsSystemInstruction{{{*/
t_bool 
PpcInsIsSystemInstruction(t_ins * ins)
{
  /*
     VERBOSE (0,("@I",ins));
     VERBOSE (0,("Implement PpcInsIsSystemInstruction please!"));
     VERBOSE (0,("This function should return TRUE if an instruction"
     "has a side-effect on system level, like e.g. flushing the"
     "pipeline or clearing the cache or enabling the MMU"));
   */

  switch (PPC_INS_OPCODE(T_PPC_INS(ins)))
  {
    case PPC_MFMSR:
    case PPC_MFSR:
    case PPC_MFSRIN:
    case PPC_MTMSR:
    case PPC_MTMSRD:
    case PPC_MTSR:
    case PPC_MTSRIN:
    case PPC_RFID:
    case PPC_SC:
    case PPC_SLBIA:
    case PPC_SLBIE:
    case PPC_SLBMFEE:
    case PPC_SLBMFEV:
    case PPC_SLBMTE:
    case PPC_TLBIA:
    case PPC_TLBIE:
    case PPC_TLBSYNC:
      /* Power PC Operating Environment Instruction Set 
       * Privileged instructions */ 
      return TRUE;
    case PPC_MFSPR:
      switch(PPC_INS_REGT(T_PPC_INS(ins)))
      {
        case PPC_REG_XER:
        case PPC_REG_LR:
        case PPC_REG_CTR:
        //case PPC_REG_CTRL:
          return FALSE;
        default:
          return TRUE;
      }
    case PPC_MTSPR:
      /* Power PC Operating Environment Instruction Set 
       * Privileged instructions depending of the source register */
      switch(PPC_INS_REGT(T_PPC_INS(ins)))
      {
        case PPC_REG_XER:
        case PPC_REG_LR:
        case PPC_REG_CTR:
          return TRUE;
        default:
          return FALSE;
      }
    case PPC_ECIWX:
    case PPC_ECOWX:
    case PPC_EIEIO:
    case PPC_ICBI:
    case PPC_ISYNC:
    case PPC_LDARX:
    case PPC_LWARX:
    case PPC_MFTB:
    case PPC_STDCX_DOT:
    case PPC_STWCX_DOT:
    case PPC_SYNC:
      /* Power PC Virtual Environment Instruction Set */
      return TRUE;
    default: 
      return FALSE;
  }

  return FALSE;
}
/*}}}*/

/*PpcInsIsSyscallExit{{{*/
t_tristate 
PpcInsIsSyscallExit(t_ins * ins)
{
  t_tristate ret = NO;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins);
  t_int32 syscall_number;

  if (PPC_INS_OPCODE(ppc_ins) == PPC_SC) 
  {

    syscall_number = PpcInsGetSyscallNumber(ins);

    /*TODO: In other architectures they take proffit and 
      store the system_call number in a T_INS field */

    if(syscall_number == -1)
    {
      return PERHAPS;
    }
    else if (syscall_number == 0x1)
    {
      return YES;
    }
    else if (syscall_number == 0xea)
    {
      return YES;
    }
  } 
  return ret;
}
/*}}}*/

/*PpcInsGetSyscallNumber{{{*/
t_uint32 
PpcInsGetSyscallNumber(t_ins * ins)
{
  t_uint32 ret = -1;
  t_ppc_ins *ppc_ins = T_PPC_INS(ins);
  t_ppc_ins *iter;

  if (PPC_INS_OPCODE(ppc_ins) != PPC_SC) 
  {
    ret = -1;
  }
  else
  {
    for (iter = PPC_INS_IPREV(ppc_ins); iter; iter = PPC_INS_IPREV(iter))
    {
        if(!iter)
        {
          break;
        }
        /*TODO: Look for definition of R0. Normally a 'li 0,sys_number'
         * instruction is used. But, for just in case, we have to look
         * for another ways to define a sys_call */
        else if( PPC_INS_OPCODE(iter) == PPC_ADDI
            && PPC_INS_REGA(iter) == PPC_REG_R0
            && PPC_INS_REGT(iter) == PPC_REG_R0 )
        {
           break;
        }
    }

    if(!iter)
    {
      ret = -1;
    }
    else
    {
      ret = AddressExtractUint32 (PPC_INS_IMMEDIATE(iter));
    }
    //FATAL(("Syscall @iB @I\n r0 defined by 
    //@I\nIMPLEMENT!",PPC_INS_BBL(ppc_ins),ppc_ins,iter));
  }
  return ret;
}
/*}}}*/

/* PpcInsUnconditionalize {{{*/
t_bool 
PpcInsUnconditionalize(t_ins * ins)
{
  t_ppc_ins *ppc_ins = T_PPC_INS(ins);
  INS_SET_ATTRIB(ins, INS_ATTRIB(ins)&(~IF_CONDITIONAL));

  switch (PPC_INS_OPCODE(ppc_ins))
  {
    case PPC_BC:
    case PPC_BCLR:
    case PPC_BCCTR:
      PPC_INS_SET_BO(ppc_ins, PPC_BOU);
      break;
    default:
      FATAL(("Instruction @I can not be unconditionalized!",ins));
  }
  /*TODO: also remove unneeded instrucions from the current basic bloc */
  /*TODO: recalculate defined/used registers...*/
  return TRUE;
}
/*}}}*/

/* PpcInsAreIdentical{{{ */
t_bool PpcInsAreIdentical(t_ins * insa, t_ins * insb)
{

  t_ppc_ins *ins1 = T_PPC_INS(insa);
  t_ppc_ins *ins2 = T_PPC_INS(insb);

  /* Catch normal cases {{{
   */
  if((PPC_INS_OPCODE(ins1)!=PPC_INS_OPCODE(ins2))||
     (PPC_INS_REGT(ins1)!=PPC_INS_REGT(ins2))||
     (PPC_INS_REGA(ins1)!=PPC_INS_REGA(ins2))||
     (PPC_INS_REGB(ins1)!=PPC_INS_REGB(ins2))||
     (PPC_INS_REGC(ins1)!=PPC_INS_REGC(ins2))||
     //(PPC_INS_IMMEDIATE(ins2)!=PPC_INS_IMMEDIATE(ins2))|| checked afterwards
     (PPC_INS_FLAGS(ins1)!=PPC_INS_FLAGS(ins2))||
     (PPC_INS_BO(ins1)!=PPC_INS_BO(ins2))||
     (PPC_INS_CB(ins1)!=PPC_INS_CB(ins2))||
     //(PPC_INS_BH(ins1)!=PPC_INS_BH(ins2))|| Branch hint never mind 
     (PPC_INS_CT(ins1)!=PPC_INS_CT(ins2))||
     (PPC_INS_MASK(ins1)!=PPC_INS_MASK(ins2)))
  {
    return FALSE;
  }
  
  {
    int i;
    for (i = 0; i < PPC_SR_NUM; i++) {
      if (PPC_INS_SREGS(ins1)[i] != PPC_INS_SREGS(ins2)[i])
        return FALSE;
    }
  }
    /*}}}*/

  /* Catch branches {{{
   *
   * - direct (immediate) jumps
   * - no relocated
   *
   * */ 
  if((PPC_INS_OPCODE(ins1)==PPC_B||
     (PPC_INS_OPCODE(ins1)==PPC_BC)))
     {
       if(!(PPC_INS_REFERS_TO(ins1))&&!(PPC_INS_REFERS_TO(ins2)))
       {
         
         t_cfg_edge *e1,*e2;
         
         BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins1),e1)
         { 
           if (CFG_EDGE_CAT(e1) & (ET_CALL | ET_IPJUMP | ET_JUMP ))
           {
             break;
           }
         }
         
         BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins2),e2)
         {
           if (CFG_EDGE_CAT(e2) & (ET_CALL | ET_IPJUMP | ET_JUMP ))
           {
             break;
           }
         }
      
         if (e1 && e2)
         {
           if (CFG_EDGE_TAIL(e1) == CFG_EDGE_TAIL(e2))
           {
             return TRUE;
           }
           else
           {
             return FALSE;
           }
         }
       }
     }
    /*}}}*/

  /* Catch switch table jumps {{{
   *
   * - bcctrl 
   * - switch edges
   *
   */

  if(PPC_INS_OPCODE(ins1) == PPC_BCCTR)
  {
    t_cfg_edge * switch1, * switch2;
    int n1=0, n2=0;

    /* check if both switches have the same number of switch edges {{{*/
    BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins1),switch1)
    {
      if (CFG_EDGE_CAT(switch1) == ET_SWITCH)
      {
        n1++;
      }
    }

    BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins2),switch2)
    {
      if (CFG_EDGE_CAT(switch2) == ET_SWITCH)
      {
        n2++;
      }
    }

    if ((n1 != n2)||(n1 == 0))
    {
      return FALSE;
    }
    /*}}}*/
    
    /* check if all switch edges from switch1 correspond with those of switch {{{ */
    BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins1),switch1)
    {
      if (CFG_EDGE_CAT(switch1) != ET_SWITCH)
      {
        continue;
      }

      BBL_FOREACH_SUCC_EDGE(PPC_INS_BBL(ins2),switch2)
      {
        if (CFG_EDGE_CAT(switch2) == ET_SWITCH && CFG_EDGE_SWITCHVALUE(switch2) == CFG_EDGE_SWITCHVALUE(switch1))
        {
          break;
        }

        if (!switch2) 
        {
          return FALSE;
        }

        if (!AddressIsEq(
                         AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch1)),PPC_INS_OLD_ADDRESS(ins1)),
                         AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch2)),PPC_INS_OLD_ADDRESS(ins2))
                        )
           )
        {
          return FALSE;
        }
      }
    }
    /*}}}*/

    /*Sanity check*/
    FATAL(("Sanity check: @I == @I ?",ins1,ins2));

    return TRUE;
  }
  /*}}}*/

  /* Catch rest of cases {{{
   *
   * - Case A: Refer different relocatables 
   * - Case B: Only one refers a relocatable 
   * - Case C: Should have the same immediate value 
   * - TODO: Check more cases !!!*/


  /* Case A */
  if (PPC_INS_REFERS_TO(ins1) && PPC_INS_REFERS_TO(ins2))
  {
    if (RelocCmp(RELOC_REF_RELOC(PPC_INS_REFERS_TO(ins1)), RELOC_REF_RELOC(PPC_INS_REFERS_TO(ins2)), TRUE)!=0) 
    {
      return FALSE;
    }
  }
  /* Case B */
  else if (PPC_INS_REFERS_TO(ins1) || PPC_INS_REFERS_TO(ins2))
  {
      return FALSE;
  }
  /* Case C */
  else if(PPC_INS_FLAGS(ins1) & PPC_FL_IMM)
  {
      if(!AddressIsEq (PPC_INS_IMMEDIATE(ins1), PPC_INS_IMMEDIATE(ins2)))
      {
        return FALSE;
      }
  }
  /* }}} */

  return TRUE;
}
/* }}} */

/* PpcInsMakeNop {{{ */
/*! Turns any instruction into a NOP */
void
PpcInsMakeNop (t_ins *ins)
{
  t_reloc_ref *rr;
  t_ppc_ins *pins = T_PPC_INS (ins);

  PpcInsSet (pins, PPC_ORI);
  PPC_INS_SET_IMMEDIATE (pins, AddressNullForIns (ins));
  PPC_INS_SET_REGT (pins, PPC_REG_R0);
  PPC_INS_SET_REGA (pins, PPC_REG_R0);
  PPC_INS_SET_TYPE (pins, IT_DATAPROC);
  PPC_INS_SET_FLAGS(pins, PPC_FL_IMM );

  INS_SET_REGS_USE (ins, PpcUsedRegisters(pins));
  INS_SET_REGS_DEF (ins, PpcDefinedRegisters(pins));
  
  while ((rr = INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (pins)),
                          RELOC_REF_RELOC(rr));
  }
}
/* }}} */

/* PpcInsMakeUncondBranch {{{ */
/*! creates an unconditional branch instruction */
void
PpcInsMakeUncondBranch(t_ins *gins)
{
  t_ppc_ins *ins = T_PPC_INS(gins);
  t_reloc_ref * relref;
  t_cfg * cfg=PPC_INS_CFG(ins);
  
  t_reloc_table * reloc_table;
  
  ASSERT(cfg,("Section not set for instruction"));
  reloc_table=OBJECT_RELOC_TABLE(CFG_OBJECT(cfg));

  PpcInsSet(ins, PPC_B);
  PPC_INS_SET_TYPE(ins,IT_BRANCH);
  PPC_INS_SET_FLAGS(ins, PPC_FL_IMM );
  PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressSubUint32(BBL_CADDRESS(PPC_INS_BBL(ins)),BBL_NINS(PPC_INS_BBL(ins))*4-4),25));
  
  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));
  
  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  /* If this instruction pointed to something else (e.g. it produced an
    * address, or jumped, or ....) then it no longer does that. So we can remove
    * the reloc (and all associated structures). */
  
  relref = PPC_INS_REFERS_TO(ins);
  while (relref)
  {
    RelocTableRemoveReloc(reloc_table, RELOC_REF_RELOC(relref));
    relref = PPC_INS_REFERS_TO(ins);
  }

}
/* }}} */

/* PpcInsMakeBlr {{{ */
/*! creates a blr instruction */
void
PpcInsMakeBlr (t_ppc_ins *ins)
{
  t_reloc_ref *rr;

  PpcInsSet(ins, PPC_BCLR);
  PPC_INS_SET_REGA(ins,PPC_REG_LR);
  /* branch always */
  PPC_INS_SET_BO(ins, PPC_BOU);
  /* no condition nor branch hint */
  PPC_INS_SET_CB(ins, 0);
  PPC_INS_SET_BH(ins, 0);
  /* it's a branch */
  PPC_INS_SET_TYPE(ins,IT_BRANCH);
  PPC_INS_SET_FLAGS(ins, PPC_FL_IMM);
  
  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));
  
  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  while ((rr = PPC_INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (ins)),
                          RELOC_REF_RELOC(rr));
  }
}
/* }}} */

/* PpcInsMakeMtspr {{{ */
/*! creates an mtspr instruction
*/
void
PpcInsMakeMtspr(t_ppc_ins *ins, t_reg spr, t_reg src)
{
  t_reloc_ref *rr;
  PpcInsSet(ins, PPC_MTSPR);
  
  PPC_INS_SET_REGT(ins, spr);
  PPC_INS_SET_REGA(ins, src);
  PPC_INS_SET_TYPE(ins, IT_DATAPROC);
  PPC_INS_SET_FLAGS(ins, 0);

  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));

  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  while ((rr = PPC_INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (ins)),
                          RELOC_REF_RELOC(rr));
  }
}
/*}}}*/

/* PpcInsMakeMfspr {{{ */
/*! creates an mfspr instruction */
void
PpcInsMakeMfspr(t_ppc_ins *ins, t_reg dest, t_reg spr)
{
  t_reloc_ref *rr;
  PpcInsSet(ins, PPC_MFSPR);

  PPC_INS_SET_REGT(ins, dest);
  PPC_INS_SET_REGA(ins, spr);
  PPC_INS_SET_TYPE(ins, IT_DATAPROC);
  PPC_INS_SET_FLAGS(ins, 0);

  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));

  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  while ((rr = PPC_INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (ins)),
                          RELOC_REF_RELOC(rr));
  }
}
/* }}} */

/* PpcInsMakeCall {{{ */
/*! creates a bl instruction ins is expected to be cleared already */
void
PpcInsMakeCall(t_ppc_ins *ins)
{
  t_reloc_ref *rr;
  PpcInsSet(ins, PPC_B);

  PPC_INS_SET_IMMEDIATE(ins, AddressSignExtend (AddressSubUint32(BBL_CADDRESS(PPC_INS_BBL(ins)),BBL_NINS(PPC_INS_BBL(ins))*4-4),25));
  PPC_INS_SET_TYPE(ins,IT_BRANCH);
  PPC_INS_SET_FLAGS(ins, PPC_FL_IMM|PPC_FL_LINK);

  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));
  
  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  while ((rr = PPC_INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (ins)),
                          RELOC_REF_RELOC(rr));
  }
}
/* }}} */


static void
PpcInsMakeOpRegRegSimm16(t_ppc_ins *ins, t_ppc_opcode op, t_reg dst, t_reg src, t_int16 value)
{
  t_reloc_ref *rr;
  PpcInsSet(ins, op);
  
  PPC_INS_SET_REGT(ins, dst);
  PPC_INS_SET_REGA(ins, src);
  PPC_INS_SET_IMMEDIATE(ins, AddressNewForIns(T_INS(ins),value));
  PPC_INS_SET_TYPE(ins, IT_DATAPROC);
  PPC_INS_SET_FLAGS(ins, 0);

  PPC_INS_SET_REGS_USE (ins, PpcUsedRegisters(ins));
  PPC_INS_SET_REGS_DEF (ins, PpcDefinedRegisters(ins));

  PPC_INS_SET_CSIZE(ins, AddressNewForIns(T_INS(ins), 4));
  PPC_INS_SET_OLD_SIZE(ins, AddressNewForIns(T_INS(ins), 4));

  while ((rr = PPC_INS_REFERS_TO(ins)))
  {
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE (PPC_INS_OBJECT (ins)),
                          RELOC_REF_RELOC(rr));
  }
}
/*}}}*/

/* PpcInsMakeLi {{{ */
/*! creates an li (load immediate = addi rx,r0,simm16) instruction
*/
void
PpcInsMakeLi(t_ppc_ins *ins, t_reg dst, t_int16 value)
{
  PpcInsMakeOpRegRegSimm16(ins,PPC_ADDI,dst,PPC_REG_R0,value);
}
/*}}}*/


/* PpcInsMakeAddi {{{ */
/*! creates an addi  rD,rA,simm16) instruction
*/
void
PpcInsMakeAddi(t_ppc_ins *ins, t_reg dst, t_reg src, t_int16 value)
{
  PpcInsMakeOpRegRegSimm16(ins,PPC_ADDI,dst,src,value);
}
/*}}}*/



/* PpcInsMakeAddis {{{ */
/*! creates an addis  rD,rA,simm16) instruction
*/
void
PpcInsMakeAddis(t_ppc_ins *ins, t_reg dst, t_reg src, t_int16 value)
{
  PpcInsMakeOpRegRegSimm16(ins,PPC_ADDIS,dst,src,value);
}
/*}}}*/



/* PpcInsIsIndirectCall{{{*/
t_bool 
PpcInsIsIndirectCall(t_ins * ins)
{
  return
  ((PPC_INS_OPCODE(T_PPC_INS(ins)) == PPC_BCCTR) ||
   (PPC_INS_OPCODE(T_PPC_INS(ins)) == PPC_BCLR)) &&
  (PpcInsUpdatesLinkRegister(T_PPC_INS(ins)));
}

/*}}}*/

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
