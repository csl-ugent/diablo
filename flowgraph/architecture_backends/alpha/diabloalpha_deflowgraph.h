#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS
#ifndef ALPHA_DEFLOWGRAPH_H
#define ALPHA_DEFLOWGRAPH_H

void AlphaDeflowgraph(t_object *obj);
void AlphaCreateChains(t_cfg * cfg, t_chain_holder * chains);
static t_bbl * AlphaChain(t_cfg * cfg);
void AlphaRelocate(t_object * obj);

void AlphaDeflowFixpoint(t_object * obj);
void AlphaLameAlign(t_section * code);

void AlphaUpdateBranchDisplacements(t_object * obj);
void AlphaUpdateControlFlowDisplacement(t_alpha_ins * ins);

void AlphaListFinalProgram(t_bbl * bbl);
static void AlphaComputeCodeSizeGain(t_object * obj);
t_uint32 AlphaKillUselessJumps(t_object * obj);



#endif
#endif

