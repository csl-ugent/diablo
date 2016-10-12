/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/*!\addtogroup DIABLO_ANALYSES Generic analysis */
/*@{ */
/*! \defgroup DOMINATOR Dominator analysis */
/*@{ */
#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLO_DOMINATOR_H
#define DIABLO_DOMINATOR_H
extern t_bool dominator_info_correct;
void ComDominators (t_cfg * cfg);
t_bool BblDominates (t_bbl * dominator, t_bbl * dominated);
t_bool BblPostdominates (t_bbl * postdominator, t_bbl * postdominated);
void Export_FunctionDominator (t_bbl * start, t_uint32 mask, const char *filename);
/*void Export_FunctionPostDominator (t_bbl * start, t_uint32 mask, const char *filename);*/
void DominatorCleanup (t_cfg * cfg);
void DominatorFree (t_bbl_list * list);
void DominatorDFSNumbering (t_cfg * cfg);

BBL_DYNAMIC_MEMBER_GLOBAL(next_in_dfs, NEXT_IN_DFS, NextInDfs, t_bbl *, NULL);
BBL_DYNAMIC_MEMBER_GLOBAL(stack_edge, STACK_EDGE, StackEdge, t_cfg_edge *, NULL);
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(dominates_array, DOMINATES_ARRAY, DominatesArray, t_set *, {*valp=SetNew(2);}, {SetFree(*valp);}, {*valp=NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(dom_marking_number, DOM_MARKING_NUMBER, DomMarkingNumber, t_uint32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(nr_dominated, NR_DOMINATED, NrDominated, t_uint32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(largest_pred_dfs, LARGEST_PRED_DFS, LargestPredDfs, t_uint32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(smallest_pred_dfs, SMALLEST_PRED_DFS, SmallestPredDfs, t_uint32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(min_dfs, MIN_DFS, MinDfs, t_uint32, {*valp=0;}, {}, {});

#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
