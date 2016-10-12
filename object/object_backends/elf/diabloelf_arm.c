/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

/* TODO: verify pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_arm_gnu_info =
{
  0x1000,         /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x8000,         /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  0x4             /*  Elf32_Word pttls_align; */
};

/* TODO: verify ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_arm_ads_info =
{
  0x0,            /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x4,            /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
};


/* are we rewriting a sysv abi binary or not?
 * (initialised by ElfReadArmSameEndian)
 */
static t_tristate sysv_abi = PERHAPS;

/* abi version in the binary's header */
static int abi_version = 0;

static enum {EXPLICIT_SOFTFP,IMPLICIT_SOFTFP,HARDFP} fpu_conv;

t_bool
IsElfArmSameEndianOnMsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
  return FALSE;
#endif

  if (fread (buffer, sizeof (Elf32_Byte), EI_NIDENT, fp) != EI_NIDENT)
    FATAL (("Could not read elf header!!!!!!\nFile corrupt"));

  fseek (fp, save, SEEK_SET);

  if ((buffer[EI_MAG0] != ELFMAG0) || (buffer[EI_MAG1] != ELFMAG1)
      || (buffer[EI_MAG2] != ELFMAG2) || (buffer[EI_MAG3] != ELFMAG3))
    FATAL (("Not an elf objectfile"));

  switch (buffer[EI_DATA])
    {
    default:
    case ELFDATANONE:
    case ELFDATA2LSB:
      return FALSE;
    case ELFDATA2MSB:
      break;
    }

  switch (buffer[EI_CLASS])
    {
    case ELFCLASS32:
      if (fread (&(hdr), sizeof (Elf32_Ehdr), 1, fp) != 1)
	{
	  FATAL (("Could not read elf header!!!!!!\nFile corrupt"));
	}
      fseek (fp, save, SEEK_SET);
      machine = hdr.e_machine;
      break;
    case ELFCLASS64:
    case ELFCLASSNONE:
    default:
      return FALSE;
      break;
    }

  if (machine != EM_ARM)
    return FALSE;

  return TRUE;
}

t_bool
IsElfArmSameEndianOnLsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_WORDS_BIGENDIAN
  return FALSE;
#endif

  if (fread (buffer, sizeof (Elf32_Byte), EI_NIDENT, fp) != EI_NIDENT)
    FATAL (("Could not read elf header!!!!!!\nFile corrupt"));

  fseek (fp, save, SEEK_SET);

  if ((buffer[EI_MAG0] != ELFMAG0) || (buffer[EI_MAG1] != ELFMAG1)
      || (buffer[EI_MAG2] != ELFMAG2) || (buffer[EI_MAG3] != ELFMAG3))
    FATAL (("Not an elf objectfile"));

  switch (buffer[EI_DATA])
    {
    default:
    case ELFDATANONE:
    case ELFDATA2MSB:
      return FALSE;
    case ELFDATA2LSB:
      break;
    }

  switch (buffer[EI_CLASS])
    {
    case ELFCLASS32:
      if (fread (&(hdr), sizeof (Elf32_Ehdr), 1, fp) != 1)
	{
	  FATAL (("Could not read elf header!!!!!!\nFile corrupt"));
	}
      fseek (fp, save, SEEK_SET);
      machine = hdr.e_machine;
      break;
    case ELFCLASS64:
    case ELFCLASSNONE:
    default:
      return FALSE;
      break;
    }

  if (machine != EM_ARM)
    return FALSE;

  return TRUE;
}


t_uint64
ElfArmLinkBaseAddress(const t_object *obj, const t_layout_script *script)
{
  if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) ||
      (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
    return 0;
  else
    return 0x8000;
}

/* Either the GOT or some other data section follows after the RELRO segment,
   if one exists. 
   If so, the one following the RELRO need to be aligned on a page boundary to
   avoid that it partially ends up in the page being protected against
   writes as part of the RELRO segment 
 */

t_uint64
ElfArmAlignGotAfterRelRO(t_object *obj, long long currpos)
{
  t_bool have_got = FALSE;
  int i;
  t_section * sec;

  if (OBJECT_RELRO_CSIZE(obj)==0)
    return currpos;

  OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      if (StringPatternMatch(".got",SECTION_NAME(sec)))
        have_got = TRUE;
    }

  if (!have_got)
    return currpos;
    
  if (OBJECT_GOT_IN_RELRO(obj))
    return currpos;

  return  (currpos + 0x1000 - 1) & ~(0x1000 - 1); 
}

t_uint64
ElfArmAlignDataAfterRelRO(t_object *obj, long long currpos)
{
  t_bool have_got = FALSE;
  int i;
  t_section * sec;

  if (OBJECT_RELRO_CSIZE(obj)==0)
    return currpos;

  OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      if (StringPatternMatch(".got",SECTION_NAME(sec)))
        have_got = TRUE;
    }

  if (!have_got)
    return (currpos + 0x1000 - 1) & ~(0x1000 - 1); 
    
  if (!OBJECT_GOT_IN_RELRO(obj))
    return currpos;

  return (currpos + 0x1000 - 1) & ~(0x1000 - 1); 
}


/* the start of the RELRO segment needs to be allocated such that
   then end of the RELRO segment comes as close as possible to a page
   boundary, and at that page boundary the first section after the RELRO
   segment will start 
 */

t_uint64
ElfArmAlignStartOfRelRO(t_object *obj, long long currpos)
{
  t_section * sec;
  t_uint64 total_size_relro = 0;
  int i;
  t_uint64 boundary = 0x1000000; /* random big address on page boundary to compute necessary padding */
  t_section * sections[32];
  int nr_sections = 0;

  if (OBJECT_RELRO_CSIZE(obj)==0)
    return currpos;

  /* first collect all sections in the order in which they will appear in the RELRO segment */

  OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      t_string secname = SECTION_NAME(sec);
      if (StringPatternMatch(".eh_frame",secname) || 
          StringPatternMatch(".gcc_except_table",secname) || 
          StringPatternMatch(".tdata",secname) || 
          StringPatternMatch(".tdata.*",secname) || 
          StringPatternMatch(".gnu.linkonce.td.*",secname) || 
          // in glibc, tbss have effective size 0
          //          StringPatternMatch(".tbss",secname) || 
          //          StringPatternMatch(".tbss.*",secname) || 
          StringPatternMatch(".gnu.linkonce.tb.*",secname) || 
          StringPatternMatch(".tcommon",secname) || 
          StringPatternMatch(".preinit_array",secname) || 
          StringPatternMatch(".init_array.*",secname) || 
          StringPatternMatch(".init_array",secname) || 
          StringPatternMatch(".fini_array.*",secname) || 
          StringPatternMatch(".fini_array",secname) || 
          StringPatternMatch(".ctors",secname) || 
          StringPatternMatch(".ctors.*",secname) || 
          StringPatternMatch(".dtors",secname) || 
          StringPatternMatch(".dtors.*",secname) || 
          StringPatternMatch(".jcr",secname) || 
          StringPatternMatch(".dynamic",secname) ||
          StringPatternMatch(".data.rel.ro",secname) || 
          (StringPatternMatch(".got",secname) && OBJECT_GOT_IN_RELRO(obj))
          )
        {
          sections[nr_sections++]=sec;
        }
    }

  /* then compute how much space will be needed, taking into account
     possible padding */

  for (i=1;i<=nr_sections;i++)
    {
      int padding = 0;
      sec = sections[nr_sections-i];

      boundary-= G_T_UINT32(SECTION_CSIZE(sec));
      total_size_relro += G_T_UINT32(SECTION_CSIZE(sec));

      padding = boundary % G_T_UINT32(SECTION_ALIGNMENT(sec));

      boundary-=padding;
      total_size_relro += padding;
    }

  /* position of end of RELRO if no shift is done */
  t_uint64 new_current_pos_after_relro = currpos + total_size_relro;

  /* aligned position of end of RELRO that needs to be achieved */
  t_uint64 new_aligned_pos_after_relro = (new_current_pos_after_relro + 0x1000 - 1) & ~(0x1000 - 1); 

  /* necessary shift */
  t_uint64 shift =  new_aligned_pos_after_relro - new_current_pos_after_relro;

  /* remember the addresses for when the binary is written out */
  OBJECT_SET_RELRO_NEW_ADDRESS(obj,currpos+shift);
  OBJECT_SET_RELRO_NEW_SIZE(obj,total_size_relro);

  /* eventually, shift the position to arrive at the proper location */
  return currpos + shift;
}

void
ElfWriteArmSameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing arm binary"));
  hdr.e_ident[EI_MAG0] = ELFMAG0;
  hdr.e_ident[EI_MAG1] = ELFMAG1;
  hdr.e_ident[EI_MAG2] = ELFMAG2;
  hdr.e_ident[EI_MAG3] = ELFMAG3;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;

#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
  hdr.e_ident[EI_DATA] = ELFDATA2LSB;
#else
  hdr.e_ident[EI_DATA] = ELFDATA2MSB;
#endif
  hdr.e_ident[EI_CLASS] = ELFCLASS32;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
    {
      hdr.e_ident[tel] = 0;
    }
  hdr.e_machine = EM_ARM;
  hdr.e_version = EV_CURRENT;
  VERBOSE(0, ("program entry point = @G", OBJECT_ENTRY (obj)));
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = EF_ARM_HASENTRY | (abi_version << 24);
  if (fpu_conv==HARDFP)
    hdr.e_flags |= EF_ARM_ABI_FLOAT_HARD;
  else if (fpu_conv==EXPLICIT_SOFTFP)
    hdr.e_flags |= EF_ARM_ABI_FLOAT_SOFT;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;		/* Set in common */
  hdr.e_shnum = 0;		/* Set in common */
  hdr.e_shstrndx = 0;		/* Set in common */

  if (!sysv_abi)
    ElfWriteCommon32 (fp, &hdr, obj, FALSE, FALSE, &elf_arm_ads_info);
  else
    ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_arm_gnu_info);
  STATUS (STOP, ("Writing arm binary"));
}


void
MaybeForcePltRedirect(t_object *obj, t_symbol **sym)
{
  /* PLT32 should point to the PLT entry for the symbol and cause
   * a PLT entry to be generated. In practice, the linker optimises
   * these and changes them into direct branches, unless a PLT entry
   * is absolutely required. We can detected this based on the fact
   * that a JUMP_SLOT symbol has been created by us (which can only
   * happen for dynamically linked binaries)
   *
   * On modern ABI versions, the same has to be done for R_ARM_CALL
   * and R_ARM_JUMP24
   */
  if (OBJECT_PARENT(obj))
  {
    t_string jumpslotname;
    t_symbol *jumpslotsym;

    /* If we're linking in a new object after linker emulation and encounter a PLT32 relocation,
     * it will either be to a symbol that is already being used in the binary (in which case there is
     * a JUMP_SLOT in case of a dynamic symbol or just the static symbol present in the SUB_SYMBOL_TABLE)
     * or to a symbol not being used yet. In the second case we will add a new entry to the PLT.
     */
    t_bool after_linker_emulation = FALSE;
    t_symbol_table* table = OBJECT_SYMBOL_TABLE(OBJECT_PARENT(obj));
    if (!table)
    {
      table = OBJECT_SUB_SYMBOL_TABLE(OBJECT_PARENT(obj));
      after_linker_emulation = TRUE;
    }

    /* jump slot symbol? */
    jumpslotname = StringConcat2("JUMP_SLOT:",SYMBOL_NAME(*sym));
    jumpslotsym = SymbolTableGetSymbolByName(table,jumpslotname);
    Free(jumpslotname);
    if (jumpslotsym)
    {
      /* yes -> redirect relocation to symbol that we'll add
       * for the PLT entry
       */
      t_string pltentryname;
      t_relocatable *undef = T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)));

      pltentryname = StringConcat2("PLTELEMSYM:",SYMBOL_NAME(*sym));
      *sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), pltentryname, "R00A00+$",10,PERHAPS,FALSE,
          undef, AddressNew32(0), AddressNew32(0), NULL, 16, SYMBOL_TYPE_NOTYPE);
      Free(pltentryname);
    }
    else
    {
      /* Check whether the symbol is present already in the binary, if not add new PLT symbol */
      if (after_linker_emulation && !SymbolTableGetSymbolByName(table, SYMBOL_NAME(*sym)))
        *sym = ElfAddPltSymbol (obj, *sym);
    }
  }
}

void
ElfReadArmSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf32_Shdr *shdrs = NULL;
  Elf32_Sym *symbol_table = NULL;
  Elf32_Sym *dynamic_symbol_table = NULL;
  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  int tel;
  char *sechdrstrtab = NULL;
  Elf32_Ehdr hdr32;
  t_uint32 abi=0;
  t_symbol **table = NULL;
  t_symbol **dynamic_table = NULL;
  t_section *gotsec = NULL;
  t_bool seen_diablo_gcc = FALSE, seen_diablo_clang = FALSE, seen_diablo_gas = FALSE, seen_compiler = FALSE;

  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  if ((hdr32.e_flags & EF_ARM_ABI_FLOAT_HARD))
    {
      VERBOSE(1,("DETECTED HARDFLOAT BINARY"));
      fpu_conv = HARDFP;
    }
  else if ((hdr32.e_flags & EF_ARM_ABI_FLOAT_SOFT))
    {
      VERBOSE(1,("DETECTED SOFTFP BINARY"));
      fpu_conv = EXPLICIT_SOFTFP;
    }
  else
    {
      VERBOSE(1,("DETECT IMPLICIT SOFTFP BINARY"));
      fpu_conv = IMPLICIT_SOFTFP;
    }

  if ((hdr32.e_flags & 0xFF000000)==0x01000000) abi=1;
  else if ((hdr32.e_flags & 0xFF000000)==0x02000000) abi=2;
  else if ((hdr32.e_flags & 0xFF000000)==0x03000000) abi=3;
  else if ((hdr32.e_flags & 0xFF000000)==0x04000000) abi=4;
  else if ((hdr32.e_flags & 0xFF000000)==0x05000000) abi=5;

  abi_version=abi;

  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
       &sechdrstrtab, &sec_table, &table, &dynamic_table, FALSE, read_debug, &elf_arm_ads_info);

  /* In case the entry address is Thumb code, it will have the lowest bit set.
   * Clear it here so that the generic Diablo code won't be confused. We'll
   * set it again when writing out the final binary. We can detect whether or
   * not the entry code is thumb thanks to the $t symbols. */
  //OBJECT_SET_ENTRY(obj, AddressAnd(OBJECT_ENTRY(obj),AddressNot(AddressNewForObject(obj,1))));

  OBJECT_SET_CODE_SYMBOL_ADDRESS_INVERSE_MASK(obj,1);

  /* on sysv targets, the R_ARM_TARGET1 is absolute, on others it's relative
   * -> detect what we should use. Similarly, R_ARM_TARGET2 is R_ARM_GOT_PREL
   * on sysv and R_ARM_ABS32 on bare metal and Symbian.
   * NOTE: this code assumes that the first read object file is the main object
   */
  if (sysv_abi == PERHAPS)
  {
    ASSERT(!OBJECT_PARENT(obj),("First read object file has a parent, not the final linked binary?"));
    if (SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),"Region$$Table$$Base") == NULL)
      sysv_abi = YES;
    else
      sysv_abi = NO;
    VERBOSE(1,("SYSV abi: %s",sysv_abi?"YES":"NO"));
  }

  if (abi)
  {
    t_symbol * sym;
    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(obj), sym)
    {
      if (SYMBOL_ORDER(sym)>0)
      {
        if (strncmp(SYMBOL_NAME(sym),"$Sub$$",6)==0)
        {
          SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),SYMBOL_NAME(sym)+6,SYMBOL_CODE(sym),20,FALSE,FALSE,SYMBOL_BASE(sym),SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), NULL, SYMBOL_SIZE(sym), 0);
        }
        if (strncmp(SYMBOL_NAME(sym),"$Super$$",8)==0)
          FATAL(("Super symbol"));
      }
    }
  }


  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
  {
    if (!OBJECT_PARENT(obj))
    {
      /* first find the .got section, if it exists */
      for (tel = 0; tel < hdr32.e_shnum; tel++)
      {
        if ((sec_table[tel]!=NULL) &&
            (strcmp(".got",SECTION_NAME(sec_table[tel]))==0))
        {
          gotsec = sec_table[tel];
          break;
        }
      }
    }

    /* now process all sections */
    tel = 0;
    while (tel < hdr32.e_shnum)
    {
      t_string secname = sechdrstrtab + shdrs[tel].sh_name;


      if ((shdrs[tel].sh_type >= SHT_LOPROC) && (shdrs[tel].sh_type <= SHT_HIPROC))
      {
        switch (shdrs[tel].sh_type)
        {
          case SHT_ARM_EXIDX:
            {
              t_section * corrsec = sec_table[shdrs[tel].sh_link];
              t_address tmp_addr, tmp_sz, tmp_align;

              ASSERT(corrsec, ("Corresponding section (%d) for exidx (exception index) section %s not found", shdrs[tel].sh_link, secname));

              VERBOSE(2, ("exception index %s for section %s", secname, SECTION_NAME(corrsec)));

              tmp_addr = AddressNew32 (shdrs[tel].sh_addr);
              tmp_sz = AddressNew32 (shdrs[tel].sh_size);
              tmp_align = AddressNew32 (shdrs[tel].sh_addralign);
              sec_table[tel] = ObjectAddSectionFromFile (obj, DATA_SECTION, TRUE, fp,
                                          OBJECT_STREAMPOS (obj) +
                                          shdrs[tel].sh_offset,
                                          tmp_addr, tmp_sz,
                                          tmp_align,
                                          secname, tel);

              if (diabloobject_options.keep_exidx)
                SECTION_SET_FLAGS(sec_table[tel], SECTION_FLAGS(sec_table[tel]) | SECTION_FLAG_KEEP);

            break;
            }
          case SHT_ARM_ATTRIBUTES:
            if (!OBJECT_PARENT(obj))
            {
              t_address tmp_addr, tmp_sz, tmp_align;
              tmp_addr = AddressNew32 (shdrs[tel].sh_addr);
              tmp_sz = AddressNew32 (shdrs[tel].sh_size);
              tmp_align = AddressNew32 (shdrs[tel].sh_addralign);
              sec_table[tel] = ObjectAddSectionFromFile (obj, ATTRIB_SECTION, TRUE, fp,
                                          OBJECT_STREAMPOS (obj) +
                                          shdrs[tel].sh_offset, 
                                          tmp_addr, tmp_sz,
                                          tmp_align,
                                          secname,
                                          tel);
            }
            break;
          default:
            FATAL(("Unhandled ARM specific section type 0x%x", (shdrs[tel].sh_type)));
        }
      }


      /* STEP 4b: handle architecture specific flags */
      if ((shdrs[tel].sh_flags & SHF_ARM_COMDEF) && (shdrs[tel].sh_flags & SHF_ALLOC))
      {
        t_section *sec = SectionGetFromObjectByName (obj, secname);
        if (!sec)
          FATAL (("Common defined section %s not found!", secname));
        else
          SECTION_SET_IS_COMMON_DEFINED (sec, TRUE);
      }

      /* STEP 5: Find the relocations */
      if (StringPatternMatch (".rel.debug*", secname))
      {
        /* if we ignore debug info, we can also ignore debug relocations */
      }
      else if (StringPatternMatch (".rela*", secname))
      {
        FATAL (("RELA relocations not implemented!"));
      }
      else if (StringPatternMatch (".rel.stab*", secname))
      {
        /* Ignore these */
      }
      else if (StringPatternMatch (".rel*exidx*", secname) && !diabloobject_options.keep_exidx)
        {
          /* Ignore these */
        }
      else if (StringPatternMatch(".rel.dyn", secname) || StringPatternMatch(".rel.plt", secname))
      {
        Elf32_Rel *rel = Malloc (shdrs[tel].sh_size);
        t_uint32 tel2;
        fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
               SEEK_SET);
        IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));

        for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel));
             tel2++)
        {
          switch (ELF32_R_TYPE (rel[tel2].r_info))
          {
            case R_ARM_COPY:
              {
                t_symbol *aliassym;
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                t_string symname;
                t_address symaddr;

                symname = StringConcat2("ARM_COPY:", SYMBOL_NAME(sym));
                VERBOSE(3,("ARM_COPY! @S\n", sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                /* if there are other symbols at this address, they should be
                 * mapped to that same copy
                 */
                symaddr = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
                aliassym = SymbolTableGetFirstSymbolByAddress(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),symaddr);
                while (aliassym)
                {
                  if (!(SYMBOL_FLAGS(aliassym) & SYMBOL_TYPE_SECTION))
                  {
                    /* make sure we store the address of this symbol in the
                     * dynamic symbol table
                     */
                    SymbolTableAddAbsPrefixedSymWithFlagsIfNonExisting(OBJECT_SYMBOL_TABLE(obj),obj,"DYNSYMPLTADDR:",SYMBOL_NAME(aliassym),0);

                    if (aliassym != sym)
                    {
                      t_string mappedsymname;
                      /* create a mapping in the linker script */
                      mappedsymname = StringConcat3(SYMBOL_NAME(aliassym),":",SYMBOL_NAME(sym));
                      SymbolTableAddAbsPrefixedSymWithFlagsIfNonExisting(OBJECT_SYMBOL_TABLE(obj),obj,"DIABLOMAPSYM:",mappedsymname,SYMBOL_FLAGS(aliassym));
                      Free(mappedsymname);
                    }
                  }
                  aliassym = SymbolTableGetNextSymbolByAddress(aliassym,symaddr);
                }
                Free(symname);
                break;
              }
            case R_ARM_JUMP_SLOT:
              {
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                t_string symname;
                VERBOSE(3,("JUMP_SLOT! @S\n", sym));
                /* add symbol for JUMP_SLOT relocation */
                symname = StringConcat2("JUMP_SLOT:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                /* add symbol for GOT entry referred by JUMP_SLOT relocation */
                ASSERT(gotsec,("R_ARM_JUMP_SLOT relocation and no .got section?"));
                symname = StringConcat2("DYNAMIC_GOT_ENTRY:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(gotsec), AddressSub(SectionGetDataAsAddress(sec_table[tel],AddressNew32(tel2 * sizeof (Elf32_Rel)),AddressNew32(32)),SECTION_CADDRESS(gotsec)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                break;
              }
            case R_ARM_RELATIVE:
              {
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                char name[30];
                /* The following is true for SVR4-like systems, but not for all (e.g. symbian)
                 * Update if it fails
                 */
                ASSERT(ELF32_R_SYM (rel[tel2].r_info)==0,("R_ARM_RELATIVE is only supported for symbol 0 currently"));
                /* dynamic relative relocations instruct the dynamic linker to add the load
                 * address of the library to value at a particular address in the library.
                 * The problem is that here we don't know what symbol was originally associated
                 * with that address, and when we encounter the original relocation (an
                 * R_ARM_ABS32 reloc) then we don't know at which address the corresponding
                 * R_ARM_RELATIVE relocation is located in the .dyn.rel section -> we add
                 * a symbol here that indicates that there's a dynamic relocation pointing
                 * to a particular address, and later on we will hook it up with the
                 * actual symbol on that address for which we found a R_ARM_ABS32 reloc
                 */
                snprintf(name,sizeof(name),"DYNRELATIVESOURCE:%x",rel[tel2].r_offset);
                VERBOSE(3,("DYN RELATIVE for data at address 0x%x!",rel[tel2].r_offset));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                break;
              }
            case R_ARM_ABS32:
              {
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                t_string symname = StringConcat2("DYNABS:", SYMBOL_NAME(sym));
                char name[30];
                /* dynamic relative relocations instruct the dynamic linker to add the load
                 * address of the library to value at a particular address in the library.
                 * The problem is that here we don't know what symbol was originally associated
                 * with that address, and when we encounter the original relocation (an
                 * R_ARM_ABS32 reloc) then we don't know at which address the corresponding
                 * R_ARM_RELATIVE relocation is located in the .dyn.rel section -> we add
                 * a symbol here that indicates that there's a dynamic relocation pointing
                 * to a particular address, and later on we will hook it up with the
                 * actual symbol on that address for which we found a R_ARM_ABS32 reloc
                 */
                snprintf(name,sizeof(name),"DYNABSOLUTESOURCE:%x",rel[tel2].r_offset);
                VERBOSE(3,("DYN ABS32 for data at address 0x%x!",rel[tel2].r_offset));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);

                /* If it doesn't exist yet, add a symbol from which we can decide this symbol
                 * is accessed through a dynamic absolute relocation.
                 */
                if (!SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj), symname))
                  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);

                break;
              }
            case R_ARM_TLS_TPOFF32:
            {
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                t_string symname;

                VERBOSE(3,("TLS_TPOFF! @S\n", sym));
                symname = StringConcat2("TLS_TPOFF:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                /* add extra symbol indicating that this symbol's got entry is filled in by
                 * the dynamic linker
                 */
                symname = StringConcat2("DYNGOTRELOC:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                break;
            }
            case R_ARM_GLOB_DAT:
              {
                t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                t_string symname;

                VERBOSE(3,("GLOB_DAT! @S\n", sym));
                symname = StringConcat2("GLOB_DAT:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                /* add extra symbol indicating that this symbol's got entry is filled in by
                 * the dynamic linker
                 */
                symname = StringConcat2("DYNGOTRELOC:", SYMBOL_NAME(sym));
                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                Free(symname);
                break;
              }
            default:
              FATAL(("Dynamic relocation %d not handled!",ELF32_R_TYPE (rel[tel2].r_info)));
              break;
          }
        }
        Free(rel);
      }
      else if (StringPatternMatch (".rel*", secname))
      {
        Elf32_Rel *rel = Malloc (shdrs[tel].sh_size);
        t_uint32 tel2;
        t_section *corrsec;
        fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
               SEEK_SET);
        IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
        corrsec = sec_table[shdrs[tel].sh_info];

        ASSERT(corrsec, ("Corresponding section (section %d) for relocation section %s in %s not found!", shdrs[tel].sh_info, secname, OBJECT_NAME(obj)));
        /* all "addends" used below are not addends, but the to be relocated
         * data.
         */
        ASSERT(!StringPatternMatch(".rela.*", secname),(".rela sections are not yet supported on ARM"));

        for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel)); tel2++)
        {
          t_uint32 reloc_type;

          ASSERT (symbol_table
                  && strtab,
                  ("Symtab or stringtab not found. Implement me"));

          reloc_type = ELF32_R_TYPE (rel[tel2].r_info);
          if (reloc_type == R_ARM_TARGET1)
          {
            if (sysv_abi)
              reloc_type = R_ARM_ABS32;
            else
              reloc_type = R_ARM_REL32;
          }
          if (reloc_type == R_ARM_TARGET2)
          {
            if (sysv_abi)
              reloc_type = R_ARM_GOT_PREL;
            else
              reloc_type = R_ARM_ABS32;
          }

          switch (reloc_type)
          {
            case R_ARM_NONE:
              break;
            case R_ARM_TARGET1:
              FATAL(("TARGET1 can be either absolute or relative, should be changed to the proper form earlier on"));
              break;
            case R_ARM_JUMP24:
            case R_ARM_PLT32:
            case R_ARM_CALL:
            case R_ARM_PC24:
              {
                t_uint32 data;
                t_symbol *sym;
                t_address generic;
                t_address addend;
                t_bool plttarget;

                plttarget = FALSE;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);

                data = Uint32SelectBits (data, 23, 0);
                data = Uint32SignExtend (data, 23);
                data <<= 2;
                addend = AddressNew32 (data);

                if ((reloc_type == R_ARM_PLT32)
                  || ((abi_version != 0) && ((reloc_type == R_ARM_CALL) || (reloc_type == R_ARM_JUMP24))))
                {
                  /* PLT32 should point to the PLT entry for the symbol and cause
                   * a PLT entry to be generated. In practice, the linker optimises
                   * these and changes them into direct branches, unless a PLT entry
                   * is absolutely required. We can detected this based on the fact
                   * that a JUMP_SLOT symbol has been created by us (which can only
                   * happen for dynamically linked binaries)
                   *
                   * On modern ABI versions, the same has to be done for R_ARM_CALL
                   * and R_ARM_JUMP24
                   */
                  if (OBJECT_PARENT(obj))
                  {
                    t_string jumpslotname;
                    t_symbol *jumpslotsym;
                    t_relocatable *undef = T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)));

                    /* If we're linking in a new object after linker emulation and encounter a PLT32 relocation,
                     * we might need to add a new PLT entry.
                     */
                    t_bool after_linker_emulation = FALSE;
                    t_symbol_table* table = OBJECT_SYMBOL_TABLE(OBJECT_PARENT(obj));
                    if (!table)
                    {
                      table = OBJECT_SUB_SYMBOL_TABLE(OBJECT_PARENT(obj));
                      after_linker_emulation = TRUE;
                    }

                    /* jump slot symbol? */
                    jumpslotname = StringConcat2("JUMP_SLOT:",SYMBOL_NAME(sym));
                    jumpslotsym = SymbolTableGetSymbolByName(table,jumpslotname);

                    /* If its after linker emulation, the symbol is undefined and not present in the
                     * original binary either, we need to create a jump slot in the linked in object.
                     */
                    if (after_linker_emulation && !jumpslotsym && (SYMBOL_BASE(sym) == undef) && !SymbolTableGetSymbolByName(table, SYMBOL_NAME(sym)))
                    {
                      jumpslotsym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), jumpslotname);

                      /* If no jump slot has been created yet in the linked in object, create one */
                      if(!jumpslotsym)
                        jumpslotsym = ElfAddPltSymbol (obj, sym);
                    }
                    Free(jumpslotname);

                    if (jumpslotsym)
                    {
                      /* yes -> redirect relocation to symbol that we'll add
                       * for the PLT entry
                       */
                      t_string pltentryname = StringConcat2("PLTELEMSYM:",SYMBOL_NAME(sym));
                      plttarget = TRUE;
                      sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), pltentryname, "R00A00+$",10,PERHAPS,FALSE,
                          undef, AddressNew32(0), AddressNew32(0), NULL, 16, SYMBOL_TYPE_NOTYPE);
                      Free(pltentryname);
                    }
                  }
                }

                if (!plttarget &&
                    ((abi == 0) ||
                     (reloc_type == R_ARM_JUMP24)))
                {
                  t_string potential_veneer_name;

                  /* if the target is not a PLT entry but still a symbol,
                   * we may need a veneer in case of branch (ARM->thumb branches
                   * don't exist); the linker script will redirect this symbol
                   * either to the veneer or to the actual target
                   */
                  if ((SYMBOL_ORDER(sym)>=10) ||
                      (SYMBOL_DUP(sym)==PERHAPS) ||
                      (SYMBOL_SEARCH(sym)))
                  {
                    potential_veneer_name = StringConcat2("DIABLO_POTENTIAL_THUMB_VENEER:",SYMBOL_NAME(sym));
                    sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), potential_veneer_name, "R00A00+$",10,PERHAPS,TRUE,
                        OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
                        AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), SYMBOL_TYPE_FUNCTION);
                    Free(potential_veneer_name);
                  }
                }


                /* In ARM ABI, weak symbol are converted: the reloc is not to null, it is to Ps0004+  */
                if (abi!=0)
                {
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "u00?s0004A00+:S00P-A00+!" "\\" WRITE_PC24);
                }
                else
                {
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "S00P-A00+" "\\" WRITE_PC24);
                }
              }
              break;
            case R_ARM_PREL31:
              {
                /* PREL31 relocations are relaxed, i.e., they are always relative to the
                   section starts. In crt1.o, they are even relative to the file start.
                   Therefore they need to be converted to a kind of non-relaxed form, i.e.,
                   relative to a symbol.
                 */

                /* for the time being, we only undo the relaxation when the user has asked to keep the exidx data */

                t_uint32 data;
                t_symbol *sym;
                t_address generic;
                t_address addend;
                t_symbol * new_sym;
                /* in object files, all sections still start at address 0, so if we want to find function symbols matching a section symbol + addend
                   we need to make sure that we only consider function symbols in the same section. This is done by checking the symbol's base section */
                t_relocatable * base;
                t_relocatable * new_base = NULL;
                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);
                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);
                base = SYMBOL_BASE(sym);

                new_sym = sym;
                if (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FILE)
                  new_base = base;

                if (sym && (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_SECTION|SYMBOL_TYPE_FILE)))
                  {
                    t_address sym_address = RELOCATABLE_CADDRESS(T_RELOCATABLE(SYMBOL_BASE(new_sym)))+SYMBOL_OFFSET_FROM_START(new_sym)+addend;

                    new_sym = SymbolTableGetFirstSymbolByAddress(OBJECT_SYMBOL_TABLE(obj),sym_address);

                    /* checking the base section only makes sense for section symbols, not for the file symbol in crt1.o */
                    if (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FILE))
                      {
                        if (new_sym)
                          {
                            new_base = SYMBOL_BASE(new_sym);
                          }
                        else
                          new_base = NULL;
                      }

                    while (new_sym && !((SYMBOL_FLAGS(new_sym) & SYMBOL_TYPE_FUNCTION) && (new_base == base)))
                      {
                        new_sym = SymbolTableGetNextSymbolByAddress(new_sym,sym_address);

                        /* same as above comment */
                        if (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FILE))
                          {
                            if (new_sym)
                              new_base = SYMBOL_BASE(new_sym);
                            else
                              new_base = NULL;
                          }
                      }
                  }

                if (new_sym && new_sym!=sym && (SYMBOL_FLAGS(new_sym) & SYMBOL_TYPE_FUNCTION))
                  {
                    /* we have found a function symbol to replace the relaxed section or file symbol */
                    sym = new_sym;
                    addend = 0;
                  }

                /* data (addend) contains the sign bit */
                /* if the exidx section is kept, so must the code fragments to which it points */
                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, diabloobject_options.keep_exidx, NULL, NULL, NULL, "S00P-A00+ i7fffffff& A00i80000000&| S00M|" "\\" WRITE_32);
              }
              break;
            case R_ARM_ABS32:
              {
                t_uint32 data;
                t_address generic;
                t_address addend;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                t_reloc *reloc;
                t_bool skip_static_reloc;
                generic = AddressNew32 (rel[tel2].r_offset);
                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);
                
                ASSERT(!SYMBOL_NAME(sym) || strcmp(SYMBOL_NAME(sym),".text") ||(data == 0), ("Absolute relocation into the text section."));

                /* Absolute addresses have to be adjusted by the load
                 * address at run time in case of a dynamic library
                 * afterwards (via a dynamic I386_RELATIVE* relocation,
                 * see the comments for R_I386_RELATIVE for more info).
                 * Here we add a symbol that the code in
                 * AddDynRelativeRelocs() will use to associate this
                 * relocation with the correct R_I386_RELATIVE
                 * relocation.
                 */
                 skip_static_reloc = FALSE;
                if (OBJECT_PARENT(obj) &&
                    ((OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_SHARED_LIBRARY_PIC) ||
                     (OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_EXECUTABLE_PIC)))
                {
                  t_object *parent = OBJECT_PARENT(obj);
                  t_string name = StringConcat2("DYNABSRELOC:",SYMBOL_NAME(sym));
                  t_string dynabsname = StringConcat2("DYNABS:",SYMBOL_NAME(sym));
                  SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                  Free(name);

                  /* We must not statically relocate this value if it will be
                   * handled by a dynamic ABS32 relocation.
                   */
                  if (SymbolTableLookup(OBJECT_SYMBOL_TABLE(parent), dynabsname))
                    skip_static_reloc = TRUE;
                  Free(dynabsname);
                }
                if (!skip_static_reloc)
                {
                  if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
                    reloc = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                        addend,
                        T_RELOCATABLE (corrsec),
                        generic, sym, TRUE, NULL,
                        NULL, NULL,
                        CALC_ABS "S00M|\\"
                        WRITE_32);
                  else
                    reloc = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                        addend,
                        T_RELOCATABLE (corrsec),
                        generic, sym, TRUE, NULL,
                        NULL, NULL,
                        CALC_ABS "S00m|\\"
                        WRITE_32);
                }
                else
                  /* fake relocation to verify that the original linker indeed
                   * didn't relocate this value either
                   */
                  reloc = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                      addend,
                      T_RELOCATABLE (corrsec),
                      generic, sym, TRUE, NULL,
                      NULL, NULL,
                      "S00*A00\\"
                      WRITE_32);
              }
              break;
            case R_ARM_REL32:
              {
                t_uint32 data;
                t_address generic;
                t_address addend;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);

                if (abi!=0)
                {
                  if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "u00?s0000A00+:S00P-A00+!" "S00M|\\" WRITE_32);
                  else
                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "u00?s0000A00+:S00P-A00+!" "S00m|\\" WRITE_32);
                }
                else
                {
                  if (SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION|SYMBOL_TYPE_FUNCTION_SMALLCODE))
                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00P-A00+" "S00M|\\" WRITE_32);
                  else
                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00P-A00+" "S00m|\\" WRITE_32);
                }
                if (OBJECT_PARENT(obj) &&
                    ((OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_SHARED_LIBRARY_PIC) ||
                     (OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_EXECUTABLE_PIC)))
                {
                  t_string name = StringConcat2("DYNABSRELOC:",SYMBOL_NAME(sym));
                  SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                  Free(name);
                }
              }
              break;
            case R_ARM_GOTPC:
              {
                t_uint32 data;
                t_address generic;
                t_address addend;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

                generic = AddressNew32 (rel[tel2].r_offset);
                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);

                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, sym, TRUE, NULL,
                                            NULL, NULL,
                                            "gP-A00+" "\\" WRITE_32
                                           );
              }
              break;
            case R_ARM_GOT32:
            case R_ARM_GOT_PREL:
              {
                t_uint32 data;
                t_address generic;
                t_address addend, loadaddr;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                t_symbol *gotuse, *gotsym;
                t_string name, got_name, relative_source, unique_source_name;
                t_string reloc_code;
                char* tentative;
                /* we have to fill in the address for static/local symbol got entries,
                 * the dynamic linker does it for dynamic/global symbol got entries
                 *
                 * The following tentative strings are parametrised:
                 *  1) %d: SYMBOL_FLAGS of the original symbol
                 *  2) %s: symbol name where to read the value created by the linker for the GOT-based relocation
                 *  3) %s: symbol expression to which the value from 2) is relative to get the address of the GOT entry
                 *  4) @G: value to subtract from 2) + 3) to get the final address of the GOT entry (for addends)
                 *  5) %d: again the SYMBOL_FLAGS of the original symbol
                 */
                static char const * tentative_dynamic =
                  "OrigSymbol {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d) }\
                    symbol { DUPLICATE(STRINGTOKENR(MATCHED_NAME(),\":\",0)) }\
                  }\
                  GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 4) }\
                    section { RELOCATED32(0x0,CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),0,0,0,\"S00A00+S00M|*s0000\\l*w\\s0000$\") }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(\"%s\")) + %s - @G }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d)\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }";
                static char const * tentative_static_no_tls =
                  "OrigSymbol {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d) }\
                    symbol { DUPLICATE(STRINGTOKENR(MATCHED_NAME(),\":\",0)) }\
                  }\
                  GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 4) }\
                    section { RELOCATED32(0x0,CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),0,0,0,\"S00A00+S00M|\\l*w\\s0000$\") }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(\"%s\")) + %s - @G }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d)\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",MATCHED_NAME())) }\
                  }";
                static char const * tentative_static_add_dynabsreloc =
                  "OrigSymbol {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d) }\
                    symbol { DUPLICATE(STRINGTOKENR(MATCHED_NAME(),\":\",0)) }\
                  }\
                  GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 4) }\
                    section { RELOCATED32(0x0,CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),0,0,0,\"S00A00+S00M|\\l*w\\s0000$\") }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(\"%s\")) + %s - @G }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d)\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }\
                  GotDynAbsReloc {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"DYNABSRELOC:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, SYMBOL_TYPE_NOTYPE)\
                    }\
                    symbol { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }";
                static char const * tentative_static_tls =
                  "OrigSymbol {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d) }\
                    symbol { DUPLICATE(STRINGTOKENR(MATCHED_NAME(),\":\",0)) }\
                  }\
                  GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 4) }\
                    section { RELOCATED32(0x0,CONCAT(\"ORIG:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),0,\"$tls_start\",0,\"S00A00+S01-\\l*w\\s0000$\") }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(\"%s\")) + %s - @G }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), 12, %d)\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }";

                if (SYMBOL_ORDER(sym) == -1)
                {
                  char *colon_pos;
                  t_string new_name=StringIo("got_global_for_local_symbol_%s_%d_from_%s",SYMBOL_NAME(sym),ELF32_R_SYM (rel[tel2].r_info),OBJECT_NAME(obj));
                  /* remove colons from the name, as we'll use those as separator below */
                  while ((colon_pos = strchr(new_name,':')))
                    *colon_pos = '_';

                  sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), new_name, SYMBOL_CODE(sym), 10, PERHAPS, FALSE, SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), NULL, SYMBOL_SIZE(sym), 0);
                  Free(new_name);
                }

                /* we cannot just check whether the GOT:* symbol already exists
                 * and if so, assume that a previous relocation will take case
                 * of the creation of the GOT entry, because this previous
                 * relocation may be removed if that part of the object file
                 * was not linked into the final binary
                 */

                generic = AddressNew32 (rel[tel2].r_offset);
                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);

                switch (reloc_type)
                {
                  case R_ARM_GOT32:
                    reloc_code = "S00A00+g-" "\\" WRITE_32;
                    break;
                  case R_ARM_GOT_PREL:
                    reloc_code = "S00A00+P-" "\\" WRITE_32;
                    break;
                  default:
                    FATAL(("add reloctype %d",reloc_type));
                    break;
                }

                /* add a unique symbol for this particular usage of the GOT entry,
                 * as we have to refer to it when calculating the GOT entry's address
                 * -> if there are multiple ones, we can't control which one is used
                 */
                unique_source_name = StringIo("DIABLO_UNIQUE_GOT_USE:%s_offset_@G:%s", OBJECT_NAME(obj), generic, SYMBOL_NAME(sym));

                switch (reloc_type)
                {
                  case R_ARM_GOT32:
                    /* offset of the item in the got */
                    relative_source = "SYMBOL(\"_GLOBAL_OFFSET_TABLE_\")";
                    break;
                  case R_ARM_GOT_PREL:
                    /* offset between the current position and the item in the got */
                    relative_source = StringConcat3("SUBSYMBOL(\"",unique_source_name,"\")");
                    break;
                  default:
                    FATAL(("add reloctype %d",reloc_type));
                    break;
                }

                if (OBJECT_PARENT(obj))
                {
                  t_relocatable *undef = T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)));

                  /* If we're linking in after emulation the symbol table is the sub symbol table, and we might need to add
                   * GLOB_DAT symbols.
                   */
                  t_bool after_linker_emulation = FALSE;
                  t_symbol_table* table = OBJECT_SYMBOL_TABLE(OBJECT_PARENT(obj));
                  if (!table)
                  {
                    table = OBJECT_SUB_SYMBOL_TABLE(OBJECT_PARENT(obj));
                    after_linker_emulation = TRUE;
                  }

                  /* if there is a globdat or tlspoff dynamic relocation for this symbol, its addrss will be calculated by the dynamic
                   * linker and we have to write 0. Otherwise we have to fill in the GOT entry ourselves.
                   */
                  name = StringConcat2("GLOB_DAT:", SYMBOL_NAME(sym));
                  if (SymbolTableGetSymbolByName(table, name))
                    tentative = StringIo(tentative_dynamic, SYMBOL_FLAGS(sym), unique_source_name, relative_source, addend, SYMBOL_FLAGS(sym));
                  /* If we're after linker emulation and dealing with an undefined symbol, assume it has to be a GLOB_DAT symbol */
                  else if (after_linker_emulation && (SYMBOL_BASE(sym) == undef) && !SymbolTableGetSymbolByName(table, SYMBOL_NAME(sym)))
                  {
                    /* If we haven't added this symbol for this subobject yet, add it as a GLOB_DAT symbol */
                    if (!SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), name))
                      ElfAddGlobDatSymbol(obj, sym);

                    tentative = StringIo(tentative_dynamic, SYMBOL_FLAGS(sym), unique_source_name, relative_source, addend, SYMBOL_FLAGS(sym));
                  }
                  else
                  {
                    /* if this is inside a PIC object, the GOT entry needs to be relocated */
                    if ((OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_EXECUTABLE_PIC))
                    {
                      tentative = StringIo(tentative_static_add_dynabsreloc,SYMBOL_FLAGS(sym),unique_source_name,relative_source,addend,SYMBOL_FLAGS(sym));
                    }
                    else
                      tentative = StringIo(tentative_static_add_dynabsreloc,SYMBOL_FLAGS(sym),unique_source_name,relative_source,addend,SYMBOL_FLAGS(sym));
                  }
                  Free(name);
                }
                else
                  tentative = StringIo(tentative_static_add_dynabsreloc,SYMBOL_FLAGS(sym),unique_source_name,relative_source,addend,SYMBOL_FLAGS(sym));

                got_name = StringConcat2("GOT:",SYMBOL_NAME(sym));
                gotsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj),got_name);
                if (!gotsym)
                  gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), got_name ,"R00A00+$",10,PERHAPS,FALSE,
                      OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))
                      ,AddressNew32(0), AddressNew32(0), NULL, SYMBOL_SIZE(sym), 0);
                Free(got_name);

                /* unique symbol that we will use in case of PREL relocation to calculate
                 * the address of the GOT entry
                 */
                gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), unique_source_name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), tentative, AddressNullForObject(obj), 0);
                Free(unique_source_name);

                Free(tentative);
                switch (reloc_type)
                {
                  case R_ARM_GOT_PREL:
                    Free(relative_source);
                    break;
                  default:
                    break;
                }

                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, gotsym, TRUE, NULL,
                                            NULL, NULL,
                                            reloc_code
                                           );
              }
              break;

            case R_ARM_THM_JUMP24:

              {
                t_uint32 data, tmp, sbit, j1, j2;
                t_symbol *sym;
                t_address generic;
                t_address addend;
                t_string possible_stub_name;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);
                /* caveat: 16-bit word order is different! */

                /* extract the S and J-bits from the encoded instruction */
                sbit = (data & 1<<10) ? 1 : 0;
                j1 = (data & 1<<(16+13)) ? 1 : 0;
                j2 = (data & 1<<(16+11)) ? 1 : 0;

                /* calculate the immediate value */
                tmp = Uint32SelectBits (data, 9, 0);
                tmp |= ((j2 ^ sbit) ^ 1) << 10;
                tmp |= ((j1 ^ sbit) ^ 1) << 11;
                tmp |= sbit << 12;
                tmp <<= 11;
                tmp = Uint32SignExtend (tmp, 23);
                data = Uint32SelectBits (data, 16+10, 16+0);
                data |= tmp;
                data <<= 1;
                addend = AddressNew32 (data);

                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, sym, FALSE, NULL,
                                            NULL, NULL,
                                            "u00?s0004P+:S00! u00?s0001:=M!|P-A00+" "\\"
                                            WRITE_THM_PC24);
              }
              break;

            case R_ARM_THM_PC22:
              {
                t_uint32 data, tmp;
                t_symbol *sym;
                t_address generic;
                t_address addend;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);

                tmp = Uint32SelectBits (data, 10, 0);
                tmp <<= 11;
                tmp = Uint32SignExtend (tmp, 21);
                data = Uint32SelectBits (data, 26, 16);
                data |= tmp;
                data <<= 1;
                addend = AddressNew32 (data);

                MaybeForcePltRedirect(obj,&sym);
                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, sym, FALSE, NULL,
                                            NULL, NULL,
                                            "u00?s0004P+:S00! u00?s0001:=M! ?P:Pifffffffc&!-A00+\\" "u00?s0001:S00M!%" WRITE_THM_PC22_EXT);
              }
              break;
            case R_ARM_GOTOFF:
              {
                t_uint32 data;
                t_symbol *sym;
                t_address generic;
                t_address addend;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);

                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, sym, TRUE, NULL,
                                            NULL, NULL,
                                            "S00A00+S00M|g-\\" WRITE_32
                                           );
              }
              break;
            case R_ARM_THM_JUMP11:
              {
                t_uint32 data;
                t_symbol *sym;
                t_address generic;
                t_address addend;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData16 (corrsec, generic);
                addend = AddressNew32 (data);

                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            addend,
                                            T_RELOCATABLE (corrsec),
                                            generic, sym, TRUE, NULL,
                                            NULL, NULL,
                                            /* check part: last bit must be zero and abs(offset)<(1<<11) */
                                            "S00P-s0004-\\=s0001>s07ff&kse800&|v\\ =s0001& % =s001f>~s0001+={^}-s0800-s001f>s0001^ |$"

                                           );
              }
              break;
            case R_ARM_V4BX:
              /* this is a hint to the linker that this instruction can
               * be changed from "bx rm" into "mov pc,rm" in case it is
               * generating code for arm4 rather than arm4t
               */
              break;
            case R_ARM_PC13:
              {
                t_uint32 data;
                t_symbol *sym;
                t_address generic;
                t_address addend;

                sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                generic = AddressNew32 (rel[tel2].r_offset);

                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);

                if (abi!=0)
                {
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "u00?s0000A00+:S00P-s0008-! \\ u00? : == i80000000& ? ~s0001+ s0fff& l iff7ff000& | : s0fff & l ifffff000 & | i00800000 | !! S00M| w \\ =s001f> - s1000+ iffffe000& $" );
                }
                else
                  RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00P-s0008-\\ == i80000000& ? ~s0001+ s0fff& l iff7ff000& | : s0fff & l ifffff000 & | i00800000 | ! S00M| w \\ =s001f> - s1000+ iffffe000& $");
              }
              break;
            case R_ARM_TLS_LDM32:
            case R_ARM_TLS_IE32:
              {
                /* This relocation causes a GOT entry to be created
                 * containing
                 *  a) for R_ARM_TLS_IE32, the offset between the start of the tbss
                 *     (or tdata) section and the symbol
                 *  b) for R_ARM_TLS_LDM32 an entry that is dynamically relocated by
                 *     a R_ARM_TLS_DTPMOD32 relocation, and next an empty (0) one
                 *
                 * At the from-location of the relocation, the offset
                 * between this GOT entry and (the address of the
                 * load using this relocated value PC-rel index + 4/8) is stored
                 * in the final binary.
                 * relations that hold for the various values:
                 *   linkedvalue = GOT_addr + GOT_entry_off - reloc_addr + reloc_addend
                 *   reloc_addend = reloc_addr - load_addr - 4/8
                 * So we can calculate the load's address from the addend and
                 * the GOT offset from the linked value. Then we can
                 * reconstruct the original relocation by using this load's
                 * address rather than the addend:
                 *   linkedvalue = GOT_addr + GOT_entry_off - reloc_addr + (reloc_addr - load_addr - 4/8)
                 *               = GOT_addr + GOT_entry_off - load_addr - 4/8
                 * We can also use this to express the address of the
                 * of the GOT_entry (linkedvalue + load_addr + 4/8).
                 *
                 * NOTE: the 4/8 depends on whether the load (or "add pc") and load_addr
                 * is ARM or thumb code. We cannot determine that here, so we always assume
                 * thumb and then patch up things in case of ARM. The reason is that
                 * if we assume thumb and it's ARM, the load_addr symbol will be placed
                 * right after the ARM instruction. Vice versa, we would place the symbol
                 * 4 bytes before the Thumb symbol, which could be in another section or
                 * even an invalid address.
                 */
                t_uint32 data;
                t_address generic;
                t_address addend;
                t_address loadaddr;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                t_symbol *gotuse, *gotsym, *gotload;
                t_string name;
                /* The s0008+ in the RELOCATED32() below stems from the
                 * fact that the first 8 bytes of the TLS segment is
                 * occupied by a thread control block (see TCB_SIZE in
                 * binutils/bfd/elf32-arm.c -- should actually be aligned
                 * to a multiple of the tls section's alignment)
                 *
                 * The "+ 4" at the end of the "address" line is because of
                 * the "NOTE" in the explanation above
                 */
                t_const_string tentative_ie32 =
                  "GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 4) }\
                    section { RELOCATED32(0x0,STRINGTOKENR(MATCHED_NAME(),\":\",0),0,\"$tls_start\",0,\"u01?s0000:S00A00+S01-s0008+!\\l*w\\s0000$\") }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(MATCHED_NAME())) + SUBSYMBOL(CONCAT(\"DIABLO_UNIQUE_GOT_LOAD:\",SUBSTRING(MATCHED_NAME(),22,0))) + 4 }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),12,SYMBOL_TYPE_NOTYPE )\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }";
                t_const_string tentative_ldm32 =
                  "GotSection {\
                    trigger { ! SUBSECTION_EXISTS(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)))} \
                    action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)), DATA, 4, 8) }\
                    section {\
                              RELOCATED32(0x0,0,0,0,0,\"s0001\\l*w\\s0000$\"),\
                              RELOCATED32(0x0,0,0,0,0,\"s0000\\l*w\\s0000$\")\
                            }\
                    address { READ_LINKED_VALUE32(SUBSYMBOL(MATCHED_NAME())) + SUBSYMBOL(CONCAT(\"DIABLO_UNIQUE_GOT_LOAD:\",SUBSTRING(MATCHED_NAME(),22,0))) + 4 }\
                  }\
                  GotSymbol {\
                    action {\
                      ADD_SYMBOL_NEW(CONCAT(\"GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0)),12,SYMBOL_TYPE_NOTYPE )\
                    }\
                    symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:GOT:\",STRINGTOKENR(MATCHED_NAME(),\":\",0))) }\
                  }";
                t_const_string tentative;
                t_string unique_source_name;

                if (SYMBOL_ORDER(sym) == -1)
                {
                  char *colon_pos;
                  t_string new_name=StringIo("got_global_for_local_symbol_%s_%d_from_%s",SYMBOL_NAME(sym),ELF32_R_SYM (rel[tel2].r_info),OBJECT_NAME(obj));
                  /* remove colons from the name, as we'll use those as separator below */
                  while ((colon_pos = strchr(new_name,':')))
                    *colon_pos = '_';

                  sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), new_name, SYMBOL_CODE(sym), 10, PERHAPS, FALSE, SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), NULL, SYMBOL_SIZE(sym), 0);

                  Free(new_name);
                }
                generic = AddressNew32 (rel[tel2].r_offset);
                data = SectionGetData32 (corrsec, generic);
                addend = AddressNew32 (data);
                /* see "NOTE" in the comment at the start for this relocation */
                loadaddr = AddressSubUint32(AddressSub(generic,addend),4);

                switch (reloc_type)
                {
                  case R_ARM_TLS_IE32:
                    tentative = tentative_ie32;
                    break;
                  case R_ARM_TLS_LDM32:
                    tentative = tentative_ldm32;
                    break;
                  default:
                    FATAL(("complete support for reloc %d",reloc_type));
                }

                /* symbol for the GOT entry */
                name  =  StringConcat2("GOT:",SYMBOL_NAME(sym));
                gotsym = SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj), name);
                if (!gotsym)
                  gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name ,"R00A00+$",10,PERHAPS,FALSE,
                      OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))
                      ,AddressNew32(0), AddressNew32(0), NULL, AddressNew32(4), 0);
                Free(name);

                /* symbol for the got use (= this location), with which we
                 * associate the tentative rules for creating the GOT entry
                 */
                unique_source_name = StringIo("DIABLO_UNIQUE_GOT_USE:%s_offset_@G:%s", OBJECT_NAME(obj), generic, SYMBOL_NAME(sym));
                gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), unique_source_name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), tentative, AddressNullForObject(obj), 0);
                /* symbol for the actual load of the GOT entry's data
                 * (necessary to calculate the original address of the
                 * GOT entry)
                 */
                name = StringConcat2("DIABLO_UNIQUE_GOT_LOAD:", unique_source_name+strlen("DIABLO_UNIQUE_GOT_USE:"));
                gotload = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), loadaddr, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                Free(name);
                Free(unique_source_name);
                /* the s0004- is because of the "NOTE" in the comment at the start
                 * for this relocation
                 */
                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            AddressNullForObject(obj),
                                            T_RELOCATABLE (corrsec),
                                            generic, gotsym, FALSE, NULL,
                                            NULL, gotload,
                                            "S00A00+S01-s0004-" "\\" WRITE_32
                                           );
              }
              break;
            case R_ARM_TLS_LE32:
            case R_ARM_TLS_LDO32:
              {
                /* This relocation does the same as what's done
                 * inside the got entry for R_ARM_TLS_IE32: get the
                 * offset of the tls data relative to the start of the tls
                 * section
                 */
                t_uint32 data;
                t_address generic;
                t_address addend;
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                t_symbol *tls_start;
                t_string reloc_code;

              generic = AddressNew32 (rel[tel2].r_offset);
              data = SectionGetData32 (corrsec, generic);
              addend = AddressNew32 (data);
              tls_start = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), "$tls_start", "R00A00+$", 0, PERHAPS, TRUE,
                OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
                AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);

              /* The s0008+ below stems from the
               * fact that the first 8 bytes of the TLS segment is
               * occupied by a thread control block (see TCB_SIZE in
               * binutils/bfd/elf32-arm.c -- should actually be aligned
               * to a multiple of the tls section's alignment)  */
              if (reloc_type == R_ARM_TLS_LE32)
                reloc_code = "S00A00+S01-s0008+" "\\" WRITE_32;
              else
                reloc_code = "S00A00+S01-" "\\" WRITE_32;
              RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                          AddressNullForObject(obj),
                                          T_RELOCATABLE (corrsec),
                                          generic, sym, FALSE, NULL,
                                          NULL, tls_start,
                                          reloc_code
                                         );
              }
              break;

            /* For relocations processing MOVW and MOVT instructions (in both
            * ARM and Thumb state), the initial addend is formed by interpreting
            * the 16-bit literal field of the instruction as a 16-bit signed
            * value in the range -32768 <= A < 32768. The interpretation is the
            * same whether the relocated place contains a MOVW instruction or
            * a MOVT instruction. */
            case R_ARM_MOVT_ABS:
            case R_ARM_THM_MOVT_ABS:
            case R_ARM_MOVT_PREL:
            case R_ARM_THM_MOVT_PREL:
            case R_ARM_MOVW_ABS_NC:
            case R_ARM_THM_MOVW_ABS_NC:
            case R_ARM_MOVW_PREL_NC:
            case R_ARM_THM_MOVW_PREL_NC:
              {
                t_uint32 data = 0;
                t_address addend;
                t_reloc *reloc;
                const char *relocscript;
                t_address generic = AddressNew32 (rel[tel2].r_offset);
                t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

                /* extract the 16bit immediate ( depends on thm/arm and movw/movt ) */
                data = SectionGetData32 (corrsec, generic);
                switch (reloc_type)
                {
                  case R_ARM_MOVW_ABS_NC:
                  case R_ARM_MOVT_ABS:
                  case R_ARM_MOVW_PREL_NC:
                  case R_ARM_MOVT_PREL:
                    /* immediate format: i4:i12
                     * First: extract the the i12 */
                    data = Uint32SelectBits(data, 11, 0);
                    /* "or" the i4 */
                    data |= Uint32SelectBits(data, 19, 16) >> 4;
                    break;
                  case R_ARM_THM_MOVW_ABS_NC:
                  case R_ARM_THM_MOVT_ABS:
                  case R_ARM_THM_MOVW_PREL_NC:
                  case R_ARM_THM_MOVT_PREL:
                    /* immediate format: i4:i1:i3:i8 (caveat: thumb-2 consists of two
                     * concatenated 16 bit words -> switched values in upper/lower parts)
                     * First: extract the i8
                     */
                    data = Uint32SelectBits(data, 7+16, 0+16);
                    /* "or" the i3 */
                    data |= Uint32SelectBits(data, 14+16, 12+16) >> 4;
                    /* "or" the i1 */
                    data |= Uint32SelectBits(data,26-16,26-16) >> 11;
                    /* "or" the i4 */
                    data |= Uint32SelectBits(data, 19-16, 16-16) >> 4;
                    break;
                  default:
                    FATAL(("add support for reloc %d",reloc_type));
                    break;
                }

                /* sign extend the 16 bits */
                addend = AddressNew32 (data);
                addend = AddressSignExtend(addend,15);

               /* the relocation formula */
                switch (reloc_type)
                {
                  case R_ARM_MOVT_ABS:
                    relocscript = CALC_ABS "s0010>" "\\" WRITE_ARM_MOVWT;
                    break;
                  case R_ARM_THM_MOVT_ABS:
                    relocscript = CALC_ABS "s0010>" "\\" WRITE_THM_MOVWT;
                    break;
                  case R_ARM_MOVT_PREL:
                    relocscript = CALC_ABS "P-s0010>\\" WRITE_ARM_MOVWT;
                    break;
                  case R_ARM_THM_MOVT_PREL:
                    relocscript = CALC_ABS "P-s0010>\\" WRITE_THM_MOVWT;
                    break;
                  case R_ARM_MOVW_ABS_NC:
                    relocscript = CALC_ABS "S00m|sffff&\\" WRITE_ARM_MOVWT;
                    break;
                  case R_ARM_THM_MOVW_ABS_NC:
                    relocscript = CALC_ABS "S00m|sffff&\\" WRITE_THM_MOVWT;
                    break;
                  case R_ARM_MOVW_PREL_NC:
                    relocscript = CALC_ABS "P-S00m|sffff&\\" WRITE_ARM_MOVWT;
                    break;
                  case R_ARM_THM_MOVW_PREL_NC:
                    relocscript = CALC_ABS "P-S00m|sffff&\\" WRITE_THM_MOVWT;
                    break;
                  default:
                    FATAL(("missing relocation formula for reloc_type %d",reloc_type));
                    break;
                }

                /* add relocaction */
                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                    addend,                             /* addend */
                    T_RELOCATABLE (corrsec), generic,   /* from & offset */
                    sym,                                /* to symbol */
                    TRUE, NULL,                         /* hell: could be address of a function */
                    NULL, NULL, relocscript);
              }
              break;
            case R_ARM_ABS16:
              FATAL (("Implement ARMABS16!"));
            case R_ARM_ABS12:
              FATAL (("Implement ARMABS12!"));
            case R_ARM_THM_ABS5:
              FATAL (("Implement ARMTHM_ABS5!"));
            case R_ARM_ABS8:
              FATAL (("Implement ARMABS8!"));
            case R_ARM_SBREL32:
              FATAL (("Implement ARMSBREL32!"));
            case R_ARM_THM_PC8:
              FATAL (("Implement ARMTHBPC8!"));
            case R_ARM_AMP_VCALL9:
              FATAL (("Implement ARMAMPVCALL9!"));
            case R_ARM_SWI24:
              FATAL (("Implement ARMSWI24!"));
            case R_ARM_THM_SWI8:
              FATAL (("Implement ARMTHBSWI8!"));
            case R_ARM_XPC25:
              FATAL (("Implement ARMXPC25!"));
            case R_ARM_THM_XPC22:
              FATAL (("Implement ARMXPC22!"));
            case R_ARM_GLOB_DAT:
              FATAL (("Implement R_ARM_GLOB_DAT relocation"));
            case R_ARM_SBREL31:
              break;
              /* Ignoring wicked references */
            case R_ARM_ME_TOO:
              break;
            default:
              FATAL (("Implement ARM relocation type %d (0x%x) (translated to %d)!",
                      ELF32_R_TYPE (rel[tel2].r_info),
                      ELF32_R_TYPE (rel[tel2].r_info),
                      reloc_type));
          }
        }
        Free (rel);
      }

      /* ARM exidx sections */
      if (StringPatternMatch(".ARM.exidx*", secname))
      {
        t_section *exidx_section = SectionGetFromObjectByName(obj, secname);

        /* data for the first exidx entry */
        t_address current_address = AddressNew32(4);

        /* iterate over every entry */
        while (AddressIsLt(current_address, SECTION_CSIZE(exidx_section)))
        {
          t_uint32 data = SectionGetData32(exidx_section, current_address);

          /* skip relocated data */
          if (data == 0x1
              || data & (1U << 31))
          {
            t_reloc *rel = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                                            data,                     /* addend is the data that was originally present */
                                            T_RELOCATABLE(exidx_section),            /* from */
                                            current_address,                  /* from offset */
                                            NULL,                     /* to; none in this case */
                                            diabloobject_options.keep_exidx,
                                            NULL, NULL, NULL,         /* edge, corresp, e_sym */
                                            "A00" "\\" WRITE_32);
          }

          /* go to the next exidx entry */
          current_address = AddressAddUint32(current_address, 8);
        }
      }

      /* Find the Diablo comment */
      if (StringPatternMatch (".comment", secname))
      {
        if (!diabloobject_options.no_toolchain_check)
        {
#define DIABLOTC_COMMENT "DiabloTC"
          char * contents = Malloc (shdrs[tel].sh_size);
          fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
          IGNORE_RESULT(fread (contents, shdrs[tel].sh_size, 1, fp));

          /* read the strings contained in this comment */
          char * current_ptr = contents;
          while (current_ptr < contents + shdrs[tel].sh_size)
          {
            t_uint32 len = strlen(current_ptr);

            if (len > 0)
            {

              /* we have found one string in the comment section */
              if (StringPatternMatch("GCC:*", current_ptr))
                {
                  /* object file compiled with GCC */
                  seen_compiler = TRUE;
                  if (StringPatternMatch("*"DIABLOTC_COMMENT"*", current_ptr))
                    seen_diablo_gcc = TRUE;
                  else
                    WARNING(("WARNING: The object file \"%s\" contains code generated using an incompatible GCC/binutils toolchain (%s)!", OBJECT_NAME(obj), current_ptr));
                }
              else if (StringPatternMatch("clang *", current_ptr))
                {
                  /* object file compiled with LLVM/clang */
                  seen_compiler = TRUE;
                  if (StringPatternMatch("*"DIABLOTC_COMMENT"*", current_ptr))
                    seen_diablo_clang = TRUE;
                  else
                    WARNING(("WARNING: The object file \"%s\" contains code generated using an incompatible LLVM/clang toolchain (%s)!", OBJECT_NAME(obj), current_ptr));
                }
              else if (StringPatternMatch("*"DIABLOTC_COMMENT"*", current_ptr))
                {
                  /* object file compiled with something else diablo-ready */
                  ASSERT(StringPatternMatch("*gas*", current_ptr), ("Unrecognized comment \"%s\" in object file \"%s\"", current_ptr, OBJECT_NAME(obj)));
                  seen_diablo_gas = TRUE;
                }

              /* advance to the next string */
              current_ptr += len;
            }
            else
            {
              /* skip this null byte */
              current_ptr++;
            }
          }

          Free(contents);
        }
      }

      tel++;
    }
  }

  if (seen_diablo_gcc || seen_diablo_clang)
    {
      OBJECT_SET_COMPILER_GENERATED(obj,TRUE);
      //      DEBUG(("SET FLAG FOR %s",OBJECT_NAME(obj)));
    }
  else
    OBJECT_SET_COMPILER_GENERATED(obj,FALSE);

  /* Detect plt entries in linked binary and add symbols
   * for them. We can't find out these addresses in any
   * other way, because no relocations point to them.
   * Instead, the .plt stubs contain (hardcoded) offsets
   * to the got entries of the corresponding symbols.
   *
   * Do this after the above, because we rely on the symbols
   * that got added there.
   */

  if  (!OBJECT_PARENT(obj))
  {
    /* look for the plt section */
    for (tel = 0; tel < hdr32.e_shnum; tel++)
    {
      if ((sec_table[tel]!=NULL) &&
          (strcmp(".plt",SECTION_NAME(sec_table[tel]))==0))
      {
        t_section *pltsec;
        t_address  offset;
        t_bool thumb;

        ASSERT(gotsec,(".plt section without got section?"));
        pltsec = sec_table[tel];
        /* the first 20 bytes of the plt section contain a stub to call
         * the dynamic linker
         */
        offset = AddressNewForSection(pltsec,20);
        /* now parse all pltsection entries */
        while (AddressIsLt(offset,SECTION_CSIZE(pltsec)))
        {
          thumb = FALSE;
          /* every PLT entry might be preceded by a Thumb stub */
          if (AddressIsLe(AddressAddUint32(offset,16),SECTION_CSIZE(pltsec)) &&
              /* bx pc; nop */
              (SectionGetData32(pltsec,offset) == 0x46c04778))
          {
            thumb = TRUE;
            /* continue processing ARM stub. We'll add a symbol for this later */
            offset = AddressAddUint32(offset,4);
          }
          /* Supported pattern 1:
           *   ADD   ip, pc, #-8:PC_OFFSET_27_20: __PLTGOT(X)
           *   ADD   ip, ip, #-4:PC_OFFSET_19_12: __PLTGOT(X)
           *   LDR   pc, [ip, #0:PC_OFFSET_11_0: __PLTGOT(X)]!
           */
          if (AddressIsLe(AddressAddUint32(offset,12),SECTION_CSIZE(pltsec)) &&
              ((SectionGetData32(pltsec,offset)                     & 0xfffff000) == 0xe28fc000) &&
              ((SectionGetData32(pltsec,AddressAddUint32(offset,4)) & 0xfffff000) == 0xe28cc000) &&
              /* the 7 is because the offset can be either positive or negative */
              ((SectionGetData32(pltsec,AddressAddUint32(offset,8)) & 0xff7ff000) == 0xe53cf000))
          {
            t_uint32 data1, data2, data3;
            t_address gotentryaddr;
            t_symbol *gotsym;
            t_string tmp;
            /* extract the offset of the referred GOT entry */
            data1 = SectionGetData32(pltsec,offset);
            data2 = SectionGetData32(pltsec,AddressAddUint32(offset,4));
            data3 = SectionGetData32(pltsec,AddressAddUint32(offset,8));
            /* ip = pc + ... -> pc + 8 */
            gotentryaddr = AddressAdd(SECTION_CADDRESS(pltsec),AddressAddUint32(offset,8));
            /* add ARM shifted immediate from first add */
            gotentryaddr = AddressAddUint32(gotentryaddr,
              Uint32RotateRight(data1 & 0x000000ff, 2 * ((data1 & 0x00000f00) >> 8)));
            /* add ARM shifted immediate from second add */
            gotentryaddr = AddressAddUint32(gotentryaddr,
              Uint32RotateRight(data2 & 0x000000ff, 2 * ((data2 & 0x00000f00) >> 8)));
            /* sub/add immediate from ldr */
            if (data3 & 0x00800000)
              gotentryaddr = AddressAddUint32(gotentryaddr,data3 & 0x00000fff);
            else
              gotentryaddr = AddressSubUint32(gotentryaddr,data3 & 0x00000fff);
            ASSERT(AddressIsGe(gotentryaddr,SECTION_CADDRESS(gotsec)) &&
                   AddressIsLt(gotentryaddr,AddressAdd(SECTION_CADDRESS(gotsec),SECTION_CSIZE(gotsec))),
                   ("gotentry calculated at @G lies at @G, which is outside @T",offset,gotentryaddr,gotsec));
            gotsym = SymbolTableGetFirstSymbolByAddress(OBJECT_SYMBOL_TABLE(obj),gotentryaddr);
            while (gotsym &&
                   (strncmp(SYMBOL_NAME(gotsym),"DYNAMIC_GOT_ENTRY:",strlen("DYNAMIC_GOT_ENTRY:"))!=0))
            {
              gotsym = SymbolTableGetNextSymbolByAddress(gotsym,gotentryaddr);
            }
            ASSERT(gotsym,("could not find 'DYNAMIC_GOT_ENTRY:' symbol at address @G",gotentryaddr));
            /* add symbol for the plt stub */
            tmp = StringConcat2("PLTELEMORIG:", SYMBOL_NAME(gotsym)+strlen("DYNAMIC_GOT_ENTRY:"));
            SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), tmp, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(pltsec), offset, AddressNew32(0), NULL, AddressNew32(0), 0);
            Free(tmp);
            if (thumb)
            {
              tmp = StringConcat2("PLTELEMTHUMBORIG:", SYMBOL_NAME(gotsym)+strlen("DYNAMIC_GOT_ENTRY:"));
              SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), tmp, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(pltsec), AddressSubUint32(offset,4), AddressNew32(0), NULL, AddressNew32(0), 0);
              Free(tmp);
            }
            /* next stub */
            offset = AddressAddUint32(offset,12);
          }
          else
          {
            FATAL(("Unrecognized stub in .plt at offset @G",offset));
          }

        }
      }
    }
  }



  if (table)
    Free (table);
  if (shdrs)
    Free (shdrs);
  if (sechdrstrtab)
    Free (sechdrstrtab);
  if (symbol_table)
    Free (symbol_table);
  if (strtab)
    Free (strtab);
  if (sec_table)
    Free (sec_table);
  if (dynamic_table)
    Free(dynamic_table);
  if(dynamic_symbol_table)
    Free(dynamic_symbol_table);

  if (!diabloobject_options.no_toolchain_check)
    {
      ASSERT((seen_diablo_gcc || seen_diablo_clang || !seen_compiler) ,("Object file \"%s\" was not generated with a Diablo-ready compiler!", OBJECT_NAME(obj)));
      ASSERT(seen_diablo_gas,("Object file \"%s\" was not generated with a Diablo-ready assembler!", OBJECT_NAME(obj)));
    }
}

extern t_object *parser_object;

void *
ArmMaybeAddFromArmToThumbStubSpaceAndSymbol(t_ast_successors *args, void *data)
{
  t_object *obj = parser_object;
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_string veneer_symname = args->args[0]->data.name;

  t_section *parent;
  t_string name;
  t_address offset;
  t_symbol *destination_sym;
  t_address destination_sym_addr;

  destination_sym = SymbolTableLookup(OBJECT_SUB_SYMBOL_TABLE(obj),veneer_symname+strlen("DIABLO_POTENTIAL_THUMB_VENEER:"));
  /* should exist (possibly undefined) */
  ASSERT(destination_sym,("Cannot find symbol %s referred by a potential veneer target in object %s",veneer_symname+strlen("DIABLO_POTENTIAL_THUMB_VENEER:"),OBJECT_NAME(obj)));
  destination_sym_addr = StackExec(SYMBOL_CODE(destination_sym), NULL, destination_sym, NULL, FALSE, 0, obj);
  /* destination is thumb (-> address is odd) -> add veneer symbol */
  if (AddressIsNull(AddressAnd(destination_sym_addr,AddressNew32(1))))
    return NULL;

  parent = SectionGetFromObjectByName(obj, ".text");

  offset = AddressAlign(4,SECTION_CSIZE(parent));
  SECTION_SET_CSIZE(parent, AddressAddUint32(offset,8));
  SECTION_SET_DATA(parent,Realloc(SECTION_DATA(parent),SECTION_CSIZE(parent)));

  name = StringConcat3("__",veneer_symname+strlen("DIABLO_POTENTIAL_THUMB_VENEER:"),"_from_arm");
  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, FALSE, FALSE, T_RELOCATABLE(parent), offset, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
  Free(name);

  return NULL;
}


#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
