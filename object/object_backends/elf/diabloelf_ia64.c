/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT64ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

#define sign_extend(x,p)      (((x) ^ (1 << (p))) - (1 << (p)))
#define SELECT_BITS(i,b,e) ((i&((1<<(b+1))-1))>>e)

/* TODO: .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf64_HeaderInfo elf_ia64_info =
{
  0x4000,         /*  Elf64_Xword pagesize; */
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
IsElfIA64SameEndianOnLsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf64_Ehdr hdr;
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

  if (machine != EM_IA_64)
    return FALSE;

  return TRUE;
}

t_bool
IsElfIA64SwitchedEndianOnMsb (FILE * fp)
{
  Elf32_Byte buffer[EI_NIDENT];
  Elf64_Ehdr hdr;
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

  if (machine != EM_SWITCHED_IA_64)
    return FALSE;

  return TRUE;
}

void
ElfWriteIA64SameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf64_Ehdr hdr;
  STATUS (START, ("Writing IA64 binary"));
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
  hdr.e_ident[EI_CLASS] = ELFCLASS64;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
    {
      hdr.e_ident[tel] = 0;
    }
  hdr.e_machine = EM_IA_64;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT64 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = EF_IA64_ABI64;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;		/* Set in common */
  hdr.e_shnum = 0;		/* Set in common */
  hdr.e_shstrndx = 0;		/* Set in common */

  /* TODO: verify .plt section type and entry size */
  ElfWriteCommonSameEndian64 (fp, &hdr, obj,TRUE, &elf_ia64_info);
  STATUS (STOP, ("Writing IA64 binary"));
}

void
ElfReadIA64SameEndian (FILE * fp, t_object * obj)
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
  /* TODO: verify .plt section type */
  ElfReadCommon64 (fp, &hdr64, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, FALSE, FALSE, &elf_ia64_info);
  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr64.e_shnum)
    {
      tel = 0;
      while (tel < hdr64.e_shnum)
	{
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
	      Elf64_Rela *rel = Malloc (shdrs[tel].sh_size);
	      int tel2;
	      t_section *corrsec;
	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      fread (rel, shdrs[tel].sh_size, 1, fp);
	      if (!(shdrs[shdrs[tel].sh_info].sh_flags & SHF_ALLOC))
		{
		  tel++;
		  Free (rel);
		  continue;
		}
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0;
		   tel2 < (shdrs[tel].sh_size / sizeof (Elf64_Rela)); tel2++)
		{
		  switch (ELF64_R_TYPE (rel[tel2].r_info))
		    {
		    case R_IA64_NONE:
		      break;
		    case R_IA64_IPLTLSB:	/* dynamic reloc, imported PLT, LSB */
		      FATAL (("Implement R_IA64_IPLTLSB"));
		      break;
		    case R_IA64_LTOFF_FPTR22:
		      {
			/* @ltoff(@fptr(s+a)), imm22: Ask for the creation of a GOT
			 * entry, containing the address of the official procedure
			 * descriptor of the function at (s+a): We present this as
			 * a pointer to the symbol of type  */
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic= AddressNew64(rel[tel2].r_offset);
			addend= AddressNew64( rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "q");
		      }
		      break;
		    case R_IA64_PCREL21B:
		      /* @pcrel(sym + add), ptb, call: Symbol + Addend - Address */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    CALC_REL_ALIGN "\\"
						    WRITE_IMM21);
		      }

		      break;
		    case R_IA64_GPREL22:	/* @gprel(sym + add), add imm22 */
		      /* The value, sym, write gp relative / imm22 */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "S00A00+g-" "\\"
						    WRITE_IMM22);
		      }

		      break;
		    case R_IA64_LTOFF22:	/* @ltoff(sym + add), add imm22 */
		      /*  Ask for the creation of a GOT entry, containing the address of the sym + add */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "u");
		      }

		      break;
		    case R_IA64_DIR64LSB:	/* symbol + addend, data8 LSB */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "S00A00+" "\\" WRITE_64);
		      }
		      break;
		    case R_IA64_SEGREL64LSB:	/* @segrel(sym + add), data8 LSB */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "S00A00+T-" "\\" WRITE_64);
		      }
		      break;
		    case R_IA64_GPREL64I:	/* @gprel(sym + add), mov imm64 */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			//DiabloPrint(stdout,"---------------- @S\n",sym);
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "S00A00+g-" "\\"
						    WRITE_IMM64);
		      }
		      break;
		    case R_IA64_FPTR64LSB:	/* @fptr(sym + add), data8 LSB */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym,
				("No symbol %d",
				 ELF64_R_SYM (rel[tel2].r_info)));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "p" "\\" WRITE_64);
		      }
		      break;
		    case R_IA64_GPREL64LSB:	/* @gprel(sym + add), data8 LSB */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    "S00A00+g-" "\\" WRITE_64);
		      }
		      break;
		    case R_IA64_PCREL64I:	/* @pcrel(sym + add), 64bit inst */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    CALC_REL_ALIGN "\\"
						    WRITE_IMM64);
		      }
		      break;
		    case R_IA64_PCREL64LSB:	/* @pcrel(sym + add), data8 LSB */
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			ASSERT (sym, ("No symbol"));
			generic = AddressNew64( rel[tel2].r_offset);
			addend = AddressNew64( rel[tel2].r_addend);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    (t_relocatable *) corrsec, generic, sym,
						    TRUE, NULL, NULL, NULL,
						    CALC_REL_ALIGN "\\"
						    WRITE_64);
		      }
		      break;
		    default:
		      FATAL (("Implement relocation type 0x%x\n",
			      ELF64_R_TYPE (rel[tel2].r_info)));
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
	    {
	   	      FATAL (("Implement relocation without addends for Itanium"));
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
ElfReadIA64SwitchedEndian (FILE * fp, t_object * obj)
{



  FATAL (("You are trying to optimize an Itanium binary. That is currently not supported"));
}
#endif
