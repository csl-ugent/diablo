/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloelf.h>

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_COMMON32_FUNCTIONS
#define DIABLOELF_COMMON32_FUNCTIONS
t_symbol * ElfAddPltSymbol (t_object * obj, t_symbol * sym);
t_symbol * ElfAddGlobDatSymbol (t_object * obj, t_symbol * sym);
void ElfAddDynamicRelativeRelocation(t_object *obj, t_relocatable *from, t_address from_offset);
void *ElfResizeHashSection(t_ast_successors *succ, void *data);
void ElfWriteCommon32(FILE * fp, Elf32_Ehdr * hdr, t_object * obj ,t_bool hoist_headers, t_bool switch_endian, Elf32_HeaderInfo const * const hinfo);
void ElfReadCommon32(FILE * fp, void * hdr, t_object * obj, Elf32_Shdr ** shdr_ret, Elf32_Sym ** symbol_table_ret, Elf32_Sym ** dynamic_symbol_table_ret, t_uint32 * numsyms_ret, char ** strtab_ret, char ** sechdrstrtab_ret, t_section *** sec_ret, t_symbol *** lookup, t_symbol *** dynamic_lookup, t_bool switch_endian, t_bool read_debug, Elf32_HeaderInfo * hinfo);
#endif
#endif
