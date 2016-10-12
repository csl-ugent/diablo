/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#ifdef DIABLOARM_FUNCTIONS
#ifndef THUMB_DISASSEMBLE_ONE_H
#define THUMB_DISASSEMBLE_ONE_H

void Thumb32DisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleHint(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleDataproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleDataprocSaturating(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleImmediate(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleBits(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleControl(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleLoadStoreMultiple(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleDataprocRegister(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleMultiply(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleCoproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

void Thumb32DisassembleVLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

void ThumbDisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleUndefined(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleV6V7ITHints(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleV6V7Status(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleV6Extract(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleCondBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleBranchLink(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleSWI(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleBKPT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassemble3Bit(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleShifted(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleALU(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleHiReg(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleLoadAddress(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleStack(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferHalf(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferSign(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferRegOff(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferPC(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleTransferSP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleMultipleTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassemblePP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleUnsupported(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ThumbDisassembleData(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

void Thumb32DisassembleSIMD(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMDImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMDDP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMDTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleSIMDLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleFPLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleFPDataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void Thumb32DisassembleFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker : */
