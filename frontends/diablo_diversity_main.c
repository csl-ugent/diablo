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
#include <diabloi386.h>
#include <diabloanopti386.h>
#include <diablodiversity.h>
#if 0
#include "frontends/diablo_i386_options.h"
#include "frontends/diablo_diversity_options.h"
#else
#include "diablo_i386_options.h"
#include "diablo_diversity_options.h"
#endif
#include "diablo_options.h"
#include <diablodiversity_cmdline.h>

int
main (int argc, char **argv)
{
  t_object *obj;
  DiabloAnoptI386Init (argc, argv);

  OptionParseCommandLine (i386_options_list, argc, argv, FALSE);
  OptionGetEnvironment (i386_options_list);
  OptionParseCommandLine (diversity_options_list, argc, argv, FALSE);
  OptionGetEnvironment (diversity_options_list);
  OptionParseCommandLine (diablodiversity_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablodiversity_option_list);
  OptionParseCommandLine (global_list, argc, argv, TRUE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);
  OptionDefaults (i386_options_list);
  OptionDefaults (diversity_options_list);
  OptionDefaults (diablodiversity_option_list);

  /* III. The REAL program {{{ */

  if (global_options.read)
  {
    ASSERT(global_options.objectfilename, ("Input objectfile/executable name not set!"));
    obj = LinkEmulate (global_options.objectfilename,FALSE);

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
	CfgEdgeInitTestCondition(cfg);
	I386StucknessAnalysis (obj);

	/* Remove unconnected blocks {{{*/
	if (global_options.initcfgopt)
	{
	  STATUS (START, ("Removing unconnected blocks"));
	  CfgRemoveDeadCodeAndDataBlocks (cfg);
	  STATUS (STOP, ("Removing unconnected blocks"));
	}
	/*}}}*/

	CfgPatchToSingleEntryFunctions (cfg);
	CfgRemoveDeadCodeAndDataBlocks (cfg);
#if 1
	if (diabloflowgraph_options.blockprofilefile)
	{
	  VERBOSE(0,("BEFORE"));
	  CfgReadBlockExecutionCounts (cfg,
	      diabloflowgraph_options.
	      blockprofilefile);
	  printf ("START WEIGHT %"PRId64"\n", CfgComputeWeight (cfg));
	  VERBOSE(0,("AFTER"));
	  if (diabloflowgraph_options.insprofilefile)
	  {
	    CfgReadInsExecutionCounts (cfg,
		diabloflowgraph_options.
		insprofilefile);
	  //  CfgComputeHotBblThreshold (cfg, 0.90);
	  }
	  CfgComputeHotBblThreshold (cfg, 0.90);
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

	  CfgDrawFunctionGraphsWithHotness (OBJECT_CFG(obj), "./dots");

	  CgBuild (cfg);
	  CgExport (CFG_CG(cfg), "./dots/callgraph.dot");
	}
	/* }}} */

	/*keep track of all old addresses (not just one)*/
	InsInitAddressList(cfg);

	InsInitScore(cfg);

	/*keep track of factored functions and bbls, so that we don't inline them again*/
	BblInitFactored(cfg);
	FunctionInitFactored(cfg);

  /* Keep track of applied transformations */
  BblInitTransformations(cfg);
  FunctionInitTransformations(cfg);
  FunctionInitPossibleTransformations(cfg);

	diabloanopt_options.rely_on_calling_conventions = FALSE;
	DiversityEngine(cfg);

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
#endif

	ObjectDeflowgraph (obj);

	/* rebuild the layout of the data sections
	 * so that every subsection sits at it's new address */
	ObjectRebuildSectionsFromSubsections (obj);
      }			/*  }}} */

      if(!diablodiversity_options.div_smc_factor1 && !diablodiversity_options.div_smc_factor4)
      {
	t_string name = StringConcat2 (global_options.output_name, ".list");
	FILE *f = fopen (name, "w");
	if (f)
	{
	  t_ins *ins;
	  SECTION_FOREACH_INS (OBJECT_CODE (obj)[0], ins)
	    FileIo (f, "@I\n", ins);
	  fclose (f);
	}
	Free (name);

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
		FileIo (f, "<ins address=\"@G\" >", INS_CADDRESS(ins));

          //FileIo(f, "orig_function_first=\"@G\" guessed_train_exec_count=\"%lli\" >", INS_ORIGINAL_ADDRESS(ins), INS_EXECCOUNT(ins));

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

      }

      ObjectAssemble (obj);
      /* End Transform and optimize }}} */
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

  /* IV. Free all structures {{{ */
  /* remove directory that held the restored dump */
  /*if (global_options.restore) RmdirR("./DUMP"); */

  GlobalFini();
  I386OptionsFini();
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
