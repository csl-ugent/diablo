/* new amd64 layout and deflowgraph algorithm, by Dominique
 * this is intended to be a lot more general than the old 
 * algorithm, with support for splitting the merged code
 * section as needed for linux kernel optimization */

#include <stdlib.h>
#include <string.h>
#include <diabloamd64.h>

void Amd64ListFinalProgram(t_bbl * bbl);
  
#define SHORT_DISPL_HI_BOUND CHAR_MAX
#define SHORT_DISPL_LO_BOUND CHAR_MIN
#define FULL_DISPL_HI_BOUND  INT_MAX
#define FULL_DISPL_LO_BOUND  INT_MIN                                

/* some handy macros */
#define BBL_CHAIN_INDEX_INT(bbl)	((int)BBL_PREV_IN_CHAIN(BBL_FIRST_IN_CHAIN(bbl)))
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
void Amd64CreateChains(t_cfg * cfg, t_chain_holder * ch)
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
void Amd64MergeTwoChains(t_bbl * ca, t_bbl * cb)
{
  t_address length_of_ca = BBL_CHAIN_SIZE(ca);
  MergeChains(ca,cb);
  AssignAddressesInChain(cb,length_of_ca);
}
/* }}} */

/* {{{ cluster chains to in order to encode jump offsets as efficient as possible */
/* {{{ find candidate for appending */
static int Amd64FindAppendable(t_chain_holder * ch, int index)
{
  t_bbl * bbl;
  t_amd64_ins * ins;
  t_cfg_edge * edge;
  int ret = -1;

  /* BBL_PREV_IN_CHAIN of the head of the chain contains the index in the
   * chains array. This slightly complicates walking backwardly through the
   * chain */
  for (bbl = BBL_LAST_IN_CHAIN(ch->chains[index]); bbl;
       bbl = (bbl == BBL_FIRST_IN_CHAIN(bbl) ? NULL :  BBL_PREV_IN_CHAIN(bbl)))
  {
    t_uint64 dist_to_end_of_chain = G_T_UINT64(AddressSub(
	  BBL_CHAIN_SIZE(ch->chains[index]),
	  AddressAdd(BBL_CADDRESS(bbl),BBL_CSIZE(bbl))));
    if (dist_to_end_of_chain >= 128) break;

    ins = T_AMD64_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;

    if (AMD64_INS_TYPE(ins) == IT_BRANCH &&
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm && 
	AMD64_INS_OPCODE(ins) != AMD64_RET &&
	AMD64_INS_OPCODE(ins) != AMD64_CALL)
    {
      t_bbl * tail;
      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	if (CfgEdgeTestCategoryOr(edge,ET_JUMP|ET_IPJUMP|ET_CALL)) /* ET_CALL still needed here for instrumentation */
	  break;
      ASSERT(edge,("need edge @ieB",bbl));

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
static int Amd64FindPrependable(t_chain_holder * ch, int index)
{
  t_bbl * bbl;
  t_amd64_ins * ins;
  t_cfg_edge * edge;

  for (bbl = ch->chains[index]; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    t_uint64 from_start = G_T_UINT64(AddressAdd(BBL_CADDRESS(bbl),BBL_CSIZE(bbl)));
    if (from_start >= 128)
      break;

    ins = T_AMD64_INS(BBL_INS_LAST(bbl));
    if (!ins) continue;

    if (AMD64_INS_TYPE(ins) == IT_BRANCH &&
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm &&
	AMD64_INS_OPCODE(ins) != AMD64_RET &&
	AMD64_INS_OPCODE(ins) != AMD64_CALL)
    {
      t_bbl * tail;
      int chainno;

      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	if (CfgEdgeTestCategoryOr(edge,ET_JUMP|ET_IPJUMP|ET_CALL))
	  break;
      ASSERT(edge,("need edge"));
      tail = CFG_EDGE_TAIL(edge);

      chainno = BBL_CHAIN_INDEX_INT(tail);
      if (chainno != index && BblInChainset(tail,ch))
      {
	t_uint64 from_end = G_T_UINT64(AddressSub(BBL_CHAIN_SIZE(ch->chains[chainno]),BBL_CADDRESS(tail)));
	if (from_start + from_end < 128)
	  return chainno;
      }
    }
  }
  
  return -1;
}
/* }}} */
/* {{{ Amd64ClusterChainsForMinimalJumpOffsets */
void Amd64ClusterChainsForMinimalJumpOffsets(t_chain_holder * ch)
{
  int i;
  t_address null = AddressSub(BBL_CADDRESS(ch->chains[0]),BBL_CADDRESS(ch->chains[0]));

  for (i=0; i<ch->nchains; i++)
  {
    AssignAddressesInChain(ch->chains[i],null);
    if (ch->chains[i])
    {
      BBL_SET_CHAIN_INDEX(ch->chains[i],  (void*)i);
    }
  }
  
  /* {{{ try to find chains that can be appended */
  for (i = 0; i < ch->nchains; i++)
  {
    int j;
    if (ch->chains[i] == NULL) continue;

    while ((j = Amd64FindAppendable(ch,i)) != -1)
    {
      Amd64MergeTwoChains(ch->chains[i],ch->chains[j]);
      ch->chains[j] = NULL;
    }
  } /* }}} */
    
  /* {{{ try to find chains that can be prepended */
  for (i = 0; i < ch->nchains; i++)
  {
    int j;
    if (ch->chains[i] == NULL) continue;
    while ((j = Amd64FindPrependable(ch,i)) != -1)
    {
      Amd64MergeTwoChains(ch->chains[j],ch->chains[i]);
      if (j > i)
      {
	ch->chains[i] = NULL;
	break;
      }
      else
      {
	ch->chains[i] = ch->chains[j];
	ch->chains[j] = NULL;
	BBL_SET_CHAIN_INDEX(ch->chains[i],  (void*)i);
      }
    }
  } /* }}} */
} /* }}} */ 
/* }}} */

/* {{{ KillUselessJumps */
void KillUselessJumps(t_object * obj)
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
      t_amd64_ins * ins = T_AMD64_INS(BBL_INS_LAST(bbl));
      t_cfg_edge * edge;

      if (!ins) continue;
      if (AMD64_INS_OPCODE(ins) != AMD64_JMP && 
	  AMD64_INS_OPCODE(ins) != AMD64_Jcc && 
	  AMD64_INS_OPCODE(ins) != AMD64_JRCXZ) 
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
	  t_bbl * bbl = AMD64_INS_BBL(ins);
	  t_amd64_ins * iter;
	  t_regset usedef = NullRegs;
	  BBL_FOREACH_AMD64_INS(bbl,iter)
	  {
	    RegsetSetUnion(usedef,AMD64_INS_REGS_USE(iter));
	    RegsetSetUnion(usedef,AMD64_INS_REGS_DEF(iter));
	  }
	  if (RegsetIn(usedef,AMD64_REG_CR0) ||
	      RegsetIn(usedef,AMD64_REG_CR1) ||
	      RegsetIn(usedef,AMD64_REG_CR2) ||
	      RegsetIn(usedef,AMD64_REG_CR3) ||
	      RegsetIn(usedef,AMD64_REG_CR4))
	    continue;
	}

	Amd64InsKill(ins);
	nkills++;
	CFG_EDGE_SET_CAT(edge, (CFG_EDGE_CAT(edge) == ET_JUMP) ? ET_FALLTHROUGH : ET_IPFALLTHRU);
      }
    }
  }
  VERBOSE(0,("[KillUselessJumps] Killed %d useless jumps\n", nkills));
}
/* }}} */

/* {{{ adjust jump offsets (make them 1 byte if possible, in 4 if necessary) */

t_bool ChainAdjustJumpOffsetsUp (t_bbl *chain)
{
  t_bool change = FALSE;
  t_bbl *i_bbl;

  CHAIN_FOREACH_BBL (chain, i_bbl)
  {
    t_address offset;
    t_int32 sval;
    t_bbl * dest;
    t_cfg_edge * i_edge;
    t_amd64_ins * last = T_AMD64_INS(BBL_INS_LAST(i_bbl));
    t_amd64_operand * op;

    if (!last || 
	(AMD64_INS_OPCODE(last) != AMD64_JMP && 
	 AMD64_INS_OPCODE(last) != AMD64_Jcc && 
	 AMD64_INS_OPCODE(last) != AMD64_JRCXZ && 
	 AMD64_INS_OPCODE(last) != AMD64_LOOP && 
	 AMD64_INS_OPCODE(last) != AMD64_LOOPZ && 
	 AMD64_INS_OPCODE(last) != AMD64_LOOPNZ))
      continue;

    /* we're only interested in jumps with an immediate short offset */
    op = AMD64_INS_SOURCE1(last);
    if (AMD64_OP_TYPE(op) != amd64_optype_imm || AMD64_OP_IMMEDSIZE(op) != 1)
      continue;

    /* find the destination of the control transfer */
    BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
      if (CfgEdgeTestCategoryOr(i_edge,ET_CALL | ET_JUMP | ET_IPJUMP))
	break;
    ASSERT(i_edge,("Could not find edge for @I",last));
    dest = CFG_EDGE_TAIL(i_edge);

    offset = AddressSub (BBL_CADDRESS (dest),
	AddressAdd (AMD64_INS_CADDRESS (last), AMD64_INS_CSIZE (last)));
    sval = G_T_UINT64 (offset);

    if (sval < -128 || sval > 127 ||
	AMD64_INS_HAS_FLAG (last, AMD64_IF_JMP_FORCE_4BYTE))
    {
      if (AMD64_INS_OPCODE(last) != AMD64_JRCXZ 
	  && AMD64_INS_OPCODE(last) != AMD64_LOOP
	  && AMD64_INS_OPCODE(last) != AMD64_LOOPZ 
	  && AMD64_INS_OPCODE(last) != AMD64_LOOPNZ)
      {
	/* make long jump */
	AMD64_OP_IMMEDSIZE(op) = 4;
	AMD64_OP_IMMEDIATE(op) = G_T_UINT64(offset);
	AMD64_INS_SET_CSIZE(last,  Amd64InsGetSize(last));
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
	t_amd64_ins * jmp_ft, * jmp_dest;
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
	jmp_ft = T_AMD64_INS(InsNewForBbl(blk_ft));
	InsAppendToBbl(T_INS(jmp_ft),blk_ft);
	Amd64InstructionMakeJump(jmp_ft);
	jmp_dest = T_AMD64_INS(InsNewForBbl(blk_dest));
	InsAppendToBbl(T_INS(jmp_dest),blk_dest);
	Amd64InstructionMakeJump(jmp_dest);

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

t_bool ChainAdjustJumpOffsetsDown (t_bbl *chain)
{
  t_bbl *i_bbl;
  t_bool change = FALSE;

  CHAIN_FOREACH_BBL (chain, i_bbl)
  {
    t_address offset;
    t_int32 sval;
    t_bbl * dest;
    t_cfg_edge * i_edge;
    t_amd64_ins * last = T_AMD64_INS(BBL_INS_LAST(i_bbl));
    t_amd64_operand * op;

    if (!last || 
	(AMD64_INS_OPCODE(last) != AMD64_JMP && 
	 AMD64_INS_OPCODE(last) != AMD64_Jcc))
      continue;

    /* we're only interested in jumps with an immediate long offset */
    op = AMD64_INS_SOURCE1(last);
    if (AMD64_OP_TYPE(op) != amd64_optype_imm || AMD64_OP_IMMEDSIZE(op) != 4)
      continue;

    /* find the destination of the control transfer */
    BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
      if (CfgEdgeTestCategoryOr(i_edge,ET_CALL | ET_JUMP | ET_IPJUMP))
	break;
    ASSERT(i_edge,("Could not find edge for @I",last));
    dest = CFG_EDGE_TAIL(i_edge);

    offset = AddressSub (BBL_CADDRESS (dest),
	AddressAdd (AMD64_INS_CADDRESS (last), AMD64_INS_CSIZE (last)));
    sval = AddressExtractInt32 (offset);

    if (sval >= -128 && sval < 128 &&
	!AMD64_INS_HAS_FLAG (last, AMD64_IF_JMP_FORCE_4BYTE))
    {
      /* make short jump */
      AMD64_OP_IMMEDSIZE(op) = 1;
      AMD64_OP_IMMEDIATE(op) = G_T_UINT64(offset);
      AMD64_INS_SET_CSIZE(last,  Amd64InsGetSize(last));
      change = TRUE;
    }
  }

  return change;
}

/* look for short jumps that should become long jumps and change the
 * instructions accordingly */
t_bool AdjustJumpOffsetsUp(t_object * obj)
{
  int i;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    if (ChainAdjustJumpOffsetsUp (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]))))
      change = TRUE;
  }
  return change;
}

t_bool AdjustJumpOffsetsDown(t_object * obj)
{
  int i;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    if (ChainAdjustJumpOffsetsDown (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]))))
      change = TRUE;
  }
  return change;
}
/* }}} */

void Amd64UpdateControlFlowDisplacement (t_amd64_ins *ins)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_amd64_operand *op;
  t_address offset;

  if (!ins) return;

  bbl = AMD64_INS_BBL (ins);

  /* does this instruction have a displacement? */
  switch (AMD64_INS_OPCODE (ins))
  {
    case AMD64_CALL:
    case AMD64_JMP: case AMD64_Jcc: case AMD64_JRCXZ:
    case AMD64_LOOP: case AMD64_LOOPZ: case AMD64_LOOPNZ:
      break;
    default:
      return;
  }

  op = AMD64_INS_SOURCE1 (ins);
  if (AMD64_OP_TYPE (op) != amd64_optype_imm)
    return;	/* no displacement to update */

  /* find the destination */
  BBL_FOREACH_SUCC_EDGE (bbl, edge)
    if (CfgEdgeTestCategoryOr (edge, ET_IPJUMP | ET_JUMP | ET_CALL))
      break;
  ASSERT (edge, ("Could not find edge for @I", ins));

  offset = AddressSub (BBL_CADDRESS (CFG_EDGE_TAIL (edge)),
      AddressAdd (AMD64_INS_CADDRESS (ins), AMD64_INS_CSIZE (ins)));
  AMD64_OP_IMMEDIATE(op) = G_T_UINT64 (offset);

  /* special case: calls to non-available weak symbols get relocated
   * by the linker to calls to absolute address 0. This is represented
   * in diablo by a jump to the instruction's own basic block. While 
   * technically incorrect, this works because the code in this basic 
   * block is guaranteed never to be executed. However, for various 
   * reasons we want to have the call pointing to 0 again in the final
   * version of the program */
  if (AMD64_INS_OPCODE (ins) == AMD64_CALL &&
      CFG_EDGE_CAT (edge) == ET_JUMP &&
      CFG_EDGE_TAIL (edge) == bbl)
    AMD64_OP_IMMEDIATE (op) = -(G_T_UINT64(AMD64_INS_CADDRESS (ins)) + 5);
}

/* {{{ UpdateJumpDisplacements */
void UpdateJumpDisplacements(t_object * obj)
{
  int i;
  t_bbl * i_bbl;
  for (i=0; i<OBJECT_NCODES(obj); i++)
    CHAIN_FOREACH_BBL (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i])), i_bbl)
      Amd64UpdateControlFlowDisplacement (T_AMD64_INS(BBL_INS_LAST (i_bbl)));
}
/* }}} */

/* {{{ Recalculate the relocations */
void Amd64CalcRelocs(t_object * obj)
{
  t_reloc * rel;

  OBJECT_FOREACH_RELOC(obj,rel)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_INS)
    {
      t_amd64_ins * ins = T_AMD64_INS(RELOC_FROM(rel));

      if (AMD64_INS_TYPE(ins) == IT_DATA)
      {
	t_uint32 data;
	t_uint8 *buf = (t_uint8 *)&data;
	t_amd64_ins * iter;

	/*VERBOSE(0,( "relocing data in the code section at @G\n",INS_CADDRESS(ins))); */

#ifdef SELFMOD
	rel->e_offset = 0;
#endif
	if (!AddressIsNull(RELOC_FROM_OFFSET(rel))) FATAL(("Implement data with non-zero from_offset"));
	StackExec(RELOC_CODE(rel),rel,NULL,(char *) buf,TRUE,0,obj);

	/* TODO: this only works on little-endian machines */
	AMD64_INS_SET_DATA(ins, buf[0]);
	
	
#ifndef SELFMOD
	iter = (t_amd64_ins *) AMD64_INS_INEXT(ins);  AMD64_INS_SET_DATA(iter, buf[1]);
	iter = (t_amd64_ins *) AMD64_INS_INEXT(iter); AMD64_INS_SET_DATA(iter, buf[2]);
	iter = (t_amd64_ins *) AMD64_INS_INEXT(iter); AMD64_INS_SET_DATA(iter, buf[3]);
#endif
      }
      else
      {
	t_address ret;
	t_amd64_operand * op;
	t_reloc_ref * rr;
	t_reloc_ref * rr2;

	/* skip relocs from jump and call instructions (these are handled
	 * in AdjustJumpOffsets) */
	if (AMD64_INS_OPCODE(ins) == AMD64_JMP ||
	    AMD64_INS_OPCODE(ins) == AMD64_Jcc ||
	    AMD64_INS_OPCODE(ins) == AMD64_JRCXZ ||
	    AMD64_INS_OPCODE(ins) == AMD64_CALL ||
	    AMD64_INS_OPCODE(ins) == AMD64_LOOP ||
	    AMD64_INS_OPCODE(ins) == AMD64_LOOPZ ||
	    AMD64_INS_OPCODE(ins) == AMD64_LOOP)
	{
	  if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_imm)
	    continue;
	}

	/* find out which operand has been relocated */
	rr = AMD64_INS_REFERS_TO(ins);
	ASSERT(rr,("relocations corrupt"));
	ASSERT(RELOC_REF_RELOC(rr) == rel || (RELOC_REF_NEXT(rr) && RELOC_REF_RELOC(RELOC_REF_NEXT(rr))== rel),("relocations corrupt")); 

	if (RELOC_REF_RELOC(rr) == rel) 
	  rr2 = RELOC_REF_NEXT(rr);
	else
	{
	  rr2 = rr;
	  rr = RELOC_REF_NEXT(rr);
	}

	if (rr2 && AddressIsGt(RELOC_FROM_OFFSET(rel),RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr2))))
	  op = Amd64InsGetSecondRelocatedOp((t_amd64_ins *)ins);
	else
	  op = Amd64InsGetFirstRelocatedOp((t_amd64_ins *)ins);

	ret = StackExec(RELOC_CODE(rel),rel,NULL,NULL,FALSE,0,obj);
	/*	VERBOSE(0,("@R %s ins @I relocing 0x%x new @G\n",rel,RELOC_CODE(rel),ins,AMD64_OP_IMMEDIATE(op),ret));*/
	AMD64_OP_IMMEDIATE(op) = G_T_UINT64(ret);
      }
    }
    else if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_CODEBYTE)
    {
      DiabloBrokerCall("SmcCalcReloc",rel,obj);
    }
    else
    {
      /*      VERBOSE(0,("relocing @R %s in section %s at @G old 0x%x new ",rel,RELOC_CODE(rel),T_SECTION(RELOC_FROM(rel))->name,AddressAdd(RELOC_FROM(rel)->caddress,RELOC_FROM_OFFSET(rel)), *(t_uint64 *)(T_SECTION(RELOC_FROM(rel))->data + RELOC_FROM_OFFSET(rel)))); */
      t_address ret = StackExec(RELOC_CODE(rel),rel,NULL,SECTION_DATA(T_SECTION(RELOC_FROM(rel))),TRUE,0,obj);
      /*      VERBOSE(0,("%x\n",*(t_uint64 *)(T_SECTION(RELOC_FROM(rel))->data + RELOC_FROM_OFFSET(rel)))); */
      ASSERT(AddressIsNull(ret),("Could not properly execute @R",rel));
    }
  }
}
/* }}} */

/* {{{ Set the right instruction sizes */
t_bool SetInsSizes(t_object * obj)
{
  int i;
  t_uint8 buf[15];
  t_bbl * bbl;
  t_amd64_ins * ins;
  t_bool change = FALSE;

  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    CHAIN_FOREACH_BBL (T_BBL (SECTION_TMP_BUF (OBJECT_CODE (obj)[i])), bbl)
    {
      BBL_FOREACH_AMD64_INS(bbl,ins)
      {
	t_uint32 len = Amd64AssembleIns(ins,buf);
	if (len != G_T_UINT64(AMD64_INS_CSIZE(ins)))
	{
	  change = TRUE;
	  AMD64_INS_SET_CSIZE(ins, AddressNew64(len));
	}
      }
    }
  }
  return change;
}
/* }}} */

/* {{{ set the right immediate size for operands */
static void DoSetSize(t_amd64_ins * ins, t_amd64_operand * op)
{
  static t_uint32 next_proposed[] = {1,4,4,4,8,1000};

  t_uint64 imm = AMD64_OP_IMMEDIATE(op);
  t_int64 simm = (t_int64) AMD64_OP_IMMEDIATE(op);
  t_uint64 orig_immedsize = AMD64_OP_IMMEDSIZE(op);
  t_uint64 proposed_immedsize = 0;
  t_amd64_opcode_entry * forms[10];
  int nfitting;
  
  if (AMD64_OP_TYPE(op) != amd64_optype_imm && AMD64_OP_TYPE(op) != amd64_optype_mem)
    return;
  if (AMD64_OP_FLAGS(op) & AMD64_OPFLAG_ISRELOCATED)
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
  else if (imm > 0 && imm < 4294967296ULL)
    proposed_immedsize = 4;
  else if (simm >= -2147483648LL && simm < 2147483648LL)
    proposed_immedsize = 4;
  else
    proposed_immedsize = 8;

  while (proposed_immedsize <= 8)
  {
    AMD64_OP_IMMEDSIZE(op) = proposed_immedsize;
    nfitting = Amd64GetPossibleEncodings(ins,forms);
    if (nfitting) 
    {
      /*if (proposed_immedsize != orig_immedsize) VERBOSE(0,("changed: @I from %d to %d\n",ins,orig_immedsize,proposed_immedsize));*/
/*      if(AMD64_OP_IMMEDSIZE(op) != orig_immedsize){
	VERBOSE(0,("changed immedsize of @I",ins));
	printf("old immsize: %d,new immsize: %d",orig_immedsize,AMD64_OP_IMMEDSIZE(op));
      }*/
      return; /* found a valid encoding */
    }
    proposed_immedsize = next_proposed[proposed_immedsize];
  }

  /* if we get here, we couldn't find a good encoding. */
  AMD64_OP_IMMEDSIZE(op) = orig_immedsize;
  FATAL(("@I: found no good encoding",ins));
}

void SetImmedSizes(t_object * obj)
{
  int i;
  t_bbl * bbl;
  t_amd64_ins * ins;
  /*static int teller = 0; */
  
  for  (i=0; i<OBJECT_NCODES(obj); i++)
  {
    CHAIN_FOREACH_BBL ((t_bbl *)SECTION_TMP_BUF(OBJECT_CODE(obj)[i]), bbl)
    {
      /*if (teller++ < diablosupport_options.debugcounter) */
      BBL_FOREACH_AMD64_INS(bbl,ins)
      {
	/* don't do jumps and calls and stuff, they are done during the 
	 * deflowgraph loop */
	if (AMD64_INS_TYPE(ins) == IT_BRANCH &&
	    AMD64_OP_TYPE (AMD64_INS_SOURCE1 (ins)) == amd64_optype_imm)
	  continue;

	DoSetSize(ins,AMD64_INS_DEST(ins));
	DoSetSize(ins,AMD64_INS_SOURCE1(ins));
	DoSetSize(ins,AMD64_INS_SOURCE2(ins));
      }
    }
  }
}
/* }}} */

/* {{{ fixpoint that places sections, updates relocations and adjusts 
 * the operand sizes of jumps */
#define UPDATE_ADDRESSES	for (i=0; i<OBJECT_NCODES(obj); i++) AssignAddressesInChain(SECTION_TMP_BUF(OBJECT_CODE(obj)[i]),SECTION_CADDRESS(OBJECT_CODE(obj)[i]))
void Amd64DeflowFixpoint(t_object * obj)
{
  t_bool loop = TRUE;
  int i;

  SetImmedSizes(obj);
  SetInsSizes(obj);
  KillUselessJumps(obj);
  UPDATE_ADDRESSES;
restart_loop:
  while (loop)
  {
    loop = FALSE;
    
    ObjectPlaceSections(obj, FALSE, TRUE, TRUE);
    
    UPDATE_ADDRESSES;
    if (AdjustJumpOffsetsUp(obj)) 
    {
      loop = TRUE;
      UPDATE_ADDRESSES;
    }
    if (AdjustJumpOffsetsDown(obj)) 
    {
      loop = TRUE;
      UPDATE_ADDRESSES;
    }
  }
  UpdateJumpDisplacements(obj);
  Amd64CalcRelocs(obj);
  /* there is a *slight* possibility that the recalculation of the relocations
   * causes the length of some instructions to change. if so, we have to start
   * it all over... */
  if (SetInsSizes(obj))
  {
    loop = TRUE;
    goto restart_loop;
  }
}
#undef UPDATE_ADDRESSES
/* }}} */

/* {{{ regular deflowgraph algorithm */
void Amd64Deflowgraph(t_object *obj)
{  
  t_cfg * cfg = OBJECT_CFG(obj);
  t_section *code = OBJECT_CODE(obj)[0];
  t_chain_holder chains;

  ASSERT(cfg,("Cfg not found!"));
  
  Amd64CreateChains(cfg,&chains);

  Amd64ClusterChainsForMinimalJumpOffsets(&chains);

  MergeAllChains(&chains);

  SECTION_SET_TMP_BUF(code, chains.chains[0]);
 
  Amd64DeflowFixpoint(obj);
  
  if(diabloflowgraph_options.listfile)
    Amd64ListFinalProgram(chains.chains[0]);

  VERBOSE(0,("old object entry was @G\n",OBJECT_ENTRY(obj)));
  OBJECT_SET_ENTRY(obj, INS_CADDRESS(BBL_INS_FIRST(CFG_ENTRY(cfg)->entry_bbl)));
  VERBOSE(0,("new object entry is @G\n", OBJECT_ENTRY(obj)));

  DiabloBrokerCall("CreateBblTranslateList",cfg);

  ComputeCodeSizeGain(obj);

  Free(chains.chains);
}
/* }}} */

/* {{{ debug code: list the final program: void Amd64ListFinalProgram(t_object *obj) */
void Amd64ListFinalProgram(t_bbl * bbl)
{
  char * filename = StringDup(diabloflowgraph_options.listfile);
  FILE * f = fopen(filename,"w");
  if (f)
  {
    t_amd64_ins * ins;
    for(;bbl;bbl=BBL_NEXT_IN_CHAIN(bbl))
      for(ins =T_AMD64_INS(BBL_INS_FIRST(bbl)); ins; ins=AMD64_INS_INEXT(ins))
	FileIo(f,"@I\n",ins); 
    fclose(f);
  }
  else
    VERBOSE(0,("Could not open %s for writing!",filename));

  Free(filename);
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker: */
