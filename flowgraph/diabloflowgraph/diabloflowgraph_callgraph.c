#include <string.h>
#include <diabloflowgraph.h>
#include <time.h>

t_bool
ExistsEdgeBetween (t_function * head, t_function * tail, t_uint32 cat)
{
  t_cg_edge *edge;


  FUNCTION_FOREACH_SUCC_EDGE(head, edge)
    if (CG_EDGE_TAIL(edge) == tail && CG_EDGE_CAT(edge) == cat)
      return TRUE;

  return FALSE;

}

t_cg_edge *
CgAppendNode (t_cg * graph, t_function * after, t_function * insert, t_uint32 edge_category)
{
  return (t_cg_edge *) GraphAppendNode(T_GRAPH(graph),T_NODE(after),T_NODE(insert), edge_category);
}


t_cg *
CgNew ()
{
  t_cg *ret = Malloc (sizeof (t_cg));

  GraphInit ((t_graph *) ret, sizeof (t_function), sizeof (t_cg_edge));
  CG_SET_CFG(ret, NULL);
  return ret;
}

void
CgFree (t_cg * cg)
{
  CFG_SET_CG(CG_CFG(cg), NULL);

  while (CG_NEDGES(cg) > 0)
    CgRemoveEdge (cg, CG_EDGE_FIRST(cg));

  /* don't kill the graph nodes, they are the functions of the program!
   * instead, just clean the node structures inside the functions */
  while (CG_NODE_FIRST(cg))
    CgUnlinkNode (cg, CG_NODE_FIRST(cg));

  Free (cg);
}

static void
CgInsertNode (t_cg * cg, t_function * fun)
{
  CgInitNode (cg, fun);
}

void
CgAddCfgEdge (t_cg * cg, t_cfg_edge * edge)
{
  t_function *head;
  t_function *tail;

  head = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
  tail = BBL_FUNCTION(CFG_EDGE_TAIL(edge));

  if (!head || !tail)
    FATAL(("trying to add edge that is not in a function"));

  if (!ExistsEdgeBetween (head, tail, CFG_EDGE_CAT(edge)))
  {
    t_cg_edge *cg_edge = CgAppendNode (cg, head, tail, CFG_EDGE_CAT(edge));

    CG_EDGE_SET_CFG_EDGE(cg_edge, edge);
  }
}

static void
CgAddCfgEdgeBlindly (t_cg * cg, t_cfg_edge * edge)
{
  t_function *head;
  t_function *tail;

  head = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
  tail = BBL_FUNCTION(CFG_EDGE_TAIL(edge));

  if (!head || !tail)
    FATAL(("trying to add edge that is not in a function"));

  {
    t_cg_edge *cg_edge = CgAppendNode (cg, head, tail, CFG_EDGE_CAT(edge));

    CG_EDGE_SET_CFG_EDGE(cg_edge, edge);
  }
}

void
CgBuild (t_cfg * cfg)
{
  t_cfg_edge *edge;
  t_function *fun;

  if (CFG_CG(cfg))
    CgFree (CFG_CG(cfg));

  CFG_SET_CG(cfg, CgNew ());
  CG_SET_CFG(CFG_CG(cfg), cfg);

  /* insert all functions */
  CFG_FOREACH_FUN(cfg, fun)
    CgInsertNode (CFG_CG(cfg), fun);

  CFG_FOREACH_FUN(cfg, fun)
  {
    t_bbl *init_bbl = FUNCTION_BBL_FIRST(fun);

    NodeMarkInit ();

    BBL_FOREACH_PRED_EDGE(init_bbl, edge)
    {
      if (CfgEdgeIsForwardInterproc (edge) && !BblIsMarked (FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_HEAD(edge)))))
      {
        BblMark (FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_HEAD(edge))));
        CgAddCfgEdgeBlindly (CFG_CG(cfg), edge);
      }
    }
  }
}

void
CgExport (t_cg * cg, const char *filename)
{
  t_cg_edge *edge;
  t_function *fun, *callee;
  FILE *out = fopen (filename, "w");

  ASSERT(out, ("Could not open %s for writing!", filename));
  FileIo (out, "digraph \"callgraph\" {\n\tnode [shape=box]\n");

  CG_FOREACH_FUN(cg, fun)
  {
    FileIo (out, "\t\"%p\" [label=\"", fun);
    FileIo (out, "@G\\n", BBL_CADDRESS(FUNCTION_BBL_FIRST(fun)));
    fprintf (out, "%s", FUNCTION_NAME(fun)?FUNCTION_NAME(fun):"noname");
    FileIo (out, "\"]\n");

    FUNCTION_FOREACH_CALLEE(fun, edge, callee)
      fprintf (out, "\t\"%p\" -> \"%p\" [style=%s];\n", fun, callee, CG_EDGE_CAT(edge) == ET_CALL ? "solid" : "dashed");

  }

  FileIo (out, "}\n");
  fclose (out);
}

typedef struct _t_function_list
{
  struct _t_function_list *next;
  t_function *cg_node;
} t_function_list;

void
MarkCallerChain (t_cfg * cfg, t_function * fun)
{
  t_function *ifun;
  t_function *node;
  t_cg_edge *edge;
  t_function_list *new;
  t_function_list *todo = Calloc (1, sizeof (t_function_list));

  todo->cg_node = fun;

  CFG_FOREACH_FUN(cfg, ifun)
    FUNCTION_SET_FLAGS(ifun, FUNCTION_FLAGS(ifun) & (~FF_IS_MARKED));

  while (todo)
  {
    t_function_list *next;

    node = todo->cg_node;
    /* remove the current one from the list */
    next = todo->next;
    Free (todo);
    todo = next;

    FUNCTION_SET_FLAGS(T_FUN(node), FUNCTION_FLAGS(T_FUN(node)) | FF_IS_MARKED);

    FUNCTION_FOREACH_PRED_EDGE(node, edge)
    {
      t_function *head = CG_EDGE_HEAD(edge);

      ifun = head;
      if (FUNCTION_CALL_HELL_TYPE(ifun))
        continue;
      if (FUNCTION_FLAGS(ifun) & FF_IS_MARKED)
        continue;
      /* add to the list */
      new = Calloc (1, sizeof (t_function_list));
      new->cg_node = head;
      new->next = todo;
      todo = new;
    }
  }
}

t_bool
FunctionIsReentrant (t_function * fun)
{
  t_cfg *cfg = FUNCTION_CFG(fun);
  t_function *ifun;
  t_cg *cg = CFG_CG(cfg);
  t_function *itself, *node;
  t_cg_edge *edge;
  t_bool ret = FALSE;

  t_function_list *todo, *new;

  if (!cg)
    FATAL(("No callgraph available!"));
  itself = fun;

  todo = (t_function_list *) Calloc (1, sizeof (t_function_list));
  todo->cg_node = itself;

  CFG_FOREACH_FUN(cfg, ifun)
    FUNCTION_SET_FLAGS(ifun, FUNCTION_FLAGS(ifun) & (~FF_IS_MARKED2));

  while (todo && !ret)
  {
    t_function_list *next;

    node = todo->cg_node;

    next = todo->next;
    Free (todo);
    todo = next;

    FUNCTION_SET_FLAGS(T_FUN(node), FUNCTION_FLAGS(T_FUN(node)) | FF_IS_MARKED2);

    FUNCTION_FOREACH_PRED_EDGE(node, edge)
    {
      t_function *head = CG_EDGE_HEAD(edge);

      if (head == itself)
      {
        ret = TRUE;
        break;
      }

      ifun = T_FUN(head);
      if (FUNCTION_CALL_HELL_TYPE(ifun))
      {
        ret = TRUE;
        break;
      }
      if (FUNCTION_FLAGS(ifun) & FF_IS_MARKED2)
        continue;
      new = (t_function_list *) Calloc (1, sizeof (t_function_list));
      new->cg_node = head;
      if (todo)
        new->next = todo;
      todo = new;
    }
  }

  while (todo)
  {
    t_function_list *next;

    next = todo->next;
    Free (todo);
    todo = next;
  }

  return ret;
}

t_int32
MarkAndStartWith (t_function * fun)
{
  t_int32 nr = 0;
  t_cg_edge *edge;

  FUNCTION_FOREACH_SUCC_EDGE(fun, edge)
  {
    if (!(FUNCTION_FLAGS(((t_function *) CG_EDGE_TAIL(edge))) & FF_IS_MARKED2))
    {
      FUNCTION_SET_FLAGS(((t_function *) CG_EDGE_TAIL(edge)), FUNCTION_FLAGS(((t_function *) CG_EDGE_TAIL(edge))) | FF_IS_MARKED2);
      // printf("Function:%s\n",((t_function*)CG_EDGE_TAIL(edge))->name);
      nr += (1 + MarkAndStartWith (CG_EDGE_TAIL(edge)));
    }
  }
  return nr;
}

t_int32
IndicateAllReachableFunctions (t_function * fun)
{
  t_cfg *cfg = FUNCTION_CFG(fun);
  t_cg *cg = CFG_CG(cfg);

  if (!cg)
  {
    printf ("No callgraph available! Make new one\n");
    CgBuild (cfg);
    cg = CFG_CG(cfg);
  }

  CFG_FOREACH_FUN(cfg, fun)
    FUNCTION_SET_FLAGS(fun, FUNCTION_FLAGS(fun) & (~FF_IS_MARKED2));

  return MarkAndStartWith (fun);
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
