/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

char *Datatypes[] = {"",  "8", "16", "32", "64",  "I8","I16","I32","I64",  "S8","S16","S32","S64",  "U8","U16","U32","U64",  "illegal","F16","F32","F64",  "P8","P16","illegal","illegal"};

t_arm_addr_info * ArmAddrInfoNew()
{
  return Malloc(sizeof(t_arm_addr_info));
}

void ArmInsInit(t_arm_ins *ins)
{
  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins, 0);
  ARM_INS_SET_MULTIPLE(ins, RegsetNewFromUint32(0));
  ARM_INS_SET_FLAGS(ins, 0);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins, 0);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_NEONFLAGS(ins, 0);
  ARM_INS_SET_REGASCALAR(ins, 0);
  ARM_INS_SET_MULTIPLEALIGNMENT(ins, 0);
  ARM_INS_SET_MULTIPLESCALAR(ins, 0);
}


/*!
 * \todo Document
 *
 * \param ins
 *
 * \return t_bool
 */
/* ArmInsHasSideEffect {{{ */
t_bool ArmInsHasSideEffect(t_arm_ins * ins)
{
  if ((ARM_INS_TYPE(ins) == IT_STORE) || RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R15))
  {
    return TRUE;
  }
  if (ARM_INS_TYPE(ins) == IT_FLT_STORE)
  {
    return TRUE;
  }
  if (ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE)
  {
    return TRUE;
  }
  if (ARM_INS_TYPE(ins) == IT_SWI)
  {
    return TRUE;
  }
  if (ARM_INS_TYPE(ins) == IT_SYNC)
  {
    return TRUE;
  }
  if (ARM_INS_OPCODE(ins)==ARM_T2IT)
    return TRUE;

  if (ARM_INS_OPCODE(ins) == ARM_MSR &&
      ((ARM_INS_FLAGS(ins) & FL_SPSR) || (ARM_INS_FLAGS(ins) & FL_CONTROL)))
    /* writes to the cpsr that only change the status flags have no side effects */
    return TRUE;
  if (ARM_INS_OPCODE(ins) == ARM_MCR || ARM_INS_OPCODE(ins) == ARM_MRC ||
      ARM_INS_OPCODE(ins) == ARM_LDC || ARM_INS_OPCODE(ins) == ARM_STC ||
      ARM_INS_OPCODE(ins) == ARM_MCR2 || ARM_INS_OPCODE(ins) == ARM_MRC2 ||
      ARM_INS_OPCODE(ins) == ARM_LDC2 || ARM_INS_OPCODE(ins) == ARM_STC2 ||
      ARM_INS_OPCODE(ins) == ARM_MRRC || ARM_INS_OPCODE(ins) == ARM_MRRC2 ||
      ARM_INS_OPCODE(ins) == ARM_MCRR || ARM_INS_OPCODE(ins) == ARM_MCRR2)
    return TRUE;
  if (ARM_INS_OPCODE(ins) == ARM_PLD || ARM_INS_OPCODE(ins)==ARM_PLDW || ARM_INS_OPCODE(ins)==ARM_PLI)
    return TRUE;

  if (ARM_INS_OPCODE(ins) == ARM_SUB && ARM_INS_REGA(ins) == ARM_REG_R15 &&
      ARM_INS_REGB(ins) == ARM_REG_R15 && (ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ARM_INS_IMMEDIATE(ins) == 4)
  {
    /* flush the instruction pipeline: used in system-level code */
    return TRUE;
  }

  if (RegsetIn(ARM_INS_REGS_DEF(ins), ARM_REG_R13))
  {
    /* instructions that define the stack pointer have a side effect! */
    return TRUE;
  }

  if (ARM_INS_OPCODE(ins) == ARM_CPS || ARM_INS_OPCODE(ins) == ARM_CPSIE || ARM_INS_OPCODE(ins) == ARM_CPSID)
  {
    /* change processor state */
    return TRUE;
  }

  if (ARM_INS_OPCODE(ins) == ARM_SETEND)
  {
    /* change endianness */
    return TRUE;
  }

  return FALSE;
}
/* }}} */

/* {{{ */
t_bool ArmInsIsSystemInstruction (t_arm_ins *ins)
{
  if (ARM_INS_OPCODE(ins) == ARM_SUB && ARM_INS_REGA(ins) == ARM_REG_R15 &&
      ARM_INS_REGB(ins) == ARM_REG_R15 && (ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ARM_INS_IMMEDIATE(ins) == 4)
  {
    /* flush the instruction pipeline: used in system-level code */
    return TRUE;
  }

  if (ARM_INS_OPCODE(ins) == ARM_MSR &&
      ((ARM_INS_FLAGS(ins) & FL_SPSR) || (ARM_INS_FLAGS(ins) & FL_CONTROL)))
  {
    /* cpsr writes that only change the status flags have no side effects */
    return TRUE;
  }

  /* reads and writes from coprocessors: can be system instruction, but
   * possibly not. play safe and assume they are. */
  if (ARM_INS_OPCODE(ins) == ARM_MCR || ARM_INS_OPCODE(ins) == ARM_MRC ||
      ARM_INS_OPCODE(ins) == ARM_LDC || ARM_INS_OPCODE(ins) == ARM_STC ||
      ARM_INS_OPCODE(ins) == ARM_MCR2 || ARM_INS_OPCODE(ins) == ARM_MRC2 ||
      ARM_INS_OPCODE(ins) == ARM_LDC2 || ARM_INS_OPCODE(ins) == ARM_STC2 ||
      ARM_INS_OPCODE(ins) == ARM_MRRC || ARM_INS_OPCODE(ins) == ARM_MRRC2 ||
      ARM_INS_OPCODE(ins) == ARM_MCRR || ARM_INS_OPCODE(ins) == ARM_MCRR2)
    return TRUE;

  return FALSE;
}
/* }}} */

/*!
 * \todo Document
 *
 * \param ins
 *
 * \return void
 */
/* ArmInsMakeNoop {{{ */
void ArmInsMakeNoop(t_arm_ins * ins)
{
  /* Make a noop */
  if (ARM_INS_FLAGS(ins) & FL_THUMB) /* if it was originally thumb, keep it that way */
  {
    ARM_INS_SET_FLAGS(ins, FL_THUMB);
    ARM_INS_SET_CSIZE(ins, AddressNew32(2));
  }
  else
  {
    ARM_INS_SET_FLAGS(ins, 0x0);
    ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  }

  ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins,  ARM_MOV);

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    ARM_INS_SET_REGA(ins,  ARM_REG_R8);
  else
    ARM_INS_SET_REGA(ins,  ARM_REG_R0);

  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    ARM_INS_SET_REGC(ins,  ARM_REG_R8);
  else
    ARM_INS_SET_REGC(ins,  ARM_REG_R0);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_IMMEDIATE(ins,  0);
  ARM_INS_SET_REGS_USE(ins,  NullRegs);
  ARM_INS_SET_REGS_DEF(ins,  NullRegs);
  ARM_INS_SET_ATTRIB(ins,  0);

  /* If this instruction pointed to something else (e.g. it produced an
   * address, or jumped, or ....) then it no longer does that. So we can remove
   * the reloc (and all associated structures). */
  RelocatableRemoveAllRefersTo(T_RELOCATABLE(ins));
}

/* This function sets some default values when making a new instruction. It is to be called after
 * regs A, B and C have been set (because of the ArmUsed/ArmDefined functions).
 */
static void ArmInsDefault(t_arm_ins * ins, t_uint32 cond)
{
  /* Set condition and assorted flag */
  ARM_INS_SET_CONDITION(ins, cond);
  if (cond != ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) | IF_CONDITIONAL);

  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins, 0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGS_USE(ins, ArmUsedRegisters(ins));
  ARM_INS_SET_REGS_DEF(ins, ArmDefinedRegisters(ins));

  /* If this instruction pointed to something else (e.g. it produced an
   * address, or jumped, or ....) then it no longer does that. So we can remove
   * the reloc (and all associated structures). */
  RelocatableRemoveAllRefersTo(T_RELOCATABLE(ins));
}

/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 *
 * \return void
*/
/* ArmInsMakeUncondThumbBranch {{{ */
void ArmInsMakeUncondThumbBranch(t_arm_ins * ins)
{
  /* Create a 32-bit Thumb branch by default.
   * These 32-bit instructions can later be translated to
   * 16-bit instructions when possible. */
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
  ARM_INS_SET_OPCODE(ins,  ARM_B);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  /* When calculating this initial displacement, take into account the fact
   * that Thumb-instructions can be 16- or 32-bits wide. So, do not use
   * BBL_NINS here. */
  ARM_INS_SET_IMMEDIATE(ins,  (t_int64)((t_int32)(G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) - (G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) + BBL_CSIZE(ARM_INS_BBL(ins)) - 4) - 4)));
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED | FL_THUMB);
  ArmInsDefault(ins, ARM_CONDITION_AL);  

  ARM_INS_SET_REGS_USE(ins, NullRegs);
  ARM_INS_SET_REGS_DEF(ins, NullRegs);
  ARM_INS_SET_REGS_DEF(ins, RegsetAddReg(ARM_INS_REGS_DEF(ins),ARM_REG_R15));
  ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins)| IF_PCDEF);
  ARM_INS_SET_CSIZE(ins,  (diabloarm_options.fullthumb2) ? AddressNew32(4) : AddressNew32(2));
}

void ArmInsMakeIT(t_arm_ins * ins, t_uint32 spec)
{
  ARM_INS_SET_TYPE(ins, IT_SYNC);
  ARM_INS_SET_IMMEDIATE(ins, spec);

  ARM_INS_SET_OPCODE(ins, ARM_T2IT);
  ARM_INS_SET_THUMBOPCODE(ins, TH_IT);

  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0);
  ARM_INS_SET_FLAGS(ins, FL_THUMB);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins, 0);

  ARM_INS_SET_CSIZE(ins, AddressNew32(2));
  ARM_INS_SET_OLD_SIZE(ins, AddressNew32(2));
}

void ArmInsMakeCondBranchExchange(t_arm_ins * ins, t_uint32 cond, t_reg reg_b)
{
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
  ARM_INS_SET_OPCODE(ins,  ARM_BX);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  reg_b);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
    ARM_INS_SET_CSIZE(ins,  (diabloarm_options.fullthumb2) ? AddressNew32(4) : AddressNew32(2));
  ArmInsDefault(ins, cond);
}

/*!
 *
 * \todo Document
 *
 * \param ins
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeCondBranchAndLink {{{ */
void ArmInsMakeCondBranchAndLink(t_arm_ins * ins, t_uint32 cond)
{
  /* Make a branch and link */
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
  ARM_INS_SET_OPCODE(ins,  ARM_BL);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) - (G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) + BBL_NINS(ARM_INS_BBL(ins)) * 4 - 4) - 8);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) | IF_PCDEF);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  ArmInsDefault(ins, cond);
}

void ArmInsMakeCondBranchLinkAndExchange(t_arm_ins * ins, t_uint32 cond, t_reg reg_b)
{
  ArmInsMakeCondBranchAndLink(ins, cond);

  ARM_INS_SET_OPCODE(ins, ARM_BLX);
  ARM_INS_SET_REGB(ins, reg_b);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & (~ FL_IMMED) ); /* Ensure FL_IMMED is off */
}

/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeCondBranch {{{ */
void ArmInsMakeCondBranch(t_arm_ins * ins, t_uint32 cond)
{
  /* Make a branch */
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
  ARM_INS_SET_OPCODE(ins,  ARM_B);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) - (G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) + BBL_NINS(ARM_INS_BBL(ins)) * 4 - 4) - 8);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
    ARM_INS_SET_CSIZE(ins,  (diabloarm_options.fullthumb2) ? AddressNew32(4) : AddressNew32(2));
  ArmInsDefault(ins, cond);
}

void ArmInsConvertToBranch(t_arm_ins * ins)
{
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);
  ARM_INS_SET_OPCODE(ins,  ARM_B);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) - (G_T_UINT32(BBL_CADDRESS(ARM_INS_BBL(ins))) + BBL_NINS(ARM_INS_BBL(ins)) * 4 - 4) - 8);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  ArmInsDefault(ins, ARM_INS_CONDITION(ins));
}

/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regB
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeLdr {{{ */
void ArmInsMakeLdr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up, t_bool wb)
{
  ARM_INS_SET_TYPE(ins,  IT_LOAD);
  ARM_INS_SET_OPCODE(ins,  ARM_LDR);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);

  ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) & ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);
  if (pre) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
  if (up) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
  if (wb) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ArmInsDefault(ins, cond);

  ARM_INS_SET_REGS_DEF(ins, RegsetEmpty());
  ARM_INS_SET_REGS_DEF(ins, RegsetAddReg(ARM_INS_REGS_DEF(ins),regA));
  if(!pre || wb)
    ARM_INS_SET_REGS_DEF(ins, RegsetAddReg(ARM_INS_REGS_DEF(ins),regB));

  ARM_INS_SET_REGS_USE(ins,ArmUsedRegisters(ins));

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    ARM_INS_SET_CSIZE(ins,  (diabloarm_options.fullthumb2) ? AddressNew32(4) : AddressNew32(2));
  else
    ARM_INS_SET_CSIZE(ins, AddressNew32(4));
}

void ArmInsMakeVldr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up)
{
  ArmInsMakeLdr(ins, regA, regB, regC, immed, cond, pre, up, FALSE);
  ARM_INS_SET_TYPE(ins, IT_FLT_LOAD);
  ARM_INS_SET_OPCODE(ins, ARM_VLDR);
}

/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regB
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeStr {{{ */
void ArmInsMakeStr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up, t_bool wb)
{
  ARM_INS_SET_TYPE(ins,  IT_STORE);
  ARM_INS_SET_OPCODE(ins,  ARM_STR);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);

  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);
  if (pre) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
  if (up) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
  if (wb) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ArmInsDefault(ins, cond);

  ARM_INS_SET_REGS_DEF(ins, RegsetEmpty());
  if(!pre || wb)
    ARM_INS_SET_REGS_DEF(ins, RegsetAddReg(ARM_INS_REGS_DEF(ins),regB));

  ARM_INS_SET_REGS_USE(ins,ArmUsedRegisters(ins));

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
}

void ArmInsMakeVstr(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond, t_bool pre, t_bool up)
{
  ArmInsMakeStr(ins, regA, regB, regC, immed, cond, pre, up, FALSE);
  ARM_INS_SET_TYPE(ins, IT_FLT_STORE);
  ARM_INS_SET_OPCODE(ins, ARM_VSTR);
}

/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regB
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeCmp {{{ */
void ArmInsMakeCmp(t_arm_ins * ins, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins,  ARM_CMP);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);

  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */


/* ArmInsMakeCmp {{{ */
void ArmInsMakeTst(t_arm_ins * ins, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ArmInsMakeCmp(ins, regB, regC, immed, cond);
  ARM_INS_SET_OPCODE(ins,  ARM_TST);
}
/* }}} */

void ArmInsMakeClz(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins, IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins, ARM_CLZ);
  ARM_INS_SET_REGA(ins, regA);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, regC);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}

/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regB
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeAdd {{{ */
void ArmInsMakeAdd(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins,  ARM_ADD);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);

  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */

void ArmInsMakeMul(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_MUL);
  ARM_INS_SET_OPCODE(ins,  ARM_MUL);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
    ARM_INS_SET_CSIZE(ins,  AddressNew32(2));
  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ArmInsDefault(ins, cond);
}

/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regB
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeSub {{{ */
void ArmInsMakeSub(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins,  ARM_SUB);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);

  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param regC
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeMsr {{{ */
void ArmInsMakeMsr(t_arm_ins * ins, t_reg regC, t_uint32 cond, t_bool set)
{
  ARM_INS_SET_TYPE(ins,  IT_STATUS);
  ARM_INS_SET_OPCODE(ins,  ARM_MSR);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins, 0);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX &~FL_S);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));

  if (set)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_S|FL_STATUS);

  ArmInsDefault(ins, cond);
}

void ArmInsMakeVmsr(t_arm_ins * ins, t_reg regB, t_uint32 cond)
{
  ArmInsMakeMsr(ins, ARM_REG_NONE, cond, TRUE);
  ARM_INS_SET_REGB(ins, regB);
  ARM_INS_SET_OPCODE(ins, ARM_VMSR);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | (NEONFL_A_SINGLE | NEONFL_B_CORE));
}

/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeMrs {{{ */
void ArmInsMakeMrs(t_arm_ins * ins, t_reg regA, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_STATUS);
  ARM_INS_SET_OPCODE(ins,  ARM_MRS);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins, 0);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX &~FL_S);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  ArmInsDefault(ins, cond);
}

void ArmInsMakeVmrs(t_arm_ins * ins, t_reg regA, t_uint32 cond)
{
  ArmInsMakeMrs(ins, regA, cond);
  ARM_INS_SET_OPCODE(ins, ARM_VMRS);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | (NEONFL_A_CORE | NEONFL_B_CORE));
}

/* }}} */
/*ArmInsMakePush {{{ */
void ArmInsMakePush(t_arm_ins * ins, t_uint32 regs, t_uint32 cond, t_bool thumb)
{
  /* for thumb, push needs minimally one register; for ARM, two */
  if ( !thumb || ((regs & (regs - 1)) != 0) )
  {
    if (thumb)
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_THUMB);
    ArmInsMakeStm(ins, ARM_REG_R13, regs, cond, TRUE, TRUE, FALSE); /* STMDB SP!, <registers>*/
  }
  else
  {
    /* extract register from regs */
    t_reg reg;
    REGSET_FOREACH_REG(arm_description.int_registers, reg)
      if (regs & (1 << reg))
        break;
    ArmInsMakeStr(ins, reg, ARM_REG_R13, ARM_REG_NONE, 4, cond, TRUE, FALSE, TRUE);
  }
}
/*}}}*/

/*ArmInsMakeStm {{{ */
void ArmInsMakeStm(t_arm_ins * ins, t_reg regB, t_uint32 regs, t_uint32 cond, t_bool writeback, t_bool preindex, t_bool dirup)
{
  ARM_INS_SET_TYPE(ins,  IT_STORE_MULTIPLE);
  ARM_INS_SET_OPCODE(ins,  ARM_STM);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  regs);

  /* Set flags */
  if (writeback) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_WRITEBACK);
  if (preindex) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_PREINDEX);
  if (dirup) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_DIRUP);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */
/*ArmInsMakePop {{{ */
void ArmInsMakePop(t_arm_ins * ins, t_uint32 regs, t_uint32 cond, t_bool thumb)
{
  /* for thumb, pop needs minimally one register; for ARM, two */
  if ( !thumb || ((regs & (regs - 1)) != 0) )
    {
      if (thumb)
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_THUMB);
      ArmInsMakeLdm(ins, ARM_REG_R13, regs, cond, TRUE, FALSE, TRUE); /* LDM SP!, <registers> */
    }
  else
  {
    /* extract register from regs */
    t_reg reg;
    REGSET_FOREACH_REG(arm_description.int_registers, reg)
      if (regs & (1 << reg))
        break;
    ArmInsMakeLdr(ins, reg, ARM_REG_R13, ARM_REG_NONE, 4, cond, FALSE /* pre */, TRUE /* up */, TRUE /* wb */);
  }
}
/*}}}*/

/*ArmInsMakeLdm {{{ */
void ArmInsMakeLdm(t_arm_ins * ins, t_reg regB, t_uint32 regs, t_uint32 cond, t_bool writeback, t_bool preindex, t_bool dirup)
{
  ARM_INS_SET_TYPE(ins,  IT_STORE_MULTIPLE);
  ARM_INS_SET_OPCODE(ins,  ARM_LDM);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  regs);

  /* Set flags */
  if (writeback) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_WRITEBACK);
  if (preindex) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_PREINDEX);
  if (dirup) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
  else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)&~FL_DIRUP);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */
/* ArmInsMakeSwi{{{ */
void ArmInsMakeSwi(t_arm_ins * ins, t_uint32 immediate, t_uint32 cond)
{
  ARM_INS_SET_OPCODE(ins,  ARM_SWI);
  ARM_INS_SET_TYPE(ins,  IT_SWI);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  immediate);
  ARM_INS_SET_FLAGS(ins,  FL_IMMED);
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
    ARM_INS_SET_CSIZE(ins,  AddressNew32(2));
  ArmInsDefault(ins, cond);
}
/* }}} */

void ArmInsMakeBkpt(t_arm_ins * ins)
{
  ARM_INS_SET_OPCODE(ins, ARM_BKPT);
  ARM_INS_SET_TYPE(ins, IT_SWI);
  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_CSIZE(ins, AddressNew32((ARM_INS_FLAGS(ins) & FL_THUMB) ? 2 : 4));

  /* BKPT can't be conditional */
  ArmInsDefault(ins, ARM_CONDITION_AL);
}

/*!
 * \todo Document
 *
 * \param f
 * \param lnno
 * \param ins
 * \param immed
 * \param rel
 *
 * \return void
 */
/* ArmMakeAddressProducer {{{ */
void ArmMakeAddressProducer(const char * f, t_uint32 lnno, t_arm_ins * ins, t_uint32 immed, t_reloc * rel)
{
  ARM_INS_SET_OPCODE(ins, ARM_ADDRESS_PRODUCER);
  /* The produced register stays the same, as do the conditionflags */
  ARM_INS_SET_REGB(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_REGC(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_TYPE(ins,  IT_CONSTS);
  ARM_INS_SET_IMMEDIATE(ins, immed);
  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
  {
    /* Thumb */
    if (immed <= (0xff << 2) && ARM_INS_REGA(ins) <= ARM_REG_R7 && !(immed & 0x3))
      ARM_INS_SET_CSIZE(ins, AddressNew32(2));
    else
      ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  }
  BBL_SET_ATTRIB(ARM_INS_BBL(ins),  BBL_ATTRIB(ARM_INS_BBL(ins))  |  BBL_ADDR_PROD);
}
/* }}} */
/* ArmMakePseudoCall {{{ */
void ArmInsMakePseudoCall(t_arm_ins * ins, t_function * fun)
{
  ARM_INS_SET_OPCODE(ins, ARM_PSEUDO_CALL);
  /* The produced register stays the same, as do the conditionflags */
  ARM_INS_SET_REGA(ins, ARM_REG_R0); /* The previous operands are useless now */
  ARM_INS_SET_REGB(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_REGC(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_TYPE(ins,  IT_CALL);
  ARM_INS_SET_IMMEDIATE(ins, 0);
  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  ARM_INS_SET_DATA(T_ARM_INS(ins), (void*)fun);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param immed
 *
 * \return void
 */
/* ArmMakeConstantProducer {{{ */
void  ArmMakeConstantProducer(t_arm_ins * ins, t_uint32 immed)
{
  ARM_INS_SET_OPCODE(ins, ARM_CONSTANT_PRODUCER);
  /* The produced register stays the same, as do the conditionflags */
  ARM_INS_SET_REGB(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_REGC(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_TYPE(ins,  IT_CONSTS);
  ARM_INS_SET_IMMEDIATE(ins, immed);
  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
  {
    /* Thumb */
    if (immed <= 0xff && ARM_INS_REGA(ins) <= ARM_REG_R7)
      ARM_INS_SET_CSIZE(ins, AddressNew32(2));
    else
      ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  }

  RelocatableRemoveAllRefersTo(T_RELOCATABLE(ins));
}
/* }}} */
/*!
 * \todo Document
 *
 *
 * \param ins
 * \param data
 *
 * \return void
 */
/* ArmMakeFloatProducer {{{ */
void  ArmMakeFloatProducer(t_arm_ins * ins, char * data)
{
  if (ARM_INS_OPCODE(ins) == ARM_LDF)
    ARM_INS_SET_OPCODE(ins, ARM_FLOAT_PRODUCER);
  else
    ARM_INS_SET_OPCODE(ins, ARM_VFPFLOAT_PRODUCER);
  /* The produced register stays the same, as do the conditionflags */
  ARM_INS_SET_REGB(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_REGC(ins, ARM_REG_NONE); /* The previous operands are useless now */
  ARM_INS_SET_TYPE(ins,  IT_CONSTS);
  if (ARM_INS_REFERS_TO(ins)) Free(ARM_INS_REFERS_TO(ins));
  ARM_INS_SET_REFERS_TO(ins, NULL);
  ARM_INS_SET_DATA(ins, data);
  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeMov {{{ */
void ArmInsMakeMov(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_OPCODE(ins,  ARM_MOV);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  0);

   if (ARM_INS_REGC(ins) != ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_IMMED);
   else
   {
     ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
     ARM_INS_SET_IMMEDIATE(ins,  immed);
   }
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~(FL_DIRUP | FL_WRITEBACK | FL_PREINDEX));
  if (ARM_INS_REGA(ins) == ARM_REG_R15)
    ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);

  ArmInsDefault(ins, cond);

  if(ARM_INS_REGA(ins) != ARM_INS_REGC(ins) || (ARM_INS_FLAGS(ins)| FL_IMMED))
  {
    ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
    ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  }
  else
  {
    ARM_INS_SET_REGS_DEF(ins,  NullRegs);
    ARM_INS_SET_REGS_USE(ins,  NullRegs);
  }

  if (ARM_INS_FLAGS(ins) & FL_THUMB)
    ARM_INS_SET_CSIZE(ins,  (diabloarm_options.fullthumb2) ? AddressNew32(4) : AddressNew32(2));
  else
    ARM_INS_SET_CSIZE(ins, AddressNew32(4));
}
/* }}} */

/* ArmInsMakeMov {{{ */
void ArmInsMakeMovwImmed(t_arm_ins * ins, t_reg regA, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_OPCODE(ins,  ARM_MOVW);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMEDW);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~(FL_DIRUP | FL_WRITEBACK | FL_PREINDEX | FL_IMMED));
  if (ARM_INS_REGA(ins) == ARM_REG_R15)
    ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);

  ArmInsDefault(ins, cond);
}
/* }}} */

/* ArmInsMakeMvn {{{ */
void ArmInsMakeMvn(t_arm_ins * ins, t_reg regA, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_OPCODE(ins,  ARM_MVN);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  0);
   if (ARM_INS_REGC(ins) != ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_IMMED);
   else
   {
     ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
     ARM_INS_SET_IMMEDIATE(ins,  immed);
   }
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~(FL_DIRUP | FL_WRITEBACK | FL_PREINDEX));

  ArmInsDefault(ins, cond);

  if (ARM_INS_REGA(ins) == ARM_REG_R15)
    ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) | IF_PCDEF);
  if(ARM_INS_REGA(ins) != ARM_INS_REGC(ins) || (ARM_INS_FLAGS(ins)| FL_IMMED))
  {
    ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
    ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
  }
  else
  {
    ARM_INS_SET_REGS_DEF(ins,  NullRegs);
    ARM_INS_SET_REGS_USE(ins,  NullRegs);
  }

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
}
/*!
 * \todo Document
 *
 * \param ins
 * \param regA
 * \param regC
 * \param immed
 * \param cond
 *
 * \return void
 */
/* ArmInsMakeAnd {{{ */
void ArmInsMakeAnd(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 immed, t_uint32 cond)
{
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_OPCODE(ins,  ARM_AND);
  ARM_INS_SET_REGA(ins,  regA);
  ARM_INS_SET_REGB(ins,  regB);
  ARM_INS_SET_REGC(ins,  regC);
  ARM_INS_SET_IMMEDIATE(ins,  immed);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)& ~FL_DIRUP & ~FL_WRITEBACK & ~FL_PREINDEX);
  if (regC == ARM_REG_NONE)
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ArmInsDefault(ins, cond);
}
/* }}} */

void ArmInsMakeSwp(t_arm_ins * ins, t_reg regA, t_reg regB, t_reg regC, t_uint32 cond)
{
  ARM_INS_SET_OPCODE(ins,  ARM_SWP);
  ARM_INS_SET_TYPE(ins,  IT_SWAP);
  ARM_INS_SET_ATTRIB(ins,  0x0);
  ARM_INS_SET_CSIZE(ins,  AddressNew32(4));

  ASSERT(! (ARM_INS_FLAGS(ins) & FL_THUMB), ("I don't think this is supported by thumb? Check!"));

  ARM_INS_SET_IMMEDIATE(ins,  0);
  ARM_INS_SET_FLAGS(ins,  0x0);

  ARM_INS_SET_REGA(ins, regA);
  ARM_INS_SET_REGB(ins, regB);
  ARM_INS_SET_REGC(ins, regC);
  ArmInsDefault(ins, cond);
}

/*!
 * \todo Document
 *
 * \param i1
 * \param i2
 *
 * \return t_bool
 */
/* ArmCompareInstructions {{{ */
t_bool ArmCompareInstructionsNoConditions(t_arm_ins *insa, t_arm_ins *insb)
{
  if (ARM_INS_OPCODE(insa) != ARM_INS_OPCODE(insb)) return FALSE;
  if (ARM_INS_FLAGS(insa) != ARM_INS_FLAGS(insb)) return FALSE;

  if (ARM_INS_REGA(insa) != ARM_INS_REGA(insb)) return FALSE;
  if (ARM_INS_REGB(insa) != ARM_INS_REGB(insb)) return FALSE;
  if (ARM_INS_REGC(insa) != ARM_INS_REGC(insb)) return FALSE;
  if (ARM_INS_REGS(insa) != ARM_INS_REGS(insb)) return FALSE;
  if (ARM_INS_SHIFTTYPE(insa) != ARM_INS_SHIFTTYPE(insb)) return FALSE;

  if (ARM_INS_SHIFTTYPE(insa) == ARM_SHIFT_TYPE_ASR_IMM ||
      ARM_INS_SHIFTTYPE(insa) == ARM_SHIFT_TYPE_LSL_IMM ||
      ARM_INS_SHIFTTYPE(insa) == ARM_SHIFT_TYPE_LSR_IMM ||
      ARM_INS_SHIFTTYPE(insa) == ARM_SHIFT_TYPE_ROR_IMM)
    if (ARM_INS_SHIFTLENGTH(insa) != ARM_INS_SHIFTLENGTH(insb))
      return FALSE;

  if (!RegsetEquals(ARM_INS_MULTIPLE(insa), ARM_INS_MULTIPLE(insb))) return FALSE;

  if (ARM_INS_OPCODE(insa)>=ARM_SIMD2REGSSCALAR_FIRST && ARM_INS_OPCODE(insa)<=ARM_SIMD2REGSSCALAR_LAST)
	 {
		if (ARM_INS_MULTIPLESCALAR(insa) != ARM_INS_MULTIPLESCALAR(insb)) return FALSE;
		if (ARM_INS_REGASCALAR(insa) != ARM_INS_REGASCALAR(insb)) return FALSE;
		if (ARM_INS_REGBSCALAR(insa) != ARM_INS_REGBSCALAR(insb)) return FALSE;
		if (ARM_INS_REGCSCALAR(insa) != ARM_INS_REGCSCALAR(insb)) return FALSE;
	 }

  if (ARM_INS_OPCODE(insa)==ARM_SSAT || ARM_INS_OPCODE(insa)==ARM_SSAT16 ||
      ARM_INS_OPCODE(insa)==ARM_USAT || ARM_INS_OPCODE(insa)==ARM_USAT16)
    if (ARM_INS_IMMEDIATE(insa)!=ARM_INS_IMMEDIATE(insb))
      return FALSE;

  if (ARM_INS_OPCODE(insa)>=ARM_BITFIELD_FIRST && ARM_INS_OPCODE(insa)<=ARM_BITFIELD_LAST)
    if (ARM_INS_IMMEDIATE(insa)!=ARM_INS_IMMEDIATE(insb))
      return FALSE;

  if (ARM_INS_MULTIPLEALIGNMENT(insa) != ARM_INS_MULTIPLEALIGNMENT(insb)) return FALSE;

  if (ARM_INS_REFERS_TO(insa) && ARM_INS_REFERS_TO(insb))
  {
    if (RelocCmp(RELOC_REF_RELOC(ARM_INS_REFERS_TO(insa)),
          RELOC_REF_RELOC(ARM_INS_REFERS_TO(insb)), FALSE) != 0)
      return FALSE;
  }
  else if (ARM_INS_REFERS_TO(insa) || ARM_INS_REFERS_TO(insb))
  {
      return FALSE;
  }
  else
  {
    if (
	(
	 (ARM_INS_FLAGS(insa) & FL_IMMED) ||
	 (ARM_INS_FLAGS(insa) & FL_IMMEDW) ||
	 (ARM_INS_OPCODE(insa) == ARM_STM) ||
	 (ARM_INS_OPCODE(insa) == ARM_CONSTANT_PRODUCER) ||
	 (ARM_INS_OPCODE(insa) == ARM_LDM)
	 )
	&&
	(
	 ARM_INS_OPCODE(insa) != ARM_B &&
	 ARM_INS_OPCODE(insa) != ARM_BL
	 )
	)
      if (ARM_INS_IMMEDIATE(insa) != ARM_INS_IMMEDIATE(insb))
	return FALSE;
  }

  if (ARM_INS_OPCODE(insa)==ARM_FLOAT_PRODUCER)
    {
      int size = 4;

      if ((ARM_INS_FLAGS(insa) & (FL_FLT_DOUBLE|FL_FLT_DOUBLE_EXTENDED|FL_FLT_PACKED)) != (ARM_INS_FLAGS(insb) & (FL_FLT_DOUBLE|FL_FLT_DOUBLE_EXTENDED|FL_FLT_PACKED)))
	return FALSE;

      if ((ARM_INS_FLAGS(insa)&FL_FLT_DOUBLE) || (ARM_INS_FLAGS(insa)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(insa)&FL_FLT_PACKED))
	size += 4;
      if ((ARM_INS_FLAGS(insa)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(insa)&FL_FLT_PACKED))
	size += 4;

      if (memcmp(ARM_INS_DATA(T_ARM_INS(insa)),ARM_INS_DATA(T_ARM_INS(insb)),size))
	return FALSE;
    }

  if (ARM_INS_DATATYPEOP(insa)!=ARM_INS_DATATYPEOP(insb))
        return FALSE;

  if (ARM_INS_DATATYPE(insa)!=ARM_INS_DATATYPE(insb))
    return FALSE;

  if (ARM_INS_OPCODE(insa)==ARM_VFPFLOAT_PRODUCER)
    if (ARM_INS_DATA(insa)!=ARM_INS_DATA(insb))
      return FALSE;

  return TRUE;
}

t_bool ArmCompareInstructions(t_arm_ins *insa, t_arm_ins *insb)
{
  if (ARM_INS_CONDITION(insa) != ARM_INS_CONDITION(insb)) return FALSE;
  return ArmCompareInstructionsNoConditions(insa, insb);
}

t_bool ArmCompareInstructionsOppositeCond(t_arm_ins * i1, t_arm_ins * i2)
{
  if (ARM_INS_CONDITION(i1) != ArmInvertCondition(ARM_INS_CONDITION(i2))) return FALSE;
  if (ARM_INS_OPCODE(i1)==ARM_B) return FALSE;
  return ArmCompareInstructionsNoConditions(i1, i2);
}
/* }}} */

t_tristate ArmIsSyscallExit(t_arm_ins * ins)
{
  if(ARM_INS_TYPE(ins) != IT_SWI) return NO;
  if((ARM_INS_OPCODE(ins) == ARM_SWI) &&
     (ARM_INS_IMMEDIATE(ins) == 0x900001))
     return YES;
  if(((ARM_INS_OPCODE(ins) == ARM_SWI) &&
      (ARM_INS_IMMEDIATE(ins) == 0x123456)) ||
     ((ARM_INS_OPCODE(ins) == ARM_BKPT) &&
      (ARM_INS_IMMEDIATE(ins) == 0xab)))
  {
    /* ARM ADS syscalls, we have to look for a MOV r0,0x18 instruction */
    t_arm_ins * prev = ARM_INS_IPREV(ins);
    while(prev)
    {
      if(RegsetIn(ARM_INS_REGS_DEF(prev),ARM_REG_R0))
      {
        if(ARM_INS_OPCODE(prev) == ARM_MOV && ARM_INS_REGA(prev) == ARM_REG_R0 && ARM_INS_FLAGS(prev) & FL_IMMED)
	{
	  if(ARM_INS_IMMEDIATE(prev) == 0x18) return YES;
	  else return NO;
	}
	else if(ARM_INS_OPCODE(prev) == ARM_CONSTANT_PRODUCER && ARM_INS_REGA(prev) == ARM_REG_R0 && ARM_INS_FLAGS(prev) & FL_IMMED)
	{
	  if(ARM_INS_IMMEDIATE(prev) == 0x18) return YES;
	  else return NO;
	}
	else
	{
	  WARNING(("Unusual way of setting the syscall number by @I",prev));
	  return PERHAPS;
	}
      }
      else
	prev = ARM_INS_IPREV(prev);
    }

  }
  return NO;
}

t_bool ArmInsUnconditionalize(t_arm_ins * ins)
{
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    {
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
      ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & (~IF_CONDITIONAL));
      return TRUE;
    }

  if (ARM_INS_OPCODE(ins)==ARM_T2CBZ || ARM_INS_OPCODE(ins)==ARM_T2CBNZ)
    {
      DEBUG(("1 make cb(n)z unconditional:",ins));
      ArmInsMakeUncondThumbBranch(ins);
      DEBUG(("2 make cb(n)z unconditional:",ins));
      return TRUE;
    }

  ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & (~IF_CONDITIONAL));
  return FALSE;
}

t_bool ArmIsControlflow(t_arm_ins * ins)
{
  if(ARM_INS_TYPE(ins) == IT_BRANCH) return TRUE;
  if(ARM_INS_TYPE(ins) == IT_SWI) return TRUE;
  if(RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R15)) return TRUE;
  return FALSE;
}

t_bool ArmIsIndirectCall(t_arm_ins * ins)
{
  if(ARM_INS_OPCODE(ins) == ARM_BL)
    return FALSE;

  if(ARM_INS_OPCODE(ins) == ARM_MOV && ARM_INS_REGA(ins) == ARM_REG_R15)
    return TRUE;

  FATAL(("IMPLEMENT ME @I",ins));
  return TRUE;
}

t_bool ArmIsBranchImmediate(t_arm_ins * ins)
{
  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_B:
  case ARM_BL:
  case ARM_T2CBZ:
  case ARM_T2CBNZ:
    return TRUE;

  case ARM_BLX:
    if (ARM_INS_FLAGS(ins) & FL_IMMED)
      return TRUE;
    break;

  default:
    break;
  }

  return FALSE;
}

void ArmInsMakeData(t_arm_ins * ins, t_uint32 data)
{
  ARM_INS_SET_TYPE(ins, IT_DATA);
  ARM_INS_SET_OPCODE(ins,  ARM_DATA);
  ARM_INS_SET_IMMEDIATE(ins, data);
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    ARM_INS_SET_CSIZE(ins,  AddressNew32(4));
  else
    ARM_INS_SET_CSIZE(ins,  AddressNew32(2));

  ARM_INS_SET_REGS_USE(ins,  NullRegs);
  ARM_INS_SET_REGS_DEF(ins,  NullRegs);
}

void ArmInsCleanup(t_arm_ins * ins)
{
  if(ARM_INS_DATA(ins))
  {
    Free(ARM_INS_DATA(ins));
    ARM_INS_SET_DATA(ins,  NULL);
  }
  if(ARM_INS_INFO(ins))
  {
    Free(ARM_INS_INFO(ins));
    ARM_INS_SET_INFO(ins,  NULL);
  }
}

void ArmInsDupDynamic(t_arm_ins * target, t_arm_ins * source)
{
  if(ARM_INS_INFO(T_ARM_INS(source)))
    {
      ARM_INS_SET_INFO(T_ARM_INS(target),  ArmAddrInfoNew());

      memcpy(ARM_INS_INFO(T_ARM_INS(target)),ARM_INS_INFO(T_ARM_INS(source)),sizeof(t_arm_addr_info));
    }

  if(ARM_INS_DATA(T_ARM_INS(source)))
    {
      int size = 4;
      if ((ARM_INS_FLAGS(source)&FL_VFP_DOUBLE) || (ARM_INS_FLAGS(source)&FL_FLT_DOUBLE) || (ARM_INS_FLAGS(source)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(source)&FL_FLT_PACKED))
	size += 4;
      if ((ARM_INS_FLAGS(source)&FL_FLT_DOUBLE_EXTENDED) || (ARM_INS_FLAGS(source)&FL_FLT_PACKED))
	size += 4;
      ARM_INS_SET_DATA(T_ARM_INS(target),  (char*) Malloc(size));
      memcpy(ARM_INS_DATA(T_ARM_INS(target)),ARM_INS_DATA(T_ARM_INS(source)),size);
    }
}

EXEC_CONDITION ArmInsCondition(t_arm_ins * ins)
{
  switch (ARM_INS_CONDITION(ins))
    {
    case ARM_CONDITION_EQ :
      return EQ;
    case ARM_CONDITION_NE :
      return NE;
    case ARM_CONDITION_GE :
      return GE;
    case ARM_CONDITION_LT :
      return LT;
    case ARM_CONDITION_GT :
      return GT;
    case ARM_CONDITION_LE :
      return LE;
    case ARM_CONDITION_AL:
      return ALWAYS;
    case ARM_CONDITION_NV :
      return NEVER;
    default:
      return UNKNOWN ;

    }
  return UNKNOWN;
}

t_arm_ins * ArmAddJumpInstruction(t_bbl * bbl)
{
  t_arm_ins * ins = ArmInsNewForBbl(bbl);
  ArmInsMakeUncondBranch(ins);
  return ins;
}


t_bool ArmInsIsCall(t_arm_ins * ins)
{
  return ARM_INS_OPCODE(ins) == ARM_BL;
}

t_bool ArmInsIsUnconditionalBranch(t_arm_ins * ins)
{
  return ((ARM_INS_OPCODE(ins) == ARM_B)&&(ARM_INS_CONDITION(ins)==ARM_CONDITION_AL));
}


void ArmComputeLiveRegsBeforeSwi(t_regset * live_after_swi, t_arm_ins * ins)
{
  switch (ARM_INS_IMMEDIATE(ins))
    {
    case 0x900005:
    case 0x900004:
    case 0x900003:
      RegsetSetSubReg(*live_after_swi,ARM_REG_R3);
      break;
    default:
      break;
    }
  return;
}

t_arm_ins_itcond ArmInsExtractITCondition(t_arm_ins * ins)
{
  t_uint32 mask = ARM_INS_IMMEDIATE(ins) & 0x0000000f;
  t_uint32 bit = ((ARM_INS_IMMEDIATE(ins) & 0x000000f0) >> 4) & 1;

  t_arm_ins_itcond c = ITCOND_OMIT;

  switch(mask)
  {
    case 8:
      c = ITCOND_OMIT;
      break;

    case 4:
      c = (bit) ? ITCOND_E : ITCOND_T;
      break;
    case 12:
      c = (bit) ? ITCOND_T : ITCOND_E;
      break;

    case 2:
      c = (bit) ? ITCOND_EE : ITCOND_TT;
      break;
    case 14:
      c = (bit) ? ITCOND_TT : ITCOND_EE;
      break;

    case 6:
      c = (bit) ? ITCOND_ET : ITCOND_TE;
      break;
    case 10:
      c = (bit) ? ITCOND_TE : ITCOND_ET;
      break;

    case 1:
      c = (bit) ? ITCOND_EEE : ITCOND_TTT;
      break;
    case 15:
      c = (bit) ? ITCOND_TTT : ITCOND_EEE;
      break;

    case 7:
      c = (bit) ? ITCOND_ETT : ITCOND_TEE;
      break;
    case 9:
      c = (bit) ? ITCOND_TEE : ITCOND_ETT;
      break;

    case 5:
      c = (bit) ? ITCOND_ETE : ITCOND_TET;
      break;
    case 11:
      c = (bit) ? ITCOND_TET : ITCOND_ETE;
      break;

    case 3:
      c = (bit) ? ITCOND_EET : ITCOND_TTE;
      break;
    case 13:
      c = (bit) ? ITCOND_TTE : ITCOND_EET;
      break;

    default:
      FATAL(("invalid IT condition encoding (%d)", mask));
  }

  return c;
}

t_uint32 ArmInsITBlockSize(t_arm_ins * it_instruction)
{
  t_arm_ins_itcond itcond;

  if (ARM_INS_OPCODE(it_instruction) != ARM_T2IT)
    return 0;

  itcond = ArmInsExtractITCondition(it_instruction);

  if (itcond == ITCOND_OMIT)
    return 1;
  if (itcond <= ITNUMINS_2_LAST)
    return 2;
  if (itcond <= ITNUMINS_3_LAST)
    return 3;
  if (itcond <= ITNUMINS_4_LAST)
    return 4;

  return 0;
}

/* Find the owning IT instruction for the given instruction.
    If this instruction is not part of an IT-block, NULL is returned.
    Otherwise, a pointer to the owning IT-instruction is returned.
 */
t_arm_ins * ArmInsFindOwningIT(t_arm_ins * ins, t_bool * is_last_ins_in_block, t_uint32 * ins_idx)
{
  t_arm_ins * found_it_instruction = NULL;
  t_arm_ins * tmp = NULL;
  t_arm_ins_itcond itcond;
  int num_prev;

  if (is_last_ins_in_block)
    *is_last_ins_in_block = FALSE;
  if (ins_idx)
    *ins_idx = -1;

  if (ARM_INS_OPCODE(ins) == ARM_T2IT)
  {
    /* the given instruction is an IT instruction, and thus
        can never be in an IT block */
    return NULL;
  }

  /* find a possible IT instruction preceding the given instruction */
  tmp = ARM_INS_IPREV(ins);
  for (num_prev = 1; (num_prev <= MAX_INS_IN_IT) && tmp && (ARM_INS_FLAGS(tmp) & FL_THUMB); num_prev++)
  {
    if (ARM_INS_OPCODE(tmp) == ARM_T2IT)
    {
      found_it_instruction = tmp;
      break;
    }

    tmp = ARM_INS_IPREV(tmp);
  }

  if (found_it_instruction)
  {
    /* If we have found an IT-instruction, check if the given instruction is located in
        the IT-block that it defines.

        In order to check this, extract the condition code from the IT-instruction
        and calculate the size of the IT-block that it defines. */
    itcond = ArmInsExtractITCondition(found_it_instruction);

    if (ins_idx)
      *ins_idx = num_prev - 1;

    /* 1-instruction IT-block */
    if ((itcond == ITCOND_OMIT) && (num_prev == 1))
    {
      if (is_last_ins_in_block)
        *is_last_ins_in_block = TRUE;

      return found_it_instruction;
    }

    /* 2-instruction IT-block */
    if ((ITNUMINS_2_FIRST <= itcond) && (itcond <= ITNUMINS_2_LAST) && (num_prev <= 2))
    {
      if (is_last_ins_in_block)
        *is_last_ins_in_block = (num_prev == 2);

      return found_it_instruction;
    }

    /* 3-instruction IT-block */
    if ((ITNUMINS_3_FIRST <= itcond) && (itcond <= ITNUMINS_3_LAST) && (num_prev <= 3))
    {
      if (is_last_ins_in_block)
        *is_last_ins_in_block = (num_prev == 3);

      return found_it_instruction;
    }

    /* 4-instruction IT-block */
    if ((ITNUMINS_4_FIRST <= itcond) && (itcond <= ITNUMINS_4_LAST) && (num_prev <= 4))
    {
      if (is_last_ins_in_block)
        *is_last_ins_in_block = (num_prev == 4);

      return found_it_instruction;
    }
  }

  return NULL;
}

t_arm_condition_code ArmInsThumbConditionCode(t_arm_ins * th_instruction, t_bool * is_last_ins_in_block, t_arm_ins ** ret_owning_it)
{
  t_uint32 ins_idx;
  t_arm_condition_code cond = ARM_CONDITION_NV;

  t_arm_ins * owning_it = ArmInsFindOwningIT(th_instruction, is_last_ins_in_block, &ins_idx);
  if (ret_owning_it)
    *ret_owning_it = owning_it;

  if (owning_it)
  {
    t_arm_condition_code firstcond = ARM_INS_IMMEDIATE(owning_it) >> 4;
    t_arm_ins_itcond itcond = ArmInsExtractITCondition(owning_it);

    if (ins_idx == 0)
    {
      cond = firstcond;
    }
    else
    {
      /* the lowest bit is determined by the IT condition and the position of the
       instruction inside the IT block */
      cond = (firstcond & 0xe) | ((ARM_INS_IMMEDIATE(owning_it) >> (4 - ins_idx)) & 1);
    }
  }

  return cond;
}

t_bool ArmInsIsValidThumbConditionCode(t_arm_ins *ins, t_bool * is_last_ins_in_block, t_arm_ins ** owning_it)
{
  t_bool valid_condition_code = TRUE;

  /* by default, only branch instructions can be conditional in Thumb */
  if ((ARM_INS_OPCODE(ins) != ARM_B) && ArmInsIsConditional(ins))
    valid_condition_code = FALSE;

  if (!valid_condition_code)
  {
    /* maybe this instruction is part of an IT block?
       If so, the instruction can be conditional, but only according to
       the condition codes specified by the owning IT instruction! */
    t_arm_condition_code itcond = ArmInsThumbConditionCode(ins, is_last_ins_in_block, owning_it);

    if (itcond == ARM_CONDITION_NV)
    {
      /* ARM_CONDITION_NV means that this instruction is not inside an IT block */
      valid_condition_code = FALSE;
    }
    else
    {
      /* an owning IT instruction is found, compare the condition code we
         expect when looking at the IT instruction with the one specified
         in the instruction datastructure */
      valid_condition_code = (itcond == ARM_INS_CONDITION(ins));
    }
  }

  return valid_condition_code;
}

void InstructionSetRegsQuadDouble(t_arm_ins * ins, t_bool is_quad)
{
  if(is_quad)
  {
    ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD | NEONFL_C_QUAD));
  }
  else
  {
    ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
  }
}

void ArmInsSetITSpecifics(t_arm_ins * first_instruction)
{
  t_arm_ins * ins = first_instruction;
  t_arm_ins * owning_it = NULL;
  t_uint32 it_ins_left = 0, it_ins_size = 0;
  t_arm_condition_code it_firstcond = ARM_CONDITION_AL;

  while (ins)
  {
    if (ARM_INS_FLAGS(ins) & FL_THUMB)
    {
      /* only check Thumb instructions */
      if (ARM_INS_OPCODE(ins) == ARM_T2IT)
      {
        /* only valid if we are not in an IT block yet */
        ASSERT(it_ins_left == 0, ("can not enter a new IT block when inside an IT block @I, %d instructions left", ins, it_ins_left));

        owning_it = ins;
        it_ins_left = ArmInsITBlockSize(ins);
        it_ins_size = it_ins_left;
        it_firstcond = (ARM_INS_IMMEDIATE(ins) >> 4) & 0xf;
      }
      else
      {
        if (it_ins_left > 0)
        {
          /* condition codes for instructions inside IT-blocks are handled in a separate function,
              ArmProcessITBlocks, for the time being */
          // /* determine the condition code of this instruction, which depends on its
          //     location inside the IT-block. The condition code for the first instruction
          //     in the IT-block is encoded in the immediate of the owning IT instruction. */
          // t_arm_condition_code cond = it_firstcond;

          // if (it_ins_size != it_ins_left)
          // {
          //   /* this is not the first instruction in the IT block; the lowest bit of the
          //       condition code is determined by the position of the current instruction
          //       inside the IT-block */
          //   cond = (it_firstcond & 0xe) | ((ARM_INS_IMMEDIATE(owning_it) >> (4 - (it_ins_size - it_ins_left))) & 1);
          // }

          /* instruction specifics for instructions INSIDE IT blocks */
          switch (ARM_INS_OPCODE(ins))
          {
          case ARM_ADC:
          case ARM_AND:
          case ARM_BIC:
          case ARM_EOR:
          case ARM_MVN:
          case ARM_ORR:
          case ARM_SBC:
            /* only the 16-bit register variant never sets the status flags */
            if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
                !(ARM_INS_FLAGS(ins) & FL_PREFER32))
              ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~(FL_S));
            break;

          case ARM_MOV:
            /* some shift types do NOT have a specific case when used INSIDE an IT-block */
            if ((ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ROR_IMM) ||
                (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_RRX))
              break;

          case ARM_ADD:
          case ARM_MUL:
          case ARM_SUB:
            if (!(ARM_INS_FLAGS(ins) & FL_PREFER32))
              ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~(FL_S));
            break;

          case ARM_RSB:
            if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
                !(ARM_INS_FLAGS(ins) & FL_PREFER32))
              ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~(FL_S));
            break;

          default:
            break;
          }

          /* one instruction in this IT-block less to process */
          it_ins_left--;
        }

        /* The instruction disassembler assumes that no IT-blocks exist.
            As such, only settings specifically for instructions INSIDE IT-blocks
            should be modified. So there is no ELSE-case here. */
      }
    }
    else
    {
      ASSERT(it_ins_left == 0, ("switch to ARM instructions, but the IT block is not completed (IT instruction @I, %d instructions left)", owning_it, it_ins_left));
    }

    ins = ARM_INS_INEXT(ins);
  }

  ASSERT(it_ins_left == 0, ("all instructions are processed, but the IT block is not completed (IT instruction @I, %d instructions left)", owning_it, it_ins_left));
}

t_bool ArmInsThumbITDoesNotSetFlags(t_arm_ins * ins, t_bool it_generated)
{
  t_bool does_not_set_flags = FALSE;

  if (!diabloarm_options.fullthumb2)
    return TRUE;

  if (!it_generated)
    return TRUE;

  if (ArmInsFindOwningIT(ins, NULL, NULL) != NULL)
  {
    /* Instructions that do not set the flags, but are allowed in IT-blocks */
    switch(ARM_INS_OPCODE(ins))
    {
    case ARM_ADC:
    case ARM_AND:
    case ARM_BIC:
    case ARM_EOR:
    case ARM_MVN:
    case ARM_ORR:
    case ARM_SBC:
      if (!(ARM_INS_FLAGS(ins) & FL_IMMED))
        does_not_set_flags = TRUE;
      break;

    case ARM_ADD:
    case ARM_MUL:
    case ARM_SUB:
      does_not_set_flags = TRUE;
      break;

    case ARM_RSB:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
        does_not_set_flags = TRUE;
      break;

    case ARM_MOV:
      if ((ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_ROR_IMM) &&
          (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_RRX))
        does_not_set_flags = TRUE;
      break;

    default:
      break;
    }
  }

  return does_not_set_flags;
}

t_bool ArmInsIsInITBlock(t_arm_ins * ins)
{
  return ArmInsFindOwningIT(ins, NULL, NULL) != NULL;
}

t_bool ArmIsStubFromThumb(t_bbl *bbl)
{
  /* The linker-generated from-Thumb stubs consist of 3 BBL's,
   * which contain the following instructions:
   *  t BX r15
   *  t NOP
   *    B <destination>
   * Where each instruction resides in its own BBL.
   */
  if (!(bbl && BBL_CSIZE(bbl)==2 && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl)))==TH_BX_R15))
    return FALSE;

  bbl = BBL_NEXT(bbl);

  if (!(bbl && BBL_CSIZE(bbl)==2 && ArmInsIsNOOP(T_ARM_INS(BBL_INS_FIRST(bbl)))))
    return FALSE;

  bbl = BBL_NEXT(bbl);

  if (!(bbl && BBL_CSIZE(bbl)==4 && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl)))==ARM_B))
    return FALSE;

  /* all tests succeeded, this is a FromThumb stub */
  return TRUE;
}

void ArmInsertThumbITInstructions(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_arm_ins * i_ins;
  t_uint32 nr_ins_in_block = 0;
  t_uint32 mask = 0;
  t_uint32 nr_it_created = 0;
  t_uint32 nr_ins_to32 = 0;

  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    nr_ins_in_block = 0;

    BBL_FOREACH_ARM_INS(i_bbl, i_ins)
    {
      /* skip the instructions that are now part of an IT-block */
      if (nr_ins_in_block > 1)
      {
        nr_ins_in_block--;
        continue;
      }

      /* only look at Thumb instructions here */
      if (!(ARM_INS_FLAGS(i_ins) & FL_THUMB)) continue;

      /* in Thumb, only the branch instruction (B and B.W) can be conditional
       * outside an IT-block */
      if (ARM_INS_OPCODE(i_ins) == ARM_B && ArmInsIsConditional(i_ins)) continue;

      /* instructions inside IT-blocks are conditional, by definition */
      if (!ArmInsIsConditional(i_ins) && !ArmInsMustBeInITBlock(i_ins)) continue;

      /* If we are here, we HAVE to create an IT-instruction wich
       * defines an IT-block with at least one instruction (i_ins). */
      nr_ins_in_block = 1;
      mask = 0;
      t_arm_ins * tmp = ARM_INS_INEXT(i_ins);
      t_arm_ins * last_in_it = i_ins;
      t_uint32 i = 0;

      /* try to make the IT-blocks as large as possible,
       * but only if this instruction (i_ins) can be followed by
       * another one in the same IT-block */
      if (tmp && !ArmInsMustBeLastInITBlock(i_ins))
      {
        if (!ArmInsIsConditional(i_ins))
        {
          /* i_ins is unconditionally executed */
          while (tmp && (nr_ins_in_block < MAX_INS_IN_IT))
          {
            if (ArmInsIsConditional(tmp) ||
                !ArmInsIsValidInITBlock(tmp))
              break;

            nr_ins_in_block++;

            /* we have added 'tmp' to the IT-block */
            last_in_it = tmp;

            if (ArmInsMustBeLastInITBlock(tmp))
              break;

            tmp = ARM_INS_INEXT(tmp);
          }
        }
        else
        {
          /* i_ins is conditionally executed, we need both
           * the condition code of i_ins and the inverse condition code. */
          t_arm_condition_code c1 = ARM_INS_CONDITION(i_ins);
          t_arm_condition_code c2 = c1 ^ 1;

          while (tmp && (nr_ins_in_block < MAX_INS_IN_IT))
          {
            if (!ArmInsIsConditional(tmp) ||
                (!(ARM_INS_CONDITION(tmp) == c1) && !(ARM_INS_CONDITION(tmp) == c2)) ||
                !ArmInsIsValidInITBlock(tmp))
              break;

            nr_ins_in_block++;

            /* we have added 'tmp' to the IT-block */
            last_in_it = tmp;

            if (ArmInsMustBeLastInITBlock(tmp))
              break;

            tmp = ARM_INS_INEXT(tmp);
          }
        }
      }
      else if (!tmp)
      {
        /* we have added 'i_ins' to the IT-block */
        last_in_it = i_ins;
      }

      /* Now the IT-block is as large as it can possibly get.
       * However, this may not be an optimal solution. Try to
       * remove every last instruction out of the IT-block, in
       * order to prevent that instructions are not inside an
       * IT-block when it isn't necessary. */
      while (nr_ins_in_block > 0)
      {
        /* look at the last instruction in the IT-block */
        if ((ARM_INS_OPCODE(last_in_it) == ARM_B && ArmInsIsConditional(last_in_it))
            || (!ArmInsIsConditional(last_in_it) && !ArmInsMustBeInITBlock(last_in_it)))
        {
          /* remove this instruction from the IT-block,
           * as it does not have to be in a block */
          nr_ins_in_block--;
          last_in_it = ARM_INS_IPREV(last_in_it);
        }
        else break;
      }
      ASSERT(nr_ins_in_block > 0, ("empty IT-block for @I!\n    @eiB", i_ins, ARM_INS_BBL(i_ins)));

      /* If only one instruction is left in the IT-block, and if it is an unconditional instruction,
       * try to make it a 32-bit variant of the instruction. This way, we can prevent unnecessary
       * IT-instructions from being generated. */
      if (nr_ins_in_block == 1
          && !ArmInsIsConditional(i_ins))
      {
        if (ArmIsThumb2EncodableCheck(i_ins, FALSE))
        {
          VERBOSE(3, ("converting @I", i_ins));
          ARM_INS_SET_CSIZE(i_ins, 4);
          VERBOSE(3, ("        to @I (so it does not have to be in an IT-block anymore)", i_ins));
          nr_ins_in_block = 0;
          nr_ins_to32++;
        }
      }

      /* Only create an IT-block if there are still instructions left to be put in there. */
      if (nr_ins_in_block > 0)
      {
        if (nr_ins_in_block > 1)
        {
          /* Construct the mask which contains the condition bits;
           * abuse the last_in_it variable here */

          /* CAVEAT: Do not consider the first instruction in the block here! */
          last_in_it = ARM_INS_INEXT(i_ins);
          for (i = 1; i < nr_ins_in_block; i++)
          {
            mask |= (ARM_INS_CONDITION(last_in_it) & 0x1) << (4 - i);

            /* go to the next instruction */
            last_in_it = ARM_INS_INEXT(last_in_it);
          }
        }

        /* the condition code of the first instruction in the IT-block */
        mask |= ARM_INS_CONDITION(i_ins) << 4;

        /* the always-on bit */
        mask |= 1 << (4 - nr_ins_in_block);

        /* create the IT-instruction */
        t_arm_ins * new_ins = ArmInsNewForBbl(i_bbl);
        ArmInsMakeIT(new_ins, mask);
        ArmInsInsertBefore(new_ins, i_ins);

        VERBOSE(3, ("created IT-instruction (mask 0x%x) for %d instructions @I", mask, nr_ins_in_block, new_ins));
        nr_it_created++;
      }
    }
  }

  VERBOSE(0, ("Inserted %d IT-instructions and converted %d instructions to 32-bit", nr_it_created, nr_ins_to32));
}

t_bool ArmStatusFlagsLiveBefore(t_arm_ins * ins)
{
  t_regset condition_regs = RegsetNew();
  RegsetSetAddReg(condition_regs,ARM_REG_Z_CONDITION);
  RegsetSetAddReg(condition_regs,ARM_REG_N_CONDITION);
  RegsetSetAddReg(condition_regs,ARM_REG_C_CONDITION);
  RegsetSetAddReg(condition_regs,ARM_REG_V_CONDITION);

  return !RegsetIsEmpty(RegsetIntersect(ArmInsRegsLiveAfter(ins), condition_regs));
}

// t_bool ArmInsMinimizeThumbSize(t_arm_ins * ins)
// {
//   /* Maybe there is nothing to do ... */
//   if (!(ARM_INS_FLAGS(ins) & FL_THUMB)) return TRUE;

//   if (ArmIsThumb1Encodable(ins))
//   {
//     ARM_INS_SET_CSIZE(ins, AddressNew32(2));
//     return TRUE;
//   }

//   /* some Thumb instructions are only encodable in 16-bits
//    * when they set the S-flag. Try to do this here. */
//   if (!(ARM_INS_FLAGS(ins) & FL_S) && !ArmStatusFlagsLiveBefore(ins))
//   {
//     ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);

//     if (ArmIsThumb1EncodableCheck(ins, FALSE))
//     {
//       ARM_INS_SET_CSIZE(ins, AddressNew32(2));
//       return TRUE;
//     }

//     /* even when setting the S-flags, this instruction is still
//      * not encodable in 16-bit. Undo our changes. */
//     ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
//   }

//   /* No more options left, set the instruction size to 4 */
//   if (ArmIsThumb2EncodableCheck(ins, FALSE))
//   {
//     ARM_INS_SET_CSIZE(ins, AddressNew32(4));
//     return TRUE;
//   }

//   return FALSE;
// }

void ArmInsLoadStoreMultipleToSingle(t_arm_ins * i_ins)
{
  t_bool is_stack_op = (ARM_INS_OPCODE(i_ins)==ARM_LDM
                            && ARM_INS_REGB(i_ins)==ARM_REG_R13
                            && ((ARM_INS_FLAGS(i_ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_DIRUP | FL_WRITEBACK)))
                        || (ARM_INS_OPCODE(i_ins)==ARM_STM
                            && ARM_INS_REGB(i_ins)==ARM_REG_R13
                            && ((ARM_INS_FLAGS(i_ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_PREINDEX | FL_WRITEBACK)));

  t_reg loaded_reg = ARM_REG_NONE;
  t_uint32 i = 0;

  t_regset regset = RegsetNewFromUint32(ARM_INS_IMMEDIATE(i_ins));
  t_uint32 nregs = RegsetCountRegs(regset);
  ASSERT(nregs>0, ("regset of LDM/STM can't be empty! @I", i_ins));

  if (nregs > 1) return;

  /* get the one and only register in this regset */
  REGSET_FOREACH_REG(regset,loaded_reg)
    break;

  /* convert this LDM/STM to a single load with writeback */
  VERBOSE(3, ("Converting LDM/STM (stack operation? %d) with one register in its regset @I", is_stack_op, i_ins));
  t_bool p = ARM_INS_FLAGS(i_ins) & FL_PREINDEX;
  t_bool w = ARM_INS_FLAGS(i_ins) & FL_WRITEBACK;
  t_bool u = ARM_INS_FLAGS(i_ins) & FL_DIRUP;

  if (ARM_INS_OPCODE(i_ins) == ARM_LDM)
  {

        if (is_stack_op)
        {
                if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
                {
                        p = FALSE; w = TRUE; u = TRUE;
                }
                else
                {
                        p = FALSE; w = FALSE; u = TRUE;
                }
        }
        else
        {
                if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
                        w = (p && u) ? TRUE : w;
                else
                        p = (!p && w) ? TRUE : p;
        }
    ArmInsMakeLdr(i_ins,
                  loaded_reg,
                  ARM_INS_REGB(i_ins),
                  (is_stack_op) ? ARM_REG_NONE : ARM_INS_REGC(i_ins),
                  (w || is_stack_op) ? 4 : ARM_INS_IMMEDIATE(i_ins),
                  ARM_INS_CONDITION(i_ins),
                  p, u, w);
  }
  else
  {
        if (is_stack_op)
        {
                p = TRUE;
                u = FALSE;
                w = TRUE;
        }
        else
        {
                if (ARM_INS_FLAGS(i_ins) & FL_THUMB)
                        w = (p && u) ? TRUE : w;
                else
                        p = (!p && w) ? TRUE : p;
        }
    ArmInsMakeStr(i_ins,
                  loaded_reg,
                  ARM_INS_REGB(i_ins),
                  (is_stack_op) ? ARM_REG_NONE : ARM_INS_REGC(i_ins),
                  (w || is_stack_op) ? 4 : ARM_INS_IMMEDIATE(i_ins),
                  ARM_INS_CONDITION(i_ins),
                  p, u, w);
  }
  VERBOSE(3, ("           to @I", i_ins));

  ASSERT(ArmInsIsEncodable(i_ins), ("Instruction can't be encoded @I", i_ins));
}
/* vim: set shiftwidth=2 foldmethod=marker : */
