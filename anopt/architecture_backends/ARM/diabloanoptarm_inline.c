/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
/* Static defines {{{ */

static int x = -1;

#define TRUE_PTR  (void*) &x 
#define FALSE_PTR  (void*) NULL 

void ArmInlineProcedureWithOnlyOneIncomingNonCallEdge(t_cfg * cfg, t_function* callee);
t_uint32 ArmInlineSmallProcedure(t_object * obj,t_cfg * cfg, t_function * fun);
t_uint32 ArmInlineSmallHotProcedure(t_object * obj,t_cfg * cfg, t_function * fun);
void ArmInlineFunAtCallsite(t_function * fun, t_bbl *call_site);
/* }}} */
/*!  
 * inline a procedure (that has multiple call sites) at one of it's call
 * sites. This is the most general form of inlining available 
 *
 * \param fun The function to inline
 * \param call_edge The call edge from the callsite at which we want to inline
 * to the function we want to inline
 *
 * \return void 
*/
/* ArmInlineProcedureAtOneCallsite  {{{ */
void ArmInlineProcedureAtOneCallsite(t_function * fun, t_cfg_edge * call_edge)
{

  /* iterators */
  t_bbl * i_bbl;
  t_arm_ins * i_ins;
  t_cfg_edge * i_edge;
  
  /* local variables */
  t_cfg * cfg;
  t_cfg_edge * return_edge;

  t_bbl * call_site;
  t_bbl * return_site;

  t_bool ok;
  t_bool switch_condition;
  t_uint32 nr_return_points = 0;

  cfg = BBL_CFG(CFG_EDGE_HEAD(call_edge));
  call_site = CFG_EDGE_HEAD(call_edge);
  return_edge = CFG_EDGE_CORR(call_edge);
  return_site = CFG_EDGE_TAIL(return_edge);

  /* step 0: check if inlining this function is possible. We don't want to
   * inline functions that perform interprocedural jumps because it's
   * impossible to remodel control flow so that it stays correct in this case.
   * -- Optimization: we will allow interprocedural jumps that are tail calls,
   * 	because we can in effect remodel control flow for this situation by 
   * 	changing the jumps to function calls.
   */
  ok = TRUE;

  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    /* the tmp2 field will be used as a boolean that indicates 
     * if a tail call starts from this block */
    BBL_SET_TMP2(i_bbl,  FALSE_PTR);


    /* skip return edges */
    if (i_bbl == FUNCTION_BBL_LAST(fun))
      continue; 

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (CfgEdgeIsInterproc(i_edge))
	if (CFG_EDGE_CAT(i_edge) != ET_CALL)
	{
	  if ((CFG_EDGE_CAT(i_edge) == ET_IPJUMP) && ((CFG_EDGE_TAIL(i_edge)) == FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge)))))
	  {
	    /* only handle interprocedural edges of type IPJUMP, because the others are too complicated to handle well */
	    BBL_SET_TMP2(i_bbl,  TRUE_PTR);
	    VERBOSE(0,("-~-~- tail call detected. still carrying on though\n"));
	  }
	  else
	  {
	    ok = FALSE;
	  }
	}
    }
  }

  if (!ok)
  {
    VERBOSE(1,("No inline for fun @G because of interprocedural jumps!\n", BBL_CADDRESS(FUNCTION_BBL_FIRST(fun))));
    return;
  }
  /* step 0b: check if all return instructions are of a format we understand.
   * At present that means they ought to be:
   * 		- mov pc,r14
   * 		- ldr pc,[r13,offset]
   * 		- ldmia r13!,{...,pc}     (ads-style return)
   * 		- ldmdb rXX, {...,r13,pc} (gcc-style return)
   */
  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_LAST(fun),i_edge)
  {
    t_arm_ins * lastins;

    /* skip compensating edges */
    if (CFG_EDGE_CAT(i_edge) == ET_COMPENSATING)
      continue;
    
    nr_return_points++;

    lastins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)));
    if ((ARM_INS_OPCODE(lastins) == ARM_MOV) && (ARM_INS_REGA(lastins) == ARM_REG_R15) && (ARM_INS_REGC(lastins) == ARM_REG_R14))
      continue;
    if ((ARM_INS_OPCODE(lastins) == ARM_LDR) && (ARM_INS_REGA(lastins) == ARM_REG_R15) && (ARM_INS_REGB(lastins) == ARM_REG_R13))
      continue;
    if ((ARM_INS_OPCODE(lastins) == ARM_LDM) && (ARM_INS_REGB(lastins) == ARM_REG_R13) && (ARM_INS_IMMEDIATE(lastins) & (1 << 15)) && ArmInsWriteBackHappens(lastins) && (ARM_INS_FLAGS(lastins) & FL_DIRUP))
      continue;
    if ((ARM_INS_OPCODE(lastins) == ARM_LDM) && (ARM_INS_REGB(lastins) != ARM_REG_R13) && (ARM_INS_IMMEDIATE(lastins) & ((1 << 13) | (1 << 15))) && !ArmInsWriteBackHappens(lastins) && !(ARM_INS_FLAGS(lastins) & FL_DIRUP))
      continue;

    ok = FALSE;
    break;
  }
  if (!ok)
  {
    VERBOSE(1,("No inline for fun @G because of abnormal return instruction\n", BBL_CADDRESS(FUNCTION_BBL_FIRST(fun))));
    return;
  }

  VERBOSE(0,("Inlining func %s from call edge @E\n",FUNCTION_NAME(fun),call_edge));

  /* an optimization:
   * if the function that we are inlining only has one return point,
   * and the call to the function is conditional, we can gain an instruction
   * by inverting the condition and making it jump to the original fallthrough case.
   * we can then drop the branch to the return site at the return point and add a 
   * fallthrough edge from the return point to the return site.
   */
  if ((nr_return_points == 1) && (ARM_INS_IS_CONDITIONAL(T_ARM_INS(BBL_INS_LAST(call_site)))))
  {
    VERBOSE(0,("Will swap condition on call\n"));
    switch_condition = TRUE;
  }
  else
    switch_condition = FALSE;






  /* step 1: duplicate all basic blocks of the function */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    t_bbl * copy;

    /* don't copy the return block */
    if (i_bbl == FUNCTION_BBL_LAST(fun))
      continue;

    copy = BblNew(cfg);
    /* temporary references between the duplicate blocks, this is for easy
     * reference from original to copy and vice versa. these references will
     * of course be removed at the end of the inlining. */
    BBL_SET_TMP(i_bbl,  copy);
    BBL_SET_TMP(copy,  i_bbl);

    /* duplicate the instructions in the basic block */
    BBL_FOREACH_ARM_INS(i_bbl,i_ins)
    {
      t_arm_ins * inscopy = ArmInsDup(i_ins);
      ArmInsAppendToBbl(inscopy, copy);
    }
  }

  /* step 1b: assign the copies of the basic blocks to the calling function */
  FUNCTION_FOREACH_BBL_R(fun,i_bbl)
  {
    if (i_bbl!=FUNCTION_BBL_LAST(fun))
    {	
      t_bbl * copy = T_BBL(BBL_TMP(i_bbl));
      BBL_SET_NEXT_IN_FUN(copy, BBL_NEXT_IN_FUN(call_site));
      BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(call_site), copy);
      BBL_SET_NEXT_IN_FUN(call_site,  copy);
      BBL_SET_PREV_IN_FUN(copy,  call_site);
      BBL_SET_FUNCTION(copy, BBL_FUNCTION(call_site));
    }
  }






  /* step 2: add edges to the newly created blocks */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    /* skip the return edges */
    if (i_bbl == FUNCTION_BBL_LAST(fun)) continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      /* intraprocedural edges should be duplicated between the copies of head
       * and tail. call edges should be duplicated between
       * the copy of the head and the original tail. interproc jump edges that
       * are used for tail calls are skipped for now. other interprocedural
       * edges should not occur. */
      t_bbl * newhead, * newtail;

      /* skip the edges to the return block */
      if (CFG_EDGE_TAIL(i_edge) == FUNCTION_BBL_LAST(fun))
	continue;

      /* skip interprocedural edges (only tail calls should happen but we handle them later) */
      if (CfgEdgeIsInterproc(i_edge) && !(CFG_EDGE_CAT(i_edge) == ET_CALL))
	continue;

      newhead = T_BBL(BBL_TMP(i_bbl));
      if (CFG_EDGE_CAT(i_edge) == ET_CALL)
      {
	t_bbl * newreturn;

	newtail = CFG_EDGE_TAIL(i_edge);
	newreturn = T_BBL(BBL_TMP(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))));
	CfgEdgeCreateCall(cfg,newhead,newtail,newreturn,FUNCTION_BBL_LAST(BBL_FUNCTION(newtail)));
      }
      else
      {
	newtail = T_BBL(BBL_TMP(CFG_EDGE_TAIL(i_edge)));
	CfgEdgeCreate(cfg,newhead,newtail,CFG_EDGE_CAT(i_edge));
      }
    }
  }






  /* step 3: change the function call into a jump */
  if (!switch_condition)
  {
    /* jump to the inlined part */
    ArmInsMakeCondBranch(T_ARM_INS(BBL_INS_LAST(call_site)),ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(call_site))));
    CfgEdgeCreate(cfg,call_site,T_BBL(BBL_TMP(FUNCTION_BBL_FIRST(fun))),ET_JUMP);
  }
  else
  {
    /* invert the condition on the original call instruction and swap taken and fallthrough cases */
    t_cfg_edge * ft_edge;

    ArmInsMakeCondBranch(T_ARM_INS(BBL_INS_LAST(call_site)),ArmInvertCondition(ARM_INS_CONDITION(T_ARM_INS(BBL_INS_LAST(call_site)))));

    /* find the original fallthrough edge */
    BBL_FOREACH_SUCC_EDGE(call_site,ft_edge)
      if ((CFG_EDGE_CAT(ft_edge) == ET_FALLTHROUGH) || (CFG_EDGE_CAT(ft_edge) == ET_IPFALLTHRU))
	break;
    ASSERT(ft_edge,("Could not find fallthrough edge with conditional control transfer"));
    /* turn it into a jump edge */
    CFG_EDGE_SET_CAT(ft_edge, (CFG_EDGE_CAT(ft_edge) == ET_IPFALLTHRU) ? ET_IPJUMP : ET_JUMP);

    /* add a fallthrough edge from the call site to the entry of the inlined function */
    CfgEdgeCreate(cfg,call_site,T_BBL(BBL_TMP(FUNCTION_BBL_FIRST(fun))),ET_FALLTHROUGH);
  }






  /* step 4: change the return instructions into jumps */
  /* This is a little tricky: if the original link edge was interprocedural,
   * the  returning jumps should also be interprocedural */
  {

    BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_LAST(fun),i_edge)
    {
      t_bbl * block;
      t_arm_ins * return_ins;

      /* skip compensating edges */
      if (CFG_EDGE_CAT(i_edge) == ET_COMPENSATING)
	continue;

      block = T_BBL(BBL_TMP(CFG_EDGE_HEAD(i_edge)));
      return_ins = T_ARM_INS(BBL_INS_LAST(block));       
      
      /* as a reminder: in the beginning of the function we checked whether
       * the return instruction was of one of the following forms:
       * 	- mov pc,r14
       * 	- ldr pc,[r13,offset]
       * 	- ldmia r13!,{...,pc}
       * 	- ldmdb rXX, {...,r13,pc}
       * if we found any other return instructions, we refused to do the
       * inlining. */
      switch (ARM_INS_OPCODE(return_ins))
      {
	case ARM_MOV:
	  /* just turn this instruction into a (conditional) branch to the
	   * right destination, and add a jump edge to the return site 
	   * alternatively, if switch_condition is TRUE, add a fallthrough
	   * edge to the return site and remove the return instruction altogether */
	  if (!switch_condition)
	    ArmInsMakeCondBranch(return_ins,ARM_INS_CONDITION(return_ins));
	  else
	    ArmInsKill(return_ins);
	  break;

	case ARM_LDR:
	  /* LDR's : Remove the LDR, add an indexing instruction if necessary and add the branch */

	  /* a small optimization:
	   * if r14 is dead at the beginning of the return site, we can just as well load the stack
	   * value in r14 (it was most probably stored from r14 as well). the idea is that we can
	   * reintroduce some symmetry in this way: in the beginning of the function we have STR r14,[r13]
	   * and we make sure that at the end of the function we have LDR r14,[r13]. This makes it easier
	   * for other analyses to remove this pair of stack instructions. Note that it isn't very useful to
	   * use another dead register for this purpose because that won't introduce the same symmetry, 
	   * and that will certainly make OptKillUseless change the load into a stack indexing operation, so we 
	   * might as well do that here in the first place. */
	  if (!RegsetIn(InsRegsLiveBefore(BBL_INS_FIRST(return_site)),ARM_REG_R14))
	  {
	    VERBOSE(0,("-~-~- Optimized LDR\n"));
	    ARM_INS_SET_REGA(return_ins,  ARM_REG_R14);
	    ARM_INS_SET_REGS_DEF(return_ins, ArmDefinedRegisters(return_ins));

	    if (!switch_condition)
	    {
	      t_arm_ins * j_ins = ArmInsNewForCfg(cfg);
	      ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
	      ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
	      ArmInsInsertAfter(j_ins,return_ins);
	      ArmInsMakeCondBranch(j_ins,ARM_INS_CONDITION(return_ins));
	    }
	  }
	  else
	  {
	    if (ArmInsWriteBackHappens(return_ins))
	    {
	      if (ARM_INS_FLAGS(return_ins) & FL_DIRUP)
	      {
		t_uint32 imm=ARM_INS_IMMEDIATE(return_ins);
		ArmInsMakeAdd(return_ins,ARM_INS_REGB(return_ins),ARM_INS_REGB(return_ins),ARM_REG_NONE,imm,ARM_INS_CONDITION(return_ins));

		if (!switch_condition)
		{
		  t_arm_ins * j_ins = ArmInsNewForCfg(cfg);
		  ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
		  ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
		  ArmInsInsertAfter(j_ins,return_ins);
		  ArmInsMakeCondBranch(j_ins,ARM_INS_CONDITION(return_ins));
		}
	      }
	      else
		FATAL(("Implement stacks that grow in upward direction"));
	    }
	    else
	      FATAL(("Implement returns that don't restore the stack correctly"));
	  }
	  break;

	case ARM_LDM:
	  /* Load multiples: remove pc from multiple load, add a stack indexing instruction */ 
	  {
	    t_arm_ins * j_ins;

	    if (!switch_condition)
	    {
	      /* Add the branch */
	      j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
	      ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
	      ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
	      ArmInsInsertAfter(j_ins,return_ins);
	      ArmInsMakeCondBranch(j_ins,ARM_INS_CONDITION(return_ins));
	    }

	    /* an optimization:
	     * if r14 is not live at the beginning of the return site, it means we can
	     * trash whatever is in it. This means we can use it to load the value that would 
	     * normally be loaded into the program counter. That way we don't have to add a stack
	     * indexing instruction to keep the stack pointer correct */
	    if (!RegsetIn(InsRegsLiveBefore(BBL_INS_FIRST(return_site)),ARM_REG_R14) && !(ARM_INS_IMMEDIATE(return_ins) & (1 << 14)))
	    {
	      VERBOSE(0,("-~-~- Optimized LDM\n"));
	      ARM_INS_SET_IMMEDIATE(return_ins, ARM_INS_IMMEDIATE(return_ins)& ~(1 << 15));
	      ARM_INS_SET_IMMEDIATE(return_ins, ARM_INS_IMMEDIATE(return_ins)| (1 << 14));
	      ARM_INS_SET_REGS_DEF(return_ins, ArmDefinedRegisters(return_ins));
	    }
	    else
	    {
	      /* check which kind of return instruction (gcc-style or ads-style)
	       * we have */
	      if (ARM_INS_REGB(return_ins) == ARM_REG_R13)
	      {
		/* ads-style */
		/* Add the indexing instruction before the branch, after the LDM */
		j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
		ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
		ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
		ArmInsInsertAfter(j_ins,return_ins);
		ArmInsMakeAdd(j_ins,ARM_INS_REGB(return_ins),ARM_INS_REGB(return_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(return_ins));
		ARM_INS_SET_IMMEDIATE(return_ins, ARM_INS_IMMEDIATE(return_ins)& ~(1<<15));
		ARM_INS_SET_REGS_DEF(return_ins, ArmDefinedRegisters(return_ins));
	      }
	      else
	      {
		/* gcc-style */
		/* Add the indexing instruction before the LDM */
		j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
		ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
		ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
		ArmInsInsertBefore(j_ins,return_ins);
		ArmInsMakeSub(j_ins,ARM_INS_REGB(return_ins),ARM_INS_REGB(return_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(return_ins));
		ARM_INS_SET_IMMEDIATE(return_ins, ARM_INS_IMMEDIATE(return_ins)& ~(1<<15));
		ARM_INS_SET_REGS_DEF(return_ins, ArmDefinedRegisters(return_ins));
	      }
	    }
	    break;
	  }

	default: FATAL(("Found a malformed return instruction. This should have been detected before!"));
      }

      /* add the jump edge */
      CfgEdgeCreate(cfg,block,return_site,switch_condition ? ET_FALLTHROUGH : ET_JUMP);

    }
  }

  /* step 4b: handle the interprocedural tail calls.
   * This basically means turning the jump instruction into a function call and adding a
   * block to which the call can return */
  {
    FUNCTION_FOREACH_BBL(fun,i_bbl)
    {
      t_bbl * copy;
      t_bbl * call_return;
      t_arm_ins * jump_ins;
      
      if (!BBL_TMP2(i_bbl)) continue;

      copy = T_BBL(BBL_TMP(i_bbl));
      /* find the interprocedural jump edge */
      BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
	if (CFG_EDGE_CAT(i_edge) == ET_IPJUMP)
	  break;
      ASSERT(i_edge,("interproc jump edge not found. this should not happen!"));

      /* change the last instruction of copy into a function call */
      jump_ins = T_ARM_INS(BBL_INS_LAST(copy));
      ArmInsMakeCondBranchAndLink(jump_ins,ARM_INS_CONDITION(jump_ins));

      /* prepare a basic block for the call to return to */
      if (!switch_condition)
      {
	call_return = BblNew(cfg);

	BBL_SET_NEXT_IN_FUN(call_return,  BBL_NEXT_IN_FUN(copy));
	BBL_SET_PREV_IN_FUN(call_return,  copy);
	BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(copy),  call_return);
	BBL_SET_NEXT_IN_FUN(copy,  call_return);

	BBL_SET_FUNCTION(call_return,  BBL_FUNCTION(call_site));

	InsAppendToBbl(InsNewForCfg(cfg),call_return);
	ARM_INS_SET_CSIZE(T_ARM_INS(BBL_INS_FIRST(call_return)), AddressNew32(4));
	ARM_INS_SET_OLD_SIZE(T_ARM_INS(BBL_INS_FIRST(call_return)), AddressNew32(0));
	ArmInsMakeCondBranch(T_ARM_INS(BBL_INS_FIRST(call_return)),ARM_CONDITION_AL);
      }
      else
      {
	call_return = return_site;
      }

      /* Add the call and return edges */
      CfgEdgeCreateCall(cfg,copy,CFG_EDGE_TAIL(i_edge),call_return,FUNCTION_BBL_LAST(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))));


      /* if the conditions on the original call are not inverted,
       * we must now add a jump edge from the call_return block to the 
       * return site. */
      if (!switch_condition)
      {
	  CfgEdgeCreate(cfg,call_return,return_site,ET_JUMP);
      }
    }
  }





  /* step 5: clean up the mess */
  /* this means: - remove the references from original to copy and vice versa
   *               that were stored in the tmp field of the basic blocks 
   *             - remove the original call, return and link edges
   */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    /* skip the return block because it doesn't have a copy */
    if (i_bbl == FUNCTION_BBL_LAST(fun))
      continue;

    BBL_SET_TMP(T_BBL(BBL_TMP(i_bbl)),  NULL);
    BBL_SET_TMP(i_bbl,  NULL);
  }
  CfgEdgeKill(call_edge);
  CfgEdgeKill(return_edge);
}
/* }}} */
/*!
 * \todo Document
 *
 * \param cfg
 *
 * \return void 
*/
/* ArmInlineTrivial {{{ */
void ArmInlineTrivial(t_cfg * cfg)
{
  t_object * obj=CFG_OBJECT(cfg);
  t_arm_ins * i_ins;
  t_cfg_edge * i_edge;
  t_function * i_fun;
  t_bbl * i_bbl;
  t_function* tmp;

  int nr_ins = 0;
  int nr_edges = 0;
  int nr_callees = 0;
  int nr_callers = 0;

  STATUS(START, ("InlineTrivial"));
    CfgComputeHotBblThreshold(cfg, 0.95);

  CFG_FOREACH_FUNCTION_SAFE(cfg,i_fun,tmp)
  {
    nr_ins = nr_edges = 0;

    FUNCTION_FOREACH_BBL(i_fun,i_bbl)
    {
      BBL_FOREACH_ARM_INS(i_bbl,i_ins)
      {
	nr_ins++;
      }

      BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
      {
	if (!CfgEdgeIsForwardInterproc(i_edge)) continue;
	if (CFG_EDGE_CAT(i_edge)==ET_CALL) nr_edges+=100;
	if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) nr_edges+=100;
	nr_edges++;
      }
    }

    if ((nr_ins==2 || nr_ins==1)&&(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))
      {
	if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==ARM_MOV && ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==15 &&  ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==14)
	  {
	    int nr = ArmInlineSmallProcedure(obj, cfg, i_fun);
	    if (nr)
	      nr_callees++;
	    nr_callers+=nr;
	  }
      }
    else if (nr_ins == 3)
      {
	if ((diabloflowgraph_options.blockprofilefile && BblIsHot(FUNCTION_BBL_FIRST(i_fun))) && BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun)))
	  {
	    if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==ARM_MOV && ARM_INS_REGA(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==15 &&  ARM_INS_REGC(T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_FIRST(i_fun))))==14) 
	      {
		
		
		int nr = ArmInlineSmallHotProcedure(obj, cfg, i_fun);
		if (nr)
		  nr_callees++;
		nr_callers+=nr;
	      }
	  }
      }
    else
      if (nr_edges<2)
	{
	  if (nr_edges==1)
	    {
	      /*	  nr_callees++; */
	      /*	  nr_callers++; */
	      /*	  ArmInlineProcedureWithOnlyOneIncomingNonCallEdge(cfg, i_fun);*/
	    }
	}
  }

  VERBOSE(0, ("%d callees inlined at %d callers",nr_callees,nr_callers));
  STATUS(STOP, ("InlineTrivial"));
}
/* }}} */
/*!
 * \todo Document
 *
 * \param cfg
 * \param callee
 *
 * \return void 
*/
/* ArmInlineProcedureWithOnlyOneIncomingNonCallEdge {{{ */
void ArmInlineProcedureWithOnlyOneIncomingNonCallEdge(t_cfg * cfg, t_function* callee)
{
  t_cfg_edge * tmp_edge;
  t_cfg_edge * i_edge=NULL;
  t_cfg_edge * call_edge;
  t_function * caller;
  t_bbl * i_bbl, *tmp, *last_bbl;

  /* Don't inline the hell function! */
  if (FUNCTION_IS_HELL(callee)) return;

  FUNCTION_FOREACH_BBL(callee,i_bbl) 
    BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
    {
      if (CfgEdgeIsForwardInterproc(i_edge)) goto found;
    }

found:
  call_edge = i_edge;

  /* Don't inline the entry point into the function containing the unique entry node */
  if (CFG_EDGE_HEAD(call_edge) == CFG_UNIQUE_ENTRY_NODE(cfg)) return;

  caller = BBL_FUNCTION(CFG_EDGE_HEAD(call_edge));

  /* move the basic blocks */

  FUNCTION_FOREACH_BBL_SAFE(callee,i_bbl,tmp)
  {
    /* insert the basic block */
    if (i_bbl==FUNCTION_BBL_LAST(callee)) continue;

    FUNCTION_SET_BBL_FIRST(callee, BBL_NEXT_IN_FUN(i_bbl));
    BBL_SET_PREV_IN_FUN(FUNCTION_BBL_FIRST(callee),  NULL);

    BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(FUNCTION_BBL_LAST(caller)), i_bbl);
    BBL_SET_PREV_IN_FUN(i_bbl, BBL_PREV_IN_FUN(FUNCTION_BBL_LAST(caller)));
    BBL_SET_NEXT_IN_FUN(i_bbl, FUNCTION_BBL_LAST(caller));
    BBL_SET_PREV_IN_FUN(FUNCTION_BBL_LAST(caller), i_bbl);
    BBL_SET_FUNCTION(i_bbl, caller);      


    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
      if (CFG_EDGE_CAT(i_edge)!=ET_CALL && CfgEdgeIsForwardInterproc(i_edge) && BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))==caller)
      {
	switch (CFG_EDGE_CAT(i_edge))
	{
	  case ET_IPUNKNOWN: 
	    CFG_EDGE_SET_CAT(i_edge,  ET_UNKNOWN); 
	    break;
	  case ET_IPFALLTHRU: 
	    CFG_EDGE_SET_CAT(i_edge, ET_FALLTHROUGH); 
	    break;
	  case ET_IPJUMP: 
	    CFG_EDGE_SET_CAT(i_edge, ET_JUMP); 
	    break;
	  default:
	    printf("we have a problem: unknown IP edge type\n");
	    exit(0);
	}
	if (CFG_EDGE_CORR(i_edge))
	{
	  CfgEdgeKill(CFG_EDGE_CORR(i_edge));
	  CFG_EDGE_SET_CORR(i_edge, NULL);
	}
      }
  }

  /* adapt the edge */

  if (CFG_EDGE_CORR(call_edge))
  {
    CfgEdgeKill(CFG_EDGE_CORR(call_edge));
    CFG_EDGE_SET_CORR(call_edge, NULL);
  }

  switch (CFG_EDGE_CAT(call_edge))
  {
    case ET_IPUNKNOWN: 
      CFG_EDGE_SET_CAT(call_edge,  ET_UNKNOWN); 
      break;
    case ET_IPFALLTHRU: 
      CFG_EDGE_SET_CAT(call_edge, ET_FALLTHROUGH); 
      break;
    case ET_IPJUMP: 
      CFG_EDGE_SET_CAT(call_edge, ET_JUMP); 
      break;
    default:
      FATAL(("Unkown IP edge type: @E\n",call_edge));
      exit(0);
  }

  /* adapt all incoming edges in the callee's last block */

  BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_LAST(callee),i_edge,tmp_edge)
  { 
    CFG_EDGE_SET_TAIL(i_edge, FUNCTION_BBL_LAST(caller));

    CFG_EDGE_SET_PRED_NEXT(i_edge, BBL_PRED_FIRST(FUNCTION_BBL_LAST(caller)));

    if (BBL_PRED_FIRST(FUNCTION_BBL_LAST(caller)))
      CFG_EDGE_SET_PRED_PREV(BBL_PRED_FIRST(FUNCTION_BBL_LAST(caller)), i_edge);

    CFG_EDGE_SET_PRED_PREV(i_edge, NULL);

    BBL_SET_PRED_FIRST(FUNCTION_BBL_LAST(caller), i_edge);

    if (BBL_PRED_LAST(FUNCTION_BBL_LAST(caller))==NULL)
      BBL_SET_PRED_LAST(FUNCTION_BBL_LAST(caller), i_edge);
  }


  BBL_SET_PRED_FIRST(FUNCTION_BBL_LAST(callee), NULL);
  BBL_SET_PRED_LAST(FUNCTION_BBL_LAST(callee), NULL);

  BBL_SET_SUCC_FIRST(FUNCTION_BBL_LAST(callee), NULL);
  BBL_SET_SUCC_LAST(FUNCTION_BBL_LAST(callee), NULL);

  last_bbl = FUNCTION_BBL_LAST(callee);

  BblKill(last_bbl);


  FunctionKill(callee);
}
/* }}} */
/*!
 * Inlines a funtion of 1 real block (maximum 2 instructions) (and one
 * exit block) at its callsites.
 *
 *
 * \todo Remove obj and cfg as parameters
 * 
 * \param obj 
 * \param cfg the control flowgraph to optimize
 * \param fun The function to inline
 *
 * \return t_uint32 The number of times the function got inlined
*/
/* ArmInlineSmallProcedure {{{ */
t_uint32 ArmInlineSmallProcedure(t_object * obj, t_cfg * cfg, t_function * fun)
{
  t_bbl* entry;
  t_bbl* i_bbl;
  t_cfg_edge * call_edge, * tmp;
  int nr_bbls=0;
  t_arm_ins * new_ins;
  t_arm_ins * i_ins;
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    nr_bbls++;
  }

  if (nr_bbls!=2)
  {
    printf("not inlined: number of bbls is %d\n",nr_bbls++);
  }

  nr_bbls=0;

  entry = FUNCTION_BBL_FIRST(fun);


  BBL_FOREACH_PRED_EDGE_SAFE(entry,call_edge,tmp)
  {
    t_bbl* call_site = CFG_EDGE_HEAD(call_edge);
    t_bbl * return_site;
    t_arm_ins* call_ins;

    if (CFG_EDGE_CAT(call_edge)!=ET_CALL) continue;
    if (BBL_IS_HELL(call_site)) continue;

    return_site = CFG_EDGE_TAIL(CFG_EDGE_CORR(call_edge));
    call_ins = T_ARM_INS(BBL_INS_LAST(call_site));

    if (ARM_INS_OPCODE(call_ins)!=ARM_BL) continue;
    if ((BBL_NINS(entry)==1 || BBL_NINS(entry)==2) && ((!ArmInsIsConditional(T_ARM_INS(BBL_INS_FIRST(entry))) && !ArmInsIsConditional(T_ARM_INS(BBL_INS_LAST(entry))) && !ArmInsUpdatesCond(T_ARM_INS(BBL_INS_FIRST(entry))) && !ArmInsUpdatesCond(T_ARM_INS(BBL_INS_LAST(entry)))) || !ArmInsIsConditional(call_ins)))
      {
	t_cfg_edge * fall_through_edge = NULL;
	t_cfg_edge * edge;
	nr_bbls++;
	
	/*	VERBOSE(0,("@eiB\n",call_site));*/
	
	/*	FunctionDrawGraph(BBL_FUNCTION(call_site),"inline.dot");*/

	/*	VERBOSE(0,("voor callsite @ieB\nvoor returnsite @ieB\n entry@ieB\n",call_site,return_site,entry)); */
	
	BBL_SET_EXEC_COUNT(entry, BBL_EXEC_COUNT(entry)-CFG_EDGE_EXEC_COUNT(call_edge));
	
	BBL_FOREACH_ARM_INS(entry,i_ins)
	  if (ARM_INS_EXEC_COUNT(i_ins)>=CFG_EDGE_EXEC_COUNT(call_edge))
	    ARM_INS_SET_EXEC_COUNT(i_ins, ARM_INS_EXEC_COUNT(i_ins)-CFG_EDGE_EXEC_COUNT(call_edge));
	

	if (BBL_NINS(entry)==2)
	  {
	    new_ins = ArmInsDup(T_ARM_INS(BBL_INS_FIRST(entry)));
	    ARM_INS_SET_REFED_BY(new_ins, NULL);
	    ArmInsAppendToBbl(new_ins,call_site);
	    ARM_INS_SET_EXEC_COUNT(new_ins, CFG_EDGE_EXEC_COUNT(call_edge));

	    if (ARM_INS_IS_CONDITIONAL(call_ins))
	      {
		ARM_INS_SET_CONDITION(new_ins, ARM_INS_CONDITION(call_ins));
		ARM_INS_SET_ATTRIB(new_ins,   ARM_INS_ATTRIB(new_ins) | IF_CONDITIONAL);
		ARM_INS_SET_REGS_USE(new_ins,  ArmUsedRegisters(new_ins));
		ARM_INS_SET_REGS_DEF(new_ins,  ArmDefinedRegisters(new_ins));
	      }
	  }
	
	
	ArmInsKill(call_ins);
	
	if (tmp==CFG_EDGE_CORR(call_edge))
	  tmp = CFG_EDGE_PRED_NEXT(tmp);
	
	CfgEdgeKill(CFG_EDGE_CORR(call_edge));
	CfgEdgeKill(call_edge);
	
	BBL_FOREACH_SUCC_EDGE(call_site,edge)
	  {
	    if  (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	      fall_through_edge = edge;
	    else
	      FATAL(("OOPS: EDGE OTHER THAN LINK OR FALLTHROUGH EDGE DURING INLINING\n"));
	  }
	
	if (!fall_through_edge)
	    CfgEdgeCreate(cfg,call_site,return_site,ET_FALLTHROUGH);

	/*	VERBOSE(0,("na callsite @ieB\nna returnsite @ieB\n",call_site,return_site)); */

      }
  }
  return nr_bbls;
  
}

t_uint32 ArmInlineSmallHotProcedure(t_object * obj, t_cfg * cfg, t_function * fun)
{
  t_bbl* entry;
  t_bbl* i_bbl;
  t_cfg_edge * call_edge, * tmp;
  int nr_bbls=0;
  t_arm_ins * new_ins;
  t_arm_ins * i_ins;
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    nr_bbls++;
  }

  if (nr_bbls!=2)
  {
    printf("not inlined: number of bbls is %d\n",nr_bbls++);
  }

  nr_bbls=0;

  entry = FUNCTION_BBL_FIRST(fun);

  BBL_FOREACH_PRED_EDGE_SAFE(entry,call_edge,tmp)
  {
    t_bbl* call_site = CFG_EDGE_HEAD(call_edge);
    t_bbl * return_site;
    t_arm_ins* call_ins;

    if (CFG_EDGE_CAT(call_edge)!=ET_CALL) continue;
    if (BBL_IS_HELL(call_site)) continue;
    if (CFG_EDGE_EXEC_COUNT(call_edge)<BBL_EXEC_COUNT(entry)/4) continue;

    return_site = CFG_EDGE_TAIL(CFG_EDGE_CORR(call_edge));
    call_ins = T_ARM_INS(BBL_INS_LAST(call_site));

    if (ARM_INS_OPCODE(call_ins)!=ARM_BL) continue;

    if (BBL_NINS(entry)==3 && ((!ArmInsIsConditional(T_ARM_INS(BBL_INS_FIRST(entry))) && !ArmInsIsConditional(T_ARM_INS(BBL_INS_LAST(entry))) && !ArmInsIsConditional(ARM_INS_INEXT(T_ARM_INS(BBL_INS_FIRST(entry)))) && !ArmInsUpdatesCond(ARM_INS_INEXT(T_ARM_INS(BBL_INS_FIRST(entry)))) && !ArmInsUpdatesCond(T_ARM_INS(BBL_INS_FIRST(entry))) && !ArmInsUpdatesCond(T_ARM_INS(BBL_INS_LAST(entry)))) || !ArmInsIsConditional(call_ins)))
      {
	t_cfg_edge * fall_through_edge = NULL;
	t_cfg_edge * edge;
	nr_bbls++;
	
	/*	VERBOSE(0,("@eiB\n",call_site));*/
	
	/*	FunctionDrawGraph(BBL_FUNCTION(call_site),"inline.dot");*/

	VERBOSE(10,("voor callsite @ieB\nvoor returnsite @ieB\n entry@ieB\n",call_site,return_site,entry));
	
	BBL_SET_EXEC_COUNT(entry,  BBL_EXEC_COUNT(entry)-CFG_EDGE_EXEC_COUNT(call_edge));
	
	BBL_FOREACH_ARM_INS(entry,i_ins)
	  if (ARM_INS_EXEC_COUNT(i_ins)>=CFG_EDGE_EXEC_COUNT(call_edge))
	    ARM_INS_SET_EXEC_COUNT(i_ins,   ARM_INS_EXEC_COUNT(i_ins)-CFG_EDGE_EXEC_COUNT(call_edge));
	
	if (BBL_NINS(entry)==3)
	  {
	    new_ins = ArmInsDup(T_ARM_INS(BBL_INS_FIRST(entry)));
	    ARM_INS_SET_REFED_BY(new_ins, NULL);
	    ArmInsAppendToBbl(new_ins,call_site);
	    ARM_INS_SET_EXEC_COUNT(new_ins, CFG_EDGE_EXEC_COUNT(call_edge));
	    
	    if (ARM_INS_IS_CONDITIONAL(call_ins))
	      {
		ARM_INS_SET_CONDITION(new_ins, ARM_INS_CONDITION(call_ins));
		ARM_INS_SET_ATTRIB(new_ins,  ARM_INS_ATTRIB(new_ins)| IF_CONDITIONAL);
		ARM_INS_SET_REGS_USE(new_ins,  ArmUsedRegisters(new_ins));
		ARM_INS_SET_REGS_DEF(new_ins,  ArmDefinedRegisters(new_ins));
	      }
	    
	    new_ins = ArmInsDup(ARM_INS_INEXT(T_ARM_INS(BBL_INS_FIRST(entry))));
	    ARM_INS_SET_REFED_BY(new_ins, NULL);
	    ArmInsAppendToBbl(new_ins,call_site);
	    ARM_INS_SET_EXEC_COUNT(new_ins, CFG_EDGE_EXEC_COUNT(call_edge));
	    
	    if (ARM_INS_IS_CONDITIONAL(call_ins))
	      {
		ARM_INS_SET_CONDITION(new_ins, ARM_INS_CONDITION(call_ins));
		ARM_INS_SET_ATTRIB(new_ins,  ARM_INS_ATTRIB(new_ins) | IF_CONDITIONAL);
		ARM_INS_SET_REGS_USE(new_ins,  ArmUsedRegisters(new_ins));
		ARM_INS_SET_REGS_DEF(new_ins,  ArmDefinedRegisters(new_ins));
	      }
	  }
		
	ArmInsKill(call_ins);
	
	if (tmp==CFG_EDGE_CORR(call_edge))
	  tmp = CFG_EDGE_PRED_NEXT(tmp);
	
	CfgEdgeKill(CFG_EDGE_CORR(call_edge));
	CfgEdgeKill(call_edge);
	
	BBL_FOREACH_SUCC_EDGE(call_site,edge)
	  {
	    if  (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	      fall_through_edge = edge;
	    else
	      FATAL(("OOPS: EDGE OTHER THAN LINK OR FALLTHROUGH EDGE DURING INLINING\n"));
	  }
	
	if (!fall_through_edge)
	    CfgEdgeCreate(cfg,call_site,return_site,ET_FALLTHROUGH);

	VERBOSE(10,("na callsite @ieB\nna returnsite @ieB\n",call_site,return_site));

      }
  }
  return nr_bbls;
  
}
/* }}} */
/*!
 * \todo Document
 *
 * \param cfg
 *
 * \return void 
*/
/* ArmGeneralInlining {{{ */
void ArmGeneralInlining(t_cfg * cfg)
{
  t_bbl * i_bbl = NULL;
  t_cfg_edge * i_edge = NULL;
  t_bool flag = TRUE; /* in and outgoing forward interproc edges are only call edges? */
  t_function * i_fun;
  int nr_callers; /* nr of call sites */
  t_bbl * call_site=NULL;
  t_arm_ins * last_arm_ins;
  t_function * tmp;
  static int total_count = 0;
  t_bool cont = FALSE;
  t_bbl * tbb_bbl = NULL;
  t_uint32 nr_inlined = 0;

  VERBOSE(1,("inlining "));

  /* we need accurate liveness information for this */
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

  CFG_FOREACH_FUNCTION_SAFE(cfg,i_fun,tmp)
  {
    flag = TRUE;
    nr_callers = 0;

    if (BBL_IS_HELL(FUNCTION_BBL_FIRST(i_fun))) continue;
    
    FUNCTION_FOREACH_BBL(i_fun,i_bbl)
    {
      BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
	if (CfgEdgeIsForwardInterproc(i_edge) 
	    && CFG_EDGE_CAT(i_edge)!=ET_CALL 
	    && CFG_EDGE_CAT(i_edge) != ET_SWI
	    )
	  flag = FALSE;

      BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
      {
	if (CfgEdgeIsForwardInterproc(i_edge))
	{
	  if (CFG_EDGE_CAT(i_edge)!=ET_CALL)
	    flag = FALSE;
	  else if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge)))
	    nr_callers+=2;
	  else
	  {
	    nr_callers++;
	    call_site = CFG_EDGE_HEAD(i_edge);
	  }
	}
      }
    }

    /*if (call_site == CFG_UNIQUE_ENTRY_NODE(cfg)) continue; */
    if (!flag || nr_callers!=1)
      continue;

    if (FunctionGetExitBlock(i_fun)==FUNCTION_BBL_LAST(i_fun))
    {
      BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_LAST(i_fun),i_edge)
      {
	last_arm_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)));
	if (!last_arm_ins)
	{
	  /*FunctionPrint(i_fun,PRINT_INSTRUCTIONS); */
	  FATAL(("Found a return block without instructions\n"));
	}
	if (ArmInsHasShiftedFlexible(last_arm_ins))
	{
	  VERBOSE(1,("@F POTENTIAL: NO INLINE 3\n",i_fun));
	  flag = FALSE;
	  break;
	}
	if (ARM_INS_OPCODE(last_arm_ins)!=ARM_MOV && ARM_INS_OPCODE(last_arm_ins)!=ARM_LDM && ARM_INS_OPCODE(last_arm_ins)!=ARM_LDR && ARM_INS_OPCODE(last_arm_ins)!=ARM_BX) 
	{
	  VERBOSE(1,("@F POTENTIAL: NO INLINE 2\n",i_fun));
	  flag = FALSE;
	  break;
	}
	/* if the return is a return from supervisor to user mode,
	 * do not inline! */
	if (ARM_INS_OPCODE(last_arm_ins) == ARM_MOV && (ARM_INS_FLAGS(last_arm_ins) & FL_S))
	{ /* movs pc,r14 */
	  flag = FALSE;
	  break;
	}
	if (ARM_INS_OPCODE(last_arm_ins) == ARM_SUB && (ARM_INS_FLAGS(last_arm_ins) & FL_S))
	{ /* subs pc,r14,(4 or 8) */
	  flag = FALSE;
	  break;
	}
	if (ARM_INS_OPCODE(last_arm_ins) == ARM_LDM && (ARM_INS_FLAGS(last_arm_ins) & FL_USERMODE_REGS))
	{ /* ldmia ...,{...,pc}^ */
	  flag = FALSE;
	  break;
	}
        if (ARM_INS_OPCODE(last_arm_ins) == ARM_BX && (ARM_INS_REGB(last_arm_ins) != ARM_REG_R14))
        { /* not "bx r14" */
          flag = FALSE;
          break;
        }
      }
    }

    /* if we would inline the callee, verify that the branch instruction does not
     * change the instruction set in any way. */
    if (flag && ArmInsChangesInstructionSet(T_ARM_INS(BBL_INS_LAST(call_site))))
    {
        VERBOSE(1,("@F not inlined at @B because the instruction set is changed", i_fun, call_site));
        flag = FALSE;
    }

    /* if we would inline the callee, verify that the call site is not part of a
     * FALLTHROUGH path originating in one of the cases of a TBB/TBH instruction */
    if (flag)
    {
        /* do not inline functions if the call site is on a fallthrough path in a TBB/TBH construction */
        i_bbl = call_site;
        tbb_bbl = NULL;

        do
        {
                cont = FALSE;

                BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
                {
                        switch (CFG_EDGE_CAT(i_edge))
                        {
                        case ET_FALLTHROUGH:
                        case ET_IPFALLTHRU:
                                /* we have found an incoming FALLTHROUGH edge,
                                 * follow it upstream */
                                i_bbl = CFG_EDGE_HEAD(i_edge);
                                cont = TRUE;
                                break;

                        case ET_RETURN:
                                /* we have found a RETURN edge,
                                 * look at the call site of the corresponding call. */
                                i_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge));
                                cont = TRUE;
                                break;

                        case ET_JUMP:
                        case ET_IPJUMP:
                                /* the fallthrough path stops here */
                                break;

                        case ET_SWITCH:
                        case ET_IPSWITCH:
                                tbb_bbl = CFG_EDGE_HEAD(i_edge);
                                break;

                        default:
                                break;
                        }
                }
        } while (cont && i_bbl);

        if (tbb_bbl)
        {
                if (BBL_INS_LAST(tbb_bbl) &&
                        (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(tbb_bbl)))==ARM_T2TBB || ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(tbb_bbl)))==ARM_T2TBH))
                {
                        VERBOSE(1,("@F not inlined at @B because the BBL is part of a FALLTHROUGH path originating in a case of a TBB/TBH instruction @I", i_fun, call_site, BBL_INS_LAST(tbb_bbl)));
                        flag = FALSE;
                }
        }
   }

    if (!flag)
      VERBOSE(1,("@F NO INLINE: PROBLEM WITH RETURN\n",i_fun));
    else if (nr_callers==1 && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(call_site)))==ARM_BL /*&& !ARM_INS_IS_CONDITIONAL(T_ARM_INS(BBL_INS_LAST(call_site)))*/)
      {
        //        if (total_count++ < diablosupport_options.debugcounter)
          ArmInlineFunAtCallsite(i_fun,call_site);
          nr_inlined++;
      }
  }

  VERBOSE(1, ("%d functions inlined", nr_inlined));
}
/* }}} */

void ArmInsCreateBranch(t_arm_ins * ins)
{
  if (ARM_INS_CONDITION(ins) != ARM_CONDITION_AL)
        ArmInsMakeCondBranch(ins,ARM_INS_CONDITION(ins));
  else
        ArmInsMakeUncondBranch(ins);
  ASSERT(ArmInsIsEncodable(ins), ("instruction not encodable anymore @I", ins));
}

/*!
 * \todo Document
 *
 * \param fun
 * \param call_site
 *
 * \return void 
*/
/* ArmInlineFunAtCallsite {{{ */
void ArmInlineFunAtCallsite(t_function * fun, t_bbl *call_site)
{
  t_bbl* entry;
  t_bbl* i_bbl;
  t_cfg_edge * call_edge, * tmp;
  t_cfg_edge * return_edge;  
  t_cfg_edge * i_edge, *j_edge;

  t_bbl * tmp_bbl;
  t_bbl * return_point = NULL;

  t_bool last_is_ip=FALSE;
  int gain = 0;

  /* Change the call of the last ins into a branch instruction */
  ArmInsCreateBranch(T_ARM_INS(BBL_INS_LAST(call_site)));

  /* Look at all the predecessors of the entry block of the function, to find
   * the call edge */

  entry = FUNCTION_BBL_FIRST(fun);
  BBL_FOREACH_PRED_EDGE_SAFE(entry,call_edge,tmp)
  {
    t_bbl* site = CFG_EDGE_HEAD(call_edge);
    if (site!=call_site) continue;
    break;
  }      
  if (!call_edge)
  {
    BBL_FOREACH_PRED_EDGE(entry,call_edge)
      VERBOSE(0,("pred @E\n",call_edge));
    FATAL(("Could not find a call edge for call site @iB",call_site));
  }

  /* Determine the return edge and return site, if any */
  return_edge = CFG_EDGE_CORR(call_edge);
  return_point = return_edge?CFG_EDGE_TAIL(return_edge):NULL;

  /* Now that we have all relevant points, remove the call and return edge */
  CfgEdgeKill(call_edge);
  if (return_edge) 
    CfgEdgeKill(return_edge);

  /* Add the jump that replaces the call */
  CfgEdgeCreate(FUNCTION_CFG(fun),call_site,FUNCTION_BBL_FIRST(fun),ET_JUMP);

  /* Add all blocks but the exit block to the calling function */
  FUNCTION_FOREACH_BBL_SAFE_R(fun,i_bbl,tmp_bbl)
    {
      if (i_bbl!=FunctionGetExitBlock(fun))
	{	
	  BBL_SET_NEXT_IN_FUN(i_bbl, BBL_NEXT_IN_FUN(call_site));
	  if (BBL_NEXT_IN_FUN(call_site)) 
	    BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(call_site), i_bbl);
	  BBL_SET_NEXT_IN_FUN(call_site,  i_bbl);
	  BBL_SET_PREV_IN_FUN(i_bbl,  call_site);
	  BBL_SET_FUNCTION(i_bbl, BBL_FUNCTION(call_site));
	}
    }

  while (BBL_NEXT_IN_FUN(FUNCTION_BBL_LAST(BBL_FUNCTION(call_site))))
    FUNCTION_SET_BBL_LAST(BBL_FUNCTION(call_site),
	BBL_NEXT_IN_FUN(FUNCTION_BBL_LAST(BBL_FUNCTION(call_site))));

  if (FunctionGetExitBlock(fun) == FUNCTION_BBL_LAST(fun))
  {
    FUNCTION_SET_BBL_FIRST(fun, FUNCTION_BBL_LAST(fun));
    BBL_SET_PREV_IN_FUN(FUNCTION_BBL_LAST(fun), NULL);
    BBL_SET_NEXT_IN_FUN(FUNCTION_BBL_LAST(fun), NULL);
  }
  else
  {
    FUNCTION_SET_BBL_FIRST(fun, NULL);
    FUNCTION_SET_BBL_LAST(fun, NULL);
  }


  /* Look at the predecessors of the exit block, to identify all returnsites */
  if (FUNCTION_BBL_LAST(fun))
  {
    BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_LAST(fun),i_edge,tmp)
    {
      t_bbl * head = CFG_EDGE_HEAD(i_edge);
      t_arm_ins * last_arm_ins;
      last_arm_ins = T_ARM_INS(BBL_INS_LAST(head));
      if (!last_arm_ins)
      {
	/*FunctionPrint(fun,PRINT_INSTRUCTIONS); */
	FATAL(("This should be impossible: we encountered a returnsite without instructions. As a return needs an explicit return instruction this is impossible\n"));
      }

      /* Change last instruction of return site */

      /* Moves and bx's are easy: */
      if (ARM_INS_OPCODE(last_arm_ins)==ARM_MOV || ARM_INS_OPCODE(last_arm_ins)==ARM_BX)
      {
        ArmInsCreateBranch(last_arm_ins);
      }

      /* Load multiples:
       * for starters, we assume the stack always grows downward.
       * there are several possibilities:
       * 	- ADS style: ldmia r13!, {...}
       * 	- GCC style: ldmdb rXX, {...,r13,...}
       * 	- possible but not yet encountered: ldmia rXX, {...,r13,...}
       */ 
      else if (ARM_INS_OPCODE(last_arm_ins)==ARM_LDM)
      {
	t_arm_ins * j_ins;

	if ((ARM_INS_FLAGS(last_arm_ins) & FL_USERMODE_REGS)
	    && (ARM_INS_FLAGS(last_arm_ins) & FL_WRITEBACK))
	{
	  /* supervisor mode ldm that loads user mode registers, including the
	   * pc, and uses writeback. If we remove the pc from the list of 
	   * loaded registers, the instruction may not use writeback any 
	   * longer. This makes inlining impossible. We should never have
	   * gotten here in the first place. */
	  FATAL(("@I is a supervisor mode return instruction. Cannot inline!",last_arm_ins));
	}
	
	/* Add a new instruction for the branch (as we cannot remove the last
	 * instruction) */
	j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
	ArmInsInsertAfter(j_ins,last_arm_ins);
        ARM_INS_SET_FLAGS(j_ins, ARM_INS_FLAGS(last_arm_ins) & FL_THUMB);
        ARM_INS_SET_CONDITION(j_ins, ARM_INS_CONDITION(last_arm_ins));
        ArmInsCreateBranch(j_ins);
	gain++;

	/* check for ADS-style instruction */
	if (ARM_INS_REGB(last_arm_ins) == ARM_REG_R13 &&
	    ArmInsWriteBackHappens(last_arm_ins) &&
	    (ARM_INS_FLAGS(last_arm_ins) & FL_DIRUP))
	{
	  /* we can avoid an indexing instruction by loading the value that
	   * comes from the pc into r14. of course r14 may not be used for
	   * anything else then. */
	  if (!(ARM_INS_IMMEDIATE(last_arm_ins) & (1<<14)) &&
	      !RegsetIn(BBL_REGS_LIVE_OUT(head), ARM_REG_R14))
	  {
	    /* do the r14 trick */
	    ARM_INS_SET_IMMEDIATE(last_arm_ins, ARM_INS_IMMEDIATE(last_arm_ins)| (1<<14));
	  }
	  else
	  {
	    /* Add the indexing instruction before the branch, after the LDM */
	    j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
	    ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
	    ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
            ARM_INS_SET_FLAGS(j_ins, ARM_INS_FLAGS(last_arm_ins) & FL_THUMB);
	    ArmInsInsertAfter(j_ins,last_arm_ins);
	    ArmInsMakeAdd(j_ins,ARM_INS_REGB(last_arm_ins),
		ARM_INS_REGB(last_arm_ins),ARM_REG_NONE,4,
		ARM_INS_CONDITION(last_arm_ins));
            ASSERT(ArmInsIsEncodable(j_ins), ("instruction not encodable @I", j_ins));
	    gain++;
	  }
	  ARM_INS_SET_IMMEDIATE(last_arm_ins, ARM_INS_IMMEDIATE(last_arm_ins)& ~(1<<15));
	  if (!ARM_INS_IMMEDIATE(last_arm_ins))
	    ArmInsMakeNoop(last_arm_ins);
	  else
	    ARM_INS_SET_REGS_DEF(last_arm_ins, ArmDefinedRegisters(last_arm_ins));

          if ((ARM_INS_OPCODE(last_arm_ins) == ARM_LDM) || (ARM_INS_OPCODE(last_arm_ins) == ARM_STM))
                ArmInsLoadStoreMultipleToSingle(last_arm_ins);
          ASSERT(ArmInsIsEncodable(last_arm_ins), ("instruction not encodable @I", last_arm_ins));
	}
	/* check for GCC-style instruction */
	else if ((ARM_INS_REGB(last_arm_ins) != ARM_REG_R13) &&
	    !ArmInsWriteBackHappens(last_arm_ins) && 
	    !(ARM_INS_FLAGS(last_arm_ins) & FL_DIRUP) && 
	    (ARM_INS_IMMEDIATE(last_arm_ins) & (1<<13)))
	{
	  /* as we are loading downward in memory, r15 is the first register
	   * that gets loaded now. we can't just not load it because all other
	   * registers would be loaded four bytes too high in memory.
	   * There are two solutions: either add an indexing instruction
	   * _before_ the load or load the stack value in r14 instead of r15
	   */
	  if (!(ARM_INS_IMMEDIATE(last_arm_ins) & (1<<14)) &&
	      !RegsetIn(BBL_REGS_LIVE_OUT(head),ARM_REG_R14))
	  {
	    /* use r14 instead of r15 */
	    ARM_INS_SET_IMMEDIATE(last_arm_ins, ARM_INS_IMMEDIATE(last_arm_ins)| (1<<14));
	  }
	  else
	  {
	    /* the safe way: add an indexing instruction before the ldm */
	    j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
	    ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
	    ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
            ARM_INS_SET_FLAGS(j_ins, ARM_INS_FLAGS(last_arm_ins) & FL_THUMB);
	    ArmInsInsertBefore(j_ins,last_arm_ins);
	    ArmInsMakeSub(j_ins,ARM_INS_REGB(last_arm_ins),ARM_INS_REGB(last_arm_ins),ARM_REG_NONE,4,ARM_INS_CONDITION(last_arm_ins));
            ASSERT(ArmInsIsEncodable(j_ins), ("instruction not encodable @I", j_ins));
	    gain++;
	  }
	  ARM_INS_SET_IMMEDIATE(last_arm_ins, ARM_INS_IMMEDIATE(last_arm_ins)& ~(1<<15));
	  ARM_INS_SET_REGS_DEF(last_arm_ins, ArmDefinedRegisters(last_arm_ins));
          ASSERT(ArmInsIsEncodable(last_arm_ins), ("instruction not encodable @I", last_arm_ins));
	}
	else if (ARM_INS_REGB(last_arm_ins) == ARM_REG_R13 &&
	    !ArmInsWriteBackHappens(last_arm_ins) &&
	    (ARM_INS_FLAGS(last_arm_ins) & FL_DIRUP) && 
	    (ARM_INS_IMMEDIATE(last_arm_ins) & (1<<13)))
	{
	  ARM_INS_SET_IMMEDIATE(last_arm_ins, ARM_INS_IMMEDIATE(last_arm_ins)& ~(1<<15));
          ASSERT(ArmInsIsEncodable(last_arm_ins), ("instruction not encodable @I", last_arm_ins));
	  printf("Inlined %s\n", FUNCTION_NAME(fun));
	}

	else
	{
	  /* This is a kind of return instruction we haven't seen before. */
	  FATAL(("Implement @I as return instruction",last_arm_ins));
	}
      }
      /* LDR's : Remove the LDR, add an indexing instruction if necessary and add the branch */
      else if (ARM_INS_OPCODE(last_arm_ins)==ARM_LDR)
      {
	if ((ARM_INS_REGA(last_arm_ins)!=ARM_REG_R15) || (ARM_INS_REGB(last_arm_ins)!=ARM_REG_R13))
	{
	  FATAL(("Strange last ins @I in @iB!",last_arm_ins,ARM_INS_BBL(last_arm_ins)));
	}
	else
	{
	  if (ArmInsWriteBackHappens(last_arm_ins))
	  {
	    if (ARM_INS_FLAGS(last_arm_ins) & FL_DIRUP)
	    {
	      t_uint32 imm=ARM_INS_IMMEDIATE(last_arm_ins);
	      t_arm_ins * j_ins = ArmInsNewForCfg(FUNCTION_CFG(fun));
	      ARM_INS_SET_CSIZE(j_ins, AddressNew32(4));
	      ARM_INS_SET_OLD_SIZE(j_ins, AddressNew32(0));
              ARM_INS_SET_FLAGS(j_ins, ARM_INS_FLAGS(last_arm_ins) & FL_THUMB);
	      ArmInsInsertAfter(j_ins,last_arm_ins);
              ARM_INS_SET_CONDITION(j_ins, ARM_INS_CONDITION(last_arm_ins));
              ArmInsCreateBranch(j_ins);
	      ArmInsMakeAdd(last_arm_ins,ARM_INS_REGB(last_arm_ins),ARM_INS_REGB(last_arm_ins),ARM_REG_NONE,imm,ARM_INS_CONDITION(last_arm_ins));
              ASSERT(ArmInsIsEncodable(last_arm_ins), ("instruction not encodable @I", last_arm_ins));
	    }
	    else
	      FATAL(("Implement 1"));
	  }
	  else
	  {
              ArmInsCreateBranch(last_arm_ins);
	  //  FATAL(("Implement 2 @I",last_ins));
	  }
	}
      }
      else  FATAL(("NO GOOD LAST INSTRUCTION\n"));


      /* Kill the return edge, add a (IP)JUMP edge */
      CfgEdgeKill(i_edge);
      if (last_is_ip)
      {
	j_edge = CfgEdgeCreate(FUNCTION_CFG(fun),head,return_point,ET_IPJUMP);
	CfgEdgeCreateCompensating(FUNCTION_CFG(fun),j_edge);
      }
      else
      {
	j_edge=  CfgEdgeCreate(FUNCTION_CFG(fun),head,return_point,ET_JUMP);
      }
      CFG_EDGE_SET_CFG(j_edge,  FUNCTION_CFG(fun));
    }
    BblKill(FUNCTION_BBL_LAST(fun));
  }

  VERBOSE(1,("Inlined @F, [gain= %d]",fun,gain));
  FunctionKill(fun);
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker : */
