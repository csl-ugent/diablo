/* Individual disassemble instructions for different instruction
 * formats.  This could probably do with some more documentation. 
 */

#include <diabloalpha.h>

void 
AlphaDisassembleInstruction(t_alpha_ins * ins, t_uint32 instr)
{
  
  t_uint32 table_offset;

  /* Find correct disassemble function and data, then disassemble. */
  table_offset = AlphaFindDisassemblyFunction(instr);
  if( table_offset == NR_INS ) 
  {
    FATAL(("Disassemble: Could not find disassemble function \
      for opcode[%lx] function[%lx]\n",instr >> 26, ALPHA_GET_FUNC(instr)));
  }

  /* Initialise registers to be NONE */

  ALPHA_INS_SET_REGA(ins, ALPHA_REG_NONE);
  ALPHA_INS_SET_REGB(ins, ALPHA_REG_NONE);
  ALPHA_INS_SET_REGD(ins, ALPHA_REG_NONE);
  /* Disassemble */
  alpha_opcode_table[table_offset].Disassemble(ins, instr, table_offset);

	if(AlphaInsIsConditional(ins))
		ALPHA_INS_SET_ATTRIB(ins, ALPHA_INS_ATTRIB(ins) | IF_CONDITIONAL);

}

/*! AlphaFindDisassemblyFunction:
 * Need to locate the correct entry in the opcode table (diabloalpha_opcodes.c)
 * so we can disassemble the instruction correctly.
 * Returns an integer i which is an offset in the table.
 */

t_uint32
AlphaFindDisassemblyFunction(t_uint32 instr)
{

  t_uint32 i;
  t_uint32 opcode = instr >> 26;

  for(i = 0; i < NR_INS; i++) 
  {
    if(opcode == alpha_opcode_table[i].opcode)
    {
      switch(alpha_opcode_table[i].type)
      {

        case ALPHA_ITYPE_BR: return i;
        case ALPHA_ITYPE_MEM: return i;
        case ALPHA_ITYPE_PAL: 
          if(ALPHA_GET_PAL_FCODE(instr) == alpha_opcode_table[i].fcode)
          {
            return i;
          }
          break;
        case ALPHA_ITYPE_OPR: 
          if(ALPHA_GET_OPR_FUNC(instr) == alpha_opcode_table[i].fcode)
          {
            return i;
          }
          break;
        case ALPHA_ITYPE_FP:
          if(ALPHA_GET_FUNC(instr) == alpha_opcode_table[i].fcode)
          {
            return i;
          }
          break;
        case ALPHA_ITYPE_MFC:
          if(ALPHA_GET_MFC_FCODE(instr) == alpha_opcode_table[i].fcode)
          {
            return i;
          }
          break;
        case ALPHA_ITYPE_MBR:
          if(ALPHA_GET_MEMBRA_OPC(instr) == alpha_opcode_table[i].fcode) \
          {
            return i;
          }
          break;
        default:
          VERBOSE((0),("Unknown instruction format for opcode: 0x%lx", opcode));
      }
    }
  }
  if(i == NR_INS) return i;
  else FATAL(("Instruction mismatch, please run chkins.pl"));
  
  return 0; // Shut up.

}

/*! Function to disassemble regular branches.
 * opcode RA branch-displacement
 */

void
AlphaDisassembleBranch(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{

  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_BR);

  ALPHA_INS_SET_TYPE(ins, IT_BRANCH);
  ALPHA_INS_SET_FLAGS(ins, 0x0);

	if(opcode == ALPHA_BR || opcode == ALPHA_BSR)
	{
		ALPHA_INS_SET_REGB(ins, ALPHA_REG_ZERO);
	  ALPHA_INS_SET_REGA(ins, ALPHA_REG_ZERO);
		ALPHA_INS_SET_REGD(ins, (instr & 0x03E00000) >> 21);
	}
	else
	{
  	/* All of these instructions use r0 since they
  	 * all compare RegA to 0.  I think.
   	*/
  	ALPHA_INS_SET_REGB(ins, ALPHA_REG_ZERO);
  	ALPHA_INS_SET_REGD(ins, ALPHA_REG_ZERO);

  	/* Floating point branch */
  	if(alpha_opcode_table[opcode].desc[0] == 'F')
   	 ALPHA_INS_SET_FLAGS(ins, F_FPCOND);

  	ALPHA_INS_SET_REGA(ins, (instr & 0x03E00000) >> 21);
  
	}
	
	if(instr & 0x00100000) /* Sign extend needed */
    ALPHA_INS_SET_IMMEDIATE(ins, (instr & 0x001FFFFF) | (-1 ^ 0x001FFFFF ));
  else
    ALPHA_INS_SET_IMMEDIATE(ins, instr & 0x001FFFFF);


}

/*! Function to disassemble operate type instructions:
 * opcode RA RB SBZ 0 FUNCTION RC
 * opcode RA   LIT  1 FUNCTION RC
 *
 * RC is the destination register as usual.
 */

void 
AlphaDisassembleOpr(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{

  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_OPR);
  ALPHA_INS_SET_TYPE(ins, IT_DATAPROC);
  ALPHA_INS_SET_FLAGS(ins, 0x0);
  
  /* Operand is a literal */
  if(instr & 0x00001000)
  {
    ALPHA_INS_SET_IMMEDIATE(ins, (instr & 0x001FE000) >> 13);
    ALPHA_INS_SET_FLAGS(ins, ALPHA_INS_FLAGS(ins) | F_IMM);
  }
  else {
    ALPHA_INS_SET_REGB(ins, ALPHA_GET_REGB(instr));
  }

  switch(opcode) {
    case ALPHA_MULQ:
    case ALPHA_UMULH:
    case ALPHA_MULL: 
      ALPHA_INS_SET_TYPE(ins, IT_MUL);
      /* Fall through and set registers */
    default:
      ALPHA_INS_SET_REGA(ins, ALPHA_GET_REGA(instr));
      ALPHA_INS_SET_REGD(ins, ALPHA_GET_REGC(instr));
      ALPHA_INS_SET_FLAGS(ins, ALPHA_INS_FLAGS(ins) | F_ICOMP);
  }

}

/*! Function to disassemble memory format instructions.
 * opcode RA RB memory-displacement
 */

void
AlphaDisassembleMem(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{

  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_MEM);
  ALPHA_INS_SET_FLAGS(ins, F_MEM);


  /* Disassembling a load instruction */
  if(alpha_opcode_table[opcode].desc[0] == 'L') 
  {
    ALPHA_INS_SET_TYPE(ins, IT_LOAD);
    ALPHA_INS_SET_FLAGS(ins, ALPHA_INS_FLAGS(ins) | F_LOAD);
    /* Set destination register: bits 21 -> 25 */
    ALPHA_INS_SET_REGD(ins, ((instr & 0x03E00000) >> 21));  
    ALPHA_INS_SET_REGB(ins, ((instr & 0x001F0000) >> 16));
  }
  else /* A store instruction */
  {
    ALPHA_INS_SET_TYPE(ins, IT_STORE);
    ALPHA_INS_SET_FLAGS(ins, ALPHA_INS_FLAGS(ins) | F_STORE);
    ALPHA_INS_SET_REGA(ins, (instr & 0x03E00000) >> 21);
    ALPHA_INS_SET_REGB(ins, (instr & 0x001F0000) >> 16);
  }

  /* bits 0 -> 15 -- sign extend if neccessary */
  if(instr & 0x00008000) 
    ALPHA_INS_SET_IMMEDIATE(ins,(instr & 0xFFFF) | (-1 ^ 0xffff));
  else
    ALPHA_INS_SET_IMMEDIATE(ins,instr & 0xFFFF);
}

/*! Memory format instructions with a function code.  In these instructions
 * the memory displacement is replaced with a function code to specify 
 * a different instruction.
 */

void
AlphaDisassembleMfc(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{
  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_MFC);
  ALPHA_INS_SET_FLAGS(ins, 0x0);

  switch(opcode) 
  {
    case ALPHA_RPCC: 
      ALPHA_INS_SET_REGD(ins, ALPHA_GET_REGA(instr));
      ALPHA_INS_SET_REGB(ins, ALPHA_GET_REGB(instr));
      break;
    default:
      ALPHA_INS_SET_REGB(ins, ALPHA_GET_REGB(instr));
      ALPHA_INS_SET_REGA(ins, ALPHA_GET_REGA(instr));
    }
}

void
AlphaDisassembleFp(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{
 
  /* According to the ISR there cannot be a literal operand */

  ALPHA_INS_SET_TYPE(ins, IT_FLT_ALU);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_FP);
  ALPHA_INS_SET_FLAGS(ins, F_FCOMP);
  ALPHA_INS_SET_OPCODE(ins, opcode);

  ALPHA_INS_SET_REGA(ins, ALPHA_GET_REGA(instr));
  ALPHA_INS_SET_REGB(ins, ALPHA_GET_REGB(instr));
  ALPHA_INS_SET_REGD(ins, ALPHA_GET_REGC(instr));

}

void
AlphaDisassembleMemBranch(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{

  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_MBR);
  ALPHA_INS_SET_REGD(ins, ALPHA_GET_REGA(instr));
  ALPHA_INS_SET_REGB(ins, ALPHA_GET_REGB(instr));
  ALPHA_INS_SET_TYPE(ins, IT_BRANCH);

  /* Branch pred hint bits */
  ALPHA_INS_SET_IMMEDIATE(ins, instr & 0x3FFF); 

}

/* Disassemble PAL format instruction.  Opcode followed by a function code */

void
AlphaDisassemblePal(t_alpha_ins * ins, t_uint32 instr, t_uint16 opcode)
{

  ALPHA_INS_SET_OPCODE(ins, opcode);
  ALPHA_INS_SET_FORMAT(ins, ALPHA_ITYPE_PAL);
  ALPHA_INS_SET_TYPE(ins, IT_SWI);
  ALPHA_INS_SET_PAL_FUNCTION(ins, alpha_opcode_table[opcode].fcode);

}
