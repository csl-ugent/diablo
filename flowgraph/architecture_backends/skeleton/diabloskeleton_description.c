#include <diabloskeleton.h>
/* names {{{ */
static char * names[] =
{
  "the names", "of your architecture's", "registers"
};
/* }}} */

t_architecture_description skeleton_description =
{
  /*! Size of an instruction: encoded minimal size */
   32, /* encoded minimal size [1 byte] */
  /*! Size of an instruction: encoded maximum size */
   32, /* encoded maximum size [15 byte] */
   8, /* mod size */
   1, /* bundle size */
   0, /* bundle template size */
   sizeof(t_skeleton_ins), /* size of an instruction */
   ADDRSIZE32, /* ADDRSIZE64 for 64-bit architectures */
  /*! The number of general purpose integer registers */
   0,  /* int regs */
  /*! The number of general purpose floating point registers */
   0,  /* float regs */
  /*! The number of predicate registers */
   0,  /* predicate regs */
  /*! The number of branch registers */
   0,  /* branch regs */
  /*! The number of special registers */
   0, /* special regs */

/* register sets: this is an ugly mess that needs to be cleaned up ASAP
 * the values filled in here are those of the i386, they just serve as
 * an example */
#if MAX_REG_ITERATOR > 64
/* more than 64 registers: regsets are represented as a structure with an array of t_uint16 values */
/* all registers            */   {MAX_REG_ITERATOR, {0xffffULL,0xffffULL,0xffffULL,0x03ffULL}},
/* int registers            */   {MAX_REG_ITERATOR, {0x00ffULL}},
/* float registers          */   {MAX_REG_ITERATOR, {0xff00ULL}},
/* predicate registers      */   {MAX_REG_ITERATOR, {0x0000ULL,0x0000ULL,0xf800ULL,0x03ffULL}},
/* callee saved             */   {MAX_REG_ITERATOR, {0x0072ULL}},
/* callee may use           */   {MAX_REG_ITERATOR, {0xffffULL,0xffffULL,0xffffULL,0x0003ULL}},
/* callee may change        */   {MAX_REG_ITERATOR, {0xffffULL,0xffffULL,0xffffULL,0x0003ULL}},
/* callee may return        */   {MAX_REG_ITERATOR, {0x0000ULL}},
/* always live              */   {MAX_REG_ITERATOR, {0x0000ULL}},
/* registers prop over hell */   {MAX_REG_ITERATOR, {0x0000ULL}},
/* const registers          */   {MAX_REG_ITERATOR, {0x0000ULL}},
/* dead over call           */   {MAX_REG_ITERATOR, {0x0000ULL}},
/* link registers           */   {MAX_REG_ITERATOR, {0x0000ULL}},
#elif MAX_REG_ITERATOR > 32
/* less than 64 registers: regsets are represented as t_uint64 */
/* all registers            */   0x3ffffffffffffffLL,
/* int registers            */   0x0000000000000ffLL,
/* float registers          */   0x00000000000ff00LL,
/* predicate registers      */   0x3fff80000000000LL,
/* callee saved             */   0x000000000000072LL,
/* callee may use           */   0x0000000ffff0080LL,
/* callee may change        */   0x3fff8000000ff8dLL, /* callee may change the stack pointer: see ret $0x... instruction */
/* callee may return        */   0x000000000000109LL,
/* always live              */   0x000000000000080LL, /* stack pointer is always live */
/* registers prop over hell */   0x0LL,
/* const registers          */   0x0LL,
/* dead over call           */   0x3fff80000000000LL,
/* link registers           */   0x0LL,
#else
/* less than 32 registers: regsets are represented as t_uint32 */
#error   skeleton has more than 32 registers!!!
#endif

   /*! The program counter */
   REG_NONE, /* REG_NONE if the program counter is not user-visible,
		the pc's register number otherwise */
   /*! An array containing the name of each register */
   names,
   /*! Callback to disassemble a section */
   NULL /*SkeletonDisassembleSection*/,
   /*! Callback to assemble a section */
   NULL /*SkeletonAssembleSection*/,
   /*! Callback to create a flowgraph for an object */
   NULL /*SkeletonFlowgraph*/,
   /*! Callback to deflowgraph a section */
   NULL /*SkeletonDeflowgraph*/,
   /*! Callback to make addressproducers for a section */
   NULL /*SkeletonMakeAddressProducers*/,
   NULL,
   NULL /*SkeletonInstructionMakeJump*/,
   NULL,
   NULL /*SkeletonInsHasSideEffect*/,
   /*! Returns true when instruction is load */
   NULL /*SkeletonInsIsLoad*/,
   /*! Returns true when instruction is a store */
   NULL /*SkeletonInsIsStore*/,
   NULL /*SkeletonInsIsProcedureCall*/,
   NULL /*SkeletonInsIsIndirectCall*/,
   NULL /*SkeletonInsIsConditional*/,
   NULL /*SkeletonInsIsUnconditionalBranch*/,
   NULL /*SkeletonInsIsControlTransfer*/,
   /*! Returns true when instruction is program exit */
   NULL /*SkeletonIsSyscallExit*/,
   NULL /*SkeletonInstructionPrint*/,
   NULL /*SkeletonInstructionMakeNoop*/,
   NULL /*SkeletonInsAreIdentical*/,
   NULL /*SkeletonInstructionUnconditionalizer*/, /* */
   NULL /*SkeletonParseFromStringAndInsertAt*/,
   NULL,
//   SkeletonFunComputeStackSavedRegisters,
   NULL /*SkeletonFunIsGlobal*/,
   /*! An array with names of unbehaved functions */
   NULL,   
   NULL,
   NULL,
   NULL
};
/* vim: set shiftwidth=2: */
