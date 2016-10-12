/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define _(x) x

t_map_handler *map_table = NULL;
t_compressed_map_handler *compressed_map_table = NULL;

t_map_handler *
MapHandlerGetByName (t_const_string name)
{
  t_map_handler *ht = map_table;

  while (ht)
  {
    if (!strcmp (ht->name, name))
    {
      return ht;
    }
    ht = ht->next_handler;
  }

  FATAL(("Could not read map file (no map handler for file)!"));
}


void MapReadCompressed(t_map * ret, const t_object * obj)
{
  t_compressed_map_handler *ht = compressed_map_table;

  while (ht)
  {
    if (ht->ident (obj))
    {
      ht->read (obj, ret);
      /* No need to set ret->handler: the compressed map reader will set it to
       * the handler of the original (uncompressed) map file */
      return;
    }
    ht = ht->next_handler;
  }

  FATAL(("Could not read compressed map file (no map handler for file).\nThis typically means that the frontend you are using is not initialized to handle objects of the type of architecture/development environment you are trying to process!"));
}

void
MapReadF(t_map * ret, FILE * fp)
{
  t_map_handler *ht = map_table;

  while (ht)
  {
    if (ht->ident (fp))
    {
      ht->read (fp, ret);
      ret->handler = ht;
      return;
    }
    ht = ht->next_handler;
  }

  FATAL(("Could not read map file (no map handler for file).\nThis typically means that the frontend you are using is not initialized to handle objects of the type of architecture/development environment you are trying to process!"));
}

/* Returns the default name of the linker map of the specified object. 
 * In most cases, the map name is <object name>.map, although other formats 
 * are possible (e.g. PECOFF) 
 */
t_string
MapNameF(const t_object* obj)
{
  t_object_handler* ht = OBJECT_OBJECT_HANDLER(obj);

  if (!ht)
    FATAL(("Cannot get the map file name for an unidentified object type!"));

  if (ht->getmapname)
    return ht->getmapname(obj);
  
  return StringConcat2(OBJECT_NAME(obj), ".map");
}

t_compressed_map_handler *
CompressedMapHandlerAdd (t_const_string handler_main, t_bool (*ident) (const t_object *),
               void (*read) (const t_object *, t_map *))
{
  static t_uint32 id = 0;
  t_compressed_map_handler *h = Malloc (sizeof (t_compressed_map_handler));

  id++;

  if (compressed_map_table)
    compressed_map_table->prev_handler = h;

  h->prev_handler = NULL;
  h->next_handler = compressed_map_table;

  compressed_map_table = h;

  h->ident = ident;
  h->read = read;
  if (handler_main)
    h->name = StringDup (handler_main);
  else
    h->name = NULL;
  h->identifier = id;
  return h;
}

t_map_handler *
MapHandlerAdd (t_const_string handler_main, t_bool (*ident) (FILE *),
               void (*read) (FILE *, t_map *))
{
  static t_uint32 id = 0;
  t_map_handler *h = Malloc (sizeof (t_map_handler));

  id++;

  if (map_table)
    map_table->prev_handler = h;

  h->prev_handler = NULL;
  h->next_handler = map_table;

  map_table = h;

  h->ident = ident;
  h->read = read;
  if (handler_main)
    h->name = StringDup (handler_main);
  else
    h->name = NULL;
  h->identifier = id;
  return h;
}

void
MapHandlerFree (const t_map_handler * mh)
{
  if (mh->name)
    Free (mh->name);
  if (mh->prev_handler)
    mh->prev_handler->next_handler = mh->next_handler;
  else
    map_table = mh->next_handler;

  if (mh->next_handler)
    mh->next_handler->prev_handler = mh->prev_handler;

  Free (mh);
}

void
CompressedMapHandlerFree (const t_compressed_map_handler * mh)
{
  if (mh->name)
    Free (mh->name);
  if (mh->prev_handler)
    mh->prev_handler->next_handler = mh->next_handler;
  else
    compressed_map_table = mh->next_handler;

  if (mh->next_handler)
    mh->next_handler->prev_handler = mh->prev_handler;

  Free (mh);
}

void
MapTableFree ()
{
  while (map_table)
    MapHandlerFree (map_table);
}

void
CompressedMapTableFree ()
{
  while (compressed_map_table)
    CompressedMapHandlerFree (compressed_map_table);
}

/*!
 * \param node The node to free
 *
 * releases allocated memory*/

void
MapNodeFree (const t_map_node * node)
{
  t_map_node *tmp;

  tmp = (t_map_node *) node;
  Free (tmp->sec_name);
  if (tmp->mapped_sec_name)
    Free (tmp->mapped_sec_name);
  Free (tmp->object);
  Free (tmp);
}

/*!
 * \param void_node The node to print
 *
 *
 * prints out a section from the map */

void
MapNodePrint (const void *void_node)
{
  const t_map_node *node = (const t_map_node *) void_node;
  char type[5], attr[3];

  if (node == NULL)
    return;
  switch (node->type)
  {
    case Code:
      strcpy (type, "Code");
      break;
    case Data:
      strcpy (type, "Data");
      break;
    case Dbug:
      strcpy (type, "Dbug");
      break;
    case Zero:
      strcpy (type, "Zero");
      break;
    case TypeOther:
      strcpy (type, "Othe");
      break;
    default:
      WARNING(("Illegal type-field in map-tree"));
  }
  switch (node->attr)
  {
    case RO:
      strcpy (attr, "RO");
      break;
    case RW:
      strcpy (attr, "RW");
      break;
    case OT:
      strcpy (attr, "OT");
      break;
    default:
      WARNING(("Illegal attr-field in map-tree"));
  }
  VERBOSE(0, ("@G @G %s %s %s %s\n", node->base_addr, node->sec_size,
              type, attr, node->sec_name, node->object));
}

/*!
 *
 * \param void_node The node to verify
 *
 * verifies if the information in the node and the loaded objectfile matches */
static void
MapNodeMoveToParent (const void *void_node, const t_object * obj)
{
  const t_map_node *node = void_node;

  if (node->obj)
  {
    t_section *sec;

    if (!node->has_sec_idx)
      sec = SectionGetFromObjectByName (node->obj, node->sec_name);
    else
      sec = SectionGetFromObjectByIndex (node->obj, node->sec_idx);

    if (strcmp (node->sec_name, ".IA_64.unwind")
        && strcmp (node->sec_name, ".ARM.attributes")
        && strcmp (node->sec_name, ".note.spu_name")
        && strcmp (node->sec_name, ".common")
        && strcmp (node->sec_name, ".scommon")
        && strcmp (node->sec_name, "COMMON"))
    {
      if (!sec) 
      {
        if ((!strcmp (node->sec_name, ".sbss") /* ppc puts common symbols in the sbss */
             || !strcmp(node->sec_name, ".eh_frame")
             || !strcmp(node->sec_name, ".eh_frame_hdr"))
             ||  (StringPatternMatch("*crti.o",node->object))
             ||  (StringPatternMatch("*crt1.o",node->object))
             ||  (StringPatternMatch("*crtbegin_dynamic.o",node->object)) /* for android */
             ||  (StringPatternMatch("*crtbegin_so.o",node->object)) /* for android shared objects */
             ||  (StringPatternMatch("*crtbegin_static.o",node->object))
             ||  (StringPatternMatch("*crtend_so.o",node->object))
             ||  (StringPatternMatch("*abi-note.o",node->object)))
          return;
        else
          FATAL(("Section %s:%s is in map, but not in file! This could be caused by a malfunctioning compiler toolchain (e.g., not using a Diablo-compatible toolchain).",
               node->object, node->sec_name));
      }

      if (!(AddressIsEq (SECTION_OLD_SIZE(sec), node->sec_size)))
      {
        WARNING(("Section %s:%s section size differs in map: MAP says @G bytes, section in object file is @G bytes",
                 node->object, node->sec_name, node->sec_size, SECTION_OLD_SIZE(sec)));
        if (StringPatternMatch(".text*",SECTION_NAME(sec)))
          {
            VERBOSE(0,("When that happens for .text sections, it is very likely that the file %s retrieved now by Diablo\nis not the file that was linked into the binary/library you are trying to rewrite now, so aborting ...",node->object));
            exit(-1);
          }
        SECTION_SET_OLD_SIZE(sec, node->sec_size);

        if (diabloobject_options.keep_exidx)
          if (StringPatternMatch(".ARM.exidx*",node->sec_name))
            SECTION_SET_FLAGS(sec, SECTION_FLAGS(sec) | SECTION_FLAG_EXIDX_NEEDS_RELAXATION);
        
      }

      if (!SECTION_IS_MAPPED(sec))
      {
        t_section * parent;
        ASSERT(node->mapped_sec_name, ("Parent section name not set for map node %s %s", node->object, node->sec_name));
        parent = SectionGetFromObjectByName (obj, node-> mapped_sec_name);
        ASSERT(parent, ("Could not find parent section %s for section %s %s", node-> mapped_sec_name, node->object, node->sec_name));
        MAP_SECTION(sec, node->mapped_sec_name, parent);
        SECTION_SET_OLD_ADDRESS(sec, node->base_addr);
        SECTION_SET_CADDRESS(sec, node->base_addr);
      }
    }
  }
}

static void
MapNodeLoad (const void *node, const t_objects_ll * objll, t_bool read_debug)
{
  /* TODO: Check type.... So, we need objecttype.... */

  if (((t_map_node *)node)->builtin)
  {
    ((t_map_node *) node)->obj =
      ObjectGetBuiltin (((t_map_node *) node)->object, objll->parent, read_debug);
  }
  else if (strcmp (((t_map_node *) node)->object, "anon$$obj.o") == 0)
  {
  }
  else if (strcmp (((t_map_node *) node)->object, "(common)") == 0)
  {
  }
  else if (strcmp (((t_map_node *) node)->object, "(scommon)") == 0)
  {
  }
  else if (strcmp (((t_map_node *) node)->object, "Linker") == 0)
  {
  }
  else if (strcmp (((t_map_node *) node)->object, "exception_gp_info") == 0)
  {
  }
  else
  {
    ((t_map_node *) node)->obj =
      ObjectGet (((t_map_node *) node)->object, objll->parent, read_debug, ((t_map_node *) node)->sec_name);
  }
}

/*! Map Constructor */
t_map *
MapNew (void)
{
  t_map *ret = (t_map *) Calloc (1, sizeof (t_map));

  T_LIST(ret)->type = DLIST;
  return ret;
}

void
MapFree (const t_map * to_free)
{
  t_map * map = (t_map *) to_free;
  t_map_node *tmp;

  while (map->list.first)
  {
    tmp = map->list.first;
    map->list.first = ((t_map_node *) map->list.first)->node.next;
    MapNodeFree (tmp);
  }
  Free (map);
}

void
MapPrint (const t_map * map)
{
  FATAL(("Implement")); /*MapNodePrint; */
}

void
MapMoveObjectsToParent (const t_map * map, t_object * obj)
{
  t_map_node *node, *tmp;

  _(DLIST_FOREACH_NODE(map, node, tmp)
  {
    MapNodeMoveToParent (node, obj);
  })
}

t_objects_ll
MapLoadObjects (const t_map * map, t_object * parent_object, t_bool read_debug)
{
  t_map_node *node, *tmp;
  t_objects_ll pass = { NULL, NULL, parent_object };
  _(DLIST_FOREACH_NODE(map, node, tmp)
  {
    MapNodeLoad (node, &pass, read_debug);
  })
  return pass;
}

/*! inserts a node in the map-tree */
void
MapInsertNode (t_map * map, t_map_node * node)
{
  ListAppend (map, node);
}

/*! returns the node in which the input-adress is contained.*/
t_map_node *
MapGetSection (const t_map * map, t_address address)
{
  t_map_node *node, *tmp = NULL;

  _(DLIST_FOREACH_NODE(map, node, tmp)
  {
    if (AddressIsLt (address, node->base_addr))
      continue;
    if (AddressIsGe (address, AddressAdd (node->base_addr, node->sec_size)))
      continue;
    break;
  })
  return node;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
