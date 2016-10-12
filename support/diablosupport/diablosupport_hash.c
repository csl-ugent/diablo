/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_HASH_NODE(x) ((data_hash_table *)(((char*)(x))+HASH_TABLE_OFFSET(table)))

void
HashTableInit (t_hash_table * ret, t_uint32 size, t_uint32 offset,
              t_hash_func hash_func,
              t_hash_cmp cmp, t_hash_node_free node_free)
{
  t_uint32 tel;

  HASH_TABLE_SET_NODES(ret, 
    (t_hash_table_node **) Malloc (size * sizeof (t_hash_table_node *)));
  HASH_TABLE_SET_TSIZE(ret, size);
  HASH_TABLE_SET_OFFSET(ret, offset);
  HASH_TABLE_SET_HASH_FUNC(ret, hash_func);
  HASH_TABLE_SET_HASH_CMP(ret, cmp);
  HASH_TABLE_SET_NODE_FREE(ret, node_free);
  HASH_TABLE_SET_NELEMENTS(ret, 0);
  for (tel = 0; tel < HASH_TABLE_TSIZE(ret); tel++)
    HASH_TABLE_NODES(ret)[tel] = NULL;

}

/* Create an empty hash table */
t_hash_table *
HashTableNew (t_uint32 size, t_uint32 offset,
              t_hash_func hash_func,
              t_hash_cmp cmp, t_hash_node_free node_free)
{
  t_hash_table *ret = (t_hash_table *) Malloc (sizeof (t_hash_table));

  HashTableInit(ret, size, offset, hash_func, cmp, node_free);
  return ret;
}

void 
HashTableFini(const t_hash_table * table)
{
  if (HASH_TABLE_NODE_FREE(table))
    HashTableWalk ((t_hash_table *) table, (void (*)(void *, void *)) HASH_TABLE_NODE_FREE(table), NULL);
  Free (HASH_TABLE_NODES(table));
}

/* Free the hash table and all elements in it */
void
HashTableFree (const t_hash_table * table)
{
  HashTableFini (table);
  Free (table);
}

void
HashTableSetKeyForNode (t_hash_table * table, void *vnode, void *key)
{
  t_hash_table_node *node =
    (t_hash_table_node *) (((char *) vnode) + HASH_TABLE_OFFSET(table));

  HASH_TABLE_NODE_SET_KEY(node, key);
}

/* Insert an allocated node in the hash table. The key of the node has to be initialized ! */

void
HashTableInsert (t_hash_table * table, void *vnode)
{
  t_hash_table_node *node =
    (t_hash_table_node *) (((char *) vnode) + HASH_TABLE_OFFSET(table));
  void *vit;
  t_hash_table_node *it;
  t_uint32 n = HASH_TABLE_HASH_FUNC(table) (HASH_TABLE_NODE_KEY(node), table);

  HASH_TABLE_NODE_SET_EQUAL(node, NULL);
  HASH_TABLE_NODE_SET_HASH_NEXT(node, NULL);
  HASH_TABLE_NODE_SET_HASH_KEY(node, n);
  for (vit = HASH_TABLE_NODES(table)[n], it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL;
       it != NULL;
       vit = HASH_TABLE_NODE_HASH_NEXT(it), it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)
  {
    if (HASH_TABLE_HASH_CMP(table) (HASH_TABLE_NODE_KEY(it), HASH_TABLE_NODE_KEY(node)) == 0)
    {
      HASH_TABLE_NODE_SET_EQUAL(node, HASH_TABLE_NODE_EQUAL(it));
      HASH_TABLE_NODE_SET_EQUAL(it, vnode);
      return;
    }

  }
  if (HASH_TABLE_NODES(table)[n])
    HASH_TABLE_NODE_SET_HASH_NEXT(node, HASH_TABLE_NODES(table)[n]);
  else
    HASH_TABLE_NODE_SET_HASH_NEXT(node, NULL);

  HASH_TABLE_NODES(table)[n] = vnode;

  HASH_TABLE_SET_NELEMENTS(table, HASH_TABLE_NELEMENTS(table) + 1);
}

void
HashTablePrepend (t_hash_table * table, void *vnode)
{
  t_hash_table_node *node =
    (t_hash_table_node *) (((char *) vnode) + HASH_TABLE_OFFSET(table));
  void *vit, *vprev;
  t_hash_table_node *it, *prev = NULL;
  t_uint32 n = HASH_TABLE_HASH_FUNC(table) (HASH_TABLE_NODE_KEY(node), table);

  HASH_TABLE_NODE_SET_EQUAL(node, NULL);
  HASH_TABLE_NODE_SET_HASH_NEXT(node, NULL);
  HASH_TABLE_NODE_SET_HASH_KEY(node, n);
  for (vit = HASH_TABLE_NODES(table)[n], it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : 0;
       it != NULL;
       prev = it, vprev = vit, vit = HASH_TABLE_NODE_HASH_NEXT(it), it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)
  {
    if (HASH_TABLE_HASH_CMP(table) (HASH_TABLE_NODE_KEY(it), HASH_TABLE_NODE_KEY(node)) == 0)
    {
      HASH_TABLE_NODE_SET_EQUAL(node, vit);
      HASH_TABLE_NODE_SET_HASH_NEXT(node, HASH_TABLE_NODE_HASH_NEXT(it));
      if (prev)
        HASH_TABLE_NODE_SET_HASH_NEXT(prev, vnode);
      else
        HASH_TABLE_NODES(table)[n] = vnode;
      return;
    }
  }

  if (HASH_TABLE_NODES(table)[n])
    HASH_TABLE_NODE_SET_HASH_NEXT(node, HASH_TABLE_NODES(table)[n]);
  else
    HASH_TABLE_NODE_SET_HASH_NEXT(node, NULL);
  HASH_TABLE_NODES(table)[n] = vnode;
}

void
HashTableUnlink (t_hash_table * table, void *vnode)
{
  t_hash_table_node *node =
    (t_hash_table_node *) (((char *) vnode) + HASH_TABLE_OFFSET(table));
  void *vit, *vprev;
  t_hash_table_node *it, *prev = NULL;
  t_uint32 n = HASH_TABLE_HASH_FUNC(table) (HASH_TABLE_NODE_KEY(node), table);

  if (HASH_TABLE_NODES(table)[n] == NULL)
    FATAL(("Trying to delete a node that is no longer in the table!!!!"));

  for (vit = HASH_TABLE_NODES(table)[n], it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : 0;
       it != NULL;
       prev = it, vprev = vit, vit = HASH_TABLE_NODE_HASH_NEXT(it), it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)
  {
    if (HASH_TABLE_HASH_CMP(table) (HASH_TABLE_NODE_KEY(it), HASH_TABLE_NODE_KEY(node)) == 0) /*iterate to first node with same key */
    {
      if (vit == vnode) /*if the first node with the same key is the node to be deleted */
      {

        if (HASH_TABLE_NODE_EQUAL(it)) /*if the equal field is not NULL, then the equal node comes in place */
        {
          HASH_TABLE_NODE_SET_HASH_NEXT((t_hash_table_node *) (((char *) HASH_TABLE_NODE_EQUAL(it)) + HASH_TABLE_OFFSET(table)), HASH_TABLE_NODE_HASH_NEXT(it));
          if (prev)
            HASH_TABLE_NODE_SET_HASH_NEXT(prev, HASH_TABLE_NODE_EQUAL(node));
          else
            HASH_TABLE_NODES(table)[n] = HASH_TABLE_NODE_EQUAL(node);
        }

        else /*otherwise the next node comes in place */


        {
          if (prev)
            HASH_TABLE_NODE_SET_HASH_NEXT(prev, HASH_TABLE_NODE_HASH_NEXT(node));
          else
            HASH_TABLE_NODES(table)[n] = HASH_TABLE_NODE_HASH_NEXT(node);
        }
        return;
      }

      for (vprev = vit, prev = it, vit = HASH_TABLE_NODE_EQUAL(it), it = vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL; it != NULL; vprev = vit, prev = it, vit = HASH_TABLE_NODE_EQUAL(it), it = vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL) /*search the correct node via the equal fields */
      {
        if (vit == vnode) /*the node is found */
        {
          HASH_TABLE_NODE_SET_EQUAL(prev, HASH_TABLE_NODE_EQUAL(it));
          return;
        }
      }
    }
  }
  FATAL(("Trying to delete a hash node which is not in the table!!, no matching key found"));
}

/* Delete a node in the hashtable (second argument=node!!!)
 * Not the most elegant code, but it works (pruts) */
void
HashTableDelete (t_hash_table * table, const void *vnode)
{
  t_hash_table_node *node =
    (t_hash_table_node *) (((char *) vnode) + HASH_TABLE_OFFSET(table));
  void *vit, *vprev;
  t_hash_table_node *it, *prev = NULL;
  t_uint32 n = HASH_TABLE_HASH_FUNC(table) (HASH_TABLE_NODE_KEY(node), table);

  if (HASH_TABLE_NODES(table)[n] == NULL)
    FATAL(("Trying to delete a node that is no longer in the table!!!!"));

  for (vit = HASH_TABLE_NODES(table)[n], it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : 0;
       it != NULL;
       prev = it, vprev = vit, vit = HASH_TABLE_NODE_HASH_NEXT(it), it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)
  {
    if (HASH_TABLE_HASH_CMP(table) (HASH_TABLE_NODE_KEY(it), HASH_TABLE_NODE_KEY(node)) == 0) /*iterate to first node with same key */
    {
      if (vit == vnode) /*if the first node with the same key is the node to be deleted */
      {
        if (HASH_TABLE_NODE_EQUAL(it)) /*if the equal field is not NULL, then the equal node comes in place */
        {
          HASH_TABLE_NODE_SET_HASH_NEXT((t_hash_table_node *) (((char *) HASH_TABLE_NODE_EQUAL(it)) + HASH_TABLE_OFFSET(table)), HASH_TABLE_NODE_HASH_NEXT(it));
          if (prev)
            HASH_TABLE_NODE_SET_HASH_NEXT(prev, HASH_TABLE_NODE_EQUAL(node));
          else
            HASH_TABLE_NODES(table)[n] = HASH_TABLE_NODE_EQUAL(node);
        }
        else /*otherwise the next node comes in place */
        {
          if (prev)
            HASH_TABLE_NODE_SET_HASH_NEXT(prev, HASH_TABLE_NODE_HASH_NEXT(node));
          else
            HASH_TABLE_NODES(table)[n] = HASH_TABLE_NODE_HASH_NEXT(node);
        }
        HASH_TABLE_NODE_FREE(table) (vit, NULL); /*free the deleted node */
        HASH_TABLE_NODE_SET_KEY(it, NULL);
        return;
      }

      for (vprev = vit, prev = it, vit = HASH_TABLE_NODE_EQUAL(it), it = vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL; it != NULL; vprev = vit, prev = it, vit = HASH_TABLE_NODE_EQUAL(it), it = (vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)) /*search the correct node via the equal fields */
      {
        if (vit == vnode) /*the node is found */
        {
          HASH_TABLE_NODE_SET_EQUAL(prev, HASH_TABLE_NODE_EQUAL(it));
          HASH_TABLE_NODE_FREE(table) (vit, NULL);
          HASH_TABLE_NODE_SET_KEY(it, NULL);
          return;
        }
      }
      /* We will fatal! */
      break;
    }
  }
  FATAL(("Trying to delete a hash node which is not in the table!!"));
}

/* Check if a certain node is present in the hash table */
t_bool HashTableIsPresent(const t_hash_table* table, const void* vnode)
{
  const t_hash_table_node* node = T_HASH_TABLE_NODE(((char *) vnode) + HASH_TABLE_OFFSET(table));

  /* If there's no key, it's not part of a hash table anymore */
  if (!HASH_TABLE_NODE_KEY(node))
    return FALSE;

  /* Go over the result chain until you find the node (or you don't) */
  const void* res = HashTableLookup (table, HASH_TABLE_NODE_KEY(node));
  while (res)
  {
    if (res == vnode)
      return TRUE;

    const t_hash_table_node* resnode = T_HASH_TABLE_NODE(((char *) res) + HASH_TABLE_OFFSET(table));
    res = HASH_TABLE_NODE_EQUAL(resnode);
  }

  return FALSE;
}

/* Find a node in the hashtable (the node that was inserted first in case of equal keys) */
void *
HashTableLookup (const t_hash_table * table, const void *key)
{
  t_hash_table_node *it;
  void *vit;
  t_uint32 n = HASH_TABLE_HASH_FUNC(table) (key, table);

  for (vit = HASH_TABLE_NODES(table)[n], it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL;
       it != NULL;
       vit = HASH_TABLE_NODE_HASH_NEXT(it), it =
       vit ? (t_hash_table_node *) (((char *) vit) + HASH_TABLE_OFFSET(table)) : NULL)
  {
    if (HASH_TABLE_HASH_CMP(table) (HASH_TABLE_NODE_KEY(it), key) == 0)
    {
      return vit;
    }
  }

  return NULL;
}

/* Visit all elements in the hash_table and perform function on them */

void
HashTableWalk (t_hash_table * table, void (*func) (void *, void *),
               void *data)
{
  t_uint32 n;
  t_hash_table_node *it, *itn, *it2, *it2n;

  void *vit, *vit2, *vit2n;

  for (n = 0; n < HASH_TABLE_TSIZE(table); n++)
  {
    for (vit = HASH_TABLE_NODES(table)[n], it =
         vit ? (t_hash_table_node *) (((char *) HASH_TABLE_NODES(table)[n]) +
                                      HASH_TABLE_OFFSET(table)) : NULL; it != NULL;
         vit = itn, it =
         vit ? (t_hash_table_node *) (((char *) vit) +
                                      HASH_TABLE_OFFSET(table)) : NULL)
    {
      itn = HASH_TABLE_NODE_HASH_NEXT(it);
      vit2n = HASH_TABLE_NODE_EQUAL(it);
      it2n =
        vit2n ? (t_hash_table_node *) (((char *) vit2n) +
                                       HASH_TABLE_OFFSET(table)) : NULL;
      func (vit, data);
      for (vit2 = vit2n, it2 = it2n; it2 != NULL;
           vit2 = vit2n, it2 = it2n)
      {
        vit2n = HASH_TABLE_NODE_EQUAL(it2);
        it2n =
          vit2n ? (t_hash_table_node *) (((char *) vit2n) +
                                         HASH_TABLE_OFFSET(table)) : 0;
        func (vit2, data);
      }
    }
  }
}

void
HashTableStats (const t_hash_table * table)
{
  t_uint32 n;
  t_hash_table_node *it;
  t_uint32 this;
  t_uint32 max = 0;
  t_uint32 maxfill = 0;
  t_uint32 minfill = ((t_uint32) 1) << 31;
  t_uint32 totalfill = 0;

  for (n = 0; n < HASH_TABLE_TSIZE(table); n++)
  {
    this = 0;
    for (it = HASH_TABLE_NODES(table)[n]; it != NULL; it = HASH_TABLE_NODE_HASH_NEXT(it))
    {
      this++;
    }
    if (this > maxfill)
    {
      maxfill = this;
      max = n;
    }
    if (this < minfill)
      minfill = this;
    totalfill += this;
  }

  printf ("Average fill= %f\n", ((float) totalfill) / n);
  printf ("Max fill= %d\n", maxfill);
  printf ("Min fill= %d\n", minfill);
  printf ("Nelems= %d\n", totalfill);

  /* This will fail if the hash_key is not a string! */
  printf ("MAX\n");
  for (it = HASH_TABLE_NODES(table)[max]; it != NULL; it = HASH_TABLE_NODE_HASH_NEXT(it))
  {
    printf ("%s\n", (char *) HASH_TABLE_NODE_KEY(it));
  }
  printf ("END MAX\n");

}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
