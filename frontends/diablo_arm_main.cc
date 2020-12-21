/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

extern "C" {
#include <diabloflowgraph.h>
#include <diabloelf.h>
#include <diablo_options.h>

#include <diabloarm.h>
#include <diablo_arm_options.h>

#include <sys/stat.h>
}

#include <diabloflowgraph_dwarf.h>
#include <diabloanoptarm.hpp>
#include <frontends/common.h>

using namespace std;

/* the following defines are present to facilitate debugging:
   whenever one of them is set to the address of an instruction, the instruction
   at that address is printed after many optimization steps, which allows us to
   track how that instruction is manipulated by the optimizations */

#define PRINT_ADDRESS 0
#define PRINT_ADDRESS2 0

static int counter = 0;

void PrintOneIns(t_cfg * cfg, t_uint32 address, int id)
{
  t_bbl * bbl;
  t_ins * ins;

  if (address==0) return;

  if (address == UINT32_MAX)
    {
      CFG_FOREACH_BBL(cfg,bbl)
        {
          t_bool has_arm = FALSE;
          t_bool has_thumb = FALSE;
          BBL_FOREACH_INS(bbl,ins)
            {
              if (ARM_INS_TYPE(T_ARM_INS(ins))==IT_DATA) continue;
              if (ARM_INS_FLAGS(T_ARM_INS(ins)) & FL_THUMB)
                has_thumb = TRUE;
              else
                has_arm = TRUE;
            }
          if (has_arm && has_thumb)
          {
            if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) != TH_BX_R15)
            {
              /* BX r15 does jump to ARM code, so we should not FATAL then.
               * The merge-BBLs pass merges the BBL's containing Thumb and ARM code for this jump.
               */
              FATAL(("LINE %d,ARM AND THUMB IN @ieB",id,bbl));
            }
          }
        }
      return;
    }

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_INS(bbl,ins)
    {
      if (G_T_UINT32(INS_OLD_ADDRESS(ins))==address)
        {
          DEBUG(("LINE %d: @I",id,ins));
          fflush(stdout);
          return;
        }
    }

  DEBUG(("LINE %d: INS NOT FOUND",id));
  fflush(stdout);
}
#define DEBUG_INS(cfg) \
  PrintOneIns(cfg, PRINT_ADDRESS, __LINE__);\
  PrintOneIns(cfg, PRINT_ADDRESS2, __LINE__);

/* OptimizeArm {{{ */
int
OptimizeArm (t_cfg * cfg)
{
  DEBUG_INS(cfg);

  /* optimize for single-threaded apps */
  if (global_options.single_threaded)
  {
    STATUS(START, ("Optimizing for single-threaded application"));
    OptimizeSingleThreaded(cfg);
    STATUS(STOP, ("Optimizing for single-threaded application"));
  }
  DEBUG_INS(cfg);

  /* Remove unconnected blocks */
  if (global_options.initcfgopt)
  {
    STATUS(START, ("Removing unconnected blocks"));
    CfgRemoveDeadCodeAndDataBlocks (cfg);
    STATUS(STOP, ("Removing unconnected blocks"));
  }
  DEBUG_INS(cfg);

  CfgPatchToSingleEntryFunctions (cfg);
  DEBUG_INS(cfg);

  CfgRemoveDeadCodeAndDataBlocks (cfg);
  DEBUG_INS(cfg);


  if (global_options.optimize)
    MakeConstProducers (cfg);
  DEBUG_INS(cfg);

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

    CfgDrawFunctionGraphsWithHotness (cfg, "./dots");

    CgBuild (cfg);
    CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
  }
  /* }}} */

  /* {{{ dominator analysis */
  if (global_options.dominator)
  {
    ComDominators (cfg);
  }
  DEBUG_INS(cfg);
  /* Export dots after dominator */

  if (global_options.dominator && global_options.generate_dots)
  {
    t_function *function;

    DirMake ("./dots-dominator", FALSE);

    Export_FunctionDominator (CFG_UNIQUE_ENTRY_NODE(cfg), 0xffff,
                              "./dots-dominator/flowgraph.dot");

    CFG_FOREACH_FUN (cfg, function)
    {
      char *fname;
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
  DEBUG_INS(cfg);
  /* }}} */

  if (global_options.factoring && global_options.function_factoring && global_options.optimize)
    WholeFunctionFactoring (cfg);
  DEBUG_INS(cfg);

  for (global_optimization_phase = 0; global_optimization_phase < 4; global_optimization_phase++)
    if (global_options.optimize)
    {
      int dead_count = 0;
      int loopcount;
      t_bool flag = diabloanopt_options.rely_on_calling_conventions;
      diabloanopt_options.rely_on_calling_conventions = global_optimization_phase < 2;
      DEBUG_INS(cfg);

      if (diabloanopt_options.rely_on_calling_conventions != flag)
      {
        VERBOSE(1, ("USE OF CALLING CONVENTIONS CHANGED: %d!!!", diabloanopt_options.rely_on_calling_conventions));
      }

      if (global_options.remove_unconnected)
        CfgRemoveDeadCodeAndDataBlocks (cfg);

      if (global_options.move_ins_up_down)
      {
        CfgMoveInsDown (cfg);
        DEBUG_INS(cfg);
      }

      CfgComputeSavedChangedRegisters (cfg);
      DEBUG_INS(cfg);

      if (!diabloanopt_options.rely_on_calling_conventions
          && global_options.factoring && (global_options.epilogue_factoring || global_options.bbl_factoring))
      {
        if (BblFactorInit(cfg))
        {
          if (global_options.epilogue_factoring)
            FunctionEpilogueFactoring (cfg);

          CfgPatchToSingleEntryFunctions (cfg);
          if (global_options.bbl_factoring)
            BblFactoring (cfg, NULL);
        }
        BblFactorFini(cfg);
      }

      if (global_optimization_phase == 3 && global_options.switch_moves)
      {
        SwitchMoves (cfg);
      }
      DEBUG_INS(cfg);

      CfgCreateExitBlockList (cfg);
      DEBUG_INS(cfg);

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
        DEBUG_INS(cfg);

        if (!diabloanopt_options.rely_on_calling_conventions)
          if (global_options.inlining)
          {
            DiabloBrokerCall ("InlineTrivial", cfg);
          }

        if (global_options.constant_propagation && loopcount < 2) /* restrict the number of times
                                                                     constant propagation is run in
                                                                     order to reduce the overall
                                                                     running time */
        {
          if (ConstantPropagationInit(cfg))
          {
            ConstantPropagation (cfg, CONTEXT_SENSITIVE);
            OptUseConstantInformation (cfg, CONTEXT_SENSITIVE);
            FreeConstantInformation (cfg);
          }

          ConstantPropagationFini(cfg);
          DEBUG_INS(cfg);

          /* the results of the renaming can only be exploited in the next round of constant
             propagation, so don't do too much of it */

          if (loopcount<1)
            DiabloBrokerCall("RenameLocalAddressProducers",cfg);
        }
        DEBUG_INS(cfg);

        if (global_options.liveness)
          dead_count += ArmKillUselessInstructions (cfg);
        DEBUG_INS(cfg);

        /* Profile guided {{{ */
        if (arm_options.loop_invariant_code_motion)
        {
          CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

          if (diabloflowgraph_options.blockprofilefile)
          {
            ComDominators (cfg);

#ifdef SPEEDUP_DOMINATORS
            if (dominator_info_correct == FALSE)
#endif
              DominatorCleanup (cfg);
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
          }

#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
          {
            ComDominators (cfg);
          }
          if (diabloflowgraph_options.blockprofilefile)
            while (CfgHoistConstantProducingCode3 (cfg))
            {
              ComDominators (cfg);
            }

#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
          {
            DominatorCleanup (cfg);
          }
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
          {
            ComDominators (cfg);
          }
          DetectLoopStackSubAdds (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
          DetectLoopInvariants (cfg);

#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
          {
            DominatorCleanup (cfg);
          }
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

          if (diabloflowgraph_options.blockprofilefile)
          {
            CfgEstimateEdgeCounts (cfg);
            CfgComputeHotBblThreshold (cfg, 0.85);
#ifdef SPEEDUP_DOMINATORS
            if (dominator_info_correct == FALSE)
#endif
            {
              ComDominators (cfg);
            }
            LoopUnrollingSimple (cfg);
            DominatorCleanup (cfg);
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
          }
        }
        DEBUG_INS(cfg);
        /* }}} */

        /* {{{ copy analysis and propagation */
        if (diabloanopt_options.copy_analysis && loopcount < 2)
        {
          /* restrict the number of times this analysis is performed in
           * order to reduce the total running time of Diablo */
          if (CopyAnalysisInit(cfg))
          {
            CopyAnalysis (cfg);
            DEBUG_INS(cfg);

            if (diabloanopt_options.copy_propagation)
              DiabloBrokerCall ("OptCopyPropagation", cfg);	/* only does something on the ARM */

            ArmUseCopyPropagationInfo(cfg);
            DEBUG_INS(cfg);

            ArmOptEliminateCmpEdges(cfg);
            DEBUG_INS(cfg);

            if (global_options.loadstorefwd)
            {
              ArmMarkNoStackChangeFunctions (cfg);
              DiabloBrokerCall ("ArmLoadStoreFwd", cfg);
            }
          }
          else
          {
            if (diabloanopt_options.copy_propagation)
              DiabloBrokerCall ("OptCopyPropagation", cfg);	/* only does something on the ARM */
            if (global_options.loadstorefwd)
            {
              ArmMarkNoStackChangeFunctions (cfg);
              DiabloBrokerCall ("ArmLoadStoreFwd", cfg);
            }
          }
          CopyAnalysisFini (cfg);
        } /* }}} */

        CfgRemoveUselessConditionalJumps (cfg);
        DEBUG_INS(cfg);

        CfgRemoveEmptyBlocks (cfg);
        DEBUG_INS(cfg);

        if (global_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);
        DEBUG_INS(cfg);

        if (global_options.move_ins_up_down)
        {
          CfgMoveInsDown (cfg);
          DEBUG_INS(cfg);
        }

        if (global_options.branch_elimination)
        {
          if (diabloflowgraph_options.blockprofilefile)
            CfgEstimateEdgeCounts (cfg);
          DiabloBrokerCall ("BranchForwarding", cfg);
        }
        DEBUG_INS(cfg);

        if (global_options.branch_elimination)
        {
          if (diabloflowgraph_options.blockprofilefile)
            CfgEstimateEdgeCounts (cfg);
          DiabloBrokerCall ("BranchElimination", cfg);
        }
        DEBUG_INS(cfg);

        if (diabloflowgraph_options.blockprofilefile)
          CfgEstimateEdgeCounts (cfg);

        if (global_options.mergebbls)
          DiabloBrokerCall ("MergeBbls", cfg);
        DEBUG_INS(cfg);

        /* Profile guided {{{ */
        if (arm_options.pre)
        {
          STATUS(START, ("Partial Redundancy Optimizations"));
          if (diabloflowgraph_options.blockprofilefile)
          {
            CfgEstimateEdgeCounts (cfg);

            STATUS(START, ("Profile-based Partial Redundancy Eliminations - Phase 1"));
            PartialRedundancyElimination1 (cfg);
            DEBUG_INS(cfg);
            PartialRedundancyElimination1ForCalls (cfg);
            DEBUG_INS(cfg);
            PartialRedundancyElimination1ForReturns (cfg);
            DEBUG_INS(cfg);
            STATUS(STOP, ("Profile-based Partial Redundancy Eliminations - Phase 1"));
          }

          CfgComputeLiveness (cfg, TRIVIAL);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

          STATUS(START, ("Partial Redundancy Optimizations - Phase 2"));
          ReplaceTriangleWithConditionalMove (cfg);
          DEBUG_INS(cfg);
          ReplaceRectangleWithConditionalIns (cfg);
          DEBUG_INS(cfg);
          STATUS(STOP, ("Partial Redundancy Optimizations - Phase 2"));

          if (diabloflowgraph_options.blockprofilefile)
          {
            STATUS(START, ("Profile-based Partial Redundancy Eliminations - Phase 3"));
            PartialRedundancyElimination2 (cfg);
            DEBUG_INS(cfg);
            PartialRedundancyElimination3 (cfg);
            DEBUG_INS(cfg);

            if (diabloflowgraph_options.insprofilefile)
            {
              CfgComputeHotBblThreshold (cfg, 0.95);
              DetectColdCodeBundles (cfg);
            }
            STATUS(STOP, ("Profile-based Partial Redundancy Eliminations - Phase 3"));

            STATUS(START, ("Profile-based Branch Switching - Phase 4"));
            ArmCfgBranchSwitch (cfg);
            DEBUG_INS(cfg);
            ArmCfgBranchSwitch2 (cfg);
            DEBUG_INS(cfg);
            ArmCfgBranchSwitch3 (cfg);
            DEBUG_INS(cfg);
            ArmCfgBranchSwitch4 (cfg);
            DEBUG_INS(cfg);
            CfgComputeHotBblThreshold (cfg, 0.90);
            DEBUG_INS(cfg);
            CfgEstimateEdgeCounts (cfg);
            DEBUG_INS(cfg);
            STATUS(STOP, ("Profile-based Branch Switching - Phase 4"));
	        }

          STATUS(STOP, ("Partial Redundancy Optimizations"));
	      }
	      /* }}} */

        if (!diabloanopt_options.rely_on_calling_conventions)
        {
          if (global_options.inlining)
          {
          DiabloBrokerCall ("GeneralInlining", cfg);	/* Arm only */
          }
        }
        DEBUG_INS(cfg);

        if (global_options.peephole)
          DiabloBrokerCall ("ArmPeepholeOptimizations", cfg);
        DEBUG_INS(cfg);

        if (global_options.stack)
          DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);	/* only for the ARM */
        DEBUG_INS(cfg);

        if (global_options.liveness)
          dead_count += ArmKillUselessInstructions (cfg);
        DEBUG_INS(cfg);
      }
      while (dead_count);
      DEBUG_INS(cfg);

      if(global_optimization_phase > 0)
        if(arm_options.spill_code_removal)
        {
          ArmMarkNoStackChangeFunctions (cfg);
          DEBUG_INS(cfg);
          ArmLookAtStack(cfg);
          DEBUG_INS(cfg);
        }

      {
        dominator_info_correct = FALSE;
        DominatorCleanup (cfg);
      }

      if (diabloflowgraph_options.blockprofilefile)
        VERBOSE(0,("END WEIGHT OF CFG ACCORDING TO PROFILE INFORMATION: %lld", CfgComputeWeight (cfg)));
      DEBUG_INS(cfg);
    }
  DEBUG_INS(cfg);

  /* Back to the section representation (still disassembled though) */
  {
    int nr_ins = 0;
    t_bbl *bbl;
    CFG_FOREACH_BBL (cfg, bbl) nr_ins += BBL_NINS (bbl);
    printf ("nr_ins %d\n", nr_ins);
  }

  if (global_options.dominator)
    ComDominators(cfg);

  return 0;
}
/* }}} */

void OutputFilenameBroker(t_string *result)
{
  *result = StringDup(global_options.output_name);
}

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj;
  double  secs;
  double  CPUsecs;
  double  CPUutilisation;

  pid_t child_process_id = 0;
  int fd_fork_stdout;
  bool forked = false;

  start_time();
  start_CPU_time();

  PrintFullCommandline(argc, argv);

  /* Initialise used Diablo libraries */
  DiabloAnoptArmInit (argc, argv);

  /* Process command line parameters for each of the used libraries */
  ArmOptionsInit ();
  OptionParseCommandLine (arm_options_list, argc, argv, FALSE);
  OptionGetEnvironment (arm_options_list);
  ArmOptionsVerify ();
  OptionDefaults (arm_options_list);

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
    SaveState(global_options.objectfilename, const_cast<char*>("b.out"));
    exit(0);
  }

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (global_options.objectfilename) Free(global_options.objectfilename);
    global_options.objectfilename = RestoreDumpedProgram ();
  }

  RNGInitialise();
  if (global_options.random_overrides_file)
    RNGReadOverrides(global_options.random_overrides_file);

  if (!global_options.objectfilename) {
    VERBOSE(0, ("Diablo needs at least an input objectfile/executable as argument to do something."));
    return -1;
  }

  diablosupport_options.enable_transformation_log = TRUE;

  /* Figure out whether orderseed has been set or not. If it hasn't we'll initialize it anyway, unless the -Z flag has been passed. If
   * no orderseed has been set AND the -Z flag has been passed, we will use the default value. This is 0 and will result in the chains
   * being sorted for efficiency instead of them being layout randomly.
   */
  if (global_options.optimize && !diabloarm_options.orderseed_set)
    diabloarm_options.orderseed = 10;

  /* The real work: Link Emulate, Dissasemble, Flowgraph, Optimize, Deflowgraph, Assemble or just Dump {{{ */
  if (global_options.read)
  {
    RNGSetRootGenerator(RNGCreateBySeed(0, "master"));

    TryParseSymbolTranslation(global_options.objectfilename);

    /* Try emulating the original linker. */
    obj = LinkEmulate (global_options.objectfilename, diabloobject_options.read_debug_info);

    /* If enabled read DWARF info */
    if(diabloobject_options.read_debug_info)
      DwarfFlowgraphInit();

    if (global_options.self_profiling)
      SelfProfilingInit(obj, global_options.self_profiling);

    RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));

    /* Do the whole process ( dissassemble.. optimize.. ) {{{ */
    if ((global_options.disassemble)
        && (!(diabloobject_options.dump || diabloobject_options.dump_multiple)))
    {
      NewDiabloPhase("Disassemble");

      ObjectDisassemble (obj);

      if (global_options.flowgraph)
      {
        t_cfg *cfg;
        t_const_string stdout_file, static_compl_file, dynamic_compl_file, static_regions_compl_file, dynamic_regions_compl_file;

        t_const_string initial_dot_path = "./diablo-obfuscator-dots-before";
        t_const_string final_dot_path = "./diablo-obfuscator-dots-after";

        vector<string> reachable_vector;

        if (global_options.dots_before_path_set)
          initial_dot_path = global_options.dots_before_path;
        if (global_options.dots_after_path_set)
          final_dot_path = global_options.dots_after_path;

        if (global_options.self_profiling)
        {
          reachable_vector.push_back(SP_IDENTIFIER_PREFIX "Init");
        }

        DiabloBrokerCallInstall("OutputFileName", "t_string *", (void *)OutputFilenameBroker, FALSE);

        t_const_string const *force_reachable_funs = stringVectorToConstStringArray(reachable_vector);

        NewDiabloPhase("Flowgraph");
        ComplexityInitTempInfo(OBJECT_CFG(obj));
        ObjectFlowgraph (obj, NULL, force_reachable_funs, TRUE);
        delete[] force_reachable_funs;

        cfg = OBJECT_CFG(obj);

        /* we need to be able to distinguish between the main CFG (this one) and
         * CFG's created by code mobility during the deflowgraphing phase */
        SetMainCfg(cfg);

        string kill_log_name = string(global_options.output_name) + ".killed";
        OpenKilledInstructionLog(const_cast<t_string>(kill_log_name.c_str()));

        RecordFunctionsAsOriginal(cfg);

        CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgPatchToSingleEntryFunctions (cfg);
        CfgRemoveDeadCodeAndDataBlocks (cfg);

        DiabloBrokerCall("CfgConstantDistribution", cfg, global_options.output_name);

        UniqueFunctionNames(cfg);

        if (diabloflowgraph_options.blockprofilefile
            && !FileExists(diabloflowgraph_options.blockprofilefile))
        {
          /* execution profile does not exist! */
          WARNING(("Execution profile (%s) does not exist!", diabloflowgraph_options.blockprofilefile));

          Free(diabloflowgraph_options.blockprofilefile);
          diabloflowgraph_options.blockprofilefile = NULL;
          diabloflowgraph_options.blockprofilefile_set = FALSE;
        }

        if (diabloflowgraph_options.blockprofilefile)
        {
          CfgReadBlockExecutionCounts (cfg, diabloflowgraph_options.blockprofilefile);

          VERBOSE(0,("START WEIGHT OF CFG ACCORDING TO PROFILE INFORMATION: %lld", CfgComputeWeight (cfg)));

          if (diabloflowgraph_options.insprofilefile)
          {
            CfgReadInsExecutionCounts (cfg, diabloflowgraph_options.insprofilefile);
            CfgComputeHotBblThreshold (cfg, 0.90);
          }
        }

        InitialiseObjectFileTracking(cfg);
        ComplexityFiniTempInfo(OBJECT_CFG(obj));

        VERBOSE(0,("INITIAL PROGRAM COMPLEXITY REPORT"));

        CfgComputeStaticComplexity(cfg);
        CfgComputeDynamicComplexity(cfg);

        if (global_options.optimize) {
          NewDiabloPhase("Optimize");

          CfgComputeLiveness (cfg, TRIVIAL);
          CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

          if (diabloflowgraph_options.postblockprofilefile
              && !FileExists(diabloflowgraph_options.postblockprofilefile))
          {
            /* execution profile does not exist! */
            WARNING(("Execution profile (%s) does not exist!", diabloflowgraph_options.postblockprofilefile));

            Free(diabloflowgraph_options.postblockprofilefile);
            diabloflowgraph_options.postblockprofilefile = NULL;
            diabloflowgraph_options.postblockprofilefile_set = FALSE;
          }

          if (diabloflowgraph_options.postblockprofilefile)
          {
            CfgReadBlockExecutionCounts (cfg, diabloflowgraph_options.postblockprofilefile);

            VERBOSE(0,("START WEIGHT OF CFG ACCORDING TO POST PROFILE INFORMATION: %lld", CfgComputeWeight (cfg)));
          }

          t_const_string bbl_file_name = StringConcat2(global_options.output_name, ".bbl_list");
          DumpBasicBlocks(cfg, bbl_file_name);
          Free(bbl_file_name);

          CfgRemoveDeadCodeAndDataBlocks (cfg);
          CfgPatchToSingleEntryFunctions (cfg);
          CfgRemoveDeadCodeAndDataBlocks (cfg);

          string bbl_factoring_filename = string(OutputFilename()) + ".bbl_factoring.log";
          string bbl_factoring_filename_ins = bbl_factoring_filename + ".instructions";
          string bbl_factoring_filename_statistics = bbl_factoring_filename + ".statistics";
          FactoringLogInit(bbl_factoring_filename, bbl_factoring_filename_ins, bbl_factoring_filename_statistics);

          OptimizeArm(cfg);

          FactoringPrintStatistics();
          FactoringLogFini();

          if (global_options.self_profiling)
          {
            /* open up a file for the stdout of the forked process */
            stdout_file = StringConcat2(global_options.output_name, ".self_profiling.stdout");
            fd_fork_stdout = open(stdout_file, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IROTH | S_IRGRP);
            ASSERT(fd_fork_stdout != -1, ("Could not open file '%s' to redirect the stdout of the forked process to.", stdout_file));

            /* flush output streams before forking */
            fflush(stdout);
            fflush(stderr);

            {
              forked = true;
              Free(stdout_file);
              child_process_id = fork();
            }

            /* a different execution path is taken for the parent and the child process */
            if (child_process_id < 0)
            {
              FATAL(("Could not fork (%d)! Maybe use a spoon instead?", child_process_id));
            }
            else if (child_process_id > 0)
            {
              /* parent process */

              /* redirect stdout */
              dup2(fd_fork_stdout, STDOUT_FILENO);

              global_options.output_name = StringConcat2(global_options.output_name, ".self_profiling");
              final_dot_path = StringConcat2(final_dot_path, "-self_profiling");

              /* Add self-profiling */
              CfgAssignUniqueOldAddresses(cfg);
              NewDiabloPhase("Self-profiling");
              CfgComputeLiveness (cfg, TRIVIAL);/* We'll need this later on */
              CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);/* We'll need this later on */
              CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);/* We'll need this later on */
              CfgAddSelfProfiling (obj, global_options.output_name);

              DisableOriginTracking();
            }
            else
            {
              /* child process */

              /* do nothing */
            }
          }
        }

        /* Export dots after optimzation {{{  */
        if (global_options.generate_dots)
        {
          t_string callgraph_dot_file = StringConcat2(final_dot_path, "/callgraph.dot");

          if (diabloflowgraph_options.blockprofilefile)
            CfgComputeHotBblThreshold (cfg, 0.90);

          CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), final_dot_path);
          CgBuild (cfg);
          CgExport (CFG_CG(cfg), callgraph_dot_file);

          Free(callgraph_dot_file);
        }
        /* }}} */

        static_compl_file = StringConcat2(global_options.output_name, ".stat_complexity_info");
        dynamic_compl_file = StringConcat2(global_options.output_name, ".dynamic_complexity_info");

        static_regions_compl_file = StringConcat2(global_options.output_name, ".stat_regions_complexity_info");
        dynamic_regions_compl_file = StringConcat2(global_options.output_name, ".dynamic_regions_complexity_info");

        VERBOSE(0,("FINAL PROGRAM COMPLEXITY REPORT"));

        CfgStaticComplexityInit(static_compl_file);
        CfgDynamicComplexityInit(dynamic_compl_file);
        Free(static_compl_file);
        Free(dynamic_compl_file);

        CfgComputeStaticComplexity(cfg);
        CfgComputeDynamicComplexity(cfg);

        CfgStaticComplexityFini();
        CfgDynamicComplexityFini();

        NewDiabloPhase("Deflowgraph");
        ObjectDeflowgraph (obj);

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);
      } else {
        SymbolTableSetSymbolTypes(OBJECT_SUB_SYMBOL_TABLE(obj));
      }

      if (global_options.print_listing)
        ObjectPrintListing (obj, global_options.output_name);

      ObjectAssemble (obj);
    }/* }}} */
    /* Or just do nothing and dump the application {{{ */
    else if (diabloobject_options.dump || diabloobject_options.dump_multiple)
    {
#if 1//def HAVE_DIABLO_EXEC
      if (global_options.exec)
        DiabloExec ();
#else
      if (global_options.exec)
        FATAL (("Exec not compiled!"));
#endif
      DumpProgram (obj, diabloobject_options.dump_multiple);
      exit (0);
    } /*}}}*/
    /*}}}*/

    /* Clean and write out the object {{{ */
    /* remove local definitions of BuildAttributes$$*: the global one from the final
     * linked binary is the combination of all of those */
    t_const_string strip_symbol_masks[3] = {"BuildAttributes$$*", "$switch", NULL};
    ObjectConstructFinalSymbolTable(obj, strip_symbol_masks);

    ObjectWrite (obj, global_options.output_name);

#ifdef DIABLOSUPPORT_HAVE_STAT
    /* make the file executable */
    chmod (global_options.output_name,
           S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
           S_IXOTH);
#endif

  } /* }}} */

  /* Diablo Exec option {{{ */
  if (global_options.exec)
  {
#if 1//def HAVE_DIABLO_EXEC
    DiabloExec ();
#else
    FATAL (("Exec not compiled!"));
#endif
  } /*}}}*/

  RNGFinalise();

  /* Free used libraries and debug print {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */
  GlobalFini ();
  ArmOptionsFini ();
  DiabloAnoptArmFini ();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  /*}}} */

  CloseAllLibraries();

  secs = end_time();
  CPUsecs = end_CPU_time();

  printf(" TIME: %7.4fs clock wall time\n", secs);
  printf(" TIME: %7.4fs cpu time\n", CPUsecs);

  return 0;
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
