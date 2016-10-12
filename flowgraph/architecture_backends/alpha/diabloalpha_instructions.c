#include <diabloalpha.h>

void AlphaInsComputeRegsets(t_alpha_ins *ins)
{
	ALPHA_INS_SET_REGS_USE(ins, AlphaInsUsedRegisters(ins));
  ALPHA_INS_SET_REGS_DEF(ins, AlphaInsDefinedRegisters(ins));
}

t_regset AlphaInsDefinedRegisters(t_alpha_ins *ins) 
{

	t_regset regs = RegsetNew();

	/* The disassemble stage should have put the defined register
	 * into the the REGD member */

	if( ALPHA_INS_REGD( ins ) != ALPHA_REG_NONE)
	{
		RegsetSetAddReg(regs, ALPHA_INS_REGD( ins ) );
	}

	if( ALPHA_INS_TYPE( ins ) == IT_CALL) {
		printf("IT_CALL\n");
	}

	return regs;

}

t_regset AlphaInsUsedRegisters(t_alpha_ins *ins) 
{

	t_regset regs = RegsetNew();

	if( ALPHA_INS_REGA( ins ) != ALPHA_REG_NONE) 
	{
		RegsetSetAddReg(regs, ALPHA_INS_REGA( ins ));
	}
	if( ALPHA_INS_REGB( ins ) != ALPHA_REG_NONE) 
	{
		RegsetSetAddReg(regs, ALPHA_INS_REGB( ins ));
	}

        return regs;
}

t_bool
AlphaInsIsSystemPlug(t_ins * ins) {

	return (ALPHA_INS_TYPE((T_ALPHA_INS(ins))) == IT_SWI);
	
}

/* Determine if the GP register has been written! */

t_bool
AlphaIsGPRecomputation(t_alpha_ins * ins)
{

	/* There are 3 generic ways to write the GP */

	switch(ALPHA_INS_OPCODE(ins)) {
		case ALPHA_LDA:
			return (ALPHA_INS_REGD(ins) == ALPHA_REG_GP);
		/* Needed because we might have an empty block with jump edges */
#if 0
		case ALPHA_BR:
		case ALPHA_BSR:
			if (ALPHA_INS_REGD(ins) == ALPHA_REG_GP && \
							ALPHA_INS_IMMEDIATE(ins) == 0) 
			{
				ALPHA_INS_SET_REGD(ins, ALPHA_REG_ZERO);
				return FALSE;
			}
#endif
		case ALPHA_AND:
		case ALPHA_BIC:
		case ALPHA_BIS:
		case ALPHA_EQV:
		case ALPHA_ORNOT:
		case ALPHA_XOR:
			return (ALPHA_INS_REGD(ins) == ALPHA_REG_GP);
		default:
			break;
	}

	return FALSE;

}

t_bool 
AlphaFunIsGlobal(t_function * fun)
{
	if( FUNCTION_FLAGS( fun ) & FF_IS_EXPORTED )
	{
		return FunctionBehaves( fun );
	}

	return FALSE;
}

void 
AlphaInsMakeBr(t_alpha_ins * ins, t_bbl * target)
{

	t_address addr;

	ALPHA_INS_SET_OPCODE(ins, ALPHA_BR);
	ALPHA_INS_SET_REGB(ins, ALPHA_REG_ZERO);
  ALPHA_INS_SET_REGA(ins, ALPHA_REG_ZERO);
	ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_BR);
  ALPHA_INS_SET_TYPE(ins, IT_BRANCH);
  ALPHA_INS_SET_FLAGS(ins, 0x0);
	
	AlphaInsComputeRegsets(ins);

	addr = AddressSub(BBL_CADDRESS(target), 
			AddressAddUint32(ALPHA_INS_CADDRESS(ins), 4));

	ALPHA_INS_SET_IMMEDIATE(ins, G_T_UINT64(addr) ); 

}

void
AlphaInsMakeBsr(t_alpha_ins * ins, t_bbl * target)
{

	t_address addr;

	ALPHA_INS_SET_OPCODE(ins, ALPHA_BSR);
	ALPHA_INS_SET_REGB(ins, ALPHA_REG_ZERO);
  ALPHA_INS_SET_REGA(ins, ALPHA_REG_ZERO);
	ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_BR);
  ALPHA_INS_SET_TYPE(ins, IT_BRANCH);
  ALPHA_INS_SET_FLAGS(ins, 0x0);

	AlphaInsComputeRegsets(ins);

	addr = AddressSub(BBL_CADDRESS(target), 
			AddressAddUint32(ALPHA_INS_CADDRESS(ins), 4));

	ALPHA_INS_SET_IMMEDIATE(ins, G_T_UINT64(addr) ); 

}



void
AlphaInsMakeNoop(t_alpha_ins * ins)
{
	ALPHA_INS_SET_OPCODE(ins, ALPHA_BIS);
	/* Register should all be r0 */
	ALPHA_INS_SET_REGA(ins, ALPHA_REG_ZERO);
	ALPHA_INS_SET_REGB(ins, ALPHA_REG_ZERO);
	ALPHA_INS_SET_REGD(ins, ALPHA_REG_ZERO);
	ALPHA_INS_SET_IMMEDIATE(ins, 0);

	ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_OPR);
	ALPHA_INS_SET_TYPE(ins, IT_DATAPROC);

	AlphaInsComputeRegsets(ins);

}

t_bool AlphaInsIsNoop(t_alpha_ins * ins )
{
	return IS_NOOP(ins);
}

/* Return true if *ins produces a side effect. The following cases exist:
 * 1) A system call ( instruction PAL_callsys )
 * 2) Branches 
 * 3) Some loads and stores depending on operands.
 * 4) Other misc cases, eg IMPLVER, AMASK, EXCB.
 */

t_bool AlphaInsHasSideEffect(t_ins * gins) 
{

	t_alpha_ins * ins = T_ALPHA_INS(gins);

	switch( ALPHA_INS_FORMAT( ins ) )
	{
		case ALPHA_ITYPE_BR:	/* Normal branches, br,bsr,bne etc (and FP branchs)*/
		case ALPHA_ITYPE_MBR: /* jmp, jsr, jsr_coroutine, ret */
			return TRUE;
		default:
			switch( ALPHA_INS_OPCODE( ins ) ) 
			{
				/* Loads and stores may produce a side effect */
				case ALPHA_LDL_L:
				case ALPHA_LDQ_L:
				case ALPHA_STB:
				case ALPHA_STF:
				case ALPHA_STG:
				case ALPHA_STS:
				case ALPHA_STL:
				case ALPHA_STL_C:
				case ALPHA_STQ:
				case ALPHA_STQ_C:
				case ALPHA_STQ_U:
				case ALPHA_STT:
				case ALPHA_STW:
			
				case ALPHA_MF_FPCR:
				case ALPHA_MT_FPCR:


				case ALPHA_AMASK:
				case ALPHA_IMPLVER:

				case ALPHA_RPCC:
				case ALPHA_EXCB:
				case ALPHA_FETCH:
				case ALPHA_FETCH_M:
				case ALPHA_MB:
				case ALPHA_WMB:

				case ALPHA_PAL_callsys:
					return TRUE;
				default: 
					return FALSE;
			}
	}

	return FALSE; // Shut up compiler 

}


t_bool AlphaInsIsConditional(t_alpha_ins * ins)
{
	
	/* The following are conditional instruction as defined by the Alpha ISR 
	 * manual */

	switch(ALPHA_INS_OPCODE(ins)) {
		/* Floating point conditional moves */
		case ALPHA_FCMOVEQ:
 	  case ALPHA_FCMOVGE:
  	case ALPHA_FCMOVGT:
  	case ALPHA_FCMOVLE:
  	case ALPHA_FCMOVLT:
  	case ALPHA_FCMOVNE:
		
		/* Integer conditional move instructions */
		
		case ALPHA_CMOVEQ:
		case ALPHA_CMOVGE:
  	case ALPHA_CMOVGT:
    case ALPHA_CMOVLBC:
    case ALPHA_CMOVLBS:
    case ALPHA_CMOVLE:
    case ALPHA_CMOVLT:
  	case ALPHA_CMOVNE:
			return TRUE;

		default:
			return FALSE;

	}

	return FALSE;

}

t_bool AlphaInsIsJump(t_alpha_ins * ins) 
{

	return (alpha_opcode_table[ALPHA_INS_OPCODE(ins)].type == ALPHA_ITYPE_MBR); 

}

t_bool AlphaInsIsSystemInstruction(t_ins * ins)
{
	return ( ALPHA_INS_TYPE( (t_alpha_ins * )  ins ) == IT_SWI );

}

t_tristate AlphaInsIsSyscallExit(t_ins * ins)
{
	t_tristate ret = FALSE;
	return ret;
}


t_bool AlphaInsIsProcedureCall(t_ins * ins) 
{
	t_bool ret = FALSE;
	return ret;
}

t_bool AlphaInsIsIndirectCall(t_ins * ins)
{
	t_bool ret = FALSE;
	return ret;
}


t_bool AlphaInsIsControlTransfer(t_ins * ins) 
{
	t_bool ret = FALSE;
	return ret;
}


t_bool AlphaInsIsUnconditionalJump(t_ins * ins) 
{
		
	t_alpha_ins * ains = T_ALPHA_INS(ins);

	return (ALPHA_INS_OPCODE(ains) == ALPHA_JMP ||
					ALPHA_INS_OPCODE(ains) == ALPHA_BR);

}


void AlphaInstructionPrint(t_ins * ins, t_string str)
{
  t_alpha_ins * ains = T_ALPHA_INS(ins);
  t_string opcode = NULL;
	
 	opcode = alpha_opcode_table[ALPHA_INS_OPCODE(T_ALPHA_INS(ins))].desc;
  
	switch(ALPHA_INS_FORMAT(ains)) 
	{
		case ALPHA_ITYPE_BR:
			sprintf(str, "%s r%d %#x", opcode, (ALPHA_INS_OPCODE(ains) == ALPHA_BR ||ALPHA_INS_OPCODE(ains) == ALPHA_BSR) ? ALPHA_INS_REGD(ains) : ALPHA_INS_REGA(ains), ALPHA_INS_IMMEDIATE(ains));
			break;
		case ALPHA_ITYPE_MEM:
			sprintf(str,"%s r%d r%d %#x", opcode, (ALPHA_INS_TYPE(ains) == IT_LOAD) ? ALPHA_INS_REGD(ains) : ALPHA_INS_REGA(ains), ALPHA_INS_REGB(ains), ALPHA_INS_IMMEDIATE(ains));
			break;
		case ALPHA_ITYPE_MFC:
			sprintf(str,"%s r%d r%d", opcode, (ALPHA_INS_OPCODE(ains) == ALPHA_RPCC) ? ALPHA_INS_REGD(ains) : ALPHA_INS_REGA(ains), ALPHA_INS_REGB(ains));
			break;
		case ALPHA_ITYPE_FP:
			sprintf(str,"%s r%d r%d r%d", opcode, ALPHA_INS_REGA(ains), ALPHA_INS_REGB(ains), ALPHA_INS_REGD(ains));
			break;
		case ALPHA_ITYPE_MBR:
			sprintf(str, "%s r%d r%d %#x", opcode, ALPHA_INS_REGA(ains), ALPHA_INS_REGB(ains), ALPHA_INS_IMMEDIATE(ains));
			break;
		case ALPHA_ITYPE_PAL:
			sprintf(str,"%s", opcode);
			break;
		case ALPHA_ITYPE_OPR:
			if(ALPHA_INS_FLAGS(ains) & F_IMM)
				sprintf(str,"%s r%d %#lx r%d",opcode,ALPHA_INS_REGA(ains),ALPHA_INS_IMMEDIATE(ains), ALPHA_INS_REGD(ains)); 
			else
				sprintf(str,"%s r%d r%d r%d",opcode,ALPHA_INS_REGA(ains),ALPHA_INS_REGB(ains), ALPHA_INS_REGD(ains)); 
			break;
			default: FATAL(("Unknown instruction format...oops"));
	}

}
