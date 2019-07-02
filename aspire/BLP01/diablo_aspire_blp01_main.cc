/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

int frontend_id = 1;
#include "aspire_frontend_common.h"

#include <diabloflowgraph_dwarf.h>
#include <diablosoftvm.h>
#include <diabloannotations.h>

t_ptr_array chunks;
void AfterLayoutBroker(t_cfg * cfg)
{
  AspireSoftVMExport(cfg, &chunks);
}

/* MAIN */
int
main (int argc, char **argv)
{
  t_object *obj;

  /* print the command-line arguments */
  PrintFullCommandline(argc, argv);

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

  DiabloSoftVMInit ();
  OptionParseCommandLine (diablosoftvm_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablosoftvm_option_list);
  DiabloSoftVMVerify ();
  OptionDefaults (diablosoftvm_option_list);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  PrintAspireVersionInformationIfRequested();

  if (global_options.random_overrides_file)
    RNGReadOverrides(global_options.random_overrides_file);

  if (!global_options.objectfilename) {
    VERBOSE(0, ("Diablo needs at least an input objectfile/executable as argument to do something."));
    return -1;
  }

  DiabloBrokerCallInstall("FrontendId", "int*", (void*)BrokerFrontendId, FALSE);

  /* The real work: Link Emulate, Dissasemble, Flowgraph, Optimize, Deflowgraph, Assemble or just Dump {{{ */
  if (global_options.read)
  {
    /* The AID is a 128-bits unsigned integer while we only require a 32-bits unsigned integer. Some randomness is thus lost */
    t_uint64 seed = strtoull(aspire_options.actc_id + 16, NULL, 16);
    RNGSetRootGenerator(RNGCreateBySeed(seed, "master"));

    /* for the Aspire project, these optioins should be set to TRUE by default */
    diabloobject_options.read_debug_info = TRUE;

    /* To enable support for the ARM compilers symbols need to be translated. */
    TryParseSymbolTranslation(global_options.objectfilename);

    /* Try emulating the original linker. */
    obj = LinkEmulate (global_options.objectfilename, diabloobject_options.read_debug_info);

    if(diabloobject_options.read_debug_info)
      DwarfFlowgraphInit();

    RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));

    /* Do the whole process ( dissassemble.. optimize.. ) {{{ */
    if (global_options.disassemble)
    {
      NewDiabloPhase("Disassemble");

      ObjectDisassemble (obj);

      if (global_options.flowgraph)
      {
        t_cfg *cfg;
        Annotations annotations;
        t_const_string initial_dot_path = "./diablo-extractor-dots-before";
        t_const_string final_dot_path = "./diablo-extractor-dots-after";

        if (global_options.dots_before_path_set)
          initial_dot_path = global_options.dots_before_path;
        if (global_options.dots_after_path_set)
          final_dot_path = global_options.dots_after_path;

        DiabloBrokerCallInstall("OutputFileName", "t_string *", (void *)OutputFilenameBroker, FALSE);

        NewDiabloPhase("Flowgraph");
        ObjectFlowgraph (obj, NULL, NULL, TRUE);

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

        ASSERT(global_options.annotation_file, ("Please provide an annotation file"));
        RegisterAnnotationInfoFactory(softvm_token, new SoftVMAnnotationInfoFactory());
        ReadAnnotationsFromJSON(global_options.annotation_file, annotations);
        RegionsInit(annotations, cfg);

        /* generate dot file if requested */
        if (global_options.generate_dots)
            CfgDrawFunctionGraphs(cfg, initial_dot_path);

        NewDiabloPhase("SoftVM");

        AspireSoftVMInit(argv[0]);

        CfgComputeLiveness (cfg, TRIVIAL);
        CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
        CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

        /* generate softvm data */
        PtrArrayInit(&chunks, FALSE);

        t_randomnumbergenerator *rng_softvm = RNGCreateChild(RNGGetRootGenerator(), "softvm");
        AspireSoftVMMarkAndSplit(cfg, &chunks, rng_softvm);
        RNGDestroy(rng_softvm);

        /* generate liveness data 
         * necesarry before exporting liveness in json */
        CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgComputeLiveness (cfg, TRIVIAL);
        CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
        CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);
        CfgComputeSavedChangedRegisters (cfg);

        DiabloBrokerCallInstall("AfterDataLayoutFixed", "const t_cfg *", reinterpret_cast<void*>(AfterLayoutBroker), FALSE, cfg);

        NewDiabloPhase("Deflowgraph");
        ObjectDeflowgraph (obj);

        AspireSoftVMFini(&chunks);
        PtrArrayFini(&chunks, FALSE);

        /* rebuild the layout of the data sections
         * so that every subsection sits at it's new address */
        ObjectRebuildSectionsFromSubsections (obj);

        /* clean up region information */
        RegionsFini(cfg);
        AnnotationsDestroy(annotations);

        UnregisterAllAnnotationInfoFactories();
      }

      ObjectAssemble (obj);
    }/* }}} */

  } /* }}} */

  /* Free used libraries and debug print {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */
  GlobalFini ();
  ArmOptionsFini ();
  DiabloAnoptArmFini ();
  AspireOptionsFini();
  DiabloSoftVMFini();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  /*}}} */

  return 0;
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
