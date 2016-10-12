#include <diabloanopti386.h>

/*#define I386COPYDEBUG */

void I386ReplaceCopiedByCopyBackward(t_i386_ins * prev_ins, t_reg copy, t_reg copied){

  while(prev_ins){
    if(RegsetIn(I386_INS_REGS_DEF(prev_ins),copied)&&!RegsetIn(I386_INS_REGS_USE(prev_ins),copied))
    {
      if(I386_OP_BASE(I386_INS_DEST(prev_ins)) == copied)
	I386_OP_BASE(I386_INS_DEST(prev_ins)) = copy;
      else {
	VERBOSE(0,("%d %d @I\n",copy, copied, prev_ins));
	FATAL(("this shouldn't happen"));
      } 
      I386_INS_SET_REGS_USE(prev_ins, I386InsUsedRegisters(prev_ins));;
      I386_INS_SET_REGS_DEF(prev_ins, I386InsDefinedRegisters(prev_ins));;
      return;
    }
    else
    {
      if(I386_OP_BASE(I386_INS_DEST(prev_ins))== copied)
	I386_OP_BASE(I386_INS_DEST(prev_ins)) = copy;
      if(I386_OP_BASE(I386_INS_SOURCE1(prev_ins))== copied)
	I386_OP_BASE(I386_INS_SOURCE1(prev_ins)) = copy;
      if(I386_OP_BASE(I386_INS_SOURCE2(prev_ins))== copied)
	I386_OP_BASE(I386_INS_SOURCE2(prev_ins)) = copy;
    }
    I386_INS_SET_REGS_USE(prev_ins, I386InsUsedRegisters(prev_ins));;
    I386_INS_SET_REGS_DEF(prev_ins, I386InsDefinedRegisters(prev_ins));;
      
    prev_ins = I386_INS_IPREV(prev_ins);
  }
}

void I386ReplaceCopiedByCopyForward(t_i386_ins * next_ins, t_reg copy, t_reg copied){

  while(next_ins){
    if(RegsetIn(I386_INS_REGS_DEF(next_ins),copied)&&!RegsetIn(I386_INS_REGS_USE(next_ins),copied))
    {
      return;
    }
    else
    {
      if(I386_OP_BASE(I386_INS_DEST(next_ins))== copied)
	I386_OP_BASE(I386_INS_DEST(next_ins)) = copy;
      if(I386_OP_BASE(I386_INS_SOURCE1(next_ins))== copied)
	I386_OP_BASE(I386_INS_SOURCE1(next_ins)) = copy;
      if(I386_OP_BASE(I386_INS_SOURCE2(next_ins))== copied)
	I386_OP_BASE(I386_INS_SOURCE2(next_ins)) = copy;
    }
    I386_INS_SET_REGS_USE(next_ins, I386InsUsedRegisters(next_ins));;
    I386_INS_SET_REGS_DEF(next_ins, I386InsDefinedRegisters(next_ins));;
      
    next_ins = I386_INS_INEXT(next_ins);
  }
}

t_bool I386CopiedIsReplacableByCopyInIns(t_i386_ins * ins, t_reg copy, t_reg copied)
{
/*  VERBOSE(0,("=><=@I, %d, %d, %d, %d\n", ins, copied, I386_OP_BASE(I386_INS_DEST(ins)),I386_OP_BASE(I386_INS_SOURCE1(ins)),I386_OP_BASE(I386_INS_SOURCE2(ins)))); */
  if(RegsetIn(I386_INS_REGS_USE(ins),copied)||RegsetIn(I386_INS_REGS_DEF(ins),copied)){
    /* implicit */
    if(!(
	  (I386_OP_TYPE(I386_INS_DEST(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_DEST(ins))==copied)
	  ||
	  (I386_OP_TYPE(I386_INS_SOURCE1(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE1(ins))==copied)
	  ||
	  (I386_OP_TYPE(I386_INS_SOURCE2(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE2(ins))==copied)
	)){
      	return FALSE;
    }
    /* explicit, check if can be replaced */
    else{
      t_i386_opcode_entry * forms[10];   /* this is certainly large enough */
      t_uint8 nfitting;
      
      if(I386_OP_TYPE(I386_INS_DEST(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_DEST(ins))==copied)
	I386_OP_BASE(I386_INS_DEST(ins))=copy;
      if(I386_OP_TYPE(I386_INS_SOURCE1(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE1(ins))==copied)
	I386_OP_BASE(I386_INS_SOURCE1(ins))=copy;
      if(I386_OP_TYPE(I386_INS_SOURCE2(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE2(ins))==copied)
	I386_OP_BASE(I386_INS_SOURCE2(ins))=copy;
      
      nfitting = I386GetPossibleEncodings(ins,forms);

      if(I386_OP_TYPE(I386_INS_DEST(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_DEST(ins))==copy)
	I386_OP_BASE(I386_INS_DEST(ins))=copied;
      if(I386_OP_TYPE(I386_INS_SOURCE1(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE1(ins))==copy)
	I386_OP_BASE(I386_INS_SOURCE1(ins))=copied;
      if(I386_OP_TYPE(I386_INS_SOURCE2(ins))==i386_optype_reg&&I386_OP_BASE(I386_INS_SOURCE2(ins))==copy)
	I386_OP_BASE(I386_INS_SOURCE2(ins))=copied;

      if(nfitting==0) return FALSE;
    } 
  }
      
  return TRUE;
}

/*
 * This function checks if the copied register can be replaced by the copy in the preceeding instructions
 * */ 
t_bool I386CopiedIsReplacableByCopyBackward(t_i386_ins * prev_ins, t_reg copy, t_reg copied){
  
  while(prev_ins){

    if(RegsetIn(I386_INS_REGS_USE(prev_ins),copy))
      return FALSE;
    if(RegsetIn(I386_INS_REGS_DEF(prev_ins),copy)){
      return FALSE;
    }
    
    if(!I386CopiedIsReplacableByCopyInIns(prev_ins, copy, copied))
      return FALSE;
    
    if(RegsetIn(I386_INS_REGS_DEF(prev_ins),copied)&&!RegsetIn(I386_INS_REGS_USE(prev_ins),copied))
      return TRUE;
    
    prev_ins = I386_INS_IPREV(prev_ins);
  }
  
  return FALSE;
  /*
  BBL_FOREACH_PRED_EDGE(INS_BBL(prev_ins),prev_edge){
    t_bbl * prev_bbl = T_CFG_HEAD(prev_edge);
    
    if(BBL_IS_HELL(prev_bbl))
      return FALSE;

    if(!I386CopiedIsReplacableByCopyBackward(BBL_INS_LAST(prev_bbl),copy,copied))
      return FALSE; 

    BBL_FOREACH_SUCC_EDGE(prev_bbl, next_edge){
      t_bbl * next_bbl = CFG_EDGE_TAIL(next_edge);
    
      if(BBL_IS_HELL(next_bbl))
	return FALSE;
      if(!I386CopiedIsReplacableForward(BBL_INS_FIRST(next_bbl), copy, copied)) 
	return FALSE;

    }
  }
  return TRUE;*/
}

/*
 * This function checks if the copied register can be replaced by the copy in the following instructions 
 * */

t_bool I386CopiedIsReplacableByCopyForward(t_i386_ins * ins, t_reg copy, t_reg copied){
  while(ins){
    if(!RegsetIn(I386InsRegsLiveBefore(ins), copied))
      return TRUE;

    if(RegsetIn(I386_INS_REGS_DEF(ins),copy)){
      return FALSE;
    }

    if(!I386CopiedIsReplacableByCopyInIns(ins, copy, copied))
      return FALSE;
 
    ins = I386_INS_INEXT(ins);
  }
  return FALSE;
/*  
  BBL_FOREACH_SUCC_EDGE(INS_BBL(ins),edge){
    t_bbl * next_bbl = CFG_EDGE_TAIL(edge);
    
    if(BBL_IS_HELL(next_bbl))
      return FALSE;
    if(!I386CopiedIsReplacableForward(BBL_INS_FIRST(next_bbl), copy, copied)) 
      return FALSE;
  }
  return TRUE;
  */
}


static t_uint32 killed=0;
/* This optimization searches for a copy instruction in which the copied register is no longer alive after the 
 * copy instruction. In this case, it is sometimes possible not to use the copied register, but to assign its 
 * value immediately to the copy where it is declared.
 * */

void I386BackwardCopyPropagation(t_section * sec){
  t_cfg *cfg = OBJECT_CFG(SECTION_OBJECT(sec));
  t_reg copy, copied;
  t_bbl * bbl;
  t_i386_ins * ins;
  t_i386_operand * dest;
  t_i386_operand * src1;

  NodeMarkInit();
  EdgeMarkInit();

  CFG_FOREACH_BBL(cfg,bbl){
    BBL_FOREACH_I386_INS(bbl,ins){
      dest = I386_INS_DEST(ins);
      src1 = I386_INS_SOURCE1(ins);
      if(I386_INS_OPCODE(ins)==I386_MOV){
	if (I386_OP_TYPE(dest) == i386_optype_reg && I386IsGeneralPurposeReg(I386_OP_BASE(dest))){
	  if (I386_OP_TYPE(src1) == i386_optype_reg && I386IsGeneralPurposeReg(I386_OP_BASE(src1))){
	    copy = I386_OP_BASE(dest);
	    copied = I386_OP_BASE(src1);
	    if(I386CopiedIsReplacableByCopyBackward(I386_INS_IPREV(ins), copy, copied)&&I386CopiedIsReplacableByCopyForward(I386_INS_INEXT(ins), copy, copied)){

#ifdef I386COPYDEBUG
	      VERBOSE(0,("voor---\n@I:\n@iB\n---\n",ins,I386_INS_BBL(ins)));
#endif
	      I386ReplaceCopiedByCopyBackward(I386_INS_IPREV(ins), copy, copied);
	      I386ReplaceCopiedByCopyForward(I386_INS_INEXT(ins), copy, copied);
	      I386InsKill(ins);
#ifdef I386COPYDEBUG
	      VERBOSE(0,("na---\n@I:\n@iB\n---\n",ins,I386_INS_BBL(ins)));
#endif
	      killed++;
	    }
	  }
	}
      }
    }
  }
  printf("%d victims\n",killed);
}

/* {{{ instruction propagator for copy analysis */
void I386CopyInstructionPropagator(t_i386_ins * ins, t_equations eqs, t_bool ignore_condition)
{
  t_i386_operand * dest = I386_INS_DEST(ins);
  t_i386_operand * src1 = I386_INS_SOURCE1(ins);
  t_regset tmp;
  t_cfg * cfg=I386_INS_CFG(ins);
  /*t_i386_operand * src2 = I386_INS_SOURCE2(ins); */

  /*VERBOSE(0,("copyprop @I\n", ins)); */

  switch (I386_INS_OPCODE(ins))
  {
    case I386_MOV:
      if (I386_OP_TYPE(dest) == i386_optype_reg 
	  && I386_OP_REGMODE(dest) == i386_regmode_full32 
	  && I386IsGeneralPurposeReg(I386_OP_BASE(dest)))
      {
	if (I386_OP_TYPE(src1) == i386_optype_reg 
	    && I386_OP_REGMODE(src1) == i386_regmode_full32 
	    && I386IsGeneralPurposeReg(I386_OP_BASE(src1)))
	{
	  /* mov %reg1,%reg2 */
	  EquationsAdd(cfg,eqs,I386_OP_BASE(dest),I386_OP_BASE(src1),0,NULL,NULL);
	  return;
	}
	else if (I386_OP_TYPE(src1) == i386_optype_imm)
	{
	  /* mov $imm,%reg */
	  if (I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED)
	  {
	    if (I386_INS_REFERS_TO(ins) == NULL)
	      FATAL(("DISCREPANCY @I in @iB",ins,I386_INS_BBL(ins)));
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),CFG_DESCRIPTION(cfg)->num_int_regs,0, RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)),NULL);
	  }
	  else
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),CFG_DESCRIPTION(cfg)->num_int_regs,0-I386_OP_IMMEDIATE(src1),NULL,NULL);
	  return;
	}
      }
      break;
    case I386_ADD:
      if (I386_OP_TYPE(dest) == i386_optype_reg 
	  && I386_OP_REGMODE(dest) == i386_regmode_full32 
	  && I386IsGeneralPurposeReg(I386_OP_BASE(dest)))
      {
	if (I386_OP_TYPE(src1) == i386_optype_imm
	    && !(I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED))
	{
	  /* add $imm,%reg */
	  EquationsAdd(cfg,eqs,I386_OP_BASE(dest),I386_OP_BASE(dest),-I386_OP_IMMEDIATE(src1),NULL,NULL);
	  return;
	}
      }
      break;
    case I386_SUB:
      if (I386_OP_TYPE(dest) == i386_optype_reg 
	  && I386_OP_REGMODE(dest) == i386_regmode_full32 
	  && I386IsGeneralPurposeReg(I386_OP_BASE(dest)))
      {
	if (I386_OP_TYPE(src1) == i386_optype_imm
	    && !(I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED))
	{
	  /* sub $imm,%reg */
	  EquationsAdd(cfg,eqs,I386_OP_BASE(dest),I386_OP_BASE(dest),I386_OP_IMMEDIATE(src1),NULL,NULL);
	  return;
	}
      }
      break;
    case I386_LEA:
      if (I386_OP_INDEX(src1) == I386_REG_NONE)
      {
	if (I386_OP_BASE(src1) == I386_REG_NONE)
	{
	  /* lea addr,%reg */
	  if (I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED)
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),CFG_DESCRIPTION(cfg)->num_int_regs,0, RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)),NULL);
	  else
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),CFG_DESCRIPTION(cfg)->num_int_regs,0-I386_OP_IMMEDIATE(src1),NULL,NULL);
	}
	else
	{
	  /* lea offset(%reg1),%reg2 */
	  if (I386_OP_FLAGS(src1) & I386_OPFLAG_ISRELOCATED)
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),I386_OP_BASE(src1),0, RELOC_REF_RELOC(I386_INS_REFERS_TO(ins)), NULL);
	  else
	    EquationsAdd(cfg,eqs,I386_OP_BASE(dest),I386_OP_BASE(src1),0-I386_OP_IMMEDIATE(src1),NULL,NULL);
	}
	return;
      }
      break;
    case I386_INSB: case I386_INSD:
    case I386_OUTSB: case I386_OUTSD:
    case I386_CMPSB: case I386_CMPSD:
    case I386_SCASB: case I386_SCASD:
    case I386_MOVSB: case I386_MOVSD:
    case I386_LODSB: case I386_LODSD:
    case I386_STOSB: case I386_STOSD:
      /* we don't know if the D flag is set, so we don't know whether 
       * %esi and %edi increase or decrease */
      break;
    case I386_POP:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,-4,NULL,NULL);
      /* invalidate register into which the stack is popped */
      if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
	if (I386IsGeneralPurposeReg(I386_OP_BASE(I386_INS_DEST(ins))))
	  EquationsInvalidate(cfg,eqs,I386_OP_BASE(I386_INS_DEST(ins)));
      return;
    case I386_POPF:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,-4,NULL,NULL);
      return;
    case I386_POPA:
      /* invalidate all registers apart from %esp, to which 32 is added */
      RegsetSetDup(tmp,CFG_DESCRIPTION(BBL_CFG(I386_INS_BBL(ins)))->int_registers);
      RegsetSetIntersect(tmp,I386_INS_REGS_DEF(ins));
      RegsetSetSubReg(tmp,I386_REG_ESP);
      EquationsInvalidateRegset(cfg,eqs,tmp);
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,-32,NULL,NULL);
      return;
    case I386_PUSH:
    case I386_PUSHF:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,+4,NULL,NULL);
      return;
    case I386_PUSHA:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,+32,NULL,NULL);
      return;
    case I386_LEAVE:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_EBP,0,NULL,NULL);
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,-4,NULL,NULL);
      EquationsInvalidate(cfg,eqs,I386_REG_EBP);
      return;
    case I386_CALL:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,+4,NULL,NULL);
      return;
    case I386_RET:
      EquationsAdd(cfg,eqs,I386_REG_ESP,I386_REG_ESP,-4,NULL,NULL);
      return;
    default:
      /* keep the compiler happy */
      break;
  }

  RegsetSetDup(tmp,CFG_DESCRIPTION(BBL_CFG(I386_INS_BBL(ins)))->int_registers);
  RegsetSetIntersect(tmp,I386_INS_REGS_DEF(ins));
  EquationsInvalidateRegset(cfg,eqs,tmp);
} /* }}} */

static void 
I386CopyInstructionPropagatorWrapper(t_ins * ins, t_equations eqs, t_bool ignore_condition)
{
  I386CopyInstructionPropagator (T_I386_INS(ins), eqs, ignore_condition);
}

extern void I386GetFirstInsOfConditionalBranchWithSideEffect(t_bbl * bbl,t_ins ** ins);
void I386CopyAnalysisInit(t_cfg * cfg)
{
	CFG_SET_COPY_INSTRUCTION_PROPAGATOR(cfg, I386CopyInstructionPropagatorWrapper);
	CFG_SET_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg,I386GetFirstInsOfConditionalBranchWithSideEffect);
}
/* vim: set shiftwidth=2 foldmethod=marker : */
