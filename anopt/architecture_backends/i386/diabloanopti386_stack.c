/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloanopti386.h>

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(stack_delta);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(stack_in);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(framepointer_offset);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(set_framepointer);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(reset_to_framepointer);

/* {{{ I386FunComputeStackSavedRegisters */
#define MAX_NR_REG  16

/* this analysis is basically unsafe because it optimistically assumes that
 * "saved" stack slots will never be used for anything else (e.g. parameter
 * passing). It turns out that this assumption is wrong in the Linux kernel
 * (__down_failed_trylock), but we can still assume that no compiler would
 * generate this code. The one case in which things go wrong is effectively
 * in hand-written assembler code. So we act conservatively if the function
 * prologue or epilogue is written in assembler */
void I386FunComputeStackSavedRegisters(t_cfg * cfg, t_function * fun)
{
  int i;
  t_bool use_fp;
  t_bbl *entryblock, *exitblock, *iter;
  t_i386_ins * ins;
  t_cfg_edge * edge;
  t_bool conservative = FALSE;
  t_bool ret_pops_only_return_address = TRUE;

  /* ugly hack only there for kdiablo - should be removed in time */
  if (!diabloanopt_options.do_compute_savedchanged_regs)
    conservative = TRUE;

  /* be conservative with respect to anything that looks even remotely weird */
  entryblock = FUNCTION_BBL_FIRST(fun);
  exitblock = FunctionGetExitBlock(fun);

  if (BBL_ATTRIB (entryblock) & BBL_IS_HANDWRITTEN_ASSEMBLY)
    conservative = TRUE;
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

    ins = T_I386_INS(BBL_INS_FIRST(entryblock));

    /* {{{ the entry block */
    if (!(I386_INS_OPCODE(ins) == I386_PUSH && 		/* push %ebp */
	  I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg && 
	  I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP))
      use_fp = FALSE;
    else
      use_fp = TRUE;

    if (use_fp)
    {
      ins = I386_INS_INEXT(ins);
      if (!(ins && I386_INS_OPCODE(ins) == I386_MOV &&		/* mov %esp, %ebp */
	    I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	    I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	    I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP &&
	    I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP ))
      {
	conservative = TRUE;
      }
      if (ins) ins = I386_INS_INEXT(ins);
    }

    changedregs = NullRegs;
    for (; ins; ins = I386_INS_INEXT(ins))
    {
      t_i386_operand * op = I386_INS_SOURCE1(ins);

      RegsetSetUnion(changedregs,I386_INS_REGS_DEF(ins));

      if (!I386InsIsStore(ins) && !RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP))
	continue;

      if (I386_INS_OPCODE(ins) != I386_PUSH) break;
      if (I386_OP_TYPE(op) != i386_optype_reg) break;
      if (RegsetIn(changedregs,I386_OP_BASE(op))) break;

      savedregs[nsaved++] = I386_OP_BASE(op);
    }
    for (i = nsaved; i < MAX_NR_REG; i++)
      savedregs[i] = I386_REG_NONE;
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
      if (BBL_ATTRIB (iter) & BBL_IS_HANDWRITTEN_ASSEMBLY)
      {
	conservative = TRUE;
	break;
      }

      ins = T_I386_INS(BBL_INS_LAST(iter));
      if (I386_INS_OPCODE(ins) != I386_RET && I386_INS_OPCODE(ins) != I386_IRET)
	FATAL(("procedure exit but no ret instruction: @iB\nexitblock @iB\nedge @E",iter,exitblock,edge));
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) != i386_optype_none || I386_INS_OPCODE(ins) == I386_IRET)
	ret_pops_only_return_address = FALSE;

      ins = I386_INS_IPREV(ins);

      if (use_fp)
      {
	if (!(ins && 
	      (I386_INS_OPCODE(ins) == I386_LEAVE ||
	       (I386_INS_OPCODE(ins) == I386_POP && 
		I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg && 
		I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP))))
	{
	  conservative = TRUE;
	  nloaded = 0;
	  break;
	}
	if (ins) ins = I386_INS_IPREV(ins);
      }
      
      for (; ins && tmploaded != nloaded; ins = I386_INS_IPREV(ins))
      {
	t_i386_operand * op = I386_INS_DEST(ins);

	if (I386_INS_OPCODE(ins) != I386_POP) break;
	if (I386_OP_TYPE(op) != i386_optype_reg) break;
	
	if (nloaded == -1)
	{
	  loadedregs[tmploaded++] = I386_OP_BASE(op);
	}
	else
	{
	  if (loadedregs[tmploaded] == I386_OP_BASE(op))
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

    nloaded = MAXIMUM(nloaded, 0); // Otherwise we'd write at loadedregs[-1]!

    for (i = nloaded; i < MAX_NR_REG; i++)
      loadedregs[i] = I386_REG_NONE;
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
      FUNCTION_SET_REGS_SAVED(fun, RegsetAddReg(FUNCTION_REGS_SAVED(fun),I386_REG_EBP));
    if (ret_pops_only_return_address)
      FUNCTION_SET_REGS_SAVED(fun, RegsetAddReg(FUNCTION_REGS_SAVED(fun),I386_REG_ESP));
    /* }}} */
  }

  /*static int count = 0;*/
  if (!conservative)
  {
    /*if (count++ >= diablosupport_options.debugcounter)*/
      conservative = TRUE;
    /*else*/
      /*VERBOSE (0, ("Computed for @B: @X", FUNCTION_BBL_FIRST (fun),*/
	    /*CPREGSET (cfg, FUNCTION_REGS_SAVED (fun))));*/
  }

  if (conservative)
  {
    FUNCTION_SET_REGS_SAVED(fun, NullRegs);
  }

    
} /* }}} */

/* {{{ I386FramePointerAnalysis */
void I386FramePointerAnalysis(t_cfg * cfg)
{
  /* for each function in the cfg, check if it sets the frame pointer (%ebp),
   * and set the FF_HAS_FRAMEPOINTER flag accordingly. If we can't find out
   * whether the frame pointer is set, we assume it isn't set */
  t_function * fun;
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_bbl * first = FUNCTION_BBL_FIRST(fun);
    t_bbl * last = FunctionGetExitBlock(fun);
    t_i386_ins * ins;
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
      BBL_FOREACH_I386_INS(first,ins)
      {
	if (!push_ebp && RegsetIn(changed,I386_REG_EBP))
	  break;
	if (RegsetIn(changed,I386_REG_ESP))	/* the stack changed induced by the push %ebp is not recorded in changed */
	  break;
	if (!push_ebp 
	    && I386_INS_OPCODE(ins) == I386_PUSH
	    && I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg
	    && I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_EBP)
	{
	  push_ebp = TRUE;
	  continue;
	}
	if (push_ebp
	    && I386_INS_OPCODE(ins) == I386_MOV
	    && I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg
	    && I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP
	    && I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg
	    && I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP)
	{
	  set_ebp = TRUE;
	  break;
	}
	RegsetSetUnion(changed,I386_INS_REGS_DEF(ins));
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
	ins = T_I386_INS(BBL_INS_LAST(head));
	if (!ins || I386_INS_OPCODE(ins) != I386_RET)
	{
	  fp_reset = FALSE;
	  break;
	}
	ins = I386_INS_IPREV(ins);
	if (ins && (I386_INS_OPCODE(ins) == I386_LEAVE || 
	      (I386_INS_OPCODE(ins) == I386_POP && I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP)))
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

int I386InsStackDelta(t_i386_ins *ins, t_bool *unknown)
{
  t_i386_operand *op;

  *unknown = FALSE;

  if (!RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP))
    return 0;

  switch (I386_INS_OPCODE(ins))
  {
    case I386_PUSH:
    case I386_PUSHF:
      return 4;
    case I386_POP:
      if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg &&
	  I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_ESP)
      {
	/* popped %esp. stack delta is unknown. */
	*unknown = TRUE;
      }
      return -4;
    case I386_POPF:
      return -4;
    case I386_POPA:
      return -32;
    case I386_PUSHA:
      return 32;

    case I386_ADD:
    case I386_SUB:
      op = I386_INS_SOURCE1(ins);
      if (I386_OP_TYPE(op) == i386_optype_imm)
	return I386_OP_IMMEDIATE(op) * (I386_INS_OPCODE(ins) == I386_ADD ? -1 : 1);

      *unknown = TRUE;
      return 0;

    case I386_CALL:
      return 4;
    case I386_RET:
      return -4;

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
  t_i386_ins * ins;

  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  FALSE);
  BBL_SET_SET_FRAMEPOINTER(bbl,  FALSE);
  
  if (BBL_IS_HELL(bbl) || IS_DATABBL(bbl))
  {
    BBL_SET_STACK_DELTA(bbl,  0);
    return;
  }

  BBL_FOREACH_I386_INS(bbl,ins)
  {
    t_i386_operand * src = I386_INS_SOURCE1(ins);

    if (I386_INS_OPCODE(ins) == I386_MOV &&
	I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg && I386_OP_BASE(I386_INS_DEST(ins)) == I386_REG_EBP && 
	I386_OP_TYPE(src) == i386_optype_reg && I386_OP_BASE(src) == I386_REG_ESP)
    {
      /* mov %esp, %ebp */
      BBL_SET_SET_FRAMEPOINTER(bbl,  TRUE);
      BBL_SET_FRAMEPOINTER_OFFSET(bbl,  delta);
    }
    
    if (!RegsetIn(I386_INS_REGS_DEF(ins),I386_REG_ESP)) continue;
    if (delta == STACKBOT)
    {
      if (I386_INS_OPCODE(ins) != I386_LEAVE 
	  && I386_INS_OPCODE(ins) != I386_LEA 
	  && I386_INS_OPCODE(ins) != I386_MOV) 
	continue;
    }

    switch (I386_INS_OPCODE(ins))
    {
      case I386_POP:
      case I386_POPF:
	delta -= 4;
	break;
      case I386_POPA:
	delta -= 32;
	break;
      case I386_PUSH:
      case I386_PUSHF:
	delta += 4;
	break;
      case I386_PUSHA:
	delta += 32;
	break;
      case I386_ADD:
	if (I386_OP_TYPE(src) == i386_optype_imm)
	  delta -= I386_OP_IMMEDIATE(src);
	else
	  delta = STACKBOT;
	break;
      case I386_SUB:
	if (I386_OP_TYPE(src) == i386_optype_imm)
	  delta += I386_OP_IMMEDIATE(src);
	else
	  delta = STACKBOT;
	break;
      case I386_MOV:
	if (I386_OP_TYPE(src) == i386_optype_reg && I386_OP_BASE(src) == I386_REG_EBP)
	  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	delta = 0;
	break;
      case I386_LEAVE:
	BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	delta = -4;
	break;
      case I386_LEA:
	if (I386_OP_BASE(src) == I386_REG_EBP && I386_OP_INDEX(src) == I386_REG_NONE)
	{
	  BBL_SET_RESET_TO_FRAMEPOINTER(bbl,  TRUE);
	  delta = -(t_int32)I386_OP_IMMEDIATE(src);
	}
	else
	  delta = STACKBOT;
	break;
      case I386_CALL:
      case I386_INT:
	/* for this analysis we act as if they don't change the stack pointer */
	break;
      case I386_RET:
	if (I386_OP_TYPE(src) == i386_optype_imm)
	  delta -= I386_OP_IMMEDIATE(src);
	break;
      default:
	/*VERBOSE(0,("STACKBOTTING out for @I\n",ins));*/
	delta = STACKBOT;
    }
    /*if (delta == STACKBOT) VERBOSE(0,("STACKBOT for @I\n",ins));*/
  }
  BBL_SET_STACK_DELTA(bbl,  delta);
} /* }}} */


/* vim: set shiftwidth=2 foldmethod=marker: */
