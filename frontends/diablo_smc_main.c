/* Includes {{{ */
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

#include <diablosmc.h>
#include <diabloanopti386.h>
#include "diablo_options.h"
#include "frontends/diablo_i386_options.h"
/*}}}*/

int
main (int argc, char **argv)
{
  t_object *obj;
  t_cfg *cfg;

  DiabloFlowgraphInit (argc, argv);
  DiabloAnoptInit (argc, argv);
  DiabloAnoptI386Init (argc, argv);

  OptionParseCommandLine (global_list, argc, argv, FALSE);
  OptionGetEnvironment (global_list);
  GlobalVerify ();
  OptionDefaults (global_list);

  OptionParseCommandLine (i386_options_list, argc, argv, FALSE);
  OptionGetEnvironment (i386_options_list);
  OptionDefaults (i386_options_list);
  I386OptionsVerify();

  /* III. The REAL program {{{ */

  if (global_options.read)
  {
    /* A. Get a binary to work with {{{ */
    obj = LinkEmulate (global_options.objectfilename, FALSE);
    /* End Load }}} */

    if (global_options.disassemble)

      if (OBJECT_MAPPED_FIRST(obj))
	ObjectMergeCodeSections (obj);

      ObjectDisassemble(obj);
      /* 2. {{{ Create the flowgraph */
      if (global_options.flowgraph)
      {
	t_uint32 i=0;
	ObjectFlowgraph (obj, NULL, NULL);

	if (diabloflowgraph_options.blockprofilefile)
	{
	  CfgReadBlockExecutionCounts (cfg,
	      diabloflowgraph_options.
	      blockprofilefile);
	  printf ("START WEIGHT %d\n", CfgComputeWeight (cfg));
	}
	
	if(global_options.generate_dots)
	{
	  SmcCreateDots(OBJECT_CFG(obj),"./dots");
	}

	DiabloSmcInitAfterwards(OBJECT_CFG(obj));
	SmcFactor(OBJECT_CFG(obj));

	if(global_options.generate_dots)
	{
	  SmcCreateDots(OBJECT_CFG(obj),"./dots-final");
	}

	ObjectDeflowgraph(obj);

	/* rebuild the layout of the data sections
	 * so that every subsection sits at it's new address */
	ObjectRebuildSectionsFromSubsections (obj);
      }			/*  }}} */

      ObjectAssemble (obj);
      
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

    /*}}}*/
    
    /* IV. Free all structures {{{ */

    GlobalFini();
    I386OptionsFini();
    DiabloSmcCmdlineFini();	

    DiabloSmcFini();
    DiabloFlowgraphFini();
    /*}}}*/
    
#ifdef DEBUG_MALLOC
  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
#endif
  return 0;
}

/* vim: set shiftwidth=2 foldmethod=marker: */
