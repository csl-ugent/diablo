#include <diablosmc.h>
#include <diabloi386.h>

/* t_uint8 StateAddToIns(t_state * state, t_ins * ins) {{{*/
t_uint8 StateAddToIns(t_state * state, t_ins * ins)
{
  t_statelist * statelist = INS_STATELIST(ins);

  STATE_SET_PARENT_INS(state,ins);
  
  if(statelist == NULL){
    statelist = Calloc(1,sizeof(t_statelist));
    INS_SET_STATELIST(ins, statelist);
  }

  StateAddToStatelist(state,statelist);

  return STATELIST_COUNT(statelist)-1; 
}
/* }}} */

t_state * StateNewForCodebyte(t_codebyte * codebyte)/*{{{*/
{
  t_state * state = Calloc(1,sizeof(t_state));
  t_statelist * statelist = CODEBYTE_STATELIST(codebyte);

  STATE_SET_CODEBYTE(state,codebyte);

  if(statelist == NULL){
    statelist = Calloc(1,sizeof(t_statelist));
    CODEBYTE_SET_STATELIST(codebyte,statelist); 
  }

  StateAddToStatelist(state,statelist);

  return state;
}

void StateRemoveFromStatelist(t_state * state, t_statelist * statelist)
{
  t_state_ref * state_ref = STATELIST_FIRST(statelist);
  while(state_ref)
  {
    if(STATE_REF_STATE(state_ref)==state)
    {
      if(STATE_REF_PREV(state_ref))
        STATE_REF_SET_NEXT(STATE_REF_PREV(state_ref),STATE_REF_NEXT(state_ref));
      else STATELIST_FIRST(statelist) = STATE_REF_NEXT(state_ref);
      
      if(STATE_REF_NEXT(state_ref))
        STATE_REF_SET_PREV(STATE_REF_NEXT(state_ref),STATE_REF_PREV(state_ref));
      else STATELIST_LAST(statelist) = STATE_REF_PREV(state_ref);
      Free(state_ref);
      STATELIST_SET_COUNT(statelist,STATELIST_COUNT(statelist)-1);
      return;
    }
    state_ref = STATE_REF_NEXT(state_ref);
  }
  FATAL(("Could not find state"));
}

void StateAddToStatelist(t_state * state,t_statelist * statelist)
{
  t_state_ref * state_ref= Calloc(1,sizeof(t_state_ref));
  STATE_REF_SET_STATE(state_ref ,state);
  
  if(STATELIST_FIRST(statelist) == NULL)
  {
    STATELIST_SET_FIRST(statelist,state_ref);
    STATELIST_SET_LAST(	statelist, state_ref);
  }
  else{
   STATE_REF_SET_NEXT(STATELIST_LAST(statelist),state_ref);
   STATE_REF_SET_PREV(state_ref,STATELIST_LAST( statelist));
   STATELIST_SET_LAST(statelist, state_ref);
  }
  STATELIST_SET_COUNT(statelist,STATELIST_COUNT(statelist)+1);
}

/* }}} */
