#include <diabloflowgraph.h>

void ArmCfgBranchSwitch(t_cfg * cfg);
void ArmCfgBranchSwitch2(t_cfg * cfg);
void ArmCfgBranchSwitch3(t_cfg * cfg);
void ArmCfgBranchSwitch4(t_cfg * cfg);
void ArmCfgHoistConstantProducingCode(t_cfg * cfg);
t_bool CfgHoistConstantProducingCode3(t_cfg * cfg);
t_regset LoopFreeRegisters(t_cfg * cfg, t_loop * loop);
t_bbl * LoopAddPreheader(t_cfg * cfg,t_loop * loop);
t_bbl * CreateNewTargetBlock(t_cfg * cfg, t_cfg_edge * edge);
t_ins * LoopContainsConstantProducers(t_cfg * cfg, t_loop * loop);
void DetectLoopStackSubAdds(t_cfg * cfg);
void DetectLoopInvariants(t_cfg * cfg);
void LoopUnrollingSimple(t_cfg * cfg);
void DetectColdCodeBundles(t_cfg * cfg);
void PartialRedundancyElimination1(t_cfg * cfg);
void PartialRedundancyElimination1ForCalls(t_cfg * cfg);
void PartialRedundancyElimination1ForReturns(t_cfg * cfg);
void ReplaceTriangleWithConditionalMove(t_cfg * cfg);
void PartialRedundancyElimination2(t_cfg * cfg);
void PartialRedundancyElimination3(t_cfg * cfg);
void ReplaceRectangleWithConditionalIns(t_cfg * cfg);
