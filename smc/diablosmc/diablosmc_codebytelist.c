#include <diablosmc.h>

t_codebyte_ref * CodebyteListGetNthElement(t_codebytelist * list, t_uint32 n)
{
  int i;
  t_codebyte_ref * ret = list->first;
  for(i=0;i<n;i++)
    ret = ret->next;
  return ret;
}
 
void CodebyteListUnlink(t_codebytelist * list, t_codebyte_ref * item)
{
  if(item == list->first)
    list->first = item->next;
  if(item == list->last)
    list->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  list->count--;
}

void AddCodebyteToList(t_codebyte * codebyte,t_codebytelist * codebytelist)
{
  t_codebyte_ref * codebyte_ref= Calloc(1,sizeof(t_codebyte_ref));
  CODEBYTE_REF_SET_CODEBYTE(codebyte_ref, codebyte);
  
  if(CODEBYTELIST_FIRST(codebytelist) == NULL)
  {
    CODEBYTELIST_SET_FIRST(codebytelist,codebyte_ref);
    CODEBYTELIST_SET_LAST(codebytelist,codebyte_ref);
  }
  else{
   CODEBYTE_REF_SET_NEXT(CODEBYTELIST_LAST(codebytelist), codebyte_ref);
   CODEBYTE_REF_SET_PREV(codebyte_ref, CODEBYTELIST_LAST(codebytelist));
   CODEBYTELIST_SET_LAST(codebytelist, codebyte_ref);
  }

  codebytelist->count++;
}

void CodebytelistKill(t_codebytelist * bytes)/*{{{*/
{
  t_codebyte_ref * ref;
  if(!bytes)
  {
    WARNING(("killing empty codebyte list"));
    return;
  }
  
  ref = CODEBYTELIST_FIRST(bytes);
  while(ref)
  {
    t_codebyte_ref * next = CODEBYTE_REF_NEXT(ref);
    Free(ref);
    ref=next;      
  }
  Free(bytes);
}
/* }}} */

