#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS
#ifndef ALPHA_ASSEMBLE_ONE_FUNCTIONS
#define ALPHA_ASSEMBLE_ONE_FUNCTIONS

void AlphaAssemblePal(t_alpha_ins *, t_uint32 *);
void AlphaAssembleMem(t_alpha_ins *, t_uint32 *);
void AlphaAssembleMemBranch(t_alpha_ins *, t_uint32 *);
void AlphaAssembleOpr(t_alpha_ins *, t_uint32 *);
void AlphaAssembleFp(t_alpha_ins *, t_uint32 *);
void AlphaAssembleMfc(t_alpha_ins *, t_uint32 *);
void AlphaAssembleBranch(t_alpha_ins *, t_uint32 *);

#endif
#endif
