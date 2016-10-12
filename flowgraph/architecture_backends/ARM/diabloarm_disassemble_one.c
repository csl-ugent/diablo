/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/* Condition Codes */
#define ARM_EXTRACT_CONDITION_CODE(i) (((i) & 0xf0000000) >> 28)
#define EXTRACT_VFP_SYSREGFIELD_FROM_INSTR(x) (((x)&0x000f0000)>>16)
#define VFP_SYSREG_FIELD_TO_ARM_REG(x) ((x)==0?ARM_REG_FPSID:(x)==1?ARM_REG_FPSCR:(x)==8?ARM_REG_FPEXC:ARM_REG_NONE)

/*!
 * Utility function: decodes an immediate shift operation
 *
 * \param ins The arm instruction to fill
 * \param instr The encode arm instruction
 *
 * \return void
*/
/* Utility Function: ArmShiftDecodeImm{{{ */
static void ArmShiftDecodeImm(t_arm_ins * ins, int shifttyp, int shiftlen) {

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_SHIFTTYPE(ins,  shifttyp);
  ARM_INS_SET_SHIFTLENGTH(ins,  shiftlen);
  /* shift length 0 is always a special case */
  if (ARM_INS_SHIFTLENGTH(ins) == 0)
  {
    if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM)
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    else if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ROR_IMM)
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_RRX);
    else if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_IMM ||
	ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM)
      ARM_INS_SET_SHIFTLENGTH(ins,  32);
  }
}
/*}}}*/
/*!
 * utility function: fills in the fields that describe a shift in t_arm_ins
 *
 * \param ins the arm instruction to fill
 * \param instr the encode arm instruction
 *
 * \return void
*/
/* utility function: ArmShiftDecode{{{ */
static void ArmShiftDecode(t_arm_ins * ins, t_uint32 instr) {

  if (instr & (1 << 4)) {
    /* shift length is in a register */
    ARM_INS_SET_REGS(ins,  (instr & 0x00000f00) >> 8);
    ARM_INS_SET_SHIFTLENGTH(ins,  0);
    ARM_INS_SET_SHIFTTYPE(ins,  ((instr & 0x00000060) >> 5)+4);
  } else {
    /* shift length is an immediate */
    ArmShiftDecodeImm(ins, (instr & 0x00000060) >> 5, (instr & 0x00000f80) >> 7);
  }
}
/*}}}*/
/*!
 * Disassemble Arm Branches (B, BL, BX)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleBranch {{{ */
void ArmDisassembleBranch(t_arm_ins * ins,  t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_REGA(ins,   ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_BRANCH);

  /* next, prev en Bblock moeten nog ingevuld worden */
  switch (opc) {
  case ARM_B:
  case ARM_BL:
    ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
    ARM_INS_SET_IMMEDIATE(ins,  (t_int32) (4 * Uint32SignExtend(instr & 0x00ffffff,23)));
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_IMMED);
    break;
  case ARM_BX:
    ARM_INS_SET_IMMEDIATE(ins,  0);
    ARM_INS_SET_REGB(ins,  instr & 0x0000000f);
    break;
  case ARM_BLX:
    ARM_INS_SET_REGB(ins,  instr & 0x0000000f);
    ARM_INS_SET_IMMEDIATE(ins,  0);
    break;
  default:
    FATAL(("Unknown branch instruction\n"));
  }
  /* patch because a special kind of BLX was wrongly disassembled */
  if (ARM_INS_CONDITION(ins) == ARM_CONDITION_NV && (opc == ARM_B || opc == ARM_BL))
  {
    ARM_INS_SET_OPCODE(ins, ARM_BLX);
    ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
    ARM_INS_SET_ATTRIB(ins,  ARM_INS_ATTRIB(ins) & (~IF_CONDITIONAL));
    ARM_INS_SET_IMMEDIATE(ins,  ARM_INS_IMMEDIATE(ins) + ((instr & 0x01000000)?2:0));
  }
}
/*}}}*/
/*!
 * Disassemble Arm Software Interrupts (SWI)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
 * ArmDisassembleSWI {{{ */
void ArmDisassembleSWI(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  FL_IMMED);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_SWI);

  ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00ffffff);
}
/*}}}*/
/*!
 * Disassemble Arm software breakpoint (BKPT)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
 * ArmDisassembleBKPT {{{ */
void ArmDisassembleBKPT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    FATAL(("BKPT instruction must be unconditional"));
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  FL_IMMED);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_SWI);

  ARM_INS_SET_IMMEDIATE(ins, ((instr & 0x000fff00) >> 4) | (instr & 0xf));
}
/*}}}*/
/*!
 * Disassemble Arm Multiplies
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleMultiplication {{{ */
void ArmDisassembleMultiplication(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
  /* Rd/RdHi = RegA */
  /* Rn/RdLo = RegS */
  /* Rs      = RegC */
  /* Rm      = RegB */

  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_MUL);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_IMMEDIATE(ins,  0);

  ARM_INS_SET_FLAGS(ins,  0x0);

  switch (opc)
  {
    case ARM_MUL:
    case ARM_MLA:
    case ARM_MLS:
    case ARM_UMULL:
    case ARM_UMLAL:
    case ARM_SMULL:
    case ARM_SMLAL:
      if (instr & (1 << 20))
	ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) | FL_S);
      break;
    default:
      /* other multiply instructions don't have an s-bit */
      break;
  }

  /* special: use RegS as fourth registers for MUL-variants */
  /* so as the lower part of the result for 64bit multiplications,
   * or as Rn for MLA etc.
   */
  ARM_INS_SET_REGA(ins,  (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGB(ins,  (instr & 0x0000000f));
  ARM_INS_SET_REGC(ins,  (instr & 0x00000f00) >> 8);
  ARM_INS_SET_REGS(ins,  (instr & 0x0000f000) >> 12);

  /* Some multiply instructions don't use a fourth register,
   * so set it to ARM_REG_NONE
   */
  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_MUL:
    case ARM_SMMUL:
    case ARM_SMUAD:
    case ARM_SMUADX:
    case ARM_SMUSD:
    case ARM_SMUSDX:
      ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
      break;
    default:
      /* do nothing */
      break;
  }

}
/*}}}*/
/*!
 * Disassemble Arm Multiple Loads/Stores (LDM, STM)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleMultipleTransfer {{{ */
void ArmDisassembleMultipleTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  switch(opc)
  {
     case ARM_LDM:
	ARM_INS_SET_TYPE(ins,  IT_LOAD_MULTIPLE);
	break;
     case ARM_STM:
	ARM_INS_SET_TYPE(ins,  IT_STORE_MULTIPLE);
	break;
     default:
	FATAL(("IMPLEMENT %s", arm_opcode_table[opc].desc));
  }
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_IMMEDIATE(ins,  0 | (instr & 0x0000ffff));

  ARM_INS_SET_FLAGS(ins,  0x0);

  if (instr & (1 << 24))
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_PREINDEX);
  if (instr & (1 << 23))
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_DIRUP);
  if (instr & (1 << 21))
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_WRITEBACK);

  if (instr & (1 << 22))
  {
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_USERMODE_REGS);
    if (opc == ARM_STM || (opc == ARM_LDM && !(instr & (1 << 15))))
      ASSERT(!(instr & (1 << 21)),("supervisor mode ldm or stm with invalid writeback"));
  }

  ARM_INS_SET_REGA(ins,   ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
}
/*}}}*/
/*!
 * Disassemble Arm Swap instructions (SWP)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleSwap{{{*/
void ArmDisassembleSwap(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_SWAP);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_IMMEDIATE(ins,  0);

  ARM_INS_SET_FLAGS(ins,  0x0);

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGC(ins,  (instr & 0x0000000f));
}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleMRS {{{ */
void ArmDisassembleMRS(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_STATUS);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_FLAGS(ins,  0x0);

  if (instr & (1 << 22))
    ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_SPSR);

  ARM_INS_SET_IMMEDIATE(ins,  0);
  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc  The opcode
 *
 * \return void
*/
/* ArmDisassembleMSR {{{ */
void ArmDisassembleMSR(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
   ARM_INS_SET_OPCODE(ins,  opc);
   ARM_INS_SET_TYPE(ins,  IT_STATUS);
   ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
   ARM_INS_SET_ATTRIB(ins,  0x0);
   if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
     ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
   ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
   ARM_INS_SET_SHIFTLENGTH(ins,  0);
   ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

   ARM_INS_SET_FLAGS(ins,  0x0);

   /* ARM DDI 01000E p.62-63:
    * if R == 1 we modify the SPSR
    * else we modify the CPSR */

   if (instr & (1 << 22))
     ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) | FL_SPSR);

   if (instr & (1<<16))
     ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) | FL_CONTROL);
   if (instr & (1<<17)) FATAL(("Sets extension bits"));
   if (instr & (1<<18)) FATAL(("Sets status bits"));
   if (instr & (1<<19))
     ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) |FL_STATUS);

   /* if the instruction writes the status field to the cpsr,
    * it affects the status bits. if it writes to the spsr,
    * it affects the status bits of a different processor mode,
    * so the S flag should not be set */
   if ((ARM_INS_FLAGS(ins) & FL_STATUS) && !(ARM_INS_FLAGS(ins) & FL_SPSR))
     ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins)| FL_S);


   ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
   ARM_INS_SET_REGB(ins,  ARM_REG_NONE);

   if (instr & (1 << 25)) {
      /* operand is een immediate */
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(ins,  Uint32RotateRight(instr & 0x000000ff, 2 * ((instr & 0x00000f00) >> 8)));
      ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins)| FL_IMMED);
   } else {
      /* operand zit in een register */
      ARM_INS_SET_IMMEDIATE(ins,  0);
      ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
   }
}
/*}}}*/
/*!
 * Disassemble normal ARM Loads and Stores (LDR, LDRB, STRH, ...)
 *
 * \param ins The arm instruction that gets filled in
 * \param instr The encoded arm instruction
 * \param opc The opcode
 *
 * \return void
*/
/* ArmDisassembleDataTransfer {{{ */
void ArmDisassembleDataTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  if ((ARM_INS_OPCODE(ins) == ARM_PLD) || (ARM_INS_OPCODE(ins) == ARM_PLDW) || (ARM_INS_OPCODE(ins) == ARM_PLI))
  {
    ARM_INS_SET_TYPE(ins,  IT_UNKNOWN);
    /* PLD is unconditional */
    ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
    ARM_INS_SET_ATTRIB(ins,   ARM_INS_ATTRIB(ins) & ~IF_CONDITIONAL);
  }
  else if (
      (ARM_INS_OPCODE(ins) == ARM_LDR)    ||
      (ARM_INS_OPCODE(ins) == ARM_LDRD)   ||
      (ARM_INS_OPCODE(ins) == ARM_LDRB)   ||
      (ARM_INS_OPCODE(ins) == ARM_LDRH)   ||
      (ARM_INS_OPCODE(ins) == ARM_LDRSH)  ||
      (ARM_INS_OPCODE(ins) == ARM_LDRSB)  ||
      (ARM_INS_OPCODE(ins) == ARM_LDREX)  ||
      (ARM_INS_OPCODE(ins) == ARM_LDREXB) ||
      (ARM_INS_OPCODE(ins) == ARM_LDREXD) ||
      (ARM_INS_OPCODE(ins) == ARM_LDREXH))
    ARM_INS_SET_TYPE(ins,  IT_LOAD);
  else if (
      (ARM_INS_OPCODE(ins) == ARM_STR)    ||
      (ARM_INS_OPCODE(ins) == ARM_STRD)   ||
      (ARM_INS_OPCODE(ins) == ARM_STRB)   ||
      (ARM_INS_OPCODE(ins) == ARM_STRH)   ||
      (ARM_INS_OPCODE(ins) == ARM_STREX)  ||
      (ARM_INS_OPCODE(ins) == ARM_STREXB) ||
      (ARM_INS_OPCODE(ins) == ARM_STREXD) ||
      (ARM_INS_OPCODE(ins) == ARM_STREXH))
    ARM_INS_SET_TYPE(ins,  IT_STORE);
  else
    FATAL(("Don't know the instruction type for this opcode"));

  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_FLAGS(ins,  0x0);

  switch (opc) {
  case ARM_STR:
  case ARM_LDR:
  case ARM_STRB:
  case ARM_LDRB:
  case ARM_PLD:
  case ARM_PLDW:
  case ARM_PLI:
    ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
    ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);

    if (ARM_INS_OPCODE(ins)==ARM_PLI) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
    if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
    else                   ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
    if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
    if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

    if ((ARM_INS_OPCODE(ins) == ARM_PLD) || (ARM_INS_OPCODE(ins) == ARM_PLDW) || (ARM_INS_OPCODE(ins) == ARM_PLI))
    {
      /* there is no destination register for a PLD, as it
       * is just a cache hint, not an actual instruction */
      ARM_INS_SET_REGA(ins,  ARM_REG_NONE);

      /* PLD instructions follow the ldr addressing modes, with
       * the restriction that P==1 && W==0. Check this. */
      if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) || !(ARM_INS_FLAGS(ins) & FL_PREINDEX))
        FATAL(("PLD instruction should have P==1 && W==0"));
    }

    if (instr & (1 << 25))
    {
      /* offset in offsetregister */
      ARM_INS_SET_IMMEDIATE(ins,  0);
      ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
      ArmShiftDecode(ins,instr);
    }
    else
    {
      /* offset gespecifieerd als 12-bit immediate */
      ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00000fff);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
    }
    break;

  case ARM_STRH:
  case ARM_LDRH:
  case ARM_LDRSH:
  case ARM_LDRSB:
  case ARM_LDRD:
  case ARM_STRD:
    ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
    ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);

    if (opc == ARM_LDRD || opc == ARM_STRD)
    {
      ASSERT((ARM_INS_REGA(ins) != ARM_REG_R14), ("ARM double-word transfers cannot use r14/r15: addr @G",ARM_INS_OLD_ADDRESS(ins)));

      ARM_INS_SET_REGABIS(ins, ARM_INS_REGA(ins)+1);
    }

    if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
    else                   ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
    if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
    if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

    if (instr & (1 << 22))
    {
      /* offset gespecifieerd als 8-bit immediate */
      ARM_INS_SET_IMMEDIATE(ins,  ((instr & 0x00000f00) >> 4) | (instr & 0x000000f));
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
    }
    else
    {
    	/* offset in offsetregister */
    	ARM_INS_SET_IMMEDIATE(ins,  0);
    	ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
    	ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    	ARM_INS_SET_SHIFTLENGTH(ins,  0);
    }
    break;

  case ARM_STREX:
  case ARM_STREXB:
  case ARM_STREXH:
  case ARM_STREXD:
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);

    /* just like a regular store ... */
    ARM_INS_SET_REGA(ins, instr & 0x0000000f);
    if (ARM_INS_OPCODE(ins) == ARM_STREXD)
    {
      ASSERT(ARM_INS_REGA(ins) < 14, ("STREXD instruction can not have R14 as the first source register: addr @G", ARM_INS_OLD_ADDRESS(ins)));
      ARM_INS_SET_REGABIS(ins, ARM_INS_REGA(ins)+1);
    }

    ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

    /* ... EXCEPT for the return value */
    ARM_INS_SET_REGC(ins, (instr & 0x0000f000) >> 12);

    ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    ARM_INS_SET_SHIFTLENGTH(ins,  0);
    break;

  case ARM_LDREX:
  case ARM_LDREXB:
  case ARM_LDREXH:
  case ARM_LDREXD:
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);

    /* just like a regular load */
    ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
    if (ARM_INS_OPCODE(ins) == ARM_LDREXD)
    {
      ASSERT(ARM_INS_REGA(ins) < 14, ("LDREXD instruction can not have R14 as the first destination register: addr @G", ARM_INS_OLD_ADDRESS(ins)));
      ARM_INS_SET_REGABIS(ins, ARM_INS_REGA(ins)+1);
    }

    ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
    ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    ARM_INS_SET_SHIFTLENGTH(ins,  0);
    break;

  default:
    FATAL(("illegal opcode"));
  }

}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleDataProcessing {{{ */
void ArmDisassembleDataProcessing(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);

  ARM_INS_SET_FLAGS(ins,  0x0);

  if (instr & (1 << 20)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);

  if (instr & (1 << 25)) {
    /* operand 2 is een immediate */
    ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
    ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    ARM_INS_SET_SHIFTLENGTH(ins,  0);
    ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

    /* MOVW/MOVT has a 16 bit immediate */
    if ((opc != ARM_MOVW) && (opc != ARM_MOVT))
    {
      ARM_INS_SET_IMMEDIATE(ins,  Uint32RotateRight(instr & 0x000000ff, 2 * ((instr & 0x00000f00) >> 8)));
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
    }
    else
    {
      ARM_INS_SET_IMMEDIATE(ins,  ((instr & 0x000f0000) >> 4) | (instr & 0x00000fff));
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMEDW);
    }


  } else {
    /* operand 2 is een register */
    ARM_INS_SET_IMMEDIATE(ins,  0);
    ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
    ArmShiftDecode(ins,instr);
  }

  if(ARM_INS_OPCODE(ins) == ARM_CLZ)
  {
    ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
    ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
    ARM_INS_SET_SHIFTLENGTH(ins,  0);
  }

  /* enkele speciale gevallen waarbij een van de registers genegeerd wordt */
  if ((opc == ARM_MOV) || (opc == ARM_MOVW) || (opc == ARM_MVN) || (opc == ARM_CLZ) || (opc == ARM_MOVT))
    ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  if ((opc == ARM_CMP) || (opc == ARM_CMN) || (opc == ARM_TST) || (opc == ARM_TEQ))
    ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleUnsupported {{{ */
void ArmDisassembleUnsupported(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
   FATAL(("Unsupported arm instruction. Opcode = %d (%s)\n"
	  "full text 0x%x",opc,arm_opcode_table[opc].desc,instr));
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* Data {{{ */
void ArmDisassembleData(t_arm_ins * ins,t_uint32 instr, t_uint16 opc ) {
  /* fills in a arm_ins with the appropriate values if we're dealing with a data word */
  ARM_INS_SET_TYPE(ins,  IT_DATA);
  ARM_INS_SET_OPCODE(ins,  ARM_DATA);
  ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_IMMEDIATE(ins,  instr);
  ARM_INS_SET_FLAGS(ins,  0);
  ARM_INS_SET_ATTRIB(ins,  IF_DATA);
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleGenericCoproc  {{{ */
void ArmDisassembleGenericCoproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* This function disassembles MCR, MCR, LDC and STC instructions.
   * These are instructions that interface with coprocessors, and as
   * such they have compeletely different fields from the regular
   * instructions. Most of these fields will be encoded in the
   * immediate field of the instruction. This isn't pretty, but it
   * will do.
   *
   * MCR/MRC
   * -------
   * bit 0-3: coproc select
   * bit 4-6: opcode 1
   * bit 7-9: opcode 2
   * bit 10-13: CRm
   * bit 14-17: CRn
   *
   * MCRR/MRRC
   * ---------
   * bit 0-3: coproc select
   * bit 4-7: opcode
   * bit 8-11: CRm
   *
   * LDC/STC
   * -------
   * bit 0-3: coproc select
   * bit 4-7: CRd
   * bit 8-15: offset_8
   */

  int coproc,opc1,opc1long,opc2,Rm,Rn,Rd,offset8;

  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_TYPE(ins,  IT_UNKNOWN);

  ARM_INS_SET_IMMEDIATE(ins,  0x0);
  coproc = (instr & 0xf00) >> 8;
  Rn = (instr & 0xf0000) >> 16;
  Rd = (instr & 0xf000) >> 12;
  Rm = (instr & 0xf);
  offset8 = (instr & 0xff);
  opc1 = (instr & 0x00e00000) >> 21;
  opc1long = (instr & 0x00f00000) >> 20;
  opc2 = (instr & 0x000000e0) >> 5;

  switch (opc)
  {
    case ARM_MCR2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_MCR:
      ARM_INS_SET_IMMEDIATE(ins,  (Rn << 14) | (Rm << 10) | (opc2 << 7) | (opc1 << 4) | coproc);
      ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  Rd);
      break;
    case ARM_MRC2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_MRC:
      ARM_INS_SET_IMMEDIATE(ins,  (Rn << 14) | (Rm << 10) | (opc2 << 7) | (opc1 << 4) | coproc);
      ARM_INS_SET_REGA(ins,  Rd);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      /* special case: Rd == r15 => condition flags are defined instead */
      if (Rd == 15)
      {
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
	ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
      }
      break;
    case ARM_MRRC2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_MRRC:
      opc1 = (instr & 0xf0) >> 4;
      ARM_INS_SET_IMMEDIATE(ins,  (Rm << 8) | (opc1 << 4) | coproc);
      ARM_INS_SET_REGA(ins,  Rd);
      ARM_INS_SET_REGB(ins,  Rn);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      break;
    case ARM_MCRR2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_MCRR:
      opc1 = (instr & 0xf0) >> 4;
      ARM_INS_SET_IMMEDIATE(ins,  (Rm << 8) | (opc1 << 4) | coproc);
      ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGB(ins,  Rd);
      ARM_INS_SET_REGC(ins,  Rn);
      break;
    case ARM_LDC2:
    case ARM_STC2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_LDC:
    case ARM_STC:
      ARM_INS_SET_IMMEDIATE(ins,  (offset8 << 8) | (Rd << 4) | coproc);
      ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGB(ins,  Rn);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      if (instr & (1 << 22)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_LONG_TRANSFER);
      if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
      if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
      if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
      break;

    case ARM_CDP2:
      ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
    case ARM_CDP:
      ARM_INS_SET_IMMEDIATE(ins, (opc1long << 20) | (Rn << 16) | (Rd << 12) | (coproc << 8) | (opc2 << 5) | (Rm));
      break;

    default:
      FATAL(("Invalid opcode"));
  }

  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

}
/* }}} */

/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleCPDO  {{{ */
void ArmDisassembleCPDO(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_ATTRIB(ins,  0x0);

  if ((ARM_INS_OPCODE(ins)!=ARM_CDP2))
  {
    ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
    if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
      ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  }

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);


  ARM_INS_SET_TYPE(ins,  IT_FLT_ALU);

  if ((instr & (1<<19)) && (instr & (1<<7)))
    FATAL(("Illegal size for float"));
  else if (instr & (1<<19))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE_EXTENDED);
  else if (instr & (1<<7))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE);
  else
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_SINGLE);


   switch(opc)
   {
     case ARM_MVF:
     case ARM_MNF:
     case ARM_SQT:
     case ARM_ABS:
       ARM_INS_SET_REGA(ins,  ARM_REG_F0+ ((instr & 0x00007000) >> 12));
       if (instr & 0x8)
       {
	 ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_IMMED | FL_IMMED);
	 ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x00000007));
	 ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
       }
       else
       {
	 ARM_INS_SET_REGB(ins,  ARM_REG_F0+(instr & 0x00000007) );
       }
       ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
       /* Rounding mode */
       if (instr & (1 << 6))
       {
	 if (instr & (1 << 5)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RZ);
	 else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RMI);
       }
       else if (instr & (1 << 5)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RPI);
       break;
     case ARM_ADF:
     case ARM_SUF:
     case ARM_RSF:
     case ARM_MUF:
     case ARM_DVF:
     case ARM_RDF:
     case ARM_FML:
     case ARM_FDV:
     case ARM_FRD:
       ARM_INS_SET_REGA(ins,  ARM_REG_F0+ ((instr & 0x00007000) >> 12));
       ARM_INS_SET_REGB(ins,  ARM_REG_F0+ ((instr & 0x00070000) >> 16));
       if (instr &0x8)
       {
	 ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x00000007));
	 ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_IMMED | FL_IMMED);
	 ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	 /*FATAL(("Implement Immediate operands!"));*/
       }
       else
       {
	 ARM_INS_SET_REGC(ins,  ARM_REG_F0+ (instr & 0x7) );
       }
       break;

     default:
       FATAL(("Unknown CPDO: Opcode = %d for ins at @G\n",opc,ARM_INS_OLD_ADDRESS(ins)));
   }


}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleCPRT {{{ */
void ArmDisassembleCPRT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);


  switch(opc)
  {
	case ARM_FLT:
	  if ((instr & (1<<19)) && (instr & (1<<7)))
	    FATAL(("Illegal size for float"));
	  else if (instr & (1<<19))
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE_EXTENDED);
	  else if (instr & (1<<7))
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE);
	  else
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_SINGLE);

	  ARM_INS_SET_TYPE(ins, IT_FLT_INT);
	  ARM_INS_SET_REGA(ins,  ARM_REG_F0+ ((instr & 0x000f0000) >> 16));
	  ARM_INS_SET_REGB(ins,  (instr & 0x0000f000) >> 12);
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  break;
	case ARM_FIX:
	  if ((instr & (1 << 19)) && (instr & (1 << 7)))
	    FATAL(("Bits should be zero!"));
	  /* Ignoring rounding mode for the moment! */
	  ARM_INS_SET_TYPE(ins, IT_FLT_INT);
	  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
	  ARM_INS_SET_REGB(ins,  ARM_REG_F0+ (instr & 0x0000000f));
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  if (instr & (1 << 6))
	  {
	    if (instr & (1 << 5)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RZ);
	    else ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RMI);
	  }
	  else if (instr & (1 << 5)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_RPI);
	  break;
	case ARM_WFS:
	  ARM_INS_SET_TYPE(ins, IT_FLT_STATUS);
	  ARM_INS_SET_IMMEDIATE(ins,  0);
	  ARM_INS_SET_REGA(ins, ARM_REG_FPSR);
	  ARM_INS_SET_REGB(ins,  (instr & 0x0000f000) >> 12);
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  break;
	case ARM_RFS:
	  ARM_INS_SET_TYPE(ins, IT_FLT_STATUS);
	  ARM_INS_SET_IMMEDIATE(ins,  0);
	  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
	  ARM_INS_SET_REGB(ins, ARM_REG_FPSR);
	  ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  break;
	case ARM_WFC:
	  FATAL(("implement wfc"));
	  break;
	case ARM_RFC:
	  FATAL(("implement rfc"));
	  break;
	case ARM_CMFE:
	case ARM_CNFE:
	case ARM_CMF:
	case ARM_CNF:

	  ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
	  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
	  ARM_INS_SET_TYPE(ins,  IT_FLT_ALU);
	  ARM_INS_SET_REGB(ins,  ARM_REG_F0+ ((instr & 0x00070000) >> 16));
	  if (instr & 0x8)
	  {
	    ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x00000007));
	    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_FLT_IMMED | FL_IMMED);
	    ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	  }
	  else
	    ARM_INS_SET_REGC(ins,  ARM_REG_F0+ (instr & 0x00000007));
	  break;

	default:
	  FATAL(("Implement unknown CPRT"));
  }
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleCPDT {{{ */
void ArmDisassembleCPDT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  if ((ARM_INS_OPCODE(ins) == ARM_LFM) ||(ARM_INS_OPCODE(ins) == ARM_LDF))
    ARM_INS_SET_TYPE(ins,  IT_FLT_LOAD);
  else
    ARM_INS_SET_TYPE(ins,  IT_FLT_STORE);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_FLAGS(ins,  0x0);

  switch (opc) {
    case ARM_STF:
    case ARM_LDF:
      if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
      if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
      if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
      if ((instr & (1<<22)) && (instr & (1<<15)))
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_PACKED);
      else if (instr & (1<<22))
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE_EXTENDED);
      else if (instr & (1<<15))
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_DOUBLE);
      else
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_FLT_SINGLE);



      ARM_INS_SET_REGA(ins,  ARM_REG_F0+ ((instr & 0x00007000) >> 12));
      ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);

      /* offset gespecifieerd als 8-bit immediate */
      ARM_INS_SET_IMMEDIATE(ins,  4*(instr & 0x000000ff));
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      break;

    case ARM_SFM:
    case ARM_LFM:
      {
	int num;
	int tel;
	if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
	if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
	if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
	ARM_INS_SET_IMMEDIATE(ins,  instr & 0x000000ff);
	ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
	ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
	ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
	ARM_INS_SET_SHIFTLENGTH(ins,  0);
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
	ARM_INS_SET_MULTIPLE(ins, RegsetNew());

	num=instr &(1<< 22)?2:0;
	num+=instr & (1 << 15)?1:0;
	if (num==0) num=4;
	for (tel=0; tel<num; tel++)
	{
	  if ((((instr&0x7000)>>12)+ARM_REG_F0+tel) > ARM_REG_F7) FATAL(("Implement wrap around in float load/store multiple"));
	  ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins),((instr&0x7000)>>12)+ARM_REG_F0+tel));
	}
	ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
      }

      break;
  }

}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* {{{ ArmDisassembleDSP */
void ArmDisassembleDSP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc )
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);

  if (ARM_INS_OPCODE(ins) == ARM_QADD ||
      ARM_INS_OPCODE(ins) == ARM_QDADD ||
      ARM_INS_OPCODE(ins) == ARM_QSUB ||
      ARM_INS_OPCODE(ins) == ARM_QDSUB)
  {
    ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
    ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
    ARM_INS_SET_REGC(ins,  (instr & 0x0000000f));
    ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
    ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  }
  else if (
      ARM_INS_OPCODE(ins) == ARM_SMLABB ||
      ARM_INS_OPCODE(ins) == ARM_SMLABT ||
      ARM_INS_OPCODE(ins) == ARM_SMLATB ||
      ARM_INS_OPCODE(ins) == ARM_SMLATT ||
      ARM_INS_OPCODE(ins) == ARM_SMLAWB ||
      ARM_INS_OPCODE(ins) == ARM_SMLAWT ||
      ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
      ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
      ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
      ARM_INS_OPCODE(ins) == ARM_SMLALTT)
  {
    /* follow the register conventions of a regular MLA:
     * Rd/RdHi -> REGA
     * Rn/RdLo -> REGS
     * Rm -> REGB
     * Rs -> REGC
     */
    ARM_INS_SET_TYPE(ins,  IT_MUL);
    ARM_INS_SET_REGA(ins,  (instr & 0x000f0000) >> 16);
    ARM_INS_SET_REGB(ins,  (instr & 0x0000000f));
    ARM_INS_SET_REGC(ins,  (instr & 0x00000f00) >> 8);
    ARM_INS_SET_REGS(ins,  (instr & 0x0000f000) >> 12);
  }
  else if (
      ARM_INS_OPCODE(ins) == ARM_SMULBB ||
      ARM_INS_OPCODE(ins) == ARM_SMULBT ||
      ARM_INS_OPCODE(ins) == ARM_SMULTB ||
      ARM_INS_OPCODE(ins) == ARM_SMULTT ||
      ARM_INS_OPCODE(ins) == ARM_SMULWB ||
      ARM_INS_OPCODE(ins) == ARM_SMULWT)
  {
    /* follow the register conventions of a regular MUL:
     * Rd -> REGA
     * Rm -> REGB
     * Rs -> REGC
     */
    ARM_INS_SET_TYPE(ins,  IT_MUL);
    ARM_INS_SET_REGA(ins,  (instr & 0x000f0000) >> 16);
    ARM_INS_SET_REGB(ins,  (instr & 0x0000000f));
    ARM_INS_SET_REGC(ins,  (instr & 0x00000f00) >> 8);
    ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  }

}
/* }}} */


/* {{{ Some macros to ease extracting VFP register fields */
#define VFP_FN(x) (ARM_REG_S0 + ((((x) & 0x000f0000) >> 15) | (((x) >>  7) & 1)))
#define VFP_FD(x) (ARM_REG_S0 + ((((x) & 0x0000f000) >> 11) | (((x) >> 22) & 1)))
#define VFP_FM(x) (ARM_REG_S0 + ((((x) & 0x0000000f) <<  1) | (((x) >>  5) & 1)))
/* }}} */

/* {{{ ArmDisassembleVFPDP */
/*!
 * Disassembles a VFP data processing instruction
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
void ArmDisassembleVFPDP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint64 imm = 0;
  t_uint64 temp = 0;
  int i;

  ASSERT(opc == ARM_VMOV_FIMM, ("old floating-point instruction: %s", arm_opcode_table[opc].desc));

  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);


  ARM_INS_SET_TYPE(ins,  IT_FLT_ALU);

  if (instr & (1<<8))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

   switch(opc)
   {
      case ARM_VMOV_FIMM:
        temp = (((instr & 0x000f0000) >> 12) | (instr & 0x0000000f));
        if(instr & 0x00000100)
        {
          ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
          ARM_INS_SET_DATATYPE(ins, DT_F64);
          ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_A_DOUBLE);

          imm = (temp & 0x80) << 56;
          imm |= (((~temp) & 0x40) << 56);
          for(i = 0; i < 8; i++)
          {
            imm |= ((temp & 0x40) << (55-i));
          }
          imm |= ((temp & 0x3f) << 48);
        }
        else
        {
          ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
          ARM_INS_SET_DATATYPE(ins, DT_F32);
          ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_A_SINGLE);

          imm = (temp & 0x80) << 24;
          imm |= (((~temp) & 0x40) << 24);
          for(i = 0; i < 5; i++)
          {
            imm |= ((temp & 0x40) << (23-i));
          }
          imm |= ((temp & 0x3f) << 19);
        }
        ARM_INS_SET_REGB(ins, ARM_REG_NONE);
        ARM_INS_SET_REGC(ins, ARM_REG_NONE);
        ARM_INS_SET_IMMEDIATE(ins, imm);
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

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
       ARM_INS_SET_REGA(ins, VFP_FD(instr) );
       ARM_INS_SET_REGB(ins, VFP_FM(instr));
       ARM_INS_SET_REGC(ins, ARM_REG_NONE);
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
       if ((instr & (1<<7)) && (instr & (1<<8)))
         FATAL(("Can't have odd-numbered registers in double precision instruction at @G",ARM_INS_OLD_ADDRESS(ins)));
       ARM_INS_SET_REGA(ins, VFP_FD(instr) );
       ARM_INS_SET_REGB(ins, VFP_FN(instr));
       ARM_INS_SET_REGC(ins, VFP_FM(instr));
       break;
/*
     case ARM_FCONSTS:
     case ARM_FCONSTD:
       break;
*/
     case ARM_FCMPS:
     case ARM_FCMPD:
     case ARM_FCMPES:
     case ARM_FCMPED:
       ARM_INS_SET_REGA(ins, ARM_REG_NONE);
       ARM_INS_SET_REGB(ins, VFP_FD(instr));
       ARM_INS_SET_REGC(ins, VFP_FM(instr));
       ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_FPSCR);
       break;
     case ARM_FCMPZS:
     case ARM_FCMPZD:
     case ARM_FCMPEZS:
     case ARM_FCMPEZD:
       ARM_INS_SET_REGA(ins, ARM_REG_NONE);
       ARM_INS_SET_REGB(ins, VFP_FD(instr));
       ARM_INS_SET_REGC(ins, ARM_REG_NONE);
       ARM_INS_SET_IMMEDIATE(ins, 0);
       ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_FPSCR|FL_FLT_IMMED);
       break;
     default:
       FATAL(("Unknown VFPDP: Opcode = %d for ins at @G\n",opc,ARM_INS_OLD_ADDRESS(ins)));
   }
}
/* }}} */


/* {{{ ArmDisassembleVFPRT */
/*!
 * Disassembles a VFP register transfer instruction
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
void ArmDisassembleVFPRT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  FATAL(("old floating-point operation: %s", arm_opcode_table[opc].desc));
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);


  if ((instr & (1<<7)) && (instr & (1<<8)))
    FATAL(("Can't have odd-numbered registers in double precision instruction at @G",ARM_INS_OLD_ADDRESS(ins)));
  else if (instr & (1<<8))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

  switch (opc)
  {

     case ARM_FMSTAT:
       ARM_INS_SET_TYPE(ins,  IT_STATUS);
       ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
       ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
       ARM_INS_SET_REGC(ins,  ARM_REG_FPSCR);
       break;
     case ARM_FMDLR:
     case ARM_FMDHR:
     case ARM_FMSR:
       ARM_INS_SET_TYPE(ins,  IT_INT_FLT);
       ARM_INS_SET_REGB(ins,  (instr & 0x0000f000)>>12);
       ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
       ARM_INS_SET_REGA(ins,  VFP_FN(instr));
       if (opc == ARM_FMDHR)
         ARM_INS_SET_REGA(ins, ARM_INS_REGA(ins)+1);
       break;
     case ARM_FMRDL:
     case ARM_FMRDH:
     case ARM_FMRS:
       ARM_INS_SET_TYPE(ins,  IT_FLT_INT);
       ARM_INS_SET_REGA(ins,  (instr & 0x0000f000)>>12);
       ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
       ARM_INS_SET_REGB(ins,  VFP_FN(instr));
       if (opc == ARM_FMRDH)
         ARM_INS_SET_REGB(ins, ARM_INS_REGB(ins)+1);
       break;
     case ARM_FMXR:
     {
       t_reg sysreg = EXTRACT_VFP_SYSREGFIELD_FROM_INSTR(instr);

       ARM_INS_SET_TYPE(ins,  IT_FLT_STATUS);
       /* REGB == used, REGA == defined */
       ARM_INS_SET_REGB(ins,  (instr & 0x0000f000)>>12);
       ARM_INS_SET_REGA(ins,  VFP_SYSREG_FIELD_TO_ARM_REG(sysreg));
       ASSERT(ARM_INS_REGA(ins)!=ARM_REG_NONE,("Invalid sysreg in FMXR at @G",ARM_INS_OLD_ADDRESS(ins)));
       break;
     }
     case ARM_FMRX:
     {
       t_reg sysreg = EXTRACT_VFP_SYSREGFIELD_FROM_INSTR(instr);

       ARM_INS_SET_TYPE(ins,  IT_FLT_STATUS);
       /* REGB == used, REGA == defined */
       ARM_INS_SET_REGA(ins,  (instr & 0x0000f000)>>12);
       ARM_INS_SET_REGB(ins,  VFP_SYSREG_FIELD_TO_ARM_REG(sysreg));
       ASSERT(ARM_INS_REGA(ins)!=ARM_REG_NONE,("Invalid sysreg in FMXR at @G",ARM_INS_OLD_ADDRESS(ins)));
       break;
     }
     default:
       FATAL(("Unknown VFPRT: Opcode = %d for ins at @G\n",opc,ARM_INS_OLD_ADDRESS(ins)));
  }
}
/* }}} */


/* {{{ ArmDisassembleVFP2R */
/*!
 * Disassembles a VFP two-register transfer instruction
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
void ArmDisassembleVFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  FATAL(("old floating-point operation: %s", arm_opcode_table[opc].desc));
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
  ARM_INS_SET_FLAGS(ins,  0x0);
  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);


  if ((instr & (1<<5)) && (instr & (1<<8)))
    FATAL(("Can't have odd-numbered registers in double precision instruction at @G",ARM_INS_OLD_ADDRESS(ins)));
  else if (instr & (1<<8))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

  switch (opc)
  {
     case ARM_FMRRD:
     case ARM_FMRRS:
       ARM_INS_SET_TYPE(ins,  IT_FLT_INT);
       ARM_INS_SET_REGB(ins,  VFP_FM(instr));
       ARM_INS_SET_REGA(ins,  (instr & 0x000f0000)>>16);
       ARM_INS_SET_REGC(ins,  (instr & 0x0000f000)>>12);
       break;
     case ARM_FMSRR:
     case ARM_FMDRR:
       ARM_INS_SET_TYPE(ins,  IT_INT_FLT);
       ARM_INS_SET_REGA(ins,  VFP_FM(instr));
       ARM_INS_SET_REGB(ins,  (instr & 0x000f0000)>>16);
       ARM_INS_SET_REGC(ins,  (instr & 0x0000f000)>>12);
       break;
     default:
       FATAL(("Unknown VFP2R: Opcode = %d for ins at @G\n",opc,ARM_INS_OLD_ADDRESS(ins)));
  }
}
/* }}} */


/* {{{ ArmDisassembleVFPDT */
/*!
 * Disassembles a VFP data transfer instruction
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
void ArmDisassembleVFPDT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ASSERT(opc == ARM_FLDMX || opc == ARM_FSTMX, ("old floating-point operation: %s", arm_opcode_table[opc].desc));
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  if ((ARM_INS_OPCODE(ins) == ARM_FLDS) ||(ARM_INS_OPCODE(ins) == ARM_FLDD) ||
      (ARM_INS_OPCODE(ins) == ARM_FLDMS) || (ARM_INS_OPCODE(ins) == ARM_FLDMD) ||
      (ARM_INS_OPCODE(ins) == ARM_FLDMX))
    ARM_INS_SET_TYPE(ins,  IT_FLT_LOAD);
  else
    ARM_INS_SET_TYPE(ins,  IT_FLT_STORE);

  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  ARM_INS_SET_FLAGS(ins,  0x0);

  if ((instr & (1<<22)) && (instr & (1<<8)))
    FATAL(("Can't have odd-numbered registers in double precision instruction at @G",ARM_INS_OLD_ADDRESS(ins)));
  else if (instr & (1<<8))
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

  switch (opc)
  {
    case ARM_FSTMS:
    case ARM_FSTMD:
    case ARM_FSTMX:
    case ARM_FLDMS:
    case ARM_FLDMD:
    case ARM_FLDMX:
      {
	int num;
	int tel;
	if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
	if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
	if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
	ARM_INS_SET_REGC(ins,  VFP_FD(instr));
	ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
	ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
	ARM_INS_SET_SHIFTLENGTH(ins,  0);
	ARM_INS_SET_MULTIPLE(ins, RegsetNew());

        /* we don't have to differentiate between single and double precision, because the
         * registers have been (retroactively) defined as being overlapping. The instructions
         * are defined in such a way that the register number and amount of registers are
         * automatically multiplied by two in case of double precision operations
         */

	num=instr & 0xfe;
        /* the lowest bit distinguishes the X variants from the D variants */
        if ((opc == ARM_FSTMX) || (opc == ARM_FLDMX))
          num &= 0xfe;
        ASSERT(!(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) || !(num&1),("Odd number of registers saved for vfp double precision load/store multiple"));
	for (tel=0; tel<num; tel++)
	{
	  if ((ARM_INS_REGC(ins)+tel) > ARM_REG_S31) FATAL(("Implement wrap around in vector float load/store multiple"));
	  ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins),ARM_INS_REGC(ins)+tel));
	}
	ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
      }

      break;
    case ARM_FSTS:
    case ARM_FSTD:
    case ARM_FLDS:
    case ARM_FLDD:
      if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
      if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
      if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

      ARM_INS_SET_REGA(ins,  VFP_FD(instr));
      ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);

      /* offset specified as 8-bit immediate */
      ARM_INS_SET_IMMEDIATE(ins,  4*(instr & 0x000000ff));
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      break;

    default:
      FATAL(("Unknown VFPDT: Opcode = %d for ins at @G\n",opc,ARM_INS_OLD_ADDRESS(ins)));
  }
}
/* }}} */


/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleV6Pkh {{{ */
void ArmDisassembleV6Pkh(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGC(ins,  instr & 0x0000000f);

  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("pkh* instruction at @G has r15 as argument, undefined behaviour",ARM_INS_OLD_ADDRESS(ins)));

  ARM_INS_SET_FLAGS(ins,  0x0);

  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_LSL_IMM);
  ARM_INS_SET_SHIFTLENGTH(ins,  (instr & 0x00000f80) >> 7);
}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleV6DataProc {{{ */
void ArmDisassembleV6DataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGC(ins,  instr & 0x0000000f);

  ARM_INS_SET_FLAGS(ins,  0x0);

  /* don't use bit 20 for deciding whether status flags are updated,
   * it's no longer used for that (see e.g. SHADD8)
   */
  switch (opc)
  {
    case ARM_SADD16:
    case ARM_SADD8:
    case ARM_SADDSUBX:
    case ARM_SSUB16:
    case ARM_SSUB8:
    case ARM_SSUBADDX:
    case ARM_UADD16:
    case ARM_UADD8:
    case ARM_UADDSUBX:
    case ARM_USUB16:
    case ARM_USUB8:
    case ARM_USUBADDX:
      break;

    case ARM_USADA8:
      ARM_INS_SET_REGS(ins, (instr & 0x0000f000) >> 12);
    case ARM_USAD8:
      ARM_INS_SET_REGA(ins, (instr & 0x000f0000) >> 16);
      ARM_INS_SET_REGB(ins, (instr & 0x0000000f));
      ARM_INS_SET_REGC(ins, (instr & 0x00000f00) >> 8);
      break;

    default:
      /* do nothing */
      break;
  }

  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("ARMv6 dataprocession instruction at @G has r15 as argument, undefined behaviour",ARM_INS_OLD_ADDRESS(ins)));

  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_SHIFTLENGTH(ins,  0);
  ARM_INS_SET_REGS(ins,  ARM_REG_NONE);

  /* these don't have an Rn field (= REGB) */
  switch (opc)
  {
    case ARM_REV:
    case ARM_REV16:
    case ARM_REVSH:
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      break;
    default:
      ASSERT((ARM_INS_REGB(ins) != ARM_REG_R15) ,("ARMv6 dataprocession instruction at @G has r15 as argument, undefined behaviour",ARM_INS_OLD_ADDRESS(ins)));
      break;
  }

}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleV6Extract {{{ */
void ArmDisassembleV6Extract(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGC(ins,  instr & 0x0000000f);

  ARM_INS_SET_FLAGS(ins,  0x0);

  ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_ROR_IMM);
  ARM_INS_SET_SHIFTLENGTH(ins,  (instr & 0x00000c00) >> 7);

  /* these don't have an Rn field (= REGB) */
  switch (opc)
  {
    case ARM_SXTH:
    case ARM_SXTB16:
    case ARM_SXTB:
    case ARM_UXTH:
    case ARM_UXTB16:
    case ARM_UXTB:
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      break;
    default:
      /* do nothing */
      break;
  }
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("ARMv6 dataprocession instruction at @G has r15 as argument, undefined behaviour",ARM_INS_OLD_ADDRESS(ins)));

}
/*}}}*/
/*!
 * \todo Document
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* ArmDisassembleV6Sat {{{ */
void ArmDisassembleV6Sat(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  ARM_INS_SET_OPCODE(ins,  opc);
  ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  ARM_INS_SET_ATTRIB(ins,  0x0);
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;

  ARM_INS_SET_REGA(ins,  (instr & 0x0000f000) >> 12);
  ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
  ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
  ASSERT((ARM_INS_REGA(ins) != ARM_REG_R15) && (ARM_INS_REGC(ins) != ARM_REG_R15),("Saturation instruction at @G has r15 as argument, undefined behaviour",ARM_INS_OLD_ADDRESS(ins)));

  ARM_INS_SET_FLAGS(ins,  0x0);

  /* extract the immediate */
  switch (opc)
  {
    case ARM_SSAT:
    case ARM_USAT:
      ARM_INS_SET_IMMEDIATE(ins, ((instr & 0x001f0000) >> 16)+1);
      /* (shift_t, shift_n) = DecodeImmShift(sh:0, imm5);
       * (sh = bit 6 -> mask 0x40)
       */
      ArmShiftDecodeImm(ins, (instr & 0x00000040) >> 5, (instr & 0x00000f80) >> 7);
      break;
    case ARM_SSAT16:
    case ARM_USAT16:
      ARM_INS_SET_IMMEDIATE(ins, ((instr & 0x000f0000) >> 16)+1);
      break;
    default:
      FATAL(("Unsupported saturation opcode"));
  }

  if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE)
  {
    ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_IMM);
    ARM_INS_SET_SHIFTLENGTH(ins, 0);
  }

}

void ArmDisassembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = DT_NONE;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
  ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  datatype = ((instr & 0x00300000) >> 20);

  switch(opc)
  {
    case ARM_VMULL:
      if(instr & 0x00000200)
      {
        datatype += DT_P_START;
      }
      else
      {
        datatype += (instr & 0x01000000) ? DT_U_START : DT_S_START;
      }
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    case ARM_VADDL:
    case ARM_VSUBL:
    case ARM_VABAL:
    case ARM_VABDL:
    case ARM_VMLAL:
    case ARM_VMLSL:
    case ARM_VQDMLAL:
    case ARM_VQDMLSL:
    case ARM_VQDMULL:
      datatype += (instr & 0x01000000) ? DT_U_START : DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    case ARM_VADDW:
    case ARM_VSUBW:
      datatype += (instr & 0x01000000) ? DT_U_START : DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD | NEONFL_C_DOUBLE));
      break;

    case ARM_VADDHN:
    case ARM_VRADDHN:
    case ARM_VSUBHN:
    case ARM_VRSUBHN:
      datatype += 1 + DT_I_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD | NEONFL_C_QUAD));
      break;

    case ARM_VMULL_POLY:
      datatype += DT_P_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    default:
      FATAL(("Unsupported SIMD[3 registers, different length]: %s", arm_opcode_table[opc].desc));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = DT_NONE;
  t_uint32 size = 0;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
  ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
  size = ((instr & 0x00300000) >> 20);

  switch(opc)
  {
    case ARM_VPMIN:
    case ARM_VPMAX:
      ASSERT((instr & 0x00000040) == 0, ("Illegal Q-field in VPADD instruction: should be zero."));

    case ARM_VHADD:
    case ARM_VHSUB:
    case ARM_VQADD:
    case ARM_VQSUB:
    case ARM_VRHADD:
    case ARM_VCGT:
    case ARM_VCGE:
    case ARM_VSHL:
    case ARM_VQSHL:
    case ARM_VRSHL:
    case ARM_VQRSHL:
    case ARM_VMAX:
    case ARM_VMIN:
    case ARM_VABD:
    case ARM_VABA:
      datatype = ((instr & 0x01000000) ? DT_U_START : DT_S_START) + size;
      break;

    case ARM_VORR:
    case ARM_VAND:
    case ARM_VBIC:
    case ARM_VORN:
    case ARM_VEOR:
    case ARM_VBIF:
    case ARM_VBIT:
    case ARM_VBSL:
      break;

    case ARM_VMUL:
      datatype = ((instr & 0x01000000) ? DT_P_START : DT_I_START) + size;
      break;

    case ARM_VPADD:
      ASSERT((instr & 0x00000040) == 0,("Illegal Q-field in VPADD instruction: should be zero."));

    case ARM_VADD:
    case ARM_VSUB:
    case ARM_VCEQ:
    case ARM_VMLA:
    case ARM_VMLS:
      datatype = DT_I_START + size;
      break;

    case ARM_VTST:
      datatype = DT_START + size;
      break;

    case ARM_VPADD_F:
    case ARM_VPMAX_F:
    case ARM_VPMIN_F:
      ASSERT((instr & 0x00000040) == 0, ("Illegal Q-field in VPADD instruction: should be zero."));

    case ARM_VADD_F:
    case ARM_VSUB_F:
    case ARM_VABD_F:
    case ARM_VMUL_F:
    case ARM_VMLA_F:
    case ARM_VMLS_F:
    case ARM_VCEQ_F:
    case ARM_VCGE_F:
    case ARM_VCGT_F:
    case ARM_VACGE_F:
    case ARM_VACGT_F:
    case ARM_VMAX_F:
    case ARM_VMIN_F:
    case ARM_VRECPS_F:
    case ARM_VRSQRTS_F:
      ASSERT((size & 1) == 0, ("Illegal size-field in floating-point SIMD instruction, should be zero."));

    case ARM_VFMA:
    case ARM_VFMS:
      datatype = DT_F32;
      break;

    case ARM_VQDMULH:
    case ARM_VQRDMULH:
      datatype = DT_S_START + size;
      break;

    default:
      FATAL(("Unsupported SIMD[3 registers, same length] instruction: %s %u", arm_opcode_table[opc].desc, opc));
  }

  if((opc == ARM_VQRSHL) || (opc == ARM_VSHL) || (opc == ARM_VQSHL) || (opc == ARM_VRSHL))
  {
    ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
    ARM_INS_SET_REGC(ins, NEON_VN_QD(instr));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 imm = 0;
  t_uint32 imminv = 0;
  t_uint32 datatype = DT_NONE;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  switch(opc)
  {
    case ARM_VSHR:
    case ARM_VSRA:
    case ARM_VRSHR:
    case ARM_VRSRA:
    case ARM_VSRI:
    case ARM_VSHL_IMM:
    case ARM_VSLI:
    case ARM_VQSHL_IMM:
    case ARM_VQSHLU_IMM:
    case ARM_VSHLL_IMM:
      imm = ((instr & 0x003f0000) >> 16);
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));

      /* TODO: incorporate the fact that there is no C-register here, or can we just set flags for the C-register and ignore them? */
      if(opc == ARM_VSHLL_IMM)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      }
      else
      {
        InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      }

      if((opc == ARM_VSRI) || (opc == ARM_VSLI))
      {
        datatype = DT_START;
      }
      else if(opc == ARM_VSHL_IMM)
      {
        datatype = DT_S_START;/* TODO: change S to I, this was done to match output of objdump */
      }
      else if((opc == ARM_VQSHLU_IMM) || (opc == ARM_VQSHL_IMM))
      {
        if(instr & 0x00000100)
        {
          /* op=1; VQSHL */
          datatype = ((instr & 0x01000000) ? DT_U_START : DT_S_START);
        }
        else
        {
          /* op=0; VQSHLU */
          if(instr & 0x01000000)
          {
            datatype = DT_S_START;
          }
          else
          {
            FATAL(("Illegal U-field in VQSHLU instruction"));
          }
        }
      }
      else
      {
        datatype = ((instr & 0x01000000) ? DT_U_START : DT_S_START);
      }

      if(instr & 0x00000080)
      {
        imminv = 64;
        datatype += 3;
      }
      else
      {
        if((imm & 0x38) == 0x08)
        {
          imminv = 8;
          imm &= 0x07;
        }
        else if((imm & 0x30) == 0x10)
        {
          imminv = 16;
          datatype += 1;
          imm &= 0x0f;
        }
        else if((imm & 0x20) == 0x20)
        {
          imminv = 32;
          datatype += 2;
          imm &= 0x1f;
        }
        else
        {
          FATAL(("Illegal immediate field in SIMD[2 registers, shift] instruction"));
        }
      }

      if((opc == ARM_VSHR) || (opc == ARM_VSRA) || (opc == ARM_VRSHR) ||
        (opc == ARM_VRSRA) || (opc == ARM_VSRI))
      {
        imm = imminv - imm;
      }

      break;

    case ARM_VQSHRN:
    case ARM_VQSHRUN:
    case ARM_VQRSHRN:
    case ARM_VQRSHRUN:
    case ARM_VSHRN:
    case ARM_VRSHRN:
      imm = ((instr & 0x003f0000) >> 16);
      ASSERT(imm & 0x38, ("Illegal imm-field in VSHRN instruction"));

      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));

      switch(opc)
      {
        case ARM_VSHRN:
        case ARM_VRSHRN:
          datatype = DT_I_START;
          break;

        case ARM_VQSHRN:
        case ARM_VQRSHRN:
          datatype = (instr & 0x01000000) ? DT_U_START : DT_S_START;
          break;
        case ARM_VQSHRUN:
        case ARM_VQRSHRUN:
          datatype = DT_S_START;
          break;
      }

      if((imm & 0x38) == 0x08)
      {
        imminv = 8;
        datatype += 1;
        imm &= 0x07;
      }
      else if((imm & 0x30) == 0x10)
      {
        imminv = 16;
        datatype += 2;
        imm &= 0x0f;
      }
      else if((imm & 0x20) == 0x20)
      {
        imminv = 32;
        datatype += 3;
        imm &= 0x1f;
      }
      else
      {
        FATAL(("Illegal immediate field in SIMD[2 registers, shift] instruction"));
      }
      imm = imminv - imm;
      break;

    case ARM_VMOVL1:
    case ARM_VMOVL2:
    case ARM_VMOVL3:
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
      datatype = ((instr & 0x01000000) ? DT_U_START : DT_S_START);
      switch((instr & 0x00380000) >> 19)
      {
        case 1:
          /* 8-bit */
          break;
        case 2:
          /* 16-bit */
          datatype++;
          break;
        case 4:
          /* 32-bit */
          datatype += 2;
          break;
        default:
          FATAL(("Illegal immediate field in SIMD VMOVL instruction"));
      }
      break;

    case ARM_VCVT_FX:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
      InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      imm = 64 - ((instr & 0x003f0000) >> 16);
      if(instr & 0x00000100)
      {
        datatype = (instr & 0x01000000) ? DT_U32 : DT_S32;
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
      }
      else
      {
        datatype = DT_F32;
        ARM_INS_SET_DATATYPEOP(ins, (instr & 0x01000000) ? DT_U32 : DT_S32);
      }
      break;

    default:
      FATAL(("Unsupported SIMD[2 registers, shift] instruction: %s", arm_opcode_table[opc].desc));
  }

  if(opc != ARM_VMOVL1
     && opc != ARM_VMOVL2
     && opc != ARM_VMOVL3)
  {
    ARM_INS_SET_IMMEDIATE(ins, imm);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  }
  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMD(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 size = 0;
  t_uint32 imm = 0;
  t_uint32 datatype = DT_NONE;
  int i;
  int len, regn;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  switch(opc)
  {
    case ARM_VDUP:
      ARM_INS_SET_REGA(ins, NEON_VN_QD(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| ((instr & 0x00200000) ? NEONFL_A_QUAD : NEONFL_A_DOUBLE));
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_B_CORE);

      ARM_INS_SET_CONDITION(ins, ARM_EXTRACT_CONDITION_CODE(instr));
      if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

      size = ((instr & 0x00400000) >> 21) | ((instr & 0x00000020) >> 5);
      switch(size)
      {
        case 0:
          datatype = DT_32;
          break;
        case 1:
          datatype = DT_16;
          break;
        case 2:
          datatype = DT_8;
          break;

        default:
          FATAL(("Illegal size-field in VDUP instruction: %u", size));
      }
      break;

    case ARM_VDUP_SCALAR:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_B_SCALAR);

      /* Datatype? */
      imm = ((instr & 0x000f0000) >> 16);
      datatype = DT_START;
      if((imm & 0x1) == 0x1)
      {
        datatype += 0;
        ARM_INS_SET_REGBSCALAR(ins, (imm & 0xe) >> 1);
      }
      else if((imm & 0x3) == 0x2)
      {
        datatype += 1;
        ARM_INS_SET_REGBSCALAR(ins, (imm & 0xc) >> 2);
      }
      else if((imm & 0x7) == 0x4)
      {
        datatype += 2;
        ARM_INS_SET_REGBSCALAR(ins, (imm & 0x8) >> 3);
      }
      else
      {
        FATAL(("Illegal size field of immediate in VDUP instruction"));
      }

      /* Quad or double operation? */
      if((instr & 0x00000040))
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_A_QUAD);
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_A_DOUBLE);
      }
      break;

    case ARM_VEXT:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
      ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x00000f00) >> 8);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      datatype = DT_8;
      break;

    case ARM_VTBL:
    case ARM_VTBX:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, ARM_REG_NONE);
      ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_C_DOUBLE));

      datatype = DT_8;
      len = ((instr & 0x00000300) >> 8) + 1;
      regn = ((instr & 0x00000080) >> 3) | ((instr & 0x000f0000) >> 16);
      ASSERT((regn+len) <= 32, ("Wraparound in VTBL/VTBX instruction register list."));

      ARM_INS_SET_MULTIPLE(ins, RegsetNew());
      for(i = regn; i < regn+len; i++)
      {
        int diablo_regn_ir = (i < 16) ? (ARM_REG_S0 + (2*i)) : (ARM_REG_D16 + (i-16));
        ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), diablo_regn_ir));
      }

      break;

    default:
      FATAL(("Unsupported SIMD usage: %s (opcode=%u)", arm_opcode_table[opc].desc, opc));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMDImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_int64 immediate;
  t_uint32 cmode;
  t_uint32 op;
  t_int64 temp = 0;
  int i, j;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins,  IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  immediate = (t_int64)(((instr & 0x01000000) >> 17) | ((instr & 0x00070000) >> 12) | (instr & 0x0000000f));
  cmode = (instr & 0x00000f00) >> 8;
  op = (instr & 0x00000020) >> 5;

  /* calculate immediate value...
   * ... according to table A7-15, ARM-DDI-0406C.b/A7-269
   */
  switch(cmode)
  {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
      temp = immediate | (immediate << 32);
      temp <<= (8 * (cmode >> 1));
      ARM_INS_SET_DATATYPE(ins, DT_I32);
      break;

    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
      for (i = 0; i < 63; i += 16)
      {
        temp |= immediate << i;
      }
      temp <<= (cmode & 2) ? 8 : 0;
      ARM_INS_SET_DATATYPE(ins, DT_I16);
      break;

    case 0xc:
      temp = (immediate << 8) | 0x000000ff;
      temp |= temp << 32;
      ARM_INS_SET_DATATYPE(ins, DT_I32);
      break;

    case 0xd:
      temp = (immediate << 16) | 0x0000ffff;
      temp |= temp << 32;
      ARM_INS_SET_DATATYPE(ins, DT_I32);
      break;

    case 0xe:
      if(op)
      {
        for(i = 0; i < 8; i++)
        {
          t_int64 bit = (immediate >> (7 - i)) & 1;

          for(j = 0; j < 8; j++)
          {
            temp |= bit << ((63 - 8*i) - j);
          }
        }

        ARM_INS_SET_DATATYPE(ins, DT_I64);
      }
      else
      {
        for (i = 0; i < 64; i += 8)
        {
          temp |= (immediate << i);
        }

        ARM_INS_SET_DATATYPE(ins, DT_I8);
      }
      break;

    case 0xf:
      if(op)
      {
        FATAL(("UNDEFINED immediate constant for SIMD modified immediate instruction"));
      }
      else
      {
        temp = (immediate & 0x80) << 24;        /* bit a */
        temp |= ((~immediate) & 0x40) << 24;      /* bit B (NOT(b)) */
        for (i = 0; i < 5; i++)
        {
          temp |= (immediate & 0x40) << (23-i); /* bit b (repeated) */
        }
        temp |= (immediate & 0x3f) << 19;       /* bits c-h */
        temp |= (temp << 32);
        ARM_INS_SET_DATATYPE(ins, DT_F32);
      }
      break;

    default:
      FATAL(("Invalid cmode-field for SIMD modified immediate instruction: 0x%x", cmode));
  }

  ARM_INS_SET_IMMEDIATE(ins, temp);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| ((instr & 0x00000040) ? NEONFL_A_QUAD : NEONFL_A_DOUBLE));
}

void ArmDisassembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 size = 0;
  t_uint32 datatype = DT_NONE;
  t_uint32 imm = 0;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  if((opc != ARM_VSHLL) &&
    (opc != ARM_VQMOVN) &&
    (opc != ARM_VQMOVUN) &&
    (opc != ARM_VMOVN) &&
    (opc != ARM_VCVT_HS))
  {
    InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
  }

  size = ((instr & 0x000c0000) >> 18);

  switch(opc)
  {
    case ARM_VREV16:
    case ARM_VREV32:
    case ARM_VREV64:
    case ARM_VTRN:
    case ARM_VUZP:
    case ARM_VZIP:
      datatype = DT_START+size;
      break;

    case ARM_VPADDL:
    case ARM_VPADAL:
      datatype = (instr & 0x00000080) ? DT_U_START : DT_S_START;
      datatype += size;
      break;

    case ARM_VCLS:
    case ARM_VQABS:
    case ARM_VQNEG:
      datatype = DT_S_START+size;
      break;

    case ARM_VSHLL:
      switch(size)
      {
        case 0:
          imm = 8;
          break;
        case 1:
          imm = 16;
          break;
        case 2:
          imm = 32;
          break;

        default:
          FATAL(("Illegal size field in VSHLL instruction"));
      }
      ARM_INS_SET_IMMEDIATE(ins, imm);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
    case ARM_VCLZ:
      datatype = DT_I_START+size;
      break;

    case ARM_VMOVN:
      datatype = DT_I_START+size+1;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));
      break;

    case ARM_VCNT:
      datatype = DT_8;
      break;

    case ARM_VSWP:
      if(size != 0)
      {
        FATAL(("Illegal size field for VSWP instruction. Should be zero but isn't zero."));
      }
    case ARM_VMVN:
      break;

    case ARM_VCGT_IMM:
    case ARM_VCGE_IMM:
    case ARM_VCEQ_IMM:
    case ARM_VCLE_IMM:
    case ARM_VCLT_IMM:
      ARM_INS_SET_IMMEDIATE(ins, 0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

    case ARM_VABS:
    case ARM_VNEG:
      if((instr & 0x00000400) && (size != 2))
      {
        FATAL(("Illegal size field in SIMD[2 regs, misc value] instruction"));
      }

      if(opc == ARM_VCEQ_IMM)
      {
        datatype = (instr & 0x00000400) ? DT_F_START : DT_I_START;
      }
      else
      {
        datatype = (instr & 0x00000400) ? DT_F_START : DT_S_START;
      }
      datatype += size;
      break;

    case ARM_VCVT_HS:
      VERBOSE(1,("WARNING --- VCVT (HS) instruction implemented but not tested: 0x%08x", instr));
      ASSERT(size == 1, ("Illegal size field in VCVT (half-single) instruction."));

      if(instr & 0x00000100)
      {
        /* half to single */
        datatype = DT_F32;
        ARM_INS_SET_DATATYPEOP(ins, DT_F16);
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      }
      else
      {
        datatype = DT_F16;
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));
      }
      break;

    case ARM_VCVT_FI:
      if(((instr & 0x000c0000) >> 18) != 2)
      {
        FATAL(("Illegal size field in VCVT (floating-integer) instruction."));
      }

      switch((instr & 0x00000180) >> 7)
      {
        case 0:
          datatype = DT_F32;
          ARM_INS_SET_DATATYPEOP(ins, DT_S32);
          break;

        case 1:
          datatype = DT_F32;
          ARM_INS_SET_DATATYPEOP(ins, DT_U32);
          break;

        case 2:
          datatype = DT_S32;
          ARM_INS_SET_DATATYPEOP(ins, DT_F32);
          break;

        case 3:
          datatype = DT_U32;
          ARM_INS_SET_DATATYPEOP(ins, DT_F32);
          break;

        default:
          FATAL(("Illegal op-field in VCVT (floating-integer) instruction."));
      }

      ARM_INS_SET_REGC(ins, ARM_REG_NONE);
      break;

    case ARM_VRECPE:
    case ARM_VRSQRTE:
      datatype = (instr & 0x00000100) ? DT_F32 : DT_U32;
      if(size != 2)
      {
        FATAL(("Illegal size-field in VRECPE/VRSQRTE instruction."));
      }
      break;

    case ARM_VQMOVN:
    case ARM_VQMOVUN:
      switch((instr & 0x000000c0) >> 6)
      {
        case 1:
        case 2:
          datatype = DT_S_START+(size+1);
          break;
        case 3:
          datatype = DT_U_START+(size+1);
          break;
        default:
          FATAL(("Illegal op-field in VQMOV(U)N instruction."));
      }

      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));

      break;

    default:
      FATAL(("Unsupported SIMD[2 regs, misc value] instruction: %s (%x)", arm_opcode_table[opc].desc, (opc-ARM_VREV64)));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = 0;
  t_uint32 vm = 0, index = 0;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  /* Third operand is a special case */
  datatype = ((instr & 0x00300000) >> 20);
  ASSERT(datatype != 3, ("Illegal size-field in scalar instruction."));

  if(datatype == 1)
  {
    /* 16-bit scalar */
    vm = (instr & 0x00000007);
    index = ((instr & 0x00000020) >> 4) | ((instr & 0x00000008) >> 3);
  }
  else
  {
    /* 32-bit scalar */
    vm = (instr & 0x0000000f);
    index = ((instr & 0x00000020) >> 5);
  }
  ARM_INS_SET_REGC(ins, ARM_REG_S0+(vm*2));
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_C_SCALAR);
  ARM_INS_SET_REGCSCALAR(ins, index);

  switch(opc)
  {
    case ARM_VMLA_SCALAR:
    case ARM_VMLS_SCALAR:
    case ARM_VMUL_SCALAR:
      /* check the F-flag of the instruction to get I or F */
      datatype += ((instr & 0x00000100) ? DT_F_START : DT_I_START);

      /* F ==> size must be 32 */
      if((instr & 0x00000100) && (datatype != DT_F32))
      {
        FATAL(("Size-field of floating-point scalar must be 32"));
      }

      /* check the Q-flag of the instruction */
      if(instr & 0x01000000)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD));
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_DOUBLE));
      }

      break;

    case ARM_VMLAL_SCALAR:
    case ARM_VMLSL_SCALAR:
    case ARM_VMULL_SCALAR:
      datatype += ((instr & 0x01000000) ? DT_U_START : DT_S_START);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      break;

    case ARM_VQDMLAL_SCALAR:
    case ARM_VQDMLSL_SCALAR:
    case ARM_VQDMULL_SCALAR:
      datatype += DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      break;

    case ARM_VQDMULH_SCALAR:
    case ARM_VQRDMULH_SCALAR:
      datatype += DT_S_START;

      if(instr & 0x01000000)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD));
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_DOUBLE));
      }
      break;

    default:
      FATAL(("Unsupported SIMD scalar instruction opcode"));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void ArmDisassembleSIMDTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc) {
  t_uint32 opc1 = 0;
  t_uint32 opc2 = 0;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  switch(opc)
  {
    case ARM_VMOV_C2S:
      ARM_INS_SET_REGA(ins, NEON_VN_S(instr));
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SINGLE | NEONFL_B_CORE));
      break;

    case ARM_VMOV_S2C:
      ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_REGB(ins, NEON_VN_S(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_SINGLE));
      break;

    case ARM_VMOV_C2SCALAR:
      opc1 = (instr & 0x00600000) >> 21;
      opc2 = (instr & 0x00000060) >> 5;
      if(opc1 & 2)
      {
        ARM_INS_SET_REGASCALAR(ins, ((opc1 & 1) << 2) | opc2);
        ARM_INS_SET_DATATYPE(ins, DT_8);
      }
      else
      {
        if(opc2 & 1)
        {
          ARM_INS_SET_REGASCALAR(ins, ((opc1 & 1) << 1) | ((opc2 & 2) >> 1));
          ARM_INS_SET_DATATYPE(ins, DT_16);
        }
        else
        {
          ARM_INS_SET_REGASCALAR(ins, opc1 & 1);
          ARM_INS_SET_DATATYPE(ins, DT_32);
        }
      }
      ARM_INS_SET_REGA(ins, NEON_VN_QD(instr));
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SCALAR | NEONFL_B_CORE));
      break;

    case ARM_VMOV_SCALAR2C:
      opc1 = (instr & 0x00600000) >> 21;
      opc2 = (instr & 0x00000060) >> 5;
      if(opc1 & 2)
      {
        ARM_INS_SET_REGBSCALAR(ins, ((opc1 & 1) << 2) | opc2);
        ARM_INS_SET_DATATYPE(ins, (instr & 0x00800000) ? DT_U8 : DT_S8);
      }
      else
      {
        if(opc2 & 1)
        {
          ARM_INS_SET_REGBSCALAR(ins, ((opc1 & 1) << 1) | ((opc2 & 2) >> 1));
          ARM_INS_SET_DATATYPE(ins, (instr & 0x00800000) ? DT_U16 : DT_S16);
        }
        else
        {
          ARM_INS_SET_REGBSCALAR(ins, opc1 & 1);
          ARM_INS_SET_DATATYPE(ins, DT_32);
        }
      }
      ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_SCALAR));
      break;

    case ARM_VMRS:
      ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
		if (ARM_INS_REGA(ins)==ARM_REG_R15) {
		  ARM_INS_SET_REGA(ins, ARM_REG_CPSR);
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);
		}
      ARM_INS_SET_REGB(ins, ARM_REG_FPSCR);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_CORE));
      ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
      break;

    case ARM_VMSR:
      ARM_INS_SET_REGA(ins, ARM_REG_FPSCR);
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
		if (ARM_INS_REGB(ins)==ARM_REG_R15)
		  ARM_INS_SET_REGB(ins, ARM_REG_CPSR);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SINGLE | NEONFL_B_CORE));
      ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
      break;

    default:
      FATAL(("Unsupported SIMD register transfer instruction: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleSIMDLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc) {
  t_uint32 align = 0;
  t_uint32 size = DT_NONE;
  t_uint32 numregs = 0;
  t_uint32 regn = (instr & 0x000f0000) >> 16;
  t_uint32 regd = 0;
  t_uint32 regm = (instr & 0x0000000f);
  t_uint32 scalar_index = 0;
  t_uint32 index_align = 0;
  int regd_increment = 1;
  int i;

  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  if((opc >= ARM_SIMD_FIRSTSTORE) && (opc <= ARM_SIMD_LASTSTORE))
  {
    ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);
  }
  else if((opc >= ARM_SIMD_FIRSTLOAD) && (opc <= ARM_SIMD_LASTLOAD))
  {
    ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);
  }
  else
  {
    FATAL(("Illegal SIMD load/store multiple opcode: %s", arm_opcode_table[opc].desc));
  }

  switch(opc)
  {
    case ARM_VLD4_MULTI:
    case ARM_VST4_MULTI:
      size = (instr & 0x000000c0) >> 6;
      ASSERT(size != 3, ("Illegal size-field in VLD4 instruction."));

      align = (instr & 0x00000030) >> 4;
      align = (align) ? (32 << align) : 1;

      numregs = 4;

      switch((instr & 0x00000f00) >> 8)
      {
        case 0:
          break;
        case 1:
          regd_increment++;
          break;
        default:
          FATAL(("Illegal type-field in VLD4 instruction."));
      }
      break;

    case ARM_VLD4_ONE:
    case ARM_VST4_ONE:
      size = (instr & 0x00000c00) >> 10;
      ASSERT(size != 3, ("Illegal size-field in VLD4 instruction."));

      index_align = (instr & 0x000000f0) >> 4;
      numregs = 4;

      switch(size)
      {
        case 0:
          scalar_index = (index_align & 0xe) >> 1;
          align = (index_align & 1) ? 32 : 1;
          break;
        case 1:
          scalar_index = (index_align & 0xc) >> 2;
          regd_increment += (index_align & 2) ? 1 : 0;
          align = (index_align & 1) ? 64 : 1;
          break;
        case 2:
          scalar_index = (index_align & 0x8) >> 3;
          regd_increment += (index_align & 4) ? 1 : 0;
          switch(index_align & 3)
          {
            case 0:
              align = 1;
              break;
            case 1:
              align = 64;
              break;
            case 2:
              align = 128;
              break;
            default:
              FATAL(("Illegal align-field in VLD4 instruction."));
          }
          break;
        default:
          FATAL(("Illegal size-field in VLD4 instruction."));
      }

      ARM_INS_SET_MULTIPLESCALAR(ins, scalar_index);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    case ARM_VLD4_ALL:
      size = (instr & 0x000000c0) >> 6;
      align = (instr & 0x00000010) >> 4;
      ASSERT(!((size == 3) && (align == 0)), ("Illegal combination of size- and align-field in VLD4 instruction."));

      numregs = 4;

      switch(size)
      {
        case 0:
          align = (align) ? 32 : 1;
          break;

        case 1:
        case 2:
          align = (align) ? 64 : 1;
          break;

        case 3:
          align = (align) ? 128 : 1;
          break;

        default:
          FATAL(("Illegal align-field in VLD4 instruction."));
      }

      regd_increment += (instr & 0x00000020) ? 1 : 0;

      ARM_INS_SET_MULTIPLESCALAR(ins, MULTIPLESCALAR_ALL);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    case ARM_VLD3_MULTI:
    case ARM_VST3_MULTI:
      size = (instr & 0x000000c0) >> 6;
      ASSERT(size != 3, ("Illegal size-field in VLD3 instruction."));

      align = (instr & 0x00000030) >> 4;
      ASSERT((align & 2) == 0, ("Illegal align-field in VLD3 instruction."));
      align = (align == 1) ? 64 : 1;

      numregs = 3;

      switch((instr & 0x00000f00) >> 8)
      {
        case 4:
          break;

        case 5:
          regd_increment++;
          break;

        default:
          FATAL(("Illegal type-field in VLD3 instruction."));
      }
      break;

    case ARM_VLD3_ONE:
    case ARM_VST3_ONE:
      size = (instr & 0x00000c00) >> 10;
      ASSERT(size != 3, ("Illegal size-field in VLD3 instruction."));

      align = 1;
      index_align = (instr & 0x000000f0) >> 4;
      numregs = 3;

      switch(size)
      {
        case 0:
          ASSERT((index_align & 1) == 0, ("Illegal align-field in VLD3 instruction."));
          scalar_index = (index_align & 0xe) >> 1;
          break;

        case 1:
          ASSERT((index_align & 1) == 0, ("Illegal align-field in VLD3 instruction."));
          scalar_index = (index_align & 0xc) >> 2;
          regd_increment += ((index_align & 2) == 2) ? 1 : 0;
          break;

        case 2:
          ASSERT((index_align & 3) == 0, ("Illegal align-field in VLD3 instruction."));
          scalar_index = (index_align & 0x8) >> 3;
          regd_increment += ((index_align & 4) == 4) ? 1 : 0;
          break;

        default:
          FATAL(("Illegal size-field in VLD3 instruction."));
      }

      ARM_INS_SET_MULTIPLESCALAR(ins, scalar_index);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    case ARM_VLD3_ALL:
      size = (instr & 0x000000c0) >> 6;
      ASSERT(size != 3, ("Illegal size-field in VLD3 instruction."));

      align = (instr & 0x00000010) >> 4;
      ASSERT(align == 0, ("Illegal align-field in VLD3 instruction."));

      numregs = 3;
      regd_increment += (instr & 0x00000020) ? 1 : 0;

      ARM_INS_SET_MULTIPLESCALAR(ins, MULTIPLESCALAR_ALL);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);

      break;

    case ARM_VLD2_MULTI1:
    case ARM_VLD2_MULTI2:
    case ARM_VST2_MULTI1:
    case ARM_VST2_MULTI2:
      size = (instr & 0x000000c0) >> 6;
      ASSERT(size != 3, ("Illegal size-field in VLD2 instruction."));

      align = (instr & 0x00000030) >> 4;

      switch((instr & 0x00000f00) >> 8)
      {
        case 3:
          numregs = 4;
          //regd_increment++;
          break;

        case 8:
          ASSERT(align != 3, ("Illegal align-field in VLD2 instruction."));
          numregs = 2;
          break;

        case 9:
          ASSERT(align != 3, ("Illegal align-field in VLD2 instruction."));
          numregs = 2;
          regd_increment++;
          break;

        default:
          FATAL(("Illegal type-field in VLD2 instruction."));
      }

      align = (align) ? (32 << align) : 1;
      break;

    case ARM_VLD2_ONE:
    case ARM_VST2_ONE:
      numregs = 2;
      size = (instr & 0x00000c00) >> 10;
      ASSERT(size != 3, ("Illegal size-field in VLD2 instruction."));

      index_align = (instr & 0x000000f0) >> 4;

      switch(size)
      {
        case 0:
          /* size = 8 */
          scalar_index = (index_align & 0xe) >> 1;
          align = (index_align & 1) ? 16 : 1;
          break;

        case 1:
          /* size = 16 */
          scalar_index = (index_align & 0xc) >> 2;
          align = (index_align & 1) ? 32 : 1;
          regd_increment += ((index_align & 2) ? 1 : 0);
          break;

        case 2:
          /* size = 32 */
          scalar_index = (index_align & 0x8) >> 3;
          align = (index_align & 3) ? 64 : 1;
          ASSERT((index_align & 3) <= 1, ("Illegal align-field in VLD2 instruction."));
          regd_increment += ((index_align & 0x4) ? 1 : 0);
          break;

        default:
          FATAL(("Illegal size-field in VLD2 instruction."));
      }

      ARM_INS_SET_MULTIPLESCALAR(ins, scalar_index);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    case ARM_VLD2_ALL:
      size = (instr & 0x000000c0) >> 6;
      numregs = 2;
      regd_increment += ((instr & 0x00000020) ? 1 : 0);

      switch(size)
      {
        case 0:
          align = (instr & 0x00000010) ? 16 : 1;
          break;
        case 1:
          align = (instr & 0x00000010) ? 32 : 1;
          break;
        case 2:
          align = (instr & 0x00000010) ? 64 : 1;
          break;

        default:
          FATAL(("Illegal size-field in VLD2 instruction."));
      }

      ARM_INS_SET_MULTIPLESCALAR(ins, MULTIPLESCALAR_ALL);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    case ARM_VLD1_MULTI1OR3:
    case ARM_VLD1_MULTI2OR4:
    case ARM_VST1_MULTI1OR3:
    case ARM_VST1_MULTI2OR4:
      size = (instr & 0x000000c0) >> 6;
      align = (instr & 0x00000030) >> 4;

      switch((instr & 0x00000f00) >> 8)
      {
        case 0x2:
          numregs = 4;
          break;

        case 0x6:
          align = (align & 2) ? -1 : (align);
          numregs = 3;
          break;

        case 0x7:
          align = (align & 2) ? -1 : (align);
          numregs = 1;
          break;

        case 0xa:
          align = (align == 3) ? -1 : (align);
          numregs = 2;
          break;

        default:
          FATAL(("Illegal type-field in VLD1 instruction."));
      }

      ASSERT(align != -1, ("Illegal align-field in VLD1 instruction."));
      align = (align) ? (32 << align) : 1;

      break;

    case ARM_VLD1_ONE:
    case ARM_VST1_ONE:
      size = (instr & 0x00000c00) >> 10;
      index_align = (instr & 0x000000f0) >> 4;

      switch(size)
      {
        case 0:
          /* size = 8 */
          scalar_index = (index_align & 0xe) >> 1;
          align = (index_align & 1) ? -1 : 1;
          break;

        case 1:
          /* size = 16 */
          scalar_index = (index_align & 0xc) >> 2;
          switch(index_align & 3)
          {
            case 0:
              align = 1;
              break;
            case 1:
              align = 2;
              break;
            default:
              /* illegal align-field; FATAL-ed further on */
              align = -1;
          }
          break;

        case 2:
          /* size = 32 */
          scalar_index = (index_align & 0x8) >> 3;
          switch(index_align & 7)
          {
            case 0:
              align = 1;
              break;
            case 3:
              align = 4;
              break;
            default:
              /* illegal align-field; FATAL-ed further on */
              align = -1;
          }
          break;

        default:
          FATAL(("Illegal size-field in VLD1 (single element to one lane) instruction."));
      }

      ASSERT(align != -1, ("Illegal align-field in VLD1 (single element to one lane) instruction."));

      ARM_INS_SET_MULTIPLESCALAR(ins, scalar_index);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);

      numregs = 1;
      break;

    case ARM_VLD1_ALL:
      size = (instr & 0x000000c0) >> 6;
      numregs = 1 + ((instr & 0x00000020) ? 1 : 0);
      align = 1;

      switch(size)
      {
        case 0:
          break;
        case 1:
          align = (instr & 0x00000010) ? 2 : 1;
          break;
        case 2:
          align = (instr & 0x00000010) ? 4 : 1;
          break;
        default:
          FATAL(("Illegal size-field in VLD1 (single element to all lanes) instruction."));
      }

      ARM_INS_SET_MULTIPLESCALAR(ins, MULTIPLESCALAR_ALL);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);
      break;

    default:
      FATAL(("Unsupported SIMD load/store instruction: %s", arm_opcode_table[opc].desc));
  }

  /* construct register list */
  /* just need the index of the D-register, not the Diablo-IR number of this register,
   * so do not call NEON_VD_QD here.
   */
  regd = ((instr & 0x00400000) >> 18) | ((instr & 0x0000f000) >> 12);
  ASSERT(regd+numregs <= 32, ("Wrap-around in SIMD load/store multiple registers."));
  ARM_INS_SET_MULTIPLE(ins, RegsetNew());
  for(i = regd; i < regd+(numregs*regd_increment); i += regd_increment)
  {
    int diablo_regd_ir = (i < 16) ? (ARM_REG_S0 + (2*i)) : (ARM_REG_D16 + (i-16));
    ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), diablo_regd_ir));
  }

  /* no destination register */
  ARM_INS_SET_REGA(ins, ARM_REG_NONE);

  /* base register */
  ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
  if(align != 0)
  {
    ARM_INS_SET_MULTIPLEALIGNMENT(ins, align);
  }
  if(regm != ARM_REG_R15)
  {
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  }

  /* offset register */
  ARM_INS_SET_REGC(ins, (((regm == ARM_REG_R15) || (regm == ARM_REG_R13)) ? ARM_REG_NONE : regm));

  /* various settings */
  ARM_INS_SET_DATATYPE(ins, DT_START+size);
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | (NEONFL_B_CORE | NEONFL_C_CORE));
}

void ArmDisassembleFPLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_reg dreg;
  t_uint32 numregs, i;
  t_uint32 puw;

  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  switch(opc)
  {
    case ARM_VLDM:
      ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);

    case ARM_VSTM:
      /* default type is IT_STORE_MULTIPLE */

      /* [D|I][A|B][!] */
      puw = ((instr & 0x01800000) >> 22) | ((instr & 0x00200000) >> 21);
      switch(puw)
      {
        case 2:
          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
          break;

        case 3:
          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| (FL_DIRUP | FL_WRITEBACK));
          break;

        case 5:
          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| (FL_PREINDEX | FL_WRITEBACK));
          break;

        default:
          FATAL(("Illegal PUW-field combination in VSTM/VLDM instruction"));
      }

      /* set registers */
      ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

    case ARM_VPOP:
    case ARM_VPUSH:
      if(opc == ARM_VPOP)
        ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);

      if((opc == ARM_VPUSH) || (opc == ARM_VPOP)) {
        /* VPUSH and VPOP have an implicit writeback effect! */
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_WRITEBACK);
        /* ... and affect the stack pointer */
        ARM_INS_SET_REGB(ins, ARM_REG_R13);
      }

      /* extract data from opcode */
      numregs = (instr & 0x000000ff);

      /* construct register list */
      ARM_INS_SET_MULTIPLE(ins, RegsetNew());
      if(instr & 0x00000100)
      {
        /* double */
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);
        dreg = ((instr & 0x00400000) >> 18) | ((instr & 0x0000f000) >> 12);

        numregs >>= 1;
        ASSERT(dreg+numregs <= 32, ("Wrap-around in D-registers for %s instruction", arm_opcode_table[opc].desc));

        for(i=dreg; i < dreg+numregs; i++)
        {
          int diablo_regd_ir = (i < 16) ? (ARM_REG_S0 + i*2) : (ARM_REG_D16 + (i-16));
          ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), diablo_regd_ir));
        }
      }
      else
      {
        /* single */
        dreg = ((instr & 0x00400000) >> 22) | ((instr & 0x0000f000) >> 11);
        dreg += ARM_REG_S0;
        ASSERT((dreg+numregs)-ARM_REG_S0 <= 32, ("Wrap-around in S-registers for %s instruction", arm_opcode_table[opc].desc));

        for(i=dreg; i < dreg+numregs; i++)
        {
          ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), i));
        }
      }
      break;

    case ARM_VLDR:
      ARM_INS_SET_TYPE(ins, IT_FLT_LOAD);

    case ARM_VSTR:
      if(ARM_INS_OPCODE(ins) == ARM_VSTR)
        ARM_INS_SET_TYPE(ins, IT_FLT_STORE);

      ARM_INS_SET_REGA(ins, (instr & 0x00000100) ? NEON_VD_QD(instr) : NEON_VD_S(instr));
      ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| ((instr & 0x00000100) ? FL_VFP_DOUBLE : 0));
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| ((instr & 0x00800000) ? FL_DIRUP : 0));
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x000000ff) << 2);
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleFPDataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_arm_ins_dt dt = DT_NONE, dtop = DT_NONE;
  t_uint32 imm = 0;

  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_TYPE(ins, IT_FLT_ALU);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);

  switch(opc)
  {
    case ARM_VMOV_F:
    case ARM_VABS_F64:
    case ARM_VNEG_F64:
    case ARM_VSQRT:
      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
        ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);
        ARM_INS_SET_DATATYPE(ins, DT_F64);
      }
      else
      {
        ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
        ARM_INS_SET_REGB(ins, NEON_VM_S(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F32);
      }
      break;

    case ARM_VCVTB:
    case ARM_VCVTT:
      VERBOSE(1,("WARNING --- VCVTB/VCVTT instruction implemented but not tested: 0x%08x", instr));

      if(instr & 0x00010000)
      {
        ARM_INS_SET_DATATYPE(ins, DT_F16);
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
      }
      else
      {
        ARM_INS_SET_DATATYPE(ins, DT_F32);
        ARM_INS_SET_DATATYPEOP(ins, DT_F16);
      }

      ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_S(instr));

      break;

    case ARM_VCVT_DS:
      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
        ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F32);
        ARM_INS_SET_DATATYPEOP(ins, DT_F64);
      }
      else
      {
        ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
        ARM_INS_SET_REGB(ins, NEON_VM_S(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F64);
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
      }
      break;

    case ARM_VCVT_X2F:
    case ARM_VCVT_F2X:
      /* integer datatype */
      dt = (instr & 0x00010000) ? DT_U_START : DT_S_START;
      dt += (instr & 0x00000080) ? DT_32 : DT_16;
      /* correct for DT_32 and DT_16 base 1 */
      dt--;

      /* registers and floating-point datatype */
      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
        ARM_INS_SET_REGB(ins, NEON_VD_QD(instr));
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);
        dtop = DT_F64;
      }
      else
      {
        ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
        ARM_INS_SET_REGB(ins, NEON_VD_S(instr));
        dtop = DT_F32;
      }

      imm = ((instr & 0x0000000f) << 1) | ((instr & 0x00000020) >> 5);
      if(instr & 0x00000080)
        imm = 32-imm;
      else
        imm = 16-imm;

      ARM_INS_SET_IMMEDIATE(ins, imm);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

      if(opc == ARM_VCVT_X2F)
      {
        t_arm_ins_dt temp = dt;
        dt = dtop;
        dtop = temp;
      }

      ARM_INS_SET_DATATYPE(ins, dt);
      ARM_INS_SET_DATATYPEOP(ins, dtop);
      break;

    case ARM_VCVT_I2F:
      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F64);
      }
      else
      {
        ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F32);
      }

      ARM_INS_SET_REGB(ins, NEON_VM_S(instr));
      ARM_INS_SET_DATATYPEOP(ins, (instr & 0x00000080) ? DT_S32 : DT_U32);
      break;

    case ARM_VCVT_F2I:
      VERBOSE(1,("WARNING --- VCVT (floating->integer) instruction implemented but not tested: 0x%08x", instr));

    case ARM_VCVTR_F2I:
      ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
      ARM_INS_SET_DATATYPE(ins, (instr & 0x00010000) ? DT_S32 : DT_U32);

      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
        ARM_INS_SET_DATATYPEOP(ins, DT_F64);
      }
      else
      {
        ARM_INS_SET_REGB(ins, NEON_VM_S(instr));
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
      }
      break;

    case ARM_VFNMA:
    case ARM_VFNMS:
    case ARM_VFMA_F64:
    case ARM_VFMS_F64:
      VERBOSE(1,("WARNING --- VFNMA/VFMA(floating) instruction implemented but not tested: 0x%08x", instr));

    case ARM_VMLA_F64:
    case ARM_VMLS_F64:
    case ARM_VNMLA:
    case ARM_VNMLS:
    case ARM_VNMUL:
    case ARM_VMUL_F64:
    case ARM_VADD_F64:
    case ARM_VSUB_F64:
    case ARM_VDIV:
      if(instr & 0x00000100)
      {
        ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
        ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
        ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);
        ARM_INS_SET_DATATYPE(ins, DT_F64);
      }
      else
      {
        ARM_INS_SET_REGA(ins, NEON_VD_S(instr));
        ARM_INS_SET_REGB(ins, NEON_VN_S(instr));
        ARM_INS_SET_REGC(ins, NEON_VM_S(instr));
        ARM_INS_SET_DATATYPE(ins, DT_F32);
      }
      break;

    case ARM_VCMP:
    case ARM_VCMPE:
      if(instr & 0x00000100)
      {
        /* double */
        ARM_INS_SET_REGB(ins, NEON_VD_QD(instr));
        if(instr & 0x00010000)
        {
          ARM_INS_SET_IMMEDIATE(ins, 0);
          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
        }
        else
        {
          ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
        }
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);
        ARM_INS_SET_DATATYPE(ins, DT_F64);
      }
      else
      {
        /* single */
        ARM_INS_SET_REGB(ins, NEON_VD_S(instr));
        if(instr & 0x00010000)
        {
          ARM_INS_SET_IMMEDIATE(ins, 0);
          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
        }
        else
        {
          ARM_INS_SET_REGC(ins, NEON_VM_S(instr));
        }
        ARM_INS_SET_DATATYPE(ins, DT_F32);
      }
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 r1, r2;
  t_reg vm;

  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  switch(opc)
  {
    case ARM_VMOV64_C2S:
      r1 = (instr & 0x0000f000) >> 12;
      r2 = (instr & 0x000f0000) >> 16;
      vm = NEON_VM_S(instr);
      ASSERT(vm < ARM_REG_S31, ("Illegal S-register for %s instruction", arm_opcode_table[opc].desc));

      if(instr & 0x00100000)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_SINGLE | NEONFL_C_SINGLE));
        ARM_INS_SET_REGA(ins, r1);
        ARM_INS_SET_REGABIS(ins, r2);
        ARM_INS_SET_REGB(ins, vm);
        ARM_INS_SET_REGC(ins, vm+1);
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SINGLE | NEONFL_B_CORE | NEONFL_C_CORE));
        ARM_INS_SET_REGA(ins, vm);
        ARM_INS_SET_REGABIS(ins, vm+1);
        ARM_INS_SET_REGB(ins, r1);
        ARM_INS_SET_REGC(ins, r2);
      }
      break;

    case ARM_VMOV64_C2D:
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_VFP_DOUBLE);

      r1 = (instr & 0x0000f000) >> 12;
      r2 = (instr & 0x000f0000) >> 16;
      vm = NEON_VM_QD(instr);

      if(instr & 0x00100000)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_DOUBLE));
        ARM_INS_SET_REGA(ins, r1);
        ARM_INS_SET_REGABIS(ins, r2);
        ARM_INS_SET_REGB(ins, vm);
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_CORE | NEONFL_C_CORE));
        ARM_INS_SET_REGA(ins, vm);
        ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
        ARM_INS_SET_REGB(ins, r1);
        ARM_INS_SET_REGC(ins, r2);
      }
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleGenericProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  ARM_INS_SET_TYPE(ins, IT_UNKNOWN);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  /* unconditional instruction */
  ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);

  switch(opc)
  {
    case ARM_CLREX:
      /* no action needed */
      break;

    case ARM_NOP:
      ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
      if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

      ARM_INS_SET_TYPE(ins, IT_NOP);
      break;

    case ARM_SMC:
      ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
      if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
        ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

      ARM_INS_SET_IMMEDIATE(ins, instr & 0x0000000f);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED);
      break;

    case ARM_SETEND:
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x00000200) >> 9);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED);
      FATAL(("The SETEND instruction is currently not supported: @I", ins));
      break;

    case ARM_CPSIE:
    case ARM_CPSID:
    case ARM_CPS:
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x000200ef));
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED);
      FATAL(("The CPS instruction and its variants are currently not supported: @I", ins));
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleBarrier(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* unconditional instruction */
  ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);

  ARM_INS_SET_TYPE(ins, IT_SYNC);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);

  switch(opc)
  {
    case ARM_DMB:
    case ARM_DSB:
    case ARM_ISB:
      ARM_INS_SET_IMMEDIATE(ins, instr & 0x0000000f);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED);
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleHint(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_TYPE(ins, IT_SYNC);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);

  switch(opc)
  {
    case ARM_YIELD:
    case ARM_WFE:
    case ARM_WFI:
    case ARM_SEV:
      /* no action needed */
      break;

    case ARM_DBG:
      ARM_INS_SET_IMMEDIATE(ins, instr & 0x0000000f);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_IMMED);
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}

void ArmDisassembleBitfield(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_TYPE(ins, IT_DATAPROC);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);

  ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
  ASSERT(ARM_INS_REGA(ins)!=ARM_REG_R15, ("PC can not be the destination register in %s instruction", arm_opcode_table[opc].desc));
  /* by default, the modified register is the same as the destination register */
  ARM_INS_SET_REGB(ins, ARM_INS_REGA(ins));

  switch(opc)
  {
    case ARM_RBIT:
      ARM_INS_SET_REGB(ins, instr & 0x0000000f);
      break;

    case ARM_BFI:
    case ARM_SBFX:
    case ARM_UBFX:
      ARM_INS_SET_REGB(ins, instr & 0x0000000f);

    case ARM_BFC:
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x001f0000) | ((instr & 0x00000f80) >> 7));
      break;

    default:
      FATAL(("Illegal opcode: %s", arm_opcode_table[opc].desc));
  }
}
void ArmDisassembleV7DataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  /* generic information */
  ARM_INS_SET_OPCODE(ins, opc);
  ARM_INS_SET_ATTRIB(ins, 0x0);

  /* condition code */
  ARM_INS_SET_CONDITION(ins,  ARM_EXTRACT_CONDITION_CODE(instr));
  if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);

  ARM_INS_SET_TYPE(ins, IT_DATAPROC);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_DATATYPEOP(ins, DT_NONE);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);

  ARM_INS_SET_REGA(ins, (instr & 0x000f0000) >> 16);
  ARM_INS_SET_REGB(ins, instr & 0x0000000f);
  ARM_INS_SET_REGC(ins, (instr & 0x00000f00) >> 8);
}

void ArmDisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  FATAL(("instruction not implemented: %s", arm_opcode_table[opc].desc));
}

/*}}}*/
/* vim: set shiftwidth=2 foldmethod=marker : */
