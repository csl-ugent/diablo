/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>

#include <time.h>

#define DEBUG_LOOP

void
BackEdgeAppendSorted (t_back_edge * insert, t_back_edge ** head)
{
  t_back_edge *iter, *iter2 = NULL;

  for (iter = *head; iter && iter->edge < insert->edge; iter2 = iter, iter = iter->next);

  if (iter2)
  {
    insert->next = iter2->next;
    iter2->next = insert;
  }
  else
  {
    insert->next = *head;
    *head = insert;
  }
}

void
LoopRefAppendSorted (t_loopref * insert, t_loopref ** head)
{
  t_loopref *iter, *iter2 = NULL;

  for (iter = *head; iter && iter->loop < insert->loop; iter2 = iter, iter = iter->next);

  if (iter2)
  {
    insert->next = iter2->next;
    iter2->next = insert;
  }
  else
  {
    insert->next = *head;
    *head = insert;
  }
}

void
LoopFunRefAppendSorted (t_loopfunref * insert, t_loopfunref ** head)
{
  t_loopfunref *iter, *iter2 = NULL;

  for (iter = *head; iter && iter->fun < insert->fun; iter2 = iter, iter = iter->next);

  if (iter2)
  {
    insert->next = iter2->next;
    iter2->next = insert;
  }
  else
  {
    insert->next = *head;
    *head = insert;
  }
}

t_bool
LoopElemAppendSortedUnique (t_loopelem * insert, t_loopelem ** head)
{
  t_loopelem *iter, *iter2 = NULL;

  for (iter = *head; iter && iter->bbl < insert->bbl; iter2 = iter, iter = iter->next);

  if (iter && iter->bbl == insert->bbl)
    return FALSE;
  if (iter2)
  {
    insert->next = iter2->next;
    iter2->next = insert;
  }
  else
  {
    insert->next = *head;
    *head = insert;
  }
  return TRUE;
}

#define MAX_STACK_HEIGHT 50000
typedef struct _t_bbl_stack
{
  t_uint32 top;
  t_bbl *stack[MAX_STACK_HEIGHT];
} data_stack;

#define STACK_NOT_EMPTY(stack) ((stack)->top)

void LoopMerge (t_cfg * cfg, t_loop * loop_a, t_loop * loop_b);
static void LoopRemoveChildFromParent (t_loop * parent, t_loop * child);
static void LoopRemoveParentFromChild (t_loop * child, t_loop * parent);
static void Push (data_stack * stack, t_bbl * bbl);
static t_bbl *Pop (data_stack * stack);
static t_bool LoopAddBbl (t_loop * loop, t_bbl * bbl);
static void AddFunToLoopRecursive (t_function * fun, t_cfg_edge * edge, t_loop * loop);

/*! Add a new loop, with backedge, to the loop in the flowgraph.
 * Also add all bbl's that are in the loop and add an entry in bbl_loops
 * of these bbl's. */
t_loop *
LoopNew (t_cfg * cfg, t_back_edge * loopedge, t_bool recursive)
     /*{{{ */
{
  t_bbl *header, *footer, *working_bbl;

  /*t_back_edge * new_backedge; */
  data_stack working_stack;
  t_loop *ret = (t_loop *) Calloc (1, sizeof (t_loop));
  t_cfg_edge *iter, *i_edge;
  t_function *fun;
  t_bbl *bbl;
  t_bool big, small;
  t_uint32 printinfo = 100;
  t_cfg_edge *backedge = loopedge->edge;

  /* t_uint32 nnodes = 0;*/

  working_stack.top = 0;

  /* insert the loop in the dll of loops and set the backedge {{{ */
  if (CFG_LOOP_FIRST(cfg) == NULL)
  {
    CFG_SET_LOOP_FIRST(cfg, ret);
    CFG_SET_LOOP_LAST(cfg, ret);
  }
  else
  {
    ret->prev = CFG_LOOP_LAST(cfg);
    CFG_LOOP_LAST(cfg)->next = ret;
    CFG_SET_LOOP_LAST(cfg, ret);
  }

  BackEdgeAppendSorted (loopedge, &LOOP_BACKEDGES(ret));
  ret->cfg = cfg;
  ret->infinite = TRUE;
  /*}}} */

  /* init the Judy Array */
  ret->bbl_array = (Pvoid_t) NULL;

  if (loopedge->has_corr == FALSE)
  {
    header = CFG_EDGE_TAIL(backedge);
    footer = CFG_EDGE_HEAD(backedge);
  }
  else
  {
    header = CFG_EDGE_TAIL(CFG_EDGE_CORR(backedge));
    footer = CFG_EDGE_HEAD(backedge);
  }

  VERBOSE(printinfo, ("---------------------New loop--------------------------\nHeader: @B\nFooter: @B\nLoop %p", header, footer, ret));

  /*{{{ Mark all forward ip edges that may be followed */
  if (BBL_FUNCTION(header) != BBL_FUNCTION(footer))
  {
    BblMarkInit ();
    BblMarkInit2 ();
    EdgeMarkInit ();
    BblMark (footer);
    small = TRUE;

    CFG_FOREACH_FUN(cfg, fun) FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED));
    CFG_FOREACH_BBL(cfg, bbl)
      if (BblIsMarked (bbl) && BBL_FUNCTION(bbl))
        FUNCTION_SET_FLAGS(BBL_FUNCTION(bbl), FUNCTION_FLAGS(BBL_FUNCTION(bbl)) | FF_IS_MARKED);
    big = TRUE;
    while (!(big = !big))
      CFG_FOREACH_FUN(cfg, fun)
        if ((!(FUNCTION_FLAGS(fun) & FF_IS_MARKED)))
          continue;
        else
        {
          FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED));
          while (!(small = !small))
            FUNCTION_FOREACH_BBL(fun, bbl)
            {
              if (!BblIsMarked (bbl))
                continue;
              else
              {
                BblUnmark (bbl);
                if (!BblIsMarked2 (bbl))
                {
                  BblMark2 (bbl);
                  VERBOSE(printinfo, ("Treating @B", bbl));
                  if (!BBL_IS_HELL (bbl))
                    BBL_FOREACH_PRED_EDGE(bbl, i_edge)
                    {
                      VERBOSE(printinfo, (" Treating @E", i_edge));
                      if (CfgEdgeIsBackwardInterproc (i_edge))
                      {
                        if (!BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))))
                          small = TRUE;

                        BblMark (CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge)));
                        VERBOSE(printinfo, (" Marking @B", CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))));
                      }
                      else
                      {
                        if (CfgEdgeIsForwardInterproc (i_edge) && BblDominates (header, CFG_EDGE_HEAD(i_edge)))
                        {
                          VERBOSE(printinfo, (" Marking edge @E", i_edge));
                          CfgEdgeMark (i_edge);
                        }
                        if (!BblIsMarked (CFG_EDGE_HEAD(i_edge)))
                        {
                          if (CfgEdgeIsForwardInterproc (i_edge))
                          {
                            big = TRUE;
                            FUNCTION_SET_FLAGS(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge)), FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge))) | FF_IS_MARKED);
                          }
                          else
                            small = TRUE;
                          BblMark (CFG_EDGE_HEAD(i_edge));
                          VERBOSE(printinfo, (" Marking @B", CFG_EDGE_HEAD(i_edge)));
                        }
                      }
                    }
                }
              }
            }
        }
  }
  /*}}} */

  /* Mark all nodes in the loop */

  MarkCallerChain (cfg, BBL_FUNCTION(header));

  BblMarkInit ();
  BblMarkInit2 ();

  if (loopedge->has_corr == TRUE)
  {
    AddFunToLoopRecursive (BBL_FUNCTION(CFG_EDGE_HEAD(CFG_EDGE_CORR(backedge))), CFG_EDGE_CORR(backedge), ret);
  }

  BblMark (footer);
  Push (&working_stack, footer);
  printinfo = 100;
  VERBOSE(printinfo, ("Pushing @B", footer));

  while (STACK_NOT_EMPTY(&working_stack))
  {
    working_bbl = Pop (&working_stack);

    VERBOSE(printinfo, ("Adding @B", working_bbl));
    if (!LoopAddBbl (ret, working_bbl))
      continue;

    if (BBL_IS_HELL(working_bbl) &&
        working_bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(working_bbl)))
    {
      t_bbl *hell = FUNCTION_BBL_FIRST(BBL_FUNCTION(working_bbl));
      if (!BblIsMarked (hell))
      {
        BblMark (hell);
        Push (&working_stack, hell);
        VERBOSE(printinfo, ("Pushing @B", hell));
      }
      continue;
    }

    if (working_bbl == header)
      continue;

    /* Check which predecessors should be investigated next */
    BBL_FOREACH_PRED_EDGE(working_bbl, iter)
    {
      /* we don't follow funlinks, they are not control flow */
      if (CFG_EDGE_CAT(iter) == ET_SWI) /* TODO: patch for removal of fun_links and swi_links */
      {
        continue;
      }
      /* in case of return (or compensating) edges, we add the head of the corresponding edge in case we have already
       * visited the first bbl of the function we return from */

      if (CFG_EDGE_CAT(iter) == ET_RETURN || CFG_EDGE_CAT(iter) == ET_COMPENSATING)
      {
        if (!(FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_HEAD(iter))) & FF_IS_MARKED))
        {
          if (CFG_EDGE_FLAGS(iter) & EF_CORR_EDGE_IS_REACHABLE)
          {
            if (!BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(iter))))
            {
              BblMark (CFG_EDGE_HEAD(CFG_EDGE_CORR(iter)));
              Push (&working_stack, CFG_EDGE_HEAD(CFG_EDGE_CORR(iter)));
              VERBOSE(printinfo, ("Pushing @B", CFG_EDGE_HEAD(CFG_EDGE_CORR(iter))));
            }
          }

          AddFunToLoopRecursive (BBL_FUNCTION(CFG_EDGE_HEAD(iter)), iter, ret);

          continue;
        }
        else
        {
          if (CFG_EDGE_CORR(iter) && BblIsMarked (CFG_EDGE_TAIL(CFG_EDGE_CORR(iter))) && (CFG_EDGE_TAIL(CFG_EDGE_CORR(iter))) != header)
          {
            if (!BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(iter))))
            {
              BblMark (CFG_EDGE_HEAD(CFG_EDGE_CORR(iter)));
              Push (&working_stack, CFG_EDGE_HEAD(CFG_EDGE_CORR(iter)));
              VERBOSE(printinfo, ("Pushing @B", CFG_EDGE_HEAD(CFG_EDGE_CORR(iter))));
            }
          }
          if (!BblIsMarked (CFG_EDGE_HEAD(iter)))
          {
            BblMark (CFG_EDGE_HEAD(iter));
            Push (&working_stack, CFG_EDGE_HEAD(iter));
            VERBOSE(printinfo, ("Pushing @B", CFG_EDGE_HEAD(iter)));
          }
          continue;
        }
      }

      if (CfgEdgeIsForwardInterproc (iter))
      {
        if ((CfgEdgeIsMarked (iter) &&
             (!BBL_IS_HELL(working_bbl) || working_bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(working_bbl))))
            || (CFG_EDGE_CORR(iter) && BblIsMarked (CFG_EDGE_TAIL(CFG_EDGE_CORR(iter))) && BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(iter)))))
        {
          if (!BblIsMarked (CFG_EDGE_HEAD(iter)))
          {
            BblMark (CFG_EDGE_HEAD(iter));
            Push (&working_stack, CFG_EDGE_HEAD(iter));
            VERBOSE(printinfo, ("Pushing @B", CFG_EDGE_HEAD(iter)));
          }
        }
        continue;
      }
      /* All other edges we haven't treated yet: push the head on the working stack */
      if (!BblIsMarked (CFG_EDGE_HEAD(iter)))
      {
        BblMark (CFG_EDGE_HEAD(iter));
        Push (&working_stack, CFG_EDGE_HEAD(iter));
        VERBOSE(printinfo, ("Pushing @B", CFG_EDGE_HEAD(iter)));
      }
    }
  }
  /* VERBOSE(0,("Owke now check: %d",nnodes));*/
/*   LoopPrint(ret,0);*/

  return ret;
} /*}}} */

t_bool
LoopContainsBbl (t_loop * loop, t_bbl * bbl)
     /*{{{ */
{
  PWord_t pdummy;

  JLG(pdummy, loop->bbl_array, (Word_t) bbl);
  if (pdummy != NULL)
    return TRUE;
  return FALSE;

} /* }}} */

void
LoopKill (t_loop * iloop)
     /*{{{*/
{
  Word_t dummy;
  t_loopref *iterref;
  t_loop *loop = iloop;

  /* Unlink the loop */
  if (loop->prev)
    loop->prev->next = loop->next;
  else if (CFG_LOOP_FIRST(LOOP_CFG(loop)) == loop)
    CFG_SET_LOOP_FIRST(LOOP_CFG(loop), loop->next);
  else
    FATAL(("Dll of loops corrupt!"));
  if (loop->next)
    loop->next->prev = loop->prev;
  else if (CFG_LOOP_LAST(LOOP_CFG(loop)) == loop)
    CFG_SET_LOOP_LAST(LOOP_CFG(loop), loop->prev);
  else
    FATAL(("Dll of loops corrupt!"));
  /*loop->prev = NULL; */
  /*loop->next = NULL; */

  {
    t_back_edge *iter;

    while (LOOP_BACKEDGES(loop))
    {
      iter = LOOP_BACKEDGES(loop);
      LOOP_BACKEDGES(loop) = iter->next;
      Free (iter);
    }
  }



  /* Free all loopelem's */
  JLFA(dummy, loop->bbl_array);

  while (LOOP_PARENTS(loop))
  {
    iterref = LOOP_PARENTS(loop);
    LoopRemoveChildFromParent (iterref->loop, loop);
    LOOP_PARENTS(loop) = LOOP_PARENTS(loop)->next;
    Free (iterref);
  }

  while (LOOP_CHILDREN(loop))
  {
    iterref = LOOP_CHILDREN(loop);
    LoopRemoveParentFromChild (iterref->loop, loop);
    LOOP_CHILDREN(loop) = LOOP_CHILDREN(loop)->next;
    Free (iterref);
  }
  /*loop->count = 0; */
  /*loop->cfg = NULL; */
  Free (loop);
  loop = NULL;
} /*}}} */

void
LoopBblCleanup (t_bbl * bbl)
{
  /* t_loopref * remove;*/
  t_uint32 i;
  t_loop *loop;


  CFG_FOREACH_LOOP(BBL_CFG(bbl), loop)
  {
    /* Delete the bbl from the bbl_array */
    JLD(i, loop->bbl_array, (Word_t) bbl);
    /* if(i == 0) FATAL(("BBL not found in loop!"));*/
  }
}

#if 0 /* Broken */
t_int32
EdgeLoopExits (t_cfg_edge * edge)
{
  t_int32 from_loops = 0;
  t_loopref *loop_ref;

  if (!BBL_LOOPS(CFG_EDGE_HEAD(edge)) && !BBL_LOOPS(CFG_EDGE_TAIL(edge)))
    return 0;
  loop_ref = BBL_LOOPS(CFG_EDGE_HEAD(edge));
  while (loop_ref)
  {
    if (LoopBblIsLoopExit (LOOPREF_LOOP(loop_ref), CFG_EDGE_HEAD(edge)) && !LoopContainsBbl (LOOPREF_LOOP(loop_ref), CFG_EDGE_TAIL(edge)))
    {
      from_loops++;
    }
    loop_ref = loop_ref->next;
  }
  return from_loops;
}
#endif

#if 0 /* No longer used */
t_int32
EdgeLoopEntries (t_cfg_edge * edge)
{
  t_int32 to_loops = 0;
  t_loopref *loop_ref;

  if (!BBL_LOOPS(CFG_EDGE_HEAD(edge)) && !BBL_LOOPS(CFG_EDGE_TAIL(edge)))
    return 0;
  loop_ref = BBL_LOOPS(CFG_EDGE_TAIL(edge));
  while (loop_ref)
  {
    if (T_BBL(LOOP_HEADER(LOOPREF_LOOP(loop_ref))) == CFG_EDGE_TAIL(edge) && !LoopContainsBbl (LOOPREF_LOOP(loop_ref), CFG_EDGE_HEAD(edge)))
      to_loops++;
    loop_ref = loop_ref->next;
  }
  return to_loops;
}
#endif

#if 0 /* Broken */
t_bool
LoopBblIsLoopExit (t_loop * loop, t_bbl * bbl)
{
  t_loopelem *iter;

  iter = LOOP_ELEMS(loop)[HASHINDEX(bbl)];
  while (iter)
  {
    if (iter->bbl == bbl)
    {
      if (iter->exit)
        return TRUE;
      else
        return FALSE;
    }
    if (iter->bbl > bbl)
      break;
    iter = iter->next;
  }

  return FALSE;
}
#endif

static void LoopAddParent (t_loop *child, t_loop *parent)
{
  t_loopref *new = Malloc (sizeof (t_loopref));
  new->loop = parent;
  new->next = LOOP_PARENTS (child);
  LOOP_PARENTS (child) = new;
}

static void LoopAddChild (t_loop *parent, t_loop *child)
{
  t_loopref *new = Malloc (sizeof (t_loopref));
  new->loop = child;
  new->next = LOOP_CHILDREN (parent);
  LOOP_CHILDREN (parent) = new;
}

/* {{{ helper functions for LoopFindNestedLoops */
static int __helper_sort_loops (const void *a, const void *b)
{
  t_loop *la = *((t_loop **) a), *lb = *((t_loop **) b);
  return LOOP_COUNT(lb) - LOOP_COUNT(la);  
}

/* We can exclude blocks from checking in two cases:
 *      - block A has only one successor, block B:
 *              if A is part of any loop, then B automatically is as well
 *      - block A has only one predecessor, block B:
 *              if A is part of any loop, then B should be as well because
 *              otherwise there'd be no way for control flow to enter A.
 * Caution! We must take care not to include any cases where both conditions
 * apply: if A has only one successor, B, whose only predecessor is A, we 
 * wouldn't check B because of reason 1, and we wouldn't check A because of
 * reason 2. However, both reasons rely on the other block being checked as
 * justification for the exclusion of a block.
 */
static t_bbl **LoopGetBlocksToCheck (t_loop *loop, t_uint32 *nblocks_ret)
{
  t_uint32 nblocks = 0, i;
  t_loopiterator *iter;
  t_bbl *bbl;
  t_bbl **blocks = Calloc (sizeof (t_bbl *), LOOP_COUNT (loop));
  loop->blocks = Malloc (sizeof (t_bbl *)*LOOP_COUNT (loop));

  NodeMarkInit ();
  NodeMarkInit2 ();
  LOOP_FOREACH_BBL (loop, iter, bbl)
  {
    loop->blocks[nblocks++] = bbl;

    if (!BblIsMarked2 (bbl))
      if (BBL_SUCC_FIRST (bbl) == BBL_SUCC_LAST (bbl) && BBL_SUCC_FIRST (bbl))
        if (CFG_EDGE_TAIL (BBL_SUCC_FIRST (bbl)) != bbl)
          BblMark (CFG_EDGE_TAIL (BBL_SUCC_FIRST (bbl)));
    if (!BblIsMarked (bbl))
      if (BBL_PRED_FIRST (bbl) == BBL_PRED_LAST (bbl) && BBL_PRED_FIRST (bbl))
        if (CFG_EDGE_HEAD (BBL_PRED_FIRST (bbl)) != bbl)
          BblMark2 (CFG_EDGE_HEAD (BBL_PRED_FIRST (bbl)));
  }
  Free (iter);

  for (nblocks = 0, i = 0; i < LOOP_COUNT (loop); ++i)
    if (!BblIsMarked (loop->blocks[i]) && !BblIsMarked2 (loop->blocks[i]))
      blocks[nblocks++] = loop->blocks[i];

  *nblocks_ret = nblocks;
  return blocks;
}

static t_bool BlocksInLoop (t_bbl **blocks, t_uint32 nblocks, t_loop *loop)
{
  t_bbl **loopblocks = loop->blocks;
  t_uint32 nloopblocks = LOOP_COUNT (loop);
  t_uint32 i, j;

  /* find first block through binary search */
  t_uint32 ubound, lbound, lookat=0;
  ubound = nloopblocks;
  lbound = 0;
  while (lbound < ubound)
  {
    lookat = (ubound + lbound) / 2;
    if (loopblocks[lookat] == blocks[0])
      break;
    if (loopblocks[lookat] > blocks[0])
      ubound = lookat;
    else
      lbound = lookat + 1;
  }

  if (lbound == ubound)
    return FALSE; /* could not find blocks[0] */

  if (nloopblocks - lookat < nblocks)
    return FALSE; /* not enough blocks left in loop */

  i = 1;
  j = lookat + 1;

  while (i < nblocks)
  {
    while (j < nloopblocks && loopblocks[j] < blocks[i])
      ++j;

    if (j == nloopblocks) return FALSE;
    if (loopblocks[j] != blocks[i]) return FALSE;
    ++i;
  }
  return TRUE;
}

#define ipart(x)                ((x)/32)
#define fpart(x)                ((x)%32)
#define unmark(line, x) \
  do { (line)[ipart(x)] &= ~(1 << fpart(x)); } while (0)
#define mark(line, x) \
  do { (line)[ipart(x)] |= (1 << fpart(x)); } while (0)
#define ismarked(line, x)          ((line)[ipart(x)] & (1 << fpart(x)))

static void unmark_line (t_uint32 **mtx, int childidx, int paridx)
{
  int i;
  for (i = 0; i <= ipart(paridx); ++i)
    mtx[childidx][i] &= mtx[paridx][i];
}
/* }}} */

/* {{{ LoopFindNestedLoops: build loop nesting hierarchy */
void LoopFindNestedLoops (t_cfg *cfg)
{
  int nloops, i;
  t_loop **looparr;
  t_loop *loop;
  t_loop *start_here;

  t_uint32 ** check_mtx;
  t_uint32 bitveclen;
  t_uint32 nblocks;

  clock_t start, end;
  start = clock();

  /* {{{ sort loops from large to small */
  nloops = 0;
  CFG_FOREACH_LOOP (cfg, loop)
    ++nloops;
  if (nloops == 0) return;

  looparr = Malloc (sizeof (t_loop *) * nloops);
  i = 0;
  CFG_FOREACH_LOOP (cfg, loop)
    looparr[i++] = loop;

  diablo_stable_sort (looparr, nloops, sizeof (t_loop *), __helper_sort_loops);

  for (i = 0; i < nloops; ++i)
  {
    if (i > 0)
      LOOP_PREV (looparr[i]) = looparr[i-1];
    if (i < nloops-1)
      LOOP_NEXT (looparr[i]) = looparr[i+1];
    looparr[i]->idx = i;
  }
  LOOP_PREV (looparr[0]) = NULL;
  LOOP_NEXT (looparr[nloops - 1]) = NULL;
  CFG_SET_LOOP_FIRST (cfg, looparr[0]);
  CFG_SET_LOOP_LAST (cfg, looparr[nloops - 1]);
  Free(looparr);
  looparr=NULL;
  /* }}} */

  /* initialize data structures */
  bitveclen = sizeof (t_uint32) * ((nloops + 31)/32);
  check_mtx = Malloc (sizeof (t_uint32 *) * nloops);
  for (i = 0; i < nloops; ++i)
  {
    check_mtx[i] = Malloc (bitveclen);
    memset (check_mtx[i], 0xff, bitveclen);
  }

  start_here = CFG_LOOP_FIRST (cfg);
  unmark (check_mtx[0], 0);
  Free (LoopGetBlocksToCheck (start_here, &nblocks));

  /* the actual searching */
  for (loop = LOOP_NEXT (start_here); loop; loop = LOOP_NEXT (loop))
  {
    t_loop *iloop;
    int idx = loop->idx;
    t_bbl **testblocks;
    t_uint32 nblocks;

    unmark (check_mtx[idx], idx);

    testblocks = LoopGetBlocksToCheck (loop, &nblocks);

    if (nblocks == 0)
      FATAL (("no blocks checked for loop %d", idx));

    for (iloop = start_here; iloop; iloop = LOOP_PREV (iloop))
    {
      /* iloop should at least be bigger than loop */
       if (LOOP_COUNT (iloop) <= LOOP_COUNT (loop)) continue; 

      /* do we have to test this one? */
      if (!ismarked (check_mtx[idx], iloop->idx)) continue;

      if (BlocksInLoop (testblocks, nblocks, iloop))
      {
        unmark_line (check_mtx, idx, iloop->idx);
        LoopAddParent (loop, iloop);
        LoopAddChild (iloop, loop);
      }
    }

    Free (testblocks);

    if (LOOP_NEXT (loop) && LOOP_COUNT (LOOP_NEXT (loop)) < LOOP_COUNT (loop))
      start_here = loop;
  }

  /* clean up */
  CFG_FOREACH_LOOP (cfg, loop)
  {
    Free (loop->blocks);
    loop->blocks = NULL;
  }
  for (i=0; i < nloops; ++i)
    Free (check_mtx[i]);
  Free (check_mtx);

  end = clock();
  VERBOSE(0,("LoopFindNestedLoops: %llu ticks", (unsigned long long) (end - start)));
} /* }}} */


/* Move this to iterators.h once bruno has finished his code rewriting */
#define CFG_FOREACH_LOOP_SAFE(cfg,loop,tmp) for (loop=CFG_LOOP_FIRST(cfg), tmp=loop?LOOP_NEXT(loop):NULL; loop!=NULL; loop=tmp, tmp=loop?LOOP_NEXT(loop):NULL)
void
LoopMergeLoopsWithSameHeader (t_cfg * cfg)
{
  t_loop *i_loop, *j_loop, *tmp;

  VERBOSE(0, ("Start merging loops"));

  CFG_FOREACH_LOOP(cfg, i_loop)
  {
    t_bool skip_this = FALSE;
    t_loopref *iterparents = LOOP_PARENTS(i_loop);

    /* Only merge loops that are not nested in another loop with the same header */
    while (iterparents)
    {
      if (LOOP_HEADER(iterparents->loop) == LOOP_HEADER(i_loop))
      {
        skip_this = TRUE;
        break;
      }
      iterparents = iterparents->next;
    }
    if (skip_this)
      continue;

    CFG_FOREACH_LOOP_SAFE(cfg, j_loop, tmp)
    {
      if (i_loop != j_loop && LOOP_HEADER(i_loop) == LOOP_HEADER(j_loop))
      {
        t_loopref *jterparents = LOOP_PARENTS(j_loop);
        t_bool stop_here = FALSE;

        /* VERBOSE(1,("Candidates! @E and @E",LOOP_BACKEDGES(i_loop)->data,LOOP_BACKEDGES(j_loop)->data));*/

        while (jterparents)
        {
          if (LOOP_HEADER(jterparents->loop) == LOOP_HEADER(i_loop))
          {
            stop_here = TRUE;
            break;
          }
          jterparents = jterparents->next;
        }
        if (stop_here)
          continue;

        /* VERBOSE(0,("I have to merge @E and @E",LOOP_BACKEDGES(i_loop)->edge,LOOP_BACKEDGES(j_loop)->edge));*/

        LoopMerge (cfg, i_loop, j_loop);

        LoopKill (j_loop);
        j_loop = NULL;
        /* {
           t_bbl * k_bbl;
           t_function *fun;
           CFG_FOREACH_FUN(cfg,fun)
           FUNCTION_FOREACH_BBL(fun,k_bbl)
           {
           VERBOSE(0,("Beezblock: @B",k_bbl));
           fflush(stdout);
           BblIsLoopHeader(k_bbl);
           }
           } */

      }
    }
  }

  CFG_FOREACH_LOOP(cfg, i_loop)
  {
    t_cfg_edge *i_edge;
    Word_t index = 0;
    PWord_t dummy;
    t_bbl *bbl;

    if (i_loop->cfg != cfg)
      FATAL(("!!!!"));

    JLF(dummy, i_loop->bbl_array, index);

    while (dummy != NULL)
    {
      bbl = (t_bbl *) index;
      /* TODO: this can be made more precise: use the type of successor edges! */
      BBL_FOREACH_SUCC_EDGE(bbl, i_edge)
        if (!LoopContainsBbl (i_loop, CFG_EDGE_TAIL(i_edge)))
        {
          LOOP_INF(i_loop) = FALSE;
          break;
        }

      if (LOOP_INF(i_loop) == FALSE)
        break;
      JLN(dummy, i_loop->bbl_array, index);
    }


    /* LoopPrint(i_loop,0);*/
    /* VERBOSE(0,("nr_blocks = %d, inf = %d",LOOP_COUNT(i_loop),LOOP_INF(i_loop)));*/
  }
}

void
LoopMerge (t_cfg * cfg, t_loop * loop_a, t_loop * loop_b)
{
  t_back_edge *new_backedge;
  t_loopref *child;
  PWord_t dummy;
  Word_t index = 0;

  /* VERBOSE(0,("Merging loops, i'm merging loohoohoohoops %p, %p",loop_a,loop_b));fflush(stdout); */

  /* LoopPrint(loop_a,0);fflush(stdout); */
  /*LoopPrint(loop_b,0);fflush(stdout); */

  if (loop_a == loop_b)
    FATAL(("Merging the same loop!"));
  /*if(loop_a->infinite || (loop_b)->infinite) FATAL(("Never thought we should have to merge infinite loops"));
  */

  if (LOOP_BACKEDGES(loop_b)->next)
    VERBOSE(1, ("Loop_b already has multiple backedges"));
  new_backedge = Malloc (sizeof (t_back_edge));
  new_backedge->edge = LOOP_BACKEDGES(loop_b)->edge;
  new_backedge->has_corr = LOOP_BACKEDGES (loop_b)->has_corr;

  new_backedge->next = LOOP_BACKEDGES(loop_a);
  LOOP_BACKEDGES(loop_a) = new_backedge;

  JLF(dummy, loop_b->bbl_array, index);

  while (dummy != NULL)
  {
    LoopAddBbl (loop_a, (t_bbl *) index);

    JLN(dummy, loop_b->bbl_array, index);
  }

  /* Set the parent of each loop nested in loop_b to loop_a */
  child = LOOP_CHILDREN(loop_b);
  while (child)
  {
    t_loopref *parents = LOOP_PARENTS(child->loop);

    while (parents)
    {
      if (parents->loop == loop_b)
        break;
      parents = parents->next;
    }
    if (!parents)
      FATAL(("Nesting info corrupt"));
    parents->loop = loop_a;
    child = child->next;
  }

  /* Merge childs-list of two loops */

  child = LOOP_CHILDREN(loop_a);
  while (child && child->next)
    child = child->next;
  if (child)
    child->next = LOOP_CHILDREN(loop_b);
  else
    LOOP_CHILDREN(loop_a) = LOOP_CHILDREN(loop_b);
  LOOP_CHILDREN(loop_b) = NULL;

}

static void
LoopRemoveChildFromParent (t_loop * parent, t_loop * child)
{
  t_loopref *iter = LOOP_CHILDREN(parent);
  t_loopref *prev = NULL;

  while (iter && iter->loop != child)
  {
    prev = iter;
    iter = iter->next;
  }
  if (iter)
  {
    if (prev)
      prev->next = iter->next;
    else
      LOOP_CHILDREN(parent) = iter->next;
    Free (iter);
  }
}

static void
LoopRemoveParentFromChild (t_loop * child, t_loop * parent)
{
  t_loopref *iter = LOOP_PARENTS(child);
  t_loopref *prev = NULL;

  while (iter && iter->loop != parent)
  {
    prev = iter;
    iter = iter->next;
  }
  if (iter)
  {
    if (prev)
      prev->next = iter->next;
    else
      LOOP_PARENTS(child) = iter->next;
    Free (iter);
  }
}

#if 0
t_bool
BblIsLoopHeader (t_bbl * bbl)
{

  t_loopref *refs = BBL_LOOPS(bbl);

  while (refs)
  {
    if (T_BBL(LOOP_HEADER(LOOPREF_LOOP(refs))) == bbl)
      return TRUE;
    refs = refs->next;
  }
  return FALSE;
}
#endif

static void
Push (data_stack * stack, t_bbl * bbl)
{
  /* VERBOSE(0,("Pushing @B",bbl));*/
  stack->stack[stack->top] = T_BBL(bbl);
  stack->top++;
  if (stack->top == MAX_STACK_HEIGHT)
    FATAL(("Increase stack height"));
}

static t_bbl *
Pop (data_stack * stack)
{
  if (!stack->top)
    FATAL(("Stack empty!"));
  stack->top--;
  return stack->stack[stack->top];
}

/* This function initialises the state of a loopiterator, so you can start calling LoopGetNextBbl */
t_loopiterator *
LoopNewIterator (t_loop * loop)
{
  t_loopiterator *ret = (t_loopiterator *) Calloc (1, sizeof (t_loopiterator));

  ret->loop = loop;
  ret->index = 0;
  return ret;
}

/* An interator function, that return all bbl of a loop. Call LoopInitIterator before you start using it! */
t_bbl *
LoopGetNextBbl (t_loopiterator * iterator)
{
  PWord_t dummy;

  if (iterator->index == 0)
  {
    JLF(dummy, iterator->loop->bbl_array, iterator->index);
    if (dummy == NULL)
      return NULL;
  }
  else
  {
    JLN(dummy, iterator->loop->bbl_array, iterator->index);
    if (dummy == NULL)
      return NULL;
  }
  return (t_bbl *) (iterator->index);
}

/* This function checks whether a function will belong to a loop in
   its entirety once a return edge belongs to the loop, and under the
   (to be later verified) condition that its callees permit backwards
   reachability. The latter condition is of course recursive */

void
CfgMarkFunsWithoutIncomingJumps (t_cfg * cfg)
{
  t_function *fun;
  t_cfg_edge *edge;

  CFG_FOREACH_FUN(cfg, fun)
    FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & ~FF_HAS_INCOMING_IPJUMP);
  CFG_FOREACH_EDGE(cfg, edge)
    if (CFG_EDGE_CAT(edge) == ET_IPJUMP || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      FUNCTION_SET_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge)), FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(edge))) | FF_HAS_INCOMING_IPJUMP);
}

void
CfgMarkEdgesForWhichInIsReachableFromOut (t_cfg * cfg)
{
  t_cfg_edge *out_edge, *edge;
  t_function *fun;
  t_bbl *bbl;
  t_bbl *exit_block;
  t_bool should_do_fun;
  t_bool bbl_change;
  t_bool fun_change;

  CFG_FOREACH_FUN(cfg, fun)
    FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_MARKED);

  CFG_FOREACH_EDGE(cfg, edge)
  {
    if (FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_HEAD(edge))))
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | EF_CORR_EDGE_IS_REACHABLE);
    else
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) & (~EF_CORR_EDGE_IS_REACHABLE));
  }

  fun_change = TRUE;

  while (fun_change)
  {
    fun_change = FALSE;

    CFG_FOREACH_FUN(cfg, fun)
    {
      if (!(FUNCTION_FLAGS(fun) & FF_IS_MARKED))
        continue;
      if (BBL_IS_HELL (FUNCTION_BBL_FIRST(fun)))
        continue;

      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED));


      should_do_fun = FALSE;

      exit_block = FunctionGetExitBlock (fun);

      if (!exit_block)
        continue;

      /* See if there are any return edges left for which EF_CORR_EDGE_IS_REACHABLE is not set yet */
      BBL_FOREACH_SUCC_EDGE(exit_block, out_edge)
        if (!(CFG_EDGE_FLAGS(out_edge) & EF_CORR_EDGE_IS_REACHABLE))
          should_do_fun = TRUE;

      if (!should_do_fun)
        continue;

      /* VERBOSE(0,("CER doing fun %s",FUNCTION_NAME(fun)));*/

      BblMarkInit ();
      BblMarkInit2 ();

      BblMark (exit_block);

      bbl_change = TRUE;

      while (bbl_change)
      {
        bbl_change = FALSE;

        FUNCTION_FOREACH_BBL_R(fun, bbl)
        {
          if (!BblIsMarked (bbl))
            continue;

          BblUnmark (bbl);
          BblMark2 (bbl);

          /* VERBOSE(0,("Unmarking and marking2 @B",bbl));*/


          BBL_FOREACH_PRED_EDGE(bbl, edge)
          {
            if (CFG_EDGE_CAT(edge) == ET_SWI) /* same todo as above on swi_links */
              continue;

            if (CfgEdgeIsForwardInterproc (edge))
              continue;

            /* in case of return (or compensating) edges, we add the head of the corresponding edge in case we have already
             * visited the first bbl of the function we return from */
            if (CfgEdgeIsBackwardInterproc (edge))
            {
              if ((CFG_EDGE_FLAGS(edge) & EF_CORR_EDGE_IS_REACHABLE))
              {
                if (!BblIsMarked2 (CFG_EDGE_HEAD(CFG_EDGE_CORR(edge))))
                {
                  /* VERBOSE(0,(" Marking @B",CFG_EDGE_HEAD(EDGE_CORR(edge))));*/
                  BblMark (CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));
                  bbl_change = TRUE;
                }
              }
            }
            else
            {
              if (!BblIsMarked2 (CFG_EDGE_HEAD(edge)))
              {
                /* VERBOSE(0,(" Marking @B",CFG_EDGE_HEAD(edge)));*/
                BblMark (CFG_EDGE_HEAD(edge));
                bbl_change = TRUE;
              }
            }
          }
        }
      }

      BBL_FOREACH_SUCC_EDGE(exit_block, out_edge)
        if (!(CFG_EDGE_FLAGS(out_edge) & EF_CORR_EDGE_IS_REACHABLE))
        {
          if (BblIsMarked2 (CFG_EDGE_TAIL(CFG_EDGE_CORR(out_edge))))
            /* We made it from the exitblock backwards to the entry of the fun,
             * set the EF_CORR_EDGE_IS_REACHABLE flag */
          {
            /* VERBOSE(0,("CORR_EDGE_REACH @E",out_edge));*/
            CFG_EDGE_SET_FLAGS(out_edge, CFG_EDGE_FLAGS(out_edge) | EF_CORR_EDGE_IS_REACHABLE);
            fun_change = TRUE;
            FUNCTION_SET_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(out_edge)), FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_TAIL(out_edge))) | FF_IS_MARKED);
          }
        }
    }
  }
#if 0
  CFG_FOREACH_FUN(cfg, fun)
  {
    exit_block = FunctionGetExitBlock (fun);

    if (exit_block)
    {
      fun_change = TRUE;

      BBL_FOREACH_SUCC_EDGE(exit_block, out_edge)
        if (!(CFG_EDGE_FLAGS(out_edge) & EF_CORR_EDGE_IS_REACHABLE))
          fun_change = FALSE;

      if (!fun_change)
      {
        DiabloPrint (stdout, "fun @B has problematic edges:\n", FUNCTION_BBL_FIRST(fun));


        BBL_FOREACH_SUCC_EDGE(exit_block, out_edge)
          if (!(CFG_EDGE_FLAGS(out_edge) & EF_CORR_EDGE_IS_REACHABLE))
            DiabloPrint (stdout, " @E\n", out_edge);
      }
      else
        DiabloPrint (stdout, "no problemo for edges in fun @B\n", FUNCTION_BBL_FIRST(fun));
    }
    else
      DiabloPrint (stdout, "fun @B has no exit block\n", FUNCTION_BBL_FIRST(fun));

  }
#endif
}

void
CfgMarkEdgesForWhichAllBblsAreReachableFromOut (t_cfg * cfg)
{
  t_cfg_edge *edge;
  t_function *fun;
  t_bbl *bbl;
  t_bbl *exit_block;
  t_bool bbl_change;
  t_bool fun_change;

  CFG_FOREACH_FUN(cfg, fun)
  {
    if (BBL_IS_HELL (FUNCTION_BBL_FIRST(fun)))
      continue;

    exit_block = FunctionGetExitBlock (fun);

    if (!exit_block)
      continue;

    BblMarkInit ();
    BblMarkInit2 ();

    BblMark (exit_block);

    bbl_change = TRUE;

    while (bbl_change)
    {
      bbl_change = FALSE;

      FUNCTION_FOREACH_BBL_R(fun, bbl)
      {
        if (!BblIsMarked (bbl))
          continue;

        BblUnmark (bbl);
        BblMark2 (bbl);

        BBL_FOREACH_PRED_EDGE(bbl, edge)
        {
          if (CFG_EDGE_CAT(edge) == ET_SWI) /* same todo */
            continue;

          /* in case of return (or compensating) edges, we add the head of the corresponding edge in case we have already
           * visited the first bbl of the function we return from */
          if (CfgEdgeIsBackwardInterproc (edge))
          {
            if ((CFG_EDGE_FLAGS(edge) & EF_CORR_EDGE_IS_REACHABLE))
            {
              if (!BblIsMarked2 (CFG_EDGE_HEAD(CFG_EDGE_CORR(edge))))
              {
                BblMark (CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));
                bbl_change = TRUE;
              }
            }
          }
          else if (CfgEdgeIsForwardInterproc (edge))
            continue;
          else
          {
            if (!BblIsMarked2 (CFG_EDGE_HEAD(edge)))
            {
              BblMark (CFG_EDGE_HEAD(edge));
              bbl_change = TRUE;
            }
          }
        }
      }
    }

    fun_change = TRUE;

    FUNCTION_FOREACH_BBL(fun, bbl)
    {
      if (!BblIsMarked2 (bbl))
      {
        BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) & (~BBL_CAN_REACH_EXIT));
        fun_change = FALSE;
      }

      else
        BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_CAN_REACH_EXIT);
    }

    if (!fun_change)
    {
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_ALL_BLOCKS_REACHABLE_FROM_EXIT));
      /* DiabloPrint(stdout,"fun @B has problematic bbls:\n",FUNCTION_BBL_FIRST(fun));


         FUNCTION_FOREACH_BBL(fun,bbl)
         if (!BblIsMarked2(bbl))
         DiabloPrint(stdout," @B\n",bbl);
         */
    }
    else
    {
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_ALL_BLOCKS_REACHABLE_FROM_EXIT);
      /* DiabloPrint(stdout,"no problemo for bbls in fun @B\n",FUNCTION_BBL_FIRST(fun)); */
    }
  }

  CFG_FOREACH_HELL_FUNCTION(cfg, fun)
  {
    if (FUNCTION_CALL_HELL_TYPE(fun))
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_ALL_BLOCKS_REACHABLE_FROM_EXIT);
  }
}

static void
AddFunToLoopRecursive (t_function * fun, t_cfg_edge * edge, t_loop * loop)
{
  t_bbl *bbl;
  t_cfg_edge *pred_edge;


  if (BBL_IS_HELL (FUNCTION_BBL_FIRST(fun)))
  {
    FUNCTION_FOREACH_BBL(fun, bbl)
      LoopAddBbl (loop, bbl);
    return;
  }

  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    if ((BBL_ATTRIB(bbl) & BBL_CAN_REACH_EXIT))
      if (LoopAddBbl (loop, bbl))
        BBL_FOREACH_PRED_EDGE(bbl, pred_edge)
          if (CfgEdgeIsBackwardInterproc (pred_edge) && (BBL_ATTRIB(CFG_EDGE_HEAD(CFG_EDGE_CORR(pred_edge))) & BBL_CAN_REACH_EXIT))
            AddFunToLoopRecursive (BBL_FUNCTION(CFG_EDGE_HEAD(pred_edge)), pred_edge, loop);
  }
}

void
CfgMarkEdgesThatCoverWholeFun (t_cfg * cfg)
{
  t_function *fun;
  t_bbl *exit_block;
  t_cfg_edge *edge;

  CFG_FOREACH_FUN(cfg, fun)
  {
    if (BBL_IS_HELL (FUNCTION_BBL_FIRST(fun)))
      continue;

    exit_block = FunctionGetExitBlock (fun);

    if (!exit_block)
      continue;

    BBL_FOREACH_SUCC_EDGE(exit_block, edge)
    {
      if (!(FUNCTION_FLAGS(fun) & FF_ALL_BLOCKS_REACHABLE_FROM_EXIT))
        CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) & (~EF_ALL_BLOCKS_REACHABLE_BETWEEN_ENTRY_AND_EXIT));
      else
        CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | EF_ALL_BLOCKS_REACHABLE_BETWEEN_ENTRY_AND_EXIT);
    }
  }
}

static t_bool
LoopAddBbl (t_loop * loop, t_bbl * bbl)
{
  Word_t dummy = 1;
  PWord_t pdummy = &dummy;

  JLI(pdummy, loop->bbl_array, (Word_t) bbl);
  if (*pdummy == 0)
  {
    *pdummy = (Word_t) bbl;
    LOOP_COUNT(loop)++;
    return TRUE;
  }
  else
    return FALSE;
}

t_bool
LoopExists (t_cfg * cfg, t_cfg_edge * edge, t_bool corr_header)
{
  t_bbl *footer, *header;
  t_loop *i_loop;

  if (!corr_header)
    header = CFG_EDGE_TAIL(edge);
  else
    header = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
  footer = CFG_EDGE_HEAD(edge);


  CFG_FOREACH_LOOP(cfg, i_loop)
  {
    if (CFG_EDGE_HEAD(LOOP_BACKEDGES(i_loop)->edge) == footer && CFG_EDGE_TAIL(LOOP_BACKEDGES(i_loop)->edge) == header)
      break;
    if (LOOP_BACKEDGES(i_loop)->has_corr == TRUE)
      if ((CFG_EDGE_HEAD(CFG_EDGE_CORR(LOOP_BACKEDGES(i_loop)->edge))) == footer
          && (CFG_EDGE_TAIL(CFG_EDGE_CORR(LOOP_BACKEDGES(i_loop)->edge))) == header)
        break;
  }
  if (i_loop)
    return TRUE;
  return FALSE;
}

void
CfgLoopSetInfiniteProperty (t_cfg * cfg)
{
  t_loop *i_loop;

  CFG_FOREACH_LOOP(cfg, i_loop)
  {
    t_bbl *ibbl;
    t_loopiterator *iterator;
    t_bool go_on = TRUE;

    LOOP_INF(i_loop) = TRUE;
    LOOP_FOREACH_BBL(i_loop, iterator, ibbl)
    {
      t_cfg_edge *i_edge;

      BBL_FOREACH_SUCC_EDGE(ibbl, i_edge)
      {
        /* TODO: this can be made more precise */
        if (!LoopContainsBbl (i_loop, CFG_EDGE_TAIL(i_edge)))
        {
          LOOP_INF(i_loop) = FALSE;
          go_on = FALSE;
          break;
        }
      }
      if (!go_on)
        break;
    }
    Free (iterator);
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
