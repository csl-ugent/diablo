/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifdef DIABLOELF_TYPES
#ifndef DIABLOELF_RAW32_TYPES
#define DIABLOELF_RAW32_TYPES
typedef struct 
{
  Elf32_Ehdr * hdr;
  Elf32_Phdr * phdrs;
  Elf32_Shdr * shdrs;
  char ** sdatas;
} t_elf_raw32;
#endif 
#endif 

#include <diabloelf.h>

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_RAW32_FUNCTIONS
#define DIABLOELF_RAW32_FUNCTIONS
t_elf_raw32 * ElfRaw32Read(FILE *);
void ElfRaw32Write(t_elf_raw32 *, FILE *);
t_address ElfRaw32AddressToFileOffset(t_elf_raw32 *, t_address);
t_address ElfRaw32FileOffsetToAddress(t_elf_raw32 *, t_address);
char * ElfRaw32AddMarkerSection(t_elf_raw32 *, t_const_string, t_address);
char * ElfRaw32AddressToSectionName(t_elf_raw32 *, t_address);
#endif
#endif
