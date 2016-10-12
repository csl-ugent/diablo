#include <diabloanoptamd64.h>
#if 1
/* old pruts functions. obsolete by now. */
t_uint32 Amd64BblCmpExtReg(t_bbl * master,t_bbl * slave,t_uint32 * in)
{
	FATAL(("Implement Amd64BblCmpExtReg() !!!"));
	return 0;
}

t_uint32 Amd64BblAbstract(t_bbl * master,t_bbl * slave,t_uint32 * in,t_uint32 * out)
{
	FATAL(("Implement Amd64BblAbstract() !!!"));
	return 0;
}
#endif
/*int total_count=0;*/

static t_bool CanAlterStackops(t_bbl *bbl)
{
  t_amd64_ins *ins;
  int delta = 0;
  t_bool stack_ok = TRUE;
  t_bool unknown;
  t_amd64_operand *op;
  t_regset defined;

  defined = RegsetNew();

  BBL_FOREACH_AMD64_INS(bbl,ins)
  {
    if (Amd64InsIsControlTransfer(ins))
      break;	/* do not consider control transfers: they will be split off */

    delta += Amd64InsStackDelta(ins,&unknown);
    if (unknown || delta < 0)
    {
      stack_ok = FALSE;
      break;
    }

    /* {{{ check the memory operands */
    AMD64_INS_FOREACH_OP(ins,op)
    {
      if (AMD64_OP_TYPE(op) != amd64_optype_mem)
	continue;
      /* allowed: - accesses against a relocated immediate (will NOT be from stack)
       *          - accesses with immediate offsets against %rsp
       *          - accesses against registers not defined in this basic block (as they
       *            cannot refer to stack slots allocated in this block)
       */
      if (AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)
      {
	/* ok. */
      }
      else if (AMD64_OP_BASE(op) == AMD64_REG_RSP
	  && AMD64_OP_INDEX(op) == AMD64_REG_NONE)
      {
	/* ok. */
      }
      else if (AMD64_OP_BASE(op) != AMD64_REG_RSP
	  && AMD64_OP_BASE(op) != AMD64_REG_NONE
	  && !RegsetIn(defined,AMD64_OP_BASE(op)))
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

    /* {{{ check uses of %rsp */
    if (RegsetIn(AMD64_INS_REGS_USE(ins),AMD64_REG_RSP))
    {
      switch (AMD64_INS_OPCODE(ins))
      {
	case AMD64_PUSH:
	  /* ok, as long as %rsp is not the register pushed */
	  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg && 
	      AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP)
	    stack_ok = FALSE;
	  break;
	case AMD64_POP:
	case AMD64_PUSHF:
	case AMD64_POPF:
	  /* ok. */
	  break;
	case AMD64_ADD:
	  case AMD64_SUB:
	  /* possibilities:
	   * 	- %rsp is used in some memory operand => no problem, checked earlier
	   * 	- %rsp is the destination operand => checked in Amd64InsStackDelta()
	   * 	- %rsp is the source operand => problem: no way to compensate for stack offset
	   */
	  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg &&
	      AMD64_OP_BASE(AMD64_INS_SOURCE1(ins)) == AMD64_REG_RSP)
	    stack_ok = FALSE;
	  break;
	default:
	  /* if %rsp is used in a memory operand, then it's
	   * ok, because we can always alter the offset.
	   * if %rsp is used as a register operand, it's not
	   * ok because we cannot compensate for the stack offset */
	  AMD64_INS_FOREACH_OP(ins,op)
	    if (AMD64_OP_TYPE(op) == amd64_optype_reg)
	      stack_ok = FALSE;
      }
    } /* }}} */
    if (!stack_ok) break;

    RegsetSetUnion(defined,AMD64_INS_REGS_DEF(ins));
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
  RegsetSetIntersect(regs,amd64_description.int_registers);
  RegsetSetSubReg(regs,AMD64_REG_RSP);
  return regs;
}

/* can a bbl be factored out? */
/* The main problem here is the fact that the 
 * return address of the call is stored on the
 * stack. So either we can patch up all stack
 * operations in the block or we have a free
 * register in which to pop the return address
 * so the effect on the stack is undone. */
t_bool Amd64BblCanFactor(t_bbl *bbl)
{
  t_regset regs;
  t_address factorsize;
  t_ins * gins, *gins2;
  int i;
  
  if(!BBL_SUCC_FIRST(bbl))
    return FALSE;

  i=0;
  BBL_FOREACH_INS(bbl,gins2){
    t_amd64_ins * ins2 = T_AMD64_INS(gins2);
    if(i>=3)
      break;
    i++;
    if(AMD64_INS_OPCODE(ins2)==AMD64_MOV){
      if(i==1)
	if(!(AMD64_OP_TYPE(AMD64_INS_DEST(ins2))==amd64_optype_reg && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins2))==amd64_optype_mem))
	  break;
      if(i==2)
	if(!(AMD64_OP_TYPE(AMD64_INS_DEST(ins2))==amd64_optype_mem && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins2))==amd64_optype_reg))
	  break;
	
	  
      if(i==3){	
	if((AMD64_OP_TYPE(AMD64_INS_DEST(ins2))==amd64_optype_mem && AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins2))==amd64_optype_imm))
	{
	  VERBOSE(0,("@ieB",bbl));
	  return FALSE;
	}else{
	  break;     
	}
      }
    }else{
      break;
    }
  }
  i=0;
  BBL_FOREACH_INS(bbl,gins2){
    t_amd64_ins * ins2 = T_AMD64_INS(gins2);
    if(i==0){
      if(AMD64_INS_OPCODE(ins2)==AMD64_MOVZX){
	i=1;
      }
    }else{
      if(AMD64_INS_OPCODE(ins2)==AMD64_OR){
	i++;
      }else{
	break;
      }
    }
    if(i==7){
      VERBOSE(0,("@ieB",bbl));
      return FALSE;
    }
  }

  
  /* is the block big enough? */
  /* recalculate BBL_CSIZE, as this is not guaranteed to be correct */
  factorsize = AddressNew64(0);
  BBL_FOREACH_INS(bbl,gins)
    factorsize = AddressAdd(factorsize,INS_CSIZE(gins));
  BBL_SET_CSIZE(bbl,  factorsize);
  if (BBL_NINS(bbl) && Amd64InsIsControlTransfer(T_AMD64_INS(BBL_INS_LAST(bbl))))
    factorsize = AddressSub(factorsize,INS_CSIZE(BBL_INS_LAST(bbl)));
  /* minimum size is 6, as a call instruction to the factored function 
   * will probably take 5 bytes */
  if (G_T_UINT64(factorsize) < 6  || BBL_NINS(bbl) < 3) { return FALSE; }

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
  t_amd64_ins *ins;
  int delta = 0;
  t_bool unknown;
  t_amd64_operand *op;

  BBL_FOREACH_AMD64_INS(bbl,ins)
  {
    int use_delta;
    delta += Amd64InsStackDelta(ins,&unknown);
    ASSERT(!unknown,("Should know all stack delta's in the block: @I",ins));
    /* if %rsp is used as the base register in the memory operand of the push,
     * it's value *prior* to the pushing is used. for pops, the value *after*
     * popping is used. */
    if (AMD64_INS_OPCODE(ins) == AMD64_PUSH)
      use_delta = delta-8;
    else use_delta = delta;

    AMD64_INS_FOREACH_OP(ins,op)
    {
      t_int64 imm;
      if (AMD64_OP_TYPE(op) != amd64_optype_mem)
	continue;
      if (AMD64_OP_BASE(op) != AMD64_REG_RSP)
	continue;

      if (AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)
	FATAL(("Should not have relocated operands here: @I",ins));

      imm = (t_int64) AMD64_OP_IMMEDIATE(op);
      if (imm >= use_delta)
	AMD64_OP_IMMEDIATE(op) += 8;
    }
  }
}

t_bool Amd64BblFactor(t_equiv_bbl_holder *equivs, t_bbl *master)
{
  t_bool alter_stackops;
  t_regset *freeregs = NULL;
  t_reg save_reg = REG_NONE,r;
  int i;

  t_cfg *cfg = BBL_CFG(master);
  t_function *factor;
  t_bbl *new;
  t_amd64_ins *ins;

  char name[80];
  static int nfactors;

  /* the preparatory work */
  alter_stackops = CanAlterStackops(master);
  if (!alter_stackops)
  {
    int regcount[16];
    for (i=0; i<16; i++) regcount[i] = 0;

    freeregs = Malloc(sizeof(t_regset)*equivs->nbbls);
    for (i=0; i<equivs->nbbls; i++)
      freeregs[i] = FreeRegsForBbl(equivs->bbl[i]);

    for (i=0; i<equivs->nbbls; i++)
      REGSET_FOREACH_REG(freeregs[i],r)
	regcount[r]++;
    save_reg = 0;
    for (i=0; i<16; i++)
      if (regcount[i] > regcount[save_reg])
	save_reg = i;
    /* only perform factoring if the result is smaller */
    if (G_T_UINT64(BBL_CSIZE(master)) + 3 + regcount[save_reg]*5 >= regcount[save_reg]*G_T_UINT64(BBL_CSIZE(master)))
    {
      Free(freeregs);
      return FALSE;
    }
  }
  else
  {
    /* only perform factoring if the result is smaller */
    if (G_T_UINT64(BBL_CSIZE(master)) + 1 + equivs->nbbls*5 >= equivs->nbbls*G_T_UINT64(BBL_CSIZE(master)))
      return FALSE;
  }

  /* create the new function */
  new = BblDup(master);
  if (!alter_stackops)
  {
    Amd64MakeInsForBbl(Pop,Prepend,ins,new,save_reg);
    Amd64MakeInsForBbl(Push,Append,ins,new,save_reg,0);
  }
  else
    AlterStackops(new);
  Amd64MakeInsForBbl(Return,Append,ins,new);
  sprintf(name,"factor-%d-0x%llx",nfactors++,G_T_UINT64(BBL_OLD_ADDRESS(master)));
  factor = FunctionMake(new,name,FT_NORMAL);
  CfgEdgeCreate(cfg,new,FunctionGetExitBlock(factor),ET_JUMP);

  /* add calls to the new function */
  for (i = 0; i < equivs->nbbls; i++)
  {
    t_bbl *orig = equivs->bbl[i];
    t_bbl *next;
    t_amd64_ins *ins;

    if (!alter_stackops && !RegsetIn(freeregs[i],save_reg))
      continue;
    DiabloBrokerCall ("BblFactorBefore", orig, new); 
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
    
    Amd64MakeInsForBbl(Call,Append,ins,orig);
    CfgEdgeCreateCall(cfg,orig,FUNCTION_BBL_FIRST(factor),next,FunctionGetExitBlock(factor));

    DiabloBrokerCall ("BblFactorAfter", orig, new);
  }

  if (freeregs) Free(freeregs);
  return TRUE;
}

void Amd64BblFactorInit(t_cfg * cfg)
{
  CFG_SET_BBL_FACTOR(cfg,Amd64BblFactor);
  CFG_SET_BBL_FINGERPRINT(cfg,Amd64BblFingerprint);
  CFG_SET_BBL_CAN_BE_FACTORED(cfg,Amd64BblCanFactor);
}


/* vim: set shiftwidth=2 foldmethod=marker: */
