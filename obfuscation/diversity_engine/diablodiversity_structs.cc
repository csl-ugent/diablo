/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

#ifdef __cplusplus
extern "C" {
#endif
#include <diablodiversity.h>
#ifdef __cplusplus
}
#endif

/*
void BoolListAdd(t_bool isTrue, t_bool_list * list)
{
  t_bool_item * item= Calloc(1,sizeof(t_bool_item));
  item->isTrue = isTrue;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;
}
*/
t_bool_list * BoolListNew()
{
  t_bool_list * ret = (t_bool_list *) Calloc(1,sizeof(t_bool_list));
/*  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;*/
  return ret;
}

void BoolListFree(t_bool_list * list)
{
  if(list == NULL)
    return;
  Free(list->flags);

  Free(list);
/*  while (list->last != NULL)
  {
    t_bool_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);*/
}

t_bool_list * BoolListNewAll(t_uint64 count, t_bool isTrue)
{
  t_bool_list * ret = BoolListNew();
  for(t_uint64 i = 0; i < count; i++)
    BoolListAdd(isTrue, ret);
  return ret;    
}

t_bool_list * BoolListCopy(t_bool_list * list)
{
  t_bool_list * ret = BoolListNew();
  for(t_uint64 i = 0; i < list->count; i++)
    BoolListAdd(list->flags[i],ret);
  return ret;
}

t_bool_list * BoolListNewAllFalse(t_uint64 count)
{
  return BoolListNewAll(count,FALSE);
}

t_bool_list * BoolListNewAllTrue(t_uint64 count)
{
  return BoolListNewAll(count,TRUE);
}

void BblListAdd(t_bblList * list,t_bbl * bbl)/* {{{ */
{
  t_bbl_item * item= (t_bbl_item *) Calloc(1,sizeof(t_bbl_item));
  item->bbl = bbl;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;
}

t_bblList * BblListNew()
{
  t_bblList * ret = (t_bblList *) Calloc(1,sizeof(t_bblList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void BblListFree(t_bblList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_bbl_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}

void BblListUnlink(t_bbl_item  * item, t_bblList * bblList)
{
  if(item == bblList->first)
    bblList->first = item->next;
  if(item == bblList->last)
    bblList->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  bblList->count--;
}

t_bbl_item * BblListGetNthElement(t_bblList * list, t_uint32 n)
{
  t_bbl_item * item = list->first;
  for(t_uint32 i = 0; i < n; i++)
    item = item->next;
  return item;
}/* }}} */

void InsListAdd(t_insList * list,t_ins * ins)/* {{{ */
{
  t_ins_item * item= (t_ins_item *) Calloc(1,sizeof(t_ins_item));
  item->ins = ins;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;
}

t_insList * InsListNew()
{
  t_insList * ret = (t_insList *) Calloc(1,sizeof(t_insList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void InsListFree(t_insList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_ins_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}

void InsListUnlink(t_ins_item  * item, t_insList * insList)
{
  if(item == insList->first)
    insList->first = item->next;
  if(item == insList->last)
    insList->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  insList->count--;
}

t_ins_item * InsListGetNthElement(t_insList * list, t_uint32 n)
{
  t_ins_item * item = list->first;
  for(t_uint32 i = 0; i < n; i++)
    item = item->next;
  return item;
}/* }}} */

t_bblint_item * BblIntListAdd(t_bblintList * list,t_bbl * bbl)/* {{{ */
{
  int i;
  t_bblint_item * item= (t_bblint_item *) Calloc(1,sizeof(t_bblint_item));
  item->bbl = bbl;
  for(i=0;i<6;i++)
    item->op[i]=0;
  item->check=FALSE;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;

  return item;
}

t_bblintList * BblIntListNew()
{
  t_bblintList * ret = (t_bblintList *) Calloc(1,sizeof(t_bblintList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void BblIntListFree(t_bblintList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_bblint_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}

void BblIntListUnlink(t_bblint_item  * item, t_bblintList * bblintList)
{
  if(item == bblintList->first)
    bblintList->first = item->next;
  if(item == bblintList->last)
    bblintList->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  bblintList->count--;
}

t_bblint_item * BblIntListGetNthElement(t_bblintList * list, t_uint32 n)
{
  t_bblint_item * item = list->first;
  for(t_uint32 i = 0; i < n; i++)
    item = item->next;
  return item;
}

/* }}} */

t_bool AddressListContains(t_addressList * list, t_address address)
{
  t_address_item * item = list->first;
  while(item)
  {
    if(item->address == address)
      return TRUE;
    item = item->next;
  }
  return FALSE;
}

void AddressListAdd(t_addressList * list,t_address address)
{
  t_address_item* item;

  if(AddressListContains(list,address))
      return;
      
  item = (t_address_item*) Calloc(1,sizeof(t_address_item));
  item->address = address;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;
}

t_addressList * AddressListNew()
{
  t_addressList * ret = (t_addressList *) Calloc(1,sizeof(t_addressList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

t_addressList * AddressListDup(t_addressList * orig)
{
  t_addressList  *  ret = AddressListNew();
  t_address_item * item = orig->first;
  while(item)
  {
    AddressListAdd(ret,item->address);
    item = item->next;
  }

  return ret;
}

void AddressListAddList(t_addressList * add_to, t_addressList * to_be_added)
{
  t_address_item * item = to_be_added->first;
  while(item)
  {
    AddressListAdd(add_to,item->address);
    item = item->next;
  }
}

void AddressListFree(t_addressList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_address_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}

void AddressListUnlink(t_address_item  * item, t_addressList * addressList)
{
  if(item == addressList->first)
    addressList->first = item->next;
  if(item == addressList->last)
    addressList->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  addressList->count--;
}
void 
InsOldAddressesInit(t_ins * ins, t_addressList ** list)
{
    *list = AddressListNew();
    AddressListAdd(*list, INS_OLD_ADDRESS(ins));
}

void 
InsOldAddressesFini(t_ins * ins, t_addressList ** list)
{
  AddressListFree(*list);
}

void 
InsOldAddressesDup(t_ins * ins, t_addressList ** list)
{
  t_ins * orig_ins = (t_ins *) global_hack_dup_orig;
  *list = AddressListDup(INS_ADDRESSLIST(orig_ins));
  return;
}

t_dynamic_member_info ins_old_addresses_array = null_info;

void 
InsScoreInit(t_ins * ins, float * score)
{
  *score = 0;
}

void 
InsScoreFini(t_ins * ins, float * score)
{
}

void 
InsScoreDup(t_ins * ins, float * score)
{
  t_ins * orig_ins = (t_ins *) global_hack_dup_orig;
  *score = INS_SCORE(orig_ins);
  return;
}

t_dynamic_member_info ins_score_array = null_info;

void 
InsCostInit(t_ins * ins, t_int32 * cost)
{
  *cost = -1;
}

void 
InsCostFini(t_ins * ins, t_int32 * cost)
{
}

void 
InsCostDup(t_ins * ins, t_int32 * cost)
{
  t_ins * orig_ins = (t_ins *) global_hack_dup_orig;
  *cost = INS_COST(orig_ins);
  return;
}

t_dynamic_member_info ins_cost_array = null_info;

void 
InsGainInit(t_ins * ins, t_int32 * gain)
{
  *gain = 0;
}

void 
InsGainFini(t_ins * ins, t_int32 * gain)
{
}

void 
InsGainDup(t_ins * ins, t_int32 * gain)
{
  t_ins * orig_ins = (t_ins *) global_hack_dup_orig;
  *gain = INS_GAIN(orig_ins);
  return;
}

t_dynamic_member_info ins_gain_array = null_info;
void 
BblFactoredInit(t_bbl * bbl, t_bool * factored)
{
    *factored = FALSE;
}

void 
BblFactoredFini(t_bbl * bbl, t_bool * factored)
{
}

void 
BblFactoredDup(t_bbl * bbl, t_bool * factored)
{
  t_bbl * orig_bbl = (t_bbl *) global_hack_dup_orig;
  *factored = BBL_FACTORED(orig_bbl);
  return;
}

t_dynamic_member_info bbl_factored_array = null_info;

void 
FunctionFactoredInit(t_function * function, t_bool * factored)
{
    *factored = FALSE;
}

void 
FunctionFactoredFini(t_function * function, t_bool * factored)
{
}

void 
FunctionFactoredDup(t_function * function, t_bool * factored)
{
  t_function * orig_function = (t_function*) global_hack_dup_orig;
  *factored = FUNCTION_FACTORED(orig_function);
  return;
}

t_dynamic_member_info function_factored_array = null_info;


/* Bbl Transformations are all defined in diablodiversity_engine_iterative.c */


