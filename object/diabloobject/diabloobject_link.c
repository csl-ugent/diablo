/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <stdio.h>
#include <string.h>
#include <strings.h>  /** for bzero */
#include <stdarg.h>
#include <diabloobject.h>


/* global variable tells the link layer in Diablo whether it is emulating a link
 * from the native linker, or linking on its own. This needs to be global
 * because it has to be passed into LinkerScriptParserparse, a function that
 * accepts no arguments */
t_bool emulate_link = FALSE;

/* Global variable that is used to link in new objects after linker emulation */
static t_object * linkin = NULL;

/* Static declarations to keep the compiler happy {{{*/
void ObjectMergeSubObjectSymbolTables (t_object * obj);
void ObjectAddLinkerSpecificDataAndSymbols (t_object * obj);
void ObjectMarkSubsectionGapsAsData (t_object * obj);
void ObjectConnectSubObjectSymbolTableWithMergedTable (t_object * obj);
void ObjectVerifySymbolTable (const t_object * obj);
void RelocateCore (t_object * obj, t_bool verify);
/*}}}*/
/* Map related stuff {{{ */
/*!
 *
 * \param obj
 *
 * \return void
 */

void
ObjectReadMap (t_object * obj)
{
  t_string map_name = MapNameF(obj);
  FILE *fp = fopen (map_name, "r");

  Free (map_name);
  OBJECT_SET_MAP(obj, MapNew ());
  OBJECT_MAP(obj)->obj = obj;

  if (!fp)
  {
    if (OBJECT_COMPRESSED_SUBOBJECTS(obj))
    {
      MapReadCompressed(OBJECT_MAP(obj), obj);
    }
    else
    {
      FATAL(("Could not open map %s\nDiablo needs a linker map file to relink the program in exactly the same way as the original linker linked the program. You can generate a map file by passing the appropriate command line switches to the linker/compiler. More information about this can be found in the Diablo manual and the FAQ. (http://www.elis.ugent.be/diablo).", OBJECT_NAME(obj)));
    }
  }
  else
  {
    MapReadF(OBJECT_MAP(obj), fp);
    fclose(fp);
  }
}

/*!
 *
 * \param obj
 *
 * \return void
 */

void
ObjectReadObjectInMap (t_object * obj, t_bool read_debug)
{
  t_objects_ll list;

  /* obj is a parent object. initialise it's subobject cache */
  OBJECT_SET_SUBOBJECT_CACHE(obj, HashTableNew (3001, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, ObjectHashElementFree));

  /* read the objects in the map */
  list = MapLoadObjects (OBJECT_MAP(obj), obj, read_debug);
}

void
ObjectAppendSubObject (t_object * parent, t_object * child)
{
  if (OBJECT_NEXT(child) || OBJECT_PREV(child) || child == OBJECT_MAPPED_LAST(parent)
      || child == OBJECT_MAPPED_FIRST(parent))
    return;

  if (OBJECT_MAPPED_LAST(parent))
  {
    OBJECT_SET_NEXT(OBJECT_MAPPED_LAST(parent), child);
    OBJECT_SET_PREV(child, OBJECT_MAPPED_LAST(parent));
    OBJECT_SET_NEXT(child, NULL);
    OBJECT_SET_MAPPED_LAST(parent, child);
  }
  else if (OBJECT_MAPPED_FIRST(parent))
  {
    FATAL(("The list with object files belonging to the executable named %s is corrupt. Unless you are modifying this list yourself, this probably is a Diablo bug... send information to the diablo mailing list (diablo@@elis.ugent.be)!", OBJECT_NAME(parent)));
  }
  else
  {
    OBJECT_SET_PREV(child, NULL);
    OBJECT_SET_NEXT(child, NULL);
    OBJECT_SET_MAPPED_LAST(parent, child);
    OBJECT_SET_MAPPED_FIRST(parent, child);
  }
}

/*!
 * for the alpha, the link-order for the lita is not included in the mapfile.
 * We have to calculate the order ourselfs. This should always be possible
 * since the order of the lita's is the same as the order of the textsections
 * */

void
ObjectMapPatchAlpha (t_object * obj)
{
  t_section *sec = SectionGetFromObjectByName (obj, ".lita");
  t_section * sub;

  if (!sec)
    return;

  SECTION_FOREACH_SUBSECTION(sec, sub)
  {
    t_reloc_ref * ref = RELOCATABLE_REFED_BY(T_RELOCATABLE(sub));
    for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
    {
      t_reloc * rel = RELOC_REF_RELOC(ref);
      t_section * parent = SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel)));
      t_address from = AddressAdd(SECTION_CADDRESS(T_SECTION(RELOC_FROM(rel))), RELOC_FROM_OFFSET(rel));
      t_int32 ins = SectionGetData32(parent, AddressSub(from, SECTION_CADDRESS(parent)));
      t_int32 ins_orig = SectionGetData32(T_SECTION(RELOC_FROM(rel)),  RELOC_FROM_OFFSET(rel));

      ins &= 0xffff | ((ins & 0x8000)?0xffff0000:0);
      ins_orig &= 0xffff | ((ins_orig & 0x8000)?0xffff0000:0);
      ins_orig += 32752;
      ins += 32752;
      ins -= ins_orig;

      SECTION_SET_CADDRESS(sub, AddressAddUint32(SECTION_CADDRESS(sec), ins));
      SECTION_SET_OLD_ADDRESS(sub, AddressAddUint32(SECTION_CADDRESS(sec), ins));
    }
  }
}

void 
ObjectResolveTentativeSymbols (t_object * obj)
{
  t_symbol * sym;
  
  STATUS(START, ("Resolving tentative symbols"));
  for (sym = SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
  {
    if (SYMBOL_TENTATIVE(sym))
    {
      t_string tentative_copy;

      /* while executing a rule for a tentative symbol, we may
       * insert a new symbol with the same name as the one to
       * which this tentative rule belongs -> the original
       * symbol's tentative rule will be freed and replaced
       * by the new tentative symbol's rule (if any) ->
       * the lexer/parser will continue in the (now freed)
       * memory
       * -> make a copy of the tentative rule and parse that
       * copy
       */
      tentative_copy = StringDup(SYMBOL_TENTATIVE(sym));
      LinkerScriptParse (0, tentative_copy, obj, sym);
      Free(tentative_copy);
    }
  }
  STATUS(STOP, ("Resolving tentative symbols"));
}

void ObjectAddMissingExidxEntries(t_object * obj)
{
  t_object * sub_obj, * tmp;
  int tel;
  t_section * sec;

  STATUS(START, ("Adding missing .ARM.exidx entries"));

  OBJECT_FOREACH_SUBOBJECT(obj, sub_obj, tmp)
    OBJECT_FOREACH_SECTION(sub_obj, sec, tel)
    {
      if (!StringPatternMatch(".ARM.exidx*",SECTION_NAME(sec))) continue;
      if (!(SECTION_FLAGS(sec) & SECTION_FLAG_EXIDX_NEEDS_RELAXATION)) continue;
      SECTION_SET_CSIZE(sec,SECTION_CSIZE(sec)+0x8);
      SECTION_SET_DATA(sec,Realloc(SECTION_DATA(sec),SECTION_CSIZE(sec)));

      SectionSetData32(sec,SECTION_OLD_SIZE(sec)-4,0x1);

      t_reloc_ref * ref = RELOCATABLE_REFERS_TO(T_RELOCATABLE(sec));
      t_reloc * last_rel;

      t_address extra_offset = 0;

      for (;ref!=NULL; ref=RELOC_REF_NEXT(ref))
        {
          t_reloc * rel = RELOC_REF_RELOC(ref);
          
          if (RELOC_FROM_OFFSET(rel)==SECTION_OLD_SIZE(sec)-16)
            {
              /* for the last relocation already in the .ARM.exidx section, find the section to which its last
                 PREL31 reloc pointed */

              ASSERT(RELOC_N_TO_SYMBOLS(rel)>0,("OOPS, PREL31 RELOC THAT IS NOT UNRELAXED TO SYMBOL"));
              t_symbol* symbol = RELOC_TO_SYMBOL(rel)[0];
              t_section *last_sec = T_SECTION(SYMBOL_BASE(symbol));              
              t_address last_address = SECTION_CADDRESS(last_sec)+SECTION_CSIZE(last_sec);
              t_address generic = AddressNew32(SECTION_OLD_SIZE(sec)-8);
              
              t_symbol * new_sym = SymbolTableGetFirstSymbolByAddress(OBJECT_SUB_SYMBOL_TABLE(obj), last_address);
              
              while (new_sym && !(SYMBOL_FLAGS(new_sym) & (SYMBOL_TYPE_FUNCTION | SYMBOL_TYPE_FUNCTION_SMALLCODE)))
                new_sym = SymbolTableGetNextSymbolByAddress(new_sym,last_address);
              
              while (!new_sym)
                {
                  last_address += 4;
                  extra_offset -= 4;
                  new_sym = SymbolTableGetFirstSymbolByAddress(OBJECT_SUB_SYMBOL_TABLE(obj), last_address);
                  
                  while (new_sym && !(SYMBOL_FLAGS(new_sym) & (SYMBOL_TYPE_FUNCTION | SYMBOL_TYPE_FUNCTION_SMALLCODE)))
                    new_sym = SymbolTableGetNextSymbolByAddress(new_sym,last_address);
                }

              ASSERT(new_sym,("did not find function symbol at address @G", last_address));
              
              t_reloc * new_reloc = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), extra_offset, T_RELOCATABLE (sec), generic, new_sym, diabloobject_options.keep_exidx, NULL, NULL, NULL, "S00P-A00+ i7fffffff& " "\\" WRITE_32);
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            0x1,                     /* addend is the data that was originally present */
                                            T_RELOCATABLE(sec),            /* from */
                                            AddressAddUint32(generic, 4),                  /* from offset */
                                            NULL,                     /* to; none in this case */
                                            diabloobject_options.keep_exidx,
                                            NULL, NULL, NULL,         /* edge, corresp, e_sym */
                                            "A00" "\\" WRITE_32);

              VERBOSE(1,("Increased size of section %s in %s in %s for adding additional exidx entry and added @R", SECTION_NAME(sec), OBJECT_NAME(obj), OBJECT_NAME(sub_obj), new_reloc));
            }
        }

      SectionSetData32(sec,SECTION_OLD_SIZE(sec)-8,0x00000000);
    }

  STATUS(STOP, ("Adding missing .ARM.exidx entries"));
}

/* }}} */
/* Linking functions {{{ */
void
ObjectBuildUnifiedSymbolTableAndRelocTable (t_object * obj)
{
  if (!OBJECT_MAP(obj))
    FATAL(("There is no map to build the global symbol table!\n"));

  ObjectMergeSubObjectSymbolTables (obj);

  ObjectResolveTentativeSymbols (obj);

  ObjectAddLinkerSpecificDataAndSymbols (obj);

  ObjectAddMissingExidxEntries(obj);

  DiabloBrokerCall("ObjectAddManualSpecificDataAndSymbols",obj);

  if (linkin) ObjectPlaceSections (obj, FALSE, FALSE, FALSE);

  ObjectMarkSubsectionGapsAsData (obj);
  ObjectConnectSubObjectSymbolTableWithMergedTable (obj);
  
  if (!linkin && OBJECT_SYMBOL_TABLE(obj))
    ObjectVerifySymbolTable (obj);
  ObjectCreateDataOrCodeTable (obj);
  if (OBJECT_SYMBOL_TABLE(obj))
    ObjectBuildRelocTableFromSubObjects (obj);
}

static void
ObjectMergeSubObjectSymbolTable (t_object * obj, t_object * sub_obj)
{
  /* First of all, insert a symbol called $a for every code section in the binary, and $d for all the rest.
   * This will be used further on to compute the $switch symbol (i.e. all places where a code section contains
   * data and all places where this stops). These might be duplicate, but they will be removed after sorting
   * {{{ */
  t_section *sec;
  t_uint32 tel;
  t_symbol *sym;
  if (OBJECT_NCODES(sub_obj))
    for (tel = 0; tel < OBJECT_NCODES(sub_obj); tel++)
    {
      t_symbol *data =
        (t_symbol *) SymbolTableLookup (OBJECT_SYMBOL_TABLE(sub_obj),
                                      "$d");
      t_symbol *thumb =
        (t_symbol *) SymbolTableLookup (OBJECT_SYMBOL_TABLE(sub_obj),
                                      "$t");

      sec = OBJECT_CODE(sub_obj)[tel];
      if (SECTION_IS_VECTORIZED(sec))
        FATAL(("Encountered a vectorized section. They should have been removed from the binary by now..."));
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      if (AddressIsNull (SECTION_CSIZE (sec)))
        continue; /* Skip empty sections */

      while (data)
      {
        if (SYMBOL_BASE(data) == T_RELOCATABLE(sec)
            && AddressIsNull (SYMBOL_OFFSET_FROM_START(data)))
        {
          break;
        }
        data = SYMBOL_EQUAL((t_symbol *) data);
      }
      while (thumb)
      {
        if (SYMBOL_BASE(thumb) == T_RELOCATABLE(sec)
            && AddressIsNull (SYMBOL_OFFSET_FROM_START(thumb)))
        {
          break;
        }
        thumb = SYMBOL_EQUAL(thumb);
      }

      if (!data && !thumb)
      {
        SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$a", "R00A00+$", -1, TRUE, FALSE,
                              T_RELOCATABLE(sec), AddressNullForObject(obj),
                              AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      }
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NDATAS(sub_obj))
    for (tel = 0; tel < OBJECT_NDATAS(sub_obj); tel++)
    {
      sec = OBJECT_DATA(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NTLSDATAS(sub_obj))
    for (tel = 0; tel < OBJECT_NTLSDATAS(sub_obj); tel++)
    {
      sec = OBJECT_TLSDATA(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NNOTES(sub_obj))
    for (tel = 0; tel < OBJECT_NNOTES(sub_obj); tel++)
    {
      sec = OBJECT_NOTE(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NBSSS(sub_obj))
    for (tel = 0; tel < OBJECT_NBSSS(sub_obj); tel++)
    {
      sec = OBJECT_BSS(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NTLSBSSS(sub_obj))
    for (tel = 0; tel < OBJECT_NTLSBSSS(sub_obj); tel++)
    {
      sec = OBJECT_TLSBSS(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NRODATAS(sub_obj))
    for (tel = 0; tel < OBJECT_NRODATAS(sub_obj); tel++)
    {
      sec = OBJECT_RODATA(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  if (OBJECT_NDEBUGS(sub_obj))
    for (tel = 0; tel < OBJECT_NDEBUGS(sub_obj); tel++)
    {
      sec = OBJECT_DEBUG(sub_obj)[tel];
      if (!SECTION_IS_MAPPED(sec))
        continue; /* Skip sections removed by the linker */
      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE,
                                        T_RELOCATABLE(sec), AddressNullForObject(obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    }
  /* }}} */

  /* Merge all defined symbols of the original object in the new symboltable.
   * {{{ */
  for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(sub_obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
  {
    t_symbol *ret;

    /* There can be multiple weak symbols with the same name at different addresses.
     * In that case, the linker seems to keep the one at the lowest address
     * -> do this as well
     */
    if (SYMBOL_ORDER(sym)==SYMBOL_ORDER_WEAK)
    {
      ret = SymbolTableLookup(OBJECT_SUB_SYMBOL_TABLE(obj), SYMBOL_NAME(sym));
      if (ret)
      {
        if (SYMBOL_ORDER(ret)==SYMBOL_ORDER_WEAK)
        {
          t_address new_address, old_address;
          new_address = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
          old_address = StackExecConst(SYMBOL_CODE(ret), NULL, ret, 0, obj);
          if (AddressIsGe(new_address,old_address))
            continue;
        }
      }
    }
    ret = SymbolTableAddAliasedSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), SYMBOL_NAME(sym), SYMBOL_KEY(sym), SYMBOL_CODE(sym), SYMBOL_ORDER(sym), SYMBOL_DUP(sym), SYMBOL_SEARCH(sym), SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), SYMBOL_TENTATIVE(sym), SYMBOL_SIZE(sym), SYMBOL_FLAGS(sym));
    SYMBOL_SET_MAPPED(sym, ret);
  }
}

void
ObjectMergeSubObjectSymbolTables (t_object * obj)
{
  t_object *tmp2, *tmp;

  /* This table will already exist if this function is called after linker emulation (because we're linking in a new object) */
  if(!linkin)
  {
    OBJECT_SET_SUB_SYMBOL_TABLE(obj, SymbolTableNew (obj));
    OBJECT_FOREACH_SUBOBJECT(obj, tmp, tmp2)
    {
      ObjectMergeSubObjectSymbolTable (obj, tmp);
    }
  }
  else ObjectMergeSubObjectSymbolTable (obj, linkin);
}

/* mark all subsection gaps in the code (caused by subsection alignment) as
 * data, because the linker doesn't automatically do that {{{ */

void
ObjectMarkSubsectionGapsAsData (t_object * obj)
{
  t_section *sec;
  t_uint32 nsubs, i;
  t_section **subs;
  t_uint32 tel;

  for (tel = 0; tel < OBJECT_NCODES(obj); tel++)
  {
    sec = OBJECT_CODE(obj)[tel];
    subs = SectionGetSubsections (sec, &nsubs);
    if (!subs)
      continue;

    /* Only indicate gaps between subsections of the same section. Gaps between
     * different parent sections will be taken care of by
     * ObjectMergeCodeSections */
    for (i = 1; i < nsubs; i++)
    {
      if (!SECTION_IS_VECTORIZED(subs[i - 1]))
      {
        t_address endprev = AddressAdd (SECTION_CADDRESS(subs[i - 1]),
                                        SECTION_CSIZE(subs[i - 1]));

        if (AddressIsLt (endprev, SECTION_CADDRESS(subs[i])))
        {
          /* yep, there is a gap: add a $d symbol to the beginning of the gap */
          SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$d", "R00A00+$", -1, TRUE, FALSE, 
                                T_RELOCATABLE((subs[i - 1])),
                                SECTION_CSIZE((subs[i - 1])), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
        }
      }
    }
    Free (subs);
  }
} /* }}} */

void ObjectGetOverlays(t_object *obj)
{
  t_layout_rule* r;
  t_bool in_overlay = FALSE;

  t_layout_script *script = OBJECT_LAYOUT_SCRIPT(obj);  
  if (!script)
    script = ObjectGetAndParseLayoutScript(obj);
      
  for (r = script->first; r; r = r->next)
  {
    if (!in_overlay)
    {
      if (r->kind != OVERLAY_START)
        continue;

      in_overlay = TRUE;

      {
        t_overlay *ovl = Calloc(1, sizeof(t_overlay));
        ovl->next = OBJECT_OVERLAYS(obj);
        OBJECT_SET_OVERLAYS(obj, ovl);
      }
    }
    else
    {
      if (r->kind == OVERLAY_END)
      {
        in_overlay = FALSE;
        continue;
      }

      ASSERT(r->kind == SECTION_SPEC,
             ("only section specifications are allowed within an overlay"));

      {
        t_overlay *ovl = OBJECT_OVERLAYS(obj);
        t_overlay_sec *ovlsec = Calloc(1, sizeof(t_overlay_sec));
        ovlsec->next = ovl->sec;
        ovl->sec = ovlsec;
        ovlsec->name = StringDup(r->u.secspec->name);
      }
    }
  }
}

void LinkerScriptParserparse ();

/* {{{ Get linker script file name (command line specified or standard from linker_descriptions) */
static t_string
GetLinkerScriptFilename (const t_object * obj)
{
  if (diabloobject_options.linkerscript_set)
  {
    /* command line specified */
    return StringDup (diabloobject_options.linkerscript);
  }
  else
  {
    t_string linkerscriptname_tmp2 =
      StringConcat3 (OBJECT_OBJECT_HANDLER(obj)->main_name, "-",
                     OBJECT_OBJECT_HANDLER(obj)->sub_name);
    t_string linkerscriptname_tmp =
      StringConcat3 (linkerscriptname_tmp2, "-", OBJECT_MAP(obj)->handler->name);
    t_string linkerscriptname = StringConcat2 (linkerscriptname_tmp, ".ld");
    t_string ret;

    if (diabloobject_options.linkerscriptprefix_set)
      ret =
        FileFind (diabloobject_options.linkerscriptprefix,
                  linkerscriptname);
    else
      ret =
        StringConcat3 (DATADIR, "/linker_descriptions/", linkerscriptname);

    Free (linkerscriptname_tmp2);
    Free (linkerscriptname_tmp);
    Free (linkerscriptname);
    return ret;
  }
  return NULL;
}

/* }}} */

void
ObjectAddLinkerSpecificDataAndSymbols (t_object * obj)
{
  t_string scriptname;
  
  STATUS(START, ("Adding linker specific symbols and data"));
  scriptname = GetLinkerScriptFilename (obj);

  ASSERT(scriptname, ("No suitable linker script found (%s %s %s)",OBJECT_OBJECT_HANDLER(obj)->main_name, OBJECT_OBJECT_HANDLER(obj)->sub_name, OBJECT_MAP(obj)->handler->name));

  LinkerScriptParse (1, scriptname, obj, NULL);

  Free (scriptname);

  STATUS(STOP, ("Adding linker specific symbols and data"));
}

void
ObjectConnectSubObjectSymbolTableWithMergedTable (t_object * obj)
{
  t_object *tmp2, *tmp;
  t_symbol *sym, *msym;

  /* Map all symbols to defined symbols */
  OBJECT_FOREACH_SUBOBJECT(obj, tmp, tmp2)
  {
    for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(tmp)); sym != NULL; sym = SYMBOL_NEXT(sym))
    {
      if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_SECTION)
        continue;
      if ((!SYMBOL_MAPPED(sym)) ||
          ((SYMBOL_BASE(sym)==T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))) &&
           (SYMBOL_BASE(SYMBOL_MAPPED(sym)) == T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)))))
      {
          msym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), SYMBOL_NAME(sym));

          if (!msym)
          {
            /* it's possible for dynamic symbols to be unmapped after link. This
             * is a weird kind of corner case that I don't really understand,
             * but it happens with ppc64 dynamically linked binaries that
             * reference the environ symbol: for some reason, this also includes
             * the _environ and __environ symbols in the resulting binary, even
             * though these are not really  referenced in the object files. This
             * is probably a manifestation of a deeper mechanism that I don't
             * really understand yet. So probably the following code will change
             * sometime in the future.
             *
             * For now, the heuristic is: if the symbol is unmapped, but it is
             * also undefined, we will just add a subsymbol and map it
             * ourselves. */
            if (SYMBOL_BASE(sym) == T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)) && (SYMBOL_SEARCH(sym)==TRUE))
            {
              msym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
                                          SYMBOL_NAME(sym), SYMBOL_CODE(sym),
                                          SYMBOL_ORDER(sym), SYMBOL_DUP(sym),
                                          SYMBOL_SEARCH(sym), SYMBOL_BASE(sym),
                                          SYMBOL_OFFSET_FROM_START(sym),
                                          SYMBOL_ADDEND(sym),NULL,SYMBOL_SIZE(sym), 0);
              SYMBOL_SET_MAPPED(sym,msym);
            }
            else
              FATAL(("Symbol %s not mapped after link: @S", SYMBOL_NAME(sym), sym));
          }
          else if (SYMBOL_BASE(msym)==T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)) && (SYMBOL_SEARCH(msym)==TRUE)) 
          {
            //FATAL(("Symbol @S (search=%d) still undefined after link!", sym, SYMBOL_SEARCH(sym)));
          }
          else
            SYMBOL_SET_MAPPED(sym, msym);
      }
    }
  }
}

void
ObjectVerifySymbolTable (const t_object * obj)
{
  t_symbol *sym;

  for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
  {
    t_symbol *tsym = SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj), SYMBOL_NAME(sym));

    if (!tsym)
    {
      // Should be WARNING but no verbosity options for that macro
      VERBOSE(2, ("<@S> was in the linked binary, but not present after relinking!\n", sym));
    }
    else
    {
      t_address sym_address = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
      t_address tsym_address = StackExecConst(SYMBOL_CODE(tsym), NULL, tsym, 0, obj);

      if (!AddressIsEq (sym_address, tsym_address) && !SYMBOL_DUP(sym))
        WARNING(("%s differs! - sym_address is: %x - tsym_address is: %x\n", SYMBOL_NAME(sym), sym_address, tsym_address));
    }
  }
}

#if 0
void
ObjectExportLinkGraph (t_object * obj)
{
  FILE *fp = NULL;
  t_object *tmp2, *tmp;
  t_reloc *rel;

#ifdef DIABLOSUPPORT_HAVE_MKDIR
  DirMake ("./dots", FALSE);
  fp = fopen ("./dots/linkgraph.dot", "w");
#else
  fp = fopen ("./linkgraph.dot", "w");
#endif
  
  fprintf (fp, "digraph \"Link_graph\" { \n");

  OBJECT_FOREACH_SUBOBJECT(obj, tmp, tmp2) if (OBJECT_RELOC_TABLE(tmp))
    OBJECT_FOREACH_RELOC(tmp, rel)
    {
      if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) != RT_SUBSECTION)
        FATAL(("Not from section @R!", rel));
      if (SECTION_IS_MAPPED(T_SECTION(RELOC_FROM(rel))))
        if ((SYMBOL_TO_TYPE(RELOC_TO_SYMBOL(rel)) == TO_SECTION
             || SYMBOL_TO_TYPE(RELOC_TO_SYMBOL(rel)) == TO_SUB_SECTION))
        {
          if (SYMBOL_BASE(SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel))))
          {
            /* Export link-graph */ fprintf (fp,

                                             "\"%p\" [label=\"%s\" style=solid color=%s]\n",
                                             RELOC_FROM(rel),
                                             OBJECT_NAME(
                                                         SECTION_OBJECT(T_SECTION(RELOC_FROM(rel)))),
                                             (SECTION_TYPE(T_SECTION(RELOC_FROM(rel))) ==
                                              CODE_SECTION) ? "blue" :
                                             "red");
            /* Export link-graph */ fprintf (fp,

                                             "\"%p\" [label=\"%s\" style=solid color=%s]\n",
                                             SYMBOL_BASE(SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel))),
                                             OBJECT_NAME(SECTION_OBJECT(
                                                                        T_SECTION(SYMBOL_BASE(SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)))))),
                                             SECTION_TYPE(T_SECTION(SYMBOL_BASE(SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel))))) ==
                                             CODE_SECTION ? "blue" :
                                             "red");
            /* Export link-graph */ fprintf (fp, "\"%p\" -> \"%p\"\n",

                                             RELOC_FROM(rel),
                                             SYMBOL_BASE(SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel))));
          }
          else
          {
            printf ("SKIPPED %s\n", SYMBOL_NAME(RELOC_TO_SYMBOL(rel)));
          }
        }

    }

  fprintf (fp, "}\n");
}
#endif

void
ObjectBuildRelocTableFromSubObjects (t_object * obj)
{
  t_object *tmp2, *tmp;
  t_reloc *rel, *reln;

  /* Do the relocations */
  OBJECT_FOREACH_SUBOBJECT(obj, tmp, tmp2)
  {
    if (OBJECT_RELOC_TABLE(tmp))
    {
      for (rel = RELOC_TABLE_FIRST(OBJECT_RELOC_TABLE(tmp)); rel != NULL; rel = reln)
      {
        if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) != RT_SUBSECTION)
          FATAL(("While building the relocation table from the input objects, we encountered a relocation that did not originate from a subsection.\nThis relocation is:\n@R\nThere can be many different reasons for this, but the most likely are:\n1. The section is a subsection that was not mapped, most likely because it was not listed in the linker map (and the section was not removed by ObjectRemoveLinkerRemovedSection).\n2. You created a relocation from a (parent object) section in your backend, which is illegal. Try to move it to a subobject-/sub-section.", rel));
        if (SECTION_IS_MAPPED(T_SECTION(RELOC_FROM(rel))))
        {
	        t_uint32 i;
           t_bool skip = FALSE;

          /* Some consistency checks {{{ */
	        for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
	        {
            if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[i]) == NULL)
	          {
              skip = TRUE;
              break;
              FATAL(("Could not merge @R (Because symbol %s from object %s is not mapped after link emulation)!", rel, SYMBOL_NAME(RELOC_TO_SYMBOL(rel)[i]), OBJECT_NAME(tmp)));
	          }
          }
          /* }}} */

          reln = RELOC_NEXT(rel);
          if (skip) 
            continue;
                

	        for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
          {
            /* TODO: the following code is very Thumb-specific.
             * This should be cleaned up one way or another */
            t_symbol *to_sym = RELOC_TO_SYMBOL(rel)[i];
            t_symbol *to_mapped = SYMBOL_MAPPED (to_sym);
            t_symbol *veneer_symbol = NULL;
            char *veneer = NULL;
            t_symbol *stub_symbol = NULL;
            char *stub = NULL;

            /* ARM specific code: control flow transfer relocations between
             * arm and thumb code have to be moved to veneer symbols in
             * some situations {{{ */

            if (StringPatternMatch("*i001ff800 & s000b > *",RELOC_CODE(rel))
                && !StringPatternMatch("*i10000000|:iefffffff*",RELOC_CODE(rel))
                && to_mapped && SYMBOL_BASE(to_mapped))
              
              {
                /* ARM_THM_JUMP24 relocations : coming from thumb code, going to
                   ARM code, we should jump to the stub instead */

                if (ADDRESS_IS_SMALL_CODE !=
                    SymbolTableGetCodeType (obj,
                                            AddressAdd (
                                                        RELOCATABLE_CADDRESS (SYMBOL_BASE (to_mapped)),
                                                        SYMBOL_OFFSET_FROM_START (to_mapped))))
                  {
                    stub = StringConcat3 ("__", SYMBOL_NAME(to_sym),"_from_thumb");
                    
                    VERBOSE(1, ("Looking for %s", stub));
                    /* look for the stub symbol in the sub symboltable, because we want the one
                     * that was added by the linker script rather than one from the original
                     * binary (which may contain multiple such symbols)
                     */
                    stub_symbol = SymbolTableGetSymbolByName ( OBJECT_SUB_SYMBOL_TABLE (obj), stub);
                    if (stub_symbol)
                    {
                      RelocSetToSymbol (rel, i, stub_symbol);
                      VERBOSE(1, ("Changed THUMB b.w to ARM from %s to %s in @R\n",SYMBOL_NAME(to_sym),stub,rel));
                    }
                    else 
                    {
                      //                        FATAL(("Had to replace target of THUMB b.w to ARM from %s to %s, but didn't find that symbol",SYMBOL_NAME(to_sym),stub));
                    }
                    Free(stub);
                  }
              }


            if (StringPatternMatch ("*= s0002 & % s0002*", RELOC_CODE(rel))
                && to_mapped && SYMBOL_BASE(to_mapped))
            {

              /* PC24 arm relocation: we come from arm code 
	            * 	- target == arm: OK
	            * 	- target == thumb && opcode == BLX imm: OK
	            * 	- target == thumb && opcode == B(L): set to veneer
	            */

              if (ADDRESS_IS_SMALL_CODE ==
                  SymbolTableGetCodeType (obj,
		                AddressAdd (
		                  RELOCATABLE_CADDRESS (SYMBOL_BASE (to_mapped)),
		                  SYMBOL_OFFSET_FROM_START (to_mapped))))
              {

                /*TODO the sectiongetdata call only works because ADS binaries
                 * have only one code section! */
                t_uint32 opcode =
                  SectionGetData32 (T_SECTION(RELOC_FROM(rel)),
                      AddressSub (
                        AddressAdd (
                          RELOCATABLE_CADDRESS (RELOC_FROM (rel)),
                          RELOC_FROM_OFFSET(rel)),
                        SECTION_CADDRESS (T_SECTION(RELOC_FROM(rel)))));

                if ((opcode & 0xfe000000) != 0xfb000000)
                {
                  /* opcode != BLX imm */
                  veneer = StringConcat2 ("$Ven$AT$I$$", SYMBOL_NAME(to_sym));
                  VERBOSE(1, ("Looking for %s", veneer));
                  veneer_symbol = SymbolTableGetSymbolByName ( OBJECT_SUB_SYMBOL_TABLE (obj), veneer);
                  Free (veneer);
                  if (!veneer_symbol)
                  {
                    veneer = StringConcat2 ("$Ven$AT$L$$", SYMBOL_NAME(to_sym));
                    VERBOSE(1, ("Looking for %s", veneer));
                    veneer_symbol = SymbolTableGetSymbolByName ( OBJECT_SUB_SYMBOL_TABLE (obj), veneer);
                    Free (veneer);
                  }
                  if (veneer_symbol)
                  {
                    VERBOSE(1, ("Changing to veneer @S: rel @R!\n",veneer_symbol,rel));
                    RelocSetToSymbol (rel, i, veneer_symbol);
                  }
                }
              }
            }
            else if (StringPatternMatch ("*s0001 > = i003ff800*", RELOC_CODE(rel))
                  && to_mapped && SYMBOL_BASE (to_mapped))
	          {
	            
              /* THM_PC22 thumb relocation
	            * 	- target == arm: set to veneer 
	            * 	- target == thumb: OK */
              if (ADDRESS_IS_CODE ==
                  SymbolTableGetCodeType (obj,
		                AddressAdd (
		                  RELOCATABLE_CADDRESS (SYMBOL_BASE (to_mapped)),
		                  SYMBOL_OFFSET_FROM_START (to_mapped))))
	            {
		            veneer = StringConcat2 ("__16", SYMBOL_NAME (to_sym));
		            veneer_symbol = SymbolTableGetSymbolByName (
		              OBJECT_SYMBOL_TABLE (obj), veneer);
		            
                if (veneer_symbol)
		            {
		              VERBOSE(1,("Changed to %s!\n",veneer));
		              RelocSetToSymbol(rel, i, veneer_symbol);
		            }
                else
                {
                  Free(veneer);
                  veneer = StringConcat2 ("$Ven$TA$I$$", SYMBOL_NAME (to_sym));
                  veneer_symbol = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE (obj), veneer);
                  if (veneer_symbol)
                  {
                    RelocSetToSymbol(rel, i, veneer_symbol);
                  }
                  else
                  {
                    Free(veneer);
                    veneer = StringConcat2 ("$Ven$TA$S$$", SYMBOL_NAME (to_sym));
                    veneer_symbol = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE (obj), veneer);
                    if (veneer_symbol)
                    {
                      RelocSetToSymbol(rel, i, veneer_symbol);
                    }
                  }
                }
		            Free (veneer);
	            }
	          }
	          /* }}} */

            if (!veneer_symbol && !stub_symbol)
	          {
              RelocSetToSymbol (rel, i, to_mapped);
	          }
          }
          RelocTableUnlinkReloc (OBJECT_RELOC_TABLE(tmp), rel);
          RelocTableLinkReloc (OBJECT_RELOC_TABLE(obj), rel);
        }
        else
        {
          VERBOSE(0, ("Verify: skipping @R\n", rel));
          reln = RELOC_NEXT(rel);
          RelocTableRemoveReloc (OBJECT_RELOC_TABLE(tmp), rel);
        }
      }
    }
  }
}

void
ObjectRelocateSubObjects (t_object * obj, t_bool verify)
{

  FixRelocsToLinkerCreatedSymbols(obj);

  RelocsMigrateToRelocatables (obj);

  RelocateCore (obj, verify);
}

void
RelocateCore (t_object * obj, t_bool verify)
{
  t_reloc *rel;

  for (rel = RELOC_TABLE_LAST(OBJECT_RELOC_TABLE(obj)); rel != NULL; rel = RELOC_PREV(rel))
  {
    t_address x, y;
    t_address ret;

    ASSERT((RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_SUBSECTION), ("While relocating, a reloc was encountered that was not from a subsection...\nWe do not know what to relocate!\n@R", rel));

    if (diabloobject_options.verbose_relocation)
    {
      if (RELOC_LABEL(rel))
        VERBOSE(0, ("Relocating reloc with label %s", RELOC_LABEL(rel)));
      else
        VERBOSE(0, ("Relocating reloc without a label"));

      VERBOSE(0, ("RELOC: @R\n", rel));
      VERBOSE(0, ("Before relocation it held   : @G", RelocGetData (rel)));
    }

    ret = StackExec (RELOC_CODE(rel), rel, NULL, SECTION_DATA(T_SECTION(RELOC_FROM(rel))), TRUE, 0, obj);

    if (AddressExtractUint32 (ret) != 0)
      FATAL(("Relocation failed!: the return value (i.e. the only value on top of the stack after the relocation program finished) was not 0 as it should be. This either means the field that needed to be relocated overflowed or there is a bug in the relocation program:\n@R",rel));

    if ((diabloobject_options.verbose_relocation)&&(!RELOC_LABEL(rel) || !((StringPatternMatch("ARM_COPY:*",RELOC_LABEL(rel)))  || StringPatternMatch("I386_COPY:*",RELOC_LABEL(rel)) || StringPatternMatch("PLTREL:*",RELOC_LABEL(rel)))))
    {
      VERBOSE(0, ("We relocated it as          : @G", RelocGetData (rel)));
      VERBOSE(0, ("The linker relocated this as: @G\n",
                  RelocGetDataFrom (rel, SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel))),
                                    AddressSub (AddressAdd (SECTION_CADDRESS (T_SECTION(RELOC_FROM(rel))),
                                                            RELOC_FROM_OFFSET(rel)
                                                           ),
                                                SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION (RELOC_FROM (rel))))
                                               )
                                   )
                 ));
    }

    if ((verify)&&(!RELOC_LABEL(rel) || !(StringPatternMatch("ARM_COPY:*",RELOC_LABEL(rel)) || StringPatternMatch("I386_COPY:*",RELOC_LABEL(rel)) || StringPatternMatch("PPC_COPY:*",RELOC_LABEL(rel)) ||StringPatternMatch("PLTREL:*",RELOC_LABEL(rel)))))
    {
      x = RelocGetData (rel);
      y = RelocGetDataFrom (rel, SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel))),
                            AddressSub (AddressAdd
                                        (SECTION_CADDRESS
                                         (T_SECTION(RELOC_FROM(rel))),
                                         RELOC_FROM_OFFSET(rel)),
                                        SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION
                                                                                (RELOC_FROM
                                                                                 (rel)))))
                           );
      if (!AddressIsEq (x, y))
      {
        /* ld messes up if .sbss is empty and there is padding before .bss */
        if (!RELOC_LABEL(rel)
            || (   (strcmp (RELOC_LABEL(rel), "__bss_start__"))
                && (strcmp (RELOC_LABEL(rel), "__preinit_array_end"))
                && (strcmp (RELOC_LABEL(rel), "_BASE_ADDRESS")) /* FIXME */
                && (strcmp (RELOC_LABEL(rel), "_etext")) /* FIXME */
                && (strcmp (RELOC_LABEL(rel), "main_thread")) /* FIXME */
                && (strcmp (RELOC_LABEL(rel), "_gpinfo")) /* FIXME */
                && (strcmp (RELOC_LABEL(rel), "_fpdata_size")) /* FIXME */
                && (strcmp (RELOC_LABEL(rel), "__preinit_array_start"))
                && (strcmp (RELOC_LABEL(rel), "__start___libc_atexit"))
                && (strcmp (RELOC_LABEL(rel), "__init_array_end"))
                && (strcmp (RELOC_LABEL(rel), "__fini_array_start"))
                && (strcmp (RELOC_LABEL(rel), "__fini_array_end"))
                && (strcmp (RELOC_LABEL(rel), "__init_array_start"))
                && (strcmp (RELOC_LABEL(rel), "__bss_start"))
                && (strcmp (RELOC_LABEL(rel), "_end_init"))
                && (strcmp (RELOC_LABEL(rel), "_end_bootmem_phys"))
                && (strcmp (RELOC_LABEL(rel), "_start_bootmem"))
                && (strcmp (RELOC_LABEL(rel), "_end_bootmem"))
                && (strcmp (RELOC_LABEL(rel), ".LC0")) /* Ye stupid fortran ! */
                && (strcmp (RELOC_LABEL(rel), "DIABLO_REMOVED_SYM"))
                && !StringPatternMatch("__*_from_thumb",RELOC_LABEL(rel))
                && !StringPatternMatch("DIABLO_POTENTIAL_THUMB_VENEER:*",RELOC_LABEL(rel))
                /* Some instructions can be optimized out (nop'ed) when linking */
                && !(RELOC_N_TO_RELOCATABLES (rel)
                     && RELOCATABLE_RELOCATABLE_TYPE (RELOC_TO_RELOCATABLE (rel)[0]) == RT_SECTION
                     && !strcmp (SECTION_NAME (T_SECTION (RELOC_TO_RELOCATABLE (rel)[0])), "*UNDEF*"))
               )
            )
            {
              t_address from_address;
              if (RELOC_FROM(rel) && (G_T_UINT32(from_address = AddressAdd(SECTION_CADDRESS(T_SECTION(RELOC_FROM(rel))),RELOC_FROM_OFFSET(rel))) & 0xffe) == 0xffe)
                {
                  WARNING(("WARNING: At address 0x%x, it seems a link-time patch for ARM erratum 657417 (A 32-bit branch instruction that spans two 4K regions and of which the target resides in the first region can result in an incorrect operation on Cortex-A8 processors) was applied."));
                  //                      FATAL(("\nFATAL:  Diablo does not yet support workarounds for ARM erratum 657417 (A 32-bit branch instruction that spans two 4K regions and of which the target resides in the first region can result in an incorrect operation on Cortex-A8 processors).\nFATAL:  So for the time being, Diablo's linker emulation cannot replicate binutils' ld workaround (of rewriting the branch instruction and inserting a veneer/trampoline.)\nFATAL:  Apparently, you are trying to rewrite a binary that features such a rewritten branch at address 0x%x, hence this error message and abort.\nFATAL:  You can disable that rewriting in ld by passing it the --no-fix-cortex-a8 flag.\nFATAL:  Beware, however, as the generated code might then not work correctly on Cortex-A8 processors.",G_T_UINT32(from_address))); 
                }
              else if ((x & 0xd000f800) == 0xc000f000 /* Diablo calculated a BLX (immediate) Thumb instruction... */
                       && (y & 0xd000f800) == 0xd000f000 /* ... but the original binary had a BL (immediate) Thumb instruction */) {
                /* This code is a fixup for a bug in binutils.
                 * The fix is only needed to be able to rewrite Android libraries (e.g., libwebrtc_audio_preprocessing.so), but the existence of this bug
                 * has already been confirmed at:
                 *  https://bugs.launchpad.net/binutils-linaro/+bug/641126
                 *  https://groups.google.com/forum/#!topic/android-ndk/HaLycHImqL8
                 *
                 * NOTE: In the above conditions, the lower and upper halfwords have changed places. This is because we are working on little-endian systems!
                 */
                WARNING(("Ignoring wrong relocation @R (Diablo: @G, original binary @G) because the linker was supposed to generate a BLX instruction, but a BL instruction was generated instead.", rel, x, y));
              }
              else {
                VERBOSE(0, ("error on relocation @R with code %s", rel, RELOC_CODE(rel)));
                FATAL(("Linker emulation failed. A relocated value differed after link emulation. There can be two reasons for this: either you are not using a patched ld (see the diablo website for details) or there is a bug in diablo. Implement %s: Diablo calculated @G, original binary has @G\nIf you think this is a bug, contact the diablo mailing list (diablo@elis.ugent.be).", RELOC_LABEL(rel), x, y));
              }
            }
      }
    }
  }
}

extern t_symbol *symbol_type_cached;
void
ObjectCreateDataOrCodeTable (t_object * obj)
{
  t_symbol *top, *he, *sym_it;

  symbol_type_cached = NULL;

  /* Sort the text symbols */
  SymbolTableSortSymbolsByAddress (OBJECT_SUB_SYMBOL_TABLE(obj), "$a");
  /* Compensate safety measures for $a, remove the doubles at the same address */
  top =
    (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$a");
  he = top;
  if (top)
    while (SYMBOL_EQUAL(top))
    {
      top = (t_symbol *) SYMBOL_EQUAL(top);
      if (AddressIsEq (StackExecConst(SYMBOL_CODE(top), NULL, top, 0, obj), StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj)))
      {
	SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(top), top);
        top = he;
      }
      else
        he = top;
    }

  /* Sort the thumb symbols if any */
  SymbolTableSortSymbolsByAddress (OBJECT_SUB_SYMBOL_TABLE(obj), "$t");
  /* Compensate safety measures for $t, remove the doubles at the same address */
  top =
    (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$t");
  he = top;
  if (top)
    while (SYMBOL_EQUAL(top))
    {
      top = (t_symbol *) SYMBOL_EQUAL(top);
      if (AddressIsEq (StackExecConst(SYMBOL_CODE(top), NULL, top, 0, obj), StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj)))
      {
	SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(top), top);
        top = he;
      }
      else
        he = top;
    }

  /* Sort the data symbols */
  SymbolTableSortSymbolsByAddress (OBJECT_SUB_SYMBOL_TABLE(obj), "$d");
  /* Compensate safety measures for $d */
  top =
    (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$d");
  he = top;
  if (top)
    while (SYMBOL_EQUAL(top))
    {
      top = (t_symbol *) SYMBOL_EQUAL(top);
      if (AddressIsEq (StackExecConst(SYMBOL_CODE(top), NULL, top, 0, obj), StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj)))
      {
	SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(top), top);
        top = he;
      }
      else
        he = top;
    }

  /* Add flags to $d, $a and $t symbols accordingly, we do this first and separate
   * because later function calls in here depend on it. */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym_it)
  {
      if (StringPatternMatch("$a.*", SYMBOL_NAME(sym_it))
              || StringPatternMatch("$a", SYMBOL_NAME(sym_it)))
      {
        SYMBOL_SET_FLAGS(sym_it, SYMBOL_TYPE_MARK_CODE);
      }
      else if (StringPatternMatch("$d.*", SYMBOL_NAME(sym_it))
              || StringPatternMatch("$d", SYMBOL_NAME(sym_it)))
      {
        SYMBOL_SET_FLAGS(sym_it, SYMBOL_TYPE_MARK_DATA);
      }
      else if (StringPatternMatch("$t.*", SYMBOL_NAME(sym_it))
              || StringPatternMatch("$t", SYMBOL_NAME(sym_it)))
      {
        SYMBOL_SET_FLAGS(sym_it, SYMBOL_TYPE_MARK_THUMB);
      }
  }

  /* in case the table is being recomputed, we need to remove the old $switch, $code_switch, $thumb
   * and $first symbols first */
  {
    t_symbol *s;
    while ((s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "$switch")))
      SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), s);
    while ((s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "$code_switch")))
      SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), s);
    while ((s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "$thumb")))
      SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), s);
    while ((s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "$first")))
      SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), s);
  }

  /* Create a new symbol called $switch = compressed $(a,t) -> $d */
  {
    t_bool prev_code = FALSE;
    t_bool first = TRUE;
    t_symbol *he = (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$d");
    t_symbol *he2 = (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$a");
    t_symbol *he3 =  (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj), "$t");

    while ((he) || (he2) || (he3))
    {

      if ((he && he2 && !he3 && AddressIsEq (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj))))
      {
        VERBOSE(1, ("(eq $d, $a : skip $d) he @S\nhe2 @S\n", he, he2));
        he = (t_symbol *) SYMBOL_EQUAL(he);
      }

      if ((he && he3 && AddressIsEq (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj))))
      {
        VERBOSE(1, ("(eq $d $t : skip $d) he @S\nhe2 @S\n", he, he3));
        he = (t_symbol *) SYMBOL_EQUAL(he);
      }


      if ((he && !he2 && !he3) || /* only $d left */
          (he && he2 && !he3      /* $d, $a left and $d < $a */
           && AddressIsLt (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj))) ||
          (he && !he2 && he3      /* $d, $t left and $d < $t */
           && AddressIsLt (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj))) ||
          (he && he2 && he3    /* $d, $t, $a left and $d < $t and $d < $a */
           && AddressIsLt (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj))
           && AddressIsLt (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj))))
      {
        /* DATA */
        if ((prev_code) || first)
        {
          if (SYMBOL_BASE(he)
              && (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_SECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_SUBSECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_INS))
          {
            if (first)
            {
              t_symbol *sym =
                SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                   "$first");

              if (!sym)
                SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$first", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)), 
                                              AddressNullForObject(obj) ,
                                              AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              else
                SYMBOL_SET_OFFSET_FROM_START(sym, AddressNullForObject(obj));
            }
	    SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)), 
                                          StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj),
					  AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
/*#define DEBUG_SWITCHTABLE*/
#ifdef DEBUG_SWITCHTABLE
            VERBOSE(1,
                    ("At @G there is data (@S)", 
                     StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), he));
#endif
          }
          else
            FATAL(("A symbol that marks data in code ($d) was encountered that is not associated to a section or a subsection\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain."));
        }
        he = (t_symbol *) SYMBOL_EQUAL(he);
#ifdef DEBUG_SWITCHTABLE
        if (he)
          VERBOSE(1,
                  ("At @G we have a $d symbol",
                   StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj)));
#endif
        prev_code = FALSE;
      }
      else if (he2
               && ((!he3)
                   || AddressIsLt (StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj),
                                   StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj))))
      {
        if ((!prev_code) || first)
        {
          if (SYMBOL_BASE(he2)
              && (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_SECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_SUBSECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_INS))
          {
            if (first)
            {
              t_symbol *sym =
                SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                   "$first");

              if (!sym)
                SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$first", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                      AddressAddUint32 (AddressNullForObject(obj), 1), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
              else
                SYMBOL_SET_OFFSET_FROM_START(sym, AddressAddUint32 (AddressNullForObject(obj), 1));
            }

            SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)), 
                                          StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj),
                                          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
#ifdef DEBUG_SWITCHTABLE
            VERBOSE(1,
                    ("At @G there is code (marked by %s)", 
                     StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj), SYMBOL_NAME(he2)));
#endif
          }
          else
            FATAL(("A symbol that marks data or code (%s) was encountered that is not associated to a section or a subsection\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain.", SYMBOL_NAME(he2)));
        }
        he2 = (t_symbol *) SYMBOL_EQUAL(he2);
#ifdef DEBUG_SWITCHTABLE
        if (he2)
          VERBOSE(1,
                  ("At @G we have a $a symbol",
                     StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj)));
#endif
        prev_code = TRUE;
      }
      else if (he3)
      {
        if ((!prev_code) || first)
        {
          if (SYMBOL_BASE(he3)
              && (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he3)) == RT_SECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he3)) == RT_SUBSECTION
                  || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he3)) == RT_INS))
          {
            if (first)
            {
              t_symbol *sym =
                SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                   "$first");

              if (!sym)
		      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$first", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                              AddressAddUint32 (AddressNullForObject(obj) , 1), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

              else
                SYMBOL_SET_OFFSET_FROM_START(sym, AddressAddUint32 (AddressNullForObject(obj) , 1));
            }
            SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                          StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
#ifdef DEBUG_SWITCHTABLE
            VERBOSE(1,
                    ("At @G there is code (marked by %s)",
                     StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj), SYMBOL_NAME(he3)));
#endif
          }
          else
            FATAL(("A symbol that marks data or code (%s) was encountered that is not associated to a section or a subsection\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain.", SYMBOL_NAME(he2)));
        }
        he3 = (t_symbol *) SYMBOL_EQUAL(he3);
#ifdef DEBUG_SWITCHTABLE
        if (he3)
          VERBOSE(1,
                  ("At @G we have a $t symbol", 
                     StackExecConst(SYMBOL_CODE(he3), NULL, he3, 0, obj)));
#endif
        prev_code = TRUE;
      }
      else
        FATAL(("A problem was encountered in processing the symbols that mark data or code (most likely a part of the program is marked both data and code\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain.!"));

      first = FALSE;
    }
  }
  /* Create new symbols called $code_switch, compressed $a -> $t */
  {
    t_bool prev_arm = TRUE, first = TRUE;
    t_symbol *he =
      (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                    "$t");
    t_symbol *he2 =
      (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                    "$a");

    if (he)
      while ((he) || (he2))
      {
        if ((he)
            && ((!he2)
                ||
                (AddressIsLt (StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj)))))
        {
          if ((prev_arm) || first)
          {
            if (SYMBOL_BASE(he)
                && (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_SECTION
                    || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_SUBSECTION
                    || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he)) == RT_INS))
            {
              if (first)
              {
                t_symbol *sym =
                  SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                     "$thumb");

                if (!sym)
                  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$thumb", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                        AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                else
                  SYMBOL_SET_OFFSET_FROM_START(sym, AddressNullForObject(obj) );
              }
	      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$code_switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                          StackExecConst(SYMBOL_CODE(he), NULL, he, 0, obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
            }
            else
              FATAL(("A symbol that marks data in code ($d) was encountered that is not associated to a section or a subsection\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain."));
          }
          he = (t_symbol *) SYMBOL_EQUAL(he);
          prev_arm = FALSE;
        }
        else
        {
          if ((!prev_arm) || first)
          {
            if (SYMBOL_BASE(he2)
                && (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_SECTION
                    || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_SUBSECTION
                    || RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(he2)) == RT_INS))
            {
              if (first)
              {
                t_symbol *sym =
                  SymbolTableGetFirstSymbolWithName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                                     "$thumb");

                if (!sym)
                  SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$thumb", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                                AddressAddUint32 (AddressNullForObject(obj), 1), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                else
                  SYMBOL_SET_OFFSET_FROM_START(sym, AddressAddUint32 (AddressNullForObject(obj) , 1));
              }

	      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), "$code_switch", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                                          StackExecConst(SYMBOL_CODE(he2), NULL, he2, 0, obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
            }
            else
              FATAL(("A symbol that marks data or code (%s) was encountered that is not associated to a section or a subsection\nThis probably indicates a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain.", SYMBOL_NAME(he2)));
          }
          he2 = (t_symbol *) SYMBOL_EQUAL(he2);
          prev_arm = TRUE;
        }
        first = FALSE;
      }
  }

  /* Sort the $switch symbols */
  SymbolTableSortSymbolsByAddress (OBJECT_SUB_SYMBOL_TABLE(obj), "$switch");
  /* Sort the $code_switch symbols */
  SymbolTableSortSymbolsByAddress (OBJECT_SUB_SYMBOL_TABLE(obj), "$code_switch");
}

static void
RemoveLinkerRemovedSections (t_uint32 * nsecs, t_section *** secs,
                             t_object * parent, t_object * obj)
{
  t_uint32 tel, tel2;
  t_section *del;
  t_bool no_local_ident_remove;

  ASSERT (OBJECT_MAP(parent)->handler, ("Map handler not set, could not determine linker!")); 

  no_local_ident_remove =
    !(strcmp (OBJECT_MAP(parent)->handler->name, "ADS_ARMLINK"));
  for (tel = 0; tel < *nsecs; tel++)
  {
    /* If it not mapped, it's removed by the linker */

    if (!(SECTION_IS_MAPPED((*secs)[tel])))
    {
      t_symbol *sym, *tmp;
      t_reloc *rel, *tmpr;
      t_section *ident = NULL;

      VERBOSE(1, ("Removing section %s:%s",OBJECT_NAME(obj),SECTION_NAME((*secs)[tel])));

      /* Shrink the array, remember the pointer in del {{{ */
      del = (*secs)[tel];

      for (tel2 = tel + 1; tel2 < *nsecs; tel2++)
        (*secs)[tel2 - 1] = (*secs)[tel2];
      (*nsecs)--;

      tel--;
      /* }}} */
      /* For common defined sections, see if we can find a common section. If
       * found keep the identical/common section it in ident, else ident=NULL
       * {{{ */

      if (SECTION_IS_COMMON_DEFINED(del))
      {
        t_object *tmp, *tmpv;

        for (tmp = OBJECT_MAPPED_FIRST(parent); tmp != NULL; tmp = OBJECT_NEXT(tmp))
        {
          for (tmpv = tmp; tmpv != NULL; tmpv = OBJECT_EQUAL(tmpv))
          {
            ident = SectionGetFromObjectByName (tmpv, SECTION_NAME(del));
            if ((ident) && (SECTION_IS_MAPPED(ident)))
              break;
          }
          if ((ident) && (SECTION_IS_MAPPED(ident)))
            break;
        }
        if (!((ident) && (SECTION_IS_MAPPED(ident))))
        {
          ident = NULL;
        }
      }

      /* }}} */
      /* Delete/Move relocs from/to deleted/identical sections {{{ */
      if (!OBJECT_RELOC_TABLE(obj))
        WARNING(("No reloc table yet. Check this!"));
      else
        OBJECT_FOREACH_RELOC_SAFE(obj, rel, tmpr)
        {
          if (T_SECTION(RELOC_FROM(rel)) == del)
          {
            t_uint32 nsyms=0;
            t_symbol ** syms=NULL;
            t_uint32 i;
            for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
            {
              if ((SYMBOL_RELOCREFCOUNT(RELOC_TO_SYMBOL(rel)[i])==1)&&(SYMBOL_BASE(RELOC_TO_SYMBOL(rel)[i])==T_RELOCATABLE(OBJECT_UNDEF_SECTION(parent))))
              {
                syms=Realloc(syms,(nsyms+1)*sizeof(t_symbol));
                syms[nsyms]=RELOC_TO_SYMBOL(rel)[i];
                nsyms++;
              }
            }

            RelocTableRemoveReloc (OBJECT_RELOC_TABLE(obj), rel);
            if (nsyms)
            {
              for (i=0; i<nsyms; i++)
              {
                if (SYMBOL_RELOCREFCOUNT(syms[i])==0)
                {
                  SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(syms[i]),syms[i]);
                }
                else
                {
                  Free(SYMBOL_CODE(syms[i]));
                  SYMBOL_SET_CODE(syms[i], StringDup("R00A00+$"));
                  SymbolSetBase(syms[i], T_RELOCATABLE(OBJECT_UNDEF_SECTION(parent)));
                  SYMBOL_SET_ORDER(syms[i], 1);
                  SYMBOL_SET_SEARCH(syms[i], FALSE);
                }
              }
            }
	    if (syms) Free(syms);
          }
          else 
          {
            t_uint32 i;
            for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
            {
              if (T_SECTION(SYMBOL_BASE(RELOC_TO_SYMBOL(rel)[i])) == del)
              {
		t_bool empty_begin = FALSE;
		VERBOSE(1, ("Removing symbol @S",RELOC_TO_SYMBOL(rel)[i]));
                if (ident && (no_local_ident_remove || empty_begin || (SYMBOL_ORDER(RELOC_TO_SYMBOL(rel)[i])>=0)))
                {
                  VERBOSE(1, (" -- Not removing symbol after all, rebasing on ident = @T",ident));
                  SymbolSetSymbolBaseAndUpdateRelocs (OBJECT_RELOC_TABLE(obj), RELOC_TO_SYMBOL(rel)[i], T_RELOCATABLE(ident));
                }
                else
                {
                  SymbolSetSymbolBaseAndUpdateRelocs (OBJECT_RELOC_TABLE(obj), RELOC_TO_SYMBOL(rel)[i], T_RELOCATABLE(OBJECT_UNDEF_SECTION(parent)));
		  Free(SYMBOL_CODE(RELOC_TO_SYMBOL(rel)[i]));
                  SYMBOL_SET_CODE(RELOC_TO_SYMBOL(rel)[i], StringDup("R00A00+$"));
                }
                SYMBOL_SET_SEARCH(RELOC_TO_SYMBOL(rel)[i], FALSE);
                if (SYMBOL_ORDER(RELOC_TO_SYMBOL(rel)[i])>0) SYMBOL_SET_ORDER(RELOC_TO_SYMBOL(rel)[i], 1);
                if (SYMBOL_DUP(RELOC_TO_SYMBOL(rel)[i])==FALSE) SYMBOL_SET_DUP(RELOC_TO_SYMBOL(rel)[i], PERHAPS);
              }
            }
          }
        }
      /* }}} */
      /* Delete all symbols pointing to the removed section {{{ */
      OBJECT_FOREACH_SYMBOL_SAFE(obj, sym, tmp)
      {
          if (SYMBOL_BASE(sym) && T_SECTION(SYMBOL_BASE(sym)) == del)
            SymbolTableRemoveSymbol (OBJECT_SYMBOL_TABLE(obj), sym);
      }
      /* }}} */
      /* Free the deleted section {{{ */
      SectionFree (del);
      /* }}} */
    }
  }
}

/*!
 *
 * \param parent the linked executable
 * \param obj the subobject
 *
 * If a part (section) of an object is not needed, certain linkers might remove
 * it from the objectfile. We remove them too to avoid confusion. This function
 * finds (and removes) the removed sections for one subobject. */

void
ObjectFindLinkerRemovedSections (t_object * parent, t_object * obj)
{
  t_section **sec;
  t_uint32 nsec;

  sec = OBJECT_CODE(obj);
  nsec = OBJECT_NCODES(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_CODE(obj, sec);
  OBJECT_SET_NCODES(obj, nsec);
  sec = OBJECT_DATA(obj);
  nsec = OBJECT_NDATAS(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_DATA(obj, sec);
  OBJECT_SET_NDATAS(obj, nsec);
  sec = OBJECT_TLSDATA(obj);
  nsec = OBJECT_NTLSDATAS(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_TLSDATA(obj, sec);
  OBJECT_SET_NTLSDATAS(obj, nsec);
  sec = OBJECT_RODATA(obj);
  nsec = OBJECT_NRODATAS(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_RODATA(obj, sec);
  OBJECT_SET_NRODATAS(obj, nsec);
  sec = OBJECT_BSS(obj);
  nsec = OBJECT_NBSSS(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_BSS(obj, sec);
  OBJECT_SET_NBSSS(obj, nsec);
  sec = OBJECT_TLSBSS(obj);
  nsec = OBJECT_NTLSBSSS(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_TLSBSS(obj, sec);
  OBJECT_SET_NTLSBSSS(obj, nsec);
  sec = OBJECT_NOTE(obj);
  nsec = OBJECT_NNOTES(obj);
  RemoveLinkerRemovedSections (&nsec, &sec, parent, obj);
  OBJECT_SET_NOTE(obj, sec);
  OBJECT_SET_NNOTES(obj, nsec);
}

/*!
 *
 * Verify that everything in the map is also in the parent object
 *
 * \param obj The object for which we want to verify the map
 *
 * \return void
 */

void
ObjectSwallowChildObjects (t_object * obj)
{
  MapMoveObjectsToParent (OBJECT_MAP(obj), obj);
}

/*!
 *
 * If a part (section) of an object is not needed, certain linkers might remove
 * it from the objectfile We remove them too to avoid confusion. This function
 * removes these sections
 *
 * \param obj the parent object
 *
 * \return void
 *
 */

void
ObjectRemoveLinkerRemovedSections (t_object * obj)
{
  t_object *tmp;
  t_object *tmpv;

  STATUS(START, ("Removing linker removed sections"));
  OBJECT_FOREACH_SUBOBJECT(obj, tmpv, tmp)
  {
    ObjectFindLinkerRemovedSections (obj, tmpv);
  }
  STATUS(STOP, ("Removing linker removed sections"));
}
/*}}}*/
/* {{{ Rebuild the data of an object's parent sections from the data of the subsections */
void
ObjectRebuildSectionsFromSubsections (t_object * obj)
{
  t_section *sec;
  int tel;

  /* put all non-bss sections in a single array for easier access */
  OBJECT_FOREACH_SECTION(obj, sec, tel)
  {
    t_section **subsecs;
    t_uint32 nsubsecs;
    t_uint32 tel2;
    void *newdata;

    /* skip disassembled code sections */
    if ((SECTION_TYPE(sec) != CODE_SECTION)
        && (SECTION_TYPE(sec) != RODATA_SECTION)
        && (SECTION_TYPE(sec) != DATA_SECTION)
        && (SECTION_TYPE(sec) != TLSDATA_SECTION))
      continue;

    newdata =
      Calloc (sizeof (char), AddressExtractUint32 (SECTION_CSIZE(sec)));
    /* check if the layout script specifies a specific filler for this
     * section */
    {
      t_layout_script *script;
      t_layout_rule *rule;
      script = OBJECT_LAYOUT_SCRIPT(obj);
      if (!script)
      {
        script = ObjectGetAndParseLayoutScript (obj);
        OBJECT_SET_LAYOUT_SCRIPT(obj, script);
      }
      rule = LayoutScriptGetRuleForSection(script, sec);
      if (rule && rule->u.secspec->filler_exp)
      {
        t_uint32 i, size;
        t_uint32 filler;
        char *fillchars = (char *) &filler;

        filler =
          (t_uint32) LayoutScriptCalcExp (rule->u.secspec->filler_exp,
                                          0LL /* this is just a dummy */, obj);
        size = AddressExtractUint32 (SECTION_CSIZE(sec));
        for (i = 0; i < size / 4; i++)
          ((t_uint32 *) newdata)[i] = filler;
        /* fill the last (size % 4) bytes */
        for (i = 0; i < size % 4; i++)
          ((char *) newdata)[4 * (size / 4) + i] = fillchars[i];
      }
    }

    subsecs = SectionGetSubsections (sec, &nsubsecs);
    for (tel2 = 0; tel2 < nsubsecs; tel2++)
    {
      t_uint32 prev_subsection_end = 0;
      t_section *subsec = subsecs[tel2];
      t_uint32 size = AddressExtractUint32 (SECTION_CSIZE(subsec));

      if (size)
      {
        t_uint32 offset =
          AddressExtractUint32 (AddressSub (SECTION_CADDRESS(subsec),
                                            SECTION_CADDRESS(sec)));
        if (prev_subsection_end > offset)
          FATAL(("When rebuilding the (output) section named %s from the (input) subsections we found subsections that overlap.\nThis probably indicates that you are using an unpatched toolchain (string section merging and exception handler compaction need to be turned of in the linker) or you have encountered a bug in the toolchain you are using. If you are using a supported toolchain contact the mailing list (diablo@@elis.ugent.be) and file a bug report. If not, you can contact the mailing list for help on how to fix your toolchain.", SECTION_NAME(sec)));

        if (offset + size >  AddressExtractUint32 (SECTION_CSIZE(sec)))
          FATAL(("When rebuilding the (output) section named %s at @G size @G from the (input) "
                 "subsections we found that section %s at @G size @G does not fit inside this section\n",
                 SECTION_NAME(sec), SECTION_CADDRESS(sec), SECTION_CSIZE(sec),
                 SECTION_NAME(subsec), SECTION_CADDRESS(subsec), SECTION_CSIZE(subsec)));

        /* Can happen if you merge bss into a data section */
        if (SECTION_DATA(subsec))
          memcpy (((char *) newdata) + offset,
                  SECTION_DATA(subsec), (size_t) size);
        else
          bzero(((char *) newdata) + offset, size);

        prev_subsection_end = offset + size;
      }
    }
    if (subsecs)
      Free (subsecs);

    if (SECTION_DATA(sec))
      Free (SECTION_DATA(sec));
    SECTION_SET_DATA(sec, newdata);
  }
}

/* }}} */
/* {{{ get and parse layout script for obj */
t_layout_script *
ObjectGetAndParseLayoutScript (t_object * obj)
{
  t_string scriptname = NULL;
  t_layout_script *ret;

  if (diabloobject_options.layoutscript)
  {
    /* layout script given at the command line */
    printf ("Using command line specified layout script %s\n",
            diabloobject_options.layoutscript);
    scriptname = diabloobject_options.layoutscript;
  }
  else
  {
    /* use the layout script embedded in the linker script */
    scriptname = GetLinkerScriptFilename (obj);
  }
  ASSERT(scriptname, ("Could not find an appropriate layout script"));

  ret = LayoutScriptParse (scriptname);
  Free (scriptname);

  return ret;
}

/* }}} */
/* Emulate the link {{{ */
/* LinkObjectsFromMap {{{ */

/** This function reads the map file of an executable object, loads all the
 * relocatable objects mentioned in it, and adds the loaded relocatable objects
 * to the subobject list of the executable object */

void
LinkObjectsFromMap (t_object * obj, t_bool read_debug)
{
  /* Read (and parse) the map file associated with this object file */
  ObjectReadMap (obj);

  /* Read all the objects referenced in the map file */
  ObjectReadObjectInMap (obj, read_debug);

  /* Check if the objects match the descriptions in the map file and swallows the children */
  ObjectSwallowChildObjects (obj);

  /* in case it's an alpha object we produce a litamap (starting (and verifying
   * the completeness of) the ordering found in the previous step) */

  ObjectMapPatchAlpha (obj);
}

/* }}} */
/* link in an extra file {{{ */
/** This function adds an extra file to a linked executable object.
 *
 * as a side effect of this, some sections will
 * be moved and possibly reordered (this is a
 * limitation of the current ObjectPlaceSections algorithm:
 * we have no absolute control over the final order
 * of the sections in the binary, in particular
 * sections that do not appear in the layout scripts)
 *
 * in order to adequately report addresses later on,
 * it might be necessary to build some kind of
 * address translation table here. this is something
 * that remains to be done.
 *
 * needed_libs is a NULL_terminated array of strings, each string being a name of a library
 * on which the linked in object depends. This argument can only be used for dynamically linked
 * binaries.
 *
 * */

#define BSS_TO_DATA

void
LinkObjectFileNew (t_object * obj, t_const_string linkin_fname, t_const_string symbol_prefix,
  t_bool read_debug, t_bool prefix_undefined_symbols, t_const_string const * needed_libs)
{
  linkin = ObjectGet (linkin_fname, obj, read_debug, NULL);
  t_section *entrysec = SectionGetFromObjectByAddress(obj, OBJECT_ENTRY(obj));
  t_address entry_offset;
  t_symbol *sym;
  t_uint32 tel;
  t_section *sec;
  t_address run = AddressNullForObject (obj);
  t_relocatable *undef = T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj));

  ASSERT(entrysec, ("Could not find the code section with the entry point!"));
  entry_offset = AddressSub(OBJECT_ENTRY(obj), SECTION_CADDRESS(entrysec));

#ifdef BSS_TO_DATA
  /* {{{ change all bss sections into data sections before linking in */
  if (OBJECT_NBSSS(linkin))
  {
    if (!OBJECT_NDATAS(linkin))
      OBJECT_SET_DATA(linkin, OBJECT_BSS(linkin));
    else
    {
      OBJECT_SET_DATA(linkin,
                      Realloc (OBJECT_DATA(linkin), sizeof (t_section *) *
                               (OBJECT_NDATAS(linkin) + OBJECT_NBSSS(linkin))));
      memcpy (OBJECT_DATA(linkin) + OBJECT_NDATAS(linkin),
              OBJECT_BSS(linkin), sizeof (t_section *) * OBJECT_NBSSS(linkin));
      Free (OBJECT_BSS(linkin));
    }
    OBJECT_SET_NDATAS(linkin, OBJECT_NDATAS(linkin) + OBJECT_NBSSS(linkin));
    OBJECT_SET_BSS(linkin, NULL);
    OBJECT_SET_NBSSS(linkin, 0);
    for (tel = 0; tel < OBJECT_NDATAS(linkin); tel++)
    {
      sec = OBJECT_DATA(linkin)[tel];
      SECTION_SET_TYPE(sec, DATA_SECTION);
      if (!SECTION_DATA(sec))
        SECTION_SET_DATA(sec,
                         Calloc (1, AddressExtractUint32 (SECTION_CSIZE(sec))));
    }
  } /* }}} */
#endif

  /* Add sections */
  OBJECT_FOREACH_SECTION(linkin, sec, tel)
  {
    t_section *parent;
    t_string sec_name;

    /* Don't add empty subsections */
    if(SECTION_CSIZE(sec) == 0)
      continue;

    VERBOSE(0, ("Linking in %s", SECTION_NAME(sec)));

    sec_name = StringConcat2 (symbol_prefix, SECTION_NAME(sec));
    parent = SectionCreateForObject (obj, SECTION_TYPE(sec), NULL, SECTION_CSIZE(sec), sec_name);
    SECTION_SET_FLAGS (parent, SECTION_FLAGS (parent) | SECTION_FLAG_LINKED_IN);
    Free(sec_name);

    /* copy section data to new parent section */
    if (SECTION_DATA(sec))
    {
      memcpy (SECTION_DATA(parent), SECTION_DATA(sec),
              AddressExtractUint32 (SECTION_CSIZE(sec)));
    }
    else
      SECTION_SET_DATA(parent, NULL);

    SECTION_SET_ALIGNMENT(parent, SECTION_ALIGNMENT(sec));
    SECTION_SET_CADDRESS(parent, run);
    SECTION_SET_OLD_ADDRESS(parent, run);
    SECTION_SET_CADDRESS(sec, run);
    SECTION_SET_OLD_ADDRESS(sec, run);

    MAP_SECTION(sec, NULL, parent);
    SECTION_SET_RELOCATABLE_TYPE(sec, RT_SUBSECTION);
    VERBOSE(1,
            ("linked in section %s (parent obj= %s) (current size = @G)\n",
             SECTION_NAME(parent), OBJECT_NAME(SECTION_OBJECT(parent)),
             SECTION_CSIZE(parent)));
    SECTION_SET_FLAGS (sec, SECTION_FLAGS (sec) | SECTION_FLAG_LINKED_IN);
  }

  /* Prefix all symbols that need to be prefixed. Also add some to the symbol table of the parent, so we can run linkerscripts on them. */
  OBJECT_SET_SYMBOL_TABLE(obj, SymbolTableNew (obj));
  OBJECT_FOREACH_SYMBOL(linkin, sym)
  {
    t_string name = SYMBOL_NAME(sym);

    /* Don't prefix symbols that aren't defined in (any of) the linked in object(s), like symbols from libc. We make that decision
     * by looking for symbols with an undefined base that do NOT start with the LINKIN_IDENTIFIER_PREFIX.
     */
    if((SYMBOL_BASE(sym) == undef) && strncmp(name, LINKIN_IDENTIFIER_PREFIX, strlen(LINKIN_IDENTIFIER_PREFIX)))
      continue;

    /* Don't prefix the symbols generated by the patched toolchain (starting with a '$'),
      and don't prefix SECTIONSYM: symbols. */
    if ((name[0] != '$') && strncmp(name, "SECTIONSYM:", strlen("SECTIONSYM:")))
    {
      if (prefix_undefined_symbols || (SYMBOL_ORDER(sym) != 0) || (SYMBOL_SEARCH(sym) != TRUE))
      {
        t_string tmp;
        char* colon_pos = strrchr(name, ':');

        /* If the string contains a ':' it has already been prefixed by diablo. We want to rewrite it so that it becomes
         * "ORIGINAL_PREFIX"+"LINKED_IN_PREFIX"+"ORIGINAL_SYMBOL_NAME"
         */
        if(colon_pos)
        {
          /* In case the prefixed symbol is related to a symbol not defined in any of the linked in objects, we shouldn't prefix it again */
          t_string orig_name = (char*) (colon_pos + 1);
          t_symbol* orig_sym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(linkin), orig_name);
          if((SYMBOL_BASE(orig_sym) == undef) && strncmp(orig_name, LINKIN_IDENTIFIER_PREFIX, strlen(LINKIN_IDENTIFIER_PREFIX)))
          {
            /* This kind of prefixed symbols should always be added to the symbol table as they are only generated if they're not yet present. */
            SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), SYMBOL_NAME(sym), SYMBOL_CODE(sym), SYMBOL_ORDER(sym), SYMBOL_DUP(sym), SYMBOL_SEARCH(sym), SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), SYMBOL_TENTATIVE(sym), SYMBOL_SIZE(sym), 0);
            continue;
          }

          *colon_pos = '\0';
          colon_pos++;
          tmp = StringConcat4(name, ":", symbol_prefix, colon_pos);
        }
        else
          tmp = StringConcat2 (symbol_prefix, SYMBOL_NAME(sym));

        SYMBOL_SET_NAME(sym, tmp);

        /* Some symbols also need to be added to the sub_symbol table to avoid problems later on, not sure why though.
         * Add them under their old AND their prefixed name. This is needed for linking between two objects linked in after linker emulation.
         * When adding symbols under their old unprefix name, set them as order 9 so they won't override any already existing
         * symbols of the same name.
         */
        SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), name, SYMBOL_CODE(sym), 9, SYMBOL_DUP(sym), SYMBOL_SEARCH(sym), SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), SYMBOL_TENTATIVE(sym), SYMBOL_SIZE(sym), 0);
        SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj), SYMBOL_NAME(sym), SYMBOL_CODE(sym), SYMBOL_ORDER(sym), SYMBOL_DUP(sym), SYMBOL_SEARCH(sym), SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), SYMBOL_TENTATIVE(sym), SYMBOL_SIZE(sym), 0);

        Free (name);
      }
    }
  }

  /* Pseudo-symbol to let the linkerscript know we're linking in after emulation */
  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$after_linker_emulation", "R00$", 10, PERHAPS, TRUE, undef, AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), AddressNullForObject(obj));

  /* Add all the dynamic libraries the linked in object depends on to the dependencies of the parent object */
  if(needed_libs)
  {
    tel = 0;
    while(needed_libs[tel])
    {
      DiabloBrokerCall ("AddNeededLib", obj, needed_libs[tel]);
      tel++;
    }
  }

  LinkMakeFinalObject(obj);
  LinkRelocate (obj, FALSE);

  /* The offset of the entrypoint within its parent section remains the same, but the CADDRESS of
   * the parent section might change. Therefore we recalculate the entrypoint.
   */
  OBJECT_SET_ENTRY(obj, AddressAdd(SECTION_CADDRESS(entrysec), entry_offset));
  return;
}

/* }}} */
/* LinkMakeFinalObject {{{ */
void
LinkMakeFinalObject (t_object * obj)
{
  /* Remove dead (sub)sections from the object (=linker removed for ARM, =
   * xdata, pdata for the alpha) */
  ObjectRemoveLinkerRemovedSections (obj);
  if (SectionGetFromObjectByName (obj, ".eh_frame"))
    DoEhFrame (obj, SectionGetFromObjectByName (obj, ".eh_frame"));
  /* Create a symbol- and reloc-table with the information from the subobjects */
  ObjectBuildUnifiedSymbolTableAndRelocTable (obj);

}

void
LinkRelocate (t_object * obj, t_bool verify)
{
  STATUS(START, ("Relocating"));

  if (OBJECT_MAPPED_FIRST(obj))
    ObjectRelocateSubObjects (obj, verify);

  /* in regular operation mode, this is some kind of a noop,
   * but in instrumentation mode, this is necessary because
   * some code has been linked in, so all addresses have
   * been changed */
  ObjectRebuildSectionsFromSubsections (obj);

  /*  TODO: We would like to remove the symbol table here, but doing so triggers
   *  a bug in the ppc backend */
  if (OBJECT_SYMBOL_TABLE(obj))
    SymbolTableFree (OBJECT_SYMBOL_TABLE(obj));
  OBJECT_SET_SYMBOL_TABLE(obj, NULL);

  STATUS(STOP, ("Relocating"));
}

/*}}}*/
/* LinkGetParent {{{ */

/** Stupid function
 *
 * \todo Remove! This should be in main!
 * */

t_object *
LinkGetParent (t_const_string objectfilename, t_bool read_debug)
{
  t_object *obj;

  /* Read the main object */
  obj = ObjectGet (objectfilename, NULL, read_debug, NULL);

  if (OBJECT_RELOC_TABLE(obj) && RELOC_TABLE_FIRST(OBJECT_RELOC_TABLE(obj)))
  {
    RelocTableFree (OBJECT_RELOC_TABLE(obj));
    OBJECT_SET_RELOC_TABLE(obj, RelocTableNew (obj));
  }
  return obj;
}

/* }}} */
/* LinkEmulate {{{ */

/** Create an object with relocation information by emulating the link of the object.
 *
 * \param name the name of the executable that is read and relinked
 */

t_object *
LinkEmulate (t_const_string name, t_bool read_debug)
{
  t_object *obj;

  STATUS(START, ("Link emulation for program %s", name));
  emulate_link = TRUE;
  obj = LinkGetParent (name, read_debug);

  LinkObjectsFromMap (obj, read_debug);

  LinkMakeFinalObject (obj);
  if (diabloobject_options.patch_ads)
  {
    t_symbol * sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj),".constdata___stdout_name");
    ((char *) AddressAddDispl(SECTION_DATA(T_SECTION(SYMBOL_BASE(sym))),SYMBOL_OFFSET_FROM_START(sym)))[0]='o' ;
    sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj),".constdata___stdin_name");
    ((char *) AddressAddDispl(SECTION_DATA(T_SECTION(SYMBOL_BASE(sym))),SYMBOL_OFFSET_FROM_START(sym)))[0]='i' ;
    sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj),".constdata___stderr_name");
    ((char *) AddressAddDispl(SECTION_DATA(T_SECTION(SYMBOL_BASE(sym))),SYMBOL_OFFSET_FROM_START(sym)))[0]='e' ;
  }

  LinkRelocate (obj, TRUE);
  emulate_link = FALSE;

  ObjectGetOverlays(obj);

  STATUS(STOP, ("Link emulation for program %s", name));
  return obj;
}

/* }}} */
/* }}} */

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
