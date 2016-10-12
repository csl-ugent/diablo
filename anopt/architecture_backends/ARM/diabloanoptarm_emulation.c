/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
#include <strings.h>

#define CARRY_FROM(_a,_b,_r) ((_a >> 31) ? ((_b >> 31) | ((~_r) >> 31)) : ((_b >> 31) * ((~_r) >> 31)))
#define BORROWED_FROM(_a,_b,_r) ((_a >> 31) ? ((_b >> 31) & (_r >> 31)) : ((_b >> 31) | (_r >> 31)))

/* Macros for flexible operands */

#define ARM_SIGN_BIT(_x) (((_x) >> 31) & 0x1)



/*!
 * This procedure Extracts the Shift operand (i.e. the value that will be used
 * after all shifts are applied) from 1. a instruction and 2. the state in
 * which the instruction is executed and stores this value in shifted_value. It
 * also modifies the condition bits in case of a S-etting instruction 
 *
 * \todo Complete documentation
 *
 * \param ins
 * \param state
 * \param shifted_value
 *
 * \return t_lattice_level 
*/
/* Utility function to emulate the shifter part of an instruction {{{ */
t_lattice_level ArmInsExtractShift(t_arm_ins * ins, t_procstate * state, t_register_content * shifted_value)
{
  t_uint32 shiftimmediate;
  t_register_content tmp_value_to_shift;
  t_register_content tmp_shiftreg_value;
  t_bool carry_set=FALSE;
  t_bool tmp_flag;
  t_uint32 value_to_shift;
  t_uint32 shiftreg_value;

  /* Get the value base register that holds the value to shift, if that fails,
   * we make worst case assumptions for the shifter {{{*/

  if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGC(ins),&tmp_value_to_shift)))
    {
      if (ArmInsUpdatesCond(ins))
	{
	  ProcStateSetCondBot(state,ARM_REG_C_CONDITION);
	}
      return CP_BOT;
    }

  /*}}}*/
  
  value_to_shift = G_T_UINT32(tmp_value_to_shift.i);
  shiftimmediate = ARM_INS_SHIFTLENGTH(ins);
  
  switch (ARM_INS_SHIFTTYPE(ins)) /* perform the right shift and set carry if necessary */
    {
      /* Shifts over an immediate value {{{ */
    case ARM_SHIFT_TYPE_LSL_IMM:
      if (shiftimmediate > 0) 
	if (Uint32GetBit(value_to_shift, (32-shiftimmediate))) 
	  carry_set=TRUE;
      shifted_value->i=AddressNew32(value_to_shift << shiftimmediate);
      break;
    case ARM_SHIFT_TYPE_LSR_IMM: 
      if (Uint32GetBit(value_to_shift, (shiftimmediate-1))) 
	carry_set=TRUE;
      shifted_value->i=AddressNew32((value_to_shift >> shiftimmediate));
      break;
    case ARM_SHIFT_TYPE_ASR_IMM: 
      if (Uint32GetBit(value_to_shift, (shiftimmediate-1))) 
	carry_set=TRUE;
      if ((t_int32) value_to_shift < 0)
	shifted_value->i=AddressNew32((value_to_shift >> shiftimmediate) | ~(0xFFFFFFFF >> shiftimmediate));
      else 
	shifted_value->i=AddressNew32(value_to_shift >> shiftimmediate);
      break;
    case ARM_SHIFT_TYPE_ROR_IMM:
      if (shiftimmediate == 0)
	{
	  shifted_value->i=AddressNew32(value_to_shift);
	  break;
	}
      if (Uint32GetBit(value_to_shift, (shiftimmediate-1))) 
	carry_set=TRUE;
      shifted_value->i=AddressNew32((value_to_shift >> shiftimmediate) | (value_to_shift << (32 - shiftimmediate)));
      break;
      /*}}}*/

      /* Shifts over a register, can be necessary to make worst case assumptions, if the amount to shift is unknown {{{*/
    case ARM_SHIFT_TYPE_LSL_REG:
      if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGS(ins),&tmp_shiftreg_value)))
	{
	  if (ArmInsUpdatesCond(ins))
	    {
	      ProcStateSetCondBot(state,ARM_REG_C_CONDITION);
	    }
	  return CP_BOT;
	}
      shiftreg_value = G_T_UINT32(tmp_shiftreg_value.i);
      
      if (ARM_SHIFT_AMOUNT(shiftreg_value) == 0) /* no carry can be shifted out here */
	shifted_value->i=AddressNew32(value_to_shift);
      else if (ARM_SHIFT_AMOUNT(shiftreg_value) > 32) /* no carry can be shifted out here */
	shifted_value->i=AddressNew32(0);
      else
	{
	  if (Uint32GetBit(value_to_shift, (32 - ARM_SHIFT_AMOUNT(shiftreg_value)))) 
	    carry_set=TRUE;
	  shifted_value->i=AddressNew32(value_to_shift << ARM_SHIFT_AMOUNT(shiftreg_value));
	}
      break;
    case ARM_SHIFT_TYPE_LSR_REG: /* no carry can be shifted out here */
      if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGS(ins),&tmp_shiftreg_value)))
	{
	  if (ArmInsUpdatesCond(ins))
	    {
	      ProcStateSetCondBot(state,ARM_REG_C_CONDITION);
	    }
	  return CP_BOT;
	}
      shiftreg_value = G_T_UINT32(tmp_shiftreg_value.i);
      shifted_value->i=AddressNew32((value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value)));
      break;
    case ARM_SHIFT_TYPE_ASR_REG: /* no carry can be shifted out here */
      if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGS(ins),&tmp_shiftreg_value)))
	{
	  if (ArmInsUpdatesCond(ins))
	    {
	      ProcStateSetCondBot(state,ARM_REG_C_CONDITION);
	    }
	  return CP_BOT;
	}
      shiftreg_value = G_T_UINT32(tmp_shiftreg_value.i);

      if ((t_int32) value_to_shift < 0)
	shifted_value->i=AddressNew32((value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value)) | ~(0xFFFFFFFF >> ARM_SHIFT_AMOUNT(shiftreg_value)));
      else shifted_value->i=AddressNew32(value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value));
      break;
    case ARM_SHIFT_TYPE_ROR_REG:
      if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGS(ins),&tmp_shiftreg_value)))
	{
	  if (ArmInsUpdatesCond(ins))
	    {
	      ProcStateSetCondBot(state,ARM_REG_C_CONDITION);
	    }
	  return CP_BOT;
	}
      shiftreg_value = G_T_UINT32(tmp_shiftreg_value.i);

      if (ARM_ROTATE_AMOUNT(shiftreg_value) == 0)
	{
	  shifted_value->i=AddressNew32(value_to_shift);
	  break;
	}
      if (Uint32GetBit(value_to_shift, (ARM_ROTATE_AMOUNT(shiftreg_value)-1))) 
	carry_set=TRUE;
      shifted_value->i=AddressNew32((value_to_shift >> ARM_ROTATE_AMOUNT(shiftreg_value)) | (value_to_shift << (32 - ARM_ROTATE_AMOUNT(shiftreg_value)))  /* CHECK THIS */);
      break;
      /*}}}*/

    case ARM_SHIFT_TYPE_RRX:
    {
      t_lattice_level carry_state;
      
      /* can't let the code at the end do this, because we may
       * have to return CP_BOT
       */
      if (ArmInsUpdatesCond(ins))
      {
        ProcStateSetCond(state,ARM_REG_C_CONDITION,value_to_shift & 0x1);
      }
      carry_state=ProcStateGetCond(state,ARM_REG_C_CONDITION,&tmp_flag);
      if (CP_BOT==carry_state)
      {
        return CP_BOT;
      }
      shifted_value->i=AddressNew32(((t_uint32)tmp_flag << 31) | (value_to_shift >> 1));
      return CP_VALUE;
      break;
    }
    default: 
      FATAL(("Unknown shifttype while emulating!!"));
      break;
    }

  if (ArmInsUpdatesCond(ins))
  {
    ProcStateSetCond(state,ARM_REG_C_CONDITION,carry_set);
  }

  return CP_VALUE;
}
/*}}}*/



#define ArmInsRegisterBMatters(ins) (ARM_INS_OPCODE(ins)!=ARM_MVN && ARM_INS_OPCODE(ins)!=ARM_MOV)
#define ArmInsNeedsCarry(ins)       (ARM_INS_OPCODE(ins)==ARM_RSC || ARM_INS_OPCODE(ins)==ARM_SBC || ARM_INS_OPCODE(ins)==ARM_ADC)
#define ArmInsIsMul(ins)            (ARM_INS_OPCODE(ins)==ARM_MUL || ARM_INS_OPCODE(ins)==ARM_MLA  || ARM_INS_OPCODE(ins)==ARM_UMULL)
#define ArmInsIsLogical(ins)        (ARM_INS_OPCODE(ins)==ARM_AND|| ARM_INS_OPCODE(ins)==ARM_TST|| ARM_INS_OPCODE(ins)==ARM_BIC|| ARM_INS_OPCODE(ins)==ARM_EOR|| ARM_INS_OPCODE(ins)==ARM_ORR|| ARM_INS_OPCODE(ins)==ARM_TEQ|| ARM_INS_OPCODE(ins)==ARM_MOV|| ARM_INS_OPCODE(ins)==ARM_MVN)

#define ArmOpcodeRegisterBMatters(opcode) (opcode!=ARM_MVN && opcode!=ARM_MOV)
#define ArmOpcodeNeedsCarry(opcode)       (opcode==ARM_RSC || opcode==ARM_SBC || opcode==ARM_ADC)
#define ArmOpcodeIsMul(opcode)            (opcode==ARM_MUL || opcode==ARM_MLA  || opcode==ARM_UMULL)
#define ArmOpcodeIsLogical(opcode) (opcode==ARM_AND|| opcode==ARM_TST|| opcode==ARM_BIC|| opcode==ARM_EOR|| opcode==ARM_ORR|| opcode==ARM_TEQ|| opcode==ARM_MOV|| opcode==ARM_MVN)

static t_bool ArmInsEmulDetermineTag(t_arm_ins * ins, t_procstate * state)
{
  t_reg reg;
  t_reg rega = ARM_INS_REGA(ins);
  t_regset used_regs = ARM_INS_REGS_USE(ins);
  t_arm_opcode opcode = ARM_INS_OPCODE(ins);
  t_reloc * dummy;
  if (ArmInsIsNOOP(ins)) return FALSE;

  if (!RegsetIn(used_regs,rega))
  {
    ProcStateSetTagTop(state,rega);
  }

  REGSET_FOREACH_REG(used_regs,reg)
  {
    if (reg > ARM_REG_R15) break; /* tags are only useful in the
                                     general-purpose registers */
    ProcStateJoinTags(state,rega,reg);
  }

  if ((opcode==ARM_RSB || opcode==ARM_SUB) && !ArmInsHasImmediate(ins) && ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE)
  {
    /* Consider the following code fragment: 
       ADR r9, begin_of_init_array
       ADR r8, end_of_init_array
       SUB r8, r9, r8

       When the produced addresses point to the same block, the above tag joins will have
       resulted in a value tag (i.e., reloc) for the destination of the sub, modeling the fact
       that the sub computes an address at some offset of that reloc. 
       But that is wrong: a true constant was computed, not an address at some offset of
       some relocatable. So the tag needs to be removed. 

       So we need to set the tag to top. By doing so (below), the sub will eventually be replaced
       by a true constant producer. 
    */

    t_reloc * relocB;
    t_reloc * relocC;
    t_lattice_level valueB = ProcStateGetTag(state,ARM_INS_REGB(ins),&relocB);
    t_lattice_level valueC = ProcStateGetTag(state,ARM_INS_REGC(ins),&relocC);
    if (valueB == CP_VALUE && valueC == CP_VALUE && TwoRelocsInSameBlock(relocB,relocC))
      ProcStateSetTagTop(state,rega);
  }

  return ISVALUE(ProcStateGetTag(state,rega,&dummy));
}

/* Emulate specific instructions {{{ */

static t_bool ArmGetRegisterB(t_arm_ins * ins, t_procstate * state, t_register_content * tmp_src1)
{
  /* Get register B and return FALSE if that fails */
  if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGB(ins),tmp_src1)))
  {
    return FALSE;
  }
 
  return TRUE;
}

static t_bool ArmGetRegisterC(t_arm_ins * ins, t_procstate * state, t_register_content * tmp_src2)
{
  /* If the instruction has an immediate, we need this value */
  if (ArmInsHasImmediate(ins))
    tmp_src2->i=AddressNew32(ARM_INS_IMMEDIATE(ins));
  /* If it has a shifted value, we need that value */
  else if (ArmInsHasShiftedFlexible(ins))
  {
    /* Get the shifted value is not constant, make worst case assumptions if
     * that fails */
    if (!ISVALUE(ArmInsExtractShift(ins,state,tmp_src2)))
    {
      return FALSE;
    }
  }
  /* If it is none of the above, simply get the value from the register and
   * make worst case assumptions if that fails */
  else if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGC(ins),tmp_src2)))
  {
    return FALSE;
  }
  return TRUE;
}

static t_bool ArmGetRegisterS(t_arm_ins * ins, t_procstate * state, t_register_content * tmp_src3)
{
  /* Get register S and return FALSE if that fails */
  if (!ISVALUE(ProcStateGetReg(state,ARM_INS_REGS(ins),tmp_src3)))
  {
    return FALSE;
  }
 
  return TRUE;
}


static t_bool ArmGetCarry(t_arm_ins * ins, t_procstate * state, t_bool * carry_flag)
{
  if (!ISVALUE(ProcStateGetCond(state,ARM_REG_C_CONDITION,carry_flag)))
  {
	  return FALSE;
  }
  return TRUE;
}

/*#define LOAD_AGGRESSIVE */
/* Emulate numerical an logical instructions {{{ */
static void ArmInsEmulNumericalAndLogical(t_arm_ins * ins, t_procstate * state)
{
  t_register_content tmp_src1, tmp_src2, tmp_src3, dest;
  t_uint32 src1,src2;
  t_regset mask = NullRegs;
  t_int64 tmp = 0L; 
  t_int64 carry_set=FALSE; /* Just to keep the compiler happy, value is never used! */
  t_bool carry_flag;
  t_bool can_emulate=TRUE;
  t_arm_opcode opcode = ARM_INS_OPCODE(ins);
  t_bool result_is_tagged = FALSE;

  /* Emulate the tags {{{*/
  if (ARM_INS_REGA(ins)!=ARM_REG_NONE)
  {
#if 0
    t_reloc * rel1;
    t_reloc * rel2;
    t_reloc * tmp_rel;

    if ((opcode==ARM_ADD)&&(!ArmInsHasImmediate(ins))&&(!ArmInsHasShiftedFlexible(ins))&&(ISVALUE( ProcStateGetTag(state,ARM_INS_REGB(ins),&rel1)) &&(ISVALUE( ProcStateGetTag(state,ARM_INS_REGC(ins),&rel2)))  ))
    {
      if (((rel1->write_type==SREL32)|| (rel2->write_type==SREL32))
      && ((rel1->write_type==ABS32)|| (rel2->write_type==ABS32) ||  (rel1->write_type==ADDRESS_PROC) ||  (rel2->write_type==ADDRESS_PROC)) )
      {
	if (rel2->write_type==SREL32)
	{
	  tmp_rel=rel2;
	  rel2=rel1;
	  rel1=tmp_rel;
	}

	/* rel1 holds an srel value */

	if (RELOC_TO_RELOCATABLE(rel2)->type==RT_BBL)
	{
	  if (rel1->from_type==RELOC_FROM_INSTRUCTION)
	  {
	    if (ARM_INS_BBL(T_INS(rel1->from))==T_BBL(rel2RELOC_TO_RELOCATABLE()))
	    {
	      /* Cool, made a srel into an abs value, the only remaining value is rel1 */
	      ProcStateSetTag(state,ARM_INS_REGA(ins),rel1);
	    }
	    else
	      FATAL(("Implement 3"));
	  }
	  else
	    FATAL(("Implement 1"));
	}
	else
	  FATAL(("Implement 2"));
	
      }
      else
      FATAL(("Adding two relocations, @R and @R\n",rel1,rel2));
    }
    else
#endif
      result_is_tagged = ArmInsEmulDetermineTag(ins,state);
  }
  /*}}}*/
 
  {
    t_reg reg;
    t_regset regs_used = ARM_INS_REGS_USE(ins);
    REGSET_FOREACH_REG(regs_used,reg)
    {
      if (ISTOP(ProcStateGetReg(state,reg,&tmp_src1)))
      {
#ifdef LOAD_AGGRESSIVE
	VERBOSE(0,("== Top encountered in reg %d in @I\n",reg,ins));
	ProcStateSetRegsetTop(state,ARM_INS_REGS_DEF(ins));
	if (ArmInsUpdatesCond(ins))
	{
	  RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
	  RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);

	  if ((!ArmOpcodeIsLogical(opcode)&& (!ArmOpcodeIsMul(opcode)))
	  {
	    RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
	    RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
	    RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
	  }
	  ProcStateSetCondsetTop(state,mask);
	}
#else
	FATAL(("Top encountered"));
#endif
	return;
      }
    }
  }

  if (ArmOpcodeRegisterBMatters(opcode)) 
    can_emulate&=ArmGetRegisterB(ins,state,&tmp_src1);
  can_emulate&=ArmGetRegisterC(ins,state,&tmp_src2);
  if (ArmOpcodeNeedsCarry(opcode))  // shouldn't this come first, because ArmGetRegisterC might already update the carry flag ???
    can_emulate&=ArmGetCarry(ins,state,&carry_flag);
  if (opcode==ARM_MLA)
    can_emulate&=ArmGetRegisterS(ins,state,&tmp_src3);


  /* logical values depend on precise alignment of addresses, which we should never
     consider known */
  if (ArmInsIsLogical(ins) && result_is_tagged)
    can_emulate = FALSE;

  if (!can_emulate)
  {
    ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
    if (ArmInsUpdatesCond(ins))
    {
      RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
      RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);

      if ((!ArmInsIsLogical(ins))&& (!ArmInsIsMul(ins)))
      {
	RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
      }
      ProcStateSetCondsetBot(state,mask);
    }
    return;
  }
  
  src1 = G_T_UINT32(tmp_src1.i);
  src2 = G_T_UINT32(tmp_src2.i);

  bzero(&dest,sizeof(t_register_content));

  /* Perform the operation and set the defined register{{{ */
  switch(opcode)
  {
    /* Logical instructions */
    case ARM_MVN:
       dest.i=AddressNew32(~src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_MOV:
       dest.i=AddressNew32(src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_TST:
       dest.i=AddressNew32(src1 & src2);
      break;
    case ARM_AND:
       dest.i=AddressNew32(src1 & src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_BIC:
       dest.i=AddressNew32(src1 & ~src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_EOR:
       dest.i=AddressNew32(src1 ^ src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_ORR:
       dest.i=AddressNew32(src1 | src2);
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_TEQ:
       dest.i=AddressNew32(src1 ^ src2);
      break;


    /* Arithmitic */
    case ARM_CMN:
      tmp =  (t_int32) src1;  
      tmp +=  (t_int32) src2;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      carry_set = CARRY_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_CMP:
      tmp =  (t_int32) src1;  
      tmp -=  (t_int32) src2;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      carry_set = !BORROWED_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_RSB:
      tmp =  (t_int32) src2;  
      tmp -=  (t_int32) src1;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = !BORROWED_FROM(src2, src1, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_SUB:
      tmp =  (t_int32) src1;  
      tmp -=  (t_int32) src2;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = !BORROWED_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_RSC:
      tmp =  (t_int32) src2;  
      tmp -=  (t_int32) src1;
      if (!carry_flag)
	tmp--;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = !BORROWED_FROM(src2, src1, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_SBC:
      tmp =  (t_int32) src1;  
      tmp -=  (t_int32) src2;
      if (!carry_flag)
	tmp--;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = !BORROWED_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_ADD:
      tmp =  (t_int32) src1;  
      tmp +=  (t_int32) src2;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = CARRY_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;
    case ARM_ADC:
      tmp =  (t_int32) src1;  
      tmp +=  (t_int32) src2;
      if (carry_flag)
	tmp++;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      carry_set = CARRY_FROM(src1, src2, (t_uint32) ((t_uint64) tmp));
      break;

      /* MUL */
    case ARM_MUL:
      tmp  = (t_int32) src1;
      tmp *= (t_int32) src2;
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    case ARM_MLA:
      tmp  = (t_int32) src1;
      tmp *= (t_int32) src2;
      tmp += (t_int32) G_T_UINT32(tmp_src3.i);
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;

    case ARM_UMULL:
      tmp  = (t_int32) src1;
      tmp *= (t_int32) src2;
      /* Lower half goes into RegS */
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGS(ins),dest);
      tmp >>= 32;
      /* Upper half goes into RegA */
       dest.i=AddressNew32((t_uint32) ((t_uint64) tmp));
      ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
      break;
    default:
      break;
    }
  /*}}}*/
  
  /* If necessary: Set Conditions {{{*/
  if (ArmInsUpdatesCond(ins))
    {
      if (opcode==ARM_UMULL) FATAL(("Implement"));

      if (G_T_UINT32(dest.i)==0)
	ProcStateSetCond(state,ARM_REG_Z_CONDITION,TRUE);
      else
	ProcStateSetCond(state,ARM_REG_Z_CONDITION,FALSE);
      
      if (ARM_SIGN_BIT(G_T_UINT32(dest.i))==1)
	ProcStateSetCond(state,ARM_REG_N_CONDITION,TRUE);
      else
	ProcStateSetCond(state,ARM_REG_N_CONDITION,FALSE);

      if ((!ArmOpcodeIsLogical(opcode)) && (!ArmOpcodeIsMul(opcode)))
      {
	if (carry_set)
	  ProcStateSetCond(state,ARM_REG_C_CONDITION,TRUE);
	else
	  ProcStateSetCond(state,ARM_REG_C_CONDITION,FALSE);
      
	/* might be undefined however */
	if (((tmp >> 32) & 0x1) != ARM_SIGN_BIT(tmp))
	  ProcStateSetCond(state,ARM_REG_V_CONDITION,TRUE);
	else
	  ProcStateSetCond(state,ARM_REG_V_CONDITION,FALSE);
      }
    }
  /* }}} */
}

/* }}} */

/* Emulate constant producers {{{ */
static void ArmInsEmulConstProd(t_arm_ins * ins, t_procstate * state)
{
  t_register_content dest;
  bzero(&dest,sizeof(t_register_content));
  dest.i=AddressNew32(ARM_INS_IMMEDIATE(ins));
  ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
  ProcStateSetTagTop(state,ARM_INS_REGA(ins));
}
/* }}} */

/* Emulate address producers {{{*/

static void ArmInsEmulAddrProd(t_arm_ins * ins, t_procstate * state)
{
  t_register_content dest;
  bzero(&dest,sizeof(t_register_content));
  dest.i=AddressNew32(ARM_INS_IMMEDIATE(ins));
  ProcStateSetTag(state,ARM_INS_REGA(ins),RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)));
  ProcStateSetReg(state,ARM_INS_REGA(ins),dest);
}
/* }}}*/


/* Emulate loads {{{*/
static t_uint32 adjust_loaded_value (t_arm_ins *ins, t_uint32 val)
{
  switch (ARM_INS_OPCODE (ins))
  {
    case ARM_LDRB:
      val &= 0xff;
      break;
    case ARM_LDRSB:
      val &= 0xff;
      if (val & 0x80) val |= 0xffffff00;
      break;
    case ARM_LDRH:
      val &= 0xffff;
      break;
    case ARM_LDRSH:
      val &= 0xffff;
      if (val & 0x8000) val |= 0xffff0000;
      break;
    default:
      ; /* do nothing */
  }
  return val;
}

static void
load_value_from_insdata_in_reg(t_arm_ins * ins, t_arm_ins * data, t_reg reg, t_uint32 byte, t_procstate * state)
{
  t_uint32 val;
  t_register_content dest;
  
  val = ARM_INS_IMMEDIATE (data) >> (8*byte);
  val = adjust_loaded_value (ins, val);
  dest.i=AddressNew32(val);
  if (ARM_INS_REFERS_TO (data))
  {
    ASSERT ((ARM_INS_OPCODE (ins) == ARM_LDR) || (ARM_INS_OPCODE (ins) == ARM_LDRD), ("partial load of relocated value: @I",ins));
    ProcStateSetTag(state,reg,RELOC_REF_RELOC(ARM_INS_REFERS_TO(data)));
  }
  else
    ProcStateSetTagTop(state,reg);
  ProcStateSetReg(state,reg,dest);
}

static void SetLoadedRegTags(t_procstate * state, t_reg reg, t_reloc *rel, t_bool multiple_rels)
{
  ProcStateSetRegBot(state,reg);
  if (multiple_rels)
    ProcStateSetTagBot (state, reg);
  else if (rel)
    ProcStateSetTag (state, reg, rel);
  else
    ProcStateSetTagTop (state, reg);
}

static void ArmInsEmulLoad(t_arm_ins * ins, t_procstate * state)
{
  t_bool can_emulate=TRUE, out_of_block = FALSE, alignment_violation = FALSE, be_conservative = FALSE;
  t_reloc * base_reloc, * offset_reloc;
  t_register_content tmp_base_address, tmp_offset, dest;
  t_lattice_level base_reloc_unknown, offset_reloc_unknown;
  int i;
  t_uint32 num_regs = 0;
  t_address loadaddr = AddressNew32(0), loadoffset;
  t_relocatable *from;
  t_bool preindex, up;

  preindex = (ARM_INS_FLAGS (ins) & FL_PREINDEX) != 0;
  up = (ARM_INS_FLAGS (ins) & FL_DIRUP) != 0;

  if (ARM_INS_OPCODE(ins) == ARM_LDM) 
    for (i=0; i<16; i++) 
      if (ARM_INS_IMMEDIATE(ins) & (1 << i)) num_regs++;

  /* get the base address and reloc from the state */
  base_reloc_unknown = ProcStateGetTag(state,ARM_INS_REGB(ins),&base_reloc);
  can_emulate &= ISVALUE (base_reloc_unknown);
  can_emulate &= ArmGetRegisterB(ins,state,&tmp_base_address);
  /* {{{ get the offset from the flexible operand */
  if (ARM_INS_OPCODE(ins)!=ARM_LDM) 
  {
    t_bool known = ArmGetRegisterC(ins,state,&tmp_offset);
    if (ARM_INS_REGC (ins) != ARM_REG_NONE)
    {
      offset_reloc_unknown = ProcStateGetTag(state,ARM_INS_REGC(ins),&offset_reloc);
    }
    else
    {
      offset_reloc_unknown = CP_TOP;
      offset_reloc = NULL;
    }
    /* do not emulate if the offset is relocated or unknown and the instruction is preindexed */
    if (preindex)
    {
      if (!known) can_emulate = FALSE;
      if (!ISTOP (offset_reloc_unknown))
      {
	can_emulate = FALSE;
	be_conservative = TRUE;
      }
    }
  } /* }}} */
  /* {{{ check alignment of the load address */
  if (can_emulate)
  {
    loadaddr = tmp_base_address.i;
    if (ARM_INS_OPCODE (ins) != ARM_LDM && preindex)
    {
      if (up)
	loadaddr = AddressAdd (loadaddr, tmp_offset.i);
      else 
	loadaddr = AddressSub (loadaddr, tmp_offset.i);
    }
    switch (ARM_INS_OPCODE (ins))
    {
      case ARM_LDR:
      case ARM_LDM:
      case ARM_LDRD:
	/* need 4-byte aligned address */
	if (G_T_UINT32 (loadaddr) & 0x3)
	  alignment_violation = TRUE;
	break;
      case ARM_LDRH:
      case ARM_LDRSH:
	/* need 2-byte aligned address */
	if (G_T_UINT32 (loadaddr) & 0x1)
	  alignment_violation = TRUE;
        break;
      default:
	/* byte loads can be at any address */
	break;
    }
    if (alignment_violation)
      can_emulate = FALSE;
  } /* }}} */

  if (ISVALUE (base_reloc_unknown))
  {
     if (RELOC_N_TO_RELOCATABLES(base_reloc)==1)
     {
	     from = RELOC_TO_RELOCATABLE (base_reloc)[0];
     }
     else
     {
	     from = NULL;
	     can_emulate = FALSE;
     }
     
  }
  else
    from = NULL;
  /* {{{ check for out-of-block load */
  if (can_emulate)
  {
    loadoffset = AddressSub (loadaddr, RELOCATABLE_CADDRESS (from));
    if (ARM_INS_OPCODE (ins) == ARM_LDM)
    {
      /* make sure _all_ loads fall in the block:
       * 	-upper bounds should be < block size
       * 	-lower bounds should be >= 0.
       * 	 because of the unsignedness of addresses, we can just as well check
       * 	 lower bound <= block size (assuming no blocks of >2Gb) */
      if (up && preindex)
      {
	/* reads [base+4 ... base+4+4*num_regs[ */
	if (AddressIsGt (AddressAddUint32 (loadoffset, 4), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
	if (AddressIsGt (AddressAddUint32 (loadoffset, 4+4*num_regs), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
      }
      else if (up && !preindex)
      {
	/* reads [base ... base+4*num_regs[ */
	if (AddressIsGt (loadoffset, RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
	if (AddressIsGt (AddressAddUint32 (loadoffset, 4*num_regs), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
      }
      else if (!up && preindex)
      {
	/* reads [base-4*num_regs ... base[ */
	if (AddressIsGt (AddressSubUint32 (loadoffset, 4*num_regs), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
	if (AddressIsGt (loadoffset, RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
      }
      else if (!up && !preindex)
      {
	/* reads [base-4*num_regs+4 ... base+4[ */
	if (AddressIsGt (AddressSubUint32 (loadoffset, 4*num_regs-4), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
	if (AddressIsGt (AddressAddUint32 (loadoffset, 4), RELOCATABLE_CSIZE (from)))
	  out_of_block = TRUE;
      }
    }
    else
    {
      /* regular load instructions */
      if (AddressIsGe (loadoffset, RELOCATABLE_CSIZE (from)))
	out_of_block = TRUE;
      if (ARM_INS_OPCODE (ins) == ARM_LDRD &&
          AddressIsGt (AddressAddUint32 (loadoffset, 4), RELOCATABLE_CSIZE (from)))
        out_of_block = TRUE;
    }
    if (out_of_block)
      can_emulate = FALSE;
  }
  /* }}} */

  bzero(&dest,sizeof(t_register_content));

  if (can_emulate)
  {
    /* {{{ emulate the load if it comes from a constant memory location
     * at this point, we know
     * 	- the address to load from
     * 	- the relocation associated with the load address 
     * 	- the relocatable from which we will load 
     * 	- we are sure the load offset falls within the block */

    switch (ARM_INS_OPCODE (ins))
    {
      case ARM_LDRD:
      case ARM_LDR:
      case ARM_LDRB:
      case ARM_LDRSH:
      case ARM_LDRH:
      case ARM_LDRSB:
	{
	  if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_SUBSECTION)
	  {
	    /* {{{ load from section */
	    t_section *sec = T_SECTION (from);
	    if (SECTION_TYPE (sec) == CODE_SECTION)
	      FATAL (("Should not happen: this should a basic block"));

	    if (SECTION_TYPE (sec) == RODATA_SECTION ||
		IsKnownToBeConstant (loadaddr, base_reloc))
	    {
	      if (SECTION_TYPE(sec) == BSS_SECTION)
	      {
		/* constant == 0 */
		t_register_content rc;
		rc.i = AddressNullForSection(sec);
		ProcStateSetReg(state,ARM_INS_REGA(ins),rc);
		ProcStateSetTagTop(state,ARM_INS_REGA(ins));
                if (ARM_INS_OPCODE(ins) == ARM_LDRD)
		{
		  ProcStateSetReg(state,ARM_INS_REGA(ins)+1,rc);
		  ProcStateSetTagTop(state,ARM_INS_REGA(ins)+1);
		}
	      }
	      else
	      {
		t_uint32 val;
		t_reloc_ref *rr;
    t_bool read_ignored = FALSE;

		/* read the actual value */
                switch (ARM_INS_OPCODE (ins))
                  {
                  case ARM_LDRD:
                  case ARM_LDR:
                    val = SectionGetData32Ignore (sec, loadoffset, &read_ignored);
                    if (read_ignored)
                      VERBOSE(1, ("out of section read while getting 32-bit word from @T at offset @G for @I", sec, loadoffset, ins));
                    break;
                  case ARM_LDRB:
                  case ARM_LDRSB:
                    val = SectionGetData8 (sec, loadoffset);
                    break;
                  case ARM_LDRSH:
                  case ARM_LDRH:
                    val = SectionGetData16 (sec, loadoffset);
                    break;

                  default:
                    val = 0;
                    break;
                  }
		val = adjust_loaded_value (ins, val);
		dest.i = AddressNew32 (val);
		ProcStateSetReg(state, ARM_INS_REGA(ins), dest);
                if (ARM_INS_OPCODE(ins) == ARM_LDRD)
		{
                        /* Sometimes the use of a LDRD-instruction results in the situation where
                         * the first 32-bit word is located right at the end of a section and the
                         * second 32-bit word is, consequently, located past the end of that section.
                         * In this situation, the out-of-section read can be ignored IF AND ONLY IF
                         * we also stop propagating the information! */
                        t_bool ignored = FALSE;
                        val = SectionGetData32Ignore (sec, AddressAddInt32(loadoffset,4), &ignored);

                        if (!ignored)
                        {
                                dest.i = AddressNew32 (val);
                                ProcStateSetReg(state, ARM_INS_REGA(ins)+1, dest);
                        }
		}
		/* is the memory location relocated? */
		
                if (ARM_INS_OPCODE(ins)==ARM_LDR || ARM_INS_OPCODE(ins)==ARM_LDRD)
                  {
                    for (rr = SECTION_REFERS_TO (sec); rr; rr = RELOC_REF_NEXT(rr))
                      if (AddressIsEq (RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)), loadoffset))
                        break;
                    if (rr)
                      ProcStateSetTag (state, ARM_INS_REGA (ins), RELOC_REF_RELOC(rr));
                    else
                      ProcStateSetTagTop (state, ARM_INS_REGA (ins));
                    
                    if (ARM_INS_OPCODE (ins) == ARM_LDRD)
                      {
                        t_address nloadoffset = AddressAddInt32(loadoffset,4);
                        
                        for (rr = SECTION_REFERS_TO (sec); rr; rr = RELOC_REF_NEXT(rr))
                          if (AddressIsEq (RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)), nloadoffset))
                            break;
                        if (rr)
                          ProcStateSetTag (state, ARM_INS_REGA (ins)+1, RELOC_REF_RELOC(rr));
                        else
                          ProcStateSetTagTop (state, ARM_INS_REGA (ins)+1);
                      }
                  }
                else if (ARM_INS_OPCODE(ins)==ARM_LDRB || 
                         ARM_INS_OPCODE(ins)==ARM_LDRSB || 
                         ARM_INS_OPCODE(ins)==ARM_LDRH || 
                         ARM_INS_OPCODE(ins)==ARM_LDRSH
                         )
                  {
                    for (rr = SECTION_REFERS_TO (sec); rr; rr = RELOC_REF_NEXT(rr))
                      if ((G_T_UINT32(RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr))) & ~0x3) == (G_T_UINT32(loadoffset) &~0x3))
                        break;
                    if (rr) {
                      /* don't do the load, we are loading a part of a relocatable address
                         which we cannot handle at all */
                      ProcStateSetRegBot(state,ARM_INS_REGA(ins));
                      ProcStateSetTagBot(state,ARM_INS_REGA(ins));
                    }
                    else
                      ProcStateSetTagTop (state, ARM_INS_REGA (ins));
                  }
              }
            }
	    else
	    {
	      /* don't do load: the memory location isn't constant */
	      ProcStateSetRegBot(state,ARM_INS_REGA(ins));
	      ProcStateSetTagBot(state,ARM_INS_REGA(ins));
	      if (ARM_INS_OPCODE(ins) == ARM_LDRD)
	      {
		ProcStateSetRegBot(state,ARM_INS_REGA(ins)+1);
		ProcStateSetTagBot(state,ARM_INS_REGA(ins)+1);
	      }
	    }
	    /* }}} */
	  }
	  else if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_BBL)
	  {
	    /* {{{ load from bbl */
	    t_bbl * bbl = T_BBL (from);
	    t_uint32 byte;	    
	    t_arm_ins *data;

	    BBL_FOREACH_ARM_INS (bbl, data)
	    {
              if (AddressIsGe(loadoffset,ARM_INS_CSIZE(data)))
                loadoffset = AddressSub(loadoffset,ARM_INS_CSIZE(data));
              else
		break;
	    }
            byte = G_T_UINT32(loadoffset);
	    ASSERT (data, ("Uncaught out-of-block load"));
	    load_value_from_insdata_in_reg(ins,data,ARM_INS_REGA(ins),byte,state);
	    if (ARM_INS_OPCODE(ins) == ARM_LDRD)
	    {
	      data=ARM_INS_INEXT(data);
	      ASSERT(data,("Uncaught out-of-block 8-byte load"));
	      load_value_from_insdata_in_reg(ins,data,ARM_INS_REGA(ins)+1,byte,state);
	    }
	    /* }}} */
	  }
	  else
	  {
	    /* probably loading from a vectorized section */
	    /* don't do load: the memory location isn't constant or this happens
	     * too unfrequently to be interesting to implement. Be our guest :-) */
	    ProcStateSetRegBot(state,ARM_INS_REGA(ins));
	    ProcStateSetTagBot(state,ARM_INS_REGA(ins));
	    if (ARM_INS_OPCODE(ins) == ARM_LDRD)
	    {
	      ProcStateSetRegBot(state,ARM_INS_REGA(ins)+1);
	      ProcStateSetTagBot(state,ARM_INS_REGA(ins)+1);
	    }

/*            FATAL (("unexpected from type for load relocation (%d) @I @R", RELOCATABLE_RELOCATABLE_TYPE (from), ins, base_reloc));*/
	  }
	}
	break;
      case ARM_LDM:
	{
	  if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_SUBSECTION)
	  {
	    t_section *sec = T_SECTION (from);
	    if (SECTION_TYPE(sec) == CODE_SECTION)
	      FATAL (("Should not happen"));

	    if (SECTION_TYPE (sec) == RODATA_SECTION)
	    {
	      for (i = 0; i < 16; ++i)
	      {
		t_uint32 val;
		t_reloc_ref *rr;

		if (!(ARM_INS_IMMEDIATE (ins) & (1 << i))) continue;

		/* read the actual value */
		val = SectionGetData32 (sec, loadoffset);

		dest.i = AddressNew32 (val);
		ProcStateSetReg(state, i, dest);

		/* is the memory location relocated? */
		for (rr = SECTION_REFERS_TO (sec); rr; rr = RELOC_REF_NEXT(rr))
		  if (AddressIsEq (RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)), loadoffset))
		    break;
		if (rr)
		  ProcStateSetTag (state, i, RELOC_REF_RELOC(rr));
		else
		  ProcStateSetTagTop (state, i);

		if (up)
		  loadoffset = AddressAddUint32 (loadoffset, 4);
		else
		  loadoffset = AddressSubUint32 (loadoffset, 4);
	      }
	    }
	    else
	    {
	      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
              ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
	    }
	  }
	  else if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_BBL)
	  {
	    /* load from bbl {{{*/
	    t_bbl * bbl = T_BBL (from);
	    t_arm_ins *data;
	    t_uint32 i;

	    if (preindex)
            {
              if (up)
                loadoffset = AddressAddUint32(loadoffset, 4);
              else
                loadoffset = AddressSubUint32(loadoffset, 4);
            }

	    i = 0;
	    BBL_FOREACH_ARM_INS (bbl, data)
	    {
              if (AddressIsGe(loadoffset,ARM_INS_CSIZE(data)))
                loadoffset = AddressSub(loadoffset,ARM_INS_CSIZE(data));
              else
		break;
	    }
	    ASSERT (data, ("out of block load undetected"));

	    for (i = 0; i < 16; ++i)
	    {
	      if (!(ARM_INS_IMMEDIATE (ins) & (1 << i))) continue;
              ASSERT(G_T_UINT32(ARM_INS_CSIZE(data))==4,("Non-4-byte instruction @I from @I, size = @G",data,ins,ARM_INS_CSIZE(ins)));

	      dest.i = AddressNew32 (ARM_INS_IMMEDIATE (data));
	      ProcStateSetReg (state, i, dest);

	      if (ARM_INS_REFERS_TO (data))
		ProcStateSetTag (state, i, RELOC_REF_RELOC(ARM_INS_REFERS_TO (data)));
	      else
		ProcStateSetTagTop (state, i);
	      if (up)
		data = ARM_INS_INEXT (data);
	      else
		data = ARM_INS_IPREV (data);
	    }
	    /*}}}*/
	  }
	  else
	  {
	    ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
	    ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
	  }
	}
	break;
      default:
	FATAL (("Implement load operation @I", ins));
    }
    /* }}} */
  }
  else if (ISVALUE (base_reloc_unknown) && from &&
      !be_conservative && !alignment_violation && !out_of_block)
  {
    /* if we know the tag and it points to a block with only one outgoing relocation,
     * we can set the tag of the loaded register to this one relocation */
    /* {{{ */
    t_reloc *rel = NULL;
    t_bool multiple_rels = FALSE;

    if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_BBL)
    {
      t_bbl *bbl = T_BBL (from);
      t_arm_ins *i_ins;
      BBL_FOREACH_ARM_INS (bbl, i_ins)
      {
	if (ARM_INS_REFERS_TO (i_ins))
	{
	  if (!rel)
	    rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(i_ins));
	  else if (!TwoRelocsInSameBlock (RELOC_REF_RELOC(ARM_INS_REFERS_TO(i_ins)), rel))
	  {
	    multiple_rels = TRUE;
	    break;
	  }
	}
      }
    }
    else if (RELOCATABLE_RELOCATABLE_TYPE (from) == RT_SUBSECTION)
    {
      t_reloc_ref *rr;
      for (rr = RELOCATABLE_REFERS_TO (from); rr; rr = RELOC_REF_NEXT(rr))
      {
	if (!rel)
	  rel = RELOC_REF_RELOC(rr);
	else if (!TwoRelocsInSameBlock (RELOC_REF_RELOC(rr), rel))
	{
	  multiple_rels = TRUE;
	  break;
	}
      }
    }
    else
      multiple_rels = TRUE;

    switch (ARM_INS_OPCODE (ins))
    {
      case ARM_LDRD:
        SetLoadedRegTags(state,ARM_INS_REGA(ins)+1,rel,multiple_rels);
      case ARM_LDR:
      case ARM_LDRB:
      case ARM_LDRSH:
      case ARM_LDRH:
      case ARM_LDRSB:
        SetLoadedRegTags(state,ARM_INS_REGA(ins),rel,multiple_rels);
	break;
      case ARM_LDM:
	ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
	if (multiple_rels)
	  ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
	else if (rel)
	{
	  int i;
	  for (i = 0; i < 16; i ++)
	    if (ARM_INS_IMMEDIATE (ins) & (1 << i))
	      ProcStateSetTag (state, i, rel);
	}
	else
	  ProcStateSetTagsetTop(state,ARM_INS_REGS_DEF(ins));
	break;
      default:
	FATAL (("unexpected instruction @I", ins));
    }
    /* }}} */
  }
#ifdef LOAD_AGGRESSIVE
  else if (out_of_block)
  {
    /* aggressive assumption: out of block loads mean that this code can never be 
     * executed in a correctly working program {{{ */
    if ((ARM_INS_OPCODE (ins) == ARM_LDM) ||
        (ARM_INS_OPCODE (ins) == ARM_LDRD))
    {
      ProcStateSetRegsetTop(state,ARM_INS_REGS_DEF(ins));
      ProcStateSetTagsetTop(state,ARM_INS_REGS_DEF(ins));
    }
    else
    {
      ProcStateSetRegTop (state, ARM_INS_REGA (ins));
      ProcStateSetTagTop (state, ARM_INS_REGA (ins));
    } /* }}} */
  }
#endif
  else
  {
    /* be conservative {{{ */
    if ((ARM_INS_OPCODE (ins) == ARM_LDM) ||
        (ARM_INS_OPCODE (ins) == ARM_LDRD))
    {
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
    }
    else
    {
      ProcStateSetRegBot (state, ARM_INS_REGA (ins));
      ProcStateSetTagBot (state, ARM_INS_REGA (ins));
    } /* }}} */
  }

  /* If we have writeback, emulate the indexing operation {{{ */
  if (ARM_INS_OPCODE(ins)==ARM_LDM)
  {
    if (ARM_INS_FLAGS (ins) & FL_WRITEBACK)
    {
      if (!can_emulate)
      {
	ProcStateSetRegBot(state,ARM_INS_REGB(ins));
      } 
      else 
      {
#ifdef THUMB_SUPPORT
	/* In thumb mode, it is legal to include the base register in the list of loaded registers. The final value
	 * is the loaded value, and not the writeback value! */
	if(!(ARM_INS_FLAGS(ins) & FL_THUMB) && 
	    (ARM_INS_IMMEDIATE(ins) & (1 <<ARM_INS_REGB(ins)))) 
	  WARNING(("Result is unpredictable!"));
	if(!(ARM_INS_IMMEDIATE(ins) & (1 <<ARM_INS_REGB(ins))))
	{
#endif
	if (up)
	  dest.i = AddressAddUint32 (loadaddr, 4*num_regs);
	else 
	  dest.i = AddressSubUint32 (loadaddr, 4*num_regs);
	ProcStateSetReg(state,ARM_INS_REGB(ins),dest);
#ifdef THUMB_SUPPORT
	}
#endif
      }
    }
  }
  else if (!preindex || (ARM_INS_FLAGS (ins) & FL_WRITEBACK)) 
  {
    if ((!can_emulate))
    {
      ProcStateSetRegBot(state,ARM_INS_REGB(ins));
    } 
    else 
    {
      if (preindex)
      {
	dest.i = loadaddr;
      }
      else
      {
	if (up)
	  dest.i=AddressAdd (loadaddr, tmp_offset.i);
	else
	  dest.i=AddressSub (loadaddr, tmp_offset.i);
      }
      ProcStateSetReg(state,ARM_INS_REGB(ins),dest);
    }
  }
  /*}}} */
}
/*}}}*/

/* Emulate multiple stores {{{*/
static void ArmInsEmulMultipleStore(t_arm_ins * ins, t_procstate * state)
{
  t_reloc * base_reloc;
  t_register_content tmp_base_address, tmp_offset, dest;
  t_uint32 base_address,offset;
  t_lattice_level base_address_unknown;
  t_lattice_level offset_unknown;
  t_lattice_level base_reloc_unknown;
  t_instruction_flags insflags = ARM_INS_FLAGS(ins);
  t_uint32 num_regs = 0;
  int i;
  
  for (i=0; i<16; i++)
    if (ARM_INS_IMMEDIATE(ins) & (1 << i)) 
      num_regs++;
  
  /* get the base address and reloc from the state */
  
  base_reloc_unknown = ProcStateGetTag(state,ARM_INS_REGB(ins),&base_reloc);
  base_address_unknown = ProcStateGetReg(state,ARM_INS_REGB(ins),&tmp_base_address);
  base_address = G_T_UINT32(tmp_base_address.i);
  
  /* get the offset from the state */

  if (ArmInsHasImmediate(ins))
    {
      offset_unknown = CP_VALUE;
      tmp_offset.i=AddressNew32(ARM_INS_IMMEDIATE(ins));
    }
  else if (ArmInsHasShiftedFlexible(ins))
    {
      offset_unknown = ArmInsExtractShift(ins,state,&tmp_offset);
    }
  else
    offset_unknown = ProcStateGetReg(state,ARM_INS_REGC(ins),&tmp_offset);
  
  offset = G_T_UINT32(tmp_offset.i);

  
  if (ISBOT(base_address_unknown)) 
    {
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
      return;
    }

  
  if (insflags & FL_WRITEBACK)
    {	/* writeback enabled */
      if ((!ISVALUE(base_address_unknown)) || (!ISVALUE(offset_unknown)))
	{
	  ProcStateSetRegBot(state,ARM_INS_REGB(ins));
	} 
      else 
	{
	  if (insflags & FL_DIRUP)
	    dest.i=AddressNew32(base_address + 4 * num_regs);
	  else 
	    dest.i=AddressNew32(base_address - 4 * num_regs);
	  if (ISVALUE(base_reloc_unknown))
	    {
	      ProcStateSetTag(state,ARM_INS_REGB(ins),base_reloc);
	    }
	  ProcStateSetReg(state,ARM_INS_REGB(ins),dest);
	}
    }
}
/*}}}*/

/* Emulate stores {{{*/
static void ArmInsEmulStore(t_arm_ins * ins, t_procstate * state)
{
  t_reloc * base_reloc;
  t_register_content tmp_base_address, tmp_offset, dest;
  t_uint32 base_address,offset;
  t_lattice_level base_address_unknown;
  t_lattice_level offset_unknown;
  t_lattice_level base_reloc_unknown;
  t_instruction_flags insflags = ARM_INS_FLAGS(ins);

  /* get the base address and reloc from the state */
  
  base_reloc_unknown = ProcStateGetTag(state,ARM_INS_REGB(ins),&base_reloc);
  base_address_unknown = ProcStateGetReg(state,ARM_INS_REGB(ins),&tmp_base_address);
  base_address = G_T_UINT32(tmp_base_address.i);
  
  /* get the offset from the state {{{ */

  if (ArmInsHasImmediate(ins))
    {
      offset_unknown = CP_VALUE;
      tmp_offset.i=AddressNew32(ARM_INS_IMMEDIATE(ins));
    }
  else if (ArmInsHasShiftedFlexible(ins))
    {
      offset_unknown = ArmInsExtractShift(ins,state,&tmp_offset);
    }
  else
    offset_unknown = ProcStateGetReg(state,ARM_INS_REGC(ins),&tmp_offset);
  /* }}} */
  
  offset = G_T_UINT32(tmp_offset.i);
  
  if (ISBOT(base_address_unknown))
    {
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      return;
    }

  bzero(&dest,sizeof(t_register_content));
  
  if (!(insflags & FL_PREINDEX) || (insflags & FL_WRITEBACK)) 
    {	/* writeback enabled */
      if ((!ISVALUE(base_address_unknown)) || (!ISVALUE(offset_unknown)))
	{
	  ProcStateSetRegBot(state,ARM_INS_REGB(ins));
	} 
      else 
	{
	  if (insflags & FL_DIRUP)
	    dest.i=AddressNew32(base_address + offset);
	  else 
	    dest.i=AddressNew32(base_address - offset);
	  if (ISVALUE(base_reloc_unknown))
	    ProcStateSetTag(state,ARM_INS_REGB(ins),base_reloc);
	  ProcStateSetReg(state,ARM_INS_REGB(ins),dest);
	}
    }
}/*}}}*/

/* }}} */

/*!
 * \todo Document and move to constant propagator
 *
 * \param reg
 * \param state_a
 * \param state_b
 *
 * \return t_bool 
*/
/* Move me to constant propagator {{{ */
t_bool RegIsEqualAndKnown(t_reg reg, t_procstate * state_a, t_procstate * state_b)
{
  t_register_content value1;
  t_register_content value2;

  if (!ISVALUE(ProcStateGetReg(state_a,reg, &value1)))
    return FALSE;

  if (!ISVALUE(ProcStateGetReg(state_b,reg, &value2)))
    return FALSE;

  if (AddressIsEq(value1.i,value2.i))
    return TRUE;

  return FALSE;
}
/* }}} */
/*!
 * \todo Document and move to constant propagator
 *
 * \param state_a
 * \param state_b
 *
 * \return t_bool 
*/
/*  Move me to constant propagator {{{ */
t_bool ConditionsAreEqualAndKnown(t_procstate * state_a, t_procstate * state_b)
{
  t_bool value_a1;
  t_bool value_a2;
  t_bool value_a3;
  t_bool value_a4;

  t_bool value_b1;
  t_bool value_b2;
  t_bool value_b3;
  t_bool value_b4;

  if (ProcStateGetCond(state_a,ARM_REG_N_CONDITION,&value_a1))
    return FALSE;
  if (ProcStateGetCond(state_b,ARM_REG_N_CONDITION,&value_b1))
    return FALSE;
  if (value_a1!=value_b1)
    return FALSE;

  if (ProcStateGetCond(state_a,ARM_REG_V_CONDITION,&value_a2))
    return FALSE;
  if (ProcStateGetCond(state_b,ARM_REG_V_CONDITION,&value_b2))
    return FALSE;
  if (value_a2!=value_b2)
    return FALSE;

  if (ProcStateGetCond(state_a,ARM_REG_Z_CONDITION,&value_a3))
    return FALSE;
  if (ProcStateGetCond(state_b,ARM_REG_Z_CONDITION,&value_b3))
    return FALSE;
  if (value_a3!=value_b3)
    return FALSE;
  
  if (ProcStateGetCond(state_a,ARM_REG_C_CONDITION,&value_a4))
    return FALSE;
  if (ProcStateGetCond(state_b,ARM_REG_C_CONDITION,&value_b4))
    return FALSE;
  if (value_a4!=value_b4)
    return FALSE;

 return TRUE;
}

/*}}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param state
 *
 * \return void 
*/
/* The instruction emulator that ignores conditions {{{ */
void ArmInsEmulNoConditions(t_arm_ins * ins, t_procstate * state)
{
  t_regset mask = NullRegs;
  /* excluding the PC is a hack as long as address producers depend on the PC */

  switch (ARM_INS_OPCODE(ins))
    {
      case ARM_PSEUDO_CALL:
	FATAL(("Pseudo call in emulation!"));
	break;
    case ARM_CONSTANT_PRODUCER:
      ArmInsEmulConstProd(ins, state);
      break;
    case ARM_ADDRESS_PRODUCER:
      ArmInsEmulAddrProd(ins, state);
      break;
    case ARM_UMULL:
    case ARM_MUL:
    case ARM_MLA:
    case ARM_AND:
    case ARM_TST:
    case ARM_BIC:
    case ARM_EOR:
    case ARM_ORR:
    case ARM_TEQ:
    case ARM_MOV:
    case ARM_MVN:
    case ARM_CMN:
    case ARM_CMP:
    case ARM_ADD:
    case ARM_ADC:
    case ARM_SUB:
    case ARM_SBC:
    case ARM_RSC:
    case ARM_RSB:
      ArmInsEmulNumericalAndLogical(ins, state);
      break;
    case ARM_LDRD:
    case ARM_LDR:
    case ARM_LDRB:
    case ARM_LDRSH:
    case ARM_LDRH:
    case ARM_LDRSB:
    case ARM_LDM:
      ArmInsEmulLoad(ins,state);
      break;
    case ARM_STR:
    case ARM_STRB:
    case ARM_STRH:
    case ARM_STRD:
      ArmInsEmulStore(ins,state);
      break;
    case ARM_STM:
      ArmInsEmulMultipleStore(ins,state);
      break;
    case ARM_B:
    case ARM_BL:
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      break;
    case ARM_SWI:
    case ARM_MRS:
    case ARM_MSR:
    case ARM_UMLAL:
    case ARM_SMULL:
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      if (ARM_INS_FLAGS(ins) & FL_S)
      {
	RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
	RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
	ProcStateSetCondsetBot(state,mask);
      }
      ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
      break;

    default:
      ProcStateSetRegsetBot(state,ARM_INS_REGS_DEF(ins));
      if (ARM_INS_FLAGS(ins) & FL_S)
        {
          RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
          ProcStateSetCondsetBot(state,mask);
        }
      ProcStateSetTagsetBot(state,ARM_INS_REGS_DEF(ins));
      break;
    }
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param state
 *
 * \return void 
*/
/* The complete instruction emulator {{{ */

void 
ArmInsEmulator(t_arm_ins *  ins, t_procstate * state, t_bool update_known_values)
{
  t_bool execute, flag1,flag2;
  t_bool uncertain = FALSE;
  t_procstate * orig_state = NULL;

  /* step one: if the predicate cannot hold, the instruction will not
     be executed */

  switch(ARM_INS_CONDITION(ins))
  {
    case ARM_CONDITION_EQ :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute)))
	if (!execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_NE :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute)))
	if (execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_CS :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_C_CONDITION,&execute)))
	if (!execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_CC :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_C_CONDITION,&execute)))
	if (execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_MI :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_N_CONDITION,&execute)))
	if (!execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_PL :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_N_CONDITION,&execute)))
	if (execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_VS : 
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_V_CONDITION,&execute)))
	if (!execute) 
	  goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_VC :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_V_CONDITION,&execute)))
	if (execute) 
	  goto nothing_to_be_done; 
      break;

    case ARM_CONDITION_HI :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_C_CONDITION,&execute)))
	if (!execute) 
	  goto nothing_to_be_done; 
      if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute))))
	if (execute) 
	  goto nothing_to_be_done; 
      break;

    case ARM_CONDITION_LS :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_C_CONDITION,&execute)))
	if (execute) 
	  if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute))))
	    if (!execute) 
	      goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_GE :
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_N_CONDITION,&flag1)))
	if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_V_CONDITION,&flag2))))
	  if (flag1 != flag2)
	    goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_LT : 
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_N_CONDITION,&flag1)))
	if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_V_CONDITION,&flag2))))
	  if (flag1 == flag2)
	    goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_GT : 
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute)))
	if (execute) 
	  goto nothing_to_be_done;
      if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_N_CONDITION,&flag1))))
	if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_V_CONDITION,&flag2))))
	  if (flag1 != flag2)
	    goto nothing_to_be_done; 
      break;

    case ARM_CONDITION_LE : 
      if (!(uncertain = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&execute)))
	if (!execute) 
	  if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_N_CONDITION,&flag1))))
	    if (!(uncertain = (uncertain || ProcStateGetCond(state,ARM_REG_V_CONDITION,&flag2))))
	      if (flag1 == flag2)
		goto nothing_to_be_done; 
      break;
    case ARM_CONDITION_AL:
      if (ARM_INS_OPCODE(ins)==ARM_T2CBZ || ARM_INS_OPCODE(ins)==ARM_T2CBNZ)
        {

          /* this is not very elegant, but we don't see another option 
             in ARM code, instructions are modeled as conditionally executed
             when they depend on condition flags, but CBZ and CBNZ thumb instructions
             are modeled as (un)executed if the condition checked in the instruction
             is (not) met */
          t_reg reg = ARM_INS_REGB(ins);
          t_register_content content;
          
          if (CP_BOT==ProcStateGetReg(state,reg,&content))
            {
              ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) | IF_EXECED);
              ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & ~IF_ALWAYS_EXECED);
            }
          else if (G_T_UINT32(content.i)==0 && ARM_INS_OPCODE(ins)==ARM_T2CBZ)
            {
              ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) | IF_EXECED);
            }
          else if (G_T_UINT32(content.i)!=0 && ARM_INS_OPCODE(ins)==ARM_T2CBNZ)
            {
              ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) | IF_EXECED);
            }
          else
            {
              ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & ~IF_ALWAYS_EXECED);
            }
          return;
        }
      else
        ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & ~IF_ALWAYS_EXECED);
      break;
    case ARM_CONDITION_NV : 
      goto nothing_to_be_done; 
  }

  /* set instruction flags */

  /* an instruction can be executed */

  ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) | IF_EXECED);

  if (uncertain)
  {
    /* but it will not always be executed */
    ARM_INS_SET_ATTRIB(ins,    ARM_INS_ATTRIB(ins) & ~IF_ALWAYS_EXECED);
    orig_state = ProcStateNew(&arm_description);
    ProcStateDup(orig_state,state,&arm_description);
  }

  /* actual emulation */

  ArmInsEmulNoConditions(ins,state);

  if (uncertain) /* We don't know what happened, join both the new and the prev state*/
  {
    ProcStateJoinSimple(state, orig_state , arm_description.all_registers, &arm_description);
    ProcStateFree(orig_state);
    orig_state = NULL;
  }

  return;

nothing_to_be_done:

  /* it might be that an instruction is never executed in one context,
     while it is executed in another one */

  ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & ~IF_ALWAYS_EXECED);

  return;
}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
