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
#include <diabloanopti386.h>
#include "diablo_i386_options.h"
#include "diablo_common_options.h"

int
main (int argc, char **argv)
{
  t_object *obj;

  DiabloFlowgraphInit (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAnoptI386Init (argc, argv);

  DiabloI386FrontendOptionsInit (argc, argv);
  DiabloCommonFrontendOptionsInit (argc, argv);

  OptionParseCommandLine (diablo_i386_frontend_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablo_i386_frontend_option_list);

  OptionParseCommandLine (diablo_common_frontend_option_list, argc, argv, TRUE);
  OptionGetEnvironment (diablo_common_frontend_option_list);

  DiabloCommonFrontendOptionsVerify ();
  DiabloI386FrontendOptionsVerify ();

  OptionDefaults (diablo_common_frontend_option_list);
  OptionDefaults (diablo_i386_frontend_option_list);

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
            obj = RestoreState (diablo_common_frontend_options.objectfilename);
            else
            */	obj = LinkEmulate (diablo_common_frontend_options.objectfilename, diablo_common_frontend_options.read_debug_info);

    /* End Load }}} */


    if (diablo_i386_frontend_options.print_data_layout)
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
        CfgEdgeInitTestCondition(cfg);
        I386StucknessAnalysis (obj);

        /* Remove unconnected blocks */
        if (diablo_common_frontend_options.initcfgopt)
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

          CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots");

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

          CFG_FOREACH_FUN(OBJECT_CFG(obj), function)
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

        if (diablo_common_frontend_options.factoring && diablo_common_frontend_options.optimize)
          WholeFunctionFactoring (cfg);

        STATUS(START,("Optimizing, respecting calling conventions"));
        /* Analyses && Optimizations  {{{ */
        for (global_optimization_phase = 0;
             global_optimization_phase < 3; global_optimization_phase++)
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
                STATUS(STOP,("Optimizing, respecting calling conventions"));
                STATUS(START,("Optimizing, ignoring calling conventions"));
              }
            }

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
                CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

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
                DiabloBrokerCall ("BranchForwarding", cfg);
              if (diablo_common_frontend_options.branch_elimination)
                DiabloBrokerCall ("BranchElimination", cfg);
              if (diablo_common_frontend_options.mergebbls)
                DiabloBrokerCall ("MergeBbls", cfg);

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


              if (diablo_common_frontend_options.stack)
                DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);	/* only for the ARM */
              /*DiabloBrokerCall("InstructieOptimalisatie",cfg); |+ only for the MIPS +| */
              CfgComputeSavedChangedRegisters (cfg);

              if (diablo_common_frontend_options.liveness)
                dead_count += CfgKillUselessInstructions (cfg);
            }
            while (dead_count);
          }

        STATUS(STOP,("Optimizing, ignoring calling conventions"));
        {
          dominator_info_correct = FALSE;
          DominatorCleanup (cfg);
        }


        /* End of Analyses && Optimizations  }}} */

        if (diabloflowgraph_options.blockprofilefile)
          printf ("END WEIGHT %d\n", CfgComputeWeight (cfg));

        /* Export dots after optimzation {{{  */
        if (diablo_common_frontend_options.generate_dots)
        {
          if (diabloflowgraph_options.blockprofilefile)
            CfgComputeHotBblThreshold (cfg, 0.90);
          CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots-final");
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

      /*	  if (diablo_common_frontend_options.saveState)
                  SaveState (obj, SAVESTATEDISASSEMBLED);
                  */
      {
        t_string name = StringConcat2 (diablo_common_frontend_options.output_name, ".list");
        FILE *f = fopen (name, "w");
        if (f)
        {
          t_ins *ins;
          SECTION_FOREACH_INS (OBJECT_CODE (obj)[0], ins)
            FileIo (f, "@I\n", ins);
          fclose (f);
        }
        Free (name);
      }

      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
    }
    else if (diabloobject_options.dump || diabloobject_options.dump_multiple)
    {
#ifdef HAVE_DIABLO_EXEC
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
#ifdef HAVE_DIABLO_EXEC
      DiabloExec ();
#else
      FATAL (("Exec not compiled!"));
#endif
    }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (diablo_common_frontend_options.restore) RmdirR("./DUMP"); */

  DiabloCommonFrontendOptionsFini ();
  DiabloI386FrontendOptionsFini ();
  DiabloAnoptI386Fini ();
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
