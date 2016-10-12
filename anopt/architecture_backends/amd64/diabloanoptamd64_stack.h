#ifndef AMD64_STACK_H_
#define AMD64_STACK_H_
#include <diabloflowgraph.h>
void Amd64FunComputeStackSavedRegisters(t_cfg * cfg, t_function * fun);
void Amd64FramePointerAnalysis(t_cfg * cfg);
t_bool Amd64FunIsGlobal(t_function* fun);
int Amd64InsStackDelta(t_amd64_ins *ins, t_bool *unknown);

extern t_dynamic_member_info bbl_stack_delta_array;
extern t_dynamic_member_info bbl_stack_in_array;
extern t_dynamic_member_info bbl_framepointer_offset_array;
extern t_dynamic_member_info bbl_set_framepointer_array;
extern t_dynamic_member_info bbl_reset_to_framepointer_array;

static void StackDeltaInit(t_bbl * b, t_int32 * i) { *i=0; }
static void StackDeltaFini(t_bbl * b, t_int32 * i) {  }
static void StackDeltaDup(t_bbl * b, t_int32 * i) {  }

static void StackInInit(t_bbl * b, t_int32 * i) { *i=0; }
static void StackInFini(t_bbl * b, t_int32 * i) {  }
static void StackInDup(t_bbl * b, t_int32 * i) {  }

static void FramepointerOffsetInit(t_bbl * b, t_int32 * i) { *i=0; }
static void FramepointerOffsetFini(t_bbl * b, t_int32 * i) {  }
static void FramepointerOffsetDup(t_bbl * b, t_int32 * i) {  }

static void SetFramepointerInit(t_bbl * b, t_bool * i) { *i=0; }
static void SetFramepointerFini(t_bbl * b, t_bool * i) {  }
static void SetFramepointerDup(t_bbl * b, t_bool * i) {  }

static void ResetToFramepointerInit(t_bbl * b, t_bool * i) { *i=0; }
static void ResetToFramepointerFini(t_bbl * b, t_bool * i) {  }
static void ResetToFramepointerDup(t_bbl * b, t_bool * i) {  }

DYNAMIC_MEMBER(bbl,t_cfg *,bbl_stack_delta_array,t_int32,stack_delta,STACK_DELTA,StackDelta,CFG_FOREACH_BBL,StackDeltaInit,StackDeltaFini,StackDeltaDup)
DYNAMIC_MEMBER(bbl,t_cfg *,bbl_stack_in_array,t_int32,stack_in,STACK_IN,StackIn,CFG_FOREACH_BBL,StackInInit,StackInFini,StackInDup)
DYNAMIC_MEMBER(bbl,t_cfg *,bbl_framepointer_offset_array,t_int32,framepointer_offset,FRAMEPOINTER_OFFSET,FramepointerOffset,CFG_FOREACH_BBL,FramepointerOffsetInit,FramepointerOffsetFini,FramepointerOffsetDup)
DYNAMIC_MEMBER(bbl,t_cfg *,bbl_set_framepointer_array,t_bool,set_framepointer,SET_FRAMEPOINTER,SetFramepointer,CFG_FOREACH_BBL,SetFramepointerInit,SetFramepointerFini,SetFramepointerDup)
DYNAMIC_MEMBER(bbl,t_cfg *,bbl_reset_to_framepointer_array,t_bool,reset_to_framepointer,RESET_TO_FRAMEPOINTER,ResetToFramepointer,CFG_FOREACH_BBL,ResetToFramepointerInit,ResetToFramepointerFini,ResetToFramepointerDup)

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
