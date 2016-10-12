/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_DISASSEMBLE_ONE_H
#define ARM_DISASSEMBLE_ONE_H
void ArmDisassembleBranch(t_arm_ins * ins,  t_uint32 instr, t_uint16 opc);
void ArmDisassembleSWI(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleBKPT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleMultiplication(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleMultipleTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleSwap(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleMRS(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleMSR(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleDataTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleDataProcessing(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleData(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleUnsupported(t_arm_ins * ins, t_uint32 instr, t_uint16 opc );
void ArmDisassembleGenericCoproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleCPDT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleCPRT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleCPDO(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleDSP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleVFPDP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleVFPRT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleVFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleVFPDT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleV6Pkh(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleV6Sat(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleV6Extract(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleV6DataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

void ArmDisassembleSIMD(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMDImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMDDP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMDTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleSIMDLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleNEONFP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleFPLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleFPDataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);

void ArmDisassembleV7DataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleBitfield(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleGenericProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleHint(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleBarrier(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
void ArmDisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
