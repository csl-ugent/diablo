#ifndef DIABLOAMD64_DISASSEMBLE_ONE_TYPEDEFS
#define DIABLOAMD64_DISASSEMBLE_ONE_TYPEDEFS
typedef enum {
  amd64_bm_NONE = 0,
  amd64_bm_AHrex, amd64_bm_AH, amd64_bm_ALrex, amd64_bm_AL, amd64_bm_AXrex, amd64_bm_AX, amd64_bm_EAX, amd64_bm_RAX, amd64_bm_rAXrex, amd64_bm_eAX, amd64_bm_rAX, amd64_bm_RAXrex,
  amd64_bm_BHrex, amd64_bm_BH, amd64_bm_BLrex, amd64_bm_BL, amd64_bm_BXrex, amd64_bm_BX, amd64_bm_EBX, amd64_bm_RBX, amd64_bm_rBXrex, amd64_bm_eBX, amd64_bm_rBX, amd64_bm_RBXrex,
  amd64_bm_CHrex, amd64_bm_CH, amd64_bm_CLrex, amd64_bm_CL, amd64_bm_CXrex, amd64_bm_CX, amd64_bm_ECX, amd64_bm_RCX, amd64_bm_rCXrex, amd64_bm_eCX, amd64_bm_rCX, amd64_bm_RCXrex,
  amd64_bm_DHrex, amd64_bm_DH, amd64_bm_DLrex, amd64_bm_DL, amd64_bm_DXrex, amd64_bm_DX, amd64_bm_EDX, amd64_bm_RDX, amd64_bm_rDXrex, amd64_bm_eDX, amd64_bm_rDX, amd64_bm_RDXrex,
  amd64_bm_SI , amd64_bm_DI , amd64_bm_SP , amd64_bm_BP ,
  amd64_bm_ESI, amd64_bm_EDI, amd64_bm_ESP, amd64_bm_EBP,
  amd64_bm_RSI, amd64_bm_RDI, amd64_bm_RSP, amd64_bm_RBP,
  amd64_bm_rSI, amd64_bm_rDI, amd64_bm_rSP, amd64_bm_rBP,
  amd64_bm_eSI, amd64_bm_eDI, amd64_bm_eSP, amd64_bm_eBP,
  amd64_bm_rSIrex, amd64_bm_rDIrex, amd64_bm_rSPrex, amd64_bm_rBPrex,
  amd64_bm_RSIrex, amd64_bm_RDIrex, amd64_bm_RSPrex, amd64_bm_RBPrex,
  amd64_bm_R8B, amd64_bm_R9B, amd64_bm_R10B, amd64_bm_R11B, amd64_bm_R12B, amd64_bm_R13B, amd64_bm_R14B, amd64_bm_R15B,  
  amd64_bm_R8W, amd64_bm_R9W, amd64_bm_R10W, amd64_bm_R11W, amd64_bm_R12W, amd64_bm_R13W, amd64_bm_R14W, amd64_bm_R15W,
  amd64_bm_R8D, amd64_bm_R9D, amd64_bm_R10D, amd64_bm_R11D, amd64_bm_R12D, amd64_bm_R13D, amd64_bm_R14D, amd64_bm_R15D,
  amd64_bm_R8 , amd64_bm_R9 , amd64_bm_R10 , amd64_bm_R11 , amd64_bm_R12 , amd64_bm_R13 , amd64_bm_R14 , amd64_bm_R15 ,
  amd64_bm_r8,  amd64_bm_r9,  amd64_bm_r10,  amd64_bm_r11,  amd64_bm_r12,  amd64_bm_r13,  amd64_bm_r14,  amd64_bm_r15 ,
  amd64_bm_e8,  amd64_bm_e9,  amd64_bm_e10,  amd64_bm_e11,  amd64_bm_e12,  amd64_bm_e13,  amd64_bm_e14,  amd64_bm_e15 ,
  amd64_bm_CS, amd64_bm_DS, amd64_bm_ES, amd64_bm_FS, amd64_bm_GS, amd64_bm_SS,
  amd64_bm_ST, amd64_bm_ST0, amd64_bm_ST1, amd64_bm_ST2, amd64_bm_ST3, amd64_bm_ST4, amd64_bm_ST5, amd64_bm_ST6, amd64_bm_ST7,
  amd64_bm_b, amd64_bm_w, amd64_bm_d, amd64_bm_v, amd64_bm_v8, amd64_bm_v48, amd64_bm_z, amd64_bm_q,
  amd64_bm_a, amd64_bm_p, amd64_bm_s,
  amd64_bm_sr, amd64_bm_dr, amd64_bm_er, amd64_bm_bcd,
  amd64_bm_2byte, amd64_bm_28byte, amd64_bm_108byte, amd64_bm_512byte, amd64_bm_unknown, amd64_bm_sd, amd64_bm_ss, amd64_bm_ps, amd64_bm_pd, amd64_bm_dq
} t_amd64_bytemode;
#endif

#include <diabloamd64.h>
#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_DISASSEMBLE_ONE_FUNCTIONS
#define DIABLOAMD64_DISASSEMBLE_ONE_FUNCTIONS
t_uint32 Amd64OpDisNone(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisReg(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisST(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisConst1(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisA(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisC(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisD(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisE(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisF(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisG(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDissI(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisI(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisJ(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisM(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisO(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisO8(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisR(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisS(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisX(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisY(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisV(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
t_uint32 Amd64OpDisW(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex);
#endif
#endif
