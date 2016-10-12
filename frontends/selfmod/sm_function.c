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
#include "sm_function.h"
#include "sm_similarity.h"
#include "../../arch/i386/i386_deflowgraph.h"
#include "../../arch/i386/i386_instructions.h"
#include "../diablo_options.h"
 #include <fcntl.h>

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
  {
    Free(ch_fun1);
    return NULL;
  }

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

/* vim: set shiftwidth=2 foldmethod=marker:*/
