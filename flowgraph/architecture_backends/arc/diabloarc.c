#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloarc.h>
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int arc_module_usecount = 0;

void
DiabloArcInit (int argc, char **argv)
{
  if (!arc_module_usecount)
  {
    DiabloFlowgraphInit(argc, argv);
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit (argc, argv);
#endif
    ArchitectureHandlerAdd ("arc",&arc_description, ADDRSIZE32);

  }

  arc_module_usecount++;
}

void 
DiabloArcFini()
{
  arc_module_usecount--;
  if (!arc_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini ();
#endif
    ArchitectureHandlerRemove("arc");
    DiabloFlowgraphFini();
  }
}
