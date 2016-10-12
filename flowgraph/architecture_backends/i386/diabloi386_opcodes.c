/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>

t_i386_opcode_info i386_opcode_table[MAX_I386_OPCODE];

#define OF	(1 << 0)
#define SF	(1 << 1)
#define ZF	(1 << 2)
#define AF	(1 << 3)
#define PF	(1 << 4)
#define CF	(1 << 5)
#define TF	(1 << 6)
#define IF	(1 << 7)
#define DF	(1 << 8)
#define NT	(1 << 9)
#define RF	(1 << 10)
#define C0	(1 << 11)
#define C1	(1 << 12)
#define C2	(1 << 13)
#define C3	(1 << 14)

#define ALL_CONDITIONS  OF | SF | ZF | AF | PF | CF
#define ALL_FPU         C0 | C1 | C2 | C3

#define DI	(1 << 0)
#define S1I	(1 << 1)
#define S2I	(1 << 2)

static t_reg condcodes[15] = {
  I386_CONDREG_OF,
  I386_CONDREG_SF,
  I386_CONDREG_ZF,
  I386_CONDREG_AF,
  I386_CONDREG_PF,
  I386_CONDREG_CF,
  I386_CONDREG_TF,
  I386_CONDREG_IF,
  I386_CONDREG_DF,
  I386_CONDREG_NT,
  I386_CONDREG_RF,
  I386_CONDREG_C0,
  I386_CONDREG_C1,
  I386_CONDREG_C2,
  I386_CONDREG_C3
};

static int __iter;

#define SetOpcodeInfo(opc,text,itype,usedcf,definedcf,pops,pushes,fixed_ops)	\
   i386_opcode_table[(opc)].textual = text; \
   i386_opcode_table[(opc)].type = itype; \
   i386_opcode_table[(opc)].fixedops = fixed_ops; \
   i386_opcode_table[(opc)].fpstack_pops = pops; \
   i386_opcode_table[(opc)].fpstack_pushes = pushes; \
   i386_opcode_table[(opc)].cf_used = NullRegs; \
   i386_opcode_table[(opc)].cf_defined = NullRegs; \
   for (__iter = 0; __iter < 15; __iter++) \
     if ((usedcf) & (1 << __iter)) \
        RegsetSetAddReg(i386_opcode_table[(opc)].cf_used, condcodes[__iter]); \
   for (__iter = 0; __iter < 15; __iter++) \
     if ((definedcf) & (1 << __iter)) \
        RegsetSetAddReg(i386_opcode_table[(opc)].cf_defined, condcodes[__iter])

void I386InitOpcodeTable(void)
{
  /* --- special entries --- */
  SetOpcodeInfo(INVALID_OPC, "(invalid)", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(UNSUPPORTED_OPC, "(unsupported)", IT_UNKNOWN, 0, 0, 0, 0, 0); 
  SetOpcodeInfo(I386_DATA, "data", IT_DATA, 0, 0, 0, 0, 0);

  /* --- general purpose instructions --- */

  /* data transfer instructions */
  SetOpcodeInfo(I386_CMOVcc, "cmovCC", IT_DATAPROC, OF | SF | ZF | PF | CF, 0 , 0, 0, 0);
  SetOpcodeInfo(I386_MOV, "mov", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_BSWAP, "bswap", IT_SWAP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_CMPXCHG, "cmpxchg", IT_SWAP, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_XADD, "xadd", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_XCHG, "xchg", IT_SWAP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_POP, "pop", IT_LOAD, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_POPA, "popa", IT_LOAD, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PUSH, "push", IT_STORE, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PUSHA, "pusha", IT_STORE, 0,0, 0, 0, 0);
  SetOpcodeInfo(I386_IN, "in", IT_LOAD, 0, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_OUT, "out", IT_STORE, 0, 0, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_CDQ, "cdq", IT_DATAPROC, 0, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_CWDE, "cwde", IT_DATAPROC, 0, 0, 0, 0, DI);
  SetOpcodeInfo(I386_MOVSX, "movsbl", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_MOVZX, "movzbl", IT_DATAPROC, 0, 0, 0, 0, 0);

  /* binary arithmetic */
  SetOpcodeInfo(I386_ADC, "adc", IT_DATAPROC, CF, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_ADD, "add", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SUB, "sub", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SBB, "sbb", IT_DATAPROC, CF, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_IMUL, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_IMULexp1, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_IMULexp2, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_MUL, "mul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_IDIV, "idiv", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_DIV, "div", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_NEG, "neg", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_INC, "inc", IT_DATAPROC, 0, OF | SF | ZF | AF | PF, 0, 0, 0);
  SetOpcodeInfo(I386_DEC, "dec", IT_DATAPROC, 0, OF | SF | ZF | AF | PF, 0, 0, 0);
  SetOpcodeInfo(I386_CMP, "cmp", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  
  /* decimal arithmetic */
  SetOpcodeInfo(I386_AAA, "aaa", IT_DATAPROC, AF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(I386_AAD, "aad", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(I386_AAM, "aam", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(I386_AAS, "aas", IT_DATAPROC, AF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(I386_DAA, "daa", IT_DATAPROC, AF | CF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(I386_DAS, "das", IT_DATAPROC, AF | CF, ALL_CONDITIONS, 0, 0, DI);

  /* logical */
  SetOpcodeInfo(I386_NOT, "not", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_AND, "and", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_OR,  "or" , IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_XOR, "xor", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);

  /* shift and rotate */
  SetOpcodeInfo(I386_SAR, "sar", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SHL, "shl", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SHLD, "shld", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SHR, "shr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SHRD, "shrd", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_ROR, "ror", IT_DATAPROC, 0, OF | CF, 0, 0, 0);
  SetOpcodeInfo(I386_ROL, "rol", IT_DATAPROC, 0, OF | CF, 0, 0, 0);
  SetOpcodeInfo(I386_RCR, "rcr", IT_DATAPROC, CF, OF | CF, 0, 0, 0);
  SetOpcodeInfo(I386_RCL, "rcl", IT_DATAPROC, CF, OF | CF, 0, 0, 0);

  /* bit and byte instructions */
  SetOpcodeInfo(I386_BT, "bt", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_BTC, "btc", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_BTR, "btr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_BTS, "bts", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_BSF, "bsf", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_BSR, "bsr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_SETcc, "setCC", IT_DATAPROC, OF | SF | ZF | PF | CF, 0, 0, 0, 0);
  SetOpcodeInfo(I386_TEST, "test", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);

  /* control transfer instructions */
  SetOpcodeInfo(I386_JMP, "jmp", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_JMPF, "jmpf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_Jcc, "jCC", IT_BRANCH, OF | SF | ZF | PF | CF, 0, 0, 0, 0);
  SetOpcodeInfo(I386_JECXZ, "jecxz", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LOOP, "loop", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LOOPNZ, "loopnz", IT_BRANCH, ZF, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LOOPZ, "loopz", IT_BRANCH, ZF, 0, 0, 0, 0);
  SetOpcodeInfo(I386_CALL, "call", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_CALLF, "callf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_RET, "ret", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_RETF, "retf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_IRET, "iret", IT_BRANCH, NT, ALL_CONDITIONS | TF | IF | DF, 0, 0, 0);
  /*SetOpcodeInfo(I386_INT, "int", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  /*SetOpcodeInfo(I386_INT3, "int3", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  /*SetOpcodeInfo(I386_INTO, "into", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  SetOpcodeInfo(I386_INT, "int", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(I386_INT3, "int3", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(I386_INTO, "into", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(I386_BOUND, "bound", IT_SWI, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_ENTER, "enter", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LEAVE, "leave", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* string instructions */
  SetOpcodeInfo(I386_CMPSB, "cmpsb", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_CMPSD, "cmpsd", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_MOVSB, "movsb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_MOVSD, "movsd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_LODSB, "lodsb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_LODSD, "lodsd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_SCASB, "scasb", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_SCASD, "scasd", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_STOSB, "stosb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_STOSD, "stosd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_INSB, "insd" , IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_INSD, "insd" , IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(I386_OUTSB, "outsb", IT_UNKNOWN, DF, 0, 0, 0, S1I|S2I);
  SetOpcodeInfo(I386_OUTSD, "outsb", IT_UNKNOWN, DF, 0, 0, 0, S1I|S2I);

  /* flag control instructions */
  SetOpcodeInfo(I386_CLC, "clc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(I386_CLD, "cld", IT_STATUS, 0, DF, 0, 0, 0);
  SetOpcodeInfo(I386_CLI, "cli", IT_STATUS, 0, IF, 0, 0, 0);
  SetOpcodeInfo(I386_STC, "stc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(I386_STD, "std", IT_STATUS, 0, DF, 0, 0, 0);
  SetOpcodeInfo(I386_STI, "sti", IT_STATUS, 0, IF, 0, 0, 0);
  SetOpcodeInfo(I386_CMC, "cmc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(I386_LAHF, "lahf", IT_STATUS, SF | ZF | AF | PF | CF, 0, 0, 0, DI);
  SetOpcodeInfo(I386_SAHF, "sahf", IT_STATUS, 0, SF | ZF | AF | PF | CF, 0, 0, S1I);
  SetOpcodeInfo(I386_POPF, "popf", IT_STATUS, 0, ALL_CONDITIONS | TF | DF | IF | NT, 0, 0, 0);
  SetOpcodeInfo(I386_PUSHF, "pushf", IT_STATUS, ALL_CONDITIONS | TF | DF | IF | NT, 0, 0, 0, 0);

  /* segment register instructions */
  SetOpcodeInfo(I386_LDS, "lds", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LES, "les", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LGS, "lgs", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LSS, "lss", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LFS, "lfs", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* miscellaneous instructions */
  SetOpcodeInfo(I386_LEA, "lea", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_NOP, "nop", IT_NOP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_UD2, "ud2", IT_SWI, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_CPUID, "cpuid", IT_UNKNOWN, 0, 0, 0, 0, S1I);
  SetOpcodeInfo(I386_XLAT, "xlat", IT_DATAPROC, 0, 0, 0, 0, DI);

  /* --- system instructions --- */
  SetOpcodeInfo(I386_LGDT, "lgdt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LIDT, "lidt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LLDT, "lldt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_SGDT, "sgdt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_SIDT, "sidt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_SLDT, "sldt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LTR, "ltr", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_STR, "str", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LMSW, "lmsw", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_SMSW, "smsw", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_CLTS, "clts", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_ARPL, "arpl", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(I386_LAR, "lar", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(I386_LSL, "lsl", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(I386_VERR, "verr", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(I386_VERW, "verw", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(I386_WBINVD, "wbinvd", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_INVD, "invd", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_INVLPG, "invlpg", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_HLT, "hlt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_RSM, "rsm", IT_UNKNOWN, 0, ALL_CONDITIONS | TF | IF | DF | NT | RF, 0, 0, 0);
  SetOpcodeInfo(I386_RDMSR, "rdmsr", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(I386_WRMSR, "wrmsr", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(I386_RDPMC, "rdpmc", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(I386_RDTSC, "rdtsc", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(I386_SYSENTER, "sysenter", IT_SWI, ALL_CONDITIONS | TF | IF | DF, TF | NT, 0, 0, 0); /* not sure about sysenter and sysexit, just guessing! */
  SetOpcodeInfo(I386_SYSEXIT, "sysexit", IT_UNKNOWN, NT, ALL_CONDITIONS | TF | IF | DF, 0, 0, 0);

  SetOpcodeInfo(I386_PREFETCH_NTA, "prefetch NTA", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PREFETCH_T0, "prefetch T0", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PREFETCH_T1, "prefetch T1", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PREFETCH_T2, "prefetch T2", IT_UNKNOWN, 0, 0, 0, 0, 0);
  
  /* --- floating point instructions --- */

  /* data transfer instructions */
  SetOpcodeInfo(I386_FLD, "fld", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FST, "fst", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSTP, "fstp", IT_UNKNOWN, ALL_FPU, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FILD, "fild", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FIST, "fist", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FISTP, "fistp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FBLD, "fbld", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FBSTP, "fbstp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FXCH, "fxch", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FCMOVcc, "fcmovCC", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* basic arithmetic */
  SetOpcodeInfo(I386_FADD, "fadd", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FADDP, "faddp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FIADD, "fiadd", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSUB, "fsub", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSUBP, "fsubp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FSUBR, "fsubr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSUBRP, "fsubrp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FISUB, "fisub", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FISUBR, "fisubr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FMUL, "fmul", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FMULP, "fmulp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FIMUL, "fimul", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FDIV, "fdiv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FDIVP, "fdivp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FDIVR, "fdivr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FDIVRP, "fdivrp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FIDIV, "fidiv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FIDIVR, "fidivr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FPREM, "fprem", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FPREM1, "fprem1", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FABS, "fabs", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FCHS, "fchs", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FRNDINT, "frndint", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSCALE, "fscale", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSQRT, "fsqrt", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FXTRACT, "fxtract", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);

  /* comparison */
  SetOpcodeInfo(I386_FCOM, "fcom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FCOMI, "fcomi", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_FCOMP, "fcomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FCOMIP, "fcomip", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 1, 0, 0);
  SetOpcodeInfo(I386_FCOMPP, "fcompp", IT_UNKNOWN, 0, ALL_FPU, 2, 0, 0);
  SetOpcodeInfo(I386_FUCOM, "fucom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FUCOMI, "fucomi", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(I386_FUCOMP, "fucomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FUCOMIP, "fucomip", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 1, 0, 0);
  SetOpcodeInfo(I386_FUCOMPP, "fucompp", IT_UNKNOWN, 0, ALL_FPU, 2, 0, 0);
  SetOpcodeInfo(I386_FICOM, "ficom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FICOMP, "ficomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FTST, "ftst", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FXAM, "fxam", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* transcendental */
  SetOpcodeInfo(I386_FCOS, "fcos", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSIN, "fsin", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSINCOS, "fsincos", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FPTAN, "fptan", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FPATAN, "fpatan", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_F2XM1, "f2xm1", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FYL2X, "fyl2x", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(I386_FYL2XP1, "fyl2xp1", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);

  /* load constants */
  SetOpcodeInfo(I386_FLD1, "fld1", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDZ, "fldz", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDPI, "fldpi", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDL2E, "fldl2e", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDL2T, "fldl2t", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDLG2, "fldlg2", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(I386_FLDLN2, "fldln2", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);

  /* x87 fpu control */
  SetOpcodeInfo(I386_FINCSTP, "fincstp", IT_UNKNOWN, 0, ALL_FPU, 1, 1, 0);	/* by setting both push and pop to 1, all fpu registers are set to used & defined */
  SetOpcodeInfo(I386_FDECSTP, "fdecstp", IT_UNKNOWN, 0, ALL_FPU, 1, 1, 0);
  SetOpcodeInfo(I386_FFREE, "ffree", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FINIT, "finit", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FCLEX, "fclex", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSTCW, "fstcw", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FLDCW, "fldcw", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSTENV, "fstenv", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FLDENV, "fldenv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSTSW, "fstsw", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FSAVE, "fsave", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FRSTOR, "frstor", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FXSAVE, "fxsave", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FXRSTOR, "fxrstor", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_FNOP, "fnop", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(I386_WAIT, "wait", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* --- stuff I don't know what to do with --- */
  SetOpcodeInfo(I386_MOVNTI, "movnti", IT_UNKNOWN, 0, 0, 0, 0, 0);
  
  SetOpcodeInfo(I386_STMXCSR, "stmxcsr", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_LDMXCSR, "ldmxcsr", IT_UNKNOWN, 0, 0, 0, 0, 0);

  SetOpcodeInfo(I386_CLFLUSH, "clflush", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* --- added for instrumentation code --- */
  SetOpcodeInfo(I386_PSEUDOCALL, "pseudocall", IT_CALL, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PSEUDOSAVE, "pseudosave", IT_PSEUDO_SAVE, 0, 0, 0, 0, 0);
  SetOpcodeInfo(I386_PSEUDOLOAD, "pseudoload", IT_PSEUDO_LOAD, 0, 0, 0, 0, 0);

}
