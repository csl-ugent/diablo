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

#define SPE_INS_SET_BBL_LEADER(i)                                           \
  do {                                                                    \
    SPE_INS_SET_ATTRIB ((i), SPE_INS_ATTRIB((i)) | IF_BBL_LEADER);      \
    nleaders++;                                                         \
  } while (0)

#define SPE_INS_IS_BBL_LEADER(i)                                            \
  (SPE_INS_ATTRIB ((i)) & IF_BBL_LEADER)

/* overlay-aware function that finds the target of a control flow transfer */
static t_spe_ins *SpeInsGetTarget(t_spe_ins *ct_ins, t_address target_address)
{
  t_section *sec = SPE_INS_SECTION(ct_ins);
  t_object *obj = SECTION_OBJECT(sec);
  int i, nfound;
  t_spe_ins *found = NULL;

  if (AddressIsGe(target_address, SECTION_CADDRESS(sec)) &&
      AddressIsLt(target_address,
                  AddressAdd(SECTION_CADDRESS(sec), SECTION_CSIZE(sec))))
    return T_SPE_INS(SecGetInsByAddress(sec, target_address));

  nfound = 0;
  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    if (AddressIsGe(target_address, SECTION_CADDRESS(sec)) &&
        AddressIsLt(target_address,
                    AddressAdd(SECTION_CADDRESS(sec), SECTION_CSIZE(sec))))
    {
      ++nfound;
      found = SecGetInsByAddress(sec, target_address);
    }
  }

  if (nfound <= 1)
    return found;

  /* This is the overlay case. We can only unambiguously choose one target 
   * instruction if we get help from the relocation information. */
  if (SPE_INS_REFERS_TO(ct_ins))
  {
    t_reloc *rel = RELOC_REF_RELOC(SPE_INS_REFERS_TO(ct_ins));
    t_relocatable *to = RELOC_TO_RELOCATABLE(rel)[0];

    ASSERT(RELOC_N_TO_RELOCATABLES(rel) == 1, ("relocation too complex"));
    ASSERT(RELOCATABLE_RELOCATABLE_TYPE(rel) == RT_INS,
           ("expected relocation to instruction"));
    return T_SPE_INS(SecGetInsByAddress(T_SECTION(to), target_address));
  }
  FATAL(("ambiguous control transfer: @I", ct_ins));
  return NULL;
}

/* SpeFindBBLLeaders {{{ */

#define DEBUG_LEADERS 0

#if DEBUG_LEADERS
#define DEBUG_BBLL(m) DEBUG(m)
#else
#define DEBUG_BBLL(m)
#endif

/*! Find BBL leaders
 *
 * There are 4 reasons to mark an instruction as a BBL leader:
 *
 * 1. The instruction is the target of a direct or indirect jump.
 *
 * 2. It's the successor of a control-flow altering instruction.
 *
 * 3. There is an address produced to this basic block, this is either:
 *      a. done directly, using the program counter (we assume no real
 *      code-address calculations are present (so not address of function + x) 
 *
 *      b. using a relocation (and not necessary detected when scanning the
 *      instructions) 
 *    For both cases we need to assume there will be an indirect jump to this
 *    address.
 *
 * 4. The start of data blocks in code or instructions following datablocks.
 *
 * \param obj The object for which a flowgraph should be constructed
 * \return t_uint32 The number of BBL leaders that have been detected
 */
static t_uint32
SpeFindBBLLeaders (t_object *obj)
{
  t_uint32 nleaders = 0;
  t_spe_ins *ins;
  t_section *sec;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    /* The first instruction of the section is always a BBL leader */
    SPE_INS_SET_BBL_LEADER (T_SPE_INS (SECTION_DATA (sec)));

    SECTION_FOREACH_SPE_INS (sec, ins)
    {
      /* Skip instructions already processed, but not if they're branches */
      if (SPE_INS_IS_BBL_LEADER (ins) && SPE_INS_TYPE (ins) != IT_BRANCH) continue;

      /* (1 & 2) Branches {{{ */
      if (SPE_INS_TYPE (ins) == IT_BRANCH)
      {
        /* The fallthrough is a leader */
        if (SPE_INS_INEXT (ins) != NULL)
        {
          SPE_INS_SET_BBL_LEADER (SPE_INS_INEXT (ins));
          DEBUG_BBLL(("BBLL: @G: branch fallthrough (@G)", SPE_INS_CADDRESS (SPE_INS_INEXT (ins)), SPE_INS_CADDRESS (ins)));
        }

        /* The target of the branch is also a leader */
        switch (SPE_INS_OPCODE (ins))
        {
          case SPE_BR:
          case SPE_BRA:
          case SPE_BRSL:
          case SPE_BRASL:
          case SPE_BRNZ:
          case SPE_BRZ:
          case SPE_BRHNZ:
          case SPE_BRHZ:
            {
              t_address target_address;
              t_spe_ins *i_target;
              target_address = AddressAnd (SPE_INS_IMMEDIATE (ins), SPE_LSLR);
              i_target = SpeInsGetTarget(ins, target_address);
              ASSERT (i_target, ("Unhandled branch flowgraphing: @I -> @G", ins, target_address));
              SPE_INS_SET_BBL_LEADER (i_target);
              DEBUG_BBLL(("BBLL: @G: branch target (@G)", target_address, SPE_INS_CADDRESS (ins)));
            }
            break;
          case SPE_BI:
          case SPE_IRET:
          case SPE_BISLED:
          case SPE_BISL:
          case SPE_BIZ:
          case SPE_BINZ:
          case SPE_BIHZ:
          case SPE_BIHNZ:
            /* Indirect branches will be resolved by constant
             * propagation */
            break;
          default:
            FATAL(("Unhandled branch opcode in: @I", ins));
            break;
        }
      }
      /* }}} */
      /* (2) Software interrupt-like {{{ */
      else if (SPE_INS_TYPE (ins) == IT_SWI)
      {
        if (SPE_INS_INEXT (ins) != NULL)
        {
          SPE_INS_SET_BBL_LEADER (SPE_INS_INEXT (ins));
          DEBUG_BBLL(("BBLL: @G: swi fallthrough (@G)", SPE_INS_CADDRESS (SPE_INS_INEXT (ins)), SPE_INS_CADDRESS (ins)));
        }
      }
      /* }}} */

      /* (3) Relocations {{{ */
      if (SPE_INS_REFED_BY (ins))
      {
        SPE_INS_SET_BBL_LEADER (ins);
        DEBUG_BBLL(("BBLL: @G: refed by", SPE_INS_CADDRESS (ins)));
      }
      /* }}} */
    }
  }

  return nleaders;
}
/* }}} */

/* SpeAddBasicBLockEdges {{{ */

#define DEBUG_EDGES 0
#define DEBUG_DOING_EDGES 0

#if DEBUG_DOING_EDGES
#define DEBUG_E(m) DEBUG(m)
#else
#define DEBUG_E(m)
#endif

/*! Add edges between the newly created BBLs
 *
 * \param obj The object for which a control flow graph is built
 * \return t_uint32 The number of created edges
 */
static t_uint32
SpeAddBasicBlockEdges(t_object *obj)
{
  int nedges = 0;
  t_spe_ins *i_start, *i_end, *i_ft;
  t_bbl *block, *b_ft;
  t_cfg *cfg = OBJECT_CFG(obj);
  t_section *sec;
  t_reloc_ref *ref;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    SECTION_FOREACH_SPE_INS (sec, i_start)
    {
      if (!SPE_INS_IS_BBL_LEADER (i_start)) continue;

      block = SPE_INS_BBL (i_start);

      /* Look for block end */
      {
        t_spe_ins *iter;
        for (i_end = i_start, iter = SPE_INS_INEXT (i_start);
             iter && !SPE_INS_IS_BBL_LEADER (iter);
             iter = SPE_INS_INEXT (iter))
        {
          i_end = iter;
        }
      }

      i_ft = SPE_INS_INEXT (i_end) ? SPE_INS_INEXT (i_end) : NULL;
      b_ft = i_ft ? SPE_INS_BBL (i_ft) : NULL;

      /* 1. Add edges for relocations {{{ */      
      for (ref = BBL_REFED_BY (block); ref; ref = RELOC_REF_NEXT (ref))
      {
        t_cfg_edge *e_found = NULL;
        t_cfg_edge *e_ins;
        
        if (RELOCATABLE_RELOCATABLE_TYPE (RELOC_FROM (RELOC_REF_RELOC (ref))) == RT_INS)
        {
          t_spe_ins *ins = T_SPE_INS(RELOC_FROM(RELOC_REF_RELOC(ref)));
          /* don't create hell call edges for direct branches & branch hints */
          switch (SPE_INS_OPCODE(ins))
          {
            case SPE_BR:
            case SPE_BRA:
            case SPE_BRSL:
            case SPE_BRASL:
            case SPE_BRNZ:
            case SPE_BRZ:
            case SPE_BRHNZ:
            case SPE_BRHZ:
            case SPE_HBR:
            case SPE_HBRA:
            case SPE_HBRR:
              {
                RELOC_SET_HELL(RELOC_REF_RELOC (ref),FALSE);
                continue;
              }
              break;
            default:
              break;
          }

          if (RELOC_REF_NEXT (SPE_INS_REFERS_TO (ins)))
          {
            VERBOSE(1, ("1. @R", RELOC_REF_RELOC (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref))))));
            VERBOSE(1, ("2. @R", RELOC_REF_RELOC (RELOC_REF_NEXT (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref)))))));
            FATAL(("Multiple relocs from instruction?"));
          }
          if (RELOC_REF_RELOC(SPE_INS_REFERS_TO(ins)) != RELOC_REF_RELOC(ref))
          {
            FATAL(("Relocation wrong"));
          }
        }        

        /* Fins a call edge to this block that hasn't been fulfilled */
        BBL_FOREACH_PRED_EDGE (block, e_ins)
        {
          if (T_BBL (CFG_EDGE_HEAD (e_ins)) == CFG_HELL_NODE (cfg) && CFG_EDGE_CAT (e_ins) == ET_CALL)
          {
            e_found = e_ins;
            break;
          }
        }

        if (e_found)
        {
          /* The relocation has an associated edge */
          RELOC_SET_EDGE (RELOC_REF_RELOC (ref), e_found);
          CFG_EDGE_SET_REFCOUNT (e_found, CFG_EDGE_REFCOUNT (e_found) + 1);
          /* Also increment the refcount of the corresponding return edge */
          ASSERT (CFG_EDGE_CORR (e_found), ("Call edge @E does not have a corresponding edge!", e_found));
          CFG_EDGE_SET_REFCOUNT (CFG_EDGE_CORR(e_found), CFG_EDGE_REFCOUNT (CFG_EDGE_CORR (e_found)) + 1);
        }
        else
        {
          /* The relocation has no associated edge */
          RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), block, NULL, NULL));
          nedges++;
        }
      }
      /* }}} */

      /* 2. Add normal edges {{{ */
      switch (SPE_INS_TYPE (i_end))
      {
        case IT_BRANCH:
          /* If the branch is conditional, add a fall-through edge */
          if (b_ft && SPE_INS_IS_CONDITIONAL (i_end))
          {
            CfgEdgeCreate (cfg, block, b_ft, ET_FALLTHROUGH);
            nedges++;
          }
          switch (SPE_INS_OPCODE (i_end))
          {
            case SPE_BR:
            case SPE_BRA:
            case SPE_BRSL:
            case SPE_BRASL:
            case SPE_BRNZ:
            case SPE_BRZ:
            case SPE_BRHNZ:
            case SPE_BRHZ:
              {
                /* Get target instruction */
                t_address target_address;
                t_spe_ins *i_target;
                target_address = AddressAnd (SPE_INS_IMMEDIATE (i_end), SPE_LSLR);
                i_target = SpeInsGetTarget(i_end, target_address);
                ASSERT (i_target, ("Unhandled branch flowgraphing: @I -> @G", i_end, target_address));


                /* Add a jump edge to the target */
                if (SPE_INS_UPDATES_LINK_REG (i_end) && b_ft)
                {
                  CfgEdgeCreateCall (cfg, block, SPE_INS_BBL(i_target), b_ft, NULL);
                }
                else
                {
                  CfgEdgeCreate (cfg, block, SPE_INS_BBL(i_target), ET_JUMP);
                }
                nedges++;
              }
              break;
            case SPE_BI:
            case SPE_IRET:
            case SPE_BISLED:
            case SPE_BISL:
            case SPE_BIZ:
            case SPE_BINZ:
            case SPE_BIHZ:
            case SPE_BIHNZ:
              /* is it a procedure return instruction? (bi $0)*/
              if (SPE_INS_OPCODE(i_end) == SPE_BI &&
                  SPE_INS_REGA(i_end) == 0)
              {
                /* yes */
                CfgEdgeCreate(cfg,  block,  CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
                nedges++;
              }
              else
              {
                /* Indirect branches will be resolved by constant
                 * propagation */
                if (SPE_INS_UPDATES_LINK_REG (i_end) && b_ft)
                {
                  CfgEdgeCreateCall (cfg, block, CFG_HELL_NODE(cfg), b_ft, NULL);
                }
                else
                {
                  CfgEdgeCreate (cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
                }
                nedges++;
              }
              break;
            default:
              FATAL(("Unknown branch type in: @I", i_end));
          }
          break;
        case IT_SWI:
          /* If the software interrupt is conditional, add a fallthrough edge */
          if (SPE_INS_IS_CONDITIONAL (i_end) && b_ft)
          {
            CfgEdgeCreate (cfg, block, b_ft, ET_FALLTHROUGH);
            nedges++;
          }
          /* since we don't know where a SWI jumps to, we'll add hell as a successor */
          CfgEdgeCreateSwi (cfg, block, b_ft);
          nedges++;
          break;
        default:
          /* As this is a BBL leader, if next block exists, add a
           * fall-through edge */
          if (b_ft)
          {
            CfgEdgeCreate (cfg, block, b_ft, ET_FALLTHROUGH);
            nedges++;
          }
          break;
      }
      /* }}} */

      DEBUG_E(("@eiB", SPE_INS_BBL (i_start)));
    }

#if DEBUG_EDGES
    SECTION_FOREACH_SPE_INS (sec, i_start)
    {
      if (SPE_INS_IS_BBL_LEADER (i_start))
      {
        DEBUG(("@eiB", SPE_INS_BBL (i_start)));
      }
    }
#endif
  }

  return nedges;
}
/* }}} */


/* SpeFoldBranchHints {{{ */
/*!
 * Removes the branch hint instructions from the code and
 * stores them in a field of the branch they referred to.
 * They will be reinsterted during deflowgraphing.
 *
 * \param code The section to be processed
 *
 * \return void
 */
static void
SpeFoldBranchHints(t_object *obj)
{
  t_spe_ins *ins;
  t_section *sec;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    SECTION_FOREACH_SPE_INS(sec, ins)
    {
      t_spe_ins *branch, *hint;

      if (((SPE_INS_OPCODE(ins) != SPE_HBR) ||
           (SPE_INS_FLAGS(ins) & IF_SPE_INLINE_PREFETCHING)) &&
          (SPE_INS_OPCODE(ins) != SPE_HBRR) &&
          (SPE_INS_OPCODE(ins) != SPE_HBRA))
        continue;

      /* We've found a branch hint instruction, now get the branch */
      hint = ins;
      branch=T_SPE_INS(SecGetInsByAddress(sec, SPE_INS_ADDRESS(hint)));
      ASSERT(branch && SpeInsIsRegularControlFlow(branch),
             ("Branch hint does not refer to jump: @I @I", hint, branch));
      /* Sometimes branches contain relocations for their target address,
       * but the corresponding branch hint doesn't...
       *
       *  ASSERT((SPE_INS_REFERS_TO(branch)==NULL) == (SPE_INS_REFERS_TO(hint)==NULL),
       *         ("Branch hint has a relocation and branch doesn't or vice versa: @I and @I",
       *           hint, branch));
       */
      ASSERT(SPE_INS_BHINT(branch) == SPE_UNKNOWN,
             ("Branch has more than one branch hint:\nBranch: @I\nSecond hint: @I",
              branch, hint));

      if (SPE_INS_REFERS_TO(branch) &&
          SPE_INS_REFERS_TO(hint))
      {
        t_reloc *hrel, *brel;
        t_address haddr, baddr;
        hrel = RELOC_REF_RELOC(SPE_INS_REFERS_TO(hint));
        brel = RELOC_REF_RELOC(SPE_INS_REFERS_TO(branch));
        haddr=StackExecConst(RELOC_CODE(hrel), hrel, NULL, 0, obj);
        baddr=StackExecConst(RELOC_CODE(brel), brel, NULL, 0, obj);
        ASSERT(AddressIsEq(haddr,haddr),("Hint and branch targets are different (@G != @G) :\nHint reloc: @R\n\nBranch reloc: @R\n\n",haddr,baddr,hrel,brel));
      }
      /* record the branch hint opcode */
      SPE_INS_SET_BHINT(branch,SPE_INS_OPCODE(hint));
      /* turn the hint into an lnop (both branch hints and lnops
       * are handled in pipeline 1)
       */
      SpeInsMakeLnop(T_INS(hint));
    }
  }
}
/* }}} */

/* SpeFlowgraph {{{ */
/*!  Flowgraph an object
 *
 * \param obj The object to flowgraph
 * \return void
 */
void
SpeFlowgraph(t_object *obj)
{
  t_uint32 count;
  t_cfg *cfg = OBJECT_CFG(obj);

  STATUS (START, ("Branch hint folding"));
  SpeFoldBranchHints (obj);
  STATUS (STOP, ("Branch hint folding"));

  STATUS (START, ("Leader detection"));
  count = SpeFindBBLLeaders (obj);
  VERBOSE(1, ("%d leaders detected", count));
  STATUS (STOP, ("Leader detection"));

  STATUS (START, ("BBL creation"));
  count = CfgCreateBasicBlocks (obj);
  VERBOSE(1, ("%d BBLs created", count));
  STATUS (STOP, ("BBL creation"));

  STATUS (START, ("BBL Edge creation"));
  count = SpeAddBasicBlockEdges (obj);
  VERBOSE(1, ("%d edges created", count));
  STATUS (STOP, ("BBL Edge creation"));

}
/* }}} */

/* SpeMakeAddressProducers {{{ */
/*! Turn position-dependent instructions into pseudo-operations.
 *
 * This turns the position-dependent instructions (i.e., instructions that use
 * the PC) into pseudo-operations that no longer depend on the program conter.
 * 
 * \param sec Section to analyze.
 */
void
SpeMakeAddressProducers (t_cfg *cfg)
{
  /* \TODO */
  DEBUG (("TODO: SpeMakeAddressProducers"));
}
/* }}} */

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
