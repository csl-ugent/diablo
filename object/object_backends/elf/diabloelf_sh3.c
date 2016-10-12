/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

/* TODO: verify .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_sh3_info =
{
  0x4,            /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x4,            /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
};

t_bool
IsElfSh3SameEndianOnMsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_LITTLE_ENDIAN
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

  if (machine != EM_SH)
    return FALSE;

  return TRUE;
}

t_bool
IsElfSh3SwitchedEndianOnMsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_LITTLE_ENDIAN
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

  if (machine != EM_SWITCHED_SH)
    return FALSE;

  return TRUE;
}

t_bool
IsElfSh3SwitchedEndianOnLsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_BIG_ENDIAN
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

  if (machine != EM_SWITCHED_SH)
    return FALSE;

  return TRUE;
}



t_bool
IsElfSh3SameEndianOnLsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf32_Ehdr hdr;
  Elf32_Half machine;		/* For 64 bits this is Elf64Machine, but currently they are the same size ! */
  t_uint32 save = ftell (fp);

#ifdef DIABLOSUPPORT_BIG_ENDIAN
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

  if (machine != EM_SH)
    return FALSE;

  return TRUE;
}

void
ElfWriteSh3SameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing SH3 binary"));
  hdr.e_ident[EI_MAG0] = ELFMAG0;
  hdr.e_ident[EI_MAG1] = ELFMAG1;
  hdr.e_ident[EI_MAG2] = ELFMAG2;
  hdr.e_ident[EI_MAG3] = ELFMAG3;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;

#ifdef DIABLOSUPPORT_LITTLE_ENDIAN
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
  hdr.e_machine = EM_SH;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = EF_ARM_HASENTRY | EF_ARM_SYMSARESORTED;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;
  hdr.e_shnum = 0;
  hdr.e_shstrndx = 0;

  /* TODO: verify .plt section type and entry size */
  ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_sh3_info);
  STATUS (STOP, ("Writing SH3 binary"));
}

void
ElfReadSh3SameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf32_Shdr *shdrs = NULL;
  Elf32_Sym *symbol_table = NULL;
  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  int tel;
  char *sechdrstrtab = NULL;
  Elf32_Ehdr hdr32;
  t_symbol **table;
  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  /* Reading elf headers does not depend on the type of objectfile */
  /* TODO: verify .plt section type */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, FALSE, read_debug, &elf_sh3_info);
  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
    {
      tel = 0;
      while (tel < hdr32.e_shnum)
	{
	  if (StringPatternMatch
	      (".rela.debug*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else
	    if (StringPatternMatch
		(".rela.stab*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* Lets ignore these */
	    }
	  else
	    if (StringPatternMatch
		(".rela*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      Elf32_Rela *rel = Malloc (shdrs[tel].sh_size);
	      t_uint32 tel2;
	      t_section *corrsec;
	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0;
		   tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));

		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
#if 0
		    case R_SH_NONE:
		      FATAL (("Relocs: R_SH_NONE found\n"));
		      break;
		    case R_SH_DIR32:
		      {
			t_address generic;
			t_address addend;
			t_uint32 implicit_addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

			if (!sym)
			  FATAL (("Symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));

			generic = AddressNew32( rel[tel2].r_offset);
			addend = AddressNew32( rel[tel2].r_addend);
			/* sh3 oddity: account for both explicit and implicit addends */
			implicit_addend =
			  SectionGetData32 (corrsec, generic);
			addend = AddressAddUint32 (addend, implicit_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend, corrsec,
						    generic, sym, TRUE, NULL,
						    NULL, NULL,
						    CALC_ABS "\\" WRITE_32,
						    TRUE); */
		      }
		      break;
		    case R_SH_REL32:
		      {
			t_address generic;
			t_address addend;
			t_uint32 implicit_addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

			if (!sym)
			  FATAL (("Symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));

			generic = AddressNew32( rel[tel2].r_offset);
			addend = AddressNew32( rel[tel2].r_addend);
			/* sh3 oddity: account for both explicit and implicit addends */
			implicit_addend =
			  SectionGetData32 (corrsec, generic);
			addend = AddressAddUint32 (addend, implicit_addend);

			if (SECTION_TYPE(corrsec) != CODE_SECTION)
			  {
			    FATAL (("REL 32 not in code section!"));
			  }

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend, corrsec,
						    generic, sym, TRUE, NULL,
						    NULL, NULL,
						    CALC_REL "\\" WRITE_32,
						    TRUE);
		      }
		      break;

		    case R_SH_GOTPC:
		      {

			t_address generic;
			t_address addend;
			t_uint32 implicit_addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];

			//DiabloPrint(stdout,"Relocs: R_SH_GOTPC found\n");
			//DiabloPrint(stdout,"Symbol %s\n",strtab+symbol_table[ELF32_R_SYM(rel[tel2].r_info)].st_name);
			//DiabloPrint(stdout,"Offset: %x, Addend: %d\n",rel[tel2].r_offset,rel[tel2].r_addend);

			generic = AddressNew32( rel[tel2].r_offset);
			addend = AddressNew32( rel[tel2].r_addend);
			/* sh3 oddity: account for both explicit and implicit addends */
			implicit_addend =
			  SectionGetData32 (corrsec, generic);
			addend = AddressAddUint32 (addend, implicit_addend);

			/* nothing special needs to be done for this kind of relocation,
			 * as the symbol it references (_GLOBAL_OFFSET_TABLE) will point 
			 * to the .got section, so 'S' in this case is the same as 'GOT' */
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend, corrsec,
						    generic, sym, FALSE, NULL,
						    NULL, NULL,
						    CALC_REL "\\" WRITE_32,
						    TRUE);
			break;
		      }

		      /* All other reltypes haven't been encountered yet.
		       * They will be implemented on an as needed basis. */
		    case R_SH_DIR8WPN:
		    case R_SH_IND12W:
		    case R_SH_DIR8WPL:
		    case R_SH_DIR8WPZ:
		    case R_SH_DIR8BP:
		    case R_SH_DIR8W:
		    case R_SH_DIR8L:
		    case R_SH_SWITCH16:
		    case R_SH_SWITCH32:
		    case R_SH_USES:
		    case R_SH_COUNT:
		    case R_SH_ALIGN:
		    case R_SH_CODE:
		    case R_SH_DATA:
		    case R_SH_LABEL:
		    case R_SH_SWITCH8:
		    case R_SH_GNU_VTINHERIT:
		    case R_SH_GNU_VTENTRY:
		    case R_SH_PLT32:
		    case R_SH_COPY:
		    case R_SH_GLOB_DAT:
		    case R_SH_JMP_SLOT:
		    case R_SH_RELATIVE:
		    case R_SH_GOTOFF:
		    case R_SH_GOT32:
#endif
		    default:
		      FATAL (("Implement RELOC_TYPE %d",
			      ELF32_R_TYPE (rel[tel2].r_info)));
		    }
		}
	      Free (rel);
	    }
	  else
	    if (StringPatternMatch
		(".rel*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      FATAL (("REL relocations not implemented!"));
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
}

void
ElfReadSh3SwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  FATAL (("SH3 Switched endian not supported"));
}
#endif
