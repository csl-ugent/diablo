/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/*! \todo Document, keep only one definition of register names */
/* names {{{ */
static char * names[] =
{
	"R0",  "R1",  "R2",  "R3",  "R4",  "R5",  "R6",  "R7",  "R8", "R9",  "R10",  "R11",  "R12",  "R13", "R14", "R15",/*"SP",  "RA",  "PC", */
  "CPSR",  "SPSR",  
	"cQ",  "cC",  "cV",  "cZ",  "cN", "cGE",
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
  "fpsr",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
        "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23", "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31", 
  "fpscr", "fpsid", "fpexc",
  "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
};
/* }}} */

/* An array of function names of which we now they behave different from calling conventions */
#ifndef THUMB_SUPPORT
static char *unbehaved_funs[] = {"__aeabi_uidiv","__aeabi_uidivmod","__aeabi_idiv","__aeabi_idivmod","__aeabi_llsr","__aeabi_lasr","__aeabi_llsl","__aeabi_dneg","__aeabi_dadd","__aeabi_drsub","__aeabi_dsub","__aeabi_f2d","__aeabi_i2d","__aeabi_l2d","__aeabi_ui2d","__aeabi_ul2d","__aeabi_ddiv","__aeabi_dmul","__aeabi_cdcmpeq","__aeabi_cdcmple","__aeabi_cdrcmple","__aeabi_dcmpeq","__aeabi_dcmpge","__aeabi_dcmpgt","__aeabi_dcmple","__aeabi_dcmplt","__aeabi_dcmpun","__aeabi_d2iz","__aeabi_d2uiz","__aeabi_d2f","__aeabi_fneg","__aeabi_fadd","__aeabi_frsub","__aeabi_fsub","__aeabi_i2f","__aeabi_l2f","__aeabi_ui2f","__aeabi_ul2f","__aeabi_fdiv","__aeabi_fmul","__aeabi_cfcmpeq","__aeabi_cfcmple","__aeabi_cfrcmple","__aeabi_fcmpeq","__aeabi_fcmpge","__aeabi_fcmpgt","__aeabi_fcmple","__aeabi_fcmplt","__aeabi_fcmpun","__aeabi_f2iz","__aeabi_f2uiz","__aeabi_lcmp","__aeabi_ulcmp","__aeabi_ldivmod","__aeabi_uldivmod","__aeabi_lmul","__aeabi_f2lz","__aeabi_d2lz","__aeabi_f2ulz","__aeabi_d2ulz","__aeabi_uread4","__aeabi_uread8","__aeabi_uwrite4","__aeabi_uwrite8", ""};
#else
static char *unbehaved_funs[] = {"_dcmple", "_dcmpge", "_fcmple", "_fcmpge", "__16_dcmpge", "__16_dcmple", "_ll_cmpge", "_ll_cmpu", "__16_ll_cmpge", "__16_ll_cmpu","_dadd","_drcmple","__aeabi_uidiv","__aeabi_uidivmod","__aeabi_idiv","__aeabi_idivmod","__aeabi_llsr","__aeabi_lasr","__aeabi_llsl","__aeabi_dneg","__aeabi_dadd","__aeabi_drsub","__aeabi_dsub","__aeabi_f2d","__aeabi_i2d","__aeabi_l2d","__aeabi_ui2d","__aeabi_ul2d","__aeabi_ddiv","__aeabi_dmul","__aeabi_cdcmpeq","__aeabi_cdcmple","__aeabi_cdrcmple","__aeabi_dcmpeq","__aeabi_dcmpge","__aeabi_dcmpgt","__aeabi_dcmple","__aeabi_dcmplt","__aeabi_dcmpun","__aeabi_d2iz","__aeabi_d2uiz","__aeabi_d2f","__aeabi_fneg","__aeabi_fadd","__aeabi_frsub","__aeabi_fsub","__aeabi_i2f","__aeabi_l2f","__aeabi_ui2f","__aeabi_ul2f","__aeabi_fdiv","__aeabi_fmul","__aeabi_cfcmpeq","__aeabi_cfcmple","__aeabi_cfrcmple","__aeabi_fcmpeq","__aeabi_fcmpge","__aeabi_fcmpgt","__aeabi_fcmple","__aeabi_fcmplt","__aeabi_fcmpun","__aeabi_f2iz","__aeabi_f2uiz","__aeabi_lcmp","__aeabi_ulcmp","__aeabi_ldivmod","__aeabi_uldivmod","__aeabi_lmul","__aeabi_f2lz","__aeabi_d2lz","__aeabi_f2ulz","__aeabi_d2ulz","__aeabi_uread4","__aeabi_uread8","__aeabi_uwrite4","__aeabi_uwrite8", ""};
#endif



static int counter = 0;
//#define DEBUGCOUNTER if(counter++ < diablosupport_options.debugcounter)
#define DEBUGCOUNTER

void ArmInsCleanupPlug(t_ins * ins)
{
  ArmInsCleanup(T_ARM_INS(ins));
}

void ArmInsMakeUncondBranchPlug(t_ins * ins)
{
  ArmInsMakeUncondBranch(T_ARM_INS(ins));
}

void ArmInsDupDynamicPlug(t_ins * target, t_ins * source)
{
  ArmInsDupDynamic(T_ARM_INS(target), T_ARM_INS(source));
}

t_bool ArmInsHasSideEffectPlug(t_ins * ins)
{
  return ArmInsHasSideEffect(T_ARM_INS(ins));
}

t_bool ArmInsIsCallPlug(t_ins * ins)
{
  return ArmInsIsCall(T_ARM_INS(ins));
}

t_bool ArmIsIndirectCallPlug(t_ins * ins)
{
  return ArmIsIndirectCall(T_ARM_INS(ins));
}

t_bool ArmIsControlflowPlug(t_ins * ins)
{
  return ArmIsControlflow(T_ARM_INS(ins));
}

t_tristate ArmIsSyscallExitPlug(t_ins * ins)
{
  return ArmIsSyscallExit(T_ARM_INS(ins));
}

void ArmInsMakeNoopPlug(t_ins * ins)
{
  ArmInsMakeNoop(T_ARM_INS(ins));
}

t_bool ArmCompareInstructionsPlug(t_ins *insa, t_ins *insb)
{
  return ArmCompareInstructions(T_ARM_INS(insa), T_ARM_INS(insb));
}

t_bool ArmInsIsUnconditionalBranchPlug(t_ins * ins)
{
  return ArmInsIsUnconditionalBranch(T_ARM_INS(ins));
}
t_bool ArmInsIsSystemPlug(t_ins * ins)
{
  return ArmInsIsSystemInstruction(T_ARM_INS(ins));
}

static t_bool ArmInsIsConditionalPlug(t_ins* ins)
{
  return ArmInsIsConditional(T_ARM_INS(ins));
}

/*! 
 * \todo Document
 *
 * \param ins
 *
 * \return t_bool
 */
/* ArmInsUnconditionalizer {{{ */
t_bool ArmInsUnconditionalizer(t_arm_ins * ins)
{
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
  {
    ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    ARM_INS_SET_REGS_USE(ins, ArmUsedRegisters(ins)); 
    ARM_INS_SET_REGS_DEF(ins, ArmDefinedRegisters(ins)); 
    ARM_INS_SET_ATTRIB(ins,     ARM_INS_ATTRIB(ins) & ~IF_CONDITIONAL);

    if ((ARM_INS_FLAGS(ins) & FL_THUMB) && (ARM_INS_CSIZE(ins) == 2) && !ArmIsThumb1Encodable(ins))
    {
      /* we have to be careful with 16-bit Thumb instructions:
       * some instructions must set the statusflags when they are uncondonditional.
       * If the statusflags are live before this instruction, we have to use the
       * 32-bit variant. */

      /* See if we can keep a 16-bit instruction by setting the S-flag */

      if (!ArmStatusFlagsLiveBefore(ins))
      {
        ASSERT(!(ARM_INS_FLAGS(ins) & FL_S), ("status flags were already set by @I", ins));
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);

        if (ArmIsThumb1Encodable(ins))
        {
          ARM_INS_SET_REGS_DEF(ins, ArmDefinedRegisters(ins));
          ARM_INS_SET_REGS_USE(ins, ArmUsedRegisters(ins));

          VERBOSE(1, ("this Thumb instruction now sets the status flags @I", ins));
          return TRUE;
        }

        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
      }

      /* take the 32-bit variant of this instruction */
      VERBOSE(1, ("changed Thumb instruction size from 2 to 4 @I", ins));
      ARM_INS_SET_CSIZE(ins, 4);
    }

    return TRUE;
  }
  else if (ARM_INS_OPCODE(ins)==ARM_T2CBZ || ARM_INS_OPCODE(ins)==ARM_T2CBNZ)
    {
      ArmInsMakeUncondThumbBranch(ins);
      return TRUE;
    }
  return FALSE;
}
/* }}} */


t_bool ArmInsUnconditionalizerPlug(t_ins * ins)
{
  return ArmInsUnconditionalizer(T_ARM_INS(ins));
}

t_ins * ArmAddJumpInstructionPlug(t_bbl * bbl)
{
  return T_INS(ArmAddJumpInstruction(bbl));
}

t_ins * ArmAddCallInstructionPlug(t_bbl * bbl)
{
  return T_INS(ArmAddCallInstruction(bbl));
}

void ArmComputeLiveRegsBeforeSwiPlug(t_regset * live_after_swi, t_ins * ins)
{
  ArmComputeLiveRegsBeforeSwi(live_after_swi,T_ARM_INS(ins));
}

t_bool ArmInsIsCopyPlug(t_ins * i_ins, t_reg * copy, t_reg * original)
{
  return ArmInsIsCopy(T_ARM_INS(i_ins),copy,original);
}

t_bool ArmInsIsInvariantPlug(t_ins *ins)
{
  return ArmInsIsInvariant(T_ARM_INS(ins));
}

t_bool ArmInsIsConstantProducerPlug(t_ins *ins) {
  return ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_CONSTANT_PRODUCER;
}

t_int64 ArmInsGetImmediatePlug(t_ins *ins) {
  return ARM_INS_IMMEDIATE(T_ARM_INS(ins));
}

void ArmInsSetImmediatePlug(t_ins *ins, t_int64 imm) {
  ARM_INS_SET_IMMEDIATE(T_ARM_INS(ins), imm);
}

/* Architecture Description */
t_architecture_description arm_description =
{
  /*! Size of an instruction: encoded minimal size */
  16,
  /*! Size of an instruction: encoded maximum size */
  32,
  /*! Size of an instruction: (mod size) used for bundles  */
  16, /* The mod size */
  /*! Size of an instruction: the number of instructions in a bundle */
  1,
  /*! Size of an instruction: size of the bundle template */ 
  0,
  /*! Size of an instruction: disassembled instruction size */
  sizeof(t_arm_ins),
  /*! \todo document */
  ADDRSIZE32,
  /*! The number of general purpose integer registers */
  16,
  /*! The number of general purpose floating point registers */
  8,
  /*! The number of predicate registers */
  5,
  /*! The number of branch registers */
  0,
  /*! The number of special registers */
  2,
#if MAX_REG_ITERATOR > 64

  /* all registers */   
  /* JENS - Added 16 bits on the right side of the last word to incorporate the 16 extra D-registers in NEON */
   {MAX_REG_ITERATOR, {0xffffffff,0xffffffff,0x000fffff}},
  /* int registers */   
   {MAX_REG_ITERATOR, {0x0000ffff,0,0}},
  /* float registers */   
   {MAX_REG_ITERATOR, {0xff000000,0xffffffff, 0x000fffff}},
  /* predicate registers */
   {MAX_REG_ITERATOR, {0x00fc0000,0,0}},
  /* callee saved registers */   
   {MAX_REG_ITERATOR, {0xf0002ff0,0xfffe0000,0x00000001}},
  /* callee may use registers */   
   {MAX_REG_ITERATOR, {0x0f00f00f,0x0001fffe,0x00000000}}, /* callee may use */
  /* callee may change        */   
   {MAX_REG_ITERATOR, {0x0fffd00f,0x0001fffe,0x000ffff2}}, /* callee may change */
   /* callee may return        */   
   {MAX_REG_ITERATOR, {0x0f00000f,0x0000001e,0}},/* may contain return value */
  /* always live              */   
   {MAX_REG_ITERATOR, {0x0000a000,0,0x00000000}},/* always live */
  /* registers prop over hell */   
   {MAX_REG_ITERATOR, {0,0,0}},
  /* const registers          */   
   {MAX_REG_ITERATOR, {0,0,0}},
  /* dead over call           */   
   {MAX_REG_ITERATOR, {0x00fc0000,0x00000000,0x00000002}}, /* Dead over call */
  /* link registers           */   
   {MAX_REG_ITERATOR, {0x00004000,0,0}},
   /* argument_regs */
   {MAX_REG_ITERATOR, {0x0000000f,0x0001fffe,0}},
   /* return_regs */
   {MAX_REG_ITERATOR, {0x0000000f,0x0000001e,0}}, 
   /* dyncall use registers */
   {MAX_REG_ITERATOR, {0,0,0}},
#elif MAX_REG_ITERATOR > 32
#error
   0x7fffffffLL,
   0x0000ffffLL,
   0x7f800000LL,
   0x007c0000LL,
   0x78002ff0LL, /* Callee saved */  
   0x0780f00fLL, /* callee may use */
   0x07ffd00fLL, /* callee may change */
   0x0780000fLL, /* may contain return value */
   0x00008000LL, /* always live (i.e. program counter) */
   0x0LL,
   0x0LL,
   0x7c0000LL,/* Dead over call */
   0x00004000LL,
   0x0000000fLL, /* argument_regs */
   0x0000000fLL, /* return_regs */
   0x00000000LL, /* dyncall use regs */
#else
#error
   0x7fffffff,
   0x0000ffff,
   0x7f800000,
   0x007c0000,
   0x78002ff0, /* Callee saved */  
   0x0780f00f, /* callee may use */
   0x07ffd00f, /* callee may change */
   0x0780000f, /* may contain return value */
   0x00008000, /* always live (i.e. program counter) */
   0x0,
   0x0,
   0x007c0000,/* Dead over call */
   0x00004000,/* Link register */
   0x0000000f, /* argument_regs */
   0x0000000f, /* return_regs */
   0x00000000, /* dyncall use regs */
#endif

   /*! The program counter */
   ARM_REG_R15,
   /*! An array containing the name of each register */
   names,
   /*! Callback to disassemble a section */
   ArmDisassembleSection,
   /*! Callback to assemble a section */
   ArmAssembleSection,
   /*! Callback to create a flowgraph for an object */
   ArmFlowgraph,
   /*! Callback to deflowgraph a section */
#ifdef THUMB_SUPPORT
   ThumbDeflowgraph,
#else
   ArmDeflowgraph, 
#endif
   /*! Callback to make addressproducers for a section */
   ArmMakeAddressProducers,
   ArmInsCleanupPlug,
   ArmInsMakeUncondBranchPlug,
   ArmInsDupDynamicPlug,
   ArmInsHasSideEffectPlug,
   /*! Returns true when instruction is a load */
   GenericInsIsLoad,
   /*! Returns true when instruction is a store */
   GenericInsIsStore,
   ArmInsIsCallPlug,
   ArmIsIndirectCallPlug,
   ArmInsIsConditionalPlug,
   ArmInsIsUnconditionalBranchPlug,
   ArmIsControlflowPlug,
   ArmInsIsSystemPlug,
   /*! Returns true when instruction is program exit */
   ArmIsSyscallExitPlug,		    
   ArmInsPrint,             /* Print Instruction */
   ArmInsMakeNoopPlug,          /* Make a noop from the instruction */
   ArmCompareInstructionsPlug,          /* */
   ArmInsUnconditionalizerPlug, /* */
   ArmParseFromStringAndInsertAt,
   ArmAddJumpInstructionPlug,
   ArmFunIsGlobal,
   /*! An array with names of unbehaved functions */
   unbehaved_funs,
   ArmComputeLiveRegsBeforeSwiPlug,
   ArmInsIsCopyPlug,		    /* */
   ArmModus,
   ArmInsIsInvariantPlug,
   ArmInsIsConstantProducerPlug,
   ArmInsGetImmediatePlug,
   ArmInsSetImmediatePlug,
   ArmAddCallInstructionPlug
};
/* \todo Clean this up and document */
#if MAX_REG_ITERATOR > 64
t_regset ARM_ALL_BUT_PC_AND_COND = {MAX_REG_ITERATOR,{0x7fff}};
#endif
/* vim: set shiftwidth=2 foldmethod=marker : */
