/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

#define sign_extend(x,p)      (((x) ^ (1 << (p))) - (1 << (p)))
#define SELECT_BITS(i,b,e) ((i&((1<<(b+1))-1))>>e)

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
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
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


t_bool
IsElfArmSwitchedEndianOnMsb (FILE * fp)
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

  if (machine != EM_SWITCHED_ARM)
    return FALSE;

  return TRUE;
}

t_bool
IsElfArmSwitchedEndianOnLsb (FILE * fp)
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

  if (machine != EM_SWITCHED_ARM)
    return FALSE;

  return TRUE;
}

void
ElfWriteArmSwitchedEndian (FILE * fp, t_object * obj)
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
  hdr.e_machine = EM_ARM;
  hdr.e_version = EV_CURRENT;
  VERBOSE(0, ("program entry point = @G", OBJECT_ENTRY (obj)));
  hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
  hdr.e_phoff = 0;
  hdr.e_flags = EF_ARM_HASENTRY | EF_ARM_SYMSARESORTED;
  hdr.e_ehsize = 0;
  /* Set in ElfWriteCommon... */
  hdr.e_phentsize = 0;
  hdr.e_phnum = 0;
  hdr.e_shentsize = 0;		/* Set in common */
  hdr.e_shnum = 0;		/* Set in common */
  hdr.e_shstrndx = 0;		/* Set in common */

  if (!strcmp (OBJECT_MAP (obj)->handler->name, "ADS_ARMLINK"))
    ElfWriteCommon32 (fp, &hdr, obj, FALSE, TRUE, &elf_arm_ads_info);
  else
    ElfWriteCommon32 (fp, &hdr, obj, TRUE, TRUE, &elf_arm_ads_info);
  STATUS (STOP, ("Writing arm binary"));
}

void
ElfReadArmSwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
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
  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }

  Elf32HdrSwitchEndian (&hdr32);
  
  if ((hdr32.e_flags & 0xFF000000)==0x01000000) abi=1;
  else if ((hdr32.e_flags & 0xFF000000)==0x02000000) abi=2;
  else if ((hdr32.e_flags & 0xFF000000)==0x03000000) abi=3;
  else if ((hdr32.e_flags & 0xFF000000)==0x04000000) abi=4;

  /* Reading elf headers does not depend on the type of objectfile */
  if (!strcmp (OBJECT_MAP (obj)->handler->name, "ADS_ARMLINK"))
    ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
         &sechdrstrtab, &sec_table, &table, &dynamic_table, TRUE, read_debug, &elf_arm_ads_info);
  else
    ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
         &sechdrstrtab, &sec_table, &table, &dynamic_table, TRUE, read_debug, &elf_arm_gnu_info);
  
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
        if (strncmp(SYMBOL_NAME(sym),"$Super$$",8)==0) FATAL(("Super symbol"));
      }
    }
  }


  OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));
  if (hdr32.e_shnum)
    {
      tel = 0;
      while (tel < hdr32.e_shnum)
	{
	  /* STEP 4b: handle architecture specific flags */


	  if ((shdrs[tel].sh_flags & SHF_ARM_COMDEF)
	      && (shdrs[tel].sh_flags & SHF_ALLOC))
	    {
	      t_section *sec = SectionGetFromObjectByName (obj,
							   sechdrstrtab +
							   shdrs[tel].
							   sh_name);
	      if (!sec)
		FATAL (("Common defined section %s not found!",
			sechdrstrtab + shdrs[tel].sh_name));
	      else
		SECTION_SET_IS_COMMON_DEFINED (sec, TRUE);
	    }


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
                  case R_ARM_COPY:
                    printf("ARM_COPY!\n");
                     SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "ARM_COPY", "S00$", 10, PERHAPS, FALSE, T_RELOCATABLE(sec_table[tel]), AddressNew32(tel2 * sizeof (Elf32_Rel)), AddressNew32(0), NULL, AddressNew32(0), 0);
                    break;
                  default:
                    break;
                    }
                }

            }
	  else
	    if (StringPatternMatch
		(".rel*", sechdrstrtab + shdrs[tel].sh_name))
	    {
	      Elf32_Rel *rel = Malloc (shdrs[tel].sh_size);
	      t_uint32 tel2;
	      t_section *corrsec;
	      fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset,
		     SEEK_SET);
	      IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
	      corrsec = sec_table[shdrs[tel].sh_info];

	      for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel));
		   tel2++)
		{
		  Elf32RelSwitchEndian (&rel[tel2]);
		}
	      for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rel));
		   tel2++)
		{
		  ASSERT (symbol_table
			  && strtab,
			  ("Symtab or stringtab not found. Implement me"));
		  switch (ELF32_R_TYPE (rel[tel2].r_info))
		    {
		    case R_ARM_NONE:
		      break;
		    case R_ARM_PLT32:
		    case R_ARM_PC24:
		      {
			t_uint32 data;
			t_symbol *sym;
			t_address generic;
			t_address addend;

			sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			generic = AddressNew32 (rel[tel2].r_offset);

			data = SectionGetData32 (corrsec, generic);

			data = Uint32SelectBits (data, 23, 0);
			data = Uint32SignExtend (data, 23);
			data <<= 2;
			addend = AddressNew32 (data);


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
		    case R_ARM_ABS32:
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
						    CALC_ABS "S00M+\\"
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
			if (SECTION_TYPE (corrsec) != CODE_SECTION)
			  {
			    FATAL (("REL 32 not in code section!"));
			  }
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);

			if (abi!=0)
			{
                          RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "u00?s0000A00+:S00P-A00+!" "S00M+\\" WRITE_32);
			}
			else
                          RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00P-A00+" "S00M+\\" WRITE_32);
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
		      {
			t_uint32 data;
			t_address generic;
			t_address addend;
			t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
			t_symbol *gotuse, *gotsym; 
			t_string tentative = 
				  "GotSection {\
				         action  { ADD_SUBSECTION(\"Linker\", \".got\", CONCAT(\"GOTELEM:\",MATCHED_NAME()), RODATA, 4, 4) }\
					 section { RELOCATED32(0x0,SUBSTRING(MATCHED_NAME(),4,0),0,0,0,\"S00A00+\\l*w\\s0000$\") }\
				         address { READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_ARM_GOT32_USE:\",MATCHED_NAME()))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
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
			data = SectionGetData32 (corrsec, generic);
			addend = AddressNew32 (data);
			
			gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), StringConcat2("DIABLO_ARM_GOT32_USE:GOT:", SYMBOL_NAME(sym)), "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    T_RELOCATABLE (corrsec),
						    generic, gotsym, TRUE, NULL,
						    NULL, NULL,
						    "S00A00+g-" "\\" WRITE_32
						    );
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

			data = SectionGetData16 (corrsec, generic);
			tmp = SELECT_BITS (data, 10, 0);
			tmp <<= 11;
			tmp = sign_extend (tmp, 21);
			data = SectionGetData16 (corrsec, AddressAddUint32(generic, 2));
			data = SELECT_BITS (data, 10, 0);
			data |= tmp;
			data <<= 1;
			addend = AddressNew32 (data);

			RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
						    addend,
						    T_RELOCATABLE (corrsec),
						    generic, sym, FALSE, NULL,
						    NULL, NULL,
						    "S00S00M?P:Pifffffffc&!-A00+\\s0001 > = i003ff800 & s000b > % i000007ff & s0010 < | l if800f800 & | S00M?:iefffffff&! w\\s0000$"
						    );
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
						    "S00A00+g-\\" WRITE_32
						    );
		      }
		      break;
		    case R_ARM_PC13:
		      FATAL (("Implement ARMPC13!"));
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
		    default:
		      FATAL (("Implement %d!",
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
