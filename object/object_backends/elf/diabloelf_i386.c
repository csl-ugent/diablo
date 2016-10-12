/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT32ADDRSUPPORT
#include <diabloelf.h>

static const Elf32_HeaderInfo elf_386_info =
{
  0x1000,         /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x1000,         /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  0x4             /*  Elf32_Word pttls_align; normally should be max(alignment_of_all_tls_sections) */
};

t_bool
IsElfI386SwitchedEndianOnMsb (FILE * fp)
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

  if (machine != EM_SWITCHED_386)
    return FALSE;

  return TRUE;
}


t_bool
IsElfI386SameEndianOnLsb (FILE * fp)
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

  if (machine != EM_386)
    return FALSE;

  return TRUE;
}

void
ElfWriteI386SameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing I386 binary"));
  hdr.e_ident[EI_MAG0] = ELFMAG0;
  hdr.e_ident[EI_MAG1] = ELFMAG1;
  hdr.e_ident[EI_MAG2] = ELFMAG2;
  hdr.e_ident[EI_MAG3] = ELFMAG3;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;
  hdr.e_ident[EI_DATA] = ELFDATA2LSB;
  hdr.e_ident[EI_CLASS] = ELFCLASS32;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
    {
      hdr.e_ident[tel] = 0;
    }
  hdr.e_machine = EM_386;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  /* The 32-bit Intel Architecture defines no flags; so this member contains
   * zero. */
  hdr.e_flags = 0;
  hdr.e_ehsize = sizeof (Elf32_Ehdr);
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;
  hdr.e_shnum = 0;
  hdr.e_shstrndx = 0;

  ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_386_info);
  STATUS (STOP, ("Writing I386 binary"));
}

void
ElfReadI386SameEndian (FILE * fp, t_object * obj, t_bool read_debug)
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
  t_symbol **table = NULL;
  t_symbol **dynamic_table = NULL;

  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, &dynamic_table, FALSE, read_debug, &elf_386_info);
  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
    {
      tel = 0;
      while (tel < hdr32.e_shnum)
	{
	  /* STEP 4b: handle architecture specific flags */
	  /* STEP 5: Find the relocations */
	  if (StringPatternMatch
	      (".rel.debug*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else
	    if (StringPatternMatch
		(".rela*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      FATAL (("RELA relocations not implemented!"));
	    }
	  else
	    if (StringPatternMatch
		(".rel.stab*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* Ignore these */
	    }
	  else
            if (StringPatternMatch(".rel.dyn", sechdrstrtab + shdrs[tel].sh_name) || StringPatternMatch(".rel.plt", sechdrstrtab + shdrs[tel].sh_name))
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
		    case R_I386_COPY:
                      {
                        t_symbol *aliassym;
                        t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                        t_string symname;
                        t_address symaddr;

                        VERBOSE(3,("I386_COPY! @S\n", sym));
                        symname = StringConcat2("I386_COPY:", SYMBOL_NAME(sym));
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

		    case R_I386_GLOB_DAT:
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
		    case R_I386_JMP_SLOT:
                    {
                      t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                      t_string symname;
                      VERBOSE(3,("JUMP_SLOT! @S\n", sym));
                      symname = StringConcat2("JUMP_SLOT:", SYMBOL_NAME(sym));
                      SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), symname, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                      Free(symname);
                      break;
                    }
		    case R_I386_TLS_TPOFF:
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
                    case R_I386_RELATIVE:
                    {
                      t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                      char name[30];
                      /* dynamic relative relocations instruct the dynamic linker to add the load
                       * address of the library to value at a particular address in the library.
                       * The problem is that here we don't know what symbol was originally associated
                       * with that address, and when we encounter the original relocation (an
                       * R_I386_32 reloc) then we don't know at which address the corresponding
                       * R_I386_RELATIVE relocation is located in the .dyn.rel section -> we add
                       * a symbol here that indicates that there's a dynamic relocation pointing
                       * to a particular address, and later on we will hook it up with the
                       * actual symbol on that address for which we found a R_I386_32 reloc
                       */
                      ASSERT(ELF32_R_SYM (rel[tel2].r_info)==0,("R_I386_RELATIVE must refer symbol 0"));
                      snprintf(name,sizeof(name),"DYNRELATIVESOURCE:%x",rel[tel2].r_offset);
                      VERBOSE(3,("DYN RELATIVE for data at address 0x%x!",rel[tel2].r_offset));
                      SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                      break;
                    }
                    case R_I386_32:
                    {
                      t_symbol *sym = dynamic_table[ELF32_R_SYM (rel[tel2].r_info)];
                      t_string symname = StringConcat2("DYNABS:", SYMBOL_NAME(sym));
                      char name[30];

                      /* similar to the RELATIVE relocations above; while here we can know the
                       * associated symbol, we still need a special symbol because we must
                       * skip the static relative relocation at the target, and from the target
                       * we can't deduce the location of this corresponding dynamic relocation
                       * (at least not until after emulating the original linker)
                       */
                      snprintf(name,sizeof(name),"DYNABSOLUTESOURCE:%x",rel[tel2].r_offset);
                      VERBOSE(3,("DYN ABSOLUTE for data at address 0x%x!",rel[tel2].r_offset));
                      SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), name, "R00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);

                      /* If it doesn't exist yet, add a symbol from which we can decide this symbol
                       * is accessed through a dynamic absolute relocation.
                       */
                      if (!SymbolTableLookup(OBJECT_SYMBOL_TABLE(obj), symname))
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
	  else
	    if (StringPatternMatch
		(".rel*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      Elf32_Rel *rel = Malloc (shdrs[tel].sh_size);
	      t_uint32 tel2;
              t_bool tls_gd_optimized = FALSE;
	      t_section *corrsec;
              t_relocatable *undef = OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj));	

	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
	      IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel)); tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
		    case R_I386_32:
		      {
			t_uint32 data;
			t_symbol *sym;
			t_address generic, addend;
                        t_bool skip_static_reloc;
			sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);

                        /* Absolute addresses have to be adjusted by the load
                         * address at run time in case of a dynamic library
                         * afterwards (via a dynamic I386_RELATIVE/_32 relocation,
                         * see the comments for R_I386_RELATIVE/_32 for more info).
                         * Here we add a symbol that the code in
                         * AddDynRelativeRelocs() will use to associate this
                         * relocation with the correct R_I386_RELATIVE
                         * relocation.
                         */
                        skip_static_reloc = FALSE;
                        if (OBJECT_PARENT(obj))
                        {
                          if ((OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_SHARED_LIBRARY_PIC) ||
                              (OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_EXECUTABLE_PIC))
                          {
                            t_object *parent = OBJECT_PARENT(obj);
                            t_const_string name = StringConcat2("DYNABSRELOC:",SYMBOL_NAME(sym));
                            t_const_string dynabsname = StringConcat2("DYNABS:",SYMBOL_NAME(sym));
                            SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                            /* We must not statically relocate this value if it will be
                             * handled by a dynamic ABS32 relocation.
                             */
                            if (SymbolTableLookup(OBJECT_SYMBOL_TABLE(parent), dynabsname))
                              skip_static_reloc = TRUE;
                            Free(name);
                            Free(dynabsname);
                          }
                          /* if the address is taken of an external function in a dynamically
                           * linked binary, then the st_value of its dynamic symbol must be
                           * the address of its PLT entry (abi386-4, sections "symbol values"
                           * p 4-3 and "function address" p 5-6).
                           *
                           * Here, we just generate a symbol indicating the address has been
                           * taken. The linker script will look it up if there is a dynsym
                           * by the same name for which it has to generate a PLT entry.
                           */
                          if (OBJECT_DYNAMIC_SYMBOL_TABLE(OBJECT_PARENT(obj)) &&
                              (OBJECT_TYPE(OBJECT_PARENT(obj)) != OBJTYP_SHARED_LIBRARY_PIC) &&
                              (OBJECT_TYPE(OBJECT_PARENT(obj)) != OBJTYP_EXECUTABLE_PIC))
                          {
                            t_symbol_table *linkedsymtab;
                            t_string jumpslotname;
                            t_symbol *dynsym;

                            linkedsymtab = OBJECT_SYMBOL_TABLE(OBJECT_PARENT(obj));

                            /* If this object is being addded through LinkObjectFileNew (after linker emulation), linkedsymtab will be NULL */
                            if (linkedsymtab)
                            {
                              jumpslotname = StringConcat2("JUMP_SLOT:",SYMBOL_NAME(sym));
                              dynsym = SymbolTableGetSymbolByName(linkedsymtab,jumpslotname);
                              Free(jumpslotname);
                              if (dynsym)
                              {
                                SymbolTableAddAbsPrefixedSymWithFlagsIfNonExisting(linkedsymtab,OBJECT_PARENT(obj),"DYNSYMPLTADDR:",SYMBOL_NAME(sym),0);
                              }
                            }
                          }
                        }
                        if (!skip_static_reloc)
                        {
                          RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                              addend,
                              T_RELOCATABLE (corrsec),
                              generic, sym, TRUE, NULL,
                              NULL, NULL,
                              CALC_ABS "\\" WRITE_32
                              );
                        }
                        else
                        {
                          /* fake relocation to verify that the original linker indeed
                           * didn't relocate this value either
                           */
                          RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                              addend,
                              T_RELOCATABLE (corrsec),
                              generic, sym, TRUE, NULL,
                              NULL, NULL,
                              "S00*A00\\"
                              WRITE_32);
                        }
		      }
		      break;
		    case R_I386_PC32:
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
						    CALC_REL "\\" WRITE_32
						    );
		      }
		      break;
		    case R_I386_TLS_GOTIE:
		    case R_I386_GOT32:
		      /* toevoegen van datasectie -> adres invullen, relocatie naar daar, daarna relocatie van daar naar uiteindelijk doel */
		      {
			t_uint32 data;
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
	                t_string name;
	                t_const_string tentative;
	                /* we have to fill in the address for static/local symbol got entries,
	                 * the dynamic linker does it for dynamic/global symbol got entries
	                 */
			static char const * tentative_dynamic =
	                  "OrigSymbol {\
	                        action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)), 12, MATCHED_SYMBOL_FLAGS()) }\
	                        symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) }\
	                              }\
	                   GotSection {\
	                         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), DATA, 4, 4) }\
	                         section { RELOCATED32(0x0,CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)),0,0,0,\"S00A00+*s0000\\l*w\\s0000$\") }\
	                         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_I386_GOT32_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
	                              }\
	                    GotSymbol {\
	                       action {\
	                                ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
	                              }\
	                       symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
	                              }";
			static char const * tentative_static_no_tls =
	                  "OrigSymbol {\
	                        action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)), 12, MATCHED_SYMBOL_FLAGS()) }\
	                        symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) }\
	                              }\
	                   GotSection {\
	                         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), DATA, 4, 4) }\
	                         section { RELOCATED32(0x0,CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)),0,0,0,\"S00A00+\\l*w\\s0000$\") }\
	                         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_I386_GOT32_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
	                              }\
	                    GotSymbol {\
	                       action {\
	                                ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
	                              }\
	                      symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
	                              }";
                        static char const * tentative_static_add_dynabsreloc =
	                  "OrigSymbol {\
	                        action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)), 12, MATCHED_SYMBOL_FLAGS()) }\
	                        symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) }\
	                              }\
	                   GotSection {\
	                         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), DATA, 4, 4) }\
	                         section { RELOCATED32(0x0,CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)),0,0,0,\"S00A00+\\l*w\\s0000$\") }\
	                         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_I386_GOT32_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
	                              }\
	                    GotSymbol {\
	                       action {\
	                                ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
	                              }\
	                      symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
	                              }\
	                    GotDynAbsReloc {\
                              action {\
                                       ADD_SYMBOL_NEW(CONCAT(\"DYNABSRELOC:\",MATCHED_NAME()), 12, SYMBOL_TYPE_NOTYPE)\
                                     }\
                              symbol { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
                                     }";
			static char const * tentative_static_tls =
	                  "OrigSymbol {\
	                        action { ADD_SYMBOL_NEW(CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)), 12, MATCHED_SYMBOL_FLAGS()) }\
	                        symbol { DUPLICATE(SUBSTRING(MATCHED_NAME(),4,0)) }\
	                              }\
	                   GotSection {\
	                         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), DATA, 4, 4) }\
	                         section { RELOCATED32(0x0,CONCAT(\"ORIG:\",SUBSTRING(MATCHED_NAME(),4,0)),0,\"$tls_start\",0,\"S00A00+S01-\\l*w\\s0000$\") }\
	                         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_I386_GOT32_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
	                              }\
	                    GotSymbol {\
	                       action {\
	                                ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS())\
	                              }\
	                      symbol  {  START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
	                              }";

			if (SYMBOL_ORDER(sym) == -1)
			{
                          t_string new_name=StringIo("got_global_for_local_symbol_%s_%d_from_%s",SYMBOL_NAME(sym),ELF32_R_SYM (rel[tel2].r_info),OBJECT_NAME(obj));

                          sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), new_name, SYMBOL_CODE(sym), 10, PERHAPS, FALSE, SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), NULL, SYMBOL_SIZE(sym), 0);
                          Free(new_name);
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
                          if (ELF32_R_TYPE (rel[tel2].r_info) == R_I386_GOT32)
                          {
                            name = StringConcat2("GLOB_DAT:", SYMBOL_NAME(sym));
                            if (SymbolTableGetSymbolByName(table, name))
                              tentative = tentative_dynamic;
                            /* If we're after linker emulation and dealing with an undefined symbol, assume it has to be a GLOB_DAT symbol */
                            else if (after_linker_emulation && (SYMBOL_BASE(sym) == undef) && !SymbolTableGetSymbolByName(table, SYMBOL_NAME(sym)))
                            {
                              /* If we haven't added this symbol for this subobject yet, add it as a GLOB_DAT symbol */
                              if (!SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), name))
                                ElfAddGlobDatSymbol(obj, sym);

                              tentative = tentative_dynamic;
                            }
                            else
                            {
                              /* if this is inside a PIC object, the GOT entry needs to be relocated */
                              if ((OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(OBJECT_PARENT(obj)) == OBJTYP_EXECUTABLE_PIC))
                                tentative = tentative_static_add_dynabsreloc;
                              else
                                tentative = tentative_static_no_tls;
                            }
                          }
                          else if (ELF32_R_TYPE (rel[tel2].r_info) == R_I386_TLS_GOTIE)
                          {
                            name = StringConcat2("TLS_TPOFF:",SYMBOL_NAME(sym));
                            if (SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(OBJECT_PARENT(obj)),name))
                              tentative = tentative_dynamic;
                            else
                              tentative = tentative_static_tls;
                          }
                          else
                            FATAL(("Add support for generating got code for reloc %d",ELF32_R_TYPE (rel[tel2].r_info)));
                          Free(name);
                        }
                        else
                          tentative = tentative_static_no_tls;

                        name = StringConcat2("GOT:",SYMBOL_NAME(sym));
                        gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$",10,PERHAPS,FALSE,
                                                      undef, AddressNew32(0), AddressNew32(0), tentative, SYMBOL_SIZE(sym), 0);
                        Free(name);
			
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);
	
                        name = StringConcat2("DIABLO_I386_GOT32_USE:GOT:", SYMBOL_NAME(sym));
			gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                        Free(name);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    T_RELOCATABLE (corrsec),
						    generic, gotsym, TRUE, NULL,
						    NULL, NULL,
						    "S00A00+g-" "\\" WRITE_32
						    );
		      }
		      break;
		    case R_I386_GOTPC:
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
						    "gA00+P-" "\\" WRITE_32
						    );
		      }
		      break;
		    case R_I386_GOTOFF:
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
						    "S00A00+g-" "\\" WRITE_32
						    );
		      }
		      break;
		    case R_I386_PLT32:
		      {
			t_uint32 data;
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

                        /* If we see this kind of relocation to ___tls_get_addr right after a R_I386_TLS_GD,
                         * it might have been optimized away.
                         */
                        if (tls_gd_optimized &&
                            !strncmp (SYMBOL_NAME(sym), "___tls_get_addr", strlen("___tls_get_addr")))
                        {
                          tls_gd_optimized = FALSE;
                          continue;
                        }

			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);

                        /* PLT32 should point to the PLT entry for the symbol and cause
                         * a PLT entry to be generated. In practice, the linker optimises
                         * these and changes them into direct branches, unless a PLT entry
                         * is absolutely required. We can detected this based on the fact
                         * that a JUMP_SLOT symbol has been created by us (which can only
                         * happen for dynamically linked binaries)
                         */
                        if (OBJECT_PARENT(obj))
                        {
                          t_string jumpslotname;
                          t_symbol *jumpslotsym;

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
                          jumpslotsym = SymbolTableGetSymbolByName(table, jumpslotname);


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
                            t_string pltentryname;

                            pltentryname = StringConcat2("PLTELEMSYM:",SYMBOL_NAME(sym));
                            sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), pltentryname, "R00A00+$",10,PERHAPS,FALSE,
                                undef, AddressNew32(0), AddressNew32(0), NULL, 16, SYMBOL_TYPE_NOTYPE);
                            Free(pltentryname);
                          }
                        }
                        RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                            addend,
                            T_RELOCATABLE (corrsec),
                            generic, sym, TRUE, NULL,
                            NULL, NULL,
                            CALC_REL "\\" WRITE_32
                            );
		      }
		      break;
		    case R_I386_16:
		      {
			t_uint32 data;
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    T_RELOCATABLE (corrsec),
						    generic, sym, TRUE, NULL,
						    NULL, NULL,
						    CALC_ABS  "i0000ffff &\\l iffff0000 & | w\\s0000$"
						    );
		      }
		      break;
		    case R_I386_TLS_IE:
                      {
                        t_symbol * eoftls = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), "_etls", "R00A00+$", 0,
                          PERHAPS,TRUE, undef , AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);

                        t_uint16 opcode;
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
                                                    NULL, eoftls,
                                                    "S00A00+S01-""\\" WRITE_32
                                                   );
			
			/* Binutils seems to optimize the TLS_IE relocations
			 * (at least for statically linked programs. No idea
			 * yet what will happen for dynamically linked programs
			 * with tls. We need to perform the optimization here
			 * as well {{{ */

                        if (G_T_UINT32(generic)!=1)
                        {
                          opcode = SectionGetData16 (corrsec, AddressSubUint32(generic, 2));


                          if ((opcode & 0xff00) == 0xa100)
                          {
                            opcode = (opcode & 0xff) | 0xb800;
                          }
                          else if ((opcode & 0xc7ff) == 0x058b)
                          {
                            opcode = (((opcode & 0x3800) >> 3) | 0xc000 |  0xc7);
                          }
                          else
                          {
                            FATAL(("Unknown R_I386_TLS_IE opcode: in %s:%s at offset @G: 0x%x", OBJECT_NAME(obj), SECTION_NAME(corrsec), generic, opcode));
                          }
                          
                          SectionSetData16 (corrsec, AddressSubUint32(generic, 2), opcode);
                        }
                        else
                        {
                          opcode = SectionGetData8 (corrsec, AddressSubUint32(generic, 1));
                          if (opcode == 0xa1)
                          {
                            opcode = 0xb8;
                          }
                          else
                            FATAL(("Unknown R_I386_TLS_IE opcode: in %s:%s at offset @G: 0x%x", OBJECT_NAME(obj), SECTION_NAME(corrsec), generic, opcode));
                          SectionSetData8 (corrsec, AddressSubUint32(generic, 1), opcode);
                        }
                        /* }}} */
                        break;

                      }
		    case R_I386_TLS_LE:
                      {
                        t_symbol * eoftls = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), "_etls", "R00A00+$", 0,
                          PERHAPS, TRUE, undef , AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);

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
                                                    NULL, eoftls,
                                                    "S00A00+S01-""\\" WRITE_32
                                                   ); 
                      }
		      break;
                    case R_I386_TLS_GD:
                      {
                        /* The General Dynamic TLS access model is the most generic and can be relaxed to one of the other
                         * access models to give more performance. In the current implementation only a relaxation to the
                         * Initial Exec access model is provided (GD itself isn't even implemented).
                         */

                        /* The GD pattern is as follows:
                         *    leal  x@tlsgd(,%ebx,1),%eax  ==> With a R_I386_TLS_GD relocation
                         *    call  x@tlsgdplt ==> With a R_I386_PLT32 relocation to ___tls_get_addr
                         *
                         * This should result in two GOT entries being created:
                         *    GOT[n] ==> With a R_I386_TLS_DTPMOD32 relocation
                         *    GOT[n+1] ==> With a  R_I386_TLS_DTPOFF32 relocation
                         */

                        t_symbol * eoftls = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),"_etls","R00A00+$",0,
                            PERHAPS,TRUE, undef ,AddressNew32(0),AddressNew32(0), NULL, AddressNew32(0), 0);

                        /* TODO: Decide when the linker will relax to Initial Exec */
                        if (TRUE)
                        {
                          t_address offset = AddressNew32 (rel[tel2].r_offset);
                          t_address addend = AddressNew32 (SectionGetData32 (corrsec, offset));
                          t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

                          /* Recognize the GD pattern and rewrite it to the IE pattern, which looks as follows:
                           *    movl  %gs:0, %eax
                           *    addl  x@indntpoff, %eax ==> With a R_I386_TLS_IE relocation
                           */
                          t_uint32 opcode = SectionGetData32(corrsec, AddressSubUint32(offset, 3));
                          if ((opcode & 0xffffff) == 0x1d048d)
                          {
                            SectionSetData16(corrsec, AddressSubUint32(offset, 3), 0xa165);
                            SectionSetData32(corrsec, AddressSubUint32(offset, 1), 0x00000000);
                            opcode = SectionGetData32(corrsec, AddressAddUint32(offset, 3));
                            if ((opcode & 0x0000ff00) == 0x0000e800)
                            {
                              SectionSetData32(corrsec, AddressAddUint32(offset, 3), 0x0000e881);
                            }
                            else
                              FATAL(("Unknown R_I386_TLS_IE opcode: in %s:%s at offset @G: 0x%x", OBJECT_NAME(obj), SECTION_NAME(corrsec), offset, opcode));
                          }
                          else
                            FATAL(("Unknown R_I386_TLS_IE opcode: in %s:%s at offset @G: 0x%x", OBJECT_NAME(obj), SECTION_NAME(corrsec), offset, opcode));

                          RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
                              addend,
                              T_RELOCATABLE (corrsec),
                              AddressAddUint32(offset,5), sym, TRUE, NULL,
                              NULL, eoftls,
                              "S01S00A00+-""\\" WRITE_32
                              );
                          /* This relocation is followed by a R_I386_PLT32
                           * relocation going to ___tls_get_addr (this is
                           * guaranteed by the ABI). When optimized to
                           * Initial Exec, we can remove this relocation.
                           */
                          tls_gd_optimized = TRUE;
                        }
                      }
                      break;
                      case R_I386_NONE:
                        /* Not sure why these relocations are even generated, but nothing should be done when they are encountered */
                        break;
		    case R_I386_GLOB_DAT:
		      FATAL (("Implement R_I386_GLOB_DAT relocation"));
		      break;
		    case R_I386_COPY:
		    case R_I386_JMP_SLOT:
		    default:
		      FATAL (("Implement relocation %d!",
			      ELF32_R_TYPE (rel[tel2].r_info)));
		    }
		}
	      Free (rel);
	    }
	  tel++;
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
}

void
ElfReadI386SwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf32_Shdr *shdrs = NULL;
  Elf32_Sym *symbol_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  int tel;
  char *sechdrstrtab = NULL;
  t_section **sec_table = NULL;
  Elf32_Ehdr hdr32;
  t_symbol **table;
  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  printf("Doing switched endian read!\n");

  Elf32HdrSwitchEndian(&hdr32);
  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, TRUE, read_debug, &elf_386_info);
  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
  {
      tel = 0;

      FATAL(("Implement relocs!"));
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
}

/* Either the GOT or some other data section follows after the RELRO segment,
   if one exists. 
   If so, the one following the RELRO need to be aligned on a page boundary to
   avoid that it partially ends up in the page being protected against
   writes as part of the RELRO segment 
 */


t_uint64
ElfI386AlignGotAfterRelRO(t_object *obj, long long currpos)
{
  t_bool have_got = FALSE;
  int i;
  t_section * sec;

  if (OBJECT_RELRO_CSIZE(obj)==0)
    return currpos;

  OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      if (StringPatternMatch(".got",SECTION_NAME(sec)) ||
          StringPatternMatch(".igot",SECTION_NAME(sec))
          )
        have_got = TRUE;
    }

  if (!have_got)
    return currpos;
    
  if (OBJECT_GOT_IN_RELRO(obj))
    return currpos;

  return  (currpos + 0x1000 - 1) & ~(0x1000 - 1); 
}

t_uint64
ElfI386AlignDataAfterRelRO(t_object *obj, long long currpos)
{
  t_bool have_got = FALSE;
  int i;
  t_section * sec;

  if (OBJECT_RELRO_CSIZE(obj)==0)
    return currpos;

  OBJECT_FOREACH_SECTION(obj,sec,i)
    {
      if (StringPatternMatch(".got",SECTION_NAME(sec)) ||
          StringPatternMatch(".igot",SECTION_NAME(sec))
          )
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

t_uint64 ElfI386AlignStartOfRelRO(t_object *obj, long long currpos)
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
          StringPatternMatch(".gcc_except_table.*",secname) || 
          StringPatternMatch(".tdata",secname) || 
          StringPatternMatch(".tdata.*",secname) || 
          StringPatternMatch(".gnu.linkonce.td.*",secname) || 
          // in glibc, tbss have effective size 0
          //          StringPatternMatch(".tbss",secname) || 
          //          StringPatternMatch(".tbss.*",secname) || 
          StringPatternMatch(".gnu.linkonce.tb.*",secname) || 
          StringPatternMatch(".tcommon",secname) || 
          StringPatternMatch(".preinit_array",secname) || 
          StringPatternMatch(".init_array",secname) || 
          StringPatternMatch(".fini_array",secname) || 
          StringPatternMatch(".ctors",secname) || 
          StringPatternMatch(".ctors.*",secname) || 
          StringPatternMatch(".dtors",secname) || 
          StringPatternMatch(".dtors.*",secname) || 
          StringPatternMatch(".jcr",secname) || 
          StringPatternMatch(".iplt",secname) || 
          StringPatternMatch(".data.rel.ro.local*",secname) || 
          StringPatternMatch(".gnu.linkonce.d.rel.ro.local.*",secname) || 
          StringPatternMatch(".data.rel.ro",secname) || 
          StringPatternMatch(".gnu.linkonce.d.rel.ro.*",secname) || 
          StringPatternMatch(".data.rel.ro",secname) || 
          (StringPatternMatch(".got",secname) && OBJECT_GOT_IN_RELRO(obj)) || 
          (StringPatternMatch(".igot",secname) && OBJECT_GOT_IN_RELRO(obj))
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


#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
