/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosmc.h>
#include <diabloi386.h>

void SmcInstallStuff(t_cfg * cfg){
  InsStartStatelist(cfg);
  CfgStartCodebytelist(CFG_OBJECT(cfg));
}

/* void SmcAddCodebytes(unsigned char * codep, t_uint32 len, t_ins * ins, t_bool relocatable) {{{ */
void SmcAddCodebytes(unsigned char * codep, t_uint32 len, t_ins * ins, t_bool relocatable)
{
  while(len>0)
  {
    t_codebyte * codebyte = CodebyteNewForCfg(INS_CFG(ins));
    t_state * state = StateNewForCodebyte(codebyte);
    
    {
      int addend = 0;
      if(INS_STATELIST(ins))
	addend = STATELIST_COUNT(INS_STATELIST(ins));
      CODEBYTE_SET_OLD_ADDRESS(codebyte,INS_OLD_ADDRESS(ins)+addend);
      CODEBYTE_SET_CADDRESS(codebyte,INS_CADDRESS(ins)+addend);
    } 
    
    STATE_SET_KNOWN(state,!relocatable);
    STATE_SET_PARENT_INS(state,ins);
    STATE_SET_PARENT_OFFSET(state,StateAddToIns(state,ins));
    STATE_SET_VALUE(state,* codep);
    codep++;
    len--;
  }
}
/* }}} */

void SmcCreateDataIns(t_ins * ins)
{
  unsigned char tmp = I386_INS_DATA((t_i386_ins *) ins);
  SmcAddCodebytes(&tmp,1,ins,FALSE);
}

void SmcInitInstruction(t_ins * ins)
{
  t_uint8 buf[15];
  t_uint8 length;
  
  if(INS_STATELIST(ins))
  {
    t_statelist * states = INS_STATELIST(ins);
    SmcInstructionFini(ins, &states);
    INS_SET_STATELIST(ins,NULL);
  }
  
  /*set immediate size to 4 of control transfer instructions where possible {{{*/
  switch (I386_INS_OPCODE(T_I386_INS(ins)))
  {
    case I386_JMP:
    case I386_JMPF:
    case I386_Jcc:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
    case I386_CALL:
    case I386_CALLF:
      if(I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(ins)))==i386_optype_imm)
      {
	I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(ins))) = 4;
      }
    default:
      break;
  }
  /*}}}*/
  
  length = I386AssembleIns(T_I386_INS(ins),buf);
  SmcAddCodebytes(buf,length,ins,FALSE);

  /*set known flag of states{{{*/
  {
    t_reloc_ref * reloc_ref = INS_REFERS_TO(ins);
    while(reloc_ref)
    {
      int i = 0;
      t_state_ref * state_ref = INS_STATE_REF_FIRST(ins);
      for(i=0;i<RELOC_FROM_OFFSET(RELOC_REF_RELOC(reloc_ref));i++)
      {
	if(state_ref==NULL)
	  FATAL((""));
	state_ref = STATE_REF_NEXT(state_ref);
      }
      for(i=0;i<4;i++)
      {
	if(state_ref==NULL)
	  FATAL((""));
	STATE_SET_KNOWN(STATE_REF_STATE(state_ref),FALSE);
	state_ref = STATE_REF_NEXT(state_ref);
      }
      reloc_ref = RELOC_REF_NEXT(reloc_ref);
    }
  }

  if(InsIsControlTransferInstructionWithImmediateTarget(ins))
  {
    if(I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(ins))) == 1)
      STATE_SET_KNOWN(STATE_REF_STATE(STATELIST_LAST(INS_STATELIST(ins))),FALSE);
    else
    { 
      int i;
      t_state_ref * state_ref = STATELIST_LAST(INS_STATELIST(ins));
      for(i=0;i<4;i++)
      {
	STATE_SET_KNOWN(STATE_REF_STATE(state_ref),FALSE);
	state_ref = STATE_REF_PREV(state_ref);
      }
    }
  }
  /*}}}*/
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
