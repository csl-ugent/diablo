#ifdef BIT32ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

/* TODO: verify .plt section type, .plt entry size, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf32_HeaderInfo elf_sparc32_info =
{
  0x1000,         /*  Elf32_Word pagesize; */
  SHT_PROGBITS,   /*  Elf32_Word pltshtype; */
  0x4,            /*  Elf32_Word pltentsize; */
  0x10000,        /*  Elf32_Word ptload_align; */
  0x4,            /*  Elf32_Word ptdynamic_align; */
  0x1,            /*  Elf32_Word ptinterp_align; */
  0x4,            /*  Elf32_Word ptnote_align; */
  0x4,            /*  Elf32_Word ptphdr_align; */
  (Elf32_Word)-1  /*  Elf32_Word pttls_align; -- keep at -1 if not supported */
};

void
ElfReadSparcSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf32_Shdr *shdrs = NULL;
  Elf32_Sym *symbol_table = NULL;
  t_section **sec_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
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
		   &sechdrstrtab, &sec_table, &table, NULL, FALSE, read_debug, &elf_sparc32_info);

   FATAL(("Sparc ELF relocations currently not supported"));
}

void ElfReadSparcSwitchedEndian(FILE * fp, void * hdr, void * data, t_object * obj)
{

   FATAL(("Sparc ELF on an machine with different endianness currently not supported. Support for Sparc ELF on machines with the same endianness is very rudementary"));
}
#endif
