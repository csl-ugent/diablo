#include <diabloamd64.h>

t_amd64_opcode_info amd64_opcode_table[MAX_AMD64_OPCODE];

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
  AMD64_CONDREG_OF,
  AMD64_CONDREG_SF,
  AMD64_CONDREG_ZF,
  AMD64_CONDREG_AF,
  AMD64_CONDREG_PF,
  AMD64_CONDREG_CF,
  AMD64_CONDREG_TF,
  AMD64_CONDREG_IF,
  AMD64_CONDREG_DF,
  AMD64_CONDREG_NT,
  AMD64_CONDREG_RF,
  AMD64_CONDREG_C0,
  AMD64_CONDREG_C1,
  AMD64_CONDREG_C2,
  AMD64_CONDREG_C3
};

static int __iter;

#define SetOpcodeInfo(opc,text,itype,usedcf,definedcf,pops,pushes,fixed_ops)	\
   amd64_opcode_table[(opc)].textual = text; \
   amd64_opcode_table[(opc)].type = itype; \
   amd64_opcode_table[(opc)].fixedops = fixed_ops; \
   amd64_opcode_table[(opc)].fpstack_pops = pops; \
   amd64_opcode_table[(opc)].fpstack_pushes = pushes; \
   amd64_opcode_table[(opc)].cf_used = NullRegs; \
   amd64_opcode_table[(opc)].cf_defined = NullRegs; \
   for (__iter = 0; __iter < 15; __iter++) \
     if ((usedcf) & (1 << __iter)) \
        RegsetSetAddReg(amd64_opcode_table[(opc)].cf_used, condcodes[__iter]); \
   for (__iter = 0; __iter < 15; __iter++) \
     if ((definedcf) & (1 << __iter)) \
        RegsetSetAddReg(amd64_opcode_table[(opc)].cf_defined, condcodes[__iter])

void Amd64InitOpcodeTable(void)
{
  /* --- special entries --- */
  SetOpcodeInfo(AMD64_INVALID_OPC, "(invalid)", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_UNSUPPORTED_OPC, "(unsupported)", IT_UNKNOWN, 0, 0, 0, 0, 0); 
  SetOpcodeInfo(AMD64_DATA, "data", IT_DATA, 0, 0, 0, 0, 0);

  /* --- general purpose instructions --- */

  /* data transfer instructions */
  SetOpcodeInfo(AMD64_CMOVcc, "cmovCC", IT_DATAPROC, OF | SF | ZF | PF | CF, 0 , 0, 0, 0);
  SetOpcodeInfo(AMD64_MOV, "mov", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_BSWAP, "bswap", IT_SWAP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_CMPXCHG, "cmpxchg", IT_SWAP, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_XADD, "xadd", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_XCHG, "xchg", IT_SWAP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_POP, "pop", IT_LOAD, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_POPA, "popa", IT_LOAD, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_PUSH, "push", IT_STORE, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_PUSHA, "pusha", IT_STORE, 0,0, 0, 0, 0);
  SetOpcodeInfo(AMD64_IN, "in", IT_LOAD, 0, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_OUT, "out", IT_STORE, 0, 0, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_CDQ, "cdq", IT_DATAPROC, 0, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_CWDE, "cwde", IT_DATAPROC, 0, 0, 0, 0, DI);
  SetOpcodeInfo(AMD64_MOVSX, "movsbl", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_MOVZX, "movzbl", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_MOVSXD, "movsxd",  IT_DATAPROC, 0, 0, 0, 0, 0);
  
  /* binary arithmetic */
  SetOpcodeInfo(AMD64_ADC, "adc", IT_DATAPROC, CF, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_ADD, "add", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SUB, "sub", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SBB, "sbb", IT_DATAPROC, CF, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_IMUL, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_IMULexp1, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_IMULexp2, "imul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_MUL, "mul", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_IDIV, "idiv", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_DIV, "div", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_NEG, "neg", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_INC, "inc", IT_DATAPROC, 0, OF | SF | ZF | AF | PF, 0, 0, 0);
  SetOpcodeInfo(AMD64_DEC, "dec", IT_DATAPROC, 0, OF | SF | ZF | AF | PF, 0, 0, 0);
  SetOpcodeInfo(AMD64_CMP, "cmp", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  
  /* decimal arithmetic */
  SetOpcodeInfo(AMD64_AAA, "aaa", IT_DATAPROC, AF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(AMD64_AAD, "aad", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(AMD64_AAM, "aam", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(AMD64_AAS, "aas", IT_DATAPROC, AF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(AMD64_DAA, "daa", IT_DATAPROC, AF | CF, ALL_CONDITIONS, 0, 0, DI);
  SetOpcodeInfo(AMD64_DAS, "das", IT_DATAPROC, AF | CF, ALL_CONDITIONS, 0, 0, DI);

  /* logical */
  SetOpcodeInfo(AMD64_NOT, "not", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_AND, "and", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_OR,  "or" , IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_XOR, "xor", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);

  /* shift and rotate */
  SetOpcodeInfo(AMD64_SAR, "sar", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SHL, "shl", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SHLD, "shld", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SHR, "shr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SHRD, "shrd", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_ROR, "ror", IT_DATAPROC, 0, OF | CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_ROL, "rol", IT_DATAPROC, 0, OF | CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_RCR, "rcr", IT_DATAPROC, CF, OF | CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_RCL, "rcl", IT_DATAPROC, CF, OF | CF, 0, 0, 0);

  /* bit and byte instructions */
  SetOpcodeInfo(AMD64_BT, "bt", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_BTC, "btc", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_BTR, "btr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_BTS, "bts", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_BSF, "bsf", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_BSR, "bsr", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_SETcc, "setCC", IT_DATAPROC, OF | SF | ZF | PF | CF, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_TEST, "test", IT_DATAPROC, 0, ALL_CONDITIONS, 0, 0, 0);

  /* control transfer instructions */
  SetOpcodeInfo(AMD64_JMP, "jmp", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_JMPF, "jmpf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_Jcc, "jCC", IT_BRANCH, OF | SF | ZF | PF | CF, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_JRCXZ, "jrcxz", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LOOP, "loop", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LOOPNZ, "loopnz", IT_BRANCH, ZF, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LOOPZ, "loopz", IT_BRANCH, ZF, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_CALL, "call", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_CALLF, "callf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_RET, "ret", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_RETF, "retf", IT_BRANCH, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_IRET, "iret", IT_BRANCH, NT, ALL_CONDITIONS | TF | IF | DF, 0, 0, 0);
  /*SetOpcodeInfo(AMD64_INT, "int", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  /*SetOpcodeInfo(AMD64_INT3, "int3", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  /*SetOpcodeInfo(AMD64_INTO, "into", IT_SWI, ALL_CONDITIONS | TF | IF | DF, NT | TF, 0, 0, 0);*/
  SetOpcodeInfo(AMD64_INT, "int", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(AMD64_INT3, "int3", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(AMD64_INTO, "into", IT_SWI, 0, NT | TF, 0, 0, 0);
  SetOpcodeInfo(AMD64_BOUND, "bound", IT_SWI, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_ENTER, "enter", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LEAVE, "leave", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* string instructions */
  SetOpcodeInfo(AMD64_CMPSB, "cmpsb", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_CMPSD, "cmpsd", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_MOVSB, "movsb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_MOVSD, "movsd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_LODSB, "lodsb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_LODSD, "lodsd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_SCASB, "scasb", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_SCASD, "scasd", IT_UNKNOWN, DF, ALL_CONDITIONS, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_STOSB, "stosb", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_STOSD, "stosd", IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_INSB, "insd" , IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_INSD, "insd" , IT_UNKNOWN, DF, 0, 0, 0, DI|S1I);
  SetOpcodeInfo(AMD64_OUTSB, "outsb", IT_UNKNOWN, DF, 0, 0, 0, S1I|S2I);
  SetOpcodeInfo(AMD64_OUTSD, "outsb", IT_UNKNOWN, DF, 0, 0, 0, S1I|S2I);

  /* flag control instructions */
  SetOpcodeInfo(AMD64_CLC, "clc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_CLD, "cld", IT_STATUS, 0, DF, 0, 0, 0);
  SetOpcodeInfo(AMD64_CLI, "cli", IT_STATUS, 0, IF, 0, 0, 0);
  SetOpcodeInfo(AMD64_STC, "stc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_STD, "std", IT_STATUS, 0, DF, 0, 0, 0);
  SetOpcodeInfo(AMD64_STI, "sti", IT_STATUS, 0, IF, 0, 0, 0);
  SetOpcodeInfo(AMD64_CMC, "cmc", IT_STATUS, 0, CF, 0, 0, 0);
  SetOpcodeInfo(AMD64_LAHF, "lahf", IT_STATUS, SF | ZF | AF | PF | CF, 0, 0, 0, DI);
  SetOpcodeInfo(AMD64_SAHF, "sahf", IT_STATUS, 0, SF | ZF | AF | PF | CF, 0, 0, S1I);
  SetOpcodeInfo(AMD64_POPF, "popf", IT_STATUS, 0, ALL_CONDITIONS | TF | DF | IF | NT, 0, 0, 0);
  SetOpcodeInfo(AMD64_PUSHF, "pushf", IT_STATUS, ALL_CONDITIONS | TF | DF | IF | NT, 0, 0, 0, 0);

  /* segment register instructions */
  SetOpcodeInfo(AMD64_LDS, "lds", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LES, "les", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LGS, "lgs", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LSS, "lss", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LFS, "lfs", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* miscellaneous instructions */
  SetOpcodeInfo(AMD64_LEA, "lea", IT_DATAPROC, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_NOP, "nop", IT_NOP, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_UD2, "ud2", IT_SWI, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_CPUID, "cpuid", IT_UNKNOWN, 0, 0, 0, 0, S1I);
  SetOpcodeInfo(AMD64_XLAT, "xlat", IT_DATAPROC, 0, 0, 0, 0, DI);

  /* --- system instructions --- */
  SetOpcodeInfo(AMD64_LGDT, "lgdt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LIDT, "lidt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LLDT, "lldt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_SGDT, "sgdt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_SIDT, "sidt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_SLDT, "sldt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LTR, "ltr", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_STR, "str", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LMSW, "lmsw", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_SMSW, "smsw", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_CLTS, "clts", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LAR, "lar", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(AMD64_LSL, "lsl", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(AMD64_VERR, "verr", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(AMD64_VERW, "verw", IT_UNKNOWN, 0, ZF, 0, 0, 0);
  SetOpcodeInfo(AMD64_WBINVD, "wbinvd", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_INVD, "invd", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_INVLPG, "invlpg", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_HLT, "hlt", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_RSM, "rsm", IT_UNKNOWN, 0, ALL_CONDITIONS | TF | IF | DF | NT | RF, 0, 0, 0);
  SetOpcodeInfo(AMD64_RDMSR, "rdmsr", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(AMD64_WRMSR, "wrmsr", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(AMD64_RDPMC, "rdpmc", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(AMD64_RDTSC, "rdtsc", IT_UNKNOWN, 0, 0, 0, 0, DI|S1I|S2I);
  SetOpcodeInfo(AMD64_SYSENTER, "sysenter", IT_SWI, ALL_CONDITIONS | TF | IF | DF, TF | NT, 0, 0, 0); /* not sure about sysenter and sysexit, just guessing! */
  SetOpcodeInfo(AMD64_SYSEXIT, "sysexit", IT_UNKNOWN, NT, ALL_CONDITIONS | TF | IF | DF, 0, 0, 0);
  SetOpcodeInfo(AMD64_SYSCALL, "syscall", IT_SWI, ALL_CONDITIONS | TF | IF | DF, TF | NT, 0, 0, 0);
  SetOpcodeInfo(AMD64_SYSRET, "sysret", IT_UNKNOWN, NT, ALL_CONDITIONS | TF | IF | DF, 0, 0, 0);
  
  
  /* --- floating point instructions --- */

  /* data transfer instructions */
  SetOpcodeInfo(AMD64_FLD, "fld", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FST, "fst", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSTP, "fstp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FILD, "fild", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FIST, "fist", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FISTP, "fistp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FBLD, "fbld", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FBSTP, "fbstp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FXCH, "fxch", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FCMOVcc, "fcmovCC", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* basic arithmetic */
  SetOpcodeInfo(AMD64_FADD, "fadd", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FADDP, "faddp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FIADD, "fiadd", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSUB, "fsub", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSUBP, "fsubp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FSUBR, "fsubr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSUBRP, "fsubrp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FISUB, "fisub", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FISUBR, "fisubr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FMUL, "fmul", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FMULP, "fmulp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FIMUL, "fimul", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FDIV, "fdiv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FDIVP, "fdivp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FDIVR, "fdivr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FDIVRP, "fdivrp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FIDIV, "fidiv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FIDIVR, "fidivr", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FPREM, "fprem", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FPREM1, "fprem1", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FABS, "fabs", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FCHS, "fchs", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FRNDINT, "frndint", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSCALE, "fscale", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSQRT, "fsqrt", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FXTRACT, "fxtract", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);

  /* comparison */
  SetOpcodeInfo(AMD64_FCOM, "fcom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FCOMI, "fcomi", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_FCOMP, "fcomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FCOMIP, "fcomip", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 1, 0, 0);
  SetOpcodeInfo(AMD64_FCOMPP, "fcompp", IT_UNKNOWN, 0, ALL_FPU, 2, 0, 0);
  SetOpcodeInfo(AMD64_FUCOM, "fucom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FUCOMI, "fucomi", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 0, 0, 0);
  SetOpcodeInfo(AMD64_FUCOMP, "fucomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FUCOMIP, "fucomip", IT_UNKNOWN, 0, ALL_FPU | ALL_CONDITIONS, 1, 0, 0);
  SetOpcodeInfo(AMD64_FUCOMPP, "fucompp", IT_UNKNOWN, 0, ALL_FPU, 2, 0, 0);
  SetOpcodeInfo(AMD64_FICOM, "ficom", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FICOMP, "ficomp", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FTST, "ftst", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FXAM, "fxam", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* transcendental */
  SetOpcodeInfo(AMD64_FCOS, "fcos", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSIN, "fsin", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSINCOS, "fsincos", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FPTAN, "fptan", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FPATAN, "fpatan", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_F2XM1, "f2xm1", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FYL2X, "fyl2x", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);
  SetOpcodeInfo(AMD64_FYL2XP1, "fyl2xp1", IT_UNKNOWN, 0, ALL_FPU, 1, 0, 0);

  /* load constants */
  SetOpcodeInfo(AMD64_FLD1, "fld1", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDZ, "fldz", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDPI, "fldpi", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDL2E, "fldl2e", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDL2T, "fldl2t", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDLG2, "fldlg2", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);
  SetOpcodeInfo(AMD64_FLDLN2, "fldln2", IT_UNKNOWN, 0, ALL_FPU, 0, 1, 0);

  /* x87 fpu control */
  SetOpcodeInfo(AMD64_FINCSTP, "fincstp", IT_UNKNOWN, 0, ALL_FPU, 1, 1, 0);	/* by setting both push and pop to 1, all fpu registers are set to used & defined */
  SetOpcodeInfo(AMD64_FDECSTP, "fdecstp", IT_UNKNOWN, 0, ALL_FPU, 1, 1, 0);
  SetOpcodeInfo(AMD64_FFREE, "ffree", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FINIT, "finit", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FCLEX, "fclex", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSTCW, "fstcw", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FLDCW, "fldcw", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSTENV, "fstenv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FLDENV, "fldenv", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSTSW, "fstsw", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FSAVE, "fsave", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FRSTOR, "frstor", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FXSAVE, "fxsave", IT_UNKNOWN, ALL_FPU, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FXRSTOR, "fxrstor", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_FNOP, "fnop", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);
  SetOpcodeInfo(AMD64_WAIT, "wait", IT_UNKNOWN, 0, ALL_FPU, 0, 0, 0);

  /* --- stuff I don't know what to do with --- */
  SetOpcodeInfo(AMD64_MOVNTI, "movnti", IT_UNKNOWN, 0, 0, 0, 0, 0);
  
  SetOpcodeInfo(AMD64_STMXCSR, "stmxcsr", IT_UNKNOWN, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_LDMXCSR, "ldmxcsr", IT_UNKNOWN, 0, 0, 0, 0, 0);

  /* --- added for instrumentation code --- */
  SetOpcodeInfo(AMD64_PSEUDOCALL, "pseudocall", IT_CALL, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_PSEUDOSAVE, "pseudosave", IT_PSEUDO_SAVE, 0, 0, 0, 0, 0);
  SetOpcodeInfo(AMD64_PSEUDOLOAD, "pseudoload", IT_PSEUDO_LOAD, 0, 0, 0, 0, 0);

  /*SSE*/
  SetOpcodeInfo(AMD64_MOVSD_SSE,"movsd",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVSS,"movss",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVAPS,"movaps",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVAPD,"movapd",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVUPS,"movups",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVUPD,"movupd",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVDDUP,"movddup",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVSLDUP,"movsldup",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVLPS,"movlps",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_MOVLPD,"movlpd",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_CVTTSD2SI,"cvttsd2si",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_CVTTSS2SI,"cvttss2si",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_XORPS,"xorps",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_XORPD,"xorpd",IT_DATAPROC,0,0,0,0,0);
  SetOpcodeInfo(AMD64_COMISS,"comiss",IT_DATAPROC,0,ALL_CONDITIONS|ALL_FPU,0,0,0);
  SetOpcodeInfo(AMD64_COMISD,"comisd",IT_DATAPROC,0,ALL_CONDITIONS|ALL_FPU,0,0,0);
  SetOpcodeInfo(AMD64_UCOMISS,"ucomiss",IT_DATAPROC,0,ALL_CONDITIONS|ALL_FPU,0,0,0);
  SetOpcodeInfo(AMD64_UCOMISD,"ucomisd",IT_DATAPROC,0,ALL_CONDITIONS|ALL_FPU,0,0,0);
	  
}
