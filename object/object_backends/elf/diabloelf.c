/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>
#include <diabloelf.h>

static int elf_module_usecount=0;

static void
ReviveDynamicSections(t_object *obj, t_graph *cfg, t_relocatablereviver reviver)
{
  t_object * lo = ObjectGetLinkerSubObject(obj);
  t_section * reladyn = SectionGetFromObjectByName(lo, "VECTOR___.rela.dyn");
  t_section * jcr = SectionGetFromObjectByName(lo, "VECTOR___.jcr");
  t_section * ctors = SectionGetFromObjectByName(lo, "VECTOR___.ctors");
  t_section * nulldyn = SectionGetFromObjectByName(lo, "NULLDYNSYM");
  t_section * nulldynstr = SectionGetFromObjectByName(lo, "NULLDYNSTR");
  t_section * nullgot = SectionGetFromObjectByName(lo, "NULLGOT");
  t_section * interp = SectionGetFromObjectByName(lo, ".interp");
  t_section * initplt = SectionGetFromObjectByName(lo, "INITPLT");
  t_section * nullplt = SectionGetFromObjectByName(lo, "NULLPLT");

  {
    t_section *sec;
    int i;

    OBJECT_FOREACH_SECTION(lo,sec,i)
    {
      /* don't kill:
       *  1) the .dynamic section and everything it points to
       *  2) .dyn.rel sections and what they point to (data locations present in the linked
       *       dynamic libraries that had to be copied into the binary)
       *  3) weak dynamic symbols (may have to be resolved in the linked dynamic libraries)
       *  4) versioning information
       */
      if (strncmp(SECTION_NAME(sec),".dynamic.",strlen(".dynamic."))==0 ||
          strncmp(SECTION_NAME(sec),"DYNREL:",strlen("DYNREL:"))==0 ||
          strncmp(SECTION_NAME(sec),".gnu.version",strlen(".gnu.version"))==0 ||
          strncmp(SECTION_NAME(sec),"DYNSYM:",strlen("DYNSYM:"))==0 ||
          strncmp(SECTION_NAME(sec),"PLTELEM:",strlen("PLTELEM:"))==0)
        (*reviver)(obj, cfg, T_RELOCATABLE(sec), TRUE);
    }

    if (diabloobject_options.keep_exidx)
      {
        OBJECT_FOREACH_SECTION(obj,sec,i)
          {
            /* in addition, keep extab and exidx section live */
            if (strncmp(SECTION_NAME(sec),".ARM.extab",strlen(".ARM.extab"))==0 || 
                strncmp(SECTION_NAME(sec),".ARM.exidx",strlen(".ARM.exidx"))==0)
              {
                (*reviver)(obj, cfg, T_RELOCATABLE(sec), TRUE);
              }
          }
      }
  }

  if (reladyn)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(reladyn), TRUE);
  }
  if (jcr)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(jcr), TRUE);
  }
  if (ctors)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(ctors), TRUE);
  }
  if (nulldyn)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(nulldyn), TRUE);
  }
  if (nulldynstr)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(nulldynstr), TRUE);
  }
  if (interp)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(interp), TRUE);
  }
  if (initplt)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(initplt), TRUE);
  }
  if (nullplt)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(nullplt), TRUE);
  }
  if (nullgot)
  {
    (*reviver)(obj, cfg, T_RELOCATABLE(nullgot), TRUE);
  }
}


static void
MarkExportSections(t_object *lo)
{
  t_section *sec;
  int i;

  OBJECT_FOREACH_SECTION(lo,sec,i)
  {
    if (strncmp(SECTION_NAME(sec),"DYNSYM:",strlen("DYNSYM:"))==0 ||
        strncmp(SECTION_NAME(sec),"DYNSYMSTR:",strlen("DYNSYMSTR:"))==0 ||
        strncmp(SECTION_NAME(sec),"DYNAMIC_BSSELEM:",strlen("DYNAMIC_BSSELEM:"))==0 ||
        strncmp(SECTION_NAME(sec),"PLTREL:",strlen("PLTREL:"))==0 ||
        strncmp(SECTION_NAME(sec),"PLTELEM:",strlen("PLTELEM:"))==0
        )
    {
      VERBOSE(4,("Marking @T as export",sec));
      SECTION_SET_FLAGS(sec,SECTION_FLAGS(sec)|SECTION_FLAG_EXPORT_SECTION|SECTION_FLAG_KEEP);
    }
  }
}

static void
AssociateWeakDynamicSymbolsWithData(t_object *obj)
{
  t_symbol *dynsymsym;
  t_object *lo;
  t_string origname;

  lo = ObjectGetLinkerSubObject(obj);
  VERBOSE(3,("Processing weak symbols in object %s",OBJECT_NAME(obj)));
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(obj),dynsymsym)
  {
    if (strncmp(SYMBOL_NAME(dynsymsym),"WEAKDYNSYMSYM:",strlen("WEAKDYNSYMSYM:"))==0)
    {
      t_symbol *orgnonweaksym, *newnonweaksym, *dynsym;
      t_address symaddr;
      t_string newnonweaksymname;

      VERBOSE(3,("weak sym @S, looking for %s",dynsymsym,SYMBOL_NAME(dynsymsym)+strlen("WEAKDYNSYMSYM:")));
      dynsym = SymbolTableLookup(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),SYMBOL_NAME(dynsymsym)+strlen("WEAKDYNSYMSYM:"));
      ASSERT(dynsym,("Could not find actual dynsym %s",SYMBOL_NAME(dynsymsym)+strlen("WEAKDYNSYMSYM:")));
      /* if it's an undefined weak symbol to be resolved by the linker, ignore */
      if (SYMBOL_BASE(dynsym)==T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)))
        continue;
      symaddr = StackExecConst(SYMBOL_CODE(dynsym), NULL, dynsym, 0, obj);
      VERBOSE(3,("Processing weak symbol @S, address == @G",dynsymsym,symaddr));
      orgnonweaksym=SymbolTableGetFirstSymbolByAddress(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),symaddr);
      if (orgnonweaksym)
        VERBOSE(3,(" First sym at same address: %s, weak: %d",SYMBOL_NAME(orgnonweaksym),SYMBOL_ORDER(orgnonweaksym)<=5));
      origname = NULL;
      /* this can either be a symbol with the same name as the original symbol,
       * or one prefixed with "ORIG:". See the comment above the DynSymOrig rule
       * in the linker script for more information
       */
      while (orgnonweaksym &&
          !SymbolTableLookup(OBJECT_SYMBOL_TABLE(lo),SYMBOL_NAME(orgnonweaksym)) &&
          !SymbolTableLookup(OBJECT_SYMBOL_TABLE(lo),((origname = StringConcat2("ORIG:",SYMBOL_NAME(orgnonweaksym))))))
      {
        Free(origname);
        origname = NULL;
        orgnonweaksym = SymbolTableGetNextSymbolByAddress(orgnonweaksym,symaddr);
        if (orgnonweaksym)
          VERBOSE(3,(" Next sym at same address: %s",SYMBOL_NAME(orgnonweaksym)));
      }
      if (origname)
        Free(origname);
      ASSERT(orgnonweaksym,("could not find non-weak symbol for @S",dynsym));
      ASSERT(!SYMBOL_SEARCH(orgnonweaksym),("could not find non-weak symbol for @S",dynsym));
      VERBOSE(3,("Found original one: @S",orgnonweaksym));

      newnonweaksym = SymbolTableLookup(OBJECT_SUB_SYMBOL_TABLE(obj),SYMBOL_NAME(orgnonweaksym));
      ASSERT(newnonweaksym,("Could not find dynbss symbol %s for weak symbol @S",SYMBOL_NAME(orgnonweaksym),dynsymsym));
      VERBOSE(3,("Found new BSS @S",newnonweaksym));

      SymbolTableAddSymbol (OBJECT_SUB_SYMBOL_TABLE(obj),SYMBOL_NAME(dynsym), "R00A00+$", 12, FALSE, FALSE,
          SYMBOL_BASE(newnonweaksym), SYMBOL_OFFSET_FROM_START(newnonweaksym),
          SYMBOL_ADDEND(newnonweaksym), NULL, AddressNullForObject(lo), 0);
    }
  }
}

void
ElfSymbolMaybeAddVersionAlias(t_object *obj, t_symbol_table *symbol_table, t_symbol *sym)
{
  t_string defaultversionedsymname;
  t_string atpos;
  t_string name;
  t_object *lo;

  lo = OBJECT_PARENT(obj)?OBJECT_PARENT(obj):obj;

  /* local symbols cannot need external resolution */
  if (SYMBOL_ORDER(sym)<0)
    return;

  /* don't add duplicate undefined references, that doesn't
   * help anyone
   */
  if (SYMBOL_BASE(sym) == T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)))
    return;

  /* not very clean, but there's no (easy?) way to only install this callback
   * when we are processing an ELF binary. Moreover, while parsing the initial
   * objects their handler isn't set yet (but then we call this function only
   * directly, so that's no problem; we only have to check this when it's
   * called as a callback)
   */
  if (OBJECT_OBJECT_HANDLER(obj) &&
      strcmp(OBJECT_OBJECT_HANDLER(obj)->main_name,"ELF"))
    return;

  name = SYMBOL_NAME(sym);
  /* is this subsymbol a versioned symbol? */
  atpos = strchr(name,'@');
  if (!atpos)
  {
    t_const_string realversionedsymname;
    /* no -> check whether a default version for this symbol was defined */
    defaultversionedsymname = StringConcat2(name,"@@DIABLO_DEFAULT_VERSION");
    VERBOSE(5,("Checking for %s",defaultversionedsymname));
    realversionedsymname = GetTranslatedSymbolName(defaultversionedsymname);
    if (realversionedsymname != defaultversionedsymname)
    {
      /* there is a default version -> it's possible that this unversioned
       * symbol was specified to become the default versioned symbol in the
       * version script -> add an alias of this symbol that has as symbol name the
       * default-versioned symbol name. We set its priority at the lowest level
       * so that if another object actually has a definition of this symbol (even
       * a weak one), it is picked over this one. */
      t_symbol *newsym;
      VERBOSE(5,("Adding symbol %s as alias for @S",realversionedsymname,sym));
      /* defaultversionedsym is defined with an aliased name: its key is the
       * xxx@@DIABLO_DEFAULT_VERSION string, but the actual symbol name is
       * xxx@@<actual_version> -> use SYMBOL_NAME(defaultversionedsym)
       */
      /* if there is already an alias, make sure this one overrides it (can happen
       * if an initial alias was added while processing te subobject, and another
       * gets added during the linker emulation)
       */
      newsym = SymbolTableDupSymbolWithOrder(symbol_table, sym, realversionedsymname, 1);
      SYMBOL_SET_ORDER(newsym,0);
      VERBOSE(5,("  -> result: @S",newsym));
    }
    Free(defaultversionedsymname);
  }
  else
  {
    /* yes, this is a versioned symbol -> check whether it's the default version */
    t_string basename;
    t_symbol *basesym;
    t_const_string realversionedsymname;

    basename = StringDup(name);
    basename[atpos-name]='\0';
    defaultversionedsymname = StringConcat2(basename,"@@DIABLO_DEFAULT_VERSION");
    VERBOSE(5,("Checking for 2 %s",defaultversionedsymname));
    realversionedsymname = GetTranslatedSymbolName(defaultversionedsymname);
    if (realversionedsymname != defaultversionedsymname)
      VERBOSE(5,(" -> found %s",realversionedsymname));
    /* default version? */
    if ((realversionedsymname != defaultversionedsymname) &&
        (strcmp(name,realversionedsymname)==0))
    {
      /* yes -> this symbol should normally be used, but in theory there
       * could be multiple ones (weak and non-weak ones) -> add an
       * unversioned alias with the same priority as this one + 20
       * -> it will override all other "normal" symbols, but relative
       * priority differences between multiple definitions of this symbol
       * will be kept
       */
      t_symbol *newsym;
      VERBOSE(5,("Adding symbol %s as alias for @S",basename,sym));
      /* same symbol -> map unversioned symbol to this address if otherwise
       * undefined
       */
      newsym = SymbolTableDupSymbolWithOrder(symbol_table, sym, basename, SYMBOL_ORDER(sym)+20);
      VERBOSE(5,("  -> result: @S",newsym));
    }
    Free(defaultversionedsymname);
    Free(basename);
  }
}

/* data structures for optimizing first and next symbol search by address */

typedef struct _t_symbol_he t_symbol_he;

struct _t_symbol_he
{
  t_hash_table_node node;
  t_symbol * sym1;
  t_symbol * sym2;
};

/* dynamic relative relocations instruct the dynamic linker to add the load
 * address of the library to value at a particular address in the library.
 * The problem is that no symbols are associated with them (so we don't
 * know what they refer to by parsing the relocations in the linked binary/
 * library), and when we encounter the absolute relocations in the object
 * files that cause them to be generated, we don't know which relative
 * relocation corresponds to them.
 *
 * The only thing we know is the absolute address to which the relative
 * relocation refers in the linked binary/library, so after the original
 * linker emulation we can see which absolute relocation was located at
 * that address connect connect them.
 */
/* AddDynRelativeRelocs{{{*/
static void
AddDynRelativeRelocs(t_object *obj)
{
  t_symbol *dynrelocsym;

  extern t_bool emulate_link;

  /* after emulating the original link, the added tentative rules
   * below will take care of everything
   */
  if (!emulate_link)
    return;

  /* hash table that stores symbols per address, and in addition to each
   * symbol also the last symbol at the same address with name "DYNABSRELOC"
   */
  t_hash_table * symbolht = SymbolTableCreateHashTableForGetFirst(OBJECT_SUB_SYMBOL_TABLE(obj),"DYNABSRELOC:");

  VERBOSE(3,("Hooking up dynamic relative relocations %s",OBJECT_NAME(obj)));
  /* walk over all dynamic relative and absolute relocations */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(obj),dynrelocsym)
  {
    if (strncmp(SYMBOL_NAME(dynrelocsym),"DYNRELATIVESOURCE:",strlen("DYNRELATIVESOURCE:"))==0 ||
        strncmp(SYMBOL_NAME(dynrelocsym),"DYNABSOLUTESOURCE:",strlen("DYNABSOLUTESOURCE:"))==0)
    {
      /* read the relocation number from the original relocation,
       * that way it's architecture independent (XXX NOT! 32/64 bit, rel/rela)
       */
      char const * const tentative_template =
        "DynAbs32 {\
          action { ADD_SUBSECTION(\"Linker\", \".rel.dyn\", CONCAT(\"DYNREL:\",SUBSTRING(MATCHED_NAME(),18,0)), RODATA, 4, 8) }\
          section {\
                    RELOCATED32(0x0, CONCAT(\"DYNABS32TARGET:\",SUBSTRING(MATCHED_NAME(),18,0)), 0, 0,0, \"S00\\l*w\\s0000$\"),\
                    %s\
                    %s\
          }\
          address { MATCHED_SYMBOL_VALUE() }\
         }\
         DynAbs32Symbol {\
           action {\
             ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
           }\
           symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"DYNREL:\",SUBSTRING(MATCHED_NAME(),18,0))) }\
         }";

      void *addr;
      t_address internaddr;
      t_symbol *absrelocsym, *dynreltargetsym, *origsym;
      t_string dynreltargetsymname;
      t_string tentative;
      t_string relinfo;
      t_symbol_he * node;

      /* the format is DYNXXXXXXXXADDR:hexaddroftarget (with XXXXXXXX either ABSOLUTE or RELATIVE) */
      if (sscanf(SYMBOL_NAME(dynrelocsym)+strlen("DYNXXXXXXXXSOURCE:"),"%p",&addr)!=1)
        FATAL(("Could not parse address of dyn relative/absolute symbol %s",SYMBOL_NAME(dynrelocsym)));
      internaddr = AddressNewForObject(obj,(unsigned long)addr);
      /* look for an absolute relocation symbol at this address */
      node = HashTableLookup(symbolht,&internaddr);

      ASSERT(node,("Couldn't find DYNABSRELOC:* symbol at @G to pair with @S",internaddr,dynrelocsym));
      absrelocsym = node->sym2;

      /* the dynamic relative/absolute relocation has to relocate the value at the
       * address corresponding to this DYNABSRELOC:* symbol. Since its name
       * is not necessarily unique, create a new name that is guaranteed
       * unique, so we can use it in a linker scripts (via a tentative symbol),
       * so we can execute it now to emulate the original link (since
       * the emulation of the original link happens before this routine is
       * called, we have to do it manually below)
       */

      /* create symbol with uni<ue name DYNABS32TARGET:hexaddroftarget */
      dynreltargetsymname = StringIo("DYNABS32TARGET:%s",SYMBOL_NAME(dynrelocsym)+strlen("DYNXXXXXXXXSOURCE:"));
      dynreltargetsym = SymbolTableDupSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), absrelocsym, dynreltargetsymname);
      Free(dynreltargetsymname);
      ASSERT(SYMBOL_TENTATIVE(dynrelocsym)==NULL,("dynrelocsym already has tentative? @S",dynrelocsym));
      /* associate the linker script rule with the original symbol */
      origsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),SYMBOL_NAME(absrelocsym)+strlen("DYNABSRELOC:"));
      /* for a dynamic ABS32 relocation, the dynamic linker needs to know which symbol
       * this is about -> reference its dynsym entry
       */
      if (strncmp(SYMBOL_NAME(dynrelocsym),"DYNABSOLUTESOURCE:",strlen("DYNABSOLUTESOURCE:"))==0)
      {
        relinfo=StringIo("RELOCATED32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4) & 0xff, CONCAT(\"ANYDYNSYMSYM:\",\"%s\"), 0, \"$dynsym_start\", 0, \"S00S01 - i00000004 > \\ i00000008 < l i000000ff & | w \\ s0000$\")\n",SYMBOL_NAME(absrelocsym)+strlen("DYNABSRELOC:"));
        /* in this case, the original ABS32 relocation must be removed */
      }
      else
        /* dynamic relative relocations only fixup the value with the load address
         * of the library. The only thing to keep is the relocation type (the byte
         * at MATCHED_SYMBOL_VALUE()+4; the rest is 0)
         */
        relinfo=StringDup("CONST32(READ_LINKED_VALUE32(MATCHED_SYMBOL_VALUE()+4))");
      /* if the referred symbol is defined in the binary, make sure it stays alive
       * by adding a fake relocation that refers it
       */
      if (origsym &&
          !SYMBOL_SEARCH(origsym))
      {
        t_string keepalive;
        keepalive = StringIo(",RELOCATED0(0x0, \"%s\", 0, 0, 0, \"S00\\*\\s0000$\")\n",SYMBOL_NAME(absrelocsym)+strlen("DYNABSRELOC:"));
        tentative = StringIo(tentative_template,relinfo,keepalive);
        Free(keepalive);
      }
      else
      {
        tentative = StringIo(tentative_template,relinfo,"");
      }
      Free(relinfo);
      /* finally set the tentative rule */
      SYMBOL_SET_TENTATIVE(dynrelocsym,StringDup(tentative));
      Free(tentative);
      /* execute the rule, since existing tentative symbols already have been resolved before this
       * routine was called
       */
      LinkerScriptParse (0, SYMBOL_TENTATIVE(dynrelocsym), obj, dynrelocsym);
    }
  }
  HashTableFree(symbolht);
}
/*}}}*/

static void
AddElfSpecificDataAndSymbols(t_object *obj)
{
  /* not very clean, but there's no (easy?) way to only install this callback
   * when we are processing an ELF binary
   */
  if (strcmp(OBJECT_OBJECT_HANDLER(obj)->main_name,"ELF"))
    return;
  AssociateWeakDynamicSymbolsWithData(obj);
  MarkExportSections(ObjectGetLinkerSubObject(obj));
  AddDynRelativeRelocs(obj);
}

/* Add a new DT_NEEDED library to an object. The obj should be an actual obj, not a subobj. */
void ElfAddNeededLib (t_object * obj, t_string libname)
{
  t_string name = StringConcat2("$dt_needed:", libname);
  t_relocatable* sec = T_RELOCATABLE(SectionGetFromObjectByName(obj, ".dynamic"));
  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, NO, NO, sec, RELOCATABLE_CSIZE(sec), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);
}

/* This function makes sure that fini_routine will be called on the end of the program. As this function
 * can result in an increase of the .fini_array or .dtors subsections it is advisible to call it after
 * linker emulation and before flowgraphing. This is because of possible issues later on during optimization.
 */
void
ElfAddFinalizationRoutine(t_object *obj, t_symbol *fini_routine_sym)
{
  /* We assume the fini_array or the dtors have been vectorized */
  t_object* linker_obj = ObjectGetLinkerSubObject(obj);
  t_section* fini_array_sec = SectionGetFromObjectByName(linker_obj, "VECTOR___.fini_array");
  t_section* dtors_sec = SectionGetFromObjectByName(linker_obj, "VECTOR___.dtors");

  /* We will install the new finalization routine by using the fini array or .dtors. The first is a section that
   * contains an array of function pointers. On finalization of the program these pointers are called in reverse
   * order. The .dtors section looks and functions much the same as the fini array, but it starts with a -1 element
   * and ends with 0 element. The pointers it contains are also called in forward instead of reverse order. We always
   * try to make sure the finalization routine is called as late as possible. For example for the fini array we will put
   * our new pointer as early on in the section as possible.
   */
  if (fini_array_sec || dtors_sec)
  {
    t_section* subsec = fini_array_sec?fini_array_sec:dtors_sec;
    t_address first = AddressNullForObject(obj);/* The offset of the new pointer inside the subsection */
    t_address last = SECTION_CSIZE(subsec);/* The offset of the last pointer inside the subsection, we'll place a new dynamic relative relocation here if needed */
    t_uint32 address_size = AddressSizeInBytes(OBJECT_ADDRESS_SIZE(obj));
    t_reloc_ref* rr;

    if (!AddressIsNull(SECTION_CSIZE(subsec)))
    {
      size_t orig_size = AddressExtractUint64(SECTION_CSIZE(subsec));
      SECTION_SET_DATA(subsec, Realloc(SECTION_DATA(subsec), orig_size + address_size));
      SECTION_SET_CSIZE(subsec, AddressAddUint32(SECTION_CSIZE(subsec), address_size));

      /* If the first address is a -1, we're dealing with android or .dtors. This value should stay first,
       * so we put our pointer on the second entry.
       */
      if (AddressIsMax(SectionGetDataAsAddress(subsec, AddressNullForObject(obj), 8 * address_size)))
      {
        /* Find the end (marked by a 0 element), this position will contain the new last element */
        t_uint32 iii = 1;
        while (!AddressIsNull(SectionGetDataAsAddress(subsec, AddressNewForObject(obj, address_size * iii), 8 * address_size)))
          iii++;

        /* Copy everything the rest of the subsection, starting from the 0 element, one step to the right (necessary for android) */
        last = AddressNewForObject(obj, address_size * iii);
        memmove (AddressAddDispl(SECTION_DATA(subsec), AddressAddUint32(last, address_size)), AddressAddDispl(SECTION_DATA(subsec), last), orig_size - AddressExtractUint64(last));

        /* For dtors the pointers are called in forward order so we'll place the new one last. For the fini
         * array they're called in reverse order so we put it first.
         */
        if(subsec == dtors_sec)
          first = last;
        else
          first = AddressAddUint32(first, address_size);
      }
    }
    else
      SECTION_SET_DATA(subsec, Malloc(address_size));

    /* If we place the new pointer at the beginning, we'll have the move all the subsequent pointers up one entry */
    if(subsec == fini_array_sec)
    {
      /* Increment the from_offset for all relocations from the subsection with the address size */
      rr = SECTION_REFERS_TO(subsec);
      while (rr)
      {
        t_reloc* reloc = RELOC_REF_RELOC(rr);

        RELOC_SET_FROM_OFFSET(reloc, AddressAddUint32(RELOC_FROM_OFFSET(reloc), address_size));

        /* Go to the next reloc_ref */
        rr = RELOC_REF_NEXT(rr);
      }
    }

    /* Create a new relocation so a pointer to the finalization routine will be written in the subsection */
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
      AddressNullForObject(obj),
      T_RELOCATABLE(subsec),
      first,
      SYMBOL_BASE(fini_routine_sym),
      SYMBOL_OFFSET_FROM_START(fini_routine_sym),
      FALSE,
      NULL,
      NULL,
      NULL,
      "R00A00+\\l*w\\s0000$");

    /* If we're dealing with PIC code we'll need to add a new dynamic relative relocation on the last entry */
    if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
      ElfAddDynamicRelativeRelocation(obj, T_RELOCATABLE(subsec), last);
  }
  else FATAL(("No fini array or .dtors into which we can install the new finalization routine is present in the object!"));
}

/* This function makes sure that init_routine will be called at the beginning of the program. As this function
 * can result in an increase of the .init_array or .ctors subsections it is advisible to call it after
 * linker emulation and before flowgraphing. This is because of possible issues later on during optimization.
 */
void
ElfAddInitializationRoutine(t_object *obj, t_symbol *init_routine_sym)
{
  /* We assume the init_array or the ctors have been vectorized */
  t_object* linker_obj = ObjectGetLinkerSubObject(obj);
  t_section* init_array_sec = SectionGetFromObjectByName(linker_obj, "VECTOR___.init_array");
  t_section* ctors_sec = SectionGetFromObjectByName(linker_obj, "VECTOR___.ctors");

  /* We will install the new initialization routine by using the init array or .ctors. The first is a section that
   * contains an array of functInitialization. On initialization of the program these pointers are called in forward 
   * order. The .ctors section looks and functions much the same as the init array, but it starts with a -1 element
   * and ends with 0 element. The pointers it contains are also called in reverse instead of forward order. We always
   * try to make sure the initialization routine is called as early as possible. For example for the init array we will put
   * our new pointer as early on in the section as possible.
   */
  if (init_array_sec || ctors_sec)
  {
    t_section* subsec = init_array_sec?init_array_sec:ctors_sec;
    t_address first = AddressNullForObject(obj);/* The offset of the new pointer inside the subsection */
    t_address last = SECTION_CSIZE(subsec);/* The offset of the last pointer inside the subsection, we'll place a new dynamic relative relocation here if needed */
    t_uint32 address_size = AddressSizeInBytes(OBJECT_ADDRESS_SIZE(obj));
    t_reloc_ref* rr;

    if (!AddressIsNull(SECTION_CSIZE(subsec)))
    {
      size_t orig_size = AddressExtractUint64(SECTION_CSIZE(subsec));
      SECTION_SET_DATA(subsec, Realloc(SECTION_DATA(subsec), orig_size + address_size));
      SECTION_SET_CSIZE(subsec, AddressAddUint32(SECTION_CSIZE(subsec), address_size));

      /* If the first address is a -1, we're dealing with android or .ctors. This value should stay first,
       * so we put our pointer on the second entry.
       */
      if (AddressIsMax(SectionGetDataAsAddress(subsec, AddressNullForObject(obj), 8 * address_size)))
      {
        /* Find the end (marked by a 0 element), this position will contain the new last element */
        t_uint32 iii = 1;
        while (!AddressIsNull(SectionGetDataAsAddress(subsec, AddressNewForObject(obj, address_size * iii), 8 * address_size)))
          iii++;

        /* Copy everything the rest of the subsection, starting from the 0 element, one step to the right (necessary for android) */
        last = AddressNewForObject(obj, address_size * iii);
        memmove (AddressAddDispl(SECTION_DATA(subsec), AddressAddUint32(last, address_size)), AddressAddDispl(SECTION_DATA(subsec), last), orig_size - AddressExtractUint64(last));

        /* For ctors the pointers are called in reverse order so we'll place the new one last. For the init
         * array they're called in forward order so we put it first.
         */
        if(subsec == ctors_sec)
          first = last;
        else
          first = AddressAddUint32(first, address_size);
      }
    }
    else
      SECTION_SET_DATA(subsec, Malloc(address_size));

    /* If we place the new pointer at the beginning, we'll have the move all the subsequent pointers up one entry */
    if(subsec == init_array_sec)
    {
      /* Increment the from_offset for all relocations from the subsection with the address size */
      rr = SECTION_REFERS_TO(subsec);
      while (rr)
      {
        t_reloc* reloc = RELOC_REF_RELOC(rr);

        RELOC_SET_FROM_OFFSET(reloc, AddressAddUint32(RELOC_FROM_OFFSET(reloc), address_size));

        /* Go to the next reloc_ref */
        rr = RELOC_REF_NEXT(rr);
      }
    }

    /* Create a new relocation so a pointer to the initialization routine will be written in the subsection */
    RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
      AddressNullForObject(obj),
      T_RELOCATABLE(subsec),
      first,
      SYMBOL_BASE(init_routine_sym),
      SYMBOL_OFFSET_FROM_START(init_routine_sym),
      FALSE,
      NULL,
      NULL,
      NULL,
      "R00A00+\\l*w\\s0000$");

    /* If we're dealing with PIC code we'll need to add a new dynamic relative relocation on the last entry */
    if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
      ElfAddDynamicRelativeRelocation(obj, T_RELOCATABLE(subsec), last);
  }
  else FATAL(("No init array or .ctors into which we can install the new initialization routine is present in the object!"));
}

void DiabloElfInit(int argc, char ** argv)
{
  VERBOSE(1, ("Initialized ELF Backend"));
  if (!elf_module_usecount)
  {
    ObjectHandlerAdd("ELF", NULL,   IsElf, NULL, NULL, NULL, NULL,NULL,NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#ifdef BIT32ADDRSUPPORT
    VERBOSE(1, ("Added ARM support"));
    ObjectHandlerAdd("ELF", "ARM",  IsElfArmSameEndianOnLsb,  ElfReadArmSameEndian,  ElfWriteArmSameEndian, ElfGetSizeofHeaders, ElfArmLinkBaseAddress, ElfArmAlignStartOfRelRO, ElfArmAlignGotAfterRelRO, ElfArmAlignDataAfterRelRO, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    ObjectHandlerAdd("ELF", "ARM",  IsElfArmSwitchedEndianOnLsb,  ElfReadArmSwitchedEndian, ElfWriteArmSwitchedEndian, ElfGetSizeofHeaders, ElfArmLinkBaseAddress, ElfArmAlignStartOfRelRO, ElfArmAlignGotAfterRelRO, ElfArmAlignDataAfterRelRO, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);

    VERBOSE(1, ("Added ARC support"));
    ObjectHandlerAdd("ELF", "ARC",  IsElfArcSameEndianOnLsb,  ElfReadArcSameEndian,  ElfWriteArcSameEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    ObjectHandlerAdd("ELF", "ARC",  IsElfArcSwitchedEndianOnLsb,  ElfReadArcSwitchedEndian, ElfWriteArcSwitchedEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);

    VERBOSE(1, ("Added i386 support"));
    ObjectHandlerAdd("ELF", "i386", IsElfI386SameEndianOnLsb, ElfReadI386SameEndian, ElfWriteI386SameEndian, ElfGetSizeofHeaders, NULL, ElfI386AlignStartOfRelRO, ElfI386AlignGotAfterRelRO, ElfI386AlignDataAfterRelRO, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    ObjectHandlerAdd("ELF", "i386", IsElfI386SwitchedEndianOnMsb, ElfReadI386SwitchedEndian, NULL, ElfGetSizeofHeaders, NULL, ElfI386AlignStartOfRelRO, ElfI386AlignGotAfterRelRO, ElfI386AlignDataAfterRelRO, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);

    VERBOSE(1, ("Added ppc support"));
    ObjectHandlerAdd("ELF", "ppc", IsElfPpcSameEndian, ElfReadPpcSameEndian, ElfWritePpcSameEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    ObjectHandlerAdd("ELF", "ppc", IsElfPpcSwitchedEndian, ElfReadPpcSwitchedEndian, ElfWritePpcSwitchedEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    LinkerScriptInstallCallback("PPC32_CHECK_PLT_SIZE", 0, "", "E", Ppc32CheckPltSize);

    VERBOSE(1, ("Added mips support"));
    ObjectHandlerAdd("ELF", "mips", IsElfMipsSameEndianOnLsb, ElfReadMipsSameEndian, NULL, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);

    VERBOSE(1, ("Added SPE support"));
    ObjectHandlerAdd("ELF", "SPE", IsElfSpeSameEndian, ElfReadSpeSameEndian, ElfWriteSpeSameEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);
    ObjectHandlerAdd("ELF", "SPE", IsElfSpeSwitchedEndian, ElfReadSpeSwitchedEndian, ElfWriteSpeSwitchedEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, (void * (*)(FILE *)) ElfRaw32Read, (void (*) (void *, FILE *)) ElfRaw32Write, (t_address (*)(void *, t_address)) ElfRaw32AddressToFileOffset, (t_address (*)(void *, t_address)) ElfRaw32FileOffsetToAddress, (char * (*) (void *, t_const_string , t_address)) ElfRaw32AddMarkerSection, (char * (*) (void *, t_address)) ElfRaw32AddressToSectionName, NULL);

    ObjectRegisterBuiltinObject("ELF", "SPE", "ovl_mgr", "spu_ovl.o");
    LinkerScriptInstallCallback("SPE_ADD_OVERLAY_STUBS", 0, "", "E", SpeAddOverlayStubs);
    LinkerScriptInstallCallback("ELF_RESIZE_HASH_SECTION", 0, "", "E", ElfResizeHashSection);
    LinkerScriptInstallCallback("ARM_MAYBE_ADD_FROM_ARM_TO_THUMB_STUB", 1, "S", "E", ArmMaybeAddFromArmToThumbStubSpaceAndSymbol);

    ArchiveHandlerAdd("COMPRESSED_ELF32", IsArchCompressedElf32, ArchCompressedElf32Open, ArchCompressedElf32GetObject, NULL, ArchCompressedElf32Close);
    CompressedMapHandlerAdd("COMPRESSED_ELF32", ObjectIsArchCompressedElf32, ArchCompressedElf32ReadMap);
#endif
#ifdef BIT64ADDRSUPPORT
    VERBOSE(1, ("Added amd64 support"));
    ObjectHandlerAdd("ELF", "amd64", IsElfAmd64SameEndianOnLsb, ElfReadAmd64SameEndian, ElfWriteAmd64SameEndian,ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    VERBOSE(1, ("Added ppc64 support"));
    ObjectHandlerAdd("ELF", "ppc64", IsElfPpc64SameEndian, ElfReadPpc64SameEndian, ElfWritePpc64SameEndian, ElfGetSizeofHeaders, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    LinkerScriptInstallCallback("PPC64_ADD_LINKER_STUBS", 0, "", "E", Ppc64ResolveTocsAndAddLinkerStubs);
#endif
    DiabloBrokerCallInstall("RevivePlatformSections", "t_object * obj, t_graph * cfg, void * reviver,", ReviveDynamicSections, TRUE);
    DiabloBrokerCallInstall("ObjectAddManualSpecificDataAndSymbols","t_object * obj,",AddElfSpecificDataAndSymbols, FALSE);
    DiabloBrokerCallInstall("?LinkerInsertedSymbol","t_object * obj, t_symbol_table *symbol_table, t_symbol * sym",ElfSymbolMaybeAddVersionAlias, FALSE);
    DiabloBrokerCallInstall("AddNeededLib","t_object * obj, t_string libname", ElfAddNeededLib, FALSE);
    DiabloBrokerCallInstall("AddDynamicRelativeRelocation", "t_object * obj, t_relocatable * from, t_address from_offset", ElfAddDynamicRelativeRelocation, FALSE);
    DiabloBrokerCallInstall("AddFinalizationRoutine", "t_object * obj, t_symbol * fini_routine_sym", ElfAddFinalizationRoutine, FALSE);
    DiabloBrokerCallInstall("AddInitializationRoutine", "t_object * obj, t_symbol * init_routine_sym", ElfAddInitializationRoutine, FALSE);
  }
  elf_module_usecount++;
}

void DiabloElfFini()
{
  elf_module_usecount--;
  if (!elf_module_usecount)
  {
  }

  ObjectDestroyBuiltinObject("ELF", "SPE", "ovl_mgr", "spu_ovl.o");
  LinkerScriptUninstallCallbacks();
}
