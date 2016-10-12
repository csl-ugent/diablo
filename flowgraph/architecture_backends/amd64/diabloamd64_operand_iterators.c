#include <diabloamd64.h>
/*#define sIMM8_MIN -128
#define sIMM8_MAX 127
#define IMM8_MAX 256
#define IMM16_MAX 65536*/

/* {{{ Helper functions */
/* {{{ R8 */ 
t_bool Amd64OpNextR8(t_amd64_operand * op){
  /*first*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_REGMODE(op) = amd64_regmode_lo8;
  }
  /*last*/
  else if(AMD64_OP_BASE(op) == AMD64_REG_RDX && AMD64_OP_REGMODE(op) == amd64_regmode_hi8){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  /*last of lower order*/
  else if(AMD64_OP_BASE(op) == AMD64_REG_RDX){
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_REGMODE(op) = amd64_regmode_hi8;
  }
  /*next*/
  else AMD64_OP_BASE(op)++;

  return TRUE;
}
 /* }}} */

/* {{{ R16 */ 
t_bool Amd64OpNextR16(t_amd64_operand * op){
  /*first*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
  }
  /*last*/
  else if(AMD64_OP_BASE(op) == AMD64_REG_RDI){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE; 
  }
  else AMD64_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ R32 */ 
t_bool Amd64OpNextR32(t_amd64_operand * op){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
  }
  /*last*/
  else if(AMD64_OP_BASE(op) == AMD64_REG_RDI){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  else AMD64_OP_BASE(op)++;
  
  return TRUE;
}
/* }}} */

/* {{{ Auxiliary functions for M0 */
t_bool Amd64OpNextImmediate(t_amd64_operand * op, t_amd64_immediates * imm){
  if(op->super == imm->count){
    op->super = 0;
    return FALSE;
  }
  else{
    AMD64_OP_IMMEDIATE(op)=imm->imms[op->super++];
    return TRUE;
  }
}

t_bool Amd64OpNextScale(t_amd64_operand * op){
  if(AMD64_OP_SCALE(op)==AMD64_SCALE_INVALID){
    AMD64_OP_SCALE(op)=0;
    return TRUE;
  }
  else if (AMD64_OP_SCALE(op)==0){
    AMD64_OP_SCALE(op)=AMD64_SCALE_INVALID;
    return FALSE;
  }
/*  else if (AMD64_OP_SCALE(op)==0){
    AMD64_OP_SCALE(op)=1; 
    return TRUE;
  }*/
  FATAL(("shouldnt happen"));
  return TRUE;
}

t_bool Amd64OpNextBase(t_amd64_operand * op){
  if(AMD64_OP_BASE(op)==AMD64_REG_INVALID){
    AMD64_OP_BASE(op)=AMD64_REG_NONE;
    return TRUE;
  }
  else if(AMD64_OP_BASE(op)==AMD64_REG_NONE){
    AMD64_OP_BASE(op)=AMD64_REG_RAX;
    return TRUE;
  }
  else if(AMD64_OP_BASE(op)==AMD64_REG_RDI){
    AMD64_OP_BASE(op)=AMD64_REG_INVALID;
    return FALSE;
  }
  else{
    AMD64_OP_BASE(op)++;
    return TRUE;
  }
}

t_bool Amd64OpNextIndex(t_amd64_operand * op){
  if(AMD64_OP_INDEX(op)==AMD64_REG_INVALID){
    /*when no index is used scale is not used, thus only one possibility*/
    if(AMD64_OP_SCALE(op)==0)
      AMD64_OP_INDEX(op)=AMD64_REG_NONE;
    else AMD64_OP_INDEX(op)=AMD64_REG_RAX;
    
    return TRUE;
  }
  else if(AMD64_OP_INDEX(op)==AMD64_REG_NONE){
    AMD64_OP_INDEX(op)=AMD64_REG_RAX;
    return TRUE;
  }
  else if(AMD64_OP_INDEX(op)==AMD64_REG_RDI){
    AMD64_OP_INDEX(op)=AMD64_REG_INVALID;
    return FALSE;
  }
  else{
    AMD64_OP_INDEX(op)++;
    return TRUE;
  }
}
/*}}}*/

/* {{{ M0 */ 
t_bool Amd64OpNextM0(t_amd64_operand * op, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 0;
    AMD64_OP_IMMEDSIZE(op) = 4;
    AMD64_OP_BASE(op) = AMD64_REG_NONE;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;
    AMD64_OP_SCALE(op) = 0;
    op->super=0;
  }
  
  if(imm->count == 0){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }

  if(!Amd64OpNextImmediate(op, imm)){
    if(!Amd64OpNextIndex(op)){
      if(!Amd64OpNextBase(op)){
	if(!Amd64OpNextScale(op)){
	  AMD64_OP_TYPE(op)=amd64_optype_invalid;
	  return FALSE;
	}
	else{
	  if(!Amd64OpNextImmediate(op,imm)||!Amd64OpNextIndex(op)||!Amd64OpNextBase(op))
	    FATAL(("shouldnt happen"));
	}
      }
      else{
	if(!Amd64OpNextImmediate(op,imm)||!Amd64OpNextIndex(op))
	FATAL(("shouldnt happen"));
      }
    }
    else{
      if(!Amd64OpNextImmediate(op,imm))
	FATAL(("shouldnt happen"));
    }
  }
  return TRUE;
}
  
/*  else if(AMD64_OP_BASE(op)==AMD64_REG_RSP && AMD64_OP_INDEX(op)==AMD64_REG_RSP && AMD64_OP_SCALE(op)==3 && op->super == imm->count){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  else{
    if(op->super = imm->count && AMD64_OP_SCALE(op)==3 && AMD64_OP_INDEX(op)==AMD64_REG_RSP){
      AMD64_OP_BASE(op)++;
      AMD64_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
      AMD64_OP_SCALE(op) = 0;
      AMD64_OP_INDEX(op)=0;

    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3){
      AMD64_OP_INDEX(op)++;
      AMD64_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
      AMD64_OP_SCALE(op) = 0;
    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SCALE(op)++;
      AMD64_OP_IMMEDIATE(op)=imm->imms[0];
      op->super=1;
    }
    else {
      AMD64_OP_IMMEDIATE(op)=imm->imms[op->super++];
    }
  }  
  return TRUE;
}*/
/*  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 0;
    AMD64_OP_IMMEDSIZE(op) = 4;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_INDEX(op) = AMD64_REG_RAX;
    AMD64_OP_IMMEDIATE(op) = AMD64_MIN_UINT32;
    AMD64_OP_SCALE(op) = 0;
  }
  //last
  else if(AMD64_OP_BASE(op)==AMD64_REG_RSP && AMD64_OP_INDEX(op)==AMD64_REG_RSP && AMD64_OP_SCALE(op)==3 && AMD64_OP_IMMEDIATE(op)==(t_uint32)AMD64_MAX_UINT32){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3 && AMD64_OP_INDEX(op)==AMD64_REG_RSP){
      AMD64_OP_BASE(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
      AMD64_OP_INDEX(op)=0;

    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3){
      AMD64_OP_INDEX(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SCALE(op)++;
      AMD64_OP_IMMEDIATE(op)=0;
    }
    else {AMD64_OP_IMMEDIATE(op)++;
    }
  }  
  return TRUE;
}*/
/* }}} */ 

/* {{{ M8 */  
t_bool Amd64OpNextM8(t_amd64_operand * op){
  /*TODO turned off*/
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
/*  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 1;
    AMD64_OP_IMMEDSIZE(op) = 4;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_INDEX(op) = AMD64_REG_RAX;
    AMD64_OP_IMMEDIATE(op) = 0;
    AMD64_OP_SCALE(op) = 0;
  }
  //last
  else if(AMD64_OP_BASE(op)==AMD64_REG_RSP && AMD64_OP_INDEX(op)==AMD64_REG_RSP && AMD64_OP_SCALE(op)==3 && AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3 && AMD64_OP_INDEX(op)==AMD64_REG_RSP){
      AMD64_OP_BASE(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
      AMD64_OP_INDEX(op)=0;

    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3){
      AMD64_OP_INDEX(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SCALE(op)++;
      AMD64_OP_IMMEDIATE(op)=0;
    }
    else AMD64_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/* }}} */

/* {{{ M16 */ 
t_bool Amd64OpNextM16(t_amd64_operand * op){
  /*TODO turned off*/
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
  /*first */
/*  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 2;
    AMD64_OP_IMMEDSIZE(op) = 4;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_INDEX(op) = AMD64_REG_RAX;
    AMD64_OP_IMMEDIATE(op) = 0;
    AMD64_OP_SCALE(op) = 0;
  }
  //last
  else if(AMD64_OP_BASE(op)==AMD64_REG_RSP && AMD64_OP_INDEX(op)==AMD64_REG_RSP && AMD64_OP_SCALE(op)==3 && AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3 && AMD64_OP_INDEX(op)==AMD64_REG_RSP){
      AMD64_OP_BASE(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
      AMD64_OP_INDEX(op)=0;

    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3){
      AMD64_OP_INDEX(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SCALE(op)++;
      AMD64_OP_IMMEDIATE(op)=0;
    }
    else AMD64_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/* }}} */

/* {{{ M32 */ 
t_bool Amd64OpNextM32(t_amd64_operand * op){
  /*TODO turned off*/
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
/*
  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 4;
    AMD64_OP_IMMEDSIZE(op) = 4;
    AMD64_OP_BASE(op) = AMD64_REG_RAX;
    AMD64_OP_INDEX(op) = AMD64_REG_RAX;
    AMD64_OP_IMMEDIATE(op) = 0;
    AMD64_OP_SCALE(op) = 0;
  }
  //last
  else if(AMD64_OP_BASE(op)==AMD64_REG_RSP && AMD64_OP_INDEX(op)==AMD64_REG_RSP && AMD64_OP_SCALE(op)==3 && AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3 && AMD64_OP_INDEX(op)==AMD64_REG_RSP){
      AMD64_OP_BASE(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
      AMD64_OP_INDEX(op)=0;

    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SCALE(op)==3){
      AMD64_OP_INDEX(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
      AMD64_OP_SCALE(op) = 0;
    }
    else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SCALE(op)++;
      AMD64_OP_IMMEDIATE(op)=0;
    }
    else AMD64_OP_IMMEDIATE(op)++;
  }  

  return TRUE;*/
}
/*  }}} */ 

/* {{{ M48 */
t_bool Amd64OpNextM48(t_amd64_operand * op){
  /*TODO turned off*/
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
/*
  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_MEMOPSIZE(op) = 6;
    AMD64_OP_IMMEDIATE(op) = 0;
    AMD64_OP_SEGSELECTOR(op) =0;
  }
  //last
  else if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32 && AMD64_OP_SEGSELECTOR(op)==AMD64_MAX_UINT16){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else{
    if(AMD64_OP_IMMEDIATE(op)==AMD64_MAX_UINT32){
      AMD64_OP_SEGSELECTOR(op)++;
      AMD64_OP_IMMEDIATE(op) = 0;
    }
    else AMD64_OP_IMMEDIATE(op)++;
  }  
  return TRUE;*/
}
/* }}} */

/* {{{ M64 */
t_bool Amd64OpNextM64(t_amd64_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ M80 */ 
t_bool Amd64OpNextM80(t_amd64_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ RM8 */  
t_bool Amd64OpNextRM8(t_amd64_operand * op){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid || AMD64_OP_TYPE(op)==amd64_optype_reg){
    if(!Amd64OpNextR8(op))
      return Amd64OpNextM8(op);
    else return TRUE;
  }
  else if(AMD64_OP_TYPE(op)==amd64_optype_mem){
      return Amd64OpNextM8(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;

  } 
}
/* }}} */

/* {{{ RM16 */ 
t_bool Amd64OpNextRM16(t_amd64_operand * op){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid || AMD64_OP_TYPE(op)==amd64_optype_reg){
    if (!Amd64OpNextR16(op))
      return Amd64OpNextM16(op);
    else return TRUE;
  }
  else if(AMD64_OP_TYPE(op)==amd64_optype_mem){
      return Amd64OpNextM16(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;
  }
}
/* }}} */

/* {{{ RM32 */
t_bool Amd64OpNextRM32(t_amd64_operand * op){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid || AMD64_OP_TYPE(op)==amd64_optype_reg){
    if(!Amd64OpNextR32(op))
      return Amd64OpNextM32(op);
    else return TRUE;
  }
  else if(AMD64_OP_TYPE(op)==amd64_optype_mem){
      return Amd64OpNextM32(op);
  }
  else{
    FATAL(("shouldnt get here"));
    return FALSE;
  } 
}
/* }}} */ 

/* {{{ SR */
t_bool Amd64OpNextSR(t_amd64_operand * op){
  /*first*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_ES;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  }
  /*last*/
  else if(AMD64_OP_BASE(op) == AMD64_REG_SS){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  else AMD64_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ I8 */
t_bool Amd64OpNextI8(t_amd64_operand * op, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDSIZE(op) = 1;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] < 256){
      AMD64_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
}
/*  
  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDIATE(op) = IMM8_MIN;
    AMD64_OP_IMMEDSIZE(op) = 1;
  }
  //last
  else if(AMD64_OP_IMMEDIATE(op) == IMM8_MAX){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else AMD64_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{ sI8 */
t_bool Amd64OpNextsI8(t_amd64_operand * op, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDSIZE(op) = 1;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] >= -128 && imm->imms[op->super] < 128){
      AMD64_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
}
/*  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDIATE(op) = UIMM8_MIN;
    AMD64_OP_IMMEDSIZE(op) = 1;
  }
  //last
  else if(AMD64_OP_IMMEDIATE(op) == UIMM8_MAX){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else AMD64_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{ I16 */
t_bool Amd64OpNextI16(t_amd64_operand * op, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDSIZE(op) = 2;
    op->super=0;
  }
  while(op->super != imm->count){
    if(imm->imms[op->super] < 65536){
      AMD64_OP_IMMEDIATE(op) = imm->imms[op->super];
      op->super++;
      return TRUE;
    }
    op->super++;
  }
  AMD64_OP_TYPE(op)=amd64_optype_invalid;
  return FALSE;
}
/*  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDIATE(op) = IMM16_MIN;
    AMD64_OP_IMMEDSIZE(op) = 2;
  }
  //last
  else if(AMD64_OP_IMMEDIATE(op) == IMM16_MAX){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  //next
  else AMD64_OP_IMMEDIATE(op)++;
  return TRUE;
}*/
/* }}} */

/* {{{  I32 */ 
t_bool Amd64OpNextI32(t_amd64_operand * op, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDSIZE(op) = 4;
    op->super=0;
  }
  if(op->super == imm->count){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  
  AMD64_OP_IMMEDIATE(op) = imm->imms[op->super];
  op->super++;
  return TRUE;
}
/*  //first
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDIATE(op) = IMM32_MIN;
    AMD64_OP_IMMEDSIZE(op) = 4;
    op->super = 0;
  }
  //last
  else if(op->super == 0){
    if(AMD64_OP_IMMEDIATE(op) == IMM32_MAX){
      if(imm->count == 0){
	AMD64_OP_TYPE(op)=amd64_optype_invalid;
	return FALSE;
      }
      else{
	AMD64_OP_IMMEDIATE(op) = imm->imms[op->super++];
      }
    }
    else AMD64_OP_IMMEDIATE(op)++;
  }
  else if(op->super == imm->count){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  else AMD64_OP_IMMEDIATE(op) = imm->imms[op->super++];
*/
  
  /*else if(op->super == imm->count){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  else if(op->super != 0 || AMD64_OP_IMMEDIATE(op) == IMM32_MAX){
    AMD64_OP_IMMEDIATE(op) = imm->imms[op->super++];
  }
  //next
  else AMD64_OP_IMMEDIATE(op)++;*/
/*
  return TRUE;
}*/
/* }}} */

/* {{{ J8 */
t_bool Amd64OpNextJ8(t_amd64_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */

/* {{{ J32 */
t_bool Amd64OpNextJ32(t_amd64_operand * op){
  FATAL(("Currently not supported"));
  return FALSE;
}
/* }}} */
/* }}} */

/* {{{ Actual Functions*/
/* {{{ None */
t_bool Amd64OpNextNone(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op)=amd64_optype_none;
    return TRUE;
  }
  /*only one possibility*/
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  } 
}
/* }}} */

/* {{{  Reg */
t_bool Amd64OpNextReg(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    
    switch(bm){
      case amd64_bm_AL: case amd64_bm_AH: case amd64_bm_AX: case amd64_bm_EAX: case amd64_bm_eAX:
	AMD64_OP_BASE(op) = AMD64_REG_RAX;
	break;
      case amd64_bm_BL: case amd64_bm_BH: case amd64_bm_BX: case amd64_bm_EBX: case amd64_bm_eBX:
	AMD64_OP_BASE(op) = AMD64_REG_RBX;
	break;
      case amd64_bm_CL: case amd64_bm_CH: case amd64_bm_CX: case amd64_bm_RCX: case amd64_bm_eCX:
	AMD64_OP_BASE(op) = AMD64_REG_RCX;
	break;
      case amd64_bm_DL: case amd64_bm_DH: case amd64_bm_DX: case amd64_bm_EDX: case amd64_bm_eDX:
	AMD64_OP_BASE(op) = AMD64_REG_RDX;
	break;
      case amd64_bm_eSI: case amd64_bm_ESI:
	AMD64_OP_BASE(op) = AMD64_REG_RSI;
	break;
      case amd64_bm_eDI: case amd64_bm_EDI:
	AMD64_OP_BASE(op) = AMD64_REG_RDI;
	break;
      case amd64_bm_eSP: case amd64_bm_RSP:
	AMD64_OP_BASE(op) = AMD64_REG_RSP;
	break;
      case amd64_bm_eBP: case amd64_bm_EBP:
	AMD64_OP_BASE(op) = AMD64_REG_RBP;
	break;
      case amd64_bm_CS:
	AMD64_OP_BASE(op) = AMD64_REG_CS;
	break;
      case amd64_bm_DS:
	AMD64_OP_BASE(op) = AMD64_REG_DS;
	break;
      case amd64_bm_ES:
	AMD64_OP_BASE(op) = AMD64_REG_ES;
	break;
      case amd64_bm_FS:
	AMD64_OP_BASE(op) = AMD64_REG_FS;
	break;
      case amd64_bm_GS:
	AMD64_OP_BASE(op) = AMD64_REG_GS;
	break;
      case amd64_bm_SS:
	AMD64_OP_BASE(op) = AMD64_REG_SS;
	break;
      default:
	FATAL(("unknown byte mode"));
    }
    
    switch (bm)
    {
      case amd64_bm_AL: case amd64_bm_BL: case amd64_bm_CL: case amd64_bm_DL:
	AMD64_OP_REGMODE(op) = amd64_regmode_lo8;
	break;
      case amd64_bm_AH: case amd64_bm_BH: case amd64_bm_CH: case amd64_bm_DH:
	AMD64_OP_REGMODE(op) = amd64_regmode_hi8;
	break;
      case amd64_bm_AX: case amd64_bm_BX: case amd64_bm_CX: case amd64_bm_DX:
      case amd64_bm_CS: case amd64_bm_DS: case amd64_bm_ES: case amd64_bm_FS: case amd64_bm_GS: case amd64_bm_SS:
	AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
	break;
      case amd64_bm_EAX: case amd64_bm_EBX: case amd64_bm_RCX: case amd64_bm_EDX:
      case amd64_bm_ESI: case amd64_bm_EDI: case amd64_bm_RSP: case amd64_bm_EBP:
	AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	break;
      case amd64_bm_eAX: case amd64_bm_eBX: case amd64_bm_eCX: case amd64_bm_eDX:
      case amd64_bm_eSI: case amd64_bm_eDI: case amd64_bm_eSP: case amd64_bm_eBP:
	/* these depend on the operand size prefix */
	if (AMD64_OPSZPREF(ins))
	  AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
	else
	  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	break;
      default:
	FATAL(("Invalid bytemode %d", bm));
    }
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ ST */
t_bool Amd64OpNextST(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;

    switch (bm){
      case amd64_bm_ST:
      case amd64_bm_ST0:
	AMD64_OP_BASE(op) = AMD64_REG_ST0;
	break;
      case amd64_bm_ST1:
	AMD64_OP_BASE(op) = AMD64_REG_ST1;
	break;
      case amd64_bm_ST2:
	AMD64_OP_BASE(op) = AMD64_REG_ST2;
	break;
      case amd64_bm_ST3:
	AMD64_OP_BASE(op) = AMD64_REG_ST3;
	break;
      case amd64_bm_ST4:
	AMD64_OP_BASE(op) = AMD64_REG_ST4;
	break;
      case amd64_bm_ST5:
	AMD64_OP_BASE(op) = AMD64_REG_ST5;
	break;
      case amd64_bm_ST6:
	AMD64_OP_BASE(op) = AMD64_REG_ST6;
	break;
      case amd64_bm_ST7:
	AMD64_OP_BASE(op) = AMD64_REG_ST7;
	break;
      default:
	FATAL(("unknown bytemode"));
    }
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ Const1 */
t_bool Amd64OpNextConst1(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_imm;
    AMD64_OP_IMMEDIATE(op) = 1;
    AMD64_OP_IMMEDSIZE(op) = 4;
    return TRUE;
  }
  /* Only one possibility for this type of operand */
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;;
  } 
}
/* }}} */

/* {{{ A */
t_bool Amd64OpNextA(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  FATAL(("only used in call far and jump far, not supported"));
  return FALSE;
}
/* }}}  */ 

/* {{{ C*/
/*possibilities: CR0, CR2, CR3 and CR4*/
t_bool Amd64OpNextC(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  ASSERT(bm == amd64_bm_d, ("unknown bytemode"));
  
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_CR0;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  }
  /*done*/
  else if(AMD64_OP_BASE(op)==AMD64_REG_CR4){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  /*next*/
  else if(AMD64_OP_BASE(op)==AMD64_REG_CR0){
    AMD64_OP_BASE(op) = AMD64_REG_CR2;
  }
  else if(AMD64_OP_BASE(op)==AMD64_REG_CR2){
    AMD64_OP_BASE(op) = AMD64_REG_CR3;
  }
  else if(AMD64_OP_BASE(op)==AMD64_REG_CR3){
    AMD64_OP_BASE(op) = AMD64_REG_CR4;
  }
  return TRUE;
}
/* }}} */

/* {{{ D*/
/*possibilities: DR0 - DR7*/
t_bool Amd64OpNextD(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  ASSERT(bm == amd64_bm_d, ("unknown bytemode"));
  
  /*first pass*/
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_DR0;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  }
  /*done*/
  else if(AMD64_OP_BASE(op)==AMD64_REG_DR7){
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;
  }
  /*next*/
  else 
    AMD64_OP_BASE(op)++;
  return TRUE;
}
/* }}} */

/* {{{ E*/
t_bool Amd64OpNextE(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextRM8(op);
    case amd64_bm_w:
      return Amd64OpNextRM16(op);
    case amd64_bm_d:
      return Amd64OpNextRM32(op);
    case amd64_bm_v:
      if(AMD64_OPSZPREF(ins))
	return Amd64OpNextRM16(op);
      else return Amd64OpNextRM32(op);
    case amd64_bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ F*/
t_bool Amd64OpNextF(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  FATAL(("only used in call far and jump far, not supported"));
  return FALSE;
}
/* }}} */

/* {{{ G*/
t_bool Amd64OpNextG(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextR8(op);
    case amd64_bm_w:
      return Amd64OpNextR16(op);
    case amd64_bm_d:
      return Amd64OpNextR32(op);
    case amd64_bm_v:
      if(AMD64_OPSZPREF(ins))
	return Amd64OpNextR16(op);
      else{
	return Amd64OpNextR32(op);
      }	
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ sI*/
t_bool Amd64OpNextsI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  ASSERT(bm == amd64_bm_b, ("unknown bytemode"));
  return Amd64OpNextsI8(op,imm);
}
/* }}} */ 

/* {{{ I*/
t_bool Amd64OpNextI(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextI8(op, imm);
    case amd64_bm_w:
      return Amd64OpNextI16(op, imm);
    case amd64_bm_d:
      return Amd64OpNextI32(op, imm);
    case amd64_bm_v:
      if(AMD64_OPSZPREF(ins))
	return Amd64OpNextI16(op, imm);
      else return Amd64OpNextI32(op, imm);
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}}  */

/* {{{ J*/
t_bool Amd64OpNextJ(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextJ8(op);
    case amd64_bm_v:
      if (AMD64_OPSZPREF(ins)){
	FATAL(("currently not supported"));
	return FALSE;
      }
      else return Amd64OpNextJ32(op);    
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}}  */

/* {{{ M*/
t_bool Amd64OpNextM(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  switch(bm){
    case 0:
      return Amd64OpNextM0(op, imm);
    case amd64_bm_b:
      return Amd64OpNextM8(op);
    case amd64_bm_a:
      if (AMD64_OPSZPREF(ins))
	return Amd64OpNextM32(op);
      else return Amd64OpNextM64(op);
    case amd64_bm_p:
      if (AMD64_OPSZPREF(ins))
	return Amd64OpNextM32(op);
      else return Amd64OpNextM48(op);
    case amd64_bm_s:
      return Amd64OpNextM48(op);
      
    case amd64_bm_sr:
      return Amd64OpNextM32(op);
    case amd64_bm_dr:
      return Amd64OpNextM64(op);
    case amd64_bm_er:
    case amd64_bm_bcd:
      return Amd64OpNextM80(op);
    case amd64_bm_w:
      return Amd64OpNextM16(op);
    case amd64_bm_d:
      return Amd64OpNextM32(op);
    case amd64_bm_q:
      return Amd64OpNextM64(op);
    default:
      FATAL(("shoudnt get here"));
      return FALSE;
  }
}
/* }}} */

/* {{{ O*/
t_bool Amd64OpNextO(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  if (AMD64_ADSZPREF(ins))
  {
    /* 16-bit offset */
    return Amd64OpNextM16(op);
  }
  else
  {
    /* 32-bit offset */
    return Amd64OpNextM32(op);
  }
}
/* }}} */

/* {{{ R*/
t_bool Amd64OpNextR(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  return Amd64OpNextR32(op);
}
/* }}} */

/* {{{ S*/
t_bool Amd64OpNextS(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  return Amd64OpNextSR(op);
}
/* }}} */

/* {{{ X*/
t_bool Amd64OpNextX(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    AMD64_OP_BASE(op) = AMD64_REG_RSI;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;
    AMD64_OP_IMMEDIATE(op) = 0;
    if (bm == amd64_bm_b)
      AMD64_OP_MEMOPSIZE(op) = 1;
    else if (bm == amd64_bm_v && AMD64_OPSZPREF(ins))
      AMD64_OP_MEMOPSIZE(op) = 2;
    else if (bm == amd64_bm_v && !AMD64_OPSZPREF(ins))
      AMD64_OP_MEMOPSIZE(op) = 4;
    else 
      FATAL (("unknown bytemode"));
    return TRUE;
  }
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;  
  }
}
/* }}} */

/* {{{ Y*/
t_bool Amd64OpNextY(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  if(AMD64_OP_TYPE(op)==amd64_optype_invalid){
    /* memory addressed by ES:DI */
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    AMD64_OP_BASE(op) = AMD64_REG_RDI;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;
    AMD64_OP_IMMEDIATE(op) = 0;
    if (bm == amd64_bm_b)
      AMD64_OP_MEMOPSIZE(op) = 1;
    else if (bm == amd64_bm_v && AMD64_OPSZPREF(ins))
      AMD64_OP_MEMOPSIZE(op) = 2;
    else if (bm == amd64_bm_v && !AMD64_OPSZPREF(ins))
      AMD64_OP_MEMOPSIZE(op) = 4;
    else 
      FATAL (("unknown bytemode"));
    return TRUE;
  }
  
  else{
    AMD64_OP_TYPE(op)=amd64_optype_invalid;
    return FALSE;  
  }
}
/* }}} */


t_bool Amd64OpNextV(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextRM8(op);
    case amd64_bm_w:
      return Amd64OpNextRM16(op);
    case amd64_bm_d:
      return Amd64OpNextRM32(op);
    case amd64_bm_v:
      if(AMD64_OPSZPREF(ins))
	return Amd64OpNextRM16(op);
      else return Amd64OpNextRM32(op);
    case amd64_bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}

t_bool Amd64OpNextW(t_amd64_ins * ins, t_amd64_operand * op, t_amd64_bytemode bm, t_amd64_immediates * imm){
  /*first pass*/
  switch(bm){
    case amd64_bm_b:
      return Amd64OpNextRM8(op);
    case amd64_bm_w:
      return Amd64OpNextRM16(op);
    case amd64_bm_d:
      return Amd64OpNextRM32(op);
    case amd64_bm_v:
      if(AMD64_OPSZPREF(ins))
	return Amd64OpNextRM16(op);
      else return Amd64OpNextRM32(op);
    case amd64_bm_p:
      FATAL(("currently not supported"));
      return FALSE;
    default:
      FATAL(("shouldnt get here"));
      return FALSE;
  }
}


/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker: */
