/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>


t_uint32 ArmPeepholeCombineIdenticalConditionalIns(t_cfg * cfg)
{
  t_bbl * bbl;
  t_arm_ins * ins, *jins, *tmp;
  t_uint32 killers = 0;
  /*static int count = 0;*/

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_ARM_INS_SAFE(bbl,ins,tmp)
    if ((jins=ARM_INS_INEXT(ins)) && 
	ArmInsIsConditional(ins) && 
	ArmCompareInstructionsOppositeCond(ins,jins) 
	&& !ArmInsUpdatesCond(ins))
      /*if (count++ < diablosupport_options.debugcounter)*/
      {
	VERBOSE(1, ("optimizing @I\n@I",ins,jins));
	ARM_INS_SET_EXEC_COUNT(jins, ARM_INS_EXEC_COUNT(jins)+ARM_INS_EXEC_COUNT(ins));
	ArmInsKill(ins);
	killers++;
	ArmInsUnconditionalize(jins);
	ARM_INS_SET_REGS_USE(jins,  ArmUsedRegisters(jins));
	ARM_INS_SET_REGS_DEF(jins,  ArmDefinedRegisters(jins));
        ASSERT(ArmInsIsEncodable(jins), ("This instruction can't be encoded (ArmPeepholeCombineIdenticalConditionalIns) @I", jins));
	VERBOSE(1, ("====== end result @I",jins));
      }

  VERBOSE(0, ("Removed %d identical instructions with opposite conditions", killers));
  return killers;
}

t_uint32 ArmPeepholeCombineAdds(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * i_ins, *j_ins;
  t_uint32 killers = 0;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
      BBL_FOREACH_ARM_INS(bbl,i_ins)
      {
	if (ARM_INS_OPCODE(i_ins) == ARM_ADD 
	    && ARM_INS_REGA(i_ins) == ARM_INS_REGB(i_ins)
	    && (ARM_INS_FLAGS(i_ins)&FL_IMMED) 
	    && ARM_INS_SHIFTTYPE(i_ins) == ARM_SHIFT_TYPE_NONE)
	{
	  if( ARM_INS_IPREV(i_ins) 
	      && ARM_INS_OPCODE(ARM_INS_IPREV(i_ins)) == ARM_ADD 
	      && ARM_INS_REGA(i_ins) == ARM_INS_REGA(ARM_INS_IPREV(i_ins)) 
	      && ARM_INS_REGC(ARM_INS_IPREV(i_ins)) == ARM_REG_NONE 
	      && ARM_INS_SHIFTTYPE(ARM_INS_IPREV(i_ins)) == ARM_SHIFT_TYPE_NONE 
	      && ARM_INS_CONDITION(i_ins) == ARM_INS_CONDITION(ARM_INS_IPREV(i_ins)))
	  {
	    t_uint32 sum = ARM_INS_IMMEDIATE(i_ins) + ARM_INS_IMMEDIATE(ARM_INS_IPREV(i_ins));
	    if(ArmInsIsEncodableConstantForOpcode(sum,ARM_ADD,ARM_INS_FLAGS(i_ins) & FL_THUMB))
	    {
	      j_ins = ARM_INS_IPREV(i_ins);
	      VERBOSE(1, ("Combining adds of @I and @I", i_ins, j_ins));
	      ARM_INS_SET_IMMEDIATE(j_ins,  sum);
	      ArmInsKill(i_ins);
	      i_ins = j_ins;
	      killers++;
	      ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	      ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
                ASSERT(ArmInsIsEncodable(j_ins), ("This instruction can't be encoded (ArmPeepholeCombineAdds) @I", j_ins));
	    }
	  }
	}
      }
  if(killers)
    VERBOSE(0,("ArmPeepholeCombineAdds killed %d",killers));
  return killers;
}


t_uint32 ArmPeepholeUselessAdds(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * i_ins;
  t_uint32 killers = 0;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
      BBL_FOREACH_ARM_INS(bbl,i_ins)
      {
	if(ARM_INS_OPCODE(i_ins) == ARM_ADD)
	{
	  if((ARM_INS_FLAGS(i_ins)&FL_IMMED) /*ARM_INS_REGC(i_ins) == ARM_REG_NONE*/ && ARM_INS_SHIFTTYPE(i_ins) == ARM_SHIFT_TYPE_NONE && !(ARM_INS_FLAGS(i_ins)&FL_S) &&  ARM_INS_IMMEDIATE(i_ins) == 0)
	  {
	    /*DiabloPrint(stdout,"ArmPeepholeUselessAdds: @I\n",i_ins);*/
	    ArmInsMakeMov(i_ins,ARM_INS_REGA(i_ins),ARM_INS_REGB(i_ins),0,ARM_INS_CONDITION(i_ins));
	    ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
	    ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
                ASSERT(ArmInsIsEncodable(i_ins), ("This instruction can't be encoded (ArmPeepholeUselessAdds) @I", i_ins));
	  }
	}
      }

  return killers;
}

t_uint32 ArmPeepholeUselessAdd_Sub(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * i_ins, *j_ins, *tmp;
  t_uint32 killers = 0;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
      BBL_FOREACH_ARM_INS_SAFE(bbl,i_ins,tmp)
      {
	if(ARM_INS_OPCODE(i_ins) == ARM_ADD
	    && ARM_INS_INEXT(i_ins)
	    && ARM_INS_OPCODE(ARM_INS_INEXT(i_ins)) == ARM_SUB
	    && ARM_INS_REGA(i_ins) == ARM_INS_REGB(i_ins)
	    && (ARM_INS_FLAGS(i_ins) & FL_IMMED)
	    && !(ARM_INS_FLAGS(i_ins) & FL_S))
	{
	  j_ins = ARM_INS_INEXT(i_ins);
	  if(ARM_INS_REGA(j_ins) == ARM_INS_REGA(i_ins)
	      && ARM_INS_REGB(j_ins) == ARM_INS_REGA(i_ins)
	      && (ARM_INS_FLAGS(j_ins) & FL_IMMED)
	      && !(ARM_INS_FLAGS(j_ins) & FL_S)
	      && ARM_INS_IMMEDIATE(i_ins) == ARM_INS_IMMEDIATE(j_ins)
	      && ARM_INS_CONDITION(i_ins) == ARM_INS_CONDITION(j_ins))
	  {
	    tmp = ARM_INS_INEXT(ARM_INS_INEXT(i_ins));
	    ArmInsKill(i_ins);
	    ArmInsKill(j_ins);
	    killers +=2;
	  }
	}
      }
  if(killers)
    VERBOSE(0,("ArmPeepholeUselessAddSub killed %d",killers));
  return killers;
}

t_uint32 ArmPeepholeWriteBackAnomaly(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * i_ins, *j_ins;
  t_uint32 killers = 0;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
      BBL_FOREACH_ARM_INS(bbl,i_ins)
      {
	if(ArmInsWriteBackHappens(i_ins))
	{
	  j_ins = ARM_INS_INEXT(i_ins);
	  while(j_ins)
	  {
	    if(ARM_INS_OPCODE(j_ins) == ARM_ADD
		&& ARM_INS_REGA(j_ins) == ARM_INS_REGB(j_ins)
		&& ARM_INS_REGA(j_ins) == ARM_INS_REGB(i_ins)
		&& !(ARM_INS_FLAGS(i_ins) & FL_DIRUP)
		&& !(ARM_INS_FLAGS(j_ins) & FL_S) 
		&& ARM_INS_CONDITION(j_ins) == ARM_INS_CONDITION(i_ins) )
	    {
	      if((ARM_INS_OPCODE(i_ins) == ARM_LDM || ARM_INS_OPCODE(i_ins) == ARM_STM) && ARM_INS_REGB(i_ins) != ARM_REG_R13)
	      {
		t_uint32 nr_regs;
		t_regset regs = NullRegs;
		
		RegsetSetAddMultipleRegs(regs,ARM_INS_IMMEDIATE(i_ins));
		nr_regs = RegsetCountRegs(regs) * 4;
		if(nr_regs == ARM_INS_IMMEDIATE(j_ins))
		{
		  //DEBUG(("killing writebackanomaly @I",j_ins));
		  ArmInsKill(j_ins);
		  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)& ~FL_WRITEBACK);
		  ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
		  ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
		  killers++;
                  ASSERT(ArmInsIsEncodable(i_ins), ("This instruction can't be encoded (ArmPeepholeWriteBackAnomaly) @I", i_ins));
		}
	      }
	      else if((ARM_INS_OPCODE(i_ins) == ARM_LDR || ARM_INS_OPCODE(i_ins) == ARM_STR) && ARM_INS_REGB(i_ins) != ARM_REG_R13 && (ARM_INS_FLAGS(i_ins) & FL_IMMED))
	      {
		if(ARM_INS_IMMEDIATE(j_ins) == 4 && ARM_INS_IMMEDIATE(i_ins) == 4)
		{
		  //DEBUG(("killing writebackanomaly @I",j_ins));
		  ArmInsKill(j_ins);
		  killers++;
		  if(!(ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
		  {
		    ARM_INS_SET_IMMEDIATE(i_ins,  0);
		    ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)| FL_PREINDEX);
		  }
		  else ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)& ~FL_WRITEBACK);
		  ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
		  ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
                  ASSERT(ArmInsIsEncodable(i_ins), ("This instruction can't be encoded (ArmPeepholeWriteBackAnomaly) @I", i_ins));
		}
	      }
	      break;
	    }
	    if(ARM_INS_OPCODE(j_ins) == ARM_SUB
		&& ARM_INS_REGA(j_ins) == ARM_INS_REGB(j_ins)
		&& ARM_INS_REGA(j_ins) == ARM_INS_REGB(i_ins)
		&& (ARM_INS_FLAGS(i_ins) & FL_DIRUP)
		&& !(ARM_INS_FLAGS(j_ins) & FL_S) 
		&& ARM_INS_CONDITION(j_ins) == ARM_INS_CONDITION(i_ins) )
	    {
	      if(ARM_INS_OPCODE(i_ins) == ARM_LDM || ARM_INS_OPCODE(i_ins) == ARM_STM)
	      {
		t_uint32 nr_regs;
		t_regset regs = NullRegs;
		
		RegsetSetAddMultipleRegs(regs,ARM_INS_IMMEDIATE(i_ins));
		nr_regs = RegsetCountRegs(regs) * 4;
		if(nr_regs == ARM_INS_IMMEDIATE(j_ins))
		{
		  //DEBUG(("killing writebackanomaly @I",j_ins));
		  ArmInsKill(j_ins);
		  ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)& ~FL_WRITEBACK);
		  ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
		  ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
		  killers++;
		}
                  ASSERT(ArmInsIsEncodable(i_ins), ("This instruction can't be encoded (ArmPeepholeWriteBackAnomaly) @I", i_ins));
	      }
	      else if((ARM_INS_OPCODE(i_ins) == ARM_LDR || ARM_INS_OPCODE(i_ins) == ARM_STR) && (ARM_INS_FLAGS(i_ins) & FL_IMMED))
	      {
		if(ARM_INS_IMMEDIATE(j_ins) == 4 && ARM_INS_IMMEDIATE(i_ins) == 4)
		{
		  //DEBUG(("killing writebackanomaly @I",j_ins));
		  ArmInsKill(j_ins);
		  killers++;
		  if(!(ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
		  {
		    ARM_INS_SET_IMMEDIATE(i_ins,  0);
		    ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)| FL_PREINDEX);
		  }
		  else ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins)& ~FL_WRITEBACK);
		  ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
		  ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
                  ASSERT(ArmInsIsEncodable(i_ins), ("This instruction can't be encoded (ArmPeepholeWriteBackAnomaly) @I", i_ins));
		}
	      }
	      break;
	    }
	    if(RegsetIn(ARM_INS_REGS_DEF(j_ins),ARM_INS_REGB(i_ins))) break;
	    if(RegsetIn(ARM_INS_REGS_USE(j_ins),ARM_INS_REGB(i_ins))) break;
	    j_ins = ARM_INS_INEXT(j_ins);
	  }
	}
      }
  
  if(killers)
    VERBOSE(0,("ArmPeepholeWritebackAnomaly killed %d",killers));
  return killers;
}

void ArmPeepholeOptimizations(t_cfg * cfg)
{
  t_uint32 killers = 0;
  STATUS(START, ("Arm Peepholes"));
  killers += ArmPeepholeCombineAdds(cfg);
  killers += ArmPeepholeUselessAdds(cfg);
  killers += ArmPeepholeUselessAdd_Sub(cfg);
  killers += ArmPeepholeWriteBackAnomaly(cfg);
  killers += ArmPeepholeCombineIdenticalConditionalIns(cfg);
  STATUS(STOP, ("Arm Peepholes", killers));
  return;
/*  return killers;*/
}

/*static teller = 0; */

void SwitchMoves(t_cfg * cfg)
{ 
  t_bbl * bbl;
  t_arm_ins * ins;
  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_ARM_INS(bbl,ins)
    {
      t_reg src_reg;
      if (ARM_INS_OPCODE(ins)==ARM_MOV && (src_reg=ARM_INS_REGC(ins))!=ARM_REG_NONE &&
			 (ARM_INS_REGC(ins) != ARM_REG_R13) && ARM_INS_SHIFTTYPE(ins)==ARM_SHIFT_TYPE_NONE && (!(ARM_INS_FLAGS(ins) & (FL_S))))
		  {
			 t_arm_ins * jins = ARM_INS_IPREV(ins);
			 
			 if (!jins)  continue;
			 if ((ARM_INS_FLAGS(jins) & (FL_S))) continue;
			 switch (ARM_INS_OPCODE(jins)){

			 /* The following commented instructions do not use the destination register as a source operand
			  */
			 /*case ARM_MUL:
			 case ARM_MLA:
			 case ARM_MLS:
			 case ARM_SMLABB:
			 case ARM_SMLABT:
			 case ARM_SMLATB:
			 case ARM_SMLATT:
			 case ARM_SMLAWB:
			 case ARM_SMLAWT:
			 case ARM_SMLAD:
			 case ARM_SMLADX:
			 case ARM_SMMLA:
			 case ARM_SMMLAR:
			 case ARM_SMMLS:
			 case ARM_SMMLSR:
			 case ARM_SMLSD:
			 case ARM_SMLSDX:
			 case ARM_SMUAD:
			 case ARM_SMULBB:
			 case ARM_SMULTT:
			 case ARM_SMULBT:
			 case ARM_SMULTB:
			 case ARM_SMULWB:
			 case ARM_SMULWT:*/

			 case ARM_MOVT:
			 case ARM_BFI:
			 case ARM_BFC:
			 case ARM_SMLAL:
			 case ARM_UMLAL:
			 case ARM_SMLALBB:
			 case ARM_SMLALBT:
			 case ARM_SMLALTB:
			 case ARM_SMLALTT:
			 case ARM_SMLALD:
			 case ARM_SMLALDX:
			 case ARM_SMLSLD:
			 case ARM_SMLSLDX:
			 case ARM_UMAAL:
			 case ARM_LDRD:
			 case ARM_LDREXD:
			 	/* these instructions also use the destination register as a source register */
			 	/* we CANNOT exchange registers */
				continue;
			 default:
				break;
			 }

			 /* we do not consider V* instructions here */
			 
			 if (src_reg==ARM_REG_R15) continue;
			 if (jins && RegsetIn(ARM_INS_REGS_DEF(jins),src_reg) && ARM_INS_REGA(jins)==src_reg && ARM_INS_CONDITION(jins)==ARM_INS_CONDITION(ins))
				{
				  t_reg dest_reg = ARM_INS_REGA(ins);
				  if (dest_reg==ARM_REG_R15) continue;
				  
				  /* if (teller < diablosupport_options.debugcounter) */
				  { 
					 /* DEBUG(("BEFORE @I @I\n",jins,ins)); */
					 /* teller++; */
            t_reg aj = ARM_INS_REGA(jins);
            t_reg a  = ARM_INS_REGA(ins);
            t_reg c  = ARM_INS_REGC(ins);
            t_bool can_encode = FALSE, can_encode_j = FALSE;

					 ARM_INS_SET_REGA(jins, dest_reg);
					 ARM_INS_SET_REGC(ins, dest_reg);
					 ARM_INS_SET_REGA(ins, src_reg);

           can_encode = ArmInsIsEncodable(ins);
           can_encode_j = ArmInsIsEncodable(jins);
            if (can_encode && can_encode_j)
            {
               ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
               ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
               ARM_INS_SET_REGS_USE(jins,  ArmUsedRegisters(jins));
               ARM_INS_SET_REGS_DEF(jins,  ArmDefinedRegisters(jins));
            }
            else
            {
               ARM_INS_SET_REGA(jins, aj);
               ARM_INS_SET_REGC(ins, c);
               ARM_INS_SET_REGA(ins, a);
               //DEBUG(("  not encodable in Thumb, reverting changes"));

               if (!can_encode)   can_encode   = ArmInsIsEncodable(ins);
               if (!can_encode_j) can_encode_j = ArmInsIsEncodable(jins);
               ASSERT(can_encode && can_encode_j, ("undid switch moves for @I @I (original), but they aren't encodable in Thumb anymore! (%d %d)", jins, ins, can_encode_j, can_encode));
            }

					 /* DEBUG(("AFTER  @I @I\n",jins,ins));*/
				  }
				  
				}
		  }
    }
}
/* vim: set shiftwidth=2 foldmethod=marker : */
