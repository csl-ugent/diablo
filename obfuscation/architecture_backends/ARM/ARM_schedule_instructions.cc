/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The functions here are adopted (i.e., *mostly* copy-pasted) from diabloarm_instructions.c */
#include "ARM_obfuscations.h"
#include "ARM_schedule_instructions.h"

void ShouldKeepInsCombination(t_ins *last_ins, t_ins *prev_ins, bool *result) {
  *result = false;

  t_arm_ins *a = T_ARM_INS(last_ins);
  t_arm_ins *b = T_ARM_INS(prev_ins);

  if (ARM_INS_OPCODE(a) == ARM_SUB
      && ARM_INS_REGA(a) == ARM_REG_R15

      && ARM_INS_OPCODE(b) == ARM_MOV
      && ARM_INS_REGC(b) == ARM_REG_R15) {
    *result = true;
  }
}

bool ARMScheduleInstructionsTransformation::modifiesStackPointer(t_ins* ins) const {
  if (RegsetIn(ARM_INS_REGS_DEF(T_ARM_INS(ins)), ARM_REG_R13)) {
    return true;
  }
  return false;
}

bool ARMScheduleInstructionsTransformation::hasSideEffects(t_ins* ins_) const {
  t_arm_ins* ins = T_ARM_INS(ins_);

  // in diabloarm_instructions this also checks for (ARM_INS_TYPE(ins) == IT_STORE), ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE, and modifiesStackPointer
  if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R15))
  {
    return true;
  }
  if (ARM_INS_TYPE(ins) == IT_FLT_STORE)
  {
    return true;
  }
  if (ARM_INS_TYPE(ins) == IT_SWI)
  {
    return true;
  }
  if (ARM_INS_TYPE(ins) == IT_SYNC)
  {
    return true;
  }
  if (ARM_INS_OPCODE(ins) == ARM_MSR &&
      ((ARM_INS_FLAGS(ins) & FL_SPSR) || (ARM_INS_FLAGS(ins) & FL_CONTROL)))
    /* writes to the cpsr that only change the status flags have no side effects */
    return true;
  if (ARM_INS_OPCODE(ins) == ARM_MCR || ARM_INS_OPCODE(ins) == ARM_MRC ||
      ARM_INS_OPCODE(ins) == ARM_LDC || ARM_INS_OPCODE(ins) == ARM_STC ||
      ARM_INS_OPCODE(ins) == ARM_MCR2 || ARM_INS_OPCODE(ins) == ARM_MRC2 ||
      ARM_INS_OPCODE(ins) == ARM_LDC2 || ARM_INS_OPCODE(ins) == ARM_STC2 ||
      ARM_INS_OPCODE(ins) == ARM_MRRC || ARM_INS_OPCODE(ins) == ARM_MRRC2 ||
      ARM_INS_OPCODE(ins) == ARM_MCRR || ARM_INS_OPCODE(ins) == ARM_MCRR2)
    return true;
  if (ARM_INS_OPCODE(ins) == ARM_PLD || ARM_INS_OPCODE(ins)==ARM_PLDW || ARM_INS_OPCODE(ins)==ARM_PLI)
    return true;

  if (ARM_INS_OPCODE(ins) == ARM_SUB && ARM_INS_REGA(ins) == ARM_REG_R15 &&
      ARM_INS_REGB(ins) == ARM_REG_R15 && (ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ARM_INS_IMMEDIATE(ins) == 4)
  {
    /* flush the instruction pipeline: used in system-level code */
    return true;
  }

  if (ARM_INS_OPCODE(ins) == ARM_CPS || ARM_INS_OPCODE(ins) == ARM_CPSIE || ARM_INS_OPCODE(ins) == ARM_CPSID)
  {
    /* change processor state */
    return true;
  }

  if (ARM_INS_OPCODE(ins) == ARM_SETEND)
  {
    /* change endianness */
    return true;
  }

  return false;
}
