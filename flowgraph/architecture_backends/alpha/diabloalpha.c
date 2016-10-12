#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloalpha.h>
#ifdef DIABLOFLOWGRAPH_HAVE_ECOFF_SUPPORT
#include <diabloecoff.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TRU64LINKER_SUPPORT
#include <diablotru64linker.h>
#endif


static int alpha_module_usecount = 0;

void
DiabloAlphaInit(int argc, char **argv)
{
  if (!alpha_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_ECOFF_SUPPORT
    DiabloEcoffInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TRU64LINKER_SUPPORT
    DiabloTru64LinkerInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit(argc, argv);
#endif
    VERBOSE(0, ("Alpha init!"));

		DiabloAlphaCmdlineInit ();
    OptionParseCommandLine (diabloalpha_option_list, argc, argv,FALSE);
    OptionGetEnvironment (diabloalpha_option_list);
    DiabloAlphaCmdlineVerify ();
    OptionDefaults (diabloalpha_option_list);

    ArchitectureHandlerAdd ("alpha",&alpha_description,ADDRSIZE64);
  }

  alpha_module_usecount++;
}

void 
DiabloAlphaFini()
{
  alpha_module_usecount--;
  if (!alpha_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_ECOFF_SUPPORT
    DiabloEcoffFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TRU64LINKER_SUPPORT
    DiabloTru64LinkerFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini ();
#endif
    ArchitectureHandlerRemove ("alpha");
  }
}
