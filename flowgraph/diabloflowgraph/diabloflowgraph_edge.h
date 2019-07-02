/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Edge Defines {{{ */
#ifndef DIABLOFLOWGRAPH_EDGE_DEFINES
#define DIABLOFLOWGRAPH_EDGE_DEFINES
/* edge types (for graphs) */
#define ET_UNKNOWN (1U<<0) /**< anything that can't be classified immediately */
#define ET_FALLTHROUGH (1U<<1)
#define ET_CALL (1U<<2) /**< branch and link */
#define ET_RETURN (1U<<3) /**< mov pc,lr */
#define ET_JUMP (1U<<5) /**< branch without link */
#define ET_SWI (1U<<6) /**< SWI is like a function call but much more predictable */

#define ET_IPUNKNOWN (1U<<8)
#define ET_IPFALLTHRU (1U<<9)
#define ET_IPJUMP (1U<<11)
#define ET_COMPENSATING (1U<<13)
#define ET_SWITCH (1U<<14)
#define ET_IPSWITCH (1U<<15)
#define ET_POSTDOM (1U<<16) /**< special edges that are created just before postdominator analysis, and that HAVE TO BE REMOVED afterwards. They connect infinite loops with the unique exit node, to make postdominator analysis work */

#define EF_FROM_SWITCH_TABLE (1<<30)

#define EF_CORR_EDGE_IS_REACHABLE (1<<29)
#define EF_ALL_BLOCKS_REACHABLE_BETWEEN_ENTRY_AND_EXIT (1<<28)
#define EF_CAN_SKIP (1<<27)
#define EF_ADVANCED_FACTORING (1<<26)
#define EF_FAKE (1<<25)
#define EF_NEEDS_REDIRECT (1<<24)

#define ET_INTERPROC          (ET_IPSWITCH | ET_CALL | ET_RETURN | ET_SWI | ET_IPUNKNOWN | ET_IPFALLTHRU | ET_IPJUMP | ET_COMPENSATING)
#define ET_FORWARD_INTERPROC  (ET_IPSWITCH | ET_CALL |             ET_SWI | ET_IPUNKNOWN | ET_IPFALLTHRU | ET_IPJUMP )
#define ET_BACKWARD_INTERPROC (                        ET_RETURN |                                                     ET_COMPENSATING)
#define ET_FT_BLOCK_TRUE (ET_FALLTHROUGH | ET_UNKNOWN | ET_IPUNKNOWN | ET_IPFALLTHRU | ET_SWITCH | ET_IPSWITCH) /**< class of types which require that there is a fallthrough block */
#define ET_FT_BLOCK_CALL (ET_CALL | ET_SWI) /**< class of types which require that there is a fallthrough block */


#define T_CFG_EDGE(edge) ((t_cfg_edge *)(edge))

#define CfgEdgeIsInterproc(edge) (CFG_EDGE_CAT(edge) & ET_INTERPROC)
#define CfgEdgeIsForwardInterproc(edge) (CFG_EDGE_CAT(edge) & ET_FORWARD_INTERPROC)
#define CfgEdgeIsBackwardInterproc(edge) (CFG_EDGE_CAT(edge) & ET_BACKWARD_INTERPROC)
#define CfgEdgeIsFallThrough(edge) (CFG_EDGE_CAT(edge) & (ET_FALLTHROUGH|ET_IPFALLTHRU))
#define CfgEdgeMark(x) EdgeMark(T_EDGE(x))
#define CfgEdgeMarkInit() EdgeMarkInit()
#define CfgEdgeIsMarked(x) EdgeIsMarked(T_EDGE(x))
#define CfgEdgeUnmark(x) EdgeUnmark(T_EDGE(x))
#define CfgEdgeIsFake(x) (CFG_EDGE_FLAGS(x) & EF_FAKE)
#define CfgEdgeMarkFake(x) (CFG_EDGE_SET_FLAGS(x, CFG_EDGE_FLAGS(x) | EF_FAKE))
#define CfgEdgeNeedsRedirect(x) (CFG_EDGE_FLAGS(x) & EF_NEEDS_REDIRECT)
#define CfgEdgeMarkNeedsRedirect(x) (CFG_EDGE_SET_FLAGS(x, CFG_EDGE_FLAGS(x) | EF_NEEDS_REDIRECT))

#define EDGE_DYNAMIC_MEMBER(lcasename,name,ccname,type,defval) \
  static void Edge ## ccname ## Init(t_cfg_edge *edge, type *valp) { *valp = defval; } \
  static void Edge ## ccname ## Fini(t_cfg_edge *edge, type *valp) { } \
  static void Edge ## ccname ## Dup (t_cfg_edge *edge, type *valp) { *valp = defval; } \
  t_dynamic_member_info lcasename ## _l_edge_array; \
  DYNAMIC_MEMBER(cfg_edge, t_cfg *, lcasename ## _l_edge_array, type, lcasename, name, ccname, CFG_FOREACH_EDGE, Edge ## ccname ## Init, Edge ## ccname ## Fini, Edge ## ccname ## Dup)

#define EDGE_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Edge ## ccname ## Init(t_cfg_edge *edge, type *valp) initbody \
  static void Edge ## ccname ## Fini(t_cfg_edge *edge, type *valp) finibody \
  static void Edge ## ccname ## Dup (t_cfg_edge *edge, type *valp) dupbody \
  extern t_dynamic_member_info lcasename ## _g_edge_array; \
  DYNAMIC_MEMBER(cfg_edge, t_cfg *, lcasename ## _g_edge_array, type, lcasename, name, ccname, CFG_FOREACH_EDGE, Edge ## ccname ## Init, Edge ## ccname ## Fini, Edge ## ccname ## Dup)

#define EDGE_DYNAMIC_MEMBER_GLOBAL(lcasename,name,ccname,type,defval) \
  EDGE_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type, { *valp = defval; }, { }, { *valp = defval; })

#define EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(lcasename) \
  t_dynamic_member_info lcasename ## _g_edge_array
#define EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(lcasename) \
  lcasename ## _g_edge_array.init

#endif
/* }}} */
#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Edge Functions {{{ */
#ifndef DIABLOFLOWGRAPH_EDGE_FUNCTIONS
#define DIABLOFLOWGRAPH_EDGE_FUNCTIONS

/*! Constructors and destructors */
t_cfg_edge *CfgEdgeChangeReturnForCall (t_cfg_edge * call, t_bbl * ret);
t_cfg_edge *CfgEdgeCreateSwi (t_cfg * flowgraph, t_bbl * from, t_bbl * ret);
void CfgEdgeMoveToLastSucc (t_cfg_edge * fall);
void CfgEdgeMoveFirstToLastPred (t_cfg_edge * first_edge);
t_cfg_edge * TakenPath (t_bbl * bbl);
t_cfg_edge * FallThroughPath (t_bbl * bbl);
t_bool FtPath (t_bbl * i_bbl, t_bbl * j_bbl, t_cfg_edge * ignore_edge);
t_string CfgEdgeTypeToString(t_cfg_edge * edge);
/* kill an edge, but not its corresponding relocation if any */
void CfgEdgeKillKeepRel (t_cfg_edge * edge);
/* change the tail of an existing edge (updating predecessor/successor links) */
void CfgEdgeChangeTail(t_cfg_edge * edge, t_bbl *new_tail);
void CfgEdgeChangeHead(t_cfg_edge * edge, t_bbl *new_head);
#endif
/* }}} */
#endif
extern t_uint32 cfg_edge_global_id;
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
