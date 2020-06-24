/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "aspire_frontend_common.h"

#include <obfuscation_opt.h>
#include <obfuscation/obfuscation_architecture_backend.h>
#include <obfuscation/obfuscation_transformation.h>
#include <obfuscation/obfuscation_json.h>
#include <flatten_function.h>
#include <opaque_predicate.h>

#include <attestation.h>
#include <callstack_checks.h>
#include <diablosoftvm.h>
#include <code_mobility.h>
#include <reaction_mechanisms.h>
#ifdef SELF_DEBUGGING
#include <self_debugging.h>
#else
#include <self_debugging_cmdline.h>
#include <self_debugging_json.h>
#endif

using namespace std;

int frontend_id = 4;

/* Some combinations of protections on the same BBLs are disabled for now. This function ensures that */
/* TODO: once each protection annotation gets its own region, this should be rewritten and be made more generic to only remove the conflicting BBLs from
 * the offending region (which will be easy then)! */
void DisableMisbehavingProtectionCombinations(t_cfg* cfg) {
  t_bbl* bbl;

  CFG_FOREACH_BBL(cfg, bbl) {
    Region *region;

    bool conflict_bf = false;
  /* Disable branch_function + { code_mobility, anti_debugging }
     call checks and code mobility are disabled in the call check's InsertCheckInFunction */
    BBL_FOREACH_REGION(bbl, region) {
      for (auto request: region->requests) {
        if (dynamic_cast<CodeMobilityAnnotationInfo*>(request) || dynamic_cast<SelfDebuggingAnnotationInfo*>(request)) {
          if (region->annotation->annotation_content)
            VERBOSE(1, ("Conflict resolution: this BBL cannot have branch functions (annotation: '%s')", region->annotation->annotation_content->c_str()));
          else
            VERBOSE(1, ("Conflict resolution: this BBL cannot have branch functions (annotation: null)"));
          conflict_bf = true;
          break;
        }
      }
    }

    BBL_FOREACH_REGION(bbl, region) {
      if (conflict_bf) {
        for (auto request: region->requests) {
          if (   request->name.c_str() == string("branch_function")
              || request->name.c_str() == string("opaque_predicate")
              || request->name.c_str() == string("flatten_function")) {
            auto bf_request = dynamic_cast<ObfuscationAnnotationInfo*>(request);
            ASSERT(bf_request, ("Annotation claimed it was a branch function or flatten function, but could not cast to ObfuscationAnnotationInfo*!"));

            if (region->annotation->annotation_content)
              VERBOSE(0, ("Conflict resolution: disabled branch/flatten function for a region that contained both branch functions and code mobility/self-debugging (annotation: '%s')",
                          region->annotation->annotation_content->c_str()));
            else
              VERBOSE(0, ("Conflict resolution: disabled branch/flatten function for a region that contained both branch functions and code mobility/self-debugging (annotation: null)"));

            bf_request->enable = false;
          }
        }
      }
    }

    /* Disable {RA,Code Guards} + Code Mobility */
    const CodeMobilityAnnotationInfo* info;
    bool needs_disabled_guards = false;
    BBL_FOREACH_CODEMOBILITY_REGION(bbl, region, info)
    {
      needs_disabled_guards = true;
    }

    if (needs_disabled_guards) {
      BBL_FOREACH_REGION(bbl, region) {
        for (auto it = region->requests.begin(); it != region->requests.end(); ) {
          if (dynamic_cast<CodeGuardAnnotationInfo*>(*it) || dynamic_cast<AttestationAnnotationInfo*>(*it)) {
            VERBOSE(0, ("BBL is used in both Code Mobility and RA/Code Guards, disabling RA/guards for this region!"));
            it = region->requests.erase(it);
          } else {
            ++it;
          }
        }
      }
    }
  }
}

t_ptr_array chunks;
void AfterLayoutBroker(t_cfg * cfg)
{
  AspireSoftVMFixups(cfg, &chunks);
  AspireSoftVMPreFini();
}

LogFile* L_SOFTVM = NULL;

int
main (int argc, char **argv)
{
  t_object *obj;
  t_ptr_array unordered_chunks;

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
  OptionParseCommandLine (aspire_options_list, argc, argv, FALSE);
  OptionGetEnvironment (aspire_options_list);
  AspireOptionsVerify ();
  OptionDefaults (aspire_options_list);

  CodeMobilityInit();
  OptionParseCommandLine (code_mobility_option_list, argc, argv, FALSE);
  OptionGetEnvironment (code_mobility_option_list);
  CodeMobilityVerify ();
  OptionDefaults (code_mobility_option_list);

  DiabloSoftVMInit ();
  OptionParseCommandLine (diablosoftvm_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablosoftvm_option_list);
  DiabloSoftVMVerify ();
  OptionDefaults (diablosoftvm_option_list);

  SelfDebuggingInit ();
  OptionParseCommandLine (self_debugging_option_list, argc, argv, FALSE);
  OptionGetEnvironment (self_debugging_option_list);
  SelfDebuggingVerify ();
  OptionDefaults (self_debugging_option_list);

  /* Do the options for the obfuscation backend */
  void *obf_arch_obj = NULL;
  InitArchitecture(&obf_arch_obj);
  ObfuscationArchitectureInitializer* obfuscationArchitecture = GetObfuscationArchitectureInitializer();
  obfuscationArchitecture->Init(argc, argv);
  AddOptionsListInitializer(obfuscation_obfuscation_option_list);
  ObfuscationOptInit();

  ParseRegisteredOptionLists(argc, argv);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE /* final */);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  /* III. The REAL program {{{ */
  PrintAspireVersionInformationIfRequested();

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

  if (global_options.read)
  {
    /* The AID is a 128-bits unsigned integer while we only require a 32-bits unsigned integer. Some randomness is thus lost */
    t_uint64 seed = strtoull(aspire_options.actc_id + 16, NULL, 16);
    RNGSetRootGenerator(RNGCreateBySeed(seed, "master"));

    /* for the Aspire project, these optioins should be set to TRUE by default */
    diabloobject_options.read_debug_info = TRUE;

    TryParseSymbolTranslation(global_options.objectfilename);

    obj = LinkEmulate (global_options.objectfilename, diabloobject_options.read_debug_info);

    if(diabloobject_options.read_debug_info)
      DwarfFlowgraphInit();

    /* Read in the annotations from the JSON file */
    Annotations annotations;
    ASSERT(global_options.annotation_file, ("Please provide an annotation file"));
    RegisterAnnotationInfoFactory(softvm_token, new SoftVMAnnotationInfoFactory());
    RegisterAnnotationInfoFactory(obfuscations_token, new ObfuscationAnnotationInfoFactory());
    RegisterAnnotationInfoFactory(codemobility_token, new CodeMobilityAnnotationInfoFactory());
    RegisterAnnotationInfoFactory(selfdebugging_token, new SelfDebuggingAnnotationInfoFactory());
    RegisterAnnotationInfoFactory(callcheck_token, new CallStackCheckAnnotationInfoFactory());
    /* Only parse attestation annotations of a certain type when their Diablo option is enabled. Otherwise,
     * parsing the annotations will add all these regions to the area's to be attested (without their code
     * or ADSes actually being linked in) */
    if (aspire_options.remote_attestation)
      RegisterAnnotationInfoFactory(remoteattestation_token, new RemoteAttestationAnnotationInfoFactory());
    if (aspire_options.code_guards)
    {
      RegisterAnnotationInfoFactory(codeguard_token, new CodeGuardAnnotationInfoFactory());
      RegisterAnnotationInfoFactory(attestator_token, new AttestatorAnnotationInfoFactory());
    }
    ReadAnnotationsFromJSON(global_options.annotation_file, annotations);

    /* CF Tagging might be requested, but it's not present in this version of Diablo */
    if (aspire_options.control_flow_tagging)
      WARNING(("CF Tagging was requested but is not present in this version of Diablo!"));

    /* Find out whether code mobility is needed and initialize it if necessary */
    unique_ptr<CodeMobilityTransformer> cm_transformer;
    aspire_options.code_mobility = aspire_options.code_mobility && AnnotationContainsToken(annotations, codemobility_token);

    if (aspire_options.code_mobility)
      cm_transformer.reset(new CodeMobilityTransformer(obj, global_options.output_name));

    if (global_options.self_profiling)
      SelfProfilingInit(obj, global_options.self_profiling);

    /* Find out whether self-debugging is needed and initialize it if necessary */
#ifdef SELF_DEBUGGING
    unique_ptr<SelfDebuggingTransformer> sd_transformer;
#endif
    aspire_options.self_debugging = aspire_options.self_debugging && AnnotationContainsToken(annotations, selfdebugging_token);
	
    if (aspire_options.self_debugging)
#ifdef SELF_DEBUGGING
      sd_transformer.reset(new SelfDebuggingTransformer(obj, global_options.output_name));
#else
      WARNING(("Self-debugging was requested but is not present in this version of Diablo!"));
#endif

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

        /* first, try to find the vmExecute function */
        if (SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "vmExecute"))
        {
          reachable_vector.push_back("vmExecute");

          int stub_id = 0;
          while (true)
          {
            string stub_name = "vmStart" + to_string(stub_id);

            if (SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), stub_name.c_str()))
            {
              reachable_vector.push_back(stub_name);

              string image_name = "vmImage" + to_string(stub_id);
              t_symbol *image_symbol = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), image_name.c_str());
              ASSERT(image_symbol, ("expected to find %s symbol, but could not find it!", image_name.c_str()));

              t_section *image_section = T_SECTION(SYMBOL_BASE(image_symbol));
              SECTION_SET_FLAGS(image_section, SECTION_FLAGS(image_section) | SECTION_FLAG_KEEP);

              stub_id++;
            }
            else
            {
              break;
            }
          }

          /* -1 because vmExecute is included in the list */
          VERBOSE(0, ("made %d stubs force-reachable", reachable_vector.size()-1));
        }

        if (aspire_options.code_mobility)
          cm_transformer->AddForceReachables(reachable_vector);

#ifdef SELF_DEBUGGING
        if (aspire_options.self_debugging)
          sd_transformer->AddForceReachables(reachable_vector);
#endif

        AddReactionForceReachables(obj, reachable_vector);

        t_const_string const *force_reachable_funs = stringVectorToConstStringArray(reachable_vector);

        NewDiabloPhase("Flowgraph");
        ObjectFlowgraph (obj, NULL, force_reachable_funs, TRUE);
        delete[] force_reachable_funs;

        cfg = OBJECT_CFG(obj);

        CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgPatchToSingleEntryFunctions (cfg);
        CfgRemoveDeadCodeAndDataBlocks (cfg);

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

        CfgComputeLiveness (cfg, TRIVIAL);
        CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
        CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

        while (ArmKillUselessInstructions (cfg));

        RegionsInit(annotations, cfg);

        DisableMisbehavingProtectionCombinations(cfg);

        if ((aspire_options.remote_attestation && AnnotationContainsToken(annotations, remoteattestation_token))
            || (aspire_options.code_guards && AnnotationContainsToken(annotations, codeguard_token)))
        {
          NewDiabloPhase("Attestation");
          AttestationInit(obj, aspire_options.actc_id, global_options.output_name);
        }

        bool vm_present = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "vmExecute") != NULL;
        if (aspire_options.softvm)
        {
          DiabloBrokerCallInstall("CfgEdgeKill", "t_cfg_edge *", (void *)CfgEdgeKillSoftVM, FALSE);
          DiabloBrokerCallInstall("BblKill", "t_bbl *", (void *)BblKillSoftVM, FALSE);

          /* generate dot file if requested */
          if (global_options.generate_dots)
            CfgDrawFunctionGraphs(cfg, initial_dot_path);

          int nr_mobile_softvm_chunks = 0;
          if (vm_present)
          {
            t_const_string logging_svm_filename = StringConcat2 (global_options.output_name, ".diablo.integration.log");
            INIT_LOGGING(L_SOFTVM, logging_svm_filename);
            Free(logging_svm_filename);

            LOG(L_SOFTVM, ("START OF INTEGRATION LOG\n"));

            NewDiabloPhase("SoftVM");

            AspireSoftVMInit();

            /* generate softvm data */
            PtrArrayInit(&unordered_chunks, FALSE);

            t_randomnumbergenerator *rng_softvm = RNGCreateChild(RNGGetRootGenerator(), "softvm");
            nr_mobile_softvm_chunks = AspireSoftVMMarkAndSplit(cfg, &unordered_chunks, rng_softvm);
            RNGDestroy(rng_softvm);

            /* fix the order of the chunks so the order in which they are written in the
             * extractor output file is equal to the order in which they are stored in the Diablo IR. */
            PtrArrayInit(&chunks, FALSE);
            AspireSoftVMReadExtractorOutput(cfg, &unordered_chunks, &chunks);
            PtrArrayFini(&unordered_chunks, FALSE);

            /* Some unreachable parts of code (all vmStartX pieces and the vmExecute
             * function) are not flowgraphed in functions. We do need them to be.
             *      - vmStartX will be taken care of while replacing the chunks
             *      - vmExecute we'll do here */
            AspireSoftVMInsertGlueJumps(cfg, &chunks);

            CfgRemoveDeadCodeAndDataBlocks (cfg);
            CfgPatchToSingleEntryFunctions (cfg);
            CfgRemoveDeadCodeAndDataBlocks (cfg);

            CfgComputeLiveness (cfg, TRIVIAL);
            CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
            CfgComputeSavedChangedRegisters (cfg);

            DiabloBrokerCallInstall("AfterDataLayoutFixed", "const t_cfg *", reinterpret_cast<void*>(AfterLayoutBroker), FALSE, cfg);

            LOG(L_SOFTVM,("END OF INTEGRATION LOG\n"));
            FINI_LOGGING(L_SOFTVM);

            if (aspire_options.code_mobility)
              cm_transformer->ReserveEntries(nr_mobile_softvm_chunks);
          }
          else
          {
            Region *region;
            const SoftVMAnnotationInfo *info;
            CFG_FOREACH_SOFTVM_REGION(cfg, region, info)
              FATAL(("SoftVM protection requested but no SoftVM present in binary"));
          }
        }

        /* TODO: can't we perhaps put these checks in the SoftVM? :-) */
        /* TODO: enable/disable with commandline args */

        if (!aspire_options.softvm_only)
        {
          if (aspire_options.call_stack_checks)
          {
            t_const_string logging_callchecks_filename = StringConcat2 (global_options.output_name, ".diablo.callchecks.log");
            INIT_LOGGING(L_CALLCHECKS, logging_callchecks_filename);
            Free(logging_callchecks_filename);
            LOG(L_CALLCHECKS, ("START OF CALLCHECKS LOG\n"));

            NewDiabloPhase("CallChecks");
            ApplyCallStackChecks(cfg);

            LOG(L_CALLCHECKS,("END OF CALLCHECKS LOG\n"));
            FINI_LOGGING(L_CALLCHECKS);
          }

          if (global_options.factoring)
          {
            t_randomnumbergenerator *rng_factoring = RNGCreateChild(RNGGetRootGenerator(), "factoring");

            NewDiabloPhase("Factoring");
            if (BblFactorInit(cfg))
            {
              FunctionEpilogueFactoring (cfg);

              CfgPatchToSingleEntryFunctions (cfg);
              BblFactoring (cfg, rng_factoring);
            }
            BblFactorFini(cfg);

            RNGDestroy(rng_factoring);
          }

          AddReactions(obj);

          if (aspire_options.obfuscations)
          {
            LogFile* L_OBF = NULL;
            t_const_string logging_obf_filename = StringConcat2 (global_options.output_name, ".diablo.obfuscation.log");
            INIT_LOGGING(L_OBF, logging_obf_filename);
            Free(logging_obf_filename);
            ATTACH_LOGGING(L_OBF_BF, L_OBF);
            ATTACH_LOGGING(L_OBF_FF, L_OBF);
            ATTACH_LOGGING(L_OBF_OOP, L_OBF);

            NewDiabloPhase("Obfuscation");
            if (global_options.annotation_file == NULL)
              ObjectObfuscate(global_options.objectfilename, obj);
            else
              CfgObfuscateRegions(cfg);

            CfgPatchToSingleEntryFunctions (cfg);
            LOG(L_OBF,("END OF OBFUSCATION LOG\n"));
            FINI_LOGGING(L_OBF);
            FINI_LOGGING(L_OBF_BF);
            FINI_LOGGING(L_OBF_FF);
            FINI_LOGGING(L_OBF_OOP);
          }

#if SELF_DEBUGGING
          if (aspire_options.self_debugging)
          {
            NewDiabloPhase("Self-Debugging");
            sd_transformer->TransformObject();
          }
#endif

          if (aspire_options.code_mobility)
          {
            NewDiabloPhase("Code Mobility");
            cm_transformer->TransformObject();
          }
        }

        if (global_options.self_profiling)
        {
          /* open up a file for the stdout of the forked process */
          stdout_file = StringConcat2(global_options.output_name, ".self_profiling.stdout");
          fd_fork_stdout = open(stdout_file, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR | S_IROTH | S_IRGRP);
          ASSERT(fd_fork_stdout != -1, ("Could not open file '%s' to redirect the stdout of the forked process to.", stdout_file));

          /* flush output streams before forking */
          fflush(stdout);
          fflush(stderr);

          if (aspire_options.no_sp_fork)
          {
            VERBOSE(0, ("As requested, no fork is executed for the insertion of self-profiling support code."));
            VERBOSE(0, ("The remaining output is redirected to \"%s\".", stdout_file));
            child_process_id = 1;
            Free(stdout_file);
          }
          else
          {
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

            if (aspire_options.code_mobility)
            {
              t_string s = StringConcat2(code_mobility_options.output_dir, ".self_profiling");
              Free(code_mobility_options.output_dir);
              code_mobility_options.output_dir = s;
            }

            global_options.output_name = StringConcat2(global_options.output_name, ".self_profiling");
            final_dot_path = StringConcat2(final_dot_path, "-self_profiling");

            /* Add self-profiling */
            CfgAssignUniqueOldAddresses(cfg);
            NewDiabloPhase("Self-profiling");
            CfgComputeLiveness (cfg, TRIVIAL);/* We'll need this later on */
            CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);/* We'll need this later on */
            CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);/* We'll need this later on */
            CfgAddSelfProfiling (obj, global_options.output_name);
          }
          else
          {
            /* child process */

            /* do nothing */
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

        if (vm_present && aspire_options.softvm)
        {
          AspireSoftVMFini(&chunks);
          PtrArrayFini(&chunks, FALSE);
        }

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);

        if (!aspire_options.softvm_only)
        {
          if (aspire_options.code_mobility)
            cm_transformer->FinalizeTransform();
#if SELF_DEBUGGING
          if (aspire_options.self_debugging)
            sd_transformer->FinalizeTransform();
#endif
        }

        t_const_string consumed_annotations_file = StringConcat2 (global_options.output_name, ".diablo.consumedannotations.json");

        DumpConsumedAnnotations(consumed_annotations_file);
        Free(consumed_annotations_file );

        /* clean up region information */
        RegionsFini(cfg);
      }
      /*  }}} */

      if (global_options.print_listing)
        ObjectPrintListing (obj, global_options.output_name);

      ObjectAssemble (obj);

      if (!aspire_options.softvm_only)
      {
        if (aspire_options.code_mobility)
          cm_transformer->Output();
      }
      /* End Transform and optimize }}} */
    }

    /* Aspire cleanup */
    AnnotationsDestroy(annotations);
    UnregisterAllAnnotationInfoFactories();
    RNGDestroy(RNGGetRootGenerator());

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

  } /* }}} */
  /* END REAL program }}} */

  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */
  GlobalFini ();
  ArmOptionsFini ();
  DiabloAnoptArmFini ();
  AspireOptionsFini();
  DiabloSoftVMFini();
  ObfuscationOptFini();
  CodeMobilityFini();
  SelfDebuggingFini();

  DestroyArchitecture(&obf_arch_obj);

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
