/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#ifndef ARM_OPCODES_TYPEDEFS
#define ARM_OPCODES_TYPEDEFS
/*! ARM instruction condition codes
 *  The conditional execution of the instructions depends on whether the instruction
 *  works on integer or floating-point arguments. When the instruction is a floating-
 *  point instruction, 'unordered' means that at least one of its operands is NaN.
 */
typedef enum _t_arm_condition_code
{
  /*! Equal
   *    Only execute the instruction when the Z(ero)-flag is set.
   */
  ARM_CONDITION_EQ=0,
  /*! Not Equal
   *  Only execute the instruction when the Z(ERO)-flag is cleared,
   *  or when the instruction is unordered.
   */
  ARM_CONDITION_NE=1,
  /*! Carry Set
   *  Integer instructions are only executed when the C(arry)-flag is set.
   *  Floating-point instructions are only executed when the first operand
   *  is larger than, or equal to the second operand. They can also be unordered.
   */
  ARM_CONDITION_CS=2,
  /*! Carry Clear
   *  Integer instructions are only executed when the C(arry)-flag is cleared.
   *  Floating-point instructions are only executed when the first operand
   *  is less than the second operand.
   */
  ARM_CONDITION_CC=3,
  /*! Carry Clear
   *  Integer instructions are only executed when the N(egate)-flag is set.
   *  Floating-point instructions are only executed when the first operand
   *  is less than the second operand.
   */
  ARM_CONDITION_MI=4,
  /*! Carry Clear
   *  Integer instructions are only executed when the N(egate)-flag is cleared.
   *  Floating-point instructions are only executed when the first operand
   *  is greater than, or equal to the second operand. They can also be unordered.
   */
  ARM_CONDITION_PL=5,
  /*! Overflow
   *  Integer instructions are only executed when the V(overflow)-flag is set.
   *  Floating-point instructions are only executed when the instruction is unordered.
   */
  ARM_CONDITION_VS=6,
  /*! \todo Document */
  ARM_CONDITION_VC=7,
  /*! \todo Document */
  ARM_CONDITION_HI=8,
  /*! \todo Document */
  ARM_CONDITION_LS=9,
  /*! \todo Document */
  ARM_CONDITION_GE=10,
  /*! \todo Document */
  ARM_CONDITION_LT=11,
  /*! \todo Document */
  ARM_CONDITION_GT=12,
  /*! \todo Document */
  ARM_CONDITION_LE=13,
  /*! \todo Document */
  ARM_CONDITION_AL=14,
  /*! \todo Document */
  ARM_CONDITION_NV=15
} t_arm_condition_code;

/*! \todo Document */
typedef enum _t_arm_shift_type
{
  /*! \todo Document */
  ARM_SHIFT_TYPE_NONE=255,
  /*! \todo Document */
  ARM_SHIFT_TYPE_LSL_IMM=0,
  /*! \todo Document */
  ARM_SHIFT_TYPE_LSR_IMM=1,
  /*! \todo Document */
  ARM_SHIFT_TYPE_ASR_IMM=2,
  /*! \todo Document */
  ARM_SHIFT_TYPE_ROR_IMM=3,
  /*! \todo Document */
  ARM_SHIFT_TYPE_LSL_REG=4,
  /*! \todo Document */
  ARM_SHIFT_TYPE_LSR_REG=5,
  /*! \todo Document */
  ARM_SHIFT_TYPE_ASR_REG=6,
  /*! \todo Document */
  ARM_SHIFT_TYPE_ROR_REG=7,
  /*! \todo Document */
  ARM_SHIFT_TYPE_RRX=8	/*speciaal geval, geen shiftlengte nodig*/
} t_arm_shift_type;

typedef enum _t_arm_barrier_option
{
  ARM_BARRIER_OSHST   = 2,
  ARM_BARRIER_OSH     = 3,
  ARM_BARRIER_NSHST   = 6,
  ARM_BARRIER_NSH     = 7,
  ARM_BARRIER_ISHST   = 10,
  ARM_BARRIER_ISH     = 11,
  ARM_BARRIER_ST      = 14,
  ARM_BARRIER_SY      = 15
} t_barrier_option;

typedef enum _t_arm_opcode
{
  /* ARM V6/V7 Thumb-2 instructions */
#define ARM_T2_FIRST ARM_T2NOP
  ARM_T2NOP,
  ARM_T2YIELD,
  ARM_T2WFE,
  ARM_T2WFI,
  ARM_T2SEV,
  ARM_T2IT,
  ARM_T2ORN,
  ARM_T2CBNZ,
  ARM_T2CBZ,
  ARM_T2TBB,
  ARM_T2TBH,
#define ARM_T2_LAST ARM_T2TBH

  ARM_CLREX,
  ARM_SETEND,

  ARM_DMB,
  ARM_DSB,
  ARM_ISB,

  /* ARMv7 Advanced SIMD instructions */
#define ARM_SIMD_FIRST ARM_VAND
  #define ARM_SIMD_FIRST3SAME ARM_VAND
  /* opcode: 5 */
  ARM_VAND,
  ARM_VBIC,
  ARM_VORR,
  ARM_VORN,
  ARM_VEOR,
  ARM_VBSL,
  ARM_VBIT,
  ARM_VBIF,
  ARM_VADD,
  ARM_VSUB,
  ARM_VTST,
  ARM_VCEQ,
  ARM_VMLA,
  ARM_VMLS,
  ARM_VQDMULH,
  ARM_VQRDMULH,
  ARM_VPADD,
  ARM_VFMA,
  ARM_VFMS,
#define ARM_SIMD_3REGSSAMELENGTH_FIRSTF ARM_VADD_F
  ARM_VADD_F,
  ARM_VSUB_F,
  ARM_VPADD_F,
  ARM_VABD_F,
  ARM_VMLA_F,
  ARM_VMLS_F,
  ARM_VMUL_F,
  ARM_VCEQ_F,
  ARM_VCGE_F,
  ARM_VCGT_F,
  ARM_VACGE_F,
  ARM_VACGT_F,
  ARM_VMAX_F,
  ARM_VMIN_F,
  ARM_VPMAX_F,
  ARM_VPMIN_F,
  ARM_VRECPS_F,
  ARM_VRSQRTS_F,
#define ARM_SIMD_3REGSSAMELENGTH_LASTF ARM_VRSQRTS_F
  ARM_VHADD,
  ARM_VQADD,
  ARM_VRHADD,
  ARM_VHSUB,
  ARM_VQSUB,
  ARM_VCGT,
  ARM_VCGE,
  ARM_VSHL,
  ARM_VQSHL,
  ARM_VRSHL,
  ARM_VQRSHL,
  ARM_VMAX,
  ARM_VMIN,
  ARM_VABD,
  ARM_VABA,
  ARM_VMUL,
  ARM_VPMAX,
  ARM_VPMIN,
  #define ARM_SIMD_LAST3SAME ARM_VPMIN

  /*
   * modified immediate values
   * see Table A7-15 [ARM-DDI-0406C.b/A7-269]
   * _IMMi: i = index to table, start from 0
   */
#define ARM_SIMDIMM_FIRST ARM_VMOV_IMM_3
  ARM_VMOV_IMM_3,
  ARM_VMVN_IMM_2,
  ARM_VMOV_IMM_1,
  ARM_VORR_IMM_1,
  ARM_VMVN_IMM_1,
  ARM_VBIC_IMM_1,
  ARM_VMOV_IMM_2,
  ARM_VMOV_IMM_0,
  ARM_VORR_IMM_0,
  ARM_VMVN_IMM_0,
  ARM_VBIC_IMM_0,
#define ARM_SIMDIMM_LAST ARM_VBIC_IMM_0

  /* 2 registers, shift
   */
#define ARM_SIMD2REGSSHIFT_FIRST ARM_VSHRN
  ARM_VSHRN,
  ARM_VRSHRN,
  ARM_VQSHRUN,
  ARM_VQRSHRUN,
  ARM_VSRI,
  ARM_VSHL_IMM,
  ARM_VSLI,
  ARM_VMOVL1,
  ARM_VMOVL2,
  ARM_VMOVL3,
  ARM_VQSHRN,
  ARM_VQRSHRN,
  ARM_VSHLL_IMM,
  ARM_VSHR,
  ARM_VSRA,
  ARM_VRSHR,
  ARM_VRSRA,
  ARM_VQSHL_IMM,
  ARM_VQSHLU_IMM,
  ARM_VCVT_FX,
#define ARM_SIMD2REGSSHIFT_LAST ARM_VCVT_FX

  /* 2 registers, miscellaneous value
   */
#define ARM_SIMD2MISC_FIRST ARM_VMOVN
  ARM_VMOVN,
  ARM_VQMOVUN,
  ARM_VSHLL,
  ARM_VREV64,
  ARM_VREV32,
  ARM_VREV16,
  ARM_VCLS,
  ARM_VCLZ,
  ARM_VCNT,
  ARM_VMVN,
  ARM_VQABS,
  ARM_VQNEG,
  ARM_VSWP,
  ARM_VTRN,
  ARM_VUZP,
  ARM_VZIP,
  ARM_VQMOVN,
  ARM_VPADDL,
  ARM_VPADAL,
  ARM_VCVT_HS,
  ARM_VRECPE,
  ARM_VRSQRTE,
  ARM_VCVT_FI,
  ARM_VCGT_IMM,
  ARM_VCGE_IMM,
  ARM_VCEQ_IMM,
  ARM_VCLE_IMM,
  ARM_VCLT_IMM,
  ARM_VABS,
  ARM_VNEG,
#define ARM_SIMD2MISC_LAST ARM_VNEG

#define ARM_SIMDVAR_FIRST ARM_VDUP_SCALAR
  /* entry in table at [ARM-DDI-0406C.b/A7-261] */
  ARM_VDUP_SCALAR,
  /* entry in table at [ARM-DDI-0406C.b/A7-261] */
  ARM_VTBL,
  ARM_VTBX,
  ARM_VEXT,
#define ARM_SIMDVAR_LAST ARM_VEXT

  /* 3 registers, different length
   */
#define ARM_SIMD3REGSDIFFLENGTH_FIRST ARM_VADDHN
  ARM_VADDHN,
  ARM_VRADDHN,
  ARM_VSUBHN,
  ARM_VRSUBHN,
  ARM_VQDMLAL,
  ARM_VQDMLSL,
  ARM_VQDMULL,
  ARM_VADDL,
  ARM_VADDW,
  ARM_VSUBL,
  ARM_VSUBW,
  ARM_VABAL,
  ARM_VABDL,
  ARM_VMLAL,
  ARM_VMLSL,
  ARM_VMULL,
  ARM_VMULL_POLY,
#define ARM_SIMD3REGSDIFFLENGTH_LAST ARM_VMULL_POLY

  /* 2 registers, scalar
   */
#define ARM_SIMD2REGSSCALAR_FIRST ARM_VQDMLAL_SCALAR
  ARM_VQDMLAL_SCALAR,
  ARM_VQDMLSL_SCALAR,
  ARM_VQDMULL_SCALAR,
  ARM_VMLAL_SCALAR,
  ARM_VMLSL_SCALAR,
  ARM_VMULL_SCALAR,
  ARM_VQDMULH_SCALAR,
  ARM_VQRDMULH_SCALAR,
  ARM_VMLA_SCALAR,
  ARM_VMLS_SCALAR,
  ARM_VMUL_SCALAR,
#define ARM_SIMD2REGSSCALAR_LAST ARM_VMUL_SCALAR

  /* element or structure load/store instructions
  TODO: fix ordering
   */
#define ARM_SIMD_FIRSTSTORE ARM_VST1_MULTI2OR4
  ARM_VST1_MULTI2OR4,
  ARM_VST1_MULTI1OR3,
  ARM_VST2_MULTI2,
  ARM_VST2_MULTI1,
  ARM_VST3_MULTI,
  ARM_VST4_MULTI,
  ARM_VST1_ONE,
  ARM_VST2_ONE,
  ARM_VST3_ONE,
  ARM_VST4_ONE,
#define ARM_SIMD_LASTSTORE ARM_VST4_ONE
#define ARM_SIMD_FIRSTLOAD ARM_VLD1_MULTI2OR4
  ARM_VLD1_MULTI2OR4,
  ARM_VLD1_MULTI1OR3,
  ARM_VLD2_MULTI2,
  ARM_VLD2_MULTI1,
  ARM_VLD3_MULTI,
  ARM_VLD4_MULTI,
  ARM_VLD1_ALL,
  ARM_VLD1_ONE,
  ARM_VLD2_ALL,
  ARM_VLD2_ONE,
  ARM_VLD3_ALL,
  ARM_VLD3_ONE,
  ARM_VLD4_ALL,
  ARM_VLD4_ONE,
#define ARM_SIMD_LASTLOAD ARM_VLD4_ONE

#define ARM_SIMD_LAST ARM_VLD4_ONE

  ARM_NOP,
  ARM_YIELD,
  ARM_WFE,
  ARM_WFI,
  ARM_SEV,
  ARM_DBG,
  ARM_SMC,

#define ARM_BITFIELD_FIRST ARM_RBIT
  ARM_RBIT,
  ARM_BFC,
  ARM_BFI,
  ARM_SBFX,
  ARM_UBFX,
#define ARM_BITFIELD_LAST ARM_UBFX

#define ARM_LDRSTREX_FIRST ARM_LDREX
  ARM_LDREX,
  ARM_LDREXD,
  ARM_LDREXB,
  ARM_LDREXH,
  ARM_STREX,
  ARM_STREXD,
  ARM_STREXB,
  ARM_STREXH,
#define ARM_LDRSTREX_LAST ARM_STREXH

  ARM_SRS,
  ARM_RFE,
  ARM_HVC,
  ARM_ERET,


  /* ARM V6 instructions. */
#define ARM_V6_FIRST ARM_CPSIE
  ARM_CPSIE,
  ARM_CPSID,
  ARM_CPS,

  ARM_MOVT,
  ARM_MOVW,
  ARM_PKHBT,
  ARM_PKHTB,
  ARM_REV,
  ARM_REV16,
  ARM_REVSH,
  ARM_SXTH,
  ARM_SXTB16,
  ARM_SXTB,
  ARM_UXTH,
  ARM_UXTB16,
  ARM_UXTB,
  ARM_QADD16,
  ARM_QADD8,
  ARM_QADDSUBX,
  ARM_QSUB16,
  ARM_QSUB8,
  ARM_QSUBADDX,
  ARM_SADD16,
  ARM_SADD8,
  ARM_SADDSUBX,
  ARM_SHADD16,
  ARM_SHADD8,
  ARM_SHADDSUBX,
  ARM_SHSUB16,
  ARM_SHSUB8,
  ARM_SHSUBADDX,
  ARM_SSUB16,
  ARM_SSUB8,
  ARM_SSUBADDX,
  ARM_UADD16,
  ARM_UADD8,
  ARM_UADDSUBX,
  ARM_UHADD16,
  ARM_UHADD8,
  ARM_UHADDSUBX,
  ARM_UHSUB16,
  ARM_UHSUB8,
  ARM_UHSUBADDX,
  ARM_UQADD16,
  ARM_UQADD8,
  ARM_UQADDSUBX,
  ARM_UQSUB16,
  ARM_UQSUB8,
  ARM_UQSUBADDX,
  ARM_USUB16,
  ARM_USUB8,
  ARM_USUBADDX,
  ARM_SXTAH,
  ARM_SXTAB16,
  ARM_SXTAB,
  ARM_UXTAH,
  ARM_UXTAB16,
  ARM_UXTAB,
  ARM_SEL,
  ARM_SMUAD,
  ARM_SMUADX,
  ARM_SMUSD,
  ARM_SMUSDX,
  ARM_SMLAD,
  ARM_SMLADX,
  ARM_SMLALD,
  ARM_SMLALDX,
  ARM_SMLSD,
  ARM_SMLSDX,
  ARM_SMLSLD,
  ARM_SMLSLDX,
  ARM_SMMUL,
  ARM_SMMULR,
  ARM_SMMLA,
  ARM_SMMLAR,
  ARM_SMMLS,
  ARM_SMMLSR,
  ARM_SSAT,
  ARM_SSAT16,
  ARM_UMAAL,
  ARM_USAD8,
  ARM_USADA8,
  ARM_USAT,
  ARM_USAT16,
#define ARM_V6_LAST ARM_USAT16

  ARM_SDIV,
  ARM_UDIV,

  /*! preload memory system hint */
  ARM_PLD,
  ARM_PLDW,
  ARM_PLI,
  /*! An Arm Branch instruction */
  ARM_B,
  /*! An Arm Branch And Link instruction */
  ARM_BL,
  /*! An Arm Branch And Exchange instruction */
  ARM_BX,
  /*! An Arm Branch, Link And Exchange instruction */
  ARM_BLX,
  /*! An Arm Software Interrupt */
  ARM_SWI,
  /* A software breakpoint */
  ARM_BKPT,
  /*! saturated arithmetic instruction */
  ARM_QADD,
  /*! saturated arithmetic instruction */
  ARM_QDADD,
  /*! saturated arithmetic instruction */
  ARM_QDSUB,
  /*! saturated arithmetic instruction */
  ARM_QSUB,
  /*! saturated arithmetic instruction */
  ARM_SMLABB,
  /*! saturated arithmetic instruction */
  ARM_SMLABT,
  /*! saturated arithmetic instruction */
  ARM_SMLATB,
  /*! saturated arithmetic instruction */
  ARM_SMLATT,
  /*! saturated arithmetic instruction */
  ARM_SMLAWB,
  /*! saturated arithmetic instruction */
  ARM_SMLAWT,
  /*! saturated arithmetic instruction */
  ARM_SMLALBB,
  /*! saturated arithmetic instruction */
  ARM_SMLALBT,
  /*! saturated arithmetic instruction */
  ARM_SMLALTB,
  /*! saturated arithmetic instruction */
  ARM_SMLALTT,
  /*! saturated arithmetic instruction */
  ARM_SMULBB,
  /*! saturated arithmetic instruction */
  ARM_SMULBT,
  /*! saturated arithmetic instruction */
  ARM_SMULTB,
  /*! saturated arithmetic instruction */
  ARM_SMULTT,
  /*! saturated arithmetic instruction */
  ARM_SMULWB,
  /*! saturated arithmetic instruction */
  ARM_SMULWT,
  ARM_MLS,
  /*! An Arm Multiply instruction */
  ARM_MUL,
  /*! An Arm Multiply and accumulate instruction */
  ARM_MLA,
  /*! An Unsigned Arm Multiply Long */
  ARM_UMULL,
  /*! An Unsigned Arm Multiply and Accumulate Long */
  ARM_UMLAL,
  ARM_SMULL,
  ARM_SMLAL,
  /*! An Arm Swap instruction */
  ARM_SWP,
  ARM_SWPB,
  ARM_MRS,
  ARM_MRS_BANKED,
  ARM_MSR,
  ARM_MSR_BANKED,
  ARM_STRH,
  ARM_LDRH,
  ARM_LDRSH,
  ARM_LDRSB,
  ARM_LDRD,
  ARM_STRD,
  ARM_AND,
  ARM_EOR,
  ARM_SUB,
  ARM_RSB,
  ARM_ADD,
  ARM_ADC,
  ARM_SBC,
  ARM_RSC,
  ARM_TST,
  ARM_TEQ,
  ARM_CMP,
  ARM_CLZ,
  ARM_CMN,
  ARM_ORR,
  ARM_MOV,
  ARM_BIC,
  ARM_MVN,
  ARM_STR,
  ARM_LDR,
  ARM_STRB,
  ARM_LDRB,
  ARM_STM,
  ARM_LDM,
  ARM_ADF,
  ARM_MUF,
  ARM_SUF,
  ARM_RSF,
  ARM_DVF,
  ARM_RDF,
  ARM_POW,
  ARM_RPW,
  ARM_RMF,
  ARM_FML,
  ARM_FDV,
  ARM_FRD,
  ARM_POL,
  ARM_MVF,
  ARM_MNF,
  ARM_ABS,
  ARM_RND,
  ARM_SQT,
  ARM_LOG,
  ARM_LGN,
  ARM_EXP,
  ARM_SIN,
  ARM_COS,
  ARM_TAN,
  ARM_ASN,
  ARM_ACS,
  ARM_ATN,
  ARM_URD,
  ARM_NRM,
  ARM_FLT,
  ARM_FIX,
  ARM_WFS,
  ARM_RFS,
  ARM_WFC,
  ARM_RFC,
  ARM_CMF,
  ARM_CNF,
  ARM_CMFE,
  ARM_CNFE,
  ARM_STF,
  ARM_LDF,
  ARM_SFM,
  ARM_LFM,


  /* 8, 16 and 32-bit transfer between ARM core and extension registers */
#define ARM_SIMDRT_FIRST ARM_VMOV_C2S
  ARM_VMOV_C2S,
  ARM_VMSR,
  ARM_VMOV_C2SCALAR,
  ARM_VDUP,
  ARM_VMOV_S2C,
  ARM_VMRS,
  ARM_VMOV_SCALAR2C,
#define ARM_SIMDRT_LAST ARM_VMOV_SCALAR2C


   /* Vector floating point coprocessor (VFP) data processing instructions */
#define ARM_FP_FIRST ARM_VMLA_F64
   /* 3 registers */
  ARM_VMLA_F64,
  ARM_VMLS_F64,
  ARM_VNMLA,
  ARM_VNMLS,
  ARM_VNMUL,
  ARM_VMUL_F64,
  ARM_VADD_F64,
  ARM_VSUB_F64,
  ARM_VDIV,
  ARM_VFNMA,
  ARM_VFNMS,
  ARM_VFMA_F64,
  ARM_VFMS_F64,

   /* other data-processing instructions */
   ARM_VMOV_FIMM,
  ARM_VMOV_F,
  ARM_VABS_F64,
  ARM_VNEG_F64,
  ARM_VSQRT,
  ARM_VCMP,
  ARM_VCMPE,

#define ARM_FP_VCVT_FIRST ARM_VCVTB
  ARM_VCVTB,
  ARM_VCVTT,
  ARM_VCVT_DS,
  ARM_VCVT_I2F,
  ARM_VCVT_X2F,
  ARM_VCVT_F2X,
  ARM_VCVT_F2I,
  ARM_VCVTR_F2I,
#define ARM_FP_VCVT_LAST ARM_VCVTR_F2I
#define ARM_FP_LAST ARM_VCVTR_F2I

  /* 64-bit transfers between ARM code and extension registers */
  ARM_VMOV64_C2S,
  ARM_VMOV64_C2D,

   /* extension register load/store instructions */
#define ARM_SIMD_LOADSTORE_FIRST ARM_VPUSH
  ARM_VPUSH,
  ARM_VPOP,
  ARM_VSTR,
  ARM_VLDR,
  ARM_FSTMX,
  ARM_VSTM,
  ARM_FLDMX,
  ARM_VLDM,
#define ARM_SIMD_LOADSTORE_LAST ARM_VLDM

#define ARM_VFP_FIRST ARM_FCPYS
#define ARM_VFP_DP_FIRST ARM_FCPYS
  ARM_FCPYS,
  ARM_FABSS,
  ARM_FCPYD,
  ARM_FABSD,
  ARM_FNEGS,
  ARM_FSQRTS,
  ARM_FNEGD,
  ARM_FSQRTD,
  ARM_FCVTDS,
  ARM_FCVTSD,
  ARM_FCMPS,
  ARM_FCMPD,
  ARM_FCMPES,
  ARM_FCMPED,
  ARM_FCMPZS,
  ARM_FCMPZD,
  ARM_FCMPEZS,
  ARM_FCMPEZD,
  ARM_FUITOS,
  ARM_FUITOD,
  ARM_FSITOS,
  ARM_FSITOD,
  ARM_FTOUIS,
  ARM_FTOUID,
  ARM_FTOUIZS,
  ARM_FTOUIZD,
  ARM_FTOSIS,
  ARM_FTOSID,
  ARM_FTOSIZS,
  ARM_FTOSIZD,
/* can't find documentation for them
  ARM_FCONSTS,
  ARM_FCONSTD,
*/
  ARM_FMACS,
  ARM_FNMACS,
  ARM_FMACD,
  ARM_FNMACD,
  ARM_FMSCS,
  ARM_FNMSCS,
  ARM_FMSCD,
  ARM_FNMSCD,
  ARM_FMULS,
  ARM_FNMULS,
  ARM_FMULD,
  ARM_FNMULD,
  ARM_FADDS,
  ARM_FSUBS,
  ARM_FADDD,
  ARM_FSUBD,
  ARM_FDIVS,
  ARM_FDIVD,
  /* Vector floating point coprocessor (VFP) single register transfer instructions */
#define ARM_VFP_RT_FIRST ARM_FMSTAT
  ARM_FMSTAT,
  ARM_FMDLR,
  ARM_FMRDL,
  ARM_FMDHR,
  ARM_FMRDH,
  ARM_FMXR,
  ARM_FMRX,
  ARM_FMSR,
  ARM_FMRS,
  /* Vector loating point coprocessor (VFP) two-register transfer instructions */
#define ARM_VFP_R2T_FIRST ARM_FMRRD
  ARM_FMRRD,
  ARM_FMSRR,
  ARM_FMDRR,
  ARM_FMRRS,
  /* Vector floating point coprocessor (VFP) data transfer instructions */
#define ARM_VFP_DT_FIRST ARM_FSTS
  ARM_FSTS,
  ARM_FSTD,
  ARM_FLDS,
  ARM_FLDD,
  ARM_FSTMS,
  ARM_FSTMD,
//  ARM_FSTMX,
  ARM_FLDMS,
  ARM_FLDMD,
//  ARM_FLDMX,
#define ARM_VFP_LAST ARM_FLDMX

  /*  Generic coprocessor instructions (catch-alls) */
  ARM_MRRC2,
  ARM_MCRR2,
  ARM_MRC2,
  ARM_MCR2,
  ARM_CDP2,
  ARM_STC2,
  ARM_LDC2,

  ARM_MRRC,
  ARM_MCRR,
  ARM_MRC,
  ARM_MCR,
  ARM_CDP,
  ARM_STC,
  ARM_LDC,

  /*! Pseudo op, used in the opcode table */
  ARM_UNDEF,
  /*! Pseudo instruction, used to represent data in basic blocks (=data blocks,
   * for data in the code section)*/
  ARM_DATA    ,
  /*! Pseudo instruction that produces an address (=integer value+ relocation) */
  ARM_ADDRESS_PRODUCER,
  /*! Pseudo instruction that produces an constant(=integer value) */
  ARM_CONSTANT_PRODUCER,
  /*! Pseudo instruction that produces a floating point value in one of the
   * possible encodings */
  ARM_FLOAT_PRODUCER,
  /*! Pseudo instruction that produces a VFP floating point value in one of the
   * possible encodings */
  ARM_VFPFLOAT_PRODUCER,
  /* Pseudo instruction to detect thumb versions of BX pc in order to align them correctly */
  TH_BX_R15,
  /*! Pseudo instruction, used to add calls during instrumentation */
  ARM_PSEUDO_CALL
} t_arm_opcode;

#endif

#ifdef DIABLOARM_TYPES
#ifndef ARM_OPCODES_TYPES
#define ARM_OPCODES_TYPES
/*! The structure of the different disassemble functions */
typedef void (*ArmDisassembleFunction) (t_arm_ins *, t_uint32 instr, t_uint16 opc);
/*! The opcode structure, used in the opcode table to specify what disassembly
 * function corresponds to what bitpattern */
typedef struct _arm_opcode
{
  /*! \todo Document */
  t_uint32 mask;
  /*! \todo Document */
  t_uint32 opcode;
  /*! \todo Document */
  t_string desc;
  /*! \todo Document */
  ArmDisassembleFunction Disassemble;
} arm_opcode;
/*! The opcode table */
extern const arm_opcode arm_opcode_table[];
/* Condition aliases */
#define ARM_CONDITION_HS		ARM_CONDITION_CS
#define ARM_CONDITION_LO		ARM_CONDITION_CC
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
