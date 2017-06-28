/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h>
#include <diabloflowgraph.h>


t_cfg *CfgCreate(t_object *obj)
{
  t_cfg *cfg = CfgNew(obj);
  OBJECT_SET_CFG(obj, cfg);
  CFG_SET_DESCRIPTION(cfg, ObjectGetArchitectureDescription(obj));
  return cfg;
}

/* CfgFree {{{*/
void
CfgFreeData (t_cfg * cfg)
{
  if (CFG_CG(cfg))
    CgFree (CFG_CG(cfg));
  CFG_SET_CG(cfg, NULL);

  while (CFG_LOOP_FIRST(cfg))
  {
    LoopKill (CFG_LOOP_FIRST(cfg));
  }

  while (CFG_EDGE_FIRST(cfg))
  {
    /* the relocations corresponding to switch edges may still be needed
     * later on (e.g., in case of a cell binary, where we'll still add
     * stuff to the (ro)data sections causing the switch tables to move)
     */
    CfgEdgeKillKeepRel (CFG_EDGE_FIRST(cfg));
  }

  CfgFreeExitBlockList (cfg);

  while (CFG_NODE_FIRST(cfg))
  {
    t_bbl *bbl = CFG_NODE_FIRST(cfg);

    while (BBL_INS_FIRST(bbl))
      InsKill (BBL_INS_FIRST(bbl));
    BblKill (bbl);
  }

  while (CFG_FUNCTION_FIRST(cfg))
  {
    FunctionKill (CFG_FUNCTION_FIRST(cfg));
  }

  if (CFG_MARKED_FUNS(cfg))
    Free (CFG_MARKED_FUNS(cfg));
  CFG_SET_MARKED_FUNS(cfg, NULL);
  if (CFG_ENTRY(cfg))
    Free (CFG_ENTRY(cfg));
  CFG_SET_ENTRY(cfg,NULL);
}
/*}}}*/

/* Functions to patch a flowgraph to function representation {{{ */
t_bool
EdgeMakeInterprocedural(t_cfg_edge *edge)
{
  if (CFG_EDGE_CAT(edge) == ET_CALL)
    return false;
  if (CFG_EDGE_CAT(edge) == ET_RETURN)
    return false;
  if (CFG_EDGE_CAT(edge) == ET_SWI)
    return false;
  if (CFG_EDGE_CAT(edge) == ET_COMPENSATING)
    return false;
  if (CFG_EDGE_CAT(edge) & ET_INTERPROC)
    return false;

  t_bbl *bfrom = CFG_EDGE_HEAD(edge);
  t_bbl *bto = CFG_EDGE_TAIL(edge);
  /* if (BBL_IS_HELL(bfrom) || BBL_IS_HELL(bto)) return false; */
  if (BBL_FUNCTION(bfrom) == BBL_FUNCTION(bto))
    return false; /* It's intraprocedural */
  /* add compensating edge */
  t_function *ffrom = BBL_FUNCTION(bfrom);
  t_function *fto = BBL_FUNCTION(bto);
  if ((!ffrom) || (!fto))
    return false;

  if (FunctionGetExitBlock (ffrom) && FunctionGetExitBlock (fto))
  {
    t_cfg *cfg = BBL_CFG(bfrom);
    CfgEdgeCreateCompensating (cfg, edge);
  }
  /* patch type of the interprocedural edge */
  switch (CFG_EDGE_CAT(edge))
  {
    case ET_UNKNOWN:
      CFG_EDGE_SET_CAT(edge, ET_IPUNKNOWN);
      break;
    case ET_FALLTHROUGH:
      CFG_EDGE_SET_CAT(edge, ET_IPFALLTHRU);
      break;
    case ET_JUMP:
      CFG_EDGE_SET_CAT(edge, ET_IPJUMP);
      break;
    case ET_SWITCH:
      CFG_EDGE_SET_CAT(edge, ET_IPSWITCH);
      break;
    default:
      FATAL(("Uncaught!"));
  }

  return true;
}

t_bool
EdgeMakeIntraProcedural(t_cfg_edge *edge)
{
  if (!(CFG_EDGE_CAT(edge) & ET_INTERPROC))
    return false;
  if (CFG_EDGE_CAT(edge) == ET_RETURN)
    return false;

  t_bbl *bfrom = CFG_EDGE_HEAD(edge);
  t_bbl *bto = CFG_EDGE_TAIL(edge);

  /* do we actually want to make it intraprocedural? */
  if (BBL_FUNCTION(bfrom) != BBL_FUNCTION(bto))
    return false;

  t_function *ffrom = BBL_FUNCTION(bfrom);
  t_function *fto = BBL_FUNCTION(bto);
  if (!ffrom || !fto)
    return false;

  if (CFG_EDGE_CORR(edge))
    CfgEdgeKill(CFG_EDGE_CORR(edge));

  switch(CFG_EDGE_CAT(edge))
  {
    case ET_IPUNKNOWN:
      CFG_EDGE_SET_CAT(edge, ET_UNKNOWN);
      break;
    case ET_IPFALLTHRU:
      CFG_EDGE_SET_CAT(edge, ET_FALLTHROUGH);
      break;
    case ET_IPJUMP:
      CFG_EDGE_SET_CAT(edge, ET_JUMP);
      break;
    case ET_IPSWITCH:
      CFG_EDGE_SET_CAT(edge, ET_SWITCH);
      break;
    default:
      FATAL(("Uncaught! @E", edge));
  }

  return true;
}

/* CfgPatchNormalEdges {{{ */
t_uint32
CfgPatchNormalEdges (t_cfg * cfg)
{
  t_cfg_edge *edge;
  t_uint32 ret = 0, ret1 = 0, ret2 = 0, ret4 = 0;
  t_bbl *bfrom, *bto;
  t_function *ffrom, *fto;

  /* {{{ avoid interprocedural funlinks */
  /* we want to avoid interprocedural funlink edges, because they require special
   * and hard to understand handling in most analyses. Therefore, for each interprocedural
   * funlink edge we add an empty block to the calling function, and make this the tail
   * of the funlink edge. From this block, a fallthrough edge (which may be interprocedural)
   * goes to the original tail of the funlink edge.
   *
   * I.e., if we have a call edge from fun1 to fun2, but the corresponding return
   * edge goes to another function than fun1, we make it return to an empty
   * block in fun1 which then gets a fallthrough edge to the return site in
   * that other function (by iterating over all predecessor edges of the
   * return site block and making those edges point to this empty block).
   */

  CFG_FOREACH_EDGE(cfg, edge)
  {
    t_bbl *head, *tail, *new;
    t_cfg_edge *edge2, *tmp;

    /* the wrapper is handled specially anyway and nothing returns
     * there, so ignore it */
    if (BBL_FUNCTION(CFG_EDGE_HEAD(edge)) == CFG_WRAP_FUNCTION(cfg))
      continue;
    /* only do this for call and swi edges with corresponding (= return)
     * edges. Exception: don't do it for the special case where by the
     * corresponding edge returns to exit hell, because we don't want to
     * redirect all predecessor edges of exit hell to some other place
     */
    if ((CFG_EDGE_CAT(edge) != ET_CALL && CFG_EDGE_CAT(edge) != ET_SWI) ||
        !CFG_EDGE_CORR(edge) ||
        BBL_IS_HELL(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))))
      continue;
    /* call source and return site are in the same function -> ok */
    if (BBL_FUNCTION(CFG_EDGE_HEAD(edge)) == BBL_FUNCTION(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))))
      continue;

    ret2++;

    head = CFG_EDGE_HEAD(edge);
    tail = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));

    new = BblNew (cfg);
    if (BBL_FUNCTION(head))
      BblInsertInFunction (new, BBL_FUNCTION(head));
    else
      BBL_SET_FUNCTION(new, NULL);

    /* move funlink and return and (if present) fallthrough edge */
    BBL_FOREACH_PRED_EDGE_SAFE(tail, edge2, tmp)
    {
      if (CFG_EDGE_CAT(edge2) != ET_FALLTHROUGH && CFG_EDGE_CAT(edge2) != ET_RETURN)
        continue;

      if (edge2 != CFG_EDGE_CORR(edge))
        continue;

      /* move the edge */
      CfgEdgeChangeTail(edge2, new);
    }
    CfgEdgeCreate (cfg, new, tail, ET_FALLTHROUGH);
  } /* }}} */

  STATUS(START, ("Patching normal edges"));
  CFG_FOREACH_EDGE(cfg, edge)
    if (EdgeMakeInterprocedural(edge))
      ret++;
  STATUS(STOP, ("Patching normal edges"));

  return ret;
}

/* }}} */

/* CfgPatchInterProcedural {{{ */
t_uint32
CfgPatchInterProcedural (t_cfg * fg)
{
  /* patching interprocedural edges */
  t_uint32 ret = 0, ret5 = 0;
  t_function *function;

  ret = CfgPatchNormalEdges (fg);

  STATUS(START, ("Moving return edges to return block"));
  CFG_FOREACH_FUN(fg, function)
  {
    ret5 += FunctionMoveReturnEdgesToReturnBlock (function);
  }
  STATUS(STOP, ("Moving return edges to return block"));

  STATUS(START, ("Adding return for normal calls"));
  CFG_FOREACH_FUN(fg, function)
  {
    ret5 += FunctionAdjustReturnEdges (function);
  }
  STATUS(STOP, ("Adding return for normal calls"));

  ret += ret5;

  return ret;
}

/* }}} */

/* Partition functions along a certain criterion, splitting them up in
 * smaller single-entry functions.
 * NOTE: we expect the first block of each function to be eligible as
 * partition leader. Make sure this is so!
 * {{{ */
/* #define VERBOSE_PART */
void
CfgPartitionFunctions (
                       t_cfg * cfg,
                       t_bool (*IsStartOfPartition) (t_bbl *),
                       t_bool (*CanMerge) (t_bbl *, t_bbl *)
                      )
{
  t_function *fun;
  t_bbl *ibbl, *jbbl, *unpartitioned, *isafe, *jsafe;
  t_bbl *orig_exit, *new_exit;
  t_cfg_edge *iedge, *iedge_safe;
  t_bool multiple_partitions, change;
  t_uint32 fun_appendix;
  t_string fun_name;

#ifdef VERBOSE_PART
  char outerloop;
  t_uint32 innerloop = 0;
  static int execcount = 0;
  char buffy[80];

  sprintf (buffy, "./pardots-%d", ++execcount);
  DirMake (buffy, FALSE);
#endif

  CFG_FOREACH_FUN(cfg, fun)
  {
    if (!FUNCTION_BBL_FIRST(fun))
      continue;
    if (BBL_IS_HELL (FUNCTION_BBL_FIRST(fun)))
      continue;

#ifdef VERBOSE_PART
    VERBOSE(0, ("Partitioning %s...", FUNCTION_NAME(fun)));
#endif

    /* initialization: every bbl with an incoming interprocedural edge begins
     * a new partition {{{ */

    /* the start of the function should always be the start of a partition */
    BBL_SET_TMP(FUNCTION_BBL_FIRST(fun), FUNCTION_BBL_FIRST(fun));

    multiple_partitions = FALSE;
    FUNCTION_FOREACH_BBL(fun, ibbl)
    {
      if ((ibbl == FUNCTION_BBL_FIRST(fun)) || BblIsExitBlock(ibbl))
        continue; /* the entry and exit block are no start of a partition anyway */

      if (IsStartOfPartition (ibbl))
      {
#ifdef VERBOSE_PART
        VERBOSE(0, ("Start of partition: @B", ibbl));
#endif
        BBL_SET_TMP(ibbl, ibbl);
        multiple_partitions = TRUE;
      }
      else
        BBL_SET_TMP(ibbl, NULL);
    } /* }}} */

    if (!multiple_partitions)
      continue;

#ifdef VERBOSE_PART
    sprintf (buffy, "./pardots-%d/%s-0x%x.dot",
             execcount,
             FUNCTION_NAME(fun),
             BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun)));
    FunctionDrawGraph (fun, buffy);
#endif

    /* Now partition the basic blocks into single entry partitions: if a bbl
     * has only predecessors from one partition, add it to that partition,
     * until you find no bbl left you can add to a partition. In that case
     * start a new partition.
     * {{{ */

    /* We make the exit block a member of the partition that starts at the
     * entry point */
    orig_exit = FunctionGetExitBlock (fun);
    if (orig_exit)
      BBL_SET_TMP(orig_exit, FUNCTION_BBL_FIRST(fun));

#ifdef VERBOSE_PART
    outerloop = 'a' - 1;
#endif

    unpartitioned = FUNCTION_BBL_FIRST(fun);

    while (unpartitioned)
    {
#ifdef VERBOSE_PART
      VERBOSE(0, ("1: Start new partition for @B", unpartitioned));
      outerloop++;
      innerloop = 0;
#endif

      BBL_SET_TMP(unpartitioned, unpartitioned);

      change = TRUE;
      while (change)
      {
        unpartitioned = NULL;
#ifdef VERBOSE_PART
        VERBOSE(0, ("iteration %c%d", outerloop, ++innerloop));
#endif
        change = FALSE;

        FUNCTION_FOREACH_BBL(fun, ibbl)
        {
          t_bbl *partition = NULL;
          t_bool unpartitioned_pred = FALSE;

          if (BBL_TMP(ibbl))
            continue; /* only interested in unpartitioned blocks */

          BBL_FOREACH_PRED_EDGE(ibbl, iedge)
          {
            t_bbl *pred = CFG_EDGE_HEAD(iedge);

            /* We are not interested in interprocedural edges */
            if (CfgEdgeIsForwardInterproc (iedge))
              continue;

            if (CfgEdgeIsBackwardInterproc (iedge))
              pred = CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge));

            /* If the predecessor is not in a partition yet, skip it */
            if (!BBL_TMP(pred))
            {
              unpartitioned_pred = TRUE;
              continue;
            }

            if (!partition)
              partition = BBL_TMP(pred);
            else if (BBL_TMP(pred) != partition)
            {
              /* Partitions of predecessors differ, then this bbl is in a new
               * partition */
#ifdef VERBOSE_PART
              VERBOSE(0, ("2:Start new partition for @B", ibbl));
              VERBOSE(0, (" @G:@G <-> @G",
                          BBL_OLD_ADDRESS(pred),
                          BBL_OLD_ADDRESS(BBL_TMP(pred)),
                          BBL_OLD_ADDRESS(partition)));
#endif
              BBL_SET_TMP(ibbl, ibbl);
              break;
            }
          }

          /* If all predecessors are in the same partition, then ibbl is
           * also in that partition */
          if (!BBL_TMP(ibbl) && !unpartitioned_pred && partition)
          {
#ifdef VERBOSE_PART
            VERBOSE(0, ("Adding to existing partition: @B(@B)",
                        ibbl, partition));
#endif
            BBL_SET_TMP(ibbl, partition);
          }

          if (!BBL_TMP(ibbl))
            unpartitioned = ibbl;
          else
            change = TRUE;
        }
      }
    }
    /* }}} */

#ifdef VERBOSE_PART
    VERBOSE(0, ("Partitioning done!"));
    VERBOSE(0, ("Start merging partitions!"));
#endif

    /* merging round: if all incoming edges from one partition come from
     * the same other partition, merge the partitions
     * {{{ */
    change = TRUE;
    while (change)
    {
      change = FALSE;
      FUNCTION_FOREACH_BBL(fun, ibbl)
      {
        t_bbl *incoming_partition = NULL;
        t_bool merge = TRUE;

        /* Skip blocks that are not the entry of a partition */
        if (BBL_TMP(ibbl) != ibbl)
          continue;
        if (BblIsExitBlock(ibbl))
          continue;

        BBL_FOREACH_PRED_EDGE(ibbl, iedge)
        {
          t_bbl *pred;

          if (CfgEdgeIsBackwardInterproc (iedge))
          {
            pred = CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge));
          }
          else if (CFG_EDGE_CAT(iedge) == ET_CALL)
          {
            merge = FALSE;
            break;
          }
          else
            pred = CFG_EDGE_HEAD(iedge);

          if (BBL_FUNCTION(pred) != BBL_FUNCTION(ibbl))
          {
            /* either incoming forward interprocedural edge or
             * call/return pair over function boundaries */
            merge = FALSE;
            break;
          }

          ASSERT(BBL_TMP(pred), ("still unpartitioned blocks"));

          if (BBL_TMP(pred) == ibbl)
            continue; /* in same partition */

          if (!incoming_partition)
          {
            incoming_partition = BBL_TMP(pred);
            if (!CanMerge (incoming_partition, ibbl))
            {
              merge = FALSE;
              break;
            }
          }
          else if (incoming_partition != BBL_TMP(pred))
          {
            merge = FALSE; /* two different incoming partitions */
            break;
          }
        }
        if (merge && incoming_partition)
        {
#ifdef VERBOSE_PART
          VERBOSE(0, ("Merging partition @B and @B",
                      ibbl, incoming_partition));
#endif
          FUNCTION_FOREACH_BBL(fun, jbbl)
            if (BBL_TMP(jbbl) == ibbl)
              BBL_SET_TMP(jbbl, incoming_partition);
          change = TRUE;
        }
      }
    }
    /* }}} */

#ifdef VERBOSE_PART
    VERBOSE(0, ("End merging partitions!"));
#endif

#ifdef VERBOSE_PART
    FUNCTION_FOREACH_BBL(fun, ibbl)
    {
      if (BBL_TMP(ibbl) != BBL_TMP(FUNCTION_BBL_FIRST(fun)))
        VERBOSE(0, ("Split off @B(@B)", ibbl, BBL_TMP(ibbl)));
    }
    printf ("-------------------------------\n");

    VERBOSE(0, ("Start creating new functions"));
#endif

    /* do the actual splitting {{{ */
    fun_appendix = 1;
    FUNCTION_FOREACH_BBL_SAFE(fun, ibbl, isafe)
    {
      t_function *new_fun;

      /* This is the entry of a new partition, create a new function for it and
       * add all blocks */
      if (BBL_TMP(ibbl) == ibbl &&
          BBL_TMP(ibbl) != BBL_TMP(FUNCTION_BBL_FIRST(fun)))
      {
        char appendix[8];

        /* {{{ create new function */
        sprintf (appendix, "%d", fun_appendix);
        if (FUNCTION_NAME(fun))
          fun_name = StringConcat3 (FUNCTION_NAME(fun), "_", appendix);
        else
          fun_name = StringDup ("noname");

        BblUnlinkFromFunction (ibbl);

#ifdef VERBOSE_PART
        VERBOSE(0, ("New function %s starting at @B", fun_name, ibbl));
#endif
        new_fun = FunctionMake (ibbl, fun_name, FT_NORMAL);
        new_exit = FunctionGetExitBlock (new_fun);
        fun_appendix++;
        Free (fun_name);
        /* }}} */

        /* {{{ move all bbls of the partition to the new function */
        FUNCTION_FOREACH_BBL_SAFE(fun, jbbl, jsafe)
          if (BBL_TMP(jbbl) == ibbl)
          {
            /* Caution: do not overwrite the iterator! */
            if (isafe == jbbl)
              isafe = BBL_NEXT_IN_FUN(isafe);

            BblUnlinkFromFunction (jbbl);
            BblInsertInFunction (jbbl, new_fun);

            /* Adjust edges so that edges that go to the exit block of the
             * original function now go to the exit block of the newly created
             * function */
            BBL_FOREACH_SUCC_EDGE_SAFE(jbbl, iedge, iedge_safe)
              if (CFG_EDGE_TAIL(iedge) == orig_exit)
                CfgEdgeChangeTail(iedge, new_exit);
          }

        /* If there's an edge from the partition leader to the original
         * function's exit block, move it to the exit block of the new
         * function */
        BBL_FOREACH_SUCC_EDGE_SAFE(ibbl, iedge, iedge_safe)
          if (CFG_EDGE_TAIL(iedge) == orig_exit)
            CfgEdgeChangeTail(iedge, new_exit);
        /* }}} */

        /* {{{ adjust the corresponding edges whenever necessary */
        BBL_FOREACH_PRED_EDGE(ibbl, iedge)
          if (CfgEdgeIsForwardInterproc (iedge) &&
              CFG_EDGE_CAT(iedge) != ET_CALL)
            if (CFG_EDGE_CORR(iedge))
            {
              CfgEdgeKill (CFG_EDGE_CORR(iedge));
              CFG_EDGE_SET_CORR(iedge, NULL);
              CfgEdgeCreateCompensating (cfg, iedge);
            }

        if (orig_exit)
          BBL_FOREACH_PRED_EDGE_SAFE(orig_exit, iedge, iedge_safe)
            if (CFG_EDGE_CAT(iedge) == ET_COMPENSATING &&
                BBL_FUNCTION(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge))) == new_fun)
            {
              t_cfg_edge *jedge = CFG_EDGE_CORR(iedge);

              CfgEdgeKill (iedge);
              CfgEdgeCreateCompensating (cfg, jedge);
            }
        /* }}} */

#ifdef VERBOSE_PART
        sprintf (buffy, "./pardots-%d/%s-0x%x-AFTER.dot",
                 execcount,
                 FUNCTION_NAME(new_fun),
                 BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(new_fun)));
        FunctionDrawGraph (new_fun, buffy);
#endif
      }
    }
#ifdef VERBOSE_PART
    sprintf (buffy, "./pardots-%d/%s-0x%x-AFTER.dot",
             execcount,
             FUNCTION_NAME(fun),
             BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun)));
    FunctionDrawGraph (fun, buffy);
#endif
    /* }}} */

#ifdef VERBOSE_PART
    VERBOSE(0, ("End creating new functions"));
#endif
  }
  CfgPatchNormalEdges (cfg);

  /* {{{ cleanup and final sanity checks */
  CFG_FOREACH_FUN(cfg, fun)
    FUNCTION_FOREACH_BBL(fun, ibbl)
    {
      BBL_SET_TMP(ibbl, NULL);
      if (ibbl == FUNCTION_BBL_FIRST(fun))
        continue;

      BBL_FOREACH_PRED_EDGE(ibbl, iedge)
      {
        if (!CfgEdgeIsForwardInterproc (iedge))
          continue;
        FunctionDrawGraph (fun, "FATAL.dot");
        FATAL(("Fun %s not completely single entry! @E\n A graph of the function has been created.",
               FUNCTION_NAME(fun), iedge));
      }
    }
  /* }}} */
}

/* }}} */

static t_bool
has_incoming_interproc (t_bbl * bbl)
{
  t_cfg_edge *edge;

  BBL_FOREACH_PRED_EDGE(bbl, edge)
    if (CfgEdgeIsForwardInterproc (edge))
      return TRUE;
    else if (CfgEdgeIsInterproc (edge))
    {
      /* backward interprocedural edge: check if we
       * have an interprocedural call/return pair */
      if (BBL_FUNCTION(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)))
          != BBL_FUNCTION(bbl))
        return TRUE;
    }
  return FALSE;
}

static t_bool
always_true (t_bbl * bbl1, t_bbl * bbl2)
{
  return TRUE;
}

t_uint32
CfgPatchToSingleEntryFunctions (t_cfg * cfg)
{
  t_function *fun;
  t_bbl *ibbl, *jbbl;
  t_cfg_edge *iedge;

  STATUS(START, ("Single entry representation"));

  /* Make sure that all function entries have at least one incoming
   * interprocedural edge. This is a prerequisite for CfgPartitionFunctions.
   * {{{ */
  CFG_FOREACH_FUN(cfg, fun)
  {
    jbbl = FUNCTION_BBL_FIRST(fun);
    BBL_FOREACH_PRED_EDGE(jbbl, iedge)
      if (CfgEdgeIsForwardInterproc (iedge))
        break;
    if (iedge)
      continue;

    FUNCTION_FOREACH_BBL(fun, ibbl)
    {
      BBL_FOREACH_PRED_EDGE(ibbl, iedge)
        if (CfgEdgeIsForwardInterproc (iedge))
          break;
      if (iedge)
        break;
    }
    if (!ibbl)
      continue; /* no incoming interproc edges at all */

    ASSERT(BBL_PREV_IN_FUN(ibbl) && BBL_NEXT_IN_FUN(ibbl),
           ("ibbl should not be first or last"));

    /* Unlink */
    BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(ibbl), BBL_NEXT_IN_FUN(ibbl));
    BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(ibbl), BBL_PREV_IN_FUN(ibbl));
    /* Set ibbl to be the first bbl of the function */
    BBL_SET_NEXT_IN_FUN(ibbl, jbbl);
    BBL_SET_PREV_IN_FUN(jbbl, ibbl);
    FUNCTION_SET_BBL_FIRST(fun, ibbl);
    BBL_SET_PREV_IN_FUN(ibbl, NULL);
  }
  /* }}} */

  CfgPartitionFunctions (cfg, has_incoming_interproc, always_true);

  STATUS(STOP, ("Single entry representation"));
  return 0;
}

/*}}}*/

/* Patch the flowgraph for setjump/longjmp {{{ */
typedef struct _node_list
{
  struct _node_list *next;
  t_bbl *node;
} node_list;

static void
_append_block (node_list ** list, t_bbl * block)
{
  node_list *elem = Calloc (1, sizeof (node_list));

  elem->node = block;
  elem->next = *list;
  *list = elem;
}

static t_bbl *
_get_block (node_list ** list)
{
  node_list *elem = *list;
  t_bbl *ret = NULL;

  if (elem)
  {
    *list = elem->next;
    ret = elem->node;
    Free (elem);
  }
  return ret;
}

t_uint32
CfgPatchSetJmp (t_cfg * cfg)
{
  t_function *fun;
  t_uint32 patched = 0;
  node_list *list = NULL;

  BblMarkInit ();

  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_NAME(fun) &&
        (!strcmp (FUNCTION_NAME(fun), "setjmp") ||
         !strcmp (FUNCTION_NAME(fun), "_setjmp") ||
         !strcmp (FUNCTION_NAME(fun), "sigsetjmp") ||
         !strcmp (FUNCTION_NAME(fun), "__sigsetjmp") ||
         !strcmp (FUNCTION_NAME(fun), "FPC_SETJMP") ||
         !strcmp (FUNCTION_NAME(fun), "_FPC_SETJMP")
        ))
    {
      t_bbl *fun_exit = FunctionGetExitBlock (fun);

      ASSERT(fun_exit, ("Function %s has no exit block, expected one", FUNCTION_NAME(fun)));
      _append_block (&list, fun_exit);
      FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) | FF_IS_SETJMP);
    }
  }

  while (list)
  {
    t_bbl *bbl = _get_block (&list);
    t_cfg_edge *edge, *edge2;

    BblMark (bbl);
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
    {
      t_bbl *tail = CFG_EDGE_TAIL(edge);

      if (!BBL_FUNCTION(tail))
        continue;

      if (CFG_EDGE_CAT(edge) == ET_RETURN)
      {
        if (BBL_FUNCTION(tail) && tail != CFG_EXIT_HELL_NODE(cfg))
        {
          edge2 = CfgEdgeCreate (cfg, CFG_HELL_NODE(cfg), tail, ET_IPJUMP);
          CfgEdgeCreateCompensating (cfg, edge2);
          patched++;
        }
      }
      else if (CFG_EDGE_CAT(edge) == ET_COMPENSATING)
      {
        if (!BblIsMarked (tail))
          _append_block (&list, tail);
      }
      else
        FATAL(("wrong kind of edge here: @E",edge));
    }
  }

  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_NAME(fun) &&
        (!strcmp (FUNCTION_NAME(fun), "longjmp") ||
         !strcmp (FUNCTION_NAME(fun), "_longjmp") ||
         !strcmp (FUNCTION_NAME(fun), "__longjmp") ||
         !strcmp (FUNCTION_NAME(fun), "siglongjmp") ||
         !strcmp (FUNCTION_NAME(fun), "__siglongjmp") ||
         !strcmp (FUNCTION_NAME(fun), "__libc_longjmp") ||
         !strcmp (FUNCTION_NAME(fun), "__libc_siglongjmp") ||
         !strcmp (FUNCTION_NAME(fun), "FPC_LONGJMP") ||
         !strcmp (FUNCTION_NAME(fun), "_FPC_LONGJMP")
        ))
    {
      t_bbl *fun_exit = FunctionGetExitBlock (fun);

      /* longjmp does not have an exit block in all cases, as it
       * can end with an indirect jump rather than with a return
       */
      if (fun_exit)
        _append_block (&list, fun_exit);
    }
  }

  while (list)
  {
    t_bbl *bbl = _get_block (&list);
    t_cfg_edge *edge, *edge2, *tmp;

    /* remove all JUMPs to the function exit block, and
     * replace them with IPJUMPs to hell {{{*/
    BBL_FOREACH_PRED_EDGE_SAFE(bbl, edge, tmp)
    {
      switch (CFG_EDGE_CAT(edge))
      {
        case ET_JUMP:
          {
            t_bbl *pred_head = CFG_EDGE_HEAD(edge);

            CfgEdgeKill(edge);
            edge2 = CfgEdgeCreate(cfg,pred_head,CFG_HELL_NODE(cfg),ET_IPJUMP);
            CfgEdgeCreateCompensating(cfg,edge2);
            patched++;
            break;
          }
        case ET_COMPENSATING:
          {
            CfgEdgeKill(edge);
            break;
          }
        default:
          FATAL(("Unexpected edge to function exit block: @E",edge));
          break;
      }
    }
    /*}}}*/

    /* Now remove all returns from the function {{{*/
    BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, tmp)
    {

      if ((CFG_EDGE_CAT(edge) == ET_RETURN) ||
          (CFG_EDGE_CAT(edge) == ET_COMPENSATING))
      {
        CfgEdgeKill(edge);
        patched++;
      }
      else
        FATAL(("wrong kind of edge here: @E",edge));
    }
    /*}}}*/
  }
  return patched;
}

/* }}} */


/* Replace the entrypoint of the CFG by a function starting with entry_bbl and ending with
 * a jump (which will point to the original entrypoint). It is important to note that
 * the kind of functions we link in to become a new entrypoint may lack an exit BBL as they
 * might not end on a return instruction but on a branch.
 */
void
CfgReplaceEntry (t_cfg * cfg, t_bbl * entry_bbl)
{
  /* Get the t_function we want to be the new entrypoint, and its exit block */
  t_function* fun = BBL_FUNCTION(entry_bbl);
  t_bbl* end_bbl = FunctionGetExitBlock(fun);

  /* Replace the old entry BBL with the new entry_bbl */
  t_cfg_entry_bbl* entrypoint = CFG_ENTRY(cfg);
  t_cfg_edge* edge = entrypoint->entry_edge;
  t_bbl* old_entry_bbl = entrypoint->entry_bbl;
  entrypoint->entry_bbl = entry_bbl;
  CfgEdgeChangeTail(edge, entry_bbl);

  /* Make the necessary changes to the outgoing edge so that at the end of the function we jump
   * to the original entrypoint. This IPJUMP won't have a compensating edge as there will be no return.
   */
  if(end_bbl)
  {
    /* Kill all outgoing edges before we kill the exit block itself */
    t_cfg_edge* tmp_edge;
    BBL_FOREACH_SUCC_EDGE_SAFE(end_bbl, edge, tmp_edge)
      CfgEdgeKill(edge);

    /* The exit block might have a compensating block coming in. Use it to find the IPJUMP going out, and
     * then kill it together with the exit block. Else we take the ET_JUMP to the exit block and make it
     * the outgoing IPJUMP.
     */
    edge = BBL_PRED_FIRST(end_bbl);
    if(CfgEdgeTestCategoryOr(edge, ET_COMPENSATING))
    {
      t_cfg_edge* compensating = edge;
      edge = CFG_EDGE_CORR(compensating);
      CfgEdgeKill(compensating);
    }
    CfgEdgeChangeTail(edge, old_entry_bbl);
    BblKill(end_bbl);
  }
  else
  {
    end_bbl = FUNCTION_BBL_LAST(fun);
    BBL_FOREACH_SUCC_EDGE(end_bbl, edge)
      if(CfgEdgeTestCategoryOr(edge, ET_JUMP | ET_IPJUMP))
        break;
    CfgEdgeChangeTail(edge, old_entry_bbl);
  }

  CFG_EDGE_SET_CAT(edge, ET_IPJUMP);/* Set the category to ET_IPJUMP, might still be ET_JUMP */
}

t_cfg_entry_bbl *
NewEntry (t_bbl * node, t_cfg_edge * edge)
{
  t_cfg_entry_bbl *ret = Malloc (sizeof (*ret));

  ret->tmp = NULL;
  ret->entry_bbl = node;
  ret->entry_edge = edge;
  return ret;
}

t_uint64
SectionRecalculateSizeFlowgraphed (t_section * sec, t_relocatable_address_type type)
{
  t_uint64 endaddr = 0;
  t_cfg *cfg = OBJECT_CFG(SECTION_OBJECT(sec));
  t_bbl *bbl;
  t_uint32 nins = 0;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (type == MIN_ADDRESS)
      endaddr += AddressExtractUint64 (BBL_MIN_SIZE(bbl));
    else
      endaddr += AddressExtractUint64 (BBL_CSIZE(bbl));
    nins += BBL_NINS(bbl);
  }
  if (SECTION_TYPE(sec) != FLOWGRAPHED_CODE_SECTION)
    SECTION_SET_NINS(sec, nins);
  return endaddr;
}

t_uint64
SectionRecalculateSizeDeflowgraphing (t_section * sec, t_relocatable_address_type type)
{
  t_uint64 endaddr = 0;
  t_address endaddr1 = 0;

  /* almost the same as deflowgraphing code section, only
   * the basic blocks are stored in a linked list here
   * (through BBL_NEXT_IN_CHAIN) */
  t_bbl *bbl;
  t_uint32 nins = 0;

  endaddr1 = AssignAddressesInChain (T_BBL(SECTION_TMP_BUF(sec)), SECTION_CADDRESS (sec));

  CHAIN_FOREACH_BBL (T_BBL(SECTION_TMP_BUF(sec)), bbl)
  {
    if (type == MIN_ADDRESS)
      endaddr += AddressExtractUint64 (BBL_MIN_SIZE(bbl));
    else
      endaddr += AddressExtractUint64 (BBL_CSIZE(bbl));
    nins += BBL_NINS(bbl);
  }
  
  /*DEBUG(("size %x =?= %x - %x = %x",(t_uint32) endaddr,G_T_UINT32(endaddr1),G_T_UINT32(SECTION_CADDRESS (sec)),(G_T_UINT32(endaddr1) - G_T_UINT32(SECTION_CADDRESS (sec)))));*/

  /* if we're adding padding bytes to align data blocks, the above computation
     is not correct, so we replace it with the current ones */
  endaddr = G_T_UINT32(endaddr1) - G_T_UINT32(SECTION_CADDRESS (sec));

  SECTION_SET_NINS(sec, nins);
  return endaddr;
}

/* Flowgraph export functions: use gviz to convert the output to PostScript
 * files 
 * for Export_Flowgraph (icfg = interprocedural control flow graph)
 *
 * {{{ */
static t_bool
node_writeout_icfg (t_node * current, t_node * parent, void *data)
{
  /* print out all successors of a node in the flowgraph */
  /* used for exportation of the flowgraph */
  t_bbl *bbl = T_BBL(current);
  FILE *out = data;
  t_ins *ins;
  t_cfg_edge *edge;

  ins = BBL_INS_FIRST(bbl);
  FileIo (out, "\t\"%p\" [label=\"", current);
  if (bbl == CFG_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_SWI_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "SWI HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_LONGJMP_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "LONGJMP HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_CALL_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "CALL HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (BBL_CALL_HELL_TYPE(bbl))
  {
    FileIo(out, "DYNAMIC CALL: %s(%d)", FUNCTION_NAME(BBL_FUNCTION(bbl))+16, BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_EXIT_CALL_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "EXIT CALL HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(bbl)) &&
           bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(bbl)))
  {
    FileIo(out, "EXIT DYNAMIC CALL: %s(%d)", FUNCTION_NAME(BBL_FUNCTION(bbl))+16, BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_EXIT_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "EXIT HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_EXIT_SWI_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "EXIT SWI HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (bbl == CFG_EXIT_LONGJMP_HELL_NODE(BBL_CFG(bbl)))
  {
    FileIo (out, "EXIT LONGJMP HELL(%d)", BBL_DFS_NUMBER(bbl));
  }
  else if (BBL_FUNCTION(bbl) && (BBL_IS_LAST(bbl)))
  { /* return block */
    FileIo (out, "RETURN FROM %s (%d)", FUNCTION_NAME(BBL_FUNCTION(bbl)) ? FUNCTION_NAME(BBL_FUNCTION(bbl)) : "no_name", BBL_DFS_NUMBER(bbl));
  }
  else
  {
    FileIo (out, "@B\\n", bbl);
    /* FileIo(out, "ADDR: @G(F: %p) (%d)\\n", BBL_CADDRESS(bbl), bbl->fun, BBL_DFS_NUMBER(bbl));*/
    while (ins != NULL)
    {
      FileIo (out, "@I\\l", ins);
      ins = INS_INEXT(ins);
    }
  }

  FileIo (out, "\"]\n");
  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    if (diabloflowgraph_options.annotate_loops)
    {
      FileIo (out, "\t\"%p\" -> \"%p\" [style=%s,color=%s,label=\"%d,%d\"];\n", bbl, CFG_EDGE_TAIL(edge),
              (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH) ? "solid" : "dashed",
              (CFG_EDGE_CAT(edge) & ET_INTERPROC) ? "red" : "black", CFG_EDGE_EXEC_COUNT(edge), CFG_EDGE_EXEC_COUNT_MIN(edge));
    }
    if ( /*TOBBL(bbl->succs[i]->to) != hell_node */ 1)
    {
      FileIo (out, "\t\"%p\" -> \"%p\" [style=%s,color=%s];\n", bbl, CFG_EDGE_TAIL(edge),
              (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH) ? "solid" : "dashed",
              (CFG_EDGE_CAT(edge) & ET_INTERPROC) ? "red" : "black");
    }
  }
  return FALSE;
}

void
Export_Flowgraph (t_bbl * start, t_uint16 mask, const char *filename)
{
  FILE *out = fopen (filename, "w");

  ASSERT(out, ("Could not open %s for writing!", filename));
  FileIo (out, "digraph \"@G\" {\n\tnode [shape=box]\n", BBL_OLD_ADDRESS(start));
  GraphDFTraversalWithCheckAndData ((t_node *) start, node_writeout_icfg, mask, ET_CALL | ET_SWI, out);
  FileIo (out, "}\n");
  fclose (out);
}

void
CfgDrawFunctionGraphsAnnotated (t_cfg * cfg, t_const_string dirprefix, void (*fun_draw_graph) (t_function *, t_string))
{
  t_function *function;
  t_string dirpref = StringDup (dirprefix);
  t_string cleaned_fun_name = NULL;
  int noname_count = 0;
  char noname[20];

  /* dirprefix is by default ./dots */
  if (!dirpref)
    dirpref = StringDup ("./dots");

  /* remove trailing slash */
  if (dirpref[strlen (dirpref) - 1] == '/')
    dirpref[strlen (dirpref) - 1] = '\0';

#ifdef DIABLOSUPPORT_HAVE_MKDIR
  DirMake (dirpref, FALSE);
#else
  Free (dirpref);
  dirpref = StringDup (".");
#endif

  CFG_FOREACH_FUN(cfg, function)
  {
    t_string fname;

    if (!FUNCTION_NAME(function))
      sprintf (noname, "-noname-%d-", noname_count++);
    else
    {
      char *c;

      cleaned_fun_name = StringDup(FUNCTION_NAME(function));
      c = cleaned_fun_name;
      while (*c)
      {
        if (*c == '/')
          *c = '_';
        c++;
      }

      if (strlen(cleaned_fun_name) > 80)
        sprintf(cleaned_fun_name + 70,"TRUNCATED");
    }

    if (FUNCTION_BBL_FIRST(function))
      fname = StringIo ("%s/@G.func-%s.dot", dirpref, BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(function)), FUNCTION_NAME(function) ? cleaned_fun_name : noname);
    else
      fname = StringIo ("%s/0x%x.func-%s.dot", dirpref, 0, FUNCTION_NAME(function) ? cleaned_fun_name : noname);
    if (FUNCTION_NAME(function)
        && cleaned_fun_name)
      Free(cleaned_fun_name);

    fun_draw_graph (function, fname);
    Free (fname);
  }
  Free (dirpref);
}

void
DrawFunctionGraphAnnotated (t_function * function, t_const_string dirprefix)
{
  t_string dirpref = StringDup (dirprefix);
  t_string cleaned_fun_name = NULL;
  int noname_count = 0;
  char noname[20];

  /* dirprefix is by default ./dots */
  if (!dirpref)
    dirpref = StringDup ("./dots");

  /* remove trailing slash */
  if (dirpref[strlen (dirpref) - 1] == '/')
    dirpref[strlen (dirpref) - 1] = '\0';

#ifdef DIABLOSUPPORT_HAVE_MKDIR
  DirMake (dirpref, FALSE);
#else
  Free (dirpref);
  dirpref = StringDup (".");
#endif

  t_string fname;

  if (!FUNCTION_NAME(function))
    sprintf (noname, "-noname-%d-", noname_count++);
  else
  {
    char *c;

    cleaned_fun_name = StringDup(FUNCTION_NAME(function));
    c = cleaned_fun_name;
    while (*c)
    {
      if (*c == '/')
        *c = '_';
      c++;
    }

    if (strlen(cleaned_fun_name) > 80)
      sprintf(cleaned_fun_name + 70,"TRUNCATED");
  }

  if (FUNCTION_BBL_FIRST(function))
    fname = StringIo ("%s/@G.func-%s.dot", dirpref, BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(function)), FUNCTION_NAME(function) ? cleaned_fun_name : noname);
  else
    fname = StringIo ("%s/0x%x.func-%s.dot", dirpref, 0, FUNCTION_NAME(function) ? cleaned_fun_name : noname);
  if (FUNCTION_NAME(function)
      && cleaned_fun_name)
    Free(cleaned_fun_name);

  FunctionDrawGraph (function, fname);
  Free (fname);

  Free (dirpref);
}

void
CfgDrawFunctionGraphs (t_cfg * cfg, t_const_string dirprefix)
{
  CfgDrawFunctionGraphsAnnotated (cfg, dirprefix, FunctionDrawGraph);
}

void
CfgDrawFunctionGraphsWithHotness (t_cfg * cfg, t_const_string dirprefix)
{
  CfgDrawFunctionGraphsAnnotated(cfg, dirprefix, FunctionDrawGraphWithHotness);
}

void
CfgDrawFunctionGraphsWithLiveness (t_cfg * cfg, t_const_string dirprefix)
{
  LivenessAnnotatorSetOpt(TRUE, TRUE);
  CfgDrawFunctionGraphsAnnotated(cfg, dirprefix, FunctionDrawGraphWithLiveness);
}

/* }}} */

/* CfgCreateBasicBlocks {{{ */

/* helper function: sort symbol array */
static int
__helper_sort_syms (const void *a, const void *b)
{
  t_symbol *syma = *((t_symbol **) a);
  t_symbol *symb = *((t_symbol **) b);

  if (AddressIsGe (RELOCATABLE_CADDRESS(SYMBOL_BASE(symb)), RELOCATABLE_CADDRESS(SYMBOL_BASE(syma))))
    return -1;
  else
    return 1;
}

t_uint32
CfgCreateBasicBlocks (t_object *obj)
{
  t_uint32 nbbls = 0;
  t_ins *i_ins;
  t_bbl *bbl = NULL;
  t_bbl *entry = NULL;
  t_reloc *reloc, *tmp;
  t_section *sec;
  t_address current_address = AddressNullForObject(obj);
  t_uint32 i, j, k, nsyms;
  t_symbol *sym, **symarr;
  t_overlay *ovl;
  t_overlay_sec *ovlsec = NULL, *ovlseci;
  t_address *compiler_generated_starts = Malloc(1024*1024*sizeof(t_address));
  t_uint32 nr_compiler_generated_sections = 0;
  t_cfg *cfg = OBJECT_CFG(obj);

  /* {{{ sort the $handwritten and $compiler symbols for this section */
  /* count the $compiler and $handwritten symbols */
  nsyms = 0;
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym)
  {
    if (SYMBOL_NAME(sym) && !strncmp (SYMBOL_NAME(sym), "$compiler", 9))
    {
      SYMBOL_NAME(sym)[9] = '\0'; /* truncate the symbols inserted for inline assembly */
      nsyms++;
    }
    if (SYMBOL_NAME(sym) && !strncmp (SYMBOL_NAME(sym), "$handwritten", 12))
    {
      SYMBOL_NAME(sym)[12] = '\0'; /* truncate the symbols inserted for inline assembly */
      nsyms++;
    }
    /* our little gcc patch trick does not flag the .text.lock.* sections in
     * the kernel, so we have to catch them by hand */
    if (SYMBOL_NAME(sym) && !strncmp (SYMBOL_NAME(sym), ".text.lock.", 11))
    {
      nsyms++;
    }
  }
  symarr = Malloc(sizeof(t_symbol *) * nsyms);

  /* sort them */
  for (i = 0, sym = SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj)); sym; sym = SYMBOL_NEXT(sym))
  {
    if (SYMBOL_NAME(sym) && (!strcmp (SYMBOL_NAME(sym), "$compiler") || !strcmp (SYMBOL_NAME(sym), "$handwritten")))
      symarr[i++] = sym;
    if (SYMBOL_NAME(sym) && !strncmp (SYMBOL_NAME(sym), ".text.lock.", 11))
      symarr[i++] = sym;
  }
  diablo_stable_sort (symarr, (size_t) nsyms, sizeof (t_symbol *), __helper_sort_syms);
  /* }}} */

  /* first allocate all basic blocks (so that they are contiguous in memory,
   * not interleaved with instructions) */

  t_object * sub;
  t_object * sub2;
  OBJECT_FOREACH_SUBOBJECT(obj,sub,sub2)
    {
      if (!OBJECT_COMPILER_GENERATED(sub)) continue;
      OBJECT_FOREACH_CODE_SECTION(sub,sec,j)
        {
          compiler_generated_starts[nr_compiler_generated_sections++]=SECTION_CADDRESS(sec);
        }
    }


  OBJECT_FOREACH_CODE_SECTION(obj, sec, j)
  {
    enum { COMPILER, HANDWRITTEN } state = HANDWRITTEN, newstate;

    bbl = NULL;

    /* find the $compiler or $handwritten symbol corresponding to the
     * start of the section {{{ */
    for (i = 0; i < nsyms; ++i)
    {
      t_relocatable *firstins = T_RELOCATABLE(SECTION_DATA(sec));
      if (AddressIsGe(RELOCATABLE_CADDRESS(SYMBOL_BASE(symarr[i])), RELOCATABLE_CADDRESS(firstins)))
        break;
    }
    ASSERT(i < nsyms, ("no handwritten mapping for this code section"));
    /* }}} */
    
    ovlsec = NULL;
    for (ovl = OBJECT_OVERLAYS(obj); ovl && !ovlsec; ovl = ovl->next)
      for (ovlseci = ovl->sec; ovlseci; ovlseci = ovlseci->next)
        if (!strcmp(ovlseci->name, SECTION_NAME(sec)))
        {
          ovlsec = ovlseci;
          break;
        }

    SECTION_FOREACH_INS(sec, i_ins)
    {
      /* {{{ update COMPILER/HANDWRITTEN state */
      t_bool start_of_compiler_generated_sections = FALSE;
      for (k=0;k<nr_compiler_generated_sections;k++)
        {
          if (AddressIsEq(INS_CADDRESS(i_ins),compiler_generated_starts[k]))
            {
              //              DEBUG(("setting state to COMPILER for @I for section start",i_ins));
              state = COMPILER;
              break;
            }
        }

      if (i < nsyms && 
          AddressIsEq(INS_CADDRESS(i_ins),
                      RELOCATABLE_CADDRESS(SYMBOL_BASE(symarr[i]))))
      {
        /* if one of the symbols pointing to this instruction is
         * not $compiler, state becomes HANDWRITTEN */
        t_bool found = FALSE;
        newstate = COMPILER;
        for (;
              i < nsyms &&
                AddressIsLe(RELOCATABLE_CADDRESS(SYMBOL_BASE(symarr[i])),
                            INS_CADDRESS(i_ins));
              ++i)
        {
          if (T_INS(SYMBOL_BASE(symarr[i])) != i_ins)
            continue; /* ignore: symbol from other overlay section */
          found = TRUE;
          if (strcmp(SYMBOL_NAME(symarr[i]), "$compiler"))
            {
              newstate = HANDWRITTEN;
            }
        }
        if (found)
          {
            /*            if (newstate==COMPILER)
              DEBUG(("setting state to COMPILER for @I for symbol",i_ins));
            else 
              DEBUG(("setting state to HANDWRITTEN for @I for symbol",i_ins));
            */
            state = newstate;
          }
      }
      /* }}} */

      if (bbl && AddressIsGt (BBL_CADDRESS(bbl), INS_CADDRESS(i_ins)))
        FATAL(("instructions not ordered correctly: @I <-> @B", i_ins, bbl));

      if (AddressIsEq (INS_CADDRESS(i_ins), OBJECT_ENTRY(obj)))
      {
        INS_SET_ATTRIB(i_ins, INS_ATTRIB(i_ins) | IF_BBL_LEADER);
        bbl = BblNew (cfg);
        if (!entry)
          entry = bbl;
        BBL_SET_OLD_ADDRESS(bbl, INS_OLD_ADDRESS(i_ins));
        BBL_SET_CADDRESS(bbl, INS_CADDRESS(i_ins));
        BBL_SET_CSIZE(bbl, INS_CSIZE(i_ins));
        INS_SET_BBL(i_ins, bbl);
        nbbls++;
      }
      else if (INS_ATTRIB(i_ins) & IF_BBL_LEADER)
      {
        bbl = BblNew (cfg);
        BBL_SET_OLD_ADDRESS(bbl, INS_OLD_ADDRESS(i_ins));
        BBL_SET_CADDRESS(bbl, INS_CADDRESS(i_ins));
        BBL_SET_CSIZE(bbl, INS_CSIZE(i_ins));
        INS_SET_BBL(i_ins, bbl);
        nbbls++;
      }
      else
        INS_SET_BBL(i_ins, NULL);

      BBL_SET_OVERLAY(bbl, ovlsec);

      if (state == HANDWRITTEN)
        BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_IS_HANDWRITTEN_ASSEMBLY);
    }
  }

  /* now copy the instructions into the basic blocks */
  OBJECT_FOREACH_CODE_SECTION(obj, sec, j)
  {
    current_address = SECTION_CADDRESS(sec);
    SECTION_FOREACH_INS(sec, i_ins)
    {
      if (INS_BBL(i_ins))
        bbl = INS_BBL(i_ins);
      InsAppendToBbl (InsDup (i_ins), bbl);
      INS_SET_BBL(i_ins, bbl);
      /* following is necessary because InsDup doesn't copy address field */
      INS_SET_OLD_ADDRESS(BBL_INS_LAST(bbl), INS_OLD_ADDRESS(i_ins));
      INS_SET_CADDRESS(BBL_INS_LAST(bbl), INS_CADDRESS(i_ins));
      INS_SET_OLD_SIZE(BBL_INS_LAST(bbl), INS_OLD_SIZE(i_ins));
      current_address = AddressAdd (INS_CADDRESS(i_ins), INS_CSIZE(i_ins));
    }
  }

  OBJECT_FOREACH_CODE_SECTION(obj, sec, j)
  {

    SECTION_FOREACH_INS(sec, i_ins)
    {
      if (AddressIsEq(INS_CADDRESS(i_ins), OBJECT_ENTRY(obj)) || (INS_ATTRIB(i_ins) & IF_BBL_LEADER))
      {
        bbl = INS_BBL(i_ins);
        if  (INS_TYPE(i_ins) == IT_DATA) 
        {

          t_section * subsec = NULL;
          SECTION_FOREACH_SUBSECTION(sec,subsec)
            {
	      if (AddressIsLe(SECTION_CADDRESS(subsec),INS_CADDRESS(i_ins)) && AddressIsLt(INS_CADDRESS(i_ins),AddressAdd(SECTION_CADDRESS(subsec),SECTION_CSIZE(subsec))))
                {
                  //                  DEBUG(("SUB %s of %s with alignment %d for ins @I",SECTION_NAME(sec),SECTION_NAME(subsec),SECTION_ALIGNMENT(subsec),i_ins));
                  break;
                }
            }
          /* this can happen for linker-inserted stubs that we did not need
           * -> ignore
           */
          if (!subsec) continue;

          /* TODO: not for branch tables, in particular in Thumb */

          if (SECTION_ALIGNMENT(subsec)==4 && BBL_CSIZE(bbl)>=4)
          {
            BBL_SET_ALIGNMENT(bbl,4);
            BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0x3); 
          }
          else if (SECTION_ALIGNMENT(subsec)==8)
          {
            if (BBL_CSIZE(bbl)>=8)
              {
                BBL_SET_ALIGNMENT(bbl,8);
                BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0x7); 
              }
            else if (BBL_CSIZE(bbl)>=4)
              {
                BBL_SET_ALIGNMENT(bbl,4);
                BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0x3); 
              }
          }
          else if (SECTION_ALIGNMENT(subsec)==16)
          {
            BBL_SET_ALIGNMENT(bbl,16);
            BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0xf);
          }
          else if (SECTION_ALIGNMENT(subsec)==32)
          {
            BBL_SET_ALIGNMENT(bbl,32);
            BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0x1f);
          }
          else if (SECTION_ALIGNMENT(subsec)==64)
          {
            BBL_SET_ALIGNMENT(bbl,64);
            BBL_SET_ALIGNMENT_OFFSET(bbl,G_T_UINT32(INS_CADDRESS(i_ins)) & 0x3f);
          }
	  //	  DEBUG(("BBL AT @I : align %x offset %x",i_ins,BBL_ALIGNMENT(bbl),BBL_ALIGNMENT_OFFSET(bbl)));
        }
      }
    }
  }

  CfgCreateHellNodesAndEntry (obj, cfg, current_address, entry);

  /* remove all relocations coming FROM instructions: address producers will be used instead */
  OBJECT_FOREACH_RELOC_SAFE(obj, reloc, tmp)
  {
    /* only remove the reloc from the old instructions, ie. the ones in the linear list */
    if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(reloc)) == RT_INS) && (INS_COPY(T_INS(RELOC_FROM(reloc)))))
    {
      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj), reloc);
    }
  }

  /* Move the relocations to the newly created basic blocks */
  /* Relocations TO instructions should instead point at the instructions' basic blocks.
   * This is because an instruction that has a relocation pointing to it, is per definition
   * a BBL leader, and we still want the relocation to point to that code sequence (BBL) even
   * if the first instruction is removed later on...
   */
  
  OBJECT_FOREACH_RELOC(obj, reloc)
  {
    t_uint32 i;
    for (i=0; i<RELOC_N_TO_RELOCATABLES(reloc); i++)
    {
      if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(reloc)[i]) == RT_INS) && (INS_COPY(T_INS(RELOC_TO_RELOCATABLE(reloc)[i]))))
      {
        t_address offset;
        t_ins *ins = T_INS(RELOC_TO_RELOCATABLE(reloc)[i]);

        offset = AddressSub (INS_CADDRESS(ins), BBL_CADDRESS(INS_BBL(INS_COPY(ins))));

        if (!IS_DATABBL(INS_BBL(INS_COPY(ins))))
          ASSERT(AddressIsNull (offset), ("Symbol @R points to middle of code block @ieB\n", reloc, INS_BBL(INS_COPY(ins))));

        RelocSetToRelocatable (reloc, i, T_RELOCATABLE(INS_BBL(INS_COPY(ins))));

	RELOC_TO_RELOCATABLE_OFFSET(reloc)[i] = AddressAdd(RELOC_TO_RELOCATABLE_OFFSET(reloc)[i], offset);
      }
    }
  }

  /* also migrate the symbols, as these might come in handy later
   * (e.g. during instrumentation, they can be used to look up functions
   * in the program !
   *
   * LUDO turned this off, because off segfault on armlinux binaries!*/
  {
    t_symbol *sym;

    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym)
    {
      if (SYMBOL_BASE(sym) && RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_INS)
      {
        t_ins *ins = T_INS(SYMBOL_BASE(sym));

        SymbolSetBase(sym, T_RELOCATABLE(INS_BBL(ins)));
        SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), AddressSub (INS_CADDRESS(ins), BBL_CADDRESS(INS_BBL(ins)))));
        /* address may be 1 more in case of a thumb symbol */
        if (AddressIsGt(SYMBOL_OFFSET_FROM_START(sym),AddressNewForObject(obj,1)))
          VERBOSE(3,("Symbol migrated to non-zero offset in BBL, may become invalid if BBL is split or modified: @S",sym));
      }
    }
  }
  /* migrate dynamic symbols if they exist */
  if (OBJECT_DYNAMIC_SYMBOL_TABLE(obj))
  {
    t_symbol *sym;

    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_DYNAMIC_SYMBOL_TABLE(obj), sym)
    {
      if (SYMBOL_BASE(sym) && RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_INS)
      {
        t_ins *ins = T_INS(SYMBOL_BASE(sym));

        SymbolSetBase(sym, T_RELOCATABLE(INS_BBL(ins)));
        SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), AddressSub (INS_CADDRESS(ins), BBL_CADDRESS(INS_BBL(ins)))));
        /* address may be 1 more in case of a thumb symbol */
        ASSERT(AddressIsLe(SYMBOL_OFFSET_FROM_START(sym),AddressNewForObject(obj,1)),("Dynamic symbol refers to non-zero offset in BBL, may become invalid if BBL is later on split or modified: @S",sym));
      }
    }
  }

  Free (symarr);

  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_address oldsize = AddressNullForCfg(cfg);
    t_ins *ins;
    BBL_FOREACH_INS(bbl,ins)
    {
     oldsize = AddressAdd(oldsize, INS_OLD_SIZE(ins));
    }
    BBL_SET_OLD_SIZE(bbl, oldsize);
  }

  Free(compiler_generated_starts);

  return nbbls;
}

void CfgCreateHellNodesAndEntry (t_object* obj, t_cfg* cfg, t_address current_address, t_bbl* entry)
{
  t_cfg_edge* edge = NULL;

  /* create hell nodes */
  CFG_SET_HELL_NODE(cfg, BblNew (cfg));
  BBL_SET_OLD_ADDRESS(CFG_HELL_NODE(cfg), current_address);
  BBL_SET_CADDRESS(CFG_HELL_NODE(cfg), current_address);
  BBL_SET_IS_HELL(CFG_HELL_NODE(cfg), TRUE);

  /* Regular call hell node. For dynamically linked functions that are called
   * specific call nodes will be constructed */
  CFG_SET_CALL_HELL_NODE(cfg, BblNew (cfg));
  BBL_SET_OLD_ADDRESS(CFG_CALL_HELL_NODE(cfg), AddressAddUint32 (current_address, 4));
  BBL_SET_CADDRESS(CFG_CALL_HELL_NODE(cfg), AddressAddUint32 (current_address, 4));
  BBL_SET_IS_HELL(CFG_CALL_HELL_NODE(cfg), TRUE);
  BBL_SET_CALL_HELL_TYPE(CFG_CALL_HELL_NODE(cfg), BBL_CH_NORMAL);

  CFG_SET_UNIQUE_ENTRY_NODE(cfg, BblNew (cfg));
  BBL_SET_OLD_ADDRESS(CFG_UNIQUE_ENTRY_NODE(cfg), AddressAddUint32 (current_address, 8));
  BBL_SET_CADDRESS(CFG_UNIQUE_ENTRY_NODE(cfg), AddressAddUint32 (current_address, 8));
  BBL_SET_IS_HELL(CFG_UNIQUE_ENTRY_NODE(cfg), TRUE);

  CFG_SET_SWI_HELL_NODE(cfg, BblNew (cfg));
  BBL_SET_OLD_ADDRESS(CFG_SWI_HELL_NODE(cfg), AddressAddUint32 (current_address, 12));
  BBL_SET_CADDRESS(CFG_SWI_HELL_NODE(cfg), AddressAddUint32 (current_address, 12));
  BBL_SET_IS_HELL(CFG_SWI_HELL_NODE(cfg), TRUE);

  CFG_SET_LONGJMP_HELL_NODE(cfg, BblNew (cfg));
  BBL_SET_OLD_ADDRESS(CFG_LONGJMP_HELL_NODE(cfg), AddressAddUint32 (current_address, 12));
  BBL_SET_CADDRESS(CFG_LONGJMP_HELL_NODE(cfg), AddressAddUint32 (current_address, 12));
  BBL_SET_IS_HELL(CFG_LONGJMP_HELL_NODE(cfg), TRUE);

  /* return_hell_node is created by the function creation routines */
  CFG_SET_CALL_HELL_FUNCTION(cfg, FunctionMake (CFG_CALL_HELL_NODE(cfg), "--CALL-HELL--", FT_HELL));
  CFG_SET_SWI_HELL_FUNCTION(cfg, FunctionMake (CFG_SWI_HELL_NODE(cfg), "--SWI-HELL--", FT_HELL));
  CFG_SET_LONGJMP_HELL_FUNCTION(cfg, FunctionMake (CFG_LONGJMP_HELL_NODE(cfg), "--LONGJMP-HELL--", FT_HELL));
  CFG_SET_HELL_FUNCTION(cfg, FunctionMake (CFG_HELL_NODE(cfg), "--HELL--", FT_HELL));
  CFG_SET_WRAP_FUNCTION(cfg, FunctionMake (CFG_UNIQUE_ENTRY_NODE(cfg), "--WRAPPER--", FT_HELL));

  CFG_SET_HELL_FUNCTIONS(cfg, CFG_CALL_HELL_FUNCTION(cfg));
  FUNCTION_SET_NEXT_HELL(CFG_CALL_HELL_FUNCTION(cfg), CFG_SWI_HELL_FUNCTION(cfg));
  FUNCTION_SET_NEXT_HELL(CFG_SWI_HELL_FUNCTION(cfg), CFG_LONGJMP_HELL_FUNCTION(cfg));
  FUNCTION_SET_NEXT_HELL(CFG_LONGJMP_HELL_FUNCTION(cfg), CFG_HELL_FUNCTION(cfg));
  FUNCTION_SET_NEXT_HELL(CFG_HELL_FUNCTION(cfg), CFG_WRAP_FUNCTION(cfg));
  FUNCTION_SET_NEXT_HELL(CFG_WRAP_FUNCTION(cfg), NULL);

  CFG_SET_EXIT_HELL_NODE(cfg, FUNCTION_BBL_LAST(CFG_HELL_FUNCTION(cfg)));
  BBL_SET_IS_HELL(CFG_EXIT_HELL_NODE(cfg), TRUE);
  CFG_SET_EXIT_CALL_HELL_NODE(cfg, FUNCTION_BBL_LAST(CFG_CALL_HELL_FUNCTION(cfg)));
  BBL_SET_IS_HELL(CFG_EXIT_CALL_HELL_NODE(cfg), TRUE);
  CFG_SET_EXIT_SWI_HELL_NODE(cfg, FUNCTION_BBL_LAST(CFG_SWI_HELL_FUNCTION(cfg)));
  BBL_SET_IS_HELL(CFG_EXIT_SWI_HELL_NODE(cfg), TRUE);
  CFG_SET_EXIT_LONGJMP_HELL_NODE(cfg, FUNCTION_BBL_LAST(CFG_LONGJMP_HELL_FUNCTION(cfg)));
  BBL_SET_IS_HELL(CFG_EXIT_LONGJMP_HELL_NODE(cfg), TRUE);

  CFG_SET_UNIQUE_EXIT_NODE(cfg, FUNCTION_BBL_LAST(CFG_WRAP_FUNCTION(cfg)));
  BBL_SET_IS_HELL(CFG_UNIQUE_EXIT_NODE(cfg), TRUE);

  /* insert a Normal edge between call_hell_node and return_hell_node */

  CfgEdgeCreateCall (cfg, CFG_CALL_HELL_NODE(cfg), CFG_HELL_NODE(cfg), CFG_EXIT_CALL_HELL_NODE(cfg), CFG_EXIT_HELL_NODE(cfg));

  VERBOSE(1, ("Adding arrow from hell! (Entry)"));

  if (!entry)
    FATAL(("Entry point @G not found!", OBJECT_ENTRY(obj)));
  edge = CfgEdgeCreateCall (cfg, CFG_UNIQUE_ENTRY_NODE(cfg), entry, NULL, NULL);
  /* we don't want a corresponding edge here, there's nowhere for it to point to */
  if (CFG_EDGE_CORR(edge))
  {
    CfgEdgeKill (CFG_EDGE_CORR(edge));
    CFG_EDGE_SET_CORR(edge, NULL);
  }

  CFG_SET_ENTRY(cfg, NewEntry (entry, edge));
}

/*}}}*/

t_bbl *CfgGetDynamicCallHell(t_cfg *cfg, t_string fname)
{
  t_bbl *ret;
  t_function *fun, *last = NULL;
  t_string hellname = StringConcat2("--DYNCALL-HELL--", fname);

  for (fun = CFG_HELL_FUNCTIONS(cfg); fun; last = fun, fun = FUNCTION_NEXT_HELL(fun))
  {
    if (!FUNCTION_IS_HELL(fun)) continue;
    if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)) != BBL_CH_DYNCALL) continue;
    if (strcmp(hellname, FUNCTION_NAME(fun))) continue;

    break;
  }
  if (fun)
    ret = FUNCTION_BBL_FIRST(fun);
  else
  {
    /* create a new hell function for this function name */
    t_bbl *hell = CFG_HELL_NODE(cfg);
    t_bbl *newhell = BblNew(cfg);

    BBL_SET_CADDRESS(newhell, AddressAdd(BBL_CADDRESS(hell),100));
    BBL_SET_OLD_ADDRESS(newhell, AddressAdd(BBL_OLD_ADDRESS(hell),100));
    BBL_SET_IS_HELL(newhell, TRUE);
    BBL_SET_CALL_HELL_TYPE(newhell, BBL_CH_DYNCALL);

    fun = FunctionMake(newhell, hellname, FT_HELL);
    FUNCTION_SET_NEXT_HELL(last, fun);
    BBL_SET_IS_HELL(FUNCTION_BBL_LAST(fun), TRUE);
    CfgEdgeCreate(cfg, newhell, FUNCTION_BBL_LAST(fun), ET_FALLTHROUGH);

    ret = newhell;
  }

  Free(hellname);
  return ret;
}

/* Structural optimizations on flowgraphs: {{{ */

/* Remove dead code and data blocks {{{ */

void
MarkFrom (t_cfg * cfg, t_bbl * from)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_function *fun;

  if (!BBL_FUNCTION(from))
    FATAL(("BBL @ieB marked by dead code and data removal, but not in function!", from));
    
  CfgMarkFun (cfg, BBL_FUNCTION(from));
  FunctionMarkBbl (BBL_FUNCTION(from), from);

  BblMark2 (from);

  while (CfgUnmarkFun (cfg, &fun))
  {
    while (FunctionUnmarkBbl (fun, &bbl))
    {
      /* VERBOSE(0,("GOING FOR @B\n",bbl)); */
      if (BBL_IS_HELL (bbl))
        continue;
      if (BblIsMarked2 (bbl))
      {
        BBL_FOREACH_SUCC_EDGE(bbl, edge)
        {
          if (!CfgEdgeIsBackwardInterproc (edge) || (CFG_EDGE_CORR(edge) && CfgEdgeIsMarked (CFG_EDGE_CORR(edge))))
          {
            if (!CfgEdgeIsMarked (edge))
            {
              CfgEdgeMark (edge);

              if (CfgEdgeIsForwardInterproc (edge) && CFG_EDGE_CORR(edge) && (BBL_IS_HELL (CFG_EDGE_TAIL(edge)) || BblIsMarked2 (CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)))))
                if (!CfgEdgeIsMarked (CFG_EDGE_CORR(edge)))
                {
                  CfgEdgeMark (CFG_EDGE_CORR(edge));
                  if (!BblIsMarked2 (CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))))
                  {
                    FunctionMarkBbl (fun, CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
                    BblMark2 (CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
                    VERBOSE(2,("MARKED 2 @iB\n",CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))));
                  }
                }
            }

            if (!BblIsMarked2 (CFG_EDGE_TAIL(edge)))
            {
              FunctionMarkBbl (BBL_FUNCTION(CFG_EDGE_TAIL(edge)), CFG_EDGE_TAIL(edge));
              BblMark2 (CFG_EDGE_TAIL(edge));
              VERBOSE(2,("MARKED 1 @iB\n",CFG_EDGE_TAIL(edge))); 
              if (CfgEdgeIsInterproc (edge))
                CfgMarkFun (cfg, BBL_FUNCTION(CFG_EDGE_TAIL(edge)));

            }
          }
        }
      }
    }
  }
}

static t_bool
Reviver (t_object * obj, t_cfg * fg, t_reloc * reloc);
static t_bool 
ReviveRelocatable(t_object * obj, t_cfg * fg, t_relocatable * points_to, t_bool reloc_hell)
{
  t_bool ret = FALSE;
  if (RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_BBL)
  {
    if (!BblIsMarked2 (points_to))
    {
      t_ins *ins = BBL_INS_FIRST(T_BBL(points_to));

      if (!ins || (INS_TYPE(ins) != IT_DATA))
      {
        /* if the reloc does not cause a hell edge (e.g. relocs from switch tables), there is no need to
         * revive the block it points to (it will be reachable because of the switch edge that enters it) */
        if (reloc_hell)
        {
          MarkFrom (fg, T_BBL(points_to));
          BblMark2 (points_to);
          ret = TRUE;
          VERBOSE(2, ("Marked @iB\n", T_BBL(points_to)));
        }
        else
        {
          VERBOSE(2, ("Skipped @iB\n", T_BBL(points_to)));
        }
      }
      else if (!BblIsMarked2 (points_to))
      {
        /* data block in code: mark as reachable, loop in CfgRemoveDeadCodeAndDataBlocks will do the rest */
        BblMark2 (points_to);
        VERBOSE(2, ("Marked @iB\n", T_BBL(points_to)));
        ret = TRUE;
      }
    }
  }
  else if ((RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SUBSECTION) &&
           ((SECTION_TYPE(T_SECTION(points_to)) == DATA_SECTION) || (SECTION_TYPE(T_SECTION(points_to)) == RODATA_SECTION) ||
           (SECTION_TYPE(T_SECTION(points_to)) == TLSDATA_SECTION)) )
  {
    if (!BblIsMarked2 (points_to))
    {
      t_reloc_ref *ref = RELOCATABLE_REFERS_TO(points_to);

      BblMark2 (points_to);
      VERBOSE(2, ("Marked @T\n", T_BBL(points_to)));
      ret = TRUE;
      /* Search relocations from points_to */

      while (ref)
      {
        if (points_to != RELOC_FROM(RELOC_REF_RELOC(ref)))
          FATAL(("Relocs are corrupt: RELOCS @R, searched with section %s", RELOC_REF_RELOC(ref), SECTION_NAME(T_SECTION(points_to))));
        Reviver (obj, fg, RELOC_REF_RELOC(ref));
        ref = RELOC_REF_NEXT(ref);
      }
    }
  }
  else if (RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SECTION && SECTION_TYPE(T_SECTION(points_to)) == RODATA_SECTION)
    {
      /* This occurs when the whole exidx or extab sections are revived */
      if (!BblIsMarked2 (points_to))
        {
          t_section * sub;

          SECTION_FOREACH_SUBSECTION(T_SECTION(points_to),sub)
            {
              if (!BblIsMarked2(sub))
                ReviveRelocatable (obj, fg, T_RELOCATABLE(sub),reloc_hell);
            }
        }
    }
  else if ((RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SUBSECTION) && (SECTION_TYPE(T_SECTION(points_to)) == BSS_SECTION))
  {
    if (!BblIsMarked2 (points_to))
    {
      t_reloc_ref *ref = RELOCATABLE_REFERS_TO(points_to);

      BblMark2 (points_to);
      VERBOSE(2, ("Marked @T\n", T_BBL(points_to)));
      ret = TRUE;
      /* Search relocations from points_to */

      while (ref)
      {
        if (points_to != RELOC_FROM(RELOC_REF_RELOC(ref)))
          FATAL(("Relocs are corrupt: RELOCS @R, searched with section %s", RELOC_REF_RELOC(ref), SECTION_NAME(T_SECTION(points_to))));
        Reviver (obj, fg, RELOC_REF_RELOC(ref));
        ref = RELOC_REF_NEXT(ref);
      }

    }
  }
  else if ((RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SUBSECTION) && (SECTION_TYPE(T_SECTION(points_to)) == TLSBSS_SECTION))
  {
    if (!BblIsMarked2 (points_to))
    {
      t_reloc_ref *ref = RELOCATABLE_REFERS_TO(points_to);

      BblMark2 (points_to);
      VERBOSE(2, ("Marked @T\n", T_BBL(points_to)));
      ret = TRUE;
      /* Search relocations from points_to */

      while (ref)
      {
        if (points_to != RELOC_FROM(RELOC_REF_RELOC(ref)))
          FATAL(("Relocs are corrupt: RELOCS @R, searched with section %s", RELOC_REF_RELOC(ref), SECTION_NAME(T_SECTION(points_to))));
        Reviver (obj, fg, RELOC_REF_RELOC(ref));
        ref = RELOC_REF_NEXT(ref);
      }

    }
  }
  else if (RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SECTION)
  {
  }
  else if (RELOCATABLE_RELOCATABLE_TYPE(points_to) == RT_SUBSECTION)
  {
  }
  else
    FATAL(("IMPLEMENT relocatable type @T in DeadCodeAndDataRemoval", points_to));
  return ret;
}


static t_bool
Reviver (t_object * obj, t_cfg * fg, t_reloc * reloc)
{
  t_bool ret = FALSE;
  t_uint32 i;

  for (i=0; i<RELOC_N_TO_RELOCATABLES(reloc); i++)
  {
    t_relocatable *points_to = RELOC_TO_RELOCATABLE(reloc)[i];

    if (points_to)
      ret |= ReviveRelocatable(obj, fg, points_to, RELOC_HELL(reloc));
  }
  return ret;
}

t_bool
CfgRemoveDeadCodeAndDataBlocks (t_cfg * fg)
{
  t_bbl *safe, *bbl;
  t_cfg_edge *edge;

  t_function *fun, *tmpfun;
  t_object *obj = CFG_OBJECT(fg);
  t_ins *ins;
  t_bool change = FALSE;
  int unmarked = 0, unmarked_c = 0, killed_ins = 0, killed_bbl = 0, killed_fun = 0, killed_edge = 0, killed_sec = 0;
  int total = 0;

  STATUS(START, ("Remove Dead Code And Data Blocks"));
  CfgUnmarkAllFun (fg);

  CFG_FOREACH_FUN(fg, fun)
  {
    FunctionUnmarkAllBbls (fun);
  }

  CFG_FOREACH_FUN(fg, fun)
    FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED));

  BblMarkInit2 ();

  CfgEdgeMarkInit ();

  /* First of all, mark all reachable blocks from the entry point, not
   * including those that can only be reached from the hell node*/

  /* Make the fake entry node itself */
  MarkFrom (fg, CFG_UNIQUE_ENTRY_NODE(fg));
  /* Don't start marking from CFG_UNIQUE_ENTRY_NODE(fg), because that
   * one is in a hell function and hence MarkFrom wouldn't do anything
   * (except for marking that node itself, as done above)
   */
  BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(fg),edge)
    MarkFrom (fg, CFG_EDGE_TAIL(edge));

  /* Mark all blocks that have the FORCE_REACHABLE flag */
  CFG_FOREACH_BBL(fg, bbl)
  {
    if ((BBL_ATTRIB(bbl) & BBL_FORCE_REACHABLE) && !BblIsMarked2 (bbl))
    {
      if (IS_DATABBL(bbl))
        BblMark2 (bbl);
      else
        MarkFrom (fg, bbl);
    }
  }

  DiabloBrokerCall("RevivePlatformSections",obj,fg,ReviveRelocatable);
  DiabloBrokerCall("ArmReviveFromThumbStubs",fg);
#if 0
  /* {{{ debugging code */
  {
    static int reviv=0;
    t_section *sec, *sub;
    int i;
    OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      if (SectionGetGeneralType(sec) == CODE_SECTION) continue;
      SECTION_FOREACH_SUBSECTION(sec,sub)
        if (reviv++ < diablosupport_options.debugcounter)
        {
          ReviveRelocatable(obj,fg,T_RELOCATABLE(sub),TRUE);
          VERBOSE(0,("Xreviving @T", sub));
        }
    }
  }
  /* }}} */
#endif

  /* Next, look at all the marked blocks, and mark all things that can be
   * reached by using addresses that are produced somewhere in the marked
   * blocks */
  do
  {
    unmarked = 0;
    unmarked_c = 0;
    total = 0;
    change = FALSE;

    CFG_FOREACH_BBL(fg, bbl)
    {
      total++;
      /* if it is not marked, there's no need to follow this block's
       * relocations */
      if (!BblIsMarked2 (bbl))
      {
        if ((BBL_INS_FIRST(bbl)) && (INS_TYPE(BBL_INS_FIRST(bbl)) != IT_DATA))
        {
          unmarked_c++;
        }
        unmarked++;
      }
      /* Else we need to look at the relocation from this block */
      else
      {

        /* Look at each instruction to find out what relocations there are in
         * each block */

        BBL_FOREACH_INS(bbl, ins)
        {
          t_reloc_ref *ref;

          if ((ref = INS_REFERS_TO(ins)))
          {
            while (ref)
            {
              /*t_bool oldchange=change; */
              /* Reviver handles the marking of the refered object and
               * will also recursively mark all things that are referenced
               * in the object */

              change |= Reviver (obj, fg, RELOC_REF_RELOC(ref));
              /* if (change && (!oldchange)) VERBOSE(0,("Revived by @R\n",RELOC_REF_RELOC(ref))); */
              ref = RELOC_REF_NEXT(ref);
            }
          }
        }
      }
    }
    /*printf("Still %d blocks unmarked (%d code) out of %d total\n",unmarked,unmarked_c,total); */
  }
  while (change);

  /* remove all instructions and data that are unmarked.
   * to avoid FATALs, we do this in the following order:
   * 1. remove all unreachable data & instructions but don't kill the blocks
   * 2. remove the (now empty) blocks and their incoming and outgoing edges
   * we have to keep this order to avoid things being killed while there are still
   * relocations pointing to them
   */
  /* {{{ kill instructions and data sections */
  /*static int killed = 0;*/
  CFG_FOREACH_BBL(fg, bbl)
  {
    /* Every bbl that is not marked can be killed */
    if (!BblIsMarked2 (bbl))
    {
      t_ins *ins, *tmp;
#if 0
      /* {{{ debugcounter code */
      if (killed++ >= diablosupport_options.debugcounter)
      {
        BblMark2(bbl);
        continue;
      }
      /* }}} */
#endif
      VERBOSE(1,("-Killing @ieB", bbl));
      BBL_FOREACH_INS_SAFE(bbl, ins, tmp)
      {
        InsKill (ins); /* this automatically kills relocs */
        killed_ins++;
      }
    }
  }

  {
    t_object *sub, *tmp;
    t_uint32 tel;

    /* This checks for dead sections in the subobjects. When found, we remove
     * the relocations in those sections. By doing this before we kill the
     * sections, we avoid cyclic references */

    OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp)
    {
      for (tel = 0; tel < OBJECT_NRODATAS(sub); tel++)
      {
        if ((SECTION_IS_MAPPED(OBJECT_RODATA(sub)[tel]))
            && !BblIsMarked2 (OBJECT_RODATA(sub)[tel])
            && !(SECTION_FLAGS(OBJECT_RODATA(sub)[tel]) & SECTION_FLAG_KEEP))
        {
          while (SECTION_REFERS_TO(OBJECT_RODATA(sub)[tel]))
          {
            RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj), RELOC_REF_RELOC(SECTION_REFERS_TO(OBJECT_RODATA(sub)[tel])));
          }
        }
      }

      for (tel = 0; tel < OBJECT_NDATAS(sub); tel++)
      {
        if ((SECTION_IS_MAPPED(OBJECT_DATA(sub)[tel]))
            && !BblIsMarked2 (OBJECT_DATA(sub)[tel])
            && !(SECTION_FLAGS(OBJECT_DATA(sub)[tel]) & SECTION_FLAG_KEEP))
        {
          while (SECTION_REFERS_TO(OBJECT_DATA(sub)[tel]))
            RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj), RELOC_REF_RELOC(SECTION_REFERS_TO(OBJECT_DATA(sub)[tel])));

        }
      }

      for (tel = 0; tel < OBJECT_NTLSDATAS(sub); tel++)
      {
        if ((SECTION_IS_MAPPED(OBJECT_TLSDATA(sub)[tel]))
            && !BblIsMarked2 (OBJECT_TLSDATA(sub)[tel])
            && !(SECTION_FLAGS(OBJECT_TLSDATA(sub)[tel]) & SECTION_FLAG_KEEP))
        {
          while (SECTION_REFERS_TO(OBJECT_TLSDATA(sub)[tel]))
            RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj), RELOC_REF_RELOC(SECTION_REFERS_TO(OBJECT_TLSDATA(sub)[tel])));

        }
      }

    }

    /* Now kill the sections */

    OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp)
    {
      t_bool restart = FALSE;
      do
      {
        restart = FALSE;
        for (tel = 0; tel < OBJECT_NRODATAS(sub); tel++)
        {
          if ((SECTION_IS_MAPPED(OBJECT_RODATA(sub)[tel]))
              && !BblIsMarked2 (OBJECT_RODATA(sub)[tel])
              && !(SECTION_FLAGS(OBJECT_RODATA(sub)[tel]) & SECTION_FLAG_KEEP))
          {
            VERBOSE(1, ("Killing @T", OBJECT_RODATA(sub)[tel])); 
            SectionKill(OBJECT_RODATA(sub)[tel]);
            killed_sec ++ ;
            restart=TRUE;
            break;
          }
        }
      } while(restart);

      do
      {
        restart = FALSE;
        for (tel = 0; tel < OBJECT_NDATAS(sub); tel++)
        {
          if ((SECTION_IS_MAPPED(OBJECT_DATA(sub)[tel]))
              && !BblIsMarked2 (OBJECT_DATA(sub)[tel])
              && !(SECTION_FLAGS(OBJECT_DATA(sub)[tel]) & SECTION_FLAG_KEEP))
          {
            VERBOSE(1, ("Killing @T", OBJECT_DATA(sub)[tel])); 
            SectionKill(OBJECT_DATA(sub)[tel]);
            killed_sec ++ ;
            restart=TRUE;
            break;
          }
        }
      } while(restart);

      do
      {
        restart = FALSE;
        for (tel = 0; tel < OBJECT_NTLSDATAS(sub); tel++)
        {
          if ((SECTION_IS_MAPPED(OBJECT_TLSDATA(sub)[tel]))
              && !BblIsMarked2 (OBJECT_TLSDATA(sub)[tel])
              && !(SECTION_FLAGS(OBJECT_TLSDATA(sub)[tel]) & SECTION_FLAG_KEEP))
          {
            VERBOSE(1, ("Killing @T", OBJECT_TLSDATA(sub)[tel])); 
            SectionKill(OBJECT_TLSDATA(sub)[tel]);
            killed_sec ++ ;
            restart=TRUE;
            break;
          }
        }
      } while(restart);

      do
      {
        restart = FALSE;
        for (tel = 0; tel < OBJECT_NBSSS(sub); tel++)
        {
          if ((SECTION_IS_MAPPED(OBJECT_BSS(sub)[tel]))
              && !BblIsMarked2 (OBJECT_BSS(sub)[tel])
              && !(SECTION_FLAGS(OBJECT_BSS(sub)[tel]) & SECTION_FLAG_KEEP))
          {
            VERBOSE(1, ("Killing @T", OBJECT_BSS(sub)[tel])); 
            SectionKill(OBJECT_BSS(sub)[tel]);
            killed_sec ++ ;
            restart=TRUE;
            break;
          }
        }
      } while(restart);
    }
  } /* }}} */

  /* {{{ remove empty blocks and edges */
  CFG_FOREACH_BBL_SAFE(fg, bbl, safe)
  {
    t_cfg_edge *edge, *tmp;

    if (BblIsMarked2 (bbl))
      continue;
    if (BBL_IS_HELL (bbl))
      continue;
    if (bbl == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)) || bbl == CFG_UNIQUE_EXIT_NODE(BBL_CFG(bbl)))
      continue;

    if (BBL_NINS(bbl))
      FATAL(("@eiB unmarked but apparently not empty %d", bbl, BBL_NINS(bbl)));

    BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, tmp)
    {
      if (CFG_EDGE_CORR(edge))
      {
        if (CFG_EDGE_REFCOUNT(edge) > 1) continue;
        if (CfgEdgeIsForwardInterproc (edge))
        {
          if (CFG_EDGE_CORR(edge) == tmp)
            tmp = T_CFG_EDGE(CFG_EDGE_SUCC_NEXT(tmp));
          CfgEdgeKill (CFG_EDGE_CORR(edge));
          CFG_EDGE_SET_CORR(edge, NULL);
          killed_edge++;
        }
        else
          CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), NULL);
      }

      CfgEdgeKill (edge);
      killed_edge++;
    }
  }

  CFG_FOREACH_BBL_SAFE(fg, bbl, safe)
  {
    t_cfg_edge *edge, *tmp;


    if (BblIsMarked2 (bbl))
      continue;

    if (BBL_IS_HELL (bbl))
      continue;

    if (bbl == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)) || bbl == CFG_UNIQUE_EXIT_NODE(BBL_CFG(bbl)))
      continue;

    BBL_FOREACH_PRED_EDGE_SAFE(bbl, edge, tmp)
    {
      if (CFG_EDGE_CORR(edge))
      {
        if (CFG_EDGE_REFCOUNT(edge) > 1) continue;
        if (CfgEdgeIsForwardInterproc (edge))
        {
          if (CFG_EDGE_CORR(edge) == tmp)
            tmp = T_CFG_EDGE(CFG_EDGE_PRED_NEXT(tmp));
          CfgEdgeKill (CFG_EDGE_CORR(edge));
          killed_edge++;
        }
        else
          CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), NULL);
      }
      CfgEdgeKill (edge);
      killed_edge++;
    }
  }

  CFG_FOREACH_BBL_SAFE(fg, bbl, safe)
  {
    if (BblIsMarked2 (bbl))
      continue;
    if (BBL_IS_HELL (bbl))
      continue;
    if (bbl == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)) || bbl == CFG_UNIQUE_EXIT_NODE(BBL_CFG(bbl)))
      continue;

    /* make sure relocations are removed as well (incl. here in particular the ones
       that do not come with hell edges */
    while (BBL_REFED_BY(bbl))
      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(BBL_REFED_BY(bbl)));

    if (!BBL_PRED_FIRST(bbl) && !BBL_SUCC_FIRST(bbl))
    {
      BblKill (bbl);
      killed_bbl++;
    }
  }

  /* clean up empty functions */

  CFG_FOREACH_FUNCTION_SAFE(fg, fun, tmpfun)
    if (FUNCTION_BBL_FIRST(fun) == NULL)
    {
      FunctionKill (fun);
      killed_fun++;
    }

  /* }}} */

  VERBOSE(0, ("removed %d instructions, %d bbls, %d edges, %d functions and %d section", killed_ins, killed_bbl, killed_edge, killed_fun, killed_sec));
  STATUS(STOP, ("Remove Dead Code And Data Blocks"));
  return (killed_ins || killed_bbl || killed_edge || killed_fun || killed_sec);
}

void
CfgMarkReachableBlockWithoutGoingThrough (t_cfg * cfg, t_bbl * stop_bbl, t_uint32 stop_number, t_bbl * test_bbl, t_uint32 test_number)
{
  t_bbl *bbl;

  t_bool change = TRUE;

  BblMarkInit ();

  BblMark (CFG_UNIQUE_ENTRY_NODE(cfg));
  BblMark (CFG_EXIT_SWI_HELL_NODE(cfg));

  if (stop_bbl == NULL)
  {
    CFG_FOREACH_BBL(cfg, stop_bbl)
      if (BBL_DFS_NUMBER(stop_bbl) == stop_number)
        break;
  }

  if (test_bbl == NULL)
  {
    CFG_FOREACH_BBL(cfg, test_bbl)
      if (BBL_DFS_NUMBER(test_bbl) == test_number)
        break;
  }

  if (test_bbl == CFG_EXIT_SWI_HELL_NODE(cfg))
    return;

  if (stop_bbl == CFG_EXIT_SWI_HELL_NODE(cfg))
    return;

  while (change)
  {
    change = FALSE;

    CFG_FOREACH_BBL(cfg, bbl)
    {
      if (BblIsMarked (bbl) && bbl != stop_bbl)
      {
        t_cfg_edge *succ;


        if (bbl == test_bbl)
        {
          VERBOSE(0, ("TEST @B(%d) can be reached without going through @B(%d)\n", test_bbl, BBL_DFS_NUMBER(test_bbl), stop_bbl, BBL_DFS_NUMBER(stop_bbl)));
          exit (0);
        }

        BBL_FOREACH_SUCC_EDGE(bbl, succ)
        {
          if (CFG_EDGE_CAT(succ) == ET_CALL || CFG_EDGE_CAT(succ) == ET_SWI || CFG_EDGE_CAT(succ) == ET_IPJUMP)
          {
            if (CFG_EDGE_CORR(succ) && BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(succ))))
            {
              if (!BblIsMarked (CFG_EDGE_TAIL(CFG_EDGE_CORR(succ))) && CFG_EDGE_TAIL(CFG_EDGE_CORR(succ)) != stop_bbl)
              {
                /* VERBOSE(0,("1 marking %d -> %d\n",BBL_DFS_NUMBER(bbl),BBL_DFS_NUMBER(CFG_EDGE_TAIL(CFG_EDGE_CORR(succ))))); */
                BblMark (CFG_EDGE_TAIL(CFG_EDGE_CORR(succ)));
                change = TRUE;
              }
            }
          }

          if (CFG_EDGE_CAT(succ) == ET_RETURN || CFG_EDGE_CAT(succ) == ET_COMPENSATING)
          {
            if (CFG_EDGE_CORR(succ) && BblIsMarked (CFG_EDGE_HEAD(CFG_EDGE_CORR(succ))))
            {
              if (!BblIsMarked (CFG_EDGE_TAIL(succ)) && CFG_EDGE_TAIL(succ) != stop_bbl)
              {
                /* VERBOSE(0,("2 marking %d -> %d\n",BBL_DFS_NUMBER(bbl),BBL_DFS_NUMBER(CFG_EDGE_TAIL(succ)))); */
                BblMark (CFG_EDGE_TAIL(succ));
                change = TRUE;
              }
            }
          }
          else
          {
            if (!BblIsMarked (CFG_EDGE_TAIL(succ)) && CFG_EDGE_TAIL(succ) != stop_bbl)
            {
              /* VERBOSE(0,("3 marking %d -> %d\n",BBL_DFS_NUMBER(bbl),BBL_DFS_NUMBER(CFG_EDGE_TAIL(succ)))); */
              BblMark (CFG_EDGE_TAIL(succ));
              change = TRUE;
            }
          }
        }
      }
    }
  }
}
/*}}}*/

/* CfgRemoveUselessConditionalJumps {{{ */
t_bool
CfgRemoveUselessConditionalJumps (t_cfg * cfg)
{
  t_bbl *bbl;
  t_bool change = FALSE;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if ((BBL_SUCC_FIRST(bbl))
        && (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)))
        && (!CFG_EDGE_SUCC_NEXT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)))))
    {
      if (CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl)) == CFG_EDGE_TAIL(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl))))
      {
        if ((CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_JUMP) || (CFG_EDGE_CAT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl))) == ET_JUMP))
          if ((CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_FALLTHROUGH) || (CFG_EDGE_CAT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl))) == ET_FALLTHROUGH))
          {
            change = TRUE;
            /* this code should work for both architectures with and without delay slots */
            if (BBL_INS_LAST(bbl) && INS_TYPE(BBL_INS_LAST(bbl)) == IT_BRANCH)
              InsKill (BBL_INS_LAST(bbl));
            else if (INS_IPREV(BBL_INS_LAST(bbl)) && INS_TYPE(INS_IPREV(BBL_INS_LAST(bbl))) == IT_BRANCH)
              InsKill (INS_IPREV(BBL_INS_LAST(bbl)));
            else
              FATAL(("Could not find jump instruction in block"));

            if (CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_JUMP)
              CfgEdgeKill (BBL_SUCC_FIRST(bbl));
            else
              CfgEdgeKill (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)));
          }
      }
    }
  }
  return change;
}

/*}}}*/

/* CfgRemoveEmptyBlocks {{{ */

t_bool
CfgRemoveEmptyBlocks (t_cfg * cfg)
{
  t_bbl *bbl, *tmp;
  t_bool change = FALSE;

  CFG_FOREACH_BBL_SAFE(cfg, bbl, tmp)
  {
    /* skip return blocks */
    if (BBL_FUNCTION(bbl) && (FUNCTION_BBL_LAST(BBL_FUNCTION(bbl)) == bbl))
      continue;
    /* skip hell nodes */
    if (BBL_IS_HELL(bbl))
      continue;
    /* Fix me, we can change this */
    if (BBL_REFED_BY(bbl))
      continue;
    if ((BBL_PRED_FIRST(bbl)) && (BBL_SUCC_FIRST(bbl))&&(BBL_NINS(bbl) == 0))
    {
      t_cfg_edge *i_edge, *safe;
      t_bbl *tail = NULL;
      t_bbl *head;
      t_uint32 type;
      t_bool ip = FALSE;

      /* the block should have exactly one successor edge, of type fallthrough */
      BBL_FOREACH_SUCC_EDGE_SAFE(bbl, i_edge, safe)
      {
        if (tail)
          FATAL(("Encountered more then one successor edge for empty block: \nEdge:@E\n and tail:@B", i_edge, tail));
        if (CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU)
        {
          ip = TRUE;
        }
        else if (CFG_EDGE_CAT(i_edge) != ET_FALLTHROUGH)
        {
          FATAL(("Empty block has successor that is not fallthrough: @iB --- @E", bbl, i_edge));
        }
        tail = CFG_EDGE_TAIL(i_edge);
        if (!ip)
          CfgEdgeKill (i_edge);
      }
      if (ip)
        continue;

      change = TRUE;
      if (!tail)
        FATAL(("Impossible 3"));
      BBL_FOREACH_PRED_EDGE_SAFE(bbl, i_edge, safe)
      {
        type = CFG_EDGE_CAT(i_edge);
        head = CFG_EDGE_HEAD(i_edge);
        if (type == ET_RETURN)
        {
          CfgEdgeChangeReturnForCall (CFG_EDGE_CORR(i_edge), tail);
        }
        else if (type == ET_CALL)
        {
          t_cfg_edge *return_edge = NULL;
          t_bbl *return_block = NULL;
          t_bbl *return_site = NULL;

          if (CFG_EDGE_CORR(i_edge))
          {
            return_edge = CFG_EDGE_CORR(i_edge);
            return_block = CFG_EDGE_HEAD(return_edge);
            return_site = CFG_EDGE_TAIL(return_edge);

            CfgEdgeKill (CFG_EDGE_CORR(i_edge));
          }
          else
            FATAL(("No corresponding edge for @E", i_edge));
          CfgEdgeKill (i_edge);
          CfgEdgeCreateCall (FUNCTION_CFG(BBL_FUNCTION(bbl)), head, tail, return_site, return_block);

        }
        else if (CfgEdgeIsForwardInterproc (i_edge))
        {
          t_cfg_edge *new_edge;

          if (CFG_EDGE_CORR(i_edge))
          {
            CfgEdgeKill (CFG_EDGE_CORR(i_edge));
            CfgEdgeKill (i_edge);
            new_edge = CfgEdgeCreate (FUNCTION_CFG(BBL_FUNCTION(bbl)), head, tail, type);
            CfgEdgeCreateCompensating (FUNCTION_CFG(BBL_FUNCTION(bbl)), new_edge);
          }
          else
          {
            CfgEdgeKill (i_edge);
            new_edge = CfgEdgeCreate (FUNCTION_CFG(BBL_FUNCTION(bbl)), head, tail, type);
          }
        }
        else if (CfgEdgeIsInterproc (i_edge))
        {
          /* this shouldn't happen: all forward interprocedural cases are handled,
           * and return edges are also handled. the only remaining type is compensating
           * edges and those should only arrive in return blocks. return blocks are
           * skipped so if we fall into such an edge here, something is definitely wrong
           */
          FATAL(("Shouldn't have this kind of edge here! (type = %d)", type));
        }
        else if (CFG_EDGE_FLAGS(i_edge) & EF_FROM_SWITCH_TABLE)
        {
          CfgEdgeKill (i_edge);
          i_edge = CfgEdgeCreate (FUNCTION_CFG(BBL_FUNCTION(bbl)), head, tail, type);
          CFG_EDGE_SET_FLAGS(i_edge, CFG_EDGE_FLAGS(i_edge) | EF_FROM_SWITCH_TABLE);
        }
        else
        {
          /* regular edges. just kill and replace */
          t_uint32 count = CFG_EDGE_EXEC_COUNT(i_edge);

          CfgEdgeKill (i_edge);
          CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate (FUNCTION_CFG(BBL_FUNCTION(bbl)), head, tail, type), count);

        }
      }

      BblKill (bbl);
    }
  }
  return change;
}

/*}}}*/

/*}}}*/

/*! Detection of infinite loops, in a very naive way. We simply look for loops
 * and check if there isn't any path that could escape from the loop.
 * Return value is true if any infinite loop is found and the backedges
 * corresponding to the infinite loops are returned in a linked list */
t_bool
CfgDetectSimpleInfiniteLoops (t_cfg * cfg, t_loopref ** inf_list)
{
  t_loop *loop;
  t_cfg_edge *iterator_edge = NULL;
  t_bool ret = FALSE;
  t_bbl *header, *footer, *ibbl, *asoc_header = NULL;
  t_uint32 loops = 0;
  t_function *ifun;
  t_back_edge *backedge = NULL;

  /* CfgMarkFunsWithoutIncomingJumps(cfg);*/
  CFG_FOREACH_FUN(cfg, ifun)
  {
    FUNCTION_SET_NR_BLOCKS(ifun, 0);
    FUNCTION_FOREACH_BBL(ifun, ibbl)
      FUNCTION_SET_NR_BLOCKS(ifun, FUNCTION_NR_BLOCKS(ifun) + 1);
  }

  /* {{{ Initialise everything for a new calculation */

  CFG_FOREACH_BBL(cfg, ibbl)
  {
    LoopBblCleanup (ibbl);

  /* VERBOSE(0,("%d ",((*((int*)(&ibbl)) & 0xf0) >> 4)));*/
  }

  CFG_FOREACH_FUN(cfg, ifun)
  {
#if 0
    t_loopref *free_me;

    while (FUNCTION_LOOPS(ifun))
    {
      free_me = FUNCTION_LOOPS(ifun);
      FUNCTION_SET_LOOPS(ifun, FUNCTION_LOOPS(ifun)->next);
      Free (free_me);
    }
#endif
  }

  /* printf("\n");*/

  while (CFG_LOOP_FIRST(cfg))
    LoopKill (CFG_LOOP_FIRST(cfg));

  CFG_SET_LOOP_LAST(cfg, NULL);
  /* }}} */

  /* Initialisation done, now loop detection really starts */

  CFG_FOREACH_EDGE(cfg, iterator_edge)
  {
    /* if (CFG_EDGE_CAT(iterator_edge) & (ET_CALL|ET_IPJUMP|ET_RETURN|ET_COMPENSATING|ET_IPFALLTHRU)) continue;*/

    asoc_header = NULL;
    header = CFG_EDGE_TAIL(iterator_edge);
    if (BBL_IS_HELL (header))
      continue;

    footer = CFG_EDGE_HEAD(iterator_edge);

    if (CfgEdgeIsForwardInterproc (iterator_edge) && CFG_EDGE_CORR(iterator_edge))
      asoc_header = CFG_EDGE_TAIL(CFG_EDGE_CORR(iterator_edge));

    /* We only consider traditional loops and loops formed by recursive function calls */
    if (BblDominates (header, footer))
    {
      if (BblIsExitBlock(header))
      {
        /* VERBOSE(0,("Skipping @E from @iB",iterator_edge,CFG_EDGE_TAIL(iterator_edge)));*/
        continue;
      }
      if (!CfgEdgeIsForwardInterproc (iterator_edge))

        /* We have a traditional loop */
      {
        /* It's possible that we already have a loop with the
         * same header and footer. Skip this one */
        if (LoopExists (cfg, iterator_edge, FALSE))
          continue;

        /* VERBOSE(0,("\n------------------------\nTraditional LOOP %d: backedge! @E",loops,iterator_edge));*/
        backedge = (t_back_edge *) Calloc (1, sizeof (t_back_edge));
        backedge->edge = iterator_edge;
        backedge->has_corr = FALSE;
        loop = LoopNew (cfg, backedge, FALSE);
        loops++;

        if (!loop)
          FATAL(("No loop"));

      }
      else if (!(CFG_EDGE_CORR(iterator_edge)))
      {
        if (BBL_FUNCTION(header) == BBL_FUNCTION(footer))
        {
          if (!FunctionIsReentrant (BBL_FUNCTION(header)))
            FATAL(("This function must be reentrant or this graph is completely messed up"));

          /* This loop is a recursive loop, but there is no exit block in the
           * function the recursion occurs. So upon exit of this loop, control
           * flow reaches a function that aborts the program execution.*/
          /* VERBOSE(0,("------------------------\nInfinite recursive LOOP %d: backedge!
           * @E\nThis loop probably escapes via exit()",loops,iterator_edge));*/
          backedge = (t_back_edge *) Calloc (1, sizeof (t_back_edge));
          backedge->edge = iterator_edge;
          backedge->has_corr = FALSE;
          loop = LoopNew (cfg, backedge, TRUE);
          loops++;

          if (!loop)
            FATAL(("No loop"));
        }
        else 
        {
          /* This loop is a recursive loop, but there is no exit block in the
           * function where the backedge arrives. This means that upon exit of
           * this loop, control flow reaches a function that aborts the program
           * execution. This case occurs e.g. in dynamically linked programs,
           * where the cfg is overly conservative and a function that calls a
           * dynamically linked exit function is placed before main (see ffmpeg
           * for the ARM, ask details to Bruno). Eventually, this case can
           * represent a real loop, but this is so rare that we don't consider
           * this to be a loop for the moment. */
          if (!FunctionIsReentrant (BBL_FUNCTION(header)))
            FATAL(("This function must be reentrant or this graph is completely messed up"));
          continue;
/*          FATAL(("Implement loops with a interproc backedge that has no corr edge! @E", iterator_edge));*/
        }
      }
      else if ((!BblDominates (CFG_EDGE_HEAD(CFG_EDGE_CORR(iterator_edge)), footer)))
      {
        /* It's possible that we already have a loop with the
         * same header and footer. Skip this one */
        if (LoopExists (cfg, iterator_edge, FALSE))
          continue;

        /*#ifdef DEBUG_LOOP */
        if (CFG_EDGE_CORR(iterator_edge) && CfgEdgeIsForwardInterproc (iterator_edge))
        {
          if (!FunctionIsReentrant (BBL_FUNCTION(header)))
          {
            /* LoopPrint(loop,0); */
            FATAL(("We have a corredge: @E\n@E", iterator_edge, CFG_EDGE_CORR(iterator_edge)));
            continue;
          }
        }
        /*#endif */

        /* VERBOSE(0,("------------------------\nRecursive LOOP %d: backedge! @E\n",loops,iterator_edge));*/
        backedge = (t_back_edge *) Calloc (1, sizeof (t_back_edge));
        backedge->edge = iterator_edge;
        backedge->has_corr = FALSE;
        loop = LoopNew (cfg, backedge, TRUE);
        loops++;

        if (!loop)
          FATAL(("No loop"));

      }
      else
      {
        /* VERBOSE(0,("Skipping backedge @E, since it is not a recursive call",iterator_edge));*/
      }
    }
    else if (asoc_header && BblDominates (asoc_header, footer))
    {
      if (LoopExists (cfg, iterator_edge, TRUE))
        continue;

      /* VERBOSE(0,("----------------------------\nCorresponding LOOP %d: backedge! @E and @E\n",loops,iterator_edge, CFG_EDGE_CORR(iterator_edge)));*/
      backedge = (t_back_edge *) Calloc (1, sizeof (t_back_edge));
      backedge->edge = iterator_edge;
      /* We set has_corr to true, since the backedge here is in fact formed by a pair of edges */
      backedge->has_corr = TRUE;

      loop = LoopNew (cfg, backedge, FALSE);
      loops++;

      if (!loop)
        FATAL(("No loop"));
    }
  }
  VERBOSE(0, ("%d backedges found", loops));

  LoopFindNestedLoops (cfg);
  LoopMergeLoopsWithSameHeader(cfg);

  CfgLoopSetInfiniteProperty (cfg);

  loops = 0;
  CFG_FOREACH_LOOP(cfg, loop)
  {
    loops++;
    /* If we've found an infinite loop, put it on the list */
    if (LOOP_INF(loop))
    {
#ifdef DEBUG_LOOP
      t_back_edge *backedges = LOOP_BACKEDGES(loop);
#endif
      t_loopref *insert = (t_loopref *) Calloc (1, sizeof (t_loopref));

      ret = TRUE;
      insert->loop = loop;
      insert->next = *inf_list;
      *inf_list = insert;
#ifdef DEBUG_LOOP
      if (backedges->next)
        FATAL(("Didn't expect to have multiple backedges in an infinite loop @E\n", backedges->edge));
      VERBOSE(0, ("Infinite loop with edge @E\n", insert->data));
#endif
    }

  }
  VERBOSE(0, ("Number of loops: %d", loops));

  return ret;
}

t_bool
BblIsSwiExitNode (t_bbl * node)
{
  return BBL_FUNCTION(node) && (node == CFG_EXIT_SWI_HELL_NODE(FUNCTION_CFG(BBL_FUNCTION(node))));
}

void
CfgSortEdges (t_cfg * cfg)
{
  t_cfg_edge *edge, *tmp;
  t_cfg_edge *prev_edge = NULL;

  CFG_FOREACH_EDGE_SAFE(cfg, edge, tmp)
  {
    if (prev_edge && CfgEdgeIsForwardInterproc (edge))
    {
      CFG_EDGE_SET_NEXT(prev_edge, tmp);
      if (tmp)
        CFG_EDGE_SET_PREV(tmp, prev_edge);
      else
        CFG_SET_EDGE_LAST(cfg, prev_edge);

      CFG_EDGE_SET_NEXT(edge, CFG_EDGE_FIRST(cfg));
      CFG_EDGE_SET_PREV(CFG_EDGE_FIRST(cfg), edge);
      CFG_EDGE_SET_PREV(edge, NULL);

      CFG_SET_EDGE_FIRST(cfg, edge);
    }
    else
      prev_edge = edge;
  }
}

/* Utility functions {{{ */

/* a first group of utility procedures is used to created linked lists
   of procedures, incoming edges into procedures and basic blocks in
   procedures that need re-evaluation
   */

void
CfgMarkFun (t_cfg * cfg, t_function * fun)
{
  if (!(FUNCTION_NEXT_MARKED(fun)))
  {
#if 0
	 /* old code */
    FUNCTION_SET_NEXT_MARKED(fun, FUNCTION_NEXT_MARKED(CFG_MARKED_FUNS(cfg)));
    FUNCTION_SET_NEXT_MARKED(CFG_MARKED_FUNS(cfg), fun);
#else
	 if (CFG_LAST_MARKED_FUN(cfg)==NULL)
		{
		  FUNCTION_SET_NEXT_MARKED(fun, FUNCTION_NEXT_MARKED(CFG_MARKED_FUNS(cfg)));
		  FUNCTION_SET_NEXT_MARKED(CFG_MARKED_FUNS(cfg), fun);
		}
	 else
		{
		  FUNCTION_SET_NEXT_MARKED(fun, CFG_MARKED_FUNS(cfg));
		  FUNCTION_SET_NEXT_MARKED(CFG_LAST_MARKED_FUN(cfg), fun);
		}
	 CFG_SET_LAST_MARKED_FUN(cfg,fun);
#endif
  }
}

t_bool
CfgUnmarkFun (t_cfg * cfg, t_function ** fun)
{

  *fun = FUNCTION_NEXT_MARKED(CFG_MARKED_FUNS(cfg));

  if (*fun == CFG_MARKED_FUNS(cfg))
    return FALSE;

  FUNCTION_SET_NEXT_MARKED(CFG_MARKED_FUNS(cfg), FUNCTION_NEXT_MARKED(*fun));
  FUNCTION_SET_NEXT_MARKED(*fun, NULL);

  if (FUNCTION_NEXT_MARKED(CFG_MARKED_FUNS(cfg))==CFG_MARKED_FUNS(cfg))
	 CFG_SET_LAST_MARKED_FUN(cfg,NULL);

  return TRUE;
}

void
CfgUnmarkAllFun (t_cfg * cfg)
{
  t_function *fun;

  if (!(CFG_MARKED_FUNS(cfg)))
  {
    CFG_SET_MARKED_FUNS(cfg, T_FUN(Malloc (sizeof (t_function))));
    FUNCTION_SET_NEXT_MARKED(CFG_MARKED_FUNS(cfg), CFG_MARKED_FUNS(cfg));
  }
  else
    while (CfgUnmarkFun (cfg, &fun));
}

/* This function verifies the correctness of the CFG. In case there are multiple CFGs there can not be any
 * link between them as this might result in problems when one of these CFGs is deflowgraped and freed.
 */
void
CfgVerifyCorrectness (t_cfg * cfg)
{
  t_bbl* bbl;
  t_cfg_edge* edge;
  t_function* fun;

  /* First check all BBL's */
  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_ins* ins;

    if (BBL_CFG(bbl) != cfg)
      FATAL(("Error while verifying CFG correctness. There is a BBL in the CFG's BBL list that doesn't belong to this CFG."));

    BBL_FOREACH_PRED_EDGE(bbl, edge)
      if (CFG_EDGE_CFG(edge) != cfg)
        FATAL(("Error while verifying CFG correctness. There is a BBL with a preceding edge that doesn't belong to this CFG."));

    BBL_FOREACH_SUCC_EDGE(bbl, edge)
      if (CFG_EDGE_CFG(edge) != cfg)
        FATAL(("Error while verifying CFG correctness. There is a BBL with a succeeding edge that doesn't belong to this CFG."));

    BBL_FOREACH_INS(bbl, ins)
      if (INS_CFG(ins) != cfg)
        FATAL(("Error while verifying CFG correctness. There is a BBL with an instruction in it that doesn't belong to this CFG."));
  }

  /* Then check all the edges */
  CFG_FOREACH_EDGE(cfg, edge)
  {
    if (CFG_EDGE_CFG(edge) != cfg)
      FATAL(("Error while verifying CFG correctness. There is an edge in the CFG's edge list that doesn't belong to this CFG."));

    if (BBL_CFG(CFG_EDGE_HEAD(edge)) != cfg)
      FATAL(("Error while verifying CFG correctness. There is an edge with a head that doesn't belong to this CFG."));

    if (BBL_CFG(CFG_EDGE_TAIL(edge)) != cfg)
      FATAL(("Error while verifying CFG correctness. There is an edge with a tail that doesn't belong to this CFG."));
  }

  /* Finally go over all functions */
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_CFG(fun) != cfg)
      FATAL(("Error while verifying CFG correctness. There is a function in the CFG's function list that doesn't belong to this CFG."));

    FUNCTION_FOREACH_BBL(fun, bbl)
      if (BBL_CFG(bbl) != cfg)
        FATAL(("Error while verifying CFG correctness. There is a function with a BBL that doesn't belong to this CFG."));
  }
}

t_bool
FunctionUnmarkBbl (t_function * fun, t_bbl ** bbl)
{
  *bbl = BBL_NEXT_MARKED_IN_FUN(FUNCTION_MARKED_BBLS(fun));

  if (*bbl == FUNCTION_MARKED_BBLS(fun))
    return FALSE;

  BBL_SET_NEXT_MARKED_IN_FUN(FUNCTION_MARKED_BBLS(fun), BBL_NEXT_MARKED_IN_FUN(*bbl));
  BBL_SET_NEXT_MARKED_IN_FUN(*bbl, NULL);

  return TRUE;
}

void
FunctionUnmarkAllBbls (t_function * fun)
{
  t_bbl *bbl;

  if (!(FUNCTION_MARKED_BBLS(fun)))
  {
    FUNCTION_SET_MARKED_BBLS(fun, (t_bbl *) Malloc (sizeof (t_bbl)));
    BBL_SET_NEXT_MARKED_IN_FUN(FUNCTION_MARKED_BBLS(fun), FUNCTION_MARKED_BBLS(fun));
  }
  else
    while (FunctionUnmarkBbl (fun, &bbl));

  /* make sure there is no garbage in the BBL_NEXT_MARKED_IN_FUN fields */
  FUNCTION_FOREACH_BBL(fun, bbl)
    BBL_SET_NEXT_MARKED_IN_FUN(bbl, NULL);
}

void
FunctionMarkBbl (t_function * fun, t_bbl * bbl)
{
  if (!(BBL_NEXT_MARKED_IN_FUN(bbl)))
  {
    BBL_SET_NEXT_MARKED_IN_FUN(bbl, BBL_NEXT_MARKED_IN_FUN(FUNCTION_MARKED_BBLS(BBL_FUNCTION(bbl))));
    BBL_SET_NEXT_MARKED_IN_FUN(FUNCTION_MARKED_BBLS(BBL_FUNCTION(bbl)), bbl);
  }
}

void
FunctionFreeMarkedSpace (t_function * fun)
{
  if (FUNCTION_MARKED_EDGES(fun))
  {
    Free (FUNCTION_MARKED_EDGES(fun));
    FUNCTION_SET_MARKED_EDGES(fun, NULL);
  }
  if (FUNCTION_MARKED_BBLS(fun))
  {
    Free (FUNCTION_MARKED_BBLS(fun));
    FUNCTION_SET_MARKED_BBLS(fun, NULL);
  }
}


t_bool
FunctionUnmarkEdge (t_function * fun, t_cfg_edge ** edge)
{
  *edge = CFG_EDGE_NEXT_MARKED_INTO_FUN(FUNCTION_MARKED_EDGES(fun));

  if (*edge == FUNCTION_MARKED_EDGES(fun))
    return FALSE;

  CFG_EDGE_SET_NEXT_MARKED_INTO_FUN(FUNCTION_MARKED_EDGES(fun), CFG_EDGE_NEXT_MARKED_INTO_FUN(*edge));
  CFG_EDGE_SET_NEXT_MARKED_INTO_FUN(*edge, NULL);

  return TRUE;
}

void
FunctionUnmarkAllEdges (t_function * fun)
{
  t_cfg_edge *edge;

  if (!(FUNCTION_MARKED_EDGES(fun)))
  {
    FUNCTION_SET_MARKED_EDGES(fun, (t_cfg_edge *) Malloc (sizeof (t_cfg_edge)));
    CFG_EDGE_SET_NEXT_MARKED_INTO_FUN(FUNCTION_MARKED_EDGES(fun), FUNCTION_MARKED_EDGES(fun));
  }
  else
    while (FunctionUnmarkEdge (fun, &edge));
}

void
FunctionMarkEdge (t_function * fun, t_cfg_edge * edge)
{
  if (!(CFG_EDGE_NEXT_MARKED_INTO_FUN(edge)))
  {
    CFG_EDGE_SET_NEXT_MARKED_INTO_FUN(edge, CFG_EDGE_NEXT_MARKED_INTO_FUN(FUNCTION_MARKED_EDGES(fun)));
    CFG_EDGE_SET_NEXT_MARKED_INTO_FUN(FUNCTION_MARKED_EDGES(fun), edge);
  }
}

void
FunctionMarkAllTrueIncomingEdges (t_function * fun)
{
  t_bbl *bbl;
  t_cfg_edge *edge;

  if ((FUNCTION_BBL_FIRST(fun)) && (BBL_IS_HELL(FUNCTION_BBL_FIRST(fun))))
  {
    return;
  }

  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    BBL_FOREACH_PRED_EDGE(bbl, edge)
    {
      if (CfgEdgeIsForwardInterproc (edge))
      {
        if (CfgEdgeIsMarked (edge))
        {
          FunctionMarkEdge (fun, edge);
        }
      }
    }
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
