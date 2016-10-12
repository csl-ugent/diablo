/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h> 

#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_ASSEMBLE_ONE_FUNCTIONS
#define ARM_ASSEMBLE_ONE_FUNCTIONS
void ArmAssembleBranch(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSwap(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleDataProc(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSWI(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleMUL(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleStatus(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFLTStatus(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFLTCPDO(t_arm_ins * ins,t_uint32 * instr);
void ArmAssembleFLTMEM(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleLoad(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleStore(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleMultipleTransfer(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleMisc(t_arm_ins * ins,t_uint32 * instr);
void ArmAssembleVFPDP(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleVFPRT(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleVFP2R(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleVFPDT(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleV6Pkh(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleV6Extract(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleV6Sat(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleV6DataProc(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMDImm(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMDDP(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMDTransfer(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleSIMDLoadStore(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFP(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFPLoadStore(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFPDataProc(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleFP2R(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleDivision(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleNOP(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleBitfield(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleHint(t_arm_ins * ins, t_uint32 * instr);
void ArmAssembleBKPT(t_arm_ins * ins, t_uint32 * instr);

#endif
#endif
