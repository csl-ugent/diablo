#include <diabloanoptppc.h>
#define NOCOPY 33
#define FREE   34

#ifdef FACTOR_SUPPORT
extern t_uint32 killcount;  				/*Defined in diablo_factor.c*/
extern t_uint32 addcount;
extern t_uint32 count;
#else
t_uint32 killcount;
t_uint32 addcount;
t_uint32 count;
#endif

#ifndef min
#define min(x,y)	((x) < (y) ? (x) : (y))
#endif

t_uint32
PpcBblFingerprint(t_bbl *bbl)
{
  t_ppc_ins *ins;
  t_uint32 key = 0;

  if (BBL_NINS(bbl) < 3) return 0;

  BBL_FOREACH_PPC_INS(bbl,ins)
  {
    key = (key << 4) | (PPC_INS_OPCODE(ins) & 0xf);
    if (key > 0x10000000)
      key = (key & 0xffff) ^ ((key >> 16) & 0xffff);
  }
  key = (key & 0xffff) ^ ((key >> 16) & 0xffff);
  key = (key << 3) | min(7,BBL_NINS(bbl));
  return key;
}

/* can a bbl be factored out? */
t_bool
PpcBblCanFactor(t_bbl *bbl)
{
  t_cfg_edge *edge;
  t_ppc_ins *ins;

  if (BBL_NINS(bbl) < 3) return FALSE;	/* too small */
  if (PpcInsIsControlTransfer(BBL_INS_LAST(bbl)) && BBL_NINS(bbl) < 4) return FALSE;

  /* don't factor blocks that contain a control transfer or which read/write LR */
  BBL_FOREACH_PPC_INS_R(bbl,ins)
  {
    if (PpcInsIsControlTransfer(T_INS(ins))) continue;
    if (RegsetIn(PPC_INS_REGS_DEF(ins),PPC_REG_LR)) return FALSE;
    if (RegsetIn(PPC_INS_REGS_USE(ins),PPC_REG_LR)) return FALSE;
  }

  BBL_FOREACH_PRED_EDGE(bbl,edge)
    if (CFG_EDGE_CAT(edge)==ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH)
      return FALSE;

  return TRUE;
}

/* factor out a series of equivalent basic blocks. use master as the basis for the final abstracted function */
/* NOTE: we need accurate liveness information for this! */
/* this is the simplest version of basic block factorization:
 * 	- no control flow allowed at the end of the block
 * 	- not trying to save lr on the stack if there are no free registers
 */
t_bool
PpcBblFactor(t_equiv_bbl_holder *equivs, t_bbl *master)
{
  static int nfactors = 0;
  t_cfg *cfg = BBL_CFG(master);
  t_bbl *new;
  t_ppc_ins *ins;
  t_function *factor;
  int i;
  char name[80];
  t_regset live, dead;
  t_regset block_defuse = RegsetNew();
  t_bool retval = TRUE;

  t_reg *save_regs;
  t_bool *need_save;
  t_bool *can_factor;
  t_bool *save_on_stack;
  int blocks_with_saves, factor_blocks, blocks_with_stack_saves;
  /*  t_bool block_has_no_memops; */

  if (PpcInsIsControlTransfer(BBL_INS_LAST(master))) FATAL(("Expect non control flow instructions!"));

  /* calculate some statistics and see if it's worth 
   * factoring anything {{{ */
  save_regs = Malloc(sizeof(t_reg)*equivs->nbbls);
  need_save = Malloc(sizeof(t_bool)*equivs->nbbls);
  can_factor = Malloc(sizeof(t_bool)*equivs->nbbls);
  save_on_stack = Malloc(sizeof(t_bool)*equivs->nbbls);

  /*  block_has_no_memops = TRUE; */
  BBL_FOREACH_PPC_INS(master,ins)
  {
    RegsetSetUnion(block_defuse,PPC_INS_REGS_USE(ins));
    RegsetSetUnion(block_defuse,PPC_INS_REGS_DEF(ins));
    /*
       if (block_has_no_memops &&
       (PpcInsIsLoad(T_INS(ins)) || PpcInsIsStore(T_INS(ins))))
       block_has_no_memops = FALSE;
     */
  }

  blocks_with_saves = blocks_with_stack_saves = factor_blocks = 0;
  for (i = 0; i < equivs->nbbls; i++)
  {
    RegsetSetDup(live,BBL_REGS_LIVE_OUT(equivs->bbl[i]));
    RegsetSetUnion(live,block_defuse);
    RegsetSetDup(dead,live);
    RegsetSetInvers(dead);
    RegsetSetIntersect(dead,CFG_DESCRIPTION(cfg)->int_registers);

    /* the most optimistic case */
    can_factor[i] = TRUE;
    save_on_stack[i] = FALSE;
    need_save[i] = FALSE;

    if (RegsetIn(live,PPC_REG_LR))
    {
      t_reg r;
      REGSET_FOREACH_REG(dead,r) break;
      if (r >= PPC_REG_R0 && r < PPC_REG_R31)
      {
        need_save[i] = TRUE;
        save_regs[i] = r;
        blocks_with_saves++;
      }
      else
      {
        can_factor[i] = FALSE;
      }
    }
    if (can_factor[i]) factor_blocks++;
  }
  /*#define DEBUG_FACTORING*/
  /* is any kind of factoring worth while? */
  if (BBL_NINS(master) + 1 + factor_blocks + 2*(blocks_with_saves+blocks_with_stack_saves) >= factor_blocks*BBL_NINS(master))
  {
#ifdef DEBUG_FACTORING
    VERBOSE(0,("FAIL: no gain %d blocks out of %d size %d\n",factor_blocks,equivs->nbbls,BBL_NINS(master)));
#endif
    retval = FALSE;
    goto cleanup_and_exit;
  }

#ifdef DEBUG_FACTORING
  VERBOSE(0,("BBL FACTORING\n"));
  VERBOSE(0,("%d blocks out of %d, with %d register saves and %d saves on stack needed\n",
             factor_blocks,equivs->nbbls,blocks_with_saves,blocks_with_stack_saves));
  for (i=0; i<equivs->nbbls; i++)
  {
    if (can_factor[i])
    {
      VERBOSE(0,("@ieB",equivs->bbl[i]));
      if (need_save[i])
        VERBOSE(0,("save lr in r%d\n\n",save_regs[i]));
    }
  }
#endif
  /* }}} */

  /* create the new function */
  new = BblDup(master);
  ins = PpcInsNewForBbl(new);
  PpcInsMakeBlr(ins);
  PpcInsAppendToBbl(ins,new);
  BBL_SET_EXEC_COUNT(new, 0);

  BBL_FOREACH_PPC_INS(new,ins)
    PPC_INS_SET_EXEC_COUNT(ins, 0);

  sprintf(name,"factor-%d-0x%x",nfactors++,G_T_UINT32(BBL_OLD_ADDRESS(master)));
  factor = FunctionMake(new,name,FT_NORMAL);
  CfgEdgeCreate(cfg,new,FunctionGetExitBlock(factor),ET_JUMP);

  /* add calls to the new function */
  for (i = 0; i < equivs->nbbls; i++)
  {
    t_bbl *orig = equivs->bbl[i];
    t_bbl *next;

    if (!can_factor[i]) continue;

    ASSERT(
           BBL_SUCC_FIRST(orig) && 
           BBL_SUCC_FIRST(orig) == BBL_SUCC_LAST(orig) &&
           (CFG_EDGE_CAT(BBL_SUCC_FIRST(orig)) & (ET_FALLTHROUGH|ET_IPFALLTHRU)),
           ("Unexpected successor edge: @ieB",orig));

    /* split off an empty block at the end, to carry the lr restore instruction if necessary */
    next = BblSplitBlock(orig,BBL_INS_LAST(orig),FALSE);

    BBL_SET_EXEC_COUNT(next, BBL_EXEC_COUNT(orig));

    /* remove the fallthrough edge BblSplitBlock added */
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(orig)));

    ins = T_PPC_INS(BBL_INS_FIRST(new));

    BBL_SET_EXEC_COUNT(new, BBL_EXEC_COUNT(new)+BBL_EXEC_COUNT(orig));

    while (BBL_INS_FIRST(orig))
    {
      PPC_INS_SET_EXEC_COUNT(ins, PPC_INS_EXEC_COUNT(ins)+PPC_INS_EXEC_COUNT(T_PPC_INS(BBL_INS_FIRST(orig))));
      InsKill(BBL_INS_FIRST(orig));
      ins = PPC_INS_INEXT(ins);
    }

    /* add save and restore LR if necessary */
    if (need_save[i])
    {
      ins = PpcInsNewForBbl(orig);
      PpcInsMakeMfspr(ins,save_regs[i],PPC_REG_LR);
      PpcInsAppendToBbl(ins,orig);
      PPC_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));

      ins = PpcInsNewForBbl(next);
      PpcInsMakeMtspr(ins,PPC_REG_LR,save_regs[i]);
      PpcInsPrependToBbl(ins,next);
      PPC_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(next));
    }

    /* add call and necessary edges */
    ins = PpcInsNewForBbl(orig);
    PpcInsAppendToBbl(ins,orig);

    PpcInsMakeCall(ins);
    CfgEdgeCreateCall(cfg,orig,new,next,FunctionGetExitBlock(factor));
    PPC_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));
  }

  PPC_INS_SET_EXEC_COUNT(T_PPC_INS(BBL_INS_LAST(new)), BBL_EXEC_COUNT(new));

cleanup_and_exit:
  Free(save_regs);
  Free(save_on_stack);
  Free(need_save);
  Free(can_factor);
  return retval;
}

void
PpcBblFactorInit(t_cfg * cfg)
{
  CFG_SET_BBL_FACTOR(cfg,PpcBblFactor);
  CFG_SET_BBL_FINGERPRINT(cfg,PpcBblFingerprint);
  CFG_SET_BBL_CAN_BE_FACTORED(cfg,PpcBblCanFactor);
}
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
