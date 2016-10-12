/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include "config.h"
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
#include <diabloanoptarm.h>
#include "diablo_arm_options.h"
#include "diablo_common_options.h"

int
OptimizeArm (t_cfg * cfg)
{
  CfgEdgeInitTestCondition(cfg);

  /* Remove unconnected blocks */
  if (diablo_common_frontend_options.initcfgopt)
  {
    STATUS(START, ("Removing unconnected blocks"));
    CfgRemoveDeadCodeAndDataBlocks (cfg);
    STATUS(STOP, ("Removing unconnected blocks"));
  }

  CfgPatchToSingleEntryFunctions (cfg);
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  if (diabloflowgraph_options.blockprofilefile)
  {
    CfgReadBlockExecutionCounts (cfg,
                                 diabloflowgraph_options.blockprofilefile);
    printf ("START WEIGHT %d\n", CfgComputeWeight (cfg));
    if (diabloflowgraph_options.insprofilefile)
    {
      CfgReadInsExecutionCounts (cfg,
                                 diabloflowgraph_options.insprofilefile);
      CfgComputeHotBblThreshold (cfg, 0.90);
    }
  }


  if (diablo_common_frontend_options.optimize)
    MakeConstProducers (cfg);

  /* Dump graphs prior to optimization {{{ */
  if (diablo_common_frontend_options.generate_dots)
  {
    if (diabloflowgraph_options.blockprofilefile)
      CfgComputeHotBblThreshold (cfg, 0.90);
    else if (diablo_common_frontend_options.annotate_loops)
    {
      ComDominators (cfg);
      Export_Flowgraph (T_BBL (CFG_UNIQUE_ENTRY_NODE (cfg)),
                        0xffff, "./dots/flowgraph_loop.dot");
    }

    CfgDrawFunctionGraphsWithHotness (cfg, "./dots");

    CgBuild (cfg);
    CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
  }
  /* }}} */
  /* {{{ dominator analysis */
  if (diablo_common_frontend_options.dominator)
  {
    ComDominators (cfg);
  }
  /* Export dots after dominator */
  if (diablo_common_frontend_options.dominator && diablo_common_frontend_options.generate_dots)
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
  if (diablo_common_frontend_options.dominator)
    DominatorCleanup (cfg);
  /* }}} */

#ifdef THUMB_SUPPORT
  if (diablo_common_frontend_options.thumbtoarm)
    ThumbToArm (OBJECT_CFG(obj));
#endif


  if (diablo_common_frontend_options.factoring && diablo_common_frontend_options.optimize)
    WholeFunctionFactoring (cfg);

  for (global_optimization_phase = 0;
       global_optimization_phase < 4; global_optimization_phase++)
    if (diablo_common_frontend_options.optimize)
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

      if (diablo_common_frontend_options.remove_unconnected)
        CfgRemoveDeadCodeAndDataBlocks (cfg);
      CfgMoveInsDown (cfg);
      CfgComputeSavedChangedRegisters (cfg);

      if (!diabloanopt_options.rely_on_calling_conventions
          && diablo_common_frontend_options.factoring)
      {
        if (BblFactorInit(cfg))
        {
          FunctionEpilogueFactoring (cfg);
          CfgPatchToSingleEntryFunctions (cfg);
          BblFactoring (cfg);
        }
        BblFactorFini(cfg);
      }

      if (global_optimization_phase == 3)
      {
        SwitchMoves (cfg);
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
        if (diablo_common_frontend_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);


        if (!diabloanopt_options.rely_on_calling_conventions)
          if (diablo_common_frontend_options.inlining)
          {
            DiabloBrokerCall ("InlineTrivial", cfg);
          }


        /*ComputeDefinedRegs(cfg); */



        if (diablo_common_frontend_options.liveness)
        {
          CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
        }

        if (diablo_common_frontend_options.constant_propagation)
        {
          if (ConstantPropagationInit(cfg))
          {
            ConstantPropagation (cfg, CONTEXT_SENSITIVE);
            OptUseConstantInformation (cfg, CONTEXT_SENSITIVE);
            FreeConstantInformation (cfg);
          }
          ConstantPropagationFini(cfg);
        }


        if (diablo_common_frontend_options.liveness)
          dead_count += ArmKillUselessInstructions (cfg);

        if (diablo_arm_frontend_options.loop_invariant_code_motion)
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
            ComDominators (cfg);
          /*			    if (diabloflowgraph_options.blockprofilefile)
                                    while (CfgHoistConstantProducingCode3 (cfg))
                                    ComDominators (cfg);
                                    */				
#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
            DominatorCleanup (cfg);

          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
            ComDominators (cfg);
          /*			    DetectLoopStackSubAdds (cfg);
          */			    CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
          /*			    DetectLoopInvariants (cfg);
          */
#ifdef SPEEDUP_DOMINATORS
          if (dominator_info_correct == FALSE)
#endif
            DominatorCleanup (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

          if (diabloflowgraph_options.blockprofilefile)
          {
            CfgEstimateEdgeCounts (cfg);
            CfgComputeHotBblThreshold (cfg, 0.85);
#ifdef SPEEDUP_DOMINATORS
            if (dominator_info_correct == FALSE)
#endif
              ComDominators (cfg);
            /*				LoopUnrollingSimple (cfg);
            */				DominatorCleanup (cfg);
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
          }
        }

        if (diabloanopt_options.copy_analysis)
        {
          if (CopyAnalysisInit(cfg))
          {
            CopyAnalysis (cfg);
            if (diabloanopt_options.copy_propagation)
              DiabloBrokerCall ("OptCopyPropagation", cfg);	/* only does something on the ARM */
            if (diablo_common_frontend_options.loadstorefwd)
              DiabloBrokerCall ("ArmLoadStoreFwd", cfg);
          }
          else
          {
            if (diabloanopt_options.copy_propagation)
              DiabloBrokerCall ("OptCopyPropagation", cfg);	/* only does something on the ARM */
            if (diablo_common_frontend_options.loadstorefwd)
              DiabloBrokerCall ("ArmLoadStoreFwd", cfg);
          }
          CopyAnalysisFini (cfg);
        }

        CfgRemoveUselessConditionalJumps (cfg);

        CfgRemoveEmptyBlocks (cfg);


        if (diablo_common_frontend_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgMoveInsDown (cfg);
        CfgComputeSavedChangedRegisters (cfg);

        if (diablo_common_frontend_options.liveness)
        {
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
        }
        if (diablo_common_frontend_options.remove_unconnected)
          CfgRemoveDeadCodeAndDataBlocks (cfg);

        if (diablo_common_frontend_options.branch_elimination)
        {
          if (diabloflowgraph_options.blockprofilefile)
            CfgEstimateEdgeCounts (cfg);
          DiabloBrokerCall ("BranchForwarding", cfg);
        }
        if (diablo_common_frontend_options.branch_elimination)
        {
          if (diabloflowgraph_options.blockprofilefile)
            CfgEstimateEdgeCounts (cfg);
          DiabloBrokerCall ("BranchElimination", cfg);
        }

        if (diabloflowgraph_options.blockprofilefile)
          CfgEstimateEdgeCounts (cfg);

        if (diablo_common_frontend_options.mergebbls)
          DiabloBrokerCall ("MergeBbls", cfg);

        if (diablo_arm_frontend_options.pre)
        {
          if (diabloflowgraph_options.blockprofilefile)
          {
            CfgEstimateEdgeCounts (cfg);
            /*				PartialRedundancyElimination1 (cfg);
                                        PartialRedundancyElimination1ForCalls (cfg);
                                        PartialRedundancyElimination1ForReturns (cfg);
                                        */		      }

            CfgComputeLiveness (cfg, TRIVIAL);
            CfgComputeSavedChangedRegisters (cfg);
            CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
            CfgComputeSavedChangedRegisters (cfg);
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

            /*			    ReplaceTriangleWithConditionalMove (cfg);

*/			    /*ReplaceRectangleWithConditionalIns (cfg);*/

            if (diabloflowgraph_options.blockprofilefile)
            {
              /*				PartialRedundancyElimination2 (cfg);
                                                PartialRedundancyElimination3 (cfg);
                                                */
              if (diabloflowgraph_options.insprofilefile)
              {
                CfgComputeHotBblThreshold (cfg, 0.95);
                /*				    DetectColdCodeBundles (cfg);
                */				  }

                /*				CfgBranchSwitch (cfg);
                                                CfgBranchSwitch2 (cfg);
                                                CfgBranchSwitch3 (cfg);
                                                CfgBranchSwitch4 (cfg);
                                                */				CfgComputeHotBblThreshold (cfg, 0.90);
                CfgEstimateEdgeCounts (cfg);
            }
        }

        /*                        if (!diablo_common_frontend_options.rely_on_calling_conventions)*/
        if(global_optimization_phase > 0)
          if(diablo_arm_frontend_options.spill_code_removal) ArmLookAtStack(cfg);

        if (!diabloanopt_options.rely_on_calling_conventions)
        {
          if (diablo_common_frontend_options.inlining)
          {
            DiabloBrokerCall ("GeneralInlining", cfg);	/* Arm only */
            DiabloBrokerCall ("I386InlineSimple", cfg);
            /*DiabloBrokerCall("I386InlineFunctionsWithOneCallSite",cfg); */
          }
        }



        if (diablo_common_frontend_options.peephole)
          DiabloBrokerCall ("I386PeepHoles", cfg);


        if (diablo_common_frontend_options.peephole)
          DiabloBrokerCall ("ArmPeepholeOptimizations", cfg);

        if (diablo_common_frontend_options.stack)
          DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);	/* only for the ARM */
        /*DiabloBrokerCall("InstructieOptimalisatie",cfg); |+ only for the MIPS +| */
        CfgComputeSavedChangedRegisters (cfg);

        if (diablo_common_frontend_options.liveness)
          dead_count += ArmKillUselessInstructions (cfg);
      }
      while (dead_count);

      {
        dominator_info_correct = FALSE;
        DominatorCleanup (cfg);
      }



      if (diabloflowgraph_options.blockprofilefile)
        printf ("END WEIGHT %d\n", CfgComputeWeight (cfg));

    }
  /* Export dots after optimzation {{{  */
  if (diablo_common_frontend_options.generate_dots)
  {
    if (diabloflowgraph_options.blockprofilefile)
      CfgComputeHotBblThreshold (cfg, 0.90);
    CfgDrawFunctionGraphsWithHotness (cfg,
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

  if (0)
  {
    t_function * fun;
    t_bbl * bbl;
    t_ins * ins;
    int weight = 0;

    CFG_FOREACH_FUN(cfg,fun)
    {
      weight = 0;
      FUNCTION_FOREACH_BBL(fun,bbl)
        if (!IS_DATABBL(bbl))
        {
          //			    VERBOSE(0,("     FUNW BLOCK %d @iB\n", BBL_EXEC_COUNT(bbl)*BBL_NINS(bbl),bbl));
          BBL_FOREACH_INS(bbl,ins)
            //      if (INS_TYPE(ins)==IT_LOAD || INS_TYPE(ins)==IT_LOAD_MULTIPLE || INS_TYPE(ins)==IT_FLT_LOAD)
            weight += BBL_EXEC_COUNT(bbl);
        }

      printf("FUNW %d %s\n",weight,FUNCTION_NAME(fun));

    }
  }

  ComDominators(cfg);

  return 0;
}

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj;

  DiabloFlowgraphInit (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAnoptArmInit (argc, argv);

  DiabloArmFrontendOptionsInit (argc, argv);
  DiabloCommonFrontendOptionsInit (argc, argv);

  OptionParseCommandLine (diablo_arm_frontend_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablo_arm_frontend_option_list);

  OptionParseCommandLine (diablo_common_frontend_option_list, argc, argv, TRUE);
  OptionGetEnvironment (diablo_common_frontend_option_list);

  DiabloCommonFrontendOptionsVerify ();
  DiabloArmFrontendOptionsVerify ();

  OptionDefaults (diablo_common_frontend_option_list);
  OptionDefaults (diablo_arm_frontend_option_list);

  /* III. The REAL program {{{ */

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (diablo_common_frontend_options.objectfilename) Free(diablo_common_frontend_options.objectfilename); 
    diablo_common_frontend_options.objectfilename = RestoreDumpedProgram ();
  }

  if (diablo_common_frontend_options.read)
  {
    /* A. Get a binary to work with {{{ */

    /*      if (diablo_common_frontend_options.restoreState)
            obj = RestoreState ();
            else
            */	obj = LinkEmulate (diablo_common_frontend_options.objectfilename,FALSE);

    /* End Load }}} */


	  RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));
	  
    if ((diablo_common_frontend_options.disassemble)
        && (!(diabloobject_options.dump || diabloobject_options.dump_multiple)))
    {
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */

      ObjectDisassemble (obj);
      /* 2. {{{ Create the flowgraph */
      if (diablo_common_frontend_options.flowgraph)
      {
        t_cfg *cfg;
        ObjectFlowgraph (obj, NULL, NULL, FALSE);
        cfg = OBJECT_CFG(obj);

        OptimizeArm(cfg);

        ObjectDeflowgraph (obj);

        /* debug code: list final program */
        {
          int i;
          t_section *sec;
          t_ins *ins;
          FILE *f;
          t_string name=StringConcat2(diablo_common_frontend_options.output_name,".list");

          f = fopen (name, "w");
          ASSERT (f, ("Could not open %s for writing", name));
          Free (name);
          OBJECT_FOREACH_CODE_SECTION (obj, sec, i)
          {
            FileIo (f,
                    "========================[%s]========================\n",
                    SECTION_NAME (sec));
            SECTION_FOREACH_INS (sec, ins)
              FileIo (f, "@I\n", ins);
          }
        }

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);
      }			/*  }}} */

      /*	  if (diablo_common_frontend_options.saveState)
                  SaveState (obj, SAVESTATEDISASSEMBLED);
                  */
      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
    }
    else if (diabloobject_options.dump || diabloobject_options.dump_multiple)
    {
#if 1//def HAVE_DIABLO_EXEC
      if (diablo_common_frontend_options.exec)
        DiabloExec ();
#else
      if (diablo_common_frontend_options.exec)
        FATAL (("Exec not compiled!"));
#endif
      DumpProgram (obj, diabloobject_options.dump_multiple);
      exit (0);
    }
    /*      else if (diablo_common_frontend_options.saveState)
            {
            SaveState (obj, SAVESTATENOTDISASSEMBLED);
            }
            */


    ObjectWrite (obj, diablo_common_frontend_options.output_name);

#ifdef DIABLOSUPPORT_HAVE_STAT
    /* make the file executable */
    chmod (diablo_common_frontend_options.output_name,
           S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
           S_IXOTH);
#endif

  }
  /* END REAL program }}} */

  if (diablo_common_frontend_options.exec)
  {
#if 1//def HAVE_DIABLO_EXEC
    DiabloExec ();
#else
    FATAL (("Exec not compiled!"));
#endif
  }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (diablo_common_frontend_options.restore) RmdirR("./DUMP"); */

  DiabloCommonFrontendOptionsFini ();
  DiabloArmFrontendOptionsFini ();
  DiabloAnoptArmFini ();
  DiabloAnoptFini ();
  DiabloFlowgraphFini ();
  DiabloBrokerFini ();

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
