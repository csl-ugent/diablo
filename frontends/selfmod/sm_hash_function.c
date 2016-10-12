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
#include "sm_hash_function.h"
#include "sm_similarity.h"
#include "../../arch/i386/i386_deflowgraph.h"
#include "../../arch/i386/i386_instructions.h"
#include "../diablo_options.h"
 #include <fcntl.h>

/* hashtable * init_table(unsigned int size) {{{ */
hashtable * init_table(unsigned int size)
{
  hashtable * table = (hashtable *) malloc(sizeof(hashtable));
  table->table_size = size;
  table->buckets = (table_entry**) calloc(size,sizeof(table_entry));
  return table;
}
/* }}} */

/* table_entry *  find(hashtable * table, int value) {{{ */
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
/* }}} */

/* table_entry * put(hashtable * table, int value, t_function_node * fun_node) {{{ */
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
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker:*/
