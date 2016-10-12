#include <diablosmc.h>
#include <diabloi386.h>

t_bool StatelistIsEmpty(t_statelist * statelist)
{
  return STATELIST_COUNT(statelist)==0;
}

void StatelistKill(t_statelist * states)
{
  t_state_ref * ref;
  if(!states)
  {
    WARNING(("killing empty state list"));
    return;
  }
  
  ref = STATELIST_FIRST(states);
  while(ref)
  {
    t_state_ref * next = STATE_REF_NEXT(ref);
    Free(ref);
    ref=next;      
  }
  Free(states);
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
