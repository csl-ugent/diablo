/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_CONSTANT_PROPAGATION_H
#define ARM_CONSTANT_PROPAGATION_H
#include <diabloflowgraph.h>
#include <diabloarm.h>
void ArmEdgePropagator(t_cfg_edge * edge, t_arm_ins * ins);
t_bool ArmInsUnconditionalizer(t_arm_ins * ins);
t_bool ArmInssConstantOptimizer(t_arm_ins * ins, t_procstate * procstate1, t_procstate * procstate2, t_analysis_complexity complexity);
unsigned int ArmInsDefinedRegCount(t_arm_ins * ins);
void RenameLocalAddressProducers(t_cfg * cfg);
void MakeConstProducers(t_cfg * cfg);
void ArmConstantPropagationInit(t_cfg * cfg);
t_bool BblEndsWithConditionalBranchAfterCMP(t_bbl * bbl);
t_arm_ins * FindCmpThatDeterminesJump(t_arm_ins * ins);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
