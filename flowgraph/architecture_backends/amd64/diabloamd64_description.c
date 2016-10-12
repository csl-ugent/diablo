#include <diabloamd64.h>
/* names {{{ */
static char * names[] =
{
  "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP",
  "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",		    	    /* GP registers */
  "ST0", "ST1", "ST2", "ST3", "ST4", "ST5", "ST6", "ST7",                   /* FP registers */
  "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7",
  "XMM8", "XMM9", "XMM10", "XMM11", "XMM12", "XMM13", "XMM14", "XMM15",     /* xmm registers */
  "CS", "DS", "ES", "FS", "GS", "SS",                                       /* segment registers */
  "CR0", "CR1", "CR2", "CR3", "CR4", "CR5", "CR6", "CR7", "CR8",            /* control registers */
  "DR0", "DR1", "DR2", "DR3", "DR4", "DR5", "DR6", "DR7", 		    /* debug registers */
  "AF" , "CF" , "DF" , "IF" , "NT" , "OF" , "PF" , "RF" , "SF", "TF", "ZF", /* eflags       */
  "C0" , "C1" , "C2" , "C3"                                                 /* FP condition */
  "RIP"
};
/* }}} */

/*{{{ Plugs to cast to T_Amd64_INS*/
static void Amd64InsCleanupPlug(t_ins * ins)
{
  return Amd64InsCleanup(T_AMD64_INS(ins));
}
static void Amd64InstructionMakeJumpPlug(t_ins * ins)
{
  return Amd64InstructionMakeJump(T_AMD64_INS(ins));
}
static void Amd64InsDupDynamicPlug(t_ins * target, t_ins * source)
{
  return Amd64InsDupDynamic(T_AMD64_INS(target), T_AMD64_INS(source));
}
static t_bool Amd64InsHasSideEffectPlug(t_ins * ins)
{
  return Amd64InsHasSideEffect(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsLoadPlug(t_ins * ins)
{
  return Amd64InsIsLoad(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsStorePlug(t_ins * ins)
{
  return Amd64InsIsStore(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsProcedureCallPlug(t_ins * ins)
{
  return Amd64InsIsProcedureCall(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsIndirectCallPlug(t_ins * ins)
{
  return Amd64InsIsIndirectCall(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsUnconditionalBranchPlug(t_ins * ins)
{
  return Amd64InsIsUnconditionalBranch(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsControlTransferPlug(t_ins * ins)
{
  return Amd64InsIsControlTransfer(T_AMD64_INS(ins));
}
static t_bool Amd64InsIsSystemPlug(t_ins * ins)
{
  return Amd64InsIsSystemControlTransfer(T_AMD64_INS(ins)) ||
    Amd64InsIsSystemInstruction (T_AMD64_INS (ins));
}
static t_tristate Amd64IsSyscallExitPlug(t_ins * ins)
{
  return Amd64IsSyscallExit(T_AMD64_INS(ins));
}
static void Amd64InstructionMakeNoopPlug(t_ins * ins)
{
  return Amd64InstructionMakeNoop(T_AMD64_INS(ins));
}
static t_bool Amd64InstructionUnconditionalizerPlug(t_ins * ins)
{
  return Amd64InstructionUnconditionalizer(T_AMD64_INS(ins));
}
static void Amd64InstructionPrintPlug(t_ins * ins, t_string outputstring)
{
  Amd64InstructionPrint(T_AMD64_INS(ins), outputstring);
}
static t_bool Amd64InsAreIdenticalPlug(t_ins * ins1, t_ins * ins2)
{
  return Amd64InsAreIdentical(T_AMD64_INS(ins1), T_AMD64_INS(ins2));
}
static t_bool Amd64ParseFromStringAndInsertAtPlug(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before)
{
  return Amd64ParseFromStringAndInsertAt(ins_text, bbl, at_ins, before);
}
/*}}}*/

t_bool Amd64FunIsGlobal(t_function* fun)
{
  if (FUNCTION_FLAGS(fun) & FF_IS_EXPORTED)
  {
    return TRUE;
  }
  return FALSE;
}




t_architecture_description amd64_description =
{
  /*! Size of an instruction: encoded minimal size */
   8, /* encoded minimal size [1 byte] */
  /*! Size of an instruction: encoded maximum size */
   120, /* encoded maximum size [15 byte] */
   8, /* mod size */
   1, /* bundel size */
   0, /* bundel template size */
  /*! Size of an instruction: disassembled instruction size */
   sizeof(t_amd64_ins), /* size of an instruction */
   ADDRSIZE64,
  /*! The number of general purpose integer registers */
   16,  /* int regs */
  /*! The number of general purpose floating point registers */
   8,  /* float regs */
  /*! The number of predicate registers */
   0,  /* predicate regs */
  /*! The number of branch registers */
   0,  /* branch regs */
  /*! The number of special registers */
   53, /* special regs */
#if MAX_REG_ITERATOR > 64
/* all registers            */   {MAX_REG_ITERATOR, {0xffffffff,0xffffffff,0x3fff}},	//alles
/* int registers            */   {MAX_REG_ITERATOR, {0x0000ffff}},				//rax-r15
/* float registers          */   {MAX_REG_ITERATOR, {0x00ff0000}},			//st0-st7
/* predicate registers      */   {MAX_REG_ITERATOR, {0x00000000,0xc0000000,0x1fff}},	//eflags+fp cond.
/* callee saved             */   {MAX_REG_ITERATOR, {0x0000f0c2}},				//rbx,rsp,rbp,r12-r15
/* callee may use           */   {MAX_REG_ITERATOR, {0xff00033d,0x00000000}},		//rax,rcx,rdx,rsi,rdi
/* callee may change        */   {MAX_REG_ITERATOR, {0xffff0f2c,0x1fffffff}},		//alles zonder
/* callee may return        */   {MAX_REG_ITERATOR, {0x03030009}},			//rax,rdx,st0,st1
/* always live              */   {MAX_REG_ITERATOR, {0x0000}},
/* registers prop over hell */   {MAX_REG_ITERATOR, {0x0000}},
/* const registers          */   {MAX_REG_ITERATOR, {0x0000}},
/* dead over call           */   {MAX_REG_ITERATOR, {0x0000}},
/* link registers           */   {MAX_REG_ITERATOR, {0x0000}},
/* argument registers       */ {MAX_REG_ITERATOR, {0x0000}},
/* return registers         */ {MAX_REG_ITERATOR, {0x0001}},
/* dyncall use registers    */ {MAX_REG_ITERATOR, {0}},
#else
#error   amd64 has more than 64 registers!!!
#endif

   /*! The program counter */
   AMD64_REG_RIP,
   /*! An array containing the name of each register */
   names,
   /*! Callback to disassemble a section */
   Amd64DisassembleSection,
   /*! Callback to assemble a section */
   Amd64AssembleSection,
   /*! Callback to create a flowgraph for an object */
   Amd64Flowgraph,
   /*! Callback to deflowgraph a section */
   Amd64Deflowgraph,
   /*! Callback to make addressproducers for a section */
   Amd64MakeAddressProducers,
   NULL,
   Amd64InstructionMakeJumpPlug,
   Amd64InsDupDynamicPlug,
   Amd64InsHasSideEffectPlug,
   /*! Returns true when instruction is load */
   Amd64InsIsLoadPlug,
   /*! Returns true when instruction is a store */
   Amd64InsIsStorePlug,
   Amd64InsIsProcedureCallPlug,
   Amd64InsIsIndirectCallPlug,
   NULL,
   Amd64InsIsUnconditionalBranchPlug,
   Amd64InsIsControlTransferPlug,
   Amd64InsIsSystemPlug,
   /*! Returns true when instruction is program exit */
   Amd64IsSyscallExitPlug,
   Amd64InstructionPrintPlug,
   Amd64InstructionMakeNoopPlug,
   Amd64InsAreIdenticalPlug,
   Amd64InstructionUnconditionalizerPlug, /* */
   Amd64ParseFromStringAndInsertAtPlug,
   NULL,
   /*Amd64FunComputeStackSavedRegisters,*/
   Amd64FunIsGlobal,
   /*! An array with names of unbehaved functions */
   NULL,   
   NULL,
   NULL,
   NULL
};
/* vim: set shiftwidth=2: */
