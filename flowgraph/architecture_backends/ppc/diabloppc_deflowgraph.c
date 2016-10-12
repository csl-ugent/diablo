/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
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
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

#include <diabloppc.h>

/* PpcCalcRelocs {{{ */
#define DEBUG_CALC_NEWRELOCS 0
#define DEBUG_PRINT_RELOC_TABLE 0
#define DEBUG_RELOCS 0

void
PpcCalcRelocs(t_object * obj)
{
  t_reloc * rel;

  OBJECT_FOREACH_RELOC(obj,rel)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_INS)
    {
      t_ppc_ins * ins = T_PPC_INS(RELOC_FROM(rel));

      /* jump and call instructions which do not have to be relocated
       * but which are handled via their edges have their relocations
       * removed in PpcFlowgraphSection(). The others are jumps/calls
       * to stubs for dynamic symbols.
       */

      if (PPC_INS_TYPE (ins) == IT_DATA)
      {
        /*VERBOSE(0,("relocating @I", ins));*/
        t_uint64 data;
        t_bool next_is_data = FALSE;
        t_address ret;
#define PPC_INS_DATA(i)         ((t_uint32)AddressExtractUint64(PPC_INS_IMMEDIATE(i)))
#define PPC_INS_SET_DATA(i,d)     PPC_INS_SET_IMMEDIATE(i, AddressNewForIns(i, d))
        data = ((t_uint64) PPC_INS_DATA(ins)) << 32;
        if (PPC_INS_INEXT(ins) &&
            PPC_INS_TYPE(PPC_INS_INEXT(ins)) == IT_DATA)
        {
          data |= PPC_INS_DATA(PPC_INS_INEXT(ins));
          next_is_data = TRUE;
          /*VERBOSE(0,("next @I",PPC_INS_INEXT(ins)));*/
        }

        ret = StackExec(RELOC_CODE(rel), rel, NULL, (char *)&data, TRUE, 0, obj);
        ASSERT(AddressIsNull(ret), ("Failed to properly execute @R", rel));

        PPC_INS_SET_DATA(ins, data >> 32);
        /*VERBOSE(0,("result @I", ins));*/
        if (next_is_data)
        {
          PPC_INS_SET_DATA(PPC_INS_INEXT(ins), data & 0xffffffff);
          /*VERBOSE(0,("nextres @I", PPC_INS_INEXT(ins)));*/
        }
        else
        {
          ASSERT((data & 0xffffffff) == 0,
                 ("64-bit reloc @R but only one data ins @I (%llx)", rel, ins, data));
        }
#undef PPC_INS_DATA
#undef PPC_INS_SET_DATA
      }
      else
      {
        t_address ret;

        t_uint32 assembled;
        ins->Assemble(ins, &assembled);
        ret = StackExec(RELOC_CODE(rel), rel, NULL, (char *)&assembled, TRUE, 0, obj);
        ASSERT(AddressIsNull(ret),("@R\n\n for ins @I\n\n fails relocation check",rel,ins));
        ppc_opcode_table[PPC_INS_OPCODE(ins)].Disassemble(ins,assembled,PPC_INS_OPCODE(ins));
      }    
    }
    else
    {
#if DEBUG_CALC_NEWRELOCS
      DEBUG(("Relocating @R", rel));
      DEBUG(("@G", RelocGetData (rel)));
      t_address addr = RelocGetData (rel);
#endif
      t_address ret = StackExec(RELOC_CODE(rel),rel,NULL,SECTION_DATA(T_SECTION(RELOC_FROM(rel))),TRUE,0,obj);
      ASSERT(AddressIsNull (ret), ("Could not properly execute @R", rel));
#if DEBUG_RELOCS
      VERBOSE(0, ("-----------------------------"));
      if (RELOC_LABEL(rel))
      {
        VERBOSE(0, ("Relocating reloc with label %s", RELOC_LABEL(rel)));
      }
      else
      {
        VERBOSE(0, ("Relocating reloc without a label"));
      }
      VERBOSE(0, ("@R\n", rel));
      VERBOSE(0, ("We relocated as: %x", AddressExtractUint32(StackExec(RELOC_CODE(rel),rel,NULL,NULL, FALSE,0,obj))));
      
#endif
#if 0
#if DEBUG_CALC_NEWRELOCS
      DEBUG(("Relocated as: @G", RelocGetData (rel)));
      if (!AddressIsEq (RelocGetData (rel), addr))
      {
        DEBUG(("Pure data relocation difference: from (@T) @G -> to (@T) @G",
               RELOC_FROM (rel), addr,
               RELOC_N_TO_RELOCATABLES (rel) && RELOC_N_TO_RELOCATABLES (rel) == 1
               ? RELOC_TO_RELOCATABLE (rel)[0]
               : NULL,
               RelocGetData (rel)));
      }
#endif
#endif
    }
  }
#if DEBUG_PRINT_RELOC_TABLE 
  RelocTablePrint(OBJECT_RELOC_TABLE(obj));
#endif
}
/* }}} */

/* PpcUpdateControlFlowDisplacement {{{*/
void
PpcUpdateControlFlowDisplacement (t_ppc_ins *ins)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_address offset;

  if (!ins)
  {
    return;
  }

  bbl = PPC_INS_BBL (ins);

  /* does this instruction have a displacement? */
  switch (PPC_INS_OPCODE (ins))
  {
    case PPC_B:
    case PPC_BC:
      break;
    default:
      return;
  }

  /* if it's a branch with relocations (got producer, call
   * to .plt entry), it will be handled by the relocation
   * code
   */
  if (PPC_INS_REFERS_TO(ins))
    return;
  
  /* find the destination in the current representation */
  BBL_FOREACH_SUCC_EDGE (bbl, edge)
    if (CfgEdgeTestCategoryOr (edge, ET_IPJUMP | ET_JUMP | ET_CALL))
    {
      break;
    }
  ASSERT (edge, ("Could not find edge for @I", ins));

  /* calculate the new offset for the jump */
  if (PpcInsAbsoluteAddress(ins))
  {
    offset = BBL_CADDRESS(CFG_EDGE_TAIL(edge));
  }
  else
  {
    offset = AddressSub (BBL_CADDRESS (CFG_EDGE_TAIL (edge)),PPC_INS_CADDRESS(ins));
  }
  PPC_INS_SET_IMMEDIATE(ins, offset);

  /* special case: calls to non-available weak symbols get relocated
   * by the linker to calls to absolute address 0. This is represented
   * in diablo by a jump to the instruction's own basic block. While 
   * technically incorrect, this works because the code in this basic 
   * block is guaranteed never to be executed. However, for various 
   * reasons we want to have the call pointing to 0 again in the final
   * version of the program */

  /*TODO: Check if the address to jump is 0x1000000 (it the default
  value for the undefined weak symbols ) */

  /*Yes, the undefined weak symbols always jump to 0x10000000 address,
    so the value that shoud be computed as immediate should be the dis-
    placement between the instruction and 0x10000000 */
 
  /*But as this code never will be executed, it's not important for
    the execution of the probram */
  
  /* if ((PPC_INS_OPCODE (ins) == PPC_B || PPC_INS_OPCODE(ins) == PPC_BC) &&
      CFG_EDGE_CAT (edge) == ET_JUMP &&
      CFG_EDGE_TAIL (edge) == bbl)
  {
    PPC_INS_SET_IMMEDIATE(ins, 0x0); //<-- correct this immediate
  }*/
}
/*}}}*/

/* UpdateJumpDisplacements {{{ */
/* For each basic block in each code section
 * calls the PpcUpdateControlFlowDisplacement
 * on each last instruction of the basic block.
 */
void 
PpcUpdateJumpDisplacements(t_object * obj)
{
  int i;
  t_bbl * i_bbl;
  for (i=0; i<OBJECT_NCODES(obj); i++)
  {
    CHAIN_FOREACH_BBL (T_BBL(SECTION_TMP_BUF(OBJECT_CODE(obj)[i])), i_bbl)
    {
      PpcUpdateControlFlowDisplacement (T_PPC_INS(BBL_INS_LAST (i_bbl)));
    }
  }
}
/* }}} */

/* PpcCreateChains {{{ */
/*!
 * Creates chains of basic blocs that have to be consecutive
 * in the final code. 
 * -The ET_MUSTCHAIN defines the types of edges that we consider
 * that have to be consecutive : FALLTHROUGHT edges
 * -The ET_MUSTCHAINMAYBE defines the type of edfes that we
 * consider that have to be consecutive with the corresponding
 * edge. It means, the CALLs ans SWIs, because the return block
 * of call have to be after the basic block that calls the 
 * function.
 */
#define ET_MUSTCHAIN  	ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE  	ET_CALL | ET_SWI
/* \TODO: SWITCH TABLES!!! Take them into account! */
void 
PpcCreateChains(t_cfg * cfg, t_chain_holder * ch)
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
    {
      nbbls++;
    }
  }

  nchains = nbbls;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_cfg_edge * i_edge;

    /* skip return blocks and hell nodes */
    if (BBL_IS_HELL(i_bbl) || 
        (BBL_FUNCTION(i_bbl) && 
         (FunctionGetExitBlock(BBL_FUNCTION(i_bbl)) == i_bbl)))
    {
      continue;
    }

    /* chain fallthrough paths and call site/return site combos */
    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (CfgEdgeTestCategoryOr(i_edge,ET_MUSTCHAIN))
      {
        break;
      }
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

#if 1
#ifndef _MSC_VER
#warning This is too ugly (needed for manually written assembler)
#endif
  if (!nchains) nchains = 1;
#endif
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
    {
      continue;
    }

    if (BBL_PREV_IN_CHAIN(i_bbl)) 
    {
      continue; /* we're looking for the heads of the chains here */
    }

    ASSERT (i < ch->nchains, ("More chains second time around"));

    ch->chains[i++] = i_bbl;

    /* Set the first of the chains */
    for (j_bbl = i_bbl; j_bbl; prev = j_bbl, j_bbl = BBL_NEXT_IN_CHAIN(j_bbl))
    {
      BBL_SET_FIRST_IN_CHAIN(j_bbl,  i_bbl);
    }

    /* Set the last of the chains */
    for (j_bbl = prev; j_bbl; j_bbl = BBL_PREV_IN_CHAIN(j_bbl))
    {
      BBL_SET_LAST_IN_CHAIN(j_bbl,  prev);
    }
  }
}
/* }}} */

/* PpcListFinalProgram {{{ */
/* debug code: list the final program */
void
PpcListFinalProgram(t_bbl * bbl)
{
  char * filename = StringDup(diabloflowgraph_options.listfile);
  FILE * f = fopen(filename,"w");
  if (f)
  {
    t_ppc_ins * ins;
    for(;bbl;bbl=BBL_NEXT_IN_CHAIN(bbl))
    {
      for(ins =T_PPC_INS(BBL_INS_FIRST(bbl)); ins; ins=PPC_INS_INEXT(ins))
      {
        FileIo(f,"@I\n",ins); 
      }
    }
    fclose(f);
  }
  else
    VERBOSE(0,("Could not open %s for writing!",filename));

  Free(filename);
}
/* }}} */

static void PpcOrderChains(t_cfg *cfg, t_chain_holder *chains)
{
  /* TODO: figure out a sensible way of reordering the chains.
   * Perhaps as a way of minimizing the number of trampolines
   * that have to be inserted? */
}

static t_address UpdateAllChainAddresses(t_chain_holder *ch, t_address start)
{
  t_address run = start;
  int i;
  for (i = 0; i < ch->nchains; ++i)
    if (ch->chains[i])
      run = AssignAddressesInChain(ch->chains[i], run);
  return run;
}

#define offset_encodable_in(offset, bits)  (AddressExtractInt64SignExtend(offset) < (1 << (bits-1)) && AddressExtractInt64SignExtend(offset) >= -(1 << (bits-1)))
#define ChainEnd(b)     AddressAdd(BBL_CADDRESS(BBL_LAST_IN_CHAIN(b)), BBL_CSIZE(BBL_LAST_IN_CHAIN(b)))

static void InsertEmptyChainAt(t_chain_holder *ch, int index)
{
  ASSERT(index <= ch->nchains, ("index out of bounds"));

  ch->chains = Realloc(ch->chains, sizeof(t_bbl *)*(ch->nchains+1));
  if (index != ch->nchains)
  {
    memmove(ch->chains+index+1, ch->chains+index,
            sizeof(t_bbl *) * (ch->nchains-index));
  }
  ch->chains[index] = NULL;
  ch->nchains++;
}

static void PpcInsertTrampolines(t_cfg *cfg, t_chain_holder *ch)
{
  int i;
  t_address start = SECTION_CADDRESS(OBJECT_CODE(CFG_OBJECT(cfg))[0]);
  t_bbl *bbl;
  t_cfg_edge *tjump;
  t_bool inserted_trampoline;

  UpdateAllChainAddresses(ch, start);

  do {
    inserted_trampoline=FALSE;
    for (i = 0; i < ch->nchains; ++i)
    {
      CHAIN_FOREACH_BBL(ch->chains[i], bbl)
      {
        t_ppc_ins *last = T_PPC_INS(BBL_INS_LAST(bbl));
        t_cfg_edge *edge;
        t_address offset;
        t_bbl *trampo;
        t_ins *branch;

        if (!last) continue;
        if (PPC_INS_OPCODE(last) != PPC_BC) continue;
        
        BBL_FOREACH_SUCC_EDGE(bbl, edge)
          if (CFG_EDGE_CAT(edge) == ET_JUMP || CFG_EDGE_CAT(edge) == ET_IPJUMP || CFG_EDGE_CAT(edge) == ET_CALL)
            break;
        ASSERT(edge, ("conditional jump without jump/call edge"));

        offset =
          AddressSub(BBL_CADDRESS(CFG_EDGE_TAIL(edge)), PPC_INS_CADDRESS(last));
        if (offset_encodable_in(offset, 16)) continue;

        /* We've found an unencodable offset. Insert a trampoline. */
        VERBOSE(1, ("trampoline for @ieB", bbl));
        ASSERT(offset_encodable_in(AddressSub(ChainEnd(bbl),
                                              PPC_INS_CADDRESS(last)),
                                   16), ("implement support for long chains"));
        InsertEmptyChainAt(ch, i+1);
        
        trampo = BblNew(cfg);
        branch = InsNewForBbl(trampo);
        InsAppendToBbl(branch, trampo);
        PpcInsMakeUncondBranch((t_ins*)T_PPC_INS(branch));
        ch->chains[i+1] = trampo;
        BBL_SET_FIRST_IN_CHAIN(trampo, trampo);
        BBL_SET_LAST_IN_CHAIN(trampo, trampo);

        if (CFG_EDGE_CAT(edge) != ET_CALL) {
          tjump = CfgEdgeCreate(cfg, trampo, CFG_EDGE_TAIL(edge),
                                CFG_EDGE_CAT(edge));
        }
        else
        {
          t_cfg_edge *ft_edge;
          t_bbl *ft_block;
          
          BBL_FOREACH_SUCC_EDGE(bbl, ft_edge)
            if (CFG_EDGE_CAT(ft_edge) == ET_FALLTHROUGH)
              break;
          ASSERT(ft_edge, ("No fallthrough block for conditional call"));
          ft_block = CFG_EDGE_TAIL (ft_edge);
          tjump = CfgEdgeCreateCall (cfg, trampo, CFG_EDGE_TAIL(edge), ft_block, NULL);
        }

        if (CFG_EDGE_CORR(edge))
        {
          CFG_EDGE_SET_CORR(tjump, CFG_EDGE_CORR(edge));
          CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), tjump);
          CFG_EDGE_SET_CORR(edge, NULL);
        }
        CfgEdgeCreate(cfg, bbl, trampo, ET_JUMP);
        CfgEdgeKill(edge);
        inserted_trampoline=TRUE;
      }
    }
    UpdateAllChainAddresses(ch, start);
  } while (inserted_trampoline);

}

static void PpcMergeDataChains(t_cfg *cfg, t_chain_holder *ch)
{
  int i,j;
  t_bbl *bbl;
  t_ins *ins;
  t_reloc_ref *rr;
  t_relocatable *from;
  t_bbl *chain2;

  for (i = 0; i < ch->nchains; ++i)
  {
    bbl = ch->chains[i];
    if (!bbl) continue;
    if (!IS_DATABBL(bbl)) continue;

    ins = BBL_INS_FIRST(bbl);
    /* the got2 data blocks are always 1 instruction */
    if (INS_INEXT(ins))
      continue;

    rr = BBL_REFED_BY(bbl);
    if (!rr) FATAL(("expect at least one reference to @ieB", bbl));
    if (RELOC_REF_NEXT(rr)) FATAL(("expect at most one reference to @ieB", bbl));

    from = RELOC_FROM(RELOC_REF_RELOC(rr));
    if (RELOCATABLE_RELOCATABLE_TYPE(from) != RT_INS)
      /* it can also be a reference from eh_frame, rather than from got2 */
      continue;

    chain2 = BBL_FIRST_IN_CHAIN(INS_BBL(T_INS(from)));
    MergeChains(bbl, chain2);

    for (j = 0; i < ch->nchains; ++j)
      if (ch->chains[j] == chain2)
      {
        ch->chains[j] = 0;
        break;
      }
  }

  for (i=0,j=0; i < ch->nchains; ++i)
    if (ch->chains[i])
      ch->chains[j++] = ch->chains[i];
  ch->nchains = j;
}

/* PpcDeflowgraph {{{ */
/*!
 * Layouts a flowgraph, and builds a linear list of instructions that can be
 * executed. 
 *
 *
 * \return void 
 */
void PpcDeflowgraph(t_object *obj)
{
  t_cfg *cfg = OBJECT_CFG(obj);
  t_chain_holder chains;
  t_section *code = OBJECT_CODE(obj)[0];

  /* Create chains of Basic Blocs that have to be together.
     The basic blocks with a fallthrough edge between them
     have to be together. Also, the call/return basic blocks
     have to be together. */
  PpcCreateChains(cfg, &chains);

  /* attach got2 data blocks to their respective chains */
  PpcMergeDataChains(cfg, &chains);

  PpcOrderChains(cfg, &chains);

  /* insert trampolines for conditional branches whose offsets can't
   * be encoded in the 16-bit offset field */
  PpcInsertTrampolines(cfg, &chains);

  /* Merging all chains */
  MergeAllChains(&chains);

  /* Initialize a temporal buffer */
  SECTION_SET_TMP_BUF(code, chains.chains[0]); 

  /* Places section, assign new addresses */
  ObjectPlaceSections(obj, FALSE, FALSE, TRUE);
  AssignAddressesInChain(chains.chains[0], SECTION_CADDRESS(code));

  /* Update jump displacements */
  PpcUpdateJumpDisplacements(obj);

  /* Compute relocations */
  PpcCalcRelocs(obj);

  if(diabloflowgraph_options.listfile)
    PpcListFinalProgram(chains.chains[0]);

  VERBOSE(0,("old object entry was @G",OBJECT_ENTRY(obj)));
  OBJECT_SET_ENTRY(obj,INS_CADDRESS(BBL_INS_FIRST(CFG_ENTRY(cfg)->entry_bbl)));
  VERBOSE(0,("new object entry is @G", OBJECT_ENTRY(obj)));

  Free(chains.chains);
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
