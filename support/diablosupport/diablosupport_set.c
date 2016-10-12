/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>
#include <string.h>

t_bool
SetIteratorHasNext (const t_set_iterator * it)
{
  if (it->count <= it->set->nr_elements)
    return TRUE;
  return FALSE;
}

void *
SetIteratorNext (t_set_iterator * it)
{
  if (SetIteratorHasNext (it))
  {
    void *result = NULL;
    t_set *set = it->set;

    while ((result = set->data[it->position]) == NULL)
      it->position++;
    it->position++;
    it->count++;
    it->may_remove = TRUE;
    return result;
  }
  else
  {
    it->may_remove = FALSE;
    return NULL;
  }
}

void
SetIteratorRemove (const t_set_iterator * it)
{
  if (it->may_remove)
  {
    it->set->data[it->position - 1] = NULL;
    (it->set->nr_elements)--;
  }
  else
    FATAL(("remove not allowed"));
}

t_set_iterator *
SetIteratorGet (t_set * set)
{
  t_set_iterator *result = (t_set_iterator *) Calloc (sizeof (t_set_iterator), 1);

  set->nr_iterators_active++;
  result->set = set;
  return result;
}

void
SetIteratorKill (const t_set_iterator * it)
{
  it->set->nr_iterators_active--;
  Free (it);
}

void
SetPrint (const t_set * set)
{
  int i;

  printf ("%p: capacity %d, nr_elements %d, first_free %d, elems: ", set, set->capacity, set->nr_elements, set->first_free);
  for (i = 0; i < set->capacity; i++)
  {
    if (set->data[i] == NULL)
      printf ("e, ");
    else
      printf ("%p, ", set->data[i]);
  }
  printf ("\n");
  fflush (stdout);

}

void
SetAddElemBlindly (t_set * set, void *elem)
{
  if (set->first_free < 0)
  {
    /* need to alloc new memory */
    set->capacity *= 2;

    set->data = Realloc (set->data, sizeof (void *) * set->capacity);

    memset (((char *) set->data) + set->capacity / 2 * sizeof (void *), 0, set->capacity / 2 * sizeof (void *));

    set->first_free = set->capacity / 2;
    set->data[set->first_free] = elem;
    set->first_free++;
    set->nr_elements++;

    return;
  }

  set->data[set->first_free] = elem;
  set->first_free++;
  set->nr_elements++;

  while (set->first_free < set->capacity)
  {
    if (set->data[set->first_free] == NULL)
    {
      return;
    }
    set->first_free++;
  }

  set->first_free = -1;

}

t_bool
SetAddElem (t_set * set, void *elem)
{
  int i;

  for (i = 0; i < set->capacity; i++)
    if (set->data[i] == elem)
      return TRUE;
  SetAddElemBlindly (set, elem);
  return FALSE;
}

void
SetRemoveElem (t_set * set, const void *elem)
{
  int i;

  for (i = 0; i < set->capacity; i++)
    if (set->data[i] == elem)
    {
      set->data[i] = NULL;
      set->nr_elements--;
      if (i < set->first_free)
        set->first_free = i;

      return;
    }
}

void
SetRemoveElemAt (t_set * set, int i)
{
  if (set->data[i] != NULL)
  {
    set->data[i] = NULL;
    set->nr_elements--;
    if (i < set->first_free)
      set->first_free = i;
    return;
  }
}

t_set *
SetNew (int size)
{
  t_set *result = (t_set *) Calloc (1, sizeof (t_set));

  result->capacity = size;
  result->data = (void **) Calloc (size, sizeof (void *));
  return result;
}

void
SetFree (const t_set * set)
{
  if ((set))
  {
    if ((set)->data)
      Free ((set)->data);
    Free (set);
  }
}

void
SetReplaceElem (t_set * set, const void *old, void *new)
{
  int i, count;

  for (i = 0, count = 0; count < set->nr_elements; i++)
  {
    if (set->data[i] == NULL)
      continue;
    if (set->data[i] == old)
    {
      set->data[i] = new;
    }
    count++;
  }
}

void *
SetGetFirstElement (const t_set * set)
{
  int i, count;

  for (i = 0, count = 0; count < set->nr_elements; i++)
  {
    if (set->data[i] == NULL)
      continue;
    return set->data[i];
    count++;
  }
  return NULL;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
