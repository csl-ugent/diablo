/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/*! \todo Document */
/* arm_opcode_table {{{ */
const arm_opcode arm_opcode_table[] = {
  /* Thumb-2 instructions ; only included for printing purposes */
  {0x00000000, 0xffffffff, "NOP", NULL},
  {0x00000000, 0xffffffff, "YIELD", NULL},
  {0x00000000, 0xffffffff, "WFE", NULL},
  {0x00000000, 0xffffffff, "WFI", NULL},
  {0x00000000, 0xffffffff, "SEV", NULL},
  {0x00000000, 0xffffffff, "IT", NULL},
  {0x00000000, 0xffffffff, "ORN", NULL},
  {0x00000000, 0xffffffff, "CBNZ", NULL},
  {0x00000000, 0xffffffff, "CBZ", NULL},
  {0x00000000, 0xffffffff, "TBB", NULL},
  {0x00000000, 0xffffffff, "TBH", NULL},

  {0xffffffff, 0xf57ff01f, "CLREX", ArmDisassembleGenericProc},
  {0xfffffdff, 0xf1010000, "SETEND", ArmDisassembleGenericProc},

  /* ARMv7 barrier instructions */
  {0xfffffff0, 0xf57ff050, "DMB", ArmDisassembleBarrier},
  {0xfffffff0, 0xf57ff040, "DSB", ArmDisassembleBarrier},
  {0xfffffff0, 0xf57ff060, "ISB", ArmDisassembleBarrier},

  /* ASIMD data-processing instructions - 3 regs, same length
   * Ref: ARM-DDI-0406C.b[A7-262]
   */
  {0xffb00f10, 0xf2000110, "VAND", ArmDisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xf2100110, "VBIC", ArmDisassembleSIMD3RegsSameLength},//DUP: register
  {0xffb00f10, 0xf2200110, "VORR", ArmDisassembleSIMD3RegsSameLength},//DUP: register     /* also VMOV */
  {0xffb00f10, 0xf2300110, "VORN", ArmDisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xf3000110, "VEOR", ArmDisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xf3100110, "VBSL", ArmDisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xf3200110, "VBIT", ArmDisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xf3300110, "VBIF", ArmDisassembleSIMD3RegsSameLength},
  {0xff800f10, 0xf2000800, "VADD", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xf3000800, "VSUB", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xf2000810, "VTST", ArmDisassembleSIMD3RegsSameLength},
  {0xff800f10, 0xf3000810, "VCEQ", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xff800f10, 0xf2000900, "VMLA", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xf3000900, "VMLS", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xf2000b00, "VQDMULH", ArmDisassembleSIMD3RegsSameLength},//DUP-T1
  {0xff800f10, 0xf3000b00, "VQRDMULH", ArmDisassembleSIMD3RegsSameLength},//DUP-T1
  {0xff800f10, 0xf2000b10, "VPADD", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xffa00f10, 0xf2000c10, "VFMA", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xffa00f10, 0xf2200c10, "VFMS", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xffa00f10, 0xf2000d00, "VADD", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xf2200d00, "VSUB", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xf3000d00, "VPADD", ArmDisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xf3200d00, "VABD", ArmDisassembleSIMD3RegsSameLength},//DUP: (floating)
  {0xffa00f10, 0xf2000d10, "VMLA", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xf2200d10, "VMLS", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xf3000d10, "VMUL", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xf2000e00, "VCEQ", ArmDisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xf3000e00, "VCGE", ArmDisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xf3200e00, "VCGT", ArmDisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xf3000e10, "VACGE", ArmDisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xf3200e10, "VACGT", ArmDisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xf2000f00, "VMAX", ArmDisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xf2200f00, "VMIN", ArmDisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xf3000f00, "VPMAX", ArmDisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xf3200f00, "VPMIN", ArmDisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xf2000f10, "VRECPS", ArmDisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xf2200f10, "VRSQRTS", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000000, "VHADD", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000010, "VQADD", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000100, "VRHADD", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000200, "VHSUB", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000210, "VQSUB", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000300, "VCGT", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: register
  {0xfe800f10, 0xf2000310, "VCGE", ArmDisassembleSIMD3RegsSameLength},//DUP-T1: register
  {0xfe800f10, 0xf2000400, "VSHL", ArmDisassembleSIMD3RegsSameLength},//DUP: register
  {0xfe800f10, 0xf2000410, "VQSHL", ArmDisassembleSIMD3RegsSameLength},//DUP: register
  {0xfe800f10, 0xf2000500, "VRSHL", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000510, "VQRSHL", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000600, "VMAX", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xfe800f10, 0xf2000610, "VMIN", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xfe800f10, 0xf2000700, "VABD", ArmDisassembleSIMD3RegsSameLength},//DUP: (integer)
  {0xfe800f10, 0xf2000710, "VABA", ArmDisassembleSIMD3RegsSameLength},
  {0xfe800f10, 0xf2000910, "VMUL", ArmDisassembleSIMD3RegsSameLength},//DUP: integer/polynomial
  {0xfe800f10, 0xf2000a00, "VPMAX", ArmDisassembleSIMD3RegsSameLength},//DUP: integer
  {0xfe800f10, 0xf2000a10, "VPMIN", ArmDisassembleSIMD3RegsSameLength},//DUP: integer

  /* ASIMD data-processing instructions - 1 reg + immediate
   * Ref: ARM-DDI-0406C.b[A7-269]
   */
  {0xfeb80fb0, 0xf2800e30, "VMOV", ArmDisassembleSIMDImm},//cmode=1110,op=1 - vmov (I64)    //DUP: immediate: I64
  {0xfeb80eb0, 0xf2800c30, "VMVN", ArmDisassembleSIMDImm},//cmode=110x,op=1 - vmvn (I32,1)  //DUP: immediate
  {0xfeb80db0, 0xf2800810, "VMOV", ArmDisassembleSIMDImm},//cmode=10x0,op=0 - vmov (I16)    //DUP: imemdiate
  {0xfeb80db0, 0xf2800910, "VORR", ArmDisassembleSIMDImm},//cmode=10x1,op=0 - vorr (I16)    //DUP: immediate
  {0xfeb80db0, 0xf2800830, "VMVN", ArmDisassembleSIMDImm},//cmode=10x0,op=1 - vmvn (I16)    //DUP: immediate
  {0xfeb80db0, 0xf2800930, "VBIC", ArmDisassembleSIMDImm},//cmode=10x1,op=1 - vbic (I16)    //DUP: immediate
  {0xfeb80cb0, 0xf2800c10, "VMOV", ArmDisassembleSIMDImm},//cmode=11xx,op=0 - vmov (I32,11) //DUP: immediate: I32, I8, F32
  {0xfeb809b0, 0xf2800010, "VMOV", ArmDisassembleSIMDImm},//cmode=0xx0,op=0 - vmov (I32)    //DUP: immediate
  {0xfeb809b0, 0xf2800110, "VORR", ArmDisassembleSIMDImm},//cmode=0xx1,op=0 - vorr (I32)    //DUP: immediate
  {0xfeb809b0, 0xf2800030, "VMVN", ArmDisassembleSIMDImm},//cmode=0xx0,op=1 - vmvn (I32)    //DUP: immediate
  {0xfeb809b0, 0xf2800130, "VBIC", ArmDisassembleSIMDImm},//cmode=0xx1,op=1 - vbic (I32)    //DUP: immediate

  /* ASIMD data-processing instructions - 2 regs + shift
   * Ref: ARM-DDI-0406C.b[A7-266]
   */
  {0xff800fd0, 0xf2800810, "VSHRN", ArmDisassembleSIMD2RegsShift},
  {0xff800fd0, 0xf2800850, "VRSHRN", ArmDisassembleSIMD2RegsShift},
  {0xff800fd0, 0xf3800810, "VQSHRUN", ArmDisassembleSIMD2RegsShift},/* U=1, op=0 (results unsigned, operands signed) */
  {0xff800fd0, 0xf3800850, "VQRSHRUN", ArmDisassembleSIMD2RegsShift},/* U=1, op=0 (results unsigned, operands signed) */
  {0xff800f10, 0xf3800410, "VSRI", ArmDisassembleSIMD2RegsShift},
  {0xff800f10, 0xf2800510, "VSHL", ArmDisassembleSIMD2RegsShift},//DUP: immediate
  {0xff800f10, 0xf3800510, "VSLI", ArmDisassembleSIMD2RegsShift},
  {0xfebf0fd0, 0xf2880a10, "VMOVL", ArmDisassembleSIMD2RegsShift},
  {0xfebf0fd0, 0xf2900a10, "VMOVL", ArmDisassembleSIMD2RegsShift},
  {0xfebf0fd0, 0xf2a00a10, "VMOVL", ArmDisassembleSIMD2RegsShift},
  {0xfe800fd0, 0xf2800910, "VQSHRN", ArmDisassembleSIMD2RegsShift},
  {0xfe800fd0, 0xf2800950, "VQRSHRN", ArmDisassembleSIMD2RegsShift},
  {0xfe800fd0, 0xf2800a10, "VSHLL", ArmDisassembleSIMD2RegsShift},//DUP-T1
  {0xfe800f10, 0xf2800010, "VSHR", ArmDisassembleSIMD2RegsShift},
  {0xfe800f10, 0xf2800110, "VSRA", ArmDisassembleSIMD2RegsShift},
  {0xfe800f10, 0xf2800210, "VRSHR", ArmDisassembleSIMD2RegsShift},
  {0xfe800f10, 0xf2800310, "VRSRA", ArmDisassembleSIMD2RegsShift},
  {0xfe800f10, 0xf2800710, "VQSHL", ArmDisassembleSIMD2RegsShift},//DUP: immediate
  {0xfe800f10, 0xf2800610, "VQSHLU", ArmDisassembleSIMD2RegsShift},//U indicates results are unsigned [ARM-DDI-0406C.b, A8-1017]
  {0xfe800e90, 0xf2800e10, "VCVT", ArmDisassembleSIMD2RegsShift},//DUP: floating-fixed

  /* ASIMD data-processing instructions - 2 regs + misc
   * Ref: ARM-DDI-0406C.b[A7-267,268]
   */
  {0xffb30fd0, 0xf3b20200, "VMOVN", ArmDisassembleSIMD2RegsMisc},
  {0xffb30fd0, 0xf3b20240, "VQMOVUN", ArmDisassembleSIMD2RegsMisc},
  {0xffb30fd0, 0xf3b20300, "VSHLL", ArmDisassembleSIMD2RegsMisc},//DUP-T2
  {0xffb30f90, 0xf3b00000, "VREV64", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00080, "VREV32", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00100, "VREV16", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00400, "VCLS", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00480, "VCLZ", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00500, "VCNT", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00580, "VMVN", ArmDisassembleSIMD2RegsMisc},//DUP: register
  {0xffb30f90, 0xf3b00700, "VQABS", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b00780, "VQNEG", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b20000, "VSWP", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b20080, "VTRN", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b20100, "VUZP", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b20180, "VZIP", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xf3b20280, "VQMOVN", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f10, 0xf3b00200, "VPADDL", ArmDisassembleSIMD2RegsMisc},
  {0xffb30f10, 0xf3b00600, "VPADAL", ArmDisassembleSIMD2RegsMisc},
  {0xffb30ed0, 0xf3b20600, "VCVT", ArmDisassembleSIMD2RegsMisc},//DUP: half-single
  {0xffb30e90, 0xf3b30400, "VRECPE", ArmDisassembleSIMD2RegsMisc},
  {0xffb30e90, 0xf3b30480, "VRSQRTE", ArmDisassembleSIMD2RegsMisc},
  {0xffb30e10, 0xf3b30600, "VCVT", ArmDisassembleSIMD2RegsMisc},//DUP: floating-integer
  {0xffb30b90, 0xf3b10000, "VCGT", ArmDisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xf3b10080, "VCGE", ArmDisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xf3b10100, "VCEQ", ArmDisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xf3b10180, "VCLE", ArmDisassembleSIMD2RegsMisc},
  {0xffb30b90, 0xf3b10200, "VCLT", ArmDisassembleSIMD2RegsMisc},
  {0xffb30b90, 0xf3b10300, "VABS", ArmDisassembleSIMD2RegsMisc},//DUP-T1: integer
  {0xffb30b90, 0xf3b10380, "VNEG", ArmDisassembleSIMD2RegsMisc},//DUP-T1: integer

  {0xffb00f90, 0xf3b00c00, "VDUP", ArmDisassembleSIMD},//DUP: scalar
  {0xffb00c50, 0xf3b00800, "VTBL", ArmDisassembleSIMD},
  {0xffb00c50, 0xf3b00840, "VTBX", ArmDisassembleSIMD},
  {0xffb00010, 0xf2b00000, "VEXT", ArmDisassembleSIMD},

  /* ASIMD data-processing instructions - 3 regs, different length
   * Ref: ARM-DDI-0406C.b[A7-264]
   */
  {0xff800f50, 0xf2800400, "VADDHN", ArmDisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xf3800400, "VRADDHN", ArmDisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xf2800600, "VSUBHN", ArmDisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xf3800600, "VRSUBHN", ArmDisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xf2800900, "VQDMLAL", ArmDisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xff800f50, 0xf2800b00, "VQDMLSL", ArmDisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xff800f50, 0xf2800d00, "VQDMULL", ArmDisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xfe800f50, 0xf2800000, "VADDL", ArmDisassembleSIMD3RegsDifferentLength},
  {0xfe800f50, 0xf2800100, "VADDW", ArmDisassembleSIMD3RegsDifferentLength},
  {0xfe800f50, 0xf2800200, "VSUBL", ArmDisassembleSIMD3RegsDifferentLength},
  {0xfe800f50, 0xf2800300, "VSUBW", ArmDisassembleSIMD3RegsDifferentLength},
  {0xfe800f50, 0xf2800500, "VABAL", ArmDisassembleSIMD3RegsDifferentLength},//T2
  {0xfe800f50, 0xf2800700, "VABDL", ArmDisassembleSIMD3RegsDifferentLength},
  {0xfe800f50, 0xf2800800, "VMLAL", ArmDisassembleSIMD3RegsDifferentLength},//DUP: integer
  {0xfe800f50, 0xf2800a00, "VMLSL", ArmDisassembleSIMD3RegsDifferentLength},//DUP: integer
  {0xfe800f50, 0xf2800c00, "VMULL", ArmDisassembleSIMD3RegsDifferentLength},//DUP-T2: integer
  {0xfe800f50, 0xf2800e00, "VMULL", ArmDisassembleSIMD3RegsDifferentLength},//DUP-T2: polynomial

  /* ASIMD data-processing instructions - 2 regs + scalar
   * Ref: ARM-DDI-0406C.b[A7-265]
   */
  {0xff800f50, 0xf2800340, "VQDMLAL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xff800f50, 0xf2800740, "VQDMLSL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xff800f50, 0xf2800b40, "VQDMULL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800f50, 0xf2800240, "VMLAL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800f50, 0xf2800640, "VMLSL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800f50, 0xf2800a40, "VMULL", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800f50, 0xf2800c40, "VQDMULH", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800f50, 0xf2800d40, "VQRDMULH", ArmDisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xfe800e50, 0xf2800040, "VMLA", ArmDisassembleSIMD2RegsScalar},//DUP-T1: scalar
  {0xfe800e50, 0xf2800440, "VMLS", ArmDisassembleSIMD2RegsScalar},//DUP-T1: scalar
  {0xfe800e50, 0xf2800840, "VMUL", ArmDisassembleSIMD2RegsScalar},//DUP-T1: scalar

  /* ASIMD element or structure load/store instructions
   * Ref: ARM-DDI-0406C.b[A7-275]
   */
  {0xffb00700, 0xf4000200, "VST1", ArmDisassembleSIMDLoadStore},//DUP - multiple single elements - 4 registers
  {0xffb00e00, 0xf4000600, "VST1", ArmDisassembleSIMDLoadStore},//DUP - 1 of 3 registers
  {0xffb00f00, 0xf4000300, "VST2", ArmDisassembleSIMDLoadStore},//DUP - multiple 2-element structures - 2 registers
  {0xffb00e00, 0xf4000800, "VST2", ArmDisassembleSIMDLoadStore},//DUP - 1 register
  {0xffb00e00, 0xf4000400, "VST3", ArmDisassembleSIMDLoadStore},//DUP-multiple 3-element structures
  {0xffb00e00, 0xf4000000, "VST4", ArmDisassembleSIMDLoadStore},//DUP-multiple 4-element structures
  {0xffb00300, 0xf4800000, "VST1", ArmDisassembleSIMDLoadStore},//DUP-single element from one lane - size=8|16
  {0xffb00300, 0xf4800100, "VST2", ArmDisassembleSIMDLoadStore},//DUP-single 2-element structure from one lane - size=8|16
  {0xffb00300, 0xf4800200, "VST3", ArmDisassembleSIMDLoadStore},//DUP-single 3-element sturcture from one lane - size=8|16
  {0xffb00300, 0xf4800300, "VST4", ArmDisassembleSIMDLoadStore},//DUP-single 4-element structure from one lane - size=8|16
  {0xffb00700, 0xf4200200, "VLD1", ArmDisassembleSIMDLoadStore},//DUP-multiple single elements - 4 registers
  {0xffb00e00, 0xf4200600, "VLD1", ArmDisassembleSIMDLoadStore},//DUP - 1 or 3 registers
  {0xffb00f00, 0xf4200300, "VLD2", ArmDisassembleSIMDLoadStore},//DUP-multiple 2-element structures - 2 registers
  {0xffb00e00, 0xf4200800, "VLD2", ArmDisassembleSIMDLoadStore},//DUP - 1 register
  {0xffb00e00, 0xf4200400, "VLD3", ArmDisassembleSIMDLoadStore},//DUP-multiple 3-element structures
  {0xffb00e00, 0xf4200000, "VLD4", ArmDisassembleSIMDLoadStore},//DUP-multiple 4-element structures
  {0xffb00f00, 0xf4a00c00, "VLD1", ArmDisassembleSIMDLoadStore},//DUP-single element to all lanes
  {0xffb00300, 0xf4a00000, "VLD1", ArmDisassembleSIMDLoadStore},//DUP-single element to one lane
  {0xffb00f00, 0xf4a00d00, "VLD2", ArmDisassembleSIMDLoadStore},//DUP-single 2-element structure to all lanes
  {0xffb00300, 0xf4a00100, "VLD2", ArmDisassembleSIMDLoadStore},//DUP-single 2-element structure to one lane - size=8|16
  {0xffb00f00, 0xf4a00e00, "VLD3", ArmDisassembleSIMDLoadStore},//DUP-single 3-element structure to all lanes
  {0xffb00300, 0xf4a00200, "VLD3", ArmDisassembleSIMDLoadStore},//DUP-single 3-element structure to one lane - size=8|16
  {0xffb00f00, 0xf4a00f00, "VLD4", ArmDisassembleSIMDLoadStore},//DUP-single 4-element structure to all lanes
  {0xffb00300, 0xf4a00300, "VLD4", ArmDisassembleSIMDLoadStore},//DUP-single 4-element structure to one lane - size=8|16

  /* ARMv7 instructions */
  {0x0fffffff, 0x0320f000, "NOP", ArmDisassembleGenericProc},
  {0x0fffffff, 0x0320f001, "YIELD", ArmDisassembleHint},
  {0x0fffffff, 0x0320f002, "WFE", ArmDisassembleHint},
  {0x0fffffff, 0x0320f003, "WFI", ArmDisassembleHint},
  {0x0fffffff, 0x0320f004, "SEV", ArmDisassembleHint},
  {0x0ffffff0, 0x0320f0f0, "DBG", ArmDisassembleHint},
  {0x0ffffff0, 0x01600070, "SMC", ArmDisassembleGenericProc},

  {0x0fff0ff0, 0x06ff0f30, "RBIT", ArmDisassembleBitfield},
  {0x0fe0007f, 0x07c0001f, "BFC", ArmDisassembleBitfield},
  {0x0fe00070, 0x07c00010, "BFI", ArmDisassembleBitfield},
  {0x0fe00070, 0x07a00050, "SBFX", ArmDisassembleBitfield},
  {0x0fe00070, 0x07e00050, "UBFX", ArmDisassembleBitfield},

  {0x0ff00fff, 0x01900f9f, "LDREX", ArmDisassembleDataTransfer},
  {0x0ff00fff, 0x01b00f9f, "LDREXD", ArmDisassembleDataTransfer},
  {0x0ff00fff, 0x01d00f9f, "LDREXB", ArmDisassembleDataTransfer},
  {0x0ff00fff, 0x01f00f9f, "LDREXH", ArmDisassembleDataTransfer},
  {0x0ff00ff0, 0x01800f90, "STREX", ArmDisassembleDataTransfer},
  {0x0ff00ff0, 0x01a00f90, "STREXD", ArmDisassembleDataTransfer},
  {0x0ff00ff0, 0x01c00f90, "STREXB", ArmDisassembleDataTransfer},
  {0x0ff00ff0, 0x01e00f90, "STREXH", ArmDisassembleDataTransfer},

  {0xfe5fffe0, 0xf84d0500, "SRS", ArmDisassembleNotImplemented},
  {0xfe50ffff, 0xf8100a00, "RFE", ArmDisassembleNotImplemented},
  {0x0ff000f0, 0x01400070, "HVC", ArmDisassembleNotImplemented},
  {0x0fffffff, 0x0160006e, "ERET", ArmDisassembleNotImplemented},

  /* ARM V6 instructions. */
  {0xfffdfe20, 0xf1080000, "CPSIE", ArmDisassembleGenericProc},
  {0xfffdfe20, 0xf10c0000, "CPSID", ArmDisassembleGenericProc},
  {0xfff1fe20, 0xf1000000, "CPS", ArmDisassembleGenericProc},

  {0x0ff00000, 0x03400000, "MOVT", ArmDisassembleDataProcessing},
  {0x0ff00000, 0x03000000, "MOVW", ArmDisassembleDataProcessing},
  {0x0ff00070, 0x06800010, "PKHBT", ArmDisassembleV6Pkh},
  {0x0ff00070, 0x06800050, "PKHTB", ArmDisassembleV6Pkh},
  {0x0fff0ff0, 0x06bf0f30, "REV", ArmDisassembleV6DataProc},
  {0x0fff0ff0, 0x06bf0fb0, "REV16", ArmDisassembleV6DataProc},
  {0x0fff0ff0, 0x06ff0fb0, "REVSH", ArmDisassembleV6DataProc},
  {0x0fff03f0, 0x06bf0070, "SXTH", ArmDisassembleV6Extract},
  {0x0fff03f0, 0x068f0070, "SXTB16", ArmDisassembleV6Extract},
  {0x0fff03f0, 0x06af0070, "SXTB", ArmDisassembleV6Extract},
  {0x0fff03f0, 0x06ff0070, "UXTH", ArmDisassembleV6Extract},
  {0x0fff03f0, 0x06cf0070, "UXTB16", ArmDisassembleV6Extract},
  {0x0fff03f0, 0x06ef0070, "UXTB", ArmDisassembleV6Extract},
  {0x0ff00ff0, 0x06200f10, "QADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06200f90, "QADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06200f30, "QASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06200f70, "QSUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06200ff0, "QSUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06200f50, "QSAX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100f10, "SADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100f90, "SADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100f30, "SASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300f10, "SHADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300f90, "SHADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300f30, "SHASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300f70, "SHSUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300ff0, "SHSUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06300f50, "SHSAX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100f70, "SSUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100ff0, "SSUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06100f50, "SSAX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500f10, "UADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500f90, "UADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500f30, "UASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700f10, "UHADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700f90, "UHADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700f30, "UHASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700f70, "UHSUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700ff0, "UHSUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06700f50, "UHSAX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600f10, "UQADD16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600f90, "UQADD8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600f30, "UQASX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600f70, "UQSUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600ff0, "UQSUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06600f50, "UQSAX", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500f70, "USUB16", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500ff0, "USUB8", ArmDisassembleV6DataProc},
  {0x0ff00ff0, 0x06500f50, "USAX", ArmDisassembleV6DataProc},
  {0x0ff003f0, 0x06b00070, "SXTAH", ArmDisassembleV6Extract},
  {0x0ff003f0, 0x06800070, "SXTAB16", ArmDisassembleV6Extract},
  {0x0ff003f0, 0x06a00070, "SXTAB", ArmDisassembleV6Extract},
  {0x0ff003f0, 0x06f00070, "UXTAH", ArmDisassembleV6Extract},
  {0x0ff003f0, 0x06c00070, "UXTAB16", ArmDisassembleV6Extract},
  {0x0ff003f0, 0x06e00070, "UXTAB", ArmDisassembleV6Extract},
  {0x0ff00ff0, 0x06800fb0, "SEL", ArmDisassembleV6DataProc},
  {0x0ff0f0f0, 0x0700f010, "SMUAD", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0700f030, "SMUADX", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0700f050, "SMUSD", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0700f070, "SMUSDX", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07000010, "SMLAD", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07000030, "SMLADX", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07400010, "SMLALD", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07400030, "SMLALDX", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07000050, "SMLSD", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07000070, "SMLSDX", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07400050, "SMLSLD", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07400070, "SMLSLDX", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0750f010, "SMMUL", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0750f030, "SMMULR", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07500010, "SMMLA", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x07500030, "SMMLAR", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x075000d0, "SMMLS", ArmDisassembleMultiplication},
  {0x0ff000f0, 0x075000f0, "SMMLSR", ArmDisassembleMultiplication},
  {0x0fe00030, 0x06a00010, "SSAT", ArmDisassembleV6Sat},
  {0x0ff00ff0, 0x06a00f30, "SSAT16", ArmDisassembleV6Sat},
  {0x0ff000f0, 0x00400090, "UMAAL", ArmDisassembleMultiplication},
  {0x0ff0f0f0, 0x0780f010, "USAD8", ArmDisassembleV6DataProc},
  {0x0ff000f0, 0x07800010, "USADA8", ArmDisassembleV6DataProc},
  {0x0fe00030, 0x06e00010, "USAT", ArmDisassembleV6Sat},
  {0x0ff00ff0, 0x06e00f30, "USAT16", ArmDisassembleV6Sat},

  /* ARMv7 division instructions */
  {0x0ff0f0f0, 0x0710f010, "SDIV", ArmDisassembleV7DataProc},
  {0x0ff0f0f0, 0x0730f010, "UDIV", ArmDisassembleV7DataProc},

  /* preload memory system hint */
  {0xfd70f000, 0xf550f000, "PLD", ArmDisassembleDataTransfer},
  {0xfd70f000, 0xf510f000, "PLDW", ArmDisassembleDataTransfer},
  {0xfd70f000, 0xf450f000, "PLI", ArmDisassembleDataTransfer},

  /* Branches */
  {0x0f000000, 0x0a000000, "B", ArmDisassembleBranch},
  {0x0f000000, 0x0b000000, "BL", ArmDisassembleBranch},
  {0x0ffffff0, 0x012fff10, "BX", ArmDisassembleBranch},
  {0x0ffffff0, 0x012fff30, "BLX", ArmDisassembleBranch},

  /* Software Interrupt */
  {0x0f000000, 0x0f000000, "SWI", ArmDisassembleSWI},
  {0x0ff000f0, 0x01200070, "BKPT", ArmDisassembleBKPT},

  /* Saturated arithmetic (Enhanced DSP Extension) */
  {0x0ff00ff0, 0x01000050, "QADD", ArmDisassembleDSP},
  {0x0ff00ff0, 0x01400050, "QDADD", ArmDisassembleDSP},
  {0x0ff00ff0, 0x01600050, "QDSUB", ArmDisassembleDSP},
  {0x0ff00ff0, 0x01200050, "QSUB", ArmDisassembleDSP},

  {0x0ff000f0, 0x01000080, "SMLABB", ArmDisassembleDSP},
  {0x0ff000f0, 0x010000c0, "SMLABT", ArmDisassembleDSP},
  {0x0ff000f0, 0x010000a0, "SMLATB", ArmDisassembleDSP},
  {0x0ff000f0, 0x010000e0, "SMLATT", ArmDisassembleDSP},

  {0x0ff000f0, 0x01200080, "SMLAWB", ArmDisassembleDSP},
  {0x0ff000f0, 0x012000c0, "SMLAWT", ArmDisassembleDSP},

  {0x0ff000f0, 0x01400080, "SMLALBB", ArmDisassembleDSP},
  {0x0ff000f0, 0x014000c0, "SMLALBT", ArmDisassembleDSP},
  {0x0ff000f0, 0x014000a0, "SMLALTB", ArmDisassembleDSP},
  {0x0ff000f0, 0x014000e0, "SMLALTT", ArmDisassembleDSP},

  {0x0ff0f0f0, 0x01600080, "SMULBB", ArmDisassembleDSP},
  {0x0ff0f0f0, 0x016000c0, "SMULBT", ArmDisassembleDSP},
  {0x0ff0f0f0, 0x016000a0, "SMULTB", ArmDisassembleDSP},
  {0x0ff0f0f0, 0x016000e0, "SMULTT", ArmDisassembleDSP},

  {0x0ff0f0f0, 0x012000a0, "SMULWB", ArmDisassembleDSP},
  {0x0ff0f0f0, 0x012000e0, "SMULWT", ArmDisassembleDSP},

  /* Multiplication */
  {0x0ff000f0, 0x00600090, "MLS", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00000090, "MUL", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00200090, "MLA", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00800090, "UMULL", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00a00090, "UMLAL", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00c00090, "SMULL", ArmDisassembleMultiplication},
  {0x0fe000f0, 0x00e00090, "SMLAL", ArmDisassembleMultiplication},

   /* Swap memory and register instructions */
  {0x0ff00ff0, 0x01000090, "SWP", ArmDisassembleSwap},
  {0x0ff00ff0, 0x01400090, "SWPB", ArmDisassembleSwap},

  /* Status register to general register Transfer instructions */
  {0x0fbf0fff, 0x010f0000, "MRS", ArmDisassembleMRS},
  {0x0fb00eff, 0x01000200, "MRS(banked)", ArmDisassembleNotImplemented},

  /* General register to status register Transfer instructions */
  {0x0db0f000, 0x0120f000, "MSR", ArmDisassembleMSR},
  {0x0fb0fef0, 0x0120f200, "MSR(banked)", ArmDisassembleNotImplemented},

  /* Half-word and signed byte data Transfer instructions,
   * mind that the order is important: otherwise, some of them are recognized as
   * dataprocessing instructions*/

  {0x0e1000f0, 0x000000b0, "STRH", ArmDisassembleDataTransfer},
  {0x0e1000f0, 0x001000b0, "LDRH", ArmDisassembleDataTransfer},
/*    {0x0e1000f0, 0x000000f0, "STRSH", ArmDisassembleDataTransfer}, */
  {0x0e1000f0, 0x001000f0, "LDRSH", ArmDisassembleDataTransfer},
/*    {0x0e1000f0, 0x000000d0, "STRSB", ArmDisassembleDataTransfer}, */
  {0x0e1000f0, 0x001000d0, "LDRSB", ArmDisassembleDataTransfer},

  /* Double word data transfer instructions */
  {0x0e1000f0, 0x000000d0, "LDRD", ArmDisassembleDataTransfer},
  {0x0e1000f0, 0x000000f0, "STRD", ArmDisassembleDataTransfer},

  /* Data Manipulation */
  {0x0de00000, 0x00000000, "AND", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00200000, "EOR", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00400000, "SUB", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00600000, "RSB", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00800000, "ADD", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00a00000, "ADC", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00c00000, "SBC", ArmDisassembleDataProcessing},
  {0x0de00000, 0x00e00000, "RSC", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01000000, "TST", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01200000, "TEQ", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01400000, "CMP", ArmDisassembleDataProcessing},
  {0x0fff0ff0, 0x016f0f10, "CLZ", ArmDisassembleDataProcessing},
  {0x0df00000, 0x01700000, "CMN", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01800000, "ORR", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01a00000, "MOV", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01c00000, "BIC", ArmDisassembleDataProcessing},
  {0x0de00000, 0x01e00000, "MVN", ArmDisassembleDataProcessing},

  /* Single word and unsigned byte data Transfer instructions */
  {0x0c500000, 0x04000000, "STR", ArmDisassembleDataTransfer},
  {0X0c500000, 0x04100000, "LDR", ArmDisassembleDataTransfer},
  {0x0c500000, 0x04400000, "STRB", ArmDisassembleDataTransfer},
  {0X0c500000, 0x04500000, "LDRB", ArmDisassembleDataTransfer},

  /* Multiple register Transfer instructions */
  {0x0e100000, 0x08000000, "STM", ArmDisassembleMultipleTransfer},
  {0x0e100000, 0x08100000, "LDM", ArmDisassembleMultipleTransfer},

  /* Coprocessor instructions - overgenomen uit de GNU binutils */

  /* Floating point coprocessor datatransfer: see ARM DDI 0077B = ARM7500FE Data Sheet */
  {0x0ff08f10, 0x0e000100, "ADF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e100100, "MUF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e200100, "SUF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e300100, "RSF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e400100, "DVF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e500100, "RDF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e600100, "POW", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e700100, "RPW", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e800100, "RMF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e900100, "FML", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0ea00100, "FDV", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0eb00100, "FRD", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0ec00100, "POL", ArmDisassembleCPDO},

  {0x0ff08f10, 0x0e008100, "MVF", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e108100, "MNF", ArmDisassembleCPDO},

  {0x0ff08f10, 0x0e208100, "ABS", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e308100, "RND", ArmDisassembleCPDO},

  {0x0ff08f10, 0x0e408100, "SQT", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0e508100, "LOG", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0e608100, "LGN", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0e708100, "EXP", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0e808100, "SIN", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0e908100, "COS", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0ea08100, "TAN", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0eb08100, "ASN", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0ec08100, "ACS", ArmDisassembleUnsupported},
  {0x0ff08f10, 0x0ed08100, "ATN", ArmDisassembleUnsupported},

  {0x0ff08f10, 0x0ee08100, "URD", ArmDisassembleCPDO},
  {0x0ff08f10, 0x0ef08100, "NRM", ArmDisassembleCPDO},
  {0x0ff00f1f, 0x0e000110, "FLT", ArmDisassembleCPRT},
  {0x0fff0f98, 0x0e100110, "FIX", ArmDisassembleCPRT},
  {0x0fff0fff, 0x0e200110, "WFS", ArmDisassembleCPRT},
  {0x0fff0fff, 0x0e300110, "RFS", ArmDisassembleCPRT},
  {0x0fff0fff, 0x0e400110, "WFC", ArmDisassembleCPRT},
  {0x0fff0fff, 0x0e500110, "RFC", ArmDisassembleCPRT},

  {0x0ff8fff0, 0x0e90f110, "CMF", ArmDisassembleCPRT},
  {0x0ff8fff0, 0x0eb0f110, "CNF", ArmDisassembleCPRT},
  {0x0ff8fff0, 0x0ed0f110, "CMFE", ArmDisassembleCPRT},
  {0x0ff8fff0, 0x0ef0f110, "CNFE", ArmDisassembleCPRT},

  /* Floating point coprocessor datatransfer */
  {0x0e100f00, 0x0c000100, "STF", ArmDisassembleCPDT}, /* Store float (p. 10 - 2) */
  {0x0e100f00, 0x0c100100, "LDF", ArmDisassembleCPDT}, /* Load float (p. 10 - 2) */
  {0x0e100f00, 0x0c000200, "SFM", ArmDisassembleCPDT}, /* Store float multiple (p. 10 - 4) */
  {0x0e100f00, 0x0c100200, "LFM", ArmDisassembleCPDT}, /* Load float multiple (p. 10 -4) */

  /* 8, 16 and 32-bit transfer between ARM core and extension registers
   * Ref: ARM-DDI-0406C.b[A7-278]
   */
  {0x0ff00f7f, 0x0e000a10, "VMOV", ArmDisassembleSIMDTransfer},//DUP: core -> single
  {0x0fff0fff, 0x0ee10a10, "VMSR", ArmDisassembleSIMDTransfer},
  {0x0f900f1f, 0x0e000b10, "VMOV", ArmDisassembleSIMDTransfer},//DUP: core -> scalar
  {0x0f900f5f, 0x0e800b10, "VDUP", ArmDisassembleSIMD},//DUP: core
  {0x0ff00f7f, 0x0e100a10, "VMOV", ArmDisassembleSIMDTransfer},//DUP: single -> core
  {0x0fff0fff, 0x0ef10a10, "VMRS", ArmDisassembleSIMDTransfer},
  {0x0f100f1f, 0x0e100b10, "VMOV", ArmDisassembleSIMDTransfer},//DUP: scalar -> core

  /* Floating-point data-processing instructions - 3 regs
   * Ref: ARM-DDI-0406C.b[A7-272]
   */
  {0x0fb00e50, 0x0e000a00, "VMLA", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0e000a40, "VMLS", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0e100a00, "VNMLA", ArmDisassembleFPDataProc},
  {0x0fb00e50, 0x0e100a40, "VNMLS", ArmDisassembleFPDataProc},
  {0x0fb00e50, 0x0e200a40, "VNMUL", ArmDisassembleFPDataProc},
  {0x0fb00e50, 0x0e200a00, "VMUL", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0e300a00, "VADD", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0e300a40, "VSUB", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0e800a00, "VDIV", ArmDisassembleFPDataProc},
  {0x0fb00e50, 0x0e900a00, "VFNMA", ArmDisassembleFPDataProc},//T2
  {0x0fb00e50, 0x0e900a40, "VFNMS", ArmDisassembleFPDataProc},//T2
  {0x0fb00e50, 0x0ea00a00, "VFMA", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fb00e50, 0x0ea00a40, "VFMS", ArmDisassembleFPDataProc},//DUP-T2: floating

  /* Floating-point other data-processing instructions
   * Ref: ARM-DDI-0406C.b[A7-272,273]
   */
  {0x0fb00ef0, 0x0eb00a00, "VMOV", ArmDisassembleVFPDP},//DUP-T2: immediate, floating
  {0x0fbf0ed0, 0x0eb00a40, "VMOV", ArmDisassembleFPDataProc},//DUP-T2: register, floating
  {0x0fbf0ed0, 0x0eb00ac0, "VABS", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fbf0ed0, 0x0eb10a40, "VNEG", ArmDisassembleFPDataProc},//DUP-T2: floating
  {0x0fbf0ed0, 0x0eb10ac0, "VSQRT", ArmDisassembleFPDataProc},
  {0x0fbe0ed0, 0x0eb40a40, "VCMP", ArmDisassembleFPDataProc},//T1 of T2
  {0x0fbe0ed0, 0x0eb40ac0, "VCMPE", ArmDisassembleFPDataProc},//T1 of T2
  {0x0fbe0fd0, 0x0eb20a40, "VCVTB", ArmDisassembleFPDataProc},
  {0x0fbe0fd0, 0x0eb20ac0, "VCVTT", ArmDisassembleFPDataProc},
  {0x0fbf0ed0, 0x0eb70ac0, "VCVT", ArmDisassembleFPDataProc},   //DUP:double/single
  {0x0fbf0e50, 0x0eb80a40, "VCVT", ArmDisassembleFPDataProc},  //DUP:integer->floating
  {0x0fbe0e50, 0x0eba0a40, "VCVT", ArmDisassembleFPDataProc},   //DUP:fixed->floating
  {0x0fbe0e50, 0x0ebe0a40, "VCVT", ArmDisassembleFPDataProc},   //DUP:floating->fixed
  {0x0fbe0ed0, 0x0ebc0ac0, "VCVT", ArmDisassembleFPDataProc},   //DUP:floating->integer
  {0x0fbe0ed0, 0x0ebc0a40, "VCVTR", ArmDisassembleFPDataProc},  //DUP:floating->integer

  /* 64-bit transfers between ARM core and extension registers
   * Ref: ARM-DDI-0406C.b[A7-279]
   */
  {0x0fe00fd0, 0x0c400a10, "VMOV", ArmDisassembleFP2R},//DUP: core -> single
  {0x0fe00fd0, 0x0c400b10, "VMOV", ArmDisassembleFP2R},//DUP: core -> double

  /* Extension register load/store instructions
   * Ref: ARM-DDI-0406C.b[A7-274]
   */
  {0x0fbf0e00, 0x0d2d0a00, "VPUSH", ArmDisassembleFPLoadStore},//T1 of T2
  {0x0fbf0e00, 0x0cbd0a00, "VPOP", ArmDisassembleFPLoadStore},//T1 of T2
  {0x0f300e00, 0x0d000a00, "VSTR", ArmDisassembleFPLoadStore},//T1 of T2
  {0x0f300e00, 0x0d100a00, "VLDR", ArmDisassembleFPLoadStore},//T1 of T2
  {0x0e100f01, 0x0c000b01, "FSTMX", ArmDisassembleVFPDT},
  {0x0e100e00, 0x0c000a00, "VSTM", ArmDisassembleFPLoadStore},//DUP-T1 of T2
  {0x0e100f01, 0x0c100b01, "FLDMX", ArmDisassembleVFPDT},
  {0x0e100e00, 0x0c100a00, "VLDM", ArmDisassembleFPLoadStore},//DUP-T1 of T2, increment after, no writeback

  /* ==============================================================================================
   * OLD FLOATING-POINT INSTRUCTIONS BELOW - These are the pre-UAL mnemonics
   */

  /* Vector floating point coprocessor (VFP) instructions (based on binutils table, the ** ones were unfolded) */
  /* Vector floating point coprocessor (VFP) data processing instructions */
  {0x0fbf0fd0, 0x0eb00a40, "FCPYS", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb00ac0, "FABSS", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb00b40, "FCPYD", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb00bc0, "FABSD", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb10a40, "FNEGS", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb10ac0, "FSQRTS", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb10b40, "FNEGD", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb10bc0, "FSQRTD", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb70ac0, "FCVTDS", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb70bc0, "FCVTSD", ArmDisassembleVFPDP},
  {0x0fbf0fd0, 0x0eb40a40, "FCMPS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb40b40, "FCMPD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb40ac0, "FCMPES", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb40bc0, "FCMPED", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb50a40, "FCMPZS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb50b40, "FCMPZD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb50ac0, "FCMPEZS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb50bc0, "FCMPEZD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb80a40, "FUITOS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb80b40, "FUITOD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb80ac0, "FSITOS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0eb80bc0, "FSITOD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebc0a40, "FTOUIS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebc0b40, "FTOUID", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebc0ac0, "FTOUIZS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebc0bc0, "FTOUIZD", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebd0a40, "FTOSIS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebd0b40, "FTOSID", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebd0ac0, "FTOSIZS", ArmDisassembleVFPDP}, /**/
  {0x0fbf0fd0, 0x0ebd0bc0, "FTOSIZD", ArmDisassembleVFPDP}, /**/
/* can't find documentation for them
  {0x0fb00ff0, 0x0eb00a00, "FCONSTS", ArmDisassembleVFPDP},
  {0x0fb00ff0, 0x0eb00b00, "FCONSTD", ArmDisassembleVFPDP},
*/
  {0x0fb00f50, 0x0e000a00, "FMACS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e000a40, "FNMACS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e000b00, "FMACD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e000b40, "FNMACD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e100a00, "FMSCS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e100a40, "FNMSCS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e100b00, "FMSCD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e100b40, "FNMSCD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e200a00, "FMULS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e200a40, "FNMULS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e200b00, "FMULD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e200b40, "FNMULD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e300a00, "FADDS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e300a40, "FSUBS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e300b00, "FADDD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e300b40, "FSUBD", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e800a00, "FDIVS", ArmDisassembleVFPDP},
  {0x0fb00f50, 0x0e800b00, "FDIVD", ArmDisassembleVFPDP},

  /* Vector floating point coprocessor (VFP) single register transfer instructions */
/* special cases of generic forms below
  {0x0fff0fff, 0x0ee00a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ee10a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ee60a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ee70a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ee80a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ee90a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0eea0a10, "FMXR", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef00a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef10a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef60a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef70a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef80a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0ef90a10, "FMRX", ArmDisassembleVFPRT},
  {0x0fff0fff, 0x0efa0a10, "FMRX", ArmDisassembleVFPRT},
*/
  {0x0fffffff, 0x0ef1fa10, "FMSTAT", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0e000b10, "FMDLR", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0e100b10, "FMRDL", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0e200b10, "FMDHR", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0e300b10, "FMRDH", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0ee00a10, "FMXR", ArmDisassembleVFPRT},
  {0x0ff00fff, 0x0ef00a10, "FMRX", ArmDisassembleVFPRT},
  {0x0ff00f7f, 0x0e000a10, "FMSR", ArmDisassembleVFPRT},
  {0x0ff00f7f, 0x0e100a10, "FMRS", ArmDisassembleVFPRT},

  /* Vector floating point coprocessor (VFP) two-register transfer instructions */
  {0x0ff00ff0, 0x0c500b10, "FMRRD", ArmDisassembleVFP2R},
  {0x0ff00fd0, 0x0c400a10, "FMSRR", ArmDisassembleVFP2R},
  {0x0ff00fd0, 0x0c400b10, "FMDRR", ArmDisassembleVFP2R},
  {0x0ff00fd0, 0x0c500a10, "FMRRS", ArmDisassembleVFP2R},

  /* Vector floating point coprocessor (VFP) data transfer instructions */
  {0x0f300f00, 0x0d000a00, "FSTS", ArmDisassembleVFPDT},
  {0x0f700f00, 0x0d000b00, "FSTD", ArmDisassembleVFPDT},
  {0x0f300f00, 0x0d100a00, "FLDS", ArmDisassembleVFPDT},
  {0x0f700f00, 0x0d100b00, "FLDD", ArmDisassembleVFPDT},
  {0x0e100f00, 0x0c000a00, "FSTMS", ArmDisassembleVFPDT},
  {0x0e500f01, 0x0c000b00, "FSTMD", ArmDisassembleVFPDT},
//  {0x0e500f01, 0x0c000b01, "FSTMX", ArmDisassembleVFPDT},
  {0x0e100f00, 0x0c100a00, "FLDMS", ArmDisassembleVFPDT},
  {0x0e500f01, 0x0c100b00, "FLDMD", ArmDisassembleVFPDT},
//  {0x0e500f01, 0x0c100b01, "FLDMX", ArmDisassembleVFPDT},

  /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   * OLD FLOATING-POINT INSTRUCTIONS ABOVE - These are the pre-UAL mnemonics
   */

  /* Generic coprocessor instructions (catch-alls) */
  {0xfff00000, 0xfc500000, "MRRC2", ArmDisassembleGenericCoproc}, /* Move to double reg from coproc */
  {0xfff00000, 0xfc400000, "MCRR2", ArmDisassembleGenericCoproc}, /* Move to coproc from double reg */
  {0xff100010, 0xfe100010, "MRC2", ArmDisassembleGenericCoproc}, /* Move to reg from coproc */
  {0xff100010, 0xfe000010, "MCR2", ArmDisassembleGenericCoproc}, /* Move to coproc from reg */
  {0xff000010, 0xfe000000, "CDP2", ArmDisassembleGenericCoproc},
  {0xfe100000, 0xfc000000, "STC2", ArmDisassembleGenericCoproc}, /* Store floating Point (p. 10 - 2) */
  {0xfe100000, 0xfc100000, "LDC2", ArmDisassembleGenericCoproc}, /* Load floating Point (p. 10 -2 ) */

  {0x0ff00000, 0x0c500000, "MRRC", ArmDisassembleGenericCoproc}, /* Move to double reg from coproc */
  {0x0ff00000, 0x0c400000, "MCRR", ArmDisassembleGenericCoproc}, /* Move to coproc from double reg */
  {0x0f100010, 0x0e100010, "MRC", ArmDisassembleGenericCoproc}, /* Move to reg from coproc */
  {0x0f100010, 0x0e000010, "MCR", ArmDisassembleGenericCoproc}, /* Move to coproc from reg */
  {0x0f000010, 0x0e000000, "CDP", ArmDisassembleGenericCoproc}, /* FP CP Data operations (p. 10-7) */
  {0x0e100000, 0x0c000000, "STC", ArmDisassembleGenericCoproc}, /* Store floating Point (p. 10 - 2) */
  {0x0e100000, 0x0c100000, "LDC", ArmDisassembleGenericCoproc}, /* Load floating Point (p. 10 -2 ) */

  /* opvang voor niet-herkende instructies */
  {0x00000000, 0x00000000, "UNDEF", ArmDisassembleUnsupported}
};
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker : */
