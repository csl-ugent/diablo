#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS cg_edge
#define cg_edge_field_select_prefix CG_EDGE
#define cg_edge_function_prefix CgEdge
#endif

/*! \brief This class is used to represent the edges (i.e. calls, returns,
 * ip-edges and compensating edges) inside a callgraph of a program. */
DIABLO_CLASS_BEGIN
EXTENDS(t_edge)
/*! The call graph edge has a corresponding edge in the cfg. This field contains
 * a pointer to that edge */
MEMBER(t_cfg_edge *, cfg_edge, CFG_EDGE)
DIABLO_CLASS_END
#define renamed_t_node t_function
#define renamed_t_graph t_cg
#undef BASECLASS
#define BASECLASS edge
#include <diablosupport_edge.class.h>
#undef BASECLASS
#undef renamed_t_node
#undef renamed_t_graph
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
