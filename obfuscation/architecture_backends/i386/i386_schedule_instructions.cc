/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

extern "C" {
#include "diabloi386.h"
}

#include "i386_schedule_instructions.h"

/* The functions here are adopted (i.e., *mostly* copy-pasted) from diabloarm_instructions.c */

bool I386ScheduleInstructionsTransformation::modifiesStackPointer(t_ins* ins) const {
  if (RegsetIn(I386_INS_REGS_DEF(T_I386_INS(ins)), I386_REG_ESP))
    return true;
  return false;
}

bool I386ScheduleInstructionsTransformation::hasSideEffects(t_ins* ins_) const {
  t_i386_ins* ins = T_I386_INS(ins_);

  /* in diabloi386_instructions this also checks for:
   *   if (I386InsIsStore(ins)) if (I386InsIsControlTransfer(ins)) and modifiesStackPointer */

  if (I386InsIsSystemInstruction(ins))
    return true;

  /* some special cases */
  switch (I386_INS_OPCODE(ins))
  {
    case I386_IN:
    case I386_INSB:
    case I386_INSD:
    case I386_OUT:
    case I386_OUTSB:
    case I386_OUTSD:
    case I386_LEAVE:
    case I386_LDS:
    case I386_LES:
    case I386_LFS:
    case I386_LGS:
    case I386_LSS:
    case I386_FLDCW:
    case I386_FSTCW:
    case I386_FLDENV:
    case I386_FSTENV:
    case I386_FSAVE:
    case I386_FRSTOR:
    case I386_STMXCSR:
    case I386_LDMXCSR:
    case I386_PREFETCH_NTA:
    case I386_PREFETCH_T0:
    case I386_PREFETCH_T1:
    case I386_PREFETCH_T2:
      return true;
    default:
      break;
  }

  /* all instructions that push/pop the fpu stack have a side effect
   * (they change the top-of-stack pointer, which is an invisible and
   * unmodeled register) */
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops ||
      i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pushes)
    return true;

  return false;
} /* }}} */
