/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef I386_STACK_H_
#define I386_STACK_H_
#include <diabloflowgraph.h>
void I386FunComputeStackSavedRegisters(t_cfg * cfg, t_function * fun);
void I386FramePointerAnalysis(t_cfg * cfg);
t_bool I386FunIsGlobal(t_function* fun);
int I386InsStackDelta(t_i386_ins *ins, t_bool *unknown);

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(stack_delta, STACK_DELTA, StackDelta, t_int32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(stack_in, STACK_IN, StackIn, t_int32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(framepointer_offset, FRAMEPOINTER_OFFSET, FramepointerOffset, t_int32, {*valp=0;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(set_framepointer, SET_FRAMEPOINTER, SetFramepointer, t_bool, {*valp=FALSE;}, {}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(reset_to_framepointer, RESET_TO_FRAMEPOINTER, ResetToFramepointer, t_bool, {*valp=FALSE;}, {}, {});

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
