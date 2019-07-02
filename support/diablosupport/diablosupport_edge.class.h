#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS edge
#define edge_field_select_prefix EDGE
#define edge_function_prefix Edge
#endif

/*! \brief An edge in a graph.
 *
 * This is the general abstraction for edges in Diablo. Also see t_node and
 * t_graph */
DIABLO_CLASS_BEGIN
/*! The node from which this edge originates */
MEMBER(t_node *, head, HEAD)
/*! The node to which this edge points */
IMEMBER(t_node *, tail, TAIL)
/*! The next edge in the list that holds all edges */
MEMBER(t_CLASS *, next, NEXT)
/*! The previous edge in the list that holds all edges */
MEMBER(t_CLASS *, prev, PREV)
/*! Pointer to the next outgoing edge of head */
MEMBER(t_CLASS *, succ_next, SUCC_NEXT)
/*! Pointer to the previous outgoing edge of head */
MEMBER(t_CLASS *, succ_prev, SUCC_PREV)
/*! Pointer to the next incoming edge of tail */
MEMBER(t_CLASS *, pred_next, PRED_NEXT)
/*! Pointer to the previous incoming edge of tail */
MEMBER(t_CLASS *, pred_prev, PRED_PREV)
/*! Field that can be used to express a correspondence relation between two
 * edges. This field stores the corresponding edge. */
MEMBER(t_CLASS *, corr, CORR)
/*! The category of the edge (a numeric value, identifying the type of the edge
 * */
IMEMBER(t_uint32, cat, CAT)
/*! Edges are marked by assigning a number to them (this avoids unmarking
 * them). This field is used to store that number if the edge is marked, else
 * it can hold any other number
 */
MEMBER(t_uint32, marked_number, MARKED_NUMBER)
DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
