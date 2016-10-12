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
#include "sm_cluster.h"

#ifndef SM_SIMILARITY
#define SM_SIMILARITY

typedef struct _t_similarity_list t_similarity_list;
typedef struct _t_similarity_item t_similarity_item;
typedef struct _t_similarity t_similarity;
typedef struct _t_full_sim_list t_full_sim_list;
typedef struct _t_full_sim_item t_full_sim_item;

struct _t_full_sim_list{
  t_full_sim_item * first;
  t_full_sim_item * last;
};

struct _t_full_sim_item{
  t_similarity_list * similarity_list;
  t_full_sim_item * next;
  t_full_sim_item * prev;
};
  
struct _t_similarity_list{
  t_similarity_item * first;
  t_similarity_item * last;
};

struct _t_similarity_item{
  t_similarity * similarity;
  t_similarity_item * next;
  t_similarity_item * prev;
};
  
struct _t_similarity{
  t_state_list * f1_sl;
  t_state_list * f2_sl;
  t_int32 offset;
  t_bool bound;
  t_int32 nr_of_edits;
  t_int32 nr_equal;
};

void AddFullSimToList(t_similarity_list * sim,t_full_sim_list * list);
void RemoveSimularityFromList(t_similarity * similarity,t_similarity_list * similarity_list);
void AddSimularityToList(t_similarity * similarity,t_similarity_list * similarity_list);
t_similarity_list * NrOfEdits(t_state_list * help_state_fun1,t_state_list * help_state_fun2);
t_similarity_list * SortList(t_similarity_list * connected_list);
void Impossible(t_similarity_list * similarity_list_hulp);
void SetTheImpossibility(t_state_list * f1_list, t_state_list * f2_list, t_full_sim_list * sim_list);
t_similarity_list * NewSimilarityList();
t_full_sim_list * NewFullSimList();
void SetTheImpossibilityLists(t_state_list_list * f1_list,t_state_list_list * f2_list, t_full_sim_list * sim_list);

#endif

/* vim: set shiftwidth=2 foldmethod=marker:*/
