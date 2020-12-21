/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS bbl
#define bbl_field_select_prefix BBL
#define bbl_function_prefix Bbl
#define MANAGER_TYPE t_cfg *
#define MANAGER_NAME cfg
#define MANAGER_FIELD bbl_manager
#endif

/*! \brief This class is used to represents basic blocks.
 *
 * It extends t_relocatable, so it can be used as the target (or origin) of
 * relocations. It also extends t_node (as t_relocatable extend t_node). The
 * t_node superclass is used to store bbls in a control flowgraph (see t_cfg)
 * */

DIABLO_CLASS_BEGIN
EXTENDS(t_relocatable)
/*! The function to which this basic block belongs */
IMEMBER(t_function *, function, FUNCTION)
/*! The control flow graph to which this basic block belongs */
MEMBER(t_cfg *, cfg, CFG)
/*! Pointer to the next basic block in the function to which this basic block
 * belongs */
MEMBER(t_CLASS *, next_in_fun, NEXT_IN_FUN)
/*! Pointer to the prev basic block in the function to which this basic block
 * belongs */
MEMBER(t_CLASS *, prev_in_fun, PREV_IN_FUN)
/*! Pointer to the next marked basic block in the function to which this basic
 * block belongs */
MEMBER(t_CLASS *, next_marked_in_fun, NEXT_MARKED_IN_FUN)
/*! Pointer to the first instruction in this basic block */
IMEMBER(t_ins *, ins_first, INS_FIRST)
/*! Pointer to the last instruction in this basic block */
MEMBER(t_ins *, ins_last, INS_LAST)
/*! The number of instructions in this basic block */
IMEMBER(t_uint32, nins, NINS)
/*! A list of attributes for this basic block (as a bitvector) */
IMEMBER(t_uint32, attrib, ATTRIB)
/*! A pointer to an overlay definition, if the code in the basic block
 *  comes from an overlay */
MEMBER(t_overlay_sec *, overlay, OVERLAY)
/*! A set of registers that are live at the end of the basic block. This field
 * is used during liveness analysis*/
IMEMBER(t_regset, regs_live_out, REGS_LIVE_OUT)
/*! A set of registers that are used by the instructions in the basic block */
MEMBER(t_regset, regs_use, REGS_USE)
/*! A set of registers that are overwritten by the instructions in the basic block */
MEMBER(t_regset, regs_def, REGS_DEF)
/*! A set of registers that are defined in the basic block. This field is used
 * in forward liveness analysis */
MEMBER(t_regset, regs_defined_in, REGS_DEFINED_IN)
/*! A set of registers that are definitely overwritten by the instructions in 
 * the basic block */
MEMBER(t_regset, regs_def_certain, REGS_DEF_CERTAIN)
/*! A set of registers that could be overwritten by the instructions in the
 * basic block, but it is not certain. This field is used when the architecture 
 * conatains conditional instructions */
MEMBER(t_regset, regs_def_perhaps, REGS_DEF_PERHAPS)
/*! The register that cannot be live under any circumstances. These are
 * determined e.g. by using debug information */
IMEMBER(t_regset, regs_never_live, REGS_NEVER_LIVE)
/*! During layout basic blocks are stored in "chains", i.e. lists of basic
 * blocks that need to be place one after the other. This field is a pointer to
 * the previous basic block in the chain to which this basic block belongs */
IMEMBER(t_CLASS *, prev_in_chain, PREV_IN_CHAIN)
/*! During layout basic blocks are stored in "chains", i.e. lists of basic
 * blocks that need to be place one after the other. This field is a pointer to
 * the next basic block in the chain to which this basic block belongs */
IMEMBER(t_CLASS *, next_in_chain, NEXT_IN_CHAIN)
/*! During layout basic blocks are stored in "chains", i.e. lists of basic
 * blocks that need to be place one after the other. This field is a pointer to
 * the first basic block in the chain to which this basic block belongs */
MEMBER(t_CLASS *, first_in_chain, FIRST_IN_CHAIN)
/*! During layout basic blocks are stored in "chains", i.e. lists of basic
 * blocks that need to be place one after the other. This field is a pointer to
 * the last basic block in the chain to which this basic block belongs */
MEMBER(t_CLASS *, last_in_chain, LAST_IN_CHAIN)
/*! Depth First Search number is used in dominator analysis*/
MEMBER(int, dfs_number, DFS_NUMBER)
/*! list of basic blocks that are dominated by this basic block */
MEMBER(t_bbl_list *, dominated_by, DOMINATED_BY)
/*! \todo */
/*MEMBER(t_int32, pdfs_number, PDFS_NUMBER)*/
/*! \todo */
/*MEMBER(t_bbl_list *, p_dominated_by, P_DOMINATED_BY)*/
/*! When profile information is used, this field holds the number of times this
 * basic block was executed according to the profile */
MEMBER(t_int64, exec_count, EXEC_COUNT)
MEMBER(t_int64, sequence_id, SEQUENCE_ID)
/* An id that uniquely identifies each BBL, and can be used to order BBLs */
IMEMBER(t_uint32, id, ID)
/*! Bbl is part of a hell function */
IMEMBER(t_bool, is_hell, IS_HELL)
/*! Bbl is call-hell-like (IS_HELL should be set as well for these blocks)
 *    BBL_CH_NORMAL  - regular call hell
 *    BBL_CH_DYNCALL - dynamic call hell
 */
MEMBER(t_uint8, call_hell_type, CALL_HELL_TYPE)
/*! For layout of data blocks, we need to know whether or not the block needs
  to be allocated at a speficic offset of some alignment */
MEMBER(t_int32, alignment_offset, ALIGNMENT_OFFSET)
/*! Multifunctional field. Was used by all kinds of analysis and optimizations.
 * Now, it is deprecated, so avoid its use. Use DYNAMIC_MEMBERS instead
 *
 * \deprecated Use DYNAMIC_MEMBERS instead
 * */
MEMBER(void *, tmp, TMP)
/*! Multifunctional field. Was used by all kinds of analysis and optimizations.
 * Now, it is deprecated, so avoid its use. Use DYNAMIC_MEMBERS instead
 *
 * \deprecated Use DYNAMIC_MEMBERS instead
 * */
MEMBER(void *, tmp2, TMP2)

IMEMBER(t_regset, cached_regs_live_before, CACHED_REGS_LIVE_BEFORE)
IMEMBER(t_uint16, af_flags, AF_FLAGS)
IMEMBER(t_uint32, object_set, OBJECT_SET)
IMEMBER(t_uint32, original_function, ORIGINAL_FUNCTION)
/*!
 * \brief The default destructor
 *
 * \param p1 The basic block to kill
 * \todo remove: all functionality should move to BblFree (Destructor)
 */
FUNCTION1 (void, Kill, t_CLASS *)

CONSTRUCTOR(
            {
              CfgInitNode (cfg, ret);
              BBL_SET_RELOCATABLE_TYPE(ret, RT_BBL);
              BBL_SET_OLD_ADDRESS(ret,AddressNullForCfg(cfg));
              BBL_SET_OLD_SIZE(ret,AddressNullForCfg(cfg));
              BBL_SET_CADDRESS(ret,AddressNullForCfg(cfg));
              BBL_SET_CSIZE(ret,AddressNullForCfg(cfg));
              BBL_SET_IS_HELL(ret, FALSE);
              BBL_SET_ID(ret, bbl_global_id++);
              BBL_SET_AF_FLAGS(ret, 0);
              BBL_SET_OBJECT_SET(ret, -1);
              BBL_SET_SEQUENCE_ID(ret, -1);
            }
           )

DESTRUCTOR(
           {
             CfgUnlinkNode (cfg, (t_CLASS *)to_free);
             if (BBL_REFED_BY(to_free)) 
	       FATAL(("BBL @B Still refered by @R(rel->hell == %s)", to_free, RELOC_REF_RELOC(BBL_REFED_BY(to_free)), RELOC_HELL(RELOC_REF_RELOC(BBL_REFED_BY(to_free))) ? "TRUE" : "FALSE"));
           }
          )
DUPLICATOR(
           {
             t_ins * ins;
             t_ins * copy;
             memset (ret, 0, sizeof (t_bbl));
             BBL_SET_CFG(ret, cfg);
             CfgInitNode (cfg, ret);
             BBL_SET_RELOCATABLE_TYPE(ret, RT_BBL);
             BBL_SET_OLD_ADDRESS(ret,AddressNullForCfg(cfg));
             BBL_SET_OLD_SIZE(ret,AddressNullForCfg(cfg));
             BBL_SET_CADDRESS(ret,AddressNullForCfg(cfg));
             BBL_SET_CSIZE(ret,AddressNullForCfg(cfg));
             BBL_SET_NEXT_MARKED_IN_FUN(ret, NULL);
             BBL_SET_ID(ret, bbl_global_id++);
             BBL_SET_AF_FLAGS(ret, 0);
             BBL_SET_OBJECT_SET(ret, -1);
             BBL_FOREACH_INS(to_dup, ins)
             {
               copy = InsDup (ins);
               InsAppendToBbl (copy, ret);
             }
           }
          )
DIABLO_CLASS_END
#define renamed_t_edge t_cfg_edge
#define renamed_t_graph t_cfg
#undef BASECLASS
#define BASECLASS relocatable
#include <diabloobject_relocatable.class.h>
#undef BASECLASS
#undef renamed_t_edge
#undef renamed_t_graph
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
