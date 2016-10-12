/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>

#define COPYPROPINFO_VERBOSITY 10

/*#include <lancet_dialogs.h>*/
/*!
 * For a copy instruction, this function finds all uses of the copy in the
 * basic block and renames them to the original register whenever possible. 
 *
 * \param j_ins The instruction following the copy instruction
 * \param copy The copy of the register 
 * \param copied The original register
 *
 * \return int (0) if we can keep renaming in the following blocks, else (-1) 
*/
/* ArmOptCopyInner {{{ */
static int ArmOptCopyInner (t_arm_ins * j_ins, t_reg copy, t_reg copied)
{
  static int debug = 0;
  t_reg bkreg = ARM_REG_NONE;

  for (; j_ins; j_ins = ARM_INS_INEXT (j_ins))
  {
    /* for doubleword loads/stores, we would have to propagate two registers */
    if ((ARM_INS_OPCODE(j_ins) == ARM_LDRD || (ARM_INS_OPCODE(j_ins) == ARM_STRD)) &&
        ((ARM_INS_REGA(j_ins) == copy) || ((ARM_INS_REGA(j_ins)+1) == copy)))
      return -1;

    /* note: multiply instructions have some restrictions on which registers may
     * be used as arguments, so we treat them specially */

    if ((ARM_INS_TYPE(j_ins) != IT_MUL) &&
	(!ArmInsWriteBackHappens(j_ins) || ARM_INS_REGB(j_ins) != copy))
    {
      if ((ARM_INS_REGB(j_ins) == copy)&&(ARM_INS_REGB(j_ins) != ARM_REG_R13)
          && !((ARM_INS_OPCODE(j_ins)==ARM_T2CBZ || ARM_INS_OPCODE(j_ins)==ARM_T2CBNZ) && copied>=ARM_REG_R8))
      {
	VERBOSE (1, ("@I: regb -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGB(j_ins);
	ARM_INS_SET_REGB(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
	       ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
	       debug++;
        }
        else
        {
                ARM_INS_SET_REGB(j_ins, bkreg);
        }
      }

      if (ARM_INS_REGC(j_ins)==copy && !ArmInsIsNOOP(j_ins) &&(ARM_INS_REGC(j_ins) != ARM_REG_R13))
      {
	VERBOSE (1, ("@I: regc -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGC(j_ins);
        ARM_INS_SET_REGC(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGC(j_ins, bkreg);
        }
        /* Got to be careful here: if the instruction turns in a no-op
         * something bad happens */
        if ((ARM_INS_OPCODE(j_ins)==ARM_MOV)&&(ARM_INS_REGA(j_ins)==copied))
          ARM_INS_SET_REGS_DEF(j_ins,  ArmDefinedRegisters(j_ins));
      }
      if ((ARM_INS_REGS(j_ins)==copy)&&(ARM_INS_REGS(j_ins) != ARM_REG_R13))
      {
	VERBOSE (1, ("@I: regs -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGS(j_ins);
        ARM_INS_SET_REGS(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGS(j_ins, bkreg);
        }
      }

      if ((ARM_INS_TYPE(j_ins)==IT_STORE) && (ARM_INS_REGA(j_ins)==copy) && (ARM_INS_REGA(j_ins) != ARM_REG_R13))
      {
	VERBOSE (1, ("@I: rega -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGA(j_ins);
        ARM_INS_SET_REGA(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGA(j_ins, bkreg);
        }
      }
    }

    if(ARM_INS_OPCODE(j_ins) == ARM_MUL || ARM_INS_OPCODE(j_ins) == ARM_MLA || ARM_INS_OPCODE(j_ins) == ARM_MLS)
    {
      if(ARM_INS_REGB(j_ins) == copy && ARM_INS_REGA(j_ins) != copied)
      {
	VERBOSE (1, ("@I: regb -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGB(j_ins);
        ARM_INS_SET_REGB(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGB(j_ins, bkreg);
        }
      }
      else if (ARM_INS_REGB(j_ins) == copy
	  && ARM_INS_REGC(j_ins) != copy
	  && ARM_INS_REGC(j_ins) != ARM_INS_REGA(j_ins))
      {
	VERBOSE (1, ("@I: regc -> r%d\n (switch reg b and c)", j_ins, copied));
	ARM_INS_SET_REGB(j_ins,  ARM_INS_REGC(j_ins));
        bkreg = ARM_INS_REGC(j_ins);
        ARM_INS_SET_REGC(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGC(j_ins, bkreg);
        }
      }
      if(ARM_INS_REGC(j_ins) == copy)
      {
	VERBOSE (1, ("@I: regc -> r%d\n", j_ins, copied));
        bkreg = ARM_INS_REGC(j_ins);
        ARM_INS_SET_REGC(j_ins,  copied);
        if (ArmInsIsEncodable(j_ins))
        {
               ARM_INS_SET_REGS_USE(j_ins,  ArmUsedRegisters(j_ins));
               debug++;
        }
        else
        {
                ARM_INS_SET_REGC(j_ins, bkreg);
        }
      }
    }

    if (RegsetIn(ARM_INS_REGS_DEF(j_ins),copy) ||
	RegsetIn(ARM_INS_REGS_DEF(j_ins),copied))
      return -1;

    /*if (old_debug != debug) VERBOSE(0,("Changed @I",j_ins));*/
  }
  return 0;
}
/* }}} */
/*!
 * Tries to follow all paths and applies ArmCopyInner on all basic blocks on the path 
 *
 * \param i_bbl a bbl in which copy propagation was already applied.
 * \param copy The copy of the register 
 * \param copied The original register
 *
 * \return void 
*/
/* ArmOptCopyRecurse {{{ */
static void ArmOptCopyRecurse(t_bbl * i_bbl, t_reg copy, t_reg copied)
{
  t_cfg * cfg=BBL_CFG(i_bbl);
  t_cfg_edge * edge,* edge2;
  t_uint32 real_preds;
  if (BblIsMarked(i_bbl)) return;
  BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
   {
      CfgEdgeMark(edge);

      /* don't propagate over function calls: this might change the callee-saved registers
       * of the function we propagate the constant into, and we can't handle that at the moment */
      if (diabloanopt_options.rely_on_calling_conventions)
	if (CFG_EDGE_CAT(edge) == ET_CALL)
	  {
	    continue;
	  }
      
      if (EquationIsTop(BBL_EQS_IN(CFG_EDGE_TAIL(edge))[0]))
        continue;

#if 0
      if (EDGE_CAT(edge)==ET_CALL || EDGE_CAT(edge)==ET_SWI) 
	if (EDGE_CORR(edge))
      {
	continue;
	if ((FUN_REGS_SAVED(BBL_FUNCTION(EDGE_TAIL(edge))) & (1<<copy) ) && (FUN_REGS_SAVED(BBL_FUNCTION(EDGE_TAIL(edge))) & (1<<copied) ) )
	  continue;
	real_preds=0;
	if(diabloanopt_options.copy_analysis)// &&BBL_EQS_IN(CFG_EDGE_TAIL(edge)))
	/* We have extra information from extended copy-analysis*/
	{
	  if(EquationsRegsEqual(BBL_EQS_IN(CFG_EDGE_TAIL(EDGE_CORR(edge))),copy,copied,0) == TRUE)
	  {
	    NodeMark(i_bbl);
	    if (!ArmOptCopyInner(BBL_INS_FIRST(CFG_EDGE_TAIL(EDGE_CORR(edge))),copy,copied))
	    {
	      ArmOptCopyRecurse(CFG_EDGE_TAIL(EDGE_CORR(edge)),copy,copied);
	    }
	  }
	}
	else
	{
	  t_cfg_edge * edge2;
	  BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(EDGE_CORR(edge)),edge2)
	    {
	      /* Fun link edges don't count as predecessor edges for copy propagation */
	      if (EDGE_CAT(edge2)==ET_SWITCH)  continue;
	      if (EDGE_CAT(edge2)==ET_RETURN)  continue;
	      if (EdgeIsMarked(edge2)) continue;
	      real_preds++;
	    }
	  if (real_preds==0)
	  {
	    NodeMark(i_bbl);
	    if (!ArmOptCopyInner(BBL_INS_FIRST(CFG_EDGE_TAIL(EDGE_CORR(edge))),copy,copied))
	    {
	      ArmOptCopyRecurse(CFG_EDGE_TAIL(EDGE_CORR(edge)),copy,copied);
	    }
	  }
	}
      }
#endif

      {
	if(diabloanopt_options.copy_analysis)/* && BBL_EQS_IN(CFG_EDGE_TAIL(edge)))*/
	/* We have extra information from extended copy-analysis*/
	{
	  if(EquationsRegsEqual(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(edge)),copy,copied,0,NULL,NULL) == TRUE)
	  {
	    BblMark(i_bbl);
	    if (!ArmOptCopyInner(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge))),copy,copied))
	    {
	      ArmOptCopyRecurse(CFG_EDGE_TAIL(edge),copy,copied);
	    }
	  }
	}
	else
	{
	  real_preds=0;
	  BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(edge),edge2)
	  {
	    /* Fun link edges don't count as predecessor edges for copy propagation */
	    if (CFG_EDGE_CAT(edge2)==ET_SWITCH)  continue;
	    if (CfgEdgeIsMarked(edge2)) continue;
	    real_preds++;
	  }
	  if (real_preds==0)
	  {
	    BblMark(i_bbl);
	    if (!ArmOptCopyInner(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge))),copy,copied))
	    {
	      ArmOptCopyRecurse(CFG_EDGE_TAIL(edge),copy,copied);
	    }
	  }
	}
      }
   }
}
/* }}} */
/*!
 * Copy propagation for the arm. This optimization tries to eliminate
 * unnecessary register copies. Calls ArmOptCopyInner to find a candidate and
 * rename copies in one bbl and calls ArmOptCopyRecurse to rename copies in the
 * following blocks.
 *
 * \param cfg The flowgraph to work on
 *
 * \return void 
*/
/* ArmOptCopyPropagation {{{ */
void ArmOptCopyPropagation(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_arm_ins * i_ins, *j_ins;
  t_reg copied;
  t_reg copy;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (!BBL_FUNCTION(i_bbl)) continue;

    BBL_FOREACH_ARM_INS(i_bbl,i_ins)
    {
      if (!CFG_DESCRIPTION(cfg)->IsCopy(T_INS(i_ins),&copy,&copied)) continue;
      if(copied == ARM_REG_R13) continue;
      j_ins=ARM_INS_INEXT(i_ins);

      /*DiabloPrint(stdout,"LOOKING FOR COPY @I\n",i_ins);*/

      if (ArmOptCopyInner(j_ins,copy,copied)) continue;
      NodeMarkInit();
      EdgeMarkInit();
      ArmOptCopyRecurse(i_bbl,copy,copied);
    }
  }
}
/* }}} */

/*{{{ ArmInsPrecomputeCopyPropEvaluation*/

void ArmInsPrecomputeCopyPropEvaluation(t_arm_ins * ins)
{
  int nr_regs = 0;
  t_reg one_reg= ARM_REG_NONE;
  t_reg reg;

/*  if(!ARM_INS_IS_CONDITIONAL(ins))*/
    {
      switch(ARM_INS_OPCODE(ins))
	{
	case ARM_ADD:
	  {
	    if(ARM_INS_FLAGS(ins) & FL_IMMED || !ArmInsHasShiftedFlexible(ins))
	      {
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
		return;
	      }
	  }
	  break;
	case ARM_SUB:
	  {
	    if(ARM_INS_FLAGS(ins) & FL_IMMED || !ArmInsHasShiftedFlexible(ins))
	      {
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
		return;
	      }
	  }
	  break;
	case ARM_MOV:
	  {
	    if(ARM_INS_REGA(ins) != ARM_REG_NONE && ARM_INS_REGC(ins) != ARM_REG_NONE && ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE)
	      {
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
		return;
	      }
	    else if(ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_FLAGS(ins) & FL_IMMED)
	      {
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
		return;
	      }
	  }
	  break;
	case ARM_MVN:
	  {
	    if(ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_FLAGS(ins) & FL_IMMED)
	      {
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
		return;
	      }
	  }
	  break;
  case ARM_VSTR:
	case ARM_STR:
	  if( ArmInsWriteBackHappens(ins) && ARM_INS_FLAGS(ins) & FL_IMMED)
	    /* Case : str  r2,[r8,#12]!  or str r2,[r8],#12 */
	    {
	      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	      return;
	    }
	  break;
  case ARM_VLDR:
	case ARM_LDR:
/*          if( ArmInsWriteBackHappens(ins) && ARM_INS_FLAGS(ins) & FL_IMMED)*/
	    /* Case : ldr  r2,[r8,#12]!  or ldr r2,[r8],#12 */
	    {
	      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	      return;
	    }
	  break;
  case ARM_VSTM:
  case ARM_VLDM:
	case ARM_STM:
	case ARM_LDM: 
	  if((ARM_INS_FLAGS(ins) & (FL_WRITEBACK) ))
	    /* Case : stm/ldm r13!,(r1,r2,r5,r9) */
	    {
	      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	      return;
	    }
	  break;
	case ARM_CONSTANT_PRODUCER:
	  {
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	    return;
	  }
	  break;
	case ARM_ADDRESS_PRODUCER:
	  {
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	    return;
	  }
	  break;
	case ARM_LDF:
	case ARM_STF:
	case ARM_SFM:
	case ARM_LFM:
	  if(ArmInsWriteBackHappens(ins))
	  {
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	    return;
	  }
	  break;

	case ARM_VPUSH:
	case ARM_VPOP:
	  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_EVAL_IN_COPY_PROP);
	  return;

	default:
	  break;
	}
    }

  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_EVAL_IN_COPY_PROP);
  
  REGSET_FOREACH_REG(ARM_INS_REGS_DEF(ins),reg)
    {
      if(RegsetIn(CFG_DESCRIPTION(BBL_CFG(ARM_INS_BBL(ins)))->int_registers,reg))
	{
	  nr_regs++;
	  one_reg = reg;
	}
    }
  
  if (nr_regs==1 && one_reg == ARM_INS_REGA(ins))
    {
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_NO_EVAL_FAST_IN_COPY_PROP);
    }
  else
    {
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_NO_EVAL_FAST_IN_COPY_PROP);
    }
}/*}}}*/

/* {{{ ArmCopyInstructionPropagator*/
void ArmCopyInstructionPropagator(t_arm_ins * ins, t_equations eqs, t_bool ignore_condition)
{
  t_cfg * cfg=ARM_INS_CFG(ins);
  t_reloc * base_reloc;
  t_int32 offset;
/*  VERBOSE(0,("Propping @I\n",ins)); fflush(stdout);*/

  if (ARM_INS_FLAGS(ins) & FL_EVAL_IN_COPY_PROP)
  if(!ARM_INS_IS_CONDITIONAL(ins) || ignore_condition)
  {
    switch(ARM_INS_OPCODE(ins))
    {
      case ARM_ADD:
      {
	if(ARM_INS_FLAGS(ins) & FL_IMMED)
	{
	  EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGB(ins),0-ARM_INS_IMMEDIATE(ins),NULL,NULL);
	  return;
	}
	else if (!ArmInsHasShiftedFlexible(ins))
	{
	  t_int32 difference;
	  if (EquationsRegsDiffer(cfg,eqs,ARM_INS_REGC(ins),ZERO_REG(cfg),&difference) == YES)
	  {
/*            VERBOSE(0,("3Yezzzz, @I %d",ins,difference));*/
	    EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGB(ins),-difference,NULL,NULL);
	  }
	  else if (EquationsRegsDiffer(cfg,eqs,ARM_INS_REGB(ins),ZERO_REG(cfg),&difference) == YES)
	  {
/*            VERBOSE(0,("4Yezzzz, @I %d",ins,difference));*/
	    EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGC(ins),-difference,NULL,NULL);
	  }
	}
      }
      break;
      case ARM_SUB:
      {
	if(ARM_INS_FLAGS(ins) & FL_IMMED)
	{
	  EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGB(ins),ARM_INS_IMMEDIATE(ins),NULL,NULL);
	  return;
	}
	else if (!ArmInsHasShiftedFlexible(ins))
	{
	  t_int32 difference;
/*          VERBOSE(0,("Check if we know the difference between these registers!"));*/
	  if(EquationsRegsDiffer(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGC(ins),&difference) == YES)
	  {
/*            VERBOSE(0,("Yezzzz, @I %d",ins,difference));*/
	    EquationsAdd(cfg,eqs,ARM_INS_REGA(ins), ZERO_REG(cfg),-difference,NULL,NULL);
	  }
	  else if (EquationsRegsDiffer(cfg,eqs,ARM_INS_REGC(ins),ZERO_REG(cfg),&difference) == YES)
	  {
/*            VERBOSE(0,("2Yezzzz, @I %d",ins,difference));*/
	    EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGB(ins),difference,NULL,NULL);
	  }
	}
      }
      break;
      case ARM_MOV:
      {
	if((ARM_INS_REGA(ins) != ARM_REG_NONE && ARM_INS_REGA(ins) != ARM_REG_R15) && ARM_INS_REGC(ins) != ARM_REG_NONE && ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE)
	{
	  EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),ARM_INS_REGC(ins),0,NULL,NULL);
	  return;
	}
	else if(ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_FLAGS(ins) & FL_IMMED)
	{
	  EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),CFG_DESCRIPTION(cfg)->num_int_regs,0-ARM_INS_IMMEDIATE(ins),NULL,NULL);
	  return;
	}
      }
      break;
      case ARM_MVN:
      {
	if(ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_FLAGS(ins) & FL_IMMED)
	{
	  t_uint32 imm = ARM_INS_IMMEDIATE(ins);
	  imm = ~imm;
	  EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),CFG_DESCRIPTION(cfg)->num_int_regs,0-imm,NULL,NULL);
	  return;
	}
      }
      break;

      case ARM_VSTR:
      case ARM_STR:
        if(ArmInsWriteBackHappens(ins) && ARM_INS_FLAGS(ins) & FL_IMMED)
  	      /* Case : str  r2,[r8,#12]!  or str r2,[r8],#12 */
        {
        	if(ARM_INS_FLAGS(ins) & FL_DIRUP)
        	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),0-ARM_INS_IMMEDIATE(ins),NULL,NULL); 
        	else
        	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),ARM_INS_IMMEDIATE(ins),NULL,NULL); 
        	return;
        }
        break;
      
      case ARM_VLDR:
      case ARM_LDR:
        if(ArmInsWriteBackHappens(ins) && ARM_INS_FLAGS(ins) & FL_IMMED)
  	      /* Case : ldr  r2,[r8,#12]!  or ldr r2,[r8],#12 */
        {
        	if(ARM_INS_FLAGS(ins) & FL_DIRUP)
        	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),0-ARM_INS_IMMEDIATE(ins),NULL,NULL); 
        	else
        	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),ARM_INS_IMMEDIATE(ins),NULL,NULL); 
        	EquationsInvalidate(cfg,eqs,ARM_INS_REGA(ins)); 
        	return;
        }

        if (EquationsRegsDifferByTag(cfg,eqs,ARM_INS_REGB(ins),ZERO_REG(cfg),&base_reloc, &offset) == YES && base_reloc != SYMBOLIC_STACK_POINTER)
        {
        	/* We know from where this ins loads its value. */
        	if(ARM_INS_REGC(ins) == ARM_REG_NONE)
        	{
        	  if(ArmInsHasImmediate(ins) && (ARM_INS_FLAGS (ins) & FL_PREINDEX) != 0)
        	  {
        	    /* We have to add an extra offset before loading */
        	    if(ARM_INS_FLAGS (ins) & FL_DIRUP)
        	      offset += ARM_INS_IMMEDIATE(ins);
        	    else offset -= ARM_INS_IMMEDIATE(ins);

        /*            VERBOSE(0,("Loading at offset %d from reloc @R",offset, base_reloc));*/

        	  }
        	}
        }
        break;
      case ARM_STM:
      case ARM_LDM: 
      if((ARM_INS_FLAGS(ins) & (FL_WRITEBACK) ))
	/* Case : stm/ldm r13!,(r1,r2,r5,r9) */
      {
        t_regset tmp_set = RegsetNewFromUint32(ARM_INS_IMMEDIATE(ins));
	t_int32 nr_regs = RegsetCountRegs(tmp_set);
	nr_regs *= 4;
	if(ARM_INS_FLAGS(ins) & FL_DIRUP)
	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),0-nr_regs,NULL,NULL);
	else
	  EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),nr_regs,NULL,NULL);
	if(ARM_INS_OPCODE(ins) == ARM_LDM)
	  EquationsInvalidateRegset(cfg,eqs,tmp_set);
	return;
      }
      break;
  case ARM_VSTM:
  case ARM_VLDM:
    if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
      /* only if base address register is modified by the instruction */
    {
      /* loaded/stored registers are saved by the disassembler in ARM_INS_MULTIPLE */
      t_regset tmp_set = ARM_INS_MULTIPLE(ins);
      t_int32 nr_regs = RegsetCountRegs(tmp_set);

      /* consider 32-bit registers by default */
      nr_regs *= 4;
      /* depending on the flags, we are loading/storing 64-bit registers */
      if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
        nr_regs *= 2;

      /* find out the direction in which the base address is modified */
      if (ARM_INS_FLAGS(ins) & FL_DIRUP)
        EquationsAdd(cfg, eqs, ARM_INS_REGB(ins), ARM_INS_REGB(ins), 0-nr_regs, NULL, NULL);
      else
        EquationsAdd(cfg, eqs, ARM_INS_REGB(ins), ARM_INS_REGB(ins), nr_regs, NULL, NULL);

      return;
    }
    break;
	 case ARM_VPUSH:
		{
		  t_regset tmp_set = ARM_INS_MULTIPLE(ins);
		  t_int32 nr_regs = RegsetCountRegs(tmp_set);
		  nr_regs *= 4;
		  if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
			 nr_regs *=2;
		  EquationsAdd(cfg,eqs,ARM_REG_R13,ARM_REG_R13,nr_regs,NULL,NULL);
		  return;
		}
	 case ARM_VPOP:
		{
		  t_regset tmp_set = ARM_INS_MULTIPLE(ins);
		  t_int32 nr_regs = RegsetCountRegs(tmp_set);
		  nr_regs *= 4;
		  if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
			 nr_regs *=2;
		  EquationsAdd(cfg,eqs,ARM_REG_R13,ARM_REG_R13,0-nr_regs,NULL,NULL);
		  RegsetSetIntersect(tmp_set,CFG_DESCRIPTION(BBL_CFG(ARM_INS_BBL(ins)))->int_registers);
		  EquationsInvalidateRegset(cfg,eqs,tmp_set);
		  return;
		}

      case ARM_CONSTANT_PRODUCER:
      {
	EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),CFG_DESCRIPTION(cfg)->num_int_regs,0-ARM_INS_IMMEDIATE(ins),NULL,NULL);
	return;
      }
      break;
      case ARM_ADDRESS_PRODUCER:
      {
	VERBOSE(2,("Adding address prod @I with reloc @R\n",ins, RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(T_RELOCATABLE(ins)))));
	EquationsAdd(cfg,eqs,ARM_INS_REGA(ins),CFG_DESCRIPTION(cfg)->num_int_regs,0, RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins)), NULL);
	return;
      }
      break;
      case ARM_LDF:
      case ARM_STF:
      case ARM_LFM:
      case ARM_SFM:
        {
	  if(ARM_INS_FLAGS(ins) & FL_DIRUP)
	    EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),0-ARM_INS_IMMEDIATE(ins),NULL,NULL);
	  else
	    EquationsAdd(cfg,eqs,ARM_INS_REGB(ins),ARM_INS_REGB(ins),ARM_INS_IMMEDIATE(ins),NULL,NULL);
	  return;
	}
	break;
      default:
        if(RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13) && ARM_INS_OPCODE(ins) != ARM_RSB)
	  VERBOSE(0,("What to do with this instruction: @I",ins));
        break;
    }
  }


  if (ARM_INS_FLAGS(ins) & FL_NO_EVAL_FAST_IN_COPY_PROP)
    {
      EquationsInvalidate(cfg,eqs,ARM_INS_REGA(ins));
    }
  else
    {
      t_regset regs = RegsetNew();
      RegsetSetDup(regs,CFG_DESCRIPTION(BBL_CFG(ARM_INS_BBL(ins)))->int_registers);
      RegsetSetIntersect(regs,ARM_INS_REGS_DEF(ins));

      if (!RegsetIsEmpty(regs))
	EquationsInvalidateRegset(cfg,eqs,regs);
    }
  return;
}/*}}}*/

void 
ArmCopyInstructionPropagatorWrapper(t_ins * ins, t_equations eqs, t_bool ignore_condition)
{
  ArmCopyInstructionPropagator(T_ARM_INS(ins), eqs, ignore_condition);
}

extern void ArmGetFirstInsOfConditionalBranchWithSideEffect(t_bbl * bbl,t_ins ** ins);
void ArmCopyAnalysisInit(t_cfg * cfg)
{
	CFG_SET_COPY_INSTRUCTION_PROPAGATOR(cfg, ArmCopyInstructionPropagatorWrapper);
	CFG_SET_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg,ArmGetFirstInsOfConditionalBranchWithSideEffect);
}

void ArmUseCopyPropagationInfo(t_cfg *cfg)
{
  t_arm_ins * ins, *tmp;
  t_bbl * ibbl;
  t_function * ifun;
  t_bool killed, changed;
  t_int32 diff;
  t_reg reg;
  t_reg bkreg;
  static t_uint32 nr_killed = 0;
  t_equation* equations = (t_equation*)Malloc((CFG_DESCRIPTION(cfg)->num_int_regs + 1)*sizeof(t_equation));

  if (!CFG_COPY_INSTRUCTION_PROPAGATOR(cfg)) 
  {
    Free(equations);
    return;
  }

  CFG_FOREACH_FUN(cfg,ifun)
  {
    FUNCTION_FOREACH_BBL(ifun,ibbl)
    {
      if (EquationIsTop(BBL_EQS_IN(ibbl)[0]))
        continue;

      if(BBL_EQS_IN(ibbl))
	      EquationsCopy(cfg,BBL_EQS_IN(ibbl),equations);
      else continue;

      /*VERBOSE(0,("@B",ibbl));*/
      /*EquationsPrint(cfg,equations); printf("--\n");*/
      BBL_FOREACH_ARM_INS_SAFE(ibbl,ins,tmp)
      {
        killed = changed = FALSE;

        /*if(diablosupport_options.debugcounter <= nr_killed) break;*/

        if(ARM_INS_OPCODE(ins) == ARM_MOV && ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE && !(ARM_INS_FLAGS(ins) & FL_S))
        {
          if (EquationsRegsDiffer(cfg,equations,ARM_INS_REGA(ins), ARM_INS_REGC(ins),&diff) == YES)
          {
            if(diff == 0)
            {
              /*if(diablosupport_options.debugcounter == nr_killed+1) GFATALBBL((ibbl,"1:Here we are"));*/
              VERBOSE(COPYPROPINFO_VERBOSITY,("Deleting1 @I",ins));
              /*nr_killed++;*/
              ArmInsKill(ins);
              continue;
            }
          }
        }
        if(ARM_INS_OPCODE(ins) == ARM_ADD && ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_REGA(ins) != ARM_INS_REGB(ins) && !(ARM_INS_FLAGS(ins) & FL_S))
        {
          t_reg candidate = ARM_REG_NONE;
          
          for(reg=0; reg < CFG_DESCRIPTION(cfg)->num_int_regs-1; reg++)
            if (EquationsRegsEqual(cfg,equations,reg, ARM_INS_REGB(ins), -ARM_INS_IMMEDIATE(ins), NULL,NULL) == YES)
            {
              if(reg == (ARM_INS_REGA(ins)))
              {
                /*if(diablosupport_options.debugcounter == nr_killed+1) GFATALBBL((ibbl,"4:Here we are"));*/
                VERBOSE(COPYPROPINFO_VERBOSITY,("Deleting2 @I",ins));
                killed = TRUE;
                /*nr_killed++;*/
                ArmInsKill(ins);
                break;
              }
              else candidate = reg;
            }
          
          if(killed) continue;
          else if(candidate != ARM_REG_NONE)
          {
            VERBOSE(COPYPROPINFO_VERBOSITY,("1:Changing @I to",ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),candidate,0,ARM_INS_CONDITION(ins));
            VERBOSE(COPYPROPINFO_VERBOSITY,("  @I",ins));
            /*changed = TRUE;*/
          }
        }
        
        if(ARM_INS_OPCODE(ins) == ARM_SUB && ARM_INS_REGC(ins) == ARM_REG_NONE && ARM_INS_REGA(ins) != ARM_INS_REGB(ins) && !(ARM_INS_FLAGS(ins) & FL_S))
        {
          t_reg candidate = ARM_REG_NONE;
	  
          for(reg=0; reg < CFG_DESCRIPTION(cfg)->num_int_regs-1; reg++)
            if (EquationsRegsEqual(cfg,equations,reg, ARM_INS_REGB(ins), +ARM_INS_IMMEDIATE(ins), NULL,NULL) == YES)
            {
              if(reg == ARM_INS_REGA(ins))
              {
                /*if(diablosupport_options.debugcounter == nr_killed+1) GFATALBBL((ibbl,"5:Here we are"));*/
                VERBOSE(COPYPROPINFO_VERBOSITY,("Deleting3 @I",ins));
                killed = TRUE;
                /*nr_killed++;*/
                ArmInsKill(ins);
                break;
              }
              else candidate = reg;
            }
	  
          if(killed) continue;
          else if(candidate != ARM_REG_NONE)
          {
            VERBOSE(COPYPROPINFO_VERBOSITY,("2:Changing @I to",ins));
            ArmInsMakeMov(ins,ARM_INS_REGA(ins),candidate,0,ARM_INS_CONDITION(ins));
            VERBOSE(COPYPROPINFO_VERBOSITY,("  @I",ins));
            /*changed = TRUE;*/
          }
        }
        
        if(ARM_INS_OPCODE(ins) == ARM_STR || ARM_INS_OPCODE(ins)==ARM_VSTR)
        {
          /*for(reg=0; reg < ARM_INS_REGA(ins); reg++)*/
          for(reg=0; reg < CFG_DESCRIPTION(cfg)->num_int_regs-1; reg++)
          {
            if(reg != ARM_INS_REGA(ins) && EquationsRegsEqual(cfg,equations,ARM_INS_REGA(ins),reg,0,NULL,NULL) == YES)
              break;
          }
          
          /*if(reg != ARM_INS_REGA(ins))*/
          if(reg != CFG_DESCRIPTION(cfg)->num_int_regs-1)
          {
            /*if(diablosupport_options.debugcounter == nr_killed+1)*/
            /*{*/
              /*EquationsPrint(cfg,equations); printf("--\n");*/
              /*GFATALBBL((ibbl,"9:Here we are @I",ins));*/
            /*}*/
            VERBOSE(COPYPROPINFO_VERBOSITY,("Substituting %d with %d: @I",ARM_INS_REGA(ins),reg,ins));

            bkreg = ARM_INS_REGA(ins);
            ARM_INS_SET_REGA(ins, reg);
            if (ArmInsIsEncodable(ins))
            {
              ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
            }
            else
            {
              ARM_INS_SET_REGA(ins, bkreg);
            }
           /*changed = TRUE;*/
          }
        }

        if(ARM_INS_REGB(ins) != ARM_REG_R13 && (ARM_INS_OPCODE(ins) == ARM_STR || ARM_INS_OPCODE(ins) == ARM_LDR || ARM_INS_OPCODE(ins) == ARM_VLDR || ARM_INS_OPCODE(ins) == ARM_VSTR))
        {
          /* Only consider LOADs and STOREs that don't include the stack pointer (R13) as a base address.
           * Additionally, only do a copy propagation on the instructions that have a writeback effect,
           * have an immediate value different of zero, and do a pre-index operation.
           */
          if(!(ARM_INS_FLAGS(ins) & FL_WRITEBACK) && ArmInsHasImmediate(ins) && (ARM_INS_FLAGS(ins) & FL_PREINDEX) && ARM_INS_IMMEDIATE(ins) != 0)
          {
            for(reg=0; reg < CFG_DESCRIPTION(cfg)->num_int_regs-1; reg++)
            {
              if (reg != ARM_INS_REGB(ins))
              {
                if ( ((ARM_INS_FLAGS(ins) & FL_DIRUP) && EquationsRegsEqual(cfg,equations, ARM_INS_REGB(ins), reg, ARM_INS_IMMEDIATE(ins), NULL,NULL) == YES)
                || (!((ARM_INS_FLAGS(ins) & FL_DIRUP)) && (EquationsRegsEqual(cfg, equations, ARM_INS_REGB(ins), reg, -ARM_INS_IMMEDIATE(ins), NULL,NULL) == YES))
                )
                {
                  /*if(diablosupport_options.debugcounter == nr_killed+1)*/
		              /*{*/
                    /*EquationsPrint(cfg,equations); printf("--\n");*/
                    /*GFATALBBL((ibbl,"9:Here we are @I",ins));*/
                  /*}*/

                  t_uint64 bkimm;
                  bkreg = ARM_INS_REGB(ins);
                  bkimm = ARM_INS_IMMEDIATE(ins);
                  VERBOSE(COPYPROPINFO_VERBOSITY,("Simpler %d with %d: @I",ARM_INS_REGB(ins),reg,ins));
                  ARM_INS_SET_IMMEDIATE(ins,0);
                  ARM_INS_SET_REGB(ins,reg);

                  if (ArmInsIsEncodable(ins))
                  {
                    ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
                    changed = TRUE;
                  }
                  else
                  {
                    ARM_INS_SET_REGB(ins, bkreg);
                    ARM_INS_SET_IMMEDIATE(ins, bkimm);
                  }
                  break;
                }
              }
            }
          }
        }

        CFG_COPY_INSTRUCTION_PROPAGATOR(cfg)(T_INS(ins),equations,FALSE);
        if(changed) nr_killed++;
      }
    }
  }

  Free(equations);
}

void ArmOptEliminateCmpEdges(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_arm_ins * ins;
  t_regset arm_conditions = CFG_DESCRIPTION(cfg)->cond_registers;
  t_regset used_conditions;
  t_equation* equations = (t_equation*)Malloc((CFG_DESCRIPTION(cfg)->num_int_regs + 1)*sizeof(t_equation));
  
  {
    Free(equations);
    return;
  }

  CFG_FOREACH_FUN(cfg, fun)
  {
    FUNCTION_FOREACH_BBL(fun, bbl)
    {
      if (!BBL_INS_LAST(bbl)) continue;
      if (!ArmInsIsConditional(T_ARM_INS(BBL_INS_LAST(bbl)))) continue;
      if(BBL_EQS_IN(bbl))
	EquationsCopy(cfg,BBL_EQS_IN(bbl),equations);
      else continue;

      used_conditions = RegsetIntersect(ARM_INS_REGS_USE(T_ARM_INS(BBL_INS_LAST(bbl))), arm_conditions);
      VERBOSE(0,("Take a look at @iB\nUsed condition registers: @X", bbl,CPREGSET(cfg, used_conditions)));

      BBL_FOREACH_ARM_INS_R(bbl, ins)
	if(! RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),used_conditions))) break;

      if (ins)
      {
	BblCopyAnalysisUntilIns(T_INS(ins),equations);

	if(ARM_INS_OPCODE(ins) == ARM_CMP)
	{
	  if (ARM_INS_REGC(ins) != ARM_REG_NONE && !ArmInsHasShiftedFlexible(ins))
	  {
	  }
	  if (ARM_INS_REGC(ins) == ARM_REG_NONE)
	  {
	  }
	  VERBOSE(0,("Defining instruction : @I",ins));
	  EquationsPrint(cfg,equations);
	}
      }
    }
  }

  Free(equations);
}

/* vim: set shiftwidth=2 foldmethod=marker : */
