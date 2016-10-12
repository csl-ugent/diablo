#include <diabloanopti386.h>
#if 1
/* old pruts functions. obsolete by now. */
t_uint32 I386BblCmpExtReg(t_bbl * master,t_bbl * slave,t_uint32 * in)
{
	FATAL(("Implement I386BblCmpExtReg() !!!"));
	return 0;
}

t_uint32 I386BblAbstract(t_bbl * master,t_bbl * slave,t_uint32 * in,t_uint32 * out)
{
	FATAL(("Implement I386BblAbstract() !!!"));
	return 0;
}
#endif


static t_bool CanAlterStackops(t_bbl *bbl)
{
  t_i386_ins *ins;
  int delta = 0;
  t_bool stack_ok = TRUE;
  t_bool unknown;
  t_i386_operand *op;
  t_regset defined;

  defined = RegsetNew();

  BBL_FOREACH_I386_INS(bbl,ins)
  {
    if (I386InsIsControlTransfer(ins))
      break;	/* do not consider control transfers: they will be split off */

    delta += I386InsStackDelta(ins,&unknown);
    if (unknown || delta < 0)
    {
      stack_ok = FALSE;
      break;
    }

    /* {{{ check the memory operands */
    I386_INS_FOREACH_OP(ins,op)
    {
      if (I386_OP_TYPE(op) != i386_optype_mem)
	continue;
      /* allowed: - accesses against a relocated immediate (will NOT be from stack)
       *          - accesses with immediate offsets against %esp
       *          - accesses against registers not defined in this basic block (as they
       *            cannot refer to stack slots allocated in this block)
       */
      if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
      {
	/* ok. */
      }
      else if (I386_OP_BASE(op) == I386_REG_ESP
	  && I386_OP_INDEX(op) == I386_REG_NONE)
      {
	/* ok. */
      }
      else if (I386_OP_BASE(op) != I386_REG_ESP
	  && I386_OP_BASE(op) != I386_REG_NONE
	  && !RegsetIn(defined,I386_OP_BASE(op)))
      {
	/* ok. */
      }
      else
      {
	/* not ok. */
	stack_ok = FALSE;
	break;
      }
    } /* }}} */
    if (!stack_ok) break;

    /* {{{ check uses of %esp */
    if (RegsetIn(I386_INS_REGS_USE(ins),I386_REG_ESP))
    {
      switch (I386_INS_OPCODE(ins))
      {
	case I386_PUSH:
	  /* ok, as long as %esp is not the register pushed */
	  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg && 
	      I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP)
	    stack_ok = FALSE;
	  break;
	case I386_POP:
	case I386_PUSHF:
	case I386_POPF:
	case I386_PUSHA:
	case I386_POPA:
	  /* ok. */
	  break;
	case I386_ADD:
	case I386_SUB:
	  /* possibilities:
	   * 	- %esp is used in some memory operand => no problem, checked earlier
	   * 	- %esp is the destination operand => checked in I386InsStackDelta()
	   * 	- %esp is the source operand => problem: no way to compensate for stack offset
	   */
	  if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg &&
	      I386_OP_BASE(I386_INS_SOURCE1(ins)) == I386_REG_ESP)
	    stack_ok = FALSE;
	  break;
	default:
	  /* if %esp is used in a memory operand, then it's
	   * ok, because we can always alter the offset.
	   * if %esp is used as a register operand, it's not
	   * ok because we cannot compensate for the stack offset */
	  I386_INS_FOREACH_OP(ins,op)
	    if (I386_OP_TYPE(op) == i386_optype_reg)
	      stack_ok = FALSE;
      }
    } /* }}} */
    if (!stack_ok) break;

    RegsetSetUnion(defined,I386_INS_REGS_DEF(ins));
  }
  return stack_ok && delta == 0;
}

static t_regset FreeRegsForBbl(t_bbl *bbl)
{
  t_regset regs;
  t_ins *ins;

  RegsetSetDup(regs,BBL_REGS_LIVE_OUT(bbl));
  BBL_FOREACH_INS(bbl,ins)
  {
    RegsetSetUnion(regs,INS_REGS_DEF(ins));
    RegsetSetUnion(regs,INS_REGS_USE(ins));
  }
  RegsetSetInvers(regs);
  RegsetSetIntersect(regs,i386_description.int_registers);
  RegsetSetSubReg(regs,I386_REG_ESP);
  return regs;
}

/* can a bbl be factored out? */
/* The main problem here is the fact that the 
 * return address of the call is stored on the
 * stack. So either we can patch up all stack
 * operations in the block or we have a free
 * register in which to pop the return address
 * so the effect on the stack is undone. */
t_bool I386BblCanFactor(t_bbl *bbl)
{
  t_regset regs;
  t_address factorsize;
  t_ins *ins;

  /* is the block big enough? */
  /* recalculate BBL_CSIZE, as this is not guaranteed to be correct */
  factorsize = AddressNew32(0);
  BBL_FOREACH_INS(bbl,ins)
    factorsize = AddressAdd(factorsize,INS_CSIZE(ins));
  BBL_SET_CSIZE(bbl,  factorsize);
  if (BBL_NINS(bbl) && I386InsIsControlTransfer(T_I386_INS(BBL_INS_LAST(bbl))))
    factorsize = AddressSub(factorsize,INS_CSIZE(BBL_INS_LAST(bbl)));
  /* minimum size is 6, as a call instruction to the factored function 
   * will probably take 5 bytes */
  if (G_T_UINT32(factorsize) < 6  || BBL_NINS(bbl) < 3) { return FALSE; }

  if (CanAlterStackops(bbl))
  {
    return TRUE; /* factoring is always possible */
  }

  /* if we cannot reliably alter the stack operations,
   * we can still factor if there is a free register in
   * which to save the return address during execution
   * of the factored basic block. */
  regs = FreeRegsForBbl(bbl);

  return !RegsetIsEmpty(regs);
}

static void AlterStackops(t_bbl *bbl)
{
  t_i386_ins *ins;
  int delta = 0;
  t_bool unknown;
  t_i386_operand *op;

  BBL_FOREACH_I386_INS(bbl,ins)
  {
    int use_delta;
    delta += I386InsStackDelta(ins,&unknown);
    ASSERT(!unknown,("Should know all stack delta's in the block: @I",ins));
    /* if %esp is used as the base register in the memory operand of the push,
     * it's value *prior* to the pushing is used. for pops, the value *after*
     * popping is used. */
    if (I386_INS_OPCODE(ins) == I386_PUSH)
      use_delta = delta-4;
    else use_delta = delta;

    I386_INS_FOREACH_OP(ins,op)
    {
      t_int32 imm;
      if (I386_OP_TYPE(op) != i386_optype_mem)
	continue;
      if (I386_OP_BASE(op) != I386_REG_ESP)
	continue;

      if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
	FATAL(("Should not have relocated operands here: @I",ins));

      imm = (t_int32) I386_OP_IMMEDIATE(op);
      if (imm >= use_delta)
      {
	I386_OP_IMMEDIATE(op) += 4;
	if(I386_OP_IMMEDSIZE(op)==0)
	  I386_OP_IMMEDSIZE(op)=1;
      }
    }
  }
}

/*if limited, factoring is only done when the result is smaller
 * unlimited version used in diversity*/

t_bool I386BblFactorConditional(t_equiv_bbl_holder *equivs, t_bbl *master, t_bool limited)
{
  t_bool alter_stackops;
  t_regset *freeregs = NULL;
  t_reg save_reg = REG_NONE,r;
  int i;

  t_cfg *cfg = BBL_CFG(master);
  t_function *factor;
  t_bbl *new;
  t_i386_ins *ins;

  char name[80];
  static int nfactors;

  /* the preparatory work */
  alter_stackops = CanAlterStackops(master);
  if (!alter_stackops)
  {
    int regcount[8];
    for (i=0; i<8; i++) regcount[i] = 0;

    freeregs = Malloc(sizeof(t_regset)*equivs->nbbls);
    for (i=0; i<equivs->nbbls; i++)
      freeregs[i] = FreeRegsForBbl(equivs->bbl[i]);

    for (i=0; i<equivs->nbbls; i++)
      REGSET_FOREACH_REG(freeregs[i],r)
	regcount[r]++;
    save_reg = 0;
    for (i=0; i<8; i++)
      if (regcount[i] > regcount[save_reg])
	save_reg = i;
    /* only perform factoring if the result is smaller */
    if (limited && 
	G_T_UINT32(BBL_CSIZE(master)) + 3 + regcount[save_reg]*5 >=
	regcount[save_reg] * G_T_UINT32(BBL_CSIZE(master)))
    {
      Free(freeregs);
      return FALSE;
    }
  }
  else
  {
    /* only perform factoring if the result is smaller */
    if (limited && 
	G_T_UINT32(BBL_CSIZE(master)) + 1 + equivs->nbbls*5 >= 
	equivs->nbbls * G_T_UINT32(BBL_CSIZE(master)))
      return FALSE;
  }

  /* hook for adding extra checks to see if factoring is viable */
  {
    t_bool doit = TRUE;
    DiabloBrokerCall ("?BblFactoringDo", equivs, &doit);
    if (!doit) return FALSE;
  }

  /* create the new function */
  new = BblDup(master);

  for (i = 0; i < equivs->nbbls; i++)
  {
    if (!alter_stackops && !RegsetIn(freeregs[i],save_reg))
      continue;
    DiabloBrokerCall ("RegisterBblFactoring", equivs->bbl[i], new);
  }

  if (!alter_stackops)
  {
    I386MakeInsForBbl(Pop,Prepend,ins,new,save_reg);
    I386MakeInsForBbl(Push,Append,ins,new,save_reg,0);
  }
  else
    AlterStackops(new);
  I386MakeInsForBbl(Return,Append,ins,new);
  sprintf(name,"factor-%d-0x%x",nfactors++,G_T_UINT32(BBL_OLD_ADDRESS(master)));
  factor = FunctionMake(new,name,FT_NORMAL);
  DiabloBrokerCall ("RegisterFunFactoring", factor);
  CfgEdgeCreate(cfg,new,FunctionGetExitBlock(factor),ET_JUMP);

  /* add calls to the new function */
  for (i = 0; i < equivs->nbbls; i++)
  {
    t_bbl *orig = equivs->bbl[i];
    t_bbl *next;

    if (!alter_stackops && !RegsetIn(freeregs[i],save_reg))
      continue;
    DiabloBrokerCall ("BblFactorBefore", orig, new);
    
    ASSERT(
	BBL_SUCC_FIRST(orig),
	("Corrupt flowgraph: Bbl has no successor edge: @ieB",orig));

    ASSERT(
	BBL_SUCC_FIRST(orig) && 
	BBL_SUCC_FIRST(orig) == BBL_SUCC_LAST(orig) &&
	(CFG_EDGE_CAT(BBL_SUCC_FIRST(orig)) & (ET_FALLTHROUGH|ET_IPFALLTHRU)),
	("Unexpected successor edge: @ieB",orig));
    next = CFG_EDGE_TAIL(BBL_SUCC_FIRST(orig));
    if (CFG_EDGE_CAT(BBL_SUCC_FIRST(orig)) == ET_IPFALLTHRU)
    {
      /* avoid problems with call-return pairs spanning function boundaries */
      next = BblSplitBlock(orig,BBL_INS_LAST(orig),FALSE);
    }
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(orig)));

    while (BBL_INS_FIRST(orig))
      InsKill(BBL_INS_FIRST(orig));
    I386MakeInsForBbl(Call,Append,ins,orig);
    
    CfgEdgeCreateCall(cfg,orig,FUNCTION_BBL_FIRST(factor),next,FunctionGetExitBlock(factor));

    DiabloBrokerCall ("BblFactorAfter", orig, new);
  }
  
  if (freeregs) Free(freeregs);
  return TRUE;
}

t_bool I386BblFactor(t_equiv_bbl_holder *equivs, t_bbl *master)
{
  return I386BblFactorConditional(equivs, master, TRUE);
}

void I386BblFactorInit(t_cfg * cfg)
{
	CFG_SET_BBL_FACTOR(cfg,I386BblFactor);
	CFG_SET_BBL_FINGERPRINT(cfg,I386BblFingerprint);
	CFG_SET_BBL_CAN_BE_FACTORED(cfg,I386BblCanFactor);
}


/* vim: set shiftwidth=2 foldmethod=marker: */
