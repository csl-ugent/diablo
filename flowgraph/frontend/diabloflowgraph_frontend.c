/* An example frontend for diabloflowgraph 
 *
 * You can reuse this code if you want to write your own frontends. However,
 * you cannot use DIABLOFLOWGRAPH_INTERNAL, DIABLOFLOWGRAPH_ARMSUPPORT,
 * DIABLOFLOWGRAPH_I386SUPPORT, DIABLOFLOWGRAPH_AMD64SUPPORT, ... when the
 * libraries are installed, so you need to know which architecture backends you
 * want to initialize.
 *
 * Compile your frontend with: 
 *
 * gcc `diabloflowgraph_opt32-config --cflags` my_frontend.c -o my_frontend `diabloflowgraph_opt32-config --libs` 
 *
 * or
 * 
 * gcc `diabloflowgraph_opt64-config --cflags` my_frontend.c -o my_frontend `diabloflowgraph_opt64-config --libs` 
 *
 * or
 *
 * gcc `diabloflowgraph_generic-config --cflags` my_frontend.c -o my_frontend `diabloflowgraph_generic-config --libs` 
 *
 * */

#define DIABLOFLOWGRAPH_INTERNAL
#include <diabloflowgraph.h>
#ifdef DIABLOFLOWGRAPH_ARMSUPPORT
#include <diabloarm.h>
#endif
#ifdef DIABLOFLOWGRAPH_I386SUPPORT
#include <diabloi386.h>
#endif
#ifdef DIABLOFLOWGRAPH_AMD64SUPPORT
#include <diabloamd64.h>
#endif
#ifdef DIABLOFLOWGRAPH_PPCSUPPORT
#include <diabloppc.h>
#endif
#ifdef DIABLOFLOWGRAPH_PPC64SUPPORT
#include <diabloppc64.h>
#endif
#ifdef DIABLOFLOWGRAPH_SPE_SUPPORT
#include <diablospe.h>
#endif

int
Optimize (t_cfg * cfg)
{
  /* Remove unconnected blocks */
  STATUS(START, ("Removing unconnected blocks"));
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  STATUS(STOP, ("Removing unconnected blocks"));

  CfgPatchToSingleEntryFunctions (cfg);
  CfgRemoveDeadCodeAndDataBlocks (cfg);

  CfgRemoveUselessConditionalJumps (cfg);
  CfgRemoveEmptyBlocks (cfg);

  CfgRemoveDeadCodeAndDataBlocks (cfg);
  return 0;
}

/* MAIN */
int
main (int argc, char **argv)
{
  int i;

  DiabloFlowgraphInit (argc, argv);
#ifdef DIABLOFLOWGRAPH_ARMSUPPORT
  DiabloArmInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_I386SUPPORT
  DiabloI386Init (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_AMD64SUPPORT
  DiabloAmd64Init (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_PPCSUPPORT
  DiabloPpcInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_PPC64SUPPORT
  DiabloPpc64Init (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_SPE_SUPPORT
  DiabloSpeInit (argc, argv);
#endif

  for (i=1; i<argc;i++)
  {
    if (argv[i])
    {
      t_string outname = StringConcat2 (argv[i], "-optimized");
      VERBOSE(0, ("Optimizing %s",argv[i]));
      ObjectRewrite (argv[i], Optimize, outname);
      Free (outname);
    }
  }

#ifdef DIABLOFLOWGRAPH_I386SUPPORT
  DiabloI386Fini ();
#endif
#ifdef DIABLOFLOWGRAPH_ARMSUPPORT
  DiabloArmFini ();
#endif
#ifdef DIABLOFLOWGRAPH_AMD64SUPPORT
  DiabloAmd64Fini ();
#endif
#ifdef DIABLOFLOWGRAPH_PPCSUPPORT
  DiabloPpcFini ();
#endif
#ifdef DIABLOFLOWGRAPH_PPC64SUPPORT
  DiabloPpc64Fini ();
#endif
#ifdef DIABLOFLOWGRAPH_SPE_SUPPORT
  DiabloSpeFini ();
#endif
  DiabloFlowgraphFini ();
  DiabloBrokerFini ();

  diablosupport_options.verbose++;
  PrintRemainingBlocks ();
  return 0;
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
