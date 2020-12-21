/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*! \file Basic Blocks
 *
 * Basic blocks. This file holds the typedefs, types, function, and defines
 * needed to handle and manipulate them.
 */

/* Bbl Typedefs {{{ */
#ifndef DIABLOFLOWGRAPH_BBL_TYPEDEFS
#define DIABLOFLOWGRAPH_BBL_TYPEDEFS
typedef struct _t_bbl_list t_bbl_list;
typedef struct _t_bbl_he t_bbl_he;
#endif
/* }}} */
/* Bbl Defines {{{ */
#ifndef DIABLOFLOWGRAPH_BBL_DEFINES
#define DIABLOFLOWGRAPH_BBL_DEFINES
#define AddressNullForBbl(bbl) AddressNullForCfg(BBL_CFG(bbl))
#define AddressNewForBbl(bbl,a) AddressNewForCfg(BBL_CFG(bbl),a)
/*! Basic block attributes */
/*! If set, this block must be placed at the base_address specified */
#define BBL_FIXED_ADDR 0x1
/*! If set, this block has been given an definitive address during placing */
#define BBL_EMITTED 0x2
/*! If set, this block contains an address producer */
#define BBL_ADDR_PROD 0x4
#define BBL_ENLARGED 0x8
/*! If set, this block is a data block */
#define BBL_IS_DATA 0x10
#define BBL_IS_INSTRUMENT_CODE 0x20
#define BBL_IS_FUNCTION_ENTRY 0x40
/*! if set, the block is an unreachable return block (e.g. from exit()) */
#define BBL_UNREACHABLE 0x80
#define BBL_DATA_POOL 0x100
#define BBL_DATA_POOL_MIGRATED 0x200
/*! block is function return block */
#define BBL_IS_EXITBLOCK 0x400
/*! useful for instrumentation (code address translation) */
#define BBL_IS_POSSIBLE_TARGET_OF_INDIRECT_CONTROL_FLOW 0x800
/*! block contains hand-written assembler code */
#define BBL_IS_HANDWRITTEN_ASSEMBLY 0x1000
#define BBL_IS_REACHABLE_FROM_EXIT 0x2000
#define BBL_CAN_BE_REACHED 0x4000
#define BBL_CAN_REACH_EXIT 0x8000
/*! block should always be considered reachable */
#define BBL_FORCE_REACHABLE 0x10000
/*! Block has only one pred-edge or one return and one link edge */
#define BBL_FIXED_DOMINATORS 0x20000
#define NEED_TO_REDO 0x40000
#define CAN_SKIP 0x80000
#define HAS_INCOMING_RETURN_EDGE 0x100000
#define HAS_OUTGOING_CALL_EDGE 0x100000
#define HAS_A_LOT_OF_INCOMING_EDGES 0x200000
#define HAS_A_LOT_OF_OUTGOING_EDGES 0x200000
#define BBL_HAS_MORE_THAN_ONE_INCOMING_DFS_EDGE 0x400000
#define BBL_HAS_MORE_THAN_ONE_OUTGOING_DFS_EDGE 0x800000
#define BBL_HAS_MORE_THAN_ONE_OUTGOING_PDFS_EDGE 0x400000
#define BBL_HAS_MORE_THAN_ONE_INCOMING_PDFS_EDGE 0x800000
#define BBL_IS_PREFERRED_FOR_DATA_POOLS 0x1000000
#define BBL_FORWARD_DATAPOOL 0x2000000
#define BBL_ADVANCED_FACTORING 0x4000000
#define BBL_AOP_SETTER 0x8000000
#define BBL_AOP_CLOUD 0x10000000
#define BBL_AOP_SELECT 0x20000000
/* the following does NOT mean that the bbl is an exit block of a leaf
 * function. See the comments in CfgDetectPseudoLeaves for more information
 */
#define IS_EXIT_OF_LEAF 0x1000000


/* types of call hell nodes */
/* regular call hell node */
#define BBL_CH_NORMAL  0x1
/* dynamic call hell node (may require the GOT register to stay live) */
#define BBL_CH_DYNCALL 0x2

#define T_BBL(x) ((t_bbl *) x)
#define BblMark(x) NodeMark(T_NODE(x))
#define BblUnmark(x) NodeUnmark(T_NODE(x))
#define BblMarkInit() NodeMarkInit()
#define BblIsMarked(x) NodeIsMarked(T_NODE(x))
#define BblMark2(x) NodeMark2(T_NODE(x))
#define BblUnmark2(x) NodeUnmark2(T_NODE(x))
#define BblMarkInit2() NodeMarkInit2()
#define BblIsMarked2(x) NodeIsMarked2(T_NODE(x))
#define BblIsExitBlock(x) (BBL_ATTRIB(x) & BBL_IS_EXITBLOCK)

#define HAS_FIXED_ADDR(bbl) (T_BBL(bbl)->bbl_attributes & BBL_FIXED_ADDR)
#define IS_EMITTED(bbl) (T_BBL(bbl)->bbl_attributes & BBL_EMITTED)
#define HAS_ADDR_PROD(bbl) (T_BBL(bbl)->bbl_attributes & BBL_ADDR_PROD)
#define IS_ENLARGED(bbl) (T_BBL(bbl)->flags & BBL_ENLARGED)
#define BBL_PREV_DATA_POOL(bbl) (BBL_ATTRIB(bbl) & BBL_DATA_POOL)

#define BBL_DESCRIPTION(bbl) (CFG_DESCRIPTION(BBL_CFG(bbl)))

#define IS_DATABBL(bbl) ((BBL_INS_FIRST(bbl)!=NULL)?INS_TYPE(BBL_INS_FIRST(bbl)) == IT_DATA : 0)
#define IS_SWITCH_TABLE(bbl) ((BBL_INS_FIRST(bbl)!=NULL)?INS_ATTRIB(BBL_INS_FIRST(bbl)) & IF_SWITCHTABLE : 0)

#define BBL_IS_LAST(ibbl) (FUNCTION_BBL_LAST(BBL_FUNCTION(ibbl))==T_BBL(ibbl))
#define BBL_IS_FIRST(ibbl) (FUNCTION_BBL_FIRST(BBL_FUNCTION(ibbl))==T_BBL(ibbl))
#define BBL_DATA_POOL_IS_MIGRATED(i_bbl) ((BBL_ATTRIB(i_bbl)) & BBL_DATA_POOL_MIGRATED)

#define BBL_DYNAMIC_MEMBER_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Bbl ## ccname ## Init(t_bbl *bbl, type *valp) initbody \
  static void Bbl ## ccname ## Fini(t_bbl *bbl, type *valp) finibody \
  static void Bbl ## ccname ## Dup (t_bbl *bbl, type *valp) dupbody \
  t_dynamic_member_info lcasename ## _l_bbl_array; \
  DYNAMIC_MEMBER(bbl, t_cfg *, lcasename ## _l_bbl_array, type, lcasename, name, ccname, CFG_FOREACH_BBL, Bbl ## ccname ## Init, Bbl ## ccname ## Fini, Bbl ## ccname ## Dup)

#define BBL_DYNAMIC_MEMBER(lcasename,name,ccname,type,defval) \
  BBL_DYNAMIC_MEMBER_BODY(lcasename,name,ccname,type,{*valp=defval;},{},{*valp=defval;})

#define BBL_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Bbl ## ccname ## Init(t_bbl *bbl, type *valp) initbody \
  static void Bbl ## ccname ## Fini(t_bbl *bbl, type *valp) finibody \
  static void Bbl ## ccname ## Dup (t_bbl *bbl, type *valp) dupbody \
  extern t_dynamic_member_info lcasename ## _g_bbl_array; \
  DYNAMIC_MEMBER(bbl, t_cfg *, lcasename ## _g_bbl_array, type, lcasename, name, ccname, CFG_FOREACH_BBL, Bbl ## ccname ## Init, Bbl ## ccname ## Fini, Bbl ## ccname ## Dup)

#define BBL_DYNAMIC_MEMBER_GLOBAL(lcasename,name,ccname,type,defval) \
  BBL_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,{ *valp = defval; }, {}, { *valp = defval; })

#define BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(lcasename) \
  t_dynamic_member_info lcasename ## _g_bbl_array
#define BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(lcasename) \
  lcasename ## _g_bbl_array.init

#endif
/* }}} */
#include <diabloflowgraph.h>
#ifdef DIABLOFLOWGRAPH_TYPES
/* Bbl Types {{{ */
#ifndef DIABLOFLOWGRAPH_BBL_TYPES
#define DIABLOFLOWGRAPH_BBL_TYPES
struct _t_bbl_list
{
  struct _t_bbl_list *next;
  t_bbl *bbl;
};

struct _t_bbl_he
{
  t_hash_table_node node;
  t_bbl *bbl;
};
#endif
/* }}} */
#endif
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Bbl Functions {{{ */
#ifndef DIABLOFLOWGRAPH_BBL_FUNCTIONS
#define DIABLOFLOWGRAPH_BBL_FUNCTIONS

void BblSetDefinedRegs (t_bbl * bbl);
t_regset BblRegsDef (t_bbl * bbl);
t_regset BblRegsUse (t_bbl * bbl);
t_regset BblRegsLiveAfter (const t_bbl * bbl);
t_regset BblRegsLiveBefore (const t_bbl * bbl);
t_regset BblRegsMaybeDef (t_bbl * bbl);
t_regset BblRegsNeverLive(t_bbl * bbl);

void realBblInsertInFunction (const char *file, int lnno, t_bbl * bbl, t_function * func);

#define BblInsertInFunction(bbl,func) realBblInsertInFunction(__FILE__,__LINE__,bbl,func)
void BblMoveInstructionAfter (t_ins * move_ins, t_ins * dest_ins);
void BblMoveInstructionBefore (t_ins * move_ins, t_ins * dest_ins);
void BblSetAddressSuper (t_bbl * bbl, t_address address);
void BblSetAllAddressSuper (t_bbl * bbl, t_address old, t_address new_);
t_cfg_edge *BblGetSuccEdgeOfType (t_bbl * bbl, t_uint32 types);
t_cfg_edge *BblGetPredEdgeOfType (t_bbl * bbl, t_uint32 types);
t_bbl *BblSplitBlock (t_bbl * orig_bbl, t_ins * where, t_bool before);
t_bbl *BblSplitBlockNoTestOnBranches (t_bbl * orig_bbl, t_ins * where, t_bool before);
void BblUnlinkFromFunction (t_bbl * bbl);
void BblInsertInChainAfter(t_bbl *new_bbl, t_bbl *chain_bbl);
void BblInsertInChainBefore(t_bbl *new_bbl, t_bbl *chain_bbl);
#endif
/* }}} */
#endif
extern t_uint32 bbl_global_id;
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
