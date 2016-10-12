#include <diabloelf.h>

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_ENDIAN_FUNCTIONS
#define DIABLOELF_ENDIAN_FUNCTIONS
void Elf32HdrSwitchEndian(Elf32_Ehdr * hdr);
void Elf32PhdrSwitchEndian(Elf32_Phdr * hdr);
void Elf32ShdrSwitchEndian(Elf32_Shdr * shdr);
void Elf32SymSwitchEndian(Elf32_Sym * sym);
void Elf32RelSwitchEndian(Elf32_Rel * rel);
void Elf32RelaSwitchEndian(Elf32_Rela * rel);
void Elf64HdrSwitchEndian(Elf64_Ehdr * hdr);
void Elf64ShdrSwitchEndian(Elf64_Shdr * shdr);
void Elf64SymSwitchEndian(Elf64_Sym * sym);
void Elf64RelaSwitchEndian(Elf64_Rela * rel);
#endif
#endif
