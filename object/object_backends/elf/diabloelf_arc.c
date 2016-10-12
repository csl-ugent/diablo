/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

/* TODO: verify .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_arc_info =
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


t_bool
IsElfArcSameEndianOnMsb (FILE * fp)
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

  if (machine != EM_ARCOMPACT)
    return FALSE;

  return TRUE;
}

t_bool
IsElfArcSameEndianOnLsb (FILE * fp)
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

  if (machine != EM_ARCOMPACT)
    return FALSE;

  return TRUE;
}

void
ElfWriteArcSameEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing arc binary"));
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
  hdr.e_machine = EM_ARCOMPACT;
  hdr.e_version = EV_CURRENT;
  VERBOSE(0, ("program entry point = @G", OBJECT_ENTRY (obj)));
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

  ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_arc_info);
  STATUS (STOP, ("Writing arc binary"));
}

void
ElfReadArcSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
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
  /* TODO: verify .plt section type */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, &dynamic_table, FALSE, read_debug, &elf_arc_info);
  


  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
  {
    tel = 0;
    while (tel < hdr32.e_shnum)
    {
      t_string secname = sechdrstrtab + shdrs[tel].sh_name;




      if ((shdrs[tel].sh_type >= SHT_LOPROC) && (shdrs[tel].sh_type <= SHT_HIPROC))
      {
        switch (shdrs[tel].sh_type)
        {
          default:
            FATAL(("Unhandled ARC specific section type 0x%x", (shdrs[tel].sh_type)));
        }
      }


      /* STEP 4b: handle architecture specific flags */


      /* STEP 5: Find the relocations */
      if ((StringPatternMatch (".rel.debug*", secname)) || (StringPatternMatch (".rela.debug*", secname)))
      {
        /* if we ignore debug info, we can also ignore debug relocations */
      }
      else if ((StringPatternMatch (".rel.stab*", secname)) || (StringPatternMatch (".rela.stab*", secname)))
      {
        /* Ignore these */
      }
      else if (StringPatternMatch (".rela*", secname))
      {
        Elf32_Rela *rel = Malloc (shdrs[tel].sh_size);
        t_uint32 tel2;
        t_section *corrsec;
        fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
               SEEK_SET);
        IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
        corrsec = sec_table[shdrs[tel].sh_info];

        for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)
        {
          ASSERT (symbol_table && strtab, ("Symtab or stringtab not found. Implement me"));
          switch (ELF32_R_TYPE (rel[tel2].r_info))
          {
	    case R_ARC_32_ME:
	      {
		t_uint32 data;
		t_symbol *sym;
		t_address generic;
		t_address addend;

		sym = table[ELF32_R_SYM (rel[tel2].r_info)];
		generic = AddressNew32 (rel[tel2].r_offset);

		data = SectionGetData32 (corrsec, generic);
		addend = AddressNew32 (rel[tel2].r_addend);

		RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					    addend,
					    T_RELOCATABLE (corrsec),
					    generic, sym, TRUE, NULL,
					    NULL, NULL,
					    "S00A00+ \\"
					    "= i0000ffff& s0010< } iffff0000& s0010> | l*w"
					    "\\ s0000 $");
	      }
	      break;

	    case R_ARC_S25H_PCREL:
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
					    "A00 =="
					    "i0001ff80& s0001> {"
					    "iffc00000& s000c> {"
					    "i000000f0& s0004< ||"
					    "s0001< S00 + Pifffffffd& -\\"

					    "s0001> =="
					    "i000003ff& s0001< }"
					    "i000ffc00& s000c< }"
					    "i00f00000& s0004> ||"
					    "l i0030f801& | w"

					    "\\s0000$"
					    );
	      }
	      break;

	    case R_ARC_S25W_PCREL:
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
					    "A00 =="
					    "i000007fc& s0002> {"
					    "iffc00000& s000d> {"
					    "i000f0000& s0003< ||"
					    "s0002< S00 + Pifffffffd& -\\"

					    "s0002> =="
					    "i000001ff& s0002< }"
					    "i0007fe00& s000d< }"
					    "i00780000& s0003> ||"
					    "l i0030f803& | w"

					    "\\s0000$"
					    );
	      }
	      break;

	  case R_ARC_SDA_LDST2:
	    {
		t_uint32 data;
		t_symbol *sym;
		t_address generic;
		t_address addend;

		sym = table[ELF32_R_SYM (rel[tel2].r_info)];
		generic = AddressNew32 (rel[tel2].r_offset);

		data = SectionGetData32 (corrsec, generic);
		addend = AddressNew32 (rel[tel2].r_addend);

		RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					    addend,
					    T_RELOCATABLE (corrsec),
					    generic, sym, TRUE, NULL,
					    NULL, NULL,
					    /* Note: Compensating for middle-endianness */
					    "s0004\\" /* TODO: fix - eleiper */
					    "s0002> s01ff& = s00ff& { s0008>s001f<|l|w"
					    "\\s0000$");
	    }
	    break;

	    case R_ARC_32:
	      {
		t_uint32 data;
		t_symbol *sym;
		t_address generic;
		t_address addend;

		sym = table[ELF32_R_SYM (rel[tel2].r_info)];
		generic = AddressNew32 (rel[tel2].r_offset);

		data = SectionGetData32 (corrsec, generic);
		addend = AddressNew32 (rel[tel2].r_addend);

		RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					    addend,
					    T_RELOCATABLE (corrsec),
					    generic, sym, TRUE, NULL,
					    NULL, NULL,
					    "S00A00+\\" WRITE_32);
	      }
	      break;

	    case R_ARC_SDA32_ME:
	      {
		t_uint32 data;
		t_symbol *sym;
		t_address generic;
		t_address addend;

		sym = table[ELF32_R_SYM (rel[tel2].r_info)];
		generic = AddressNew32 (rel[tel2].r_offset);

		data = SectionGetData32 (corrsec, generic);
		addend = AddressNew32 (rel[tel2].r_addend);

		RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					    addend,
					    T_RELOCATABLE (corrsec),
					    generic, sym, TRUE, NULL,
					    NULL, NULL,
					    "S00\\"
					    "= iffff0000& s0010> { i0000ffff& s0010< | "
					    "l|w \\ s0000$");
	      }
	      break;

            default:
              FATAL (("Implement RELOC_TYPE %d",
                      ELF32_R_TYPE (rel[tel2].r_info)));
          }
        }
        Free (rel);
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
            default:
              break;
          }
        }
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

        for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel)); tel2++)
        {
          ASSERT (symbol_table
                  && strtab,
                  ("Symtab or stringtab not found. Implement me"));
          switch (ELF32_R_TYPE (rel[tel2].r_info))
          {
            default:
              FATAL (("Implement ARC relocation type %d (0x%x)!",
                      ELF32_R_TYPE (rel[tel2].r_info),
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
}
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
