#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

//#define DEBUG_NONZERO

static bool is_debug_nonzero = true;
void SetIsDebugNonzero(bool x) {
  is_debug_nonzero = x;
}

#ifdef DEBUG_NONZERO
#define DEBUG_NZ(x) if (is_debug_nonzero) DEBUG(x)
#else
#define DEBUG_NZ(x)
#endif

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(nonzeroregs_after);

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(nonzeroregs_before, NONZEROREGS_BEFORE, NonZeroRegsBefore, t_regset *, {*valp = NULL;}, {if (*valp != NULL) delete *valp;}, {*valp = NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(nonzeroregs_before);

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(nonzeroregs_beforei, NONZEROREGS_BEFOREI, NonZeroRegsBeforeI, t_regset *, {*valp = NULL;}, {if (*valp != NULL) delete *valp;}, {*valp = NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(nonzeroregs_beforei);

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(nonzeroregs_afteri, NONZEROREGS_AFTERI, NonZeroRegsAfterI, t_regset *, {*valp = NULL;}, {if (*valp != NULL) delete *valp;}, {*valp = NULL;});
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(nonzeroregs_afteri);

static t_regset all_registers;

#define GetRegsetMember(result, x, bbl) {\
  if (!BBL_ ## x (bbl)) result = NullRegs; \
  else result = *BBL_ ## x (bbl); \
}
#define SetRegsetMember(x, bbl, r) {\
  if (!BBL_ ## x (bbl)) BBL_SET_ ## x (bbl, new t_regset()); \
  *BBL_ ## x (bbl) = r; \
}

#define GetRegset(x, bbl) (BBL_ ## x (bbl)) ? (*BBL_ ## x (bbl)) : (NullRegs)

bool nonzero_init = false;
static
void IoModifierBblNonZero(t_bbl *bbl, t_string_array *array)
{
  if (!nonzero_init || !af_dynamic_member_init)
    return;

  t_string s1 = StringIo("NZB (@X)\\l", CPREGSET(BBL_CFG(bbl), BblNonZeroRegistersBeforeM(bbl)));
  t_string s2 = StringIo("NZA (@X)\\l", CPREGSET(BBL_CFG(bbl), BblNonZeroRegistersAfter(bbl)));

  StringArrayAppendString(array, StringConcat2(s1, s2));
  Free(s1);
  Free(s2);
}

static
t_reg InsGetNonZeroReg(t_arm_ins *ins)
{
  if ((ARM_INS_TYPE(ins) == IT_LOAD || ARM_INS_TYPE(ins) == IT_LOAD_MULTIPLE
            || ARM_INS_TYPE(ins) == IT_STORE || ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE)
          && ARM_INS_REGC(ins) == ARM_REG_NONE)
    return ARM_INS_REGB(ins);

  return ARM_REG_NONE;
}

static
bool FollowEdge(t_cfg_edge *edge)
{
  bool result = true;
  auto desc = CFG_DESCRIPTION(BBL_CFG(CFG_EDGE_TAIL(edge)));

  switch (CFG_EDGE_CAT(edge))
  {
  case ET_CALL:
  case ET_RETURN:
  case ET_COMPENSATING:
  case ET_SWI:
    result = false;
    break;

  default:
  {
    t_bbl *bbl = CFG_EDGE_TAIL(edge);
    if (BBL_FUNCTION(bbl)
        && bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(bbl)))
      /* this edge goes to a return block */
      result = false;
  }
  }

  return result;
}

static
t_reg NonZeroProducedInReg(t_arm_ins *ins, t_regset nonzero)
{
  //TODO: check that no relocation can overwrite the immediates here!
  if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER
      && ARM_INS_IMMEDIATE(ins) != 0)
    return ARM_INS_REGA(ins);

  if (ARM_INS_OPCODE(ins) == ARM_ADDRESS_PRODUCER)
    return ARM_INS_REGA(ins);

  if (ARM_INS_OPCODE(ins) == ARM_MOV
      && ARM_INS_FLAGS(ins) & FL_IMMED
      && ARM_INS_IMMEDIATE(ins) != 0)
    return ARM_INS_REGA(ins);

  if (ARM_INS_OPCODE(ins) == ARM_MOV
      && !(ARM_INS_FLAGS(ins) & FL_IMMED)
      && RegsetIn(nonzero, ARM_INS_REGC(ins)))
    return ARM_INS_REGA(ins);

  if (ARM_INS_OPCODE(ins) == ARM_MVN
      && ARM_INS_FLAGS(ins) & FL_IMMED
      && ARM_INS_IMMEDIATE(ins) != 0xffffffff)
    return ARM_INS_REGA(ins);

  if ((ARM_INS_OPCODE(ins) == ARM_MOVW || ARM_INS_OPCODE(ins) == ARM_MOVT)
      && ARM_INS_IMMEDIATE(ins) != 0)
    return ARM_INS_REGA(ins);

  if (ArmInsIsPush(ins)
      || ArmInsIsPop(ins))
    return ARM_REG_R13;

  return ARM_REG_NONE;
}

static
void PropagateNonzeroOverInstruction(t_arm_ins *ins, t_regset& result, bool forward = false) {
  if (ARM_INS_OPCODE(ins) == ARM_PSEUDO_SWAP) {
    /* swap instruction */
    t_reg rx = ARM_INS_REGA(ins);
    t_reg ry = ARM_INS_REGB(ins);

    bool rx_nonzero = RegsetIn(result, rx);
    bool ry_nonzero = RegsetIn(result, ry);

    RegsetSetSubReg(result, rx);
    RegsetSetSubReg(result, ry);

    if (rx_nonzero)
      RegsetSetAddReg(result, ry);
    if (ry_nonzero)
      RegsetSetAddReg(result, rx);
  }
  else {
    if (forward) {
      /* forward */
      t_regset defs = ARM_INS_REGS_DEF(ins);

      t_reg nonzero_reg = NonZeroProducedInReg(ins, result);
      if (nonzero_reg != ARM_REG_NONE) {
        if (!ArmInsIsConditional(ins)) {
          RegsetSetAddReg(result, nonzero_reg);
          RegsetSetSubReg(defs, nonzero_reg);
        }
      }
      else {
        nonzero_reg = InsGetNonZeroReg(ins);
        if (nonzero_reg != ARM_REG_NONE)
          if (!ArmInsIsConditional(ins))
            RegsetSetAddReg(result, nonzero_reg);
      }

      RegsetSetDiff(result, defs);
    }
    else {
      /* backward */
      bool move_nonzero = ARM_INS_OPCODE(ins) == ARM_MOV
                            && !(ARM_INS_FLAGS(ins) & FL_IMMED)
                            && RegsetIn(result, ARM_INS_REGA(ins));

      RegsetSetDiff(result, ARM_INS_REGS_DEF(ins));

      t_reg nonzero_reg = InsGetNonZeroReg(ins);
      if (nonzero_reg != ARM_REG_NONE
          && !ArmInsIsConditional(ins))
        RegsetSetAddReg(result, nonzero_reg);

      if (move_nonzero)
        RegsetSetAddReg(result, ARM_INS_REGC(ins));
    }
  }
}

t_regset BblNonZeroRegistersBefore(t_bbl *bbl, bool use_start_regs, t_regset start_regs, bool include_procstate /* default true */)
{
  t_regset result = BblNonZeroRegistersAfter(bbl);
  if (use_start_regs)
    result = start_regs;

  t_arm_ins *ins;
  BBL_FOREACH_ARM_INS_R(bbl, ins)
    PropagateNonzeroOverInstruction(ins, result);

  if (include_procstate && BBL_PROCSTATE_IN(bbl)) {
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      t_uint64 constant = 0;
      if (ProcstateGetConstantValue(BBL_PROCSTATE_IN(bbl), r, constant)
          && constant == 0)
        RegsetSetSubReg(result, r);
    }
  }

  return result;
}

static
t_regset SliceRegsNonZeroAfter(Slice *slice, size_t slice_size)
{
  t_regset result = BblNonZeroRegistersAfter(slice->Bbl());

  for (auto ins : IterInsAfter(slice, slice_size))
    PropagateNonzeroOverInstruction(T_ARM_INS(ins), result);

  return result;
}

t_regset SliceRegsNonZeroIn(Slice *slice, size_t slice_size)
{
  t_regset result = SliceRegsNonZeroAfter(slice, slice_size);

  for (auto ins : IterInsIn(slice, slice_size))
    PropagateNonzeroOverInstruction(T_ARM_INS(ins), result);

  return result;
}

t_regset BblNonZeroRegistersBeforeM(t_bbl *bbl)
{
  t_regset result; GetRegsetMember(result, NONZEROREGS_BEFORE, bbl);
  return result;
}

t_regset BblCalculateNonZeroRegistersAfter(t_bbl *bbl, t_regset start_regs)
{
  t_regset result = RegsetDup(start_regs);

  t_arm_ins *ins;
  BBL_FOREACH_ARM_INS(bbl, ins)
    PropagateNonzeroOverInstruction(ins, result, true);

  return result;
}

static
void BblRefreshNonZeroRegistersAfter(t_bbl *bbl)
{
  t_regset nonzero_after = BblCalculateNonZeroRegistersAfter(bbl);
  SetRegsetMember(NONZEROREGS_AFTER, bbl, nonzero_after);
}

void BblSetNonZeroRegistersAfter(t_bbl *bbl, t_regset regs)
{
  SetRegsetMember(NONZEROREGS_AFTER, bbl, regs);
}

void BblSetNonZeroRegistersBeforeM(t_bbl *bbl, t_regset regs)
{
  SetRegsetMember(NONZEROREGS_BEFORE, bbl, regs);
}

bool BblClearRegNonZeroBeforeM(t_bbl *bbl, t_reg reg)
{
  t_regset regs = BblNonZeroRegistersBeforeM(bbl);
  RegsetSetSubReg(regs, reg);
  BblSetNonZeroRegistersBeforeM(bbl, regs);

  return true;
}

bool BblSetRegNonZeroBeforeM(t_bbl *bbl, t_reg reg)
{
  t_regset regs = BblNonZeroRegistersBeforeM(bbl);
  RegsetSetAddReg(regs, reg);
  BblSetNonZeroRegistersBeforeM(bbl, regs);

  return true;
}

void BblCopyNonZeroRegistersAfter(t_bbl *from, t_bbl *to)
{
  SetRegsetMember(NONZEROREGS_AFTER, to, BblNonZeroRegistersAfter(from));
}

bool BblSetRegNonZeroAfter(t_bbl *bbl, t_reg reg) {
  t_regset regs = BblNonZeroRegistersAfter(bbl);
  bool result = !RegsetIn(regs, reg);
  RegsetSetAddReg(regs, reg);
  BblSetNonZeroRegistersAfter(bbl, regs);

  return result;
}

bool BblClearRegNonZeroAfter(t_bbl *bbl, t_reg reg) {
  t_regset regs = BblNonZeroRegistersAfter(bbl);
  bool result = RegsetIn(regs, reg);
  RegsetSetSubReg(regs, reg);
  BblSetNonZeroRegistersAfter(bbl, regs);

  return result;
}

static
t_regset AFGetNonzeroAfter(t_cfg_edge *edge) {
  /* assuming this edge is an outgoing AF edge */
  ASSERT(CfgEdgeIsAF(edge), ("edge is not AF! @E", edge));
  ASSERT(FUNCTION_IS_AF(BBL_FUNCTION(CFG_EDGE_HEAD(edge))), ("not an AF function @eiB @E", CFG_EDGE_HEAD(edge), edge));

  /* calculate new incoming nonzero registers */
  t_regset common_nonzero_in = CFG_DESCRIPTION(CFG_EDGE_CFG(edge))->all_registers;
  for (auto bbl : EdgeHead(edge))
    RegsetSetIntersect(common_nonzero_in, BblCalculateNonZeroRegistersAfter(bbl, BblNonZeroRegistersBeforeM(bbl)));

  /* propagate this information inside the AF function */
  t_bbl *bbl = FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_HEAD(edge)));

  bool cont = true;
  t_bbl *dispatch_block = NULL;
  while (cont) {
    common_nonzero_in = BblCalculateNonZeroRegistersAfter(bbl, common_nonzero_in);

    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(bbl, e) {
      bbl = CFG_EDGE_TAIL(e);

      if (BBL_FUNCTION(CFG_EDGE_HEAD(e)) != BBL_FUNCTION(CFG_EDGE_TAIL(e))) {
        dispatch_block = CFG_EDGE_HEAD(e);
        cont = false;
        break;
      }
    }
  }

  /* dispatch register */
  t_reg dispatch_reg = ARM_REG_NONE;
  t_uint64 dispatch_value = 0;
  t_arm_condition_code dispatch_condition;
  if (RegisterInfoForConditionalBranch(T_ARM_INS(BBL_INS_LAST(dispatch_block)),
                                       edge, dispatch_reg, dispatch_value, dispatch_condition))
  {
    if (dispatch_value == 0)
    {
      if (dispatch_condition == ARM_CONDITION_EQ)
        RegsetSetSubReg(common_nonzero_in, dispatch_reg);
      else if (dispatch_condition == ARM_CONDITION_NE)
        RegsetSetAddReg(common_nonzero_in, dispatch_reg);
      else
        FATAL(("implement me"));
    }
    else
    {
      if (dispatch_condition == ARM_CONDITION_EQ)
        RegsetSetAddReg(common_nonzero_in, dispatch_reg);
      else if (dispatch_condition == ARM_CONDITION_NE)
        RegsetSetSubReg(common_nonzero_in, dispatch_reg);
      else
        FATAL(("implement me"));
    }
  }

  return common_nonzero_in;
}

static
t_regset AFGetNonzeroBefore(t_cfg_edge *edge) {
  /* assuming this edge is an incoming AF edge */

  t_regset common_nonzero_out = BblNonZeroRegistersAfter(EdgeTail(edge));

  t_bbl *entry_bbl = FUNCTION_BBL_FIRST(BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
  ASSERT(FUNCTION_IS_AF(BBL_FUNCTION(entry_bbl)), ("what? this function is not AF! @F @eiB @E", BBL_FUNCTION(entry_bbl), entry_bbl, edge));

  /* find the last BBL */
  t_bbl *last_bbl = entry_bbl;
  while (true) {
    bool ip = false;
    int successor_count = 0;

    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(last_bbl, e) {
      successor_count++;
      ip |= CfgEdgeIsInterproc(e);
    }

    if (!ip) {
      ASSERT(successor_count == 1, ("expected only one outgoing edge @eiB", last_bbl));
      last_bbl = CFG_EDGE_TAIL(BBL_SUCC_FIRST(last_bbl));
    }
    else
      /* 'bbl' is now the last BBL in the function */
      break;
  }

  /* propagate the information backwards */
  t_regset result = common_nonzero_out;

  t_bbl *current_bbl = EdgeTail(edge);
  while (true) {
    result = BblNonZeroRegistersBefore(current_bbl, true, result, false);

    if (current_bbl == entry_bbl)
      break;

    current_bbl = CFG_EDGE_HEAD(BBL_PRED_FIRST(current_bbl));
  }

  return result;
}

t_regset NonzeroIncoming(t_bbl *bbl, BblSet& already_visited) {
  t_regset result = NullRegs;
  bool has_been_set = false;

  already_visited.insert(bbl);

  t_cfg_edge *back_edge = NULL;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e) {
    t_regset x;

    t_bbl *head = CFG_EDGE_HEAD(e);

    if (head == bbl) {
      back_edge = e;
      continue;
    }

    if (BblIsExitBlock(head)) {
      if (BBL_FUNCTION(head) && FUNCTION_IS_AF(BBL_FUNCTION(head))) {
        /* go to the next edge */
        continue;
      }
      if (already_visited.find(head) == already_visited.end()) {
        x = NonzeroIncoming(head, already_visited);
        DEBUG_NZ(("in(x) @X", CPREGSET(BBL_CFG(bbl), x)));

        if (CFG_EDGE_CAT(e) == ET_RETURN) {
          t_regset nonzero_after_callsite = BblNonZeroRegistersAfter(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)));
          DEBUG_NZ(("in(after call) @X", CPREGSET(BBL_CFG(bbl), nonzero_after_callsite)));

          t_regset saved = FUNCTION_REGS_SAVED(BBL_FUNCTION(head));
          DEBUG_NZ(("in(saved) @X", CPREGSET(BBL_CFG(bbl), saved)));
          RegsetSetDiff(x, saved);
          RegsetSetIntersect(saved, nonzero_after_callsite);

          t_regset unchanged = FUNCTION_REGS_CHANGED(BBL_FUNCTION(head));
          RegsetSetInvers(unchanged);
          DEBUG_NZ(("in(unchanged) @X", CPREGSET(BBL_CFG(bbl), unchanged)));
          RegsetSetDiff(x, unchanged);
          RegsetSetIntersect(unchanged, nonzero_after_callsite);

          RegsetSetUnion(x, saved);
          RegsetSetUnion(x, unchanged);
          DEBUG_NZ(("in(x) @X", CPREGSET(BBL_CFG(bbl), x)));

          if (FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(head))) {
            /* CALL HELL - callee may change some registers */
            RegsetSetDiff(x, CFG_DESCRIPTION(BBL_CFG(bbl))->callee_may_change);
          }
        }
        else {
          if (CFG_EDGE_CAT(e) == ET_COMPENSATING
              && FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(head)))
            RegsetSetDiff(x, CFG_DESCRIPTION(BBL_CFG(bbl))->callee_may_change);
        }
      }
    }
    else {
      if (CfgEdgeIsAF(e))
        x = AFGetNonzeroAfter(e);
      else {
        x = BblNonZeroRegistersAfter(head);

        /* if the predecessor ends in a conditional branch */
        t_arm_condition_code edge_condition;
        t_reg compare_register;
        t_uint64 compare_value;
        if (EdgeFromConditionalBranch(e, edge_condition, compare_register, compare_value)) {
          if (edge_condition == ARM_CONDITION_EQ) {
            if (compare_value == 0)
              RegsetSetSubReg(x, compare_register);
            else
              RegsetSetAddReg(x, compare_register);
          }
          else if (edge_condition == ARM_CONDITION_NE) {
            if (compare_value == 0)
              RegsetSetAddReg(x, compare_register);
          }
        }
      }
    }

    if (has_been_set)
      RegsetSetIntersect(result, x);
    else {
      result = x;
      has_been_set = true;
    }
  }


  if (back_edge) {
    /* a back edge was found */

    /* propagate the old results to the end of the block */
    t_regset new_after = BblCalculateNonZeroRegistersAfter(bbl, result);

    /* intersect it */
    result = RegsetIntersect(result, new_after);
  }

  return result;
}

t_regset NonzeroOutgoing(t_bbl *bbl, BblSet& already_visited) {
  t_regset result = NullRegs;
  bool has_been_set = false;

  already_visited.insert(bbl);

  t_cfg_edge *e;
  BBL_FOREACH_SUCC_EDGE(bbl, e) {
    t_regset x;

    t_bbl *tail = CFG_EDGE_TAIL(e);

    if (BblIsExitBlock(tail)) {
      if (already_visited.find(tail) == already_visited.end())
        x = NonzeroOutgoing(tail, already_visited);
    }
    else {
      if (CfgEdgeIsAF(e))
        x = AFGetNonzeroBefore(e);
      else {
        x = BblNonZeroRegistersBeforeM(tail);

        /* if the successor ends in a conditional branch */
        t_arm_condition_code edge_condition;
        t_reg compare_register;
        t_uint64 compare_value;
        if (EdgeFromConditionalBranch(e, edge_condition, compare_register, compare_value)) {
          if (edge_condition == ARM_CONDITION_EQ) {
            if (compare_value == 0)
              RegsetSetSubReg(x, compare_register);
            else
              RegsetSetAddReg(x, compare_register);
          }
          else if (edge_condition == ARM_CONDITION_NE) {
            if (compare_value == 0)
              RegsetSetAddReg(x, compare_register);
          }
        }
      }
    }

    if (has_been_set)
      RegsetSetIntersect(result, x);
    else {
      result = x;
      has_been_set = true;
    }
  }

  return result;
}

static
void AnalyseNonZeroRegistersFixpoint(t_cfg *cfg, vector<t_bbl *> worklist)
{
  t_bbl *bbl;

  BblMarkInit();
  NodeMarkInit();

  CFG_FOREACH_BBL(cfg, bbl)
    BblMark(bbl);

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun)
    FunctionMark(fun);

  t_function *last_popped_function = NULL;

  /* save memory */
  auto QueuePush = [] (t_bbl *bbl) {
    BblMark(bbl);
    FunctionMark(BBL_FUNCTION(bbl));
  };

  auto look_for_pop = [&last_popped_function] (t_function *fun_start) {
    for (t_function *fun = fun_start; fun != NULL; fun = FUNCTION_FNEXT(fun)) {
      if (!FunctionIsMarked(fun))
        continue;

      /* function is marked */
      t_bbl *bbl;
      FUNCTION_FOREACH_BBL(fun, bbl) {
        if (!BblIsMarked(bbl))
          continue;

        /* found */
        BblUnmark(bbl);
        last_popped_function = fun;
        return bbl;
      }

      FunctionUnmark(fun);
    }

    return (t_bbl *)NULL;
  };

  auto QueuePop = [&cfg, &look_for_pop, &last_popped_function] () {
    t_bbl *result = look_for_pop(last_popped_function);
    if (!result)
      result = look_for_pop(CFG_FUNCTION_FIRST(cfg));
    return result;
  };

  while (true)
  {
    t_cfg_edge *edge;

    /* fetch a new element to be processed */
    bbl = QueuePop();
    if (bbl == NULL)
      break;

    /* process this BBL */
    DEBUG_NZ(("doingNZ @iB", bbl));

    /* incoming non-zero registers */
    BblSet already_visited;
    t_regset incoming_nonzero = NonzeroIncoming(bbl, already_visited);
    DEBUG_NZ(("  incoming_nonzero @X", CPREGSET(cfg, incoming_nonzero)));

    RegsetSetUnion(incoming_nonzero, GetRegset(NONZEROREGS_BEFOREI, bbl));
    DEBUG_NZ(("  incoming_nonzero @X", CPREGSET(cfg, incoming_nonzero)));

    /* outgoing non-zero registers */
    already_visited.clear();
    t_regset outgoing_nonzero = NonzeroOutgoing(bbl, already_visited);
    DEBUG_NZ(("  outgoing_nonzero @X", CPREGSET(cfg, outgoing_nonzero)));

    RegsetSetUnion(outgoing_nonzero, GetRegset(NONZEROREGS_AFTERI, bbl));
    DEBUG_NZ(("  outgoing_nonzero @X", CPREGSET(cfg, outgoing_nonzero)));

    bool reprocess = false;

    t_regset old_before = BblNonZeroRegistersBeforeM(bbl);
    t_regset in_from_out = BblNonZeroRegistersBefore(bbl, true, outgoing_nonzero);
    t_regset new_in = RegsetUnion(in_from_out, incoming_nonzero);

    /* remove zero registers */
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      t_uint64 constant = 0;
      if (BBL_PROCSTATE_IN(bbl)) {
        if (ProcstateGetConstantValue(BBL_PROCSTATE_IN(bbl), r, constant)
            && constant == 0)
          RegsetSetSubReg(new_in, r);
      }
    }

    DEBUG_NZ(("  new_in @X", CPREGSET(cfg, new_in)));

    BblSetNonZeroRegistersBeforeM(bbl, new_in);
    if (!RegsetEquals(old_before, BblNonZeroRegistersBeforeM(bbl)))
      reprocess = true;

    t_regset old_after = BblNonZeroRegistersAfter(bbl);
    t_regset out_from_in = BblCalculateNonZeroRegistersAfter(bbl, incoming_nonzero);
    t_regset new_out = RegsetUnion(out_from_in, outgoing_nonzero);

    /* remove zero registers */
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      t_uint64 constant = 0;
      if (BBL_PROCSTATE_OUT(bbl)) {
        if (ProcstateGetConstantValue(BBL_PROCSTATE_OUT(bbl), r, constant)
            && constant == 0)
          RegsetSetSubReg(new_out, r);
      }
    }

    DEBUG_NZ(("  new_out @X", CPREGSET(cfg, new_out)));

    BblSetNonZeroRegistersAfter(bbl, new_out);
    if (!RegsetEquals(old_after, BblNonZeroRegistersAfter(bbl)))
      reprocess = true;

    if (reprocess) {
      BBL_FOREACH_PRED_EDGE(bbl, edge) {
        t_bbl *head = CFG_EDGE_HEAD(edge);

        if (BblIsExitBlock(head)) {
          for (auto p : ExitBlockPredecessors(head))
            QueuePush(p);
        }
        else {
          if (!FollowEdge(edge)) continue;
          QueuePush(CFG_EDGE_HEAD(edge));
        }
      }

      BBL_FOREACH_SUCC_EDGE(bbl, edge) {
        t_bbl *tail = CFG_EDGE_TAIL(edge);

        if (BblIsExitBlock(tail)) {
          for (auto s : ExitBlockSuccessors(tail))
            QueuePush(s);
        }
        else {
          if (!FollowEdge(edge)) continue;
          QueuePush(CFG_EDGE_TAIL(edge));
        }
      }
    }
  }

}

void AnalyseCfgForNonZeroRegisters(t_cfg *cfg)
{
  STATUS(START, ("Non-zero analysis"));

  all_registers = RegsetDup(CFG_DESCRIPTION(cfg)->int_registers);
  BblInitNonZeroRegsBefore(cfg);
  BblInitNonZeroRegsBeforeI(cfg);
  BblInitNonZeroRegsAfterI(cfg);
  vector<t_bbl *> worklist;

  DiabloBrokerCallInstall("IoModifierBblNonZero", "t_bbl *, t_string_array *", (void *)IoModifierBblNonZero, FALSE);
  nonzero_init = true;

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    BblRefreshNonZeroRegistersAfter(bbl);

    SetRegsetMember(NONZEROREGS_AFTERI, bbl, BblNonZeroRegistersAfter(bbl));
    SetRegsetMember(NONZEROREGS_BEFOREI, bbl, BblNonZeroRegistersBefore(bbl, false, NullRegs));

    worklist.push_back(bbl);
  }

  AnalyseNonZeroRegistersFixpoint(cfg, worklist);

  /* */
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_IS_HELL(bbl))
      continue;

    /* e.g., exit blocks */
    if (BBL_NINS(bbl) == 0)
      continue;

    /* e.g., data blocks */
    /* only one outgoing edge */
    if (!BBL_SUCC_FIRST(bbl)
        || CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) == NULL)
      continue;

    /* conditional jump */
    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(bbl, e) {
      t_bbl *tail = CFG_EDGE_TAIL(e);

      if (BBL_IS_HELL(tail)
          || BblIsExitBlock(tail))
        continue;

      t_regset nonzero_path = BblNonZeroRegistersBeforeM(tail);
      RegsetSetDiff(nonzero_path, BblNonZeroRegistersAfter(bbl));
    }
  }

#ifdef DEBUG_NONZERO
  DumpDots(cfg, "nonzero-analysis", 0);
#endif

  BblFiniNonZeroRegsBeforeI(cfg);
  BblFiniNonZeroRegsAfterI(cfg);

  STATUS(STOP, ("Non-zero analysis"));
}

void NonZeroAnalysisInit(t_cfg *cfg) {
  BblInitNonZeroRegsAfter(cfg);
}

void NonZeroAnalysisFini(t_cfg *cfg) {
  BblFiniNonZeroRegsAfter(cfg);
}
