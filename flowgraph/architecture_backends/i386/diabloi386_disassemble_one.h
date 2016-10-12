#ifndef DIABLOI386_DISASSEMBLE_ONE_TYPEDEFS
#define DIABLOI386_DISASSEMBLE_ONE_TYPEDEFS
typedef enum {
  bm_NONE = 0,
  bm_AH, bm_AL, bm_AX, bm_EAX, bm_eAX,
  bm_BH, bm_BL, bm_BX, bm_EBX, bm_eBX,
  bm_CH, bm_CL, bm_CX, bm_ECX, bm_eCX,
  bm_DH, bm_DL, bm_DX, bm_EDX, bm_eDX,
  bm_ESI, bm_EDI, bm_ESP, bm_EBP,
  bm_eSI, bm_eDI, bm_eSP, bm_eBP,
  bm_CS, bm_DS, bm_ES, bm_FS, bm_GS, bm_SS,
  bm_ST, bm_ST0, bm_ST1, bm_ST2, bm_ST3, bm_ST4, bm_ST5, bm_ST6, bm_ST7,
  bm_b, bm_w, bm_d, bm_v, bm_q,
  bm_a, bm_p, bm_s,
  bm_sr, bm_dr, bm_er, bm_bcd,
  bm_2byte, bm_28byte, bm_108byte, bm_512byte, bm_unknown, bm_sd, bm_ss, bm_ps, bm_pd, bm_dq
} t_i386_bytemode;
#endif

#include <diabloi386.h>
#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_DISASSEMBLE_ONE_FUNCTIONS
#define DIABLOI386_DISASSEMBLE_ONE_FUNCTIONS
t_uint32 I386OpDisNone(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisReg(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisST(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisConst1(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisA(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisC(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisD(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisE(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisF(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisG(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDissI(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisI(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisJ(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisM(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisO(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisR(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisS(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisX(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisY(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisV(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
t_uint32 I386OpDisW(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm);
#endif
#endif
