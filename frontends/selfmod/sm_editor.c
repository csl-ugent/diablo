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
#include <diabloobject.h>
#include <diablosupport.h>
#include "../../kernel/diablo_bbl.h"
#include "../../kernel/diablo_iterators.h"
#include "../../arch/i386/i386_opcode.h"
#include "../../arch/i386/i386_instructions.h"

t_ins * AddInsToBbl(t_string string, t_bbl * bbl)
{
  t_ins * ins = InsNewForBbl(bbl);
  i386_stringToIns(string,ins);
  InsAppendToBbl(ins,bbl);
  return ins;
}

void AddLelijkaardToBbl(bbl)
{
  t_ins * lelijkaard;
  lelijkaard = InsNewForBbl(bbl);
  I386ClearIns(lelijkaard);
  I386_INS_OPCODE(lelijkaard) = I386_MOVZX;
  
  I386_OP_TYPE(I386_INS_DEST(lelijkaard)) = i386_optype_reg;
  I386_OP_REGMODE(I386_INS_DEST(lelijkaard)) = i386_regmode_full32;
  I386_OP_BASE(I386_INS_DEST(lelijkaard)) = I386_REG_EDX;
  
  I386_OP_TYPE(I386_INS_SOURCE2(lelijkaard)) = i386_optype_none;
  
  I386_OP_TYPE(I386_INS_SOURCE1(lelijkaard)) = i386_optype_mem;
  I386_OP_REGMODE(I386_INS_SOURCE1(lelijkaard)) = i386_regmode_full32;
  I386_OP_BASE(I386_INS_SOURCE1(lelijkaard)) = I386_REG_ECX;
  I386_OP_INDEX(I386_INS_SOURCE1(lelijkaard)) = I386_REG_NONE;
  I386_OP_MEMOPSIZE(I386_INS_SOURCE1(lelijkaard)) = 1;

  InsAppendToBbl(lelijkaard,bbl);
}
  
t_bbl * AddEditorFunction(t_cfg * cfg)
{
  t_bbl * bbl1 = BblNew(cfg);
  t_bbl * bbl2 = BblNew(cfg);
  t_bbl * bbl3 = BblNew(cfg);
  t_bbl * bbl4 = BblNew(cfg);
  t_bbl * bbl5 = BblNew(cfg);
  t_bbl * bbl6 = BblNew(cfg);
  t_bbl * bbl7 = BblNew(cfg);
  t_bbl * bbl8 = BblNew(cfg);
  t_bbl * bbl9 = BblNew(cfg);
  t_bbl * bbl10 = BblNew(cfg);
  
  AddInsToBbl("pushl	%ebp"		,bbl1);
  AddInsToBbl("movl	%esp, %ebp"	,bbl1);
  AddInsToBbl("movl	%esp, %ebp"	,bbl1);
  AddInsToBbl("pushl	%eax"		,bbl1);
  AddInsToBbl("pushl	%ebx"		,bbl1);
  AddInsToBbl("pushl	%ecx"		,bbl1);
  AddInsToBbl("pushl	%edx"		,bbl1);
  AddInsToBbl("movl	8(%ebp), %eax"	,bbl1);
  AddInsToBbl("movb	4(%eax), %dl"	,bbl1);
  AddInsToBbl("testb	%dl, %dl"	,bbl1);
  AddInsToBbl("movl	(%eax), %ebx"	,bbl1);
  AddInsToBbl("movb	%dl, -13(%ebp)"	,bbl1);
  AddInsToBbl("leal	5(%eax), %ecx"	,bbl1);
  AddInsToBbl("je	.L17"		,bbl1);

  AddLelijkaardToBbl(bbl2);/*movsbl (%ecx),%edx*/
  AddInsToBbl("addl	%edx, %ebx"	,bbl2);
  AddInsToBbl("movb	1(%ecx), %dl"	,bbl2);
  AddInsToBbl("addl	$2, %ecx"	,bbl2);
  AddInsToBbl("testb	%dl, %dl"	,bbl2);
  AddInsToBbl("jmp	.L35"		,bbl2);
  
  AddInsToBbl("movb	(%ecx), %al"	,bbl3);
  AddInsToBbl("movb	%al, (%ebx)"	,bbl3);
  AddInsToBbl("incl	%ecx"		,bbl3);	
  AddInsToBbl("incl	%ebx"		,bbl3);
  AddInsToBbl("decb	%dl"		,bbl3);

  AddInsToBbl("jne	.L38"		,bbl4);

  AddInsToBbl("decb	-13(%ebp)"	,bbl5);
  AddInsToBbl("jne	.L33"		,bbl5);

  AddInsToBbl("movb	(%ecx), %dl"	,bbl6);
  AddInsToBbl("movb	%dl, -13(%ebp)"	,bbl6);
  AddInsToBbl("incl	%ecx"		,bbl6);	
  AddInsToBbl("testb	%dl, %dl"	,bbl6);
  AddInsToBbl("jne	.L33"		,bbl6);

  AddInsToBbl("movl	12(%ebp), %ebx"	,bbl7);
  AddInsToBbl("movb	(%ecx), %dl"	,bbl7);
  AddInsToBbl("testb	%dl, %dl"	,bbl7);
  AddInsToBbl("movb	1(%ecx), %cl"	,bbl7);
  AddInsToBbl("jmp	.L36"		,bbl7);
  
  AddInsToBbl("cmpb	%cl, %dl"	,bbl8);
  AddInsToBbl("setne	%al"		,bbl8);
  AddInsToBbl("leal	(%eax,%eax,4), %eax"	,bbl8);
  AddInsToBbl("movb	%al, (%ebx)"	,bbl8);
  AddInsToBbl("addl	$30, %ebx"	,bbl8);
  AddInsToBbl("decb	%dl"		,bbl8);

  AddInsToBbl("jne	.L39"		,bbl9);

  AddInsToBbl("popl	%edx"		,bbl10);
  AddInsToBbl("popl	%ecx"		,bbl10);
  AddInsToBbl("popl	%ebx"		,bbl10);
  AddInsToBbl("popl	%eax"		,bbl10);
  AddInsToBbl("leave"			,bbl10);
  
  {
    t_ins * ins = AddInsToBbl("ret",bbl10);
    I386_OP_TYPE(I386_INS_SOURCE1(ins))=i386_optype_imm;
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(ins))=2;
    I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins))=8;
  }

  CfgEdgeCreate(cfg,bbl1,bbl2,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl3,bbl4,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl4,bbl5,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl5,bbl6,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl6,bbl7,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl8,bbl9,ET_FALLTHROUGH);
  CfgEdgeCreate(cfg,bbl9,bbl10,ET_FALLTHROUGH);
  
  CfgEdgeCreate(cfg,bbl1,bbl6,ET_JUMP);
  CfgEdgeCreate(cfg,bbl2,bbl4,ET_JUMP);
  CfgEdgeCreate(cfg,bbl4,bbl3,ET_JUMP);
  CfgEdgeCreate(cfg,bbl5,bbl2,ET_JUMP);
  CfgEdgeCreate(cfg,bbl6,bbl2,ET_JUMP);
  CfgEdgeCreate(cfg,bbl7,bbl9,ET_JUMP);
  CfgEdgeCreate(cfg,bbl9,bbl8,ET_JUMP);

  return bbl1;
}
