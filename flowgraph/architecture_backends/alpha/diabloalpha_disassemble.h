#include <diabloalpha.h>
#ifdef DIABLOALPHA_FUNCTIONS
#ifndef DIABLOALPHA_DISASSEMBLE_FUNCTIONS
#define DIABLOALPHA_DISASSEMBLE_FUNCTIONS
void * AlphaDisassembleOneInstruction(t_object * obj, t_address start, int * size_ret);
void * AlphaDisassembleOneInstructionForSection(t_section * sec, t_uint32 offset, int * size_ret);
void AlphaDisassembleSection(t_section * code);
#endif
#endif
/* vim: set shiftwidth=2: */
