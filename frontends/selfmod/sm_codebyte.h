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
#include <diablosupport.h>
#include <diabloobject.h>
#include <Judy.h>
#include "../../kernel/diablo_ins.h"

#ifndef SM_CODEBYTE
#define SM_CODEBYTE
/*Extra fields are stored in a judy map, instructions refer to a t_state_list, cfg to a t_codebyte_list*/
Pvoid_t JudyMapSM;
typedef struct _t_codebyte_list t_codebyte_list;
t_codebyte_list * ugly_global_codebyte_list;

typedef struct _t_state t_state;
typedef struct _t_state_list t_state_list;
typedef struct _t_state_item t_state_item;

typedef struct _t_codebyte t_codebyte;
typedef struct _t_codebyte_item t_codebyte_item;

typedef struct _t_constraint t_constraint;
typedef struct _t_constraint_list t_constraint_list;
typedef struct _t_constraint_item t_constraint_item;

typedef struct _t_ins_list t_ins_list;
typedef struct _t_ins_list_item t_ins_list_item;
  
typedef struct _t_build_out_of t_build_out_of;

struct _t_build_out_of
{
  t_int32 offset;
  t_state_list * list;
  t_build_out_of * next;
};

struct _t_state
{
  t_ins * parent_ins;
  t_ins * byte_ins;
  t_uint8 offset;
  t_bool dontcare;
  t_bool value_relocatable;
  t_uint8 value;
  t_codebyte * codebyte;
};

struct _t_state_item
{
  t_state * state;
  t_state_item * next;
  t_state_item * prev;
};

struct _t_state_list
{
  t_state_item * first;
  t_state_item * last;
  t_build_out_of * build;
  t_state_list * f1_sl;
  t_state_list * f2_sl;
};

struct _t_codebyte
{
  t_state_list * states;

  t_ins_list * readers;
  t_ins_list * writers;
  
  t_constraint_list * constraints;
};

struct _t_codebyte_item
{
  t_codebyte * codebyte;
  t_codebyte_item * next;
  t_codebyte_item * prev;
};

struct _t_codebyte_list
{
  t_codebyte_item * first;
  t_codebyte_item * last;
};

struct _t_ins_list_item
{
  t_ins * ins;
  t_ins_list_item * next;
  t_ins_list_item * prev;
};

struct _t_ins_list
{
  t_ins_list_item * first;
  t_ins_list_item * last;
};

struct _t_constraint
{
  t_codebyte * base;
  t_int32 offset;  
};

struct _t_constraint_item
{
  t_constraint * constraint;
  t_constraint_item * next;
  t_constraint_item * prev;
};

struct _t_constraint_list
{
  t_constraint_item * first;
  t_constraint_item * last;
};

void PrintState(t_state * state);
void AddCodebyteToList(t_codebyte * codebyte,t_codebyte_list * codebyte_list);
void AddStateToList(t_state * state,t_state_list * state_list);
void AddConstraintToList(t_constraint * constraint,t_constraint_list * constraint_list);
t_codebyte * CodebyteNew();
t_state * NewStateEmpty();
t_state_list * NewStateList();

#define INS_STATE_LIST(ins) (InsStateList(ins,JudyMapSM))
t_state_list * InsStateList(t_ins * ins, Pvoid_t JudyMapSM);

#define CFG_CODEBYTE_LIST(cfg) (ugly_global_codebyte_list)
//t_codebyte_list * CfgCodebyteListList(t_cfg * cfg, Pvoid_t JudyMapSM);

#define INS_STATE_ITEM_FIRST(ins) (((t_state_list *)INS_STATE_LIST(ins))->first)
#define INS_STATE_ITEM_LAST(ins) (((t_state_list *)INS_STATE_LIST(ins))->last)
#define INS_CODEBYTE_FIRST(ins) (INS_STATE_ITEM_FIRST(ins)->state->codebyte)
#define INS_CODEBYTE_LAST(ins) (INS_STATE_ITEM_LAST(ins)->state->codebyte)
#define INS_CAN_POINT_TO_TEMPLATE(ins) (ins->can_point_to_template)

#define INS_FOREACH_STATE_ITEM(ins,state_item) for (state_item = INS_STATE_ITEM_FIRST(ins);state_item!=NULL;state_item = state_item->next)

#define INS_FOREACH_STATE(ins,state,state_item) for (state_item = INS_STATE_ITEM_FIRST(ins), state = (state_item)?state_item->state:NULL;state_item!=NULL;state_item = state_item->next, state = (state_item)?state_item->state:NULL)

#define INS_FOREACH_CODEBYTE(ins,codebyte,state_item) for (state_item = INS_STATE_ITEM_FIRST(ins), codebyte = (state_item)?state_item->state->codebyte:NULL; state_item!=NULL; state_item = state_item->next, codebyte = (state_item)?state_item->state->codebyte:NULL)

#define STATE_ITEM_NEXT(state_item) (state_item->next)
#define STATE_ITEM_CODEBYTE(state_item) (state_item->state->codebyte)

#define CODEBYTELIST_FOREACH_CODEBYTEITEM(codebyte_list, codebyte_item) for(codebyte_item = codebyte_list->first;codebyte_item!=NULL;codebyte_item = codebyte_item->next)
#define CODEBYTELIST_FOREACH_CODEBYTE(codebyte_list, codebyte, codebyte_item) for(codebyte_item = codebyte_list->first, codebyte = (codebyte_item)?codebyte_item->codebyte:NULL;codebyte_item!=NULL;codebyte_item = codebyte_item->next, codebyte = (codebyte_item)?codebyte_item->codebyte:NULL)

#define CODEBYTE_FOREACH_CONSTRAINT(codebyte, constraint, constraint_item) for(constraint_item = codebyte->constraints->first, constraint=(constraint_item)?constraint_item->constraint:NULL; constraint_item !=NULL; constraint_item = constraint_item->next, constraint = (constraint_item)?constraint_item->constraint:NULL)

#define CODEBYTE_INITIALSTATE(codebyte) (codebyte->states->first->state)
#define CODEBYTE_INS(codebyte) (codebyte->ins)
#define STATE_OFFSET(state) (state->offset)
#define STATE_INS(state) (state->ins)

#endif
