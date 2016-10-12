#include <diablosupport.h>

#ifndef MY_GROUP_CG_TYPEDEFS
#define MY_GROUP_CG_TYPEDEFS
typedef struct _t_group_cg t_group_cg;
typedef struct _t_group_cg_edge t_group_cg_edge;
typedef struct _t_group_cg_node t_group_cg_node;
typedef struct _t_cg_nodes t_cg_nodes;
#endif

#ifndef MY_CALLGRAPH_H
#define MY_CALLGRAPH_H

#define CG_NODE_FUN(node) T_FUN(node)
#define T_CG_EDGE(edge) ((t_cg_edge*)(edge))
#define T_CG_NODE(node) ((t_cg_node*)(node))
#define T_CG(cg) ((t_cg*)(cg))
#endif

#include "diabloflowgraph.h"

#ifdef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLOFLOWGRAPH_CALLGRAPH_FUNCTIONS
#define DIABLOFLOWGRAPH_CALLGRAPH_FUNCTIONS
t_cg *CgNew ();
void CgFree (t_cg * cg);
void CgAddCfgEdge (t_cg * cg, t_cfg_edge * edge);
void CgBuild (t_cfg * cfg);
void CgExport (t_cg * cg, const char *name);
void MarkCallerChain (t_cfg * cfg, t_function * fun);
t_bool FunctionIsReentrant (t_function * fun);
t_int32 IndicateAllReachableFunctions (t_function * fun);
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
