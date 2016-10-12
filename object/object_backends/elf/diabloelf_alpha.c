#ifdef BIT64ADDRSUPPORT
#include <diabloobject.h>
#include <diabloelf.h>

/* TODO: verify pagesize, .plt section type, .plt entry size, ptload_align, ptdynamic_align, pt_interp_align, ptnote_align, ptphdr_align, pttls_align */
static const Elf64_HeaderInfo elf_alpha_info =
{
  0x10000,        /*  Elf64_Xword pagesize; */
  SHT_PROGBITS,   /*  Elf64_Xword pltshtype; */
  0x10,           /*  Elf64_Xword pltentsize; */
  0x10000,        /*  Elf64_Xword ptload_align; */
  0x8,            /*  Elf64_Xword ptdynamic_align; */
  0x1,            /*  Elf64_Xword ptinterp_align; */
  0x4,            /*  Elf64_Xword ptnote_align; */
  0x8,            /*  Elf64_Xword ptphdr_align; */
  (Elf32_Word)-1  /*  Elf64_Xword pttls_align; -- keep at -1 if not supported */
};

void
ElfReadAlphaSwitchedEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf64_Shdr *shdrs = NULL;
  Elf64_Sym *symbol_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  char *sechdrstrtab = NULL;
  t_section **sec_table = NULL;
  Elf64_Ehdr hdr64;
  t_symbol **table;
  if (fread ((&hdr64), sizeof (Elf64_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }


  Elf64HdrSwitchEndian (&hdr64);
  /* Reading elf headers does not depend on the type of objectfile */
  /* TODO: verify .plt section type */
  ElfReadCommon64 (fp, &hdr64, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL, TRUE, read_debug, &elf_alpha_info);

  FATAL (("Alpha binaries on a machine with different endianness currently not supported."));
}

void
ElfReadAlphaSameEndian (FILE * fp, t_object * obj, t_bool read_debug)
{
  Elf64_Shdr *shdrs = NULL;
  Elf64_Sym *symbol_table = NULL;
  char *strtab = NULL;
  t_uint32 numsyms = 0;
  char *sechdrstrtab = NULL;
  t_section **sec_table = NULL;
  Elf32_Ehdr hdr64;
  t_symbol **table;
  if (fread ((&hdr64), sizeof (Elf64_Ehdr), 1, fp) != 1)
    {
      FATAL (("Could not read elf header!"));
    }


  /* TODO: verify .plt section type */
  ElfReadCommon64 (fp, &hdr64, obj, &shdrs, &symbol_table, NULL, &numsyms, &strtab,
		   &sechdrstrtab, &sec_table, &table, NULL,  TRUE, read_debug, &elf_alpha_info);

  FATAL (("Alpha binaries on a machine with same endianness currently not supported."));
}
#endif
