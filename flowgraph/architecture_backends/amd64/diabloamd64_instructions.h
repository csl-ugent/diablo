#ifndef DIABLOAMD64_INSTRUCTION_TYPEDEFS
#define DIABLOAMD64_INSTRUCTION_TYPEDEFS
typedef struct _t_amd64_operand t_amd64_operand;
typedef enum {
  amd64_optype_none = 0,    /* no operand */
  amd64_optype_reg,         /* register only */
  amd64_optype_imm,         /* immediate only */
  amd64_optype_mem,         /* scale-index-base-displacement */
  amd64_optype_farptr,	   /* 48-bit memory pointer (16-bit segment selector + 32-bit offset) */
  amd64_optype_invalid	 
} t_amd64_optype;

typedef enum {
  amd64_regmode_full64 = 0, /* the full register, e.g. %rax */
  amd64_regmode_lo32,       /* low 32 bits, e.g. %eax */
  amd64_regmode_lo16,       /* low 16 bits, e.g. %ax */
  amd64_regmode_lo8,        /* low 8 bits, e.g. %al */
  amd64_regmode_hi8,        /* bits 8-15, e.g. %ah */
  amd64_regmode_invalid
} t_amd64_regmode;

typedef enum _t_amd64_condition_code
{
  AMD64_CONDITION_O  = 0,
  AMD64_CONDITION_NO = 1,
  AMD64_CONDITION_B  = 2,
  AMD64_CONDITION_AE = 3,
  AMD64_CONDITION_Z  = 4,
  AMD64_CONDITION_NZ = 5,
  AMD64_CONDITION_BE = 6,
  AMD64_CONDITION_A  = 7,
  AMD64_CONDITION_S  = 8,
  AMD64_CONDITION_NS = 9,
  AMD64_CONDITION_P  = 10,
  AMD64_CONDITION_NP = 11,
  AMD64_CONDITION_L  = 12,
  AMD64_CONDITION_GE = 13,
  AMD64_CONDITION_LE = 14,
  AMD64_CONDITION_G  = 15,
  AMD64_CONDITION_LOOP = 251,    /* rcx != 0 */
  AMD64_CONDITION_LOOPZ = 252,   /* rcx != 0 &&  ZF */
  AMD64_CONDITION_LOOPNZ = 253,  /* rcx != 0 && !ZF */
  AMD64_CONDITION_RCXZ = 254,    /* rcx == 0 */
  AMD64_CONDITION_NONE = 255
} t_amd64_condition_code;

#endif

#ifndef DIABLOAMD64_INSTRUCTION_DEFINES
#define DIABLOAMD64_INSTRUCTION_DEFINES

#define AMD64_OP_TYPE(op)         (op)->type
#define AMD64_OP_BASE(op)         (op)->base
#define AMD64_OP_INDEX(op)        (op)->index
#define AMD64_OP_IMMEDIATE(op)    (op)->immediate
#define AMD64_OP_SEGSELECTOR(op)  (op)->segment_selector
#define AMD64_OP_SCALE(op)        (op)->scale
#define AMD64_OP_REGMODE(op)      (op)->regmode
#define AMD64_OP_IMMEDSIZE(op)    (op)->immed_size
#define AMD64_OP_MEMOPSIZE(op)    (op)->memop_size
#define AMD64_OP_FLAGS(op)        (op)->flags

/** instruction operand flags */
#define AMD64_OPFLAG_ISRELOCATED       (1 << 0)

/** register scale possibilities for sib operands */
#define AMD64_SCALE_1         0
#define AMD64_SCALE_2         1
#define AMD64_SCALE_4         2
#define AMD64_SCALE_8         3
#define AMD64_SCALE_INVALID   255


#define BBL_FOREACH_AMD64_INS(bbl,ins) for(ins=T_AMD64_INS(BBL_INS_FIRST(bbl)); ins!=NULL; ins=AMD64_INS_INEXT(ins))
#define BBL_FOREACH_AMD64_INS_SAFE(bbl,ins,tmp)   for(ins=T_AMD64_INS(BBL_INS_FIRST(bbl)), tmp=ins?AMD64_INS_INEXT(ins):0; ins!=NULL; ins=tmp, tmp=ins?AMD64_INS_INEXT(ins):0)
#define SECTION_FOREACH_AMD64_INS(code,ins) for(ins=T_AMD64_INS(SECTION_DATA(code)); ins!=NULL; ins=AMD64_INS_INEXT(ins))


/* operand iterator */
#define AMD64_INS_FOREACH_OP(ins,op)	\
	for (op = AMD64_INS_DEST(ins); op; op = (op == AMD64_INS_SOURCE2(ins) ? NULL : (op == AMD64_INS_SOURCE1(ins) ? AMD64_INS_SOURCE2(ins) : AMD64_INS_SOURCE1(ins) )))

#define T_AMD64_INS(ins)		((t_amd64_ins *)(ins))

/* shortcuts */
#define AMD64_INS_HAS_FLAG(ins,flag)	(AMD64_INS_FLAGS(ins) & (flag))
#define AMD64_INS_HAS_PREFIX(ins,pref)   (AMD64_INS_PREFIXES(ins) & (pref))
#define AMD64_ADSZPREF(ins)	 	AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_ADDRSIZE_OVERRIDE)
#define AMD64_OPSZPREF(ins)	 	AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_OPSIZE_OVERRIDE)
#define AMD64_INS_SET_FLAG(ins, flag)	(AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | (flag)))
#define AMD64_INS_FLIP_FLAG(ins, flag)	(AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) ^ (flag));)

/** possible instruction prefixes */
#define AMD64_PREFIX_LOCK	(1 << 0)
#define AMD64_PREFIX_REPNZ	(1 << 1)
#define AMD64_PREFIX_REP		(1 << 2)
#define AMD64_PREFIX_REPZ	AMD64_PREFIX_REP
#define AMD64_PREFIX_REPE	AMD64_PREFIX_REP
#define AMD64_PREFIX_REPNE	AMD64_PREFIX_REPNZ

#define AMD64_PREFIX_CS_OVERRIDE	(1 << 3)
#define AMD64_PREFIX_DS_OVERRIDE	(1 << 4)
#define AMD64_PREFIX_ES_OVERRIDE	(1 << 5)
#define AMD64_PREFIX_FS_OVERRIDE	(1 << 6)
#define AMD64_PREFIX_GS_OVERRIDE	(1 << 7)
#define AMD64_PREFIX_SS_OVERRIDE	(1 << 8)

#define AMD64_PREFIX_OPSIZE_OVERRIDE	(1 << 9)
#define AMD64_PREFIX_ADDRSIZE_OVERRIDE	(1 << 10)

#define AMD64_PREFIX_SD_OPERANDS         (1 << 1)
#define AMD64_PREFIX_SS_OPERANDS         (1 << 2)
#define AMD64_PREFIX_PS_OPERANDS         (1 << 9)

/** instruction flags */
#define AMD64_IF_DEST_IS_SOURCE		(1 << 0)
#define AMD64_IF_SOURCE1_DEF		(1 << 1)
#define AMD64_IF_SOURCE2_DEF		(1 << 2)
#define AMD64_IF_JMP_FORCE_4BYTE		(1 << 3)

/* convenient shortcut when dynamically generating code.
 * e.g. Amd64MakeInsForBbl(Noop,Append,ins,bbl) will append a noop to bbl */
#define Amd64MakeInsForBbl(type,where,ins,bbl, ...)    \
do {                                                  \
    ins = T_AMD64_INS(InsNewForBbl(bbl));              \
    Amd64InstructionMake ## type(ins, ## __VA_ARGS__); \
    Ins ## where ## ToBbl(T_INS(ins),bbl);            \
    DiabloBrokerCall("SmcInitInstruction",ins);       \
} while (0)


#define Amd64MakeInsForIns(type,where,ins,existing, ...)     \
do {                                                        \
    ins = T_AMD64_INS(InsNewForBbl(AMD64_INS_BBL(existing))); \
    Amd64InstructionMake ## type(ins, ## __VA_ARGS__);       \
    InsInsert ## where (T_INS(ins),T_INS(existing));        \
    DiabloBrokerCall("SmcInitInstruction",ins);             \
} while (0)


#endif

#include <diabloamd64.h>
#ifdef DIABLOAMD64_TYPES
#ifndef DIABLOAMD64_INSTRUCTION_TYPES
#define DIABLOAMD64_INSTRUCTION_TYPES

	
struct _t_amd64_operand {
  /** defines the type of operand (not all types are possible for all instructions) */
  t_amd64_optype type:8;
  /** operand flags */
  t_uint8 flags;
  /** scale for sib type */
  t_uint8 scale;
  /** register mode (8, 16 or 32 bits) */
  t_amd64_regmode regmode:8;
  /** base register for sib or plain_reg types */
  t_reg base;
  /** index register for sib type */
  t_reg index;
  /** immediate for plain_immed, displacement for sib and 32-bit offset for far_ptr */
  t_uint64 immediate;
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


#ifdef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_INSTRUCTION_FUNCTIONS
#define DIABLOAMD64_INSTRUCTION_FUNCTIONS
/** function prototypes */
t_bool Amd64InsIsConditional(t_amd64_ins * ins);
t_bool Amd64InsHasSideEffect(t_amd64_ins * ins);
t_bool Amd64InsIsStore(t_amd64_ins * ins);
t_bool Amd64InsIsLoad(t_amd64_ins * ins);
t_bool Amd64InsIsControlTransfer(t_amd64_ins * ins);
t_bool Amd64InsIsSystemControlTransfer(t_amd64_ins * ins);
t_bool Amd64InsIsRegularControlTransfer(t_amd64_ins * ins);
t_bool Amd64InsIsSystemInstruction(t_amd64_ins * ins);
t_bool Amd64InsIsIndirectCall(t_amd64_ins * ins);
t_bool Amd64InsIsUnconditionalBranch(t_amd64_ins * ins);
t_tristate Amd64IsSyscallExit(t_amd64_ins * ins);
t_uint32 Amd64GetSyscallNo(t_amd64_ins * ins);

void Amd64InsSetOperandFlags(t_amd64_ins * ins);

void Amd64InstructionMakeNoop(t_amd64_ins * ins);
void Amd64InstructionMakeCall(t_amd64_ins * ins);
void Amd64InstructionMakeJump(t_amd64_ins * ins);
void Amd64InstructionMakeJumpReg(t_amd64_ins * ins, t_reg src);
void Amd64InstructionMakeCondJump(t_amd64_ins * ins, t_amd64_condition_code condition);
void Amd64InstructionMakeCondMov(t_amd64_ins * ins, t_reg src, t_reg dst, t_amd64_condition_code condition);
void Amd64InstructionMakeJumpMem(t_amd64_ins * ins, t_reg base, t_reg index);
void Amd64InstructionMakePush(t_amd64_ins * ins, t_reg reg, t_uint64 immval);
void Amd64InstructionMakePopA(t_amd64_ins * ins);
void Amd64InstructionMakePushA(t_amd64_ins * ins);
void Amd64InstructionMakePushMem(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakePop(t_amd64_ins * ins, t_reg reg);
void Amd64InstructionMakeArithmetic(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_reg src, t_uint64 imm);
void Amd64InstructionMakeArithmetic32(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_reg src, t_uint64 imm);
void Amd64InstructionMakeArithmeticToMem(t_amd64_ins * ins, t_amd64_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm);
void Amd64InstructionMakeArithmeticFromMem(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakeMovToMem(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm);
void Amd64InstructionMakeMovToMemLen(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm, t_uint64 imm_len);
void Amd64InstructionMakeMovToMem8bits(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm);
void Amd64InstructionMakeMovFromMem(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakeRepMovSB(t_amd64_ins * ins);
void Amd64InstructionMakeRepMovSD(t_amd64_ins * ins);
void Amd64InstructionMakeMovFromMemLen(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint64 imm_len);
void Amd64InstructionMakeMovFromMem8bits(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakeMovToReg(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm);
void Amd64InstructionMakeRM32IMM32(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm, t_amd64_opcode opcode);
void Amd64InstructionMakeRM32IMM8(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm, t_amd64_opcode opcode);
void Amd64InstructionMakeLeave(t_amd64_ins * ins);
void Amd64InstructionMakeLea(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakeCmp(t_amd64_ins * ins, t_reg reg, t_reg cmpreg, t_uint64 cmpimm);
void Amd64InstructionMakeSetcc(t_amd64_ins * ins, t_amd64_condition_code cond, t_uint32 offset, t_reg base, t_reg index, int scale, t_bool memop);
void Amd64InstructionMakePseudoCall(t_amd64_ins * ins, t_function * to);
void Amd64InstructionMakePseudoLoad(t_amd64_ins * ins, t_reg reg);
void Amd64InstructionMakePseudoSave(t_amd64_ins * ins, t_reg reg);
void Amd64InstructionMakeSimple(t_amd64_ins * ins, t_amd64_opcode opc);
void Amd64InstructionMakeLSahf(t_amd64_ins * ins, t_amd64_opcode opc);
void Amd64InstructionMakeFSaveRstor(t_amd64_ins * ins, t_amd64_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale);
void Amd64InstructionMakeIncDec(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest);
void Amd64InstructionMakeIncDeci32(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest);
void Amd64InstructionMakeTest(t_amd64_ins * ins, t_reg src1, t_reg src2, t_uint64 imm);
void Amd64InstructionMakeTest32(t_amd64_ins * ins, t_reg src1, t_reg src2, t_uint64 imm);
void Amd64InstructionMakeDirect(t_amd64_ins * ins);
void Amd64InstructionMakeReturn(t_amd64_ins * ins);
void Amd64InstructionMakeIncDec32(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest);
void Amd64InsDupDynamic(t_amd64_ins * target, t_amd64_ins * source);

void Amd64InsSetGenericInsInfo(t_amd64_ins * ins);
t_regset Amd64InsUsedRegisters(t_amd64_ins * ins);
t_regset Amd64InsDefinedRegisters(t_amd64_ins * ins);
t_address Amd64InsGetSize(t_amd64_ins * ins);
t_amd64_operand * Amd64InsGetFirstRelocatedOp(t_amd64_ins * ins);
t_amd64_operand * Amd64InsGetSecondRelocatedOp(t_amd64_ins * ins);
t_amd64_operand * Amd64InsGetMemLoadOp(t_amd64_ins * ins);
t_amd64_operand * Amd64InsGetMemStoreOp(t_amd64_ins * ins);
t_reloc * Amd64GetRelocForOp(t_amd64_ins * ins, t_amd64_operand * op);

void Amd64OpSetReg(t_amd64_operand * op, t_reg reg, t_amd64_regmode mode);
void Amd64OpSetImm(t_amd64_operand * op, t_uint64 imm, t_uint32 immedsize);
void Amd64OpSetMem(t_amd64_operand * op, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize);

void Amd64InsCleanup(t_amd64_ins * ins);
t_bool Amd64InsIsProcedureCall(t_amd64_ins * ins);
t_uint32 Amd64BblFingerprint(t_bbl * bbl);
t_bool Amd64InsAreIdentical(t_amd64_ins * ins1, t_amd64_ins * ins2);
t_bool Amd64InstructionUnconditionalizer(t_amd64_ins * ins);
#endif

typedef struct _t_amd64_opposite t_amd64_opposite;

t_bool Amd64InvertConditionExistBbl(t_bbl * i_bbl);
t_bool Amd64InvertConditionBbl(t_bbl * i_bbl);
t_bool Amd64InvertConditionExist(t_amd64_condition_code test_cond);
t_amd64_condition_code Amd64InvertCondition(t_amd64_condition_code condition);
t_bool Amd64InvertBranchBbl(t_bbl * bbl);
  
struct _t_amd64_opposite
{
  t_uint32 cond0;
  t_uint32 cond1;
};

#endif
