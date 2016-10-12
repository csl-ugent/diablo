#include <diabloanoptarm.h>

#ifndef DIABLO_ARM_INLINE_H
#define DIABLO_ARM_INLINE_H
void ArmInlineTrivial(t_cfg * cfg);
void ArmGeneralInlining(t_cfg * cfg);
void ArmInlineProcedureAtOneCallsite(t_function * fun, t_cfg_edge * call_edge);
#endif
/*@}*/
/* vim: set shiftwidth=2 foldmethod=marker: */
