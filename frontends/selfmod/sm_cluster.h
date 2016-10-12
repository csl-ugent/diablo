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
#include "../../kernel/diablo_code_layout.h"
#include "../../kernel/diablo_ins.h"
#include "sm_codebyte.h"

#ifndef SM_CLUSTER
#define SM_CLUSTER

typedef struct _t_state_list_list t_state_list_list;
typedef struct _t_state_list_item t_state_list_item;

/* Used for templates ... */
struct _t_state_list_list
{
  t_state_list_item * first;
  t_state_list_item * last;
};

struct _t_state_list_item
{
  t_state_list * state_list;
  t_state_list_item * next;
  t_state_list_item * prev;
};

typedef struct _t_bbl_chain_item t_bbl_chain_item;
typedef struct _t_bbl_chain_list t_bbl_chain_list;

struct _t_bbl_chain_item
{
  t_bbl * bbl;
  t_int32 nr_of_bytes;
  t_bbl_chain_item * next;
  t_bbl_chain_item * prev;
};

struct _t_bbl_chain_list
{
  t_bbl_chain_item * first;
  t_bbl_chain_item * last;
};

void SmClusterFunctions(t_object * obj, t_bbl * editor);
t_int32 GetNrOfBytesStateList (t_state_list * state_list);

typedef struct _t_made t_made;

struct _t_made
{
  t_state_list_list * from;
  t_state_list * to;
  t_bool inter;
  t_made * f1;
  t_made * f2;
};

typedef struct _t_function_list t_function_list;
typedef struct _t_function_item t_function_item;

struct _t_function_list
{
  t_function_item * first;
  t_function_item * last;
};

struct _t_function_item
{
  t_function * fun;
  t_function_item * next;
  t_function_item * prev;
};

#endif

/* vim: set shiftwidth=2 foldmethod=marker:*/
