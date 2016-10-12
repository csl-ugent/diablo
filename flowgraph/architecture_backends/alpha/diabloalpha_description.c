#include <diabloalpha.h>
/* names {{{ */
static char * AlphaRegs[] = {
		
	/* t: temporary */
	/* v: hold integer function results */
	/* s: saved registers */
	/* ra: return address */
	/* t12 == pv: procedure value */

		"v0", "t0","t1","t2","t3","t4","t5","t6","t7", "s0","s1","s2","s3",
		"s4","s5", "fp", "a0","a1","a2","a3","a4","a5","t8","t9","t10","t11",
		"ra", "t12", "at", "gp", "sp", "zero",

	/* FP registers */
	
		"f0","f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12",
		"f13","f14","f15","f16","f17","f18","f19","f20","f21","f22","f23",
		"f24","f25","f26","f27","f28","f29","f30","f31"


};
/* }}} */

static char *unbehaved_funs[] = {"_Ots", "__rem", "__dib", ""};

t_architecture_description alpha_description =
{
  /*! Size of an instruction: encoded minimal size */
  32,
  /*! Size of an instruction: encoded maximum size */
  32,
  /*! Size of an instruction: \todo document */
  32, /* mod size */
  /*! Size of an instruction: the number of instructions in a bundle */
  1,
  /*! Size of an instruction: size of the bundle template */ 
  0, /* bundel template size */
  /*! Size of an instruction: disassembled instruction size */
  sizeof(t_alpha_ins), /* sizeof an instruction */
  /*! \todo document */
  ADDRSIZE64,
  /*! The number of general purpose integer registers */
  32,
  /*! The number of general purpose floating point registers */
  32,
  /*! The number of predicate registers */
  0,
  /*! The number of branch registers */
  0,
  /*! The number of special registers */
  0,
#if MAX_REG_ITERATOR > 64

  /* all registers */   
  {MAX_REG_ITERATOR, {0xffffffff,0xffffffff}},
  /* int registers */   
  {MAX_REG_ITERATOR, {0xffffffff}},
  /* float registers */   
  {MAX_REG_ITERATOR, {0x00000000,0xffffffff}},
  /* predicate registers */
  {MAX_REG_ITERATOR, {0x00000000,0x00000000}},
  /* callee saved registers */   
  {MAX_REG_ITERATOR, {0x4000fe00,0x0000fe00}}, 
  /* callee may use registers */   
  {MAX_REG_ITERATOR, {0x4c3f0001,0x002f0000}},
  /* callee may change        */   
  {MAX_REG_ITERATOR, {0x9fff01ff,0xffff01ff}},
  /* callee may return        */   
  {MAX_REG_ITERATOR, {0x00000001,0x00000001}},
  /* always live              */   
  {MAX_REG_ITERATOR, {0x0}},
  /* registers prop over hell */   
  {MAX_REG_ITERATOR, {0x20000000}},
  /* const registers          */   
  {MAX_REG_ITERATOR, {0x80000000,0x80000000}},
  /* dead over call           */   
  {MAX_REG_ITERATOR, {0x00000000,0x00000000}},
  /* link registers           */   
  {MAX_REG_ITERATOR, {0x00000000,0x00000000}},
  /* argument_regs */
  {MAX_REG_ITERATOR, {0x00000000,0}},
  /* return_regs */
  {MAX_REG_ITERATOR, {0x00000000,0}}, 
  /* dyncall use registers */
  {MAX_REG_ITERATOR, {0}},
#elif MAX_REG_ITERATOR > 32
/* all registers            */   0xffffffffffffffffULL,
/* int registers            */   0x00000000ffffffffULL,
/* float registers          */   0xffffffff00000000ULL,
/* predicate registers      */   0x0000000000000000ULL,
/* callee saved             */   0x0000fe004000fe00ULL,
/* callee may use           */   0x002f00004c3f0001ULL,
/* callee may change        */   0xffff01ff9fff01ffULL,
/* callee may return        */   0x0000000100000001ULL,
/* always live              */   0x0000000000000000ULL,
/* registers prop over hell */   0x0000000020000000ULL,
/* const registers          */   0x8000000080000000ULL,
/* dead over call           */   0x0000000000000000ULL,
/* link registers           */   0x0LL,
/* dyncall use registers   */    0x0LL,
#else
#error  The Alpha architecture has more than 32 registers!!!
#endif

   /*! The program counter */
   REG_NONE,
   /*! An array containing the name of each register */
   AlphaRegs,
   /*! Callback to disassemble a section */
   AlphaDisassembleSection,
   /*! Callback to assemble a section */
   AlphaAssembleSection,
   /*! Callback to create a flowgraph for an object */
   AlphaFlowgraph,
   /*! Callback to deflowgraph a section */
   AlphaDeflowgraph,
   /*! Callback to make addressproducers for a section */
   AlphaMakeAddressProducers,
   NULL,
   NULL /*AlphaInstructionMakeJump*/,
   NULL,
   AlphaInsHasSideEffect,
   /*! Returns true when instruction is load */
   GenericInsIsLoad,
   /*! Returns true when instruction is a store */
   GenericInsIsStore,
   AlphaInsIsProcedureCall,
   AlphaInsIsIndirectCall,
   NULL,
   AlphaInsIsUnconditionalJump,
   AlphaInsIsControlTransfer,
   AlphaInsIsSystemPlug,
   /*! Returns true when instruction is program exit */
   AlphaInsIsSyscallExit,
   AlphaInstructionPrint,
   (void (*)(t_ins*))AlphaInsMakeNoop, /*AlphaInstructionMakeNoop*/
   NULL /*AlphaInsAreIdentical*/,
   NULL /*AlphaInstructionUnconditionalizer*/, /* */
   NULL /*AlphaParseFromStringAndInsertAt*/,
   NULL,/*AlphaFunComputeStackSavedRegisters, */
   AlphaFunIsGlobal,/*AlphaFunIsGlobal */
   /*! An array with names of unbehaved functions */
   unbehaved_funs,
   NULL,
   NULL,
   NULL
};
