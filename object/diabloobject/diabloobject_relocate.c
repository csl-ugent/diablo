/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>
#include <string.h>

typedef struct _t_one_sym_to_relocatable_info
{
  char * symbol_code;
  char * undef_code;
  t_uint32 symbol_code_len;
  t_uint32 undef_code_len;
  t_int32 * RtoRmap;
  t_int32 * AtoAmap;
} t_one_sym_to_relocatable_info;

typedef struct  _t_sym_to_relocatable_info
{
  t_uint32 mapped_syms;
  t_one_sym_to_relocatable_info * symbols;
} t_sym_to_relocatable_info;

static void SymToRelocInfoFreeData(t_sym_to_relocatable_info * info)
{
  t_uint32 j;
  for (j=0; j<info->mapped_syms; j++)
  {
    if (info->symbols[j].symbol_code) Free(info->symbols[j].symbol_code);
    if (info->symbols[j].undef_code) Free(info->symbols[j].undef_code);
    if (info->symbols[j].RtoRmap) Free(info->symbols[j].RtoRmap);
    if (info->symbols[j].AtoAmap) Free(info->symbols[j].AtoAmap);
  }
  if (info->symbols) Free(info->symbols);
}

/* For a given reloc rel, create the code fragments that are needed to inline
 * the index-th symbol. Two different code fragments can be needed:  The code
 * to replace Sxx and the code to replace uxx with xx == index. As a reminder:
 * Sxx returns the value of a symbol, uxx returns whether the symbol is defined
 * or not.  */

static void RelocMigrateOneSymbol(t_sym_to_relocatable_info * sym_reloc_map,
                                  t_reloc * rel, t_uint32 index) 
{
  /* The first free relocatable */	
  t_uint32 free_relocatable=RELOC_N_TO_RELOCATABLES(rel);
  t_uint32 free_addend=RELOC_N_ADDENDS(rel);

  /* Check if the index-th symbol exists in rel */
  ASSERT(index<RELOC_N_TO_SYMBOLS(rel),
         ("Reloc @R code %s refers symbol that does not exist!",
          rel,RELOC_CODE(rel)));

  /* Allocate space in the symbol map if necessary. The symbol map records the
   * code used to inline a symbol. Subsequent inlining operations can reuse
   * this code. */
  if ((index+1)>sym_reloc_map->mapped_syms) 
  {
    sym_reloc_map->symbols=Realloc(sym_reloc_map->symbols, (index+1) * sizeof(t_one_sym_to_relocatable_info));

    for (; sym_reloc_map->mapped_syms<(index+1); sym_reloc_map->mapped_syms++) 
    {
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].symbol_code = NULL;
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].undef_code = NULL;
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].RtoRmap = Malloc(sizeof(t_int32)*1);
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].RtoRmap[0] = -1;
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].AtoAmap = Malloc(sizeof(t_int32)*1);
      sym_reloc_map->symbols[sym_reloc_map->mapped_syms].AtoAmap[0] = -1;
    }
  }

  if (sym_reloc_map->symbols[index].symbol_code==NULL)
  {
    t_uint32 nextra=0;
    t_uint32 naddextra=0;
    t_uint32 j;
    t_uint32 nrelocatables=1;
    t_uint32 naddends=1;
    t_string_array * symcode = StringArrayNew ();
    t_symbol * sym=RELOC_TO_SYMBOL(rel)[index];

    for (j=0; j<strlen(SYMBOL_CODE(sym)); j++)
    {
      t_string code=SYMBOL_CODE(sym);

      if (code[j]=='$')
      {
        break;
      } 
      else if (code[j]=='A')
      {
        t_uint32 aindex;
        ASSERT((((code[j+1]) >= '0') && ((code[j+1]) <= '9')), ("Depredecated A operand without suffix in %s!", code));
        aindex = 10 * ((code[j+1]) - '0');
        ASSERT((((code[j+2]) >= '0') && ((code[j+2]) <= '9')), ("Depredecated A operand without suffix in %s!", code));
        aindex += ((code[j+2]) - '0');
        j+=2;
        
        ASSERT (aindex<naddends, ("A operand refers to non existing addend"));

        if (sym_reloc_map->symbols[index].AtoAmap[aindex]==-1)
        {
          sym_reloc_map->symbols[index].AtoAmap[aindex]=free_addend++;
          naddextra++;
        }
        
        StringArrayAppendString(symcode, StringIo("A%02d",sym_reloc_map->symbols[index].AtoAmap[aindex]));
      }
      else if (code[j]=='R')
      {
        t_uint32 rindex;
        ASSERT((((code[j+1]) >= '0') && ((code[j+1]) <= '9')), ("Depredecated R operand without suffix in %s!", code));
        rindex = 10 * ((code[j+1]) - '0');
        ASSERT((((code[j+2]) >= '0') && ((code[j+2]) <= '9')), ("Depredecated R operand without suffix in %s!", code));
        rindex += ((code[j+2]) - '0');
        j+=2;

        ASSERT (rindex<nrelocatables, ("R operand refers to non existing relocatable"));

        if (sym_reloc_map->symbols[index].RtoRmap[rindex]==-1)
        {
          sym_reloc_map->symbols[index].RtoRmap[rindex]=free_relocatable++;
          nextra++;
        }

        StringArrayAppendString(symcode, StringIo("R%02d",sym_reloc_map->symbols[index].RtoRmap[rindex]));
      }
      else if (code[j]=='U')
      {
        t_uint32 rindex;
        ASSERT((((code[j+1]) >= '0') && ((code[j+1]) <= '9')), ("U operand without suffix in %s!", code));
        rindex = 10 * ((code[j+1]) - '0');
        ASSERT((((code[j+2]) >= '0') && ((code[j+2]) <= '9')), ("U operand without suffix in %s!", code));
        rindex += ((code[j+2]) - '0');
        j+=2;

        ASSERT (rindex<nrelocatables, ("R operand refers to non existing relocatable"));

        if (sym_reloc_map->symbols[index].RtoRmap[rindex]==-1)
        {
          sym_reloc_map->symbols[index].RtoRmap[rindex]=free_relocatable++;
          nextra++;
        }

        StringArrayAppendString(symcode, StringIo("U%02d",sym_reloc_map->symbols[index].RtoRmap[rindex]));
      }
      else if (code[j]=='Z')
      {
        t_uint32 rindex;
        ASSERT((((code[j+1]) >= '0') && ((code[j+1]) <= '9')), ("Depredecated Z operand without suffix in %s!", code));
        rindex = 10 * ((code[j+1]) - '0');
        ASSERT((((code[j+2]) >= '0') && ((code[j+2]) <= '9')), ("Depredecated Z operand without suffix in %s!", code));
        rindex += ((code[j+2]) - '0');
        j+=2;


        ASSERT (rindex<nrelocatables, ("R operand refers to non existing relocatable"));

        if (sym_reloc_map->symbols[index].RtoRmap[rindex]==-1)
        {
          sym_reloc_map->symbols[index].RtoRmap[rindex]=free_relocatable++;
          nextra++;
        }

        StringArrayAppendString(symcode, StringIo("Z%02d",sym_reloc_map->symbols[index].RtoRmap[rindex]));
      }
      else
      {
        StringArrayAppendString(symcode, StringIo("%c",code[j]));
      }
    }


    /* The symbol refered to nextra relocatables. We move them into the
     * to_relocatable array of the reloc. */

    if (nextra)
    {
      t_object *obj = SYMBOL_TABLE_OBJECT(SYMBOL_SYMBOL_TABLE(sym));

      RELOC_SET_N_TO_RELOCATABLES(rel, RELOC_N_TO_RELOCATABLES(rel) + nextra);
      RELOC_SET_TO_RELOCATABLE(rel, Realloc(RELOC_TO_RELOCATABLE(rel),sizeof(t_relocatable *)*RELOC_N_TO_RELOCATABLES(rel)));
      RELOC_SET_TO_RELOCATABLE_OFFSET(rel, Realloc(RELOC_TO_RELOCATABLE_OFFSET(rel),sizeof(t_address)*RELOC_N_TO_RELOCATABLES(rel)));
      RELOC_SET_TO_RELOCATABLE_REF(rel, Realloc(RELOC_TO_RELOCATABLE_REF(rel),sizeof(t_reloc_ref *)*RELOC_N_TO_RELOCATABLES(rel)));
      for (j=0; j<nextra; j++) 
      {
        t_address symoffs;

        RELOC_TO_RELOCATABLE(rel)[RELOC_N_TO_RELOCATABLES(rel) - 1]=NULL; // SYMBOL_BASE(sym);
        RELOC_TO_RELOCATABLE_REF(rel)[RELOC_N_TO_RELOCATABLES(rel)-1]=NULL; //RELOC_TO_SYMBOL_REF(rel, index);
        RelocSetToRelocatable (rel, RELOC_N_TO_RELOCATABLES(rel) - 1, SYMBOL_BASE(sym));
        symoffs = SYMBOL_OFFSET_FROM_START(sym);
        if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
          symoffs = AddressInverseMaskUint32(symoffs,OBJECT_CODE_SYMBOL_ADDRESS_INVERSE_MASK(obj));
        RELOC_TO_RELOCATABLE_OFFSET(rel)[RELOC_N_TO_RELOCATABLES(rel) - 1] = AddressAdd(symoffs,RELOC_TO_SYMBOL_OFFSET(rel)[index]);

        /* if there is already a label, this means the relocation is
         * annotated. For example, relocations to removed sections now
         * point to the undefined section, but their label is
         * DIABLO_REMOVED_SYM. */

        if (!RELOC_LABEL(rel)) RELOC_SET_LABEL(rel, SYMBOL_NAME(sym) ? StringDup (SYMBOL_NAME(sym)) : StringDup ("Noname")); 
      } 
    }

    if (naddextra)
    {
      RELOC_SET_N_ADDENDS(rel, RELOC_N_ADDENDS(rel) + naddextra);
      RELOC_SET_ADDENDS(rel, Realloc(RELOC_ADDENDS(rel),sizeof(t_address)*RELOC_N_ADDENDS(rel)));
      for (j=0; j<naddextra; j++) 
      { 
        RELOC_ADDENDS(rel)[ RELOC_N_ADDENDS(rel) - 1] = SYMBOL_ADDEND(sym);
      }
    }

    sym_reloc_map->symbols[index].symbol_code= StringArrayJoin (symcode, "");
    sym_reloc_map->symbols[index].symbol_code_len = strlen(sym_reloc_map->symbols[index].symbol_code);
    StringArrayFree(symcode);
  }

  if (sym_reloc_map->symbols[index].undef_code==NULL)
  {
    t_uint32 nrelocatables=1;
    t_uint32 j;
    t_string_array * symcode = StringArrayNew ();
    t_uint32 nextra=0;

    for (j=0; j<nrelocatables; j++)
    {
      if (sym_reloc_map->symbols[index].RtoRmap[j]==-1)
      {
        sym_reloc_map->symbols[index].RtoRmap[j]=free_relocatable++;
        nextra++;
      }
      if (j==0)
        StringArrayAppendString(symcode, StringIo("U%02d",sym_reloc_map->symbols[index].RtoRmap[j]));
      else
        StringArrayAppendString(symcode, StringIo("U%02d&",sym_reloc_map->symbols[index].RtoRmap[j]));
    }

    if (nextra)
      FATAL(("Implement MigrateToRelocatables for reloc @R and symbol @S",rel, RELOC_TO_SYMBOL(rel)[index]));

    sym_reloc_map->symbols[index].undef_code = StringArrayJoin (symcode, "");
    sym_reloc_map->symbols[index].undef_code_len = strlen(sym_reloc_map->symbols[index].undef_code);
    StringArrayFree(symcode);
  }
}

void FixRelocsToLinkerCreatedSymbols(t_object * obj)
{
  t_reloc *rel=NULL;
  
  t_symbol * symbol;
  t_symbol * symbol2;

  /* for ARM:

     The symbols ORIG:__exidx_start and ORIG:__exidx_end are placed in *UNDEF* sections
     when they were created when tentative symbols were resolved. The reason is 
     that Diablo first created symbols for __exidx_start and __exidx_end for GOT32
     relocations in unwind-arm.o, and those symbols were allocated in the *UNDEF* 
     section. After those symbols' creation, however, appropriate linker symbols
     were created in proper sections.

     In the following loop, we replace the ORIG: symbols by their corresponding, proper
     linker symbols in the relevant relocations (in the GOT), such that those relocs are 
     migrated to point to the proper sections instead of to the *UNDEF* section 
  */

  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), symbol)
    {
      if (OBJECT_UNDEF_SECTION(obj)==T_SECTION(SYMBOL_BASE(symbol)))
        if (StringPatternMatch("ORIG:*",SYMBOL_NAME(symbol)))
          {
            SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), symbol2)
              {
                if (OBJECT_UNDEF_SECTION(obj)==T_SECTION(SYMBOL_BASE(symbol2))) continue;                
                if (strcmp(SYMBOL_NAME(symbol)+5,SYMBOL_NAME(symbol2))) continue;

                OBJECT_FOREACH_RELOC(obj,rel)
                  {
                    if (!RELOC_TO_SYMBOL(rel)) continue;
                    if (RELOC_TO_SYMBOL(rel)[0]!=symbol) continue;
                    VERBOSE(2,("found reloc @R",rel));
                    RelocSetToSymbol(rel,0,symbol2);
                    VERBOSE(2,("and replaced it by @R",rel));
                  }

              }
          }
    }  
}


/*!  \brief Migrate relocs from symbols to relocatables
 *
 * Indirect references (i.e. references via symbols) from relocations to
 * relocatables can be made direct by replacing all the references to symbols
 * inside the relocation's LDAM code by the LDAM code that computes the value
 * of the symbol, i.e., we inline the symbol's value code in the relocation's
 * LDAM code. This process considerably simplifies the handling of relocations
 * in subsequent phases.
 *
 */

void 
RelocsMigrateToRelocatables (t_object * obj) 
{ 
  t_reloc *rel=NULL;
  
  STATUS(START, ("Migrate Relocations to Relocatables"));
  
  if (!OBJECT_RELOC_TABLE(obj))
    FATAL(("RelocsMigrateToRelocatables called on an object that has no reloc table"));

  OBJECT_FOREACH_RELOC_R(obj, rel)
  {
    /* For each relocation we will look for occurences of Sxx and uxx in the
     * LDAM code of the relocation.  For each occurence
     * - Sxx is replaced by the symbols LDAM code, the refered relocatables are
     *   added to the relocation's list of relocatables and Rxx, Zxx,... are
     *   renamed to refer to the correct relocatable.
     * - uxx is replaced by the and of Uxx for each relocatable
     *
     * Mind that multiple references to the same symbol can occur. To avoid
     * multiple inclusions of the same relocatable, we keep a cache of
     * antecedent inlining operations.
     */
    
    t_uint32 len=strlen(RELOC_CODE(rel));
    t_uint32 outlen=0;
    t_uint32 o=0;
    t_uint32 i;

    /* String used to store the joined code of the final code string of the
     * relocation */
    t_string code2;

    /* The cache of antecedent inlining operations for this relocation */
    t_sym_to_relocatable_info sym_reloc_map={0, NULL};


    if (!RELOC_TO_SYMBOL(rel))
    {
      if (RELOC_N_TO_SYMBOLS(rel)!=0)
        FATAL(("Weird reloc (%d symbols) @R\n", RELOC_N_TO_SYMBOLS(rel), rel));
      continue;
    }

    /* Determine the length of the output string */
    for (i=0; i< len; i++)
    {
      /* Sxx -> inline */
      if (RELOC_CODE(rel)[i]=='S')
      {
        t_uint32 index=0;
        ASSERT((((RELOC_CODE(rel)[i+1]) >= '0') && ((RELOC_CODE(rel)[i+1]) <= '9')), ("Depredecated S operand without suffix in %s!", RELOC_CODE(rel)));
        index = 10 * ((RELOC_CODE(rel)[i+1]) - '0');
        ASSERT((((RELOC_CODE(rel)[i+2]) >= '0') && ((RELOC_CODE(rel)[i+2]) <= '9')), ("Depredecated S operand without suffix in %s!", RELOC_CODE(rel)));
        index += ((RELOC_CODE(rel)[i+2]) - '0');
        i+=2;

        if (index<RELOC_N_TO_SYMBOLS(rel))
        {
          /* an 'm' following an Sxx means that the modus needs to be or'ed
           * *iff* the symbol refers to code. We cannot do this when
           * generating the initial formula, because the symbol may still be
           * unresolved at that point (and hence we don't know whether it
           * refers to code). At this point it has been resolved, so if it
           * does refer to code, replace the m with an M
           * so we check if the symbol was a function, in that case the modus is really needed
           * and we mark this by replacing the 'm' by an 'M'
           */
          t_symbol * sym = RELOC_TO_SYMBOL(rel)[index];
          if (sym)
            if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
              if (RELOC_CODE(rel)[i+1]=='m')
                RELOC_CODE(rel)[i+1]='M';
        }

        RelocMigrateOneSymbol(&sym_reloc_map,rel,index);
        outlen+=sym_reloc_map.symbols[index].symbol_code_len;
      }

      /* Zxx -> migrate corresponding symbol */
      else if (RELOC_CODE(rel)[i]=='Z')
      {
        t_uint32 index=0;
        ASSERT((((RELOC_CODE(rel)[i+1]) >= '0') && ((RELOC_CODE(rel)[i+1]) <= '9')), ("Depredecated S operand without suffix in %s!", RELOC_CODE(rel)));
        index = 10 * ((RELOC_CODE(rel)[i+1]) - '0');
        ASSERT((((RELOC_CODE(rel)[i+2]) >= '0') && ((RELOC_CODE(rel)[i+2]) <= '9')), ("Depredecated S operand without suffix in %s!", RELOC_CODE(rel)));
        index += ((RELOC_CODE(rel)[i+2]) - '0');
        i+=2;
        
        
        RelocMigrateOneSymbol(&sym_reloc_map,rel,index);
	outlen+=3;
      }
      /* uxx -> replace with Uxx Uxx & Uxx & .... */
      else if (RELOC_CODE(rel)[i]=='u')
      {
        t_uint32 index=0;
        ASSERT((((RELOC_CODE(rel)[i+1]) >= '0') && ((RELOC_CODE(rel)[i+1]) <= '9')), ("u operand without suffix in %s!", RELOC_CODE(rel)));
        index = 10 * ((RELOC_CODE(rel)[i+1]) - '0');
        ASSERT((((RELOC_CODE(rel)[i+2]) >= '0') && ((RELOC_CODE(rel)[i+2]) <= '9')), ("u operand without suffix in %s!", RELOC_CODE(rel)));
        index += ((RELOC_CODE(rel)[i+2]) - '0');
        i+=2;
        
        RelocMigrateOneSymbol(&sym_reloc_map,rel,index);
	outlen+=sym_reloc_map.symbols[index].undef_code_len;
      }
      /* the rest -> copy verbatim */
      else
	      outlen++;
    }

    code2 = Malloc(outlen+1);


    /* Process one token in the relocation string at a time */
    for (i=0; i< len; i++)
    {
      /* Sxx -> inline */
      if (RELOC_CODE(rel)[i]=='S')
      {
        t_uint32 index=0;
        index = 10 * ((RELOC_CODE(rel)[i+1]) - '0');
        index += ((RELOC_CODE(rel)[i+2]) - '0');
        i+=2;

        if (RELOC_CODE(rel)[i+1]=='m')
        {
          /* if above the 'm' in Sxxm| has not been replaced by an 'M', it
           * means the whole Sxxm| was not needed after all, so we simply
           * skip it completely
           */
          ASSERT(RELOC_CODE(rel)[i+2]=='|',("potential modus not followed by | in @R",rel));
          /* skip "m|" */
          i+=2;
        }
        else
        {
          strcpy(code2+o, sym_reloc_map.symbols[index].symbol_code);
          o+=sym_reloc_map.symbols[index].symbol_code_len;
        }

      }
      /* uxx -> replace with Uxx Uxx & Uxx & .... */
      else if (RELOC_CODE(rel)[i]=='u')
      {
        t_uint32 index=0;
        index = 10 * ((RELOC_CODE(rel)[i+1]) - '0');
        index += ((RELOC_CODE(rel)[i+2]) - '0');
        i+=2;
        
	strcpy(code2+o, sym_reloc_map.symbols[index].undef_code); 
	o+=sym_reloc_map.symbols[index].undef_code_len;
      }
      /* the rest -> copy verbatim */
      else
      {
	      code2[o]=RELOC_CODE(rel)[i];
	      o++;
      }
    }

    code2[o]= '\0';

    /* Produce the final code string */
    Free(RELOC_CODE(rel));
    RELOC_SET_CODE(rel, code2);


    /* By now, we can throw away all symbols */
 
    for (i = 0; i< RELOC_N_TO_SYMBOLS(rel); i++)
    {
	    RelocRemoveToSymbolRef(rel, i);
    }
    RELOC_SET_N_TO_SYMBOLS(rel, 0);
    Free(RELOC_TO_SYMBOL(rel));
    RELOC_SET_TO_SYMBOL(rel, NULL);

    /* Free the cache */
    SymToRelocInfoFreeData(&sym_reloc_map);
  }

  STATUS(STOP, ("Migrate Relocations to Relocatables"));


}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
