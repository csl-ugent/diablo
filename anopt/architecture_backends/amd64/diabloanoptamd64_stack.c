#include <diabloanoptamd64.h>

t_dynamic_member_info bbl_stack_delta_array=null_info;
t_dynamic_member_info bbl_stack_in_array=null_info;
t_dynamic_member_info bbl_framepointer_offset_array=null_info;
t_dynamic_member_info bbl_set_framepointer_array=null_info;
t_dynamic_member_info bbl_reset_to_framepointer_array=null_info;
/* {{{ Amd64FunComputeStackSavedRegisters */
#define MAX_NR_REG  16

void Amd64FunComputeStackSavedRegisters(t_cfg * cfg, t_function * fun)
{
  int i;
  t_bool use_fp;
  t_bbl *entryblock, *exitblock, *iter;
  t_amd64_ins * ins;
  t_cfg_edge * edge;
  t_bool conservative = FALSE;
  t_bool ret_pops_only_return_address = TRUE;

  /* be conservative with respect to anything that looks even remotely weird */
  entryblock = FUNCTION_BBL_FIRST(fun);
  exitblock = FunctionGetExitBlock(fun);

  if (!exitblock) /* this can actually happen if the exitblock is killed because it was unreachable */
    conservative = TRUE;
  if (BBL_NINS(entryblock) == 0)
    conservative = TRUE;
  if (FUNCTION_IS_HELL(fun))
    conservative = TRUE;
  if (entryblock == exitblock)
    conservative = TRUE;
  FUNCTION_FOREACH_BBL(fun,iter)
    BBL_FOREACH_PRED_EDGE(iter,edge)
    {
      if (CfgEdgeIsForwardInterproc(edge) && CFG_EDGE_CAT(edge)!=ET_CALL)
	conservative = TRUE;
    }
  FUNCTION_FOREACH_BBL(fun,iter)
    BBL_FOREACH_SUCC_EDGE(iter,edge)
    {
      if (CfgEdgeIsForwardInterproc(edge) && CFG_EDGE_CAT(edge)!=ET_CALL)
	conservative = TRUE;
    }

  if (!conservative)
  {
    t_reg savedregs[MAX_NR_REG];
    t_reg loadedregs[MAX_NR_REG];
    int nsaved = 0;
    int nloaded = 0;
    t_regset changedregs;
    
    ins = T_AMD64_INS(BBL_INS_FIRST(entryblock));

    /* {{{ the entry block */
    if (!(AMD64_INS_OPCODE(ins) == AMD64_PUSH && 		/* push %rbp */
	  AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg && 
	  AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RBP))
      use_fp = FALSE;
    else
      use_fp = TRUE;

    if (use_fp)
    {
      ins = AMD64_INS_INEXT(ins);
      if (!(ins && AMD64_INS_OPCODE(ins) == AMD64_MOV &&		/* mov %rsp, %rbp */
	    AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	    AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg &&
	    AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP &&
	    AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP ))
      {
	conservative = TRUE;
      }
      if (ins) ins = AMD64_INS_INEXT(ins);
    }

    changedregs = NullRegs;
    for (; ins; ins = AMD64_INS_INEXT(ins))
    {
      t_amd64_operand * op = AMD64_INS_SOURCE1(ins);

      RegsetSetUnion(changedregs,AMD64_INS_REGS_DEF(ins));

      if (!Amd64InsIsStore(ins) && !RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP))
	continue;

      if (AMD64_INS_OPCODE(ins) != AMD64_PUSH) break;
      if (AMD64_OP_TYPE(op) != amd64_optype_reg) break;
      if (RegsetIn(changedregs,AMD64_OP_BASE(op))) break;

      savedregs[nsaved++] = AMD64_OP_BASE(op);
    }
    for (i = nsaved; i < MAX_NR_REG; i++)
      savedregs[i] = AMD64_REG_NONE;
    /* }}} */

    /* {{{ the exit blocks */
    nloaded = -1;
    BBL_FOREACH_PRED_EDGE(exitblock,edge)
    {
      int tmploaded = 0;

      iter = CFG_EDGE_HEAD(edge);
      
      if (BBL_NINS(iter) == 0)
      {
	conservative = TRUE;
	break;
      }

      ins = T_AMD64_INS(BBL_INS_LAST(iter));
      if (AMD64_INS_OPCODE(ins) != AMD64_RET && AMD64_INS_OPCODE(ins) != AMD64_IRET)
	FATAL(("procedure exit but no ret instruction: @iB\nexitblock @iB\nedge @E",iter,exitblock,edge));
      if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) != amd64_optype_none || AMD64_INS_OPCODE(ins) == AMD64_IRET)
	ret_pops_only_return_address = FALSE;

      ins = AMD64_INS_IPREV(ins);

      if (use_fp)
      {
	if (!(ins && 
	      (AMD64_INS_OPCODE(ins) == AMD64_LEAVE ||
	       (AMD64_INS_OPCODE(ins) == AMD64_POP && 
		AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg && 
		AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP))))
	{
	  conservative = TRUE;
	  nloaded = 0;
	  break;
	}
	if (ins) ins = AMD64_INS_IPREV(ins);
      }
      
      for (; ins && tmploaded != nloaded; ins = AMD64_INS_IPREV(ins))
      {
	t_amd64_operand * op = AMD64_INS_DEST(ins);

	if (AMD64_INS_OPCODE(ins) != AMD64_POP) break;
	if (AMD64_OP_TYPE(op) != amd64_optype_reg) break;
	
	if (nloaded == -1)
	{
	  loadedregs[tmploaded++] = AMD64_OP_BASE(op);
	}
	else
	{
	  if (loadedregs[tmploaded] == AMD64_OP_BASE(op))
	    tmploaded++;
	  else
	    break;
	}
      }
      if (nloaded == -1) 
	nloaded = tmploaded;
      else
	nloaded = (nloaded < tmploaded) ? nloaded : tmploaded;
    }
    for (i = nloaded; i < MAX_NR_REG; i++)
      loadedregs[i] = AMD64_REG_NONE;
    /* }}} */

    /* {{{ check if the saved & loaded registers match */
    FUNCTION_SET_REGS_SAVED(fun, NullRegs);
    for (i = 0; (i < nsaved) && (i < nloaded); i++)
    {
      if (savedregs[i] == loadedregs[i])
	FUNCTION_SET_REGS_SAVED(fun, RegsetAddReg(FUNCTION_REGS_SAVED(fun),savedregs[i]));
      else
	break;
   
    }
    if (use_fp)
      FUNCTION_SET_REGS_SAVED(fun, RegsetAddReg(FUNCTION_REGS_SAVED(fun),AMD64_REG_RBP));
    if (ret_pops_only_return_address)
      FUNCTION_SET_REGS_SAVED(fun, RegsetAddReg(FUNCTION_REGS_SAVED(fun),AMD64_REG_RSP));
    /* }}} */
  }

  if (conservative)
  {
    FUNCTION_SET_REGS_SAVED(fun, NullRegs);
  }

    
} /* }}} */

/* {{{ Amd64FramePointerAnalysis */
void Amd64FramePointerAnalysis(t_cfg * cfg)
{
  /* for each function in the cfg, check if it sets the frame pointer (%rbp),
   * and set the FF_HAS_FRAMEPOINTER flag accordingly. If we can't find out
   * whether the frame pointer is set, we assume it isn't set */
  t_function * fun;
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_bbl * first = FUNCTION_BBL_FIRST(fun);
    t_bbl * last = FunctionGetExitBlock(fun);
    t_amd64_ins * ins;
    t_bool fp_set_up = FALSE;
    t_bool fp_reset = TRUE;

    if (!first || !last)
    {
      FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) & (~FF_HAS_FRAMEPOINTER));
      continue;
    }

    /* is the frame pointer set up at the entry point? */
    {
      t_regset changed = NullRegs;
      t_bool push_ebp = FALSE, set_ebp = FALSE;
      BBL_FOREACH_AMD64_INS(first,ins)
      {
	if (!push_ebp && RegsetIn(changed,AMD64_REG_RBP))
	  break;
	if (RegsetIn(changed,AMD64_REG_RSP))	/* the stack changed induced by the push %rbp is not recorded in changed */
	  break;
	if (!push_ebp 
	    && AMD64_INS_OPCODE(ins) == AMD64_PUSH
	    && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg
	    && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RBP)
	{
	  push_ebp = TRUE;
	  continue;
	}
	if (push_ebp
	    && AMD64_INS_OPCODE(ins) == AMD64_MOV
	    && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg
	    && AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP
	    && AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg
	    && AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP)
	{
	  set_ebp = TRUE;
	  break;
	}
	RegsetSetUnion(changed,AMD64_INS_REGS_DEF(ins));
      }
      fp_set_up = set_ebp;
    }

    if (fp_set_up)
    {
      t_cfg_edge * edge;
      BBL_FOREACH_PRED_EDGE(last,edge)
      {
	t_bbl * head = CFG_EDGE_HEAD(edge);
	if (CFG_EDGE_CAT(edge) == ET_COMPENSATING) continue;
	ins = T_AMD64_INS(BBL_INS_LAST(head));
	if (!ins || AMD64_INS_OPCODE(ins) != AMD64_RET)
	{
	  fp_reset = FALSE;
	  break;
	}
	ins = AMD64_INS_IPREV(ins);
	if (ins && (AMD64_INS_OPCODE(ins) == AMD64_LEAVE || 
	      (AMD64_INS_OPCODE(ins) == AMD64_POP && AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP)))
	  continue;

	/* if we get here, we didn't see the instructions we expected */
	fp_reset = FALSE;
	break;
      }
    }

    if (fp_set_up && fp_reset)
      FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) | FF_HAS_FRAMEPOINTER);
    else
      FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun) & (~FF_HAS_FRAMEPOINTER));
  }
}

/* }}} */

int Amd64InsStackDelta(t_amd64_ins *ins, t_bool *unknown)
{
  t_amd64_operand *op;

  *unknown = FALSE;

  if (!RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP))
    return 0;

  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_PUSH:
    case AMD64_PUSHF:
      return 8;
    case AMD64_POP:
      if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg &&
	  AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RSP)
      {
	/* popped %rsp. stack delta is unknown. */
	*unknown = TRUE;
      }
      return -8;
    case AMD64_POPF:
      return -8;

    case AMD64_ADD:
    case AMD64_SUB:
      op = AMD64_INS_SOURCE1(ins);
      if (AMD64_OP_TYPE(op) == amd64_optype_imm)
	return AMD64_OP_IMMEDIATE(op) * (AMD64_INS_OPCODE(ins) == AMD64_ADD ? -1 : 1);

      *unknown = TRUE;
      return 0;

    case AMD64_CALL:
      return 8;
    case AMD64_RET:
      return -8;

    default:
      *unknown = TRUE;
      return 0;
  }
  return 0;
}

/* stack height analysis
 * ---------------------
 *
 * This analysis tries to determine for each basic block 
 * in a function the height of the stack at the beginning
 * of that block.
 *
 * The analysis is context-sensitive: in a first step, 
 * it tries to determine the stack delta caused by following
 * any interprocedural edge (this is the same for all edges
 * that have the same tail). This information is stored
 * in the edge->stack_delta field.
 *
 * The second step calculates the stack height at the beginning
 * each basic block by setting the stack height to 0 at every 
 * procedure entry point (i.e. all blocks where forward
 * interprocedural edges enter a function), and doing an 
 * intraprocedural fixpoint calculation using the information
 * on the outgoing interprocedural edges. The stack height
 * at the beginning of each basic block is stored in 
 * BBL_STACK_IN(bbl).
 *
 * The analysis tries to account for the use of a frame 
 * pointer, by detecting whether one is set at the function
 * entry and whether the stack pointer is reset to it
 * at the end of the function */

#define STACKBOT ((t_int32) 0x80000000)
#define STACKTOP ((t_int32) 0x7fffffff)

/* for each bbl, fill in following fields:
 * BBL_RESET_TO_FRAMEPOINTER(bbl): TRUE if the block resets the stack pointer
 * to the value stored in the frame pointer
 * BBL_SET_FRAMEPOINTER(bbl): TRUE if the block sets the frame pointer
 * BBL_FRAMEPOINTER_OFFSET(bbl): stack height at which the frame pointer points
 * relative to the beginning stack height at the beginning of the bbl
 * BBL_STACK_DELTA(bbl): if the stack pointer is not reset to the frame pointer,
 * this field contains the number of bytes the stack grows or shrinks in this
 * bbl. if the stack pointer _is_ reset to the frame pointer, this field 
 * indicates what the stack delta was _after_ this reset.
 */
/* {{{ ComputeStackDelta */

static void ComputeStackDelta(t_bbl * bbl)
{
  t_int32 delta = 0;
  t_amd64_ins * ins;

  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  FALSE);
  BBL_SET_SET_FRAMEPOINTER(bbl,  FALSE);
  
  if (BBL_IS_HELL(bbl) || IS_DATABBL(bbl))
  {
    BBL_SET_STACK_DELTA(bbl,  0);
    return;
  }

  BBL_FOREACH_AMD64_INS(bbl,ins)
  {
    t_amd64_operand * src = AMD64_INS_SOURCE1(ins);

    if (AMD64_INS_OPCODE(ins) == AMD64_MOV &&
	AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg && AMD64_OP_BASE(AMD64_INS_DEST(ins)) == AMD64_REG_RBP && 
	AMD64_OP_TYPE(src) == amd64_optype_reg && AMD64_OP_BASE(src) == AMD64_REG_RSP)
    {
      /* mov %esp, %rbp */
      BBL_SET_SET_FRAMEPOINTER(bbl,  TRUE);
      BBL_SET_FRAMEPOINTER_OFFSET(bbl,  delta);
    }
    
    if (!RegsetIn(AMD64_INS_REGS_DEF(ins),AMD64_REG_RSP)) continue;
    if (delta == STACKBOT)
    {
      if (AMD64_INS_OPCODE(ins) != AMD64_LEAVE 
	  && AMD64_INS_OPCODE(ins) != AMD64_LEA 
	  && AMD64_INS_OPCODE(ins) != AMD64_MOV) 
	continue;
    }

    switch (AMD64_INS_OPCODE(ins))
    {
      case AMD64_POP:
      case AMD64_POPF:
	delta -= 4;
	break;
      case AMD64_POPA:
	delta -= 32;
	break;
      case AMD64_PUSH:
      case AMD64_PUSHF:
	delta += 4;
	break;
      case AMD64_PUSHA:
	delta += 32;
	break;
      case AMD64_ADD:
	if (AMD64_OP_TYPE(src) == amd64_optype_imm)
	  delta -= AMD64_OP_IMMEDIATE(src);
	else
	  delta = STACKBOT;
	break;
      case AMD64_SUB:
	if (AMD64_OP_TYPE(src) == amd64_optype_imm)
	  delta += AMD64_OP_IMMEDIATE(src);
	else
	  delta = STACKBOT;
	break;
      case AMD64_MOV:
	if (AMD64_OP_TYPE(src) == amd64_optype_reg && AMD64_OP_BASE(src) == AMD64_REG_RBP)
	  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	delta = 0;
	break;
      case AMD64_LEAVE:
	BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	delta = -4;
	break;
      case AMD64_LEA:
	if (AMD64_OP_BASE(src) == AMD64_REG_RBP && AMD64_OP_INDEX(src) == AMD64_REG_NONE)
	{
	  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	  delta = -(t_int32)AMD64_OP_IMMEDIATE(src);
	}
	else
	  delta = STACKBOT;
	break;
      case AMD64_CALL:
      case AMD64_INT:
	/* for this analysis we act as if they don't change the stack pointer */
	break;
      case AMD64_RET:
	if (AMD64_OP_TYPE(src) == amd64_optype_imm)
	  delta -= AMD64_OP_IMMEDIATE(src);
	break;
      default:
	/*VERBOSE(0,("STACKBOTTING out for @I\n",ins));*/
	delta = STACKBOT;
    }
    /*if (delta == STACKBOT) VERBOSE(0,("STACKBOT for @I\n",ins));*/
  }
  BBL_SET_STACK_DELTA(bbl,  delta);
} /* }}} */

/* {{{ join functions */
static t_int32 Join(t_int32 a, t_int32 b)
{
  if (a == STACKBOT || b == STACKBOT) return STACKBOT;
  if (a == STACKTOP) return b;
  if (b == STACKTOP) return a;
  if (a == b) return a;
  return STACKBOT;
}
#if 0
static t_bool EdgeDeltaJoin(t_cfg_edge * edge, t_int32 delta)
{
  if (delta != CFG_EDGE_STACK_DELTA(edge))
  {
    CFG_EDGE_SET_STACK_DELTA(edge,  delta);
    return TRUE;
  }
  return FALSE;
}

static t_bool BlockDeltaJoin(t_bbl * bbl, t_int32 delta)
{
  delta = Join(BBL_STACK_IN(bbl),delta);
  if (delta != BBL_STACK_IN(bbl))
  {
    BBL_SET_STACK_IN(bbl,  delta);
    return TRUE;
  }
  return FALSE;
}
/* }}} */

/* {{{ function stack height propagation for a particular entry block */
/* after the fixpoint is reached, call this function with entry == NULL
 * to perform a final intraprocedural propagation that takes into account
 * all entry blocks at once */
static void DoRealFunctionProp(t_function * fun, t_bbl * entry)
{
  t_bbl * bbl;
  t_bbl * exit_block = FunctionGetExitBlock(fun);
  t_cfg_edge * edge;
  t_bool entry_sets_framepointer;
  t_int32 framepointer_offset;
  t_int32 fun_stack_delta;

  if (FUNCTION_IS_HELL(fun)) return;

  if (!entry)
  {
    /* not during fixpoint calculations - we don't care about frame pointers */
    entry_sets_framepointer = FALSE;
    framepointer_offset = 0;
  }
  else
  {
    entry_sets_framepointer = BBL_SET_FRAMEPOINTER(entry);
    framepointer_offset = BBL_FRAMEPOINTER_OFFSET(entry);
    /* {{{ is the framepointer set at function entry? */
    /* we only consider the frame pointer to be set when it is set in the entry block and nowhere else */
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      t_bool allowed_def = TRUE, found_allowed_def = FALSE;
      if (bbl == entry) continue;
      if (!RegsetIn(BBL_REGS_DEF(bbl),AMD64_REG_RBP)) continue;
      /* the only definitions of %rbp that are allowed are leave and pop %rbp in functions that reset the stack pointer to the frame pointer */
      if (BBL_RESET_TO_FRAMEPOINTER(bbl))
      {
	t_ins * ins;
	BBL_FOREACH_INS(bbl,ins)
	{
	  if (RegsetIn(INS_REGS_DEF(ins),AMD64_REG_RBP))
	  {
	    if (!found_allowed_def && (AMD64_INS_OPCODE(ins) == AMD64_LEAVE ||
		  (AMD64_INS_OPCODE(ins) == AMD64_POP && INS_IPREV(ins) && AMD64_INS_OPCODE(INS_IPREV(ins)) == AMD64_MOV &&
		   AMD64_OP_TYPE(AMD64_INS_DEST(INS_IPREV(ins))) == amd64_optype_reg && AMD64_OP_BASE(AMD64_INS_DEST(INS_IPREV(ins))) == AMD64_REG_RSP &&
		   AMD64_OP_TYPE(AMD64_INS_SOURCE1(INS_IPREV(ins))) == amd64_optype_reg && AMD64_OP_BASE(AMD64_INS_SOURCE1(INS_IPREV(ins))) == AMD64_REG_RBP)))
	      found_allowed_def = TRUE;
	    else
	      allowed_def = FALSE;
	  }
	}
      }
      if (!allowed_def)
	entry_sets_framepointer = FALSE;
    }
    /* }}} */
  }

  /* {{{ initialization */
  FUNCTION_FOREACH_BBL(fun,bbl)
    BBL_SET_STACK_IN(bbl,  STACKTOP);
  if (entry)
  {
    BBL_SET_STACK_IN(entry,  0);
    FunctionMarkBbl(fun,entry);
  }
  else
  {
    /* mark all blocks with incoming forward interprocedural edges */
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      BBL_FOREACH_PRED_EDGE(bbl,edge)
	if (CfgEdgeIsForwardInterproc(edge))
	  break;
      if (edge)
      {
	BBL_SET_STACK_IN(bbl,  0);
	FunctionMarkBbl(fun,bbl);
      }
    }
  } /* }}} */

  while (FunctionUnmarkBbl(fun,&bbl))
  {
    /* real propagation */
    t_int32 stack_out;
    t_cfg_edge * edge;

    /* {{{ determine stack_out */
    if (BBL_RESET_TO_FRAMEPOINTER(bbl))
    {
      if (entry_sets_framepointer)
	stack_out = framepointer_offset + BBL_STACK_DELTA(bbl);
      else
	stack_out = STACKBOT;
    }
    else
    {
      stack_out = (BBL_STACK_IN(bbl) == STACKBOT || BBL_STACK_DELTA(bbl) == STACKBOT) ? STACKBOT : (BBL_STACK_IN(bbl) + BBL_STACK_DELTA(bbl));
    } /* }}} */

    /* propagation over non-interprocedural edges */
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if (CfgEdgeIsInterproc(edge))
      if (BlockDeltaJoin(CFG_EDGE_TAIL(edge),stack_out))
	FunctionMarkBbl(fun,CFG_EDGE_TAIL(edge));
    }

    /* propagation over interprocedural edges (but ignore swi-edges, syscalls don't change the stack) */
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);
      t_bbl * next;
      t_int32 tmp_stack_out;

      if (!CfgEdgeIsForwardInterproc(edge) || CFG_EDGE_CAT(edge) == ET_SWI) continue;

      if (!corr_edge) FATAL(("expected corresponding edge for @E",edge));
      next = CFG_EDGE_TAIL(corr_edge);
      if (BBL_FUNCTION(next) != fun)
      {
	VERBOSE(0,("!xx!!xx! vind een oplossing voor interprocedurale funlinks\n"));
	continue;
      }

      tmp_stack_out = (stack_out == STACKBOT || CFG_EDGE_STACK_DELTA(edge) == STACKBOT) ? STACKBOT : stack_out + CFG_EDGE_STACK_DELTA(edge);
      if (BlockDeltaJoin(next,tmp_stack_out))
	FunctionMarkBbl(fun,next);
    }
  }

  /* determine fun_stack_delta */
  if (entry)
  {
    if (!exit_block || (BBL_ATTRIB(exit_block) & BBL_UNREACHABLE))
    {
      /* assume that functions that do not return could just as well be considered to be well-behaved */
      fun_stack_delta = 0;
    }
    else if (BBL_STACK_IN(exit_block) == STACKTOP)
    {
      /* this means the exit block of the function is not reachable for this given entry point:
       * entering the function via this point means it will never return... */
      fun_stack_delta = 0;
    }
    else
    {
      fun_stack_delta = BBL_STACK_IN(exit_block);
    }

    BBL_FOREACH_PRED_EDGE(entry,edge)
    {
      if (!CfgEdgeIsForwardInterproc(edge)) continue;
      if (EdgeDeltaJoin(edge,fun_stack_delta))
	CfgMarkFun(FUNCTION_CFG(fun),BBL_FUNCTION(CFG_EDGE_HEAD(edge)));
    }
  }
}
/* }}} */

static void FunctionStackHeightAnalysis(t_function * fun)
{
  t_bbl * entry;
  t_cfg_edge * edge;

  FUNCTION_FOREACH_BBL(fun,entry)
  {
    BBL_FOREACH_PRED_EDGE(entry,edge)
      if (CfgEdgeIsForwardInterproc(edge))
	break;
    if (!edge) continue;

    /* propagate function for this entry block */
    DoRealFunctionProp(fun,entry);
  }
}
#endif
#if 0
void Amd64StackHeightAnalysis(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_cfg_edge * edge;
  /*int nfunc = 0, nzero = 0, nbot = 0, nother = 0;*/
  /*t_bool bot,zero,other;*/
  /*t_bool multi_entry;*/


  /* initialize */
  EdgeMarkInit();
  CfgUnmarkAllFun(cfg);
  CFG_FOREACH_FUN(cfg,fun)
  {
    FunctionUnmarkAllEdges(fun);
    FunctionUnmarkAllBbls(fun);
  }
  CFG_FOREACH_BBL(cfg,bbl)
    ComputeStackDelta(bbl);
  CFG_FOREACH_EDGE(cfg,edge)
    CFG_EDGE_SET_STACK_DELTA(edge,  0);

  BBL_FOREACH_PRED_EDGE(CFG_HELL_NODE(cfg),edge)
    CFG_EDGE_SET_STACK_DELTA(edge,  STACKBOT);
  FATAL(("adapt for multiple call hell nodes (dynamic linking)"));
  BBL_FOREACH_PRED_EDGE(CFG_CALL_HELL_NODE(cfg),edge)
    CFG_EDGE_SET_STACK_DELTA(edge,  STACKBOT);
  
  /* start propagation */
  CFG_FOREACH_FUN(cfg,fun)
  {
    if (fun != CFG_CALL_HELL_FUNCTION(cfg) && fun != CFG_HELL_FUNCTION(cfg))
      CfgMarkFun(cfg,fun);
  }

  while (CfgUnmarkFun(cfg,&fun))
    FunctionStackHeightAnalysis(fun);

  /* propagate final information into all basic blocks */
  CFG_FOREACH_FUN(cfg,fun)
    DoRealFunctionProp(fun,NULL);
}
#endif

/* vim: set shiftwidth=2 foldmethod=marker: */
