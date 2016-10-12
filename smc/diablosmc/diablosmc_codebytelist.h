#include <diablosmc.h>

#ifndef SMC_CODEBYTELIST_TYPEDEFS
#define SMC_CODEBYTELIST_TYPEDEFS
typedef struct _t_codebyte_ref t_codebyte_ref;
typedef struct _t_codebytelist t_codebytelist;
#endif

#ifdef DIABLOSMC_TYPES
#ifndef DIABLOSMC_CODEBYTELIST_TYPES
#define DIABLOSMC_CODEBYTELIST_TYPES

struct _t_codebytelist
{
   struct _t_codebyte_ref * first;
   struct _t_codebyte_ref * last;
   t_uint32 count;
};

#define CODEBYTELIST_FIRST(codebytelist) ((codebytelist)->first)
#define CODEBYTELIST_SET_FIRST(codebytelist,codebyte_ref) ((codebytelist)->first=codebyte_ref)

#define CODEBYTELIST_LAST(codebytelist) ((codebytelist)->last)
#define CODEBYTELIST_SET_LAST(codebytelist,codebyte_ref) ((codebytelist)->last=codebyte_ref)

#define CODEBYTELIST_COUNT(codebytelist) ((codebytelist)->count)
#define CODEBYTELIST_SET_COUNT(codebytelist,_count) ((codebytelist)->count=_count)

struct _t_codebyte_ref
{
   struct _t_codebyte_ref * next;
   struct _t_codebyte_ref * prev;
   t_codebyte * codebyte;
};

#define CODEBYTE_REF_NEXT(codebyte_ref) ((codebyte_ref)->next)
#define CODEBYTE_REF_SET_NEXT(codebyte_ref,_next) ((codebyte_ref)->next=(_next))

#define CODEBYTE_REF_PREV(codebyte_ref) ((codebyte_ref)->prev)
#define CODEBYTE_REF_SET_PREV(codebyte_ref,_prev) ((codebyte_ref)->prev=(_prev))

#define CODEBYTE_REF_CODEBYTE(codebyte_ref) ((codebyte_ref)->codebyte)
#define CODEBYTE_REF_SET_CODEBYTE(codebyte_ref,_codebyte) ((codebyte_ref)->codebyte=(_codebyte))

#define CODEBYTELIST_FOREACH_CODEBYTE(codebytelist, codebyte, codebyte_ref) \
for(codebyte_ref = CODEBYTELIST_FIRST(codebytelist), codebyte = (codebyte_ref)?CODEBYTE_REF_CODEBYTE(codebyte_ref):NULL; \
    codebyte_ref!=NULL; \
    codebyte_ref = codebyte_ref->next, codebyte=(codebyte_ref)?CODEBYTE_REF_CODEBYTE(codebyte_ref):NULL)
									      
#endif
#endif

#ifdef DIABLOSMC_FUNCTIONS
#ifndef DIABLOSMC_CODEBYTELIST_FUNCTIONS
#define DIABLOSMC_CODEBYTELIST_FUNCTIONS
void AddCodebyteToList(t_codebyte * codebyte,t_codebytelist * codebytelist);
void CodebytelistKill(t_codebytelist * codebytes);
void CodebyteListUnlink(t_codebytelist * list, t_codebyte_ref * item);
t_codebyte_ref * CodebyteListGetNthElement(t_codebytelist * list, t_uint32 n);
#endif
#endif
