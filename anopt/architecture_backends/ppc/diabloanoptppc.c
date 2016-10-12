/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Jonas Maebe <Jonas.Maebe@elis.ugent.be>
 *      Lluis Vilanova <xscript@gmx.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * }}}
 * 
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diabloanoptppc.h>

/** This function computes the registers that a procedure saved on the stack */
/* XXX: in fact this is unsafe! (hand-written assembly code can easily change
* the saved registers on the stack) */
/*{{{*/
extern void PpcFunComputeStackSavedRegisters(t_cfg * cfg, t_function * ifun)
{
  if (diabloanopt_options.rely_on_calling_conventions)
  {
    if (FUNCTION_FLAGS(ifun) & FF_IS_EXPORTED)
    {
      t_architecture_description *desc = CFG_DESCRIPTION(FUNCTION_CFG(ifun));
      t_regset saved = FUNCTION_REGS_SAVED (ifun);
      RegsetSetUnion (saved,
                      RegsetDiff (desc->callee_saved, desc->callee_may_use));
      FUNCTION_SET_REGS_SAVED(ifun, saved);
    }
  }
}

void 
DiabloAnoptPpcInitCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;

  /* Generic MergeBbls for the PPC*/
  DiabloBrokerCallInstall("MergeBbls", "const t_cfg *" , MergeBbls, TRUE, cfg);

  /* Generic BranchElimination for the PPC */
  DiabloBrokerCallInstall("BranchElimination", "const t_cfg *" , BranchElimination,TRUE, cfg);

  /* Generic Branchforwarding for the PPC */
  DiabloBrokerCallInstall("BranchForwarding", "const t_cfg *" , BranchForwarding,TRUE, cfg);

  /*! Optimizes multiple loads and stores. */
  //DiabloBrokerCallInstall("OptimizeStackLoadAndStores", "const t_cfg *" , PpcOptimizeStackLoadAndStores,TRUE, cfg);
  
  /*! Copy propagation for the Ppc */
  //DiabloBrokerCallInstall("OptCopyPropagation", "const t_cfg *" , PpcOptCopyPropagation,TRUE, cfg);
  //DiabloBrokerCallInstall("PpcLoadStoreFwd", "const t_cfg *" , PpcLoadStoreFwd,TRUE, cfg);
  //DiabloBrokerCallInstall("PpcLoadStoreFwdNoDom", "const t_cfg *" , PpcLoadStoreFwdNoDom,TRUE, cfg);
  //DiabloBrokerCallInstall("ConstantPropagationInit", "const t_cfg *" , PpcConstantPropagationInit,TRUE, cfg);
  //DiabloBrokerCallInstall("CopyAnalysisInit", "const t_cfg *" , PpcCopyAnalysisInit,TRUE, cfg);
  DiabloBrokerCallInstall("BblFactorInit", "const t_cfg *" , PpcBblFactorInit,TRUE, cfg);
  //DiabloBrokerCallInstall("PrecomputeCopyPropEvaluation","const t_cfg *" ,PpcPrecomputeCopyPropEvaluation, TRUE, cfg);
  //DiabloBrokerCallInstall("RenameLocalAddressProducers","const t_cfg *,t_bbl *",RenameLocalAddressProducers,TRUE,cfg);
  DiabloBrokerCallInstall("CfgComputeStackSavedRegistersForFun","const t_cfg *,t_function*",PpcFunComputeStackSavedRegisters,TRUE,cfg);

  /* Misc PPC specific optimizations */
  //DiabloBrokerCallInstall("CfgBranchSwitch", "const t_cfg *", PpcCfgBranchSwitch, TRUE, cfg);
  //DiabloBrokerCallInstall("CfgBranchSwitch2", "const t_cfg *", PpcCfgBranchSwitch2, TRUE, cfg);
  //DiabloBrokerCallInstall("CfgBranchSwitch3", "const t_cfg *", PpcCfgBranchSwitch3, TRUE, cfg);
  //DiabloBrokerCallInstall("CfgBranchSwitch4", "const t_cfg *", PpcCfgBranchSwitch4, TRUE, cfg);
  //DiabloBrokerCallInstall("MakeConstProducers", "const t_cfg *", MakeConstProducers, TRUE, cfg);
  //DiabloBrokerCallInstall("SwitchMoves", "const t_cfg *", SwitchMoves, TRUE, cfg);

  /*! Inlining for the Ppc */
  //DiabloBrokerCallInstall("GeneralInlining", "const t_cfg *" , PpcGeneralInlining,TRUE, cfg);
  
  /*! Inlining for the Ppc (one call site/small functions) */
  //DiabloBrokerCallInstall("InlineTrivial", "const t_cfg *" , PpcInlineTrivial,TRUE, cfg);

  /* Needed in stack.c */
  //DiabloBrokerCallInstall("PpcLookAtStack", "const t_cfg *", PpcLookAtStack, TRUE, cfg);
  BblInitEqsIn(cfg);
  CfgEdgeInitEqs(cfg);
  //FunctionInitStackInfo(cfg);

}

void 
DiabloAnoptPpcFiniCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;
  BblFiniEqsIn(cfg);
  CfgEdgeFiniEqs(cfg);
  //FunctionFiniStackInfo(cfg);
}

void 
DiabloAnoptPpcObjectFreeCallback(t_object * obj)
{
  CfgCallbackUninstall(obj,CB_NEW,100,DiabloAnoptPpcInitCfg,NULL);
  CfgCallbackUninstall(obj,CB_FREE,-100,DiabloAnoptPpcFiniCfg,NULL);
}

void DiabloAnoptPpcObjectNewCallback(t_object * obj)
{
  if (!OBJECT_SUBOBJECT_CACHE(obj) &&
      ((strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name,"ppc")==0) ||
       (strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name,"ppc64")==0)))
  {
    CfgCallbackInstall(obj,CB_NEW,100,DiabloAnoptPpcInitCfg,NULL);
    CfgCallbackInstall(obj,CB_FREE,-100,DiabloAnoptPpcFiniCfg,NULL);
    DiabloBrokerCallInstall("BeforeObjectFree","const t_object *",DiabloAnoptPpcObjectFreeCallback,FALSE, obj);
  }
}

static int anoptppc_module_usecount = 0;

void
DiabloAnoptPpcInit (int argc, char **argv)
{
  if (!anoptppc_module_usecount)
  {
    DiabloAnoptInit(argc, argv);
    DiabloPpcInit(argc, argv);
    DiabloBrokerCallInstall("AfterObjectRead","t_object *",DiabloAnoptPpcObjectNewCallback,FALSE);
  }
  anoptppc_module_usecount++;
}

void 
DiabloAnoptPpcFini()
{
  anoptppc_module_usecount--;
  if (!anoptppc_module_usecount)
  {
    DiabloPpcFini();
    DiabloAnoptFini();
  }
}
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
