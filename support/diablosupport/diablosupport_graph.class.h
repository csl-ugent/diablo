/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS graph
#define graph_field_select_prefix GRAPH
#define graph_function_prefix Graph
#endif

/*! \brief A graph.
 *
 * This is the general abstraction for graphs in Diablo. This graph structure
 * can be iterated both by walking over the nodes and the edges in the graph.
 * Nodes are of type t_node and edges of type t_edge. */
DIABLO_CLASS_BEGIN
/*! \brief the number of nodes in a graph */
MEMBER(t_uint32, nnodes, NNODES)
/*! \brief the number of edges in a graph */
MEMBER(t_uint32, nedges, NEDGES)
/*! \brief the size of an edge in the graph.
 *
 * The size of an edge in the graph. This property was used to allow the graph
 * to allocate memory for edges. However, currently, edges should be allocated
 * with an ...EdgeNew function. Avoid the use of this property as it will be removed
 * in a future release.  */
MEMBER(t_uint32, edge_size, EDGE_SIZE)
/*! \brief the size of an node in the graph.
 *
 * The size of an node in the graph. This property was used to allow the graph
 * to allocate memory for nodes. However, currently, nodes should be allocated
 * with a ...NodeNew function. Avoid the use of this property as it will be
 * removed in a future release.  */
MEMBER(t_uint32, node_size, NODE_SIZE)
/*! \brief pointer to the first node in the graph */
MEMBER(t_node *, node_first, NODE_FIRST)
/*! \brief pointer to the last node in the graph */
MEMBER(t_node *, node_last, NODE_LAST)
/*! \brief pointer to the first edge in the graph */
MEMBER(t_edge *, edge_first, EDGE_FIRST)
/*! \brief pointer to the last edge in the graph */
MEMBER(t_edge *, edge_last, EDGE_LAST)
PFUNCTION3 (void, Init, t_CLASS *, t_uint32, t_uint32)
FUNCTION2 (void, InitNode, t_CLASS *, t_node *)
FUNCTION5 (void, InitEdge, t_CLASS *, t_edge *, t_node *, t_node *, t_uint32)
/*! \brief Create a new edge (you need to provide two nodes, as edges always
 * need to be initialized
 *
 * \warning This function is private for a reason: CfgNewEdge should call
 * CfgEdgeNew (the constructor of t_cfg_edge). It should not just allocate
 * memory as NewEdge would do. */

PFUNCTION4 (t_edge *, NewEdge, t_CLASS *, t_node *, t_node *, t_uint32)

/*! \brief Create a new node
 *
 * \warning This function is private for a reason: CfgNewNode should call
 * BblNew (the constructor of t_bbl). It should not just allocate
 * memory as NewNode would do. */
	
PFUNCTION2 (t_node *, NewNode, t_CLASS *, t_uint32)
FUNCTION2 (void, RemoveEdge, t_CLASS *, t_edge const *)
FUNCTION2 (void, UnlinkEdge, t_CLASS *, t_edge *)
FUNCTION2 (void, UnlinkEdgeFromGraph, t_CLASS *, t_edge const *)
FUNCTION1 (void, UnlinkEdgeFromNodes, t_edge const *)
FUNCTION2 (void, InsertEdgeInGraph, t_CLASS *, t_edge *)
FUNCTION3 (void, RemoveAllEdgesFromTo, t_CLASS *, t_node *, t_node *)
FUNCTION2 (void, RemoveNode, t_CLASS *, t_node const *)
/*! \brief Create an edge between two nodes 
 *
 * \warning This function is private for a reason: CfgAppendNode should call
 * CfgEdgeNew (the constructor of t_cfg_edge). It should not just allocate
 * memory as CfgAppendNode would do */
PFUNCTION4 (t_edge *, AppendNode, t_CLASS *, t_node *, t_node *, t_uint32)
FUNCTION2 (void, UnlinkNodeFromGraph, t_CLASS *, t_node const *)
FUNCTION2 (void, UnlinkNode, t_CLASS *, t_node *)
FUNCTION2 (void, InsertNodeInGraph, t_CLASS *, t_node *)
PFUNCTION2 (t_CLASS *, New, t_uint32, t_uint32)
FUNCTION2 (void, Order, t_CLASS *, NodeCmpFun)
DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
