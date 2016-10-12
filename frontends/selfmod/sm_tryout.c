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
#include "sm_tryout.h"
#include "sm_initialize.h"
#include "sm_finalize.h"
#include "sm_codebyte.h"
#include "sm_cluster.h"
#include <diabloobject.h>
#include <diablosupport.h>
#include "../../kernel/diablo_function.h"
#include "../../kernel/diablo_flowgraph.h"
#include "../../kernel/diablo_iterators.h"
#include "../../arch/i386/i386_registers.h"
#include "../../arch/i386/i386_instructions.h"

t_bbl * CreateStub(t_bbl * entry_of_editor, t_ins * entry_of_function, t_ins * edit_script, t_cfg * cfg, int count, t_bbl * previous_stub, t_address address)
{
  t_bbl * ret = BblNew(cfg);
  t_bbl * bbl_middle = BblNew(cfg);
  t_bbl * bbl_last = BblNew(cfg);
  t_ins * ins;
  t_ins * skipjump;
  t_state_item * state_item;
  
  /*skipjump: jump over next jump*/
  ins = InsNewForBbl(ret);
  INS_OLD_ADDRESS(ins) = address;
  I386InstructionMakeJump(ins);
  CfgEdgeCreate(cfg,ret,bbl_last,ET_JUMP);
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  InsAppendToBbl(ins,ret);

  skipjump = ins;
  
  /*jmp to entry point of function*/
  ins = InsNewForBbl(bbl_middle);
  I386InstructionMakePush(ins, I386_REG_NONE, 0);
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table,4,ins,1,entry_of_function,0,FALSE,NULL,NULL,NULL,"S01PA+-\\l*w\\s0000$",FALSE);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins))|=I386_OPFLAG_ISRELOCATED;
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  I386_INS_DATA(INS_STATE_ITEM_FIRST(ins)->state->byte_ins) = 0xe9;
  InsAppendToBbl(ins,bbl_middle);

  if(previous_stub)
    skipjump = BBL_INS_FIRST(previous_stub);
  skipjump = INS_STATE_ITEM_FIRST(skipjump)->state->byte_ins;
  
  /*push location of first skipjump offset*/
  ins = InsNewForBbl(bbl_last);
  I386InstructionMakePush(ins, I386_REG_NONE, 0);
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table,1,ins,0,skipjump,0,FALSE,NULL,NULL,NULL,"S01A+\\l*w\\s0000$",FALSE);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins))|=I386_OPFLAG_ISRELOCATED;
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  InsAppendToBbl(ins,bbl_last);
  
  ins = InsNewForBbl(bbl_last);
  I386InstructionMakePush(ins, I386_REG_NONE, 0);
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table,0,ins,1,edit_script,0,FALSE,NULL,NULL,NULL,"S01\\l*w\\s0000$",FALSE);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins))|=I386_OPFLAG_ISRELOCATED;
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  InsAppendToBbl(ins,bbl_last);
  
  ins = InsNewForBbl(bbl_last);
  I386InstructionMakePush(ins, I386_REG_NONE, 0);
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table,0,ins,1,entry_of_function,0,FALSE,NULL,NULL,NULL,"S01\\l*w\\s0000$",FALSE);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins))|=I386_OPFLAG_ISRELOCATED;
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  InsAppendToBbl(ins,bbl_last);

  ins = InsNewForBbl(bbl_last);
  I386InstructionMakeJump(ins);
  CfgEdgeCreate(cfg,bbl_last,entry_of_editor,ET_JUMP);
  SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
  InsAppendToBbl(ins,bbl_last);

  /*Force ordering*/
  CfgEdgeCreate(cfg,ret,bbl_middle,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl_middle,bbl_last,ET_FALLTHROUGH);
  
  return ret;
}

void SmSetStateToValueOfState(t_state * source, t_state * target)
{
  if(!target->value_relocatable)
    I386_INS_DATA(source->byte_ins) = I386_INS_DATA(target->byte_ins);
  else 
  {
    {
    char * code;
    t_reloc * orig = RELOCATABLE_REFERS_TO(target->byte_ins)->rel;
    t_reloc * rel = 
      RelocTableDupReloc(REL_RELOCTABLE(orig),orig);
    
    RelocSetFrom(rel,source->byte_ins);
    if(strchr(rel->code,'P'))
      FATAL((""));
    }
  }
  return;
}

void AddStatelistToBbl(t_state_list * state_list, t_bbl * bbl, t_cfg * cfg)
{
  t_state_item * state_item = state_list->first;
  while(state_item)
  {
    t_ins * new_ins = InsNewForBbl(bbl);
    t_codebyte * codebyte = CodebyteNew();
    t_state_list * ins_state_list = Calloc(1,sizeof(t_state_list));
    PWord_t JudyValue;

    JLI(JudyValue, JudyMapSM, new_ins);
    *JudyValue = ins_state_list;

    AddCodebyteToList(codebyte,CFG_CODEBYTE_LIST(cfg));
    AddStateToList(state_item->state,ins_state_list);
    AddStateToList(state_item->state,codebyte->states);
    
    INS_TYPE(new_ins) = IT_DATA;
    InsAppendToBbl(new_ins, bbl);
    state_item->state->codebyte = codebyte;

    state_item = state_item->next;
  }
}

void StatelistAddStatesForAddressOfIns(t_state_list * state_list, t_ins * ins, t_cfg * cfg)
{
  t_state * state;
  t_ins * byte_ins;
  
  state = Calloc(1,sizeof(t_state));
  byte_ins = InsNewForCfg(cfg);
  I386ClearIns(byte_ins);
  INS_TYPE(byte_ins) = IT_DATA;
  state->byte_ins = byte_ins;
  state->value_relocatable = TRUE;
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table, 0, T_RELOCATABLE(byte_ins), 0, T_RELOCATABLE(ins), 0, FALSE, NULL, NULL, NULL, "S01iff000000|\\l*w\\s0000$", FALSE);
  AddStateToList(state,state_list);
  
  state = Calloc(1,sizeof(t_state));
  byte_ins = InsNewForCfg(cfg);
  I386ClearIns(byte_ins);
  INS_TYPE(byte_ins) = IT_DATA;
  state->byte_ins = byte_ins;
  state->value_relocatable = TRUE;
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table, 0, T_RELOCATABLE(byte_ins), 0, T_RELOCATABLE(ins), 0, FALSE, NULL, NULL, NULL, "S01i00ff0000|i00000008>\\l*w\\s0000$", FALSE);
  AddStateToList(state,state_list);

  state = Calloc(1,sizeof(t_state));
  byte_ins = InsNewForCfg(cfg);
  I386ClearIns(byte_ins);
  INS_TYPE(byte_ins) = IT_DATA;
  state->byte_ins = byte_ins;
  state->value_relocatable = TRUE;
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table, 0, T_RELOCATABLE(byte_ins), 0, T_RELOCATABLE(ins), 0, FALSE, NULL, NULL, NULL, "S01i0000ff00|i00000010>\\l*w\\s0000$", FALSE);
  AddStateToList(state,state_list);
  
  state = Calloc(1,sizeof(t_state));
  byte_ins = InsNewForCfg(cfg);
  I386ClearIns(byte_ins);
  INS_TYPE(byte_ins) = IT_DATA;
  state->byte_ins = byte_ins;
  state->value_relocatable = TRUE;
  RelocTableAddRelocToRelocatable(CFG_OBJECT(cfg)->reloc_table, 0, T_RELOCATABLE(byte_ins), 0, T_RELOCATABLE(ins), 0, FALSE, NULL, NULL, NULL, "S01i000000ff|i00000018>\\l*w\\s0000$", FALSE);
  AddStateToList(state,state_list);
}
  
#define CHANGE (!goal_state_item->state->dontcare && (goal_state_item->state->value_relocatable || template_state_item->state->value_relocatable || goal_state_item->state->value != template_state_item->state->value))
#define ADDOFFSETANDCOUNT()  {t_state * state;t_ins * byte_ins;state = Calloc(1,sizeof(t_state));byte_ins = InsNewForCfg(cfg);I386ClearIns(byte_ins);INS_TYPE(byte_ins) = IT_DATA;state->byte_ins = byte_ins;AddStateToList(state,state_list);offset_ptr = &I386_INS_DATA(byte_ins);state = Calloc(1,sizeof(t_state));byte_ins = InsNewForCfg(cfg);I386ClearIns(byte_ins);INS_TYPE(byte_ins) = IT_DATA;state->byte_ins = byte_ins;AddStateToList(state,state_list);count_ptr = &I386_INS_DATA(byte_ins);}
#define ADDGLOBALCOUNT()  {t_state * state;t_ins * byte_ins;state = Calloc(1,sizeof(t_state));byte_ins = InsNewForCfg(cfg);I386ClearIns(byte_ins);INS_TYPE(byte_ins) = IT_DATA;state->byte_ins = byte_ins;AddStateToList(state,state_list);global_count_ptr = &I386_INS_DATA(byte_ins);}

//#define EDITSCRIPT_VERBOSE
t_state_list * CreateEditScript(t_state_list * goal, t_state_list * template, t_cfg * cfg, int arg_count, int arg_total)
{
  t_state_list * state_list = Calloc(1,sizeof(t_state_list));
  t_state_item * goal_state_item =  goal->first;
  t_state_item * template_state_item =  template->first;
  t_uint8 * offset_ptr;
  t_uint8 * count_ptr;
  t_uint8 * global_count_ptr;
  t_uint8 offset=0;
  int total_offset=0;
  t_uint8 count=0;
  t_uint8 global_count=0;
  t_bool changing = FALSE;
  int byte_nr=0;
  int template_nr=0;

  VERBOSE(0,("START EDITSCRIPT CREATION"));

  /*Add address of template*/
  StatelistAddStatesForAddressOfIns(state_list, template->first->state->byte_ins, cfg);

#ifdef EDITSCRIPT_VERBOSE
  VERBOSE(0,("%x start_of_edit", byte_nr++));
  VERBOSE(0,("%x start_of_edit", byte_nr++));
  VERBOSE(0,("%x start_of_edit", byte_nr++));
  VERBOSE(0,("%x start_of_edit", byte_nr++));
#endif
  
  ADDGLOBALCOUNT();
#ifdef EDITSCRIPT_VERBOSE
  VERBOSE(0,("%x global_count",byte_nr++));
#endif
  
  for(goal_state_item = goal->first, template_state_item =  template->first; goal_state_item!=NULL && template_state_item != NULL; goal_state_item = goal_state_item->next, template_state_item = template_state_item->next,template_nr++)
  {
    t_state * goal_state = goal_state_item->state;
    t_state * template_state = template_state_item->state;

#ifdef EDITSCRIPT_VERBOSE
    VERBOSE(0,("offset in template: %x",template_nr));
#endif
    
    /*A difference*/
    if(CHANGE)
    {
      /*First Difference*/
      if(!changing)
      {
	if(global_count==255){
	  *global_count_ptr = 255;
#ifdef EDITSCRIPT_VERBOSE
          VERBOSE(0,("PREVIOUS global_count %x",global_count));
#endif
          global_count = 0;
	  ADDGLOBALCOUNT();
#ifdef EDITSCRIPT_VERBOSE
          VERBOSE(0,("%x global_count",byte_nr++));
#endif
	}
	changing = TRUE;
	ADDOFFSETANDCOUNT();
	global_count++;
	total_offset += offset;
	*offset_ptr = offset;
#ifdef EDITSCRIPT_VERBOSE
	VERBOSE(0,("%x offset (%x) (%x)",byte_nr++, offset, total_offset));
	VERBOSE(0,("%x count",byte_nr++));
#endif
	count = 0;
      }

      if(count==255)
      {
	*count_ptr=255;
#ifdef EDITSCRIPT_VERBOSE
          VERBOSE(0,("PREVIOUS count %x",255));
#endif
        ADDOFFSETANDCOUNT();
	global_count++;
#ifdef EDITSCRIPT_VERBOSE
	VERBOSE(0,("%x offset (%x)",byte_nr++,0));
	VERBOSE(0,("%x count",byte_nr++));
#endif
        *offset_ptr=0;
        count = 0;
      }

      {
	t_state * state = Calloc(1,sizeof(t_state));
	t_ins * byte_ins = InsNewForCfg(cfg);
	I386ClearIns(byte_ins);
	INS_TYPE(byte_ins) = IT_DATA;
	state->byte_ins = byte_ins;
	AddStateToList(state,state_list);
	SmSetStateToValueOfState(state, goal_state);
#ifdef EDITSCRIPT_VERBOSE
	VERBOSE(0,("%x change2 %x", byte_nr++, I386_INS_DATA(state->byte_ins)));
#endif
      }
      count++;
    }
    /*End of difference*/
    else if(!CHANGE && changing)
    {
#ifdef EDITSCRIPT_VERBOSE
          VERBOSE(0,("PREVIOUS count %x",count));
#endif
      *count_ptr = count;
      changing = FALSE;
      offset = 1;
      total_offset += count;
    }
    /*Continued overlap*/
    else if(!CHANGE && !changing)
    {
      if(offset==255)
      {
	//FATAL(("case 1"));
        ADDOFFSETANDCOUNT();
	global_count++;
        *offset_ptr=255;
        *count_ptr=0;
        total_offset+=255;
#ifdef EDITSCRIPT_VERBOSE
	VERBOSE(0,("%x offset (%x) (%x)",byte_nr++,255,total_offset));
	VERBOSE(0,("%x count (%x)",byte_nr++,0));
	VERBOSE(0,("PREVIOUS count %x",0));
#endif
        offset = 0;
      }
      offset++;
    }
  }
  if(changing)
  {
#ifdef EDITSCRIPT_VERBOSE
	  VERBOSE(0,("PREVIOUS count %x",count));
#endif
    *count_ptr = count;
  }
#ifdef EDITSCRIPT_VERBOSE
  VERBOSE(0,("PREVIOUS global_count %x",global_count));
#endif
  *global_count_ptr=global_count;
  
  ADDGLOBALCOUNT();
#ifdef EDITSCRIPT_VERBOSE
  VERBOSE(0,("%x global_count",byte_nr++));
  VERBOSE(0,("PREVIOUS global_count %x",0));
#endif
  *global_count_ptr=0;
  
  ADDOFFSETANDCOUNT();

#ifdef EDITSCRIPT_VERBOSE
  VERBOSE(0,("%x stub nr",byte_nr++));
  VERBOSE(0,("%x stub count",byte_nr++));
#endif
  *offset_ptr = arg_total;
  *count_ptr = arg_count;

  if(goal_state_item != NULL || template_state_item != NULL)
    FATAL(("shouldn't happen"));
  
  return state_list;
}

void FixEditor(t_function * fun)
{
  t_bbl * bbl = NULL;
  FUN_FOREACH_BBL(fun,bbl)
  {
    if(I386_INS_OPCODE(BBL_INS_LAST(bbl))==I386_RET)
      break;
  }
  if(!bbl)
    FATAL(("broken assumption"));

  I386_OP_TYPE(I386_INS_SOURCE1(BBL_INS_LAST(bbl)))=i386_optype_imm;
  I386_OP_IMMEDSIZE(I386_INS_SOURCE1(BBL_INS_LAST(bbl)))=2;
  I386_OP_IMMEDIATE(I386_INS_SOURCE1(BBL_INS_LAST(bbl)))=8;
}

void SmTryout(t_object * obj)
{
  t_cfg * cfg = T_CFG(obj->code[0]->data);
  t_function * fun;
  t_bbl * bbl;
  t_ins * ins;
  t_state_item * state_item;
  t_ins * new_ins;
  
  t_bbl * editor = NULL;
 
  CFG_FOREACH_FUN(cfg, fun)  
  {
    if(!strcmp(fun->name,"__sm_prefix_editor")){
      FixEditor(fun);
      editor = FUN_BBL_FIRST(fun);
      break;
    }
  }
  if(!editor)
    FATAL((""));
    
//  editor = AddEditorFunction(cfg);

  SmInitialize(obj);

  printf("SmClusterFunctions(obj);\n");
  SmClusterFunctions(obj, editor);

  SmFinalize(obj);
}
