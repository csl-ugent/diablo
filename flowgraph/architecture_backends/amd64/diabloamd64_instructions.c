#include <diabloamd64.h>
#include <string.h>

/* amd64_opposite_table {{{ */
const t_amd64_opposite amd64_opposite_table[]={
  {AMD64_CONDITION_A,AMD64_CONDITION_BE},
  {AMD64_CONDITION_AE,AMD64_CONDITION_B},
  {AMD64_CONDITION_Z,AMD64_CONDITION_NZ},
  {AMD64_CONDITION_NP,AMD64_CONDITION_P},
  {AMD64_CONDITION_GE,AMD64_CONDITION_L},
  {AMD64_CONDITION_LE,AMD64_CONDITION_G},
  {AMD64_CONDITION_NS,AMD64_CONDITION_S},
  {AMD64_CONDITION_LOOP,AMD64_CONDITION_RCXZ},
  {AMD64_CONDITION_NONE,AMD64_CONDITION_NONE}
};  
/* }}} */

/** {{{ ins is conditional? */
t_bool Amd64InsIsConditional(t_amd64_ins * ins)
{
  return AMD64_INS_CONDITION(ins) != AMD64_CONDITION_NONE;
}
/* }}} */

/** {{{ ins has side effect? */
t_bool Amd64InsHasSideEffect(t_amd64_ins * ins)
{

  if (Amd64InsIsStore(ins))
    return TRUE;

  if (Amd64InsIsControlTransfer(ins))
    return TRUE;

  if (Amd64InsIsSystemInstruction(ins))
    return TRUE;

  /* all instructions that define the stack pointer have a side effect */
  if (RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP))
    return TRUE;

  /* some special cases */
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_FFREEP:
    case AMD64_IN:
    case AMD64_INSB:
    case AMD64_INSD:
    case AMD64_OUT:
    case AMD64_OUTSB:
    case AMD64_OUTSD:
    case AMD64_LEAVE:
    case AMD64_LDS:
    case AMD64_LES:
    case AMD64_LFS:
    case AMD64_LGS:
    case AMD64_LSS:
    case AMD64_FLDCW:
    case AMD64_FSTCW:
    case AMD64_FLDENV:
    case AMD64_FSTENV:
    case AMD64_FSAVE:
    case AMD64_FRSTOR:
    case AMD64_STMXCSR:
    case AMD64_LDMXCSR:
      return TRUE;
    default:
      break;
  }

  /* all instructions that push/pop the fpu stack have a side effect
   * (they change the top-of-stack pointer, which is an invisible and
   * unmodeled register) */
  if (amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops ||
      amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pushes)
    return TRUE;

  return FALSE;
} /* }}} */

/** {{{ ins writes to memory? */
t_bool Amd64InsIsStore(t_amd64_ins * ins)
{
  /* first the obvious cases */
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_PUSH:
    case AMD64_PUSHA:
    case AMD64_PUSHF:
    case AMD64_ENTER:
    case AMD64_MOVSB:
    case AMD64_MOVSD:
    case AMD64_STOSB:
    case AMD64_STOSD:
    case AMD64_OUTSB:
    case AMD64_OUTSD:
    /* calls store their return address on the stack */
    case AMD64_CALL:
    case AMD64_CALLF:
    /* pseudo save, used in instrumentation */
    case AMD64_PSEUDOSAVE:
      return TRUE;
    default:
      break;
  }

  if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem)
    return TRUE;
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE1_DEF))
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem)
      return TRUE;
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE2_DEF))
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
      return TRUE;

  return FALSE;

} /* }}} */

/** {{{ ins reads from memory? */
t_bool Amd64InsIsLoad(t_amd64_ins * ins)
{
  /* instructions in which the memory operation is implicit */
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_POP:
    case AMD64_POPA:
    case AMD64_POPF:
    case AMD64_LEAVE:
    case AMD64_MOVSB:
    case AMD64_MOVSD:
    case AMD64_CMPSB:
    case AMD64_CMPSD:
    case AMD64_LODSB:
    case AMD64_LODSD:
      /* return instructions pop the return address off the stack */
    case AMD64_RET:
    case AMD64_RETF:
    case AMD64_IRET:
      /* pseudo load, used in instrumentation */
    case AMD64_PSEUDOLOAD:
      return TRUE;
      /* lea has a memory operand but doesn't actually perform a load from it */
    case AMD64_LEA:
      return FALSE;
    default:
      break;
  }

  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem)
    return TRUE;
  if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
    return TRUE;
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_DEST_IS_SOURCE))
    if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem)
      return TRUE;

  return FALSE;

} /* }}} */

/** {{{ ins is control transfer? */
t_bool Amd64InsIsSystemControlTransfer(t_amd64_ins * ins)
{
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_INT:
    case AMD64_INTO:
    case AMD64_BOUND:
    case AMD64_UD2:
    case AMD64_HLT:
    case AMD64_SYSENTER:
    case AMD64_SYSEXIT:
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

t_bool Amd64InsIsRegularControlTransfer(t_amd64_ins * ins)
{
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_JMP:
    case AMD64_JMPF:
    case AMD64_Jcc:
    case AMD64_JRCXZ:
    case AMD64_LOOP:
    case AMD64_LOOPZ:
    case AMD64_LOOPNZ:
    case AMD64_CALL:
    case AMD64_CALLF:
    case AMD64_RET:
    case AMD64_RETF:
    case AMD64_IRET:
      return TRUE;
    default:
      break;
  }
  return FALSE;
}

t_bool Amd64InsIsControlTransfer(t_amd64_ins * ins)
{
  if (Amd64InsIsSystemControlTransfer(ins) || Amd64InsIsRegularControlTransfer(ins))
    return TRUE;

  /* in fact there's no need to see this as control transfer instructions
     if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_REP) || AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_REPNZ))
    return TRUE; */

  return FALSE;
} /* }}} */

/** {{{ ins is system instruction? */
t_bool Amd64InsIsSystemInstruction(t_amd64_ins * ins)
{
  switch (AMD64_INS_OPCODE(ins))
  {
    /* system instructions as defined in the amd64 architecture reference */
    case AMD64_LGDT:
    case AMD64_LIDT:
    case AMD64_LLDT:
    case AMD64_SGDT:
    case AMD64_SIDT:
    case AMD64_SLDT:
    case AMD64_LTR:
    case AMD64_STR:
    case AMD64_LMSW:
    case AMD64_SMSW:
    case AMD64_CLTS:
    case AMD64_LAR:
    case AMD64_LSL:
    case AMD64_VERR:
    case AMD64_VERW:
    case AMD64_WBINVD:
    case AMD64_INVD:
    case AMD64_INVLPG:
    case AMD64_HLT:
    case AMD64_RSM:
    case AMD64_RDMSR:
    case AMD64_WRMSR:
    case AMD64_RDPMC:
    case AMD64_RDTSC:
    case AMD64_SYSENTER:
    case AMD64_SYSEXIT:
    case AMD64_SYSCALL:
    case AMD64_SYSRET:
    /* other instructions with system-wide consequences */
    case AMD64_CLI:
    case AMD64_STI:
    case AMD64_FINIT:
    case AMD64_FCLEX:
    case AMD64_WAIT:
    case AMD64_CPUID:
    /* for certainty, add these */
    case AMD64_CLD:
    case AMD64_STD:
      return TRUE;
    default:
      {
	/* instructions that use or define system registers are also system instructions */
	t_reg reg;
	for (reg = AMD64_REG_CR0; reg <= AMD64_REG_DR7; reg++)
	  if (RegsetIn(AMD64_INS_REGS_DEF(ins),reg) || RegsetIn(AMD64_INS_REGS_USE(ins),reg))
	    return TRUE;
      }
      return FALSE;
  }
  return FALSE;
} /* }}} */

/** {{{ clear the current contents of the instruction */
void ClearIns(t_amd64_ins * ins)
{
  if (AMD64_INS_DEST(ins)) memset(AMD64_INS_DEST(ins), 0, sizeof(t_amd64_operand));
  else AMD64_INS_SET_DEST(ins, Calloc(1, sizeof(t_amd64_operand)));
  if (AMD64_INS_SOURCE1(ins)) memset(AMD64_INS_SOURCE1(ins), 0, sizeof(t_amd64_operand));
  else AMD64_INS_SET_SOURCE1(ins, Calloc(1, sizeof(t_amd64_operand)));
  if (AMD64_INS_SOURCE2(ins)) memset(AMD64_INS_SOURCE2(ins), 0, sizeof(t_amd64_operand));
  else AMD64_INS_SET_SOURCE2(ins, Calloc(1, sizeof(t_amd64_operand)));

  AMD64_INS_SET_FLAGS(ins, 0x0);
  AMD64_INS_SET_PREFIXES(ins, 0x0);
  AMD64_INS_SET_CONDITION(ins, AMD64_CONDITION_NONE);
  AMD64_INS_SET_ATTRIB(ins,  0x0);

  while (AMD64_INS_REFERS_TO(ins))
    RelocTableRemoveReloc(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(AMD64_INS_BBL(ins)))),RELOC_REF_RELOC(AMD64_INS_REFERS_TO(ins)));
  {                                                 
    t_amd64_operand * operand;                       
    AMD64_INS_FOREACH_OP(ins,operand)                
    {	                                              
      AMD64_OP_BASE(operand)=AMD64_REG_NONE;          
      AMD64_OP_INDEX(operand)=AMD64_REG_NONE;         
    }                                               
  }                                                 
} /* }}}*/

/** {{{ set some generic characteristics of the instruction */
void Amd64InsSetGenericInsInfo(t_amd64_ins * ins)
{
  AMD64_INS_SET_TYPE(ins,  amd64_opcode_table[AMD64_INS_OPCODE(ins)].type);
  AMD64_INS_SET_CSIZE(ins,  Amd64InsGetSize(ins));
  AMD64_INS_SET_REGS_USE(ins,   Amd64InsUsedRegisters(ins));
  AMD64_INS_SET_REGS_DEF(ins,   Amd64InsDefinedRegisters(ins));
  if (AMD64_INS_CONDITION(ins) != AMD64_CONDITION_NONE)
    AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) | IF_CONDITIONAL);
  DiabloBrokerCall("SmcInitInstruction",ins);
}
/* }}} */

/** {{{ make noop */
void Amd64InstructionMakeNoop(t_amd64_ins * ins)
{
  ClearIns(ins);
   
  AMD64_INS_SET_OPCODE(ins, AMD64_XCHG);
  Amd64OpSetReg(AMD64_INS_DEST(ins),AMD64_REG_RAX,amd64_regmode_lo32);
  Amd64OpSetReg(AMD64_INS_SOURCE1(ins),AMD64_REG_RAX,amd64_regmode_lo32);
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make noop */
void Amd64InstructionMakeInt3(t_amd64_ins * ins)
{
  ClearIns(ins);
   FATAL(("Making Int3, check"));
   
  AMD64_INS_SET_OPCODE(ins, AMD64_INT3);
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make call */
void Amd64InstructionMakeCall(t_amd64_ins * ins)
{
  t_amd64_operand * op;

  ClearIns(ins);
 
  AMD64_INS_SET_OPCODE(ins, AMD64_CALL);
  op = AMD64_INS_SOURCE1(ins);
  AMD64_OP_TYPE(op) = amd64_optype_imm;
  AMD64_OP_IMMEDSIZE(op) = 4;
  AMD64_OP_IMMEDIATE(op) = 0;

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make return */
void Amd64InstructionMakeReturn(t_amd64_ins * ins)
{
  ClearIns(ins);


  AMD64_INS_SET_OPCODE(ins, AMD64_RET);

  Amd64InsSetGenericInsInfo(ins);
 } /* }}} */

/** {{{ make data */
void Amd64InstructionMakeData(t_amd64_ins * ins)
{
  ClearIns(ins);
  FATAL(("Making Data, check"));
    
  AMD64_INS_SET_OPCODE(ins, AMD64_DATA);
  AMD64_INS_SET_DATA(ins, 0x00);
  
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */


/** {{{ make jump */
void Amd64InstructionMakeJump(t_amd64_ins * ins)
{
  ClearIns(ins);

  AMD64_INS_SET_OPCODE(ins, AMD64_JMP);
  Amd64OpSetImm(AMD64_INS_SOURCE1(ins),0,4);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make jump mem */
void Amd64InstructionMakeJumpMem(t_amd64_ins * ins, t_reg base, t_reg index)
{
  ClearIns(ins);
  FATAL(("Making JumpMem, check"));

  AMD64_INS_SET_OPCODE(ins, AMD64_JMP);

  if (base == AMD64_REG_NONE)
  {
    if(index == AMD64_REG_NONE)
      Amd64OpSetMem(AMD64_INS_SOURCE1(ins), 0x0, AMD64_REG_NONE,AMD64_REG_NONE, 0,4 );
    else
      Amd64OpSetMem(AMD64_INS_SOURCE1(ins), 0x0, AMD64_REG_NONE,index, AMD64_SCALE_4 ,4 );
  }
  else
  {
    if(index == AMD64_REG_NONE)
      Amd64OpSetMem(AMD64_INS_SOURCE1(ins), 0x0, base,AMD64_REG_NONE, 0,4 );
    else
      Amd64OpSetMem(AMD64_INS_SOURCE1(ins), 0x0, base,index, 0,4 );
  }

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make jump reg */
void Amd64InstructionMakeJumpReg(t_amd64_ins * ins, t_reg src)
{
  ClearIns(ins);
  FATAL(("Making JumpReg, check"));


  AMD64_INS_SET_OPCODE(ins, AMD64_JMP);

  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make conditional jump (also jecxz and loop variants) */
void Amd64InstructionMakeCondJump(t_amd64_ins * ins, t_amd64_condition_code condition)
{
  ClearIns(ins);
  FATAL(("Making CondJump, check"));
  
  switch (condition)
  {
    case AMD64_CONDITION_RCXZ:
      AMD64_INS_SET_OPCODE(ins, AMD64_JRCXZ);
      break;
    case AMD64_CONDITION_LOOP:
      AMD64_INS_SET_OPCODE(ins, AMD64_LOOP);
      break;
    case AMD64_CONDITION_LOOPZ:
      AMD64_INS_SET_OPCODE(ins, AMD64_LOOPZ);
      break;
    case AMD64_CONDITION_LOOPNZ:
      AMD64_INS_SET_OPCODE(ins, AMD64_LOOPNZ);
      break;
    case AMD64_CONDITION_NONE:
      FATAL(("cannot make unconditional conditional jump!"));
    default:
      AMD64_INS_SET_OPCODE(ins, AMD64_Jcc);
  }
  AMD64_INS_SET_CONDITION(ins, condition);
  Amd64OpSetImm(AMD64_INS_SOURCE1(ins),0,1);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make pop */
void Amd64InstructionMakePop(t_amd64_ins * ins, t_reg reg)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, AMD64_POP);
  Amd64OpSetReg(AMD64_INS_DEST(ins),reg,amd64_regmode_full64);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/* {{{ make popf */
void Amd64InstructionMakePopF(t_amd64_ins * ins)
{
  ClearIns(ins);
  FATAL(("Making Popf, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_POPF);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

 /* {{{ make pushf */
void Amd64InstructionMakePushF(t_amd64_ins * ins)
{
  ClearIns(ins);
  FATAL(("Making PushF, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_PUSHF);
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/* {{{ make popa */
void Amd64InstructionMakePopA(t_amd64_ins * ins)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, AMD64_POPA);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/* {{{ make pusha */
void Amd64InstructionMakePushA(t_amd64_ins * ins)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, AMD64_PUSHA);
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make push */
void Amd64InstructionMakePush(t_amd64_ins * ins, t_reg reg, t_uint64 immval)
{
  ClearIns(ins);

  AMD64_INS_SET_OPCODE(ins, AMD64_PUSH);
  if (reg == AMD64_REG_NONE)
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),immval,4);
  else
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),reg,amd64_regmode_full64);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make push memory operand */
void Amd64InstructionMakePushMem(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
  FATAL(("Making PushMem, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_PUSH);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,8);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" or "opc imm, dest" with dest a register */
void Amd64InstructionMakeArithmetic(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_reg src, t_uint64 imm)
{

  ClearIns(ins);

  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);

  /* these instructions have in common that they all use their destination
   * operand */
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);

  Amd64InsSetGenericInsInfo(ins);
}

void Amd64InstructionMakeArithmetic32(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_reg src, t_uint64 imm)
{
  ClearIns(ins);
      
  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_lo32);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_lo32);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);
  
  /* these instructions have in common that they all use their destination
   *    * operand */
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" or "opc imm, dest" with dest a memory location */
void Amd64InstructionMakeArithmeticToMem(t_amd64_ins * ins, t_amd64_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm)
{
  ClearIns(ins);
  FATAL(("Making Arit2, check"));

  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,4);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);

  /* these instructions have in common that they all use their destination
   * operand */
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an arithmetic instruction of the form "opc src, dest" where src is a memory location */
void Amd64InstructionMakeArithmeticFromMem(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
  FATAL(("Making Arit3, check"));

  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,4);

  /* these instructions have in common that they all use their destination
   * operand */
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an instruction from reg/imm (imm8) to reg */
void Amd64InstructionMakeRM32IMM8(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm, t_amd64_opcode opcode)
{
  ClearIns(ins);
  FATAL(("Making RM32IMM8, check"));
  
  AMD64_INS_SET_OPCODE(ins, opcode);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,1);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make an instruction from reg/imm (imm32) to reg */
void Amd64InstructionMakeRM32IMM32(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm, t_amd64_opcode opcode)
{
  ClearIns(ins);
  FATAL(("Making RM32IMM32, check"));
  
  AMD64_INS_SET_OPCODE(ins, opcode);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a mov instruction from reg/imm to reg */
void Amd64InstructionMakeMovToReg(t_amd64_ins * ins, t_reg dest, t_reg src, t_uint64 imm)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,8);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a mov instruction to memory + len*/
void Amd64InstructionMakeMovToMemLen(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm, t_uint64 imm_len)
{
  ClearIns(ins);
  FATAL(("Making MovToLen, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,imm_len);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,imm_len);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction to memory +RegMode*/
void Amd64InstructionMakeMovToMem8bits(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm)
{
  ClearIns(ins);
  FATAL(("Making MovToMem8, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,1);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_lo8);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction to memory */
void Amd64InstructionMakeMovToMem(t_amd64_ins * ins, t_uint32 offset, t_reg base, t_reg index, int scale, t_reg src, t_uint64 imm)
{
  ClearIns(ins);
  FATAL(("Making MovToMem, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,4);
  if (src != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE1(ins),imm,4);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a rep movsb instruction to memory */
void Amd64InstructionMakeRepMovSB(t_amd64_ins * ins)
{
  ClearIns(ins);
  FATAL(("Making RepMovSB, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOVSB);
  AMD64_INS_SET_PREFIXES(ins, AMD64_INS_PREFIXES(ins) | AMD64_PREFIX_REP);

  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),0, AMD64_REG_RSI,AMD64_REG_NONE,0,1);
  Amd64OpSetMem(AMD64_INS_DEST(ins),0, AMD64_REG_RDI,AMD64_REG_NONE,0,1);
  
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a rep movsd instruction to memory */
void Amd64InstructionMakeRepMovSD(t_amd64_ins * ins)
{
  ClearIns(ins);
  FATAL(("Making RepMovSD, check"));
  
  AMD64_INS_SET_OPCODE(ins, AMD64_MOVSD);
  AMD64_INS_SET_PREFIXES(ins, AMD64_INS_PREFIXES(ins) | AMD64_PREFIX_REP);

  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),0, AMD64_REG_RSI,AMD64_REG_NONE,0,4);
  Amd64OpSetMem(AMD64_INS_DEST(ins),0, AMD64_REG_RDI,AMD64_REG_NONE,0,4);
  
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory +len*/
void Amd64InstructionMakeMovFromMemLen(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint64 imm_len)
{
  ClearIns(ins);
    
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,imm_len);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory +RegMode*/
void Amd64InstructionMakeMovFromMem8bits(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
  FATAL(("Making MovFromMem8, check"));
    
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_lo8);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,1);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a mov instruction from memory */
void Amd64InstructionMakeMovFromMem(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
  AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,4);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a leave instruction */
void Amd64InstructionMakeLeave(t_amd64_ins * ins) {
  ClearIns(ins);
  FATAL(("Making Leave, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_LEAVE);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a lea instruction */
void Amd64InstructionMakeLea(t_amd64_ins * ins, t_reg dest, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
  FATAL(("Making Lea, check"));

  AMD64_INS_SET_OPCODE(ins, AMD64_LEA);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,0);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

 /** {{{ make conditional mov (CMOVcc) */
void Amd64InstructionMakeCondMov(t_amd64_ins * ins, t_reg src, t_reg dst, t_amd64_condition_code condition)
{
  ClearIns(ins);
  FATAL(("Making CondMov, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_CMOVcc);
      
  AMD64_INS_SET_CONDITION(ins, condition);
  
  Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src,amd64_regmode_full64);
  
  Amd64OpSetReg(AMD64_INS_DEST(ins),dst,amd64_regmode_full64);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ compare register to register or immediate */
void Amd64InstructionMakeCmp(t_amd64_ins * ins, t_reg reg, t_reg cmpreg, t_uint64 cmpimm)
{
  ClearIns(ins);
  FATAL(("Making Cmp, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_CMP);
  Amd64OpSetReg(AMD64_INS_SOURCE1(ins),reg,amd64_regmode_full64);
  if (cmpreg == AMD64_REG_NONE)
    Amd64OpSetImm(AMD64_INS_SOURCE2(ins),cmpimm,4);
  else
    Amd64OpSetReg(AMD64_INS_SOURCE2(ins),cmpreg,amd64_regmode_full64);

  Amd64InsSetGenericInsInfo(ins);
} /* }}} */

/** {{{ make a setcc instruction */
void Amd64InstructionMakeSetcc(t_amd64_ins * ins, t_amd64_condition_code cond, t_uint32 offset, t_reg base, t_reg index, int scale, t_bool memop)
{
  ClearIns(ins);
  FATAL(("Making Setcc, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_SETcc);
  AMD64_INS_SET_CONDITION(ins, cond);
  if (memop)
    Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,1);
  else
    Amd64OpSetReg(AMD64_INS_DEST(ins),base,amd64_regmode_full64);

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make a pseudo call (used in instrumentation) */
void Amd64InstructionMakePseudoCall(t_amd64_ins * ins, t_function * to)
{
  ClearIns(ins);
  FATAL(("Making PseudoCall, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_PSEUDOCALL);
  /* this is really ugly but we don't care. pseudo calls are hackish as hell anyway */
  FATAL(("if you want to use this, fix the next line first"));
  /*AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) = (t_uint32) to;*/
  /* used to be an lvalue cast to t_function * */

  AMD64_INS_SET_CSIZE(ins,  AddressNew64(0));
  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
  AMD64_INS_SET_TYPE(ins,  IT_CALL);
}
/* }}} */

/** {{{ make a pseudo save (used in instrumentation) */
void Amd64InstructionMakePseudoSave(t_amd64_ins * ins, t_reg reg)
{
  ClearIns(ins);
  FATAL(("Making PseudoSave, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_PSEUDOSAVE);
  AMD64_INS_SET_TYPE(ins,  IT_PSEUDO_SAVE);

  Amd64OpSetReg(AMD64_INS_SOURCE1(ins),reg,amd64_regmode_full64);
  AMD64_INS_SET_CSIZE(ins, AddressNew64(6));
  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
}
/* }}} */

/** {{{ make a pseudo load (used in instrumentation) */
void Amd64InstructionMakePseudoLoad(t_amd64_ins * ins, t_reg reg)
{
  ClearIns(ins);
  FATAL(("Making PseudoLoad, check"));
  AMD64_INS_SET_OPCODE(ins, AMD64_PSEUDOLOAD);
  AMD64_INS_SET_TYPE(ins,  IT_PSEUDO_LOAD);

  Amd64OpSetReg(AMD64_INS_DEST(ins),reg,amd64_regmode_full64);
  AMD64_INS_SET_CSIZE(ins, AddressNew64(6));
  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));
}
/* }}} */

/** {{{ make a simple no-argument instruction with a given opcode */
void Amd64InstructionMakeSimple(t_amd64_ins * ins, t_amd64_opcode opc)
{
  ClearIns(ins);
  FATAL(("Making Simple, check"));
  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make LAHF/SAHF instruction */
void Amd64InstructionMakeLSahf(t_amd64_ins * ins, t_amd64_opcode opc)
{
  ClearIns(ins);
  FATAL(("Making LSahf, check"));
  AMD64_INS_SET_OPCODE(ins, opc);
  if (opc == AMD64_LAHF)
    Amd64OpSetReg(AMD64_INS_DEST(ins),AMD64_REG_RAX,amd64_regmode_hi8);
  else if (opc == AMD64_SAHF)
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),AMD64_REG_RAX,amd64_regmode_hi8);
  else 
    FATAL(("wrong opcode"));
  Amd64InsSetGenericInsInfo(ins);
}
/* }}}*/

/** {{{ make FSAVE/FRSTOR instruction */
void Amd64InstructionMakeFSaveRstor(t_amd64_ins * ins, t_amd64_opcode opc, t_uint32 offset, t_reg base, t_reg index, int scale)
{
  ClearIns(ins);
   FATAL(("Making FSaveRstor, check"));

  AMD64_INS_SET_OPCODE(ins, opc);
  if (opc == AMD64_FSAVE)
    Amd64OpSetMem(AMD64_INS_DEST(ins),offset,base,index,scale,0);
  else if (opc == AMD64_FRSTOR)
    Amd64OpSetMem(AMD64_INS_SOURCE1(ins),offset,base,index,scale,0);
  else
    FATAL(("wrong opcode"));

  Amd64InsSetGenericInsInfo(ins);
}
/* }}} */

/** {{{ make inc/dec instruction */
void Amd64InstructionMakeIncDec(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_full64);
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);
  Amd64InsSetGenericInsInfo(ins);
}

void Amd64InstructionMakeIncDec32(t_amd64_ins * ins, t_amd64_opcode opc, t_reg dest)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, opc);
  Amd64OpSetReg(AMD64_INS_DEST(ins),dest,amd64_regmode_lo32);
  AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);
  Amd64InsSetGenericInsInfo(ins);
}

/* }}}*/

/** {{{ make test instruction */
void Amd64InstructionMakeTest(t_amd64_ins * ins, t_reg src1, t_reg src2, t_uint64 imm)
{
  ClearIns(ins);
  
  AMD64_INS_SET_OPCODE(ins, AMD64_TEST);
  Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src1,amd64_regmode_full64);
  if (src2 != AMD64_REG_NONE)
    Amd64OpSetReg(AMD64_INS_SOURCE2(ins),src2,amd64_regmode_full64);
  else
    Amd64OpSetImm(AMD64_INS_SOURCE2(ins),imm,4);
  Amd64InsSetGenericInsInfo(ins);
}

void Amd64InstructionMakeTest32(t_amd64_ins * ins, t_reg src1, t_reg src2, t_uint64 imm)
{
    ClearIns(ins);

    AMD64_INS_SET_OPCODE(ins, AMD64_TEST);
    Amd64OpSetReg(AMD64_INS_SOURCE1(ins),src1,amd64_regmode_lo32);
    if (src2 != AMD64_REG_NONE)
      Amd64OpSetReg(AMD64_INS_SOURCE2(ins),src2,amd64_regmode_lo32);
    else
      Amd64OpSetImm(AMD64_INS_SOURCE2(ins),imm,4);
    Amd64InsSetGenericInsInfo(ins);
}

/* }}} */

/* {{{ make indirect call/jump/... direct */
void Amd64InstructionMakeDirect(t_amd64_ins * ins)
{
  t_amd64_condition_code cond = AMD64_INS_CONDITION(ins);
  ClearIns(ins);
  FATAL(("Making Direct, check"));
  
  AMD64_INS_SET_CONDITION(ins, cond);
  Amd64OpSetImm(AMD64_INS_SOURCE1(ins),0,4);
  Amd64InsSetGenericInsInfo(ins);
} /* }}} */


/* Amd64IsUseException {{{ */
t_bool Amd64IsUseException(t_amd64_ins * ins)
{
  switch(AMD64_INS_OPCODE(ins)){
    case AMD64_IMULexp2:
      if(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE2(ins))!=0)
	return FALSE;
    case AMD64_SUB:
    case AMD64_XOR:
      return 
	(
	 (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg && AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg)
	 &&
	 (AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_OP_BASE(AMD64_INS_DEST(ins)))
	);
    case AMD64_AND:
      return
	(
	 (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
	 &&
	 (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins))==0)
	);
    default:
      return FALSE;
  }
}
/* }}} */

/* {{{ Amd64InsUsedRegisters */
t_regset Amd64InsUsedRegisters(t_amd64_ins * ins)
{
  t_regset use = NullRegs;
  t_amd64_operand * op;

  if (AMD64_INS_TYPE(ins) == IT_DATA)
    return use;

  /* {{{ used condition flags */
  if (AMD64_INS_CONDITION(ins) == AMD64_CONDITION_NONE)
    RegsetSetUnion(use, amd64_opcode_table[AMD64_INS_OPCODE(ins)].cf_used);
  else
  {
    switch (AMD64_INS_CONDITION(ins))
    {
      case AMD64_CONDITION_O:
      case AMD64_CONDITION_NO:
	RegsetSetAddReg(use, AMD64_CONDREG_OF);
	break;
      case AMD64_CONDITION_B:
      case AMD64_CONDITION_AE:
	RegsetSetAddReg(use, AMD64_CONDREG_CF);
	break;
      case AMD64_CONDITION_Z:
      case AMD64_CONDITION_NZ:
	RegsetSetAddReg(use, AMD64_CONDREG_ZF);
	break;
      case AMD64_CONDITION_BE:
      case AMD64_CONDITION_A:
	RegsetSetAddReg(use, AMD64_CONDREG_CF);
	RegsetSetAddReg(use, AMD64_CONDREG_ZF);
	break;
      case AMD64_CONDITION_S:
      case AMD64_CONDITION_NS:
	RegsetSetAddReg(use, AMD64_CONDREG_SF);
	break;
      case AMD64_CONDITION_P:
      case AMD64_CONDITION_NP:
	RegsetSetAddReg(use, AMD64_CONDREG_PF);
	break;
      case AMD64_CONDITION_L:
      case AMD64_CONDITION_GE:
	RegsetSetAddReg(use, AMD64_CONDREG_SF);
	RegsetSetAddReg(use, AMD64_CONDREG_OF);
	break;
      case AMD64_CONDITION_LE:
      case AMD64_CONDITION_G:
	RegsetSetAddReg(use, AMD64_CONDREG_SF);
	RegsetSetAddReg(use, AMD64_CONDREG_OF);
	RegsetSetAddReg(use, AMD64_CONDREG_ZF);
	break;
      case AMD64_CONDITION_LOOPZ:
      case AMD64_CONDITION_LOOPNZ:
	RegsetSetAddReg(use, AMD64_CONDREG_ZF);
	break;
      case AMD64_CONDITION_RCXZ:
      case AMD64_CONDITION_LOOP:
	/* no condition flags used */
	break;
      default:
	FATAL(("unexpected condition value for @I",ins));
    }
  } /* }}} */

  /* {{{ source operands */
  op = AMD64_INS_SOURCE1(ins);
  switch (AMD64_OP_TYPE(op))
  {
    case amd64_optype_reg:
      if(!Amd64IsUseException(ins))
      RegsetSetAddReg(use, AMD64_OP_BASE(op));
      break;
    case amd64_optype_mem:
      if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
	RegsetSetAddReg(use, AMD64_OP_BASE(op));
      if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	RegsetSetAddReg(use, AMD64_OP_INDEX(op));
      break;
    default:
      /* keep the compiler happy */
      break;
  }
  op = AMD64_INS_SOURCE2(ins);
  switch (AMD64_OP_TYPE(op))
  {
    case amd64_optype_reg:
      RegsetSetAddReg(use, AMD64_OP_BASE(op));
      break;
    case amd64_optype_mem:
      if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
	RegsetSetAddReg(use, AMD64_OP_BASE(op));
      if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	RegsetSetAddReg(use, AMD64_OP_INDEX(op));
      break;
    default:
      /* keep the compiler happy */
      break;
  }
  /* instructions where the destination operand is also used as a source operand */
  if (AMD64_INS_HAS_FLAG(ins,AMD64_IF_DEST_IS_SOURCE))
  {
    op = AMD64_INS_DEST(ins);
    switch (AMD64_OP_TYPE(op))
    {
      case amd64_optype_reg:
	if(!Amd64IsUseException(ins))
	RegsetSetAddReg(use, AMD64_OP_BASE(op));
	break;
      case amd64_optype_mem:
	if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
	  RegsetSetAddReg(use, AMD64_OP_BASE(op));
	if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
	  RegsetSetAddReg(use, AMD64_OP_INDEX(op));
	break;
      default:
	/* keep the compiler happy */
	break;
    }
  }
  /* }}} */

  /* {{{ the destination operand */
  op = AMD64_INS_DEST(ins);
  if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    if (AMD64_OP_BASE(op) != AMD64_REG_NONE)
      RegsetSetAddReg(use, AMD64_OP_BASE(op));
    if (AMD64_OP_INDEX(op) != AMD64_REG_NONE)
      RegsetSetAddReg(use, AMD64_OP_INDEX(op));
  } /* }}} */

  /* {{{ prefixes */ 
  /* the REP/REPNZ prefixes */
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REP) ||
      AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REPNZ))
    RegsetSetAddReg(use, AMD64_REG_RCX);
  
  /* used segment registers: instructions always use the CS register, because this register
   * is used by the processor to determine from where it has to load the instruction. Instructions
   * that have memory operands usually get them from the data segment, so these instructions also 
   * the DS register. However, this can be overridden using the segment override prefix. The string
   * instructions also implicitly use the ES register (all of this is primarily of importance for 
   * kernel level code, not for user level code) */
#if 0
  if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_CS_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_CS);
  else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_DS_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_DS);
  else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_ES_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_RS);
  else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_FS_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_FS);
  else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_GS_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_GS);
  else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_SS_OVERRIDE))
    RegsetSetAddReg(use, AMD64_REG_SS);
  else
  {
    t_amd64_operand * op;
    /* instructions with memory operands use the DS register, except when the 
     * reference is relative to the %e{b,s}p register */
    op = AMD64_INS_DEST(ins);
    if (AMD64_OP_TYPE(op) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_RSP)
	RegsetSetAddReg(use, AMD64_REG_SS);
      else
	RegsetSetAddReg(use, AMD64_REG_DS);
    }
    op = AMD64_INS_SOURCE1(ins);
    if (AMD64_OP_TYPE(op) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_RSP)
	RegsetSetAddReg(use, AMD64_REG_SS);
      else
	RegsetSetAddReg(use, AMD64_REG_DS);
    }
    op = AMD64_INS_SOURCE2(ins);
    if (AMD64_OP_TYPE(op) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_RSP)
	RegsetSetAddReg(use, AMD64_REG_SS);
      else
	RegsetSetAddReg(use, AMD64_REG_DS);
    }
  }
  
  if (AMD64_INS_OPCODE(ins) == AMD64_MOVSB || AMD64_INS_OPCODE(ins) == AMD64_MOVSD ||
      AMD64_INS_OPCODE(ins) == AMD64_SCASB || AMD64_INS_OPCODE(ins) == AMD64_SCASD ||
      AMD64_INS_OPCODE(ins) == AMD64_STOSB || AMD64_INS_OPCODE(ins) == AMD64_STOSD ||
      AMD64_INS_OPCODE(ins) == AMD64_CMPSB || AMD64_INS_OPCODE(ins) == AMD64_CMPSD ||
      AMD64_INS_OPCODE(ins) == AMD64_INSB  || AMD64_INS_OPCODE(ins) == AMD64_INSD  )
    RegsetSetAddReg(use, AMD64_REG_RS);

  if (AMD64_INS_OPCODE(ins) == AMD64_PUSH || AMD64_INS_OPCODE(ins) == AMD64_POP ||
      AMD64_INS_OPCODE(ins) == AMD64_PUSHA || AMD64_INS_OPCODE(ins) == AMD64_POPA ||
      AMD64_INS_OPCODE(ins) == AMD64_PUSHF || AMD64_INS_OPCODE(ins) == AMD64_POPF )
    RegsetSetAddReg(use, AMD64_REG_SS);
#endif
  /* }}}*/
  
  /* {{{ partially defined registers */
  /* if the destination of the instruction is a partial register,
   * the destination register is also used (to preserve the 
   * unaffected bits of the register) */
  op = AMD64_INS_DEST(ins);
  
  if (AMD64_OP_TYPE(op) == amd64_optype_reg && 
      Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op)) && 
      AMD64_OP_REGMODE(op) != amd64_regmode_full64)
  {
    RegsetSetAddReg(use,AMD64_OP_BASE(op));
  }

  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE1_DEF))
  {
    op = AMD64_INS_SOURCE1(ins);
    if (AMD64_OP_TYPE(op) == amd64_optype_reg && 
	Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op)) && 
	AMD64_OP_REGMODE(op) != amd64_regmode_full64)
    {
      RegsetSetAddReg(use,AMD64_OP_BASE(op));
    }
  }
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE2_DEF))
  {
    op = AMD64_INS_SOURCE2(ins);
    if (AMD64_OP_TYPE(op) == amd64_optype_reg && 
	Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op)) && 
	AMD64_OP_REGMODE(op) != amd64_regmode_full64)
    {
      RegsetSetAddReg(use,AMD64_OP_BASE(op));
    }
  }
  /* }}} */

  /* {{{ fp instructions that push or pop the fp stack */
  if (amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops != 0)
  {
    int i;
    for (i = 0; i < 8-amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops; i++)
      RegsetSetAddReg(use, AMD64_REG_ST0 + amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops + i);
  }
  if (amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pushes != 0)
  {
    int i;
    for (i = 0; i < 8-amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pushes; i++)
      RegsetSetAddReg(use, AMD64_REG_ST0 + i);
  }
  /* }}} */

  /* special cases */
  switch (AMD64_INS_OPCODE(ins))
  {
    /* {{{ push and pop implicitly use the stack pointer */
    case AMD64_PUSH:
    case AMD64_PUSHA:
    case AMD64_PUSHF:
    case AMD64_POP:
    case AMD64_POPA:
    case AMD64_POPF:
      RegsetSetAddReg(use, AMD64_REG_RSP);
      /* pusha uses all general purpose registers */
      if (AMD64_INS_OPCODE(ins) == AMD64_PUSHA)
      {
	RegsetSetAddReg(use, AMD64_REG_RAX);
	RegsetSetAddReg(use, AMD64_REG_RBX);
	RegsetSetAddReg(use, AMD64_REG_RCX);
	RegsetSetAddReg(use, AMD64_REG_RDX);
	RegsetSetAddReg(use, AMD64_REG_RSI);
	RegsetSetAddReg(use, AMD64_REG_RDI);
	RegsetSetAddReg(use, AMD64_REG_RSP);
	RegsetSetAddReg(use, AMD64_REG_RBP);
      }
      break;
      /* }}} */

    /* {{{ call and return instructions use the stack pointer */
    case AMD64_CALL:
    case AMD64_CALLF:
    case AMD64_RET:
    case AMD64_RETF:
    case AMD64_IRET:
      RegsetSetAddReg(use, AMD64_REG_RSP);
      break;
      /* }}} */

    /* {{{ enter and leave instructions use %esp and %ebp */
    case AMD64_ENTER:
      RegsetSetAddReg(use, AMD64_REG_RSP);
      RegsetSetAddReg(use, AMD64_REG_RBP);
      break;
    case AMD64_LEAVE:
      RegsetSetAddReg(use, AMD64_REG_RBP);
      break;
      /* }}}*/
    
    /* {{{ div and idiv instructions use edx */
    case AMD64_DIV:
    case AMD64_IDIV:
      if (AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64 || 
	  AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo16 )
	RegsetSetAddReg(use, AMD64_REG_RDX);
      break;
      /* }}}*/

    case AMD64_INT:
    case AMD64_INTO:
    case AMD64_INT3:
    case AMD64_SYSENTER:
    case AMD64_SYSEXIT:
    case AMD64_SYSCALL:
    case AMD64_SYSRET:
      /* this modelling is overly conservative for 
       * sysenter and sysexit, but they are too
       * complicated for a decent modelling anyway */
      RegsetSetAddReg(use, AMD64_REG_RAX);
      RegsetSetAddReg(use, AMD64_REG_RBX);
      RegsetSetAddReg(use, AMD64_REG_RCX);
      RegsetSetAddReg(use, AMD64_REG_RDX);
      RegsetSetAddReg(use, AMD64_REG_RSI);
      RegsetSetAddReg(use, AMD64_REG_RDI);
      RegsetSetAddReg(use, AMD64_REG_RSP);
      RegsetSetAddReg(use, AMD64_REG_RBP);
      RegsetSetAddReg(use, AMD64_REG_R8);
      RegsetSetAddReg(use, AMD64_REG_R9);
      RegsetSetAddReg(use, AMD64_REG_R10);
      RegsetSetAddReg(use, AMD64_REG_R11);
      RegsetSetAddReg(use, AMD64_REG_R12);
      RegsetSetAddReg(use, AMD64_REG_R13);
      RegsetSetAddReg(use, AMD64_REG_R14);
      RegsetSetAddReg(use, AMD64_REG_R15);
      break;

    /* jrcxz and loop(z/nz) use the %rcx register */
    case AMD64_JRCXZ:
    case AMD64_LOOP:
    case AMD64_LOOPZ:
    case AMD64_LOOPNZ:
      RegsetSetAddReg(use, AMD64_REG_RCX);
      break;
    /*cmpxchg uses eax*/
    case AMD64_CMPXCHG:
      RegsetSetAddReg(use, AMD64_REG_RAX);
      break;

    default:
      break;
  }

  return use;
} /*}}} */

/* {{{ Amd64InsDefinedRegisters */
t_regset Amd64InsDefinedRegisters(t_amd64_ins * ins)
{
  t_regset def = NullRegs;

  if (AMD64_INS_TYPE(ins) == IT_DATA)
    return def;

  /* defined condition flags */
  RegsetSetUnion(def, amd64_opcode_table[AMD64_INS_OPCODE(ins)].cf_defined);

  /* if the destination operand is a register, this register is, of course, defined */
  if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg)
    RegsetSetAddReg(def, AMD64_OP_BASE(AMD64_INS_DEST(ins)));

  /* source operands that are defined as well */
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE1_DEF))
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg)
      RegsetSetAddReg(def, AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)));
  if (AMD64_INS_HAS_FLAG(ins, AMD64_IF_SOURCE2_DEF))
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg)
      RegsetSetAddReg(def, AMD64_OP_BASE(AMD64_INS_SOURCE2(ins)));

  /* the REP/REPNZ prefixes */
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REP) ||
      AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REPNZ))
  {
    RegsetSetAddReg(def, AMD64_REG_RCX);
  }

  /* floating point instructions that push or pop the stack
   * redefine the fpu registers */
  if (amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops != 0)
  {
    int i;
    for (i = 0; i < 8-amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pops; i++)
      RegsetSetAddReg(def, AMD64_REG_ST0 + i);
  }
  if (amd64_opcode_table[AMD64_INS_OPCODE(ins)].fpstack_pushes != 0)
  {
    int i;
    for (i = 0; i < 8; i++)
      RegsetSetAddReg(def, AMD64_REG_ST0 + i);
  }

  /* special cases */
  switch (AMD64_INS_OPCODE(ins))
  {
    /* {{{ 1. string instructions: these define %edi and %esi */
    case AMD64_MOVSB: case AMD64_MOVSD:
    case AMD64_CMPSB: case AMD64_CMPSD:
      RegsetSetAddReg(def, AMD64_REG_RDI);
      RegsetSetAddReg(def, AMD64_REG_RSI);
      break;
    case AMD64_LODSB: case AMD64_LODSD:
    case AMD64_OUTSB: case AMD64_OUTSD:
      RegsetSetAddReg(def, AMD64_REG_RSI);
      break;
    case AMD64_STOSB: case AMD64_STOSD:
    case AMD64_SCASB: case AMD64_SCASD:
    case AMD64_INSB:  case AMD64_INSD:
      RegsetSetAddReg(def, AMD64_REG_RDI);
      break;
      /* }}}*/

    /* {{{ 2. call and return instructions change the stack pointer */
    case AMD64_CALL:
    case AMD64_CALLF:
    case AMD64_RET:
    case AMD64_RETF:
    case AMD64_IRET:
      RegsetSetAddReg(def, AMD64_REG_RSP);
      break;
      /* }}} */
      
    /* {{{ 3. enter and leave instructions change %esp and %ebp */
    case AMD64_ENTER:
    case AMD64_LEAVE:
      RegsetSetAddReg(def, AMD64_REG_RSP);
      RegsetSetAddReg(def, AMD64_REG_RBP);
      break;
      /* }}}*/
      
    /* 4. {{{ mul, imul, div and idiv implicitly define %edx
     * if executed with 16-bit or 32-bit operands */
    case AMD64_MUL:
    case AMD64_IMUL:
    case AMD64_DIV:
    case AMD64_IDIV:
      if (AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_full64 || 
	  AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_lo16 )
	RegsetSetAddReg(def, AMD64_REG_RDX);
      break;
      /* }}} */

    /* 5. fstsw modifies the %eax register */
    case AMD64_FSTSW:
      RegsetSetAddReg(def, AMD64_REG_RAX);
      break;

    /* 6. {{{ push and pop implicitly define the stack pointer */
    case AMD64_PUSH:
    case AMD64_PUSHA:
    case AMD64_PUSHF:
    case AMD64_POP:
    case AMD64_POPA:
    case AMD64_POPF:
      RegsetSetAddReg(def, AMD64_REG_RSP);

      /* POPA defines all general purpose registers */
      if (AMD64_INS_OPCODE(ins) == AMD64_POPA)
      {
	RegsetSetAddReg(def, AMD64_REG_RAX);
	RegsetSetAddReg(def, AMD64_REG_RBX);
	RegsetSetAddReg(def, AMD64_REG_RCX);
	RegsetSetAddReg(def, AMD64_REG_RDX);
	RegsetSetAddReg(def, AMD64_REG_RSI);
	RegsetSetAddReg(def, AMD64_REG_RDI);
	RegsetSetAddReg(def, AMD64_REG_RSP);
	RegsetSetAddReg(def, AMD64_REG_RBP);
      }
      break;
      /* }}} */

    case AMD64_INT:
    case AMD64_INTO:
    case AMD64_INT3:
      /* syscalls define %rax */
      RegsetSetAddReg(def, AMD64_REG_RAX);
      break;

    case AMD64_LOOP:
    case AMD64_LOOPZ:
    case AMD64_LOOPNZ:
      /* loop instructions define rcx */
      RegsetSetAddReg(def, AMD64_REG_RCX);
      break;

    /* calling conventions state that any function call (and consequently all pseudo calls)
     * may change %rax, %rcx and %rdx */
    case AMD64_PSEUDOCALL:
      RegsetSetAddReg(def, AMD64_REG_RAX);
      RegsetSetAddReg(def, AMD64_REG_RCX);
      RegsetSetAddReg(def, AMD64_REG_RDX);
      break;
    case AMD64_CMPXCHG:
      RegsetSetAddReg(def, AMD64_REG_RAX);
      break;

    case AMD64_CPUID:
      RegsetSetAddReg(def, AMD64_REG_RAX);
      RegsetSetAddReg(def, AMD64_REG_RBX);
      RegsetSetAddReg(def, AMD64_REG_RCX);
      RegsetSetAddReg(def, AMD64_REG_RDX);
      break;

    default:
      break;
  }
  return def;
} /* }}} */

t_address Amd64InsGetSize(t_amd64_ins * ins)
{
  t_uint8 buf[15];
  t_address ret;
  t_uint32 len;
  len = Amd64AssembleIns(ins,buf);
  ret=AddressNew64(len);
  return ret;
}

/** {{{ Get first relocated operand */
t_amd64_operand * Amd64InsGetFirstRelocatedOp(t_amd64_ins * ins)
{
  if (AMD64_OP_FLAGS(AMD64_INS_DEST(ins)) & AMD64_OPFLAG_ISRELOCATED)
    return AMD64_INS_DEST(ins);
  else if (AMD64_OP_FLAGS(AMD64_INS_SOURCE1(ins)) & AMD64_OPFLAG_ISRELOCATED)
    return AMD64_INS_SOURCE1(ins);
  else if (AMD64_OP_FLAGS(AMD64_INS_SOURCE2(ins)) & AMD64_OPFLAG_ISRELOCATED)
    return AMD64_INS_SOURCE2(ins);
  else FATAL(("Could not find a relocated operand"));
  return NULL;
} /* }}} */

/** {{{ Get second relocated operand */
t_amd64_operand * Amd64InsGetSecondRelocatedOp(t_amd64_ins * ins)
{
  t_bool foundfirst = FALSE;

  if (AMD64_OP_FLAGS(AMD64_INS_DEST(ins)) & AMD64_OPFLAG_ISRELOCATED)
    foundfirst = TRUE;

  if (AMD64_OP_FLAGS(AMD64_INS_SOURCE1(ins)) & AMD64_OPFLAG_ISRELOCATED)
  {
    if (foundfirst)
      return AMD64_INS_SOURCE1(ins);
    else
      foundfirst = TRUE;
  }

  if (AMD64_OP_FLAGS(AMD64_INS_SOURCE2(ins)) & AMD64_OPFLAG_ISRELOCATED)
  {
    if (foundfirst)
      return AMD64_INS_SOURCE2(ins);
    else 
      FATAL(("found only one relocated operand"));
  }

  VERBOSE(0,("@iB\n@I\n",AMD64_INS_BBL(ins), ins));
  FATAL(("not enough relocated operands"));
  return NULL;
} /* }}} */

/** {{{ Get reloc for operand */
t_reloc * Amd64GetRelocForOp(t_amd64_ins * ins, t_amd64_operand * op)
{
  t_amd64_operand * ops[3];
  int i;
  t_bool second_relocated = FALSE;
  t_reloc_ref * rr, * rr2, * tmp;

  if (!(AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)) return NULL;

  ops[0] = AMD64_INS_DEST(ins);
  ops[1] = AMD64_INS_SOURCE1(ins);
  ops[2] = AMD64_INS_SOURCE2(ins);
  
  for (i=0; i<3; i++)
  {
    if (ops[i] == op) break;
    if (AMD64_OP_FLAGS(ops[i]) & AMD64_OPFLAG_ISRELOCATED)
      second_relocated = TRUE;
  }

  rr = AMD64_INS_REFERS_TO(ins);
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

/* Amd64IsSyscallExit {{{ */
t_tristate Amd64IsSyscallExit(t_amd64_ins * ins)
{
  t_uint32 syscallno;
  
  if (AMD64_INS_OPCODE(ins) != AMD64_INT) return NO;
  if (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) != 0x80) return NO;

  syscallno = Amd64GetSyscallNo(ins);
  /* store this syscall number in the instruction, this might be handy */
  AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE2(ins)) = syscallno;
  
  if (syscallno == -1)
    return PERHAPS;
  else if (syscallno == 0x1 || syscallno == 0xfc)
    return YES;

  return NO;
}
/* }}} */

/* Amd64GetSyscallNo {{{ */
t_uint32 Amd64GetSyscallNo(t_amd64_ins * ins)
{
  t_amd64_ins * iter, * use;
  t_reg lookfor = AMD64_REG_RAX;

  if (AMD64_INS_OPCODE(ins) != AMD64_INT) return -1;
  if (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) != 0x80) return -1;

  /* find the instruction that defines %eax */
  use = ins;
look_for_definition:
  for (iter = AMD64_INS_IPREV(use); iter; iter = AMD64_INS_IPREV(iter))
  {
    if (RegsetIn(AMD64_INS_REGS_DEF(iter), lookfor))
      break;
  }
  if (!iter)
    return -1;

  if (AMD64_INS_OPCODE(iter) == AMD64_MOV)
  {
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(iter)) == amd64_optype_imm)
      return AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(iter));
    else if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(iter)) == amd64_optype_reg)
    {
      lookfor = AMD64_OP_BASE(AMD64_INS_SOURCE1(iter));
      use = iter;
      goto look_for_definition;
    }
  }
  else
    FATAL(("Syscall @iB @I\n eax defined by @I\nIMPLEMENT!",AMD64_INS_BBL(ins),ins,iter));

  return -1;
}
/* }}} */

void Amd64OpSetReg(t_amd64_operand * op, t_reg reg, t_amd64_regmode mode)
{
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_BASE(op) = reg;
  AMD64_OP_REGMODE(op) = mode;
  AMD64_OP_FLAGS(op) = 0x0;
}

void Amd64OpSetImm(t_amd64_operand * op, t_uint64 imm, t_uint32 immedsize)
{
  AMD64_OP_TYPE(op) = amd64_optype_imm;
  AMD64_OP_IMMEDIATE(op) = imm;
  AMD64_OP_IMMEDSIZE(op) = immedsize;
  AMD64_OP_FLAGS(op) = 0x0;
}

void Amd64OpSetMem(t_amd64_operand * op, t_uint32 offset, t_reg base, t_reg index, int scale, t_uint32 memopsize)
{
  AMD64_OP_TYPE(op) = amd64_optype_mem;
  AMD64_OP_IMMEDIATE(op) = offset;
  AMD64_OP_IMMEDSIZE(op) = 4;
  AMD64_OP_MEMOPSIZE(op) = memopsize;
  AMD64_OP_BASE(op) = base;
  AMD64_OP_INDEX(op) = index;
  AMD64_OP_SCALE(op) = scale;
  AMD64_OP_FLAGS(op) = 0x0;
}

/* Amd64InsGetMemLoadOp {{{ */
/* returns the memory operand of an instruction. if there is none, returns NULL */
t_amd64_operand * Amd64InsGetMemLoadOp(t_amd64_ins * ins)
{

  if (AMD64_INS_FLAGS(ins) & AMD64_IF_DEST_IS_SOURCE)
    if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem)
      return AMD64_INS_DEST(ins);

  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem)
    return AMD64_INS_SOURCE1(ins);

  if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
    return AMD64_INS_SOURCE2(ins);

  return NULL;
}
/* }}} */

/* Amd64InsGetMemStoreOp {{{ */
t_amd64_operand * Amd64InsGetMemStoreOp(t_amd64_ins * ins)
{
  if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem)
    return AMD64_INS_DEST(ins);

  if (AMD64_INS_FLAGS(ins) & AMD64_IF_SOURCE1_DEF)
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem)
      return AMD64_INS_SOURCE1(ins);
  
  if (AMD64_INS_FLAGS(ins) & AMD64_IF_SOURCE2_DEF)
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
      return AMD64_INS_SOURCE2(ins);

  return NULL;
}
/* }}} */

t_bool Amd64InsIsIndirectCall(t_amd64_ins * ins)
{
  return (AMD64_INS_OPCODE(ins) == AMD64_CALL) && (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_imm);
}

t_bool Amd64InsIsUnconditionalBranch(t_amd64_ins * ins)
{
  if (AMD64_INS_OPCODE(ins) == AMD64_JMP)  return TRUE;
  return FALSE;
}

t_bool Amd64InsIsProcedureCall(t_amd64_ins * ins)
{
  return AMD64_INS_OPCODE(ins) == AMD64_CALL;
}

/*! Callback function, called when InsFree is called. Needed to free dynamically
 * allocated fields */

void Amd64InsCleanup(t_amd64_ins * ins)
{
  Free(AMD64_INS_DEST(ins));
  Free(AMD64_INS_SOURCE1(ins));
  Free(AMD64_INS_SOURCE2(ins));
}

/*! Callback function, called when InsDup is called. Needed to copy dynamically
 * allocated fields */

void Amd64InsDupDynamic(t_amd64_ins * target, t_amd64_ins * source)
{
  AMD64_INS_SET_DEST(target, Malloc(sizeof(t_amd64_operand)));
  AMD64_INS_SET_SOURCE1(target, Malloc(sizeof(t_amd64_operand)));
  AMD64_INS_SET_SOURCE2(target, Malloc(sizeof(t_amd64_operand)));
  memcpy(AMD64_INS_DEST(target), AMD64_INS_DEST(source), sizeof(t_amd64_operand));
  memcpy(AMD64_INS_SOURCE1(target), AMD64_INS_SOURCE1(source), sizeof(t_amd64_operand));
  memcpy(AMD64_INS_SOURCE2(target), AMD64_INS_SOURCE2(source), sizeof(t_amd64_operand));
}

/* Amd64InsSetOperandFlags {{{ */
void Amd64InsSetOperandFlags(t_amd64_ins * ins)
{
  t_amd64_opcode_entry * entry = NULL;
  if (!Amd64InsIsConditional(ins)) 
  { 
    t_amd64_opcode opcode=AMD64_INS_OPCODE(ins);	  
    void * key = (void *) &opcode;
    t_amd64_opcode_he * he = HashTableLookup(amd64_opcode_hash, key);
    while (he)
    {
      entry = (t_amd64_opcode_entry *) he->entry;
      if (entry->op1check(AMD64_INS_DEST(ins),entry->op1bm) && entry->op2check(AMD64_INS_SOURCE1(ins),entry->op2bm) && entry->op3check(AMD64_INS_SOURCE2(ins),entry->op3bm))
	break;
      he = (t_amd64_opcode_he *) HASH_TABLE_NODE_EQUAL(&he->node);
    }
    if (entry->usedefpattern & DU)
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_DEST_IS_SOURCE);
    else
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) & (~AMD64_IF_DEST_IS_SOURCE));
    
    if (entry->usedefpattern & S1D)
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_SOURCE1_DEF);
    else
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) & (~AMD64_IF_SOURCE1_DEF));
    
    if (entry->usedefpattern & S2D)
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) | AMD64_IF_SOURCE2_DEF);
    else
      AMD64_INS_SET_FLAGS(ins, AMD64_INS_FLAGS(ins) & (~AMD64_IF_SOURCE2_DEF));
  }
}
/* }}} */

/* Amd64OpsAreIdentical {{{ */
t_bool Amd64OpsAreIdentical(t_amd64_operand * op1, t_amd64_operand * op2)
{
  if (AMD64_OP_TYPE(op1) != AMD64_OP_TYPE(op2)) return FALSE;
  
  switch (AMD64_OP_TYPE(op1))
  {
    case amd64_optype_none:
      return TRUE;
    case amd64_optype_imm:
      return AMD64_OP_IMMEDIATE(op1) == AMD64_OP_IMMEDIATE(op2);
    case amd64_optype_reg:
      return AMD64_OP_BASE(op1)         == AMD64_OP_BASE(op2)
	&& AMD64_OP_REGMODE(op1)      == AMD64_OP_REGMODE(op2);
    case amd64_optype_mem:
      return AMD64_OP_BASE(op1)         == AMD64_OP_BASE(op2)  &&
	AMD64_OP_INDEX(op1)        == AMD64_OP_INDEX(op2)  &&
	AMD64_OP_IMMEDIATE(op1)    == AMD64_OP_IMMEDIATE(op2) &&
	AMD64_OP_SCALE(op1)        == AMD64_OP_SCALE(op2) &&
	AMD64_OP_MEMOPSIZE(op1)    == AMD64_OP_MEMOPSIZE(op2);
    default:
      break;
  }
  return 
    AMD64_OP_TYPE(op1)         == AMD64_OP_TYPE(op2) &&
    AMD64_OP_BASE(op1)         == AMD64_OP_BASE(op2)  &&
    AMD64_OP_INDEX(op1)        == AMD64_OP_INDEX(op2)  &&
    AMD64_OP_IMMEDIATE(op1)    == AMD64_OP_IMMEDIATE(op2) &&
    AMD64_OP_SEGSELECTOR(op1)  == AMD64_OP_SEGSELECTOR(op2) &&
    AMD64_OP_SCALE(op1)        == AMD64_OP_SCALE(op2) &&
    AMD64_OP_REGMODE(op1)      == AMD64_OP_REGMODE(op2) &&
    AMD64_OP_IMMEDSIZE(op1)    == AMD64_OP_IMMEDSIZE(op2)  &&
    AMD64_OP_MEMOPSIZE(op1)    == AMD64_OP_MEMOPSIZE(op2) &&
    AMD64_OP_FLAGS(op1)        == AMD64_OP_FLAGS(op2); 
}
/* }}} */

/* Amd64InsAreIdentical {{{ */
t_bool Amd64InsAreIdentical(t_amd64_ins * ins1, t_amd64_ins * ins2)
{
  if (AMD64_INS_OPCODE(ins1) != AMD64_INS_OPCODE(ins2))
    return FALSE;

  /* catch calls and jumps with direct offsets */
  if (AMD64_INS_OPCODE(ins1) == AMD64_CALL || AMD64_INS_OPCODE(ins1) == AMD64_JMP || AMD64_INS_OPCODE(ins1) == AMD64_Jcc || AMD64_INS_OPCODE(ins1) == AMD64_JRCXZ || AMD64_INS_OPCODE(ins1) == AMD64_LOOP || AMD64_INS_OPCODE(ins1) == AMD64_LOOPZ || AMD64_INS_OPCODE(ins1) == AMD64_LOOPNZ|| AMD64_INS_OPCODE(ins1) == AMD64_JMPF || AMD64_INS_OPCODE(ins1) == AMD64_CALLF)
  {
    if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins1)) == AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins2))
	&& AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins1)) == amd64_optype_imm)
    {
      t_cfg_edge * e1, * e2;
      BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins1),e1)
	if (CFG_EDGE_CAT(e1) & (ET_CALL | ET_IPJUMP/* | ET_JUMP */))
	  break;
      BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins2),e2)
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
  if (AMD64_INS_OPCODE(ins1) == AMD64_JMP)
  {
    t_amd64_operand * op1 = AMD64_INS_SOURCE1(ins1);
    t_amd64_operand * op2 = AMD64_INS_SOURCE1(ins2);

    if (AMD64_OP_TYPE(op1) == AMD64_OP_TYPE(op2)
	&& AMD64_OP_TYPE(op1) == amd64_optype_mem
	&& AMD64_OP_BASE(op1) == AMD64_OP_BASE(op2)
	&& AMD64_OP_BASE(op1) == AMD64_REG_NONE
	&& AMD64_OP_INDEX(op1) == AMD64_OP_INDEX(op2)
	&& AMD64_OP_INDEX(op1) != AMD64_REG_NONE
	&& AMD64_OP_SCALE(op1) == AMD64_OP_SCALE(op2)
	&& AMD64_OP_SCALE(op1) == AMD64_SCALE_4
       )
    {
      t_cfg_edge * switch1, * switch2;
      int n1=0, n2=0;
      /* check if both switches have the same number of switch edges */
      BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins1),switch1)
	if (CFG_EDGE_CAT(switch1) == ET_SWITCH)
	  n1++;
      BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins2),switch2)
	if (CFG_EDGE_CAT(switch2) == ET_SWITCH)
	  n2++;
      if (n1 != n2)
	return FALSE;

      if (n1 > 0)
      {
	/* check if all switch edges from switch1 correspond with those of switch 2 */
	BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins1),switch1)
	{
	  if (CFG_EDGE_CAT(switch1) != ET_SWITCH) continue;
	  BBL_FOREACH_SUCC_EDGE(AMD64_INS_BBL(ins2),switch2)
	    if (CFG_EDGE_CAT(switch2) == ET_SWITCH && CFG_EDGE_SWITCHVALUE(switch2) == CFG_EDGE_SWITCHVALUE(switch1))
	      break;
	  if (!switch2) return FALSE;
	  if (!AddressIsEq(
		AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch1)),AMD64_INS_OLD_ADDRESS(ins1)),
		AddressSub(BBL_OLD_ADDRESS(CFG_EDGE_TAIL(switch2)),AMD64_INS_OLD_ADDRESS(ins2))
		)
	     )
	    return FALSE;
	}
	return TRUE;
      }
    }
  }
  
  return  
    AMD64_INS_OPCODE(ins1) == AMD64_INS_OPCODE(ins2) &&
    /*AMD64_INS_DATA(ins1)	==AMD64_INS_DATA(ins2) &&*/
    AMD64_INS_PREFIXES(ins1) ==	AMD64_INS_PREFIXES(ins2)	&&
    AMD64_INS_FLAGS(ins1) == AMD64_INS_FLAGS(ins2) &&
    Amd64OpsAreIdentical(AMD64_INS_DEST(ins1),AMD64_INS_DEST(ins2)) &&
    Amd64OpsAreIdentical(AMD64_INS_SOURCE1(ins1),AMD64_INS_SOURCE1(ins2)) &&
    Amd64OpsAreIdentical(AMD64_INS_SOURCE2(ins1),AMD64_INS_SOURCE2(ins2)) &&
    AMD64_INS_CONDITION(ins1) ==	AMD64_INS_CONDITION(ins2);
}
/* }}} */

/* Amd64BblFingerprint {{{ */
t_uint32 Amd64BblFingerprint(t_bbl * bbl)
{
  t_amd64_ins * ins;
  t_uint32 return_value = 0;
  if (BBL_NINS(bbl)<3) return 0;
  ins = T_AMD64_INS(BBL_INS_FIRST(bbl));
  return_value|=AMD64_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_DEST(ins));
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins));
  return_value<<=3;
  ins = AMD64_INS_INEXT(ins);
  return_value|=AMD64_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_DEST(ins));
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins));
  return_value<<=3;
  ins = AMD64_INS_INEXT(ins);
  return_value|=AMD64_INS_OPCODE(ins);
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_DEST(ins));
  return_value<<=3;
  return_value|=AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins));

  return return_value;
}
/* }}} */

/* Amd64InstructionUnconditionalizer {{{ */
/* unconditionalize conditional instructions of which constant propagation has determined that
 * they will always be executed */
t_bool Amd64InstructionUnconditionalizer(t_amd64_ins * ins)
{
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_Jcc:
    case AMD64_LOOP:
    case AMD64_LOOPZ:
    case AMD64_LOOPNZ:
    case AMD64_JRCXZ:
      AMD64_INS_SET_OPCODE(ins, AMD64_JMP);
      break;
    case AMD64_CMOVcc:
      AMD64_INS_SET_OPCODE(ins, AMD64_MOV);
      break;
      /*TODO SETcc */
    default:
      return FALSE;
  }

  AMD64_INS_SET_ATTRIB(ins, AMD64_INS_ATTRIB(ins) & (~IF_CONDITIONAL));
  AMD64_INS_SET_CONDITION(ins, AMD64_CONDITION_NONE);
  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));

  return TRUE;
}
/* }}} */

/* Amd64InvertConditionExistBbl {{{ */
t_bool Amd64InvertConditionExistBbl(t_bbl * i_bbl)
{
  if(AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(i_bbl)))==AMD64_Jcc)
  if(Amd64InvertConditionExist(AMD64_INS_CONDITION(T_AMD64_INS(BBL_INS_LAST(i_bbl)))))
    return TRUE;
  return FALSE;
}
/* }}} */

/* Amd64InvertConditionBbl {{{ */
t_bool Amd64InvertConditionBbl(t_bbl * i_bbl)
{
  if(Amd64InvertConditionExistBbl(i_bbl))
  {
    AMD64_INS_SET_CONDITION(T_AMD64_INS(BBL_INS_LAST(i_bbl)), Amd64InvertCondition(AMD64_INS_CONDITION(T_AMD64_INS(BBL_INS_LAST(i_bbl)))));
    return TRUE;
  }
  return FALSE;
}
/* }}} */

/* Amd64InvertConditionExist {{{ */
t_bool Amd64InvertConditionExist(t_amd64_condition_code test_cond)
{
  t_uint32 i=0;
  for(i=0;amd64_opposite_table[i].cond0!=AMD64_CONDITION_NONE;i++)
    if(amd64_opposite_table[i].cond0 == test_cond)
      return TRUE;
  for(i=0;amd64_opposite_table[i].cond1!=AMD64_CONDITION_NONE;i++)
    if(amd64_opposite_table[i].cond1 == test_cond)
      return TRUE;
  return FALSE;
}
/* }}} */

/* Amd64InvertCondition {{{ */
t_amd64_condition_code Amd64InvertCondition(t_amd64_condition_code condition)
{
  t_uint32 i=0;
  for(i=0;amd64_opposite_table[i].cond0!=AMD64_CONDITION_NONE;i++)
  {
    if(amd64_opposite_table[i].cond0 == condition)
      return amd64_opposite_table[i].cond1;
  }
  for(i=0;amd64_opposite_table[i].cond1!=AMD64_CONDITION_NONE;i++)
  {
    if(amd64_opposite_table[i].cond1 == condition)
      return amd64_opposite_table[i].cond0;
  }
  return AMD64_CONDITION_NONE;
}
/* }}} */

/* Amd64InvertBranchBbl {{{ */
t_bool Amd64InvertBranchBbl(t_bbl * bbl)
{
  t_bbl * target_bbl = NULL, * split_off;
  t_cfg_edge * edge, * s_edge;
  t_amd64_ins * ins;
  t_bool ipjump=FALSE;

  if(BBL_INS_LAST(bbl)==NULL)
    return FALSE;

  if(AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(bbl)))!=AMD64_Jcc)
    return FALSE;

  if(!Amd64InvertConditionBbl(bbl))
    FATAL(("ERROR! No inverse condition (Impossible!)"));

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

  ins = T_AMD64_INS(InsNewForBbl(split_off));
  Amd64InstructionMakeJump(ins);
  InsAppendToBbl(T_INS(ins), split_off);

  CfgEdgeCreate(BBL_CFG(bbl),bbl,split_off,ET_FALLTHROUGH);
  if(ipjump)
    CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_IPJUMP);
  else
    CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_JUMP);

  return TRUE;
}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
