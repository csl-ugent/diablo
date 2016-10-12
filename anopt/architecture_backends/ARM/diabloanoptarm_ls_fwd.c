/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*#define DEBUG_LS_FWD*/
#define DEBUG_LS_VERBOSE_LEVEL 2
/*#define EVEN_MORE_UNSAFE*/
/* Includes {{{*/
#include <diabloanoptarm.h>
#include <string.h>
#include <strings.h>
/*}}}*/

t_arm_ins * tmp_backup = NULL; // global backup for tmp in the BBL_FOREACH_ARM_INS_SAFE below, because tmp might be killed in the call chain of the loop body

t_bool worst_case_untraced_load, worst_case_untraced_store;

t_tristate loaded_registers[ARM_REG_R15+1];

t_bool ArmFltLoadStoreIsSupported(t_arm_ins * i)
{
	if (ARM_INS_TYPE(i)==IT_FLT_LOAD && ARM_INS_OPCODE(i)==ARM_VLDR)
		return TRUE;
	if (ARM_INS_TYPE(i)==IT_FLT_STORE && ARM_INS_OPCODE(i)==ARM_VSTR)
		return TRUE;

	//VERBOSE(1, ("JENS - unsupported IT_FLT_[STORE|LOAD] instruction in load/store forwarding: @I\n", i));
	return FALSE;
}

t_bool ArmIsSupportedFltStore(t_arm_ins * i)
{
	if (ARM_INS_TYPE(i)==IT_FLT_STORE && ARM_INS_OPCODE(i)==ARM_VSTR)
		return TRUE;

	//VERBOSE(1, ("JENS - unsupported IT_FLT_STORE instruction in load/store forwarding: @I\n", i));
	return FALSE;
}

t_bool ArmIsSupportedFltLoad(t_arm_ins * i)
{
	if (ARM_INS_TYPE(i)==IT_FLT_LOAD && ARM_INS_OPCODE(i)==ARM_VLDR)
		return TRUE;

	//VERBOSE(1, ("JENS - unsupported IT_FLT_LOAD instruction in load/store forwarding: @I\n", i));
	return FALSE;
}

void AddLiveRegisterBetween(t_bbl * from, t_bbl * to, t_reg reg, int lineno)/*{{{*/
{
  t_cfg_edge * edge;
  t_uint32 npreds = 0;
  if (from==to) return;

  BBL_FOREACH_PRED_EDGE(from,edge)
  {
    if(npreds) break;
    npreds++;
  }
  
  if ((npreds > 1))// && !BblDominates(to,from))
  {
    CfgDrawFunctionGraphs(FUNCTION_CFG(BBL_FUNCTION(from)),"./dots-troubles");
    FATAL(("must update impossible path at line %d",lineno));
  }

  BBL_FOREACH_PRED_EDGE(from,edge)
    {
      t_bbl * head = CFG_EDGE_HEAD(edge);
      BBL_SET_REGS_LIVE_OUT(head, RegsetAddReg(BBL_REGS_LIVE_OUT(head),reg));
      AddLiveRegisterBetween(head,to,reg, lineno);
    }
  
  return;
}/*}}}*/


t_uint32 load_count =0, store_count=0, load_tag_count=0, store_tag_count=0;
t_uint32 killed_store=0, changed_load=0, added_moves=0, added_adds=0, changed_loadmultiple=0, changed_storemultiple=0, killed_storemultiple=0, killed_loadmultiple=0, regs_to_load=0;

void ArmUseLSForwarding(t_arm_ins * j_ins, t_arm_ins * i_ins, t_partial_value * propregs, t_bool untraced_load, t_bool untraced_store, t_bool * instruction_is_killed);
/*!
 * Given a load or store instruction, try to find out at which offset from the
 * base register the memory is accessed 
 *
 * \param ins
 * \param offset
 *
 * \return t_lattice_level 
*/
/* ArmComputeLSOffset {{{ */
/* IMPORTANT TODO
 * At the moment, we are not multithreaded, so memory barriers should not be an issue.
 * However, when we want to make Diablo multithreaded, care should be taken when doing
 * load/store forwarding across a memory barrier!
 */
t_lattice_level ArmComputeLSOffset(t_arm_ins * g_ins, t_int32* offset)
{
  t_arm_ins * ins = (t_arm_ins*) g_ins;
  t_uint32 shiftimmediate;
  t_register_content tmp_value_to_shift;
  t_register_content tmp_shiftreg_value;
  t_bool carry_set=FALSE;
  t_bool tmp_flag;
  t_uint32 value_to_shift;
  t_uint32 shiftreg_value;
  
  *offset = 0;

  if (ArmInsHasShiftedFlexible(ins))
    /* Try to find out what is the offset to shift */
  {
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
    if (RegStateGetRegValue(T_INS(ins),&ARM_INS_REGS_USE(ins),ARM_INS_REGC(ins),&tmp_value_to_shift))
    {
      return CP_BOT;
    }
#else
    return CP_BOT;
#endif
    if (ARM_INS_REGS(ins) != ARM_REG_NONE)
    {
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
      if (RegStateGetRegValue(T_INS(ins),&ARM_INS_REGS_USE(ins),ARM_INS_REGS(ins),&tmp_shiftreg_value))
      {
	return CP_BOT;
      }
#else
      return CP_BOT;
#endif
      shiftreg_value = G_T_UINT32(tmp_shiftreg_value.i);
    }

    value_to_shift = G_T_UINT32(tmp_value_to_shift.i);
    shiftimmediate = ARM_INS_SHIFTLENGTH(ins);

    /* apply the shifting */
    switch (ARM_INS_SHIFTTYPE(ins)) 
    {
      case ARM_SHIFT_TYPE_LSL_IMM:
	if (shiftimmediate > 0) 
	  if (Uint32GetBit(value_to_shift, (32-shiftimmediate))) 
	    carry_set=TRUE;
	*offset = value_to_shift << shiftimmediate;
	break;
      case ARM_SHIFT_TYPE_LSR_IMM:
	*offset = value_to_shift >> shiftimmediate;
	break;
      case ARM_SHIFT_TYPE_ASR_IMM:
	if ((t_int32) value_to_shift < 0)
	  *offset = (value_to_shift >> shiftimmediate) | ~(0xFFFFFFFF >> shiftimmediate);
	else 
	  *offset = value_to_shift >> shiftimmediate;
	break;
      case ARM_SHIFT_TYPE_ROR_IMM:
	if (shiftimmediate == 0)
	{
	  *offset = value_to_shift;
	  break;
	}
	*offset = (value_to_shift >> shiftimmediate) | (value_to_shift << (32 - shiftimmediate));
	break;
      case ARM_SHIFT_TYPE_LSL_REG:

	if (ARM_SHIFT_AMOUNT(shiftreg_value) == 0) /* no carry can be shifted out here */
	  *offset = value_to_shift;
	else if (ARM_SHIFT_AMOUNT(shiftreg_value) > 32) /* no carry can be shifted out here */
	  *offset = 0;
	else
	  *offset = value_to_shift << ARM_SHIFT_AMOUNT(shiftreg_value);
	break;
      case ARM_SHIFT_TYPE_LSR_REG: /* no carry can be shifted out here */
	*offset = (value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value));
	break;
      case ARM_SHIFT_TYPE_ASR_REG: /* no carry can be shifted out here */
	if ((t_int32) value_to_shift < 0)
	  *offset = (value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value)) | ~(0xFFFFFFFF >> ARM_SHIFT_AMOUNT(shiftreg_value));
	else *offset = value_to_shift >> ARM_SHIFT_AMOUNT(shiftreg_value);
	break;
      case ARM_SHIFT_TYPE_ROR_REG:
	if (ARM_ROTATE_AMOUNT(shiftreg_value) == 0)
	{
	  *offset = value_to_shift;
	  break;
	}
	*offset = (value_to_shift >> ARM_ROTATE_AMOUNT(shiftreg_value)) | (value_to_shift << (32 - ARM_ROTATE_AMOUNT(shiftreg_value)))  /* CHECK THIS */;
	break;
      case ARM_SHIFT_TYPE_RRX:
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
	if (RegStateGetRegCond(T_INS(ins),&ARM_INS_REGS_USE(ins),ARM_REG_C_CONDITION,&tmp_flag))
	{
	  return CP_BOT;
	}
#else
	return CP_BOT;
#endif
	*offset = tmp_flag ? ((LL(0x1) << 31) & (value_to_shift >> 1)) : (value_to_shift >> 1);
	break;
      default: 
	FATAL(("Unknown shifttype while calculating offset!!"));
	break;
    }
  } 
  else if (ArmInsHasImmediate(ins) && ARM_INS_TYPE(ins) != IT_STORE_MULTIPLE && ARM_INS_TYPE(ins) != IT_LOAD_MULTIPLE)
  {
    *offset = ARM_INS_IMMEDIATE(ins);
  }
  else if (ARM_INS_REGC(ins) != ARM_REG_NONE)
  {
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
    if (RegStateGetRegValue(T_INS(ins),&ARM_INS_REGS_USE(ins),ARM_INS_REGC(ins),&tmp_value_to_shift))
    {
      return CP_BOT;
    }
#else
    return CP_BOT;
#endif
    *offset = G_T_UINT32(tmp_value_to_shift.i);
  }
  else if (ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE || ARM_INS_TYPE(ins) == IT_LOAD_MULTIPLE)
  {

        if((ARM_INS_OPCODE(ins) == ARM_VPOP || ARM_INS_OPCODE(ins) == ARM_VPUSH || ARM_INS_OPCODE(ins) == ARM_VSTM || ARM_INS_OPCODE(ins) == ARM_VLDM))
        {
        	/* The register list for these instructions is stored in the MULTIPLE member-field of the instruction class */
			t_regset tmp_set = ARM_INS_MULTIPLE(ins);
        	*offset = RegsetCountRegs(tmp_set)*4;

        	/* The registers can be either 32- or 64-bit. Modify the offset if appropriate. */
        	if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
			  *offset *= 2;
        }
        else
        {
		t_regset tmp_set = RegsetNewFromUint32(ARM_INS_IMMEDIATE(ins));
		*offset = RegsetCountRegs(tmp_set)*4;
        }
  }
   
  if (!(ARM_INS_FLAGS(ins) & FL_DIRUP)) *offset = -(*offset);

  return CP_VALUE;
  
}
/* }}} */

/*! 
 *
 * Checks if i_ins accesses the same memory location as all instructions after
 * j_ins in the same bbl. If a pair is found, the function  calls
 * ArmUseLSForwarding, to make use of the results if possible. The base
 * register used in storing/loading is load_reg. If there is a intermediate
 * memory access, that could not be disambiguated, untraced_load is set to
 * TRUE. If intermediate calculations are done on the base address that can be
 * evaluated, it's results are kept in propregs. 
 *
 * \param j_ins Instruction we are currently forwarding TO
 * \param load_reg The register used a base register in the instruction we are forwarding FROM
 * \param propregs Datastructure to keep track of changes to other registers while forwarding
 * \param i_ins The instruction we are currently forwarding FROM
 * \param untraced_load A bool, indicates when an untraced load has happened
 * \param untraced_store A bool, indicates when an untraced store has appeared
 *
 * \return int 
*/

int ArmLoadStoreFwdInner2(t_cfg * cfg, t_arm_ins * j_ins, t_reg load_reg, t_partial_value * propregs, t_arm_ins * i_ins, t_bool * untraced_load, t_bool * untraced_store, t_bool * instruction_is_killed)/*{{{*/
{
  t_bool found = FALSE; /* A bool to indicate if we have found an instruction that load/stores to the same memory location */
  t_int32 addend = 0; /* Used in calculating the offset in some addressing modes */
  t_equation* equations = EquationsNew(cfg);
#ifdef EVEN_MORE_UNSAFE
  t_reloc * load_tag = NULL, * current_tag = NULL; /* tags used to see if intervening loads definitely overlap or not */
  t_lattice_level current_tag_level;
#endif
  t_lattice_level load_tag_level; 
  t_reg tmpreg;
  t_ins * last_ins_propagated_equations = NULL;
  t_arm_opcode i_opcode = ARM_INS_OPCODE(i_ins);

  //  if (i_ins && j_ins)
  //    VERBOSE(0,("starting for @I @I",i_ins,j_ins));

#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
  load_tag_level = RegStateGetRegTag(T_INS(i_ins),&ARM_INS_REGS_USE(i_ins),ARM_INS_REGB(i_ins),&load_tag);
#else
  load_tag_level = CP_BOT;
#endif
  while(j_ins != NULL) /* while there are no more instructions in this basic block */
  {
    /* When this instruction conditionally defines the base register, we just can't keep track of things anymore */
    t_arm_opcode j_opcode = ARM_INS_OPCODE(j_ins);

    if ((ARM_INS_IS_CONDITIONAL(j_ins) && RegsetIn(ARM_INS_REGS_DEF(j_ins),load_reg)) || ArmInsIsExclusive(j_ins) ||
          j_opcode==ARM_DMB || j_opcode==ARM_DSB || j_opcode==ARM_ISB || 
          j_opcode==ARM_SWP || j_opcode==ARM_SWPB ||
          j_opcode==ARM_WFI || j_opcode==ARM_DBG)
    {
      Free(equations);
      return 0;
    }
	
    /* If this instruction is no load or store, we can just skip to the next, except when the base register gets
     * defined in this instruction. We will then try to partially evaluate the instruction {{{*/
    if (ARM_INS_TYPE(j_ins) != IT_LOAD && ARM_INS_TYPE(j_ins) != IT_STORE && ARM_INS_TYPE(j_ins) != IT_STORE_MULTIPLE && ARM_INS_TYPE(j_ins) != IT_LOAD_MULTIPLE && !ArmFltLoadStoreIsSupported(j_ins))
    {
      /* Is the base register defined? */
      if (RegsetIn(ARM_INS_REGS_DEF(j_ins),load_reg))
      {
        t_register_content tmp_offset;
        t_reg i_reg;
        tmp_offset.i = AddressNullForIns(T_INS(j_ins));
        
        /* Try to track changes */
        if (ArmPartialEvaluation(j_ins,&tmp_offset,load_reg))
        {
          Free(equations);
          return 0;
        }
        
        //printf("added %d\n",tmp_offset.i);
        
        /* Add the offset to every register in propregs */
        if (!AddressIsNull(tmp_offset.i))
          REGSET_FOREACH_REG(propregs->registers,i_reg) 
            propregs->offset[i_reg] += *(t_int32*)(&G_T_UINT32(tmp_offset.i));
      }
    }
    /*}}}*/

    /* This instruction is a load or a store, see if it uses the same base register or an alias of it.
     */
    if(ARM_INS_TYPE(j_ins) == IT_LOAD || ARM_INS_TYPE(j_ins) == IT_STORE || ArmFltLoadStoreIsSupported(j_ins))
    {
      if(ARM_INS_REGB(j_ins) != load_reg)
      /*{{{*/
      {
        t_int32 distance;
        t_tristate alias = PERHAPS;
        
        if(diabloanopt_options.copy_analysis)
          {
            if (last_ins_propagated_equations)
              BblCopyAnalysisFromInsToUntilIns(last_ins_propagated_equations,T_INS(j_ins),equations);
            else
              BblCopyAnalysisUntilIns(T_INS(j_ins),equations);
            last_ins_propagated_equations = T_INS(j_ins);
          }
        
        if(!ArmComputeLSOffset(j_ins,&distance))
        {
          t_reg reg = ARM_INS_REGA(i_ins);

          /* ArmComputeLSOffset computes the offset compared to the value in the base register
             AFTER the instruction is executed. But if it was a POSTINDEX instruction, the
             offset at which the ld/st actually took place was zero */

          if (!(ARM_INS_FLAGS(j_ins) & FL_PREINDEX))// && !(ARM_INS_FLAGS(j_ins) & FL_WRITEBACK))
            distance = 0;

          distance += propregs->offset[reg];

#ifdef DEBUG_LS_FWD
          VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Found distance, it is: %d\n",distance));
#endif
          if(diabloanopt_options.copy_analysis)
          {
#ifdef DEBUG_LS_FWD
            VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("checking aliasing of @I and @I, reg1 = %d reg2 = %d\n",i_ins,j_ins,load_reg,ARM_INS_REGB(j_ins)));
#endif
            alias = EquationsRegsEqual(cfg,equations,load_reg,ARM_INS_REGB(j_ins),-distance,NULL,NULL);
          }
        }

        if(alias == YES)
        {
          found = TRUE;
          /*VERBOSE(0,("We can use copy info here @I @I!(Alias = TRUE)",i_ins,j_ins));*/
          break;
          /* We have certainly found a pair */
        }
        else if (alias == NO)
        {
          /* We certainly did not find a pair */
          /*VERBOSE(0,("We can use copy info here @I @I!(Alias = FALSE)",i_ins,j_ins));*/
          if (ARM_INS_REGA(j_ins) == load_reg && (ARM_INS_TYPE(j_ins) == IT_LOAD || ARM_INS_TYPE(j_ins) == IT_FLT_LOAD)) 
          {
            Free(equations);
            return 0;
          }
        }
        else
        /* It is unknown where this instruction accesses memory */
        {
          /* If the load_reg gets overwritten, then we cannot continue anymore */
          if (RegsetIn(ARM_INS_REGS_DEF(j_ins),load_reg))
          {
            Free(equations);
            return 0;
          }
          
          /* If the first ins (i_ins) was a store, we want to know if there is a load
          * from memory which we couldn't trace, because if this is so, the store should
          * never be removed */
#ifndef EVEN_MORE_UNSAFE
          if(ARM_INS_TYPE(j_ins) == IT_LOAD || ARM_INS_TYPE(j_ins)==IT_FLT_LOAD) *untraced_load = TRUE;
          else *untraced_store = TRUE;
#else
          if ((ARM_INS_TYPE(j_ins) == IT_LOAD || ARM_INS_TYPE(j_ins)==IT_FLT_LOAD) && (ARM_INS_TYPE(i_ins) == IT_STORE || ArmIsSupportedFltStore(i_ins)))
          {
            if(load_reg != ARM_REG_R13 && ARM_INS_REGB(j_ins) != ARM_REG_R13)
            {
              if (load_tag_level)
                *untraced_load = TRUE;
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
              else if ((current_tag_level = RegStateGetRegTag(T_INS(j_ins),&ARM_INS_REGS_USE(j_ins),ARM_INS_REGB(j_ins),&current_tag)))
#else 
              else if ((current_tag_level = CP_BOT))
#endif
                *untraced_load = TRUE;
              else if (REL_TO_RELOCATABLE(load_tag) == REL_TO_RELOCATABLE(current_tag))
                *untraced_load = TRUE;
            }
          }
          
          /* If the first ins (i_ins) was a load, we want to know if there is a store
          * to memory that cannot be traced. If this is so, we should never remove a load
          * following the first load */
          if (ARM_INS_TYPE(j_ins) == IT_STORE || ARM_INS_TYPE(j_ins)==IT_FLT_STORE)
          {
            if(load_reg != ARM_REG_R13 && ARM_INS_REGB(j_ins) != ARM_REG_R13)
            {
              if (load_tag_level)
                *untraced_store = TRUE;
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
              else if ((current_tag_level = RegStateGetRegTag(T_INS(j_ins),&ARM_INS_REGS_USE(j_ins),ARM_INS_REGB(j_ins),&current_tag)))
#else 
              else if ((current_tag_level = CP_BOT))
#endif
                *untraced_store = TRUE;
              else if (REL_TO_RELOCATABLE(load_tag) == REL_TO_RELOCATABLE(current_tag))
                *untraced_store = TRUE;
            }
          }
#endif
        }
      }/*}}}*/
      else /* ARM_INS_REGB(j_ins) == load_reg */
      /*{{{*/
      {
        /* try to find out at which offset from the load_reg the memory is accessed,
        * return if the offset is not computable */
        if (!ArmComputeLSOffset(j_ins,&addend)) 
        {
          if (addend != 0 && (ARM_INS_FLAGS(j_ins) & FL_PREINDEX))
          {
            REGSET_FOREACH_REG(propregs->registers,tmpreg) 
              propregs->offset[tmpreg] += addend;
          }

          REGSET_FOREACH_REG(propregs->registers,tmpreg)
          {
            if (propregs->offset[tmpreg] == 0) /* then we're accessing the same memory location */
            {
              if (found == FALSE) found = TRUE;
              else
                /* FATAL because we're not dealing with multiple loads or stores */
                FATAL(("Strange load or store\n1)@I\n2)@I\n",i_ins,j_ins));
              propregs->corresponding[tmpreg] = ARM_INS_REGA(j_ins);
            }
            else if (
                     (propregs->offset[tmpreg] <= 3 && propregs->offset[tmpreg] >= -3 && (i_opcode == ARM_LDR || i_opcode == ARM_STR || j_opcode == ARM_LDR || j_opcode == ARM_STR))
                     || (propregs->offset[tmpreg] <= 7 && propregs->offset[tmpreg] >= -7 && (i_opcode == ARM_LDRD || i_opcode == ARM_STRD || j_opcode == ARM_LDRD || j_opcode == ARM_STRD))
                     || (propregs->offset[tmpreg] <= 7 && propregs->offset[tmpreg] >= -7 && (i_opcode==ARM_VLDR || i_opcode==ARM_VSTR || j_opcode==ARM_VLDR || j_opcode==ARM_VSTR) && ((ARM_INS_FLAGS(i_ins)&FL_VFP_DOUBLE) || (ARM_INS_FLAGS(j_ins)&FL_VFP_DOUBLE))) )
            {
              /* we are still accessing an overlapping memory location */
              Free(equations);
              return 0;
            }
          }
          /* see if we have to restore the offset, when there was no writeback{{{ */
          if ( addend != 0 && (!(ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) && (ARM_INS_FLAGS(j_ins) & FL_PREINDEX))) 
          {
            /*if(ARM_INS_FLAGS(j_ins) & FL_DIRUP)
            {*/
            REGSET_FOREACH_REG(propregs->registers,tmpreg) propregs->offset[tmpreg] -= addend;
            /*}
            else 
            {
              REGSET_FOREACH_REG(propregs->registers,tmpreg) propregs->offset[tmpreg] += addend;
            }*/
          }
          /*}}}*/
          if (found == TRUE) break;
          if (RegsetIn(ARM_INS_REGS_DEF(j_ins),load_reg)) 
          {
            Free(equations);
            return 0; 
          }
        }
        else 
          /* Offset was not computable, we indicate that there was
          an untraced load or store */
        {
          /* There is an offset in a register and there is writeback: we cannot go on */
          if(RegsetIn(ARM_INS_REGS_DEF(j_ins),load_reg))
          {
            Free(equations);
            return 0;
          }
          
          /* There is no writeback */
          if(ARM_INS_TYPE(j_ins) == IT_LOAD || ARM_INS_TYPE(j_ins)==IT_FLT_LOAD) *untraced_load = TRUE;
          else *untraced_store = TRUE;
        }
      }
      /*}}}*/
    }
    
    if(ARM_INS_TYPE(j_ins) == IT_LOAD_MULTIPLE || ARM_INS_TYPE(j_ins) == IT_STORE_MULTIPLE)
    {
      t_regset tmp_set = RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins));
      /* register size in bytes */
      int reg_size = 4;

	if (j_opcode==ARM_VPUSH || j_opcode==ARM_VPOP || j_opcode==ARM_VLDM || j_opcode==ARM_VSTM)
	{
		/* for these instructions, the register set is defined in the MULTIPLE member-field of the instruction class */
		tmp_set = ARM_INS_MULTIPLE(j_ins);

		/* the registers can be either 32 or 64-bit */
		if (ARM_INS_FLAGS(j_ins) & FL_VFP_DOUBLE)
		{
			reg_size = 8;
		}
	}

      /* If load_reg is loaded, we cannot continue anymore */
      if(RegsetIn(tmp_set,load_reg) && ARM_INS_TYPE(j_ins) == IT_LOAD_MULTIPLE) 
      {
        Free(equations);
        return 0;
      }

      if(ARM_INS_REGB(j_ins) != load_reg)
      {
        t_int32 difference;
        t_tristate known = PERHAPS;

        if(diabloanopt_options.copy_analysis)
        {
          if (last_ins_propagated_equations)
            BblCopyAnalysisFromInsToUntilIns(last_ins_propagated_equations,T_INS(j_ins),equations);
          else
            BblCopyAnalysisUntilIns(T_INS(j_ins),equations);
          last_ins_propagated_equations = T_INS(j_ins);
            /*VERBOSE(0,("CA until @I\n",j_ins));*/
          /*VERBOSE(0,("Checking equations for @I and @I\n",i_ins,j_ins));*/
          known = EquationsRegsDiffer(cfg,equations,load_reg,ARM_INS_REGB(j_ins),&difference);
        }
        
        if(known == YES)
        {
          t_uint16 i = 0;
          t_int16 i_reg;
          t_reg j_reg;

#ifdef DEBUG_LS_FWD
          VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("We know a lot %d!\n",difference));
#endif
          if(ARM_INS_FLAGS(j_ins) & FL_DIRUP)
          {
            if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i++;
            REGSET_FOREACH_REG(tmp_set,i_reg)
            {
              REGSET_FOREACH_REG(propregs->registers,j_reg)
              {
              	/* We should not have to worry about different sizes of registers being handled,
              	 * because the manual does not define an instruction that supports storing or loading
              	 * of mixed-size registers.
              	 */
                if ((propregs->offset[j_reg] + (i*reg_size) - difference) == 0)
                {
                  propregs->corresponding[j_reg] = i_reg;
                  found = TRUE;
                  /*printf("Correspondig %d, %d",j_reg,i_reg);*/
                }
              }
              i++;
            }
            
            if (found == TRUE)
            {
              /*VERBOSE(0,("1:And we use it! (%d) @I, @I\n",difference,i_ins,j_ins));*/
              break;
            }
          }
          else /* !DIRUP */
          {
            if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i++;

            /* Here we traverse the register list in an inverse direction. Why? See below.
             *
             * UP
             * [_|_|_|_|_|base|a|b|c|d|_]
             *              --------->
             * DOWN
             * [_|a|b|c|d|base|_|_|_|_|_]
             *    <---------
             *
             * 'i' is incremented on each register we encounter, so we have to start from the
             * base register, going down. This means we have to start at the last register
             * in the list.
             */
            REGSET_FOREACH_REG_INVERSE(tmp_set,i_reg)
            {
              REGSET_FOREACH_REG(propregs->registers,j_reg)
              {
              	/* we should not worry about mixed-size registers in the lists (reason, see higher) */
                if (propregs->offset[j_reg] - (i*reg_size) - difference == 0)
                {
                  propregs->corresponding[j_reg] = i_reg;
                  found = TRUE;
                  /*printf("Corresponding2 %d, %d",j_reg,i_reg);*/
                }
              }
              i++;
            }
            
            if (found == TRUE) 
            {
              /*VERBOSE(0,("2:And we use it! (%d) @I, @I\n",difference,i_ins,j_ins));*/
              break;
            }
          }
          /*VERBOSE(0,("Skipping a multiple because of disambiguation (1)!\n"));*/
        }
        else if (known == NO)
          /* The tags are different so we will not have aliases. */
        {
          /*VERBOSE(0,("Skipping a multiple because of disambiguation (2)!\n"));*/
        }
        else /* known == PERHAPS */
        {
#ifndef EVEN_MORE_UNSAFE
          if(ARM_INS_TYPE(j_ins) == IT_LOAD_MULTIPLE) *untraced_load = TRUE;
          else *untraced_store = TRUE;
#endif
        }
      }
      else
      /* In this case, the to-be-checked instruction is a LOAD_MULTIPLE or STORE_MULTIPLE instruction.
       * Additianally, the base register of this instruction is equal to the base register of the original instruction,
       * so we have to consider the fact that the base register can be modified by the to-be-checked instruction.
       *
       * ARM_INS_REGB(j_ins) == load_reg
       */
      {
        t_reg j_reg;
        t_bool wb = ARM_INS_FLAGS(j_ins) & FL_WRITEBACK;
        t_uint16 i = 0;
        t_int16 i_reg;

        if (ARM_INS_FLAGS(j_ins) & FL_DIRUP)
        {
          if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i++;
          REGSET_FOREACH_REG(tmp_set,i_reg)
          {
            REGSET_FOREACH_REG(propregs->registers,j_reg)
            {
		/* Again, we should not worry about mixed-size register lists (see higher) */
		if (propregs->offset[j_reg] + (i*reg_size) == 0)
		{
			propregs->corresponding[j_reg] = i_reg;
			found = TRUE;
		}
            }
            i++;
          }
          if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i--;

          /* Modify the propagated registers if appropriate */
          if(wb)
          	REGSET_FOREACH_REG(propregs->registers,j_reg)
          		propregs->offset[j_reg] += i*reg_size;

          if (found == TRUE) break;
        }
        else
        /* We are going down in memory (!DIRUP); same comments as higher */
        {
          if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i++;
          REGSET_FOREACH_REG_INVERSE(tmp_set,i_reg)
          {
            REGSET_FOREACH_REG(propregs->registers,j_reg)
            {
              if (propregs->offset[j_reg] - (i*reg_size) == 0)
              {
                propregs->corresponding[j_reg] = i_reg;
                found = TRUE;
              }
            }
            i++;
          }
          if (ARM_INS_FLAGS(j_ins) & FL_PREINDEX) i--;

          /* Consider a possible writeback */
          if(wb)
          	REGSET_FOREACH_REG(propregs->registers,j_reg)
          		propregs->offset[j_reg] -= i*reg_size;

          if (found == TRUE) break;
        }
      }
    }
    
    /* See if the stored or loaded register(s) get changed in between the two memory operations,
     * if so try to keep track of these changes. With this info, we could avoid a load by undoing the change {{{*/
    /* TODO : rewrite this piece, it's not doing what it should, especially with multiple load/store! */
    if (!RegsetIsMutualExclusive(ARM_INS_REGS_DEF(j_ins),propregs->registers))
    {
      t_reg i_reg;
      t_regset tmpregset;
      t_register_content tmp_offset;
      tmp_offset.i = AddressNullForIns(T_INS(j_ins));

      tmpregset = RegsetIntersect(ARM_INS_REGS_DEF(j_ins),propregs->registers);

      REGSET_FOREACH_REG(tmpregset,i_reg)
      {
        /* If there is still a value propagated along, we can continue partial evaluation */
        if(!propregs->level[i_reg])
          propregs->level[i_reg] = ArmPartialEvaluation(j_ins,&tmp_offset,i_reg);
        if(!propregs->level[i_reg]) 
          propregs->value[i_reg] += G_T_UINT32(tmp_offset.i);
        propregs->changed[i_reg] = TRUE;
      }

      /*if(RegsetIn(propregs->registers,ARM_INS_REGA(i_ins)))
      {
        if(!propregs->level[ARM_INS_REGA(i_ins)])
          propregs->level[ARM_INS_REGA(i_ins)] = ArmPartialEvaluation(j_ins,&tmp_offset);
        if (!propregs->level[ARM_INS_REGA(i_ins)])
        {
          propregs->value[ARM_INS_REGA(i_ins)] += G_T_UINT32(tmp_offset.i);
        }
      }
      REGSET_FOREACH_REG(ARM_INS_REGS_DEF(j_ins),i_reg) propregs->changed[i_reg] = TRUE;*/
    }/*}}}*/

    j_ins = ARM_INS_INEXT(j_ins);
  }

  /* Didn't find a pair, go on with the next basic block */
  if (!found)
  {
    Free(equations);
    return 1;
  }
  
  /* We did find a pair, let's try to use it */
  ArmUseLSForwarding(j_ins,i_ins,propregs,*untraced_load,*untraced_store,instruction_is_killed);

  Free(equations);
  return 0;
}/*}}}*/

/*!
 * \todo Try to make use of the pairs of loads/stores that are found to be accessing the same memory location.
 * Whether this is possible depends on things such as intervening untraced load or stores, changes to the register
 * that was originally stored or loaded. This info is all contained in propregs, untraced_load and untraced_store.
 *
 * \param j_ins Instruction that is forwarded TO
 * \param i_ins Instruction that is forwarded FROM
 * \param propregs Info about registers that were originally stored or loaded (ie. in i_ins)
 * \param untraced_load Bool that indicates if an untraced load has happened
 * \param untraced_store Bool that indicates if an untraced store has happened
 *
 * \return void 
*/
/* ArmUseLSForwarding {{{*/
void ArmUseLSForwarding(t_arm_ins * j_ins, t_arm_ins * i_ins, t_partial_value * propregs, t_bool untraced_load, t_bool untraced_store, t_bool* instruction_is_killed)
{
  static int teller = 0;
  t_arm_ins * insert_arm_ins;
  t_bool up = FALSE;

#ifdef DEBUG_LS_FWD
  if (diablosupport_options.debugcounter < teller) return;
  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Case: %d",teller));
  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Investigating pair @I and @I",i_ins,j_ins));
#endif /* DEBUG_LS_FWD */


  if (!untraced_load && ARM_INS_OPCODE(i_ins) == ARM_STR 
      && ARM_INS_OPCODE(j_ins) == ARM_STR && !ARM_INS_IS_CONDITIONAL(j_ins) 
       && BblPostdominates(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins)))
  /* First case: two stores following each other, with no untraced load in between: kill the first
   * store, since it is useless. However check if the first store has a writeback! */
  /*{{{*/
  {
#ifdef DEBUG_LS_FWD
    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I before @I\n",i_ins,j_ins));
    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Store killed or changed to dataproc!\n"));
#endif /* DEBUG_LS_FWD */
    /* we can safely adjust the first store */
    if ((ARM_INS_FLAGS(i_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
    {
      if (ARM_INS_FLAGS(i_ins) & FL_DIRUP) up = TRUE;
      if (up) ArmInsMakeAdd(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(i_ins));
      else ArmInsMakeSub(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(i_ins));
      ASSERT(ArmInsIsEncodable(i_ins), ("instruction @I not encodable!", i_ins));

      ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
      ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
      killed_store++;regs_to_load++;
    }
    else
    {
      ArmInsKill(i_ins);
      *instruction_is_killed = TRUE;
      killed_store++;regs_to_load++;
    }
    teller++;
    return;
  }/*}}}*/
  else if (!untraced_load && ARM_INS_OPCODE(i_ins) == ARM_STRB
      && ARM_INS_OPCODE(j_ins) == ARM_STRB && !ARM_INS_IS_CONDITIONAL(j_ins)
      && BblPostdominates(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins)))
 /*{{{*/
  {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I before @I\n",i_ins,j_ins));
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Store killed or changed to dataproc!\n"));
#endif /* DEBUG_LS_FWD */
    /* we can safely adjust the first store */
    if ((ARM_INS_FLAGS(i_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
    {
      if (ARM_INS_FLAGS(i_ins) & FL_DIRUP) up = TRUE;
      if (up) ArmInsMakeAdd(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,1,ARM_INS_CONDITION(i_ins));
      else ArmInsMakeSub(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,1,ARM_INS_CONDITION(i_ins));
      ASSERT(ArmInsIsEncodable(i_ins), ("instruction @I not encodable!", i_ins));

      ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
      ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
      killed_store++;regs_to_load++;
    }
    else
    {
      ArmInsKill(i_ins);
      *instruction_is_killed = TRUE;
      killed_store++;regs_to_load++;
    }
    teller++;
    return;
  }/*}}}*/
  else if ( (ARM_INS_OPCODE(i_ins) == ARM_STRB || ARM_INS_OPCODE(i_ins) == ARM_LDRB)
      && ARM_INS_OPCODE(j_ins) == ARM_LDRB && !untraced_store)
  /*{{{*/
  {
    if(propregs->offset[ARM_INS_REGA(i_ins)]!=0) goto unused;
    if(!propregs->changed[ARM_INS_REGA(i_ins)])
    {
      if ((ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(j_ins) & FL_PREINDEX))
      {
	insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
  ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	ArmInsInsertAfter(insert_arm_ins,j_ins);

	if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	if (up) ArmInsMakeAdd(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	else  ArmInsMakeSub(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	added_adds++;
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Added instruction: @I and",insert_arm_ins));
#endif
      }
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed @I to",j_ins));
#endif
      AddLiveRegisterBetween(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins),ARM_INS_REGA(i_ins),__LINE__);
      ArmInsMakeAnd(j_ins,ARM_INS_REGA(j_ins),ARM_INS_REGA(i_ins),ARM_REG_NONE,0xff,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I\n",j_ins));
#endif
      if (ARM_INS_REGA(j_ins) != ARM_INS_REGC(j_ins))
      {
	ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
      }
      else
      {
	ARM_INS_SET_REGS_DEF(j_ins,  NullRegs);
	ARM_INS_SET_REGS_USE(j_ins,  NullRegs);
      }
      changed_load++;regs_to_load++;
#ifdef DEBUG_LS_FWD
      teller++;
#endif
      return;
    }
      
  }/*}}}*/

  else if (
      (ARM_INS_OPCODE(i_ins) == ARM_STR 
      || ARM_INS_OPCODE(i_ins) == ARM_LDR 
      || ARM_INS_OPCODE(i_ins) == ARM_STM
      || ARM_INS_OPCODE(i_ins) == ARM_LDM
      )
      && ARM_INS_OPCODE(j_ins) == ARM_LDR 
      && !untraced_store)
  /* Second case: a load or store followed by a load, with no untraced store in between :
   * get rid of the second load and put a move instead. If necessary (writeback...) insert an extra
   * instruction to adjust the load_reg */
  /*{{{*/
  {
    t_reg tmpreg;

    if(ARM_INS_OPCODE(i_ins) == ARM_STR || ARM_INS_OPCODE(i_ins) == ARM_LDR)
      tmpreg = ARM_INS_REGA(i_ins);
    else
      /* i_ins is a load/store multiple */
      REGSET_FOREACH_REG(propregs->registers,tmpreg)
	if(propregs->corresponding[tmpreg] == ARM_INS_REGA(j_ins))
	  break;
    /*    VERBOSE(0,("@I, @I, changed? %d",i_ins,j_ins,propregs->changed[tmpreg]));*/
    if(tmpreg >= MAX_REG_ITERATOR)
    {
      VERBOSE(1,("Why do we get here then? @I , @I",i_ins,j_ins));
      return;
    }

    if(!propregs->changed[tmpreg])
      /* change the load in a copy instruction if possible */
    {
      if ((ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(j_ins) & FL_PREINDEX))
      {

	insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
  ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	ArmInsInsertBefore(insert_arm_ins,j_ins);

	if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	if (up) ArmInsMakeAdd(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	else  ArmInsMakeSub(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	added_adds++;
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Added instruction: @I and",insert_arm_ins));
#endif
      }
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed @I to",j_ins));
#endif
      /*      VERBOSE(0,("Updating between @iB and @iB\n",ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins)));*/
      AddLiveRegisterBetween(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins),tmpreg,__LINE__);
      ArmInsMakeMov(j_ins,ARM_INS_REGA(j_ins),tmpreg,0,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I\n",j_ins));
#endif
      if (ARM_INS_REGA(j_ins) != ARM_INS_REGC(j_ins))
      {
	ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
      }
      else
      {
	ARM_INS_SET_REGS_DEF(j_ins,  NullRegs);
	ARM_INS_SET_REGS_USE(j_ins,  NullRegs);
      }
      changed_load++;regs_to_load++;
#ifdef DEBUG_LS_FWD
      teller++;
#endif
      return;
    }
    else
      /* look for a dead register, or try to regenerate the orig value by a calculation */
    {
      t_regset tmp_set = ARM_ALL_BUT_PC_AND_COND;
      t_arm_ins * tmp_ins = i_ins;
      t_int16 i_reg;
      t_cfg_edge * edge;
      t_uint32 realpreds = 0;

      if ((ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(j_ins) & FL_PREINDEX))
      {

	/* since it is not likely to be profitable, we bail out here (BJORN)
	   remove the return to try safely otherwise 
	   */
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Unprofitable"));
	goto unused;/*return;  */

	insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
  ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	ArmInsInsertBefore(insert_arm_ins,j_ins);

	if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	if (up) ArmInsMakeAdd(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	else  ArmInsMakeSub(insert_arm_ins, ARM_INS_REGB(j_ins), ARM_INS_REGB(j_ins), ARM_REG_NONE, ARM_INS_IMMEDIATE(j_ins), ARM_INS_CONDITION(j_ins));
	ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	added_adds++;
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Added instruction: @I and",insert_arm_ins));
#endif
      }

      BBL_FOREACH_PRED_EDGE(ARM_INS_BBL(j_ins),edge)
      {
	/*	if(EDGE_CAT(edge) == ET_SWITCH) continue;*/
	if(CFG_EDGE_HEAD(edge)==ARM_INS_BBL(i_ins))
	{
	  /* this test needs to be inserted as long as liveness
	     exploits calling conventions */
	  if (diabloanopt_options.rely_on_calling_conventions)
	    if (CFG_EDGE_CAT(edge)==ET_CALL)
	    {
	      if (FUNCTION_FLAGS(BBL_FUNCTION(ARM_INS_BBL(j_ins))) & FF_IS_EXPORTED)
	      {
		RegsetSetIntersect(tmp_set,arm_description.callee_may_use);
	      }
	    }
	  continue;
	}
	if(realpreds) break;
	realpreds++;
      }

      RegsetSetDiff(tmp_set,InsRegsLiveAfter(T_INS(i_ins)));

      if(!RegsetIsEmpty(tmp_set))
      {
	if ( ARM_INS_BBL(j_ins) == ARM_INS_BBL(i_ins))
	{
	  tmp_ins = ARM_INS_INEXT(i_ins);
	  while(tmp_ins != j_ins && !RegsetIsEmpty(tmp_set))
	  {
	    RegsetSetDiff(tmp_set,ARM_INS_REGS_DEF(tmp_ins));
	    tmp_ins = ARM_INS_INEXT(tmp_ins);
	  }
	}
	else if (!realpreds)
	  /* There is only one incoming edge */
	{
	  tmp_ins =ARM_INS_INEXT(i_ins);
	  while(tmp_ins && !RegsetIsEmpty(tmp_set))
	  {
	    RegsetSetDiff(tmp_set,ARM_INS_REGS_DEF(tmp_ins));
	    tmp_ins = ARM_INS_INEXT(tmp_ins);
	  }

	  if(!tmp_ins) 
	  {
	    tmp_ins = T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(j_ins)));
	  }

	  while(tmp_ins != j_ins && !RegsetIsEmpty(tmp_set))
	  {
	    RegsetSetDiff(tmp_set,ARM_INS_REGS_DEF(tmp_ins));
	    tmp_ins = T_ARM_INS(ARM_INS_INEXT(tmp_ins));
	  }
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Crossed the basic block border!"));
#endif
	}
	else
	{
#ifdef DEBUG_LS_FWD
          //	  if(BblDominates(ARM_INS_BBL(i_ins),ARM_INS_BBL(j_ins)))
          //	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Use dominator info in LS! @I, @I",i_ins,j_ins));
#endif
	  goto unused;/*return;*/ /* \todo TODO: use dominator info here! */
	}
      }

      
      if(diabloanopt_options.copy_analysis)
        {
          /* we should not use registers used in equations, because doing so would invalidate equations on 
             which later transformations may still rely */
          t_equation* eqs = EquationsNew(INS_CFG(T_INS(j_ins)));
          int i;
          
          if (!RegsetIsEmpty(tmp_set))
            {
              BblCopyAnalysisUntilIns(T_INS(j_ins),eqs);
              for(i=0; i<=arm_description.num_int_regs; i++)
                {
                  if (EquationIsBot(eqs[i])) continue;
                  RegsetSetSubReg(tmp_set,eqs[i].regb);
                  RegsetSetSubReg(tmp_set,eqs[i].rega);
                }			 
            }
            Free(eqs);
        }

      REGSET_FOREACH_REG(tmp_set,i_reg) break;

      if(!RegsetIsEmpty(tmp_set) ) /* We have found a dead register that we can use */
      {
	BBL_SET_REGS_LIVE_OUT(ARM_INS_BBL(i_ins), RegsetAddReg(BBL_REGS_LIVE_OUT(ARM_INS_BBL(i_ins)),i_reg));
	/*if (teller++ < diablosupport_options.debugcounter)*/
	{
	  VERBOSE(2,("-----------------------------------------------------------------\n"));
	  VERBOSE(2,("going with r%d\n",i_reg));
	  VERBOSE(2,("i_ins: @I\nj_ins: @I\n",i_ins,j_ins));

	  if (i_reg != tmpreg)
	  {
	    tmp_ins = ArmInsNewForBbl(ARM_INS_BBL(i_ins));
	    ARM_INS_SET_CSIZE(tmp_ins, AddressNew32(4));
	    ARM_INS_SET_OLD_SIZE(tmp_ins, AddressNew32(4));
      ARM_INS_SET_FLAGS(tmp_ins, ARM_INS_FLAGS(tmp_ins) | (ARM_INS_FLAGS(i_ins) & FL_THUMB));

	    ArmInsMakeMov(tmp_ins,i_reg,tmpreg,0,ARM_INS_CONDITION(i_ins));
      ASSERT(ArmInsIsEncodable(tmp_ins), ("instruction @I not encodable!", tmp_ins));

	    ArmInsInsertAfter(tmp_ins,i_ins);
	    ARM_INS_SET_REGS_DEF(tmp_ins,  ArmDefinedRegisters(tmp_ins));
	    ARM_INS_SET_REGS_USE(tmp_ins,  ArmUsedRegisters(tmp_ins));
	    added_moves++;
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Added @I\n", tmp_ins));
#endif
	  }

	  if (ARM_INS_REGA(j_ins) != i_reg)
	  {
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I, Changed @I",i_ins,j_ins));
#endif
	    AddLiveRegisterBetween(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins),i_reg,__LINE__);
	    ArmInsMakeMov(j_ins,ARM_INS_REGA(j_ins),i_reg,0,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	    ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	    ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,(" to @I using a dead register",j_ins));
#endif
	    changed_load++;regs_to_load++;
	    /*VERBOSE(0,("Changed second load to @I\n", j_ins));*/
	  }
	  else
	  {
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Removed @I as it became idempotent\n", j_ins));
#endif
            if (j_ins==tmp_backup)
            {
              /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
              tmp_backup = ARM_INS_INEXT(tmp_backup);
            }
	    ArmInsKill(j_ins);
	    *instruction_is_killed = TRUE;
	    changed_load++;regs_to_load++;
	  }
	}
#ifdef DEBUG_LS_FWD
	teller++;
#endif
	return;
      }
      //      else if (!propregs->level[tmpreg] && ArmIsEncodableConstantForOpcode(propregs->value[tmpreg],ARM_ADD) && !BblDominates(ARM_INS_BBL(i_ins),ARM_INS_BBL(j_ins)))
      //        {
      //          FATAL(("need dominator!!!"));
      //        }
      else if (!propregs->level[tmpreg] && ArmIsEncodableConstantForOpcode(propregs->value[tmpreg],ARM_ADD))// && BblDominates(ARM_INS_BBL(i_ins),ARM_INS_BBL(j_ins)))
	/* We do not have e dead register available, but if we know that we can calculate
	 * the value to be loaded, by simply adding a constant
	 * to a register, we change the load to an add */
      {
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Before partial eval : @I, @I\n",i_ins,j_ins));
	if ((ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(j_ins) & FL_PREINDEX))
	{
	  insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	  ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	  ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
    ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	  ArmInsInsertAfter(insert_arm_ins,j_ins);

	  if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	  if (up) ArmInsMakeAdd(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(j_ins));
	  else  ArmInsMakeSub(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	  ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	  ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	  added_adds++;
	}

  if (/* value > 0 */ (t_signed_address)(propregs->value[tmpreg]) > 0)
  {
    /* make SUB instruction */
    ArmInsMakeSub(j_ins, ARM_INS_REGA(j_ins), tmpreg, ARM_REG_NONE, propregs->value[tmpreg], ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
  }
  else
  {
    /* make ADD instruction */
    ArmInsMakeAdd(j_ins, ARM_INS_REGA(j_ins), tmpreg, ARM_REG_NONE, propregs->value[tmpreg], ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
  }

	if (ARM_INS_IMMEDIATE(j_ins))
	{
	  ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	  ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	}
	else
	{
	  ARM_INS_SET_REGS_DEF(j_ins,  NullRegs);
	  ARM_INS_SET_REGS_USE(j_ins,  NullRegs);
	}
	changed_load++;regs_to_load++;
	VERBOSE(2,("After partial eval : @I\n",j_ins));
#ifdef DEBUG_LS_FWD
	teller++;
#endif
	return;
      }
#ifdef DEBUG_LS_FWD
      else
      {
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("No dead register found to propagate value"));
	goto unused;
      }
#endif
    }
  }/*}}}*/

  else if (ARM_INS_OPCODE(i_ins) == ARM_STM && (ARM_INS_OPCODE(j_ins) == ARM_LDM || ARM_INS_OPCODE(j_ins)== ARM_LDR) && !untraced_store)
  /* Third case: a store multiple followed by a load : depending on the fact that all registers
   * get restored(restored,nr_diff) and none is written in the mean time(nr_dirty,clean) we can kill one
   * or both instructions, or change the load into a move */
/*{{{*/
  {
    t_regset tmp_set1 = NullRegs;
    t_reg tmp_reg;
    t_bool restored = TRUE, clean = TRUE;
    t_uint16 restore_value;
    t_int8 diff = 0; /* is the number of regs in the stm and ldm/ldr the same? */
    t_uint8 nr_diff=0; /* if not restored, how many regs are (inter)changed? */
    t_uint8 nr_dirty=0; /* how many aren't clean? */ 

    /*    VERBOSE(0,("Now using LS-info :\n1)@I\n2)@I\n",i_ins,j_ins)); */

    if (ARM_INS_OPCODE(j_ins) == ARM_LDM)
      tmp_set1 = RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins));
    else
      RegsetSetAddReg(tmp_set1,ARM_INS_REGA(j_ins));
    
    diff = (RegsetCountRegs(propregs->registers) - RegsetCountRegs(tmp_set1));

    if (diff < 0) return; /* TODO: we could do something with it, but that requires some coding */

    REGSET_FOREACH_REG(propregs->registers,tmp_reg)
    {
      if (propregs->corresponding[tmp_reg] != tmp_reg/* && propregs->corresponding[tmp_reg] != MAX_REG_ITERATOR*/)
      {
	if(propregs->corresponding[tmp_reg] == MAX_REG_ITERATOR && diff == 0) return;
	restored = FALSE;
	nr_diff++;
      }
      if (propregs->changed[tmp_reg])
      {
	clean = FALSE;
	nr_dirty++;
      }
    }
    if (ARM_INS_OPCODE(j_ins) == ARM_LDR)
    {
      REGSET_FOREACH_REG(propregs->registers,tmp_reg)
	if (propregs->corresponding[tmp_reg] == ARM_INS_REGA(j_ins)) break;
      if (!propregs->changed[tmp_reg]) clean = TRUE;
      else clean = FALSE;
      restored = FALSE;
      nr_diff = 1;
    }

    if (clean && restored ) 
    {
/*The load will surely disappear, the store only if we have a stack operation{{{*/
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Clean & restored :\n1)@I\n2)@I",i_ins,j_ins));
#endif
      /* TODO: if dominator AND postdominator info is available, we can do a little more */
      /* BJORN: this then-part below makes no sense: you cannot decide to skip the store simply because of a first load
         What if there are more loads to the stored data.
         So I commented it out ... 
      */
      if (0 && !diff && ARM_INS_REGB(j_ins) == ARM_REG_R13 && ARM_INS_BBL(j_ins) == ARM_INS_BBL(i_ins))
      {
	if(!(FUNCTION_FLAGS(BBL_FUNCTION(ARM_INS_BBL(j_ins))) & FF_DONT_CHANGE_STACK)
	    || (!((ARM_INS_FLAGS(j_ins) & FL_WRITEBACK) && (ARM_INS_FLAGS(i_ins) & FL_WRITEBACK))))
	{
	/* kill em both */
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Killed @I and @I",i_ins,j_ins));
#endif
	ArmInsKill(i_ins);
	*instruction_is_killed = TRUE;
	killed_storemultiple++;
        if (j_ins==tmp_backup)
          {
            /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
              tmp_backup = ARM_INS_INEXT(tmp_backup);
          }
	ArmInsKill(j_ins);
	killed_loadmultiple++;
	regs_to_load += 2* RegsetCountRegs(propregs->registers);
	teller++;
	return;
	}
	else
	{
	  restore_value = RegsetCountRegs(tmp_set1);
	  if(ARM_INS_FLAGS(i_ins) & FL_WRITEBACK)
	  {
	    if (ARM_INS_FLAGS(i_ins) & FL_DIRUP) up = TRUE;
	    if (up) ArmInsMakeAdd(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(i_ins));
	    else ArmInsMakeSub(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(i_ins));
      ASSERT(ArmInsIsEncodable(i_ins), ("instruction @I not encodable!", i_ins));
	    ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
	    ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed to : @I!!!!!!!!",i_ins));
#endif
	    changed_loadmultiple++;
	    regs_to_load += RegsetCountRegs(propregs->registers);

	  }
	  if(ARM_INS_FLAGS(j_ins) & FL_WRITEBACK)
	  {
	    if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	    if (up) ArmInsMakeAdd(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(j_ins));
	    else ArmInsMakeSub(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	    ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	    ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed to : @I!!!!!!!!",j_ins));
#endif
	    changed_loadmultiple++;
	    regs_to_load += RegsetCountRegs(propregs->registers);

	  }

	}
      }
      else
      {
	/* change the load into a sub or add */
	restore_value = RegsetCountRegs(tmp_set1);
	if (ARM_INS_FLAGS(j_ins) & FL_WRITEBACK)
	{
	  if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	  if (up) ArmInsMakeAdd(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(j_ins));
	  else ArmInsMakeSub(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,restore_value * 4,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	  ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	  ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed to : @I!!!!!!!!",j_ins));
#endif
	  changed_loadmultiple++;
	  regs_to_load += RegsetCountRegs(propregs->registers);

	}
	else
	{
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("1:Killed @I",j_ins));
#endif
        if (j_ins==tmp_backup)
          {
            /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
            tmp_backup = ARM_INS_INEXT(tmp_backup);
          }
	  ArmInsKill(j_ins);
	  *instruction_is_killed = TRUE;
	  killed_loadmultiple++;
	  regs_to_load += RegsetCountRegs(propregs->registers);
	}
#ifdef DEBUG_LS_FWD
	teller++;
#endif
	return;
      }/*}}}*/
    }
    else if (clean && !restored)
    {
/* In this case, we need to copy a reg into another, but the load can surely disappear{{{*/
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Clean but not restored: \n1)@I\n2)@I",i_ins,j_ins));
#endif
      if (nr_diff == 1)
      {
	t_reg tmpreg = MAX_REG_ITERATOR;
	
	/*REGSET_FOREACH_REG(propregs->registers,tmpreg)
	{
	  VERBOSE(0,("%d corresponds to %d\n",tmpreg,propregs->corresponding[tmpreg]));
	}*/

	/* find out which register has not been restored */
	REGSET_FOREACH_REG(propregs->registers,tmpreg)
	{
	  if (propregs->corresponding[tmpreg] != MAX_REG_ITERATOR && ARM_INS_OPCODE(j_ins) == ARM_LDR) break;
	  if (propregs->corresponding[tmpreg] != tmpreg && propregs->corresponding[tmpreg] != MAX_REG_ITERATOR) break;
	}

	if (!diff && ARM_INS_REGB(j_ins) == ARM_REG_R13)
	{

	  if(tmpreg >= MAX_REG_ITERATOR) FATAL(("Have to break somewhere! @I, @I",i_ins, j_ins)); 
	  if(BblPostdominates(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins)))
	  {
	    printf("Killing the store!\n");
	    ArmInsKill(i_ins);
	    *instruction_is_killed = TRUE;
	    killed_storemultiple++;
	    regs_to_load += RegsetCountRegs(propregs->registers);
	  }
	  else
	  {
	    return; /* lets return for now, until postdominators works fine again */
	    if(ARM_INS_OPCODE(j_ins) == ARM_LDM && (ARM_INS_FLAGS(j_ins) & FL_WRITEBACK))
	    {
	      insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	      ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	      ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
        ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	      ArmInsInsertBefore(insert_arm_ins,j_ins);

	      if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	      if (up) ArmInsMakeAdd(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(tmp_set1),ARM_INS_CONDITION(j_ins));
	      else ArmInsMakeSub(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(tmp_set1),ARM_INS_CONDITION(j_ins));
        ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	      ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	      ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	      added_adds++;
	    }
	  }
	  /* In this case we usually have a function return, like LDMIA 13!,{r4,pc} */
	  ArmInsMakeMov(j_ins,propregs->corresponding[tmpreg],tmpreg,0,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	  ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	  ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	  changed_loadmultiple++;
	  regs_to_load += RegsetCountRegs(propregs->registers);
#ifdef DEBUG_LS_FWD
	  teller++;
#endif
	  return;
	}
	else
	{
	  if (ARM_INS_OPCODE(j_ins) == ARM_LDM && (ARM_INS_FLAGS(j_ins) & FL_WRITEBACK))
	  {
	    insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	    ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	    ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
      ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

	    ArmInsInsertBefore(insert_arm_ins,j_ins);

	    if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	    if (up) ArmInsMakeAdd(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(tmp_set1),ARM_INS_CONDITION(j_ins));
	    else ArmInsMakeSub(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(tmp_set1),ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	    added_adds++;
	  }
	  else if (ARM_INS_FLAGS(j_ins) & FL_WRITEBACK)
	  {
	    insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	    ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	    ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
      ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));

            ArmInsInsertBefore(insert_arm_ins,j_ins);

	    if (ARM_INS_FLAGS(j_ins) & FL_DIRUP) up = TRUE;
	    if (up) ArmInsMakeAdd(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 ,ARM_INS_CONDITION(j_ins));
	    else ArmInsMakeSub(insert_arm_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 ,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));

	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	    added_adds++;
#ifdef DEBUG_LS_FWD
	    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Inserted @I\n",insert_arm_ins));
#endif
	  }
	  if(tmpreg != MAX_REG_ITERATOR)
	  {


#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed @I",j_ins));
#endif
	  AddLiveRegisterBetween(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins),tmpreg,__LINE__);
	  ArmInsMakeMov(j_ins,propregs->corresponding[tmpreg],tmpreg,0,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	  if (ARM_INS_REGA(j_ins) != ARM_INS_REGC(j_ins))
	  {
	    ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	    ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	  }
	  else
	  {
	    ARM_INS_SET_REGS_DEF(j_ins,  NullRegs);
	    ARM_INS_SET_REGS_USE(j_ins,  NullRegs);
	  }
	  if (ARM_INS_OPCODE(j_ins) == ARM_LDM)
	  {
	    changed_loadmultiple++;
	    regs_to_load += RegsetCountRegs(propregs->registers);
	  }
	  else
	  {
	    changed_load++;
	    regs_to_load++;
	  }
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,(" to @I\n",j_ins));
#endif
	  }
	  else 
          {
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Killed @I\n",j_ins));
#endif
          if (j_ins==tmp_backup)
            {
              /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
              tmp_backup = ARM_INS_INEXT(tmp_backup);
            }
            ArmInsKill(j_ins);
          }
#ifdef DEBUG_LS_FWD
	  teller++;
#endif
	  return;
	}
      }
      /*{{{Contains bugs, don't use!*/
      /*else if (nr_diff == 2)
      {
	t_reg i_reg;
	t_bool do_it = TRUE;
	REGSET_FOREACH_REG(propregs->registers,i_reg)
	  if(RegsetIn(propregs->registers,propregs->corresponding[i_reg])) do_it = FALSE;
	if(do_it && !(ARM_INS_FLAGS(j_ins) & FL_WRITEBACK))
	{
	  REGSET_FOREACH_REG(propregs->registers,i_reg)
	    break;
	  insert_arm_ins = InsNewForBbl(ARM_INS_BBL(j_ins));
	  InsInsertAfter(insert_arm_ins,j_ins);
	  
	  ArmInsMakeMov(insert_arm_ins,propregs->corresponding[i_reg],i_reg,0,ARM_INS_CONDITION(j_ins));
	  added_moves++;
	  changed_loadmultiple++;
	  regs_to_load+=2;
	  REGSET_FOREACH_REG_INVERSE(propregs->registers,i_reg)
	    break;
	  ArmInsMakeMov(j_ins,propregs->corresponding[i_reg],i_reg,0,ARM_INS_CONDITION(j_ins));
	  VERBOSE(0,("Added @I and @I",insert_arm_ins,j_ins));
	  if(ARM_INS_REGB(i_ins) == ARM_REG_R13)
	  {
	    VERBOSE(0,("2:Killed @I",i_ins));
	    ArmInsKill(i_ins);
	    *instruction_is_killed = TRUE;
	    regs_to_load +=2;
	    killed_storemultiple++;
	  }
	}
      }*//*}}}*/
      else
      {
	VERBOSE(1,("Missing opportunity! (%d) @I, @I\n",nr_diff, i_ins, j_ins));
	goto unused;
      }
      /* add some moves here, requires 1 dead registers in worst case */
    }/*}}}*/
    else if (restored) /* !clean and restored */
    /*In this case, we try to use dead registers if any or a copy instruction if possible{{{*/
    {
      t_regset tmp_set = ARM_ALL_BUT_PC_AND_COND,tmp_set2 = propregs->registers;
      t_arm_ins * tmp_ins = i_ins;
      t_int16 i_reg, j_reg;

      goto unused;

      /* This is a heuristic: if write back happens, and we are working on the stack we can change the store */
      /* In all other cases do nothing, because it requires too much work to be profitable */
      if(!ArmInsWriteBackHappens(j_ins) || !ArmInsWriteBackHappens(i_ins)) goto unused;
      if( ARM_INS_REGB(i_ins) != ARM_REG_R13 || ARM_INS_REGB(j_ins) != ARM_REG_R13) goto unused;
      if(FUNCTION_FLAGS(BBL_FUNCTION(ARM_INS_BBL(i_ins))) & FF_DONT_CHANGE_STACK) goto unused;

#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("%d not clean, but restored: \n1)@I\n2)@I",nr_dirty,i_ins,j_ins));
#endif
      /* add some moves here, using dead registers if any */
      /*REGSET_FOREACH_REG(propregs->registers, tmp_reg) VERBOSE(0,("%d) corresponding = %d, %s, offset = %d\n",tmp_reg,propregs->corresponding[tmp_reg],propregs->changed[tmp_reg]?"dirty":"clean",propregs->offset[tmp_reg])); */
      RegsetSetDiff(tmp_set,InsRegsLiveAfter(T_INS(i_ins)));
      REGSET_FOREACH_REG(propregs->registers,i_reg)
	if (propregs->changed[i_reg]) RegsetSetSubReg(tmp_set2,i_reg);
      RegsetSetDiff(tmp_set,tmp_set2);
      if(!RegsetIsEmpty(tmp_set))
      {
	if ( ARM_INS_BBL(j_ins) == ARM_INS_BBL(i_ins))
	{
	  tmp_ins = ARM_INS_INEXT(i_ins);
	  while(tmp_ins != j_ins && !RegsetIsEmpty(tmp_set))
	  {
	    RegsetSetDiff(tmp_set,ARM_INS_REGS_DEF(tmp_ins));
	    tmp_ins = ARM_INS_INEXT(tmp_ins);
	  }
	}
	else 
	  goto unused;/*return;*/
      }

      while(!RegsetIsEmpty(tmp_set) && nr_dirty)
      {
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Dead: "));
	REGSET_FOREACH_REG(tmp_set,i_reg) VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("%d ",i_reg));
#endif
	REGSET_FOREACH_REG(tmp_set,i_reg) break;
	REGSET_FOREACH_REG(propregs->registers,j_reg)
	  if (propregs->changed[j_reg])
	  {
	    propregs->changed[j_reg] = FALSE;
	    break;
	  }
	/* change the store and load instructions to move's */
	if (nr_dirty == 1)
	{

#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed @I",i_ins));
#endif
	  ArmInsMakeMov(i_ins,i_reg,j_reg,0,ARM_INS_CONDITION(i_ins));
    ASSERT(ArmInsIsEncodable(i_ins), ("instruction @I not encodable!", i_ins));
	  if (ARM_INS_REGA(i_ins) != ARM_INS_REGC(i_ins))
	  {
	    ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
	    ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
	  }
	  else
	  {
	    ARM_INS_SET_REGS_DEF(i_ins,  NullRegs);
	    ARM_INS_SET_REGS_USE(i_ins,  NullRegs);
	  }
	  changed_storemultiple++;
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,(" to @I",i_ins));
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Changed @I",j_ins));
#endif
	  AddLiveRegisterBetween(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins),i_reg,__LINE__);
	  ArmInsMakeMov(j_ins,j_reg,i_reg,0,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
	  if (ARM_INS_REGA(j_ins) != ARM_INS_REGC(j_ins))
	  {
	    ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	    ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	  }
	  else
	  {
	    ARM_INS_SET_REGS_DEF(j_ins,  NullRegs);
	    ARM_INS_SET_REGS_USE(j_ins,  NullRegs);
	  }

#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,(" to @I",j_ins));
#endif
	  changed_loadmultiple++;
	  regs_to_load += RegsetCountRegs(propregs->registers) - 1;
#ifdef DEBUG_LS_FWD
	  teller++;
#endif
	  return;
	}
	/* add two move's to spill the register */
	else
	{
	  insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(i_ins));
	  ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	  ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
    ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(i_ins) & FL_THUMB));

	  ArmInsInsertBefore(insert_arm_ins,i_ins);

	  ArmInsMakeMov(insert_arm_ins,i_reg,j_reg,0,ARM_INS_CONDITION(i_ins));
    ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));
	  if (ARM_INS_REGA(insert_arm_ins) != ARM_INS_REGC(insert_arm_ins))
	  {
	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	  }
	  else
	  {
	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  NullRegs);
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  NullRegs);
	  }
	  ARM_INS_SET_IMMEDIATE(i_ins, ARM_INS_IMMEDIATE(i_ins) & (~j_reg));
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("One reg less to load\n1)@I\n2)@I",insert_arm_ins,i_ins));
#endif

	  insert_arm_ins = ArmInsNewForBbl(ARM_INS_BBL(j_ins));
	  ARM_INS_SET_CSIZE(insert_arm_ins, AddressNew32(4));
	  ARM_INS_SET_OLD_SIZE(insert_arm_ins, AddressNew32(0));
    ARM_INS_SET_FLAGS(insert_arm_ins, ARM_INS_FLAGS(insert_arm_ins) | (ARM_INS_FLAGS(j_ins) & FL_THUMB));
	  ArmInsInsertBefore(insert_arm_ins,j_ins);

	  ArmInsMakeMov(insert_arm_ins,j_reg,i_reg,0,ARM_INS_CONDITION(j_ins));
    ASSERT(ArmInsIsEncodable(insert_arm_ins), ("instruction @I not encodable!", insert_arm_ins));
	  if (ARM_INS_REGA(insert_arm_ins) != ARM_INS_REGC(insert_arm_ins))
	  {
	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  ArmDefinedRegisters(insert_arm_ins));
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  ArmUsedRegisters(insert_arm_ins));
	  }
	  else
	  {
	    ARM_INS_SET_REGS_DEF(insert_arm_ins,  NullRegs);
	    ARM_INS_SET_REGS_USE(insert_arm_ins,  NullRegs);
	  }
	  ARM_INS_SET_IMMEDIATE(j_ins, ARM_INS_IMMEDIATE(j_ins)&(~j_reg));
	  if(ARM_INS_IMMEDIATE(j_ins) == 0) FATAL(("Make a noop!"));
	  regs_to_load++;
#ifdef DEBUG_LS_FWD
	  VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("3)@I\n4)@I",insert_arm_ins,j_ins));
#endif
	}
	nr_dirty--;
	RegsetSetSubReg(tmp_set,i_reg);
#ifdef DEBUG_LS_FWD
	teller++;
#endif
      }
      if (nr_dirty)
      {
	REGSET_FOREACH_REG(propregs->registers,j_reg)
	  if (!propregs->changed[j_reg])
	  {
	    ARM_INS_SET_IMMEDIATE(i_ins, ARM_INS_IMMEDIATE(i_ins)&(~j_reg));
	    ARM_INS_SET_IMMEDIATE(j_ins, ARM_INS_IMMEDIATE(j_ins)&(~j_reg));
	    regs_to_load++;
	  }
	ARM_INS_SET_ATTRIB(i_ins,   0);
	ARM_INS_SET_ATTRIB(j_ins,  0);
	ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
	ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
	ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Store and load not gone, now @I and @I",i_ins,j_ins));
#endif
      }
    }
    /*}}}*/
    else
    {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("%d not clean and %d not restored",nr_dirty,nr_diff));
#endif
      goto unused;
    }
    /*teller++;*/
  }/*}}}*/

  else if (ARM_INS_OPCODE(i_ins) == ARM_STM && ARM_INS_OPCODE(j_ins) == ARM_STM && BblPostdominates(ARM_INS_BBL(j_ins),ARM_INS_BBL(i_ins)))
  /* fourth case : two store multiples following each other, the second overwriting
   * what the first has written. Kill the first one, preserve side-effects */
  /*{{{*/
  {
    t_regset tmp_set1 = RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins));
    t_reg tmp_reg;
    t_bool overwrite = TRUE;
    t_int32 diff = 0;

    /*VERBOSE(0,("Now using LS-info :\n1)@I\n2)@I\n",i_ins,j_ins)); */
    diff = (RegsetCountRegs(propregs->registers) - RegsetCountRegs(tmp_set1));

    REGSET_FOREACH_REG(propregs->registers,tmp_reg)
    {
      if (propregs->corresponding[tmp_reg] == MAX_REG_ITERATOR) overwrite = FALSE;
    }
    if(!overwrite && diff <= 0) return;/*FATAL(("How is this possible?\n"));*/
    /* TODO: use this info!!!! */

    if (overwrite)
    {
      if (ArmInsWriteBackHappens(i_ins))
      {
#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I changed because it is immediately overwritten ",i_ins));
#endif
	if (ARM_INS_FLAGS(i_ins) & FL_DIRUP) up = TRUE;
	if (up) ArmInsMakeAdd(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,4 * RegsetCountRegs(propregs->registers),ARM_INS_CONDITION(i_ins));
	else ArmInsMakeSub(i_ins,ARM_INS_REGB(i_ins),ARM_INS_REGB(i_ins),ARM_REG_NONE,4 * RegsetCountRegs(propregs->registers),ARM_INS_CONDITION(i_ins));
  ASSERT(ArmInsIsEncodable(i_ins), ("instruction @I not encodable!", i_ins));

	ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));
	ARM_INS_SET_REGS_USE(i_ins,  ArmUsedRegisters(i_ins));
        changed_storemultiple++;
	regs_to_load += RegsetCountRegs(propregs->registers);
#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("to @I",i_ins));
#endif
      }
      else
      {
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("STM Killed because it is immediately overwritten!!! @I",i_ins));
#endif
	ArmInsKill(i_ins);
	*instruction_is_killed = TRUE;
	killed_storemultiple++;
	regs_to_load += RegsetCountRegs(propregs->registers);
      }
#ifdef DEBUG_LS_FWD
      teller++;
#endif
      return;
    }
    

  }/*}}}*/
  else if ((ARM_INS_OPCODE(i_ins) == ARM_LDR || ARM_INS_OPCODE(i_ins) == ARM_STR) && ARM_INS_OPCODE(j_ins) == ARM_LDM)
    /* {{{ */
  {
    t_reg loadreg = ARM_INS_REGA(i_ins);
    t_regset tmp_set = RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins));
    t_bool first=FALSE,last=FALSE;
    t_reg i_reg,f_reg = ARM_REG_NONE,l_reg=ARM_REG_NONE;

    if(propregs->changed[loadreg]) return;
    if(!(ARM_INS_FLAGS(j_ins) & FL_WRITEBACK)) return;
    REGSET_FOREACH_REG(tmp_set,i_reg)
    {
      if(f_reg == ARM_REG_NONE) f_reg = i_reg;
      l_reg = i_reg;
    }
    if (loadreg == f_reg) first = TRUE;
    if (loadreg == l_reg) last = TRUE;
    if (first || last) VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Ok we can do something with this! @I, @I",i_ins,j_ins));
    goto unused;
    
  }/*}}}*/
  else if (ARM_INS_OPCODE(i_ins) == ARM_LDR && ARM_INS_OPCODE(j_ins) == ARM_STR && ARM_INS_REGA(i_ins) == ARM_INS_REGA(j_ins) && !untraced_store)
  {
    t_reg loadreg = ARM_INS_REGA(i_ins);

    if (propregs->corresponding[ARM_INS_REGB(i_ins)] != ARM_INS_REGB(j_ins))
    {
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("do not use this combination because the base registers do not match! (propregs->corresponding[%d] != %d)", ARM_INS_REGB(i_ins), ARM_INS_REGB(j_ins)));
      goto unused;
    }
    if(propregs->changed[loadreg]) goto unused;

    if(!ArmInsWriteBackHappens(j_ins))
    {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("STM Killed because it is unnecessary!!! @I",j_ins));
#endif
      if (j_ins==tmp_backup)
        {
          /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
          tmp_backup = ARM_INS_INEXT(tmp_backup);
        }
      ArmInsKill(j_ins);
      *instruction_is_killed = TRUE;
      regs_to_load++;
      killed_store++;
      teller++;
    }
    else
    {
#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I changed because it is unnecessary ",j_ins));
#endif
      if(ARM_INS_FLAGS(j_ins) & FL_DIRUP) ArmInsMakeAdd(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(j_ins));
      else ArmInsMakeSub(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(j_ins));
      ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));
      regs_to_load++;
      changed_load++;
      teller++;
#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("to @I",j_ins));
#endif
    }
    return;
    
  }
  else if(ARM_INS_OPCODE(i_ins) == ARM_LDM && ARM_INS_OPCODE(j_ins) == ARM_STM && !untraced_store)
  {
    t_regset tmp_set1 = RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins));
    t_reg tmp_reg;
    t_bool do_it = TRUE;
    t_int32 diff = 0;

    diff = (RegsetCountRegs(propregs->registers) - RegsetCountRegs(tmp_set1));
    if(diff) goto unused;/* For the moment we return, but we can do better!*/

    REGSET_FOREACH_REG(propregs->registers,tmp_reg)
    {
      if(propregs->corresponding[tmp_reg] != tmp_reg) do_it = FALSE;
      if(propregs->changed[tmp_reg]) do_it = FALSE;
    }

    if(do_it)
    {
      if(ArmInsWriteBackHappens(j_ins))
      {
#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("@I changed because it is unnecessary ",j_ins));
#endif
	if(ARM_INS_FLAGS(j_ins) & FL_DIRUP) ArmInsMakeAdd(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(propregs->registers),ARM_INS_CONDITION(j_ins));
	else ArmInsMakeSub(j_ins,ARM_INS_REGB(j_ins),ARM_INS_REGB(j_ins),ARM_REG_NONE,4 * RegsetCountRegs(propregs->registers),ARM_INS_CONDITION(j_ins));
  ASSERT(ArmInsIsEncodable(j_ins), ("instruction @I not encodable!", j_ins));

	ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
	ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
        changed_storemultiple++;
	regs_to_load += RegsetCountRegs(propregs->registers);

#ifdef DEBUG_LS_FWD
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("to @I",j_ins));
#endif
      }
      else
      {
#ifdef DEBUG_LS_FWD
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("STM Killed because it is unnecessary!!! @I",j_ins));
#endif
        if (j_ins==tmp_backup)
          {
            /* apparently, we are killing the next instruction of the BBL_FOREACH_ARM_INS_SAFE loop, we need to avoid a dangling tmp pointer! */
            tmp_backup = ARM_INS_INEXT(tmp_backup);
          }
	ArmInsKill(j_ins);
	*instruction_is_killed = TRUE;
	killed_storemultiple++;
	regs_to_load += RegsetCountRegs(propregs->registers);
      }
      teller++;
      return;
    }
  }
#ifdef DEBUG_LS_FWD
  else
#endif
unused:
#ifdef DEBUG_LS_FWD
    VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Unused pair @I and @I",i_ins,j_ins));
#endif
  
  return;
}/*}}}*/
/*!
 * \todo Document
 *
 * \param i_bbl
 * \param load_reg
 * \param propregs
 * \param i_ins
 * \param untraced_load
 * \param untraced_store
 *
 * \return void 
*/
/* ArmLoadStoreFwdRecurse {{{ */
void ArmLoadStoreFwdRecurse(t_cfg * cfg, t_bbl * i_bbl, t_reg load_reg, t_partial_value * propregs, t_arm_ins * i_ins, t_bool * untraced_load, t_bool * untraced_store, t_bool * instruction_is_killed)
{
  t_cfg_edge * edge,* edge2;
  t_uint32 real_preds;
  t_bool untraced_load_traceback = *untraced_load, untraced_store_traceback = *untraced_store;
  t_partial_value propregs_traceback = *propregs;
  if (BblIsMarked(i_bbl)) return;

  if(*untraced_store) worst_case_untraced_store = TRUE;
  if(*untraced_load) worst_case_untraced_load = TRUE;

  BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
  {
    t_bool worst_case = FALSE;
    *untraced_load = untraced_load_traceback;
    *untraced_store = untraced_store_traceback;
    *propregs = propregs_traceback;
    CfgEdgeMark(edge);

    /* These edges are no real control flow, skip them, Hell node are useless to propagate to, skip them */
    if (BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
    {
      continue;
    }
    else if (
             diabloanopt_options.rely_on_calling_conventions && 
             (CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge)==ET_RETURN || CFG_EDGE_CAT(edge)==ET_COMPENSATING || CFG_EDGE_CAT(edge)==ET_IPJUMP) //&&
             )
    {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("stop propagation of %d over @E into fun %s",load_reg,edge,FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_TAIL(edge)))));
#endif
      continue;
    }
    else 
    {
#ifdef DEBUG_LS_FWD
      if (CFG_EDGE_CAT(edge)==ET_CALL)
        VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("propagating %d over @E into fun %s",load_reg,edge,FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_TAIL(edge)))));
#endif
      real_preds=0;
      BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(edge),edge2)
      {
	/*Fun link edges don't count as predecessor edges for load_store fwd */
	if (edge == edge2) continue;
#if 0
	if (EdgeIsMarked(edge2))
	{
	  t_bool stop = FALSE;
	  t_reg tmp_reg;
	  REGSET_FOREACH_REG(propregs->registers,tmp_reg)
	    if(propregs->offset[tmp_reg] != propregs_traceback.offset[tmp_reg]) stop = TRUE;

	  /* We merge info here, used worst case assumptions (ie. when going through a diamondlike piece of the graph) */
	  if(stop == FALSE)
	  {
	    worst_case = TRUE;
	    continue;
	  }
	}
#endif
	real_preds++;
	break;
      }
      if (real_preds==0)
      {
	if(worst_case)
	{
	  *untraced_load = worst_case_untraced_load;
	  *untraced_store = worst_case_untraced_store;
	}
	BblMark(i_bbl);
	if (ArmLoadStoreFwdInner2(cfg,T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge))), load_reg, propregs, i_ins, untraced_load, untraced_store, instruction_is_killed))
	{
	  ArmLoadStoreFwdRecurse(cfg,CFG_EDGE_TAIL(edge),load_reg,propregs,i_ins, untraced_load, untraced_store, instruction_is_killed);
	}
      }
    }
  }
}/*}}}*/

/*!
 * \todo Document
 *
 * \param cfg
 *
 * \return void 
*/
/* ArmLoadStoreFwd {{{ */



static void RealArmLoadStoreFwd(t_cfg* cfg, t_bool do_dominator_analysis)
{
  t_bbl * i_bbl;
  t_arm_ins * i_ins, * j_ins, * tmp;
  t_partial_value propregs;
  t_reg load_reg, tmpreg;
  t_bool untraced_load, untraced_store;
#if 0
  t_reloc * dummytag;
#endif
  t_uint16 nr_regs,i;
  int reg_size;
  
  load_count =0;
  store_count=0;
  load_tag_count=0;
  store_tag_count=0;

  /* we need liveness information and dominator information to do this correctly */
  //  if (do_dominator_analysis)
    //    ComDominators(cfg);

  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE); 

  //  BblInitDomMarkingNumber(cfg);

  STATUS(START,("ArmLoadStore"));

  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    t_bool instruction_is_killed = TRUE;

    worst_case_untraced_store = worst_case_untraced_load = FALSE;

    if (!BBL_FUNCTION(i_bbl)) continue;

    /*    VERBOSE(0,("In function @F\n@iB",BBL_FUNCTION(i_bbl),i_bbl));*/

    while (instruction_is_killed)
      {
	instruction_is_killed = FALSE;

    /* Safe, because instructions can get killed */
	BBL_FOREACH_ARM_INS_SAFE(i_bbl,i_ins,tmp_backup)
	  {
	    /* First some statistics: count loads and stores {{{*/
	    if (ARM_INS_TYPE(i_ins) == IT_LOAD || ARM_INS_TYPE(i_ins) == IT_LOAD_MULTIPLE /*|| ArmIsSupportedFltLoad(i_ins)*/)
	      {
		load_count++;
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
		if(!RegStateGetRegTag(i_ins,&ARM_INS_REGS_USE(i_ins),ARM_INS_REGB(i_ins),&dummytag)) load_tag_count++;
#else
		if (!CP_BOT) load_tag_count++;
#endif
	      }
	    if (ARM_INS_TYPE(i_ins) == IT_STORE || ARM_INS_TYPE(i_ins) == IT_STORE_MULTIPLE /*|| ArmIsSupportedFltStore(i_ins)*/)
	      {
		store_count++;
#if 0 /* REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE*/
		if(!RegStateGetRegTag(i_ins,&ARM_INS_REGS_USE(i_ins),ARM_INS_REGB(i_ins),&dummytag)) store_tag_count++;
#else
		if (!CP_BOT) store_tag_count++;
#endif
	      }
	    /* }}}*/
      
      

	    if (ARM_INS_IS_CONDITIONAL(i_ins)) continue;


	    /* TODO: The following could be less restricting, because loading/storing bytes and
	     * halfwords might be included as well, since they will overwrite memory, corrupting what
	     * was there (except when some does tricky things!) */

	    if (ARM_INS_OPCODE(i_ins) == ARM_STR || 
		ARM_INS_OPCODE(i_ins) == ARM_LDR ||
		ARM_INS_OPCODE(i_ins) == ARM_LDRB ||
		ARM_INS_OPCODE(i_ins) == ARM_STRB/* ||
		ARM_INS_OPCODE(i_ins) == ARM_VSTR ||
		ARM_INS_OPCODE(i_ins) == ARM_VLDR*/)
	      {
		if (ARM_INS_REGB(i_ins) == ARM_INS_REGA(i_ins) && (ARM_INS_TYPE(i_ins) == IT_LOAD || ArmIsSupportedFltLoad(i_ins)))
		  continue; /* Too hard to handle */

		memset(&propregs,0,sizeof(t_partial_value));

		/* Vector stores and loads can either work with single or double registers (32 or 64-bit).
		 * Here we indicate whether the instruction works with double registers or not, because this
		 * is not always clear in the regset (for example: register S0-S31 are mapped to D0-D15, and if
		 * S0 is checked, how can we know from the regset if the register is used as a double or a single one?).
		 */

		/*		if (ARM_INS_OPCODE(i_ins) == ARM_VLDR || ARM_INS_OPCODE(i_ins)==ARM_VSTR && ARM_INS_FLAGS(i_ins)&FL_VFP_DOUBLE)
		{
			propregs.double_registers = TRUE;
		}
		*/
		FOREACH_REG(tmpreg) propregs.corresponding[tmpreg] = MAX_REG_ITERATOR;

		if (ArmComputeLSOffset(i_ins,&(propregs.offset[ARM_INS_REGA(i_ins)]))) continue;

		RegsetSetAddReg(propregs.registers,ARM_INS_REGA(i_ins));

		load_reg = ARM_INS_REGB(i_ins);

		/* Set load_offset so that 'Store_addres used' + load_offset = load_reg content */

                if (propregs.offset[ARM_INS_REGA(i_ins)] != 0
		    && (ARM_INS_FLAGS(i_ins) & FL_PREINDEX)
		    && !(ARM_INS_FLAGS(i_ins) & FL_WRITEBACK) )
		  propregs.offset[ARM_INS_REGA(i_ins)] = -(propregs.offset[ARM_INS_REGA(i_ins)]);
		else if (propregs.offset[ARM_INS_REGA(i_ins)] != 0 && !(ARM_INS_FLAGS(i_ins) & FL_PREINDEX))
		  { /* nothing to do */ }
		else if (propregs.offset[ARM_INS_REGA(i_ins)] != 0) propregs.offset[ARM_INS_REGA(i_ins)] = 0; 

		j_ins = ARM_INS_INEXT(i_ins);
		untraced_load = FALSE;
		untraced_store = FALSE;
	
		/*	if(ARM_INS_INEXT(i_ins) && ARM_INS_OPCODE(ARM_INS_INEXT(i_ins)) == ARM_STR && ARM_INS_INEXT(ARM_INS_INEXT(i_ins)) && ARM_INS_OPCODE(ARM_INS_INEXT(ARM_INS_INEXT(i_ins))) == ARM_STR)*/
		/*	VERBOSE(0,("Doing @I\n",i_ins));*/

		if (ArmLoadStoreFwdInner2(cfg,j_ins, load_reg, &propregs, i_ins, &untraced_load, &untraced_store,&instruction_is_killed))
		  {
		    NodeMarkInit();
		    EdgeMarkInit();
		    ArmLoadStoreFwdRecurse(cfg,i_bbl, load_reg, &propregs, i_ins, &untraced_load, &untraced_store, &instruction_is_killed);
		  }
	      }
	    else if (	(ARM_INS_OPCODE(i_ins) == ARM_STM 	|| ARM_INS_OPCODE(i_ins) == ARM_LDM /*
		      || ARM_INS_OPCODE(i_ins) == ARM_VSTM 	|| ARM_INS_OPCODE(i_ins) == ARM_VLDM
		      || ARM_INS_OPCODE(i_ins) == ARM_VPUSH 	|| ARM_INS_OPCODE(i_ins) == ARM_VPOP*/))
	      {
		memset(&propregs,0,sizeof(t_partial_value));

#if 0
		if (ARM_INS_OPCODE(i_ins) == ARM_VSTM 	|| ARM_INS_OPCODE(i_ins) == ARM_VLDM
		      || ARM_INS_OPCODE(i_ins) == ARM_VPUSH 	|| ARM_INS_OPCODE(i_ins) == ARM_VPOP)
		{
			/* for these instructions, the register list is stored in the MULTIPLE-member of the instruction class */
			propregs.registers = ARM_INS_MULTIPLE(i_ins);

			if(ARM_INS_FLAGS(i_ins) & FL_VFP_DOUBLE)
			{
				/* indicate whether we are working with double registers or not */
				propregs.double_registers = TRUE;
			}
		}
		else
#endif
		{
			propregs.registers = RegsetNewFromUint32(ARM_INS_IMMEDIATE(i_ins));
		}

		/* this test only works for stm and ldm, it needs to be extended in case we also gonna propagate from other load_multiple or store_multiple instructions */
		if (ARM_INS_TYPE(i_ins) == IT_LOAD_MULTIPLE && ARM_INS_REGB(i_ins) == ARM_REG_R13 && (ARM_INS_FLAGS(i_ins) & FL_DIRUP) && ARM_INS_FLAGS(i_ins) & FL_WRITEBACK)
		  continue; /* values above the stack pointer (which the loaded values are in this case) should not be considered valid and should hence not be propagated */

		if (ARM_INS_TYPE(i_ins) == IT_LOAD_MULTIPLE && RegsetIn(propregs.registers,ARM_INS_REGB(i_ins)))
		  continue; /* Too hard to handle */

		FOREACH_REG(tmpreg) propregs.corresponding[tmpreg] = MAX_REG_ITERATOR;

		load_reg = ARM_INS_REGB(i_ins);

		/* Now for each register that gets loaded/stored : calculate the offset from the load_reg
		 * and put this in the propreg structure */
		nr_regs = RegsetCountRegs(propregs.registers);
		i = 0;

		reg_size = 4;
#if 0
		if (propregs.double_registers)
		{
			reg_size = 8;
		}
#endif
		if (ARM_INS_FLAGS(i_ins) & FL_DIRUP)
		  {
		    if (ARM_INS_FLAGS(i_ins) & FL_WRITEBACK)
		      {
			if (ARM_INS_FLAGS(i_ins) & FL_PREINDEX) i++;
			REGSET_FOREACH_REG(propregs.registers,tmpreg)
			  {
			    propregs.offset[tmpreg] = reg_size * (nr_regs-i);
			    i++;
			  }
		      }
		    else /* no writeback, dirup */
		      {
			if (ARM_INS_FLAGS(i_ins) & FL_PREINDEX) i++;
			REGSET_FOREACH_REG(propregs.registers,tmpreg)
			  {
			    propregs.offset[tmpreg] = -reg_size * i;
			    i++;
			  }
		      }
		  }
		else /* no dirup */
		  {
		    if (ARM_INS_FLAGS(i_ins) & FL_WRITEBACK)
		      {
			if (!(ARM_INS_FLAGS(i_ins) & FL_PREINDEX)) i++;
			REGSET_FOREACH_REG(propregs.registers,tmpreg)
			  {
			    propregs.offset[tmpreg] = -reg_size * i;
			    i++;
			  }
		      }
		    else /* no writeback */
		      {
			if (!(ARM_INS_FLAGS(i_ins) & FL_PREINDEX)) i++;
			REGSET_FOREACH_REG(propregs.registers,tmpreg)
			  {
			    propregs.offset[tmpreg] = reg_size * (nr_regs-i);
			    i++;
			  }
		      }
		  }

		j_ins = ARM_INS_INEXT(i_ins);
		/*if(ARM_INS_OPCODE(i_ins) == ARM_LDM && ARM_INS_OPCODE(j_ins) == ARM_STM)*/
		/*VERBOSE(0,("Doing @I and @I\n",i_ins,j_ins));*/
		untraced_load = FALSE;
		untraced_store = FALSE;
		if (ArmLoadStoreFwdInner2(cfg,j_ins, load_reg, &propregs, i_ins, &untraced_load, &untraced_store, &instruction_is_killed))
		  {
		    NodeMarkInit();
		    EdgeMarkInit();
		    ArmLoadStoreFwdRecurse(cfg,i_bbl, load_reg, &propregs, i_ins, &untraced_load, &untraced_store,&instruction_is_killed);
		  }
	      }
	    if (instruction_is_killed)
	      break;
	  }
        tmp_backup = NULL; 
      }
  }
  VERBOSE(0,("%d loads,  %d with tags",load_count,load_tag_count));
  VERBOSE(0,("%d stores, %d with tags",store_count,store_tag_count));
  VERBOSE(0,("%d = sum,  %d with tags",store_count+load_count,store_tag_count+load_tag_count));

  /*VERBOSE(0,("[load-store forward'n] %d stores killed, %d loads changed, %d moves added, %d add's added, %d ldm's changed, %d stm's changed %d stm's killed, %d ldm's killed\n",killed_store,changed_load,added_moves,added_adds,changed_loadmultiple,changed_storemultiple,killed_storemultiple,killed_loadmultiple)); */
  /*VERBOSE(0,("[load-store forward'n] %d registers less to spill, %d load/stores less\n",regs_to_load,killed_store+changed_load+changed_loadmultiple+changed_storemultiple+killed_storemultiple+killed_loadmultiple)); */
  STATUS(STOP,("ArmLoadStore"));

  /* clean up the dominator information */
  //  BblFiniDomMarkingNumber(cfg);
  //  DominatorCleanup(cfg);

  return; /*killed_store+changed_load+added_moves+added_adds+changed_loadmultiple+killed_storemultiple+killed_loadmultiple;  */
}/*}}}*/


void ArmLoadStoreFwd(t_cfg* cfg)
{
  RealArmLoadStoreFwd(cfg,TRUE);
}

void ArmLoadStoreFwdNoDom(t_cfg* cfg)
{
  RealArmLoadStoreFwd(cfg,FALSE);
}



/*!
 * given an instruction, and an offset, we try to keep track
 * of the value of this offset. For the moment, we only try to trace add's and sub's
 * of a register with an immediate into that same register 
 *
 * \param ins
 * \param offset
 *
 * \return t_lattice_level 
*/
/* ArmPartialEvaluation{{{*/
t_lattice_level ArmPartialEvaluation(t_arm_ins* ins, t_register_content * offset, t_reg reg)
{
  if (ARM_INS_IS_CONDITIONAL(ins)) return CP_BOT;

  /*t_register_content tmp_content; */
  if (ARM_INS_REGA(ins) == ARM_INS_REGB(ins) && ArmInsHasImmediate(ins) && ARM_INS_REGA(ins) == reg)
  {
    if(ARM_INS_OPCODE(ins) == ARM_ADD)
    {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Add evaluated! @I\n",ins));
#endif
      offset->i = AddressAddUint32(offset->i,ARM_INS_IMMEDIATE(ins));
      return CP_VALUE;
    }
    if(ARM_INS_OPCODE(ins) == ARM_SUB)
    {
#ifdef DEBUG_LS_FWD
      VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Sub evaluated! @I\n",ins));
#endif
      offset->i = AddressSubUint32(offset->i,ARM_INS_IMMEDIATE(ins));
      return CP_VALUE;
    }
    else return CP_BOT;
  }
  else if (ARM_INS_REGB(ins) == reg && (ARM_INS_TYPE(ins) == IT_LOAD || ARM_INS_TYPE(ins) == IT_STORE || ArmFltLoadStoreIsSupported(ins) ) && ARM_INS_FLAGS(ins) & FL_WRITEBACK && ARM_INS_FLAGS(ins) & FL_IMMED)
  {
    if(ARM_INS_FLAGS(ins) & FL_DIRUP)
    {
      offset->i = AddressAddUint32(offset->i,ARM_INS_IMMEDIATE(ins));
      return CP_VALUE;
    }
    else
    {
      offset->i = AddressSubUint32(offset->i,ARM_INS_IMMEDIATE(ins));
      return CP_VALUE;
    }
  }
  else if (ARM_INS_REGB(ins) == reg
  	&& (ARM_INS_OPCODE(ins) == ARM_STM || ARM_INS_OPCODE(ins) == ARM_LDM
  		|| ARM_INS_OPCODE(ins)==ARM_VLDM || ARM_INS_OPCODE(ins)==ARM_VSTM
  		|| ARM_INS_OPCODE(ins)==ARM_VPUSH || ARM_INS_OPCODE(ins)==ARM_VPOP)
  	&& (ARM_INS_FLAGS(ins) & (FL_WRITEBACK) ))
  {
    t_regset tmp_set = RegsetNewFromUint32 (ARM_INS_IMMEDIATE(ins));
    int reg_size = 4;

    if (ARM_INS_OPCODE(ins)==ARM_VLDM || ARM_INS_OPCODE(ins)==ARM_VSTM || ARM_INS_OPCODE(ins)==ARM_VPUSH || ARM_INS_OPCODE(ins)==ARM_VPOP)
    {
    	/* these instructions store their register list in the MULTIPLE member of the instruction class */
    	tmp_set = ARM_INS_MULTIPLE(ins);

    	if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
    	{
    		reg_size = 8;
    	}
    }

    t_int32 nr_regs = RegsetCountRegs(tmp_set);
    nr_regs *= reg_size;
    RegsetFree(tmp_set);
    if(ARM_INS_FLAGS(ins) & FL_DIRUP)
    {
      offset->i = AddressAddUint32(offset->i,nr_regs);
      return CP_VALUE;
    }
    else
    {
      offset->i = AddressSubUint32(offset->i,nr_regs);
      return CP_VALUE;
    }
  }
#if 0
  else if (ARM_INS_REGA(ins) == ARM_INS_REGB(ins) && ARM_INS_REGC(ins) != ARM_REG_NONE && ARM_INS_REGA(ins) == reg)
  {
    if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_INS_REGC(ins)))
    {
      t_register_content tmp_content;
#if 0 // REGSTATES ARE NEVER SET FOR THE MOMENT, AND THEREFORE NOT DECLARED ANYMORE
      if(RegStateGetRegValue(ins,&ARM_INS_REGS_USE(ins),ARM_INS_REGC(ins),&tmp_content))
#else
	if (CP_BOT)
#endif
	return CP_BOT;
      if(ARM_INS_OPCODE(ins) == ARM_ADD)
      {
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Add evaluated! @I\n",ins));
	FATAL((""));
	offset->i = AddressAdd(offset->i,tmp_content.i);
	return CP_VALUE;
      }
      if(ARM_INS_OPCODE(ins) == ARM_SUB)
      {
	VERBOSE(DEBUG_LS_VERBOSE_LEVEL,("Sub evaluated! @I\n",ins));
	offset->i = AddressSubUint32(offset->i,tmp_content.i);
	return CP_VALUE;
      }
      else return CP_BOT;
    }
    else return CP_BOT;
  }
  /* TODO: implement */
#endif
  return CP_BOT;
}/*}}}*/

/* vim: set shiftwidth=2 foldmethod=marker : */
