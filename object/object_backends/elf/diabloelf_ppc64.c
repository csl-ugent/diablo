/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/*
 * Copyright (C) 2006 Lluis Vilanova <xscript@gmx.net> {{{
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

#ifdef BIT64ADDRSUPPORT

#include <diabloelf.h>

/* TODO: .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf64_HeaderInfo elf_ppc64_info =
{
  0x1000,         /*  Elf64_Xword pagesize; */
  SHT_NOBITS,     /*  Elf64_Xword pltshtype; */
  0x18,           /*  Elf64_Xword pltentsize; */
  0x10000,        /*  Elf64_Xword ptload_align; */
  0x8,            /*  Elf64_Xword ptdynamic_align; */
  0x1,            /*  Elf64_Xword ptinterp_align; */
  0x4,            /*  Elf64_Xword ptnote_align; */
  0x8,            /*  Elf64_Xword ptphdr_align; */
  (Elf32_Word)-1  /*  Elf64_Xword pttls_align; -- keep at -1 if not supported */
};

/* write-and-check macros for relocations {{{ */
/* XXX ppc64-16 relocations point to the halfword that has
 * to be relocated, not to the beginning of the instruction
 * itself. So while we actually relocate the lowest 16 bits
 * of an instruction, this will be the highest 16 bits of the
 * word we read with the l operater
 *
 * -- now we read/write only 16 bits, since the next 16
 *  bits may still have to be changed by another relocation,
 *  so the relocation check of the original linking will fail
 *  if we read/write 32 bits.
 */
#define WRITE_LOW16 "i0000ffff & k*v \\ s0000$"
#define WRITE_NEXTLOW16 "s0010 > " WRITE_LOW16
#define WRITE_LOW16_SHIFT2 "i0000fffc & k s0003 & | v \\ s0000$"
#define WRITE_REL32 "= I00000000ffffffff & l*w \\ Iffffffff80000000 & ? Iffffffff80000000 - : !$"
/* }}} */

/* IsElfPpc64SameEndian {{{ */
t_bool
IsElfPpc64SameEndian (FILE * fp)
{
    Elf64_Byte buffer[EI_NIDENT];
    Elf64_Ehdr hdr;
    Elf64_Half e_machine;
    t_uint32 save = ftell (fp);

#ifndef DIABLOSUPPORT_WORDS_BIGENDIAN
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
        case ELFDATA2LSB:
            return FALSE;
        case ELFDATA2MSB:
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
            e_machine = hdr.e_machine;
            break;
        case ELFCLASS32:
        case ELFCLASSNONE:
        default:
            return FALSE;
            break;
    }

    if (e_machine != EM_PPC64)
        return FALSE;

    return TRUE;
}
/* }}} */

/* ElfReadPpc64SameEndian {{{ */
#define _NewDynReloc(name)                                      \
    do {                                                        \
        t_symbol *sym = dynamic_table[ELF64_R_SYM(rel[tel2].r_info)];     \
	t_string symname = StringConcat2 (name, SYMBOL_NAME (sym)); \
        SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE (obj),        \
                symname,        \
                "R00$", 10, TRUE, FALSE,                     \
                T_RELOCATABLE (sec_table[tel]),                 \
                AddressNew64 (tel2 * sizeof (Elf64_Rela)),      \
                AddressNew64(0), NULL, AddressNew64(0), 0);        \
		Free(symname);                                  \
    } while(0)

#define _NewReloc(calcstr)                                      \
    do {                                                        \
        t_symbol *sym;                                          \
        t_address generic, addend;                              \
                                                                \
        sym = table[ELF64_R_SYM(rel[tel2].r_info)];             \
        generic = AddressNew64 (rel[tel2].r_offset);            \
        addend = AddressNew64 (rel[tel2].r_addend);             \
                                                                \
        RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),  \
                addend, T_RELOCATABLE (corrsec), generic, sym,  \
                TRUE, NULL, NULL, NULL,                         \
                calcstr);                                       \
    } while(0)

void
ElfReadPpc64SameEndian (FILE *fp, t_object *obj, t_bool read_debug)
{
    Elf64_Shdr *shdrs = NULL;
    Elf64_Sym *symbol_table = NULL;
    Elf64_Sym *dynamic_symbol_table = NULL;
    t_section **sec_table = NULL;
    char *strtab = NULL;
    t_uint32 numsyms = 0;
    int tel;
    char *sechdrstrtab = NULL;
    Elf64_Ehdr hdr64;
    t_symbol **table = NULL;
    t_symbol **dynamic_table = NULL;

    if (fread ((&hdr64), sizeof (Elf64_Ehdr), 1, fp) != 1)
    {
        FATAL (("Could not read elf header!"));
    }

    /* Reading elf headers does not depend on the type of objectfile */
    ElfReadCommon64 (fp, &hdr64, obj, &shdrs, &symbol_table, &dynamic_symbol_table, &numsyms, &strtab,
            &sechdrstrtab, &sec_table, &table, &dynamic_table, FALSE, read_debug, &elf_ppc64_info);

    OBJECT_SET_RELOC_TABLE (obj, RelocTableNew (obj));

    if (hdr64.e_shnum)
    {
        tel = 0;
        while (tel < hdr64.e_shnum)
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
            else if (StringPatternMatch (".dynamic", sechdrstrtab + shdrs[tel].sh_name))
            {
                int dynamic_tel=0;
                Elf64_Dyn * dynamic_table = SECTION_DATA(sec_table[tel]);

                while (dynamic_table[dynamic_tel].d_tag != DT_NULL)	/* An entry with a DT_NULL tag marks the end of the _DYNAMIC array. */
                {
                    if ((dynamic_table[dynamic_tel].d_tag >= DT_LOPROC)
                            && (dynamic_table[dynamic_tel].d_tag <=
                                DT_HIPROC))
                    {
                        switch (dynamic_table[dynamic_tel].d_tag)
                        {
                            case DT_PPC64_GLINK:
                                SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(obj), "$dt_ppc64_glink", "R00$", 10, FALSE, FALSE,  T_RELOCATABLE(sec_table[tel]), AddressNew64(sizeof(Elf64_Dyn)*dynamic_tel), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                                break;
                            default:
                                FATAL (("Unknown d_tag: %d %x",
                                            dynamic_table[dynamic_tel].d_tag,
                                            dynamic_table[dynamic_tel].d_tag));
                        }
                    }
                    dynamic_tel++;
                }
            }
            else if ((StringPatternMatch (".rela.dyn", sechdrstrtab + shdrs[tel].sh_name))
                    || (StringPatternMatch (".rela.plt", sechdrstrtab + shdrs[tel].sh_name)))
            {
                if (shdrs[tel].sh_size)
                {
                    Elf64_Rela *rel = Malloc (shdrs[tel].sh_size);
                    t_uint32 tel2;

                    fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
                    fread (rel, shdrs[tel].sh_size, 1, fp);

                    for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf64_Rela)); tel2++)
                    {
                        switch (ELF64_R_TYPE (rel[tel2].r_info))
                        {
                            case R_PPC64_JMP_SLOT:
                                printf("DYNAMIC RELOC! %s",  SYMBOL_NAME(dynamic_table[ELF64_R_SYM(rel[tel2].r_info)]));
                                _NewDynReloc("PPC64_JMP_SLOT:");
                                break;
                            case R_PPC64_ADDR64:
                                printf("DYNAMIC RELOC! %s",  SYMBOL_NAME(dynamic_table[ELF64_R_SYM(rel[tel2].r_info)]));
                                _NewDynReloc("PPC64_ADDR64:");
                                break;
                            default:
                                FATAL (("Implement dynamic RELOC_TYPE %d",
                                            ELF64_R_TYPE (rel[tel2].r_info)));
                        }
                    }
                    Free(rel);
                }
            }
            else if (StringPatternMatch (".rela*", sechdrstrtab + shdrs[tel].sh_name))
            {
                Elf64_Rela *rel = Malloc (shdrs[tel].sh_size);
                t_uint32 tel2;
                t_section *corrsec;

                fseek (fp, OBJECT_STREAMPOS (obj) + shdrs[tel].sh_offset, SEEK_SET);
                fread (rel, shdrs[tel].sh_size, 1, fp);
                corrsec = sec_table[shdrs[tel].sh_info];

                for (tel2 = 0; tel2 < (shdrs[tel].sh_size / sizeof (Elf64_Rela)); tel2++)
                {
                    switch (ELF64_R_TYPE (rel[tel2].r_info))
                    {
                        case R_PPC64_NONE:
                            break;
                            /*
                               R_PPC64_ADDR32             =   1,
                               R_PPC64_ADDR24             =   2,
                               R_PPC64_ADDR16             =   3,
                             */
                        case R_PPC64_ADDR16_LO:
                            _NewReloc("S00A00+ \\" WRITE_LOW16);
                            break;
                        case R_PPC64_ADDR16_HI:
                            _NewReloc("S00A00+ \\" WRITE_NEXTLOW16);
                            break;
                        case R_PPC64_ADDR16_HA:
                            _NewReloc("S00A00+ = I0000000000008000 & s0001 < + \\" WRITE_NEXTLOW16);
                            break;
                            /*
                               R_PPC64_ADDR14             =   7,
                               R_PPC64_ADDR14_BRTAKEN     =   8,
                               R_PPC64_ADDR14_BRNTAKEN    =   9,
                             */
                        case R_PPC64_REL24:
                            {    
                                t_symbol *sym;                
                                if (SYMBOL_ORDER(table[ELF64_R_SYM(rel[tel2].r_info)]) == -1)
                                {
                                    sym = table[ELF64_R_SYM(rel[tel2].r_info)];
                                }
                                else
                                {
                                    t_section * sec= OBJECT_PARENT(obj)?OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)):OBJECT_UNDEF_SECTION(obj);
                                    t_string name = StringConcat2(".bl.", SYMBOL_NAME(table[ELF64_R_SYM(rel[tel2].r_info)])); 
                                    sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), name, "R00A00+$" , 0, PERHAPS, TRUE, T_RELOCATABLE(sec), AddressNullForObject(obj), AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
                                    Free(name);
                                }
                                        
                                t_address generic = AddressNew64 (rel[tel2].r_offset);
                                t_address addend = AddressNew64 (rel[tel2].r_addend);  
                                RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj), 
                                        addend, T_RELOCATABLE (corrsec), generic, sym, 
                                        TRUE, NULL, NULL, NULL,                       
                                        "S00A00+ d ? Ns.opd ? f : ! P- : ! \\ i03fffffc & l ifc000003 & | w \\ s0000$");
                                        /*"S00A00+ d ? Ns.opd ? f : ! P- : ! \\ i03fffffc & l ifc000003 & | = ~ s0001 & Ns.opd | ?: l * ie8410028 w! w \\ s0000$", 24, 6);*/
                            }
                            break;
                            /*
                               R_PPC64_REL14              =  11,
                               R_PPC64_REL14_BRTAKEN      =  12,
                               R_PPC64_REL14_BRNTAKEN     =  13,
                               R_PPC64_GOT16              =  14,
                               R_PPC64_GOT16_LO           =  15,
                               R_PPC64_GOT16_HI           =  16,
                               R_PPC64_GOT16_HA           =  17,
                               R_PPC64_COPY               =  19,
                               R_PPC64_GLOB_DAT           =  20,
                               R_PPC64_JMP_SLOT           =  21,
                               R_PPC64_RELATIVE           =  22,
                               R_PPC64_UADDR32            =  24,
                               R_PPC64_UADDR16            =  25,
                             */
                        case R_PPC64_REL32:
                            _NewReloc("S00A00+P- \\" WRITE_REL32);
                            break;
                            /*
                               R_PPC64_PLT32              =  27,
                               R_PPC64_PLTREL32           =  28,
                               R_PPC64_PLT16_LO           =  29,
                               R_PPC64_PLT16_HI           =  30,
                               R_PPC64_PLT16_HA           =  31,
                               R_PPC64_SECTOFF            =  33,
                               R_PPC64_SECTOFF_LO         =  34,
                               R_PPC64_SECTOFF_HI         =  35,
                               R_PPC64_SECTOFF_HA         =  36,
                               R_PPC64_ADDR30             =  37,
                             */
                        case R_PPC64_ADDR64:
                            _NewReloc("S00A00+ \\" WRITE_64);
                            break;
                        /* for this and the next ones: the ppc disassembler/assembler stores
                         * 32 bit constants for many (all?) opcodes which expects 16 bit
                           immediates, and then depending on the opcode only considers the
                           upper/lower 16 bits (e.g. upper for oris/addis, lower for ori/addi.

                           This breaks horribly when you load a 64 bit address, since those are
                           loaded using sequences like this:
                            lis   r4, longint_to_real_helper@highesta
                            ori   r4, r4, longint_to_real_helper@highera
                            sldi  r4, r4, 32
                            oris  r4, r4, longint_to_real_helper@ha
                            addi   r4, r4, longint_to_real_helper@lo
                            
                           This means that the highest(a)/higher(a) also take their value from
                           the lower 32 bits of the address. The workaround is to already shift
                           right the relocation value by 32 bits when loading it. In case of
                           highera/highesta, this means we also immediately already have to
                           adjust for the sign of the lower bits */
                        case R_PPC64_ADDR16_HIGHER:
                            _NewReloc("S00A00+ s0020 > \\" WRITE_LOW16);
                            break;
                        case R_PPC64_ADDR16_HIGHERA:
                            _NewReloc("S00A00+ = I00000000ffff8000 & I00000000ffff8000 - ? s0000 : s0001 ! s0020 < + s0020 > \\" WRITE_LOW16);
                            break;
                        case R_PPC64_ADDR16_HIGHEST:
                            _NewReloc("S00A00+ s0020 > \\" WRITE_NEXTLOW16);
                            break;
                        case R_PPC64_ADDR16_HIGHESTA:
                            _NewReloc("S00A00+ = I0000ffffffff8000 & I0000ffffffff8000 - ? s0000 : s0001 ! s0030 < + s0020 > \\" WRITE_NEXTLOW16);
                            break;
                            /*
                               R_PPC64_UADDR64            =  43,
                             */
                        case R_PPC64_REL64:
                            _NewReloc("S00A00+P- \\" WRITE_64);
                            break;
                            /*
                               R_PPC64_PLT64              =  45,
                               R_PPC64_PLTREL64           =  46,
                             */
                        case R_PPC64_TOC16:
                            {
                              /* add undefined symbol to represent the actual
                               * TOC pointer. This symbol will be resolved
                               * later on */
                              t_section *undef = OBJECT_PARENT(obj) ?
                                OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)) :
                                OBJECT_UNDEF_SECTION(obj);

                              t_symbol *sym =
                                table[ELF64_R_SYM(rel[tel2].r_info)];
                              t_symbol *gotsym = SymbolTableAddSymbol(
                                  OBJECT_SYMBOL_TABLE(obj),
                                  ".unkgotptr@@", "R00A00+$" , 0, PERHAPS,
                                  TRUE, T_RELOCATABLE(undef),
                                  AddressNullForObject(obj),
                                  AddressNullForObject(obj), NULL,
                                  AddressNullForObject(obj), 0);

                              t_address offset = AddressNew64 (rel[tel2].r_offset);
                              t_address addend = AddressNew64 (rel[tel2].r_addend);  
                              RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                  addend, T_RELOCATABLE(corrsec), offset, sym,
                                  TRUE, NULL, NULL, gotsym,
                                  "S00A00+ S01- \\" WRITE_LOW16);
                            }
                            break;
                            /*
                               R_PPC64_TOC16_LO           =  48,
                               R_PPC64_TOC16_HI           =  49,
                               R_PPC64_TOC16_HA           =  50,
                             */
                        case R_PPC64_TOC:
                            {
                              /* add undefined symbol to represent the actual
                               * TOC pointer. This symbol will be resolved
                               * later on */
                              t_section *undef = OBJECT_PARENT(obj) ?
                                OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)) :
                                OBJECT_UNDEF_SECTION(obj);

                              t_symbol *gotsym = SymbolTableAddSymbol(
                                  OBJECT_SYMBOL_TABLE(obj),
                                  ".unkgotptr@@", "R00A00+$" , 0, PERHAPS,
                                  TRUE, T_RELOCATABLE(undef),
                                  AddressNullForObject(obj),
                                  AddressNullForObject(obj), NULL,
                                  AddressNullForObject(obj), 0);

                              t_address offset = AddressNew64 (rel[tel2].r_offset);
                              t_address addend = AddressNew64 (rel[tel2].r_addend);  
                              RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                  addend, T_RELOCATABLE(corrsec), offset, gotsym,
                                  TRUE, NULL, NULL, NULL,
                                  "S00 \\" WRITE_64);
                            }
                            break;
                            /*
                               R_PPC64_PLTGOT16           =  52,
                               R_PPC64_PLTGOT16_LO        =  53,
                               R_PPC64_PLTGOT16_HI        =  54,
                               R_PPC64_PLTGOT16_HA        =  55,
                               R_PPC64_ADDR16_DS          =  56,
                            */
                        case R_PPC64_ADDR16_LO_DS:
                            _NewReloc("S00A00+ \\" WRITE_LOW16_SHIFT2);
                            break;
                             /*
                               R_PPC64_GOT16_DS           =  58,
                               R_PPC64_GOT16_LO_DS        =  59,
                               R_PPC64_PLT16_LO_DS        =  60,
                               R_PPC64_SECTOFF_DS         =  61,
                               R_PPC64_SECTOFF_LO_DS      =  62,
                             */
                        case  R_PPC64_TOC16_DS:
                            {
                              /* add undefined symbol to represent the actual
                               * TOC pointer. This symbol will be resolved
                               * later on */
                              t_section *undef = OBJECT_PARENT(obj) ?
                                OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj)) :
                                OBJECT_UNDEF_SECTION(obj);

                              t_symbol *sym =
                                table[ELF64_R_SYM(rel[tel2].r_info)];
                              t_symbol *gotsym = SymbolTableAddSymbol(
                                  OBJECT_SYMBOL_TABLE(obj),
                                  ".unkgotptr@@", "R00A00+$" , 0, PERHAPS,
                                  TRUE, T_RELOCATABLE(undef),
                                  AddressNullForObject(obj),
                                  AddressNullForObject(obj), NULL,
                                  AddressNullForObject(obj), 0);

                              t_address offset = AddressNew64 (rel[tel2].r_offset);
                              t_address addend = AddressNew64 (rel[tel2].r_addend);  
                              RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                  addend, T_RELOCATABLE(corrsec), offset, sym,
                                  TRUE, NULL, NULL, gotsym,
                                  "S00A00+ S01- \\" WRITE_LOW16_SHIFT2);
                            }
                            break;
                            /*
                               R_PPC64_TOC16_LO_DS        =  64,
                               R_PPC64_PLTGOT16_DS        =  65,
                               R_PPC64_PLTGOT16_LO_DS     =  66,
			     */
			case R_PPC64_TLS:
			    {
			      /* In case of a staticaly linked program, these relocations are attached to
			       * instructions calculating the lower 16 bits of a tls symbol (i.e., sort
			       * of equivalent to a R_PPC64_TPREL16_LO).
			       *
			       * One annoying part about this relocation is that it also has to transform
			       * the instruction it applies to, and there are many different possibilities
			       * (hence the huge calculate string).
			       */
                               t_address reloc_offset;
			       t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			       t_address addend = AddressNew64 (rel[tel2].r_addend);
			       t_symbol *tls_start;
			       t_uint32 org_instr;
			       t_reloc *nrel;

			       reloc_offset=AddressNew64 (rel[tel2].r_offset);
			       org_instr = SectionGetData32(corrsec, reloc_offset);

			       /* now add a R_PPC64_TPREL16_LO relocation to correct the offset
				* of the symbol in the <whatever it is> instruction (which we will
				* also construct here)
				*/
			       tls_start = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringDup("$tls_start"),"R00A00+$",0,
				       PERHAPS,TRUE, OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
				       AddressNullForObject(obj),AddressNullForObject(obj),
				       NULL, AddressNullForObject(obj), 0);
			       /* WARNING: this relocation requires the *original* instruction data
				*   from the object file, because depending on that instruction it
				*   may have to perform a different kind of relocation. During the
				*   initial check phase this is fine because then we indeed use this
				*   data, but during the final writing of the binary we will try to
				*   relocate the instruction that we initially generated (which could
				*   be completely different now)
				*
				*   To solve this, we pass the original instruction data as an addend (A01).
				*/

			       nrel = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					       addend,
					       T_RELOCATABLE (corrsec),
					       reloc_offset, sym, TRUE, NULL,
					       NULL, tls_start,
					       /* immediately swap the symbol address and the loaded instruction, because we often
						* have to duplicate the loaded intruction later on (we get it from the top of the
						* stack when needed)
						*/
					       "S00A00+S01-s7000- sffff&" "l*A01 %"

					       "S00A00+S01-s7000+I0000000080000000+I0000000100000000-s003f> ?"
					       /* To check whether two values are equal, we subtract them. We then want "if x==0",
						* but the stack machine only supports "if x!=0" -> compute the "x==0" predicate
						* using the fact that x | (-x) == 0 if x==0, and < 0 otherwise -> extracting the
						* sign bit and inverting it gives us the desired result. In stack machine language:
						* =s0000%-|s003f>s0001^ (shr 63 because 64 bit addresses)^
						*/

					       /* if ((insn & ((0x3f << 26) | (31 << 11))) == ((31 << 26) | (13 << 11))) */
					       "} = { ifc00f800& i7c006800- =s0000%-|s003f>s0001^ ?"
					       /* rtra = insn & ((1 << 26) - (1 << 16)); */
					       "} = { i03ff0000&"
					       /* else if ((insn & ((0x3f << 26) | (31 << 16))) == ((31 << 26) | (13 << 16))) */
					       ": } = { ifc1f0000& i7c0d0000- =s0000%-|s003f>s0001^ ? "
					       /* rtra = (insn & (31 << 21)) | ((insn & (31 << 11)) << 5); */
					       "} == { i03e00000& % sf800& s0005< |"
					       /* else abort();  -> load error code "1" */
					       ": s0001 s0001 !!"
					       /* stack is now (in case of correct execution): <insn>> <sym> <rtra>
					        * Now we compute the second part of the instruction.
						*
						* if ((insn & ((1 << 11) - (1 << 1))) == 266 << 1)
					        */
					       "} = { s07fe& s0214- =s0000%-|s003f>s0001^ ?"
					       /* add -> addi: insn = 14 << 26 (discard original instruction) */
					       "} * i38000000"
					       /* 
						* check for load and store indexed -> dform.
						* for &&/||, we'll use -1 as true to easily do the and/or'ing and check at the end
						*
						* else if ((insn & (31 << 1)) == 23 << 1
						*/
					       ": } = { s003e& s002e- =s0000%-|s003f>s0001^ ~"
					       /*          && ((insn & (31 << 6)) < 14 << 6  ->  0-(((insn & (31 << 6))-14 << 6)>>63) (= -signbit) */
					       "} = { s0000% s07c0& s0380- s003f> - %"
					       /*              || ((insn & (31 << 6)) >= 16 << 6  -> 0-(((16 << 6 -1)-(insn & (31 << 6)))>>63) (= -signbit) */
					       "} = { s0000% s07c0& s03ff % - s003f> - % "
					       /*                  && (insn & (31 << 6)) < 24 << 6)) -> 0-(((insn & (31 << 6)) - (24 << 6))>>63) (= -signbit) */
					       "} = { s0000% s07c0& s0600- s003f> -"
					       /* now the and/or'ing. The stack contains "insn sym tra expr1 expr2 expr3 expr4" and we have to calculate (expr1 && (expr2 || (expr3 && expr4)))
						* Since true == -1, add 1 at the end to make it zero so we can use our regular "compare-with-zero" trick*/
					       "& | & s0001+ =s0000%-|s003f>s0001^ ?"
					       /* insn = (32 | ((insn >> 6) & 31)) << 26*/
					       "} s0006> s001f& s0020| s001a<"
					       /* 
						* ldx, ldux, stdx, stdux -> ld, ldu, std, stdu.
						* 
						* else if ((insn & (31 << 1)) == 21 << 1  && (insn & (0x1a << 6)) == 0 -> (insn & ((31 << 1) | (0x1a << 6)) == (21 << 1))
						*/
					       ": } = { s06be& s002a- =s0000%-|s003f>s0001^ ?"
					       /* insn = (((58 | ((insn >> 6) & 4)) << 26) | ((insn >> 6) & 1)) */
					       "} = s0006> s0004& s003a| s001a< % s0006> s0001& |"
					       /* else if ((insn & (31 << 1)) == 21 << 1 && (insn & ((1 << 11) - (1 << 1))) == 341 << 1)
						* -> if ((insn & ((31 << 1) | ((1 << 11) - (1 << 1)))) == ((21 << 1) | (341 << 1))) (actually the first test is also comprised in the second one)
						*/
					       ": } = { s07fe& s02bf- =s0000%-|s003f>s0001^ ?"
					       /* lwax -> lwa : insn = (58 << 26) | 2 */
					       "} * ie8000002"
					       /* else abort(); -> load "error code 2" */
					       ": } * s0002 s0002 !!!!"
					       /* We did it! The stack now contains, in case no error occurred, <sym> <tra> <insn> -> or them all and write the full 32 bits */
					       " | | "
					       " : * ! "
					       "\\" "w \\ s0000$"
					       );
			      RelocAddAddend(nrel,AddressNew64(org_instr));
			    }
			    break;
                            /*
                               R_PPC64_DTPMOD64           =  68,
                               R_PPC64_TPREL16            =  69,
			    */
                        case R_PPC64_TPREL16_LO:
			    {
                               t_address reloc_offset;
			       t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			       t_symbol *tls_start;
			       t_address addend = AddressNew64 (rel[tel2].r_addend);

			       reloc_offset=AddressNew64 (rel[tel2].r_offset);

			       /* now add a R_PPC64_TPREL16_LO relocation to correct the offset
				* of the symbol in the <whatever it is> instruction (which we will
				* also construct here)
				*/
			       tls_start = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringDup("$tls_start"),"R00A00+$",0,
				       PERHAPS,TRUE, OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
				       AddressNullForObject(obj),AddressNullForObject(obj),
				       NULL, AddressNullForObject(obj), 0);

			       RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					       addend,
					       T_RELOCATABLE (corrsec),
					       reloc_offset, sym, TRUE, NULL,
					       NULL, tls_start,
					       "S00A00+S01-s7000- " "\\" WRITE_LOW16
					       );
			    }
			    break;
			    /*
                               R_PPC64_TPREL16_HI         =  71,
                               R_PPC64_TPREL64            =  73,
                               R_PPC64_DTPREL16           =  74,
                               R_PPC64_DTPREL16_LO        =  75,
                               R_PPC64_DTPREL16_HI        =  76,
                               R_PPC64_DTPREL16_HA        =  77,
                               R_PPC64_DTPREL64           =  78,
                               R_PPC64_GOT_TLSGD16        =  79,
                               R_PPC64_GOT_TLSGD16_LO     =  80,
                               R_PPC64_GOT_TLSGD16_HI     =  81,
                               R_PPC64_GOT_TLSGD16_HA     =  82,
                               R_PPC64_GOT_TLSLD16        =  83,
                               R_PPC64_GOT_TLSLD16_LO     =  84,
                               R_PPC64_GOT_TLSLD16_HI     =  85,
                               R_PPC64_GOT_TLSLD16_HA     =  86,
			     */
			case R_PPC64_TPREL16_HA:
                        case R_PPC64_GOT_TPREL16_DS:
			    {
			      /* In case R_PPC64_GOT_TPREL16_DS appear in a statically linked program, they can be treated as
			       * a R_PPC64_TPREL16_HA after transforming the load to an add. This relocation
			       * is the HA of "addend -= htab->elf.tls_sec->vma + TP_OFFSET;".
			       * TP_OFFSET == 0x7000 (offset of the TP pointer from the start of the TLS block).
			       */
			      
			      /* The relocation points to "instruction address + 2 */
                               t_address ins_address, reloc_offset;
			       t_symbol *sym = table[ELF64_R_SYM (rel[tel2].r_info)];
			       t_address addend = AddressNew64 (rel[tel2].r_addend);
			       t_symbol *loadsym, *tls_start;
			       t_string loadsymname;
			       reloc_offset=AddressNew64 (rel[tel2].r_offset);
			       t_reloc *nrel;

			       tls_start = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj),StringDup("$tls_start"),"R00A00+$",0,
				       PERHAPS,TRUE, OBJECT_PARENT(obj)?T_RELOCATABLE(OBJECT_UNDEF_SECTION(OBJECT_PARENT(obj))):T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
				       AddressNullForObject(obj),AddressNullForObject(obj),
				       NULL, AddressNullForObject(obj), 0);

			       if (ELF64_R_TYPE (rel[tel2].r_info)==R_PPC64_GOT_TPREL16_DS)
			       {
			         t_uint32 org_instr;

			         ins_address = AddressSubInt32(reloc_offset,2);
				 org_instr = SectionGetData16(corrsec, ins_address);
			         loadsymname = StringConcat2("DIABLO_PPC64_GOT_TPREL16:LOAD:", SYMBOL_NAME(sym));
			         loadsym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(obj), loadsymname, "R00A00+$", 10, TRUE, FALSE, T_RELOCATABLE(corrsec), ins_address, AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
				 /* WARNING: see comments for R_PPC64_TLS above regarding the fact that
				  *   that the original instruction data is passed as an addend here!
				  */
			         /* (ins & (31 << 21)) | 0x3c0d0000: convert the load to an add */
			         nrel=RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
			       			addend,
						T_RELOCATABLE (corrsec),
						ins_address, sym, TRUE, NULL,
						NULL, tls_start,
						"k*A01 S00A00+S01-s7000+I0000000080000000+I0000000100000000-s003f> ? s03e0& s3c0d | ! " "\\" "v \\ s0000$");
			         RelocAddAddend(nrel,AddressNew64(org_instr));
			       }

			       /* now add a R_PPC64_TPREL16_HA relocation to correct the offset
				* of the symbol in the add instruction
				*/
			       if (ELF64_R_TYPE (rel[tel2].r_info)==R_PPC64_GOT_TPREL16_DS)
			       {
			         t_uint32 org_instr;
  
				 org_instr = SectionGetData16(corrsec, reloc_offset);
				 /* WARNING: see comments for R_PPC64_TLS above regarding the fact that
				  *   that the original instruction data is passed as an addend here!
				  */
			         nrel=RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					       addend,
					       T_RELOCATABLE (corrsec),
					       reloc_offset, sym, TRUE, NULL,
					       NULL, tls_start,
					       "k*A01 S00A00+S01-s7000+I0000000080000000+I0000000100000000-s003f> ? * S00S01-s7000- = I0000000000008000 & s0001 < + s0010> sffff& ! " "\\ v\\s0000$" 
					       );
			         RelocAddAddend(nrel,AddressNew64(org_instr));
		               }
			       else
			       {
			         RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE (obj),
					       addend,
					       T_RELOCATABLE (corrsec),
					       reloc_offset, sym, TRUE, NULL,
					       NULL, tls_start,
					       "k * S00A00+S01-s7000- = I0000000000008000 & s0001 < + s0010> sffff& " "\\ v\\s0000$" 
					       );
			       }
			    }
			    break;
			  
			   /*
                               R_PPC64_GOT_TPREL16_LO_DS  =  88,
                               R_PPC64_GOT_TPREL16_HI     =  89,
                               R_PPC64_GOT_TPREL16_HA     =  90,
                               R_PPC64_GOT_DTPREL16_DS    =  91,
                               R_PPC64_GOT_DTPREL16_LO_DS =  92,
                               R_PPC64_GOT_DTPREL16_HI    =  93,
                               R_PPC64_GOT_DTPREL16_HA    =  94,
                               R_PPC64_TPREL16_DS         =  95,
                               R_PPC64_TPREL16_LO_DS      =  96,
                               R_PPC64_TPREL16_HIGHER     =  97,
                               R_PPC64_TPREL16_HIGHERA    =  98,
                               R_PPC64_TPREL16_HIGHEST    =  99,
                               R_PPC64_TPREL16_HIGHESTA   = 100,
                               R_PPC64_DTPREL16_DS        = 101,
                               R_PPC64_DTPREL16_LO_DS     = 102,
                               R_PPC64_DTPREL16_HIGHER    = 103,
                               R_PPC64_DTPREL16_HIGHERA   = 104,
                               R_PPC64_DTPREL16_HIGHEST   = 105,
                               R_PPC64_DTPREL16_HIGHESTA  = 106,
                             */
                        default:
                            FATAL (("Implement RELOC_TYPE %d",
                                        ELF64_R_TYPE (rel[tel2].r_info)));
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
/* }}} */

/* ElfWritePpc64SameEndian {{{ */
void
ElfWritePpc64SameEndian (FILE * fp, t_object * obj)
{
    int tel = 0;
    Elf64_Ehdr hdr;

    STATUS (START, ("Writing Ppc64 binary"));

    hdr.e_ident[EI_MAG0] = ELFMAG0;
    hdr.e_ident[EI_MAG1] = ELFMAG1;
    hdr.e_ident[EI_MAG2] = ELFMAG2;
    hdr.e_ident[EI_MAG3] = ELFMAG3;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    /* \TODO Need to use DIABLOSUPPORT_WORDS_BIGENDIAN ? */
    hdr.e_ident[EI_DATA] = ELFDATA2MSB;
    hdr.e_ident[EI_CLASS] = ELFCLASS64;

    /* Padding : EI_PAD=7 from e_ident[7] to e_ident[15] */
    for (tel = EI_PAD; tel < 16; tel++)
    {
        hdr.e_ident[tel] = 0;
    }
    hdr.e_machine = EM_PPC64;
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

    ElfWriteCommonSameEndian64 (fp, &hdr, obj, TRUE, &elf_ppc64_info);
    STATUS (STOP, ("Writing Ppc64 binary"));
}
/* }}} */


/* XXX experimental code from Dominique: support multi-TOC binaries for ppc64.
 * The problem here is that the linker script language is not expressive enough
 * to properly do this, so instead I have opted for the very unclean option
 * of defining a new "super-rule" that does everything at once 
 *
 * This new rule will be installed as a callback in the linker rule parser */
typedef struct _t_ppc64_toc {
  t_address ptr;
  t_symbol *ptrsym;
  struct _t_ppc64_toc *next;
} t_ppc64_toc;

typedef enum {ppc64_plt_call, ppc64_long_branch_r2off} t_ppc64_stubtype;

typedef struct _t_ppc64_stub {
  t_ppc64_stubtype type;
  t_symbol *sym;        /* symbol that identifies the stub */
  t_ppc64_toc *toc;     /* associated toc */
  t_ppc64_toc *to_toc;  /* new toc for *_r2off stubs */
  t_string assoc_name;  /* associated name extracted from symbol */
  struct _t_ppc64_stub *next;
} t_ppc64_stub;

t_ppc64_toc *Ppc64TocNew(t_ppc64_toc **list)
{
  t_ppc64_toc *new = Calloc(1, sizeof(t_ppc64_toc));
  new->next = *list;
  *list = new;
  return new;
}

t_ppc64_stub *Ppc64StubNew(t_ppc64_stub **list)
{
  t_ppc64_stub *new = Calloc(1, sizeof(t_ppc64_stub));
  new->next = *list;
  *list = new;
  return new;
}

struct toc_offset {
  t_uint32 offset;
  t_ppc64_toc *from;
  t_ppc64_toc *to;
};

/* {{{ Ppc64ExtractAssociatedNameFromStubSym */
static t_string Ppc64ExtractAssociatedNameFromStubSym(t_symbol *sym)
{
  /* stub symbols have this format:
   * nnnnnnnn.type.name[@+]fluff
   * where type is either plt_call or long_branch_r2off */

  int i;
  t_string start = SYMBOL_NAME(sym) + 8; /* skip the 8-digit numeral */

  if (StringPatternMatch(".plt_call.*", start))
  {
    start += 10;
  }
  else if (StringPatternMatch(".long_branch_r2off.*", start))
  {
    start += 19;
  }
  else
    FATAL(("cannot extract associated name from @S", sym));

  t_string name = StringDup(start);
  for (i = 0; name[i] != '\0'; ++i)
    if (name[i] == '@' || name[i] == '+')
    {
      name[i] = '\0';
      break;
    }
  return name;
}
/* }}} */

static const t_uint32 plt_call_stub[] = {
  0x3d820000,
  0xf8410028,
  0xe96c0000,
  0xe84c0000,
  0x7d6903a6,
  0xe96c0000,
  0x4e800420
};

static const t_uint32 long_branch_r2off_stub[] = {
  0xf8410028,
  0x3c420000,
  0x38420000,
  0x48000000
};


extern t_object *parser_object;
void *Ppc64ResolveTocsAndAddLinkerStubs(t_ast_successors *succ, void *data)
{
  static int invokecount = 0;
  if (++invokecount > 1) return NULL;

  t_object *obj = parser_object;
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_section *gotsec = SectionGetFromObjectByName(obj, ".got");
  t_symbol *sym;

  t_ppc64_toc *tocs = NULL;
  t_ppc64_stub *stubs = NULL;

  t_ppc64_toc *toc;
  t_ppc64_stub *stub;
  
  int i, ntocs;

  if (!gotsec)
  {
    gotsec = SectionGetFromObjectByName(obj, ".data");
    ASSERT(gotsec, ("Could not find a suitable section to serve as GOT"));
  }

  /* {{{ create list of all stubs */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(obj), sym)
  {
    if (StringPatternMatch("*.plt_call.*", SYMBOL_NAME(sym)))
    {
      /* plt_call stub */
      t_ppc64_stub *stub = Ppc64StubNew(&stubs);
      stub->type = ppc64_plt_call;
      stub->sym = sym;
      stub->assoc_name = Ppc64ExtractAssociatedNameFromStubSym(sym);
    }
    else if (StringPatternMatch("*.long_branch_r2off.*", SYMBOL_NAME(sym)))
    {
      /* long_branch_r2off stub */
      t_ppc64_stub *stub = Ppc64StubNew(&stubs);
      stub->type = ppc64_long_branch_r2off;
      stub->sym = sym;
      stub->assoc_name = Ppc64ExtractAssociatedNameFromStubSym(sym);
    }
  }
  /* }}} */

  /* {{{ reverse engineer the toc pointers from the plt_call stubs */
  for (stub = stubs; stub; stub = stub->next)
  {
    if (stub->type != ppc64_plt_call) continue;

    t_string jmpslotname = StringConcat2("PPC64_JMP_SLOT:", stub->assoc_name);
    t_symbol *jmpslotsym =
      SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), jmpslotname);
    ASSERT(jmpslotsym, ("Could not find symbol %s", jmpslotname));
    t_uint64 plt_entry = SectionGetData64(T_SECTION(SYMBOL_BASE(jmpslotsym)),
                                          SYMBOL_OFFSET_FROM_START(jmpslotsym));
    
    /* load the linker-produced instruction and extract the correct offset */
    t_uint32 origval, hi_offset, lo_offset, offset;
    origval = SectionGetData32(T_SECTION(SYMBOL_BASE(stub->sym)), SYMBOL_OFFSET_FROM_START(stub->sym));
    hi_offset = (origval & 0xffff) << 16;
    origval = SectionGetData32(T_SECTION(SYMBOL_BASE(stub->sym)),
                          AddressAddUint32(SYMBOL_OFFSET_FROM_START(stub->sym),8));
    lo_offset = (origval & 0xffff);
    if (lo_offset & 0x8000) lo_offset |= 0xffff0000;    /* sign extend */
    offset = hi_offset + lo_offset;

    t_address tocptr = AddressNewForObject(obj,
                               plt_entry-(t_uint64)(t_int64)(t_int32)offset);

    for (toc = tocs; toc; toc = toc->next)
      if (AddressIsEq(toc->ptr, tocptr))
        break;
    if (!toc)
    {
      toc = Ppc64TocNew(&tocs);
      toc->ptr = tocptr;
      VERBOSE(1, ("New TOC ptr found: @G", tocptr));
    }
    stub->toc = toc;
  }

  /* if no toc pointers were found (i.e. there are no plt call stubs), 
   * just assume there is one toc pointer, offset 0x8000 from the beginning
   * of the got section */
  /* FIXME this will probably not work with statically linked gcc binaries.
   * These binaries don't have plt_call stubs but can have multiple TOC 
   * pointers! */
  if (!tocs)
  {
    toc = Ppc64TocNew(&tocs);
    toc->ptr = AddressAddUint32(SECTION_CADDRESS(gotsec), 0x8000);
  }
  
  /* }}} */

  /* add symbols for the different TOC pointers {{{ */
  i = 0;
  for (toc = tocs; toc; toc = toc->next)
  {
    char name[30];
    sprintf(name, "$tocptr:%d", i++);
    toc->ptrsym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
                                         name, "R00A00+$", 12, NO, NO,
                                         T_RELOCATABLE(gotsec),
                                         AddressSub(toc->ptr, SECTION_CADDRESS(gotsec)),
                                         AddressNullForObject(obj),
                                         NULL,
                                         AddressNullForObject(obj), 0);
  }
  ntocs = i;
  /* }}} */

  /* figure out toc and to_toc for long_branch_r2off stubs {{{ */
  /* XXX this is really fishy: we assume no two toc pointer pairs have
   * the same relative distance. If that is the case, we're in trouble.
   * Luckily, we expect at most 2 different toc pointers, in which case
   * this will always work */
  struct toc_offset offsets[ntocs * ntocs];
  i = 0;
  for (toc = tocs; toc; toc = toc->next)
  {
    t_ppc64_toc *toc2;
    for (toc2 = tocs; toc2; toc2 = toc2->next)
    {
      offsets[i].from = toc;
      offsets[i].to = toc2;
      offsets[i].offset = AddressExtractUint32(AddressSub(toc2->ptr, toc->ptr));
      ++i;
    }
  }
  for (stub = stubs; stub; stub = stub->next)
  {
    if (stub->type != ppc64_long_branch_r2off) continue;
    t_uint32 hi_add =
      SectionGetData32(T_SECTION(SYMBOL_BASE(stub->sym)),
                       AddressAddUint32(SYMBOL_OFFSET_FROM_START(stub->sym), 4));
    t_uint32 lo_add =
      SectionGetData32(T_SECTION(SYMBOL_BASE(stub->sym)),
                       AddressAddUint32(SYMBOL_OFFSET_FROM_START(stub->sym), 8));
    t_uint32 offset = lo_add & 0xffff;
    if (offset & 0x8000) offset |= 0xffff0000; /* sign extend */
    offset += (hi_add << 16);

    for (i = 0; i < ntocs*ntocs; ++i)
      if (offsets[i].offset == offset)
        break;
    ASSERT(i < ntocs*ntocs,
           ("Could not find the toc pointer combination for @S", stub->sym));
    stub->toc = offsets[i].from;
    stub->to_toc = offsets[i].to;
  }
  /* }}} */

  for (stub = stubs; stub; stub = stub->next)
  {
    if (stub->type == ppc64_plt_call)
    {
      /* create *.plt_call.* stubs {{{ */

      /* A plt_call stub looks like this:
       *
       * addis   r12,r2,1        <- relocated
       * std     r2,40(r1)
       * ld      r11,-21448(r12) <- relocated
       * ld      r2,-21440(r12)  <- relocated
       * mtctr   r11
       * ld      r11,-21432(r12) <- relocated
       * bctr
       */
      t_reloc *rel;
      t_string subname = StringConcat2(".plt_call.",stub->assoc_name);
      t_section *sub = SectionCreateForObject(linker,CODE_SECTION,
                                              T_SECTION(SYMBOL_BASE(stub->sym)),
                                              AddressNewForObject(obj,28),
                                              subname);
      SECTION_SET_CADDRESS(sub, AddressAdd(RELOCATABLE_CADDRESS(SYMBOL_BASE(stub->sym)),
                                           SYMBOL_OFFSET_FROM_START(stub->sym)));
      t_string jmpslotname = StringConcat2("JMP_SLOT:", stub->assoc_name);
      t_symbol *jmpslotsym =
        SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), jmpslotname);
      ASSERT(jmpslotsym, ("Could not find symbol %s", jmpslotname));

      Free(subname);
      Free(jmpslotname);

      /* add $a symbol to start of stub */
      SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
          "$a", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(sub),
          AddressNullForObject(obj), AddressNullForObject(obj),
          NULL, AddressNullForObject(obj), 0);

      memcpy(SECTION_DATA(sub), plt_call_stub, 28);
      /* addis r12, r2, 1 */
      RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNullForObject(obj),
                                 jmpslotsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01 - = i00008000 & s0001 < + s0010 > i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      /* ld r11,xxx(r12) */
      RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,8),
                                 jmpslotsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01 - i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      /* ld r2,xxx+8(r12) */
      rel = RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,12),
                                 jmpslotsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01 - i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      RELOC_TO_SYMBOL_OFFSET(rel)[0] = AddressNewForObject(obj, 8);
      /* ld r2,xxx+16(r12) */
      rel = RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,20),
                                 jmpslotsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01 - i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      RELOC_TO_SYMBOL_OFFSET(rel)[0] = AddressNewForObject(obj, 16);

      /* find the PPC64_REL24 relocations that point to the corresponding
       * .bl.xxx symbol, and redirect those that belong to this particular
       * plt_call stub to us (for multiple TOCs, there will also be multiple
       * plt_call stubs for one dynamically linked function) {{{ */
      t_string dynsymsymname = StringConcat2("DYNSYMSYM:", stub->assoc_name);
      t_symbol *dynsym =
        SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj), dynsymsymname);
      if (dynsym)
      {
        t_string id = StringDup(SYMBOL_NAME(stub->sym));
        /* we only want the 8-digit identifier to make our .bl. sym unique */
        id[8] = '\0'; 
        t_string newblsymname = StringConcat3(id,".bl.", stub->assoc_name);
        t_symbol *newblsym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
                             newblsymname, "R00 A00+$", 12, 
                             FALSE, FALSE, T_RELOCATABLE(sub), AddressNullForObject(obj),
                             AddressNullForObject(obj), NULL,
                             AddressNullForObject(obj), 0
                             );
        t_string blsymname = StringConcat2(".bl.", stub->assoc_name);
        t_symbol *blsym =
          SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), blsymname);
        ASSERT(blsym, ("Could not find the .bl.%s symbol", stub->assoc_name));
        ASSERT(SYMBOL_SEARCH(blsym), ("@S should be undefined", blsym));

        /* find all calls to the stub and replace the succeeding nop with a
         * ld r2, 40(r1) instruction that restores the TOC pointer upon return
         * {{{ */
        t_reloc_ref *rr, *rr2;
        t_relocatable *base = SYMBOL_BASE(blsym);
        for (rr = RELOCATABLE_REFED_BY(base), rr2 = rr?RELOC_REF_NEXT(rr):NULL;
             rr;
             rr = rr2, rr2 = rr ? RELOC_REF_NEXT(rr) : NULL)
        {
          t_reloc *rel = RELOC_REF_RELOC(rr);
          if (RELOC_N_TO_SYMBOLS(rel) != 1) continue;
          if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[0]) != blsym) continue;
          t_section *from = T_SECTION(RELOC_FROM(rel));
          t_uint32 nextins = SectionGetData32(from,
              AddressAddUint32(RELOC_FROM_OFFSET(rel), 4));
          if (nextins == 0x60000000 || nextins == 0xe8410028)
          {
            t_uint32 index;
            index = (AddressExtractUint32(RELOC_FROM_OFFSET(rel))+4)/4;
            ((t_uint32 *)SECTION_DATA(from))[index] = 0xe8410028;
          }
        }
        /* }}} */

        /* look at all relocations to (the subobject copies of) blsym, find
         * those that have to end up referring to this stub
         * {{{ */
        for (rr = RELOCATABLE_REFED_BY(base), rr2 = rr?RELOC_REF_NEXT(rr):NULL;
             rr;
             rr = rr2, rr2 = rr ? RELOC_REF_NEXT(rr) : NULL)
        {
          t_reloc *rel = RELOC_REF_RELOC(rr);
          if (RELOC_N_TO_SYMBOLS(rel) != 1) continue;
          if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[0]) != blsym) continue;

          /* This is a good relocation. Does it point to our stub? */
          t_relocatable *from = RELOC_FROM(rel);
          ASSERT(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_SUBSECTION,
                 ("expected to come from subsection: @R", rel));
          t_address fromaddr = AddressAdd(RELOCATABLE_CADDRESS(from),
                                          RELOC_FROM_OFFSET(rel));
          t_section *parsec = SECTION_PARENT_SECTION(T_SECTION(from));
          t_uint32 relocated =
            SectionGetData32(parsec,
                             AddressSub(fromaddr, SECTION_CADDRESS(parsec)));
          relocated &= 0xfffffffc;      /* last 2 bits encode part of opcode */
          t_uint64 offset =
            (relocated & 0x02000000) ? (0xfffffffffc000000ll|(t_uint64)relocated):
            (t_uint64) (relocated & 0x03ffffff);
          t_address destination = AddressAddUint64(fromaddr, offset);

          if (!AddressIsEq(destination, SECTION_CADDRESS(sub)))
          {
            continue;   /* nope, meant for other PLT stub to same function */
          }

          SYMBOL_SET_MAPPED(RELOC_TO_SYMBOL(rel)[0], newblsym);
        }
        /* }}} */

        Free(blsymname);
        Free(newblsymname);
        Free(id);
      }
      Free(dynsymsymname);
      /* }}} */
      /* }}} */
    }
    else if (stub->type == ppc64_long_branch_r2off)
    {
      /* create *.long_branch_r2off.* stubs {{{ */

      /* a long_branch_r2off stub looks like this:
       *
       * std     r2,40(r1)
       * addis   r2,r2,1        <-relocated
       * addi    r2,r2,-864     <-relocated
       * b       1033d3f0       <-relocated
       */
      t_string targetname = StringConcat2(".L.",stub->assoc_name);
      t_symbol *targetsym;
      t_string subname = StringConcat2(".long_branch_r2off.",stub->assoc_name);
      t_section *sub = SectionCreateForObject(linker,CODE_SECTION,
                                              T_SECTION(SYMBOL_BASE(stub->sym)),
                                              AddressNewForObject(obj,16),
                                              subname);
      SECTION_SET_CADDRESS(sub, AddressAdd(RELOCATABLE_CADDRESS(SYMBOL_BASE(stub->sym)),
                                           SYMBOL_OFFSET_FROM_START(stub->sym)));

      /* find the target symbol for the branch instruction */
      t_uint64 branchoffset = (t_uint64)
        SectionGetData32(T_SECTION(SYMBOL_BASE(stub->sym)),
                         AddressAddUint32(SYMBOL_OFFSET_FROM_START(stub->sym),12));
      branchoffset &= 0x00ffffffull;
      if (branchoffset & 0x00800000ull) branchoffset |= 0xffffffffff000000ll;
      t_address targetaddr =
        AddressAddUint64(AddressAdd(RELOCATABLE_CADDRESS(SYMBOL_BASE(stub->sym)),
                                    SYMBOL_OFFSET_FROM_START(stub->sym)),
                         12ull+branchoffset);
      targetsym = SymbolTableGetFirstSymbolWithName(OBJECT_SUB_SYMBOL_TABLE(obj), targetname);
      while (targetsym &&
             !AddressIsEq(AddressAdd(RELOCATABLE_CADDRESS(SYMBOL_BASE(targetsym)),
                                     SYMBOL_OFFSET_FROM_START(targetsym)),
                          targetaddr))
      {
        targetsym = SYMBOL_EQUAL(targetsym);
      }
      if (!targetsym)
      {
        /* jump stub to a local symbol: hope to find this symbol */
        targetsym =
          SymbolTableGetSymbolByAddress(OBJECT_SUB_SYMBOL_TABLE(obj),
                                        targetaddr);
        if (!targetsym)
          FATAL(("cannot find a symbol with name %s or address @G",
                 targetname, targetaddr));
      }

      Free(subname);
      Free(targetname);

      /* add $a symbol to start of stub */
      SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
          "$a", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(sub),
          AddressNullForObject(obj), AddressNullForObject(obj),
          NULL, AddressNullForObject(obj), 0);

      memcpy(SECTION_DATA(sub), long_branch_r2off_stub, 16);
      /* addis   r2,r2,1 */
      RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,4),
                                 stub->to_toc->ptrsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01 - = i00008000 & s0001 < + s0010 > i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      /* addi    r2,r2,-864 */
      RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,8),
                                 stub->to_toc->ptrsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 stub->toc->ptrsym,
                                 "S00 S01- i0000ffff & \\ l iffff0000 & | w \\ s0000$");
      /* b       1033d3f0 */
      RelocTableAddRelocToSymbol(OBJECT_RELOC_TABLE(obj),
                                 AddressNullForObject(obj),
                                 T_RELOCATABLE(sub),
                                 AddressNewForObject(obj,12),
                                 targetsym,
                                 FALSE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 "S00 P- i03fffffc & \\ l ifc000003 & | w \\ s0000$");

      /* find the PPC64_REL24 relocations that point to the corresponding
       * .bl.xxx symbol, and redirect those that belong to this particular
       * long_branch stub to us {{{ */
      {
        t_string id = StringDup(SYMBOL_NAME(stub->sym));
        /* we only want the 8-digit identifier to make our .bl. sym unique */
        id[8] = '\0'; 
        t_string newblsymname = StringConcat3(id,".bl.", stub->assoc_name);
        t_symbol *newblsym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
                             newblsymname, "R00 A00+$", 12, 
                             FALSE, FALSE, T_RELOCATABLE(sub), AddressNullForObject(obj),
                             AddressNullForObject(obj), NULL,
                             AddressNullForObject(obj), 0
                             );
        t_string blsymname = StringConcat2(".bl.", stub->assoc_name);
        t_symbol *blsym =
          SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), blsymname);
        if (!blsym)
        {
          /* the stub belonged to a local symbol.
           * This means we don't have a .bl symbol to work with, and we should
           * find the correct local symbol, which points to the .opd section, 
           * and replace that */

          /* we'll just do brute-force searching for now, but there should be
           * a better way to do this */
          t_object *subobj, *tmpobj;
          OBJECT_FOREACH_SUBOBJECT(obj, subobj, tmpobj)
          {
            t_reloc *rel;
            OBJECT_FOREACH_RELOC(subobj, rel)
            {
              if (SECTION_TYPE(T_SECTION(RELOC_FROM(rel))) != CODE_SECTION)
                continue;
              if (RELOC_N_TO_SYMBOLS(rel) != 1)
                continue;
              t_symbol *tosym = RELOC_TO_SYMBOL(rel)[0];
              if (!SYMBOL_BASE(tosym) || 
                  strcmp(SECTION_NAME(T_SECTION(SYMBOL_BASE(tosym))), ".opd"))
                continue;

              t_address fromaddr =
                AddressAdd(RELOC_FROM_OFFSET(rel),
                           RELOCATABLE_CADDRESS(RELOC_FROM(rel)));
              t_section *parsec = SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel)));
              t_uint32 relocated =
                SectionGetData32(parsec, AddressSub(fromaddr, SECTION_CADDRESS(parsec)));
              t_uint64 offset = (t_uint64)relocated & 0x03fffffc;
              if (offset & 0x02000000)
                offset |= 0xfffffffffc000000ull;
              if (!AddressIsEq(SECTION_CADDRESS(sub), AddressAddUint64(fromaddr, offset)))
                continue;       /* not the relocation we're looking for */

              /* we cannot simply change the mapping of this symbol, as it may
               * have legitimate uses as a pointer to the .opd section as well.
               * Therefore, we just add a new symbol instead */
              t_symbol *subsym =
                SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(subobj),
                                     SYMBOL_NAME(newblsym), "R00A00+$",
                                     0, PERHAPS, TRUE,
                                     T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
                                     AddressNullForObject(obj),
                                     AddressNullForObject(obj), NULL,
                                     AddressNullForObject(obj), 0);
              SYMBOL_SET_MAPPED(subsym, newblsym);
              RelocSetToSymbol(rel, 0, subsym);

              /* add the TOC restore instruction after the call */
              t_section *from = T_SECTION(RELOC_FROM(rel));
              t_uint32 nextins = SectionGetData32(from,
                  AddressAddUint32(RELOC_FROM_OFFSET(rel), 4));
              if (nextins == 0x60000000 || nextins == 0xe8410028)
              {
                t_uint32 index;
                index = (AddressExtractUint32(RELOC_FROM_OFFSET(rel))+4)/4;
                ((t_uint32 *)SECTION_DATA(from))[index] = 0xe8410028;
              }
            }
          }
        }
        else
        {
          ASSERT(SYMBOL_SEARCH(blsym), ("@S should be undefined", blsym));

          /* find all calls to the stub and replace the succeeding nop with a
           * ld r2, 40(r1) instruction that restores the TOC pointer upon return
           * {{{ */
          t_reloc_ref *rr, *rr2;
          t_relocatable *base = T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj));
          for (rr = RELOCATABLE_REFED_BY(base), rr2 = rr?RELOC_REF_NEXT(rr):NULL;
              rr;
              rr = rr2, rr2 = rr ? RELOC_REF_NEXT(rr) : NULL)
          {
            t_reloc *rel = RELOC_REF_RELOC(rr);
            if (RELOC_N_TO_SYMBOLS(rel) != 1) continue;
            if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[0]) != blsym) continue;

            /* This is a good relocation. Does it point to our stub? */
            t_relocatable *from = RELOC_FROM(rel);
            ASSERT(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_SUBSECTION,
                   ("expected to come from subsection: @R", rel));
            t_address fromaddr = AddressAdd(RELOCATABLE_CADDRESS(from),
                                            RELOC_FROM_OFFSET(rel));
            t_section *parsec = SECTION_PARENT_SECTION(T_SECTION(from));
            t_uint32 relocated =
              SectionGetData32(parsec,
                               AddressSub(fromaddr, SECTION_CADDRESS(parsec)));
            relocated &= 0xfffffffc;      /* last 2 bits encode part of opcode */
            t_uint64 offset =
              (relocated & 0x02000000) ? (0xfffffffffc000000ll|(t_uint64)relocated):
              (t_uint64) (relocated & 0x03ffffff);
            t_address destination = AddressAddUint64(fromaddr, offset);

            if (!AddressIsEq(destination, SECTION_CADDRESS(sub)))
            {
              continue;   /* nope, not meant for our stub */
            }

            t_uint32 nextins = SectionGetData32(T_SECTION(from),
                AddressAddUint32(RELOC_FROM_OFFSET(rel), 4));
            if (nextins == 0x60000000 || nextins == 0xe8410028)
            {
              t_uint32 index;
              index = (AddressExtractUint32(RELOC_FROM_OFFSET(rel))+4)/4;
              ((t_uint32 *)SECTION_DATA(T_SECTION(from)))[index] = 0xe8410028;
            }
          }
          /* }}} */

          /* look at all relocations to (the subobject copies of) blsym, find
           * those that have to end up referring to this stub
           * {{{ */
          t_bool found = FALSE;
          for (rr = RELOCATABLE_REFED_BY(base); rr; rr = RELOC_REF_NEXT(rr))
          {
            t_reloc *rel = RELOC_REF_RELOC(rr);
            if (RELOC_N_TO_SYMBOLS(rel) != 1) continue;
            if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[0]) != blsym)
              continue;

            /* This is a good relocation. Does it point to our stub? */
            t_relocatable *from = RELOC_FROM(rel);
            ASSERT(RELOCATABLE_RELOCATABLE_TYPE(from) == RT_SUBSECTION,
                   ("expected to come from subsection: @R", rel));
            t_address fromaddr = AddressAdd(RELOCATABLE_CADDRESS(from),
                                            RELOC_FROM_OFFSET(rel));
            t_section *parsec = SECTION_PARENT_SECTION(T_SECTION(from));
            t_uint32 relocated =
              SectionGetData32(parsec,
                               AddressSub(fromaddr, SECTION_CADDRESS(parsec)));
            relocated &= 0xfffffffc;      /* last 2 bits encode part of opcode */
            t_uint64 offset =
              (relocated & 0x02000000) ? (0xfffffffffc000000ll|(t_uint64)relocated):
              (t_uint64) (relocated & 0x03ffffff);
            t_address destination = AddressAddUint64(fromaddr, offset);

            if (!AddressIsEq(destination, SECTION_CADDRESS(sub)))
            {
              continue;   /* nope, not meant for our stub */
            }

            SYMBOL_SET_MAPPED(RELOC_TO_SYMBOL(rel)[0], newblsym);
            found = TRUE;
          }
          /* }}} */

          ASSERT(found,("No referers to stub %s", SYMBOL_NAME(stub->sym)));
        }

        Free(blsymname);
        Free(newblsymname);
        Free(id);
      }
      /* }}} */
      /* }}} */
    }
    else
      FATAL(("implement"));
  }

  /* resolve the .unkgotptr@@ symbols {{{ */
  t_symbol *unkgot =
    SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), ".unkgotptr@@");
  if (unkgot)
  {
    t_section *undef = OBJECT_UNDEF_SECTION(obj);
    t_reloc_ref *rr, *rr2;

    for (rr = SECTION_REFED_BY(undef), rr2 = rr ? RELOC_REF_NEXT(rr) : NULL;
         rr;
         rr = rr2, rr2 = rr ? RELOC_REF_NEXT(rr) : NULL)
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);
      int i;
      for (i = 0; i < RELOC_N_TO_SYMBOLS(rel); ++i)
        if (SYMBOL_MAPPED(RELOC_TO_SYMBOL(rel)[i]) == unkgot)
          break;
      if (i == RELOC_N_TO_SYMBOLS(rel))
        continue;

      /* the ith symbol refers to a TOC pointer */

      /* try the relocation with the different GOT pointers until
       * we find one that delivers the same result as the original
       * value produced by the linker {{{ */
      t_section *fromsec = T_SECTION(RELOC_FROM(rel));
      t_uint32 fromsize = AddressExtractUint32(SECTION_CSIZE(fromsec));
      t_uint32 offset = AddressExtractUint32(RELOC_FROM_OFFSET(rel));
      t_uint64 original =
        SectionGetData64(SECTION_PARENT_SECTION(fromsec),
                         AddressSub(AddressAdd(SECTION_CADDRESS(fromsec),
                                               RELOC_FROM_OFFSET(rel)),
                                    SECTION_CADDRESS(SECTION_PARENT_SECTION(fromsec))));
      char *datacopy = Malloc(fromsize);
      memcpy(datacopy, SECTION_DATA(fromsec), fromsize);
      t_uint64 *resultptr = (t_uint64 *)(datacopy+offset);

      t_symbol *subunkgot = RELOC_TO_SYMBOL(rel)[i];
      t_ppc64_toc *toc;
      for (toc = tocs; toc; toc = toc->next)
      {
        RELOC_TO_SYMBOL(rel)[i] = toc->ptrsym;
        *resultptr = original;
        StackExec(RELOC_CODE(rel), rel, NULL, datacopy, TRUE, 0, obj);
        if (*resultptr == original)
          break;
      }
      RELOC_TO_SYMBOL(rel)[i] = subunkgot;
      Free(datacopy);
      if (!toc)
        FATAL(("could not find appropriate TOC pointer for @R", rel));
      /* }}} */

      /* replace the unkgotptr symbol with one that maps to the
       * correct toc pointer */
      t_symbol *subtoc =
        SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(SECTION_OBJECT(fromsec)),
                             SYMBOL_NAME(toc->ptrsym), "R00$", 0,
                             PERHAPS, TRUE, T_RELOCATABLE(OBJECT_UNDEF_SECTION(obj)),
                             AddressNullForObject(obj),
                             AddressNullForObject(obj), NULL,
                             AddressNullForObject(obj), 0);

      SYMBOL_SET_MAPPED(subtoc, toc->ptrsym);
      RelocSetToSymbol(rel, i, subtoc);
    }
  }
  /* }}} */

  /* {{{ clean up */
  while (tocs)
  {
    t_ppc64_toc *toc = tocs;
    tocs = tocs->next;
    Free(toc);
  }
  while (stubs)
  {
    t_ppc64_stub *stub = stubs;
    stubs = stubs->next;
    Free(stub->assoc_name);
    Free(stub);
  }
  /* }}} */

  return NULL;
}


#endif

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab: */
