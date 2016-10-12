#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS 
#ifndef DIABLOALPHA_ASSEMBLE_FUNCTIONS
#define DIABLOALPHA_ASSEMBLE_FUNCTIONS
void AlphaAssembleSection(t_section * sec);
t_uint32 AlphaAssembleOne(t_alpha_ins * ins);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
