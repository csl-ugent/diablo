/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloanopt.h>
#ifndef DIABLO_CONSTPROP_H
#define DIABLO_CONSTPROP_H

void FunctionPropagateConstantsAfterIterativeSolution(t_function * fun, t_analysis_complexity complexity);

/* Add dynamic member testcondition to cfg edges */
EDGE_DYNAMIC_MEMBER_GLOBAL(testcondition, TESTCONDITION, TestCondition, TestAndSetConditionFunction, NULL);
EDGE_DYNAMIC_MEMBER_GLOBAL_BODY(procstate, PROCSTATE, Procstate, t_procstate *, { *valp=NULL; }, { if (*valp) { ProcStateFree(*valp); *valp = NULL; } }, {});

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(procstate_in, PROCSTATE_IN, ProcstateIn, t_procstate *, {*valp=NULL;}, {}, {});

typedef void (*t_ConstantPropagationInsEmul)(t_ins *, t_procstate *, t_bool);
typedef void (*t_EdgePropagator)(t_cfg_edge *, t_ins *);
typedef t_bool (*t_Unconditionalizer)(t_ins * );
typedef void (*t_InsConstantOptimizer)(t_ins *, t_procstate *, t_procstate * , t_analysis_complexity);
typedef void (*t_GetFirstInsOfConditionalBranchWithSideEffect)(t_bbl *,t_ins **);


void ConstantPropagation(t_cfg * cfg, t_analysis_complexity complexity);
void OptUseConstantInformation(t_cfg * cfg, t_analysis_complexity complexity);

CFG_DYNAMIC_MEMBER_GLOBAL_BODY(instruction_emulator, INSTRUCTION_EMULATOR, InstructionEmulator, t_ConstantPropagationInsEmul, {*valp=NULL;}, {}, {});
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(instruction_constant_optimizer, INSTRUCTION_CONSTANT_OPTIMIZER, InstructionConstantOptimizer, t_InsConstantOptimizer, {*valp=NULL;}, {}, {});
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(edge_propagator, EDGE_PROPAGATOR, EdgePropagator, t_EdgePropagator, {*valp=NULL;}, {}, {});
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(get_first_ins_of, GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT, GetFirstInsOfConditionalBranchWithSideEffect, t_GetFirstInsOfConditionalBranchWithSideEffect, {*valp=NULL;}, {}, {});

INS_DYNAMIC_MEMBER_GLOBAL(usearg,USEARG,UseArg,long,-1L);
INS_DYNAMIC_MEMBER_GLOBAL(defarg,DEFARG,DefArg,long,-1L);
INS_DYNAMIC_MEMBER_GLOBAL(defedge,DEFEDGE,DefEdge,t_cfg_edge *,NULL);
FUNCTION_DYNAMIC_MEMBER_GLOBAL(nargs,NARGS,Nargs,long,0);
EDGE_DYNAMIC_MEMBER_GLOBAL(args,ARGS,Args,t_argstate *,NULL);
EDGE_DYNAMIC_MEMBER_GLOBAL(args_changed,ARGS_CHANGED,ArgsChanged,t_bool,FALSE);

extern t_uint32 global_optimization_phase;

void	FreeConstantInformation(t_cfg * cfg);
void AddKnownConstantSym(t_symbol *sym);
void KillKnownConstantSymTable(void);
t_bool IsKnownToBeConstant(t_address address, t_reloc *rel);
void BblPropagateConstants(t_bbl * bbl, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations, t_analysis_complexity complexity);

t_bool ConstantPropagationInit(t_cfg * cfg);
void ConstantPropagationFini(t_cfg * cfg);
#endif
