/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#include <stdio.h>
#include <string.h> /* For memmove */

/* TODO */
#define CONFIG_GT_STACK_ALLOC_SIZE 10

t_uint32 global_node_marking_number = 1;
t_uint32 global_node_marking_number2 = 1;
t_uint32 global_edge_marking_number = 1;

void
NodeMarkInit (void)
{
  if (++global_node_marking_number == 0x7fffffff)
  {
    FATAL(("Marking number overflow\n"));
  }
}
void
NodeMarkInit2 (void)
{
  if (++global_node_marking_number2 == 0x7fffffff)
  {
    FATAL(("Marking number overflow\n"));
  }
}

void
EdgeMarkInit (void)
{
  if (++global_edge_marking_number == 0x7fffffff)
  {
    FATAL(("Edge marking number overflow\n"));
  }
}

void
GraphInit (t_graph * temp, t_uint32 node_size, t_uint32 edge_size)
{
  GRAPH_SET_NODE_SIZE(temp, node_size);
  GRAPH_SET_EDGE_SIZE(temp, edge_size);
  GRAPH_SET_NODE_FIRST(temp, NULL);
  GRAPH_SET_NODE_LAST(temp, NULL);
  GRAPH_SET_EDGE_FIRST(temp, NULL);
  GRAPH_SET_EDGE_LAST(temp, NULL);
  GRAPH_SET_NNODES(temp, 0);
  GRAPH_SET_NEDGES(temp, 0);
}

t_graph *
GraphNew (t_uint32 node_size, t_uint32 edge_size)
{
  t_graph *temp = Malloc (sizeof (t_graph));

  GraphInit (temp, node_size, edge_size);
  return temp;
}

void
GraphInitNode (t_graph * graph, t_node * temp)
{
  GraphInsertNodeInGraph (graph, temp);

  NODE_SET_MARKED_NUMBER(temp, 0);
  NODE_SET_MARKED_NUMBER2(temp, 0);
  NODE_SET_PRED_FIRST(temp, NULL);
  NODE_SET_PRED_LAST(temp, NULL);
  NODE_SET_SUCC_FIRST(temp, NULL);
  NODE_SET_SUCC_LAST(temp, NULL);
}

void
GraphInsertNodeInGraph (t_graph * graph, t_node * node)
{
  NODE_SET_NEXT(node, NULL);
  if (GRAPH_NODE_LAST(graph))
  {
    NODE_SET_PREV(node, GRAPH_NODE_LAST(graph));
    NODE_SET_NEXT(GRAPH_NODE_LAST(graph), node);
    GRAPH_SET_NODE_LAST(graph, node);
  }
  else
  {
    NODE_SET_PREV(node, NULL);
    GRAPH_SET_NODE_FIRST(graph, node);
    GRAPH_SET_NODE_LAST(graph, node);
  }

  GRAPH_SET_NNODES(graph, GRAPH_NNODES(graph) + 1);
}

t_node *
GraphNewNode (t_graph * graph, t_uint32 node_category)
{
  t_node *temp;

  ASSERT(GRAPH_NODE_SIZE(graph),
         ("Can't use GraphNodeNew when node_size == 0"));
  temp = Calloc (1,GRAPH_NODE_SIZE(graph));

  /* NodeSetCategory(temp,node_category); */
  GraphInitNode (graph, temp);
  return temp;
}

void
GraphInitEdge (t_graph * graph, t_edge * temp, t_node * head, t_node * tail, t_uint32 edge_category)
{
  EDGE_SET_HEAD(temp, head);
  EDGE_SET_TAIL(temp, tail);
  EDGE_SET_CAT(temp, edge_category);
  EDGE_SET_NEXT(temp, NULL);

  if (NODE_SUCC_LAST(head))
  {
    EDGE_SET_SUCC_NEXT(temp, NULL);
    EDGE_SET_SUCC_PREV(temp, NODE_SUCC_LAST(head));
    EDGE_SET_SUCC_NEXT(NODE_SUCC_LAST(head), temp);
    NODE_SET_SUCC_LAST(head, temp);
  }
  else
  {
    EDGE_SET_SUCC_NEXT(temp, NULL);
    EDGE_SET_SUCC_PREV(temp, NULL);
    NODE_SET_SUCC_LAST(head, temp);
    NODE_SET_SUCC_FIRST(head, temp);
  }

  if (NODE_PRED_LAST(tail))
  {
    EDGE_SET_PRED_NEXT(temp, NULL);
    EDGE_SET_PRED_PREV(temp, NODE_PRED_LAST(tail));
    EDGE_SET_PRED_NEXT(NODE_PRED_LAST(tail), temp);
    NODE_SET_PRED_LAST(tail, temp);
  }
  else
  {
    EDGE_SET_PRED_NEXT(temp, NULL);
    EDGE_SET_PRED_PREV(temp, NULL);
    NODE_SET_PRED_LAST(tail, temp);
    NODE_SET_PRED_FIRST(tail, temp);
  }

  GraphInsertEdgeInGraph (graph, temp);
}

void
GraphInsertEdgeInGraph (t_graph * graph, t_edge * edge)
{
  EDGE_SET_NEXT(edge, NULL);
  if (GRAPH_EDGE_LAST(graph))
  {
    EDGE_SET_NEXT(GRAPH_EDGE_LAST(graph), edge);
    EDGE_SET_PREV(edge, GRAPH_EDGE_LAST(graph));
    GRAPH_SET_EDGE_LAST(graph, edge);
  }
  else
  {
    GRAPH_SET_EDGE_FIRST(graph, edge);
    GRAPH_SET_EDGE_LAST(graph, edge);
    EDGE_SET_PREV(edge, NULL);
  }
  GRAPH_SET_NEDGES(graph, GRAPH_NEDGES(graph) + 1);
}

t_edge *
GraphNewEdge (t_graph * graph, t_node * head, t_node * tail, t_uint32 edge_category)
{
  t_edge *temp;

  temp = Calloc (1, GRAPH_EDGE_SIZE(graph));
  GraphInitEdge (graph, temp, head, tail, edge_category);
  return temp;
}

void
GraphUnlinkEdge (t_graph * graph, t_edge * edge)
{
  GraphUnlinkEdgeFromNodes (edge);
  GraphUnlinkEdgeFromGraph (graph, edge);

  EDGE_SET_PREV(edge, NULL);
  EDGE_SET_NEXT(edge, NULL);
}

void
GraphUnlinkEdgeFromGraph (t_graph * graph, const t_edge * edge)
{
  if (!edge)
    FATAL(("Null edge!"));

  /* remove from list of edges */
  if (edge == GRAPH_EDGE_FIRST(graph))
  {
    GRAPH_SET_EDGE_FIRST(graph, EDGE_NEXT(edge));
    if (GRAPH_EDGE_FIRST(graph))
      EDGE_SET_PREV(GRAPH_EDGE_FIRST(graph), NULL);
    else
    {
      GRAPH_SET_EDGE_FIRST(graph, NULL);
      GRAPH_SET_EDGE_LAST(graph, NULL);
    }
  }
  else
  {
    if (edge == GRAPH_EDGE_LAST(graph))
    {
      GRAPH_SET_EDGE_LAST(graph, EDGE_PREV(edge));
      if (GRAPH_EDGE_LAST(graph))
        EDGE_SET_NEXT(GRAPH_EDGE_LAST(graph), NULL);
    }
    else
    {
      EDGE_SET_PREV(EDGE_NEXT(edge), EDGE_PREV(edge));
      EDGE_SET_NEXT(EDGE_PREV(edge), EDGE_NEXT(edge));
    }
  }

  GRAPH_SET_NEDGES(graph, GRAPH_NEDGES(graph) - 1);
}

void
GraphUnlinkEdgeFromNodes (const t_edge * edge)
{
  if (!edge)
    FATAL(("Null edge!"));

  /* Delete from predecessor list */
  if (EDGE_PRED_PREV(edge))
    EDGE_SET_PRED_NEXT(EDGE_PRED_PREV(edge), EDGE_PRED_NEXT(edge));
  else
  {
    /* removing the first predecessor */
    NODE_SET_PRED_FIRST(EDGE_TAIL(edge), EDGE_PRED_NEXT(edge));
  }

  if (EDGE_PRED_NEXT(edge))
    EDGE_SET_PRED_PREV(EDGE_PRED_NEXT(edge), EDGE_PRED_PREV(edge));
  else
  {
    /* removing the last predecessor */
    NODE_SET_PRED_LAST(EDGE_TAIL(edge), EDGE_PRED_PREV(edge));
  }

  /* Delete from successor list */
  if (EDGE_SUCC_PREV(edge))
    EDGE_SET_SUCC_NEXT(EDGE_SUCC_PREV(edge), EDGE_SUCC_NEXT(edge));
  else
  {
    /* removing the first predecessor */
    NODE_SET_SUCC_FIRST(EDGE_HEAD(edge), EDGE_SUCC_NEXT(edge));
  }

  if (EDGE_SUCC_NEXT(edge))
    EDGE_SET_SUCC_PREV(EDGE_SUCC_NEXT(edge), EDGE_SUCC_PREV(edge));
  else
  {
    /* removing the first predecessor */
    NODE_SET_SUCC_LAST(EDGE_HEAD(edge), EDGE_SUCC_PREV(edge));
  }
}

/* removes an edge from the graph, this means: remove it from the nodes AND from the linked list of edges */
void
GraphRemoveEdge (t_graph * graph, const t_edge * edge)
{
  GraphUnlinkEdgeFromNodes (edge);
  GraphUnlinkEdgeFromGraph (graph, edge);

  /* Free the memory */
  Free (edge);
}

void
GraphRemoveNode (t_graph * graph, const t_node * node)
{
  GraphUnlinkNodeFromGraph (graph, node);

  /* Free the memory */
  Free (node);
}

void
GraphUnlinkNode (t_graph * graph, t_node * node)
{
  GraphUnlinkNodeFromGraph (graph, node);

  /* zero out all node-related fields (necessary in case we're not working
   * with a bare-bones node, but a structure derived from t_node that is
   * allocated and deallocated separately. e.g. t_ins in diablo */
  NODE_SET_NEXT(node, NULL);
  NODE_SET_PREV(node, NULL);
  NODE_SET_SUCC_FIRST(node, NULL);
  NODE_SET_SUCC_LAST(node, NULL);
  NODE_SET_PRED_FIRST(node, NULL);
  NODE_SET_PRED_LAST(node, NULL);
  NODE_SET_MARKED_NUMBER(node, 0);
  NODE_SET_SUCC_FIRST(node, NULL);
  NODE_SET_SUCC_LAST(node, NULL);
}

/* completely remove node from graph, but don't free the node itself */
void
GraphUnlinkNodeFromGraph (t_graph * graph, const t_node * node)
{
  if (NODE_PREV(node))
    NODE_SET_NEXT(NODE_PREV(node), NODE_NEXT(node));

  if (NODE_NEXT(node))
    NODE_SET_PREV(NODE_NEXT(node), NODE_PREV(node));

  /* remove from list of nodes */
  if (node == GRAPH_NODE_FIRST(graph))
  {
    GRAPH_SET_NODE_FIRST(graph, NODE_NEXT(node));
    if (GRAPH_NODE_FIRST(graph))
      NODE_SET_PREV(GRAPH_NODE_FIRST(graph), NULL);
    else
    {
      GRAPH_SET_NODE_FIRST(graph, NULL);
      GRAPH_SET_NODE_LAST(graph, NULL);
    }
  }
  else
  {
    if (node == GRAPH_NODE_LAST(graph))
    {
      GRAPH_SET_NODE_LAST(graph, NODE_PREV(node));
      if (GRAPH_NODE_LAST(graph))
        NODE_SET_NEXT(GRAPH_NODE_LAST(graph), NULL);
    }
    else
    {
      NODE_SET_PREV(NODE_NEXT(node), NODE_PREV(node));
      NODE_SET_NEXT(NODE_PREV(node), NODE_NEXT(node));
    }
  }

  GRAPH_SET_NNODES(graph, GRAPH_NNODES(graph) - 1);
}

void
GraphRemoveAllEdgesFromTo (t_graph * graph, t_node * head, t_node * tail)
{

  t_edge *edge;
  t_edge *next;

  for (edge = NODE_SUCC_FIRST(head); edge != NULL; edge = next)
  {
    next = EDGE_SUCC_NEXT(edge);
    if (EDGE_TAIL(edge) == tail)
    {
      GraphRemoveEdge (graph, edge);
    }
  }
}

t_bool
NodeIsPred (const t_node * succ, const t_node * pred, t_uint32 edge_category) /* Arg2 is predecessor of Arg1 */
{
  t_edge *edge;

  if (NODE_SUCC_FIRST(pred) == NULL)
    return FALSE;
  if (NODE_PRED_FIRST(succ) == NULL)
    return FALSE;

  NODE_FOREACH_SUCC_EDGE(pred, edge)
    if (EDGE_TAIL(edge) == succ && EDGE_CAT(edge) & edge_category)
      return TRUE;

  return FALSE;
}

/* Arg2 is successor of Arg1 */
t_bool
NodeIsSucc (const t_node * pred, const t_node * succ, t_uint32 edge_category)
{
  return (NodeIsPred (succ, pred, edge_category));
}

/* Append arg2 to arg1 */
t_edge *
GraphAppendNode (t_graph * graph, t_node * after, t_node * insert, t_uint32 edge_category)
{
  return GraphNewEdge (graph, after, insert, edge_category);
}

void
GraphDFTraversalWithCheckAndData (t_node * start,
                                  t_bool (*CallBack) (t_node * curr,
                                                      t_node * par,
                                                      void *data),
                                  t_uint32 edge_category_or_mask_to_traverse,
                                  t_uint32
                                  edge_category_or_mask_to_traverse_corresponding,
                                  void *data)
{
  t_node **pstack = Malloc (sizeof (t_node *) * CONFIG_GT_STACK_ALLOC_SIZE);
  t_edge **nstack = Malloc (sizeof (t_edge *) * CONFIG_GT_STACK_ALLOC_SIZE);
  int stack_size = CONFIG_GT_STACK_ALLOC_SIZE;
  int stack_pointer = 0;
  t_edge *current_succ_edge = NODE_SUCC_FIRST(start);
  t_node *current_node = start;
  t_node *par = NULL;
  t_bool ret = FALSE;

  NodeMarkInit ();

  ret = CallBack (current_node, par, data);

  do
  {
    NodeMark (current_node);
    if ((!current_succ_edge) || (ret))
    {
      ret = FALSE;
      if (stack_pointer)
      {
        /* Pop old successors (and nodes) form the stack */
        current_node = pstack[stack_pointer - 1];
        current_succ_edge = nstack[stack_pointer - 1];
        stack_pointer--;
      }
      else
      {
        break;
      }
    }
    else
    {
      if (EdgeTestCategoryOr
          (current_succ_edge,
           edge_category_or_mask_to_traverse_corresponding)
          && EDGE_CORR(current_succ_edge)
          && !NodeIsMarked (EDGE_TAIL(EDGE_CORR(current_succ_edge))))
      {
        if (stack_pointer == stack_size)
        {
          nstack =
            Realloc (nstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_edge *));
          pstack =
            Realloc (pstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_node *));
          stack_size += CONFIG_GT_STACK_ALLOC_SIZE;
        }
        par = current_node;
        nstack[stack_pointer] = current_succ_edge;
        pstack[stack_pointer] = current_node;
        stack_pointer++;

        /*DiabloPrint(stdout, "@iB\n", current_node); */

        current_node = EDGE_TAIL(EDGE_CORR(current_succ_edge));
        current_succ_edge = NODE_SUCC_FIRST(current_node);
        ret = CallBack (current_node, par, data);
      }
      else if (!NodeIsMarked (EDGE_TAIL(current_succ_edge))
               && EdgeTestCategoryOr (current_succ_edge,
                                      edge_category_or_mask_to_traverse))
      {
        if (stack_pointer == stack_size)
        {
          nstack =
            Realloc (nstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_edge *));
          pstack =
            Realloc (pstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_node *));
          stack_size += CONFIG_GT_STACK_ALLOC_SIZE;
        }
        par = current_node;
        nstack[stack_pointer] = current_succ_edge;
        pstack[stack_pointer] = current_node;
        stack_pointer++;

        /*DiabloPrint(stdout, "@iB\n", current_node); */

        current_node = EDGE_TAIL(current_succ_edge);
        current_succ_edge = NODE_SUCC_FIRST(current_node);
        ret = CallBack (current_node, par, data);
      }
      else
      {
        current_succ_edge = EDGE_SUCC_NEXT(current_succ_edge);
      }
    }
  }
  while (1);

  Free (nstack);
  Free (pstack);
}

typedef struct _bf_queue_node
{
  t_node *node;
  t_node *parent;
  struct _bf_queue_node *next;
} bf_queue_node;

void
GraphBFTraversal (t_node * start,
                  void (*CallBack) (t_node * curr, t_node * par),
                  t_uint32 edge_category_or_mask)
{

  bf_queue_node *Qhead = NULL;
  bf_queue_node *Qtail = NULL;
  bf_queue_node *temp;
  t_edge *edge;

  t_node *curr;
  t_node *par;

  NodeMarkInit ();

  if (start != NULL)
  {
    Qhead = Qtail = Malloc (sizeof (bf_queue_node));
    Qhead->node = start;
    NodeMark (Qhead->node);
    Qhead->parent = NULL;
    Qhead->next = NULL;
  }

  while (Qhead != NULL)
  {

    curr = Qhead->node;
    par = Qhead->parent;

    CallBack (curr, par);

    /* add successors to the queue */
    NODE_FOREACH_SUCC_EDGE(curr, edge)
    {
      if (!NodeIsMarked (EDGE_TAIL(edge))
          && (EDGE_CAT(edge) & edge_category_or_mask))
      { /* only add to queue if not already processed or in queue */
        Qtail->next = Malloc (sizeof (bf_queue_node));
        Qtail = Qtail->next;
        Qtail->next = NULL;
        Qtail->node = EDGE_TAIL(edge);
        NodeMark (EDGE_TAIL(edge));
        Qtail->parent = curr;
      }
    }

    /* remove the element we just processed from the queue */
    temp = Qhead;
    Qhead = Qhead->next;
    Free (temp);

  }

}

void
GraphDFReverseTraversalWithCheckAndData (t_node * start,
                                         t_bool (*CallBack) (t_node * curr,
                                                             t_node * par,
                                                             void *data),
                                         t_uint32 edge_category_or_mask,
                                         void *data)
{
  t_node **pstack = Malloc (sizeof (t_node *) * CONFIG_GT_STACK_ALLOC_SIZE);
  t_edge **nstack = Malloc (sizeof (t_edge *) * CONFIG_GT_STACK_ALLOC_SIZE);
  int stack_size = CONFIG_GT_STACK_ALLOC_SIZE;
  int stack_pointer = 0;
  t_edge *current_pred_edge = NODE_PRED_FIRST(start);
  t_node *current_node = start;
  t_node *par = NULL;
  t_bool ret = FALSE;

  NodeMarkInit ();

  ret = CallBack (current_node, par, data);

  do
  {
    NodeMark (current_node);
    if ((!current_pred_edge) || (ret))
    {
      ret = FALSE;
      if (stack_pointer)
      {
        /* Pop old successors (and nodes) form the stack */
        current_node = pstack[stack_pointer - 1];
        current_pred_edge = nstack[stack_pointer - 1];
        stack_pointer--;
        current_pred_edge = EDGE_PRED_NEXT(current_pred_edge);
      }
      else
      {
        break;
      }
    }
    else
    {
      if (!NodeIsMarked (EDGE_HEAD(current_pred_edge))
          && EdgeTestCategoryOr (current_pred_edge,
                                 edge_category_or_mask))
      {
        if (stack_pointer == stack_size)
        {
          nstack =
            Realloc (nstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_edge *));
          pstack =
            Realloc (pstack,
                     (stack_size +
                      CONFIG_GT_STACK_ALLOC_SIZE) *
                     sizeof (t_node *));
          stack_size += CONFIG_GT_STACK_ALLOC_SIZE;
        }
        par = current_node;
        nstack[stack_pointer] = current_pred_edge;
        pstack[stack_pointer] = current_node;
        stack_pointer++;
        current_node = EDGE_HEAD(current_pred_edge);
        current_pred_edge = NODE_PRED_FIRST(current_node);
        ret = CallBack (current_node, par, data);
      }
      else
      {
        current_pred_edge = EDGE_PRED_NEXT(current_pred_edge);
      }
    }
  }
  while (1);

  Free (nstack);
  Free (pstack);
}

/* Orders the lists of nodes, edges, successors and predecessors */
void
GraphOrder (t_graph * graph, NodeCmpFun fun)
{
  t_node *current_node;
  t_node *next_node;
  t_edge *current_edge;
  t_edge *next_edge;
  unsigned int nr_cessors, i;  

  /*order nodes {{{ */
  for (i = 0; i < GRAPH_NNODES(graph); i++)
  {

    current_node = GRAPH_NODE_FIRST(graph);
    if (current_node == NULL)
      return;
    next_node = NODE_NEXT(current_node);

    while (next_node != NULL)
    {
      if (fun (current_node, next_node) == 1)
      {

        NODE_SET_PREV(next_node, NODE_PREV(current_node));
        NODE_SET_NEXT(current_node, NODE_NEXT(next_node));

        if (NODE_PREV(next_node) == NULL)
          GRAPH_SET_NODE_FIRST(graph, next_node);
        else
          NODE_SET_NEXT(NODE_PREV(next_node), next_node);

        if (NODE_NEXT(current_node) == NULL)
          GRAPH_SET_NODE_LAST(graph, current_node);
        else
          NODE_SET_PREV(NODE_NEXT(current_node), current_node);

        NODE_SET_NEXT(next_node, current_node);
        NODE_SET_PREV(current_node, next_node);
      }
      else
      {
        current_node = NODE_NEXT(current_node);
      }
      next_node = NODE_NEXT(current_node);
    }
  }
  /*}}} */

  /*Order list of edges {{{ */
  for (i = 0; i < GRAPH_NEDGES(graph); i++)
  {

    current_edge = GRAPH_EDGE_FIRST(graph);
    if (current_edge == NULL)
      return;
    next_edge = EDGE_NEXT(current_edge);

    while (next_edge != NULL)
    {
      t_uint8 tmp = fun (EDGE_TAIL(current_edge), EDGE_TAIL(next_edge));
      t_uint8 tmp2 =
        fun (EDGE_HEAD(current_edge), EDGE_HEAD(next_edge));

      if (tmp == 1 || (tmp == 0 && tmp2 == 1))
      {
        EDGE_SET_PREV(next_edge, EDGE_PREV(current_edge));
        EDGE_SET_NEXT(current_edge, EDGE_NEXT(next_edge));

        if (EDGE_PREV(next_edge) == NULL)
          GRAPH_SET_EDGE_FIRST(graph, next_edge);
        else
          EDGE_SET_NEXT(EDGE_PREV(next_edge), next_edge);

        if (EDGE_NEXT(current_edge) == NULL)
          GRAPH_SET_EDGE_LAST(graph, current_edge);
        else
          EDGE_SET_PREV(EDGE_NEXT(current_edge), current_edge);

        EDGE_SET_NEXT(next_edge, current_edge);
        EDGE_SET_PREV(current_edge, next_edge);
      }
      else
      {
        current_edge = EDGE_NEXT(current_edge);
      }
      next_edge = EDGE_NEXT(current_edge);
    }
  }
  /*}}} */

  /*Order list of successors {{{ */
  GRAPH_FOREACH_NODE(graph, current_node)
  {
    nr_cessors = 0;
    NODE_FOREACH_SUCC_EDGE(current_node, current_edge)
    {
      nr_cessors++;
    }

    for (i = 0; i < nr_cessors; i++)
    {

      current_edge = NODE_SUCC_FIRST(current_node);
      if (current_edge == NULL)
      {
        return;
      }
      next_edge = EDGE_SUCC_NEXT(current_edge);

      while (next_edge != NULL)
      {
        if (fun (EDGE_TAIL(current_edge), EDGE_TAIL(next_edge)) == 1)
        {
          EDGE_SET_SUCC_PREV(next_edge, EDGE_SUCC_PREV(current_edge));
          EDGE_SET_SUCC_NEXT(current_edge, EDGE_SUCC_NEXT(next_edge));

          if (EDGE_SUCC_PREV(next_edge) == NULL)
            NODE_SET_SUCC_FIRST(current_node, next_edge);
          else
            EDGE_SET_SUCC_NEXT(EDGE_SUCC_PREV(next_edge), next_edge);

          if (EDGE_SUCC_NEXT(current_edge) == NULL)
            NODE_SET_SUCC_LAST(current_node, current_edge);
          else
            EDGE_SET_SUCC_PREV(EDGE_SUCC_NEXT(current_edge), current_edge);

          EDGE_SET_SUCC_NEXT(next_edge, current_edge);
          EDGE_SET_SUCC_PREV(current_edge, next_edge);
        }
        else
        {
          current_edge = EDGE_SUCC_NEXT(current_edge);
        }
        next_edge = EDGE_SUCC_NEXT(current_edge);
      }
    }
  }
  /*}}} */

  /*Order list of predecessors {{{ */
  GRAPH_FOREACH_NODE(graph, current_node)
  {
    nr_cessors = 0;
    NODE_FOREACH_PRED_EDGE(current_node, current_edge)
    {
      nr_cessors++;
    }

    for (i = 0; i < nr_cessors; i++)
    {

      current_edge = NODE_PRED_FIRST(current_node);
      if (current_edge == NULL)
      {
        return;
      }
      next_edge = EDGE_PRED_NEXT(current_edge);

      while (next_edge != NULL)
      {
        if (fun (EDGE_TAIL(current_edge), EDGE_TAIL(next_edge)) == 1)
        {
          EDGE_SET_PRED_PREV(next_edge, EDGE_PRED_PREV(current_edge));
          EDGE_SET_PRED_NEXT(current_edge, EDGE_PRED_NEXT(next_edge));

          if (EDGE_PRED_PREV(next_edge) == NULL)
            NODE_SET_PRED_FIRST(current_node, next_edge);
          else
            EDGE_SET_PRED_NEXT(EDGE_PRED_PREV(next_edge), next_edge);

          if (EDGE_PRED_NEXT(current_edge) == NULL)
            NODE_SET_PRED_LAST(current_node, current_edge);
          else
            EDGE_SET_PRED_PREV(EDGE_PRED_NEXT(current_edge), current_edge);

          EDGE_SET_PRED_NEXT(next_edge, current_edge);
          EDGE_SET_PRED_PREV(current_edge, next_edge);
        }
        else
        {
          current_edge = EDGE_PRED_NEXT(current_edge);
        }
        next_edge = EDGE_PRED_NEXT(current_edge);
      }
    }
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
