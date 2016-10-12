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
 * This file contains all arch-independent functions to handle basic blocks in
 * objects
 *
 * Written by Bertrand Anckaert  (Bertrand.Anckaert@UGent.be)
 *
 * Copyright (c) 2005 Bertrand Anckaert
 *
 * THIS FILE IS PART OF DIABLO (SM)
 *
 * MAINTAINER: Bertrand Anckaert (Bertrand.Anckaert@UGent.be)  }}}
 *****************************************************************************/

#include "sm_codebyte.h"

void PrintState(t_state * state)
{
  char x[100];
  if(state->parent_ins)
  {
    I386InstructionPrint(state->parent_ins,x);
    printf("t_ins * parent_ins:%s\n",x);
  }
  if(state->byte_ins)
  {
    I386InstructionPrint(state->byte_ins,x);
    printf("t_ins * byte_ins:%s\n",x);
  }
  printf("t_uint8 offset:d:%d x:0x%x\n",state->offset,state->offset);
  printf("t_bool dontcare:%d\n",state->dontcare);
  printf("t_bool value_relocatable:%d\n",state->value_relocatable);
  printf("t_uint8 value:d:%d x:0x%x\n",state->value,state->value);
}

void AddCodebyteToList(t_codebyte * codebyte,t_codebyte_list * codebyte_list)
{
  t_codebyte_item * codebyte_item= Calloc(1,sizeof(t_codebyte_item));
  codebyte_item->codebyte = codebyte;
  
  if(codebyte_list->first == NULL)
  {
    codebyte_list->first = codebyte_list->last = codebyte_item;
  }
  else{
   codebyte_list->last->next = codebyte_item;
   codebyte_item->prev = codebyte_list->last;
   codebyte_list->last = codebyte_item;
  }
}

void AddStateToList(t_state * state,t_state_list * state_list)
{
  t_state_item * state_item= Calloc(1,sizeof(t_state_item));
  state_item->state = state;
  
  if(state_list->first == NULL)
  {
    state_list->first = state_list->last = state_item;
  }
  else{
   state_list->last->next = state_item;
   state_item->prev = state_list->last;
   state_list->last = state_item;
  }
}

void AddConstraintToList(t_constraint * constraint,t_constraint_list * constraint_list)
{
  t_constraint_item * constraint_item= Calloc(1,sizeof(t_constraint_item));
  constraint_item->constraint = constraint;
  
  if(constraint_list->first == NULL)
  {
    constraint_list->first = constraint_list->last = constraint_item;
  }
  else{
   constraint_list->last->next = constraint_item;
   constraint_item->prev = constraint_list->last;
   constraint_list->last = constraint_item;
  }
}

t_codebyte * CodebyteNew()
{
  t_codebyte * ret = Calloc(1,sizeof(t_codebyte));

  ret->states = Calloc(1,sizeof(t_state_list));
  ret->constraints = Calloc(1,sizeof(t_constraint_list));
  ret->readers = Calloc(1,sizeof(t_ins_list));
  ret->writers = Calloc(1,sizeof(t_ins_list));

  return ret;
}

t_state_list * InsStateList(t_ins * ins, Pvoid_t JudyMapSM)
{
  t_state_list * ret;
  PWord_t JudyValue;
  JLG(JudyValue, JudyMapSM, ins);

  if(JudyValue==NULL){
    VERBOSE(0,("@I",ins));
    FATAL((""));
  }

  ret = ((t_state_list *)(*JudyValue));

  return ret;
}

t_codebyte_list * CfgCodebyteList(t_cfg * cfg, Pvoid_t JudyMapSM)
{
  t_codebyte_list * ret;
  PWord_t JudyValue;
  JLG(JudyValue, JudyMapSM, cfg);

  if(JudyValue==NULL){
    VERBOSE(0,("could not find"));
    FATAL((""));
  }

  ret = ((t_codebyte_list *)(*JudyValue));

  return ret;
}

t_state_list * NewStateList()
{
  t_state_list * state_list=Malloc(sizeof(t_state_list));
  state_list->first=NULL;
  state_list->last=NULL;
  state_list->build=NULL;
  state_list->f1_sl=NULL;
  state_list->f2_sl=NULL;
  return state_list;
}

t_state * NewStateEmpty()
{
  t_state * state;
  state=Malloc(sizeof(t_state));
  state->parent_ins=NULL;
  state->byte_ins=NULL;
  state->offset=0;
  state->dontcare=FALSE;
  state->value_relocatable=FALSE;
  state->value=0x0;
  state->codebyte=NULL;
  return state;
}

