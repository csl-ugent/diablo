/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <random>
#include <string>

extern "C" {
#include <diabloanopt.h>
// #include <diablodiversity_cmdline.h>
#include "diablo_options.h"
#include "obfuscation_opt.h"

#include "diablodiversity_engine.h"
#include "diversity_engine/diablodiversity_cmdline.h"
}

#include <vector>
#include <iostream>

#include "obfuscation_architecture_backend.h"
#include "obfuscation_transformation.h"

#include <diabloflowgraph_dwarf.h>



using namespace std;

void ObjectDiversify(t_string filename, t_object* obj) {
  t_cfg* cfg = OBJECT_CFG(obj);
  t_bbl* bbl;
  t_bbl* bbl_safe;

  t_function* fun;
  t_function* fun_safe;
  
  /* Stats */
  int tot_functions = 0;
  int tot_bbls = 0;
  int tot_insts = 0;
  int couldTransform = 0;
  
  string obfuscation_class;
  bool obfuscation_class_set = false;

  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_safe) {
    if(FUNCTION_IS_HELL(fun)) {
      continue;
    }
    tot_functions++;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      tot_bbls++;
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        tot_insts++;
      }
    }
  }
  
  string prefix = filename;
  prefix = prefix + ",";
  
  VERBOSE(0, ("%s,All_Stats,functions_transformed,%i", filename, tot_functions));
  VERBOSE(0, ("%s,All_Stats,bbls_transformed,%i", filename, tot_bbls));
  VERBOSE(0, ("%s,All_Stats,insts_in_bbls,%i", filename, tot_insts));
 
  vector<t_bbl*> bbls;  
  vector<t_function*> funs;
  uniform_int_distribution<uint32_t> uniform_selector(0,99);

  /* So we don't double-split BBLs. Right now, don't transform created code, this will change in the future (TODO) */
  bbls.clear();
  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_safe) {
    funs.push_back(fun);
    
    t_bbl* bbl;    
    FUNCTION_FOREACH_BBL(fun, bbl) {
      bbls.push_back(bbl);
    }
  }

  //CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_before");

  /* TODO ensure the liveness is correct through transformations... */
  /* TODO: audit all transformations, check that all BBLs they introduce contain correct liveness, REGS_DEFINED_IN, etc */

  CfgComputeLiveness (cfg, TRIVIAL);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

  if (ConstantPropagationInit(cfg))
    ConstantPropagation(cfg, CONTEXT_SENSITIVE);
  else
    FATAL(("ERR\n"));
    /* TODO: this is also needed, but why isn't the above enough? */

  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(fun))) {
      FunctionUnmarkAllBbls(fun);
      FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);
    }
  }



  auto bbl_obfuscators = GetTransformationsForType("bbl_obfuscation");

  if (bbl_obfuscators.size() > 0) {
    for (auto obfuscator: bbl_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No bbl_obfuscation found, skipping"));
  }
  
  auto function_obfuscators = GetTransformationsForType("function_obfuscation");

  if (function_obfuscators.size() > 0) {
    for (auto obfuscator: function_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No function_obfuscation found, skipping"));
  }

  SetAllObfuscationsEnabled(true);
  DiversityEngine(cfg);
}

int
main (int argc, char **argv)
{
  t_object *obj;
  void *obf_arch;

  SetUseDiversityTransforms(true);

  InitArchitecture(&obf_arch);
  
  ObfuscationArchitectureInitializer* obfuscationArchitecture = GetObfuscationArchitectureInitializer();
  
  /* Process command line parameters for each of the used libraries */
  obfuscationArchitecture->Init(argc, argv);
  AddOptionsListInitializer(obfuscation_obfuscation_option_list); ObfuscationOptInit();
  AddOptionsListInitializer(diablodiversity_option_list); DiabloDiversityCmdlineInit();

  ParseRegisteredOptionLists(argc, argv);

  /* The final option parsing should have TRUE as its last argument */
  GlobalInit ();
  OptionParseCommandLine (global_list, argc, argv, TRUE /* final */);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  PrintVersionInformationIfRequested();

  /* III. The REAL program {{{ */
  if (global_options.random_overrides_file)
    RNGReadOverrides(global_options.random_overrides_file);
  
  if (!global_options.objectfilename) {
    VERBOSE(0, ("Diablo needs at least an input objectfile/executable as argument to do something."));
    return -1;
  }

  if (global_options.read)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    TryParseSymbolTranslation(global_options.objectfilename);

    obj = LinkEmulate (global_options.objectfilename,FALSE);

    if(diabloobject_options.read_debug_info)
      DwarfFlowgraphInit();
        
        //RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));

    if (global_options.disassemble)
    {
      /* B. Transform and optimize  {{{ */
      /* 1. Disassemble */

      ObjectDisassemble (obj);
      
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
                t_cfg *cfg;
                ObjectFlowgraph (obj, NULL, NULL, FALSE);
                
                cfg = OBJECT_CFG(obj);

                DiabloBrokerCall("StucknessAnalysis", obj);

                CfgRemoveDeadCodeAndDataBlocks (cfg);
                CfgPatchToSingleEntryFunctions (cfg);
                CfgRemoveDeadCodeAndDataBlocks (cfg);
                
                if (diabloflowgraph_options.blockprofilefile)
                {
                  CfgReadBlockExecutionCounts (cfg,
                                                                           diabloflowgraph_options.blockprofilefile);
                  printf ("START WEIGHT %" PRId64 "\n", CfgComputeWeight (cfg));
                  if (diabloflowgraph_options.insprofilefile)
                  {
                        CfgReadInsExecutionCounts (cfg,
                                                                           diabloflowgraph_options.insprofilefile);
                        CfgComputeHotBblThreshold (cfg, 0.90);
                  }
                }

                // MakeConstProducers (cfg);
                
                  /* Keep track of applied transformations */
                  BblInitTransformations(cfg);
                  FunctionInitTransformations(cfg);
                  FunctionInitPossibleTransformations(cfg);
                  InsInitAddressList(cfg);


                ObjectDiversify(global_options.objectfilename, obj);

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

                ObjectDeflowgraph (obj);

                if (global_options.print_listing)
                  ObjectPrintListing (obj, global_options.output_name);

                /* rebuild the layout of the data sections
                * so that every subsection sits at it's new address */
                ObjectRebuildSectionsFromSubsections (obj);
      }
      /*  }}} */

#if 1
        {
          FILE *f = fopen ("mapping.xml", "w");
          FileIo(f,"<mapping>");
          if (f)
          {
            t_ins *ins;
            SECTION_FOREACH_INS (OBJECT_CODE (obj)[0], ins)
            {
        t_address_item * item = INS_ADDRESSLIST(ins)->first;
                FileIo (f, "<ins address=\"@G\" ", INS_CADDRESS(ins));

          FileIo(f, "orig_function_first=\"@G\" guessed_train_exec_count=\"%lli\" >", INS_ORIGINAL_ADDRESS(ins), INS_EXECCOUNT(ins));

        while(item) {
                      FileIo (f, "@G ", item->address);
                      item = item->next;
                    }

              FileIo (f, "</ins>\n");
            }
            FileIo(f,"</mapping>");
            fclose (f);
          }
        }
#endif

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
        S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
        S_IXOTH);
#endif

  }
  /* END REAL program }}} */

  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  DestroyArchitecture(&obf_arch);
  GlobalFini();
  DiabloFlowgraphFini ();

  /* If DEBUG MALLOC is defined print a list of all memory
   * leaks. Needs to increase the verbose-level to make
   * sure these get printed. This is a hack but it may be
   * removed when a real version of diablo is made */

#ifdef DEBUG_MALLOC
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
#endif

  //scanf("%s", NULL);

  /* End Fini }}} */
  return 0;
}
/* vim: set shiftwidth=2 foldmethod=marker:*/
