/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT32ADDRSUPPORT
#include <diabloelf.h>


/* Configuration {{{ */
#define SYMTABNAME ".symtab"
#define SYMSTRTABNAME ".strtab"
/*#define VERBOSE_WRITE */
#define MAX_PDR_DIFF 100
#define SEPARATE_WRITABLE_SEGMENT
/* }}} */

#define VERDEF_FOREACH_ENTRY(verdefdata, entry) \
  for (entry = (Elf32_Verdef *)verdefdata; entry != NULL; entry = entry->vd_next != 0? (Elf32_Verdef *)(((void *)entry)+entry->vd_next):NULL)

#define VERDEF_ENTRY_FOREACH_AUX(entry, aux, auxidx) \
  for (auxidx = 0, aux = (Elf32_Verdaux *)(((void *)entry)+entry->vd_aux); auxidx < entry->vd_cnt; auxidx++, aux = aux->vda_next != 0? (Elf32_Verdaux *)(((void *)aux) + aux->vda_next):NULL)


#define VERNEED_FOREACH_ENTRY(verneeddata, entry) \
  for (entry = (Elf32_Verneed *)verneeddata; entry != NULL; entry = entry->vn_next != 0? (Elf32_Verneed *)(((void *)entry)+entry->vn_next):NULL)

#define VERNEED_ENTRY_FOREACH_AUX(entry, aux, auxidx) \
  for (auxidx = 0, aux = (Elf32_Vernaux *)(((void *)entry)+entry->vn_aux); auxidx < entry->vn_cnt; auxidx++, aux = aux->vna_next != 0? (Elf32_Vernaux *)(((void *)aux) + aux->vna_next):NULL)


static void ElfProcessSymbolTable32(t_object * obj, Elf32_Sym ** symbol_table_ret, char **strtab_ret, t_section *** sec_ret, t_uint32 * numsyms_ret, t_symbol *** lookup, t_section * sec);

/* sec_store_info: A struct used to describes how and where a section will be
 * stored in the file {{{ */
typedef struct
{
  t_section *sec;
  Elf32_Shdr hdr;
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

  /* non-alloc sections come last */
  if ((seca->hdr.sh_flags & SHF_ALLOC) &&
      !(secb->hdr.sh_flags & SHF_ALLOC))
    return -1;
  else if ((secb->hdr.sh_flags & SHF_ALLOC) &&
      !(seca->hdr.sh_flags & SHF_ALLOC))
    return 1;

  if (seca->hdr.sh_addr > secb->hdr.sh_addr)
    return 1;
  else if (seca->hdr.sh_addr < secb->hdr.sh_addr)
    return -1;
  else if (seca->hdr.sh_addr == secb->hdr.sh_addr)
  {
    /* Ensure that SHF_TLS sections come first. This is necessary, because
     * the .tbss section might have the same address as the section following
     * the .tbss section, yet we want to make sure it is the section
     * following the .tdata section */
    if (seca->hdr.sh_flags & SHF_TLS) return -1;
    else if (secb->hdr.sh_flags & SHF_TLS) return 1;
  }

  return 0;
}

/* }}} */

/* {{{*/
static t_section *
GetDynSymSection(t_object *obj, t_string symname)
{
  t_section * psec = SectionGetFromObjectByName(obj, ".dynsym");
  t_object * lo = ObjectGetLinkerSubObject(obj);

  t_section * sec;
  t_string secname;

  secname = StringConcat2("DYNSYM:", symname);
  sec = SectionGetFromObjectByName(lo, secname);
  Free(secname);
  if (!sec)
  {
    secname = StringConcat2("WEAKDYNSYM:", symname);
    sec = SectionGetFromObjectByName(lo, secname);
    Free(secname);
  }
  return sec;
}
/*}}}*/

/*! This function writes a diablo object as an elf-object file
 *
 * \param fp The file pointer on which we will store the object file
 * \param hdr The file header (Ident, Type, Machine, Version and Entry must already be set)
 * \param obj The object to write
 * \param hinfo ELF header info for the target architecture (pagesize, alignment of various headers)
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
/* ElfWriteCommon32 {{{ */
static t_uint32 _align(t_uint32 addr, t_uint32 align)
{
  if (align == 0) return addr;
  return (addr + (align - 1)) & ~(align - 1);
}

#define SYMTAB
void
ElfWriteCommon32 (FILE * fp, Elf32_Ehdr * hdr, t_object * obj,
                          t_bool hoist_headers, t_bool switch_endian,
                          Elf32_HeaderInfo const * const hinfo)
{
#ifdef SYMTAB
  t_symbol * sym;
  Elf32_Shdr symtabshdr;
  Elf32_Shdr symstrtabshdr;
  t_string symstrtab;
#endif

  Elf32_Phdr *phdrs = NULL;
  Elf32_Shdr nullshdr, sectabshdr;
  t_segment *seg = OBJECT_SEGMENT_FIRST (obj);
  t_uint64 secstrtabsz = 1, runstrtabsz = 1;
  t_uint32 tel, prev_handled_secnr, nphdrs = 0, nsecs = 0;
  t_uint32 run = 0;
  t_uint32 paddr = 0, vaddr = 0;
  t_uint32 can_be_placed_at;
  int phdr_pdr = -1;
  t_string secstrtab;
  t_bool sec_first = TRUE;
  t_bool prev_was_overlay = FALSE;
  t_bool prev_was_writable = FALSE;
  t_bool prev_was_tls = FALSE;
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

  for (tel = 0; tel < OBJECT_NATTRIBS (obj); tel++)
  {
    VERBOSE (0,
             ("Attrib section %d: %s at @G size @G old address @G", tel,
              SECTION_NAME (OBJECT_ATTRIB (obj)[tel]),
              SECTION_CADDRESS (OBJECT_ATTRIB (obj)[tel]),
              SECTION_CSIZE (OBJECT_ATTRIB (obj)[tel]),
              SECTION_OLD_ADDRESS (OBJECT_ATTRIB (obj)[tel])));
  }
#endif

  /* End of testing }}} */
  /* Calculate the section - string table size "\0.section\0... and the number of sections" {{{ */

  for (tel = 0; tel < OBJECT_NCODES (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_CODE (obj)[tel])))
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
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel])))
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
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_DATA (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_DATA (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_DATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NTLSDATAS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSDATA (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_TLSDATA (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_TLSDATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NBSSS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_BSS (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_BSS (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_BSS (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NTLSBSSS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSBSS (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_TLSBSS (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_TLSBSS (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NNOTES (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel])))
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
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_DEBUG (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_DEBUG (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NATTRIBS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_ATTRIB (obj)[tel])))
      continue;
    secs = Realloc (secs, sizeof (sec_store_info) * (nsecs + 1));
    secs[nsecs].sec = OBJECT_ATTRIB (obj)[tel];
    secs[nsecs].dynamic_pdr = -1;
    secs[nsecs].interp_pdr = -1;
    secstrtabsz += strlen (SECTION_NAME (OBJECT_ATTRIB (obj)[tel])) + 1;
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
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_CODE (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    DiabloBrokerCall("SetSectionFlags",&(secs[nsecs].hdr.sh_flags));
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_CODE (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_CODE (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_CODE (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NRODATAS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC;
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;

    if (strcmp(".dynsym",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_DYNSYM;
      secs[nsecs].hdr.sh_entsize = 16;
    }
    else if (strcmp(".dynstr",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_STRTAB;
    }
    else if (strcmp(".rel.dyn",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_REL;
      secs[nsecs].hdr.sh_entsize = 0x8;
    }
    else if (strcmp(".rel.plt",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_REL;
      secs[nsecs].hdr.sh_entsize = 0x8;
    }
    else if (strcmp(".rela.dyn",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_RELA;
      secs[nsecs].hdr.sh_entsize = 0xc;
    }
    else if (strcmp(".rela.plt",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_RELA;
      secs[nsecs].hdr.sh_entsize = 0xc;
    }
    else if (strcmp(".gnu.version",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_GNU_versym;
      secs[nsecs].hdr.sh_entsize = sizeof(Elf32_Half);
    }
    else if (strcmp(".gnu.version_d",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      Elf32_Verdef *entry;
      int nverdefentries;

      secs[nsecs].hdr.sh_type = SHT_GNU_verdef;
      /* sh_info must contain the number of verdef entries */
      nverdefentries = 0;
      VERDEF_FOREACH_ENTRY(SECTION_DATA(OBJECT_RODATA (obj)[tel]),entry)
      {
        nverdefentries++;
      }
      secs[nsecs].hdr.sh_info = nverdefentries;
    }
    else if (strcmp(".gnu.version_r",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      Elf32_Verneed *entry;
      int nverneedentries;

      secs[nsecs].hdr.sh_type = SHT_GNU_verneed;
      /* sh_info must contain the number of verdef entries */
      nverneedentries = 0;
      VERNEED_FOREACH_ENTRY(SECTION_DATA(OBJECT_RODATA (obj)[tel]),entry)
      {
        nverneedentries++;
      }
      secs[nsecs].hdr.sh_info = nverneedentries;
    }
    else if (strcmp(".hash",SECTION_NAME (OBJECT_RODATA (obj)[tel]))==0)
    {
      t_symbol * symptr;
      t_uint32 nbuckets = ElfComputeBucketCount (obj, sizeof(Elf32_Sym), 4096, 0);

      /* The magic number 3 below consists of: 1 entry for nbuckets, 1 for nchains, and 1 for the NULLDYNSYM (which isn't in the table). */
      t_uint32 size = 4* (3 + nbuckets + SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)));
      t_uint32 * data = SECTION_DATA(OBJECT_RODATA (obj)[tel]);

      ASSERT(AddressIsGe(SECTION_CSIZE(OBJECT_RODATA (obj)[tel]), AddressNew32(size)), ("Hash table under dimensioned (need size @G have @G)", size, SECTION_CSIZE(OBJECT_RODATA (obj)[tel])));

      /* Zero out the contents of the section */
      memset(data, 0, size);

      data[0]=nbuckets;
      data[1]=SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj))+1;

      for (symptr=SYMBOL_TABLE_FIRST(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
      {
        t_section * psec = SectionGetFromObjectByName(obj, ".dynsym");
        t_section *sec;

        sec = GetDynSymSection(obj,SYMBOL_NAME(symptr));
        if (sec)
        {
          t_uint32 mybucket = ElfHash(SYMBOL_NAME(symptr)) % nbuckets;
          t_uint32 idx = AddressExtractUint32(AddressSub(SECTION_CADDRESS(sec), SECTION_CADDRESS(psec)))/sizeof(Elf32_Sym);
          ASSERT(idx <= SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)),("Symbol index for @S higher than number of dynsyms (check for duplicate generated dynsyms)",symptr));
          VERBOSE(1, ("Doing %s putting it in bucket %d (idx = %d)", SYMBOL_NAME(symptr),  mybucket, idx));
          data [2 + nbuckets + idx] = data [2 + mybucket ];
          data [2 + mybucket ] = idx;
        }
        else
          VERBOSE(1, ("NOT Doing %s", SYMBOL_NAME(symptr)));

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
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_RODATA (obj)[tel]));
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_RODATA (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_RODATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NDATAS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_DATA (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    if (!strcmp(SECTION_NAME(OBJECT_DATA (obj)[tel]),".preinit_array"))
      secs[nsecs].hdr.sh_type = SHT_PREINIT_ARRAY;
    else if (!strcmp(SECTION_NAME(OBJECT_DATA (obj)[tel]),".init_array"))
      secs[nsecs].hdr.sh_type = SHT_INIT_ARRAY;
    else if (!strcmp(SECTION_NAME(OBJECT_DATA (obj)[tel]),".fini_array"))
      secs[nsecs].hdr.sh_type = SHT_FINI_ARRAY;
    else
      secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE;
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;

    /* Power PC has .got section executable. */
    if ((strcmp(".got",SECTION_NAME (OBJECT_DATA (obj)[tel]))==0)&&(hdr->e_machine==EM_PPC))
    {
      secs[nsecs].hdr.sh_flags = secs[nsecs].hdr.sh_flags | SHF_EXECINSTR;
    }
    else if (strcmp(".got",SECTION_NAME (OBJECT_DATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_entsize = 0x4;
    }
    else if (strcmp(".dynamic",SECTION_NAME (OBJECT_DATA (obj)[tel]))==0)
    {
      secs[nsecs].hdr.sh_type = SHT_DYNAMIC;
      secs[nsecs].hdr.sh_entsize = 0x8;
    }

    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_DATA (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_DATA (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_DATA (obj)[tel]));
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_DATA (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_DATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NTLSDATAS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSDATA (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE | SHF_TLS;
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;

    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_TLSDATA (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSDATA (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_TLSDATA (obj)[tel]));
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_TLSDATA (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_TLSDATA (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NBSSS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_BSS (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_NOBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_BSS (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_BSS (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_BSS (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NTLSBSSS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSBSS (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_NOBITS;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC | SHF_WRITE | SHF_TLS;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_TLSBSS (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_TLSBSS (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_TLSBSS (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_TLSBSS (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_TLSBSS (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NNOTES (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_NOTE;
    secs[nsecs].hdr.sh_flags = SHF_ALLOC;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_NOTE (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_NOTE (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_NOTE (obj)[tel])) + 1;
    nsecs++;
  }
  for (tel = 0; tel < OBJECT_NDEBUGS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_PROGBITS;
    secs[nsecs].hdr.sh_flags = 0;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_DEBUG (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_DEBUG (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_DEBUG (obj)[tel])) + 1;
    nsecs++;
  }

  for (tel = 0; tel < OBJECT_NATTRIBS (obj); tel++)
  {
    if (!G_T_UINT32 (SECTION_CSIZE (OBJECT_ATTRIB (obj)[tel])))
      continue;
    secs[nsecs].hdr.sh_name = runstrtabsz;
    secs[nsecs].hdr.sh_type = SHT_ARM_ATTRIBUTES;
    secs[nsecs].hdr.sh_flags = 0;
    secs[nsecs].hdr.sh_addr =
      G_T_UINT32 (SECTION_CADDRESS (OBJECT_ATTRIB (obj)[tel]));
    secs[nsecs].hdr.sh_size =
      G_T_UINT32 (SECTION_CSIZE (OBJECT_ATTRIB (obj)[tel]));
    secs[nsecs].hdr.sh_offset = 0;
    secs[nsecs].hdr.sh_link = SHN_UNDEF;
    secs[nsecs].hdr.sh_info = 0;
    secs[nsecs].hdr.sh_addralign =
      G_T_UINT32 (SECTION_ALIGNMENT (OBJECT_ATTRIB (obj)[tel]));
    secs[nsecs].hdr.sh_entsize = 0;
    secs[nsecs].prepadding = -1;
    sprintf (secstrtab + runstrtabsz, "%s",
             SECTION_NAME (OBJECT_ATTRIB (obj)[tel]));
    runstrtabsz += strlen (SECTION_NAME (OBJECT_ATTRIB (obj)[tel])) + 1;
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
    t_int32 versymidx = -1;
    t_int32 verdefidx = -1;
    t_int32 verneedidx = -1;
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
      else if (strcmp(".gnu.version",SECTION_NAME (secs[tel].sec)) == 0)
      {
        versymidx = tel;
      }
      else if (strcmp(".gnu.version_d",SECTION_NAME (secs[tel].sec)) == 0)
      {
        verdefidx = tel;
      }
      else if (strcmp(".gnu.version_r",SECTION_NAME (secs[tel].sec)) == 0)
      {
        verneedidx = tel;
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

    if (versymidx != -1)
    {
      ASSERT(dynsymidx != -1, ("versym should be dynsym section"));
      secs[versymidx].hdr.sh_link = dynsymidx + 1;
    }

    if (verdefidx != -1)
    {
      ASSERT(dynstridx != -1, ("verdef strings should be in dynstr section"));
      secs[verdefidx].hdr.sh_link = dynstridx + 1;
    }

    if (verneedidx != -1)
    {
      ASSERT(dynstridx != -1, ("verneed strings should be in dynstr section"));
      secs[verneedidx].hdr.sh_link = dynstridx + 1;
    }
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
    symtabshdr.sh_size=sizeof(Elf32_Sym);

    if (OBJECT_SYMBOL_TABLE(obj))
    for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
    {
      symtabshdr.sh_size += sizeof(Elf32_Sym);
    }

    symtabshdr.sh_offset=0;
    symtabshdr.sh_link=nsecs+3;
    symtabshdr.sh_info=symtabshdr.sh_size/sizeof(Elf32_Sym);
    symtabshdr.sh_addralign=4;
    symtabshdr.sh_entsize=sizeof(Elf32_Sym);

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
        /* We won't write away the names of section symbols */
        if (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_SECTION))/* Vade retro satana! */
          symstrtabshdr.sh_size += strlen(SYMBOL_NAME(sym)) + 1;
      }
    symstrtabshdr.sh_offset=0;
    symstrtabshdr.sh_link=0;
    symstrtabshdr.sh_info=0;
    symstrtabshdr.sh_addralign=4;
    symstrtabshdr.sh_entsize=0;
  }
#endif

  if (OBJECT_DYNAMIC_SYMBOL_TABLE(obj))
  {
    /* set the section index for dynamic symbols that aren't undefined */
    t_symbol *symptr;

    for (symptr=SYMBOL_TABLE_FIRST(OBJECT_DYNAMIC_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
    {
      t_relocatable *baserel;
      t_section *base;
      VERBOSE(3,("checking dynsym %s",SYMBOL_NAME(symptr)));
      baserel = SYMBOL_BASE(symptr);
      if (RELOCATABLE_RELOCATABLE_TYPE(baserel) == RT_SUBSECTION)
        base = SECTION_PARENT_SECTION(T_SECTION(baserel));
      else
        base = T_SECTION(baserel);
      VERBOSE(3,("Base: @T",base));
      if (base != OBJECT_UNDEF_SECTION(obj))
      {
        int i, secnr;
        t_section *dynsymsec, *dynsymparentsec;
        t_address shndxoffset;

        if (base == OBJECT_ABS_SECTION(obj))
        {
          secnr = SHN_ABS;
        }
        else
        {
          secnr = -1;
          for (i=0; i<nsecs; i++)
          {
            if (secs[i].sec == base)
            {
              /* secnr + 1, because the first section is a dummy NULL section */
              secnr = i+1;
              break;
            }
          }
        }
        /* in case of eh_frame section symbol, or other section symbols
         * for sections we don't handle
         */
        if (secnr != -1)
        {
          if (secnr != SHN_ABS)
          {
            VERBOSE(3,("  Found in section nr %d == @T",secnr,secs[secnr-1].sec));
          }
          else
          {
            VERBOSE(3,("  Found in section @T",OBJECT_ABS_SECTION(obj)));
          }

          dynsymsec = GetDynSymSection(obj,SYMBOL_NAME(symptr));
          /* the symbol may have been killed */
          if (dynsymsec)
          {
            dynsymparentsec = SECTION_PARENT_SECTION(dynsymsec);
            shndxoffset = AddressAddUint32(AddressSub(SECTION_CADDRESS(dynsymsec),SECTION_CADDRESS(dynsymparentsec)),14);
            SectionSetData16(dynsymparentsec,G_T_UINT32(shndxoffset),secnr);
          }
        }
        else
        {
          VERBOSE(3,("  Not found in any section, not setting section index"));
        }
      }
    }
  }

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
  run = secs[0].hdr.sh_addr;
  prev_handled_secnr = -1;

  if (diabloobject_options.keep_exidx)
    {
      for (tel = 0; tel < nsecs; tel++)
        {
          if (strcmp(secstrtab + secs[tel].hdr.sh_name, ".ARM.exidx") == 0)
            {
              phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
              phdrs[nphdrs].p_type = PT_ARM_EXIDX;
              phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */
              
              phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr = secs[tel].hdr.sh_addr;
              phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
              phdrs[nphdrs].p_align = secs[tel].hdr.sh_addralign;
              phdrs[nphdrs].p_flags = PF_R;
              phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
              nphdrs++;
            }
        }
    }

  /* "PT_PHDR ... If it is present, it must precede any loadable segment
      entry" (ELF standard). This is only strictly required for PIE
      binaries, but since those cannot be distinguished from dynamic
      libraries, add them for for all dynamic binaries.
   */
  /* for the dynamic binaries, this was added because libld functionality
     such as dladdr() depends on the header being present */

  if ((OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC) ||
      (OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC)||
      (OBJECT_DYNAMIC(obj))
      )
  {
    phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
    phdrs[nphdrs].p_type = PT_PHDR;
    phdrs[nphdrs].p_offset = sizeof (Elf32_Ehdr);

    phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
      sizeof (Elf32_Ehdr);
    phdrs[nphdrs].p_memsz = 0; 
    phdrs[nphdrs].p_align = hinfo->ptphdr_align;
    phdrs[nphdrs].p_flags = PF_R;
    phdrs[nphdrs].p_filesz = 0; 
    phdr_pdr = nphdrs;
    nphdrs++;
  }
  /* "PT_INTERP ... If it is present, it must precede any loadable segment
      entry" (ELF standard)
   */
  for (tel = 0; tel < nsecs; tel++)
  {
    if (strcmp(secstrtab + secs[tel].hdr.sh_name, ".interp") == 0)
    {
        phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_INTERP;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
        secs[tel].interp_pdr = nphdrs;
        phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
        ASSERT(hinfo->ptinterp_align!=(Elf32_Word)-1,("Have to write PT_INTERP header but its alignment is not yet set for this target"));
        phdrs[nphdrs].p_align = hinfo->ptinterp_align;
        phdrs[nphdrs].p_flags = PF_R;
        phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
        nphdrs++;
    }
  }
  for (tel = 0; tel < nsecs; tel++)
  {
    paddr = _align(run, secs[tel].hdr.sh_addralign);
    vaddr = secs[tel].hdr.sh_addr;

    if (vaddr > paddr)  /* happens if there is a gap between sections */
      paddr = vaddr;

    /* PT_LOAD: all SHF_ALLOC sections - the .tbss sections need a PT_TLS
     * segment. .tdata resides in two segments (PT_LOAD and PT_TLS) */
    if ((secs[tel].hdr.sh_flags & SHF_ALLOC) &&
        (!(((secs[tel].hdr.sh_type == SHT_NOBITS) &&
            (secs[tel].hdr.sh_flags & SHF_TLS)))))
    {
      /* Alloc new segment when necessary */
      if ((paddr > run + MAX_PDR_DIFF) || (sec_first)
          || (seg && (G_T_UINT32 (SEGMENT_CADDRESS (seg)) <= secs[tel].hdr.sh_addr))
          || (SECTION_FLAGS(secs[tel].sec) & SECTION_FLAG_IN_OVERLAY)
          || prev_was_overlay
#ifdef SEPARATE_WRITABLE_SEGMENT
          || ((secs[tel].hdr.sh_flags & SHF_WRITE) && !prev_was_writable)
#endif
         )
      {
#ifdef VERBOSE_WRITE
        VERBOSE(0,
                ("Allocing new program header (%d) for %s with memsize %d, diff between previous programheader = %d bytes",
                 nphdrs, secstrtab + secs[tel].hdr.sh_name,
                 secs[tel].hdr.sh_size,
                 paddr - run));
#endif

        prev_was_overlay = FALSE;
        if (SECTION_FLAGS(secs[tel].sec) & SECTION_FLAG_IN_OVERLAY)
          prev_was_overlay = TRUE;

        phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_LOAD;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        /*phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =*/
          /*secs[tel].hdr.sh_addr;*/
        phdrs[nphdrs].p_paddr = paddr;
        phdrs[nphdrs].p_vaddr = secs[tel].hdr.sh_addr;

        phdrs[nphdrs].p_memsz = 0;
        phdrs[nphdrs].p_align = hinfo->ptload_align;
        phdrs[nphdrs].p_flags = PF_R;
        phdrs[nphdrs].p_filesz = 0;
        nphdrs++;
        secs[tel].prepadding = -1;
        secs[tel].hoist_headers = FALSE;

        if (seg)
          VERBOSE(1,("Current segment=%s\n", SEGMENT_NAME (seg)));

        if (sec_first)
          run = secs[tel].hdr.sh_addr;
        if ((sec_first) && (!seg))
          secs[tel].hoist_headers = TRUE;

        if (seg && (G_T_UINT32 (SEGMENT_CADDRESS (seg)) <= secs[tel].hdr.sh_addr))
        {
          if (SEGMENT_HOIST_HEADERS (seg))
          {
            /*if (!sec_first)
              FATAL (("Segment hoists the headers, but is not first in file")); */
            secs[tel].hoist_headers = TRUE;
          }
          VERBOSE(1,("Seg %s started\n", SEGMENT_NAME (seg)));
          seg = SEGMENT_NEXT (seg);
          if (seg)
            VERBOSE(1,("New seg is %s\n", SEGMENT_NAME (seg)));
        }
        sec_first = FALSE;
        prev_handled_secnr = tel;
      }
      /* XXX Dominique: disabled this as it is no longer possible to perform this check when you have overlays
      else if (secs[tel].hdr.sh_addr < run_v)
      {
        FATAL (("Sections overlap (or are not sorted as expected) for section %s at @G size @G!(run_v = 0x%x)", SECTION_NAME (secs[tel].sec), SECTION_CADDRESS (secs[tel].sec), SECTION_CSIZE (secs[tel].sec), run_v));
      }*/
      else
      {
        /* the previously used section is not necessarily tel-1,
         * namely when a section has been skipped (such as tbss)
         */
        secs[tel].prepadding = secs[tel].hdr.sh_addr - (secs[prev_handled_secnr].hdr.sh_addr + secs[prev_handled_secnr].hdr.sh_size);
        secs[tel].hoist_headers = FALSE;
        phdrs[nphdrs - 1].p_memsz += secs[tel].prepadding;
        phdrs[nphdrs - 1].p_filesz +=
          (secs[tel].hdr.sh_type != SHT_NOBITS) ? secs[tel].
          prepadding : 0;
#ifdef VERBOSE_WRITE
        VERBOSE(0,
                ("Hoisting %s with memsize %d in previous program header, diff with previous section = %d bytes. Current segment size: %d",
                 secstrtab + secs[tel].hdr.sh_name,
                 secs[tel].hdr.sh_size,
                 secs[tel].prepadding,
                 phdrs[nphdrs - 1].p_memsz));
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
      run = paddr + secs[tel].hdr.sh_size;

      prev_was_writable = ((secs[tel].hdr.sh_flags & SHF_WRITE) != 0);
      prev_was_tls = ((secs[tel].hdr.sh_flags & SHF_TLS) != 0);
      prev_handled_secnr = tel;
    }
    else
    {
      secs[tel].described_by_pdr = -1;
      /* for .tbss */
      if (secs[tel].hdr.sh_flags & SHF_TLS)
      {
        prev_was_writable = TRUE;
        prev_was_tls = TRUE;
      }
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
        phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
        phdrs[nphdrs].p_type = PT_DYNAMIC;
        phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

        phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
        secs[tel].dynamic_pdr = nphdrs;
        phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
        ASSERT(hinfo->ptdynamic_align!=(Elf32_Word)-1,("Have to write PT_DYNAMIC header but its alignment is not yet set for this target"));
        phdrs[nphdrs].p_align = hinfo->ptdynamic_align;
        phdrs[nphdrs].p_flags = PF_R | PF_W;
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
          phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
          phdrs[nphdrs].p_type = PT_TLS;
          phdrs[nphdrs].p_offset = 0;	/* Depends on nphdrs */

          phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr =
          secs[tel].hdr.sh_addr;
          secs[tel].dynamic_pdr = nphdrs;
          phdrs[nphdrs].p_memsz = secs[tel].hdr.sh_size;
          ASSERT(hinfo->pttls_align!=(Elf32_Word)-1,("Have to write PT_TLS header but its alignment is not yet set for this target"));
          phdrs[nphdrs].p_align = hinfo->pttls_align;
          phdrs[nphdrs].p_flags = PF_R;

          if (secs[tel].hdr.sh_type != SHT_NOBITS)
            phdrs[nphdrs].p_filesz = secs[tel].hdr.sh_size;
          else
            phdrs[nphdrs].p_filesz = 0;

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

  /* if original object had GNU_STACK_FLAGS segment to indicate memory
     protection of the stack segment, make sure the rewritten binary gets
     the same */

  if (OBJECT_GNU_STACK_FLAGS(obj))
  {
    phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
    phdrs[nphdrs].p_type = PT_GNU_STACK;
    phdrs[nphdrs].p_offset = 0;
    phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr = 0;
    phdrs[nphdrs].p_memsz = 0; 
    phdrs[nphdrs].p_align = 4;
    phdrs[nphdrs].p_flags = OBJECT_GNU_STACK_FLAGS(obj);
    phdrs[nphdrs].p_filesz = 0; 
    nphdrs++;
  }

  if (OBJECT_RELRO_CSIZE(obj))
  {
    /* offset will be filled in later!!! */
    t_uint32 offset = 0;
    t_uint32 lastload;

    phdrs = Realloc (phdrs, sizeof (Elf32_Phdr) * (nphdrs + 1));
    phdrs[nphdrs].p_type = PT_RELRO;
    phdrs[nphdrs].p_offset = offset;
    phdrs[nphdrs].p_paddr = phdrs[nphdrs].p_vaddr = OBJECT_RELRO_NEW_ADDRESS(obj);
    phdrs[nphdrs].p_memsz = OBJECT_RELRO_NEW_SIZE(obj); 
    phdrs[nphdrs].p_align = 1; /* nothing to add on top of alignment of load segment with which this overlaps */
    phdrs[nphdrs].p_flags = PF_R; /* by construction */
    phdrs[nphdrs].p_filesz = OBJECT_RELRO_NEW_SIZE(obj); 
    nphdrs++;
  }

  /* now that we know the number of segments, we can set the size of
   * the program headers
   */
  if (phdr_pdr != -1)
  {
    phdrs[phdr_pdr].p_memsz = nphdrs * sizeof (Elf32_Phdr);
    phdrs[phdr_pdr].p_filesz = nphdrs * sizeof (Elf32_Phdr);
  }

  /* }}} */
  /* Now place all segments in the file: segments file offsets need to be
   * congruent with the virtual address modulo the alignment
   * {{{ */
  can_be_placed_at = sizeof (Elf32_Ehdr) + nphdrs * sizeof (Elf32_Phdr);

  for (tel = 0; tel < nsecs; tel++)
  {
    /* PT_LOAD: all SHF_ALLOC sections - the .tbss sections need a PT_TLS
     * segment */
    if ((secs[tel].hdr.sh_flags & SHF_ALLOC) &&
        (!(((secs[tel].hdr.sh_type == SHT_NOBITS) &&
            (secs[tel].hdr.sh_flags & SHF_TLS)))))
    {
      if (secs[tel].prepadding == -1)
      {
        if ((hinfo->ptload_align != 0)
            && ((can_be_placed_at & (hinfo->ptload_align - 1)) <
                (secs[tel].hdr.sh_addr & (hinfo->ptload_align - 1))))
        {
          secs[tel].prepadding =
            (secs[tel].hdr.sh_addr & (hinfo->ptload_align - 1)) -
            (can_be_placed_at & (hinfo->ptload_align - 1));
#ifdef VERBOSE_WRITE
          VERBOSE(0, ("Segment prepadding =%d", secs[tel].prepadding));
#endif
        }
        else if ((hinfo->ptload_align != 0)
                 && ((can_be_placed_at & (hinfo->ptload_align - 1)) >
                     (secs[tel].hdr.sh_addr & (hinfo->ptload_align - 1))))
        {
          secs[tel].prepadding =
            hinfo->ptload_align + (secs[tel].hdr.sh_addr & (hinfo->ptload_align - 1)) -
            (can_be_placed_at & (hinfo->ptload_align - 1));
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

        /* Overlay handling {{{ */
        if (SECTION_FLAGS(secs[tel].sec) & SECTION_FLAG_IN_OVERLAY)
        {
          /* write the file_off field in the .ovtab structure */
          t_object *linker = ObjectGetLinkerSubObject(obj);
          t_section *ovtab = SectionGetFromObjectByName(linker, ".ovtab");
          t_section *parent = SECTION_PARENT_SECTION(ovtab);

          t_uint32 ovtab_offset =
            AddressExtractUint32(AddressSub(SECTION_CADDRESS(ovtab),
                                            SECTION_CADDRESS(parent)));
          t_overlay *ovl; t_overlay_sec *ovlsec;
          int index = 0;
          for (ovl = OBJECT_OVERLAYS(obj); ovl; ovl = ovl->next)
            for (ovlsec = ovl->sec; ovlsec; ovlsec = ovlsec->next)
            {

              if (ovlsec->section == secs[tel].sec)
                goto found_overlay_descriptor;
              ++index;
            }
found_overlay_descriptor:
          ASSERT(ovl && ovlsec, ("Could not find the overlay descriptor for @T", secs[tel].sec));
          {
            t_uint32 *foff =
              (t_uint32 *) (((char *)SECTION_DATA(parent)) +
                                     ovtab_offset + 16*index + 8);
            *foff = can_be_placed_at;
          }
        }
        /*}}}*/

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

        if (diabloobject_options.keep_exidx)
          {
            if (strcmp(secstrtab + secs[tel].hdr.sh_name, ".ARM.exidx") == 0)
              {
                phdrs[0].p_offset = can_be_placed_at;
                secs[tel].hdr.sh_type = SHT_ARM_EXIDX;
                secs[tel].hdr.sh_flags |= SHF_LINK_ORDER;
                /* TODO: find this link stuff in a better way  */
                int tel2;
                for (tel2 = 0; tel2 < nsecs; tel2++)
                  {
                    if (strcmp(secstrtab + secs[tel2].hdr.sh_name, ".text") == 0)
                      secs[tel].hdr.sh_link = tel2+1;
                  }
              }
          }

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
    /* The non-alloc sections (debug info, .comment, ...)  */
    else
    {
        secs[tel].hdr.sh_offset = can_be_placed_at;
        can_be_placed_at += secs[tel].hdr.sh_size;
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


  if (OBJECT_RELRO_CSIZE(obj))
  {
    t_uint32 offset = 0;
    t_uint32 lastload; 
    for (lastload=0; lastload<nphdrs; lastload++)
      if (phdrs[lastload].p_type==PT_LOAD && phdrs[lastload].p_vaddr==phdrs[nphdrs-1].p_vaddr)
        phdrs[nphdrs-1].p_offset = phdrs[lastload].p_offset;
  }


  /* include the PHDR in the first PT_LOAD segment, since it
   * must be mapped
   */
  if (phdr_pdr!=-1)
  {
    int firstload;
    Elf32_Off delta;

    for (firstload=0; firstload<nphdrs; firstload++)
      if (phdrs[firstload].p_type==PT_LOAD)
        break;
    delta = phdrs[firstload].p_offset - phdrs[phdr_pdr].p_offset;
    phdrs[firstload].p_offset -= delta;
    phdrs[firstload].p_paddr -= delta;
    phdrs[firstload].p_vaddr -= delta;
    phdrs[firstload].p_memsz += delta;
    phdrs[firstload].p_filesz += delta;
    /* and also map the PHDR into the PT_LOAD address space */
    phdrs[phdr_pdr].p_paddr = phdrs[firstload].p_paddr;
    phdrs[phdr_pdr].p_vaddr = phdrs[firstload].p_vaddr;
  }
  /* }}} */

  hdr->e_shoff = can_be_placed_at;

  can_be_placed_at = can_be_placed_at + (nsecs + 2) * sizeof (Elf32_Shdr);

#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    can_be_placed_at += sizeof (Elf32_Shdr) * 2;
  }
#endif

  sectabshdr.sh_offset = can_be_placed_at;

  /* Set ELF file header {{{ */
  /* Ident, Machine, Version and Entry are already set */
  hdr->e_phoff = sizeof (Elf32_Ehdr);
  hdr->e_ehsize = sizeof(Elf32_Ehdr);
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
  hdr->e_phentsize = sizeof (Elf32_Phdr);
  hdr->e_phnum = nphdrs;
  hdr->e_shentsize = sizeof (Elf32_Shdr);
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

    symtabshdr.sh_offset = (symtabshdr.sh_offset + 0xf) & ~0xf;

    can_be_placed_at = symtabshdr.sh_offset + symtabshdr.sh_size;

    symstrtabshdr.sh_offset = can_be_placed_at;
  }
#endif
  /* }}} */
  /* Write ELF header + Program headers {{{ */

  if (switch_endian)
    Elf32HdrSwitchEndian(hdr);

  fwrite (hdr, 1, sizeof (Elf32_Ehdr), fp);

  if (switch_endian)
    Elf32HdrSwitchEndian(hdr);

  for (tel = 0; tel < nphdrs; tel++)
  {
    if (switch_endian)
      Elf32PhdrSwitchEndian(&phdrs[tel]);
    fwrite (&phdrs[tel], sizeof (Elf32_Phdr), 1, fp);
    if (switch_endian)
      Elf32PhdrSwitchEndian(&phdrs[tel]);
  }
  /* }}} */
  /* Write section data {{{ */
  for (tel = 0; tel < nsecs; tel++)
  {
    /* skip .tbss */
    if (!((secs[tel].hdr.sh_type == SHT_NOBITS) &&
          (secs[tel].hdr.sh_flags & SHF_TLS)))
    {
      char *padd = Calloc(secs[tel].prepadding,1);
      fwrite (padd, sizeof (char), secs[tel].prepadding, fp);
      Free(padd);
      ASSERT (secs[tel].hdr.sh_offset == ftell (fp),
              ("Section %d (%s) not at the right place (should be %x, is %x)",
               tel, SECTION_NAME(secs[tel].sec), secs[tel].hdr.sh_offset, ftell (fp)));
      if (secs[tel].hdr.sh_type != SHT_NOBITS)
        fwrite (SECTION_DATA (secs[tel].sec), sizeof (char),
                G_T_UINT32 (SECTION_CSIZE (secs[tel].sec)), fp);
    }
  }
  /* }}} */
  /* Write section headers {{{ */

  /* Verify correctness: section table should be at hdr->e_shoff */
  ASSERT (ftell (fp) == hdr->e_shoff,
          ("Section header table is not at %x as said in the program header, but at %x",
           hdr->e_shoff, ftell (fp)));

  /* 1 - zero header */
  if (switch_endian) Elf32ShdrSwitchEndian (&nullshdr);
  fwrite (&nullshdr, sizeof (Elf32_Shdr), 1, fp);
  if (switch_endian) Elf32ShdrSwitchEndian (&nullshdr);
  /* 2 - normal section headers */
  for (tel = 0; tel < nsecs; tel++)
  {
    if (switch_endian) Elf32ShdrSwitchEndian (&secs[tel].hdr);
    fwrite (&secs[tel].hdr, sizeof (Elf32_Shdr), 1, fp);
    if (switch_endian) Elf32ShdrSwitchEndian (&secs[tel].hdr);
  }
  /* 3 - section name string table header */
  if (switch_endian) Elf32ShdrSwitchEndian (&sectabshdr);
  fwrite (&sectabshdr, sizeof (Elf32_Shdr), 1, fp);
  if (switch_endian) Elf32ShdrSwitchEndian (&sectabshdr);

#ifdef SYMTAB
  if (diabloobject_options.symbols)
  {
    if (switch_endian) Elf32ShdrSwitchEndian (&symtabshdr);
    fwrite (&symtabshdr, sizeof (Elf32_Shdr), 1, fp);
    if (switch_endian) Elf32ShdrSwitchEndian (&symtabshdr);

    if (switch_endian) Elf32ShdrSwitchEndian (&symstrtabshdr);
    fwrite (&symstrtabshdr, sizeof (Elf32_Shdr), 1, fp);
    if (switch_endian) Elf32ShdrSwitchEndian (&symstrtabshdr);
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
    Elf32_Sym e_sym;
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

    fwrite(&e_sym,sizeof(Elf32_Sym),1,fp);

    if (OBJECT_SYMBOL_TABLE(obj))
      for (sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(obj)); sym != NULL; sym = SYMBOL_NEXT(sym))
      {
        symtabshdr.sh_size+=sizeof(Elf32_Sym);
        /* section symbols must not have a name */
        if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_SECTION)
          e_sym.st_name=0;
        else
        {
          e_sym.st_name = runsymstrtab;
          sprintf(symstrtab + runsymstrtab, "%s", SYMBOL_NAME(sym));
          runsymstrtab += strlen(SYMBOL_NAME(sym)) + 1;
        }

        if (SYMBOL_BASE(sym) == T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)))
        {
          e_sym.st_info=ELF32_ST_INFO(STB_WEAK, 0);
          e_sym.st_shndx=SHN_UNDEF;
          e_sym.st_value=0;
        }
        else
        {
          t_uint32 symtyp = STT_NOTYPE;
          t_address symaddr = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
          
          if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FUNCTION)
            symtyp = STT_FUNC;
          else if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_SECTION)
            symtyp = STT_SECTION;
          else if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARKER)
            symtyp = STT_OBJECT;
          else if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FUNCTION_SMALLCODE)
          {
            symtyp = STT_FUNC;
            symaddr = AddressOr(symaddr,AddressNew32(1));
          }
          else if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_TLS)
          {
            t_section *tlssec;

            if (OBJECT_NTLSDATAS (obj)>=1)
              tlssec = OBJECT_TLSDATA (obj)[0];
            else if (OBJECT_NTLSBSSS (obj)>=1)
              tlssec = OBJECT_TLSBSS (obj)[0];
            else
              FATAL(("TLS section was not found!"));

            symtyp = STT_TLS;
            symaddr = AddressSub(symaddr,SECTION_CADDRESS(tlssec));
          }

          if (SYMBOL_ORDER(sym)<0)
            e_sym.st_info=ELF32_ST_INFO(STB_LOCAL, symtyp);
          else
            e_sym.st_info=ELF32_ST_INFO(STB_GLOBAL, symtyp);

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
          e_sym.st_value=G_T_UINT32(symaddr);
        }
        fwrite(&e_sym,sizeof(Elf32_Sym),1,fp);
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

/* }}} */

/* Parse the GNU symbol version info */
/* {{{ */
static t_symbol *
SymbolTableGetFirstDefinedSymbolByName(t_symbol_table *st, t_string name)
{
  t_symbol *sym;

  sym = SymbolTableGetFirstSymbolWithName(st,name);
  while (sym &&
         SYMBOL_SEARCH(sym))
  {
    sym = SYMBOL_NEXT(sym);
  }
  return sym;
}


static t_symbol *
SymbolGetNextDefinedSymbol(t_symbol *sym)
{
  sym = SYMBOL_NEXT(sym);
  while (sym &&
         SYMBOL_SEARCH(sym))
  {
    sym = SYMBOL_NEXT(sym);
  }
  return sym;
}


static t_symbol *
ElfGnuVersionAddSym(t_object *obj, t_string symname_to_free, t_section *base, void *data, t_address size)
{
  t_symbol *sym;

  sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), symname_to_free, "R00$", 10, FALSE, FALSE,
      T_RELOCATABLE(base), AddressNew32((data - SECTION_DATA(base))),
      AddressNullForObject(obj), NULL, size, 0);
  Free(symname_to_free);
  return sym;
}


static void
ElfGnuVersionAddEntryLabels(t_object *obj, t_string basename, t_string filename, t_section *basesec, void *entrydata, size_t entrysize, int entryidx, t_bool has_next_entry, t_bool has_aux)
{
  t_string entryname;

  if (filename)
    entryname=StringIo("%sDATA:ENTRY:%d:%s",basename,entryidx,filename);
  else
    entryname=StringIo("%sDATA:ENTRY:%d",basename,entryidx);
  /* symbol for the start of this entry */
  ElfGnuVersionAddSym(obj,entryname,basesec,entrydata,AddressNew32(entrysize));
  /* symbol for the "next" entry of the previous entry */
  if (entryidx != 0)
  {
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:ENTRYNEXT:%d",basename,entryidx-1),basesec,entrydata,AddressNew32(entrysize));
  }
  /* no next entry -> start of "next" entry is also this entry */
  if (!has_next_entry)
  {
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:ENTRYNEXT:%d",basename,entryidx),basesec,entrydata,AddressNew32(0));
  }

  /* no aux entries -> ensure vd_aux/vn_aux will be 0 */
  if (!has_aux)
  {
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:AUX_FOR_ENTRY:%d",basename,entryidx),basesec,entrydata,AddressNew32(entrysize));
  }
}


static void
ElfGnuVersionAddAuxLabels(t_object *obj, t_string basename, t_string versionname, t_section *basesec, void *auxdata, size_t auxsize, int entryidx, int auxidx, int globalauxidx, t_bool has_next_aux)
{
  if (auxidx == 0)
  {
    /* reference to aux from verdef/verneed entry */
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:AUX_FOR_ENTRY:%d",basename,entryidx),basesec,auxdata,AddressNew32(0));
  }
  /* start of this aux */
  ElfGnuVersionAddSym(obj, StringIo("%sDATA:AUX:%d:%s",basename,globalauxidx,versionname),basesec,auxdata,AddressNew32(auxsize));
  /* symbol for the "next" aux of the previous aux */
  if (auxidx != 0)
  {
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:AUXNEXT:%d",basename,globalauxidx-1),basesec,auxdata,AddressNew32(auxsize));
  }
  /* no next aux -> set next symbol also here so that the difference will be 0  */
  if (!has_next_aux)
  {
    ElfGnuVersionAddSym(obj, StringIo("%sDATA:AUXNEXT:%d",basename,globalauxidx),basesec,auxdata,AddressNew32(auxsize));
  }
}


static void
ElfGnuVersionAddRelocs(t_object *obj, char *usestrtab, t_section *usestrtabsec, t_section *versymsec, t_section *verdefsec, t_section *verneedsec)
{
  t_symbol *dynsym;

  /* the versym section contains one Elf32_Half entry per dynamic symbol, indexed
   * the same way as the dynamic symbol table -> walk over all dynamic symbols
   * we added, and create a corresponding symbol in the versym section (which will
   * be used by the linker script to (re)construct the versym section
   */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),dynsym)
  {
    t_string versymname, dynsymname;
    t_address dynsymidx;
    t_symbol *versymsym, *dynsymsym;

    dynsymname = StringConcat2("ANYDYNSYMSYM:",SYMBOL_NAME(dynsym));
    dynsymsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),dynsymname);
    ASSERT(dynsymsym,("Cannot find ANYDYNSYMSYM:%s",dynsymname));
    Free(dynsymname);

    versymname = StringConcat2("VERSYM:",SYMBOL_NAME(dynsym));
    dynsymidx = AddressDivUint32(AddressSub(StackExec(SYMBOL_CODE(dynsymsym),NULL,dynsymsym,NULL,FALSE,0,obj),RELOCATABLE_CADDRESS(SYMBOL_BASE(dynsymsym))),16);
    versymsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), versymname, "R00$", 10, FALSE, FALSE,
        T_RELOCATABLE(versymsec), AddressMulUint32(dynsymidx,sizeof(Elf32_Half)),
        AddressNullForObject(obj), NULL, AddressNew32(2), SYMBOL_TYPE_NOTYPE);
    Free(versymname);
  }

  if (verdefsec)
  {
    void *verdef = SECTION_DATA(verdefsec);
    Elf32_Verdef *entry;
    Elf32_Verdaux *aux;
    Elf32_Word auxidx;
    int globalauxidx;
    int verdefidx;

    /* for now we only support version strings in the dynsymstr table */
    ASSERT(strcmp(SECTION_NAME(usestrtabsec),".dynstr")==0,("Add support for non-dynstr section containing the verdef strings: %s",SECTION_NAME(usestrtabsec)));

    /* we will create symbols that enable us to reconstruct the original
     * verdef section in the linker script:
     *
     *  - the start of every entry is marked with VERDEF:DATA:ENTRY:NEXT:<idx-1> and with
     *    VERDEF:DATA:ENTRY:<idx>. The last entry is also marked with the NEXT label for
     *    itself. The vd_next field will be calculated by subtracting the start and the
     *    NEXT label for the same idx (so for the last entry it will be 0, as intended)
     *  - for aux entries, we follow the same logic, and add extra information regarding
     *    which dynstr entries have to be referred
     */
    globalauxidx = 0;
    verdefidx = 0;
    VERDEF_FOREACH_ENTRY(verdef,entry)
    {
      ElfGnuVersionAddEntryLabels(obj,"VERDEF",NULL,verdefsec,entry,sizeof(*entry),verdefidx,entry->vd_next!=0,entry->vd_aux!=0);

      VERDEF_ENTRY_FOREACH_AUX(entry,aux,auxidx)
      {
        t_symbol *sym;
        t_string versionname;
        t_string tentative;

        globalauxidx++;
        versionname = usestrtab + aux->vda_name;
        ElfGnuVersionAddAuxLabels(obj,"VERDEF",versionname,verdefsec,aux,sizeof(*aux),verdefidx,auxidx,globalauxidx,aux->vda_next!=0);
      }
      verdefidx++;
    }
  }

  if (verneedsec)
  {
    void *verneed = SECTION_DATA(verneedsec);
    Elf32_Verneed *entry;
    Elf32_Vernaux *aux;
    Elf32_Word auxidx;
    int globalauxidx;
    int verneedidx;

    /* for now we only support version strings in the dynsymstr table */
    ASSERT(strcmp(SECTION_NAME(usestrtabsec),".dynstr")==0,("Add support for non-dynstr section containing the verneed strings: %s",SECTION_NAME(usestrtabsec)));

    /* we will create symbols that enable us to reconstruct the original
     * verneed section in the linker script:
     *
     *  - the start of every entry is marked with VERNEED:DATA:ENTRY:NEXT:<idx-1> and with
     *    VERNEED:DATA:ENTRY:<idx>. The last entry is also marked with the NEXT label for
     *    itself. The vd_next field will be calculated by subtracting the start and the
     *    NEXT label for the same idx (so for the last entry it will be 0, as intended)
     *  - for aux entries, we follow the same logic, and add extra information regarding
     *    which dynstr entries have to be referred
     */
    globalauxidx = 0;
    verneedidx = 0;
    VERNEED_FOREACH_ENTRY(verneed,entry)
    {
      char *filename;

      filename = usestrtab + entry->vn_file;
      ElfGnuVersionAddEntryLabels(obj,"VERNEED",filename,verneedsec,entry,sizeof(*entry),verneedidx,entry->vn_next!=0,entry->vn_aux!=0);

      VERNEED_ENTRY_FOREACH_AUX(entry,aux,auxidx)
      {
        t_symbol *sym;
        t_string versionname;
        t_string tentative;

        globalauxidx++;
        versionname = usestrtab + aux->vna_name;
        ElfGnuVersionAddAuxLabels(obj,"VERNEED",versionname,verneedsec,aux,sizeof(*aux),verneedidx,auxidx,globalauxidx,aux->vna_next!=0);
      }
      verneedidx++;
    }
  }
}

static void
ElfProcessGnuVersionInformation32(t_object *obj, Elf32_Sym *dynamic_symbol_table, int ndynsym, char *usestrtab, t_section *usestrtabsec, t_section *versymsec, t_section *verdefsec, t_section *verneedsec)
{
  Elf32_Half *versym = NULL;
  void *verdef = NULL;
  void *verneed = NULL;
  int versymidx;

  versym = (Elf32_Half *)SECTION_DATA(versymsec);
  SECTION_SET_FLAGS(versymsec,SECTION_FLAGS(versymsec) & SECTION_FLAG_KEEP);

  if (verdefsec)
  {
    verdef = SECTION_DATA(verdefsec);
    SECTION_SET_FLAGS(verdefsec,SECTION_FLAGS(verdefsec) & SECTION_FLAG_KEEP);
  }

  if (verneedsec)
  {
    verneed = SECTION_DATA(verneedsec);
    SECTION_SET_FLAGS(verneedsec,SECTION_FLAGS(verneedsec) & SECTION_FLAG_KEEP);
  }

  ElfGnuVersionAddRelocs(obj, usestrtab, usestrtabsec, versymsec, verdefsec, verneedsec);

  for (versymidx = 0; versymidx<ndynsym; versymidx++)
  {
    Elf32_Sym *elfsym;

    /* the versym array contains one entry for every dynamic symbol */
    elfsym = &dynamic_symbol_table[versymidx];
    if (versym[versymidx] >= VER_NDX_LORESERVE)
    {
      WARNING(("Unsupported reserved versioning directive %d",versym[versymidx]));
    }
    else if (versym[versymidx] == VER_NDX_GLOBAL)
    {
      /* default, nothing to do */
    }
    else
    {
      t_const_string symname;
      t_string dynname, generic_dynname;
      t_symbol *sym;
      t_symbol *dynsym, *generic_dynsym;

      /* dynsym names */
      symname = GetTranslatedSymbolName(usestrtab+elfsym->st_name);
      /* skip empty symbol at start */
      if (*symname == 0)
        continue;
      if (ELF32_ST_BIND (dynamic_symbol_table[versymidx].st_info) != STB_WEAK)
        dynname=StringConcat2("DYNSYMSYM:", symname);
      else
        dynname=StringConcat2("WEAKDYNSYMSYM:", symname);
      generic_dynname=StringConcat2("ANYDYNSYMSYM:", symname);

      /* the dynsyms themselves */
      VERBOSE(5,("looking for dynsym %s",symname));
      sym = SymbolTableLookup(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),symname);
      ASSERT(sym!=NULL,("Could not find dynamic symbol %s",symname));
      dynsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),dynname);
      ASSERT(dynsym!=NULL,("Could not find dynamic meta symbol %s",dynname));
      generic_dynsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),generic_dynname);
      ASSERT(generic_dynsym!=NULL,("Could not find dynamic meta symbol %s",generic_dynname));
      /* "If the high order bit (bit number 15) of the version symbol
       * is set, the object cannot be used and the static linker shall
       * ignore the symbol's presence in the object."
       */
      if (versym[versymidx] & 0x8000)
      {
        /* not sure what to do here */
        continue;
      }
      switch (versym[versymidx])
      {
        case VER_NDX_LOCAL:
          {
            /* local symbol -> change order of all related symbols to local */
            VERBOSE(5,("Making dynamic symbol %s local",symname));
            SYMBOL_SET_ORDER(sym,-1);
            SYMBOL_SET_ORDER(dynsym,-1);
            SYMBOL_SET_ORDER(generic_dynsym,-1);
            break;
          }
        default:
        {
          /* index in the verdef/verneed entries with versioning information */
          if (verneed &&
              (elfsym->st_shndx == SHN_UNDEF))
          {
            /* symbol reference -> look up entry in verneed */
            /* not yet important, we just have to replicate this, except that we
             * may have to update the strtab offsets when writing out the binary
             */
          }
          else
          {
            if (verdef) {
              /* symbol definition -> look up entry in verdef */
              Elf32_Verdef *entry;
              Elf32_Verdaux *aux;
              Elf32_Word auxidx;
              VERBOSE(5,("Looking for verdef entry %d for symbol %s",versym[versymidx],symname));
              VERDEF_FOREACH_ENTRY(verdef,entry)
              {
                if (entry->vd_ndx == versym[versymidx])
                  break;
              }

              if (entry && entry->vd_ndx==versym[versymidx]) {
                /* no VER_DEF_NUM documentation found, not sure what it means */
                ASSERT(entry->vd_version == VER_DEF_CURRENT,("Unsupported verdef version %d for dynamic symbol %s",entry->vd_version,symname));
                ASSERT((entry->vd_cnt!=0) && (entry->vd_aux!=0),("No aux entries for entry %d in verdef for dynamyc symbol %s",versym[versymidx],symname));
                /* process all aux entries for this verdef entry. They contain
                * the versioning information about this symbol
                *
                * "The first entry (pointed
                *  to by the Elfxx_Verdef entry), contains the actual
                *  defined name. The second and all later entries name
                *  predecessor versions."
                *
                *  We are only interested in what we have to map the symbol
                *  onto -> just process the first entry.
                */
                VERDEF_ENTRY_FOREACH_AUX(entry,aux,auxidx)
                {
                  t_string versionname, aliasname, defaultvername;
                  t_symbol *aliassym, *orgsym;
                  /* check in which object file this symbol is defined;
                  * if there are multiple versioned symbols and this is
                  * the default one, it will be declared with @@, otherwise
                  * with @ -> check in that order
                  */
                  versionname = usestrtab+aux->vda_name;
                  aliasname = StringConcat3(symname,"@@",versionname);
                  defaultvername = StringConcat2(symname,"@@DIABLO_DEFAULT_VERSION");
                  VERBOSE(5,("Adding symbol %s as alias for %s",defaultvername,aliasname));
                  AddTranslatedSymbolName(defaultvername,aliasname);
                  Free(defaultvername);
                  Free(aliasname);

                  /* stop after first entry */
                  break;
                }
              }
            }
          }
        }
      }
      Free(dynname);
      Free(generic_dynname);
    }
  }
}
/*}}}*/

/*!
 * This function reads the common information in a 32 bit ELF file and adapts the corresponding
 * t_object structure. Structures that need further parsing are returned.
 *
 * \param fp A file pointer pointing to the data of an objectfile after the header
 * \param hdr the header
 * \param obj the t_object structure to fill
 * \param shdr_ret A pointer to return the section headers in
 * \param symbol_table_ret  A pointer to return the symbol table in
 * \param numsyms_ret A pointer to return the number of syms in
 * \param strtab_ret A pointer to return the stringtable in
 * \param sechdrstrtab_ret A pointer to return the section header stringtable in
 * \param lookup A lookup table for the symbols
 * \param switch_endian If TRUE, switch the endianess (not implemented)
 *
 * \return void
 */
/* ElfReadCommon32 {{{ */
void
ElfReadCommon32 (FILE * fp, void *hdr, t_object * obj, Elf32_Shdr ** shdr_ret,
                 Elf32_Sym ** symbol_table_ret, Elf32_Sym ** dynamic_symbol_table_ret, t_uint32 * numsyms_ret,
                 char **strtab_ret, char **sechdrstrtab_ret,
                 t_section *** sec_ret, t_symbol *** lookup, t_symbol *** dynamic_lookup,
                 t_bool switch_endian, t_bool read_debug, Elf32_HeaderInfo * hinfo)
{
  t_bool dynlib;

  char *dynstrtab = NULL;	/* Will hold the dynamic string table */

  Elf32_Dyn *dynamic_table = NULL;	/* Will hold the dynamic table */

  t_section * dynamic_symbol_table_section = NULL;

  t_uint32 numdynsym = 0;

  t_section *versymsec = NULL;
  t_section *verdefsec = NULL;
  t_section *verneedsec = NULL;
  Elf32_Word verstrtab = 0;

  t_address tmp_addr, tmp_sz, tmp_align;	/* Temporaries used in conversion from uint32 -> generic_address */

  Elf32_Ehdr *hdr32 = (Elf32_Ehdr *) hdr;	/* The Objectfileheader (casted correctly, it's passed as a union) */
  t_uint32 section_tel = 0;	/* Counter for the sections */
  t_uint32 segment_tel = 0;     /* Counter for the segments (program headers) */
  t_uint32 dynamic_tel = 0;	/* Counter for the dynamic table entries */
  t_uint32 dynamic_entries;     /* total number of dynamic table entries */

  if (!obj)
    FATAL (("Object = NULL"));
  OBJECT_SET_ADDRESS_SIZE (obj, ADDRSIZE32);

  dynlib = hdr32->e_type == ET_DYN;

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
  ASSERT (hdr32->e_version == EV_CURRENT,
          ("Elf version differs from EV_CURRENT.\nThis could mean that a new ELF specification is created.\nMore likely this means your file is corrupt/truncated or not an ELF objectfile.!"));

  /* Store the entry point in the object structure  */
  OBJECT_SET_ENTRY (obj, AddressNew32 (hdr32->e_entry));

  OBJECT_SET_SYMBOL_TABLE (obj, SymbolTableNew (obj));

  if (hdr32->e_phnum) /* If there are segments (program headers) */
  {
    /* currently we will not do to much, just look for GNU_STACK and RELRO segment ... */
    Elf32_Phdr *  phdrs = Calloc (sizeof (Elf32_Phdr), hdr32->e_phnum);
    fseek (fp, OBJECT_STREAMPOS (obj) + hdr32->e_phoff, SEEK_SET);
    IGNORE_RESULT(fread (phdrs, sizeof (Elf32_Phdr), hdr32->e_phnum, fp));

    if (switch_endian)
    {
      t_uint32 tel = 0;

      for (tel = 0; tel < hdr32->e_phnum; tel++)
        Elf32PhdrSwitchEndian (&(phdrs)[tel]);
    }

    OBJECT_SET_GNU_STACK_FLAGS(obj,0);
    OBJECT_SET_RELRO_CADDRESS(obj,0);
    OBJECT_SET_RELRO_CSIZE(obj,0);
    OBJECT_SET_GOT_IN_RELRO(obj,FALSE);
    while (segment_tel < hdr32->e_phnum)
    {
      if (phdrs[segment_tel].p_type == PT_GNU_STACK)
        OBJECT_SET_GNU_STACK_FLAGS(obj,phdrs[segment_tel].p_flags);
      if (phdrs[segment_tel].p_type == PT_RELRO)
        {
#if 1         
          ASSERT(phdrs[segment_tel].p_paddr==phdrs[segment_tel].p_vaddr,("OOPS: CANNOT HANDLE RELRO SEGMENT OF WHICH VADDR AND PADDR DIFFER!")); 
          OBJECT_SET_RELRO_CADDRESS(obj,phdrs[segment_tel].p_paddr);
          ASSERT(phdrs[segment_tel].p_memsz==phdrs[segment_tel].p_filesz,("OOPS: CANNOT HANDLE RELRO SEGMENT OF WHICH FILESIZE AND MEMSIZE DIFFER!")); 
          OBJECT_SET_RELRO_CSIZE(obj,phdrs[segment_tel].p_memsz);
#endif
        }
      if (phdrs[segment_tel].p_type == PT_TLS)
        {
          /* copy over alignment */
          hinfo->pttls_align = phdrs[segment_tel].p_align;
        }
      segment_tel++;
    }
    Free(phdrs);
  }

  if (hdr32->e_shnum)		/* If there are sections */
  {
    /* STEP 1: Read the headers */
    /* Allocated room to store the headers */
    (*shdr_ret) =
      (Elf32_Shdr *) Malloc (sizeof (Elf32_Shdr) * hdr32->e_shnum);
    (*sec_ret) =
      (t_section **) Calloc (sizeof (t_section *), hdr32->e_shnum);
    /* Look for the headers in the file
     * streampos is used for when we are directly reading from an archive */
    fseek (fp, OBJECT_STREAMPOS (obj) + hdr32->e_shoff, SEEK_SET);
    /* Read the headers */
    IGNORE_RESULT(fread ((*shdr_ret), sizeof (Elf32_Shdr), hdr32->e_shnum, fp));
    /* If we are doing a switched endian read, endian-swap all headers */

    if (switch_endian)
    {
      t_uint32 tel = 0;

      for (tel = 0; tel < hdr32->e_shnum; tel++)
        Elf32ShdrSwitchEndian (&(*shdr_ret)[tel]);
    }

    /* STEP 2: Find the sectionheader string table */
    (*sechdrstrtab_ret) =
      (char *) Malloc ((*shdr_ret)[hdr32->e_shstrndx].sh_size);
    fseek (fp,
           OBJECT_STREAMPOS (obj) +
           (*shdr_ret)[hdr32->e_shstrndx].sh_offset, SEEK_SET);
    IGNORE_RESULT(fread ((*sechdrstrtab_ret), (*shdr_ret)[hdr32->e_shstrndx].sh_size, 1,
                         fp));

    /* STEP 3: find the symboltable and the (symbol) stringtable and the dynamic
     * stringtable (we handle them before the others, because we'll need them later on)*/
    while (section_tel < hdr32->e_shnum)
    {
      if ((*shdr_ret)[section_tel].sh_type == SHT_SYMTAB)
      {
        /* A normal symbol table.... current ABI says we should only have one symbol table */
        ASSERT (!(*symbol_table_ret),
                ("More than one symbol_table in %s",
                 OBJECT_NAME (obj)));
        ASSERT (!
                ((*shdr_ret)[section_tel].sh_size % sizeof (Elf32_Sym)),
                ("Symtab size is not a multiple of %d in %s",
                 sizeof (Elf32_Sym), OBJECT_NAME (obj)));
        (*numsyms_ret) =
          (*shdr_ret)[section_tel].sh_size / sizeof (Elf32_Sym);
        (*symbol_table_ret) = Malloc ((*shdr_ret)[section_tel].sh_size);
        fseek (fp,
               OBJECT_STREAMPOS (obj) +
               (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
        IGNORE_RESULT(fread ((*symbol_table_ret), (*shdr_ret)[section_tel].sh_size, 1, fp));

        if (switch_endian)
        {
          t_uint32 tel = 0;

          for (tel = 0; tel < (*shdr_ret)[section_tel].sh_size / sizeof (Elf32_Sym); tel++)
          {
            Elf32SymSwitchEndian(&((Elf32_Sym*)(*symbol_table_ret))[tel]);
          }
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
        ASSERT (!((*shdr_ret)[section_tel].sh_size % sizeof(Elf32_Sym)),
                ("Symtab size is not a multiple of %d in %s",
                 sizeof(Elf32_Sym), OBJECT_NAME (obj)));

        numdynsym=(*shdr_ret)[section_tel].sh_size/sizeof(Elf32_Sym);
        (*dynamic_symbol_table_ret)=Malloc((*shdr_ret)[section_tel].sh_size);
        fseek (fp, OBJECT_STREAMPOS (obj) + (*shdr_ret)[section_tel].sh_offset, SEEK_SET);

        IGNORE_RESULT(fread((*dynamic_symbol_table_ret),(*shdr_ret)[section_tel].sh_size,1,fp));

        if (switch_endian)
        {
          t_uint32 tel = 0;

          for (tel = 0; tel < (*shdr_ret)[section_tel].sh_size / sizeof (Elf32_Sym); tel++)
          {
            Elf32SymSwitchEndian(&((Elf32_Sym*)(*dynamic_symbol_table_ret))[tel]);
          }
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
        IGNORE_RESULT(fread ((*strtab_ret), (*shdr_ret)[section_tel].sh_size, 1, fp));
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
        IGNORE_RESULT(fread ((dynstrtab), (*shdr_ret)[section_tel].sh_size, 1, fp));
      }
      section_tel++;
    }

    /* STEP 4: find the loaded sections */
    section_tel = 0;

    while (section_tel < hdr32->e_shnum)
    {
      VERBOSE (1,
               (" A SECTION named %s (size=%d - type =%d)",
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
          if (OBJECT_RELRO_CSIZE(obj))
            if (!StringCmp(((*sechdrstrtab_ret) +(*shdr_ret)[section_tel].sh_name),".got"))
              {
                if ((*shdr_ret)[section_tel].sh_addr>= OBJECT_RELRO_CADDRESS(obj) && 
                    (*shdr_ret)[section_tel].sh_addr < OBJECT_RELRO_CADDRESS(obj) +  OBJECT_RELRO_CSIZE(obj))
                  OBJECT_SET_GOT_IN_RELRO(obj,TRUE);
              }

          /* This section holds information defined by the program, whose format
           * and meaning are determined solely by the program. */
          VERBOSE (2,
                   ("Found program specific information. Looking at the flags to determine it's data"));
          if (((*shdr_ret)[section_tel].sh_flags & SHF_ALLOC))	/* This section will be allocated at run-time */
          {
            if (((*shdr_ret)[section_tel].sh_flags & SHF_EXECINSTR)
                && StringCmp(((*sechdrstrtab_ret) +(*shdr_ret)[section_tel].sh_name),".got")
                && StringCmp(((*sechdrstrtab_ret) +(*shdr_ret)[section_tel].sh_name),".plt"))	/* This section contains code */
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
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].
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

                tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
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
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].
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
                SECTION_SET_FLAGS((*sec_ret)[section_tel],SECTION_FLAGS((*sec_ret)[section_tel]) | SECTION_FLAG_KEEP);
              }
              else if (((*shdr_ret)[section_tel].sh_flags & SHF_WRITE))	/* Writable section */
              {
                VERBOSE (2,
                         ("   It's a writable data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].
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
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].
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

            if (diabloobject_options.keep_exidx)
              if (StringPatternMatch(".ARM.extab*", (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
                SECTION_SET_FLAGS((*sec_ret)[section_tel],SECTION_FLAGS((*sec_ret)[section_tel]) | SECTION_FLAG_KEEP);
            
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
            if((StringPatternMatch(".debug*",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
               ||(StringPatternMatch(".stab",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name)))
            {
              if (read_debug)
              {
                tmp_addr =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
                (*sec_ret)[section_tel] =
                  ObjectAddSectionFromFile (obj, DEBUG_SECTION,
                                            TRUE, fp,
                                            OBJECT_STREAMPOS (obj) + (*shdr_ret)[section_tel].sh_offset,
                                            tmp_addr, tmp_sz, tmp_align,
                                            (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,
                                            section_tel);
              }
            }
            else if(StringPatternMatch(".comment*",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
            {
              /* OK to skip comment.... */
            }
            else if (StringPatternMatch(".diablo*",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name))
            {
              VERBOSE(1,("Found compressed subobject information in %s",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
              OBJECT_SET_COMPRESSED_SUBOBJECTS(obj, Malloc((*shdr_ret)[section_tel].sh_size));
              fseek (fp, OBJECT_STREAMPOS (obj) + (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
              IGNORE_RESULT(fread (OBJECT_COMPRESSED_SUBOBJECTS(obj), 1, (*shdr_ret)[section_tel].sh_size, fp));
              VERBOSE(1,("SIZE = %x\n", (*shdr_ret)[section_tel].sh_size));
              VERBOSE(1,("OFFSET = %x\n", (*shdr_ret)[section_tel].sh_offset));
            }
            else
            {
              VERBOSE(2,("Preserving unknown section %s as note section",(*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
              tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
              tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
              tmp_align =
                AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
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
		AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
	      tmp_sz =
		AddressNew32 ((*shdr_ret)[section_tel].sh_size);
	      tmp_align =
		AddressNew32 ((*shdr_ret)[section_tel].
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
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

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
		AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
	      tmp_sz =
		AddressNew32 ((*shdr_ret)[section_tel].sh_size);
	      tmp_align =
		AddressNew32 ((*shdr_ret)[section_tel].
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
          dynamic_entries = (*shdr_ret)[section_tel].sh_size/sizeof(dynamic_table[0]);
          fseek (fp,
                 OBJECT_STREAMPOS (obj) +
                 (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
          IGNORE_RESULT(fread (dynamic_table, 1, (*shdr_ret)[section_tel].sh_size, fp));

          tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
	  tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
	  tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

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

          while ((dynamic_tel<dynamic_entries) &&
                 (dynamic_table[dynamic_tel].d_tag != DT_NULL))	/* An entry with a DT_NULL tag marks the end of the _DYNAMIC array. */
          {
            switch (dynamic_table[dynamic_tel].d_tag)
            {
              /* Holds a string table offset to a NULL-determined string, given the name of a needed libraryi.
               * The offset points in the dynamic table to a DT_STRTAB entry*/
              case DT_NEEDED:
              {
                t_string tmp = StringConcat2("$dt_needed:",dynstrtab + dynamic_table[dynamic_tel].d_un.d_val);
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), tmp, "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                Free(tmp);
                VERBOSE (1,
                         ("Need something dynamic: %s",
                          dynstrtab +
                          dynamic_table[dynamic_tel].d_un.d_val));
                break;
              }
                /* Total size in bytes of the plt. Must be present if we have a DT_JMPREL entry */
              case DT_PLTRELSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltrelsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("PLTSIZE = %d\n",
                           dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* An address associated with the PLT or GOT */
              case DT_PLTGOT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltgot", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("PLTGOT at %x\n",
                           dynamic_table[dynamic_tel].d_un.d_ptr));
                break;
                /* The address of the (dynamic) Symbol hash table */
              case DT_HASH:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_hash", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("HASH at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_ptr));
                break;
                /* The address of the (dynamic) string table */
              case DT_STRTAB:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_strtab", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("STRTAB at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_ptr));
                break;
                /* The address of the (dynamic) symbol table */
              case DT_SYMTAB:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_symtab", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("SYMTAB at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_ptr));
                break;
                /* The size in bytes of the (dynamic) string table */
              case DT_STRSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_strsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("STRSIZE = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size in bytes of a (dynamic) symbol table entry */
              case DT_SYMENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_syment", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("SYMENTSIZE = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The address of a relocation table  with explicit addends. The dynamic linker sees only one table! */
              case DT_RELA:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_rela", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("RELA TABLE at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size of the the relocation table  with explicit addends. */
              case DT_RELASZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relasize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("RELASZ = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size of an element in the relocation table  with explicit addends. */
              case DT_RELAENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relaent", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("RELAENT = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The address of the init section */
              case DT_INIT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_init", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("INIT at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The address of the fini section */
              case DT_FINI:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_fini", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("FINI at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              /* offset of the soname in the string table */
              case DT_SONAME:
              {
                t_string tmp = StringConcat2("$dt_soname:",dynstrtab + dynamic_table[dynamic_tel].d_un.d_val);
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), tmp, "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                Free(tmp);
                VERBOSE(1,("SONAME: %s\n",
                        dynstrtab +
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              }
                /* The address of a relocation table  with implicit addends. The dynamic linker sees only one table! */
              case DT_REL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_rel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("REL TABLE at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_ptr));
                break;
                /* The size of the reloc table */
              case DT_RELSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relsize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("RELSIZE = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size of an element in the relocation table */
              case DT_RELENT:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_relent", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("RELENT = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The type of relocation used in the PLT (rel or rela) */
              case DT_PLTREL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_pltrel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("PLTREL =%d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* Used for debugging. Not specified by the ABI. Can be ignored?. */
              case DT_DEBUG:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_debug", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("DEBUG\n"));
                break;
              case DT_JMPREL:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_jmprel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("JMPREL\n"));
                break;
              case DT_BIND_NOW:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_bind_now", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("BIND_NOW\n"));
                break;
              case DT_RPATH:
              {
                t_string rpathstring = StringConcat2("$dt_rpath:",dynstrtab + dynamic_table[dynamic_tel].d_un.d_val);
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), rpathstring , "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                Free(rpathstring);
                VERBOSE (1,
                         ("RPATH: %s",
                          dynstrtab +
                          dynamic_table[dynamic_tel].d_un.d_val));
                break;
              }
                /* Array of finalization functions executed by the dynamic linker */
              case DT_INIT_ARRAY:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_initarray", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("INIT_ARRAY at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size in bytes of the INIT_ARRAY */
              case DT_INIT_ARRAYSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_initarraysize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("INIT_ARRAYSZ = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* Array of preinitialization functions executed by the dynamic linker */
              case DT_PREINIT_ARRAY:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_preinitarray", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("PREINIT_ARRAY at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size in bytes of the INIT_ARRAY */
              case DT_PREINIT_ARRAYSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_preinitarraysize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("PREINIT_ARRAYSZ = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* Array of initialization functions executed by the dynamic linker */
              case DT_FINI_ARRAY:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_finiarray", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("FINI_ARRAY at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* The size in bytes of the FINI_ARRAY */
              case DT_FINI_ARRAYSZ:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_finiarraysize", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("FINI_ARRAYSZ = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* Flag values specific to the object being loaded (informative, no?) */
              case DT_FLAGS:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_flags", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("FLAGS = %d\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
                /* State Flags */
              case DT_FLAGS_1:
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_flags_1", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("FLAGS_1 = %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_VERSYM:
                /* address of .gnu.version section */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_versym", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("VERSYM at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_VERDEF:
                /* address of .gnu.version_d section */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_verdef", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("VERDEF at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_VERDEFNUM:
                /* number of entries in the .gnu.version_d section */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_verdefnum", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("VERDEFNUM at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_VERNEED:
                /* address of .gnu.version_r section */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_verneed", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("VERNEED at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_VERNEEDNUM:
                /* number of entries in the .gnu.version_r section */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_verneednum", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("VERNEEDNUM at %x\n",
                        dynamic_table[dynamic_tel].d_un.d_val));
                break;
              case DT_SYMBOLIC:
                /* Entry to show this is a symbolically bound binary */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_symbolic", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("SYMBOLIC\n"));
                break;
              case DT_TEXTREL:
                /* Entry to show that one or more relocation entries might request modifications to a non-writable segment */
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_textrel", "R00$", 10, FALSE, FALSE,   T_RELOCATABLE((*sec_ret)[section_tel]), AddressNew32(sizeof(Elf32_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                VERBOSE(1,("TEXTREL\n"));
                break;
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
                else if (dynamic_table[dynamic_tel].d_tag == DT_RUNPATH)
                {
                  WARNING (("Ignoring Runpath dynamic!"));
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
          break;
        case SHT_NOTE:
          /* TODO */
          /* The section holds information that marks the file in some way */
          VERBOSE (2, ("Found a section marking the binary"));
          if (((*shdr_ret)[section_tel].sh_flags & SHF_ALLOC))	/* Must have alloc flags */
          {
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align =
              AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
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
            /* ".plt" on ppc32 is nobits and (obviously) executable
             */
            if (((*shdr_ret)[section_tel].sh_flags & SHF_EXECINSTR) &&
                ((hinfo->pltshtype!=SHT_NOBITS)||
                 (StringCmp((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,".plt") != 0)))
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
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
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
                SECTION_SET_FLAGS((*sec_ret)[section_tel],SECTION_FLAGS((*sec_ret)[section_tel]) | SECTION_FLAG_KEEP);
              }
              else if (StringCmp((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name,".plt") == 0)
              {
                VERBOSE (2,("   It's a .plt section, treating as data")) ;

                tmp_addr =
                AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                AddressNew32 ((*shdr_ret)[section_tel].
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
              else
              {
                VERBOSE (2,
                         ("   It's a writable data section named %s",
                          (*sechdrstrtab_ret) +
                          (*shdr_ret)[section_tel].sh_name));
                tmp_addr =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
                tmp_sz =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_size);
                tmp_align =
                  AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);
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
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

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
	  tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
	  tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
	  tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

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
        case SHT_MIPS_REGINFO:
          {
            Elf_RegInfo reginfo;
            t_uint32 stored = ftell (fp);
            fseek (fp,
                   OBJECT_STREAMPOS (obj) +
                   (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
            IGNORE_RESULT(fread ((&reginfo), (*shdr_ret)[section_tel].sh_size, 1, fp));
            fseek (fp, stored, SEEK_SET);
            OBJECT_SET_GP (obj, AddressNew32 (reginfo.ri_gp_value));
            VERBOSE (2,
                     ("An SHT_MIPS_REGINFO section named %s (size=%d) orig gp = %d",
                      (*sechdrstrtab_ret) +
                      (*shdr_ret)[section_tel].sh_name,
                      (*shdr_ret)[section_tel].sh_size,
                      reginfo.ri_gp_value));
          }
          break;
	case SHT_GROUP:
          /* handled in Step 4a */
	  break;
        case SHT_GNU_verdef:
          /* gnu symbol version definition information */
          if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".gnu.version_d") == 0)
          {
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

            VERBOSE (2,
                ("Found GNU version definition info in section %s",
                 (*sechdrstrtab_ret) +
                 (*shdr_ret)[section_tel].sh_name));

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
            if (verstrtab == 0)
              verstrtab = (*shdr_ret)[section_tel].sh_link;
            else
              ASSERT(verstrtab == (*shdr_ret)[section_tel].sh_link,("different verdef/verneed strtabs?"));
              verdefsec = (*sec_ret)[section_tel];
          }
          break;
        case SHT_GNU_verneed:
          /* gnu symbol version requirements information */
          if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".gnu.version_r") == 0)
          {
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

            VERBOSE (2,
                ("Found GNU version requirements info in section %s",
                 (*sechdrstrtab_ret) +
                 (*shdr_ret)[section_tel].sh_name));

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
          if (verstrtab == 0)
            verstrtab = (*shdr_ret)[section_tel].sh_link;
          else
            ASSERT(verstrtab == (*shdr_ret)[section_tel].sh_link,("different verdef/verneed strtabs?"));
          verneedsec = (*sec_ret)[section_tel];
          break;
        case SHT_GNU_versym:
          /* gnu version information */
          if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name, ".gnu.version") == 0)
          {
            tmp_addr = AddressNew32 ((*shdr_ret)[section_tel].sh_addr);
            tmp_sz = AddressNew32 ((*shdr_ret)[section_tel].sh_size);
            tmp_align = AddressNew32 ((*shdr_ret)[section_tel].sh_addralign);

            VERBOSE (2,
                ("Found GNU version symbol info in section %s",
                 (*sechdrstrtab_ret) +
                 (*shdr_ret)[section_tel].sh_name));

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
          versymsec = (*sec_ret)[section_tel];
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
              VERBOSE (1, ("Ignoring processor specific section type %x, name = %s", ((*shdr_ret)[section_tel].sh_type), (*sechdrstrtab_ret) + (*shdr_ret)[section_tel].sh_name));
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


    /* Step 4a. Process the section groups */
    section_tel = 0;
    while (section_tel < hdr32->e_shnum)
    {
      switch ((*shdr_ret)[section_tel].sh_type)
      {
        case SHT_GROUP:
          /* First 32 word contains the flags, then come the section numbers of the sections
           * part of this group.
           *
           *
           */
        {
          t_uint32 stored = ftell (fp);
          Elf32_Word *entries = Malloc((*shdr_ret)[section_tel].sh_size);
          char *symtabname;
          char *groupsigsymtab_str;
          Elf32_Sym *groupsigsymtab;
          t_section_group *section_group;
          int i;

          fseek (fp,
                 OBJECT_STREAMPOS (obj) +
                 (*shdr_ret)[section_tel].sh_offset, SEEK_SET);
          IGNORE_RESULT(fread (entries, (*shdr_ret)[section_tel].sh_size, 1, fp));
          fseek (fp, stored, SEEK_SET);

          /* section group flags are in the first word; only defined/supported
           * flag until now is GRP_COMDAT: if there are multiple groups with
           * the same identifier, only one is retained
           */
          if (entries[0] != GRP_COMDAT)
            FATAL(("Non-COMDAT section group: %x",entries[0]));
          /* get the group signature */
          ASSERT((*shdr_ret)[section_tel].sh_link < hdr32->e_shnum, ("Invalid section number for group signature symtab: %d",(*shdr_ret)[section_tel].sh_link));
          symtabname = (*sechdrstrtab_ret) + (*shdr_ret)[(*shdr_ret)[section_tel].sh_link].sh_name;
          if (!strcmp(symtabname,".symtab"))
          {
            groupsigsymtab_str = *strtab_ret;
            groupsigsymtab = *symbol_table_ret;
          }
          else if (!strcmp(symtabname,".dynstr"))
          {
            groupsigsymtab_str = dynstrtab;
            groupsigsymtab = *dynamic_symbol_table_ret;
          }
          else
            FATAL(("group signature in unknown symtab (section nr %d)",(*shdr_ret)[section_tel].sh_info));
          section_group = SectionGroupNew(groupsigsymtab_str+groupsigsymtab[(*shdr_ret)[section_tel].sh_info].st_name);
          VERBOSE(5,("Parsing section group %s containing sections",section_group->signature));
          /* now add all sections to the section group list of the allocated section
           * (if all allocated sections are removed by Diablo, it will also remove the
           *  non-allocated ones)
           */
          for (i = 1; i < (*shdr_ret)[section_tel].sh_size / sizeof(Elf32_Word); i++)
          {
            if ((*sec_ret)[entries[i]])
            {
              SectionGroupAddSection(section_group,(*sec_ret)[entries[i]],((*shdr_ret)[entries[i]].sh_flags & SHF_ALLOC) != 0);
              if (entries[0] & GRP_COMDAT)
                SECTION_SET_IS_COMMON_DEFINED ((*sec_ret)[entries[i]], TRUE);
              VERBOSE(5,("  group item: @T",(*sec_ret)[entries[i]]));
            }
          }
          Free(entries);
          ObjectAddComdatSectionGroup(obj,section_group);
        }
        break;
      }
      section_tel++;
    }
  }

  if (hdr32->e_shnum)
  {
    ElfProcessSymbolTable32(obj, symbol_table_ret, strtab_ret, sec_ret, numsyms_ret, lookup, NULL);
  }

  if (numdynsym)
  {
    ElfProcessSymbolTable32(obj, dynamic_symbol_table_ret, &dynstrtab, sec_ret, &numdynsym, dynamic_lookup, dynamic_symbol_table_section);
    /* process symbol versioning information */
    if ((versymsec != NULL) &&
        ((verdefsec != NULL) ||
         (verneedsec != NULL)))
    {
      char *usestrtab;
      if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[verstrtab].sh_name, ".strtab") == 0)
        usestrtab = *strtab_ret;
      else if (strcmp ((*sechdrstrtab_ret) + (*shdr_ret)[verstrtab].sh_name, ".dynstr") == 0)
        usestrtab = dynstrtab;
      else
        FATAL(("verdef/verneed sections don't use strtab nor dynstr sections, but section nr %d",verstrtab));
      ElfProcessGnuVersionInformation32(obj,*dynamic_symbol_table_ret, numdynsym, usestrtab, (*sec_ret)[verstrtab], versymsec, verdefsec, verneedsec);
    }
  }

  if (dynstrtab) Free(dynstrtab);
  if (dynamic_table) Free(dynamic_table);
}


t_symbol *
ElfAddCommonSymbol(t_object *obj, t_symbol_table *symbol_table, t_const_string name, t_address size, t_uint32 alignment, t_uint32 flags, t_tristate search)
{
  t_address null = AddressNullForObject (obj);
  /* Common symbols have a somewhat lower priority than global symbols */
  t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
  t_symbol *tmp;

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

  tmp = SymbolTableAddSymbol (symbol_table,
                              GetTranslatedSymbolName(name),  "R00A00+$", 8, PERHAPS, search,
                              T_RELOCATABLE (undef),
                              null,
                              null,
                              symcode,
                              size, 0);
  Free(symcode);

  SYMBOL_SET_FLAGS(tmp, flags);
  return tmp;
}

/* Read the symbol table and put it in a generic form... */
static void ElfProcessSymbolTable32(t_object * obj, Elf32_Sym ** symbol_table_ret, char **strtab_ret, t_section *** sec_ret, t_uint32 * numsyms_ret, t_symbol *** lookup, t_section * dynsec)
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
    t_string name=NULL, alias=NULL;

    /* ignore the reserved symbol */
    if (symbol_tel == 0)
      continue;

    name = StringDup((*strtab_ret) + (*symbol_table_ret)[symbol_tel].st_name);
    generic = AddressNew32 ((*symbol_table_ret)[symbol_tel].st_value);

    VERBOSE(3,("Symbol %s at @G",name,generic));
    if (ELF32_ST_TYPE((*symbol_table_ret)[symbol_tel].st_info) == STT_SECTION)
    {
      t_string section_name;
      /* section symbols have an empty name -> give them one, so the symbols
       * don't clash in Diablo's symbol table
       */
      Free(name);
      switch ((*symbol_table_ret)[symbol_tel].st_shndx)
      {
        case SHN_COMMON:
          section_name = "COMMON";
          break;
        case SHN_UNDEF:
          section_name = "UNDEFINED";
          break;
        case SHN_ABS:
          section_name = "ABSOLUTE";
          break;
        default:
        {
          t_section *sec = (*sec_ret)[(*symbol_table_ret)[symbol_tel].st_shndx];
          /* can be NULL in case of sections skipped by Diablo */
          if (sec)
            section_name = SECTION_NAME(sec);
          else
            section_name = "<skipped_by_diablo>";
          break;
        }
      }
      name = StringConcat2("SECTIONSYM:",section_name);
    }

    if (dynsec)
    {
      t_string dynname;

      if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) != STB_WEAK)
        dynname=StringConcat2("DYNSYMSYM:", GetTranslatedSymbolName(name));
       else
        dynname=StringConcat2("WEAKDYNSYMSYM:", GetTranslatedSymbolName(name));

      /* We may already have added this symbol name in case of versioned symbols.
       * For now we assume that the first one encountered is the "main" symbol.
       * In the future, we should check the version information here to determine
       * which symbols are hidden and not, generate different symbols for the
       * hidden ones also reconstitute their dynsyms at the end
       */
      if (!SymbolTableLookup (OBJECT_SYMBOL_TABLE(obj), dynname))
      {
        SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), dynname, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(dynsec), AddressNew32(sizeof(Elf32_Sym) * symbol_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
        Free(dynname);
        /* for common linker script rules */
        dynname=StringConcat2("ANYDYNSYMSYM:", GetTranslatedSymbolName(name));
        SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), dynname, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(dynsec), AddressNew32(sizeof(Elf32_Sym) * symbol_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      }
      Free(dynname);
    }

    if (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_NOTYPE)
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
        if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
        {
          order = 10;
          dup = FALSE;
          search = FALSE;
        }
        else if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_LOCAL)
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
    else if ((ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_OBJECT)
             || (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_FILE)
             || (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_SECTION)
             || (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_FUNC)
             || (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_TLS))
    {
      switch (ELF32_ST_TYPE((*symbol_table_ret)[symbol_tel].st_info))
      {
        case STT_FILE:
          flags |= SYMBOL_TYPE_FILE;
          break;
        case STT_OBJECT:
          flags |= SYMBOL_TYPE_OBJECT;
          break;
        case STT_FUNC:
          flags |= SYMBOL_TYPE_FUNCTION;
          break;
        case STT_SECTION:
          flags |= SYMBOL_TYPE_SECTION;
          break;
        default:
          /* do nothing */
          break;
      }
      if (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info) == STT_OBJECT)
      {
        flags |= SYMBOL_TYPE_OBJECT;
      }

      if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
      {
        order = 10;
        dup = FALSE;
        search = FALSE;
      }
      else if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_LOCAL)
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
    else if ((ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info)>=STT_LOPROC)
             && (ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].st_info)<=STT_HIPROC))
    {
      /* Proc specific. Handle these in the architecture backend..... */
      continue;
    }
    else
    {
      FATAL (("Symbol %s has unknown symbol type %d\n",
              (*strtab_ret) +
              (*symbol_table_ret)[symbol_tel].st_name,
              ELF32_ST_TYPE ((*symbol_table_ret)[symbol_tel].
                             st_info)));
    }

    if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_COMMON)
    {
      /* a common symbol's st_value field details the alignment constraints
       * the corresponding data subsection should adhere to. These should be
       * reflected in the symbol's code */
      t_uint32 alignment = (*symbol_table_ret)[symbol_tel].st_value;
      t_address size = AddressNew32((*symbol_table_ret)[symbol_tel].st_size);

      tmp = ElfAddCommonSymbol(obj,symbol_table,GetTranslatedSymbolName(name),size,alignment,flags,search);
      if (lookup)  (*lookup)[symbol_tel] = tmp;
    }
    /* Constants */
    else if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_ABS)
    {
      t_section * abs= OBJECT_PARENT(obj)?OBJECT_ABS_SECTION(OBJECT_PARENT(obj)):OBJECT_ABS_SECTION(obj);


      ASSERT(abs, ("Could not get abs section: object %s %s has no abs section", OBJECT_NAME(obj), OBJECT_PARENT(obj)?"has a parent, but this parent":"has no parent but it also"));

      tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                  GetTranslatedSymbolName(name), alias, "R00A00+$", order, dup, search,
                                  T_RELOCATABLE (abs),
                                  generic,
                                  null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), flags);

      if (lookup)  (*lookup)[symbol_tel] = tmp;
    }
    else if ((*symbol_table_ret)[symbol_tel].st_shndx == SHN_UNDEF)
    {
      t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);

      if (diabloobject_options.gc_sections && ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_LOCAL) {
        WARNING(("Found an undefined local symbol %s, but skipping it because -gc flag is enabled",(*strtab_ret) + (*symbol_table_ret)[symbol_tel].st_name));
        Free(name);
        continue;
      }

      ASSERT ((ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) != STB_LOCAL),("Found an undefined local symbol (might be caused by using --gc-sections with linker which Diablo supports with -gc flag but which is not enabled right now!"));

      if (ELF32_ST_BIND ((*symbol_table_ret)[symbol_tel].st_info) == STB_GLOBAL)
      {
	if (!dynsec)
          tmp = SymbolTableAddAliasedSymbol (symbol_table, GetTranslatedSymbolName(name), alias, "R00A00+$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), null, null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), flags);
        else
          tmp = SymbolTableAddAliasedSymbol (symbol_table, GetTranslatedSymbolName(name), alias, "A00$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), null, null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), flags);

        if (lookup)  (*lookup)[symbol_tel] = tmp;
      }
      else /* WEAK UNDEFINED */
      {
        tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                    GetTranslatedSymbolName(name), alias, "R00A00+$", 4, PERHAPS, FALSE,
                                    T_RELOCATABLE (undef),
                                    null,
                                    null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), flags);
        if (lookup)  (*lookup)[symbol_tel] = tmp;
      }
    }
    else
    {
      t_section *sec = (*sec_ret)[(*symbol_table_ret)[symbol_tel].st_shndx];

      if (!sec)
      {
        Free(name);
        continue;
      }

      tmp = SymbolTableAddAliasedSymbol (symbol_table,
                                  GetTranslatedSymbolName(name), alias, "R00A00+$", order, dup, search,
                                  T_RELOCATABLE (sec),
                                  AddressSub(generic,SECTION_CADDRESS(sec)),
                                  null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), flags);
      if (lookup)  (*lookup)[symbol_tel] = tmp;
    }

    /* add symbol aliases to ensure that versioned symbols resolve to the
     * right place. These only have to be added for defined symbols in
     * subobjects (in the linked object, everything is already resolved,
     * and because we add aliases the undefined symbols will resolve to
     * to the right place)
     */
    if ((*lookup)[symbol_tel]!=NULL && OBJECT_PARENT(obj) &&
        lookup &&
        !SYMBOL_SEARCH((*lookup)[symbol_tel])
        )
    {
      ElfSymbolMaybeAddVersionAlias(obj, symbol_table, (*lookup)[symbol_tel]);
    }

    Free(name);
  }

  /* Adjust the lookup array for the renamed symbols {{{ */
   if (lookup)
   {
     for (symbol_tel = 0; symbol_tel < (*numsyms_ret); symbol_tel++)
     {
       if (!(*lookup)[symbol_tel])
         continue;

       {
         t_string orgname = (*strtab_ret) + (*symbol_table_ret)[symbol_tel].st_name;
         if (orgname &&
             strcmp(orgname,SYMBOL_NAME((*lookup)[symbol_tel])))
         {
           t_symbol *nsym = SymbolTableLookup(symbol_table,orgname);
           if (nsym)
              (*lookup)[symbol_tel] = nsym;
           else if (GetReverseTranslatedSymbolName(orgname))
           {
             /* this symbol will still be replaced with another one later on,
              * so for now make it undefined
              */
             t_section * undef= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
             t_symbol *tmp = SymbolTableAddSymbol (symbol_table, orgname,  "R00A00+$", 0, PERHAPS, TRUE, T_RELOCATABLE (undef), null, null, NULL, AddressNew32((*symbol_table_ret)[symbol_tel].st_size), 0);
             SYMBOL_SET_FLAGS(tmp,SYMBOL_FLAGS((*lookup)[symbol_tel]));
             (*lookup)[symbol_tel] = tmp;
           }
           /* otherwise, also leave the original symbol refering to whatever
            * it originally referred to, like armlink appears to do
            */
         }
       }
     }
   }
   /* }}} */

}

/* Add a symbol from an ELF subobject as a dynamic symbol. This can be a function or data. All the necessary helper symbols will be generated. */
static void ElfAddDynamicSymbol (t_object * obj, t_symbol * sym, t_bool function)
{
  t_object * parent = OBJECT_PARENT(obj);
  t_relocatable * undef = parent ? T_RELOCATABLE(OBJECT_UNDEF_SECTION(parent)) : T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj));
  t_section * sec;
  t_const_string name;
  t_uint32 offset;

  if(OBJECT_DYNAMIC_SYMBOL_TABLE(parent) == NULL)
    FATAL(("Trying to add a dynamic symbol in a statically linked binary!"));

  /* Add symbol to the dynamic symbol table */
  SymbolTableAddAliasedSymbol (OBJECT_DYNAMIC_SYMBOL_TABLE(parent), SYMBOL_NAME(sym), NULL, "A00$", 0, PERHAPS, TRUE, undef,
    AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), function ? SYMBOL_TYPE_FUNCTION : SYMBOL_TYPE_OBJECT);

  /* Use the section size to reflect the number of symbols aded, so we can put symbols at the right offsets */
  sec = SectionGetFromObjectByName(parent, ".dynsym");
  offset = SECTION_CSIZE(sec);
  SECTION_SET_CSIZE(sec, AddressAddUint32(offset, sizeof (Elf32_Sym)));

  name = StringConcat2("DYNSYMSYM:", SYMBOL_NAME(sym));
  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);

  name = StringConcat2("ANYDYNSYMSYM:", SYMBOL_NAME(sym));
  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);

  /* Now start with adding versioning info */
  sec = SectionGetFromObjectByName(parent, ".gnu.version");
  if(sec != NULL)/* Don't do this for android binaries, they don't have this section */
  {
    offset = SECTION_CSIZE(sec);
    SECTION_SET_CSIZE(sec, AddressAddUint32(offset, sizeof (Elf32_Half)));

    name = StringConcat2("VERSYM:", SYMBOL_NAME(sym));
    SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(obj), NULL, AddressNew32(2), SYMBOL_TYPE_NOTYPE);
    Free(name);
  }
}

/* Add symbol sym as a PLT symbol. We will add this as a dynamic symbol and return a newly generated JUMP_SLOT: symbol. */
t_symbol * ElfAddPltSymbol (t_object * obj, t_symbol * sym)
{
  ElfAddDynamicSymbol(obj, sym, TRUE);

  /* Use the section size to reflect the number of symbols added, so we can put symbols at the right offsets */
  t_section * sec = SectionGetFromObjectByName(OBJECT_PARENT(obj), ".rel.plt");
  t_uint32 offset = SECTION_CSIZE(sec);
  SECTION_SET_CSIZE(sec, AddressAddUint32(offset, sizeof (Elf32_Rel)));

  t_const_string name = StringConcat2("JUMP_SLOT:", SYMBOL_NAME(sym));
  sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);

  return sym;
}

/* Add symbol sym as a GLOB_DAT symbol. We will add this as a dynamic symbol and return a newly generated GLOB_DAT: symbol. */
t_symbol * ElfAddGlobDatSymbol (t_object * obj, t_symbol * sym)
{
  ElfAddDynamicSymbol(obj, sym, FALSE);

  /* Use the section size to reflect the number of symbols added, so we can put symbols at the right offsets */
  t_section * sec = SectionGetFromObjectByName(OBJECT_PARENT(obj), ".rel.dyn");
  t_uint32 offset = SECTION_CSIZE(sec);
  SECTION_SET_CSIZE(sec, AddressAddUint32(offset, sizeof (Elf32_Rel)));

  t_const_string name = StringConcat2("GLOB_DAT:", SYMBOL_NAME(sym));
  sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec), offset, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);

  return sym;
}

/* Function to resize the .hash section. It changes size when new dynamic symbols are added. */
extern t_object *parser_object;
void *ElfResizeHashSection(t_ast_successors *succ, void *data)
{
  t_uint32 nbuckets = ElfComputeBucketCount (parser_object, sizeof(Elf32_Sym), 4096, 0);
  t_uint32 size = 4* (3 + nbuckets + SYMBOL_TABLE_NSYMS(OBJECT_DYNAMIC_SYMBOL_TABLE(parser_object)));
  t_section* hash = SectionGetFromObjectByName(parser_object, ".hash");
  t_section* sub = SECTION_SUBSEC_FIRST(hash);

  /* The first time this function is called no subsection exists yet, so we make one and initialize it correctly. */
  if (sub == NULL)
  {
    t_object* linker = ObjectGetLinkerSubObject(parser_object);
    sub = SectionCreateForObject(linker, RODATA_SECTION, hash, SECTION_CSIZE(hash), ".hash");
    SECTION_SET_CADDRESS(sub, SECTION_CADDRESS(hash));
    SECTION_SET_FLAGS(sub, SECTION_FLAGS(sub) | SECTION_FLAG_KEEP);/* Make sure the subsection won't be cleaned up by CfgRemoveDeadCodeAndDataBlocks */
  }

  /* If the size of the .hash section has changed, change the section and subsection size. */
  if (size != SECTION_CSIZE(sub))
  {
    SECTION_SET_CSIZE(sub, size);
    SECTION_SET_DATA(sub, Realloc(SECTION_DATA(sub), size));
    SECTION_SET_CSIZE(hash, size);
    SECTION_SET_DATA(hash, Realloc(SECTION_DATA(hash), size));
  }

  return NULL;
}

void
ElfAddDynamicRelativeRelocation(t_object *obj, t_relocatable *from, t_address from_offset)
{
  /* We need to add DYNREL: subsection, with its name containing an original address.
   * As we're adding these symbols for locations that weren't present in the original binary, the addresses
   * we'll give them have no meaning and so we just start at the maximum and start going down.
   */
  static t_uint32 addr = -1;

  /* Determine the relocation type we'll have to write into the subsection. This differs between architectures */
  t_uint32 relative_reloc_type;
  if (!strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name, "ARM"))
    relative_reloc_type = R_ARM_RELATIVE;
  else if (!strcmp(OBJECT_OBJECT_HANDLER(obj)->sub_name, "i386"))
    relative_reloc_type = R_I386_RELATIVE;
  else FATAL(("Trying to add dynamic relative relocation for unsupported architecture!"));

  /* Create a new subsection for the .rel.dyn parent section. The first 4 bytes must contain address to be
   * fixed up, so we put a relocation there. The last 4 bytes contain the relocation type.
   */
  char name[strlen("DYNREL:") + 9];
  t_section *dynrel_sec;
  sprintf(name, "DYNREL:%x", addr);
  dynrel_sec = SectionCreateForObject (ObjectGetLinkerSubObject(obj), RODATA_SECTION, SectionGetFromObjectByName(obj, ".rel.dyn"), AddressNewForObject(obj, 8), name);
  SECTION_SET_ALIGNMENT(dynrel_sec, 0x4);
  SectionSetData32(dynrel_sec, AddressNewForObject(obj, 4), relative_reloc_type);

  /* Create a new relocation so address where the base has to be added by the linker is calculated */
  RelocTableAddRelocToRelocatable (OBJECT_RELOC_TABLE(obj),
    AddressNullForObject(obj),
    T_RELOCATABLE(dynrel_sec),
    AddressNullForObject(obj),
    from,
    from_offset,
    FALSE,
    NULL,
    NULL,
    NULL,
    "R00A00+\\l*w\\s0000$");

  addr--;
}

#endif
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
