/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "aspire_frontend_common.h"

using namespace std;

int frontend_id = 0;

int
main (int argc, char **argv)
{
  t_object *obj;

  pid_t child_process_id = 0;
  int fd_fork_stdout;

  /* Initialise used Diablo libraries */
  DiabloAnoptArmInit (argc, argv);

  /* Process command line parameters for each of the used libraries */
  ArmOptionsInit ();
  OptionParseCommandLine (arm_options_list, argc, argv, FALSE);
  OptionGetEnvironment (arm_options_list);
  ArmOptionsVerify ();
  OptionDefaults (arm_options_list);

  AspireOptionsInit ();
  OptionParseCommandLine (aspire_options_list, argc, argv, FALSE /* final */);
  OptionGetEnvironment (aspire_options_list);
  AspireOptionsVerify ();
  OptionDefaults (aspire_options_list);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  /* III. The REAL program {{{ */
  PrintAspireVersionInformationIfRequested();

  if (!global_options.objectfilename) {
    VERBOSE(0, ("Diablo needs at least an input objectfile/executable as argument to do something."));
    return -1;
  }

  if (global_options.read)
  {
    /* for the Aspire project, these optioins should be set to TRUE by default */
    diabloobject_options.read_debug_info = TRUE;

    TryParseSymbolTranslation(global_options.objectfilename);

    obj = LinkEmulate (global_options.objectfilename, diabloobject_options.read_debug_info);

    if(diabloobject_options.read_debug_info)
      DwarfFlowgraphInit();

    Annotations annotations;
    if (global_options.annotation_file)
        ReadAnnotationsFromJSON(global_options.annotation_file, annotations);

    if (global_options.self_profiling)
      SelfProfilingInit(obj, global_options.self_profiling);

    RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));

    if (global_options.disassemble)
    {
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */
      NewDiabloPhase("Disassemble");

      ObjectDisassemble (obj);
      
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
        t_cfg *cfg;
        t_const_string stdout_file, static_compl_file, dynamic_compl_file, static_regions_compl_file, dynamic_regions_compl_file;

        t_const_string initial_dot_path = "./diablo-selfprofiler-dots-before";
        t_const_string final_dot_path = "./diablo-selfprofiler-dots-after";

        vector<string> reachable_vector;

        if (global_options.dots_before_path_set)
          initial_dot_path = global_options.dots_before_path;
        if (global_options.dots_after_path_set)
          final_dot_path = global_options.dots_after_path;

        if (global_options.self_profiling)
        {
          reachable_vector.push_back(FINAL_PREFIX_FOR_LINKED_IN_PROFILING_OBJECT "print");
          reachable_vector.push_back(FINAL_PREFIX_FOR_LINKED_IN_PROFILING_OBJECT "Init");
        }

        t_const_string const *force_reachable_funs = stringVectorToConstStringArray(reachable_vector);

        NewDiabloPhase("Flowgraph");
        ObjectFlowgraph (obj, NULL, force_reachable_funs, TRUE);
        delete[] force_reachable_funs;
    
        cfg = OBJECT_CFG(obj);

        CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgPatchToSingleEntryFunctions (cfg);
        CfgRemoveDeadCodeAndDataBlocks (cfg);

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

        CfgComputeLiveness (cfg, TRIVIAL);
        CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
        CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

        while (ArmKillUselessInstructions (cfg));

        RegionsInit(annotations, cfg);

        /* generate dot file if requested */
        if (global_options.generate_dots)
            CfgDrawFunctionGraphs(cfg, initial_dot_path);

        if (aspire_options.no_sp_fork)
          child_process_id = 1;
        else
        {
          /* open up a file for the stdout of the forked process */
          stdout_file = StringConcat2(global_options.output_name, ".self_profiling.stdout");
          fd_fork_stdout = open(stdout_file, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IROTH | S_IRGRP);
          ASSERT(fd_fork_stdout != -1, ("Could not open file '%s' to redirect the stdout of the forked process to.", stdout_file));
          Free(stdout_file);

          /* flush output streams before forking */
          fflush(stdout);
          fflush(stderr);

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

          if (!aspire_options.no_sp_fork)
          {
            /* redirect stdout */
            dup2(fd_fork_stdout, STDOUT_FILENO);

            global_options.output_name = StringConcat2(global_options.output_name, ".self_profiling");
            final_dot_path = StringConcat2(final_dot_path, "-self_profiling");
          }

          /* Add self-profiling */
          if (global_options.self_profiling)
          {
            CfgAssignUniqueOldAddresses(cfg);
            NewDiabloPhase("Self-profiling");
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);/* We'll need this later on */
            CfgAddSelfProfiling (obj, global_options.output_name);
          }
        }
        else
        {
          /* child process */

          /* do nothing */
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
        }/*}}}*/

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

        LogRegionsStaticComplexity(static_regions_compl_file, cfg);
        LogRegionsDynamicComplexity(dynamic_regions_compl_file, cfg);
        Free(static_regions_compl_file);
        Free(dynamic_regions_compl_file);

        CfgStaticComplexityFini();
        CfgDynamicComplexityFini();

        NewDiabloPhase("Deflowgraph");
        ObjectDeflowgraph (obj);

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);
      }
      /*  }}} */

      if (global_options.print_listing)
        ObjectPrintListing (obj, global_options.output_name);

      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
    }
    
    /* Clean and write out the object {{{ */
    /* remove local definitions of BuildAttributes$$*: the global one from the final
     * linked binary is the combination of all of those */
    t_const_string strip_symbol_masks[3] = {"BuildAttributes$$*", "$switch", NULL};
    ObjectConstructFinalSymbolTable(obj, strip_symbol_masks);

    ObjectWrite (obj, global_options.output_name);

#ifdef DIABLOSUPPORT_HAVE_STAT
    /* make the file executable */
    chmod (global_options.output_name,
            S_IRUSR | S_IWUSR | S_IXUSR
            | S_IRGRP | S_IXGRP
            | S_IROTH | S_IXOTH);
#endif

  }/*}}}*/
  /* END REAL program }}} */

  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */
  GlobalFini ();
  ArmOptionsFini ();
  DiabloAnoptArmFini ();
  AspireOptionsFini();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();

  /* End Fini }}} */
  return 0;
}
/* vim: set shiftwidth=2 foldmethod=marker:*/
