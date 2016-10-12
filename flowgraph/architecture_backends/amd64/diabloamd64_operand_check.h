#include <diabloamd64.h>
#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_OPCODE_CHECK_FUNCTIONS
#define DIABLOAMD64_OPCODE_CHECK_FUNCTIONS
t_bool Amd64OpCheckNone(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckReg(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckST(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckConst1(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckA(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckC(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckD(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckE(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckF(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckG(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpChecksI(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckI(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpChecksI2(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckI2(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckJ(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckM(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckO(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckO8(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckR(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckS(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckX(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckY(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckV(t_amd64_operand * op, t_amd64_bytemode bm);
t_bool Amd64OpCheckW(t_amd64_operand * op, t_amd64_bytemode bm);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
