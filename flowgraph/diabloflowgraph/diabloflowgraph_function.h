/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Function Typedefs {{{ */
#ifndef DIABLOFLOWGRAPH_FUNCTION_TYPEDEFS
#define DIABLOFLOWGRAPH_FUNCTION_TYPEDEFS
typedef struct _t_bbl_draw_data t_bbl_draw_data;
typedef struct _t_edge_draw_data t_edge_draw_data;

/*! Different function types */
typedef enum
{
  FT_NORMAL,
  FT_HELL
} t_function_type;

#endif
/* }}} */
/* Function Defines {{{ */
#ifndef DIABLOFLOWGRAPH_FUNCTION_DEFINES
#define DIABLOFLOWGRAPH_FUNCTION_DEFINES

/*! \todo Just use the function flags for FUNCTION_IS_HELL */
#define FUNCTION_IS_HELL(fun) (FUNCTION_BBL_FIRST(fun) && BBL_IS_HELL(FUNCTION_BBL_FIRST(fun)))
#define FUNCTION_CALL_HELL_TYPE(fun)    (FUNCTION_BBL_FIRST(fun) && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)))

/* Function flags */
/*! Function is marked */
#define FF_IS_MARKED 0x2
/*! Function was an exported function (in the original objectfile) */
#define FF_IS_EXPORTED 0x4
/*! Function uses explicit frame pointer */
#define FF_HAS_FRAMEPOINTER 0x8
/*! Function has incoming edges other than calls */
#define FF_HAS_INCOMING_IPJUMP 0x40
/*! Is this flag is set, the function belongs to a loop entirely as
  soon as one of its return edges belongs to the loop */
#define FF_SPECIAL_LOOP_FUN 0x80
/*! If this flag is set, it means the function will be a
  FF_SPECIAL_LOOP_FUN iff all of its callees are either
  FF_SPECIAL_LOOP_FUN and/or FF_ENTRY_REACHABLE_FROM_EXIT */
#define FF_COND_LOOP_REACH 0x100
/*! If this flag is set, it means the entry point is (backwards)
  reachable from the exit point through the function itself,
  FF_COND_LOOP_REACH or FF_SPECIAL_LOOP_FUN callees */
#define FF_ENTRY_REACHABLE_FROM_EXIT 0x200

/*! Obfuscation: Function calls only functions who are leaf */
#define FF_IS_1_TO_LEAF 0x400
/*! Obfuscation: Function has an incomming Hell-edge */
#define FF_FROM_HELL 0x800

#define FF_ALL_BLOCKS_REACHABLE_FROM_EXIT 0x1000
/*! This function uses the stack only to spill registers */
#define FF_ONLY_SPILLS 0x2000
/*! In this function the stackpointer can be traced and is known to be restored at the end of the function */
#define FF_STACKPOINTER_TRACEABLE 0x4000
/*! Function is marked 2 */
#define FF_IS_MARKED2 0x8000
/*! Don't change the stack layout of this function when this flag is set */
#define FF_DONT_CHANGE_STACK 0x10000
/*! A call to this function will not change the stack pointer in the caller */
#define FF_PRESERVE_STACKHEIGHT 0x20000
/*! This function implements setjmp-like behaviour */
#define FF_IS_SETJMP 0x40000
/*! Function is leaf with one regular bbl and return only */
#define FF_IS_SIMPLE_LEAF 0x80000

#define FUNCTION_DESCRIPTOR(fun) (CFG_DESCRIPTION(FUNCTION_CFG(fun)))

#define T_FUN(fun) ((t_function*)(fun))

#define FunctionMark(fun)     FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED)
#define FunctionMark2(fun)    FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED2)
#define FunctionUnmark(fun)   FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED))
#define FunctionUnmark2(fun)  FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED2))
#define FunctionIsMarked(fun) (FUNCTION_FLAGS(fun) & FF_IS_MARKED)
#define FunctionIsMarked2(fun)(FUNCTION_FLAGS(fun) & FF_IS_MARKED2)

#define T_FUN(fun) ((t_function*)(fun))

#define FUNCTION_DYNAMIC_MEMBER_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Function ## ccname ## Init(t_function *fun, type *valp) initbody \
  static void Function ## ccname ## Fini(t_function *fun, type *valp) finibody \
  static void Function ## ccname ## Dup (t_function *fun, type *valp) dupbody \
  t_dynamic_member_info lcasename ## _l_fun_array; \
  DYNAMIC_MEMBER(function, t_cfg *, lcasename ## _l_fun_array, type, lcasename, name, ccname, CFG_FOREACH_FUN, Function ## ccname ## Init, Function ## ccname ## Fini, Function ## ccname ## Dup)

#define FUNCTION_DYNAMIC_MEMBER(lcasename,name,ccname,type,defval) \
  FUNCTION_DYNAMIC_MEMBER_BODY(lcasename,name,ccname,type,{*valp=defval;},{},{*valp=defval;})

#define FUNCTION_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,initbody,finibody,dupbody) \
  static void Function ## ccname ## Init(t_function *fun, type *valp) initbody \
  static void Function ## ccname ## Fini(t_function *fun, type *valp) finibody \
  static void Function ## ccname ## Dup (t_function *fun, type *valp) dupbody \
  extern t_dynamic_member_info lcasename ## _g_fun_array; \
  DYNAMIC_MEMBER(function, t_cfg *, lcasename ## _g_fun_array, type, lcasename, name, ccname, CFG_FOREACH_FUN, Function ## ccname ## Init, Function ## ccname ## Fini, Function ## ccname ## Dup)

#define FUNCTION_DYNAMIC_MEMBER_GLOBAL(lcasename,name,ccname,type,defval) \
  FUNCTION_DYNAMIC_MEMBER_GLOBAL_BODY(lcasename,name,ccname,type,{ *valp = defval; }, {}, { *valp = defval; })

#define FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(lcasename) \
  t_dynamic_member_info lcasename ## _g_fun_array
#define FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(lcasename) \
  lcasename ## _g_fun_array.init

#endif
/* }}} */
#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_TYPES
/* Function Types {{{ */
#ifndef DIABLOFLOWGRAPH_FUNCTION_TYPES
#define DIABLOFLOWGRAPH_FUNCTION_TYPES
/* data structures and functions for drawing a function cfg with dot */
struct _t_bbl_draw_data
{
  t_string fillcolor;
  t_string label;
  t_string style;
  t_string extra;
};

struct _t_edge_draw_data
{
  t_string style;
  t_string color;
  t_string label;
  t_string extra;
};
#endif
/* }}} */
#endif
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Function Functions {{{ */
#ifndef DIABLOFLOWGRAPH_FUNCTION_FUNCTIONS
#define DIABLOFLOWGRAPH_FUNCTION_FUNCTIONS
t_function * FunctionDuplicate (t_function *fun);

/* version without annotations */
void FunctionDrawGraph(t_function * fun, t_string filename);
void FunctionDrawGraphMarked(t_function *fun, t_string filename, t_bool marked);

/* version with only hotness annotations */
void FunctionDrawGraphWithHotness (t_function * fun, t_string filename);

/* version with liveness annotations */
void FunctionDrawGraphWithLiveness (t_function * fun, t_string filename);
void LivenessAnnotatorSetOpt(t_bool print_before, t_bool print_after);

/* generic bbl and edge annotations */
void FunctionDrawGraphAnnotatedMarked (
                                 t_function * fun, t_string filename,
                                 void (*bbl_annotator) (t_bbl *, t_bbl_draw_data *),
                                 void (*edge_annotator) (t_cfg_edge *, t_edge_draw_data *), t_bool marked);
#define FunctionDrawGraphAnnotated(fun, filename, bbl_annotator, edge_annotator) FunctionDrawGraphAnnotatedMarked(fun, filename, bbl_annotator, edge_annotator, FALSE)

void FunctionCreateExitBlock(t_function *ret);

#endif
/* }}} */
#endif
extern t_uint32 function_global_id;
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
