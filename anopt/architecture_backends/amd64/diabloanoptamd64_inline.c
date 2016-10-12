#include <diabloanoptamd64.h>

/* {{{ check whether it is possible to inline the function */
static t_bool CheckInlinable(t_function * fun)
{
  t_cfg_edge * edge;
  t_bbl * retblock = FunctionGetExitBlock(fun);

  if (!retblock) return FALSE;
  /* incoming compensating edges => outgoing interprocedural edges => don't
   * inline the function */
  BBL_FOREACH_PRED_EDGE(retblock,edge)
    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING) return FALSE;
  /* outgoing compensating edges => incoming interprocedural edges => don't
   * inline the function */
  BBL_FOREACH_SUCC_EDGE(retblock,edge)
    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING) return FALSE;

  return TRUE;
} /* }}} */

/*TODO: this is in fact architecture-independent */
/* {{{ copy the structure of the function we want to inline */
static void CopyFunctionStructureToCallSite(t_function * fun, t_bbl * call_site)
{
  t_bbl * i_bbl;
  t_ins * i_ins;
  t_cfg_edge * i_edge;
  t_cfg * cfg = FUNCTION_CFG(fun);

  /* step 1: duplicate all basic blocks of the function */
  FUNCTION_FOREACH_BBL(fun,i_bbl)
  {
    t_bbl * copy;

    /* don't copy the return block */
    if (i_bbl == FunctionGetExitBlock(fun))
      continue;

    copy = BblNew(cfg);
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
/*	FATAL(("Implement inlining of functions with interprocedural jumps or fallthroughs"));*/
	VERBOSE(0,("Implement inlining of functions with interprocedural jumps or fallthroughs"));
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
    if (AMD64_INS_OPCODE(T_AMD64_INS(BBL_INS_LAST(i_bbl)))!=AMD64_JMP)continue;

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
      t_reloc * switch_rel=RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(i_bbl)));

      RelocTableRemoveReloc (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),RELOC_REF_RELOC(INS_REFERS_TO(BBL_INS_LAST(BBL_TMP(i_bbl)))));

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
t_bool Amd64InlineFunAtCallSite(t_function * fun, t_bbl * callsite)
{
  t_cfg_edge * ecall, * eret = NULL, * i_edge;
  t_bbl * bentry = FUNCTION_BBL_FIRST(fun);
  t_bbl * bexit = FUNCTION_BBL_LAST(fun);
  t_bbl * returnsite = NULL;
  t_amd64_ins * cjmp, * rjmp, * sub;
  t_cfg * cfg = BBL_CFG(callsite);
  t_bool returning;

  /*if (!CheckInlinable(fun)) return FALSE;*/

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
/*	  FATAL(("Implement inlining of functions with interprocedural jumps or fallthroughs"));*/
	  VERBOSE(0,("Implement inlining of functions with interprocedural jumps or fallthroughs"));
	}
      }
    }
  }

  /* copy the structure of the function into the caller. */
  CopyFunctionStructureToCallSite(fun,callsite);

  /* change the call instruction into a jump + move the stack pointer 8 bytes
   * down. (this is necessary to keep all stack references in the inlined
   * function correct) */
  InsKill(BBL_INS_LAST(callsite));
  Amd64MakeInsForBbl(Arithmetic,Append,sub,callsite,AMD64_SUB,AMD64_REG_RSP,AMD64_REG_NONE,8);
  Amd64MakeInsForBbl(Jump,Append,cjmp,callsite);
  CfgEdgeCreate(cfg,callsite,T_BBL(BBL_TMP(bentry)),ET_JUMP);
  
  /* change the return instructions into jumps + move the stack pointer 8
   * bytes up */
  if(returning)
    BBL_FOREACH_PRED_EDGE(bexit,i_edge)
    {
      t_bbl * bret = BBL_TMP(CFG_EDGE_HEAD(i_edge));
      t_amd64_ins * ret = T_AMD64_INS(BBL_INS_LAST(bret));
      int popbytes = 8;
      ASSERT(CFG_EDGE_CAT(i_edge) == ET_JUMP,("flow graph corrupt"));
      ASSERT(AMD64_INS_OPCODE(ret) == AMD64_RET,("weird return instruction @I",ret));

      if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ret)) == amd64_optype_imm)
	popbytes += AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ret));
      Amd64InstructionMakeArithmetic(ret,AMD64_ADD,AMD64_REG_RSP,AMD64_REG_NONE,popbytes);
      Amd64MakeInsForBbl(Jump,Append,rjmp,bret);
      CfgEdgeCreate(cfg,bret,returnsite,ET_JUMP);
    }

  /* clean up the old edges */
  CfgEdgeKill(ecall);
  
  if(returning)
    CfgEdgeKill(eret);
  
  return TRUE;
} /* }}} */

/* {{{ helper function: determines whether it is necessary to keep the 
 * function's stack behaviour exactly the same before and after inlining */
/* store the stack offset in the tmp field of every instruction */
static t_bool CheckFunctionSimpleness(t_function * fun, t_bool * fp_ret)
{
  t_bbl * bbl = FUNCTION_BBL_FIRST(fun);
  t_amd64_ins * ins;
  t_bool simple = TRUE;
  long offset = 0;
  t_bool fp = FALSE;
  t_uint32 fp_offset = 0;

  ins = T_AMD64_INS(BBL_INS_FIRST(bbl));
  /* if the first block of tile instruction is empty, something funny is going on - just be conservative */
  if (!ins) return FALSE;

  /* does the function use a frame pointer? */
  if (AMD64_INS_OPCODE(ins) == AMD64_PUSH && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RBP)
  {
    AMD64_INS_SET_TMP(ins,  (void*)offset);
    ins = AMD64_INS_INEXT(ins);
    offset += 8;

    if (ins && AMD64_INS_OPCODE(ins) == AMD64_MOV && 
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP &&
	AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg && AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP)
    {
      AMD64_INS_SET_TMP(ins,  (void*) offset);
      fp = TRUE;
      fp_offset = 8;
      ins = AMD64_INS_INEXT(ins);
    }
  }

  for (; ins; ins = AMD64_INS_INEXT(ins))
  {
    AMD64_INS_SET_TMP(ins,  (void*) offset);

    /* skip instructions that don't influence the stack */
    if (!RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP) && !RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP) &&
	(!fp || (!RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RBP) && !RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RBP))))
      continue;

    switch (AMD64_INS_OPCODE(ins))
    {
      case AMD64_PUSH:
      case AMD64_PUSHF:
	offset += 8;
	break;
      case AMD64_POP:
      case AMD64_POPF:
	offset -= 8;
	break;

      case AMD64_MOV:
	{
	  t_amd64_operand * src = AMD64_INS_SOURCE1(ins), * dest = AMD64_INS_DEST(ins);

	  if (AMD64_OP_TYPE(src) == amd64_optype_reg)
	    if (AMD64_OP_BASE(src) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(src) == AMD64_REG_RBP))
	    {
	      /* danger of aliasing */
	      simple = FALSE;
	      break;
	    }
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg)
	    if (AMD64_OP_BASE(dest) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(dest) == AMD64_REG_RBP))
	    {
	      /* we don't know the offsets in the stack frame any more */
	      simple = FALSE;
	      break;
	    }

	  /* load and store relative to esp or ebp without index register are well understood and
	   * ok. all other cases fail the test. */
	  if (AMD64_OP_TYPE(dest) == amd64_optype_mem && AMD64_OP_INDEX(dest) == AMD64_REG_NONE && 
	      (AMD64_OP_BASE(dest) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(dest) == AMD64_REG_RBP)))
	  { /* ok */ }
	  else if (AMD64_OP_TYPE(src) == amd64_optype_mem && AMD64_OP_INDEX(src) == AMD64_REG_NONE &&
	      (AMD64_OP_BASE(src) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(src) == AMD64_REG_RBP)))
	  { /* ok */ }
	  else
	  {
	    /* not ok */
	    simple = FALSE;
	    break;
	  }
	}
	break;

      case AMD64_ADD:
	{
	  t_amd64_operand * src = AMD64_INS_SOURCE1(ins), * dest = AMD64_INS_DEST(ins);
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg)
	  {
	    if (AMD64_OP_BASE(dest) == AMD64_REG_RSP)
	    {
	      if (AMD64_OP_TYPE(src) == amd64_optype_imm)
		offset -= AMD64_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (fp && AMD64_OP_BASE(dest) == AMD64_REG_RBP)
	    {
	      if (AMD64_OP_TYPE(src) == amd64_optype_imm)
		fp_offset -= AMD64_OP_IMMEDIATE(src);
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

      case AMD64_SUB:
	{
	  t_amd64_operand * src = AMD64_INS_SOURCE1(ins), * dest = AMD64_INS_DEST(ins);
	  if (AMD64_OP_TYPE(dest) == amd64_optype_reg)
	  {
	    if (AMD64_OP_BASE(dest) == AMD64_REG_RSP)
	    {
	      if (AMD64_OP_TYPE(src) == amd64_optype_imm)
		offset += AMD64_OP_IMMEDIATE(src);
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (fp && AMD64_OP_BASE(dest) == AMD64_REG_RBP)
	    {
	      if (AMD64_OP_TYPE(src) == amd64_optype_imm)
		fp_offset += AMD64_OP_IMMEDIATE(src);
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

      case AMD64_RET:
	/* ok */
	break;

      default:
	/* ok as long as stack pointer (and frame pointer) are only used
	 * as base register in memory operands */
	{
	  t_amd64_operand * src1 = AMD64_INS_SOURCE1(ins), * src2 = AMD64_INS_SOURCE2(ins),* dest = AMD64_INS_DEST(ins);

	  if (!RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP) && (!fp || (!RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RBP))))
	  {
	    if (AMD64_OP_BASE(dest) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(dest) == AMD64_REG_RBP))
	    {
	      if (AMD64_OP_TYPE(dest) == amd64_optype_mem && AMD64_OP_INDEX(dest) == AMD64_REG_NONE)
	      { /* ok */ }
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (AMD64_OP_BASE(src1) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(src1) == AMD64_REG_RBP))
	    {
	      if (AMD64_OP_TYPE(src1) == amd64_optype_mem && AMD64_OP_INDEX(src1) == AMD64_REG_NONE)
	      { /* ok */ }
	      else
	      {
		simple = FALSE;
		break;
	      }
	    }
	    if (AMD64_OP_BASE(src2) == AMD64_REG_RSP || (fp && AMD64_OP_BASE(src2) == AMD64_REG_RBP))
	    {
	      if (AMD64_OP_TYPE(src2) == amd64_optype_mem && AMD64_OP_INDEX(src2) == AMD64_REG_NONE)
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

    if (fp && fp_offset != 8) simple = FALSE;
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

/* {{{ helper function: remove superfluous stack operations after inlining */
static void RemoveUnnecessaryStackOps(t_bbl * callsite, t_bool fp)
{
  t_bbl * inlined;
  t_amd64_ins * ins;

  ASSERT(BBL_SUCC_FIRST(callsite),("need outgoing edge"));
  ASSERT(BBL_SUCC_FIRST(callsite) == BBL_SUCC_LAST(callsite),("should only be one outgoing edge"));
  inlined = CFG_EDGE_TAIL(BBL_SUCC_FIRST(callsite));
  /* kill the stack instructions */
  ins = AMD64_INS_IPREV(T_AMD64_INS(BBL_INS_LAST(callsite)));
  ASSERT(AMD64_INS_OPCODE(ins) == AMD64_SUB,("did not expect @I",ins));
  ASSERT(AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP,("did not expect @I",ins));
  ASSERT(AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) == 8,("did not expect @I",ins));
  Amd64InsKill(ins);
  ins = AMD64_INS_IPREV(T_AMD64_INS(BBL_INS_LAST(inlined)));
  ASSERT(AMD64_INS_OPCODE(ins) == AMD64_ADD,("did not expect @I",ins));
  ASSERT(AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP,("did not expect @I",ins));
  if (AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) > 8)
    AMD64_OP_IMMEDIATE(AMD64_INS_SOURCE1(ins)) -= 8;
  else
    Amd64InsKill(ins);

  /* iterate through the inlined instructions and adapt stack offsets */
  BBL_FOREACH_AMD64_INS(inlined,ins)
  {
    long offset = (long) AMD64_INS_TMP(ins);
    t_amd64_operand *src1 = AMD64_INS_SOURCE1(ins), *src2 = AMD64_INS_SOURCE2(ins), *dest = AMD64_INS_DEST(ins);
    if (AMD64_OP_TYPE(dest) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(dest) == AMD64_REG_RSP)
      {
	if (AMD64_OP_IMMEDIATE(dest) > offset) AMD64_OP_IMMEDIATE(dest) -= 4;
	else if (AMD64_OP_IMMEDIATE(dest) == offset) FATAL(("function references return address on the stack"));
      }
      if (AMD64_OP_BASE(dest) == AMD64_REG_RBP)
      {
	if (AMD64_OP_IMMEDIATE(dest) > 4) AMD64_OP_IMMEDIATE(dest) -= 4;
	else if (AMD64_OP_IMMEDIATE(dest) == 4) FATAL(("function references return address on the stack"));
      }
    }
    if (AMD64_OP_TYPE(src1) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(src1) == AMD64_REG_RSP)
      {
	if (AMD64_OP_IMMEDIATE(src1) > offset) AMD64_OP_IMMEDIATE(src1) -= 4;
	else if (AMD64_OP_IMMEDIATE(src1) == offset) FATAL(("function references return address on the stack"));
      }
      if (AMD64_OP_BASE(src1) == AMD64_REG_RBP)
      {
	if (AMD64_OP_IMMEDIATE(src1) > 4) AMD64_OP_IMMEDIATE(src1) -= 4;
	else if (AMD64_OP_IMMEDIATE(src1) == 4) FATAL(("function references return address on the stack"));
      }
    }
    if (AMD64_OP_TYPE(src1) == amd64_optype_mem)
    {
      if (AMD64_OP_BASE(src2) == AMD64_REG_RSP)
      {
	if (AMD64_OP_IMMEDIATE(src2) > offset) AMD64_OP_IMMEDIATE(src2) -= 4;
	else if (AMD64_OP_IMMEDIATE(src2) == offset) FATAL(("function references return address on the stack"));
      }
      if (AMD64_OP_BASE(src2) == AMD64_REG_RBP)
      {
	if (AMD64_OP_IMMEDIATE(src2) > 4) AMD64_OP_IMMEDIATE(src2) -= 4;
	else if (AMD64_OP_IMMEDIATE(src2) == 4) FATAL(("function references return address on the stack"));
      }
    }
  }
}
/* }}} */

/* {{{ inline simple functions of only one basic block */
#define MAX_NR_INS	2
/*int total_count=0;*/
void Amd64InlineSmallFunctions(t_cfg * cfg)
{
  t_function * fun;
  STATUS(START,("Inlining small functions"));
  CFG_FOREACH_FUN(cfg,fun)
  {
/*    if (total_count < diablosupport_options.debugcounter)
    {
      total_count++;
    }else{
      return;
    }*/
    
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
      if (Amd64InlineFunAtCallSite(fun,callsite))
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
void Amd64InlineFunctionsWithOneCallSite(t_cfg * cfg)
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
      if (teller == 1){
/*	VERBOSE(0,("ONLY ONE RETURN @B\n",FUNCTION_BBL_FIRST(fun)));*/
      }
      else continue;
    }

    {
      ok = Amd64InlineFunAtCallSite(fun,CFG_EDGE_HEAD(call));
      if (!ok) printf("failed!\n");
      else 
      {
	/*VERBOSE(0,("Inline %s in %s\n",FUNCTION_NAME(fun),FUNCTION_NAME(BBL_FUNCTION(CFG_EDGE_HEAD(call)))));*/
	count++;
      }
    }

  }
  VERBOSE(0,("inlined %d functions at one call site\n",count));
  STATUS(STOP,("Inlining functions with one call site"));
}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker : */
