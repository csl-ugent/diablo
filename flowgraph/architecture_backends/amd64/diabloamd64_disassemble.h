#include <diabloamd64.h>
#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_DISASSEMBLE_FUNCTIONS
#define DIABLOAMD64_DISASSEMBLE_FUNCTIONS

#define rexw(rex)               (rex&0x08)
#define rexr(rex)               (rex&0x04)
#define rexx(rex)               (rex&0x02)
#define rexb(rex)               (rex&0x01)


void Amd64DisassembleSection(t_section * sec);
t_uint32 Amd64DisassembleOne(t_amd64_ins * ins, t_uint8 * codep, t_reloc ** rels, int max_rel, int curr_rel);
#endif
#endif
