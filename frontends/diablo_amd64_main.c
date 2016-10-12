#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <diabloanopt.h>
#include <diabloflowgraph.h>
#include "frontends/diablo_amd64_options.h"
#include "diablo_options.h"

int
main (int argc, char **argv)
{
  t_object *obj;
  t_cfg *cfg;
   
  DiabloFlowgraphInit (argc, argv);
  DiabloAmd64Init (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAnoptAmd64Init (argc, argv);

  GlobalInit (argc, argv);
  Amd64OptionsInit (argc, argv);

  OptionParseCommandLine (amd64_options_list, argc, argv, FALSE);
  OptionGetEnvironment (amd64_options_list);
  Amd64OptionsVerify ();
  OptionDefaults (amd64_options_list);

  OptionParseCommandLine(global_list,argc,argv,TRUE);
  OptionGetEnvironment(global_list);
  GlobalVerify();
  OptionDefaults(global_list);

  /* III. The REAL program {{{ */
  if (global_options.read)
  {
     obj = LinkEmulate (global_options.objectfilename, FALSE);

    if (global_options.disassemble)
    {
      /* Merge code sections to ease disassembly */
      if (OBJECT_MAPPED_FIRST(obj))
	ObjectMergeCodeSections (obj);
      ObjectDisassemble (obj);
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
	t_cfg *cfg;
	ObjectFlowgraph (obj, NULL, NULL);
	cfg = OBJECT_CFG(obj);

	/* Optimize  {{{ */
	
//	CfgEdgeInitTestCondition(cfg);

	/* Remove unconnected blocks */
	if (global_options.initcfgopt)
	{
	  STATUS (START, ("Removing unconnected blocks"));
	  CfgRemoveDeadCodeAndDataBlocks (cfg);
	  STATUS (STOP, ("Removing unconnected blocks"));
	}

	CfgPatchToSingleEntryFunctions (cfg);
	CfgRemoveDeadCodeAndDataBlocks (cfg);

 /* blockprofile (dissabled)  {{{ */
	
#if 0
	if (global_options.blockprofilefile)
	{
	  CfgReadBlockExecutionCounts (cfg,
	      global_options.
	      blockprofilefile);
	  printf ("START WEIGHT %d\n", CfgComputeWeight (cfg));
	  if (global_options.insprofilefile)
	  {
	    CfgReadInsExecutionCounts (cfg,
		global_options.
		insprofilefile);
	    CfgComputeHotBblThreshold (cfg, 0.90);
	  }
	}
#endif
/* }}} */
	

	/* Dump graphs prior to optimization (dissabled) {{{ */
#if 0
	if (global_options.generate_dots)
	{
	  if (global_options.blockprofilefile)
	    CfgComputeHotBblThreshold (cfg, 0.90);
	  else if (global_options.annotate_loops)
	  {
	    ComDominators (cfg);
	    Export_Flowgraph (CFG_UNIQUE_ENTRY_NODE(cfg)),
		0xffff, "./dots/flowgraph_loop.dot");
	  }

	  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots");

	  CgBuild (cfg);
	  CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
	}
#endif
	/* }}} */

	/* {{{ dominator analysis (dissabled)*/
#if 0	
	if (global_options.dominator)
	{
	  ComDominators (cfg);
	}
	/* Export dots after dominator */
	if (global_options.dominator && global_options.generate_dots)
	{
	  t_function *function;


	  DirMake ("./dots-dominator", FALSE);

	  Export_FunctionDominator (CFG_UNIQUE_ENTRY_NODE(cfg), 0xffff,
	      "./dots-dominator/flowgraph.dot");

	  CFG_FOREACH_FUN(OBJECT_CFG(obj), function)
	  {
	    char *fname, *fname2;
	    if (FUNCTION_BBL_FIRST(function))
	    {
	      fname =
		StringIo ("./dots-dominator/@G.func-%s.dot",
		    BBL_OLD_ADDRESS (FUNCTION_BBL_FIRST(function)),
		    FUNCTION_NAME (function));
	    }
	    else
	    {
	      fname =
		StringIo ("./dots-dominator/0x0.func-%s.dot",
		    FUNCTION_NAME (function));
	    }
	    Export_FunctionDominator (FUNCTION_BBL_FIRST(function),
		~ET_INTERPROC, fname);
	    Free (fname);
	  }
	}
	if (global_options.dominator)
	  DominatorCleanup (cfg);
#endif	
	/* }}} */


//	if (global_options.factoring && global_options.optimize)
//	  WholeFunctionFactoring (cfg);

	/* Analyses && Optimizations  {{{ */
	for (global_optimization_phase = 0;
	    global_optimization_phase < 3; global_optimization_phase++)
	  if (global_options.optimize)
	  {
	    int dead_count = 0;
	    int loopcount;
	    {
	      t_bool flag =
		diabloanopt_options.rely_on_calling_conventions;
	      diabloanopt_options.rely_on_calling_conventions =
		global_optimization_phase < 2;
	      if (diabloanopt_options.rely_on_calling_conventions != flag)
	      {
		printf ("CALLING CONVENTIONS CHANGED!!!\n");
	      }


	    }

	    if (!diabloanopt_options.rely_on_calling_conventions
		&& global_options.factoring)
	    {
	      if (BblFactorInit(cfg))
	      {
		FunctionEpilogueFactoring (cfg);
		CfgPatchToSingleEntryFunctions (cfg);
		BblFactoring (cfg);
	      }
	      BblFactorFini(cfg);
	    }


	    CfgCreateExitBlockList (cfg);

	    /* always do this to make sure the live out sets of bbls are
	     * properly initialized, even if liveness is turned off */

	    CfgComputeLiveness (cfg, TRIVIAL);


	    loopcount = 0;

	    do
	    {
	      dead_count = 0;

	      loopcount++;
	      if (global_options.remove_unconnected)
		CfgRemoveDeadCodeAndDataBlocks (cfg);

	      if (!diabloanopt_options.rely_on_calling_conventions)
		if (global_options.inlining)
		{
		  DiabloBrokerCall ("InlineTrivial", cfg);
		}

//	      ComputeDefinedRegs(cfg); 


	      if (global_options.liveness)
		CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

/*	      if (global_options.constant_propagation)
	      {
		if (ConstantPropagationInit(cfg))
		{
		  ConstantPropagation (cfg, CONTEXT_SENSITIVE);
		  OptUseConstantInformation (cfg, CONTEXT_SENSITIVE);
		  FreeConstantInformation (cfg);
		}
		ConstantPropagationFini(cfg);
	      }*/


/*	      if (global_options.liveness)
	      {
		int gain = 0;


		CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
		CfgComputeSavedChangedRegisters (cfg);
		CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
		dead_count += gain = GenericOptKillUseless (cfg);

		if (gain)
		{
		  gain = 0;
		  CfgComputeSavedChangedRegisters (cfg);
		  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
		  dead_count += gain = GenericOptKillUseless (cfg);
		}

		if (gain)
		{
		  CfgComputeSavedChangedRegisters (cfg);
		  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
		  dead_count += gain = GenericOptKillUseless (cfg);
		}
	      }

*/
/*	      if (global_options.copy_analysis)
	      {
		if (CopyAnalysisInit(cfg))
		{
		  CopyAnalysis (cfg);
		}
		CopyAnalysisFini (cfg);
	      }*/

	      CfgRemoveUselessConditionalJumps (cfg);

	      CfgRemoveEmptyBlocks (cfg);


	      if (global_options.remove_unconnected)
		CfgRemoveDeadCodeAndDataBlocks (cfg);
	      CfgMoveInsDown (cfg);
//	      CfgComputeSavedChangedRegisters (cfg);

/*	      if (global_options.liveness)
	      {
		CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
	      }*/
	      if (global_options.remove_unconnected)
		CfgRemoveDeadCodeAndDataBlocks (cfg);

	      if (global_options.branch_elimination)
		DiabloBrokerCall ("BranchForwarding", cfg);
	      if (global_options.branch_elimination)
		DiabloBrokerCall ("BranchElimination", cfg);
	      if (global_options.mergebbls)
		DiabloBrokerCall ("MergeBbls", cfg);

/*	      if (!diabloanopt_options.rely_on_calling_conventions)
	      {
		if (global_options.inlining)
		{
		  DiabloBrokerCall ("GeneralInlining", cfg);	/* Arm only */
/*		}
	      }*/

//	      CfgComputeSavedChangedRegisters (cfg);

/*	      if (global_options.liveness)
	      {
		CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
		dead_count += GenericOptKillUseless (cfg);
	      }*/
	    }
	    while (dead_count);
	  }

	{
	  dominator_info_correct = FALSE;
	  DominatorCleanup (cfg);
	}


	/* End of Analyses && Optimizations  }}} */

#if 0
	if (global_options.blockprofilefile)
	  printf ("END WEIGHT %d\n", CfgComputeWeight (cfg));
#endif

	/* Export dots after optimzation (dissabled) {{{  */
#if 0	
	if (global_options.generate_dots)
	{
	  if (global_options.blockprofilefile)
	    CfgComputeHotBblThreshold (cfg, 0.90);
	  CfgDrawFunctionGraphsWithHotness (cfg, "./dots-final");
	  CgBuild (cfg);
	  CgExport (CFG_CG(cfg), "./dots-final/callgraph.dot");
	}
#endif
	/* }}} */
	/* }}} */

	/* Back to the section representation (still disassembled though) */
	ObjectDeflowgraph (obj);

	{
	  t_string list = StringConcat2 (global_options.output_name, ".list");
	  FILE *f = fopen (list, "w");
	  t_ins *ins;

	  SECTION_FOREACH_INS (OBJECT_CODE (obj)[0], ins)
	    FileIo (f, "@I\n", ins);
	  fclose (f);
	  Free (list);
	}

	/* rebuild the layout of the data sections
	 * so that every subsection sits at it's new address */
	ObjectRebuildSectionsFromSubsections (obj);
      }			/*  }}} */
      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
    }

    ObjectWrite (obj, global_options.output_name);

    /* make the file executable */
    chmod (global_options.output_name,
	S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
	S_IXOTH);
  }
  /* END REAL program }}} */

  DiabloFlowgraphFini ();
  /*DiabloAnoptFini ();*/

#ifdef DEBUG_MALLOC
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
#endif
  return 0;
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
