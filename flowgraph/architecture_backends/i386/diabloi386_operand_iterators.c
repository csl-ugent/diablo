#include <diabloi386.h>
/*#define sIMM8_MIN -128
#define sIMM8_MAX 127
#define IMM8_MAX 256
#define IMM16_MAX 65536*/

/* {{{ Helper functions */
/* {{{ R8 */ 
t_bool I386OpNextR8(t_i386_operand * op){
  /*first*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_REGMODE(op) = i386_regmode_lo8;
  }
  /*last*/
  else if(I386_OP_BASE(op) == I386_REG_EDX && I386_OP_REGMODE(op) == i386_regmode_hi8){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  /*last of lower order*/
  else if(I386_OP_BASE(op) == I386_REG_EDX){
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_REGMODE(op) = i386_regmode_hi8;
  }
  /*next*/
  else I386_OP_BASE(op)++;

  return TRUE;
}
 /* }}} */

/* {{{ R16 */ 
t_bool I386OpNextR16(t_i386_operand * op){
  /*first*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_REGMODE(op) = i386_regmode_lo16;
  }
  /*last*/
  else if(I386_OP_BASE(op) == I386_REG_EDI){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE; 
  }
  else I386_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ R32 */ 
t_bool I386OpNextR32(t_i386_operand * op){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_REGMODE(op) = i386_regmode_full32;
  }
  /*last*/
  else if(I386_OP_BASE(op) == I386_REG_EDI){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  else I386_OP_BASE(op)++;
  
  return TRUE;
}
/* }}} */

/* {{{ Auxiliary functions for M0 */
t_bool I386OpNextImmediate(t_i386_operand * op, t_immediates * imm){
  if(op->super == imm->count){
    op->super = 0;
    return FALSE;
  }
  else{
    I386_OP_IMMEDIATE(op)=imm->imms[op->super++];
    return TRUE;
  }
}

t_bool I386OpNextScale(t_i386_operand * op){
  if(I386_OP_SCALE(op)==I386_SCALE_INVALID){
    I386_OP_SCALE(op)=0;
    return TRUE;
  }
  else if (I386_OP_SCALE(op)==0){
    I386_OP_SCALE(op)=I386_SCALE_INVALID;
    return FALSE;
  }
/*  else if (I386_OP_SCALE(op)==0){
    I386_OP_SCALE(op)=1; 
    return TRUE;
  }*/
  FATAL(("shouldnt happen"));
  return TRUE;
}

t_bool I386OpNextBase(t_i386_operand * op){
  if(I386_OP_BASE(op)==I386_REG_INVALID){
    I386_OP_BASE(op)=I386_REG_NONE;
    return TRUE;
  }
  else if(I386_OP_BASE(op)==I386_REG_NONE){
    I386_OP_BASE(op)=I386_REG_EAX;
    return TRUE;
  }
  else if(I386_OP_BASE(op)==I386_REG_EDI){
    I386_OP_BASE(op)=I386_REG_INVALID;
    return FALSE;
  }
  else{
    I386_OP_BASE(op)++;
    return TRUE;
  }
}

t_bool I386OpNextIndex(t_i386_operand * op){
  if(I386_OP_INDEX(op)==I386_REG_INVALID){
    /*when no index is used scale is not used, thus only one possibility*/
    if(I386_OP_SCALE(op)==0)
      I386_OP_INDEX(op)=I386_REG_NONE;
    else I386_OP_INDEX(op)=I386_REG_EAX;
    
    return TRUE;
  }
  else if(I386_OP_INDEX(op)==I386_REG_NONE){
    I386_OP_INDEX(op)=I386_REG_EAX;
    return TRUE;
  }
  else if(I386_OP_INDEX(op)==I386_REG_EDI){
    I386_OP_INDEX(op)=I386_REG_INVALID;
    return FALSE;
  }
  else{
    I386_OP_INDEX(op)++;
    return TRUE;
  }
}
/*}}}*/

/* {{{ M0 */ 
t_bool I386OpNextM0(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 0;
    I386_OP_IMMEDSIZE(op) = 4;
    I386_OP_BASE(op) = I386_REG_NONE;
    I386_OP_INDEX(op) = I386_REG_NONE;
    I386_OP_SCALE(op) = 0;
    op->super=0;
  }
  
  if(imm->count == 0){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }

  if(!I386OpNextImmediate(op, imm)){
    if(!I386OpNextIndex(op)){
      if(!I386OpNextBase(op)){
	if(!I386OpNextScale(op)){
	  I386_OP_TYPE(op)=i386_optype_invalid;
	  return FALSE;
	}
	else{
	  if(!I386OpNextImmediate(op,imm)||!I386OpNextIndex(op)||!I386OpNextBase(op))
	    FATAL(("shouldnt happen"));
	}
      }
      else{
	if(!I386OpNextImmediate(op,imm)||!I386OpNextIndex(op))
	FATAL(("shouldnt happen"));
      }
    }
    else{
      if(!I386OpNextImmediate(op,imm))
	FATAL(("shouldnt happen"));
    }
  }
  return TRUE;
}
  
/*  else if(I386_OP_BASE(op)==I386_REG_ESP && I386_OP_INDEX(op)==I386_REG_ESP && I386_OP_SCALE(op)==3 && op->super == imm->count){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  else{
    if(op->super = imm->count && I386_OP_SCALE(op)==3 && I386_OP_INDEX(op)==I386_REG_ESP){
      I386_OP_BASE(op)++;
      I386_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
      I386_OP_SCALE(op) = 0;
      I386_OP_INDEX(op)=0;

    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3){
      I386_OP_INDEX(op)++;
      I386_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
      I386_OP_SCALE(op) = 0;
    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SCALE(op)++;
      I386_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
    }
    else {
      I386_OP_IMMEDIATE(op)=imm->imms[op->super++];
    }
  }  
  return TRUE;
}*/
/*  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 0;
    I386_OP_IMMEDSIZE(op) = 4;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_INDEX(op) = I386_REG_EAX;
    I386_OP_IMMEDIATE(op) = I386_MIN_UINT32;
    I386_OP_SCALE(op) = 0;
  }
  //last
  else if(I386_OP_BASE(op)==I386_REG_ESP && I386_OP_INDEX(op)==I386_REG_ESP && I386_OP_SCALE(op)==3 && I386_OP_IMMEDIATE(op)==(t_uint32)I386_MAX_UINT32){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3 && I386_OP_INDEX(op)==I386_REG_ESP){
      I386_OP_BASE(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
      I386_OP_INDEX(op)=0;

    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3){
      I386_OP_INDEX(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SCALE(op)++;
      I386_OP_IMMEDIATE(op)=0;
    }
    else {I386_OP_IMMEDIATE(op)++;
    }
  }  
  return TRUE;
}*/
/* }}} */ 

/* {{{ M8 */  
t_bool I386OpNextM8(t_i386_operand * op){
  /*TODO turned off*/
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
/*  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 1;
    I386_OP_IMMEDSIZE(op) = 4;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_INDEX(op) = I386_REG_EAX;
    I386_OP_IMMEDIATE(op) = 0;
    I386_OP_SCALE(op) = 0;
  }
  //last
  else if(I386_OP_BASE(op)==I386_REG_ESP && I386_OP_INDEX(op)==I386_REG_ESP && I386_OP_SCALE(op)==3 && I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3 && I386_OP_INDEX(op)==I386_REG_ESP){
      I386_OP_BASE(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
      I386_OP_INDEX(op)=0;

    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3){
      I386_OP_INDEX(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SCALE(op)++;
      I386_OP_IMMEDIATE(op)=0;
    }
    else I386_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/* }}} */

/* {{{ M16 */ 
t_bool I386OpNextM16(t_i386_operand * op){
  /*TODO turned off*/
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
  /*first */
/*  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 2;
    I386_OP_IMMEDSIZE(op) = 4;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_INDEX(op) = I386_REG_EAX;
    I386_OP_IMMEDIATE(op) = 0;
    I386_OP_SCALE(op) = 0;
  }
  //last
  else if(I386_OP_BASE(op)==I386_REG_ESP && I386_OP_INDEX(op)==I386_REG_ESP && I386_OP_SCALE(op)==3 && I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3 && I386_OP_INDEX(op)==I386_REG_ESP){
      I386_OP_BASE(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
      I386_OP_INDEX(op)=0;

    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3){
      I386_OP_INDEX(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SCALE(op)++;
      I386_OP_IMMEDIATE(op)=0;
    }
    else I386_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/* }}} */

/* {{{ M32 */ 
t_bool I386OpNextM32(t_i386_operand * op){
  /*TODO turned off*/
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
/*
  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 4;
    I386_OP_IMMEDSIZE(op) = 4;
    I386_OP_BASE(op) = I386_REG_EAX;
    I386_OP_INDEX(op) = I386_REG_EAX;
    I386_OP_IMMEDIATE(op) = 0;
    I386_OP_SCALE(op) = 0;
  }
  //last
  else if(I386_OP_BASE(op)==I386_REG_ESP && I386_OP_INDEX(op)==I386_REG_ESP && I386_OP_SCALE(op)==3 && I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3 && I386_OP_INDEX(op)==I386_REG_ESP){
      I386_OP_BASE(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
      I386_OP_INDEX(op)=0;

    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SCALE(op)==3){
      I386_OP_INDEX(op)++;
      I386_OP_IMMEDIATE(op) = 0;
      I386_OP_SCALE(op) = 0;
    }
    else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SCALE(op)++;
      I386_OP_IMMEDIATE(op)=0;
    }
    else I386_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/*  }}} */ 

/* {{{ M48 */
t_bool I386OpNextM48(t_i386_operand * op){
  /*TODO turned off*/
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
/*
  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_MEMOPSIZE(op) = 6;
    I386_OP_IMMEDIATE(op) = 0;
    I386_OP_SEGSELECTOR(op) =0;
  }
  //last
  else if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32 && I386_OP_SEGSELECTOR(op)==I386_MAX_UINT16){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(I386_OP_IMMEDIATE(op)==I386_MAX_UINT32){
      I386_OP_SEGSELECTOR(op)++;
      I386_OP_IMMEDIATE(op) = 0;
    }
    else I386_OP_IMMEDIATE(op)++;
  }  
  return TRUE;*/
}
/* }}} */

/* {{{ M64 */
t_bool I386OpNextM64(t_i386_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ M80 */ 
t_bool I386OpNextM80(t_i386_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ RM8 */  
t_bool I386OpNextRM8(t_i386_operand * op){
  if(I386_OP_TYPE(op)==i386_optype_invalid || I386_OP_TYPE(op)==i386_optype_reg){
    if(!I386OpNextR8(op))
      return I386OpNextM8(op);
    else return TRUE;
  }
  else if(I386_OP_TYPE(op)==i386_optype_mem){
      return I386OpNextM8(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;

  } 
}
/* }}} */

/* {{{ RM16 */ 
t_bool I386OpNextRM16(t_i386_operand * op){
  if(I386_OP_TYPE(op)==i386_optype_invalid || I386_OP_TYPE(op)==i386_optype_reg){
    if (!I386OpNextR16(op))
      return I386OpNextM16(op);
    else return TRUE;
  }
  else if(I386_OP_TYPE(op)==i386_optype_mem){
      return I386OpNextM16(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;
  }
}
/* }}} */

/* {{{ RM32 */
t_bool I386OpNextRM32(t_i386_operand * op){
  if(I386_OP_TYPE(op)==i386_optype_invalid || I386_OP_TYPE(op)==i386_optype_reg){
    if(!I386OpNextR32(op))
      return I386OpNextM32(op);
    else return TRUE;
  }
  else if(I386_OP_TYPE(op)==i386_optype_mem){
      return I386OpNextM32(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;
  } 
}
/* }}} */ 

/* {{{ SR */
t_bool I386OpNextSR(t_i386_operand * op){
  /*first*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_ES;
    I386_OP_REGMODE(op) = i386_regmode_lo16;
  }
  /*last*/
  else if(I386_OP_BASE(op) == I386_REG_GS){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  else I386_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ I8 */
t_bool I386OpNextI8(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 1;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] < 256){
      I386_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
}
/*  
  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDIATE(op) = IMM8_MIN;
    I386_OP_IMMEDSIZE(op) = 1;
  }
  //last
  else if(I386_OP_IMMEDIATE(op) == IMM8_MAX){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else I386_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{ sI8 */
t_bool I386OpNextsI8(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 1;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] >= -128 && imm->imms[op->super] < 128){
      I386_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
}

t_bool I386OpNextsI16(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 2;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] >= -32768 && imm->imms[op->super] < 32768){
      I386_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
}

t_bool I386OpNextsI32(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 4;
    op->super=0;
  }
  while(op->super != imm->count){
    /* No use to check bounds here, int is 32 bit */
    /*if((imm->imms[op->super]+1) >= -2147483647 && imm->imms[op->super] <= 2147483647){*/
    I386_OP_IMMEDIATE(op) = imm->imms[op->super];
    op->super++;
    return TRUE;
    /*}*/
    op->super++;
  }
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
}

/*  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDIATE(op) = UIMM8_MIN;
    I386_OP_IMMEDSIZE(op) = 1;
  }
  //last
  else if(I386_OP_IMMEDIATE(op) == UIMM8_MAX){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else I386_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{ I16 */
t_bool I386OpNextI16(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 2;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] < 65536){
      I386_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  I386_OP_TYPE(op)=i386_optype_invalid;
  return FALSE;
}
/*  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDIATE(op) = IMM16_MIN;
    I386_OP_IMMEDSIZE(op) = 2;
  }
  //last
  else if(I386_OP_IMMEDIATE(op) == IMM16_MAX){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  //next
  else I386_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{  I32 */ 
t_bool I386OpNextI32(t_i386_operand * op, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDSIZE(op) = 4;
    op->super=0;
  }
  if(op->super == imm->count){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  
  I386_OP_IMMEDIATE(op) = imm->imms[op->super];
  op->super++;
  return TRUE;
}
/*  //first
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDIATE(op) = IMM32_MIN;
    I386_OP_IMMEDSIZE(op) = 4;
    op->super = 0;
  }
  //last
  else if(op->super == 0){
    if(I386_OP_IMMEDIATE(op) == IMM32_MAX){
      if(imm->count == 0){
	I386_OP_TYPE(op)=i386_optype_invalid;
	return FALSE;
      }
      else{
	I386_OP_IMMEDIATE(op) = imm->imms[op->super++];
      }
    }
    else I386_OP_IMMEDIATE(op)++;
  }
  else if(op->super == imm->count){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  else I386_OP_IMMEDIATE(op) = imm->imms[op->super++];
*/
  
  /*else if(op->super == imm->count){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  else if(op->super != 0 || I386_OP_IMMEDIATE(op) == IMM32_MAX){
    I386_OP_IMMEDIATE(op) = imm->imms[op->super++];
  }
  //next
  else I386_OP_IMMEDIATE(op)++;*/
/*
  return TRUE;
}*/
/* }}} */

/* {{{ J8 */
t_bool I386OpNextJ8(t_i386_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ J32 */
t_bool I386OpNextJ32(t_i386_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */
/* }}} */

/* {{{ Actual Functions*/
/* {{{ None */
t_bool I386OpNextNone(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op)=i386_optype_none;
    return TRUE;
  }
  /*only one possibility*/
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  } 
}
/* }}} */

/* {{{  Reg */
t_bool I386OpNextReg(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    
    I386_OP_TYPE(op) = i386_optype_reg;
    
    switch(bm){
      case bm_AL: case bm_AH: case bm_AX: case bm_EAX: case bm_eAX:
	I386_OP_BASE(op) = I386_REG_EAX;
	break;
      case bm_BL: case bm_BH: case bm_BX: case bm_EBX: case bm_eBX:
	I386_OP_BASE(op) = I386_REG_EBX;
	break;
      case bm_CL: case bm_CH: case bm_CX: case bm_ECX: case bm_eCX:
	I386_OP_BASE(op) = I386_REG_ECX;
	break;
      case bm_DL: case bm_DH: case bm_DX: case bm_EDX: case bm_eDX:
	I386_OP_BASE(op) = I386_REG_EDX;
	break;
      case bm_eSI: case bm_ESI:
	I386_OP_BASE(op) = I386_REG_ESI;
	break;
      case bm_eDI: case bm_EDI:
	I386_OP_BASE(op) = I386_REG_EDI;
	break;
      case bm_eSP: case bm_ESP:
	I386_OP_BASE(op) = I386_REG_ESP;
	break;
      case bm_eBP: case bm_EBP:
	I386_OP_BASE(op) = I386_REG_EBP;
	break;
      case bm_CS:
	I386_OP_BASE(op) = I386_REG_CS;
	break;
      case bm_DS:
	I386_OP_BASE(op) = I386_REG_DS;
	break;
      case bm_ES:
	I386_OP_BASE(op) = I386_REG_ES;
	break;
      case bm_FS:
	I386_OP_BASE(op) = I386_REG_FS;
	break;
      case bm_GS:
	I386_OP_BASE(op) = I386_REG_GS;
	break;
      case bm_SS:
	I386_OP_BASE(op) = I386_REG_SS;
	break;
      default:
	FATAL(("unknown byte mode"));
    }
    
    switch (bm)
    {
      case bm_AL: case bm_BL: case bm_CL: case bm_DL:
	I386_OP_REGMODE(op) = i386_regmode_lo8;
	break;
      case bm_AH: case bm_BH: case bm_CH: case bm_DH:
	I386_OP_REGMODE(op) = i386_regmode_hi8;
	break;
      case bm_AX: case bm_BX: case bm_CX: case bm_DX:
      case bm_CS: case bm_DS: case bm_ES: case bm_FS: case bm_GS: case bm_SS:
	I386_OP_REGMODE(op) = i386_regmode_lo16;
	break;
      case bm_EAX: case bm_EBX: case bm_ECX: case bm_EDX:
      case bm_ESI: case bm_EDI: case bm_ESP: case bm_EBP:
	I386_OP_REGMODE(op) = i386_regmode_full32;
	break;
      case bm_eAX: case bm_eBX: case bm_eCX: case bm_eDX:
      case bm_eSI: case bm_eDI: case bm_eSP: case bm_eBP:
	/* these depend on the operand size prefix */
	if (I386_OPSZPREF(ins))
	  I386_OP_REGMODE(op) = i386_regmode_lo16;
	else
	  I386_OP_REGMODE(op) = i386_regmode_full32;
	break;
      default:
	FATAL(("Invalid bytemode %d", bm));
    }
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ ST */
t_bool I386OpNextST(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_REGMODE(op) = i386_regmode_full32;

    switch (bm){
      case bm_ST:
      case bm_ST0:
	I386_OP_BASE(op) = I386_REG_ST0;
	break;
      case bm_ST1:
	I386_OP_BASE(op) = I386_REG_ST1;
	break;
      case bm_ST2:
	I386_OP_BASE(op) = I386_REG_ST2;
	break;
      case bm_ST3:
	I386_OP_BASE(op) = I386_REG_ST3;
	break;
      case bm_ST4:
	I386_OP_BASE(op) = I386_REG_ST4;
	break;
      case bm_ST5:
	I386_OP_BASE(op) = I386_REG_ST5;
	break;
      case bm_ST6:
	I386_OP_BASE(op) = I386_REG_ST6;
	break;
      case bm_ST7:
	I386_OP_BASE(op) = I386_REG_ST7;
	break;
      default:
	FATAL(("unknown bytemode"));
    }
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ Const1 */
t_bool I386OpNextConst1(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_imm;
    I386_OP_IMMEDIATE(op) = 1;
    I386_OP_IMMEDSIZE(op) = 4;
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ A */
t_bool I386OpNextA(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  FATAL(("only used in call far and jump far, not supported"));
  return FALSE;
}
/* }}}  */ 

/* {{{ C*/
/*possibilities: CR0, CR2, CR3 and CR4*/
t_bool I386OpNextC(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  ASSERT(bm == bm_d, ("unknown bytemode"));
  
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_CR0;
    I386_OP_REGMODE(op) = i386_regmode_full32;
  }
  /*done*/
  else if(I386_OP_BASE(op)==I386_REG_CR4){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  /*next*/
  else if(I386_OP_BASE(op)==I386_REG_CR0){
    I386_OP_BASE(op) = I386_REG_CR2;
  }
  else if(I386_OP_BASE(op)==I386_REG_CR2){
    I386_OP_BASE(op) = I386_REG_CR3;
  }
  else if(I386_OP_BASE(op)==I386_REG_CR3){
    I386_OP_BASE(op) = I386_REG_CR4;
  }
  return TRUE;
}
/* }}} */

/* {{{ D*/
/*possibilities: DR0 - DR7*/
t_bool I386OpNextD(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  ASSERT(bm == bm_d, ("unknown bytemode"));
  
  /*first pass*/
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_reg;
    I386_OP_BASE(op) = I386_REG_DR0;
    I386_OP_REGMODE(op) = i386_regmode_full32;
  }
  /*done*/
  else if(I386_OP_BASE(op)==I386_REG_DR7){
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;
  }
  /*next*/
  else 
    I386_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ E*/
t_bool I386OpNextE(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  switch(bm){
    case bm_b:
      return I386OpNextRM8(op);
    case bm_w:
      return I386OpNextRM16(op);
    case bm_d:
      return I386OpNextRM32(op);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextRM16(op);
      else return I386OpNextRM32(op);
    case bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ F*/
t_bool I386OpNextF(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  FATAL(("only used in call far and jump far, not supported"));
  return FALSE;
}
/* }}} */

/* {{{ G*/
t_bool I386OpNextG(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  switch(bm){
    case bm_b:
      return I386OpNextR8(op);
    case bm_w:
      return I386OpNextR16(op);
    case bm_d:
      return I386OpNextR32(op);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextR16(op);
      else{
	return I386OpNextR32(op);
      }	
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ sI*/
t_bool I386OpNextsI(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  switch(bm){
    case bm_b:
      return I386OpNextsI8(op, imm);
    case bm_w:
      return I386OpNextsI16(op, imm);
    case bm_d:
      return I386OpNextsI32(op, imm);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextsI16(op, imm);
      else return I386OpNextsI32(op, imm);
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}} */ 

/* {{{ I*/
t_bool I386OpNextI(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  switch(bm){
    case bm_b:
      return I386OpNextI8(op, imm);
    case bm_w:
      return I386OpNextI16(op, imm);
    case bm_d:
      return I386OpNextI32(op, imm);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextI16(op, imm);
      else return I386OpNextI32(op, imm);
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}}  */

/* {{{ J*/
t_bool I386OpNextJ(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  switch(bm){
    case bm_b:
      return I386OpNextJ8(op);
    case bm_v:
      if (I386_OPSZPREF(ins)){
	FATAL(("currently not supported"));
	return FALSE;
      }
      else return I386OpNextJ32(op);    
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}}  */

/* {{{ M*/
t_bool I386OpNextM(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  switch(bm){
    case 0:
      return I386OpNextM0(op, imm);
    case bm_b:
      return I386OpNextM8(op);
    case bm_a:
      if (I386_OPSZPREF(ins))
	return I386OpNextM32(op);
      else return I386OpNextM64(op);
    case bm_p:
      if (I386_OPSZPREF(ins))
	return I386OpNextM32(op);
      else return I386OpNextM48(op);
    case bm_s:
      return I386OpNextM48(op);
      
    case bm_sr:
      return I386OpNextM32(op);
    case bm_dr:
      return I386OpNextM64(op);
    case bm_er:
    case bm_bcd:
      return I386OpNextM80(op);
    case bm_w:
      return I386OpNextM16(op);
    case bm_d:
      return I386OpNextM32(op);
    case bm_q:
      return I386OpNextM64(op);
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ O*/
t_bool I386OpNextO(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  if (I386_ADSZPREF(ins))
  {
    /* 16-bit offset */
    return I386OpNextM16(op);
  }
  else
  {
    /* 32-bit offset */
    return I386OpNextM32(op);
  }
}
/* }}} */

/* {{{ R*/
t_bool I386OpNextR(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  return I386OpNextR32(op);
}
/* }}} */

/* {{{ S*/
t_bool I386OpNextS(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  return I386OpNextSR(op);
}
/* }}} */

/* {{{ X*/
t_bool I386OpNextX(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_REGMODE(op) = i386_regmode_full32;
    I386_OP_BASE(op) = I386_REG_ESI;
    I386_OP_INDEX(op) = I386_REG_NONE;
    I386_OP_IMMEDIATE(op) = 0;
    if (bm == bm_b)
      I386_OP_MEMOPSIZE(op) = 1;
    else if (bm == bm_v && I386_OPSZPREF(ins))
      I386_OP_MEMOPSIZE(op) = 2;
    else if (bm == bm_v && !I386_OPSZPREF(ins))
      I386_OP_MEMOPSIZE(op) = 4;
    else 
      FATAL (("unknown bytemode"));
    return TRUE;
  }
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;  
  }
}
/* }}} */

/* {{{ Y*/
t_bool I386OpNextY(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  if(I386_OP_TYPE(op)==i386_optype_invalid){
    /* memory addressed by ES:DI */
    I386_OP_TYPE(op) = i386_optype_mem;
    I386_OP_REGMODE(op) = i386_regmode_full32;
    I386_OP_BASE(op) = I386_REG_EDI;
    I386_OP_INDEX(op) = I386_REG_NONE;
    I386_OP_IMMEDIATE(op) = 0;
    if (bm == bm_b)
      I386_OP_MEMOPSIZE(op) = 1;
    else if (bm == bm_v && I386_OPSZPREF(ins))
      I386_OP_MEMOPSIZE(op) = 2;
    else if (bm == bm_v && !I386_OPSZPREF(ins))
      I386_OP_MEMOPSIZE(op) = 4;
    else 
      FATAL (("unknown bytemode"));
    return TRUE;
  }
  
  else{
    I386_OP_TYPE(op)=i386_optype_invalid;
    return FALSE;  
  }
}
/* }}} */


t_bool I386OpNextV(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  switch(bm){
    case bm_b:
      return I386OpNextRM8(op);
    case bm_w:
      return I386OpNextRM16(op);
    case bm_d:
      return I386OpNextRM32(op);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextRM16(op);
      else return I386OpNextRM32(op);
    case bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}

t_bool I386OpNextW(t_i386_ins * ins, t_i386_operand * op, t_i386_bytemode bm, t_immediates * imm){
  /*first pass*/
  switch(bm){
    case bm_b:
      return I386OpNextRM8(op);
    case bm_w:
      return I386OpNextRM16(op);
    case bm_d:
      return I386OpNextRM32(op);
    case bm_v:
      if(I386_OPSZPREF(ins))
	return I386OpNextRM16(op);
      else return I386OpNextRM32(op);
    case bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}


/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker: */
