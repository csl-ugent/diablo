/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOI386_INSTRUCTION_TYPEDEFS
#define DIABLOI386_INSTRUCTION_TYPEDEFS
typedef struct _t_i386_operand t_i386_operand;
typedef enum {
  i386_optype_none = 0,    /* no operand */
  i386_optype_reg,         /* register only */
  i386_optype_imm,         /* immediate only */
  i386_optype_mem,         /* scale-index-base-displacement */
  i386_optype_farptr,	   /* 48-bit memory pointer (16-bit segment selector + 32-bit offset) */
  i386_optype_invalid	 
} t_i386_optype;

typedef enum {
  i386_regmode_full32 = 0, /* the full register, e.g. %eax */
  i386_regmode_lo16,       /* low 16 bits, e.g. %ax */
  i386_regmode_lo8,        /* low 8 bits, e.g. %al */
  i386_regmode_hi8,         /* bits 8-15, e.g. %ah */
  i386_regmode_invalid
} t_i386_regmode;

typedef enum _t_i386_condition_code
{
  I386_CONDITION_O  = 0,
  I386_CONDITION_NO = 1,
  I386_CONDITION_B  = 2,
  I386_CONDITION_AE = 3,
  I386_CONDITION_Z  = 4,
  I386_CONDITION_NZ = 5,
  I386_CONDITION_BE = 6,
  I386_CONDITION_A  = 7,
  I386_CONDITION_S  = 8,
  I386_CONDITION_NS = 9,
  I386_CONDITION_P  = 10,
  I386_CONDITION_NP = 11,
  I386_CONDITION_L  = 12,
  I386_CONDITION_GE = 13,
  I386_CONDITION_LE = 14,
  I386_CONDITION_G  = 15,
  I386_CONDITION_LOOP = 251,    /* ecx != 0 */
  I386_CONDITION_LOOPZ = 252,   /* ecx != 0 &&  ZF */
  I386_CONDITION_LOOPNZ = 253,  /* ecx != 0 && !ZF */
  I386_CONDITION_ECXZ = 254,    /* ecx == 0 */
  I386_CONDITION_NONE = 255
} t_i386_condition_code;

#endif

#ifndef DIABLOI386_INSTRUCTION_DEFINES
#define DIABLOI386_INSTRUCTION_DEFINES

#define I386_OP_TYPE(op)         (op)->type
#define I386_OP_BASE(op)         (op)->base
#define I386_OP_INDEX(op)        (op)->index
#define I386_OP_IMMEDIATE(op)    (op)->immediate
#define I386_OP_SEGSELECTOR(op)  (op)->segment_selector
#define I386_OP_SCALE(op)        (op)->scale
#define I386_OP_REGMODE(op)      (op)->regmode
#define I386_OP_IMMEDSIZE(op)    (op)->immed_size
#define I386_OP_MEMOPSIZE(op)    (op)->memop_size
#define I386_OP_FLAGS(op)        (op)->flags

/** instruction operand flags */
#define I386_OPFLAG_ISRELOCATED       (1 << 0)

/** register scale possibilities for sib operands */
#define I386_SCALE_1         0
#define I386_SCALE_2         1
#define I386_SCALE_4         2
#define I386_SCALE_8         3
#define I386_SCALE_INVALID   255


#define BBL_FOREACH_I386_INS(bbl,ins) for(ins=T_I386_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=I386_INS_INEXT(ins))
#define BBL_FOREACH_I386_INS_R(bbl,ins) for(ins=T_I386_INS(BBL_INS_LAST(bbl)); ins!=NULL; ins=I386_INS_IPREV(ins))
#define BBL_FOREACH_I386_INS_SAFE(bbl,ins,tmp)   for(ins=T_I386_INS(BBL_INS_FIRST(bbl)), tmp=ins?I386_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?I386_INS_INEXT(ins):0)
#define SECTION_FOREACH_I386_INS(code,ins) for(ins=T_I386_INS(SECTION_DATA(code)); ins!=NULL; ins=I386_INS_INEXT(ins))


/* operand iterator */
#define I386_INS_FOREACH_OP(ins,op)	\
	for (op = I386_INS_DEST(ins); op; op = (op == I386_INS_SOURCE2(ins) ? NULL : (op == I386_INS_SOURCE1(ins) ? I386_INS_SOURCE2(ins) : I386_INS_SOURCE1(ins) )))

#define T_I386_INS(ins)		((t_i386_ins *)(ins))

/** field selectors */
//#define I386_INS_OPCODE(ins)		(T_I386_INS(ins)->opcode)
//#define I386_INS_DATA(ins)		(T_I386_INS(ins)->data)
//#define I386_INS_PREFIXES(ins)		(T_I386_INS(ins)->prefixes)
//#define I386_INS_FLAGS(ins)		(T_I386_INS(ins)->flags)
//#define I386_INS_DEST(ins)		(&(T_I386_INS(ins)->ops[0]))
//#define I386_INS_SOURCE1(ins)		(&(T_I386_INS(ins)->ops[1]))
//#define I386_INS_SOURCE2(ins)		(&(T_I386_INS(ins)->ops[2]))
//#define I386_INS_CONDITION(ins)		(T_I386_INS(ins)->condition)

/* shortcuts */
#define I386_INS_HAS_FLAG(ins,flag)	(I386_INS_FLAGS(ins) & (flag))
#define I386_INS_HAS_PREFIX(ins,pref)   (I386_INS_PREFIXES(ins) & (pref))
#define I386_ADSZPREF(ins)	 	I386_INS_HAS_PREFIX(ins,I386_PREFIX_ADDRSIZE_OVERRIDE)
#define I386_OPSZPREF(ins)	 	I386_INS_HAS_PREFIX(ins,I386_PREFIX_OPSIZE_OVERRIDE)
#define I386_INS_SET_FLAG(ins, flag)	(I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | (flag)))
#define I386_INS_FLIP_FLAG(ins, flag)	(I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) ^ (flag));)

/** possible instruction prefixes */
#define I386_PREFIX_LOCK	(1 << 0)
#define I386_PREFIX_REPNZ	(1 << 1)
#define I386_PREFIX_REP		(1 << 2)
#define I386_PREFIX_REPZ	I386_PREFIX_REP
#define I386_PREFIX_REPE	I386_PREFIX_REP
#define I386_PREFIX_REPNE	I386_PREFIX_REPNZ

#define I386_PREFIX_CS_OVERRIDE	(1 << 3)
#define I386_PREFIX_DS_OVERRIDE	(1 << 4)
#define I386_PREFIX_ES_OVERRIDE	(1 << 5)
#define I386_PREFIX_FS_OVERRIDE	(1 << 6)
#define I386_PREFIX_GS_OVERRIDE	(1 << 7)
#define I386_PREFIX_SS_OVERRIDE	(1 << 8)

#define I386_PREFIX_OPSIZE_OVERRIDE	(1 << 9)
#define I386_PREFIX_ADDRSIZE_OVERRIDE	(1 << 10)

#define I386_PREFIX_SD_OPERANDS         (1 << 1)
#define I386_PREFIX_SS_OPERANDS         (1 << 2)
#define I386_PREFIX_PS_OPERANDS         (1 << 9)

/** instruction flags */
#define I386_IF_DEST_IS_SOURCE		(1 << 0)
#define I386_IF_SOURCE1_DEF		(1 << 1)
#define I386_IF_SOURCE2_DEF		(1 << 2)
#define I386_IF_JMP_FORCE_4BYTE		(1 << 3)

/* convenient shortcut when dynamically generating code.
 * e.g. I386MakeInsForBbl(Noop,Append,ins,bbl) will append a noop to bbl */
#define I386MakeInsForBbl(type,where,ins,bbl, ...)    \
do {                                                  \
    ins = T_I386_INS(InsNewForBbl(bbl));              \
    I386InstructionMake ## type(ins, ## __VA_ARGS__); \
    Ins ## where ## ToBbl(T_INS(ins),bbl);            \
} while (0)


#define I386MakeInsForIns(type,where,ins,existing, ...)     \
do {                                                        \
    ins = T_I386_INS(InsNewForBbl(I386_INS_BBL(existing))); \
    I386InstructionMake ## type(ins, ## __VA_ARGS__);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
} while (0)


#endif

#include <diabloi386.h>
#ifdef DIABLOI386_TYPES
#ifndef DIABLOI386_INSTRUCTION_TYPES
#define DIABLOI386_INSTRUCTION_TYPES

	
struct _t_i386_operand {
  /** defines the type of operand (not all types are possible for all instructions) */
  t_i386_optype type:8;
  /** operand flags */
  t_uint8 flags;
  /** scale for sib type */
  t_uint8 scale;
  /** register mode (8, 16 or 32 bits) */
  t_i386_regmode regmode:8;
  /** base register for sib or plain_reg types */
  t_reg base;
  /** index register for sib type */
  t_reg index;
  /** immediate for plain_immed, displacement for sib and 32-bit offset for far_ptr */
  t_uint32 immediate;
  /** segment selector for far_ptr */
  t_uint16 segment_selector;
  /** memory operand size */
  t_uint16 memop_size;
  /** size in bytes of the immediate field */
  t_uint8 immed_size;
  /*superoptimizer*/
  t_uint8 super;
};
#endif
#endif


#ifdef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_INSTRUCTION_FUNCTIONS
#define DIABLOI386_INSTRUCTION_FUNCTIONS
/** function prototypes */
t_bool I386InsIsConditional(t_i386_ins * ins);
t_bool I386InsHasSideEffect(t_i386_ins * ins);
t_bool I386InsIsStore(t_i386_ins * ins);
t_bool I386InsIsLoad(t_i386_ins * ins);
t_bool I386InsIsControlTransfer(t_i386_ins * ins);
t_bool I386InsIsSystemControlTransfer(t_i386_ins * ins);
t_bool I386InsIsRegularControlTransfer(t_i386_ins * ins);
t_bool I386InsIsSystemInstruction(t_i386_ins * ins);
t_bool I386InsIsIndirectCall(t_i386_ins * ins);
t_bool I386InsIsUnconditionalBranch(t_i386_ins * ins);
t_tristate I386IsSyscallExit(t_i386_ins * ins);
t_uint32 I386GetSyscallNo(t_i386_ins * ins);

void I386InsSetOperandFlags(t_i386_ins * ins);

void I386InstructionMakeNoop(t_i386_ins * ins);
void I386InstructionMakeCall(t_i386_ins * ins);
void I386InstructionMakeJump(t_i386_ins * ins);
void I386InstructionMakeJumpReg(t_i386_ins * ins, t_reg src);
void I386InstructionMakeCondJump(t_i386_ins * ins, t_i386_condition_code condition);
void I386InstructionMakeCondMov(t_i386_ins * ins, t_reg src, t_reg dst, t_i386_condition_code condition);
void I386InstructionMakeJumpMem(t_i386_ins * ins, t_reg base, t_reg index);
void I386InstructionMakePush(t_i386_ins * ins, t_reg reg, t_uint32 immval);
void I386InstructionMakePopA(t_i386_ins * ins);
void I386InstructionMakePushA(t_i386_ins * ins);
void I386InstructionMakePushMem(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakePop(t_i386_ins * ins, t_reg reg);
void I386InstructionMakeIDiv(t_i386_ins * ins,  t_reg dest, t_reg src);
void I386InstructionMakeCDQ(t_i386_ins * ins,  t_reg dest, t_reg src);
void I386InstructionMakePushF(t_i386_ins * ins);
void I386InstructionMakePopF(t_i386_ins * ins);
void I386InstructionMakeArithmetic(t_i386_ins * ins, t_i386_opcode opc, t_reg dest, t_reg src, t_uint32 imm);
void I386InstructionMakeArithmeticToMem(t_i386_ins * ins, t_i386_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm);
void I386InstructionMakeArithmeticFromMem(t_i386_ins * ins, t_i386_opcode opc, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakeMovToMem(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm);
void I386InstructionMakeMovToMemLen(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm, t_uint32 imm_len);
void I386InstructionMakeMovToMem8bits(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm);
void I386InstructionMakeMovFromMem(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakeRepMovSB(t_i386_ins * ins);
void I386InstructionMakeRepMovSD(t_i386_ins * ins);
void I386InstructionMakeMovFromMemLen(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 imm_len);
void I386InstructionMakeMovFromMem8bits(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakeMovToReg(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm);
void I386InstructionMakeRM32IMM32(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm, t_i386_opcode opcode);
void I386InstructionMakeRM32IMM8(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm, t_i386_opcode opcode);
void I386InstructionMakeLeave(t_i386_ins * ins);
void I386InstructionMakeLea(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakeCmp(t_i386_ins * ins, t_reg reg, t_reg cmpreg, t_uint32 cmpimm);
void I386InstructionMakeSetcc(t_i386_ins * ins, t_i386_condition_code cond, t_uint32 offset, t_reg base, t_reg index, int scale, t_bool memop);
void I386InstructionMakePseudoCall(t_i386_ins * ins, t_function * to);
void I386InstructionMakePseudoLoad(t_i386_ins * ins, t_reg reg);
void I386InstructionMakePseudoSave(t_i386_ins * ins, t_reg reg);
void I386InstructionMakeSimple(t_i386_ins * ins, t_i386_opcode opc);
void I386InstructionMakeLSahf(t_i386_ins * ins, t_i386_opcode opc);
void I386InstructionMakeFSaveRstor(t_i386_ins * ins, t_i386_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale);
void I386InstructionMakeIncDec(t_i386_ins * ins, t_i386_opcode opc, t_reg dest);
void I386InstructionMakeTest(t_i386_ins * ins, t_reg src1, t_reg src2, t_uint32 imm);
void I386InstructionMakeDirect(t_i386_ins * ins);
void I386InstructionMakeReturn(t_i386_ins * ins);
void I386InstructionMakeData(t_i386_ins * ins);
void I386InstructionMakeBt(t_i386_ins * ins, t_reg src1, t_uint8 imm);
void I386InsDupDynamic(t_i386_ins * target, t_i386_ins * source);
void I386SetGenericInsInfo(t_i386_ins * ins);

t_regset I386InsUsedRegisters(t_i386_ins * ins);
t_regset I386InsDefinedRegisters(t_i386_ins * ins);
t_address I386InsGetSize(t_i386_ins * ins);
t_i386_operand * I386InsGetFirstRelocatedOp(t_i386_ins * ins);
t_i386_operand * I386InsGetSecondRelocatedOp(t_i386_ins * ins);
t_i386_operand * I386InsGetMemLoadOp(t_i386_ins * ins);
t_i386_operand * I386InsGetMemStoreOp(t_i386_ins * ins);
t_reloc * I386GetRelocForOp(t_i386_ins * ins, t_i386_operand * op);

void I386OpSetReg(t_i386_operand * op, t_reg reg, t_i386_regmode mode);
void I386OpSetImm(t_i386_operand * op, t_uint32 imm, t_uint32 immedsize);
void I386OpSetMem(t_i386_operand * op, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize);

void I386InsCleanup(t_i386_ins * ins);
t_bool I386InsIsProcedureCall(t_i386_ins * ins);
t_uint32 I386BblFingerprint(t_bbl * bbl);
t_bool I386InsAreIdentical(t_i386_ins * ins1, t_i386_ins * ins2);
t_bool I386InstructionUnconditionalizer(t_i386_ins * ins);
#endif

typedef struct _t_i386_opposite t_i386_opposite;

t_bool I386InvertConditionExistBbl(t_bbl * i_bbl);
t_bool I386InvertConditionBbl(t_bbl * i_bbl);
t_bool I386InvertConditionExist(t_i386_condition_code test_cond);
t_i386_condition_code I386InvertCondition(t_i386_condition_code condition);
t_bool I386InvertBranchBbl(t_bbl * bbl);
t_uint32 I386InvertBranchAllBblInFunction(t_function * fun);
void I386ClearIns(t_i386_ins * ins);
  
struct _t_i386_opposite
{
  t_uint32 cond0;
  t_uint32 cond1;
};

#endif
