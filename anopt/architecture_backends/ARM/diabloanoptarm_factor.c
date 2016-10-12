/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
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
t_uint32 ArmBblFingerprint(t_bbl *bbl)
{
  t_arm_ins *ins;
  t_uint32 key = 0;

  if (BBL_NINS(bbl) < 3) return 0;

  BBL_FOREACH_ARM_INS(bbl,ins)
  {
    key = (key << 4) | (ARM_INS_OPCODE(ins) & 0xf);
    if (key > 0x10000000)
      key = (key & 0xffff) ^ ((key >> 16) & 0xffff);
  }
  key = (key & 0xffff) ^ ((key >> 16) & 0xffff);
  key = (key << 3) | min(7,BBL_NINS(bbl));
  return key;
}

/* can a bbl be factored out? */
t_bool ArmBblCanFactor(t_bbl *bbl)
{
  t_cfg_edge *edge;
  t_arm_ins *ins;

  if (BBL_NINS(bbl) < 3) return FALSE;	/* too small */
  if (ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(bbl))) && BBL_NINS(bbl) < 4) return FALSE;
  
  /* don't factor blocks that define or use r14 */
  BBL_FOREACH_ARM_INS_R(bbl,ins)
  {
    if (ArmIsControlflow(ins)) continue;
    if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R14)) return FALSE;
    if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R14)) return FALSE;
  }
  
  /* don't factor basic blocks that end in indirect function calls:
   * the architecture-independent code splits off only the last instruction
   * of a basic block for control flow, but here you cannot separate the 
   * definition of r14 and the actual call instruction, so we just let
   * these pass */
  if (ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(bbl))) && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl))) != ARM_BL)
  {
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
      if (CFG_EDGE_CAT(edge) == ET_CALL)
	return FALSE;
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
 * 	- always using r14 as the return register (we could save one instruction if we use another register when
 * 	  r14 is in use)
 * 	- not trying to save r14 on the stack if there are no free registers
 */
t_bool ArmBblFactor(t_equiv_bbl_holder *equivs, t_bbl *master)
{
  static int nfactors = 0;
  t_cfg *cfg = BBL_CFG(master);
  t_bbl *new;
  t_arm_ins *ins;
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
  t_bool block_has_no_memops;
  t_bool block_does_not_define_stack_pointer;

  if (ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(master)))) FATAL(("Expect non control flow instructions!"));

  /* not yet supported for thumb */
  if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(master))) & FL_THUMB)
    return FALSE;

  /* calculate some statistics and see if it's worth 
   * factoring anything {{{ */
  save_regs = Malloc(sizeof(t_reg)*equivs->nbbls);
  need_save = Malloc(sizeof(t_bool)*equivs->nbbls);
  can_factor = Malloc(sizeof(t_bool)*equivs->nbbls);
  save_on_stack = Malloc(sizeof(t_bool)*equivs->nbbls);

  block_has_no_memops = TRUE;
  block_does_not_define_stack_pointer = TRUE;
  BBL_FOREACH_ARM_INS(master,ins)
  {
    RegsetSetUnion(block_defuse,ARM_INS_REGS_USE(ins));
    RegsetSetUnion(block_defuse,ARM_INS_REGS_DEF(ins));
	 if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13))
		block_does_not_define_stack_pointer = FALSE;
    switch (ARM_INS_TYPE(ins))
    {
      case IT_LOAD:
      case IT_STORE:
      case IT_LOAD_MULTIPLE:
      case IT_STORE_MULTIPLE:
      case IT_FLT_LOAD:
      case IT_FLT_STORE:
	block_has_no_memops = FALSE;
	break;
      default:
	; /* keep the compiler happy */
    }
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

    if (!RegsetIn(dead,ARM_REG_R14))
    {
      t_reg r;
      REGSET_FOREACH_REG(dead,r) break;
      if (r < ARM_REG_R15 && r >= ARM_REG_R0)
      {
	need_save[i] = TRUE;
	save_regs[i] = r;
	blocks_with_saves++;
      }
      else if (block_has_no_memops && block_does_not_define_stack_pointer)
      {
	save_on_stack[i] = TRUE;
	blocks_with_stack_saves++;
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
  else
  {
     VERBOSE(1,(" Factoring gain: %d instructions",factor_blocks*BBL_NINS(master)-(BBL_NINS(master) + 1 + factor_blocks + 2*(blocks_with_saves+blocks_with_stack_saves))));
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
	VERBOSE(0,("save r14 in r%d\n\n",save_regs[i]));
    }
  }
#endif
  /* }}} */

  /* create the new function */
  new = BblDup(master);
  ins = ArmInsNewForBbl(new);
  ArmInsMakeMov(ins,ARM_REG_R15,ARM_REG_R14,0,ARM_CONDITION_AL);
  ArmInsAppendToBbl(ins,new);
  BBL_SET_EXEC_COUNT(new, 0);

  BBL_FOREACH_ARM_INS(new,ins)
    ARM_INS_SET_EXEC_COUNT(ins, 0);

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

    /* split off an empty block at the end, to carry the r14 restore instruction if necessary */
    next = BblSplitBlock(orig,BBL_INS_LAST(orig),FALSE);
    
    BBL_SET_EXEC_COUNT(next, BBL_EXEC_COUNT(orig));

    /* remove the fallthrough edge BblSplitBlock added */
    CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(orig)));

    ins = T_ARM_INS(BBL_INS_FIRST(new));

    BBL_SET_EXEC_COUNT(new, BBL_EXEC_COUNT(new)+BBL_EXEC_COUNT(orig));

    while (BBL_INS_FIRST(orig))
      {
	ARM_INS_SET_EXEC_COUNT(ins, ARM_INS_EXEC_COUNT(ins)+ARM_INS_EXEC_COUNT(T_ARM_INS(BBL_INS_FIRST(orig))));
	InsKill(BBL_INS_FIRST(orig));
	ins = ARM_INS_INEXT(ins);
      }

    /* add save and restore of r14 if necessary */
    if (need_save[i])
    {
      ins = ArmInsNewForBbl(orig);
      ArmInsMakeMov(ins,save_regs[i],ARM_REG_R14,0,ARM_CONDITION_AL);
      ArmInsAppendToBbl(ins,orig);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));

      ins = ArmInsNewForBbl(next);
      ArmInsMakeMov(ins,ARM_REG_R14,save_regs[i],0,ARM_CONDITION_AL);
      ArmInsPrependToBbl(ins,next);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(next));
    }
    else if (save_on_stack[i])
    {
      ins = ArmInsNewForBbl(orig);
      ArmInsMakeStr(ins,ARM_REG_R14,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,TRUE,FALSE,TRUE);
      ArmInsAppendToBbl(ins,orig);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));

      ins = ArmInsNewForBbl(next);
      ArmInsMakeLdr(ins,ARM_REG_R14,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,FALSE,TRUE,FALSE);
      ArmInsPrependToBbl(ins,next);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(next));
    }

    /* add call and necessary edges */
    ins = ArmInsNewForBbl(orig);
    ArmInsAppendToBbl(ins,orig);
    ArmInsMakeCondBranchAndLink(ins,ARM_CONDITION_AL);
    CfgEdgeCreateCall(cfg,orig,new,next,FunctionGetExitBlock(factor));
    ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));
  }

  ARM_INS_SET_EXEC_COUNT(T_ARM_INS(BBL_INS_LAST(new)), BBL_EXEC_COUNT(new));

cleanup_and_exit:
  Free(save_regs);
  Free(save_on_stack);
  Free(need_save);
  Free(can_factor);
  return retval;
}

void ArmBblFactorInit(t_cfg * cfg)
{
	CFG_SET_BBL_FACTOR(cfg,ArmBblFactor);
	CFG_SET_BBL_FINGERPRINT(cfg,ArmBblFingerprint);
	CFG_SET_BBL_CAN_BE_FACTORED(cfg,ArmBblCanFactor);
}

void ArmEpilogueFactorAfter(t_bbl * bbl, t_bbl * master)
{
        /* A new branch instruction has been appended to 'bbl'
         * by the caller, DoEpilogueFactoring. However, the Thumb
         * flag for this instruction is not set yet. This is what
         * we do here */

        /* A BBL can't contain both ARM and Thumb instructions.
         * So, if the BBL is not empty, and the first instruction
         * of this BBL is a Thumb instruction, the last instruction
         * (the newly created branch instruction) has to be made Thumb. */
        if (BBL_INS_FIRST(master)
                && (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(master))) & FL_THUMB))
        {
                /* sanity check */
                ASSERT(BBL_INS_FIRST(bbl), ("expected non-empty BBL, got @eiB", bbl));
                ASSERT(ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_B, ("Unconditional branch expected, but got @I", BBL_INS_LAST(bbl)));

                ARM_INS_SET_FLAGS(T_ARM_INS(BBL_INS_LAST(bbl)), ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(bbl))) | FL_THUMB);
        }
}

void ArmFunctionFactorAfterJumpCreation(t_bbl * jump, t_bbl * existing)
{
        /* During whole function factoring, branches are inserted when
         * Diablo encounters identical chunks of code. These branches
         * have to be made Thumb when necessary. */

        ASSERT(BBL_INS_FIRST(jump), ("expected non-empty new jump BBL @eiB", jump));
        ASSERT(BBL_INS_FIRST(jump)==BBL_INS_LAST(jump) && ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(jump)))==ARM_B,
                ("jump BBL is expected to contain only one branch instruction, but actually is @eiB", jump));
        while (!BBL_INS_FIRST(existing))
        {
          ASSERT(BBL_SUCC_FIRST(existing), ("empty BBLs are expected to have at least one outgoing edge, got @eiB", existing));
          existing = CFG_EDGE_TAIL(BBL_SUCC_FIRST(existing));
        }
        ASSERT(BBL_INS_FIRST(existing), ("ka-boom!"));

        if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(existing))) & FL_THUMB)
        {
                ARM_INS_SET_FLAGS(T_ARM_INS(BBL_INS_FIRST(jump)), ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(jump))) | FL_THUMB);
                VERBOSE(3, ("Thumbed\n   JUMP @eiB\n   EXISTING @eiB", jump, existing));
        }
}

/* vim: set shiftwidth=2 foldmethod=marker: */
