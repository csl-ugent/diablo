/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* #define VERBOSE_RELOCS */
/* #define DEBUG_RELOCS */

/* Includes {{{ */
#include <strings.h>
#include <diabloobject.h>
/*}}}*/

t_uint32 RelocGetNumberOfTouchedBytes(const t_reloc *rel)
{
  int sret = 0;
  int offset = 0;
  char *c;
  for (c = RELOC_CODE(rel); *c; ++c)
  {
    if (*c == 'l')
      offset += 4;
    else if (*c == 'L')
      offset += 8;
    else if (*c == 'k')
      offset += 2;
    else if (*c == 'w')
    {
      offset -= 4;
      if (offset + 4 > sret)
        sret = offset + 4;
    }
    else if (*c == 'W')
    {
      offset -= 8;
      if (offset + 8 > sret)
        sret = offset + 8;
    }
    else if (*c == 'v')
    {
      offset -= 2;
      if (offset + 2 > sret)
        sret = offset + 2;
    }
    /* TODO: 'e' */
  }

  return (t_uint32) sret;
}

t_address
RelocGetDataFrom (const t_reloc *rel, const t_section *sec, t_address addr)
{
  t_address res;
  t_address size;

  if (SECTION_TYPE(sec) == BSS_SECTION)
    res = AddressNullForSection (sec);
  else
  {
    size = 8*RelocGetNumberOfTouchedBytes(rel);
    if (size)
      res = SectionGetDataAsAddress (sec, addr, size);
    else
      res = AddressNullForSection (sec);
  }
  return res;
}

int 
RelocCmp(const t_reloc * rel1, const t_reloc * rel2, t_bool position_dependent)
{
  if (strcmp(RELOC_CODE(rel1), RELOC_CODE(rel2))==0)
  {
    if (position_dependent && (strchr(RELOC_CODE(rel1), 'P') != NULL))
    {
      /* if the relocation's computation is based on the relocated object's
       * location, make sure both relocations have the same FROM value */
      if (RELOC_FROM(rel1) != RELOC_FROM(rel2) ||
          !AddressIsEq(RELOC_FROM_OFFSET(rel1), RELOC_FROM_OFFSET(rel2)))
        return 1;
    }

    t_uint32 nrel1 = RELOC_N_TO_RELOCATABLES(rel1);
    t_uint32 nrel2 = RELOC_N_TO_RELOCATABLES(rel2);
    if ((nrel1 == nrel2) && (nrel1 != 0) && (RELOC_N_ADDENDS(rel1) == RELOC_N_ADDENDS(rel2)))
    {
      if (0 == memcmp(RELOC_TO_RELOCATABLE(rel1), RELOC_TO_RELOCATABLE(rel2), sizeof(t_relocatable *) * nrel1))
      {
        t_uint32 i;
        for (i = 0; i< nrel1; i++)
        {
          if (!AddressIsEq(RELOC_TO_RELOCATABLE_OFFSET(rel1)[i], (RELOC_TO_RELOCATABLE_OFFSET(rel2)[i])))
            return 1;
        }
        for (i = 0; i< RELOC_N_ADDENDS(rel1); i++)
        {
          if (!AddressIsEq(RELOC_ADDENDS(rel1)[i], (RELOC_ADDENDS(rel2)[i])))
            return 1;
        }
        return 0;
      }
    }
  }
  return 2;
}

void
RelocTableSetCallbacks (t_reloc_table * table, void (*AddEdgeCallback) (t_reloc_table *, t_reloc *, void *),
                        void (*DelEdgeCallback) (t_reloc_table *, t_reloc *, void *),
                        void (*DelSwitchEdgeCallback) (t_reloc_table *, t_reloc *, void *))
{
  RELOC_TABLE_SET_ADD_EDGE_CALLBACK(table, AddEdgeCallback);
  RELOC_TABLE_SET_DEL_EDGE_CALLBACK(table, DelEdgeCallback);
  RELOC_TABLE_SET_DEL_SWITCH_EDGE_CALLBACK(table, DelSwitchEdgeCallback);
}


static void
RelocUtilAddRefs (t_relocatable * from, t_relocatable * to, t_reloc * rel,t_bool sym, t_uint32 i)
{
  t_reloc_ref *tmp;

  /* The refs */
  if (from)
  {
    tmp = RelocRefNew (rel);
    RELOC_REF_SET_PREV(tmp, NULL);
    RELOC_REF_SET_NEXT(tmp, RELOCATABLE_REFERS_TO(from));
    if (RELOCATABLE_REFERS_TO(from))
      RELOC_REF_SET_PREV(RELOCATABLE_REFERS_TO(from), tmp);
    RELOCATABLE_SET_REFERS_TO(from, tmp);
    RELOC_SET_FROM_REF(rel, tmp);
  }
  /* the refed by */
  if (to)
  { 
    tmp = RelocRefNew (rel);
    RELOC_REF_SET_PREV(tmp, NULL);
    RELOC_REF_SET_NEXT(tmp, RELOCATABLE_REFED_BY(to));
    if (RELOCATABLE_REFED_BY(to))
      RELOC_REF_SET_PREV(RELOCATABLE_REFED_BY(to), tmp);
    RELOCATABLE_SET_REFED_BY(to, tmp);
    if (sym)
    {
      RELOC_TO_SYMBOL_REF(rel)[i] = tmp;
      SYMBOL_SET_RELOCREFCOUNT(RELOC_TO_SYMBOL(rel)[i], SYMBOL_RELOCREFCOUNT(RELOC_TO_SYMBOL(rel)[i]) + 1);
    }
    else
      RELOC_TO_RELOCATABLE_REF(rel)[i]=tmp;
  }
}

/* RelocTable Constructors/Destructors {{{*/
/*! Allocate a new relocation table */
t_reloc_table *
RelocTableNew (t_object * obj)
{
  t_reloc_table *ret = Malloc (sizeof (t_reloc_table));

  RELOC_TABLE_SET_FIRST(ret, NULL); 
  RELOC_TABLE_SET_LAST(ret, NULL);
  RELOC_TABLE_SET_NRELOCS(ret, 0);
  RELOC_TABLE_SET_OBJECT(ret, obj);
  RELOC_TABLE_SET_ADD_EDGE_CALLBACK(ret, NULL);
  RELOC_TABLE_SET_DEL_EDGE_CALLBACK(ret, NULL);
  RELOC_TABLE_SET_DEL_SWITCH_EDGE_CALLBACK(ret, NULL);
  return ret;
}

/*! Free a relocation table */
void
RelocTableFree (const t_reloc_table * tofree)
{
  t_reloc *next, *rel = RELOC_TABLE_FIRST(tofree);

  while (rel)
  {
    next = RELOC_NEXT(rel);
    RelocTableRemoveReloc ((t_reloc_table *) tofree, rel);
    rel = next;
  }
  Free (tofree);
}

/*}}}*/

void
RelocTableRemoveConstantRelocs(t_reloc_table * table)
{
  t_reloc *rel, * remove;
  t_uint32 i;

  STATUS(START, ("Removing constant relocs"));
  for (rel = RELOC_TABLE_FIRST(table); rel != NULL; )
  {
    t_bool found = FALSE;
    /* As long as we have symbols, constant value might change if more symbols
     * are added */

    if (RELOC_N_TO_SYMBOLS(rel)) {rel = RELOC_NEXT(rel); continue;}

    /* Check code for address operations: 'P' */

    if (StringPatternMatch("*P*",RELOC_CODE(rel))) {rel = RELOC_NEXT(rel); continue;}
    if (StringPatternMatch("*g*",RELOC_CODE(rel))) {rel = RELOC_NEXT(rel); continue;}
    if (StringPatternMatch("A00\\*",RELOC_CODE(rel))) {rel = RELOC_NEXT(rel); continue;}
    
    for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
    {
      t_relocatable * to = RELOC_TO_RELOCATABLE(rel)[i];

      if (to == T_RELOCATABLE(OBJECT_UNDEF_SECTION(RELOC_TABLE_OBJECT(table)))) continue;
      if (to == T_RELOCATABLE(OBJECT_ABS_SECTION(RELOC_TABLE_OBJECT(table)))) continue;

      found = TRUE;
      break;
    }

    if (found) {rel = RELOC_NEXT(rel); continue;}

    remove = rel;
    rel = RELOC_NEXT(rel);
    RelocTableRemoveReloc(table, remove);
    if (!rel) break;
  }
  STATUS(STOP, ("Removing constant relocs"));
}


/* RelocTablePrint {{{ */
void
RelocTablePrint (const t_reloc_table * table)
{
  t_reloc *rel;

  printf ("RELOC TABLE:\n");
  for (rel = RELOC_TABLE_FIRST(table); rel != NULL; rel = RELOC_NEXT(rel))
  {
    VERBOSE(0, ("@R\n", rel));
  }
}

/* }}} */
/* RelocTableVerify {{{ */
void
RelocTableVerify (const t_object * obj, const t_reloc_table * table, t_bool hard)
{
  t_reloc *relptr;
  t_relocatable *to;

  printf ("VERIFY START for %s\n", OBJECT_NAME(obj));
  for (relptr = RELOC_TABLE_FIRST(table); relptr != NULL; relptr = RELOC_NEXT(relptr))
  {
    t_uint32 i;
    t_reloc_ref *ref = RELOCATABLE_REFERS_TO(RELOC_FROM(relptr)), *prev;

    while (ref != NULL && RELOC_REF_RELOC(ref) != relptr)
    {
      prev = ref;
      ref = RELOC_REF_NEXT(ref);
    }

    if (!ref)
      FATAL(("VERIFY: Relocations corrupt! Could not find ref on from for reloc @R\n", relptr));


    for (i=0; i<RELOC_N_TO_SYMBOLS(relptr); i++)
    {
      to = SYMBOL_BASE(RELOC_TO_SYMBOL(relptr)[i]);
      
      if (!to)
        continue;

      ref = RELOCATABLE_REFED_BY(to);

      while (ref != NULL && RELOC_REF_RELOC(ref) != relptr)
      {
        prev = ref;
        ref = RELOC_REF_NEXT(ref);
      }

      if (!ref)
        FATAL(("VERIFY: Relocations corrupt! Could not find ref on to for reloc @R %p\n", relptr, relptr));
    }

    for (i=0; i<RELOC_N_TO_RELOCATABLES(relptr); i++)
    {
      to = RELOC_TO_RELOCATABLE(relptr)[i];
      
      if (!to)
        continue;

      ref = RELOCATABLE_REFED_BY(to);

      while (ref != NULL && RELOC_REF_RELOC(ref) != relptr)
      {
        prev = ref;
        ref = RELOC_REF_NEXT(ref);
      }

      if (!ref)
        FATAL(("VERIFY: Relocations corrupt! Could not find ref on to for reloc @R\n", relptr));
    }
  }
  printf ("VERIFY END\n");
}

/* }}} */
/* RelocTableAddRelocToSymbol {{{ */
t_reloc *
RelocTableAddRelocToSymbol (t_reloc_table * table,
                            t_address addend,
                            t_relocatable * from,
                            t_address from_offset,
                            t_symbol * to,
                            t_bool hell,
                            void *edge,
                            t_reloc * corresp,
                            t_symbol * e_sym, t_const_string code)
{
  t_address null = AddressSub(from_offset, from_offset);
  t_reloc *rel = Calloc (1, sizeof (t_reloc));
  RELOC_SET_SWITCH_EDGE(rel, NULL);
 
  ASSERT(from, ("The from of a relocation cannot be NULL!"));

  RELOC_SET_N_ADDENDS(rel, 1);
  RELOC_SET_ADDENDS(rel, Calloc(1,sizeof(t_address)));
  RELOC_ADDENDS(rel)[0] = addend;
  RELOC_SET_FROM(rel, from);
  RELOC_SET_FROM_OFFSET(rel, from_offset);
  RELOC_SET_TO_SYMBOL(rel, Calloc(1, sizeof(t_symbol *)*(e_sym?2:1)));
  RELOC_SET_TO_SYMBOL_OFFSET(rel, Calloc(1,sizeof(t_address)*(e_sym?2:1)));
  RELOC_SET_TO_SYMBOL_REF(rel, Calloc(1,sizeof(t_reloc_ref*)*(e_sym?2:1)));
  RELOC_SET_N_TO_SYMBOLS(rel, (e_sym?1:0) + (to?1:0));
  if (to)
  {
    RELOC_TO_SYMBOL(rel)[0] = to;
    RELOC_TO_SYMBOL_OFFSET(rel)[0] = null;
  }
  RELOC_SET_HELL(rel, hell);

  if (e_sym) 
  { 
    RELOC_TO_SYMBOL(rel)[to?1:0] = e_sym;
    RELOC_TO_SYMBOL_OFFSET(rel)[to?1:0] = null;
    if (to)
    ASSERT(StringPatternMatch("*S01*", code)||StringPatternMatch("*Z01*", code)||StringPatternMatch("*u01*", code), ("Use of extra symbol, not referenced in the reloc code"));
    else 
    ASSERT(StringPatternMatch("*S00*", code)||StringPatternMatch("*Z00*", code)||StringPatternMatch("*u00*", code), ("Use of extra symbol, not referenced in the reloc code"));
  }
  
  RELOC_SET_CODE(rel, StringDup (code));
  RELOC_SET_LABEL(rel, NULL);

  RelocTableLinkReloc(table, rel);

#ifdef VERBOSE_RELOCS
  VERBOSE(0, ("Added symbol reloc %d: @R\n", RELOC_TABLE_NRELOCS(table), rel));
#endif

  RELOC_SET_EDGE(rel, edge);
  if (edge)
    RELOC_TABLE_ADD_EDGE_CALLBACK(table) (table, rel, edge);

  if (to)
  {
    RelocUtilAddRefs (from, SYMBOL_BASE(to), rel, TRUE, 0);
  }
  else
  {
    RelocUtilAddRefs (from, NULL, rel, TRUE, 0);
  }
  if (e_sym)
  {
    RelocUtilAddRefs (NULL, SYMBOL_BASE(e_sym), rel, TRUE, 1);
  }

  return rel;
}

/* }}} */
/*RelocTableAddRelocToRelocatable {{{ */
t_reloc *
RelocTableAddRelocToRelocatable (t_reloc_table * table,
                                 t_address addend,
                                 t_relocatable * from,
                                 t_address from_offset,
                                 t_relocatable * to,
                                 t_address to_offset,
                                 t_bool hell,
                                 void *edge,
                                 t_reloc * corresp,
                                 t_relocatable * sec,
                                 t_const_string code)
{
  t_uint32 tcnt = 0;
  t_reloc *rel = Calloc (1, sizeof (t_reloc));
  RELOC_SET_SWITCH_EDGE(rel, NULL);

  RELOC_SET_N_ADDENDS(rel, 1);
  RELOC_SET_ADDENDS(rel, Calloc(1,sizeof(t_address)));
  RELOC_ADDENDS(rel)[0] = addend;
  RELOC_SET_FROM(rel, from);
  RELOC_SET_FROM_OFFSET(rel, from_offset);
  RELOC_SET_TO_RELOCATABLE(rel, Calloc(1,sizeof(t_relocatable *)*(sec?2:1)));
  RELOC_SET_TO_RELOCATABLE_OFFSET(rel, Calloc(1,sizeof(t_address)*(sec?2:1)));
  RELOC_SET_TO_RELOCATABLE_REF(rel, Calloc(1,sizeof(t_reloc_ref *)*(sec?2:1)));
  RELOC_SET_N_TO_RELOCATABLES(rel, (sec?1:0)+(to?1:0));

  if (to)
  {
    RELOC_TO_RELOCATABLE(rel)[tcnt] = to;
    RELOC_TO_RELOCATABLE_OFFSET(rel)[tcnt] = to_offset;
    tcnt++;
  }
  
  RELOC_SET_HELL(rel, hell);
  
  if (sec)
  {
    RELOC_TO_RELOCATABLE(rel)[tcnt] = sec;
    RELOC_TO_RELOCATABLE_OFFSET(rel)[tcnt] = AddressSub(to_offset,to_offset);
    ASSERT(StringPatternMatch("*R01*", code), ("Use of extra relocatable, not referenced in the reloc code"));
    tcnt++;
  }
  
  RELOC_SET_CODE(rel, StringDup (code));
  RELOC_SET_LABEL(rel, NULL);

  RelocTableLinkReloc(table, rel);

#ifdef VERBOSE_RELOCS
  VERBOSE(0, ("Added relocatable reloc %d: @R\n", RELOC_TABLE_NRELOCS(table), rel));
#endif

  RELOC_SET_EDGE(rel, edge);
  if (edge)
    RELOC_TABLE_ADD_EDGE_CALLBACK(table) (table, rel, edge);

  RelocUtilAddRefs (from, to, rel, FALSE, 0);
  if (sec)
    RelocUtilAddRefs (NULL, sec, rel, FALSE, 1);

  return rel;
}

/*}}}*/

void RelocAddRelocatable(t_reloc * rel, t_relocatable * relocatable, t_address offset)
{
    t_uint32 i=RELOC_N_TO_RELOCATABLES(rel);
   
    RELOC_SET_N_TO_RELOCATABLES(rel, RELOC_N_TO_RELOCATABLES(rel) + 1);
    RELOC_SET_TO_RELOCATABLE_OFFSET(rel, Realloc(RELOC_TO_RELOCATABLE_OFFSET(rel),sizeof(t_address)*RELOC_N_TO_RELOCATABLES(rel)));
    RELOC_SET_TO_RELOCATABLE_REF(rel, Realloc(RELOC_TO_RELOCATABLE_REF(rel),sizeof(t_reloc_ref *)*RELOC_N_TO_RELOCATABLES(rel)));
    RELOC_SET_TO_RELOCATABLE(rel, Realloc(RELOC_TO_RELOCATABLE(rel),sizeof(t_relocatable *)*RELOC_N_TO_RELOCATABLES(rel)));

    RELOC_TO_RELOCATABLE(rel)[i]=relocatable;
    RelocUtilAddRefs (NULL, RELOC_TO_RELOCATABLE(rel)[i], rel, FALSE, i);
    RELOC_TO_RELOCATABLE_OFFSET(rel)[i]=offset;
}

void RelocAddSymbol(t_reloc * rel, t_symbol * symbol, t_address offset)
{
    t_uint32 i=RELOC_N_TO_SYMBOLS(rel);
   
    RELOC_SET_N_TO_SYMBOLS(rel, RELOC_N_TO_SYMBOLS(rel) + 1);
    RELOC_SET_TO_SYMBOL_OFFSET(rel, Realloc(RELOC_TO_SYMBOL_OFFSET(rel),sizeof(t_address)*RELOC_N_TO_SYMBOLS(rel)));
    RELOC_SET_TO_SYMBOL_REF(rel, Realloc(RELOC_TO_SYMBOL_REF(rel),sizeof(t_reloc_ref *)*RELOC_N_TO_SYMBOLS(rel)));
    RELOC_SET_TO_SYMBOL(rel, Realloc(RELOC_TO_SYMBOL(rel),sizeof(t_relocatable *)*RELOC_N_TO_SYMBOLS(rel)));

    RELOC_TO_SYMBOL(rel)[i]=symbol;
    RelocUtilAddRefs (NULL, SYMBOL_BASE(RELOC_TO_SYMBOL(rel)[i]), rel, TRUE, i);
    RELOC_TO_SYMBOL_OFFSET(rel)[i]=offset;
}

void RelocAddAddend (t_reloc *rel, t_address addend)
{
  t_uint32 i = RELOC_N_ADDENDS (rel);
  t_address *addends = Realloc (RELOC_ADDENDS (rel), (i+1)*sizeof(t_address));
  addends[i] = addend;
  RELOC_SET_ADDENDS (rel, addends);
  RELOC_SET_N_ADDENDS (rel, i+1);
}


/* RelocTableDupReloc {{{ */
t_reloc *
RelocTableDupReloc (t_reloc_table * table, const t_reloc * orig)
{
  t_reloc *rel = Calloc (1, sizeof (t_reloc));
  t_relocatable *from;

  RELOC_SET_SWITCH_EDGE(rel, RELOC_SWITCH_EDGE(orig));
  
  if (RELOC_N_ADDENDS(orig))
  {
    t_uint32 i;
    RELOC_SET_N_ADDENDS(rel, RELOC_N_ADDENDS(orig));
    RELOC_SET_ADDENDS(rel, Calloc(1,sizeof(t_address)*RELOC_N_ADDENDS(orig)));
    for (i=0; i<RELOC_N_ADDENDS(orig); i++)
    {
      RELOC_ADDENDS(rel)[i]=RELOC_ADDENDS(orig)[i];
    }
  }

  RELOC_SET_FROM_OFFSET(rel, RELOC_FROM_OFFSET(orig));
  RELOC_SET_FROM(rel, RELOC_FROM(orig));

  from = RELOC_FROM(rel);

  RelocUtilAddRefs (from, NULL, rel, TRUE, 0);

  if (RELOC_N_TO_SYMBOLS(orig))
  {
    t_uint32 i;
    RELOC_SET_N_TO_SYMBOLS(rel, RELOC_N_TO_SYMBOLS(orig));
    RELOC_SET_TO_SYMBOL_OFFSET(rel, Calloc(1,sizeof(t_address)*RELOC_N_TO_SYMBOLS(orig)));
    RELOC_SET_TO_SYMBOL_REF(rel, Calloc(1,sizeof(t_reloc_ref *)*RELOC_N_TO_SYMBOLS(orig)));
    RELOC_SET_TO_SYMBOL(rel, Calloc(1,sizeof(t_symbol *)*RELOC_N_TO_SYMBOLS(orig)));
    for (i=0; i<RELOC_N_TO_SYMBOLS(orig); i++)
    {
      RELOC_TO_SYMBOL(rel)[i]=RELOC_TO_SYMBOL(orig)[i];
      RelocUtilAddRefs (NULL, SYMBOL_BASE(RELOC_TO_SYMBOL(rel)[i]), rel, TRUE, i);
      RELOC_TO_SYMBOL_OFFSET(rel)[i]=RELOC_TO_SYMBOL_OFFSET(orig)[i];
    }
  }
  
  if (RELOC_N_TO_RELOCATABLES(orig))
  {
    t_uint32 i;
    RELOC_SET_N_TO_RELOCATABLES(rel, RELOC_N_TO_RELOCATABLES(orig));
    RELOC_SET_TO_RELOCATABLE_OFFSET(rel, Calloc(1,sizeof(t_address)*RELOC_N_TO_RELOCATABLES(orig)));
    RELOC_SET_TO_RELOCATABLE_REF(rel, Calloc(1,sizeof(t_reloc_ref *)*RELOC_N_TO_RELOCATABLES(orig)));
    RELOC_SET_TO_RELOCATABLE(rel, Calloc(1,sizeof(t_relocatable *)*RELOC_N_TO_RELOCATABLES(orig)));
    for (i=0; i<RELOC_N_TO_RELOCATABLES(orig); i++)
    {
      RELOC_TO_RELOCATABLE(rel)[i]=RELOC_TO_RELOCATABLE(orig)[i];
      RelocUtilAddRefs (NULL, RELOC_TO_RELOCATABLE(rel)[i], rel, FALSE, i);
      RELOC_TO_RELOCATABLE_OFFSET(rel)[i]=RELOC_TO_RELOCATABLE_OFFSET(orig)[i];
    }
  }
  
  RELOC_SET_HELL(rel, RELOC_HELL(orig));
  RELOC_SET_EDGE(rel, NULL);
  RELOC_SET_CODE(rel, StringDup (RELOC_CODE(orig)));
  if (RELOC_LABEL(orig))
    RELOC_SET_LABEL(rel, StringConcat2 ("Copied", RELOC_LABEL(orig)));
  else
    RELOC_SET_LABEL(rel, NULL);

  RelocTableLinkReloc(table, rel);

#ifdef VERBOSE_RELOCS
  VERBOSE(0, ("Duplicated reloc @R\n", orig));
#endif

  RELOC_SET_EDGE(rel, RELOC_EDGE(orig));
  if (RELOC_EDGE(orig))
    RELOC_TABLE_ADD_EDGE_CALLBACK(table) (table, rel, RELOC_EDGE(orig));

  return rel;
}

/* }}} */

void
RelocTableUnlinkReloc (t_reloc_table * table, t_reloc * reloc)
{
  if (RELOC_TABLE_FIRST(table) == reloc)
    RELOC_TABLE_SET_FIRST(table, RELOC_NEXT(reloc));
  if (RELOC_TABLE_LAST(table) == reloc)
    RELOC_TABLE_SET_LAST(table, RELOC_PREV(reloc));

  if (RELOC_PREV(reloc))
    RELOC_SET_NEXT(RELOC_PREV(reloc), RELOC_NEXT(reloc));
  if (RELOC_NEXT(reloc))
    RELOC_SET_PREV(RELOC_NEXT(reloc), RELOC_PREV(reloc));

  RELOC_SET_TABLE(reloc, NULL);
  RELOC_TABLE_SET_NRELOCS(table, RELOC_TABLE_NRELOCS(table) - 1);
}

void
RelocTableLinkReloc (t_reloc_table * table, t_reloc * rel)
{
  RELOC_SET_NEXT(rel, NULL);
  if (RELOC_TABLE_FIRST(table))
  {
    RELOC_SET_NEXT(RELOC_TABLE_LAST(table), rel);
    RELOC_SET_PREV(rel, RELOC_TABLE_LAST(table));
    RELOC_TABLE_SET_LAST(table, rel);
  }
  else
  {
    RELOC_TABLE_SET_FIRST(table, rel);
    RELOC_TABLE_SET_LAST(table, rel);
    RELOC_SET_PREV(rel, NULL);
  }

  RELOC_SET_TABLE(rel, table);
  RELOC_TABLE_SET_NRELOCS(table, RELOC_TABLE_NRELOCS(table) + 1);
}

/* RelocTableRemoveReloc {{{ */
void
RelocTableRemoveReloc (t_reloc_table * table, const t_reloc * to_remove)
{
  t_reloc * reloc = (t_reloc *) to_remove;
  /* First of all we make sure we're removing the relocation from its table.
   * Therefore we disregard the 'table' argument and just look at the relocation
   * itself.
   */
  table = RELOC_TABLE(reloc);
  t_uint32 i;
#ifdef DEBUG_RELOCS
  /* Debugging stuff */
  t_reloc *iter = RELOC_TABLE_FIRST(table);
#endif
  {
    t_relocatable *relocatable;

#ifdef DEBUG_RELOCS
    /* Debugging stuff */
    while (iter && iter != reloc)
      iter = RELOC_NEXT(iter);
    if (!iter)
      FATAL(("Reloc table corrupt: while removing reloc @R, reloc not found in table", reloc));
#endif

    RelocTableUnlinkReloc(table, reloc);

#ifdef VERBOSE_RELOCS
    /* if the following causes a crash, comment it out. This can happen because
     * at the end of Diablo's execution, the relocation may point to a
     * relocatable that has already been freed. Kept by default (at least if
     * VERBOSE_RELOCS is defined) because seeing where relocations are removed
     * can be useful during Diablo's execution to debug certain issues.
     */
    VERBOSE(0, ("Removed reloc @R\n", reloc));
#endif

    /* TODO: Replace with a call to remove to relocs */

    for (i=0; i<RELOC_N_TO_SYMBOLS(reloc); i++)
    {
      RelocRemoveToSymbolRef(reloc,i);
      relocatable = SYMBOL_BASE(RELOC_TO_SYMBOL(reloc)[i]);
      if (relocatable)
      {
      /* if the relocation is to a bbl and it has hell set to FALSE,
       * it probably is the corresponding reloc to a switch edge.
       * in that case we must find the edge and set its corresponding reloc field to NULL
       * in order to avoid dangling pointers */

     
        if (RELOCATABLE_RELOCATABLE_TYPE(relocatable) == RT_BBL && RELOC_SWITCH_EDGE(reloc))
        RELOC_TABLE_DEL_SWITCH_EDGE_CALLBACK(table) (table, reloc, RELOC_SWITCH_EDGE(reloc));
      }
    }

    for (i=0; i<RELOC_N_TO_RELOCATABLES(reloc); i++)
    {
      relocatable = RELOC_TO_RELOCATABLE(reloc)[i];
      if (relocatable)
      {
        t_reloc_ref *refedby = RELOC_TO_RELOCATABLE_REF(reloc)[i];
        t_reloc_ref *prev = RELOC_REF_PREV(refedby);
        if (prev)
          RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refedby));
        else
          RELOCATABLE_SET_REFED_BY(relocatable, RELOC_REF_NEXT(refedby));

        if (RELOC_REF_NEXT(refedby))
          RELOC_REF_SET_PREV(RELOC_REF_NEXT(refedby), prev);
        Free (refedby);

      /* if the relocation is to a bbl and it has hell set to FALSE,
       * it probably is the corresponding reloc to a switch edge.
       * in that case we must find the edge and set its corresponding reloc field to NULL
       * in order to avoid dangling pointers */


      if (RELOCATABLE_RELOCATABLE_TYPE(relocatable) == RT_BBL && RELOC_SWITCH_EDGE(reloc))
        RELOC_TABLE_DEL_SWITCH_EDGE_CALLBACK(table) (table, reloc, RELOC_SWITCH_EDGE(reloc));
      }
    }
    
    /* Remove refby from {{{ */
    relocatable = RELOC_FROM(reloc);
    if (relocatable)
    {
      t_reloc_ref *refto, *prev = NULL;

      refto = RELOC_FROM_REF(reloc);
      if (refto)
      {
        prev = RELOC_REF_PREV(refto);

        if (prev)
          RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refto));
        else
          RELOCATABLE_SET_REFERS_TO(relocatable, RELOC_REF_NEXT(refto));
        if (RELOC_REF_NEXT(refto))
          RELOC_REF_SET_PREV(RELOC_REF_NEXT(refto), prev);
        Free (refto);
      }
    }
    /* }}} */
    /* removing the edge {{{ */
    if (RELOC_EDGE(reloc))
    {
      if (!RELOC_TABLE_DEL_EDGE_CALLBACK(table)) 
        FATAL(("RelocTableDelEdgeCallback not set, but edge of reloc is set!"));
      RELOC_TABLE_DEL_EDGE_CALLBACK(table) (table, reloc, RELOC_EDGE(reloc));
    }
    /* }}} */

    if (RELOC_LABEL(reloc))
      Free (RELOC_LABEL(reloc));
    if (RELOC_CODE(reloc))
      Free (RELOC_CODE(reloc));
    if (RELOC_TO_SYMBOL(reloc))
      Free(RELOC_TO_SYMBOL(reloc));
    if (RELOC_TO_RELOCATABLE(reloc))
      Free(RELOC_TO_RELOCATABLE(reloc));
    if (RELOC_TO_SYMBOL_OFFSET(reloc))
      Free(RELOC_TO_SYMBOL_OFFSET(reloc));
    if (RELOC_TO_SYMBOL_REF(reloc))
      Free(RELOC_TO_SYMBOL_REF(reloc));
    if (RELOC_TO_RELOCATABLE_REF(reloc))
      Free(RELOC_TO_RELOCATABLE_REF(reloc));
    if (RELOC_TO_RELOCATABLE_OFFSET(reloc))
      Free(RELOC_TO_RELOCATABLE_OFFSET(reloc));
    if (RELOC_ADDENDS(reloc))
      Free(RELOC_ADDENDS(reloc));
    
    Free (reloc);
  }
}

/*}}}*/
void
RelocSetFrom (t_reloc * rel, t_relocatable * new_from)
{
  t_reloc_ref *refto, *prev = NULL;

  if (RELOC_FROM(rel) == new_from)
    return;

  refto = RELOC_FROM_REF(rel);

  if (!refto)
  {
    FATAL(("Relocations corrupt! Could not find refedby for killed reloc @R\n", rel));
  }
  else
  {
    prev = RELOC_REF_PREV(refto);

    if (prev)
      RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refto));
    else
      RELOCATABLE_SET_REFERS_TO(RELOC_FROM(rel), RELOC_REF_NEXT(refto));

    if (RELOC_REF_NEXT(refto))
      RELOC_REF_SET_PREV(RELOC_REF_NEXT(refto), prev);
    Free (refto);
  }

  RelocUtilAddRefs (new_from, NULL, rel, FALSE, 0);
  RELOC_SET_FROM(rel, new_from);
}

void
RelocRemoveToSymbolRef(t_reloc * reloc, t_uint32 i)
{
  t_relocatable *relocatable = NULL;
  t_reloc_ref *refedby;
  t_reloc_ref *prev = NULL;
  SYMBOL_SET_RELOCREFCOUNT(RELOC_TO_SYMBOL(reloc)[i], SYMBOL_RELOCREFCOUNT(RELOC_TO_SYMBOL(reloc)[i]) - 1);
  if (SYMBOL_RELOCREFCOUNT(RELOC_TO_SYMBOL(reloc)[i]) < 0 ) FATAL(("Symbol/Reloc info corrupt"));
  relocatable = SYMBOL_BASE(RELOC_TO_SYMBOL(reloc)[i]);
  if (relocatable)
  {
    refedby = RELOC_TO_SYMBOL_REF(reloc)[i];
    prev = RELOC_REF_PREV(refedby);
    if (prev)
      RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refedby));
    else
      RELOCATABLE_SET_REFED_BY(relocatable, RELOC_REF_NEXT(refedby));

    if (RELOC_REF_NEXT(refedby))
      RELOC_REF_SET_PREV(RELOC_REF_NEXT(refedby), prev);
    Free (refedby);
  }

}


void
RelocRemoveToRefs (t_reloc * reloc)
{
  t_uint32 i;
  t_relocatable *relocatable = NULL;
  t_reloc_ref *refedby;
  t_reloc_ref *prev = NULL;

  for (i=0; i<RELOC_N_TO_SYMBOLS(reloc); i++)
  {
    RelocRemoveToSymbolRef(reloc,i);
  }

  for (i=0; i<RELOC_N_TO_RELOCATABLES(reloc); i++)
  {
    relocatable = RELOC_TO_RELOCATABLE(reloc)[i];
    if (relocatable)
    {
      refedby = RELOC_TO_RELOCATABLE_REF(reloc)[i];
      prev = RELOC_REF_PREV(refedby);
      if (prev)
        RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refedby));
      else
        RELOCATABLE_SET_REFED_BY(relocatable, RELOC_REF_NEXT(refedby));

      if (RELOC_REF_NEXT(refedby))
        RELOC_REF_SET_PREV(RELOC_REF_NEXT(refedby), prev);
      Free (refedby);
    }
  }
}

void
RelocSetToSymbol (t_reloc * rel, t_uint32 i, t_symbol * new_to)
{
  t_relocatable *to =NULL;

  ASSERT(RELOC_N_TO_SYMBOLS(rel)>i, ("RelocSetToSymbol called for non existing symbol."));

  if (RELOC_TO_SYMBOL(rel)[i])
  {
    to=SYMBOL_BASE(RELOC_TO_SYMBOL(rel)[i]);
    RelocRemoveToSymbolRef(rel, i);
  }
  
  RELOC_TO_SYMBOL(rel)[i] = new_to;
  RelocUtilAddRefs (NULL, SYMBOL_BASE(new_to), rel, TRUE, i);
}

void
SymbolSetSymbolBaseAndUpdateRelocs(t_reloc_table * reltab, t_symbol * sym, t_relocatable * new_to)
{
  t_reloc *rel;
  t_uint32 i;

  if (new_to==SYMBOL_BASE(sym)) return;

  for (rel = RELOC_TABLE_FIRST(reltab); rel != NULL; rel = RELOC_NEXT(rel))
  {
    for (i=0; i<RELOC_N_TO_SYMBOLS(rel); i++)
    {
      if (RELOC_TO_SYMBOL(rel)[i]==sym)
      {
        RelocRemoveToSymbolRef(rel,i);
        RelocUtilAddRefs(NULL, new_to, rel, TRUE, i);
      }
    }
  }
	 
  SymbolSetBase(sym, new_to);
}

void
RelocSetToRelocatable (t_reloc * rel, t_uint32 i, t_relocatable * new_to)
{
  t_reloc_ref *refby;
  t_reloc_ref *prev = NULL;
  t_relocatable *to=NULL;

  if (RELOC_N_TO_RELOCATABLES(rel)<i)
  {
    FATAL(("Attempt to assign a non existing relocatable"));
  }
  else
  {
    to = RELOC_TO_RELOCATABLE(rel)[i];
  }

  if ((to) && (new_to != to))
  {
    refby = RELOC_TO_RELOCATABLE_REF(rel)[i];
    prev = RELOC_REF_PREV(refby);

    if (prev)
      RELOC_REF_SET_NEXT(prev, RELOC_REF_NEXT(refby));
    else
      RELOCATABLE_SET_REFED_BY(to, RELOC_REF_NEXT(refby));

    if (RELOC_REF_NEXT(refby))
      RELOC_REF_SET_PREV(RELOC_REF_NEXT(refby), prev);
    Free (refby);

  }

  /* the refed by */
  if ((new_to) && (new_to != to))
  {
    RelocUtilAddRefs (NULL, new_to, rel, FALSE, i);
  }
  RELOC_TO_RELOCATABLE(rel)[i] = new_to;
}

t_reloc_ref *
RealRelocRefNew (FORWARD_MALLOC_FUNCTION_DEF t_reloc * rel)
{
  t_reloc_ref *ret =
    RealMalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (t_reloc_ref));


  RELOC_REF_SET_RELOC(ret, rel);
  RELOC_REF_SET_NEXT(ret, NULL);
  RELOC_REF_SET_PREV(ret, NULL);
  return ret;
}

t_symbol_ref *
RealSymbolRefNew (FORWARD_MALLOC_FUNCTION_DEF t_symbol * sym)
{
  t_symbol_ref *ret =
    RealMalloc (FORWARD_MALLOC_FUNCTION_USE sizeof (t_symbol_ref));


  ret->sym = sym;
  ret->next = NULL;
  ret->prev = NULL;
  return ret;
}


t_reloc *
RelocatableGetRelocForAddress(const t_relocatable *relable, t_address addr) {
  t_address address;
  t_reloc_ref *rr;
  t_reloc *rel;
  t_section *sub;
  
  if (RELOCATABLE_RELOCATABLE_TYPE(relable)==RT_SECTION)
  {
    SECTION_FOREACH_SUBSECTION(T_SECTION(relable),sub)
    {
      t_reloc *res = RelocatableGetRelocForAddress(T_RELOCATABLE(sub),addr);
      if (res)
        return res;
    }
  }
  else
  {
    rr = RELOCATABLE_REFERS_TO(relable);
    while (rr!=NULL)
    {
      rel = RELOC_REF_RELOC(rr);
      address = AddressAdd (RELOCATABLE_CADDRESS(relable), RELOC_FROM_OFFSET(rel));
      if (AddressIsEq(addr,address)) {
        return rel;
      }
      rr=RELOC_REF_NEXT(rr);
    }
  }
  return NULL;
}

void RelocatableRemoveAllRefersTo(t_relocatable* relocatable)
{
  while (RELOCATABLE_REFERS_TO(relocatable))
  {
    t_reloc* reloc = RELOC_REF_RELOC(RELOCATABLE_REFERS_TO(relocatable));
    RelocTableRemoveReloc(RELOC_TABLE(reloc), reloc);
  }
}

t_uint32 RelocGetToRelocatableIndex (const t_reloc *rel, const t_relocatable *relocatable)
{
  t_uint32 index;

  for (index = 0; index < RELOC_N_TO_RELOCATABLES(rel); index++)
  {
    if (RELOC_TO_RELOCATABLE(rel)[index] == relocatable)
      return index;
  }
  return -1;
}

void RelocMoveToRelocTable(t_reloc* rel, t_reloc_table* table)
{
  t_reloc_table* orig = RELOC_TABLE(rel);
  if (orig != table)
  {
    RelocTableUnlinkReloc(orig, rel);
    RelocTableLinkReloc(table, rel);
  }
}

t_bool RelocIsRelative(const t_reloc *rel)
{
  char* writepos = strchr(RELOC_CODE(rel),'\\');
  /* subtracting the address of the got? */
  char* checkpos = strstr(RELOC_CODE(rel),"g-");
  if ((checkpos != NULL) &&
      (checkpos < writepos))
    return TRUE;

  /* Obfuscation-related relative relocations */
  if (strcmp(RELOC_CODE(rel), "R00R01-\\l*w\\s0000$") == 0)
    return TRUE;
  if (strcmp(RELOC_CODE(rel), "R00A00|R01-A01-\\l*w\\s0000$") == 0)
    return TRUE;
  if (strcmp(RELOC_CODE(rel), "R00R01Z01+-\\l*w\\s0000$") == 0)
    return TRUE;

  /* Allow broker calls to check whether a relocation is relative, if so, return TRUE */
  t_bool bc_isrelative = FALSE;
  DiabloBrokerCall("RelocIsRelative", rel, &bc_isrelative);
  if (bc_isrelative)
    return TRUE;

  return FALSE;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
