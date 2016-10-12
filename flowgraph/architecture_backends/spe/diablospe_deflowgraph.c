/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * }}}
 *
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diablospe.h>

/* SpeDeflowgraph {{{ */

/* SpeCreateChains {{{ */
/*! Create chains of BBLS that must be consecutive on the final code.
 *
 * The ET_MUSTCHAIN defines the types of edges that must be consecutive (e.g.,
 * FALLTHROUGH edges).
 *
 * The ET_MUSTCHAINMAYBE defines the types of edges  that must be consecutive
 * with its corresponding edge (e.g., CALLs and SWIs, because of the return
 * block).
 */
#define ET_MUSTCHAIN        ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE   ET_CALL | ET_SWI

static void
SpeCreateChains (t_cfg *cfg, t_chain_holder *ch)
{
  int i;
  t_bbl *bbl;
  t_uint32 nbbls = 0;
  t_uint32 nchains = 0;

  /* Initialize chained BBLs */
  CFG_FOREACH_BBL (cfg, bbl)
  {
    BBL_SET_FIRST_IN_CHAIN (bbl, NULL);
    BBL_SET_LAST_IN_CHAIN (bbl, NULL);
    BBL_SET_NEXT_IN_CHAIN (bbl, NULL);
    BBL_SET_PREV_IN_CHAIN (bbl, NULL);

    /* Count BBLs except return blocks and hell nodes (those won't be
     * chained) */
    if (!BBL_IS_HELL (bbl) &&
        (!BBL_FUNCTION (bbl) || (FunctionGetExitBlock (BBL_FUNCTION (bbl)) != bbl)))
    {
      nbbls++;
    }
  }

  nchains = nbbls;

  /* Chain 'em */
  CFG_FOREACH_BBL (cfg, bbl)
  {
    t_cfg_edge *edge;

    /* Skip return blocks and hell nodes */
    if (BBL_IS_HELL (bbl) || (BBL_FUNCTION (bbl) && (FunctionGetExitBlock (BBL_FUNCTION (bbl)) == bbl)))
    {
      continue;
    }

    BBL_FOREACH_SUCC_EDGE (bbl, edge)
    {
      if (CfgEdgeTestCategoryOr (edge, ET_MUSTCHAIN))
      {
        break;
      }
      if (CfgEdgeTestCategoryOr (edge, ET_MUSTCHAINMAYBE) && CFG_EDGE_CORR (edge))
      {
        edge = CFG_EDGE_CORR (edge);
        break;
      }
    }

    if (edge)
    {
      t_bbl *b_tail = CFG_EDGE_TAIL (edge);

      ASSERT (!BBL_PREV_IN_CHAIN (b_tail), ("Tail already chained to @ieB", BBL_PREV_IN_CHAIN (b_tail)));
      ASSERT (!BBL_NEXT_IN_CHAIN (bbl), ("Head already chained to @ieB", BBL_NEXT_IN_CHAIN (bbl)));
      BBL_SET_PREV_IN_CHAIN (b_tail, bbl);
      BBL_SET_NEXT_IN_CHAIN (bbl, b_tail);
      /* Each time two blocks get chained, we have one less chain */
      nchains--;
    }
  }

  ch->chains = Malloc (nchains * sizeof(*(ch->chains)));
  ch->nchains = nchains;

  i = 0;

  CFG_FOREACH_BBL (cfg, bbl)
  {
    t_bbl *b_j, *b_prev = NULL;

    /* Skip return blocks and hell nodes */
    if (BBL_IS_HELL (bbl) || (BBL_FUNCTION (bbl) && (FunctionGetExitBlock (BBL_FUNCTION (bbl)) == bbl)))
    {
      continue;
    }
    else if (BBL_PREV_IN_CHAIN (bbl))
    {
      /* We're looking for the heads of the chains here */
      continue;
    }

    ASSERT (i < ch->nchains, ("More chains left second time around"));

    ch->chains[i++] = bbl;

    /* Set the first of the chains */
    for (b_j = bbl; b_j; b_prev = b_j, b_j = BBL_NEXT_IN_CHAIN (b_j))
    {
      BBL_SET_FIRST_IN_CHAIN (b_j, bbl);
    }

    /* Set the last of the chains */
    for (b_j = b_prev; b_j; b_j = BBL_PREV_IN_CHAIN (b_j))
    {
      BBL_SET_LAST_IN_CHAIN (b_j, b_prev);
    }
  }
}
/* }}} */

/* SpeDeflowFixpoint {{{ */

/* SpeUpdateJumpDisplacements {{{ */

/* SpeUpdateControlFlowDisplacement {{{ */
/*! Update the immediate for a jump displacement.
 * 
 * \param ins Instruction to update.
 */
static void
SpeUpdateControlFlowDisplacement (t_spe_ins *ins)
{
  t_bbl *bbl;
  t_cfg_edge *edge;

  if (!ins)
  {
    return;
  }

  bbl = SPE_INS_BBL (ins);

  /* Does this instruction have a displacement? */
  if (!SpeInsIsControlFlowWithDisplacement (ins))
  {
    return;
  }

  /* Find the destination */
  BBL_FOREACH_SUCC_EDGE (bbl, edge)
  {
    if (CfgEdgeTestCategoryOr (edge, ET_IPJUMP | ET_JUMP | ET_CALL))
    {
      break;
    }
  }
  ASSERT (edge, ("Could not find destination edge for @I", ins));

  SPE_INS_SET_IMMEDIATE (ins, BBL_CADDRESS (CFG_EDGE_TAIL (edge)));
}
/* }}} */

/*! Update the immediate for jump displacements.
 * 
 * \param obj Object to update
 */
static void
SpeUpdateJumpDisplacements (t_object *obj)
{
  int i;
  t_bbl *bbl;

  for (i=0; i < OBJECT_NCODES (obj); i++)
  {
    CHAIN_FOREACH_BBL (T_BBL (SECTION_TMP_BUF (OBJECT_CODE (obj)[i])), bbl)
    {
      SpeUpdateControlFlowDisplacement (T_SPE_INS (BBL_INS_LAST (bbl)));
    }
  }
}
/* }}} */

/* SpeCalcRelocs {{{ */
/*! Recalculate all the relocations of a given object.
 * 
 * \param obj Object to recalculate.
 */
static void
SpeCalcRelocs (t_object *obj)
{
  t_reloc *rel;

  OBJECT_FOREACH_RELOC (obj, rel)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE (RELOC_FROM (rel)) == RT_INS)
    {
      t_spe_ins *ins = T_SPE_INS (RELOC_FROM (rel));

      /* Skip relocs from branch instructions (already handled by
       * SpeUpdateJumpDisplacements) */
      if (SPE_INS_TYPE (ins) == IT_BRANCH)
      {
        continue;
      }
      else
      {
        t_address relres;
        t_uint32 assembled = SpeAssembleOne(ins);
        relres=StackExec(RELOC_CODE(rel), rel, NULL, (char*)(&assembled), TRUE, 0, obj);
        ASSERT(AddressIsNull(relres),("Failed to properly execute @R\nResult = @G",rel,relres));
        SpeDisassembleOne(assembled, ins);
      }
    }
    else
    {
      /* Data relocations can be written directly */
      StackExec(RELOC_CODE(rel), rel, NULL, SECTION_DATA(T_SECTION(RELOC_FROM(rel))),
                TRUE, 0, obj);
    }
  }
}
/* }}} */

#define UPDATE_ADDRESSES(i,obj)                                             \
  do {                                                                    \
    for (i = 0; i < OBJECT_NCODES (obj); i++)                           \
    {                                                                   \
      AssignAddressesInChain (SECTION_TMP_BUF (OBJECT_CODE (obj)[i]), SECTION_CADDRESS (OBJECT_CODE (obj)[i])); \
    }                                                                   \
  } while (0)

/*! Place sections and update relocations.
 * 
 * \param obj Object to deflow
 */
static void
SpeDeflowFixpoint (t_object *obj)
{
  int i;

  /* Assign new addresses in the chains */
  UPDATE_ADDRESSES (i, obj);

  ObjectPlaceSections (obj, FALSE, FALSE, TRUE);
  UPDATE_ADDRESSES (i, obj);

  SpeUpdateJumpDisplacements (obj);
  SpeCalcRelocs (obj);
}

#undef UPDATE_ADDRESSES
/* }}} */

/* SpeListFinalProgram {{{ */
/*! Print the list of instruction f the program after going through Diablo.
 * 
 * \param bbl Root BBL to print.
 * \param filename Filename where output must go.
 */
void
SpeListFinalProgram (t_bbl *bbl, char *filename)
{
  FILE *f = fopen (filename, "w");

  if (f)
  {
    t_spe_ins *ins;

    for ( ; bbl; bbl = BBL_NEXT_IN_CHAIN (bbl))
    {
      for (ins = T_SPE_INS (BBL_INS_FIRST (bbl)); ins; ins = SPE_INS_INEXT (ins))
      {
        FileIo (f, "@I\n", ins);
      }
    }
    fclose (f);
  }
  else
  {
    VERBOSE (0, ("Could not open %s for writing!", filename));
  }
}
/* }}} */

/* SpeUnfoldBranchHints {{{ */
/* ! Restore the branch hints which were folded into the branches during flowgraphing
 */
static void SpeUnfoldBranchHints(t_cfg *cfg)
{
  t_bbl *bbl;
  
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_spe_ins *ins, *branch, *hint;    
    
    if (BBL_NINS(bbl)==0)
      continue;

    ins = T_SPE_INS(BBL_INS_LAST(bbl));
    if (SPE_INS_BHINT(ins) == SPE_UNKNOWN)
      continue;

    /* We've found a branch with hint */
    branch = ins;
    ASSERT(branch && SpeInsIsRegularControlFlow(branch), ("Instruction with stored branch hint is not a jump: @I", branch));

    /* The branch hint should be followed first by at least 11 cycles worth of
     * instructions (otherwise a stall will occurr), and after that there must
     * be 4 pairs of instructions before the branch (otherwise the hint will be
     * ignored).
     * The heuristic of 8 instructions is far from optimal, since it depends on
     * in which pipeline the instructions are actually handled (so less
     * instructions could still be enough to fulfill the 4-pairs-requirement)
     */

    if (BBL_NINS(bbl)>=8)
    {
      t_ins *ins, *foundlnop;
      int count;
      t_reg targetreg;

      targetreg = SPE_REG_NONE;
      if (!SpeInsIsControlFlowWithDisplacement(branch))
      {
        ASSERT(SPE_INS_REGA(branch)!=SPE_REG_NONE,("Indirect branch with REGA == SPE_REG_NONE: @I",branch));
        targetreg = SPE_INS_REGA(branch);
      }
      foundlnop = NULL;
      /* first check whether there's an lnop which we can replace */
      BBL_FOREACH_INS_R(bbl,ins)
      {
        count++;
        if ((targetreg!=SPE_REG_NONE) &&
            RegsetIn(SPE_INS_REGS_DEF(T_SPE_INS(ins)),targetreg))
        {
          count--;
          ins=INS_INEXT(ins);
          break;
        }

        if ((count > 8) &&
            (SPE_INS_OPCODE(T_SPE_INS(ins))==SPE_LNOP))
        {
          /* we have found an lnop we can replace */
          foundlnop=ins;
          /* keep searching to find the earliest possible insertion
           * point (both for the 11 cycle stuff, and also because on
           * later revisions of the cell having a greater distance
           * between the hint and the branch may be benficial
           */
        }
        /* hint-to-branch offset is 11 bits (signed) ->
         * maximally 255 instructions before (leave some room
         * for small reordering/instruction adding
         */
        if (count == 220)
          break;
      }

      if (foundlnop)
      {
        SpeInsMakeHintForBranch(T_SPE_INS(foundlnop),branch);
        VERBOSE(1,("Inserted branch hint for branch (replaced lnop) @I",branch));
        VERBOSE(1,("-- The hint: @I",foundlnop));
      }
      else if ((targetreg==SPE_REG_NONE) ||
               (count > 8))
      {
        hint = T_SPE_INS(InsNewForBbl(bbl));
        SpeInsMakeHintForBranch(hint,branch);
        if (ins)
          InsInsertBefore(T_INS(hint),ins);
        else
          InsPrependToBbl(T_INS(hint),bbl);
        VERBOSE(1,("Inserted branch hint for branch @I",branch));
        VERBOSE(1,("-- The hint: @I",hint));
      }
    }
    else
    {
      VERBOSE(0,("Unable to find a suitable location to place the branch hint for @I",branch));
    }
  }
}
/* }}} */

static void SpeMakeOverlaysIndependent(t_cfg *cfg)
{
  t_bbl *bbl;
  t_cfg_edge *edge;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
    {
      t_bbl *tail = NULL;
      switch (CFG_EDGE_CAT(edge))
      {
        case ET_FALLTHROUGH:
        case ET_IPFALLTHRU:
          tail = CFG_EDGE_TAIL(edge);
          break;
        case ET_CALL:
        case ET_SWI:
          if (CFG_EDGE_CORR(edge))
            tail = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
          break;
        default:
          ; /* no tail */
      }

      if (!tail) continue;

      if (BBL_OVERLAY(bbl) != BBL_OVERLAY(tail))
        FATAL(("TODO: implement chain splitting"));
    }
  }
}

#define ovlchains(x) ((t_chain_holder *)(x->chains))
static void SpeSplitChainsPerOverlay(t_cfg *cfg, t_chain_holder *ch)
{
  t_object *obj = CFG_OBJECT(cfg);
  t_overlay *ovl;
  t_overlay_sec *ovlsec;
  int i, j;

  for (ovl = OBJECT_OVERLAYS(obj); ovl; ovl = ovl->next)
  {
    for (ovlsec = ovl->sec; ovlsec; ovlsec = ovlsec->next)
    {
      ovlsec->chains = Malloc(sizeof(t_chain_holder));
      ovlchains(ovlsec)->nchains = 0;
      ovlchains(ovlsec)->chains = Malloc(sizeof(t_bbl *) * ch->nchains);
    }
  }

  for (i = 0; i < ch->nchains; ++i)
  {
    t_bbl *chain = ch->chains[i];
    
    if ((ovlsec = BBL_OVERLAY(chain)))
    {
      ovlchains(ovlsec)->chains[ovlchains(ovlsec)->nchains++] = chain;
      ch->chains[i] = NULL;
    }
  }

  for (i = 0, j = 0; i < ch->nchains; ++i)
    if (ch->chains[i])
      ch->chains[j++] = ch->chains[i];
  ch->nchains = j;

  MergeAllChains(ch);

  for (ovl = OBJECT_OVERLAYS(obj); ovl; ovl = ovl->next)
  {
    for (ovlsec = ovl->sec; ovlsec; ovlsec = ovlsec->next)
    {
      if (ovlchains(ovlsec)->nchains)
      {
        MergeAllChains(ovlsec->chains);
        ovlsec->section =
          SectionCreateDeflowgraphingFromChain(obj,
                                               ovlchains(ovlsec)->chains[0],
                                               ovlsec->name);
        Free(ovlchains(ovlsec)->chains);
        Free(ovlchains(ovlsec));
      }
    }
  }
}
#undef ovlchains

static void SpeRecreateOvtab(t_object *obj)
{
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_section *ovtab = SectionGetFromObjectByName(linker, ".ovtab");
  t_section *ovtab_buf;
  t_uint32 tablesize = 0, bufsize = 0;
  t_overlay *ovl; t_overlay_sec *ovlsec;
  int index = 0, bufindex = 0;

  if (!ovtab) FATAL((".ovtab is deleted"));
  ovtab_buf = SectionGetFromObjectByName(linker, ".ovtab_buf");
  if (!ovtab_buf) FATAL((".ovtab_buf is deleted"));

  /* compute the size of the .ovtab section */  
  for (ovl = OBJECT_OVERLAYS(obj); ovl; ovl = ovl->next)
  {
    bufsize += 4;
    for (ovlsec = ovl->sec; ovlsec; ovlsec = ovlsec->next)
    {
      tablesize += 16;
    }
  }
  
  if (!AddressIsEq(AddressNew32(tablesize), SECTION_CSIZE(ovtab)))
  {
    FATAL(("implement resizing the overlay table"));
  }
  if (!AddressIsEq(AddressNew32(bufsize), SECTION_CSIZE(ovtab_buf)))
  {
    FATAL(("implement resizing the buffer"));
  }

  /* the buffer is filled with zeroes */
  memset(SECTION_DATA(ovtab_buf), 0, bufsize);

  /* for the overlay table, we can fill in everything but the file offset field
   */  
  for (ovl = OBJECT_OVERLAYS(obj); ovl; ovl = ovl->next)
  {
    ++bufindex;
    for (ovlsec = ovl->sec; ovlsec; ovlsec = ovlsec->next)
    {
      t_uint32 *entry = (t_uint32 *) (((char *)SECTION_DATA(ovtab))+(16*index));
      entry[0] = AddressExtractUint32(SECTION_CADDRESS(ovlsec->section));
      entry[1] = AddressExtractUint32(SECTION_CSIZE(ovlsec->section));
      entry[2] = 0;  /* file offset is not yet known */
      entry[3] = bufindex;
      ++index;
    }
  }

}

/*! Layout a flowgraph and build a linear list of instructions to be executed
 *
 * \return void
 */
void SpeDeflowgraph(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);
  t_chain_holder chains;
  t_section *code = OBJECT_CODE(obj)[0];

  SpeMakeOverlaysIndependent(cfg);

  SpeCreateChains (cfg, &chains);

  SpeSplitChainsPerOverlay(cfg, &chains);

  STATUS (START, ("Branch hint unfolding"));
  SpeUnfoldBranchHints (cfg);
  STATUS (STOP, ("Branch hint unfolding"));
  
  /* Initialize a temporal buffer */
  SECTION_SET_TMP_BUF (code, chains.chains[0]);

  SpeDeflowFixpoint (obj);

  if (diabloflowgraph_options.listfile)
  {
    SpeListFinalProgram (chains.chains[0], diabloflowgraph_options.listfile);
  }

  VERBOSE (0, ("Old object entry was @G", OBJECT_ENTRY (obj)));
  OBJECT_SET_ENTRY (obj, INS_CADDRESS (BBL_INS_FIRST (CFG_ENTRY(cfg)->entry_bbl)));
  VERBOSE (0, ("New object entry is @G", OBJECT_ENTRY (obj)));

  if (OBJECT_OVERLAYS(obj))
    SpeRecreateOvtab(obj);

  Free (chains.chains);
}
/* }}} */

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
