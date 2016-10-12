#include <diabloamd64.h>
#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_DEFLOWGRAPH_FUNCTIONS
#define DIABLOAMD64_DEFLOWGRAPH_FUNCTIONS
t_uint32 Amd64CountBbl(t_bbl * list);
t_bool Amd64IsListSortedByAddress(t_bbl * list);
void Amd64PrintList(t_bbl * list);
void Amd64Deflowgraph(t_object *obj);
t_bbl * Amd64FindBbl(t_bbl ** list,t_int64 offset);
t_address Amd64FindAddressBblList(t_bbl ** list,t_int64 offset);
void Amd64SwapBbl(t_bbl ** list,t_int64 i,t_int64 j);
void Amd64QuickSort(t_bbl ** list,t_int64 left,t_int64 right);
void Amd64ClusterChainsForMinimalJumpOffsets(t_chain_holder * ch);
void Amd64CreateChains(t_cfg * cfg, t_chain_holder * ch);
void Amd64UpdateControlFlowDisplacement (t_amd64_ins *ins);
void Amd64CalcRelocs(t_object * obj);
t_bool ChainAdjustJumpOffsetsUp (t_bbl *chain);
t_bool ChainAdjustJumpOffsetsDown (t_bbl *chain);
void SetImmedSizes(t_object * obj);
t_bool SetInsSizes(t_object * obj);
void KillUselessJumps(t_object * obj);
t_bool AdjustJumpOffsetsUp(t_object * obj);
t_bool AdjustJumpOffsetsDown(t_object * obj);
void UpdateJumpDisplacements(t_object * obj);
#endif
#endif
/* vim: set shiftwidth=2: */
