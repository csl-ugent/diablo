/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopt.h>
#ifndef _COPY_ANALYSIS_H
#define _COPY_ANALYSIS_H

typedef void (*t_CopyInstructionPropagator)(t_ins * ins, t_equations eqs, t_bool ignore_condition);
t_bool CopyAnalysisInit(t_cfg * cfg);
void CopyAnalysis(t_cfg *);
void CopyAnalysisFini(t_cfg *);
void BblCopyAnalysisUntilIns(t_ins * ins, t_equations eqs);
void BblCopyAnalysisFromInsToUntilIns(t_ins * from_ins, t_ins * to_ins, t_equations eqs);
extern t_uint32 max_nr_equations, null_register;
void BblCopyAnalysis(t_bbl * bbl, t_cfg_edge * edge_out, t_bool during_fixpoint_calculations);

EDGE_DYNAMIC_MEMBER_GLOBAL_BODY(equations, EQS, Eqs, t_equations, {*valp=NULL;}, { if (valp && *valp) EquationsFree (*valp); }, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(eqs_in, EQS_IN, EqsIn, t_equations, {*valp=NULL;}, { if (valp && *valp) EquationsFree (*valp); }, {});
CFG_DYNAMIC_MEMBER_GLOBAL_BODY(copy_instruction_propagator, COPY_INSTRUCTION_PROPAGATOR, CopyInstructionPropagator, t_CopyInstructionPropagator, { *valp=NULL; }, {}, {});

#endif
