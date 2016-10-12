/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef BIT32ADDRSUPPORT

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

/* IsElfPpcSameEndian {{{*/
t_bool
IsElfPpcSameEndian (FILE * fp)
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

  if (machine != EM_PPC)
    return FALSE;

  return TRUE;
}
/*}}}*/

/* ElfWritePpcSameEndian {{{*/
void
ElfWritePpcSameEndian (FILE * fp, t_object * obj)
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

  ElfWriteCommon32 (fp, &hdr, obj, TRUE, FALSE, &elf_ppc32_info);
  STATUS (STOP, ("Writing Ppc binary"));
}/*}}}*/

/* Relocation Macros {{{ */
#define PPC_HA	"= i00008000 & s0001< + iffff0000 & "
#define PPC_LO  "sffff & "
#define WRITE_PPC_HALF16 "s0010 < l sffff & | w"
#define WRITE_PPC_HA_HALF16 "l sffff & | w"
/* }}} */

/* ElfReadPpcSameEndian {{{*/
#define _NewDynReloc(name)                                      \
do {                                                        \
t_symbol *sym = dynamic_table[ELF32_R_SYM(rel[tel2].r_info)];     \
t_string symname = StringConcat2 (name, SYMBOL_NAME (sym)); \
SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj),        \
symname,        \
"R00$", 10, TRUE, FALSE,                     \
T_RELOCATABLE (sec_table[tel]),                 \
AddressNew32 (tel2 * sizeof (Elf32_Rela)),      \
AddressNew32(0), NULL, AddressNew32(0), 0);        \
Free(symname);                                  \
} while(0)


void
ElfReadPpcSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf32_Shdr *shdrs = NULL;

  t_symbol **table = NULL;
  Elf32_Sym *symbol_table = NULL;

  t_symbol **dynamic_table = NULL;
  Elf32_Sym *dynamic_symbol_table = NULL;

  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  int tel;
  char *sechdrstrtab = NULL;
  Elf32_Ehdr hdr32;

  if (fread ((&hdr32), sizeof (Elf32_Ehdr), 1, fp) != 1)
  {
    FATAL (("Could not read elf header!"));
  }

  /* Reading elf headers does not depend on the type of objectfile */
  ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
      &sechdrstrtab, &sec_table, &table, &dynamic_table, FALSE, read_debug, &elf_ppc32_info);

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
        else if ((StringPatternMatch (".rela.dyn", sechdrstrtab + shdrs[tel].sh_name))
                 || (StringPatternMatch (".rela.plt", sechdrstrtab + shdrs[tel].sh_name)))
        {
          if (shdrs[tel].sh_size)
          {
            Elf32_Rela *rel = Malloc (shdrs[tel].sh_size);
            t_uint32 tel2;
            
            fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
            IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
            
            for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)
            {
              switch (ELF32_R_TYPE (rel[tel2].r_info))
              {
                case R_PPC_JMP_SLOT:
                  printf("DYNAMIC RELOC! %s",  SYMBOL_NAME(dynamic_table[ELF32_R_SYM(rel[tel2].r_info)]));
                  _NewDynReloc("PPC_JMP_SLOT:");
                  break;
                case R_PPC_GLOB_DAT:
                  printf("DYNAMIC RELOC! %s",  SYMBOL_NAME(dynamic_table[ELF32_R_SYM(rel[tel2].r_info)]));
                  _NewDynReloc("PPC_GLOB_DAT:");
                  break;
                case R_PPC_COPY:
                  printf("DYNAMIC RELOC! %s",  SYMBOL_NAME(dynamic_table[ELF32_R_SYM(rel[tel2].r_info)]));
                  _NewDynReloc("PPC_COPY:");
                  break;
                default:
                  FATAL (("Implement dynamic RELOC_TYPE %d",
                          ELF32_R_TYPE (rel[tel2].r_info)));
              }
            }
            Free(rel);
          }
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
                case R_PPC_PLTREL24:
                  {
                    t_symbol *sym;
                    t_address generic, addend;
                    t_object *main_object = OBJECT_PARENT(obj)?OBJECT_PARENT(obj):obj;
                    t_section * sec= OBJECT_UNDEF_SECTION(main_object);
                    t_string jmpslotname;

                    sym = table[ELF32_R_SYM(rel[tel2].r_info)];
                    jmpslotname = StringConcat2("PPC_JMP_SLOT:",SYMBOL_NAME(sym));

                    /* undefined targets with an R_PPC_JMP_SLOT relocation have to be redirected
                     * to the plt. R_PPC_PLTREL24's always have to be redirected to the plt.
                     * We cannot use the check for SYMBOL_ORDER=-1 (= check whether the symbol
                     * is local), because the symbol order for symbols in the *UNDEF* section
                     * does not follow this rule)
                     */
                    if (((ELF32_R_TYPE(rel[tel2].r_info) == R_PPC_PLTREL24) ||
                         (SYMBOL_BASE(sym)==T_RELOCATABLE(sec))) &&
                        (SymbolTableLookup(OBJECT_SYMBOL_TABLE(main_object),jmpslotname)!=NULL))
                    {
                      t_string name = StringConcat2("JMP_SLOT:", SYMBOL_NAME(sym)); 
                      sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$" , 0, PERHAPS, TRUE, T_RELOCATABLE(sec), AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                      Free(name);
                    }
                    else
                    {
                      /* for some reason there are sometimes R_PPC_PLTREL24 relocations which do not refer to plt entries, e.g.
                         the call to _init from __libc_csu_init in libc_nonshared.a (it's replaced by a regular branch by ld)
                         ASSERT((ELF32_R_TYPE(rel[tel2].r_info) != R_PPC_PLTREL24),("R_PPC_PLTREL24 relocation but not referring to recognised plt entry, can't find %s",jmpslotname)); */
                    }
                    Free(jmpslotname);

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
                    t_symbol *sym = table[ELF32_R_SYM (rel[tel2].r_info)];
                    t_address generic = AddressNew32 (rel[tel2].r_offset);
                    t_address addend = AddressNew32 (rel[tel2].r_addend);
                    t_symbol *gotuse, *gotsym; 
                    t_string name;
                    t_string tentative = 
                    "GotSectionWithGlobDat {\
                        trigger { LINKED_SYMBOL_EXISTS(CONCAT(\"PPC_GLOB_DAT:\", SUBSTRING(MATCHED_NAME(),8,0)))}\
                        action  { ADD_SUBSECTION(\"Linker\", \".got\", MATCHED_NAME(), DATA, 4, 4) }\
                        section { RELOCATED32(0x0,CONCAT(\"GOTREL:\",SUBSTRING(MATCHED_NAME(),8,0)),0,CONCAT(\"DYNSYM:\", SUBSTRING(MATCHED_NAME(),8,0)),0,\"S00*S01*\\\\s0000$\") }\
                        address { SIGN_EXTEND(15,(READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_PPC_GOT16_USE:\",MATCHED_NAME()))) >> ABS(16))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
                     }\
                     GotSectionWithoutGlobDat {\
                        trigger { !LINKED_SYMBOL_EXISTS(CONCAT(\"PPC_GLOB_DAT:\", SUBSTRING(MATCHED_NAME(),8,0)))}\
                        action  { ADD_SUBSECTION(\"Linker\", \".got\", MATCHED_NAME(), DATA, 4, 4) }\
                        section { CONST32(0x0) }\
                        address { SIGN_EXTEND(15,(READ_LINKED_VALUE32(SUBSYMBOL(CONCAT(\"DIABLO_PPC_GOT16_USE:\",MATCHED_NAME()))) >> ABS(16))) + SYMBOL(\"_GLOBAL_OFFSET_TABLE_\") }\
                     }\
                     GotSymbol {\
                        action { ADD_SYMBOL_NEW(MATCHED_NAME(), 12, MATCHED_SYMBOL_FLAGS()) }\
                        symbol { START_OF_SUBSECTION(\"Linker\",MATCHED_NAME()) }\
                     }";
                    
                    if (SYMBOL_ORDER(sym) == -1)
                    {
                      t_string new_name=StringIo("got_global_for_local_symbol_%s_%d_from_%s",SYMBOL_NAME(sym),ELF32_R_SYM (rel[tel2].r_info),OBJECT_NAME(obj));
                      sym = SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), new_name, SYMBOL_CODE(sym), 5, PERHAPS, FALSE, SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), NULL, SYMBOL_SIZE(sym), 0);
                    } 

                    /* symbol for the GOT entry */
                    name=StringConcat2("GOTELEM:",SYMBOL_NAME(sym));
                    gotsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),name,"R00A00+$",5,PERHAPS,FALSE, OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),SYMBOL_OFFSET_FROM_START(sym),SYMBOL_ADDEND(sym), tentative, SYMBOL_SIZE(sym), 0);
                    Free(name);

                    /* symbol for the use of the GOT entry */
                    name=StringConcat2("DIABLO_PPC_GOT16_USE:GOTELEM:",SYMBOL_NAME(sym));
                    gotuse = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),name,"R00A00+$",5,TRUE,FALSE, T_RELOCATABLE(corrsec), generic, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                    Free(name);

                    /* relocation for the use of the GOT entry */
                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, gotsym, TRUE, NULL, NULL, NULL, "S00A00+g-\\" PPC_LO WRITE_PPC_HALF16 "\\s0000$");
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

                case R_PPC_GLOB_DAT:
                  {
                    t_symbol *sym;
                    t_address generic, addend;

                    sym = table[ELF32_R_SYM(rel[tel2].r_info)];
                    generic = AddressNew32 (rel[tel2].r_offset);
                    addend = AddressNew32 (rel[tel2].r_addend);

                    VERBOSE(0,("R_PPC_GLOB_DAT relocation found!"));
                    VERBOSE(0,("Corrsec=%x",T_RELOCATABLE(corrsec)));
                    VERBOSE(0,("Symvol=@S",sym));
                    VERBOSE(0,("Generic=@G",generic));
                    VERBOSE(0,("Addend=@G",addend));

                    RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), addend, T_RELOCATABLE (corrsec), generic, sym, TRUE, NULL, NULL, NULL, "S00A00+\\l*w\\s0000$");
                  }

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
  if (dynamic_table)
    Free (dynamic_table);
  if (shdrs)
    Free (shdrs);
  if (sechdrstrtab)
    Free (sechdrstrtab);
  if (symbol_table)
    Free (symbol_table);
  if (dynamic_symbol_table)
    Free (dynamic_symbol_table);
  if (strtab)
    Free (strtab);
  if (sec_table)
    Free (sec_table);
}/*}}}*/


/* Ppc32CheckPltSize {{{*/
extern t_object *parser_object;

void *
Ppc32CheckPltSize(t_ast_successors *succ, void *data)
{
  t_object *obj = parser_object;
  t_symbol *initpltsym, *sym, *lastsym, *fillersym;
  t_address lastsymaddr;
  t_section *pltsec, *fillersec;
  int npltentries;

  initpltsym =
    SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "INITPLT");
  if (!initpltsym)
  {
    /* no .plt section -> not a dynamically linked binary */
    return NULL;
  }
  pltsec = SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE(initpltsym)));

  ASSERT(pltsec, ("Could not find .plt section"));

  /* {{{ Verify the plt section size and find the last plt entry */
  /* in case the plt section has 2^13 entries or more, entry 2^13 and following
   * need to become 16 instead of 8 bytes (the first 72 bytes are just filler)
   * We don't support this yet, so give an error in that case.
   */
  npltentries=0;
  lastsym=NULL;
  lastsymaddr=AddressNullForObject(obj);
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj),sym)
  {
    if (StringPatternMatch("JMP_SLOT:*", SYMBOL_NAME(sym)))
    {
      npltentries++;
      /* record the last plt symbol */
      if (AddressIsGt(
         AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym)),
           lastsymaddr))
      {
        lastsymaddr=AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym));
        lastsym=sym;
      }
    }
  }
  VERBOSE(1,("Original .plt section: @T, size: @G",T_RELOCATABLE(pltsec),SECTION_CSIZE(pltsec)));
  VERBOSE(1,("Number of plt entries: %d\n",npltentries));
  ASSERT(npltentries < (1<<13),("PLT section has 2^13 entries or more, not yet supported"));
  ASSERT(AddressIsEq(SECTION_CSIZE(pltsec),AddressNewForObject(obj,72+npltentries*12)),("Some .plt entries are unaccounted for or counted double, size is 0x%x but should be 0x%x!",72+npltentries*12,SECTION_CSIZE(pltsec)));
  /* }}} */

  /* {{{ Add a filler for the rest of the plt section size */
  /* All plt entries (before nr 1<<13) consist of 12 byte entries
   * The first 8 bytes of these entries are added in the linker script.
   * The last 4 bytes have to be placed together after all those 8-byte stubs.
   */
  VERBOSE(1,("Last .plt entry (at @G): @S",lastsymaddr,lastsym));
  fillersec = SectionCreateForObject(ObjectGetLinkerSubObject(parser_object),DATA_SECTION,
                                     pltsec,
                                     AddressNewForObject(ObjectGetLinkerSubObject(parser_object),npltentries*4),
                                     "PLTFILLERSEC");
  SECTION_SET_OLD_ADDRESS(fillersec, AddressAdd(lastsymaddr,
                                       SECTION_CSIZE(T_SECTION(SYMBOL_BASE(lastsym)))));
  SECTION_SET_CADDRESS(fillersec, SECTION_OLD_ADDRESS(fillersec));
  VERBOSE(1,("Added .plt filler subsection of size @G: @T",SECTION_CSIZE(fillersec),fillersec));
  /* add PLTFILLER symbol to start of fillersec */
  fillersym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      "PLTFILLER", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(fillersec),
      AddressNullForObject(obj), AddressNullForObject(obj),
      NULL, AddressNullForObject(obj), 0);

  /* add reference from INITPLT section to filler so it won't be killed */
  RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                             AddressNullForObject(obj),
                             SYMBOL_BASE(initpltsym),
                             AddressNullForObject(obj),
                             fillersym,
                             FALSE,
                             NULL,
                             NULL,
                             NULL,
                             "S00\\*\\s0000$");
  /* }}} */
  return NULL;
}
/*}}}*/

#endif

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab: */
