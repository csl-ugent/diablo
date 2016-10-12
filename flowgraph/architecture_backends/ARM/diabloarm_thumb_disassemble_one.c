/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

/* Initialises an instruction object in which a Thumb instruction will be stored.
 * All fields of the instruction object will be set to their default value.
 * Additionally, the given Thumb opcode is translated to an ARM opcode, as Diablo
 * internally works on ARM instructions.
 *
 * \param ins The instruction object.
 * \param topc The Thumb opcode for this instruction.
 */
static void ThumbInitInstruction(t_arm_ins * ins, t_thumb_opcode topc)
{
	ARM_INS_SET_OPCODE(ins, thumb_opcode_table[topc].arm_opcode);
	ARM_INS_SET_THUMBOPCODE(ins, topc);

	ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
	ARM_INS_SET_ATTRIB(ins, 0);
	ARM_INS_SET_FLAGS(ins, 0);

	ARM_INS_SET_REGA(ins, ARM_REG_NONE);
	ARM_INS_SET_REGB(ins, ARM_REG_NONE);
	ARM_INS_SET_REGC(ins, ARM_REG_NONE);
	ARM_INS_SET_REGS(ins, ARM_REG_NONE);

	ARM_INS_SET_IMMEDIATE(ins, 0);

	ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
	ARM_INS_SET_SHIFTLENGTH(ins, 0);

	ARM_INS_SET_TYPE(ins, IT_UNKNOWN);
}

/* Extracts a modified immediate from a 32-bit Thumb instruction, according to
 * Section A6.3.2 of the ARM ARM (ARM-DDI-0406c.b). The extracted immediate is
 * stored to the second parameter.
 *
 * \param instr A 32-bit Thumb instruction (modified immediate)
 * \param immed A pointer to a 32-bit integer in which the extracted immediate will be saved.
 * \return TRUE if the carry flag is to be modified by this instruction (see ARM-DDI-0406c.b/A6-232, "Carry out").
 */
t_bool Thumb32DecodeImm(t_uint32 instr, t_uint32 * immed)
{
	t_uint32 imm = ((instr & 0x04000000) >> 22) | ((instr & 0x00007000) >> 11) | ((instr & 0x00000080) >> 7);
	t_uint32 cst = 0;

	if (imm < 8)
	{
		cst = instr & 0x000000ff;

		if (imm > 1)
			cst |= (cst << 16);
		if (imm > 3)
			cst <<= 8;
		if (imm > 5)
			cst |= (cst >> 8);
	}
	else
	{
		cst = ((instr & 0x000000ff) << 24) | 0x80000000;
		cst >>= imm-8;
	}

	*immed = cst;

	return (imm & 0x18) != 0;
}

/* Decode a shift-by-immediate value from a 32-bit Thumb instruction, and set
 * the approprate fields in the given instruction object to the correct values.
 * If the shift amount is specified to be zero, special cases are handled, based
 * on the shift type.
 *	- LSL (immediate) -> no shift specified
 *	- ROR (immediate) -> assume RRX
 *	- LSR/ASR (immediate) -> assume a 32-bit shift
 *
 * \param ins The instruction object.
 * \param shifttype The shift type extracted from the encoded instruction, as defined in diabloarm_instruction.h (ARM_SHIFT_TYPE_*).
 * \param shiftlength The shift amount.
 */
void Thumb32ShiftDecodeImm(t_arm_ins * ins, int shifttype, int shiftlength)
{
	ARM_INS_SET_REGS(ins, ARM_REG_NONE);
	ARM_INS_SET_SHIFTTYPE(ins, shifttype);
	ARM_INS_SET_SHIFTLENGTH(ins, shiftlength);

	if (ARM_INS_SHIFTLENGTH(ins) == 0)
	{
		/* special case */
		if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM)
			ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

		else if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ROR_IMM)
			ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_RRX);

		else if (	(ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_IMM) ||
							(ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM))
			ARM_INS_SET_SHIFTLENGTH(ins, 32);
	}
}

/* A generic handler to handle the non-implemented instructions.
 * This handler just fatals telling the user a non-implemented instruction is found.
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
}

/* Disassembles a 32-bit Thumb hint instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are:
 *	NOP, DBG, DSB, DMB, ISB
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleHint(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling hint instruction"));

	ThumbInitInstruction(ins, opc);

	if ((opc != TH32_DBG) &&
			(opc != TH32_DSB) &&
			(opc != TH32_DMB) &&
			(opc != TH32_ISB))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	ARM_INS_SET_TYPE(ins, IT_SYNC);

	switch(opc)
	{
		case TH32_NOP:
			ARM_INS_SET_TYPE(ins, IT_NOP);
			break;

		case TH32_DBG:
		case TH32_DSB:
		case TH32_DMB:
		case TH32_ISB:
			ARM_INS_SET_IMMEDIATE(ins, instr & 0x0000000f);
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}

/* Disassembles a 32-bit Thumb dataprocessing instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are (ARM DDI-0406c.b):
 *	- Data-processing (register), except for the shift instructions (Section A6.3.12)
 *	- Parallel addition and subtraction, signed (Section A6.3.13)
 *	- Parallel addition and subtraction, unsigned (Section A6.3.14)
 *	- Miscellaneous operations (Section A6.3.15)
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleDataproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling dataprocessing instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	if ((opc == TH32_SXTH) ||
			(opc == TH32_UXTH) ||
			(opc == TH32_SXTB) ||
			(opc == TH32_UXTB) ||
			(opc == TH32_REV) ||
			(opc == TH32_REV16) ||
			(opc == TH32_REVSH))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);

	if ((opc == TH32_SXTH) ||
			(opc == TH32_SXTB) ||
			(opc == TH32_SXTB16) ||
			(opc == TH32_UXTH) ||
			(opc == TH32_UXTB) ||
			(opc == TH32_UXTB16))
	{
		ARM_INS_SET_REGC(ins, (instr & 0x0000000f));

		ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_ROR_IMM);
		ARM_INS_SET_SHIFTLENGTH(ins, (instr & 0x00000030) >> 1);
	}
	else if (opc == TH32_RBIT)
	{
		ARM_INS_SET_REGB(ins, instr & 0x0000000f);
	}
	else if ((opc == TH32_CLZ) ||
						(opc == TH32_REV) ||
						(opc == TH32_REV16) ||
						(opc == TH32_REVSH))
	{
		ARM_INS_SET_REGC(ins, instr & 0x0000000f);
	}
	else
	{
		ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
		ARM_INS_SET_REGC(ins,  instr & 0x0000000f);
	}

	if ((opc == TH32_SXTAB) ||
			(opc == TH32_SXTAB16) ||
			(opc == TH32_SXTAH) ||
			(opc == TH32_UXTAB) ||
			(opc == TH32_UXTAB16) ||
			(opc == TH32_UXTAH))
	{
		ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_ROR_IMM);
		ARM_INS_SET_SHIFTLENGTH(ins, (instr & 0x00000030) >> 1);
	}

	if (opc == TH32_USADA8)
	{
		ARM_INS_SET_REGS(ins, (instr & 0x0000f000) >> 12);
	}
}

/* Disassembles a 32-bit Thumb saturation instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are (ARM DDI-0406c.b):
 *	SSAT(16), USAT(16)
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleDataprocSaturating(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling saturation instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
	ARM_INS_SET_REGC(ins, (instr & 0x000f0000) >> 16);

	if ((opc == TH32_SSAT) ||
			(opc == TH32_USAT))
	{
		Thumb32ShiftDecodeImm(ins, (instr & (1<<(16+5))) >> (16+4), ((instr & 0x00007000) >> 10) | ((instr & 0x000000c0) >> 6));
		ARM_INS_SET_IMMEDIATE(ins, (instr & 0x0000001f) + 1);
	}
	else
	{
		ARM_INS_SET_IMMEDIATE(ins, (instr & 0x0000000f) + 1);
	}
}

/* Disassembles a 32-bit Thumb saturation instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are (ARM DDI-0406c.b):
 *	- Data-processing (modified immediate) (Section A6.3.1)
 *	- Data-processing (plain binary immediate) (Section A6.3.3), except for:
 *		SSAT(16) and USAT(16), as these are handled as saturation instructions.
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleImmediate(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling dataprocessing (immediate) instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	if ((opc == TH32_LSL_IMM) ||
			(opc == TH32_LSR_IMM) ||
			(opc == TH32_ASR_IMM) ||
			(opc == TH32_CMP_IMM) ||
			(opc == TH32_MOV_IMM) ||
			(opc == TH32_ADR) ||
			(opc == TH32_ADD_IMM) ||
			(opc == TH32_RSB_IMM) ||
			(opc == TH32_SUB_IMM) ||
			(opc == TH32_CMN_IMM))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	t_bool affects_carry = FALSE;
	t_uint32 immed = 0;

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	switch(opc)
	{
		case TH32_LSL_IMM:
		case TH32_LSR_IMM:
		case TH32_ASR_IMM:
		case TH32_ROR_IMM:
			ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
			ARM_INS_SET_REGC(ins, instr & 0x0000000f);

			Thumb32ShiftDecodeImm(ins, (instr & 0x00000030) >> 4, (((instr & 0x00007000) >> 10) | ((instr & 0x000000c0) >> 6)));

			if (instr & 0x00100000)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);

			break;

		case TH32_ADR:
			if (instr & 0x00a00000)
				ARM_INS_SET_OPCODE(ins, ARM_SUB);
			else
				ARM_INS_SET_OPCODE(ins, ARM_ADD);

			ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
			ARM_INS_SET_REGB(ins, ARM_REG_R15);

			ARM_INS_SET_IMMEDIATE(ins, ((instr & 0x04000000) >> 15) | ((instr & 0x00007000) >> 4) | (instr & 0x000000ff));
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
			break;

		case TH32_CMP_IMM:
		case TH32_ADD_IMM:
		case TH32_RSB_IMM:
		case TH32_SUB_IMM:
		case TH32_TST_IMM:
		case TH32_TEQ_IMM:
		case TH32_CMN_IMM:
		case TH32_AND_IMM:
		case TH32_ADDW:
		case TH32_BIC_IMM:
		case TH32_ORR_IMM:
		case TH32_ORN_IMM:
		case TH32_EOR_IMM:
		case TH32_ADC_IMM:
		case TH32_SBC_IMM:
		case TH32_SUBW:
			ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
			/* not all instructions have a destination register (e.g. compare instructions) */
			if ((opc == TH32_CMP_IMM) ||
					(opc == TH32_CMN_IMM) ||
					(opc == TH32_TST_IMM) ||
					(opc == TH32_TEQ_IMM))
			{
				ARM_INS_SET_REGA(ins, ARM_REG_NONE);
			}

			ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

			if (instr & 1<<20)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);

			affects_carry = Thumb32DecodeImm(instr, &immed);
			/* not all instructions affect the carry flag by default */
			/*if ((opc == TH32_EOR_IMM) ||
					(opc == TH32_ORN_IMM) ||
					(opc == TH32_ORR_IMM) ||
					(opc == TH32_BIC_IMM) ||
					(opc == TH32_AND_IMM) ||
					(opc == TH32_TST_IMM) ||
					(opc == TH32_TEQ_IMM))
			{
				if (affects_carry)
					ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
			}*/

			/* special case for the ADDW immediate instruction: constant is encoded literally */
			if ((opc == TH32_ADDW) || (opc == TH32_SUBW))
				immed = ((instr & 0x04000000) >> 15) | ((instr & 0x00007000) >> 4) | (instr & 0x000000ff);

			ARM_INS_SET_IMMEDIATE(ins, immed);
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
			break;

		case TH32_MVN_IMM:
		case TH32_MOV_IMM:
		case TH32_MOVT:
		case TH32_MOVW:
			ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);

			/* for MOVW and MOVT, the immediate value is encoded literally */
			if ((opc == TH32_MOVW) || (opc == TH32_MOVT))
			{
				ARM_INS_SET_IMMEDIATE(ins,
						((instr & 0x000f0000) >> 4) | ((instr & 0x04000000) >> 15) |
						((instr & 0x00007000) >> 4) | (instr & 0x000000ff));
			}
			else
			{
				affects_carry = Thumb32DecodeImm(instr, &immed);
				/*if (affects_carry)
					ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);*/

				ARM_INS_SET_IMMEDIATE(ins, immed);
			}

                        if (opc == TH32_MOVW)
                          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMEDW);
                        else
                          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

			if (instr & 1<<(16+4))
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}

        ARM_INS_SET_FLAGS(ins,ARM_INS_FLAGS(ins)|FL_THUMB);

}

/* Disassembles a 32-bit Thumb bitfield instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are (ARM DDI-0406c.b):
 *	RBIT, BFI, BFC, SBFX, UBFX
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleBits(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling bitfield instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);

	switch(opc)
	{
		case TH32_RBIT:
			ARM_INS_SET_REGB(ins, instr & 0x0000000f);
			break;

		case TH32_SBFX:
		case TH32_UBFX:
		case TH32_BFI:
			ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

		case TH32_BFC:
			{
				t_uint32 lsb = ((instr & 0x00007000) >> 10) | ((instr & 0x000000c0) >> 6);
				/* match the encoding used by the disassembler for regular ARM instructions */
				ARM_INS_SET_IMMEDIATE(ins, ((instr & 0x0000001f) << 16) | lsb);
			}
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}

/* Disassembles a 32-bit Thumb control (status) instruction and stores the results in
 * the given instruction object.
 * Instructions covered by this handler are (ARM DDI-0406c.b):
 *	MRS, MSR
 *
 * \param ins The instruction object.
 * \param instr The encoded instruction.
 * \param opc The detected opcode.
 */
void Thumb32DisassembleControl(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling control instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_STATUS);

	switch(opc)
	{
		case TH32_MRS:
			ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_SPSR);
			break;

		case TH32_MSR:
			ARM_INS_SET_REGC(ins, (instr & 0x000f0000) >> 16);

			if (instr & 1<<10)
				FATAL(("sets status bits"));

			if (instr & 1<<11)
      {
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_STATUS);
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
      }
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}

void Thumb32DisassembleBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling branch instruction"));

	ThumbInitInstruction(ins, opc);

	if (opc == TH32_B)
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	t_uint32 tgt = 0;

	ARM_INS_SET_TYPE(ins, IT_BRANCH);

	/* the following assignment to tgt only applies to BL(X) instructions,
	 * for a B instruction, tgt will be assigned a new value
	 */

	/* I1, I2 and S are common for the BL(X) instructions */
	if (! (((instr & 1<<13) >> 13) ^ ((instr & 1<<26) >> 26)))
		tgt |= 1<<22;
	if (! (((instr & 1<<11) >> 11) ^ ((instr & 1<<26) >> 26)))
		tgt |= 1<<21;
	if (instr & 1<<26)
		tgt |= 1<<23;

	switch(opc)
	{
		case TH32_B:
			if (instr & 1<<12)
			{
				/* put the immediate in tgt */
				tgt |= ((instr & 0x03ff0000) >> 5) | (instr & 0x000007ff);

				ARM_INS_SET_IMMEDIATE(ins, 2 * Uint64SignExtend(tgt, 23));
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
			}
			else
			{
				/* condition */
				ARM_INS_SET_CONDITION(ins, (instr & 0x03c00000) >> (16+6));
				if (ARM_INS_CONDITION(ins) != ARM_CONDITION_AL)
					ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)| IF_CONDITIONAL);

				/* immediate value */
				tgt = ((instr & 0x003f0000) >> 5) | (instr & 0x000007ff);
				/* J1, J2, S */
				tgt |= (instr & 1<<13) << 4;
				tgt |= (instr & 1<<11) << 7;
				tgt |= (instr & 1<<26) >> 7;

				ARM_INS_SET_IMMEDIATE(ins, 2 * Uint64SignExtend(tgt, 19));
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
			}

			break;

		case TH32_BL:
			/* calculate target address */
			tgt |= ((instr & 0x03ff0000) >> 5) | ((instr & 0x000007ff));

			ARM_INS_SET_IMMEDIATE(ins, 2 * Uint64SignExtend(tgt, 23));
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

			break;

		case TH32_BLX:
			/* calculate target address */
			tgt |= ((instr & 0x03ff0000) >> 5) | ((instr & 0x000007fe));

			ARM_INS_SET_IMMEDIATE(ins, 2 * Uint64SignExtend(tgt, 23));
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

			break;

		case TH32_TBB:
		case TH32_TBH:
			if(opc == TH32_TBH)
			{
				ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_IMM);
				ARM_INS_SET_SHIFTLENGTH(ins, 1);
			}

			ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
			ARM_INS_SET_REGC(ins, (instr & 0x0000000f));

			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);

			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}

void Thumb32DisassembleLoadStoreMultiple(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling load/store multiple instruction"));

	ThumbInitInstruction(ins, opc);

	if ((opc != TH32_STMDB) &&
			(opc != TH32_LDMDB))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	switch(opc)
	{
		case TH32_PUSHONE:
			/* PUSH Rx
			 *		equals
			 * STR Rx, [SP, #-4]!
			 */
			ARM_INS_SET_TYPE(ins, IT_STORE);

			ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
			ARM_INS_SET_REGB(ins, ARM_REG_R13);
			ARM_INS_SET_IMMEDIATE(ins, 4);

			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| (FL_WRITEBACK | FL_PREINDEX | FL_IMMED));
			break;

		case TH32_POPONE:
			/* POP Rx
			 *		equals
			 * LDR Rx, [SP], #4
			 */
			ARM_INS_SET_TYPE(ins, IT_LOAD);

			ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
			ARM_INS_SET_REGB(ins, ARM_REG_R13);
			ARM_INS_SET_IMMEDIATE(ins, 4);

			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| (FL_WRITEBACK | FL_DIRUP | FL_IMMED));
			break;

		case TH32_PUSH:
		case TH32_POP:
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

		case TH32_STM:
		case TH32_STMDB:
		case TH32_LDM:
		case TH32_LDMDB:
			switch(opc)
			{
				case TH32_STM:
				case TH32_STMDB:
				case TH32_PUSH:
					ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);
					break;

				case TH32_LDM:
				case TH32_LDMDB:
				case TH32_POP:
					ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);
					break;
			}

			ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
			ARM_INS_SET_IMMEDIATE(ins, instr & 0x0000ffff);

			if (opc == TH32_STMDB)
				ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins) & ~0x0000a000);
			if (opc == TH32_LDMDB)
				ARM_INS_SET_IMMEDIATE(ins, ARM_INS_IMMEDIATE(ins) & ~0x00002000);

			if (instr & 1<<24)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
			if (instr & 1<<23)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
			if (instr & 1<<21)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}

void Thumb32DisassembleLoadStoreExclusive(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling load/store exclusive instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);

	if ((opc == TH32_LDREX) ||
			(opc == TH32_LDREXB) ||
			(opc == TH32_LDREXH) ||
			(opc == TH32_LDREXD))
	{
		/* load exclusive instructions */
		ARM_INS_SET_TYPE(ins, IT_LOAD);
	}
	else
	{
		/* store exclusive instructions */
		ARM_INS_SET_TYPE(ins, IT_STORE);

		/* the return value */
		if (opc == TH32_STREX)
			ARM_INS_SET_REGC(ins, (instr & 0x00000f00) >> 8);
		else
			ARM_INS_SET_REGC(ins, instr & 0x0000000f);
	}

	ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
	if ((opc == TH32_LDREXD) || (opc == TH32_STREXD))
		ARM_INS_SET_REGABIS(ins, (instr & 0x00000f00) >> 8);

	ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

	if ((opc == TH32_LDREX) || (opc == TH32_STREX))
	{
		ARM_INS_SET_IMMEDIATE(ins, (instr & 0x000000ff) << 2);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
	}
}

void Thumb32DisassembleLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling load/store instruction"));

	ThumbInitInstruction(ins, opc);

	if ((opc == TH32_STRB) ||
			(opc == TH32_STRH) ||
			(opc == TH32_STR) ||
			(opc == TH32_LDRB) ||
			(opc == TH32_LDRH) ||
			(opc == TH32_LDR))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	switch(opc)
	{
		case TH32_STR:
		case TH32_STRB:
		case TH32_STRH:
		case TH32_STRD:
			ARM_INS_SET_TYPE(ins, IT_STORE);
			break;

		case TH32_LDR:
		case TH32_LDRB:
		case TH32_LDRH:
		case TH32_LDRD:
		case TH32_LDRSB:
		case TH32_LDRSH:
			ARM_INS_SET_TYPE(ins, IT_LOAD);
			break;

		case TH32_PLD:
		case TH32_PLDW:
		case TH32_PLI:
			ARM_INS_SET_TYPE(ins, IT_UNKNOWN);
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}

	ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
	ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);

	if ((opc == TH32_STRD) || (opc == TH32_LDRD))
	{
		ARM_INS_SET_REGABIS(ins, (instr & 0x00000f00) >> 8);

		ARM_INS_SET_IMMEDIATE(ins, (instr & 0x000000ff) << 2);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

		if (instr & 1<<24)
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
		if (instr & 1<<23)
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
		if (instr & 1<<21)
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
	}
	else if ((instr & 1<<23) ||
                 (ARM_INS_REGB(ins) == ARM_REG_R15))
	{
		/* 12-bit immediate */
		ARM_INS_SET_IMMEDIATE(ins, instr & 0x00000fff);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
                if ((ARM_INS_REGB(ins) != ARM_REG_R15) ||
                    (instr & 1<<23))
                  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
	}
	else
	{
		if (instr & 1<<11)
		{
			/* 8-bit immediate */
			ARM_INS_SET_IMMEDIATE(ins, instr & 0x000000ff);
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

			if (instr & 1<<10)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
			if (instr & 1<<9)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
			if (instr & 1<<8)
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
		}
		else
		{
			/* shifted register */
			ARM_INS_SET_REGC(ins, instr & 0x0000000f);
			Thumb32ShiftDecodeImm(ins, ARM_SHIFT_TYPE_LSL_IMM, (instr & 0x00000030) >> 4);

			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| (FL_PREINDEX | FL_DIRUP));

			if ((opc == TH32_LDRSB) ||
					(opc == TH32_LDRSH))
			{
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
			}
		}
	}

	if ((opc == TH32_PLD) ||
			(opc == TH32_PLDW) ||
			(opc == TH32_PLI))
	{
		ARM_INS_SET_REGA(ins, ARM_REG_NONE);
	}
}

void Thumb32DisassembleDataprocRegister(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling dataprocessing (register) instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_DATAPROC);

	if ((opc != TH32_RRX) &&
			(opc != TH32_TEQ) &&
			(opc != TH32_ORN) &&
			(opc != TH32_PKHBT) &&
			(opc != TH32_PKHTB) &&
			(opc != TH32_RSB))
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	if ((opc == TH32_LSL) ||
			(opc == TH32_LSR) ||
			(opc == TH32_ASR) ||
			(opc == TH32_ROR))
	{
		ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
		ARM_INS_SET_REGC(ins, (instr & 0x000f0000) >> 16);

		ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_REG + ((instr & 0x00600000) >> (16+5)));
		ARM_INS_SET_REGS(ins, instr & 0x0000000f);
	}
	else
	{
		ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
		ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
		ARM_INS_SET_REGC(ins, instr & 0x0000000f);

		if (opc != TH32_MOV)
			Thumb32ShiftDecodeImm(ins, (instr & 0x00000030) >> 4, ((instr & 0x00007000) >> 10) | ((instr & 0x000000c0) >> 6));
	}

	/* some instructions do not use the first operand */
	if ((opc == TH32_MVN) ||
			(opc == TH32_RRX) ||
			(opc == TH32_MOV))
	{
		ARM_INS_SET_REGB(ins, ARM_REG_NONE);
	}

	/* some instructions do not save their result in a destination register */
	if ((opc == TH32_TST) ||
			(opc == TH32_TEQ) ||
			(opc == TH32_CMN) ||
			(opc == TH32_CMP))
	{
		ARM_INS_SET_REGA(ins, ARM_REG_NONE);
	}

	/* S-flag */
	if (instr & 1<<(16+4))
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
}

void Thumb32DisassembleMultiply(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling multiply instruction"));

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_MUL);

	if (opc == TH32_MUL)
	{
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREFER32);
	}

	ARM_INS_SET_REGA(ins, (instr & 0x00000f00) >> 8);
	ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
	ARM_INS_SET_REGC(ins, instr & 0x0000000f);

	/* some instructions use a fourth register */
	if ((opc == TH32_MLS) ||
			(opc == TH32_MLA) ||
			(opc == TH32_SMLAD) ||
			(opc == TH32_SMLADX) ||
			(opc == TH32_SMLAWB) ||
			(opc == TH32_SMLAWT) ||
			(opc == TH32_SMLSD) ||
			(opc == TH32_SMLSDX) ||
			(opc == TH32_SMMLA) ||
			(opc == TH32_SMMLAR) ||
			(opc == TH32_SMMLS) ||
			(opc == TH32_SMMLSR) ||
			(opc == TH32_SMLABB) ||
			(opc == TH32_SMLABT) ||
			(opc == TH32_SMLATB) ||
			(opc == TH32_SMLATT) ||
			/* 64-bit multiplications use REGS for the lowe 32-bit of the result */
			(opc == TH32_SMULL) ||
			(opc == TH32_UMULL) ||
			(opc == TH32_SMLAL) ||
			(opc == TH32_SMLALBB) ||
			(opc == TH32_SMLALBT) ||
			(opc == TH32_SMLALTB) ||
			(opc == TH32_SMLALTT) ||
			(opc == TH32_SMLALD) ||
			(opc == TH32_SMLALDX) ||
			(opc == TH32_SMLSLD) ||
			(opc == TH32_SMLSLDX) ||
			(opc == TH32_UMLAL) ||
			(opc == TH32_UMAAL))
	{
		ARM_INS_SET_REGS(ins, (instr & 0x0000f000) >> 12);
	}
}

void Thumb32DisassembleCoproc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling coprocessor instruction"));

	t_uint32 coproc, opc1, opc1long, opc2, Rm, Rn, Rd, offset8;

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_UNKNOWN);

	coproc = (instr & 0x00000f00) >> 8;
	opc1 = (instr & 0x00e00000) >> 4;
  opc1long = (instr & 0x00f00000) >> 20;
  opc2 = (instr & 0x000000e0) >> 5;
	Rd = (instr & 0x0000f000) >> 12;
	Rn = (instr & 0x000f0000) >> 16;
	Rm = instr & 0x0000000f;
  offset8 = (instr & 0xff);

	switch(opc)
	{
		case TH32_MRRC2:
		case TH32_MRRC:
			ARM_INS_SET_REGA(ins, Rd);
			ARM_INS_SET_REGB(ins, Rn);

			opc1 = (instr & 0x000000f0) >> 4;
			ARM_INS_SET_IMMEDIATE(ins, (Rm << 8) | (opc1 << 4) | coproc);
			break;

		case TH32_MCRR2:
		case TH32_MCRR:
			ARM_INS_SET_REGB(ins, Rd);
			ARM_INS_SET_REGC(ins, Rn);

			opc1 = (instr & 0x000000f0) >> 4;
			ARM_INS_SET_IMMEDIATE(ins, (Rm << 8) | (opc1 << 4) | coproc);
			break;

		case TH32_MRC:
		case TH32_MRC2:
			ARM_INS_SET_REGA(ins, Rd);

			ARM_INS_SET_IMMEDIATE(ins, (Rn << 14) | (Rm << 10) | (opc2 << 7) | (opc1 << 4) | coproc);

			if (Rd == ARM_REG_R15)
			{
				ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
				ARM_INS_SET_REGA(ins, ARM_REG_NONE);
			}
			break;

		case TH32_MCR:
		case TH32_MCR2:
			ARM_INS_SET_REGC(ins, Rd);

			ARM_INS_SET_IMMEDIATE(ins, (Rn << 14) | (opc2 << 7) | (opc1 << 4) | coproc);
			break;

		case TH32_CDP:
		case TH32_CDP2:
			ARM_INS_SET_IMMEDIATE(ins, (opc1long << 20) | (Rn << 16) | (Rd << 12) | (coproc << 8) | (opc2 << 5) | Rm);
			break;

		case TH32_LDC:
		case TH32_LDC2:
		case TH32_STC:
		case TH32_STC2:
			ARM_INS_SET_REGB(ins, Rn);

			ARM_INS_SET_IMMEDIATE(ins, ((offset8) << 8) | (Rd << 4) | coproc);

      if (instr & (1 << 22)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_LONG_TRANSFER);
      if (instr & (1 << 24)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
      if (instr & (1 << 23)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
      if (instr & (1 << 21)) ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
			break;

		default:
			FATAL(("unsupported coprocessor instruction %s", thumb_opcode_table[opc].desc));
	}
}

#define VFP_FD(x) (((((x) & 0x0000f000) >> 11) | (((x) >> 22) & 1)) + ARM_REG_S0)
void Thumb32DisassembleVLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling vector load/store"));
	t_uint32 num = 0, i = 0, puw = 0, numregs = 0, dreg = 0;

	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);

	switch(opc)
	{
	case TH32_FLDMX:
		ARM_INS_SET_TYPE(ins, IT_FLT_LOAD);

	case TH32_FSTMX:
		if (opc == TH32_FSTMX)
			ARM_INS_SET_TYPE(ins, IT_FLT_STORE);

    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

		if (instr & 1<<(16+8))
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX);
		if (instr & 1<<(16+7))
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
		if (instr & 1<<(16+5))
			ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);

		ARM_INS_SET_REGB(ins, (instr & 0x000f0000) >> 16);
		ARM_INS_SET_REGC(ins, VFP_FD(instr));

		ARM_INS_SET_MULTIPLE(ins, RegsetNew());
		num = instr & 0xfe;

		for (i = 0; i < num; i++)
		{
			if (ARM_INS_REGC(ins)+i > ARM_REG_S31)
				FATAL(("implement wrap around in vfp load/store multiple"));

			ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), ARM_INS_REGC(ins)+i));
		}
		break;

  case TH32_VLDM:
    ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);

  case TH32_VSTM:
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
      ASSERT(dreg+numregs <= 32, ("Wrap-around in D-registers for %s instruction", thumb_opcode_table[opc].desc));

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
      ASSERT((dreg+numregs)-ARM_REG_S0 <= 32, ("Wrap-around in S-registers for %s instruction", thumb_opcode_table[opc].desc));

      for(i=dreg; i < dreg+numregs; i++)
      {
        ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), i));
      }
    }
    break;

	default:
		FATAL(("unsupported vector load/store %s", thumb_opcode_table[opc].desc));
	}
}

/*
void Thumb32Disassemble(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	switch(opc)
	{
		case TH32_:
			ARM_INS_SET_OPCODE(ins, ARM_);
			break;

		default:
			FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
	}
}
*/

void ThumbDisassembleNotImplemented(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	FATAL(("Implement 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
}

void ThumbDisassembleUndefined(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	FATAL(("Implement undefined 0x%08x (%s)", instr, thumb_opcode_table[opc].desc));
}

/*!
 * Disassemble Thumb If-Then and hint opcodes
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 * ThumbDisassembleV6V7ITHints {{{ */
void ThumbDisassembleV6V7ITHints(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling V6V7 IT Hints"));
	ThumbInitInstruction(ins, opc);

	switch (opc)
	{
	  case TH_NOP:
	    ARM_INS_SET_TYPE(ins,  IT_NOP);
	    break;

	  case TH_IT:
	  	ARM_INS_SET_IMMEDIATE(ins, instr & 0x000000ff);

	  case TH_YIELD:
	  case TH_WFI:
	  case TH_WFE:
	  case TH_SEV:
	    ARM_INS_SET_TYPE(ins,  IT_SYNC);
	    break;

	  default:
	    FATAL(("Unsupported thumb it/hints instruction %d",instr));
	    break;
	}
}
/*}}}*/
/*!
 * Disassemble Thumb special register move instructions
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 * ThumbDisassembleV6V7ITHints {{{ */
void ThumbDisassembleV6V7Status(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling V6V7 status"));
	ThumbInitInstruction(ins, opc);

  ARM_INS_SET_TYPE(ins, IT_STATUS);

	switch (opc)
	{
      //      case TH_MRS:
      //        ARM_INS_SET_OPCODE(ins,  ARM_MRS);
      //        ARM_INS_SET_REGA(ins,  (instr & 0x0f000000) >> 24);
	     // ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      //        ARM_INS_SET_FLAGS(ins,  FL_STATUS);
      //        /* R bit */
      //        if (instr & 0x10)
      //          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_SPSR);
      //        break;
      //      case TH_MSR:
      //        ARM_INS_SET_OPCODE(ins,  ARM_MSR);
	     // ARM_INS_SET_REGA(ins,  ARM_REG_NONE);
      //        ARM_INS_SET_REGB(ins,  instr & 0xf);
      //        ARM_INS_SET_FLAGS(ins,  0);
      //        /* R bit */
      //        if (instr & 0x10)
      //          ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_SPSR);
      //        if (instr & (1<<24))
      //          ARM_INS_SET_FLAGS(ins,   ARM_INS_FLAGS(ins) | FL_CONTROL);
      //        if (instr & (1<<25)) FATAL(("Sets extension bits"));
      //        if (instr & (1<<26)) FATAL(("Sets status bits"));
      //        if (instr & (1<<27))
      //          ARM_INS_SET_FLAGS(ins,  ARM_INS_FLAGS(ins) | FL_STATUS);
      //        break;
           default:
             FATAL(("Unsupported thumb V6V7 special register instruction %x, %s",instr, thumb_opcode_table[opc].desc));
             break;
        }
}
/*}}}*/
/*!
 * Disassemble Thumb Armv6 extract opcodes
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 * ThumbDisassembleV6Extract {{{ */
void ThumbDisassembleV6Extract(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling V6 extract"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_REGA(ins,  instr & 0x7);
	ARM_INS_SET_REGC(ins,  (instr & 0x38) >> 3);
}
/*}}}*/
/*!
 * Disassemble Thumb Conditional Branches
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleCondBranch {{{ */
void ThumbDisassembleCondBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling conditional branch"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_BRANCH);
	ARM_INS_SET_CONDITION(ins,  (instr & 0x0f00) >> 8);
	if (ARM_INS_CONDITION(ins)!=ARM_CONDITION_AL)
	  ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins) |IF_CONDITIONAL);;
	ARM_INS_SET_IMMEDIATE(ins,  2 * Uint64SignExtend(instr & 0x00ff, 7));
	ARM_INS_SET_FLAGS(ins,  FL_IMMED);
	ARM_INS_SET_OPCODE(ins,  ARM_B);
}
/*}}}*/
/*!
 * Disassemble Thumb Long Branch with Link (BL)
 *
 */
/* ThumbDisassembleBranchLink {{{ */
void ThumbDisassembleBranchLink(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling branchlink"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_BRANCH);
	ARM_INS_SET_REGB(ins, (instr & 0x0078) >> 3);
}
/*}}}*/
/*!
 * Disassemble Thumb Branches (B, BX)
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleBranch {{{ */
void ThumbDisassembleBranch(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling 16-bit thumb branch"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_BRANCH);
	switch(opc) {
	case TH_B2: /* conditional branch */
		ARM_INS_SET_IMMEDIATE(ins, 2 * Uint64SignExtend(instr & 0x00ff, 7));
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

		ARM_INS_SET_CONDITION(ins, (instr & 0x0f00) >> 8);
		if (ARM_INS_CONDITION(ins) != ARM_CONDITION_AL)
			ARM_INS_SET_ATTRIB(ins, ARM_INS_ATTRIB(ins)| IF_CONDITIONAL);

		break;
	case TH_B: /* unconditional branch */
		ARM_INS_SET_IMMEDIATE(ins,  2 * Uint64SignExtend(instr & 0x07ff, 10));
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
		break;
	case TH_BX:
		ARM_INS_SET_REGB(ins,  (instr & 0x0078) >> 3);
		ARM_INS_SET_OPCODE(ins, ((instr & 0x0080) == 0x0080) ? ARM_BLX : ARM_BX);
		break;

	case TH_CBZ:
	case TH_CBNZ:
		ARM_INS_SET_REGB(ins, (instr & 0x0007));
		ARM_INS_SET_IMMEDIATE(ins, 2 * (((instr & 0x0200) >> 4) | ((instr & 0x00f8) >> 3)));
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

		break;

	default:
		FATAL(("implement %s", thumb_opcode_table[opc].desc));
	};
}
/*}}}*/
/*!
 * Disassemble Thumb Software Interrupts (SWI)
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 * ThumbDisassembleSWI {{{ */
void ThumbDisassembleSWI(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling SWI"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_SWI);
	ARM_INS_SET_FLAGS(ins,  FL_IMMED);
	ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00ff);
}
/*}}}*/
/*!
 * Disassemble thumb softwar breakpoint (BKPT), used by ARM Semihosting as kind of swi
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 * ThumbDisassembleBKPT {{{ */
void ThumbDisassembleBKPT(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling bkpt"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_SWI);
	ARM_INS_SET_FLAGS(ins,  FL_IMMED);
	ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00ff);

}
/*}}}*/
/*!
 * Disassemble Format 2: add/subtract
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassemble3Bit {{{ */
void ThumbDisassemble3Bit(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling 3bit"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
	ARM_INS_SET_FLAGS(ins,  FL_S);
	switch (opc) {
	case TH_ADD_3R:
	case TH_SUB_3R:
		ARM_INS_SET_REGC(ins,  (instr & 0x01c0) >> 6);
		ARM_INS_SET_IMMEDIATE(ins,  0);
		break;
	case TH_ADD_3I:
	case TH_SUB_3I:
		ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
		ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x01c0) >> 6);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
	};
}
/*}}}*/
/*!
 * Disassemble Format 3: move/compare/add/subtract immediate
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleImm {{{ */
void ThumbDisassembleImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling immediate"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00ff);
	ARM_INS_SET_FLAGS(ins,  FL_S | FL_IMMED);
	switch(opc) {
	case TH_MOV_I:
		ARM_INS_SET_REGA(ins,  (instr & 0x0700) >> 8);
		break;
	case TH_CMP_I:
		ARM_INS_SET_REGB(ins,  (instr & 0x0700) >> 8);
		break;
	case TH_ADD_2I:
	case TH_SUB_2I:
		ARM_INS_SET_REGA(ins,  (instr & 0x0700) >> 8);
		ARM_INS_SET_REGB(ins,  (instr & 0x0700) >> 8);
	};
}
/*}}}*/
/*!
 * Disassemble Format 1: move shifted register
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleShifted {{{ */
void ThumbDisassembleShifted(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling shifted"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_CONDITION(ins,  ARM_CONDITION_AL);
	ARM_INS_SET_FLAGS(ins,  FL_S);
	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGC(ins,  (instr & 0x0038) >> 3);
	ARM_INS_SET_SHIFTLENGTH(ins,  (instr & 0x07c0) >> 6);
	if (opc == TH_LSL_I)
		ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_LSL_IMM);
	else if (opc == TH_LSR_I)
		ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_LSR_IMM);
	else
		ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_ASR_IMM);
}
/*}}}*/
/*!
 * Disassemble Thumb ALU operations
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleALU {{{ */
void ThumbDisassembleALU(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling ALU"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_FLAGS(ins,  FL_S);
	switch(opc) {
	case TH_AND:
	case TH_EOR:
	case TH_ADC:
	case TH_SBC:
	case TH_ORR:
	case TH_BIC:
		ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
		ARM_INS_SET_REGA(ins,  instr & 0x0007);
		ARM_INS_SET_REGB(ins,  instr & 0x0007);
		ARM_INS_SET_REGC(ins,  (instr & 0x0038) >> 3);
		break;
	case TH_LSL_R:
	case TH_LSR_R:
	case TH_ASR_R:
	case TH_ROR:
		ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
		ARM_INS_SET_REGA(ins,  instr & 0x0007);
		ARM_INS_SET_REGC(ins,  instr & 0x0007);
		ARM_INS_SET_REGS(ins,  (instr & 0x0038) >> 3);
		if (opc == TH_LSL_R)
			ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_LSL_REG);
		else if (opc == TH_LSR_R)
			ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_LSR_REG);
		else if (opc == TH_ASR_R)
			ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_ASR_REG);
		else
			ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_ROR_REG);
		break;
	case TH_TST:
	case TH_CMN:
	case TH_CMP_R:
		ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
		ARM_INS_SET_REGB(ins,  instr & 0x0007);
		ARM_INS_SET_REGC(ins,  (instr & 0x0038) >> 3);
		break;
	case TH_NEG:
		ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
		ARM_INS_SET_REGA(ins,  instr & 0x0007);
		ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
		break;
	case TH_MUL:
		ARM_INS_SET_TYPE(ins,  IT_MUL);
		ARM_INS_SET_REGA(ins,  instr & 0x0007);
		ARM_INS_SET_REGB(ins,  instr & 0x0007);
		ARM_INS_SET_REGC(ins,  (instr & 0x0038) >> 3);
		break;
	case TH_MVN:
		ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
		ARM_INS_SET_REGA(ins,  instr & 0x0007);
		ARM_INS_SET_REGC(ins,  (instr & 0x0038) >> 3);
	};
}
/*}}}*/
/*!
 * Disassemble Format 5: Hi register operations
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleHiReg {{{ */
void ThumbDisassembleHiReg(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling hireg"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_REGC(ins,  (instr & 0x0078) >> 3);
	ARM_INS_SET_FLAGS(ins,  0x0);
	switch(opc) {
	case TH_ADD_2RH:
		ARM_INS_SET_REGA(ins,  ((instr & 0x0080) >> 4) | (instr & 0x0007));
		ARM_INS_SET_REGB(ins,  ((instr & 0x0080) >> 4) | (instr & 0x0007));
		break;
	case TH_CMP_RH:
		ARM_INS_SET_REGB(ins,  ((instr & 0x0080) >> 4) | (instr & 0x0007));
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
		break;
	case TH_MOV_RH:
		ARM_INS_SET_REGA(ins,  ((instr & 0x0080) >> 4) | (instr & 0x0007));
		break;
	case TH_MOVS:
		ARM_INS_SET_REGA(ins, instr & 0x0007);
		ARM_INS_SET_REGC(ins, (instr & 0x0038) >> 3);

		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_S);
		break;
	};
}
/*}}}*/
/*!
 * Disassemble Load address
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleLoadAddress {{{ */
void ThumbDisassembleLoadAddress(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling load address"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_REGA(ins,  (instr & 0x0700) >> 8);
	ARM_INS_SET_REGB(ins,  (opc == TH_ADD_PC) ? ARM_REG_R15 : ARM_REG_R13);
	ARM_INS_SET_IMMEDIATE(ins,  4 * (instr & 0x00ff));
	ARM_INS_SET_FLAGS(ins,  FL_IMMED);
}
/*}}}*/
/*!
 * Disassemble Format 13: add offset to SP
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleStack {{{ */
void ThumbDisassembleStack(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling stack"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
	ARM_INS_SET_REGA(ins,  ARM_REG_R13);
	ARM_INS_SET_REGB(ins,  ARM_REG_R13);
	ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x007f) << 2);
	ARM_INS_SET_FLAGS(ins,  FL_IMMED);
}
/*}}}*/
/*!
 * Disassemble Format 9: load/store with immediate offset
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferImm {{{ */
void ThumbDisassembleTransferImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling immediate xfer"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_FLAGS(ins,  FL_PREINDEX | FL_DIRUP | FL_IMMED);
	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
	switch (opc) {
	case TH_STR_I:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		ARM_INS_SET_IMMEDIATE(ins,  4 * ((instr & 0x07c0) >> 6));
		break;
	case TH_LDR_I:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
		ARM_INS_SET_IMMEDIATE(ins,  (instr & 0x07c0) >> 4);
		break;
	case TH_STRB_I:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		ARM_INS_SET_IMMEDIATE(ins,  1 * ((instr & 0x07c0) >> 6));
		break;
	case TH_LDRB_I:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
		ARM_INS_SET_IMMEDIATE(ins,  1 * ((instr & 0x07c0) >> 6));
	};
}
/*}}}*/
/*!
 * Disassemble Format 10: load/store halfword
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferHalf {{{ */
void ThumbDisassembleTransferHalf(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling half xfer"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_PREINDEX | FL_DIRUP | FL_IMMED);
	ARM_INS_SET_IMMEDIATE(ins,  2 * ((instr & 0x07c0) >> 6));
	switch (opc) {
	case TH_STRH_I:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		break;
	case TH_LDRH_I:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
	};
}
/*}}}*/
/*!
 * Disassemble Format 8: load/store sign-extended byte/halfword
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferSign {{{ */
void ThumbDisassembleTransferSign(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling sign xfer"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
	ARM_INS_SET_REGC(ins,  (instr & 0x01c0) >> 6);
	ARM_INS_SET_FLAGS(ins,  FL_PREINDEX | FL_DIRUP);
	switch (opc) {
	case TH_STRH_R:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		break;
	case TH_LDRH_R:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
		break;
	case TH_LDRSB:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
		break;
	case TH_LDRSH:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
	};
}
/*}}}*/
/*!
 * Disassemble Format 7: load/store with register offset
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferRegOff {{{ */
void ThumbDisassembleTransferRegOff(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling regoff xfer"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_REGA(ins,  instr & 0x0007);
	ARM_INS_SET_REGB(ins,  (instr & 0x0038) >> 3);
	ARM_INS_SET_REGC(ins,  (instr & 0x01c0) >> 6);
	ARM_INS_SET_FLAGS(ins,  FL_PREINDEX | FL_DIRUP);
	switch (opc) {
	case TH_STR_R:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		break;
	case TH_LDR_R:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
		break;
	case TH_STRB_R:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		break;
	case TH_LDRB_R:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
	};
}
/*}}}*/
/*!
 * Disassemble Format 6: PC-relative load
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferPC {{{ */
void ThumbDisassembleTransferPC(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling xfer PC"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  IT_LOAD);
	ARM_INS_SET_REGA(ins,  (instr & 0x0700) >> 8);
	ARM_INS_SET_REGB(ins,  ARM_REG_R15);
	ARM_INS_SET_FLAGS(ins,  FL_PREINDEX | FL_DIRUP | FL_IMMED);
	ARM_INS_SET_IMMEDIATE(ins,  4 * (instr & 0x00ff));
}
/*}}}*/
/*!
 * Disassemble Format 11: SP-relative load
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleTransferSP {{{ */
void ThumbDisassembleTransferSP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling xfer SP"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_REGA(ins,  (instr & 0x0700) >> 8);
	ARM_INS_SET_REGB(ins,  ARM_REG_R13);
	ARM_INS_SET_FLAGS(ins,  FL_PREINDEX | FL_DIRUP | FL_IMMED);
	ARM_INS_SET_IMMEDIATE(ins,  4 * (instr & 0x00ff));
	switch (opc) {
	case TH_STR_SP:
		ARM_INS_SET_TYPE(ins,  IT_STORE);
		break;
	case TH_LDR_SP:
		ARM_INS_SET_TYPE(ins,  IT_LOAD);
	};
}
/*}}}*/
/*!
 * Disassemble Format 15: multiple load/store
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassembleMultipleTransfer {{{ */
void ThumbDisassembleMultipleTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling xfer multiple"));
	ThumbInitInstruction(ins, opc);

	switch(opc) {
	case TH_STM:
	case TH_LDM:
		ARM_INS_SET_TYPE(ins,  (opc == TH_STM) ? IT_STORE_MULTIPLE : IT_LOAD_MULTIPLE);
		ARM_INS_SET_REGB(ins,  (instr & 0x0700) >> 8);
		ARM_INS_SET_IMMEDIATE(ins,  instr & 0x00ff);
		ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_DIRUP);
		if ((opc == TH_STM) ||
		    !(ARM_INS_IMMEDIATE(ins) & (1 << ARM_INS_REGB(ins))))
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_WRITEBACK);
	};
}
/*}}}*/
/*!
 * Disassemble Thumb Push & Pop
 *
 * \param ins The thumb instruction that gets filled in
 * \param instr The encoded thumb instruction
 * \param opc The opcode
 *
 * \return void
 */
/* ThumbDisassemblePP {{{ */
 void ThumbDisassemblePP(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling PP"));
	ThumbInitInstruction(ins, opc);

	ARM_INS_SET_TYPE(ins,  (opc == TH_PUSH) ? IT_STORE_MULTIPLE : IT_LOAD_MULTIPLE);
	ARM_INS_SET_REGB(ins,  ARM_REG_R13);
	ARM_INS_SET_IMMEDIATE(ins, (instr & 0x00ff) | ((instr & 0x0100) << ((opc == TH_PUSH) ? 6 : 7)));
	ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK | ( (opc == TH_PUSH) ? FL_PREINDEX : FL_DIRUP ));
}
/* }}} */
/* ThumbDisassembleUnsupported {{{ */
void ThumbDisassembleUnsupported(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	FATAL(("Unsupported thumb instruction. Instr = 0x%x\n", instr));
}
/* }}} */
/*!
 * \todo Document
 *
 *
 * \param ins
 * \param instr
 * \param opc
 *
 * \return void
*/
/* Data {{{ */
void ThumbDisassembleData(t_arm_ins * ins, t_uint32 instr, t_uint16 opc ) {
	//DEBUG(("disassembling data"));
	ThumbInitInstruction(ins, opc);

	  /* fills in a thumb_ins with the appropriate values if we're dealing with a data word */
	ARM_INS_SET_TYPE(ins,  IT_DATA);
	ARM_INS_SET_OPCODE(ins,  ARM_DATA);
	ARM_INS_SET_IMMEDIATE(ins,  instr);
	ARM_INS_SET_FLAGS(ins,  FL_THUMB | FL_IMMED);
	ARM_INS_SET_ATTRIB(ins,  IF_DATA);
}

void Thumb32DisassembleSIMD3RegsDifferentLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = DT_NONE;

	ThumbInitInstruction(ins, opc);

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
    case TH32_VMULL:
      if(instr & 0x00000200)
      {
        datatype += DT_P_START;
      }
      else
      {
        datatype += (instr & 0x10000000) ? DT_U_START : DT_S_START;
      }
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    case TH32_VADDL:
    case TH32_VSUBL:
    case TH32_VABAL:
    case TH32_VABDL:
    case TH32_VMLAL:
    case TH32_VMLSL:
    case TH32_VQDMLAL:
    case TH32_VQDMLSL:
    case TH32_VQDMULL:
      datatype += (instr & 0x10000000) ? DT_U_START : DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    case TH32_VADDW:
    case TH32_VSUBW:
      datatype += (instr & 0x10000000) ? DT_U_START : DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD | NEONFL_C_DOUBLE));
      break;

    case TH32_VADDHN:
    case TH32_VRADDHN:
    case TH32_VSUBHN:
    case TH32_VRSUBHN:
      datatype += 1 + DT_I_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD | NEONFL_C_QUAD));
      break;

    case TH32_VMULL_POLY:
      datatype += DT_P_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE | NEONFL_C_DOUBLE));
      break;

    default:
      FATAL(("Unsupported SIMD[3 registers, different length]: %s", thumb_opcode_table[opc].desc));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void Thumb32DisassembleSIMD3RegsSameLength(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = DT_NONE;
  t_uint32 size = 0;

	ThumbInitInstruction(ins, opc);

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
    case TH32_VPMIN:
    case TH32_VPMAX:
      ASSERT((instr & 0x00000040) == 0, ("Illegal Q-field in VPADD instruction: should be zero."));

    case TH32_VHADD:
    case TH32_VHSUB:
    case TH32_VQADD:
    case TH32_VQSUB:
    case TH32_VRHADD:
    case TH32_VCGT:
    case TH32_VCGE:
    case TH32_VSHL:
    case TH32_VQSHL:
    case TH32_VRSHL:
    case TH32_VQRSHL:
    case TH32_VMAX:
    case TH32_VMIN:
    case TH32_VABD:
    case TH32_VABA:
      datatype = ((instr & 0x10000000) ? DT_U_START : DT_S_START) + size;
      break;

    case TH32_VORR:
    case TH32_VAND:
    case TH32_VBIC:
    case TH32_VORN:
    case TH32_VEOR:
    case TH32_VBIF:
    case TH32_VBIT:
    case TH32_VBSL:
      break;

    case TH32_VMUL:
      datatype = ((instr & 0x10000000) ? DT_P_START : DT_I_START) + size;
      break;

    case TH32_VPADD:
      ASSERT((instr & 0x00000040) == 0,("Illegal Q-field in VPADD instruction: should be zero."));

    case TH32_VADD:
    case TH32_VSUB:
    case TH32_VCEQ:
    case TH32_VMLA:
    case TH32_VMLS:
      datatype = DT_I_START + size;
      break;

    case TH32_VTST:
      datatype = DT_START + size;
      break;

    case TH32_VPADD_F:
    case TH32_VPMAX_F:
    case TH32_VPMIN_F:
      ASSERT((instr & 0x00000040) == 0, ("Illegal Q-field in VPADD instruction: should be zero."));

    case TH32_VADD_F:
    case TH32_VSUB_F:
    case TH32_VABD_F:
    case TH32_VMUL_F:
    case TH32_VMLA_F:
    case TH32_VMLS_F:
    case TH32_VCEQ_F:
    case TH32_VCGE_F:
    case TH32_VCGT_F:
    case TH32_VACGE_F:
    case TH32_VACGT_F:
    case TH32_VMAX_F:
    case TH32_VMIN_F:
    case TH32_VRECPS_F:
    case TH32_VRSQRTS_F:
      ASSERT((size & 1) == 0, ("Illegal size-field in floating-point SIMD instruction, should be zero."));

    case TH32_VFMA:
    case TH32_VFMS:
      datatype = DT_F32;
      break;

    case TH32_VQDMULH:
    case TH32_VQRDMULH:
      datatype = DT_S_START + size;
      break;

    default:
      FATAL(("Unsupported SIMD[3 registers, same length] instruction: %s %u", thumb_opcode_table[opc].desc, opc));
  }

  if((opc == TH32_VQRSHL) || (opc == TH32_VSHL) || (opc == TH32_VQSHL) || (opc == TH32_VRSHL))
  {
    ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
    ARM_INS_SET_REGC(ins, NEON_VN_QD(instr));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void Thumb32DisassembleSIMD2RegsShift(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 imm = 0;
  t_uint32 imminv = 0;
  t_uint32 datatype = DT_NONE;

	ThumbInitInstruction(ins, opc);

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
    case TH32_VSHR:
    case TH32_VSRA:
    case TH32_VRSHR:
    case TH32_VRSRA:
    case TH32_VSRI:
    case TH32_VSHL_IMM:
    case TH32_VSLI:
    case TH32_VQSHL_IMM:
    case TH32_VQSHLU_IMM:
    case TH32_VSHLL_IMM:
      imm = ((instr & 0x003f0000) >> 16);
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));

      /* TODO: incorporate the fact that there is no C-register here, or can we just set flags for the C-register and ignore them? */
      if(opc == TH32_VSHLL_IMM)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      }
      else
      {
        InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      }

      if((opc == TH32_VSRI) || (opc == TH32_VSLI))
      {
        datatype = DT_START;
      }
      else if(opc == TH32_VSHL_IMM)
      {
        datatype = DT_S_START;/* TODO: change S to I, this was done to match output of objdump */
      }
      else if((opc == TH32_VQSHLU_IMM) || (opc == TH32_VQSHL_IMM))
      {
        if(instr & 0x00000100)
        {
          /* op=1; VQSHL */
          datatype = ((instr & 0x10000000) ? DT_U_START : DT_S_START);
        }
        else
        {
          /* op=0; VQSHLU */
          if(instr & 0x10000000)
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
        datatype = ((instr & 0x10000000) ? DT_U_START : DT_S_START);
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

      if((opc == TH32_VSHR) || (opc == TH32_VSRA) || (opc == TH32_VRSHR) ||
        (opc == TH32_VRSRA) || (opc == TH32_VSRI))
      {
        imm = imminv - imm;
      }

      break;

    case TH32_VQSHRN:
    case TH32_VQSHRUN:
    case TH32_VQRSHRN:
    case TH32_VQRSHRUN:
    case TH32_VSHRN:
    case TH32_VRSHRN:
      imm = ((instr & 0x003f0000) >> 16);
      ASSERT(imm & 0x38, ("Illegal imm-field in VSHRN instruction"));

      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));

      switch(opc)
      {
        case TH32_VSHRN:
        case TH32_VRSHRN:
          datatype = DT_I_START;
          break;

        case TH32_VQSHRN:
        case TH32_VQRSHRN:
          datatype = (instr & 0x10000000) ? DT_U_START : DT_S_START;
          break;
        case TH32_VQSHRUN:
        case TH32_VQRSHRUN:
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

    case TH32_VMOVL1:
    case TH32_VMOVL2:
    case TH32_VMOVL3:
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
      datatype = ((instr & 0x10000000) ? DT_U_START : DT_S_START);
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

    case TH32_VCVT_FX:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
      InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      imm = 64 - ((instr & 0x003f0000) >> 16);
      if(instr & 0x00000100)
      {
        datatype = (instr & 0x10000000) ? DT_U32 : DT_S32;
        ARM_INS_SET_DATATYPEOP(ins, DT_F32);
      }
      else
      {
        datatype = DT_F32;
        ARM_INS_SET_DATATYPEOP(ins, (instr & 0x10000000) ? DT_U32 : DT_S32);
      }
      break;

    default:
      FATAL(("Unsupported SIMD[2 registers, shift] instruction: %s", thumb_opcode_table[opc].desc));
  }

  if(opc != TH32_VMOVL1
     && opc != TH32_VMOVL2
     && opc != TH32_VMOVL3)
  {
    ARM_INS_SET_IMMEDIATE(ins, imm);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
  }
  ARM_INS_SET_DATATYPE(ins, datatype);
}

void Thumb32DisassembleSIMD(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 size = 0;
  t_uint32 imm = 0;
  t_uint32 datatype = DT_NONE;
  int i;
  int len, regn;

	ThumbInitInstruction(ins, opc);

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
    case TH32_VDUP:
      ARM_INS_SET_REGA(ins, NEON_VN_QD(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| ((instr & 0x00200000) ? NEONFL_A_QUAD : NEONFL_A_DOUBLE));
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| NEONFL_B_CORE);

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

    case TH32_VDUP_SCALAR:
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

    case TH32_VEXT:
      ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
      ARM_INS_SET_REGB(ins, NEON_VN_QD(instr));
      ARM_INS_SET_REGC(ins, NEON_VM_QD(instr));
      ARM_INS_SET_IMMEDIATE(ins, (instr & 0x00000f00) >> 8);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);
      InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
      datatype = DT_8;
      break;

    case TH32_VTBL:
    case TH32_VTBX:
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
      FATAL(("Unsupported SIMD usage: %s (opcode=%u)", thumb_opcode_table[opc].desc, opc));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void Thumb32DisassembleSIMDImm(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint64 immediate;
  t_uint32 cmode;
  t_uint32 op;
  t_int64 temp = 0;
  t_uint32 i, j;

	ThumbInitInstruction(ins, opc);

  ARM_INS_SET_TYPE(ins,  IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  immediate = (t_uint64)(((instr & 0x10000000) >> 21) | ((instr & 0x00070000) >> 12) | (instr & 0x0000000f));
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
          t_uint64 bit = (immediate >> (7 - i)) & 1;

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

void Thumb32DisassembleSIMD2RegsMisc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 size = 0;
  t_uint32 datatype = DT_NONE;
  t_uint32 imm = 0;

	ThumbInitInstruction(ins, opc);

  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, NEON_VD_QD(instr));
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, NEON_VM_QD(instr));
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  if((opc != TH32_VSHLL) &&
    (opc != TH32_VQMOVN) &&
    (opc != TH32_VQMOVUN) &&
    (opc != TH32_VMOVN) &&
    (opc != TH32_VCVT_HS))
  {
    InstructionSetRegsQuadDouble(ins, (instr & 0x00000040));
  }

  size = ((instr & 0x000c0000) >> 18);

  switch(opc)
  {
    case TH32_VREV16:
    case TH32_VREV32:
    case TH32_VREV64:
    case TH32_VTRN:
    case TH32_VUZP:
    case TH32_VZIP:
      datatype = DT_START+size;
      break;

    case TH32_VPADDL:
    case TH32_VPADAL:
      datatype = (instr & 0x00000080) ? DT_U_START : DT_S_START;
      datatype += size;
      break;

    case TH32_VCLS:
    case TH32_VQABS:
    case TH32_VQNEG:
      datatype = DT_S_START+size;
      break;

    case TH32_VSHLL:
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
    case TH32_VCLZ:
      datatype = DT_I_START+size;
      break;

    case TH32_VMOVN:
      datatype = DT_I_START+size+1;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_QUAD));
      break;

    case TH32_VCNT:
      datatype = DT_8;
      break;

    case TH32_VSWP:
      if(size != 0)
      {
        FATAL(("Illegal size field for VSWP instruction. Should be zero but isn't zero."));
      }
    case TH32_VMVN:
      break;

    case TH32_VCGT_IMM:
    case TH32_VCGE_IMM:
    case TH32_VCEQ_IMM:
    case TH32_VCLE_IMM:
    case TH32_VCLT_IMM:
      ARM_INS_SET_IMMEDIATE(ins, 0);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_IMMED);

    case TH32_VABS:
    case TH32_VNEG:
      if((instr & 0x00000400) && (size != 2))
      {
        FATAL(("Illegal size field in SIMD[2 regs, misc value] instruction"));
      }

      if(opc == TH32_VCEQ_IMM)
      {
        datatype = (instr & 0x00000400) ? DT_F_START : DT_I_START;
      }
      else
      {
        datatype = (instr & 0x00000400) ? DT_F_START : DT_S_START;
      }
      datatype += size;
      break;

    case TH32_VCVT_HS:
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

    case TH32_VCVT_FI:
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

    case TH32_VRECPE:
    case TH32_VRSQRTE:
      datatype = (instr & 0x00000100) ? DT_F32 : DT_U32;
      if(size != 2)
      {
        FATAL(("Illegal size-field in VRECPE/VRSQRTE instruction."));
      }
      break;

    case TH32_VQMOVN:
    case TH32_VQMOVUN:
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
      FATAL(("Unsupported SIMD[2 regs, misc value] instruction: %s (%x)", thumb_opcode_table[opc].desc, (opc-TH32_VREV64)));
  }

  ARM_INS_SET_DATATYPE(ins, datatype);
}

void Thumb32DisassembleSIMD2RegsScalar(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 datatype = 0;
  t_uint32 vm = 0, index = 0;

	ThumbInitInstruction(ins, opc);

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
    case TH32_VMLA_SCALAR:
    case TH32_VMLS_SCALAR:
    case TH32_VMUL_SCALAR:
      /* check the F-flag of the instruction to get I or F */
      datatype += ((instr & 0x00000100) ? DT_F_START : DT_I_START);

      /* F ==> size must be 32 */
      if((instr & 0x00000100) && (datatype != DT_F32))
      {
        FATAL(("Size-field of floating-point scalar must be 32"));
      }

      /* check the Q-flag of the instruction */
      if(instr & 0x10000000)
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_QUAD));
      }
      else
      {
        ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_DOUBLE | NEONFL_B_DOUBLE));
      }

      break;

    case TH32_VMLAL_SCALAR:
    case TH32_VMLSL_SCALAR:
    case TH32_VMULL_SCALAR:
      datatype += ((instr & 0x10000000) ? DT_U_START : DT_S_START);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      break;

    case TH32_VQDMLAL_SCALAR:
    case TH32_VQDMLSL_SCALAR:
    case TH32_VQDMULL_SCALAR:
      datatype += DT_S_START;
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_QUAD | NEONFL_B_DOUBLE));
      break;

    case TH32_VQDMULH_SCALAR:
    case TH32_VQRDMULH_SCALAR:
      datatype += DT_S_START;

      if(instr & 0x10000000)
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

void Thumb32DisassembleSIMDTransfer(t_arm_ins * ins, t_uint32 instr, t_uint16 opc) {
  t_uint32 opc1 = 0;
  t_uint32 opc2 = 0;

	ThumbInitInstruction(ins, opc);

  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_DATATYPE(ins, DT_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  switch(opc)
  {
    case TH32_VMOV_C2S:
      ARM_INS_SET_REGA(ins, NEON_VN_S(instr));
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SINGLE | NEONFL_B_CORE));
      break;

    case TH32_VMOV_S2C:
      ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
      ARM_INS_SET_REGB(ins, NEON_VN_S(instr));
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_SINGLE));
      break;

    case TH32_VMOV_C2SCALAR:
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

    case TH32_VMOV_SCALAR2C:
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

    case TH32_VMRS:
      ARM_INS_SET_REGA(ins, (instr & 0x0000f000) >> 12);
		if (ARM_INS_REGA(ins)==ARM_REG_R15) {
		  ARM_INS_SET_REGA(ins, ARM_REG_CPSR);
		  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);
		}
      ARM_INS_SET_REGB(ins, ARM_REG_FPSCR);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_CORE | NEONFL_B_CORE));
      ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
      break;

    case TH32_VMSR:
      ARM_INS_SET_REGA(ins, ARM_REG_FPSCR);
      ARM_INS_SET_REGB(ins, (instr & 0x0000f000) >> 12);
		if (ARM_INS_REGB(ins)==ARM_REG_R15)
		  ARM_INS_SET_REGB(ins, ARM_REG_CPSR);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins)| (NEONFL_A_SINGLE | NEONFL_B_CORE));
      ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);
      break;

    default:
      FATAL(("Unsupported SIMD register transfer instruction: %s", thumb_opcode_table[opc].desc));
  }
}

void Thumb32DisassembleSIMDLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc) {
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

	ThumbInitInstruction(ins, opc);

  ARM_INS_SET_CONDITION(ins, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  if((opc >= TH32_SIMD_FIRSTSTORE) && (opc <= TH32_SIMD_LASTSTORE))
  {
    ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);
  }
  else if((opc >= TH32_SIMD_FIRSTLOAD) && (opc <= TH32_SIMD_LASTLOAD))
  {
    ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);
  }
  else
  {
    FATAL(("Illegal SIMD load/store multiple opcode: %s", thumb_opcode_table[opc].desc));
  }

  switch(opc)
  {
    case TH32_VLD4_MULTI:
    case TH32_VST4_MULTI:
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

    case TH32_VLD4_ONE:
    case TH32_VST4_ONE:
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

    case TH32_VLD4_ALL:
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

    case TH32_VLD3_MULTI:
    case TH32_VST3_MULTI:
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

    case TH32_VLD3_ONE:
    case TH32_VST3_ONE:
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

    case TH32_VLD3_ALL:
      size = (instr & 0x000000c0) >> 6;
      ASSERT(size != 3, ("Illegal size-field in VLD3 instruction."));

      align = (instr & 0x00000010) >> 4;
      ASSERT(align == 0, ("Illegal align-field in VLD3 instruction."));

      numregs = 3;
      regd_increment += (instr & 0x00000020) ? 1 : 0;

      ARM_INS_SET_MULTIPLESCALAR(ins, MULTIPLESCALAR_ALL);
      ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | NEONFL_MULTI_SCALAR);

      break;

    case TH32_VLD2_MULTI1:
    case TH32_VLD2_MULTI2:
    case TH32_VST2_MULTI1:
    case TH32_VST2_MULTI2:
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

    case TH32_VLD2_ONE:
    case TH32_VST2_ONE:
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

    case TH32_VLD2_ALL:
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

    case TH32_VLD1_MULTI1OR3:
    case TH32_VLD1_MULTI2OR4:
    case TH32_VST1_MULTI1OR3:
    case TH32_VST1_MULTI2OR4:
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

    case TH32_VLD1_ONE:
    case TH32_VST1_ONE:
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

    case TH32_VLD1_ALL:
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
      FATAL(("Unsupported SIMD load/store instruction: %s", thumb_opcode_table[opc].desc));
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
  if(regm == 13)
  {
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)| FL_WRITEBACK);
  }

  /* offset register */
  ARM_INS_SET_REGC(ins, (((regm == 15) || (regm == 13)) ? ARM_REG_NONE : regm));

  /* various settings */
  ARM_INS_SET_DATATYPE(ins, DT_START+size);
  ARM_INS_SET_NEONFLAGS(ins, ARM_INS_NEONFLAGS(ins) | (NEONFL_B_CORE | NEONFL_C_CORE));
}

void Thumb32DisassembleFPLoadStore(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_reg dreg;
  t_uint32 numregs, i;
  t_uint32 puw;

	ThumbInitInstruction(ins, opc);

  /* generic information */
  ARM_INS_SET_ATTRIB(ins, 0x0);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_TYPE(ins, IT_STORE_MULTIPLE);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  switch(opc)
  {
    case TH32_VLDM:
      ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);

    case TH32_VSTM:
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

    case TH32_VPOP:
    case TH32_VPUSH:
      if(opc == TH32_VPOP)
        ARM_INS_SET_TYPE(ins, IT_LOAD_MULTIPLE);

      if((opc == TH32_VPUSH) || (opc == TH32_VPOP)) {
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
        ASSERT(dreg+numregs <= 32, ("Wrap-around in D-registers for %s instruction", thumb_opcode_table[opc].desc));

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
        ASSERT((dreg+numregs)-ARM_REG_S0 <= 32, ("Wrap-around in S-registers for %s instruction", thumb_opcode_table[opc].desc));

        for(i=dreg; i < dreg+numregs; i++)
        {
          ARM_INS_SET_MULTIPLE(ins, RegsetAddReg(ARM_INS_MULTIPLE(ins), i));
        }
      }
      break;

    case TH32_VLDR:
      ARM_INS_SET_TYPE(ins, IT_FLT_LOAD);

    case TH32_VSTR:
      if(opc == TH32_VSTR)
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
      FATAL(("Illegal opcode: %s", thumb_opcode_table[opc].desc));
  }
}

void Thumb32DisassembleFPDataProc(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
	//DEBUG(("disassembling FP [dataproc] instruction"));
  t_arm_ins_dt dt = DT_NONE, dtop = DT_NONE;
  t_uint64 imm = 0;
  t_uint64 temp = 0;
 	int i;

	ThumbInitInstruction(ins, opc);

  /* generic information */
  ARM_INS_SET_ATTRIB(ins, 0x0);

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
      case TH32_VMOV_FIMM:
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

			  ARM_INS_SET_TYPE(ins,  IT_FLT_ALU);

			  if (instr & (1<<8))
			    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins)|FL_VFP_DOUBLE);

        break;

    case TH32_VMOV_F:
    case TH32_VABS_F64:
    case TH32_VNEG_F64:
    case TH32_VSQRT:
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

    case TH32_VCVTB:
    case TH32_VCVTT:
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

    case TH32_VCVT_DS:
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

    case TH32_VCVT_X2F:
    case TH32_VCVT_F2X:
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

      if(opc == TH32_VCVT_X2F)
      {
        t_arm_ins_dt temp = dt;
        dt = dtop;
        dtop = temp;
      }

      ARM_INS_SET_DATATYPE(ins, dt);
      ARM_INS_SET_DATATYPEOP(ins, dtop);
      break;

    case TH32_VCVT_I2F:
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

    case TH32_VCVT_F2I:
      VERBOSE(1,("WARNING --- VCVT (floating->integer) instruction implemented but not tested: 0x%08x", instr));

    case TH32_VCVTR_F2I:
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

    case TH32_VFNMA:
    case TH32_VFNMS:
    case TH32_VFMA_F64:
    case TH32_VFMS_F64:
      VERBOSE(1,("WARNING --- VFNMA/VFMA(floating) instruction implemented but not tested: 0x%08x", instr));

    case TH32_VMLA_F64:
    case TH32_VMLS_F64:
    case TH32_VNMLA:
    case TH32_VNMLS:
    case TH32_VNMUL:
    case TH32_VMUL_F64:
    case TH32_VADD_F64:
    case TH32_VSUB_F64:
    case TH32_VDIV:
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

    case TH32_VCMP:
    case TH32_VCMPE:
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
      FATAL(("Illegal opcode: %s", thumb_opcode_table[opc].desc));
  }
}

void Thumb32DisassembleFP2R(t_arm_ins * ins, t_uint32 instr, t_uint16 opc)
{
  t_uint32 r1, r2;
  t_reg vm;

	ThumbInitInstruction(ins, opc);

  /* generic information */
  ARM_INS_SET_ATTRIB(ins, 0x0);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  ARM_INS_SET_REGA(ins, ARM_REG_NONE);
  ARM_INS_SET_REGABIS(ins, ARM_REG_NONE);
  ARM_INS_SET_REGB(ins, ARM_REG_NONE);
  ARM_INS_SET_REGC(ins, ARM_REG_NONE);
  ARM_INS_SET_REGS(ins, ARM_REG_NONE);

  ARM_INS_SET_TYPE(ins, IT_SIMD);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_NONE);

  switch(opc)
  {
    case TH32_VMOV64_C2S:
      r1 = (instr & 0x0000f000) >> 12;
      r2 = (instr & 0x000f0000) >> 16;
      vm = NEON_VM_S(instr);
      ASSERT(vm < ARM_REG_S31, ("Illegal S-register for %s instruction", thumb_opcode_table[opc].desc));

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

    case TH32_VMOV64_C2D:
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
      FATAL(("Illegal opcode: %s", thumb_opcode_table[opc].desc));
  }
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker : */
