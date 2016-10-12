#ifndef DIABLOAMD64_OPERAND_ITERATORS_TYPEDEFS
#define DIABLOAMD64_OPERAND_ITERATORS_TYPEDEFS
typedef struct _t_amd64_immediates t_amd64_immediates;
#endif

#include <diabloamd64.h>

#ifdef DIABLOAMD64_TYPES
#ifndef DIABLOAMD64_OPERAND_ITERATORS_TYPES
#define DIABLOAMD64_OPERAND_ITERATORS_TYPES
struct _t_amd64_immediates{
  t_uint8 size;
  t_uint8 count;
  t_uint64 * imms;
};
#endif
#endif

#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_OPERAND_ITERATORS_FUNCTIONS
#define DIABLOAMD64_OPERAND_ITERATORS_FUNCTIONS
t_bool Amd64OpNextNone(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextReg(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextST(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextConst1(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextA(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextC(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextD(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextE(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextF(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextG(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextsI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextJ(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextM(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextO(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextR(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextS(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextX(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextY(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextV(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
t_bool Amd64OpNextW(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
