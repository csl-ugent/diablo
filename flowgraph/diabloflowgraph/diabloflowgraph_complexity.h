/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLO_COMPLEXITY_H
#define DIABLO_COMPLEXITY_H

void CfgStaticComplexityInit(t_const_string fname);
void CfgStaticComplexityFini();
void CfgStaticComplexityOriginInit(t_const_string fname);
void CfgStaticComplexityOriginFini();
void CfgDynamicComplexityInit(t_const_string fname);
void CfgDynamicComplexityFini();
void CfgDynamicComplexityOriginInit(t_const_string fname);
void CfgDynamicComplexityOriginFini();

void CfgComputeStaticComplexity(t_cfg * cfg);
void CfgComputeStaticComplexityOrigin(t_cfg * cfg, size_t nr_archives, size_t nr_objects, size_t nr_functions);
void CfgComputeDynamicComplexity(t_cfg * cfg);

t_bool IsCfgEdgeDirect(t_cfg_edge* edge, t_bool are_switches_direct_edges);

void ComplexityInitTempInfo(t_cfg *cfg);
void ComplexityFiniTempInfo(t_cfg *cfg);
void ComplexityRecordTransformedInstruction(t_ins *to, t_ins *from);

#endif
#endif
