/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#ifndef ARM_INSTRUCTION_TYPEDEFS
#define ARM_INSTRUCTION_TYPEDEFS
typedef struct _t_arm_addr_info t_arm_addr_info;
#endif

#ifndef ARM_INSTRUCTION_H
#define ARM_INSTRUCTION_H

#define ARM_INS_OBJECT(ins) CFG_OBJECT(ARM_INS_CFG(ins))
#define ARM_INS_IS_CONDITIONAL(x)   (ARM_INS_ATTRIB(x) & IF_CONDITIONAL)
#define ARM_INS_IS_MEMORY(x)	(ARM_INS_TYPE(x) == IT_LOAD || ARM_INS_TYPE(x) == IT_STORE || ARM_INS_TYPE(x) == IT_STORE_MULTIPLE || ARM_INS_TYPE(x) == IT_LOAD_MULTIPLE || ARM_INS_TYPE(x) == IT_FLT_STORE || ARM_INS_TYPE(x) == IT_FLT_LOAD)
#define BBL_FOREACH_ARM_INS(bbl,ins) for(ins=T_ARM_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=ARM_INS_INEXT(ins))
#define BBL_FOREACH_ARM_INS_R(bbl,ins) for(ins=T_ARM_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=ARM_INS_IPREV(ins))
#define BBL_FOREACH_ARM_INS_SAFE(bbl,ins,tmp)   for(ins=T_ARM_INS(BBL_INS_FIRST(bbl)), tmp=ins?ARM_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?ARM_INS_INEXT(ins):0)
#define SECTION_FOREACH_ARM_INS(code,ins) for(ins=T_ARM_INS(SECTION_DATA(code)); ins!=NULL; ins=ARM_INS_INEXT(ins))

/* convenient shortcut when dynamically generating code. Based on I386MakeInsForBbl.
 * e.g. ArmMakeInsForBbl(Noop,Append,ins,bbl,isthumb) will append a noop to bbl */
#define ArmMakeInsForBbl(type,where,ins,bbl,isthumb, ...)    \
do {                                                  \
    ins = T_ARM_INS(InsNewForBbl(bbl));              \
    if (isthumb) ARM_INS_SET_FLAGS(ins,FL_THUMB);         \
    ArmInsMake ## type(ins, ## __VA_ARGS__); \
    Ins ## where ## ToBbl(T_INS(ins),bbl);            \
} while (0)

#define ArmMakeInsForIns(type,where,ins,existing,isthumb, ...)     \
do {                                                        \
    ins = T_ARM_INS(InsNewForBbl(ARM_INS_BBL(existing))); \
    if (isthumb) ARM_INS_SET_FLAGS(ins,FL_THUMB);         \
    ArmInsMake ## type(ins, ## __VA_ARGS__);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
} while (0)

#define ArmInsMakeAddressProducer(i, j, k) ArmMakeAddressProducer(__FILE__, __LINE__, i, j, k)

struct _t_arm_addr_info
{
  char original_type;
  t_uint32 load_address;
  t_uint32 load_value;
};


/*! \todo Document */
#define T_ARM_INS(arm_ins)            ((t_arm_ins *) arm_ins)

/* Instruction flags {{{ */
#define FL_S	        	0x1
#define FL_PREINDEX		0x2
#define FL_DIRUP		0x4
#define FL_WRITEBACK		0x8
#define FL_SPSR			0x10
#define FL_IMMED		0x20	/* instructie heeft immediate tweede operand -> voor gevallen */
				        /* waarin twijfel kan bestaan */
#define FL_IMMEDW		0x40
#define FL_CONTROL		0x80
#define FL_STATUS		0x100
#define FL_FLT_SINGLE       	0x200
#define FL_FLT_DOUBLE       	0x400
#define FL_FLT_DOUBLE_EXTENDED 	0x800
#define FL_FLT_PACKED 	        0x1000  
#define FL_FLT_IMMED		0x2000  /* instructie heeft float immediate ipv REGC */
#define FL_FLT_RPI		0x4000  /* float: rounding mode: towards plus infinity */
#define FL_FLT_RMI		0x8000  /* float: rounding mode: towards minus infinity */
#define FL_FLT_RZ		0x10000 /* float: rounding mode: towards zero */

#define USED_TO_BE_ADD_WITH_PC  0x10000 /* overloading the previous flag: this one will only be set for MOV instructions, which the previous won't */
                                
#define FL_NO_LOAD		0x20000 /* address producer MAY NOT load its value */
#define FL_THUMB		0x40000 /* thumb code */
#define FL_ORIG_CONST_PROD	0x80000
#define FL_EVAL_IN_COPY_PROP	0x100000
#define FL_NO_EVAL_FAST_IN_COPY_PROP 0x200000 

#define FL_LONG_TRANSFER	0x400000 /* generic flag for STC/LDC: meaning is coprocessor dependent */
#define FL_USERMODE_REGS	0x800000 /* load/store user mode registers in some supervisor mode */

#define FL_VFP_FPSCR            0x4000000 /* sets status bits in FPSCR */
#define FL_VFP_DOUBLE           0x8000000 /* double precision VFP instruction (-> uses two single precision regs) */

/* Used to prefer the 32-bit encoding of a Thumb instruction
    when both a 16- and a 32-bit encoding is available.
 */
#define FL_PREFER32             0x10000000

#define FL_SWITCHEDBL_SIGNEXT   0x20000000

#define FL_SWITCHEDBL_SZ_MASK   0xc0000000
#define FL_SWITCHEDBL_SZ_SHIFT  30

/* }}} */

/* Possible data types according to Figure A2-2 [ARM-DDI-0406C.b/A2-60] */
/* The corresponding string array definition is found in diabloarm_print.c */
typedef enum _t_arm_ins_dt
{
  DT_NONE       = 0,

#define DT_START DT_8
  DT_8          = 1,
  DT_16         = 2,
  DT_32         = 3,
  DT_64         = 4,
#define DT_END DT_64

#define DT_I_START DT_I8
  DT_I8         = 5,
  DT_I16        = 6,
  DT_I32        = 7,
  DT_I64        = 8,
#define DT_I_END DT_I64

#define DT_S_START DT_S8
  DT_S8         = 9,
  DT_S16        = 10,
  DT_S32        = 11,
  DT_S64        = 12,
#define DT_S_END DT_S64

#define DT_U_START DT_U8
  DT_U8         = 13,
  DT_U16        = 14,
  DT_U32        = 15,
  DT_U64        = 16,
#define DT_U_END DT_U64

#define DT_F_START DT_F8_UNUSED
  DT_F8_UNUSED  = 17,
  DT_F16        = 18,
  DT_F32        = 19,
  DT_F64        = 20,
#define DT_F_END DT_F64

#define DT_P_START DT_P8
  DT_P8         = 21,
  DT_P16        = 22,
  DT_P32_UNUSED = 23,
  DT_P64_UNUSED = 24
#define DT_P_END DT_P64_UNUSED
} t_arm_ins_dt;

typedef enum _t_arm_ins_itcond
{
  ITCOND_OMIT,

#define ITNUMINS_2_FIRST ITCOND_T
  ITCOND_T,
  ITCOND_E,
#define ITNUMINS_2_LAST ITCOND_E

#define ITNUMINS_3_FIRST ITCOND_TT
  ITCOND_TT,
  ITCOND_ET,
  ITCOND_TE,
  ITCOND_EE,
#define ITNUMINS_3_LAST ITCOND_EE

#define ITNUMINS_4_FIRST ITCOND_TTT
  ITCOND_TTT,
  ITCOND_ETT,
  ITCOND_TET,
  ITCOND_EET,
  ITCOND_TTE,
  ITCOND_ETE,
  ITCOND_TEE,
  ITCOND_EEE
#define ITNUMINS_4_LAST ITCOND_EEE
} t_arm_ins_itcond;
#define MAX_INS_IN_IT 4

typedef t_uint16 t_arm_neon_flags;
#define NEONFL_A_SINGLE 0x00000001
#define NEONFL_A_DOUBLE 0x00000002
#define NEONFL_A_QUAD   0x00000004
#define NEONFL_A_SCALAR 0x00000008
#define NEONFL_B_SINGLE 0x00000010
#define NEONFL_B_DOUBLE 0x00000020
#define NEONFL_B_QUAD   0x00000040
#define NEONFL_B_SCALAR 0x00000080
#define NEONFL_C_SINGLE 0x00000100
#define NEONFL_C_DOUBLE 0x00000200
#define NEONFL_C_QUAD   0x00000400
#define NEONFL_C_SCALAR 0x00000800
#define NEONFL_A_CORE   0x00001000
#define NEONFL_B_CORE   0x00002000
#define NEONFL_C_CORE   0x00004000
#define NEONFL_MULTI_SCALAR 0x00008000

#define MULTIPLESCALAR_ALL 0xff

#endif



#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_INSTRUCTION_FUNCTIONS
#define ARM_INSTRUCTION_FUNCTIONS
void ArmInsInit(t_arm_ins *ins);
void ArmInsMakeNoop(t_arm_ins * ins);
void ArmInsMakeData(t_arm_ins * ins, t_uint32 data);
void ArmInsMakeUncondThumbBranch(t_arm_ins * ins);
#define ArmInsMakeUncondBranch(ins) ArmInsMakeCondBranch(ins, ARM_CONDITION_AL)
#define ArmInsMakeUncondBranchExchange(ins,reg_b) ArmInsMakeCondBranchExchange(ins, ARM_CONDITION_AL, reg_b)
void ArmInsMakeCondBranchExchange(t_arm_ins * ins, t_uint32 cond, t_reg reg_b);
void ArmInsMakeCondBranch(t_arm_ins * ins, t_uint32 cond);
void ArmInsMakeCondBranchAndLink(t_arm_ins * ins, t_uint32 cond);
void ArmInsMakeCondBranchLinkAndExchange(t_arm_ins * ins, t_uint32 cond, t_reg reg_b);
void ArmInsMakeClz(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 cond);
void ArmInsMakeAdd(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeMul(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeDiv(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond); //custom
void ArmInsMakeCmp(t_arm_ins * ins, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeTst(t_arm_ins * ins, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeSub(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeAnd(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeMov(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmInsMakeMovwImmed(t_arm_ins * ins, t_reg regA, t_uint32 immed, t_uint32 cond);
void ArmInsMakePop(t_arm_ins * ins, t_uint32 regs, t_uint32 cond, t_bool thumb);
void ArmInsMakeLdm(t_arm_ins * ins, t_reg regB, t_uint32 regs, t_uint32 cond, t_bool writeback, t_bool preindex, t_bool dirup);
void ArmInsMakePush(t_arm_ins * ins, t_uint32 regs, t_uint32 cond, t_bool thumb);
void ArmInsMakeStm(t_arm_ins * ins, t_reg regB, t_uint32 regs, t_uint32 cond, t_bool writeback, t_bool preindex, t_bool dirup);
void ArmInsMakeMrs(t_arm_ins * ins, t_reg regA, t_uint32 cond);
void ArmInsMakeVmrs(t_arm_ins * ins, t_reg regA, t_uint32 cond);
void ArmMakeAddressProducer(const char * f, t_uint32 lnno, t_arm_ins * ins, t_uint32 immed, t_reloc * rel);
void ArmInsMakeLdr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up, t_bool wb);
void ArmInsMakeVldr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up);
void ArmInsMakeStr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up, t_bool wb);
void ArmInsMakeVstr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up);
void ArmInsMakeMsr(t_arm_ins * ins, t_reg regC, t_uint32 cond, t_bool set);
void ArmInsMakeVmsr(t_arm_ins * ins, t_reg regB, t_uint32 cond);
void ArmInsMakeMvn(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 immed, t_uint32 cond);
void ArmMakeConstantProducer(t_arm_ins * ins, t_uint32 immed);
void ArmMakeFloatProducer(t_arm_ins * ins, char * data);
void ArmInsMakePseudoCall(t_arm_ins * ins, t_function * fun);
void ArmInsMakeSwi(t_arm_ins * ins, t_uint32 immediate, t_uint32 cond);
void ArmInsMakeBkpt(t_arm_ins * ins);
void ArmInsMakeSwp(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 cond);
void ArmInsMakeIT(t_arm_ins * ins, t_uint32 spec);
t_bool ArmCompareInstructions(t_arm_ins *,t_arm_ins *);
t_bool ArmCompareInstructionsOppositeCond(t_arm_ins * i1, t_arm_ins * i2);
t_bool ArmInsHasSideEffect(t_arm_ins * ins);
t_tristate ArmIsSyscallExit(t_arm_ins * ins);
t_bool ArmIsControlflow(t_arm_ins * ins);
t_bool ArmInsUnconditionalize(t_arm_ins * ins);
t_arm_addr_info * ArmAddrInfoNew();
t_bool ArmIsIndirectCall(t_arm_ins * ins);
t_bool ArmIsBranchImmediate(t_arm_ins * ins);
t_bool ArmInsIsUnconditionalBranch(t_arm_ins * ins);
t_bool ArmInsIsSystemInstruction (t_arm_ins *ins);
void ArmInsCleanup(t_arm_ins * ins);
void ArmInsDupDynamic(t_arm_ins * target, t_arm_ins * source);
EXEC_CONDITION ArmInsCondition(t_arm_ins * ins);
void ArmInsConvertToBranch(t_arm_ins * ins);
t_arm_ins * ArmAddJumpInstruction(t_bbl * bbl);
t_bool ArmInsIsCall(t_arm_ins * ins);
void ArmComputeLiveRegsBeforeSwi(t_regset *, t_arm_ins * );
t_bool ArmInsAssembleFromString(t_arm_ins *, t_string);
t_arm_ins_itcond ArmInsExtractITCondition(t_arm_ins *);
t_uint32 ArmInsITBlockSize(t_arm_ins *);
t_arm_ins * ArmInsFindOwningIT(t_arm_ins *, t_bool * is_last_ins_in_block, t_uint32 * ins_idx);
t_arm_condition_code ArmInsThumbConditionCode(t_arm_ins *, t_bool *, t_arm_ins **);
void ArmInsSetITSpecifics(t_arm_ins *);
t_bool ArmInsThumbITDoesNotSetFlags(t_arm_ins *, t_bool);
t_bool ArmInsIsInITBlock(t_arm_ins *);
t_bool ArmInsIsValidThumbConditionCode(t_arm_ins *ins, t_bool * is_last_ins_in_block, t_arm_ins ** owning_it);
t_bool ArmIsStubFromThumb(t_bbl *bbl);

void InstructionSetRegsQuadDouble(t_arm_ins * ins, t_bool is_quad);
void ArmInsertThumbITInstructions(t_cfg * cfg);

t_bool ArmStatusFlagsLiveBefore(t_arm_ins * ins);
void ArmInsLoadStoreMultipleToSingle(t_arm_ins * ins);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
