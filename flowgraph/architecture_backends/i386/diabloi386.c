/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloi386.h>
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_PECOFF_SUPPORT
#include <diablopecoff.h>
#include <diablomsil.h>
#endif

static int i386_module_usecount = 0;

void
DiabloI386Init (int argc, char **argv)
{
  if (!i386_module_usecount)
  {
    DiabloFlowgraphInit(argc, argv);
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_PECOFF_SUPPORT
    DiabloPeCoffInit(argc, argv);
    DiabloMsilInit(argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit(argc, argv);
#endif
    DiabloI386CmdlineInit ();
    OptionParseCommandLine (diabloi386_option_list, argc, argv,FALSE);
    OptionGetEnvironment (diabloi386_option_list);
    DiabloI386CmdlineVerify ();
    OptionDefaults (diabloi386_option_list);
    ArchitectureHandlerAdd("i386",&i386_description,ADDRSIZE32);
    I386InitOpcodeTable();
    I386CreateOpcodeHashTable();
    DiabloBrokerCallInstall ("AddInstrumentationToBbl", "t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_section* sequencing_counter_sec, t_address offset", I386AddInstrumentationToBbl, TRUE);
    DiabloBrokerCallInstall("AddCallFromBblToBbl", "t_object* obj, t_bbl* from, t_bbl* to", I386AddCallFromBblToBbl, TRUE);
    DiabloI386CppInit (argc, argv);
  }

  i386_module_usecount++;
}

void 
DiabloI386Fini()
{
  i386_module_usecount--;
  if (!i386_module_usecount)
  {
    DiabloI386CppFini ();

#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsFini();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfFini();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_PECOFF_SUPPORT
    DiabloPeCoffFini();
    DiabloMsilFini();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini();
#endif
    I386DestroyOpcodeHashTable();
    ArchitectureHandlerRemove("i386");
    DiabloI386CmdlineFini();
    DiabloFlowgraphFini();
  }
}
