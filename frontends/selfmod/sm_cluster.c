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
#include "sm_cluster.h"
#include "sm_similarity.h"
#include "../../arch/i386/i386_deflowgraph.h"
#include "../../arch/i386/i386_instructions.h"
#include "../diablo_options.h"
 #include <fcntl.h>

void MapFunctionsToTemplate(t_state_list * state_template, t_function_list * fun_list, t_state_list_list * state_lists, t_bbl * editor);

/* t_bool AllBblsInOneFunction(t_function * fun) {{{ */
t_bool AllBblsInOneFunction(t_function * fun)
{
  t_bbl * bbl;

  FUN_FOREACH_BBL(fun,bbl)
  {
    t_bbl * prev_bbl=BBL_PREV_IN_CHAIN(bbl);
    t_bbl * next_bbl=BBL_NEXT_IN_CHAIN(bbl);

    while(prev_bbl!=NULL && prev_bbl!=BBL_FIRST_IN_CHAIN(bbl))
    {
      if(BBL_FUNCTION(prev_bbl)!=BBL_FUNCTION(bbl))
	return FALSE;
      prev_bbl=BBL_PREV_IN_CHAIN(prev_bbl);
    }

    while(next_bbl!=NULL && next_bbl!=BBL_LAST_IN_CHAIN(bbl))
    {
      if(BBL_FUNCTION(next_bbl)!=BBL_FUNCTION(bbl))
	return FALSE;
      next_bbl=BBL_NEXT_IN_CHAIN(next_bbl);
    }
  }

  return TRUE;
}
/* }}} */

/* t_int32 GetNrOfBytes(t_bbl_chain_item * bbl_chain_item) {{{ */
t_int32 GetNrOfBytes(t_bbl_chain_item * bbl_chain_item)
{
  t_bbl * bbl_in_chain;
  t_int32 nr_of_bytes=0;
  t_ins * ins;

  for(bbl_in_chain=BBL_FIRST_IN_CHAIN(bbl_chain_item->bbl);bbl_in_chain!=NULL;bbl_in_chain=BBL_NEXT_IN_CHAIN(bbl_in_chain))
  {
    BBL_FOREACH_INS(bbl_in_chain,ins)
    {
      t_state_list * ins_state_list=InsStateList(ins,JudyMapSM);
      t_state_item * state_item;
      for(state_item=ins_state_list->first;state_item!=NULL;state_item=state_item->next)
      {
	nr_of_bytes++;
      }
    }
  }
  return nr_of_bytes;
}
/* }}} */

/* void AddBblChainToList(t_bbl * bbl,t_bbl_chain_list * ch_fun1) {{{ */
void AddBblChainToList(t_bbl * bbl,t_bbl_chain_list * ch_fun1)
{
  t_bbl_chain_item * bbl_chain_item;

  /* Special bbls like function entry and function exit */
  if(BBL_FIRST_IN_CHAIN(bbl)==NULL)
    return;

  bbl_chain_item=Malloc(sizeof(t_bbl_chain_item));
  bbl_chain_item->bbl=BBL_FIRST_IN_CHAIN(bbl);
  
  if(ch_fun1->first==NULL)
  {
    ch_fun1->first=bbl_chain_item;
    ch_fun1->last=bbl_chain_item;
    bbl_chain_item->next=NULL;
    bbl_chain_item->prev=NULL;
  }
  else
  {
    bbl_chain_item->next=NULL;
    bbl_chain_item->prev=ch_fun1->last;
    ch_fun1->last->next=bbl_chain_item;
    ch_fun1->last=bbl_chain_item;
  }

  bbl_chain_item->nr_of_bytes=GetNrOfBytes(bbl_chain_item);
}
/* }}} */

 /* t_int32 GetNrOfBytesStateList (t_state_list * state_list) {{{ */
t_int32 GetNrOfBytesStateList (t_state_list * state_list)
{
  t_int32 nr_of_bytes=0;
  t_state_item * state_item;

  for(state_item=state_list->first;state_item!=NULL;state_item=state_item->next)
  {
    nr_of_bytes++;
  }

  return nr_of_bytes;
}
/* }}} */
      
/* t_bool TestInChainList(t_bbl * bbl,t_bbl_chain_list *ch_fun1) {{{ */
t_bool TestInChainList(t_bbl * bbl,t_bbl_chain_list *ch_fun1)
{
  t_bbl_chain_item * bbl_chain_item;
  
  if(BBL_FIRST_IN_CHAIN(bbl)==NULL)
    return FALSE;
  
  /* foreach t_bbl_chain_item in the t_bbl_chain_list */
  for(bbl_chain_item=ch_fun1->first;bbl_chain_item!=NULL;bbl_chain_item=bbl_chain_item->next)
    /* look at all bbls in the chain for an equal one */
      if(bbl_chain_item->bbl==BBL_FIRST_IN_CHAIN(bbl))
	return TRUE;

  return FALSE;
}
/* }}} */

/* t_bool NotASingleInstructionInChain(t_bbl * bbl) {{{ */
t_bool NotASingleInstructionInChain(t_bbl * bbl)
{
  t_bbl * bbl_in_chain;
  t_ins * ins;

  for(bbl_in_chain=BBL_FIRST_IN_CHAIN(bbl);bbl_in_chain!=NULL;bbl_in_chain=BBL_NEXT_IN_CHAIN(bbl_in_chain))
  {
    BBL_FOREACH_INS(bbl_in_chain,ins)
    {
//      char x[100];
//      I386InstructionPrint(ins,x);
//      printf("t_ins * ins:%s\n",x);
      return FALSE;
    }
  }

  return TRUE;
}
/* }}} */

/* void AddStateListToStateListList(t_state_list * add,t_state_list_list * list) {{{ */
void AddStateListToStateListList(t_state_list * add,t_state_list_list * list)
{
  t_state_list_item * sli;
  sli=Malloc(sizeof(t_state_list_item));
  sli->state_list=add;
  
  if(list->first==NULL)
  {
    list->first=sli;
    list->last=sli;
    sli->next=NULL;
    sli->prev=NULL;
  }
  else
  {
    sli->next=NULL;
    sli->prev=list->last;
    list->last->next=sli;
    list->last=sli;
  }
  
}
/* }}} */

/* void PrintStateList(t_state_list *state_list) {{{ */
void PrintStateList(t_state_list *state_list)
{
  t_state_item * state_item;
  for(state_item=state_list->first;state_item!=NULL;state_item=state_item->next)
  {
    printf("%d 0x%02x\n",state_item->state->value_relocatable,state_item->state->value);
  }
  printf("\n");
}
/* }}} */
  
/* void AddListItemToListLists(t_state_list * sli1, t_state_list_list * state_fun1, t_state_list_list * state_template, t_state_list_list * state_other) {{{ */
void AddListItemToListLists(t_state_list * sli1, t_state_list_list * state_fun1_list, t_state_list_list * state_template_list, t_state_list_list * state_other_list)
{
  t_state_item * state_item1;
  t_state_list * state_fun1=NewStateList();
  t_state_list * state_template=NewStateList();
  t_state_list * state_other=NewStateList();

//  printf("0x%08x; size:%08d\n",sli1, GetNrOfBytesStateList (sli1));

  for(state_item1=sli1->first;state_item1!=NULL;state_item1=state_item1->next)
  {
    t_state * state=NewStateEmpty();
    state->dontcare=TRUE;

    AddStateToList(state_item1->state,state_fun1);
    AddStateToList(state,state_other);
    AddStateToList(state_item1->state,state_template);
  }

  AddStateListToStateListList(state_template,state_template_list);
  AddStateListToStateListList(state_fun1,state_fun1_list);
  AddStateListToStateListList(state_other,state_other_list);
}
 /* }}} */

/* t_bbl_chain_list *  PutBblChainInList(t_function * fun1) {{{ */
t_bbl_chain_list *  PutBblChainInList(t_function * fun1)
{
  t_bbl_chain_list * ch_fun1=Malloc(sizeof(t_bbl_chain_list));
  t_bbl * bbl;

  ch_fun1->first=ch_fun1->last=NULL;
  t_int32 aantal=0;

  FUN_FOREACH_BBL(fun1,bbl)
    if(!TestInChainList(bbl,ch_fun1))
      if(!NotASingleInstructionInChain(bbl))
      {
	AddBblChainToList(bbl,ch_fun1);
	aantal++;
      }
//  printf("Aantal bblchains in de functie:%d\n",aantal);

  if(aantal==0)
    return NULL;

  return ch_fun1;
}
/* }}} */
  
/* t_state_list * TransformBblChainToStateList(t_bbl * bbl) {{{ */
t_state_list * TransformBblChainToStateList(t_bbl * bbl)
{
  t_bbl * bbl_in_chain1;
  t_ins * ins1;
  t_state_list * help_state_fun1=NewStateList();

  for(bbl_in_chain1=BBL_FIRST_IN_CHAIN(bbl);bbl_in_chain1!=NULL;bbl_in_chain1=BBL_NEXT_IN_CHAIN(bbl_in_chain1))
  {
    BBL_FOREACH_INS(bbl_in_chain1,ins1)
    {
      {
	t_state_list * ins_state_list1=InsStateList(ins1,JudyMapSM);
	t_state_item * state_item1;
	for(state_item1=ins_state_list1->first;state_item1!=NULL;state_item1=state_item1->next)
	{
	  t_state * state1=state_item1->state;
	  AddStateToList(state1,help_state_fun1);
	}
      }
    }
  }
  return help_state_fun1;
}
/* }}} */

/* void StateListAddBuildOutOf(t_state_list * h_state_fun1,t_int32 template_nr,t_state_list * help_state_fun1) {{{ */
void StateListAddBuildOutOf(t_state_list * h_state_fun1,t_int32 template_nr,t_state_list * help_state_fun1)
{
  t_build_out_of * prev=Malloc(sizeof(t_build_out_of));
  t_build_out_of * tmp;
  prev->offset=template_nr;
  prev->list=help_state_fun1;
  prev->next=NULL;
  if(h_state_fun1->build==NULL)
    h_state_fun1->build=prev;
  else
  {
    for(tmp=h_state_fun1->build;tmp->next!=NULL;tmp=tmp->next);
    tmp->next=prev;
  }
}
/* }}} */

/* void AddStandAloneBlockListsToList(t_state_list_list * sll1,t_state_list_list * sll2,t_full_sim_list * sim_list,t_state_list_list *state_template_list) {{{ */
void AddStandAloneBlockListsToList(t_state_list_list * sll1,t_state_list_list * sll2,t_full_sim_list * sim_list,t_state_list_list *state_template_list)
{

  t_state_list_item * sliX;

  for(sliX=sll1->first;sliX!=NULL;sliX=sliX->next)
  {
    /* for-loop {{{ */
    t_bool reeds_aanwezig=FALSE;
    t_state_list * to_check=sliX->state_list;
    t_full_sim_item * full_sim_item;

    for(full_sim_item=sim_list->first;full_sim_item!=NULL;full_sim_item=full_sim_item->next)
    {
      t_similarity_item * similarity_item;

      for(similarity_item=full_sim_item->similarity_list->first;similarity_item!=NULL;similarity_item=similarity_item->next)
      {
	t_similarity * sim=similarity_item->similarity;
	if(sim->f1_sl==to_check || sim->f2_sl==to_check)
	{
	  if(sim->bound)
	  {
	    reeds_aanwezig=TRUE;
	  }
	}
      }
    }
    /* }}} */

    /* toevoegen {{{ */
    if(!reeds_aanwezig)
    {
      t_state_item * state_item1;
      t_state_list * state_fun1=NewStateList();
      t_state_list * state_template=NewStateList();
      t_state_list * state_fun2=NewStateList();

      for(state_item1=to_check->first;state_item1!=NULL;state_item1=state_item1->next)
      {
	t_state * state=NewStateEmpty();
	state->dontcare=TRUE;

	AddStateToList(state_item1->state,state_fun1);
	AddStateToList(state,state_fun2);
	AddStateToList(state_item1->state,state_template);
      }

      state_template->f1_sl=state_fun1;
      state_template->f2_sl=state_fun2;

      StateListAddBuildOutOf(state_fun1,0,to_check);

      AddStateListToStateListList(state_template,state_template_list);
    }
    /* }}} */
  }

  for(sliX=sll2->first;sliX!=NULL;sliX=sliX->next)
  {
    /* for-loop {{{ */
    t_bool reeds_aanwezig=FALSE;
    t_state_list * to_check=sliX->state_list;
    t_full_sim_item * full_sim_item;

    for(full_sim_item=sim_list->first;full_sim_item!=NULL;full_sim_item=full_sim_item->next)
    {
      t_similarity_item * similarity_item;

      for(similarity_item=full_sim_item->similarity_list->first;similarity_item!=NULL;similarity_item=similarity_item->next)
      {
	t_similarity * sim=similarity_item->similarity;
	if(sim->f1_sl==to_check || sim->f2_sl==to_check)
	{
	  if(sim->bound)
	  {
	    reeds_aanwezig=TRUE;
	  }
	}
      }
    }
    /* }}} */

    /* toevoegen {{{ */
    if(!reeds_aanwezig)
    {
      {
	t_state_item * state_item2;
	t_state_list * state_fun1=NewStateList();
	t_state_list * state_template=NewStateList();
	t_state_list * state_fun2=NewStateList();

	for(state_item2=to_check->first;state_item2!=NULL;state_item2=state_item2->next)
	{
	  t_state * state=NewStateEmpty();
	  state->dontcare=TRUE;

	  AddStateToList(state_item2->state,state_fun2);
	  AddStateToList(state,state_fun1);
	  AddStateToList(state_item2->state,state_template);
	}

	state_template->f1_sl=state_fun1;
	state_template->f2_sl=state_fun2;

	StateListAddBuildOutOf(state_fun2,0,to_check);

	AddStateListToStateListList(state_template,state_template_list);
      }
    }
    /* }}} */
  }
}
/* }}} */

/* t_state_list_list * NewStateListList() {{{ */
t_state_list_list * NewStateListList()
{
  t_state_list_list * sim_list=Malloc(sizeof(t_state_list_list));
  sim_list->first=NULL;
  sim_list->last=NULL;
  return sim_list;
}
/* }}} */

/* t_state_list_list * FromFunToStateList(t_function * fun) {{{ */
t_state_list_list * FromFunToStateList(t_function * fun)
{
  t_bbl_chain_list * ch_fun;
  t_state_list_list * sll=NewStateListList();
  t_bbl_chain_item * chlist;
  t_state_list * help_state_fun;
  
  /* All Basic Blocks in the chain are of one function */
  if(!AllBblsInOneFunction(fun))
    return NULL;
//    FATAL(("Different functions in one chain"));
  
  /* Put all basic block chains of one function in a list (t_bbl_chain_list) */
  ch_fun=PutBblChainInList(fun);
  if(ch_fun==NULL)
    return NULL;
//    FATAL(("ERR"));

  
  /* Transform all bbl_chains to t_state_list */
  for(chlist=ch_fun->first;chlist!=NULL;chlist=chlist->next)
  {
    help_state_fun=TransformBblChainToStateList(chlist->bbl);
    AddStateListToStateListList(help_state_fun,sll);
  }

  return sll;
}
/* }}} */

/* void TransformFunctionsToClusteredLists(t_state_list_list * sll1,t_state_list_list * sll2,t_state_list_list * state_template_list) {{{ */
void TransformFunctionsToClusteredLists(t_state_list_list * sll1,t_state_list_list * sll2,t_state_list_list * state_template_list)
{
  t_similarity_list * connected_list=NewSimilarityList();
  t_similarity_list * h_connected_list;
  t_full_sim_list * sim_list=NewFullSimList();

  /* Production of t_full_sim_list *sim_list {{{ */ 
  {
    t_state_list_item * sli1, * sli2;

    for(sli1=sll1->first;sli1!=NULL;sli1=sli1->next)
    {
      t_state_list * help_state_fun1=sli1->state_list;

      for(sli2=sll2->first;sli2!=NULL;sli2=sli2->next)
      {
	t_state_list * help_state_fun2=sli2->state_list;

	t_similarity_list * similarity_list;
	similarity_list=NrOfEdits(help_state_fun1,help_state_fun2);

	AddFullSimToList(similarity_list,sim_list);
      }
    }
  }
  /* }}} */

  /* Zoek de grootste gelijkenis */
  /* Algoritme die ervoor zorgt dat een bbl_chain uit fun1 gekoppeld wordt aan een bbl_chain uit fun2 EN er ook voor zorgt dat het niet meer mogelijk is 
   * dat een bbl_chain overlapt met een bbl_chain uit dezelfde functie */
  /* search for pieces of code with a lot of similarity. Lay them on top of each other {{{ */

  for(;;)
  {
    t_full_sim_item * full_sim_item;
    t_similarity * sim_grootste;
    t_similarity_list * similarity_list_grootste;
    t_int32 grootste=0;
    t_int32 offset=-1;
    t_state_list * gr_f1_sl;
    t_state_list * gr_f2_sl;

    for(full_sim_item=sim_list->first;full_sim_item!=NULL;full_sim_item=full_sim_item->next)
    {
      t_similarity_list * similarity_list=full_sim_item->similarity_list;
      t_similarity_item * similarity_item;

      for(similarity_item=similarity_list->first;similarity_item!=NULL;similarity_item=similarity_item->next)
      {
	t_similarity * sim=similarity_item->similarity;
	  
	if(sim->nr_equal>grootste)
	{
	  grootste=sim->nr_equal;
	  offset=sim->offset;
	  sim_grootste=sim;
	  similarity_list_grootste=similarity_list;
	  gr_f1_sl=sim->f1_sl;
	  gr_f2_sl=sim->f2_sl;

	}
      }
    }

    if(offset>-1)
    {
      sim_grootste->bound=TRUE;
//      printf("ADD:%x %x %d\n",sim_grootste->f1_sl,sim_grootste->f2_sl,sim_grootste->offset);
      AddSimularityToList(sim_grootste,connected_list);
      /* Make sure that there is no overlap */
      {
	t_full_sim_item * full_sim_item;

	for(full_sim_item=sim_list->first;full_sim_item!=NULL;full_sim_item=full_sim_item->next)
	{
	  t_similarity_list * similarity_list_hulp=full_sim_item->similarity_list;
	  t_similarity_item * similarity_item;
	  
	  {
	    /* original diff */
	    t_int32 size_f1=GetNrOfBytesStateList(gr_f1_sl);
	    t_int32 size_f2=GetNrOfBytesStateList(gr_f2_sl);

	    /* bepaling start f1: max(0,[(offset+1)-size_f2]) */
	    t_int32 start_f1_offset= (0 > (offset+1-size_f2) ? 0 :offset+1-size_f2);
	    /* bepaling last f1: min(offset, size_f1) */
	    t_int32 last_f1_offset= (offset < size_f1 ? offset : size_f1);
	    /* bepaling start f2: max(0,size_f2-offset-1) */
	    t_int32 start_f2_offset= (0 > size_f2-offset-1 ? 0 : size_f2-offset-1);
	    /* bepaling last f2: min(size_f2,size_f1+size_f2-offset-2) */
	    t_int32 last_f2_offset= (size_f2 < size_f1+size_f2-offset-2 ?size_f2 : size_f1+size_f2-offset-2);

	    for(similarity_item=similarity_list_hulp->first;similarity_item!=NULL;similarity_item=similarity_item->next)
	    {
	      t_similarity * h_sim=similarity_item->similarity;

	      if(h_sim->f1_sl==gr_f1_sl)
	      {
		t_state_list * h_f2_sl=similarity_item->similarity->f2_sl;

		t_int32 h_size_f2=GetNrOfBytesStateList(h_f2_sl);

		t_int32 h_start_offset= (0 > (h_sim->offset+1-h_size_f2) ? 0 :h_sim->offset+1-h_size_f2);
		t_int32 h_last_offset= (h_sim->offset < size_f1 ? h_sim->offset :size_f1);

		if(!(h_start_offset<start_f1_offset && h_last_offset<start_f1_offset))
		  if(!(h_start_offset>last_f1_offset && h_last_offset>last_f1_offset))
		    similarity_item->similarity->nr_equal=0;
	      }
	    }


	    for(similarity_item=similarity_list_hulp->first;similarity_item!=NULL;similarity_item=similarity_item->next)
	    {
	      t_similarity * h_sim=similarity_item->similarity;

	      if(h_sim->f2_sl==gr_f2_sl)
	      {
		t_state_list * h_f1_sl=similarity_item->similarity->f1_sl;

		t_int32 h_size_f1=GetNrOfBytesStateList(h_f1_sl);

		t_int32 h_start_offset= (0 > size_f2-h_sim->offset-1 ? 0 : size_f2-h_sim->offset-1);
		t_int32 h_last_offset= (size_f2 < h_size_f1+size_f2-h_sim->offset-2 ?size_f2 : h_size_f1+size_f2-h_sim->offset-2);

		if(!(h_start_offset<start_f2_offset && h_last_offset<start_f2_offset))
		  if(!(h_start_offset>last_f2_offset && h_last_offset>last_f2_offset))
		    similarity_item->similarity->nr_equal=0;
	      }
	    }
	  }
	}
	{
	  {


	    /* prevent siutation as {{{:
	     * new overlap: y-p
	     * x
	     * x o
	     * x o
	     * x o
	     *   o
	     *   o
	     *   o
	     * y o
	     * y
	     * y p
	     * y p
	     * y p
	     *   p
	     *   p
	     *
	     * Possible next overlap x-p => loop of bbl chains.... 
	     * }}}*/

	    h_connected_list=SortList(connected_list);
	    connected_list=h_connected_list;

	    {
	      t_similarity_item * similarity_itemX=connected_list->first;

	      for(;;)
	      {
//		printf("Outer for-loop\n");
		t_state_list_list * f1_list=NewStateListList();
		t_state_list_list * f2_list=NewStateListList();

		if(similarity_itemX==NULL)
		  break;
		t_similarity * simX=similarity_itemX->similarity;
		if(simX==NULL)
		  FATAL(("???"));

		AddStateListToStateListList(simX->f1_sl,f1_list);
		AddStateListToStateListList(simX->f2_sl,f2_list);

		for(;;)
		{
//		  printf("Inner for-loop\n");

		  similarity_itemX=similarity_itemX->next;
		  if(similarity_itemX==NULL)
		    break;
		  simX=similarity_itemX->similarity;
		  if(simX==NULL)
		    FATAL(("???"));

		  t_state_list_item * lst1=f1_list->last;
		  t_state_list_item * lst2=f2_list->last;

		  if(lst1->state_list==simX->f1_sl)
		    AddStateListToStateListList(simX->f2_sl,f2_list);
		  else if(lst2->state_list==simX->f2_sl)
		    AddStateListToStateListList(simX->f1_sl,f1_list);
		  else
		    break;
		}
		SetTheImpossibilityLists(f1_list,f2_list,sim_list);
	      }
	    }
#if 0
	    {
	      t_similarity_item * similarity_itemX=connected_list->first;

	      for(;;)
	      {
		printf("Outer for-loop\n");
		t_state_list_list * f1_list=NewStateListList();
		t_state_list_list * f2_list=NewStateListList();

		if(similarity_itemX==NULL)
		  break;
		t_similarity * simX=similarity_itemX->similarity;
		if(simX==NULL)
		  FATAL(("???"));
//		  break;

		AddStateListToStateListList(simX->f1_sl,f1_list);
		AddStateListToStateListList(simX->f2_sl,f2_list);

		similarity_itemX=similarity_itemX->next;
		if(similarity_itemX==NULL)
		  break;
		simX=similarity_itemX->similarity;
		if(simX==NULL)
		  FATAL(("???"));
		
		for(;;)
		{
		  printf("Inner for-loop\n");
		  t_state_list_item * lst1=f1_list->last;
		  t_state_list_item * lst2=f2_list->last;

		  if(lst1->state_list==simX->f1_sl)
		    AddStateListToStateListList(simX->f2_sl,f2_list);
		  else if(lst2->state_list==simX->f2_sl)
		    AddStateListToStateListList(simX->f1_sl,f1_list);
		  else
		  {
//		    similarity_itemX=similarity_itemX->next;
		    break;
		  }
		  
		  similarity_itemX=similarity_itemX->next;
		  if(similarity_itemX==NULL)
		    break;
		  simX=similarity_itemX->similarity;
		}
		SetTheImpossibilityLists(f1_list,f2_list,sim_list);
		//SetTheImpossibility(simX->f1_sl,simX->f2_sl,sim_list);
	      }
	    }
#endif
#if 0
	    {
	      t_similarity_item * similarity_itemX=connected_list->first;

	      for(;;)
	      {
//		  printf("Outer for-loop\n");
		t_state_list_list * f1_list=NewStateListList();
		t_state_list_list * f2_list=NewStateListList();

		if(similarity_itemX==NULL)
		  break;
		t_similarity * simX=similarity_itemX->similarity;
		if(simX==NULL)
		  break;

		AddStateListToStateListList(simX->f1_sl,f1_list);
		AddStateListToStateListList(simX->f2_sl,f2_list);

		for(;;)
		{
//		  printf("Inner for-loop\n");
		  t_state_list_item * lst1=f1_list->last;
		  t_state_list_item * lst2=f2_list->last;

		  if(lst1->state_list==simX->f1_sl)
		    AddStateListToStateListList(simX->f2_sl,f2_list);
		  else if(lst2->state_list==simX->f2_sl)
		    AddStateListToStateListList(simX->f1_sl,f1_list);
		  else
		  {
		    similarity_itemX=similarity_itemX->next;
		    break;
		  }
		  SetTheImpossibility(simX->f1_sl,simX->f2_sl,sim_list);
#if 0
		  else
		  {
		    SetTheImpossibility(f1_list,f2_list,sim_list);
		    similarity_itemX=similarity_itemX->next;
		    break;
		  }
#endif
		  similarity_itemX=similarity_itemX->next;
		  if(similarity_itemX==NULL)
		    break;
		  simX=similarity_itemX->similarity;
		}
		SetTheImpossibility(simX->f1_sl,simX->f2_sl,sim_list);
	      }
	    }
#endif
	  }
	}
      }
    }
    else
      break;
  }
  /* }}} */

  h_connected_list=SortList(connected_list);
  connected_list=h_connected_list;

  /*make een state_list voor fun1, fun2 and template {{{ */
  {
    t_similarity_item * similarity_item;

    for(similarity_item=connected_list->first;similarity_item!=NULL;similarity_item=similarity_item->next)
    {
      t_state_list * h_state_template=NewStateList();
      t_state_list * h_state_fun1=NewStateList();
      t_state_list * h_state_fun2=NewStateList();

      t_similarity * sim=similarity_item->similarity;
      t_state_list * help_state_fun1=sim->f1_sl;
      t_state_list * help_state_fun2=sim->f2_sl;

      t_state_item * state_item_iter_fun1=help_state_fun1->first;
      t_state_item * state_item_iter_fun2=help_state_fun2->first;

      t_int32 offset=sim->offset;

      t_int32 size_f1=GetNrOfBytesStateList(help_state_fun1);
      t_int32 size_f2=GetNrOfBytesStateList(help_state_fun2);

      t_int32 start_f1=size_f2-1;
      t_int32 start_f2=offset;
      t_int32 nr1=0;
      t_int32 nr2=0;
      t_int32 template_nr=0;

      //	printf("0x%08x; size:%08d 0x%08x size %08d offset:%d\n",help_state_fun1,size_f1,help_state_fun2,size_f2,offset);
      /* Aanmaken van cluster template {{{ */
      for(;nr1!=size_f1||nr2!=size_f2;)
      {
	/* if(nr1==size_f1) {{{ */
	if(nr1==size_f1)
	{
	  //	    printf("if(nr1==size_f1)\n");
	  t_state * state=NewStateEmpty();
	  state->dontcare=TRUE;
	  AddStateToList(state,h_state_fun1);
	  AddStateToList(state_item_iter_fun2->state,h_state_fun2);
	  AddStateToList(state_item_iter_fun2->state,h_state_template);
	  start_f2++;
	  if(nr2==0)
	    StateListAddBuildOutOf(h_state_fun2,template_nr,help_state_fun2);
	  nr2++;
	  state_item_iter_fun2=state_item_iter_fun2->next;
	}
	/* }}} */
	/* else if(nr2==size_f2) {{{ */
	else if(nr2==size_f2)
	{
	  //	    printf("if(nr2==size_f2)\n");
	  t_state * state=NewStateEmpty();
	  state->dontcare=TRUE;
	  AddStateToList(state,h_state_fun2);
	  AddStateToList(state_item_iter_fun1->state,h_state_fun1);
	  AddStateToList(state_item_iter_fun1->state,h_state_template);
	  start_f1++;
	  if(nr1==0)
	    StateListAddBuildOutOf(h_state_fun1,template_nr,help_state_fun1);
	  nr1++;
	  state_item_iter_fun1=state_item_iter_fun1->next;
	}
	/* }}} */
	/* else if(start_f1>start_f2) {{{ */
	else if(start_f1>start_f2)
	{
	  //	    printf("if(start_f1>start_f2)\n");
	  t_state * state=NewStateEmpty();
	  state->dontcare=TRUE;
	  AddStateToList(state,h_state_fun1);
	  AddStateToList(state_item_iter_fun2->state,h_state_fun2);
	  AddStateToList(state_item_iter_fun2->state,h_state_template);
	  start_f2++;
	  if(nr2==0)
	    StateListAddBuildOutOf(h_state_fun2,template_nr,help_state_fun2);
	  nr2++;
	  state_item_iter_fun2=state_item_iter_fun2->next;
	}
	/* }}} */
	/* else if(start_f1<start_f2) {{{ */
	else if(start_f1<start_f2)
	{
	  //	    printf("if(start_f1<start_f2)\n");
	  t_state * state=NewStateEmpty();
	  state->dontcare=TRUE;
	  AddStateToList(state,h_state_fun2);
	  AddStateToList(state_item_iter_fun1->state,h_state_fun1);
	  AddStateToList(state_item_iter_fun1->state,h_state_template);
	  start_f1++;
	  if(nr1==0)
	    StateListAddBuildOutOf(h_state_fun1,template_nr,help_state_fun1);
	  nr1++;
	  state_item_iter_fun1=state_item_iter_fun1->next;
	}
	/* }}} */
	/* else {{{ */
	else
	{
	  t_state * state;
	  //printf("if(start_f1==start_f2)\n");
	  if(state_item_iter_fun1->state->value_relocatable==TRUE ||
	      state_item_iter_fun2->state->value_relocatable==TRUE ||
	      state_item_iter_fun1->state->dontcare==TRUE ||
	      state_item_iter_fun2->state->dontcare==TRUE)
	  {
	    state=NewStateEmpty();
	    state->dontcare=TRUE;
	    state->value_relocatable=TRUE;
	  }
	  else if(state_item_iter_fun2->state->value==state_item_iter_fun1->state->value)
	    state=state_item_iter_fun1->state;
	  else
	  {
	    state=NewStateEmpty();
	    state->dontcare=TRUE;
	    state->value_relocatable=TRUE;
	  }

	  AddStateToList(state_item_iter_fun1->state,h_state_fun1);
	  AddStateToList(state_item_iter_fun2->state,h_state_fun2);
	  AddStateToList(state,h_state_template);
	  start_f1++;
	  start_f2++;
	  if(nr1==0)
	    StateListAddBuildOutOf(h_state_fun1,template_nr,help_state_fun1);
	  if(nr2==0)
	    StateListAddBuildOutOf(h_state_fun2,template_nr,help_state_fun2);
	  nr1++;
	  nr2++;
	  state_item_iter_fun1=state_item_iter_fun1->next;
	  state_item_iter_fun2=state_item_iter_fun2->next;
	}
	/* }}} */
	{
	  if(nr1==size_f1 || nr2==size_f2)
	  {
	    if(similarity_item->next && similarity_item->next->similarity->f1_sl==help_state_fun1)
	    {
	      similarity_item=similarity_item->next;
	      sim=similarity_item->similarity;
	      help_state_fun2=sim->f2_sl;

	      state_item_iter_fun2=help_state_fun2->first;
	      offset=sim->offset;
	      size_f2=GetNrOfBytesStateList(help_state_fun2);

	      start_f1=size_f2-1+nr1;
	      start_f2=offset;
	      nr2=0;
	    }
	    else if(similarity_item->next && similarity_item->next->similarity->f2_sl==help_state_fun2)
	    {
	      similarity_item=similarity_item->next;
	      sim=similarity_item->similarity;
	      help_state_fun1=sim->f1_sl;

	      state_item_iter_fun1=help_state_fun1->first;
	      offset=sim->offset;
	      size_f1=GetNrOfBytesStateList(help_state_fun1);

	      start_f1=size_f2-1;
	      start_f2=offset+nr2;
	      nr1=0;
	    }

	  }
	}
	template_nr++;
      }
      /* }}} */

      h_state_template->f1_sl=h_state_fun1;
      h_state_template->f2_sl=h_state_fun2;

      AddStateListToStateListList(h_state_template,state_template_list);
    }
  }
  /* }}} */

  /* TEMP functie; om te testen voegen we de rest gewoon toe.... */
  AddStandAloneBlockListsToList(sll1,sll2,sim_list,state_template_list);
}
/* }}} */
  
/* t_int32 CopyStateListToStateList(t_state_list * from,t_state_list *to) {{{*/
t_int32 CopyStateListToStateList(t_state_list * from,t_state_list *to)
{
  t_state_item * help_state_item;
  
  t_int32 nr_of_bytes=GetNrOfBytesStateList(from);
  
  for(help_state_item=from->first;help_state_item!=NULL;help_state_item=help_state_item->next)
    AddStateToList(help_state_item->state,to);

  return nr_of_bytes;
}
/* }}} */
      
/* void CopyDontCaresToStateList(t_state_list * state_list,t_int32 nr_of_dontcares) {{{ */
void CopyDontCaresToStateList(t_state_list * state_list,t_int32 nr_of_dontcares)
{
  t_int32 i;
  for(i=0;i<nr_of_dontcares;i++)
  {
    t_state * state=NewStateEmpty();
    state->dontcare=TRUE;
    AddStateToList(state,state_list);
  }
}
/* }}} */

/* void RecursiveDontCares(t_made * top,t_int32 nr_of_dontcares) {{{ */
void RecursiveDontCares(t_made * top,t_int32 nr_of_dontcares)
{
  CopyDontCaresToStateList(top->f1->to,nr_of_dontcares);
  CopyDontCaresToStateList(top->f2->to,nr_of_dontcares);
  if(top->f1->inter)
    RecursiveDontCares(top->f1,nr_of_dontcares);
  if(top->f2->inter)
    RecursiveDontCares(top->f2,nr_of_dontcares);
}
/* }}} */

/* t_made * Made(t_state_list_list * from,t_state_list * to,t_bool inter,t_made * f1,t_made * f2) {{{ */
t_made * Made(t_state_list_list * from,t_state_list * to,t_bool inter,t_made * f1,t_made * f2)
{
  t_made * top=Malloc(sizeof(t_made));
  top->from=from;
  top->to=to;
  top->inter=inter;
  top->f1=f1;
  top->f2=f2;
  return top;
}
/* }}} */

/* void BuildPiece(t_made * top, t_build_out_of * build,t_int32 total) {{{ */
void BuildPiece(t_made * top, t_build_out_of * build,t_int32 total)
{
  t_build_out_of * build_hulp;
  t_uint32 n=0;
  t_int32 nr_of_copies=0;
  t_int32 nr_of_dontcares=0;
	
  for(;build!=NULL;build=build->next)
  {
    nr_of_dontcares=build->offset-n;
    if(nr_of_dontcares!=0)
    {
      RecursiveDontCares(top,nr_of_dontcares);
    }
    n+=nr_of_dontcares;
    nr_of_copies=CopyStateListToStateList(build->list->f1_sl,top->f1->to);
      
    if(top->f1->inter)
      {
	build_hulp=build->list->f1_sl->build; // dat is den build van temp
	BuildPiece(top->f1,build_hulp,nr_of_copies);
      }
      
    /* CHECK_INTEGRITY {{{ */
#ifdef CHECK_INTEGRITY
    if(!build)
      FATAL((""));
    if(!build->list)
      FATAL((""));
    if(!build->list->f2_sl)
      FATAL((""));
    if(!top->f2)
      FATAL((""));
    if(!top->f2->to)
      FATAL((""));
#endif
    /* }}} */
    
    nr_of_copies=CopyStateListToStateList(build->list->f2_sl,top->f2->to);
    
    if(top->f2->inter)
      {
	build_hulp=build->list->f2_sl->build; // dat is den build van temp
	BuildPiece(top->f2,build_hulp,nr_of_copies);
      }
    
    n+=nr_of_copies;
  }
  nr_of_dontcares=total-n;
  if(nr_of_dontcares!=0)
  {
      RecursiveDontCares(top,nr_of_dontcares);
  }
}
/* }}} */

/* void AddFunToFunList(t_function * fun,t_function_list * fun_list) {{{ */
void AddFunToFunList(t_function * fun,t_function_list * fun_list)
{
  t_function_item * fun_item= Calloc(1,sizeof(t_function_item));
  fun_item->fun = fun;
  
  if(fun_list->first == NULL)
  {
    fun_list->first = fun_list->last = fun_item;
  }
  else{
   fun_list->last->next = fun_item;
   fun_item->prev = fun_list->last;
   fun_list->last = fun_item;
  }
}
/* }}} */

/* t_state_list * ClusterFunctionsToStateList(t_function_list * fun_list, t_state_list_list * fun_state_list) {{{ */
t_state_list * ClusterFunctionsToStateList(t_function_list * fun_list, t_state_list_list * fun_state_list)
{
  t_state_list_list * state_template_list=NewStateListList();
  
  t_function_item * h_fun=fun_list->first;
//  printf("XXX:%s\n",h_fun->fun->name);
  t_state_list_list * sll1=FromFunToStateList(h_fun->fun);
  h_fun=h_fun->next;
//  printf("XXX:%s\n",h_fun->fun->name);
  t_state_list_list * sll2=FromFunToStateList(h_fun->fun);
  TransformFunctionsToClusteredLists(sll1,		  sll2, state_template_list);
  t_state_list_item * h_state=fun_state_list->first;
  t_made * node1=Made(sll1,h_state->state_list,FALSE,NULL,NULL);
  h_state=h_state->next;
  t_made * node2=Made(sll2,h_state->state_list,FALSE,NULL,NULL);
  t_state_list * state_template2=NewStateList();
  t_made * top=Made(state_template_list,state_template2,TRUE,node1,node2);
  
  h_fun=h_fun->next;
  while(h_fun!=NULL)
  {
//    printf("in while-loop\n");
//  printf("XXX:%s\n",h_fun->fun->name);
    t_state_list_list * sll3=FromFunToStateList(h_fun->fun);
    t_state_list_list * state_template_list_final2=NewStateListList();
    TransformFunctionsToClusteredLists(state_template_list, sll3,	state_template_list_final2);
    h_state=h_state->next;
    t_made * node3=Made(sll3,h_state->state_list,FALSE,NULL,NULL);
    state_template2=NewStateList();
    top=Made(state_template_list_final2,state_template2,TRUE,top,node3);
    h_fun=h_fun->next;
    state_template_list=state_template_list_final2;
  };
  
  {
    t_state_list_item * sli;
    t_build_out_of * build; // dat is den build van temp
    t_int32 total=0;

    /* state_template_list->state_template {{{ */
    for(sli=top->from->first;sli!=NULL;sli=sli->next)
    {

      total=CopyStateListToStateList(sli->state_list,top->to);
      CopyStateListToStateList(sli->state_list->f1_sl,top->f1->to);
      CopyStateListToStateList(sli->state_list->f2_sl,top->f2->to);

      if(top->f1->inter)
      {
	build=sli->state_list->f1_sl->build; // dat is den build van temp
	BuildPiece(top->f1,build,total);
      }
      if(top->f2->inter)
      {
	build=sli->state_list->f2_sl->build; // dat is den build van temp
	BuildPiece(top->f2,build,total);
      }
    }
    /* }}} */

  }

  return state_template2;
}
/* }}} */

/* void PrintCluster(t_state_list_list * fun_state_list,t_state_list * state_template) {{{ */
void PrintCluster(t_state_list_list * fun_state_list,t_state_list * state_template)
{
  t_uint32 i=0;
  t_int32 total=0;
  t_state_item ** pointers;
  t_state_list_item * h_state_list=fun_state_list->first;
  t_int32 teller=0;

  while(h_state_list!=NULL)
  {
    total++;
    h_state_list=h_state_list->next;
  };

  pointers=Malloc(total*sizeof(t_state_item*));
  printf("Total:%d\n",total);

  for(i=0,h_state_list=fun_state_list->first;i!=total;i++,h_state_list=h_state_list->next)
  {
    pointers[i]=h_state_list->state_list->first;
  };

  t_state_item * state_temp=state_template->first;
  printf("Fun1\t\tFun2\t\tFun3\t\tFun4\t\t...\t\tTemplate\n");
  for(;state_temp!=NULL;)
  {
    printf("%04d ",teller);

    for(i=0;i!=total;i++)
    {
      printf("%d %d 0x%02x\t",pointers[i]->state->dontcare,pointers[i]->state->value_relocatable,pointers[i]->state->value);
      pointers[i]=pointers[i]->next;
    }
    printf("%d %d 0x%02x\n",state_temp->state->dontcare,state_temp->state->value_relocatable,state_temp->state->value);
    state_temp=state_temp->next;
    teller++;
  }
  printf("total nr of bytes in cluster template:%d\n",teller);
  /* CHECK_INTEGRITY {{{ */
#define CHECK_INTEGRITY
#ifdef CHECK_INTEGRITY
    if(state_temp!=NULL)
      FATAL(("pointers state_temp moet leeg zijn!\n"));
  for(i=0;i!=total;i++)
    if(pointers[i]!=NULL)
      FATAL(("pointers %d moet leeg zijn!\n",i));
#if 0 
  //DEBUG-code
  {
    printf("pointers %d moet leeg zijn!\n",i);
    for(;pointers[i];)
    {
      printf("%04d ",teller);
      printf("%d %d 0x%02x\t\n",pointers[i]->state->dontcare,pointers[i]->state->value_relocatable,pointers[i]->state->value);
      pointers[i]=pointers[i]->next;
      teller++;
    }
  }
#endif
#endif
  /* }}} */

}
/* }}} */

typedef struct _t_function_node t_function_node;

struct _t_function_node
{
  t_node node;
  t_function * fun;
  t_uint64 called;
  t_bool escaping;
  t_bool destructive;
  t_bool hulp;
};

typedef struct _t_function_edge t_function_edge;

//#define INFINITY 0xffffffff;

struct _t_function_edge
{
  t_edge edge;
  t_int32 last;
  t_uint64 switched;
  t_bool path_exists;
  t_tristate cl_edge;
};

#define T_FUN_NODE(x)     		((t_function_node *)(x))

typedef struct _table_entry
{
  int value;
  t_function_node * fun_node;
  struct _table_entry * next;
} table_entry;

typedef struct _hashtable
{
  table_entry ** buckets;
  unsigned int table_size;
} hashtable;


hashtable * init_table(unsigned int size)
{
  hashtable * table = (hashtable *) malloc(sizeof(hashtable));
  table->table_size = size;
  table->buckets = (table_entry**) calloc(size,sizeof(table_entry));
  return table;
}

table_entry *  find(hashtable * table, int value)
{
  int hash = (value % table->table_size);
  table_entry * entry = table->buckets[hash];

  if (entry)
  {
    table_entry * prev = NULL;

    while (entry)
    {
      if (entry->value==value)
      {
	return entry;
      }
      entry=entry->next;
    }
  }

  return NULL;
}

table_entry * put(hashtable * table, int value, t_function_node * fun_node)
{
  int hash = (value % table->table_size);
  table_entry * entry = table->buckets[hash];

  if (entry)
  {
    table_entry * prev = NULL;

    while (entry)
    {
      if (entry->value==value)
      {
	printf("Warning Bucket exists...\n");
	return entry;
      }
      entry=entry->next;
    }
  }

  table_entry * new_entry = (table_entry*) Malloc(sizeof(table_entry));
  new_entry->value=value;
  new_entry->fun_node=fun_node;
  new_entry->next=table->buckets[hash];
  table->buckets[hash]=new_entry;
  return new_entry;
}

t_function_edge * SearchForEdgeAndMakeTrue(t_function_node * node1,t_function_node * node2)
{
//  printf("MAKE TRUE;\n");
  t_function_edge *c_edge; 

  NODE_FOREACH_PRED_EDGE(node1,c_edge)
  {
    t_function_node * target=(t_function_node*)EDGE_HEAD(c_edge);
    if(target==node2)
    {
      if(c_edge->cl_edge==FALSE)
	return NULL;
      else
      {
	c_edge->cl_edge=TRUE;
	return c_edge;
      }
    }
  }
  NODE_FOREACH_SUCC_EDGE(node1,c_edge)
  {
    t_function_node * target=(t_function_node*)EDGE_TAIL(c_edge);
    if(target==node2)
    {
      if(c_edge->cl_edge==FALSE)
	return NULL;
      else
      {
	c_edge->cl_edge=TRUE;
	return c_edge;
      }
    }
  }
}

void AddNodeToCluster(t_function_edge * edge)
{
//  printf("ADDNODETOVLUSTER\n");
  t_function_node * member1;
      
  t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
  t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
  
  t_edge *c_edge,* d_edge,*x_edge; 
  t_int32 cost=0;
  
  if(node1->hulp==FALSE || node2->hulp==FALSE)
    return -1;
  
  member1=node1;

  member1->hulp=FALSE;
  /* node2 {{{ */
  x_edge=SearchForEdgeAndMakeTrue(member1,node2);
  /* }}} */
  /* SUCC {{{ */
  NODE_FOREACH_SUCC_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */
  /* PRED {{{ */
  NODE_FOREACH_PRED_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */

  NODE_FOREACH_PRED_EDGE(node1,c_edge)
  {
    if(((t_function_edge *)c_edge)->cl_edge==TRUE)
    {
      member1=(t_function_node*)EDGE_HEAD(c_edge);
      member1->hulp=FALSE;
      {
  /* node2 {{{ */
  x_edge=SearchForEdgeAndMakeTrue(member1,node2);
  /* }}} */
  /* SUCC {{{ */
  NODE_FOREACH_SUCC_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */
  /* PRED {{{ */
  NODE_FOREACH_PRED_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */
      }
    }
  }

  NODE_FOREACH_SUCC_EDGE(node1,c_edge)
  {
    if(((t_function_edge *)c_edge)->cl_edge==TRUE)
    {
      member1=(t_function_node*)EDGE_TAIL(c_edge);
      member1->hulp=FALSE;
      {
  /* node2 {{{ */
  x_edge=SearchForEdgeAndMakeTrue(member1,node2);
  /* }}} */
  /* SUCC {{{ */
  NODE_FOREACH_SUCC_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */
  /* PRED {{{ */
  NODE_FOREACH_PRED_EDGE(node2,d_edge)
  {
    if(((t_function_edge *)d_edge)->cl_edge==TRUE)
    {
      t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
      x_edge=SearchForEdgeAndMakeTrue(member1,member2);
      member2->hulp=FALSE;
    }
  }
  /* }}} */
      }
    }
  }
}


t_function_edge *  SearchForEdge(t_function_node * node1,t_function_node * node2)
{
  t_function_edge *c_edge; 

  NODE_FOREACH_PRED_EDGE(node1,c_edge)
  {
    t_function_node * target=(t_function_node*)EDGE_HEAD(c_edge);
    if(target==node2)
    {
      if(c_edge->cl_edge==TRUE)
	FATAL(("IMPOSSIBLE"));
      else if(c_edge->cl_edge==FALSE)
      {
//	printf("cl_edge is FALSE!\n");
	return NULL;
      }
      else
      {
	return c_edge;
      }
    }
  }
  NODE_FOREACH_SUCC_EDGE(node1,c_edge)
  {
    t_function_node * target=(t_function_node*)EDGE_TAIL(c_edge);
    if(target==node2)
    {
      if(c_edge->cl_edge==TRUE)
	FATAL(("IMPOSSIBLE"));
      else if(c_edge->cl_edge==FALSE)
      {
//	printf("cl_edge is FALSE!\n");
	return NULL;
      }
      else
      {
	return c_edge;
      }
    }
  }

  int c=0/0;
  FATAL(("NIE GEVODNEN"));
}
    
t_int32 IntegrityCheck(t_function_edge * edge)
{
  t_function_node * member1;
      
  t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
  t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
  
  t_function_edge *c_edge,* d_edge,*x_edge; 
  t_int32 cost=0;
  
#if 0
  if(node1->hulp==FALSE || node2->hulp==FALSE)
  {
//    printf("One of the nodes is False\n");
    return -1;
  }
#endif
  
  member1=node1;
	/* node2 {{{ */
	x_edge=SearchForEdge(member1,node2);
	if(x_edge)
	  cost+=x_edge->switched;
	else
	  return -1;
	/* }}} */
	/* SUCC {{{ */
	NODE_FOREACH_SUCC_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	  }
	}
	/* }}} */
	/* PRED {{{ */
	NODE_FOREACH_PRED_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	  }
	}
	/* }}} */
    
  NODE_FOREACH_PRED_EDGE(node1,c_edge)
  {
    if(((t_function_edge *)c_edge)->cl_edge==TRUE)
    {
      member1=(t_function_node*)EDGE_HEAD(c_edge);
      {
	/* node2 {{{ */
	x_edge=SearchForEdge(member1,node2);
	if(x_edge)
	  cost+=x_edge->switched;
	else
	  return -1;
	/* }}} */
	/* SUCC {{{ */
	NODE_FOREACH_SUCC_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
	    {
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	    }
	  }
	}
	/* }}} */
	/* PRED {{{ */
	NODE_FOREACH_PRED_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
	    {
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	    }
	  }
	}
	/* }}} */
      }
    }
  }

  NODE_FOREACH_SUCC_EDGE(node1,c_edge)
  {
    if(((t_function_edge *)c_edge)->cl_edge==TRUE)
    {
      member1=(t_function_node*)EDGE_TAIL(c_edge);
      {
	/* node2 {{{ */
	x_edge=SearchForEdge(member1,node2);
	if(x_edge)
	  cost+=x_edge->switched;
	else
	  return -1;
	/* }}} */
	/* SUCC {{{ */
	NODE_FOREACH_SUCC_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_TAIL(d_edge);
	    {
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	    }
	  }
	}
	/* }}} */
	/* PRED {{{ */
	NODE_FOREACH_PRED_EDGE(node2,d_edge)
	{
	  if(((t_function_edge *)d_edge)->cl_edge==TRUE)
	  {
	    t_function_node * member2=(t_function_node*)EDGE_HEAD(d_edge);
	    {
	      x_edge=SearchForEdge(member1,member2);
	      if(x_edge)
		cost+=x_edge->switched;
	      else
		return -1;
	    }
	  }
	}
	/* }}} */
      }
    }
  }

  return cost;
}

void Cluster(t_graph * cluster, t_bbl * editor)
{
  t_function_edge * edge; 
  t_function_node *node;
  t_state_list * state_template=NewStateList();

  GRAPH_FOREACH_NODE(cluster,node)
    node->hulp=TRUE;

  t_int32 nn=0;
  GRAPH_FOREACH_NODE(cluster,node)
  {
    if(node->hulp==FALSE)
      continue;

//    printf("\nNEW CLUSTER:\n");
    t_function_list * fun_list=Malloc(sizeof(t_function_list));
    fun_list->first=NULL;
    fun_list->last=NULL;
    t_state_list_list * fun_state_list=NewStateListList();
//    printf("%s ",node->fun->name);
    node->hulp=FALSE;
    t_state_list * state_fun=NewStateList();
    AddFunToFunList(node->fun,fun_list);
    AddStateListToStateListList(state_fun,fun_state_list);

    t_int32 number=1;
    
    NODE_FOREACH_SUCC_EDGE(node,edge)
    {
      if(edge->cl_edge==TRUE)
      {
	//if(nn==0 || nn==1 || ((number >=40&&number<56) || number==20 || number == 56))
	{
	  t_state_list * state_fun1=NewStateList();

	  AddFunToFunList(((t_function_node*)EDGE_TAIL(edge))->fun,fun_list);
	  AddStateListToStateListList(state_fun1,fun_state_list);

//	  printf("%s ",((t_function_node*)EDGE_TAIL(edge))->fun->name);
	  ((t_function_node*)EDGE_TAIL(edge))->hulp=FALSE;
	}
	number++;
      }
    }

    NODE_FOREACH_PRED_EDGE(node,edge)
    {
      if(edge->cl_edge==TRUE)
      {
//	if(nn==0 || nn==1 || (number<20 || number == 56))
	{
	  t_state_list * state_fun1=NewStateList();

	  AddFunToFunList(((t_function_node*)EDGE_TAIL(edge))->fun,fun_list);
	  AddStateListToStateListList(state_fun1,fun_state_list);

//	  printf("%s ",((t_function_node*)EDGE_HEAD(edge))->fun->name);
	  ((t_function_node*)EDGE_TAIL(edge))->hulp=FALSE;
	}
	number++;
      }
    }

//    if(number>1 && nn==0)
    if(number>1)
    {
//      if(!strcmp(fun_list->first->fun->name,"recog_memoized"))
      {
	printf("GEVODNENi=================================================================================================\n");
      state_template = ClusterFunctionsToStateList(fun_list,fun_state_list);
//      PrintCluster(fun_state_list,state_template);
      MapFunctionsToTemplate(state_template, fun_list, fun_state_list, editor);
//      if(nn==1)
//	break;
//      nn++;

      }
    }
//    else nn++;
  }
}

typedef struct _ll_bbl ll_bbl;
struct _ll_bbl{
  t_string name;
  struct _ll_bbl * next;
};

/* void SmClusterFunctions(t_object * obj) {{{ */
void SmClusterFunctions(t_object * obj, t_bbl * editor)
  /*Determine the order of the codebytes and migrate them to instructions of type data so we can keep the rest of diablo*/
{
  t_cfg * cfg = T_CFG(obj->code[0]->data);
  t_chain_holder chains;
  t_function * fun,*fun1=NULL,*fun2=NULL,*fun3=NULL,*fun4=NULL;
  t_state_list * state_fun1=NewStateList();
  t_state_list * state_fun2=NewStateList();
  t_state_list * state_fun3=NewStateList();
  //  t_state_list * state_fun4=NewStateList();
  t_state_list * state_template=NewStateList();
  t_function_list * fun_list=Malloc(sizeof(t_function_list));
  fun_list->first=NULL;
  fun_list->last=NULL;
  t_state_list_list * fun_state_list=NewStateListList();
  //  t_int32 teller=0;
  ll_bbl * first=NULL;

#define USE_TRACE
#ifdef USE_TRACE
  hashtable * list = init_table(30031);

  t_graph * cluster;
  cluster=GraphNew(sizeof(t_function_node),sizeof(t_function_edge));

  I386CreateChains(cfg,&chains);


  /* REad function names {{{*/
  {
    FILE * fd;
    t_string tmp;
    tmp=Malloc(100);
    ll_bbl * tmp_ll;
    t_string fun_str=Malloc(200);

    strcpy(fun_str,diabloobject_options.objpath->dirs[0]);
    strcat(fun_str,"/fun");

    fd=fopen(fun_str,"r");
    if(fd==NULL)
    {
      printf("%s\n",fun_str);
      FATAL(("Unable to open previous file. All functions out of the main should be in that file for the given benchmark."));
    }

    if(fd)
    {
      for(;;)
      {
	ll_bbl * nextl;
//	printf("iNew\n");

	if(fscanf(fd,"%s\n",tmp)==EOF)
	  break;

	nextl=Malloc(sizeof(ll_bbl));
	nextl->name=Malloc(100);
	strcpy(nextl->name,tmp);
	nextl->next=NULL;

	if(first==NULL)
	  first=nextl;
	else
	{
	  tmp_ll=first;
	  while( tmp_ll->next!=NULL)
	  {
	    tmp_ll=tmp_ll->next;
	  }
	  tmp_ll->next=nextl;
	}

      }
    }
  }
  /* }}} */
  
  /* Make nodes for the new graph {{{ */
  CFG_FOREACH_FUN(cfg,fun)
  {
    if(!FUN_IS_HELL(fun))
    {
      if(FromFunToStateList(fun)!=NULL)
      {

	ll_bbl * tmp_ll;
	for(tmp_ll=first;tmp_ll!=NULL;tmp_ll=tmp_ll->next)
	{
	  if(!strcmp(tmp_ll->name,fun->name))
	  {
	    t_function_node * entry=Malloc(sizeof(t_function_node));
	    entry->fun=fun;
	    entry->called=0;
	    entry->destructive=FALSE;
	    entry->escaping=FALSE;
	    GraphInitNode(cluster,T_NODE(entry));
	    put(list,BBL_OLD_ADDRESS(FUN_BBL_FIRST(fun)),entry);
//	    printf("%s %x\n",fun->name,BBL_OLD_ADDRESS(FUN_BBL_FIRST(fun)));
	    break;
	  }
	}
      }
    }
  }
  /* }}} */

  /* Look for escaping edges and indicate{{{ */
  {
    t_function_node * node;

    GRAPH_FOREACH_NODE(cluster,node)
    {
      t_function * h_fun=node->fun;
      t_bbl * bbl;

      FUN_FOREACH_BBL(h_fun,bbl)
      {
	t_cfg_edge * cf_edge;

	BBL_FOREACH_SUCC_EDGE(bbl,cf_edge)
	{

	  if(EDGE_CAT(cf_edge)==ET_IPJUMP ||
	      EDGE_CAT(cf_edge)==ET_IPFALLTHRU ||
	      EDGE_CAT(cf_edge)==ET_IPUNKNOWN )
	  {
	    //	    printf("ESCAPING EDGE:%s to %s\n",BBL_FUNCTION(T_BBL(EDGE_HEAD(cf_edge)))->name,BBL_FUNCTION(T_BBL(EDGE_TAIL(cf_edge)))->name);
	    node->escaping=TRUE;
	  }
	}
	BBL_FOREACH_PRED_EDGE(bbl,cf_edge)
	{
	  if(EDGE_CAT(cf_edge)==ET_IPJUMP ||
	      EDGE_CAT(cf_edge)==ET_IPFALLTHRU ||
	      EDGE_CAT(cf_edge)==ET_IPUNKNOWN )
	  {
	    //	    printf("ESCAPING EDGE:%s to %s\n",BBL_FUNCTION(T_BBL(EDGE_HEAD(cf_edge)))->name,BBL_FUNCTION(T_BBL(EDGE_TAIL(cf_edge)))->name);
	    node->escaping=TRUE;
	  }
	}
      }
    }
  }
  /* }}} */

  /* Make edges: No edges if one node has escaping edges{{{ */
  {
    t_function_node * node,*node2;
    t_function_edge * edge; 
    for (node=T_FUN_NODE(GRAPH_NODE_FIRST(cluster)); node!=NULL && T_FUN_NODE(NODE_NEXT(node))!=NULL; node=T_FUN_NODE(NODE_NEXT(node)))
    {
      for (node2=T_FUN_NODE(NODE_NEXT(node)); node2!=NULL; node2=T_FUN_NODE(NODE_NEXT(node2)))
      {
	if((!node->escaping)&&(!node2->escaping))
	{
	  edge=(t_function_edge*)GraphAppendNode(cluster,T_NODE(node),T_NODE(node2),0);
	  edge->switched=0;
	  edge->last=-1;
	  edge->path_exists=FALSE;
	  edge->cl_edge=PERHAPS;
	}
      }
    }
  }
  /* }}} */

  /* Read trace file and ste numbers in graph to the right number {{{ */
  {
    t_string fun_str=Malloc(200);
    t_function_edge * edge; 

    t_uint32 word;
    strcpy(fun_str,diabloobject_options.objpath->dirs[0]);
    strcat(fun_str,"/trace");
    int fd = open(fun_str,O_RDONLY);
    if(fd==-1)
    {
      printf("%s\n",fun_str);
      FATAL(("No trace-file given...."));
    }
    else
      printf("Reading trace file\n");
    t_uint64 x=0;

//    printf("bzip:14.000.000 entries\n");
//    printf("FD:%d\n",fd);
    t_uint64 nr_entry=0;
    for(;;)
    {

      if( read(fd,&word,4)!=4)
	break;
      //    printf("%x ",word);

      nr_entry++;
//      if(nr_entry==1000001)
//	break;
      table_entry * te=find(list,word);
      if(te!=NULL)
      {
	if(te->fun_node==NULL)
	  FATAL(("Hoe kan da?"));
	te->fun_node->called++;
	NODE_FOREACH_PRED_EDGE(te->fun_node,edge)
	{
	  if(edge->last==-1)
	    edge->last=word;
	  else if(word!=edge->last)
	  {
	    edge->switched++;
	    edge->last=word;
	  }
	}
	NODE_FOREACH_SUCC_EDGE(te->fun_node,edge)
	{
	  if(edge->last==-1)
	    edge->last=word;
	  else if(word!=edge->last)
	  {
	    edge->switched++;
	    edge->last=word;
	  }
	}
      }
      x++;
      if(x%1000000==0)
	printf("%d\n",x);
    }
    printf("Entries in trace:%ld\n",x);

    close(fd);
  }
  /* }}} */

  /* path_exists for edges: CG: one node can reach the other {{{ */
  {
    t_function_node * node;
    t_function_edge * edge; 
    t_int32 l,x=0;

    GRAPH_FOREACH_NODE(cluster,node)
    {
      t_function * h_fun=node->fun;
      t_bbl * bbl;
      x=0;

//      printf("%s:\n",h_fun->name);
      l=IndicateAllReachableFunctions(h_fun);
//            printf("%s:%d\n",h_fun->name,l);


      NODE_FOREACH_SUCC_EDGE(node,edge)
      {
	if(FUN_FLAGS(((((t_function_node*)EDGE_TAIL(edge))->fun)))& FF_IS_MARKED2)
	{
//	  printf("From:%s To:%s \n",h_fun->name,((t_function_node*)EDGE_TAIL(edge))->fun->name);
	  edge->path_exists=TRUE;
//	  printf("MAKE EDGE FALSE!\n");
	  edge->cl_edge=FALSE;
	}

      }
      NODE_FOREACH_PRED_EDGE(node,edge)
      {
	if(FUN_FLAGS(((((t_function_node*)EDGE_HEAD(edge))->fun)))& FF_IS_MARKED2)
	{
//	  	  printf("%03d From:%s\n",x++,((t_function_node*)EDGE_HEAD(edge))->fun->name);
//	  printf("From:%s To:%s \n",((t_function_node*)EDGE_HEAD(edge))->fun->name,h_fun->name);
	  edge->path_exists=TRUE;
	  edge->cl_edge=FALSE;
//	  printf("MAKE EDGE FALSE!\n");
	}
      }
    }
  }
  /* }}} */

  /* Nodes that are executed more than a given number/percentage can't be clustered; we make them self-destructive {{{ */
  {
    t_function_node * node;
    t_function_edge * edge; 
    t_function_edge * tmp; 
    t_uint64 total=0;
    //double percentage=0.5;
    double percentage=1; //AFZETTEN!

    printf("Execution percentage:%lf\n",percentage);

    GRAPH_FOREACH_NODE(cluster,node)
    {
      total+=node->called;
    }

    t_uint64 max_nr=percentage*total;

    printf("max:%d\n",max_nr);

    GRAPH_FOREACH_NODE(cluster,node)
    {
      if(node->escaping==FALSE && node->called > max_nr)
      {
	printf("%s \t\t%d TOO MUCH EXECUTED!\n",node->fun->name,node->called);
	node->destructive=TRUE;
	/* Remove all edges from and too this node {{{*/
	NODE_FOREACH_PRED_EDGE_SAFE(node,edge,tmp)
	  GraphRemoveEdge(cluster, edge);
	NODE_FOREACH_SUCC_EDGE_SAFE(node,edge,tmp)
	  GraphRemoveEdge(cluster, edge);
	/* }}} */
      }
    }
  }
  /* }}} */

  /* Cluster functions Can't be clustered: Functions that interact too much {{{*/
  {
    t_function_node * node;
    t_function_node * smallest_candidate;
    t_function_edge * edge; 
    t_function_edge * tmp; 
    t_int64 total=0;
    t_int32 total_nr_functions=0;
    double percentage=0.1;
    t_int32 i=0;
    percentage/=2;/*Correctie omdat alle switchings 2X geteld worden.*/

    printf("Switching percentage:%lf\n",percentage);
    GRAPH_FOREACH_NODE(cluster,node)
    {
      total_nr_functions++;
      NODE_FOREACH_SUCC_EDGE(node,edge)
	total+=edge->switched;
      NODE_FOREACH_PRED_EDGE(node,edge)
	total+=edge->switched;
    }
    t_int64 max_nr=percentage*total;
    printf("total:%d\n",total);
    printf("max:%d\n",max_nr);

    GRAPH_FOREACH_NODE(cluster,node)
      node->hulp=TRUE;

    GRAPH_FOREACH_NODE(cluster,node)
    {
      t_int64 smallest=-1;
      t_function_edge * smallest_edge=NULL;

      NODE_FOREACH_PRED_EDGE(node,edge)
      {
	if(edge->cl_edge!=PERHAPS)
	{
	  continue;
	}

	t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
	t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
	t_int32 add=0;

	if(node1->hulp==FALSE || node2->hulp==FALSE)
	  add=-1;
	else
	  add=IntegrityCheck(edge);

	if(smallest==-1 || (add!=-1 && add<smallest))
	{
	  smallest_edge=edge;
	  smallest=add;
	}
      }
      NODE_FOREACH_SUCC_EDGE(node,edge)
      {
	if(edge->cl_edge!=PERHAPS)
	{
	  continue;
	}

	t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
	t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
	t_int32 add=0;

	if(node1->hulp==FALSE || node2->hulp==FALSE)
	  add=-1;
	else
	  add=IntegrityCheck(edge);

	if(smallest==-1 || (add!=-1 && add<smallest))
	{
	  smallest_edge=edge;
	  smallest=add;
	}
      }
      if(smallest!=-1)
	if (max_nr-smallest>=0)
	{
	  //	  printf("smallest:: %d\n",smallest);
	  AddNodeToCluster(smallest_edge);
	  max_nr-=smallest;
	  //	  printf("max:%d\n",max_nr);
	}
	else
	  printf("Over Budget:Budget:%d smallest:%d\n",max_nr,smallest);
    }
    t_uint32 k;
    for(k=1;k<total_nr_functions;k++)
    {

      GRAPH_FOREACH_NODE(cluster,node)
	node->hulp=FALSE;

      GRAPH_FOREACH_NODE(cluster,node)
      {
	t_int64 smallest=-1;
	t_function_edge * smallest_edge=NULL;
	t_int32 nr_in_cluster=1;

	NODE_FOREACH_PRED_EDGE(node,edge)
	  if(edge->cl_edge==TRUE)
	    nr_in_cluster++;
	NODE_FOREACH_SUCC_EDGE(node,edge)
	  if(edge->cl_edge==TRUE)
	    nr_in_cluster++;

	if(nr_in_cluster==k)
	{
	  //	printf("nr_in_cluster==%d\n%s\n",k,node->fun->name);

	  node->hulp=TRUE;

	  NODE_FOREACH_PRED_EDGE(node,edge)
	    if(edge->cl_edge==TRUE)
	      ((t_function_node*)EDGE_HEAD(edge))->hulp=TRUE;
	  NODE_FOREACH_SUCC_EDGE(node,edge)
	    if(edge->cl_edge==TRUE)
	      ((t_function_node*)EDGE_TAIL(edge))->hulp=TRUE;
	}
      }

      GRAPH_FOREACH_NODE(cluster,node)
      {
	t_int64 smallest=-1;
	t_function_edge * smallest_edge=NULL;

	NODE_FOREACH_PRED_EDGE(node,edge)
	{
	  if(edge->cl_edge!=PERHAPS)
	  {
	    continue;
	  }
	  t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
	  t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
	  t_int32 add=0;

	  if(node1->hulp==FALSE)
	    add=-1;
	  else
	    add=IntegrityCheck(edge);

	  if(smallest==-1 || (add!=-1 && add<smallest))
	  {
	    smallest_edge=edge;
	    smallest=add;
	  }
	}
	NODE_FOREACH_SUCC_EDGE(node,edge)
	{
	  if(edge->cl_edge!=PERHAPS)
	  {
	    continue;
	  }
	  t_function_node * node1=(t_function_node*)EDGE_HEAD(edge);
	  t_function_node * node2=(t_function_node*)EDGE_TAIL(edge);
	  t_int32 add=0;

	  if(node2->hulp==FALSE)
	    add=-1;
	  else
	    add=IntegrityCheck(edge);


	  if(smallest==-1 || (add!=-1 && add<smallest))
	  {
	    smallest_edge=edge;
	    smallest=add;
	  }
	}
	if(smallest!=-1)
	  if (max_nr-smallest>=0)
	  {
	    //	  printf("smallest:: %d\n",smallest);
	    AddNodeToCluster(smallest_edge);

	    t_function_node * node1=(t_function_node*)EDGE_HEAD(smallest_edge);
	    t_function_node * node2=(t_function_node*)EDGE_TAIL(smallest_edge);
	    //	printf("CLUSTER:\n1)%s\n%s\n",node1->fun->name,node2->fun->name);

	    max_nr-=smallest;
	    //	  printf("max:%d\n",max_nr);
	  }
	  else
	    printf("Over Budget:Budget:%d smallest:%d\n",max_nr,smallest);
      }
    }
    printf("Na aftrek van alle pijlen:%d\n", max_nr);
  }
  /* }}} */

  /* Pri nt-functie: {{{ */
  {
    t_function_node * node;
    t_int32 escape=0;
    t_int32 execution=0;
    t_int32 clusters=0;
    t_int32 destructive=0;
    t_int32 last_destructive=0;
    t_int32 fun_in_clusters=0;

    printf("\nFunctions; Impossible to make self-mod due to escaping edges:\n");
    printf("-------------------------------------------------------------\n");

    GRAPH_FOREACH_NODE(cluster,node)
    {
      if(node->escaping==TRUE)
      {
	printf("%s\n",node->fun->name);
	escape++;
      }
    }

    printf("Functions; Impossible to cluster due to high execution count:\n");
    printf("-------------------------------------------------------------\n");

    GRAPH_FOREACH_NODE(cluster,node)
    {
      if(node->destructive==TRUE)
      {
	printf("%s\n",node->fun->name);
	execution++;
      }
    }
    
    printf("Functions; Clusters:\n");
    printf("--------------------\n");

    t_function_edge * edge; 

    GRAPH_FOREACH_NODE(cluster,node)
      node->hulp=TRUE;

    GRAPH_FOREACH_NODE(cluster,node)
    {
      if(node->hulp==FALSE)
	continue;

      printf("\nNEW CLUSTER:\n");
      printf("%s ",node->fun->name);
      node->hulp=FALSE;
      destructive++;
      fun_in_clusters++;

      NODE_FOREACH_SUCC_EDGE(node,edge)
      {
	if(edge->cl_edge==TRUE)
	{
	  printf("%s ",((t_function_node*)EDGE_TAIL(edge))->fun->name);
	  ((t_function_node*)EDGE_TAIL(edge))->hulp=FALSE;
	  fun_in_clusters++;
	  if(destructive>last_destructive)
	  {
	    clusters++;
	    last_destructive=destructive;
	  }
	}
      }

      NODE_FOREACH_PRED_EDGE(node,edge)
      {
	if(edge->cl_edge==TRUE)
	{
	  printf("%s ",((t_function_node*)EDGE_HEAD(edge))->fun->name);
	  ((t_function_node*)EDGE_TAIL(edge))->hulp=FALSE;
	  fun_in_clusters++;
	  if(destructive>last_destructive)
	  {
	    clusters++;
	    last_destructive=destructive;
	  }
	}
      }
    }
    printf("Numbers:\n(1)Nr of functions with escaping edges\n(2)Nr of functions with a too high execution\n(3)Desctructive functions (ex.:main)\n(4)Nr of Clusters\n(5)Nr Of functions in Cluster\n");
    printf("%d\n%d\n%d\n%d\n%d\n",escape,execution,destructive-clusters,clusters,fun_in_clusters-destructive+clusters);
  }
  /* }}}*/

  Cluster(cluster,editor);

#endif


  I386CreateChains(cfg,&chains);

#if 0
  /* Hier komt functie die beslist welke functies er geclustered moeten worden 
   * Voorlopig nemen we spec_uncompress en spec_compress {{{ */
  CFG_FOREACH_FUN(cfg,fun)
  {
    //    if(!strcmp(fun->name,"spec_getc"))
//    if(!strcmp(fun->name,"__libc_check_standard_fds"))
    if(!strcmp(fun->name,"hbCreateDecodeTables"))
    {
      printf("fun1:%s Found\n",fun->name);
      fun1=fun;
    }
        if(!strcmp(fun->name,"spec_ungetc"))
    //if(!strcmp(fun->name,"__do_global_ctors_aux"))
    {
      printf("fun2:%s Found\n",fun->name);
      fun2=fun;
    }
//    if(!strcmp(fun->name,"spec_load"))
  //  {
    //  printf("fun3:%s Found\n",fun->name);
      //fun3=fun;
//    }
    //    if(!strcmp(fun->name,"spec_write")||
    //	!strcmp(fun->name,"fullGtU"))//bzip2
    //    {
    //      printf("fun4 Found\n");
    //      fun4=fun;
    //    }
  }
  if(!fun1)
    FATAL(("Geen fun1!"));
  if(!fun2)
    FATAL(("Geen fun2!"));
//  if(!fun3)
  //  FATAL(("Geen fun3!"));
  //  if(!fun4)
  //    FATAL(("Geen fun4!"));
  /* }}} */

  AddFunToFunList(fun1,fun_list);
  AddFunToFunList(fun2,fun_list);
//  AddFunToFunList(fun3,fun_list);
  //  AddFunToFunList(fun4,fun_list);
  AddStateListToStateListList(state_fun1,fun_state_list);
  AddStateListToStateListList(state_fun2,fun_state_list);
//  AddStateListToStateListList(state_fun3,fun_state_list);
  //  AddStateListToStateListList(state_fun4,fun_state_list);

  state_template = ClusterFunctionsToStateList(fun_list,fun_state_list);

  MapFunctionsToTemplate(state_template, fun_list, fun_state_list, editor);

  PrintCluster(fun_state_list,state_template);
#endif

  /* Vanaf hier Bertrand zijn stuk {{{ */
#if 0
  {
    t_state_list * editscript1;
    t_state_list * editscript2;
    t_state_list * editscript3;
    t_bbl * bbl;
    t_bbl * stub1;
    t_bbl * stub2;
    t_bbl * stub3;
    t_cfg_edge * edge, * iter_edge;
    t_state * state;
    t_state_item * state_item;


    FUN_FOREACH_BBL(fun1,bbl)
    {
      while(RELOCATABLE_REFED_BY(bbl))
	RelocSetToRelocatable(RELOCATABLE_REFED_BY(bbl)->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(bbl))->state->byte_ins);
    }
    FUN_FOREACH_BBL(fun2,bbl)
    {
      while(RELOCATABLE_REFED_BY(bbl))
	RelocSetToRelocatable(RELOCATABLE_REFED_BY(bbl)->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(bbl))->state->byte_ins);
    }
    FUN_FOREACH_BBL(fun3,bbl)
    {
      while(RELOCATABLE_REFED_BY(bbl))
	RelocSetToRelocatable(RELOCATABLE_REFED_BY(bbl)->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(bbl))->state->byte_ins);
    }
    {
      state_item = state_template->first;
      while(state_item)
      {
	t_state * state_old = state_item->state;
	t_state * state_new;
	/*wijzer werd gekopieerd*/
	if(state_old->byte_ins)
	{
	  state_new = NewStateEmpty();
	  state_new->byte_ins = InsNewForCfg(cfg);
	  state_new->value_relocatable = state_old->value_relocatable;
	  state_new->dontcare = state_old->dontcare;
	  INS_TYPE(state_new->byte_ins) = IT_DATA;
	  I386_INS_DATA(state_new->byte_ins) = I386_INS_DATA(state_old->byte_ins);
	  state_new->value = state_old->value;
	  if(RELOCATABLE_REFERS_TO(state_old->byte_ins))
	  {
	    t_reloc * rel = RELOCATABLE_REFERS_TO(state_old->byte_ins)->rel;
	    RelocTableDupReloc(REL_RELOCTABLE(rel),rel);
	    RelocSetFrom(rel,state_new->byte_ins);
	  }
	  state_item->state = state_new;
	}
	/*nieuwe state werd aangemaakt*/
	else
	{
	  state_old->byte_ins = InsNewForCfg(cfg);
	  INS_TYPE(state_old->byte_ins) = IT_DATA;
	  I386_INS_DATA(state_old->byte_ins) = 0;
	}
	state_item = state_item->next;
      }
    }
    {
      t_state_item * state_item1=state_fun1->first;
      t_state_item * state_item2=state_fun2->first;
      t_state_item * state_item3=state_fun3->first;
      t_state_item * state_item_template=state_template->first;
      t_int32 teller=0;
      printf("END: --------------------------------------------\nFun1\t\tFun2\t\tFun3\t\tTemplate\n");
      for(;state_item1!=NULL&&state_item2!=NULL&&state_item3!=NULL&&state_item_template!=NULL;)
      {
	printf("%d %d 0x%02x\t",state_item1->state->dontcare,state_item1->state->value_relocatable,state_item1->state->value);
	printf("%d %d 0x%02x\t",state_item2->state->dontcare,state_item2->state->value_relocatable,state_item2->state->value);
	printf("%d %d 0x%02x\n",state_item3->state->dontcare,state_item3->state->value_relocatable,state_item3->state->value);
	printf("%d %d 0x%02x\n",state_item_template->state->dontcare,state_item_template->state->value_relocatable,state_item_template->state->value);
	state_item1=state_item1->next;
	state_item2=state_item2->next;
	state_item3=state_item3->next;
	state_item_template=state_item_template->next;
	teller++;
      }
      printf("total nr of bytes in cluster template:%d\n",teller);
    }

    editscript1 = CreateEditScript(state_fun1, state_template, cfg, 3, 0);
    editscript2 = CreateEditScript(state_fun2, state_template, cfg, 2, 0);
    editscript3 = CreateEditScript(state_fun3, state_template, cfg, 1, 3);

    {
      t_state_item * item_fun2 = state_fun2->first;
      t_state_item * item_fun1 = state_fun1->first;
      t_state_item * item_fun3 = state_fun3->first;
      t_state_item * item_template = state_template->first;
      while(item_fun2)
      {
	while(!item_fun3->state->dontcare && RELOCATABLE_E_REFED_BY(item_fun3->state->byte_ins))
	  RelocESetToRelocatable(RELOCATABLE_E_REFED_BY(item_fun3->state->byte_ins)->rel,item_template->state->byte_ins);

	while(!item_fun2->state->dontcare && RELOCATABLE_E_REFED_BY(item_fun2->state->byte_ins))
	  RelocESetToRelocatable(RELOCATABLE_E_REFED_BY(item_fun2->state->byte_ins)->rel,item_template->state->byte_ins);

	while(!item_fun1->state->dontcare && RELOCATABLE_E_REFED_BY(item_fun1->state->byte_ins))
	  RelocESetToRelocatable(RELOCATABLE_E_REFED_BY(item_fun1->state->byte_ins)->rel,item_template->state->byte_ins);

	while(!item_fun3->state->dontcare && RELOCATABLE_REFED_BY(item_fun3->state->byte_ins))
	  RelocSetToRelocatable(RELOCATABLE_REFED_BY(item_fun3->state->byte_ins)->rel,item_template->state->byte_ins);

	while(!item_fun2->state->dontcare && RELOCATABLE_REFED_BY(item_fun2->state->byte_ins))
	  RelocSetToRelocatable(RELOCATABLE_REFED_BY(item_fun2->state->byte_ins)->rel,item_template->state->byte_ins);

	while(!item_fun1->state->dontcare && RELOCATABLE_REFED_BY(item_fun1->state->byte_ins))
	  RelocSetToRelocatable(RELOCATABLE_REFED_BY(item_fun1->state->byte_ins)->rel,item_template->state->byte_ins);

	item_fun3 = item_fun3->next;
	item_fun2 = item_fun2->next;
	item_fun1 = item_fun1->next;
	item_template = item_template->next;
      }
    }

    bbl = BblNew(cfg);
    AddStatelistToBbl(state_template,bbl,cfg);

    bbl = BblNew(cfg);
    AddStatelistToBbl(editscript1,bbl);

    bbl = BblNew(cfg);
    AddStatelistToBbl(editscript2,bbl);

    bbl = BblNew(cfg);
    AddStatelistToBbl(editscript3,bbl);

    {
      t_ins * fun1_entry_ins = NULL;
      t_ins * fun2_entry_ins = NULL;
      t_ins * fun3_entry_ins = NULL;
      {
	t_state * fun1_entry_state = INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun1)))->state;
	t_state * fun2_entry_state = INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun2)))->state;
	t_state * fun3_entry_state = INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun3)))->state;

	t_state_item * item_fun3 = state_fun3->first;
	t_state_item * item_fun2 = state_fun2->first;
	t_state_item * item_fun1 = state_fun1->first;
	t_state_item * item_template = state_template->first;
	while(item_fun2)
	{
	  if(item_fun3->state==fun3_entry_state)
	    fun3_entry_ins = item_template->state->byte_ins;
	  if(item_fun1->state==fun1_entry_state)
	    fun1_entry_ins = item_template->state->byte_ins;
	  if(item_fun2->state==fun2_entry_state)
	    fun2_entry_ins = item_template->state->byte_ins;

	  item_fun3 = item_fun3->next;
	  item_fun2 = item_fun2->next;
	  item_fun1 = item_fun1->next;
	  item_template = item_template->next;
	}
      }
      if(fun1_entry_ins == NULL || fun2_entry_ins == NULL || fun3_entry_ins == NULL)
	FATAL((""));

      stub1 = CreateStub(editor, fun1_entry_ins, editscript1->first->state->byte_ins, cfg, 0);
      stub2 = CreateStub(editor, fun2_entry_ins, editscript2->first->state->byte_ins, cfg, 1);
      stub3 = CreateStub(editor, fun3_entry_ins, editscript3->first->state->byte_ins, cfg, 2);
    }

    {
      t_bool again = TRUE;
      while(again){
	again = FALSE;
	edge = NULL;
	FUN_FOREACH_BBL(fun1,bbl)
	{
	  BBL_FOREACH_PRED_EDGE(bbl,iter_edge)
	  {
	    if(EDGE_CAT(iter_edge)==ET_CALL)
	    {
	      edge = iter_edge;
	      break;
	    }
	  }
	  if(edge!=NULL)
	    break;
	}

	if(edge)
	{
	  t_state_item * state_item;
	  again = TRUE;
	  INS_FOREACH_STATE_ITEM(BBL_INS_LAST(EDGE_HEAD(edge)),state_item)
	  {
	    t_reloc * rel;
	    if(RELOCATABLE_REFERS_TO(state_item->state->byte_ins)){
	      rel = RELOCATABLE_REFERS_TO(state_item->state->byte_ins)->rel;
	      RelocSetToRelocatable(rel,stub1);
	    }
	  }
	  GraphRemoveEdge(T_GRAPH(cfg),edge);
	}

      }
    }

    {
      t_bool again = TRUE;
      while(again){
	again = FALSE;

	edge = NULL;
	FUN_FOREACH_BBL(fun2,bbl)
	{
	  BBL_FOREACH_PRED_EDGE(bbl,iter_edge)
	  {
	    if(EDGE_CAT(iter_edge)==ET_CALL)
	    {
	      edge = iter_edge;
	      break;
	    }
	  }
	  if(edge!=NULL)
	    break;
	}

	if(edge)
	{
	  t_state_item * state_item;
	  again = TRUE;
	  INS_FOREACH_STATE_ITEM(BBL_INS_LAST(EDGE_HEAD(edge)),state_item)
	  {
	    t_reloc * rel;
	    if(RELOCATABLE_REFERS_TO(state_item->state->byte_ins)){
	      rel = RELOCATABLE_REFERS_TO(state_item->state->byte_ins)->rel;
	      RelocSetToRelocatable(rel,stub2);
	    }
	  }
	  GraphRemoveEdge(T_GRAPH(cfg),edge);
	}

      }
    }

    {
      t_bool again = TRUE;
      while(again){
	again = FALSE;

	edge = NULL;
	FUN_FOREACH_BBL(fun3,bbl)
	{
	  BBL_FOREACH_PRED_EDGE(bbl,iter_edge)
	  {
	    if(EDGE_CAT(iter_edge)==ET_CALL)
	    {
	      edge = iter_edge;
	      break;
	    }
	  }
	  if(edge!=NULL)
	    break;
	}

	if(edge)
	{
	  t_state_item * state_item;
	  again = TRUE;
	  INS_FOREACH_STATE_ITEM(BBL_INS_LAST(EDGE_HEAD(edge)),state_item)
	  {
	    t_reloc * rel;
	    if(RELOCATABLE_REFERS_TO(state_item->state->byte_ins)){
	      rel = RELOCATABLE_REFERS_TO(state_item->state->byte_ins)->rel;
	      RelocSetToRelocatable(rel,stub3);
	    }
	  }
	  GraphRemoveEdge(T_GRAPH(cfg),edge);
	}

      }
    }

  }
#endif
  /* }}} */
}
/* }}} */

void MapFunctionsToTemplate(t_state_list * state_template, t_function_list * fun_list, t_state_list_list * state_lists, t_bbl * editor)
{
  t_state_list * editscript, * state_fun;
  t_state_list_item * state_list_item;
  t_bbl * stub, * bbl;
  t_function_item * fun_item;
  t_function * fun;
  t_cfg * cfg = FUNCTION_CFG(fun_list->first->fun);
  t_bbl * previous_stub = NULL;
  t_bbl * first_stub = NULL;
  int i;
  int nr_of_functions = 0;
  t_state_list_list * list_of_edit_scripts = Calloc(1,sizeof(t_state_list_list));
  
  for(fun_item = fun_list->first;
      fun_item!=NULL;
      fun_item = fun_item->next)
    nr_of_functions++;

  /*Make sure the template has his own instructions*/
  {
    t_state_item * state_item = state_template->first;
    while(state_item)
    {
      t_state * state_old = state_item->state;
      t_state * state_new;
      /*wijzer werd gekopieerd*/
      if(state_old->byte_ins)
      {
	state_new = NewStateEmpty();
	state_new->byte_ins = InsNewForCfg(cfg);
	state_new->value_relocatable = state_old->value_relocatable;
	state_new->dontcare = state_old->dontcare;
	INS_TYPE(state_new->byte_ins) = IT_DATA;
	I386_INS_DATA(state_new->byte_ins) = I386_INS_DATA(state_old->byte_ins);
	state_new->value = state_old->value;
	if(RELOCATABLE_REFERS_TO(state_old->byte_ins))
	{
	  t_reloc * rel = RELOCATABLE_REFERS_TO(state_old->byte_ins)->rel;
	  RelocTableDupReloc(REL_RELOCTABLE(rel),rel);
	  RelocSetFrom(rel,state_new->byte_ins);
	}
	state_item->state = state_new;
      }
      /*nieuwe state werd aangemaakt*/
      else
      {
	state_old->byte_ins = InsNewForCfg(cfg);
	INS_TYPE(state_old->byte_ins) = IT_DATA;
	I386_INS_DATA(state_old->byte_ins) = 0;
      }
      state_item = state_item->next;
    }
  }
  
  /*add template*/
  bbl = BblNew(cfg);
  AddStatelistToBbl(state_template,bbl,cfg);
  
  /*for each function (and corresponding state_list) in the cluster*/
  for(fun_item = fun_list->first, state_list_item = state_lists->first, i=0;
      fun_item!=NULL && state_list_item!=NULL;
      fun_item = fun_item->next, state_list_item = state_list_item->next, i++)
  {
    t_reloc_ref * relocs_to_entry = NULL;
    t_reloc_ref * relocs_current = NULL;

    t_reloc_ref * e_relocs_to_entry = NULL;
    t_reloc_ref * e_relocs_current = NULL;
  
    fun = fun_item->fun;
    state_fun = state_list_item->state_list;
    
    /*remember relocations to entry of ins {{{*/
    {
      t_reloc_ref * reloc_ref = RELOCATABLE_REFED_BY(FUN_BBL_FIRST(fun));
      while(reloc_ref)
      {
	if(relocs_to_entry == NULL)
	{
	  relocs_to_entry = Calloc(1,sizeof(t_reloc_ref));
	  relocs_current = relocs_to_entry;
	}
	else
	{
	  relocs_current->next = Calloc(1,sizeof(t_reloc_ref));
	  relocs_current->next->prev = relocs_current;
	  relocs_current = relocs_current->next;
	}
	relocs_current->rel = reloc_ref->rel;
	reloc_ref = reloc_ref->next;
      }
      
      if(RELOCATABLE_REFED_BY(BBL_INS_FIRST(FUN_BBL_FIRST(fun))))
	FATAL((""));
      if(RELOCATABLE_REFED_BY(INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun)))->state->byte_ins))
	FATAL((""));
      
      if(RELOCATABLE_E_REFED_BY(BBL_INS_FIRST(FUN_BBL_FIRST(fun))))
	FATAL((""));
      if(RELOCATABLE_E_REFED_BY(INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun)))->state->byte_ins))
	FATAL((""));
      
      reloc_ref = RELOCATABLE_E_REFED_BY(FUN_BBL_FIRST(fun));
      while(reloc_ref)
      {
	if(e_relocs_to_entry == NULL)
	{
	  e_relocs_to_entry = Calloc(1,sizeof(t_reloc_ref));
	  e_relocs_current = e_relocs_to_entry;
	}
	else
	{
	  e_relocs_current->next = Calloc(1,sizeof(t_reloc_ref));
	  e_relocs_current->next->prev = e_relocs_current;
	  e_relocs_current = e_relocs_current->next;
	}
	e_relocs_current->rel = reloc_ref->rel;
	reloc_ref = reloc_ref->next;
      }
    }/*}}}*/
    
    /*move relocations to byte_ins{{{*/
    FUN_FOREACH_BBL(fun,bbl)
    {
      while(RELOCATABLE_REFED_BY(bbl))
	RelocSetToRelocatable(RELOCATABLE_REFED_BY(bbl)->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(bbl))->state->byte_ins);
      while(RELOCATABLE_E_REFED_BY(bbl))
	RelocESetToRelocatable(RELOCATABLE_E_REFED_BY(bbl)->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(bbl))->state->byte_ins);
    }/*}}}*/
    
    /*migrate relocations to template{{{*/
    {
      t_state_item * item_fun = state_fun->first, * item_template = state_template->first;
      while(item_fun)
      {
	while(!item_fun->state->dontcare && RELOCATABLE_E_REFED_BY(item_fun->state->byte_ins))
	  RelocESetToRelocatable(RELOCATABLE_E_REFED_BY(item_fun->state->byte_ins)->rel,item_template->state->byte_ins);
	
	while(!item_fun->state->dontcare && RELOCATABLE_REFED_BY(item_fun->state->byte_ins))
	  RelocSetToRelocatable(RELOCATABLE_REFED_BY(item_fun->state->byte_ins)->rel,item_template->state->byte_ins);
	item_fun = item_fun->next;
	item_template = item_template->next;
      }
    }
    /*}}}*/
    
    /*guess*/
    //editscript = CreateEditScript(state_fun, state_template, cfg, 0, 0);
    if(nr_of_functions > 255)
      FATAL(("too many functions in template"));
    editscript = CreateEditScript(state_fun, state_template, cfg, nr_of_functions-i, nr_of_functions);
    
    AddStateListToStateListList(editscript,list_of_edit_scripts);
     
    bbl = BblNew(cfg);
    AddStatelistToBbl(editscript,bbl);

    
    /*Create Stub; migrate relocations to fun_entry_ins to stub*/
    {
      t_ins * fun_entry_ins = NULL;
      t_state * fun_entry_state = INS_STATE_ITEM_FIRST(BBL_INS_FIRST(FUN_BBL_FIRST(fun)))->state;
      t_state_item * item_fun = state_fun->first;
      t_state_item * item_template = state_template->first;
      /*find entry in template*/
      while(item_fun)
      {
	if(item_fun->state==fun_entry_state)
	{
	  fun_entry_ins = item_template->state->byte_ins;
	  break;
	}
	item_fun=item_fun->next;
	item_template=item_template->next;
      }
      if(fun_entry_ins==NULL)
	FATAL((""));

      stub = CreateStub(editor, fun_entry_ins, editscript->first->state->byte_ins, cfg, i, first_stub, INS_OLD_ADDRESS(BBL_INS_FIRST(FUN_BBL_FIRST(fun))));
      if(first_stub == NULL)
	first_stub = stub;


      /*force stub sequence*/
      if(previous_stub)
      {
	CfgEdgeCreate(cfg,previous_stub,stub,ET_FALLTHROUGH);
      }
      previous_stub = BBL_NEXT(BBL_NEXT(stub));

    }

    {
      t_reloc_ref * reloc_ref = relocs_to_entry;
      while(reloc_ref)
      {
	RelocSetToRelocatable(reloc_ref->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(stub))->state->byte_ins);
	reloc_ref = reloc_ref->next;
      }

      reloc_ref = e_relocs_to_entry;
      while(reloc_ref)
      {
	RelocESetToRelocatable(reloc_ref->rel,INS_STATE_ITEM_FIRST(BBL_INS_FIRST(stub))->state->byte_ins);
	reloc_ref = reloc_ref->next;
      }
    }
  } 
}

/* vim: set shiftwidth=2 foldmethod=marker:*/
