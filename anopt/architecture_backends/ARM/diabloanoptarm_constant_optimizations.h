#include <diabloanopt.h>
#ifndef ARM_CONSTANTOPTIMIZATIONS_H
#define ARM_CONSTANTOPTIMIZATIONS_H
t_bool ArmInsOptKillSBit(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptKillIdempotent(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptMakeIndirectJumpsDirect(t_arm_ins * ins, t_procstate * before, t_procstate * after, t_analysis_complexity complexity);
t_bool ArmInsOptSwitchOptimizer(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptEncodeConstantResult(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptProduceConstantFromOtherRegister(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptEncodeConstantOperands(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmInsOptEncodeConstantResultInTwoInstructions(t_arm_ins * ins, t_procstate * before, t_procstate * after);
t_bool ArmOptMoveNullIntoPC(t_arm_ins * ins);
t_bool ArmInsOptChangeLoadBaseRegister(t_arm_ins * ins, t_procstate * before, t_procstate * after);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
