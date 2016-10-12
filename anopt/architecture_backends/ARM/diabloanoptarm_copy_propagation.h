#include <diabloanoptarm.h>

#ifndef ARM_COPY_PROPAGATION_H
#define ARM_COPY_PROPAGATION_H
void ArmOptCopyPropagation(t_cfg * cfg);
t_bool ArmInsIsCopy(t_arm_ins * i_ins, t_reg * copy, t_reg * original);
void ArmCopyInstructionPropagator(t_arm_ins * ins, t_equations eqs, t_bool ignore_condition);
void ArmCopyAnalysisInit(t_cfg * cfg);
void ArmUseCopyPropagationInfo(t_cfg * cfg);
void ArmOptEliminateCmpEdges(t_cfg * cfg);
void ArmInsPrecomputeCopyPropEvaluation(t_arm_ins * ins);
#endif
