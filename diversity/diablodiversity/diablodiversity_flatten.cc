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

#include <obfuscation/obfuscation_architecture_backend.h>
#include <obfuscation/obfuscation_transformation.h>


/*{{{ List stuff, what year are we living in anyway?*/
typedef struct _t_function_item t_function_item;
typedef struct _t_functionList t_functionList;

struct _t_function_item
{
  t_function * function;
  t_function_item * next;
  t_function_item * prev;
};

struct _t_functionList
{
  t_function_item * first;
  t_function_item * last;
  t_uint64 count;
};

void FunctionListAdd(t_function * function, t_functionList * list)
{
  t_function_item * item= (t_function_item *) Calloc(1,sizeof(t_function_item));
  item->function = function;

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

t_functionList * FunctionListNew()
{
  t_functionList * ret = (t_functionList *) Calloc(1,sizeof(t_functionList));
  ret->first = NULL;
  ret->last = NULL;
  ret->count = 0;
  return ret;
}

void FunctionListFree(t_functionList * list)
{
  if(list == NULL)
    return;
  while (list->last != NULL)
  {
    t_function_item * item = list->last;
    list->last = list->last->prev;
    Free(item);
  }
  Free(list);
}

void FunctionListUnlink(t_function_item  * item, t_functionList * functionList)
{
  if(item == functionList->first)
    functionList->first = item->next;
  if(item == functionList->last)
    functionList->last = item->prev;

  if(item->prev)
    item->prev->next = item->next;
  if(item->next)
    item->next->prev = item->prev;

  functionList->count--;
}
/*}}}*/

Pvoid_t JudyMapSM1;

typedef struct t_fun_hulp t_fun_hulp;
struct t_fun_hulp
{
  t_uint32 nr_bbl;
  t_bbl ** switch_list;
  t_uint32 from_nr_bbl;
  t_bbl ** from_switch_list;
  t_uint32 toc_nr_bbl;
  t_bbl *** toc_switch_list;
};

t_bool CanBeFlattened(t_function * fun) /* {{{ */
{
  t_bbl * bbl;

  if(FUNCTION_IS_HELL(fun))
    return FALSE;

  /* No flattening of functions with a call as last instruction. Some library-functions will not be flattened... */

  if(FUNCTION_BBL_LAST(fun) && BBL_INS_LAST(FUNCTION_BBL_LAST(fun)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(FUNCTION_BBL_LAST(fun))))==I386_CALL)
    return FALSE;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    t_cfg_edge * edge;

    BBL_FOREACH_PRED_EDGE(bbl,edge)
      if(EDGE_CAT((t_edge*)(t_edge*)edge)==ET_IPJUMP ||
	  EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
	  EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN )
	return FALSE;
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
      if(EDGE_CAT((t_edge*)edge)==ET_IPJUMP ||
	  EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
	  EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN ||
	  EDGE_CAT((t_edge*)edge)==ET_SWITCH)
	return FALSE;

    if(bbl && BBL_INS_LAST(bbl) &&
	( I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_ECXZ ||
	 I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOP ||
	 I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOPZ ||
	 I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOPNZ))
      return FALSE;
  }
  return TRUE;
} /* }}} */

t_int32 CountBasicBlocks(t_function * fun)/* {{{ */
{
  t_int32 to_nr_bbl=0;
  t_bbl * bbl;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    t_cfg_edge * edge;
    t_bool yes=TRUE;

    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      if(EDGE_CAT((t_edge*)(t_edge*)edge)==ET_IPJUMP ||
	  EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
	  EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN )
	return FALSE;
    }
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if(EDGE_CAT((t_edge*)edge)==ET_IPJUMP ||
	  EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
	  EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN ||
	  EDGE_CAT((t_edge*)edge)==ET_SWITCH)
	return FALSE;

      if(EDGE_CAT((t_edge*)edge)==ET_CALL ||
	  EDGE_CAT((t_edge*)edge)==ET_RETURN ||
	  EDGE_CAT((t_edge*)edge)==ET_COMPENSATING ||
	  EDGE_CAT((t_edge*)edge)==ET_SWI)
      {
	yes=FALSE;
	continue;
      }
      if(BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
	yes=FALSE;
      if(EDGE_CORR((t_edge*)edge)!=NULL)
      {
	yes=FALSE;
      }
    }
    if(yes)
    {
      to_nr_bbl++;
    }
  }

  return to_nr_bbl;
}/* }}} */

t_bool FlattenFunctionBasicDiversity(t_function * fun, t_bool more_switch, t_bool_list * flagList) /* {{{ */
{
  t_section * var_section;
  t_bbl * bbl, * switch_bbl = NULL, *switch_bbl1, *split_off, ** switch_list, ** from_switch_list, ** to_switch_list, *** toc_switch_list;
  t_i386_ins * ins;
  t_cfg * cfg=FUNCTION_CFG(fun);
  t_uint32 tot_bbl=1, nr_bbl=0, to_nr_bbl=0, to_nr_bbl2=0,toc_nr_bbl=0, from_nr_bbl=0, i=0, j=0, l=0;
  t_bool first_bbl_is_flattened = TRUE;

  t_fun_hulp * fun_h;
  void* JudyValue;

  if(FUNCTION_IS_HELL(fun))
    return FALSE;

  fun_h = (t_fun_hulp*) Calloc(1,sizeof(t_fun_hulp));

  JLI(JudyValue, JudyMapSM1, (Word_t)fun);
  *((PWord_t)JudyValue) = (Word_t)fun_h;

  /* Number of basic blocks in the function */
  FUNCTION_FOREACH_BBL(fun,bbl)
    tot_bbl++;

  /* Put all blocks; into a list => every block has a number in the switch-table {{{ */
  switch_list=(t_bbl**) Malloc(sizeof(t_bbl*)*tot_bbl);
  fun_h->switch_list=switch_list;

  switch_list[0]=FUNCTION_BBL_FIRST(fun);
  nr_bbl=1;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    switch_list[nr_bbl]=bbl;
    nr_bbl++;
  }
  fun_h->nr_bbl=nr_bbl;
  /* }}} */

  /* Put all blocks going to switch into a list {{{ */
  to_switch_list=(t_bbl**) Malloc(sizeof(t_bbl*)*tot_bbl);
  to_nr_bbl=0;
  to_nr_bbl2=0;

  /* No flattening of functions with a call as last instruction. Some library-functions will not be flattened... */

  if(FUNCTION_BBL_LAST(fun) && BBL_INS_LAST(FUNCTION_BBL_LAST(fun)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(FUNCTION_BBL_LAST(fun))))==I386_CALL)
    return FALSE;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    t_cfg_edge * edge;
    t_bool yes=TRUE;

    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      if(EDGE_CAT((t_edge*)(t_edge*)edge)==ET_IPJUMP ||
        EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
        EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN )
        return FALSE;
    }
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if(EDGE_CAT((t_edge*)edge)==ET_IPJUMP ||
        EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
        EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN ||
        EDGE_CAT((t_edge*)edge)==ET_SWITCH)
        return FALSE;

      if(EDGE_CAT((t_edge*)edge)==ET_CALL ||
        EDGE_CAT((t_edge*)edge)==ET_RETURN ||
        EDGE_CAT((t_edge*)edge)==ET_COMPENSATING ||
        EDGE_CAT((t_edge*)edge)==ET_SWI)
      {
        yes=FALSE;
        continue;
      }
      if(BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
        yes=FALSE;
      if(EDGE_CORR((t_edge*)edge)!=NULL)
      {
        yes=FALSE;
      }
    }
    if(yes)
    {
      if(flagList->flags[to_nr_bbl])
      {
        to_switch_list[to_nr_bbl2]=bbl;
        to_nr_bbl2++;
      }
      to_nr_bbl++;
    }
  }
  /* }}} */
  to_nr_bbl=to_nr_bbl2;

  /* Look for basic blocks which will have a add stubs {{{ */
  toc_switch_list=(t_bbl***) Malloc(sizeof(t_bbl**)*2);
  fun_h->toc_switch_list=toc_switch_list;
  toc_switch_list[0]=(t_bbl**) Malloc(sizeof(t_bbl*)*tot_bbl);
  toc_switch_list[1]=(t_bbl**) Malloc(sizeof(t_bbl*)*tot_bbl);
  toc_nr_bbl=0;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    t_cfg_edge * edge;
    t_bool yes=TRUE;

    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      //      if(EDGE_CAT((t_edge*)edge)==ET_JUMP ||
      //	  EDGE_CAT((t_edge*)edge)==ET_FALLTHROUGH )
      yes=TRUE;
    }
    if(yes) {
      /*NO STUB FOR RETURN-BLOCK */
      if(bbl!=FUNCTION_BBL_LAST(fun))
      {
        toc_switch_list[0][toc_nr_bbl]=bbl;
        toc_nr_bbl++;
      }
      else
      {
      }
    }
  }
  fun_h->toc_nr_bbl=toc_nr_bbl;
  /* }}} */

  /* List: From switch-block to .... blocks{{{ */
  from_switch_list=(t_bbl**) Malloc(sizeof(t_bbl*)*tot_bbl);
  fun_h->from_switch_list=from_switch_list;
  from_nr_bbl=1;

  from_switch_list[0]=FUNCTION_BBL_FIRST(fun);

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    t_cfg_edge * edge;
    t_bool yes=TRUE;

    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      if(EDGE_CAT((t_edge*)edge)==ET_IPJUMP ||
        EDGE_CAT((t_edge*)edge)==ET_IPFALLTHRU ||
        EDGE_CAT((t_edge*)edge)==ET_IPUNKNOWN ||
        EDGE_CAT((t_edge*)edge)==ET_SWITCH)
      {
        return FALSE;
      }

      if(EDGE_CAT((t_edge*)edge)==ET_CALL ||
        EDGE_CAT((t_edge*)edge)==ET_RETURN ||
        EDGE_CAT((t_edge*)edge)==ET_JUMP ||
        EDGE_CAT((t_edge*)edge)==ET_FALLTHROUGH ||
        EDGE_CAT((t_edge*)edge)==ET_SWI)
      {
        yes=FALSE;
        continue;
      }
      if(BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
        yes=FALSE;
    }
    if(yes)
    {
      /* test: ander gaat de switch ook naar het return-blok! */
      if(bbl!=FUNCTION_BBL_LAST(fun) && bbl!=FUNCTION_BBL_FIRST(fun))
      {
        from_switch_list[from_nr_bbl]=bbl;
        from_nr_bbl++;
      }
    }
  }
  fun_h->from_nr_bbl=from_nr_bbl;
  /* }}} */

  var_section=ObjectNewSubsection(CFG_OBJECT(cfg),tot_bbl*sizeof(t_uint32),RODATA_SECTION);

  /* Top Switch Production {{{ */

  /* If BBL_FIRST is not actually in the list of basic blocks to flatten, we should not modify it, especially not by splitting it/adding pushes to it! */
  first_bbl_is_flattened = FALSE;
  for (i = 0; i < to_nr_bbl; i++) {
    if (to_switch_list[i] == switch_list[0]) {
      first_bbl_is_flattened = TRUE;
    }
  }
  
  if (first_bbl_is_flattened) {
    if(BBL_INS_FIRST(switch_list[0])==NULL)
    {
      split_off=BblNew(cfg);
      BBL_SET_REGS_LIVE_OUT(split_off,BBL_REGS_LIVE_OUT(switch_list[0]));
      BBL_SET_EXEC_COUNT(split_off, BBL_EXEC_COUNT(switch_list[0]));
    }
    else
    {
      split_off=BblSplitBlock(switch_list[0],BBL_INS_FIRST(switch_list[0]),TRUE);
    }

    {
      t_cfg_edge * edge;
      t_cfg_edge * h_edge;
      BBL_FOREACH_SUCC_EDGE_SAFE(switch_list[0],edge,h_edge)
        CfgEdgeKill(edge);
    }
    switch_bbl1= switch_list[0];
    switch_list[0]=split_off;
  } else {
    /* The first basic block won't be flattened */
    split_off = NULL;
    switch_bbl1 = BblNew(cfg);
    BblInsertInFunction(switch_bbl1, fun);
  }

  for(j=0;j<from_nr_bbl;j++)
    if(from_switch_list[j]==switch_bbl1)
      from_switch_list[j]=split_off;
  for(j=0;j<to_nr_bbl;j++)
    if(to_switch_list[j]==switch_bbl1)
      to_switch_list[j]=split_off;
  for(j=0;j<toc_nr_bbl;j++)
    if(toc_switch_list[0][j]==switch_bbl1)
      toc_switch_list[0][j]=split_off;

  // Make the switch at beginning of the function
  /* switch_bbl:
  * jmp *0x<...>(,%eax,4) */
  if (first_bbl_is_flattened) {
    I386MakeInsForBbl(Push,Append,ins,switch_bbl1,I386_REG_EAX,0x0);
    I386MakeInsForBbl(Push,Append,ins,switch_bbl1,I386_REG_EBX,0x0);
    //I386MakeInsForBbl(PushF,Append,ins,switch_bbl1);
    I386MakeInsForBbl(MovToReg,Append,ins,switch_bbl1,I386_REG_EAX,I386_REG_NONE,0);
  }
  I386MakeInsForBbl(JumpMem,Append,ins,switch_bbl1,I386_REG_NONE,I386_REG_EAX);
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),0x0,T_RELOCATABLE(ins),0x0,T_RELOCATABLE(var_section),0x0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) =I386_OPFLAG_ISRELOCATED;

  if(!more_switch && first_bbl_is_flattened)
    switch_bbl1=BblSplitBlock(switch_bbl1,BBL_INS_LAST(switch_bbl1),TRUE);
  /* }}} */

  for(j=0;j<nr_bbl;j++)
  {
    t_reloc * rel;
    t_cfg_edge * edge;
    t_bool bbl_itself=FALSE;

    for(i=0;i<from_nr_bbl;i++)
      if(from_switch_list[i]==switch_list[j])
      {
        t_uint32 k;
        bbl_itself=TRUE;

        for(k=0;k<toc_nr_bbl;k++) {
          if(toc_switch_list[0][k]==switch_list[j])
          {
            t_bbl* bblX;
            {
              bblX= BblNew(cfg);
              BblInsertInFunction(bblX,fun);

              //I386MakeInsForBbl(PopF,Append,ins,bblX);
              I386MakeInsForBbl(Pop,Append,ins,bblX,I386_REG_EBX);
              I386MakeInsForBbl(Pop,Append,ins,bblX,I386_REG_EAX);
              I386MakeInsForBbl(Jump,Append,ins,bblX);

              CfgEdgeCreate(cfg,bblX,toc_switch_list[0][k],ET_JUMP);

              /* TODO: this is an over-approximation */
              BBL_SET_EXEC_COUNT(bblX, BBL_EXEC_COUNT(toc_switch_list[0][k]));
            }
            toc_switch_list[1][k]=bblX;


            edge=CfgEdgeCreate(cfg,switch_bbl1,bblX,ET_SWITCH);
            rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),0x0,T_RELOCATABLE(var_section),4*j,T_RELOCATABLE(bblX),0x0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
            CFG_EDGE_SET_REL(edge,rel);
            CFG_EDGE_SET_SWITCHVALUE(edge,j);
            RELOC_SET_SWITCH_EDGE(rel,edge);
          }
        }
      }

      if(!bbl_itself)
      {
        for(i=0;i<toc_nr_bbl;i++)
          if(toc_switch_list[0][i]==switch_list[j])
          {
            t_bbl * bblX;

            {
              bblX= BblNew(cfg);
              BblInsertInFunction(bblX,fun);

              //I386MakeInsForBbl(PopF,Append,ins,bblX);
              I386MakeInsForBbl(Pop,Append,ins,bblX,I386_REG_EBX);
              I386MakeInsForBbl(Pop,Append,ins,bblX,I386_REG_EAX);
              I386MakeInsForBbl(Jump,Append,ins,bblX);

              CfgEdgeCreate(cfg,bblX,toc_switch_list[0][i],ET_JUMP);

              /* TODO: this is an over-approximation */
              BBL_SET_EXEC_COUNT(bblX, BBL_EXEC_COUNT(toc_switch_list[0][i]));
            }
            toc_switch_list[1][i]=bblX;

            edge=CfgEdgeCreate(cfg,switch_bbl1,toc_switch_list[1][i],ET_SWITCH);
            rel=RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),0x0,T_RELOCATABLE(var_section),4*j,T_RELOCATABLE(toc_switch_list[1][i]),0x0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
            CFG_EDGE_SET_REL(edge,rel);
            CFG_EDGE_SET_SWITCHVALUE(edge,j);
            RELOC_SET_SWITCH_EDGE(rel,edge);
          }
      }
  }

  for(i=0;i<nr_bbl;i++)
  {
    t_cfg_edge * edge;
    t_cfg_edge * h_edge;
    t_int32 integrity=0;
    bbl=switch_list[i];

    for(j=0;j<from_nr_bbl;j++)
    {
      if(from_switch_list[j]==bbl)
      {
        /* We already added pop instructions to all basic blocks' predecessors in the to-flatten bbs */
        if(integrity!=0)
        {
          FATAL(("ERR: A basic block is in more than one time in  the from_switch_list-list\n"));
          /* Temp solution. A basic block is sometimes more than one time in the from_switch_list-list*/
          //break;
        }
        else
          integrity++;
      }
    }

    for(l=0;l<to_nr_bbl;l++)
    {
      if(to_switch_list[l]==bbl)
      {
        if(bbl && BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_Jcc)
        {
          /* Jcc-BBL transformation {{{ */
          /* push %eax
          * push %ebx
          * pushf */
          I386MakeInsForIns(Push,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EAX,0x0);
          I386MakeInsForIns(Push,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EBX,0x0);
          //I386MakeInsForIns(PushF,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)));

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
          {
            if(EDGE_CAT((t_edge*)edge)==ET_JUMP)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForIns(MovToReg,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EBX,I386_REG_NONE,j);
            }
            else if(EDGE_CAT((t_edge*)edge)==ET_FALLTHROUGH)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForIns(MovToReg,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EAX,I386_REG_NONE,j);
            }
            else
              FATAL(("IMPOSSIBLE"));
          }

          BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,h_edge)
            CfgEdgeKill(edge);

          I386MakeInsForIns(CondMov,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EBX,I386_REG_EAX,I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))));
          I386InstructionMakeJump(T_I386_INS(BBL_INS_LAST(bbl)));

          if(more_switch)
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl,ET_JUMP);
          }
          else
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl1,ET_JUMP);
          }
          /* }}} */
        }
        else if(bbl && BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_JMP)
        {
          /* JMP BBL transformation {{{ */
          I386MakeInsForIns(Push,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EAX,0x0);
          I386MakeInsForIns(Push,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EBX,0x0);
          //I386MakeInsForIns(PushF,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)));

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
          {
            if(EDGE_CAT((t_edge*)edge)==ET_JUMP)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForIns(MovToReg,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EAX,I386_REG_NONE,j);
            }
            else if(EDGE_CAT((t_edge*)edge)==ET_FALLTHROUGH)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForIns(MovToReg,Before,ins,T_I386_INS(BBL_INS_LAST(bbl)),I386_REG_EAX,I386_REG_NONE,j);
            }
            else
            {
              printf("EDGE:%d %x Test:%x\n",EDGE_CAT((t_edge*)edge),EDGE_CAT((t_edge*)edge),(1U<<15));
              FATAL(("IMPOSSIBLE"));
            }
          }

          BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,h_edge)
            CfgEdgeKill(edge);

          if(more_switch)
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl,ET_JUMP);
          }
          else
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl1,ET_JUMP);
          }

          /* }}} */
        }
        else  if(bbl && BBL_INS_LAST(bbl) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))!=I386_RET)
        {
          /* BBL transformation {{{ */
          I386MakeInsForBbl(Push,Append,ins,bbl,I386_REG_EAX,0x0);
          I386MakeInsForBbl(Push,Append,ins,bbl,I386_REG_EBX,0x0);
          //I386MakeInsForBbl(PushF,Append,ins,bbl);

          BBL_FOREACH_SUCC_EDGE(bbl,edge)
          {
            if(EDGE_CAT((t_edge*)edge)==ET_JUMP)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForBbl(MovToReg,Append,ins,bbl,I386_REG_EAX,I386_REG_NONE,j);
            }
            else if(EDGE_CAT((t_edge*)edge)==ET_FALLTHROUGH)
            {
              for(j=0;j<nr_bbl;j++)
                if(switch_list[j]==CFG_EDGE_TAIL(edge))
                  break;

              I386MakeInsForBbl(MovToReg,Append,ins,bbl,I386_REG_EAX,I386_REG_NONE,j);
            }
            else
            {
              printf("EDGE:%d %x Test:%x\n",EDGE_CAT((t_edge*)edge),EDGE_CAT((t_edge*)edge),(1U<<9));
              FATAL(("IMPOSSIBLE"));
            }
          }

          BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,h_edge)
            CfgEdgeKill(edge);

          I386MakeInsForBbl(Jump,Append,ins,bbl);

          if(more_switch)
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl,ET_JUMP);
          }
          else
          {
            CfgEdgeCreate(cfg,bbl,switch_bbl1,ET_JUMP);
          }
          /* }}} */
        }
      }
    }
  }

  return TRUE;
}
 /* }}} */

t_functionList * functionList;

t_diversity_options DiversityFlatten(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase) /*{{{*/
{
  t_diversity_options ret;
  t_function * fun;
  t_int32 fun_tel=0;
  
  auto obfuscators = GetObfuscatorsForType("flattenfunction:nomultiple");
  auto obfuscator  = dynamic_cast<FunctionObfuscationTransformation*>(obfuscators.at(0)); // TODO cleaner
      
  if(phase == 0)
  {
    functionList = FunctionListNew();
    CFG_FOREACH_FUN(cfg,fun)
    {
      //if(CanBeFlattened(fun) /*&& !FunIsHot(fun) && !FunIsFrozen(fun)*/)
      if (obfuscator->canTransform(fun))
      {
	FunctionListAdd(fun, functionList);
      }
    }
    ret.range=functionList->count;
    ret.flags=TRUE;
    if(ret.range==0)
      ret.done=TRUE;
    else
      ret.done=FALSE;
    return ret;
  }
  else if(phase == 1)
  {
    t_function_item * function_item;
    t_uint32 i;
    for(function_item = functionList->first, i=0; function_item; i++)
    {
      t_function_item * next = function_item->next;
      if(!choice->flagList->flags[i])
	FunctionListUnlink(function_item, functionList);
      function_item = next;
    }
  }
  else if(phase == 2)
  {
    //FlattenFunctionBasicDiversity(functionList->first->function, FALSE, choice->flagList);
    obfuscator->doTransform(functionList->first->function, NULL); // TODO: flaglist to select which BBLs!
    FunctionListUnlink(functionList->first, functionList);
  }

  if(functionList->count ==0)
    ret.done = TRUE;
  else
  {
    ret.range=CountBasicBlocks(functionList->first->function);
    ret.flags = TRUE;
    ret.done = FALSE;
  }

  return ret;
}
/* }}} */

t_arraylist* DiversityFlattenFunctionCanTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void** additional_info) {
  if (CanBeFlattened(fun)) {
    int bblcount = 0, maxbbls = CountBasicBlocks(fun);
    t_bbl* bbl;
    t_arraylist* list = ArrayListNew(sizeof(t_transformation_cost_info*), 2);

    /* We add one transformation possibility per basic block -> we will merge from 1 to bblcount basic blocks.
       Which basic blocks will be merged, will be decided when */
    for (bblcount = 0; bblcount < maxbbls; bblcount++) {
      t_transformation_cost_info* cost_data = (t_transformation_cost_info*) Malloc(sizeof(t_transformation_cost_info));
      int* count_ptr = (int*) Malloc(sizeof(int));

      *count_ptr = bblcount;

      cost_data->bbl = NULL;
      cost_data->transformation = NULL; /* Must be filled out later */
      cost_data->cost = 1;
      cost_data->additional_info = count_ptr;

      ArrayListAdd(list, &cost_data);
    }

    return list;
  } else {
    return NULL;
  }
}

t_diversity_options DiversityFlattenFunctionDoTransform(t_cfg* cfg, t_function* fun, t_bbl* bbl, void* additional_info) {
  /* The additional info is the number of basic blocks to merge, chose that many at random */
  // TODO: actually chose at random, not just the first n
  t_bool_list* list = BoolListNewAllFalse(CountBasicBlocks(fun));
  int i;
  int max_bbl = *(int*)additional_info;
  Free(additional_info);

  VERBOSE(0, ("Flattening %i BBs out of %i in function '%s'", max_bbl, CountBasicBlocks(fun), FUNCTION_NAME(fun)));

  for (i = 0; i < max_bbl; i++) {
    list->flags[i] = TRUE;
  }

  //FlattenFunctionBasicDiversity(fun, FALSE, list);
  auto obfuscators = GetObfuscatorsForType("flattenfunction:nomultiple");
  auto obfuscator  = dynamic_cast<FunctionObfuscationTransformation*>(obfuscators.at(0)); // TODO cleaner
  obfuscator->doTransform(fun, NULL); // TODO: flaglist to select which BBLs!

  Free(list);
  return diversity_options_null;
}