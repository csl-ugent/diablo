/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/*! \todo Document */
/* thumb_opcode_table {{{ */
const thumb_opcode thumb_opcode_table[] = {
	/* 32-bit thumb instruction encoding */

	{0xffffffff, 0xf3bf8f0f, "LEAVEX", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xffffffff, 0xf3bf8f1f, "ENTERX", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xffffffff, 0xf3bf8f2f, "CLREX", ARM_CLREX, Thumb32DisassembleLoadStoreExclusive},

	{0xffffffff, 0xf3af8000, "NOP", ARM_NOP, Thumb32DisassembleHint},
	{0xffffffff, 0xf3af8001, "YIELD", ARM_YIELD, Thumb32DisassembleHint},
	{0xffffffff, 0xf3af8002, "WFE", ARM_WFE, Thumb32DisassembleHint},
	{0xffffffff, 0xf3af8003, "WFI", ARM_WFI, Thumb32DisassembleHint},
	{0xffffffff, 0xf3af8004, "SEV", ARM_SEV, Thumb32DisassembleHint},
	{0xfffffff0, 0xf3af80f0, "DBG", ARM_DBG, Thumb32DisassembleHint},
	{0xfffffff0, 0xf3bf8f40, "DSB", ARM_DSB, Thumb32DisassembleHint},
	{0xfffffff0, 0xf3bf8f50, "DMB", ARM_DMB, Thumb32DisassembleHint},
	{0xfffffff0, 0xf3bf8f60, "ISB", ARM_ISB, Thumb32DisassembleHint},

  /* ASIMD data-processing instructions - 3 regs, same length
   * Ref: ARM-DDI-0406C.b[A7-262]
   */
  {0xffb00f10, 0xef000110, "VAND", ARM_VAND, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xef100110, "VBIC", ARM_VBIC, Thumb32DisassembleSIMD3RegsSameLength},//DUP: register
  {0xffb00f10, 0xef200110, "VORR", ARM_VORR, Thumb32DisassembleSIMD3RegsSameLength},//DUP: register     /* also VMOV */
  {0xffb00f10, 0xef300110, "VORN", ARM_VORN, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xff000110, "VEOR", ARM_VEOR, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xff100110, "VBSL", ARM_VBSL, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xff200110, "VBIT", ARM_VBIT, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffb00f10, 0xff300110, "VBIF", ARM_VBIF, Thumb32DisassembleSIMD3RegsSameLength},
  {0xff800f10, 0xef000800, "VADD", ARM_VADD, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xff000800, "VSUB", ARM_VSUB, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xef000810, "VTST", ARM_VTST, Thumb32DisassembleSIMD3RegsSameLength},
  {0xff800f10, 0xff000810, "VCEQ", ARM_VCEQ, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xff800f10, 0xef000900, "VMLA", ARM_VMLA, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xff000900, "VMLS", ARM_VMLS, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xff800f10, 0xef000b00, "VQDMULH", ARM_VQDMULH, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1
  {0xff800f10, 0xff000b00, "VQRDMULH", ARM_VQRDMULH, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1
  {0xff800f10, 0xef000b10, "VPADD", ARM_VPADD, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xffa00f10, 0xef000c10, "VFMA", ARM_VFMA, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xffa00f10, 0xef200c10, "VFMS", ARM_VFMS, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: integer
  {0xffa00f10, 0xef000d00, "VADD", ARM_VADD_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xef200d00, "VSUB", ARM_VSUB_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xff000d00, "VPADD", ARM_VPADD_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xff200d00, "VABD", ARM_VABD_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: (floating)
  {0xffa00f10, 0xef000d10, "VMLA", ARM_VMLA_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xef200d10, "VMLS", ARM_VMLS_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xff000d10, "VMUL", ARM_VMUL_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: floating
  {0xffa00f10, 0xef000e00, "VCEQ", ARM_VCEQ_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xff000e00, "VCGE", ARM_VCGE_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xff200e00, "VCGT", ARM_VCGT_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T2: floating
  {0xffa00f10, 0xff000e10, "VACGE", ARM_VACGE_F, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xff200e10, "VACGT", ARM_VACGT_F, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xef000f00, "VMAX", ARM_VMAX_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xef200f00, "VMIN", ARM_VMIN_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xff000f00, "VPMAX", ARM_VPMAX_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xff200f00, "VPMIN", ARM_VPMIN_F, Thumb32DisassembleSIMD3RegsSameLength},//DUP: floating
  {0xffa00f10, 0xef000f10, "VRECPS", ARM_VRECPS_F, Thumb32DisassembleSIMD3RegsSameLength},
  {0xffa00f10, 0xef200f10, "VRSQRTS", ARM_VRSQRTS_F, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000000, "VHADD", ARM_VHADD, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000010, "VQADD", ARM_VQADD, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000100, "VRHADD", ARM_VRHADD, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000200, "VHSUB", ARM_VHSUB, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000210, "VQSUB", ARM_VQSUB, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000300, "VCGT", ARM_VCGT, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: register
  {0xef800f10, 0xef000310, "VCGE", ARM_VCGE, Thumb32DisassembleSIMD3RegsSameLength},//DUP-T1: register
  {0xef800f10, 0xef000400, "VSHL", ARM_VSHL, Thumb32DisassembleSIMD3RegsSameLength},//DUP: register
  {0xef800f10, 0xef000410, "VQSHL", ARM_VQSHL, Thumb32DisassembleSIMD3RegsSameLength},//DUP: register
  {0xef800f10, 0xef000500, "VRSHL", ARM_VRSHL, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000510, "VQRSHL", ARM_VQRSHL, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000600, "VMAX", ARM_VMAX, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xef800f10, 0xef000610, "VMIN", ARM_VMIN, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xef800f10, 0xef000700, "VABD", ARM_VABD, Thumb32DisassembleSIMD3RegsSameLength},//DUP: (integer)
  {0xef800f10, 0xef000710, "VABA", ARM_VABA, Thumb32DisassembleSIMD3RegsSameLength},
  {0xef800f10, 0xef000910, "VMUL", ARM_VMUL, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer/polynomial
  {0xef800f10, 0xef000a00, "VPMAX", ARM_VPMAX, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer
  {0xef800f10, 0xef000a10, "VPMIN", ARM_VPMIN, Thumb32DisassembleSIMD3RegsSameLength},//DUP: integer

  /* ASIMD data-processing instructions - 1 reg + immediate
   * Ref: ARM-DDI-0406C.b[A7-269]
   */
  {0xefb80fb0, 0xef800e30, "VMOV", ARM_VMOV_IMM_3, Thumb32DisassembleSIMDImm},//cmode=1110,op=1 - vmov (I64)    //DUP: immediate: I64
  {0xefb80eb0, 0xef800c30, "VMVN", ARM_VMVN_IMM_2, Thumb32DisassembleSIMDImm},//cmode=110x,op=1 - vmvn (I32,1)  //DUP: immediate
  {0xefb80db0, 0xef800810, "VMOV", ARM_VMOV_IMM_1, Thumb32DisassembleSIMDImm},//cmode=10x0,op=0 - vmov (I16)    //DUP: imemdiate
  {0xefb80db0, 0xef800910, "VORR", ARM_VORR_IMM_1, Thumb32DisassembleSIMDImm},//cmode=10x1,op=0 - vorr (I16)    //DUP: immediate
  {0xefb80db0, 0xef800830, "VMVN", ARM_VMVN_IMM_1, Thumb32DisassembleSIMDImm},//cmode=10x0,op=1 - vmvn (I16)    //DUP: immediate
  {0xefb80db0, 0xef800930, "VBIC", ARM_VBIC_IMM_1, Thumb32DisassembleSIMDImm},//cmode=10x1,op=1 - vbic (I16)    //DUP: immediate
  {0xefb80cb0, 0xef800c10, "VMOV", ARM_VMOV_IMM_2, Thumb32DisassembleSIMDImm},//cmode=11xx,op=0 - vmov (I32,11) //DUP: immediate: I32, I8, F32
  {0xefb809b0, 0xef800010, "VMOV", ARM_VMOV_IMM_0, Thumb32DisassembleSIMDImm},//cmode=0xx0,op=0 - vmov (I32)    //DUP: immediate
  {0xefb809b0, 0xef800110, "VORR", ARM_VORR_IMM_0, Thumb32DisassembleSIMDImm},//cmode=0xx1,op=0 - vorr (I32)    //DUP: immediate
  {0xefb809b0, 0xef800030, "VMVN", ARM_VMVN_IMM_0, Thumb32DisassembleSIMDImm},//cmode=0xx0,op=1 - vmvn (I32)    //DUP: immediate
  {0xefb809b0, 0xef800130, "VBIC", ARM_VBIC_IMM_0, Thumb32DisassembleSIMDImm},//cmode=0xx1,op=1 - vbic (I32)    //DUP: immediate

  /* ASIMD data-processing instructions - 2 regs + shift
   * Ref: ARM-DDI-0406C.b[A7-266]
   */
  {0xff800fd0, 0xef800810, "VSHRN", ARM_VSHRN, Thumb32DisassembleSIMD2RegsShift},
  {0xff800fd0, 0xef800850, "VRSHRN", ARM_VRSHRN, Thumb32DisassembleSIMD2RegsShift},
  {0xff800fd0, 0xff800810, "VQSHRUN", ARM_VQSHRUN, Thumb32DisassembleSIMD2RegsShift},/* U=1, op=0 (results unsigned, operands signed) */
  {0xff800fd0, 0xff800850, "VQRSHRUN", ARM_VQRSHRUN, Thumb32DisassembleSIMD2RegsShift},/* U=1, op=0 (results unsigned, operands signed) */
  {0xff800f10, 0xff800410, "VSRI", ARM_VSRI, Thumb32DisassembleSIMD2RegsShift},
  {0xff800f10, 0xef800510, "VSHL", ARM_VSHL_IMM, Thumb32DisassembleSIMD2RegsShift},//DUP: immediate
  {0xff800f10, 0xff800510, "VSLI", ARM_VSLI, Thumb32DisassembleSIMD2RegsShift},
  {0xefbf0fd0, 0xef880a10, "VMOVL", ARM_VMOVL1, Thumb32DisassembleSIMD2RegsShift},
  {0xefbf0fd0, 0xef900a10, "VMOVL", ARM_VMOVL2, Thumb32DisassembleSIMD2RegsShift},
  {0xefbf0fd0, 0xefa00a10, "VMOVL", ARM_VMOVL3, Thumb32DisassembleSIMD2RegsShift},
  {0xef800fd0, 0xef800910, "VQSHRN", ARM_VQSHRN, Thumb32DisassembleSIMD2RegsShift},
  {0xef800fd0, 0xef800950, "VQRSHRN", ARM_VQRSHRN, Thumb32DisassembleSIMD2RegsShift},
  {0xef800fd0, 0xef800a10, "VSHLL", ARM_VSHLL_IMM, Thumb32DisassembleSIMD2RegsShift},//DUP-T1
  {0xef800f10, 0xef800010, "VSHR", ARM_VSHR, Thumb32DisassembleSIMD2RegsShift},
  {0xef800f10, 0xef800110, "VSRA", ARM_VSRA, Thumb32DisassembleSIMD2RegsShift},
  {0xef800f10, 0xef800210, "VRSHR", ARM_VRSHR, Thumb32DisassembleSIMD2RegsShift},
  {0xef800f10, 0xef800310, "VRSRA", ARM_VRSRA, Thumb32DisassembleSIMD2RegsShift},
  {0xef800f10, 0xef800710, "VQSHL", ARM_VQSHL_IMM, Thumb32DisassembleSIMD2RegsShift},//DUP: immediate
  {0xef800f10, 0xef800610, "VQSHLU", ARM_VQSHLU_IMM, Thumb32DisassembleSIMD2RegsShift},//U indicates results are unsigned [ARM-DDI-0406C.b, A8-1017]
  {0xef800e90, 0xef800e10, "VCVT", ARM_VCVT_FX, Thumb32DisassembleSIMD2RegsShift},//DUP: floating-fixed

  /* ASIMD data-processing instructions - 2 regs + misc
   * Ref: ARM-DDI-0406C.b[A7-267,268]
   */
  {0xffb30fd0, 0xffb20200, "VMOVN", ARM_VMOVN, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30fd0, 0xffb20240, "VQMOVUN", ARM_VQMOVUN, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30fd0, 0xffb20300, "VSHLL", ARM_VSHLL, Thumb32DisassembleSIMD2RegsMisc},//DUP-T2
  {0xffb30f90, 0xffb00000, "VREV64", ARM_VREV64, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00080, "VREV32", ARM_VREV32, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00100, "VREV16", ARM_VREV16, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00400, "VCLS", ARM_VCLS, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00480, "VCLZ", ARM_VCLZ, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00500, "VCNT", ARM_VCNT, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00580, "VMVN", ARM_VMVN, Thumb32DisassembleSIMD2RegsMisc},//DUP: register
  {0xffb30f90, 0xffb00700, "VQABS", ARM_VQABS, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb00780, "VQNEG", ARM_VQNEG, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb20000, "VSWP", ARM_VSWP, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb20080, "VTRN", ARM_VTRN, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb20100, "VUZP", ARM_VUZP, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb20180, "VZIP", ARM_VZIP, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f90, 0xffb20280, "VQMOVN", ARM_VQMOVN, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f10, 0xffb00200, "VPADDL", ARM_VPADDL, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30f10, 0xffb00600, "VPADAL", ARM_VPADAL, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30ed0, 0xffb20600, "VCVT", ARM_VCVT_HS, Thumb32DisassembleSIMD2RegsMisc},//DUP: half-single
  {0xffb30e90, 0xffb30400, "VRECPE", ARM_VRECPE, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30e90, 0xffb30480, "VRSQRTE", ARM_VRSQRTE, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30e10, 0xffb30600, "VCVT", ARM_VCVT_FI, Thumb32DisassembleSIMD2RegsMisc},//DUP: floating-integer
  {0xffb30b90, 0xffb10000, "VCGT", ARM_VCGT_IMM, Thumb32DisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xffb10080, "VCGE", ARM_VCGE_IMM, Thumb32DisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xffb10100, "VCEQ", ARM_VCEQ_IMM, Thumb32DisassembleSIMD2RegsMisc},//DUP: immediate
  {0xffb30b90, 0xffb10180, "VCLE", ARM_VCLE_IMM, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30b90, 0xffb10200, "VCLT", ARM_VCLT_IMM, Thumb32DisassembleSIMD2RegsMisc},
  {0xffb30b90, 0xffb10300, "VABS", ARM_VABS, Thumb32DisassembleSIMD2RegsMisc},//DUP-T1: integer
  {0xffb30b90, 0xffb10380, "VNEG", ARM_VNEG, Thumb32DisassembleSIMD2RegsMisc},//DUP-T1: integer

  {0xffb00f90, 0xffb00c00, "VDUP", ARM_VDUP_SCALAR, Thumb32DisassembleSIMD},//DUP: scalar
  {0xffb00c50, 0xffb00800, "VTBL", ARM_VTBL, Thumb32DisassembleSIMD},
  {0xffb00c50, 0xffb00840, "VTBX", ARM_VTBX, Thumb32DisassembleSIMD},
  {0xffb00010, 0xefb00000, "VEXT", ARM_VEXT, Thumb32DisassembleSIMD},

  /* ASIMD data-processing instructions - 3 regs, different length
   * Ref: ARM-DDI-0406C.b[A7-264]
   */
  {0xff800f50, 0xef800400, "VADDHN", ARM_VADDHN, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xff800400, "VRADDHN", ARM_VRADDHN, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xef800600, "VSUBHN", ARM_VSUBHN, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xff800600, "VRSUBHN", ARM_VRSUBHN, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xff800f50, 0xef800900, "VQDMLAL", ARM_VQDMLAL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xff800f50, 0xef800b00, "VQDMLSL", ARM_VQDMLSL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xff800f50, 0xef800d00, "VQDMULL", ARM_VQDMULL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP-T1: integer
  {0xef800f50, 0xef800000, "VADDL", ARM_VADDL, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xef800f50, 0xef800100, "VADDW", ARM_VADDW, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xef800f50, 0xef800200, "VSUBL", ARM_VSUBL, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xef800f50, 0xef800300, "VSUBW", ARM_VSUBW, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xef800f50, 0xef800500, "VABAL", ARM_VABAL, Thumb32DisassembleSIMD3RegsDifferentLength},//T2
  {0xef800f50, 0xef800700, "VABDL", ARM_VABDL, Thumb32DisassembleSIMD3RegsDifferentLength},
  {0xef800f50, 0xef800800, "VMLAL", ARM_VMLAL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP: integer
  {0xef800f50, 0xef800a00, "VMLSL", ARM_VMLSL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP: integer
  {0xef800f50, 0xef800c00, "VMULL", ARM_VMULL, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP-T2: integer
  {0xef800f50, 0xef800e00, "VMULL", ARM_VMULL_POLY, Thumb32DisassembleSIMD3RegsDifferentLength},//DUP-T2: polynomial

  /* ASIMD data-processing instructions - 2 regs + scalar
   * Ref: ARM-DDI-0406C.b[A7-265]
   */
  {0xff800f50, 0xef800340, "VQDMLAL", ARM_VQDMLAL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xff800f50, 0xef800740, "VQDMLSL", ARM_VQDMLSL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xff800f50, 0xef800b40, "VQDMULL", ARM_VQDMULL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800f50, 0xef800240, "VMLAL", ARM_VMLAL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800f50, 0xef800640, "VMLSL", ARM_VMLSL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800f50, 0xef800a40, "VMULL", ARM_VMULL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800f50, 0xef800c40, "VQDMULH", ARM_VQDMULH_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800f50, 0xef800d40, "VQRDMULH", ARM_VQRDMULH_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T2: scalar
  {0xef800e50, 0xef800040, "VMLA", ARM_VMLA_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T1: scalar
  {0xef800e50, 0xef800440, "VMLS", ARM_VMLS_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T1: scalar
  {0xef800e50, 0xef800840, "VMUL", ARM_VMUL_SCALAR, Thumb32DisassembleSIMD2RegsScalar},//DUP-T1: scalar

  /* ASIMD element or structure load/store instructions
   * Ref: ARM-DDI-0406C.b[A7-275]
   */
  {0xffb00700, 0xf9000200, "VST1", ARM_VST1_MULTI2OR4, Thumb32DisassembleSIMDLoadStore},//DUP - multiple single elements - 4 registers
  {0xffb00e00, 0xf9000600, "VST1", ARM_VST1_MULTI1OR3, Thumb32DisassembleSIMDLoadStore},//DUP - 1 of 3 registers
  {0xffb00f00, 0xf9000300, "VST2", ARM_VST2_MULTI2, Thumb32DisassembleSIMDLoadStore},//DUP - multiple 2-element structures - 2 registers
  {0xffb00e00, 0xf9000800, "VST2", ARM_VST2_MULTI1, Thumb32DisassembleSIMDLoadStore},//DUP - 1 register
  {0xffb00e00, 0xf9000400, "VST3", ARM_VST3_MULTI, Thumb32DisassembleSIMDLoadStore},//DUP-multiple 3-element structures
  {0xffb00e00, 0xf9000000, "VST4", ARM_VST4_MULTI, Thumb32DisassembleSIMDLoadStore},//DUP-multiple 4-element structures
  {0xffb00300, 0xf9800000, "VST1", ARM_VST1_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single element from one lane - size=8|16
  {0xffb00300, 0xf9800100, "VST2", ARM_VST2_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 2-element structure from one lane - size=8|16
  {0xffb00300, 0xf9800200, "VST3", ARM_VST3_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 3-element sturcture from one lane - size=8|16
  {0xffb00300, 0xf9800300, "VST4", ARM_VST4_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 4-element structure from one lane - size=8|16
  {0xffb00700, 0xf9200200, "VLD1", ARM_VLD1_MULTI2OR4, Thumb32DisassembleSIMDLoadStore},//DUP-multiple single elements - 4 registers
  {0xffb00e00, 0xf9200600, "VLD1", ARM_VLD1_MULTI1OR3, Thumb32DisassembleSIMDLoadStore},//DUP - 1 or 3 registers
  {0xffb00f00, 0xf9200300, "VLD2", ARM_VLD2_MULTI2, Thumb32DisassembleSIMDLoadStore},//DUP-multiple 2-element structures - 2 registers
  {0xffb00e00, 0xf9200800, "VLD2", ARM_VLD2_MULTI1, Thumb32DisassembleSIMDLoadStore},//DUP - 1 register
  {0xffb00e00, 0xf9200400, "VLD3", ARM_VLD3_MULTI, Thumb32DisassembleSIMDLoadStore},//DUP-multiple 3-element structures
  {0xffb00e00, 0xf9200000, "VLD4", ARM_VLD4_MULTI, Thumb32DisassembleSIMDLoadStore},//DUP-multiple 4-element structures
  {0xffb00f00, 0xf9a00c00, "VLD1", ARM_VLD1_ALL, Thumb32DisassembleSIMDLoadStore},//DUP-single element to all lanes
  {0xffb00300, 0xf9a00000, "VLD1", ARM_VLD1_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single element to one lane
  {0xffb00f00, 0xf9a00d00, "VLD2", ARM_VLD2_ALL, Thumb32DisassembleSIMDLoadStore},//DUP-single 2-element structure to all lanes
  {0xffb00300, 0xf9a00100, "VLD2", ARM_VLD2_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 2-element structure to one lane - size=8|16
  {0xffb00f00, 0xf9a00e00, "VLD3", ARM_VLD3_ALL, Thumb32DisassembleSIMDLoadStore},//DUP-single 3-element structure to all lanes
  {0xffb00300, 0xf9a00200, "VLD3", ARM_VLD3_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 3-element structure to one lane - size=8|16
  {0xffb00f00, 0xf9a00f00, "VLD4", ARM_VLD4_ALL, Thumb32DisassembleSIMDLoadStore},//DUP-single 4-element structure to all lanes
  {0xffb00300, 0xf9a00300, "VLD4", ARM_VLD4_ONE, Thumb32DisassembleSIMDLoadStore},//DUP-single 4-element structure to one lane - size=8|16

/* regular Thumb instructions */
	{0xfffff0c0, 0xfa0ff080, "SXTH", ARM_SXTH, Thumb32DisassembleDataproc},
	{0xfffff0c0, 0xfa1ff080, "UXTH", ARM_UXTH, Thumb32DisassembleDataproc},
	{0xfffff0c0, 0xfa2ff080, "SXTB16", ARM_SXTB16, Thumb32DisassembleDataproc},
	{0xfffff0c0, 0xfa3ff080, "UXTB16", ARM_UXTB16, Thumb32DisassembleDataproc},
	{0xfffff0c0, 0xfa4ff080, "SXTB", ARM_SXTB, Thumb32DisassembleDataproc},
	{0xfffff0c0, 0xfa5ff080, "UXTB", ARM_UXTB, Thumb32DisassembleDataproc},

	{0xffeff0f0, 0xea4f0030, "RRX", ARM_MOV, Thumb32DisassembleDataprocRegister},
  {0xffeff0f0, 0xea4f0000, "MOV", ARM_MOV, Thumb32DisassembleDataprocRegister},//dup: register

	{0xffef8030, 0xea4f0030, "ROR", ARM_MOV, Thumb32DisassembleImmediate},//dup: immediate
	{0xffef8030, 0xea4f0000, "LSL", ARM_MOV, Thumb32DisassembleImmediate},//dup: immediate
	{0xffef8030, 0xea4f0010, "LSR", ARM_MOV, Thumb32DisassembleImmediate},//dup: immediate
	{0xffef8030, 0xea4f0020, "ASR", ARM_MOV, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbf08f00, 0xf0100f00, "TST", ARM_TST, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbf08f00, 0xf0900f00, "TEQ", ARM_TEQ, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbf08f00, 0xf1100f00, "CMN", ARM_CMN, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbf08f00, 0xf1b00f00, "CMP", ARM_CMP, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbf08000, 0xf2c00000, "MOVT", ARM_MOVT, Thumb32DisassembleImmediate},
	{0xfbef8000, 0xf04f0000, "MOV", ARM_MOV, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbef8000, 0xf06f0000, "MVN", ARM_MVN, Thumb32DisassembleImmediate},//dup: immediate
	{0xfb5f8000, 0xf20f0000, "ADR", ARM_UNDEF, Thumb32DisassembleImmediate},
	{0xfbe08000, 0xf0000000, "AND", ARM_AND, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf1000000, "ADD", ARM_ADD, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf2000000, "ADDW", ARM_ADD, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf0200000, "BIC", ARM_BIC, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf0400000, "ORR", ARM_ORR, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf0600000, "ORN", ARM_T2ORN, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf0800000, "EOR", ARM_EOR, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf1400000, "ADC", ARM_ADC, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf1600000, "SBC", ARM_SBC, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf1c00000, "RSB", ARM_RSB, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf2400000, "MOVW", ARM_MOVW, Thumb32DisassembleImmediate},
	{0xfbe08000, 0xf1a00000, "SUB", ARM_SUB, Thumb32DisassembleImmediate},//dup: immediate
	{0xfbe08000, 0xf2a00000, "SUBW", ARM_SUB, Thumb32DisassembleImmediate},//dup: immediate

	{0xffff8020, 0xf36f0000, "BFC", ARM_BFC, Thumb32DisassembleBits},
	{0xfff08020, 0xf3600000, "BFI", ARM_BFI, Thumb32DisassembleBits},
	{0xfff08020, 0xf3400000, "SBFX", ARM_SBFX, Thumb32DisassembleBits},
	{0xfff08020, 0xf3c00000, "UBFX", ARM_UBFX, Thumb32DisassembleBits},

	{0xfff0f000, 0xf7e08000, "HVC", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfff0f000, 0xf7f08000, "SMC", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfff0f000, 0xf7f0a000, "UDF", ARM_UNDEF, Thumb32DisassembleNotImplemented},

	{0xffffff00, 0xf3de8f00, "SUBS_PC_LR", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfffff0ff, 0xf3ef8000, "MRS", ARM_MRS, Thumb32DisassembleControl},//dup: regular
	{0xfff0f3ff, 0xf3808000, "MSR", ARM_MSR, Thumb32DisassembleControl},//dup: regular
	{0xfff0d0ff, 0xf3d08000, "ERET", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfff0d000, 0xf3c08000, "BXJ", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfff0d000, 0xf3a08000, "CPS", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xffe0f0ef, 0xf3808020, "MSR", ARM_UNDEF, Thumb32DisassembleNotImplemented},//dup: banked
	{0xffe0f0ef, 0xf3e08020, "MRS", ARM_UNDEF, Thumb32DisassembleNotImplemented},//dup: banked
	{0xf800d000, 0xf000c000, "BLX", ARM_BLX, Thumb32DisassembleBranch},
	{0xf800d000, 0xf000d000, "BL", ARM_BL, Thumb32DisassembleBranch},
	{0xf800c000, 0xf0008000, "B", ARM_B, Thumb32DisassembleBranch},

	{0xffff0fff, 0xf84d0d04, "PUSH", ARM_STR, Thumb32DisassembleLoadStoreMultiple},//dup: 1 register
	{0xffff0fff, 0xf85d0b04, "POP", ARM_LDR, Thumb32DisassembleLoadStoreMultiple},//dup: 1 register
	{0xffffa000, 0xe92d0000, "PUSH", ARM_STM, Thumb32DisassembleLoadStoreMultiple},//dup: >1 register
	{0xffff2000, 0xe8bd0000, "POP", ARM_LDM, Thumb32DisassembleLoadStoreMultiple},//dup: >1 register
	{0xffd0a000, 0xe8800000, "STM", ARM_STM, Thumb32DisassembleLoadStoreMultiple},
	{0xffd0a000, 0xe9000000, "STMDB", ARM_STM, Thumb32DisassembleLoadStoreMultiple},
	{0xffd02000, 0xe8900000, "LDM", ARM_LDM, Thumb32DisassembleLoadStoreMultiple},
	{0xffd02000, 0xe9100000, "LDMDB", ARM_LDM, Thumb32DisassembleLoadStoreMultiple},

	{0xfe5fffe0, 0xe8000000, "SRS", ARM_UNDEF, Thumb32DisassembleNotImplemented},
	{0xfe50ffff, 0xe8000000, "RFE", ARM_UNDEF, Thumb32DisassembleNotImplemented},

	{0xfff0fff0, 0xe8d0f000, "TBB", ARM_T2TBB, Thumb32DisassembleBranch},
	{0xfff0fff0, 0xe8d0f010, "TBH", ARM_T2TBH, Thumb32DisassembleBranch},

	{0xfff00fff, 0xe8d00f4f, "LDREXB", ARM_LDREXB, Thumb32DisassembleLoadStoreExclusive},
	{0xfff00fff, 0xe8d00f5f, "LDREXH", ARM_LDREXH, Thumb32DisassembleLoadStoreExclusive},
	{0xfff00ff0, 0xe8c00f40, "STREXB", ARM_STREXB, Thumb32DisassembleLoadStoreExclusive},
	{0xfff00ff0, 0xe8c00f50, "STREXH", ARM_STREXH, Thumb32DisassembleLoadStoreExclusive},
	{0xfff00f00, 0xe8500f00, "LDREX", ARM_LDREX, Thumb32DisassembleLoadStoreExclusive},
	{0xfff000f0, 0xe8c00070, "STREXD", ARM_STREXD, Thumb32DisassembleLoadStoreExclusive},
	{0xfff000ff, 0xe8d0007f, "LDREXD", ARM_LDREXD, Thumb32DisassembleLoadStoreExclusive},
	{0xfff00000, 0xe8400000, "STREX", ARM_STREX, Thumb32DisassembleLoadStoreExclusive},

	{0xff70f000, 0xf810f000, "PLD", ARM_PLD, Thumb32DisassembleLoadStore},
	{0xff70f000, 0xf830f000, "PLDW", ARM_PLDW, Thumb32DisassembleLoadStore},
	{0xff70f000, 0xf910f000, "PLI", ARM_PLI, Thumb32DisassembleLoadStore},

	{0xff700000, 0xf8000000, "STRB", ARM_STRB, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf8100000, "LDRB", ARM_LDRB, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf8200000, "STRH", ARM_STRH, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf8300000, "LDRH", ARM_LDRH, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf8400000, "STR", ARM_STR, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf8500000, "LDR", ARM_LDR, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf9100000, "LDRSB", ARM_LDRSB, Thumb32DisassembleLoadStore},
	{0xff700000, 0xf9300000, "LDRSH", ARM_LDRSH, Thumb32DisassembleLoadStore},
	{0xfe500000, 0xe8400000, "STRD", ARM_STRD, Thumb32DisassembleLoadStore},
	{0xfe500000, 0xe8500000, "LDRD", ARM_LDRD, Thumb32DisassembleLoadStore},

	{0xfff0f0f0, 0xfa90f000, "SADD16", ARM_SADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f000, "SASX", ARM_SADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f000, "SSAX", ARM_SSUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f000, "SSUB16", ARM_SSUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f000, "SADD8", ARM_SADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f020, "SHADD16", ARM_SHADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f020, "SHASX", ARM_SHADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f020, "SHSAX", ARM_SHSUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f020, "SHSUB16", ARM_SHSUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f020, "SHADD8", ARM_SHADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f020, "SHSUB8", ARM_SHSUB8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f040, "UADD16", ARM_UADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f040, "UASX", ARM_UADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f040, "USAX", ARM_USUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f040, "USUB16", ARM_USUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f040, "UADD8", ARM_UADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfb70f000, "USAD8", ARM_USAD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f040, "USUB8", ARM_USUB8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f070, "UHADD16", ARM_UHADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f070, "UHASX", ARM_UHADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f070, "UHSAX", ARM_UHSUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f070, "UHSUB16", ARM_UHSUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f070, "UHADD8", ARM_UHADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f070, "UHSUB8", ARM_UHSUB8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f080, "REV", ARM_REV, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f090, "REV16", ARM_REV16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f0b0, "REVSH", ARM_REVSH, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f080, "SEL", ARM_SEL, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfab0f080, "CLZ", ARM_CLZ, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f0a0, "RBIT", ARM_RBIT, Thumb32DisassembleBits},
	{0xfff0f0c0, 0xfa40f080, "SXTAB", ARM_SXTAB, Thumb32DisassembleDataproc},
	{0xfff0f0c0, 0xfa20f080, "SXTAB16", ARM_SXTAB16, Thumb32DisassembleDataproc},
	{0xfff0f0c0, 0xfa00f080, "SXTAH", ARM_SXTAH, Thumb32DisassembleDataproc},
	{0xfff0f0c0, 0xfa50f080, "UXTAB", ARM_UXTAB, Thumb32DisassembleDataproc},
	{0xfff0f0c0, 0xfa30f080, "UXTAB16", ARM_UXTAB16, Thumb32DisassembleDataproc},
	{0xfff0f0c0, 0xfa10f080, "UXTAH", ARM_UXTAH, Thumb32DisassembleDataproc},
	{0xfff000f0, 0xfb700000, "USADA8", ARM_USADA8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f000, "SSUB8", ARM_SSUB8, Thumb32DisassembleDataproc},

	{0xfff0f0f0, 0xfa90f010, "QADD16", ARM_QADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f010, "QASX", ARM_QADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f010, "QSAX", ARM_QSUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f010, "QSUB16", ARM_QSUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f010, "QADD8", ARM_QADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f010, "QSUB8", ARM_QSUB8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa90f050, "UQADD16", ARM_UQADD16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfaa0f050, "UQASX", ARM_UQADDSUBX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfae0f050, "UQSAX", ARM_UQSUBADDX, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfad0f050, "UQSUB16", ARM_UQSUB16, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f050, "UQADD8", ARM_UQADD8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfac0f050, "UQSUB8", ARM_UQSUB8, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f080, "QADD", ARM_QADD, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f0a0, "QSUB", ARM_QSUB, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f090, "QDADD", ARM_QDADD, Thumb32DisassembleDataproc},
	{0xfff0f0f0, 0xfa80f0b0, "QDSUB", ARM_QDSUB, Thumb32DisassembleDataproc},

	{0xfff0f0f0, 0xf3200000, "SSAT16", ARM_SSAT16, Thumb32DisassembleDataprocSaturating},
	{0xfff0f0f0, 0xf3a00000, "USAT16", ARM_USAT16, Thumb32DisassembleDataprocSaturating},
	{0xffd08020, 0xf3000000, "SSAT", ARM_SSAT, Thumb32DisassembleDataprocSaturating},
	{0xffd08020, 0xf3800000, "USAT", ARM_USAT, Thumb32DisassembleDataprocSaturating},

	{0xfff08f00, 0xea100f00, "TST", ARM_TST, Thumb32DisassembleDataprocRegister},//dup: register
	{0xfff08f00, 0xea900f00, "TEQ", ARM_TEQ, Thumb32DisassembleDataprocRegister},//dup: register
	{0xfff08f00, 0xeb100f00, "CMN", ARM_CMN, Thumb32DisassembleDataprocRegister},//dup: register
	{0xfff08f00, 0xebb00f00, "CMP", ARM_CMP, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe0f0f0, 0xfa00f000, "LSL", ARM_MOV, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe0f0f0, 0xfa20f000, "LSR", ARM_MOV, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe0f0f0, 0xfa40f000, "ASR", ARM_MOV, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe0f0f0, 0xfa60f000, "ROR", ARM_MOV, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffef8000, 0xea6f0000, "MVN", ARM_MVN, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xea000000, "AND", ARM_AND, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xea200000, "BIC", ARM_BIC, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xea400000, "ORR", ARM_ORR, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xea600000, "ORN", ARM_T2ORN, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xea800000, "EOR", ARM_EOR, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08010, 0xeac00000, "PKHBT", ARM_PKHBT, Thumb32DisassembleDataprocRegister},
	{0xffe08010, 0xeac00010, "PKHTB", ARM_PKHTB, Thumb32DisassembleDataprocRegister},
	{0xffe08000, 0xeb000000, "ADD", ARM_ADD, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xeb400000, "ADC", ARM_ADC, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xeb600000, "SBC", ARM_SBC, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xeba00000, "SUB", ARM_SUB, Thumb32DisassembleDataprocRegister},//dup: register
	{0xffe08000, 0xebc00000, "RSB", ARM_RSB, Thumb32DisassembleDataprocRegister},//dup: register

	{0xfff0f0f0, 0xfb00f000, "MUL", ARM_MUL, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb20f000, "SMUAD", ARM_SMUAD, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb20f010, "SMUADX", ARM_SMUADX, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb30f000, "SMULWB", ARM_SMULWB, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb30f010, "SMULWT", ARM_SMULWT, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb40f000, "SMUSD", ARM_SMUSD, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb40f010, "SMUSDX", ARM_SMUSDX, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb50f000, "SMMUL", ARM_SMMUL, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb50f010, "SMMULR", ARM_SMMULR, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb10f000, "SMULBB", ARM_SMULBB, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb10f010, "SMULBT", ARM_SMULBT, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb10f020, "SMULTB", ARM_SMULTB, Thumb32DisassembleMultiply},
	{0xfff0f0f0, 0xfb10f030, "SMULTT", ARM_SMULTT, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb000010, "MLS", ARM_MLS, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb000000, "MLA", ARM_MLA, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb200000, "SMLAD", ARM_SMLAD, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb200010, "SMLADX", ARM_SMLADX, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb300000, "SMLAWB", ARM_SMLAWB, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb300010, "SMLAWT", ARM_SMLAWT, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb400000, "SMLSD", ARM_SMLSD, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb400010, "SMLSDX", ARM_SMLSDX, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb500000, "SMMLA", ARM_SMMLA, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb500010, "SMMLAR", ARM_SMMLAR, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb600000, "SMMLS", ARM_SMMLS, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb600010, "SMMLSR", ARM_SMMLSR, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb100000, "SMLABB", ARM_SMLABB, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb100010, "SMLABT", ARM_SMLABT, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb100020, "SMLATB", ARM_SMLATB, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb100030, "SMLATT", ARM_SMLATT, Thumb32DisassembleMultiply},

	{0xfff0f0f0, 0xfb90f0f0, "SDIV", ARM_SDIV, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbb000f0, "UDIV", ARM_UDIV, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfb800000, "SMULL", ARM_SMULL, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfba00000, "UMULL", ARM_UMULL, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc00000, "SMLAL", ARM_SMLAL, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc00080, "SMLALBB", ARM_SMLALBB, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc00090, "SMLALBT", ARM_SMLALBT, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc000a0, "SMLALTB", ARM_SMLALTB, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc000b0, "SMLALTT", ARM_SMLALTT, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc000c0, "SMLALD", ARM_SMLALD, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbc000d0, "SMLALDX", ARM_SMLALDX, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbd000c0, "SMLSLD", ARM_SMLSLD, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbd000d0, "SMLSLDX", ARM_SMLSLDX, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbe00000, "UMLAL", ARM_UMLAL, Thumb32DisassembleMultiply},
	{0xfff000f0, 0xfbe00060, "UMAAL", ARM_UMAAL, Thumb32DisassembleMultiply},

  /* 8, 16 and 32-bit transfer between ARM core and extension registers
   * Ref: ARM-DDI-0406C.b[A7-278]
   */
  {0xfff00f7f, 0xee000a10, "VMOV", ARM_VMOV_C2S, Thumb32DisassembleSIMDTransfer},//DUP: core -> single
  {0xffff0fff, 0xeee10a10, "VMSR", ARM_VMSR, Thumb32DisassembleSIMDTransfer},
  {0xff900f1f, 0xee000b10, "VMOV", ARM_VMOV_C2SCALAR, Thumb32DisassembleSIMDTransfer},//DUP: core -> scalar
  {0xff900f5f, 0xee800b10, "VDUP", ARM_VDUP, Thumb32DisassembleSIMD},//DUP: core
  {0xfff00f7f, 0xee100a10, "VMOV", ARM_VMOV_S2C, Thumb32DisassembleSIMDTransfer},//DUP: single -> core
  {0xffff0fff, 0xeef10a10, "VMRS", ARM_VMRS, Thumb32DisassembleSIMDTransfer},
  {0xff100f1f, 0xee100b10, "VMOV", ARM_VMOV_SCALAR2C, Thumb32DisassembleSIMDTransfer},//DUP: scalar -> core

  /* Floating-point data-processing instructions - 3 regs
   * Ref: ARM-DDI-0406C.b[A7-272]
   */
  {0xffb00e50, 0xee000a00, "VMLA", ARM_VMLA_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xee000a40, "VMLS", ARM_VMLS_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xee100a00, "VNMLA", ARM_VNMLA, Thumb32DisassembleFPDataProc},
  {0xffb00e50, 0xee100a40, "VNMLS", ARM_VNMLS, Thumb32DisassembleFPDataProc},
  {0xffb00e50, 0xee200a40, "VNMUL", ARM_VNMUL, Thumb32DisassembleFPDataProc},
  {0xffb00e50, 0xee200a00, "VMUL", ARM_VMUL_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xee300a00, "VADD", ARM_VADD_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xee300a40, "VSUB", ARM_VSUB_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xee800a00, "VDIV", ARM_VDIV, Thumb32DisassembleFPDataProc},
  {0xffb00e50, 0xee900a00, "VFNMA", ARM_VFNMA, Thumb32DisassembleFPDataProc},//T2
  {0xffb00e50, 0xee900a40, "VFNMS", ARM_VFNMS, Thumb32DisassembleFPDataProc},//T2
  {0xffb00e50, 0xeea00a00, "VFMA", ARM_VFMA_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffb00e50, 0xeea00a40, "VFMS", ARM_VFMS_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating

  /* Floating-point other data-processing instructions
   * Ref: ARM-DDI-0406C.b[A7-272,273]
   */
  {0xffb00ef0, 0xeeb00a00, "VMOV", ARM_VMOV_FIMM, Thumb32DisassembleFPDataProc},//DUP-T2: immediate, floating
  {0xffbf0ed0, 0xeeb00a40, "VMOV", ARM_VMOV_F, Thumb32DisassembleFPDataProc},//DUP-T2: register, floating
  {0xffbf0ed0, 0xeeb00ac0, "VABS", ARM_VABS_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffbf0ed0, 0xeeb10a40, "VNEG", ARM_VNEG_F64, Thumb32DisassembleFPDataProc},//DUP-T2: floating
  {0xffbf0ed0, 0xeeb10ac0, "VSQRT", ARM_VSQRT, Thumb32DisassembleFPDataProc},
  {0xffbe0ed0, 0xeeb40a40, "VCMP", ARM_VCMP, Thumb32DisassembleFPDataProc},//T1 of T2
  {0xffbe0ed0, 0xeeb40ac0, "VCMPE", ARM_VCMPE, Thumb32DisassembleFPDataProc},//T1 of T2
  {0xffbe0fd0, 0xeeb20a40, "VCVTB", ARM_VCVTB, Thumb32DisassembleFPDataProc},
  {0xffbe0fd0, 0xeeb20ac0, "VCVTT", ARM_VCVTT, Thumb32DisassembleFPDataProc},
  {0xffbf0ed0, 0xeeb70ac0, "VCVT", ARM_VCVT_DS, Thumb32DisassembleFPDataProc},   //DUP:double/single
  {0xffbf0e50, 0xeeb80a40, "VCVT", ARM_VCVT_I2F, Thumb32DisassembleFPDataProc},  //DUP:integer->floating
  {0xffbe0e50, 0xeeba0a40, "VCVT", ARM_VCVT_X2F, Thumb32DisassembleFPDataProc},   //DUP:fixed->floating
  {0xffbe0e50, 0xeebe0a40, "VCVT", ARM_VCVT_F2X, Thumb32DisassembleFPDataProc},   //DUP:floating->fixed
  {0xffbe0ed0, 0xeebc0ac0, "VCVT", ARM_VCVT_F2I, Thumb32DisassembleFPDataProc},   //DUP:floating->integer
  {0xffbe0ed0, 0xeebc0a40, "VCVTR", ARM_VCVTR_F2I, Thumb32DisassembleFPDataProc},  //DUP:floating->integer

  /* 64-bit transfers between ARM core and extension registers
   * Ref: ARM-DDI-0406C.b[A7-279]
   */
  {0xffe00fd0, 0xec400a10, "VMOV", ARM_VMOV64_C2S, Thumb32DisassembleFP2R},//DUP: core -> single
  {0xffe00fd0, 0xec400b10, "VMOV", ARM_VMOV64_C2D, Thumb32DisassembleFP2R},//DUP: core -> double

  /* Extension register load/store instructions
   * Ref: ARM-DDI-0406C.b[A7-274]
   */
  {0xefbf0e00, 0xed2d0a00, "VPUSH", ARM_VPUSH, Thumb32DisassembleFPLoadStore},//T1 of T2
  {0xefbf0e00, 0xecbd0a00, "VPOP", ARM_VPOP, Thumb32DisassembleFPLoadStore},//T1 of T2
  {0xef300e00, 0xed000a00, "VSTR", ARM_VSTR, Thumb32DisassembleFPLoadStore},//T1 of T2
  {0xef300e00, 0xed100a00, "VLDR", ARM_VLDR, Thumb32DisassembleFPLoadStore},//T1 of T2
  {0xee100f01, 0xec000b01, "FSTMX", ARM_FSTMX, Thumb32DisassembleVLoadStore},
  {0xee100e00, 0xec000a00, "VSTM", ARM_VSTM, Thumb32DisassembleFPLoadStore},//DUP-T1 of T2
  {0xee100f01, 0xec100b01, "FLDMX", ARM_FLDMX, Thumb32DisassembleVLoadStore},
  {0xee100e00, 0xec100a00, "VLDM", ARM_VLDM, Thumb32DisassembleFPLoadStore},//DUP-T1 of T2, increment after, no writeback

	{0xfff00000, 0xec500000, "MRRC", ARM_MRRC, Thumb32DisassembleCoproc},
	{0xfff00000, 0xfc500000, "MRRC2", ARM_MRRC2, Thumb32DisassembleCoproc},
	{0xfff00000, 0xec400000, "MCRR", ARM_MCRR, Thumb32DisassembleCoproc},
	{0xfff00000, 0xfc400000, "MCRR2", ARM_MCRR2, Thumb32DisassembleCoproc},
	{0xff100010, 0xee100010, "MRC", ARM_MRC, Thumb32DisassembleCoproc},
	{0xff100010, 0xfe100010, "MRC2", ARM_MRC2, Thumb32DisassembleCoproc},
	{0xff100010, 0xee000010, "MCR", ARM_MCR, Thumb32DisassembleCoproc},
	{0xff100010, 0xfe000010, "MCR2", ARM_MCR2, Thumb32DisassembleCoproc},
	{0xff000010, 0xee000000, "CDP", ARM_CDP, Thumb32DisassembleCoproc},
	{0xff000010, 0xfe000000, "CDP2", ARM_CDP2, Thumb32DisassembleCoproc},
	{0xfe100000, 0xec100000, "LDC", ARM_LDC, Thumb32DisassembleCoproc},
	{0xfe100000, 0xfc100000, "LDC2", ARM_LDC2, Thumb32DisassembleCoproc},
	{0xfe100000, 0xec000000, "STC", ARM_STC, Thumb32DisassembleCoproc},
	{0xfe100000, 0xfc000000, "STC2", ARM_STC2, Thumb32DisassembleCoproc},

	/* 16-bit thumb encoding */
	{0xffffffe8, 0x0000b660, "CPS", ARM_CPS, ThumbDisassembleNotImplemented},
	{0xfffffff7, 0x0000b650, "SETEND", ARM_SETEND, ThumbDisassembleNotImplemented},

	/* Thumb-2 (v6T2 or v7) */
	{0xffffffff, 0x0000bf00, "NOP", ARM_T2NOP, ThumbDisassembleV6V7ITHints},
	{0xffffffff, 0x0000bf10, "YIELD", ARM_T2YIELD, ThumbDisassembleV6V7ITHints},
	{0xffffffff, 0x0000bf20, "WFE", ARM_T2WFE, ThumbDisassembleV6V7ITHints},
	{0xffffffff, 0x0000bf30, "WFI", ARM_T2WFI, ThumbDisassembleV6V7ITHints},
	{0xffffffff, 0x0000bf40, "SEV", ARM_T2SEV, ThumbDisassembleV6V7ITHints},
  {0xffffff00, 0x0000bf00, "IT", ARM_T2IT, ThumbDisassembleV6V7ITHints},

	/* Software Interrupt */
	{0xffffff00, 0x0000df00, "SVC", ARM_SWI, ThumbDisassembleSWI},
  {0xffffff00, 0x0000be00, "BKPT", ARM_BKPT, ThumbDisassembleBKPT},

	/* ARMv6+ */
	{0xffffffc0, 0x0000b2c0, "UXTB", ARM_UXTB, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000b280, "UXTH", ARM_UXTH, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000b240, "SXTB", ARM_SXTB, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000b200, "SXTH", ARM_SXTH, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000ba00, "REV", ARM_REV, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000ba40, "REV16", ARM_REV16, ThumbDisassembleV6Extract},
	{0xffffffc0, 0x0000bac0, "REVSH", ARM_REVSH, ThumbDisassembleV6Extract},

	/* Branches */
	{0xffffff00, 0x0000de00, "UDF", ARM_UNDEF, ThumbDisassembleUndefined},
	{0xfffff800, 0x0000e000, "B", ARM_B, ThumbDisassembleBranch},
	{0xfffff000, 0x0000d000, "B", ARM_B, ThumbDisassembleBranch},
	{0xffffff87, 0x00004700, "BX", ARM_BX, ThumbDisassembleBranch},
	{0xffffff87, 0x00004780, "BLX", ARM_BLX, ThumbDisassembleBranchLink},
	{0xfffffd00, 0x0000b100, "CBZ", ARM_T2CBZ, ThumbDisassembleBranch},
	{0xfffffd00, 0x0000b900, "CBNZ", ARM_T2CBNZ, ThumbDisassembleBranch},

	/* Add/Subtract */
	{0xfffffe00, 0x00001a00, "SUB", ARM_SUB, ThumbDisassemble3Bit},
	{0xfffffe00, 0x00001800, "ADD", ARM_ADD, ThumbDisassemble3Bit},
	{0xfffffe00, 0x00001e00, "SUB", ARM_SUB, ThumbDisassemble3Bit},
	{0xfffffe00, 0x00001c00, "ADD", ARM_ADD, ThumbDisassemble3Bit},

	/* Move, compare, add, subtract immediate */
	{0xfffff800, 0x00002000, "MOV", ARM_MOV, ThumbDisassembleImm},
	{0xfffff800, 0x00002800, "CMP", ARM_CMP, ThumbDisassembleImm},
	{0xfffff800, 0x00003000, "ADD", ARM_ADD, ThumbDisassembleImm},

	{0xffffffc0, 0x00000000, "MOV", ARM_MOV, ThumbDisassembleHiReg},
	/* ALU operations */
	{0xffffffc0, 0x00004000, "AND", ARM_AND, ThumbDisassembleALU},
	{0xffffffc0, 0x00004040, "EOR", ARM_EOR, ThumbDisassembleALU},
	{0xffffffc0, 0x00004080, "LSL", ARM_MOV, ThumbDisassembleALU},
	{0xffffffc0, 0x000040c0, "LSR", ARM_MOV, ThumbDisassembleALU},
	{0xffffffc0, 0x00004100, "ASR", ARM_MOV, ThumbDisassembleALU},
	{0xffffffc0, 0x00004140, "ADC", ARM_ADC, ThumbDisassembleALU},
	{0xffffffc0, 0x00004180, "SBC", ARM_SBC, ThumbDisassembleALU},
	{0xffffffc0, 0x000041c0, "ROR", ARM_MOV, ThumbDisassembleALU},
	{0xffffffc0, 0x00004200, "TST", ARM_TST, ThumbDisassembleALU},
	{0xffffffc0, 0x00004240, "RSB", ARM_RSB, ThumbDisassembleALU},
	{0xffffffc0, 0x00004280, "CMP", ARM_CMP, ThumbDisassembleALU},
	{0xffffffc0, 0x000042c0, "CMN", ARM_CMN, ThumbDisassembleALU},
	{0xffffffc0, 0x00004300, "ORR", ARM_ORR, ThumbDisassembleALU},
	{0xffffffc0, 0x00004340, "MUL", ARM_MUL, ThumbDisassembleALU},
	{0xffffffc0, 0x00004380, "BIC", ARM_BIC, ThumbDisassembleALU},
	{0xffffffc0, 0x000043c0, "MVN", ARM_MVN, ThumbDisassembleALU},
	{0xfffff800, 0x0000a000, "ADR", ARM_ADD, ThumbDisassembleLoadAddress},

	/* Hi register operations */
	{0xffffff00, 0x00004400, "ADD", ARM_ADD, ThumbDisassembleHiReg},
	{0xffffff00, 0x00004500, "CMP", ARM_CMP, ThumbDisassembleHiReg},
	{0xffffff00, 0x00004600, "MOV", ARM_MOV, ThumbDisassembleHiReg},
	{0xfffff800, 0x00003800, "SUB", ARM_SUB, ThumbDisassembleImm},

	/* Move shifted register */
	{0xfffff800, 0x00000000, "LSL", ARM_MOV, ThumbDisassembleShifted},
	{0xfffff800, 0x00000800, "LSR", ARM_MOV, ThumbDisassembleShifted},
	{0xfffff800, 0x00001000, "ASR", ARM_MOV, ThumbDisassembleShifted},

	/* Load address */
	{0xfffff800, 0x0000a800, "ADD", ARM_ADD, ThumbDisassembleLoadAddress},

	/* Add offset to stack pointer */
	{0xffffff80, 0x0000b000, "ADD", ARM_ADD, ThumbDisassembleStack},
	{0xffffff80, 0x0000b080, "SUB", ARM_SUB, ThumbDisassembleStack},

	/* Load, store with immediate offset */
	{0xfffff800, 0x00006000, "STR", ARM_STR, ThumbDisassembleTransferImm},
	{0xfffff800, 0x00006800, "LDR", ARM_LDR, ThumbDisassembleTransferImm},
	{0xfffff800, 0x00007000, "STRB", ARM_STRB, ThumbDisassembleTransferImm},
	{0xfffff800, 0x00007800, "LDRB", ARM_LDRB, ThumbDisassembleTransferImm},

	/* Load, store halfword */
	{0xfffff800, 0x00008000, "STRH", ARM_STRH, ThumbDisassembleTransferHalf},
	{0xfffff800, 0x00008800, "LDRH", ARM_LDRH, ThumbDisassembleTransferHalf},

	/* Load, store sign-extended byte/halfword */
	{0xfffffe00, 0x00005200, "STRH", ARM_STRH, ThumbDisassembleTransferSign},
	{0xfffffe00, 0x00005a00, "LDRH", ARM_LDRH, ThumbDisassembleTransferSign},
	{0xfffffe00, 0x00005600, "LDRSB", ARM_LDRSB, ThumbDisassembleTransferSign},
	{0xfffffe00, 0x00005e00, "LDRSH", ARM_LDRSH, ThumbDisassembleTransferSign},

	/* Load, store with register offset */
	{0xfffffe00, 0x00005000, "STR", ARM_STR, ThumbDisassembleTransferRegOff},
	{0xfffffe00, 0x00005800, "LDR", ARM_LDR, ThumbDisassembleTransferRegOff},
	{0xfffffe00, 0x00005400, "STRB", ARM_STRB, ThumbDisassembleTransferRegOff},
	{0xfffffe00, 0x00005c00, "LDRB", ARM_LDRB, ThumbDisassembleTransferRegOff},

	/* PC-relative load */
	{0xfffff800, 0x00004800, "LDR", ARM_LDR, ThumbDisassembleTransferPC},

	/* SP-relative load/store */
	{0xfffff800, 0x00009000, "STR", ARM_STR, ThumbDisassembleTransferSP},
	{0xfffff800, 0x00009800, "LDR", ARM_LDR, ThumbDisassembleTransferSP},

	/* Multiple load/store */
	{0xfffff800, 0x0000c000, "STM", ARM_STM, ThumbDisassembleMultipleTransfer},
	{0xfffff800, 0x0000c800, "LDM", ARM_LDM, ThumbDisassembleMultipleTransfer},

	/* Push, pop registers */
	{0xfffffe00, 0x0000b400, "PUSH", ARM_STM, ThumbDisassemblePP},
	{0xfffffe00, 0x0000bc00, "POP", ARM_LDM, ThumbDisassemblePP},

	/* opvang voor niet-herkende instructies */
  {0x00000000, 0x00000000, "UNDEF(Thumb16)", ARM_UNDEF, ThumbDisassembleUnsupported},
  {0x00000000, 0x00000000, "UNDEF(Thumb32)", ARM_UNDEF, ThumbDisassembleUnsupported},

  {0x00000000, 0x00000000, "DATA", ARM_DATA, ThumbDisassembleUnsupported}
};
/* }}} */

const t_uint32 arm2thumb32_opcode_table[] = {
	/* Thumb2 instructions */
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_ORN,
	TH32_UNDEF,
	TH32_UNDEF,
	TH32_TBB,
	TH32_TBH,

	TH32_CLREX,
	TH32_UNDEF,

	TH32_DMB,
	TH32_DSB,
	TH32_ISB,

/* SIMD */
  TH32_VAND,
  TH32_VBIC,
  TH32_VORR,
  TH32_VORN,
  TH32_VEOR,
  TH32_VBSL,
  TH32_VBIT,
  TH32_VBIF,
  TH32_VADD,
  TH32_VSUB,
  TH32_VTST,
  TH32_VCEQ,
  TH32_VMLA,
  TH32_VMLS,
  TH32_VQDMULH,
  TH32_VQRDMULH,
  TH32_VPADD,
  TH32_VFMA,
  TH32_VFMS,
  TH32_VADD_F,
  TH32_VSUB_F,
  TH32_VPADD_F,
  TH32_VABD_F,
  TH32_VMLA_F,
  TH32_VMLS_F,
  TH32_VMUL_F,
  TH32_VCEQ_F,
  TH32_VCGE_F,
  TH32_VCGT_F,
  TH32_VACGE_F,
  TH32_VACGT_F,
  TH32_VMAX_F,
  TH32_VMIN_F,
  TH32_VPMAX_F,
  TH32_VPMIN_F,
  TH32_VRECPS_F,
  TH32_VRSQRTS_F,
  TH32_VHADD,
  TH32_VQADD,
  TH32_VRHADD,
  TH32_VHSUB,
  TH32_VQSUB,
  TH32_VCGT,
  TH32_VCGE,
  TH32_VSHL,
  TH32_VQSHL,
  TH32_VRSHL,
  TH32_VQRSHL,
  TH32_VMAX,
  TH32_VMIN,
  TH32_VABD,
  TH32_VABA,
  TH32_VMUL,
  TH32_VPMAX,
  TH32_VPMIN,

  TH32_VMOV_IMM_3,
  TH32_VMVN_IMM_2,
  TH32_VMOV_IMM_1,
  TH32_VORR_IMM_1,
  TH32_VMVN_IMM_1,
  TH32_VBIC_IMM_1,
  TH32_VMOV_IMM_2,
  TH32_VMOV_IMM_0,
  TH32_VORR_IMM_0,
  TH32_VMVN_IMM_0,
  TH32_VBIC_IMM_0,

  TH32_VSHRN,
  TH32_VRSHRN,
  TH32_VQSHRUN,
  TH32_VQRSHRUN,
  TH32_VSRI,
  TH32_VSHL_IMM,
  TH32_VSLI,
  TH32_VMOVL1,
  TH32_VMOVL2,
  TH32_VMOVL3,
  TH32_VQSHRN,
  TH32_VQRSHRN,
  TH32_VSHLL_IMM,
  TH32_VSHR,
  TH32_VSRA,
  TH32_VRSHR,
  TH32_VRSRA,
  TH32_VQSHL_IMM,
  TH32_VQSHLU_IMM,
  TH32_VCVT_FX,

  TH32_VMOVN,
  TH32_VQMOVUN,
  TH32_VSHLL,
  TH32_VREV64,
  TH32_VREV32,
  TH32_VREV16,
  TH32_VCLS,
  TH32_VCLZ,
  TH32_VCNT,
  TH32_VMVN,
  TH32_VQABS,
  TH32_VQNEG,
  TH32_VSWP,
  TH32_VTRN,
  TH32_VUZP,
  TH32_VZIP,
  TH32_VQMOVN,
  TH32_VPADDL,
  TH32_VPADAL,
  TH32_VCVT_HS,
  TH32_VRECPE,
  TH32_VRSQRTE,
  TH32_VCVT_FI,
  TH32_VCGT_IMM,
  TH32_VCGE_IMM,
  TH32_VCEQ_IMM,
  TH32_VCLE_IMM,
  TH32_VCLT_IMM,
  TH32_VABS,
  TH32_VNEG,

  TH32_VDUP_SCALAR,
  TH32_VTBL,
  TH32_VTBX,
  TH32_VEXT,

  TH32_VADDHN,
  TH32_VRADDHN,
  TH32_VSUBHN,
  TH32_VRSUBHN,
  TH32_VQDMLAL,
  TH32_VQDMLSL,
  TH32_VQDMULL,
  TH32_VADDL,
  TH32_VADDW,
  TH32_VSUBL,
  TH32_VSUBW,
  TH32_VABAL,
  TH32_VABDL,
  TH32_VMLAL,
  TH32_VMLSL,
  TH32_VMULL,
  TH32_VMULL_POLY,

  TH32_VQDMLAL_SCALAR,
  TH32_VQDMLSL_SCALAR,
  TH32_VQDMULL_SCALAR,
  TH32_VMLAL_SCALAR,
  TH32_VMLSL_SCALAR,
  TH32_VMULL_SCALAR,
  TH32_VQDMULH_SCALAR,
  TH32_VQRDMULH_SCALAR,
  TH32_VMLA_SCALAR,
  TH32_VMLS_SCALAR,
  TH32_VMUL_SCALAR,

  TH32_VST1_MULTI2OR4,
  TH32_VST1_MULTI1OR3,
  TH32_VST2_MULTI2,
  TH32_VST2_MULTI1,
  TH32_VST3_MULTI,
  TH32_VST4_MULTI,
  TH32_VST1_ONE,
  TH32_VST2_ONE,
  TH32_VST3_ONE,
  TH32_VST4_ONE,
  TH32_VLD1_MULTI2OR4,
  TH32_VLD1_MULTI1OR3,
  TH32_VLD2_MULTI2,
  TH32_VLD2_MULTI1,
  TH32_VLD3_MULTI,
  TH32_VLD4_MULTI,
  TH32_VLD1_ALL,
  TH32_VLD1_ONE,
  TH32_VLD2_ALL,
  TH32_VLD2_ONE,
  TH32_VLD3_ALL,
  TH32_VLD3_ONE,
  TH32_VLD4_ALL,
  TH32_VLD4_ONE,

/* ARM v6 */
	TH32_NOP,//ARM_NOP
	TH32_YIELD,
	TH32_WFE,
	TH32_WFI,
	TH32_SEV,
	TH32_DBG,
	TH32_SMC,

	TH32_RBIT,//ARM_RBIT
	TH32_BFC,
	TH32_BFI,
	TH32_SBFX,
	TH32_UBFX,

	TH32_LDREX,//ARM_LDREX
	TH32_LDREXD,
	TH32_LDREXB,
	TH32_LDREXH,
	TH32_STREX,
	TH32_STREXD,
	TH32_STREXB,
	TH32_STREXH,

	TH32_SRS,//ARM_SRS
	TH32_RFE,
	TH32_HVC,
	TH32_ERET,

	TH32_CPS,//ARM_CPSIE
	TH32_CPS,
	TH32_CPS,

	TH32_MOVT,//ARM_MOVT
	TH32_MOVW,
	TH32_PKHBT,
	TH32_PKHTB,
	TH32_REV,
	TH32_REV16,
	TH32_REVSH,
	TH32_SXTH,
	TH32_SXTB16,
	TH32_SXTB,
	TH32_UXTH,
	TH32_UXTB16,
	TH32_UXTB,
	TH32_QADD16,
	TH32_QADD8,
	TH32_QASX,
	TH32_QSUB16,
	TH32_QSUB8,
	TH32_QSAX,
	TH32_SADD16,
	TH32_SADD8,
	TH32_SASX,
	TH32_SHADD16,
	TH32_SHADD8,
	TH32_SHASX,
	TH32_SHSUB16,
	TH32_SHSUB8,
	TH32_SHSAX,
	TH32_SSUB16,
	TH32_SSUB8,
	TH32_SSAX,
	TH32_UADD16,
	TH32_UADD8,
	TH32_UASX,
	TH32_UHADD16,
	TH32_UHADD8,
	TH32_UHASX,
	TH32_UHSUB16,
	TH32_UHSUB8,
	TH32_UHSAX,
	TH32_UQADD16,
	TH32_UQADD8,
	TH32_UQASX,
	TH32_UQSUB16,
	TH32_UQSUB8,
	TH32_UQSAX,
	TH32_USUB16,
	TH32_USUB8,
	TH32_USAX,
	TH32_SXTAH,
	TH32_SXTAB16,
	TH32_SXTAB,
	TH32_UXTAH,
	TH32_UXTAB16,
	TH32_UXTAB,
	TH32_SEL,
	TH32_SMUAD,
	TH32_SMUADX,
	TH32_SMUSD,
	TH32_SMUSDX,
	TH32_SMLAD,
	TH32_SMLADX,
	TH32_SMLALD,
	TH32_SMLALDX,
	TH32_SMLSD,
	TH32_SMLSDX,
	TH32_SMLSLD,
	TH32_SMLSLDX,
	TH32_SMMUL,
	TH32_SMMULR,
	TH32_SMMLA,
	TH32_SMMLAR,
	TH32_SMMLS,
	TH32_SMMLSR,
	TH32_SSAT,
	TH32_SSAT16,
	TH32_UMAAL,
	TH32_USAD8,
	TH32_USADA8,
	TH32_USAT,
	TH32_USAT16,

	TH32_SDIV,//ARM_SDIV
	TH32_UDIV,

	TH32_PLD,
	TH32_PLDW,
	TH32_PLI,

	TH32_B,
	TH32_BL,
	TH32_UNDEF,
	TH32_BLX,

	TH32_UNDEF,

  TH32_UNDEF,
  TH32_QADD,
  TH32_QDADD,
  TH32_QDSUB,
  TH32_QSUB,
  TH32_SMLABB,
  TH32_SMLABT,
  TH32_SMLATB,
  TH32_SMLATT,
  TH32_SMLAWB,
  TH32_SMLAWT,
  TH32_SMLALBB,
  TH32_SMLALBT,
  TH32_SMLALTB,
  TH32_SMLALTT,
  TH32_SMULBB,
  TH32_SMULBT,
  TH32_SMULTB,
  TH32_SMULTT,
  TH32_SMULWB,
  TH32_SMULWT,
  TH32_MLS,
  TH32_MUL,
  TH32_MLA,
  TH32_UMULL,
  TH32_UMLAL,
  TH32_SMULL,
  TH32_SMLAL,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_MRS,
  TH32_MRS_BANKED,
  TH32_MSR,
  TH32_MSR_BANKED,
  TH32_STRH,
  TH32_LDRH,
  TH32_LDRSH,
  TH32_LDRSB,
  TH32_LDRD,
  TH32_STRD,
  TH32_AND,
  TH32_EOR,
  TH32_SUB,
  TH32_RSB,
  TH32_ADD,
  TH32_ADC,
  TH32_SBC,
  TH32_UNDEF,
  TH32_TST,
  TH32_TEQ,
  TH32_CMP,
  TH32_CLZ,
  TH32_CMN,
  TH32_ORR,
  TH32_MOV,
  TH32_BIC,
  TH32_MVN,
  TH32_STR,
  TH32_LDR,
  TH32_STRB,
  TH32_LDRB,
  TH32_STM,
  TH32_LDM,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,

  TH32_VMOV_C2S,
  TH32_VMSR,
  TH32_VMOV_C2SCALAR,
  TH32_VDUP,
  TH32_VMOV_S2C,
  TH32_VMRS,
  TH32_VMOV_SCALAR2C,

  TH32_VMLA_F64,
  TH32_VMLS_F64,
  TH32_VNMLA,
  TH32_VNMLS,
  TH32_VNMUL,
  TH32_VMUL_F64,
  TH32_VADD_F64,
  TH32_VSUB_F64,
  TH32_VDIV,
  TH32_VFNMA,
  TH32_VFNMS,
  TH32_VFMA_F64,
  TH32_VFMS_F64,

  TH32_VMOV_FIMM,
  TH32_VMOV_F,
  TH32_VABS_F64,
  TH32_VNEG_F64,
  TH32_VSQRT,
  TH32_VCMP,
  TH32_VCMPE,

  TH32_VCVTB,
  TH32_VCVTT,
  TH32_VCVT_DS,
  TH32_VCVT_I2F,
  TH32_VCVT_X2F,
  TH32_VCVT_F2X,
  TH32_VCVT_F2I,
  TH32_VCVTR_F2I,

  TH32_VMOV64_C2S,
  TH32_VMOV64_C2D,

  TH32_VPUSH,
  TH32_VPOP,
  TH32_VSTR,
  TH32_VLDR,
  TH32_FSTMX,
  TH32_VSTM,
  TH32_FLDMX,
  TH32_VLDM,

  TH32_UNDEF,//TH32_FCPYS
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,

  TH32_UNDEF,//ARM_FMSTAT
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,

  TH32_UNDEF,//ARM_FMRRD
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,

  TH32_UNDEF,//ARM_FSTS
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,

  TH32_MRRC2,
  TH32_MCRR2,
  TH32_MRC2,
  TH32_MCR2,
  TH32_CDP2,
  TH32_STC2,
  TH32_LDC2,

  TH32_MRRC,
  TH32_MCRR,
  TH32_MRC,
  TH32_MCR,
  TH32_CDP,
  TH32_STC,
  TH32_LDC,

  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF,
  TH32_UNDEF
};

/* vim: set shiftwidth=2 foldmethod=marker : */
