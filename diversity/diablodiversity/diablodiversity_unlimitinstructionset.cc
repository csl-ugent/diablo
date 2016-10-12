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
#include <diablodiversity_limitinstructionset.h>
#ifdef __cplusplus
}
#endif

#define ITERATIONS 2

/*
 * Unlimit* functions
 */
void UnlimitJmp(t_cfg *cfg, t_bbl *bbl, t_i386_ins *ins)/*{{{*/
{
  t_i386_ins *jmp = ins;
  t_i386_ins *push, *ret;

  t_reg src_reg = I386_OP_BASE(I386_INS_SOURCE1(jmp));
  /*VERBOSE(0, ("ORIG: @I", jmp));*/

  push = dlis_I386InsNewForBbl(bbl);
  ret = dlis_I386InsNewForBbl(bbl);

  I386InstructionMakePush(push, src_reg, (t_uint32)0);
  I386InstructionMakeReturn(ret);

  dlis_I386InsInsertBefore(push, jmp);
  dlis_I386InsInsertBefore(ret, jmp);

  I386InsKill(jmp);
}/*}}}*/


void UnlimitImmJmp(t_cfg *cfg, t_bbl *bbl, t_i386_ins *ins)/*{{{*/
{
  t_i386_ins *jmp = ins;
  t_i386_ins *push, *ret;
  t_bbl* link_path = NULL;

  push = dlis_I386InsNewForBbl(bbl);
  ret = dlis_I386InsNewForBbl(bbl);

  I386InstructionMakePush(push, I386_REG_NONE, (t_uint32)0);
  I386InstructionMakeReturn(ret);

  link_path = T_BBL(EDGE_TAIL((t_edge*) BBL_SUCC_FIRST(bbl)));

  RelocTableAddRelocToRelocatable(
      OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      0x0,
      T_RELOCATABLE(push),
      0x0,
      T_RELOCATABLE(link_path),
      0,
      FALSE,
      NULL,
      NULL,
      NULL,
      "R00" "\\" WRITE_32
      );
  I386_OP_FLAGS(I386_INS_SOURCE1(push)) = I386_OPFLAG_ISRELOCATED;

  {
    t_bbl * target = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
    CfgEdgeKill(BBL_SUCC_FIRST(bbl));
    CfgEdgeNew(cfg,bbl,target,ET_RETURN);
  }

  dlis_I386InsInsertBefore(push, jmp);
  dlis_I386InsInsertBefore(ret, jmp);

  I386InsKill(jmp);

}/*}}}*/

/*
 * Works
 */
void UnlimitImmAdd(t_cfg *cfg, t_bbl *bbl, t_i386_ins *ins)/*{{{*/
{
  t_i386_ins *add = ins;
  t_i386_ins *lea;

  t_reg dest_reg = I386_OP_BASE(I386_INS_DEST(add));
  t_reg base_reg = dest_reg;
  t_reg index_reg = I386_REG_NONE;
  t_uint32 offset = I386_OP_IMMEDIATE(I386_INS_SOURCE1(add));
  t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(add));

  lea = dlis_I386InsNewForBbl(bbl);

  // Scale doesn't matter here
  dlis_I386InstructionMakeLea(lea, dest_reg, offset, base_reg, index_reg, /*int scale*/0, /*memopsize*/0, /*regmode*/regmode);
  /*VERBOSE(0, ("ADD: @I", add));*/
  /*VERBOSE(0, ("LEA: @I (%d)", lea, (int)offset));*/

  dlis_I386InsInsertBefore(lea, add);

  I386InsKill(add);
}/*}}}*/

/*
 * Works
 * TODO: size
 */
void UnlimitAdd(t_cfg *cfg, t_bbl *bbl, t_i386_ins *ins)/*{{{*/
{
  t_i386_ins *add = ins;
  t_i386_ins *lea;

  t_regset r = InsRegsLiveAfter((t_ins*)add);
  if (!RegsetIn(r, I386_CONDREG_OF) &&
      !RegsetIn(r, I386_CONDREG_SF) &&
      !RegsetIn(r, I386_CONDREG_ZF) &&
      !RegsetIn(r, I386_CONDREG_AF) &&
      !RegsetIn(r, I386_CONDREG_CF) &&
      !RegsetIn(r, I386_CONDREG_PF))
  {
    t_reg dest_reg = I386_OP_BASE(I386_INS_DEST(add));
    t_reg base_reg = dest_reg;
    t_reg index_reg = I386_OP_BASE(I386_INS_SOURCE1(add));
    t_uint32 offset = 0;
    t_i386_regmode regmode = I386_OP_REGMODE(I386_INS_DEST(add));

    lea = dlis_I386InsNewForBbl(bbl);

    dlis_I386InstructionMakeLea(lea, dest_reg, offset, base_reg, index_reg, /*int scale*/0, /*memopsize*/0, regmode);
    /*VERBOSE(0, ("ADD: @I", add));*/
    /*VERBOSE(0, ("LEA: @I (%d)", lea, (int)offset));*/

    dlis_I386InsInsertBefore(lea, add);

    I386InsKill(add);
  }
}/*}}}*/

/*
 * Main
 */
  t_diversity_options 
DiversityUnLimitInstructionSet(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)
{
  t_diversity_options ret;
  t_bbl * bbl;
  t_i386_ins * ins, *tmp;
  t_uint32 i = 0;
  /*t_uint32 doneSoFar = 0;*/

  CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);

  for(i=0; i< ITERATIONS; i++)
  {
    CFG_FOREACH_BBL(cfg, bbl)
    {
      BBL_FOREACH_I386_INS_SAFE(bbl, ins, tmp)
      {
	if(I386_INS_OPCODE(ins) == I386_JMP)
	{
	  if(I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg)
	  {
	    UnlimitJmp(cfg, bbl, ins);
	  }
	  else if(I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)
	  {
	    UnlimitImmJmp(cfg, bbl, ins);
	  }
	}
	else if(I386_INS_OPCODE(ins) == I386_ADD)
	{
	  if(I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm && I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
	  {
	    UnlimitImmAdd(cfg, bbl, ins);
	  }
	  else if(I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg && I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg)
	  {
	    UnlimitAdd(cfg, bbl, ins);
	  }
	}
	else if(I386_INS_OPCODE(ins) == I386_LEA)
	{
	  /*VERBOSE(0, ("LEA: @I", ins));*/
	}
      }
    }
  }
  return ret;
}

/* vim: set shiftwidth=2 foldmethod=marker: */

