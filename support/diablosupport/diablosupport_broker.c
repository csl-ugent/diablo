#include <diablosupport.h>
#include <stdarg.h>
#include <string.h>

static t_hash_table *brokerht = NULL;

typedef struct _t_broker_cbdef
{
  t_hash_table_node node;
  t_string_array *proto;
  void *impl;
  void *data;
  t_bool final;
  struct _t_broker_cbdef *next;
} t_broker_cbdef;

static t_broker_cbdef *
DiabloBrokerCallLookup (const char *name)
{
  if (!brokerht)
    return NULL;
  return HashTableLookup (brokerht, name);
}

t_bool
DiabloBrokerCallExists (const char *name)
{
  return (DiabloBrokerCallLookup (name) != NULL);
}

void
DiabloBrokerCallInstall (const char *name, const char *prototype, void *implementation, t_bool final, ...)
{
  va_list ap;
  t_string_array_elem *elem, *elem2;
  t_broker_cbdef *cb, *cb2;

  if (!brokerht)
    brokerht = HashTableNew (121, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, NULL);

  if ((cb = DiabloBrokerCallLookup (name)))
  {
    cb2 = Calloc (1, sizeof (t_broker_cbdef));
    cb2->proto = StringDivide (prototype, ",", FALSE, FALSE);
    cb2->data = NULL;
    cb2->impl = implementation;
    cb2->final = final;
    if (!final)
    {
      t_string_array *tmp;
      void *timpl;

      tmp = cb->proto;
      cb->proto = cb2->proto;
      cb2->proto = tmp;
      cb2->data = cb->data;
      cb2->final = cb->final;
      cb->final = final;
      cb->data = NULL;
      timpl = cb->impl;
      cb->impl = cb2->impl;
      cb2->impl = timpl;
      cb2->next = cb->next;
      cb->next = cb2;
    }
    else
    {
restart:
      elem2 = cb2->proto->first;
      STRING_ARRAY_FOREACH_ELEM(cb->proto, elem)
      {
        if (!(strncmp (elem->string, "const ", 6)))
        {
          if ((!(strncmp (elem2->string, "const ", 6))) || (!cb->next))
          {
            cb2->next = cb->next;
            cb->next = cb2;
            cb = cb2;
            break;
          }
          else
          {
            cb = cb->next;
            goto restart;
          }
        }
        else if (!(strncmp (elem2->string, "const ", 6)))
        {
          /* Call cb, first, then cb2 */
          t_string_array *tmp;
          void *timpl;

          tmp = cb->proto;
          cb->proto = cb2->proto;
          cb2->proto = tmp;
          cb2->data = cb->data;
          cb2->final = cb->final;
          cb->final = final;
          cb->data = NULL;
          timpl = cb->impl;
          cb->impl = cb2->impl;
          cb2->impl = timpl;
          cb2->next = cb->next;
          cb->next = cb2;
          break;
        }
        else if (strcmp (elem->string, elem2->string))
        {
          FATAL(("Prototype disambiguity for two broker calls: proto1 = %s, proto2=%s\n", StringArrayJoin (cb->proto, ","), prototype));
        }

        elem2 = elem2->next;
        if (!elem2)
          FATAL(("Cannot distiguish between two broker calls: proto1 = %s, proto2=%s\n", StringArrayJoin (cb->proto, ","), prototype));
      }
    }
  }
  else
  {
    cb = Calloc (1, sizeof (t_broker_cbdef));
    cb->proto = StringDivide (prototype, ",", FALSE, FALSE);
    cb->data = NULL;
    cb->impl = implementation;
    cb->final = final;
    HASH_TABLE_NODE_SET_KEY(&cb->node, StringDup (name));
    HashTableInsert (brokerht, cb);
  }

  STRING_ARRAY_FOREACH_ELEM(cb->proto, elem)
  {
    if (!(strncmp (elem->string, "const ", 6)))
    {
      if (cb->data)
        FATAL(("DiabloBrokerCallInstall: More then one const argument!"));
      else
      {
        va_start (ap, final);
        cb->data = va_arg (ap, void *);
      }
    }
  }

  if (cb->proto->nstrings > 10)
    FATAL(("To many arguments for broker call"));
}

t_bool
DiabloBrokerCall (const char *name, ...)
{
  t_bool execed = FALSE;
  va_list ap;
  int tel = 0;
  void *args[10];
  t_string_array_elem *elem;

  /* Lookup name in hash table */
  t_broker_cbdef *callback_definition = DiabloBrokerCallLookup (name);

  do
  {
    t_bool found = FALSE;

    while (callback_definition && (!found))
    {
      found = TRUE;
      va_start (ap, name);
      tel = 0;
      STRING_ARRAY_FOREACH_ELEM(callback_definition->proto, elem)
      {
        args[tel] = (void *) va_arg (ap, void *);

        if (!(strncmp (elem->string, "const ", 6)))
        {
          if (args[tel] != callback_definition->data)
          {
            found = FALSE;
            break;
          }
        }
        tel++;
      }
      if (!found)
        callback_definition = callback_definition->next;
    }

    if (!callback_definition)
      return execed;

    switch (callback_definition->proto->nstrings)
    {
      case 0:
        ((void (*)()) (callback_definition->impl)) ();
        break;
      case 1:
        ((void (*)(void *)) (callback_definition->impl)) (args[0]);
        break;
      case 2:
        ((void (*)(void *, void *)) (callback_definition->impl)) (args[0], args[1]);
        break;
      case 3:
        ((void (*)(void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2]);
        break;
      case 4:
        ((void (*)(void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3]);
        break;
      case 5:
        ((void (*)(void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4]);
        break;
      case 6:
        ((void (*)(void *, void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4], args[5]);
        break;
      case 7:
        ((void (*)(void *, void *, void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        break;
      case 8:
        ((void (*)(void *, void *, void *, void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        break;
      case 9:
        ((void (*)(void *, void *, void *, void *, void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
        break;
      case 10:
        ((void (*)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *)) (callback_definition->impl)) (args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
        break;
    }

    execed = TRUE;
  }
  while ((!callback_definition->final) && (callback_definition = callback_definition->next));

  return execed;
}

static void
BrokerCbdefFree (void *hash_node, void *data)
{
  t_broker_cbdef *broker_cbdef = (t_broker_cbdef *) hash_node;

  while (broker_cbdef)
  {
    t_broker_cbdef *next = broker_cbdef->next;

    StringArrayFree (broker_cbdef->proto);
    if(HASH_TABLE_NODE_KEY(&(broker_cbdef->node)))
      Free (HASH_TABLE_NODE_KEY(&(broker_cbdef->node)));
    Free (broker_cbdef);
    broker_cbdef = next;
  }
}

void
DiabloBrokerFini ()
{
  if (brokerht)
  {
    HashTableWalk (brokerht, BrokerCbdefFree, NULL);
    HashTableFree (brokerht);
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
