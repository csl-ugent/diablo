/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/*
 * Copyright (C) 2007 Lluis Vilanova <xscript@gmx.net> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * }}}
 */

#ifdef BIT32ADDRSUPPORT

#include <diabloelf.h>

static const Elf32_HeaderInfo elf_spe_info =
{
  0x80,         /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  (Elf32_Word)-1, /*  Elf32_Word pltentsize; */
  0x80,           /*  Elf32_Word ptload_align; */
  (Elf32_Word)-1, /*  Elf32_Word ptdynamic_align; */
  (Elf32_Word)-1, /*  Elf32_Word ptinterp_align; */
  0x10,           /*  Elf32_Word ptnote_align; */
  (Elf32_Word)-1, /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
};


/* IsElfSpe* {{{ */
static t_bool
IsElfSpeAnyEndian (FILE * fp, t_bool same_endian)
{
    Elf32_Byte buffer[EI_NIDENT];
    Elf32_Ehdr hdr;
    Elf32_Half e_machine;
    t_uint32 save = ftell (fp);

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
            return FALSE;
        case ELFDATA2MSB:
            if (!same_endian) return FALSE;
            break;
        case ELFDATA2LSB:
            if (same_endian) return FALSE;
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
            e_machine = hdr.e_machine;
            break;
        case ELFCLASS64:
        case ELFCLASSNONE:
        default:
            return FALSE;
            break;
    }

    if (same_endian && e_machine != EM_SPE)
        return FALSE;
    else if (!same_endian && e_machine != EM_SWITCHED_SPE)
        return FALSE;

    return TRUE;
}

t_bool
IsElfSpeSameEndian (FILE * fp)
{
#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
    return FALSE;
#endif
    return IsElfSpeAnyEndian (fp, TRUE);
}

t_bool
IsElfSpeSwitchedEndian (FILE * fp)
{
#ifdef DIABLOSUPPORT_WORDS_BIGENDIAN
    return FALSE;
#endif
    return IsElfSpeAnyEndian (fp, FALSE);
}
/* }}} */

/* ElfReadSpe* {{{ */
#define _NewReloc(calcstr)                                      \
    do {                                                        \
        t_symbol *sym;                                          \
        t_address generic, addend;                              \
                                                                \
        sym = table[ELF32_R_SYM(rel[tel2].r_info)];             \
        generic = AddressNew32 (rel[tel2].r_offset);            \
        addend = AddressNew32 (rel[tel2].r_addend);             \
                                                                \
        RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),   \
                addend, T_RELOCATABLE (corrsec), generic, sym,  \
                TRUE, NULL, NULL, NULL,                         \
                calcstr);                                       \
    } while(0)

static void
ElfReadSpeAnyEndian (FILE *fp, t_object *obj, t_bool read_debug,
        t_bool same_endian)
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
    ElfReadCommon32 (fp, &hdr32, obj, &shdrs, &symbol_table,
            &dynamic_symbol_table, &numsyms, &strtab, &sechdrstrtab, &sec_table,
            &table, &dynamic_table, !same_endian, read_debug, &elf_spe_info);

    OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));

    if (hdr32.e_shnum)
    {
        tel = 0;
        while (tel < hdr32.e_shnum)
        {
            /* Find the relocations */
            if (StringPatternMatch (".rela.debug*", sechdrstrtab + shdrs[tel].sh_name))
            {
                /* if we ignore debug info, we can also ignore debug relocations */
            }
            else if (StringPatternMatch (".rela.stab*", sechdrstrtab + shdrs[tel].sh_name))
            {
                /* Lets ignore these */
            }
            else if (StringPatternMatch (".rela*", sechdrstrtab + shdrs[tel].sh_name))
            {
                Elf32_Rela *rel = Malloc (shdrs[tel].sh_size);
                t_uint32 tel2;
                t_section *corrsec;
                fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
                IGNORE_RESULT(fread (rel, shdrs[tel].sh_size, 1, fp));
                corrsec = sec_table[shdrs[tel].sh_info];

                for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf32_Rela)); tel2++)
                {
                    switch (ELF32_R_TYPE (rel[tel2].r_info))
                    {
                        case R_SPU_NONE:
                            break;
                        /*
                           R_SPU_ADDR10    1  I10*   (S + A) >> 4     lqd $3, symbol($4)
                         */
                        case R_SPU_ADDR16:
                            _NewReloc("S00A00+ \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ = i0003fffc & - $");
                            break;
                        case R_SPU_ADDR16_HI: /* 3  I16    #hi(S + A)       ilhu $3, symbol@h */
                            _NewReloc("S00A00+ \\ s0010 > i0000ffff & s0007 < l iff80007f & | w \\ s0000 $");
                          break;
                        case R_SPU_ADDR16_LO: /* 4  I16    #lo(S + A)       iohl $3, synbol@l */
                          _NewReloc("S00A00+ \\ i0000ffff & s0007 < l iff80007f & | w \\ s0000 $");
                          break;
                        case R_SPU_ADDR18:
                            _NewReloc("S00A00+ \\ = i0003ffff & s0007 < l ife00007f & | w \\ = i0003ffff & - $");
                            break;
                        case R_SPU_GLOB_DAT:
                            _NewReloc("S00A00+ \\ l*w \\ s0000$");
                            break;
                        case R_SPU_REL16:
                            _NewReloc("S00A00+ P- \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ ifffc0003 & = ? ifffc0000 - :! $");
                            /* The above relocation expression is actually
                             * incorrect according to the SPU ABI spec. However,
                             * the correct relocation expression, given below,
                             * turns out not to work in practice. This is
                             * because sometimes the computed offset is
                             * apparently interpreted as an unsigned value, for
                             * example in lqr instructions.
                             *
                             * _NewReloc("S00A00+ P- \\ = s0002 > i0000ffff & s0007 < l iff80007f & | w \\ = i00020000 & ? ifffc0003 & ifffc0000 - : ifffc0003 & ! $");*/
                            break;
                        case R_SPU_ADDR7:
                            _NewReloc("S00A00+ \\ s007f & s000e < l iffe03fff & | w \\ s0000$");
                            break;
                        /*
                           R_SPU_REL9      9  I9*    (S + A - P) >> 2 hbra function, -100
                           R_SPU_REL9I     10 I9I*   (S + A - P) >> 2 hbr function, $3
                           R_SPU_ADDR10I   11 I10*   S+A              ai $3, $3, symbol
                         */
                        default:
                            FATAL (("Implement RELOC_TYPE %d",
                                        ELF32_R_TYPE (rel[tel2].r_info)));
                    }
                }
                Free (rel);
            }
            else if (StringPatternMatch (".rel*", sechdrstrtab + shdrs[tel].sh_name))
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
}

void
ElfReadSpeSameEndian (FILE *fp, t_object *obj, t_bool read_debug)
{
    ElfReadSpeAnyEndian (fp, obj, read_debug, TRUE);
}

void
ElfReadSpeSwitchedEndian (FILE *fp, t_object *obj, t_bool read_debug)
{
    ElfReadSpeAnyEndian (fp, obj, read_debug, FALSE);
}
/* }}} */

/* ElfWriteSpe* {{{ */
static void
ElfWriteSpeAnyEndian (FILE * fp, t_object * obj, t_bool same_endian)
{
    int tel = 0;
    Elf32_Ehdr hdr;

    STATUS (START, ("Writing SPE binary"));

    hdr.e_ident[EI_MAG0] = ELFMAG0;
    hdr.e_ident[EI_MAG1] = ELFMAG1;
    hdr.e_ident[EI_MAG2] = ELFMAG2;
    hdr.e_ident[EI_MAG3] = ELFMAG3;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    /* \TODO Need to use DIABLOSUPPORT_WORDS_BIGENDIAN ? */
    hdr.e_ident[EI_DATA] = ELFDATA2MSB;
    hdr.e_ident[EI_CLASS] = ELFCLASS32;

    /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
    for (tel = EI_PAD; tel < 16; tel++)
    {
        hdr.e_ident[tel] = 0;
    }
    hdr.e_machine = same_endian ? EM_SPE : EM_SWITCHED_SPE;
    hdr.e_version = EV_CURRENT;
    hdr.e_entry = G_T_UINT32 (OBJECT_ENTRY (obj));
    hdr.e_phoff = 0;
    hdr.e_flags = 0;
    hdr.e_ehsize = sizeof (Elf32_Ehdr);
    /* Set in ElfWriteCommon... */
    hdr.e_phentsize = 0;
    hdr.e_phnum = 0;
    hdr.e_shentsize = 0;
    hdr.e_shnum = 0;
    hdr.e_shstrndx = 0;

    /* SPU binaries have no .plt section */
    ElfWriteCommon32 (fp, &hdr, obj, FALSE, !same_endian, &elf_spe_info);

    STATUS (STOP, ("Writing SPE binary"));
}

void
ElfWriteSpeSameEndian (FILE * fp, t_object * obj)
{
    ElfWriteSpeAnyEndian (fp, obj, TRUE);
}

void
ElfWriteSpeSwitchedEndian (FILE * fp, t_object * obj)
{
    ElfWriteSpeAnyEndian (fp, obj, FALSE);
}
/* }}} */

/* In case there are overlays, we need to add two linker-created subsections to
 * the binary: 
 *  .stubs contains call stubs that allow for overlay cross-calling
 *  .ovtab contains a table detailing the overlays
 * We also have to add some symbols to the .ovtab section
 */
extern t_object *parser_object;
static t_symbol *create_intermediate_sym(t_symbol *sym, t_uint32 offset, t_section *stub, t_object *obj)
{
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_symbol *msym = SYMBOL_MAPPED(sym);
  t_symbol *imsym;
  t_symbol *unsym;
  t_string name;

  ASSERT(msym, ("should be mapped: @S", sym));

  /* create intermediate symbol */  
  name = StringConcat2("_stub_", SYMBOL_NAME(sym));
  imsym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      name, SYMBOL_CODE(msym), SYMBOL_ORDER(msym), SYMBOL_DUP(msym),
      SYMBOL_SEARCH(msym), T_RELOCATABLE(stub), AddressNew32(offset),
      AddressNew32(0), SYMBOL_TENTATIVE(msym), SYMBOL_SIZE(msym), 0);
  Free(name);  

  /* add undefined symbol to the linker subobject */
  unsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(linker),
      SYMBOL_NAME(sym), SYMBOL_CODE(sym), SYMBOL_ORDER(sym),
      SYMBOL_DUP(sym), SYMBOL_SEARCH(sym), SYMBOL_BASE(sym),
      SYMBOL_OFFSET_FROM_START(sym), SYMBOL_ADDEND(sym), 
      SYMBOL_TENTATIVE(sym), SYMBOL_SIZE(sym), 0);

  /* attach relocation to the stub entry */
  RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(linker),
      AddressNew32(0), T_RELOCATABLE(stub), AddressNew32(offset),
      unsym, TRUE, NULL, NULL, NULL,
      "S00A00+\\= i0000ffff & s0007 < l iff80007f & | w\\= i0003fffc & - $");

  SYMBOL_SET_MAPPED(unsym, msym);

  return imsym;
}

void *SpeAddOverlayStubs(t_ast_successors *succ, void *data)
{
  t_object *obj = parser_object;
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_map *map = OBJECT_MAP(obj);
  t_section *parent, *stub;
  t_uint32 soffset, start, end;
  t_symbol **intermediates;
  t_object *subobj, *tmpobj;
  t_symbol *startsym, *endsym, *bufstartsym, *bufendsym;
  t_address ovtab_size, ovtab_buf_size;
  t_section *ovtab_buf, *ovtab;

  /* {{{ copy the .stub section from the final binary */
  t_map_node *mnode, *tmpnode;
  DLIST_FOREACH_NODE(map, mnode, tmpnode)
  {
    if (!strcmp(mnode->sec_name, ".stub") &&
        !strcmp(mnode->mapped_sec_name, ".text"))
      break;
  }
  if (!mnode)
    FATAL(("Could not find the stubs section in the map"));
  
  parent = SectionGetFromObjectByName(obj, ".text");
  ASSERT(parent, ("Could not find the .text section in the parent object"));
  stub = SectionCreateForObject(linker, CODE_SECTION, parent,
      mnode->sec_size, ".stub");
  SECTION_SET_CADDRESS(stub, mnode->base_addr);
  SECTION_SET_OLD_ADDRESS(stub, mnode->base_addr);

  soffset = AddressExtractUint32(AddressSub(mnode->base_addr,
        SECTION_OLD_ADDRESS(parent)));
  memcpy(SECTION_DATA(stub), ((char *)SECTION_DATA(parent)) + soffset,
      AddressExtractUint32(mnode->sec_size));

  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      "$a", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(stub),
      AddressNullForObject(obj), AddressNullForObject(obj),
      NULL, AddressNullForObject(obj), 0);
  /* }}} */

  /* {{{ add relocations to the .stub section */
  start = AddressExtractUint32(mnode->base_addr);
  end = AddressExtractUint32(AddressAdd(mnode->base_addr, mnode->sec_size));
  intermediates = Calloc((end-start)/4, sizeof(t_symbol *));

  /* walk over the linked binary to find all redirected relocations */  
  OBJECT_FOREACH_SUBOBJECT(obj, subobj, tmpobj)
  {
    t_reloc *rel;
    OBJECT_FOREACH_RELOC(subobj, rel)
    {
      t_relocatable *from = RELOC_FROM(rel);
      t_section* fromsec;
      t_uint32 data;

      ASSERT(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_SUBSECTION,
          ("expected only subsections here"));
      fromsec = T_SECTION(from);

      data = 
        SectionGetData32(SECTION_PARENT_SECTION(fromsec),
            AddressAdd(RELOC_FROM_OFFSET(rel),
              AddressSub(SECTION_OLD_ADDRESS(fromsec),
                SECTION_OLD_ADDRESS(SECTION_PARENT_SECTION(fromsec)))));

      if (SECTION_TYPE(fromsec) == CODE_SECTION)
      {
        /* disassemble instruction and reverse engineer relocation result */
        if ((data & 0xfe800000) == 0x32000000)
        {
          /* br[sl] <destination> */
          t_uint32 offset = ((data & 0x007fff80) >> 7) << 2;
          t_uint32 dest;
          t_symbol *sym;
          t_uint32 entry;

          if (offset & 0x20000) offset |= 0xfffc0000;

          dest = offset +
            AddressExtractUint32(AddressAdd(SECTION_OLD_ADDRESS(fromsec),
                  RELOC_FROM_OFFSET(rel)));

          if (dest < start || dest >= end)
            continue;

          ASSERT(RELOC_N_TO_SYMBOLS(rel) == 1,
              ("unexpectedly complex relocation @R", rel));

          sym = RELOC_TO_SYMBOL(rel)[0];
          entry = (dest - start) / 4;

          if (!intermediates[entry]) 
          {
            intermediates[entry] = create_intermediate_sym(sym, entry*4, stub, obj);
          }

          /* redirect relocation through intermediate symbol */
          SYMBOL_SET_MAPPED(sym, intermediates[entry]);
        }
      }
      else
      {
        t_symbol *sym;
        t_uint32 entry;

        /* data */
        if (data < start || data >= end)
          continue;

        ASSERT(RELOC_N_TO_SYMBOLS(rel) == 1,
            ("unexpectedly complex relocation @R", rel));

        sym = RELOC_TO_SYMBOL(rel)[0];
        entry = (data - start) / 4;
        if (!intermediates[entry]) 
        {
          intermediates[entry] = create_intermediate_sym(sym, entry*4, stub, obj);
        }

        /* redirect relocation through intermediate symbol */
        SYMBOL_SET_MAPPED(sym, intermediates[entry]);
      }
    }
  }
  Free(intermediates);
  /* }}} */

  /* create the .ovtab section */
  /* {{{ get the ovtab symbols from the linked binary */  
  startsym =
    SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), "_ovly_table");
  endsym =
    SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), "_ovly_table_end");
  bufstartsym =
    SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), "_ovly_buf_table");
  bufendsym =
    SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), "_ovly_buf_table_end");
  ASSERT(startsym && endsym && bufstartsym && bufendsym,
      ("Could not find .ovtab symbols"));
  /* }}} */

  /* {{{ copy the .ovtab section from the final binary */
  DLIST_FOREACH_NODE(map, mnode, tmpnode)
  {
    if (!strcmp(mnode->sec_name, ".ovtab") &&
        !strcmp(mnode->mapped_sec_name, ".data"))
      break;
  }
  if (!mnode)
    FATAL(("Could not find the .ovtab section in the map"));

  

  ovtab_size =
    AddressSub(SYMBOL_OFFSET_FROM_START(endsym),
      SYMBOL_OFFSET_FROM_START(startsym));
  ovtab_buf_size =
    AddressSub(SYMBOL_OFFSET_FROM_START(bufendsym),
      SYMBOL_OFFSET_FROM_START(bufstartsym));
  
  parent = SectionGetFromObjectByName(obj, ".data");
  ASSERT(parent, ("Could not find the .data section in the parent object"));
  ovtab = SectionCreateForObject(linker, DATA_SECTION, parent,
      ovtab_size, ".ovtab");
  SECTION_SET_CADDRESS(ovtab, mnode->base_addr);
  SECTION_SET_OLD_ADDRESS(ovtab, mnode->base_addr);
  soffset = AddressExtractUint32(AddressSub(mnode->base_addr,
        SECTION_OLD_ADDRESS(parent)));
  memcpy(SECTION_DATA(ovtab), ((char *)SECTION_DATA(parent)) + soffset,
      AddressExtractUint32(ovtab_size));

  ovtab_buf = SectionCreateForObject(linker, DATA_SECTION, parent,
      ovtab_buf_size, ".ovtab_buf");
  SECTION_SET_CADDRESS(ovtab_buf, AddressAdd(mnode->base_addr, ovtab_size));
  SECTION_SET_OLD_ADDRESS(ovtab_buf, AddressAdd(mnode->base_addr, ovtab_size));
  soffset += AddressExtractUint32(ovtab_size);
  memcpy(SECTION_DATA(ovtab), ((char *)SECTION_DATA(parent)) + soffset,
      AddressExtractUint32(ovtab_buf_size));
  /* }}} */

  /* {{{ add symbols to the .ovtab section */
  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      "_ovly_table", "R00A00+$", 10, FALSE, FALSE, T_RELOCATABLE(ovtab),
      AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);
  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      "_ovly_table_end", "R00A00+Z00+$", 10, FALSE, FALSE, T_RELOCATABLE(ovtab),
      AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);
  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
      "_ovly_buf_table", "R00A00+$", 10, FALSE, FALSE, T_RELOCATABLE(ovtab_buf),
      AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);
  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), "_ovly_buf_table_end",
      "R00A00+Z00+$", 10, FALSE, FALSE, T_RELOCATABLE(ovtab_buf),
      AddressNew32(0), AddressNew32(0), NULL, AddressNew32(0), 0);
  /* }}} */
  return NULL;
}

#endif

/* vim:set ts=2 sw=2 tw=80 foldmethod=marker expandtab: */
