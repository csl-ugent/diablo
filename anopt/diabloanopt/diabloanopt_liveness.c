/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define LIVENESS_OPTION_CONDITIONAL_CALL_OPT
#define AGGRESSIVE_LIVENESS 

#include <diabloanopt.h>

#define DIABLOANOPT_HAVE_DWARF_SUPPORT

#ifdef DIABLOANOPT_HAVE_DWARF_SUPPORT
#define DWARF_ARG_INFO
#define DWARF_RET_TYPE_INFO
#define OPTNEVERLIVE
// #include "../../dwarf/dwarf_function.h"
#endif

/*! FunctionComputeChangedRegisters: This function computes the registers that
 * a procedure changes, ignoring loads that restore a register. 
 * {{{ */
static void 
FunctionComputeChangedRegisters(t_function * fun) 
{
  t_bbl *  i_bbl; 
  t_ins * i_ins; 
  t_regset changed;
  t_architecture_description * desc = CFG_DESCRIPTION(FUNCTION_CFG(fun));

  if( FUNCTION_IS_HELL(fun) )
  {
    /* for now assume very worst-case behavior */
    FUNCTION_SET_REGS_CHANGED(fun, desc->all_registers);

    if (diabloanopt_options.rely_on_calling_conventions)
    {
      if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)))
      {
	FUNCTION_SET_REGS_CHANGED(fun, RegsetDiff(FUNCTION_REGS_CHANGED(fun),CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_saved));
      }
    }

    if (FUNCTION_BBL_FIRST(fun)==CFG_SWI_HELL_NODE(FUNCTION_CFG(fun)))
    {
      FUNCTION_SET_REGS_CHANGED(fun, RegsetDiff(FUNCTION_REGS_CHANGED(fun),CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_saved));
    }
    return;
  }

  changed = RegsetNew();

  FUNCTION_FOREACH_BBL(fun,i_bbl)
  { 
    BBL_FOREACH_INS(i_bbl,i_ins) 
    {
      RegsetSetUnion(changed,INS_REGS_DEF(i_ins)); 
    } 
  }


  if (diabloanopt_options.rely_on_calling_conventions)
    if (CFG_DESCRIPTION(FUNCTION_CFG(fun))->FunIsGlobal(fun))
      RegsetSetDiff(changed,CFG_DESCRIPTION(FUNCTION_CFG(fun))->callee_saved);
  
  FUNCTION_SET_REGS_CHANGED(fun, changed);
  RegsetFree(changed);
}

/*}}}*/

/*! FunctionComputeSavedRegisters: This function computes the registers that a
 * procedure saves, THESE ARE ONLY THOSE REGISTERS WHO's VALUE UPON ENTRY IS
 * NOT USED OTHERWISE BY THE PROCEDURE 
 * {{{ */
void 
FunctionComputeSavedRegisters(t_function * fun)
{
  t_regset saved= RegsetNew();
  FUNCTION_SET_REGS_SAVED(fun, saved);
  RegsetFree(saved);
  
  DiabloBrokerCall("CfgComputeStackSavedRegistersForFun",FUNCTION_CFG(fun),fun);
}
/*}}} */

/*! CfgComputeSavedChangedRegisters: Fix-point computation to find the
 * registers that are not changed, but saved by procedures (in themselves or in
 * their call chain). 
 * {{{ */
void 
CfgComputeSavedChangedRegisters(t_cfg * cfg)
{
  t_cfg_edge * i_edge;
  t_function * caller;
  t_function * callee;
  t_regset changed;
  t_regset current;
  t_bool  flag;

  t_function * i_fun;

  CfgSortEdges(cfg);

  CFG_FOREACH_FUN(cfg,i_fun)
    {
      FunctionComputeChangedRegisters(i_fun);
      FunctionComputeSavedRegisters(i_fun);
      FUNCTION_SET_REGS_CHANGED(i_fun, RegsetDiff(FUNCTION_REGS_CHANGED(i_fun),FUNCTION_REGS_SAVED(i_fun)));
    }
  
  do
    {
      flag = FALSE;
      CFG_FOREACH_EDGE(cfg,i_edge)
	{
	  if( !CfgEdgeIsForwardInterproc(i_edge) ) break;
	  caller = BBL_FUNCTION(CFG_EDGE_HEAD(i_edge));
	  callee = BBL_FUNCTION(CFG_EDGE_TAIL(i_edge));

	  if ((!caller)||(!callee)) continue;
	  /* TODO: this used to be turned off for instrumentation because
	   * CfgComputeSavedChangedRegisters is called from 
	   * within OptimizePseudoLoadStores and for that
	   * function, this optimization is not allowed. */

	  /* call hell function _always_ adheres to calling conventions */

	  if (diabloanopt_options.rely_on_calling_conventions &&
	      BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(caller)))
	    continue;


	  RegsetSetDup(current,FUNCTION_REGS_CHANGED(caller));
	  RegsetSetDup(changed,FUNCTION_REGS_CHANGED(callee));
	  
	  RegsetSetDiff(changed,FUNCTION_REGS_SAVED(caller));
	  
	  if (!RegsetEquals(RegsetUnion(changed,current),current))
	    {
	      FUNCTION_SET_REGS_CHANGED(caller, RegsetUnion(current,changed));
	      flag = TRUE;
	    }
	  
        }
    }   
  while( flag );
}
/*}}}*/

/* CfgComputeLivenessTrivial {{{ */
static void 
CfgComputeLivenessTrivial(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);
        
  CFG_FOREACH_BBL(cfg,i_bbl)
    {
      BBL_SET_REGS_LIVE_OUT(i_bbl, desc->all_registers);
    }
}
/*}}}*/

/* CfgComputeLivenessInsensitive: Perform a straight-forward
 * context-insensitive liveness analysis
 * {{{ */
static void 
CfgComputeLivenessInsensitive(t_cfg * cfg)
{
  /* iterators */

  t_cfg_edge * i_edge;
  t_bbl * i_bbl;
  t_function * i_fun;

  /* local variables */

  t_bool  flag,flag2;
  t_regset in = RegsetNew();
  t_regset tmp;
  t_regset new_out = RegsetNew();
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);
		  
  /* Initialize the def,use,in,out fields for each bbl */
       
  NodeMarkInit();

  CFG_FOREACH_BBL(cfg,i_bbl)
    {
      i_fun = BBL_FUNCTION(i_bbl);
 
      /* initialize it to a ... */
      tmp = BblRegsUse(i_bbl);
      BBL_SET_REGS_USE(i_bbl, tmp);
      RegsetFree(tmp);
      tmp = BblRegsDef(i_bbl);
      BBL_SET_REGS_DEF(i_bbl, tmp); 
      RegsetFree(tmp);
      tmp = BblRegsNeverLive(i_bbl);
      BBL_SET_REGS_NEVER_LIVE(i_bbl, tmp);
      RegsetFree(tmp);
      
      if( !i_fun || FUNCTION_IS_HELL(i_fun) )
        {
	  /* this information is not really used since AbblRegLiveIn called
	   * below has special handling for the PSEUDO functions.  It will keep
	   * these bbls from beeing unecessarily updated, though */
	  
	  BBL_SET_REGS_LIVE_OUT(i_bbl, desc->all_registers);
        }
      else
        {
	  BBL_SET_REGS_LIVE_OUT(i_bbl, desc->always_live);
        }
     
      /* mark all basic blocks */

      BblMark(i_bbl);
    }

  /* initialize: Mark all functions */
  



  CFG_FOREACH_FUN(cfg,i_fun) 
    FunctionMark(i_fun);

  /* Do cfa0 fixpoint computation */
  do
    {
      flag = 0;
      CFG_FOREACH_FUN(cfg,i_fun)
	{
	 
	  /* Do not propagate non changed information */
	  if( !(FunctionIsMarked(i_fun)) ) continue;
	  
	  /* Unmark the function */
	  FunctionUnmark(i_fun);
	  
	  do
	    {
	      flag2 = 0;  

	      FUNCTION_FOREACH_BBL(i_fun,i_bbl)
		{
		  /* Do not propagate non changed information */
		  if( !BblIsMarked(i_bbl) ) continue;
		  
		  /* Unmark the basic block */
		  BblUnmark(i_bbl);

		  flag = 1; /* TODO: Is this necessary ? */
		  
		  RegsetSetDup(in,BBL_REGS_LIVE_OUT(i_bbl));
		  RegsetSetDiff(in,BBL_REGS_DEF(i_bbl));
		  RegsetSetUnion(in,BBL_REGS_USE(i_bbl));
		  
		  BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
		    {
		      t_bbl * hd = (t_bbl *) CFG_EDGE_HEAD(i_edge);
		      /* the following test was added by Bjorn om 05/12/2003 */
		      RegsetSetDup(new_out,in);
		      RegsetSetUnion(new_out,BBL_REGS_LIVE_OUT(hd));
		      if(!RegsetEquals(new_out,BBL_REGS_LIVE_OUT(hd)))
			{
			  BBL_SET_REGS_LIVE_OUT(hd, new_out);
			  BblMark(hd);
			  if (CfgEdgeIsInterproc(i_edge))
			    {
			      if (BBL_FUNCTION(hd))
				{
				  FunctionMark(BBL_FUNCTION(hd));
				  flag = 1;
				}
			    }
			  else
			    flag2=1;	  /* BBL_FLAGS(hd) |= FF_IS_MARKED; */
			}
		    }
		}
	    }
	  while (flag2);
        }
    }
  while( flag );

  RegsetFree(new_out);
  RegsetFree(in);
}
/* }}} */

static void SensitiveLivenessFixpoint(t_cfg * cfg, t_bool with_kill_useless);
static void CfgComputeFunctionThroughRegs(t_cfg * cfg);
static void CfgComputeFunctionUsedRegs(t_cfg * cfg);
static void CfgComputeMeaningfulRegs(t_cfg * cfg);

/*! Liveness Sensitive: Context-sensitive liveness analyses, in three phases,
 * as described in Robert Muth's thesis. 
 * {{{ */
static void 
CfgComputeLivenessSensitive(t_cfg * cfg, t_bool with_kill_useless)
{
  /* iterators */
  t_function * i_fun;
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;
  
  /* local variables */
  const t_architecture_description * desc = CFG_DESCRIPTION(cfg);

  /* First we initialize all functions:
   *
   * - The call hell function gets initialized as:
   *   - regs through = callee saved
   *   - regs used    = callee may use
   *
   * - All other hell functions get initialized as:
   *   - regs through = all regs
   *   - regs used    = all regs
   *
   * - All other functions get initialized as:
   *   - regs through = no regs
   *   - regs used    = no regs
   * */

  CFG_FOREACH_FUN(cfg,i_fun)
  {
    if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)))
    {
      FUNCTION_SET_REGS_THROUGH(i_fun, desc->callee_saved);
      if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)) == BBL_CH_DYNCALL || FUNCTION_BBL_FIRST(i_fun) == CFG_CALL_HELL_NODE(cfg))
	FUNCTION_SET_REGS_USED(i_fun, RegsetUnion(desc->callee_may_use,desc->dyncall_may_use));
      else
	FUNCTION_SET_REGS_USED(i_fun, desc->callee_may_use);
    }
    else if( FUNCTION_IS_HELL(i_fun) )
    {
      FUNCTION_SET_REGS_THROUGH(i_fun, desc->all_registers);
      FUNCTION_SET_REGS_USED(i_fun, desc->all_registers);
    }
    else
    {
      FUNCTION_SET_REGS_THROUGH(i_fun, NullRegs);
      FUNCTION_SET_REGS_USED(i_fun, NullRegs);
    }
  }
  
  /* Next we compute the used and defined registers of each basic block.
   *
   * This information never changes during the liveness analysis! */

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    /* If the basic block is not part of a function, it is either a data basic
     * block or an unreachable block, that was not yet removed by unreachable
     * code removal. We can safely ignore those blocks here. */
    if (!BBL_FUNCTION(i_bbl))
      continue;

    BBL_SET_REGS_USE(i_bbl, BblRegsUse(i_bbl));
    
    BBL_SET_REGS_DEF(i_bbl, BblRegsDef(i_bbl));
    
    BBL_SET_REGS_NEVER_LIVE(i_bbl, BblRegsNeverLive(i_bbl));

    /* We set the exit blocks to 'all registers live' */
    if (!BBL_IS_HELL(i_bbl) && BblIsExitBlock(i_bbl))
    {
      BBL_SET_REGS_LIVE_OUT(i_bbl, desc->all_registers);

      /* Use debugging information extension {{{ */
#ifdef DWARF_RET_TYPE_INFO
      /* Subtract the return registers that are not used by
       * this function from the set of live registers.
       */
      if(diabloanopt_options.rely_on_calling_conventions)
        BBL_SET_REGS_LIVE_OUT(i_bbl, RegsetDiff(BBL_REGS_LIVE_OUT(i_bbl), RegsetDiff(desc->return_regs,FUNCTION_RET_REGS(BBL_FUNCTION(i_bbl)))));
#endif
      /* }}} */
    }
    else
    {
      BBL_SET_REGS_LIVE_OUT(i_bbl, desc->always_live);
    }
  }

  /* FUNCTION_REGS_THROUGH will be set now */

  CfgComputeFunctionThroughRegs(cfg);

  /* Use debugging information extension {{{ */
#ifdef DWARF_ARG_INFO
  if(diabloanopt_options.rely_on_calling_conventions)
  {
    CFG_FOREACH_FUN(cfg,i_fun)
    {
      {
	FUNCTION_SET_REGS_THROUGH(i_fun, RegsetDiff(FUNCTION_REGS_THROUGH(i_fun),
	      RegsetDiff(FUNCTION_DESCRIPTOR(i_fun)->argument_regs,FUNCTION_ARG_REGS(i_fun))));
      }
    }
  }
#endif
  /* }}} */

  CFG_FOREACH_HELL_FUNCTION(cfg, i_fun)
  {
    if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)))
    {
      FUNCTION_SET_REGS_THROUGH (i_fun, desc->callee_saved);
      if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)) == BBL_CH_DYNCALL)
        FUNCTION_SET_REGS_USED(i_fun, RegsetUnion(desc->callee_may_use,desc->dyncall_may_use));
    }
  }
  FUNCTION_SET_REGS_THROUGH (CFG_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_HELL_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_SWI_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_SWI_HELL_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_WRAP_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_WRAP_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_LONGJMP_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_LONGJMP_HELL_FUNCTION(cfg), desc->all_registers);

  BBL_FOREACH_SUCC_EDGE(CFG_HELL_NODE(cfg),i_edge)
    if (CFG_EDGE_CAT(i_edge)==ET_CALL && !BBL_IS_HELL(CFG_EDGE_TAIL(i_edge)))
    {
#ifdef AGGRESSIVE_LIVENESS
      t_bbl *tail = CFG_EDGE_TAIL (i_edge);
      if (diabloanopt_options.rely_on_calling_conventions && CFG_DESCRIPTION(cfg)->FunIsGlobal(BBL_FUNCTION(tail)))
	FUNCTION_SET_REGS_USED(BBL_FUNCTION(tail),
	    RegsetIntersect (FUNCTION_REGS_USED(BBL_FUNCTION(tail)),
	      desc->callee_may_use));
#endif
    }


  /*
   * Now we want to compute FUNCTION_REGS_USED and hence we set the
   * exit blocks to "no register live"
   */


  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_SET_REGS_LIVE_OUT(i_bbl, desc->always_live);
  }

  CfgComputeFunctionUsedRegs(cfg);

  /* Use debugging information extension {{{ */
#ifdef DWARF_ARG_INFO
  if (diabloanopt_options.rely_on_calling_conventions)
  {
    CFG_FOREACH_FUN(cfg,i_fun)
    {
	FUNCTION_SET_REGS_USED(i_fun, RegsetDiff(FUNCTION_REGS_USED(i_fun),
	      RegsetDiff(FUNCTION_DESCRIPTOR(i_fun)->argument_regs,FUNCTION_ARG_REGS(i_fun))));
    }
  }
#endif
  /* }}} */

  CFG_FOREACH_HELL_FUNCTION(cfg, i_fun)
  {
    if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)))
    {
      FUNCTION_SET_REGS_THROUGH (i_fun, desc->callee_saved);
      if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(i_fun)) == BBL_CH_DYNCALL)
        FUNCTION_SET_REGS_USED(i_fun, RegsetUnion(desc->callee_may_use,desc->dyncall_may_use));
    }
  }
  FUNCTION_SET_REGS_THROUGH (CFG_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_HELL_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_SWI_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_SWI_HELL_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_WRAP_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_WRAP_FUNCTION(cfg), desc->all_registers);
  
  FUNCTION_SET_REGS_THROUGH (CFG_LONGJMP_HELL_FUNCTION(cfg), desc->all_registers);
  FUNCTION_SET_REGS_USED (CFG_LONGJMP_HELL_FUNCTION(cfg), desc->all_registers);

  BBL_FOREACH_SUCC_EDGE(CFG_HELL_NODE(cfg),i_edge)
    if (CFG_EDGE_CAT(i_edge)==ET_CALL && !BBL_IS_HELL(CFG_EDGE_TAIL(i_edge)))
      {
#ifdef AGGRESSIVE_LIVENESS
	t_bbl *tail = CFG_EDGE_TAIL(i_edge);
	if (diabloanopt_options.rely_on_calling_conventions &&  CFG_DESCRIPTION(cfg)->FunIsGlobal(BBL_FUNCTION(tail)))
	  FUNCTION_SET_REGS_USED(BBL_FUNCTION(tail),
	    RegsetIntersect(FUNCTION_REGS_USED(BBL_FUNCTION(tail)),
	      desc->callee_may_use));
#endif
      }

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_SET_REGS_LIVE_OUT(i_bbl, desc->always_live);
  }

  /*
   * Finally we provide the true information about what is live at the exit block
   * we can use a clever way which requires us to save some information earlier
   * or use another fix point iteration.
   */

  SensitiveLivenessFixpoint(cfg, with_kill_useless);

  /* and than we add a forward analysis */

  if(diabloanopt_options.forward_liveness)
    CfgComputeMeaningfulRegs(cfg);

#ifdef DIABLOANOPT_DEBUG /*{{{*/
  {
    t_uint32 nr_live_out = 0, nr_live_in = 0, nr_bbls = 0;
    t_uint32 nr_live_out2 = 0, nr_live_in2 = 0, nr_bbls2 = 0;
    t_uint32 nr_arg_ret_live = 0;
    t_uint32 nr_arg_ret_live2 = 0;
    CFG_FOREACH_FUN(cfg, i_fun)
      FUNCTION_FOREACH_BBL(i_fun, i_bbl)
      {
	if(BBL_IS_HELL(i_bbl)) continue;
	if(BBL_NINS(i_bbl))
	{
	nr_bbls++;
	nr_live_out += RegsetCountRegs(RegsetIntersect(BBL_REGS_LIVE_OUT(i_bbl),description->int_registers));
	nr_live_in += RegsetCountRegs(RegsetIntersect(BblRegsLiveBefore(i_bbl),description->int_registers));
	nr_arg_ret_live += RegsetCountRegs(RegsetIntersect(BBL_REGS_LIVE_OUT(i_bbl),description->argument_regs));
	}
	/*      VERBOSE(0,("LO @B: @X",i_bbl, DPREGSET(description,BBL_REGS_LIVE_OUT(i_bbl))));*/
      }
    VERBOSE(0,("1 Average live-out: %f",((float)nr_live_out)/nr_bbls));
    VERBOSE(0,("1 Average live-in: %f",((float)nr_live_in)/nr_bbls));
    VERBOSE(0,("1 Average arg-live-out: %f",((float)nr_arg_ret_live)/nr_bbls));
    CFG_FOREACH_FUN(cfg, i_fun)
      FUNCTION_FOREACH_BBL(i_fun, i_bbl)
      {
	if(BBL_IS_HELL(i_bbl)) continue;
	nr_bbls2++;
	nr_live_out2 += RegsetCountRegs(RegsetIntersect(BBL_REGS_LIVE_OUT(i_bbl),description->int_registers));
	nr_live_in2 += RegsetCountRegs(RegsetIntersect(BblRegsLiveBefore(i_bbl),description->int_registers));
	nr_arg_ret_live2 += RegsetCountRegs(RegsetIntersect(BBL_REGS_LIVE_OUT(i_bbl),description->argument_regs));
	/*      VERBOSE(0,("LO @B: @X",i_bbl, DPREGSET(description,BBL_REGS_LIVE_OUT(i_bbl))));*/
      }
    VERBOSE(0,("2 Average live-out: %f",((float)nr_live_out2)/nr_bbls2));
    VERBOSE(0,("2 Average live-in: %f",((float)nr_live_in2)/nr_bbls2));
    VERBOSE(0,("2 Average arg-live-out: %f",((float)nr_arg_ret_live2)/nr_bbls2));

  }
#endif /*}}}*/
}
/* }}} */

/* InitSensitiveLivenessFixpoint: Init all datastructures for a
 * context-sensitive liveness analysis, i.e. mark all basic blocks and
 * functions
 *
 * {{{ */
void InitSensitiveLivenessFixpoint(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_function * i_fun;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BblMark(i_bbl);
  }

  CFG_FOREACH_FUN(cfg,i_fun)
  {
    FunctionMark(i_fun);
  }
}
/*}}}*/

/* Liveness fixpoint {{{ */

/*! Fix-point computation for context-sensitive liveness analysis. Implements
 * all three phases in the algorithm. The equations for the fixpoint can be
 * found in Robert Muth's thesis. A more detailed description can be found in
 * Bjorn De Sutter's thesis.  */

t_regset 
BblComputeLiveInFromLiveOut(t_bbl * bbl, t_bool update, t_bool with_kill_useless)
{
  t_regset in = RegsetDup(BBL_REGS_LIVE_OUT(bbl));

  if ((!update) && with_kill_useless)
  {
    t_ins * i_ins;
    if (BBL_FUNCTION(bbl) && 
	(bbl==CFG_HELL_NODE(FUNCTION_CFG(BBL_FUNCTION(bbl))) || 
	 bbl==CFG_EXIT_HELL_NODE(FUNCTION_CFG(BBL_FUNCTION(bbl))) || 
	 bbl==CFG_SWI_HELL_NODE(FUNCTION_CFG(BBL_FUNCTION(bbl))) ||  
	 bbl==CFG_EXIT_SWI_HELL_NODE(FUNCTION_CFG(BBL_FUNCTION(bbl)))))
    {
      RegsetSetDiff(in,BBL_REGS_DEF(bbl));
      RegsetSetUnion(in,BBL_REGS_USE(bbl));
    }
    else
    {
      BBL_FOREACH_INS_R(bbl,i_ins)
      {
	if (InsHasSideEffect(i_ins))
	{
	  if (!INS_IS_CONDITIONAL(i_ins)) 
	    RegsetSetDiff(in,INS_REGS_DEF(i_ins));
	  RegsetSetUnion(in,INS_REGS_USE(i_ins));
	}
	else
	{
	  t_regset tmp;
	  RegsetSetDup(tmp,in);
	  RegsetSetIntersect(tmp,INS_REGS_DEF(i_ins));
	  if (!(RegsetIsEmpty(tmp)))
	  {
	    if (!INS_IS_CONDITIONAL(i_ins)) 
	      RegsetSetDiff(in,INS_REGS_DEF(i_ins));
	    RegsetSetUnion(in,INS_REGS_USE(i_ins));
	  }
	} 
      }
#ifdef OPTNEVERLIVE
      if (diabloanopt_options.rely_on_calling_conventions)
	RegsetSetDiff(in, BBL_REGS_NEVER_LIVE(bbl));
#endif
    }
  }
  else
  {
    RegsetSetDiff(in,BBL_REGS_DEF(bbl));
    RegsetSetUnion(in,BBL_REGS_USE(bbl));
#ifdef OPTNEVERLIVE
    if (diabloanopt_options.rely_on_calling_conventions)
      RegsetSetDiff(in, BBL_REGS_NEVER_LIVE(bbl));
#endif
  }
  return in;
}

t_bool
BblPropLivenessToPredSwiReturnEdge(t_regset in, const t_architecture_description *description, t_regset *out, t_regset *old_out, t_bool *do_continue, t_cfg_edge *i_edge)
{
  t_bool change = FALSE;

  t_bbl * callsite = CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge));

  *do_continue = FALSE;

  RegsetSetDup(*old_out,BBL_REGS_LIVE_OUT(callsite));
  RegsetSetDup(*out,*old_out);

  if (description->computeLiveRegsBeforeSwi)
    description->computeLiveRegsBeforeSwi(&in,BBL_INS_LAST(callsite));

  RegsetSetUnion(*out,in);

  if(!RegsetEquals(*out,*old_out))
  {
    if (BBL_IS_HELL(callsite)) *do_continue = TRUE;
    BblMark(callsite);
    change = TRUE;
    BBL_SET_REGS_LIVE_OUT(callsite, *out);
  }
  return change;
}


t_bool
BblPropLivenessToPredNoReturnForEdge(t_regset in, const t_architecture_description *description, t_regset *out, t_regset *old_out, t_bool *do_continue, t_cfg_edge *i_edge)
{
  t_function  *callee;
  t_bool change = FALSE;

  *do_continue = FALSE;

  switch (CFG_EDGE_CAT(i_edge))
  {
    case ET_COMPENSATING:
      RegsetSetDup(*out,*old_out);
      break;
    case ET_RETURN:
      /* Just copy the regset, so that we will stop */

      if (BblIsSwiExitNode(CFG_EDGE_HEAD(i_edge)))
      {
        change = BblPropLivenessToPredSwiReturnEdge(in,description,out,old_out,do_continue,i_edge);
        return change;
      }
      else if (CFG_EDGE_CORR(i_edge))
      {
        t_regset tmp = RegsetNew();
        t_bbl * callsite = CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge));

        RegsetSetDup(*out,*old_out);
        if (!BBL_IS_HELL(callsite) && BBL_FUNCTION(callsite) && !FUNCTION_IS_HELL(BBL_FUNCTION(callsite)))
        {

          t_regset callsite_out = RegsetNew();
          t_regset callsite_oldout = RegsetNew();

          RegsetSetDup(callsite_oldout,BBL_REGS_LIVE_OUT(callsite));
          RegsetSetDup(callsite_out,callsite_oldout);

          callee = BBL_FUNCTION(CFG_EDGE_HEAD(i_edge));

          RegsetSetDup(tmp,in);
          RegsetSetIntersect(tmp,FUNCTION_REGS_THROUGH(callee));
          RegsetSetUnion(callsite_out,tmp);

          if(!RegsetEquals(callsite_out,callsite_oldout))
          {
            BblMark(callsite);
            change = TRUE;
            BBL_SET_REGS_LIVE_OUT(callsite, callsite_out);
          }
        }
      }
      break;
    default:
      FATAL(("Only return or compensating edges must be passed to BblPropLivenessToPredNoReturnForEdge, got @E",i_edge));
      break;
  }
  return change;
}


t_bool
BblPropLivenessToPredIncludingReturnForEdge(t_regset in, const t_architecture_description * description, t_regset *out, t_regset *old_out, t_bool *do_continue, t_cfg_edge *i_edge)
{
  t_function *callee;
  t_bool change = FALSE;
  *do_continue = FALSE;

  switch (CFG_EDGE_CAT(i_edge))
  {
    case ET_COMPENSATING:
    case ET_RETURN:
      {
        if (BblIsSwiExitNode(CFG_EDGE_HEAD(i_edge)))
        {
          change = BblPropLivenessToPredSwiReturnEdge(in,description,out,old_out,do_continue,i_edge);
          return change;
        }
        else
        {
          t_regset tmp = RegsetNew();
          RegsetSetDup(tmp,in);

          if (FunctionBehaves(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge))) && description->FunIsGlobal(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge))) && diabloanopt_options.rely_on_calling_conventions)
            RegsetSetDiff(tmp,description->dead_over_call);

          if (BBL_IS_HELL(CFG_EDGE_TAIL(i_edge)))
          {
            RegsetSetDiff(tmp,description->callee_may_change);
            RegsetSetUnion(tmp,description->callee_may_return);
          }

          RegsetSetDup(*out,*old_out);
          RegsetSetUnion(*out,tmp);
        }

        if (CFG_EDGE_CAT(i_edge)==ET_RETURN && CFG_EDGE_CORR(i_edge))
        {
          t_regset tmp = RegsetNew();
          t_bbl * callsite = CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge));

          if (!BBL_IS_HELL(callsite) && BBL_FUNCTION(callsite) && !FUNCTION_IS_HELL(BBL_FUNCTION(callsite)))
          {

            t_regset callsite_out = RegsetNew();
            t_regset callsite_oldout = RegsetNew();

            RegsetSetDup(callsite_oldout,BBL_REGS_LIVE_OUT(callsite));
            RegsetSetDup(callsite_out,callsite_oldout);

            callee = BBL_FUNCTION(CFG_EDGE_HEAD(i_edge));

            RegsetSetDup(tmp,in);
            RegsetSetIntersect(tmp,FUNCTION_REGS_THROUGH(callee));
            RegsetSetUnion(callsite_out,tmp);

            if(!RegsetEquals(callsite_out,callsite_oldout))
            {
              BblMark(callsite);
              change = TRUE;
              BBL_SET_REGS_LIVE_OUT(callsite, callsite_out);
            }
          }
        }
      }
      break;
    default:
      FATAL(("Only return or compensating edges must be passed to BblPropLivenessToPredNoReturnForEdge, got @E",i_edge));
      break;
  }
  return change;
}


t_bool
BblPropLivenessToPred(t_bbl * bbl, t_regset in, const t_architecture_description * description, t_bool return_edges)
{
  t_cfg_edge * i_edge;

  t_regset old_out, out;
  t_function * i_fun = BBL_FUNCTION(bbl);
  t_function *headfun;
  t_bool change = FALSE;
  t_bool do_continue;

  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
    {
      register t_bbl * head = CFG_EDGE_HEAD(i_edge);
      t_function * callee;
      t_bbl * returnsite;

      RegsetSetDup(old_out,BBL_REGS_LIVE_OUT(head));

      if (CfgEdgeIsInterproc(i_edge))
	{
	  /* IP edges {{{ */
	  switch (CFG_EDGE_CAT(i_edge))
	    {
	    case ET_COMPENSATING:
	    case ET_RETURN:
              if (!return_edges)
                change |= BblPropLivenessToPredNoReturnForEdge(in,description,&out,&old_out,&do_continue,i_edge);
              else
                change |= BblPropLivenessToPredIncludingReturnForEdge(in,description,&out,&old_out,&do_continue,i_edge);
              if (do_continue)
                continue;
              break;

            case ET_IPJUMP:
                /* tail call to hell function? -> take into account registers
                 * used by the hell call (it only contains a dummy bbl)
                 * Otherwise the registers used by the target of the
                 * IPJUMP will be propagated like those of any other bbl
                 */
                if (!FUNCTION_CALL_HELL_TYPE(i_fun))
                {
                  /* not a tail call to hell -> same as in default case */
                  RegsetSetDup(out,old_out);
                  RegsetSetUnion(out,in);
                }
                else
                {
                  t_bool handled = FALSE;
#if 0
                  The following is a nice idea, but unfortunately breaks down
                  as soon as the called routine or one it (or one of its callees)
                  calls is longjmp. In that case, registers that may be dead at
                  all return points of this routine may be live. This cannot even be
                  solved by properly adding support everywhere for
                  CFG_LONGJMP_HELL_NODE, because the setjmp may happen inside the
                  dynlib and hence we can't know what's live at every possible
                  destination of longjmp

                  /* the corresponding edge will point to a return block from
                   * which all return edges depart that go back to the original
                   * calls that can lead to this ipjump -> the live registers
                   * are the union of all live registers at the tails of all those
                   * return edges. Problem: there can be more compensating edges
                   * for ipjumps and there can even be cycles -> would require
                   * extra marking -> in that case: just assume that all registers
                   * used and passed through the called function are live
                   */
                   if (CFG_EDGE_CORR(i_edge))
                   {
                     t_cfg_edge *i_returnedge;

                     returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge));
                     ASSERT(BBL_ATTRIB(returnsite) & BBL_IS_EXITBLOCK,("corresponding edge of IPJUMP does not go to a function exit block @eiB",returnsite));
                     out = NullRegs;
                     handled = TRUE;
                     BBL_FOREACH_SUCC_EDGE(returnsite,i_returnedge)
                     {
                       switch (CFG_EDGE_CAT(i_returnedge))
                       {
                         case ET_RETURN:
                         {
                           t_bbl *returntail = CFG_EDGE_TAIL(i_returnedge);
                           RegsetSetDup(out,BBL_REGS_LIVE_OUT(returntail));
                           RegsetSetDiff(out,BBL_REGS_DEF(returntail));
                           RegsetSetUnion(out,BBL_REGS_USE(returntail));
#ifdef OPTNEVERLIVE
                           if (diabloanopt_options.rely_on_calling_conventions)
                             RegsetSetDiff(out,BBL_REGS_NEVER_LIVE(returntail));
#endif
                           break;
                         }
                         default:
                           handled = FALSE;
                           break;
                       }
                       if (!handled)
                         break;
                     }
                   }
#endif
                   if (!handled)
                   {
                     /* no corresponding edge or something else than
                      * return edges -> assume all registers used and
                      * passed through the called function may be live
                      */
                      RegsetSetDup(out,FUNCTION_REGS_THROUGH(i_fun));
                   }
                   else
                   {
                     RegsetSetIntersect(out,FUNCTION_REGS_THROUGH(i_fun));
                   }
                   RegsetSetUnion(out,FUNCTION_REGS_USED(i_fun));
                   RegsetSetUnion(out,old_out);
                }
                break;

	    case ET_CALL:
	      callee = i_fun;

	      returnsite = NULL;

	      if (CFG_EDGE_CORR(i_edge))
		returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)); 

	      if(returnsite)
		{
		  RegsetSetDup(out,BBL_REGS_LIVE_OUT(returnsite));
		  RegsetSetDiff(out,BBL_REGS_DEF(returnsite));
		  RegsetSetUnion(out,BBL_REGS_USE(returnsite));
#ifdef OPTNEVERLIVE
		  if (diabloanopt_options.rely_on_calling_conventions)
		    RegsetSetDiff(out,BBL_REGS_NEVER_LIVE(returnsite));
#endif
		}
	      else
		{
		  out = NullRegs;
		}

	      RegsetSetIntersect(out,FUNCTION_REGS_THROUGH(callee));
	      RegsetSetUnion(out,FUNCTION_REGS_USED(callee));

#ifdef LIVENESS_OPTION_CONDITIONAL_CALL_OPT
	      /* On the arm, when we have a conditional bl instruction, the
	       * link register is not live */
	      {
		t_bbl * callsite = CFG_EDGE_HEAD(i_edge);
		if (callsite && BBL_INS_LAST(callsite))
		  {
		    if (description->InsIsProcedureCall(BBL_INS_LAST(callsite))
			&& INS_ATTRIB(BBL_INS_LAST(callsite)) & IF_CONDITIONAL)
		      RegsetSetDiff(out,description->link_register);
		  }
	      }
#endif

	      if(FunctionBehaves(callee) && description->FunIsGlobal(callee) && diabloanopt_options.rely_on_calling_conventions)
		RegsetSetDiff(out,description->dead_over_call);

	      RegsetSetUnion(out,old_out);
	      break;
	    case ET_SWI:
	      {	
		t_regset tmp = RegsetNew();
		RegsetSetDup(tmp,in);
		RegsetSetDup(out,old_out);

		if (description->computeLiveRegsBeforeSwi)
		  description->computeLiveRegsBeforeSwi(&tmp,BBL_INS_LAST(head));
		
		RegsetSetUnion(out,tmp);
		RegsetFree(tmp);
	      }
	      break;
	    default:
	      RegsetSetDup(out,old_out);
	      RegsetSetUnion(out,in);
	    }
          RegsetSetUnion(out,description->always_live);
	  /*}}}*/
	  if(!RegsetEquals(out,old_out))
	    {
	      if (BBL_IS_HELL(head)) continue;
	      
	      BblMark(head);
	      if ((headfun = BBL_FUNCTION(head))) 
		FunctionMark(headfun);
	      change = TRUE;
	      BBL_SET_REGS_LIVE_OUT(head, out);
	    }
      
	}
      else 
	{
	  RegsetSetDup(out,old_out);

	  RegsetSetUnion(out,in);

	  if(!RegsetEquals(out,old_out))
	    {
	      if (BBL_IS_HELL(head)) continue;
	      BblMark(head);
	      change = TRUE;
	      BBL_SET_REGS_LIVE_OUT(head, out);
	      continue;
	    }
	}

      if(!RegsetEquals(out,old_out))
	{
	  if (BBL_IS_HELL(head)) continue;

	  BblMark(head);
	  if (CfgEdgeIsInterproc(i_edge))
	    {
	      if ((headfun = BBL_FUNCTION(head)))
		FunctionMark(headfun);
	    }
	  change = TRUE;
	  BBL_SET_REGS_LIVE_OUT(head, out);
	}
    }

  return change;
}


static void 
CfgComputeFunctionThroughRegs(t_cfg * cfg)
{
  /* iterators */
  t_bbl * i_bbl;
  t_function * i_fun;

  /* local variables */
  const t_architecture_description * description = CFG_DESCRIPTION(cfg);
  t_bool change,big_change;
  t_regset in;

  /* we assume that bbl and fun stuff has been computed already here we just do
   * the fixpoint computation */

  /* Initialize fixpoint. Basically, mark all functions and basic blocks */
  InitSensitiveLivenessFixpoint(cfg);

  /* This outer loop loops until a fixpoint is found over the entire cfg */
  do
  {
    big_change = FALSE;
    CFG_FOREACH_FUN(cfg,i_fun)
    {
      if(!(FunctionIsMarked(i_fun)))
	continue;

      FunctionUnmark(i_fun);

      /* This inner loops tries to find a fixpoint over the current function */
      do
      {
	change = FALSE;

	FUNCTION_FOREACH_BBL_R(i_fun,i_bbl)
	{
	  if(!BblIsMarked(i_bbl))
	    continue;

	  BblUnmark(i_bbl);

	  in = BblComputeLiveInFromLiveOut(i_bbl, TRUE, FALSE);

	  /* The rest is passing everything to the predecessors or function information */
	  if( BBL_IS_FIRST(i_bbl) && !FUNCTION_IS_HELL(i_fun) && FunctionGetExitBlock(i_fun))
	    {	 
	      FUNCTION_SET_REGS_THROUGH(i_fun, in);
	      FUNCTION_SET_REGS_THROUGH(i_fun, RegsetUnion(FUNCTION_REGS_THROUGH(i_fun),FUNCTION_REGS_SAVED(i_fun)));
	    }

	  change |= BblPropLivenessToPred(i_bbl,in,description,FALSE);

	  big_change |= change;
	  RegsetFree(in);
	}
      } while (change);
    }
  }
  while(big_change);
}

static void 
CfgComputeFunctionUsedRegs(t_cfg * cfg)
{
  /* iterators */
  t_bbl * i_bbl;
  t_function * i_fun;

  /* local variables */
  const t_architecture_description * description = CFG_DESCRIPTION(cfg);
  t_bool change,big_change;
  t_regset in;

  /* we assume that bbl and fun stuff has been computed already here we just do
   * the do fixpoint computation */

  /* Initialize fixpoint. Basically, mark all functions and basic blocks */
  InitSensitiveLivenessFixpoint(cfg);

  /* This outer loop loops until a fixpoint is found over the entire cfg */
  do
  {
    big_change = FALSE;
    CFG_FOREACH_FUN(cfg,i_fun)
    {
      if(!(FunctionIsMarked(i_fun)))
	continue;

      FunctionUnmark(i_fun);

      /* This inner loops tries to find a fixpoint over the current function */
      do
      {
	change = FALSE;

	FUNCTION_FOREACH_BBL_R(i_fun,i_bbl)
	{
	  if(!BblIsMarked(i_bbl))
	    continue;

	  BblUnmark(i_bbl);

	  in = BblComputeLiveInFromLiveOut(i_bbl, TRUE, FALSE);

	  /* The rest is passing everything to the predecessors or function information */
	  if( BBL_IS_FIRST(i_bbl) && !FUNCTION_IS_HELL(i_fun) )
	    {
	      /* Some registers are still used after they're saved on the stack,
	       * we have to include them in FUNCTION_REGS_USED */
	      FUNCTION_SET_REGS_USED(i_fun, in);
	      FUNCTION_SET_REGS_USED(i_fun, RegsetDiff(FUNCTION_REGS_USED(i_fun),FUNCTION_REGS_SAVED(i_fun)));
	    }

	  change |= BblPropLivenessToPred(i_bbl,in,description,FALSE);
	  big_change |= change;
	  RegsetFree(in);
	}
      } while (change);
    }
  }
  while(big_change);
}

static void 
SensitiveLivenessFixpoint(t_cfg * cfg, t_bool with_kill_useless)
{
  /* iterators */
  t_bbl * i_bbl;
  t_function * i_fun;
#ifdef LIVENESS_OPTION_CONDITIONAL_CALL_OPT
  t_cfg_edge *i_edge;
#endif
  
  /* local variables */
  const t_architecture_description * description = CFG_DESCRIPTION(cfg);
  t_bool change,big_change;
  t_regset in;


  /*
     we assume that bbl and fun stuff has been computed already
     here we just do the do fixpoint computation
   */

  InitSensitiveLivenessFixpoint(cfg);

  do
  {
    big_change = FALSE;
    CFG_FOREACH_FUN(cfg,i_fun)
    {
      if(!(FunctionIsMarked(i_fun)))
	continue;

      FunctionUnmark(i_fun);

      do
      {
	change = FALSE;

	FUNCTION_FOREACH_BBL_R(i_fun,i_bbl)
	{
	  if(!BblIsMarked(i_bbl))
	    continue;

	  BblUnmark(i_bbl);

	  in = BblComputeLiveInFromLiveOut(i_bbl, FALSE, with_kill_useless);

	  /* The rest is passing everything to the predecessors or function information */

	  change |= BblPropLivenessToPred(i_bbl,in,description,TRUE);
	  big_change |= change;
	  RegsetFree(in);
	}
      } while (change);
    }
  }
  while(big_change);

#ifdef LIVENESS_OPTION_CONDITIONAL_CALL_OPT
  CFG_FOREACH_EDGE(cfg,i_edge)
    {
      t_bbl *head = CFG_EDGE_HEAD (i_edge);
      if (CFG_EDGE_CAT(i_edge) == ET_CALL)
	if (BBL_INS_LAST(head) && INS_IS_CONDITIONAL(BBL_INS_LAST(head)))
	{
	  BBL_SET_REGS_LIVE_OUT(head, 
	      RegsetUnion(BBL_REGS_LIVE_OUT(head), description->link_register));
	}
    }
#endif
}
/* }}} */

void 
CfgComputeLiveness(t_cfg * cfg, t_analysis_complexity level)
{
  if(level==CONTEXT_SENSITIVE)
  {
    CfgComputeLivenessSensitive(cfg,FALSE);
  }
  else if(level==CONTEXT_INSENSITIVE)
  {
    CfgComputeLivenessInsensitive(cfg);
  }
  else if(level==TRIVIAL)
  {
    CfgComputeLivenessTrivial(cfg);
  }
  else
  {
    FATAL(("Bad liveness analysis method\n"));
  } 
}

void 
CfgComputeUselessRegisters (t_cfg *cfg)
{
  /* Do a context-sensitive liveness analysis with the with_kill_useless
   * flag enabled. This makes liveness analysis slightly more aggressive,
   * but the results are only valid if immediately after this analysis
   * the useless instructions are effectively removed from the program */

  CfgComputeLivenessSensitive (cfg, TRUE);
}

static void 
CfgComputeMeaningfulRegs(t_cfg * cfg)
{
/*#ifdef FORWARD_LIVENESS*/
  t_bbl * bbl;
  t_cfg_edge * edge;
  t_bool change;
  t_function * fun;

  const t_architecture_description * description = CFG_DESCRIPTION(cfg);

  t_bool marked_interprocedural, marked_intraprocedural;
  t_ins * ins;

  if (diabloanopt_options.rely_on_calling_conventions)
    {
      
      InitBblMarking();
      
      CFG_FOREACH_BBL(cfg,bbl)
	{
	  BblMark(bbl);
	  BBL_SET_REGS_DEFINED_IN(bbl, RegsetNew());
	  BBL_FOREACH_INS(bbl,ins)
	    BBL_SET_REGS_DEF(bbl,  RegsetUnion(BBL_REGS_DEF(bbl),INS_REGS_DEF(ins)));
	}
      
      CFG_FOREACH_FUN(cfg,fun)
	{
	  FunctionMark(fun);
	}
      
      BBL_FOREACH_SUCC_EDGE(CFG_HELL_NODE(cfg),edge)
	{
	  if (CFG_EDGE_CAT(edge)==ET_CALL)
	    {
	      BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), description->callee_may_use);
#ifdef DWARF_ARG_INFO
	      BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), RegsetDiff(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),RegsetDiff(description->argument_regs, FUNCTION_ARG_REGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))));
#endif
	      BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->callee_saved));
	      BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge),  RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->always_live));
	    }
	  else if (CFG_EDGE_CAT(edge)!=ET_RETURN)
	    {
	      BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), description->all_registers);      
	    }
	}

      CFG_FOREACH_HELL_FUNCTION(cfg, fun)
      {
	t_bbl *hell = FUNCTION_BBL_FIRST(fun);
	if (!BBL_CALL_HELL_TYPE(hell)) continue;

	BBL_FOREACH_SUCC_EDGE(hell, edge)
	{
	  if (CFG_EDGE_CAT(edge)==ET_CALL)
	  {
	    BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), description->callee_may_use);
#ifdef DWARF_ARG_INFO
	    BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), RegsetDiff(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),RegsetDiff(description->argument_regs, FUNCTION_ARG_REGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))));
#endif
	    BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge),  RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->callee_saved));
	    BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge),  RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->always_live));
	  }
	  else if (CFG_EDGE_CAT(edge)!=ET_RETURN)
	  {
	    BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), description->all_registers);      
	  }
	}
      }

      BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg), edge)
        {
          BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), description->callee_may_use);
          BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->callee_saved));
          BBL_SET_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge), RegsetUnion(BBL_REGS_DEFINED_IN(CFG_EDGE_TAIL(edge)),description->always_live));
        }
      
      do
	{
	  marked_interprocedural = 0;
	  
	  CFG_FOREACH_FUN(cfg,fun)
	    {
	      if (!FunctionIsMarked(fun))
		continue;
	      
	      FunctionUnmark(fun);
	      
	      do
		{
		  marked_intraprocedural = 0;
		  
		  FUNCTION_FOREACH_BBL(fun,bbl)
		    {
		      if(!BblIsMarked(bbl))
			continue;
		      
		      BblUnmark(bbl);
		      
		      if (BBL_IS_HELL(bbl)) continue;
		      
		      BBL_FOREACH_SUCC_EDGE(bbl,edge)
			{
			  t_bbl * tail = CFG_EDGE_TAIL(edge);
			  t_regset old, new;
			  t_regset max;
			  
			  if (!tail) continue;
			  
			  if (BBL_IS_HELL(tail) && CFG_EDGE_CAT(edge)!=ET_CALL) continue;

			  RegsetSetDup(old,BBL_REGS_DEFINED_IN(tail));
			  RegsetSetDup(new,old);

			  /* Forward propagation through block */
			  RegsetSetDup(max,BBL_REGS_DEFINED_IN(bbl));
			  RegsetSetUnion(max,BBL_REGS_DEF(bbl));

			  RegsetSetIntersect(max,BBL_REGS_LIVE_OUT(bbl));

			  switch (CFG_EDGE_CAT(edge))
			    {
			    case ET_CALL:
			      {
				t_function * callee = BBL_FUNCTION(tail);
				t_regset old2, new2;
				if (BBL_IS_HELL(tail))
				  {
				    t_bbl * return_node = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
				    t_regset prop;
				    /* Change callee_may_return, so you can fill in this regset with info from debug-info */
				    RegsetSetDup(prop,description->callee_may_return);
				    RegsetSetUnion(prop,description->always_live);
				    RegsetSetUnion(prop,RegsetIntersect(description->callee_saved,max));
				
				    RegsetSetDup(old2,BBL_REGS_DEFINED_IN(return_node));
				    RegsetSetDup(new2,old2);
				    RegsetSetUnion(new2,prop);

				    if (!RegsetEquals(new2,old2))				  
				      {
					BblMark(return_node);
					BBL_SET_REGS_DEFINED_IN(return_node, new2);
					if (BBL_FUNCTION(return_node) != BBL_FUNCTION(bbl))
					  {
					    if (BBL_FUNCTION(tail))
					      {
						FunctionMark(BBL_FUNCTION(tail));
						marked_interprocedural=1;
					      }
					  }
					else
					  marked_intraprocedural=1;
					change = TRUE;
				      }
				  }
				else if (CFG_EDGE_CORR(edge))
				  {
				    t_bbl * return_node;
				    t_regset prop;

				    return_node = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));


				    RegsetSetIntersect(max,RegsetUnion(RegsetUnion(FUNCTION_REGS_THROUGH(callee),FUNCTION_REGS_USED(callee)),description->always_live));
				
				    if (description->FunIsGlobal(callee) && description->InsIsIndirectCall(BBL_INS_LAST(bbl)))
				      {
					RegsetSetIntersect(max,RegsetUnion(RegsetUnion(description->callee_may_use,description->callee_saved),description->always_live));
					RegsetSetDup(prop,RegsetIntersect(description->callee_saved,max));
				      }
				    else
				      {
					RegsetSetDup(prop,RegsetIntersect(FUNCTION_REGS_THROUGH(callee),max));
				      }
				
				    RegsetSetDup(old2,BBL_REGS_DEFINED_IN(return_node));
				    RegsetSetDup(new2,old2);
				    RegsetSetUnion(new2,prop);

				    if (!RegsetEquals(new2,old2))				  
				      {
					BblMark(return_node);
					BBL_SET_REGS_DEFINED_IN(return_node, new2);
					if (BBL_FUNCTION(return_node) != BBL_FUNCTION(bbl))
					  {
					    if (BBL_FUNCTION(tail))
					      {
						FunctionMark(BBL_FUNCTION(tail));
						marked_interprocedural=1;
					      }
					  }
					else
					  marked_intraprocedural=1;
					change = TRUE;
				      }
				  }
				break;
			      }

			    case ET_RETURN:
			      {
				t_function * callee = BBL_FUNCTION(bbl);
				t_regset def_caller_out;
				t_bbl * caller = CFG_EDGE_HEAD(CFG_EDGE_CORR(edge));

				if (FUNCTION_NAME(callee) && StringPatternMatch(FUNCTION_NAME(callee),"exit"))
				  continue;

				RegsetSetDup(def_caller_out,RegsetUnion(BBL_REGS_DEF(caller),BBL_REGS_DEFINED_IN(caller)));
				RegsetSetIntersect(def_caller_out,BBL_REGS_LIVE_OUT(caller));
			    
				if (description->FunIsGlobal(callee) && description->InsIsIndirectCall(BBL_INS_LAST(caller)))
				  RegsetSetIntersect(def_caller_out,description->callee_saved);
				else
				  RegsetSetIntersect(def_caller_out,FUNCTION_REGS_THROUGH(callee));
			    
				if (description->FunIsGlobal(callee) || description->InsIsIndirectCall(BBL_INS_LAST(caller)))
				  {
				    /* Make the intersection smaller by using debuginfo */
				    RegsetSetIntersect(max,description->callee_may_return);
				    RegsetSetDiff(max,RegsetDiff(description->return_regs,FUNCTION_RET_REGS(callee)));
				  }

				RegsetSetUnion(max,def_caller_out);
				RegsetSetUnion(max,description->always_live);
			      }
			      break;
			    case ET_IPFALLTHRU:
			      if (BBL_FUNCTION(bbl) && FUNCTION_NAME(BBL_FUNCTION(bbl))  && StringPatternMatch(FUNCTION_NAME(BBL_FUNCTION(bbl)),"exit"))
				continue;
			      break;
			    }
		      
			  RegsetSetDiff(max,description->const_registers);
			  RegsetSetUnion(new,max);
		      
			  if (!RegsetEquals(new,old))
			    {
			      BblMark(tail);
			      BBL_SET_REGS_DEFINED_IN(tail, new);
			      if (CfgEdgeIsInterproc(edge))
				{
				  if (BBL_FUNCTION(tail))
				    {
				      FunctionMark(BBL_FUNCTION(tail));
				      marked_interprocedural=1;
				    }
				}
			      else
				marked_intraprocedural=1;
			      change = TRUE;
			    }
			}
		    }
		} while (marked_intraprocedural);
	    }
	}
      while(marked_interprocedural);
  

      CFG_FOREACH_BBL(cfg,bbl)
	{
	  t_regset live_out;
	  t_regset defined_out;
	  RegsetSetDup(live_out,BBL_REGS_LIVE_OUT(bbl));
	  RegsetSetDup(defined_out,BBL_REGS_DEFINED_IN(bbl));
	  RegsetSetUnion(defined_out,BBL_REGS_DEF(bbl));
 	  RegsetSetIntersect(defined_out,live_out);
	  if (!RegsetIsEmpty(RegsetDiff(live_out,defined_out)))
	    {
	      BBL_SET_REGS_LIVE_OUT(bbl, defined_out);
	    }
	}
    }
/*#endif*/

#ifdef DIABLOANOPT_DEBUG
  int regsets[23];
  int i;
  int nr_bbls = 0;
  t_reg reg;
  for (i=0;i<23;i++)
    regsets[i]=0;
      
  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BBL_IS_HELL(bbl)) continue;
      if (!BBL_FUNCTION(bbl)) continue;
      if (BBL_NINS(bbl)==0) continue;
      if (FUNCTION_BBL_LAST(BBL_FUNCTION(bbl))==bbl) continue;
      if (BBL_INS_FIRST(bbl) && INS_TYPE(BBL_INS_FIRST(bbl))==IT_DATA) continue;
      if ((AddressExtractUint32(BBL_OLD_ADDRESS(bbl)) & 0xffff) == 0xffff) continue;
	  
      if (RegsetCountRegs(RegsetDiff(RegsetIntersect(BBL_REGS_LIVE_OUT(bbl),description->int_registers),description->const_registers))==0)
	{
	  /*printf("old: %x",AddressExtractUint32(BBL_OLD_ADDRESS(bbl)));
	    DiabloPrintArch(stdout,description,"block with zero live registers @iB\n@A",bbl,BBL_REGS_LIVE_OUT(bbl) & description->int_registers);*/
	  continue;
	}
      regsets[RegsetCountRegs(RegsetDiff(RegsetIntersect(BBL_REGS_LIVE_OUT(bbl),description->int_registers),description->const_registers))]++;
    }
      
  printf("----------------------------------\n");
      
  for (i=0;i<=RegsetCountRegs(description->int_registers);i++)
    {
      printf("REGS %2d %10d\n",i,regsets[i]);
    }

  for (i=0;i<23;i++)
    regsets[i]=0;
      
  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BBL_IS_HELL(bbl)) continue;
      if (!BBL_FUNCTION(bbl)) continue;
      if (BBL_NINS(bbl)==0) continue;
      if (FUNCTION_BBL_LAST(BBL_FUNCTION(bbl))==bbl) continue;
      if (BBL_INS_FIRST(bbl) && INS_TYPE(BBL_INS_FIRST(bbl))==IT_DATA) continue;
      if ((AddressExtractUint32(BBL_OLD_ADDRESS(bbl)) & 0xffff) == 0xffff) continue;
	  
      nr_bbls++;
      if (RegsetCountRegs(RegsetDiff(RegsetIntersect(BBL_REGS_LIVE_OUT(bbl),description->int_registers),description->const_registers))==0)
	{
	  /*printf("old: %x",AddressExtractUint32(BBL_OLD_ADDRESS(bbl)));
	    DiabloPrintArch(stdout,description,"block with zero live registers @iB\n@A",bbl,BBL_REGS_LIVE_OUT(bbl) & description->int_registers); */
	  continue;
	}
      REGSET_FOREACH_REG(RegsetDiff(description->int_registers,RegsetDiff(RegsetIntersect(BBL_REGS_LIVE_OUT(bbl),description->int_registers),description->const_registers)),reg)
	regsets[reg]++;
    }
      
  printf("----------------------------------\n");
      
  for (i=0;i<23;i++)
    {
      printf("HIST %2d %10d\n",i,regsets[i]);
    }      
  VERBOSE(0,("Nr bbls %d",nr_bbls));
#endif
}

/* {{{ Dump dot files annotated with liveness information */
void CfgDumpWithLiveness(t_cfg * cfg, t_string dotsdir, t_bool livebefore, t_bool liveafter, t_bool compute_fresh)
{
  if (compute_fresh)
  {
    CfgComputeLiveness(cfg,TRIVIAL);
    CfgComputeSavedChangedRegisters(cfg);
    CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
    CfgComputeSavedChangedRegisters(cfg);
    CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  }

  LivenessAnnotatorSetOpt(livebefore, liveafter);
  CfgDrawFunctionGraphsWithLiveness (cfg, dotsdir);
}
/* }}} */

/*! General purpose OptKillUseless. Some architectures have a specialized
 * version of this function. */
static t_bool 
GenericOptKillUseless(t_cfg * cfg)
{
  /* iterators */
  t_bbl * i_bbl;
  t_ins * i_ins, * i_tmp;

  /* local variables */
  t_regset live = RegsetNew();
  t_regset use  = RegsetNew();
  t_regset def  = RegsetNew();
  t_regset tmp  = RegsetNew();
  t_uint32 kcount=0;

  STATUS(START,("Kill Useless Instructions"));
  
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (IS_DATABBL(i_bbl)) continue;
    if (!BBL_FUNCTION(i_bbl)) continue;
    if (FUNCTION_IS_HELL(BBL_FUNCTION(i_bbl)) ) continue;

    RegsetSetDup(live,BBL_REGS_LIVE_OUT(i_bbl));

    BBL_FOREACH_INS_R_SAFE(i_bbl,i_ins,i_tmp)
    {
      RegsetSetDup(use,INS_REGS_USE(i_ins));
      RegsetSetDup(def,INS_REGS_DEF(i_ins));
      RegsetSetDup(tmp,def);
      RegsetSetIntersect(tmp,live);

      if (RegsetIsEmpty(tmp) && !InsHasSideEffect(i_ins))
      {
	/*static int total_count = 0;*/
	/*if (total_count < diablosupport_options.debugcounter)*/
	  {
	    /*VERBOSE(0,("-- Killing @I in @ieB\n",i_ins,INS_BBL(i_ins)));*/
	    InsKill(i_ins);
	    /*total_count++;*/
	    kcount++;
	  }
      }
      else
      {
	if (!INS_IS_CONDITIONAL(i_ins))
	  RegsetSetDiff(live,def);
	RegsetSetUnion(live,use);
      }
    }
  }

  RegsetFree(live);
  RegsetFree(use);
  RegsetFree(def);
  RegsetFree(tmp);

  STATUS(STOP,("Kill Useless Instructions"));
  return kcount > 0;
}

t_bool 
CfgKillUselessInstructions (t_cfg *cfg)
{
  /* iteratively call CfgComputeUselessRegisters and OptKillUseless */
  t_bool change = FALSE;

  STATUS (START, ("Killing useless instructions"));
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeUselessRegisters (cfg);

  while (GenericOptKillUseless (cfg))
  {
    change = TRUE;
    CfgComputeSavedChangedRegisters (cfg);
    CfgComputeUselessRegisters (cfg);
  }
  STATUS (STOP, ("Killing useless instructions"));
  return change;
}
/* vim: set shiftwidth=2 foldmethod=marker: */
