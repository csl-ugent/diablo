/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#define IS_THUMB_REG(x) (((x) >= ARM_REG_R0) && ((x) <= ARM_REG_R7))
#define THUMB_ASSEMBLER_VERBOSITY_LEVEL 3

int insregToQDregnumT(t_reg reg)
{
  int num = (reg - ARM_REG_S0) / 2;

  if(reg >= ARM_REG_D16)
  {
    num = (reg - ARM_REG_D16) + 16;
  }

  return num;
}

t_uint32 ASMT_NEON_VD_S(t_uint32 reg)
{
  reg -= ARM_REG_S0;
  return ((reg & 0x01) << 22) | ((reg & 0x1e) << 11);
}
t_uint32 ASMT_NEON_VD_QD(t_uint32 reg)
{
  int num = insregToQDregnumT(reg);
  return ((num & 0x10) << 18) | ((num & 0x0f) << 12);
}

void Thumb32EncodeImmediate(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 imm = ARM_INS_IMMEDIATE(ins);
  t_uint32 encoded = 0;
  t_uint32 b = imm & 0x000000ff;
  t_uint32 constant = b;

  if ((imm & ~0x000000ff) == 0)
    encoded = 0;
  else if (((imm & ~0x00ff00ff) == 0) && ((imm >> 16) == (imm & 0xff)))
    encoded = 2;
  else if (((imm & ~0xff00ff00) == 0) && ((imm >> 16) == (imm & 0xff00)))
  {
    encoded = 4;
    constant = (imm >> 8) & 0xff;
  }
  else if (imm == ((b<<24) | (b<<16) | (b<<8) | b))
    encoded = 6;
  else
  {
    /* try different rotations */
    t_uint32 i;
    for(i = 0; i < 24; i++)
    {
      if (imm & 0x80000000)
      {
        encoded = 8+i;
        constant = (imm >> 24) & 0x7f;
        break;
      }

      imm <<= 1;
    }
  }

  /* encode i:imm3:a */
  *instr |= (encoded & 1<<4) << (16+6);
  *instr |= (encoded & 0x0e) << 11;
  *instr |= (encoded & 1) << 7;

  /* encode constant */
  *instr |= constant;
}

void Thumb32EncodeShift(t_arm_ins * ins, t_uint32 * instr)
{
  *instr |= ARM_INS_SHIFTTYPE(ins) << 4;
  *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x03) << 6;
  *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x1c) << 10;
}

void Thumb32AssembleDataprocImmediate(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling dataproc-immediate @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_ADC:
    *instr = thumb_opcode_table[TH32_ADC_IMM].opcode;
    break;

  case ARM_ADD:
    *instr = thumb_opcode_table[TH32_ADD_IMM].opcode;
    break;

  case ARM_AND:
    *instr = thumb_opcode_table[TH32_AND_IMM].opcode;
    break;

  case ARM_BIC:
    *instr = thumb_opcode_table[TH32_BIC_IMM].opcode;
    break;

  case ARM_EOR:
    *instr = thumb_opcode_table[TH32_EOR_IMM].opcode;
    break;

  case ARM_MOV:
    *instr = thumb_opcode_table[TH32_MOV_IMM].opcode;
    break;

  case ARM_CMN:
    *instr = thumb_opcode_table[TH32_CMN_IMM].opcode;
    break;

  case ARM_CMP:
    *instr = thumb_opcode_table[TH32_CMP_IMM].opcode;
    break;

  case ARM_MVN:
    *instr = thumb_opcode_table[TH32_MVN_IMM].opcode;
    break;

  case ARM_T2ORN:
    *instr = thumb_opcode_table[TH32_ORN_IMM].opcode;
    break;

  case ARM_ORR:
    *instr = thumb_opcode_table[TH32_ORR_IMM].opcode;
    break;

  case ARM_RSB:
    *instr = thumb_opcode_table[TH32_RSB_IMM].opcode;
    break;

  case ARM_SBC:
    *instr = thumb_opcode_table[TH32_SBC_IMM].opcode;
    break;

  case ARM_SUB:
    *instr = thumb_opcode_table[TH32_SUB_IMM].opcode;
    break;

  case ARM_TEQ:
    *instr = thumb_opcode_table[TH32_TEQ_IMM].opcode;
    break;

  case ARM_TST:
    *instr = thumb_opcode_table[TH32_TST_IMM].opcode;
    break;

  default:
    break;
  }

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_ADD:
  case ARM_SUB:
  case ARM_ADC:
  case ARM_AND:
  case ARM_BIC:
  case ARM_EOR:
  case ARM_MOV:
  case ARM_MVN:
  case ARM_T2ORN:
  case ARM_ORR:
  case ARM_RSB:
  case ARM_SBC:
    if (((ARM_INS_OPCODE(ins) == ARM_ADD) || (ARM_INS_OPCODE(ins) == ARM_SUB)) && (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)) || (ARM_INS_REGB(ins) == ARM_REG_R15)))
    {
      /* try encoding ADDW/SUBW */
      ASSERT((ARM_INS_IMMEDIATE(ins) & ~0xfff) == 0, ("ADDW/SUBW only supports 12-bit immediates @I", ins));

      if (ARM_INS_OPCODE(ins) == ARM_ADD)
        *instr = thumb_opcode_table[TH32_ADDW].opcode;
      else
        *instr = thumb_opcode_table[TH32_SUBW].opcode;

      *instr |= ((ARM_INS_IMMEDIATE(ins) & 0x800) >> 11) << (16+10);
      *instr |= (ARM_INS_IMMEDIATE(ins) & 0x700) << 4;
      *instr |= ARM_INS_IMMEDIATE(ins) & 0x0ff;
    }
    else
    {
      *instr |= (ARM_INS_FLAGS(ins) & FL_S) ? 1<<(16+4) : 0;
      ASSERT(ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)), ("immediate %llx not encodable for @I", ARM_INS_IMMEDIATE(ins), ins));
      Thumb32EncodeImmediate(ins, instr);
    }

    if ((ARM_INS_OPCODE(ins) != ARM_MOV) &&
        (ARM_INS_OPCODE(ins) != ARM_MVN))
      *instr |= ARM_INS_REGB(ins) << 16;

    *instr |= ARM_INS_REGA(ins) << 8;
    break;

  case ARM_CMN:
  case ARM_CMP:
  case ARM_TEQ:
  case ARM_TST:
    *instr |= ARM_INS_REGB(ins) << 16;
    ASSERT(ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)), ("immediate %llx not encodable for @I", ARM_INS_IMMEDIATE(ins), ins));
    Thumb32EncodeImmediate(ins, instr);
    break;

  case ARM_MOVW:
  case ARM_MOVT:
    *instr |= ARM_INS_REGA(ins) << 8;

    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x0800) << 15;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0xf000) << 4;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x0700) << 4;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x00ff);
    break;

  default:
    FATAL(("unsupported dataprocessing (immediate) Thumb32 instruction @I", ins));
  }
}

void Thumb32AssembleDataprocRegister(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling dataproc-register @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_ADC:
  case ARM_ADD:
  case ARM_AND:
  case ARM_BIC:
  case ARM_EOR:
  case ARM_MVN:
  case ARM_T2ORN:
  case ARM_ORR:
  case ARM_PKHBT:
  case ARM_PKHTB:
  case ARM_RSB:
  case ARM_SBC:
  case ARM_SUB:
    *instr |= (ARM_INS_FLAGS(ins) & FL_S) ? 1<<(16+4) : 0;
    *instr |= ARM_INS_REGA(ins) << 8;

  case ARM_CMN:
  case ARM_CMP:
  case ARM_TEQ:
  case ARM_TST:
    if (ARM_INS_OPCODE(ins) != ARM_MVN)
      *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGC(ins);

    if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
    {
      *instr |= ARM_INS_SHIFTTYPE(ins) << 4;
      *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x03) << 6;
      *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x1c) << 10;
      ASSERT((ARM_INS_SHIFTTYPE(ins) & 4) == 0, ("only immediate shift is supported in @I", ins));
    }
    break;

  case ARM_MOV:
    switch(ARM_INS_SHIFTTYPE(ins))
    {
    case ARM_SHIFT_TYPE_ASR_IMM:
      *instr = thumb_opcode_table[TH32_ASR_IMM].opcode;
      break;

    case ARM_SHIFT_TYPE_ASR_REG:
      *instr = thumb_opcode_table[TH32_ASR].opcode;
      break;

    case ARM_SHIFT_TYPE_LSL_IMM:
      *instr = thumb_opcode_table[TH32_LSL_IMM].opcode;
      break;

    case ARM_SHIFT_TYPE_LSL_REG:
      *instr = thumb_opcode_table[TH32_LSL].opcode;
      break;

    case ARM_SHIFT_TYPE_LSR_IMM:
      *instr = thumb_opcode_table[TH32_LSR_IMM].opcode;
      break;

    case ARM_SHIFT_TYPE_LSR_REG:
      *instr = thumb_opcode_table[TH32_LSR].opcode;
      break;

    case ARM_SHIFT_TYPE_ROR_IMM:
      *instr = thumb_opcode_table[TH32_ROR_IMM].opcode;
      break;

    case ARM_SHIFT_TYPE_ROR_REG:
      *instr = thumb_opcode_table[TH32_ROR].opcode;
      break;

    case ARM_SHIFT_TYPE_RRX:
      *instr = thumb_opcode_table[TH32_RRX].opcode;
      break;

    case ARM_SHIFT_TYPE_NONE:
      *instr = thumb_opcode_table[TH32_MOV].opcode;
      break;

    default:
      FATAL(("unsupported shift type in Thumb32 MOV immediate @I", ins));
    }

    switch(ARM_INS_SHIFTTYPE(ins))
    {
    case ARM_SHIFT_TYPE_ASR_IMM:
    case ARM_SHIFT_TYPE_LSL_IMM:
    case ARM_SHIFT_TYPE_LSR_IMM:
    case ARM_SHIFT_TYPE_ROR_IMM:
      *instr |= ARM_INS_REGA(ins) << 8;
      *instr |= ARM_INS_REGC(ins);

      *instr |= ARM_INS_SHIFTTYPE(ins) << 4;
      *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x03) << 6;
      *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x1c) << 10;
      break;

    case ARM_SHIFT_TYPE_ASR_REG:
    case ARM_SHIFT_TYPE_LSL_REG:
    case ARM_SHIFT_TYPE_LSR_REG:
    case ARM_SHIFT_TYPE_ROR_REG:
      *instr |= ARM_INS_REGA(ins) << 8;
      *instr |= ARM_INS_REGC(ins) << 16;
      *instr |= ARM_INS_REGS(ins);
      break;

    case ARM_SHIFT_TYPE_RRX:
    case ARM_SHIFT_TYPE_NONE:
      *instr |= ARM_INS_REGA(ins) << 8;
      *instr |= ARM_INS_REGC(ins);
      break;

    default:
      FATAL(("unsupported shift type in Thumb32 MOV immediate @I", ins));
    }

    *instr |= (ARM_INS_FLAGS(ins) & FL_S) ? 1<<(16+4) : 0;
    break;

  case ARM_CLZ:
  case ARM_REV:
  case ARM_REV16:
  case ARM_REVSH:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= ARM_INS_REGC(ins) << 16;
    *instr |= ARM_INS_REGC(ins);
    break;

  case ARM_USADA8:
    *instr |= ARM_INS_REGS(ins) << 12;
  case ARM_QADD:
  case ARM_QADD16:
  case ARM_QADD8:
  case ARM_QADDSUBX:
  case ARM_QDADD:
  case ARM_QDSUB:
  case ARM_QSUBADDX:
  case ARM_QSUB:
  case ARM_QSUB16:
  case ARM_QSUB8:
  case ARM_SADD16:
  case ARM_SADD8:
  case ARM_SADDSUBX:
  case ARM_SDIV:
  case ARM_SEL:
  case ARM_SHADD16:
  case ARM_SHADD8:
  case ARM_SHADDSUBX:
  case ARM_SHSUBADDX:
  case ARM_SHSUB16:
  case ARM_SHSUB8:
  case ARM_SSUBADDX:
  case ARM_SSUB16:
  case ARM_SSUB8:
  case ARM_UADD16:
  case ARM_UADD8:
  case ARM_UADDSUBX:
  case ARM_UDIV:
  case ARM_UHADD16:
  case ARM_UHADD8:
  case ARM_UHADDSUBX:
  case ARM_UHSUBADDX:
  case ARM_UHSUB16:
  case ARM_UHSUB8:
  case ARM_UQADD16:
  case ARM_UQADD8:
  case ARM_UQADDSUBX:
  case ARM_UQSUBADDX:
  case ARM_UQSUB16:
  case ARM_UQSUB8:
  case ARM_USAD8:
  case ARM_USUBADDX:
  case ARM_USUB16:
  case ARM_USUB8:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGC(ins);
    break;

  case ARM_RBIT:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGB(ins);
    break;

  case ARM_SSAT:
  case ARM_USAT:
    ASSERT((ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM) || (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM) || (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE),
                ("saturation instructions only support ASR/LSL shift @I", ins));
    ASSERT((ARM_INS_IMMEDIATE(ins)-1) < 32, ("bit position for saturation should be smaller than, or equal to 32 @I", ins));

    if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
      *instr |= ARM_INS_SHIFTTYPE(ins) << (16+4);
    *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x1c) << 10;
    *instr |= (ARM_INS_SHIFTLENGTH(ins) & 0x03) << 6;

  case ARM_SSAT16:
  case ARM_USAT16:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= ARM_INS_REGC(ins) << 16;

    *instr |= ARM_INS_IMMEDIATE(ins) - 1;
    break;

  case ARM_SXTAB:
  case ARM_SXTAB16:
  case ARM_SXTAH:
  case ARM_UXTAB:
  case ARM_UXTAB16:
  case ARM_UXTAH:
    *instr |= ARM_INS_REGB(ins) << 16;

  case ARM_SXTB:
  case ARM_SXTB16:
  case ARM_SXTH:
  case ARM_UXTB:
  case ARM_UXTB16:
  case ARM_UXTH:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= ARM_INS_REGC(ins);

    *instr |= ARM_INS_SHIFTLENGTH(ins) << 1;
    break;

  case ARM_BFI:
  case ARM_SBFX:
  case ARM_UBFX:
    *instr |= ARM_INS_REGB(ins) << 16;

  case ARM_BFC:
    *instr |= ARM_INS_REGA(ins) << 8;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x001f0000) >> 16;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x00000003) << 6;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x0000001c) << 10;
    break;

  default:
    FATAL(("unsupported dataprocessing (register) Thumb32 instruction @I", ins));
  }
}

void Thumb32AssembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling load-store exclusive @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_CLREX:
    break;

  case ARM_LDREX:
  case ARM_LDREXB:
  case ARM_LDREXD:
  case ARM_LDREXH:
    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGA(ins) << 12;

    if (ARM_INS_OPCODE(ins) == ARM_LDREXD)
      *instr |= ARM_INS_REGABIS(ins) << 8;
    break;

  case ARM_STREX:
  case ARM_STREXB:
  case ARM_STREXD:
  case ARM_STREXH:
    *instr |= ARM_INS_REGA(ins) << 12;
    *instr |= ARM_INS_REGB(ins) << 16;

    if (ARM_INS_OPCODE(ins) == ARM_STREX)
      *instr |= ARM_INS_REGC(ins) << 8;
    else
      *instr |= ARM_INS_REGC(ins);

    if (ARM_INS_OPCODE(ins) == ARM_STREXD)
      *instr |= ARM_INS_REGABIS(ins) << 8;
    break;

  default:
    FATAL(("unsupported load/store exclusive instruction @I", ins));
  }

  if ((ARM_INS_OPCODE(ins) == ARM_LDREX) || (ARM_INS_OPCODE(ins) == ARM_STREX))
  {
    ASSERT((ARM_INS_IMMEDIATE(ins) & ~(0xff << 2)) == 0, ("LDREX/STREX immediate can only be 8 bits wide @I", ins));
    *instr |= ARM_INS_IMMEDIATE(ins) >> 2;
  }
}

void Thumb32AssembleLoadStore(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling load/store @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_LDR:
  case ARM_LDRB:
  case ARM_LDRH:
  case ARM_LDRSB:
  case ARM_LDRSH:
  case ARM_PLD:
  case ARM_PLDW:
  case ARM_PLI:

  case ARM_STR:
  case ARM_STRB:
  case ARM_STRH:
    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      /* immediate */

      /* first try to encode in a 12-bit immediate, then try an 8-bit immediate */
      if ((((ARM_INS_IMMEDIATE(ins) & ~0xfff) == 0) && ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_PREINDEX | FL_DIRUP))) ||
          (ARM_INS_REGB(ins) == ARM_REG_R15))
      {
        /* encoding can be done in 12-bit immediate, but with a small exception for literal loads */
        if (ARM_INS_REGB(ins) == ARM_REG_R15)
          *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
        else
          *instr |= 1<<(16+7);

        *instr |= ARM_INS_IMMEDIATE(ins);

        *instr |= ARM_INS_REGB(ins) << 16;

        if ((ARM_INS_OPCODE(ins) != ARM_PLD) &&
            (ARM_INS_OPCODE(ins) != ARM_PLDW) &&
            (ARM_INS_OPCODE(ins) != ARM_PLI))
        {
          *instr |= ARM_INS_REGA(ins) << 12;
        }
      }
      else if((ARM_INS_IMMEDIATE(ins) & ~0xff) == 0)
      {
        /* POP/PUSH or 8-bit immediate */

        if ((ARM_INS_IMMEDIATE(ins) == 4) &&
            ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_DIRUP | FL_WRITEBACK)) &&
            (ARM_INS_REGB(ins) == ARM_REG_R13) &&
            (ARM_INS_OPCODE(ins) == ARM_LDR))
        {
          /* POP (one register) */
          *instr = thumb_opcode_table[TH32_POPONE].opcode;
          *instr |= ARM_INS_REGA(ins) << 12;
        }
        else if ((ARM_INS_IMMEDIATE(ins) == 4) &&
                  ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_PREINDEX | FL_WRITEBACK)) &&
                  (ARM_INS_REGB(ins) == ARM_REG_R13) &&
                  (ARM_INS_OPCODE(ins) == ARM_STR))
        {
          /* PUSH (one register) */
          *instr |= thumb_opcode_table[TH32_PUSHONE].opcode;
          *instr |= ARM_INS_REGA(ins) << 12;
        }
        else
        {
          /* LDR (8-bit immediate) */
          *instr |= 1<<11;

          *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<10 : 0;
          *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<9 : 0;
          *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<8 : 0;

          *instr |= ARM_INS_IMMEDIATE(ins);

          *instr |= ARM_INS_REGB(ins) << 16;

          if ((ARM_INS_OPCODE(ins) != ARM_PLD) &&
              (ARM_INS_OPCODE(ins) != ARM_PLDW) &&
              (ARM_INS_OPCODE(ins) != ARM_PLI))
          {
            *instr |= ARM_INS_REGA(ins) << 12;
          }
        }
      }
      else
      {
        FATAL(("can't assemble load/store instruction @I", ins));
      }
    }
    else
    {
      /* register */
      if ((ARM_INS_OPCODE(ins) != ARM_PLD) &&
          (ARM_INS_OPCODE(ins) != ARM_PLDW) &&
          (ARM_INS_OPCODE(ins) != ARM_PLI))
      {
        *instr |= ARM_INS_REGA(ins) << 12;
      }

      *instr |= ARM_INS_REGB(ins) << 16;

      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
      {
        ASSERT(ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM, ("load/store (register) can only have an LSL (immediate) shift @I", ins));
        ASSERT(ARM_INS_SHIFTLENGTH(ins) <= 3, ("load/store (register) can only be shifted with a maximum of 3 positions @I", ins));

        *instr |= ARM_INS_SHIFTLENGTH(ins) << 4;
      }

      *instr |= ARM_INS_REGC(ins);
    }
    break;

  case ARM_LDRD:
  case ARM_STRD:
    if (ARM_INS_FLAGS(ins) & FL_IMMED)
    {
      /* immediate */
      ASSERT((ARM_INS_IMMEDIATE(ins) & ~(0xff << 2)) == 0, ("load/store double immediate should be 8 bits wide @I", ins));

      *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<(16+8) : 0;
      *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
      *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;

      *instr |= ARM_INS_REGB(ins) << 16;
      *instr |= ARM_INS_REGA(ins) << 12;
      *instr |= ARM_INS_REGABIS(ins) << 8;
      *instr |= ARM_INS_IMMEDIATE(ins) >> 2;
    }
    else
    {
      /* register */
      FATAL(("can't assemble load/store double instruction @I", ins));
    }
    break;

  default:
    FATAL(("unsupported load/store instruction: @I", ins));
  }
}

void Thumb32AssembleLoadStoreMultiple(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling load/store multiple @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_LDM:
    if ((ARM_INS_FLAGS(ins) & FL_PREINDEX) && !(ARM_INS_FLAGS(ins) & FL_DIRUP))
      *instr = thumb_opcode_table[TH32_LDMDB].opcode;
    else if (!(ARM_INS_FLAGS(ins) & FL_PREINDEX) && (ARM_INS_FLAGS(ins) & FL_DIRUP))
      *instr = thumb_opcode_table[TH32_LDM].opcode;
    else
      FATAL(("can't assemble LDM instruction @I", ins));
    break;

  case ARM_STM:
    if ((ARM_INS_FLAGS(ins) & FL_PREINDEX) && !(ARM_INS_FLAGS(ins) & FL_DIRUP))
      *instr = thumb_opcode_table[TH32_STMDB].opcode;
    else if (!(ARM_INS_FLAGS(ins) & FL_PREINDEX) && (ARM_INS_FLAGS(ins) & FL_DIRUP))
      *instr = thumb_opcode_table[TH32_STM].opcode;
    else
      FATAL(("can't assemble STM instruction @I", ins));
    break;

  default:
    FATAL(("unsupported load/store multiple instruction @I", ins));
  }

  ASSERT(Uint32CountSetBits(ARM_INS_IMMEDIATE(ins)) >= 2, ("number of selected registers should be larger than 1: @I", ins));

  *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;
  *instr |= ARM_INS_REGB(ins) << 16;
  *instr |= ARM_INS_IMMEDIATE(ins) & 0xffff;
}

void Thumb32AssembleBranch(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling branch @I", ins));
  t_uint32 imm = ARM_INS_IMMEDIATE(ins);

  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_BL:
  case ARM_BLX:
    ASSERT(ARM_INS_FLAGS(ins) & FL_IMMED, ("only branch immediate instructions are supported in 32-bit Thumb @I", ins));

  case ARM_B:
    if (ArmInsIsConditional(ins) && (ARM_INS_OPCODE(ins) == ARM_B))
    {
      /* encoding B(T3): conditional branch */
      ASSERT(Uint32CheckSignExtend(ARM_INS_IMMEDIATE(ins), 20), ("conditional branch can only have a 21-bit sign-extended immediate @I", ins));

      *instr |= (imm & 1<<20) << 6;
      *instr |= ARM_INS_CONDITION(ins) << (16+6);
      *instr |= (imm & 0x0003f000) << 4;
      *instr |= (imm & 1<<18) >> 5;
      *instr |= (imm & 1<<19) >> 8;
      *instr |= (imm & 0x00000ffe) >> 1;
    }
    else
    {
      /* B(T4) / BLX */
      t_uint32 s = (imm & 1<<24) ? 1 : 0;

      ASSERT(Uint32CheckSignExtend(ARM_INS_IMMEDIATE(ins), 24), ("branch can only have a 24-bit sign-extended immediate @I", ins));

      *instr |= s<<(16+10);
      /* J1, J2 */
      *instr |= ((s<<13) ^ ((imm & 1<<23) >> 10)) ^ 1<<13;
      *instr |= ((s<<11) ^ ((imm & 1<<22) >> 11)) ^ 1<<11;

      *instr |= (imm & 0x00000ffe) >> 1;
      *instr |= (imm & 0x003ff000) << 4;

      if (ARM_INS_OPCODE(ins) == ARM_B)
        *instr |= 1<<12;
    }

    break;

  case ARM_T2TBB:
  case ARM_T2TBH:
    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_REGC(ins);
    break;

  default:
    FATAL(("unsupported instruction: @I", ins));
  }
}

void Thumb32AssembleHint(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling hint @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_NOP:
  case ARM_SEV:
    break;

  case ARM_DBG:
  case ARM_DMB:
  case ARM_DSB:
  case ARM_ISB:
    *instr |= ARM_INS_IMMEDIATE(ins);
    break;

  default:
    FATAL(("unsupported instruction: @I", ins));
  }
}

void Thumb32AssembleCoproc(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling coprocessor @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_CDP:
  case ARM_CDP2:
    *instr |= ARM_INS_IMMEDIATE(ins);
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

      *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<(16+8) : 0;
      *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
      *instr |= (ARM_INS_FLAGS(ins) & FL_LONG_TRANSFER) ? 1<<(16+6) : 0;
      *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;
    }
    break;

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

      *instr |= (opc1 << 21) | (Rn << 16) | (Rd << 12) | (coproc << 8) | (opc2 << 5) | Rm;
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

  default:
    FATAL(("unsupported coprocessor instruction @I", ins));
  }
}

void Thumb32AssembleMultiply(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling multiply @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  *instr |= ARM_INS_REGA(ins) << 8;
  *instr |= ARM_INS_REGB(ins) << 16;
  *instr |= ARM_INS_REGC(ins);

  if (ARM_INS_REGS(ins) != ARM_REG_NONE)
    *instr |= ARM_INS_REGS(ins) << 12;
}

void Thumb32AssembleStatus(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling status @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_MRS:
    *instr |= ARM_INS_REGA(ins) << 8;
    break;

  case ARM_MSR:
    ASSERT((ARM_INS_FLAGS(ins) & FL_IMMED) == 0, ("MSR (immediate) instruction variant not supported in Thumb32 @I", ins));

    *instr |= ARM_INS_REGC(ins) << 16;

    if (ARM_INS_FLAGS(ins) & (FL_STATUS | FL_S))
      *instr |= 1<<11;
    else
      FATAL(("unsupported MSR variant @I", ins));

    break;

  default:
    FATAL(("unsupported status instruction @I", ins));
  }
}

#define ASM_VFP_FD(_reg) (((((_reg)-ARM_REG_S0) & 0x1e) << 11) | ((((_reg)-ARM_REG_S0) & 0x1) << 22))
void Thumb32AssembleVLoadStore(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling vector load/store @I", ins));
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_FSTMX:
  case ARM_FLDMX:
  {
    t_reg tel, max = ARM_REG_NONE, min = ARM_REG_NONE;
    for (tel = ARM_REG_S0; tel <= ARM_REG_S31; tel++)
    {
      if (RegsetIn(ARM_INS_MULTIPLE(ins), tel))
      {
        if ((max != ARM_REG_NONE) && (max+1 != tel))
        {
          FATAL(("wrong regs for VFP load/store multiple"));
        }
        else if (min == ARM_REG_NONE)
        {
          min = max = tel;
        }
        else
        {
          max = tel;
        }
      }
    }

    ASSERT((max-min-ARM_INS_REGC(ins)) <= 15, ("too many registers in VFP load/store multiple"));
    ASSERT(min == ARM_INS_REGC(ins), ("REGC not set correctly for VFP load/store multiple"));

    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ASM_VFP_FD(ARM_INS_REGC(ins));
    *instr |= max-min+1;

    *instr |= 1;

    *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<(16+8) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;
  }
    break;

  case ARM_VLDM:
  case ARM_VSTM:
  case ARM_VPOP:
  case ARM_VPUSH:
  {
    t_uint32 len = 0;
    t_reg reg = ARM_REG_NONE;

    *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<(16+8) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;

    *instr |= ARM_INS_REGB(ins) << 16;

    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
    {
      *instr |= 1<<8;
      *instr |= ASMT_NEON_VD_QD(RegsetFindFirstBlob(ARM_INS_MULTIPLE(ins), &len));
    }
    else
      *instr |= ASMT_NEON_VD_S(RegsetFindFirstBlob(ARM_INS_MULTIPLE(ins), &len));

    len = 0;
    REGSET_FOREACH_REG(ARM_INS_MULTIPLE(ins), reg)
      len++;

    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      len *= 2;

    *instr |= len & 0xff;
  }
    break;

  case ARM_VLDR:
  case ARM_VSTR:
  {
    *instr |= (ARM_INS_FLAGS(ins) & FL_PREINDEX) ? 1<<(16+8) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_DIRUP) ? 1<<(16+7) : 0;
    *instr |= (ARM_INS_FLAGS(ins) & FL_WRITEBACK) ? 1<<(16+5) : 0;

    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      *instr|=ASM_NEON_VD_QD(ARM_INS_REGA(ins)) | 0x100;
    else
      *instr|=ASM_NEON_VD_S(ARM_INS_REGA(ins));

    *instr |= ARM_INS_REGB(ins) << 16;
    *instr |= ARM_INS_IMMEDIATE(ins) >> 2;

    break;
  }

  default:
    FATAL(("unsupported vector load/store instruction @I", ins));
  }
}

/* ThumbAssembleV6V7ITHints {{{ */
void ThumbAssembleV6V7ITHints(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit hints @I", ins));
  *instr = thumb_opcode_table[ARM_INS_THUMBOPCODE(ins)].opcode;

	/* Set the opcode */
	switch (ARM_INS_OPCODE(ins))
	{
	  case ARM_T2NOP:
	  case ARM_T2YIELD:
	  case ARM_T2WFE:
	  case ARM_T2WFI:
	  case ARM_T2SEV:
	    break;

	  default:
	    FATAL(("Unsupported IT/Hints thumb instruction @I",ins));
	    break;
	}
}
/* }}} */

/*
 * Assembles the instructions that query the ARM Status registers (SPSR and
 * CPSR): MSR, MRS
 *
 * \param ins the instruction to assemble (t_arm_ins*)
 * \param instr the 32 bit placeholder (t_uint32*)
 *
 * \return void
 */
/* ThumbAssembleV6V7ITHints {{{ */
void ThumbAssembleV6V7Status(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit status @I", ins));
	/* Set the opcode */
	switch (ARM_INS_OPCODE(ins))
	{
	  case ARM_MRS:
            *instr = 0x8000f3ef;
            *instr = *instr | (ARM_INS_REGA(ins) << 24);
            if (ARM_INS_FLAGS(ins) & FL_SPSR)
              *instr = *instr | (1 << 4);
	    break;

	  case ARM_MSR:
            *instr = 0x8000f380;
            *instr = *instr | ARM_INS_REGB(ins);
            if (ARM_INS_FLAGS(ins) & FL_SPSR)
              *instr = *instr | (1 << 4);
            if (ARM_INS_FLAGS(ins) & FL_CONTROL)
              *instr = *instr | (1 << 24);
            if (ARM_INS_FLAGS(ins) & FL_STATUS)
              *instr = *instr | (1 << 27);
	    break;

	  default:
	    FATAL(("Unsupported V6V7 special reg instruction @I",ins));
	    break;
	}
}
/* }}} */


/* ThumbAssembleV6Extract {{{ */
void ThumbAssembleV6Extract(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit extract @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));

	/* Set the opcode */
	switch (ARM_INS_OPCODE(ins))
	{
	  case ARM_UXTB:
	    *instr = 0xb2c0;
	    break;
	  case ARM_UXTH:
	    *instr = 0xb280;
	    break;
	  case ARM_SXTB:
	    *instr = 0xb240;
	    break;
	  case ARM_SXTH:
	    *instr = 0xb200;
	    break;
	  case ARM_REV:
	    *instr = 0xba00;
	    break;
    case ARM_REV16:
      *instr = 0xba40;
      break;
    case ARM_REVSH:
      *instr = 0xbac0;
      break;
	  default:
	    FATAL(("Unsupported v6 extract thumb instruction @I",ins));
	    break;
	}
	*instr |= ARM_INS_REGA(ins);
	*instr |= ARM_INS_REGC(ins) << 3;
}
/* }}} */

/* ThumbAssembleCondBranch {{{ */
void ThumbAssembleCondBranch(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit cond branch @I", ins));
  if (G_T_UINT32(ARM_INS_CSIZE(ins)) == 2)
  {
    ASSERT((ARM_INS_IMMEDIATE(ins) & 0x1) == 0, ("immediate of 16-bit Thumb branch should be even @I", ins));
    ASSERT(Uint32CheckSignExtend(ARM_INS_IMMEDIATE(ins), 8), ("16-bit Thumb branch can only have an even 9-bit sign-extended immediate @I", ins));

    *instr = 0xd000;  /* Set the opcode */
    *instr |= (ARM_INS_CONDITION(ins)) << 8;
    *instr |= (ARM_INS_IMMEDIATE(ins) & 0x1ff) >> 1;
  }
  else
  {
    FATAL(("Should not get here (assembling 32-bit Thumb instruction in 16-bit handler) @I", ins));

    t_uint32 offset;
    ASSERT((ARM_INS_IMMEDIATE(ins) & 1)==0,("Invalid branch target"));
    offset = (ARM_INS_IMMEDIATE(ins) & 0x1fffff) >> 1;
    *instr = 0xf000;
    *(instr+1) = 0x8000;
    *instr |= (ARM_INS_CONDITION(ins)) << 6;
    /* imm11 */
    *(instr+1) |= offset & 0x7ff;
    /* imm6 */
    *instr |= ((offset >> 11) & 0x3f);
    /* J1 */
    *(instr+1) |= ((offset >> 17) & 1) << 13;
    /* J2 */
    *(instr+1) |= ((offset >> 18) & 1) << 11;
    /* S */
    *instr |= ((offset >> 19) & 1) << 10;
  }
}
/* }}} */

/* ThumbAssembleBranchLink {{{ */
void ThumbAssembleBranchLink(t_arm_ins * ins, t_uint32 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit branch-link @I", ins));
  ASSERT(ARM_INS_OPCODE(ins) == ARM_BLX, ("Thumb doesn't support 16-bit BL instructions anymore @I", ins));

  *instr = (ARM_INS_OPCODE(ins) == ARM_BL) ? 0xf800f000 : 0xe800f000; /* Set the opcode */
  if ((ARM_INS_IMMEDIATE(ins) & 0x2) && (ARM_INS_OPCODE(ins) == ARM_BLX))
    ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins)+ 2);
  *instr |= (ARM_INS_IMMEDIATE(ins) & 0x00000ffe) << 15;
  *instr |= (ARM_INS_IMMEDIATE(ins) & 0x007ff000) >> 12;
  if (ARM_INS_OPCODE(ins) == ARM_BLX)
    *instr &= 0xfffeffff;
}
/* }}} */

/* ThumbAssembleBranch {{{ */
void ThumbAssembleBranch(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit branch @I", ins));
  *instr = 0;
  if (ARM_INS_FLAGS(ins) & FL_IMMED) {
    if (G_T_UINT32(ARM_INS_CSIZE(ins)) == 2)
    {
      if ((ARM_INS_OPCODE(ins) == ARM_T2CBZ) ||
          (ARM_INS_OPCODE(ins) == ARM_T2CBNZ))
      {
        *instr = thumb_opcode_table[(ARM_INS_OPCODE(ins) == ARM_T2CBZ) ? TH_CBZ : TH_CBNZ].opcode;

        *instr |= ARM_INS_REGB(ins);
        *instr |= (ARM_INS_IMMEDIATE(ins) & 0x0000003e) << 2;
        *instr |= (ARM_INS_IMMEDIATE(ins) & 0x00000040) << 3;

        ASSERT((ARM_INS_REGB(ins) & ~7) == 0, ("only the lower 8 registers are allowed (r%d)", ARM_INS_REGB(ins)));
        ASSERT((ARM_INS_IMMEDIATE(ins) & ~0x7e) == 0, ("CB(N)Z instructions can only have an even immediate <= 126 @I", ins));
      }
      else
      {
        *instr = 0xe000;  /* Set the opcode */
        *instr |= (ARM_INS_IMMEDIATE(ins) & 0xfff) >> 1;

        ASSERT((ARM_INS_IMMEDIATE(ins) & 0x1) == 0, ("16-bit Thumb branch-immediate instructions must have an even immediate @I", ins));
        ASSERT(Uint32CheckSignExtend(ARM_INS_IMMEDIATE(ins), 12), ("16-bit Thumb branch-immediate instructions can only have a 12-bit sign-extended immediate @I", ins));
      }
    }
    else
    {
      FATAL(("Should not get here, 32-bit Thumb instructions have separate handlers! @I", ins));

      t_uint32 offset, S;
      ASSERT((ARM_INS_IMMEDIATE(ins) & 1)==0,("Invalid branch target"));
      offset = (ARM_INS_IMMEDIATE(ins) & 0x1ffffff) >> 1;
      S = (offset >> 23) & 1;
      *instr = 0xf000;
      *(instr+1) = 0x9000;
      /* imm11 */
      *(instr+1) |= offset & 0x7ff;
      /* imm10 */
      *instr |= ((offset >> 11) & 0x3ff);
      /* J2 -- note: order reversed compared to BC */
      *(instr+1) |= ((((offset >> 21) & 1) ^ S) ^ 1) << 11;
      /* J1 */
      *(instr+1) |= ((((offset >> 22) & 1) ^ S) ^ 1) << 13;
      /* S */
      *instr |= S << 10;
    }
  }
  else if(ARM_INS_OPCODE(ins) == TH_BX_R15)
  {
    *instr = 0x4778;
  }
  else
  {
    *instr = 0x4700;  /* Set the opcode */
    if (ARM_INS_OPCODE(ins) == ARM_BLX)
      *instr |= 1 << 7;
    *instr |= (ARM_INS_REGB(ins)) << 3;
  }
}
/* }}} */

/* ThumbAssembleSWI {{{ */
void ThumbAssembleSWI(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit SWI @I", ins));
	/* Set the opcode */
	switch (ARM_INS_OPCODE(ins))
	{
	  case ARM_SWI:
	    *instr = 0xdf00;
	    break;
	  case ARM_BKPT:
	    *instr = 0xbe00;
	    break;
	  default:
	    FATAL(("Unsupported IT_SWI instruction @I",ins));
	    break;
	}
	*instr |= ARM_INS_IMMEDIATE(ins);
}
/* }}} */

/* ThumbAssemble3Bit {{{ */
void ThumbAssemble3Bit(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit 3-bit @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));

  if (ARM_INS_REGB(ins)==ARM_REG_R13)
    {
      ASSERT(ARM_INS_REGA(ins)!=ARM_REG_R13,("@I assembler problem, case not foreseen yet here",ins));
      *instr = 0xa800;
      *instr |= ((ARM_INS_IMMEDIATE(ins)>>2) & 0xff);
      *instr |= ARM_INS_REGA(ins)<<8;
    }
  else {
    ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));
    *instr = 0x1800; /* Set the opcode */
    if (ARM_INS_FLAGS(ins) & FL_IMMED) {
      *instr |= 0x1 << 10;
      *instr |= (ARM_INS_IMMEDIATE(ins) & 0x7) << 6;
    }
    else
    {
      ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));
      *instr |= (ARM_INS_REGC(ins)) << 6;
    }
    if (ARM_INS_OPCODE(ins) == ARM_SUB)
      *instr |= 0x1 << 9;
    *instr |= (ARM_INS_REGB(ins)) << 3;
    *instr |= ARM_INS_REGA(ins);
  }
}
/* }}} */

/* ThumbAssembleImm {{{ */
void ThumbAssembleImm(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit immediate @I", ins));

  if (ARM_INS_REGB(ins)==ARM_REG_R13 || ARM_INS_REGA(ins)==ARM_REG_R13)
  {
    if (ARM_INS_OPCODE(ins) == ARM_ADD)
    {
      /* SP, immediate */
      if (ARM_INS_REGA(ins)==ARM_REG_R13)
      {
        *instr = 0xb000;
        *instr |= ARM_INS_IMMEDIATE(ins) >> 2;

        ASSERT(!(ARM_INS_IMMEDIATE(ins) & ~0x1fc), ("can only have 7-bit immediate, 4-byte aligned %x", ARM_INS_IMMEDIATE(ins)));
      }
      else
      {
        *instr = 0xa800;
        *instr |= ARM_INS_REGA(ins) << 8;
        *instr |= ARM_INS_IMMEDIATE(ins) >> 2;

        ASSERT(!(ARM_INS_IMMEDIATE(ins) & ~0x3fc), ("can only have 8-bit immediate, 4-byt aligned %x", ARM_INS_IMMEDIATE(ins)));
      }
    }
    else if (ARM_INS_OPCODE(ins) == ARM_SUB)
    {
      ASSERT(ARM_INS_REGA(ins) == ARM_REG_R13, ("REGA should be SP, %d @I", ARM_INS_REGA(ins),ins));

      *instr = 0xb080;
      *instr |= ARM_INS_IMMEDIATE(ins) >> 2;

      ASSERT(!(ARM_INS_IMMEDIATE(ins) & ~0x1fc), ("can only have 7-bit immediate, 4-byte aligned %x", ARM_INS_IMMEDIATE(ins)));
    }
    else
      FATAL(("BOEM @I", ins));
  }
  else
  {
    /* regular immediate */
  	*instr = 0x2000;
  	*instr |= ARM_INS_IMMEDIATE(ins) & 0xff;
  	switch(ARM_INS_OPCODE(ins)) {
  	case ARM_MOV:
      ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  		*instr |= (ARM_INS_REGA(ins)) << 8;
  		break;
  	case ARM_CMP:
      ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));
  		*instr |= (ARM_INS_REGB(ins)) << 8;
  		*instr |= 0x1 << 11;
  		break;
  	case ARM_ADD:
      ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  		*instr |= (ARM_INS_REGA(ins)) << 8;
  		*instr |= 0x2 << 11;
  		break;
  	case ARM_SUB:
      ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));
  		*instr |= (ARM_INS_REGB(ins)) << 8;
  		*instr |= 0x3 << 11;
  		break;
  	default:
  		FATAL(("No thumb instruction"));
  	}
  }
}
/* }}} */

/* ThumbAssembleShifted {{{ */
void ThumbAssembleShifted(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit shifted @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));

	*instr = ARM_INS_SHIFTLENGTH(ins) << 6;
	*instr |= (ARM_INS_REGC(ins)) << 3;
	*instr |= ARM_INS_REGA(ins);
	if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_IMM)
		*instr |= 0x1 << 11;
	if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM)
		*instr |= 0x2 << 11;
}
/* }}} */

/* ThumbAssembleALU {{{ */
void ThumbAssembleALU(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit ALU @I", ins));
  if (ARM_INS_OPCODE(ins)!=ARM_RSB)
    ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));

  if (ARM_INS_OPCODE(ins)==ARM_RSB || ARM_INS_OPCODE(ins)==ARM_TST || ARM_INS_OPCODE(ins)==ARM_CMP ||
      ARM_INS_OPCODE(ins)==ARM_CMN)
    ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));

  if (ARM_INS_OPCODE(ins)!=ARM_TST && ARM_INS_OPCODE(ins)!=ARM_CMP && ARM_INS_OPCODE(ins)!=ARM_CMN)
    ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));

  if (ARM_INS_OPCODE(ins)==ARM_MOV)
    ASSERT(IS_THUMB_REG(ARM_INS_REGS(ins)), ("REGS of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGS(ins)));

	*instr = 0x4000;
	switch(ARM_INS_OPCODE(ins)) {
	case ARM_AND:
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_EOR:
		*instr |= 0x1 << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_MOV:
		if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_REG)
			*instr |= 0x2 << 6;
		if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_REG)
			*instr |= 0x3 << 6;
		if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_REG)
			*instr |= 0x4 << 6;
		if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ROR_REG)
			*instr |= 0x7 << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGS(ins)) << 3;
		break;
	case ARM_ADC:
		*instr |= 0x5 << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_SBC:
		*instr |= 0x6 << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_TST:
		*instr |= 0x8 << 6;
		*instr |= ARM_INS_REGB(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_RSB:
		*instr |= 0x9 << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGB(ins)) << 3;
		break;
	case ARM_CMP:
		*instr |= 0xa << 6;
		*instr |= ARM_INS_REGB(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_CMN:
		*instr |= 0xb << 6;
		*instr |= ARM_INS_REGB(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_ORR:
		*instr |= 0xc << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_MUL:
		*instr |= 0xd << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_BIC:
		*instr |= 0xe << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	case ARM_MVN:
		*instr |= 0xf << 6;
		*instr |= ARM_INS_REGA(ins);
		*instr |= (ARM_INS_REGC(ins)) << 3;
		break;
	default:
		FATAL(("No thumb instruction"));
	}
}
/* }}} */

/* ThumbAssembleHiReg {{{ */
void ThumbAssembleHiReg(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit hi-reg @I", ins));
  *instr = 0;
  if (ARM_INS_OPCODE(ins) == ARM_MOV)
  {
    if (!(ARM_INS_FLAGS(ins) & FL_S))
    {
      *instr = 0x4600;
      *instr |= (ARM_INS_REGA(ins) & 0x8) << 4;
    }

    *instr |= ARM_INS_REGA(ins) & 0x7;
  }
  else {
  	*instr = 0x4400;
  	if (ARM_INS_OPCODE(ins) == ARM_ADD) {
  		*instr |= ((ARM_INS_REGA(ins) & 0x8) << 4);
  		*instr |= ARM_INS_REGA(ins) & 0x7;
  	}
  	if (ARM_INS_OPCODE(ins) == ARM_CMP) {
  		*instr |= ((ARM_INS_REGB(ins) & 0x8) << 4);
  		*instr |= ARM_INS_REGB(ins) & 0x7;
  		*instr |= 0x1 << 8;
  	}
  }

  *instr |= (ARM_INS_REGC(ins)) << 3;
}
/* }}} */

/* ThumbAssembleLoadAddress {{{ */
void ThumbAssembleLoadAddress(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit load-address @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));

	*instr = 0xa000;
	*instr |= (ARM_INS_REGA(ins)) << 8;
	if (ARM_INS_REGB(ins) == ARM_REG_R13)
		*instr |= 0x0800;
	*instr |= (ARM_INS_IMMEDIATE(ins) & 0x3ff) >> 2;
}
/* }}} */

/* ThumbAssembleStack {{{ */
void ThumbAssembleStack(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit stack @I", ins));
	*instr = 0xb000;
	if (ARM_INS_OPCODE(ins) == ARM_SUB)
		*instr |= 0x1 << 7;
	*instr |= (ARM_INS_IMMEDIATE(ins) & 0x1ff) >> 2;
}
/* }}} */

/* ThumbAssembleTransferImm {{{ */
void ThumbAssembleTransferImm(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit immediate transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));

	*instr = 0x6000;
	*instr |= ARM_INS_REGA(ins);
	*instr |= (ARM_INS_REGB(ins)) << 3;
	switch(ARM_INS_OPCODE(ins)) {
	case ARM_LDR:
		*instr |= 0x1 << 11;
	case ARM_STR:
		*instr |= ((ARM_INS_IMMEDIATE(ins) & 0x7f) >> 2) << 6;
		break;
	case ARM_LDRB:
		*instr |= 0x1 << 11;
	case ARM_STRB:
		*instr |= 0x1 << 12;
		*instr |= (ARM_INS_IMMEDIATE(ins) & 0x1f) << 6;
		break;
	default:
		FATAL(("No thumb instruction"));
	}
}
/* }}} */

/* ThumbAssembleTransferHalf {{{ */
void ThumbAssembleTransferHalf(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit half transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));

	*instr = 0x8000;
	*instr |= ARM_INS_REGA(ins);
	*instr |= (ARM_INS_REGB(ins)) << 3;
	if (ARM_INS_OPCODE(ins) == ARM_LDRH)
		*instr |= 0x1 << 11;
	*instr |= ((ARM_INS_IMMEDIATE(ins) & 0x3f) >> 1) << 6;
}
/* }}} */

/* ThumbAssembleTransferSign {{{ */
void ThumbAssembleTransferSign(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit sign transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));

	*instr = 0x5200;
	*instr |= ARM_INS_REGA(ins);
	*instr |= (ARM_INS_REGB(ins)) << 3;
	*instr |= (ARM_INS_REGC(ins)) << 6;
	if (ARM_INS_OPCODE(ins) == ARM_LDRSB)
		*instr |= 0x1 << 10;
	if (ARM_INS_OPCODE(ins) == ARM_LDRH)
		*instr |= 0x2 << 10;
	if (ARM_INS_OPCODE(ins) == ARM_LDRSH)
		*instr |= 0x3 << 10;
}
/* }}} */

/* ThumbAssembleTransferRegOff {{{ */
void ThumbAssembleTransferRegOff(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit register-offset transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));
  ASSERT(IS_THUMB_REG(ARM_INS_REGC(ins)), ("REGC of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGC(ins)));

	*instr = 0x5000;
	*instr |= ARM_INS_REGA(ins);
	*instr |= (ARM_INS_REGB(ins)) << 3;
	*instr |= (ARM_INS_REGC(ins)) << 6;
	if (ARM_INS_OPCODE(ins) == ARM_STRB)
		*instr |= 0x1 << 10;
	if (ARM_INS_OPCODE(ins) == ARM_LDR)
		*instr |= 0x2 << 10;
	if (ARM_INS_OPCODE(ins) == ARM_LDRB)
		*instr |= 0x3 << 10;
}
/* }}} */

/* ThumbAssembleTransferPC {{{ */
void ThumbAssembleTransferPC(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit PC-transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));

	*instr = 0x4800;
	*instr |= (ARM_INS_REGA(ins)) << 8;
	*instr |= (ARM_INS_IMMEDIATE(ins) & 0x3ff) >> 2;
}
/* }}} */

/* ThumbAssembleTransferSP {{{ */
void ThumbAssembleTransferSP(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit SP-transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGA(ins)), ("REGA of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGA(ins)));

	*instr = 0x9000;
	*instr |= (ARM_INS_REGA(ins)) << 8;
	*instr |= (ARM_INS_IMMEDIATE(ins) & 0x3ff) >> 2;
	if (ARM_INS_OPCODE(ins) == ARM_LDR)
		*instr |= 0x1 << 11;
}
/* }}} */

/* ThumbAssembleMultipleTransfer {{{ */
void ThumbAssembleMultipleTransfer(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit multiple-transfer @I", ins));
  ASSERT(IS_THUMB_REG(ARM_INS_REGB(ins)), ("REGB of @I should be a Thumb register (have r%d)!", ins, ARM_INS_REGB(ins)));

	*instr = 0xc000;
	*instr |= (ARM_INS_REGB(ins)) << 8;
	*instr |= ARM_INS_IMMEDIATE(ins) & 0xff;
	if (ARM_INS_OPCODE(ins) == ARM_LDM)
		*instr |= 0x1 << 11;
}
/* }}} */

/* ThumbAssemblePP {{{ */
void ThumbAssemblePP(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit PP @I", ins));
	*instr = 0xb400;
	*instr |= (ARM_INS_IMMEDIATE(ins) & 0x00ff);
	if (ARM_INS_OPCODE(ins) == ARM_LDM) {
		*instr |= 0x1 << 11;
		*instr |= (ARM_INS_IMMEDIATE(ins) & 0x8000) >> 7;
	}
	else
		*instr |= (ARM_INS_IMMEDIATE(ins) & 0x4000) >> 6;
}
/* }}} */
/* ThumbAssembleData {{{ */
void ThumbAssembleData(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit data @I", ins));
	*instr = ARM_INS_IMMEDIATE(ins);
}
/* }}} */
/* ThumbAssembleUnsupported {{{ */
void ThumbAssembleUnsupported(t_arm_ins * ins, t_uint16 * instr)
{
        FATAL(("Unsupported thumb instruction. Ins = @I\n", ins));
}
/* }}} */

void ThumbAssembleHint(t_arm_ins * ins, t_uint16 * instr)
{
  VERBOSE(THUMB_ASSEMBLER_VERBOSITY_LEVEL, ("assembling 16-bit hint @I", ins));
  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_T2NOP:
    *instr = thumb_opcode_table[TH_NOP].opcode;
    break;

  case ARM_T2IT:
    *instr = thumb_opcode_table[TH_IT].opcode;
    *instr |= ARM_INS_IMMEDIATE(ins) & 0xff;
    break;

  default:
    FATAL(("unsupported Thumb hint instruction @I", ins));
  }
}

void Thumb32AssembleSIMD(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 imm=0, dreg=0, len=0;
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;
  int i;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

void Thumb32AssembleSIMDImm(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint64 imm = ARM_INS_IMMEDIATE(ins);
  t_uint32 temp = 0;
  t_uint32 cmode = -1;
  t_uint32 op = 0;
  t_uint64 i;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

  *instr |= (temp & 0x80) << 21;
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

void Thumb32AssembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a=0, u=0, b=0;
  t_uint32 size=0;
  t_arm_ins_dt sizetype = DT_NONE;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));

  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_VMLA_SCALAR:
    case ARM_VMLS_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VML[A|S] (scalar) instruction"));
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x10000000 : 0;
      *instr |= (size << 20);
      *instr |= (sizetype == DT_F_START) ? 0x00000100 : 0;
      break;

    case ARM_VMLAL_SCALAR:
    case ARM_VMLSL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VML[A|S]L (scalar) instruction"));
      *instr |= (size << 20);
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
      break;

    case ARM_VQDMLAL_SCALAR:
    case ARM_VQDMLSL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQDML[A|S]L (scalar) instruction"));
      *instr |= (size << 20);
      break;

    case ARM_VMUL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VMUL (scalar) instruction"));
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x10000000 : 0;
      *instr |= (sizetype == DT_F_START) ? 0x00000100 : 0;
      *instr |= (size << 20);
      break;

    case ARM_VMULL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VMULL (scalar) instruction"));
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
      *instr |= (size << 20);
      break;

    case ARM_VQDMULH_SCALAR:
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x10000000 : 0;
    case ARM_VQDMULL_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQDMUL[L|H] (scalar) instruction"));
      *instr |= (size << 20);
      break;

    case ARM_VQRDMULH_SCALAR:
      ASSERT(size != 0, ("Illegal size-field in VQRDMULH (scalar) instruction"));
      *instr |= (size << 20);
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x10000000 : 0;
      break;

    default:
      FATAL(("Illegal opcode: @I", ins));
  }

  *instr |= ASM_NEON_VM_SCALAR(ARM_INS_REGC(ins), ARM_INS_REGCSCALAR(ins), size+DT_START);
}
void Thumb32AssembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a = 0, b = 0;
  t_uint32 size = 0;
  t_arm_ins_dt sizetype = DT_NONE;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

void Thumb32AssembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a = 0, b = 0, u = 0, c = 0;
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;
  t_uint32 uflag = 0, qflag = 0;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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
  *instr |= (u << 28);
  *instr |= (size << 20);
}

void Thumb32AssembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 a=0, u=0, b=0, l=0, imm=0;
  t_uint32 opc = ARM_INS_OPCODE(ins);

  t_arm_ins_dt sizetype = DT_NONE;
  t_uint32 size=0, imminv=0;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
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
      *instr |= 0x10000000;
    case ARM_VQSHL_IMM:
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
      *instr |= (imm | ARM_INS_IMMEDIATE(ins)) << 16;
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      break;

    case ARM_VQSHRN:
    case ARM_VQRSHRN:
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
    case ARM_VSHRN:
    case ARM_VRSHRN:
    case ARM_VQSHRUN:
    case ARM_VQRSHRUN:
      *instr |= (imm | (imminv - ARM_INS_IMMEDIATE(ins))) << 16;
      break;

    case ARM_VMOVL1:
    case ARM_VMOVL2:
    case ARM_VMOVL3:
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
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
      *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
      *instr |= (imm | ARM_INS_IMMEDIATE(ins)) << 16;
      break;

    case ARM_VCVT_FX:
      *instr |= (ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD) ? 0x00000040 : 0;
      *instr |= (64 - ARM_INS_IMMEDIATE(ins)) << 16;

      if(sizetype == DT_F_START)
      {
        *instr |= (ARM_INS_DATATYPEOP(ins) == DT_U32) ? 0x10000000 : 0;
      }
      else
      {
        /* op=1 */
        *instr |= 0x00000100;
        *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;
      }
      break;

    default:
      FATAL(("Illegal opcode: @I.", ins));
  }

  *instr |= (u << 28);
  *instr |= (a << 8);
  *instr |= (l << 7);
  *instr |= (b << 6);
}

void Thumb32AssembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 opc = ARM_INS_OPCODE(ins);
  t_arm_ins_dt sizetype = DT_NONE, size = DT_NONE;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  /* registers */
  *instr |= ASM_NEON_VD_QD(ARM_INS_REGA(ins));
  *instr |= ASM_NEON_VN_QD(ARM_INS_REGB(ins));
  *instr |= ASM_NEON_VM_QD(ARM_INS_REGC(ins));

  /* size and normalized size type */
  size = ASM_DATATYPE_NORMALIZE(ins, &sizetype);

  *instr |= (sizetype == DT_U_START) ? 0x10000000 : 0;

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
void Thumb32AssembleSIMDTransfer(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint32 l=0, c=0, a=0, b=0;
  t_uint32 op1=0, op2=0, s=0;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

  *instr |= (a << 21);
  *instr |= (l << 20);
  *instr |= (c << 8);
  *instr |= (b << 5);
}

#define ASM_SIMD_LOADSTORE_ALIGN(x) ((x == 64) ? 1 : ((x == 128) ? 2 : ((x == 256) ? 3 : 0)))
//#define ASM_SIMD_LOADSTORE_ALIGN(x) 0
#define CORRECT_ALIGNMENT 1
void Thumb32AssembleSIMDLoadStore(t_arm_ins * ins, t_uint32 * instr)
{
  t_arm_ins_dt size = DT_NONE, sizetype = DT_NONE;
  t_uint32 align = 0, len = 0, totallen = 0, index_align = 0;
  t_reg dreg = 0;

  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

  /* Rm */
  if(ARM_INS_FLAGS(ins) & FL_WRITEBACK)
  {
    *instr |= 13;
  }
  else if(ARM_INS_REGC(ins) == ARM_REG_NONE)
  {
    *instr |= 15;
  }
  else
  {
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

void Thumb32AssembleFP2R(t_arm_ins * ins, t_uint32 * instr)
{
  /* set opcode */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

void
Thumb32AssembleVFPDP(t_arm_ins * ins, t_uint32 * instr)
{
  t_uint64 immed=0;
  t_uint32 imm=0;
  t_arm_ins_dt size=DT_NONE, sizetype=DT_NONE;

  if (ARM_INS_TYPE(ins) != IT_FLT_ALU)
    FATAL(("Wrong assembly function (FLT_ALU) for instruction %s", arm_opcode_table[ARM_INS_OPCODE(ins)].desc));

  /* opcode and condition code */
  *instr = thumb_opcode_table[arm2thumb32_opcode_table[ARM_INS_OPCODE(ins)]].opcode;

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

    default:
      FATAL(("Unsupported instruction for VFP data processing: @I",ins));
  }
}
/* vim: set shiftwidth=2 foldmethod=marker : */
