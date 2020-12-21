/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#define GENERATE_CLASS_CODE
#ifndef DIABLOFLOWGRAPH_INTERNAL
#define DIABLOFLOWGRAPH_INTERNAL
#endif
#include <diabloarm.h>
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
#include <diablobinutils.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ARM_ADS_SUPPORT
#include <diabloarmads.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TILINKER_SUPPORT
#include <diablotilinker.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
#include <diabloelf.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TICOFF_SUPPORT
#include <diabloticoff.h>
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
#include <diabloar.h>
#endif


static int arm_module_usecount = 0;

void
DiabloArmInit (int argc, char **argv)
{
  if (!arm_module_usecount)
  {
    DiabloFlowgraphInit(argc, argv);
#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ARM_ADS_SUPPORT
    DiabloArmAdsInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TILINKER_SUPPORT
    DiabloTiLinkerInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_TICOFF_SUPPORT
    DiabloTiCoffInit (argc, argv);
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArInit (argc, argv);
#endif
    DiabloArmCmdlineInit ();
    OptionParseCommandLine (diabloarm_option_list, argc, argv,FALSE);
    OptionGetEnvironment (diabloarm_option_list);
    DiabloArmCmdlineVerify ();
    OptionDefaults (diabloarm_option_list);
    ArchitectureHandlerAdd ("ARM",&arm_description,ADDRSIZE32);

    /* install helper callback for FunctionDuplicate */
    DiabloBrokerCallInstall ("FunctionDuplicateAdditionalDataBlocks",
	   "t_function *, t_function *", ArmDupSwitchTables, FALSE);

    DiabloBrokerCallInstall ("InstructionIsDirectControlTransfer", "t_ins *, t_bool *", ArmInstructionIsDirectControlTransfer, FALSE);

    /* detect copied code regions and mark them as data (since otherwise
     * the wrong stuff may be copied)
     */
    DiabloBrokerCallInstall ("ObjectFlowgraphBefore", "t_object *", ChangeCopiedCodeToData, FALSE);

    DiabloBrokerCallInstall ("CfgCreated", "t_object *, t_cfg *", DiabloFlowgraphArmCfgCreated, FALSE);

    DiabloBrokerCallInstall("AddInstrumentationToBbl", "t_object* obj, t_bbl* bbl, t_section* profiling_sec, t_section* sequencing_counter_sec, t_address offset", ArmAddInstrumentationToBbl, TRUE);
    DiabloBrokerCallInstall("AddCallFromBblToBbl", "t_object* obj, t_bbl* from, t_bbl* to", ArmAddCallFromBblToBbl, TRUE);

    DiabloBrokerCallInstall("ArchitectureSpecificRegsetPrinter", "t_regset *, t_string_array *, t_bool *", ArmRegsetPrinterBroker, TRUE);

    DiabloArmCppInit (argc, argv);
  }

  arm_module_usecount++;
}

void 
DiabloArmFini()
{
  arm_module_usecount--;
  if (!arm_module_usecount)
  {
    DiabloArmCppFini ();

#ifdef DIABLOFLOWGRAPH_HAVE_BINUTILS_SUPPORT
    DiabloBinutilsFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ARM_ADS_SUPPORT
    DiabloArmAdsFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_ELF_SUPPORT
    DiabloElfFini ();
#endif
#ifdef DIABLOFLOWGRAPH_HAVE_AR_SUPPORT
    DiabloArFini ();
#endif
    ArchitectureHandlerRemove ("ARM");
    DiabloArmCmdlineFini();
    DiabloFlowgraphFini();
  }
}
