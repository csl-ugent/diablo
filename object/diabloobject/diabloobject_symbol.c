/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>

#define I_DONT_WANT_MY_SYMBOLREFS_SORTED

/*
 * This table is used to tokenize expressions found in $diablo symbols
 * inserted by our assembler patches. The entries should be interpreted as
 * bitmasks. The following legend is to be considered lsb -> msb
 *
 *    - illegal/unhandled byte
 *    - possible part of a symbol name
 *    - digit
 *    - arithmetic
 *    - whitespace
 */
static const char dst_tab[256] = {
/*                                        \t  \n                */
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 16, 16, 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
/*  , ! , " , # , $ , % , & , ' , ( , ) , * , + , , , - , . , / */
  16, 2 , 1 , 2 , 2 , 8 , 1 , 1 , 8 , 8 , 8 , 8 , 2 , 8 , 2 , 8,
/*0 , 0 , 2 , 0 , 4 , 5 , 6 , 7 , 8 , 9 , : , ; , < , = , > , ? */
  6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 6 , 0 , 0 , 0 , 0 , 0 , 0,
/*@ , A , B , C , D , E , F , G , H , I , J , K , L , M , N , O */
  2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2,
/*P , Q , R , S , T , U , V , W , X , Y , Z , [ , \ , ] , ^ , _ */
  2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 2,
/*` , a , b , c , d , e , f , g , h , i , j , k , l , m , n , o */
  1 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2,
/*p , q , r , s , t , u , v , w , x , y , z , { , | , } , ~ , . */
  2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 2,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1,
  1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1
};

static t_symbol *
SymbolTableAllocNewSymbol(t_symbol_table * symbol_table, t_const_string name, t_const_string alias, t_bool prepend)
{
  t_symbol * ret= Calloc (1, sizeof (t_symbol));
  SYMBOL_SET_NAME(ret, StringDup (name));
  SYMBOL_SET_NEXT(ret, NULL);
  if (!SYMBOL_TABLE_FIRST(symbol_table))
  {
    SYMBOL_TABLE_SET_FIRST(symbol_table, ret);
    SYMBOL_TABLE_SET_LAST(symbol_table, ret);
    SYMBOL_SET_NEXT(ret, NULL);
    SYMBOL_SET_PREV(ret, NULL);
  }
  else
  {
    SYMBOL_SET_NEXT(SYMBOL_TABLE_LAST(symbol_table), ret);
    SYMBOL_SET_PREV(ret, SYMBOL_TABLE_LAST(symbol_table));
    SYMBOL_SET_NEXT(ret, NULL);
    SYMBOL_TABLE_SET_LAST(symbol_table, ret);
  }


  SYMBOL_TABLE_SET_NSYMS(symbol_table, SYMBOL_TABLE_NSYMS(symbol_table) + 1);

  /*SYMBOL_SET_KEY(ret, StringDup (name));*/
  SYMBOL_SET_KEY(ret, StringDup (alias));
  SYMBOL_SET_TENTATIVE(ret, NULL);

  // print some debug info is aliased symbols were used
  if (strcmp(alias, name) != 0)
    VERBOSE(1,("Adding aliased symbol | alias: %s name: %s ", alias, name));

  if (prepend)
  {
    SymbolTablePrepend (symbol_table, (void *) ret);
  }
  else
    SymbolTableInsert (symbol_table, (void *) ret);

  return ret;

}

void SymbolSetBase(t_symbol * sym, t_relocatable * base)
{
  t_symbol_ref * ref = SYMBOL_BASE_REF(sym);
#ifndef I_DONT_WANT_MY_SYMBOLREFS_SORTED
  t_symbol_ref *ref_iter;
#endif
  /* When this symbol's base is moved, remove the ref from the list of symbol
   * ref's from the old base */
  if (ref)
  {
    if (ref->prev)
    {
      ref->prev->next=ref->next;
      if (ref->next)
        ref->next->prev = ref->prev;
      Free(ref);
    }
    else
    {
      RELOCATABLE_SET_REFED_BY_SYM(SYMBOL_BASE(sym), ref->next);
      if (ref->next)
        ref->next->prev = NULL;
      Free(ref);
    }
  }

  SYMBOL_SET_BASE(sym, base);
  ref=SymbolRefNew(sym);
#ifdef I_DONT_WANT_MY_SYMBOLREFS_SORTED
  ref->prev = NULL;
  ref->next = RELOCATABLE_REFED_BY_SYM(base);
  if (RELOCATABLE_REFED_BY_SYM(base))
  {
    RELOCATABLE_REFED_BY_SYM(base)->prev=ref;
  }
  RELOCATABLE_SET_REFED_BY_SYM(base, ref);
  SYMBOL_SET_BASE_REF(sym, ref);
#else
  /* Insert sorted by offset from start */
  ref_iter = RELOCATABLE_REFED_BY_SYM(base);

  if(ref_iter && AddressIsLe(SYMBOL_OFFSET_FROM_START(ref_iter->sym),SYMBOL_OFFSET_FROM_START(sym)))
    while(ref_iter->next && AddressIsLe(SYMBOL_OFFSET_FROM_START(ref_iter->next->sym),SYMBOL_OFFSET_FROM_START(sym)))
    {
      ref_iter = ref_iter->next;
    }

  ref->prev = ref_iter;
  if (ref_iter)
  {
    ref->next = ref_iter->next;
    ref_iter->next = ref;
  }
  else RELOCATABLE_SET_REFED_BY_SYM(base, ref);
  if(ref->next) ref->next->prev = ref;

/*  if (RELOCATABLE_REFED_BY_SYM(base))*/
/*  {*/
/*    RELOCATABLE_REFED_BY_SYM(base)->prev=ref;*/
/*  }*/
/*  RELOCATABLE_SET_REFED_BY_SYM(base, ref);*/
  SYMBOL_SET_BASE_REF(sym, ref);
#endif
}

t_symbol *
SymbolTableDupSymbolWithOrder(t_symbol_table * symbol_table, const t_symbol * in, t_const_string new_name, t_int32 order)
{
  t_string code_copy  = StringDup(SYMBOL_CODE(in));
  t_symbol * dup = SymbolTableAddSymbol (symbol_table, new_name, code_copy, order, FALSE, FALSE, SYMBOL_BASE(in), SYMBOL_OFFSET_FROM_START(in), SYMBOL_ADDEND(in), SYMBOL_TENTATIVE(in), SYMBOL_SIZE(in), SYMBOL_FLAGS(in));
  Free(code_copy);
  return dup;
}


t_symbol *
SymbolTableDupSymbol(t_symbol_table * symbol_table, const t_symbol * in, t_const_string new_name)
{
  t_string code_copy  = StringDup(SYMBOL_CODE(in));
  t_symbol * dup = SymbolTableAddSymbol (symbol_table, new_name, code_copy, SYMBOL_ORDER(in), SYMBOL_DUP(in), SYMBOL_SEARCH(in), SYMBOL_BASE(in), SYMBOL_OFFSET_FROM_START(in), SYMBOL_ADDEND(in), SYMBOL_TENTATIVE(in), SYMBOL_SIZE(in), SYMBOL_FLAGS(in));
  Free(code_copy);
  return dup;
}

t_symbol *
realSymbolTableAddAliasedSymbol (t_const_string file, int line, t_symbol_table * symbol_table, t_const_string name, t_const_string alias, t_const_string code, t_int32 order, t_tristate dup, t_tristate search, t_relocatable * sec, t_address offset, t_address addend, t_const_string tentative, t_address size, t_uint32 flags)
{

  t_symbol * duplicate = NULL;

  if (sec && SECTION_NAME(T_SECTION(sec)) && StringPatternMatch(".debug_*",SECTION_NAME(T_SECTION(sec))) && (StringPatternMatch("$diablo*",name) || StringPatternMatch("$d",name)))
    {
      VERBOSE(2,("SKIPPING SYMBOL %s in SECTION %s",name,SECTION_NAME(T_SECTION(sec))));
      return NULL;
    }

  /* Without an alias the alias is the name ofc */
  if (alias == NULL)
      alias = name;

   VERBOSE(1,("At %s:%d: Adding symbol %s %s order:%d dup:%d search:%d to @T offset @G size @G flags 0x%x", file, line, name, code, order, dup, search,sec,offset,size,flags));

  if (order>=0)
    duplicate = (t_symbol *) SymbolTableLookup (symbol_table, name);

  if (duplicate)
  {
    /* duplicate is local symbol, create new symbol, prepend it */
    if (SYMBOL_ORDER(duplicate)<0)
    {
      duplicate = SymbolTableAllocNewSymbol(symbol_table, name, alias, TRUE);
    }
    /* duplicate is a symbol that can be overwritten by our new symbol */
    else if (SYMBOL_ORDER(duplicate)<order)
    {
      /* Fall through to fill, this will overwrite the dup symbol */
    }
    /* duplicate can be overwrite our new symbol, simply return the duplicate */
    else if (SYMBOL_ORDER(duplicate)>order)
    {
      return duplicate;
    }
    /* This symbol (and the newly added symbol allows duplicates (weird if the order<0), create new symbol,
     * append */
    else if ((SYMBOL_DUP(duplicate)==TRUE)&&(dup==TRUE))
    {
      duplicate = SymbolTableAllocNewSymbol(symbol_table, name, alias, FALSE);
    }
    else if ((SYMBOL_DUP(duplicate)==PERHAPS)&&(dup==PERHAPS))
    {
      ASSERT(SYMBOL_ORDER(duplicate)==order,("Symbol @S has a duplicate of the same order that is not identical",duplicate));
      ASSERT(SYMBOL_SEARCH(duplicate)==search,("Symbol @S has a duplicate of the same order that is not identical",duplicate));
      ASSERT(strcmp(SYMBOL_CODE(duplicate),code)==0,("Symbol @S has a duplicate of the same order that is not identical",duplicate));
      /* a weak symbol may have to be unified with another weak symbol
       * (i.e., the order of both symbols is the same) -> only give
       * an error if these are symbols with a higher order than
       * weak
       */
      if (order>SYMBOL_ORDER_WEAK)
      {
        ASSERT(SYMBOL_BASE(duplicate)==sec,("Symbol @S (dup %d) to @T has a duplicate of the same order that is not identical (code=%s order=%d dup=%d search=%d @T)",duplicate, SYMBOL_DUP(duplicate), SYMBOL_BASE(duplicate), code, order, dup, search, sec));
        ASSERT(AddressIsEq(SYMBOL_OFFSET_FROM_START(duplicate),offset),("Symbol @S (dup %d) to @T has a duplicate of the same order that is not identical (code=%s order=%d dup=%d search=%d offset=%x)",duplicate, SYMBOL_DUP(duplicate), SYMBOL_BASE(duplicate), code, order, dup, search, offset));
      }

      /* if the symbols have different sizes, the one with the largest size is chosen by
       * the linker (at least in case of common symbols)
       */
      if (AddressIsLe(size,SYMBOL_SIZE(duplicate)))
        return duplicate;
      else
        /* else fall through and replace dupplicate with new one */
        VERBOSE(1,(" -> Replacing previous symbol %s (size: @G) with larger one (size: @G)",name,SYMBOL_SIZE(duplicate),size));
    }
    /* This symbol allows duplicates if they are identical */
    else if ((SYMBOL_DUP(duplicate)!=FALSE)&&(dup!=FALSE))
    {
      FATAL(("Implement"));
    }
    else
    {
      VERBOSE(2, ("Trying to add the symbol %s with order %d but a symbol @S is already present in the symbol table, and this symbol or the new symbol does not allow duplicates", name, order, duplicate));
      VERBOSE(2, ("(ignoring for now since we don't need this debug information).\n"));
    }
  }
  else
  {
    duplicate = SymbolTableAllocNewSymbol(symbol_table, name, alias, FALSE);
  }

  ASSERT(sec, ("At %s:%d: Trying to add a symbol without relocatable, which is illegal... You can use either OBJECT_UNDEF_SECTION(parent_object) as a relocatable for undefined symbols or OBJECT_ABS_SECTION(parent_object) as a relocatable for symbols that represent constant values", file, line));

  SYMBOL_SET_ORDER(duplicate, order);
  SYMBOL_SET_DUP(duplicate, dup);
  SYMBOL_SET_SEARCH(duplicate, search);

  if (SYMBOL_CODE(duplicate))
     Free(SYMBOL_CODE(duplicate));

  SYMBOL_SET_CODE(duplicate, StringDup(code));

  if (SYMBOL_TENTATIVE(duplicate))
     Free(SYMBOL_TENTATIVE(duplicate));

  if (tentative)
	  SYMBOL_SET_TENTATIVE(duplicate, StringDup(tentative));
  else
	  SYMBOL_SET_TENTATIVE(duplicate,NULL);

  SYMBOL_SET_SIZE(duplicate, size);
  SYMBOL_SET_OFFSET_FROM_START(duplicate, offset);
  SYMBOL_SET_ADDEND(duplicate, addend);
  SYMBOL_SET_FLAGS(duplicate, flags);
  SYMBOL_SET_MAPPED(duplicate, NULL);
  SYMBOL_SET_SYMBOL_TABLE(duplicate, symbol_table);

  SymbolSetBase(duplicate, sec);

  return duplicate;
}

t_symbol *
SymbolTableAddAbsPrefixedSymWithFlagsIfNonExisting(t_symbol_table *st, t_object *absobj, t_const_string prefix, t_const_string symname, t_uint32 flags)
{
  t_string prefixedsymname;
  t_symbol *sym;

  prefixedsymname = StringConcat2(prefix,symname);
  sym = SymbolTableGetSymbolByName(st,prefixedsymname);
  if (!sym)
    sym = SymbolTableAddSymbol(st, prefixedsymname, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(OBJECT_ABS_SECTION(absobj)), AddressNullForObject(absobj), AddressNullForObject(absobj), NULL, AddressNullForObject(absobj), flags);
  Free(prefixedsymname);

  return sym;
}

/* Free symbol table {{{ */
void
SymbolTableFree (const t_symbol_table * st)
{
  t_symbol *sym;
  t_symbol_ref * ref = NULL;
  t_symbol *sym_next;

  HashTableFini (T_HASH_TABLE(st));
  for (sym = SYMBOL_TABLE_FIRST(st); sym != NULL; sym = sym_next)
  {
    sym_next = SYMBOL_NEXT(sym);
    if (SYMBOL_NAME(sym))
    {
      Free (SYMBOL_NAME(sym));
    }
    if (SYMBOL_CODE(sym))
    {
      Free (SYMBOL_CODE(sym));
    }
    if (SYMBOL_TENTATIVE(sym))
    {
      Free (SYMBOL_TENTATIVE(sym));
    }
    /* unlink symbol refs */
    if (SYMBOL_BASE_REF(sym))
    {
      if (SYMBOL_BASE_REF(sym)->prev)
      {
        ref=SYMBOL_BASE_REF(sym);
        ref->prev->next=ref->next;
        if (ref->next)
          ref->next->prev = ref->prev;
        Free(ref);
      }
      else
      {
        ref=SYMBOL_BASE_REF(sym);
        RELOCATABLE_SET_REFED_BY_SYM(SYMBOL_BASE(sym), ref->next);
        if (ref->next)
          ref->next->prev = NULL;
        Free(ref);
      }
    }



    Free (sym);
  }
  Free (st);
}

/* }}} */

/* SymbolTableHEFree: free's a hash element */

void
SymbolTableHEFree (const void *f, void *data)
{
  t_symbol *tf = ((t_symbol *) f);

  Free (SYMBOL_KEY(tf));
}

/* Allocate a new symbol table {{{*/
t_symbol_table *
SymbolTableNew (t_object * obj)
{
  t_symbol_table *ret = Calloc (sizeof (t_symbol_table), 1);

  SYMBOL_TABLE_SET_OBJECT(ret, obj);
  HashTableInit (T_HASH_TABLE(ret), 2003, 0,
                                  (t_hash_func)StringHash,
                                  (t_hash_cmp)StringCmp,
                                  SymbolTableHEFree);
  return ret;
}
/*}}} */

/* FUN : SymbolTableGetDataType
 * PAR : The object to search and the address
 * RET : ADDRESS_IS_CODE or ADDRESS_IS_DATA
 * DESC: this function returns the datatype for the given address */
#define CACHED_TYPES
#ifdef CACHED_TYPES
t_symbol *symbol_type_cached = NULL;
#endif
t_uint32
SymbolTableGetDataType (const t_object * obj, t_address adr)
{
#ifdef CACHED_TYPES
  static t_address cacheaddr;
  static t_bool cache_type;
#endif
  t_symbol *top, *sym_it;
  t_bool code = TRUE;
  t_symbol *first;

#ifdef CACHED_TYPES
  if ((symbol_type_cached) && (AddressIsGe (adr, cacheaddr)))
  {
    top = symbol_type_cached;
    code = cache_type;
  }
  else
  {
    top =
      (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                    "$switch");
    first =
      (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                    "$first");
    if ((first) && (AddressIsNull (StackExecConst(SYMBOL_CODE(first), NULL, first, 0, obj))))
      code = FALSE;
    if (!top)
      return ADDRESS_IS_CODE;
  }
#endif

  while (SYMBOL_EQUAL(top))
  {
    if (AddressIsGt (StackExecConst(SYMBOL_CODE(SYMBOL_EQUAL(top)), NULL, SYMBOL_EQUAL(top), 0, obj), adr))
      break;
    code = !code;
    top = SYMBOL_EQUAL(top);
  }

#ifdef CACHED_TYPES
  symbol_type_cached = top;
  cache_type = code;
  cacheaddr = adr;
#endif

  if (!code)
    return ADDRESS_IS_DATA;
  else
    return ADDRESS_IS_CODE;
}

/* Given an address, tell if it is in a thumb or an arm piece of code */
t_uint32
SymbolTableGetCodeType (const t_object * obj, t_address addr)
{
  t_symbol *top = NULL;
  t_bool arm = TRUE;
  t_symbol *first = NULL;
  t_bool found;

  t_section *sec;
  t_uint32 i;
  sec = NULL;
  found = FALSE;
  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    if (AddressIsGe(addr, SECTION_CADDRESS(sec)) &&
        AddressIsLt(addr, AddressAdd(SECTION_CADDRESS(sec), SECTION_CSIZE(sec))))
    {
      found = TRUE;
      break;
    }
  }
  if (!found)
  {
    /* not in code section, so definitely not thumb code */
    return ADDRESS_IS_CODE;
  }

  top =
    (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                  "$code_switch");
  first =
    (t_symbol *) SymbolTableLookup (OBJECT_SUB_SYMBOL_TABLE(obj),
                                  "$thumb");
  if ((first) && (AddressIsNull (StackExecConst(SYMBOL_CODE(first), NULL, first, 0, obj))))
    arm = FALSE;
  if (!top)
    return ADDRESS_IS_CODE;

  while (SYMBOL_EQUAL(top))
  {
    if (AddressIsGt (StackExecConst(SYMBOL_CODE(SYMBOL_EQUAL(top)), NULL, SYMBOL_EQUAL(top), 0, obj), addr))
      break;
    arm = !arm;
    top = (t_symbol *) SYMBOL_EQUAL(top);
  }

  if (!arm)
    return ADDRESS_IS_SMALL_CODE;
  else
    return ADDRESS_IS_CODE;
}


/* Set an appropriate symbol type for all symbols in the symbol table  */
void
SymbolTableSetSymbolTypes (t_symbol_table *st)
{
  t_object *obj = SYMBOL_TABLE_OBJECT(st);
  t_symbol *sym;

  /* recompute the $switch labels, since this happens after final
   * layouting
   */
  ObjectCreateDataOrCodeTable(obj);
  /* flush the lookup cache */
  SymbolTableGetDataType(obj,AddressNullForObject(obj));

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    t_address symaddr;

    if (!SYMBOL_BASE(sym))
      continue;

    if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FILE | SYMBOL_TYPE_OBJECT | SYMBOL_TYPE_NOTYPE | SYMBOL_TYPE_SECTION))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;

    symaddr = AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)),
                          SYMBOL_OFFSET_FROM_START(sym));
    if (SymbolTableGetDataType(obj,symaddr) == ADDRESS_IS_CODE)
    {
      if (SymbolTableGetCodeType(obj,symaddr) == ADDRESS_IS_SMALL_CODE
          && (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FUNCTION))
          SYMBOL_SET_FLAGS(sym,SYMBOL_TYPE_FUNCTION_SMALLCODE);
      //      else
      //        SYMBOL_SET_FLAGS(sym,SYMBOL_TYPE_FUNCTION);
    }
    else
    {
      if ((SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym))) == TLSBSS_SECTION) ||
          (SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym))) == TLSDATA_SECTION))
        SYMBOL_SET_FLAGS(sym,SYMBOL_TYPE_TLS);
      else
        SYMBOL_SET_FLAGS(sym,SYMBOL_TYPE_NOTYPE);
    }
  }
}

/* Query the symbol table {{{ */
t_symbol *
SymbolTableGetSymbolByName (const t_symbol_table * st, t_const_string name)
{
  t_symbol *he = (t_symbol *) SymbolTableLookup (st, name);

  if (he == NULL)
    return NULL;

  if ((SYMBOL_EQUAL(he)) && (!(SYMBOL_ORDER(SYMBOL_EQUAL(he)) < 0)))
  {
    FATAL(("Request for symbol by name, but Symbol %s has multiple definitions: @S @S (more definitions could be present) Implement!", name,he, SYMBOL_EQUAL(he)));
  }

  return he;
}

t_symbol *
SymbolTableGetFirstSymbolWithName (const t_symbol_table * st, t_const_string name)
{
  t_symbol *he = (t_symbol *) SymbolTableLookup (st, name);

  if (he == NULL)
    return NULL;
  return he;
}

/* return symbol at given address.
 * as this function is only used to determine function names, we try to
 * find the symbol at that address that is most likely to contain the function name */
t_symbol *
SymbolTableGetSymbolByAddress (const t_symbol_table * st, t_address addr)
{
  t_symbol *sym, *tmp = NULL;

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    if (!SYMBOL_BASE(sym))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;

    if (AddressIsEq
        (AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym)), addr))
    {
        if (!(SYMBOL_ORDER(sym) < 0))
          tmp = sym;
        else if (!tmp)
          tmp = sym;
    }
  }
  return tmp;
}

/* data structures and interfaces for hash table that stores symbols per address*/


typedef struct _t_symbol_he t_symbol_he;

struct _t_symbol_he
{
  t_hash_table_node node;
  t_symbol * sym1; /* first symbol */
  t_symbol * sym2; /* next symbol with correct name */
};

void * SymbolAddressKey(const t_symbol * sym)
{
  t_address * result = (t_address*) Malloc(sizeof(t_address));
  *result = AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym));
  return result;
}

static t_uint32
SymbolAddressHash(const void * key, const struct _t_hash_table *ht)
{
  t_address a = *(t_address*) key;
  return G_T_UINT32(a) % HASH_TABLE_TSIZE(ht);
}

static t_int32
SymbolAddressCmp(const void *addr1, const void *addr2)
{
  t_address a1 = *(t_address*)addr1;
  t_address a2 = *(t_address*)addr2;
  
  /* return 0 if equal */
  return !AddressIsEq(a1,a2);
}

static void
SymbolAddressFree(const void *node, void *null)
{
  t_symbol_he *symbol_node = (t_symbol_he *)node;

  Free(HASH_TABLE_NODE_KEY(&symbol_node->node));
  Free(symbol_node);
}

/* create a hash table with per address the first symbol as well as the first next symbol 
 * that matches the name specified by next_symbol_target
 */ 
t_hash_table * 
SymbolTableCreateHashTableForGetFirst(const t_symbol_table * st, t_const_string next_symbol_target)
{
  t_hash_table * ht = HashTableNew(21011,0,SymbolAddressHash,SymbolAddressCmp,SymbolAddressFree);
  t_symbol *sym;
  t_address address;

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    t_symbol_he * node;

    if (!SYMBOL_BASE(sym))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;

    /* check if there already was a symbol at the address */
    address = AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym));
    node = HashTableLookup(ht,&address);
    
    if (!node)
      {
        /* no symbol at this address was found yet, create new one */
        node = (t_symbol_he*) Malloc(sizeof(t_symbol_he));
        node->sym1 = sym;
        node->sym2 = NULL;
        HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(node), SymbolAddressKey(sym));
        HashTableInsert(ht,node);
      }

    /* if the node has the right name, store that as the next node with the correct name */
    if (node->sym2==NULL && strncmp(SYMBOL_NAME(sym),next_symbol_target,strlen(next_symbol_target))==0)
      node->sym2 = sym;
  }
  return ht;
}

/* create a hash table with per address the first symbol as well as the first next symbol 
 * that matches the name specified by next_symbol_target
 */ 
t_hash_table * 
SymbolTableCreateHashTableForGetFirstLimitedToNamePattern(const t_symbol_table * st, t_const_string name_pattern)
{
  t_hash_table * ht = HashTableNew(21011,0,SymbolAddressHash,SymbolAddressCmp,SymbolAddressFree);
  t_symbol *sym;
  t_address address;

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    t_symbol_he * node;

    if (!SYMBOL_BASE(sym))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;
    
    if (!StringPatternMatch(name_pattern, SYMBOL_NAME(sym)))
      continue;

    /* check if there already was a symbol at the address */
    address = AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym));
    node = HashTableLookup(ht,&address);
    
    if (!node)
      {
        /* no symbol at this address was found yet, create new one */
        node = (t_symbol_he*) Malloc(sizeof(t_symbol_he));
        node->sym1 = sym;
        node->sym2 = NULL;
        HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(node), SymbolAddressKey(sym));
        HashTableInsert(ht,node);
      }
    else 
      /* store at most one next symbol (never the same symbol (as was possible in the previous function) 
         at the same address to avoid expensive GetNextSymbolByAddress calls */
      if (node->sym2==NULL) 
        node->sym2 = sym;
  }
  return ht;
}

/*! Return first symbol at given address.
 *
 * \param st   Symbol table to search in.
 * \param addr Address to search for.
 *
 * \return Found symbol or NULL if not found.
 */
t_symbol *
SymbolTableGetFirstSymbolByAddress (const t_symbol_table *st, t_address addr)
{
  t_symbol *sym;

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    if (!SYMBOL_BASE(sym))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;

    if (AddressIsEq
        (AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym)), addr))
    {
      return sym;
    }
  }
  return NULL;
}

/*! Return next symbol at given address.
 *
 * \param prev Where to start searching from.
 * \param addr Address to search for.
 *
 * \return Found symbol or NULL if not found.
 */
t_symbol *
SymbolTableGetNextSymbolByAddress (const t_symbol * prev, t_address addr)
{
  t_symbol *sym;

  for (sym = SYMBOL_NEXT(prev); sym != NULL; sym = SYMBOL_NEXT(sym))
  {
    if (!SYMBOL_BASE(sym))
      continue;

    if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
            || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB))
      continue;

    if (AddressIsEq
        (AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym)), addr))
    {
      return sym;
    }
  }
  return NULL;
}

/* }}} */

/* Print the symbol table {{{ */
void
SymbolTablePrint (const t_symbol_table * st)
{
  t_symbol *sym;

  SYMBOL_TABLE_FOREACH_SYMBOL(st, sym)
  {
    VERBOSE(0, ("@S\n", sym));
  }
}

/* }}} */

void
realSymbolTableRemoveSymbol (t_const_string file, int line, t_symbol_table * table, const t_symbol * sym)
{
  t_symbol_ref * ref = NULL;
  ASSERT(SYMBOL_RELOCREFCOUNT(sym)==0, ("Trying to remove symbol still referenced (count %d) by relocation at %s:%d",SYMBOL_RELOCREFCOUNT(sym), file,line));
  /* remove from the linked list of symbols in the symbol table */
  if (SYMBOL_PREV(sym))
    SYMBOL_SET_NEXT(SYMBOL_PREV(sym), SYMBOL_NEXT(sym));
  else
    SYMBOL_TABLE_SET_FIRST(table, SYMBOL_NEXT(sym));

  if (SYMBOL_NEXT(sym))
    SYMBOL_SET_PREV(SYMBOL_NEXT(sym), SYMBOL_PREV(sym));
  else
    SYMBOL_TABLE_SET_LAST(table, SYMBOL_PREV(sym));

  /* remove from the hash table associated with the symbol table */
  SymbolTableDelete (table, sym);

  /* unlink symbol refs */
  if (SYMBOL_BASE_REF(sym))
  {
    if (SYMBOL_BASE_REF(sym)->prev)
    {
      ref=SYMBOL_BASE_REF(sym);
      ref->prev->next=ref->next;
      if (ref->next)
        ref->next->prev = ref->prev;
      Free(ref);
    }
    else
    {
      ref=SYMBOL_BASE_REF(sym);
      RELOCATABLE_SET_REFED_BY_SYM(SYMBOL_BASE(sym), ref->next);
      if (ref->next)
        ref->next->prev = NULL;
      Free(ref);
    }
  }

  /* free the symbol structure itself */
  Free (SYMBOL_NAME(sym));
  Free (SYMBOL_CODE(sym));
  if (SYMBOL_TENTATIVE(sym))
    Free(SYMBOL_TENTATIVE(sym));
  Free (sym);

  SYMBOL_TABLE_SET_NSYMS(table, SYMBOL_TABLE_NSYMS(table) - 1);
}

static t_object * pobj=NULL;

static int
__cmp_symb_addresses (const void *a, const void *b)
{
  t_symbol *A = *((t_symbol **) a);
  t_symbol *B = *((t_symbol **) b);

  if (AddressIsLt (StackExec(SYMBOL_CODE(A), NULL, A, NULL, FALSE, 0, pobj), StackExec(SYMBOL_CODE(B), NULL, B, NULL, FALSE, 0, pobj)))
    return -1;
  return 1;
}

void
SymbolTableSortSymbolsByAddress (t_symbol_table * table, t_const_string name)
{
  t_symbol **symarr;
  t_symbol *top, *he;
  int count;
  int tel;

  top = (t_symbol *) SymbolTableLookup (table, name);
  if (!top)
    return;
  count = 0;
  for (he = top; he; he = (t_symbol *) SYMBOL_EQUAL(he))
    count++;
  symarr = Malloc (sizeof (t_symbol *) * count);
  count = 0;
  for (he = top; he; he = (t_symbol *) SYMBOL_EQUAL(he))
    symarr[count++] = he;

  /* unlink first, sort later: this dramatically improves the
   * performance of the unlink function */
  for (tel = 0; tel < count; tel++)
    SymbolTableUnlink (table, symarr[tel]);

  pobj=SYMBOL_TABLE_OBJECT(table);
  diablo_stable_sort (symarr, count, sizeof (t_symbol *), __cmp_symb_addresses);

  SymbolTableInsert (table, symarr[0]);
  for (tel = count - 1; tel >= 1; tel--)
  {
    SymbolTableInsert (table, symarr[tel]);
  }
  Free (symarr);
}

static int __helper_sort_symrefs(const void *a, const void *b)
{
  const t_symbol_ref *A = *(const t_symbol_ref **)a;
  const t_symbol_ref *B = *(const t_symbol_ref **)b;

  if (AddressIsLe(SYMBOL_OFFSET_FROM_START(A->sym),
                  SYMBOL_OFFSET_FROM_START(B->sym)))
    return -1;
  return 1;
}
void RelocatableSortSymRefs(t_relocatable *r)
{
  t_symbol_ref *ref;
  int i, nrefs = 0;
  t_symbol_ref **arr;

  for (ref = RELOCATABLE_REFED_BY_SYM(r); ref; ref = ref->next)
    ++nrefs;

  if (nrefs < 2) return;

  arr = Malloc(nrefs*sizeof(t_symbol_ref *));
  i = 0;
  for (ref = RELOCATABLE_REFED_BY_SYM(r); ref; ref = ref->next)
    arr[i++] = ref;

  diablo_stable_sort(arr, nrefs, sizeof (t_symbol_ref *), __helper_sort_symrefs);
  for (i = 0; i < nrefs-1; ++i)
  {
    arr[i]->next = arr[i+1];
    arr[i+1]->prev = arr[i];
  }
  arr[0]->prev = NULL;
  arr[nrefs-1]->next = NULL;
  RELOCATABLE_SET_REFED_BY_SYM(r, arr[0]);
  Free(arr);
}


/* hash table containing t_string -> t_string mapping for translating
 * (certain) symbol names. The translation has to be performed when
 * inserting new symbols, but at the caller side. It cannot be done
 * here because there are e.g. for ARM ads translations like this:
 *   __rt_locale -> __rt_locale_default
 *   __rt_locale_intlibspace -> __rt_locale
 * So if we perform the translation here, we may eventually
 * translate __rt_locale_intlibspace into __rt_locale_default
 * when symbols are inserted twice, with the second time via
 * SYMBOL_NAME(sym).
 */
static t_hash_table *symtranstable;
static t_hash_table *symrevtranstable;

typedef struct {
  t_hash_table_node node;
  t_string translated_name;
} t_sym_trans_item;

static void
FreeSymTransItem(const void *item, void *para)
{
  Free(((t_sym_trans_item*)item)->translated_name);
  Free(item);
}

void
SymbolInit()
{
  symtranstable = HashTableNew (1001, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, FreeSymTransItem);
  symrevtranstable = HashTableNew (1001, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, FreeSymTransItem);
}

void
AddTranslatedSymbolName(t_const_string orgname, t_const_string newname)
{
  t_sym_trans_item *it;

  VERBOSE(1,("Adding translation of \"%s\" -> \"%s\"",orgname,newname));
  it = (t_sym_trans_item*)HashTableLookup(symtranstable,orgname);
  if (it)
    FATAL(("Adding same name name twice to symbol name translation table. Old: %s -> %s. New: %s -> %s",
          HASH_TABLE_NODE_KEY(&it->node),it->translated_name,orgname,newname));

  it = Malloc(sizeof(t_sym_trans_item));
  HASH_TABLE_NODE_SET_KEY(&it->node,StringDup(orgname));
  it->translated_name = StringDup(newname);
  HashTableInsert(symtranstable,it);

  it = Malloc(sizeof(t_sym_trans_item));
  HASH_TABLE_NODE_SET_KEY(&it->node,StringDup(newname));
  it->translated_name = StringDup(orgname);
  HashTableInsert(symrevtranstable,it);
}

t_const_string
GetTranslatedSymbolName(t_const_string name)
{
  t_sym_trans_item *it;

  if (!symtranstable)
    return name;

  VERBOSE(1,("Looking up %s for translation",name));
  it = (t_sym_trans_item*)HashTableLookup(symtranstable,name);
  if (!it)
    return name;
  VERBOSE(1,("Translated %s -> %s",name,it->translated_name));
  return it->translated_name;
}

t_const_string
GetReverseTranslatedSymbolName(t_const_string name)
{
  t_sym_trans_item *it;

  if (!symrevtranstable)
    return NULL;

  VERBOSE(1,("Looking up %s for translation",name));
  it = (t_sym_trans_item*)HashTableLookup(symrevtranstable,name);
  if (!it)
    return NULL;
  VERBOSE(1,("Untranslated %s -> %s",name,it->translated_name));
  return it->translated_name;
}

/* Small helper function */
static size_t GetTranslatedSymbolLine(t_string *line, size_t *len, FILE *file)
{
  ssize_t res = getline(line, len, file);
  if (res<=1)
    return SIZE_MAX;
  (*line)[res-1]='\0';
  return 0;
}

void TryParseSymbolTranslation(t_const_string objectfilename)
{
  t_const_string fname, fullname;
  FILE *file;
  t_string orgname = NULL, newname = NULL;
  t_string org2name = NULL, new2name = NULL;
  size_t orglen = 0, newlen = 0, org2len = 0, new2len = 0;

  fname = StringConcat2(objectfilename, ".symtrans");
  fullname = FileFind(diabloobject_options.objpath, fname);
  Free(fname);

  if (!fullname)
    return;

  file = fopen(fullname, "r");
  Free(fullname);

  if (!file)
    return;

  while (GetTranslatedSymbolLine(&orgname, &orglen, file) != SIZE_MAX)
  {
    ASSERT(GetTranslatedSymbolLine(&newname, &newlen, file) != SIZE_MAX,("Missing new symbol name after %s", orgname));
 
    /* check for swap pattern */
    if (!strcmp(newname,"$$Temp"))
    {
      ASSERT(GetTranslatedSymbolLine(&newname, &newlen, file) != SIZE_MAX,("Missing new symbol name after first $$Temp"));
      ASSERT(GetTranslatedSymbolLine(&org2name, &org2len, file) != SIZE_MAX,("Missing org symbol name after first $$Temp"));
      ASSERT(!strcmp(org2name, orgname),("Original names in swap pattern don't match: %s != %s",orgname, org2name));
      ASSERT(GetTranslatedSymbolLine(&org2name, &org2len, file) != SIZE_MAX,("Missing old symbol in last part of swap pattern"));
      ASSERT(GetTranslatedSymbolLine(&new2name, &new2len, file) != SIZE_MAX,("Missing new symbol in last part of swap pattern"));
      ASSERT(!strcmp("$$Temp", org2name),("$$Temp doesn't match in last part of swap")); 
      ASSERT(!strcmp(newname, new2name),("Org names don't match in last part of swap: %s != %s", newname, new2name)); 
      AddTranslatedSymbolName(newname, orgname);
    }
    AddTranslatedSymbolName(orgname, newname);
    free(orgname);
    orgname=NULL;
    free(newname);
    newname=NULL;
    orglen=0;
    newlen=0;
  }

  fclose(file);
}

void
SymbolFini()
{
  if (symtranstable)
  {
    HashTableFree(symtranstable);
  }
  if (symrevtranstable)
  {
    HashTableFree(symrevtranstable);
  }
}

/*
 * Parses an instance of the $diablo symbols added by our assembler patches
 * returning 1 if there was an error.
 * When successful the data at the symbol location will be returned with the
 * t_uint64* data pointer and the expression string with *expr.
 */
int
SymbolDiabloSymbolReadData(const t_symbol *sym, t_uint64 *data, t_string *expr)
{
  int leb = -1;       /* -1 no, 0 unsigned , 1 signed */
  t_section *sec = T_SECTION(SYMBOL_BASE(sym));
  int nbytes;
  char *end;
  t_string symstr = SYMBOL_NAME (sym) + /*$diablo:*/ 8;
  int length = 0;

  /* get the u/s leb128 situation */
  if (strcspn(symstr, ":"))
  {
    ASSERT (*symstr == 'u' || *symstr == 's',
        ("Failed to parse a $diablo symbol (%s): illegal leb128 identifier %c",
         SYMBOL_NAME(sym), *symstr));
    leb = (*symstr == 'u') ? 0 : 1;
    symstr++;
  }

  /* get the size if know */
  nbytes = strtol(++symstr, &end, 10);
  symstr = end + 1;

  /* read the data accordingly */
  if (leb == -1)
  {
    ASSERT (nbytes != 0L, ("Failed to parse a $diablo symbol (%s): neither leb128 \
          or size were specified", SYMBOL_NAME(sym)));
    switch (nbytes)
    {
      case 1:
        *data = SectionGetData8 (sec, SYMBOL_OFFSET_FROM_START(sym));
        break;
      case 2:
        *data = SectionGetData16 (sec, SYMBOL_OFFSET_FROM_START(sym));
        break;
      case 4:
        *data = SectionGetData32 (sec, SYMBOL_OFFSET_FROM_START(sym));
        break;
      case 8:
        *data = SectionGetData64 (sec, SYMBOL_OFFSET_FROM_START(sym));
        break;
      default:
        FATAL (("Failed to parse a $diablo symbol (%s): illegal number of \
              bytes.", SYMBOL_NAME (sym)));
    }

    length = nbytes;
  } else {
    /* parse leb */
    *data = ((leb == 0) ? SectionGetULEB128 (sec, SYMBOL_OFFSET_FROM_START(sym), &length)
        : SectionGetSLEB128 (sec, SYMBOL_OFFSET_FROM_START(sym), &length));
  }

  /* return the remaining expression */
  *expr = symstr;

  /* if we got to this point, success! */
  return length;
}

/*
 * Parses expressions extracted from $diablo symbols. These expressions are
 * evaluated by the assembler into constants and preserved in the $diablo symbols.
 *
 * DiabloSymbolParseExpression parses into tokens and then convert the expressions
 * to a diablo stack language expression string. The symbol names found in the
 * expression are also returned in the arguments provided.
 */
t_const_string
SymbolDiabloSymbolExprToRelocString(t_const_string expr, t_string *sa, t_string *sb)
{
  /* tokenize expression {{{*/
  char const *it = expr;
  int symbol_count = 0;

  /* initialize output parameters */
  *sa = NULL;
  *sb = NULL;

  /* infix token list */
  t_ptr_array *tokens = Malloc(sizeof(t_ptr_array));
  PtrArrayInit(tokens, FALSE);

  /* postfix token list */
  t_ptr_array *pf_tokens = Malloc(sizeof(t_ptr_array));
  PtrArrayInit(pf_tokens, FALSE);

  /* we don't handle empty string */
  ASSERT (*it != 0, ("SymbolDiabloSymbolExprToRelocString; doesn't handle empty strings!"));

  do {
    ASSERT (dst_valid(dst_tab, *it),
        ("SymbolDiabloSymbolExprToRelocString; unhandled character '%c' (=%d) found in expression:\n%s\n\t%*s",
         *it, *it, expr, (it-expr), "\t^"));

    /* eat whitespace */
    while (dst_whitespace(dst_tab, *it))
      ++it;

    /* eat symbolname or integer */
    if (dst_symbolname(dst_tab, *it))
    {
      t_bool is_integer = TRUE;
      char const *end, *start = it;
      while (*it &&
          !dst_whitespace(dst_tab, *it) &&
          !dst_arithmetic(dst_tab, *it))
      {
        is_integer = (dst_digit(dst_tab, *it) && is_integer);
        it++;
      }
      end = it;

      /* handle integer */
      if (is_integer)
      {
        int result = strtol(start, (char**)&end, 10);
        ASSERT (end == it || result == 0L,
            ("SymbolDiabloSymbolExprToRelocString; error parsing integer in expression:\n%s\n\t%*s",
             expr, (it-expr), "\t^"));

        PtrArrayAdd(tokens, (DSTC DST_INTEGER));
        PtrArrayAdd(tokens, (void *)(long)result);
      } else {
        /* handle symbol name */
        PtrArrayAdd(tokens, (DSTC DST_SYMBOL));

        /* Return symbol names too */
        switch (symbol_count)
        {
            case 0:
                *sa = StringnDup(start, end-start);
                PtrArrayAdd(tokens, *sa);
                break;
            case 1:
                *sb = StringnDup(start, end-start);
                PtrArrayAdd(tokens, *sb);
                break;
            default:
                FATAL (("SymbolDiabloSymbolExprToRelocString; expression can have no more than 2 symbols."));
        }
        symbol_count++;
      }
      /* handle arithmetic symbols, we group the tokens for now, extend this if
         necesarry */
    } else if (dst_arithmetic(dst_tab, *it)) {
      switch (*it)
      {
        case '+': case '-': case '*': case '/': case '%':
          PtrArrayAdd(tokens, (DSTC DST_OP));
          PtrArrayAdd(tokens, (void*)(long)*it);
          break;
        case ')':
          PtrArrayAdd(tokens, (DSTC DST_RPAR));
          break;
        case '(':
          PtrArrayAdd(tokens, (DSTC DST_LPAR));
          break;
        default:
          FATAL (("SymbolDiabloSymbolExprToRelocString; unhandled arithmetic character: %c", *it));
      }
      ++it;
    }
  }
  while (*it);
  /* }}} */

  /* convert infix tokenlist to postfix {{{ */
  int i = 0;
  void *op, *st_op;

  t_ptr_array *op_stack = Malloc(sizeof(t_ptr_array));
  PtrArrayInit(op_stack, FALSE);

  do
  {
    switch ((long)PtrArrayGet(tokens, i++))
    {
      case DST_SYMBOL:
        PtrArrayAdd(pf_tokens, (DSTC DST_SYMBOL));
        PtrArrayAdd(pf_tokens, PtrArrayGet(tokens, i));
        break;

      case DST_INTEGER:
        PtrArrayAdd(pf_tokens, (DSTC DST_INTEGER));
        PtrArrayAdd(pf_tokens, PtrArrayGet(tokens, i));
        break;

      case DST_OP:
        op = PtrArrayGet(tokens, i);

        /* pop stack until it has an op with lower precedence */
        while ((st_op = PtrArrayGetLast(op_stack)) != NULL &&
            !(dst_precedence(st_op) < dst_precedence(op)) &&
            st_op != (DSTC DST_LPAR))
        {
          PtrArrayAdd(pf_tokens, (DSTC DST_OP));
          PtrArrayAdd(pf_tokens, st_op);
          PtrArrayRemoveLast(op_stack, FALSE); /* pop */
        }

        PtrArrayAdd(op_stack, op);  /* push */
        break;

      case DST_LPAR:
        /* lowest precedence so will always go to the stack */
        PtrArrayAdd(op_stack, (DSTC DST_LPAR));
        i--;
        break;

      case DST_RPAR:
        while ((st_op = PtrArrayGetLast(op_stack)) != NULL &&
            ( st_op != (DSTC DST_LPAR)))
        {
          PtrArrayAdd(pf_tokens, (DSTC DST_OP));
          PtrArrayAdd(pf_tokens, st_op);
          PtrArrayRemoveLast(op_stack, FALSE); /* pop */
        }

        if (PtrArrayCount(op_stack) == 0)
          FATAL (("SymbolDiabloSymbolExprToRelocString; infix to postfix failed, no matching left parenthesis"));

        PtrArrayRemoveLast(op_stack, FALSE); /* pop left parenthes itself */
        i--;
        break;

      default:
        FATAL (("SymbolDiabloSymbolExprToRelocString; unreachable."));
    }
  } while (++i < PtrArrayCount(tokens));
   
  /* pop remaining stack */
  while ((st_op = PtrArrayGetLast(op_stack)) != NULL)
  {
    PtrArrayAdd(pf_tokens, (DSTC DST_OP));
    PtrArrayAdd(pf_tokens, st_op);
    PtrArrayRemoveLast(op_stack, FALSE); /* pop */
  }

  PtrArrayFini(op_stack, FALSE);
  Free((t_ptr_array *)op_stack);

  PtrArrayFini(tokens, FALSE);
  Free((t_ptr_array *)tokens);
  /* }}} */

  /* convert tokens to diablo stack language {{{ */
  symbol_count = 0;
  /* TODO find better solution; max 2 symbols 'RxxAxx+', assume 3 chars for rest */
  int expr_max_size = 14 * (3*(PtrArrayCount(pf_tokens)-2));
  char *stackexpr = Malloc(expr_max_size + strlen(WRITE_32) + 1);
  char *expr_ptr = stackexpr;

  for (i=0; i < PtrArrayCount(pf_tokens); i+=2)
  {

    ASSERT (expr_ptr <= stackexpr + expr_max_size,
            ("SymbolDiabloSymbolExprToRelocString; reserverd stack language "
             "expression size was too small."));

    switch ((long)PtrArrayGet(pf_tokens, i))
    {
      case DST_SYMBOL:
        ASSERT ( symbol_count < 2,
            ("SymbolDiabloSymbolExprToRelocString; a maximum of 2 symbols can "
             "be in an expression, something went wrong!"));

        expr_ptr += sprintf(expr_ptr, "%s", (symbol_count == 0) ? "R00A01+" : "R01A02+");
        ++symbol_count;
        break;

      case DST_INTEGER:
        expr_ptr += sprintf(expr_ptr, "i%08d", (int)(long)PtrArrayGet(pf_tokens, i+1));
        break;

      case DST_OP:
        expr_ptr += sprintf(expr_ptr, "%c", (int)(long)PtrArrayGet(pf_tokens, i+1));
        break;
    }
  }
  /* }}} */

  PtrArrayFini(pf_tokens, FALSE);
  Free((t_ptr_array *)pf_tokens);
  
  sprintf(expr_ptr, "\\%s", WRITE_32);
  return stackexpr;
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
