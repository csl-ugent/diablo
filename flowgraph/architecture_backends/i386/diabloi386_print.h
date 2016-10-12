#include <diabloi386.h>
#ifdef DIABLOI386_FUNCTIONS
#ifndef I386_PRINT_FUNCTIONS
#define I386_PRINT_FUNCTIONS
void I386InstructionPrint(t_i386_ins * data, t_string outputstring) ;
void I386PrintReg(t_i386_operand * op, t_string buf);
#endif
#endif
/* vim: set shiftwidth=2: */
