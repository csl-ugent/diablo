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

/*{{{ List stuff, what year are we living in anyway?*/
typedef struct _t_bbl_edge_item t_bbl_edge_item;
typedef struct _t_bblEdgeList t_bblEdgeList;

struct _t_bbl_edge_item
{
  t_bbl * bbl;
  t_cfg_edge * edge;
  t_bbl_edge_item * next;
  t_bbl_edge_item * prev;
};

struct _t_bblEdgeList
{
  t_bbl_edge_item * first;
  t_bbl_edge_item * last;
  t_uint64 count;
};

void BblEdgeListAdd(t_bbl * bbl, t_cfg_edge * edge, t_bblEdgeList * list)
{
  t_bbl_edge_item * item= (t_bbl_edge_item *) Calloc(1,sizeof(t_bbl_edge_item));
  item->bbl = bbl;
  item->edge = edge;
  
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

t_bblEdgeList * BblEdgeListNew()
{
  t_bblEdgeList * ret = (t_bblEdgeList *) Calloc(1,sizeof(t_bblEdgeList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void BblEdgeListFree(t_bblEdgeList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_bbl_edge_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}
/*}}}*/

static t_bool CanUnfoldBbl(t_bbl * bbl)/*{{{*/
{
  t_cfg_edge * edge;
  if(BBL_NINS(bbl)==0)
    return FALSE;
  if(BBL_FACTORED(bbl))
    return FALSE;
  if(BBL_IS_HELL(bbl))
    return FALSE;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if(CFG_EDGE_CAT(edge) == ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH)
      return FALSE;
    /*
      A Branch Function redirects the control to a location in the binary based on the return address. When we duplicate
      such a basic block, one of those computations will still be correct, but for the other one, since we cannot have
      2 fall-through paths to the same basic block, this doesn't work. It probably could get worked around so that it still
      works (TODO this), but for now this is easier and works too.
    */
    if(strcmp("BranchFunction", FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_TAIL(edge)))) == 0)
	    return FALSE;
  }
  return TRUE;
}/*}}}*/

static t_bool CanUnfoldBblByEdge(t_bbl * bbl, t_cfg_edge * edge)/*{{{*/
{
  if(BBL_IS_HELL(CFG_EDGE_HEAD(edge)) ||  CFG_EDGE_CAT(edge)==ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH || CFG_EDGE_CAT(edge) == ET_COMPENSATING)
    return FALSE;
	  
  if(!CanUnfoldBbl(bbl))
    return FALSE;
  
  return TRUE;
}/*}}}*/

static t_bbl * UnfoldBblByEdge(t_bbl * bblOrig, t_cfg_edge * edgeToMove)/*{{{*/
{
  t_bbl * bblCopy;
  t_cfg_edge * edge;
  t_cfg * cfg = BBL_CFG(bblOrig);

  if(!CanUnfoldBblByEdge(bblOrig, edgeToMove))
    FATAL(("@ieB, Not supported!",bblOrig));  
  
  bblCopy = BblDup(bblOrig);

  /* Ensure that if we split a hot bbl, the split off bbls is still considered hot! */
  BBL_SET_EXEC_COUNT(bblCopy, BBL_EXEC_COUNT(bblOrig));

  //make sure that the old addresses are maintained
  /*
  {
    t_ins * i_ins, * j_ins;
    for(i_ins=BBL_INS_FIRST(bblCopy),j_ins=BBL_INS_FIRST(bblOrig); i_ins!=NULL && j_ins != NULL; i_ins=INS_INEXT(i_ins), j_ins = INS_INEXT(j_ins))
    {
      INS_SET_OLD_ADDRESS(i_ins, INS_OLD_ADDRESS(j_ins));
    }
    if(i_ins != NULL || j_ins !=NULL)
      FATAL((""));
  }
  */
  BblInsertInFunction(bblCopy, BBL_FUNCTION(bblOrig));
  
  if(!CFG_EDGE_CORR(edgeToMove))
  {
    CfgUnlinkEdge(cfg, edgeToMove);
    CfgInitEdge(cfg, edgeToMove, CFG_EDGE_HEAD(edgeToMove), bblCopy,CFG_EDGE_CAT(edgeToMove));
    //      CfgEdgeNew(cfg,CFG_EDGE_HEAD(edgeToMove),bblCopy,CFG_EDGE_CAT(edgeToMove));
    //      CfgEdgeKill(edgeToMove);
  }
  else 
  {
    t_bbl * head_one;
    t_bbl * tail_one;
    t_bbl * head_corr;
    t_bbl * tail_corr;
    t_uint32 cat_one;
    t_uint32 cat_corr;
    if (CFG_EDGE_CAT(edgeToMove) == ET_CALL)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);
      head_corr = CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove));
      tail_corr = CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else if (CFG_EDGE_CAT(edgeToMove) == ET_RETURN)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);

      head_corr = CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove));
      tail_corr = CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else if (CFG_EDGE_CAT(edgeToMove) == ET_IPJUMP || CFG_EDGE_CAT(edgeToMove) == ET_IPFALLTHRU)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);

      head_corr = T_BBL(CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove)));
      tail_corr = T_BBL(CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove)));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else
    {
      VERBOSE(0,("@E @ieB",edgeToMove,bblOrig)); 	    
      VERBOSE(0,("%s",FUNCTION_NAME(BBL_FUNCTION(bblOrig))));
      VERBOSE(0,("%d",BBL_IS_HELL(bblOrig)));
      FATAL((""));
    }
    CfgUnlinkEdge(cfg, edgeToMove);
    CfgUnlinkEdge(cfg, CFG_EDGE_CORR(edgeToMove));
    CfgInitEdge(cfg, edgeToMove, head_one, tail_one, cat_one);
    CfgInitEdge(cfg, CFG_EDGE_CORR(edgeToMove), head_corr, tail_corr, cat_corr);
  }
  
  BBL_FOREACH_SUCC_EDGE(bblOrig,edge)
  {
    if(CFG_EDGE_CAT(edge) == ET_FALLTHROUGH || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
    {
      t_bbl * new_bbl = BblNew(cfg);

      t_ins * jump_ins;
      jump_ins = InsNewForBbl(new_bbl);
      I386InstructionMakeJump(T_I386_INS(jump_ins));
      InsAppendToBbl(jump_ins,new_bbl);

      if(CFG_EDGE_CAT(edge) == ET_FALLTHROUGH)
	CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
      else if(CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      {
	t_cfg_edge * new_edge1 = CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_IPJUMP);
	if(CFG_EDGE_CORR(edge))
	{
	  t_cfg_edge * new_edge2 =  CfgEdgeNew(cfg,CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)),CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),CFG_EDGE_CAT(CFG_EDGE_CORR(edge)));
	  CFG_EDGE_SET_CORR(new_edge1,new_edge2);
	  CFG_EDGE_SET_CORR(new_edge2,new_edge1);
	}
      }

      CfgEdgeNew(cfg,bblCopy,new_bbl,ET_FALLTHROUGH);
      if(BBL_FUNCTION(bblOrig))
	BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
    }
    else if (CFG_EDGE_CAT(edge) == ET_CALL)
    {
      t_bbl * new_bbl;
      t_ins * jump_ins;
      
      if(CFG_EDGE_CORR(edge))
      {
	new_bbl = BblNew(cfg);
	jump_ins = InsNewForBbl(new_bbl);
	I386InstructionMakeJump(T_I386_INS(jump_ins));
	InsAppendToBbl(jump_ins,new_bbl);

	CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),ET_JUMP);
	CfgEdgeCreateCall(cfg,bblCopy,CFG_EDGE_TAIL(edge),new_bbl,FunctionGetExitBlock(BBL_FUNCTION(bblOrig)));

	if(BBL_FUNCTION(bblOrig))
	  BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
      }
      else
      {
	CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),ET_CALL);
      }
    }
    else if (CFG_EDGE_CAT(edge) == ET_SWI)
    {
      t_bbl * new_bbl;
      t_ins * jump_ins;
      
      if(CFG_EDGE_CORR(edge))
      {
	new_bbl = BblNew(cfg);
	jump_ins = InsNewForBbl(new_bbl);
	I386InstructionMakeJump(T_I386_INS(jump_ins));
	InsAppendToBbl(jump_ins,new_bbl);

	CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),ET_JUMP);

	CfgEdgeCreateSwi(cfg,bblCopy,new_bbl);

	if(BBL_FUNCTION(bblOrig))
	  BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
      }
      else
      {
	CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),ET_SWI);
      }
    }
    else if (CFG_EDGE_CORR(edge))
    {
		t_cfg_edge * new_edge1, * new_edge2;
		if(CFG_EDGE_CAT(CFG_EDGE_CORR(edge))==ET_IPFALLTHRU)
			continue;
		new_edge1 = CfgEdgeNew(cfg, bblCopy,CFG_EDGE_TAIL(edge),CFG_EDGE_CAT(edge));
		new_edge2 = CfgEdgeNew(cfg, CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)),CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),CFG_EDGE_CAT(CFG_EDGE_CORR(edge)));
		CFG_EDGE_SET_CORR(new_edge1,new_edge2);
		CFG_EDGE_SET_CORR(new_edge2,new_edge1);
    }
    else
    {
      CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),CFG_EDGE_CAT(edge));
    }
  }
  return bblCopy;
}/*}}}*/

static t_bool CanBblPrependWithTwoWayOpaquePredicate(t_bbl * bbl)/*{{{*/
{
  static t_regset regset_flags;
  static t_bool initialized = FALSE;
  if(!initialized)
  {
    regset_flags = RegsetNew();
    RegsetSetAddReg(regset_flags, I386_CONDREG_OF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_SF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_ZF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_AF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_PF);
    RegsetSetAddReg(regset_flags, I386_CONDREG_CF);
    initialized = TRUE;
  }

  if(!RegsetIsEmpty(RegsetIntersect(BblRegsLiveBefore(bbl), regset_flags)))
    return FALSE;
  return TRUE;
}/*}}}*/

static t_bool BblPrependWithTwoWayOpaquePredicate(t_bbl * bbl)/*{{{*/
{
  t_i386_ins * ins;
  t_bbl * secondHalf;
  
  if(!CanBblPrependWithTwoWayOpaquePredicate(bbl))
    FATAL(("can't do it cleanly"));

  ins = T_I386_INS(InsNewForBbl(bbl));
  I386InstructionMakeCondJump(ins, I386_CONDITION_B);
  InsPrependToBbl(T_INS(ins),bbl);
  
  ins = T_I386_INS(InsNewForBbl(bbl));
  I386InstructionMakeBt(ins, I386_REG_EAX, 0);
  InsPrependToBbl(T_INS(ins), bbl);

  secondHalf = BblSplitBlock(bbl, INS_INEXT(BBL_INS_FIRST(bbl)), FALSE);
  
  CfgEdgeNew(BBL_CFG(bbl), bbl, secondHalf, ET_JUMP);

  
  return TRUE;  
}/*}}}*/

static t_bblEdgeList * bblEdgeList = NULL;
t_diversity_options DiversityUnfold(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  t_bbl * bbl;
  t_cfg_edge * edge;
  
  /*PHASE 0: ask user which basic blocks to split{{{*/
  if(phase == 0)
  {
    t_uint32 counter=0;
    bblEdgeList = BblEdgeListNew();
    
    CFG_FOREACH_BBL(cfg,bbl)
    {
      if(CanUnfoldBbl(bbl) && !BblIsFrozen(bbl))
      {
	edge = BBL_PRED_FIRST(bbl);
	while(edge && CFG_EDGE_PRED_NEXT(edge))
	{
	  edge = CFG_EDGE_PRED_NEXT(edge);
	  if(CanUnfoldBblByEdge(bbl,edge))
	      {
		  BblEdgeListAdd(bbl,edge, bblEdgeList);
	      }
	}
      }
    }
    
    ret.flags = TRUE;
    ret.done = FALSE;
    ret.range = bblEdgeList->count;
    ret.phase = 1;
    ret.element1 = bblEdgeList;
    phase = 1;
    return ret;
  }
  /*}}}*/

  /*PHASE 1 split basic blocks selected by the user{{{*/
  else if (phase == 1)
  {
    t_bbl_edge_item * item = bblEdgeList->first;
    t_uint32 counter = 0;

    while(item != NULL)
    {
      if(choice->flagList->flags[counter])
      {
	UnfoldBblByEdge(item->bbl, item->edge);
      }
      item = item->next;
      counter++;
    }
    ret.done = TRUE;
  }
  /*}}}*/
  
  return ret;
}/*}}}*/

static t_bblList * bblList = NULL;
t_diversity_options DiversitySplitByTwoWayPredicate(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  t_bbl * bbl;
  
  /*PHASE 0: ask user which basic blocks to split{{{*/
  if(phase == 0)
  {
    t_uint32 counter=0;
    bblList = BblListNew();
    
    CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);
    if (ConstantPropagationInit(cfg))
      ConstantPropagation (cfg, CONTEXT_SENSITIVE);
    else
      FATAL(("constant propagation not available"));
    
    CFG_FOREACH_BBL(cfg,bbl)
    {
      if(CanBblPrependWithTwoWayOpaquePredicate(bbl) && BBL_NINS(bbl)!=0 && CanUnfoldBbl(bbl) && !BblIsHot(bbl) && !BblIsFrozen(bbl))
//	if(counter++ == diablosupport_options.debugcounter)
	{
	  BblListAdd(bblList,bbl);
	}
    }
    
    ret.flags = TRUE;
    ret.done = FALSE;
    ret.range = bblList->count;
    ret.phase = 1;
    ret.element1 = bblList;
    phase = 1;
    return ret;
  }
  /*}}}*/
  /*PHASE 1 split basic blocks selected by the user{{{*/
  else if (phase == 1)
  {
    t_bbl_item * item = bblList->first;
    t_uint32 counter = 0;

    while(item != NULL)
    {
      t_bbl_item * tmp = NULL;
      if(choice->flagList->flags[counter])
      {
	BblPrependWithTwoWayOpaquePredicate(item->bbl);
      }
      else 
	tmp = item;
      
      item = item->next;
      counter++;
      if(tmp)
	BblListUnlink(tmp,bblList);
    }

    item = bblList->first;
    while(item != NULL)
    {
      UnfoldBblByEdge(CFG_EDGE_TAIL(BBL_SUCC_FIRST(item->bbl)), BBL_SUCC_FIRST(item->bbl));
      item = item->next;
    }

    FreeConstantInformation (cfg);
    ConstantPropagationFini(cfg);
    ret.done = TRUE;
  }
  /*}}}*/
  
  return ret;
}/*}}}*/


t_arraylist* DiversitySplitByTwoWayPredicateCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (CanBblPrependWithTwoWayOpaquePredicate(bbl) && BBL_NINS(bbl)!=0 && CanUnfoldBbl(bbl))
    return SimpleCanTransform(bbl, NULL, 1);
  return NULL;
}

t_diversity_options DiversitySplitByTwoWayPredicateDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  BblPrependWithTwoWayOpaquePredicate(bbl);
  UnfoldBblByEdge(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl)), BBL_SUCC_FIRST(bbl));
  return diversity_options_null;
}



t_arraylist* DiversityUnfoldCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  t_cfg_edge * edge;
  t_arraylist* list;
  
  if(!CanUnfoldBbl(bbl))
    return NULL;

  list = ArrayListNew(sizeof(t_transformation_cost_info*), 2);
	edge = BBL_PRED_FIRST(bbl);
	while(edge && CFG_EDGE_PRED_NEXT(edge)) {
    edge = CFG_EDGE_PRED_NEXT(edge);
    if(CanUnfoldBblByEdge(bbl,edge)) {
      t_transformation_cost_info* cost_data = (t_transformation_cost_info*) Malloc(sizeof(t_transformation_cost_info));

      /* For each possible unfolding of this basic block, we keep track of the associated edge */
      t_cfg_edge** edge_ptr = (t_cfg_edge**) Malloc(sizeof(t_cfg_edge*));
      *edge_ptr = edge;

      cost_data->bbl = bbl;
      cost_data->transformation = NULL; /* Must be filled out later */
      cost_data->cost = 1;
      cost_data->additional_info = edge_ptr;

      ArrayListAdd(list, &cost_data);
    }
	}

  return list;
}

t_diversity_options DiversityUnfoldDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  t_cfg_edge* edge_ptr = *(t_cfg_edge**)additional_info;

  UnfoldBblByEdge(bbl, edge_ptr);
  Free(additional_info);

  return diversity_options_null;
}
