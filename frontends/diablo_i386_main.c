/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <diabloanopti386.h>
#include <diablosupport.h>
#include <diabloelf.h>
#ifndef _MSC_VER
#include <diablo_i386_options.h>
#else
#include "diablo_i386_frontend_options.h"
#endif
#include <diablo_options.h>

#include <support/version.h>

/* OptimizeI386 {{{ */
int
OptimizeI386(t_cfg * cfg)
{
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
    CfgReadBlockExecutionCounts (cfg, diabloflowgraph_options.blockprofilefile);
    printf ("START WEIGHT %"PRId64"\n", CfgComputeWeight (cfg));
    if (diabloflowgraph_options.insprofilefile)
    {
      CfgReadInsExecutionCounts (cfg, diabloflowgraph_options.insprofilefile);
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
      Export_Flowgraph (CFG_UNIQUE_ENTRY_NODE(cfg), 0xffff, "./dots/flowgraph_loop.dot");
    }

    CfgDrawFunctionGraphsWithHotness (cfg, "./dots");

    CgBuild (cfg);
    CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
  }
  /* }}} */
  /* {{{ dominator analysis */
  if (global_options.dominator)
    ComDominators (cfg);

  /* Export dots after dominator */
  if (global_options.dominator && global_options.generate_dots)
  {
    t_function *function;

    DirMake ("./dots-dominator", FALSE);

    Export_FunctionDominator (CFG_UNIQUE_ENTRY_NODE(cfg), 0xffff, "./dots-dominator/flowgraph.dot");

    CFG_FOREACH_FUN(cfg, function)
    {
      char *fname;
      if (FUNCTION_BBL_FIRST(function))
      {
        fname = StringIo ("./dots-dominator/@G.func-%s.dot", BBL_OLD_ADDRESS (FUNCTION_BBL_FIRST(function)),
          FUNCTION_NAME (function));
      }
      else
      {
        fname = StringIo ("./dots-dominator/0x0.func-%s.dot", FUNCTION_NAME (function));
      }
      Export_FunctionDominator (FUNCTION_BBL_FIRST(function), ~ET_INTERPROC, fname);
      Free (fname);
    }
  }

  if (global_options.dominator)
    DominatorCleanup (cfg);
  /* }}} */

  if (global_options.factoring && global_options.optimize)
    WholeFunctionFactoring (cfg);

  /* Analyses && Optimizations  {{{ */
  for (global_optimization_phase = 0; global_optimization_phase < 3; global_optimization_phase++)
  {
    if (global_options.optimize)
    {
      int dead_count = 0;
      int loopcount = 0;

      t_bool flag = diabloanopt_options.rely_on_calling_conventions;
      diabloanopt_options.rely_on_calling_conventions = global_optimization_phase < 2;
      if (diabloanopt_options.rely_on_calling_conventions != flag)
        printf ("CALLING CONVENTIONS CHANGED!!!\n");

      if (!diabloanopt_options.rely_on_calling_conventions && global_options.factoring)
      {
        if (BblFactorInit (cfg))
        {
          FunctionEpilogueFactoring (cfg);
          CfgPatchToSingleEntryFunctions (cfg);
          BblFactoring (cfg, NULL);
        }
        BblFactorFini (cfg);
      }

      CfgCreateExitBlockList (cfg);

      /* always do this to make sure the live out sets of bbls are properly
       * initialized, even if liveness is turned off */
      CfgComputeLiveness (cfg, TRIVIAL);

      do
      {
        dead_count = 0;

        loopcount++;
        if (global_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);


        if (!diabloanopt_options.rely_on_calling_conventions && global_options.inlining)
          DiabloBrokerCall ("InlineTrivial", cfg);

        /*ComputeDefinedRegs(cfg); */

        if (global_options.liveness)
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

        if (global_options.constant_propagation)
        {
          if (ConstantPropagationInit (cfg))
          {
            ConstantPropagation (cfg, CONTEXT_SENSITIVE);
            OptUseConstantInformation (cfg, CONTEXT_SENSITIVE);
            FreeConstantInformation (cfg);
          }
          ConstantPropagationFini (cfg);
        }


        if (global_options.liveness)
          dead_count += CfgKillUselessInstructions (cfg);


        if (diabloanopt_options.copy_analysis)
        {
          if (CopyAnalysisInit (cfg))
            CopyAnalysis (cfg);

          CopyAnalysisFini (cfg);
        }

        CfgRemoveUselessConditionalJumps (cfg);

        CfgRemoveEmptyBlocks (cfg);

        if (global_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);

        if (global_options.move_ins_up_down)
          CfgMoveInsDown (cfg);

        CfgComputeSavedChangedRegisters (cfg);

        if (global_options.liveness)
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

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
            DiabloBrokerCall ("GeneralInlining", cfg);  /* Arm only */
            DiabloBrokerCall ("I386InlineSimple", cfg);
            /*DiabloBrokerCall("I386InlineFunctionsWithOneCallSite",cfg); */
          }
        }

        if (global_options.peephole)
          DiabloBrokerCall ("I386PeepHoles", cfg);

        if (global_options.stack)
          DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg); /* only for the ARM */

        CfgComputeSavedChangedRegisters (cfg);

        if (global_options.liveness)
          dead_count += CfgKillUselessInstructions (cfg);
      }
      while (dead_count);
    }
  }
  /*}}}*/

  dominator_info_correct = FALSE;
  DominatorCleanup (cfg);

  return 0;
}
/*}}}*/

int
main (int argc, char **argv)
{
  t_object *obj;
  double  secs;
  double  CPUsecs;
  double  CPUutilisation;

  start_time();
  start_CPU_time();

  /* Initialise used Diablo libraries */
  DiabloAnoptI386Init (argc, argv);

  /* Process command line parameters for each of the used libraries */
  I386OptionsInit ();
  OptionParseCommandLine (i386_options_list, argc, argv, FALSE);
  OptionGetEnvironment (i386_options_list);
  I386OptionsVerify ();
  OptionDefaults (i386_options_list);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  PrintVersionInformationIfRequested();

  if (global_options.saveState)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    SaveState(global_options.objectfilename, "b.out");
    GlobalFini();
    I386OptionsFini();
    DiabloAnoptI386Fini ();

    /* If DEBUG MALLOC is defined print a list of all memory
     * leaks. Needs to increase the verbose-level to make
     * sure these get printed. This is a hack but it may be
     * removed when a real version of diablo is made */

    diablosupport_options.verbose++;
    PrintRemainingBlocks ();

    exit(0);
  }

  /* III. The REAL program {{{ */
  if (global_options.random_overrides_file)
    RNGReadOverrides(global_options.random_overrides_file);

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (global_options.objectfilename)
      Free(global_options.objectfilename);
    global_options.objectfilename = RestoreDumpedProgram ();
  }

  if (global_options.read)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    obj = LinkEmulate (global_options.objectfilename,FALSE);

    if (i386_options.print_data_layout)
    {
      t_object *subobj;
      t_object *tmp;
      t_section *sec;
      int tel;
      OBJECT_FOREACH_SUBOBJECT (obj, subobj, tmp)
	OBJECT_FOREACH_SECTION (subobj, sec, tel)
	{
	  if (SECTION_TYPE (sec) == DATA_SECTION)
	    VERBOSE (0,
		(".data 0x%x 0x%x\n", SECTION_OLD_ADDRESS (sec),
		 SECTION_CSIZE (sec)));
	  else if (SECTION_TYPE (sec) == BSS_SECTION)
	    VERBOSE (0,
		(".bss 0x%x 0x%x\n", SECTION_OLD_ADDRESS (sec),
		 SECTION_CSIZE (sec)));
	}
      exit (0);
    }

    if (global_options.self_profiling)
      SelfProfilingInit(obj, global_options.self_profiling);

    if ((global_options.disassemble)
        && (!(diabloobject_options.dump || diabloobject_options.dump_multiple)))
    {
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */
      /* Merge code sections to ease disassembly */
      NewDiabloPhase("Disassemble");

      ObjectDisassemble (obj);
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
	t_cfg *cfg;

        NewDiabloPhase("Flowgraph");

        if (global_options.self_profiling)
        {
          t_const_string force_reachable[3];
          force_reachable[0] = FINAL_PREFIX_FOR_LINKED_IN_PROFILING_OBJECT "print";
          force_reachable[1] = FINAL_PREFIX_FOR_LINKED_IN_PROFILING_OBJECT "Init";
          force_reachable[2] = NULL;
          ObjectFlowgraph (obj, NULL, force_reachable, FALSE);
        }
        else
          ObjectFlowgraph (obj, NULL, NULL, FALSE);
        cfg = OBJECT_CFG(obj);
        I386StucknessAnalysis (obj);

        NewDiabloPhase("Optimize");

        OptimizeI386(cfg);

	/* Add self-profiling */
        if (global_options.self_profiling)
        {
          NewDiabloPhase("Self-profiling");
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);/* We'll need this later on */
          CfgAddSelfProfiling (obj, global_options.output_name);
        }

	if (diabloflowgraph_options.blockprofilefile)
	  printf ("END WEIGHT %"PRId64"\n", CfgComputeWeight (cfg));

	/* Export dots after optimzation {{{  */
	if (global_options.generate_dots)
	{
	  if (diabloflowgraph_options.blockprofilefile)
	    CfgComputeHotBblThreshold (cfg, 0.90);
	  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots-final");
	  CgBuild (cfg);
	  CgExport (CFG_CG(cfg), "./dots-final/callgraph.dot");
	}
	/* }}} */

        NewDiabloPhase("Deflowgraph");

	ObjectDeflowgraph (obj);

        if (global_options.print_listing)
          ObjectPrintListing (obj, global_options.output_name);

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

    /* construct the final symbol table */
    ObjectConstructFinalSymbolTable(obj, NULL);

    ObjectWrite (obj, global_options.output_name);

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
#ifdef HAVE_DIABLO_EXEC
    DiabloExec ();
#else
    FATAL (("Exec not compiled!"));
#endif
  }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  GlobalFini();
  I386OptionsFini();
  DiabloAnoptI386Fini ();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */

  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  /* End Fini }}} */

  secs = end_time();
  CPUsecs = end_CPU_time();
  CPUutilisation = CPUsecs /  secs * 100.0;
  printf(" TIME: %7.4fs clock wall time\n", secs);
  printf(" TIME: %7.4fs cpu time\n", CPUsecs);

  return 0;
}
/* vim: set shiftwidth=2 foldmethod=marker:*/
