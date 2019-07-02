/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/********************************
  IMPORTANT NOTICE: from now on, constant propagation only operates
  on graphs with single-entry - single exit procedures, and on graphs
  without ET_IPFUNLINK edges.
 *******************************/

/*#define DEBUG_CONST_PROP*/
#define NO_CONSTPROP_OVER_HELL

#include <stdlib.h>
#include <diabloanopt.h>

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(procstate_in);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(procstate_out);

CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(edge_propagator);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(instruction_constant_optimizer);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(get_first_ins_of);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(instruction_emulator);

EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(testcondition);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(procstate);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(args);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(args_changed);

FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(nargs);

INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(usearg);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(defarg);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(defedge);

EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(conditional_procstate);

t_procstate * conditional_state;

t_bool constprop_enable_helpers = FALSE;
void ConstantPropagationHelpers(t_bool state)
{
        constprop_enable_helpers = state;
}

t_bool disable_transformations = FALSE;
void ConstantPropagationDisableTransformations(t_bool state)
{
  disable_transformations = state;
}

t_bool is_advanced_factoring_phase = FALSE;
void ConstantPropagationAdvancedFactoringPhase(t_bool state)
{
  is_advanced_factoring_phase = state;
}

/*TODO
 * this is an ugly hack, but if we don't do this we have to change the function
 * signature of just about _all_ constant propagation related functions to
 * accomodate the extra t_argstate* argument */
t_argstate *current_argstate = NULL;


t_uint32 global_optimization_phase=0;

t_procstate * temp_procstate;
t_procstate * temp_procstate2;
t_procstate * temp_procstate3;

/* Defines for static functions {{{ */
static void FunctionPropagateBotThrough(t_function * fun, t_analysis_complexity complexity);
void FunctionPropagateConstantsDuringIterativeSolution(t_function * function, t_cfg_edge * edge_in, t_analysis_complexity complexity);
t_bool FunctionUnmarkBbl(t_function * fun, t_bbl** bbl);
/*}}}*/

t_bool
ConstantPropagationInit(t_cfg * cfg)
{
  CfgEdgeInitProcstate(cfg);
  BblInitProcstateIn(cfg);
  BblInitProcstateOut(cfg);
  CfgEdgeInitTestCondition(cfg);
  CfgInitInstructionEmulator(CFG_OBJECT(cfg));
  CfgInitInstructionConstantOptimizer(CFG_OBJECT(cfg));
  CfgInitEdgePropagator(CFG_OBJECT(cfg));
  CfgInitGetFirstInsOfConditionalBranchWithSideEffect(CFG_OBJECT(cfg));
  CfgEdgeInitConditionalProcstate(cfg);

  /* argstate forwarding init */
  InsInitUseArg(cfg);
  InsInitDefArg(cfg);
  InsInitDefEdge(cfg);
  FunctionInitNargs(cfg);
  CfgEdgeInitArgs(cfg);
  CfgEdgeInitArgsChanged(cfg);

  DiabloBrokerCall("ConstantPropagationInit",cfg);

  if (!CFG_INSTRUCTION_EMULATOR(cfg))
  {
    WARNING(("Architecture does not support constant propagation (instruction emulator not set)"));
    return FALSE;
  }
  else if (!CFG_INSTRUCTION_CONSTANT_OPTIMIZER(cfg))
  {
    WARNING(("Architecture does not support constant propagation (instruction constant optimizer not set)"));
    return FALSE;
  }
  else if (!CFG_EDGE_PROPAGATOR(cfg))
  {
    WARNING(("Architecture does not support constant propagation (edge propagator not set)"));
    return FALSE;
  }
  else if (!CFG_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg))
  {
    WARNING(("Architecture does not support constant propagation (GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT not set)."));
    return FALSE;
  }

  return TRUE;
}

void
ConstantPropagationFini(t_cfg * cfg)
{
  t_cfg_edge *edge;

  CfgEdgeFiniProcstate(cfg);
  BblFiniProcstateIn(cfg);
  BblFiniProcstateOut(cfg);
  CfgFiniInstructionEmulator(CFG_OBJECT(cfg));
  CfgFiniInstructionConstantOptimizer(CFG_OBJECT(cfg));
  CfgFiniEdgePropagator(CFG_OBJECT(cfg));
  CfgFiniGetFirstInsOfConditionalBranchWithSideEffect(CFG_OBJECT(cfg));
  CfgEdgeFiniTestCondition(cfg);
  CfgEdgeFiniConditionalProcstate(cfg);

  /* argstate forwarding cleanup */
  CFG_FOREACH_EDGE(cfg, edge)
    if (CFG_EDGE_ARGS(edge))
      ArgStateFree(CFG_EDGE_ARGS(edge));
  CfgEdgeFiniArgs(cfg);
  InsFiniUseArg(cfg);
  InsFiniDefArg(cfg);
  InsFiniDefEdge(cfg);
  FunctionFiniNargs(cfg);
  CfgEdgeFiniArgsChanged(cfg);

  DiabloBrokerCall("ConstantPropagationFini",cfg);
}

static void GenericSetjmpReturnEdgePropagator(t_cfg_edge * edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **conditional_state)
{
  t_architecture_description *desc = CFG_DESCRIPTION(CFG_EDGE_CFG(edge));
  ProcStateSetRegsetBot(state, desc->return_regs);
  *state_false = state;
  *state_true = ProcStateNew(desc);
  ProcStateDup(*state_true, state, desc);
}

/* ConstantPropagation {{{ */
/*!
 *
 * \param cfg
 *
 * \param complexity
 *
 * Now we come to the actual constant propagation implementation.
 */
void
ConstantPropagation(t_cfg * cfg, t_analysis_complexity complexity)
{
  /* iterators */
  t_function * i_fun;
  t_cfg_edge * i_edge;
  t_ins * i_ins;
  t_bbl * i_bbl;

  /* local variables */
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);

  /* The following are temporary variables that we don't want to allocate over
   * and over again. Instead we allocate them once, and reuse them often. */
  temp_procstate = ProcStateNew(desc);
  temp_procstate2 = ProcStateNew(desc);
  temp_procstate3 = ProcStateNew(desc);

  STATUS(START,("Constant Propagation"));

  /* In case we are propagating context-insensitive, constant information
   * (procstates) will be stored on the first interprocedural incoming edge.
   * To optimize the handling of conditional branches, we sort predecessor
   * edges of nodes first */

  if (complexity == CONTEXT_INSENSITIVE)
  {
    CFG_FOREACH_FUN(cfg,i_fun)
    {
      t_bbl * first_bbl = FUNCTION_BBL_FIRST(i_fun);
      if (BBL_IS_HELL(first_bbl)) continue;
      if (BBL_PRED_FIRST(first_bbl))
	while (!CfgEdgeIsInterproc(BBL_PRED_FIRST(first_bbl)))
	{
	  CfgEdgeMoveFirstToLastPred(BBL_PRED_FIRST(first_bbl));
	}
    }
  }

  /* in order to optimize the handling of conditional branches, we
     sort successor edges of nodes first */

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
      if (CfgEdgeIsFallThrough(i_edge))
      {
	CfgEdgeMoveToLastSucc(i_edge);
	break;
      }

    BBL_SET_REGS_DEFINED_IN(i_bbl, BblRegsLiveBefore(i_bbl));
  }

  /* Our algorithms use extensive marking of blocks (meaning adding blocks to
   * todo-worklists */
  /* All linked lists of to be evaluated items should be emptied. */

  EdgeMarkInit();

  CfgUnmarkAllFun(cfg);

  CFG_FOREACH_FUN(cfg,i_fun)
  {
    FunctionUnmarkAllEdges(i_fun);
    FunctionUnmarkAllBbls(i_fun);
  }

  /* All instructions are initially assumed not executed (not reachable), as
   * this constant propagation includes a reachable code detection. We also
   * assume that all (conditional) instructions, are always executed once they
   * are reachable.
   *
   * The IF_EXECED flag needs to be set during the instruction evaluation for
   * instructions that might be executed (are reachable and, for conditional
   * instructions, for which the condition flags might be true), the
   * IF_ALWAYS_EXECED flag needs to be unset for conditional instructions if
   * their condition might evaluate to false.
   *
   * As a result, when the constant propagation has finished, instructions in
   * reachable blocks with IF_EXECED not set are turned into NOOPS. Instructions
   * in unreachable blocks are removed turned into deadcode. Conditional
   * instructions which have IF_EXECED and IF_ALWAYS_EXECUTED set are
   * unconditionalized.
   */
  {
    t_uint32 simple_leaf_counter = 0;
    t_uint32 other_fun_counter = 0;

    CFG_FOREACH_FUN(cfg,i_fun)
      {
        int bbl_counter = 0;
        t_bool form_simple_leaf = TRUE;
        FUNCTION_FOREACH_BBL(i_fun,i_bbl)
          {
            BBL_FOREACH_INS(i_bbl,i_ins)
              {
                INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) & (~IF_EXECED));
                INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) | IF_ALWAYS_EXECED);
              }
            bbl_counter++;
          }
        if (bbl_counter!=2)
          form_simple_leaf = FALSE;
        else
          {
            t_bbl * first_bbl = FUNCTION_BBL_FIRST(i_fun);
            t_cfg_edge * edge;
            if (BBL_IS_HELL(first_bbl)) continue;
            BBL_FOREACH_PRED_EDGE(first_bbl, edge)
              if (CFG_EDGE_CAT(edge)!=ET_CALL)
                {
                  form_simple_leaf = FALSE;
                  break;
                }
            BBL_FOREACH_SUCC_EDGE(first_bbl, edge)
              if (CFG_EDGE_CAT(edge)!=ET_JUMP)
                {
                  form_simple_leaf = FALSE;
                  break;
                }
          }
        if (form_simple_leaf)
          {
            //            DEBUG(("FOUND A FUNCTION WITH BBL @ieB",FUNCTION_BBL_FIRST(i_fun)));
            FUNCTION_SET_FLAGS(i_fun,FUNCTION_FLAGS(i_fun) | FF_IS_SIMPLE_LEAF);
            simple_leaf_counter++;
          }
        else
          {
            FUNCTION_SET_FLAGS(i_fun,FUNCTION_FLAGS(i_fun) &~ FF_IS_SIMPLE_LEAF);
            other_fun_counter++;
          }
      }
    //    DEBUG(("function distribution: %d %d", simple_leaf_counter, other_fun_counter));
  }
  /* As long as calling conventions are assumed to be respected, we should not
   * propagate values into functions that are dead (because making them alive
   * over the call edge will result in calling conventions being broken). For
   * that reason, we need liveness information. */

  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);

  BBL_SET_REGS_LIVE_OUT(CFG_EXIT_CALL_HELL_NODE(cfg), desc->all_registers);
  BBL_SET_REGS_LIVE_OUT(CFG_CALL_HELL_NODE(cfg), desc->all_registers);

  /* No edge are initially assumed reachable, ProcStates are allocated, the
   * architecture dependent evaluation of whether information needs to be
   * propagated over an edge is initialized, and the registers to be
   * propagated over edges is initialized.
   *
   * The latter for the moment is the same for all edges, as we do not have a
   * saved-changed register analysis for procedures yet. */

  CFG_FOREACH_EDGE(cfg,i_edge)
  {
    CfgEdgeUnmark(i_edge);
    CFG_EDGE_SET_PROCSTATE(i_edge,  NULL);
    if (CfgEdgeIsInterproc(i_edge))
    {
      if (
	  (complexity==CONTEXT_SENSITIVE)
	  || (CfgEdgeIsForwardInterproc(i_edge) && i_edge == BBL_PRED_FIRST(CFG_EDGE_TAIL(i_edge)))
	  || (CfgEdgeIsBackwardInterproc(i_edge) && i_edge == BBL_SUCC_FIRST(CFG_EDGE_HEAD(i_edge)))
	 )
	CFG_EDGE_SET_PROCSTATE(i_edge,  ProcStateNew(desc));
    }

    CFG_EDGE_PROPAGATOR (cfg)(i_edge,BBL_INS_LAST(CFG_EDGE_HEAD(i_edge)));

    CFG_EDGE_SET_PROP_REGS(i_edge, desc->all_registers);

    if (CFG_EDGE_CAT(i_edge)==ET_RETURN)
    {
      if (CFG_EDGE_CORR(i_edge) && CFG_EDGE_CAT(CFG_EDGE_CORR(i_edge))==ET_SWI)
	CFG_EDGE_SET_PROP_REGS(i_edge, NullRegs);
      else
	CFG_EDGE_SET_PROP_REGS(i_edge, RegsetIntersect(CFG_EDGE_PROP_REGS(i_edge),FUNCTION_REGS_CHANGED(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge)))));
    }
  }

  /* SPECIAL CASE: return edges coming from setjmp:
   * here we have to set all return registers to BOT because their value will differ
   * depending on whether we reach the return site from a call to setjmp() or from
   * a call to longjmp() */
  CFG_FOREACH_FUN(cfg, i_fun)
  {
    if (FUNCTION_FLAGS(i_fun) & FF_IS_SETJMP)
    {
      t_bbl *exitblock = FunctionGetExitBlock(i_fun);
      t_cfg_edge *edge;

      ASSERT(exitblock, ("setjmp without exit block? check function %s", FUNCTION_NAME(i_fun)));

      BBL_FOREACH_SUCC_EDGE(exitblock, edge)
	CFG_EDGE_SET_TESTCONDITION(edge, GenericSetjmpReturnEdgePropagator);
    }
  }

  /* Constant propagation is started at the entry point(s) of the program. This
   * is not done through incoming edges, therefore the edge is null. As we
   * want to propagate the outgoing information over outgoing edges, the last
   * but one parameter is TRUE.  As we want to propagate over all backward
   * outgoing edges, the LAST parameter is true as well.  */

  BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg),i_edge)
  {
    CfgMarkFun(cfg, BBL_FUNCTION(CFG_EDGE_TAIL(i_edge)));
    ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge), desc->all_registers);
    FunctionMarkEdge(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge)), i_edge);
    CfgEdgeMark(i_edge);
  }

  BBL_FOREACH_SUCC_EDGE(CFG_EXIT_SWI_HELL_NODE(cfg),i_edge)
  {
    if (CFG_EDGE_PROCSTATE(i_edge))
      ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),CFG_DESCRIPTION(cfg)->all_registers);
    CfgEdgeMark(i_edge);
  }

  BBL_FOREACH_PRED_EDGE(CFG_SWI_HELL_NODE(cfg),i_edge)
  {
    if (CFG_EDGE_PROCSTATE(i_edge))
      ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),CFG_DESCRIPTION(cfg)->all_registers);
  }

#ifdef NO_CONSTPROP_OVER_HELL
  t_function *hell_fun = CFG_HELL_FUNCTIONS(cfg);
  while (hell_fun) {
    if (FUNCTION_NAME(hell_fun)
        && StringPatternMatch("*GLOBALTARGETHELL*", FUNCTION_NAME(hell_fun)))
    {
      BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(hell_fun), i_edge) {
        if (CFG_EDGE_PROCSTATE(i_edge)) {
          ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),RegsetNewInvers(CFG_DESCRIPTION(cfg)->registers_prop_over_hell,CFG_DESCRIPTION(cfg)->all_registers));
        }
      }
    }
    hell_fun = FUNCTION_NEXT_HELL(hell_fun);
  }

  /* We want to avoid propagation over hell every time something changes on the
   * incoming edges of the hell nodes. Therefore, we just set all procstates on
   * these incoming edges to BOT */
  BBL_FOREACH_PRED_EDGE(CFG_HELL_NODE(cfg),i_edge)
  {
    if (CFG_EDGE_PROCSTATE(i_edge))
    {
      ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),RegsetNewInvers(CFG_DESCRIPTION(cfg)->registers_prop_over_hell,CFG_DESCRIPTION(cfg)->all_registers));
    }
  }

  BBL_FOREACH_PRED_EDGE(CFG_EXIT_HELL_NODE(cfg),i_edge)
  {
    if (CFG_EDGE_PROCSTATE(i_edge))
    {
      ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),RegsetNewInvers(CFG_DESCRIPTION(cfg)->registers_prop_over_hell,CFG_DESCRIPTION(cfg)->all_registers));
    }
  }
  FunctionPropagateBotThrough(BBL_FUNCTION(CFG_HELL_NODE(cfg)), complexity);

  CFG_FOREACH_HELL_FUNCTION(cfg, i_fun)
  {
    t_bbl *hell = FUNCTION_BBL_FIRST(i_fun);
    t_bbl *exithell = FUNCTION_BBL_LAST(i_fun);

    if (!BBL_CALL_HELL_TYPE(hell)) continue;

    BBL_FOREACH_PRED_EDGE(hell,i_edge)
    {
      if (CFG_EDGE_PROCSTATE(i_edge))
      {
	ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),RegsetNewInvers(CFG_DESCRIPTION(cfg)->registers_prop_over_hell,CFG_DESCRIPTION(cfg)->all_registers));
      }
    }

    BBL_FOREACH_PRED_EDGE(exithell,i_edge)
    {
      if (CFG_EDGE_PROCSTATE(i_edge))
      {
	ProcStateSetAllBot(CFG_EDGE_PROCSTATE(i_edge),RegsetNewInvers(CFG_DESCRIPTION(cfg)->registers_prop_over_hell,CFG_DESCRIPTION(cfg)->all_registers));
      }
    }

    FunctionPropagateBotThrough(i_fun, complexity);
  }
#endif

  /* Actual interprocedural fixpoint stuff: as long as there are marked
   * procedures, propagate information over marked (incoming) edges through
   * them. */

  int counter = 0;

  while (CfgUnmarkFun(cfg,&i_fun))
  {
    while (FunctionUnmarkEdge(i_fun,&i_edge))
    {
      counter++;
      FunctionPropagateConstantsDuringIterativeSolution(i_fun, i_edge, complexity);
    }
  }

  //  DEBUG(("counter: %d",counter));

  STATUS(STOP,("Constant Propagation"));
}
/* }}} */

/* FreeConstantInformation {{{ */
void
FreeConstantInformation(t_cfg * cfg)
{
  t_cfg_edge * i_edge;
  t_function * i_fun;
  /* Free the allocated space for interprocedural procstates.  */

  CFG_FOREACH_EDGE(cfg,i_edge)
  {
    if (CfgEdgeIsInterproc(i_edge) && CFG_EDGE_PROCSTATE(i_edge))
    {
      ProcStateFree(CFG_EDGE_PROCSTATE(i_edge));
      CFG_EDGE_SET_PROCSTATE(i_edge, NULL);
    }
  }
  CFG_FOREACH_FUN(cfg,i_fun)
  {
    FunctionFreeMarkedSpace(i_fun);
  }

  ProcStateFree(temp_procstate);
  ProcStateFree(temp_procstate2);
  ProcStateFree(temp_procstate3);
  ProcStateRealFree();
  Free(CFG_MARKED_FUNS(cfg));
  CFG_SET_MARKED_FUNS(cfg, NULL);
}
/*}}}*/

/* FunctionPropagateConstants {{{ */

/*! Propagate information through a procedure
 *
 * \par fun
 *
 * \par edge_in if NULL this propagation is not for an incoming edge, but for
 * the program entry point
 *
 * \par during_fixpoint_calculations is false if no outgoing information has to
 * be propagated over interprocedural edges
 */

static void
FunctionPropagateConstantsIteratively(t_function * fun, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations, t_analysis_complexity complexity)
{
  t_bbl * i_bbl;
  while (FunctionUnmarkBbl(fun, &i_bbl))
  {
    BblPropagateConstants(i_bbl, edge_out, during_fixpoint_calculations, complexity);
  }
}

static void
FunctionPropagateConstantsFreeProcstates(t_function * fun)
{
  t_bbl * i_bbl;
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    ProcStateFree(BBL_PROCSTATE_IN(i_bbl));
    BBL_SET_PROCSTATE_IN(i_bbl,  NULL);
  }
}

void
FunctionPropagateConstantsAfterIterativeSolution(t_function * fun, t_analysis_complexity complexity)
{
  /* iterators */
  t_cfg_edge * i_edge;
  t_bbl * i_bbl;

  /* local variables */
  t_architecture_description * desc  = CFG_DESCRIPTION(FUNCTION_CFG(fun));
  t_regset registers_prop_over_hell = desc->registers_prop_over_hell;
  t_regset all_registers = desc->all_registers;

  /* Allocate init propagation inside fun */

  FUNCTION_FOREACH_BBL(fun, i_bbl) {
    if (BBL_PROCSTATE_IN(i_bbl))
      ProcStateFree(BBL_PROCSTATE_IN(i_bbl));
    BBL_SET_PROCSTATE_IN(i_bbl, ProcStateNew(desc));
  }

  /*  propagate the necessary information into the procedure */


  /* if there are more other incoming edges, propagate the information only if we are not in the fixpoint calculations */

  i_bbl = FUNCTION_BBL_FIRST(fun);

  if (complexity == CONTEXT_SENSITIVE)
  {
    /* If we did context sensitive constant propagation, we need to join the
     * procstates on all the incoming edges to get the procstate that can be
     * propagated into the first basic block */
    BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
    {
      if (CfgEdgeIsForwardInterproc(i_edge))
	if (CfgEdgeIsMarked(i_edge) && CFG_EDGE_PROCSTATE(i_edge))
	{
	  if (ProcStateJoinSimple(BBL_PROCSTATE_IN(i_bbl),CFG_EDGE_PROCSTATE(i_edge),CFG_EDGE_PROP_REGS(i_edge), desc))
	    FunctionMarkBbl(BBL_FUNCTION(i_bbl),i_bbl);
	}
    }
  }
  else
  {
    i_edge = BBL_PRED_FIRST(i_bbl);
    if (CfgEdgeIsForwardInterproc(i_edge))
      if (CfgEdgeIsMarked(i_edge) && CFG_EDGE_PROCSTATE(i_edge))
      {
	if (ProcStateJoinSimple(BBL_PROCSTATE_IN(i_bbl),CFG_EDGE_PROCSTATE(i_edge),CFG_EDGE_PROP_REGS(i_edge), desc))
	  FunctionMarkBbl(BBL_FUNCTION(i_bbl),i_bbl);
      }
  }

  /* join argstates */
  if (FUNCTION_NARGS(fun) > 0)
  {
    BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
      if (CfgEdgeIsForwardInterproc(i_edge) &&
	  CfgEdgeIsMarked(i_edge) &&
	  CFG_EDGE_ARGS(i_edge) == NULL)
	break;
    if (i_edge)
      current_argstate = NULL;
    else
    {
      current_argstate = ArgStateNew(FUNCTION_NARGS(fun));
      BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
	if (CfgEdgeIsForwardInterproc(i_edge) &&
	    CfgEdgeIsMarked(i_edge))
	  ArgStateJoin(current_argstate, CFG_EDGE_ARGS(i_edge));
    }
  }
  else
    current_argstate = NULL;

  FunctionMarkBbl(fun,FUNCTION_BBL_FIRST(fun));

  if (BBL_IS_HELL(FUNCTION_BBL_FIRST(fun)))
  {
    ProcStateSetAllBot(BBL_PROCSTATE_IN(FUNCTION_BBL_FIRST(fun)),RegsetNewInvers(registers_prop_over_hell,all_registers));
    ProcStateSetAllBot(BBL_PROCSTATE_IN(FUNCTION_BBL_LAST(fun)),RegsetNewInvers(registers_prop_over_hell,all_registers));
    FunctionMarkBbl(fun,FUNCTION_BBL_LAST(fun));
  }

  /* do the actual fixpoint calculation inside the fun */
  FunctionPropagateConstantsIteratively(fun, NULL, FALSE, complexity);

  /* TODO: if argstates are needed for the instruction constant optimizer,
   * they shouldn't be released here */
  if (current_argstate)
  {
    ArgStateFree(current_argstate);
    current_argstate = NULL;
  }
}

void
FunctionPropagateConstantsDuringIterativeSolution(t_function * fun,
    t_cfg_edge * edge_in,
    t_analysis_complexity complexity
    )
{
  /* iterators */
  t_bbl * i_bbl;
  t_architecture_description * desc =  CFG_DESCRIPTION(FUNCTION_CFG(fun));

  ASSERT(edge_in, ("FunctionPropagateConstantsDuringIterativeSolution called without incoming edge!"));
  ASSERT(BBL_FUNCTION(CFG_EDGE_TAIL(edge_in)) == fun, ("FunctionPropagateConstantsDuringIterativeSolution called with edge that is no predecessor of fun!"));

  /* Allocate init propagation inside fun */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
    BBL_SET_PROCSTATE_IN(i_bbl, ProcStateNew(desc));

  if (BBL_IS_HELL(CFG_EDGE_TAIL(edge_in)))
  {
    t_regset registers_prop_over_hell = desc->registers_prop_over_hell;
    t_regset all_registers = desc->all_registers;
    VERBOSE(0, ("%s, @E", FUNCTION_NAME(fun), edge_in));
    FATAL(("We propagated to hell.... This should never happen!"));
    ProcStateSetAllBot(BBL_PROCSTATE_IN(CFG_EDGE_TAIL(edge_in)),
	RegsetNewInvers(registers_prop_over_hell,all_registers));
  }

  /* Propagate the necessary information into the procedure. Propagation will
   * start at the tail of the edge, where either the incoming information is
   * used or everything is set to bot (in case of hell nodes). This is not
   * correct once we will track addresses or relocs propagated to hell. */

  ASSERT(CFG_EDGE_PROCSTATE(edge_in), ("no procstate for edge! @E", edge_in));
  ProcStateJoinSimple(BBL_PROCSTATE_IN(CFG_EDGE_TAIL(edge_in)),CFG_EDGE_PROCSTATE(edge_in),CFG_EDGE_PROP_REGS(edge_in), desc);

  FunctionMarkBbl(fun, CFG_EDGE_TAIL(edge_in));

  /* set the correct argstate for this propagation
   * TODO ugly hack!!! */
  if (complexity == CONTEXT_SENSITIVE)
  {
    current_argstate = CFG_EDGE_ARGS(edge_in);
    if (current_argstate)
      ArgStateSetUndefinedBot(current_argstate);
    if (BBL_FUNCTION(CFG_EDGE_HEAD(edge_in)) == fun && current_argstate)
    {
      /* recursive function: duplicate the argstate to avoid corruption */
      t_argstate *new = ArgStateNew(current_argstate->nargs);
      ArgStateDup(new, current_argstate);
      current_argstate = new;
    }
  }
  else
  {
    /* join over all incoming edges */
    t_cfg_edge *edge;
    if (!CFG_EDGE_ARGS(edge_in))
      current_argstate = NULL;
    else
    {
      current_argstate = ArgStateNew(CFG_EDGE_ARGS(edge_in)->nargs);
      BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun), edge)
      {
	if (!CfgEdgeIsForwardInterproc(edge) ||
	    !CfgEdgeIsMarked(edge))
	  continue;

	if (!CFG_EDGE_ARGS(edge))
	  break;
	ArgStateSetUndefinedBot(CFG_EDGE_ARGS(edge));
	ArgStateJoin(current_argstate, CFG_EDGE_ARGS(edge));
      }
      if (edge)
      {
	ArgStateFree(current_argstate);
	current_argstate = NULL;
      }
    }
  }

  /* Do the actual fixpoint calculation inside the fun. The out edge is the
   * corresponding edge of edge_in (if edge_in has a corresponding edge). */

  FunctionPropagateConstantsIteratively(fun, CFG_EDGE_CORR(edge_in), TRUE, complexity);

  /* Once a fixpoint is reached, we can free the memory allocated in the
   * functions to store bbl procstates. All that matters are the procstates
   * that are left on the outgoing interprocedural edges (In this case only
   * CFG_EDGE_CORR(edge_in)). */

  FunctionPropagateConstantsFreeProcstates(fun);

  if (BBL_FUNCTION(CFG_EDGE_HEAD(edge_in)) == fun || complexity == CONTEXT_INSENSITIVE)
    if (current_argstate)
    {
      ArgStateFree(current_argstate);
      current_argstate = NULL;
    }
}

static void
FunctionPropagateBotThrough(t_function * fun, t_analysis_complexity complexity)
{
  /* iterators */
  t_bbl * i_bbl;

  /* local variables */
  t_architecture_description * desc = CFG_DESCRIPTION(FUNCTION_CFG(fun));
  t_regset registers_prop_over_hell = CFG_DESCRIPTION(FUNCTION_CFG(fun))->registers_prop_over_hell;
  t_regset all_registers = CFG_DESCRIPTION(FUNCTION_CFG(fun))->all_registers;

  /* Allocate init propagation inside fun */

  FUNCTION_FOREACH_BBL(fun,i_bbl)
    BBL_SET_PROCSTATE_IN(i_bbl, ProcStateNew(desc));


  /* We are working on the entry fun or on hell initially. */
  FunctionMarkBbl(fun,FUNCTION_BBL_FIRST(fun));

  if (BBL_IS_HELL(FUNCTION_BBL_FIRST(fun)))
  {
    ProcStateSetAllBot(BBL_PROCSTATE_IN(FUNCTION_BBL_FIRST(fun)), RegsetNewInvers(registers_prop_over_hell,all_registers));
    ProcStateSetAllBot(BBL_PROCSTATE_IN(FUNCTION_BBL_LAST(fun)), RegsetNewInvers(registers_prop_over_hell,all_registers));
    FunctionMarkBbl(fun,FUNCTION_BBL_LAST(fun));
    FunctionMarkBbl(fun,FUNCTION_BBL_FIRST(fun));
  }
  else
    ProcStateSetAllBot(BBL_PROCSTATE_IN(FUNCTION_BBL_FIRST(fun)), all_registers);

  /* do the actual fixpoint calculation inside the fun */
  FunctionPropagateConstantsIteratively(fun, NULL, TRUE, complexity);

  /* free the allocated memory */
  FunctionPropagateConstantsFreeProcstates(fun);
}
/*}}}*/

void UpdateConditionalProcstate(t_cfg_edge *edge, t_procstate *ps) {
  /* free the old procstate if there is one */
  if (CFG_EDGE_CONDITIONAL_PROCSTATE(edge)) {
    ProcStateFree(CFG_EDGE_CONDITIONAL_PROCSTATE(edge));
    CFG_EDGE_SET_CONDITIONAL_PROCSTATE(edge, NULL);
  }

  /* set the new procstate, but only if the edge is to be taken conditionally */
  if (ps)
    CFG_EDGE_SET_CONDITIONAL_PROCSTATE(edge, ProcStateNewDup(ps));
}

/* PropOverEdge {{{ */

/*! propagate a state over an edge
 *
 * PRECONDITION: the edge_propagators must have
 * been initialized
 *
 * POSTCONDITION: the state returned is not the original one:
 * it is adapted with information about the state in case the edge was not
 * followed: the returning state is the state if an edge is not taken
 *
 * \return whether or not there is a need to propagate further (can it be that
 * the edge is not taken?)
 *
 * \todo does not work for link edges if there need to
 * be propagated something over them */

static t_bool
PropOverEdge(t_cfg_edge * edge, t_procstate ** state, t_bool do_it, t_bool during_fixpoint_calculations, t_regset all_registers)
{
  t_procstate * state_true;
  t_procstate * state_false;

  t_architecture_description * desc = CFG_DESCRIPTION(CFG_EDGE_CFG(edge));

  /* Call testcondition. This call does two things:
   *
   * 1. It makes constant propagation take conditional jumps into account: if a
   * branch cannot be taken (according to the current input procstate) it will
   * return null as state_true and (a potentially modified copy of - see 2) the
   * state as state false. If it will certainly be taken it will return null as
   * state false. Otherwise it will return both true and false.
   *
   * 2. It uses information about conditional jumps to partition the current
   * procstate into two different procstates: one procstate (state_true) that
   * can be propagated over the jump, and one procstate (state_false) that can
   * be propagated over the other outgoing edges. An examples of this
   * refinement: jump if equal => over the jump edge the equal condition will
   * be set, over the fall through it will be unset, even if before evaluating
   * this instruction the equal condition register was bot.
   *
   * It is conservative to simply duplicate state and return it as state_true
   * AND state_false, however, then you will not take conditional jumps into
   * account and performance/analysis correctness will deteriorate.  */

  conditional_state = NULL;
  CFG_EDGE_TESTCONDITION(edge)(edge, *state, &state_true, &state_false, &conditional_state);

  /* If the call to testcondition resulted in a state_true not being null, the
   * state pointed to by state_true can be propagated over the edge */
  if (state_true)
  {
    /* We propagate only over the edge if do_it is true. In practice do_it will
     * be true for all edges that are not backward interprocedural edges, and
     * for the backward interprocedural edge that corresponds to context we are
     * currently propagating. */
    if (do_it)
    {
      /* Mark the edge as taken */
      CfgEdgeMark(edge);

      /* Interprocedural edges need special handling */
      if (CfgEdgeIsInterproc(edge))
      {
	/* In case we are still relying on calling conventions, we need to make
	 * sure that we do not propagate dead register values into other
	 * procedures. The reason for this is that by propagating them we make
	 * it possible to use them in constant optimizations and by doing so,
	 * we could make registers live that were dead before. If we make these
	 * registers live, liveness analysis could produce faulty results in
	 * the next run (as long as we rely on calling conventions, certain
	 * registers will never be considered live over calls/returns). */

	if (diabloanopt_options.rely_on_calling_conventions)
	{
	  ProcStateSetAllBot(state_true,RegsetNewInvers(BBL_REGS_LIVE_OUT(CFG_EDGE_HEAD(edge)),all_registers));
	  ProcStateSetAllBot(state_true,RegsetNewInvers(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),all_registers));
	}

	/* During fixpoint calculations we need to set the edge procstate for
	 * interprocedural edges and repropagate for all changed
	 * interprocedural edges. After the fixpoint (during optimizations) we
	 * do not need to changes these procstates (we use the fixpoint
	 * obtained during fixpoint calculations */
	if (during_fixpoint_calculations)
	{
	  if ((ProcStateJoinSimple(CFG_EDGE_PROCSTATE(edge),state_true,CFG_EDGE_PROP_REGS(edge), desc)) || CFG_EDGE_ARGS_CHANGED(edge))
	  {
	    t_bbl* tail = CFG_EDGE_TAIL(edge);

	    /* If the edge we are considering is a backward interprocedural
	     * edge (return, swi_return, compensating), we need to repropagate
	     * all the incoming edges of the function of the return block (the
	     * tail of the edge) */
	    if (!CfgEdgeIsForwardInterproc(edge))
	    {
	      /* repropagation for all true incoming edges is necessary */
	      CfgMarkFun(CFG_EDGE_CFG(edge),BBL_FUNCTION(tail));
	      FunctionMarkAllTrueIncomingEdges(BBL_FUNCTION(tail));
	    }
	    else
	    {
	      /* only this edge will need to be repropagated */
	      CfgMarkFun(CFG_EDGE_CFG(edge),BBL_FUNCTION(tail));
	      FunctionMarkEdge(BBL_FUNCTION(tail),edge);
              /* The following optimisation crashes diablo when processing
               * gcc for i486 compiled with clang 3.[2-4] at -O0. Probably
               * needs to be extended for ArgState propagation
               */
#if 0
              if (CFG_EDGE_CAT(edge) == ET_CALL && (FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge))) & FF_IS_SIMPLE_LEAF))
                {
                  t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);

                  if (corr_edge)
                    {
                      /* step 1: propagate the state through the bbl */
                      t_ConstantPropagationInsEmul propagator = CFG_INSTRUCTION_EMULATOR(CFG_EDGE_CFG(edge));
                      t_ins * i_ins;
                      ProcStateDup(temp_procstate3, state_true, desc);
                      BBL_FOREACH_INS(CFG_EDGE_TAIL(edge),i_ins)
                        {
                          if (INS_DEFARG(i_ins) == -1 && INS_USEARG(i_ins) == -1 && (INS_ATTRIB(i_ins) & IF_FAST_CP_EVAL) && !RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(i_ins),temp_procstate->bot)))
                            {
                              ProcStateSetAllBot(temp_procstate3, INS_REGS_DEF(i_ins));
                              INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) | IF_EXECED);
                              INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) & (~IF_ALWAYS_EXECED));
                            }
                          else
                            propagator(i_ins, temp_procstate3, FALSE);
                        }
                      /* step 2: assign the resulting state to the return edge */
                      ProcStateJoinSimple(CFG_EDGE_PROCSTATE(corr_edge),temp_procstate3,CFG_EDGE_PROP_REGS(corr_edge), desc);
                      CfgEdgeMark(corr_edge);
                    }
                }
#endif
	    }
	  }
	}

	/* If it is a true outgoing edge, the corresponding incoming edge must
	 * have its information propagated into the procedure (if it is marked
	 * that is).  */

	if (CfgEdgeIsForwardInterproc(edge))
	{
	  t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);


	  /* If our forward interprocedural (call, swi, ipjump) edge has a
	   * corresponding edge (return, swi-return or compensating) and we
	   * already propagate something over this corresponding edge, we need
	   * to have this information propagated into our function. */

	  if ((corr_edge)&&(CfgEdgeIsMarked(corr_edge) &&
              /* can be null in case a call instruction is followed by data,
               * e.g. becuase the compiler knows it won't return (__abort)
               * and the function needs the return address for backtracing
               */
              BBL_PROCSTATE_IN(CFG_EDGE_TAIL(corr_edge))))
	  {
	    t_bbl * return_bbl = CFG_EDGE_TAIL(corr_edge);

	    if (ProcStateJoinSimple(BBL_PROCSTATE_IN(return_bbl), CFG_EDGE_PROCSTATE(corr_edge), CFG_EDGE_PROP_REGS(corr_edge), desc))
	      FunctionMarkBbl(BBL_FUNCTION(return_bbl), return_bbl);

	    /* If the edge has a corresponding edge, it returns from a
	     * function/system call. In this case we also propagate part of the
	     * information directly over the "link edge" because we know this
	     * information will not be changed by the called function/system
	     * call.  */

	    if (CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge) == ET_SWI)
	    {
	      if (ProcStateJoinSimple(BBL_PROCSTATE_IN(return_bbl),state_true,RegsetNewInvers(CFG_EDGE_PROP_REGS(corr_edge),all_registers), desc))
		FunctionMarkBbl(BBL_FUNCTION(return_bbl),return_bbl);
	    }
	  }
	}
      }
      /* Normal, non-interprocedural edge: the procstate is joined with the
       * procstate already stored for the bbl that is the tail of this edge. */
      else
      {
	t_bbl* tail = CFG_EDGE_TAIL(edge);

  UpdateConditionalProcstate(edge, conditional_state);
	if (ProcStateJoinSimple(BBL_PROCSTATE_IN(tail),state_true,CFG_EDGE_PROP_REGS(edge), desc))
	{
	  FunctionMarkBbl(BBL_FUNCTION(tail),tail);
	}
      }
    }

    if (state_false)
    {
      /* if there is a state to be propagated over the other outgoing edges,
       * the state over the true edge was freshly allocated in the call to
       * testcondition() and must be free'ed here. The only exception is when
       * both are the same */
      if (state_false!=state_true)
      {
	ProcStateFree(state_true);
      }
      state_true = NULL;
    }
  }

  if (conditional_state)
    ProcStateFree(conditional_state);

  if (state_false)
  {
    /* The state to be sent over the next edge going out of a block is the
     * false_state */
    *state = state_false;
    return TRUE;
  }
  else
  {
    /* if state_false is null after the call to testcondition(), it was
     * determined that no more states need to propagated over outgoing edges
     * of the basic block */

    return FALSE;
  }
}

static t_bool
PropOverEdgeCI(t_cfg_edge * edge, t_procstate ** state, t_bool during_fixpoint_calculations, t_regset all_registers)
{
  t_procstate * state_true;
  t_procstate * state_false;
  t_architecture_description * desc = CFG_DESCRIPTION(CFG_EDGE_CFG(edge));

  conditional_state = NULL;
  CFG_EDGE_TESTCONDITION(edge)(edge,*state,&state_true,&state_false,&conditional_state);

  if (state_true)
  {
    /*
       if the call to testcondition resulted in a state_true not
       being null, the state pointed to by state_true can be
       propagated over the edge */

    /* mark it as taken */
    CfgEdgeMark(edge);


    if (CfgEdgeIsInterproc(edge))
    {
      /*
	 if it is a true outgoing edge, the corresponding incoming edge must have its information propagated into the procedure (if it is marked that is).
	 */

      if (diabloanopt_options.rely_on_calling_conventions)
      {
	ProcStateSetAllBot(state_true,RegsetNewInvers(BBL_REGS_LIVE_OUT(CFG_EDGE_HEAD(edge)),all_registers));
	ProcStateSetAllBot(state_true,RegsetNewInvers(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),all_registers));
      }

      if (CfgEdgeIsForwardInterproc(edge))
      {
	/* First, propagate what is known over corresponding edge */
	t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);

	if (corr_edge)
	{
	  if (CfgEdgeIsMarked(BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge))))
	  {
	    t_bbl * return_bbl = CFG_EDGE_TAIL(corr_edge);
	    if (ProcStateJoinSimple(BBL_PROCSTATE_IN(return_bbl), CFG_EDGE_PROCSTATE(BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge))),CFG_EDGE_PROP_REGS(corr_edge),desc))
	      FunctionMarkBbl(BBL_FUNCTION(return_bbl),return_bbl);

	    /* secondly, propagate over link_edge */

	    if (CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge) == ET_SWI)
	    {
	      if (ProcStateJoinSimple(BBL_PROCSTATE_IN(return_bbl),state_true,RegsetNewInvers(CFG_EDGE_PROP_REGS(corr_edge), all_registers), desc))
		FunctionMarkBbl(BBL_FUNCTION(return_bbl),return_bbl);
	    }
	  }
	}
      }

      if (during_fixpoint_calculations)
      {
	if (CfgEdgeIsForwardInterproc(edge))
	{
	  if (ProcStateJoinSimple(CFG_EDGE_PROCSTATE(BBL_PRED_FIRST(CFG_EDGE_TAIL(edge))),state_true,CFG_EDGE_PROP_REGS(edge),desc) || CFG_EDGE_ARGS_CHANGED(edge))
	  {
	    t_bbl* tail = CFG_EDGE_TAIL(edge);
	    /* only this edge will need to be repropagated */
	    CfgMarkFun(CFG_EDGE_CFG(edge),BBL_FUNCTION(tail));
	    FunctionMarkEdge(BBL_FUNCTION(tail), BBL_PRED_FIRST(FUNCTION_BBL_FIRST(BBL_FUNCTION(tail))));
	  }
	}
	else
	{
	  t_cfg_edge * succ;
	  if (ProcStateJoinSimple(CFG_EDGE_PROCSTATE(BBL_SUCC_FIRST(CFG_EDGE_HEAD(edge))),state_true,CFG_EDGE_PROP_REGS(edge),desc))
	  {
	    /* only this edge will need to be repropagated */
	    BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(edge),succ)
	    {
	      if (CfgEdgeIsMarked(CFG_EDGE_CORR(succ)))
	      {
		t_bbl* tail = CFG_EDGE_TAIL(succ);
		t_function *caller = BBL_FUNCTION(tail);
		t_bbl *caller_entry = FUNCTION_BBL_FIRST(caller);

#ifdef NO_CONSTPROP_OVER_HELL
		if (!BBL_IS_HELL(caller_entry))
#endif
		{
		  CfgMarkFun(CFG_EDGE_CFG(edge),caller);
		  FunctionMarkEdge(caller, BBL_PRED_FIRST(caller_entry));
		}
	      }
	    }
	  }
	  else
	  {

	    BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(edge),succ)
	    {
	      if (!CfgEdgeIsMarked(succ) && CfgEdgeIsMarked(CFG_EDGE_CORR(succ)))
	      {
		t_bbl* tail = CFG_EDGE_TAIL(succ);
		t_function *caller = BBL_FUNCTION(tail);
		t_bbl *caller_entry = FUNCTION_BBL_FIRST(caller);

#ifdef NO_CONSTPROP_OVER_HELL
		if (!BBL_IS_HELL(caller_entry))
#endif
		{
		  CfgMarkFun(CFG_EDGE_CFG(edge), caller);
		  FunctionMarkEdge(caller, BBL_PRED_FIRST(caller_entry));
		}
	      }
	    }
	  }
	}
      }
    }
    else
    {
      t_bbl* tail = CFG_EDGE_TAIL(edge);

      UpdateConditionalProcstate(edge, conditional_state);

      if (ProcStateJoinSimple(BBL_PROCSTATE_IN(tail),state_true,CFG_EDGE_PROP_REGS(edge),desc))
      {
	FunctionMarkBbl(BBL_FUNCTION(tail),tail);
      }
    }
    if (state_false)
    {
      /* if there is a state to be propagated over the other
	 outgoing edges, the state over the true edge was freshly
	 allocated in the call to testcondition() and must be
	 free'ed here. The only exception is when both are the same */
      if (state_false!=state_true)
      {
	ProcStateFree(state_true);
      }
      state_true = NULL;
    }
  }

  if (conditional_state)
    ProcStateFree(conditional_state);

  if (state_false)
  {
    /*
       The state to be sent over the next edge going out of a block
       is the false_state */
    *state=state_false;
    return TRUE;
  }
  else
  {
    /*
       if state_false is null after the call to testcondition(), it
       was determined that no more states need to propagated over
       outgoing edges of the basic block */

    return FALSE;
  }
}
/*}}}*/

void InsPropagateConstants(t_procstate *procstate, t_ins *ins)
{
  t_cfg *cfg = INS_CFG(ins);
  t_ConstantPropagationInsEmul propagator = CFG_INSTRUCTION_EMULATOR(cfg);

  if ((INS_ATTRIB(ins) & IF_FAST_CP_EVAL) && !RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(ins),procstate->bot)) &&
        INS_DEFARG(ins) == -1 && INS_USEARG(ins) == -1)
  {
    ProcStateSetAllBot(procstate, INS_REGS_DEF(ins));
  }
  else
    propagator(ins, procstate, FALSE);
}

/* Propagate BBL
 *
 * Propagate a state over a bbl and to the successor edges {{{ */
void
BblPropagateConstants(t_bbl * bbl, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations, t_analysis_complexity complexity)
{
  /* iterators */
  t_ins * i_ins;
  t_ins * last_ins;
  t_cfg_edge * i_edge;
  t_bool special_last_ins;
  /* local variables */
  t_cfg * cfg = BBL_CFG(bbl);
  t_bool prop_further = TRUE;
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);
  t_regset allregs = desc->all_registers;
  t_ConstantPropagationInsEmul propagator = CFG_INSTRUCTION_EMULATOR(cfg);

  /* If any of the incoming registers is still top, we should not yet
   * propagate: other incoming edges of this block still need to be evaluated
   * before we can propagate this block. */
  if (!RegsetIsMutualExclusive(((t_procstate*)BBL_PROCSTATE_IN(bbl))->top,allregs))
    return;

  /* */
  CFG_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg)(bbl,&last_ins);
  if (last_ins)
  {
    special_last_ins=TRUE;
  }
  else
  {
    special_last_ins = FALSE;
    last_ins=BBL_INS_LAST(bbl);
  }

  ProcStateDup(temp_procstate, BBL_PROCSTATE_IN(bbl), desc);
  BBL_FOREACH_INS(bbl,i_ins)
  {
    if ((last_ins == i_ins) && (special_last_ins))
    {
      ProcStateDup(temp_procstate2, temp_procstate, desc);
    }

    if ((INS_ATTRIB(i_ins) & IF_FAST_CP_EVAL) && !RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(i_ins),temp_procstate->bot)) &&
	INS_DEFARG(i_ins) == -1 && INS_USEARG(i_ins) == -1)
    {
      ProcStateSetAllBot(temp_procstate, INS_REGS_DEF(i_ins));
      INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) | IF_EXECED);
      INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) & (~IF_ALWAYS_EXECED));
    }
#ifdef DEBUG_CONST_PROP
    else if (G_T_UINT32(INS_OLD_ADDRESS(i_ins))>diablosupport_options.debugcounter)
    {
      ProcStateSetAllBot(temp_procstate, INS_REGS_DEF(i_ins));
      INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) | IF_EXECED);
      INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) & (~IF_ALWAYS_EXECED));
    }
#endif
    else
      propagator(i_ins, temp_procstate, FALSE);
  }

  if (complexity == CONTEXT_SENSITIVE)
  {
    /* special case for architectures like the arm: when an LDM
     * {r1,r2,r3,r4,...,pc} instruction is conditional, the old values of the
     * registers should be propagated over the fallthrough edge */
    BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
    {
      if (prop_further)
      {
	if ((CFG_EDGE_CAT(i_edge)==ET_FALLTHROUGH)&&(special_last_ins))
	{
	  /* We duplicate the condition registers here because
	   * they only are adapted in temp_procstate by previous
	   * calls of PropOverEdge. */
	  RegsetSetDup(temp_procstate2->cond_true,temp_procstate->cond_true);
	  RegsetSetDup(temp_procstate2->cond_false,temp_procstate->cond_false);
	  PropOverEdge(i_edge, &temp_procstate2, TRUE, during_fixpoint_calculations,allregs);
	  prop_further = FALSE;
	}
	else
	{
	  prop_further = PropOverEdge(i_edge, &temp_procstate, (!CfgEdgeIsBackwardInterproc(i_edge) || i_edge == edge_out || edge_out == NULL), during_fixpoint_calculations,allregs);
	}
      }
    }
  }
  else /* CONTEXT INSENSITIVE {{{ */
  {
    if (!special_last_ins)
    {
      BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
      {
	if (prop_further && temp_procstate)
	{
	  {
	    prop_further = PropOverEdgeCI(i_edge, &temp_procstate, during_fixpoint_calculations,allregs);
	  }
	}
      }
    }
    else
    {

      /* special case for architectures like the arm: when an LDM
       * {r1,r2,r3,r4,...,pc} instruction is conditional, the old values of
       * the registers should be propagated over the fallthrough edge */

      BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
      {
	if (prop_further && temp_procstate)
	{
	  if (CFG_EDGE_CAT(i_edge)==ET_FALLTHROUGH)
	  {
	    /* We duplicate the condition registers here because they only are adapted in temp_procstate by previous calls of
	     * PropOverEdge. */
	    RegsetSetDup(temp_procstate2->cond_true,temp_procstate->cond_true);
	    RegsetSetDup(temp_procstate2->cond_false,temp_procstate->cond_false);
	    PropOverEdgeCI(i_edge, &temp_procstate2, during_fixpoint_calculations,allregs);
	    prop_further = FALSE;
	  }
	  else
	  {
	    prop_further = PropOverEdgeCI(i_edge, &temp_procstate, during_fixpoint_calculations,allregs);
	  }
	}
      }
    }
  } /* }}} */

  /* turn off the args_changed flags on outgoing interproc edges */
  BBL_FOREACH_SUCC_EDGE(bbl, i_edge)
    if (CfgEdgeIsForwardInterproc(i_edge))
      CFG_EDGE_SET_ARGS_CHANGED(i_edge, FALSE);
}
/*}}}*/

/* Optimizations using CP information {{{*/
/* Remove unreachable edges {{{ */
static void
CfgRemoveUnreachableEdges(t_cfg * cfg, t_analysis_complexity complexity)
{
  t_cfg_edge * i_edge, * i_safe;
  t_cfg_edge * corr_edge;

  int dead = 0;

  CFG_FOREACH_EDGE_SAFE(cfg,i_edge,i_safe)
  {
    if (CFG_EDGE_HEAD(i_edge) == CFG_UNIQUE_ENTRY_NODE(cfg))
    {
      continue;
    }

    if (!(CfgEdgeIsMarked(i_edge)))
    {
      if (CFG_EDGE_CAT(i_edge)!=ET_RETURN &&
	  CFG_EDGE_CAT(i_edge)!=ET_COMPENSATING
	 )
      {
	corr_edge = CFG_EDGE_CORR(i_edge);

	if (corr_edge)
	{
	  if (complexity == CONTEXT_INSENSITIVE && corr_edge == BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge)))
	  {
	    if (CFG_EDGE_SUCC_NEXT(corr_edge))
	    {
	      CFG_EDGE_SET_PROCSTATE(CFG_EDGE_SUCC_NEXT(corr_edge),  CFG_EDGE_PROCSTATE(corr_edge));
	      CFG_EDGE_SET_PROCSTATE(corr_edge,  NULL);
	    }
	  }

	  /* kill the corresponding edge */
	  if (corr_edge == i_safe)
	    i_safe = CFG_EDGE_NEXT(i_safe);
    if (CFG_EDGE_ARGS(corr_edge))
      ArgStateFree(CFG_EDGE_ARGS(corr_edge));
	  CfgEdgeKill(corr_edge);
	  dead++;
	}


	/* finally kill the edge itself */
	if (complexity == CONTEXT_INSENSITIVE && i_edge == BBL_PRED_FIRST(CFG_EDGE_TAIL(i_edge)) && CfgEdgeIsForwardInterproc(i_edge))
	{
	  if (CFG_EDGE_PRED_NEXT(i_edge))
	  {
	    CFG_EDGE_SET_PROCSTATE(CFG_EDGE_PRED_NEXT(i_edge),  CFG_EDGE_PROCSTATE(i_edge));
	    CFG_EDGE_SET_PROCSTATE(i_edge,  NULL);
	  }
	}

	VERBOSE(10,("killing edge @E\n",i_edge));
  if (CFG_EDGE_ARGS(i_edge))
    ArgStateFree(CFG_EDGE_ARGS(i_edge));
	CfgEdgeKill(i_edge);

	dead++;
      }
    }
  }
  VERBOSE(0,("%d unreachable edges removed",dead));
}
/*}}} */
/* Unconditionalize {{{ */
static void CfgUnconditionalizeIns(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_function * i_fun;
  t_ins * i_ins;

  int nr_uncond_ins = 0;

  CFG_FOREACH_FUN(cfg,i_fun)
    FUNCTION_FOREACH_BBL(i_fun,i_bbl)
    BBL_FOREACH_INS(i_bbl,i_ins)
    if (INS_ATTRIB(i_ins) & IF_EXECED)
      if (INS_ATTRIB(i_ins) & IF_ALWAYS_EXECED)
      {
	if (CFG_DESCRIPTION(cfg)->InstructionUnconditionalizer(i_ins))
	{
	  VERBOSE(10,("Unconditionalizing @I\n",i_ins));
	  nr_uncond_ins++;
	}
      }

  VERBOSE(0,("%d instructions unconditionalized",nr_uncond_ins));

}
/*}}}*/
/* Remove unreachable blocks {{{ */
static void CfgRemoveUnreachableBlocks(t_cfg *cfg)
{
  /* iterators */
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;
  t_ins * i_ins, *i_tmp;

  /* local variables */
  int dead_ins = 0;
  int dead_blocks = 0;
  t_bool has_predecessor;
  int old_deadins = 0;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (IS_DATABBL(i_bbl)) continue;
    if (i_bbl == CFG_UNIQUE_ENTRY_NODE(cfg)) continue;

    BblUnmark(i_bbl);
    has_predecessor = FALSE;
    BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
    {
      has_predecessor = TRUE;
      if (CfgEdgeIsMarked(i_edge))
	BblMark(i_bbl);
    }

    if (has_predecessor)
    {
      dead_blocks++;
    }

    old_deadins = dead_ins;

    BBL_FOREACH_INS_SAFE(i_bbl,i_ins,i_tmp)
    {
      if (!(INS_ATTRIB(i_ins) & IF_EXECED))
      {
	if (has_predecessor)
	{
	  dead_ins++;
	}
	VERBOSE(10,("Unexeced: @I\n",i_ins));
	InsKill(i_ins);
      }
    }
  }
  VERBOSE(0,("%d unreachable instructions in %d unreachable blocks killed",dead_ins,dead_blocks));
}
/*}}}*/
/* RemoveNotExecutedIns {{{ */
static void CfgRemoveNotExecutedIns(t_cfg *cfg, t_reloc_table * table)
{

  /* iterators */

  t_bbl * i_bbl;
  t_cfg_edge * i_edge;
  t_ins * i_ins, * i_tmp;

  /* local variables */

  int dead_ins = 0;
  int old_deadins = 0;
  t_bool has_predecessor;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (IS_DATABBL(i_bbl)) continue;
    BblUnmark(i_bbl);
    has_predecessor = FALSE;
    BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
    {
      has_predecessor = TRUE;
      if (CfgEdgeIsMarked(i_edge))
	BblMark(i_bbl);
    }
    if (!BblIsMarked(i_bbl)) continue;

    old_deadins = dead_ins;
    BBL_FOREACH_INS_SAFE(i_bbl,i_ins,i_tmp)
    {
      if (!(INS_ATTRIB(i_ins) & IF_EXECED))
      {
	VERBOSE(10, ("Unexecuted ins @I", i_ins));
	InsKill(i_ins);
	if (has_predecessor)
	  dead_ins++;
      }
    }
  }
  VERBOSE(0,("%d unexecuted instructions killed",dead_ins));
}

void BblPropagateConstantInformation(t_bbl *i_bbl, t_analysis_complexity complexity, t_procstate *prev_state, t_procstate *next_state)
{
        t_cfg *cfg = BBL_CFG(i_bbl);
        t_architecture_description *desc = CFG_DESCRIPTION(cfg);
        t_regset allregs = desc->all_registers;

        if (!RegsetIsMutualExclusive(((t_procstate*)BBL_PROCSTATE_IN(i_bbl))->top,allregs))
                return;

        t_bool have_to_free = FALSE;
        if (!prev_state)
        {
                have_to_free = TRUE;
                prev_state = ProcStateNew(desc);
                next_state = ProcStateNew(desc);
        }

        ProcStateDup(next_state,BBL_PROCSTATE_IN(i_bbl),desc);
        ProcStateDup(prev_state,BBL_PROCSTATE_IN(i_bbl),desc);

        /* We need to use a safe iterator, because useless instruction can be
        * killed in Optimizer */

        t_ins *i_tmp;
        t_ins *i_ins;
        BBL_FOREACH_INS_SAFE(i_bbl,i_ins,i_tmp)
        {
                /* Instructions that are marked IF_FAST_CP_EVAL have the property
                 * that if one of their used registers is bot then the defined
                 * registers will also be bot.
                 *
                 * It is always safe to set the IF_FAST_CP_EVAL on instructions, but
                 * results might deteriorate, depending on the architecture (and the
                 * implementation of the cp transfer functions).
                 *
                 * As an example of an instruction where results deteriorate if
                 * IF_FAST_CP_EVAL is used, consider conditional execution on the ARM
                 * architecture: if the condition is known, but the rest of the used
                 * registers are bot, we might still be able to know the result if
                 * the result register is known before evaluation and if the
                 * condition is false (i.e. the instruction does not execute).  */
                if ((INS_ATTRIB(i_ins) & IF_FAST_CP_EVAL) && !RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(i_ins), prev_state->bot)))
                  ProcStateSetAllBot(next_state,INS_REGS_DEF(i_ins));
                else
                  CFG_INSTRUCTION_EMULATOR(cfg)(i_ins,next_state,TRUE);

                if (!disable_transformations)
                  CFG_INSTRUCTION_CONSTANT_OPTIMIZER(cfg)(i_ins,prev_state,next_state, complexity);

                ProcStateDup(prev_state,next_state,desc);
        }

        /* should be set after the loop in case the BBL does not contain any instructions */
        if (BBL_PROCSTATE_OUT(i_bbl) == NULL)
          BBL_SET_PROCSTATE_OUT(i_bbl, ProcStateNewDup(next_state));
        else
          ProcStateDup(BBL_PROCSTATE_OUT(i_bbl), next_state, desc);

        if (have_to_free)
        {
                ProcStateFree(prev_state);
                ProcStateFree(next_state);
        }
}

/*}}} */
/* CfgApplyConstantInformation. This function computes constant information for
 * each instruction in the cfg using the results of the iterative constant
 * propagation solution, and calls the architecture specific constant
 * instruction optimizer with this information.
 *
 * {{{ */
void
CfgApplyConstantInformation(t_cfg * cfg, t_analysis_complexity complexity)
{
  /* iterators */
  t_function * i_fun;
  t_bbl * i_bbl;

  /* for speedups */
  t_procstate * prev_state = ProcStateNew(CFG_DESCRIPTION(cfg));
  t_procstate * next_state = ProcStateNew(CFG_DESCRIPTION(cfg));

  /* first of all, if the instructions in the graph already contain regstates,
   * remove them */

  CFG_FOREACH_FUN(cfg,i_fun)
    FunctionUnmarkAllBbls(i_fun);

  CFG_FOREACH_FUN(cfg,i_fun)
  {
    /* We can ignore functions without blocks, hell functions and functions
     * without incoming edges (can happen if some other optimizations are
     * turned off). */

    if (FUNCTION_BBL_FIRST(i_fun) && !BBL_IS_HELL(FUNCTION_BBL_FIRST(i_fun)) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(i_fun)))
    {
      /* Constant propagation dumped its information on the incoming
       * interprocedural edges of the function. We now have to join the
       * information of the different edges and propagate it through the
       * function to get values that are usable for constant optimization. The
       * following function does just that. It stores the obtained results at
       * the start of each basic block in BBL_PROCSTATE_IN. Results before each
       * seperate instruction still need to be computed. */

      FunctionPropagateConstantsAfterIterativeSolution(i_fun, complexity);

      FUNCTION_FOREACH_BBL(i_fun,i_bbl)
      {
        if (IS_DATABBL(i_bbl))
          continue;

        BblPropagateConstantInformation(i_bbl, complexity, prev_state, next_state);
      }

      if (!is_advanced_factoring_phase)
      {
        FUNCTION_FOREACH_BBL(i_fun,i_bbl)
        {
        	ProcStateFree(BBL_PROCSTATE_IN(i_bbl));
        	BBL_SET_PROCSTATE_IN(i_bbl,  NULL);
        }
      }
    }
  }

  ProcStateFree(prev_state);
  ProcStateFree(next_state);
}
/*}}} */
/* UseConstantInformation {{{ */
void
OptUseConstantInformation(t_cfg * cfg, t_analysis_complexity complexity)
{
  t_object * obj=CFG_OBJECT(cfg);

  STATUS(START,("Constant propagation optimizations"));

  /* Remove unreachable nodes from the graph */
  CfgRemoveUnreachableBlocks(cfg);

  /* The same for edges. */
  CfgRemoveUnreachableEdges(cfg, complexity);

  /* Instructions that are executed and are always executed (conditions always
   * hold) are unconditionalized. */
  CfgUnconditionalizeIns(cfg);

  /* Instructions that are never executed because of non holding conditions
   * are turned into NOOPS. */

  CfgRemoveNotExecutedIns(cfg,OBJECT_RELOC_TABLE(obj));


  /* All incoming edges will be propagated into the procedures, to detect the
   * constants inside the procedure and optimize the code based on those
   * constants. */

  InitDelayedInsKilling();
  CfgApplyConstantInformation(cfg, complexity);
  ApplyDelayedInsKilling();
  STATUS(STOP,("Constant propagation optimizations"));
}
/*}}}*/
/*}}}*/

/* generic list of symbols known to point to _constant_ values in memory.
 * Constant propagation implementations can use this to introduce some extra
 * (a priori known) constants in the program */
static t_symbol ** known_constant_syms;
static int nknown_syms = 0;

void AddKnownConstantSym(t_symbol *sym)
{
  if (known_constant_syms == 0)
  {
    known_constant_syms = Malloc(sizeof(t_symbol *));
    known_constant_syms[0] = sym;
    nknown_syms = 1;
  }
  else
  {
    known_constant_syms = Realloc(known_constant_syms,sizeof(t_symbol)*(nknown_syms+1));
    known_constant_syms[nknown_syms++] = sym;
  }
}

void KillKnownConstantSymTable(void)
{
  if (known_constant_syms) Free(known_constant_syms);
  known_constant_syms = NULL;
  nknown_syms = 0;
}

/* {{{ IsKnownToBeConstant */
t_bool IsKnownToBeConstant(t_address address, t_reloc *rel)
{
  t_address offset;
  int i;

  if (!known_constant_syms) return FALSE;
  if (RELOC_N_TO_RELOCATABLES(rel)!=1) return FALSE;
  offset = AddressSub(address,RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[0]));

  if (AddressIsGe(offset,RELOCATABLE_CSIZE(RELOC_TO_RELOCATABLE(rel)[0])))
    return FALSE;

  for (i=0; i<nknown_syms; i++)
    if (SYMBOL_BASE(known_constant_syms[i]) == RELOC_TO_RELOCATABLE(rel)[0])
      if (AddressIsEq(SYMBOL_OFFSET_FROM_START(known_constant_syms[i]),offset))
      {
	VERBOSE(1,("Assuming constant for @S\n",known_constant_syms[i]));
	return TRUE;
      }
  return FALSE;
} /* }}} */
/* vim: set shiftwidth=2 foldmethod=marker: */
