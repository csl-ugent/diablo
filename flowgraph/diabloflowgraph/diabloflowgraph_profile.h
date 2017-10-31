/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>


#ifdef DIABLOFLOWGRAPH_FUNCTIONS

#ifndef DIABLO_PROFILE_FUNCTIONS
#define DIABLO_PROFILE_FUNCTIONS

#define SELFPROFILING_PREFIX "Profiling_"
#define SP_IDENTIFIER_PREFIX LINKIN_IDENTIFIER_PREFIX SELFPROFILING_PREFIX
#define PREFIX_FOR_LINKED_IN_SP_OBJECT "LINKED_IN_PROFILING_OBJECT_"
#define FINAL_PREFIX_FOR_LINKED_IN_SP_OBJECT PREFIX_FOR_LINKED_IN_SP_OBJECT SP_IDENTIFIER_PREFIX

void SelfProfilingInit(t_object* obj, t_string profiling_object_path);
void CfgAddSelfProfiling (t_object* obj, t_string output_name);
void CfgReadBlockExecutionCounts (t_cfg * cfg, t_string name);
void CfgComputeHotBblThreshold (t_cfg * cfg, double weight_threshold);
void CfgEstimateEdgeCounts (t_cfg * cfg);
t_int64 CfgComputeWeight (t_cfg * cfg);
void CfgReadInsExecutionCounts (t_cfg * cfg, t_string name);
t_bool BblIsHot (t_bbl * bbl);
t_bool FunIsHot (t_function * fun);
t_bool BblIsFrozen (t_bbl * bbl);
t_bool FunIsFrozen (t_function * fun);
t_bool BblIsAlmostHot (t_bbl * bbl);
t_bool EdgeIsHot (t_cfg_edge * edge);
double ORDER(t_int64 arg);

void CfgAssignUniqueOldAddresses(t_cfg * cfg);
void CfgResetOldAddresses(t_cfg * cfg);
#endif /* DIABLO_PROFILE_FUNCTIONS */
#endif /* DIABLOFLOWGRAPH_FUNCTIONS */
