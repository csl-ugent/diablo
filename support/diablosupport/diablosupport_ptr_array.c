/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diablosupport_ptr_array.h"


void
PtrArrayInit(t_ptr_array *ea, t_bool trypreventdup)
{
  memset(ea,0,sizeof(*ea));
  ea->trypreventdup=trypreventdup;
}


int
PtrArrayCount(const t_ptr_array *ea)
{
  return ea->count;
}


t_bool
PtrArrayIsEmpty(const t_ptr_array *ea)
{
  return ((ea->count) == 0);
}


int
PtrArrayLength(const t_ptr_array *ea)
{
  return ea->length;
}


void
PtrArrayAdd(t_ptr_array *ea, void* edge)
{
  if (ea->length==0)
  {
    ea->length=64;
    ea->arr = Calloc(ea->length,sizeof(edge));
  }
  else if (ea->length == ea->count)
  {
    ea->length*=2;
    ea->arr = Realloc(ea->arr,ea->length*sizeof(edge));
  }
  if ((ea->count==0) ||
      !ea->trypreventdup ||
      (ea->arr[ea->count-1]!=edge))
  { 
    ea->arr[ea->count]=edge;
    ea->count++;
  }
}


void
PtrArrayInsert(t_ptr_array *ea, void* edge, int pos)
{
  if (pos==ea->count)
  {
    PtrArrayAdd(ea,edge);
    return;
  }
 if ((pos>ea->count) ||
     (pos<0))
   FATAL(("Inserting ptrarray element at invalid index"));

 if (ea->length == ea->count)
  {
    ea->length*=2;
    ea->arr = Realloc(ea->arr,ea->length*sizeof(edge));
  }

  memmove(&ea->arr[pos+1],&ea->arr[pos],(ea->count-pos)*sizeof(edge));
  ea->arr[pos]=edge;
  ea->count++;
}


void
PtrArrayRemove(t_ptr_array *ea, int index, t_bool freeit)
{
  if (index>=ea->count || index < 0)
  {
    FATAL(("Removing illegal index from ptr array: %d >= %d",index,ea->count));
  }
  if (freeit)
    Free(ea->arr[index]);
  memmove(&(ea->arr[index]),&(ea->arr[index+1]),(ea->count-index-1)*sizeof(ea->arr[1]));
  ea->count--;
}

void
PtrArrayRemoveLast(t_ptr_array *ea, t_bool freeit)
{
    PtrArrayRemove(ea, ea->count-1, freeit);
}

int
PtrArrayFind(const t_ptr_array *ea, const void *item)
{
  int index;

  for (index = 0; index < ea->count; index++)
  {
    if (ea->arr[index] == item)
      break;
  }
  if (index == ea->count)
    return -1;
  return index;
}


void
PtrArrayRemoveContent(t_ptr_array *ea, void *item, t_bool freeit)
{
  int index;

  index = PtrArrayFind(ea,item);
  ASSERT(index != -1,("Content not found in PtrArrayRemoveContent"));
  PtrArrayRemove(ea,index,freeit);
}


void*
PtrArrayGet(const t_ptr_array *ea, int index)
{
  return ea->arr[index];
}


void*
PtrArrayGetFirst(const t_ptr_array *ea)
{
  int counter;
  void *first;
  
  for(counter = 0; counter < ea->length; counter++)
  {
    first = PtrArrayGet(ea,counter);
	if(first)
	  return first;
  }
  return ((void*) NULL);
}


void*
PtrArrayGetLast(const t_ptr_array *ea)
{
  /*int counter;*/
  /*void *last, *last_candidate;*/
  
  /*last = (void*) NULL;*/
  /*for(counter = 0; counter < ea->length; counter++)*/
  /*{*/
    /*last_candidate = PtrArrayGet(ea,counter);*/
	/*if(last_candidate)*/
	  /*last = last_candidate;*/
  /*}*/
  /*return last;*/
  if (ea->count == 0)
  {
    return ((void*)NULL);
  } else {
    return PtrArrayGet(ea, ea->count-1);
  }
}


void
PtrArrayFini(const t_ptr_array *ea, t_bool free_elements)
{
  int i;
  if (free_elements)
  {
    for (i = 0; i < ea->count; i++)
      if (ea->arr[i])
        Free(ea->arr[i]);
  }
  /* can be NULL in case the array was empty */
  if (ea->arr)
    Free(ea->arr);
}
