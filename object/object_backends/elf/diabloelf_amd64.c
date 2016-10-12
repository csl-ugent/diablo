/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT64ADDRSUPPORT
#include <diabloelf.h>

static const Elf64_HeaderInfo elf_amd64_info =
{
  0x1000,         /*  Elf64_Xword pagesize; */
  SHT_PROGBITS,   /*  Elf64_Xword pltshtype; */
  0x10,           /*  Elf64_Xword pltentsize; */
  0x200000,       /*  Elf64_Xword ptload_align; */
  0x8,            /*  Elf64_Xword ptdynamic_align; */
  0x1,            /*  Elf64_Xword ptinterp_align; */
  0x4,            /*  Elf64_Xword ptnote_align; */
  0x8,            /*  Elf64_Xword ptphdr_align; */
  (Elf32_Word)-1  /*  Elf64_Xword pttls_align; -- keep at -1 if not supported */
};

t_bool
IsElfAmd64SwitchedEndianOnMsb (FILE * fp)
{
  Elf64_Byte buffer[EI_NIDENT];
  Elf64_Ehdr hdr;
  Elf64_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
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
    case ELFCLASS64:
      if (fread (&(hdr), sizeof (Elf64_Ehdr), 1, fp) != 1)
	{
	  FATAL (("Could not read elf header!!!!!!\nFile corrupt"));
	}
      fseek (fp, save, SEEK_SET);
      machine = hdr.e_machine;
      break;
    case ELFCLASS32:
    case ELFCLASSNONE:
    default:
      return FALSE;
      break;
    }

  if (machine != EM_SWITCHED_AMD64)
    return FALSE;

  return TRUE;
}


t_bool
IsElfAmd64SameEndianOnLsb (FILE * fp)
{
  Elf64_Byte buffer[EI_NIDENT];
  Elf64_Ehdr hdr;
  Elf64_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_WORDS_BIGENDIAN
  return FALSE;
#endif

  if (fread (buffer, sizeof (Elf64_Byte), EI_NIDENT, fp) != EI_NIDENT)
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
    case ELFCLASS64:
      if (fread (&(hdr), sizeof (Elf64_Ehdr), 1, fp) != 1)
	{
	  FATAL (("Could not read elf header!!!!!!\nFile corrupt"));
	}
      fseek (fp, save, SEEK_SET);
      machine = hdr.e_machine;
      break;
    case ELFCLASS32:
    case ELFCLASSNONE:
    default:
      return FALSE;
      break;
    }

  if (machine != EM_AMD64)
    return FALSE;

  return TRUE;
}

void
ElfWriteAmd64SameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf64_Ehdr hdr;
  STATUS (START, ("Writing Amd64 binary"));
  hdr.e_ident[EI_MAG0] = ELFMAG0;
  hdr.e_ident[EI_MAG1] = ELFMAG1;
  hdr.e_ident[EI_MAG2] = ELFMAG2;
  hdr.e_ident[EI_MAG3] = ELFMAG3;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;
  hdr.e_ident[EI_DATA] = ELFDATA2LSB;
  hdr.e_ident[EI_CLASS] = ELFCLASS64;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
    {
      hdr.e_ident[tel] = 0;
    }
  hdr.e_machine = EM_AMD64;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT64 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = 0;
  hdr.e_ehsize = sizeof (Elf64_Ehdr);
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;
  hdr.e_shnum = 0;
  hdr.e_shstrndx = 0;

  ElfWriteCommonSameEndian64 (fp, &hdr, obj, TRUE, &elf_amd64_info);
  STATUS (STOP, ("Writing Amd64 binary"));
}

void
ElfReadAmd64SameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf64_Shdr *shdrs = NULL;
  Elf64_Sym *symbol_table = NULL;
  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  t_uint32 tel;
  char *sechdrstrtab = NULL;
  Elf64_Ehdr hdr64;
  t_symbol **table;
  if (fread ((&hdr64), sizeof (Elf64_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon64 (fp, &hdr64, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, FALSE, FALSE, &elf_amd64_info);

  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr64.e_shnum)
    {
      tel = 0;
      while (tel < hdr64.e_shnum)
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
	      (".rela.debug*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else
	    if (StringPatternMatch
		(".rela*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      Elf64_Rela *rel = Malloc (shdrs[tel].sh_size);
	      t_uint32 tel2;
	      t_section *corrsec;
	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      fread (rel, shdrs[tel].sh_size, 1, fp);
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf64_Rela));
		   tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  switch (ELF64_R_TYPE (rel[tel2].r_info))
		    {
		    case R_AMD64_NONE:
		      break;
		    case R_AMD64_64:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64);
		      }
		      break;
		    case R_AMD64_PC32:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+P-\\" WRITE_64_32);
		      }
		      break;
		    case R_AMD64_GOT32:
		      {
			t_section * undef = OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_string tentative = 
			"GotSection {\
			  action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 8, 8) }\
			  section { RELOCATED64(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\L*W\\s0000$\") }\
			  address { SIGN_EXTEND(31,READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOT32_USE:\",MATCHED_NAME())))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
			}\
			GotSymbol {\
			  action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS()) }\
			  symbol  { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
			}";


		      gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,PERHAPS,FALSE,
			  T_RELOCATABLE(undef),
			  SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);

		      generic = AddressNew64 (rel[tel2].r_offset);
		      addend = AddressNew64 (rel[tel2].r_addend);

		      gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("DIABLO_AMD64_GOT32_USE:GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,TRUE,FALSE, T_RELOCATABLE(corrsec),generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

		      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			  addend,
			  T_RELOCATABLE (corrsec),
			  generic, gotsym, TRUE, NULL,
			  NULL, NULL,
			  "S00A00+g-" "\\" WRITE_64_32
			  );
		      }
		      break;
		    case R_AMD64_PLT32:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64_32);
		      }
		      break;
		    case R_AMD64_COPY:
		      {

		      }
		      break;
		    case R_AMD64_GLOB_DAT:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00\\" WRITE_64);
		      }
		      break;
		    case R_AMD64_JMP_SLOT:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00\\" WRITE_64);
		      }
		      break;
		    case R_AMD64_RELATIVE:
		      {
			FATAL (("Relocation R_AMD64_RELATIVE not implemented\n"));
		      }
		      break;
		    case R_AMD64_GOTPCREL:
		      {
			t_section * undef = OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_address generic = AddressNew64 (rel[tel2].r_offset);
			t_address addend = AddressNew64 (rel[tel2].r_addend);
			t_string tentative = StringIo (
			  "GotSection {\
			  action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 8, 8) }\
			  section { RELOCATED64(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\L*W\\s0000$\") }\
			  address { SIGN_EXTEND(31,READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOTPCREL_USE:\",MATCHED_NAME())))) + \
			            SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOTPCREL_USE:\",MATCHED_NAME())) - \
				    ABS(@G)}\
		              }\
		          GotSymbol {\
			    action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12) }\
			    symbol  { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
		          }", addend);


		      gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,PERHAPS,FALSE,
			  T_RELOCATABLE(undef),
			  SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);
		      Free (tentative);


		      gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("DIABLO_AMD64_GOTPCREL_USE:GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,TRUE,FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

		      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			  addend,
			  T_RELOCATABLE (corrsec),
			  generic, gotsym, TRUE, NULL,
			  NULL, NULL,
			  "S00A00+P-" "\\" WRITE_64_32
			  );
		      }
		      break;
		    case R_AMD64_32:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64_32);
		      }
		      break;
		    case R_AMD64_32S:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64_32);
		      }
		      break;
		    case R_AMD64_16:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64_16);
		      }
		      break;
		    case R_AMD64_PC16:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+P-\\" WRITE_64_16);
		      }
		      break;
		    case R_AMD64_8:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+\\" WRITE_64_8);
		      }
		      break;
		    case R_AMD64_PC8:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+P-\\" WRITE_64_8);
		      }
		      break;
		    case R_AMD64_DPTMOD64:
		      {
			FATAL (("Relocation R_AMD64_DTPMOD64 not implemented\n"));
		      }
		      break;
		    case R_AMD64_DTPOFF64:
		      {
			FATAL (("Relocation R_AMD64_DTPOFF64 not implemented\n"));
		      }
		      break;
		    case R_AMD64_TPOFF64:
		      {
			FATAL (("Relocation R_AMD64_TPOFF64 not implemented\n"));
		      }
		      break;
		    case R_AMD64_TLSGD:
		      {
			FATAL (("Relocation R_AMD64_TLSGD not implemented\n"));
		      }
		      break;
		    case R_AMD64_TLSLD:
		      {
			FATAL (("Relocation R_AMD64_TLSLD not implemented\n"));
		      }
		      break;
		    case R_AMD64_DTPOFF32:
		      {
			FATAL (("Relocation R_AMD64_DTPOFF32 not implemented\n"));
		      }
		      break;
		    case R_AMD64_GOTTPOFF:
		      {
			FATAL (("Relocation R_AMD64_GOTTPOFF not implemented\n"));
		      }
		      break;
		    case R_AMD64_TPOFF32:
		      {
			FATAL (("Relocation R_AMD64_TPOFF32 not implemented\n"));
		      }
		      break;
		    case R_AMD64_PC64:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+P-\\" WRITE_64);
		      }
		      break;
		    case R_AMD64_GOTOFF64:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "S00A00+g-\\" WRITE_64);
		      }
		      break;
		    case R_AMD64_GOTPC32:
		      {
			t_symbol *sym;
			t_address generic, addend;
			sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				strtab +
				symbol_table[ELF64_R_SYM
				(rel[tel2].r_info)].st_name));
			generic = AddressNew64 (rel[tel2].r_offset);
			addend = AddressNew64 (rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			    addend,
			    T_RELOCATABLE (corrsec),
			    generic, sym, TRUE, NULL,
			    NULL, NULL,
			    "gA00+P-\\" WRITE_64_32);
		      }
		      break;
		    case R_AMD64_GOT64:
		      {
			t_section * undef = OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_string tentative = 
			"GotSection {\
			  action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 8, 8) }\
			  section { RELOCATED64(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\L*W\\s0000$\") }\
			  address { READ_LINKED_VALUE64(SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOT64_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
			}\
			GotSymbol {\
			  action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12) }\
			  symbol  { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
			}";


		      gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,PERHAPS,FALSE,
			  T_RELOCATABLE(undef),
			  SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);

		      generic = AddressNew64 (rel[tel2].r_offset);
		      addend = AddressNew64 (rel[tel2].r_addend);

		      gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("DIABLO_AMD64_GOT64_USE:GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,TRUE,FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

		      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			  addend,
			  T_RELOCATABLE (corrsec),
			  generic, gotsym, TRUE, NULL,
			  NULL, NULL,
			  "S00A00+g-" "\\" WRITE_64
			  );
		      }
		      break;
		    case R_AMD64_GOTPCREL64:
		      {
			t_section * undef = OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_address generic = AddressNew64 (rel[tel2].r_offset);
			t_address addend = AddressNew64 (rel[tel2].r_addend);
			t_string tentative = StringIo (
			  "GotSection {\
			  action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 8, 8) }\
			  section { RELOCATED64(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\L*W\\s0000$\") }\
			  address { READ_LINKED_VALUE64(SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOTPCREL_USE:\",MATCHED_NAME()))) + \
			            SUBSYMBOL(CONCAT(\"DIABLO_AMD64_GOTPCREL64_USE:\",MATCHED_NAME())) - \
				    ABS(@G)}\
		              }\
		          GotSymbol {\
			    action  { ADD_SYMBOL_NEW(MATCHED_NAME(), 12) }\
			    symbol  { START_OF_SUBSECTION(\"Linker\",CONCAT(\"GOTELEM:\",MATCHED_NAME())) }\
		          }", addend);


		      gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,PERHAPS,FALSE,
			  T_RELOCATABLE(undef),
			  SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);
		      Free (tentative);


		      gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("DIABLO_AMD64_GOTPCREL64_USE:GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,TRUE,FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

		      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			  addend,
			  T_RELOCATABLE (corrsec),
			  generic, gotsym, TRUE, NULL,
			  NULL, NULL,
			  "S00A00+P-" "\\" WRITE_64_32
			  );
		      }
		      break;
		    case R_AMD64_GOTPC64:
		      FATAL (("implement"));
		      break;
		    case R_AMD64_GOTPLT64:
		      FATAL (("implement"));
		      break;
		    case R_AMD64_PLTOFF32:
		      {
			FATAL (("Relocation R_AMD64_PLTOFF32 not implemented\n"));
		      }
		      break;
		    default:
		  	FATAL (("Unknown relocation\n"));
		    }
		}
	      Free (rel);
	    }
	  else
	    if (StringPatternMatch
		(".rel.stab*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* Lets ignore these */
	    }
	  else
	    if (StringPatternMatch
		(".rel*", sechdrstrtab + shdrs[tel].sh_name))
		    FATAL (("REL relocations not implemented"));
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
}

void
ElfReadAmd64SwitchedEndian (FILE * fp, t_object * obj)
{
  FATAL (("AMD64 on an machine with different endianness currently not supported. Support for 386 ELF on machines with the same endianness is very rudementary"));
}
#endif
