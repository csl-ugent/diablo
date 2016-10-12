/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <diabloobject.h>

/* For documentation see diabloobject_object.class.h */

t_object *
ObjectNew ()
{
  t_object *ret;

  ret = Calloc (1, sizeof (t_object));
  DiabloBrokerCall("AfterObjectNew", ret);
  return ret;
}

void
ObjectWriteF(t_object * in, FILE * fp)
{
  t_section * sec;
  t_uint32 i;
  if (!in)
    FATAL(("No handler set for object %s\n", OBJECT_NAME(in)));


  OBJECT_FOREACH_SECTION(in, sec, i)
  {
    switch (SECTION_TYPE(sec))
    {
      case DISASSEMBLING_CODE_SECTION:
        FATAL(("Trying to write an object that is being disassembled"));
      case DISASSEMBLED_CODE_SECTION:
        FATAL(("Trying to write an object that is not assembled"));
      case FLOWGRAPHING_CODE_SECTION:
        FATAL(("Trying to write an object that is being flowgraphed"));
      case FLOWGRAPHED_CODE_SECTION:
        FATAL(("Trying to write an object that is not deflowgraphed"));
      case DEFLOWGRAPHING_CODE_SECTION:
        FATAL(("Trying to write an object that is not fully deflowgraphed"));
      case ASSEMBLING_CODE_SECTION:
        FATAL(("Trying to write an object that is being assembled"));
        break;
      default:
        break;
    }
  }

  if (OBJECT_OBJECT_HANDLER(in)->write)
    OBJECT_OBJECT_HANDLER(in)->write (fp, in);
  else
    FATAL(("Handler for object %s has no write support", OBJECT_NAME(in)));
}

t_object_handler *object_table = NULL;
t_builtin_object *builtin_object_table = NULL;

void
ObjectReadF(FILE * fp, t_object * ret, t_bool read_debug)
{
  t_object_handler *ht = object_table;
  int tel = 0;
  unsigned char buffer[10];

  OBJECT_SET_STREAMPOS(ret, ftell (fp));
  VERBOSE(1, ("Reading file %s", OBJECT_NAME(ret)));

  ASSERT(ht, ("No object handlers installed! Cannot read object..."));

  /* Try to identify the object */
  while (ht != NULL)
  {
    if (!ht->sub_name)
      VERBOSE(2, ("Trying %s", ht->main_name));
    else
      VERBOSE(2, ("Trying %s-%s", ht->main_name, ht->sub_name));
    if (ht->ident (fp))
    {
      if (ht->htype != GROUPHANDLER)
      {
        /* If we have a match use the corresponding load operation */
        ht->read (fp, ret, read_debug);
        OBJECT_SET_OBJECT_HANDLER(ret, ht);

        /* try to find out if the code was handwritten in assembler or created by a compiler */
        {
          t_bool handwritten = TRUE;
          t_symbol *sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(ret));
          t_uint32 i;


          while (sym && (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FILE)))
            sym = SYMBOL_NEXT(sym);

          /* file symbol is first absolute symbol */
          if (sym && SYMBOL_NAME(sym))
          {
            char *ext = strrchr (SYMBOL_NAME(sym), '.');

            if (ext)
            {
              ext++;
              if (!strcmp (ext, "c") || !strcmp (ext, "cc")
                  || !strcmp (ext, "h") || !strcmp (ext, "C")
                  || !strcmp (ext, "cpp") || !strcmp (ext, "hpp")
		  || !strcmp (ext, "i")
		  )
                handwritten = FALSE;
            }
          }

          for (i = 0; i < OBJECT_NCODES(ret); i++)
          {
            if (handwritten)
              SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$handwritten", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_CODE(ret)[i]), 
                                    AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
            else
              SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$compiler", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_CODE(ret)[i]), 
                                    AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
          }
          for (i = 0; i < OBJECT_NRODATAS(ret); i++)
          {
            if (handwritten)
              SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$handwritten", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_RODATA(ret)[i]), 
                                    AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
            else
              SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$compiler", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_RODATA(ret)[i]), 
                                    AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
          }
        }

        DiabloBrokerCall ("AfterObjectRead", ret);

        return;
      }
      else
      {
        ht = ht->next_handler;
        continue;
      }
    }
    if (ht->htype != GROUPHANDLER)
      ht = ht->next_handler;
    else
      ht = ht->next_group;
  }

  /* Debug: show the first 10 bytes of the program (most likely the bytes used
   * to identify this binary */

  tel = fread (buffer, 1, 10, fp);
  if (tel < 10)
    FATAL(("Could not get 10 bytes, so either the object is truncated or this is not an object file"));

  printf ("Dump of first 10 bytes\n");
  printf ("Hex:\n");
  for (tel = 0; tel < 10; tel++)
  {
    printf ("0x%x ", buffer[tel]);
  }
  printf ("\n");

  printf ("Octal:\n");
  for (tel = 0; tel < 10; tel++)
  {
    printf ("0%o ", buffer[tel]);
  }
  printf ("\n");

  printf ("Decimal:\n");
  for (tel = 0; tel < 10; tel++)
  {
    printf ("%d ", buffer[tel]);
  }
  printf ("\n");

  FATAL(("Unknown object filetype %s!", OBJECT_NAME(ret)));
}

/*!
 *
 * \param name
 * \param fp
 *
 * \return t_object *
 */

t_object *
ObjectGetFromStream (t_const_string name, FILE * fp, t_object * parent, t_bool read_debug)
{
  t_object *ret = ObjectNewCached (name, parent);

  ObjectReadF(fp, ret, read_debug);
  return ret;
}

void
ObjectWrite (t_object * obj, t_const_string name)
{
  FILE *fp = fopen (name, "wb");

  ObjectWriteF(obj, fp);
}

t_object *
ObjectRead (t_const_string name, t_object * parent, t_bool read_debug)
{
  FILE *fp;
  t_object *ret;

  /* THIS WAS SO WRONG */
  fp = fopen (name, "rb");
  ASSERT(fp, ("Could not open: %s", name));
  ret = ObjectNewCached (name, parent);
  fseek (fp, 0, SEEK_END);
  OBJECT_SET_SIZE(ret, ftell (fp));
  fseek (fp, 0, SEEK_SET);
  ObjectReadF(fp, ret, read_debug);
  fclose (fp);
  return ret;
}

/*!
 *
 * \param real_name
 * \param fake_name
 *
 * \return t_object *
 */

t_object *
ObjectReadWithFakeName (t_const_string real_name, t_const_string fake_name, t_object * parent, t_bool read_debug)
{
  t_const_string full_real_name =
    FileFind (diabloobject_options.objpath, real_name);
  FILE *fp;
  t_object *ret;

  ASSERT(full_real_name, ("File %s not found in objectpath", real_name));
  fp = fopen (full_real_name, "rb");
  ASSERT(fp, ("Could not open: %s", full_real_name));
  ret = ObjectNewCached (fake_name, parent);
  fseek (fp, 0, SEEK_END);
  OBJECT_SET_SIZE(ret, ftell (fp));
  fseek (fp, 0, SEEK_SET);
  ObjectReadF(fp, ret, read_debug);
  fclose (fp);
  return ret;
}

extern t_hash_table *global_object_hash;

t_object *
ObjectGet (t_const_string name, t_object * parent, t_bool read_debug, t_const_string hint)
{
  t_hash_table *cache;
  t_object_hash_node *hn;

  /* if we're loading a sub object, use the parent object's cache, otherwise
   * use the global cache */
  cache = parent ? OBJECT_SUBOBJECT_CACHE(parent) : global_object_hash;

  /* First look in the object hash table, to see if we already opened it */
  hn = (t_object_hash_node *) HashTableLookup (cache, name);
  /* If not: Open */
  if (!hn)
  {
    t_object *obj;

    /* Check if it is from an archive */
    /* Dirty hack here for windows absolute paths support --svolckae */
    if (StringPatternMatch ("??*:*", name))
    {
      t_archive *ar;
      t_string copy = StringDup (name);
      t_string libn = strtok (copy, ":");
      t_string objn = strtok (NULL, ":");

      ASSERT(libn, ("Illegal objectname %s\n", name));
      ASSERT(objn, ("Illegal objectname %s\n", name));
      ASSERT(!strtok (NULL, ":"), ("Illegal objectname %s\n", name));

      if (strcmp(libn, "@") == 0)
      {
        libn = OBJECT_NAME(parent); 
      }
      
      ar = ArchiveOpenCached (libn);

      ASSERT(ar != NULL, ("Library %s not found in libpath (or library file format not recognized)\n", libn));
      obj = ArchiveGetObject (ar, objn, parent, read_debug);
      ASSERT(obj != NULL, ("Object %s not found in %s\n", objn, libn));
      Free (copy);
    }
    /* Try to read a normal file */
    else
    {
      t_string full_name = FileFind (diabloobject_options.objpath, name);      

      if (!full_name)
      {
        FATAL(("Object %s not found\n", name));
        exit (0); /* Keep the compiler happy */
      }
      else
      {
        /* Maybe it was in the hash table under the full name? */
        hn = (t_object_hash_node *) HashTableLookup (cache, full_name);

        /* If so, add a fake hash entry for the alternative name and return... */
        if (hn)
        {
          t_object_hash_node *hn2 = Malloc (sizeof (t_object_hash_node));
          OBJECT_HASH_NODE_SET_KEY(hn2, StringDup (name));
          OBJECT_HASH_NODE_SET_OBJ(hn, OBJECT_HASH_NODE_OBJ(hn));
          OBJECT_HASH_NODE_SET_FAKE_HASH_ENTRY(hn, TRUE);
          HashTableInsert (cache, (t_hash_table_node *) hn);

          return OBJECT_HASH_NODE_OBJ(hn);
        }
        else
        {
          obj = ObjectRead (full_name, parent, read_debug);
          ASSERT(obj != NULL, ("Could not read object %s\n", name));
        }
      }
      Free(full_name);
    }

    /* Put it in the object cache under name */
    if (strcmp (OBJECT_NAME(obj), name) != 0)
    {
      hn = Malloc (sizeof (t_object_hash_node));
      OBJECT_HASH_NODE_SET_KEY(hn, StringDup(name));
      OBJECT_HASH_NODE_SET_OBJ(hn, obj);
      OBJECT_HASH_NODE_SET_FAKE_HASH_ENTRY(hn, TRUE);
      HashTableInsert (cache, (t_hash_table_node *) hn);
    }

    if (parent)
      ObjectAppendSubObject (parent, obj);

    return obj;
  }

  /* Else return the cached object */
  return OBJECT_HASH_NODE_OBJ(hn);
}

t_object *
ObjectGetBuiltin (t_const_string name, t_object * parent, t_bool read_debug)
{
  t_object_handler *handler;
  t_builtin_object *b;
  char oname[200];

  ASSERT(parent, ("Built-in object cannot be parent object"));
  handler = OBJECT_OBJECT_HANDLER(parent);

  for (b = builtin_object_table; b; b = b->next)
  {
    if (!strcmp (b->fileformat, handler->main_name) &&
        !strcmp (b->arch, handler->sub_name) &&
        !strcmp (b->id, name))
    {
      sprintf(oname, "%s/builtin/%s", DATADIR, b->object_name);
      break;
    }
  }
  ASSERT(b, ("Built-in object %s not found", name));

  return ObjectGet (oname, parent, read_debug, NULL);
}

t_object *
ObjectGetFromCache (t_const_string name, const t_object * parent)
{
  t_object_hash_node *hn;
  t_hash_table *cache = parent ? OBJECT_SUBOBJECT_CACHE(parent) : global_object_hash;

  hn = (t_object_hash_node *) HashTableLookup (cache, name);
  if (!hn)
    return NULL;
  return OBJECT_HASH_NODE_OBJ(hn);
}

t_object *
ObjectNewCached (t_const_string name, t_object * parent)
{
  t_object_hash_node *hn;
  t_hash_table *cache = parent ? OBJECT_SUBOBJECT_CACHE(parent) : global_object_hash;

  hn = Malloc (sizeof (t_object_hash_node));
  OBJECT_HASH_NODE_SET_KEY(hn, StringDup (name));
  OBJECT_HASH_NODE_SET_OBJ(hn, ObjectNew ());
  OBJECT_SET_NAME(OBJECT_HASH_NODE_OBJ(hn), StringDup (name));
  OBJECT_SET_PARENT(OBJECT_HASH_NODE_OBJ(hn), parent);
  if (parent)
    OBJECT_SET_CODE_SYMBOL_ADDRESS_INVERSE_MASK(OBJECT_HASH_NODE_OBJ(hn), OBJECT_CODE_SYMBOL_ADDRESS_INVERSE_MASK(parent));
  OBJECT_HASH_NODE_SET_FAKE_HASH_ENTRY(hn, FALSE);
  
  HashTableInsert (cache, (t_hash_table_node *) hn);

  OBJECT_SET_SECTION_TABLE(OBJECT_HASH_NODE_OBJ(hn), HashTableNew(101,ClassOffsetOf(t_section,parent_t_hash_table_node),
                                             (t_hash_func)StringHash, (t_hash_cmp)strcmp,
                                             SectionHashTableNodeFree)); 

  if (parent)
    OBJECT_SET_ADDRESS_SIZE (OBJECT_HASH_NODE_OBJ(hn), OBJECT_ADDRESS_SIZE(parent));

  return OBJECT_HASH_NODE_OBJ(hn);
}

void
ObjectFree (const t_object * to_free)
{
  DiabloBrokerCall ("BeforeObjectFree", to_free);
 
  if (OBJECT_RELOC_TABLE(to_free))
    RelocTableFree (OBJECT_RELOC_TABLE(to_free));
  
  /* if the object is a parent object, free it's associated cache, and the
   * undef and abs section associated with it */
  if (OBJECT_SUBOBJECT_CACHE(to_free))
  {
    HashTableFree (OBJECT_SUBOBJECT_CACHE(to_free));
  }
 
  if (OBJECT_ORIG_CODES(to_free))
  {
    t_uint32 i;

    for (i = 0; i < OBJECT_N_ORIG_CODES(to_free); i++)
      Free (OBJECT_ORIG_CODES(to_free)[i].name);
    Free (OBJECT_ORIG_CODES(to_free));
  }

  if (OBJECT_LAYOUT_SCRIPT(to_free))
    LayoutScriptFree (OBJECT_LAYOUT_SCRIPT(to_free));

  if (OBJECT_MAP(to_free))
    MapFree (OBJECT_MAP(to_free));
  if (OBJECT_SYMBOL_TABLE(to_free))
    SymbolTableFree (OBJECT_SYMBOL_TABLE(to_free));
  if (OBJECT_SUB_SYMBOL_TABLE(to_free))
    SymbolTableFree (OBJECT_SUB_SYMBOL_TABLE(to_free));
  if (OBJECT_DYNAMIC_SYMBOL_TABLE(to_free))
    SymbolTableFree (OBJECT_DYNAMIC_SYMBOL_TABLE(to_free));

  if (OBJECT_CODE(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NCODES(to_free); tel++)
      SectionFree (OBJECT_CODE(to_free)[tel]);
    Free (OBJECT_CODE(to_free));
  }
  if (OBJECT_RODATA(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NRODATAS(to_free); tel++)
      SectionFree (OBJECT_RODATA(to_free)[tel]);
    Free (OBJECT_RODATA(to_free));
  }
  if (OBJECT_DATA(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NDATAS(to_free); tel++)
      SectionFree (OBJECT_DATA(to_free)[tel]);
    Free (OBJECT_DATA(to_free));
  }
  if (OBJECT_TLSDATA(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NTLSDATAS(to_free); tel++)
      SectionFree (OBJECT_TLSDATA(to_free)[tel]);
    Free (OBJECT_TLSDATA(to_free));
  }
  if (OBJECT_NOTE(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NNOTES(to_free); tel++)
      SectionFree (OBJECT_NOTE(to_free)[tel]);
    Free (OBJECT_NOTE(to_free));
  }
  if (OBJECT_BSS(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NBSSS(to_free); tel++)
      SectionFree (OBJECT_BSS(to_free)[tel]);
    Free (OBJECT_BSS(to_free));
  }
  if (OBJECT_TLSBSS(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NTLSBSSS(to_free); tel++)
      SectionFree (OBJECT_TLSBSS(to_free)[tel]);
    Free (OBJECT_TLSBSS(to_free));
  }
  if (OBJECT_DEBUG(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NDEBUGS(to_free); tel++)
      SectionFree (OBJECT_DEBUG(to_free)[tel]);
    Free (OBJECT_DEBUG(to_free));
  }
  if (OBJECT_ATTRIB(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NATTRIBS(to_free); tel++)
      SectionFree (OBJECT_ATTRIB(to_free)[tel]);
    Free (OBJECT_ATTRIB(to_free));
  }
  
  if (OBJECT_ABS_SECTION(to_free))
    SectionFree(OBJECT_ABS_SECTION(to_free));
  
  if (OBJECT_UNDEF_SECTION(to_free))
    SectionFree(OBJECT_UNDEF_SECTION(to_free));

  if (OBJECT_SECTION_TABLE(to_free))
    HashTableFree (OBJECT_SECTION_TABLE(to_free));
  
  t_segment *seg = OBJECT_SEGMENT_FIRST(to_free);
  while (seg)
  {
    t_segment *tmp_seg = SEGMENT_SEGMENT_NEXT(seg);

    Free (SEGMENT_NAME(seg));
    Free (seg);
    seg = tmp_seg;
  }

  if (OBJECT_COMDAT_SECTION_GROUPS(to_free))
  {
    t_uint32 tel;

    for (tel = 0; tel < OBJECT_NCOMDAT_SECTION_GROUPS(to_free); tel++)
      SectionGroupFree (OBJECT_COMDAT_SECTION_GROUPS(to_free)[tel]);
    Free(OBJECT_COMDAT_SECTION_GROUPS(to_free));
  }

  if (OBJECT_NAME(to_free))
    Free (OBJECT_NAME(to_free));

  Free (to_free);
}

/* ObjectAddSectionFromFile
 * The object, char type (C,D,or B), a bool to say if it's read only, FILE * fp, filepos fileoffset, t_address address, t_address size
 * nothing
 * Add's a section from an objectfile.*/

t_section *
ObjectAddSectionFromFile (t_object * obj, char type, t_bool ro, FILE * fp,
                          t_uint32 off, t_address address, t_address size,
                          t_address align, t_const_string name, t_int32 secnr)
{
  t_uint32 stored = ftell (fp);

  /* Just some asserts */
  switch (type)
  {
    case CODE_SECTION:
      ASSERT(ro, ("Non constant code...."));
      break;
    case DATA_SECTION:
    case RODATA_SECTION:
      if (ro)/* These can be switched around */
        type = RODATA_SECTION;
      else
        type = DATA_SECTION;
      break;
    case TLSDATA_SECTION:
      ASSERT(!ro, ("Constant tls data..."));
      break;
    case TLSBSS_SECTION:
      ASSERT(!ro, ("Constant tls bss..."));
      break;
    case BSS_SECTION:
      ASSERT(!ro, ("Constant bss..."));
      break;
  }
  t_section* new_section = SectionNew (obj, type, name);
  SectionInsertInObject(new_section, obj);

  /* Set the size of the new section */
  SECTION_SET_OLD_SIZE(new_section, size);
  SECTION_SET_CSIZE(new_section, size);
  /* Set the start address */
  SECTION_SET_OLD_ADDRESS(new_section, address);
  SECTION_SET_CADDRESS(new_section, address);
  SECTION_SET_ALIGNMENT(new_section, align);
  /* set the section index (used by some map parsers, in case
   * duplicate section names are possible)
   */
  SECTION_SET_INDEX_IN_OBJ(new_section,secnr);

  if ((!AddressIsNull (size)) && (type != BSS_SECTION))
  {
    SECTION_SET_DATA(new_section, Malloc (AddressExtractUint32 (size)));
  }
  else
    SECTION_SET_DATA(new_section, NULL);

  if (SECTION_DATA(new_section))
  {
    fseek (fp, off, SEEK_SET);
    IGNORE_RESULT(fread (SECTION_DATA(new_section), AddressExtractUint32 (size), 1, fp));
  }

  fseek (fp, stored, SEEK_SET);
  return new_section;
}

t_uint64
ObjectGetData64 (const t_object * obj, t_const_string name, t_address address)
{
  t_section *sec = SectionGetFromObjectByName (obj, name);
  t_address real_address;
  void *load;

  ASSERT(sec != NULL, ("Could not get section %s", name));
  real_address = AddressSub (address, SECTION_CADDRESS(sec));
  ASSERT(AddressIsGt (SECTION_CSIZE(sec), real_address),
         ("Out of section load!"));
  load = AddressAddDispl (SECTION_DATA(sec), real_address);

  return (*((t_uint64 *) load));
}

t_uint32
ObjectGetData32 (const t_object * obj, t_const_string name, t_address address)
{
  t_section *sec = SectionGetFromObjectByName (obj, name);
  t_address real_address;
  void *load;

  ASSERT(sec != NULL, ("Could not get section %s", name));
  real_address = AddressSub (address, SECTION_CADDRESS(sec));
  ASSERT(AddressIsGt (SECTION_CSIZE(sec), real_address),
         ("Out of section load!"));
  load = AddressAddDispl (SECTION_DATA(sec), real_address);

  return (*((t_uint32 *) load));
}

t_uint32
ObjectHandlerAdd (t_const_string handler_main, t_const_string handler_sub,
                  t_bool (*ident) (FILE *), void (*read) (FILE *, t_object *, t_bool),
                  void (*write) (FILE *, t_object *),
                  t_uint64 (*sizeofheaders) (t_object *, const t_layout_script *),
                  t_uint64 (*linkbaseaddress) (const t_object *, const t_layout_script *),
                  t_uint64 (*alignstartofrelro) (t_object *, long long),
                  t_uint64 (*aligngotafterrelro) (t_object *, long long),
                  t_uint64 (*aligndataafterrelro) (t_object *, long long),
                  void * (*rawread) (FILE *),
                  void (*rawwrite) (void *, FILE * fp),
                  t_address (*rawaddrtofile) (void *, t_address),
                  t_address (*rawfiletoaddr) (void *, t_address),
                  char * (*rawaddsec)(void *, t_const_string, t_address),
                  char * (*rawaddrtosecname)(void *, t_address),
                  char * (*getmapname)(const t_object* obj))
{
  static t_uint32 id = 0;
  t_object_handler *h = Malloc (sizeof (t_object_handler));
  t_object_handler *ht = object_table;

  id++;

  if (!handler_sub)
  {
    h->next_group = object_table;
    if (object_table)
      object_table->prev_group = h;
    h->prev_group = h->next_handler = h->prev_handler = NULL;
    object_table = h;
    h->htype = GROUPHANDLER;
  }
  else
  {
    while (ht && StringCmp (ht->main_name, handler_main))
      ht = ht->next_group;
    ASSERT(ht, ("No group handler found for sub handler %s %s!", handler_main, handler_sub));
    h->next_group = h->prev_group = h->prev_handler = NULL;
    if (ht->next_handler)
      ht->next_handler->prev_handler = h;
    h->next_handler = ht->next_handler;
    ht->next_handler = h;
    h->htype = OBJECTHANDLER;
  }

  h->ident = ident;
  h->read = read;
  h->write = write;
  h->sizeofheaders = sizeofheaders;
  h->linkbaseaddress = linkbaseaddress;
  h->alignstartofrelro = alignstartofrelro;
  h->aligngotafterrelro = aligngotafterrelro;
  h->aligndataafterrelro = aligndataafterrelro;

  h->rawread = rawread;
  h->rawwrite = rawwrite;
  h->rawaddrtofile = rawaddrtofile;
  h->rawfiletoaddr = rawfiletoaddr;
  h->rawaddsec = rawaddsec;
  h->rawaddrtosecname = rawaddrtosecname;
  h->getmapname = getmapname;
  
  if (handler_main)
    h->main_name = StringDup (handler_main);
  else
    h->main_name = NULL;
  if (handler_sub)
    h->sub_name = StringDup (handler_sub);
  else
    h->sub_name = NULL;
  h->identifier = id;

  return id;
}

static void
ObjectHandlerFree (const t_object_handler * oh, t_object_handler * gh)
{
  if (oh->htype == GROUPHANDLER)
  {
    ASSERT(!gh, ("ObjectHandlerFree called on group handler with gh != NULL"));

    while(oh->next_handler) ObjectHandlerFree(oh->next_handler, (t_object_handler *) oh);
    if (oh->prev_group)
      oh->prev_group->next_group = oh->next_group;
    else
      object_table = oh->next_group;

    if (oh->next_group)
      oh->next_group->prev_group = oh->prev_group;
  }
  else
  {
    ASSERT(gh, ("ObjectHandlerFree called on group handler with gh == NULL"));

    if (oh->prev_handler)
      oh->prev_handler->next_handler = oh->next_handler;
    else
      gh->next_handler = oh->next_handler;

    if (oh->next_handler)
      oh->next_handler->prev_handler = oh->prev_handler;
  }
  if (oh->main_name)
    Free (oh->main_name);
  if (oh->sub_name)
    Free (oh->sub_name);
  Free (oh);
}

void
ObjectTableFree ()
{
  while (object_table)
    ObjectHandlerFree (object_table, NULL);
}

/* ================ OBJECT CACHE ================ */

/* DOC: Struct and definitions for object cache:
 * The objectcache is used to avoid reloading already
 * opened objectfiles */

t_hash_table *global_object_hash = NULL;

/*!
 * \param tf The element to free
 * \param data Unused data element (needed when walking a hash table)
 *
 * Utility function: used to free the objects in the cache by the t_hashtable structure.
 *
 * \return void
 */

void
ObjectHashElementFree (const void *tf, void *data)
{
  t_object_hash_node *node = (t_object_hash_node *) tf;

  if (!OBJECT_HASH_NODE_FAKE_HASH_ENTRY(node))
    ObjectFree (OBJECT_HASH_NODE_OBJ(node));
  Free (OBJECT_HASH_NODE_KEY(node));
  Free (node);
}
extern t_symbol *symbol_type_cached;

void
ObjectInit ()
{
  ASSERT(!global_object_hash, ("Reinitialization of Object!"));
  global_object_hash =
    HashTableNew (4003, 0, (t_hash_func) StringHash,
                  (t_hash_cmp) StringCmp,
                  ObjectHashElementFree);

}

/*! Static deinitialization for the t_object class */
void
ObjectFini ()
{
  ASSERT(global_object_hash,
         ("Deinitialization without initialization of Object!"));
  HashTableFree (global_object_hash);
  global_object_hash = NULL;

  ObjectTableFree ();
}

/* #define VERBOSE_MERGING */

/* Merge code sections */
void
ObjectMergeCodeSections (t_object * obj)
{
  t_uint32 tel;
  t_bool ok = TRUE;

  t_address start;
  t_address run, size;
  t_section *sec, *tmpsec;
  t_reloc *rel;
  t_symbol *sym;

  FATAL(("who still uses this?"));

#ifdef VERBOSE_MERGING
  /* {{{ */
  STATUS(START, ("Before merging"));
  {
    t_section *sec;
    t_uint32 tel;
    t_address totalsize = AddressNullForObject(obj);

    OBJECT_FOREACH_SECTION(obj, sec, tel)
    {
      VERBOSE(0, ("Section %s new address = @G, old = @G size = @G",
                  SECTION_NAME(sec), SECTION_CADDRESS(sec),
                  SECTION_OLD_ADDRESS(sec), SECTION_CSIZE(sec)));

      t_object *sub, *tmp;
      t_address size = AddressNullForObject(obj);

      OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp)
      {
        t_section *sec2;
        int tel2;

        OBJECT_FOREACH_SECTION(sub, sec2, tel2)
        {
          if (SECTION_PARENT_SECTION(sec2) == sec)
          {
            size = AddressAdd(size, SECTION_CSIZE(sec2));
            /*VERBOSE(0,("\t sub %s in %s old @G\n",sec2->name,OBJECT_NAME(sub),SECTION_OLD_ADDRESS(sec2))); */
          }
        }
      }
      VERBOSE(0, ("total size of subsecs is @G", size));
      if (SECTION_TYPE(sec) == CODE_SECTION)
        totalsize = AddressAdd (totalsize, SECTION_CSIZE(sec));
    }

    VERBOSE(0, ("Have %d code sections (total size of code sections @G)",
                OBJECT_NCODES(obj), totalsize));
  }
  STATUS(STOP, ("Before merging"));
  /* }}} */
#endif

  if (OBJECT_NCODES(obj) < 2)
  {
    /* just fill out the original code sections table and return */
    OBJECT_SET_N_ORIG_CODES(obj, 1);
    OBJECT_SET_ORIG_CODES(obj, Calloc (1, sizeof (t_section_descriptor)));
    OBJECT_ORIG_CODES(obj)->name = StringDup (SECTION_NAME(OBJECT_CODE(obj)[0]));
    OBJECT_ORIG_CODES(obj)->base = SECTION_CADDRESS(OBJECT_CODE(obj)[0]);
    OBJECT_ORIG_CODES(obj)->size = SECTION_CSIZE(OBJECT_CODE(obj)[0]);
    OBJECT_ORIG_CODES(obj)->obase = SECTION_OLD_ADDRESS(OBJECT_CODE(obj)[0]);
    OBJECT_ORIG_CODES(obj)->osize = SECTION_OLD_SIZE(OBJECT_CODE(obj)[0]);
    OBJECT_ORIG_CODES(obj)->type = CODE_SECTION;
    return;
  }

  /* first step: check if all code sections are laid out contiguously in memory */
  start = run = SECTION_CADDRESS(OBJECT_CODE(obj)[0]);
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
  {
#ifdef VERBOSE_MERGING
    VERBOSE(0, ("Section %s at @G @G", SECTION_NAME(sec),
                SECTION_CADDRESS(sec), SECTION_CSIZE(sec)));
#endif
    run = AddressAlign (SECTION_ALIGNMENT(sec), run);
    if (!AddressIsEq (run, SECTION_CADDRESS(sec)))
      ok = FALSE;
    run = AddressAdd (run, SECTION_CSIZE(sec));
  }
  size = AddressSub (run, start);

  /* rebuild sections and bring relocs and symbols and stuff up to date */
  if (!ok)
  {
    /* reorder the sections */
    VERBOSE(0, ("Code sections not laid out contiguously"));
    ObjectOrderCodeSectionsContiguously (obj);
    /* recalculate the relocations */
    OBJECT_FOREACH_RELOC(obj, rel)
    {
      t_address ret = StackExec (RELOC_CODE(rel), rel, NULL, SECTION_DATA(T_SECTION(RELOC_FROM(rel))), TRUE, 0, obj);

      ASSERT(AddressIsNull (ret), ("Could not execute @R(code %s)", rel, RELOC_CODE(rel)));
    }

    /* write the changes back into the data of the parent sections */
    ObjectRebuildSectionsFromSubsections (obj);

    ok=TRUE;
    start = run = SECTION_CADDRESS(OBJECT_CODE(obj)[0]);
    OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
    {
#ifdef VERBOSE_MERGING
	    VERBOSE(0, ("Section %s at @G @G", SECTION_NAME(sec),
				    SECTION_CADDRESS(sec), SECTION_CSIZE(sec)));
#endif
	    run = AddressAlign (SECTION_ALIGNMENT(sec), run);
	    if (!AddressIsEq (run, SECTION_CADDRESS(sec)))
		    ok = FALSE;
	    run = AddressAdd (run, SECTION_CSIZE(sec));
    }
    if (!ok) FATAL(("Failed to reorder sections contiguously"));
    size = AddressSub (run, start);
  }
  /* Kill all existing $switch symbols, as they are no longer guaranteed to be
   * correct (section reordering screws them up ... */
  {

    t_symbol *top =
      SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch");
    t_symbol *he;

    symbol_type_cached = NULL;

    while (top)
    {
      he = (t_symbol *) SYMBOL_EQUAL(top);
      SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(top), top);
      top = he;
    }
  }
  /* Kill all existing $code_switch symbols, as they are no longer guaranteed to be correct
   * (section reordering screws them up ... */
  {

    t_symbol *top =
      SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$code_switch");
    t_symbol *he;

    symbol_type_cached = NULL;

    while (top)
    {
      he = (t_symbol *) SYMBOL_EQUAL(top);
      SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(top), top);
      top = he;
    }
  }

  /* second step: create new section from the different code sections */
  tmpsec = SectionCreateForObject (obj, CODE_SECTION, NULL,
                                   size, ".text");
  SECTION_SET_CADDRESS(tmpsec, start);
  SECTION_SET_OLD_ADDRESS(tmpsec, SECTION_OLD_ADDRESS(OBJECT_CODE(obj)[0]));
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
  {
    if (sec == tmpsec) continue;
    {
      t_uint32 offset =
        AddressExtractUint32 (AddressSub (SECTION_CADDRESS(sec),
                                          SECTION_CADDRESS(tmpsec)));
      t_uint32 size = AddressExtractUint32 (SECTION_CSIZE(sec));

      printf("Copying to %s:%s from %s:%s size %x offset %x\n",
             OBJECT_NAME(SECTION_OBJECT(tmpsec)), SECTION_NAME(tmpsec),
             OBJECT_NAME(SECTION_OBJECT(sec)), SECTION_NAME(sec), size, offset);
      memcpy (((char *) SECTION_DATA(tmpsec)) + offset, SECTION_DATA(sec), (size_t) size);
    }
  }

  /* third step: make all subsections point to the new section as their parent section */
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
  {
    if (sec == tmpsec) continue;
    while (SECTION_SUBSEC_FIRST (sec))
      SectionReparent (SECTION_SUBSEC_FIRST (sec), tmpsec);
  }

  /* fourth step: make all relocations point to the right section */
  OBJECT_FOREACH_RELOC(obj, rel)
  {
    t_uint32 i;
    for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
    {
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_SECTION)
        if (SECTION_TYPE(T_SECTION(RELOC_TO_RELOCATABLE(rel)[i])) == CODE_SECTION)
        {
          t_address offset =
            AddressSub (RELOCATABLE_CADDRESS
                        (RELOC_TO_RELOCATABLE(rel)[i]),
                        SECTION_CADDRESS(tmpsec));

          RelocSetToRelocatable (rel, i, T_RELOCATABLE(tmpsec));
          RELOC_TO_RELOCATABLE_OFFSET(rel)[i] = AddressAdd (RELOC_TO_RELOCATABLE_OFFSET(rel)[i], offset);
      }
    }
    /* relocations should always be from _sub_section, never from section... */
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_SECTION)
      FATAL(("Relocations corrupt"));
  }

  /* fifth step: migrate all symbols that point to the old code sections */
  for (sym = SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj)); sym; sym = SYMBOL_NEXT(sym))
  {
    if (SYMBOL_BASE(sym) && RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SECTION
        && SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym))) == CODE_SECTION)
    {
      t_address offset = AddressSub (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), start);

      SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), offset));
      SymbolSetBase(sym, T_RELOCATABLE(tmpsec));
    }
  }

  if (OBJECT_DYNAMIC_SYMBOL_TABLE(obj))
  {
    /* fifth step b: migrate all dynamic symbols that point to the old code sections */
    for (sym = SYMBOL_TABLE_FIRST(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)); sym; sym = SYMBOL_NEXT(sym))
    {
      if (SYMBOL_BASE(sym) && RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SECTION
          && SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym))) == CODE_SECTION)
      {
        t_address offset = AddressSub (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), start);

        SYMBOL_SET_OFFSET_FROM_START(sym, AddressAdd (SYMBOL_OFFSET_FROM_START(sym), offset));
        SymbolSetBase(sym, T_RELOCATABLE(tmpsec));
      }
    }
  }

  /* sixth step: add $d symbols for the padding between the merged code sections */
  run = start;
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
  {
    if (sec == tmpsec) continue;
    
    {
      t_address tmp = AddressAlign (SECTION_ALIGNMENT(sec), run);

      /* sanity check */
      ASSERT(AddressIsEq (tmp, SECTION_CADDRESS(sec)),
             ("Ouch: @G <-> @G\n", tmp, SECTION_CADDRESS(sec)));

      if (!AddressIsEq (tmp, run))
      {
        /* add $d symbols for the padding */
        SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                       T_RELOCATABLE(tmpsec), AddressSub(run,SECTION_CADDRESS(tmpsec)), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      }
      run = AddressAdd (tmp, SECTION_CSIZE(sec));
    }
  }

  /* seventh step: save information about the original code sections in
   * OBJECT_ORIG_CODES(obj) */
  OBJECT_SET_N_ORIG_CODES(obj, OBJECT_NCODES(obj)-1);
  OBJECT_SET_ORIG_CODES(obj, Calloc (OBJECT_NCODES(obj) - 1, sizeof (t_section_descriptor)));
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
  {
    if (sec == tmpsec) continue;
    OBJECT_ORIG_CODES(obj)[tel].name = StringDup (SECTION_NAME(sec));
    OBJECT_ORIG_CODES(obj)[tel].base = SECTION_CADDRESS(sec);
    OBJECT_ORIG_CODES(obj)[tel].size = SECTION_CSIZE(sec);
    OBJECT_ORIG_CODES(obj)[tel].obase = SECTION_OLD_ADDRESS(sec);
    OBJECT_ORIG_CODES(obj)[tel].osize = SECTION_OLD_SIZE(sec);
    OBJECT_ORIG_CODES(obj)[tel].type = CODE_SECTION;
  }

  /* eighth step: remove the old code sections */
  OBJECT_FOREACH_CODE_SECTION (obj, sec, tel)
    if (sec != tmpsec)
      SectionFree (sec);
  OBJECT_SET_NCODES(obj, 1);
  Free (OBJECT_CODE(obj));
  OBJECT_SET_CODE(obj, Malloc (sizeof (t_section *)));
  OBJECT_CODE(obj)[0] = tmpsec;

  /* ninth step: rebuild the list of $switch symbols */
  ObjectCreateDataOrCodeTable (obj);
#ifdef VERBOSE_MERGING
  VERBOSE(0, ("Total size=@G", size));
#endif
}

t_segment *
ObjectGetSegmentByName (const t_object * obj, t_const_string name)
{
  t_segment *ret;

  for (ret = OBJECT_SEGMENT_FIRST(obj); ret; ret = SEGMENT_SEGMENT_NEXT(ret))
    if (!strcmp (SEGMENT_NAME(ret), name))
      break;
  return ret;
}

void
ObjectAddSegment (t_object * obj, t_segment * seg)
{
  SEGMENT_SET_SEGMENT_NEXT(seg, OBJECT_SEGMENT_FIRST(obj));
  if (SEGMENT_SEGMENT_NEXT(seg))
    SEGMENT_SET_SEGMENT_PREV(SEGMENT_SEGMENT_NEXT(seg), seg);
  OBJECT_SET_SEGMENT_FIRST(obj, seg);
  if (!OBJECT_SEGMENT_LAST(obj))
    OBJECT_SET_SEGMENT_LAST(obj, seg);
}

typedef t_section *t_section_ptr;
static int
_sec_reorder (const void *a, const void *b)
{
  const t_section_ptr *seca = (const t_section_ptr*)a;
  const t_section_ptr *secb = (const t_section_ptr*)b;

  return AddressIsLt (SECTION_OLD_ADDRESS(*seca),
                      SECTION_OLD_ADDRESS(*secb)) ? -1 : 1;
}

typedef struct _t_new_layout_table t_new_layout_table;

struct _t_new_layout_table
{
  t_address orig;
  t_address new;
};


t_section *
ObjectGetSectionContainingAddress (const t_object * obj, t_address addr)
{
  t_section *sec = NULL;
  int i;

  OBJECT_FOREACH_SECTION(obj, sec, i)
  {
    if (AddressIsGe (addr, SECTION_OLD_ADDRESS(sec)) &&
        AddressIsLt (addr,
                     AddressAdd (SECTION_OLD_ADDRESS(sec),
                                 SECTION_OLD_SIZE(sec))))
      return sec;
  }
  return NULL;
}

t_object *
ObjectGetLinkerSubObject (t_object * obj)
{
  t_object *sub = ObjectGetFromCache ("Linker", obj);

  if (!sub)
  {
    sub = ObjectNewCached ("Linker", obj);
    if (OBJECT_MAPPED_LAST(obj) == NULL)
    {
      OBJECT_SET_MAPPED_LAST(obj, sub);
      OBJECT_SET_MAPPED_FIRST(obj, sub);
    }
    else
    {
      OBJECT_SET_NEXT(OBJECT_MAPPED_LAST(obj), sub);
      OBJECT_SET_MAPPED_LAST(obj, sub);
    }
    OBJECT_SET_NEXT(sub, NULL);
    OBJECT_SET_RELOC_TABLE(sub, RelocTableNew (sub));
    OBJECT_SET_SYMBOL_TABLE(sub, SymbolTableNew (sub));
    OBJECT_SET_SWITCHED_ENDIAN(sub, OBJECT_SWITCHED_ENDIAN(obj));
  }
  return sub;
}

t_object * 
ObjectGetSubobjectContainingAddress (const t_object * obj, t_address addr)
{
  t_object *subobj, *tmpobj;
  t_section * section;

  OBJECT_FOREACH_SUBOBJECT(obj,subobj,tmpobj)
    if((section = ObjectGetSectionContainingAddress (subobj, addr)) != NULL)
        return subobj;

  return NULL;
}

void ObjectRegisterBuiltinObject(t_const_string fileformat, t_const_string arch, t_const_string id, t_const_string oname)
{
  t_builtin_object *b = Malloc(sizeof(t_builtin_object));
  b->fileformat = StringDup(fileformat);
  b->arch = StringDup(arch);
  b->id = StringDup(id);
  b->object_name = StringDup(oname);

  b->next = builtin_object_table;
  builtin_object_table = b;
}

void ObjectDestroyBuiltinObject(t_const_string fileformat, t_const_string arch, t_const_string id, t_const_string oname)
{
  t_builtin_object * b = builtin_object_table;
  t_builtin_object * prev = NULL;

  while (b)
  {
    if (strcmp(fileformat, b->fileformat) == 0
        && strcmp(arch, b->arch) == 0
        && strcmp(id, b->id) == 0
        && strcmp(oname, b->object_name) == 0)
    {
      /* remove this object from the linked list */
      if (b == builtin_object_table)
        builtin_object_table = b->next;
      else
        prev->next = b->next;

      /* free allocated strings and actual object */
      Free(b->fileformat);
      Free(b->arch);
      Free(b->id);
      Free(b->object_name);
      Free(b);

      return;
    }

    prev = b;
    b = b->next;
  }
}

/* Processes a 'Local Label' as defined by bintutils. We handle nf and nb with n
 * being an integer. Starting from the provided address we search in either direction
 * in the original subobject for the first matching label. The labels are constructed
 * as shown here: .L[n]C-B[ordinal] but we have no clue what the ordinal will be.
 */
t_symbol *
ObjectGetBinutilsLocalLabelSymbol(const t_object *secobj, t_const_string sname)
{
    t_symbol *sym, *fsym = NULL;
    char regex[6];

    /* Build our regex */
    sprintf (regex, ".L%c%c*", sname[0], 2);

    /* TODO really search back or forward in the symbol table based on an address */
    /* TODO support for bigger integer labels than 0-9 */
    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(secobj), sym)
    {
        if (StringPatternMatch(regex, SYMBOL_NAME(sym)))
        {
            ASSERT (fsym == NULL, 
                    ("ObjectGetBinutilsLocalLabelSymbol; more than 1 local label found. We don't support this yet."));
            fsym = sym;
        }
    }

    return fsym;
}

/* obj is the subobject this symbol comes from. */
/* Use subobject because of possible duplicates */
t_symbol *
ObjectGetSymbolByNameAndContext(const t_object *parentobj, const t_object *secobj, t_const_string sname)
{ 
  t_symbol *sym;

  if (sname == NULL)
    return NULL;

  /* Check if we need to handle the annoying binutils [n]b/f Local Labels 
   * TODO binutils support bigger integer than 0-9, need real regex here */
  if ( strlen(sname) == 2 &&
      (sname[1] == 'b' || sname[1] == 'f') &&
      (sname[0] <= '9' && sname[0] >= '0') )
      return ObjectGetBinutilsLocalLabelSymbol (secobj, sname);
  
  sym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(secobj), sname);

  /* the previous approach won't work for symbols in vectorized sections, as
   * the original symbol and its mapped counterpart are no longer in the
   * same object file. Therefore, we need to take the long way round to find
   * the symbols
   * first approach works if the referenced symbols come from the same
   * section as the @-@ symbol */
  if (!sym)
  {
    sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(parentobj), sname);
    while (sym)
    {
      if (SYMBOL_BASE(sym) &&
          RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION &&
          SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym))) == secobj)
        break;
      sym = SYMBOL_EQUAL(sym);
    }
  }

  /* second approach works if the referenced symbols come from a different,
   * non-vectorized section in the original object */
  if (!sym)
  {
    sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(parentobj), sname);
    while (sym)
    {
      if (SYMBOL_BASE(sym) &&
          RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION)
      {
        t_object *secobj = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym)));
        t_symbol *tsym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(secobj), SYMBOL_NAME(sym));
        if (tsym && SYMBOL_MAPPED(tsym) == sym)
          break;
      }
      sym = SYMBOL_EQUAL(sym);
    }
  }

  return sym;
}

void ObjectAddComdatSectionGroup(t_object *obj, t_section_group *group)
{
  OBJECT_SET_NCOMDAT_SECTION_GROUPS(obj,OBJECT_NCOMDAT_SECTION_GROUPS(obj)+1);
  OBJECT_SET_COMDAT_SECTION_GROUPS(obj,Realloc(OBJECT_COMDAT_SECTION_GROUPS(obj),OBJECT_NCOMDAT_SECTION_GROUPS(obj)*sizeof(*group)));
  OBJECT_COMDAT_SECTION_GROUPS(obj)[OBJECT_NCOMDAT_SECTION_GROUPS(obj)-1] = group;
}

t_section_group *ObjectFindComdatSectionGroupBySignature(const t_object *obj, t_const_string signature)
{
  t_uint32 i;

  for (i = 0; i < OBJECT_NCOMDAT_SECTION_GROUPS(obj); i++)
  {
    if (strcmp((OBJECT_COMDAT_SECTION_GROUPS(obj)[i])->signature,signature)==0)
      return OBJECT_COMDAT_SECTION_GROUPS(obj)[i];
  }
  return NULL;
}

/* Construct the final symbol table for an object, in the process of which we'll remove the
 * symbols that match one of the patterns declared the the NULL-terminated string array.
 */
void ObjectConstructFinalSymbolTable(t_object *obj, t_const_string const *strip_symbol_masks)
{
  t_symbol *sym, *tmp;

  SYMBOL_TABLE_FOREACH_SYMBOL_SAFE(OBJECT_SUB_SYMBOL_TABLE(obj), sym, tmp)
  {
    t_uint32 iii = 0;
    t_const_string mask = strip_symbol_masks?strip_symbol_masks[0]:NULL;
    t_bool removed = FALSE;

    /* Try all masks and remove the symbol if one of them matches */
    while(mask)
    {
      if (StringPatternMatch(mask, SYMBOL_NAME(sym)) && (SYMBOL_ORDER(sym) < 0))
      {
        SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), sym);
        removed = TRUE;
        break;
      }

      iii++;
      mask = strip_symbol_masks[iii];
    }

    /* If the symbol has been removed, go to the next one */
    if(removed) continue;

    if (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION)
    {
      SYMBOL_SET_OFFSET_FROM_START(sym,
          AddressAdd(SYMBOL_OFFSET_FROM_START(sym),
          AddressSub(SECTION_CADDRESS(T_SECTION(SYMBOL_BASE(sym))),
          SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE(sym)))))));

      SymbolSetBase(sym, T_RELOCATABLE(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE(sym)))));
    }
  }

  if (OBJECT_SYMBOL_TABLE(obj))
    SymbolTableFree(OBJECT_SYMBOL_TABLE(obj));

  OBJECT_SET_SYMBOL_TABLE(obj, OBJECT_SUB_SYMBOL_TABLE(obj));
  OBJECT_SET_SUB_SYMBOL_TABLE(obj, NULL);
}

int sizeof_uleb128(t_uint32 value)
{
  int bytes = 0;
  while (value!=0)
    {
      bytes++;
      value >>= 7;
    }
  if (bytes==0)
    bytes=1;
  return bytes;
}

/* this function creates more space (in ARM.extab sections typically) for ULEB relocations that need
   more bytes in the rewritten binary than in the original binary */

/* TODO: replace uint32 constants and computations with proper t_address computations */
t_bool ObjectAdaptSpaceForLEBRelocs(t_object * obj)
{
  t_bool change = FALSE;
  t_reloc * rel, *rel2;
  t_relocatable * lrel, * rrel;
  t_address laddress, raddress;
  t_int32 displacement;
  int bytes;
  int orig_bytes;

  OBJECT_FOREACH_RELOC(obj,rel)
    {
      /* here and later, we assume the code pattern in relocations added for ULEB128 references 
         in .ARM.extab sections in the ARMv7 disassembler for $diablo ULEB128 symbols */
      if (!StringPatternMatch("R00A01+R01A02+-\\e*",RELOC_CODE(rel))) continue;
      lrel = RELOC_TO_RELOCATABLE(rel)[0];
      rrel = RELOC_TO_RELOCATABLE(rel)[1];
      laddress = RELOC_TO_RELOCATABLE_OFFSET(rel)[0];
      raddress = RELOC_TO_RELOCATABLE_OFFSET(rel)[1];
      laddress = AddressAdd(laddress,RELOCATABLE_CADDRESS(lrel));
      raddress = AddressAdd(raddress,RELOCATABLE_CADDRESS(rrel));

      displacement = G_T_UINT32(AddressSub(laddress,raddress));
      
      bytes = sizeof_uleb128(displacement);
      orig_bytes = RELOC_CODE(rel)[17]-'0';

      if (bytes > orig_bytes)
        {
          bytes = orig_bytes + 4; /* add 4 bytes right away, to keep alignment of later data */
          VERBOSE(2,("FOUND ULEB RELOC THAT NEEDS MORE PLACE: 0x%p @R",rel,rel));
          change = TRUE;
          int diff = bytes-orig_bytes;
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel))==RT_SUBSECTION,("OOPS, not a subsection"));
          t_section * sec = T_SECTION(RELOC_FROM(rel));
          t_uint32 offset = RELOC_FROM_OFFSET(rel);
          t_uint32 size = SECTION_CSIZE(sec);
          t_uint32 orig_size = size;
          size += diff;

          /* create additional space */
          SECTION_SET_DATA(sec,Realloc(SECTION_DATA(sec),size));
          SECTION_SET_CSIZE(sec,size);

          memmove((void*)SECTION_DATA(sec)+offset+bytes,(void*)SECTION_DATA(sec)+offset+orig_bytes,orig_size-(offset+orig_bytes));

          /* update the reloc such that more bytes will be written */
          RELOC_CODE(rel)[17]='0'+bytes;

          VERBOSE(2,("REWRITTEN INTO @R",rel));

          /* update the offset of other relocs in the shifted part of the same section */
          t_reloc_ref * ref = RELOCATABLE_REFERS_TO(T_RELOCATABLE(sec));
          for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
            {
              t_reloc * rel2 = RELOC_REF_RELOC(ref);

              if (RELOC_FROM_OFFSET(rel2)>=offset+orig_bytes)
                {
                  VERBOSE(2,("   ALSO UPDATING 0x%p @R",rel2, rel2));
                  RELOC_SET_FROM_OFFSET(rel2,RELOC_FROM_OFFSET(rel2)+diff);
                  VERBOSE(2,("   INTO @R",rel2));
                }
            }

          /* First check that we can use SWITCH_EDGE to indicate which
             RELOCS have already been updated. This is necessary because if two or more to-relocatables
             of a reloc are in this section, we will iterate over them twice. But of course we don't want
             to increment both of their offsets twice*/

          ref = RELOCATABLE_REFED_BY(T_RELOCATABLE(sec));
          for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
            {
              t_reloc * rel2 = RELOC_REF_RELOC(ref);
              ASSERT(RELOC_SWITCH_EDGE(rel2)==NULL,("OOPS"));
            }

          /* Then actually update the offsets of all relocs to the shifted part of the same section */
          ref = RELOCATABLE_REFED_BY(T_RELOCATABLE(sec));
          for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
            {
              t_reloc * rel2 = RELOC_REF_RELOC(ref);
              if (RELOC_SWITCH_EDGE(rel2)!=NULL) continue;
              RELOC_SET_SWITCH_EDGE(rel2,(void*)rel);
              for (t_uint32 i=0;i<RELOC_N_TO_RELOCATABLES(rel2);i++)
                {
                  if (RELOC_TO_RELOCATABLE(rel2)[i]!=T_RELOCATABLE(sec)) continue;
                  if (RELOC_TO_RELOCATABLE_OFFSET(rel2)[i]+RELOC_ADDENDS(rel2)[0]>=offset+orig_bytes)
                    {
                      VERBOSE(2,("   ALSO UPDATING 0x%p @R",rel2, rel2));
                      RELOC_ADDENDS(rel2)[0]+=diff;
                      VERBOSE(2,("   INTO @R",rel2));
                    }
                }
            }

          /* finally, reset the to symbol offsets */
          ref = RELOCATABLE_REFED_BY(T_RELOCATABLE(sec));
          for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
            {
              t_reloc * rel2 = RELOC_REF_RELOC(ref);
              RELOC_SET_SWITCH_EDGE(rel2,NULL);
            }
        }
    }
  return change;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */

