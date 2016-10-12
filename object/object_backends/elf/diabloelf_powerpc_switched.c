/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

static const Elf32_HeaderInfo elf_ppc32_info =
{
  0x1000,         /*  Elf32_Word pagesize; */
  SHT_NOBITS,     /*  Elf32_Word pltshtype; */
  0,              /*  Elf32_Word pltentsize; */
  0x10000,        /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- not yet supported for elf/ppc32 it seems */
};


typedef struct
{
  t_section *sec;
  Elf32_Shdr hdr;
  t_int32 described_by_pdr;
  t_int32 prepadding;
} sec_store_info;

t_bool
IsElfPpcSwitchedEndian (FILE * fp)
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

  if (machine != EM_SWITCHED_PPC)
    return FALSE;

  return TRUE;
}

void
ElfWritePpcSwitchedEndian (FILE * fp, t_object * obj)
{
  int tel = 0;
  Elf32_Ehdr hdr;
  STATUS (START, ("Writing Ppc binary"));
  hdr.e_ident[EI_MAG0] = ELFMAG0;
  hdr.e_ident[EI_MAG1] = ELFMAG1;
  hdr.e_ident[EI_MAG2] = ELFMAG2;
  hdr.e_ident[EI_MAG3] = ELFMAG3;
  hdr.e_ident[EI_VERSION] = EV_CURRENT;

#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
  hdr.e_ident[EI_DATA] = ELFDATA2MSB;
#else
  hdr.e_ident[EI_DATA] = ELFDATA2LSB;
#endif
  hdr.e_ident[EI_CLASS] = ELFCLASS32;

  /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
  for (tel = EI_PAD; tel < 16; tel++)
    {
      hdr.e_ident[tel] = 0;
    }
  hdr.e_machine = EM_PPC;
  hdr.e_version = EV_CURRENT;
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = 0;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;
  hdr.e_shnum = 0;
  hdr.e_shstrndx = 0;

  ElfWriteCommon32 (fp, &hdr, obj, TRUE, TRUE, &elf_ppc32_info);
  STATUS (STOP, ("Writing Ppc binary"));
}


/* Powerpc #ha() */

#define PPC_HA	"= i00008000 & s0001< + iffff0000 & "
#define PPC_LO  "sffff & "

#define WRITE_PPC_HALF16 "s0010 < l sffff & | w"
#define WRITE_PPC_HA_HALF16 "l sffff & | w"

void
ElfReadPpcSwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
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
  
  Elf32HdrSwitchEndian (&hdr32);

  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, TRUE, read_debug, &elf_ppc32_info);
 
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


              for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela));
                   tel2++)
              {
                Elf32RelaSwitchEndian (&rel[tel2]);
              }

	      for (tel2 = 0;
		   tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)

		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
		    case R_PPC_NONE:
		      break;
		    case R_PPC_ADDR14_BRNTAKEN:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "S00A00+\\l*w\\s0001$");
		      }
		      break;
		    case R_PPC_ADDR16_HA:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), 
					            addend, T_RELOCATABLE 
						    (corrsec), generic, sym, 
						    TRUE, NULL, NULL, NULL, 
						    "S00A00+" PPC_HA "\\" WRITE_PPC_HA_HALF16 "\\s0000$");
		      }

		      break;
		    case R_PPC_ADDR16_LO:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00A00+\\ " PPC_LO WRITE_PPC_HALF16 "\\s0000$");
		      }

		      break;
		    case R_PPC_REL24:	/* PC relative 26 bit */
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "S00A00+P-\\ i03fffffc & l ifc000003 & | w\\s0000$");
		      }

		      break;
		    case R_PPC_ADDR32:	/* 32bit absolute address */
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00A00+\\l*w\\s0000$");
		      }

		      break;
		    case R_PPC_LOCAL24PC:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "S00A00+P-\\i03fffffc & l ifc000003 & | w\\s0000$");
		      }

		      break;
		    case R_PPC_GOT16:
		      {
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_string tentative = 
				  "GotSection {\
				         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 4, 4) }\
					 section { RELOCATED32(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\l*w\\s0000$\") }\
				         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_PPC_GOT16_USE:\",MATCHED_NAME()))) >> ABS(16) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
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
			} 

			gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,PERHAPS,FALSE,
					
				OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj))	
					
					,SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);


			
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);
			
			gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringConcat2("DIABLO_PPC_GOT16_USE:GOT:",SYMBOL_NAME(sym)),"R00A00+$",10,TRUE,FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);


			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend,
			    T_RELOCATABLE (corrsec), generic, gotsym, TRUE, NULL, NULL, NULL, "S00A00+g-\\" PPC_LO WRITE_PPC_HALF16 "\\s0000$");
		      }

		      break;
		    case R_PPC_REL32:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

 			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00A00+P-\\l*w\\s0000$");
		      }

		      break;
		      
		    case R_PPC_PLTREL24:
		      {
			t_symbol *sym;
			t_address generic, addend;

			sym = table[ELF32_R_SYM(rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);
			addend = AddressNew32 (rel[tel2].r_addend);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, FALSE, NULL, NULL, NULL, "S00A00+P-\\ i03fffffc & l ifc000003 & | w\\s0000$");

		      }
		      break;

		    case R_PPC_ADDR24:	/* 26bit address, 2 bits ignored.  */
		    case R_PPC_ADDR16:	/* 16bit absolute address */

		      /* lower 16bit of absolute address */
		    case R_PPC_ADDR16_HI:	/* high 16bit of absolute address */
		    case R_PPC_ADDR14:	/* 16bit address, 2 bits ignored */
		    case R_PPC_ADDR14_BRTAKEN:
		    case R_PPC_REL14:	/* PC relative 16 bit */
		    case R_PPC_REL14_BRTAKEN:
		    case R_PPC_REL14_BRNTAKEN:
		    case R_PPC_GOT16_LO:
		    case R_PPC_GOT16_HI:
		    case R_PPC_GOT16_HA:
		    case R_PPC_COPY:
		    case R_PPC_GLOB_DAT:
		    case R_PPC_JMP_SLOT:
		    case R_PPC_RELATIVE:
		    case R_PPC_UADDR32:
		    case R_PPC_UADDR16:
		    case R_PPC_PLT32:
		    case R_PPC_PLTREL32:
		    case R_PPC_PLT16_LO:
		    case R_PPC_PLT16_HI:
		    case R_PPC_PLT16_HA:
		    case R_PPC_SDAREL16:
		    case R_PPC_SECTOFF:
		    case R_PPC_SECTOFF_LO:
		    case R_PPC_SECTOFF_HI:
		    case R_PPC_SECTOFF_HA:
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
#endif
