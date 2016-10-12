/* Individual functions to assemble instructions.
 * We assemble instructions according to their format:
 * 1) PAL format.
 * 2) Memory format.
 * 3) Memory type branches (i.e. jsr, jmp, ret, jsr_coroutine).
 * 4) Memory format with a function code (e.g. RPCC, ECB, MB etc)
 * 5) Branch format (includes FP branches instructions and regular 
      conditional branches).
 * 6) Operate format (regular operate instructions such as AND, OR, BIS etc).
 * 7) Floating Point format.
 *
 * It might be nice to make some macros for the bit twiddling rather than 
 * having it exposed in this source file
 */

#include <diabloalpha.h>


/* Assemble a PALcode type instruction */

void
AlphaAssemblePal(t_alpha_ins * ins, t_uint32 * instr) 
{

  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;
  *instr |= alpha_opcode_table[ALPHA_INS_OPCODE(ins)].fcode;
  
}

void
AlphaAssembleMem(t_alpha_ins * ins, t_uint32 * instr) 
{
  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;

  if(ALPHA_INS_TYPE(ins) == IT_LOAD) 
  {
    /* Destination register */
    *instr |= ALPHA_INS_REGD(ins) << 21;
  }
  else /* A Store instruction */
  {
    *instr |= ALPHA_INS_REGA(ins) << 21;
  }

  *instr |= ALPHA_INS_REGB(ins) << 16;
  *instr |= (ALPHA_INS_IMMEDIATE(ins) & 0xFFFF);

}

void
AlphaAssembleMemBranch(t_alpha_ins * ins, t_uint32 * instr) 
{  
  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;

  *instr |= ALPHA_INS_REGD(ins) << 21;
  *instr |= ALPHA_INS_REGB(ins) << 16;
  *instr |= alpha_opcode_table[ALPHA_INS_OPCODE(ins)].fcode << 14; // check me!
  *instr |= (ALPHA_INS_IMMEDIATE(ins) & 0x3FFF); // check me.

}

void
AlphaAssembleMfc(t_alpha_ins * ins, t_uint32 * instr) 
{
  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;

  switch(ALPHA_INS_OPCODE(ins)) {
    case ALPHA_RPCC:
      *instr |= ALPHA_INS_REGD(ins) << 21;
      *instr |= ALPHA_INS_REGB(ins) << 16;
      break;
    default:  
      *instr |= ALPHA_INS_REGA(ins) << 21;
      *instr |= ALPHA_INS_REGB(ins) << 16;
      break;
  
  }
  
  *instr |= alpha_opcode_table[ALPHA_INS_OPCODE(ins)].fcode;

}

void
AlphaAssembleBranch(t_alpha_ins * ins, t_uint32 * instr) 
{

  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;

	if(ALPHA_INS_OPCODE(ins) == ALPHA_BR || 
	ALPHA_INS_OPCODE(ins) == ALPHA_BSR)
	{
		*instr |= (ALPHA_INS_REGD(ins) << 21);
	}
	else
	{
  	*instr |= (ALPHA_INS_REGA(ins) << 21);
	}

  *instr |= (ALPHA_INS_IMMEDIATE(ins) & 0x1FFFFF);
}

void
AlphaAssembleOpr(t_alpha_ins * ins, t_uint32 * instr) 
{
  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;
  *instr |= ALPHA_INS_REGA(ins) << 21; /* Source Register */

  if(ALPHA_INS_FLAGS(ins) & F_IMM) // Immediate
  {
    *instr |= ALPHA_INS_IMMEDIATE(ins) << 13;
    *instr |= 1 << 12; /* Set bit 12 */
  }
  else
  {
    *instr |= ALPHA_INS_REGB(ins) << 16;
    *instr &= 0xFFFF1FFF; /* Zero SBZ..Should this be done, its not MBZ */
    *instr &= 0xFFFFF7FF; /* Bit 12 = 0 */
  }
  /* Function code */
  *instr |= alpha_opcode_table[ALPHA_INS_OPCODE(ins)].fcode << 5;
  *instr |= ALPHA_INS_REGD(ins);  /* Dest Register    */

}


void
AlphaAssembleFp(t_alpha_ins * ins, t_uint32 * instr) 
{
  *instr = alpha_opcode_table[ALPHA_INS_OPCODE(ins)].opcode << 26;

  *instr |= ALPHA_INS_REGA(ins) << 21;
  *instr |= ALPHA_INS_REGB(ins) << 16;
  *instr |= alpha_opcode_table[ALPHA_INS_OPCODE(ins)].fcode << 5;
  *instr |= ALPHA_INS_REGD(ins);

}
