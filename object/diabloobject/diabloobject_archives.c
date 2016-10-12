/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <diabloobject.h>

static t_archive_handler *archive_table = NULL;
t_hash_table *archives_global_hash_table = NULL;

/*! Utility function, used to free all structures stored in the global archive
 * hash table
 *
 * \param to_free A node of the hash table that must be freed
 * \param unused_data Unused data
 */
void
ArchiveHashTableNodeFree (const void *to_free, void *unused_data)
{
  t_archive_hash_table_node *node = (t_archive_hash_table_node *) to_free;

  ArchiveFree (node->arch);
  Free (HASH_TABLE_NODE_KEY(&node->node));
  Free (node);
}

/*! Free an node from the archive-object hash table (used in the HashTable
 * constructor, automatically called by the hashtable code).
 *
 * \param to_free A node of the archive objec
 * \param data \todo
 * */

void
ArchiveObjectHashTableNodeFree (const void *to_free, void *data)
{
  t_archive_object_hash_table_node *node = (t_archive_object_hash_table_node *) to_free;

  Free (HASH_TABLE_NODE_KEY(&node->node));
  Free (node);
}

void
ArchiveSymbolHashTableNodeFree (const void *tf, void *data)
{
  t_archive_symbol_hash_table_node *node = (t_archive_symbol_hash_table_node *) tf;

  Free (HASH_TABLE_NODE_KEY(&node->node));
  Free (node->name);
  Free (node);
}
/*! Free all structures and substructure used by an archive
 *
 * \param arch the archive to free
 */

void
ArchiveFree (const t_archive * arch)
{
  /* Use a non-const version to close the archive. Bit dirty but oh well */
  if (arch->handler->close) arch->handler->close((t_archive*)arch);
 
  Free (arch->name);
  if (arch->objects) HashTableFree (arch->objects);
  if (arch->symbols) HashTableFree (arch->symbols);
  Free (arch);
}

/*! Constructor of an archive. Should probably not be used. Use ArchiveOpenCached
 * instead
 */

static t_archive *
ArchiveNew ()
{
  t_archive *ret;

  ret = Malloc (sizeof (t_archive));
  return ret;
}

t_archive_handler *
ArchiveHandlerAdd (t_const_string handler_main, t_bool (*ident) (FILE *),
                   void (*read) (FILE *, t_archive *),
                   t_object * (*get_object) (const t_archive *, t_const_string, t_object *, t_bool),
                   t_object * (*get_object_by_symbol) (const t_archive *, t_const_string, t_object *, t_bool),
                   void (*close)(t_archive *))
{
  static t_uint32 id = 0;
  t_archive_handler *h = Malloc (sizeof (t_archive_handler));

  id++;

  h->next_handler = archive_table;
  if (archive_table)
    archive_table->prev_handler = h;
  h->prev_handler = NULL;
  archive_table = h;

  h->ident = ident;
  h->read = read;
  h->close = close;
  h->get_object = get_object;
  h->get_object_by_symbol = get_object_by_symbol;
  if (handler_main)
    h->name = StringDup (handler_main);
  else
    h->name = NULL;
  h->identifier = id;
  return h;
}

/*! Unlink (from the table ontaining the archive handlers) and free a specific
 * archive handler. */
void
ArchiveHandlerFree (const t_archive_handler * ah)
{
  if (ah->name)
    Free (ah->name);
  if (ah->prev_handler)
    ah->prev_handler->next_handler = ah->next_handler;
  else
    archive_table = ah->next_handler;

  if (ah->next_handler)
    ah->next_handler->prev_handler = ah->prev_handler;

  Free (ah);
}

/*! Free the table containing the archive handlers. Called by ArchivesFini. */

static void
ArchiveTableFree ()
{
  while (archive_table)
    ArchiveHandlerFree (archive_table);
}

/*! Constructor of an archive with a filename. Should not be used outside this
 * file (hence static), as the opened archive is not cached. You should use
 * ArchiveOpenCached instead.
 *
 * \param name The name of the archive to open
 */

static t_archive *
ArchiveOpen (t_const_string name)
{
  FILE *fp;
  t_archive *ret;
  t_string full_name;
  t_archive_handler *ht = archive_table;

  ASSERT(diabloobject_options.libpath, ("Lib path not set!"));

  full_name = FileFind (diabloobject_options.libpath, name);
  
  if (!full_name)
    return NULL;

  fp = fopen (full_name, "rb");
  ASSERT(fp, ("Could not open: %s", full_name));
  ret = ArchiveNew ();
  ret->name = full_name;

  while (ht)
  {
    if (ht->ident (fp))
    {
      ht->read (fp, ret);
      ret->handler = ht;
      return ret;
    }
    ht = ht->next_handler;
  }

  return NULL;
}

/*! Cached constructor of an archive with a filename. Opens the named archive
 * if it has not been opened yet and reads/stores the indexes (object file and
 * symbol index). If the archive already has been opened, simply return the
 * cached version. 
 * 
 * \param name The name of the archive to open
 */

t_archive *
ArchiveOpenCached (t_const_string name)
{
  t_archive_hash_table_node *hn =
    (t_archive_hash_table_node *) HashTableLookup (archives_global_hash_table,
                                                   name);

  /* First look in the archive hash table, to see if we already opened it */

  /* If not: Open */
  if (!hn)
  {
    t_archive *ar = ArchiveOpen (name);

    if (!ar) return NULL;

    hn = Malloc (sizeof (t_archive_hash_table_node));
    HASH_TABLE_NODE_SET_KEY(&hn->node, StringDup (name));
    hn->arch = ar;
    HashTableInsert (archives_global_hash_table, (t_hash_table_node *) hn);
    return ar;
  }
  /* Else: simply return archive */
  else
  {
    return hn->arch;
  }
}

/*! Initializes the global datastructures used for archives. Called by
 * DiabloObjectInit. Do not call this function directly, use DiabloObjectInit
 * instead. */

void
ArchivesInit ()
{
  archives_global_hash_table =
    HashTableNew (7, 0, (t_hash_func) StringHash,
                  (t_hash_cmp) StringCmp, ArchiveHashTableNodeFree);
}

/*! Frees the global datastructures used for archives. Called by
 * DiabloObjectFini. Do not call this function directly, use DiabloObjectFini
 * instead. */ 

void
ArchivesFini ()
{
  if (archives_global_hash_table != NULL)
  {
    HashTableFree (archives_global_hash_table);
  }
  ArchiveTableFree ();
}

/*! Get an object from an archive by name. In typical diablo scenarios, you
 * will never need to use this function. Instead you can use ObjectGet, and
 * provide archive_name:object_name as the name of the object to get. This has
 * the added benefit that the loaded object file will be cached for later use.  
 *
 * \param ar the archive from which the object will be fetched 
 * 
 * \param name the name of the object to fetch 
 *
 * \param parent the parent object of the loaded object. Can be NULL in case
 * you want to load a parent object 
 *
 * \param read_debug TRUE or FALSE depending on whether you want to load debug
 * information or not */

t_object *
ArchiveGetObject (const t_archive * ar, t_const_string name, t_object * parent, t_bool read_debug)
{
  return ar->handler->get_object (ar, name, parent, read_debug);
}

t_object *
ArchiveGetObjectBySymbol (const t_archive * ar, t_const_string symbol_name, t_object * parent, t_bool read_debug)
{
  if (ar->handler->get_object_by_symbol)
    return ar->handler->get_object_by_symbol(ar, symbol_name, parent, read_debug);
  return NULL;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
