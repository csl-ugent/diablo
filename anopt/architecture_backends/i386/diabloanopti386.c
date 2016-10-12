#include <diabloanopti386.h>

void 
DiabloAnoptI386InitCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;
  
  DiabloBrokerCallInstall("MergeBbls", "const t_cfg *" , MergeBbls, TRUE, cfg);
  /* BranchElimination for the I386 */
  DiabloBrokerCallInstall("BranchElimination", "const t_cfg *" , BranchElimination,TRUE, cfg);
  /*! Branchforwarding for the I386... Seems to have a negative effect TODO: Check */
  //DiabloBrokerCallInstall("BranchForwarding", "const t_cfg *" , BranchForwarding,TRUE, cfg);
  DiabloBrokerCallInstall("ConstantPropagationInit", "const t_cfg *" , I386ConstantPropagationInit,TRUE, cfg);
  DiabloBrokerCallInstall("ConstantPropagationFini", "const t_cfg *" , I386ConstantPropagationFini,TRUE, cfg);
  DiabloBrokerCallInstall("CopyAnalysisInit", "const t_cfg *" , I386CopyAnalysisInit,TRUE, cfg);
  DiabloBrokerCallInstall("BblFactorInit", "const t_cfg *" , I386BblFactorInit,TRUE, cfg);
  DiabloBrokerCallInstall("I386InlineSimple", "const t_cfg *" , I386InlineSmallFunctions,TRUE,cfg);
  DiabloBrokerCallInstall("I386InlineFunctionsWithOneCallSite", "const t_cfg *" , I386InlineFunctionsWithOneCallSite,TRUE,cfg);
  DiabloBrokerCallInstall("I386PeepHoles", "const t_cfg *" , I386PeepHoles,TRUE,cfg);
  DiabloBrokerCallInstall("CfgComputeStackSavedRegistersForFun","const t_cfg *,t_function*",I386FunComputeStackSavedRegisters,TRUE,cfg);
}

void DiabloAnoptI386ObjectFreeCallback(t_object * obj)
{
      CfgCallbackUninstall(obj,CB_NEW,100,DiabloAnoptI386InitCfg,NULL);
}


void DiabloAnoptI386ObjectNewCallback(t_object * obj)
{
  if (!OBJECT_SUBOBJECT_CACHE(obj) && strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name,"i386")==0)
    {
      CfgCallbackInstall(obj,CB_NEW,100,DiabloAnoptI386InitCfg,NULL);
      DiabloBrokerCallInstall("BeforeObjectFree","const t_object *",DiabloAnoptI386ObjectFreeCallback,FALSE, obj);
    }
}

static int anopti386_module_usecount = 0;

void
DiabloAnoptI386Init (int argc, char **argv)
{
  if (!anopti386_module_usecount)
  {
    DiabloAnoptInit(argc, argv);
    DiabloI386Init(argc, argv);
    // DiabloAnoptI386CmdlineInit ();
    // OptionParseCommandLine (diabloanopti386_option_list, argc, argv,FALSE);
    // OptionGetEnvironment (diabloanopti386_option_list);
    // DiabloAnoptI386CmdlineVerify ();
    // OptionDefaults (diabloanopti386_option_list);
    // 
    DiabloBrokerCallInstall("AfterObjectRead","t_object *",DiabloAnoptI386ObjectNewCallback,FALSE);
  }

  anopti386_module_usecount++;
}

void 
DiabloAnoptI386Fini()
{
//  DiabloAnoptI386CmdlineFini();
  anopti386_module_usecount--;

  if (!anopti386_module_usecount)
  {
    DiabloI386Fini();
    DiabloAnoptFini();
  }
}
