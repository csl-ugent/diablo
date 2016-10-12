#include <diabloi386.h>
#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_OPCODE_CHECK_FUNCTIONS
#define DIABLOI386_OPCODE_CHECK_FUNCTIONS
t_bool I386OpCheckNone(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckReg(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckST(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckConst1(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckA(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckC(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckD(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckE(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckF(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckG(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpChecksI(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckI(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpChecksI2(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckI2(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckJ(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckM(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckO(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckR(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckS(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckX(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckY(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckV(t_i386_operand * op, t_i386_bytemode bm);
t_bool I386OpCheckW(t_i386_operand * op, t_i386_bytemode bm);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
