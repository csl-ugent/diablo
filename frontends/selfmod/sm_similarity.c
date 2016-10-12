/******************************************************************************
 * Copyright 2001,2002,2003: {{{
 *
 * Bertrand Anckaert, Bruno De Bus, Bjorn De Sutter, Dominique Chanet, 
 * Matias Madou and Ludo Van Put
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * Diablo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Diablo; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Written by Bertrand Anckaert  (Bertrand.Anckaert@UGent.be)
 *
 * Copyright (c) 2005 Bertrand Anckaert
 *
 * THIS FILE IS PART OF DIABLO (SM)
 *
 * MAINTAINER: Bertrand Anckaert (Bertrand.Anckaert@UGent.be)  }}}
 *****************************************************************************/
#include "sm_similarity.h"
#include "sm_cluster.h"
#include "../../arch/i386/i386_deflowgraph.h"

/*  void AddFullSimToList(t_similarity_list * sim,t_full_sim_list * list) {{{ */
void AddFullSimToList(t_similarity_list * sim,t_full_sim_list * list)
{
  t_full_sim_item * new_item;

  new_item=Malloc(sizeof(t_full_sim_item));
  new_item->similarity_list=sim;
  
  if(list->first==NULL)
  {
    list->first=new_item;
    list->last=new_item;
    new_item->next=NULL;
    new_item->prev=NULL;
  }
  else
  {
    new_item->next=NULL;
    new_item->prev=list->last;
    list->last->next=new_item;
    list->last=new_item;
  }
}
/* }}} */

/* void RemoveSimularityFromList(t_similarity * similarity,t_similarity_list * similarity_list) {{{ */
void RemoveSimularityFromList(t_similarity * similarity,t_similarity_list * similarity_list)
{
  t_similarity_item * similarity_item;
  
  for(similarity_item=similarity_list->first;similarity_item!=NULL;similarity_item=similarity_item->next)
  {
    if(similarity_item->similarity==similarity)
    {
      if(similarity_list->first==similarity_item && 
	  similarity_list->last==similarity_item)
      {
	similarity_list->first=NULL;
	similarity_list->last=NULL;
      }
      else if(similarity_list->first==similarity_item)
      {
	similarity_item->next->prev=NULL;
	similarity_list->first=similarity_item->next;
      }
      else if (similarity_list->last==similarity_item)
      {
	similarity_item->prev->next=NULL;
	similarity_list->last=similarity_item->prev;
      }
      else
      {
	similarity_item->prev->next=similarity_item->next;
	similarity_item->next->prev=similarity_item->prev;
      }
      return;
    }
  }
}
/* }}} */

/* void AddSimularityToList(t_similarity * similarity,t_similarity_list * similarity_list) {{{ */
void AddSimularityToList(t_similarity * similarity,t_similarity_list * similarity_list)
{
  t_similarity_item * similarity_item;
  similarity_item=Malloc(sizeof(t_similarity_item));
  similarity_item->similarity=similarity;
  
  if(similarity_list->first==NULL)
  {
    similarity_list->first=similarity_item;
    similarity_list->last=similarity_item;
    similarity_item->next=NULL;
    similarity_item->prev=NULL;
  }
  else
  {
    similarity_item->next=NULL;
    similarity_item->prev=similarity_list->last;
    similarity_list->last->next=similarity_item;
    similarity_list->last=similarity_item;
  }
  
}
/* }}} */

/* t_similarity_list * NrOfEdits(t_state_list * help_state_fun1,t_state_list * help_state_fun2) {{{ */
t_similarity_list * NrOfEdits(t_state_list * help_state_fun1,t_state_list * help_state_fun2)
{
  t_similarity_list * similarity_list=Malloc(sizeof(t_similarity_list));
  t_similarity_item * similarity_item_iterator1;
  t_uint32 i=0;
  t_int32 nr_of_bytes1=0,nr_of_bytes2=0;

  similarity_list->first=similarity_list->last=NULL;

  /*nr bytes in state_list*/
  nr_of_bytes1=GetNrOfBytesStateList(help_state_fun1);
  nr_of_bytes2=GetNrOfBytesStateList(help_state_fun2);
  for(i=0;i<(nr_of_bytes1+nr_of_bytes2-1);i++)
  {
    t_similarity * similarity=Malloc(sizeof(t_similarity));

    similarity->offset=i;
    similarity->bound=FALSE;
    similarity->nr_of_edits=0;
    similarity->nr_equal=0;
    similarity->f1_sl=help_state_fun1;
    similarity->f2_sl=help_state_fun2;
    AddSimularityToList(similarity,similarity_list);
  }

  similarity_item_iterator1=similarity_list->first;

  t_state_item * state_item1;
  t_state_item * state_item2;
  for(state_item1=help_state_fun1->first; state_item1!=NULL; state_item1=state_item1->next)
  {
    t_state * state1=state_item1->state;
    t_similarity_item * similarity_item_iterator=similarity_item_iterator1;

    for(state_item2=help_state_fun2->last; state_item2!=NULL; state_item2=state_item2->prev)
    {
      t_state * state2=state_item2->state;
      {
	if(state1->value_relocatable==TRUE ||
	    state2->value_relocatable==TRUE ||
	    state1->dontcare==TRUE ||
	    state2->dontcare==TRUE)
	  similarity_item_iterator->similarity->nr_of_edits++;
	else if(state2->value==state1->value)
	  similarity_item_iterator->similarity->nr_equal++;
	else
	{
//	  printf("offset:%d Check:%x %x\n",similarity_item_iterator->similarity->offset,state1->value,state2->value);
	  similarity_item_iterator->similarity->nr_of_edits++;
	}

      }
      similarity_item_iterator=similarity_item_iterator->next;

    }
    similarity_item_iterator1=similarity_item_iterator1->next;
  }

//  {
//    t_similarity_item * similarity_item_iterator=similarity_list->first;
//    for(similarity_item_iterator=similarity_list->first;similarity_item_iterator!=NULL;similarity_item_iterator=similarity_item_iterator->next)
//    {
//      printf("offset:%d nr_of_edits:%d nr_equal:%d\n",similarity_item_iterator->similarity->offset,similarity_item_iterator->similarity->nr_of_edits,similarity_item_iterator->similarity->nr_equal);
//    }
//  }

  return similarity_list;
}
/* }}} */

/* t_similarity_list * SortList(t_similarity_list * connected_list) {{{ */
t_similarity_list * SortList(t_similarity_list * connected_list)
{
  t_similarity_list * ret_list=Malloc(sizeof(t_similarity_list));
  t_similarity_item * similarity_itemX;
  t_similarity_item * similarity_itemY;

  ret_list->first=NULL;
  ret_list->last=NULL;

  if(connected_list->first==NULL)
    return connected_list;

//#define PRINT_BA
#ifdef PRINT_BA
  printf("BEFORE SortList\n");
  for(similarity_itemY=connected_list->first;similarity_itemY!=NULL;similarity_itemY=similarity_itemY->next)
    printf("0x%08x; size:%08d 0x%08x size %08d offset:%d\n",similarity_itemY->similarity->f1_sl,GetNrOfBytesStateList(similarity_itemY->similarity->f1_sl),similarity_itemY->similarity->f2_sl,GetNrOfBytesStateList(similarity_itemY->similarity->f2_sl),similarity_itemY->similarity->offset);
#endif

  /* first loop: as long as there are items that are not placed into the ret_list
   * second loop: search for the next similarity which can be placed in the ret_list */
  similarity_itemX=connected_list->first;
  
  for(;;)
  {
    t_similarity * simX=similarity_itemX->similarity;
    t_state_list * gr_f1_sl=simX->f1_sl;
    t_state_list * gr_f2_sl=simX->f2_sl;
    t_bool redo=FALSE;

    for(similarity_itemY=connected_list->first;similarity_itemY!=NULL;similarity_itemY=similarity_itemY->next)
    {
      t_similarity * simY=similarity_itemY->similarity;
      if(gr_f1_sl==simY->f1_sl)
      {
	if(simY->offset<simX->offset)
	{
#ifdef PRINT_BA
	  printf("Redo f1 %d %d simY:%x simX:%x\n",simY->offset,simX->offset,simY,simX);
#endif
	  /* Restart with similarity_itemY */
	  redo=TRUE;
	  similarity_itemX=similarity_itemY;
	  break;
	}
      }
      if(gr_f2_sl==simY->f2_sl)
      {
	if(simX->offset<simY->offset)
	{
#ifdef PRINT_BA
	  printf("Redo f2 %d>%d simY:%x simX:%x\n",simY->offset,simX->offset,simY,simX);
#endif
	  /* Restart with similarity_itemY */
	  redo=TRUE;
	  similarity_itemX=similarity_itemY;
	  break;
	}
      }
    }
    if(redo)
      continue;
    else
    {
      t_similarity * sim=similarity_itemX->similarity;

      AddSimularityToList(sim,ret_list);
      RemoveSimularityFromList(sim,connected_list);
      similarity_itemX=connected_list->first;

      for(similarity_itemY=connected_list->first;similarity_itemY!=NULL;similarity_itemY=similarity_itemY->next)
      {
	t_similarity * simY=similarity_itemY->similarity;
	if(gr_f1_sl==simY->f1_sl||gr_f2_sl==simY->f2_sl)
	{
	  similarity_itemX=similarity_itemY;
	  break;
	}
      }

      if(similarity_itemX==NULL)
	break;
    }

  }
  
#ifdef PRINT_BA
  printf("AFTER SortList\n");
  for(similarity_itemY=ret_list->first;similarity_itemY!=NULL;similarity_itemY=similarity_itemY->next)
    printf("0x%08x; size:%08d 0x%08x size %08d offset:%d\n",similarity_itemY->similarity->f1_sl,GetNrOfBytesStateList(similarity_itemY->similarity->f1_sl),similarity_itemY->similarity->f2_sl,GetNrOfBytesStateList(similarity_itemY->similarity->f2_sl),similarity_itemY->similarity->offset);
#endif
	
  return ret_list;
}
/*}}}*/

/* void Impossible(t_similarity_list * similarity_list_hulp) {{{ */
void Impossible(t_similarity_list * similarity_list_hulp)
{
  t_similarity_item * similarity_item;

  for(similarity_item=similarity_list_hulp->first;similarity_item!=NULL;similarity_item=similarity_item->next)
  {
//    printf("ok");
      similarity_item->similarity->nr_equal=0;
  }
//    printf("\n");
}
/* }}} */

void SetTheImpossibilityLists(t_state_list_list * f1_list,t_state_list_list * f2_list, t_full_sim_list * sim_list)
{
    t_state_list_item * sli1, * sli2;

    for(sli1=f1_list->first;sli1!=NULL;sli1=sli1->next)
    {
      for(sli2=f2_list->first;sli2!=NULL;sli2=sli2->next)
      {
	SetTheImpossibility(sli1->state_list, sli2->state_list, sim_list);
      }
    }

}
		
/* void SetTheImpossibility(t_state_list * f1_list, t_state_list * f2_list, t_full_sim_list * sim_list) {{{ */
void SetTheImpossibility(t_state_list * f1_list, t_state_list * f2_list, t_full_sim_list * sim_list)
{
  t_full_sim_item * full_sim_item;
//  printf("Impossible call %x %x\n",f1_list,f2_list);

  for(full_sim_item=sim_list->first;full_sim_item!=NULL;full_sim_item=full_sim_item->next)
  {
    t_bool first=FALSE;
    t_bool sec=FALSE;

    if(full_sim_item->similarity_list->first->similarity->f1_sl==f1_list)
      first=TRUE;
    if(full_sim_item->similarity_list->first->similarity->f2_sl==f2_list)
      sec=TRUE;
    if(first && sec)
    {
//      printf("Impossible:%x %x\n",full_sim_item->similarity_list->first->similarity->f1_sl,full_sim_item->similarity_list->first->similarity->f2_sl);
      Impossible(full_sim_item->similarity_list);
    }
  }
}
/* }}} */

t_similarity_list * NewSimilarityList()
{
  t_similarity_list * similarity_list=Malloc(sizeof(t_similarity_list));
  similarity_list->first=NULL;
  similarity_list->last=NULL;
  return similarity_list;
}

t_full_sim_list * NewFullSimList()
{
  t_full_sim_list * sim_list=Malloc(sizeof(t_full_sim_list));
  sim_list->first=NULL;
  sim_list->last=NULL;
  return sim_list;
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
