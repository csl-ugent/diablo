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
#include <diabloarc.h>
#include <diabloelf.h>
#include "frontends/diablo_arc_options.h"
#include "diablo_options.h"

/* OptimizeArc {{{ */
int
OptimizeArc (t_cfg * cfg)
{
  /* Remove unconnected blocks */
  if (global_options.initcfgopt)
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
    printf ("START WEIGHT %lld\n", CfgComputeWeight (cfg));
    if (diabloflowgraph_options.insprofilefile)
    {
      CfgReadInsExecutionCounts (cfg,
                                 diabloflowgraph_options.insprofilefile);
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



  /* Export dots after optimzation {{{  */
  if (global_options.generate_dots)
  {
    if (diabloflowgraph_options.blockprofilefile)
      CfgComputeHotBblThreshold (cfg, 0.90);
    CfgDrawFunctionGraphsWithHotness (cfg, "./dots-final");
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
/* }}} */

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj;

  DiabloArcInit (argc, argv);

  ArcOptionsInit (argc, argv);
  GlobalInit (argc, argv);

  OptionParseCommandLine (arc_options_list, argc, argv, FALSE);
  OptionGetEnvironment (arc_options_list);

  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);

  GlobalVerify ();
  ArcOptionsVerify ();

  OptionDefaults (global_list);
  OptionDefaults (arc_options_list);

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
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */

      ObjectDisassemble (obj);
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
        t_cfg *cfg;
        ObjectFlowgraph (obj, NULL, NULL);
        cfg = OBJECT_CFG(obj);

        OptimizeArc(cfg);

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

      /*	  if (global_options.saveState)
                  SaveState (obj, SAVESTATEDISASSEMBLED);
                  */
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
    for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
    {
      if (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (symptr)) == RT_SUBSECTION)
      {
        SYMBOL_SET_OFFSET_FROM_START(symptr, AddressAdd(SYMBOL_OFFSET_FROM_START(symptr) ,AddressSub (SECTION_CADDRESS(T_SECTION(SYMBOL_BASE (symptr))),SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))))));
          SymbolSetBase(symptr, T_RELOCATABLE(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))));
      }
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

  GlobalFini ();
  ArcOptionsFini ();
  DiabloArcFini ();

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
