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

#include "sm_codebyte.h"
#include "../../arch/i386/i386_instructions.h"
#include "../../kernel/diablo_iterators.h"
#include "../../kernel/diablo_code_layout.h"
#include <diablosupport.h>
#include <diabloobject.h>
#include "../../diablo_options.h"
#include <Judy.h>

#define ET_MUSTCHAIN  	ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE  	ET_CALL | ET_SWI


static t_bool CodebyteIsLeader(t_codebyte * codebyte)
{
  t_constraint_item * constraint_item;
  t_constraint * constraint;
  CODEBYTE_FOREACH_CONSTRAINT(codebyte, constraint, constraint_item)
  {
    if(constraint->offset == -1)
      return FALSE;
  }
  return TRUE;
}

static t_codebyte * CodebyteSuccessor(t_codebyte * codebyte)
{
  t_constraint * constraint;
  t_constraint_item * constraint_item;
  CODEBYTE_FOREACH_CONSTRAINT(codebyte, constraint, constraint_item)
  {
    if(constraint->offset == 1)
      return constraint->base;
  }
  return NULL;
}

static void AddFollowConstraint(t_codebyte * pred, t_codebyte * succ)
{
  t_constraint * constraint = Calloc(1,sizeof(t_constraint));
  constraint->base = succ;
  constraint->offset = 1;
  AddConstraintToList(constraint,pred->constraints);
  
  constraint = Calloc(1,sizeof(t_constraint));
  constraint->base = pred;
  constraint->offset = -1;
  AddConstraintToList(constraint,succ->constraints);
}

static void AddConstraintsForBbl(t_bbl * bbl)
{
  t_ins * ins;
  
  BBL_FOREACH_INS(bbl, ins)
  {
    /*codebytes of the same instruction must be placed successively*/
    t_state_item * state_item;
    INS_FOREACH_STATE_ITEM(ins,state_item)
    {
      if(STATE_ITEM_NEXT(state_item))
      {
	AddFollowConstraint(STATE_ITEM_CODEBYTE(state_item), STATE_ITEM_CODEBYTE(STATE_ITEM_NEXT(state_item)));
      }
    }
    
    /*instructions within a basic block must be placed successively*/
    if(INS_NEXT(ins))
    {
      AddFollowConstraint(INS_CODEBYTE_LAST(ins), INS_CODEBYTE_FIRST(INS_NEXT(ins)));
    }
  }
}

void SmFinalize(t_object * obj)
/*Determine the order of the codebytes and migrate them to instructions of type data so we can keep the rest of diablo {{{*/
{
  t_cfg * cfg = T_CFG(obj->code[0]->data);
  t_codebyte_list * leaders = Calloc(1,sizeof(t_codebyte_list));
  t_codebyte * codebyte;
  t_codebyte_item * codebyte_item;
  t_bbl * final_bbl = BblNew(cfg), * iter_bbl, * tmp_bbl;
  t_ins * ins, *ins_tmp;
  t_ins * entry_ins;
  t_chain_holder chains;
  t_codebyte_list * cfg_codebyte_list = CFG_CODEBYTE_LIST(cfg);

  /*create constraints between codebytes{{{*/
  {
    t_bbl * bbl;
    t_bbl * iter_bbl;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      t_cfg_edge * edge;

      /*Add constraints within basic block*/
      AddConstraintsForBbl(bbl);

      /*Add constraints for ft_like edges */
      if( IsHellNode(bbl) || (BBL_FUNCTION(bbl) && (FunctionGetExitBlock(BBL_FUNCTION(bbl)) == bbl)) || BBL_NINS(bbl)==0 )
	continue;

      iter_bbl = bbl;
      do
      {
	BBL_FOREACH_SUCC_EDGE(iter_bbl,edge)
	{
	  if (EdgeTestCategoryOr(edge,ET_MUSTCHAIN))
	  {
	    break;
	  }
	  if (EdgeTestCategoryOr(edge,ET_MUSTCHAINMAYBE) && EDGE_CORR(edge))
	  {
	    edge = CFG_EDGE_CORR(edge);
	    break;
	  }
	}
	if(edge)
	  iter_bbl = T_BBL(CFG_EDGE_TAIL(edge));
      } while(edge && BBL_NINS(iter_bbl)==0);
      
      if (edge)
      {
	AddFollowConstraint(INS_CODEBYTE_LAST(BBL_INS_LAST(bbl)), INS_CODEBYTE_FIRST(BBL_INS_FIRST(iter_bbl)));
      }
    }
  }
  
  /*}}}*/
  /*Phase 1: determine ordering by solving constraints (for the time being limited to follow constraints)*/
  CODEBYTELIST_FOREACH_CODEBYTE(cfg_codebyte_list, codebyte, codebyte_item)
  {
    if(CodebyteIsLeader(codebyte))
    {
      AddCodebyteToList(codebyte,leaders);
    }
  }

  /*Phase 2: create new data instructions*/
  CODEBYTELIST_FOREACH_CODEBYTE(leaders, codebyte, codebyte_item)
  {
    t_codebyte * inchain = codebyte;
    while(inchain)
    {
      InsAppendToBbl(CODEBYTE_INITIALSTATE(inchain)->byte_ins, final_bbl);
      inchain=CodebyteSuccessor(inchain);
    }
  }

  /*set entry point*/
  entry_ins =  CODEBYTE_INITIALSTATE(INS_CODEBYTE_FIRST(BBL_INS_FIRST(cfg->entries->entry_bbl)))->byte_ins;
  
  /*Phase 3: kill all old relocations*/
  {
    t_reloc * rel;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      if(iter_bbl == final_bbl)
	continue;

      while(RELOCATABLE_REFERS_TO(iter_bbl))
      {
	rel = (RELOCATABLE_REFERS_TO(iter_bbl))->rel;
	RelocTableRemoveReloc(REL_RELOCTABLE(rel), rel);
      }
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	while(RELOCATABLE_REFERS_TO(ins))
	{
	  rel = (RELOCATABLE_REFERS_TO(ins))->rel;
	  RelocTableRemoveReloc(REL_RELOCTABLE(rel), rel);
	}
      }
    }
  }

  /*Phase 3bis: migrate all relocations */
  {
    t_reloc * rel;
    CFG_FOREACH_BBL(cfg,iter_bbl)
    {
      if(iter_bbl == final_bbl)
	continue;
      
      while(RELOCATABLE_REFED_BY(iter_bbl))
      {
	rel = (RELOCATABLE_REFED_BY(iter_bbl))->rel;
	RelocSetToRelocatable(rel, CODEBYTE_INITIALSTATE(INS_CODEBYTE_FIRST(BBL_INS_FIRST(iter_bbl)))->byte_ins);
	rel->edge = NULL;
      }
      
      if(RELOCATABLE_E_REFED_BY(iter_bbl))
	FATAL((""));
      
      BBL_FOREACH_INS(iter_bbl,ins)
      {
	while(RELOCATABLE_REFED_BY(ins))
	{
	  rel = (RELOCATABLE_REFED_BY(ins))->rel;
	  RelocSetToRelocatable(rel, CODEBYTE_INITIALSTATE(INS_CODEBYTE_FIRST(ins))->byte_ins);
	  rel->edge = NULL;
	}
	
	if(RELOCATABLE_E_REFED_BY(ins))
	  FATAL((""));
	
      }
    }
  }

  /*Phase 4: kill all other instructions and basic blocks*/
  CFG_FOREACH_BBL_SAFE(cfg, iter_bbl, tmp_bbl)
  {
    t_reloc * rel;
    if(iter_bbl == final_bbl)
      continue;
    BBL_FOREACH_INS_SAFE(iter_bbl, ins, ins_tmp)
    {
      InsKill(ins);
    }
    BblKill(iter_bbl);
  }

#if 0
  BBL_FOREACH_INS(final_bbl,ins)
  {
    if(INS_OLD_ADDRESS(ins)==0x80484b6)
      if(RELOCATABLE_E_REFED_BY(ins))
      {
	VERBOSE(0,("%x",INS_OLD_ADDRESS(RELOCATABLE_E_REFED_BY(ins)->rel->e.sec)));
	VERBOSE(0,("%x",INS_OLD_ADDRESS(REL_TO_RELOCATABLE(RELOCATABLE_REFERS_TO(ins)->rel))));
      }
  }
#endif
  
  //SMDeflowgraph(CFG_OBJECT(cfg));
  

  I386CreateChains(cfg,&chains);
  
  MergeAllChains(&chains);
  
  OBJECT_CODE(CFG_OBJECT(cfg))[0]->type = DEFLOWGRAPHING_CODE_SECTION;
  OBJECT_CODE(CFG_OBJECT(cfg))[0]->callbacks.SectionRecalculateSize=SectionRecalculateSizeDeflowgraphing;
  OBJECT_CODE(CFG_OBJECT(cfg))[0]->data = chains.chains[0];
  
  I386DeflowFixpoint(obj);

  obj->entry=INS_CADDRESS(entry_ins);

  SectionFromDeflowgraphedToDisassembled(OBJECT_CODE(CFG_OBJECT(cfg))[0]);
  

  /* {{{ debug code: list the final program */
  {
    char * filename = StringConcat2(global_options.output_name,".list");
    FILE * f = fopen(filename,"w");
    if (f)
    {
      t_ins * ins;
      for (ins=obj->code[0]->data; ins; ins=INS_NEXT(ins))
	FileIo(f,"@I\n",ins);
      fclose(f);
    }
    else
      VERBOSE(0,("Could not open %s for writing!",filename));
    Free(filename);
  } /* }}} */
  
  CfgFree(cfg);

  Free(chains.chains);
}
  /*}}}*/
/* vim: set shiftwidth=2 foldmethod=marker:*/
