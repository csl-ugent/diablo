#include <diabloppc64.h>
#include "diablo_ppc64_options.h"
#include "diablo_options.h"
#include <diabloanopt.h>
#include <diabloanoptppc.h>

/* OptimizePpc64 {{{ */
static int
ppc64_rewrite (t_object* obj, t_cfg *cfg)
{
  /* Remove unconnected blocks */
  if (global_options.initcfgopt)
  {
    STATUS (START, ("Removing unconnected blocks"));
    CfgRemoveDeadCodeAndDataBlocks (cfg);
    STATUS (STOP, ("Removing unconnected blocks"));
  }
  
  CfgPatchToSingleEntryFunctions (cfg);

  if (global_options.initcfgopt)
    CfgRemoveDeadCodeAndDataBlocks (cfg);
  
  if (diabloflowgraph_options.blockprofilefile)
  {
    CfgReadBlockExecutionCounts (cfg,
                                 diabloflowgraph_options.
                                 blockprofilefile);
    printf ("START WEIGHT %lld\n", CfgComputeWeight (cfg));
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
  /* Export dots after dominator */
  if (global_options.dominator && global_options.generate_dots)
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
  if (global_options.dominator)
    DominatorCleanup (cfg);
  /* }}} */
  
  if (global_options.factoring &&
      global_options.function_factoring &&
      global_options.optimize)
    WholeFunctionFactoring (cfg);
  
  /* Analyses && Optimizations  {{{ */
  for (global_optimization_phase = 0;
       global_optimization_phase < 3; global_optimization_phase++)
    if (global_options.optimize)
    {
      int dead_count = 0;
      int loopcount;
      {
        t_bool flag = diabloanopt_options.rely_on_calling_conventions;
        diabloanopt_options.rely_on_calling_conventions =
          flag && (global_optimization_phase < 2);

        if (diabloanopt_options.rely_on_calling_conventions != flag)
        {
          printf ("CALLING CONVENTIONS CHANGED!!!\n");
        }
        
        
      }
      
      if (!diabloanopt_options.rely_on_calling_conventions
          && global_options.factoring
          && global_options.bbl_factoring)
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
          dead_count+=CfgRemoveDeadCodeAndDataBlocks (cfg);

/*        
        Not yet implemented for PPC

        if (!diabloanopt_options.rely_on_calling_conventions)
          if (global_options.inlining)
          {
            DiabloBrokerCall ("InlineTrivial", cfg);
          }
*/

        /*ComputeDefinedRegs(cfg); */

        
        if (global_options.liveness)
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

/*
      Not yet implemented for PPC

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
*/

        if (global_options.liveness)
          dead_count += CfgKillUselessInstructions (cfg);
        
/*
        Not yet implemented for PPC

        if (diabloanopt_options.copy_analysis)
        {
          if (CopyAnalysisInit(cfg))
          {
            CopyAnalysis (cfg);
          }
          CopyAnalysisFini (cfg);
        }
 */
        
        CfgRemoveUselessConditionalJumps (cfg);

        CfgRemoveEmptyBlocks (cfg);

        if (global_options.remove_unconnected)
          dead_count+=CfgRemoveDeadCodeAndDataBlocks (cfg);

        CfgMoveInsDown (cfg);

        CfgComputeSavedChangedRegisters (cfg);

        if (global_options.liveness)
        {
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
        }
        if (global_options.remove_unconnected)
          dead_count+=CfgRemoveDeadCodeAndDataBlocks (cfg);

        if (global_options.branch_elimination)
        {
          DiabloBrokerCall ("BranchForwarding", cfg);
          DiabloBrokerCall ("BranchElimination", cfg);
        }

        if (global_options.mergebbls)
          DiabloBrokerCall ("MergeBbls", cfg);

/*
      Not yet implemented for PPC

      if (!diabloanopt_options.rely_on_calling_conventions)
        {
          if (global_options.inlining)
          {
            DiabloBrokerCall ("GeneralInlining", cfg);
                              DiabloBrokerCall ("I386InlineSimple", cfg);
           DiabloBrokerCall("I386InlineFunctionsWithOneCallSite",cfg);
          }
        }
*/

/*
        Not yet implemented for PPC

        if (global_options.peephole)
          DiabloBrokerCall ("I386PeepHoles", cfg);

        if (global_options.stack)
          DiabloBrokerCall ("OptimizeStackLoadAndStores", cfg);
*/

        CfgComputeSavedChangedRegisters (cfg);
                  
        if (global_options.liveness)
          dead_count+=CfgKillUselessInstructions (cfg);

        if (global_options.remove_unconnected)
          dead_count+=CfgRemoveDeadCodeAndDataBlocks (cfg);
      }
      while (dead_count);
    }

    {
      dominator_info_correct = FALSE;
      DominatorCleanup (cfg);
    }

  /* End of Analyses && Optimizations  }}} */

  if (diabloflowgraph_options.blockprofilefile)
    printf ("END WEIGHT %lld\n", CfgComputeWeight (cfg));
  
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

  return 1;
}
/* }}} */

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj, *tmp;

  DiabloAnoptPpcInit (argc, argv);
  DiabloPpc64Init (argc, argv);

  DiabloPpc64OptionsInit (argc, argv);
  GlobalInit (argc, argv);

  OptionParseCommandLine (diablo_ppc64_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablo_ppc64_option_list);

  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);

  GlobalVerify ();
  DiabloPpc64OptionsVerify ();

  OptionDefaults (global_list);
  OptionDefaults (diablo_ppc64_option_list);

  if (global_options.saveState)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    SaveState(global_options.objectfilename, global_options.output_name);
    exit(0);
  }

  /* III. The REAL program {{{ */

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    if (global_options.objectfilename) Free(global_options.objectfilename); 
    global_options.objectfilename = RestoreDumpedProgram ();
  }


  if (global_options.read)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    obj = LinkEmulate (global_options.objectfilename,FALSE);

    RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));

	  
    if ((global_options.disassemble)
        && (!(diabloobject_options.dump || diabloobject_options.dump_multiple)))
    {
      ObjectDisassemble (obj);

      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
        t_cfg *cfg;
        ObjectFlowgraph (obj, NULL, NULL);
        cfg = OBJECT_CFG(obj);

	ppc64_rewrite(obj, cfg);

        ObjectDeflowgraph (obj);

        /* debug code: list final program */
        {
          int i;
          t_section *sec;
          t_ins *ins;
          FILE *f;
          t_string name=StringConcat2(global_options.output_name,".list");

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
          fclose(f);
        }

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);
      }			/*  }}} */

      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
    }
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
    }
  {
    t_symbol * symptr;

    OBJECT_FOREACH_SYMBOL_SAFE(obj, symptr, tmp)
    {
        if (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (symptr)) == RT_SUBSECTION)
        {
            SYMBOL_SET_OFFSET_FROM_START(symptr, AddressAdd(SYMBOL_OFFSET_FROM_START(symptr),
                        AddressSub (SECTION_CADDRESS(T_SECTION(SYMBOL_BASE (symptr))),
                        SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))))));
            SymbolSetBase(symptr, T_RELOCATABLE(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))));
        }

        /* Removing symbols we added in Diablo for internal purposes. */
        if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
                || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
                || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB)
                || strcmp("$compiler", SYMBOL_NAME(sym)) == 0
                || strcmp("$switch", SYMBOL_NAME(sym)) == 0)
            SymbolTableRemoveSymbol (OBJECT_SYMBOL_TABLE(obj), sym);
    }

    printf("+++++++++++++++++++++++++++++++++++++\n");
    if (OBJECT_SYMBOL_TABLE(obj)) SymbolTableFree(OBJECT_SYMBOL_TABLE(obj));
    OBJECT_SET_SYMBOL_TABLE(obj, OBJECT_SUB_SYMBOL_TABLE(obj));
    OBJECT_SET_SUB_SYMBOL_TABLE(obj, NULL);
  }


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
#if 1//def HAVE_DIABLO_EXEC
    DiabloExec ();
#else
    FATAL (("Exec not compiled!"));
#endif
  }
  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  GlobalFini();
  DiabloPpc64OptionsFini ();
  DiabloPpc64Fini();

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
