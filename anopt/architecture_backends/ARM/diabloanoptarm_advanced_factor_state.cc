#include "diabloanoptarm_advanced_factor.hpp"

#include <unordered_map>

using namespace std;

static bool is_debug_state = true;
void SetIsDebugState(bool x) {
  is_debug_state = x;
}

#ifdef NONZERO_VERBOSE
#define DEBUG_CHANGEPROP_EXT
#define DEBUG_FASTPROP
#define DEBUG_NONZERO
#define DEBUG_CHANGEPROP
#endif

#ifdef DEBUG_FASTPROP
#define DEBUG_FP(x) do { if (is_debug_state) DEBUG(x); } while(0)
#else
#define DEBUG_FP(x)
#endif

#ifdef DEBUG_PRODPROP
#define DEBUG_PP(x) do { if (is_debug_state) DEBUG(x); } while(0);
#else
#define DEBUG_PP(x)
#endif

#ifdef DEBUG_NONZERO
#define DEBUG_NZ(x) do { if (is_debug_state) DEBUG(x); } while(0);
#else
#define DEBUG_NZ(x)
#endif

#ifdef DEBUG_CHANGEPROP
#define DEBUG_CP(x) do { if (is_debug_state) DEBUG(x); } while(0);
#else
#define DEBUG_CP(x)
#endif

BblSet modified_bbls;
BblSet propagation_sources;

typedef std::map<t_reg, ProducedValue> ProducedValueInRegMap;
typedef std::map<t_bbl *, ProducedValueInRegMap> ProducedValueInBblMap;
ProducedValueInBblMap constants_defined_in_bbls;
ProducedValueInBblMap liveness_in_bbls;
ProducedValueInBblMap inserted_producers;

RegisterAssumptionInBblMap assumptions;

typedef std::map<t_arm_ins *, RegisterValue> RegisterValueToInsMap;
RegisterValueToInsMap register_moves;

map<t_bbl *, bool> producer_after;
static map<t_bbl *, t_regset> save_already_done;

static bool liveness_include_start = false;

#define TLS_TEMPDISPATCHREG_NAME "tls_dispatchregbackup"
static bool tls_tempdispatchreg_created = false;

/* infinite loop detection */
static const t_uint32 INFINITE_LOOP_GUARD_1 = 1<<31;
static const t_uint32 INFINITE_LOOP_GUARD_2 = 1<<30;
static const t_uint32 INFINITE_LOOP_GUARD_MASK = INFINITE_LOOP_GUARD_1 | INFINITE_LOOP_GUARD_2;
static const t_uint32 INFINITE_LOOP_GUARD_MAX_COUNTER = 1024;
static t_uint32 current_infinite_loop_guard = INFINITE_LOOP_GUARD_1;

static inline
void ResetInfiniteLoopGuard() {
  current_infinite_loop_guard = (current_infinite_loop_guard == INFINITE_LOOP_GUARD_1) ? INFINITE_LOOP_GUARD_2 : INFINITE_LOOP_GUARD_1;
}

static inline
void InfiniteLoopGuardCheck(t_bbl *bbl) {
  if (BBL_IS_HELL(bbl)
      || BblIsExitBlock(bbl))
    return;

  if ((BBL_PRODPROP(bbl) & INFINITE_LOOP_GUARD_MASK) != current_infinite_loop_guard)
    BBL_SET_PRODPROP(bbl, current_infinite_loop_guard);
  else
    BBL_SET_PRODPROP(bbl, BBL_PRODPROP(bbl) + 1);

  /* check infinite loop */
  if ((BBL_PRODPROP(bbl) & ~INFINITE_LOOP_GUARD_MASK) == INFINITE_LOOP_GUARD_MAX_COUNTER) {
    if (bbl != FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl))) {
      CfgDrawFunctionGraphs(BBL_CFG(bbl), "infinite-loop");
      FATAL(("infinite loop! @iB", bbl));
    }
  }
}

string ScoringData::Print() {
  stringstream ss;

  /* print generic information */
  ss << "Minimal number of needed registers: " << min_regs_needed << endl;
  ss << "Number of registers available: " << nr_regs_available << endl;

  /* print source information */
  ss << "Covered archives (executed): " << source_info.archives.size() << " (" << source_info.exec_archives.size() << ")" << endl;
  ss << "Covered objects (executed): " << source_info.objects.size() << " (" << source_info.exec_objects.size() << ")" << endl;
  ss << "Covered functions (executed): " << source_info.functions.size() << " (" << source_info.exec_functions.size() << ")" << endl;

  return ss.str();
}

void RecordModifiedBbl(t_bbl *bbl)
{
  modified_bbls.insert(bbl);
}

void RecordProducingInstruction(t_bbl *bbl, t_reg reg, ProducedValue value, bool after) {
  if (value.ins == NULL)
    liveness_in_bbls[bbl][reg] = value;
  else {
    constants_defined_in_bbls[bbl][reg] = value;
    inserted_producers[bbl][reg] = value;
    producer_after[bbl] = after;

    if (value.ins) {
      t_arm_ins *ins = value.ins;
      t_function *fun = BBL_FUNCTION(bbl);
      if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER)
        RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
      else if (ARM_INS_OPCODE(ins) == ARM_MOV)
        RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
      else if (ARM_INS_OPCODE(ins) == ARM_LDR)
        RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
    }
  }
}

void RecordPropagationSource(t_bbl *bbl) {
  propagation_sources.insert(bbl);
}

void RecordRegisterAssumption(t_bbl *bbl, t_reg reg, RegisterAssumption assumption) {
  assumptions[bbl][reg] = assumption;
}

static
bool FollowEdge(t_cfg_edge *edge) {
  bool result = true;

  if (CFG_EDGE_CAT(edge) == ET_RETURN
      || CFG_EDGE_CAT(edge) == ET_COMPENSATING
      || CFG_EDGE_CAT(edge) == ET_SWI)
    result = false;
  else {
    switch (CFG_EDGE_CAT(edge)) {
      case ET_JUMP:
      case ET_IPJUMP:
      case ET_FALLTHROUGH:
      case ET_IPFALLTHRU:
      case ET_SWITCH:
      case ET_IPSWITCH:
      case ET_CALL:
        break;

      default:
        result = false;
        break;
    }
  }

  return result;
}

static
RegisterValue ProcstateGetRegisterValue(t_procstate *ps, t_reg reg) {
  t_uint64 constant = 0;

  if (ProcstateGetConstantValue(ps, reg, constant))
    return (constant == 0) ? RegisterValue::Zero : RegisterValue::Nonzero ;

  return RegisterValue::Unknown;
}

static
RegisterValue RegisterValueJoin(RegisterValue a, RegisterValue b) {
  if (a == RegisterValue::Unknown
      || b == RegisterValue::Unknown)
    return RegisterValue::Unknown;

  if (a == b)
    return a;

  return RegisterValue::Unknown;
}

static
void AFSetOutgoingConstant(t_cfg_edge *incoming_edge, t_reg reg, t_uint64 constant, bool bottom, bool propagate_constant_still) {
  t_architecture_description *desc = CFG_DESCRIPTION(CFG_EDGE_CFG(incoming_edge));

  TransformedSliceInformation *slice_information = CFG_EDGE_SLICE_INFORMATION(incoming_edge);
  t_gpregisters overwritten_registers = slice_information->overwritten_registers;
  t_all_register_info all_registers = slice_information->register_info;

  /* construct a new procstate for the outgoing edge */
  t_procstate *outgoing_procstate = ProcStateNewDup(BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(incoming_edge)));

  /* common nonzero registers for all inputs */

  /* NOTE that this needs to be '*BeforeM', because later on, the source of a register is
   * determined based on this point. Not after the compensation instructions. */
  t_regset common_nonzero = BblNonZeroRegistersBeforeM(CFG_EDGE_HEAD(incoming_edge));

  /* next, join all the procstates from the incoming edges
   * taking special care of the modified, incoming edge */
  t_cfg_edge *edge;
  BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(incoming_edge), edge) {
    /* we've already taken the incoming edge into account */
    if (edge == incoming_edge)
      continue;

    /* only need to look at the incoming edges that correspond to the same outgoing edge */
    if (CFG_EDGE_AF_CORR(edge) != CFG_EDGE_AF_CORR(incoming_edge))
      continue;

    RegsetSetIntersect(common_nonzero, BblNonZeroRegistersBeforeM(CFG_EDGE_HEAD(edge)));

    TransformedSliceInformation *slice_information_extra = CFG_EDGE_SLICE_INFORMATION(edge);
    ASSERT(slice_information_extra->overwritten_registers == overwritten_registers,
            ("overwritten registers not equal! %04x %04x", slice_information_extra->overwritten_registers, overwritten_registers));
    ProcStateJoinSimple(outgoing_procstate, BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(edge)), desc->all_registers, desc);
  }

  /* propagate the procstate over the factored BBL */
  t_bbl *target = CFG_EDGE_TAIL(incoming_edge);
  do {
    /* propagate information over target */
    if (!BBL_PROCSTATE_IN(target))
      BBL_SET_PROCSTATE_IN(target, ProcStateNew(desc));
    ProcStateDup(BBL_PROCSTATE_IN(target), outgoing_procstate, desc);
    BblPropagateConstantInformation(target, CONTEXT_SENSITIVE);
    ProcStateDup(outgoing_procstate, BBL_PROCSTATE_OUT(target), desc);

    if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(target)))
      break;

    target = CFG_EDGE_TAIL(BBL_SUCC_FIRST(target));
  } while (true);

  /* landing site */
  t_bbl *final_bbl = CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(incoming_edge));
  ASSERT(BBL_ATTRIB(final_bbl) & BBL_ADVANCED_FACTORING, ("expected block to be AF! @eiB", final_bbl));

  DEBUG_FP(("FASTPROP af procstate for @iB @C @C", target, desc, BBL_PROCSTATE_IN(target), desc, outgoing_procstate));

  ProcStateDup(CFG_EDGE_PROCSTATE(CFG_EDGE_AF_CORR(incoming_edge)), outgoing_procstate, desc);

  /* set the outgoing procstate */
  ProcStateFree(outgoing_procstate);

  /* calculate nonzero in, but without overwritten registers */
  t_regset new_after = RegsetDiffGPRegisters(BblNonZeroRegistersAfter(final_bbl), overwritten_registers);
  RegsetSetIntersect(new_after, common_nonzero);
  BblSetNonZeroRegistersAfter(final_bbl, new_after);

  /* dispatch register */
  t_reg dispatch_reg = ARM_REG_NONE;
  t_uint64 dispatch_value = 0;
  t_arm_condition_code dispatch_condition;
  if (RegisterInfoForConditionalBranch(T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(CFG_EDGE_AF_CORR(incoming_edge)))),
                                        CFG_EDGE_AF_CORR(incoming_edge), dispatch_reg, dispatch_value, dispatch_condition)) {
    if (dispatch_value == 0) {
      if (dispatch_condition == ARM_CONDITION_EQ)
        BblClearRegNonZeroAfter(final_bbl, dispatch_reg);
      else if (dispatch_condition == ARM_CONDITION_NE)
        BblSetRegNonZeroAfter(final_bbl, dispatch_reg);
      else
        FATAL(("implement me"));
    }
    else {
      if (dispatch_condition == ARM_CONDITION_EQ)
        BblSetRegNonZeroAfter(final_bbl, dispatch_reg);
      else if (dispatch_condition == ARM_CONDITION_NE)
        BblClearRegNonZeroAfter(final_bbl, dispatch_reg);
      else
        FATAL(("implement me"));
    }
  }

  /* update non-zero information */
  for (t_reg reg = ARM_REG_R0; reg < ARM_REG_R15; reg++)
  {
    if (!GPRegistersIn(overwritten_registers, reg))
      continue;

    if (!RegsetIn(BblNonZeroRegistersAfter(final_bbl), reg))
      continue;

    if (all_registers[reg] == REGISTER_INFO_UNMODIFIED)
      continue;

    t_reg new_register = all_registers[reg] & 0x0f;
    auto new_register_origin = (all_registers[reg] >> 4) & 0x0f;

    if (new_register_origin == REGISTER_ORIGIN_INPUT) {
      bool new_register_is_nonzero_before = RegsetIn(common_nonzero, new_register);
      bool old_register_is_nonzero_after = RegsetIn(BblNonZeroRegistersAfter(final_bbl), reg);

      if (new_register_is_nonzero_before ^ old_register_is_nonzero_after) {
        /* need to adapt the nonzero information */
        if (new_register_is_nonzero_before)
          BblSetRegNonZeroAfter(final_bbl, reg);
        else
          BblClearRegNonZeroAfter(final_bbl, reg);

        BblMark(final_bbl);
      }
    }
    else
      FATAL(("boem! %d r%d", new_register_origin, new_register));
  }
}

static
bool ProcStateRegIsBot(t_procstate *ps, t_reg reg) {
  t_register_content content;
  return ProcStateGetReg(ps, reg, &content) == CP_BOT;
}

typedef function<bool(t_bbl *, t_reg)> F_UpdateNonZero;
static
bool UpdateNonZeroBasedOnProcstate(t_bbl *bbl, t_procstate *ps, F_UpdateNonZero f_setnonzero, F_UpdateNonZero f_clearnonzero) {
  bool result = false;

  /* add any non-zero procstate constant values */
  for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
    t_uint64 constant;
    t_reloc *tag;

    /* maybe it is bottom */
    if (ProcstateGetConstantValue(ps, r, constant)
        && constant != 0) {
      result |= f_setnonzero(bbl, r);
    }
    else if (ProcstateGetTag(ps, r, tag)) {
      result |= f_setnonzero(bbl, r);
    }
  }

  return result;
}

static
void MergeIncomingInformation(t_bbl *bbl, t_reg reg, t_procstate *all_bot_procstate, t_procstate *new_incoming_procstate, bool& lookat_incoming_nonzero, bool& has_unfollowed, t_regset& incoming_nonzero, bool& set, RegisterValue& incoming_reg_value, bool& set_to_all_bot, BblSet& already_visited) {
  t_cfg_edge *edge;
  auto desc = CFG_DESCRIPTION(BBL_CFG(bbl));

  if (set_to_all_bot
      || has_unfollowed)
    return;

#ifdef DEBUG_CHANGEPROP_EXT
  bool debug = BBL_OLD_ADDRESS(bbl) == 0x51480;
#endif

  already_visited.insert(bbl);

  BBL_FOREACH_PRED_EDGE(bbl, edge) {
    t_procstate *ps = nullptr;

    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING
        /* in case an initial IP jump is present, for example in Init_Motion_Search_Module in h264ref */
        && FUNCTION_IS_AF(BBL_FUNCTION(CFG_EDGE_HEAD(edge))))
      continue;

    t_bbl *head = CFG_EDGE_HEAD(edge);
    if (BBL_IS_HELL(head)) {
      /* no information known at all */
      ProcStateDup(new_incoming_procstate, all_bot_procstate, desc);
      incoming_nonzero = NullRegs;
      lookat_incoming_nonzero = false;
      set_to_all_bot = true;
    }
    else if (BblIsExitBlock(head)) {
      if (already_visited.find(head) == already_visited.end())
        MergeIncomingInformation(head, reg, all_bot_procstate, new_incoming_procstate, lookat_incoming_nonzero, has_unfollowed, incoming_nonzero, set, incoming_reg_value, set_to_all_bot, already_visited);

      if (CFG_EDGE_CAT(edge) == ET_RETURN) {
        t_regset x = BblNonZeroRegistersAfter(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));

        t_regset y = FUNCTION_REGS_CHANGED(BBL_FUNCTION(head));
        RegsetSetInvers(y);
        RegsetSetUnion(y, FUNCTION_REGS_SAVED(BBL_FUNCTION(head)));

        RegsetSetIntersect(x, y);

        RegsetSetIntersect(incoming_nonzero, x);
      }

      lookat_incoming_nonzero = true;
    }
    else {
      /* look up incoming procstate for this edge */
      if (CfgEdgeIsAF(edge)) {
        ps = CFG_EDGE_PROCSTATE(edge);

        lookat_incoming_nonzero = false;
      }
      else {
        ASSERT(CFG_EDGE_CAT(edge) != ET_RETURN, ("should not come here @E", edge));

        if (!FollowEdge(edge)) {
          has_unfollowed = true;
        }
        else {
          ps = BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(edge));
          /* TODO merge in conditional procstate */
          if (!ps) ps = all_bot_procstate;

          RegsetSetIntersect(incoming_nonzero, BblNonZeroRegistersAfter(CFG_EDGE_HEAD(edge)));
        }
      }

      /* accumulate all the procstates, but only if the edge has been followed */
      if (!has_unfollowed) {
        if (!set) {
          ProcStateDup(new_incoming_procstate, ps, desc);
#ifdef DEBUG_CHANGEPROP_EXT
          if (debug)
            DEBUG_FP(("  SET @tC", CFG_DESCRIPTION(BBL_CFG(bbl)), ps));
#endif
          set = true;

          if (reg != ARM_REG_NONE)
            incoming_reg_value = ProcstateGetRegisterValue(ps, reg);
        }
        else {
#ifdef DEBUG_CHANGEPROP_EXT
          if (debug)
            DEBUG_FP(("  MERGING @tC", CFG_DESCRIPTION(BBL_CFG(bbl)), ps));
#endif

          ProcStateJoinSimple(new_incoming_procstate, ps, desc->all_registers, desc);

#ifdef DEBUG_CHANGEPROP_EXT
          if (debug)
            DEBUG_FP(("  RESULT @tC", CFG_DESCRIPTION(BBL_CFG(bbl)), new_incoming_procstate));
#endif

          if (reg != ARM_REG_NONE)
            incoming_reg_value = RegisterValueJoin(incoming_reg_value, ProcstateGetRegisterValue(ps, reg));
        }
      }
    }

    if (set_to_all_bot
        || has_unfollowed)
        break;
  }
}

bool recalculate_nonzero(t_bbl *bbl, bool also_do_backward) {
  t_cfg *cfg = BBL_CFG(bbl);
  bool changed = false;

  if (BBL_IS_HELL(bbl))
    return false;

  DEBUG_NZ(("STATE recalculate nonzero @eiB", bbl));

  /* calculate incoming non-zero registers */
  BblSet already_visited;
  t_regset nonzero_incoming = NonzeroIncoming(bbl, already_visited);
  DEBUG_NZ(("STATE   nonzero_incoming @X", CPREGSET(BBL_CFG(bbl), nonzero_incoming)));

  /* */
  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) == NULL) {
    /* only one incoming edge */
    t_cfg_edge *pred_edge = BBL_PRED_FIRST(bbl);
    t_bbl *pred_block = CFG_EDGE_HEAD(pred_edge);
    t_arm_ins *last_ins = T_ARM_INS(BBL_INS_LAST(pred_block));
    if (last_ins
        && ArmIsControlflow(last_ins)
        && ArmInsIsConditional(last_ins)
        && (ARM_INS_CONDITION(last_ins)==ARM_CONDITION_EQ || ARM_INS_CONDITION(last_ins)==ARM_CONDITION_NE)) {
      /* the only predecessor ends in a conditional branch */

      /* calculate the null registers */
      t_regset null_before = RegsetNew();

      if (BBL_PROCSTATE_IN(bbl)) {
        t_regset constant_before = RegsetNew();
        t_regset tag_before = RegsetNew();
        ProcstateConstantRegisters(BBL_PROCSTATE_IN(bbl), constant_before, null_before, tag_before);
      }

      /* take the intersection with the non-zero incoming registers */
      t_regset intersect = RegsetIntersect(nonzero_incoming, null_before);

      if (RegsetCountRegs(intersect) == 1) {
        t_reg null_reg;
        REGSET_FOREACH_REG(intersect, null_reg)
          break;
        WARNING(("removing r%d from non-zero incoming in @eiB @X", null_reg, bbl, CPREGSET(BBL_CFG(bbl), nonzero_incoming)));
        RegsetSetSubReg(nonzero_incoming, null_reg);
      }
      /*else {
        WARNING(("intersect contains multiple registers @eiB @X", bbl, CPREGSET(BBL_CFG(bbl), intersect)));
      }*/
    }
  }

  /* add stack pointer if needed */
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e) {
    if (CFG_EDGE_CAT(e) == ET_RETURN
        && RegsetIn(BblNonZeroRegistersBeforeM(bbl), ARM_REG_R13))
      RegsetSetAddReg(nonzero_incoming, ARM_REG_R13);
  }

  /* non-zero registers for this BBL on entry */
  t_regset nonzero_before = BblNonZeroRegistersBefore(bbl, true, NullRegs);

  t_regset before = RegsetUnion(nonzero_before, nonzero_incoming);
  DEBUG_NZ(("STATE   before @X", CPREGSET(BBL_CFG(bbl), before)));

  /* calculate new AFTER */
  t_regset after = BblCalculateNonZeroRegistersAfter(bbl, before);
  DEBUG_NZ(("STATE   after @X", CPREGSET(BBL_CFG(bbl), after)));

  /* need to set the incoming nonzero registers always, because this BBL may be the end point! */
#ifdef DEBUG_NONZERO
  bool before_differs = !RegsetEquals(BblNonZeroRegistersBeforeM(bbl), before);
#endif
  BblSetNonZeroRegistersBeforeM(bbl, before);

  if (!RegsetEquals(BblNonZeroRegistersAfter(bbl), after)) {
    /* difference detected, need to propagate further still! */
    BblSetNonZeroRegistersAfter(bbl, after);

    DEBUG_NZ(("STATE CHANGE @eiB", bbl));

    changed = true;
  }
#ifdef DEBUG_NONZERO
  else if (before_differs) {
    DEBUG_NZ(("STATE CHANGE-before @eiB", bbl));
  }
#endif

  return changed;
}

/* propagate a produced constant forward */
static
void PropagateProducedConstant(t_procstate *running_procstate, t_arm_ins *ins, t_reg reg, t_uint64 constant, bool bottom, BblSet only_forward_nonzero, t_bbl *start_bbl = NULL) {
  t_cfg_edge *edge;
  bool propagating_reg = (reg != ARM_REG_NONE);
  BblSet nonzero_changed_list;

  if (diabloanoptarm_options.af_no_constprop)
    return;

  /* we'll mark BBLs that had some data changed (thus should be propagated further */
  BblMarkInit();

  /* starting point */
  if (!start_bbl) {
    ASSERT(ins, ("no start block and no instruction given"));
    start_bbl = ARM_INS_BBL(ins);
  }
  else if (ins)
    ASSERT(ARM_INS_BBL(ins) == start_bbl, ("@I not in start block @iB", ins, start_bbl));

  t_cfg *cfg = BBL_CFG(start_bbl);

  /* for correct non-zero updates */
  if (bottom)
    constant = 0;

  if (ins)
    DEBUG_FP(("FASTPROP producing @I: r%d = %lld (%s)", ins, reg, constant, (bottom ? "BOT" : "CONSTANT")));
  else
    DEBUG_FP(("FASTPROP producing (null) in @iB: r%d = %lld (%s)", start_bbl, reg, constant, (bottom ? "BOT" : "CONSTANT")));

  ResetInfiniteLoopGuard();

  t_architecture_description *desc = CFG_DESCRIPTION(BBL_CFG(start_bbl));

  if (propagating_reg) {
    /* set this register to have a constant value at the end of the start BBL */
    ProcstateSetReg(BBL_PROCSTATE_OUT(start_bbl), reg, constant, bottom);

    if (constant == 0)
      BblClearRegNonZeroAfter(start_bbl, reg);
    else
      BblSetRegNonZeroAfter(start_bbl, reg);
  }

  /* we're working with a worklist algorithm here */
  struct PropagationData {
    t_bbl *bbl;
    bool bottom;
    bool constant;
  };
  vector<PropagationData> worklist = {};

  /* create the initial work list:
   * populate it with the immediate successors of the start BBL */
  BBL_FOREACH_SUCC_EDGE(start_bbl, edge) {
    /* take into account special factoring edges */
    bool propagate_bottom = false;

    if (CfgEdgeIsAF(edge)) {
      propagate_bottom = propagating_reg ? RegsetIn(FUNCTION_REGS_OVERWRITTEN(BBL_FUNCTION(CFG_EDGE_TAIL(edge))), reg) : false;
      AFSetOutgoingConstant(edge, reg, constant, bottom || propagate_bottom, propagating_reg);
    }

    /* stop propagation if necessary */
    if (!FollowEdge(edge))
      continue;

    worklist.push_back(PropagationData{EdgeTail(edge), propagate_bottom || bottom, propagating_reg});
    DEBUG_FP(("FASTPROP start push %d @eiB", propagate_bottom || bottom, worklist.back()));

    if (CFG_EDGE_CAT(edge) == ET_CALL
        && BBL_CALL_HELL_TYPE(CFG_EDGE_TAIL(edge)) == BBL_CH_DYNCALL
        && CFG_EDGE_CORR(edge)) {
      worklist.push_back(PropagationData{CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)), propagate_bottom || bottom, propagating_reg});
      DEBUG_FP(("FASTPROP start push %d @eiB", propagate_bottom || bottom, worklist.back()));
    }
  }

  t_procstate *new_incoming_procstate = ProcStateNew(desc);
  t_procstate *all_bot_procstate = ProcStateNew(desc);
  ProcStateSetAllBot(all_bot_procstate, RegsetNewInvers(desc->registers_prop_over_hell, desc->all_registers));

  while (worklist.size() > 0) {
    PropagationData propagation_data = worklist.back();
    worklist.pop_back();

    t_bbl *subject = propagation_data.bbl;
    bool bottom = propagation_data.bottom;
    bool propagate_constant_still = propagation_data.constant;

    /* this can be triggered by e.g. CALLs to tail functions, and
     * where the call-fallthrough path ends in DATA */
    if (IS_DATABBL(subject))
      continue;

    DEBUG_FP(("FASTPROP doing %d r%d @iB", bottom, reg, subject));
    InfiniteLoopGuardCheck(subject);

    bool set = false;

    /* if the BBL has an incoming AF edge, it is assumed to be the only one */
    /* we need to join the non-zero incoming registers e.g. for function entry points
     * that have multiple incoming CALL edges */
    RegisterValue incoming_reg_value = RegisterValue::Unknown;

    /* inoming nonzero */
    bool lookat_incoming_nonzero = true;
    t_regset incoming_nonzero = desc->int_registers;

    /* in case at least one of the incoming edges has not been followed,
     * we don't want to modify the constant information data.
     * This is especially the case where the BBL has an incoming RETURN edge
     * from which we don't know (at least in this simplified constant propagation analysis)
     * which registers are modified and which are kept. */
    bool has_unfollowed = false;

    bool set_to_all_bot = false;

    BblSet already_visited;
    MergeIncomingInformation(subject, reg, all_bot_procstate, new_incoming_procstate, lookat_incoming_nonzero, has_unfollowed, incoming_nonzero, set, incoming_reg_value, set_to_all_bot, already_visited);

    /* see comment above */
    if (has_unfollowed)
      continue;

    /* old values */
    t_regset old_nz_before = BblNonZeroRegistersBeforeM(subject);
    t_regset old_nz_after = BblNonZeroRegistersAfter(subject);

    /* merge in conditional edge procstate if possible */
    t_reg conditional_register = ARM_REG_NONE;
    bool conditional_register_nonzero = false;
    if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(subject)) == NULL)
      MergeConditionalProcstate(BBL_PRED_FIRST(subject), new_incoming_procstate, conditional_register, conditional_register_nonzero);

    /* merge in return information */
    bool following_return = false;
    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(subject, e) {
      /* RETURN, look at corresponding CALL */
      if (CFG_EDGE_CAT(e) != ET_RETURN)
        continue;

      t_function *callee = BBL_FUNCTION(CFG_EDGE_HEAD(e));

      /* TODO this check should be removed safely */
      if (callee != BBL_FUNCTION(start_bbl)) {
        /* join the procstates, but only of the unchanged registers */
        t_procstate *ps = BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)));
        if (!ps) ps = all_bot_procstate;

        t_regset unchanged = FUNCTION_REGS_CHANGED(callee);
        RegsetSetInvers(unchanged);
        RegsetSetUnion(unchanged, FUNCTION_REGS_SAVED(callee));
        RegsetSetIntersect(unchanged, desc->int_registers);
        ProcStateJoinSimple(new_incoming_procstate, ps, unchanged, desc);

        if (reg != ARM_REG_NONE
            && RegsetIn(unchanged, reg))
          incoming_reg_value = RegisterValueJoin(incoming_reg_value, ProcstateGetRegisterValue(ps, reg));

        following_return = true;
      }
    }

    bool procstate_changed = !BBL_PROCSTATE_IN(subject) || !ProcStateEquals(BBL_PROCSTATE_IN(subject), new_incoming_procstate, desc);
    bool nonzero_changed = false;

    t_regset subject_defines_maybe = BblRegsMaybeDef(subject);
    if (propagate_constant_still) {
      if (reg != ARM_REG_NONE) {
        /* update non-zero information */
        switch (incoming_reg_value) {
        case RegisterValue::Zero:
        case RegisterValue::Unknown:
          /* first calculate which registers MUST be nonzero before */
          if ((!RegsetIn(BblNonZeroRegistersBefore(subject, false, NullRegs), reg) || following_return)
              && !RegsetIn(incoming_nonzero, reg)) {
            DEBUG_NZ(("STATE clear nonzero before r%d", reg));
            BblClearRegNonZeroBeforeM(subject, reg);

            if (!RegsetIn(subject_defines_maybe, reg)) {
              DEBUG_NZ(("STATE clear nonzero after r%d", reg));
              nonzero_changed |= BblClearRegNonZeroAfter(subject, reg);
            }
          }
          break;
        case RegisterValue::Nonzero:
          if (!bottom) {
            DEBUG_NZ(("STATE set nonzero before r%d", reg));
            BblSetRegNonZeroBeforeM(subject, reg);

            if (!RegsetIn(subject_defines_maybe, reg)) {
              DEBUG_NZ(("STATE set nonzero after r%d", reg));
              nonzero_changed |= BblSetRegNonZeroAfter(subject, reg);
            }
          }
          break;

        default: FATAL(("unhandled value"));
        }
      }
    }

    if (conditional_register != ARM_REG_NONE
        && conditional_register_nonzero) {
      /* conditional register defined */
      DEBUG_NZ(("STATE set nonzero conditional before r%d", conditional_register));
      BblSetRegNonZeroBeforeM(subject, conditional_register);

      if (!RegsetIn(subject_defines_maybe, conditional_register)) {
        DEBUG_NZ(("STATE set nonzero conditional after r%d", conditional_register));
        nonzero_changed |= BblSetRegNonZeroAfter(subject, conditional_register);
      }
    }

    /* 'subject' will be marked if the non-zero-after register set is changed */
    nonzero_changed |= BblIsMarked(subject);
    BblUnmark(subject);

    /* maybe now the register is bottom? */
    if (reg != ARM_REG_NONE)
      bottom = ProcStateRegIsBot(new_incoming_procstate, reg);

    bool recalculated = false;
    if (lookat_incoming_nonzero
        && !RegsetEquals(BblNonZeroRegistersBeforeM(subject), incoming_nonzero)) {
#ifdef DEBUG_NONZERO
      DEBUG(("need to recalculate\n@X\n@X", CPREGSET(cfg, BblNonZeroRegistersBeforeM(subject)), CPREGSET(cfg, incoming_nonzero)));
#endif
      /* TODO: need to know why not bidirectional here */
      nonzero_changed |= recalculate_nonzero(subject, false);
      recalculated = true;
    }

    if (!recalculated) {
      /* previous block is compensation block? */
      bool prev_is_compensation = false;

      t_bbl *prev_bbl = CFG_EDGE_HEAD(BBL_PRED_FIRST(subject));
          /* incoming fallthrough */
      if (CFG_EDGE_CAT(BBL_PRED_FIRST(subject)) == ET_FALLTHROUGH
          /* one incoming edge */
          && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(subject)) == NULL
          /* pred has incoming AF edge */
          && CfgEdgeIsAF(BBL_PRED_FIRST(prev_bbl)))
        nonzero_changed |= recalculate_nonzero(subject, false);
    }

    /* special case */
    bool must_do_succs = false;
    BBL_FOREACH_PRED_EDGE(subject, edge)
    {
      if (CfgEdgeIsAF(edge))
      {
        must_do_succs = true;
        break;
      }
    }

    if (!procstate_changed && !nonzero_changed && !must_do_succs)
      /* shortcut in case nothing has changed */
      continue;

    if (procstate_changed) {
      DEBUG_FP(("FASTPROP procstate change! (nonzero? %d)", nonzero_changed));
      if (BBL_PROCSTATE_IN(subject))
        DEBUG_FP(("FASTPROP   old-in @tC", CFG_DESCRIPTION(BBL_CFG(subject)), BBL_PROCSTATE_IN(subject)));
      else
        DEBUG_FP(("FASTPROP   old-in NULL"));
      DEBUG_FP(("FASTPROP   new-in @tC", CFG_DESCRIPTION(BBL_CFG(subject)), new_incoming_procstate));

      /* set if not exist */
      if (!BBL_PROCSTATE_IN(subject))
        BBL_SET_PROCSTATE_IN(subject, ProcStateNew(desc));

      /* update BBL_PROCSTATE_IN */
      ProcStateDup(BBL_PROCSTATE_IN(subject), new_incoming_procstate, desc);
      if (BBL_PROCSTATE_OUT(subject))
        DEBUG_FP(("FASTPROP   old-out @tC", CFG_DESCRIPTION(BBL_CFG(subject)), BBL_PROCSTATE_OUT(subject)));
      else
        DEBUG_FP(("FASTPROP   old-out NULL"));

      /* update BBL_PROCSTATE_OUT */
      BblPropagateConstantInformation(subject, CONTEXT_SENSITIVE);
      DEBUG_FP(("FASTPROP   new-out @tC", CFG_DESCRIPTION(BBL_CFG(subject)), BBL_PROCSTATE_OUT(subject)));

      /* update non-zero information based on procstate information, but only ADD registers! */
      UpdateNonZeroBasedOnProcstate(subject, BBL_PROCSTATE_IN(subject), BblSetRegNonZeroBeforeM, BblClearRegNonZeroBeforeM);
    }

    nonzero_changed |= UpdateNonZeroBasedOnProcstate(subject, BBL_PROCSTATE_OUT(subject), BblSetRegNonZeroAfter, BblClearRegNonZeroAfter);

    if (nonzero_changed) {
      DEBUG_FP(("FASTPROP nonzero change for @iB (defs: @X)! @X", subject, CPREGSET(BBL_CFG(subject), subject_defines_maybe), CPREGSET(BBL_CFG(subject), BblNonZeroRegistersAfter(subject))));
      nonzero_changed_list.insert(subject);
    }

    bool propagate_constant_still_new = true;

    /* do we need to process the successors? */
    if (reg != ARM_REG_NONE
        && RegsetIn(subject_defines_maybe, reg)
        && subject != start_bbl) {
      propagate_constant_still_new = false;

      ASSERT(BBL_SUCC_FIRST(subject), ("no successor! @iB", subject));
      t_bbl *tail = CFG_EDGE_TAIL(BBL_SUCC_FIRST(subject));
      if (ProcStateRegIsBot(BBL_PROCSTATE_OUT(subject), reg)
          && CFG_EDGE_CAT(BBL_SUCC_FIRST(subject)) == ET_FALLTHROUGH
          && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(tail)) == NULL
          && BBL_PROCSTATE_IN(tail)
          && !ProcStateRegIsBot(BBL_PROCSTATE_IN(tail), reg)) {
        propagate_constant_still_new = true;
      }
    }

    /* recalculate nonzero changed: compare with initial values
     * sometimes the bool variables are set to true when no fixpoint is reached yet in the above calculations
     * and ultimately, at the end of the loop (right before this code) it could be that the end result is that
     * no change in non-zero registers has been propagated */
    if (nonzero_changed
        && RegsetEquals(BblNonZeroRegistersBeforeM(subject), old_nz_before)
        && RegsetEquals(BblNonZeroRegistersAfter(subject), old_nz_after)) {
      nonzero_changed = false;
      propagate_constant_still_new = false;
    }

    if (!propagate_constant_still_new
        && !nonzero_changed
        && !procstate_changed
        && !must_do_succs)
      continue;

    DEBUG_FP(("FASTPROP   %d %d %d %d", propagate_constant_still_new, nonzero_changed, procstate_changed, must_do_succs));

    /* process the successors if needed */
    BBL_FOREACH_SUCC_EDGE(subject, edge) {
      bool propagate_bottom = false;
      if (CfgEdgeIsAF(edge)) {
        propagate_bottom = propagating_reg ? RegsetIn(FUNCTION_REGS_OVERWRITTEN(BBL_FUNCTION(CFG_EDGE_TAIL(edge))), reg) : false;
        AFSetOutgoingConstant(edge, reg, constant, bottom || propagate_bottom, propagate_constant_still_new && propagate_constant_still);
      }

      /* stop propagation if necessary */
      if (!FollowEdge(edge))
        continue;

      /* need to look at return point too */
      if (CFG_EDGE_CAT(edge) == ET_CALL
          && CFG_EDGE_CORR(edge)) {
        worklist.push_back(PropagationData{CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)), propagate_bottom || bottom, propagate_constant_still && propagate_constant_still});
        DEBUG_FP(("FASTPROP push back CALL %d @eiB", propagate_bottom || bottom, worklist.back()));
      }

      t_bbl *tail = EdgeTail(edge);
      if (BblIsExitBlock(tail)) {
        t_cfg_edge *e;
        BBL_FOREACH_SUCC_EDGE(tail, e) {
          if (CFG_EDGE_CAT(e) == ET_RETURN) {
            worklist.push_back(PropagationData{CFG_EDGE_TAIL(e), propagate_bottom || bottom, propagate_constant_still_new && propagate_constant_still});
            DEBUG_FP(("FASTPROP push back RETURN %d @eiB", propagate_bottom || bottom, worklist.back()));
          }
        }
      }
      else {
        worklist.push_back(PropagationData{tail, propagate_bottom || bottom, propagate_constant_still_new && propagate_constant_still});
        DEBUG_FP(("FASTPROP push back %d @eiB", propagate_bottom || bottom, worklist.back()));
      }
    }
  }

  ProcStateFree(new_incoming_procstate);
  ProcStateFree(all_bot_procstate);

  /* propagate nonzero information */
  {
    BblMarkInit();

    vector<t_bbl *> worklist;
    worklist.insert(worklist.begin(), nonzero_changed_list.begin(), nonzero_changed_list.end());

    recalculate_nonzero(start_bbl, false);
    worklist.push_back(start_bbl);

    while (worklist.size() > 0) {
      t_bbl *bbl = worklist.back();
      t_cfg *cfg = BBL_CFG(bbl);
      worklist.pop_back();

      if (BblIsMarked(bbl))
        continue;

      BblMark(bbl);
      DEBUG_FP(("FASTPROP doing nonzero @eiB", bbl));

      /* the AF compensation block information has been updated in advance.
       * As such, a change here will not be propagated. For conservativeness,
       * we should thus always propagate to the next block. */
      bool must_do_succs = false;
      t_cfg_edge *e;
      BBL_FOREACH_PRED_EDGE(bbl, e) {
        if (!CfgEdgeIsAF(e))
          continue;

        must_do_succs = true;
        break;
      }

      /* 'bbl' had its nonzero-after set changed.
       * Iterate over the successors, and adapt the nonzero-before correctly. */
      t_cfg_edge *succ;
      BBL_FOREACH_SUCC_EDGE(bbl, succ) {
        t_bbl *modify = EdgeTail(succ);

        // bool also_do_backward = true;
        // if (only_forward_nonzero.find(modify) != only_forward_nonzero.end())
        //   also_do_backward = false;
        bool changed = recalculate_nonzero(modify, false);

        if (changed || must_do_succs) {
          if (modify != bbl) {
            /* don't do loops */
            BblUnmark(modify);
            worklist.push_back(modify);
          }
        }
      }
    }
  }
}

static
t_regset AFFunctionUses(t_function *function) {
  return BBL_REGS_USE(FUNCTION_BBL_FIRST(function));
}

static
bool PropagateProducedRegister(t_arm_ins *ins, t_reg reg) {
  t_bbl *start_bbl = ARM_INS_BBL(ins);
  bool need_to_propagate = true;
  bool result = false;

  DEBUG_FP(("PRODPROP propagating produced register r%d in @I in @iB", reg, ins, start_bbl));

  t_arm_ins *tmp = ARM_INS_IPREV(ins);
  while (tmp) {
    if (RegsetIn(ARM_INS_REGS_USE(tmp), reg)
        || RegsetIn(ARM_INS_REGS_DEF(tmp), reg)) {
      need_to_propagate = false;
      break;
    }

    tmp = ARM_INS_IPREV(tmp);
  }

  /* early exit if the register is defined INSIDE the BBL */
  if (!need_to_propagate)
    return result;

  /* if we get here, we need to propagate to the predecessors */
  BblMarkInit();

  BblVector worklist = {start_bbl};

  while (worklist.size() > 0) {
    t_bbl *subject = worklist.back();
    worklist.pop_back();

    if (BblIsMarked(subject))
      continue;

    DEBUG_FP(("PRODPROP doing @eiB", subject));

    BblMark(subject);
    result = true;

    /* remove the register from the live-out set */
    if (subject != start_bbl) {
      t_regset regs = BBL_REGS_LIVE_OUT(subject);
      RegsetSetSubReg(regs, reg);
      BBL_SET_REGS_LIVE_OUT(subject, regs);
      DEBUG_FP(("PRODPROP removed r%d from regset @eiB", reg, subject));
    }

    t_cfg_edge *edge;
    BBL_FOREACH_PRED_EDGE(subject, edge) {
      switch (CFG_EDGE_CAT(edge)) {
      case ET_FALLTHROUGH:
      case ET_IPFALLTHRU:
      case ET_JUMP:
      case ET_IPJUMP:
      case ET_CALL:
      case ET_SWITCH:
      case ET_IPSWITCH:
      case ET_RETURN:
      case ET_COMPENSATING:
        break;

      default: FATAL(("edge! @E @eiB @eiB", edge, CFG_EDGE_HEAD(edge), CFG_EDGE_TAIL(edge)));
      }

      t_regset livebefore = RegsetNew();
      bool edge_is_af = CfgEdgeIsAF(edge);
      if (edge_is_af) {
        /* no need to propagate */
        if (RegsetIn(AFFunctionUses(BBL_FUNCTION(CFG_EDGE_HEAD(edge))), reg))
          continue;

        /* calculate the registers */
        livebefore = AFFunctionLiveBefore(subject);
      }
      else {
        livebefore = BblRegsLiveBefore(subject);
      }

      for (auto new_bbl : EdgeHead(edge)) {
        t_regset other_live = RegsetNew();

        t_cfg_edge *e;
        BBL_FOREACH_SUCC_EDGE(new_bbl, e) {
          t_bbl *tail = EdgeTail(e);

          /* only need to look at other BBLs */
          if (tail == subject)
            continue;

          if (CfgEdgeIsAF(e)) {
            /* AF edge */
            RegsetSetUnion(other_live, AFFunctionLiveBefore(tail));
          }
          else {
            /* regular case */
            RegsetSetUnion(other_live, BblRegsLiveBefore(tail));
          }
        }

        RegsetSetUnion(other_live, livebefore);

        if (!RegsetIn(other_live, reg)
            && RegsetIn(BBL_REGS_LIVE_OUT(new_bbl), reg)) {
          DEBUG_FP(("PRODPROP need to remove r%d from live-out @iB", reg, new_bbl));
          worklist.push_back(new_bbl);
        }
      }
    }
  }

  return result;
}

static map<t_function *, t_regset> additional_changed;
void RecordChangedRegisterInFunction(t_function *fun, t_reg reg) {
  if (additional_changed.find(fun) == additional_changed.end())
    additional_changed[fun] = RegsetNew();

  RegsetSetAddReg(additional_changed[fun], reg);
};

static
void PropagateFunctionChangedRegisters() {
  auto OutgoingFunctions = [] (t_function *fun) {
    FunctionSet result;
    t_cfg_edge *e;

    BBL_FOREACH_PRED_EDGE(FunctionGetExitBlock(fun), e) {
      t_function *outgoing = BBL_FUNCTION(CFG_EDGE_HEAD(e));

      if (CFG_EDGE_CAT(e) != ET_COMPENSATING
          || FUNCTION_IS_AF(outgoing)
          || FUNCTION_IS_HELL(outgoing))
        continue;

      result.insert(outgoing);
    }

    return result;
  };

  auto IncomingFunctions = [] (t_function *fun, BblVector& return_sites) {
    FunctionSet result;
    t_cfg_edge *e;

    DEBUG_CP(("CHANGEPROP @F, entry @eiB", fun, FUNCTION_BBL_FIRST(fun)));
    BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun), e) {
      t_function *incoming = BBL_FUNCTION(CFG_EDGE_HEAD(e));

      if (!CfgEdgeIsInterproc(e)
          || FUNCTION_IS_AF(incoming)
          || FUNCTION_IS_HELL(incoming))
        continue;

      DEBUG_CP(("  incoming @F", incoming));
      result.insert(incoming);

      if (CFG_EDGE_CAT(e) == ET_CALL
          && CFG_EDGE_CORR(e)) {
        DEBUG_CP(("  return @B", CFG_EDGE_TAIL(CFG_EDGE_CORR(e))));
        return_sites.push_back(CFG_EDGE_TAIL(CFG_EDGE_CORR(e)));
      }
    }

    return result;
  };

  struct AdditionalChanged {
    t_function *fun;
    t_regset regs;
  };

  for (auto pair : additional_changed) {
    t_function *fun = pair.first;
    t_regset regs = pair.second;

    /* worklist algorithm */
    vector<AdditionalChanged> worklist;

    worklist.push_back(AdditionalChanged{fun, regs});
    DEBUG_CP(("CHANGEPROP(init) @F: +@X", fun, CPREGSET(FUNCTION_CFG(fun), regs)));
    while (worklist.size() > 0) {
      AdditionalChanged subject = worklist.back();
      worklist.pop_back();
      DEBUG_CP(("CHANGEPROP looking at @F", subject.fun));

      /* calculate registers to propagate to incoming functions */
      t_regset old_changed = FUNCTION_REGS_CHANGED(subject.fun);
      DEBUG_CP(("  changed @X", CPREGSET(FUNCTION_CFG(subject.fun), old_changed)));
      t_regset old_saved = FUNCTION_REGS_SAVED(subject.fun);
      DEBUG_CP(("  saved @X", CPREGSET(FUNCTION_CFG(subject.fun), old_saved)));

      /* changed but not explicitely saved */
      t_regset changed_not_expl_saved = RegsetDiff(subject.regs, FunctionExplicitelySavedRegs(subject.fun));
      DEBUG_CP(("  changed not expl saved @X", CPREGSET(FUNCTION_CFG(subject.fun), changed_not_expl_saved)));
      RegsetSetDiff(changed_not_expl_saved, CFG_DESCRIPTION(FUNCTION_CFG(subject.fun))->callee_may_change);
      FUNCTION_SET_REGS_SAVED(subject.fun, RegsetDiff(FUNCTION_REGS_SAVED(subject.fun), changed_not_expl_saved));

      t_regset new_changed = RegsetUnion(old_changed, RegsetDiff(subject.regs, FUNCTION_REGS_SAVED(subject.fun)));
      FUNCTION_SET_REGS_CHANGED(subject.fun, new_changed);
      DEBUG_CP(("  new changed @X", CPREGSET(FUNCTION_CFG(subject.fun), new_changed)));

      if (!RegsetEquals(old_changed, new_changed)) {
        /* propagate changes */
        t_regset delta = RegsetDiff(new_changed, old_changed);
        DEBUG_CP(("  delta @X", CPREGSET(FUNCTION_CFG(subject.fun), delta)));

        /* remove changed registers from nonzero regsets of jump-to-return bbls in this function */
        t_bbl *exit_block = FunctionGetExitBlock(subject.fun);
        if (exit_block) {
          t_cfg_edge *e;
          BBL_FOREACH_PRED_EDGE(exit_block, e) {
            if (CfgEdgeIsInterproc(e))
              continue;

            t_bbl *bbl = CFG_EDGE_HEAD(e);

            /* */
            DEBUG_CP(("CHANGEPROP remove from jump-to-exit @eiB", bbl));

            t_reg reg;
            REGSET_FOREACH_REG(delta, reg) {
              BblClearRegNonZeroBeforeM(bbl, reg);
              BblClearRegNonZeroAfter(bbl, reg);
            }
          }
        }

        BblVector return_sites;
        for (auto fun : IncomingFunctions(subject.fun, return_sites)) {
          DEBUG_CP(("CHANGEPROP(loop) @F: +@X", fun, CPREGSET(FUNCTION_CFG(fun), delta)));
          worklist.push_back(AdditionalChanged{fun, delta});
        }

        BblSet only_forward_nonzero;
        for (auto bbl : return_sites) {
          DEBUG_CP(("CHANGEPROP(registers) @iB @X", bbl, CPREGSET(FUNCTION_CFG(fun), delta)));
          t_reg reg;
          REGSET_FOREACH_REG(delta, reg)
            PropagateProducedConstant(BBL_PROCSTATE_IN(bbl), T_ARM_INS(BBL_INS_FIRST(bbl)), reg, 0, true, only_forward_nonzero, bbl);
        }
      }
    }
  }

  additional_changed.clear();
}

void FastProducerPropagation(t_cfg *cfg, bool liveness, BblSet only_forward_nonzero) {
  auto& worklist = (liveness) ? liveness_in_bbls : constants_defined_in_bbls;
  BblMarkInit2();

  map<t_function *, t_regset> additional_changed;

  auto bbl_defines_reg_after_ins = [] (t_arm_ins *ins, t_reg reg) {
    t_arm_ins *tmp = ARM_INS_INEXT(ins);
    while (tmp) {
      if (reg != ARM_REG_NONE
          && RegsetIn(ARM_INS_REGS_DEF(tmp), reg))
        return true;

      tmp = ARM_INS_INEXT(tmp);
    }

    return false;
  };

  /* create an address-insensitive list of Bbls.
   * We need to do this to get reproducable behaviour across multiple Diablo runs. */
  BblSet ordered_worklist;
  for (auto pair1 : worklist)
    ordered_worklist.insert(pair1.first);

  /* Another criterium is that we need to FIRST propagate constant information for the predecessor BBLs,
   * and SECOND propagate constant information for the successor BBLs. This is because the constant information
   * present in the BBL_PROCSTATE_IN still contains the information as correct prior to the transformation.
   * However, if, e.g., a non-null constant is produced in Rx in a predecessor, and Rx contained 0, then
   * the BBL_PROCSTATE_IN for the successor will have Rx marked as, which of course is incorrect. */
  vector<t_bbl *> befores, afters;
  for (auto bbl : ordered_worklist)
    if (producer_after[bbl])
      afters.push_back(bbl);
    else
      befores.push_back(bbl);
  befores.insert(befores.end(), afters.begin(), afters.end());

  PropagateFunctionChangedRegisters();

  for (auto bbl : befores) {
    t_procstate *bbl_running_procstate = NULL;
    t_arm_ins *ins;

    if (!liveness) {
      /* reset instruction marks for this BBL */
      BBL_FOREACH_ARM_INS(bbl, ins)
        INS_SET_MARK(T_INS(ins), false);

      /* keep some BBL-internal procstate information while propagating constant information */
      bbl_running_procstate = ProcStateNewDup(BBL_PROCSTATE_IN(bbl));
      BblPropagateConstantInformation(bbl, CONTEXT_SENSITIVE);
    }

    /* clear the bbl from the propagation source BBL list */
    propagation_sources.erase(bbl);

    /* we need to process the producing instructions in the BBL in-order.
     * here, we first iterate over all the producer instructions and mark them for further processing. */
    for (auto pair2 : worklist[bbl]) {
      t_reg reg = pair2.first;
      ProducedValue pval = pair2.second;

      if (!pval.ins)
        PropagateLiveRegister(bbl, NULL, reg);
      else
        INS_SET_MARK(T_INS(pval.ins), true);
    }

    if (!liveness) {
      /* when 'liveness' is true, no instructions will have been marked,
       * so also no propagation needs to be done */

      BBL_FOREACH_ARM_INS(bbl, ins) {
        t_function *fun = BBL_FUNCTION(bbl);

        /* only process producer instructions */
        if (!INS_MARK(T_INS(ins))) {
          InsPropagateConstants(bbl_running_procstate, T_INS(ins));
          continue;
        }

        /* process this instruction */
        DEBUG_FP(("FASTPROP producer: @I in @iB IN @C\nOUT @C", ins, bbl, CFG_DESCRIPTION(cfg), BBL_PROCSTATE_IN(bbl), CFG_DESCRIPTION(cfg), BBL_PROCSTATE_OUT(bbl)));

        auto alias_registers = [&bbl_running_procstate, &bbl_defines_reg_after_ins, &only_forward_nonzero] (t_arm_ins *ins, t_reg dst, t_reg src, t_uint64& constant_value) {
          bool result;

          if (ProcstateGetConstantValue(bbl_running_procstate, src, constant_value)) {
            /* the source register is constant */
            if (!bbl_defines_reg_after_ins(ins, dst))
              PropagateProducedConstant(bbl_running_procstate, ins, dst, constant_value, false, only_forward_nonzero);

            result = false;
          }
          else {
            /* the source register is not constant */
            PropagateProducedConstant(bbl_running_procstate, ins, dst, constant_value, true, only_forward_nonzero);

            result = true;
          }

          PropagateProducedRegister(ins, dst);

          return result;
        };

        if (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER) {
          //if (!bbl_defines_reg_after_ins(ins, ARM_INS_REGA(ins)))
            PropagateProducedConstant(bbl_running_procstate, ins, ARM_INS_REGA(ins), ARM_INS_IMMEDIATE(ins), false, only_forward_nonzero);
          //else
          //  FATAL(("bbl defines r%d after @I: @iB", ARM_INS_REGA(ins), ins, bbl));

          /* keep the current procstate up to date */
          ProcstateSetReg(bbl_running_procstate, ARM_INS_REGA(ins), ARM_INS_IMMEDIATE(ins), false);
          PropagateProducedRegister(ins, ARM_INS_REGA(ins));

          /* changed register */
          RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
        }
        else if (ARM_INS_OPCODE(ins) == ARM_MOV
                  && !(ARM_INS_FLAGS(ins) & FL_IMMED)) {
          /* a move-register instruction */
          t_uint64 constant_value = 0;
          bool foo = alias_registers(ins, ARM_INS_REGA(ins), ARM_INS_REGC(ins), constant_value);
          ProcstateSetReg(bbl_running_procstate, ARM_INS_REGA(ins), constant_value, foo);

          PropagateLiveRegister(bbl, ins, ARM_INS_REGC(ins));

          /* changed register */
          RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
        }
        else if (ARM_INS_OPCODE(ins) == ARM_LDR
                  || ARM_INS_OPCODE(ins) == ARM_MRC) {
          /* load, e.g. from TLS-load for register restore operations */
          /* MRC to get the thread ID */
          PropagateProducedConstant(bbl_running_procstate, ins, ARM_INS_REGA(ins), 0, true, only_forward_nonzero);
          ProcstateSetReg(bbl_running_procstate, ARM_INS_REGA(ins), 0, true);
          PropagateProducedRegister(ins, ARM_INS_REGA(ins));
          RecordChangedRegisterInFunction(fun, ARM_INS_REGA(ins));
        }
        else if (ARM_INS_OPCODE(ins) == ARM_PSEUDO_SWAP) {
          /* swap */
          t_uint64 constant_value1 = 0;
          bool foo = alias_registers(ins, ARM_INS_REGA(ins), ARM_INS_REGB(ins), constant_value1);

          t_uint64 constant_value2 = 0;
          bool bar = alias_registers(ins, ARM_INS_REGB(ins), ARM_INS_REGA(ins), constant_value2);

          ProcstateSetReg(bbl_running_procstate, ARM_INS_REGA(ins), constant_value1, foo);
          ProcstateSetReg(bbl_running_procstate, ARM_INS_REGB(ins), constant_value2, bar);
        }
        else
          FATAL(("unhandled producer @I", ins));
      }
    }

    if (bbl_running_procstate)
      ProcStateFree(bbl_running_procstate);
  }

  for (auto bbl : propagation_sources) {
    t_procstate *procstate = ProcStateNewDup(BBL_PROCSTATE_IN(bbl));
    BblPropagateConstantInformation(bbl, CONTEXT_SENSITIVE);
    PropagateProducedConstant(procstate, T_ARM_INS(BBL_INS_FIRST(bbl)), ARM_REG_NONE, 0, false, only_forward_nonzero);
    ProcStateFree(procstate);
  }
  propagation_sources.clear();

  worklist.clear();
}

struct BblRegTuple {
  t_bbl *bbl;
  t_reg reg;
};

static
BblSet restoration_bbls;

static
AddedInstructionInfo FixupRegister(t_bbl *bbl, t_reg reg, RegisterAssumption assumption, vector<BblRegTuple>& bbl_reg_tuples_to_remove) {
  AddedInstructionInfo result = AddedInstructionInfo();

  t_cfg *cfg = BBL_CFG(bbl);

  auto mark_already_done = [] (t_bbl *bbl, t_reg reg) {
    if (save_already_done.find(bbl) == save_already_done.end())
      save_already_done[bbl] = RegsetNew();
    RegsetSetAddReg(save_already_done[bbl], reg);
  };
  auto is_already_done = [] (t_bbl *bbl, t_reg reg) {
    if (save_already_done.find(bbl) == save_already_done.end())
      return false;

    if (RegsetIn(save_already_done[bbl], reg))
      return true;

    return false;
  };

  if (RegsetIn(BblRegsLiveAfter(bbl), reg)) {
    /* store the register in a TLS variable to not overwrite any other register */

    /* first create a TLS variable slot if needed */
    if (!tls_tempdispatchreg_created) {
      TlsCreate(CFG_OBJECT(cfg), TLS_TEMPDISPATCHREG_NAME);
      tls_tempdispatchreg_created = true;
    }

    t_bbl *save_bbl = NULL;
    t_bbl *restore_bbl = NULL;

    auto get_af_edge = [] (t_bbl *bbl) {
      t_cfg_edge *result = BBL_SUCC_FIRST(bbl);

      int counter = 0;
      while (!CfgEdgeIsAF(result)) {
        ASSERT(counter <= 3, ("too many BBLs in between, study this case or increase max counter! @eiB", bbl));

        t_bbl *tmp = CFG_EDGE_TAIL(result);
        ASSERT(BBL_SUCC_FIRST(tmp), ("no outgoing edges? @eiB", tmp));
        ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(tmp)) == NULL, ("expected only one outgoing edge @eiB @eiB", bbl, tmp));
        result = BBL_SUCC_FIRST(tmp);
        counter++;
      }

      return result;
    };

    for (auto p1 : inserted_producers) {
      t_bbl *prodbbl = p1.first;

      /* no compensation needs to be done for the producers we created here */
      if (restoration_bbls.find(prodbbl) != restoration_bbls.end())
        continue;

      /* also, exclude any block that does not go to an AF function */
      if (!CfgEdgeIsAF(BBL_SUCC_FIRST(prodbbl)))
        continue;

      for (auto p2 : p1.second) {
        t_reg overwritten_reg = p2.first;

        if (overwritten_reg == reg) {
          /* 'bbl' overwrites 'reg' */
          save_bbl = prodbbl;
        }
      }
    }

    /* if 'save' is NULL, perhaps no producer was found */
    if (save_bbl == NULL)
      save_bbl = bbl;

    /* sanity checks */
    if (save_bbl == NULL) {
      DumpDots(cfg, "save", 0);
      FATAL(("no BBL found in which to save the register! @eiB", bbl));
    }

    /* look for the restore BBL */
    t_cfg_edge *incoming_af = get_af_edge(save_bbl);
    if (!CfgEdgeIsAF(incoming_af)) {
      DumpDots(cfg, "fatal", 0);
      FATAL(("expected @eiB to have a following AF block!", save_bbl));
    }

    t_cfg_edge *outgoing_af = CFG_EDGE_AF_CORR(incoming_af);
    restore_bbl = CFG_EDGE_TAIL(outgoing_af);

    /* look for possible additional save locations */
    set<t_bbl *> save_locations;
    save_locations.insert(save_bbl);

    t_cfg_edge *edge;
    BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(incoming_af), edge) {
      /* don't look at other paths */
      if (CFG_EDGE_AF_CORR(edge) != outgoing_af)
        continue;

      /* look at the head */
      t_bbl *head = CFG_EDGE_HEAD(edge);

      if (BBL_NINS(head) == 0) {
        DEBUG(("empty head @eiB", head));

        t_cfg_edge *e;
        BBL_FOREACH_PRED_EDGE(head, e)
          save_locations.insert(CFG_EDGE_HEAD(e));
      }
      else
        save_locations.insert(head);
    }

    /* need to save, actually? */
    bool no_def = true;
    for (auto save_location : save_locations) {
      if (RegsetIn(BblRegsMaybeDef(save_location), reg))
        no_def = false;
    }

    if (no_def) {
      /* no instructions found defining this register,
       * this is a consequence of the conservativeness of the non-zero analysis */
      bbl_reg_tuples_to_remove.push_back(BblRegTuple{bbl, reg});
      return result;
    }

    /* save the register in each of the selected locations */
    for (auto save_bbl : save_locations) {
      if (is_already_done(save_bbl, reg)) {
        bbl_reg_tuples_to_remove.push_back(BblRegTuple{bbl, reg});
        continue;
      }

      t_reg tmp = ARM_REG_NONE;
      t_regset dead_before = BblRegsLiveBefore(save_bbl);
      RegsetSetInvers(dead_before);
      REGSET_FOREACH_REG(dead_before, tmp) {
        if (tmp == reg) continue;
        break;
      }

      t_arm_ins *tls_pos = NULL;
      if (!(tmp < ARM_REG_R15 && tmp != ARM_REG_NONE)) {
        DEBUG(("assuming valid stack pointer!", save_bbl));

        /* TODO: random register */
        tmp = ARM_REG_R0;
        if (reg == tmp)
          tmp++;

        /* assume a valid stack pointer */
        t_arm_ins *push_ins = AddInstructionToBblI(save_bbl, NULL, false);
        ArmInsMakePush(push_ins, 1<<tmp, ARM_CONDITION_AL, false);
        AFFactoringLogInstruction(push_ins, "BACKUP");
        result.AddInstruction(T_INS(push_ins));

        tls_pos = push_ins;

        t_arm_ins *pop_ins = AddInstructionToBblI(save_bbl, push_ins, true);
        ArmInsMakePop(pop_ins, 1<<tmp, ARM_CONDITION_AL, false);
        AFFactoringLogInstruction(pop_ins, "BACKUP");
        result.AddInstruction(T_INS(pop_ins));
      }

      VERBOSE(AF_VERBOSITY_LEVEL, (AF "saving r%d in @eiB to a TLS variable (temporary register r%d)", reg, save_bbl, tmp));
      ASSERT(reg != tmp, ("what? temporary register should be different r%d/r%d\nsaving @eiB\nrestoring @eiB", reg, tmp, save_bbl, restore_bbl));
      t_arm_ins *tls_store_ins = T_ARM_INS(TlsStore(save_bbl, T_INS(tls_pos), reg, tmp, TLS_TEMPDISPATCHREG_NAME));
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "       result @eiB", save_bbl));
      AFFactoringLogInstruction(tls_store_ins, "BACKUP");
      result.AddInstruction(T_INS(tls_store_ins), 2);

      t_arm_ins *tls_store_mrc = ARM_INS_IPREV(tls_store_ins);
      ASSERT(tls_store_mrc && ARM_INS_OPCODE(tls_store_mrc) == ARM_MRC, ("expected MRC instruction before TLS-store in @eiB", save_bbl));
      AFFactoringLogInstruction(tls_store_mrc, "BACKUP");

      ProducedValue pval;
      pval.ins = tls_store_mrc;
      RecordProducingInstruction(save_bbl, tmp, pval, false);

      mark_already_done(save_bbl, reg);
    }

    if (!is_already_done(restore_bbl, reg)) {
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "restoring r%d in @eiB from a TLS variable", reg, restore_bbl));
      t_arm_ins *load_instruction = T_ARM_INS(TlsLoad(restore_bbl, NULL, reg, TLS_TEMPDISPATCHREG_NAME));
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "       result @eiB", restore_bbl));
      AFFactoringLogInstruction(load_instruction);
      result.AddInstruction(T_INS(load_instruction), 2);

      t_arm_ins *mrc_instruction = ARM_INS_IPREV(load_instruction);
      ASSERT(mrc_instruction && ARM_INS_OPCODE(mrc_instruction) == ARM_MRC, ("expected MRC instruction before TLS-load in @eiB", restore_bbl));
      AFFactoringLogInstruction(mrc_instruction);

      bbl_reg_tuples_to_remove.push_back(BblRegTuple{bbl, reg});

      ProducedValue pval;
      pval.ins = load_instruction;
      RecordProducingInstruction(restore_bbl, reg, pval, true);
      restoration_bbls.insert(restore_bbl);

      mark_already_done(restore_bbl, reg);
    }
  }
  else {
    VERBOSE(AF_VERBOSITY_LEVEL, (AF "producing r%d = %lld", reg, assumption.constant));
    t_arm_ins *x = ProduceConstantInBbl(bbl, reg, assumption.constant, false, true);
    result.AddInstruction(T_INS(x));
  }

  return result;
}

static
bool RegisterIsNonzeroBefore(t_bbl *bbl, t_reg reg) {
  t_uint64 constant;
  t_reloc *tag;

  return (ProcstateGetConstantValue(BBL_PROCSTATE_IN(bbl), reg, constant) && constant != 0)
          || (ProcstateGetTag(BBL_PROCSTATE_IN(bbl), reg, tag))
          || RegsetIn(BblNonZeroRegistersBeforeM(bbl), reg);
}

static
bool RegisterIsNonzeroAfter(t_bbl *bbl, t_reg reg) {
  t_uint64 constant;
  t_reloc *tag;

  return RegsetIn(BblNonZeroRegistersAfter(bbl), reg)
          || (ProcstateGetConstantValue(BBL_PROCSTATE_OUT(bbl), reg, constant) && constant != 0)
          || (ProcstateGetTag(BBL_PROCSTATE_OUT(bbl), reg, tag));
}

static
bool RegisterIsZeroAfter(t_bbl *bbl, t_reg reg) {
  t_uint64 constant;

  return ProcstateGetConstantValue(BBL_PROCSTATE_OUT(bbl), reg, constant) && constant == 0;
}

AddedInstructionInfo CheckAndFixAssumptions(t_cfg *cfg, bool fix) {
  AddedInstructionInfo result = AddedInstructionInfo();
  vector<BblRegTuple> bbl_reg_tuples_to_remove;
  restoration_bbls.clear();

  save_already_done.clear();

  for (auto pair1 : assumptions) {
    t_bbl *bbl = pair1.first;

    for (auto pair2 : pair1.second) {
      t_reg reg = pair2.first;
      RegisterAssumption assumption = pair2.second;

      switch (assumption.value) {
      case RegisterValue::Unknown:
        break;

      case RegisterValue::Zero:
        if (RegisterIsZeroAfter(bbl, reg)) {
          /* ok */
        }
        else {
          /* not ok */
          if (fix) {
            WARNING(("assumption does not hold anymore! r%d == 0 after @iB, inserting instruction!", reg, bbl));
            auto res = FixupRegister(bbl, reg, assumption, bbl_reg_tuples_to_remove);
            result.Merge(res);
          }
          else {
            DumpDots(cfg, "boe", 0);
            FATAL(("assumption does not hold anymore! r%d == 0 after @iB", reg, bbl));
          }
        }
        break;

      case RegisterValue::Nonzero:
        if (RegisterIsNonzeroAfter(bbl, reg)) {
          /* ok */
        }
        else {
          /* not ok */
          if (fix) {
            /* fix it */
            WARNING(("assumption does not hold anymore! r%d != 0 after @iB, inserting instruction!", reg, bbl));
            auto res = FixupRegister(bbl, reg, assumption, bbl_reg_tuples_to_remove);
            result.Merge(res);
          }
          else {
            /* terminate */
            DumpDots(cfg, "boe", 1);
            FATAL(("assumption does not hold anymore! r%d != 0 after @iB", reg, bbl));
          }
        }
        break;

      default: FATAL(("unhandled assumption value type"));
      }
    }
  }

  /* remove selected tuples from the assumptions list */
  while (bbl_reg_tuples_to_remove.size() > 0) {
    BblRegTuple tuple = bbl_reg_tuples_to_remove.back();
    bbl_reg_tuples_to_remove.pop_back();

    assumptions[tuple.bbl].erase(tuple.reg);
    if (assumptions[tuple.bbl].empty())
      assumptions.erase(tuple.bbl);
  }

  return result;
}

void ClearAssumptions() {
  assumptions.clear();
  inserted_producers.clear();
}

void ClearBeforeAfters() {
  producer_after.clear();
}

void RecordRegisterMoveConstant(t_arm_ins *ins, RegisterValue value) {
  register_moves[ins] = value;
}

void ClearRegisterMoves() {
  register_moves.clear();
}

void SetLivePropIncludeStart(bool b) {
  liveness_include_start = b;
}

void CheckRegisterMoves() {
  for (auto pair : register_moves) {
    t_arm_ins *ins = pair.first;
    RegisterValue value = pair.second;

    t_reg dst_reg = ARM_INS_REGA(ins);
    t_reg src_reg = ARM_INS_REGC(ins);

    t_uint64 constant;
    if (value == RegisterValue::Zero) {
      if (ProcstateGetConstantValue(BBL_PROCSTATE_IN(ARM_INS_BBL(ins)), dst_reg, constant)
          && constant == 0) {
        VERBOSE(0, (AF "move unnecessary! @I in @eiB", ins, ARM_INS_BBL(ins)));

        /* remove the instruction and execute liveness analysis for this register! */
        AFFactoringLogInstruction(ins, "MOVECHECK_Z0_REMOVE");
        ArmInsMakeConstantProducer(ins, dst_reg, 0);
        AFFactoringLogInstruction(ins, "MOVECHECK_Z0");
      }
      else if (!ProcstateGetConstantValue(BBL_PROCSTATE_IN(ARM_INS_BBL(ins)), src_reg, constant)
                || constant != 0) {
        /* make it a constant producer */
        VERBOSE(0, (AF "making constant producer of @I in @eiB to produce 0", ins, ARM_INS_BBL(ins)));

        AFFactoringLogInstruction(ins, "MOVECHECK_Z1_REMOVE");
        ArmInsMakeConstantProducer(ins, dst_reg, 0);
        AFFactoringLogInstruction(ins, "MOVECHECK_Z1");

        VERBOSE(0, (AF "   made constant producer @I", ins));
      }
    }
    else if (value == RegisterValue::Nonzero) {
      if (ProcstateGetConstantValue(BBL_PROCSTATE_IN(ARM_INS_BBL(ins)), dst_reg, constant)
          && constant != 0) {
        VERBOSE(0, (AF "move unnecessary! @I in @eiB", ins, ARM_INS_BBL(ins)));

        AFFactoringLogInstruction(ins, "MOVECHECK_NZ1_REMOVE");
        ArmInsMakeConstantProducer(ins, dst_reg, constant);
        AFFactoringLogInstruction(ins, "MOVECHECK_NZ1");
      }
      else if (!RegisterIsNonzeroBefore(ARM_INS_BBL(ins), src_reg)) {
        /* make it a constant producer */
        VERBOSE(0, (AF "making constant producer of @I in @eiB to produce 1", ins, ARM_INS_BBL(ins)));

        AFFactoringLogInstruction(ins, "MOVECHECK_NZ0_REMOVE");
        ArmInsMakeConstantProducer(ins, dst_reg, 1);
        AFFactoringLogInstruction(ins, "MOVECHECK_NZ0");

        VERBOSE(0, (AF "   made constant producer @I", ins));
      }
    }
  }
}
