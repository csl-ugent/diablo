#include <diabloanopti386.h>
#ifndef I386_CONSTANT_PROPAGATION_H
#define I386_CONSTANT_PROPAGATION_H
void I386EdgePropagator(t_cfg_edge * edge, t_i386_ins * ins);
t_bool I386InstructionUnconditionalizer(t_i386_ins * ins);
t_bool I386InstructionsConstantOptimizer(t_i386_ins * ins, t_procstate * procstate1, t_procstate * procstate2, t_analysis_complexity complexity);
void I386ConstantPropagationInit(t_cfg * cfg);
void I386ConstantPropagationFini(t_cfg *cfg);
#endif
/* vim: set shiftwidth=2: */
