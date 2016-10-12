/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT64ADDRSUPPORT
#include <diabloelf.h>

/* Configuration {{{ */
#define SYMTABNAME ".symtab"
#define SYMSTRTABNAME ".strtab"
/* #define VERBOSE_WRITE */
#define MAX_PDR_DIFF 100
/* }}} */

static void ElfProcessSymbolTable64(t_object * obj, Elf64_Sym ** symbol_table_ret, char **strtab_ret, t_section *** sec_ret, t_uint32 * numsyms_ret, t_symbol *** lookup, t_section * sec);

/* sec_store_info: A struct used to describes how and where a section will be
 * stored in the file {{{ */
typedef struct
{
  t_section *sec;
  Elf64_Shdr hdr;
  t_int32 prepadding;
  t_bool hoist_headers;
  t_int32 described_by_pdr;
  t_int32 dynamic_pdr;
  t_int32 interp_pdr;
} sec_store_info;
/* }}} */
/* Common operations on objectfiles */

/* Utility function : __sort_secs {{{ */
static int
__sort_secs (const void *a, const void *b)
{
  sec_store_info *seca = (sec_store_info *) a, *secb = (sec_store_info *) b;

  if (seca->hdr.sh_addr > secb->hdr.sh_addr)
    return 1;
  else if (seca->hdr.sh_addr < secb->hdr.sh_addr)
    return -1;
  else if (seca->hdr.sh_addr == secb->hdr.sh_addr)
  {
    /* Assure that SHF_TLS sections come first. This is necessary, because
     * the .tbss section might have the same address as the section following
     * the .tbss section, yet we want to make sure it is the section
     * following the .tdata section */
    if (seca->hdr.sh_flags & SHF_TLS) return -1;
    else if (secb->hdr.sh_flags & SHF_TLS) return 1;
  }

  return 0;
}

/* }}} */

/*! This function writes a diablo object as an elf-object file
 *
 * \param fp The file pointer on which we will store the object file
 * \param hdr The file header (Ident, Type, Machine, Version and Entry must already be set)
 * \param obj The object to write
 * \param pagesize The pagesize of the target architecture
 *
 * \return void 
 *
 * Current layout:
 * 
 *    +--------------------------+
 *    |        ELF HEADER        |
 *    +--------------------------+
 *    |    PROGRAM HEADERS(s)    |
 *    +--------------------------+
 *    |   NULL SECTION HEADER    |
 *    |  NORMAL SECTION HEADERS  |
 *    | SECSTRTAB SECTION HEADER |
 *    +--------------------------+
 *    |   NORMAL SECTION DATA    |
 *    |   SECTION STRING TAB     |
 *    +--------------------------+
 */
/* ElfWriteCommonSameEndian {{{ */
#define SYMTAB
void
ElfWriteCommonSameEndian64 (FILE * fp, Elf64_Ehdr * hdr, t_object * obj,
			    t_bool hoist_headers, Elf64_HeaderInfo const * const hinfo)
{
#ifdef SYMTAB
  t_symbol * sym;
  Elf64_Shdr symtabshdr;
  Elf64_Shdr symstrtabshdr;
  t_string symstrtab;
#endif

  Elf64_Phdr *phdrs = NULL;
  Elf64_Shdr nullshdr, sectabshdr;
  t_segment *seg = OBJECT_SEGMENT_FIRST (obj);
  t_uint64 secstrtabsz = 1, runstrtabsz = 1;
  t_uint32 tel, nphdrs = 0, nsecs = 0;
  t_uint64 run = 0;
  t_uint64 can_be_placed_at;
  t_string secstrtab;
  t_bool sec_first = TRUE;
  sec_store_info *secs = NULL;

  /* Print section layout {{{ */
#ifdef VERBOSE_WRITE
  for (tel = 0; tel < OBJECT_NCODES (obj); tel++)
  {
    VERBOSE (0,
             ("Code section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_CODE (obj)[tel]),
              SECTION_CADDRESS (OBJECT_CODE (obj)[tel]),
              SECTION_CSIZE (OBJECT_CODE (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_CODE (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NRODATAS (obj); tel++)
  {
    VERBOSE (0,
             ("Rodata section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_RODATA (obj)[tel]),
              SECTION_CADDRESS (OBJECT_RODATA (obj)[tel]),
              SECTION_CSIZE (OBJECT_RODATA (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_RODATA (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NDATAS (obj); tel++)
  {
    VERBOSE (0,
             ("Data section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_DATA (obj)[tel]),
              SECTION_CADDRESS (OBJECT_DATA (obj)[tel]),
              SECTION_CSIZE (OBJECT_DATA (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_DATA (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NTLSDATAS (obj); tel++)
  {
    VERBOSE (0,
             ("TLS data section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_TLSDATA (obj)[tel]),
              SECTION_CADDRESS (OBJECT_TLSDATA (obj)[tel]),
              SECTION_CSIZE (OBJECT_TLSDATA (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_TLSDATA (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NNOTES (obj); tel++)
  {
    VERBOSE (0,
             ("Note section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_NOTE (obj)[tel]),
              SECTION_CADDRESS (OBJECT_NOTE (obj)[tel]),
              SECTION_CSIZE (OBJECT_NOTE (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_NOTE (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NBSSS (obj); tel++)
  {
    VERBOSE (0,
             ("Bss section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_BSS (obj)[tel]),
              SECTION_CADDRESS (OBJECT_BSS (obj)[tel]),
              SECTION_CSIZE (OBJECT_BSS (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_BSS (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NTLSBSSS (obj); tel++)
  {
    VERBOSE (0,
             ("TLS bss section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_TLSBSS (obj)[tel]),
              SECTION_CADDRESS (OBJECT_TLSBSS (obj)[tel]),
              SECTION_CSIZE (OBJECT_TLSBSS (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_TLSBSS (obj)[tel])));
  }

  for (tel = 0; tel < OBJECT_NDEBUGS (obj); tel++)
  {
    VERBOSE (0,
             ("Debug section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_DEBUG (obj)[tel]),
              SECTION_CADDRESS (OBJECT_DEBUG (obj)[tel]),
              SECTION_CSIZE (OBJECT_DEBUG (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_DEBUG (obj)[tel])));
  }
#endif

  /* End of testing }}} */
  /* Calculate the section - string table size "\0.section\0... and the number of sections" {{{ */

  for (tel = 0; tel < OBJECT_NCODES (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_CODE (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_CODE (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_CODE (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NRODATAS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_RODATA (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_RODATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NDATAS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_DATA (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_DATA (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_DATA (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NBSSS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_BSS (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_BSS (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_BSS (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NNOTES (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_NOTE (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_NOTE (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NDEBUGS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_DEBUG (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_DEBUG (obj)[tel])) + 1;
    nsecs++;
  }
  secstrtabsz += strlen (".shstrtab") + 1;

#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    secstrtabsz+=strlen(SYMTABNAME)+1;
    secstrtabsz+=strlen(SYMSTRTABNAME)+1;
  }
#endif


#ifdef VERBOSE_WRITE
  VERBOSE (0, ("Total string table size - .shstrtab :  %d", secstrtabsz));
  VERBOSE (0, ("Total number of sections :  %d", nsecs));
#endif

  /* End of Calculate the section - string table size }}} */
  /* Allocate space for string table and initialize it {{{ */
  secstrtab = Malloc (secstrtabsz);
  secstrtab[0] = '\0';
  /* }}} */
  /* Calculate section headers {{{ */
  nsecs = 0;

  /* the zero sectionheader */
  nullshdr.sh_name = 0;
  nullshdr.sh_type = SHT_NULL;
  nullshdr.sh_flags = 0;
  nullshdr.sh_addr = 0;
  nullshdr.sh_offset = 0;
  nullshdr.sh_size = 0;
  nullshdr.sh_link = SHN_UNDEF;
  nullshdr.sh_info = 0;
  nullshdr.sh_addralign = 0;
  nullshdr.sh_entsize = 0;

  /* The normal sectionheaders */
  for (tel = 0; tel < OBJECT_NCODES (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_CODE (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    DiabloBrokerCall("SetSectionFlags",&(secs[nsecs].hdr.sh_flags));
    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_CODE (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_CODE (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NRODATAS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC;
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;

    if (strcmp(".dynsym",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_DYNSYM;
      secs[nsecs].hdr.sh_entsize = 24;
    } 
    else if (strcmp(".dynstr",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_STRTAB;
    }
    else if (strcmp(".rela.dyn",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_RELA;
      secs[nsecs].hdr.sh_entsize = 0x18;
    }
    else if (strcmp(".rela.plt",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_RELA;
      secs[nsecs].hdr.sh_entsize = 0x18;
    }
    else if (strcmp(".hash",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      t_symbol * symptr;
      t_uint32 nbuckets = ElfComputeBucketCount (obj, sizeof(Elf64_Sym), 4096, 0);
      t_uint32 size = 4* (2 + nbuckets + SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)));

      t_uint32 * data = SECTION_DATA(OBJECT_RODATA (obj)[tel]);


      ASSERT(AddressIsGe(SECTION_CSIZE(OBJECT_RODATA (obj)[tel]), AddressNew64(size)), ("Hash table under dimensioned (need size @G have @G)", size, SECTION_CSIZE(OBJECT_RODATA (obj)[tel])));

      data[0]=nbuckets;
      data[1]=SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj))+1;

      for (symptr=SYMBOL_TABLE_FIRST(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
      {
        t_string dynsymsecname = StringConcat2("DYNSYM:", SYMBOL_NAME(symptr));
        t_section * psec = SectionGetFromObjectByName(obj, ".dynsym");
        t_object * lo = ObjectGetLinkerSubObject(obj);

        t_section * sec = SectionGetFromObjectByName(lo, dynsymsecname);

	Free(dynsymsecname);

        if (sec)
        {
          t_uint32 mybucket = ElfHash(SYMBOL_NAME(symptr)) % nbuckets;
          t_uint32 idx = AddressExtractUint32(AddressSub(SECTION_CADDRESS(sec), SECTION_CADDRESS(psec)))/sizeof(Elf64_Sym);
          VERBOSE(1, ("Doing %s putting it in bucket %d (idx = %d)", SYMBOL_NAME(symptr),  mybucket, idx));
          data [2 + nbuckets + idx] = data [2 + mybucket ];
          data [2 + mybucket ] = idx;
        }
        else
          VERBOSE(0, ("NOT Doing %s", SYMBOL_NAME(symptr)));

      }


      secs[nsecs].hdr.sh_type = SHT_HASH;

      secs[nsecs].hdr.sh_entsize = 0x4;
    }
    else if (strcmp(".plt",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
      secs[nsecs].hdr.sh_type = hinfo->pltshtype;
      secs[nsecs].hdr.sh_entsize = hinfo->pltentsize;
    }

    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_RODATA (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_RODATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NDATAS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_DATA (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE;
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;

    if (strcmp(".got",SECTION_NAME (OBJECT_DATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_entsize = 0x8;
    }
    if (strcmp(".dynamic",SECTION_NAME (OBJECT_DATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_DYNAMIC;
      secs[nsecs].hdr.sh_entsize = 0x10;
    }

    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_DATA (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_DATA (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_DATA (obj)[tel]));
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_DATA (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_DATA (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NBSSS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_BSS (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_NOBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_BSS (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_BSS (obj)[tel])) + 1;

    if (strcmp(".plt",SECTION_NAME (OBJECT_BSS (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = hinfo->pltshtype;
      secs[nsecs].hdr.sh_entsize = hinfo->pltentsize;
    }

    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NNOTES (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_NOTE;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_NOTE (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_NOTE (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NDEBUGS (obj); tel++)
  {
    if (!G_T_UINT64 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = 0;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT64 (SECTION_CADDRESS (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT64 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT64 (SECTION_ALIGNMENT (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_DEBUG (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_DEBUG (obj)[tel])) + 1;
    nsecs++;
  }

  /* sort the sections */
  diablo_stable_sort (secs, nsecs, sizeof (sec_store_info), __sort_secs);

  {
    t_int32 interpidx = -1;
    t_int32 dynamicidx = -1;
    t_int32 dynsymidx = -1;
    t_int32 dynstridx = -1;
    t_int32 reldynidx = -1;
    t_int32 relpltidx = -1;
    t_int32 pltidx = -1;
    t_int32 hashidx = -1;
    for (tel = 0; tel < nsecs; tel++)
    {
      if (strcmp(".dynsym",SECTION_NAME (secs[tel].sec)) == 0)
      {
        dynsymidx = tel;
      }
      else if (strcmp(".hash",SECTION_NAME (secs[tel].sec)) == 0)
      {
        hashidx = tel;
      }
      else if (strcmp(".dynamic",SECTION_NAME (secs[tel].sec)) == 0)
      {
        dynamicidx = tel;
      }
      else if (strcmp(".dynstr",SECTION_NAME (secs[tel].sec)) == 0)
      {
        dynstridx = tel;
      }
      else if ((strcmp(".rel.dyn",SECTION_NAME (secs[tel].sec)) == 0)
               || (strcmp(".rela.dyn",SECTION_NAME (secs[tel].sec)) == 0))
      {
        reldynidx = tel;
      }
      else if ((strcmp(".rel.plt",SECTION_NAME (secs[tel].sec)) == 0)
      || (strcmp(".rela.plt",SECTION_NAME (secs[tel].sec)) == 0))
      {
        relpltidx = tel;
      }
      else if (strcmp(".plt",SECTION_NAME (secs[tel].sec)) == 0)
      {
        pltidx = tel;
      }
      else if (strcmp(".interp",SECTION_NAME (secs[tel].sec)) == 0)
      {
        interpidx = tel;
      }
    }

    if ((dynsymidx != -1) && (hashidx != -1))
      secs[hashidx].hdr.sh_link = dynsymidx + 1;

    if ((dynsymidx != -1) && (interpidx != -1))
      secs[dynsymidx].hdr.sh_info = interpidx + 1;

    if ((dynsymidx != -1) && (dynstridx != -1))
      secs[dynsymidx].hdr.sh_link = dynstridx + 1;
    
    if ((dynsymidx != -1) && (reldynidx != -1))
      secs[reldynidx].hdr.sh_link = dynsymidx + 1;

    if ((dynsymidx != -1) && (relpltidx != -1))
      secs[relpltidx].hdr.sh_link = dynsymidx + 1;

    if ((relpltidx != -1) && (pltidx != -1))
      secs[relpltidx].hdr.sh_info = pltidx + 1;
    
    if ((dynamicidx != -1) && (dynstridx != -1))
      secs[dynamicidx].hdr.sh_link = dynstridx + 1;
  }
  /* Write a symbol table */
#ifdef SYMTAB
  /* The symbol table */
  if (diabloobject_options.symbols)
  {
    symtabshdr.sh_name=runstrtabsz;
    symtabshdr.sh_type=SHT_SYMTAB;
    symtabshdr.sh_flags=0;
    sprintf(secstrtab+runstrtabsz,"%s",SYMTABNAME);
    runstrtabsz+=strlen(SYMTABNAME)+1;
    symtabshdr.sh_addr=0;
    symtabshdr.sh_size=sizeof(Elf64_Sym);

    if (OBJECT_SYMBOL_TABLE(obj))
    for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
    {
      symtabshdr.sh_size += sizeof(Elf64_Sym);
    }

    symtabshdr.sh_offset=0;
    symtabshdr.sh_link=nsecs+3;
    symtabshdr.sh_info=symtabshdr.sh_size/sizeof(Elf64_Sym);
    symtabshdr.sh_addralign=4;
    symtabshdr.sh_entsize=sizeof(Elf64_Sym);

    /* The strtable */
    symstrtabshdr.sh_name=runstrtabsz;
    symstrtabshdr.sh_type=SHT_STRTAB;
    symstrtabshdr.sh_flags=0;
    sprintf(secstrtab+runstrtabsz,"%s",SYMSTRTABNAME);
    runstrtabsz+=strlen(SYMTABNAME)+1;
    symstrtabshdr.sh_addr=0;
    symstrtabshdr.sh_size=1;
    if (OBJECT_SYMBOL_TABLE(obj))
      for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
      {
        symstrtabshdr.sh_size += strlen(SYMBOL_NAME(sym)) + 1;
      }
    symstrtabshdr.sh_offset=0;
    symstrtabshdr.sh_link=0;
    symstrtabshdr.sh_info=0;
    symstrtabshdr.sh_addralign=4;
    symstrtabshdr.sh_entsize=0;
  }
#endif

  /* data_string table */
  sectabshdr.sh_name = runstrtabsz;
  sectabshdr.sh_type = SHT_STRTAB;
  sectabshdr.sh_addr = 0;
  sectabshdr.sh_size = runstrtabsz + strlen (".shstrtab") + 1;
  sectabshdr.sh_flags = 0;
  sectabshdr.sh_offset = 0;
  sectabshdr.sh_link = 0;
  sectabshdr.sh_info = 0;
  sectabshdr.sh_addralign = 0;
  sectabshdr.sh_entsize = 0;
  sprintf (secstrtab + runstrtabsz, "%s", ".shstrtab");
  runstrtabsz += strlen (".shstrtab") + 1;
  /* }}} */

  ASSERT (runstrtabsz == secstrtabsz,
          ("Something went wrong while building the string table!"));

  /* Calculate the number of phdrs needed. The phdrs describes which segments
   * the OS/debugger/whatever needs to make to run the program. So it describes
   * a collection of sections  
   * {{{ */
  for (tel = 0; tel < nsecs; tel++)
  {
    /* PT_LOAD: all SHF_ALLOC sections - the .tbss sections need a PT_LOAD
     * segment. .tdata resides in two segments (PT_LOAD and PT_TLS) */
    if ((secs[tel].hdr.sh_flags & SHF_ALLOC) && (!(((secs[tel].hdr.sh_type == SHT_NOBITS) && (secs[tel].hdr.sh_flags & SHF_TLS)))))
    {
      /* Alloc new segment when necessary */
      if ((secs[tel].hdr.sh_addr > run + MAX_PDR_DIFF) || (sec_first)
          || (seg && (G_T_UINT64 (SEGMENT_CADDRESS (seg)) <= run)))
      {
#ifdef VERBOSE_WRITE
        VERBOSE(0,
                ("Allocing new program header (%d) for %s, diff between previous programheader = %lld bytes\n",
                 nphdrs, secstrtab + secs[tel].hdr.sh_name,
                 secs[tel].hdr.sh_addr - run));
#endif
        phdrs = Realloc (phdrs, sizeof (Elf64_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_LOAD;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
        phdrs[nphdrs].p_memsz = 0;
        phdrs[nphdrs].p_align = hinfo->ptload_align;
        phdrs[nphdrs].p_flags = PF_R;
        phdrs[nphdrs].p_filesz = 0;
        nphdrs++;
        secs[tel].prepadding = -1;
        secs[tel].hoist_headers = FALSE;

        if (seg)
          printf ("Current segment=%s\n", SEGMENT_NAME (seg));

        if (sec_first)
          run = secs[tel].hdr.sh_addr;
        if ((sec_first) && (!seg))
          secs[tel].hoist_headers = TRUE;

        if (seg && (G_T_UINT64 (SEGMENT_CADDRESS (seg)) <= run))
        {
          if (SEGMENT_HOIST_HEADERS (seg))
          {
            /*if (!sec_first)
              FATAL (("Segment hoists the headers, but is not first in file")); */
            secs[tel].hoist_headers = TRUE;
          }
          printf ("Seg %s started\n", SEGMENT_NAME (seg));
          seg = SEGMENT_NEXT (seg);
          if (seg)
            printf ("New seg is %s\n", SEGMENT_NAME (seg));
        }
        sec_first = FALSE;

      }
      else if (secs[tel].hdr.sh_addr < run)
      {
        FATAL (("Sections overlap (or are not sorted as expected) for section %s at @G(%llx) size @G!(run = 0x%llx)", SECTION_NAME (secs[tel].sec), SECTION_CADDRESS (secs[tel].sec), secs[tel].hdr.sh_addr, SECTION_CSIZE (secs[tel].sec), run));
      }
      else
      {
        secs[tel].prepadding = secs[tel].hdr.sh_addr - run;
        secs[tel].hoist_headers = FALSE;
        phdrs[nphdrs - 1].p_memsz += secs[tel].prepadding;
        phdrs[nphdrs - 1].p_filesz +=
          (secs[tel].hdr.sh_type != SHT_NOBITS) ? secs[tel].
          prepadding : 0;
#ifdef VERBOSE_WRITE
        VERBOSE(0, 
                ("Hoisting %s in previous program header, diff with previous section = %d bytes",
                 secstrtab + secs[tel].hdr.sh_name,
                 secs[tel].hdr.sh_addr - run));
#endif
      }
      phdrs[nphdrs - 1].p_filesz +=
        (secs[tel].hdr.sh_type != SHT_NOBITS) ? secs[tel].hdr.
        sh_size : 0;
      phdrs[nphdrs - 1].p_memsz += secs[tel].hdr.sh_size;

      secs[tel].described_by_pdr = nphdrs - 1;
      if (secs[tel].hdr.sh_flags & SHF_WRITE)
        phdrs[nphdrs - 1].p_flags |= PF_W;
      if (secs[tel].hdr.sh_flags & SHF_EXECINSTR)
        phdrs[nphdrs - 1].p_flags |= PF_X;
      run = secs[tel].hdr.sh_addr + secs[tel].hdr.sh_size;
    }
    else
    {
      secs[tel].described_by_pdr = -1;
#ifdef VERBOSE_WRITE
      VERBOSE(0, ("Skipping section: %s (not in program header)",
                  secstrtab + secs[tel].hdr.sh_name));
#endif
    }
  }

  for (tel = 0; tel < nsecs; tel++)
  {
    if (strcmp(secstrtab + secs[tel].hdr.sh_name, ".dynamic") == 0)
    {
        phdrs = Realloc (phdrs, sizeof (Elf64_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_DYNAMIC;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
        secs[tel].dynamic_pdr = nphdrs;
        phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
        phdrs[nphdrs].p_align = hinfo->ptdynamic_align;
        phdrs[nphdrs].p_flags = PF_R | PF_W;
        phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
        nphdrs++;
    }
    else if (strcmp(secstrtab + secs[tel].hdr.sh_name, ".interp") == 0)
    {
        phdrs = Realloc (phdrs, sizeof (Elf64_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_INTERP;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
        secs[tel].interp_pdr = nphdrs;
        phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
        phdrs[nphdrs].p_align = hinfo->ptinterp_align;
        phdrs[nphdrs].p_flags = PF_R;
        phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
        nphdrs++;
    }
  }
  /* Allocate a PT_TLS segment for the tls sections */
  {
    t_bool first_tls=FALSE;
    for (tel = 0; tel < nsecs; tel++)
    {
      if (secs[tel].hdr.sh_flags & SHF_TLS)
      {
        if (!first_tls)
        {
          phdrs = Realloc (phdrs, sizeof (Elf64_Phdr) * (nphdrs + 1));
          phdrs[nphdrs].p_type = PT_TLS;
          phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

          phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
          secs[tel].dynamic_pdr = nphdrs;
          phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
          phdrs[nphdrs].p_align = hinfo->pttls_align;
          phdrs[nphdrs].p_flags = PF_R;
          phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
          nphdrs++;
        }
        else
        {
          phdrs[nphdrs-1].p_memsz += secs[tel].hdr.sh_size;
          if (secs[tel].hdr.sh_type != SHT_NOBITS)
          phdrs[nphdrs-1].p_filesz += secs[tel].hdr.sh_size;
        }
        first_tls = TRUE;
      }
    }
  }



  /* }}} */
  /* Now place all segments in the file: segments file offsets need to be
   * congruent with the virtual address modulo the pagesize  
   * {{{ */
  can_be_placed_at = sizeof (Elf64_Ehdr) + nphdrs * sizeof (Elf64_Phdr);

  for (tel = 0; tel < nsecs; tel++)
  {
    /* PT_LOAD: all SHF_ALLOC sections - the .tbss sections need a PT_LOAD
     * segment */
    if ((secs[tel].hdr.sh_flags & SHF_ALLOC) && (!(((secs[tel].hdr.sh_type == SHT_NOBITS) && (secs[tel].hdr.sh_flags & SHF_TLS)))))
    {
      if (secs[tel].prepadding == -1)
      {
        if ((hinfo->pagesize != 0)
            && ((can_be_placed_at & (hinfo->pagesize - 1)) <
                (secs[tel].hdr.sh_addr & (hinfo->pagesize - 1))))
        {
          secs[tel].prepadding =
            (secs[tel].hdr.sh_addr & (hinfo->pagesize - 1)) -
            (can_be_placed_at & (hinfo->pagesize - 1));
#ifdef VERBOSE_WRITE
          VERBOSE(0, ("Segment prepadding =%d", secs[tel].prepadding));
#endif
        }
        else if ((hinfo->pagesize != 0)
                 && ((can_be_placed_at & (hinfo->pagesize - 1)) >
                     (secs[tel].hdr.sh_addr & (hinfo->pagesize - 1))))
        {
          secs[tel].prepadding =
            hinfo->pagesize + (secs[tel].hdr.sh_addr & (hinfo->pagesize - 1)) -
            (can_be_placed_at & (hinfo->pagesize - 1));
#ifdef VERBOSE_WRITE
          printf ("Segment prepadding =%d\n", secs[tel].prepadding);
#endif
        }
        else
        {
          secs[tel].prepadding = 0;
        }
        can_be_placed_at += secs[tel].prepadding;
        phdrs[secs[tel].described_by_pdr].p_offset = can_be_placed_at;
        secs[tel].hdr.sh_offset = can_be_placed_at;
        if ((hoist_headers) && (secs[tel].hoist_headers))
        {
          phdrs[secs[tel].described_by_pdr].p_offset = 0;
          phdrs[secs[tel].described_by_pdr].p_memsz +=
            can_be_placed_at;
          phdrs[secs[tel].described_by_pdr].p_filesz +=
            can_be_placed_at;
          phdrs[secs[tel].described_by_pdr].p_vaddr -=
            can_be_placed_at;
          phdrs[secs[tel].described_by_pdr].p_paddr -=
            can_be_placed_at;
        }
        if (secs[tel].hdr.sh_type != SHT_NOBITS)
          can_be_placed_at += secs[tel].hdr.sh_size;
      }
      else
      {
        can_be_placed_at += secs[tel].prepadding;
        secs[tel].hdr.sh_offset = can_be_placed_at;
        if (secs[tel].hdr.sh_type != SHT_NOBITS)
          can_be_placed_at += secs[tel].hdr.sh_size;
      }
    }
    /* The .tbss sections */
    else if (secs[tel].hdr.sh_flags & SHF_ALLOC)
    {
        secs[tel].hdr.sh_offset = can_be_placed_at;
        secs[tel].prepadding = 0;
    }
    if (secs[tel].dynamic_pdr != -1)
    {
      phdrs[secs[tel].dynamic_pdr].p_offset = can_be_placed_at - secs[tel].hdr.sh_size;
    }
    if (secs[tel].interp_pdr != -1)
    {
      phdrs[secs[tel].interp_pdr].p_offset = can_be_placed_at - secs[tel].hdr.sh_size;
    }
  }
  /* }}} */

  hdr->e_shoff = can_be_placed_at;

  can_be_placed_at = can_be_placed_at + (nsecs + 2) * sizeof (Elf64_Shdr);

#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    can_be_placed_at += sizeof (Elf64_Shdr) * 2;
  }
#endif

  sectabshdr.sh_offset = can_be_placed_at;

  /* Set ELF file header {{{ */
  /* Ident, Machine, Version and Entry are already set */
  hdr->e_phoff = sizeof (Elf64_Ehdr);
  hdr->e_ehsize = sizeof(Elf64_Ehdr);
  switch (OBJECT_TYPE(obj))
  {
    case OBJTYP_EXECUTABLE:
    case OBJTYP_EXECUTABLE_PIC:
      hdr->e_type = ET_EXEC;
      break;
    case OBJTYP_SHARED_LIBRARY:
    case OBJTYP_SHARED_LIBRARY_PIC:
      hdr->e_type = ET_DYN;
      break;
    default:
      FATAL(("Final binary should be an executable or a dynamic library, but is %d",OBJECT_TYPE(obj)));
      break;
  }
  /* Flags : TODO */
  /* Ehsize is already set */
  hdr->e_phentsize = sizeof (Elf64_Phdr);
  hdr->e_phnum = nphdrs;
  hdr->e_shentsize = sizeof (Elf64_Shdr);
  /* #entries in section header table =
   * #normal sections + 1 zero-section + 1 section name string table */
  hdr->e_shnum = nsecs + 2;
  hdr->e_shstrndx = nsecs + 1;
#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    hdr->e_shnum += 2;
    can_be_placed_at += sectabshdr.sh_size;
    symtabshdr.sh_offset = can_be_placed_at;

    if (symtabshdr.sh_offset & 3) 
      symtabshdr.sh_offset = ((symtabshdr.sh_offset + 4) & (~3));

    can_be_placed_at = symtabshdr.sh_offset + symtabshdr.sh_size;

    symstrtabshdr.sh_offset = can_be_placed_at;
  }
#endif
  /* }}} */
  /* Write ELF header + Program headers {{{ */
  fwrite (hdr, 1, sizeof (Elf64_Ehdr), fp);
  fwrite (phdrs, sizeof (Elf64_Phdr), nphdrs, fp);
  /* }}} */
  /* Write section data {{{ */
  for (tel = 0; tel < nsecs; tel++)
  {
    if (secs[tel].hdr.sh_flags & SHF_ALLOC)
    {
      static char padd[1 << 14];	/*todo..aanpassen aan pagesize=16kB [OK] */
      fwrite (padd, sizeof (char), secs[tel].prepadding, fp);
      ASSERT (secs[tel].hdr.sh_offset == ftell (fp),
              ("Section %d not at the right place (should be %x, is %x)",
               tel, secs[tel].hdr.sh_offset, ftell (fp)));
      if (secs[tel].hdr.sh_type != SHT_NOBITS)
        fwrite (SECTION_DATA (secs[tel].sec), sizeof (char),
                G_T_UINT64 (SECTION_CSIZE (secs[tel].sec)), fp);
    }
  }
  /* }}} */
  /* Write section headers {{{ */

  /* Verify correctness: section table should be at hdr->e_shoff */
  ASSERT (ftell (fp) == hdr->e_shoff,
          ("Section header table is not at %x as said in the program header, but at %x",
           hdr->e_shoff, ftell (fp)));
  
  /* 1 - zero header */
  fwrite (&nullshdr, sizeof (Elf64_Shdr), 1, fp);
  /* 2 - normal section headers */
  for (tel = 0; tel < nsecs; tel++)
  {
    fwrite (&secs[tel].hdr, sizeof (Elf64_Shdr), 1, fp);
  }
  /* 3 - section name string table header */
  fwrite (&sectabshdr, sizeof (Elf64_Shdr), 1, fp);

#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    fwrite (&symtabshdr, sizeof (Elf64_Shdr), 1, fp);

    fwrite (&symstrtabshdr, sizeof (Elf64_Shdr), 1, fp);
  }
#endif
  /* }}} */
  /* Write section string table {{{ */
  ASSERT (sectabshdr.sh_offset == ftell (fp),
          ("String table not at the right place: should be %x but is %x",
           sectabshdr.sh_offset, ftell (fp)));
#ifdef VERBOSE_WRITE
  VERBOSE(0, ("Writing secstrtab at 0x%x", (t_int32) ftell (fp)));
#endif
  fwrite (secstrtab, sizeof (char), secstrtabsz, fp);

  /* }}} */
#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    int tel=0;
    int runsymstrtab = 1;
    Elf64_Sym e_sym;
    e_sym.st_name=0;
    e_sym.st_value=0;
    e_sym.st_size=0;
    e_sym.st_info=0;
    e_sym.st_other=0;
    e_sym.st_shndx=0;
    symstrtab = Malloc(symstrtabshdr.sh_size);


    while(symtabshdr.sh_offset>ftell (fp)) 
    {
      char c='\0';
      fwrite(&c,1,1,fp);
    }

    ASSERT (ftell (fp) == symtabshdr.sh_offset,
            ("Symbol table not at %x as said in the symtab section header, but at %x",
             symtabshdr.sh_offset, ftell (fp)));

    symstrtab [0] = '\0';

    fwrite(&e_sym,sizeof(Elf64_Sym),1,fp);

    if (OBJECT_SYMBOL_TABLE(obj))
      for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
      {
        symtabshdr.sh_size+=sizeof(Elf64_Sym);
        e_sym.st_name = runsymstrtab;
        sprintf(symstrtab + runsymstrtab, "%s", SYMBOL_NAME(sym));
        runsymstrtab += strlen(SYMBOL_NAME(sym)) + 1;
        if (SYMBOL_ORDER(sym)<0)
          e_sym.st_info=ELF64_ST_INFO(STB_LOCAL, 0);
        else
          e_sym.st_info=ELF64_ST_INFO(STB_GLOBAL, 0);

	if (StringPatternMatch(".L.*", SYMBOL_NAME(sym)))    e_sym.st_info=ELF64_ST_INFO(STB_GLOBAL, 0);


        if (SYMBOL_BASE(sym) == T_RELOCATABLE(OBJECT_ABS_SECTION(obj)))
        {
          e_sym.st_shndx=SHN_ABS;
        }
        else
        {
          e_sym.st_shndx=SHN_ABS;
          for (tel=0; tel<nsecs; tel++)
          {
            if (SYMBOL_BASE(sym) == T_RELOCATABLE(secs[tel].sec))
              e_sym.st_shndx = tel + 1;
          }
        }
        e_sym.st_value=G_T_UINT64(StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj));
        fwrite(&e_sym,sizeof(Elf64_Sym),1,fp);
        tel++;
      }

    fwrite(symstrtab,symstrtabshdr.sh_size,sizeof(char),fp);
    Free(symstrtab);
  }
#endif

  /* Free allocated structures {{{ */
  if (nphdrs)
    Free (phdrs);
  Free (secs);
  Free (secstrtab);
  /* }}} */
  fclose (fp);
}

/* FUN : ElfReadCommon64
 * PAR : A file pointer pointing to the data of an objectfile after the header, the header, an obsolete/unused parameter, 
 *       the t_object structure to fill, A pointer to return the section headers in, 
 *       A pointer to return the symbol table in, A pointer to return the number of syms in, 
 *       A pointer to return the stringtable in, A pointer to return the section header stringtable in
 *       t_address (= the value of the symbol), type_uint8 (TODO= the type of the symbol)
 * RET : nothing 
 * DESC: This function reads the common information in a 64 bit ELF file and adapts the corresponding 
 *       t_object structure. Structures that need further parsing are returned. 
 *       TODO: The symboltable change probably made most returns obsolete ... */

void
ElfReadCommon64 (FILE * fp, void *hdr, t_object * obj, Elf64_Shdr ** shdr_ret,
                 Elf64_Sym ** symbol_table_ret, Elf64_Sym ** dynamic_symbol_table_ret, t_uint32 * numsyms_ret,
                 char **strtab_ret, char **sechdrstrtab_ret,
                 t_section *** sec_ret, t_symbol *** lookup, t_symbol *** dynamic_lookup,
                 t_bool switch_endian, t_bool read_debug, Elf64_HeaderInfo const * const hinfo)
{
  t_bool dynlib;

  char *dynstrtab = NULL;	/* Will hold the dynamic string table */

  Elf64_Dyn *dynamic_table = NULL;	/* Will hold the dynamic table */

  t_section * dynamic_symbol_table_section = NULL;

  t_uint32 numdynsym = 0;

  t_address tmp_addr, tmp_sz, tmp_align;	/* Temporaries used in conversion from uint32 -> generic_address */

  Elf64_Ehdr *hdr64 = (Elf64_Ehdr *) hdr;	/* The Objectfileheader (casted correctly, it's passed as a union) */
  t_uint32 section_tel = 0;	/* Counter for the sections */
  t_uint32 dynamic_tel = 0;	/* Counter for the dynamic table entries */

  if (!obj)
    FATAL (("Object = NULL"));
  OBJECT_SET_ADDRESS_SIZE (obj, ADDRSIZE64);

  dynlib = hdr64->e_type == ET_DYN;

  if (!OBJECT_PARENT(obj))
  {
    /* create global dummy sections (abs and undef) */
    OBJECT_SET_UNDEF_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*UNDEF*"));
    OBJECT_SET_ABS_SECTION(obj, SectionCreateForObject (obj, SPECIAL_SECTION, NULL, AddressNullForObject (obj), "*ABS*"));
    if (dynlib)
    {
      VERBOSE(3,("Parsing dynamic library"));
      OBJECT_SET_TYPE(obj,OBJTYP_SHARED_LIBRARY_PIC);
    }
    else
    {
      VERBOSE(3,("Parsing executable"));
      OBJECT_SET_TYPE(obj,OBJTYP_EXECUTABLE);
    }
  }
  else
  {
    OBJECT_SET_TYPE(obj,OBJTYP_RELOCATABLE);
  }
  
  /* Let the object know that it has to be careful if the endianness is switched */
  OBJECT_SET_SWITCHED_ENDIAN (obj, switch_endian);
  /* Check if the use of this function is correct */
  ASSERT (shdr_ret != NULL, ("Must supply a shdr_return"));
  ASSERT (symbol_table_ret != NULL, ("Must supply a symbol_table_return"));
  ASSERT (numsyms_ret != NULL, ("Must supply a numsyms_return"));
  ASSERT (strtab_ret != NULL, ("Must supply a strtab_return"));
  ASSERT (sechdrstrtab_ret != NULL, ("Must supply a strtab_return"));
  ASSERT (lookup != NULL, ("Must supply a symbol lookup table return"));
  ASSERT (sec_ret != NULL, ("Must supply a section table return"));

  /* We need to know the endianness to check this */
  ASSERT (hdr64->e_version == EV_CURRENT,
          ("Elf version differs from EV_CURRENT.\nThis could mean that a new ELF specification is created.\nMore likely this means your file is corrupt/truncated or not an ELF objectfile.!"));

  /* Store the entry point in the object structure  */
  OBJECT_SET_ENTRY (obj, AddressNew64 (hdr64->e_entry));
    
  OBJECT_SET_SYMBOL_TABLE (obj, SymbolTableNew (obj));

  if (hdr64->e_shnum)		/* If there are sections */
  {
    /* STEP 1: Read the headers */
    /* Allocated place to store the headers */
    (*shdr_ret) =
      (Elf64_Shdr *) Malloc (sizeof (Elf64_Shdr) * hdr64->e_shnum);
    (*sec_ret) =
      (t_section **) Calloc (sizeof (t_section *), hdr64->e_shnum);
    /* Look for the headers in the file 
     * streampos is used for when we are directly reading from an archive */
    fseek (fp, OBJECT_STREAMPOS (obj) + hdr64->e_shoff, SEEK_SET);
    /* Read the headers */
    fread ((*shdr_ret), sizeof (Elf64_Shdr), hdr64->e_shnum, fp);
    /* If we are doing a switched endian read, endian-swap all headers */

    if (switch_endian)
    {
      t_uint32 tel = 0;

      for (tel = 0; tel < hdr64->e_shnum; tel++)
        Elf64ShdrSwitchEndian (&(*shdr_ret)[tel]);
    }

    /* STEP 2: Find the sectionheader string table */
    (*sechdrstrtab_ret) =
      (char *) Malloc ((*shdr_ret)[hdr64->e_shstrndx].sh_size);
    fseek (fp,
           OBJECT_STREAMPOS (obj) +
           (*shdr_ret)[hdr64->e_shstrndx].sh_offset, SEEK_SET);
    fread ((*sechdrstrtab_ret), (*shdr_ret)[hdr64->e_shstrndx].sh_size, 1,
           fp);

    /* STEP 3: find the symboltable and the (symbol) stringtable and the dynamic 
     * stringtable (we handle them before the others, because we'll need them later on)*/
    while (section_tel < hdr64->e_shnum)
    {
      if ((*shdr_ret)[section_tel].sh_type == SHT_SYMTAB)
      {
        /* A normal symbol table.... current ABI says we should only have one symbol table */
        ASSERT (!(*symbol_table_ret),
                ("More than one symbol_table in %s",
                 OBJECT_NAME (obj)));
        ASSERT (!
                ((*shdr_ret)[section_tel].sh_size % sizeof (Elf64_Sym)),
                ("Symtab size is not a multiple of %d in %s",
                 sizeof (Elf64_Sym), OBJECT_NAME (obj)));
        (*numsyms_ret) =
          (*shdr_ret)[section_tel].sh_size / sizeof (Elf64_Sym);
        (*symbol_table_ret) = Malloc ((*shdr_ret)[section_tel].sh_size);
        fseek (fp,
               OBJECT_STREAMPOS (obj) +
               (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
        fread ((*symbol_table_ret), (*shdr_ret)[section_tel].sh_size, 1, fp);

        if (switch_endian)
        {
          FATAL (("Switched endian not implemented!"));
        }
      }
      else if ((*shdr_ret)[section_tel].sh_type==SHT_DYNSYM)
      {
        ASSERT(dynamic_symbol_table_ret, ("Found a dynamic symbol table, but dynamic symbol tables are not yet implemented for this architecture!"));
        /* A dynamic symbol table.... current ABI says we should only have
         * one symbol table */
        ASSERT (!(*dynamic_symbol_table_ret),
                ("More than one dynamic symbol_table in %s",
                 OBJECT_NAME (obj)));
        ASSERT (!((*shdr_ret)[section_tel].sh_size % sizeof(Elf64_Sym)),
                ("Symtab size is not a multiple of %d in %s",
                 sizeof(Elf64_Sym), OBJECT_NAME (obj)));

        numdynsym=(*shdr_ret)[section_tel].sh_size/sizeof(Elf64_Sym);
        (*dynamic_symbol_table_ret)=Malloc((*shdr_ret)[section_tel].sh_size);
        fseek (fp, OBJECT_STREAMPOS (obj) + (*shdr_ret)[section_tel].sh_offset, SEEK_SET);

        fread((*dynamic_symbol_table_ret),(*shdr_ret)[section_tel].sh_size,1,fp);

        if (switch_endian)
        {
          FATAL (("Switched endian not implemented!"));
        }
      }
      else if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".strtab") == 0)
      {
        /*  A normal string table... current ABI says we should only have one (normal) string table */
        ASSERT (!(*strtab_ret),
                ("More than one stringtable in %s", OBJECT_NAME (obj)));
        (*strtab_ret) = Malloc ((*shdr_ret)[section_tel].sh_size);
        fseek (fp,
               OBJECT_STREAMPOS (obj) +
               (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
        fread ((*strtab_ret), (*shdr_ret)[section_tel].sh_size, 1, fp);
      }
      else if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".dynstr") == 0)
      {
        /*  A dynamic string table... current ABI says we should only have one (dynamic) string table */
        ASSERT (!(dynstrtab),
                ("More than one dynamic stringtable in %s",
                 OBJECT_NAME (obj)));
        (dynstrtab) = Malloc ((*shdr_ret)[section_tel].sh_size);
        fseek (fp,
               OBJECT_STREAMPOS (obj) +
               (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
        fread ((dynstrtab), (*shdr_ret)[section_tel].sh_size, 1, fp);
      }
      section_tel++;
    }

    /* STEP 4: find the loaded sections */
    section_tel = 0;

    while (section_tel < hdr64->e_shnum)
    {
      VERBOSE (1,
               (" A SECTION named %s (size=%lld - type =%d)",
                (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,
                (*shdr_ret)[section_tel].sh_size, (*shdr_ret)[section_tel].sh_type));

      switch ((*shdr_ret)[section_tel].sh_type)
      {
        case SHT_NULL:
          /* The null section: an inactive section, probably only the first section i
           * in the binary. It has no associated data */
          if (section_tel != 0)
            FATAL (("NULL SECTION as %d-th section while reading %s!",
                    section_tel, OBJECT_NAME (obj)));
          break;
        case SHT_INIT_ARRAY:
        case SHT_FINI_ARRAY:
        case SHT_PREINIT_ARRAY:
        case SHT_PROGBITS:

          /* This section holds information defined by the program, whose format
           * and meaning are determined solely by the program. */
          VERBOSE (2,
                   ("Found program specific information. Looking at the flags to determine it's data"));
          if (((*shdr_ret)[section_tel].sh_flags & SHF_ALLOC))	/* This section will be allocated at run-time */
          {
            if (((*shdr_ret)[section_tel].sh_flags & SHF_EXECINSTR) 
                && StringCmp(((*sechdrstrtab_ret) +(*shdr_ret)[section_tel].sh_name),".got")    /* Do not disassemble got and plt */
                && StringCmp(((*sechdrstrtab_ret) +(*shdr_ret)[section_tel].sh_name),".plt"))	/* Do not disassemble got and plt */
            {
              if (((*shdr_ret)[section_tel].sh_flags & SHF_WRITE))	/* It's a writable section */
              {
                WARNING (("Writable code section %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                VERBOSE (2,
                         ("   It's an executable (code) section named %s (size=%d)",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name,
                          (*shdr_ret)[section_tel].sh_size));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].
                                sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, CODE_SECTION, TRUE,
                                            fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);

              }
              else	/* Normal code section */
              {
                VERBOSE (2, ("   It's an executable (code) section named %s (size=%d)", (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, (*shdr_ret)[section_tel].sh_size));

                tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align = AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, CODE_SECTION, TRUE,
                                            fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);
              }
            }
            else		/* Non-code sections */
            {
              if (((*shdr_ret)[section_tel].sh_flags & SHF_TLS))
              {
                ASSERT(((*shdr_ret)[section_tel].sh_flags & SHF_WRITE), ("Found a read only tls section. This has no use, and is thus not implemented."));
                VERBOSE (2, ("   It's a thread local storage data section named %s",
                             (*sechdrstrtab_ret) +
                             (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].
                                sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, TLSDATA_SECTION,
                                            FALSE, fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);

              }
              else if (((*shdr_ret)[section_tel].sh_flags & SHF_WRITE))	/* Writable section */
              {
                VERBOSE (1,
                         ("   It's a writable data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].
                                sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, DATA_SECTION,
                                            FALSE, fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);
              }
              else	/* Constant section */
              {
                VERBOSE (2,
                         ("   It's a constant data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].
                                sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
                                            fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);
              }
            }

            if (StringPatternMatch (".gnu.linkonce*", (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
            {
              SECTION_SET_IS_COMMON_DEFINED ((*sec_ret)[section_tel], TRUE);
            }
          }
          else		/* Not allocated section */
          {
            /* Non alloced progbits are normally data describing the program, unused during dynamic 
             * linking or normal execution. If it is (dwarf2)debug information and the flag read_debug
             * is set, we will read these sections also */
            /* stab, comment, dwarf debug information, ??? */
            VERBOSE (2, ("   It's a debug section named %s", (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
            if (read_debug)
              if(StringPatternMatch(".debug*",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
              {
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);
                (*sec_ret)[section_tel] = 
                  ObjectAddSectionFromFile (obj, DEBUG_SECTION,
                                            TRUE, fp,
                                            OBJECT_STREAMPOS (obj) + (*shdr_ret)[section_tel].sh_offset,
                                            tmp_addr, tmp_sz, tmp_align,
                                            (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,
                                            section_tel);
              }

          }
          /* The only other real sections are bss sections (SHT_NOBITS) */
          break;
        case SHT_SYMTAB:
          /* This section hold a symbol table. There can be only one! (currently). We ignore it 
           * here: symbol_tableles should be fetched in STEP 3 */
          VERBOSE (2,
                   ("Found a symbol table name %s",
                    (*sechdrstrtab_ret) +
                    (*shdr_ret)[section_tel].sh_name));
          break;
        case SHT_STRTAB:
          /* This section holds a string table. We can have multiple string tables (in theory, the current ABI does not allow them) ! We ignore it
           * here: all necessary stringtables should be fetched in STEP 3 */
          VERBOSE (2, ("Found a string table"));

	  if (strcmp
              ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,
               ".dynstr") == 0)
	    {
	      tmp_addr =
		AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
	      tmp_sz =
		AddressNew64 ((*shdr_ret)[section_tel].sh_size);
	      tmp_align =
		AddressNew64 ((*shdr_ret)[section_tel].
			      sh_addralign);
	      (*sec_ret)[section_tel] =
		ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
					  fp,
					  OBJECT_STREAMPOS (obj) +
					  (*shdr_ret)
					  [section_tel].sh_offset,
					  tmp_addr, tmp_sz,
					  tmp_align,
					  (*sechdrstrtab_ret) +
					  (*shdr_ret)
					  [section_tel].sh_name,
                                          section_tel);
	    }
          break;
        case SHT_RELA:
          /* This section contains relocations with explicit addends. Currently unsupported... */
          VERBOSE (2,
                   ("Found relocations with explicit addends for section %s",
                    (*sechdrstrtab_ret) +
                    (*shdr_ret)[section_tel].sh_name + 5));
          if ((strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".rela.dyn") == 0)
          || (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".rela.plt") == 0))
          {
            tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);

            (*sec_ret)[section_tel] =
              ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
                                        fp,
                                        OBJECT_STREAMPOS (obj) +
                                        (*shdr_ret)
                                        [section_tel].sh_offset,
                                        tmp_addr, tmp_sz,
                                        tmp_align,
                                        (*sechdrstrtab_ret) +
                                        (*shdr_ret)
                                        [section_tel].sh_name,
                                        section_tel);
          }
          break;
        case SHT_HASH:
          /* This section contains a hash table (for dynamic linking). There can be only one! (currently).
           * No need to load this, although we could verify it's correctness, but we can regenerate it when
           * the binary is written*/
          VERBOSE (1, ("Found a hash table"));
	  tmp_addr =
		AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
	      tmp_sz =
		AddressNew64 ((*shdr_ret)[section_tel].sh_size);
	      tmp_align =
		AddressNew64 ((*shdr_ret)[section_tel].
			      sh_addralign);
	      (*sec_ret)[section_tel] =
		ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
					  fp,
					  OBJECT_STREAMPOS (obj) +
					  (*shdr_ret)
					  [section_tel].sh_offset,
					  tmp_addr, tmp_sz,
					  tmp_align,
					  (*sechdrstrtab_ret) +
					  (*shdr_ret)
					  [section_tel].sh_name,
                                          section_tel);
          break;
        case SHT_DYNAMIC:
          /* TODO */
          /* The section holds information for dynamic linking. There can be only one! (currently) */
          VERBOSE (1, ("Found a dynamic section...."));
          ASSERT (!dynamic_table,
                  ("Found 2 dynamic tables. This does not conform to current ELF (SYSV - ARM - INTEL) ABI specifications"));
          dynamic_table = Malloc ((*shdr_ret)[section_tel].sh_size);
          fseek (fp,
                 OBJECT_STREAMPOS (obj) +
                 (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
          fread (dynamic_table, 1, (*shdr_ret)[section_tel].sh_size, fp);

          tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
	  tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
	  tmp_align = AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);

	  (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, DATA_SECTION, FALSE,
                                            fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);

          while (dynamic_table[dynamic_tel].d_tag != DT_NULL)	/* An entry with a DT_NULL tag marks the end of the _DYNAMIC array. */
          {
            switch (dynamic_table[dynamic_tel].d_tag)
            {
              /* Holds a string table offset to a NULL-determined string, given the name of a needed libraryi. 
               * The offset points in the dynamic table to a DT_STRTAB entry*/
              case DT_NEEDED:
	      {
	      	t_string neededstring = StringConcat2("$dt_needed:",dynstrtab + dynamic_table[dynamic_tel].d_un.d_val);
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), neededstring , "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
		Free(neededstring);
                VERBOSE (1,
                         ("Need something dynamic: %s",
                          dynstrtab +
                          dynamic_table[dynamic_tel].d_un.d_val));
                break;
	      }
                /* Total size in bytes of the plt. Must be present if we have a DT_JMPREL entry */
              case DT_PLTRELSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltrelsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* An address associated with the PLT or GOT */
              case DT_PLTGOT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltgot", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of the (dynamic) Symbol hash table */
              case DT_HASH:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_hash", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of the (dynamic) string table */
              case DT_STRTAB:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_strtab", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of the (dynamic) symbol table */
              case DT_SYMTAB:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_symtab", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size in bytes of the (dynamic) string table */
              case DT_STRSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_strsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size in bytes of a (dynamic) symbol table entry */
              case DT_SYMENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_syment", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of a relocation table  with explicit addends. The dynamic linker sees only one table! */
              case DT_RELA:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_rela", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size of the the relocation table  with explicit addends. */
              case DT_RELASZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relasize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size of an element in the relocation table  with explicit addends. */
              case DT_RELAENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relaent", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of the init section */
              case DT_INIT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_init", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of the fini section */
              case DT_FINI:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_fini", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The address of a relocation table  with implicit addends. The dynamic linker sees only one table! */
              case DT_REL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_rel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size of the reloc table */
              case DT_RELSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The size of an element in the relocation table */
              case DT_RELENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relent", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* The type of relocation used in the PLT (rel or rela) */
              case DT_PLTREL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltrel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
                /* Used for debugging. Not specified by the ABI. Can be ignored?. */
              case DT_DEBUG:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_debug", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
              case DT_JMPREL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_jmprel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
              case DT_SYMBOLIC:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_symbolic", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
              case DT_FLAGS:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_flags", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                break;
              case DT_RPATH:
                {
                  t_string rpathstring = StringConcat2("$dt_rpath:",dynstrtab + dynamic_table[dynamic_tel].d_un.d_val);
                  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), rpathstring , "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                  Free(rpathstring);
                  VERBOSE (1,
                           ("RPATH: %s",
                            dynstrtab +
                            dynamic_table[dynamic_tel].d_un.d_val));
                  break;
                }
              default:
                if ((dynamic_table[dynamic_tel].d_tag >= DT_LOOS)
                    && (dynamic_table[dynamic_tel].d_tag <= DT_HIOS))
                {
                  WARNING (("Ignoring OS specific dynamic!"));
                }
                else if ((dynamic_table[dynamic_tel].d_tag >= DT_LOPROC)
                         && (dynamic_table[dynamic_tel].d_tag <=
                             DT_HIPROC))
                {
                  WARNING (("Ignoring Processor specific dynamic!"));
                }
                else
                {
                  FATAL (("Unknown d_tag: %d %x",
                          dynamic_table[dynamic_tel].d_tag,
                          dynamic_table[dynamic_tel].d_tag));
                }
            }
            dynamic_tel++;
          }
          WARNING (("Object has cross - object references ! "));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!! DYNAMICALLY LINKED PROGRAM DETECTED !!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("Seems like you are trying to optimize/rewrite a dynamically linked program"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("!!!!!!!!!!!!!!!!!!!! THIS IS CURRENTLY NOT SUPPORTED !!!!!!!!!!!!!!!!!!!!!"));
	  VERBOSE(0, ("But just for kicks we will continue and see how long it takes before Diablo exits..."));

          break;
        case SHT_NOTE:
          /* TODO */
          /* The section holds information that marks the file in some way */
          VERBOSE (2, ("Found a section marking the binary"));
          if (((*shdr_ret)[section_tel].sh_flags & SHF_ALLOC))	/* Must have alloc flags */
          {
            tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
            tmp_align =
              AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);
            (*sec_ret)[section_tel] =
              ObjectAddSectionFromFile (obj, NOTE_SECTION, TRUE, fp,
                                        OBJECT_STREAMPOS (obj) +
                                        (*shdr_ret)[section_tel].
                                        sh_offset, tmp_addr, tmp_sz,
                                        tmp_align,
                                        (*sechdrstrtab_ret) +
                                        (*shdr_ret)[section_tel].
                                        sh_name,
                                        section_tel);
          }
          break;
        case SHT_NOBITS:
          /* A section of this type occupies no space in the file but otherwise resembles SHT_PROGBITS. 
           * Although this section contains no bytes, the sh_offset member contains the conceptual 
           * file offset. */
          VERBOSE (2,
                   ("Found program specific information but no bits. Looking at the flags to determine it's data"));
          if (((*shdr_ret)[section_tel].sh_flags & SHF_ALLOC))	/* Must have alloc flags */
          {
            if (((*shdr_ret)[section_tel].sh_flags & SHF_EXECINSTR))
              FATAL (("Executable NOBITS!"));
            if (((*shdr_ret)[section_tel].sh_flags & SHF_WRITE))	/* Must be writable */
            {
              if (((*shdr_ret)[section_tel].sh_flags & SHF_TLS)) 
              {
                VERBOSE (2,
                         ("   It's a writable data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, TLSBSS_SECTION, FALSE, fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)[section_tel].
                                            sh_offset, tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)[section_tel].
                                            sh_name,
                                            section_tel);
              }
              else
              {
                VERBOSE (2,
                         ("   It's a writable data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, BSS_SECTION, FALSE, fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)[section_tel].
                                            sh_offset, tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)[section_tel].
                                            sh_name,
                                            section_tel);
              }
            }
            else
              FATAL (("No bits section without write(useless)"));
          }
          else if ((*shdr_ret)[section_tel].sh_size!=0)
            FATAL (("No bits section without alloc (useless) %s", (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name ));

          break;
        case SHT_REL:
          /* This section contains relocations without addends. We ignore those here, because they get
           * processed in STEP 5 */
          VERBOSE (2,
                   ("Found relocations without explicit addends for section %s",
                    (*sechdrstrtab_ret) +
                    (*shdr_ret)[section_tel].sh_name + 4));

          if ((strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".rel.dyn") == 0)
          || (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".rel.plt") == 0))
          {
            tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);

            (*sec_ret)[section_tel] =
              ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
                                        fp,
                                        OBJECT_STREAMPOS (obj) +
                                        (*shdr_ret)
                                        [section_tel].sh_offset,
                                        tmp_addr, tmp_sz,
                                        tmp_align,
                                        (*sechdrstrtab_ret) +
                                        (*shdr_ret)
                                        [section_tel].sh_name,
                                        section_tel);
          }
          break;
        case SHT_SHLIB:
          /* SHLIB is named in the System V ABI specs, but it's meaning is unspecified... Programs 
           * conformant to the ABI should not contain it */
          FATAL (("Found a SHT_SHLIB section. This is named in the System V ABI specs, but it's meaning is unspecified... Programs conformant to the ABI should not contain it. Send more info on this file to bdebus@@elis.ugent.be"));
          break;
        case SHT_DYNSYM:
          /* TODO */
          VERBOSE (2, ("SHT_DYNSYM"));
	  tmp_addr = AddressNew64 ((*shdr_ret)[section_tel].sh_addr);
	  tmp_sz = AddressNew64 ((*shdr_ret)[section_tel].sh_size);
	  tmp_align = AddressNew64 ((*shdr_ret)[section_tel].sh_addralign);

	  (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE,
                                            fp,
                                            OBJECT_STREAMPOS (obj) +
                                            (*shdr_ret)
                                            [section_tel].sh_offset,
                                            tmp_addr, tmp_sz,
                                            tmp_align,
                                            (*sechdrstrtab_ret) +
                                            (*shdr_ret)
                                            [section_tel].sh_name,
                                            section_tel);

          dynamic_symbol_table_section = (*sec_ret)[section_tel];
          break;
	case SHT_GROUP:
	  break;
        default:
          if (((*shdr_ret)[section_tel].sh_type >= SHT_LOOS)
              && ((*shdr_ret)[section_tel].sh_type <= SHT_HIOS))
          {
            /* TODO */
            WARNING (("Ignoring operating specific section type %x, name = %s", ((*shdr_ret)[section_tel].sh_type), (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
          }
          else if (((*shdr_ret)[section_tel].sh_type >= SHT_LOPROC)
                   && ((*shdr_ret)[section_tel].sh_type <= SHT_HIPROC))
          {
            /* TODO */
              WARNING (("Ignoring processor specific section type %x, name = %s", ((*shdr_ret)[section_tel].sh_type), (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
          }
          else if (((*shdr_ret)[section_tel].sh_type >= SHT_LOUSER)
                   && ((*shdr_ret)[section_tel].sh_type <= SHT_HIUSER))
          {
            /* TODO */
            WARNING (("Ignoring user specific section type %x, name = %s", ((*shdr_ret)[section_tel].sh_type), (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
          }
          else
          {
            FATAL (("Unknown section type! 0x%x, name %s of object %s",
                    ((*shdr_ret)[section_tel].sh_type),
                    (*sechdrstrtab_ret) +
                    (*shdr_ret)[section_tel].sh_name,
                    OBJECT_NAME(obj)));
          }
      }
      section_tel++;
    }
  }

  /* Read the symbol table and put it in a generic form... */
  if (hdr64->e_shnum)
  {
    ElfProcessSymbolTable64(obj, symbol_table_ret, strtab_ret, sec_ret, numsyms_ret, lookup, NULL);
  }

  if (numdynsym)
  {
    ElfProcessSymbolTable64(obj, dynamic_symbol_table_ret, &dynstrtab, sec_ret, &numdynsym, dynamic_lookup, dynamic_symbol_table_section);
  }

  if (dynstrtab) Free(dynstrtab);
  if (dynamic_table) Free(dynamic_table);
}

/* Read the symbol table and put it in a generic form... */
static void ElfProcessSymbolTable64(t_object * obj, Elf64_Sym ** symbol_table_ret, char **strtab_ret, t_section *** sec_ret, t_uint32 * numsyms_ret, t_symbol *** lookup, t_section * dynsec)
{
  t_symbol_table * symbol_table;
  t_symbol * tmp;
  t_uint32 symbol_tel = 0;	/* Counter for the symbols */
  t_address null = AddressNullForObject (obj);

  /* Create a new symbol table */
  if (!dynsec)
  {
    symbol_table = OBJECT_SYMBOL_TABLE (obj);
  }
  else
  {
    OBJECT_SET_DYNAMIC_SYMBOL_TABLE (obj, SymbolTableNew (obj));
    OBJECT_SET_DYNAMIC(obj, TRUE);
    symbol_table = OBJECT_DYNAMIC_SYMBOL_TABLE (obj);
  }

  if (lookup)
  {
    *lookup = (t_symbol **) Calloc ((*numsyms_ret), sizeof (t_symbol *));
  }

  /* Add all symbols */
  for (symbol_tel = 0; symbol_tel < (*numsyms_ret); symbol_tel++)
  {
    t_uint32 flags = 0;
    t_int32 order = 0;
    t_tristate dup;
    t_tristate search;

    t_address generic;
    t_const_string name=NULL, alias=NULL;

    /* ignore the reserved symbol */
    if (symbol_tel == 0)
      continue;

    name = StringDup((*strtab_ret) + (*symbol_table_ret)[symbol_tel].st_name);
    generic = AddressNew64 ((*symbol_table_ret)[symbol_tel].st_value);

    if (dynsec)
    {
      t_string dynname=StringConcat2("DYNSYMSYM:", name);
      VERBOSE(0, ("%ssymbol %s at @G: pos in file: @G", dynsec?"Dynamic ":"", name, generic, AddressNew64(sizeof(Elf64_Sym) * symbol_tel)));

      SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), dynname, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(dynsec), AddressNew64(sizeof(Elf64_Sym) * symbol_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      Free(dynname);
    }

    if (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_NOTYPE)
    {
      flags |= SYMBOL_TYPE_NOTYPE;

      if (StringPatternMatch("$d.*", name))
      {
        alias = "$d";
        order = -1;
        dup = TRUE;
        search = FALSE;
      }
      else if (StringPatternMatch("$a.*", name))
      {
        alias = "$a";
        order = -1;
        dup = TRUE;
        search = FALSE;
      }
      else if (StringPatternMatch("$t.*", name))
      {
        alias = "$t";
        order = -1;
        dup = TRUE;
        search = FALSE;
      }
      else
      {
        if (ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
        {
          order = 10;
          dup = FALSE;
          search = FALSE;
        }
        else if (ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_LOCAL)
        {
          order = -1;
          dup = TRUE;
          search = FALSE;
        }
        else /* WEAK */
        {
          order = 5;
          dup = PERHAPS;
          search = FALSE;
        }

      }
    }
    else if ((ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_OBJECT) 
             || (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_FILE)
             || (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_SECTION)
             || (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_FUNC)
             || (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_TLS))
    {
      switch (ELF64_ST_TYPE((*symbol_table_ret)[symbol_tel].st_info))
      {
        case STT_FILE:
          flags |= SYMBOL_TYPE_FILE;
          break;
        case STT_OBJECT:
          flags |= SYMBOL_TYPE_OBJECT;
          break;
        default:
          /* do nothing */
          break;
      }

      if (ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
      {
        order = 10;
        dup = FALSE;
        search = FALSE;
      }
      else if (ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_LOCAL)
      {
        order = -1;
        dup = TRUE;
        search = FALSE;
      }
      else
      {
        order = 5;
        dup = PERHAPS;
        search = FALSE;
      }
    }
    else if ((ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info)>=STT_LOPROC)
             && (ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info)<=STT_HIPROC))
    {
      /* Proc specific. Handle these in the architecture backend..... */
      continue;
    }
    else
    {
      FATAL (("Symbol %s has unknown symbol type %d\n",
              (*strtab_ret) +
              (*symbol_table_ret)[symbol_tel].st_name,
              ELF64_ST_TYPE ((*symbol_table_ret)[symbol_tel].
                             st_info)));
    }

    if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_COMMON)
    {
      /* Common symbols have a somewhat lower priority than global symbols */

      t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
      t_uint32 alignment = (*symbol_table_ret)[symbol_tel].st_value;
      t_string symcode = StringIo("CommonSection {\
                                  action {\
                                  ADD_SUBSECTION(\"Linker\", \".bss\", CONCAT(\"COMMON:\",MATCHED_NAME()), BSS, %d, MATCHED_SYMBOL_SIZE())\
                                  }\
                                  address { SYMBOL(MATCHED_NAME()) }\
                                  }\
                                  CommonSymbol {\
                                  action {\
                                  ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
                                  }\
                                  symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"COMMON:\",MATCHED_NAME())) }\
                                  }\
                                  ", alignment);
      tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                  name, alias, "R00A00+$", 8, PERHAPS, search,
                                  T_RELOCATABLE (undef),
                                  null,
                                  null,
                                  symcode,
                                  AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);
      Free(symcode);
      SYMBOL_SET_FLAGS(tmp, flags);
      if (lookup)  (*lookup)[symbol_tel] = tmp;
    }
    /* Constants */
    else if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_ABS)
    {
      t_section * abs= OBJECT_PARENT(obj)?OBJECT_ABS_SECTION(OBJECT_PARENT(obj)):OBJECT_ABS_SECTION(obj);


      ASSERT(abs, ("Could not get abs section: object %s %s has no abs section", OBJECT_NAME(obj), OBJECT_PARENT(obj)?"has a parent, but this parent":"has no parent but it also"));

      tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                  name, alias, "R00A00+$", order, dup, search,
                                  T_RELOCATABLE (abs),
                                  generic,
                                  null, NULL, AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);

      SYMBOL_SET_FLAGS(tmp, flags);
      if (lookup)  (*lookup)[symbol_tel] = tmp;
    }
    else if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_UNDEF)
    {
      t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);

      ASSERT ((ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) != STB_LOCAL),("Found an undefined local symbol!"));

      if (ELF64_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
      {
	if (!dynsec)
          tmp = SymbolTableAddAliasedSymbol (symbol_table, name, alias, "R00A00+$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), null, null, NULL, AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);
        else
          tmp = SymbolTableAddAliasedSymbol (symbol_table, name, alias, "A00$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), null, null, NULL, AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);

        SYMBOL_SET_FLAGS(tmp, flags);
        if (lookup)  (*lookup)[symbol_tel] = tmp;
      }
      else /* WEAK UNDEFINED */
      {
        tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                    name, alias, "R00A00+$", 4, PERHAPS, FALSE,
                                    T_RELOCATABLE (undef),
                                    null,
                                    null, NULL, AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);

        SYMBOL_SET_FLAGS(tmp, flags);
        if (lookup)  (*lookup)[symbol_tel] = tmp;
      }
    }
    else 
    {
      t_section *sec = (*sec_ret)[(*symbol_table_ret)[symbol_tel].st_shndx];

      if (!sec)
      {
        Free(name);
        Free(alias);
        continue;
      }

      tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                  name, alias, "R00A00+$", order, dup, search,
                                  T_RELOCATABLE (sec),
                                  AddressSub(generic,SECTION_CADDRESS(sec)),
                                  null, NULL, AddressNew64((*symbol_table_ret)[symbol_tel].st_size), 0);

      SYMBOL_SET_FLAGS(tmp, flags);
      if (lookup) (*lookup)[symbol_tel] = tmp;
    }

    Free(name);
  }
}
#endif
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
