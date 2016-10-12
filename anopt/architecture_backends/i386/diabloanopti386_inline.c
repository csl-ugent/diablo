/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopti386.h>

/* {{{ helper function: determines whether it is necessary to keep the
 * function's stack behaviour exactly the same before and after inlining */
/* store the stack offset in the tmp field of every instruction */
static t_bool CheckFunctionSimpleness(t_function * fun, t_bool * fp_ret)
{
  t_bbl * bbl = FUNCTION_BBL_FIRST(fun);
  t_i386_ins * ins;
  t_bool simple = TRUE;
  long offset = 0;
  t_bool fp = FALSE;
  t_uint32 fp_offset = 0;

  ins = T_I386_INS(BBL_INS_FIRST(bbl));
  /* if the first block of tile instruction is empty, something funny is going on - just be conservative */
  if (!ins) return FALSE;

  /* does the function use a frame pointer? */
  if (I386_INS_OPCODE(ins) == I386_PUSH && I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP)
  {
    I386_INS_SET_TMP(ins,  (void*)offset);
    ins = I386_INS_INEXT(ins);
    offset += 4;

    if (ins && I386_INS_OPCODE(ins) == I386_MOV &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg && I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP &&
	I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg && I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP)
    {
      I386_INS_SET_TMP(ins,  (void*) offset);
      fp = TRUE;
      fp_offset = 4;
      ins = I386_INS_INEXT(ins);
    }
  }

  for (; ins; ins = I386_INS_INEXT(ins))
  {
    I386_INS_SET_TMP(ins,  (void*) offset);

    /* skip instructions that don't influence the stack */
    if (!RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP) && !RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP) &&
	(!fp || (!RegsetIn(I386_INS_REGS_USE(ins),I386_REG_EBP) && !RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_EBP))))
      continue;

    switch (I386_INS_OPCODE(ins))
    {
      case I386_PUSH:
      case I386_PUSHF:
	offset += 4;
	break;
      case I386_PUSHA:
	offset += 32;
	break;
      case I386_POP:
      case I386_POPF:
	offset -= 4;
	break;
      case I386_POPA:
	offset -= 32;
	break;

      case I386_MOV:
	{
	  t_i386_operand * src = I386_INS_SOURCE1(ins), * dest = I386_INS_DEST(ins);

	  if (I386_OP_TYPE(src) == i386_optype_reg)
	    if (I386_OP_BASE(src) == I386_REG_ESP || (fp && I386_OP_BASE(src) == I386_REG_EBP))
	    {
	      /* danger of aliasing */
	      simple = FALSE;
	      break;
	    }
	  if (I386_OP_TYPE(dest) == i386_optype_reg)
	    if (I386_OP_BASE(dest) == I386_REG_ESP || (fp && I386_OP_BASE(dest) == I386_REG_EBP))
	    {
	      /* we don't know the offsets in the stack frame any more */
	      simple = FALSE;
	      break;
	    }

	  /* load and store relative to esp or ebp without index register are well understood and
	   * ok. all other cases fail the test. */
	  if (I386_OP_TYPE(dest) == i386_optype_mem && I386_OP_INDEX(dest) == I386_REG_NONE &&
	      (I386_OP_BASE(dest) == I386_REG_ESP || (fp && I386_OP_BASE(dest) == I386_REG_EBP)))
	  { /* ok */ }
	  else if (I386_OP_TYPE(src) == i386_optype_mem && I386_OP_INDEX(src) == I386_REG_NONE &&
	      (I386_OP_BASE(src) == I386_REG_ESP || (fp && I386_OP_BASE(src) == I386_REG_EBP)))
	  { /* ok */ }
	  else
	  {
	    /* not ok */
	    simple = FALSE;
	    break;
	  }
	}
	break;

      case I386_ADD:
	{
	  t_i386_operand * src = I386_INS_SOURCE1(ins), * dest = I386_INS_DEST(ins);
	  if (I386_OP_TYPE(dest) == i386_optype_reg)
	  {
	    if (I386_OP_BASE(dest) == I386_REG_ESP)
	    {
	      if (I386_OP_TYPE(src) == i386_optype_imm)
		offset -= I386_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (fp && I386_OP_BASE(dest) == I386_REG_EBP)
	    {
	      if (I386_OP_TYPE(src) == i386_optype_imm)
		fp_offset -= I386_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	  }
	  /* other cases are all ok */
	}
	break;

      case I386_SUB:
	{
	  t_i386_operand * src = I386_INS_SOURCE1(ins), * dest = I386_INS_DEST(ins);
	  if (I386_OP_TYPE(dest) == i386_optype_reg)
	  {
	    if (I386_OP_BASE(dest) == I386_REG_ESP)
	    {
	      if (I386_OP_TYPE(src) == i386_optype_imm)
		offset += I386_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (fp && I386_OP_BASE(dest) == I386_REG_EBP)
	    {
	      if (I386_OP_TYPE(src) == i386_optype_imm)
		fp_offset += I386_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	  }
	  /* other cases are all ok */
	}
	break;

      case I386_RET:
	/* ok */
	break;

      default:
	/* ok as long as stack pointer (and frame pointer) are only used
	 * as base register in memory operands */
	{
	  t_i386_operand * src1 = I386_INS_SOURCE1(ins), * src2 = I386_INS_SOURCE2(ins),* dest = I386_INS_DEST(ins);

	  if (!RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP) && (!fp || (!RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_EBP))))
	  {
	    if (I386_OP_BASE(dest) == I386_REG_ESP || (fp && I386_OP_BASE(dest) == I386_REG_EBP))
	    {
	      if (I386_OP_TYPE(dest) == i386_optype_mem && I386_OP_INDEX(dest) == I386_REG_NONE)
	      { /* ok */ }
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (I386_OP_BASE(src1) == I386_REG_ESP || (fp && I386_OP_BASE(src1) == I386_REG_EBP))
	    {
	      if (I386_OP_TYPE(src1) == i386_optype_mem && I386_OP_INDEX(src1) == I386_REG_NONE)
	      { /* ok */ }
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (I386_OP_BASE(src2) == I386_REG_ESP || (fp && I386_OP_BASE(src2) == I386_REG_EBP))
	    {
	      if (I386_OP_TYPE(src2) == i386_optype_mem && I386_OP_INDEX(src2) == I386_REG_NONE)
	      { /* ok */ }
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	  }
	  else
	  {
	    simple = FALSE;
	    break;
	  }
	}
	break;
    }

    if (fp && fp_offset != 4) simple = FALSE;
    if (offset < 0) simple = FALSE;
    if (!simple)
    {
      /*VERBOSE(0,( "simpleness test failed on @I\n", ins));*/
      break;
    }

  }
  /*if (simple) VERBOSE(0,( "eligible function %s passes simpleness test\n", FUNCTION_NAME(fun))); */

  *fp_ret = fp;
  return simple;
} /* }}} */

t_bool I386CheckFunctionSimpleness(t_function * fun, t_bool * fp_ret)
{
  return CheckFunctionSimpleness(fun,fp_ret);
}

/* {{{ check whether it is possible to inline the function */

static t_bool OpReferencesReturnAddressOnStack(t_i386_operand *op, long offset)
{
  if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    if (I386_OP_BASE(op) == I386_REG_ESP)
    {
      if (I386_OP_IMMEDIATE(op) == offset)
        return TRUE;
    }
    if (I386_OP_BASE(op) == I386_REG_EBP)
    {
      if (I386_OP_IMMEDIATE(op) == 4)
        return TRUE;
    }
  }
  return FALSE;
}

static t_bool CheckInlinable(t_function * fun)
{
  t_cfg_edge * edge;
  t_bbl * retblock = FunctionGetExitBlock(fun);
  t_bbl * i_bbl;
  t_i386_ins *ins;
  t_bool uses_fp;

  if (!retblock) return FALSE;
  /* incoming compensating edges => outgoing interprocedural edges => don't
   * inline the function */
  BBL_FOREACH_PRED_EDGE(retblock,edge)
    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING) return FALSE;
  /* outgoing compensating edges => incoming interprocedural edges => don't
   * inline the function */
  BBL_FOREACH_SUCC_EDGE(retblock,edge)
    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING) return FALSE;

  /* calculate stack offsets */
  CheckFunctionSimpleness(fun,&uses_fp);

  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    t_cfg_edge * i_edge;
    /* skip the return edges */
    if (i_bbl == FunctionGetExitBlock(fun)) continue;
    if (BBL_INS_LAST(i_bbl)==NULL) continue;

    /* check whether the function references the return address */
    BBL_FOREACH_I386_INS(i_bbl,ins)
    {
      long offset = (long) I386_INS_TMP(ins);
      t_i386_operand *src1 = I386_INS_SOURCE1(ins), *src2 = I386_INS_SOURCE2(ins), *dest = I386_INS_DEST(ins);
      if (OpReferencesReturnAddressOnStack(dest,offset))
        return FALSE;
      if (OpReferencesReturnAddressOnStack(src1,offset))
        return FALSE;
      if (OpReferencesReturnAddressOnStack(src2,offset))
        return FALSE;
    }
    if (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(i_bbl)))!=I386_JMP)continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if(CFG_EDGE_CAT(i_edge)==ET_SWITCH)
	return FALSE;
    }
  }
  
  return TRUE;
} /* }}} */

t_bool I386CheckInlinable(t_function * fun)
{
  return CheckInlinable(fun);
}

/*TODO: this is in fact architecture-independent */
/* {{{ copy the structure of the function we want to inline */
static void CopyFunctionStructureToCallSite(t_function * fun, t_bbl * call_site)
{
  t_bbl * i_bbl;
  t_ins * i_ins;
  t_cfg_edge * i_edge;
  t_cfg_edge * ecall;
  t_cfg * cfg = FUNCTION_CFG(fun);
  t_uint64 extra =0;

  /*if (!CheckInlinable(fun)) return FALSE;*/

  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun), ecall)
  {
    extra +=BBL_EXEC_COUNT(CFG_EDGE_HEAD(ecall));
  }

  /* step 1: duplicate all basic blocks of the function */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    t_bbl * copy;

    /* don't copy the return block */
    if (i_bbl == FunctionGetExitBlock(fun))
      continue;

    copy = BblNew(cfg);

    if (extra)
    {
      BBL_SET_EXEC_COUNT(copy, BBL_EXEC_COUNT(i_bbl) * BBL_EXEC_COUNT(call_site)/extra);
      BBL_SET_EXEC_COUNT(i_bbl, BBL_EXEC_COUNT(i_bbl) - BBL_EXEC_COUNT(copy));
    }

    /* temporary references between the duplicate blocks, this is for easy
     * reference from original to copy and vice versa. these references will
     * of course be removed at the end of the inlining. */
    BBL_SET_TMP(i_bbl,  copy);
    BBL_SET_TMP(copy,  i_bbl);

    /* duplicate the instructions in the basic block */
    BBL_FOREACH_INS(i_bbl,i_ins)
    {
      t_ins * inscopy = InsDup(i_ins);
      INS_SET_OLD_ADDRESS(inscopy,  INS_OLD_ADDRESS(i_ins));
      InsAppendToBbl(inscopy, copy);
    }
  }

  /* step 1b: assign the copies of the basic blocks to the calling function */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    if (i_bbl!=FunctionGetExitBlock(fun))
    {	
      t_bbl * copy = T_BBL(BBL_TMP(i_bbl));
      BblInsertInFunction(copy,BBL_FUNCTION(call_site));
    }
  }

  /* step 2: add edges to the newly created blocks */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    /* skip the return edges */
    if (i_bbl == FunctionGetExitBlock(fun)) continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      /* intraprocedural edges should be duplicated between the copies of head
       * and tail. call edges should be duplicated between
       * the copy of the head and the original tail. interproc jump edges that
       * are used for tail calls are skipped for now. other interprocedural
       * edges should not occur. */
      t_bbl * newhead, * newtail;

      /* skip the edges to the return block */
      if (CFG_EDGE_TAIL(i_edge) == FunctionGetExitBlock(fun))
	continue;

      /* interprocedural jumps or fallthroughs are not yet supported */
      if (CfgEdgeIsInterproc(i_edge) && !(CFG_EDGE_CAT(i_edge) == ET_CALL) && CFG_EDGE_CAT(i_edge) != ET_SWI)
      {
	FATAL(("Implement inlining of functions with interprocedural jumps or fallthroughs"));
      }

      newhead = T_BBL(BBL_TMP(i_bbl));
      if (CFG_EDGE_CAT(i_edge) == ET_CALL)
      {
	newtail = CFG_EDGE_TAIL(i_edge);
	if(CFG_EDGE_CORR(i_edge)==NULL)
	{
	  t_cfg_edge * x_edge= CfgEdgeCreateCall(cfg,newhead,newtail,CFG_HELL_NODE(cfg),FUNCTION_BBL_LAST(BBL_FUNCTION(newtail)));
	  CfgEdgeKill(CFG_EDGE_CORR(x_edge));
	  CFG_EDGE_SET_CORR(x_edge,NULL);

	}
	else
	{
	  CfgEdgeCreateCall(cfg,newhead,newtail,T_BBL(BBL_TMP(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)))),FUNCTION_BBL_LAST(BBL_FUNCTION(newtail)));
	}
      }
      else if (CFG_EDGE_CAT(i_edge) == ET_SWI)
      {
	newtail = CFG_EDGE_TAIL(i_edge);
	if(CFG_EDGE_CORR(i_edge)!=NULL)
	{
	  CfgEdgeCreateSwi(cfg,newhead,T_BBL(BBL_TMP(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)))));
	}
	else
	  CfgEdgeNew(cfg,newhead,CFG_SWI_HELL_NODE(cfg),ET_SWI);
      }
      else
      {
	if(!strcmp(FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))),FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_HEAD(i_edge)))))
	  newtail = T_BBL(BBL_TMP(CFG_EDGE_TAIL(i_edge)));
	else
	  newtail = CFG_EDGE_TAIL(i_edge);

	CfgEdgeCreate(cfg,newhead,newtail,CFG_EDGE_CAT(i_edge));
      }
    }
  }
  /* step 3: Look for switch-tables! */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    t_bool found=FALSE;
    /* skip the return edges */
    if (i_bbl == FunctionGetExitBlock(fun)) continue;
    if (BBL_INS_LAST(i_bbl)==NULL) continue;
    if (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(i_bbl)))!=I386_JMP)continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if(CFG_EDGE_CAT(i_edge)==ET_SWITCH)
	found=TRUE;
      else
      {
	found=FALSE;
	break;
      }
    }
    if(found)
    {
      
      t_reloc * switch_rel = RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(i_bbl)));

      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)), RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(BBL_TMP(i_bbl)))));

      if(!switch_rel)
	WARNING(("No Switch Table Found!"));
      else
      {
        FATAL(("Reimplement"));
      }
    }
  }
}
/* }}} */

/* {{{ generic function inlining */
t_bool I386InlineFunAtCallSite(t_function * fun, t_bbl * callsite)
{
  t_cfg_edge * ecall, * eret = NULL, * i_edge;
  t_bbl * bentry = FUNCTION_BBL_FIRST(fun);
  t_bbl * bexit = FUNCTION_BBL_LAST(fun);
  t_bbl * returnsite = NULL;
  t_i386_ins * cjmp, * rjmp, * sub;
  t_cfg * cfg = BBL_CFG(callsite);
  t_bool returning;


  /* find call, link and return edges */
  BBL_FOREACH_SUCC_EDGE(callsite, ecall)
    if (CFG_EDGE_CAT(ecall) == ET_CALL && CFG_EDGE_TAIL(ecall) == bentry)
      break;
  ASSERT(ecall,("inlining: could not find call edge"));
  if(CFG_EDGE_CORR(ecall))
  {
    returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(ecall));
    BBL_FOREACH_PRED_EDGE(returnsite,eret)
      if (CFG_EDGE_CAT(eret) == ET_RETURN && CFG_EDGE_HEAD(eret) == bexit)
	break;
    ASSERT(eret,("inlining: could not find return edge"));
    returning = TRUE;
  }
  else returning = FALSE;

  /* check if there are no interprocedural jumps or fallthroughs */
  {
    t_bbl * bbl;
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
      {
	if ((CFG_EDGE_CAT(i_edge) == ET_IPJUMP) || CFG_EDGE_CAT(i_edge) == ET_IPFALLTHRU)
	{
	  FATAL(("Implement inlining of functions with interprocedural jumps or fallthroughs"));
	}
      }
    }
  }

  /* copy the structure of the function into the caller. */
  CopyFunctionStructureToCallSite(fun,callsite);

  /* change the call instruction into a jump + move the stack pointer 4 bytes
   * down. (this is necessary to keep all stack references in the inlined
   * function correct) */
  InsKill(BBL_INS_LAST(callsite));
  I386MakeInsForBbl(Arithmetic,Append,sub,callsite,I386_SUB,I386_REG_ESP,I386_REG_NONE,4);
  I386MakeInsForBbl(Jump,Append,cjmp,callsite);
  CfgEdgeCreate(cfg,callsite,T_BBL(BBL_TMP(bentry)),ET_JUMP);
  
  /* change the return instructions into jumps + move the stack pointer 4
   * bytes up */
  if(returning)
    BBL_FOREACH_PRED_EDGE(bexit,i_edge)
    {
      t_bbl * bret = BBL_TMP(CFG_EDGE_HEAD(i_edge));
      t_i386_ins * ret = T_I386_INS(BBL_INS_LAST(bret));
      int popbytes = 4;
      ASSERT(CFG_EDGE_CAT(i_edge) == ET_JUMP,("flow graph corrupt"));
      ASSERT(I386_INS_OPCODE(ret) == I386_RET,("weird return instruction @I",ret));

      if (I386_OP_TYPE(I386_INS_SOURCE1(ret)) == i386_optype_imm)
	popbytes += I386_OP_IMMEDIATE(I386_INS_SOURCE1(ret));
      I386InstructionMakeArithmetic(ret,I386_ADD,I386_REG_ESP,I386_REG_NONE,popbytes);
      I386MakeInsForBbl(Jump,Append,rjmp,bret);
      CfgEdgeCreate(cfg,bret,returnsite,ET_JUMP);
    }

  /* clean up the old edges */
  CfgEdgeKill(ecall);
  
  if(returning)
    CfgEdgeKill(eret);
  
  return TRUE;
} /* }}} */

/* {{{ helper function: remove superfluous stack operations after inlining */
static void RemoveUnnecessaryStackOps(t_bbl * callsite, t_bool fp)
{
  t_bbl * inlined;
  t_i386_ins * ins;

  ASSERT(BBL_SUCC_FIRST(callsite),("need outgoing edge"));
  ASSERT(BBL_SUCC_FIRST(callsite) == BBL_SUCC_LAST(callsite),("should only be one outgoing edge"));
  inlined = CFG_EDGE_TAIL(BBL_SUCC_FIRST(callsite));
  /* kill the stack instructions */
  ins = I386_INS_IPREV(T_I386_INS(BBL_INS_LAST(callsite)));
  ASSERT(I386_INS_OPCODE(ins) == I386_SUB,("did not expect @I",ins));
  ASSERT(I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP,("did not expect @I",ins));
  ASSERT(I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) == 4,("did not expect @I",ins));
  I386InsKill(ins);
  ins = I386_INS_IPREV(T_I386_INS(BBL_INS_LAST(inlined)));
  ASSERT(I386_INS_OPCODE(ins) == I386_ADD,("did not expect @I",ins));
  ASSERT(I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP,("did not expect @I",ins));
  if (I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) > 4)
    I386_OP_IMMEDIATE(I386_INS_SOURCE1(ins)) -= 4;
  else
    I386InsKill(ins);

  /* iterate through the inlined instructions and adapt stack offsets */
  BBL_FOREACH_I386_INS(inlined,ins)
  {
    long offset = (long) I386_INS_TMP(ins);
    t_i386_operand *src1 = I386_INS_SOURCE1(ins), *src2 = I386_INS_SOURCE2(ins), *dest = I386_INS_DEST(ins);
    if (I386_OP_TYPE(dest) == i386_optype_mem)
    {
      if (I386_OP_BASE(dest) == I386_REG_ESP)
      {
	if (I386_OP_IMMEDIATE(dest) > offset) I386_OP_IMMEDIATE(dest) -= 4;
	else if (I386_OP_IMMEDIATE(dest) == offset) FATAL(("function references return address on the stack"));
      }
      if (I386_OP_BASE(dest) == I386_REG_EBP)
      {
	if (I386_OP_IMMEDIATE(dest) > 4) I386_OP_IMMEDIATE(dest) -= 4;
	else if (I386_OP_IMMEDIATE(dest) == 4) FATAL(("function references return address on the stack"));
      }
    }
    if (I386_OP_TYPE(src1) == i386_optype_mem)
    {
      if (I386_OP_BASE(src1) == I386_REG_ESP)
      {
	if (I386_OP_IMMEDIATE(src1) > offset) I386_OP_IMMEDIATE(src1) -= 4;
	else if (I386_OP_IMMEDIATE(src1) == offset) FATAL(("function references return address on the stack"));
      }
      if (I386_OP_BASE(src1) == I386_REG_EBP)
      {
	if (I386_OP_IMMEDIATE(src1) > 4) I386_OP_IMMEDIATE(src1) -= 4;
	else if (I386_OP_IMMEDIATE(src1) == 4) FATAL(("function references return address on the stack"));
      }
    }
    if (I386_OP_TYPE(src2) == i386_optype_mem)
    {
      if (I386_OP_BASE(src2) == I386_REG_ESP)
      {
	if (I386_OP_IMMEDIATE(src2) > offset) I386_OP_IMMEDIATE(src2) -= 4;
	else if (I386_OP_IMMEDIATE(src2) == offset) FATAL(("function references return address on the stack"));
      }
      if (I386_OP_BASE(src2) == I386_REG_EBP)
      {
	if (I386_OP_IMMEDIATE(src2) > 4) I386_OP_IMMEDIATE(src2) -= 4;
	else if (I386_OP_IMMEDIATE(src2) == 4) FATAL(("function references return address on the stack"));
      }
    }
  }
}
/* }}} */
void I386RemoveUnnecessaryStackOps(t_bbl * callsite, t_bool fp)
{
  return RemoveUnnecessaryStackOps(callsite, fp);
}

/* {{{ inline simple functions of only one basic block */
#define MAX_NR_INS	2
void I386InlineSmallFunctions(t_cfg * cfg)
{
  t_function * fun;
  STATUS(START,("Inlining small functions"));
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_cfg_edge * edge, * tmp;
    t_bool simple, use_fp;
    
    if (!FUNCTION_BBL_FIRST(fun)) continue;
    if (BBL_NEXT_IN_FUN(FUNCTION_BBL_FIRST(fun)) != FUNCTION_BBL_LAST(fun)) continue;
    if (BBL_NINS(FUNCTION_BBL_LAST(fun)) != 0) continue;
    if (FUNCTION_IS_HELL(fun)) continue;
    
    /* avoid unreasonable expansion of the program: cap the number of
     * instructions in the function */
    if (BBL_NINS(FUNCTION_BBL_FIRST(fun)) > MAX_NR_INS) continue;

    if (!CheckInlinable(fun)) continue;

    simple = CheckFunctionSimpleness(fun,&use_fp);

    /*VERBOSE(0,("IS: will try to inline @iB\n",FUNCTION_BBL_FIRST(fun)));*/

    BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_FIRST(fun),edge,tmp)
    {
      t_bbl * callsite = CFG_EDGE_HEAD(edge);

      if (CFG_EDGE_CAT(edge) != ET_CALL) continue;
      if (BBL_IS_HELL(callsite)) continue;

      /*VERBOSE(0,("Inlining %s at call site @B\n", fun->name, callsite));*/
      if (I386InlineFunAtCallSite(fun,callsite))
      {
	/*printf("[ OK ]\n");*/
	if (simple) RemoveUnnecessaryStackOps(callsite,use_fp);
      }
      /*else*/
	/*printf("[FAIL]\n");*/
    }
    
  }
  STATUS(STOP,("Inlining small functions"));
} /* }}} */

/* {{{ inline functions with only one call site
 * until we have a good stack analysis, this will probably always increase code size :-( */
void I386InlineFunctionsWithOneCallSite(t_cfg * cfg)
{
  t_function * fun;
  t_int32 count = 0;

  STATUS(START,("Inlining functions with one call site"));
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_cfg_edge * edge, * call = NULL;
    int ncallers = 0;
    t_bool ok;

    if (FUNCTION_IS_HELL(fun)) continue;
    if (!FUNCTION_BBL_FIRST(fun)) continue;
    if (!CheckInlinable(fun)) continue;
    BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun),edge)
      if (CFG_EDGE_CAT(edge) == ET_CALL)
      {
	call = edge;
	if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
	  ncallers += 10;
	else
	  ncallers++;
      }
    if (ncallers != 1) continue;

    {
      t_bbl *ex = FunctionGetExitBlock(fun);
      t_cfg_edge * edge;
      int teller = 0;
      BBL_FOREACH_PRED_EDGE(ex,edge)
      {
	if (CFG_EDGE_CAT(edge) == ET_JUMP)
	  teller++;
      }
      if (teller == 1)
	VERBOSE(0,("ONLY ONE RETURN @B\n",FUNCTION_BBL_FIRST(fun)));
      else continue;
    }

    {
      ok = I386InlineFunAtCallSite(fun,CFG_EDGE_HEAD(call));
      if (!ok) printf("failed!\n");
      else 
      {
	//VERBOSE(0,("Inline %s in %s\n",FUNCTION_NAME(fun),FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_HEAD(call)))));
	count++;
      }
    }

  }
  VERBOSE(0,("inlined %d functions at one call site\n",count));
  STATUS(STOP,("Inlining functions with one call site"));
}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker : */
