/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/*#define OLD_IMPL*/
#ifndef MY_LOOP_TYPEDEFS
#define MY_LOOP_TYPEDEFS
typedef struct _t_loop t_loop;
typedef struct _t_loopelem t_loopelem;
typedef struct _t_loopref t_loopref;
typedef struct _t_loopfunref t_loopfunref;
typedef struct _t_loopiterator t_loopiterator;
typedef struct _t_back_edge t_back_edge;
typedef struct _t_loop_list t_loop_list;
#endif
#include "diabloflowgraph.h"

#ifdef DIABLOFLOWGRAPH_TYPES
/* Loop Types {{{ */
#ifndef DIABLOFLOWGRAPH_LOOP_TYPES
#define DIABLOFLOWGRAPH_LOOP_TYPES
#define HASH_TABLE_SIZE 0x10
/*#define HASHINDEX(bbl) (((*((unsigned int*)(&bbl)))>>4) & (HASH_TABLE_SIZE-1))*/
#define HASHINDEX(bbl) ((((unsigned int)(bbl)) >> 4) & (HASH_TABLE_SIZE-1))

struct _t_back_edge
{
  struct _t_back_edge *next;
  t_cfg_edge *edge;
  t_bool has_corr;
};

struct _t_loop
{
  /* Structures to maintain a dll of loops */
  struct _t_loop *prev;
  struct _t_loop *next;
  /* The edge(s) that was(were) responsible for defining this loop. */
  t_back_edge *backedges;
  /* A table of linked lists, hashed by a very simple function */
#ifdef OLD_IMPL
  struct _t_loopelem *elems[16];
#else
  Pvoid_t bbl_array;
#endif
  struct _t_loopfunref *functions;
  /* The number of elements in this loop */
  t_uint32 count;
  /* Is the loop infinite? */
  t_bool infinite;
  /* The flowgraph this loop is in */
  t_cfg *cfg;
  /* The loops in which this loop is nested */
  struct _t_loopref *parents;
  /* The loops that are nested in this loop */
  struct _t_loopref *childs;

  t_uint32 idx;
  t_bbl **blocks;
};

struct _t_loopiterator
{
#ifdef OLD_IMPL
  struct _t_loopelem *iterator_elem;
  t_uint32 iterator_hash;
  struct _t_loopfunref *iterator_fun;
  t_bbl *iterator_bbl;
#else
  Word_t index;
#endif
  t_loop *loop;
};

#define T_LOOP(loop) ((t_loop*)loop)
#define LOOP_NEXT(loop) (T_LOOP(loop)->next)
#define LOOP_PREV(loop) (T_LOOP(loop)->prev)
#define LOOP_BACKEDGES(loop) (T_LOOP(loop)->backedges)
#define LOOP_HEADER(loop) (LOOP_BACKEDGES(loop)->has_corr?CFG_EDGE_TAIL(CFG_EDGE_CORR(LOOP_BACKEDGES(loop)->edge)):CFG_EDGE_TAIL(LOOP_BACKEDGES(loop)->edge))
#define LOOP_ELEMS(loop) (T_LOOP(loop)->elems)
#define LOOP_COUNT(loop) (T_LOOP(loop)->count)
#define LOOP_CFG(loop) (T_LOOP(loop)->cfg)
#define LOOP_INF(loop) (T_LOOP(loop)->infinite)
#define LOOP_PARENTS(loop) (T_LOOP(loop)->parents)
#define LOOP_CHILDREN(loop) (T_LOOP(loop)->childs)
#define LOOP_FUNCTIONS(loop) (T_LOOP(loop)->functions)

                                                                                                                   struct _t_loopelem
{
  /* Structure to make a linked list */
  struct _t_loopelem *next;
  /* The bbl that is in the loop */
  t_bbl *bbl;
};

struct _t_loopfunref
{
  struct _t_loopfunref *next;
  t_function *fun;
};

struct _t_loopref
{
  struct _t_loopref *next;
  t_loop *loop;
};

#define T_LOOPELEM(elem) ((t_loopelem *)elem)
#define LOOPELEM_BBL(elem) (T_LOOPELEM(elem)->bbl)
#define T_LOOPREF(elem) ((t_loopref *) elem)
#define LOOPREF_LOOP(elem) (T_LOOPREF(elem)->loop)

t_loop *LoopNew (t_cfg * cfg, t_back_edge * loopedge, t_bool recursive);
void LoopKill (t_loop *);
t_bool LoopContainsBbl (t_loop * loop, t_bbl * bbl);
void LoopBblCleanup (t_bbl * bbl);
t_int32 EdgeLoopCount (t_cfg_edge * edge);
t_bool LoopBblIsLoopExit (t_loop * loop, t_bbl * bbl);
void LoopFindNestedLoops (t_cfg * cfg);
//t_bool BblIsLoopHeader (t_bbl * bbl);
//t_int32 EdgeLoopEntries (t_cfg_edge * edge);
t_int32 EdgeLoopExits (t_cfg_edge * edge);
void LoopMergeLoopsWithSameHeader (t_cfg * cfg);
void CfgMarkFunsWithoutIncomingJumps (t_cfg * cfg);
t_loopiterator *LoopNewIterator (t_loop * loop);
t_bbl *LoopGetNextBbl (t_loopiterator * iterator);
t_bool LoopExists (t_cfg * cfg, t_cfg_edge * edge, t_bool corr_header);
void CfgLoopSetInfiniteProperty (t_cfg * cfg);

void CfgMarkEdgesForWhichInIsReachableFromOut (t_cfg * cfg);
void CfgMarkEdgesForWhichAllBblsAreReachableFromOut (t_cfg * cfg);
void CfgMarkEdgesThatCoverWholeFun (t_cfg * cfg);

#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
