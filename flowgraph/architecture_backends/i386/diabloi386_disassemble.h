#include <diabloi386.h>
#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_DISASSEMBLE_FUNCTIONS
#define DIABLOI386_DISASSEMBLE_FUNCTIONS
void I386DisassembleSection(t_section * sec);
t_uint32 I386DisassembleOne(t_i386_ins * ins, t_uint8 * codep, t_reloc ** rels, int max_rel, int curr_rel);
#endif
#endif
