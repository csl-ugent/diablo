#include <diabloanoptamd64.h>

void 
DiabloAnoptAmd64InitCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;
  printf("AMD64 CFG!\n");
  
  /* these callback functions don't exist yet!!!!!!!!!!!!!!!!!!!! */
  DiabloBrokerCallInstall("MergeBbls", "const t_cfg *" , MergeBbls, TRUE, cfg);
  /* BranchElimination for the AMD64 */
  DiabloBrokerCallInstall("BranchElimination", "const t_cfg *" , BranchElimination,TRUE, cfg);
  /*! Branchforwarding for the AMD64... Seems to have a negative effect TODO: Check */
  /*DiabloBrokerCallInstall("BranchForwarding", "const t_cfg *" , BranchForwarding,TRUE, cfg);*/

  /*DiabloBrokerCallInstall("ConstantPropagationInit", "const t_cfg *" , Amd64ConstantPropagationInit,TRUE, cfg);*/
  /*DiabloBrokerCallInstall("CopyAnalysisInit", "const t_cfg *" , Amd64CopyAnalysisInit,TRUE, cfg);*/
  DiabloBrokerCallInstall("BblFactorInit", "const t_cfg *" , Amd64BblFactorInit,TRUE, cfg);
  DiabloBrokerCallInstall("Amd64InlineSimple", "const t_cfg *" , Amd64InlineSmallFunctions,TRUE,cfg);
  DiabloBrokerCallInstall("Amd64InlineFunctionsWithOneCallSite", "const t_cfg *" , Amd64InlineFunctionsWithOneCallSite,TRUE,cfg);
  DiabloBrokerCallInstall("Amd64PeepHoles", "const t_cfg *" , Amd64PeepHoles,TRUE,cfg);

  DiabloBrokerCallInstall("CfgComputeStackSavedRegistersForFun","const t_cfg *,t_function*",Amd64FunComputeStackSavedRegisters,TRUE,cfg);

}

void DiabloAnoptAmd64ObjectFreeCallback(t_object * obj)
{
      CfgCallbackUninstall(obj,CB_NEW,100,DiabloAnoptAmd64InitCfg,NULL);
}


void DiabloAnoptAmd64ObjectNewCallback(t_object * obj)
{
  if (!OBJECT_SUBOBJECT_CACHE(obj) && strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name,"amd64")==0)
    {
      CfgCallbackInstall(obj,CB_NEW,100,DiabloAnoptAmd64InitCfg,NULL);
      DiabloBrokerCallInstall("BeforeObjectFree","const t_object *",DiabloAnoptAmd64ObjectFreeCallback,FALSE, obj);
    }
}

static int anoptamd64_module_usecount = 0;

void
DiabloAnoptAmd64Init (int argc, char **argv)
{
  if (!anoptamd64_module_usecount)
  {
    DiabloAmd64Init(argc, argv);
    //  DiabloAnoptAmd64CmdlineInit ();
    //  OptionParseCommandLine (diabloamd64_option_list, argc, argv,FALSE);
    //  OptionGetEnvironment (diabloamd64_option_list);
    //  DiabloAnoptAmd64CmdlineVerify ();
    //  OptionDefaults (diabloamd64_option_list);
    //
    DiabloBrokerCallInstall("AfterObjectRead","t_object *",DiabloAnoptAmd64ObjectNewCallback,FALSE);
  }

  anoptamd64_module_usecount++;
}

void 
DiabloAnoptAmd64Fini()
{
//  DiabloAnoptAmd64CmdlineFini();
    anoptamd64_module_usecount--;

    if (!anoptamd64_module_usecount)
    {
      DiabloAmd64Fini();
    }
}
