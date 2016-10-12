#include <diabloi386.h>
#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_DEFLOWGRAPH_FUNCTIONS
#define DIABLOI386_DEFLOWGRAPH_FUNCTIONS
t_uint32 I386CountBbl(t_bbl * list);
t_bool I386IsListSortedByAddress(t_bbl * list);
void I386PrintList(t_bbl * list);
void I386Deflowgraph(t_object *obj);
t_bbl * I386FindBbl(t_bbl ** list,t_int32 offset);
t_address I386FindAddressBblList(t_bbl ** list,t_int32 offset);
void I386SwapBbl(t_bbl ** list,t_int32 i,t_int32 j);
void I386QuickSort(t_bbl ** list,t_int32 left,t_int32 right);
void I386ClusterChainsForMinimalJumpOffsets(t_chain_holder * ch);
void I386CreateChains(t_cfg * cfg, t_chain_holder * ch);
void I386UpdateControlFlowDisplacement (t_i386_ins *ins);
void I386CalcRelocs(t_object * obj);
t_bool ChainAdjustJumpOffsetsUp (t_bbl *chain);
t_bool ChainAdjustJumpOffsetsDown (t_bbl *chain);
void SetImmedSizes(t_object * obj);
t_bool SetInsSizes(t_object * obj);
void KillUselessJumps(t_object * obj);
t_bool AdjustJumpOffsetsUp(t_object * obj);
t_bool AdjustJumpOffsetsDown(t_object * obj);
void I386UpdateJumpDisplacements(t_object * obj);
void I386SetImmedSizes(t_object * obj);
t_bool I386SetInsSizes(t_object * obj);
void I386KillUselessJumps(t_object * obj);
t_bool I386AdjustJumpOffsetsUp(t_object * obj);
t_bool I386AdjustJumpOffsetsDown(t_object * obj);
void I386DoSetSize(t_i386_ins * ins, t_i386_operand * op);
#endif
#endif
/* vim: set shiftwidth=2: */
