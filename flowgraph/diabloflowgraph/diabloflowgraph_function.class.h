/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS function
#define function_field_select_prefix FUNCTION
#define function_function_prefix Function
#define MANAGER_TYPE t_cfg * 
#define MANAGER_NAME cfg
#define MANAGER_FIELD function_manager
#endif

/*! \brief Functions
 *
 * This class is used to represent the functions inside a cfg (t_cfg) of a
 * program. Functions in Diablo are sets of blocks, with a unique (virtual)
 * exit block (i.e. t_function::bbl_last). There are two main reason to have them:
 * collect information about them to threat callees as atomic
 * operations in the analysis of a caller, and to structure the
 * algorithms and our reasoning about transformations. As functions are also
 * stored as nodes in the callgraph (t_cg), this class extends
 * t_node. */

DIABLO_CLASS_BEGIN
EXTENDS(t_node)
/*! The cfg to which this function belongs */ 
MEMBER(t_cfg *,cfg,CFG)
/*! The name of this functions */
MEMBER(t_string,name,NAME)
/*! The next function in the list of all functions in the cfg. Mind that
 * "next" was not available, because nodes have a next field. */
MEMBER(struct _t_function *,fnext,FNEXT) 
/*! The previous function in the list of all functions in the cfg. Mind that
 * "prev" was not available, because nodes have a prev field. */
MEMBER(struct _t_function *,fprev,FPREV)
/*! The next hell function. This field implements a singly-linked list of 
 * hell functions per cfg */
MEMBER(struct _t_function *,nexthell,NEXT_HELL)
/*! Pointer to the next function that is marked. Field is useful to remove
 * unreachable functions for instance */
MEMBER(t_function *,next_marked,NEXT_MARKED)
/*! Pointer to the next marked edge, which will be a fake element or null. If it
 * is the fake element, we are in the list. */
MEMBER(t_cfg_edge *,marked_edges,MARKED_EDGES)
/*! Pointer to the next marked bbl. Field is useful to remove dead code and data */
MEMBER(t_bbl *,marked_bbls,MARKED_BBLS)
/*! Flags of the function */
IMEMBER(t_uint32,flags,FLAGS)       
/*! The first basic block of this function. */
MEMBER(t_bbl *,bbl_first,BBL_FIRST)
/*! The last basic block of this function. If the function has a dummy exit
 * block bbl_last must be set to this dummy exit block.  However, as not all
 * functions have a dummy exit block, you need to use FunctionGetExitBlock()
 * to obtain the dummy exit block of a function.  */
MEMBER(t_bbl *,bbl_last,BBL_LAST)
/*! Used for liveness analysis. Set of registers that are live through the function.
 * Should be a dynamic member in liveness analysis TODO */
MEMBER(t_regset,regs_through,REGS_THROUGH)
/*! Saved registers of the function */
MEMBER(t_regset,regs_saved,REGS_SAVED)
/*! The set of registers that are changed by the instructions of the function*/
MEMBER(t_regset,regs_changed,REGS_CHANGED)
/*! The set of registers that are used by instructions in the function */
MEMBER(t_regset,regs_used,REGS_USED)
MEMBER(t_regset,arg_regs,ARG_REGS)
MEMBER(t_regset,ret_regs,RET_REGS)
MEMBER(t_regset,regs_overwritten,REGS_OVERWRITTEN)
/*! The number of basic blocks in the function */
MEMBER(t_uint32,nr_blocks,NR_BLOCKS)

/*! Temp field. At the moment only used in FIT. Should be a dynamic member in
 * FIT and removed here TODO */
MEMBER(void *,tmp,TMP)

/*! Size of the function */
MEMBER(t_uint32,size,SIZE)
/*! Useful for dominator analysis. Should be a dynamic member and removed
 * here TODO */
MEMBER(t_uint32,min_dfs,MIN_DFS)
MEMBER(t_bool,behaves,BEHAVES)
/* An id that uniquely identifies each function, and can be used to order functions */
MEMBER(t_uint32, id, ID)

/*!
 * \brief Create dummy exit blocks for a function
 *
 * \param p1 The function for which we wish to create a dummy exit block. 
 * 
 * This function traverses the input function and creates a list of all return blocks of
 * that function. Once all return blocks are found, a dummy return block is added that
 * becomes the target of all the other return blocks. This is an internal
 * function used during flowgraph construction.  */

FUNCTION1(t_uint32, MoveReturnEdgesToReturnBlock, t_CLASS *)

/*! 
 * \brief Assign basic blocks to their functions.
 * 
 * \param p1 The function for which we want to assign basic blocks.
 * 
 * Traverse the basic blocks of a function and store in each basic block 
 * the function to which it belongs. Fatal error, if a basic block 
 * belongs to 2 different functions. */

FUNCTION1(void, AssociateBbls, t_CLASS *)

/*! \brief Make the return edges of a function point to the right blocks. 
 *
 * This is an internal function used during flowgraph construction. When
 * creating the cfg, it is not easy to add the correct return edges as
 * function detection is not yet performed. So during flowgraph construction
 * "dummy" return edges are added to HELL. When function detection is done,
 * this function is called to make the return edges of a function point to the
 * right blocks. */

FUNCTION1(t_uint32, AdjustReturnEdges, t_CLASS *)

/*! \brief Return the exit block of a function if it exists.
 *
 * \param p1 The function from which we want the exit block.
 *
 * \return The dummy exit block of a function, or NULL if this block has been
 * removed.
 * 
 * Returns the dummy exit block of a function, or NULL if this block has been
 * removed. The dummy exit block is only removed when the function cannot
 * return (e.g. because it calls exit or because it contains an endless loop).
 * */

FUNCTION1(t_bbl *, GetExitBlock, t_CLASS *)

/*! \brief Check whether a global function respects the calling conventions.
 *
 * \param p1 The function to check.
 *
 * \return FALSE if the function is known as a unbehaved function, else
 * TRUE
 * 
 * Look up the function in a list of unbehaved functions, return FALSE if it
 * is found. These unbehaved functions are (global) functions that do not
 * respect the calling conventions. Typically all (global) functions respect
 * calling conventions, except for some special functions introduced by the
 * compiler (e.g. to implement floating point arithmetic on architectures that
 * do not have hardware floating point support).  */

FUNCTION1(t_bool,Behaves, t_CLASS *)

/*!
 * \brief Create a new function 
 * 
 * \param p1 the entrypoint for the function 
 * \param p2 the functions name
 * \param p3 the type of the function (FT_NORMAL or FT_HELL) 
 *
 * \return a new function 
 *
 * constructor for the t_function structure: allocates memory for a function
 * node and fills in the appropriate default values.
 *
 * \todo Shouldn't this be FunctionCreate?
 */

FUNCTION3(t_CLASS *, Make, t_bbl *, t_const_string, t_function_type)

/*!  
 *
 * \param p1 The function to kill
 *
 * Destructor. Removes the function from the flowgraph. Basic blocks are not
 * freed, this should be done separate */

FUNCTION1(void, Kill, t_CLASS *)

/*! 
 *
 * \param p1 The function to check 
 *
 * Check whether the function is called by hell
 */

FUNCTION1(t_bool,IsCalledByHell,t_CLASS *)

/*!
 *
 * \param p1 The function to unlink
 *
 * Unlink this function from the CFG.
 */
FUNCTION1(void, UnlinkFromCfg, t_CLASS *)

/*!
 *
 * \param p1 The CFG where the function will be inserted in
 * \param p2 The function to insert
 *
 * Insert this function in the CFG.
 */
FUNCTION2(void, InsertInCfg, t_cfg *, t_CLASS *)

/*! 
 *
 * \param p1 The function to check
 *
 * Get an estimate for the heat of a function */
     
FUNCTION1(t_uint32,GetHeat, t_CLASS *)

CONSTRUCTOR({  /* insert the function in the dll */
  FunctionInsertInCfg (cfg, ret);
  FUNCTION_SET_RET_REGS(ret, CFG_DESCRIPTION(cfg)->return_regs);
  FUNCTION_SET_ARG_REGS(ret, CFG_DESCRIPTION(cfg)->argument_regs);
  FUNCTION_SET_BEHAVES(ret, TRUE);
  FUNCTION_SET_ID(ret, function_global_id++);
})
DESTRUCTOR({ FunctionUnlinkFromCfg ((t_CLASS *)to_free); if (FUNCTION_NAME(to_free)) Free(FUNCTION_NAME(to_free));  FunctionFreeMarkedSpace((t_CLASS *)to_free); })
DIABLO_CLASS_END

#define renamed_t_edge t_cg_edge
#define renamed_t_graph t_cg
#undef BASECLASS
#define BASECLASS node
#include  <diablosupport_node.class.h>
#undef BASECLASS
#undef renamed_t_edge
#undef renamed_t_graph
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
