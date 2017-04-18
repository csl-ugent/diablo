/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */
/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

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
#include <diabloanopti386.h>
#include <diablodiversity.h>
#ifdef __cplusplus
}
#endif

t_bool canInlineFunctionHere(t_bbl* callSite, t_function* caller, t_function* callee, t_bool failWhenNrPredsTooSmall);

/*{{{list stuff, what year are we living in anyway?*/
typedef struct {
  t_function * function;
  t_bbl * callsite;
}t_function_callsite_pair;

typedef struct{
  t_uint64 count;
  t_function_callsite_pair ** function_callsite_pairs;
}t_function_callsite_list;

void FunctionCallsiteListAdd(t_function * fun,t_bbl * callsite,t_function_callsite_list * list)	
{
  if (list->function_callsite_pairs) 
    list->function_callsite_pairs = (t_function_callsite_pair**) Realloc(list->function_callsite_pairs,(list->count+1)*sizeof(t_function_callsite_pair *)); 
  else 
    list->function_callsite_pairs = (t_function_callsite_pair**) Malloc(sizeof(t_function_callsite_pair *)); 

  list->function_callsite_pairs[list->count] = (t_function_callsite_pair*) Calloc(1,sizeof(t_function_callsite_pair)); 
  list->function_callsite_pairs[list->count]->function = fun; 
  list->function_callsite_pairs[list->count]->callsite = callsite; 
  list->count++;
}

t_function_callsite_list * FunctionCallsiteListNew()
{
  t_function_callsite_list * ret = (t_function_callsite_list*) Calloc(1,sizeof(t_function_callsite_list));
  return ret;
}

void FunctionCallsiteListFree(t_function_callsite_list * list)
{
  if(list == NULL)
    return;
  for(t_uint64 i = 0; i < list->count; i++)
    Free(list->function_callsite_pairs[i]);
  if (list->function_callsite_pairs)
    Free(list->function_callsite_pairs);
  Free(list);
}
/*}}}*/

static t_function_callsite_list * staticList = NULL;

static t_bool funHasIPs(t_function* fun)
{
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    if (i_bbl == FunctionGetExitBlock(fun)) continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      /* skip the edges to the return block */
      if (CFG_EDGE_TAIL(i_edge) == FunctionGetExitBlock(fun))
	continue;

      /* interprocedural jumps or fallthroughs are not yet supported */
      if (CfgEdgeIsInterproc(i_edge) && !(CFG_EDGE_CAT(i_edge) == ET_CALL) && CFG_EDGE_CAT(i_edge) != ET_SWI)
      {
	return TRUE;
      }
    }
  }
  return FALSE;
}

t_diversity_options DiversityInlineFunctions(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_function * fun;
  t_diversity_options ret;
  
  /*{{{ PHASE 0*/
  if(phase == 0)
  {
    staticList = FunctionCallsiteListNew();

    CFG_FOREACH_FUN(cfg,fun)
    {
      t_bool first = TRUE;
      t_cfg_edge * edge, * tmp;

      if (!FUNCTION_BBL_FIRST(fun) || FUNCTION_IS_HELL(fun) || !I386CheckInlinable(fun) || funHasIPs(fun) || FUNCTION_FACTORED(fun) || FunIsFrozen(fun))
	continue;

      if(StringPatternMatch("*setjmp*",FUNCTION_NAME(fun))||StringPatternMatch("*longjmp*",FUNCTION_NAME(fun)))
	continue;

      BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_FIRST(fun),edge,tmp)
      {
	t_bbl * callsite = CFG_EDGE_HEAD(edge);
	if (CFG_EDGE_CAT(edge) != ET_CALL || BBL_IS_HELL(callsite) || BblIsFrozen(callsite)) continue;
	if(fun==BBL_FUNCTION(callsite))
	    continue;

	if(!first)
	  FunctionCallsiteListAdd(fun, callsite, staticList);
	else first = FALSE;
      }
    }
    
    if(staticList->count!=0)
      ret.done = TRUE;
    else
      ret.done = FALSE;
    ret.flags = TRUE;
    ret.range = staticList->count;
  }
  /*}}}*/
  /*{{{ PHASE 1*/
  else
  {
    t_uint64 count = 0;

    if(staticList == NULL)
      FATAL(("Seems like phase 0 didn't run properly"));
    if(choice->flagList->count != staticList->count)
      FATAL(("counts do not match"));
    
    for(count = 0; count < choice->flagList->count; count++)
    {
      if(choice->flagList->flags[count])
      {
          VERBOSE(0, ("Inlining function %s in a bbl @eiB", FUNCTION_NAME(staticList->function_callsite_pairs[count]->function),
                      staticList->function_callsite_pairs[count]->callsite));

	if(!I386InlineFunAtCallSite(staticList->function_callsite_pairs[count]->function,staticList->function_callsite_pairs[count]->callsite))
	  FATAL(("could not inline as expected!"));
      }
    }

    ret.done = TRUE;
    
    FunctionCallsiteListFree(staticList);
    staticList=NULL;
  }
  /*}}}*/
  return ret;
}/*}}}*/


/* {{{ Iterative framework */

/*
  We keep a duplicate list of functions, before they got transformed. That way we can be sure that when we inline functions
  in different iterations, the results are still the same (because the functions being inlined are not transformed in different
  ways. */
/* TODO, factor out, or use C++ ffs */

static t_hash_table* duplicate_functions;
static t_hash_table* duplicate_functions_set;

void InitInlineTransformations(t_cfg* cfg) {
  t_function* fun;
  t_arraylist* list = ArrayListNew(sizeof(t_function*), 10);
  int i;

  /* Before applying any transformations, we make a mapping: original instruction address -> original function address */
  duplicate_functions = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  duplicate_functions_set = HashTableNew(20033,0, FunHashAddress, FunAddressCmp, FunAddrKeyFree);

  /* We have to do this in two stages: if we don't, we duplicate a function; it turns up in the cfg, and we duplicate it again, etc */
  CFG_FOREACH_FUN(cfg, fun) {
    /* The FALSE as last argument: We want to duplicate functions when nr of predecessors is <= 1, in case it would increase later on */
    if (canInlineFunctionHere(NULL, NULL, fun, FALSE)) {
      ArrayListAdd(list, &fun);
    }
  }

  FOREACH_ARRAY_ITEM(list, i, fun, t_function*) {
    t_address_hash_entry* entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    t_function* new_function;

    VERBOSE(0, ("Duplicating function %p (%s)", fun, FUNCTION_NAME(fun)));

    /*
    We have to duplicate the function in its current CFG, because there can still be recursive call edges to the cloned function,
    and setting this in a cloned CFG would mean that this would take more work...
    */
    new_function = FunctionDuplicate(fun);

    /* Get this cloned function back out of the CFG again */
    if (FUNCTION_FPREV(new_function)) {
      FUNCTION_SET_FNEXT(FUNCTION_FPREV(new_function), FUNCTION_FNEXT(new_function));
    } else {
      CFG_SET_FUNCTION_FIRST(cfg, FUNCTION_FNEXT(new_function));
    }
    if (FUNCTION_FNEXT(new_function)) {
      FUNCTION_SET_FPREV(FUNCTION_FNEXT(new_function), FUNCTION_FPREV(new_function));
    } else {
      CFG_SET_FUNCTION_LAST(cfg, FUNCTION_FPREV(new_function));
    }

    FUNCTION_SET_FNEXT(new_function, NULL);
    FUNCTION_SET_FPREV(new_function, NULL);

    entry->address = (uintptr_t)fun;
    entry->data    = new_function;

    HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(duplicate_functions, entry);

    entry = (t_address_hash_entry*) Malloc(sizeof(t_address_hash_entry));
    entry->address = (uintptr_t) new_function;

    HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(entry), &entry->address);
    HashTableInsert(duplicate_functions_set, entry);
  }

  ArrayListFree(list);
}

static void FreeDuplicateFunction(void* e, void* data) {
  t_address_hash_entry* entry = (t_address_hash_entry*) e;
  FunctionKill((t_function*) entry->data);
}

t_bool isDuplicate(t_function* fun) {
  uintptr_t addr = (uintptr_t) fun;
  t_address_hash_entry* entry = (t_address_hash_entry*) HashTableLookup(duplicate_functions_set, (void*) &addr);

  return t_bool(entry != NULL);
}

void FiniInlineTransformations(t_cfg* cfg) {
  /* First we remove all the basic blocks and instructions from the duplicated functions (they can still have references to the duplicated function */
  CfgRemoveDeadCodeAndDataBlocks(cfg);
  HashTableWalk(duplicate_functions, FreeDuplicateFunction, NULL);
  HashTableFree(duplicate_functions);
  HashTableFree(duplicate_functions_set);
}

/*
  In this case, we get a basic block, which is (if it canBeTransformed) a call site of a non-hell function. We can then inline the
  callee after this call site.

  We need to be sure that a function ALWAYS has at least one edge pointing to it. So if we'd remove the last call-edge to a function,
  we will not transform it.
  We want to distinguish if we cannot inline due to the number of predecessors being <= 1. In that case, it is possible that
  later on, the number of predecessors increases, and we suddenly can inline, thus when we initialize the list of functions
  we can duplicate, this function needs to actually be duplicated...
*/

t_bool canInlineFunctionHere(t_bbl* callSite, t_function* caller, t_function* callee, t_bool failWhenNrPredsTooSmall) {
  t_cfg_edge* edge;
  t_cfg_edge* tmp;
  int nrPreds = 0;

  if (!FUNCTION_BBL_FIRST(callee) || FUNCTION_IS_HELL(callee) || !I386CheckInlinable(callee) || funHasIPs(callee) || FUNCTION_FACTORED(callee)) {
    return FALSE;
  }

  if(StringPatternMatch("*setjmp*", FUNCTION_NAME(callee)) || StringPatternMatch("*longjmp*", FUNCTION_NAME(callee))) {
    return FALSE;
  }

  /* We cannot inline recursive functions! */
  if(callee==caller) {
    VERBOSE(0, ("Skipping recursive function"));
    return FALSE;
  }

  /* We will not remove the last reference to a function! */
  BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_FIRST(callee),edge,tmp)
  {
    /* If the edge is actually a call edge from a duplicated function, don't count it towards the number of predecessors, since this function will be removed later on anyway! */
    if (CFG_EDGE_CAT(edge) == ET_CALL) {
      if ( isDuplicate(BBL_FUNCTION(CFG_EDGE_HEAD(edge))) ) {
        continue;
      }
    }
    nrPreds++;
  }

  if (nrPreds <= 1 && failWhenNrPredsTooSmall) {
    return FALSE;
  }

  return TRUE;
}

t_function* getCallingFunction(t_bbl* bbl) {
  t_cfg_edge* edge;
  t_cfg_edge* tmp;

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,tmp) {
	  if (CFG_EDGE_CAT(edge) == ET_CALL) {
      t_function* fun = BBL_FUNCTION(CFG_EDGE_TAIL(edge));
      uintptr_t addr = (uintptr_t) fun;

      if (!fun) {
        FATAL(("Function was expected to be non-NULL!"));
      }

      return fun;
    }
  }

  /* No function call here, just return nothing */ 
  return NULL;
}

t_function* getDuplicatedCallingFunction(t_function* fun) {
  uintptr_t addr = (uintptr_t) fun;
  t_address_hash_entry* entry = (t_address_hash_entry*) HashTableLookup(duplicate_functions, (void*) &addr);

  if (!entry) {
    VERBOSE(0, ("Looking up duplicate for %s FAILED!", FUNCTION_NAME(fun)));
    return 0;
  }

  return (t_function*) entry->data;
}

t_arraylist* DiversityInlineFunctionCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  t_function* callee = getCallingFunction(bbl);
  if (callee) {
    if (canInlineFunctionHere(bbl, fun, callee, TRUE /* we don't want to remove the last reference to the inlined function */)) {
      return SimpleCanTransform(bbl, NULL, 1);
    }
  }
  return NULL;
}

/* When we duplicated a function, of course we did not let this (or any) old call site point to the duplicate.
     I386InlineFunAtCallSite expects this, though, so fix that up first */
void changeCallSiteToDuplicate(t_bbl* bbl, t_function* callee, t_function* duplicatedcallee) {
  t_cfg_edge* ecall = NULL;
  t_cfg_edge* eret = NULL;
  t_bool returning = FALSE;
  t_bbl* bentry = FUNCTION_BBL_FIRST(callee);
  t_bbl* bexit = FUNCTION_BBL_LAST(callee);
  t_bbl* returnsite = NULL;
  t_cfg* cfg = FUNCTION_CFG(callee);

  /* find call edge */
  BBL_FOREACH_SUCC_EDGE(bbl, ecall) {
    if (CFG_EDGE_CAT(ecall) == ET_CALL && CFG_EDGE_TAIL(ecall) == bentry) {
      break;
    }
  }
  ASSERT(ecall, ("diversity inlining: could not find call edge"));

  /* find possible return edge */
  // TODO: there should only be 1 return edge!
  if(CFG_EDGE_CORR(ecall))
  {
    returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(ecall));
    BBL_FOREACH_PRED_EDGE(returnsite, eret) {
      if (CFG_EDGE_CAT(eret) == ET_RETURN && CFG_EDGE_HEAD(eret) == bexit) {
        break;
      }
    }
    ASSERT(eret,("diversity inlining: could not find return edge"));
  }

  /* Fix edges by killing them, and recreating them (with a single call, which takes care of potentially non-existing return) */
  CfgEdgeKill(ecall);
  if (eret) {
    CfgEdgeKill(eret);
  }
  CfgEdgeCreateCall(cfg, bbl, FUNCTION_BBL_FIRST(duplicatedcallee), returnsite, FUNCTION_BBL_LAST(duplicatedcallee));
}

/*
  When we inlined the duplicate of a function, the recursive calls of the function now point to the duplicate of the function. We patch this up by letting the
  call now point to the original function again. We have to do this because
  1) it's possible that we want to inline the function again later on. But that function is not on the list of 'functions that have a duplicate'!,
  2) afterwards, we want to delete the duplicated functions again, since it'd have quite an _extra_ space overhead...
  We have to do it right after inlining the function; we cannot do it right after duplicating the function because then this function would be a reference
  to the original function. It's possible that then because of this, we remove the last reference to the function from a function that is not a duplicate. If at
  the end we then remove this function, the original function would have no more references!
*/
void fixRecursiveCalls(t_function* fun, t_function* original, t_function* duplicate) {
  t_bbl* bbl;
  t_bbl* duplicate_entry = FUNCTION_BBL_FIRST(duplicate);
  t_bbl* orig_entry = FUNCTION_BBL_FIRST(original);
  t_bbl* duplicate_exit = FUNCTION_BBL_LAST(duplicate);
  t_bbl* orig_exit = FUNCTION_BBL_LAST(original);
  t_cfg* cfg = FUNCTION_CFG(original);

  FUNCTION_FOREACH_BBL (fun, bbl) {
    t_cfg_edge* edge;
    BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      if (CFG_EDGE_CAT (edge) == ET_CALL && CFG_EDGE_TAIL(edge) == duplicate_entry) {
        t_bbl* returnsite = NULL;
        t_cfg_edge* eret;
      
        if (CFG_EDGE_CORR (edge)) {
          returnsite = CFG_EDGE_TAIL (CFG_EDGE_CORR (edge));
          CfgEdgeKill(CFG_EDGE_CORR(edge));
        }
        CfgEdgeKill(edge);

        CfgEdgeCreateCall(cfg, bbl, orig_entry, returnsite, orig_exit);

        /* There will be only one such edge, so return now, that we don't loop infinitely */
        break;
      }
    }
	}
}

t_diversity_options DiversityInlineFunctionDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  t_function* callee = getCallingFunction(bbl);
  t_function* duplicated = getDuplicatedCallingFunction(callee);

  VERBOSE(0, ("Inlining function %p (%s) in a bbl of function %p (%s)", callee, FUNCTION_NAME(callee), fun, FUNCTION_NAME(fun)));

  //FunctionUnmarkAllBbls(callee);
  //FunctionPropagateConstantsAfterIterativeSolution(callee,CONTEXT_SENSITIVE);

  changeCallSiteToDuplicate(bbl, callee, duplicated);
    
  /* Now we can inline the duplicated function at the call site */
  if(!I386InlineFunAtCallSite(duplicated, bbl)) {
    FATAL(("could not inline as expected!"));
  }

  fixRecursiveCalls(fun, callee, duplicated);

  /*FunctionUnmarkAllBbls(fun);
  FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);*/

  return diversity_options_null;
}
/* }}} */

