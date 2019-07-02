/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.hpp>
#include <memory>
#include <set>

using namespace std;

#define NOCOPY 33
#define FREE   34

#ifdef FACTOR_SUPPORT
extern t_uint32 killcount;                              /*Defined in diablo_factor.c*/
extern t_uint32 addcount;
extern t_uint32 count;
#else
t_uint32 killcount;
t_uint32 addcount;
t_uint32 count;
#endif




#ifndef min
#define min(x,y)        ((x) < (y) ? (x) : (y))
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

  if (BBL_NINS(bbl) < 3) return FALSE;  /* too small */
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
 *      - no control flow allowed at the end of the block
 *      - always using r14 as the return register (we could save one instruction if we use another register when
 *        r14 is in use)
 *      - not trying to save r14 on the stack if there are no free registers
 */
t_bool ArmBblFactor(t_equiv_bbl_holder *equivs, t_bbl *master)
{
  static int nfactors = 0;
  t_cfg *cfg = BBL_CFG(master);
  t_bbl *new_bbl;
  t_arm_ins *ins;
  t_function *factor, *f;
  int i;
  char name[80];
  t_regset live, dead;
  t_regset block_defuse = RegsetNew();
  t_bool retval = TRUE;

  int blocks_with_saves, factor_blocks, blocks_with_stack_saves;
  t_bool block_has_no_memops;
  t_bool block_does_not_define_stack_pointer;

  if (ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(master)))) FATAL(("Expect non control flow instructions!"));

  /* not yet supported for thumb */
  if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(master))) & FL_THUMB)
    return FALSE;

  /* calculate some statistics and see if it's worth
   * factoring anything {{{ */
  /* array of registers to be saved */
  auto save_regs = std::make_unique<t_reg[]>(equivs->nbbls);
  /* array of booleans whether a register should be saved or not */
  auto need_save = make_unique<t_bool[]>(equivs->nbbls);
  /* array of booleans whether factoring can be applied or not */
  auto can_factor = make_unique<t_bool[]>(equivs->nbbls);
  /* array of booleans whether the register should be saved on the stack or not, if any */
  auto save_on_stack = make_unique<t_bool[]>(equivs->nbbls);

  /* calculate the defuse regset of the MASTER block
   * + whether the stack pointer is defined
   * + whether memory operations are present */
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

  FunctionSet functions_to_log;
  BblSet all_bbls;
  BblSet possible_bbls;

  blocks_with_saves = blocks_with_stack_saves = factor_blocks = 0;
  for (i = 0; i < equivs->nbbls; i++)
  {
    t_bbl *bbl = equivs->bbl[i];

    RegsetSetDup(live,BBL_REGS_LIVE_OUT(bbl));
    RegsetSetUnion(live,block_defuse);
    RegsetSetDup(dead,live);
    RegsetSetInvers(dead);
    RegsetSetIntersect(dead,CFG_DESCRIPTION(cfg)->int_registers);

    /* the most optimistic case */
    can_factor[i] = TRUE;
    save_on_stack[i] = FALSE;
    need_save[i] = FALSE;

    /* if the link register (LR, r14) is not dead, we need to save it somewhere */
    if (!RegsetIn(dead,ARM_REG_R14))
    {
      /* first try to look for a dead register to create a move */
      t_reg r;
      REGSET_FOREACH_REG(dead,r) break;
      if (r < ARM_REG_R15 && r >= ARM_REG_R0)
      {
        need_save[i] = TRUE;
        save_regs[i] = r;
        blocks_with_saves++;
      }
      /* if that doesn't work, save LR on the stack
       * but ONLY if the SP (r13) is not defined in the factorised block AND
       * if no memory operations happen inside the block (possibly aliasing memory!) */
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

    if (can_factor[i]) {
      factor_blocks++;
      functions_to_log.insert(BBL_FUNCTION(bbl));
      possible_bbls.insert(bbl);
    }

    all_bbls.insert(bbl);
  }

/*#define DEBUG_FACTORING*/
  /* is any kind of factoring worth while? */
  if ((BBL_NINS(master) + 1 + factor_blocks + 2*(blocks_with_saves+blocks_with_stack_saves) >= factor_blocks*BBL_NINS(master))
      || !BblFactoringHolderConsiderForFactoring(equivs, can_factor.get()))
  {
#ifdef DEBUG_FACTORING
    VERBOSE(0,("FAIL: no gain %d blocks out of %d size %d\n",factor_blocks,equivs->nbbls,BBL_NINS(master)));
#endif
    return FALSE;
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

  sprintf(name,"factor-%d-0x%x",nfactors++,G_T_UINT32(BBL_OLD_ADDRESS(master)));

  /* start logging transformation */
  START_LOGGING_TRANSFORMATION_NONEWLINE(L_FACTORING, "BblFactoring,%s,%d,", name, BBL_NINS(master));

  /* statistics */
  FactoringResult result;

  /* create the new function */
  new_bbl = BblDup(master);
  result.added_ins_info.nr_added_insns += BBL_NINS(new_bbl);
  ins = ArmInsNewForBbl(new_bbl);
  ArmInsMakeMov(ins,ARM_REG_R15,ARM_REG_R14,0,ARM_CONDITION_AL);
  ArmInsAppendToBbl(ins,new_bbl);
  BBL_SET_EXEC_COUNT(new_bbl, 0);

  BBL_FOREACH_ARM_INS(new_bbl,ins) {
    ARM_INS_SET_EXEC_COUNT(ins, 0);
    FactoringLogInstruction(T_INS(ins), "MERGED");
  }

  factor = FunctionMake(new_bbl,name,FT_NORMAL);
  BblSetOriginalFunctionUID(new_bbl, bbl_factor_function_uid);
  CfgEdgeCreate(cfg,new_bbl,FunctionGetExitBlock(factor),ET_JUMP);
  
  /* log before */
  for (auto f : functions_to_log)
    LOG_MORE(L_FACTORING)
      LogFunctionTransformation("before", f);
  LOG_MORE(L_FACTORING)
    LogFunctionTransformation("before", factor);

  /* selected bbls, also those that will not be transformed */
  FactoringRecordTransformation(all_bbls, BBL_NINS(new_bbl), result, true);

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

    ins = T_ARM_INS(BBL_INS_FIRST(new_bbl));

    BBL_SET_EXEC_COUNT(new_bbl, BBL_EXEC_COUNT(new_bbl)+BBL_EXEC_COUNT(orig));

    result.nr_factored_insns += BBL_NINS(orig);
    result.added_ins_info.nr_added_insns -= BBL_NINS(orig);

    while (BBL_INS_FIRST(orig))
      {
        ARM_INS_SET_EXEC_COUNT(ins, ARM_INS_EXEC_COUNT(ins)+ARM_INS_EXEC_COUNT(T_ARM_INS(BBL_INS_FIRST(orig))));

        string x = "FACTORED:" + to_string(i);
        FactoringLogInstruction(BBL_INS_FIRST(orig), x);
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
      FactoringLogInstruction(T_INS(ins), "ADD");
      result.added_ins_info.AddInstruction(T_INS(ins));

      ins = ArmInsNewForBbl(next);
      ArmInsMakeMov(ins,ARM_REG_R14,save_regs[i],0,ARM_CONDITION_AL);
      ArmInsPrependToBbl(ins,next);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(next));
      FactoringLogInstruction(T_INS(ins), "ADD");
      result.added_ins_info.AddInstruction(T_INS(ins));
    }
    else if (save_on_stack[i])
    {
      ins = ArmInsNewForBbl(orig);
      ArmInsMakeStr(ins,ARM_REG_R14,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,TRUE,FALSE,TRUE);
      ArmInsAppendToBbl(ins,orig);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));
      FactoringLogInstruction(T_INS(ins), "ADD");
      result.added_ins_info.AddInstruction(T_INS(ins));

      ins = ArmInsNewForBbl(next);
      ArmInsMakeLdr(ins,ARM_REG_R14,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,FALSE,TRUE,FALSE);
      ArmInsPrependToBbl(ins,next);
      ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(next));
      FactoringLogInstruction(T_INS(ins), "ADD");
      result.added_ins_info.AddInstruction(T_INS(ins));
    }

    /* add call and necessary edges */
    ins = ArmInsNewForBbl(orig);
    ArmInsAppendToBbl(ins,orig);
    ArmInsMakeCondBranchAndLink(ins,ARM_CONDITION_AL);
    CfgEdgeCreateCall(cfg,orig,new_bbl,next,FunctionGetExitBlock(factor));
    ARM_INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(orig));
    FactoringLogInstruction(T_INS(ins), "ADD");
    result.added_ins_info.AddInstruction(T_INS(ins));
  }

  ARM_INS_SET_EXEC_COUNT(T_ARM_INS(BBL_INS_LAST(new_bbl)), BBL_EXEC_COUNT(new_bbl));
  FactoringLogInstruction(BBL_INS_LAST(new_bbl), "DISPATCH");
  result.added_ins_info.AddInstruction(BBL_INS_LAST(new_bbl));

  /* log after */
  for (auto f : functions_to_log)
    LOG_MORE(L_FACTORING)
      LogFunctionTransformation("after", f);
  LOG_MORE(L_FACTORING)
    LogFunctionTransformation("after", factor);

  FactoringRecordTransformation(all_bbls, BBL_NINS(new_bbl)-1/* for the dispatch instruction */, result, false);
  STOP_LOGGING_TRANSFORMATION(L_FACTORING);

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
