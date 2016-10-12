/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloflowgraph.h>
#include <stdlib.h>

/* This counter is increased every time a new BBL is created and serves to
 * give every BBL a unique ID, allowing t_bbl's to be ordered in a way that is
 * not address-dependent.
 */
t_uint32 bbl_global_id = 0;

void
BblKill (t_bbl * bbl)
{
  t_function *fun = BBL_FUNCTION(bbl);
  dominator_info_correct = FALSE;

  while (BBL_REFED_BY_SYM(bbl))
  {
    SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(BBL_REFED_BY_SYM(bbl)->sym), BBL_REFED_BY_SYM(bbl)->sym);
  }

  if (fun)
  {
    if (BBL_PREV_IN_FUN(bbl))
    {
      BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(bbl), BBL_NEXT_IN_FUN(bbl));
    }
    else
    {
      /* this is in fact dangerous. if the first bbl of a function is killed
       * and we're not killing the entire function, we should look for the new
       * function entry and make sure that block becomes the new FUNCTION_BBL_FIRST
       */
      FUNCTION_SET_BBL_FIRST(fun, BBL_NEXT_IN_FUN(bbl));
    }

    if (BBL_NEXT_IN_FUN(bbl))
    {
      BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(bbl), BBL_PREV_IN_FUN(bbl));
    }
    else
    {
      FUNCTION_SET_BBL_LAST(fun, BBL_PREV_IN_FUN(bbl));
    }
  }

  if (BBL_DOMINATED_BY(bbl))
    DominatorFree (BBL_DOMINATED_BY(bbl));
  BBL_SET_DOMINATED_BY(bbl, NULL);
/* Post dominance is not implemented correctly yet */
/*  if (BBL_P_DOMINATED_BY(bbl))*/
/*    DominatorFree (BBL_P_DOMINATED_BY(bbl));*/
/*  BBL_SET_P_DOMINATED_BY(bbl, NULL);*/
  LoopBblCleanup (bbl);

  DiabloBrokerCall("BblKill", bbl);

  /* remove bbl from cfg TODO should we remove it from entries as well? */
  BblFree (bbl);
}

/*! Determine the defined registers for a bbl, determine whether they are
 * always defined or not and fill in BBL_REGS_DEF_CERTAIN and
 * BBL_REGS_DEF_PERHAPS */
void
BblSetDefinedRegs (t_bbl * bbl)
{
  t_ins *i_ins;
  t_regset certain, perhaps, defined;

  certain = RegsetNew ();
  perhaps = RegsetNew ();

  BBL_FOREACH_INS(bbl, i_ins)
  {
    if (INS_IS_CONDITIONAL(i_ins))
    {
      RegsetSetUnion (perhaps, INS_REGS_DEF(i_ins));
    }
    else
    {
      RegsetSetUnion (certain, INS_REGS_DEF(i_ins));
    }
  }

  /* all registers that are in certain should not be in perhaps */
  RegsetSetDiff (perhaps, certain);

  BBL_SET_REGS_DEF_CERTAIN(bbl, certain);
  BBL_SET_REGS_DEF_PERHAPS(bbl, perhaps);

  RegsetSetDup (defined, certain);
  RegsetSetUnion (defined, perhaps);
  BBL_SET_REGS_DEF(bbl, defined);
}

/*! Compute the registers that are defined in a basic block. */
t_regset
BblRegsDef (t_bbl * bbl)
{
  t_ins *i_ins;
  t_regset def;
  t_function *fun = BBL_FUNCTION(bbl);
  t_cfg *cfg = BBL_CFG(bbl);

  if (!fun || 
      BBL_IS_HELL(bbl) ||
      bbl == CFG_UNIQUE_ENTRY_NODE(cfg) || bbl == CFG_UNIQUE_EXIT_NODE(cfg))
  {
    if (fun && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)))
    {
      def = RegsetNewInvers (CFG_DESCRIPTION(cfg)->callee_saved, CFG_DESCRIPTION(cfg)->all_registers);
      RegsetSetDiff (def, CFG_DESCRIPTION(cfg)->cond_registers);
      return def;
    }
    else
      return CFG_DESCRIPTION(cfg)->all_registers;
  }

  def = RegsetNew ();

  BBL_FOREACH_INS(bbl, i_ins)
  {
    if (!INS_IS_CONDITIONAL(i_ins))
    {
      RegsetSetUnion (def, INS_REGS_DEF(i_ins));
    }
  }
  return def;
}

t_regset
BblRegsMaybeDef (t_bbl * bbl)
{
  t_ins *i_ins;
  t_regset def;
  t_function *fun = BBL_FUNCTION(bbl);
  t_cfg *cfg = BBL_CFG(bbl);

  if (!fun || 
      BBL_IS_HELL(bbl) ||
      bbl == CFG_UNIQUE_ENTRY_NODE(cfg) || bbl == CFG_UNIQUE_EXIT_NODE(cfg))
  {
    if (fun && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)))
    {
      def = RegsetNewInvers (CFG_DESCRIPTION(cfg)->callee_saved, CFG_DESCRIPTION(cfg)->all_registers);
      RegsetSetDiff (def, CFG_DESCRIPTION(cfg)->cond_registers);
      return def;
    }
    else
      return CFG_DESCRIPTION(cfg)->all_registers;
  }

  def = RegsetNew ();

  BBL_FOREACH_INS(bbl, i_ins)
  {
    RegsetSetUnion (def, INS_REGS_DEF(i_ins));
  }
  return def;
}

/*! Compute the registers that are used in a basic block*/
t_regset
BblRegsUse (t_bbl * bbl)
{
  t_ins *i_ins;

  t_regset use;

  t_function *fun = BBL_FUNCTION(bbl);
  t_cfg *cfg = BBL_CFG(bbl);

  if (!fun || 
      BBL_IS_HELL(bbl) ||
      bbl == CFG_UNIQUE_ENTRY_NODE(cfg) || bbl == CFG_UNIQUE_EXIT_NODE(cfg))
  {
    if (fun && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)))
    {
      use = RegsetNewInvers (CFG_DESCRIPTION(cfg)->callee_saved, CFG_DESCRIPTION(cfg)->all_registers);
      RegsetSetDiff (use, CFG_DESCRIPTION(cfg)->cond_registers);
      if (BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(fun)) == BBL_CH_DYNCALL)
        RegsetSetUnion (use, CFG_DESCRIPTION(cfg)->dyncall_may_use);
      return use;
    }
    else
      return CFG_DESCRIPTION(cfg)->all_registers;
  }

  use = RegsetNew ();

  BBL_FOREACH_INS_R(bbl, i_ins)
  {
    if (!INS_IS_CONDITIONAL(i_ins))
    {
      RegsetSetDiff (use, INS_REGS_DEF(i_ins));
    }
    RegsetSetUnion (use, INS_REGS_USE(i_ins));
  }
  return use;
}

/* Registers that cannot be live under any circumstances */
t_regset
BblRegsNeverLive(t_bbl * bbl)
{
  t_architecture_description * description;
  t_function * fun = BBL_FUNCTION(bbl);

  if (!fun)
    return NullRegs;
  description = FUNCTION_DESCRIPTOR(fun);

  if (BblIsExitBlock(bbl))
  {
    return RegsetDiff(description->return_regs, FUNCTION_RET_REGS(fun));
  }

  if (bbl == FUNCTION_BBL_FIRST(fun))
  {
    return RegsetDiff(description->argument_regs, FUNCTION_ARG_REGS(fun));
  }
  
  return NullRegs;
}
/* registers live before a bbl */
t_regset
BblRegsLiveBefore (const t_bbl * bbl)
{
  return BBL_INS_FIRST(bbl) ? InsRegsLiveBefore (BBL_INS_FIRST(bbl)) : RegsetDiff(BBL_REGS_LIVE_OUT(bbl),BBL_REGS_NEVER_LIVE(bbl));
}

/* registers live after a bbl */
t_regset
BblRegsLiveAfter (const t_bbl * bbl)
{
  return BBL_REGS_LIVE_OUT(bbl);
}

/*! Set address for bbl and all instructions in the block.
 * also set new size (not nins) */
void
BblSetAddressSuper (t_bbl * bbl, t_address address)
{
  t_address run = address;

  t_ins *ins;

  BBL_SET_CADDRESS(bbl, address);

  BBL_FOREACH_INS(bbl, ins)
  {
    INS_SET_CADDRESS(ins, run);
    run = AddressAdd (run, INS_CSIZE(ins));
  }
  BBL_SET_CSIZE(bbl, AddressSub (run, address));
}

/* Insert a bbl in a function */
void
realBblInsertInFunction (const char *file, int lnno, t_bbl * bbl, t_function * fun)
{
  if (BBL_PREV_IN_FUN(bbl))
    FATAL(("Reinserting already inserted bbl (FUNC=%d addr=@G prev=set)! at %s %d", BBL_FUNCTION(bbl), BBL_CADDRESS(bbl), file, lnno));
  if (BBL_NEXT_IN_FUN(bbl))
    FATAL(("Reinserting already inserted bbl (FUNC=%d addr=@G next=set)! at %s %d", BBL_FUNCTION(bbl), BBL_CADDRESS(bbl), file, lnno));

  BBL_SET_FUNCTION(bbl, fun);
  BBL_SET_CFG(bbl, FUNCTION_CFG(fun));

  if (bbl == FUNCTION_BBL_FIRST(fun))
  {
    BBL_SET_PREV_IN_FUN(bbl, NULL);
    BBL_SET_NEXT_IN_FUN(bbl, NULL);
  }
  else
  {
    if (!FUNCTION_BBL_FIRST(fun))
      FATAL(("You cannot insert bbl's in a function without an entry point (fun name=%s)!\nIf you are trying to insert the entry point of the function, first set FUNCTION_BBL_FIRST of the function to the basic block you wish to make the entry point, then call BblInsertInFunction for the basic block.\nYou also need to set FUNCTION_BBL_LAST. If the function you are trying to create is a function that can return, set FUNCTION_BBL_LAST to a new (empty) basic block for which BBL_ATTRIB contains BBL_IS_EXITBLOCK and call BblInsertInFunction for the basic block. If your function will not return you can simply set FUNCTION_BBL_LAST to the entry point.", FUNCTION_NAME(fun)));
    else if (bbl == FUNCTION_BBL_LAST(fun))
    {
      BBL_SET_NEXT_IN_FUN(FUNCTION_BBL_FIRST(fun), bbl);
      BBL_SET_PREV_IN_FUN(bbl, FUNCTION_BBL_FIRST(fun));
      BBL_SET_NEXT_IN_FUN(bbl, NULL);
    }
    else if (!FUNCTION_BBL_LAST(fun))
      FATAL(("Exit blocks should be create before the insertion of a non-entry block!"));
    else if (FUNCTION_BBL_FIRST(fun) == FUNCTION_BBL_LAST(fun))
    {
      FUNCTION_SET_BBL_LAST(fun, bbl);
      BBL_SET_PREV_IN_FUN(bbl, FUNCTION_BBL_FIRST(fun));
      BBL_SET_NEXT_IN_FUN(FUNCTION_BBL_FIRST(fun), bbl);
    }
    else
    {
      /* insert a normal basic block */
      if (BBL_PREV_IN_FUN(FUNCTION_BBL_LAST(fun)))
        BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(FUNCTION_BBL_LAST(fun)), bbl);
      BBL_SET_PREV_IN_FUN(bbl, BBL_PREV_IN_FUN(FUNCTION_BBL_LAST(fun)));
      BBL_SET_NEXT_IN_FUN(bbl, FUNCTION_BBL_LAST(fun));
      BBL_SET_PREV_IN_FUN(FUNCTION_BBL_LAST(fun), bbl);
    }
  }
}

/* Get a successor edge */
/*! Get the first successor edge of type contained in mask 'types', if
 * there is none, return NULL */
t_cfg_edge *
BblGetSuccEdgeOfType (t_bbl * bbl, t_uint32 types)
{
  t_cfg_edge *ret = NULL;

  BBL_FOREACH_SUCC_EDGE(bbl, ret)
  {
    if (CFG_EDGE_CAT(ret) & types)
      break;
  }
  return ret;
}

t_cfg_edge *
BblGetPredEdgeOfType (t_bbl * bbl, t_uint32 types)
{
  t_cfg_edge *ret = NULL;

  BBL_FOREACH_PRED_EDGE(bbl, ret)
  {
    if (CFG_EDGE_CAT(ret) & types)
      break;
  }
  return ret;
}

/* Move an instruction in a bbl before another instruction. DANGER! UNTESTED! */
void
BblMoveInstructionBefore (t_ins * move_ins, t_ins * dest_ins)
{
  t_ins *prev_ins = INS_IPREV(move_ins);
  t_ins *next_ins = INS_INEXT(move_ins);

  if (INS_BBL(move_ins) != INS_BBL(dest_ins))
    FATAL(("Only use this function for instructions in the same bbl!"));

  /* decouple move_ins from dll */
  if (prev_ins)
    INS_SET_INEXT(prev_ins, next_ins);
  else
    BBL_SET_INS_FIRST(INS_BBL(move_ins), next_ins);

  if (next_ins)
    INS_SET_IPREV(next_ins, prev_ins);
  else
    BBL_SET_INS_LAST(INS_BBL(move_ins), prev_ins);

  /* Insert it before dest_ins */
  INS_SET_IPREV(move_ins, INS_IPREV(dest_ins));
  INS_SET_INEXT(move_ins, dest_ins);
  if (INS_IPREV(move_ins))
    INS_SET_INEXT(INS_IPREV(move_ins), move_ins);
  else
    BBL_SET_INS_FIRST(INS_BBL(move_ins), move_ins);
  INS_SET_IPREV(dest_ins, move_ins);
}

/* Move an instruction in a bbl after another instruction DANGER! UNTESTED! */
void
BblMoveInstructionAfter (t_ins * move_ins, t_ins * dest_ins)
{
  t_ins *prev_ins = INS_IPREV(move_ins);
  t_ins *next_ins = INS_INEXT(move_ins);

  if (INS_BBL(move_ins) != INS_BBL(dest_ins))
    FATAL(("Only use this function for instructions in the same bbl!"));

  /* decouple move_ins from dll */
  if (prev_ins)
    INS_SET_INEXT(prev_ins, next_ins);
  else
    BBL_SET_INS_FIRST(INS_BBL(move_ins), next_ins);

  if (next_ins)
    INS_SET_IPREV(next_ins, prev_ins);
  else
    BBL_SET_INS_LAST(INS_BBL(move_ins), prev_ins);

  /* Insert it after dest_ins */
  INS_SET_IPREV(move_ins, dest_ins);
  INS_SET_INEXT(move_ins, INS_INEXT(dest_ins));
  if (INS_INEXT(move_ins))
    INS_SET_IPREV(INS_INEXT(move_ins), move_ins);
  else
    BBL_SET_INS_LAST(INS_BBL(move_ins), move_ins);
  INS_SET_INEXT(dest_ins, move_ins);
}

/* Split a bbl before or after an instruction */
t_bbl *
RealBblSplitBlock (t_bbl * orig_bbl, t_ins * where, t_bool before, t_bool test_branches)
{
  t_cfg *cfg = BBL_CFG(orig_bbl);
  t_ins *i_ins;
  t_cfg_edge *i_edge;
  t_bbl *split_off;
  t_cfg_exits *ext;
  t_regset liveout2;

  if (BBL_INS_FIRST(orig_bbl))
  {
    if (before)
    {
      liveout2 = InsRegsLiveBefore (where);
    }
    else
    {
      liveout2 = InsRegsLiveAfter (where);
    }
  }
  else
    liveout2 = BBL_REGS_LIVE_OUT (orig_bbl);

  split_off = BblNew (BBL_CFG(orig_bbl));
  BBL_SET_REGS_LIVE_OUT(split_off, BBL_REGS_LIVE_OUT(orig_bbl));
  BBL_SET_REGS_LIVE_OUT(orig_bbl, liveout2);

  if (BBL_INS_FIRST(orig_bbl))
  {
    if ((!before) && (where == BBL_INS_LAST(orig_bbl)))
    {
      /* splitting after last ins of block: only allowed if
       * there is only a fallthrough edge out */

      if (test_branches && BBL_SUCC_FIRST(orig_bbl)  &&( !(CFG_EDGE_CAT(BBL_SUCC_FIRST(orig_bbl)) & (ET_FALLTHROUGH | ET_IPFALLTHRU))))
      {
        FATAL(("Need other edge out @ieB", orig_bbl));
      }
      BBL_SET_INS_FIRST(split_off, NULL);
      BBL_SET_INS_LAST(split_off, NULL);
    }
    else
    {
      BBL_SET_INS_LAST(split_off, BBL_INS_LAST(orig_bbl));

      if (before)
      {
        BBL_SET_INS_FIRST(split_off, where);
      }
      else
      {
        ASSERT(INS_INEXT(where), ("Bbl Corrupt!"));
        BBL_SET_INS_FIRST(split_off, INS_INEXT(where));
      }

      BBL_SET_OLD_ADDRESS(split_off, INS_OLD_ADDRESS(BBL_INS_FIRST(split_off)));

      if (INS_IPREV(BBL_INS_FIRST(split_off)))
      {
        BBL_SET_INS_LAST(orig_bbl, INS_IPREV(BBL_INS_FIRST(split_off)));
        INS_SET_INEXT(INS_IPREV(BBL_INS_FIRST(split_off)), NULL);
        INS_SET_IPREV(BBL_INS_FIRST(split_off), NULL);
      }
      else
      {
        BBL_SET_INS_LAST(orig_bbl, NULL);
        BBL_SET_INS_FIRST(orig_bbl, NULL);
      }
    }

    BBL_SET_CSIZE(split_off, AddressNullForBbl (orig_bbl));

    BBL_FOREACH_INS(split_off, i_ins)
    {
      if (BBL_NINS(split_off) == 0)
      {
        BBL_SET_CADDRESS(split_off, INS_CADDRESS(i_ins));
      }
      INS_SET_BBL(i_ins, split_off);
      BBL_SET_NINS(split_off, BBL_NINS(split_off) + 1);
      BBL_SET_NINS(orig_bbl, BBL_NINS(orig_bbl) - 1);
      BBL_SET_CSIZE(orig_bbl, AddressSub (BBL_CSIZE(orig_bbl), INS_CSIZE(i_ins)));
      BBL_SET_CSIZE(split_off, AddressAdd (BBL_CSIZE(split_off), INS_CSIZE(i_ins)));
    }
  }

  BBL_SET_SUCC_FIRST(split_off, BBL_SUCC_FIRST(orig_bbl));
  BBL_SET_SUCC_LAST(split_off, BBL_SUCC_LAST(orig_bbl));
  BBL_FOREACH_SUCC_EDGE(split_off, i_edge)
  {
    CFG_EDGE_SET_HEAD(i_edge, split_off);
  }
  BBL_SET_SUCC_FIRST(orig_bbl, NULL);
  BBL_SET_SUCC_LAST(orig_bbl, NULL);

  if (!IS_DATABBL(split_off) && BBL_FUNCTION(orig_bbl))
  {
    BblInsertInFunction (split_off, BBL_FUNCTION(orig_bbl));
  }

  CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate (cfg, orig_bbl, split_off, ET_FALLTHROUGH), BBL_EXEC_COUNT(orig_bbl));
  BBL_SET_EXEC_COUNT(split_off, BBL_EXEC_COUNT(orig_bbl));

  for (ext = CFG_EXITBLOCKS(cfg); ext != NULL; ext = ext->next)
  {
    if (ext->exit_bbl == orig_bbl)
    {
      t_ins *ins;
      t_bool found = FALSE;

      BBL_FOREACH_INS(orig_bbl, ins)
      {
        if (ins == ext->exit_ins)
        {
          found = TRUE;
          break;
        }
      }
      if (found)
        break;
      ext->exit_bbl = split_off;
    }
  }

  /* insert in chain if necessary */
  if (BBL_FIRST_IN_CHAIN(orig_bbl))
  {
    BBL_SET_FIRST_IN_CHAIN(split_off, BBL_FIRST_IN_CHAIN(orig_bbl));
    if (BBL_LAST_IN_CHAIN(orig_bbl) == orig_bbl)
    {
      t_bbl *bbl;
      CHAIN_FOREACH_BBL(BBL_FIRST_IN_CHAIN(orig_bbl), bbl)
        BBL_SET_LAST_IN_CHAIN(bbl, split_off);
    }
    BBL_SET_LAST_IN_CHAIN(split_off, BBL_LAST_IN_CHAIN(orig_bbl));

    BBL_SET_NEXT_IN_CHAIN(split_off, BBL_NEXT_IN_CHAIN(orig_bbl));
    BBL_SET_NEXT_IN_CHAIN(orig_bbl, split_off);
    if (BBL_NEXT_IN_CHAIN(split_off))
    {
      BBL_SET_PREV_IN_CHAIN(BBL_NEXT_IN_CHAIN(split_off), split_off);
    }
    BBL_SET_PREV_IN_CHAIN(split_off, orig_bbl);
  }

  /* hook for others to add functionality: setting dynamic member fields etc. */
  DiabloBrokerCall ("BblSplitAfter", orig_bbl, split_off);

  return split_off;
}

t_bbl *
BblSplitBlock (t_bbl * orig_bbl, t_ins * where, t_bool before)
{
  return RealBblSplitBlock (orig_bbl, where, before, TRUE);
}

t_bbl *
BblSplitBlockNoTestOnBranches (t_bbl * orig_bbl, t_ins * where, t_bool before)
{
  return RealBblSplitBlock (orig_bbl, where, before, FALSE);
}

void
BblUnlinkFromFunction (t_bbl * bbl)
{
  t_function *fun = BBL_FUNCTION(bbl);

  if (!fun)
    return;

  if (BBL_PREV_IN_FUN(bbl))
  {
    BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(bbl), BBL_NEXT_IN_FUN(bbl));
  }
  else
  {
    /* this is in fact dangerous. if the first bbl of a function is killed
     * and we're not killing the entire function, we should look for the new
     * function entry and make sure that block becomes the new FUNCTION_BBL_FIRST
     */
    FUNCTION_SET_BBL_FIRST(fun, BBL_NEXT_IN_FUN(bbl));
  }

  if (BBL_NEXT_IN_FUN(bbl))
  {
    BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(bbl), BBL_PREV_IN_FUN(bbl));
  }
  else
  {
    FUNCTION_SET_BBL_LAST(fun, BBL_PREV_IN_FUN(bbl));
  }

  /* Now clear some pointers */
  BBL_SET_FUNCTION(bbl, NULL);
  BBL_SET_PREV_IN_FUN(bbl, NULL);
  BBL_SET_NEXT_IN_FUN(bbl, NULL);

  return;
}

void
BblInsertInChainAfter(t_bbl *new_bbl, t_bbl *chain_bbl)
{
  t_bbl *iter;

  BBL_SET_PREV_IN_CHAIN (new_bbl, chain_bbl);
  BBL_SET_NEXT_IN_CHAIN (new_bbl, BBL_NEXT_IN_CHAIN (chain_bbl));
  BBL_SET_NEXT_IN_CHAIN (chain_bbl, new_bbl);
  if (BBL_NEXT_IN_CHAIN (new_bbl))
    BBL_SET_PREV_IN_CHAIN (BBL_NEXT_IN_CHAIN (new_bbl), new_bbl);
  BBL_SET_FIRST_IN_CHAIN (new_bbl, BBL_FIRST_IN_CHAIN (chain_bbl));
  if (BBL_NEXT_IN_CHAIN (new_bbl))
    BBL_SET_LAST_IN_CHAIN (new_bbl, BBL_LAST_IN_CHAIN (chain_bbl));
  else
    CHAIN_FOREACH_BBL (BBL_FIRST_IN_CHAIN (new_bbl), iter)
      BBL_SET_LAST_IN_CHAIN (iter, new_bbl);
}


void
BblInsertInChainBefore(t_bbl *new_bbl, t_bbl *chain_bbl)
{
  t_bbl *iter;

  BBL_SET_NEXT_IN_CHAIN (new_bbl, chain_bbl);
  BBL_SET_PREV_IN_CHAIN (new_bbl, BBL_PREV_IN_CHAIN (chain_bbl));
  BBL_SET_PREV_IN_CHAIN (chain_bbl, new_bbl);
  if (BBL_NEXT_IN_CHAIN (new_bbl))
    BBL_SET_PREV_IN_CHAIN (BBL_NEXT_IN_CHAIN (new_bbl), new_bbl);
  BBL_SET_LAST_IN_CHAIN (new_bbl, BBL_LAST_IN_CHAIN (chain_bbl));
  if (BBL_PREV_IN_CHAIN (new_bbl))
    BBL_SET_FIRST_IN_CHAIN (new_bbl, BBL_FIRST_IN_CHAIN (chain_bbl));
  else
    CHAIN_FOREACH_BBL (BBL_FIRST_IN_CHAIN (new_bbl), iter)
      BBL_SET_FIRST_IN_CHAIN (iter, new_bbl);
}


/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
