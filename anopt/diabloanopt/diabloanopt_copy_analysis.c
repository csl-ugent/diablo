/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define DONT_GO_THROUGH_HELL /* +this make the analysis quite a bit faster! don't uncomment*/
/*#define DEBUG_COPY_ANALYSIS  |+ Uncomment for debugging purposes +|*/

#include <diabloanopt.h>
#ifdef I386_SUPPORT
#include "../arch/i386/i386_registers.h"
#endif

EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(equations);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(eqs_in);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(copy_instruction_propagator);

#define BBL_REGS_LIVE_BEFORE(bbl) ((bbl)->regs_defined_in)

/* this avoids having to allocate a set of equations for each call to BblCopyAnalysis */
static t_equations eqs_for_BblCopyAnalysis = NULL; 
static t_equations eqs_for_BblCopyAnalysis_after_split = NULL; 

static void FunctionCopyAnalysis(t_function* fun, t_cfg_edge* edge_in, t_bool during_fixpoint_calculations);
void BblCopyAnalysis(t_bbl * bbl, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations);
void CfgStoreCPInfoInEquations(t_object * obj, t_section * sec);

static t_equations GetAugmentedReturnEqs(t_cfg_edge * ret_edge)
{
  int i;
  t_cfg * cfg=CFG_EDGE_CFG(ret_edge);
  t_cfg_edge * call_edge;
  t_function * fun;
  t_equations before_fun;
  t_equations augmented;
  t_regset preserved;

  call_edge = CFG_EDGE_CORR(ret_edge);
  if (!call_edge) return CFG_EDGE_EQS(ret_edge);
  fun = BBL_FUNCTION(CFG_EDGE_TAIL(call_edge));
  augmented = EquationsNew(cfg);
  before_fun = EquationsNew(cfg);
  EquationsCopy(cfg,CFG_EDGE_EQS(call_edge),before_fun);
#ifdef I386_SUPPORT
  /* this is an ugly hack, but a more general solution would be very long-winded. */
  /* in the equations that are propagated over the link edge, undo the effect the
   * call instruction had on the stack pointer */
  EquationsAdd(cfg,before_fun,I386_REG_ESP,I386_REG_ESP,-4,NULL,NULL);
#endif

  RegsetSetDup(preserved,FUNCTION_REGS_CHANGED(fun));
  RegsetSetDiff(preserved,FUNCTION_REGS_SAVED(fun));
  RegsetSetInvers(preserved);

  RegsetSetDiff(preserved,BBL_REGS_LIVE_OUT(CFG_EDGE_HEAD(CFG_EDGE_CORR(ret_edge))));
  RegsetSetDiff(preserved,BblRegsLiveBefore(CFG_EDGE_TAIL(ret_edge)));

  EquationsCopy(cfg,CFG_EDGE_EQS(ret_edge),augmented);

  for (i=0; i<ZERO_REG(cfg) + 1; i++)
    {
      t_equation tmp;
      if (EquationIsTop(before_fun[i]) || EquationIsBot(before_fun[i])) continue;
      if (!RegsetIn(preserved,i)) continue;

      tmp = before_fun[i];
      if (RegsetIn(preserved,i) && (RegsetIn(preserved,tmp.regb) || tmp.regb == CFG_DESCRIPTION(CFG_EDGE_CFG(ret_edge))->num_int_regs))
	if((!tmp.taga && !tmp.tagb) || (tmp.taga && !tmp.tagb && tmp.regb == CFG_DESCRIPTION(CFG_EDGE_CFG(ret_edge))->num_int_regs))
	  EquationsAdd(cfg,augmented,i,tmp.regb,tmp.constant,tmp.taga,tmp.tagb);

      while (!EquationIsBot(before_fun[tmp.regb]) && !EquationIsTop(before_fun[tmp.regb]))
	{
	  tmp = EquationSum(cfg,i,tmp,tmp.regb,before_fun[tmp.regb]);
	  if (RegsetIn(preserved,i) && (RegsetIn(preserved,tmp.regb) || tmp.regb == CFG_DESCRIPTION(CFG_EDGE_CFG(ret_edge))->num_int_regs))
	    if((!tmp.taga && !tmp.tagb) || (tmp.taga && !tmp.tagb && tmp.regb == CFG_DESCRIPTION(CFG_EDGE_CFG(ret_edge))->num_int_regs))
	      EquationsAdd(cfg,augmented,i,tmp.regb,tmp.constant,tmp.taga,tmp.tagb);
	}
    }

  /*DiabloPrint(stdout,"call to %s coming from @B returning to @B\n",FUNCTION_NAME(fun),CFG_EDGE_HEAD(call_edge),CFG_EDGE_TAIL(ret_edge));
    printf("--------------------\n");
    PrintRegset(stdout,fun->cfg->sec->obj->description,preserved);
    printf("--------------------\n");
    EquationsPrint(before_fun);
    printf("--------------------\n");
    EquationsPrint(augmented);
    printf("====================\n"); */

  EquationsFree(before_fun);
  return augmented;
}

/* {{{ copy analysis initialization*/
/*
  \param sec 

  Perform copy-analysis on the flowgraph of section sec.
*/

static    t_bool init_by_copy_analysis=FALSE;
t_bool CopyAnalysisInit(t_cfg * cfg)
{
  t_function * i_fun;
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;


  CfgInitCopyInstructionPropagator(CFG_OBJECT(cfg));
  CfgInitGetFirstInsOfConditionalBranchWithSideEffect(CFG_OBJECT(cfg));
  if (!BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(eqs_in))
    {
      init_by_copy_analysis=TRUE;
      BblInitEqsIn(cfg);
    }
  else
    CFG_FOREACH_FUN(cfg,i_fun)
      FUNCTION_FOREACH_BBL(i_fun,i_bbl)
	{
	  if(BBL_EQS_IN(i_bbl))
	    {
	      BBL_SET_EQS_IN(i_bbl,  NULL);
	    }
	}

  if (!EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(equations))
    {
      init_by_copy_analysis=TRUE;
      CfgEdgeInitEqs(cfg);
    }
  else
    CFG_FOREACH_EDGE(cfg,i_edge)
      {
	if (CfgEdgeIsInterproc(i_edge))
	  {
	    CFG_EDGE_SET_EQS(i_edge,  NULL);
	  }
      }

  DiabloBrokerCall("CopyAnalysisInit",cfg);
  if (!CFG_COPY_INSTRUCTION_PROPAGATOR(cfg)) return FALSE;
  if (!CFG_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg)) return FALSE;
  return TRUE;
}
/* }}} */

/* {{{ copy analysis */
/*
  \param sec 

  Perform copy-analysis on the flowgraph of section sec.
*/

void CopyAnalysis(t_cfg * cfg)
{
  /* iterators */
  t_function * i_fun;
  t_cfg_edge * i_edge;
  t_bbl * i_bbl;

  eqs_for_BblCopyAnalysis = EquationsNew(cfg);
  eqs_for_BblCopyAnalysis_after_split = EquationsNew(cfg);

  STATUS(START,("Copy analysis"));

  /* for enhanced preciseness, we need information about the changed registers of each function */

  CfgRemoveDeadCodeAndDataBlocks(cfg);

  CfgComputeSavedChangedRegisters(cfg);

  /* {{{ initialization */
  CfgEdgeMarkInit();

  /*
    CFG_FOREACH_BBL(cfg,i_bbl)
    if (BBL_FUNCTION(i_bbl))
    DiabloPrint(stdout,"___ Propping @eB\n",i_bbl);
  */

  CfgUnmarkAllFun(cfg);

  DiabloBrokerCall("PrecomputeCopyPropEvaluation",cfg);
  CFG_FOREACH_FUN(cfg,i_fun)
    {
      FunctionUnmarkAllEdges(i_fun);
      FunctionUnmarkAllBbls(i_fun);
      FUNCTION_FOREACH_BBL(i_fun,i_bbl)
	{
	  BBL_SET_REGS_DEFINED_IN(i_bbl, BblRegsLiveBefore(i_bbl));
	  if(!BBL_EQS_IN(i_bbl))
	    BBL_SET_EQS_IN(i_bbl,  EquationsNew(cfg));
	  else
            EquationsSetAllBot(cfg, BBL_EQS_IN(i_bbl));
	}
    }

  CFG_FOREACH_EDGE(cfg,i_edge)
    {
      CfgEdgeUnmark(i_edge);
      /* Allocate space for equations on the interprocedural edges */
      if (CfgEdgeIsInterproc(i_edge))
	{
	  CFG_EDGE_SET_EQS(i_edge,  EquationsNew(cfg));
	}
    }
  /* }}} */
  
  /* 
     Copy analysis is started at the entry point of the
     program. This is not done through incoming edges, therefore the
     edge is null. As we want to propagate the outgoing information
     over outgoing edges, the last parameter is TRUE.
  */

  BBL_FOREACH_SUCC_EDGE(CFG_EXIT_SWI_HELL_NODE(cfg),i_edge)
    {
      /* propagation at the beginning of the program should start with BOT. */
      EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      CfgEdgeMark(i_edge);
    }

  BBL_FOREACH_PRED_EDGE(CFG_SWI_HELL_NODE(cfg),i_edge)
    {
      /* propagation at the beginning of the program should start with BOT. */
      EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
    }

  BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg),i_edge)
    {
      /* propagation at the beginning of the program should start with BOT. */
      EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      CfgEdgeMark(i_edge);
    }

  BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg),i_edge)
    {
      /* now that all edges from the unique entry node have been marked,
       * perform the copy analysis on all entry point functions. Don't
       * simply start from BBL_FUNCTION(CFG_UNIQUE_ENTRY_NODE(cfg)),
       * because that is a hell function and we don't go through hell
       */
      FunctionCopyAnalysis(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge)),NULL,TRUE);
    }


#ifdef DONT_GO_THROUGH_HELL
  BBL_FOREACH_PRED_EDGE(CFG_HELL_NODE(cfg),i_edge)
    if (CFG_EDGE_EQS(i_edge))
      {
	CfgEdgeMark(i_edge);
	EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      }
  BBL_FOREACH_PRED_EDGE(CFG_EXIT_HELL_NODE(cfg),i_edge)
    if (CFG_EDGE_EQS(i_edge))
      {
	CfgEdgeMark(i_edge);
	EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      }
  FunctionCopyAnalysis(BBL_FUNCTION(CFG_HELL_NODE(cfg)),NULL,TRUE);

  CFG_FOREACH_HELL_FUNCTION(cfg, i_fun)
  {
    t_bbl *hell =  FUNCTION_BBL_FIRST(i_fun);
    t_bbl *exithell = FUNCTION_BBL_LAST(i_fun);

    BBL_FOREACH_PRED_EDGE(hell, i_edge)
      if (CFG_EDGE_EQS(i_edge))
      {
	CfgEdgeMark(i_edge);
	EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      }

    BBL_FOREACH_PRED_EDGE(exithell, i_edge)
      if (CFG_EDGE_EQS(i_edge))
      {
	CfgEdgeMark(i_edge);
	EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      }
    FunctionCopyAnalysis(i_fun,NULL,TRUE);
  }

  BBL_FOREACH_SUCC_EDGE(CFG_EXIT_CALL_HELL_NODE(cfg),i_edge)
    {
      EquationsSetAllBot(cfg,CFG_EDGE_EQS(i_edge));
      CfgEdgeMark(i_edge);
    }
#endif

  /* 
     Actual interprocedural fixpoint stuff: as long as there are
     marked procedures, propagate information over marked (incoming)
     edges through them 
  */
  while (CfgUnmarkFun(cfg,&i_fun))
    while (FunctionUnmarkEdge(i_fun,&i_edge))
      FunctionCopyAnalysis(i_fun, i_edge,TRUE);

  STATUS(STOP,("Copy analysis"));

  CFG_FOREACH_FUN(cfg,i_fun)
    FunctionUnmarkAllBbls(i_fun);

  CFG_FOREACH_FUN(cfg,i_fun)
    {
      if(FUNCTION_BBL_FIRST(i_fun))
	{
          t_bool not_entry_fun;

          not_entry_fun = TRUE;
          BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg),i_edge)
            if (i_fun == BBL_FUNCTION(CFG_EDGE_TAIL(i_edge)))
              {
                not_entry_fun = FALSE;
                break;
              }
	  FunctionCopyAnalysis(i_fun,(t_cfg_edge *) not_entry_fun,FALSE);
#ifdef DEBUG_COPY_ANALYSIS
          VERBOSE(0,("FUNCTION %s",FUNCTION_NAME(i_fun)));
	  FUNCTION_FOREACH_BBL(i_fun,i_bbl)
	    {
	      if(BBL_EQS_IN(i_bbl))
		{
                  VERBOSE(0,("@ieB:\n  equations:",i_bbl));
		  EquationsPrint(cfg,BBL_EQS_IN(i_bbl));
		}	    else
                VERBOSE(0,("No equations for @B!",i_bbl));
	    }
#endif
	}
    }

  CFG_FOREACH_FUN(cfg,i_fun)
    {
      FunctionFreeMarkedSpace(i_fun);
    }
  Free(CFG_MARKED_FUNS(cfg));
  CFG_SET_MARKED_FUNS(cfg,  NULL);
}
/* }}} */
/* Clean up datastructures for Copy Analysis {{{ */
void CopyAnalysisFini(t_cfg * cfg)
{
  t_function * i_fun;
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;

  if((!CFG_COPY_INSTRUCTION_PROPAGATOR(cfg)) || (!CFG_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg)))
  {
    CfgFiniCopyInstructionPropagator(CFG_OBJECT(cfg));
    CfgFiniGetFirstInsOfConditionalBranchWithSideEffect(CFG_OBJECT(cfg));
    if (init_by_copy_analysis) 
    { 
      BblFiniEqsIn(cfg);
      CfgEdgeFiniEqs(cfg);
      init_by_copy_analysis=FALSE;
    }
    return; /* No copy analysis has been done, we don't have to clean up anything */
  }

  Free(eqs_for_BblCopyAnalysis);
  Free(eqs_for_BblCopyAnalysis_after_split);
  eqs_for_BblCopyAnalysis = NULL;
  eqs_for_BblCopyAnalysis_after_split = NULL;

  CFG_FOREACH_FUN(cfg, i_fun)
    FUNCTION_FOREACH_BBL(i_fun, i_bbl)
    {
      if (BBL_EQS_IN(i_bbl))
      {
	EquationsFree (BBL_EQS_IN(i_bbl));
	BBL_SET_EQS_IN(i_bbl, NULL);
      }
    }

  CFG_FOREACH_EDGE(cfg, i_edge)
  {
    if (CFG_EDGE_EQS(i_edge))
    {
      EquationsFree (CFG_EDGE_EQS(i_edge));
      CFG_EDGE_SET_EQS(i_edge, NULL);
    }
  }

  CfgFiniGetFirstInsOfConditionalBranchWithSideEffect(CFG_OBJECT(cfg));
  CfgFiniCopyInstructionPropagator(CFG_OBJECT(cfg));
  if (init_by_copy_analysis)
  {

    BblFiniEqsIn(cfg);
    CfgEdgeFiniEqs(cfg);
  }
  EquationsRealFree();
}
/* }}} */

/* {{{  Propagate information through a procedure 
   PARAMETERS: 
   - fun is the function to propagate through
   - edge_in is NULL if this propagation is not for an
   incoming edge, but for the program entry point 
   - during_fixpoint_calculations is false if no outgoing
   information has to be propagated over interprocedural
   edges
*/
static void FunctionCopyAnalysis(t_function * fun, t_cfg_edge * edge_in, t_bool during_fixpoint_calculations)
{
  t_cfg_edge * i_edge;
  t_bbl * i_bbl;
  t_cfg_edge * edge_out = NULL;
  t_cfg *cfg = FUNCTION_CFG(fun);

  /*  if (edge_in && during_fixpoint_calculations)
      DiabloPrint(stdout,"incoming edge: @E\n", edge_in); */

  FUNCTION_FOREACH_BBL(fun,i_bbl)
    EquationsSetAllTop(cfg,BBL_EQS_IN(i_bbl));

  /* {{{ propagate the necessary information into the procedure */
  if (edge_in)
    {
      if (!during_fixpoint_calculations)
	{
	  /* this means we have to propagate information over all incoming edges into the procedure */
	  FUNCTION_FOREACH_BBL(fun,i_bbl)
	    BBL_FOREACH_PRED_EDGE(i_bbl, i_edge) 
	    {
	      if (CfgEdgeIsInterproc(i_edge) && CfgEdgeIsMarked(i_edge))
		{
		  t_reg i;
		  t_bool stop = FALSE;
		  /* join equations from incoming edge only if the are not TOP */
		  for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1);i++)
		    {
		      if(CFG_EDGE_EQS(i_edge)[i].rega == REG_TOP) 
			{
			  stop = TRUE;
			  break;
			}
		    }
		  if (!stop)
		    {
		      t_equations real_eqs = (CFG_EDGE_CAT(i_edge) == ET_RETURN) ? GetAugmentedReturnEqs(i_edge) : CFG_EDGE_EQS(i_edge);
		      /*DiabloPrint(stdout,"propping interprocedural info from @E\n",i_edge);*/
		      /*EquationsPrint(real_eqs);*/
		      if (EquationsJoin(cfg,BBL_EQS_IN(i_bbl),real_eqs))
			{
			  FunctionMarkBbl(fun,i_bbl);
			}
		      if (real_eqs != CFG_EDGE_EQS(i_edge)) EquationsFree(real_eqs);
		    }
		}
	    }
	}
      else
	{
	  /* 
	     Propagation will start at the tail of the edge, where
	     the incoming information is used (unlike constant propagation,
	     we don't treat hell nodes any different from normal nodes)
	  */
	  t_equations real_eqs = CFG_EDGE_EQS(edge_in);
      
      
	  if (!BBL_IS_HELL(CFG_EDGE_TAIL(edge_in)))
	    EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(edge_in)),real_eqs);
	  else
	    {
#ifdef DONT_GO_THROUGH_HELL
	      FATAL(("Don't want to get here anymore!"));
#endif
	      EquationsSetAllBot(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(edge_in)));
	    }
	  FunctionMarkBbl(fun,CFG_EDGE_TAIL(edge_in));
	  edge_out = CFG_EDGE_CORR(edge_in);

	  if (real_eqs != CFG_EDGE_EQS(edge_in)) EquationsFree(real_eqs);
	}
    }
  else
    {
      /* program entry: no incoming edge */
      if (!during_fixpoint_calculations)
	{
	  FUNCTION_FOREACH_BBL(fun,i_bbl)
	    BBL_FOREACH_PRED_EDGE(i_bbl, i_edge) 
	    {
	      if (CfgEdgeIsForwardInterproc(i_edge))
		if (CfgEdgeIsMarked(i_edge))
		  if (EquationsJoin(cfg,BBL_EQS_IN(i_bbl),CFG_EDGE_EQS(i_edge)))
		    FunctionMarkBbl(fun,i_bbl);
	    }
	}
      FunctionMarkBbl(fun,FUNCTION_BBL_FIRST(fun));
      /* propagation of the program entry point always starts with BOT */
      EquationsSetAllBot(cfg,BBL_EQS_IN(FUNCTION_BBL_FIRST(fun)));
    } /* }}} */

  /* the actual fixpoint calculation */
  while (FunctionUnmarkBbl(fun,&i_bbl))
    BblCopyAnalysis(i_bbl,edge_out,during_fixpoint_calculations);
}
/* }}} */

/* {{{ propagate copy propagation information over an edge */
static void PropOverEdge(t_cfg_edge * edge, t_equations eqs, t_bool during_fixpoint_calculations)
{
  t_cfg_edge * corr_edge;
  t_cfg * cfg=CFG_EDGE_CFG(edge);
#ifdef DEBUG_COPY_ANALYSIS
  t_reg i;

  /* {{{ */
  for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-1;i++)
    {
      if(eqs[i].rega == REG_TOP) FATAL(("Propping top over edge @E\n",edge));
      if(eqs[i].regb == REG_TOP) FATAL(("Propping top over edge @E\n",edge));
    } /* }}} */
#endif

  /*DiabloPrint(stdout,"propping over edge @E\n",edge);*/
  /*EquationsPrint(eqs);*/
  /* mark it as taken */
  CfgEdgeMark(edge);

  if (CfgEdgeIsInterproc(edge))
    {
      /* if it is a true outgoing edge, the corresponding incoming edge must have its 
	 information propagated into the procedure (if it is marked that is). */

      if (diabloanopt_options.rely_on_calling_conventions)
	{
	  t_regset invalid_regs = RegsetNew();
	
	  RegsetSetDup(invalid_regs,BBL_REGS_LIVE_OUT(CFG_EDGE_HEAD(edge)));
	  RegsetSetUnion(invalid_regs,BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)));
	
	  RegsetSetIntersect(invalid_regs,CFG_DESCRIPTION(CFG_EDGE_CFG(edge))->int_registers);
	
	  EquationsInvalidateRegset(cfg,eqs,invalid_regs);
	}
    
      if (CfgEdgeIsForwardInterproc(edge))
	{
	  corr_edge = CFG_EDGE_CORR(edge);
	  if (corr_edge && CfgEdgeIsMarked(corr_edge))
	    {
	      {
		/*	  DiabloPrint(stdout,"propping over corr_edge @E\n",corr_edge);*/
		t_equations real_eqs = (CFG_EDGE_CAT(corr_edge) == ET_RETURN) ? GetAugmentedReturnEqs(corr_edge) : CFG_EDGE_EQS(corr_edge);
		/*	  EquationsPrint(real_eqs);*/
		if (EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(corr_edge)),real_eqs))
		  FunctionMarkBbl(BBL_FUNCTION(CFG_EDGE_HEAD(edge)),CFG_EDGE_TAIL(corr_edge));
		if (real_eqs != CFG_EDGE_EQS(corr_edge)) EquationsFree(real_eqs);
	      } 
	    }
	}
    
      if (during_fixpoint_calculations)
	{
#ifdef DONT_GO_THROUGH_HELL
	  if (!BBL_IS_HELL(CFG_EDGE_TAIL(edge)) && EquationsJoin(cfg,CFG_EDGE_EQS(edge),eqs))
#else
	    if (EquationsJoin(CFG_EDGE_EQS(edge),eqs))
#endif
	      {
		if (!CfgEdgeIsForwardInterproc(edge))
		  {
		    /* repropagation for all true incoming edges of the function this edge points to is necessary */
		    CfgMarkFun(FUNCTION_CFG(BBL_FUNCTION(CFG_EDGE_TAIL(edge))),BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
		    FunctionMarkAllTrueIncomingEdges(BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
		  }
		else 
		  {
		    /* only this edge will need to be repropagated in the function it points to */
		    CfgMarkFun(FUNCTION_CFG(BBL_FUNCTION(CFG_EDGE_TAIL(edge))),BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
		    FunctionMarkEdge(BBL_FUNCTION(CFG_EDGE_TAIL(edge)),edge);
		  }
	      }
	}
    }
  else 
    {
      /* the edge was INTRAprocedural */
    
      /*    DiabloPrint(stdout,"propping over intraproc edge @E\n",edge); */
      /*    EquationsPrint(eqs);*/

      if (EquationsJoin(cfg,BBL_EQS_IN(CFG_EDGE_TAIL(edge)),eqs))
	FunctionMarkBbl(BBL_FUNCTION(CFG_EDGE_TAIL(edge)),CFG_EDGE_TAIL(edge));
    }
}
/* }}} */

/* {{{ Propagate a state over a bbl and to the successor edges */
void BblCopyAnalysis(t_bbl * bbl, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations)
{
  t_cfg * cfg=BBL_CFG(bbl);
  t_cfg_edge * i_edge;
  t_ins * i_ins, *last_ins=NULL;
  t_equations eqs = eqs_for_BblCopyAnalysis;
  t_equations eqs_after_split = eqs_for_BblCopyAnalysis_after_split;
  t_bool ignore_condition = FALSE;

  CFG_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(BBL_CFG(bbl))(bbl,&last_ins);

  EquationsCopy(cfg,BBL_EQS_IN(bbl),eqs);
#ifdef DEBUG_COPY_ANALYSIS
  { /* {{{ */
    t_reg i;
    for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-1;i++)
      {
	if(eqs[i].rega == REG_TOP) FATAL(("Propping top in bbl @B\n",bbl));
	if(eqs[i].regb == REG_TOP) FATAL(("Propping top in bbl @B\n",bbl));
      }
  } /* }}} */
#endif

  /*  DiabloPrint(stdout,"___ propping @eB\n",bbl);*/
  BBL_FOREACH_INS(bbl,i_ins)
    {
      if (last_ins == i_ins)
      {
	EquationsCopy (cfg, eqs, eqs_after_split);
	ignore_condition = TRUE;
      }
      /*DiabloPrint(stdout,"----------------------------------\n");*/
      /*EquationsPrint(eqs);*/
      /*DiabloPrint(stdout,"CA: @I\n",i_ins);*/
      CFG_COPY_INSTRUCTION_PROPAGATOR(BBL_CFG(bbl))(i_ins,eqs,ignore_condition);
      /*EquationsPrint(eqs); */
    }

  /* special hack: for hell nodes, this is necessary to ensure
   * propagation of _all_ the registers on the outgoing edges */
  if (BBL_IS_HELL(bbl))
    EquationsSetAllBot(cfg,eqs);

#ifdef DEBUG_COPY_ANALYSIS
  { /* {{{ */
    t_reg i;
    for(i=0;i<(CFG_DESCRIPTION(cfg)->num_int_regs + 1)-1;i++)
      {
	if(eqs[i].rega == REG_TOP) FATAL(("Propping top over bbl @B\n",bbl));
	if(eqs[i].regb == REG_TOP) FATAL(("Propping top over bbl @B\n",bbl));
      }
  } /* }}} */
#endif

  /* propagate the outgoing information over the outgoing edges */
  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
    {
      t_bool do_it;
      do_it = !CfgEdgeIsBackwardInterproc(i_edge);
      /* propagate over backward interprocedural edges if they correspond
       * with the incoming edge for which the function was propagated, or if
       * there isn't an incoming edge (for the program entry point) */
      do_it = do_it || (i_edge == edge_out || edge_out == NULL);
      if (do_it)
	{
	  if (last_ins && CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH)
	    {
	      PropOverEdge(i_edge, eqs_after_split, during_fixpoint_calculations);
	    }
	  else
	    {
	      PropOverEdge(i_edge, eqs, during_fixpoint_calculations);
	    }
	}
    }
}
/* }}} */

/* {{{ BblCopyAnalysisUntilIns */
void BblCopyAnalysisFromInsToUntilIns(t_ins * from_ins, t_ins * to_ins, t_equations eqs)
{
  t_cfg * cfg=INS_CFG(from_ins);
  t_bool ignore_condition = FALSE;

  while (from_ins!=to_ins)
    {
      CFG_COPY_INSTRUCTION_PROPAGATOR(cfg)(from_ins,eqs,ignore_condition);
      from_ins=INS_INEXT(from_ins);
    }
}

void BblCopyAnalysisUntilIns(t_ins * ins, t_equations eqs)
{

  /* iterators */

  t_cfg * cfg=INS_CFG(ins);
  t_ins * i_ins;
  t_bbl * bbl = INS_BBL(ins);
  t_bool ignore_condition = FALSE;

  /*  if(diabloanopt_options.copy_analysis)*/
  {

    if(!bbl) FATAL(("No bbl for this ins @I",ins));
    if(!eqs) FATAL(("No equations!"));
    if(BBL_EQS_IN(bbl))
      EquationsCopy(cfg,BBL_EQS_IN(bbl),eqs);
    else
      EquationsSetAllBot(cfg,eqs);

    if (!CFG_COPY_INSTRUCTION_PROPAGATOR(BBL_CFG(bbl))) return;
    BBL_FOREACH_INS(bbl,i_ins)
      {
	/*      DiabloPrint(stdout,"----------------------------------\n");
		EquationsPrint(eqs);
		DiabloPrint(stdout,"CA: @I\n",i_ins); */
	if(i_ins == ins) break;
	CFG_COPY_INSTRUCTION_PROPAGATOR(BBL_CFG(bbl))(i_ins,eqs,ignore_condition);
	/*      EquationsPrint(eqs);*/
      }
  }

  return;
} /* }}} */

#if 0
void CfgStoreCPInfoInEquations(t_object * obj, t_section * sec)
{
  t_bbl * i_bbl;
  t_ins * i_ins;
  t_function * i_fun, * entry_fun;
  t_cfg * cfg = OBJECT_CFG(obj);
  t_regset tmp_set;
  t_reg i_reg,j_reg;
  t_register_content c1, c2;
  t_bbl * program_entry;
  
  t_procstate * prev_state;
  t_procstate * next_state;

  t_regset allregs = obj->description->all_registers;
  
  prev_state = ProcStateNew();
  next_state = ProcStateNew();
  
  program_entry = cfg->entries->entry_bbl;
  entry_fun = BBL_FUNCTION(program_entry);

  /* first of all, if the instructions in the graph already contain regstates, remove them */
  CFG_FOREACH_BBL(cfg,i_bbl)
    {
      BBL_FOREACH_INS(i_bbl,i_ins)
	{
	  /*      if (INS_USED_REGSTATE(i_ins)) Free(INS_USED_REGSTATE(i_ins));
	        if (INS_DEFINED_REGSTATE(i_ins)) Free(INS_DEFINED_REGSTATE(i_ins));
	        INS_SET_SET_USED_REGSTATE(i_ins,  INS_DEFINED_REGSTATE(i_ins,  NULL));
	  */
	}
    }

  CFG_FOREACH_FUN(cfg,i_fun)
    {
      FunctionUnmarkAllBbls(i_fun);
    }

  CFG_FOREACH_FUN(cfg,i_fun)
    {
      if(FUNCTION_BBL_FIRST(i_fun))
	{
	  FunctionPropagateConstants(obj,i_fun,sec,T_CFG_EDGE(i_fun!=entry_fun),FALSE);

	  FUNCTION_FOREACH_BBL(i_fun,i_bbl)
	    {
	      if (!RegsetIsMutualExclusive(((t_procstate*)BBL_PROCSTATE_IN(i_bbl))->top,allregs))
		continue;
	
	      tmp_set = RegsetNewInvers(((t_procstate*)BBL_PROCSTATE_IN(i_bbl))->bot);
	      REGSET_FOREACH_REG(tmp_set,i_reg)
		break;
	
	      if(i_reg < (CFG_DESCRIPTION(cfg)->num_int_regs + 1)-1)
		{
		  if(ProcStateGetReg(BBL_PROCSTATE_IN(i_bbl),i_reg,&c1)) FATAL(("Not a constant?"));
		  REGSET_FOREACH_REG(tmp_set,j_reg)
		    {
		      if(i_reg == j_reg) continue;
		      if(j_reg >= (CFG_DESCRIPTION(cfg)->num_int_regs + 1)) break;
		      if(ProcStateGetReg(BBL_PROCSTATE_IN(i_bbl),j_reg,&c2))
			FATAL(("Not a constant?"));
		      EquationInsert(BBL_EQS_CTE(i_bbl),i_reg,j_reg,c2.i-c1.i,NULL,NULL);
		    }
		  DiabloPrint(stdout,"In @B equations are:\n",i_bbl);
		  EquationsPrint(BBL_EQS_CTE(i_bbl));
		}
	    }
	  {
	    ProcStateFree(BBL_PROCSTATE_IN(i_bbl));
	  }

	}
    }
  ProcStateFree(prev_state);
  ProcStateFree(next_state);
}
#endif

/* vim: set shiftwidth=2 foldmethod=marker: */
