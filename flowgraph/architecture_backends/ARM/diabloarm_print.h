/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#ifdef DIABLOARM_FUNCTIONS
#ifndef DIABLOARM_PRINT_FUNCTIONS
#define DIABLOARM_PRINT_FUNCTIONS
void ArmInsPrint(t_ins * data, t_string outputstring) ;
t_bool ArmInsPrintSIMD(t_arm_ins * ins, t_string opcode, t_string operands);
t_bool ArmInsPrintCoproc(t_arm_ins * ins, t_string opcode, t_string operands);
t_bool ArmInsPrintDiabloSpecific(t_arm_ins * instruction, t_string opcode, t_string operands);
void ArmInsPrintVLoadStore(t_arm_ins * ins, t_string opcode, t_string operands);
void ArmPrintVirtualRegisters(t_bool b);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
