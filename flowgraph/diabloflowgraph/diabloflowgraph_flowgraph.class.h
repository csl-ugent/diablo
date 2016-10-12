/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */


#include <diablosupport_class.h>
#undef OBJECT
#ifndef CLASS
#define CLASS cfg
#define cfg_field_select_prefix CFG
#define cfg_function_prefix Cfg
#define MANAGER_TYPE t_object *
#define MANAGER_NAME object
#define MANAGER_FIELD cfg_manager
#endif

/*! \brief This class is used to represent the cfg of a program. */
DIABLO_CLASS_BEGIN
EXTENDS(t_graph)
/*! \brief The entry point of a library (e.g. for PE) or program.
 *
 * Does not include externally callable functions, contents of .init_array/
 * .ctors/etc. These are either reachable via hell edges or through
 * CFG_UNIQUE_ENTRY_NODE()
 */
MEMBER(t_cfg_entry_bbl *, entry, ENTRY)
/*! \brief Pointer to the first function in the list of functions for the cfg. */
MEMBER(t_function *, function_first, FUNCTION_FIRST)
/*! \brief Pointer to the first function to instrument.
 *
 * \todo Remove (should be dynamic member of fit)
 */
MEMBER(t_function *, function_instrument_first, FUNCTION_INSTRUMENT_FIRST)
/*! \brief Pointer to the last function in the list of functions for the cfg. */
MEMBER(t_function *, function_last, FUNCTION_LAST)
/*! \brief Pointer to aux func marking begin and end of list of marked funcs */
MEMBER(t_function *, marked_funs, MARKED_FUNS)
/*! \brief Pointer to last function that was marked */
MEMBER(t_function *, last_marked_fun, LAST_MARKED_FUN)
/*! \brief Pointer to the section for which this cfg was created. */
MEMBER(t_section *, section, SECTION)
/*! \brief Pointer to the object for which this cfg was created. */
MEMBER(t_object *, object, OBJECT)
MEMBER(t_function *, hell_function, HELL_FUNCTION)
MEMBER(t_function *, call_hell_function, CALL_HELL_FUNCTION)
MEMBER(t_function *, wrap_function, WRAP_FUNCTION)
MEMBER(t_function *, swi_hell_function, SWI_HELL_FUNCTION)
MEMBER(t_function *, longjmp_hell_function, LONGJMP_HELL_FUNCTION)
/* singly linked list containing all hell functions, also those for dynamic
 * calls */
MEMBER(t_function *, hell_functions, HELL_FUNCTIONS)
MEMBER(t_bbl *, hell_node, HELL_NODE)
MEMBER(t_bbl *, call_hell_node, CALL_HELL_NODE)
MEMBER(t_bbl *, swi_hell_node, SWI_HELL_NODE)
MEMBER(t_bbl *, longjmp_hell_node, LONGJMP_HELL_NODE)
MEMBER(t_bbl *, exit_hell_node, EXIT_HELL_NODE)
MEMBER(t_bbl *, exit_call_hell_node, EXIT_CALL_HELL_NODE)
MEMBER(t_bbl *, exit_swi_hell_node, EXIT_SWI_HELL_NODE)
MEMBER(t_bbl *, exit_longjmp_hell_node, EXIT_LONGJMP_HELL_NODE)
MEMBER(t_bbl *, unique_entry_node, UNIQUE_ENTRY_NODE)
MEMBER(t_bbl *, unique_exit_node, UNIQUE_EXIT_NODE)
MEMBER(t_loop *, loop_first, LOOP_FIRST)
MEMBER(t_loop *, loop_last, LOOP_LAST)
MEMBER(t_cfg_exits *, exitblocks, EXITBLOCKS)
MEMBER(t_int64, hot_threshold_count, HOT_THRESHOLD_COUNT)
MEMBER(t_cg *, cg, CG)
/*! \brief Manager object for t_bbl. See \ref dynamic_members */
MEMBER(t_manager, bbl_manager, BBL_MANAGER)
/*! \brief Manager object for t_cfg_edge. See \ref dynamic_members */
MEMBER(t_manager, edge_manager, EDGE_MANAGER)
/*! \brief Manager object for t_ins. See \ref dynamic_members */
MEMBER(t_manager, ins_manager, INS_MANAGER)
/*! \brief Manager object for t_function. See \ref dynamic_members */
MEMBER(t_manager, function_manager, FUNCTION_MANAGER)
/*! \brief Manager object for codebytes. See \ref dynamic_members */
MEMBER(t_manager, codebyte_manager, CODEBYTE_MANAGER)
MEMBER(t_architecture_description *, description, DESCRIPTION)
FUNCTION1 (t_bool, RemoveDeadCodeAndDataBlocks, t_CLASS *)
FUNCTION1 (void, FreeData, t_CLASS *)
FUNCTION1 (void, SortEdges, t_CLASS *)
/*!
 *
 * \param p1 The flowgraph for which we want to create functions
 *
 * Iterates over flowgraph and constructs a linked list of all functions */
FUNCTION2 (t_uint32, CreateFunctions, t_CLASS *, t_bool)
/*! Allocate a new control flow graph for section p1. */
FUNCTION1 (t_CLASS *, Create, t_object *)
/*! Cfg reconstruction (Flowgraph) builds a cfg without functions (just basic
 * blocks). When function detection has found the functions in this graph this
 * function needs to be called. It checks all (normal) edges in the cfg to
 * see if they connect blocks in different functions with a
 * non-interprocedural edge. If so, it patches the type of the edge and adds a
 * compensating edge */
FUNCTION1 (t_uint32, PatchNormalEdges, t_CLASS *)
/*! Cfg reconstruction (Flowgraph) builds a cfg without functions (just basic
 * blocks). When function detection has found the functions in this graph this
 * function needs to be called. This function patches the return edges to the
 * function representation (uniq exit block)
 *
 * \todo Change the name: PatchReturnEdges
 */
FUNCTION1 (t_uint32, PatchInterProcedural, t_CLASS *)
/*! \brief Add hell edges to the return points of calls to setjmp() and
 * friends
 *
 * In order to model the behaviour of setjmp() and longjmp() correctly, it is
 * necessary to add a hell edge to every program point longjmp() can jump to.
 * In the easiest case, this is the block following the call to setjmp(). It
 * gets slightly more complicated if setjmp() is called via a tail call. Then
 * we must follow the chain of compensating edges to find the real return
 * point. */
FUNCTION1 (t_uint32, PatchSetJmp, t_CLASS *)
/*! This function transforms the cfg to ensure that functions are single
 * entry. This is done by splitting off parts of a function that have an
 * incoming interprocedural jump */
FUNCTION1 (t_uint32, PatchToSingleEntryFunctions, t_CLASS *)

FUNCTION4 (void, CreateHellNodesAndEntry, t_object *, t_CLASS *, t_address, t_bbl *)

FUNCTION2 (void, ReplaceEntry, t_CLASS *, t_bbl *)

CONSTRUCTOR(
            {
              GraphInit ((t_graph *) ret, sizeof (t_bbl), sizeof (t_cfg_edge));
            }
           )
/*! Free a control flow graph */
DESTRUCTOR(
           {
             CfgFreeData ((t_CLASS *)to_free);
             OBJECT_SET_CFG(CFG_OBJECT(to_free), NULL);
           }
          )
DIABLO_CLASS_END
#define renamed_t_node t_bbl
#define renamed_t_edge t_cfg_edge
#undef BASECLASS
#define BASECLASS graph
#include <diablosupport_graph.class.h>
#undef BASECLASS
#undef renamed_t_node
#undef renamed_t_edge
#undef Node
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
