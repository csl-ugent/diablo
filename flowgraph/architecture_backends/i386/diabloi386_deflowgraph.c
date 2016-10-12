/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* new i386 layout and deflowgraph algorithm, by Dominique
 * this is intended to be a lot more general than the old 
 * algorithm, with support for splitting the merged code
 * section as needed for linux kernel optimization */

#include <stdlib.h>
#include <string.h>
#include <diabloi386.h>

void I386ListFinalProgram(t_bbl * bbl);
  
#define SHORT_DISPL_HI_BOUND CHAR_MAX
#define SHORT_DISPL_LO_BOUND CHAR_MIN
#define FULL_DISPL_HI_BOUND  INT_MAX
#define FULL_DISPL_LO_BOUND  INT_MIN                                

/* some handy macros */
#define BBL_CHAIN_INDEX_INT(bbl)	((long)BBL_PREV_IN_CHAIN(BBL_FIRST_IN_CHAIN(bbl)))
#define BBL_CHAIN_INDEX(bbl)	(BBL_PREV_IN_CHAIN(BBL_FIRST_IN_CHAIN(bbl)))
#define BBL_SET_CHAIN_INDEX(bbl,x)(BBL_SET_PREV_IN_CHAIN(BBL_FIRST_IN_CHAIN(bbl),x))
#define BBL_CHAIN_SIZE(bbl)	AddressAdd(BBL_CSIZE(BBL_LAST_IN_CHAIN(bbl)),BBL_CADDRESS(BBL_LAST_IN_CHAIN(bbl)))
#define BblInChainset(bbl,ch) \
  (BBL_CHAIN_INDEX_INT(bbl) < ch->nchains && \
   BBL_FIRST_IN_CHAIN(bbl) == ch->chains[BBL_CHAIN_INDEX_INT(bbl)])
#define ChainSwap(ch,__i,__j) \
  do { \
    int __k = (__i), __l = (__j); \
    t_bbl *tmp = ch->chains[__k]; \
    ch->chains[__k] = ch->chains[__l]; \
    ch->chains[__l] = tmp; \
    BBL_SET_CHAIN_INDEX(ch->chains[__k], (void*)__k); \
    BBL_SET_CHAIN_INDEX(ch->chains[__l], (void*)__l); \
  } while (0)

/* {{{ create chains of basic blocks */
#define ET_MUSTCHAIN  	ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE  	ET_CALL | ET_SWI
/*TODO: SWITCH TABLES!!! */
void I386CreateChains(t_cfg * cfg, t_chain_holder * ch)
{
  t_bbl * i_bbl;
  t_uint32 nbbls = 0, nchains = 0;
  int i;
 
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    BBL_SET_FIRST_IN_CHAIN(i_bbl, NULL);
    BBL_SET_LAST_IN_CHAIN(i_bbl, NULL);
    BBL_SET_NEXT_IN_CHAIN(i_bbl, NULL);
    BBL_SET_PREV_IN_CHAIN(i_bbl, NULL);
    /* count all bbls except return blocks and hell nodes (those will not be
     * chained) */
    if (!BBL_IS_HELL(i_bbl) && 
	(!BBL_FUNCTION(i_bbl) || 
	 (FunctionGetExitBlock(BBL_FUNCTION(i_bbl)) != i_bbl)))
      nbbls++;
  }
  nchains = nbbls;
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_cfg_edge * i_edge;

    /* skip return blocks and hell nodes */
    if (BBL_IS_HELL(i_bbl) || 
	(BBL_FUNCTION(i_bbl) && 
	 (FunctionGetExitBlock(BBL_FUNCTION(i_bbl)) == i_bbl)))
      continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (CfgEdgeTestCategoryOr(i_edge,ET_MUSTCHAIN))
	break;
      if (CfgEdgeTestCategoryOr(i_edge,ET_MUSTCHAINMAYBE) && 
	  CFG_EDGE_CORR(i_edge))
	{
	  /*VERBOSE (0, ("Taking corr edge for @E", i_edge));*/
	  i_edge = CFG_EDGE_CORR(i_edge);
	  break;
	}
    }
    if (i_edge)
    {
      t_bbl * tail = CFG_EDGE_TAIL(i_edge);
      /*VERBOSE (0, ("Chaining @B and @B", i_bbl, tail));*/
      ASSERT (!BBL_PREV_IN_CHAIN (tail), 
	  ("tail already chained to @ieB", BBL_PREV_IN_CHAIN (tail)));
      ASSERT (!BBL_NEXT_IN_CHAIN (i_bbl), 
	  ("head already chained to @ieB", BBL_NEXT_IN_CHAIN (i_bbl)));
      BBL_SET_PREV_IN_CHAIN(tail,  i_bbl);
      BBL_SET_NEXT_IN_CHAIN(i_bbl,  tail);
      nchains--; /* each time two blocks get chained, the number of chains
		    decreases by one */
    }
  }

  ch->chains = Malloc(nchains*sizeof(t_bbl *));
  ch->nchains = nchains;

  i = 0;
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bbl * j_bbl, * prev = NULL;

    /* skip return blocks and hell nodes */
    if (BBL_IS_HELL(i_bbl) || 
	(BBL_FUNCTION(i_bbl) && 
	 (FunctionGetExitBlock(BBL_FUNCTION(i_bbl)) == i_bbl)))
      continue;

    if (BBL_PREV_IN_CHAIN(i_bbl)) 
      continue; /* we're looking for the heads of the chains here */

    ASSERT (i < ch->nchains, ("More chains second time around"));
    ch->chains[i++] = i_bbl;
    for (j_bbl = i_bbl; j_bbl; prev = j_bbl, j_bbl = BBL_NEXT_IN_CHAIN(j_bbl))
    {
      BBL_SET_FIRST_IN_CHAIN(j_bbl,  i_bbl);
    }
    for (j_bbl = prev; j_bbl; j_bbl = BBL_PREV_IN_CHAIN(j_bbl))
    {
      BBL_SET_LAST_IN_CHAIN(j_bbl,  prev);
    }
  }
}
/* }}} */

/* {{{ merge two chains */
void I386MergeTwoChains(t_bbl * ca, t_bbl * cb)
{
  t_address length_of_ca = BBL_CHAIN_SIZE(ca);
  MergeChains(ca,cb);
  AssignAddressesInChain(cb,length_of_ca);
}
/* }}} */

/* {{{ cluster chains to in order to encode jump offsets as efficient as possible */
/* {{{ find candidate for appending */
static int I386FindAppendable(t_chain_holder * ch, int index)
{
  t_bbl * bbl;
  t_i386_ins * ins;
  t_cfg_edge * edge;
  long ret = -1;

  /* BBL_PREV_IN_CHAIN of the head of the chain contains the index in the
   * chains array. This slightly complicates walking backwardly through the
   * chain */
  for (bbl = BBL_LAST_IN_CHAIN(ch->chains[index]); bbl;
       bbl = (bbl == BBL_FIRST_IN_CHAIN(bbl) ? NULL :  BBL_PREV_IN_CHAIN(bbl)))
  {
    t_uint32 dist_to_end_of_chain = G_T_UINT32(AddressSub(
	  BBL_CHAIN_SIZE(ch->chains[index]),
	  AddressAdd(BBL_CADDRESS(bbl),BBL_CSIZE(bbl))));
    if (dist_to_end_of_chain >= 128) break;

    ins = T_I386_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;

    if (I386_INS_TYPE(ins) == IT_BRANCH &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm && 
	I386_INS_OPCODE(ins) != I386_RET &&
	I386_INS_OPCODE(ins) != I386_CALL)
    {
      t_bbl * tail;
      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	if ((CfgEdgeTestCategoryOr(edge,ET_JUMP|ET_IPJUMP|ET_CALL))&&(!BBL_IS_HELL(CFG_EDGE_TAIL(edge)))) /* ET_CALL still needed here for instrumentation */
	  break;

      if (!edge) continue;

      tail = CFG_EDGE_TAIL(edge);
      /*VERBOSE(0,("@iB\n@E\n@iB\n",bbl,edge,tail)); */
      ret = BBL_CHAIN_INDEX_INT(tail);
      if (ret != index && BblInChainset(tail,ch))
	return ret;
    }
  }
  return -1;
}
/* }}} */
/* {{{ find candidate for prepending */
static long I386FindPrependable(t_chain_holder * ch, int index)
{
  t_bbl * bbl;
  t_i386_ins * ins;
  t_cfg_edge * edge;

  for (bbl = ch->chains[index]; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    t_uint32 from_start = G_T_UINT32(AddressAdd(BBL_CADDRESS(bbl),BBL_CSIZE(bbl)));
    if (from_start >= 128)
      break;

    ins = T_I386_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;

    if (I386_INS_TYPE(ins) == IT_BRANCH &&
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm &&
	I386_INS_OPCODE(ins) != I386_RET &&
	I386_INS_OPCODE(ins) != I386_CALL)
    {
      t_bbl * tail;
      long chainno;

      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	if ((CfgEdgeTestCategoryOr(edge,ET_JUMP|ET_IPJUMP|ET_CALL))&&(!BBL_IS_HELL(CFG_EDGE_TAIL(edge)))) /* ET_CALL still needed here for instrumentation */
	  break;
      if (!edge) continue;
      
      tail = CFG_EDGE_TAIL(edge);

      chainno = BBL_CHAIN_INDEX_INT(tail);
      if (chainno != index && BblInChainset(tail,ch))
      {
	t_uint32 from_end = G_T_UINT32(AddressSub(BBL_CHAIN_SIZE(ch->chains[chainno]),BBL_CADDRESS(tail)));
	if (from_start + from_end < 128)
	  return chainno;
      }
    }
  }
  
  return -1;
}
/* }}} */
/* {{{ I386ClusterChainsForMinimalJumpOffsets */
void I386ClusterChainsForMinimalJumpOffsets(t_chain_holder * ch)
{
  int i, j;
  t_address null = AddressSub(BBL_CADDRESS(ch->chains[0]),BBL_CADDRESS(ch->chains[0]));

  for (i=0; i<ch->nchains; i++)
  {
    AssignAddressesInChain(ch->chains[i],null);
    if (ch->chains[i])
    {
      BBL_SET_CHAIN_INDEX(ch->chains[i],  (void*)(long)i);
    }
  }

  /* {{{ try to find chains that can be appended */
  for (i = 0; i < ch->nchains; i++)
  {
    int j;
    if (ch->chains[i] == NULL) continue;

    while ((j = I386FindAppendable(ch,i)) != -1)
    {
      I386MergeTwoChains(ch->chains[i],ch->chains[j]);
      ch->chains[j] = NULL;
    }
  } /* }}} */
  
  /* {{{ try to find chains that can be prepended */
  for (i = 0; i < ch->nchains; i++)
  {
    int j;
    if (ch->chains[i] == NULL) continue;
    while ((j = I386FindPrependable(ch,i)) != -1)
    {
      I386MergeTwoChains(ch->chains[j],ch->chains[i]);
      if (j > i)
      {
	ch->chains[i] = NULL;
	break;
      }
      else
      {
	ch->chains[i] = ch->chains[j];
	ch->chains[j] = NULL;
	BBL_SET_CHAIN_INDEX(ch->chains[i],  (void*)(long)i);
      }
    }
  } /* }}} */

  for (i = 0, j = 0; i < ch->nchains; ++i)
    if (ch->chains[i])
      ch->chains[j++] = ch->chains[i];
  ch->nchains = j;

} /* }}} */ 
/* }}} */


/* {{{ Randomly order chains */
void
I386OrderChainsRandomly(t_chain_holder * ch)
{
  t_bbl *tmpchain;
  int i, index;

  srand(diabloi386_options.orderseed);
  /* standard shuffle */
  for (i = ch->nchains - 1; i >= 0; --i)
  {
    index = random() % (i + 1);
    tmpchain = ch->chains[index];
    ch->chains[index] = ch->chains[i];
    ch->chains[i] = tmpchain;
  }
}
/* }}} */

/* {{{ I386KillUselessJumps */
void I386KillUselessJumps(t_object * obj)
{
  int i;
  t_uint32 nkills = 0;

  for (i = 0; i < OBJECT_NCODES (obj); i++)
  {
    t_bbl * bbl;
    t_section * sec = OBJECT_CODE (obj)[i];
    ASSERT (SECTION_TYPE (sec) == DEFLOWGRAPHING_CODE_SECTION,
	("Section %s has unexpected type %c",
	 SECTION_NAME (sec), SECTION_TYPE (sec)));

    CHAIN_FOREACH_BBL (T_BBL (SECTION_TMP_BUF (sec)), bbl)
    {
      t_i386_ins * ins = T_I386_INS(BBL_INS_LAST(bbl));
      t_cfg_edge * edge;

      if (!ins) continue;
      if (I386_INS_OPCODE(ins) != I386_JMP && 
	  I386_INS_OPCODE(ins) != I386_Jcc && 
	  I386_INS_OPCODE(ins) != I386_JECXZ) 
	continue;

      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	if (CfgEdgeTestCategoryOr (edge,
	      ET_JUMP | ET_IPJUMP | ET_SWITCH | ET_IPSWITCH))
	  break;
      /* when doing instrumentation using old addresses, calls are changed into
       * jumps, but the call edges remain. */
      if (!edge) continue;

      if (CfgEdgeTestCategoryOr (edge, ET_SWITCH | ET_IPSWITCH)) continue;
      
      if (CFG_EDGE_TAIL(edge) == BBL_NEXT_IN_CHAIN(bbl))
      {
	/* if the control registers have been updated in the same block, 
	 * the jump has a side effect: the prefetch queue is flushed.
	 * consequently, we can't delete the jump instruction */
	{
	  t_bbl * bbl = I386_INS_BBL(ins);
	  t_i386_ins * iter;
	  t_regset usedef = NullRegs;
	  BBL_FOREACH_I386_INS(bbl,iter)
	  {
	    RegsetSetUnion(usedef,I386_INS_REGS_USE(iter));
	    RegsetSetUnion(usedef,I386_INS_REGS_DEF(iter));
	  }
	  if (RegsetIn(usedef,I386_REG_CR0) ||
	      RegsetIn(usedef,I386_REG_CR1) ||
	      RegsetIn(usedef,I386_REG_CR2) ||
	      RegsetIn(usedef,I386_REG_CR3) ||
	      RegsetIn(usedef,I386_REG_CR4))
	    continue;
	}

	I386InsKill(ins);
	nkills++;
	CFG_EDGE_SET_CAT(edge, (CFG_EDGE_CAT(edge) == ET_JUMP) ? ET_FALLTHROUGH : ET_IPFALLTHRU);
      }
    }
  }
  VERBOSE(0,("[I386KillUselessJumps] Killed %d useless jumps", nkills));
}
/* }}} */

/* {{{ adjust jump offsets (make them 1 byte if possible, in 4 if necessary) */

t_bool I386ChainAdjustJumpOffsetsUp (t_bbl *chain)
{
  t_bool change = FALSE;
  t_bbl *i_bbl;

  CHAIN_FOREACH_BBL (chain, i_bbl)
  {
    t_address offset;
    t_int32 sval;
    t_bbl * dest;
    t_cfg_edge * i_edge;
    t_i386_ins * last = T_I386_INS(BBL_INS_LAST(i_bbl));
    t_i386_operand * op;

    if (!last || 
	(I386_INS_OPCODE(last) != I386_JMP && 
	 I386_INS_OPCODE(last) != I386_Jcc && 
	 I386_INS_OPCODE(last) != I386_JECXZ && 
	 I386_INS_OPCODE(last) != I386_LOOP && 
	 I386_INS_OPCODE(last) != I386_LOOPZ && 
	 I386_INS_OPCODE(last) != I386_LOOPNZ))
      continue;

    /* we're only interested in jumps with an immediate short offset */
    op = I386_INS_SOURCE1(last);
    if (I386_OP_TYPE(op) != i386_optype_imm || I386_OP_IMMEDSIZE(op) != 1)
      continue;

    /* find the destination of the control transfer */
    BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
      if (CfgEdgeTestCategoryOr(i_edge,ET_CALL | ET_JUMP | ET_IPJUMP))
	break;
    ASSERT(i_edge,("Could not find edge for @I",last));
    dest = CFG_EDGE_TAIL(i_edge);

    offset = AddressSub (BBL_CADDRESS (dest),
	AddressAdd (I386_INS_CADDRESS (last), I386_INS_CSIZE (last)));
    sval = G_T_UINT32 (offset);

    if (sval < -128 || sval > 127 ||
	I386_INS_HAS_FLAG (last, I386_IF_JMP_FORCE_4BYTE))
    {
      if (I386_INS_OPCODE(last) != I386_JECXZ 
	  && I386_INS_OPCODE(last) != I386_LOOP
	  && I386_INS_OPCODE(last) != I386_LOOPZ 
	  && I386_INS_OPCODE(last) != I386_LOOPNZ)
      {
	/* make long jump */
	I386_OP_IMMEDSIZE(op) = 4;
	I386_OP_IMMEDIATE(op) = G_T_UINT32(offset);
	I386_INS_SET_CSIZE(last,  I386InsGetSize(last));
      }
      else
      {
	/* no long jump version of jecxz/loop(nz) available: we'll have to do 
	 * something rather complicated instead
	 * jecxz <dest> becomes:
	 *
	 * jecxz <trampoline>
	 * jmp <fallthrough>
	 * trampoline: jmp <dest>
	 * fallthrough: ...
	 */
	t_bbl * blk_ft, * blk_dest;
	t_i386_ins * jmp_ft, * jmp_dest;
	t_cfg * cfg = BBL_CFG(i_bbl);
	t_cfg_edge * edge;

	/* add blocks */
	blk_ft = BblNew(cfg);
	blk_dest = BblNew(cfg);
	BBL_SET_NEXT_IN_CHAIN(blk_ft,  blk_dest);
	BBL_SET_PREV_IN_CHAIN(blk_dest,  blk_ft);
	BBL_SET_NEXT_IN_CHAIN(blk_dest,  BBL_NEXT_IN_CHAIN(i_bbl));
	BBL_SET_PREV_IN_CHAIN(blk_ft,  i_bbl);
	BBL_SET_NEXT_IN_CHAIN(i_bbl,  blk_ft);
	BBL_SET_PREV_IN_CHAIN(BBL_NEXT_IN_CHAIN(blk_dest),  blk_dest);

	/* add instructions */
	jmp_ft = T_I386_INS(InsNewForBbl(blk_ft));
	InsAppendToBbl(T_INS(jmp_ft),blk_ft);
	I386InstructionMakeJump(jmp_ft);
	jmp_dest = T_I386_INS(InsNewForBbl(blk_dest));
	InsAppendToBbl(T_INS(jmp_dest),blk_dest);
	I386InstructionMakeJump(jmp_dest);

	/* replace the edges */
	BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	  if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH)
	    break;
	ASSERT(edge, ("Could not find fallthrough edge after @I\n", last));
	CfgEdgeKill(edge);
	BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	  if (CFG_EDGE_CAT(edge) == ET_JUMP)
	    break;
	ASSERT(edge, ("Could not find jump edge after @I\n", last));
	CfgEdgeCreate(cfg,blk_dest,CFG_EDGE_TAIL(edge),ET_JUMP);
	CfgEdgeKill(edge);
	CfgEdgeCreate(cfg,i_bbl,blk_ft,ET_FALLTHROUGH);
	CfgEdgeCreate(cfg,blk_ft,BBL_NEXT_IN_CHAIN(blk_dest),ET_JUMP);
	CfgEdgeCreate(cfg,i_bbl,blk_dest,ET_JUMP);

	VERBOSE(0,("@I out of reach: extending by trampoline", last));
      }
      change = TRUE;
    }
  }
  return change;
}

t_bool I386ChainAdjustJumpOffsetsDown (t_bbl *chain)
{
  t_bbl *i_bbl;
  t_bool change = FALSE;
  t_bool first = FALSE;
  t_int32 maxalign, alignadjustment;

  /* If a bbl needs to be aligned, it's possible that reducing the size of an
   * instruction pushes the target of another one out of range. E.g.
   *
   *   jmp yyy:
   *   ...
   *  .balign 2
   *   yyy:
   *
   * If at the start xxx is exactly 127 bytes from its jump, then reducing
   * "jmp xxx" from 5 to 2 bytes may move its target out of range (due to the
   * alignment the target only moves back 2 bytes).
   */
  maxalign = 1;
  CHAIN_FOREACH_BBL (chain, i_bbl)
  {
    if (BBL_ALIGNMENT(i_bbl) > maxalign)
      maxalign = BBL_ALIGNMENT(i_bbl);
  }

  CHAIN_FOREACH_BBL (chain, i_bbl)
  {
    t_address offset;
    t_int32 sval;
    t_bbl * dest;
    t_cfg_edge * i_edge;
    t_i386_ins * last = T_I386_INS(BBL_INS_LAST(i_bbl));
    t_i386_operand * op;

    if (!last || 
	(I386_INS_OPCODE(last) != I386_JMP && 
	 I386_INS_OPCODE(last) != I386_Jcc))
      continue;

    /* we're only interested in jumps with an immediate long offset */
    op = I386_INS_SOURCE1(last);
    if (I386_OP_TYPE(op) != i386_optype_imm || I386_OP_IMMEDSIZE(op) != 4 || I386_INS_REFERS_TO(last))
      continue;

    /* find the destination of the control transfer */
    BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
      if (CfgEdgeTestCategoryOr(i_edge,ET_CALL | ET_JUMP | ET_IPJUMP))
	break;
    ASSERT(i_edge,("Could not find edge for @I",last));
    dest = CFG_EDGE_TAIL(i_edge);

    offset = AddressSub (BBL_CADDRESS (dest),
	AddressAdd (I386_INS_CADDRESS (last), I386_INS_CSIZE (last)));
    sval = AddressExtractInt32 (offset);

    if (sval >= -128 && sval < 128 &&
	!I386_INS_HAS_FLAG (last, I386_IF_JMP_FORCE_4BYTE))
    {
      t_bool shorten;

      shorten = TRUE;
      if (sval >= 128 - (maxalign-1))
      {
        /* check if there is an aligned block between the jump and its
         * destination that may push the target out of range and if so, don't
         * reduce the jump's size
         */
        if (BBL_FIRST_IN_CHAIN(dest) == chain)
        {
          t_bbl *runner;
          t_int32 maxlocalalign;

          runner = i_bbl;
          maxlocalalign = 1;
          do
          {
            runner = BBL_NEXT_IN_CHAIN(runner);
            if (BBL_ALIGNMENT(runner) > maxlocalalign)
              maxlocalalign = BBL_ALIGNMENT(runner);
          } while (runner != dest);
          shorten = sval < 128 - (maxlocalalign - 1);
        }
        else
          shorten = FALSE;
        shorten = FALSE;
      }
      if (shorten)
      {
        /* make short jump */
        I386_OP_IMMEDSIZE(op) = 1;
        I386_OP_IMMEDIATE(op) = G_T_UINT32(offset);
        I386_INS_SET_CSIZE(last,  I386InsGetSize(last));
        change = TRUE;
      }
    }
  }

  return change;
}

/* look for short jumps that should become long jumps and change the
 * instructions accordingly */
t_bool I386AdjustJumpOffsetsUp(t_object * obj)
{
  int i;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    if (I386ChainAdjustJumpOffsetsUp (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]))))
      change = TRUE;
  }
  return change;
}

t_bool I386AdjustJumpOffsetsDown(t_object * obj)
{
  int i;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    if (I386ChainAdjustJumpOffsetsDown (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]))))
      change = TRUE;
  }
  return change;
}
/* }}} */

void I386UpdateControlFlowDisplacement (t_i386_ins *ins)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_i386_operand *op;
  t_address offset;

  if (!ins) return;

  bbl = I386_INS_BBL (ins);

  /* does this instruction have a displacement? */
  switch (I386_INS_OPCODE (ins))
  {
    case I386_CALL:
    case I386_JMP: case I386_Jcc: case I386_JECXZ:
    case I386_LOOP: case I386_LOOPZ: case I386_LOOPNZ:
      break;
    default:
      return;
  }

  op = I386_INS_SOURCE1 (ins);
  if (I386_OP_TYPE (op) != i386_optype_imm)
    return;	/* no displacement to update */

  /* find the destination */
  BBL_FOREACH_SUCC_EDGE (bbl, edge)
    if (CfgEdgeTestCategoryOr (edge, ET_IPJUMP | ET_JUMP | ET_CALL))
      break;
  ASSERT (edge, ("Could not find edge for @I", ins));

  offset = AddressSub (BBL_CADDRESS (CFG_EDGE_TAIL (edge)),
      AddressAdd (I386_INS_CADDRESS (ins), I386_INS_CSIZE (ins)));
  I386_OP_IMMEDIATE(op) = G_T_UINT32 (offset);

  /* special case: calls to non-available weak symbols get relocated
   * by the linker to calls to absolute address 0. This is represented
   * in diablo by a jump to the instruction's own basic block. While 
   * technically incorrect, this works because the code in this basic 
   * block is guaranteed never to be executed. However, for various 
   * reasons we want to have the call pointing to 0 again in the final
   * version of the program */
  if (I386_INS_OPCODE (ins) == I386_CALL &&
      CFG_EDGE_CAT (edge) == ET_JUMP &&
      CFG_EDGE_TAIL (edge) == bbl)
    I386_OP_IMMEDIATE (op) = -(G_T_UINT32(I386_INS_CADDRESS (ins)) + 5);
}

/* {{{ I386UpdateJumpDisplacements */
void I386UpdateJumpDisplacements(t_object * obj)
{
  int i;
  t_bbl * i_bbl;
  for (i=0; i<OBJECT_NCODES(obj); i++)
    CHAIN_FOREACH_BBL (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i])), i_bbl)
      I386UpdateControlFlowDisplacement (T_I386_INS(BBL_INS_LAST (i_bbl)));
}
/* }}} */

/* {{{ Recalculate the relocations */
void I386CalcRelocs(t_object * obj)
{
  t_reloc * rel;

  OBJECT_FOREACH_RELOC(obj,rel)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_INS)
    {
      t_i386_ins * ins = T_I386_INS(RELOC_FROM(rel));

      if (I386_INS_TYPE(ins) == IT_DATA)
      {
	t_uint32 data;
	t_uint8 *buf = (t_uint8 *)&data;
	t_i386_ins * iter;

	/*VERBOSE(0,( "relocing data in the code section at @G\n",INS_CADDRESS(ins))); */

#ifdef SELFMOD
	rel->e_offset = 0;
#endif
	if (!AddressIsNull(RELOC_FROM_OFFSET(rel))) FATAL(("Implement data with non-zero from_offset"));
	StackExec(RELOC_CODE(rel),rel,NULL,(char *) buf,TRUE,0,obj);

	/* TODO: this only works on little-endian machines */
	I386_INS_SET_DATA(ins, buf[0]);
	
	
#ifndef SELFMOD
	iter = (t_i386_ins *) I386_INS_INEXT(ins);  I386_INS_SET_DATA(iter, buf[1]);
	iter = (t_i386_ins *) I386_INS_INEXT(iter); I386_INS_SET_DATA(iter, buf[2]);
	iter = (t_i386_ins *) I386_INS_INEXT(iter); I386_INS_SET_DATA(iter, buf[3]);
#endif
      }
      else
      {
	t_address ret;
	t_i386_operand * op;
	t_reloc_ref * rr;
	t_reloc_ref * rr2;

	/* skip relocs from jump and call instructions (these are handled
	 * in AdjustJumpOffsets) */
/*	if (I386_INS_OPCODE(ins) == I386_JMP ||
	    I386_INS_OPCODE(ins) == I386_Jcc ||
	    I386_INS_OPCODE(ins) == I386_JECXZ ||
	    I386_INS_OPCODE(ins) == I386_CALL ||
	    I386_INS_OPCODE(ins) == I386_LOOP ||
	    I386_INS_OPCODE(ins) == I386_LOOPZ ||
	    I386_INS_OPCODE(ins) == I386_LOOP)
	{
	  if ((I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_imm)&&(!I386_INS_REFERS_TO(ins)))
	    continue;
	} */

        /* Ignore relocations that do do not write anything (that only exist
         * to keep things alive), otherwise their return value could overwrite
         * the result of a real relocation.
         *
         * We cannot easily assemble the entire instruction, relocate the
         * resulting bytes and then disassemble it again, because there
         * may be multiple representations for the instruction with different
         * lengths, and the relocation code would only be valid for one of
         * them
         */
        if ((strchr(RELOC_CODE(rel),'w')==0) &&
            (strchr(RELOC_CODE(rel),'W')==0) &&
            (strchr(RELOC_CODE(rel),'v')==0))
        {
          continue;
        }

	/* find out which operand has been relocated */
	rr = I386_INS_REFERS_TO(ins);
	ASSERT(rr,("relocations corrupt"));
	ASSERT(RELOC_REF_RELOC(rr) == rel || (RELOC_REF_NEXT(rr) && RELOC_REF_RELOC(RELOC_REF_NEXT(rr)) == rel),("relocations corrupt")); 

	if (RELOC_REF_RELOC(rr) == rel) 
	  rr2 = RELOC_REF_NEXT(rr);
	else
	{
	  rr2 = rr;
	  rr = RELOC_REF_NEXT(rr);
	}

	if (rr2 && AddressIsGt(RELOC_FROM_OFFSET(rel),RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr2))))
	  op = I386InsGetSecondRelocatedOp((t_i386_ins *)ins);
	else
	  op = I386InsGetFirstRelocatedOp((t_i386_ins *)ins);

	ret = StackExec(RELOC_CODE(rel),rel,NULL,NULL,FALSE,0,obj);
	/*	VERBOSE(0,("@R %s ins @I relocing 0x%x new @G\n",rel,RELOC_CODE(rel),ins,I386_OP_IMMEDIATE(op),ret));*/
	I386_OP_IMMEDIATE(op) = G_T_UINT32(ret);
      }
    }
    else if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_CODEBYTE)
    {
      DiabloBrokerCall("SmcCalcReloc",rel,obj);
    }
    else
    {
      /*      VERBOSE(0,("relocing @R with label %s old 0x%x new ",rel,RELOC_LABEL(rel)?RELOC_LABEL(rel):"<none>",RelocGetData(rel))); */
      t_address ret = StackExec(RELOC_CODE(rel),rel,NULL,SECTION_DATA(T_SECTION(RELOC_FROM(rel))),TRUE,0,obj);
      /*      VERBOSE(0,("0x%x\n",RelocGetData(rel))); */
      ASSERT(AddressIsNull(ret),("Could not properly execute @R",rel));
    }
  }
}
/* }}} */

/* {{{ Set the right instruction sizes */
t_bool I386SetInsSizes(t_object * obj)
{
  int i;
  t_uint8 buf[15];
  t_bbl * bbl;
  t_i386_ins * ins;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    CHAIN_FOREACH_BBL (T_BBL (SECTION_TMP_BUF (OBJECT_CODE (obj)[i])), bbl)
    {
      BBL_FOREACH_I386_INS(bbl,ins)
      {
	t_uint32 len = I386AssembleIns(ins,buf);
	if (len != G_T_UINT32(I386_INS_CSIZE(ins)))
	{
	  change = TRUE;
	  I386_INS_SET_CSIZE(ins, AddressNew32(len));
	}
      }
    }
  }
  return change;
}
/* }}} */

/* {{{ set the right immediate size for operands */
static void DoSetSize(t_i386_ins * ins, t_i386_operand * op)
{
  static t_uint32 next_proposed[] = {1,4,4,4,1000};

  t_uint32 imm = I386_OP_IMMEDIATE(op);
  t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);
  t_uint32 orig_immedsize = I386_OP_IMMEDSIZE(op);
  t_uint32 proposed_immedsize = 0;
  t_i386_opcode_entry * forms[10];
  int nfitting;

  if (I386_OP_TYPE(op) != i386_optype_imm && I386_OP_TYPE(op) != i386_optype_mem)
    return;
  if (I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED)
    return;
  if (orig_immedsize == 2) 
    /* stay away from 16-bit immediates. they are complicated and not worth the hassle */
    return;

  if (imm == 0)
    proposed_immedsize = 0;
  else if (imm > 0 && imm < 256)
    proposed_immedsize = 1;
  else if (simm >= -128 && simm < 128)
    proposed_immedsize = 1;
  else
    proposed_immedsize = 4;

  while (proposed_immedsize <= 4)
  {
    I386_OP_IMMEDSIZE(op) = proposed_immedsize;
    nfitting = I386GetPossibleEncodings(ins,forms);
    if (nfitting) 
    {
      /*if (proposed_immedsize != orig_immedsize) VERBOSE(0,("changed: @I from %d to %d\n",ins,orig_immedsize,proposed_immedsize));*/
      return; /* found a valid encoding */
    }
    proposed_immedsize = next_proposed[proposed_immedsize];
  }

  /* if we get here, we couldn't find a good encoding. */
  I386_OP_IMMEDSIZE(op) = orig_immedsize;
  FATAL(("@I: found no good encoding",ins));
}

void I386SetImmedSizes(t_object * obj)
{
  int i;
  t_bbl * bbl;
  t_i386_ins * ins;
  /*static int teller = 0; */
  
  for  (i=0; i<OBJECT_NCODES(obj); i++)
  {
    CHAIN_FOREACH_BBL ((t_bbl *)SECTION_TMP_BUF(OBJECT_CODE(obj)[i]), bbl)
    {
      /*if (teller++ < diablosupport_options.debugcounter) */
      BBL_FOREACH_I386_INS(bbl,ins)
      {
	/* don't do jumps and calls and stuff, they are done during the 
	 * deflowgraph loop */
	if (I386_INS_TYPE(ins) == IT_BRANCH &&
	    I386_OP_TYPE (I386_INS_SOURCE1 (ins)) == i386_optype_imm)
	  continue;

	DoSetSize(ins,I386_INS_DEST(ins));
	DoSetSize(ins,I386_INS_SOURCE1(ins));
	DoSetSize(ins,I386_INS_SOURCE2(ins));
      }
    }
  }
}
/* }}} */

void I386DoSetSize(t_i386_ins * ins, t_i386_operand * op)
{
  DoSetSize(ins, op);
}

/* {{{ fixpoint that places sections, updates relocations and adjusts 
 * the operand sizes of jumps */
#define UPDATE_ADDRESSES	for (i=0; i<OBJECT_NCODES(obj); i++) AssignAddressesInChain(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]),SECTION_CADDRESS(OBJECT_CODE(obj)[i]))
void I386DeflowFixpoint(t_object * obj)
{
  t_bool loop = TRUE;
  int i;

  I386SetImmedSizes(obj);
  I386SetInsSizes(obj);
  I386KillUselessJumps(obj);
  UPDATE_ADDRESSES;
restart_loop:
  while (loop)
  {
    loop = FALSE;
    ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
    UPDATE_ADDRESSES;
    if (I386AdjustJumpOffsetsUp(obj)) 
    {
      loop = TRUE;
      UPDATE_ADDRESSES;
    }
    if (I386AdjustJumpOffsetsDown(obj)) 
    {
      loop = TRUE;
      UPDATE_ADDRESSES;
    }
  }
  I386UpdateJumpDisplacements(obj);
  I386CalcRelocs(obj);
  /* there is a *slight* possibility that the recalculation of the relocations
   * causes the length of some instructions to change. if so, we have to start
   * it all over... */
  if (I386SetInsSizes(obj))
  {
    loop = TRUE;
    goto restart_loop;
  }
}
#undef UPDATE_ADDRESSES
/* }}} */

/* C++ member functions must be aligned to two bytes (because a member pointer
 * that's odd has a special meaning, see the Itanium C++ ABI 2.3)
 */
/* I386AlignFunctionEntries {{{*/
void I386AlignFunctionEntries(t_cfg *cfg)
{
  t_function *fun;

  CFG_FOREACH_FUN(cfg, fun)
  {
    t_bbl *bbl;
    if (FUNCTION_IS_HELL(fun))
      continue;
    bbl = FUNCTION_BBL_FIRST(fun);
    if ((BBL_ALIGNMENT(bbl)<2) &&
        !(G_T_UINT32(BBL_ALIGNMENT_OFFSET(bbl)) & 1))
    {
      BBL_SET_ALIGNMENT(bbl,2);
    }
  }
}
/*}}}*/


/* {{{ regular deflowgraph algorithm */
void I386Deflowgraph(t_object *obj)
{
  t_cfg * cfg = OBJECT_CFG(obj);
  t_section *code = OBJECT_CODE(obj)[0];
  t_chain_holder chains;

  ASSERT(cfg,("Cfg not found!"));

  I386AlignFunctionEntries(cfg);

  I386CreateChains(cfg,&chains);

  if (diabloi386_options.orderseed==0)
    I386ClusterChainsForMinimalJumpOffsets(&chains);
  else
    I386OrderChainsRandomly(&chains);

  DiabloBrokerCall("AfterChainsOrdered", cfg, &chains);

  MergeAllChains(&chains);

  SECTION_SET_TMP_BUF(code, chains.chains[0]);

  I386DeflowFixpoint(obj);
  
  if(diabloflowgraph_options.listfile)
    I386ListFinalProgram(chains.chains[0]);

  VERBOSE(0,("old object entry was @G",OBJECT_ENTRY(obj)));
  OBJECT_SET_ENTRY(obj, INS_CADDRESS(BBL_INS_FIRST(CFG_ENTRY(cfg)->entry_bbl)));
  VERBOSE(0,("new object entry is @G", OBJECT_ENTRY(obj)));

  DiabloBrokerCall("CreateBblTranslateList",cfg);

  Free(chains.chains);
}
/* }}} */

/* {{{ debug code: list the final program: void I386ListFinalProgram(t_object *obj) */
void I386ListFinalProgram(t_bbl * bbl)
{
  char * filename = StringDup(diabloflowgraph_options.listfile);
  FILE * f = fopen(filename,"w");
  if (f)
  {
    t_i386_ins * ins;
    for(;bbl;bbl=BBL_NEXT_IN_CHAIN(bbl))
      for(ins =T_I386_INS(BBL_INS_FIRST(bbl)); ins; ins=I386_INS_INEXT(ins))
	FileIo(f,"@I\n",ins); 
    fclose(f);
  }
  else
    VERBOSE(0,("Could not open %s for writing!",filename));

  Free(filename);
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker: */
