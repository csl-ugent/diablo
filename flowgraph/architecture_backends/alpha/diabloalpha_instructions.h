#include <diabloalpha.h>

#ifndef DIABLOALPHA_INSTRUCTION_DEFINES
#define DIABLOALPHA_INSTRUCTION_DEFINES

#define T_ALPHA_INS(alpha_ins) ((t_alpha_ins*) alpha_ins)
#define SECTION_FOREACH_ALPHA_INS(code,ins) for(ins=T_ALPHA_INS(SECTION_DATA(code)); ins!=NULL; ins=ALPHA_INS_INEXT(ins))

#define IS_NOOP(ins) \
  (IS_FNOP(ins) || _IS_NOP(ins) || IS_UNOP(ins)) 

#define IS_FNOP(ins) \
  (ALPHA_INS_OPCODE(ins) == ALPHA_CPYS && \
  ALPHA_INS_REGA(ins) == ALPHA_REG_FZERO && \
  ALPHA_INS_REGB(ins) == ALPHA_REG_FZERO && \
  ALPHA_INS_REGD(ins) == ALPHA_REG_FZERO)

#define _IS_NOP(ins) \
  (ALPHA_INS_OPCODE(ins) == ALPHA_BIS && ALPHA_INS_REGA(ins) == ALPHA_REG_ZERO && \
	  ALPHA_INS_REGB(ins) == ALPHA_REG_ZERO && ALPHA_INS_REGD(ins) == ALPHA_REG_ZERO)

#define IS_UNOP(ins) \
  (ALPHA_INS_OPCODE(ins) == ALPHA_LDQ_U && \
  ALPHA_INS_REGD(ins) == ALPHA_REG_ZERO && \
  ALPHA_INS_IMMEDIATE(ins) == 0)

#define IS_CLR(opc, instr) \
  (opc == ALPHA_BIS && ALPHA_GET_REGA(instr) == 31 && \
  ALPHA_GET_REGB(instr) == 31)

#define IS_FCLR(opc, instr) \
  (opc == ALPHA_CPYS && ALPHA_GET_REGA(instr) == 61 && \
  ALPHA_GET_REGB(instr) == 61)


#endif

/* Following defines taken from SimpleScalar. */

#ifndef ALPHA_INSTRUCTION_FLAGS
#define ALPHA_INSTRUCTION_FLAGS

/* instruction flags */
#define F_ICOMP    0x00000001  /* integer computation */
#define F_FCOMP    0x00000002  /* FP computation */
#define F_CTRL    0x00000004  /* control inst */
#define F_UNCOND  0x00000008  /*   unconditional change */
#define F_COND    0x00000010  /*   conditional change */
#define F_MEM    0x00000020  /* memory access inst */
#define F_LOAD    0x00000040  /*   load inst */
#define F_STORE    0x00000080  /*   store inst */
#define F_DISP    0x00000100  /*   displaced (R+C) addr mode */
#define F_RR    0x00000200  /*   R+R addr mode */
#define F_DIRECT  0x00000400  /*   direct addressing mode */
#define F_TRAP    0x00000800  /* traping inst */
#define F_LONGLAT  0x00001000  /* long latency inst (for sched) */
#define F_DIRJMP  0x00002000  /* direct jump */
#define F_INDIRJMP  0x00004000  /* indirect jump */
#define F_CALL    0x00008000  /* function call */
#define F_FPCOND  0x00010000  /* FP conditional branch */
#define F_IMM    0x00020000  /* instruction has immediate operand */

#endif

#include <diabloalpha.h>

#ifdef DIABLOALPHA_FUNCTIONS
#ifndef DIABLOALPHA_INSTRUCTION_FUNCTIONS
#define DIABLOALPHA_INSTRUCTION_FUNCTIONS

t_regset AlphaInsUsedRegisters(t_alpha_ins *ins);
t_regset AlphaInsDefinedRegisters(t_alpha_ins *ins);

t_bool AlphaInsHasSideEffect(t_ins * ins);
t_bool AlphaInsIsJump(t_alpha_ins * ins);
t_bool AlphaInsIsSystemInstruction(t_ins *);
t_tristate AlphaInsIsSyscallExit(t_ins *);
t_bool AlphaInsIsProcedureCall(t_ins *);
t_bool AlphaInsIsIndirectCall(t_ins *);
t_bool AlphaInsIsControlTransfer(t_ins *);
t_bool AlphaInsIsUnconditionalJump(t_ins *);
t_bool AlphaInsIsConditional(t_alpha_ins * ins);
void AlphaInstructionPrint(t_ins *, t_string str);
void AlphaInsMakeNoop(t_alpha_ins * ins);
t_bool AlphaInsIsNoop(t_alpha_ins * ins);
t_bool AlphaFunIsGlobal(t_function* fun);
t_bool AlphaIsGPRecomputation(t_alpha_ins * ins);
t_bool AlphaInsIsSystemPlug(t_ins * ins);
void AlphaInsMakeBr(t_alpha_ins * ins, t_bbl * target);
void AlphaInsMakeBsr(t_alpha_ins * ins, t_bbl * target);

#endif
#endif
/* vim: set shiftwidth=2: */
