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

/* PpcUsedRegisters {{{ */
/*!
 * Returns a bitmask specifying which registers are used by this instruction.
 *
 * \param ins
 *
 * \return t_regset 
 */
t_regset 
PpcUsedRegisters(t_ppc_ins * ins) 
{
  t_regset mask = NullRegs;

  /*TODO: In compare instruccions, have we to mark the 
    SO bit of the XER as a source ? */

  /*TODO: In floating point instructions, have we to mark the
    rounding mode bit of the FPSCR as a source ? */

  /*TODO: Check Altivec ISA for special cases */

  /*TODO: treat the instruction with the PPC_FL_MULTIPLE flag */

  /* Handle NOP (which is actually ORI R0,R0,0)
   * and PIC bcl's (they "use" cr7 but at the same time
   * are unconditional -- this nonsensical combinartion
   * enables the cpu to detect it's a PIC bcl so it
   *  doesn't add the "return" address to its internal
   * call stack)
   */
  if (PpcInsIsNop(ins) ||
      PpcInsIsPicBcl(ins))
    return mask;
  
  /* Source registers {{{*/
  if(!(PPC_INS_FLAGS(ins)&PPC_FL_CR_LOGIC))
  {  
    if(PPC_INS_REGA(ins)!=PPC_REG_NONE)
    {
      t_bool rega_is_reg;
      
      if (PPC_INS_REGA(ins)==PPC_REG_R0)
      {
        switch (PPC_INS_OPCODE(ins))
        {
          case PPC_ADDI: case PPC_ADDIS:
          case PPC_LBZX: case PPC_LHZ:
          case PPC_LHZX: case PPC_LHA: case PPC_LHAX: case PPC_LWZ: case PPC_LWZX: case PPC_LD: case PPC_LDX: case PPC_LWA: case PPC_LWAX:
          case PPC_LFS: case PPC_LFSX: case PPC_LFD: case PPC_LFDX:
          case PPC_LHBRX: case PPC_LWBRX: case PPC_LWARX: case PPC_LDARX: 
          /* the disassembler reverses the register order of stmw, so here it is RA and not RB like
           * for other store instructions
           */
          case PPC_ECIWX: case PPC_LMW: case PPC_STMW:
          /* case PPC_DCBA: case PPC_DCBI: */ case PPC_DCBST: case PPC_DCBTST: case PPC_DCBZ: case PPC_DCBF: case PPC_ICBI:
            rega_is_reg = FALSE;
            break;
          default:
            rega_is_reg = TRUE;
            break;
        }
      }
      else
        rega_is_reg = TRUE;
      if (rega_is_reg)
        RegsetSetAddReg(mask,PPC_INS_REGA(ins));
    }
    if(PPC_INS_REGB(ins)!=PPC_REG_NONE)
    {
      t_bool regb_is_reg;
      
      if (PPC_INS_REGB(ins)==PPC_REG_R0)
      {
        switch (PPC_INS_OPCODE(ins))
        {
          case PPC_STB: case PPC_STBX: case PPC_STH: case PPC_STHX: case PPC_STD: case PPC_STDX:
          case PPC_STW: case PPC_STWX:
          case PPC_STFS: case PPC_STFSX: case PPC_STFD: case PPC_STFDX: case PPC_STFIWX:
          case PPC_STHBRX: case PPC_STWBRX: case PPC_STWCX_DOT: case PPC_STDCX_DOT:
          case PPC_ECOWX:
            regb_is_reg = FALSE;
            break;
          default:
            regb_is_reg = TRUE;
            break;
        }
      }
      else
        regb_is_reg = TRUE;
      if (regb_is_reg)
        RegsetSetAddReg(mask,PPC_INS_REGB(ins));
    }
    if(PPC_INS_REGC(ins)!=PPC_REG_NONE)
    {
      RegsetSetAddReg(mask,PPC_INS_REGC(ins));
    }
    /* rlwimi and rldimi also use the previous contents of
     * the target register
     */
    if ((PPC_INS_OPCODE(ins) == PPC_RLWIMI) ||
        (PPC_INS_OPCODE(ins) == PPC_RLDIMI))
    {
      RegsetSetAddReg(mask,PPC_INS_REGT(ins));
    }
  }
  /*}}}*/

  /* Counter Register {{{*/
  if(PPC_INS_FLAGS(ins)&PPC_FL_CTR)
  {
    RegsetSetAddReg(mask,PPC_REG_CTR);
  }
  /*}}}*/

  /* Jumps that use a condition bit from CR {{{*/
  if(PPC_INS_CB(ins)!=PPC_CB_INVALID)
  {
    RegsetSetAddReg(mask,PPC_REG_CR0 + (PPC_INS_CB(ins)/4));
  }
  /*}}}*/

  /* Condition Register Logical Instructions {{{*/
  if((PPC_INS_FLAGS(ins)&PPC_FL_CR_LOGIC))
  {  
    if(PPC_INS_REGA(ins)!=PPC_REG_NONE)
    {
      RegsetSetAddReg(mask,PPC_REG_CR0 + (PPC_INS_REGA(ins)/4));
    }
    if(PPC_INS_REGB(ins)!=PPC_REG_NONE)
    {
      RegsetSetAddReg(mask,PPC_REG_CR0 + (PPC_INS_REGB(ins)/4));
    } 
    if(PPC_INS_REGC(ins)!=PPC_REG_NONE)
    {
      RegsetSetAddReg(mask,PPC_REG_CR0 + (PPC_INS_REGC(ins)/4));
    }
  }
  /*}}}*/

  /* Instructions that use the carry bit from the XER {{{*/
  if((PPC_INS_FLAGS(ins)&PPC_FL_EXTENDED))
  {  
    RegsetSetAddReg(mask,PPC_REG_XER); 
  }
  /*}}}*/

  /* Special instruction MFCR {{{*/
  if(PPC_INS_OPCODE(ins)==PPC_MFCR)
  {
    RegsetSetAddReg(mask,PPC_REG_CR0);
    RegsetSetAddReg(mask,PPC_REG_CR1);
    RegsetSetAddReg(mask,PPC_REG_CR2);
    RegsetSetAddReg(mask,PPC_REG_CR3);
    RegsetSetAddReg(mask,PPC_REG_CR4);
    RegsetSetAddReg(mask,PPC_REG_CR5);
    RegsetSetAddReg(mask,PPC_REG_CR6);
    RegsetSetAddReg(mask,PPC_REG_CR7);
  }
  /*}}}*/

  return mask;
}
/* }}} */

/* PpcDefinedRegisters {{{ */
/*!
 * Returns a bitmask specifying which registers are modified by this
 * instruction.
 * 
 * \param ins
 *
 * \return t_regset 
 */
t_regset 
PpcDefinedRegisters(t_ppc_ins * ins) 
{
  t_regset mask = NullRegs;
  t_uint32 *sregs;

  /*TODO: treat the instruction with the PPC_FL_MULTIPLE flag */

  /* Handle NOP (which is actually ORI R0,R0,0) */
  if (PpcInsIsNop(ins))
    return mask;
  
  /* Generic case REG target {{{*/
  if(PPC_INS_REGT(ins)!=PPC_REG_NONE)
  {
    if (!(PPC_INS_FLAGS(ins)&PPC_FL_CR_LOGIC))
    {
      RegsetSetAddReg(mask,PPC_INS_REGT(ins));
    }
    else
    {
      RegsetSetAddReg(mask,PPC_REG_CR0 + (PPC_INS_REGT(ins)/4));
    }
  }
  /*}}}*/

  /* Special Registers {{{*/
  sregs = PPC_INS_SREGS(ins);

  if(PPC_SR_MARKED_VAL(sregs,CR,CR0))
  {
    RegsetSetAddReg(mask,PPC_REG_CR0);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR1))
  {
    RegsetSetAddReg(mask,PPC_REG_CR1);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR2))
  {
    RegsetSetAddReg(mask,PPC_REG_CR2);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR3))
  {
    RegsetSetAddReg(mask,PPC_REG_CR3);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR4))
  {
    RegsetSetAddReg(mask,PPC_REG_CR4);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR5))
  {
    RegsetSetAddReg(mask,PPC_REG_CR5);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR6))
  {
    RegsetSetAddReg(mask,PPC_REG_CR6);
  }
  if(PPC_SR_MARKED_VAL(sregs,CR,CR7))
  {
    RegsetSetAddReg(mask,PPC_REG_CR7);
  }

  if(sregs[PPC_SR_FPSCR] ||
     sregs[PPC_SR_FPSCR_SET] ||
     sregs[PPC_SR_FPSCR_MARK])
  {
    RegsetSetAddReg(mask,PPC_REG_FPSCR);
  }
  if(sregs[PPC_SR_XER] ||
     sregs[PPC_SR_XER_SET] ||
     sregs[PPC_SR_XER_MARK])
  {
    RegsetSetAddReg(mask,PPC_REG_XER);
  }
  if(sregs[PPC_SR_VSCR] ||
     sregs[PPC_SR_VSCR_SET] ||
     sregs[PPC_SR_VSCR_MARK])
  {
    RegsetSetAddReg(mask,PPC_REG_ALTIVEC_VSCR);
  }

  /* Counter Register */
  if(PPC_INS_FLAGS(ins)&PPC_FL_CTR)
  {
    RegsetSetAddReg(mask,PPC_REG_CTR);
  }
  
  /* Link Register */
  if(PPC_INS_FLAGS(ins)&PPC_FL_LINK)
  {
    RegsetSetAddReg(mask,PPC_REG_LR);
  }
  
  /* load with update (and store with update, although in */
  /* those cases REGT is already set)                     */
  if (PPC_INS_FLAGS(ins)&PPC_FL_WITH_UPDATE)
  {
    RegsetSetAddReg(mask,PPC_INS_REGA(ins));
  }
  
  /*}}}*/

  return mask;
}
/* }}} */

/* PpcInsIsConditional{{{*/
t_bool 
PpcInsIsConditional(t_ppc_ins * ins)
{
  if(PPC_INS_ATTRIB(ins) & IF_CONDITIONAL)
  {
    if((PPC_INS_BO(ins)&PPC_BOU)==PPC_BOU)
    {
      return FALSE;
    }
    else
    {
      return TRUE;
    }
  }
  else
  {
    return FALSE;
  }
}
/*}}}*/


/* PpcInsIsNop{{{*/
t_bool 
PpcInsIsNop(t_ppc_ins * ins)
{
  if ((PPC_INS_OPCODE(ins)==PPC_ORI) &&
      (PPC_INS_REGT(ins)==PPC_REG_R0) &&
      (PPC_INS_REGA(ins)==PPC_REG_R0) &&
      (AddressIsEq(PPC_INS_IMMEDIATE(ins),AddressNullForIns(T_INS(ins)))))
return TRUE;
  else
    return FALSE;
}
/*}}}*/


/* PpcInsIsNop{{{*/
t_bool
PpcInsIsPicBcl(t_ppc_ins * ins) {
  return
    (PPC_INS_OPCODE(ins) == PPC_BC) &&
    (((PPC_INS_FLAGS(ins) & (PPC_FL_LINK|PPC_FL_ABSOLUTE))) == PPC_FL_LINK) &&
    (PPC_INS_BO(ins) == PPC_BOU) &&
    (PPC_INS_CB(ins) == 31) &&
    (AddressIsEq(PPC_INS_IMMEDIATE(ins), AddressNewForIns(ins,4)));
}
/*}}}*/

/* PpcFindInstructionThatDefinesRegister{{{*/
t_ppc_ins * 
PpcFindInstructionThatDefinesRegister(t_ppc_ins * start, t_regset reg)
{
  t_ppc_ins * i_ins=start;

  VERBOSE(1,("PpcFindInstructionThatDefinesRegister ( @I ,...",start)); 
  
  if(RegsetIsEmpty(reg))
  {
    i_ins = NULL; 
    return i_ins;
  }

  while((i_ins=PPC_INS_IPREV(i_ins)))
  {
    VERBOSE(1,("Actual instruction: @I",i_ins));
    if (!PpcInsIsConditional(i_ins)
        && PpcInsIsControlTransfer(T_INS(i_ins))) 
    {
      i_ins = NULL; 
      return i_ins;
    }
    else if(RegsetIsSubset(PPC_INS_REGS_DEF(i_ins), reg))
    {
      return i_ins;
    }
  }
  return NULL;
}
/*}}}*/

/* PpcComputeRelocatedAddressValue {{{*/
t_address
PpcComputeRelocatedAddressValue(t_ppc_ins ** ins, t_reg reg, t_bool *found)
{
  t_regset regset = PpcDefinedRegisters(*ins);
  t_ppc_ins * definition = *ins;
  t_ppc_ins * prevdef;
  t_regset lookingfor = NullRegs;
  t_address address;
  t_address high, low; 
  t_bool foundprev;

  *found = FALSE;
  VERBOSE(1,("PpcComputeRelocatedAddressValue ( @I, %s )",*ins,PpcRegisterName(reg))); 
  RegsetSetAddReg(lookingfor, reg);

  if(!RegsetIn(regset,reg))  
  {
    VERBOSE(1,("Looking the definition of %s",PpcRegisterName(reg)));
    definition = (t_ppc_ins *) PpcFindInstructionThatDefinesRegister(definition,lookingfor);
  }

  if(!definition)
  {
    VERBOSE(1,("Unknown Addres Value Calcultation for %s on @I",PpcRegisterName(reg),*ins));
    return AddressNullForIns (T_INS(*ins));
  }
 
  VERBOSE(1,("Get Partial definition... @I",definition));
 
  /* Keep looking for the case that the definition is a mr regt, rega, regb */
  if((PPC_INS_OPCODE(definition)==PPC_OR)&&(PPC_INS_REGA(definition)==PPC_INS_REGB(definition)))
  {
    lookingfor = NullRegs; 
    RegsetSetAddReg(lookingfor, PPC_INS_REGA(definition));
    VERBOSE(1,("Looking the definition of %s",PpcRegisterName(PPC_INS_REGA(definition))));
    definition = (t_ppc_ins *) PpcFindInstructionThatDefinesRegister(definition,lookingfor);
    if(!definition)
    {
      VERBOSE(1,("Unknown Addres Value Calcultation for %s on @I",PpcRegisterName(reg),*ins));
      return AddressNullForIns (T_INS(*ins));
    }
    VERBOSE(1,("Was a mr, so new partial definition... @I",definition));
  }
  
  low = AddressNullForIns (T_INS(*ins));
  switch (PPC_INS_OPCODE(definition))
  {
    
    case PPC_ADDI:
    case PPC_ORI:
      low = PPC_INS_IMMEDIATE(definition); 
      lookingfor = NullRegs; 
      RegsetSetAddReg(lookingfor, PPC_INS_REGA(definition));  
      VERBOSE(1,("Looking the definition of %s",PpcRegisterName(PPC_INS_REGA(definition))));
      definition = PpcFindInstructionThatDefinesRegister(definition,lookingfor);
    case PPC_ADDIS:
    case PPC_ORIS:
      if(!definition)
      {
        VERBOSE(1,("Unknown definition"));
        return AddressNullForIns(T_INS(*ins));
      }
      high = PPC_INS_IMMEDIATE(definition); 
      if (PPC_INS_OPCODE(definition)==PPC_ADDIS)
        address = AddressAdd (high, low);
      else
        address = AddressOr (high, low);
      /* it's only over if this instruction is a lis, otherwise the computed */
      /* value also depends on REGA                                          */
      if ((PPC_INS_OPCODE(definition)==PPC_ADDIS)&&
          (PPC_INS_REGA(definition)==PPC_REG_R0))
      {
        *found = TRUE;
        *ins = definition;
        return address;
      }
      else
      {
        /* recognize sequences like
          100004a0:   3c a0 00 00     lis     r5,0
          100004a4:   60 a5 00 00     ori     r5,r5,0
          100004a8:   78 a5 07 c6     rldicr  r5,r5,32,31
          100004ac:   64 a5 10 20     oris    r5,r5,4128
          100004b0:   60 a5 dc 80     ori     r5,r5,56448
        */
        /* keep track of the addis/oris for later (0x100004ac in above example) */
        prevdef = definition;
        /* next up is an rldicr? */
        lookingfor = NullRegs; 
        RegsetSetAddReg(lookingfor, PPC_INS_REGA(definition));  
        definition = PpcFindInstructionThatDefinesRegister(definition,lookingfor);
        if (definition &&
            (PPC_INS_OPCODE(definition)==PPC_RLDICR) &&
            /* destination must be the source of the oris/addis */
            (PPC_INS_REGT(definition)==PPC_INS_REGA(prevdef)) &&
            /* source can be anything, but must be a "sldi" */
            (AddressExtractUint64(PPC_INS_IMMEDIATE(definition))==32) &&
            (PPC_INS_MASK(definition)==31))
       {
          prevdef = PPC_INS_IPREV(definition);
          high = PpcComputeRelocatedAddressValue(&prevdef,PPC_INS_REGA(definition),&foundprev);
          /* low could be null/zero on a 64 bit system */
          if (foundprev)
          {
            address = AddressOr(AddressAnd(address,AddressBitMask(32,AddressType(address))),
                                AddressShl(high,AddressNewTyped(32,AddressType(high))));
            *found = TRUE;
            *ins = prevdef;
            return address;
          }
        }
      }
      return AddressNullForIns (T_INS(*ins));
    case PPC_RLDIMI:
      /* recognize sequences like
          1001aa70:   3c a0 00 00     lis     r5,0
          1001aa74:   60 a5 00 00     ori     r5,r5,0
          1001aa78:   3c c0 10 20     lis     r6,4128
          1001aa7c:   60 c6 dd 00     ori     r6,r6,56576
          1001aa80:   78 a6 00 0e     rldimi  r6,r5,32,0
      */
      if ((PPC_INS_REGT(definition)==reg) &&
          (PPC_INS_REGA(definition)!=reg) &&
          (AddressExtractUint64(PPC_INS_IMMEDIATE(definition))==32) &&
          (PPC_INS_MASK(definition)==0) &&
          PPC_INS_IPREV(definition))
      {
        prevdef=PPC_INS_IPREV(definition);
        high = PpcComputeRelocatedAddressValue(&prevdef,PPC_INS_REGA(definition),&foundprev);
        prevdef=PPC_INS_IPREV(definition);
        low = PpcComputeRelocatedAddressValue(&prevdef,reg,found);
        /* low and/or high can be NULL on a 64 bit system */
        if (*found && foundprev)
        {
          address = AddressOr(AddressAnd(low,AddressBitMask(32,AddressType(low))),
                            AddressShl(high,AddressNewTyped(32,AddressType(high))));
          /* we have to return one of the instructions which may use a relocation */
          *ins = prevdef;
          return address;
        }
        else
          *found = FALSE;
      }
      return AddressNullForIns(T_INS(*ins));
    case PPC_LWAX:
      /* recognize sequences like
        ld     r11,.LC0@toc(r2)
        rldic  r0,r3,2,30
        lwax   r9,r11,r0
      */
      prevdef = PPC_INS_IPREV(definition);
      if (!prevdef)
        return AddressNullForIns(T_INS(*ins));
      address = PpcComputeRelocatedAddressValue(&prevdef,PPC_INS_REGA(definition),found);
      if (found)
      {
        *ins = prevdef;
        return address;
      }
      prevdef = PPC_INS_IPREV(definition);
      address = PpcComputeRelocatedAddressValue(&prevdef,PPC_INS_REGB(definition),found);
      if (found)
      {
        *ins = prevdef;
        return address;
      }
      return AddressNullForIns(T_INS(*ins));
      break;
    case PPC_LWZ:
    case PPC_LD:
      {
        t_reloc_ref *rr;
        t_reloc *rel;
        t_address reladdr;
        t_section *got, *toc;
        t_object *obj;

        /* consider loads from the .toc as "constant" */
        if (PPC_INS_REGA(definition)==PPC_REG_R2)
        {
          rr = PPC_INS_REFERS_TO(PPC_INS_COPY(definition));
          if (!rr)
            return AddressNullForIns(T_INS(*ins));
          rel = RELOC_REF_RELOC(rr);

          if (RELOC_N_TO_RELOCATABLES(rel) > 1)
            return AddressNullForIns(T_INS(*ins));
          if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) != RT_SUBSECTION)
            return AddressNullForIns(T_INS(*ins));
          toc = T_SECTION(RELOC_TO_RELOCATABLE(rel)[0]);
          if (strncmp(SECTION_NAME(toc),".toc",4))
            return AddressNullForIns(T_INS(*ins));

          /* get the object representing the final binary (the intermediate objects
           * do not have a .got)
           */
          obj = SECTION_OBJECT(PPC_INS_SECTION(definition));
          reladdr = StackExec(RELOC_CODE(rel),rel,NULL,NULL, FALSE,0,obj);
          /* the address got-relative, so 0x8000 past the real address
           */
          reladdr = AddressAddInt32(reladdr,0x8000);
          /* get the .got section
           */
          got = SectionGetFromObjectByName (obj, ".got");
          /* and now extract the loaded address from it
           */
          if (PPC_INS_OPCODE(definition) == PPC_LD)
          {
            address = AddressNewTyped(((t_uint64*)((t_uint64)SECTION_DATA(got) +
                                               AddressExtractUint64(reladdr)))[0],AddressType(reladdr));
          }
          else
          {
            address = AddressNewTyped(((t_uint32*)((t_uint64)SECTION_DATA(got) +
                                               AddressExtractUint64(reladdr)))[0],AddressType(reladdr));
          }
          *found = TRUE;
          *ins = definition;
          return address;
        }
        else
        {
          return AddressNullForIns(T_INS(*ins));
        }
      }
    default: 
      VERBOSE(1,("Unknown Addres Value Calcultation for %s on @I",PpcRegisterName(reg),definition));
      VERBOSE(1,("Instruction: @I",*ins));
      break;
  }
  return AddressNullForIns (T_INS(*ins));
}
/*}}}*/

/* PpcLoadValueFromAddress {{{*/
/*
t_uint32 
PpcLoadValueFromAddress(t_object * obj, t_address_generic value) 
{
  t_uint32 ret;
  VERBOSE(0,("Loading from address : @G",value));
  ret = ObjectGetData32(obj,".rodata", value);
  VERBOSE(0,("Value loaded: %x",ret));
  return ret;
}
*/
/*}}}*/

/* PpcEdgeKillAndUpdateBblRecursive{{{*/
t_bool
PpcEdgeKillAndUpdateBblRecursive(t_cfg_edge * edge)
{
  t_bbl *head;
  t_cfg_edge *edge2;
  t_uint32 count = 0;

  if (!edge)
  {
    return FALSE;
  }

  head = CFG_EDGE_HEAD(edge);

  if(CfgEdgeKillAndUpdateBbl(edge))
  {
    BBL_FOREACH_SUCC_EDGE_SAFE(head,edge,edge2)
    {
      count++;
    }

    if(count==0)
    {
      CfgEdgeKillAndUpdateBbl(edge);
    }
    
  }
  return TRUE;
}
/*}}}*/

/* PpcFunIsGlobal {{{*/
t_bool 
PpcFunIsGlobal(t_function * fun)
{
  if (FUNCTION_FLAGS(fun) & FF_IS_EXPORTED)
  {
    if(FunctionBehaves(fun))
    {
      return TRUE;
    }
    else 
    {
      return FALSE;
    }
  }
  return FALSE;
}
/*}}}*/

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
