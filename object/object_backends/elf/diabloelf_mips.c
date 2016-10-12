/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

#define sign_extend(x,p)      (((x) ^ (1 << (p))) - (1 << (p)))
#define SELECT_BITS(i,b,e) ((i&((1<<(b+1))-1))>>e)

/* TODO: verify .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_mips_info =
{
  0x1000,         /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x1000,         /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
};

/* Write an elf object file. The elfhdr is supplied by the arch-specific code */

t_bool
IsElfMipsSwitchedEndianOnMsb (FILE * fp)
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

  if (machine != EM_SWITCHED_MIPS)
    return FALSE;

  return TRUE;
}


t_bool
IsElfMipsSameEndianOnLsb (FILE * fp)
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

  if (machine != EM_MIPS)
    return FALSE;

  return TRUE;
}

void
ElfWriteMipsSameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing mips binary"));
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
  hdr.e_machine = EM_MIPS;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = 0;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;		/* Set in common */
  hdr.e_shnum = 0;		/* Set in common */
  hdr.e_shstrndx = 0;		/* Set in common */

  /* TODO: verify .plt section type and entry size */
  ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_mips_info);
  STATUS (STOP, ("Writing mips binary"));
}

void
ElfReadMipsSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
	FATAL(("Mips disabled for now"));
#if 0
  Elf32_Shdr *shdrs = NULL;
  Elf32_Sym *symbol_table = NULL;
  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  int tel;
  char *sechdrstrtab = NULL;
  Elf32_Ehdr hdr32;
  t_symbol **table;
  t_symbol *gp;
  t_symbol *gp0;
  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  /* Reading elf headers does not depend on the type of objectfile */
  /* TODO: verify .plt section type */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, FALSE, read_debug, &elf_mips_info);

  gp0 = SymbolTableGetSymbolByName (OBJECT_SYMBOL_TABLE (obj), "_gp");


  FATAL(("Implement"));
  //  gp = SymbolTableAddUndefinedSymbol (OBJECT_SYMBOL_TABLE (obj), "__global");

  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
    {
      tel = 0;
      while (tel < hdr32.e_shnum)
	{
	  /* STEP 4b: handle architecture specific flags */








	  /* STEP 5: Find the relocations */
	  if (StringPatternMatch
	      (".rela.debug*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else if (StringPatternMatch
		   (".rela.pdr*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else if (StringPatternMatch
		   (".rel.debug*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else if (StringPatternMatch
		   (".rel.pdr*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      /* if we ignore debug info, we can also ignore debug relocations */
	    }
	  else
	    if (StringPatternMatch
		(".rela*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      t_section *corrsec;
	      Elf32_Rela *rel = Malloc (shdrs[tel].sh_size);
	      int tel2;

	      if (strcmp (sechdrstrtab + shdrs[tel].sh_name, ".rela.dyn") ==
		  0)
		{
		  FATAL (("Dynamic relocations not implemented!"));
		}

	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      fread (rel, shdrs[tel].sh_size, 1, fp);
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0;
		   tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  ASSERT (ELF32_R_TYPE (rel[tel2].r_info) <= 16,
			  ("Implement relocation %d",
			   ELF32_R_TYPE (rel[tel2].r_info)));
		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
		    default:
		      FATAL (("Implement relocation %d\n for mips",
			      ELF32_R_TYPE (rel[tel2].r_info)));
		      exit (0);
		      break;
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
		(".rel*", sechdrstrtab + shdrs[tel].sh_name)
		&& (shdrs[tel].sh_size != 0))
	    {
	      Elf32_Rel *rel = Malloc (shdrs[tel].sh_size);
	      t_bool prev_hi16 = FALSE;
	      t_bool prev_hi16_added = FALSE;
	      t_bool prev_got = FALSE;
	      t_relocatable *relocatable_hi16=NULL;
	      t_address addend_hi16=AddressNew32(0), generic_hi16=AddressNew32(0);
	      t_symbol *sym_hi16=NULL;
	      t_reloc *got16_corr=NULL;
	      t_reloc *temp;
	      int tel2;
	      t_section *corrsec;

	      if (strcmp (sechdrstrtab + shdrs[tel].sh_name, ".rel.dyn") == 0)
		{
		  FATAL (("Dynamic relocations not implemented!"));
		}

	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      fread (rel, shdrs[tel].sh_size, 1, fp);
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel));
		   tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
		    case R_MIPS_32:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));

			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    T_RELOCATABLE (corrsec),
						    generic, sym, TRUE, NULL,
						    NULL, NULL,
						    CALC_ABS "\\" WRITE_32
						    );
			prev_got = FALSE;
		      }
		      break;
		    case R_MIPS_26:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			data = SELECT_BITS (data, 25, 0);
			addend = AddressNew32 (data);
			if (SYMBOL_LOCAL (sym))
			  {
			    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							(obj), addend,
							T_RELOCATABLE (corrsec),
							generic, sym, FALSE,
							NULL, NULL, NULL,
							"As0002< Pif0000000& | S00+ s0002 > \\ i03ffffff & = l ifc000000 &|w \\l i03ffffff &-$"
							);
			  }
			else	/* external symbol */
			  {
			    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							(obj), addend,
							T_RELOCATABLE (corrsec),
							generic, sym, FALSE,
							NULL, NULL, NULL,
							"As0002< #0f S00+ s0002 > \\ i03ffffff & = l ifc000000 &|w \\l i03ffffff &-$"
							);
			  }
			//prev_hi16 = FALSE;
			prev_got = FALSE;
		      }
		      break;
		    case R_MIPS_HI16:
		      {
			t_uint32 data;
			sym_hi16 = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym_hi16)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic_hi16 = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic_hi16);
			data <<= 16;
			addend_hi16 = AddressNew32 (data);
			relocatable_hi16 = T_RELOCATABLE (corrsec);
			prev_hi16 = TRUE;
			prev_hi16_added = FALSE;
			prev_got = FALSE;
		      }
		      break;
		    case R_MIPS_LO16:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			data &= 0x0000ffff;
			addend = AddressNew32 (data);
			if (!prev_hi16)
			  FATAL (("No previous HI16 for LO16!"));
			if (prev_got)
			  {
			    temp =
			      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							  (obj), addend,
							  T_RELOCATABLE (corrsec),
							  generic, sym, TRUE,
							  NULL, got16_corr,
							  NULL,
							  "AS00+ \\ sffff& = l iffff0000 &|w \\l sffff &-$"
							  );
			    got16_corr->corresponding = temp;
			  }

			else
			  {
			    addend = AddressAdd (addend_hi16, addend);

			    if (!prev_hi16_added)
			      {
				if (!strcmp
				    (SYMBOL_NAME (sym_hi16), "_gp_disp"))
				  {
				    RelocTableAddRelocToSymbol
				      (OBJECT_RELOC_TABLE (obj), addend,
				       relocatable_hi16, generic_hi16,
				       sym_hi16, TRUE, NULL, NULL, gp,
				       "A #0f R+ P- = sffff& #0f - s0010>  \\ sffff& = l iffff0000&|w\\l sffff &-$"
				       );
				  }
				else
				  {
				    RelocTableAddRelocToSymbol
				      (OBJECT_RELOC_TABLE (obj), addend,
				       relocatable_hi16, generic_hi16,
				       sym_hi16, TRUE, NULL, NULL, NULL,
				       "A #0f S00+ = sffff& #0f - s0010> \\ sffff& = l iffff0000&|w\\l sffff &-$"
				       );
				  }
				prev_hi16_added = TRUE;
			      }

			    if (!strcmp (SYMBOL_NAME (sym), "_gp_disp"))
			      {
				RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							    (obj), addend,
							    T_RELOCATABLE
							    (corrsec),
							    generic, sym,
							    TRUE, NULL, NULL,
							    gp,
							    "A R+ P- i00000004 + \\ sffff& = l iffff0000 &|w \\l sffff &-$"
							    );
			      }
			    else	/* not _gp_disp */
			      {
				RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							    (obj), addend,
							    T_RELOCATABLE
							    (corrsec),
							    generic, sym,
							    TRUE, NULL, NULL,
							    NULL,
							    "AS00+ \\ sffff& = l iffff0000 &|w \\l sffff &-$"
							    );
			      }
			  }
			prev_got = FALSE;
		      }
		      break;
		    case R_MIPS_GPREL16:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);

			if (SYMBOL_LOCAL (sym))
			  {
			    /* Assuming here that GP0, the gp-value used to create the relocatable object, was zero (see
			     * also System V ABI for mips) */


			    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							(obj),
							AddressAdd
							(addend,
							 OBJECT_GP (obj)),
							T_RELOCATABLE (corrsec),
							generic, sym, TRUE,
							NULL, NULL, gp,
							"A #0f S00+ R- \\ sffff& = l iffff0000 &|w l sffff &-$"
							);
			  }
			else	/* global */
			  {
			    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							(obj), addend,
							T_RELOCATABLE (corrsec),
							generic, sym, TRUE,
							NULL, NULL, gp,
							"A #0f S00+ R-  \\ sffff& = l iffff0000 &|w l sffff &-$"
							);
			  }

			prev_got = FALSE;

		      }
		      break;
		    case R_MIPS_LITERAL:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    AddressAdd (addend,
								     OBJECT_GP
								     (obj)),
						    T_RELOCATABLE (corrsec),
						    generic, sym, TRUE, NULL,
						    NULL, gp,
						    "A #0f S00+ R- r+\\ sffff& = l iffff0000 &|w \\l sffff &-$"
						    );
		      }
		      prev_got = FALSE;
		      break;
		    case R_MIPS_REL32:
		      FATAL (("MIPS_REL32 relocation only for use with dynamic linker!"));
		      break;
		    case R_MIPS_GOT16:
		    case R_MIPS_CALL16:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			data &= 0x0000ffff;
			addend = AddressNew32 (data);
			if (SYMBOL_LOCAL (sym))
			  {
			    got16_corr =
			      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							  (obj), addend,
							  T_RELOCATABLE (corrsec),
							  generic, sym, TRUE,
							  NULL, NULL, NULL,
							  "H \\ sffff& = l iffff0000 &|w \\l sffff &-$"
							  );
			  }
			else
			  {
			    got16_corr =
			      RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE
							  (obj), addend,
							  T_RELOCATABLE (corrsec),
							  generic, sym, TRUE,
							  NULL, NULL, NULL,
							  "H \\ sffff& = l iffff0000 &|w \\l sffff &-$"
							  );
			  }
			prev_got = TRUE;
			break;
		      }
		    case R_MIPS_GPREL32:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			if (!sym)
			  FATAL (("Ajaj: symbol %s not found in symbol table",
				  strtab +
				  symbol_table[ELF32_R_SYM
					       (rel[tel2].r_info)].st_name));
			generic = AddressNew32 (rel[tel2].r_offset);
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);
/*			sym =
			  SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj),
						SYMBOL_TO_TYPE (sym),
						SYMBOL_TYPE (sym),
						SYMBOL_BASE (sym),
						AddressAdd (addend,
							    SECTION_CADDRESS
							    (T_SECTION
							     (SYMBOL_BASE
							      (sym)))),
						SYMBOL_NAME (sym),
						SYMBOL_LOCAL (sym),
                                                0); */
			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    OBJECT_GP (obj),
						    T_RELOCATABLE (corrsec),
						    generic, sym, TRUE, NULL,
						    NULL, gp,
						    "A S00+ R- \\" WRITE_32
						    );

			prev_got = FALSE;
			break;
		      }
		    case R_MIPS_PC32:
		      {
			t_address generic, addend;
			t_uint32 data;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
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
						    CALC_REL "\\" WRITE_32
						    );
			prev_got = FALSE;
			break;
		      }
		    case R_MIPS_PC16:
		    case R_MIPS_16:
		    default:
		      FATAL (("Implement relocation %d\n for mips",
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
#endif
}

void
ElfReadMipsSwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  FATAL (("MIPS Switched endian not supported"));
}
#endif
/* vim : set shiftwidth=2 foldmethod=marker: */
