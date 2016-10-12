/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>

#define CONST_OPT_VERBOSITY 5
/*!
 * Resets an instruction's S bit when it turns out the instruction doesn't
 * affect the status bits.
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \return t_bool FALSE if the optimization killed an instruction 
*/
/* ArmInsOptKillSBit {{{ */
t_bool ArmInsOptKillSBit(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  /* In Thumb, care should be taken when killing the S-bit.
   *  - Except for CMP, CMN and TST, no 16-bit Thumb instructions can set the S-bit in an IT-block;
   *  - 32-bit Thumb instructions CAN set the S-bit in an IT-block.
   * At this time, IT-instructions do not exist yet. However, Thumb instructions can be conditional.
   * Thus, we can assume that a Thumb instruction is inside an IT-block when its condition code
   * does not equal AL. */

  if (ARM_INS_FLAGS(ins) & FL_S)
    if (ConditionsAreEqualAndKnown(before,after))
    {
      if (ARM_INS_OPCODE(ins)==ARM_CMP || ARM_INS_OPCODE(ins)==ARM_CMN || ARM_INS_OPCODE(ins)==ARM_TST || ARM_INS_OPCODE(ins)==ARM_TEQ)
      {
        //DEBUG(("killing ins that supposedly only sets constant flags: @I",ins));
        ArmInsKillDelayed(ins);
        return FALSE;
      }
      else

      {
        if (ARM_INS_FLAGS(ins) & FL_THUMB)
          {
            if (ArmIsThumb1Encodable(ins))
              return TRUE;

            //DEBUG(("killing setting of flags that supposedly only sets constant flags: @I",ins));
            ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) & (~FL_S));
            ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
            ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          }
        else
          {
            ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) & (~FL_S));
            ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
            ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          }
        return TRUE;
      }
    }
  return TRUE;
}
/* }}} */
/*!
 * Kills the instruction if it is idempotent.
 *     
 *
 * \warning This should only be called after ArmInsOptKillSBit! 
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \return t_bool FALSE if the optimization killed an instruction 
*/
/* ArmInsOptKillIdempotent {{{ */
t_bool ArmInsOptKillIdempotent(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  t_bool doesnothing;
  t_reg r;

  doesnothing = TRUE;

  REGSET_FOREACH_REG(ARM_INS_REGS_DEF(ins),r)
  {
    if (!RegIsEqualAndKnown(r,before,after))
    {
      doesnothing = FALSE;
      break;
    }
  }
  /* If the S bit was set, the instruction effectively changes the status bits, otherwise ArmInsOptKillSBit
   * would have reset the S bit. */
  if ((ARM_INS_FLAGS(ins) & FL_S) &&
      /* Thumb instructions are a special case */
      (ARM_INS_FLAGS(ins) & FL_THUMB) &&
      /* ... inside an IT-block ... */
      (ArmInsIsConditional(ins)) &&
      /* ... of 16 bits wide ... */
      (ARM_INS_CSIZE(ins) == 2))
  {
    /* ... never set the condition flags, except for CMP, CMN and TST */
    if ((ARM_INS_OPCODE(ins) == ARM_CMP) || (ARM_INS_OPCODE(ins) == ARM_CMN) || (ARM_INS_OPCODE(ins) == ARM_TST))
        doesnothing = FALSE;
  }
  else
  {
    doesnothing = doesnothing && !(ARM_INS_FLAGS(ins) & FL_S);
  }

  /* only remove the instruction if it doesn't have a side effect (like writing something to memory) */
  if (doesnothing && !ArmInsHasSideEffect(ins))
  {
    {
      VERBOSE(CONST_OPT_VERBOSITY,("Idempotent: @I\n",ins));
      ArmInsKillDelayed(ins);
      return FALSE;
    }
  }
  return TRUE;
}
/* }}} */
/*!
 * turns indirect jumps and calls of which the destination is known into direct
 * jumps or calls, and changes the control flow graph accordingly.
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \return t_bool always TRUE (this optimization kills no instructions) 
*/
/* ArmInsOptMakeIndirectJumpsDirect {{{ */
t_bool ArmInsOptMakeIndirectJumpsDirect(t_arm_ins * ins, t_procstate * before, t_procstate * after, t_analysis_complexity complexity)
{
  t_register_content value;

  /* MOV pc,rXX with rXX == constant value
   * or
   * BX rXX with rXX == constant value and !(value & 1)
   * (otherwise this BX switches to thumb mode)
   */
  if (((ARM_INS_REGA(ins)==ARM_REG_R15) && (ARM_INS_OPCODE(ins)==ARM_MOV) && (ARM_INS_REGC(ins)!=ARM_REG_NONE) && (!ProcStateGetReg(before,ARM_INS_REGC(ins),&value)))
      || ((ARM_INS_OPCODE(ins)==ARM_BX) &&  (ARM_INS_REGB(ins)!=ARM_REG_NONE) && (!ProcStateGetReg(before,ARM_INS_REGB(ins),&value)) && !(AddressExtractInt32(value.i)&1)))
  {
    t_reloc * rel;
    t_cfg_edge * i_edge, *tmp;
    t_bbl * link = NULL;
    t_uint32 found=0;

    t_cfg_edge * corr_edge;

    if (((ARM_INS_OPCODE(ins)==ARM_MOV) && (!ProcStateGetTag(before,ARM_INS_REGC(ins),&rel))) ||
        ((ARM_INS_OPCODE(ins)==ARM_BX) && (!ProcStateGetTag(before,ARM_INS_REGB(ins),&rel))))
    {
      if ((RELOC_N_TO_RELOCATABLES(rel)==1) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0])==RT_BBL))
      {
	BBL_FOREACH_SUCC_EDGE_SAFE(ARM_INS_BBL(ins),i_edge,tmp)
	{
	  if ((CFG_EDGE_CAT(i_edge)==ET_IPJUMP) || (CFG_EDGE_CAT(i_edge)==ET_CALL))
	  {
	    ASSERT(!found,("Multiple edges!"));
	    found = CFG_EDGE_CAT(i_edge);
/*            if (CFG_EDGE_CAT(i_edge)==ET_CALL)*/
	    {
	      link = CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge));
	    }
	    if (CFG_EDGE_CORR(i_edge)==tmp) 
	      tmp = CFG_EDGE_NEXT(tmp);

	    corr_edge = CFG_EDGE_CORR(i_edge);

	    if (complexity == CONTEXT_INSENSITIVE && corr_edge == BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge)))
	      {
		
		if (CFG_EDGE_SUCC_NEXT(corr_edge))
		  {
		    CFG_EDGE_SET_PROCSTATE(CFG_EDGE_SUCC_NEXT(corr_edge),  CFG_EDGE_PROCSTATE(corr_edge));
		     CFG_EDGE_SET_PROCSTATE(corr_edge,  NULL);
		  }
	      }
	    
	    CfgEdgeKill(CFG_EDGE_CORR(i_edge));

	    if (complexity == CONTEXT_INSENSITIVE && i_edge == (BBL_PRED_FIRST(CFG_EDGE_TAIL(i_edge))) && CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (CFG_EDGE_PRED_NEXT(i_edge))
		  {
		    CFG_EDGE_SET_PROCSTATE(CFG_EDGE_PRED_NEXT(i_edge),  CFG_EDGE_PROCSTATE(i_edge));
		    CFG_EDGE_SET_PROCSTATE(i_edge,  NULL);
		  }
	      }


	    CfgEdgeKill(i_edge);
	  }
	}

	ASSERT(found,("No edge found"));
	VERBOSE(CONST_OPT_VERBOSITY,("Change @I to ",ins));

	if (found == ET_IPJUMP)
	{
	  t_cfg_edge * edge;
	  t_bbl * exit_bbl = FunctionGetExitBlock(BBL_FUNCTION(T_BBL(RELOC_TO_RELOCATABLE(rel)[0])));
	  t_cfg_edge * comp_edge;
	  ARM_INS_SET_OPCODE(ins,  ARM_B);
	  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
	  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
	  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  ARM_INS_SET_IMMEDIATE(ins,  G_T_UINT32(value.i));
	  edge = CfgEdgeCreate(BBL_CFG(ARM_INS_BBL(ins)),ARM_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),(BBL_FUNCTION(ARM_INS_BBL(ins))==BBL_FUNCTION(T_BBL(RELOC_TO_RELOCATABLE(rel)[0])))?ET_JUMP:ET_IPJUMP);
	  CFG_EDGE_SET_CFG(edge, BBL_CFG(ARM_INS_BBL(ins)));
	  CFG_EDGE_SET_REFCOUNT(edge,1);
	    
          if (exit_bbl)
            comp_edge = CfgEdgeCreateCompensating(BBL_CFG(ARM_INS_BBL(ins)), edge);
	  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
	  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));

	  /* if the newly added edge is interprocedural,
	   * allocate a procstate for it (the cleanup
	   * loop at the end of ConstantPropagation expects
	   * an allocated procstate for all interprocedural
	   * edges) */
	  if (CFG_EDGE_CAT(edge) == ET_IPJUMP)
	  {
	    CFG_EDGE_SET_PROCSTATE(edge,  ProcStateNew(&arm_description));
            if (exit_bbl)
              CFG_EDGE_SET_PROCSTATE(comp_edge,  ProcStateNew(&arm_description));
	    ArmEdgePropagator(edge,ins);
            if (exit_bbl)
              ArmEdgePropagator(comp_edge,ins);
	    /* set all registers to CP_BOT, just to make sure 
	     * we can't do anything wrong should the procstate
	     * information on the edges be used afterwards (shouldn't 
	     * happen though...) */
	    ProcStateSetAllBot(CFG_EDGE_PROCSTATE(edge),arm_description.all_registers);
            if (exit_bbl)
              ProcStateSetAllBot(CFG_EDGE_PROCSTATE(comp_edge),arm_description.all_registers);
	  }

	  VERBOSE(CONST_OPT_VERBOSITY,("@I\n",ins));fflush(stdout);
	}
	else
	{
	  t_cfg_edge * edge;
	  t_bbl * exit_bbl = FunctionGetExitBlock(BBL_FUNCTION(T_BBL(RELOC_TO_RELOCATABLE(rel)[0])));
	  
	  ARM_INS_SET_OPCODE(ins,  ARM_BL);
	  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
	  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  ARM_INS_SET_IMMEDIATE(ins,  G_T_UINT32(value.i));

	  /* There is not necessarily an exit block, so check, and only create a call return pair if it exists */
	  if(exit_bbl != NULL)
	    edge = CfgEdgeCreateCall(BBL_CFG(ARM_INS_BBL(ins)),ARM_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]),link,FUNCTION_BBL_LAST(BBL_FUNCTION(T_BBL(RELOC_TO_RELOCATABLE(rel)[0]))));
	  else
	    edge = CfgEdgeNew(BBL_CFG(ARM_INS_BBL(ins)),ARM_INS_BBL(ins),T_BBL(RELOC_TO_RELOCATABLE(rel)[0]), ET_CALL);


	  ARM_INS_SET_ATTRIB(ins,     ARM_INS_ATTRIB(ins)  |IF_PCDEF);
	  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
	  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
	  ARM_INS_SET_SHIFTLENGTH(ins,  0);
	  ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) | FL_IMMED);
	  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
	  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
	  CFG_EDGE_SET_PROCSTATE(edge,  ProcStateNew(&arm_description));
	  ArmEdgePropagator(edge,ins);
	  if(exit_bbl != NULL)
	  {
	    ArmEdgePropagator(CFG_EDGE_CORR(edge),ins);
	    CFG_EDGE_SET_PROCSTATE(CFG_EDGE_CORR(edge), ProcStateNew(&arm_description));
	  }
	}
      }
      else
      {
	WARNING(("Constant move detected... Weird: @I -> @R",ins,rel));
      }
    }
    else
    {
      VERBOSE(CONST_OPT_VERBOSITY,("InsOpt: found constant move into pc but not tagged! @I: value %d\n",ins,value.i));
    }
  }

  /* always return true, because the instruction is never deleted */
  return TRUE;
}
/* }}} */
/*!
 * Optimizes switches of which the destination is known 
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \todo Verify this for other kinds of switches (gcc-style LDR)
 *
 * \return t_bool FALSE if the optimization killed an instruction 
*/
/* ArmInsOptSwitchOptimizer {{{ */
t_bool ArmInsOptSwitchOptimizer(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  t_cfg_edge * i_edge;
  t_bbl * block;
  t_reloc * reloc;
  
  /* switch instructions are always at the end of a block */
  if (ARM_INS_INEXT (ins))
    return TRUE;

  /* check for outgoing switch edges */
  block = ARM_INS_BBL(ins);
  BBL_FOREACH_SUCC_EDGE(block,i_edge)
    if (CFG_EDGE_CAT(i_edge) == ET_SWITCH)
      break;
  if (!i_edge)
    return TRUE;

  if (ARM_INS_OPCODE(ins)==ARM_T2TBB || ARM_INS_OPCODE(ins)==ARM_T2TBH
      || ARM_INS_OPCODE(ins) == ARM_BL)
    {
      t_register_content content;
      t_reg discriminator = (ARM_INS_OPCODE(ins) == ARM_BL) ? ARM_REG_R0 : ARM_INS_REGC(ins);
      if (ISVALUE(ProcStateGetReg(before,discriminator,&content)))
        {
          t_uint32 val = G_T_UINT32 (content.i);
          t_cfg_edge *edge;
          t_bbl *dest;

          VERBOSE(CONST_OPT_VERBOSITY, ("switch has constant value %d: @I", val, ins));
          /* find the constant destination of the switch */
          BBL_FOREACH_SUCC_EDGE (block, edge)
            {
              if (CFG_EDGE_CAT (edge) == ET_SWITCH || CFG_EDGE_CAT (edge) == ET_IPSWITCH)
                if (CFG_EDGE_SWITCHVALUE (edge) == val)
                  break;
            }
          ASSERT (edge, ("Could not find the switch destination:\n@I value %d", ins, val));
          dest = CFG_EDGE_TAIL (edge);
          
          /* remove all the outgoing edges */
          while ((edge = BBL_SUCC_FIRST (block)))
            {
              if (CFG_EDGE_CORR (edge))
                {
                  CfgEdgeKill (CFG_EDGE_CORR (edge));
                }
              
              CfgEdgeKill (edge);
            }
          
          /* replace switch by unconditional branch */
          ArmInsMakeUncondThumbBranch (ins);
          if (BBL_FUNCTION (block) == BBL_FUNCTION (dest))
            CfgEdgeCreate (BBL_CFG (block), block, dest, ET_JUMP);
          else
            {
              edge = CfgEdgeCreate (BBL_CFG (block), block, dest, ET_IPJUMP);
              CfgEdgeCreateCompensating (BBL_CFG (block), edge);
            }
          return FALSE;
        }
      else
        return TRUE;
    }
  else if (ARM_INS_REGA(ins) != ARM_REG_R15)
  {
    /* the switch instruction has been killed earlier on because it was
     * guaranteed never to be executed => this means we should always take
     * the default case (fallthrough edge), and all switch edges may be
     * deleted */
    /* {{{ */
    t_cfg_edge * tmp;

    VERBOSE(CONST_OPT_VERBOSITY,("Switch at @G always takes fallthrough case\n",ARM_INS_CADDRESS(ins)));

    /* remove all switch edges */
    BBL_FOREACH_SUCC_EDGE_SAFE(block,i_edge,tmp)
    {
      if (CFG_EDGE_CAT(i_edge) == ET_SWITCH)
      {
	CfgEdgeKill(i_edge);
      }
    }

    /* for gcc-style switches: remove the data block containing the jump
     * table for ads-style switches: cut the branch instructions for the
     * switch cases loose from the default case. the branch instructions will
     * be cleaned up later by dead code removal */
    if (BBL_NEXT(block) && BBL_NEXT(BBL_NEXT(block)) && IS_DATABBL(BBL_NEXT(BBL_NEXT(block))))
    {
      /* gcc-style switch table */
      t_bbl * jumptable = T_BBL(BBL_NEXT(BBL_NEXT(block)));
      t_arm_ins * ins;
      BBL_FOREACH_ARM_INS(jumptable,ins)
	ArmInsKillDelayed(ins);
      while (BBL_REFED_BY(jumptable)) 
	RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(block))),RELOC_REF_RELOC(BBL_REFED_BY(jumptable)));
      BblKill(jumptable);
    }
    else
    {
      /* ads-style switch table */
      t_bbl * ft_block;

      VERBOSE(CONST_OPT_VERBOSITY,("switch_opt 1: @I\n", ins));

      BBL_FOREACH_SUCC_EDGE(block,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH)
	  break;
      ASSERT(i_edge,("Switch statement at @G has no fallthrough block!",ARM_INS_CADDRESS(ins)));

      ft_block = CFG_EDGE_TAIL(i_edge);
      BBL_FOREACH_SUCC_EDGE(ft_block,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH)
	  break;
      ASSERT(i_edge,("Switch statement at @G has no case blocks!",ARM_INS_CADDRESS(ins)));

      CfgEdgeKill(i_edge);

      /* now also remove the EF_FROM_SWITCH_TABLE from the jump edge out of the fallthrough block */
      BBL_FOREACH_SUCC_EDGE(ft_block,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_JUMP)
	  break;
      ASSERT(i_edge,("Switch statement at @G has no jump after fallthrough block!",ARM_INS_CADDRESS(ins)));

      CFG_EDGE_SET_FLAGS(i_edge,    CFG_EDGE_FLAGS(i_edge)  &(~EF_FROM_SWITCH_TABLE));
    }
    /* }}} */
  }
  else if (ARM_INS_OPCODE(ins) == ARM_ADD && ARM_INS_SHIFTLENGTH(ins) == 2)
  {
    /* {{{ ads-style switch */
    /* if the value of the switch discriminator is known, we can optimize this */
    t_register_content content;
    t_reg discriminator = ARM_INS_REGC(ins);

    /* Sanity check */
    ASSERT(ARM_INS_OPCODE(ins) == ARM_ADD,("Switch statement with really odd pc-definer: @I. Implement!",ins));
    ASSERT(ARM_INS_REGB(ins) == ARM_REG_R15,("Switch statement with really odd pc-definer: @I. Implement!",ins));
    ASSERT(ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM,("Switch statement with really odd pc-definer: @I. Implement!",ins));
    ASSERT(ARM_INS_SHIFTLENGTH(ins) == 2,("Switch statement with really odd pc-definer: @I. Implement!",ins));

    if (!ProcStateGetReg(before,discriminator,&content))
    {
      /* it's a constant, optimize the switch statement */
      t_cfg_edge * tmp;
      t_bbl * switchblock;
      t_address switchaddr;

      VERBOSE(CONST_OPT_VERBOSITY,("Switch at @G has constant discriminator %d\n",ARM_INS_CADDRESS(ins),content.i));

      /* find the taken switch block */
      switchaddr = AddressAddUint32(ARM_INS_CADDRESS(ins),8+4*G_T_UINT32(content.i));
      BBL_FOREACH_SUCC_EDGE(block,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_SWITCH)
	  if (AddressIsEq(BBL_CADDRESS(CFG_EDGE_TAIL(i_edge)), switchaddr))
	    break;
      ASSERT(i_edge,("Switch at @G doesn't have a case %d!",ARM_INS_CADDRESS(ins),content.i));

      switchblock = CFG_EDGE_TAIL(i_edge);

      /* cut the fallthrough edge from the switch block
       * => all underlying blocks are cut loose so they will be removed by dead block removal */
      BBL_FOREACH_SUCC_EDGE_SAFE(switchblock,i_edge,tmp)
	if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH)
	{
	  CfgEdgeKill(i_edge);
	}

      /* remove the EF_FROM_SWITCH_TABLE from the jump edge out of the switch block */
      BBL_FOREACH_SUCC_EDGE(switchblock,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_JUMP)
	  break;
      ASSERT(i_edge,("Switch case at @G does not have a jump edge!",switchaddr));

      CFG_EDGE_SET_FLAGS(i_edge,     CFG_EDGE_FLAGS(i_edge) &(~EF_FROM_SWITCH_TABLE));

      /* cut all edges from the first block of the switch statement to the switch cases */
      BBL_FOREACH_SUCC_EDGE_SAFE(block,i_edge,tmp)
      {
	CfgEdgeKill(i_edge);
      }

      /* Add a new fallthrough edge from the first block of the switch statement to the taken switch case */
      {
	t_cfg_edge * edge=CfgEdgeCreate(BBL_CFG(block),block,switchblock,ET_FALLTHROUGH);
	CFG_EDGE_SET_CFG(edge, BBL_CFG(block));
	CFG_EDGE_SET_REFCOUNT(edge,1);
      }

      /* Kill the last instruction of the first block of the switch statement (it is no longer needed) */
      ArmInsKillDelayed(ins);
      return FALSE;
    }
    else if (ARM_INS_CONDITION(ins) == ARM_CONDITION_AL)
    {
      /* switch discriminator is not constant, but the switch instruction
       * has been unconditionalized. this means that the value of the 
       * switch statement was always within bounds, so the fallthrough case
       * will never be taken. we should remove the fallthrough edge into the
       * fallthrough case accordingly. */
      t_cfg_edge * edge;

      BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(ins), edge)
        if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH
            || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
        {
          VERBOSE(CONST_OPT_VERBOSITY,("switch_opt 3: @I\n", ins));
          CfgEdgeKill(edge);

          break;
        }

    }
    /* }}} */
  }
  else if (ARM_INS_OPCODE(ins) == ARM_LDR)
  {
    /* gcc-style switches */

    /* if the value of the switch discriminator is known, we can optimize this */
    t_register_content content;
    t_reg discriminator = ARM_INS_REGC(ins);
    t_uint32 multiplier;

    /* if REGC holds an address (i.e., a base address), REGB must be the discriminator */
    if (ProcStateGetTag(before,discriminator,&reloc) != CP_TOP && ProcStateGetTag(before,discriminator,&reloc) != CP_BOT)
      {
        discriminator = ARM_INS_REGB(ins);
      }
      
    if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM && ARM_INS_SHIFTLENGTH(ins) == 2)
      multiplier = 1; // value of discriminator equals values edges
    else 
      multiplier = 4; // value of discriminator equals values edges times 4 (because 4-byte addresses in switch table */

    if (ISVALUE (ProcStateGetReg(before,discriminator,&content)))
    {
      t_uint32 val = G_T_UINT32 (content.i);
      t_cfg_edge *edge;
      t_bbl *dest;

      VERBOSE(CONST_OPT_VERBOSITY, ("switch has constant value %d: @I", val, ins));
      /* find the constant destination of the switch */
      BBL_FOREACH_SUCC_EDGE (block, edge)
      {
	if (CFG_EDGE_CAT (edge) == ET_SWITCH || CFG_EDGE_CAT (edge) == ET_IPSWITCH)
	  if (CFG_EDGE_SWITCHVALUE (edge) * multiplier == val)
	    break;
      }
      ASSERT (edge, (
	    "Could not find the switch destination:\n@I value %d", ins, val));
      dest = CFG_EDGE_TAIL (edge);

      /* remove all the outgoing edges */
      while ((edge = BBL_SUCC_FIRST (block)))
      {
	if (CFG_EDGE_CORR (edge))
	{
	  CfgEdgeKill (CFG_EDGE_CORR (edge));
	}

	CfgEdgeKill (edge);
      }

      /* replace switch by unconditional branch */
      ArmInsMakeUncondBranch (ins);
      if (BBL_FUNCTION (block) == BBL_FUNCTION (dest))
	CfgEdgeCreate (BBL_CFG (block), block, dest, ET_JUMP);
      else
      {
	edge = CfgEdgeCreate (BBL_CFG (block), block, dest, ET_IPJUMP);
	CfgEdgeCreateCompensating (BBL_CFG (block), edge);
      }
      return FALSE;
    } 
    else if (ARM_INS_CONDITION(ins) == ARM_CONDITION_AL)
    {
      /* switch discriminator is not constant, but the switch instruction
       * has been unconditionalized. this means that the value of the 
       * switch statement was always within bounds, so the fallthrough case
       * will never be taken. we should remove the fallthrough edge into the
       * fallthrough case accordingly. */
      t_cfg_edge * edge;

      /* we need an extra check: even if the fallthrough case has 
       * already been removed, we will reach this point again for 
       * every subsequent invocation of ConstantPropagation.
       * Therefore, we need to check if the fallthrough case
       * still exists: this is true if there is no incoming switch edge */
      BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(ins), edge)
	if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH)
	  break;

      if (edge)
      {
	VERBOSE(CONST_OPT_VERBOSITY,("switch_opt 3: @I\n", ins));
	/* it suffices to kill the fallthrough edge, other optimizations
	 * will clean up the rest of the mess */
	CfgEdgeKill(edge);
      }
    }
  }
  return TRUE;
}
/* }}} */
/*!
 * Tries to produce the constant result of a function via a MOV/MVN rXX,#const
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * 
 * \return t_bool always TRUE (this optimization kills no instructions)
*/
/*ArmInsOptEncodeConstantResult {{{ */
t_bool ArmInsOptEncodeConstantResult(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  t_reg regA = ARM_INS_REGA(ins);
  t_register_content value;
  t_uint32 valueA = 0;
  t_lattice_level level;
  t_reloc * reloc;

  if ((ArmInsDefinedRegCount(ins) != 1) || ArmInsIsNOOP(ins)) /* don't try to encode more than one constant in an instruction */
    return TRUE;

  if (regA == ARM_REG_NONE) /* if the defined register isn't stored in the Reg A slot, we can't do anything in this optimization */
    if (ARM_INS_OPCODE(ins) != ARM_LDM) /* only exception: ARM_LDM (see further) */
      return TRUE;

  if (ARM_INS_OPCODE(ins)==ARM_LDM)
    {
      REGSET_FOREACH_REG(RegsetNewFromUint32(ARM_INS_IMMEDIATE(ins)),regA)
	break;
    }

  level = ProcStateGetReg(after,regA,&value);

  if (!level)
    valueA = G_T_UINT32(value.i);
  else
    return TRUE;	/* regA should contain a constant value for this optimization to be possible */

  if (ProcStateGetTag(after,regA,&reloc) != CP_TOP)	/* the constant may not be an address, otherwise this 
							   optimization is not correct! */
    return TRUE;

  switch (ARM_INS_TYPE(ins)) 
  {
    case IT_DATAPROC:
      /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
         by default, not because it is needed, we could do better */ 
      if (!(ARM_INS_FLAGS(ins) & FL_S))
      {
        if (ARM_INS_FLAGS(ins) & FL_THUMB)
          {
                if (ARM_INS_REGA(ins)!=ARM_REG_R13 && ARM_INS_REGA(ins)!=ARM_REG_R15 && ArmIsEncodableConstantForOpcodeThumb(valueA,ARM_MOV) && !((ARM_INS_OPCODE(ins) == ARM_MOV) && (ARM_INS_FLAGS(ins) & FL_IMMED)))
                {
                  /* by default, always create 32-bit Thumb instructions which do not set the flags */
                  VERBOSE(CONST_OPT_VERBOSITY,("replacing T IT_DATAPROC @I",ins));
                  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
                  ARM_INS_SET_OPCODE(ins,  ARM_MOV);
                  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
                  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
                  ARM_INS_SET_IMMEDIATE(ins,  valueA);
                  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
                  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
                  ARM_INS_SET_SHIFTLENGTH(ins,  0);
                  ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | FL_THUMB);
                  /* flag the changes for the assembler */
                  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
                  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
                  ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
                  VERBOSE(CONST_OPT_VERBOSITY,("   by @I",ins));
                }
                // temporary hack: R13 and R15 cannot be destination of MVN
                else if (ARM_INS_REGA(ins)!=ARM_REG_R13 && ARM_INS_REGA(ins)!=ARM_REG_R15 && ArmIsEncodableConstantForOpcodeThumb(~valueA, ARM_MVN) && !((ARM_INS_OPCODE(ins) == ARM_MVN) && (ARM_INS_FLAGS(ins) & FL_IMMED)))
                {
                  VERBOSE(CONST_OPT_VERBOSITY,("replacing T IT_DATAPROC @I", ins));
                  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
                  ARM_INS_SET_OPCODE(ins,  ARM_MVN);
                  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
                  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
                  ARM_INS_SET_IMMEDIATE(ins,  ~valueA);
                  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
                  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
                  ARM_INS_SET_SHIFTLENGTH(ins,  0);
                  ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | FL_THUMB);
                  /* flag the changes for the assembler */
                  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
                  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
                  ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
                  VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
                }
          }
        else
          {
                /* certainly not thumb2 anymore */
                if (ArmIsEncodableConstantForOpcode(valueA,ARM_MOV) && !((ARM_INS_OPCODE(ins) == ARM_MOV) && (ARM_INS_FLAGS(ins) & FL_IMMED)))
                {
                  VERBOSE(CONST_OPT_VERBOSITY,("replacing A IT_DATAPROC @I", ins));
                  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
                  ARM_INS_SET_OPCODE(ins,  ARM_MOV);
                  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
                  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
                  ARM_INS_SET_IMMEDIATE(ins,  valueA);
                  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
                  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
                  ARM_INS_SET_SHIFTLENGTH(ins,  0);
                  ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
                  /* flag the changes for the assembler */
                  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
                  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
                  VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
                }
                else if (ArmIsEncodableConstantForOpcode(~valueA,ARM_MVN) && !((ARM_INS_OPCODE(ins) == ARM_MVN) && (ARM_INS_FLAGS(ins) & FL_IMMED)))
                {
                  VERBOSE(CONST_OPT_VERBOSITY,("replacing A IT_DATAPROC @I", ins));
                  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
                  ARM_INS_SET_OPCODE(ins,  ARM_MVN);
                  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
                  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
                  ARM_INS_SET_IMMEDIATE(ins,  ~valueA);
                  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
                  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
                  ARM_INS_SET_SHIFTLENGTH(ins,  0);
                  ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
                  /* flag the changes for the assembler */
                  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
                  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
                  VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
                }
          }
      }
      break;

    case IT_MUL:
    case IT_LOAD:
      /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
         by default, not because it is needed, we could do better */ 
      if (!(ARM_INS_FLAGS(ins) & FL_S))
      {
        if (ARM_INS_FLAGS(ins) & FL_THUMB)
        {
          if (ARM_INS_REGA(ins)!=ARM_REG_R13 && ARM_INS_REGA(ins)!=ARM_REG_R15 && ArmIsEncodableConstantForOpcodeThumb(valueA,ARM_MOV))
            {
              VERBOSE(CONST_OPT_VERBOSITY,("replacing T IT_MUL/IT_LOAD @I",ins));
              ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
              ARM_INS_SET_OPCODE(ins,  ARM_MOV);
              ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
              ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
              ARM_INS_SET_IMMEDIATE(ins,  valueA);
              ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
              ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
              ARM_INS_SET_SHIFTLENGTH(ins,  0);
              ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | FL_THUMB);
              /* flag the changes for the assembler */
              ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
              ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
              ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
              VERBOSE(CONST_OPT_VERBOSITY,("   by @I",ins));
            }
          else if (ARM_INS_REGA(ins)!=ARM_REG_R13 && ARM_INS_REGA(ins)!=ARM_REG_R15 && ArmIsEncodableConstantForOpcodeThumb(~valueA,ARM_MVN))
            {
              VERBOSE(CONST_OPT_VERBOSITY,("replacing IT_MUL/IT_LOAD @I", ins));
              ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
              ARM_INS_SET_OPCODE(ins,  ARM_MVN);
              ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
              ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
              ARM_INS_SET_IMMEDIATE(ins,  ~valueA);
              ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
              ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
              ARM_INS_SET_SHIFTLENGTH(ins,  0);
              ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | FL_THUMB);
              /* flag the changes for the assembler */
              ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
              ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
              ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
              VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
            }
          VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
	}
        else
        {
          if (ArmIsEncodableConstantForOpcode(valueA,ARM_MOV))
            {
              VERBOSE(CONST_OPT_VERBOSITY,("replacing IT_MUL/IT_LOAD @I", ins));
              ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
              ARM_INS_SET_OPCODE(ins,  ARM_MOV);
              ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
              ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
              ARM_INS_SET_IMMEDIATE(ins,  valueA);
              ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
              ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
              ARM_INS_SET_SHIFTLENGTH(ins,  0);
              ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
              /* flag the changes for the assembler */
              ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
              ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
              VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
            }
          else if (ArmIsEncodableConstantForOpcode(~valueA,ARM_MVN))
            {
              VERBOSE(CONST_OPT_VERBOSITY,("replacing IT_MUL/IT_LOAD @I", ins));
              ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
              ARM_INS_SET_OPCODE(ins,  ARM_MVN);
              ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
              ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
              ARM_INS_SET_IMMEDIATE(ins,  ~valueA);
              ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
              ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
              ARM_INS_SET_SHIFTLENGTH(ins,  0);
              ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
              /* flag the changes for the assembler */
              ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
              ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
              VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
            }
        }
      }
      break;

    case IT_STORE_MULTIPLE:
      break;		/* we can't do anything for STMs */
    case IT_LOAD_MULTIPLE:
      /* I DON'T SEE WHY WE SHOULD COMPUTE THE VALUE BEFORE THIS INSTRUCTION 
      level = ProcStateGetReg(before,regA,&value);
      if (!level)
	valueA = G_T_UINT32(value.i);
      */
      /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
         by default, not because it is needed, we could do better */ 
      if (!level && !(ARM_INS_FLAGS(ins) & FL_S))
      {
	if (ArmIsEncodableConstantForOpcode(valueA,ARM_MOV))
	{
          VERBOSE(CONST_OPT_VERBOSITY,("replacing IT_LOAD_MULTIPLE @I", ins));
          ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
          ARM_INS_SET_OPCODE(ins,  ARM_MOV);
          ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
          ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
          ARM_INS_SET_IMMEDIATE(ins,  valueA);
          ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
          ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
          ARM_INS_SET_SHIFTLENGTH(ins,  0);
          ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | (ARM_INS_FLAGS(ins) & FL_THUMB));
          /* flag the changes for the assembler */
          ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
          ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
          VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
	}
	else if (ArmIsEncodableConstantForOpcode(~valueA,ARM_MVN))
	{
          VERBOSE(CONST_OPT_VERBOSITY,("replacing IT_LOAD_MULTIPLE @I", ins));
          ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
          ARM_INS_SET_OPCODE(ins,  ARM_MVN);
          ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
          ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
          ARM_INS_SET_IMMEDIATE(ins,  ~valueA);
          ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
          ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
          ARM_INS_SET_SHIFTLENGTH(ins,  0);
          ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED | (ARM_INS_FLAGS(ins) & FL_THUMB));
          /* flag the changes for the assembler */
          ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
          ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
          VERBOSE(CONST_OPT_VERBOSITY,("   by @I", ins));
	}

      }
      break;


    case IT_BRANCH:
      /* do nothing, branches always have their immediates encoded in them from the start */


    case IT_SWI:
    case IT_STORE:
    case IT_STATUS:
    case IT_SWAP:
    case IT_DATA:
    case IT_CONSTS:
      break;


    default:
      printf("Instruction can't be changed for now. Implement!\n");
  }

  return TRUE;
}
/* }}} */
/*!
 * use MOV or MVN or ADD or SUB to produce a constant value or address from a known register 
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
*/
/* ArmInsOptProduceConstantFromOtherRegister {{{ */
t_bool ArmInsOptProduceConstantFromOtherRegister(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
#if 1
  t_register_content prod_value;
  t_reloc * prod_reloc;
  t_lattice_level prod_value_level;
  t_lattice_level prod_reloc_level;
  t_reg r,regdef;
  t_regset allowedregs;
  t_bool missed_opportunity;
  t_arm_ins * i_ins;


  
  if (global_optimization_phase == 0)
    allowedregs = ArmInsRegsLiveBefore(ins);
  else
    allowedregs = CFG_DESCRIPTION(ARM_INS_CFG(ins))->all_registers;

  /* there must only be one defined register and the S bit must be cleared */
  /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
     by default, not because it is needed, we could do better */ 
  if ((ARM_INS_FLAGS(ins) & FL_S) ||  (ArmInsDefinedRegCount(ins) != 1))
    return TRUE;
  /* the instruction mustn't have a side effect */
  if ((ARM_INS_TYPE(ins)==IT_STATUS) || (ARM_INS_TYPE(ins) == IT_STORE) ||(ARM_INS_TYPE(ins) == IT_SWAP) || (ARM_INS_OPCODE(ins) == ARM_STM))
    return TRUE;
  /* if the instruction already is a MOV or MVN, we can't do much about it */
  if ((ARM_INS_OPCODE(ins) == ARM_MOV) || (ARM_INS_OPCODE(ins) == ARM_MVN))
    return TRUE;

  
  /* find the defined register */
  REGSET_FOREACH_REG(ARM_INS_REGS_DEF(ins),regdef) break;

  prod_value_level = ProcStateGetReg(after,regdef,&prod_value);
  prod_reloc_level = ProcStateGetTag(after,regdef,&prod_reloc);

  /* the register must contain a value, and the associated relocation mustn't be CP_BOT (CP_TOP is okay) */
  if (prod_value_level || (prod_reloc_level == CP_BOT))
    return TRUE;

  /* to be on the safe side, we only try this optimization with relocations pointing
   * TO_BBL or TO_SUBSECTION */
  if ((!prod_reloc_level) && ((RELOC_N_TO_RELOCATABLES(prod_reloc)==1) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(prod_reloc)[0]) != RT_BBL) && (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(prod_reloc)[0]) != RT_SUBSECTION)))
    return TRUE;

  /* try to find a live register that already contains this value and produce it
   * from there */

  if (diabloanopt_options.rely_on_calling_conventions)
    {
      t_regset meaningfull = RegsetNew();
      RegsetSetDup(meaningfull,BBL_REGS_DEFINED_IN(ARM_INS_BBL(ins)));
      BBL_FOREACH_ARM_INS(ARM_INS_BBL(ins),i_ins)
	{
	  if (i_ins==ins) break;
	  RegsetSetUnion(meaningfull,ARM_INS_REGS_DEF(i_ins));
	}
      
      RegsetSetIntersect(allowedregs,meaningfull);
    }
  missed_opportunity = FALSE;

  REGSET_FOREACH_REG(allowedregs,r)
  {
    t_register_content other_value;
    t_reloc * other_reloc;
    t_lattice_level other_value_level;
    t_lattice_level other_reloc_level;

    /*    if (r == regdef) continue; */

    other_value_level = ProcStateGetReg(before,r,&other_value);
    other_reloc_level = ProcStateGetTag(before,r,&other_reloc);

    /* the register needs to contain a known value of the same type (const or address) of the produced register */
    if (other_value_level || (other_reloc_level != prod_reloc_level)) continue;

    /* the value in the register must of course be equal to the one in the produced register */
    if (!AddressIsEq(prod_value.i,other_value.i)) continue;

    /* last check: if we are looking at addresses, do they point to the same block? */
    if(!prod_reloc_level)
    {
      if(RELOC_N_TO_RELOCATABLES(prod_reloc) > 0 && RELOC_N_TO_RELOCATABLES(other_reloc) > 0)
      {
	if ((RELOC_TO_RELOCATABLE(prod_reloc)[0] != RELOC_TO_RELOCATABLE(other_reloc)[0]) || (RELOC_N_TO_RELOCATABLES(prod_reloc) != RELOC_N_TO_RELOCATABLES(other_reloc)))
	{
	  missed_opportunity = TRUE;
	  continue;
	}
      }
      if(StringCmp(RELOC_CODE(prod_reloc),RELOC_CODE(other_reloc)))
      {
	missed_opportunity = TRUE;
	continue;
      }
    }

    /* we're certain now: change the instruction to a MOV */

    /*    DiabloPrintArch(stdout,ARM_INS_BBL(ins)->fun->cfg->sec->obj->description,"TAKEN %d MOV OPP @I\n @P\n",count,ins,before); */

    {
      VERBOSE(CONST_OPT_VERBOSITY, ("A> before @I", ins));
      ARM_INS_SET_OPCODE(ins,  ARM_MOV);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  r);
      ARM_INS_SET_IMMEDIATE(ins,  0);
      ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) & FL_THUMB);
      ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
      ArmInsFreeReferedRelocsDelayed(ins);
      ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
      ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
      ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
      VERBOSE(CONST_OPT_VERBOSITY, ("A> after @I", ins));
    }

    return TRUE;
  }

  if (missed_opportunity)
    {
      /*      DiabloPrintArch(stdout,ARM_INS_BBL(ins)->fun->cfg->sec->obj->description,"MISSED MOV OPP @I\n @P\n",ins,before); */
    }

  /* for constants (NOT addresses) we can try another option: find a register that contains the exact opposite
   * of the value we want to use, and produce it using a MVN */
  if (prod_reloc_level == CP_TOP)
  {
    REGSET_FOREACH_REG(allowedregs,r)
    {
      t_register_content other_value;
      t_reloc * other_reloc;
      t_lattice_level other_value_level;
      t_lattice_level other_reloc_level;

      /*if (r == regdef) continue;*/

      other_value_level = ProcStateGetReg(before,r,&other_value);
      other_reloc_level = ProcStateGetTag(before,r,&other_reloc);

      /* the register needs to contain a known value that is not an address */
      if (other_value_level || (prod_reloc_level != CP_TOP)) continue;

      /* the value in the register must be the exact opposite of the one in the produced register */
      if (!AddressIsEq(prod_value.i,AddressNot(other_value.i))) continue;

      /* bad registers not allowed in Thumb2 encoding */
      if ((ARM_INS_FLAGS(ins) & FL_THUMB) && ((ARM_INS_REGA(ins)==ARM_REG_R13 || ARM_INS_REGA(ins)==ARM_REG_R15 || r==ARM_REG_R13 || r==ARM_REG_R15)))
        continue;

      /* we're certain now: change the instruction to a MVN */
      VERBOSE(CONST_OPT_VERBOSITY, ("B> before @I", ins));
      /* TODO: Bjorn noticed that often this instruction is immediately rewritten as a mov with an immediate operand */
      ARM_INS_SET_OPCODE(ins,  ARM_MVN);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  r);
      ARM_INS_SET_IMMEDIATE(ins,  0);
      ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) & FL_THUMB);
      ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
      ArmInsFreeReferedRelocsDelayed(ins);
      ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
      ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
      ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
      VERBOSE(CONST_OPT_VERBOSITY, ("B> after @I", ins));

      return TRUE;
    }
  }

  /* try to find a live register that contains a value from which this one can be produced 
   * by an ADD or SUB. Of course, this only make sense if the instruction isn't already of a 
   * similar form. */
  if (!((ARM_INS_TYPE(ins) == IT_DATAPROC) && (ARM_INS_FLAGS(ins) & FL_IMMED)))
  {
    t_reloc * other_reloc;

    missed_opportunity = FALSE;

    REGSET_FOREACH_REG(allowedregs,r)
    {
      t_register_content other_value;
      t_lattice_level other_value_level;
      t_lattice_level other_reloc_level;
      t_uint32 difference;

      other_value_level = ProcStateGetReg(before,r,&other_value);
      other_reloc_level = ProcStateGetTag(before,r,&other_reloc);

      /* the value of the register must be known and of the same type of the produced register (const or address)
       * for the register to be of any use to us */
      if (other_value_level || (other_reloc_level != prod_reloc_level)) continue;

      /* we can only optimize if both relocations point to the same block */
      if (
	  (prod_reloc_level == CP_TOP) || (RELOC_N_TO_RELOCATABLES(prod_reloc) != RELOC_N_TO_RELOCATABLES(other_reloc)) || RELOC_N_TO_RELOCATABLES(prod_reloc) !=1 ||
	  (
	   RELOC_TO_RELOCATABLE(prod_reloc)[0] != RELOC_TO_RELOCATABLE(other_reloc)[0]
#if 0
	   && !(
		RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(prod_reloc, 0))==RT_SUBSECTION && 
		RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(other_reloc, 0))==RT_SUBSECTION &&
		SECTION_PARENT_SECTION(T_SECTION(RELOC_TO_RELOCATABLE(prod_reloc, 0))) == SECTION_PARENT_SECTION(T_SECTION(RELOC_TO_RELOCATABLE(other_reloc, 0)))
		)
#endif
	   )
	  )
	{
	  /*	  printf("types: %d %d\n",RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(prod_reloc, 0)),RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(other_reloc, 0)));*/
	  difference = G_T_UINT32(prod_value.i) - G_T_UINT32(other_value.i);
	  if (ArmInsIsEncodableConstantForOpcode(difference,ARM_ADD, (ARM_INS_FLAGS(ins) & FL_THUMB)))
	    {
	      /*	      VERBOSE(CONST_OPT_VERBOSITY,("a    1: @R\na    2: @R\n",prod_reloc,other_reloc));*/
	      missed_opportunity = TRUE;
	    }
	  else if (ArmInsIsEncodableConstantForOpcode(-difference,ARM_SUB, (ARM_INS_FLAGS(ins) & FL_THUMB)))
	    {
	      /*	      VERBOSE(CONST_OPT_VERBOSITY,("s    1: @R\ns    2: @R\n",prod_reloc,other_reloc));*/
	      missed_opportunity = TRUE;
	    }
	  continue;
	}
      

      difference = G_T_UINT32(prod_value.i) - G_T_UINT32(other_value.i);

      if (ArmInsIsEncodableConstantForOpcode(difference,ARM_ADD, (ARM_INS_FLAGS(ins) & FL_THUMB)))
      {
	/*	DiabloPrintArch(stdout,ARM_INS_BBL(ins)->fun->cfg->sec->obj->description,"TAKEN %d ADD OPP @I\n @P\n",count,ins,before); */
	/* encode with an add */
        VERBOSE(CONST_OPT_VERBOSITY, ("C> before @I", ins));
        ARM_INS_SET_OPCODE(ins,  ARM_ADD);
        ARM_INS_SET_REGB(ins,  r);
        ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
        ARM_INS_SET_IMMEDIATE(ins,  difference);
        ARM_INS_SET_FLAGS(ins,  FL_IMMED | (ARM_INS_FLAGS(ins) & FL_THUMB));
        ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
        ArmInsFreeReferedRelocsDelayed(ins);
        ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
        ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
        ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
        ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
        ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
        missed_opportunity = FALSE;
        VERBOSE(CONST_OPT_VERBOSITY, ("C> after @I", ins));
      }
      else if (ArmInsIsEncodableConstantForOpcode(-difference,ARM_SUB, (ARM_INS_FLAGS(ins) & FL_THUMB)))
      {
	/*	DiabloPrintArch(stdout,ARM_INS_BBL(ins)->fun->cfg->sec->obj->description,"TAKEN %d SUB OPP @I\n @P\n",count,ins,before); */
	/* encode with a sub */
        VERBOSE(CONST_OPT_VERBOSITY, ("D> before @I", ins));
        ARM_INS_SET_OPCODE(ins,  ARM_SUB);
        ARM_INS_SET_REGB(ins,  r);
        ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
        ARM_INS_SET_IMMEDIATE(ins,  -difference);
        ARM_INS_SET_FLAGS(ins,  FL_IMMED | (ARM_INS_FLAGS(ins) & FL_THUMB));
        ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
        ArmInsFreeReferedRelocsDelayed(ins);
        ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
        ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
        ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
        ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
        ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
        missed_opportunity = FALSE;

        VERBOSE(CONST_OPT_VERBOSITY, ("D> after @I", ins));
      }
      else
      {
	/* no encoding possible, let's continue to the next register */
	missed_opportunity = TRUE;
	continue;
      }

      /* if we get here, the address producer has been changed to an add or sub so we 
       * don't need to continue looking for a good value. we can just return from here.
       */
      return TRUE;
    }

    if (missed_opportunity)
      {
/*	DiabloPrintArch(stdout,ARM_INS_BBL(ins)->fun->cfg->sec->obj->description,"MISSED ADD OR SUB OPP @I\n @P\n",ins,before); 
	if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(other_reloc)) == RT_SUBSECTION &&
	    SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(other_reloc)) && SECTION_NAME(SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(other_reloc))))
	  VERBOSE(0,("Section: %s\n",SECTION_NAME(SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(other_reloc)))));
	if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(prod_reloc)) == RT_SUBSECTION &&
	    SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(prod_reloc)) && SECTION_NAME(SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(prod_reloc))))
	  VERBOSE(0,("Prod Section: %s\n",SECTION_NAME(SECTION_PARENT_SECTION(RELOC_TO_RELOCATABLE(prod_reloc))))); */
      }
    
  }

#endif
  /* if we get here, no optimization was possible so we just return */
  return TRUE;
}
/* }}} */
/*!
 *  tries to encode operands whose value are known into the immediate operand of the instruction
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \return t_bool always TRUE (this optimization kills no instructions)
*/
/* ArmInsOptEncodeConstantOperands {{{ */
t_bool ArmInsOptEncodeConstantOperands(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  t_bool retrying_with_switched_operands = FALSE; 
  t_reg regB = ARM_INS_REGB(ins);
  t_reg regC = ARM_INS_REGC(ins);
  t_reg regS = ARM_INS_REGS(ins);

  t_register_content value;
  t_uint32 valueB = 0;
  t_lattice_level levelB=CP_BOT;

  t_uint32 valueC = 0;
  t_lattice_level levelC=CP_BOT;

  t_uint32 valueS = 0;
  t_lattice_level levelS=CP_BOT;
  t_reloc * relocB;
  t_reloc * relocC;
  t_reloc * relocS;
  t_lattice_level level_relocB=CP_BOT;
  t_lattice_level level_relocC=CP_BOT;
  t_lattice_level level_relocS=CP_BOT;

  t_bool encodable = FALSE;

  /* don't do anything for NOOPs and instructions that already have an immediate operand */

  if (ArmInsIsNOOP(ins)) return TRUE;
  if (regC == ARM_REG_NONE) return TRUE;

retry_with_switched_operands:
  if (regB!=ARM_REG_NONE)
  {
    levelB = ProcStateGetReg(before,regB,&value);
    if (!levelB)
      valueB = G_T_UINT32(value.i);
    level_relocB = ProcStateGetTag(after,regB,&relocB);

  }

  /*if (regC!=ARM_REG_NONE)*/
  {
    levelC = ProcStateGetReg(before,regC,&value);
    if (!levelC)
      valueC = G_T_UINT32(value.i);
    level_relocC = ProcStateGetTag(after,regC,&relocC);
  }

  if (regS!=ARM_REG_NONE)
  {
    levelS = ProcStateGetReg(before,regS,&value);
    level_relocS = ProcStateGetTag(after,regS,&relocS);
  }
  else
  {
    level_relocS = CP_TOP;
  }

  switch (ARM_INS_TYPE(ins)) 
  {
    case IT_DATAPROC:
      /* Calculate the second operand */
      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
      {
	levelS = ArmInsExtractShift(ins,before,&value);
	
	if (!levelS)
	  valueS = G_T_UINT32(value.i);

	if (level_relocC != CP_TOP)
	{
	  if (level_relocS != CP_TOP)
	    level_relocS = CP_BOT;
	  else
	    level_relocS = level_relocC;
	}
	
      }
      else
      {
	valueS=valueC;
	levelS=levelC;
	level_relocS=level_relocC;
      }

      /* At this point levelS,valueS,level_relocS hold the second
       * operand used in the data processing instruction (shifts are
       * removed) */

      /* If we know the second operand and the value is encodable and it is not a relocatable value */

      if (!levelS && ArmInsIsEncodableConstantForOpcode(valueS,ARM_INS_OPCODE(ins), ARM_INS_FLAGS(ins) & FL_THUMB) && level_relocS==CP_TOP)
      {

	/* If the instruction does not have the S bit or it doesn't
	 * have a shifted flexible .... We can encode it if it does
	 * not have an encode flexible or if it has no FL_S bit*/

        /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
           by default, not because it is needed, we could do better */ 


	if (((!(ARM_INS_FLAGS(ins) & FL_S)) || (!ArmInsHasShiftedFlexible(ins))) && (!(ARM_INS_FLAGS(ins) & FL_THUMB) || (ARM_INS_REGA(ins)!=ARM_REG_R13 && ARM_INS_REGA(ins)!=ARM_REG_R15)))
	{
          VERBOSE(CONST_OPT_VERBOSITY, ("A> before @I", ins));
          /* incorporate the operand in the instruction */
          ARM_INS_SET_IMMEDIATE(ins,  valueS);
          ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins)| FL_IMMED);
          ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
          ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
          ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
          ARM_INS_SET_SHIFTLENGTH(ins,  0);
          ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
          ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
          VERBOSE(CONST_OPT_VERBOSITY, ("A> after @I", ins));
	}
      }
     break;

    case IT_STORE:
    case IT_LOAD:
      /* Calculate the second operand */
      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
      {
	levelS  = ArmInsExtractShift(ins,before,&value);
	if (!levelS)
	  valueS = G_T_UINT32(value.i);
      }
      else
      {
	valueS=valueC;
	levelS=levelC;
	level_relocS = level_relocC;
      }

      if (!levelS &&
	  ArmInsIsEncodableConstantForOpcode(valueS,ARM_INS_OPCODE(ins), ARM_INS_FLAGS(ins) & FL_THUMB) &&
	  level_relocS==CP_TOP)
      {
	/* don't do this for gcc-style switches. We have another
	 * constant optimization that does this, and that is guaranteed
	 * to be correct. This one isn't */
	if (ARM_INS_REGB (ins) != ARM_REG_R15)
	{
          VERBOSE(CONST_OPT_VERBOSITY, ("B> before @I", ins));
          /* incorporate the operand in the instruction */
          /* be influenced by our changing of the second operand */
          ARM_INS_SET_IMMEDIATE(ins,  valueS);
          ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) | FL_IMMED);
          ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
          ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
          ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
          ARM_INS_SET_SHIFTLENGTH(ins,  0);
          ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
          ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
          ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable @I", ins));
          VERBOSE(CONST_OPT_VERBOSITY,("9 Changed to: @I\n",ins));
          VERBOSE(CONST_OPT_VERBOSITY, ("B> after @I", ins));
	}
      } 
      break;

    case IT_MUL:
      if ((!levelC)||(!levelB))
      {
	if (!levelC) 
	{ 
	  if ((ARM_INS_OPCODE(ins)==ARM_MUL) && (valueC==1))
	  {
            VERBOSE(CONST_OPT_VERBOSITY, ("C> before @I", ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),ARM_INS_REGB(ins),0,ARM_INS_CONDITION(ins));
            VERBOSE(CONST_OPT_VERBOSITY, ("C> after @I", ins));
	  }
	  else if ((ARM_INS_OPCODE(ins)==ARM_MUL || ARM_INS_OPCODE(ins)==ARM_UMULL) && (valueC==0))
	  {
            VERBOSE(CONST_OPT_VERBOSITY, ("D> before @I", ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),ARM_REG_NONE,0,ARM_INS_CONDITION(ins));
            VERBOSE(CONST_OPT_VERBOSITY, ("D> after @I", ins));
	  }
	  else
	  {
	  }
	}
	else if (!levelB)  
	{
	  if ((ARM_INS_OPCODE(ins)==ARM_MUL) && (valueB==1))
          {
            VERBOSE(CONST_OPT_VERBOSITY, ("E> before @I", ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),ARM_INS_REGC(ins),0,ARM_INS_CONDITION(ins));
            VERBOSE(CONST_OPT_VERBOSITY, ("E> after @I", ins));
	  }
	  else if ((ARM_INS_OPCODE(ins)==ARM_MUL || ARM_INS_OPCODE(ins)==ARM_UMULL) && (valueB==0))
	  {
            VERBOSE(CONST_OPT_VERBOSITY, ("F> before @I", ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),ARM_REG_NONE,0,ARM_INS_CONDITION(ins));
            VERBOSE(CONST_OPT_VERBOSITY, ("F> after @I", ins));
	  }
	  else if ((ARM_INS_OPCODE(ins)==ARM_MLA) && (valueB==0))
	  {
            VERBOSE(CONST_OPT_VERBOSITY, ("G> before @I", ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),ARM_INS_REGS(ins),0,ARM_INS_CONDITION(ins));
            VERBOSE(CONST_OPT_VERBOSITY, ("G> after @I", ins));
	  }
	  else
	  {
	  }
	}
      }
      break;
    default:
      if ((!levelC)||(!levelB))
      {
        if (!levelC)
          {
            VERBOSE(CONST_OPT_VERBOSITY,("A >>>>>>>> @I C:%d\n",ins,valueC));
          }
        if (!levelB)
          {
            VERBOSE(CONST_OPT_VERBOSITY,("A >>>>>>>> @I B:%d\n",ins,valueB));
          }
      }
      /* do nothing because none of the other instructions offer possibilities for optimisation */
      break;
  }

  /* For commutative instructions without immediate operand or shifted operand:
   * 	try to switch regB and regC and retry the previous optimization */
  if (!retrying_with_switched_operands && ArmInsIsCommutative(ins))
  {
    if (!ArmInsHasShiftedFlexible(ins))
    {
      /* regB needs to be constant, and may not contain a relocation */
      if (!levelB && (level_relocB == CP_TOP))
      {
	/* swap regB and regC */
	if (ARM_INS_REGC(ins)!=ARM_REG_NONE) 
	{
          t_reg tmp = ARM_INS_REGC(ins);
          ARM_INS_SET_REGC(ins,  ARM_INS_REGB(ins));
          ARM_INS_SET_REGB(ins,  tmp);
          VERBOSE(CONST_OPT_VERBOSITY,("-~-~- Swapped operands: ins now @I\n",ins));
          retrying_with_switched_operands = TRUE;
          goto retry_with_switched_operands;
	}
      }
    }
  }
  return TRUE;
}
/* }}} */
/*!
 *  tries to encode a constant in two instructions if encoding it in one instructions isn't possible
 *
 * \param ins The instruction to optimize
 * \param before The register state before executing the instruction
 * \param after The register state after executing the instruction
 *
 * \return t_bool always TRUE (this optimization kills no instructions)
*/
/* ArmInsOptEncodeConstantResultInTwoInstructions {{{ */
t_bool ArmInsOptEncodeConstantResultInTwoInstructions(t_arm_ins * ins, t_procstate * before, t_procstate * after)
{
  t_reg regdef;
  t_register_content content;
  t_uint32 value = 0;
  t_lattice_level level;
  t_reloc * reloc;

  /* don't try to do this in the first optimization phase - it potentially increases program size */
  if (global_optimization_phase == 0)
    return TRUE;

  if ((ArmInsDefinedRegCount(ins) != 1) || ArmInsIsNOOP(ins)) /* don't try to encode more than one constant in an instruction */
    return TRUE;

  /* TODO THUMB2: in case the status bits are not live, which may often be the case in THUMB2 code because there the FL_S bit is mostly set 
     by default, not because it is needed, we could do better */ 

  if (ARM_INS_FLAGS(ins) & FL_S) /* don't do this for instructions that set the S bit: potentially dangerous */
    return TRUE;

  /* find the defined register */
  REGSET_FOREACH_REG(ARM_INS_REGS_DEF(ins),regdef) break;

  /* the instruction mustn't have a side effect */
  if ((ARM_INS_TYPE(ins)==IT_STATUS) || (ARM_INS_TYPE(ins) == IT_STORE) ||(ARM_INS_TYPE(ins) == IT_SWAP) || (ARM_INS_OPCODE(ins) == ARM_STM))
    return TRUE;

  level = ProcStateGetReg(after,regdef,&content);
  if (!level)
    value = G_T_UINT32(content.i);
  else
    return TRUE;	/* regdef should contain a constant value for this optimization to be possible */
  
  if (ProcStateGetTag(after,regdef,&reloc) != CP_TOP)	/* the constant may not be an address, otherwise this 
							   optimization is not correct! */
    return TRUE;

  /* don't do this for instructions of the form MOV or MVN or DATAPROC rX,rY,#constant */
  if ((ARM_INS_OPCODE(ins) == ARM_MOV) || (ARM_INS_OPCODE(ins) == ARM_MVN))
    return TRUE;
  if ((ARM_INS_TYPE(ins) == IT_DATAPROC) && (ARM_INS_FLAGS(ins) & FL_IMMED))
    return TRUE;

  /* try to do it... */
  if (ArmConstantIsProducableInTwoInstructions(value))
  {
    VERBOSE(CONST_OPT_VERBOSITY,("--- Encoding %x in 2 instructions (orig: @I)\n",value,ins));
    ArmEncodeConstantInTwoInstructions(ins, value);
  }
  return TRUE;
}
/* }}} */


t_bool ArmOptMoveNullIntoPC(t_arm_ins * ins)
{
  if (ARM_INS_OPCODE(ins) == ARM_MOV && 
      ARM_INS_REGA(ins)==ARM_REG_R15 && 
      ARM_INS_REGB(ins)==ARM_REG_NONE &&  
      ARM_INS_REGC(ins)==ARM_REG_NONE && 
      (!ArmInsIsConditional(ins)) &&
      ARM_INS_IMMEDIATE(ins)==0)
  {
    t_bbl * bbl = ARM_INS_BBL(ins), *i_bbl, *j_bbl;
    t_cfg_edge * edge, *safe, *i_edge;
    t_cfg * cfg = BBL_CFG(ARM_INS_BBL(ins));
    t_function * i_fun;

    VERBOSE(CONST_OPT_VERBOSITY,("killing pred chain to @ieB\n",bbl));fflush(stdout);

    BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,safe)
    {
      if (CFG_EDGE_CORR(edge))
      {
        CfgEdgeKill(CFG_EDGE_CORR(edge));
      }

      CfgEdgeKill(edge);
    }

        VERBOSE(CONST_OPT_VERBOSITY,("\nTrying to kill some preds of @eiB",bbl));

    CfgUnmarkAllFun(cfg);

    CFG_FOREACH_FUN(cfg,i_fun)
    {
      FunctionUnmarkAllEdges(i_fun);
      FunctionUnmarkAllBbls(i_fun);
    }

    FunctionMarkBbl(BBL_FUNCTION(bbl),bbl);
    CfgMarkFun(cfg,BBL_FUNCTION(bbl));

    while(CfgUnmarkFun(cfg,&i_fun))
    {
      while(FunctionUnmarkBbl(i_fun,&i_bbl))
      {
        VERBOSE(CONST_OPT_VERBOSITY,("Looking at preds of @B",i_bbl));

        BBL_FOREACH_PRED_EDGE_SAFE(i_bbl,edge,safe)
        {
          t_uint32 nsuccs = 0;
          j_bbl = CFG_EDGE_HEAD(edge);

          if (CFG_EDGE_CAT(edge)==ET_RETURN)
            j_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(edge));

          BBL_FOREACH_SUCC_EDGE(j_bbl,i_edge)
          {
            nsuccs++;
            if(nsuccs > 1) break;
          }

          if (!nsuccs) 
            FATAL(("No successors? @iB",j_bbl));

          if( nsuccs == 1)
          {
            if (CFG_EDGE_CORR(edge))
            {
              CfgEdgeKill(CFG_EDGE_CORR(edge));
            }
            CfgEdgeKill(edge);
            VERBOSE(CONST_OPT_VERBOSITY,("killed single succ of @eiB\n",j_bbl));
            FunctionMarkBbl(BBL_FUNCTION(j_bbl),j_bbl);
            if (BBL_FUNCTION(j_bbl)!=BBL_FUNCTION(i_bbl))
              CfgMarkFun(cfg,BBL_FUNCTION(j_bbl));
          }
          else if (nsuccs == 2)
          {

            if(BBL_INS_LAST(j_bbl))
            {
              if(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(j_bbl))) != ARM_CONDITION_AL)
              {
                if(CFG_EDGE_CAT(edge) == ET_FALLTHROUGH)
                {
                  t_arm_ins * cond_ins = ARM_INS_IPREV(T_ARM_INS(BBL_INS_LAST(j_bbl)));
                  while(cond_ins)
                  {
                    ARM_INS_SET_CONDITION(cond_ins,  ARM_CONDITION_AL);
                    if(ARM_INS_IPREV(cond_ins) && ARM_INS_CONDITION(ARM_INS_IPREV(cond_ins)) == ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(j_bbl))))
                      cond_ins = ARM_INS_IPREV(cond_ins);
                    else break;
                  }
                  VERBOSE(CONST_OPT_VERBOSITY,("Gonna unconditionalize this one: @I",BBL_INS_LAST(j_bbl)));
                  ARM_INS_SET_CONDITION(T_ARM_INS(BBL_INS_LAST(j_bbl)),  ARM_CONDITION_AL);
                  VERBOSE(CONST_OPT_VERBOSITY,("5 Killing @E",edge));
                  CfgEdgeKill(edge);
                }
                else if(CFG_EDGE_CAT(edge) == ET_JUMP)
                {
                  t_arm_ins * cond_ins = ARM_INS_IPREV(T_ARM_INS(BBL_INS_LAST(j_bbl))), *prev = T_ARM_INS(BBL_INS_LAST(j_bbl));
                  while(cond_ins)
                  {
                    if(ARM_INS_CONDITION(cond_ins) == ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(j_bbl))))
                    {
                      prev = cond_ins;
                      cond_ins = ARM_INS_IPREV(cond_ins);
                      VERBOSE(CONST_OPT_VERBOSITY,("killing ins @I\n",prev));
                      ArmInsKill(prev);
                    }
                    else break;
                  }
                  VERBOSE(CONST_OPT_VERBOSITY,("killing ins @I\n",prev));
                  ArmInsKill(T_ARM_INS(BBL_INS_LAST(j_bbl)));
                  VERBOSE(CONST_OPT_VERBOSITY,("4 Killing @E",edge));
                  CfgEdgeKill(edge);

                }
                else if(CFG_EDGE_CAT(edge) == ET_IPJUMP)
                  FATAL(("implement me\n"));
                else if(CFG_EDGE_CAT(edge) == ET_CALL)
                  FATAL(("implement me\n"));
                else
                  FATAL(("This bbl has 2 successors, what shall we do? @ieB",j_bbl));
              }
              else
                FATAL(("Not conditional? @eiB",j_bbl));
            }
            else
            {
              if(CFG_EDGE_CAT(edge) == ET_RETURN)
              {
                FATAL(("should not happen \n"));
              }
              else if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
              {
                VERBOSE(CONST_OPT_VERBOSITY,("BEFORE @ieB\n",i_bbl));
                while (BBL_REFED_BY(i_bbl))
                {
                  t_reloc * rel = RELOC_REF_RELOC(BBL_REFED_BY(i_bbl));
                  t_relocatable * relocatable = RELOC_FROM(rel);
                  if (RELOCATABLE_RELOCATABLE_TYPE(relocatable)==RT_INS)
                  {
                    VERBOSE(CONST_OPT_VERBOSITY,( "killing @I\n",relocatable));
                    ArmInsKill(T_ARM_INS(relocatable));
                  }
                  else
                  {
                    RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),rel);
                  }
                }
                VERBOSE(CONST_OPT_VERBOSITY,("AFTER @ieB\n",i_bbl));
                continue;
              }
              else
                FATAL(("This bbl has no instructions, what to do? @eiB",j_bbl));
            }

          }

        }
      }
    }
    VERBOSE(CONST_OPT_VERBOSITY,("inserting edge @E\n",CfgEdgeCreate(FUNCTION_CFG(BBL_FUNCTION(bbl)),bbl,bbl,ET_JUMP)));
  }

  return TRUE;

}
/* vim: set shiftwidth=2 foldmethod=marker : */
