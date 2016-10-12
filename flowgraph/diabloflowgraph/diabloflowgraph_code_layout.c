/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>

/* {{{ assign addresses in chain */
t_address
AssignAddressesInChain (t_bbl * bbl, t_address start)
{
  for (; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    int required_alignment = BBL_ALIGNMENT(bbl);
    int required_offset = BBL_ALIGNMENT_OFFSET(bbl);
    ASSERT((required_alignment==0) || (required_alignment>required_offset),("required alignment %d <= required offset %d for @eiB",required_alignment,required_offset,bbl));
    switch (required_alignment)
    {
      case 0:
      case 1:
        /* always ok */
        break;
      case 2:
      case 4:
      case 8:
      case 16:
      case 32:
      case 64:
        start = AddressAddUint32(AddressAlign(required_alignment,AddressSubUint32(start,required_offset)),required_offset);
        break;
      default:
        /* TODO: make the maximum required alignment architecture-dependent */
        break;
    }
    BblSetAddressSuper (bbl, start);
    start = AddressAdd (start, BBL_CSIZE(bbl));
  }
  return start;
} /* }}} */

/* {{{ create chains of basic blocks */
#define ET_MUSTCHAIN ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE ET_CALL | ET_SWI
/*TODO: SWITCH TABLES!!!*/
void
CreateChains (t_cfg * cfg, t_chain_holder * ch)
{
  t_bbl *i_bbl;
  t_uint32 nbbls = 0, nchains = 0;
  int i;


  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    BBL_SET_FIRST_IN_CHAIN(i_bbl, NULL);
    BBL_SET_LAST_IN_CHAIN(i_bbl, NULL);
    BBL_SET_NEXT_IN_CHAIN(i_bbl, NULL);
    BBL_SET_PREV_IN_CHAIN(i_bbl, NULL);
    /* count all bbls except return blocks and hell nodes (those will not be chained) */
    if (!BBL_IS_HELL (i_bbl) && (!BBL_FUNCTION(i_bbl) || (FunctionGetExitBlock (BBL_FUNCTION(i_bbl)) != i_bbl)))
      nbbls++;
  }
  nchains = nbbls;
  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    t_cfg_edge *i_edge;


    /* skip return blocks and hell nodes */
    if (BBL_IS_HELL (i_bbl) || (BBL_FUNCTION(i_bbl) && (FunctionGetExitBlock (BBL_FUNCTION(i_bbl)) == i_bbl)))
      continue;

    BBL_FOREACH_SUCC_EDGE(i_bbl, i_edge)
    {
      if (CfgEdgeTestCategoryOr (i_edge, ET_MUSTCHAIN))
      {
        break;
      }
      if (CfgEdgeTestCategoryOr (i_edge, ET_MUSTCHAINMAYBE) && CFG_EDGE_CORR(i_edge))
      {
        i_edge = CFG_EDGE_CORR(i_edge);
        break;
      }
    }

    if (i_edge)
    {
      t_bbl *tail = CFG_EDGE_TAIL(i_edge);

      BBL_SET_PREV_IN_CHAIN(tail, i_bbl);
      BBL_SET_NEXT_IN_CHAIN(i_bbl, tail);
      nchains--; /* each time two blocks get chained, the number of chains decreases by one */
    }
  }

  ch->chains = Malloc (nchains * sizeof (t_bbl *));
  ch->nchains = nchains;

  i = 0;
  CFG_FOREACH_BBL(cfg, i_bbl)
  {
    t_bbl *j_bbl, *prev = NULL;


    /* skip return blocks and hell nodes */
    if (BBL_IS_HELL (i_bbl) || (BBL_FUNCTION(i_bbl) && (FunctionGetExitBlock (BBL_FUNCTION(i_bbl)) == i_bbl)))
      continue;

    if (BBL_PREV_IN_CHAIN(i_bbl))
      continue; /* we're looking for the heads of the chains here */

    ch->chains[i++] = i_bbl;
    for (j_bbl = i_bbl; j_bbl; prev = j_bbl, j_bbl = BBL_NEXT_IN_CHAIN(j_bbl))
    {
      BBL_SET_FIRST_IN_CHAIN(j_bbl, i_bbl);
    }
    for (j_bbl = prev; j_bbl; j_bbl = BBL_PREV_IN_CHAIN(j_bbl))
    {
      BBL_SET_LAST_IN_CHAIN(j_bbl, prev);
    }
  }
}

/* }}} */

/* {{{ determine the layout order for a set of layout chains
 * after the final order has been determined, all chains
 * are concatenated to ch->chains[0] */

/* {{{ helper function for qsort */
int
__helper_order_chains (const void *a, const void *b)
{
  t_bbl *blocka = *((t_bbl **) a);
  t_bbl *blockb = *((t_bbl **) b);

  if (AddressIsLt (BBL_OLD_ADDRESS(blocka), BBL_OLD_ADDRESS(blockb)))
    return -1;
  else if (AddressIsGt (BBL_OLD_ADDRESS(blocka), BBL_OLD_ADDRESS(blockb)))
    return 1;

  return 0;
}

/* }}} */

void
DefaultOrderChains (t_chain_holder * ch)
{
  t_bbl *bbl, *prev;

  /* for now, we limit ourselves to the simplest of all orderings:
   * order according to original address of the chain heads */
  diablo_stable_sort (ch->chains, (size_t) ch->nchains, sizeof (t_bbl *), __helper_order_chains);

  prev = NULL;
  for (t_uint32 i = 0; i < ch->nchains; i++)
  {
    for (bbl = ch->chains[i]; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
    {
      BBL_SET_FIRST_IN_CHAIN(bbl, ch->chains[0]);
      BBL_SET_LAST_IN_CHAIN(bbl, BBL_LAST_IN_CHAIN(ch->chains[ch->nchains - 1]));
      if (!BBL_PREV_IN_CHAIN(bbl) && prev)
      {
        BBL_SET_PREV_IN_CHAIN(bbl, prev);
        BBL_SET_NEXT_IN_CHAIN(prev, bbl);
      }
      prev = bbl;
    }
  }
} /* }}} */

/* {{{ Comment out by Bruno, does not compile for all arches (G_T_UINT64 is not supported on 32 bit arches */
void
DefaultGiveAddressesToChain (t_address start_address, t_chain_holder * ch)
{
  t_bbl *bbl = ch->chains[0];
  t_address address = start_address;
  int nr_ins = 0;

  t_uint32 width_min = CFG_DESCRIPTION(BBL_CFG(bbl))->minimal_encoded_instruction_size;
  t_uint32 width_max = CFG_DESCRIPTION(BBL_CFG(bbl))->maximum_encoded_instruction_size;
  t_uint32 width_mod = CFG_DESCRIPTION(BBL_CFG(bbl))->encoded_instruction_mod_size;


  if (width_min != width_max)
    FATAL(("You should not use the default address assigner of the default code layouter on architectures that have no fixed instruction width!!! (or this should be implemented first)\n"));

  if (width_min != width_mod)
    FATAL(("You should not use the default address assigner of the default code layouter on architectures that have no fixed instruction width!!! (or this should be implemented first)\n"));

  while (bbl)
  {
    /* DiabloPrint(stdout,"new BBL: @iB\n",bbl); */
    nr_ins += BBL_NINS(bbl);
    /* printf("old: %lx new %lx\n",G_T_UINT64(BBL_OLD_ADDRESS(bbl)),address); */
    BblSetAddressSuper (bbl, address);
    address = AddressAddUint32 (address, BBL_NINS(bbl) * (width_mod / 8));
    bbl = BBL_NEXT_IN_CHAIN(bbl);
  }

  /* printf("NR_INS: %d\n",nr_ins); */

} /* }}} */

/* {{{ merge all chains in chain holder */
void
MergeAllChains (t_chain_holder * ch)
{
  t_bbl *bbl, *first, *last;
  first = last = NULL;
  for (t_uint32 i = 0; i < ch->nchains; i++)
  {
    if (!ch->chains[i])
      continue;
    if (!first)
    {
      first = ch->chains[i];
      last = BBL_LAST_IN_CHAIN(first);
    }
    else
    {
      BBL_SET_NEXT_IN_CHAIN(last, ch->chains[i]);
      BBL_SET_PREV_IN_CHAIN(ch->chains[i], last);
      last = BBL_LAST_IN_CHAIN(ch->chains[i]);
    }
  }
  for (bbl = first; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    BBL_SET_FIRST_IN_CHAIN(bbl, first);
    BBL_SET_LAST_IN_CHAIN(bbl, last);
  }
  ch->nchains = 1;
  ch->chains[0] = first;
  BBL_SET_PREV_IN_CHAIN(first, NULL);
}

/* }}} */

/* {{{ merge two chains */
void
MergeChains (t_bbl * ca, t_bbl * cb)
{
  t_bbl *bbl;

  ASSERT(ca!=cb, ("Can't merge chain with itself! (@B)", ca));

  BBL_SET_NEXT_IN_CHAIN(BBL_LAST_IN_CHAIN(ca), cb);
  BBL_SET_PREV_IN_CHAIN(cb, BBL_LAST_IN_CHAIN(ca));
  for (bbl = ca; bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    BBL_SET_FIRST_IN_CHAIN(bbl, ca);
    BBL_SET_LAST_IN_CHAIN(bbl, BBL_LAST_IN_CHAIN(cb));
  }
} /* }}} */

t_ins *
InsPrevInChain (t_ins * ins)
{
  t_bbl *bbl;

  if (INS_IPREV(ins))
    return INS_IPREV(ins);
  for (bbl = BBL_PREV_IN_CHAIN(INS_BBL(ins)); bbl; bbl = BBL_PREV_IN_CHAIN(bbl))
  {
    if (BBL_INS_LAST(bbl))
      return BBL_INS_LAST(bbl);
  }
  return NULL;
}

t_ins *
InsNextInChain (t_ins * ins)
{
  t_bbl *bbl;

  if (INS_INEXT(ins))
    return INS_INEXT(ins);
  for (bbl = BBL_NEXT_IN_CHAIN(INS_BBL(ins)); bbl; bbl = BBL_NEXT_IN_CHAIN(bbl))
  {
    if (BBL_INS_FIRST(bbl))
      return BBL_INS_FIRST(bbl);
  }
  return NULL;
}

t_bbl * GetHeadOfChain(t_bbl * chain)
{
  while (BBL_PREV_IN_CHAIN(chain))
    chain = BBL_PREV_IN_CHAIN(chain);

  return chain;
}

t_bbl * GetTailOfChain(t_bbl * chain)
{
  while (BBL_NEXT_IN_CHAIN(chain))
    chain = BBL_NEXT_IN_CHAIN(chain);

  return chain;
}

t_bool AlreadyInChain(t_bbl * head, t_bbl * test)
{
  t_bbl * tmp = head;
  while (tmp)
  {
    if (tmp == test) return TRUE;
    tmp = BBL_NEXT_IN_CHAIN(tmp);
  }

  return FALSE;
}

void DetectLoopsInChains(t_cfg * cfg)
{
  t_bbl * i_bbl = NULL;

  /* loop detection in chains */
  CFG_FOREACH_BBL(cfg, i_bbl)
  {
        /* if this BBL is the head or tail of a chain, no loop is possible */
    if (BBL_PREV_IN_CHAIN(i_bbl) == NULL
        || BBL_NEXT_IN_CHAIN(i_bbl) == NULL)
      continue;

    t_bbl * tmp = BBL_NEXT_IN_CHAIN(i_bbl);
    while (tmp)
    {
      if (tmp == i_bbl) {
        t_bbl * tmp2 = i_bbl;
        t_uint32 idx_in_chain = 0;
        do
        {
          VERBOSE(0, ("Chain entry %d: @B", idx_in_chain, tmp2));
          tmp2 = BBL_NEXT_IN_CHAIN(tmp2);
          idx_in_chain++;
        } while (tmp2 != i_bbl);
        FATAL(("Loop detected in chain @eiB", tmp));
      }

      tmp = BBL_NEXT_IN_CHAIN(tmp);
    }
  }
}

void AppendBblToChain(t_bbl * chain, t_bbl * bbl)
{
        t_bbl * chain_tail = GetTailOfChain(chain);
        t_bbl * chain_head = GetHeadOfChain(bbl);

        BBL_SET_PREV_IN_CHAIN(chain_head, chain_tail);
        BBL_SET_NEXT_IN_CHAIN(chain_tail, chain_head);
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
