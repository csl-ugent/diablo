#include <diabloanopti386.h>
#ifndef I386_COPY_PROPAGATION_H
#define I386_COPY_PROPAGATION_H
void I386CopyInstructionPropagator(t_i386_ins * ins, t_equations eqs, t_bool ignore_condition);
void I386BackwardCopyPropagation(t_section * sec);
void I386CopyAnalysisInit(t_cfg * cfg);
#endif

/* vim: set shiftwidth=2 foldmethod=marker : */
