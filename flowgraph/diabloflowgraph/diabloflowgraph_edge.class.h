/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS cfg_edge
#define cfg_edge_field_select_prefix CFG_EDGE
#define cfg_edge_function_prefix CfgEdge
#define MANAGER_TYPE t_cfg *
#define MANAGER_NAME cfg
#define MANAGER_FIELD edge_manager
#endif

/*! \brief This class is used to represent the edges inside a cfg of a program. */
DIABLO_CLASS_BEGIN
EXTENDS(t_edge)
/*! The cfg to which this cfg_edge belongs. */
MEMBER(t_cfg *, cfg, CFG)
/*! cfg edge specific flags, like EF_FROM_SWITCH_TABLE. You can use this field
 * to add you own analysis specific flags */
IMEMBER(t_uint32, flags, FLAGS)
/*! Linked list of edges in the function it belongs to that are marked, is used
 * in FunctionMarkEdge, FunctionUnmarkEdge, etc. */
MEMBER(t_cfg_edge *, next_marked_into_fun, NEXT_MARKED_INTO_FUN)
/*! Keeps track of the registers that should be propagated over this edge in constant
 * propagation */
MEMBER(t_regset, prop_regs, PROP_REGS)
/*! Refcount reduces memory usage of edges by keeping a count instead of
 * allocating multiple edges with the same head, tail and type */
MEMBER(t_uint32, refcount, REFCOUNT)
//MEMBER(t_uint32, n_unevaluated_ins, N_UNEVALUATED_INS)
/*! For switch edges, this field holds the relocation that is responsible for the
 * creation of this edge */
MEMBER(t_reloc *, rel, REL)
/*! For switch edges, this field holds which case this edge is representing */
MEMBER(t_uint32, switchvalue, SWITCHVALUE)
/*! Holds an execution count when (instruction)profiles are available */
MEMBER(t_int64, exec_count, EXEC_COUNT)
/* An id that uniquely identifies each edge, and can be used to order edges */
IMEMBER(t_uint32, id, ID)
/*! In the case where from the blockprofiles it is not clear how many times this
 * edge has been followed, this field gives the minimal execution frequency */
MEMBER(t_int64, execution_count_min, EXEC_COUNT_MIN)
/*! In the case where from the blockprofiles it is not clear how many times this
 * edge has been followed, this field gives the maximal execution frequency */
MEMBER(t_int64, execution_count_max, EXEC_COUNT_MAX)
/*! \todo move to dynamic member, only used in dominators */
MEMBER(t_uint32, following_calls, FOLLOWING_CALLS)
/*!
 * \brief The default destructor
 *
 * \param p1 The cfg_edge to kill
 * */
FUNCTION1 (void, Kill, t_CLASS *)
/*!
 * \brief Constructor to create a compensating edge.
 *
 * \param p1 The cfg this cfg_edge belongs to 
 * \param p2 An interprocedural edge for which you want to add a compensating
 * edge.
 *
 * \return The compensating edge.
 *
 * The compensating edge will refer to the ip edge and vice versa with the
 * t_cfg_edge::corr field.
 * Example: when you create an edge of type ET_IPJUMP, you should also
 * create a compensating with function. The compensating edge is drawn between
 * the exit blocks of the function where the head and tail of p2 are in.
 *
 * \todo remove the first argument, it can be extracted from the second argument
 * */
FUNCTION2 (t_CLASS *, CreateCompensating, t_cfg *, t_CLASS *)
/*!
 * \brief Constructor for all edges except of type call, return and compensating
 *
 * \param p1 The cfg, this argument is redundant, should be removed
 * \param p2 The starting block of the edge, we call it the head (as there can
 * be confusion with this)
 * \param p3 The target block of the edge, we call it the tail
 * \param p4 The type of the edge, see e.g. ET_JUMP
 *
 * \return  an edge between head and tail of the given type */
FUNCTION4 (t_CLASS *, Create, t_cfg *, t_bbl *, t_bbl *, t_uint32)
/*!
 * \brief Creates a call and return edge pair.
 *
 * \param p1 The cfg, this argument is redundant, should be removed
 * \param p2 The t_bbl representing the call site
 * \param p3 The t_bbl representing the first block of the called function
 * \param p4 The t_bbl representing the return site (can be NULL)
 * \param p5 The t_bbl representing the exit block of the called function (can
 * be NULL)
 * \return The call edge. The corresponding return can be accessed via
 * t_cfg_edge::corr
 *
 * This constructor creates a call edge between p2 and p3. If p4 is NULL, then
 * the return site is set to CFG_EXIT_HELL_NODE. If p5 is NULL, then the exit
 * block is assumed to be CFG_EXIT_HELL_NODE. When flowgraphing a t_section,
 * p5 will be NULL, because no exit blocks are now at that stage.
 * Later on, a pass will patch the return edges so that they will point between
 * the newly created exit block and the return site. */
FUNCTION5 (t_CLASS *, CreateCall, t_cfg *, t_bbl *, t_bbl *, t_bbl *, t_bbl *)

/*!
 * \brief Kill an edge and has some side-effects in the basic block where the
 * edge originated.
 *
 * Kill an edge and perform all actions necessary to make a consistent graph
 * afterwards. If code becomes unreachable, it is NOT killed as a side effect!
 * To do so, you could call CfgRemoveDeadCodeAndDataBlocks for instance. In
 * the case of killing a call or return, a new edge is created, if it does not
 * exist yet, that is a fallthrough edge connecting the call site with the
 * return site */
FUNCTION1 (t_bool, KillAndUpdateBbl, t_CLASS *)
 /*!
  * \brief Checked version of Create. Doesn't create an edge when layouting the
  * graph would become impossible.
  *
  * \param p1 The head of the edge
  * \param p2 The tail of the edge or the return site in case of a call edge
  * \param p3 The type of the edge
  * \param p4 The entry of the called function in case of a call edge
  * \param p5 The exit block of the called function in case of a call edge
  * \param p6 An edge which should be ignored when performing the validation
  * of the layout
  *
  * \return A t_cfg_edge when all checks have passed
  * Try to add an edge of type p3 between p1 and p2. This function
  * checks if it is possible to add the desired edge. If it is not possible,
  * because e.g. your create a circular fallthrough path or you end up with a
  * bbl that has two incoming fallthroughs, the function returns NULL.
  * If creating the edge is possible, it is returned.
  * In the case where the type is a call edge, p1 is the call site, p2 is the
  * returnsite, p4 is the entry of the called function, p5 is the exit block of the called
  * function. The last argument can be an edge that should be ignored when
  * performing the checks. This ignored edge MUST be deleted if creating the
  * new edge succeeds.
  *
  * */
FUNCTION6 (t_CLASS *, CreateChecked, t_bbl *, t_bbl *, t_uint32, t_bbl *, t_bbl *, t_CLASS *)

CONSTRUCTOR3 (t_bbl *, from, t_bbl *, to, t_uint32, type,
              {
              CfgInitEdge (cfg, ret, from, to, type);
              CFG_EDGE_SET_REFCOUNT(ret, 1);
              CFG_EDGE_SET_ID(ret, cfg_edge_global_id++);
              })

DUPLICATOR(
           {
           })

DESTRUCTOR(
           {
           ASSERT(CFG_EDGE_REFCOUNT(to_free) == 0, ("CfgEdge refcount is not zero. Somebody must call CfgEdgeFree instead of CfgEdgeKill!"));
           CfgUnlinkEdge (cfg, (t_CLASS *)to_free);
           })

DIABLO_CLASS_END
#define renamed_t_node t_bbl
#define renamed_t_graph t_cfg
#undef BASECLASS
#define BASECLASS edge
#include <diablosupport_edge.class.h>
#undef BASECLASS
#undef renamed_t_node
#undef renamed_t_graph
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
