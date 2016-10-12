#include <diablosmc.h>
#include <diabloi386.h>

t_dynamic_member_info codebyte_next_in_chain_array = null_info;
t_dynamic_member_info codebyte_prev_in_chain_array = null_info;
t_dynamic_member_info ins_statelist_array = null_info;
t_dynamic_member_info cfg_codebytelist_array = null_info;

void InsStartStatelist(t_cfg * cfg){InsInitStatelist(cfg);}
void InsStopStatelist(t_cfg * cfg){InsFiniStatelist(cfg);}
void CfgStartCodebytelist(t_object * obj){CfgInitCodebytelist(obj);}
void CfgStopCodebytelist(t_object * obj){CfgFiniCodebytelist(obj);}
void CodebyteStartPrevInChain(t_cfg * cfg){CodebyteInitPrevInChain(cfg);}
void CodebyteStopPrevInChain(t_cfg * cfg){CodebyteFiniPrevInChain(cfg);}
void CodebyteStartNextInChain(t_cfg * cfg){CodebyteInitNextInChain(cfg);}
void CodebyteStopNextInChain(t_cfg * cfg){CodebyteFiniNextInChain(cfg);}

void SmcInstructionInit(t_ins * ins, t_statelist ** states)
{
  *states = NULL;
}

void SmcInstructionFini(t_ins * ins, t_statelist ** states)
{
  t_state * state;
  t_state_ref * state_ref;
  INS_FOREACH_STATE(ins,state,state_ref)
  {
    t_codebyte * codebyte = STATE_CODEBYTE(state);
    /*unlink state from codebyte*/
    StateRemoveFromStatelist(state,CODEBYTE_STATELIST(codebyte));
    if(StatelistIsEmpty(CODEBYTE_STATELIST(codebyte)))
    {
      StatelistKill(CODEBYTE_STATELIST(codebyte));
      CodebyteUnlinkFromCfg(codebyte);
      CodebyteFree(codebyte);
    }
    Free(state);
  }
  StatelistKill(*states);
}

void SmcInstructionDup(t_ins * duplicated_ins, t_statelist ** pointer_to_original_state_list)
{
  t_ins * orig=global_hack_dup_orig;
  
  t_state_ref * state_ref;
  t_state * iter_state;
  t_uint32 i=0;

  INS_SET_STATELIST(duplicated_ins,NULL);
  
  STATELIST_FOREACH_STATE(INS_STATELIST(orig),iter_state,state_ref)
  {
    t_codebyte * new_codebyte = CodebyteNewForCfg(INS_CFG(duplicated_ins));
    t_state * new_state = StateNewForCodebyte(new_codebyte);
    
    CODEBYTE_SET_OLD_ADDRESS(new_codebyte,INS_OLD_ADDRESS(orig)+i);
    STATE_SET_KNOWN(new_state,STATE_KNOWN(iter_state));
    STATE_SET_PARENT_INS(new_state,duplicated_ins);
    STATE_SET_PARENT_OFFSET(new_state,StateAddToIns(new_state,duplicated_ins));
    STATE_SET_VALUE(new_state,STATE_VALUE(iter_state));
    i++;
  }
}

void SmcCfgInit(t_cfg * cfg, t_codebytelist ** codebytes)
{
  *codebytes = Calloc(1,sizeof(t_codebytelist));
  CFG_SET_CODEBYTELIST(cfg,*codebytes);
  CODEBYTELIST_SET_FIRST(*codebytes,NULL);
  CODEBYTELIST_SET_LAST(*codebytes,NULL);
  CODEBYTELIST_SET_COUNT(*codebytes,0);
}

void SmcCfgFini(t_cfg * cfg, t_codebytelist ** codebytes)
{
  t_codebyte * codebyte;
  t_codebyte_ref * codebyte_ref;
  CODEBYTELIST_FOREACH_CODEBYTE(*codebytes,codebyte,codebyte_ref)
  {
    CODEBYTE_SET_CODEBYTE_REF(codebyte,NULL);
  }
  CodebytelistKill(*codebytes);
}

void SmcCfgDup(t_cfg * duplicated_cfg, t_codebytelist ** pointer_to_original_codebyte_list)
{
  FATAL(("Write Duplicate for Smc Cfg"));
}
