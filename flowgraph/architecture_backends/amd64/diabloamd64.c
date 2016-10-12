#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloamd64.h>
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int amd64_module_usecount = 0;

void
DiabloAmd64Init (int argc, char **argv)
{
  if (!amd64_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit(argc, argv);
#endif
    //  DiabloAmd64CmdlineInit ();
    //  OptionParseCommandLine (diabloamd64_option_list, argc, argv,FALSE);
    //  OptionGetEnvironment (diabloamd64_option_list);
    //  DiabloAmd64CmdlineVerify ();
    //  OptionDefaults (diabloamd64_option_list);
    ArchitectureHandlerAdd("amd64",&amd64_description,ADDRSIZE64);
    Amd64InitOpcodeTable();
    Amd64CreateOpcodeHashTable();
  }

  amd64_module_usecount++;
}

void 
DiabloAmd64Fini()
{
  amd64_module_usecount--;
  if (!amd64_module_usecount)
  {
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsFini();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfFini();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini();
#endif
    /*Amd64DestroyOpcodeHashTable();*/
    ArchitectureHandlerRemove("amd64");
    //  DiabloAmd64CmdlineFini();
  }
}
