/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>
#include <string.h>

/* i386_opposite_table {{{ */
const t_i386_opposite i386_opposite_table[]={
  {I386_CONDITION_O,I386_CONDITION_NO},
  {I386_CONDITION_A,I386_CONDITION_BE},
  {I386_CONDITION_AE,I386_CONDITION_B},
  {I386_CONDITION_Z,I386_CONDITION_NZ},
  {I386_CONDITION_NP,I386_CONDITION_P},
  {I386_CONDITION_GE,I386_CONDITION_L},
  {I386_CONDITION_LE,I386_CONDITION_G},
  {I386_CONDITION_NS,I386_CONDITION_S},
  {I386_CONDITION_LOOP,I386_CONDITION_ECXZ},
  {I386_CONDITION_NONE,I386_CONDITION_NONE}
};  
/* }}} */

/** {{{ ins is conditional? */
t_bool I386InsIsConditional(t_i386_ins * ins)
{
  return I386_INS_CONDITION(ins) != I386_CONDITION_NONE;
}
/* }}} */

/** {{{ ins has side effect? */
t_bool I386InsHasSideEffect(t_i386_ins * ins)
{

  if (I386InsIsStore(ins))
    return TRUE;

  if (I386InsIsControlTransfer(ins))
    return TRUE;

  if (I386InsIsSystemInstruction(ins))
    return TRUE;

  /* all instructions that define the stack pointer have a side effect */
  if (RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP))
    return TRUE;

  /* some special cases */
  switch (I386_INS_OPCODE(ins))
  {
    case I386_IN:
    case I386_INSB:
    case I386_INSD:
    case I386_OUT:
    case I386_OUTSB:
    case I386_OUTSD:
    case I386_LEAVE:
    case I386_LDS:
    case I386_LES:
    case I386_LFS:
    case I386_LGS:
    case I386_LSS:
    case I386_FLDCW:
    case I386_FSTCW:
    case I386_FLDENV:
    case I386_FSTENV:
    case I386_FSAVE:
    case I386_FRSTOR:
    case I386_STMXCSR:
    case I386_LDMXCSR:
    case I386_PREFETCH_NTA:
    case I386_PREFETCH_T0:
    case I386_PREFETCH_T1:
    case I386_PREFETCH_T2:
      return TRUE;
    default:
      break;
  }

  /* all instructions that push/pop the fpu stack have a side effect
   * (they change the top-of-stack pointer, which is an invisible and
   * unmodeled register) */
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops ||
      i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pushes)
    return TRUE;

  return FALSE;
} /* }}} */

/** {{{ ins writes to memory? */
t_bool I386InsIsStore(t_i386_ins * ins)
{
  /* first the obvious cases */
  switch (I386_INS_OPCODE(ins))
  {
    case I386_PUSH:
    case I386_PUSHA:
    case I386_PUSHF:
    case I386_ENTER:
    case I386_MOVSB:
    case I386_MOVSD:
    case I386_STOSB:
    case I386_STOSD:
    case I386_OUTSB:
    case I386_OUTSD:
    /* calls store their return address on the stack */
    case I386_CALL:
    case I386_CALLF:
    /* pseudo save, used in instrumentation */
    case I386_PSEUDOSAVE:
      return TRUE;
    default:
      break;
  }

  if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
    return TRUE;
  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE1_DEF))
    if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
      return TRUE;
  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE2_DEF))
    if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
      return TRUE;

  return FALSE;

} /* }}} */

/** {{{ ins reads from memory? */
t_bool I386InsIsLoad(t_i386_ins * ins)
{
  /* instructions in which the memory operation is implicit */
  switch (I386_INS_OPCODE(ins))
  {
    case I386_POP:
    case I386_POPA:
    case I386_POPF:
    case I386_LEAVE:
    case I386_MOVSB:
    case I386_MOVSD:
    case I386_CMPSB:
    case I386_CMPSD:
    case I386_LODSB:
    case I386_LODSD:
      /* return instructions pop the return address off the stack */
    case I386_RET:
    case I386_RETF:
    case I386_IRET:
      /* pseudo load, used in instrumentation */
    case I386_PSEUDOLOAD:
      return TRUE;
      /* lea has a memory operand but doesn't actually perform a load from it */
    case I386_LEA:
      return FALSE;
    default:
      break;
  }

  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
    return TRUE;
  if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
    return TRUE;
  if (I386_INS_HAS_FLAG(ins, I386_IF_DEST_IS_SOURCE))
    if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
      return TRUE;

  return FALSE;

} /* }}} */

/** {{{ ins is control transfer? */
t_bool I386InsIsSystemControlTransfer(t_i386_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
  {
    case I386_INT:
    case I386_INTO:
    case I386_BOUND:
    case I386_UD2:
    case I386_HLT:
    case I386_SYSENTER:
    case I386_SYSEXIT:
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

t_bool I386InsIsRegularControlTransfer(t_i386_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
  {
    case I386_JMP:
    case I386_JMPF:
    case I386_Jcc:
    case I386_JECXZ:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
    case I386_CALL:
    case I386_CALLF:
    case I386_RET:
    case I386_RETF:
    case I386_IRET:
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

t_bool I386InsIsControlTransfer(t_i386_ins * ins)
{
  if (I386InsIsSystemControlTransfer(ins) || I386InsIsRegularControlTransfer(ins))
    return TRUE;

  /* in fact there's no need to see this as control transfer instructions
     if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_REP) || I386_INS_HAS_PREFIX(ins,I386_PREFIX_REPNZ))
    return TRUE; */

  return FALSE;
} /* }}} */

/** {{{ ins is system instruction? */
t_bool I386InsIsSystemInstruction(t_i386_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
  {
    /* system instructions as defined in the i386 architecture reference */
    case I386_LGDT:
    case I386_LIDT:
    case I386_LLDT:
    case I386_SGDT:
    case I386_SIDT:
    case I386_SLDT:
    case I386_LTR:
    case I386_STR:
    case I386_LMSW:
    case I386_SMSW:
    case I386_CLTS:
    case I386_ARPL:
    case I386_LAR:
    case I386_LSL:
    case I386_VERR:
    case I386_VERW:
    case I386_WBINVD:
    case I386_INVD:
    case I386_INVLPG:
    case I386_HLT:
    case I386_RSM:
    case I386_RDMSR:
    case I386_WRMSR:
    case I386_RDPMC:
    case I386_RDTSC:
    case I386_SYSENTER:
    case I386_SYSEXIT:
    /* other instructions with system-wide consequences */
    case I386_CLI:
    case I386_STI:
    case I386_FINIT:
    case I386_FCLEX:
    case I386_WAIT:
    case I386_CPUID:
    case I386_CLFLUSH:
    /* for certainty, add these */
    case I386_CLD:
    case I386_STD:
      return TRUE;
    default:
      {
	/* instructions that use or define system registers are also system instructions */
	t_reg reg;
	for (reg = I386_REG_CR0; reg <= I386_REG_DR7; reg++)
	  if (RegsetIn(I386_INS_REGS_DEF(ins),reg) || RegsetIn(I386_INS_REGS_USE(ins),reg))
	    return TRUE;
      }
      return FALSE;
  }
  return FALSE;
} /* }}} */

/** {{{ clear the current contents of the instruction */
void I386ClearIns(t_i386_ins * ins)
{
  if (I386_INS_DEST(ins)) memset(I386_INS_DEST(ins), 0, sizeof(t_i386_operand));
  else I386_INS_SET_DEST(ins, Calloc(1, sizeof(t_i386_operand)));
  if (I386_INS_SOURCE1(ins)) memset(I386_INS_SOURCE1(ins), 0, sizeof(t_i386_operand));
  else I386_INS_SET_SOURCE1(ins, Calloc(1, sizeof(t_i386_operand)));
  if (I386_INS_SOURCE2(ins)) memset(I386_INS_SOURCE2(ins), 0, sizeof(t_i386_operand));
  else I386_INS_SET_SOURCE2(ins, Calloc(1, sizeof(t_i386_operand)));

  I386_INS_SET_FLAGS(ins, 0x0);
  I386_INS_SET_PREFIXES(ins, 0x0);
  I386_INS_SET_CONDITION(ins, I386_CONDITION_NONE);
  I386_INS_SET_ATTRIB(ins,  0x0);

  while (I386_INS_REFERS_TO(ins))
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(I386_INS_BBL(ins)))), RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)));
  {                                                 
    t_i386_operand * operand;                       
    I386_INS_FOREACH_OP(ins,operand)                
    {	                                              
      I386_OP_BASE(operand)=I386_REG_NONE;          
      I386_OP_INDEX(operand)=I386_REG_NONE;         
    }                                               
  }                                                 
} /* }}}*/

/** {{{ set some generic characteristics of the instruction */
void I386SetGenericInsInfo(t_i386_ins * ins)
{
  I386_INS_SET_TYPE(ins,  i386_opcode_table[I386_INS_OPCODE(ins)].type);
  I386_INS_SET_CSIZE(ins,  I386InsGetSize(ins));
  I386_INS_SET_REGS_USE(ins,   I386InsUsedRegisters(ins));
  I386_INS_SET_REGS_DEF(ins,   I386InsDefinedRegisters(ins));
  if (I386_INS_CONDITION(ins) != I386_CONDITION_NONE)
    I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_CONDITIONAL);
  DiabloBrokerCall("SmcInitInstruction",ins);
}
/* }}} */

/** {{{ make CDQ */
void I386InstructionMakeCDQ(t_i386_ins * ins,  t_reg dest, t_reg src)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_CDQ);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  I386SetGenericInsInfo(ins);
 } /* }}} */

 /** {{{ make CDQ */
void I386InstructionMakeIDiv(t_i386_ins * ins,  t_reg dest, t_reg src)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_IDIV);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  I386SetGenericInsInfo(ins);
 } /* }}} */

/** {{{ make noop */
void I386InstructionMakeNoop(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_NOP);
  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make noop */
void I386InstructionMakeInt3(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_INT3);
  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make call */
void I386InstructionMakeCall(t_i386_ins * ins)
{
  t_i386_operand * op;

  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_CALL);
  op = I386_INS_SOURCE1(ins);
  I386_OP_TYPE(op) = i386_optype_imm;
  I386_OP_IMMEDSIZE(op) = 4;
  I386_OP_IMMEDIATE(op) = 0;

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make return */
void I386InstructionMakeReturn(t_i386_ins * ins)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_RET);

  I386SetGenericInsInfo(ins);
 } /* }}} */

/** {{{ make data */
void I386InstructionMakeData(t_i386_ins * ins)
{
  I386ClearIns(ins);
  
  I386_INS_SET_OPCODE(ins, I386_DATA);
  I386_INS_SET_DATA(ins, 0x00);
  
  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make jump */
void I386InstructionMakeJump(t_i386_ins * ins)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_JMP);
  I386OpSetImm(I386_INS_SOURCE1(ins),0,4);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make jump mem */
void I386InstructionMakeJumpMem(t_i386_ins * ins, t_reg base, t_reg index)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_JMP);

  if (base == I386_REG_NONE)
  {
    if(index == I386_REG_NONE)
      I386OpSetMem(I386_INS_SOURCE1(ins), 0x0, I386_REG_NONE,I386_REG_NONE, 0,4 );
    else
      I386OpSetMem(I386_INS_SOURCE1(ins), 0x0, I386_REG_NONE,index, I386_SCALE_4 ,4 );
  }
  else
  {
    if(index == I386_REG_NONE)
      I386OpSetMem(I386_INS_SOURCE1(ins), 0x0, base,I386_REG_NONE, 0,4 );
    else
      I386OpSetMem(I386_INS_SOURCE1(ins), 0x0, base,index, 0,4 );
  }

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make jump reg */
void I386InstructionMakeJumpReg(t_i386_ins * ins, t_reg src)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_JMP);

  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);

  I386SetGenericInsInfo(ins);
} /* }}} */

void I386InstructionMakeBt(t_i386_ins * ins, t_reg src1, t_uint8 imm)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_BT);
  I386OpSetReg(I386_INS_SOURCE1(ins), src1, i386_regmode_full32);
  I386OpSetImm(I386_INS_SOURCE2(ins), imm, 1);
  I386SetGenericInsInfo(ins);
}

/** {{{ make conditional jump (also jecxz and loop variants) */
void I386InstructionMakeCondJump(t_i386_ins * ins, t_i386_condition_code condition)
{
  I386ClearIns(ins);

  switch (condition)
  {
    case I386_CONDITION_ECXZ:
      I386_INS_SET_OPCODE(ins, I386_JECXZ);
      break;
    case I386_CONDITION_LOOP:
      I386_INS_SET_OPCODE(ins, I386_LOOP);
      break;
    case I386_CONDITION_LOOPZ:
      I386_INS_SET_OPCODE(ins, I386_LOOPZ);
      break;
    case I386_CONDITION_LOOPNZ:
      I386_INS_SET_OPCODE(ins, I386_LOOPNZ);
      break;
    case I386_CONDITION_NONE:
      FATAL(("cannot make unconditional conditional jump!"));
    default:
      I386_INS_SET_OPCODE(ins, I386_Jcc);
  }
  I386_INS_SET_CONDITION(ins, condition);
  I386OpSetImm(I386_INS_SOURCE1(ins),0,1);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make pop */
void I386InstructionMakePop(t_i386_ins * ins, t_reg reg)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_POP);
  I386OpSetReg(I386_INS_DEST(ins),reg,i386_regmode_full32);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/* {{{ make popf */
void I386InstructionMakePopF(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_POPF);
  I386SetGenericInsInfo(ins);
}
/* }}} */

 /* {{{ make pushf */
void I386InstructionMakePushF(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_PUSHF);
  I386SetGenericInsInfo(ins);
} /* }}} */

/* {{{ make popa */
void I386InstructionMakePopA(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_POPA);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/* {{{ make pusha */
void I386InstructionMakePushA(t_i386_ins * ins)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_PUSHA);
  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make push */
void I386InstructionMakePush(t_i386_ins * ins, t_reg reg, t_uint32 immval)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_PUSH);
  if (reg == I386_REG_NONE)
    I386OpSetImm(I386_INS_SOURCE1(ins),immval,4);
  else
    I386OpSetReg(I386_INS_SOURCE1(ins),reg,i386_regmode_full32);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make push memory operand */
void I386InstructionMakePushMem(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_PUSH);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,4);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" or "opc imm, dest" with dest a register */
void I386InstructionMakeArithmetic(t_i386_ins * ins, t_i386_opcode opc, t_reg dest, t_reg src, t_uint32 imm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opc);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" or "opc imm, dest" with dest a memory location */
void I386InstructionMakeArithmeticToMem(t_i386_ins * ins, t_i386_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opc);
  I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,4);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" where src is a memory location */
void I386InstructionMakeArithmeticFromMem(t_i386_ins * ins, t_i386_opcode opc, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opc);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,4);

  /* these instructions have in common that they all use their destination
   * operand */
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an instruction from reg/imm (imm8) to reg */
void I386InstructionMakeRM32IMM8(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm, t_i386_opcode opcode)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,1);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an instruction from reg/imm (imm32) to reg */
void I386InstructionMakeRM32IMM32(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm, t_i386_opcode opcode)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opcode);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a mov instruction from reg/imm to reg */
void I386InstructionMakeMovToReg(t_i386_ins * ins, t_reg dest, t_reg src, t_uint32 imm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a mov instruction to memory + len*/
void I386InstructionMakeMovToMemLen(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm, t_uint32 imm_len)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,imm_len);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,imm_len);

  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction to memory +RegMode*/
void I386InstructionMakeMovToMem8bits(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,1);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_lo8);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction to memory */
void I386InstructionMakeMovToMem(t_i386_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint32 imm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,4);
  if (src != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE1(ins),imm,4);

  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a rep movsb instruction to memory */
void I386InstructionMakeRepMovSB(t_i386_ins * ins)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOVSB);
  I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_REP);

  I386OpSetMem(I386_INS_SOURCE1(ins),0, I386_REG_ESI,I386_REG_NONE,0,1);
  I386OpSetMem(I386_INS_DEST(ins),0, I386_REG_EDI,I386_REG_NONE,0,1);
  
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a rep movsd instruction to memory */
void I386InstructionMakeRepMovSD(t_i386_ins * ins)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_MOVSD);
  I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_REP);

  I386OpSetMem(I386_INS_SOURCE1(ins),0, I386_REG_ESI,I386_REG_NONE,0,4);
  I386OpSetMem(I386_INS_DEST(ins),0, I386_REG_EDI,I386_REG_NONE,0,4);
  
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory +len*/
void I386InstructionMakeMovFromMemLen(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 imm_len)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,imm_len);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory +RegMode*/
void I386InstructionMakeMovFromMem8bits(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_lo8);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,1);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory */
void I386InstructionMakeMovFromMem(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_MOV);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,4);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a leave instruction */
void I386InstructionMakeLeave(t_i386_ins * ins) {
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_LEAVE);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a lea instruction */
void I386InstructionMakeLea(t_i386_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_LEA);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,0);

  I386SetGenericInsInfo(ins);
}
/* }}} */

 /** {{{ make conditional mov (CMOVcc) */
void I386InstructionMakeCondMov(t_i386_ins * ins, t_reg src, t_reg dst, t_i386_condition_code condition)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_CMOVcc);
      
  I386_INS_SET_CONDITION(ins, condition);
  
  I386OpSetReg(I386_INS_SOURCE1(ins),src,i386_regmode_full32);
  
  I386OpSetReg(I386_INS_DEST(ins),dst,i386_regmode_full32);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ compare register to register or immediate */
void I386InstructionMakeCmp(t_i386_ins * ins, t_reg reg, t_reg cmpreg, t_uint32 cmpimm)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_CMP);
  I386OpSetReg(I386_INS_SOURCE1(ins),reg,i386_regmode_full32);
  if (cmpreg == I386_REG_NONE)
    I386OpSetImm(I386_INS_SOURCE2(ins),cmpimm,4);
  else
    I386OpSetReg(I386_INS_SOURCE2(ins),cmpreg,i386_regmode_full32);

  I386SetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a setcc instruction */
void I386InstructionMakeSetcc(t_i386_ins * ins, t_i386_condition_code cond, t_uint32 offset, t_reg base, t_reg index, int scale, t_bool memop)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, I386_SETcc);
  I386_INS_SET_CONDITION(ins, cond);
  if (memop)
    I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,1);
  else
    I386OpSetReg(I386_INS_DEST(ins),base,i386_regmode_full32);

  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a pseudo call (used in instrumentation) */
void I386InstructionMakePseudoCall(t_i386_ins * ins, t_function * to)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_PSEUDOCALL);
  /* this is really ugly but we don't care. pseudo calls are hackish as hell anyway */
  FATAL(("faulty pointer conversion. Fix if you want to use this ever again"));
  /*I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) = (t_uint32) to;*/
  /* used to be an lvalue cast to t_function * */

  I386_INS_SET_CSIZE(ins,  AddressNew32(0));
  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
  I386_INS_SET_TYPE(ins,  IT_CALL);
}
/* }}} */

/** {{{ make a pseudo save (used in instrumentation) */
void I386InstructionMakePseudoSave(t_i386_ins * ins, t_reg reg)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_PSEUDOSAVE);
  I386_INS_SET_TYPE(ins,  IT_PSEUDO_SAVE);

  I386OpSetReg(I386_INS_SOURCE1(ins),reg,i386_regmode_full32);
  I386_INS_SET_CSIZE(ins, AddressNew32(6));
  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
}
/* }}} */

/** {{{ make a pseudo load (used in instrumentation) */
void I386InstructionMakePseudoLoad(t_i386_ins * ins, t_reg reg)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_PSEUDOLOAD);
  I386_INS_SET_TYPE(ins,  IT_PSEUDO_LOAD);

  I386OpSetReg(I386_INS_DEST(ins),reg,i386_regmode_full32);
  I386_INS_SET_CSIZE(ins, AddressNew32(6));
  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));
}
/* }}} */

/** {{{ make a simple no-argument instruction with a given opcode */
void I386InstructionMakeSimple(t_i386_ins * ins, t_i386_opcode opc)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opc);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make LAHF/SAHF instruction */
void I386InstructionMakeLSahf(t_i386_ins * ins, t_i386_opcode opc)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opc);
  if (opc == I386_LAHF)
    I386OpSetReg(I386_INS_DEST(ins),I386_REG_EAX,i386_regmode_hi8);
  else if (opc == I386_SAHF)
    I386OpSetReg(I386_INS_SOURCE1(ins),I386_REG_EAX,i386_regmode_hi8);
  else 
    FATAL(("wrong opcode"));
  I386SetGenericInsInfo(ins);
}
/* }}}*/

/** {{{ make FSAVE/FRSTOR instruction */
void I386InstructionMakeFSaveRstor(t_i386_ins * ins, t_i386_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  I386ClearIns(ins);

  I386_INS_SET_OPCODE(ins, opc);
  if (opc == I386_FSAVE)
    I386OpSetMem(I386_INS_DEST(ins),offset,base,index,scale,0);
  else if (opc == I386_FRSTOR)
    I386OpSetMem(I386_INS_SOURCE1(ins),offset,base,index,scale,0);
  else
    FATAL(("wrong opcode"));

  I386SetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make inc/dec instruction */
void I386InstructionMakeIncDec(t_i386_ins * ins, t_i386_opcode opc, t_reg dest)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, opc);
  I386OpSetReg(I386_INS_DEST(ins),dest,i386_regmode_full32);
  I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
  I386SetGenericInsInfo(ins);
}
/* }}}*/

/** {{{ make test instruction */
void I386InstructionMakeTest(t_i386_ins * ins, t_reg src1, t_reg src2, t_uint32 imm)
{
  I386ClearIns(ins);
  I386_INS_SET_OPCODE(ins, I386_TEST);
  I386OpSetReg(I386_INS_SOURCE1(ins),src1,i386_regmode_full32);
  if (src2 != I386_REG_NONE)
    I386OpSetReg(I386_INS_SOURCE2(ins),src2,i386_regmode_full32);
  else
    I386OpSetImm(I386_INS_SOURCE2(ins),imm,4);
  I386SetGenericInsInfo(ins);
}
/* }}} */

/* {{{ make indirect call/jump/... direct */
void I386InstructionMakeDirect(t_i386_ins * ins)
{
  t_i386_condition_code cond = I386_INS_CONDITION(ins);
  I386ClearIns(ins);
  I386_INS_SET_CONDITION(ins, cond);
  I386OpSetImm(I386_INS_SOURCE1(ins),0,4);
  I386SetGenericInsInfo(ins);
} /* }}} */


/* I386IsUseException {{{ */
t_bool I386IsUseException(t_i386_ins * ins)
{
  switch(I386_INS_OPCODE(ins)){
    case I386_IMULexp2:
      if(I386_OP_IMMEDIATE(I386_INS_SOURCE2(ins))!=0)
	return FALSE;
    case I386_SUB:
    case I386_XOR:
      return 
	(
	 (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg && I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
	 &&
	 (I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_OP_BASE(I386_INS_DEST(ins)))
	);
    case I386_AND:
      return
	(
	 (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg && I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	 &&
	 (I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins))==0)
	);
    default:
      return FALSE;
  }
}
/* }}} */

/* {{{ I386InsUsedRegisters */
t_regset I386InsUsedRegisters(t_i386_ins * ins)
{
  t_regset use = NullRegs;
  t_i386_operand * op;

  if (I386_INS_TYPE(ins) == IT_DATA)
    return use;

  /* {{{ used condition flags */
  if (I386_INS_CONDITION(ins) == I386_CONDITION_NONE)
    RegsetSetUnion(use, i386_opcode_table[I386_INS_OPCODE(ins)].cf_used);
  else
  {
    switch (I386_INS_CONDITION(ins))
    {
      case I386_CONDITION_O:
      case I386_CONDITION_NO:
	RegsetSetAddReg(use, I386_CONDREG_OF);
	break;
      case I386_CONDITION_B:
      case I386_CONDITION_AE:
	RegsetSetAddReg(use, I386_CONDREG_CF);
	break;
      case I386_CONDITION_Z:
      case I386_CONDITION_NZ:
	RegsetSetAddReg(use, I386_CONDREG_ZF);
	break;
      case I386_CONDITION_BE:
      case I386_CONDITION_A:
	RegsetSetAddReg(use, I386_CONDREG_CF);
	RegsetSetAddReg(use, I386_CONDREG_ZF);
	break;
      case I386_CONDITION_S:
      case I386_CONDITION_NS:
	RegsetSetAddReg(use, I386_CONDREG_SF);
	break;
      case I386_CONDITION_P:
      case I386_CONDITION_NP:
	RegsetSetAddReg(use, I386_CONDREG_PF);
	break;
      case I386_CONDITION_L:
      case I386_CONDITION_GE:
	RegsetSetAddReg(use, I386_CONDREG_SF);
	RegsetSetAddReg(use, I386_CONDREG_OF);
	break;
      case I386_CONDITION_LE:
      case I386_CONDITION_G:
	RegsetSetAddReg(use, I386_CONDREG_SF);
	RegsetSetAddReg(use, I386_CONDREG_OF);
	RegsetSetAddReg(use, I386_CONDREG_ZF);
	break;
      case I386_CONDITION_LOOPZ:
      case I386_CONDITION_LOOPNZ:
	RegsetSetAddReg(use, I386_CONDREG_ZF);
	break;
      case I386_CONDITION_ECXZ:
      case I386_CONDITION_LOOP:
	/* no condition flags used */
	break;
      default:
	FATAL(("unexpected condition value for @I",ins));
    }
  } /* }}} */

  /* {{{ source operands */
  op = I386_INS_SOURCE1(ins);
  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      if(!I386IsUseException(ins))
      RegsetSetAddReg(use, I386_OP_BASE(op));
      break;
    case i386_optype_mem:
      if (I386_OP_BASE(op) != I386_REG_NONE)
	RegsetSetAddReg(use, I386_OP_BASE(op));
      if (I386_OP_INDEX(op) != I386_REG_NONE)
	RegsetSetAddReg(use, I386_OP_INDEX(op));
      break;
    default:
      /* keep the compiler happy */
      break;
  }
  op = I386_INS_SOURCE2(ins);
  switch (I386_OP_TYPE(op))
  {
    case i386_optype_reg:
      RegsetSetAddReg(use, I386_OP_BASE(op));
      break;
    case i386_optype_mem:
      if (I386_OP_BASE(op) != I386_REG_NONE)
	RegsetSetAddReg(use, I386_OP_BASE(op));
      if (I386_OP_INDEX(op) != I386_REG_NONE)
	RegsetSetAddReg(use, I386_OP_INDEX(op));
      break;
    default:
      /* keep the compiler happy */
      break;
  }
  /* instructions where the destination operand is also used as a source operand */
  if (I386_INS_HAS_FLAG(ins,I386_IF_DEST_IS_SOURCE))
  {
    op = I386_INS_DEST(ins);
    switch (I386_OP_TYPE(op))
    {
      case i386_optype_reg:
	if(!I386IsUseException(ins))
	RegsetSetAddReg(use, I386_OP_BASE(op));
	break;
      case i386_optype_mem:
	if (I386_OP_BASE(op) != I386_REG_NONE)
	  RegsetSetAddReg(use, I386_OP_BASE(op));
	if (I386_OP_INDEX(op) != I386_REG_NONE)
	  RegsetSetAddReg(use, I386_OP_INDEX(op));
	break;
      default:
	/* keep the compiler happy */
	break;
    }
  }
  /* }}} */

  /* {{{ the destination operand */
  op = I386_INS_DEST(ins);
  if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    if (I386_OP_BASE(op) != I386_REG_NONE)
      RegsetSetAddReg(use, I386_OP_BASE(op));
    if (I386_OP_INDEX(op) != I386_REG_NONE)
      RegsetSetAddReg(use, I386_OP_INDEX(op));
  } /* }}} */

  /* {{{ prefixes */ 
  /* the REP/REPNZ prefixes */
  if (I386_INS_HAS_PREFIX(ins, I386_PREFIX_REP) ||
      I386_INS_HAS_PREFIX(ins, I386_PREFIX_REPNZ))
  {
    RegsetSetAddReg(use, I386_REG_ECX);
    //not always true, if done properly, ZF is not used by REP, but only for REPE REPNE REPZ REPNZ
    //because REP and REPZ are the same for diablo, we cannot easily make the disctinction -> conservative behavior.
    RegsetSetAddReg(use, I386_CONDREG_ZF);
  }
  
  /* used segment registers: instructions always use the CS register, because this register
   * is used by the processor to determine from where it has to load the instruction. Instructions
   * that have memory operands usually get them from the data segment, so these instructions also 
   * the DS register. However, this can be overridden using the segment override prefix. The string
   * instructions also implicitly use the ES register (all of this is primarily of importance for 
   * kernel level code, not for user level code) */
#if 1
  if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_CS_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_CS);
  else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_DS_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_DS);
  else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_ES_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_ES);
  else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_FS_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_FS);
  else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_GS_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_GS);
  else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_SS_OVERRIDE))
    RegsetSetAddReg(use, I386_REG_SS);
  else
  {
    t_i386_operand * op;
    /* instructions with memory operands use the DS register, except when the 
     * reference is relative to the %e{b,s}p register */
    op = I386_INS_DEST(ins);
    if (I386_OP_TYPE(op) == i386_optype_mem)
    {
      if (I386_OP_BASE(op) == I386_REG_EBP || I386_OP_BASE(op) == I386_REG_ESP)
	RegsetSetAddReg(use, I386_REG_SS);
      else
	RegsetSetAddReg(use, I386_REG_DS);
    }
    op = I386_INS_SOURCE1(ins);
    if (I386_OP_TYPE(op) == i386_optype_mem)
    {
      if (I386_OP_BASE(op) == I386_REG_EBP || I386_OP_BASE(op) == I386_REG_ESP)
	RegsetSetAddReg(use, I386_REG_SS);
      else
	RegsetSetAddReg(use, I386_REG_DS);
    }
    op = I386_INS_SOURCE2(ins);
    if (I386_OP_TYPE(op) == i386_optype_mem)
    {
      if (I386_OP_BASE(op) == I386_REG_EBP || I386_OP_BASE(op) == I386_REG_ESP)
	RegsetSetAddReg(use, I386_REG_SS);
      else
	RegsetSetAddReg(use, I386_REG_DS);
    }
  }
  
  if (I386_INS_OPCODE(ins) == I386_MOVSB || I386_INS_OPCODE(ins) == I386_MOVSD ||
      I386_INS_OPCODE(ins) == I386_SCASB || I386_INS_OPCODE(ins) == I386_SCASD ||
      I386_INS_OPCODE(ins) == I386_STOSB || I386_INS_OPCODE(ins) == I386_STOSD ||
      I386_INS_OPCODE(ins) == I386_CMPSB || I386_INS_OPCODE(ins) == I386_CMPSD ||
      I386_INS_OPCODE(ins) == I386_INSB  || I386_INS_OPCODE(ins) == I386_INSD  )
    RegsetSetAddReg(use, I386_REG_ES);

  if (I386_INS_OPCODE(ins) == I386_PUSH || I386_INS_OPCODE(ins) == I386_POP ||
      I386_INS_OPCODE(ins) == I386_PUSHA || I386_INS_OPCODE(ins) == I386_POPA ||
      I386_INS_OPCODE(ins) == I386_PUSHF || I386_INS_OPCODE(ins) == I386_POPF )
    RegsetSetAddReg(use, I386_REG_SS);
#endif
  /* }}}*/
  
  /* {{{ partially defined registers */
  /* if the destination of the instruction is a partial register,
   * the destination register is also used (to preserve the 
   * unaffected bits of the register) */
  op = I386_INS_DEST(ins);
  
  if (I386_OP_TYPE(op) == i386_optype_reg && 
      I386IsGeneralPurposeReg(I386_OP_BASE(op)) && 
      I386_OP_REGMODE(op) != i386_regmode_full32)
  {
    RegsetSetAddReg(use,I386_OP_BASE(op));
  }

  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE1_DEF))
  {
    op = I386_INS_SOURCE1(ins);
    if (I386_OP_TYPE(op) == i386_optype_reg && 
	I386IsGeneralPurposeReg(I386_OP_BASE(op)) && 
	I386_OP_REGMODE(op) != i386_regmode_full32)
    {
      RegsetSetAddReg(use,I386_OP_BASE(op));
    }
  }
  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE2_DEF))
  {
    op = I386_INS_SOURCE2(ins);
    if (I386_OP_TYPE(op) == i386_optype_reg && 
	I386IsGeneralPurposeReg(I386_OP_BASE(op)) && 
	I386_OP_REGMODE(op) != i386_regmode_full32)
    {
      RegsetSetAddReg(use,I386_OP_BASE(op));
    }
  }
  /* }}} */

  /* {{{ fp instructions that push or pop the fp stack */
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops != 0)
  {
    int i;
    for (i = 0; i < 8-i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops; i++)
      RegsetSetAddReg(use, I386_REG_ST0 + i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops + i);
  }
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pushes != 0)
  {
    int i;
    for (i = 0; i < 8-i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pushes; i++)
      RegsetSetAddReg(use, I386_REG_ST0 + i);
  }
  /* }}} */

  /* special cases */
  switch (I386_INS_OPCODE(ins))
  {
    /* {{{ push and pop implicitly use the stack pointer */
    case I386_PUSH:
    case I386_PUSHA:
    case I386_PUSHF:
    case I386_POP:
    case I386_POPA:
    case I386_POPF:
      RegsetSetAddReg(use, I386_REG_ESP);
      /* pusha uses all general purpose registers */
      if (I386_INS_OPCODE(ins) == I386_PUSHA)
      {
	RegsetSetAddReg(use, I386_REG_EAX);
	RegsetSetAddReg(use, I386_REG_EBX);
	RegsetSetAddReg(use, I386_REG_ECX);
	RegsetSetAddReg(use, I386_REG_EDX);
	RegsetSetAddReg(use, I386_REG_ESI);
	RegsetSetAddReg(use, I386_REG_EDI);
	RegsetSetAddReg(use, I386_REG_ESP);
	RegsetSetAddReg(use, I386_REG_EBP);
      }
      break;
      /* }}} */

    /* {{{ call and return instructions use the stack pointer */
    case I386_CALL:
    case I386_CALLF:
    case I386_RET:
    case I386_RETF:
    case I386_IRET:
      RegsetSetAddReg(use, I386_REG_ESP);
      break;
      /* }}} */

    /* {{{ enter and leave instructions use %esp and %ebp */
    case I386_ENTER:
      RegsetSetAddReg(use, I386_REG_ESP);
      RegsetSetAddReg(use, I386_REG_EBP);
      break;
    case I386_LEAVE:
      RegsetSetAddReg(use, I386_REG_EBP);
      break;
      /* }}}*/
    
    /* {{{ div and idiv instructions use edx */
    case I386_DIV:
    case I386_IDIV:
      if (I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32 || 
	  I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_lo16 )
	RegsetSetAddReg(use, I386_REG_EDX);
      break;
      /* }}}*/

    case I386_INT:
    case I386_INTO:
    case I386_INT3:
    case I386_SYSENTER:
    case I386_SYSEXIT:
      /* this modelling is overly conservative for 
       * sysenter and sysexit, but they are too
       * complicated for a decent modelling anyway */
      RegsetSetAddReg(use, I386_REG_EAX);
      RegsetSetAddReg(use, I386_REG_EBX);
      RegsetSetAddReg(use, I386_REG_ECX);
      RegsetSetAddReg(use, I386_REG_EDX);
      RegsetSetAddReg(use, I386_REG_ESI);
      RegsetSetAddReg(use, I386_REG_EDI);
      RegsetSetAddReg(use, I386_REG_ESP);
      RegsetSetAddReg(use, I386_REG_EBP);
      break;

    /* jecxz and loop(z/nz) use the %ecx register */
    case I386_JECXZ:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
      RegsetSetAddReg(use, I386_REG_ECX);
      break;
    /*cmpxchg uses eax*/
    case I386_CMPXCHG:
      RegsetSetAddReg(use, I386_REG_EAX);
      break;

    case I386_FSTCW:
    case I386_FSTENV:
    case I386_FISTP:
    case I386_FIST:
    case I386_FLD1:
    case I386_FLDL2T:
    case I386_FLDL2E:
    case I386_FLDPI:
    case I386_FLDLG2:
    case I386_FLDLN2:
    case I386_FLDZ:
    case I386_FSAVE:
    case I386_FST:
    case I386_FSTP:
    case I386_FRNDINT:
      RegsetSetAddReg(use, I386_REG_FPCW);
      break;
      

    default:
      break;
  }

  return use;
} /*}}} */

/* {{{ I386InsDefinedRegisters */
t_regset I386InsDefinedRegisters(t_i386_ins * ins)
{
  t_regset def = NullRegs;

  if (I386_INS_TYPE(ins) == IT_DATA)
    return def;

  /* defined condition flags */
  RegsetSetUnion(def, i386_opcode_table[I386_INS_OPCODE(ins)].cf_defined);

  /* if the destination operand is a register, this register is, of course, defined */
  if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
    RegsetSetAddReg(def, I386_OP_BASE(I386_INS_DEST(ins)));

  /* source operands that are defined as well */
  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE1_DEF))
    if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg)
      RegsetSetAddReg(def, I386_OP_BASE(I386_INS_SOURCE1(ins)));
  if (I386_INS_HAS_FLAG(ins, I386_IF_SOURCE2_DEF))
    if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_reg)
      RegsetSetAddReg(def, I386_OP_BASE(I386_INS_SOURCE2(ins)));

  /* the REP/REPNZ prefixes */
  if (I386_INS_HAS_PREFIX(ins, I386_PREFIX_REP) ||
      I386_INS_HAS_PREFIX(ins, I386_PREFIX_REPNZ))
  {
    RegsetSetAddReg(def, I386_REG_ECX);
  }

  /* floating point instructions that push or pop the stack
   * redefine the fpu registers */
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops != 0)
  {
    int i;
    for (i = 0; i < 8-i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pops; i++)
      RegsetSetAddReg(def, I386_REG_ST0 + i);
  }
  if (i386_opcode_table[I386_INS_OPCODE(ins)].fpstack_pushes != 0)
  {
    int i;
    for (i = 0; i < 8; i++)
      RegsetSetAddReg(def, I386_REG_ST0 + i);
  }

  /* special cases */
  switch (I386_INS_OPCODE(ins))
  {
    /* {{{ 1. string instructions: these define %edi and %esi */
    case I386_MOVSB: case I386_MOVSD:
    case I386_CMPSB: case I386_CMPSD:
      RegsetSetAddReg(def, I386_REG_EDI);
      RegsetSetAddReg(def, I386_REG_ESI);
      break;
    case I386_LODSB: case I386_LODSD:
    case I386_OUTSB: case I386_OUTSD:
      RegsetSetAddReg(def, I386_REG_ESI);
      break;
    case I386_STOSB: case I386_STOSD:
    case I386_SCASB: case I386_SCASD:
    case I386_INSB:  case I386_INSD:
      RegsetSetAddReg(def, I386_REG_EDI);
      break;
      /* }}}*/

    /* {{{ 2. call and return instructions change the stack pointer */
    case I386_CALL:
    case I386_CALLF:
    case I386_RET:
    case I386_RETF:
    case I386_IRET:
      RegsetSetAddReg(def, I386_REG_ESP);
      break;
      /* }}} */
      
    /* {{{ 3. enter and leave instructions change %esp and %ebp */
    case I386_ENTER:
    case I386_LEAVE:
      RegsetSetAddReg(def, I386_REG_ESP);
      RegsetSetAddReg(def, I386_REG_EBP);
      break;
      /* }}}*/
      
    /* 4. {{{ mul, imul, div and idiv implicitly define %edx
     * if executed with 16-bit or 32-bit operands */
    case I386_MUL:
    case I386_IMUL:
    case I386_DIV:
    case I386_IDIV:
      if (I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_full32 || 
	  I386_OP_REGMODE(I386_INS_DEST(ins)) == i386_regmode_lo16 )
	RegsetSetAddReg(def, I386_REG_EDX);
      break;
      /* }}} */

    /* 5. fstsw modifies the %eax register */
    case I386_FSTSW:
      RegsetSetAddReg(def, I386_REG_EAX);
      break;

    /* 6. {{{ push and pop implicitly define the stack pointer */
    case I386_PUSH:
    case I386_PUSHA:
    case I386_PUSHF:
    case I386_POP:
    case I386_POPA:
    case I386_POPF:
      RegsetSetAddReg(def, I386_REG_ESP);

      /* POPA defines all general purpose registers */
      if (I386_INS_OPCODE(ins) == I386_POPA)
      {
	RegsetSetAddReg(def, I386_REG_EAX);
	RegsetSetAddReg(def, I386_REG_EBX);
	RegsetSetAddReg(def, I386_REG_ECX);
	RegsetSetAddReg(def, I386_REG_EDX);
	RegsetSetAddReg(def, I386_REG_ESI);
	RegsetSetAddReg(def, I386_REG_EDI);
	RegsetSetAddReg(def, I386_REG_ESP);
	RegsetSetAddReg(def, I386_REG_EBP);
      }
      break;
      /* }}} */

    case I386_INT:
    case I386_INTO:
    case I386_INT3:
      /* syscalls define %eax */
      RegsetSetAddReg(def, I386_REG_EAX);
      break;

    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
      /* loop instructions define ecx */
      RegsetSetAddReg(def, I386_REG_ECX);
      break;

    /* calling conventions state that any function call (and consequently all pseudo calls)
     * may change %eax, %ecx and %edx */
    case I386_PSEUDOCALL:
      RegsetSetAddReg(def, I386_REG_EAX);
      RegsetSetAddReg(def, I386_REG_ECX);
      RegsetSetAddReg(def, I386_REG_EDX);
      break;
    case I386_CMPXCHG:
      RegsetSetAddReg(def, I386_REG_EAX);
      break;

    case I386_CPUID:
      RegsetSetAddReg(def, I386_REG_EAX);
      RegsetSetAddReg(def, I386_REG_EBX);
      RegsetSetAddReg(def, I386_REG_ECX);
      RegsetSetAddReg(def, I386_REG_EDX);
      break;

    case I386_FLDCW:
    case I386_FLDENV:
    case I386_FRSTOR:
      RegsetSetAddReg(def, I386_REG_FPCW);


    default:
      break;
  }
  return def;
} /* }}} */

t_address I386InsGetSize(t_i386_ins * ins)
{
  t_uint8 buf[15];
  t_address ret;
  t_uint32 len;
  len = I386AssembleIns(ins,buf);
  ret=AddressNew32(len);
  return ret;
}

/** {{{ Get first relocated operand */
t_i386_operand * I386InsGetFirstRelocatedOp(t_i386_ins * ins)
{
  if (I386_OP_FLAGS(I386_INS_DEST(ins)) & I386_OPFLAG_ISRELOCATED)
    return I386_INS_DEST(ins);
  else if (I386_OP_FLAGS(I386_INS_SOURCE1(ins)) & I386_OPFLAG_ISRELOCATED)
    return I386_INS_SOURCE1(ins);
  else if (I386_OP_FLAGS(I386_INS_SOURCE2(ins)) & I386_OPFLAG_ISRELOCATED)
    return I386_INS_SOURCE2(ins);
//  else FATAL(("Could not find a relocated operand"));
  return NULL;
} /* }}} */

/** {{{ Get second relocated operand */
t_i386_operand * I386InsGetSecondRelocatedOp(t_i386_ins * ins)
{
  t_bool foundfirst = FALSE;

  if (I386_OP_FLAGS(I386_INS_DEST(ins)) & I386_OPFLAG_ISRELOCATED)
    foundfirst = TRUE;

  if (I386_OP_FLAGS(I386_INS_SOURCE1(ins)) & I386_OPFLAG_ISRELOCATED)
  {
    if (foundfirst)
      return I386_INS_SOURCE1(ins);
    else
      foundfirst = TRUE;
  }

  if (I386_OP_FLAGS(I386_INS_SOURCE2(ins)) & I386_OPFLAG_ISRELOCATED)
  {
    if (foundfirst)
      return I386_INS_SOURCE2(ins);
    else 
      FATAL(("found only one relocated operand"));
  }

  VERBOSE(0,("@iB\n@I\n",I386_INS_BBL(ins), ins));
  FATAL(("not enough relocated operands"));
  return NULL;
} /* }}} */

/** {{{ Get reloc for operand */
t_reloc * I386GetRelocForOp(t_i386_ins * ins, t_i386_operand * op)
{
  t_i386_operand * ops[3];
  int i;
  t_bool second_relocated = FALSE;
  t_reloc_ref * rr, * rr2, * tmp;

  if (!(I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)) return NULL;

  ops[0] = I386_INS_DEST(ins);
  ops[1] = I386_INS_SOURCE1(ins);
  ops[2] = I386_INS_SOURCE2(ins);
  
  for (i=0; i<3; i++)
  {
    if (ops[i] == op) break;
    if (I386_OP_FLAGS(ops[i]) & I386_OPFLAG_ISRELOCATED)
      second_relocated = TRUE;
  }

  rr = I386_INS_REFERS_TO(ins);
  ASSERT(rr, ("need at least one reloc in this instruction: @I",ins));
  rr2 = RELOC_REF_NEXT(rr);
  ASSERT(!second_relocated || rr2,("need two relocs in this instruction: @I",ins));

  if (rr2 && AddressIsGt(RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)),RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr2))))
  {
    /* swap */
    tmp = rr2;
    rr2 = rr;
    rr = tmp;
  }
  
  if (second_relocated) return RELOC_REF_RELOC(rr2);
  return RELOC_REF_RELOC(rr);
} /* }}} */

/* I386IsSyscallExit {{{ */
t_tristate I386IsSyscallExit(t_i386_ins * ins)
{
  t_uint32 syscallno;
  
  if (I386_INS_OPCODE(ins) != I386_INT) return NO;
  if (I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) != 0x80) return NO;

  syscallno = I386GetSyscallNo(ins);
  /* store this syscall number in the instruction, this might be handy */
  I386_OP_IMMEDIATE(I386_INS_SOURCE2(ins)) = syscallno;
  
  if (syscallno == UINT32_MAX)
    return PERHAPS;
  else if (syscallno == 0x1 || syscallno == 0xfc)
    return YES;

  return NO;
}
/* }}} */

/* I386GetSyscallNo {{{ */
t_uint32 I386GetSyscallNo(t_i386_ins * ins)
{
  t_i386_ins * iter, * use;
  t_reg lookfor = I386_REG_EAX;

  if (I386_INS_OPCODE(ins) != I386_INT) return UINT32_MAX;
  if (I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) != 0x80) return UINT32_MAX;

  /* find the instruction that defines %eax */
  use = ins;
look_for_definition:
  for (iter = I386_INS_IPREV(use); iter; iter = I386_INS_IPREV(iter))
  {
    if (RegsetIn(I386_INS_REGS_DEF(iter), lookfor))
      break;
  }
  if (!iter)
    return UINT32_MAX;

  if (I386_INS_OPCODE(iter) == I386_MOV)
  {
    if (I386_OP_TYPE(I386_INS_SOURCE1(iter)) == i386_optype_imm)
      return I386_OP_IMMEDIATE(I386_INS_SOURCE1(iter));
    else if (I386_OP_TYPE(I386_INS_SOURCE1(iter)) == i386_optype_reg)
    {
      lookfor = I386_OP_BASE(I386_INS_SOURCE1(iter));
      use = iter;
      goto look_for_definition;
    }
  }
  else
    FATAL(("Syscall @iB @I\n eax defined by @I\nIMPLEMENT!",I386_INS_BBL(ins),ins,iter));

  return UINT32_MAX;
}
/* }}} */

void I386OpSetReg(t_i386_operand * op, t_reg reg, t_i386_regmode mode)
{
  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_BASE(op) = reg;
  I386_OP_REGMODE(op) = mode;
  I386_OP_FLAGS(op) = 0x0;
}

void I386OpSetImm(t_i386_operand * op, t_uint32 imm, t_uint32 immedsize)
{
  I386_OP_TYPE(op) = i386_optype_imm;
  I386_OP_IMMEDIATE(op) = imm;
  I386_OP_BASE(op) = I386_REG_NONE;
  I386_OP_IMMEDSIZE(op) = immedsize;
  I386_OP_FLAGS(op) = 0x0;
}

void I386OpSetMem(t_i386_operand * op, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize)
{
  I386_OP_TYPE(op) = i386_optype_mem;
  I386_OP_IMMEDIATE(op) = offset;
  I386_OP_IMMEDSIZE(op) = 4;
  I386_OP_MEMOPSIZE(op) = memopsize;
  I386_OP_BASE(op) = base;
  I386_OP_INDEX(op) = index;
  I386_OP_SCALE(op) = scale;
  I386_OP_FLAGS(op) = 0x0;
}

/* I386InsGetMemLoadOp {{{ */
/* returns the memory operand of an instruction. if there is none, returns NULL */
t_i386_operand * I386InsGetMemLoadOp(t_i386_ins * ins)
{

  if (I386_INS_FLAGS(ins) & I386_IF_DEST_IS_SOURCE)
    if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
      return I386_INS_DEST(ins);

  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
    return I386_INS_SOURCE1(ins);

  if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
    return I386_INS_SOURCE2(ins);

  return NULL;
}
/* }}} */

/* I386InsGetMemStoreOp {{{ */
t_i386_operand * I386InsGetMemStoreOp(t_i386_ins * ins)
{
  if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
    return I386_INS_DEST(ins);

  if (I386_INS_FLAGS(ins) & I386_IF_SOURCE1_DEF)
    if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
      return I386_INS_SOURCE1(ins);
  
  if (I386_INS_FLAGS(ins) & I386_IF_SOURCE2_DEF)
    if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
      return I386_INS_SOURCE2(ins);

  return NULL;
}
/* }}} */

t_bool I386InsIsIndirectCall(t_i386_ins * ins)
{
  return (I386_INS_OPCODE(ins) == I386_CALL) && (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_imm);
}

t_bool I386InsIsUnconditionalBranch(t_i386_ins * ins)
{
  if (I386_INS_OPCODE(ins) == I386_JMP)  return TRUE;
  return FALSE;
}

t_bool I386InsIsProcedureCall(t_i386_ins * ins)
{
  return I386_INS_OPCODE(ins) == I386_CALL;
}

/*! Callback function, called when InsFree is called. Needed to free dynamically
 * allocated fields */

void I386InsCleanup(t_i386_ins * ins)
{
  Free(I386_INS_DEST(ins));
  Free(I386_INS_SOURCE1(ins));
  Free(I386_INS_SOURCE2(ins));
  
  if (I386_INS_AP_ORIGINAL(ins))
  {
    I386InsCleanup(I386_INS_AP_ORIGINAL(ins));
    Free(I386_INS_AP_ORIGINAL(ins));
  }
  return;
}

/*! Callback function, called when InsDup is called. Needed to copy dynamically
 * allocated fields */

void I386InsDupDynamic(t_i386_ins * target, t_i386_ins * source)
{
  I386_INS_SET_DEST(target, Malloc(sizeof(t_i386_operand)));
  I386_INS_SET_SOURCE1(target, Malloc(sizeof(t_i386_operand)));
  I386_INS_SET_SOURCE2(target, Malloc(sizeof(t_i386_operand)));
  memcpy(I386_INS_DEST(target), I386_INS_DEST(source), sizeof(t_i386_operand));
  memcpy(I386_INS_SOURCE1(target), I386_INS_SOURCE1(source), sizeof(t_i386_operand));
  memcpy(I386_INS_SOURCE2(target), I386_INS_SOURCE2(source), sizeof(t_i386_operand));
  
  if (I386_INS_AP_ORIGINAL(source))
  {
    I386_INS_SET_AP_ORIGINAL(target, Malloc(sizeof(t_i386_ins)));
    memcpy(I386_INS_AP_ORIGINAL(target),I386_INS_AP_ORIGINAL(source),sizeof(t_i386_ins));
    I386InsDupDynamic(I386_INS_AP_ORIGINAL(target),I386_INS_AP_ORIGINAL(source));
  }
}

/* I386InsSetOperandFlags {{{ */
void I386InsSetOperandFlags(t_i386_ins * ins)
{
  t_i386_opcode_entry * entry = NULL;
  if (!I386InsIsConditional(ins)) 
  { 
    t_i386_opcode opcode=I386_INS_OPCODE(ins);	  
    void * key = (void *) &opcode;
    t_i386_opcode_he * he = HashTableLookup(i386_opcode_hash, key);
    while (he)
    {
      entry = (t_i386_opcode_entry *) he->entry;
      if (entry->op1check(I386_INS_DEST(ins),entry->op1bm) && entry->op2check(I386_INS_SOURCE1(ins),entry->op2bm) && entry->op3check(I386_INS_SOURCE2(ins),entry->op3bm))
	break;
      he = (t_i386_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    if (entry->usedefpattern & DU)
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);
    else
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) & (~I386_IF_DEST_IS_SOURCE));
    
    if (entry->usedefpattern & S1D)
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_SOURCE1_DEF);
    else
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) & (~I386_IF_SOURCE1_DEF));
    
    if (entry->usedefpattern & S2D)
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_SOURCE2_DEF);
    else
      I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) & (~I386_IF_SOURCE2_DEF));
  }
}
/* }}} */

/* I386OpsAreIdentical {{{ */
t_bool I386OpsAreIdentical(t_i386_operand * op1, t_i386_ins * ins1, t_i386_operand * op2, t_i386_ins * ins2)
{
  t_bool is_relocated = FALSE;

  if (I386_OP_TYPE(op1) != I386_OP_TYPE(op2)) return FALSE;

  if (I386_OP_FLAGS (op1) != I386_OP_FLAGS (op2)) return FALSE;
  if (I386_OP_FLAGS (op1) & I386_OPFLAG_ISRELOCATED)
  {
    t_reloc *rel1, *rel2;
    rel1 = I386GetRelocForOp (ins1, op1);
    rel2 = I386GetRelocForOp (ins2, op2);
    if (RelocCmp (rel1, rel2, FALSE))
      return FALSE;
    is_relocated = TRUE;
  }
  
  switch (I386_OP_TYPE(op1))
  {
    case i386_optype_none:
      return TRUE;
    case i386_optype_imm:
      if (is_relocated) 
	return TRUE; 
      else
	return I386_OP_IMMEDIATE(op1) == I386_OP_IMMEDIATE(op2);
    case i386_optype_reg:
      return I386_OP_BASE(op1) == I386_OP_BASE(op2) &&
	I386_OP_REGMODE(op1) == I386_OP_REGMODE(op2);
    case i386_optype_mem:
      return I386_OP_BASE(op1) == I386_OP_BASE(op2)  &&
	I386_OP_INDEX(op1) == I386_OP_INDEX(op2)  &&
	(is_relocated || I386_OP_IMMEDIATE(op1) == I386_OP_IMMEDIATE(op2)) &&
	I386_OP_SCALE(op1) == I386_OP_SCALE(op2) &&
	I386_OP_MEMOPSIZE(op1) == I386_OP_MEMOPSIZE(op2);
    default:
      break;
  }
  return 
    I386_OP_TYPE(op1)         == I386_OP_TYPE(op2) &&
    I386_OP_BASE(op1)         == I386_OP_BASE(op2)  &&
    I386_OP_INDEX(op1)        == I386_OP_INDEX(op2)  &&
    I386_OP_IMMEDIATE(op1)    == I386_OP_IMMEDIATE(op2) &&
    I386_OP_SEGSELECTOR(op1)  == I386_OP_SEGSELECTOR(op2) &&
    I386_OP_SCALE(op1)        == I386_OP_SCALE(op2) &&
    I386_OP_REGMODE(op1)      == I386_OP_REGMODE(op2) &&
    I386_OP_IMMEDSIZE(op1)    == I386_OP_IMMEDSIZE(op2)  &&
    I386_OP_MEMOPSIZE(op1)    == I386_OP_MEMOPSIZE(op2) &&
    I386_OP_FLAGS(op1)        == I386_OP_FLAGS(op2); 
}
/* }}} */

/* I386InsAreIdentical {{{ */
t_bool I386InsAreIdentical(t_i386_ins * ins1, t_i386_ins * ins2)
{
  if (I386_INS_OPCODE(ins1) != I386_INS_OPCODE(ins2))
    return FALSE;

  /* catch calls and jumps with direct offsets */
  if (I386_INS_OPCODE(ins1) == I386_CALL || I386_INS_OPCODE(ins1) == I386_JMP || I386_INS_OPCODE(ins1) == I386_Jcc || I386_INS_OPCODE(ins1) == I386_JECXZ || I386_INS_OPCODE(ins1) == I386_LOOP || I386_INS_OPCODE(ins1) == I386_LOOPZ || I386_INS_OPCODE(ins1) == I386_LOOPNZ|| I386_INS_OPCODE(ins1) == I386_JMPF || I386_INS_OPCODE(ins1) == I386_CALLF)
  {
    /* For jumps: 
     *
     * - jump type has to be the same (immediate)
     * - if they have relocs, we can have a dynamic call, so we fall through to
     *   the normal case
     * - condition has to be the same
     */
    if (I386_OP_TYPE(I386_INS_SOURCE1(ins1)) == I386_OP_TYPE(I386_INS_SOURCE1(ins2)) &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins1)) == i386_optype_imm &&
        I386_INS_CONDITION(ins1) == I386_INS_CONDITION(ins2) &&
	(!I386_INS_REFERS_TO(ins1)) &&(!I386_INS_REFERS_TO(ins2)) )
    {
      t_cfg_edge * e1, * e2;
      BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins1),e1)
	if (CFG_EDGE_CAT(e1) & (ET_CALL | ET_IPJUMP/* | ET_JUMP */))
	  break;
      BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins2),e2)
	if (CFG_EDGE_CAT(e2) & (ET_CALL | ET_IPJUMP/* | ET_JUMP */))
	  break;
      if (e1 && e2)
      {
	if (CFG_EDGE_TAIL(e1) == CFG_EDGE_TAIL(e2))
	  return TRUE;
	else
	  return FALSE;
      }
    }
  }

  /* catch switch table jumps */
  if (I386_INS_OPCODE(ins1) == I386_JMP)
  {
    t_i386_operand * op1 = I386_INS_SOURCE1(ins1);
    t_i386_operand * op2 = I386_INS_SOURCE1(ins2);

    if (I386_OP_TYPE(op1) == I386_OP_TYPE(op2)
	&& I386_OP_TYPE(op1) == i386_optype_mem
	&& I386_OP_BASE(op1) == I386_OP_BASE(op2)
	&& I386_OP_BASE(op1) == I386_REG_NONE
	&& I386_OP_INDEX(op1) == I386_OP_INDEX(op2)
	&& I386_OP_INDEX(op1) != I386_REG_NONE
	&& I386_OP_SCALE(op1) == I386_OP_SCALE(op2)
	&& I386_OP_SCALE(op1) == I386_SCALE_4
       )
    {
      t_cfg_edge * switch1, * switch2;
      int n1=0, n2=0;
      /* check if both switches have the same number of switch edges */
      BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins1),switch1)
	if (CFG_EDGE_CAT(switch1) == ET_SWITCH)
	  n1++;
      BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins2),switch2)
	if (CFG_EDGE_CAT(switch2) == ET_SWITCH)
	  n2++;
      if (n1 != n2)
	return FALSE;

      if (n1 > 0)
      {
	/* check if all switch edges from switch1 correspond with those of switch 2 */
	BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins1),switch1)
	{
	  if (CFG_EDGE_CAT(switch1) != ET_SWITCH) continue;
	  BBL_FOREACH_SUCC_EDGE(I386_INS_BBL(ins2),switch2)
	    if (CFG_EDGE_CAT(switch2) == ET_SWITCH && CFG_EDGE_SWITCHVALUE(switch2) == CFG_EDGE_SWITCHVALUE(switch1))
	      break;
	  if (!switch2) return FALSE;
	  if (!AddressIsEq(
		AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch1)),I386_INS_OLD_ADDRESS(ins1)),
		AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch2)),I386_INS_OLD_ADDRESS(ins2))
		)
	     )
	    return FALSE;
	}
	return TRUE;
      }
    }
  }
  
  return  
    I386_INS_OPCODE(ins1) == I386_INS_OPCODE(ins2) &&
    /*I386_INS_DATA(ins1)	==I386_INS_DATA(ins2) &&*/
    I386_INS_PREFIXES(ins1) ==	I386_INS_PREFIXES(ins2)	&&
    I386_INS_FLAGS(ins1) == I386_INS_FLAGS(ins2) &&
    I386OpsAreIdentical(I386_INS_DEST(ins1),ins1,I386_INS_DEST(ins2),ins2) &&
    I386OpsAreIdentical(I386_INS_SOURCE1(ins1),ins1,I386_INS_SOURCE1(ins2),ins2) &&
    I386OpsAreIdentical(I386_INS_SOURCE2(ins1),ins1,I386_INS_SOURCE2(ins2),ins2) &&
    I386_INS_CONDITION(ins1) ==	I386_INS_CONDITION(ins2);
}
/* }}} */

/* I386BblFingerprint {{{ */
t_uint32 I386BblFingerprint(t_bbl * bbl)
{
  t_i386_ins * ins;
  t_uint32 return_value = 0;
  if (BBL_NINS(bbl)<3) return 0;
  ins = T_I386_INS(BBL_INS_FIRST(bbl));
  return_value|=I386_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_DEST(ins));
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_SOURCE1(ins));
  return_value<<=3;
  ins = I386_INS_INEXT(ins);
  return_value|=I386_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_DEST(ins));
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_SOURCE1(ins));
  return_value<<=3;
  ins = I386_INS_INEXT(ins);
  return_value|=I386_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_DEST(ins));
  return_value<<=3;
  return_value|=I386_OP_TYPE(I386_INS_SOURCE1(ins));

  return return_value;
}
/* }}} */

/* I386InstructionUnconditionalizer {{{ */
/* unconditionalize conditional instructions of which constant propagation has determined that
 * they will always be executed */
t_bool I386InstructionUnconditionalizer(t_i386_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
  {
    case I386_Jcc:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
    case I386_JECXZ:
      I386_INS_SET_OPCODE(ins, I386_JMP);
      break;
    case I386_CMOVcc:
      I386_INS_SET_OPCODE(ins, I386_MOV);
      break;
      /*TODO SETcc */
    default:
      return FALSE;
  }

  I386_INS_SET_ATTRIB(ins, I386_INS_ATTRIB(ins) & (~IF_CONDITIONAL));
  I386_INS_SET_CONDITION(ins, I386_CONDITION_NONE);
  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));

  return TRUE;
}
/* }}} */

/* I386InvertConditionExistBbl {{{ */
t_bool I386InvertConditionExistBbl(t_bbl * i_bbl)
{
  if(I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(i_bbl)))==I386_Jcc)
  if(I386InvertConditionExist(I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(i_bbl)))))
    return TRUE;
  return FALSE;
}
/* }}} */

/* I386InvertConditionBbl {{{ */
t_bool I386InvertConditionBbl(t_bbl * i_bbl)
{
  if(I386InvertConditionExistBbl(i_bbl))
  {
    I386_INS_SET_CONDITION(T_I386_INS(BBL_INS_LAST(i_bbl)), I386InvertCondition(I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(i_bbl)))));
    return TRUE;
  }
  return FALSE;
}
/* }}} */

/* I386InvertConditionExist {{{ */
t_bool I386InvertConditionExist(t_i386_condition_code test_cond)
{
  t_uint32 i=0;
  for(i=0;i386_opposite_table[i].cond0!=I386_CONDITION_NONE;i++)
    if(i386_opposite_table[i].cond0 == test_cond)
      return TRUE;
  for(i=0;i386_opposite_table[i].cond1!=I386_CONDITION_NONE;i++)
    if(i386_opposite_table[i].cond1 == test_cond)
      return TRUE;
  return FALSE;
}
/* }}} */

/* I386InvertCondition {{{ */
t_i386_condition_code I386InvertCondition(t_i386_condition_code condition)
{
  t_uint32 i=0;
  for(i=0;i386_opposite_table[i].cond0!=I386_CONDITION_NONE;i++)
  {
    if(i386_opposite_table[i].cond0 == condition)
      return i386_opposite_table[i].cond1;
  }
  for(i=0;i386_opposite_table[i].cond1!=I386_CONDITION_NONE;i++)
  {
    if(i386_opposite_table[i].cond1 == condition)
      return i386_opposite_table[i].cond0;
  }
  return I386_CONDITION_NONE;
}
/* }}} */

/* I386InvertBranchBbl {{{ */
t_bool I386InvertBranchBbl(t_bbl * bbl)
{
  t_bbl * target_bbl = NULL, * split_off;
  t_cfg_edge * edge, * s_edge;
  t_i386_ins * ins;
  t_bool ipjump=FALSE;

  if(BBL_INS_LAST(bbl)==NULL)
    return FALSE;

  if(I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))!=I386_Jcc)
    return FALSE;

  if(!I386InvertConditionBbl(bbl))
  {
    VERBOSE(0,("@iB\n",bbl));
    FATAL(("ERROR! No inverse condition (Impossible!)"));
  }

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge,s_edge)
  {
    if(CFG_EDGE_CAT(edge)==ET_FALLTHROUGH)
      CFG_EDGE_SET_CAT(edge,ET_JUMP);
    else if(CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
      CFG_EDGE_SET_CAT(edge,ET_IPJUMP);
    else if(CFG_EDGE_CAT(edge)==ET_JUMP)
    {
      target_bbl=(t_bbl*)CFG_EDGE_TAIL(edge);
      CfgEdgeKill(edge);
    }
    else if(CFG_EDGE_CAT(edge)==ET_IPJUMP)
    {
      ipjump=TRUE;
      target_bbl=(t_bbl*)CFG_EDGE_TAIL(edge);
      CfgEdgeKill(edge);
    }
    else
    {
      VERBOSE(0,("@E\n",edge));
      FATAL(("ERROR! No edge (Impossible!)"));
    }
  }

  split_off=BblNew(BBL_CFG(bbl));
  BblInsertInFunction(split_off,BBL_FUNCTION(bbl));

  ins = T_I386_INS(InsNewForBbl(split_off));
  I386InstructionMakeJump(ins);
  InsAppendToBbl(T_INS(ins), split_off);

  CfgEdgeCreate(BBL_CFG(bbl),bbl,split_off,ET_FALLTHROUGH);
  if(ipjump)
  CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_IPJUMP);
  else
  CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_JUMP);
  
  return TRUE;
}
/* }}} */

/* I386InvertBranchAllBblInFunction {{{ */
t_uint32 I386InvertBranchAllBblInFunction(t_function * fun) 
{
  t_bbl * bbl;
  t_uint32 tel=0;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    if(BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_Jcc)
    {
      tel++;
      if(!I386InvertBranchBbl(bbl))
	FATAL(("Impossible to invert branch!"));
    }
  }
  return tel;
}/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
