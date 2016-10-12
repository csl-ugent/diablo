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

#define i386OperandIsRelocated(op) ((I386_OPFLAG_ISRELOCATED & I386_OP_FLAGS(op)) != 0)

t_bool InsIsControlTransferInstructionWithImmediateTarget(t_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
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
      if(I386_OP_TYPE(I386_INS_SOURCE1(ins))==i386_optype_imm)
      {
	if(I386_INS_OPCODE(ins)!=I386_JECXZ)
	  I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins)) = 4;
	return TRUE;
      }
      else return FALSE;
    default:
      break;
  }
  return FALSE;
}

static t_reloc * NewRelocForByte(t_reloc * reloc_orig, t_uint8 byte, t_ins * ins)
{
  t_reloc * ret = RelocTableDupReloc(REL_RELOCTABLE(reloc_orig),reloc_orig);
  t_string_array * string_array;

  RelocSetFrom(ret,T_RELOCATABLE(ins));
  ret->from_offset = 0;

  if(strchr(reloc_orig->code,'P'))
  {
    ret->addend-=byte;
    WARNING(("jeopardy"));
  }
//    FATAL(("Might need to migrate from to refed instruction instead of parent instruction"));
  
  string_array = StringDivide(reloc_orig->code,"\\",TRUE,FALSE);
  if(StringArrayNStrings(string_array)!=3)
    FATAL((""));
  
  switch(byte)
  {
    case 0:
      string_array->first->string = StringConcat2(string_array->first->string,"i000000ff&");
      break;
    case 1:
      string_array->first->string = StringConcat2(string_array->first->string,"i0000ff00&i00000008>");
      break;
    case 2:
      string_array->first->string = StringConcat2(string_array->first->string,"i00ff0000&i00000010>");
      break;
    case 3:
      string_array->first->string = StringConcat2(string_array->first->string,"iff000000&i00000018>");
      break;
  }

  ret->code = StringArrayJoin(string_array,"\\");
  
  return ret;
}

static t_bbl * ControlTransferGetTarget(t_ins * ins)
{
  t_bbl * bbl;
  t_cfg_edge * edge;
  bbl=INS_BBL(ins);
  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if(!(EDGE_CAT(edge)&ET_FT_BLOCK_TRUE))
      break;
  }
  if(edge==NULL)
    FATAL(("@I",ins));
  return T_BBL(EDGE_TAIL(edge));
}

static t_reloc * CreateControlTransferReloc(t_uint8 offset, t_ins * ins, t_bbl * target)
{
  
  t_reloc * ret;
  switch(offset)
  {
    case 1: ret = RelocTableAddRelocToRelocatable (INS_OBJECT(ins)->reloc_table, offset, T_RELOCATABLE(ins), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(ins), "S01RA+-iff000000&i00000018>\\l*w\\s0000$", FALSE);
	    break;
    case 2: ret = RelocTableAddRelocToRelocatable (INS_OBJECT(ins)->reloc_table, offset, T_RELOCATABLE(ins), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(ins), "S01RA+-i00ff0000&i00000010>\\l*w\\s0000$", FALSE);
	    break;
    case 3: ret = RelocTableAddRelocToRelocatable (INS_OBJECT(ins)->reloc_table, offset, T_RELOCATABLE(ins), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(ins), "S01RA+-i0000ff00&i00000008>\\l*w\\s0000$", FALSE);
	    break;
    case 4: ret = RelocTableAddRelocToRelocatable (INS_OBJECT(ins)->reloc_table, offset, T_RELOCATABLE(ins), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(ins), "S01RA+-i000000ff&\\l*w\\s0000$", FALSE);
	    break;
    default: FATAL(("%d",offset));
  }
  return ret;
}

t_reloc * CreateShortTransferReloc(t_uint8 offset, t_ins * ins, t_bbl * target)
{
  return RelocTableAddRelocToRelocatable (INS_OBJECT(ins)->reloc_table, offset, T_RELOCATABLE(ins), 0, T_RELOCATABLE(target), 0, FALSE, NULL, NULL, T_RELOCATABLE(ins), "S01RA+-i000000ff&\\l*w\\s0000$", FALSE);
}

static void StateCreateIns(t_state * state)
{
  t_ins * base_ins = state->parent_ins;
  t_ins * new_ins = InsNewForCfg(BBL_CFG(INS_BBL(base_ins)));
  t_reloc * source1_reloc = I386GetRelocForOp(base_ins, I386_INS_SOURCE1(base_ins));
  t_reloc * source2_reloc = I386GetRelocForOp(base_ins, I386_INS_SOURCE2(base_ins));
  t_reloc * dest_reloc = I386GetRelocForOp(base_ins, I386_INS_DEST(base_ins));
  t_reloc * to_be_added = NULL;

  INS_OLD_ADDRESS(new_ins) = INS_OLD_ADDRESS(base_ins);
  
  state->byte_ins = new_ins;
  
  INS_TYPE(new_ins) = IT_DATA;

  if(!state->value_relocatable)
  {
    I386_INS_DATA(new_ins) = state->value;
    return;
  }

  else if(InsIsControlTransferInstructionWithImmediateTarget(base_ins))
  {
    if((I386_INS_OPCODE(base_ins)==I386_JECXZ))
    {
      to_be_added = CreateShortTransferReloc(INS_CSIZE(base_ins)-state->offset, new_ins, ControlTransferGetTarget(base_ins));    
      return;
    }
    to_be_added = CreateControlTransferReloc(INS_CSIZE(base_ins)-state->offset, new_ins, ControlTransferGetTarget(base_ins));    
    return;
  }
  
  else if(source1_reloc && (state->offset >= source1_reloc->from_offset) && (state->offset < source1_reloc->from_offset+4))
    to_be_added = NewRelocForByte(source1_reloc, state->offset - source1_reloc->from_offset, new_ins);
  
  else if(source2_reloc && (state->offset >= source2_reloc->from_offset) && (state->offset < source2_reloc->from_offset+4)) 
    to_be_added = NewRelocForByte(source2_reloc, state->offset - source2_reloc->from_offset, new_ins);
  
  else if(dest_reloc && (state->offset >= dest_reloc->from_offset) && (state->offset < dest_reloc->from_offset+4))
    to_be_added = NewRelocForByte(dest_reloc, state->offset - dest_reloc->from_offset, new_ins);

  if(to_be_added == NULL)
    FATAL(("@I %d",base_ins,state->offset));
  
  return;
}

void SmInitializeIns(t_ins * ins, t_codebyte_list * cfg_codebyte_list)
{
  t_reloc * source1_reloc = NULL;
  t_reloc * source2_reloc = NULL;
  t_reloc * dest_reloc = NULL;
  char buf [15];
  t_uint8 size;
  t_uint8 i;
  t_state_list * ins_state_list = Calloc(1,sizeof(t_state_list));
  PWord_t JudyValue;

  JLI(JudyValue, JudyMapSM, ins);
  *JudyValue = ins_state_list;

  if(InsIsControlTransferInstructionWithImmediateTarget(ins) && !(I386_INS_OPCODE(ins)==I386_JECXZ))
  {
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins)) = 4;
  }
  if(i386OperandIsRelocated(I386_INS_SOURCE1(ins)))
  {
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins)) = 4;
    source1_reloc = I386GetRelocForOp(ins,I386_INS_SOURCE1(ins));
  }
  if(i386OperandIsRelocated(I386_INS_SOURCE2(ins)))
  {
    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(ins)) = 4;
    source2_reloc = I386GetRelocForOp(ins,I386_INS_SOURCE2(ins));
  }
  if(i386OperandIsRelocated(I386_INS_DEST(ins)))
  {
    I386_OP_IMMEDSIZE(I386_INS_DEST(ins)) = 4;
    dest_reloc = I386GetRelocForOp(ins,I386_INS_DEST(ins));
  }
  
  size = I386AssembleIns(ins, buf);
  INS_CSIZE(ins)=size;
  
  if(size > 15)
    FATAL(("wow"));

  for(i=0;i<size;i++)
  {
    t_codebyte * codebyte = CodebyteNew();
    t_state * state = Calloc(1,sizeof(t_state));
    
    AddCodebyteToList(codebyte,cfg_codebyte_list);
    AddStateToList(state,ins_state_list);
    AddStateToList(state,codebyte->states);
    
    state->parent_ins = ins;
    state->offset = i;
    state->value = buf[i];
    state->codebyte = codebyte;
    if(	
	(InsIsControlTransferInstructionWithImmediateTarget(ins) && ((I386_INS_OPCODE(ins)==I386_JECXZ && i== 1) || (I386_INS_OPCODE(ins)!=I386_JECXZ && i>= size-4))) ||
	(source1_reloc && (i >= source1_reloc->from_offset) && (i< source1_reloc->from_offset+4)) ||
        (source2_reloc && (i >= source2_reloc->from_offset) && (i< source2_reloc->from_offset+4)) ||
      	(dest_reloc && (i >= dest_reloc->from_offset) && (i< dest_reloc->from_offset+4))
      )
      state->value_relocatable = TRUE;
    StateCreateIns(state);
  }
}

void SmInitialize(t_object * obj)
{
  t_cfg * cfg = T_CFG(obj->code[0]->data);
  t_ins * ins;
  t_bbl * bbl;
  PWord_t JudyValue;
  t_reloc * rel;

  JudyMapSM = (Pvoid_t) NULL;
  ugly_global_codebyte_list = Calloc(1,sizeof(t_codebyte_list));
  
  OBJECT_FOREACH_RELOC(obj,rel)
  {
    char * code;
    if(strchr(rel->code,'R'))
      FATAL(("Not taken into account"));
    if(strchr(rel->code,'P'))
    {
      RelocESetToRelocatable(rel, REL_FROM(rel));
      REL_E_IS_TO_SYMBOL(rel)=FALSE;
    }
    while(code = strchr(rel->code,'P'))
    {
      *code = 'R';
    }	
  }
  
  /*create codebytes{{{*/
  CFG_FOREACH_BBL(cfg, bbl)
  {
    BBL_FOREACH_INS(bbl, ins)
    {
      SmInitializeIns(ins,CFG_CODEBYTE_LIST(cfg));
    }
  }
  
#if 0
  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_ins * ins;
    t_reloc * rel;
    t_state_item * state_item;
    BBL_FOREACH_INS(bbl,ins)
    {
      t_state_item * state_item;
      if(INS_OLD_ADDRESS(ins)==0x804c9f4)
      {
	INS_FOREACH_STATE_ITEM(ins, state_item)
	{
	  rel = RELOCATABLE_REFERS_TO(state_item->state->byte_ins);
	  if(rel)
	  {
	    VERBOSE(0,("1<<"));
	    VERBOSE(0,("@R",rel));
	    VERBOSE(0,(">>2"));
	  }
	}
      }
    }
  }
#endif
  /*}}}*/
}


/* vim: set shiftwidth=2 foldmethod=marker:*/
