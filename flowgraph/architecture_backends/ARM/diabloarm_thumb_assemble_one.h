/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#ifdef DIABLOARM_FUNCTIONS
#ifndef DIABLOARM_THUMB_ASSEMBLE_ONE_FUNCTIONS
#define DIABLOARM_THUMB_ASSEMBLE_ONE_FUNCTIONS
void Thumb32AssembleDataprocImmediate(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleDataprocRegister(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleLoadStore(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleLoadStoreMultiple(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleBranch(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleHint(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleCoproc(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleMultiply(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleStatus(t_arm_ins * ins, t_uint32 * instr);

void Thumb32AssembleVLoadStore(t_arm_ins * ins, t_uint32 * instr);

void ThumbAssembleV6V7ITHints(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleV6V7Status(t_arm_ins * ins, t_uint32 * instr);
void ThumbAssembleV6Extract(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleCondBranch(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleBranchLink(t_arm_ins * ins, t_uint32 * instr);
void ThumbAssembleBranch(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleSWI(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssemble3Bit(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleImm(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleShifted(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleALU(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleHiReg(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleLoadAddress(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleStack(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferImm(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferHalf(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferSign(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferRegOff(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferPC(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleTransferSP(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleMultipleTransfer(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssemblePP(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleData(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleUnsupported(t_arm_ins * ins, t_uint16 * instr);
void ThumbAssembleHint(t_arm_ins * ins, t_uint16 * instr);

void Thumb32AssembleSIMD(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMDImm(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMDDP(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMDTransfer(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleSIMDLoadStore(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleFP2R(t_arm_ins * ins, t_uint32 * instr);
void Thumb32AssembleVFPDP(t_arm_ins * ins, t_uint32 * instr);

#endif
#endif
