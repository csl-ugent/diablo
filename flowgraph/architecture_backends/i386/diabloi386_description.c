/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>
/* names {{{ */
static char * names[] =
{
  "EAX", "EBX", "ECX", "EDX", "ESI", "EDI", "EBP", "ESP",                   /* GP registers */
  "ST0", "ST1", "ST2", "ST3", "ST4", "ST5", "ST6", "ST7",                   /* FP registers */
  "CS" , "DS" , "ES" , "FS" , "GS" , "SS",                                  /* segment registers */
  "CR0", "CR1", "CR2", "CR3", "CR4",					    /* control registers */
  "DR0", "DR1", "DR2", "DR3", "DR4", "DR5", "DR6", "DR7", 		    /* debug registers */
  "XMM0", "XMM1", "XMM2", "XMM3", "XMM4", "XMM5", "XMM6", "XMM7", 	    /* xmm registers */
  "AF" , "CF" , "DF" , "IF" , "NT" , "OF" , "PF" , "RF" , "SF", "TF", "ZF", /* eflags       */
  "C0" , "C1" , "C2" , "C3" , "FPSW"                                        /* FP condition */
};
/* }}} */

/*{{{ Plugs to cast to T_I386_INS*/
static void I386InsCleanupPlug(t_ins * ins)
{
  return I386InsCleanup(T_I386_INS(ins));
}
static void I386InstructionMakeJumpPlug(t_ins * ins)
{
  return I386InstructionMakeJump(T_I386_INS(ins));
}
static void I386InsDupDynamicPlug(t_ins * target, t_ins * source)
{
  return I386InsDupDynamic(T_I386_INS(target), T_I386_INS(source));
}
static t_bool I386InsHasSideEffectPlug(t_ins * ins)
{
  return I386InsHasSideEffect(T_I386_INS(ins));
}
static t_bool I386InsIsLoadPlug(t_ins * ins)
{
  return I386InsIsLoad(T_I386_INS(ins));
}
static t_bool I386InsIsStorePlug(t_ins * ins)
{
  return I386InsIsStore(T_I386_INS(ins));
}
static t_bool I386InsIsProcedureCallPlug(t_ins * ins)
{
  return I386InsIsProcedureCall(T_I386_INS(ins));
}
static t_bool I386InsIsIndirectCallPlug(t_ins * ins)
{
  return I386InsIsIndirectCall(T_I386_INS(ins));
}
static t_bool I386InsIsUnconditionalBranchPlug(t_ins * ins)
{
  return I386InsIsUnconditionalBranch(T_I386_INS(ins));
}
static t_bool I386InsIsConditionalPlug(t_ins * ins)
{
  return I386InsIsConditional(T_I386_INS(ins));
}
static t_bool I386InsIsControlTransferPlug(t_ins * ins)
{
  return I386InsIsControlTransfer(T_I386_INS(ins));
}
static t_bool I386InsIsSystemPlug(t_ins * ins)
{
  return I386InsIsSystemControlTransfer(T_I386_INS(ins)) ||
    I386InsIsSystemInstruction (T_I386_INS (ins));
}
static t_tristate I386IsSyscallExitPlug(t_ins * ins)
{
  return I386IsSyscallExit(T_I386_INS(ins));
}
static void I386InstructionMakeNoopPlug(t_ins * ins)
{
  return I386InstructionMakeNoop(T_I386_INS(ins));
}
static t_bool I386InstructionUnconditionalizerPlug(t_ins * ins)
{
  return I386InstructionUnconditionalizer(T_I386_INS(ins));
}
static void I386InstructionPrintPlug(t_ins * ins, t_string outputstring)
{
  return I386InstructionPrint(T_I386_INS(ins), outputstring);
}
static t_bool I386InsAreIdenticalPlug(t_ins * ins1, t_ins * ins2)
{
  return I386InsAreIdentical(T_I386_INS(ins1), T_I386_INS(ins2));
}
static t_bool I386ParseFromStringAndInsertAtPlug(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before)
{
  return I386ParseFromStringAndInsertAt(ins_text, bbl, T_I386_INS(at_ins), before);
}
/*}}}*/

t_bool I386FunIsGlobal(t_function* fun)
{
  if (FUNCTION_FLAGS(fun) & FF_IS_EXPORTED)
  {
    return TRUE;
  }
  return FALSE;
}



t_architecture_description i386_description =
{
  /*! Size of an instruction: encoded minimal size */
   8, /* encoded minimal size [1 byte] */
  /*! Size of an instruction: encoded maximum size */
   120, /* encoded maximum size [15 byte] */
   8, /* mod size */
   1, /* bundel size */
   0, /* bundel template size */
  /*! Size of an instruction: disassembled instruction size */
   sizeof(t_i386_ins), /* size of an instruction */
   ADDRSIZE32,
  /*! The number of general purpose integer registers */
   8,  /* int regs */
  /*! The number of general purpose floating point registers */
   8,  /* float regs */
  /*! The number of predicate registers */
   0,  /* predicate regs */
  /*! The number of branch registers */
   0,  /* branch regs */
  /*! The number of special registers */
   43, /* special regs */
#if MAX_REG_ITERATOR > 64
/* all registers            */   {MAX_REG_ITERATOR, {0xffffffff,0x07ffffff}},
/* int registers            */   {MAX_REG_ITERATOR, {0x000000ff}},
/* float registers          */   {MAX_REG_ITERATOR, {0x0000ff00}},
/* predicate registers      */   {MAX_REG_ITERATOR, {0x00000000,0x07fff800}},
/* callee saved             */   {MAX_REG_ITERATOR, {0x00000072}},
/* callee may use           */   {MAX_REG_ITERATOR, {0xffff0080}},
/* callee may change        */   {MAX_REG_ITERATOR, {0x0000ff8d,0x07fff800}},
/* callee may return        */   {MAX_REG_ITERATOR, {0x0109}},
/* always live              */   {MAX_REG_ITERATOR, {0x0080}},
/* registers prop over hell */   {MAX_REG_ITERATOR, {0x0000}},
/* const registers          */   {MAX_REG_ITERATOR, {0x0000}},
/* dead over call           */   {MAX_REG_ITERATOR, {0x00000000,0x03fff800}},
/* link registers           */   {MAX_REG_ITERATOR, {0x00000000,0x00000000}},
/* argument registers       */   {MAX_REG_ITERATOR, {0x00000000,0x00000000}},
/* return registers         */   {MAX_REG_ITERATOR, {0x00000001,0x00000000}},
/* dyncall use registers    */   {MAX_REG_ITERATOR, {0x00000002,0x00000000}}, /* ebx is used by plt stubs */
#elif MAX_REG_ITERATOR > 32
/* all registers            */   0x7ffffffffffffffLL,
/* int registers            */   0x0000000000000ffLL,
/* float registers          */   0x00000000000ff00LL,
/* predicate registers      */   0x7fff80000000000LL,
/* callee saved             */   0x000000000000072LL,
/* callee may use           */   0x0000000ffff0080LL,
/* callee may change        */   0x7fff8000000ff8dLL, /* callee may change the stack pointer: see ret $0x... instruction */
/* callee may return        */   0x000000000000109LL,
/* always live              */   0x000000000000080LL, /* stack pointer is always live */
/* registers prop over hell */   0x0LL,
/* const registers          */   0x0LL,
/* dead over call           */   0x3fff80000000000LL,
/* link registers           */   0x0LL,
/* argument registers       */   0x0LL,
/* return registers         */   0x1LL,
/* dyncall use registers    */   0x2LL, /* ebx is used by plt stubs */
#else
#error   i386 has more than 32 registers!!!
#endif

   /*! The program counter */
   REG_NONE,
   /*! An array containing the name of each register */
   names,
   /*! Callback to disassemble a section */
   I386DisassembleSection,
   /*! Callback to assemble a section */
   I386AssembleSection,
   /*! Callback to create a flowgraph for an object */
   I386Flowgraph,
   /*! Callback to deflowgraph */
   I386Deflowgraph,
   /*! Callback to make addressproducers for a section */
   I386MakeAddressProducers,
   I386InsCleanupPlug,
   I386InstructionMakeJumpPlug,
   I386InsDupDynamicPlug,
   I386InsHasSideEffectPlug,
   /*! Returns true when instruction is load */
   I386InsIsLoadPlug,
   /*! Returns true when instruction is a store */
   I386InsIsStorePlug,
   I386InsIsProcedureCallPlug,
   I386InsIsIndirectCallPlug,
   I386InsIsConditionalPlug,
   I386InsIsUnconditionalBranchPlug,
   I386InsIsControlTransferPlug,
   I386InsIsSystemPlug,
   /*! Returns true when instruction is program exit */
   I386IsSyscallExitPlug,
   I386InstructionPrintPlug,
   I386InstructionMakeNoopPlug,
   I386InsAreIdenticalPlug,
   I386InstructionUnconditionalizerPlug, /* */
   I386ParseFromStringAndInsertAtPlug,
   NULL,
//   I386FunComputeStackSavedRegisters,
   I386FunIsGlobal,
   /*! An array with names of unbehaved functions */
   NULL,   
   NULL,
   NULL,
   NULL
};
/* vim: set shiftwidth=2: */
