/* Program Exit Typedefs {{{ */
#ifndef DIABLOFLOWGRAPH_PROGRAMEXIT_TYPEDEFS
#define DIABLOFLOWGRAPH_PROGRAMEXIT_TYPEDEFS
typedef struct _t_cfg_exits t_cfg_exits;
#endif
/* }}} */
#include "diabloflowgraph.h"
#ifdef DIABLOFLOWGRAPH_TYPES
/* Program Exit Types {{{ */
#ifndef DIABLOFLOWGRAPH_PROGRAMEXIT_TYPES
#define DIABLOFLOWGRAPH_PROGRAMEXIT_TYPES
struct _t_cfg_exits
{
  t_bbl *exit_bbl;
  t_ins *exit_ins;
  /* TRUE if we're certain, FALSE if it's a syscall that might be exit but we're not sure */
  t_bool certain;
  void *tmp;
  struct _t_cfg_exits *next;
};
#endif
/* }}} */
#endif
#ifdef DIABLOFLOWGRAPH_FUNCTIONS
/* Program Exit Types {{{ */
#ifndef DIABLOFLOWGRAPH_PROGRAMEXIT_FUNCTIONS
#define DIABLOFLOWGRAPH_PROGRAMEXIT_FUNCTIONS
void CfgCreateExitBlockList (t_cfg * cfg);
void CfgFreeExitBlockList (t_cfg * cfg);
#endif
/* }}} */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
