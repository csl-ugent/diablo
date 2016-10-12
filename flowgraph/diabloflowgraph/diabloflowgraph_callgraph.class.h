#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS cg
#define cg_field_select_prefix CG
#define cg_function_prefix Cg
#endif

/*! \brief This class is used to represent the callgraph of a program. */
DIABLO_CLASS_BEGIN
EXTENDS(t_graph)
/*! Pointer to the control flow graph. The call graph is derived from this cfg */
MEMBER(t_cfg *, cfg, CFG)
DIABLO_CLASS_END
#define renamed_t_node t_function
#define renamed_t_edge t_cg_edge
#undef BASECLASS
#define BASECLASS graph
#include <diablosupport_graph.class.h>
#undef BASECLASS
#undef renamed_t_node
#undef renamed_t_edge
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
