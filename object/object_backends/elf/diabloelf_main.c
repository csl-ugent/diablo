/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloelf.h>

/* UNIX ELF hash function (needed to build/use the dynamic hash tables) */
t_uint32 
ElfHash(char *name)
{
  t_uint32 h = 0, g;
  while ( *name ) {
    h = ( h << 4 ) + *name++;
    if ( (g = (h & 0xF0000000)) != 0)
      h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

/* Code taken from binutils 2.16.1/bfd and changed to better fit Diablo */

/* Array used to determine the number of hash table buckets to use
   based on the number of symbols there are.  If there are fewer than
   3 symbols we use 1 bucket, fewer than 17 symbols we use 3 buckets,
   fewer than 37 we use 17 buckets, and so forth.  We never use more
   than 32771 buckets.  */


static const size_t elf_buckets[] =
{
  1, 3, 17, 37, 67, 97, 131, 197, 263, 521, 1031, 2053, 4099, 8209,
  16411, 32771, 0
};


/* This function will be called though elf_link_hash_traverse to store
   all hash value of the exported symbols in an array.  */
t_bool
elf_collect_hash_codes (t_symbol * sym, void *data)
{
  t_uint32 **valuep = data;
  t_uint32 ha;
  char *alc = NULL;

  /* Ignore indirect symbols.  These are added by the versioning code.  */
  /*if (h->dynindx == -1)
    return TRUE;
*/
  /* TODO: Versioning 
  name = h->root.root.string;
  p = strchr (name, ELF_VER_CHR);
  if (p != NULL)
    {
      alc = bfd_malloc (p - name + 1);
      memcpy (alc, name, p - name);
      alc[p - name] = '\0';
      name = alc;
    }
    */

  /* Compute the hash value.  */
  if (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_SECTION))
    ha = ElfHash (SYMBOL_NAME(sym));
  else
    /* section symbols have an empty name in the final binary */
    ha = ElfHash ("");

  /* Store the found hash value in the array given as the argument.  */
  *(*valuep)++ = ha;

  if (alc != NULL)
    free (alc);

  return TRUE;
}
/* Compute bucket count for hashing table.  We do not use a static set
   of possible tables sizes anymore.  Instead we determine for all
   possible reasonable sizes of the table the outcome (i.e., the
   number of collisions etc) and choose the best solution.  The
   weighting functions are not too simple to allow the table to grow
   without bounds.  Instead one of the weighting factors is the size.
   Therefore the result is always a good payoff between few collisions
   (= short chain lengths) and table size.  */
t_uint32
ElfComputeBucketCount (t_object * obj, t_uint32 symsize, t_uint32 pagesize, t_uint32 optimize)
{
  t_uint32 ndynsyms = SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj));
  t_uint32 best_size = 0;
  t_uint32 *hashcodes;
  t_uint32 *hashcodesp;
  t_uint32 i;
  t_symbol * symptr;

  /* Compute the hash values for all exported symbols.  At the same
     time store the values in an array so that we could use them for
     optimizations.  */
  hashcodes = Malloc (sizeof (t_uint32) * ndynsyms);
  if (hashcodes == NULL)
    return 0;
  hashcodesp = hashcodes;

  /* Put all hash values in HASHCODES.  */
  for (symptr=SYMBOL_TABLE_FIRST(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
  {
    elf_collect_hash_codes(symptr, &hashcodesp);
  }

  if (optimize)
  {
    t_uint32 nsyms = hashcodesp - hashcodes;
    t_uint32 minsize;
    t_uint32 maxsize;
    t_uint64 best_chlen = ~((t_uint64) 0);
    t_uint32 *counts ;

    /* Possible optimization parameters: if we have NSYMS symbols we say
       that the hashing table must at least have NSYMS/4 and at most
       2*NSYMS buckets.  */
    minsize = nsyms / 4;
    if (minsize == 0)
      minsize = 1;
    best_size = maxsize = nsyms * 2;

    /* Create array where we count the collisions in. */
    counts = Malloc (sizeof (t_uint32) * maxsize);
    if (counts == NULL)
    {
      Free (hashcodes);
      return 0;
    }


    printf("Assuming hash table size between %d and %d\n", minsize, maxsize);

    /* Compute the "optimal" size for the hash table.  The criteria is a
       minimal chain length.  The minor criteria is (of course) the size
       of the table.  */
    for (i = minsize; i < maxsize; ++i)
    {
      /* Walk through the array of hashcodes and count the collisions.  */
      t_uint64 max;
      t_uint32 j;
      t_uint32 fact;

      memset (counts, '\0', i * sizeof (t_uint32));

      /* Determine how often each hash bucket is used.  */
      for (j = 0; j < nsyms; ++j)
        ++counts[hashcodes[j] % i];

      /* For the weight function we need some information about the
         pagesize on the target.  This is information need not be 100%
         accurate.  Since this information is not available (so far) we
         define it here to a reasonable default value.  If it is crucial
         to have a better value some day simply define this value.  */

      /* We in any case need 2 + NSYMS entries for the size values and
         the chains.  */
      max = (2 + nsyms) * (symsize / 8);

      if (optimize == 1)
      {
        /* Variant 1: optimize for short chains.  We add the squares
           of all the chain lengths (which favors many small chain
           over a few long chains).  */
        for (j = 0; j < i; ++j)
          max += counts[j] * counts[j];

        /* This adds penalties for the overall size of the table.  */
        fact = i / (pagesize / (symsize / 8)) + 1;
        max *= fact * fact;
      }
      else
      {
        /* Variant 2: Optimize a lot more for small table.  Here we
           also add squares of the size but we also add penalties for
           empty slots (the +1 term).  */
        for (j = 0; j < i; ++j)
          max += (1 + counts[j]) * (1 + counts[j]);

        /* The overall size of the table is considered, but not as
           strong as in variant 1, where it is squared.  */
        fact = i / (pagesize / (symsize / 8)) + 1;
        max *= fact;
      }

      /* Compare with current best results.  */
      if (max < best_chlen)
      {
        best_chlen = max;
        best_size = i;
      }
    }

    Free (counts);
  }
  else
  {
    /* This is the fallback solution if no 64bit type is available or if we
       are not supposed to spend much time on optimizations.  We select the
       bucket count using a fixed set of numbers.  */
    for (i = 0; elf_buckets[i] != 0; i++)
    {
      best_size = elf_buckets[i];
      if (ndynsyms < elf_buckets[i + 1])
        break;
    }
  }

  /* Free the arrays we needed.  */
  Free (hashcodes);

  return best_size;
}

t_bool IsElf(FILE * fp)
{
   Elf32_Byte buffer[4];
   long fpos=ftell(fp);
   int ret=fread(buffer,sizeof(Elf32_Byte),4,fp); 
   fseek(fp,fpos,SEEK_SET);
   
   if (ret!=4) return FALSE;
   if ((buffer[0]!=ELFMAG0)||(buffer[1]!=ELFMAG1)||(buffer[2]!=ELFMAG2)||(buffer[3]!=ELFMAG3))
   {
      return FALSE;
   }
   VERBOSE(1,("File is an ELF objectfile")); 
   return TRUE;
}

t_bool IsElf32(FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  t_uint32 save = ftell (fp);

  if (fread (buffer, sizeof (Elf32_Byte), EI_NIDENT, fp) != EI_NIDENT)
  {
    fseek (fp, save, SEEK_SET);
    return FALSE;
  }

  fseek (fp, save, SEEK_SET);

  if ((buffer[EI_MAG0] != ELFMAG0) || (buffer[EI_MAG1] != ELFMAG1)
      || (buffer[EI_MAG2] != ELFMAG2) || (buffer[EI_MAG3] != ELFMAG3))
    return FALSE;

  switch (buffer[EI_CLASS])
  {
    case ELFCLASS32:
      break;
    case ELFCLASS64:
    case ELFCLASSNONE:
    default:
      return FALSE;
      break;
  }

  return TRUE;
}

/* This struct is used to pass the right type of elf header to the elf-reader */
typedef union 
{
   Elf32_Ehdr h32;
   Elf64_Ehdr h64;
} mixed_elf_hdr;

t_uint64 ElfGetSizeofHeaders(t_object * obj, const t_layout_script * layoutscript)
{
  t_uint64 elfhdrsz, phdrsz, shdrsz;
  int gaps;
  t_layout_rule * r;
  int prev = -1;
  /* this is a bit of a problem: we cannot accurately predict the number 
     of needed phdrs as long as we don't know the complete section layout.
     on the other hand, we need the size of the phdrs to determine the
     section layout :-(
     we'll just have to make an educated guess:
     # of phdrs = number of possible gaps (first placed section adds 1)
      +1: read-only and read-write data are put into separate sections
          (assume there's at least one read-only and one read-write section)
          -- handled via explicit SEGMENT declarations and
          PUT_REMAINING_SECTIONS in the linker scripts
      +2 for dynamically linked programs/libraries (interp, dynamic)
      +1 for dynamically linked libraries and pie binaries (phdr)
      +1 in case there is a TLS section (needs its own program header)
   */
  if (OBJECT_ADDRESS_SIZE(obj) == ADDRSIZE32)
  {
    elfhdrsz = sizeof(Elf32_Ehdr);
    phdrsz = sizeof(Elf32_Phdr);
    shdrsz = sizeof(Elf32_Shdr);
  }
  else if (OBJECT_ADDRESS_SIZE(obj) == ADDRSIZE64)
  {
    elfhdrsz = sizeof(Elf64_Ehdr);
    phdrsz = sizeof(Elf64_Phdr);
    shdrsz = sizeof(Elf64_Shdr);
  }
  else
  {
    FATAL(("Unknown address size for this object"));
    exit(0); /* Keeps the compiler happy */
  }

#define PREV_WAS_NOT_ASSIGN	1
#define PREV_WAS_ASSIGN		2


  {
     t_section * sec;
     t_uint32 tel;
     OBJECT_FOREACH_SECTION(obj,sec,tel)
     {
	SECTION_SET_TMP(sec, 0);
     }
  }
  
  /* now count the number of gaps */
  gaps = 0;
  prev = 3;
  for (r = layoutscript->first; r; r=r->next)
  {
    if (r->kind == ASSIGN)
    {
      if (strcmp(r->u.assign->lhs,".")) continue;
      prev = PREV_WAS_ASSIGN;
    }
    else if (r->kind == SECTION_SPEC) 
    {
       t_section * sec=SectionGetFromObjectByName(obj,r->u.secspec->name);
       if (!sec) continue;
       if (!SECTION_IS_MAPPED(sec)) continue;

       SECTION_SET_TMP(sec, 1);
       if (prev == PREV_WAS_ASSIGN || (prev == 3)) { gaps++; }
       prev = PREV_WAS_NOT_ASSIGN;
    }
    else if (r->kind == PUT_SECTIONS)
    {
       t_uint32 tel;
       t_uint32 nplaced=1;
       if (r->u.sectype==NOTE_SECTION)
       {
	  for (tel=0; tel<OBJECT_NNOTES(obj); tel++)
	     nplaced&=SECTION_TMP(OBJECT_NOTE(obj)[tel]);
       }
       else if (r->u.sectype=='C')
       {
	  for (tel=0; tel<OBJECT_NCODES(obj); tel++)
	     nplaced&=SECTION_TMP(OBJECT_CODE(obj)[tel]);
       }
       else if (r->u.sectype=='R')
       {
	  for (tel=0; tel<OBJECT_NRODATAS(obj); tel++)
	  {
	     nplaced&=SECTION_TMP(OBJECT_RODATA(obj)[tel]);
	  }
       }
       else if (r->u.sectype=='D')
       {
	  for (tel=0; tel<OBJECT_NDATAS(obj); tel++)
	  {
	     nplaced&=SECTION_TMP(OBJECT_DATA(obj)[tel]);
	  }
       }
       else if (r->u.sectype=='B')
       {
	  for (tel=0; tel<OBJECT_NBSSS(obj); tel++)
	     nplaced&=SECTION_TMP(OBJECT_BSS(obj)[tel]);
       }
       else if (r->u.sectype==DEBUG_SECTION)
       {
	  for (tel=0; tel<OBJECT_NDEBUGS(obj); tel++)
	     nplaced&=SECTION_TMP(OBJECT_DEBUG(obj)[tel]);
       }
       else if (r->u.sectype==ATTRIB_SECTION)
       {
	  for (tel=0; tel<OBJECT_NATTRIBS(obj); tel++)
	     nplaced&=SECTION_TMP(OBJECT_ATTRIB(obj)[tel]);
       }

       if (nplaced) continue;
	  
       if ((prev == PREV_WAS_ASSIGN) || (prev == 3)) { /*printf("Putsection\n");*/ gaps++; }
       prev = PREV_WAS_NOT_ASSIGN;
    }
    else if (r->kind == SEG_END)
    {
       prev=3;
    }
    else if (r->kind == SEG_START)
    {
    }
    else
      FATAL(("unexpected kind of rule"));
  }
  /* adjust if the last rule was an assignment (this creates no gap) */
  if (prev == PREV_WAS_ASSIGN) gaps--;
#undef PREV_WAS_NOT_ASSIGN
#undef PREV_WAS_ASSIGN

  /* tls header */
  if (OBJECT_NTLSDATAS(obj) + OBJECT_NTLSBSSS(obj) != 0)
    gaps++;
  /* dynamic-specific headers */
  if (OBJECT_DYNAMIC(obj))
    gaps+=2;
  /* phdr for dynamic libraries, binaries and pie binaries */
  /* for the dynamic binaries, this was added because libld functionality
     such as dladdr() depends on it */
  if ((OBJECT_TYPE(obj)==OBJTYP_SHARED_LIBRARY_PIC) ||
      (OBJECT_TYPE(obj)==OBJTYP_EXECUTABLE_PIC) ||
      (OBJECT_DYNAMIC(obj))
      )
    gaps++;

  if (OBJECT_GNU_STACK_FLAGS(obj))
    gaps++;

  if (OBJECT_RELRO_CSIZE(obj))
    gaps++;

  /* if we will create an additional EXIDX segment, we have to count that as well ... */
  if (diabloobject_options.keep_exidx)
    {
      t_section * sec;
      t_uint32 tel;
      OBJECT_FOREACH_SECTION(obj,sec,tel)
        {
          if (strcmp(SECTION_NAME(sec), ".ARM.exidx") == 0)
            gaps++;
        } 
    }

  return (t_uint64) elfhdrsz + gaps*phdrsz;
}
