/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Ins Typedefs {{{ */
#ifndef DIABLOFLOWGRAPH_INS_TYPEDEFS
#define DIABLOFLOWGRAPH_INS_TYPEDEFS
typedef t_uint32 t_instruction_flags, renamed_t_instruction_flags;
typedef struct _t_inslist t_inslist;
typedef enum
{ ALWAYS, NEVER, EQ, NE, LT, GT, LE, GE, UNKNOWN } EXEC_CONDITION;
#endif
/* }}} */
/* Ins Defines {{{ */
#ifndef DIABLOFLOWGRAPH_INS_DEFINES
#define DIABLOFLOWGRAPH_INS_DEFINES
/*! INS_ATTRIB Instruction flags */
/*! The instruction is actualy a data element stored in the code section */
#define IF_DATA 0x1
/*! The instruction is a bbl leader */
#define IF_BBL_LEADER 0x2
/*! The instruction is the start of a function, because there is a (function) symbol pointing to it or there are calls to this instruction */
#define IF_FUNCTION_ENTRY_OBSOLETE 0x4
/*! The instruction is contained in a basic block (depends on the current state of the object)*/
#define IF_IN_BBL 0x8
/*! The address of the instruction can be loaded or computed somewhere */
#define IF_ADDRESS_TAKEN 0x10
/*! Constant propagation thinks the instruction can be executed */
#define IF_EXECED 0x20
/*! Constant propagation thinks the instruction is always executed (this means that for a conditional instruction, the condition can be removed) */
#define IF_ALWAYS_EXECED 0x40
/*! The instruction is dead (it can be killed, but it wasn't possible to free it's memory yet) */
#define IF_DEAD 0x80
/*! The instruction defines the PC */
#define IF_PCDEF 0x100
/*! The instruction is actually a data element stored in the code section, and it is part of a switch table */
#define IF_SWITCHTABLE 0x200
/*! The instruction is conditional */
#define IF_CONDITIONAL 0x800
#define IF_DELAYSLOT 0x2000
#define IF_SWITCHJUMP 0x4000
/*! The instruction can be evaluated fast during constant propagation:
 * if any of its source operands are unknown, the result is
 * unknown */
#define IF_FAST_CP_EVAL 0x8000
/* Branch obfuscation added; attention when we want to give a new address to this instruction */
#define IF_BRANCH_OBF 0x10000
#define IF_ADDRESS_PRODUCER 0x20000
#define IF_ADDRESS_POOL_ENTRY 0x40000
#define IF_COMPILER_GENERATED 0x80000
#define IF_SWITCHJUMP_FIXEDCASESIZE 0x200000
#define IF_AF_INDEX_INSTRUCTION 0x400000
/* A flag useful if you want to mark some instruction during debugging */
#define IF_DEBUG_MARK 0x80000000

/*@}*/

/*! \defgroup IT_TYPES Generic Instruction types */
/*@{*/
/*! Instruction type is unknown */
#define IT_UNKNOWN 0
/*! Branch instructions */
#define IT_BRANCH 1
/*! Software interupts */
#define IT_SWI 2
/*! Normal dataprocessing instructions (add, sub, ...) */
#define IT_DATAPROC 3
/*! Multiplies */
#define IT_MUL 4
/*! Loads */
#define IT_LOAD 5
/*! Stores */
#define IT_STORE 6
/*! Multiple loads and stores */
#define IT_LOAD_MULTIPLE 7
#define IT_STORE_MULTIPLE 8
/*! Status register enquieries */
#define IT_STATUS 9
/*! Swap instructions */
#define IT_SWAP 10
/*! Data in the code section */
#define IT_DATA 11
/*! Address producers and constant producers */
#define IT_CONSTS 12
/*! No operation instruction */
#define IT_NOP 13
/*! Prefetch instruction */
#define IT_PREF 14
/*! Floating point ALU */
#define IT_FLT_ALU 15
/*! Floating point -> INS transfer*/
#define IT_FLT_INT 16
#define IT_FLT_LOAD 17
#define IT_FLT_STORE 18
#define IT_FLT_STATUS 19
#define IT_INT_FLT 20
#define IT_CALL 21
#define IT_PSEUDO_LOAD 22
#define IT_PSEUDO_SAVE 23

/* Operation for syncronization purposes */
#define IT_SYNC 24
/* Instruction and data cache management */
#define IT_CACHE 25
/* Divisions instructions */
#define IT_DIV 26
/* SIMD instructions i.e. Altivec */
#define IT_SIMD 27

#define AddressNullForIns(ins) AddressNullForSection(INS_SECTION(T_INS(ins)))
#define AddressNewForIns(ins,a) AddressNewForSection(INS_SECTION(T_INS(ins)),a)

/*! INS_SELECT Macro's to access fields of the generic instruction structure */
#define INS_OBJECT(x) (SECTION_OBJECT(INS_SECTION(x)))
/*! \defgroup INS_TYPE Shorthands to check the type of an instruction */
/*@{*/
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is unknown */
#define INS_IS_UNKNOWN(x) (INS_TYPE(x) == IT_UNKNOWN)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a branch */
#define INS_IS_BRANCH(x) (INS_TYPE(x) == IT_BRANCH)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a software interrupt */
#define INS_IS_SWI(x) (INS_TYPE(x) == IT_SWI)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is dataprocessing instruction */
#define INS_IS_DATAPROC(x) (INS_TYPE(x) == IT_DATAPROC)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is multiply instruction */
#define INS_IS_MUL(x) (INS_TYPE(x) == IT_MUL)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a load */
#define INS_IS_LOAD(x) (INS_TYPE(x) == IT_LOAD)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a store */
#define INS_IS_STORE(x) (INS_TYPE(x) == IT_STORE)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a load/store multiple */
#define INS_IS_LOAD_MULTIPLE(x) (INS_TYPE(x) == IT_LOAD_MULTIPLE)
#define INS_IS_STORE_MULTIPLE(x) (INS_TYPE(x) == IT_STORE_MULTIPLE)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a status instruction */
#define INS_IS_STATUS(x) (INS_TYPE(x) == IT_STATUS)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a swap instruction */
#define INS_IS_SWAP(x) (INS_TYPE(x) == IT_SWAP)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is a swap instruction */
#define INS_IS_DATA(x) (INS_TYPE(x) == IT_DATA)
/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the type of the instruction is an address producer */
#define INS_IS_CONSTS(x) (INS_TYPE(x) == IT_CONSTS)

/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the instruction is conditional */
#define INS_IS_CONDITIONAL(x) (INS_ATTRIB(x) & IF_CONDITIONAL)

/*! \param x An instruction (automaticaly casted to a generic instruction)
 * \return TRUE or FALSE
 *
 * Check if the instruction accesses memory */
#define INS_IS_MEMORY(x) (INS_TYPE(x) == IT_LOAD || INS_TYPE(x) == IT_STORE || INS_TYPE(x) == IT_STORE_MULTIPLE || INS_TYPE(x) == IT_LOAD_MULTIPLE || INS_TYPE(x) == IT_FLT_STORE || INS_TYPE(x) == IT_FLT_LOAD)

/*@}*/

/*! (down)Cast to instruction */

#define T_INS(x) ((t_ins*)(x))

#define INS_DYNAMIC_MEMBER(lcasename,name,ccname,type,defval) \
  static void ccname ## Init(t_ins *ins, type *valp) { *valp = defval; } \
  static void ccname ## Fini(t_ins *ins, type *valp) { } \
  static void ccname ## Dup (t_ins *ins, type *valp) { *valp = defval; } \
  t_dynamic_member_info lcasename ## _l_ins_array; \
  DYNAMIC_MEMBER(ins, t_cfg *, lcasename ## _l_ins_array, type, lcasename, name, ccname, CFG_FOREACH_INS, ccname ## Init, ccname ## Fini, ccname ## Dup)

#define INS_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void ccname ## Init(t_ins *ins, type *valp) initbody \
  static void ccname ## Fini(t_ins *ins, type *valp) finibody \
  static void ccname ## Dup (t_ins *ins, type *valp) dupbody \
  extern t_dynamic_member_info lcasename ## _g_ins_array; \
  DYNAMIC_MEMBER(ins, t_cfg *, lcasename ## _g_ins_array, type, lcasename, name, ccname, CFG_FOREACH_INS, ccname ## Init, ccname ## Fini, ccname ## Dup)

#define INS_DYNAMIC_MEMBER_GLOBAL(lcasename,name,ccname,type,defval) \
  INS_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,{ *valp = defval; },{},{ *valp = defval; })
#endif

#define INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(lcasename) \
  t_dynamic_member_info lcasename ## _g_ins_array

/* }}} */
#ifdef DIABLOFLOWGRAPH_TYPES
/* Ins Types {{{ */
#ifndef DIABLOFLOWGRAPH_INS_TYPES
#define DIABLOFLOWGRAPH_INS_TYPES
#include "diabloflowgraph.h"

struct _t_inslist
{
  t_ins *ins;
  struct _t_inslist *next;
};
#endif
/* }}} */
#endif
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Ins Functions {{{ */
#ifndef DIABLOFLOWGRAPH_INS_FUNCTIONS
#define DIABLOFLOWGRAPH_INS_FUNCTIONS

/* TODO */
#define AINS(x,y,z) (RealGetInsByNumber(__FILE__,__LINE__,y,z))
void *RealGetInsByNumber (const char *, int, t_section * section, t_uint32 i);
t_ins *SecGetInsByAddress (t_section * sec, t_address addr);
t_ins *ObjectGetInsByAddress(t_object *obj, t_address addr);
t_ins *InsFindMovableDownEqualInsInBbl (t_ins * search, t_bbl * bbl);
t_ins *InsFindMovableUpEqualInsInBbl (t_ins * search, t_bbl * bbl);
void InsMoveToEndOfBbl(t_ins *ins, t_bbl *bbl);

void InitDelayedInsKilling ();
void ApplyDelayedInsKilling ();


t_bool GenericInsIsStore (t_ins * ins);
t_bool GenericInsIsLoad (t_ins * ins);

void ObjectDumpDisassembledCode (t_object *obj, t_string dumpname);

#define InsIsAfIndexInstruction(x) (INS_ATTRIB(x) & IF_AF_INDEX_INSTRUCTION)
#define InsMarkAfIndexInstruction(x) (INS_SET_ATTRIB(x, INS_ATTRIB(x) | IF_AF_INDEX_INSTRUCTION))
#endif
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
