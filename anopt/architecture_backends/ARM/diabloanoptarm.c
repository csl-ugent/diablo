/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
#include <diabloarm.h>

/* This function iterates over every instruction in a CFG,
 * and minimizes the size of the Thumb instructions while doing this.
 * As some instructions in Thumb can't be encoded in 16-bit while FL_S
 * is not set, this function tries to set FL_S if possible (i.e. when
 * the statusflags are dead before this instruction) and verifies whether
 * the instruction can be encoded in 16-bit. Otherwise, a 32-bit encoding
 * is selected. */
static void ArmMinimizeThumbInstructionSize(t_cfg * cfg)
{
  t_bbl * i_bbl = NULL;
  t_arm_ins * i_ins = NULL;
  t_uint32 gain = 0;
  t_uint32 nr_s = 0;

  CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);

  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    t_cfg_edge * i_edge = NULL;
    t_bool bbl_has_incoming_switch_edge = FALSE;

    /* set this boolean to handle switch tables of branches correctly */
    BBL_FOREACH_PRED_EDGE(i_bbl, i_edge)
      if (CFG_EDGE_CAT(i_edge) == ET_SWITCH
          || CFG_EDGE_CAT(i_edge) == ET_IPSWITCH)
      {
        bbl_has_incoming_switch_edge = TRUE;
        break;
      }

    BBL_FOREACH_ARM_INS(i_bbl, i_ins)
    {
      /* is the instruction uberhaupt encodable in ARM or Thumb? */
      ASSERT(ArmInsIsEncodable(i_ins), ("instruction not encodable @I", i_ins));

      /* if it is a Thumb instruction, try to minimize its size */
      if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
      {
        /* do not resize branches that are part of switch tables */
        if (!(bbl_has_incoming_switch_edge
              && BBL_NINS(i_bbl) == 1
              && ARM_INS_OPCODE(i_ins) == ARM_B))
        {
          if (ArmIsThumb1EncodableCheck(i_ins, FALSE))
          {
            /* the instruction is immediately encodable in 16-bit Thumb */
            ARM_INS_SET_CSIZE(i_ins, 2);
            gain += 2;
          }
          else if (!(ARM_INS_FLAGS(i_ins) & FL_S)
                    && !ArmStatusFlagsLiveBefore(i_ins))
          {
            /* the instruction is not immediately encodable in 16-bit Thumb,
             * but it may be encodable if the S-flag is set. However, we can
             * only do this when the statusflags are dead before this instruction. */
            ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) | FL_S);

            if (ArmIsThumb1EncodableCheck(i_ins, FALSE))
            {
              /* now the instruction is encodable in 16-bit Thumb */
              ARM_INS_SET_CSIZE(i_ins, 2);
              gain += 2;
              nr_s++;
            }
            else
            {
              /* the instruction still isn't encodable in 16-bit Thumb;
               * undo our changes as they didn't have any effect */
              ARM_INS_SET_FLAGS(i_ins, ARM_INS_FLAGS(i_ins) & ~FL_S);
              ARM_INS_SET_CSIZE(i_ins, 4);
            }
          }
          else
          {
            /* this instruction is certainly not encodable in 16-bit Thumb, do nothing */
            ARM_INS_SET_CSIZE(i_ins, 4);
          }
        }
      }
    }
  }

  VERBOSE(0, ("made %d 2-byte Thumb instructions (gain %d), of which %d now set the statusflags", gain>>1, gain, nr_s));
}

void
ArmPrecomputeCopyPropEvaluation(t_cfg * cfg)
{
    t_ins * i_ins;
    t_bbl * i_bbl;
    CFG_FOREACH_BBL(cfg,i_bbl)
      BBL_FOREACH_INS(i_bbl,i_ins)
      ArmInsPrecomputeCopyPropEvaluation(T_ARM_INS(i_ins));
}

int anoptarm_cfg_usecount = 0;

void
DiabloAnoptArmInitCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;

  if(!anoptarm_cfg_usecount)
  {
    DiabloBrokerCallInstall("MergeBbls", "const t_cfg *" , MergeBbls, TRUE, cfg);
    /* BranchElimination for the ARM */
    DiabloBrokerCallInstall("BranchElimination", "const t_cfg *" , BranchElimination,TRUE, cfg);
    /*! Branchforwarding for the ARM */
    DiabloBrokerCallInstall("BranchForwarding", "const t_cfg *" , BranchForwarding,TRUE, cfg);
    DiabloBrokerCallInstall("ForwardToReturnDo", "t_bbl *, t_bool *" , ArmForwardToReturnDo, TRUE, cfg);

    /*! Optimizes multiple loads and stores. */
    DiabloBrokerCallInstall("OptimizeStackLoadAndStores", "const t_cfg *" , ArmOptimizeStackLoadAndStores,TRUE, cfg);
    DiabloBrokerCallInstall("OptimizeSingleThreaded", "t_cfg *", OptimizeSingleThreaded, TRUE, cfg);
    /*! Copy propagation for the Arm */
    DiabloBrokerCallInstall("OptCopyPropagation", "const t_cfg *" , ArmOptCopyPropagation,TRUE, cfg);
    DiabloBrokerCallInstall("ArmLoadStoreFwd", "const t_cfg *" , ArmLoadStoreFwd,TRUE, cfg);
    DiabloBrokerCallInstall("ArmLoadStoreFwdNoDom", "const t_cfg *" , ArmLoadStoreFwdNoDom,TRUE, cfg);

    DiabloBrokerCallInstall("ConstantPropagationInit", "const t_cfg *" , ArmConstantPropagationInit,TRUE, cfg);
    DiabloBrokerCallInstall("CopyAnalysisInit", "const t_cfg *" , ArmCopyAnalysisInit,TRUE, cfg);
    DiabloBrokerCallInstall("BblFactorInit", "const t_cfg *" , ArmBblFactorInit,TRUE, cfg);
    DiabloBrokerCallInstall("PrecomputeCopyPropEvaluation","const t_cfg *" ,ArmPrecomputeCopyPropEvaluation, TRUE, cfg);
    DiabloBrokerCallInstall("RenameLocalAddressProducers","const t_cfg *",RenameLocalAddressProducers,TRUE,cfg);
    DiabloBrokerCallInstall("CfgComputeStackSavedRegistersForFun","const t_cfg *,t_function*",ArmFunComputeStackSavedRegisters,TRUE,cfg);

    /* Misc ARM specific optimizations */
    DiabloBrokerCallInstall("CfgBranchSwitch", "const t_cfg *", ArmCfgBranchSwitch, TRUE, cfg);
    DiabloBrokerCallInstall("CfgBranchSwitch2", "const t_cfg *", ArmCfgBranchSwitch2, TRUE, cfg);
    DiabloBrokerCallInstall("CfgBranchSwitch3", "const t_cfg *", ArmCfgBranchSwitch3, TRUE, cfg);
    DiabloBrokerCallInstall("CfgBranchSwitch4", "const t_cfg *", ArmCfgBranchSwitch4, TRUE, cfg);
    DiabloBrokerCallInstall("MakeConstProducers", "const t_cfg *", MakeConstProducers, TRUE, cfg);
    DiabloBrokerCallInstall("SwitchMoves", "const t_cfg *", SwitchMoves, TRUE, cfg);
    /*! Inlining for the Arm */
    DiabloBrokerCallInstall("GeneralInlining", "const t_cfg *" , ArmGeneralInlining,TRUE, cfg);
    /*! Inlining for the Arm (one call site/small functions) */
    DiabloBrokerCallInstall("InlineTrivial", "const t_cfg *" , ArmInlineTrivial,TRUE, cfg);
    /* Needed in stack.c */
    DiabloBrokerCallInstall("ArmLookAtStack", "const t_cfg *", ArmLookAtStack, TRUE, cfg);

    DiabloBrokerCallInstall("EpilogueFactorAfter", "t_bbl *, t_bbl *", ArmEpilogueFactorAfter, TRUE, cfg);
    DiabloBrokerCallInstall("FunctionFactorAfterJumpCreation", "t_bbl *, t_bbl *", ArmFunctionFactorAfterJumpCreation, TRUE, cfg);

    /* Needed in CfgMoveIns[Down|Up] to e.g. check whether an instruction can be moved between ARM-Thumb BBL's */
    DiabloBrokerCallInstall("InsCanMoveDown", "t_arm_ins *, t_bool *", ArmInsCanMoveDown, TRUE, cfg);
    DiabloBrokerCallInstall("InsCanMoveUp", "t_arm_ins *, t_bool *", ArmInsCanMoveUp, TRUE, cfg);

    DiabloBrokerCallInstall("BblCanMerge", "t_bbl *, t_bbl *, t_bool *", ArmBblCanMerge, TRUE, cfg);

    DiabloBrokerCallInstall("BranchEliminationDo", "t_bbl *, t_bbl *, t_bool *", ArmBranchEliminationDo, TRUE, cfg);
    DiabloBrokerCallInstall("BranchForwardingDo", "t_cfg_edge *, t_bool *", ArmBranchForwardingDo, TRUE, cfg);

    /* Minimizing ARM instruction size is a sort of optimalization pass that should happen right before deflowgraphing */
    DiabloBrokerCallInstall("BeforeDeflowgraph", "t_cfg *", ArmMinimizeThumbInstructionSize, FALSE);

    /* advanced factoring helpers */
    DiabloBrokerCallInstall("BblConstructCompareMatrices", "const t_cfg *, t_string", ConstructBblCompareMatrices, TRUE, cfg);
  }

  DiabloAnoptArmInitCfgCpp(vcfg, data);

  /* Do dynamic member initialization */
  BblInitEqsIn(cfg);
  CfgEdgeInitEqs(cfg);
  FunctionInitStackInfo(cfg);

  anoptarm_cfg_usecount++;
}

void DiabloAnoptArmFiniCfg(void * vcfg, void * data)
{
  t_cfg * cfg=vcfg;
  anoptarm_cfg_usecount--;

  /* Do dynamic member finalization */
  BblFiniEqsIn(cfg);
  CfgEdgeFiniEqs(cfg);
  FunctionFiniStackInfo(cfg);
}

void DiabloAnoptArmObjectFreeCallback(t_object * obj)
{
  CfgCallbackUninstall(obj,CB_NEW,100,DiabloAnoptArmInitCfg,NULL);
  CfgCallbackUninstall(obj,CB_FREE,-100,DiabloAnoptArmFiniCfg,NULL);
}

void DiabloAnoptArmObjectNewCallback(t_object * obj)
{
  if (!OBJECT_SUBOBJECT_CACHE(obj) && strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name,"ARM")==0)
  {
    CfgCallbackInstall(obj,CB_NEW,100,DiabloAnoptArmInitCfg,NULL);
    CfgCallbackInstall(obj,CB_FREE,-100,DiabloAnoptArmFiniCfg,NULL);
    DiabloBrokerCallInstall("BeforeObjectFree","const t_object *",DiabloAnoptArmObjectFreeCallback,FALSE, obj);
  }
}

static int anoptarm_module_usecount = 0;

void
DiabloAnoptArmInit (int argc, char **argv)
{
  if (!anoptarm_module_usecount)
  {
    DiabloAnoptInit(argc, argv);
    DiabloArmInit(argc, argv);

    DiabloAnoptArmCmdlineInit ();
    OptionParseCommandLine (diabloanoptarm_option_list, argc, argv,FALSE);
    OptionGetEnvironment (diabloanoptarm_option_list);
    DiabloAnoptArmCmdlineVerify ();
    OptionDefaults (diabloanoptarm_option_list);

    DiabloBrokerCallInstall("AfterObjectRead","t_object *",DiabloAnoptArmObjectNewCallback,FALSE);
  }

  anoptarm_module_usecount++;
}

void
DiabloAnoptArmFini()
{
  anoptarm_module_usecount--;

  if (!anoptarm_module_usecount)
  {
    DiabloAnoptArmCmdlineFini();
    DiabloArmFini();
    DiabloAnoptFini();
  }
}
