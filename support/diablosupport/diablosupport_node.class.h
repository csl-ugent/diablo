/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS node
#define node_field_select_prefix NODE
#define node_function_prefix Node
#endif

/*! \brief A node for a graph.
 *
 * This is the general abstraction for nodes in Diablo. Also see t_edge and
 * t_graph */
DIABLO_CLASS_BEGIN
/*! The next node in the linked list that holds all the nodes (even those not
 * held in the graph) */
MEMBER(t_CLASS *, next, NEXT)
/*! The previous node in the linked list that holds all the nodes (even those not
 * held in the graph) */
MEMBER(t_CLASS *, prev, PREV)
/*! The first outgoing edge */
MEMBER(t_edge *, succ_first, SUCC_FIRST)
/*! The last outgoing edge */
MEMBER(t_edge *, succ_last, SUCC_LAST)
/*! The first incoming edge */
MEMBER(t_edge *, pred_first, PRED_FIRST)
/*! The last incoming edge */
MEMBER(t_edge *, pred_last, PRED_LAST)
/*! Nodes are marked by assigning a number to them (this avoids unmarking
 * them). This field is used to store that number if the node is marked (first
 * mark), else it can hold any other number
 */
MEMBER(t_uint32, marked_number, MARKED_NUMBER)
/*! Nodes are marked by assigning a number to them (this avoids unmarking
 * them). This field is used to store that number if the node is marked
 * (second mark), else it can hold any other number
 */
MEMBER(t_uint32, marked_number2, MARKED_NUMBER2)
/*! \brief return TRUE if the first node is a predecessor of the second node.
 * \todo document the final argument */
FUNCTION3(t_bool, IsPred, t_CLASS const *, t_CLASS const *, t_uint32)
/*! \brief return TRUE if the first node is a successor of the second node.
 * \todo document the final argument */
FUNCTION3(t_bool, IsSucc, t_CLASS const *, t_CLASS const *, t_uint32)
DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
