#include <diabloelf.h>

/* Endianness swapping routines */

void Elf32HdrSwitchEndian(Elf32_Ehdr * hdr)
{
   hdr->e_type      = Uint16SwapEndian(hdr->e_type);
   hdr->e_machine   = Uint16SwapEndian(hdr->e_machine);
   hdr->e_version   = Uint32SwapEndian(hdr->e_version);
   hdr->e_entry     = Uint32SwapEndian(hdr->e_entry);
   hdr->e_phoff     = Uint32SwapEndian(hdr->e_phoff);
   hdr->e_shoff     = Uint32SwapEndian(hdr->e_shoff);
   hdr->e_flags     = Uint32SwapEndian(hdr->e_flags);
   hdr->e_ehsize    = Uint16SwapEndian(hdr->e_ehsize);
   hdr->e_phentsize = Uint16SwapEndian(hdr->e_phentsize);
   hdr->e_phnum     = Uint16SwapEndian(hdr->e_phnum);
   hdr->e_shentsize = Uint16SwapEndian(hdr->e_shentsize);
   hdr->e_shnum     = Uint16SwapEndian(hdr->e_shnum);
   hdr->e_shstrndx  = Uint16SwapEndian(hdr->e_shstrndx);
}

void Elf32PhdrSwitchEndian(Elf32_Phdr * phdr)
{
   phdr->p_type = Uint32SwapEndian(phdr->p_type);
   phdr->p_offset = Uint32SwapEndian(phdr->p_offset);
   phdr->p_vaddr = Uint32SwapEndian(phdr->p_vaddr);
   phdr->p_paddr = Uint32SwapEndian(phdr->p_paddr);
   phdr->p_filesz = Uint32SwapEndian(phdr->p_filesz);
   phdr->p_memsz = Uint32SwapEndian(phdr->p_memsz);
   phdr->p_flags = Uint32SwapEndian(phdr->p_flags);
   phdr->p_align = Uint32SwapEndian(phdr->p_align);
}

void Elf64HdrSwitchEndian(Elf64_Ehdr * hdr)
{
   hdr->e_type      = Uint16SwapEndian(hdr->e_type);
   hdr->e_machine   = Uint16SwapEndian(hdr->e_machine);
   hdr->e_version   = Uint32SwapEndian(hdr->e_version);
   hdr->e_entry     = Uint64SwapEndian(hdr->e_entry);
   hdr->e_phoff     = Uint64SwapEndian(hdr->e_phoff);
   hdr->e_shoff     = Uint64SwapEndian(hdr->e_shoff);
   hdr->e_flags     = Uint32SwapEndian(hdr->e_flags);
   hdr->e_ehsize    = Uint16SwapEndian(hdr->e_ehsize);
   hdr->e_phentsize = Uint16SwapEndian(hdr->e_phentsize);
   hdr->e_phnum     = Uint16SwapEndian(hdr->e_phnum);
   hdr->e_shentsize = Uint16SwapEndian(hdr->e_shentsize);
   hdr->e_shnum     = Uint16SwapEndian(hdr->e_shnum);
   hdr->e_shstrndx  = Uint16SwapEndian(hdr->e_shstrndx);
}

void  Elf64ShdrSwitchEndian(Elf64_Shdr * shdr)
{
   shdr->sh_name      = Uint32SwapEndian(shdr->sh_name);
   shdr->sh_type      = Uint32SwapEndian(shdr->sh_type);
   shdr->sh_flags     = Uint64SwapEndian(shdr->sh_flags);
   shdr->sh_addr      = Uint64SwapEndian(shdr->sh_addr);
   shdr->sh_offset    = Uint64SwapEndian(shdr->sh_offset);
   shdr->sh_size      = Uint64SwapEndian(shdr->sh_size);
   shdr->sh_link      = Uint32SwapEndian(shdr->sh_link);
   shdr->sh_info      = Uint32SwapEndian(shdr->sh_info);
   shdr->sh_addralign = Uint64SwapEndian(shdr->sh_addralign);
   shdr->sh_entsize   = Uint64SwapEndian(shdr->sh_entsize);
}

void  Elf32ShdrSwitchEndian(Elf32_Shdr * shdr)
{
   shdr->sh_name      = Uint32SwapEndian(shdr->sh_name);
   shdr->sh_type      = Uint32SwapEndian(shdr->sh_type);
   shdr->sh_flags     = Uint32SwapEndian(shdr->sh_flags);
   shdr->sh_addr      = Uint32SwapEndian(shdr->sh_addr);
   shdr->sh_offset    = Uint32SwapEndian(shdr->sh_offset);
   shdr->sh_size      = Uint32SwapEndian(shdr->sh_size);
   shdr->sh_link      = Uint32SwapEndian(shdr->sh_link);
   shdr->sh_info      = Uint32SwapEndian(shdr->sh_info);
   shdr->sh_addralign = Uint32SwapEndian(shdr->sh_addralign);
   shdr->sh_entsize   = Uint32SwapEndian(shdr->sh_entsize);
}

void  Elf32SymSwitchEndian(Elf32_Sym * sym)
{
   sym->st_name  = Uint32SwapEndian(sym->st_name);
   sym->st_value = Uint32SwapEndian(sym->st_value);
   sym->st_size  = Uint32SwapEndian(sym->st_size);
   sym->st_shndx = Uint16SwapEndian(sym->st_shndx);
}

void  Elf64SymSwitchEndian(Elf64_Sym * sym)
{
   sym->st_name  = Uint32SwapEndian(sym->st_name);
   sym->st_value = Uint64SwapEndian(sym->st_value);
   sym->st_size  = Uint64SwapEndian(sym->st_size);
   sym->st_shndx = Uint16SwapEndian(sym->st_shndx);
}

void  Elf32RelSwitchEndian(Elf32_Rel * rel)
{
   rel->r_offset = Uint32SwapEndian(rel->r_offset);
   rel->r_info   = Uint32SwapEndian(rel->r_info);
}

void  Elf32RelaSwitchEndian(Elf32_Rela * rel)
{
   rel->r_offset = Uint32SwapEndian(rel->r_offset);
   rel->r_info   = Uint32SwapEndian(rel->r_info);
   rel->r_addend = Uint32SwapEndian(rel->r_addend);
}

void  Elf64RelaSwitchEndian(Elf64_Rela * rel)
{
   rel->r_offset = Uint64SwapEndian(rel->r_offset);
   rel->r_info   = Uint64SwapEndian(rel->r_info);
   rel->r_addend = Uint64SwapEndian(rel->r_addend);
}
