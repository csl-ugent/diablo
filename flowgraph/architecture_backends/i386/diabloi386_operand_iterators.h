#ifndef DIABLOI386_OPERAND_ITERATORS_TYPEDEFS
#define DIABLOI386_OPERAND_ITERATORS_TYPEDEFS
typedef struct _t_immediates t_immediates;
#endif

#include <diabloi386.h>

#ifdef DIABLOI386_TYPES
#ifndef DIABLOI386_OPERAND_ITERATORS_TYPES
#define DIABLOI386_OPERAND_ITERATORS_TYPES
struct _t_immediates{
  t_uint8 size;
  t_uint8 count;
  t_uint32 * imms;
};
#endif
#endif

#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_OPERAND_ITERATORS_FUNCTIONS
#define DIABLOI386_OPERAND_ITERATORS_FUNCTIONS
t_bool I386OpNextNone(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextReg(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextST(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextConst1(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextA(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextC(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextD(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextE(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextF(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextG(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextsI(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextI(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextJ(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextM(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextO(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextR(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextS(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextX(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextY(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextV(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
t_bool I386OpNextW(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
