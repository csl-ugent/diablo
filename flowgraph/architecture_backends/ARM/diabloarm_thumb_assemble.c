/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

/*!
 *
 * Assemble a Thumb-2 instruction (t_thumb_ins to raw thumb instruction)
 *
 * \param i_ins The instruction to assemble
 *
 * \param data A buffer (must be at least 4 bytes long in which the instruction
 * is assembled
 *
 * \return void
*/
/* ThumbAssembleT2 {{{ */
void ThumbAssembleT2(t_arm_ins * i_ins, char * data)
{
  switch (ARM_INS_OPCODE(i_ins))
  {
    case ARM_T2NOP:
    case ARM_T2YIELD:
    case ARM_T2WFE:
    case ARM_T2WFI:
    case ARM_T2SEV:
      ThumbAssembleV6V7ITHints(i_ins, (t_uint16*)data);
      break;
    default:
      FATAL(("Unsupported Thumb-2 instruction @I",i_ins));
      break;
  }
}
/* }}} */

void ThumbAssemble32(t_arm_ins * ins, char * data)
{
  switch(ARM_INS_TYPE(ins))
  {
  case IT_DATAPROC:
    if (ARM_INS_FLAGS(ins) & (FL_IMMED|FL_IMMEDW))
      Thumb32AssembleDataprocImmediate(ins, (t_uint32 *)data);
    else
      Thumb32AssembleDataprocRegister(ins, (t_uint32 *)data);
    break;

  case IT_LOAD:
  case IT_STORE:
    if ((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_LDRSTREX_LAST))
      Thumb32AssembleLoadStoreExclusive(ins, (t_uint32 *)data);
    else
    {
      Thumb32AssembleLoadStore(ins, (t_uint32 *)data);
    }
    break;

  case IT_LOAD_MULTIPLE:
  case IT_STORE_MULTIPLE:
    if ((ARM_INS_OPCODE(ins) == ARM_VLDM) ||
        (ARM_INS_OPCODE(ins) == ARM_VSTM) ||
        (ARM_INS_OPCODE(ins) == ARM_VPUSH) ||
        (ARM_INS_OPCODE(ins) == ARM_VPOP))
      Thumb32AssembleVLoadStore(ins, (t_uint32 *)data);
    else if( ((ARM_INS_OPCODE(ins) >= ARM_SIMD_FIRSTSTORE) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTSTORE)) ||
            ((ARM_INS_OPCODE(ins) >= ARM_SIMD_FIRSTLOAD) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTLOAD)) )
      Thumb32AssembleSIMDLoadStore(ins, (t_uint32 *)data);
    else
      Thumb32AssembleLoadStoreMultiple(ins, (t_uint32 *)data);
    break;

  case IT_BRANCH:
    Thumb32AssembleBranch(ins, (t_uint32 *)data);
    break;

  case IT_NOP:
  case IT_SYNC:
    Thumb32AssembleHint(ins, (t_uint32 *)data);
    break;

  case IT_STATUS:
    Thumb32AssembleStatus(ins, (t_uint32 *)data);
    break;

  case IT_MUL:
  case IT_DIV:
    Thumb32AssembleMultiply(ins, (t_uint32 *)data);
    break;

  case IT_UNKNOWN:
    if ((ARM_INS_OPCODE(ins) == ARM_PLD) ||
        (ARM_INS_OPCODE(ins) == ARM_PLDW) ||
        (ARM_INS_OPCODE(ins) == ARM_PLI))
      Thumb32AssembleLoadStore(ins, (t_uint32 *)data);
    else
      Thumb32AssembleCoproc(ins, (t_uint32 *)data);
    break;

  case IT_FLT_LOAD:
  case IT_FLT_STORE:
    Thumb32AssembleVLoadStore(ins, (t_uint32 *)data);
    break;

  case IT_SIMD:
    if((ARM_INS_OPCODE(ins) >= ARM_SIMD_FIRST3SAME) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LAST3SAME))
    {
      Thumb32AssembleSIMD3RegsSameLength(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMDIMM_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMDIMM_LAST))
    {
      Thumb32AssembleSIMDImm(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMDVAR_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMDVAR_LAST))
    {
      Thumb32AssembleSIMD(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMD2MISC_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMD2MISC_LAST))
    {
      Thumb32AssembleSIMD2RegsMisc(ins, (t_uint32 *) data);
    }
    else if(ARM_INS_OPCODE(ins) == ARM_VDUP)
    {
      Thumb32AssembleSIMD(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMDRT_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMDRT_LAST))
    {
      Thumb32AssembleSIMDTransfer(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMD2REGSSHIFT_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMD2REGSSHIFT_LAST))
    {
      Thumb32AssembleSIMD2RegsShift(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMD2REGSSCALAR_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMD2REGSSCALAR_LAST))
    {
      Thumb32AssembleSIMD2RegsScalar(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) >= ARM_SIMD3REGSDIFFLENGTH_FIRST) && (ARM_INS_OPCODE(ins) <= ARM_SIMD3REGSDIFFLENGTH_LAST))
    {
      Thumb32AssembleSIMD3RegsDifferentLength(ins, (t_uint32 *) data);
    }
    else if((ARM_INS_OPCODE(ins) == ARM_VMOV64_C2S) || (ARM_INS_OPCODE(ins) == ARM_VMOV64_C2D))
    {
      Thumb32AssembleFP2R(ins, (t_uint32 *) data);
    }
    else
    {
      FATAL(("Attempt to assemble SIMD instruction: @I", ins));
    }
    break;

  case IT_FLT_ALU:
    if ((ARM_FP_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_FP_LAST))
      Thumb32AssembleVFPDP(ins, (t_uint32 *) data);
    else
      FATAL(("Unsupported IT_FLT_ALU instruction @I", ins));
    break;

  default:
    FATAL(("Unsupported 32-bit Thumb instruction type (@I)", ins));
  }
}

t_bool ArmInsIs32BitThumb1(t_arm_ins * ins)
{
  switch (ARM_INS_OPCODE(ins))
  {
  case ARM_BLX:
  case ARM_BL:
    return (ARM_INS_FLAGS(ins) & FL_IMMED);
    break;

  default:
    break;
  }

  return FALSE;
}

/*!
 *
 * Assemble a Thumb instruction (t_thumb_ins to raw thumb instruction)
 *
 * \param i_ins The instruction to assemble
 *
 * \param data A buffer (must be at least 4 bytes long in which the instruction
 * is assembled
 *
 * \return void
*/
/* ThumbAssembleOne {{{ */
void ThumbAssembleOne(t_arm_ins * i_ins, char * data)
{
  if (ARM_INS_CSIZE(i_ins) == 4)
  {
    /* this instruction cannot be encoded in a Thumb-1 instruction,
     *  try encoding it in a Thumb-2 instruction if this is allowed
     *  if this also can't be done, there are no options left...
     */
    ASSERT(ArmIsThumb2Encodable(i_ins), ("Thumb instruction not encodable in 4 bytes @I", i_ins));
    ASSERT(diabloarm_options.fullthumb2 || ArmInsIs32BitThumb1(i_ins), ("Full thumb2 support is not enabled. Can not encode 4-byte Thumb instructions (add '--fullthumb2' to Diablo's commandline parameters) @I", i_ins));
    return ThumbAssemble32(i_ins, data);
  }

  if (!ArmIsThumb1EncodableCheckItNoIt(i_ins, TRUE, TRUE))
    FATAL(("Instruction can't be encoded in Thumb-1 @I%s", i_ins, (ArmIsThumb2Encodable(i_ins)) ? ", but it can be encoded in Thumb-2!" : ""));

  /* encode this instruction as Thumb-1
   */
    switch (ARM_INS_TYPE(i_ins)) {
    case IT_DATA:
	ThumbAssembleData(i_ins, (t_uint16*)data);
	break;
    case IT_BRANCH:
	if ((ARM_INS_CONDITION(i_ins) != ARM_CONDITION_AL) && !ArmInsIsInITBlock(i_ins))
	  ThumbAssembleCondBranch(i_ins, (t_uint16*)data);
	else
	{
	  if (((ARM_INS_OPCODE(i_ins) == ARM_BL) ||
	       (ARM_INS_OPCODE(i_ins) == ARM_BLX)) &&
	       (ARM_INS_FLAGS(i_ins) & FL_IMMED))
	    ThumbAssembleBranchLink(i_ins, (t_uint32*)data);
	  else
	    ThumbAssembleBranch(i_ins, (t_uint16*)data);
	}
	break;
    case IT_STATUS:
        switch (ARM_INS_OPCODE(i_ins))
        {
          case ARM_MSR:
          case ARM_MRS:
            ThumbAssembleV6V7Status(i_ins, (t_uint32*)data);
            break;
          default:
            FATAL(("Unsupported thumb status instruction @i",i_ins));
        }
        break;
    case IT_DATAPROC:
	if ((ARM_INS_OPCODE(i_ins) >= ARM_T2_FIRST) &&
	    (ARM_INS_OPCODE(i_ins) <= ARM_T2_LAST))
	    ThumbAssembleT2(i_ins, data);
	else if ((ARM_INS_OPCODE(i_ins) >= ARM_V6_FIRST) &&
		 (ARM_INS_OPCODE(i_ins) <= ARM_V6_LAST))
	    ThumbAssembleV6Extract(i_ins, (t_uint16*)data);
	else if (ARM_INS_SHIFTTYPE(i_ins) != ARM_SHIFT_TYPE_NONE) {
	    if (ARM_INS_REGS(i_ins) != ARM_REG_NONE)
		ThumbAssembleALU(i_ins, (t_uint16*)data);
	    else
		ThumbAssembleShifted(i_ins, (t_uint16*)data);
	}
	else {
      t_bool does_not_set_flags = ArmInsThumbITDoesNotSetFlags(i_ins, TRUE);

	    switch (ARM_INS_OPCODE(i_ins)) {
	    case ARM_AND:
	    case ARM_EOR:
	    case ARM_ADC:
	    case ARM_SBC:
	    case ARM_ORR:
	    case ARM_BIC:
	    case ARM_TST:
	    case ARM_CMN:
	    case ARM_RSB:
	    case ARM_MVN:
		ThumbAssembleALU(i_ins, (t_uint16*)data);
		break;
	    case ARM_ADD:
	    case ARM_SUB:
		if (ARM_INS_FLAGS(i_ins) & FL_IMMED) {
      /* immediate */
                    if (ARM_INS_REGA(i_ins)==ARM_REG_R13 || ARM_INS_REGB(i_ins)==ARM_REG_R13)
                        ThumbAssembleImm(i_ins, (t_uint16*)data);

                    else if ((ARM_INS_FLAGS(i_ins) & FL_S) || does_not_set_flags) {
                        if(ARM_INS_REGA(i_ins) == ARM_INS_REGB(i_ins))
                            ThumbAssembleImm(i_ins, (t_uint16*)data);
                        else
                            ThumbAssemble3Bit(i_ins, (t_uint16*)data);
                    }
                    else {
			    ThumbAssembleLoadAddress(i_ins, (t_uint16*)data);
		    }
		}
		else
    {
      /* register */
      if ((ARM_INS_REGA(i_ins) <= ARM_REG_R7) && (ARM_INS_REGB(i_ins) <= ARM_REG_R7) && (ARM_INS_REGC(i_ins) <= ARM_REG_R7))
      {
        if ((ARM_INS_FLAGS(i_ins) & FL_S) || does_not_set_flags)
          ThumbAssemble3Bit(i_ins, (t_uint16 *)data);
        else
          ThumbAssembleHiReg(i_ins, (t_uint16 *)data);
      }
      else
        ThumbAssembleHiReg(i_ins, (t_uint16 *)data);
		}
		break;
	    case ARM_MOV:
	    case ARM_CMP:
		if (ARM_INS_FLAGS(i_ins) & FL_IMMED)
		    ThumbAssembleImm(i_ins, (t_uint16*)data);
		else {
		    if (ARM_INS_REGB(i_ins) < 0x8 && ARM_INS_REGC(i_ins) < 0x8)
			ThumbAssembleALU(i_ins, (t_uint16*)data);
		    else
			ThumbAssembleHiReg(i_ins, (t_uint16*)data);
		}
		break;
	    default:
		FATAL(("No thumb instruction @I", i_ins));
	    }
	}
	break;
    case IT_LOAD:
    case IT_STORE:
	if (ARM_INS_REGB(i_ins) == ARM_REG_R13)
	    ThumbAssembleTransferSP(i_ins, (t_uint16 *) data);
	else
	{
	  if (ARM_INS_REGB(i_ins) == ARM_REG_R15)
	    ThumbAssembleTransferPC(i_ins, (t_uint16 *) data);
	  else
	  {
	    if (ARM_INS_FLAGS(i_ins) & FL_IMMED)
	    {
	      if (ARM_INS_OPCODE(i_ins) == ARM_STRH || ARM_INS_OPCODE(i_ins) == ARM_LDRH)
		ThumbAssembleTransferHalf(i_ins, (t_uint16 *) data);
	      else
		ThumbAssembleTransferImm(i_ins, (t_uint16 *) data);
	    }
	    else
	    {
	      if (ARM_INS_OPCODE(i_ins) == ARM_STR || ARM_INS_OPCODE(i_ins) == ARM_LDR || ARM_INS_OPCODE(i_ins) == ARM_STRB || ARM_INS_OPCODE(i_ins) == ARM_LDRB)
		ThumbAssembleTransferRegOff(i_ins, (t_uint16 *) data);
	      else
		ThumbAssembleTransferSign(i_ins, (t_uint16 *) data);
	    }
	  }
	}
	break;
    case IT_LOAD_MULTIPLE:
    case IT_STORE_MULTIPLE:
	if (ARM_INS_REGB(i_ins) == ARM_REG_R13)
	    ThumbAssemblePP(i_ins, (t_uint16 *) data);
	else
	    ThumbAssembleMultipleTransfer(i_ins, (t_uint16 *) data);
	break;
    case IT_SWI:
	ThumbAssembleSWI(i_ins, (t_uint16 *) data);
	break;
    case IT_MUL:
    case IT_DIV:
	ThumbAssembleALU(i_ins, (t_uint16 *) data);
	break;
    case IT_NOP:
    case IT_SYNC:
      ThumbAssembleHint(i_ins, (t_uint16 *) data);
      break;

    default:
    	ThumbAssembleUnsupported(i_ins, (t_uint16 *) data);
    }
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker : */
