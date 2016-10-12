#include <diabloelf.h>

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_COMMON64_FUNCTIONS
#define DIABLOELF_COMMON64_FUNCTIONS
void ElfWriteCommonSameEndian64 (FILE * fp, Elf64_Ehdr * hdr, t_object * obj, t_bool hoist_headers, Elf64_HeaderInfo const * const hinfo);
void ElfReadCommon64 (FILE * fp, void *hdr, t_object * obj, Elf64_Shdr ** shdr_ret, Elf64_Sym ** symbol_table_ret, Elf64_Sym ** dynamic_symbol_table_ret, t_uint32 * numsyms_ret, char ** strtab_ret, char ** sechdrstrtab_ret, t_section *** sec_ret, t_symbol *** lookup, t_symbol *** dynamic_lookup, t_bool switch_endian, t_bool read_debug, Elf64_HeaderInfo const * const hinfo);
#endif
#endif
