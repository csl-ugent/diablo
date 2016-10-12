/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>
/* Query functions {{{ */

void
CfgEdgeMoveToLastSucc (t_cfg_edge * fall)
{

  while (CFG_EDGE_SUCC_NEXT(fall))
  {
    t_cfg_edge *nextnext = NULL;
    t_cfg_edge *prev = CFG_EDGE_SUCC_PREV(fall);
    t_cfg_edge *next = CFG_EDGE_SUCC_NEXT(fall);


    if (next)
      nextnext = CFG_EDGE_SUCC_NEXT(next);

    CFG_EDGE_SET_SUCC_PREV(next, prev);
    if (prev)
      CFG_EDGE_SET_SUCC_NEXT(prev, next);
    else
      BBL_SET_SUCC_FIRST(CFG_EDGE_HEAD(fall), next);

    CFG_EDGE_SET_SUCC_NEXT(next, fall);
    CFG_EDGE_SET_SUCC_PREV(fall, next);

    if (nextnext)
      CFG_EDGE_SET_SUCC_PREV(nextnext, fall);
    else
      BBL_SET_SUCC_LAST(CFG_EDGE_HEAD(fall), fall);


    CFG_EDGE_SET_SUCC_NEXT(fall, nextnext);
  }
}

void
CfgEdgeMoveFirstToLastPred (t_cfg_edge * first_edge)
{
  t_bbl *tail = CFG_EDGE_TAIL(first_edge);

  BBL_SET_PRED_FIRST(tail, CFG_EDGE_PRED_NEXT(first_edge));
  CFG_EDGE_SET_PRED_PREV(BBL_PRED_FIRST(tail), NULL);

  CFG_EDGE_SET_PRED_NEXT(BBL_PRED_LAST(tail), first_edge);
  CFG_EDGE_SET_PRED_PREV(first_edge, BBL_PRED_LAST(tail));
  CFG_EDGE_SET_PRED_NEXT(first_edge, NULL);

  BBL_SET_PRED_LAST(tail, first_edge);
}

/*}}}*/
/* Constructors {{{ */
t_cfg_edge *
CfgEdgeCreate (t_cfg * cfg, t_bbl * from, t_bbl * to, t_uint32 type)
{
  t_cfg_edge *ret;

  if (type == ET_RETURN && !BBL_IS_HELL (to))
    FATAL(("Cannot use CfgEdgeCreate to add return edges! (Use CfgEdgeCreateCall)"));
  if (type == ET_COMPENSATING && !BBL_IS_HELL (to))
    FATAL(("Cannot use CfgEdgeCreate to add compensating edges! (Use CfgEdgeCreateCompensating)"));
  if (type == ET_CALL && !BBL_IS_HELL (to))
  {
    FATAL(("Cannot use CfgEdgeCreate to add call edges!"));
  }

  ret = CfgEdgeNew (cfg, from, to, type);
  dominator_info_correct = FALSE;

  return ret;
}

t_cfg_edge *
CfgEdgeCreateCompensating (t_cfg * cfg, t_cfg_edge * ip)
{
  t_cfg_edge *ret;
  t_bbl *bbl = FunctionGetExitBlock (BBL_FUNCTION(CFG_EDGE_TAIL(ip)));
  t_bbl *bbl2 = FunctionGetExitBlock (BBL_FUNCTION(CFG_EDGE_HEAD(ip)));


  ASSERT(bbl, ("No exit block when creating new compensating for @E", ip));
  ASSERT(bbl2, ("No exit block2 when creating new compensating for @E", ip));
  ret = CfgEdgeNew (cfg, bbl, bbl2, ET_COMPENSATING);
  CFG_EDGE_SET_CORR(ret, ip);
  CFG_EDGE_SET_CORR(ip, ret);
  CFG_EDGE_SET_REFCOUNT(ret, CFG_EDGE_REFCOUNT(ip));
  dominator_info_correct = FALSE;
  return ret;
}

t_cfg_edge *
CfgEdgeCreateCall (t_cfg * cfg, t_bbl * from, t_bbl * to, t_bbl * ret, t_bbl * exit_bbl)
{
  t_cfg_edge *call = CfgEdgeNew (cfg, from, to, ET_CALL);
  t_cfg_edge *retu = CfgEdgeNew (cfg, (!exit_bbl) ? CFG_EXIT_HELL_NODE(cfg) : exit_bbl, (!ret) ? CFG_EXIT_HELL_NODE(cfg) : ret, ET_RETURN);

  CFG_EDGE_SET_CORR(call, retu);
  CFG_EDGE_SET_CORR(retu, call);
  dominator_info_correct = FALSE;

  if (!cfg)
    FATAL(("No flowgraph"));
  return call;
}

t_cfg_edge *
CfgEdgeCreateSwi (t_cfg * cfg, t_bbl * from, t_bbl * ret)
{
  t_cfg_edge *call = CfgEdgeNew (cfg, from, CFG_SWI_HELL_NODE(cfg), ET_SWI);
  t_cfg_edge *retu = CfgEdgeNew (cfg, CFG_EXIT_SWI_HELL_NODE(cfg), (!ret) ? CFG_EXIT_HELL_NODE(cfg) : ret, ET_RETURN);

  CFG_EDGE_SET_CORR(call, retu);
  CFG_EDGE_SET_CORR(retu, call);
  dominator_info_correct = FALSE;
  if (!cfg)
    FATAL(("No flowgraph"));
  return call;
}

/* }}} */
/* Destructor {{{ */
void
CfgEdgeRelKill (t_cfg_edge * edge, t_bool killrel)
{
  dominator_info_correct = FALSE;
  CFG_EDGE_SET_REFCOUNT(edge, CFG_EDGE_REFCOUNT(edge) - 1);
  if (CFG_EDGE_REFCOUNT(edge) == 0)
  {
    if (CFG_EDGE_REL(edge))
    {
      ASSERT(CFG_EDGE_CAT(edge) == ET_SWITCH
             || CFG_EDGE_CAT(edge) == ET_IPSWITCH,
             ("we only expect corresponding relocs with switch edges, not for @E",
              edge));
      if (killrel)
        RelocTableRemoveReloc (OBJECT_RELOC_TABLE(CFG_OBJECT(CFG_EDGE_CFG(edge))), CFG_EDGE_REL(edge));
    }
    if(CFG_EDGE_CORR(edge))
    {
      CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge),NULL);
    }

    DiabloBrokerCall("CfgEdgeKill", edge);
    CfgEdgeFree (edge);
  }
}

void
CfgEdgeKill (t_cfg_edge * edge)
{
  CfgEdgeRelKill(edge,TRUE);
}

void
CfgEdgeKillKeepRel (t_cfg_edge * edge)
{
  CfgEdgeRelKill(edge,FALSE);
}

/* }}} */

t_bool
FtPath (t_bbl * i_bbl, t_bbl * j_bbl, t_cfg_edge * ignore_edge)
{
  t_bbl *it_bbl = i_bbl;
  t_cfg_edge *i_edge;

  while (1)
  {
    BBL_FOREACH_SUCC_EDGE(it_bbl, i_edge)
    {
      if(i_edge == ignore_edge) continue;
      if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH || CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU
          || ((CFG_EDGE_CAT(i_edge) == ET_CALL || CFG_EDGE_CAT(i_edge) == ET_SWI) && CFG_EDGE_CORR(i_edge)))
      {
        break;
      }
    }
    if (!i_edge)
      return FALSE;
    if (CFG_EDGE_CAT(i_edge) == ET_CALL || CFG_EDGE_CAT(i_edge) == ET_SWI)
      i_edge = CFG_EDGE_CORR(i_edge);
    if (CFG_EDGE_TAIL(i_edge) == j_bbl)
      return TRUE;
    it_bbl = CFG_EDGE_TAIL(i_edge);
  }
}

t_cfg_edge *
TakenPath (t_bbl * bbl)
{
  t_cfg_edge *edge;

  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    if (CFG_EDGE_CAT(edge) == ET_JUMP || CFG_EDGE_CAT(edge) == ET_IPJUMP || CFG_EDGE_CAT(edge) == ET_CALL || CFG_EDGE_CAT(edge) == ET_SWI)
      return edge;
  }
  return NULL;
}

t_cfg_edge *
FallThroughPath (t_bbl * bbl)
{
  t_cfg_edge *edge;


  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      return edge;
  }
  return NULL;
}

t_cfg_edge *
CfgEdgeChangeReturnForCall (t_cfg_edge * call, t_bbl * ret)
{
  if (!BBL_FUNCTION(CFG_EDGE_TAIL(call)))
  {
    FATAL(("Not implemented for non function calls"));
  }
  if (CFG_EDGE_CORR(call))
  {
    CfgEdgeKill (CFG_EDGE_CORR(call));
    CFG_EDGE_SET_CORR(call, NULL);
  }
  CFG_EDGE_SET_CORR(call, CfgEdgeNew (CFG_EDGE_CFG(call), FUNCTION_BBL_LAST(BBL_FUNCTION(CFG_EDGE_TAIL(call))), ret, ET_RETURN));
  CFG_EDGE_SET_CORR(CFG_EDGE_CORR(call), call);
  CFG_EDGE_SET_CFG(CFG_EDGE_CORR(call), CFG_EDGE_CFG(call));
  CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(call), CFG_EDGE_REFCOUNT(call));
  return NULL;
}

t_bool
CfgEdgeKillAndUpdateBbl (t_cfg_edge * edge)
{
  t_bbl *head, *tail, *corr_head = NULL, *corr_tail = NULL;
  t_cfg *cfg;
  t_object *obj;
  t_ins *i_ins;
  t_uint32 delay_slot_count = 0;

  if (!edge)
    return FALSE;

  head = CFG_EDGE_HEAD(edge);
  tail = CFG_EDGE_TAIL(edge);
  cfg = CFG_EDGE_CFG(edge);
  obj = CFG_OBJECT(cfg);

  if (BBL_IS_HELL (head) || BBL_IS_HELL (tail))
    return FALSE;

  switch (CFG_EDGE_CAT(edge))
  {
    case ET_JUMP:
      {
        /* Let's find the jump instruction and kill it */
        BBL_FOREACH_INS_R(head, i_ins)
        {
          if (delay_slot_count > 1)
            FATAL(("There is no supported architecture with more than one instruction in the delay slot"));
          if (CFG_DESCRIPTION(cfg)->InsIsControlflow (i_ins))
            break;
          delay_slot_count++;
        }
        if (i_ins)
          InsKill (i_ins);
        CfgEdgeKill (edge);
      }
      return TRUE;
    case ET_FALLTHROUGH:
      {
        /* Let's find the conditional jump instruction and make it unconditional.
         * If none is found, nothing has to be done */
        BBL_FOREACH_INS_R(head, i_ins)
        {
          if (CFG_DESCRIPTION(cfg)->InsIsControlflow (i_ins))
            break;
        }
        if (i_ins && INS_IS_CONDITIONAL(i_ins))
          CFG_DESCRIPTION(cfg)->InstructionUnconditionalizer (i_ins);
        CfgEdgeKill (edge);
      }
      return TRUE;
    case ET_CALL:
    case ET_IPJUMP:
      {
        t_bool make_fallthrough = TRUE;

        /* Let's find the call instruction and kill it */
        BBL_FOREACH_INS_R(head, i_ins)
        {
          if (delay_slot_count > 1)
            FATAL(("There is no supported architecture with more than one instruction in the delay slot"));
          if (CFG_DESCRIPTION(cfg)->InsIsControlflow (i_ins))
            break;
          delay_slot_count++;
        }
        if (i_ins && INS_IS_CONDITIONAL(i_ins))
          make_fallthrough = FALSE;
        if (i_ins)
          InsKill (i_ins);

        if (CFG_EDGE_CORR(edge))
        {
          corr_head = CFG_EDGE_HEAD(CFG_EDGE_CORR(edge));
          corr_tail = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
          CfgEdgeKill (CFG_EDGE_CORR(edge));
        }
        CfgEdgeKill (edge);

        if (make_fallthrough == TRUE && CFG_EDGE_CAT(edge) == ET_CALL)
          CfgEdgeCreate (cfg, head, corr_tail, ET_FALLTHROUGH);
      }
      return TRUE;
    case ET_RETURN:
    case ET_COMPENSATING:
      {
        if (CFG_EDGE_CORR(edge))
          return CfgEdgeKillAndUpdateBbl (CFG_EDGE_CORR(edge));
        else
          FATAL(("Return needs to have a corresponding edge!"));
      }
      return FALSE;
    default:
      ; /* keep the compiler happy */
  }
  return FALSE;
}

t_cfg_edge *
CfgEdgeCreateChecked (t_bbl * from, t_bbl * to, t_uint32 type, t_bbl * callee, t_bbl * exit_bbl, t_cfg_edge* ignore_edge)
{
  t_cfg_edge *i_edge;
  t_cfg * cfg = BBL_CFG(from);

  /* t_cfg_edge * ret = NULL;*/
  if (!from || !to)
    return NULL;

  switch (type)
  {
    case ET_FALLTHROUGH:
      {
        BBL_FOREACH_SUCC_EDGE(from, i_edge)
        {
          if(i_edge != ignore_edge && CFG_EDGE_CAT(i_edge) & (ET_FALLTHROUGH | ET_COMPENSATING | ET_RETURN))
            return NULL;
        }
        BBL_FOREACH_PRED_EDGE(to, i_edge)
        {
          if(i_edge == ignore_edge) continue;
          if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH)
            return NULL;
          if (CFG_EDGE_CAT(i_edge) == ET_RETURN && CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge)) != from)
            return NULL;
        }
        if (FtPath (to, from, ignore_edge) == TRUE)
          return NULL;

        return CfgEdgeCreate (cfg, from, to, type);
      }
      break;
    case ET_JUMP:
      {
        BBL_FOREACH_SUCC_EDGE(from, i_edge)
        {
          if (i_edge != ignore_edge && CFG_EDGE_CAT(i_edge) & (ET_JUMP | ET_CALL | ET_IPJUMP | ET_SWI | ET_RETURN | ET_COMPENSATING))
            return NULL;
        }

        return CfgEdgeCreate (cfg, from, to, type);
      }
      break;
    case ET_CALL:
      {
        BBL_FOREACH_SUCC_EDGE(from, i_edge)
        {
          if(i_edge == ignore_edge) continue;
          if (CFG_EDGE_CAT(i_edge) & (ET_JUMP | ET_CALL | ET_IPJUMP | ET_SWI | ET_RETURN | ET_COMPENSATING))
            return NULL;
          if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH && CFG_EDGE_TAIL(i_edge) != to)
            return NULL;
        }
        BBL_FOREACH_PRED_EDGE(to, i_edge)
        {
          if(i_edge == ignore_edge) continue;
          if (CFG_EDGE_CAT(i_edge) == ET_RETURN)
            return NULL;
          if (CFG_EDGE_CAT(i_edge) == ET_FALLTHROUGH && CFG_EDGE_HEAD(i_edge) != from)
            return NULL;
        }
        if (FtPath (to, from, ignore_edge) == TRUE)
          return NULL;

        return CfgEdgeCreateCall (cfg, from, callee, to, exit_bbl);
      }
    default:
      ;
  }

  return NULL;
}

void CfgEdgeChangeTailIntern(t_cfg_edge * edge, t_bbl *new_head, t_bool
                             updateCorr);

void CfgEdgeChangeHeadIntern(t_cfg_edge * edge, t_bbl *new_head, t_bool
                             updateCorr)
{
  t_bbl *old_head;

  old_head = CFG_EDGE_HEAD(edge);

  /* When we are dealing with a compensating or return edge,
     its head should always be the exit block of the function */
  if (CfgEdgeIsBackwardInterproc(edge))
  {
    t_function* fun = BBL_FUNCTION(new_head);
    ASSERT(fun, ("BBL not in function @eiB", new_head));

    new_head = FunctionGetExitBlock(fun);
    ASSERT(new_head,("exit block expected for @F", fun));
  }

  /* move the edge */
  CFG_EDGE_SET_HEAD(edge, new_head);
  /* remove from successor list of head */
  if (CFG_EDGE_SUCC_NEXT(edge))
    CFG_EDGE_SET_SUCC_PREV(CFG_EDGE_SUCC_NEXT(edge), CFG_EDGE_SUCC_PREV(edge));
  else
    BBL_SET_SUCC_LAST(old_head, CFG_EDGE_SUCC_PREV(edge));
  if (CFG_EDGE_SUCC_PREV(edge))
    CFG_EDGE_SET_SUCC_NEXT(CFG_EDGE_SUCC_PREV(edge), CFG_EDGE_SUCC_NEXT(edge));
  else
    BBL_SET_SUCC_FIRST(old_head, CFG_EDGE_SUCC_NEXT(edge));
  /* add to successor list of new */
  CFG_EDGE_SET_SUCC_NEXT(edge, BBL_SUCC_FIRST(new_head));
  CFG_EDGE_SET_SUCC_PREV(edge, NULL);
  if (CFG_EDGE_SUCC_NEXT(edge))
    CFG_EDGE_SET_SUCC_PREV(CFG_EDGE_SUCC_NEXT(edge), edge);
  BBL_SET_SUCC_FIRST(new_head, edge);
  if (!BBL_SUCC_LAST(new_head))
   BBL_SET_SUCC_LAST(new_head, edge);
  /* update tail of corresponding edge as well */
  if (updateCorr &&
      CFG_EDGE_CORR(edge))
    CfgEdgeChangeTailIntern(CFG_EDGE_CORR(edge),new_head,FALSE);
}

void CfgEdgeChangeHead(t_cfg_edge * edge, t_bbl *new_head)
{
  t_bool updateCorr = TRUE;

  /* When changing the head of a forward interprocedural edge (e.g. a call) we should't change
   * the tail of the corresponding backward edge (e.g. a return).
   */
  if(CfgEdgeIsForwardInterproc(edge))
    updateCorr = FALSE;

  CfgEdgeChangeHeadIntern(edge,new_head,updateCorr);
}

void CfgEdgeChangeTailIntern(t_cfg_edge * edge, t_bbl *new_tail, t_bool
                             updateCorr)
{
  t_bbl *old_tail;

  old_tail = CFG_EDGE_TAIL(edge);

  /* move the edge */
  CFG_EDGE_SET_TAIL(edge, new_tail);
  /* remove from predecessor list of tail */
  if (CFG_EDGE_PRED_NEXT(edge))
    CFG_EDGE_SET_PRED_PREV(CFG_EDGE_PRED_NEXT(edge), CFG_EDGE_PRED_PREV(edge));
  else
    BBL_SET_PRED_LAST(old_tail, CFG_EDGE_PRED_PREV(edge));
  if (CFG_EDGE_PRED_PREV(edge))
    CFG_EDGE_SET_PRED_NEXT(CFG_EDGE_PRED_PREV(edge), CFG_EDGE_PRED_NEXT(edge));
  else
    BBL_SET_PRED_FIRST(old_tail, CFG_EDGE_PRED_NEXT(edge));
  /* add to predecessor list of new */
  CFG_EDGE_SET_PRED_NEXT(edge, BBL_PRED_FIRST(new_tail));
  CFG_EDGE_SET_PRED_PREV(edge, NULL);
  if (CFG_EDGE_PRED_NEXT(edge))
    CFG_EDGE_SET_PRED_PREV(CFG_EDGE_PRED_NEXT(edge), edge);
  BBL_SET_PRED_FIRST(new_tail, edge);
  if (!BBL_PRED_LAST(new_tail))
    BBL_SET_PRED_LAST(new_tail, edge);
  /* update head of corresponding edge as well */
  if (updateCorr &&
      CFG_EDGE_CORR(edge))
    CfgEdgeChangeHeadIntern(CFG_EDGE_CORR(edge),new_tail,FALSE);
}

void CfgEdgeChangeTail(t_cfg_edge * edge, t_bbl *new_tail)
{
  t_bool updateCorr = TRUE;

  if(CFG_EDGE_CAT(edge) == ET_COMPENSATING)
    FATAL(("Attempt to change the tail of a compensating edge in function CfgEdgeChangeTail()"));

  /* When changing the tail of a backward interprocedural edge (e.g. a return) we should't change
   * the head of the corresponding forward edge (e.g. a call).
   */
  if(CfgEdgeIsBackwardInterproc(edge))
    updateCorr = FALSE;

  CfgEdgeChangeTailIntern(edge,new_tail,updateCorr);
}

t_string CfgEdgeTypeToString(t_cfg_edge * edge)
{
  switch (CFG_EDGE_CAT(edge))
  {
    case ET_UNKNOWN: return "unknown";
    case ET_FALLTHROUGH: return "fallthrough";
    case ET_CALL: return "call";
    case ET_RETURN: return "return";
    case ET_JUMP: return "jump";
    case ET_SWI: return "syscall";
    case ET_IPUNKNOWN: return "unknown interproc";
    case ET_IPFALLTHRU: return "fallthrough interproc";
    case ET_IPJUMP: return "jump interproc";
    case ET_COMPENSATING: return "compensating";
    case ET_SWITCH: return "switch";
    case ET_IPSWITCH: return "switch interproc";
    case ET_POSTDOM: return "postdominator";
    default: return "invalid";
  }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
