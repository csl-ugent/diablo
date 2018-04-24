/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*! Flowgraphs
 *
 * Flowgraphs are derived from graphs. Instead of nodes and edges,
 * they consist of basic blocks (t_bbl) and control flow edges
 * (t_cfg_edge). The graph themselves also contains procedures, and
 * the blocks and edges are partitioned into the procedures.
 */

/* Flowgraph Typedefs {{{ */
#ifndef DIABLOFLOWGRAPH_FLOWGRAPH_TYPEDEFS
#define DIABLOFLOWGRAPH_FLOWGRAPH_TYPEDEFS
typedef struct _t_cfg_entry_bbl t_cfg_entry_bbl;
#endif
/* }}} */
/* Flowgraph Defines {{{ */
#ifndef DIABLOFLOWGRAPH_FLOWGRAPH_DEFINES
#define DIABLOFLOWGRAPH_FLOWGRAPH_DEFINES
#define AddressNullForCfg(cfg) AddressNullForObject(CFG_OBJECT(cfg))
#define AddressNewForCfg(cfg,a) AddressNewForObject(CFG_OBJECT(cfg),a)
#define CfgEdgeTestCategoryOr(edge,_category) (CFG_EDGE_CAT(edge) & (_category))
#define T_CFG(cfg) ((t_cfg*)(cfg))
#define InitBblMarking NodeMarkInit
#define CfgAppendBbl(cfg,after, insert, edge_category) T_CFG_EDGE(GraphAppendNode(T_GRAPH(cfg), T_NODE(after), T_NODE(insert), edge_category))

#define OBJECT_CFG(obj)           ((t_cfg *) OBJECT_VCFG(obj))
#define OBJECT_SET_CFG(obj, cfg)  OBJECT_SET_VCFG(obj, (void *)cfg)

#define CFG_DYNAMIC_MEMBER(lcasename,name,ccname,type,defval) \
  static void Cfg ## ccname ## Init(t_cfg *cfg, type *valp) { *valp = defval; } \
  static void Cfg ## ccname ## Fini(t_cfg *cfg, type *valp) { } \
  static void Cfg ## ccname ## Dup (t_cfg *cfg, type *valp) { *valp = defval; } \
  t_dynamic_member_info lcasename ## _l_cfg_array; \
  DYNAMIC_MEMBER(cfg, t_object *, lcasename ## _l_cfg_array, type, lcasename, name, ccname, OBJECT_FOREACH_CFG, Cfg ## ccname ## Init, Cfg ## ccname ## Fini, Cfg ## ccname ## Dup)

#define CFG_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Cfg ## ccname ## Init(t_cfg *cfg, type *valp) initbody \
  static void Cfg ## ccname ## Fini(t_cfg *cfg, type *valp) finibody \
  static void Cfg ## ccname ## Dup (t_cfg *cfg, type *valp) dupbody \
  extern t_dynamic_member_info lcasename ## _g_cfg_array; \
  DYNAMIC_MEMBER(cfg, t_object *, lcasename ## _g_cfg_array, type, lcasename, name, ccname, OBJECT_FOREACH_CFG, Cfg ## ccname ## Init, Cfg ## ccname ## Fini, Cfg ## ccname ## Dup)

#define CFG_DYNAMIC_MEMBER_GLOBAL(lcasename,name,ccname,type,defval) \
  CFG_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,{*valp=defval;},{},{*valp=defval;})

#endif

#define CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(lcasename) \
  t_dynamic_member_info lcasename ## _g_cfg_array

/* }}} */
#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_TYPES
/* Flowgraph Types {{{ */
#ifndef DIABLOFLOWGRAPH_FLOWGRAPH_TYPES
#define DIABLOFLOWGRAPH_FLOWGRAPH_TYPES
/*! Describes the entry points for a cfg (linked list) */
struct _t_cfg_entry_bbl
{
  t_bbl *entry_bbl;
  t_cfg_edge *entry_edge;
  void *tmp;
};
#endif
/* }}} */
#endif
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Flowgraph Functions {{{ */
#ifndef DIABLOFLOWGRAPH_FLOWGRAPH_FUNCTIONS
#define DIABLOFLOWGRAPH_FLOWGRAPH_FUNCTIONS
t_uint32 CfgCreateBasicBlocks (t_object *obj);
t_bbl *CfgGetDynamicCallHell(t_cfg *cfg, t_string fname);
t_uint32 CfgCreateFunctions (t_cfg * fg, t_bool preserve_functions_by_symbol);
void CfgRemoveDeadCode (t_cfg * fg);
t_bool CfgRemoveUselessConditionalJumps (t_cfg * cfg);
t_bool CfgRemoveEmptyBlocks (t_cfg * cfg);
void CfgMoveInsDown (t_cfg * cfg);
t_bool CfgPatchForNonReturningFunctions (t_cfg * cfg);
void CfgPartitionFunctions (t_cfg * cfg,
                            t_bool (*IsStartOfPartition) (t_bbl *),
                            t_bool (*CanMerge) (t_bbl *, t_bbl *));
void Export_Flowgraph (t_bbl * start, t_uint16 mask, const char *filename);
void CfgPrintNrIns (t_cfg * cfg);
void CfgVerifyCorrectness (t_cfg * cfg);
void CfgDetectLeaves (t_cfg * cfg);

void CfgDrawFunctionGraphs (t_cfg * cfg, t_const_string dirprefix);
void CfgDrawFunctionGraphsAnnotated (t_cfg * cfg, t_const_string dirprefix, void (*fun_draw_graph) (t_function *, t_string));
void CfgDrawFunctionGraphsWithHotness (t_cfg * cfg, t_const_string dirprefix);
void CfgDrawFunctionGraphsWithLiveness (t_cfg * cfg, t_const_string dirprefix);

t_bool CfgDetectSimpleInfiniteLoops (t_cfg * cfg, t_loopref ** head);

t_bool BblIsSwiExitNode (t_bbl * node);

void MarkFrom (t_cfg * cfg, t_bbl * from);
void CfgMarkFun (t_cfg * cfg, t_function * fun);
t_bool CfgUnmarkFun (t_cfg * cfg, t_function ** fun);
void CfgUnmarkAllFun (t_cfg * cfg);

void FunctionMarkBbl (t_function * fun, t_bbl * bbl);
t_bool FunctionUnmarkBbl (t_function * fun, t_bbl ** bbl);
void FunctionUnmarkAllBbls (t_function * fun);

void FunctionMarkEdge (t_function * fun, t_cfg_edge * edge);
t_bool FunctionUnmarkEdge (t_function * fun, t_cfg_edge ** edge);
void FunctionUnmarkAllEdges (t_function * fun);
void FunctionMarkAllTrueIncomingEdges (t_function * fun);

void FunctionFreeMarkedSpace (t_function * fun);

t_uint64 SectionRecalculateSizeFlowgraphed (t_section *, t_relocatable_address_type);
t_uint64 SectionRecalculateSizeDeflowgraphing (t_section * sec, t_relocatable_address_type type);
t_bbl *CfgGetDynamicCallHell(t_cfg *cfg, t_string fname);
t_bool EdgeMakeInterprocedural(t_cfg_edge *edge);
t_bool EdgeMakeIntraProcedural(t_cfg_edge *edge);
void DrawFunctionGraphAnnotated (t_function * function, t_const_string dirprefix);
#endif
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
