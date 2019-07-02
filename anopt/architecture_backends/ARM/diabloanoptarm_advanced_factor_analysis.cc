#include "diabloanoptarm_advanced_factor.hpp"

//#define DEBUG_SPLIT_EDGE
//#define DEBUG_SPLIT_EDGE_DOTS
//#define DEBUG_LIVE 0

#define FORBID_VERBOSITY 1

using namespace std;

t_regset status_registers;

#define GetRegsetMember(result, x, bbl) {\
  if (!BBL_ ## x (bbl)) result = NullRegs; \
  else result = *BBL_ ## x (bbl); \
}
#define SetRegsetMember(x, bbl, r) {\
  if (!BBL_ ## x (bbl)) BBL_SET_ ## x (bbl, new t_regset()); \
  *BBL_ ## x (bbl) = r; \
}

#define INS_IS_SWITCH(x) (ARM_INS_ATTRIB(T_ARM_INS(x)) & IF_SWITCHJUMP)

static set<t_function *> forbidden_functions;

void ForbidFunction(t_function *fun, bool include_callees) {
  forbidden_functions.insert(fun);
  VERBOSE(FORBID_VERBOSITY, ("forbidding  @F", fun));

  if (include_callees) {
    auto x = IndicateAllReachableFunctions(fun);
    VERBOSE(FORBID_VERBOSITY, ("forbidding %d reachables from @F", x, fun));

    CFG_FOREACH_FUN(FUNCTION_CFG(fun), fun) {
      if (FUNCTION_FLAGS(fun) & FF_IS_MARKED2) {
        forbidden_functions.insert(fun);
        VERBOSE(FORBID_VERBOSITY, ("  @F", fun));
      }
    }
  }
}

void UpdateUseDef(t_bbl *bbl, t_bbl *original)
{
  if (original)
  {
    BBL_SET_REGS_USE(bbl, BBL_REGS_USE(original));
    BBL_SET_REGS_DEF(bbl, BBL_REGS_DEF(original));
  }
  else
  {
    BBL_SET_REGS_USE(bbl, BblRegsUse(bbl));
    BBL_SET_REGS_DEF(bbl, BblRegsDef(bbl));
  }
}

void DefineRegisterInBbl(t_bbl *bbl, t_reg reg)
{
  t_regset regs = BBL_REGS_DEF(bbl);
  RegsetSetAddReg(regs, reg);
  BBL_SET_REGS_DEF(bbl, regs);
}

void UseRegisterInBbl(t_bbl *bbl, t_reg reg)
{
  t_regset regs = BBL_REGS_USE(bbl);
  RegsetSetAddReg(regs, reg);
  BBL_SET_REGS_USE(bbl, regs);
}

void RemoveUnreachableBbl(t_bbl *bbl)
{
  /* for debugging purposes, it could happen that this BBL is not factored at all.
   * In this case, it does not need to be removed from the CFG, as it is not unreachable. */
  if (BBL_PRED_FIRST(bbl))
    return;

  /* some sanity checks */
  ASSERT(!BBL_PRED_FIRST(bbl), ("this BBL still has an incoming edge! @eiB", bbl));
  //ASSERT(!BBL_REFERS_TO(bbl), ("BBL refers to! @eiB", bbl));
  ASSERT(!BBL_REFED_BY(bbl), ("BBL is referred by! @eiB", bbl));

  /* from CfgRemoveDeadCodeAndDataBlocks */

  t_ins *ins, *ins_tmp;

  bool has_address_producer = false;
  BBL_FOREACH_INS(bbl, ins) {
    for (auto slice : *INS_SLICES(ins))
      slice->Invalidate();

    /* need to keep address producers alive */
    if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_ADDRESS_PRODUCER)
      has_address_producer = true;
  }

  if (!has_address_producer) {
    /* 1. kill the instructions contained within the BBL */
    BBL_FOREACH_INS_SAFE(bbl, ins, ins_tmp)
    {
      /*if (INS_SLICE(ins))
        INS_SLICE(ins)->Invalidate();*/
      //ASSERT(!INS_REFERS_TO(ins), ("instruction refers to! @I: @R", ins, RELOC_REF_RELOC(INS_REFERS_TO(ins))));
      ASSERT(!INS_REFED_BY(ins), ("instruction referred by! @I", ins));
      InsKill(ins);
    }
  }

  /* 2. kill the outgoing edges, and corresponding edges if they exist */
  t_cfg_edge *edge, *edge_tmp;
  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_tmp)
  {
    if (CFG_EDGE_CORR(edge))
    {
      if (CFG_EDGE_REFCOUNT(edge) > 1) continue;
      if (CfgEdgeIsForwardInterproc (edge))
      {
        if (CFG_EDGE_CORR(edge) == edge_tmp)
          edge_tmp = T_CFG_EDGE(CFG_EDGE_SUCC_NEXT(edge_tmp));
        CfgEdgeKill (CFG_EDGE_CORR(edge));
        CFG_EDGE_SET_CORR(edge, NULL);
      }
      else
        CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), NULL);
    }

    CfgEdgeKill(edge);
  }

  /* make sure that the block is NOT in the marked linked list anymore */
  FunctionUnmarkAllBbls(BBL_FUNCTION(bbl));

  if (!has_address_producer) {
    /* 3. kill the block itself */
    BblKill(bbl);
  }
}

static
t_regset RegsLiveIn(t_bbl *bbl)
{
  t_regset live_in = BBL_REGS_LIVE_OUT(bbl);

  RegsetSetDiff(live_in, BBL_REGS_DEF(bbl));
  RegsetSetUnion(live_in, BBL_REGS_USE(bbl));

  return live_in;
}

t_regset RegsLiveOut(t_bbl *bbl)
{
  t_regset live_out = NullRegs;

  t_cfg_edge *e;
  BBL_FOREACH_SUCC_EDGE(bbl, e)
  {
    t_bbl *successor = CFG_EDGE_TAIL(e);
    RegsetSetUnion(live_out, RegsLiveIn(successor));
  }

  return live_out;
}

vector<t_ins *> IterInsAfter(Slice *slice, size_t slice_size) {
  vector<t_ins *> result;

  int min_order_in_slice = INS_ORDER(slice->GetR(slice_size - 1));

  size_t nr_instructions = 0;
  t_ins *ins;
  BBL_FOREACH_INS_R(slice->Bbl(), ins)
  {
    /* we can't break early because the instructions in the BBL may be in any order */

    /* don't take instructions into account that will come before the slice,
     * or instructions that are part of the slice itself */
    if (slice->is_sequence) {
      /* before the sequence */
      if (INS_ORDER(ins) < min_order_in_slice)
        continue;

      /* part of the sequence */
      if (slice->min_order <= INS_ORDER(ins)
          && INS_ORDER(ins) <= slice->max_order) {
        if (slice->address_after.find(T_ARM_INS(ins)) != slice->address_after.end()
            && nr_instructions < slice_size) {
          /* this instruction should be taken into account */
        }
        else {
          if (slice->ContainsInstruction(ins, slice_size)
                || nr_instructions >= slice_size) {
            if (slice->address_before.find(T_ARM_INS(ins)) == slice->address_before.end()) {
              // if (debug) VERBOSE(0, ("in slice @I", ins));
              nr_instructions++;
            }
            continue;
          }
        }
      }
    }
    else {
      /* before the slice */
      if (INS_SLICE_ID(ins) > INS_SLICE_ID(slice->base_instruction))
        continue;

      /* part of the slice */
      if (INS_SLICE_ID(ins) == INS_SLICE_ID(slice->base_instruction)) {
        if (slice->ContainsInstruction(ins, slice_size)
            || nr_instructions >= slice_size) {
          // if (debug) VERBOSE(0, ("in slice @I", ins));
          nr_instructions++;
          continue;
        }
      }
    }

    /* only look at instructions that will be scheduled AFTER the slice */
    result.push_back(ins);
  }

  return result;
}

vector<t_ins *> IterInsIn(Slice *slice, size_t slice_size) {
  vector<t_ins *> result;

  bool limit = slice_size > 0;
  size_t to_go = slice_size;

  t_ins *ins;
  BBL_FOREACH_INS_R(slice->Bbl(), ins)
  {
    /* only look at instructions that are part of the slice */
    if (slice->is_sequence) {
      if (slice->address_before.find(T_ARM_INS(ins)) != slice->address_before.end()
          /* the instruction is scheduled _before_ the slice */
          || INS_ORDER(ins) < slice->min_order
          /* the instruction is scheduled _after_ the slice */
          || INS_ORDER(ins) > slice->max_order
          || !slice->ContainsInstruction(ins, slice_size)) {
        // if (debug) VERBOSE(0, ("skip1 @I", ins));
        continue;
      }
    }
    else {
      if (INS_SLICE_ID(ins) != INS_SLICE_ID(slice->base_instruction) || !slice->ContainsInstruction(ins, slice_size)) {
        // if (debug) VERBOSE(0, ("skip2 @I", ins));
        continue;
      }
    }

    result.push_back(ins);

    if (limit)
    {
      to_go--;
      if (to_go == 0)
        break;
    }
  }

  return result;
}

/* no need to take the slice size into account as we only need to look
 * at the instructions AFTER the slice */
t_regset SliceRegsLiveAfter(Slice *slice, size_t slice_size)
{
#ifdef DEBUG_LIVE
  bool debug = slice->uid == DEBUG_LIVE;
#else
  bool debug = false;
#endif

  if (debug)
    VERBOSE(0, ("live-after for %d %s", slice_size, slice->Print().c_str()));
  t_regset result = BblRegsLiveAfter(slice->Bbl());

  for (auto ins : IterInsAfter(slice, slice_size)) {
    /* only look at instructions that will be scheduled AFTER the slice */
    if (!INS_IS_CONDITIONAL(ins))
      RegsetSetDiff(result, INS_REGS_DEF(ins));
    RegsetSetUnion(result, INS_REGS_USE(ins));

    if (debug) VERBOSE(0, ("did @I", ins));
  }

  if (debug) VERBOSE(0, ("result @X", CPREGSET(BBL_CFG(slice->Bbl()), result)));
  return result;
}

t_regset SliceRegsLiveBefore(Slice *slice, size_t slice_size)
{
#ifdef DEBUG_LIVE
  bool debug = slice->uid == DEBUG_LIVE;
#else
  bool debug = false;
#endif

  /* this function call already takes all instructions into account that
   * will be scheduled AFTER the slice */
  t_regset result = SliceRegsLiveAfter(slice, slice_size);
  if (debug) {
    VERBOSE(0, ("calculate live-before for %d %d %s", (slice_size > 0), slice_size, slice->Print().c_str()));
    VERBOSE(0, ("live-after @X", CPREGSET(BBL_CFG(slice->Bbl()), result)));
  }

  for (auto ins : IterInsIn(slice, slice_size)) {
    /* only look at instructions that are part of the slice itself */
    if (!INS_IS_CONDITIONAL(ins))
      RegsetSetDiff(result, INS_REGS_DEF(ins));
    RegsetSetUnion(result, INS_REGS_USE(ins));

    if (debug)
      VERBOSE(0, ("@I: @X", ins, CPREGSET(BBL_CFG(slice->Bbl()), result)));
  }

  if (debug)
    VERBOSE(0, ("live-before @X", CPREGSET(BBL_CFG(slice->Bbl()), result)));

  return result;
}

static
t_regset UsableRegisters(t_regset regs)
{
  t_regset result = RegsetNew();

  t_reg r;
  REGSET_FOREACH_REG(regs, r)
  {
    if (r > ARM_REG_R14)
      break;
    if (r == ARM_REG_R13)
      break;

    RegsetSetAddReg(result, r);
  }

  return result;
}

t_regset SliceRegsDeadThrough(Slice *slice, size_t slice_size)
{
  if (slice->combine_data_fixed)
    return slice->dead_through;

  t_regset result = SliceRegsLiveBefore(slice, slice_size);
  RegsetSetInvers(result);

  for (size_t i = 0; i < slice_size; i++)
  {
    t_ins *ins = slice->Get(i, slice_size);

    RegsetSetDiff(result, INS_REGS_DEF(ins));
  }

  return UsableRegisters(result);
}

bool CanAddInstructionToBbl(t_bbl *bbl)
{
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e)
  {
    /* careful with switch instructions and tables! */
    t_bbl *pred = CFG_EDGE_HEAD(e);

    if (BBL_INS_LAST(pred)
        && INS_IS_SWITCH(BBL_INS_LAST(pred)))
    {
      /* can we add instructions to the cases? */
      if (ARM_INS_ATTRIB(T_ARM_INS(BBL_INS_LAST(pred))) & IF_SWITCHJUMP_FIXEDCASESIZE)
        return false;

      /* the predecessor ends with a switch instruction */
      if (CfgEdgeIsFallThrough(e))
        /* changing fallthrough paths of switch instructions is risky business! */
        return false;

      /* if no fallthrough, this is a (IP-)switch edge */
      if (BBL_NINS(bbl) == 1
          && RegsetIn(INS_REGS_DEF(BBL_INS_LAST(bbl)), ARM_REG_R15))
        /* this BBL contains only a branch instruction,
         * which could possibly mean that the switch instruction is implemented as a branch-instruction table */
        return false;
    }
  }

  return true;
}

bool BblHasRelocationFromIns(t_bbl *bbl)
{
  for (auto rr = BBL_REFED_BY(bbl); rr; rr = RELOC_REF_NEXT(rr))
  {
    /* return TRUE when the FROM of this relocation is a real instruction (not data) */
    t_relocatable *from = RELOC_FROM(RELOC_REF_RELOC(rr));
    if (RELOCATABLE_RELOCATABLE_TYPE(from) == RT_INS
        && !INS_IS_DATA(T_INS(from)))
      return true;
  }

  return false;
}

static
bool FunctionIsForbidden(t_function *fun) {
  if (!fun)
    return true;

  if (forbidden_functions.find(fun) != forbidden_functions.end())
    return true;

  return false;
}

bool PossiblyFactorBbl(t_bbl *bbl)
{
  auto log_impossible = [] (string reason, t_bbl *bbl) {
    t_arm_ins *ins;
    BBL_FOREACH_ARM_INS(bbl, ins) {
      t_string insn = StringIo("IMPOSSIBLE:%s:@I", reason.c_str(), ins);
      FactoringLogComment(std::string(insn));
      Free(insn);
    }
  };

#ifdef DONT_FACTOR_NONEXECUTED_BBLS
  if (BBL_EXEC_COUNT(bbl) == 0)
    return false;
#endif

  /* BBLs inside forbidden functions */
  if (FunctionIsForbidden(BBL_FUNCTION(bbl))) {
    log_impossible("forbidden", bbl);
    return false;
  }

  /* fallthrough BBLs of switch statements should NOT be touched!
   * This is especially the case when e.g., the switch is a conditional jump instruction
   * where the fallthrough BBL contains another jump instruction and the address of the
   * switch data table is assumed to be hardcoded after this one jump instruction. */
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e)
    if ((CFG_EDGE_CAT(e) == ET_FALLTHROUGH || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
        && BBL_INS_LAST(CFG_EDGE_HEAD(e))
        && INS_IS_SWITCH(BBL_INS_LAST(CFG_EDGE_HEAD(e)))) {
      log_impossible("switchFT", bbl);
      return false;
    }

  if (BblHasRelocationFromIns(bbl)) {
    log_impossible("relocation", bbl);
    return false;
  }

  /* is the BBL in a code mobility region? */
  t_bool is_in_code_mobility_region = false;
  DiabloBrokerCall("BblIsInCodeMobilityRegion", bbl, &is_in_code_mobility_region);
  if (is_in_code_mobility_region) {
    log_impossible("code-mobility", bbl);
    return false;
  }

  /* is the BBL in an anti debugging region? */
  t_bool is_in_anti_debugging_region = false;
  DiabloBrokerCall("BblIsInSelfDebuggingRegion", bbl, &is_in_anti_debugging_region);
  if (is_in_anti_debugging_region) {
    log_impossible("anti-debugging", bbl);
    return false;
  }

  /* any immediate predecessor in an anti debugging region? */
  BBL_FOREACH_PRED_EDGE(bbl, e) {
    DiabloBrokerCall("BblIsInSelfDebuggingRegion", CFG_EDGE_HEAD(e), &is_in_anti_debugging_region);
    if (is_in_anti_debugging_region) {
      log_impossible("anti-debugging-pred", bbl);
      return false;
    }
  }

  return true;
}

vector<t_reg> RegsetToVector(t_regset regs)
{
  vector<t_reg> result;

  t_reg r;
  REGSET_FOREACH_REG(regs, r)
    result.push_back(r);

  return result;
}

vector<CfgPath> GetIncomingPathsRecursive(t_bbl *bbl, t_cfg_edge *came_from_edge, bool& success)
{
  vector<CfgPath> incoming_paths;
  t_cfg_edge *e;

  if (BBL_IS_HELL(bbl)
      || BblIsMarked(bbl))
    success = false;
  else
  {
    /* mark this BBL as visited so we don't end up in an infinite loop */
    BblMark(bbl);

    if (came_from_edge == NULL)
    {
      /* this is the start of the paths */
      BBL_FOREACH_PRED_EDGE(bbl, e)
      {
        auto new_paths = GetIncomingPathsRecursive(CFG_EDGE_HEAD(e), e, success);

        for (auto new_path : new_paths)
          incoming_paths.push_back(new_path);
      }
    }
    else
    {
      /* this BBL is in the middle of some path(s) */
      //DEBUG(("edge @E", came_from_edge));

      /* TRUE if this BBL is an empty BBL on a fallthrough path */
      bool empty_on_ft = BBL_NINS(bbl) == 0
                          && (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) == NULL && CfgEdgeIsFallThrough(BBL_PRED_FIRST(bbl)))
                          && (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) == NULL && CfgEdgeIsFallThrough(BBL_SUCC_FIRST(bbl)));

      if ((BBL_NINS(bbl) == 0 && !empty_on_ft)
          || !CanAddInstructionToBbl(bbl))
      {
        //DEBUG(("special case, iterating over all incoming edges!"));
        /* need to iterate over incoming edges */
        BBL_FOREACH_PRED_EDGE(bbl, e)
        {
          auto new_paths = GetIncomingPathsRecursive(CFG_EDGE_HEAD(e), e, success);
          for (auto new_path : new_paths)
          {
            CfgPath assembled = new_path;
            assembled.insert(assembled.begin(), came_from_edge);
            incoming_paths.push_back(assembled);
          }
        }
      }
      else
      {
        //DEBUG(("can add instructions here!"));
        /* end of path */
        ASSERT(BBL_NINS(bbl) > 0 || empty_on_ft, ("wiiiiii! @eiB", bbl));
        incoming_paths.push_back(CfgPath{came_from_edge});
      }
    }
  }

  return incoming_paths;
}

/* HEAD of first element = BBL to be modified,
 * TAIL of last  element = BBL itself */
/* success is should be set to TRUE before this function is called
 * and is reset to FALSE when a conflicting to-be-modified BBL is found. */
vector<CfgPath> GetIncomingPaths(t_bbl *bbl, bool& success)
{
  //DEBUG(("incoming paths for @B", bbl));
  success = true;
  BblMarkInit();
  auto result = GetIncomingPathsRecursive(bbl, NULL, success);

  int idx = 0;
  for (auto& cfg_path : result)
  {
    reverse(cfg_path.begin(), cfg_path.end());
    /*for (auto e : cfg_path)
      DEBUG(("path[%d] @E", idx, e));
    idx++;*/
  }

  return result;
}

bool BblShouldBeFactored(t_bbl *bbl)
{
  if (BBL_IS_HELL(bbl))
    return false;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e)
  {
    /* where do we have to save the continuation address if one of the predecessors is a HELL bbl? I don't know... */
    if (BBL_IS_HELL(CFG_EDGE_HEAD(e)))
      return false;
  }

  return true;
}

bool BblIsReturnSite(t_bbl *bbl)
{
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e)
    if (CFG_EDGE_CAT(e) == ET_RETURN)
      return true;

  return false;
}

bool EdgeFromConditionalBranch(t_cfg_edge *edge, t_arm_condition_code& edge_condition, t_reg& compare_register, t_uint64& compare_value) {
  bool result = false;

  t_bbl *head = CFG_EDGE_HEAD(edge);

  t_arm_ins *last_ins = T_ARM_INS(BBL_INS_LAST(head));
  if (last_ins
      && ArmIsControlflow(last_ins)
      && ArmInsIsConditional(last_ins)) {
    t_arm_condition_code branch_condition = ARM_INS_CONDITION(last_ins);

    if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH
        || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      /* invert the condition code, on ARM this is done by inverting the lowest bit */
      edge_condition = static_cast<t_arm_condition_code>(static_cast<t_uint32>(branch_condition) ^ 0x1);
    else
      /* the edge condition is equal to the branch condition */
      edge_condition = branch_condition;

    if (BblEndsWithConditionalBranchAfterCMP(head)) {
        result = true;

        t_arm_ins *compare_instruction = FindCmpThatDeterminesJump(last_ins);
        compare_register = ARM_INS_REGB(compare_instruction);
        compare_value = ARM_INS_IMMEDIATE(compare_instruction);
    }
  }

  return result;
}

bool RegisterInfoForConditionalBranch(t_arm_ins *branch_ins, t_cfg_edge *taken, t_reg& reg, t_uint64& value, t_arm_condition_code& condition) {
  if (!(ArmIsControlflow(branch_ins) && ArmInsIsConditional(branch_ins)))
    return false;
  if (!BblEndsWithConditionalBranchAfterCMP(ARM_INS_BBL(branch_ins)))
    return false;

  bool condition_is_eq = ARM_INS_CONDITION(branch_ins) == ARM_CONDITION_EQ;
  bool condition_is_ne = ARM_INS_CONDITION(branch_ins) == ARM_CONDITION_NE;

  if (!condition_is_eq && !condition_is_ne)
    return false;

  t_arm_ins *cmp = FindCmpThatDeterminesJump(branch_ins);

  /* only support cmp-immediate */
  if (!(ARM_INS_FLAGS(cmp) & FL_IMMED))
    return false;

  /* conditional register and value */
  reg = ARM_INS_REGB(cmp);
  value = ARM_INS_IMMEDIATE(cmp);

  /* condition of the taken edge */
  condition = ARM_INS_CONDITION(branch_ins);
  if (!(CFG_EDGE_CAT(taken) == ET_JUMP || CFG_EDGE_CAT(taken) == ET_IPJUMP))
    condition = static_cast<t_arm_condition_code>(static_cast<t_uint32>(ARM_INS_CONDITION(branch_ins)) ^ 0x1);

  return true;
}

t_bbl *SplitEdge(t_cfg_edge *e, bool add_to_tail)
{
#ifdef DEBUG_SPLIT_EDGE
  static int nr = 0;
#endif

  t_cfg *cfg = CFG_EDGE_CFG(e);
  t_architecture_description *desc = CFG_DESCRIPTION(cfg);

  t_bbl *from = CFG_EDGE_HEAD(e);
  t_bbl *tail = CFG_EDGE_TAIL(e);

#ifdef DEBUG_SPLIT_EDGE
  DEBUG(("splitting edge @E: @eiB @eiB", e, from, tail));
#endif

  /* create a new BBL to put the branch instruction in */
  t_bbl *new_bbl = BblNew(BBL_CFG(tail));
  BblInsertInFunction(new_bbl, BBL_FUNCTION(add_to_tail ? tail : from));
  BBL_SET_AF_FLAGS(new_bbl, BBL_AF_FLAGS(from));

  /* update object tracking information */
  UpdateObjectTrackingAfterEdgeSplit(e, new_bbl, tail);

  /* redirect the original edge and create a fallthough edge */
  CfgEdgeChangeTail(e, new_bbl);
  t_cfg_edge *new_edge = CfgEdgeCreate(BBL_CFG(new_bbl), new_bbl, tail, ET_FALLTHROUGH);
  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(e));

  /* exec count */
  if (CFG_EDGE_CAT(e) == ET_RETURN)
    BblCopyExecInformation(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)), new_bbl);
  else
    BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(e));

  /* update nonzero */
  BblSetNonZeroRegistersAfter(new_bbl, RegsetUnion(BblNonZeroRegistersAfter(from), BblNonZeroRegistersBeforeM(tail)));
  BblSetNonZeroRegistersBeforeM(new_bbl, BblNonZeroRegistersAfter(new_bbl));

  DistributedTableCopyIds(from, new_bbl);

  /* update liveness */
  BBL_SET_REGS_LIVE_OUT(new_bbl, BblRegsLiveAfter(new_bbl));

  bool merged = false;
  if (CfgEdgeIsAF(e)) {
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("edge is AF!"));
#endif

    BBL_SET_PROCSTATE_IN(new_bbl, ProcStateNewDup(CFG_EDGE_PROCSTATE(e)));
  }
  else if (!BBL_PROCSTATE_OUT(from))
  {
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("no previous procstate out!"));
#endif

    ASSERT(BBL_IS_HELL(from) || CFG_EDGE_CAT(e) == ET_RETURN, ("no procstate associated with bbl! @E @eiB", e, from));

    if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(tail)) == NULL
        && BBL_PROCSTATE_IN(tail))
      BBL_SET_PROCSTATE_IN(new_bbl, ProcStateNewDup(BBL_PROCSTATE_IN(tail)));
    else
      BBL_SET_PROCSTATE_IN(new_bbl, ProcStateNew(CFG_DESCRIPTION(BBL_CFG(new_bbl))));
  }
  else {
    t_procstate *source_procstate = BBL_PROCSTATE_OUT(from);
    if (CFG_EDGE_CAT(e) == ET_RETURN) {
      /* this is a conservatove approximation */
      source_procstate = BBL_PROCSTATE_IN(tail);

#ifdef DEBUG_SPLIT_EDGE
      DEBUG(("edge was return"));
#endif
    }
    else if (BBL_INS_LAST(from)
              && ArmInsIsConditional(T_ARM_INS(BBL_INS_LAST(from)))
              && RegsetIn(INS_REGS_DEF(BBL_INS_LAST(from)), ARM_REG_R15)
              && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(tail)) == NULL) {
      /* need to merge in the conditional stuff */
#ifdef DEBUG_SPLIT_EDGE
      DEBUG(("using tail of conditional jump @eiB @eiB", from, tail));
#endif
      source_procstate = BBL_PROCSTATE_IN(tail);
    }
    t_procstate *new_procstate = nullptr;
    if (source_procstate != nullptr)
      new_procstate = ProcStateNewDup(source_procstate);
    else {
      new_procstate = ProcStateNew(desc);
      ProcStateSetAllBot(new_procstate, desc->all_registers);
    }

    /* do we come from a conditional jump? */
    t_reg reg;
    bool reg_is_nonzero;
    MergeConditionalProcstate(e, new_procstate, reg, reg_is_nonzero);
    if (reg_is_nonzero)
      BblSetRegNonZeroAfter(new_bbl, reg);

    BBL_SET_PROCSTATE_IN(new_bbl, new_procstate);
  }

  BBL_SET_PROCSTATE_OUT(new_bbl, ProcStateNewDup(BBL_PROCSTATE_IN(new_bbl)));

  CFG_EDGE_SET_PROP_REGS(BBL_SUCC_FIRST(new_bbl), desc->all_registers);

#if AF_COPY_ANALYSIS
  BBL_SET_EQS_IN(new_bbl, EquationsNew(BBL_CFG(from)));

  if (CfgEdgeIsAF(e))
  {
    /* this is an edge going to a factored block */
    EquationsCopy(BBL_CFG(from), CFG_EDGE_EQUATIONS(e), BBL_EQS_IN(new_bbl));
  }
  else
  {
    /* regular case */
    ASSERT(BBL_EQS_IN(tail), ("no incoming equations for @eiB (edge @E)", tail, e));
    EquationsCopy(BBL_CFG(from), BBL_EQS_IN(tail), BBL_EQS_IN(new_bbl));
  }
#endif

  if (merged) {
#ifdef DEBUG_SPLIT_EDGE_DOTS
    DumpDots(CFG_EDGE_CFG(e), "merge", nr);
#endif

#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("merged @eiB", new_bbl));
    nr++;
#endif
  }

#ifdef DEBUG_SPLIT_EDGE
  DEBUG(("created new @eiB", new_bbl));
#endif
  return new_bbl;
}

t_bbl * AddBranchIndirectionOnEdge(t_cfg_edge *e, bool add_to_tail, AddedInstructionInfo& added_ins_info)
{
  t_bbl *branch_bbl = CFG_EDGE_HEAD(e);
  t_cfg_edge *to_kill = e;
  bool new_block = false;

  if (CFG_EDGE_CAT(e) != ET_FALLTHROUGH
      && CFG_EDGE_CAT(e) != ET_IPFALLTHRU) {
    branch_bbl = SplitEdge(e, add_to_tail);
    to_kill = BBL_SUCC_FIRST(branch_bbl);
    new_block = true;
  }
  EdgeMakeIntraProcedural(e);

  /* create the branch instruction */
  t_arm_ins *ins;
  bool is_thumb = false;
  if (BBL_INS_LAST(CFG_EDGE_HEAD(e))) /* TODO: fixme for Thumb */
    is_thumb = ARM_INS_FLAGS(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(e)))) & FL_THUMB;
  ArmMakeInsForBbl(UncondBranch, Append, ins, branch_bbl, is_thumb);

  /* create the new edge */
  t_bbl *dest = CFG_EDGE_TAIL(to_kill);
  t_cfg_edge *new_edge = CfgEdgeCreate(BBL_CFG(dest), branch_bbl, dest, ET_JUMP);
  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(to_kill));
  EdgeMakeInterprocedural(new_edge);

  /* move AF edge information */
  if (!add_to_tail
      && CfgEdgeIsAF(e))
    CfgEdgeMoveAF(e, new_edge);

  /* correct the outgoing edge by killing it (there could be a corresponding edge if it is IP) */
  t_uint64 old_exec_count = CFG_EDGE_EXEC_COUNT(to_kill);
  CfgEdgeKill(to_kill);

  /* constant propagation */
  CFG_EDGE_SET_PROP_REGS(new_edge, CFG_DESCRIPTION(BBL_CFG(dest))->all_registers);

  /* update liveness information */
  UpdateUseDef(branch_bbl);

  added_ins_info.AddInstruction(T_INS(ins));

  return branch_bbl;
}

void CfgEdgeMakeAF(t_cfg_edge *edge, t_procstate *ps, t_equations eqs)
{
  CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | EF_ADVANCED_FACTORING);

  ASSERT(ps, ("no procstate! @E", edge));
  if (!CFG_EDGE_PROCSTATE(edge))
    CFG_EDGE_SET_PROCSTATE(edge, ProcStateNewDup(ps));
  else
    ProcStateDup(CFG_EDGE_PROCSTATE(edge), ps, CFG_DESCRIPTION(CFG_EDGE_CFG(edge)));

#if AF_COPY_ANALYSIS
  ASSERT(eqs, ("no equations! @E", edge));
  CFG_EDGE_SET_EQUATIONS(edge, EquationsNew(CFG_EDGE_CFG(edge)));
  EquationsCopy(CFG_EDGE_CFG(edge), eqs, CFG_EDGE_EQUATIONS(edge));
#endif
}

void CfgEdgeMoveAF(t_cfg_edge *from, t_cfg_edge *to) {
  CFG_EDGE_SET_FLAGS(from, CFG_EDGE_FLAGS(from) & ~EF_ADVANCED_FACTORING);
  CFG_EDGE_SET_FLAGS(to, CFG_EDGE_FLAGS(to) | EF_ADVANCED_FACTORING);

  CFG_EDGE_SET_PROCSTATE(to, CFG_EDGE_PROCSTATE(from));
  CFG_EDGE_SET_PROCSTATE(from, NULL);

#if AF_COPY_ANALYSIS
  CFG_EDGE_SET_EQUATIONS(to, CFG_EDGE_EQUATIONS(from));
  CFG_EDGE_SET_EQUATIONS(from, NULL);
#endif

  /* corresponding */
  t_cfg_edge *corresponding_af_edge = CFG_EDGE_AF_CORR(from);
  CFG_EDGE_SET_AF_CORR(from, NULL);

  CFG_EDGE_SET_AF_CORR(to, corresponding_af_edge);
  CFG_EDGE_SET_AF_CORR(corresponding_af_edge, to);

  /* slice information */
  CFG_EDGE_SET_SLICE_INFORMATION(to, CFG_EDGE_SLICE_INFORMATION(from));
  CFG_EDGE_SET_SLICE_INFORMATION(from, NULL);
}

static
t_regset InsRegsLiveBefore (t_ins * ins, t_regset live_start)
{
  t_ins *i_ins;

  t_bbl *bbl = INS_BBL(ins);
  t_regset live = RegsetNew ();

  RegsetSetDup (live, live_start);

  BBL_FOREACH_INS_R(bbl, i_ins)
  {
    if (!INS_IS_CONDITIONAL(i_ins))
      RegsetSetDiff (live, INS_REGS_DEF (i_ins));
    RegsetSetUnion (live, INS_REGS_USE (i_ins));

    if (i_ins == ins) break;
  }
  if(!INS_IPREV(ins))
    RegsetSetDiff(live, BBL_REGS_NEVER_LIVE(bbl));

#ifdef DEBUG_LIVENESS
  if (!i_ins)
    FATAL(("ins is not in his own basic block!"));
#endif

  return live;
}

t_regset AFBblRegsLiveBeforeTail(t_cfg_edge *edge) {
  t_regset result = RegsetNew();

  if (CfgEdgeIsAF(edge)) {
    t_regset start_regs = AFBblRegsLiveBeforeTail(CFG_EDGE_AF_CORR(edge));
    result = InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(edge)), start_regs);
  }
  else {
    /* regular tail */
    result = BblRegsLiveBefore(CFG_EDGE_TAIL(edge));
  }

  return result;
}

t_regset AFFunctionLiveBefore(t_bbl *landing_site) {
  t_bbl *factored_bbl_tail = CFG_EDGE_HEAD(BBL_PRED_FIRST(landing_site));
  t_bbl *factored_bbl = FUNCTION_BBL_FIRST(BBL_FUNCTION(factored_bbl_tail));

  BBL_SET_REGS_LIVE_OUT(factored_bbl_tail, BblRegsLiveBefore(landing_site));
  t_regset live_before = BblRegsLiveBefore(factored_bbl_tail);

  /* propagate this liveness information inside the AF function if it consists of multiple BBLs */
  if (factored_bbl_tail != factored_bbl)
  {
    t_bbl *current_liveness_bbl = CFG_EDGE_HEAD(BBL_PRED_FIRST(factored_bbl_tail));
    do
    {
      ASSERT(FUNCTION_IS_AF(BBL_FUNCTION(current_liveness_bbl)), ("what? not in AF function @eiB", current_liveness_bbl));
      BBL_SET_REGS_LIVE_OUT(current_liveness_bbl, live_before);

      live_before = BblRegsLiveBefore(current_liveness_bbl);
      if (current_liveness_bbl == factored_bbl)
        break;

      current_liveness_bbl = CFG_EDGE_HEAD(BBL_PRED_FIRST(current_liveness_bbl));
    } while (true);
  }

  return live_before;
}

t_regset AFFunctionDefines(t_function *function) {
  t_regset result = RegsetNew();
  t_bbl *bbl;

  FUNCTION_FOREACH_BBL(function, bbl)
    RegsetSetUnion(result, BblRegsMaybeDef(bbl));

  return result;
}

static
BblSet ExitBlockSuccessorsRecursive(t_bbl *bbl, BblSet& already_done) {
  BblSet result;

  if (already_done.find(bbl) != already_done.end())
    return result;
  already_done.insert(bbl);

  t_cfg_edge *edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    t_bbl *tail = CFG_EDGE_TAIL(edge);

    if (BblIsExitBlock(tail)) {
      BblSet succs = ExitBlockSuccessorsRecursive(tail, already_done);
      result.insert(succs.begin(), succs.end());
    }
    else
      result.insert(tail);
  }

  return result;
}

BblSet ExitBlockSuccessors(t_bbl *exit_block) {
  BblSet already_done;
  return ExitBlockSuccessorsRecursive(exit_block, already_done);
}

static
BblSet ExitBlockPredecessorsRecursive(t_bbl *bbl, BblSet& already_done) {
  BblSet result;

  if (already_done.find(bbl) != already_done.end())
    return result;
  already_done.insert(bbl);

  t_cfg_edge *edge;
  BBL_FOREACH_PRED_EDGE(bbl, edge) {
    t_bbl *head = CFG_EDGE_HEAD(edge);

    if (BblIsExitBlock(head)) {
      BblSet preds = ExitBlockPredecessorsRecursive(head, already_done);
      result.insert(preds.begin(), preds.end());
    }
    else
      result.insert(head);
  }

  return result;
}

BblSet ExitBlockPredecessors(t_bbl *exit_block) {
  BblSet already_done;
  return ExitBlockPredecessorsRecursive(exit_block, already_done);
}

BblVector EdgeHead(t_cfg_edge *edge) {
  vector<t_bbl *> result;

  if (CfgEdgeIsAF(edge)) {
    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(edge)), e) {
      if (CFG_EDGE_AF_CORR(e) == edge)
        result.push_back(CFG_EDGE_HEAD(e));
    }
  }
  else
    result.push_back(CFG_EDGE_HEAD(edge));

  return result;
}

t_bbl *EdgeTail(t_cfg_edge *e) {
  t_bbl *result = CFG_EDGE_TAIL(e);

  if (CfgEdgeIsAF(e))
    result = CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(e));

  return result;
}
