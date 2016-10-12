/** \file diablosupport_graph.h
 *
 * Graph support
 */

#include <diablosupport.h>

/* Graph Globals {{{ */
#ifndef DIABLOSUPPORT_GRAPH_GLOBALS_H
#define DIABLOSUPPORT_GRAPH_GLOBALS_H
extern t_uint32 global_node_marking_number;
extern t_uint32 global_node_marking_number2;
extern t_uint32 global_edge_marking_number;
#endif /* Graph Globals }}} */
/* Graph Defines {{{ */
#ifndef DIABLOSUPPORT_GRAPH_DEFINES_H
#define DIABLOSUPPORT_GRAPH_DEFINES_H
/* Casts */
#define T_EDGE(x) ((t_edge *) (x))
#define T_NODE(x) ((t_node *) (x))
#define T_GRAPH(x) ((t_graph *) (x))
/* Edges function like macros */
#define EdgeClearCategory(edge,_category) (T_EDGE(edge)->category &= ~_category)
#define EdgeTestCategoryOr(edge,_category) (EDGE_CAT(edge) & (_category))
#define EdgeTestCategoryAnd(edge,_category) (T_EDGE(edge)->category & (_category) == (_category))
#define EdgeIsMarked(edge) (EDGE_MARKED_NUMBER(edge)==global_edge_marking_number)
#define EdgeMark(edge) (EDGE_SET_MARKED_NUMBER(edge, global_edge_marking_number))
#define EdgeUnmark(edge) (EDGE_SET_MARKED_NUMBER(edge, 0))

/* Edge function like macros */
#define NodeSetCategory(node,_category) (T_NODE(node))->category |= _category
#define NodeClearCategory(node,_category) (T_NODE(node))->category &= ~_category
#define NodeTestCategoryOr(node,_category) ((T_NODE(node))->category & (_category))
#define NodeTestCategoryAnd(node,_category) ((T_NODE(node))->category & (_category) == (_category))
#define NodeIsMarked(node) (NODE_MARKED_NUMBER(node)==global_node_marking_number)
#define NodeMark(node) (NODE_SET_MARKED_NUMBER(node,global_node_marking_number))
#define NodeUnmark(node) (NODE_SET_MARKED_NUMBER(node,0))

#define NodeIsMarked2(node) (NODE_MARKED_NUMBER2(node)==global_node_marking_number2)
#define NodeMark2(node) (NODE_SET_MARKED_NUMBER2(node, global_node_marking_number2))
#define NodeUnmark2(node) (NODE_SET_MARKED_NUMBER2(node,0))
/* Iterators */
#define GRAPH_FOREACH_NODE(graph,node) for (node=GRAPH_NODE_FIRST(graph); node!=NULL; node=NODE_NEXT(node))
#define GRAPH_FOREACH_NODE_SAFE(graph,node,tmp) for (node=GRAPH_NODE_FIRST(graph), tmp=node?NODE_NEXT(node):NULL; node!=NULL; node=tmp, tmp=node?NODE_NEXT(node):NULL)

#define GRAPH_FOREACH_EDGE(graph,edge) for (edge=GRAPH_EDGE_FIRST(graph); edge!=NULL; edge=EDGE_NEXT(edge))
#define GRAPH_FOREACH_EDGE_SAFE(graph,edge,tmp) for (edge=GRAPH_EDGE_FIRST(graph), tmp=edge?EDGE_NEXT(edge):NULL; edge!=NULL; edge=tmp, tmp=edge?EDGE_NEXT(edge):NULL)

#define NODE_FOREACH_SUCC_EDGE(node,edge) for (edge=NODE_SUCC_FIRST(node); edge!=NULL; edge=EDGE_SUCC_NEXT(edge))

#define NODE_FOREACH_SUCC_EDGE_SAFE(node,edge,tmp) for (edge=NODE_SUCC_FIRST(node), tmp=edge?EDGE_SUCC_NEXT(edge):NULL; edge!=NULL; edge= tmp, tmp=edge?EDGE_SUCC_NEXT(edge):NULL)

#define NODE_FOREACH_PRED_EDGE(node,edge) for (edge=NODE_PRED_FIRST(node); edge!=NULL; edge=EDGE_PRED_NEXT(edge))

#define NODE_FOREACH_PRED_EDGE_SAFE(node,edge,tmp) for (edge=NODE_PRED_FIRST(node), tmp=edge?EDGE_PRED_NEXT(edge):NULL; edge!=NULL; edge= tmp, tmp=edge?EDGE_PRED_NEXT(edge):NULL)
#endif /* }}} Graph Defines */
#ifdef DIABLOSUPPORT_FUNCTIONS
typedef t_int8 (*NodeCmpFun) (t_node * node1, t_node * node2);
typedef t_int8 (*renamed_NodeCmpFun) (t_node * node1, t_node * node2);

/* Graph Functions {{{ */
#ifndef DIABLOSUPPORT_GRAPH_FUNCTIONS
#define DIABLOSUPPORT_GRAPH_FUNCTIONS
void EdgeMarkInit (void);
void NodeMarkInit (void);
void NodeMarkInit2 (void);

void GraphDFTraversalWithCheckAndData (t_node *, t_bool (*CallBack) (t_node * curr, t_node * par, void *data), t_uint32, t_uint32, void *);
void GraphBFTraversal (t_node *, void (*CallBack) (t_node * curr, t_node * par), t_uint32 edge_category_or_mask);
void GraphDFReverseTraversalWithCheckAndData (t_node *, t_bool (*CallBack) (t_node * curr, t_node * par, void *data), t_uint32 edge_category_mask, void *data);
#endif /* }}} Graph Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
