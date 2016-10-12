#include <diablosupport.h>
t_slist *
SlistNew ()
{
  t_slist *ret = (t_slist *) calloc (1, sizeof (t_slist));

  ret->type = SLIST;
  return ret;
}

t_dlist *
DlistNew ()
{
  t_dlist *ret = (t_dlist *) calloc (1, sizeof (t_dlist));

  ret->type = DLIST;
  return ret;
}

t_xlist *
XlistNew ()
{
  t_xlist *ret = (t_xlist *) calloc (1, sizeof (t_xlist));

  ret->type = XLIST;
  return ret;
}

void
SlistAppend (t_slist * list, void *in)
{

  t_dlist_node *node_in = ListGetElem (list, in);

  node_in->next = list->first;
  list->first = in;
}

void
DlistAppend (t_dlist * list, void *in)
{

  t_dlist_node *node_in = ListGetElem (list, in);

  if (list->last)
    DListGetElem (list, list->last)->next = in;
  node_in->prev = list->last;
  node_in->next = NULL;
  list->last = in;
  if (!list->first)
    list->first = in;
}

void
XlistAppend (t_xlist * list, void *in)
{

  t_xlist_node *node_in = ListGetElem (list, in);

  if (list->last)
    XListGetElem (list, list->last)->pointer ^= (t_pointer_int) in;
  node_in->pointer = (t_pointer_int) list->last;
  list->last = in;
  if (!list->first)
    list->first = in;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
