
#include <diabloanopti386.h>
#ifdef DIABLOANOPTI386_FUNCTIONS
#ifndef DIABLOANOPTI386_CONSTANTOPTIMIZATIONS_H
#define DIABLOANOPTI386_CONSTANTOPTIMIZATIONS_H


t_reg I386InversRegisterNumber(t_reg reg);
void I386PrintInsInfo(t_i386_ins * ins);
t_bool I386IsFP(t_ins * ins);
t_bool I386RegIsEqualAndKnown(t_reg reg,t_procstate * prev_state,t_procstate * next_state);
t_bool I386CondIsEqualAndKnown(t_reg reg,t_procstate * prev_state,t_procstate * next_state);

t_bool I386UnconditionalizeJumps(t_ins * ins,t_procstate * prev_state);
t_bool I386KillIdempotentIns(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);
t_bool I386KillUselessIns(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);
t_bool I386MakeIndirectJumpsDirect(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);
t_bool I386MakeIndirectCallsDirect(t_ins * ins, t_procstate * prev_state, t_procstate * next_state, t_analysis_complexity complexity);
t_bool I386EncodeConstantOperand(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);
t_bool I386StrengthReduction(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);
t_bool I386ProduceConstantFromRegister(t_ins * ins,t_procstate * prev_state,t_procstate * next_state);

#endif
#endif
