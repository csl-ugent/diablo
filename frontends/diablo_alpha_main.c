#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT
#include <sys/wait.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif
#include <diabloanopt.h>
#include <diabloalpha.h>
#include "frontends/diablo_alpha_options.h"
#include "diablo_options.h"

int
main (int argc, char **argv)
{
  t_object *obj;

  DiabloFlowgraphInit (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAlphaInit (argc, argv);

  AlphaOptionsInit (argc, argv);
  GlobalInit (argc, argv);

  OptionParseCommandLine (alpha_options_list, argc, argv, FALSE);
  OptionGetEnvironment (alpha_options_list);

  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);

  GlobalVerify ();
  AlphaOptionsVerify();

  OptionDefaults (global_list);
  OptionDefaults (alpha_options_list);

  /* III. The REAL program {{{ */

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (global_options.objectfilename) Free(global_options.objectfilename); 
    global_options.objectfilename = RestoreDumpedProgram ();
  }

  if (global_options.read)
    {
      /* A. Get a binary to work with {{{ */

/*      if (global_options.restoreState)
	obj = RestoreState (global_options.objectfilename);
      else
      */obj = LinkEmulate (global_options.objectfilename, FALSE);

      /* End Load }}} */


	if ((global_options.disassemble)
	    && (!(diabloobject_options.dump || diabloobject_options.dump_multiple)))

	  if (OBJECT_MAPPED_FIRST(obj))
	    ObjectMergeCodeSections (obj);

	  ObjectDisassemble (obj);
	  /* 2. {{{ Create the flowgraph */
	  if (global_options.flowgraph)
	    {
	      t_cfg *cfg;
	      ObjectFlowgraph (obj, NULL, NULL);
	      cfg = OBJECT_CFG(obj);
	      CfgEdgeInitTestCondition(cfg);

	      /* Dump graphs prior to optimization {{{ */
	      if (global_options.generate_dots)
		{
		  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots-pre");

		}
	      /* }}} */
	      /* Remove unconnected blocks */
	      if (global_options.initcfgopt)
		{
		  STATUS (START, ("Removing unconnected blocks"));
		  CfgRemoveDeadCodeAndDataBlocks (cfg);
		  STATUS (STOP, ("Removing unconnected blocks"));
		}

	      CfgPatchToSingleEntryFunctions (cfg);
	      CfgRemoveDeadCodeAndDataBlocks (cfg);

	      if (diabloflowgraph_options.blockprofilefile)
		{
		  CfgReadBlockExecutionCounts (cfg,
					       diabloflowgraph_options.
					       blockprofilefile);
		  printf ("START WEIGHT %d\n", CfgComputeWeight (cfg));
		  if (diabloflowgraph_options.insprofilefile)
		    {
		      CfgReadInsExecutionCounts (cfg,
						 diabloflowgraph_options.
						 insprofilefile);
		      CfgComputeHotBblThreshold (cfg, 0.90);
		    }
		}

	      /* Dump graphs prior to optimization {{{ */
	      if (global_options.generate_dots)
		{
		  if (diabloflowgraph_options.blockprofilefile)
		    CfgComputeHotBblThreshold (cfg, 0.90);
		  else if (global_options.annotate_loops)
		    {
		      ComDominators (cfg);
		      Export_Flowgraph (CFG_UNIQUE_ENTRY_NODE(cfg),
					0xffff, "./dots/flowgraph_loop.dot");
		    }

		  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots");

		  CgBuild (cfg);
		  CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
		}
	      /* }}} */
	      /* {{{ dominator analysis */
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
	      /* }}} */

	      if (global_options.factoring && global_options.optimize)
		WholeFunctionFactoring (cfg);

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

		    /* always do this to make sure the live out sets of bbls are properly
		     * initialized, even if liveness is turned off */

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


			/*ComputeDefinedRegs(cfg); */



			if (global_options.liveness)
			  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

			if (global_options.constant_propagation)
			  {
			    if (ConstantPropagationInit(cfg))
			    {
				    ConstantPropagation (cfg, CONTEXT_SENSITIVE);
				    OptUseConstantInformation (cfg, CONTEXT_SENSITIVE);
				    FreeConstantInformation (cfg);
			    }
			    ConstantPropagationFini(cfg);
			  }


			if (global_options.liveness)
			  dead_count += CfgKillUselessInstructions (cfg);


			if (diabloanopt_options.copy_analysis)
			{
				if (CopyAnalysisInit(cfg))
				{
					CopyAnalysis (cfg);
				}
				CopyAnalysisFini (cfg);
			}

			CfgRemoveUselessConditionalJumps (cfg);

			CfgRemoveEmptyBlocks (cfg);

			if (global_options.remove_unconnected)
			  CfgRemoveDeadCodeAndDataBlocks (cfg);

			CfgMoveInsDown (cfg);

			CfgComputeSavedChangedRegisters (cfg);

			if (global_options.liveness)
			  {
			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
			  }
			if (global_options.remove_unconnected)
			  CfgRemoveDeadCodeAndDataBlocks (cfg);

			if (global_options.branch_elimination)
			  DiabloBrokerCall ("BranchForwarding", cfg);
			if (global_options.branch_elimination)
			  DiabloBrokerCall ("BranchElimination", cfg);
			if (global_options.mergebbls)
			  DiabloBrokerCall ("MergeBbls", cfg);

			if (!diabloanopt_options.rely_on_calling_conventions)
			  {
			    if (global_options.inlining)
			      {
				DiabloBrokerCall ("GeneralInlining", cfg);	/* Arm only */
			      }
			  }


			  DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);	/* only for the ARM */
			/*DiabloBrokerCall("InstructieOptimalisatie",cfg); |+ only for the MIPS +| */
			CfgComputeSavedChangedRegisters (cfg);

			if (global_options.liveness)
			  dead_count += CfgKillUselessInstructions (cfg);
		      }
		    while (dead_count);
		  }

	      {
		dominator_info_correct = FALSE;
		DominatorCleanup (cfg);
	      }


	      /* End of Analyses && Optimizations  }}} */

	      if (diabloflowgraph_options.blockprofilefile)
		printf ("END WEIGHT %d\n", CfgComputeWeight (cfg));

	      /* Export dots after optimzation {{{  */
	      if (global_options.generate_dots)
		{
		  if (diabloflowgraph_options.blockprofilefile)
		    CfgComputeHotBblThreshold (cfg, 0.90);
		  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj),
						    "./dots-final");
		  CgBuild (cfg);
		  CgExport (CFG_CG(cfg), "./dots-final/callgraph.dot");
		}
	      /* }}} */


	      /* Back to the section representation (still disassembled though) */

	      if (1)
		{
		  int nr_ins = 0;
		  t_bbl *bbl;
		  CFG_FOREACH_BBL (cfg, bbl) nr_ins += BBL_NINS (bbl);
		  printf ("nr_ins %d\n", nr_ins);
		}


	      ObjectDeflowgraph (obj);

	      /* rebuild the layout of the data sections
	       * so that every subsection sits at it's new address */
	      ObjectRebuildSectionsFromSubsections (obj);
	    }			/*  }}} */

/*	  if (global_options.saveState)
	    SaveState (obj, SAVESTATEDISASSEMBLED);
*/
	  ObjectAssemble (obj);
	  /* End Transform and optimize }}} */
	}
      else if (diabloobject_options.dump || diabloobject_options.dump_multiple)
	{
#ifdef HAVE_DIABLO_EXEC
	  if (global_options.exec)
	    DiabloExec ();
#else
	  if (global_options.exec)
	    FATAL (("Exec not compiled!"));
#endif
	  DumpProgram (obj, diabloobject_options.dump_multiple);
	  exit (0);
	}
/*      else if (global_options.saveState)
	{
	  SaveState (obj, SAVESTATENOTDISASSEMBLED);
	}
*/


    ObjectWrite (obj, "b.out");

#ifdef DIABLOSUPPORT_HAVE_STAT
    /* make the file executable */
    chmod (global_options.output_name,
           S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
           S_IXOTH);
#endif

  }
  /* END REAL program }}} */

  if (global_options.exec)
  {
#if 1//def HAVE_DIABLO_EXEC
    DiabloExec ();
#else
    FATAL (("Exec not compiled!"));
#endif
  }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  GlobalFini ();
  AlphaOptionsFini ();
  DiabloAnoptFini ();
  DiabloFlowgraphFini ();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */

  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  /* End Fini }}} */
  return 0;
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
