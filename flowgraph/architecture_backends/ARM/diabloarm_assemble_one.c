/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */


   #include <diabloarm.h>
/* Defines (utility functions) {{{ */
#define ASM_COND(_cc)          (_cc << 28)
#define ASM_IMM24(_imm)        ((_imm >> 2) & 0x00ffffff)
#define ASM_HAS_IMMEDIATE      (0x1 << 25)
#define ASM_LOADSTORE_WITH_REG (0x1 << 25)
#define ASM_UPDATES_CC         (0x1 << 20)
#define ASM_REGA(_reg)         (_reg << 12)
#define ASM_REGB(_reg)         (_reg << 16)
#define ASM_REGC(_reg)         (_reg)
#define ASM_REGS(_reg)         (_reg << 8)
#define ASM_SHREG_TYPE(_sh)    (((_sh - 4) << 5) | 0x00000010)
#define ASM_SHIMM_TYPE(_sh)    (_sh << 5)
#define ASM_SHIFT_IMM(imm)     ((imm & 0x1f) << 7)     /* amount to shift if there is a flexible   */
#define ASM_IMM_SHAMOUNT(_sh)  ((_sh / 2) << 8)        /* amount to shift if there is an immediate */
#define ASM_PREINDEX           (0x1 << 24)
#define ASM_DIRUP              (0x1 << 23)
#define ASM_WRITEBACK          (0x1 << 21)
#define ASM_VFP_FN(_reg)       (((((_reg)-ARM_REG_S0) & 0x1e) << 15) | ((((_reg)-ARM_REG_S0) & 0x1) <<  7))
#define ASM_VFP_FD(_reg)       (((((_reg)-ARM_REG_S0) & 0x1e) << 11) | ((((_reg)-ARM_REG_S0) & 0x1) << 22))
#define ASM_VFP_FM(_reg)       (((((_reg)-ARM_REG_S0) & 0x1e) >>  1) | ((((_reg)-ARM_REG_S0) & 0x1) <<  5))
#define ASM_VFP_SYSREG(_reg)   (((_reg)==ARM_REG_FPSID?0:(_reg)==ARM_REG_FPSCR?1:(_reg)==ARM_REG_FPEXC?8:-1)<<16)

#define ARM_ASSEMBLER_VERBOSITY_LEVEL 10

/* }}} */
/*!
 * \par ins The shift to assemble
 * \return 32 bit word with flexible operand in the right place
 *
 * Takes an instruction, and encodes the flexible operand in a 32 bit word,
 * so one can 'or' this with an instruction opcode
 *
 * \todo add some checks
 */
/* ArmEncodeShift {{{ */
t_uint32 ArmEncodeShift(t_arm_ins * ins)
{
  t_uint32 ret = 0;
  switch (ARM_INS_SHIFTTYPE(ins))
    {
    case ARM_SHIFT_TYPE_LSL_IMM :
    case ARM_SHIFT_TYPE_LSR_IMM :
    case ARM_SHIFT_TYPE_ASR_IMM :
    case ARM_SHIFT_TYPE_ROR_IMM :
      /* shifting is done with an immediate */
      ret |= ASM_SHIMM_TYPE(ARM_INS_SHIFTTYPE(ins)) | ASM_SHIFT_IMM(ARM_INS_SHIFTLENGTH(ins));
      break;
    case ARM_SHIFT_TYPE_LSL_REG :
    case ARM_SHIFT_TYPE_LSR_REG :
    case ARM_SHIFT_TYPE_ASR_REG :
    case ARM_SHIFT_TYPE_ROR_REG :
      /* shifting is done with a register */
      if ((ARM_INS_REGS(ins) == ARM_REG_NONE))
        FATAL(("Assembly : Shift with register, but no register specified!"));
      ret |= ASM_REGS(ARM_INS_REGS(ins)) | ASM_SHREG_TYPE(ARM_INS_SHIFTTYPE(ins));
      break;
    case ARM_SHIFT_TYPE_RRX : ret |= ASM_SHIMM_TYPE(ARM_SHIFT_TYPE_ROR_IMM);
      break;
    case ARM_SHIFT_TYPE_NONE : ret |= ASM_SHIMM_TYPE(ARM_SHIFT_TYPE_LSL_IMM);
      break;
    default : FATAL(("Assembly : Unknown shifttype"));
    }
  return ret;
}
/* }}} */
/*!
 * \par ins the instruction to assemble
 * \par instr the 32 bit placeholder
 * \return TRUE if immediate could be encoded, FALSE otherwise
 *
 * Tries to encode an immediate in an instruction, return TRUE if succes
 */
/* ArmEncodeImmediate {{{ */
t_bool ArmEncodeImmediate(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 i;
  t_uint32 value = (t_uint32) ARM_INS_IMMEDIATE(ins);
  for (i = 0; i < 32; i+=2)
    {
      if ((Uint32RotateLeft(((t_uint32) value),i) & 0xffffff00) == 0)
	/* result can be stored in 8 bits */
	{
	  *instr |= Uint32RotateLeft(value,i); /* the rotated imm is the one to insert */
	  /* the amount to rotate to get the original imm back is i : TODO check this */
	  *instr |= ASM_IMM_SHAMOUNT(i);
	  return TRUE;
	}
    }
  return FALSE;
}
/*!
 * \par ins the instruction to assemble
 * \par instr the 32 bit placeholder
 * \return TRUE if immediate could be encoded, FALSE otherwise
 *
 * Tries to encode an immediate16 in an instruction, return TRUE if succes
 */
/* ArmEncodeImmediate16 {{{ */
t_bool ArmEncodeImmediate16(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 value = (t_uint32) ARM_INS_IMMEDIATE(ins);

  if (/*value < 0 ||*/
      value > 0xffff)
    return FALSE;
  *instr |= value & 0xfff;
  *instr |= (value & 0xf000) << 4;
  return TRUE;
}
/* }}} */
/* Arm Integer  Assemble Functions {{{ */
/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles B and BL
 */
/* ArmAssembleBranch {{{ */
void ArmAssembleBranch(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM branch @I", ins));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  if (ARM_INS_TYPE(ins) != IT_BRANCH)
    FATAL(("Wrong assembly function (BRANCH) for instruction of type %s", arm_opcode_table[ARM_INS_OPCODE(ins)].opcode));
  switch (ARM_INS_OPCODE(ins))
  {
  	case ARM_B:
  	case ARM_BL:
	  *instr |= ASM_COND(ARM_INS_CONDITION(ins));
	  *instr |= ASM_IMM24(ARM_INS_IMMEDIATE(ins));
	  break;
	case ARM_BX:
	  *instr|= ASM_COND(ARM_INS_CONDITION(ins));
	  *instr|=(ARM_INS_REGB(ins));
	  break;
	case ARM_BLX:
	  if (ARM_INS_FLAGS(ins) & FL_IMMED)
	  {
	    *instr = 0xfa000000;
	    if (ARM_INS_IMMEDIATE(ins) & 0x2)
	    {
	      *instr |= 0x01000000;
	      ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins)& 0xfffffffc);
	    }
	    *instr|= ASM_IMM24(ARM_INS_IMMEDIATE(ins));
	  }
	  else
	  {
	    *instr|= ASM_COND(ARM_INS_CONDITION(ins));
	    *instr|=(ARM_INS_REGB(ins));
	  }
	  break;
	default:
	  FATAL(("Unknown branch instruction @I!", ins));
  }

}
/* }}} */
/*!
 * Assembles Arm swap (SWP) instructions
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleSwap {{{ */
void ArmAssembleSwap(t_arm_ins * ins, t_uint32 * instr)
{
   VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM swap @I", ins));

   *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
   *instr |= ASM_COND(ARM_INS_CONDITION(ins));
   *instr|=(ARM_INS_REGA(ins)<<12);
   *instr|=(ARM_INS_REGC(ins));
   *instr|=(ARM_INS_REGB(ins)<<16);
}
/* }}} */
/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles Mov and Mvn, i
 */
/* ArmAssembleDataProc {{{ */
void ArmAssembleDataProc(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 tmp = 0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM dataproc @I", ins));

  if (ARM_INS_TYPE(ins) != IT_DATAPROC)
    FATAL(("Wrong assembly function (DATAPROC) for instruction %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  if (ARM_INS_FLAGS(ins) & FL_S)	/* set updates-flag */
    *instr |= ASM_UPDATES_CC;

  if (ARM_INS_FLAGS(ins) & FL_IMMED)
  {
    /* encode the immediate */
    if (ArmEncodeImmediate(ins,&tmp) == FALSE)
      FATAL(("Immediate cannot be assembled in @I", ins));
      /* TODO : find a solution if this happens! */
    else
  	{
  	  *instr |= tmp;
  	  *instr |= ASM_HAS_IMMEDIATE;
  	}
  }
  else if (ARM_INS_FLAGS(ins) & FL_IMMEDW)
  {
    if (ArmEncodeImmediate16(ins,&tmp) == FALSE)
      FATAL(("Immediate cannot be assembled in @I",ins));
    else
  	{
  	  *instr |= tmp;
  	}
  }
  else
  {
    /* encode the flexible operand */
    *instr |= ASM_REGC(ARM_INS_REGC(ins));
    *instr |= ArmEncodeShift(ins);
  }

  /* fill in ARM_INS_REGA */
  if (ARM_INS_REGA(ins) != ARM_REG_NONE)
    *instr |= ASM_REGA(ARM_INS_REGA(ins));
  else
    *instr |= ASM_REGA(0);

  /* fill in ARM_INS_REGB */
  if (ARM_INS_REGB(ins) != ARM_REG_NONE)
    *instr |= ASM_REGB(ARM_INS_REGB(ins));
  else
    *instr |= ASM_REGB(0);
}
/* }}} */
/*!
 * Assembles Arm System Calls (SWI)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleSWI {{{ */
void ArmAssembleSWI(t_arm_ins * ins, t_uint32 * instr)
{
   VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SWI @I", ins));

   *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
   *instr|= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
   *instr|=ARM_INS_IMMEDIATE(ins);
}
/* }}} */
/*!
 * Assembles Arm software breakpoints (BKPT)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleBKPT {{{ */
void ArmAssembleBKPT(t_arm_ins * ins, t_uint32 * instr)
{
   VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM BKPT @I", ins));

   ASSERT((ARM_INS_IMMEDIATE(ins)>=0) && (ARM_INS_IMMEDIATE(ins)<=0xffff),("Invalid software break point number"));
   *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
   *instr|= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
   *instr|= (ARM_INS_IMMEDIATE(ins) & 0x0000fff0) << 4;
   *instr|= (ARM_INS_IMMEDIATE(ins) & 0x0000000f);
}
/* }}} */
/*!
 * Assembles Multiplies (MUL, MULL, ...)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleMUL {{{ */
void  ArmAssembleMUL(t_arm_ins * ins, t_uint32 * instr)
{
   VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM MUL @I", ins));

   *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
   *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
   if (ARM_INS_FLAGS(ins) & FL_S) *instr |=(1 << 20);
   *instr|=(ARM_INS_REGA(ins)<<16);
   *instr|=(ARM_INS_REGB(ins));
   *instr|=(ARM_INS_REGC(ins)<<8);
   if (ARM_INS_REGS(ins) != ARM_REG_NONE)
     *instr|=(ARM_INS_REGS(ins)<<12);
}
/* }}} */
/*!
 * Assembles the instructions that query the ARM Status registers (SPSR and
 * ...): MSR, ...
 *
 * \todo MRS is not implemented
 * \todo What was the name of the other status register?
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleStatus {{{ */
void ArmAssembleStatus(t_arm_ins * ins, t_uint32 * instr)
{
  t_bool set=FALSE;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM status @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
  if (ARM_INS_FLAGS(ins) & FL_SPSR) *instr |=(1 << 22);
  if (ARM_INS_OPCODE(ins)==ARM_MSR)
  {
    if (ARM_INS_FLAGS(ins) & FL_STATUS)
    {
      *instr |=(1 << 19);
      *instr |=(1 << 18);
      set=TRUE;
    }
    if (ARM_INS_FLAGS(ins) & FL_CONTROL)
    {
      *instr |=(1 << 16);
      set=TRUE;
    }

    if (!set) FATAL(("MSR is NOOP, @I",ins));

    if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
	*instr |=(1 << 25);
	if (!ArmEncodeImmediate(ins, instr))
	  FATAL(("Cannot encode constant @I", ins));
      }
    else
      {
	*instr|=ARM_INS_REGC(ins);
      }

  }
  else
  {
    *instr|=(ARM_INS_REGA(ins)<<12);
  }
}
/* }}} */
/*!
 * Assembles the normal loads (LDR, LDRH, ...)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleLoad {{{ */
void ArmAssembleLoad(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  *instr |= ASM_REGA(ARM_INS_REGA(ins));
  *instr |= ASM_REGB(ARM_INS_REGB(ins));

  if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
    *instr |= ASM_PREINDEX;
  if (ARM_INS_FLAGS(ins) & FL_DIRUP)
    *instr |= ASM_DIRUP;
  if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_FLAGS(ins) & FL_PREINDEX))
    *instr |= ASM_WRITEBACK;

  if (ARM_INS_OPCODE(ins) == ARM_LDR || ARM_INS_OPCODE(ins) == ARM_LDRB)
  {
    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      /* encode the immediate */
      ASSERT ((ARM_INS_IMMEDIATE(ins) >= 0) && (ARM_INS_IMMEDIATE(ins) < 4096) ,("Only 12bit immediates can be assembled in a LDR/LDRB instruction! %lld, @I",ARM_INS_IMMEDIATE(ins),ins));
      *instr |= ARM_INS_IMMEDIATE(ins);
    }
    else
    {
      /* encode the flexible operand */
      *instr |= ASM_REGC(ARM_INS_REGC(ins));
      *instr |= ArmEncodeShift(ins);
      *instr |= ASM_LOADSTORE_WITH_REG;
    }
  }
  else if (ARM_INS_OPCODE(ins) == ARM_LDRSH  || ARM_INS_OPCODE(ins) == ARM_LDRH || ARM_INS_OPCODE(ins)==ARM_LDRSB || ARM_INS_OPCODE(ins)==ARM_LDRD)
  {
    if (ARM_INS_OPCODE(ins) == ARM_LDRD)
      ASSERT((ARM_INS_REGA(ins) != 14),("Doubleword load must not have R14 as destination register"));

    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      if (ARM_INS_IMMEDIATE(ins) > 256) FATAL(("Only 8bit immediates can be assembled in a LDRH/LDRD instruction!\nIns was @I",ins));
      *instr |= ((ARM_INS_IMMEDIATE(ins)<<4) & 0xf00) | (ARM_INS_IMMEDIATE(ins) & 0xf);
      *instr |= 1<<22;
    }
    else
    {
      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
        FATAL(("flexible operand not supported for ARM_LDR(S)(H|B|D)\nInstruction was: @I\nShifttype was %d\n",ins,ARM_INS_SHIFTTYPE(ins)));

      *instr |= ASM_REGC(ARM_INS_REGC(ins));
      *instr |= ArmEncodeShift(ins);
    }
  }
  else
  {
    FATAL(("Only LDR and LDRB implemented for assembly"));
  }
}
/* }}} */
/*!
 *  Assembles the normal stores (STR, STRH, ...)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleStore {{{ */
void ArmAssembleStore(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM store @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  *instr |= ASM_REGA(ARM_INS_REGA(ins));
  *instr |= ASM_REGB(ARM_INS_REGB(ins));

  if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
    *instr |= ASM_PREINDEX;
  if (ARM_INS_FLAGS(ins) & FL_DIRUP)
    *instr |= ASM_DIRUP;
  if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_FLAGS(ins) & FL_PREINDEX))
    *instr |= ASM_WRITEBACK;

  if (ARM_INS_OPCODE(ins) == ARM_STR || ARM_INS_OPCODE(ins) == ARM_STRB)
  {
    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      /* encode the immediate */
      if (ARM_INS_IMMEDIATE(ins) < 0 || ARM_INS_IMMEDIATE(ins) > 4095) FATAL(("Only 12bit immediates can be assembled in a LDR/LDRB instruction!"));
      *instr |= ARM_INS_IMMEDIATE(ins);
    }
    else
    {
      /* encode the flexible operand */
      *instr |= ASM_REGC(ARM_INS_REGC(ins));
      *instr |= ArmEncodeShift(ins);
      *instr |= ASM_LOADSTORE_WITH_REG;
    }
  }
  else if (ARM_INS_OPCODE(ins) == ARM_STRH || ARM_INS_OPCODE(ins) == ARM_STRD)
  {
    if (ARM_INS_OPCODE(ins) == ARM_STRD)
      ASSERT(!(ARM_INS_REGA(ins) % 2),("Doubleword store should have even source register"));

    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      if (ARM_INS_IMMEDIATE(ins) > 255) FATAL(("Only 8bit immediates can be assembled in a LDR/LDRB instruction!"));
      *instr |= ((ARM_INS_IMMEDIATE(ins)<<4) & 0xf00) | (ARM_INS_IMMEDIATE(ins) & 0xf);
      *instr |= 1<<22;
    }
    else
    {
      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
        FATAL(("flexible operand not supported for ARM_STRH\n"));

      *instr |= ASM_REGC(ARM_INS_REGC(ins));
      *instr |= ArmEncodeShift(ins);
    }
  }
  else
  {
    FATAL(("Only STR and STRB implemented for assembly: @I",ins));
  }
}
/* }}} */
/*!
 * Assembles Multiple loads/stores (LDM, STM)
 *
 * \todo Remove dependence on immediates (use multiple...)
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleMultipleTransfer {{{ */
void ArmAssembleMultipleTransfer(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM load/store multiple @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;
  *instr |= ASM_COND(ARM_INS_CONDITION(ins));

  if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
    *instr |= ASM_PREINDEX;
  if (ARM_INS_FLAGS(ins) & FL_DIRUP)
    *instr |= ASM_DIRUP;
  if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
    *instr |= ASM_WRITEBACK;

  if (ARM_INS_FLAGS(ins) & FL_USERMODE_REGS)
  {
    *instr |= (1 << 22);
    if (ARM_INS_OPCODE(ins) == ARM_STM ||
	(ARM_INS_OPCODE(ins) == ARM_LDM && !(ARM_INS_IMMEDIATE(ins) & (1 << 15))))
      ASSERT(!(ARM_INS_FLAGS(ins) & FL_WRITEBACK),
	  ("Illegal use of writeback in supervisor mode ldm or stm: @I",ins));
  }

  *instr |= ASM_REGB(ARM_INS_REGB(ins));
  *instr |= ARM_INS_IMMEDIATE(ins) & 0xffff;
}
/* }}} */
/* }}} */
/* Arm Floating Assemble Functions {{{ */
/*!
 * Assembles the instructions that get/set the floating point status register
 * (WFS and RFS).
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleFLTStatus {{{ */
void ArmAssembleFLTStatus(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM floating-point status @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  if (ARM_INS_OPCODE(ins)==ARM_WFS)
  {
    *instr|=(ARM_INS_REGB(ins)<<12);
  }
  else if (ARM_INS_OPCODE(ins)==ARM_RFS)
  {
    *instr|=(ARM_INS_REGA(ins)<<12);
  }
  else
  {
    FATAL(("Unknown ARM_FLT_Status!"));
  }
}
/* }}} */
/*!
 * Assembles all but the memory and status register floating point
 * instructions.
 *
 * \todo change the name
 * \todo rounding mode does not get set in all cases, simple to fix
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* ArmAssembleFLTCPDO {{{ */
void ArmAssembleFLTCPDO(t_arm_ins * ins,t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM floating-point memory/status @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
   switch(ARM_INS_OPCODE(ins))
   {
     case ARM_MVF:
     case ARM_MNF:
     case ARM_SQT:
     case ARM_ABS:
       *instr|=((ARM_INS_REGA(ins) - ARM_REG_F0 )<<12);

       if (ARM_INS_FLAGS(ins)&FL_IMMED)
       {
	 if (ARM_INS_FLAGS(ins)&FL_FLT_IMMED)
	 {
	   *instr|=0x8;
	   *instr|= (ARM_INS_IMMEDIATE(ins) & 0x7);
	 }
	 else
	 {
	   FATAL(("IMPLEMENT!"));
	 }
       }
       else
	 *instr|=ARM_INS_REGB(ins) - ARM_REG_F0;
       if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE_EXTENDED)
	 *instr|=1<<19;
       else if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE)
	 *instr|=1<<7;
       else if (!(ARM_INS_FLAGS(ins)& FL_FLT_SINGLE))
        FATAL(("No size for float"));
       if (ARM_INS_FLAGS(ins) & FL_FLT_RZ) *instr|= (0x3 << 5);
       if (ARM_INS_FLAGS(ins) & FL_FLT_RMI) *instr|= (0x1 << 6);
       if (ARM_INS_FLAGS(ins) & FL_FLT_RPI) *instr|= (0x1 << 5);
       break;
     case ARM_FLT:
        *instr|=((ARM_INS_REGA(ins)- ARM_REG_F0)<<16);
	*instr|=ARM_INS_REGB(ins)<<12;
	if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE_EXTENDED)
	  *instr|=1<<19;
	else if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE)
	  *instr|=1<<7;
	else if (!(ARM_INS_FLAGS(ins)& FL_FLT_SINGLE))
	  FATAL(("No size for float"));
	break;
     case ARM_ADF:
     case ARM_SUF:
     case ARM_RSF:
     case ARM_MUF:
     case ARM_DVF:
     case ARM_RDF:
     case ARM_FDV:
     case ARM_FRD:
     case ARM_FML:
	*instr|=((ARM_INS_REGB(ins)- ARM_REG_F0)<<16);
	*instr|=((ARM_INS_REGA(ins)- ARM_REG_F0)<<12);
	if (ARM_INS_REGC(ins)!=ARM_REG_NONE)
	{
	  *instr|=((ARM_INS_REGC(ins)- ARM_REG_F0));
	}
	else
	{
	  if ((ARM_INS_FLAGS(ins)&FL_IMMED) && (ARM_INS_FLAGS(ins)&FL_FLT_IMMED))
	  {
	    *instr|=0x8;
	    *instr|= (ARM_INS_IMMEDIATE(ins) & 0x7);
	  }
	  else
	    FATAL(("implement"));
	}
	if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE_EXTENDED)
	  *instr|=1<<19;
	else if (ARM_INS_FLAGS(ins)& FL_FLT_DOUBLE)
	  *instr|=1<<7;
	else if (!(ARM_INS_FLAGS(ins)& FL_FLT_SINGLE))
	  FATAL(("No size for float"));
	break;
     case ARM_CMFE:
     case ARM_CMF:
     case ARM_CNF:
     case ARM_CNFE:
       *instr|=((ARM_INS_REGB(ins)- ARM_REG_F0)<<16);
       if (ARM_INS_FLAGS(ins)&FL_IMMED)
       {
	 if (ARM_INS_FLAGS(ins)&FL_FLT_IMMED)
	 {
	   *instr|=0x8;
	   *instr|= (ARM_INS_IMMEDIATE(ins) & 0x7);
	 }
	 else
	 {
	   FATAL(("IMPLEMENT!"));
	 }
       }
       else
	 *instr|=((ARM_INS_REGC(ins)- ARM_REG_F0));
       break;
     case ARM_FIX:
       *instr|=ARM_INS_REGA(ins)<<12;
       *instr|=(ARM_INS_REGB(ins)- ARM_REG_F0);
       if (ARM_INS_FLAGS(ins) & FL_FLT_RZ) *instr|= (0x3 << 5);
       if (ARM_INS_FLAGS(ins) & FL_FLT_RMI) *instr|= (0x1 << 6);
       if (ARM_INS_FLAGS(ins) & FL_FLT_RPI) *instr|= (0x1 << 5);
       break;
     default:
       FATAL(("Implement @I",ins));
   }
}
/* }}} */
/*!
 * Assembles the Arm floating point loads and stores
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 *
 * \todo Check if all dataformats for loads and stores are implemented correct
 *
 * \return void
*/
/* ArmAssembleFLTMEM {{{ */
void ArmAssembleFLTMEM(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM floating-point load/store @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */
  if ((ARM_INS_OPCODE(ins)==ARM_SFM)||(ARM_INS_OPCODE(ins)==ARM_LFM))
  {
    t_reg tel;
    t_reg min=ARM_REG_NONE;
    t_reg max=ARM_REG_NONE;
    for (tel=0; tel<32; tel++)
    {
      if (RegsetIn(ARM_INS_MULTIPLE(ins),tel))
      {
		  if ((tel<ARM_REG_F0) || (tel>ARM_REG_F7))
			 {
				FATAL(("Illegal register in SFM"));
			 }
		  else
			 {
				if ((max!=ARM_REG_NONE)&&(max+1!=tel))
				  {
					 FATAL(("Wrong regs for SFM!"));
				  }
				else if (min==ARM_REG_NONE)
				  {
					 min=max=tel;
				  }
				else
				  {
					 max=tel;
				  }
			 }

      }
    }

    if (max-min>3) FATAL(("Too much registers in SFM"));

    *instr|=(ARM_INS_REGB(ins)<<16);
    *instr|=(min-ARM_REG_F0)<<12;
    if ((max-min)==0)
      *instr|=1<<15;
    else if ((max-min)==1)
      *instr|=1<<22;
    else if ((max-min)==2)
    {
      *instr|=1<<15;
      *instr|=1<<22;
    }
    *instr|=ARM_INS_IMMEDIATE(ins);
    if (ARM_INS_FLAGS(ins)&FL_PREINDEX) *instr|=1 << 24;
    if (ARM_INS_FLAGS(ins)&FL_DIRUP) *instr|=1 << 23;
    if (ARM_INS_FLAGS(ins)&FL_WRITEBACK) *instr|=1 << 21;
  }
  else if (ARM_INS_OPCODE(ins)==ARM_VSTM || ARM_INS_OPCODE(ins)==ARM_VLDM
			  || ARM_INS_OPCODE(ins)==ARM_VPUSH || ARM_INS_OPCODE(ins)==ARM_VPOP) {
	 t_uint32 len;
    if (ARM_INS_FLAGS(ins)&FL_PREINDEX) *instr|=1 << 24;
    if (ARM_INS_FLAGS(ins)&FL_DIRUP) *instr|=1 << 23;
    if (ARM_INS_FLAGS(ins)&FL_WRITEBACK) *instr|=1 << 21;
	 if (ARM_INS_OPCODE(ins)!=ARM_VPUSH && ARM_INS_OPCODE(ins)!=ARM_VPOP)
		*instr|=(ARM_INS_REGB(ins)<<16);
	 if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
		*instr|=ASM_NEON_VD_QD(RegsetFindFirstBlob(ARM_INS_MULTIPLE(ins), &len));
	 else
		*instr|=ASM_NEON_VD_S(RegsetFindFirstBlob(ARM_INS_MULTIPLE(ins), &len));
	 if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
		*instr|=1<<8;
	 len = 0;
	 t_reg reg;
	 REGSET_FOREACH_REG(ARM_INS_MULTIPLE(ins),reg)
		len++;
	 if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
		len *=2;
    *instr|= len & 0xff;
  } else {
	 //	 DEBUG(("@I\n BEGIN %x",ins,*instr));
    if (ARM_INS_FLAGS(ins)&FL_PREINDEX) *instr|=1 << 24;
    if (ARM_INS_FLAGS(ins)&FL_DIRUP) *instr|=1 << 23;
    if (ARM_INS_FLAGS(ins)&FL_WRITEBACK) *instr|=1 << 21;
	 //	 DEBUG(("@I\n FLAGS %x",ins,*instr));
    *instr|=(ARM_INS_REGB(ins)<<16);
	 //	 DEBUG(("@I\n  REG_B %x",ins,*instr));
	 if (ARM_INS_OPCODE(ins)==ARM_VLDR || ARM_INS_OPCODE(ins)==ARM_VSTR)
		{
		  if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
			 {
				*instr|=ASM_NEON_VD_QD(ARM_INS_REGA(ins)) | 0x100;
			 }
		  else
			 {
				*instr|=ASM_NEON_VD_S(ARM_INS_REGA(ins));
			 }
		}
	 else
		*instr|=(ARM_INS_REGA(ins)-ARM_REG_F0)<<12;

	 //	 DEBUG(("@I\n  REG_A %x",ins,*instr));
    if (ARM_INS_IMMEDIATE(ins)&0x3) FATAL(("Non aligned floating load/store"));
      *instr|=ARM_INS_IMMEDIATE(ins)>>2;
	 //	 DEBUG(("@I\n IMMED %x",ins,*instr));

    if (ARM_INS_FLAGS(ins)&FL_FLT_PACKED) *instr|=(1<<22) | (1<<15);
    else if (ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE_EXTENDED) *instr|=(1<<22);
    else if (ARM_INS_FLAGS(ins)&FL_FLT_DOUBLE) *instr|=(1<<15);
	 //	 DEBUG(("@I\n EIND %x",ins,*instr));
 }
}
/* }}} */
/* }}} */

/*!
 * Assembles miscellaneous ARM instructions that fit in no other category
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* {{{ ArmAssembleMisc */
void ArmAssembleMisc(t_arm_ins * ins,t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM misc @I", ins));

  /* opcode and condition code */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_MCR:
    case ARM_MCR2:
    case ARM_MRC:
    case ARM_MRC2:
      {
      	int Rn, Rm, Rd;
      	int opc1, opc2;
      	int coproc;

      	coproc = ARM_INS_IMMEDIATE(ins) & 0xf;
      	opc1 = (ARM_INS_IMMEDIATE(ins) >> 4) & 0x7;
      	opc2 = (ARM_INS_IMMEDIATE(ins) >> 7) & 0x7;
      	Rm = (ARM_INS_IMMEDIATE(ins) >> 10) & 0xf;
      	Rn = (ARM_INS_IMMEDIATE(ins) >> 14) & 0xf;
      	Rd = (ARM_INS_OPCODE(ins) == ARM_MCR) ? ARM_INS_REGC(ins) : ARM_INS_REGA(ins);
      	/* special case: if the condition flags are defined, Rd == r15 */
      	if (ARM_INS_FLAGS(ins) & FL_S)
      	{
      	  ASSERT(Rd == ARM_REG_NONE,("internal representation corrupt"));
      	  Rd = 15;
      	}

      	*instr |= (opc1 << 21) | (Rn << 16) | (Rd << 12) | (coproc << 8) | (opc2 << 5) | Rm;
      }
      break;
    case ARM_LDC:
    case ARM_LDC2:
    case ARM_STC:
    case ARM_STC2:
      {
      	int Rn, Rd;
      	int coproc;
      	int offset;

      	coproc = ARM_INS_IMMEDIATE(ins) & 0xf;
      	Rn = ARM_INS_REGB(ins);
      	Rd = (ARM_INS_IMMEDIATE(ins) >> 4) & 0xf;
      	offset = (ARM_INS_IMMEDIATE(ins) >> 8) & 0xff;

      	*instr |= (Rn << 16) | (Rd << 12) | (coproc << 8) | offset;
      	if (ARM_INS_FLAGS(ins) & FL_LONG_TRANSFER)
      	  *instr |= 1 << 22;
      	if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
      	  *instr |= 1 << 21;
      	if (ARM_INS_FLAGS(ins) & FL_DIRUP)
      	  *instr |= 1 << 23;
      	if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
      	  *instr |= 1 << 24;
      }
      break;
    case ARM_MCRR:
    case ARM_MCRR2:
    case ARM_MRRC:
    case ARM_MRRC2:
      {
      	int Rn, Rm, Rd;
      	int opc;
      	int coproc;

      	coproc = ARM_INS_IMMEDIATE(ins) & 0xf;
      	opc = (ARM_INS_IMMEDIATE(ins) & 0xf0) >> 4;
      	Rm = (ARM_INS_IMMEDIATE(ins) & 0xf00) >> 8;
      	Rd = (ARM_INS_OPCODE(ins) == ARM_MRRC) ? ARM_INS_REGA(ins) : ARM_INS_REGB(ins);
      	Rn = (ARM_INS_OPCODE(ins) == ARM_MRRC) ? ARM_INS_REGB(ins) : ARM_INS_REGC(ins);

      	*instr |= (Rn << 16) | (Rd << 12) | (coproc << 8) | (opc << 4) | Rm;
      }
      break;

    case ARM_CDP:
    case ARM_CDP2:
      {
        *instr |= ARM_INS_IMMEDIATE(ins);
      }
      break;

    case ARM_PLD:
    case ARM_PLDW:
    case ARM_PLI:
      {
        *instr |= ARM_INS_REGB(ins) << 16;
      	if (ARM_INS_FLAGS(ins) & FL_DIRUP)
      	  *instr |= ASM_DIRUP;
      	if (ARM_INS_FLAGS(ins) & FL_IMMED)
      	{
      	  /* encode the immediate */
      	  if (ARM_INS_IMMEDIATE(ins) > 4095)
      	    FATAL(("Only 12bit immediates can be assembled in a LDR/LDRB instruction! %lld, @I",ARM_INS_IMMEDIATE(ins),ins));
      	  *instr |= ARM_INS_IMMEDIATE(ins);
      	}
            else
      	{
      	  /* encode the flexible operand */
      	  *instr |= ASM_REGC(ARM_INS_REGC(ins));
      	  *instr |= ArmEncodeShift(ins);
      	  *instr |= ASM_LOADSTORE_WITH_REG;
      	}

      }
      break;

    case ARM_CLREX:
    case ARM_NOP:
      break;

    case ARM_SMC:
      {
        /* encode option */
        *instr |= ARM_INS_IMMEDIATE(ins) & 0x0000000f;
      }
      break;

    case ARM_SETEND:
      {
        if (ARM_INS_IMMEDIATE(ins))
          *instr |= 2<<9;
      }
      break;

    case ARM_CPSIE:
    case ARM_CPSID:
    case ARM_CPS:
      *instr |= ARM_INS_IMMEDIATE(ins);
      break;

    default:
      FATAL(("Implement assembly of misc type instruction @I",ins));
  }
}
/* }}} */



/*!
 * Assembles VFP data processing instructions
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* {{{ ArmAssembleVFPDP */
void
ArmAssembleVFPDP(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint64 immed=0;
  t_uint32 imm=0;
  t_arm_ins_dt size=DT_NONE, sizetype=DT_NONE;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM VFP dataproc @I", ins));

  if (ARM_INS_TYPE(ins) != IT_FLT_ALU)
    FATAL(("Wrong assembly function (FLT_ALU) for instruction %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));

  /* opcode and condition code */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_VCVTB:
    case ARM_VCVTT:
      *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
      *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
      *instr |= (ARM_INS_DATATYPE(ins) == DT_F16) ? 0x00010000 : 0;
      break;

    case ARM_VCVT_DS:
      if(ARM_INS_DATATYPE(ins) == DT_F32)
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VM_S(ARM_INS_REGB(ins));
      }
      break;

    case ARM_VCVT_X2F:
    case ARM_VCVT_F2X:
      if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      {
        /* Dd */
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= 0x00000100;
      }
      else
      {
        /* Sd */
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
      }

      /* extract integer size */
      size = (ARM_INS_OPCODE(ins) == ARM_VCVT_X2F) ? ARM_INS_DATATYPEOP(ins) : ARM_INS_DATATYPE(ins);

      /* u */
      *instr |= (size==DT_U32 || size==DT_U16) ? 0x00010000 : 0;

      /* sx + imm */
      if(size==DT_U32 || size==DT_S32) {
        *instr |= 0x00000080;
        imm = 32 - ARM_INS_IMMEDIATE(ins);
      }
      else
      {
        imm = 16 - ARM_INS_IMMEDIATE(ins);
      }

      *instr |= (imm & 0x00000001) << 5;
      *instr |= (imm & 0x0000001e) >> 1;
      break;

    case ARM_VCVTR_F2I:
    case ARM_VCVT_F2I:
    case ARM_VCVT_I2F:
      /* Dd, Sd */
      if(ARM_INS_DATATYPE(ins) == DT_F64)
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
      }

      /* Dm, Sm */
      if(ARM_INS_DATATYPEOP(ins) == DT_F64)
      {
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VM_S(ARM_INS_REGB(ins));
      }

      /* op */
      *instr |= (ARM_INS_DATATYPEOP(ins) == DT_S32) ? 0x00000080 : 0;

      /* opc2 */
      *instr |= (ARM_INS_DATATYPE(ins) == DT_S32) ? 0x00010000 : 0;
      break;

    case ARM_VMLA_F64:
    case ARM_VMLS_F64:
    case ARM_VNMLA:
    case ARM_VNMLS:
    case ARM_VNMUL:
    case ARM_VMUL_F64:
    case ARM_VADD_F64:
    case ARM_VSUB_F64:
    case ARM_VDIV:
    case ARM_VFNMA:
    case ARM_VFNMS:
    case ARM_VFMA_F64:
    case ARM_VFMS_F64:
      if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VN_S(ARM_INS_REGB(ins));
        *instr |= ASM_NEON_VM_S(ARM_INS_REGC(ins));
      }
      break;

    case ARM_VMOV_FIMM:
      immed = ARM_INS_IMMEDIATE(ins);
      if(ARM_INS_DATATYPE(ins) == DT_F64)
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= 0x00000100;
        immed >>= 32;

        imm = (immed & 0x003f0000) >> 16;
      }
      else
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
        imm = (immed & 0x01f80000) >> 19;
      }

      imm |= (immed & 0x80000000) >> 24;
      imm |= (immed & 0x20000000) >> 23;

      *instr |= (((imm & 0xf0) << 12) | (imm & 0x0f));
      break;

    case ARM_VMOV_F:
    case ARM_VABS_F64:
    case ARM_VNEG_F64:
    case ARM_VSQRT:
      if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGA(ins));
        *instr |= ASM_NEON_VM_S(ARM_INS_REGB(ins));
      }
      break;

    case ARM_VCMP:
    case ARM_VCMPE:
      if(ARM_INS_DATATYPE(ins) == DT_F64)
      {
        *instr |= ASM_NEON_VD_QD(ARM_INS_REGB(ins));
        *instr |= (ARM_INS_FLAGS(ins) & FL_IMMED) ? 0x00010000 : ASM_NEON_VM_QD(ARM_INS_REGC(ins));
        *instr |= 0x00000100;
      }
      else
      {
        *instr |= ASM_NEON_VD_S(ARM_INS_REGB(ins));
        *instr |= (ARM_INS_FLAGS(ins) & FL_IMMED) ? 0x00010000 : ASM_NEON_VM_S(ARM_INS_REGC(ins));
      }
      break;

    case ARM_FCMPS:
    case ARM_FCMPD:
    case ARM_FCMPES:
    case ARM_FCMPED:
    case ARM_FCMPZS:
    case ARM_FCMPZD:
    case ARM_FCMPEZS:
    case ARM_FCMPEZD:
      *instr |= ASM_VFP_FD(ARM_INS_REGB(ins));
      *instr |= ASM_VFP_FM(ARM_INS_REGC(ins));
      break;
    case ARM_FCPYS:
    case ARM_FABSS:
    case ARM_FCPYD:
    case ARM_FABSD:
    case ARM_FNEGS:
    case ARM_FSQRTS:
    case ARM_FNEGD:
    case ARM_FSQRTD:
    case ARM_FCVTDS:
    case ARM_FCVTSD:
    case ARM_FUITOS:
    case ARM_FUITOD:
    case ARM_FSITOS:
    case ARM_FSITOD:
    case ARM_FTOUIS:
    case ARM_FTOUID:
    case ARM_FTOUIZS:
    case ARM_FTOUIZD:
    case ARM_FTOSIS:
    case ARM_FTOSID:
    case ARM_FTOSIZS:
    case ARM_FTOSIZD:
      *instr |= ASM_VFP_FD(ARM_INS_REGA(ins));
      *instr |= ASM_VFP_FM(ARM_INS_REGB(ins));
      break;
    case ARM_FMACS:
    case ARM_FNMACS:
    case ARM_FMACD:
    case ARM_FNMACD:
    case ARM_FMSCS:
    case ARM_FNMSCS:
    case ARM_FMSCD:
    case ARM_FNMSCD:
    case ARM_FMULS:
    case ARM_FNMULS:
    case ARM_FMULD:
    case ARM_FNMULD:
    case ARM_FADDS:
    case ARM_FSUBS:
    case ARM_FADDD:
    case ARM_FSUBD:
    case ARM_FDIVS:
    case ARM_FDIVD:
      *instr |= ASM_VFP_FD(ARM_INS_REGA(ins));
      *instr |= ASM_VFP_FN(ARM_INS_REGB(ins));
      *instr |= ASM_VFP_FM(ARM_INS_REGC(ins));
      break;
    default:
      FATAL(("Unsupported instruction for VFP data processing: @I",ins));
  }
}
/* }}} */



/*!
 * Assembles VFP register transfer instructions
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* {{{ ArmAssembleVFPRT */
void
ArmAssembleVFPRT(t_arm_ins * ins, t_uint32 * instr)
{
  t_reg rega, regb;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM VFP register transfer @I", ins));

  /* opcode and condition code */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */


  rega = ARM_INS_REGA(ins);
  regb = ARM_INS_REGB(ins);
  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_FMSTAT:
      /* nothing more to do */
      break;
    case ARM_FMDHR:
      /* always refers to even register, but since it updates
       * the upper 32 bits we changed it to an odd number during
       * disassembling
       */
      rega &= ~1;
      /* fall through */
    case ARM_FMDLR:
    case ARM_FMSR:
      *instr |= regb<<12;
      *instr |= ASM_VFP_FN(rega);
      break;
    case ARM_FMRDH:
      /* always refers to even register, but since it reads
       * the upper 32 bits we changed it to an odd number during
       * disassembling
       */
      regb &= ~1;
      /* fall through */
    case ARM_FMRDL:
    case ARM_FMRS:
      *instr |= rega<<12;
      *instr |= ASM_VFP_FN(regb);
      break;
    case ARM_FMXR:
      *instr |= regb << 12;
      *instr |= ASM_VFP_SYSREG(rega);
      break;
    case ARM_FMRX:
      *instr |= rega << 12;
      *instr |= ASM_VFP_SYSREG(regb);
      break;
    default:
      FATAL(("Wrong assembly function (VFPRT) for instruction %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));
  }
}
/* }}} */


/*!
 * Assembles VFP two-register transfer instructions
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* {{{ ArmAssembleVFP2R */
void
ArmAssembleVFP2R(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM VFP two-register transfer @I", ins));

  /* opcode and condition code */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_FMRRD:
    case ARM_FMRRS:
      *instr |= ARM_INS_REGA(ins)<<16;
      *instr |= ASM_VFP_FM(ARM_INS_REGB(ins));
      *instr |= ARM_INS_REGC(ins)<<12;
      break;
    case ARM_FMSRR:
    case ARM_FMDRR:
      *instr |= ASM_VFP_FM(ARM_INS_REGA(ins));
      *instr |= ARM_INS_REGB(ins)<<16;
      *instr |= ARM_INS_REGC(ins)<<12;
    break;
    default:
      FATAL(("Wrong assembly function (VFP2R) for instruction %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));
  }
}
/* }}} */


/*!
 * Assembles VFP data processing instructions
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
*/
/* {{{ ArmAssembleVFPDT */
void ArmAssembleVFPDT(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM VFP dataproc @I", ins));

  /* opcode and condition code */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */
  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  if ((ARM_INS_OPCODE(ins)==ARM_FSTMS)||(ARM_INS_OPCODE(ins)==ARM_FSTMD)||(ARM_INS_OPCODE(ins)==ARM_FSTMX) ||
      (ARM_INS_OPCODE(ins)==ARM_FLDMS)||(ARM_INS_OPCODE(ins)==ARM_FLDMD)||(ARM_INS_OPCODE(ins)==ARM_FLDMX))
  {
    t_reg tel;
    t_reg min=ARM_REG_NONE;
    t_reg max=ARM_REG_NONE;
    for (tel=ARM_REG_S0; tel<=ARM_REG_S31; tel++)
    {
      if (RegsetIn(ARM_INS_MULTIPLE(ins),tel))
      {
        if ((max!=ARM_REG_NONE)&&(max+1!=tel))
        {
          FATAL(("Wrong regs for VFP load/store multiple!"));
        }
        else if (min==ARM_REG_NONE)
        {
          min=max=tel;
        }
        else
        {
          max=tel;
        }

      }
    }

    if (max-min-ARM_INS_REGC(ins)>15) FATAL(("Too many registers in VFP load/store multiple"));
    if (min!=ARM_INS_REGC(ins))
      FATAL(("REGC not set correctly for VFP load/store multiple"));

    *instr|=(ARM_INS_REGB(ins)<<16);
    *instr|=ASM_VFP_FD(ARM_INS_REGC(ins));
    *instr|=max-min+1;
    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      ASSERT(((max-min+1)&1)==0,("Invalid number of registers in VFP load/store multiple"));

    /* FSTMX/FLDMX are distinguished from FSTMD/FLDMD by having the lowest bit set to one */
    if ((ARM_INS_OPCODE(ins) == ARM_FSTMX) || (ARM_INS_OPCODE(ins) == ARM_FLDMX))
      *instr|=1;

    if (ARM_INS_FLAGS(ins)&FL_PREINDEX) *instr|=1 << 24;
    if (ARM_INS_FLAGS(ins)&FL_DIRUP) *instr|=1 << 23;
    if (ARM_INS_FLAGS(ins)&FL_WRITEBACK) *instr|=1 << 21;
  }
  else
  {
    if (ARM_INS_FLAGS(ins)&FL_PREINDEX) *instr|=1 << 24;
    if (ARM_INS_FLAGS(ins)&FL_DIRUP) *instr|=1 << 23;
    if (ARM_INS_FLAGS(ins)&FL_WRITEBACK) *instr|=1 << 21;
    *instr|=(ARM_INS_REGB(ins)<<16);
    *instr|=ASM_VFP_FD(ARM_INS_REGA(ins));
    ASSERT((ARM_INS_IMMEDIATE(ins)&0x3)==0,("Non-aligned vector floating load/store"));
    ASSERT((ARM_INS_IMMEDIATE(ins)>>2) < 256,("Offset too large for vector floating load/store"));
    *instr|=ARM_INS_IMMEDIATE(ins)>>2;
  }
}
/* }}} */

/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles pkhbt, pkhtb
 */
/* ArmAssembleV6Pkh {{{ */
void ArmAssembleV6Pkh(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM V6 PKH @I", ins));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  ASSERT(ARM_INS_REGA(ins) != ARM_REG_NONE,("pkh* does not have a valid REGA: @I",ins));
  ASSERT(ARM_INS_REGB(ins) != ARM_REG_NONE,("pkh* does not have a valid REGB: @I",ins));
  ASSERT(ARM_INS_REGC(ins) != ARM_REG_NONE,("pkh* does not have a valid REGC: @I",ins));
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGB(ins) != ARM_REG_R15) &&(ARM_INS_REGC(ins) != ARM_REG_R15),("pkh* instruction at @I has r15 as argument, undefined behaviour",ins));

  ASSERT(ARM_INS_SHIFTTYPE(ins) ==  ARM_SHIFT_TYPE_LSL_IMM, ("pkh* only supports ARM_SHIFT_TYPE_LSL_IMM shifter: @I",ins));
  ASSERT(ARM_INS_SHIFTLENGTH(ins) <= 31, ("Invalid shifter length for pkh*: @I",ins));

  /* encode the flexible operand */
  *instr |= ASM_REGC(ARM_INS_REGC(ins));

  /* fill in ARM_INS_REGA */
  *instr |= ASM_REGA(ARM_INS_REGA(ins));

  /* fill in ARM_INS_REGB */
  *instr |= ASM_REGB(ARM_INS_REGB(ins));

  /* fill in the (custom) shifter info */
  *instr |= ARM_INS_SHIFTLENGTH(ins) << 7;
}
/* }}} */


/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles most ARMv6 data processing instructions
 */
/* ArmAssembleV6DataProc {{{ */
void ArmAssembleV6DataProc(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM V6 dataproc @I", ins));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  ASSERT(ARM_INS_REGA(ins) != ARM_REG_NONE,("ARMv6 data processing ins does not have a valid REGA: @I",ins));
  ASSERT(ARM_INS_REGC(ins) != ARM_REG_NONE,("ARMv6 data processing ins does not have a valid REGC: @I",ins));
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("ARMv6 data processing ins instruction at @I has r15 as argument, undefined behaviour",ins));

  ASSERT(ARM_INS_SHIFTTYPE(ins) ==  ARM_SHIFT_TYPE_NONE, ("ARMv6 data processing instructions don't support shifters: @I",ins));

  /* encode the flexible operand */
  *instr |= ASM_REGC(ARM_INS_REGC(ins));

  /* fill in ARM_INS_REGA */
  *instr |= ASM_REGA(ARM_INS_REGA(ins));

  /* fill in ARM_INS_REGB */
  if (ARM_INS_REGB(ins) != ARM_REG_NONE)
    *instr |= ASM_REGB(ARM_INS_REGB(ins));
  else
    /* REV/REV16/REVSH */
    *instr |= ASM_REGB(0);
}
/* }}} */


/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles ARMv6 extraction instructions
 */
/* ArmAssembleV6Extract {{{ */
void ArmAssembleV6Extract(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM V6 extraction @I", ins));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  ASSERT(ARM_INS_REGA(ins) != ARM_REG_NONE,("ARMv6 extraction ins does not have a valid REGA: @I",ins));
  ASSERT(ARM_INS_REGC(ins) != ARM_REG_NONE,("ARMv6 extraction ins does not have a valid REGC: @I",ins));
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("ARMv6 data processing ins instruction at @I has r15 as argument, undefined behaviour",ins));

  ASSERT(ARM_INS_SHIFTTYPE(ins) ==  ARM_SHIFT_TYPE_ROR_IMM, ("ARMv6 extraction instructions don't support shifters other than ROR_IMM: @I",ins));
  ASSERT((ARM_INS_SHIFTLENGTH(ins) & ~(3<<3)) == 0, ("ARMv6 extraction instructions don't support shifter lengths other than 0/8/16/24: @I",ins));

  /* encode the flexible operand */
  *instr |= ASM_REGC(ARM_INS_REGC(ins));

  /* fill in ARM_INS_REGA */
  *instr |= ASM_REGA(ARM_INS_REGA(ins));

  /* fill in ARM_INS_REGB */
  if (ARM_INS_REGB(ins) != ARM_REG_NONE)
    *instr |= ASM_REGB(ARM_INS_REGB(ins));
  else
    /* not an error: no regb here means %1111 */
    *instr |= ASM_REGB(0xf);

  /* encode the shifter length */
  *instr |= ARM_INS_SHIFTLENGTH(ins) << 7;
}
/* }}} */


/*!
 * \par ins the instruction to assemble (t_arm_ins*)
 * \par instr the 32 bit placeholder (t_uint32*)
 *
 * Assembles most ARMv6 saturation instructions
 */
/* ArmAssembleV6Sat {{{ */
void ArmAssembleV6Sat(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 tmp = 0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM V6 saturation @I", ins));

  *instr= arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;  /* Set the opcode */

  *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  ASSERT(ARM_INS_REGA(ins) != ARM_REG_NONE,("ARMv6 saturation ins does not have a valid REGA: @I",ins));
  ASSERT(ARM_INS_REGC(ins) != ARM_REG_NONE,("ARMv6 saturation ins does not have a valid REGC: @I",ins));
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("ARMv6 saturation ins instruction at @I has r15 as argument, undefined behaviour",ins));

  /* encode the flexible operand */
  *instr |= ASM_REGC(ARM_INS_REGC(ins));

  /* fill in ARM_INS_REGA */
  *instr |= ASM_REGA(ARM_INS_REGA(ins));

  /* fill in the immediate */
  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_SSAT:
    case ARM_USAT:
      ASSERT((ARM_INS_IMMEDIATE(ins) >= 1) && (ARM_INS_IMMEDIATE(ins) <= 32), ("Immediate operand of SSAT/USAT must be within the range 1-32: @I",ins));
      ASSERT((ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM) || (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM),("ARMv6 saturation ins does not support shifter types different from LSL_IMM and ASR_IMM: @I",ins));
      ASSERT((ARM_INS_SHIFTLENGTH(ins) <= 32), ("Shift length of SSAT/USAT must be within the range 0-32: @I",ins));
      /* sh bit: upper bit of LSL_IMM/ASR_IMM */
      *instr |= (ARM_INS_SHIFTTYPE(ins) >> 1) << 6;
      /* shift_imm */
      *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x1f) << 7;
      break;
    case ARM_SSAT16:
    case ARM_USAT16:
      ASSERT((ARM_INS_IMMEDIATE(ins) >= 1) && (ARM_INS_IMMEDIATE(ins) <= 16), ("Immediate operand of SSAT16/USAT16 must be within the range 1-16: @I",ins));
      break;
    default:
      FATAL(("Unsupported saturation opcode"));
  }
  *instr |= (ARM_INS_IMMEDIATE(ins) - 1) << 15;
}

void ArmAssembleSIMD(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 imm=0, dreg=0, len=0;
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;
  int i;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VDUP:
      *instr |= ASM_NEON_VN_QD(ARM_INS_REGA(ins));
      *instr |= (ARM_INS_REGB(ins) << 12);

      switch(ARM_INS_DATATYPE(ins))
      {
        case DT_8:
          *instr |= (1 << 22);
          break;

        case DT_16:
          *instr |= (1 << 5);
          break;

        case DT_32:
          break;

        default:
          FATAL(("Illegal datatype"));
      }

      if(ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD)
      {
        *instr |= (1 << 21);
      }

      *instr |= (ARM_INS_CONDITION(ins) << 28);
      break;

    case ARM_VTBL:
    case ARM_VTBX:
      *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
      *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));

      dreg = RegsetFindFirstBlob(ARM_INS_MULTIPLE(ins), &len);
      len--;
      *instr |= (len & 0x3) << 8;
      *instr |= ASM_NEON_VN_QD(dreg);
      break;

    case ARM_VEXT:
      *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
      *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));
      *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));
      *instr |= ARM_INS_IMMEDIATE(ins) << 8;
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      break;

    case ARM_VDUP_SCALAR:
      *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
      *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;

      switch(size+DT_START)
      {
        case DT_8:
          imm = 1 | (ARM_INS_REGBSCALAR(ins) << 1);
          break;

        case DT_16:
          imm = 2 | (ARM_INS_REGBSCALAR(ins) << 2);
          break;

        case DT_32:
          imm = 4 | (ARM_INS_REGBSCALAR(ins) << 3);
          break;

        default:
          FATAL(("Illegal datatype"));
      }
      *instr |= (imm << 16);
      break;

    default:
      FATAL(("Illegal opcode: @I", ins));
  }
}

void ArmAssembleSIMDImm(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint64 imm = ARM_INS_IMMEDIATE(ins);
  t_uint32 temp = 0;
  t_uint32 cmode = -1;
  t_uint32 op = 0;
  t_uint64 i;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (immediate) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  /* deconstruct modified immediate value */
  switch(ARM_INS_DATATYPE(ins))
  {
    case DT_I32:
      if((imm & 0xffffffff) == 0)
      {
        temp = 0;
        cmode = 0;
      }
      else if((imm & 0x00ffffff) == 0)
      {
        temp = (imm & 0xff000000) >> 24;
        cmode = 6;
      }
      else if( ((imm & 0x0000ffff) == 0) || ((imm & 0x0000ffff) == 0x0000ffff) )
      {
        temp = (imm & 0x00ff0000) >> 16;
        cmode = ((imm & 0x0000ffff) == 0x0000ffff) ? 13 : 4;
      }
      else if( ((imm & 0x000000ff) == 0) || ((imm & 0x000000ff) == 0x000000ff) )
      {
        temp = (imm & 0x0000ff00) >> 8;
        cmode = ((imm & 0x000000ff) == 0x000000ff) ? 12 : 2;
      }
      else
      {
        temp = (imm & 0x000000ff);
        cmode = 0;
      }
      break;

    case DT_I16:
      if((imm & 0x000000ff) == 0)
      {
        temp = (imm & 0x0000ff00) >> 8;
        cmode = 10;
      }
      else
      {
        temp = (imm & 0x000000ff);
        cmode = 8;
      }
      break;

    case DT_I8:
      temp = (imm & 0x000000ff);
      cmode = 14;
      op = 0;
      break;

    case DT_F32:
      temp = (imm & 0x80000000) >> 24;
      temp |= (imm & 0x20000000) >> 23;
      temp |= (imm & 0x01f80000) >> 19;
      cmode = 15;
      op = 0;
      break;

    case DT_I64:
      temp = 0;
      for(i = 0; i < 8; i++)
      {
        if(imm & (1ULL << 8*i))
        {
          temp |= (1 << i);
        }
      }
      cmode = 14;
      op = 1;
      break;

    default:
      FATAL(("Illegal datatype for SIMD immediate instruction."));
  }

  *instr |= (temp & 0x80) << 17;
  *instr |= (temp & 0x70) << 12;
  *instr |= (temp & 0x0f);

  *instr |= (cmode << 8);
  *instr |= (op << 5);

  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  if(ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD)
  {
    *instr |= 0x00000040;
  }
}

void ArmAssembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a=0, u=0, b=0;
  t_uint32 size=0;
  t_arm_ins_dt sizetype = DT_NONE;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (2 regs + scalar) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));

  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VMLA_SCALAR:
    case ARM_VMLS_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VML[A|S] (scalar) instruction"));
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x01000000 : 0;
      *instr |= (size << 20);
      *instr |= (sizetype == DT_F_START) ? 0x00000100 : 0;
      break;

    case ARM_VMLAL_SCALAR:
    case ARM_VMLSL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VML[A|S]L (scalar) instruction"));
      *instr |= (size << 20);
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      break;

    case ARM_VQDMLAL_SCALAR:
    case ARM_VQDMLSL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQDML[A|S]L (scalar) instruction"));
      *instr |= (size << 20);
      break;

    case ARM_VMUL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VMUL (scalar) instruction"));
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x01000000 : 0;
      *instr |= (sizetype == DT_F_START) ? 0x00000100 : 0;
      *instr |= (size << 20);
      break;

    case ARM_VMULL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VMULL (scalar) instruction"));
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      *instr |= (size << 20);
      break;

    case ARM_VQDMULH_SCALAR:
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x01000000 : 0;
    case ARM_VQDMULL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQDMUL[L|H] (scalar) instruction"));
      *instr |= (size << 20);
      break;

    case ARM_VQRDMULH_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQRDMULH (scalar) instruction"));
      *instr |= (size << 20);
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x01000000 : 0;
      break;

    default:
      FATAL(("Illegal opcode: @I", ins));
  }

  *instr |= ASM_NEON_VM_SCALAR(ARM_INS_REGC(ins), ARM_INS_REGCSCALAR(ins), size+DT_START);
}

void ArmAssembleSIMDDP(t_arm_ins * ins, t_uint32 * instr)
{
  FATAL(("Implement: @I", ins));
}

void ArmAssembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a = 0, b = 0;
  t_uint32 size = 0;
  t_arm_ins_dt sizetype = DT_NONE;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (2 regs + misc) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  /* registers */
  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));

  /* Q-field */
  if( (ARM_INS_OPCODE(ins) != ARM_VSHLL) &&
      (ARM_INS_OPCODE(ins) != ARM_VQMOVN) &&
      (ARM_INS_OPCODE(ins) != ARM_VQMOVUN) &&
      (ARM_INS_OPCODE(ins) != ARM_VMOVN) &&
      (ARM_INS_OPCODE(ins) != ARM_VCVT_HS))
  {
    *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
  }

  /* normalized size and base type */
  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VREV64:
    case ARM_VREV32:
    case ARM_VREV16:
      *instr |= (size << 18);
      break;

    case ARM_VPADDL:
    case ARM_VPADAL:
      *instr |= (size << 18);
      *instr |= (sizetype == DT_U_START) ? 0x00000080 : 0;
      break;

    case ARM_VCLS:
    case ARM_VCLZ:
    case ARM_VCNT:
      *instr |= (size << 18);
      break;

    case ARM_VMVN:
      ASSERT(size == 0, ("Illegal size-field for VMVN instruction"));
      *instr |= (size << 18);
      break;

    case ARM_VMOVN:
      size--;
    case ARM_VQABS:
    case ARM_VQNEG:
    case ARM_VTRN:
    case ARM_VUZP:
    case ARM_VZIP:
    case ARM_VSHLL:
      ASSERT(size != 3, ("Illegal size-field for VQABS/VQNEG instruction"));
      *instr |= (size << 18);
      break;

    case ARM_VCGT_IMM:
    case ARM_VCGE_IMM:
    case ARM_VCEQ_IMM:
    case ARM_VCLE_IMM:
    case ARM_VCLT_IMM:
    case ARM_VABS:
    case ARM_VNEG:
      *instr |= (size << 18);
      *instr |= (sizetype == DT_F_START) ? 0x00000400 : 0;
      break;

    case ARM_VSWP:
      ASSERT(size == 0, ("Illegal size-field in VSWP instruction"));
      *instr |= (size << 18);
      break;

    case ARM_VQMOVN:
      *instr |= (sizetype == DT_U_START) ? 0x00000040 : 0;
    case ARM_VQMOVUN:
      size--;
      *instr |= (size << 18);
      break;

    case ARM_VRECPE:
    case ARM_VRSQRTE:
      ASSERT(size == 2, ("Illegal size-field in VRECPE instruction"));
      *instr |= (size << 18);
      *instr |= (sizetype == DT_F_START) ? 0x00000100 : 0;
      break;

    case ARM_VCVT_HS:
      *instr |= (1 << 18);
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000100 : 0;
      break;

    case ARM_VCVT_FI:
      ASSERT(size == 2, ("Illegal size-field in VCVT (floating-integer) instruction"));
      *instr |= (size << 18);
      switch(ARM_INS_DATATYPE(ins))
      {
        case DT_S32:
          ASSERT(ARM_INS_DATATYPEOP(ins) == DT_F32, ("Illegal source operand type for VCVT (floating-integer) instruction."));
          *instr |= (2 << 7);
          break;

        case DT_U32:
          ASSERT(ARM_INS_DATATYPEOP(ins) == DT_F32, ("Illegal source operand type for VCVT (floating-integer) instruction."));
          *instr |= (3 << 7);
          break;

        case DT_F32:
          switch(ARM_INS_DATATYPEOP(ins))
          {
            case DT_S32:
              break;

            case DT_U32:
              *instr |= (1 << 7);
              break;

            default:
              FATAL(("Illegal source operand type for VCVT (floating-integer) instruction."));
          }
          break;

        default:
          FATAL(("Illegal datatype for VCVT (floating-integer) instruction."));
      }
      break;

    default:
      FATAL(("Unhandled instruction: @I", ins));
  }
}

void ArmAssembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a = 0, b = 0, u = 0, c = 0;
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;
  t_uint32 uflag = 0, qflag = 0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (3 regs of same length) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  /* Destination and operand registers */
  ASSERT(ARM_INS_REGA(ins) != ARM_REG_NONE, ("SIMD[3 registers, same length] instruction should have 3 registers set, but A isn't."));
  ASSERT(ARM_INS_REGB(ins) != ARM_REG_NONE, ("SIMD[3 registers, same length] instruction should have 3 registers set, but B isn't."));
  ASSERT(ARM_INS_REGC(ins) != ARM_REG_NONE, ("SIMD[3 registers, same length] instruction should have 3 registers set, but C isn't."));

  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  if((ARM_INS_OPCODE(ins) == ARM_VQRSHL) ||
      (ARM_INS_OPCODE(ins) == ARM_VSHL) ||
      (ARM_INS_OPCODE(ins) == ARM_VQSHL) ||
      (ARM_INS_OPCODE(ins) == ARM_VRSHL))
  {
    *instr |= ASM_NEON_VN_QD(ARM_INS_REGC(ins));
    *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
  }
  else
  {
    *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));
    *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));
  }

  /* Q-field */
  *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;

  /* size & U-field */
  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);
  if((ARM_SIMD_3REGSSAMELENGTH_FIRSTF <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_3REGSSAMELENGTH_LASTF))
  {
    size = 0;
    sizetype = 0;
  }

  u = ((sizetype == DT_U_START) || (sizetype == DT_P_START)) ? 1 : 0;
  *instr |= (u << 24);
  *instr |= (size << 20);
}

void ArmAssembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a=0, u=0, b=0, l=0, imm=0;
  t_uint32 opc = ARM_INS_OPCODE(ins);

  t_arm_ins_dt sizetype = DT_NONE;
  t_uint32 size=0, imminv=0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (2 regs + shifted) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[opc].opcode;

  /* registers */
  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));

  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  if((opc == ARM_VSHRN) || (opc == ARM_VRSHRN) ||
     (opc == ARM_VQSHRUN) || (opc == ARM_VQSHRN) ||
     (opc == ARM_VQRSHRUN) || (opc == ARM_VQRSHRN))
  {
    size--;
  }

  switch(size+DT_START)
  {
    case DT_8:
      imm = (1 << 3);
      imminv = 8;
      break;

    case DT_16:
      imm = (1 << 4);
      imminv = 16;
      break;

    case DT_32:
      imm = (1 << 5);
      imminv = 32;
      break;

    case DT_64:
      l = 1;
      imminv = 64;
      break;

    default:
      FATAL(("Illegal datatype for modified immediate instruction"));
  }

  switch(opc)
  {
    case ARM_VSHR:
    case ARM_VSRA:
    case ARM_VRSHR:
    case ARM_VRSRA:
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
    case ARM_VSRI:
      *instr |= (imm | (imminv - ARM_INS_IMMEDIATE(ins))) << 16;
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      break;

    case ARM_VSHL_IMM:
    case ARM_VSLI:
      *instr |= (imm | ARM_INS_IMMEDIATE(ins)) << 16;
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      break;

    case ARM_VQSHLU_IMM:
      ASSERT(sizetype != DT_U_START, ("Illegal size-field for VQSHLU instruction"));
      *instr |= 0x01000000;
    case ARM_VQSHL_IMM:
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      *instr |= (imm | ARM_INS_IMMEDIATE(ins)) << 16;
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      break;

    case ARM_VQSHRN:
    case ARM_VQRSHRN:
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
    case ARM_VSHRN:
    case ARM_VRSHRN:
    case ARM_VQSHRUN:
    case ARM_VQRSHRUN:
      *instr |= (imm | (imminv - ARM_INS_IMMEDIATE(ins))) << 16;
      break;

    case ARM_VMOVL1:
    case ARM_VMOVL2:
    case ARM_VMOVL3:
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      switch(size+DT_START)
      {
        case DT_8:
          *instr |= 1 << 19;
          break;
        case DT_16:
          *instr |= 2 << 19;
          break;
        case DT_32:
          *instr |= 4 << 19;
          break;

        default:
          FATAL(("Illegal datatype"));
      }
      break;

    case ARM_VSHLL_IMM:
      *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      *instr |= (imm | ARM_INS_IMMEDIATE(ins)) << 16;
      break;

    case ARM_VCVT_FX:
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      *instr |= (64 - ARM_INS_IMMEDIATE(ins)) << 16;

      if(sizetype == DT_F_START)
      {
        *instr |= (ARM_INS_DATATYPEOP(ins) == DT_U32) ? 0x01000000 : 0;
      }
      else
      {
        /* op=1 */
        *instr |= 0x00000100;
        *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;
      }
      break;

    default:
      FATAL(("Illegal opcode: @I.", ins));
  }

  *instr |= (u << 24);
  *instr |= (a << 8);
  *instr |= (l << 7);
  *instr |= (b << 6);
}

void ArmAssembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 opc = ARM_INS_OPCODE(ins);
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (3 regs of different length) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  /* registers */
  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));
  *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));

  /* size and normalized size type */
  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  *instr |= (sizetype == DT_U_START) ? 0x01000000 : 0;

  switch(opc)
  {
    case ARM_VADDL:
    case ARM_VADDW:
      break;

    case ARM_VSUBL:
      break;
    case ARM_VSUBW:
      break;

    case ARM_VADDHN:
    case ARM_VRADDHN:
    case ARM_VSUBHN:
    case ARM_VRSUBHN:
      size--;
      break;

    case ARM_VABAL:
      break;
    case ARM_VABDL:
      break;
    case ARM_VMLAL:
      break;
    case ARM_VMLSL:
      break;
    case ARM_VQDMLAL:
      break;
    case ARM_VQDMLSL:
      break;
    case ARM_VMULL:
      break;
    case ARM_VQDMULL:
      break;
    case ARM_VMULL_POLY:
      break;

    default:
      FATAL(("Illegal opcode: @I", ins));
  }

  *instr |= (size << 20);
}
void ArmAssembleSIMDTransfer(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 l=0, c=0, a=0, b=0;
  t_uint32 op1=0, op2=0, s=0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (transfer) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VMOV_C2S:
      *instr |= ASM_NEON_VN_S(ARM_INS_REGA(ins));
      *instr |= ARM_INS_REGB(ins) << 12;
      break;

    case ARM_VMSR:
		if (ARM_INS_REGB(ins)==ARM_REG_CPSR)
		  *instr |= ARM_REG_R15 << 12;
      *instr |= ARM_INS_REGB(ins) << 12;
      break;

    case ARM_VMOV_C2SCALAR:
      s = ARM_INS_REGASCALAR(ins);

      *instr |= ASM_NEON_VN_QD(ARM_INS_REGA(ins));
      *instr |= ARM_INS_REGB(ins) << 12;

      switch(ARM_INS_DATATYPE(ins))
      {
        case DT_8:
          op1 = 2 | ((s & 4) >> 2);
          op2 = s & 3;
          break;

        case DT_16:
          op1 = (s & 2) >> 1;
          op2 = 1 | ((s & 1) << 1);
          break;

        case DT_32:
          op1 = s & 1;
          break;

        default:
          FATAL(("Illegal datatype"));
      }
      *instr |= (op1 << 21) | (op2 << 5);

      break;

    case ARM_VDUP:
      FATAL(("VDUP not handled in this function"));
      break;

    case ARM_VMOV_S2C:
      *instr |= ARM_INS_REGA(ins) << 12;
      *instr |= ASM_NEON_VN_S(ARM_INS_REGB(ins));
      break;

    case ARM_VMRS:
  		if (ARM_INS_REGA(ins)==ARM_REG_CPSR)
  		  *instr |= ARM_REG_R15 << 12;
      else
        *instr |= ARM_INS_REGA(ins) << 12;
      break;

    case ARM_VMOV_SCALAR2C:
      s = ARM_INS_REGBSCALAR(ins);

      *instr |= ARM_INS_REGA(ins) << 12;
      *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));

      switch(ARM_INS_DATATYPE(ins))
      {
        case DT_U8:
          *instr |= 0x00800000;
        case DT_S8:
          op1 = 2 | ((s & 4) >> 2);
          op2 = (s & 3);
          break;

        case DT_U16:
          *instr |= 0x00800000;
        case DT_S16:
          op1 = (s & 2) >> 1;
          op2 = 1 | ((s & 1) << 1);
          break;

        case DT_32:
          op1 = s & 1;
          break;

        default:
          FATAL(("Illegal datatype"));
      }
      *instr |= (op1 << 21) | (op2 << 5);

      break;

    default:
      FATAL(("Illegal opcode."));
  }

  *instr |= ASM_COND(ARM_INS_CONDITION(ins));
  *instr |= (a << 21);
  *instr |= (l << 20);
  *instr |= (c << 8);
  *instr |= (b << 5);
}

#define ASM_SIMD_LOADSTORE_ALIGN(x) ((x == 64) ? 1 : ((x == 128) ? 2 : ((x == 256) ? 3 : 0)))
//#define ASM_SIMD_LOADSTORE_ALIGN(x) 0
#define CORRECT_ALIGNMENT 1
void ArmAssembleSIMDLoadStore(t_arm_ins * ins, t_uint32 * instr)
{
  t_arm_ins_dt size = DT_NONE, sizetype = DT_NONE;
  t_uint32 align = 0, len = 0, totallen = 0, index_align = 0;
  t_reg dreg = 0;

  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM SIMD (load/store) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  /* Rm */
  if(ARM_INS_FLAGS(ins) & FL_WRITEBACK)
  {
    if (ARM_INS_REGC(ins) == ARM_REG_NONE)
      *instr |= ARM_REG_R13;
    else
      *instr |= ARM_INS_REGC(ins);
  }
  else if(ARM_INS_REGC(ins) == ARM_REG_NONE)
  {
    *instr |= ARM_REG_R15;
  }
  else
  {
    FATAL(("should not come here"));
    *instr |= ARM_INS_REGC(ins) - ARM_REG_R0;
  }

  /* alignment, set in opcode handlers */
  align = ARM_INS_MULTIPLEALIGNMENT(ins);

  /* size, set in opcode handlers */
  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  /* Vd: get first register set, afterwards get number of registers set */
  dreg = ASM_MULTIPLE_FIND_DREG_BLOB(ARM_INS_MULTIPLE(ins), &len);
  totallen = RegsetCountRegs(ARM_INS_MULTIPLE(ins));
  //DEBUG(("d%u len=%u total=%u", insregToQDregnum(dreg), len, totallen));
  *instr |= ASM_NEON_VD_QD(dreg);

  /* Rn */
  *instr |= ARM_INS_REGB(ins) << 16;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_VLD1_MULTI1OR3:
  case ARM_VLD1_MULTI2OR4:
  case ARM_VST1_MULTI1OR3:
  case ARM_VST1_MULTI2OR4:
    *instr |= ASM_SIMD_LOADSTORE_ALIGN(align) << 4;
    *instr |= (size & 3) << 6;

    *instr |= (((ARM_INS_OPCODE(ins) == ARM_VLD1_MULTI1OR3) || (ARM_INS_OPCODE(ins) == ARM_VST1_MULTI1OR3)) && (len == 1)) ? 0x00000100 : 0;
    *instr |= (((ARM_INS_OPCODE(ins) == ARM_VLD1_MULTI2OR4) || (ARM_INS_OPCODE(ins) == ARM_VST1_MULTI2OR4)) && (len == 2)) ? 0x00000800 : 0;
    break;

  case ARM_VLD1_ONE:
  case ARM_VST1_ONE:
    *instr |= (size & 3) << 10;

    switch(size+DT_START)
    {
      case DT_8:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 1);
        break;

      case DT_16:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 2);
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 2) ? 1 : 0;
        break;

      case DT_32:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 3);
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 4) ? 3 : 0;
        break;

      default:
        FATAL(("Illegal datatype"));
    }
    *instr |= (index_align << 4);
    break;

  case ARM_VLD1_ALL:
    *instr |= (size & 3) << 6;
    *instr |= (len == 2) ? 0x00000020 : 0;
    if (CORRECT_ALIGNMENT)
      *instr |= (align != 1) ? 0x00000010 : 0;
    break;

  case ARM_VLD2_MULTI1:
  case ARM_VLD2_MULTI2:
  case ARM_VST2_MULTI1:
  case ARM_VST2_MULTI2:
    *instr |= ASM_SIMD_LOADSTORE_ALIGN(align) << 4;
    *instr |= (size & 3) << 6;
    /* check if registers are consecutive; totallen > len if they are not consecutive */
    *instr |= (((ARM_INS_OPCODE(ins) == ARM_VLD2_MULTI1) || (ARM_INS_OPCODE(ins) == ARM_VST2_MULTI1)) && (totallen > len)) ? 0x00000100 : 0;
    *instr |= ((ARM_INS_OPCODE(ins) == ARM_VLD2_MULTI2) || (ARM_INS_OPCODE(ins) == ARM_VST2_MULTI2)) ? 0x00000300 : 0;
    break;

  case ARM_VLD2_ONE:
  case ARM_VST2_ONE:
    *instr |= (size & 3) << 10;

    switch(size+DT_START)
    {
      case DT_8:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 1);
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 16) ? 1 : 0;
        break;

      case DT_16:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 2);
        index_align |= (totallen > len) ? 2 : 0;
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 32) ? 1 : 0;
        break;

      case DT_32:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 3);
        index_align |= (totallen > len) ? 4 : 0;
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 64) ? 1 : 0;
        break;

      default:
        FATAL(("Illegal datatype"));
    }
    *instr |= (index_align << 4);
    break;

  case ARM_VLD2_ALL:
    *instr |= (size & 3) << 6;
    *instr |= (totallen > len) ? 0x00000020 : 0;
    if (CORRECT_ALIGNMENT)
      *instr |= (align != 1) ? 0x00000010 : 0;
    break;

  case ARM_VLD3_MULTI:
  case ARM_VST3_MULTI:
    *instr |= (size & 3) << 6;
    *instr |= (totallen > len) ? 0x00000100 : 0;
    if (CORRECT_ALIGNMENT)
      *instr |= (align == 64) ? 1 : 0;
    break;

  case ARM_VLD3_ONE:
  case ARM_VST3_ONE:
    *instr |= (size & 3) << 10;

    switch(size+DT_START)
    {
      case DT_8:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 1);
        break;

      case DT_16:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 2);
        index_align |= (totallen > len) ? 2 : 0;
        break;

      case DT_32:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 3);
        index_align |= (totallen > len) ? 4 : 0;
        break;

      default:
        FATAL(("Illegal datatype"));
    }
    *instr |= (index_align << 4);
    break;

  case ARM_VLD3_ALL:
    *instr |= (size & 3) << 6;
    *instr |= (totallen > len) ? 0x00000020 : 0;
    break;

  case ARM_VLD4_MULTI:
  case ARM_VST4_MULTI:
    *instr |= (size & 3) << 6;
    *instr |= ASM_SIMD_LOADSTORE_ALIGN(align) << 4;
    *instr |= (totallen > len) ? 0x00000100 : 0;
    break;

  case ARM_VLD4_ONE:
  case ARM_VST4_ONE:
    *instr |= (size & 3) << 10;

    switch(size+DT_START)
    {
      case DT_8:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 1);
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 32) ? 1 : 0;
        break;

      case DT_16:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 2);
        index_align |= (totallen > len) ? 2 : 0;
        if (CORRECT_ALIGNMENT)
          index_align |= (align == 64) ? 1 : 0;
        break;

      case DT_32:
        index_align = (ARM_INS_MULTIPLESCALAR(ins) << 3);
        index_align |= (totallen > len) ? 4 : 0;

        switch(align)
        {
          case 1:
            break;

          case 64:
            if (CORRECT_ALIGNMENT)
              index_align |= 1;
            break;

          case 128:
            if (CORRECT_ALIGNMENT)
              index_align |= 2;
            break;

          default:
            FATAL(("Illegal alignment: %u", align));
        }
        break;

      default:
        FATAL(("Illegal datatype"));
    }
    *instr |= (index_align << 4);
    break;

  case ARM_VLD4_ALL:
    *instr |= (size & 3) << 6;
    *instr |= (totallen > len) ? 0x00000020 : 0;
    if (CORRECT_ALIGNMENT)
      *instr |= (align != 1) ? 0x00000010 : 0;
    break;

  default:
    FATAL(("Illegal opcode: @I", ins));
  }
}

void ArmAssembleFPLoadStore(t_arm_ins * ins, t_uint32 * instr)
{
  FATAL(("implement: @I", ins));
}

void ArmAssembleFPDataProc(t_arm_ins * ins, t_uint32 * instr)
{
  FATAL(("implement: @I", ins));
}

void ArmAssembleFP2R(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM FP (2 registers) @I", ins));

  /* set opcode */
  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  *instr |= (ARM_INS_CONDITION(ins) << 28);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VMOV64_C2S:
      if((ARM_REG_R0 <= ARM_INS_REGA(ins)) && (ARM_INS_REGA(ins) <= ARM_REG_R15))
      {
        /* to core */
        *instr |= (ARM_INS_REGA(ins) << 12);
        *instr |= (ARM_INS_REGABIS(ins) << 16);
        *instr |= ASM_NEON_VM_S(ARM_INS_REGB(ins));
        *instr |= 0x00100000;
      }
      else
      {
        /* to single */
        *instr |= ASM_NEON_VM_S(ARM_INS_REGA(ins));
        *instr |= (ARM_INS_REGB(ins) << 12);
        *instr |= (ARM_INS_REGC(ins) << 16);
      }
      break;

    case ARM_VMOV64_C2D:
      if((ARM_REG_R0 <= ARM_INS_REGA(ins)) && (ARM_INS_REGA(ins) <= ARM_REG_R15))
      {
        /* to core */
        *instr |= (ARM_INS_REGA(ins) << 12);
        *instr |= (ARM_INS_REGABIS(ins) << 16);
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGB(ins));
        *instr |= 0x00100000;
      }
      else
      {
        /* to double */
        *instr |= ASM_NEON_VM_QD(ARM_INS_REGA(ins));
        *instr |= (ARM_INS_REGB(ins) << 12);
        *instr |= (ARM_INS_REGC(ins) << 16);
      }
      break;

    default:
      FATAL(("Illegal opcode: @I", ins));
  }
}

void ArmAssembleBitfield(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM bitfield @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;
  *instr |= ASM_COND(ARM_INS_CONDITION(ins));

  *instr |= ARM_INS_REGA(ins) << 12;

  if(ARM_INS_REGB(ins) != ARM_REG_NONE)
    *instr |= ARM_INS_REGB(ins);

  if(ARM_INS_OPCODE(ins) != ARM_RBIT)
  {
    /* bitfield msb */
    *instr |= ARM_INS_IMMEDIATE(ins) & 0x001f0000;
    /* bitfield lsb */
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x0000001f) << 7;
  }
}

void ArmAssembleNOP(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM NOP @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;
  *instr |= ARM_INS_CONDITION(ins) << 28;
}

void ArmAssembleDivision(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM division @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;
  *instr |= ARM_INS_CONDITION(ins) << 28;

  ASSERT((ARM_INS_OPCODE(ins)==ARM_SDIV) || (ARM_INS_OPCODE(ins)==ARM_UDIV), ("Illegal assemble function called for %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));

  *instr |= ARM_INS_REGA(ins) << 16;
  *instr |= ARM_INS_REGB(ins);
  *instr |= ARM_INS_REGC(ins) << 8;
}

void ArmAssembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM load/store exclusive @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;
  *instr |= ARM_INS_CONDITION(ins) << 28;

  switch (ARM_INS_OPCODE(ins))
  {
  case ARM_STREX:
  case ARM_STREXB:
  case ARM_STREXH:
  case ARM_STREXD:
    *instr |= ARM_INS_REGA(ins);
    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGC(ins) << 12;
    break;

  case ARM_LDREX:
  case ARM_LDREXB:
  case ARM_LDREXH:
  case ARM_LDREXD:
    *instr |= ARM_INS_REGA(ins) << 12;
    *instr |= ARM_INS_REGB(ins) << 16;
    break;

  default:
    FATAL(("illegal instruction: @I", ins));
  }
}

void ArmAssembleHint(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(ARM_ASSEMBLER_VERBOSITY_LEVEL, ("assembling ARM hint @I", ins));

  *instr = arm_opcode_table[ARM_INS_OPCODE(ins)].opcode;

  if(ARM_INS_ATTRIB(ins) & IF_CONDITIONAL)
    *instr |= ASM_COND(ARM_INS_CONDITION(ins)); /* set conditionbits */

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_YIELD:
    case ARM_WFE:
    case ARM_WFI:
    case ARM_SEV:
      /* no action needed */
      break;

    case ARM_DMB:
    case ARM_DSB:
    case ARM_ISB:
    case ARM_DBG:
      {
        /* encode option */
        *instr |= ARM_INS_IMMEDIATE(ins) & 0x0000000f;
      }
      break;

    default:
      FATAL(("illegal instruction: @I", ins));
  }
}

/* }}} */


/* vim: set shiftwidth=2 foldmethod=marker : */
