/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

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

void RegisterBblFactoring(t_bbl * orig_bbl, t_bbl * new_bbl)/*{{{*/
{
  t_ins * orig_ins, * new_ins;
  if(BBL_NINS(orig_bbl)!= BBL_NINS(new_bbl))
      FATAL((""));

  for(orig_ins=BBL_INS_FIRST(orig_bbl),new_ins = BBL_INS_FIRST(new_bbl); new_ins !=NULL && orig_ins!=NULL; new_ins = INS_INEXT(new_ins), orig_ins=INS_INEXT(orig_ins))
  {
    AddressListAddList(INS_ADDRESSLIST(new_ins), INS_ADDRESSLIST(orig_ins));
  }
  BBL_SET_FACTORED(new_bbl,TRUE);
  BBL_SET_FACTORED(orig_bbl,TRUE);
}/*}}}*/

static t_bool InsIsControlTransferInstructionWithImmediateTarget(t_ins * ins)/*{{{ */
{
  switch (I386_INS_OPCODE(T_I386_INS(ins)))
  {
    case I386_JMP:
    case I386_JMPF:
    case I386_Jcc:
    case I386_JECXZ:
    case I386_LOOP:
    case I386_LOOPZ:
    case I386_LOOPNZ:
    case I386_CALL:
    case I386_CALLF:
      if(I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(ins)))==i386_optype_imm)
	return TRUE;
    default:
      return FALSE;
  }
}
/* }}} */
void RegisterBblFactoringAllButFinalJump(t_bbl * orig_bbl, t_bbl * new_bbl)/*{{{*/
{
  t_ins * orig_ins, * new_ins;

  for(orig_ins=BBL_INS_FIRST(orig_bbl),new_ins = BBL_INS_FIRST(new_bbl); new_ins && orig_ins; new_ins = INS_INEXT(new_ins), orig_ins=INS_INEXT(orig_ins))
  {
    if(!InsIsControlTransferInstructionWithImmediateTarget(orig_ins))
      AddressListAddList(INS_ADDRESSLIST(new_ins), INS_ADDRESSLIST(orig_ins));
  }
  BBL_SET_FACTORED(new_bbl,TRUE);
  BBL_SET_FACTORED(orig_bbl,TRUE);
}/*}}}*/
void RegisterFunFactoring(t_function * fun)/*{{{*/
{
  FUNCTION_SET_FACTORED(fun,TRUE);
}/*}}}*/

/*{{{ Factor Functions */
/*{{{ List stuff, what year are we living in anyway?*/
typedef struct _t_function_function_item t_function_function_item;
typedef struct _t_function_function_list t_function_function_list;

struct _t_function_function_item
{
  t_function * function1;
  t_function * function2;
  t_function_function_item * next;
  t_function_function_item * prev;
};

struct _t_function_function_list
{
  t_function_function_item * first;
  t_function_function_item * last;
  t_uint64 count;
};

void FunctionFunctionListAdd(t_function * fun1, t_function *  fun2, t_function_function_list * list)
{
  t_function_function_item * item= (t_function_function_item *) Calloc(1,sizeof(t_function_function_item));
  item->function1 = fun1;
  item->function2 = fun2;
  
  if(list->first == NULL)
  {
    list->first = list->last = item;
  }
  else{
   list->last->next = item;
   item->prev = list->last;
   list->last = item;
  }
  
  if(list->count == ((t_uint64) -1L))
    FATAL(("about to overflow"));
  list->count++;
}

t_function_function_list * FunctionFunctionListNew()
{
  t_function_function_list * ret = (t_function_function_list *) Calloc(1,sizeof(t_function_function_list));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void FunctionFunctionListFree(t_function_function_list * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_function_function_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}
/*}}}*/

static t_function_function_list * functionList = NULL;

t_diversity_options DiversityFactorFunctions(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_function * fun1, * fun2;
  t_diversity_options ret;
  
  /*PHASE 0 {{{*/
  if(phase == 0)
  {
    functionList = FunctionFunctionListNew();

    BblMarkInit2();

    CFG_FOREACH_FUN(cfg,fun1)
      if (FUNCTION_BBL_FIRST(fun1) && !BblIsMarked2(FUNCTION_BBL_FIRST(fun1)))
	for(fun2=FUNCTION_FNEXT(fun1);fun2!=NULL;fun2=FUNCTION_FNEXT(fun2))
	  if (fun1!=fun2 
	      && FUNCTION_BBL_FIRST(fun2)
	      && !BblIsMarked2(FUNCTION_BBL_FIRST(fun2))) 
	    if (CompareTwoFunctions(fun1,fun2,FALSE))
	      FunctionFunctionListAdd(fun1,fun2,functionList);

    if(functionList->count!=0)
      ret.done = TRUE;
    else
      ret.done = FALSE;
    ret.flags = TRUE;
    ret.range = functionList->count;
  }
  /*}}}*/
  /*{{{ PHASE 1*/
  else
  {
    t_function_function_item * factor_item;
    t_uint64 count = 0;

    if(functionList == NULL)
      FATAL(("Seems like phase 0 didn't run properly"));
    factor_item = functionList->first;
    for(count = 0; count < choice->flagList->count; count++)
    {
      if(choice->flagList->flags[count])
      {
	MoveDirectCallEdgesFromTo(factor_item->function2,factor_item->function1,FALSE);
	CompareTwoFunctions(factor_item->function2,factor_item->function1,TRUE);
	FUNCTION_SET_FACTORED(factor_item->function1,TRUE);
	FUNCTION_SET_FACTORED(factor_item->function2,TRUE);
      }
      factor_item = factor_item->next;
    }
    
    ret.done = TRUE;
    
    FunctionFunctionListFree(functionList);
    functionList=NULL;
  }
  /*}}}*/
  return ret;
}/*}}}*/
/*}}}*/

/* Factor Function Epilogues {{{*/
#ifdef __cplusplus
extern "C" {
#endif

void ExternDoEpilogueFactoring(t_equiv_bbl_holder *equivs);

#ifdef __cplusplus
}
#endif

/*{{{ Hash and list stuff, what year are we living in anyway?*/
/* custom hash table structure for bbl comparison - much easier to set up than the generic hash table */
#define TABLE_SIZE	1024
typedef struct _bbl_hash_entry {
  struct _bbl_hash_entry *next;
  t_bbl *bbl;
} bbl_hash_entry;

typedef struct {
  t_uint64 count;
  t_equiv_bbl_holder ** holders;
}t_holder_list;

#define AddToHolder(holder,block)	\
	do { \
	  if (holder->bbl) holder->bbl = (t_bbl**) Realloc(holder->bbl,(holder->nbbls+1)*sizeof(t_bbl *)); \
	  else holder->bbl = (t_bbl**)  Malloc(sizeof(t_bbl *)); \
	  holder->bbl[holder->nbbls++] = block; \
	} while (0)

#define HolderListAdd(holder,list)	\
	do { \
	  if (list->holders) list->holders = (t_equiv_bbl_holder**) Realloc(list->holders,(list->count+1)*sizeof(t_equiv_bbl_holder *)); \
	  else list->holders = (t_equiv_bbl_holder**) Malloc(sizeof(t_equiv_bbl_holder *)); \
	  list->holders[list->count++] = holder; \
	} while (0)

static void HolderListFree(t_holder_list * list)
{
  for(t_uint64 i = 0; i < list->count; i++)
  {
    Free(list->holders[i]->bbl);
    Free(list->holders[i]);
  }
  
  Free(list->holders);
  Free(list);
}
/*}}}*/

t_holder_list * epilogueHolderList = NULL;
t_diversity_options DiversityFactorEpilogues(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  
  /*{{{ PHASE 0*/
  if(phase == 0)
  {
    t_function *fun;
    int i;

	bbl_hash_entry *table[TABLE_SIZE];
    bbl_hash_entry *he, *he2;
    
    BblFactorInit(cfg);
    epilogueHolderList = (t_holder_list*) Calloc(1,sizeof(t_holder_list));

    if (!CFG_DESCRIPTION(cfg)->InsMakeDirectBranch) FATAL((""));
  
    /* {{{ build hash table */
    memset(&(table[0]),0,TABLE_SIZE*sizeof(bbl_hash_entry *));
    CFG_FOREACH_FUN(cfg,fun)
    {
      t_bbl *exitblock = FunctionGetExitBlock(fun);
      t_cfg_edge *edge;
      t_bbl *bbl;
      if (!exitblock) continue;

      /* only unconditional exit blocks */
      BBL_FOREACH_PRED_EDGE(exitblock,edge)
	if (CFG_EDGE_CAT(edge) == ET_JUMP &&
	    !CFG_EDGE_SUCC_NEXT(edge) && !CFG_EDGE_SUCC_PREV(edge) &&
	    BBL_NINS(CFG_EDGE_HEAD(edge)) > 1)
	{
	  /* insert block in hash table */
	  t_uint32 index;
	  bbl_hash_entry *new_;

	  new_ = (bbl_hash_entry *) Malloc(sizeof(bbl_hash_entry));
	
	  bbl = CFG_EDGE_HEAD(edge);
	  index = CFG_BBL_FINGERPRINT(cfg)(bbl) % TABLE_SIZE;
	  new_->bbl = bbl;
	  new_->next = table[index];
	  table[index] = new_;
	}
    } /* }}} */

    /* {{{ build equivalence relations and perform factoring */
    BblMarkInit();
    for (i = 0; i < TABLE_SIZE; i++)
    {
      for (he = table[i]; he; he = he->next)
      {
	t_equiv_bbl_holder * holder = (t_equiv_bbl_holder *) Calloc(1,sizeof(t_equiv_bbl_holder));
	if (BblIsMarked(he->bbl)) continue;
	BblMark(he->bbl);
	AddToHolder(holder,he->bbl);

	for (he2 = he->next; he2; he2 = he2->next)
	{
	  if (BblIsMarked(he2->bbl)) continue;
	  if (CompareTwoBlocks(he->bbl,he2->bbl))
	  {
	    AddToHolder(holder,he2->bbl);
	    BblMark(he2->bbl);
	  }
	}

	if (holder->nbbls > 1)
	{
	  HolderListAdd(holder,epilogueHolderList);
	}
	else
	{
	  Free(holder->bbl);
	  Free(holder);
	}
      }
    } /* }}} */

    /* {{{ tear down the hash table */
    for (i=0; i<TABLE_SIZE; i++)
    {
      while (table[i])
      {
	bbl_hash_entry *tmp = table[i];
	table[i] = tmp->next;
	Free(tmp);
      }
      table[i] = NULL;
    } /* }}} */

    if(epilogueHolderList->count!=0)
      ret.done = TRUE;
    else
      ret.done = FALSE;
    ret.flags = TRUE;
    ret.range = epilogueHolderList->count;
  }
  /*}}}*/
  /*{{{PHASE 1*/
  else
  {
    t_uint64 count = 0;

    if(epilogueHolderList == NULL)
      FATAL(("Seems like phase 0 didn't run properly"));
    for(count = 0; count < choice->flagList->count; count++)
      if(choice->flagList->flags[count])
	   ExternDoEpilogueFactoring(epilogueHolderList->holders[count]);
    ret.done = TRUE;
    
    HolderListFree(epilogueHolderList);
    epilogueHolderList=NULL;
    BblFactorFini(cfg);
    CfgPatchToSingleEntryFunctions (cfg);
  }
  /*}}}*/
  
  return ret;
}/*}}}*/
/*}}}*/

/*{{{ Factor Bbls*/
t_holder_list * bblHolderList = NULL;
t_diversity_options DiversityFactorBbls(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase) /*{{{*/
{
  t_diversity_options ret;
  
  /*{{{ PHASE 0*/
  if(phase == 0)
  {
    int i,nins;
    bbl_hash_entry *table[TABLE_SIZE];
    bbl_hash_entry *he, *he2;
    t_bbl *bbl;
    int factorcount = 0;
    static int totalcount = 0;

    BblFactorInit(cfg);
    bblHolderList = (t_holder_list*) Calloc(1,sizeof(t_holder_list));
    
    /* make sure we have accurate liveness information */
    CfgComputeSavedChangedRegisters(cfg);
    CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

    /* {{{ build hash table */
    for (i = 0; i < TABLE_SIZE; i++)
      table[i] = NULL;

    CFG_FOREACH_BBL(cfg,bbl)
    {
      t_uint32 index;
      bbl_hash_entry *new_;

      if (IS_DATABBL(bbl) || !CFG_BBL_CAN_BE_FACTORED(cfg)(bbl)) continue;

      /* split off control flow instructions */
      if (CFG_DESCRIPTION(cfg)->InsIsControlflow(BBL_INS_LAST(bbl)))
	BblSplitBlock(bbl,BBL_INS_LAST(bbl),TRUE);

      index = CFG_BBL_FINGERPRINT(cfg)(bbl) % TABLE_SIZE;
      new_ = (bbl_hash_entry*) Malloc(sizeof(bbl_hash_entry));
      new_->next = table[index];
      new_->bbl = bbl;
      table[index] = new_;
    } /* }}} */

    /* {{{ build equivalence relations and perform factoring */
    BblMarkInit();
    for (i = 0; i < TABLE_SIZE; i++)
    {
      for (he = table[i]; he; he = he->next)
      {
	t_equiv_bbl_holder * holder = (t_equiv_bbl_holder *) Calloc(1,sizeof(t_equiv_bbl_holder));
	if (BblIsMarked(he->bbl)) continue;
	BblMark(he->bbl);
	AddToHolder(holder,he->bbl);

	for (he2 = he->next; he2; he2 = he2->next)
	{
	  if (BblIsMarked(he2->bbl)) continue;
	  if (CompareTwoBlocks(he->bbl,he2->bbl))
	  {
	    AddToHolder(holder,he2->bbl);
	    BblMark(he2->bbl);
	  }
	}

	if (holder->nbbls > 1)
	{
	  HolderListAdd(holder,bblHolderList);
	}
	else
	{
	  Free(holder->bbl);
	  Free(holder);
	}
      }
    } /* }}} */

    /* {{{ tear down the hash table */
    for (i=0; i<TABLE_SIZE; i++)
    {
      while (table[i])
      {
	bbl_hash_entry *tmp = table[i];
	table[i] = tmp->next;
	Free(tmp);
      }
      table[i] = NULL;
    } /* }}} */

    VERBOSE(0,("approx %d instructions gained (total factorings: %d)",factorcount,totalcount));
    
    if(bblHolderList->count!=0)
      ret.done = TRUE;
    else
      ret.done = FALSE;
    ret.flags = TRUE;
    ret.range = bblHolderList->count;
  }
  /*}}}*/
  /*{{{ PHASE 1*/
  else
  {
    t_uint64 count = 0;

    if(bblHolderList == NULL)
      FATAL(("Seems like phase 0 didn't run properly"));
    for(count = 0; count < choice->flagList->count; count++)
      if(choice->flagList->flags[count])
      {
	if (!I386BblFactorConditional(bblHolderList->holders[count],bblHolderList->holders[count]->bbl[0],FALSE))
	{
	  FATAL(("did not work, although we expected it to"));
	}
      }
    ret.done = TRUE;
    
    HolderListFree(bblHolderList);
    bblHolderList=NULL;
    BblFactorFini(cfg);
    MergeBbls(cfg);
  }
  /*}}}*/
  return ret;
}/*}}}*/
/*}}}*/
