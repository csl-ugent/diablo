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

#ifndef SM_HASH_FUNCTION
#define SM_HASH_FUNCTION


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

struct _t_function_edge
{
  t_edge edge;
  t_int32 last;
  t_uint64 switched;
  t_bool path_exists;
  t_tristate cl_edge;
};

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
#endif

/* vim: set shiftwidth=2 foldmethod=marker:*/
