#include <diablosmc.h>

#ifndef SMC_STATELIST_TYPEDEFS
#define SMC_STATELIST_TYPEDEFS
typedef struct _t_state_ref t_state_ref;
typedef struct _t_statelist t_statelist;
#endif

#ifdef DIABLOSMC_TYPES
#ifndef DIABLOSMC_STATELIST_TYPES
#define DIABLOSMC_STATELIST_TYPES

struct _t_statelist
{
   struct _t_state_ref * first;
   struct _t_state_ref * last;
   t_uint32 count;
};

#define STATELIST_FIRST(statelist) ((statelist)->first)
#define STATELIST_SET_FIRST(statelist,state_ref) ((statelist)->first=state_ref)

#define STATELIST_LAST(statelist) ((statelist)->last)
#define STATELIST_SET_LAST(statelist,state_ref) ((statelist)->last=state_ref)

#define STATELIST_COUNT(statelist) ((statelist)->count)
#define STATELIST_SET_COUNT(statelist,_count) ((statelist)->count=_count)

struct _t_state_ref
{
   struct _t_state_ref * next;
   struct _t_state_ref * prev;
   t_state * state;
};

#define STATE_REF_NEXT(state_ref) ((state_ref)->next)
#define STATE_REF_SET_NEXT(state_ref,_next) ((state_ref)->next=(_next))

#define STATE_REF_PREV(state_ref) ((state_ref)->prev)
#define STATE_REF_SET_PREV(state_ref,_prev) ((state_ref)->prev=(_prev))

#define STATE_REF_STATE(state_ref) ((state_ref)->state)
#define STATE_REF_SET_STATE(state_ref,_state) ((state_ref)->state=(_state))

#define STATELIST_FOREACH_STATE(statelist, state, state_ref) \
for(state_ref = STATELIST_FIRST(statelist), state = (state_ref)?STATE_REF_STATE(state_ref):NULL; \
    state_ref!=NULL; \
    state_ref = state_ref->next, state=(state_ref)?STATE_REF_STATE(state_ref):NULL)
									      
#endif
#endif

#ifdef DIABLOSMC_FUNCTIONS
#ifndef DIABLOSMC_STATELIST_FUNCTIONS
#define DIABLOSMC_STATELIST_FUNCTIONS
t_bool StatelistIsEmpty(t_statelist * statelist);
void StatelistKill(t_statelist * states);
#endif
#endif
