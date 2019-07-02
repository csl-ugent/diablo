#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

#define DEBUG_ICONDJUMP

static
bool AreStatusRegistersDead(vector<FactoredPath>& factored_paths, BblSet& destination_bbls)
{
  for (auto path : factored_paths)
  {
    destination_bbls.insert(CFG_EDGE_TAIL(path.outgoing_edge));

    /* status flags should be dead */
    if (!RegsetIsEmpty(RegsetIntersect(BblRegsLiveAfter(CFG_EDGE_HEAD(path.outgoing_edge)), status_registers)))
      return false;
  }

  return true;
}

typedef map<t_bbl *, t_uint32> BblToIndexMap;

static
void AssignIndexesToPaths(DispatcherResult& result, vector<FactoredPath>& factored_paths, t_reg index_register, BblSet destination_bbls, BblToIndexMap bbl_indexes)
{
  /* keep a list of already modified BBLs */
  set<t_bbl *> already_modified;

  for (auto& path : factored_paths)
  {
    t_bbl *dst = CFG_EDGE_TAIL(path.outgoing_edge);

    /* look up the index of the destination block */
    t_uint32 index;
    if (bbl_indexes.size() > 0)
      index = bbl_indexes[dst];
    else
      index = distance(destination_bbls.begin(), destination_bbls.find(dst));

    /* assign an index to every path */
    path.index = index;

    t_bbl *modify = CFG_EDGE_HEAD(path.incoming_path.back());
    if (already_modified.find(modify) == already_modified.end())
    {
      ASSERT(!RegsetIn(BblRegsLiveAfter(CFG_EDGE_HEAD(path.outgoing_edge)), index_register),
              ("index register r%u is live after slice! @iB", index_register, CFG_EDGE_HEAD(path.outgoing_edge)));
      /* only save the constant if this has not been done already */
      t_arm_ins *index_producer = ProduceConstantInBbl(modify, index_register, index, false, false);
      InsMarkAfIndexInstruction(T_INS(index_producer));
      result.added_ins_info.AddInstruction(T_INS(index_producer));

      RecordModifiedBbl(modify);
      already_modified.insert(modify);

      if (index_register == ARM_REG_R13)
        BblSetAFFlag(modify, AF_FLAG_DIRTY_SP);
    }
  }
}

DispatcherResult ApplyConditionalJumpDispatcher(F_DispatchGeneratorArguments)
{
  DispatcherResult result = DispatcherResult();

  /* keep a list of already modified BBLs */
  set<t_bbl *> already_modified;

  /* choose an index register */
  t_reg index_register = ChooseRegister(usable_registers);
  RegsetSetAddReg(result.used_registers, index_register);

  /* some diagnostic information */
  VERBOSE(0, (AF "dispatcher: conditional jump"));
  VERBOSE(0, (AF "      index register r%u, %d paths", index_register, factored_paths.size()));
  dispatch_register = index_register;

  /* sometimes multiple code paths end in the same BBL.
   * We need to make sure that every destination block has a unique index associated with it. */
  BblSet destination_bbls;
  for (auto path : factored_paths)
    destination_bbls.insert(CFG_EDGE_TAIL(path.outgoing_edge));
  ASSERT(destination_bbls.size() <= 2, ("number of unique destination BBLs is not two! (%d)", destination_bbls.size()));

  /* store the cases */
  for (auto destination : destination_bbls)
    switchcase_data.push_back(SwitchcaseData{{}, destination, false, NULL, DummyEdgeData::DispatchType::Other});

  /* the following code assumes that the...
   *  - path with index 1 corresponds to the JUMP edge;
   *  - path with index 0 corresponds to the FALLTHROUGH edge.
   * If this is changed, the branch condition should be changed too (see lower)! */
  BblToIndexMap dummy_bbl_indexes;
  AssignIndexesToPaths(result, factored_paths, index_register, destination_bbls, dummy_bbl_indexes);

  /* add instructions to the factored BBL */
  t_arm_ins *new_cmp_ins = NULL;

  /* as we compare with 0, we need to JUMP when NE */
  ArmMakeInsForBbl(Cmp, Append, new_cmp_ins, factored_bbl, FALSE, index_register, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  AFFactoringLogInstruction(new_cmp_ins, "MERGED_DISPATCHER_CONDJP");
  result.added_ins_info.AddInstruction(T_INS(new_cmp_ins));

  t_arm_ins *new_branch_ins = NULL;
  ArmMakeInsForBbl(CondBranch, Append, new_branch_ins, factored_bbl, FALSE, ARM_CONDITION_NE);
  AFFactoringLogInstruction(new_branch_ins, "MERGED_DISPATCHER_CONDJP");
  result.added_ins_info.AddInstruction(T_INS(new_branch_ins));

  /* make sure liveness information is correct */
  UseRegisterInBbl(factored_bbl, index_register);

  return result;
}

/* for each of the two slices, used in two functions! */
/* whether or not a zero can be produced in the register */
static array<array<bool, 15>, 2> already_zero;
static array<array<bool, 15>, 2> move_zero;
static array<array<bool, 15>, 2> create_zero;

/* whether or not a non-zero can be produced in the register */
static array<array<bool, 15>, 2> already_nonzero;
static array<array<bool, 15>, 2> move_nonzero;
static array<array<bool, 15>, 2> create_nonzero;

bool ChooseInternalConditionalJumpDispatcherRegister(SliceSet slices, size_t slice_size, t_gpregisters preferably_dont_touch, bool use_cached,
                                                      Slice *& null_slice, t_reg& subject, int& selected_zero_index, int& selected_nonzero_index, int& nr_internal_constants)
{
#define A_INDEX 0
#define B_INDEX 1

  /* results */
  null_slice = nullptr;
  subject = ARM_REG_NONE;
  selected_zero_index = -1;
  selected_nonzero_index = -1;
  nr_internal_constants = 0;

  Slice *a = *(slices.begin());

  Slice *b = nullptr;
  if (slices.size() > 1)
    b = *(next(slices.begin()));

  /* recalculate data if needed */
  if (!use_cached || a->cache_invalidated || b->cache_invalidated) {
    a->PrecalculateCombineData(slice_size);

    if (b != nullptr)
      b->PrecalculateCombineData(slice_size);
  }

  /* collect detailed register information */
  for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
    if (!GPRegistersIn(preferably_dont_touch, r))
      continue;

    a->CanProduceZeroInRegister(r, already_zero[A_INDEX][r], move_zero[A_INDEX][r], create_zero[A_INDEX][r]);
    a->CanProduceNonzeroInRegister(r, already_nonzero[A_INDEX][r], move_nonzero[A_INDEX][r], create_nonzero[A_INDEX][r]);
    if (already_zero[A_INDEX][r] && already_nonzero[A_INDEX][r]) {
      CfgDrawFunctionGraphs(SliceSetCfg(slices), "conflict");
      FATAL(("what? A r%d is zero as well as nonzero! %s", r, a->Print().c_str()));
    }

    if (b != nullptr) {
      b->CanProduceZeroInRegister(r, already_zero[B_INDEX][r], move_zero[B_INDEX][r], create_zero[B_INDEX][r]);
      b->CanProduceNonzeroInRegister(r, already_nonzero[B_INDEX][r], move_nonzero[B_INDEX][r], create_nonzero[B_INDEX][r]);
      if (already_zero[B_INDEX][r] && already_nonzero[B_INDEX][r]) {
        CfgDrawFunctionGraphs(SliceSetCfg(slices), "conflict");
        FATAL(("what? B r%d is zero as well as nonzero! %s", r, b->Print().c_str()));
      }
    }
  }

  if (b == nullptr) {
    already_zero[B_INDEX].fill(true);
    move_zero[B_INDEX].fill(true);
    create_nonzero[B_INDEX].fill(true);
  }

  auto select_best_candidate = [preferably_dont_touch] (int zero_index, int nonzero_index, int& max_score) {
    max_score = 0;
    t_reg selected_register = ARM_REG_NONE;

    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      if (!GPRegistersIn(preferably_dont_touch, r))
        continue;

      bool ok = true;

      int score = 0;
      if (already_zero[zero_index][r])
        score += 3;
      else if (move_zero[zero_index][r])
        score += 2;
      else if (create_zero[zero_index][r])
        score += 1;
      else
        ok = false;

      /* look for the next possible register */
      if (!ok)
        continue;

      if (already_nonzero[nonzero_index][r])
        score += 3;
      else if (move_nonzero[nonzero_index][r])
        score += 2;
      else if (create_nonzero[nonzero_index][r])
        score += 1;
      else
        ok = false;

      /* look for the next possible register */
      if (!ok)
        continue;

      /* if we get here, the register is possible! */
      if (score > max_score) {
        selected_register = r;
        max_score = score;
      }
    }

    return selected_register;
  };

  /* when we choose a to be the null slice */
  int a_zero_score;
  t_reg a_zero_register = select_best_candidate(A_INDEX, B_INDEX, a_zero_score);

  /* when we choose b to be the null slice */
  int b_zero_score;
  t_reg b_zero_register = select_best_candidate(B_INDEX, A_INDEX, b_zero_score);

  if (a_zero_register == ARM_REG_NONE
      && b_zero_register == ARM_REG_NONE) {
    /* impossible */
  }
  else if (a_zero_register == ARM_REG_NONE
            && b != nullptr) {
    null_slice = b;
  }
  else if (b_zero_register == ARM_REG_NONE) {
    null_slice = a;
  }
  else {
    /* pick best */
    if (b_zero_score >= a_zero_score
        && b != nullptr) {
      /* better to choose b zero */
      null_slice = b;
    }
    else {
      /* better to choose a zero */
      null_slice = a;
    }
  }

  subject = (null_slice == a) ? a_zero_register : b_zero_register;
  selected_zero_index = (null_slice == a) ? A_INDEX : B_INDEX;
  selected_nonzero_index = (null_slice == a) ? B_INDEX : A_INDEX;

  if (already_zero[selected_zero_index][subject]
      || move_zero[selected_zero_index][subject])
    nr_internal_constants++;

  if (already_nonzero[selected_nonzero_index][subject]
      || move_nonzero[selected_nonzero_index][subject])
    nr_internal_constants++;

  return null_slice != nullptr;
}

DispatcherResult ApplyInternalConditionalJumpDispatcher(F_DispatchGeneratorArguments)
{
  DispatcherResult result = DispatcherResult();

  /* we use a register from the preferably_dont_touch regset to compare to zero */
  t_reg subject = ARM_REG_NONE;

  VERBOSE(0, (AF "dispatcher: internal conditional jump"));
  VERBOSE(0, (AF "      slice size %d", slice_size));
  VERBOSE(0, (AF "      index register not known yet, %d paths (possible registers %s)", factored_paths.size(), GPRegistersPrint(BBL_CFG(factored_bbl), preferably_dont_touch).c_str()));

  /* necessary stuff */
  BblSet destination_bbls;
  for (auto path : factored_paths)
    destination_bbls.insert(CFG_EDGE_TAIL(path.outgoing_edge));
  ASSERT(destination_bbls.size() <= 2, ("number of unique destination BBLs is not two! (%d)", destination_bbls.size()));

  /* store the cases */
  t_bbl *dest = nullptr;
  for (auto destination : destination_bbls) {
    switchcase_data.push_back(SwitchcaseData{{}, destination, false, NULL, DummyEdgeData::DispatchType::Other});
    dest = destination;
  }

  if (switchcase_data.size() == 1) {
    /* create dummy entry */
    switchcase_data.push_back(SwitchcaseData{{}, NULL, false, NULL, DummyEdgeData::DispatchType::Other});
  }

  /* assign indexes to paths
   * 0 = JUMP, 1 = FALLTHROUGH */
  SliceSet slices;
  for (auto path : factored_paths)
    slices.insert(path.slice);

  Slice *null_slice = nullptr;
  int selected_zero_index = -1;
  int selected_nonzero_index = -1;
  int nr_used_constants = 0;
  if (!ChooseInternalConditionalJumpDispatcherRegister(slices, slice_size, preferably_dont_touch, true, null_slice, subject, selected_zero_index, selected_nonzero_index, nr_used_constants)) {
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      if (!GPRegistersIn(preferably_dont_touch, r))
        continue;

      DEBUG(("A/r%d Z:%d/%d M:%d/%d C:%d/%d", r, already_zero[A_INDEX][r], already_nonzero[A_INDEX][r], move_zero[A_INDEX][r], move_nonzero[A_INDEX][r], create_zero[A_INDEX][r], create_nonzero[A_INDEX][r]));
      DEBUG(("B/r%d Z:%d/%d M:%d/%d C:%d/%d", r, already_zero[B_INDEX][r], already_nonzero[B_INDEX][r], move_zero[B_INDEX][r], move_nonzero[B_INDEX][r], create_zero[B_INDEX][r], create_nonzero[B_INDEX][r]));
    }

    FATAL(("no suitable register found!"));
  }

  RegsetSetAddReg(result.used_registers, subject);
  VERBOSE(0, (AF "      index register r%d (null slice %c)", subject, (selected_zero_index == A_INDEX) ? 'a' : 'b'));
  dispatch_register = subject;

  enum class PathIndexCreationAction {
    Illegal,
    None,
    RegMove,
    RegConstant
  };

  auto TryDoNothing = [&bbl_to_slicevector_map, &slice_to_bbl_map] (Slice *slice) {
    /* no need to do anything by default */
    PathIndexCreationAction index_action = PathIndexCreationAction::None;

    /* also check whether multiple slices are associated to the same BBL.
     * if that's the case, don't assume the register to be unmodified!
     * Especially if the slice is not the first one in the BBL. */
    t_bbl *bbl = slice_to_bbl_map[slice];
    SliceVector slices = bbl_to_slicevector_map[bbl];

    bool first = true;
    for (auto s : slices) {
      if (s == slice)
        break;

      first = false;
    }

    /* take action when the slice is found, and it is not the first one in the BBL */
    if (!first)
      index_action = PathIndexCreationAction::RegConstant;

    return index_action;
  };

  /* assign indices to code paths
   * 1 = jump, 0 = fallthrough */
  BblSet already_modified;
  for (auto& path : factored_paths) {
    t_bbl *bbl = path.slice->Bbl();
    t_cfg *cfg = BBL_CFG(bbl);
    path.index = (path.slice == null_slice) ? CONDITIONAL_JUMP_DISPATCHER_JUMP_EDGE : CONDITIONAL_JUMP_DISPATCHER_FALLTHROUGH_EDGE;

    PathIndexCreationAction index_action = PathIndexCreationAction::Illegal;
    t_uint64 constant = 0;
    t_reg source_reg = ARM_REG_NONE;

    t_bbl *modify = CFG_EDGE_HEAD(path.incoming_path.back());
    if (path.slice == null_slice) {
      path.assumption.value = RegisterValue::Zero;
      constant = 0;

      /* make sure a 0 is here */
      if (already_zero[selected_zero_index][subject]) {
        index_action = TryDoNothing(path.slice);

#ifdef DEBUG_ICONDJUMP
        if (index_action != PathIndexCreationAction::None)
          DEBUG(("can't assume r%d to be unmodified here!", subject));
#endif
      }

      if (move_zero[selected_zero_index][subject]
          && index_action == PathIndexCreationAction::Illegal) {
        /* need to move the contents of one register to this one */
        REGSET_FOREACH_REG(path.slice->null_before, source_reg) break;
        if (source_reg >= ARM_REG_R15)
          source_reg = ARM_REG_NONE;
        ASSERT(source_reg != ARM_REG_NONE, ("no source register found"));

        index_action = PathIndexCreationAction::RegMove;
      }

      if (create_zero[selected_zero_index][subject]
          && index_action == PathIndexCreationAction::Illegal) {
        /* need to create 0 constant */
        index_action = PathIndexCreationAction::RegConstant;
      }
    }
    else {
      /* TODO: random non-null constant */
      path.assumption.value = RegisterValue::Nonzero;
      constant = 1;

      /* make sure a !0 is here */
      if (already_nonzero[selected_nonzero_index][subject]) {
        index_action = TryDoNothing(path.slice);

#ifdef DEBUG_ICONDJUMP
        if (index_action != PathIndexCreationAction::None)
          DEBUG(("can't assume r%d to be unmodified here!", subject));
#endif
      }

      if (move_nonzero[selected_nonzero_index][subject]
          && index_action == PathIndexCreationAction::Illegal) {
        t_regset all_nonzero_registers = path.slice->nonzero_before;
        RegsetSetUnion(all_nonzero_registers, path.slice->tag_before);
        RegsetSetUnion(all_nonzero_registers, path.slice->constant_before);
        RegsetSetDiff(all_nonzero_registers, path.slice->null_before);

        REGSET_FOREACH_REG(all_nonzero_registers, source_reg) break;
        if (source_reg >= ARM_REG_R15)
          source_reg = ARM_REG_NONE;

        if (source_reg != ARM_REG_NONE)
          index_action = PathIndexCreationAction::RegMove;
        else {
          /* a non-0 can always be produced,
           * this is needed for the fallback case below! */
          create_nonzero[selected_nonzero_index][subject] = true;
        }
      }

      if (create_nonzero[selected_nonzero_index][subject]
          && index_action == PathIndexCreationAction::Illegal) {
        index_action = PathIndexCreationAction::RegConstant;
      }
    }

    ASSERT(index_action != PathIndexCreationAction::Illegal, ("illegal index creation action (not set in any case!)"));

    if (already_modified.find(modify) == already_modified.end()) {
      t_arm_ins *ins;
      switch (index_action) {
      case PathIndexCreationAction::None:
#ifdef DEBUG_ICONDJUMP
        DEBUG(("r%d already contains %s in @B", subject, (path.slice == null_slice) ? "0" : "!0", modify));
#endif
        /* Take note of this assumption, as this must still hold when all constant information has been propagated.
        * In some cases, the register is assumed to contain a non-zero value where it will actually contain
        * zero due to other inserted instructions (i.e., a mov-immediate 0 for the non-null slice of this transformation). */
        RegisterAssumption assumption;
        assumption.value = (path.slice == null_slice) ? RegisterValue::Zero : RegisterValue::Nonzero;
        assumption.constant = constant;
        RecordRegisterAssumption(modify, subject, assumption);

        RecordProducingInstruction(modify, subject, ProducedValue{NULL}, false);
        break;
      case PathIndexCreationAction::RegMove:
        ASSERT(subject != source_reg, ("what? r%d/r%d", subject, source_reg));
#ifdef DEBUG_ICONDJUMP
        DEBUG(("r%d <- r%d (%s) in @B", subject, source_reg, (path.slice == null_slice) ? "0" : "!0",  modify));
#endif
        ins = ProduceRegisterMoveInBbl(modify, subject, source_reg, false, false);
        result.added_ins_info.AddInstruction(T_INS(ins));

        if (subject == ARM_REG_R13)
          BblSetAFFlag(modify, AF_FLAG_DIRTY_SP);
        RecordRegisterMoveConstant(ins, (path.slice == null_slice) ? RegisterValue::Zero : RegisterValue::Nonzero);
        break;
      case PathIndexCreationAction::RegConstant:
#ifdef DEBUG_ICONDJUMP
        DEBUG(("r%d = %s in @B", subject, (path.slice == null_slice) ? "0" : "!0", modify));
#endif
        ins = ProduceConstantInBbl(modify, subject, constant, false, false);
        result.added_ins_info.AddInstruction(T_INS(ins));

        InsMarkAfIndexInstruction(T_INS(ins));
        if (subject == ARM_REG_R13)
          BblSetAFFlag(modify, AF_FLAG_DIRTY_SP);
        break;

      default:
        FATAL(("unsupported index creation action %d", static_cast<t_uint32>(index_action)));
      }

      already_modified.insert(modify);
    }

    /* keep correct list of BBL references */
    switchcase_data[path.index].bbl = CFG_EDGE_TAIL(path.outgoing_edge);
  }

  /* compare-zero */
  t_arm_ins *compare_instruction;
  ArmMakeInsForBbl(Cmp, Append, compare_instruction, factored_bbl, FALSE, subject, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  AFFactoringLogInstruction(compare_instruction, "MERGED_DISPATCHER_ICONDJP");
  result.added_ins_info.AddInstruction(T_INS(compare_instruction));

  /* conditional jump */
  t_arm_ins *branch_instruction;
  ArmMakeInsForBbl(CondBranch, Append, branch_instruction, factored_bbl, FALSE, ARM_CONDITION_EQ);
  AFFactoringLogInstruction(branch_instruction, "MERGED_DISPATCHER_ICONDJP");
  result.added_ins_info.AddInstruction(T_INS(branch_instruction));

  UseRegisterInBbl(factored_bbl, subject);

  return result;
}

DispatcherResult ApplyIndirectBranchDispatcher(F_DispatchGeneratorArguments)
{
  DispatcherResult result = DispatcherResult();

  /* keep a list of already modified BBLs */
  set<t_bbl *> already_modified;

  /* choose a branch register */
  t_reg branch_register = ChooseRegister(usable_registers);
  RegsetSetAddReg(result.used_registers, branch_register);

  /* some diagnostic information */
  VERBOSE(0, (AF "dispatcher: indirect branch"));
  VERBOSE(0, (AF "      branch register r%u, %d paths", branch_register, factored_paths.size()));
  dispatch_register = branch_register;

  /* save the return address in every predecessor */
  for (auto& path : factored_paths)
  {
    t_bbl *modify = CFG_EDGE_HEAD(path.incoming_path.back());
    if (already_modified.find(modify) == already_modified.end())
    {
      if (RegsetIn(BblRegsLiveAfter(CFG_EDGE_HEAD(path.outgoing_edge)), branch_register))
        CfgDrawFunctionGraphs(BBL_CFG(modify), "live");
      ASSERT(!RegsetIn(BblRegsLiveAfter(CFG_EDGE_HEAD(path.outgoing_edge)), branch_register),
              ("branch register r%u is live after slice! @iB", branch_register, CFG_EDGE_HEAD(path.outgoing_edge)));
      /* only save the return address if this has not been done already */
      t_arm_ins *ins = ProduceAddressOfBblInBbl(modify, branch_register, CFG_EDGE_TAIL(path.outgoing_edge));
      result.added_ins_info.AddInstruction(T_INS(ins));

      InsMarkAfIndexInstruction(T_INS(ins));
      RecordModifiedBbl(modify);
      already_modified.insert(modify);
    }
  }

  /* create an indirect branch instruction in the factored BBL */
  t_arm_ins *new_ins = NULL;
  ArmMakeInsForBbl(UncondBranchExchange, Append, new_ins, factored_bbl, FALSE, branch_register);
  AFFactoringLogInstruction(new_ins, "MERGED_DISPATCHER_IB");
  result.added_ins_info.AddInstruction(T_INS(new_ins));

  /* register bookkeeping */
  UseRegisterInBbl(factored_bbl, branch_register);

  return result;
}

static
DispatcherResult ApplySwitchDispatcherCommon(vector<t_reg>& usable_regs, vector<FactoredPath>& factored_paths, t_bbl *factored_bbl, vector<SwitchcaseData>& switchcase_data, t_bbl *& factored_bbl_tail, bool is_data, t_reg& dispatch_register, t_possibility_flags flags,
                                      function<t_arm_ins * (DispatcherResult& , t_bbl *, t_reg, bool, size_t, t_possibility_flags)> F_CreateSwitchInstructions,
                                      function<t_uint32 (t_uint32)> F_GetInstructionIndexForCase,
                                      function<t_reloc * (t_object *, t_ins *, t_bbl *, t_bbl *, t_uint32)> F_CreateRelocationForCase)
{
  DispatcherResult result = DispatcherResult();

  t_cfg *cfg = BBL_CFG(factored_bbl);
  t_object *obj = CFG_OBJECT(cfg);

  /* can we insert a CMP instruction? (i.e., an instruction that overwrites the status flags) */
  BblSet foo;
  bool can_overwrite_statusflags = AreStatusRegistersDead(factored_paths, foo);

  /* choose an index register */
  t_reg index_register = ChooseRegister(usable_regs);
  RegsetSetAddReg(result.used_registers, index_register);

  /* some diagnostic information */
  VERBOSE(0, (AF "      index register r%u, %d paths, %s", index_register, factored_paths.size(), can_overwrite_statusflags ? "statusflags" : " "));
  dispatch_register = index_register;

  /* offsets encoded in relocations will need to refer to the switch instruction.
   * As the way to install TO-relocatables in Diablo is to use BBLs (and not individual instructions),
   * it is better to put the switch-related instructions in their own BBL.
   * This way, offsets to specific instructions involved in the switch statement are constant. */
  factored_bbl_tail = BblSplitBlock(factored_bbl, BBL_INS_LAST(factored_bbl), FALSE);
  AfterSplit(factored_bbl, factored_bbl_tail, FALSE);

  /* sometimes multiple code paths end in the same BBL.
   * We need to make sure that every destination block has a unique index associated with it. */
  BblSet destination_bbls;
  for (auto path : factored_paths) {
    destination_bbls.insert(CFG_EDGE_TAIL(path.outgoing_edge));

    /* randomly insert dummy entries */
    if (RNGGeneratePercent(af_rng_dummy) <= static_cast<t_uint32>(diabloanoptarm_options.af_dummy_entry_chance))
      destination_bbls.insert(factored_bbl_tail);
  }

  /* create a data BBL, if needed */
  t_bbl *data_bbl = nullptr;
  if (is_data)
  {
    data_bbl = BblNew(cfg);
    BBL_SET_ALIGNMENT(data_bbl, 0);
    BBL_SET_ALIGNMENT_OFFSET(data_bbl, 0);
  }

  t_arm_ins *switch_ins = F_CreateSwitchInstructions(result, factored_bbl_tail, index_register, can_overwrite_statusflags, destination_bbls.size(), flags);
  ARM_INS_SET_ATTRIB(switch_ins, ARM_INS_ATTRIB(switch_ins) | IF_SWITCHJUMP);

  t_uint32 case_idx = 0;

  auto add_entry = [is_data, F_GetInstructionIndexForCase, F_CreateRelocationForCase, &case_idx, data_bbl, obj, factored_bbl_tail, cfg, switch_ins] (t_bbl *dest, SwitchcaseData& d, DispatcherResult& result) {
    if (is_data)
    {
      /* get the instruction index for this case (e.g., for TBB/TBH in the future) */
      auto insn_idx = F_GetInstructionIndexForCase ? F_GetInstructionIndexForCase(case_idx) : case_idx;

      /* try to look up the right instruction for this case index */
      t_arm_ins *data_ins = T_ARM_INS(BBL_INS_FIRST(data_bbl));
      while (insn_idx > 0
              && data_ins)
      {
        data_ins = ARM_INS_INEXT(data_ins);
        insn_idx--;
      }

      /* create a new instruction if needed */
      if (!data_ins)
      {
        ArmMakeInsForBbl(Data, Append, data_ins, data_bbl, FALSE, 0);
        ARM_INS_SET_ATTRIB(data_ins, ARM_INS_ATTRIB(data_ins) | IF_SWITCHTABLE);
        AFFactoringLogInstruction(data_ins, "MERGED_DISPATCHER_SWDATA");
        result.added_ins_info.AddData(T_INS(data_ins));
      }

      /* create branch loop instruction */
      t_bbl *dst = dest;
      if (d.is_dummy) {
        /* create a new BBL to contain the branch instruction */
        dst = BblNew(cfg);
        if (BBL_FUNCTION(dest))
          BblInsertInFunction(dst, BBL_FUNCTION(dest));

        BBL_SET_ATTRIB(dst, BBL_ATTRIB(dst) | BBL_ADVANCED_FACTORING);

        /* create new BBL */
        t_arm_ins *branch_ins;
        ArmMakeInsForBbl(UncondBranch, Append, branch_ins, dst, FALSE);
        result.added_ins_info.AddInstruction(T_INS(branch_ins));
        AFFactoringLogInstruction(branch_ins, "MERGED_DISPATCHER_SWINF");

        /* jump edge */
        t_cfg_edge *dummy_jump = CfgEdgeCreate(cfg, dst, dst, ET_JUMP);
        CfgEdgeMarkNeedsRedirect(dummy_jump);
        CfgEdgeMarkFake(dummy_jump);
      }

      d.bbl = dst;
      d.relocs.push_back(F_CreateRelocationForCase(obj, T_INS(data_ins), ARM_INS_BBL(switch_ins), dst, case_idx));
    }
    else
    {
      ASSERT(F_GetInstructionIndexForCase == nullptr, ("what?"));

      /* create a new BBL to contain the branch instruction */
      t_bbl *ins_bbl = BblNew(cfg);
      if (BBL_FUNCTION(dest))
        BblInsertInFunction(ins_bbl, BBL_FUNCTION(dest));

      BBL_SET_ATTRIB(ins_bbl, BBL_ATTRIB(ins_bbl) | BBL_ADVANCED_FACTORING);

      /* create a branch instruction in the new BBL */
      t_arm_ins *branch_ins;
      ArmMakeInsForBbl(UncondBranch, Append, branch_ins, ins_bbl, FALSE);
      AFFactoringLogInstruction(branch_ins, "MERGED_DISPATCHER_SWB");

      /* create the edge from the branch instruction to the final destination */
      t_bbl *dst = dest;
      if (d.is_dummy)
        dst = ins_bbl;

      d.landing_edge = CfgEdgeCreate(cfg, ins_bbl, dst, ET_JUMP);
      if (d.is_dummy) {
        CfgEdgeMarkNeedsRedirect(d.landing_edge);
        CfgEdgeMarkFake(d.landing_edge);
      }

      BblCopyExecInformation(dst, ins_bbl);
      result.added_ins_info.AddInstruction(T_INS(branch_ins));//needs to happen AFTER copy of exec count
      CFG_EDGE_SET_EXEC_COUNT(d.landing_edge, BBL_EXEC_COUNT(dst));

      d.bbl = ins_bbl;
    }
  };

  BblToIndexMap bbl_to_index;

  /* add the needed data instructions with their associated relocations */
  for (auto dest : destination_bbls)
  {
    /* create entry in switch table */
    SwitchcaseData d = {{}, NULL, false, NULL,
      is_data ? DummyEdgeData::DispatchType::SwitchOffset : DummyEdgeData::DispatchType::SwitchBranch};

    if (dest == factored_bbl_tail) {
      /* this is a dummy entry */
      DEBUG(("generating dummy entry %d", case_idx));
      d.is_dummy = true;
      d.bbl = factored_bbl_tail;
    }

    add_entry(dest, d, result);
    switchcase_data.push_back(d);

    bbl_to_index[dest] = case_idx;
    case_idx++;
  }

  AssignIndexesToPaths(result, factored_paths, index_register, destination_bbls, bbl_to_index);

  if (is_data)
  {
    /* keep the jump table alive by adding a relocation */
    t_reloc *rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                                    AddressNullForObject(obj), /* addend */
                                                    T_RELOCATABLE(switch_ins), /* from + offset */
                                                    AddressNullForObject(obj),
                                                    T_RELOCATABLE(data_bbl), /* to + offset */
                                                    AddressNullForObject(obj),
                                                    FALSE, NULL, NULL, NULL,
                                                    "R00A00+\\*\\s0000$");
    RELOC_SET_LABEL(rel, StringDup("Switch table reloc"));
  }

  /* need to add default branch if conditional switch instruction */
  if (ArmInsIsConditional(switch_ins)) {
    /* create a new BBL to contain the branch instruction */
    t_bbl *ins_bbl = BblNew(cfg);
    BblInsertInFunction(ins_bbl, BBL_FUNCTION(factored_bbl_tail));
    BBL_SET_ATTRIB(ins_bbl, BBL_ATTRIB(ins_bbl) | BBL_ADVANCED_FACTORING);

    /* create a branch instruction in the new BBL */
    t_arm_ins *branch_ins;
    ArmMakeInsForBbl(UncondBranch, Append, branch_ins, ins_bbl, FALSE);
    AFFactoringLogInstruction(branch_ins, "MERGED_DISPATCHER_SWDEFAULTB");
    result.added_ins_info.AddInstruction(T_INS(branch_ins));

    /* create the fallthrough edge */
    t_cfg_edge *fake_ft = CfgEdgeCreate(cfg, ARM_INS_BBL(switch_ins), ins_bbl, ET_FALLTHROUGH);
    CfgEdgeMarkFake(fake_ft);

    /* create the edge from the branch instruction to itself, for now */
    t_cfg_edge *fake = CfgEdgeCreate(cfg, ins_bbl, ins_bbl, ET_JUMP);
    CfgEdgeMarkNeedsRedirect(fake);
    CfgEdgeMarkFake(fake);

    /* make sure the edge will be redirected */
    DummyEdgeData dummy_data;
    dummy_data.dispatch_type = is_data ? DummyEdgeData::DispatchType::SwitchOffset : DummyEdgeData::DispatchType::SwitchBranch;
    GlobalRedirect(fake, dummy_data);
  }

  factored_bbl_tail = ARM_INS_BBL(switch_ins);

  return result;
}

DispatcherResult ApplySwitchDispatcherLdrAdd(F_DispatchGeneratorArguments)
{
  VERBOSE(0, (AF "dispatcher: switch (offset)"));

  auto result = ApplySwitchDispatcherCommon(usable_registers, factored_paths, factored_bbl, switchcase_data, factored_bbl_tail, true, dispatch_register, flags,
    [](DispatcherResult& result, t_bbl *bbl, t_reg index_register, bool can_overwrite_statusflags, size_t nr_cases, t_possibility_flags flags) -> t_arm_ins * {
      /*  */
      t_arm_condition_code switch_condition = ARM_CONDITION_AL;
      if (can_overwrite_statusflags)
      {
        /* random immediate for the CMP (immediate) instruction;
         * can encode 0xff at the max in ARM */

        /* need to generate an immediate with lower-bound being the number of cases */
        t_uint32 cmp_imm = nr_cases-1;
        if (diabloanoptarm_options.af_random_switch_check)
          cmp_imm = RNGGenerateWithRange(af_rng_randomswitch, nr_cases, 0xff);

        /* create the compare instruction */
        t_arm_ins *cmp_ins = NULL;
        ArmMakeInsForBbl(Cmp, Append, cmp_ins, bbl, FALSE, index_register, ARM_REG_NONE, cmp_imm, ARM_CONDITION_AL);
        AFFactoringLogInstruction(cmp_ins, "MERGED_DISPATCHER_SWO");
        result.added_ins_info.AddInstruction(T_INS(cmp_ins));

        /* need to check whether the immediate value can be encoded */
        t_uint32 dummy = 0;
        auto can_encode_immediate = ArmEncodeImmediate(cmp_ins, &dummy);

        if (can_encode_immediate)
        {
          switch_condition = ARM_CONDITION_LS;

          t_arm_ins *inc_ins = NULL;
          ArmMakeInsForBbl(Add, Append, inc_ins, bbl, FALSE, index_register, index_register, ARM_REG_NONE, 1, ARM_CONDITION_AL);
          AFFactoringLogInstruction(inc_ins, "MERGED_DISPATCHER_SWO");
          result.added_ins_info.AddInstruction(T_INS(inc_ins));

          UseRegisterInBbl(bbl, index_register);
          DefineRegisterInBbl(bbl, index_register);
        }
        else
        {
          ArmInsKill(cmp_ins);
          DEBUG(("can't encode immediate %x", nr_cases+1));
        }
      }

      t_bbl *next = BblSplitBlock(bbl, BBL_INS_LAST(bbl), FALSE);
      AfterSplit(bbl, next, FALSE);

      /* as we also want to support library code, the switch table should be PIC! */
      t_arm_ins *load_ins = NULL;
      ArmMakeInsForBbl(Ldr, Append, load_ins, next, FALSE, index_register, ARM_REG_R15, index_register, 0, switch_condition, TRUE, TRUE, FALSE);
      ARM_INS_SET_SHIFTTYPE(load_ins, ARM_SHIFT_TYPE_LSL_IMM);
      ARM_INS_SET_SHIFTLENGTH(load_ins, 2);
      AFFactoringLogInstruction(load_ins, "MERGED_DISPATCHER_SWO");
      result.added_ins_info.AddInstruction(T_INS(load_ins));

      /* create the switch instruction: LDR pc, [pc, index_register] */
      t_arm_ins *switch_ins = NULL;
      ArmMakeInsForBbl(Add, Append, switch_ins, next, FALSE, ARM_REG_R15, ARM_REG_R15, index_register, 0, switch_condition);
      AFFactoringLogInstruction(switch_ins, "MERGED_DISPATCHER_SWO");
      result.added_ins_info.AddInstruction(T_INS(switch_ins));

      /* register bookkeeping */
      UseRegisterInBbl(next, index_register);
      DefineRegisterInBbl(next, index_register);

      return switch_ins;
    },
    nullptr,
    [](t_object *obj, t_ins *data_ins, t_bbl *switch_bbl, t_bbl *destination, t_uint32 case_idx) -> t_reloc * {
      /* create a relocation for the data word in the data BBL */
      t_reloc *data_rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                                          AddressNew32(8), /* addend */
                                                          T_RELOCATABLE(data_ins),   /* from + offset */
                                                          AddressNullForObject(obj),
                                                          T_RELOCATABLE(destination), /* to + offset */
                                                          AddressNullForObject(obj),
                                                          FALSE, NULL, NULL, NULL,
                                                          "R00R01-A00-\\" WRITE_32);
      /* offset 4 because we want to calculate relative to the ADD instruction */
      RelocAddRelocatable(data_rel, T_RELOCATABLE(switch_bbl), AddressNew32((BBL_NINS(switch_bbl) - 1) * 4));

      return data_rel;
    });

  return result;
}

DispatcherResult ApplySwitchDispatcherBranchTable(F_DispatchGeneratorArguments)
{
  VERBOSE(0, (AF "dispatcher: switch (branch)"));

  auto result = ApplySwitchDispatcherCommon(usable_registers, factored_paths, factored_bbl, switchcase_data, factored_bbl_tail, false, dispatch_register, flags,
    [](DispatcherResult& result, t_bbl *bbl, t_reg index_register, bool can_overwrite_statusflags, size_t nr_cases, t_possibility_flags flags) -> t_arm_ins * {
      /* */
      t_arm_condition_code switch_condition = ARM_CONDITION_AL;
      if (can_overwrite_statusflags)
      {
        /* random immediate for the CMP (immediate) instruction;
         * can encode 0xff at the max in ARM */

        /* need to generate an immediate with lower-bound being the number of cases */
        t_uint32 cmp_imm = nr_cases-1;
        if (diabloanoptarm_options.af_random_switch_check)
          cmp_imm = RNGGenerateWithRange(af_rng_randomswitch, nr_cases, 0xff);

        /* create the compare instruction */
        t_arm_ins *cmp_ins = NULL;
        ArmMakeInsForBbl(Cmp, Append, cmp_ins, bbl, FALSE, index_register, ARM_REG_NONE, cmp_imm, ARM_CONDITION_AL);
        AFFactoringLogInstruction(cmp_ins, "MERGED_DISPATCHER_SWB");
        result.added_ins_info.AddInstruction(T_INS(cmp_ins));

        /* need to check whether the immediate value can be encoded */
        t_uint32 dummy = 0;
        auto can_encode_immediate = ArmEncodeImmediate(cmp_ins, &dummy);

        if (can_encode_immediate)
        {
          switch_condition = ARM_CONDITION_LS;
        }
        else
        {
          ArmInsKill(cmp_ins);
          DEBUG(("can't encode immediate %x", nr_cases+1));
        }
      }

      t_arm_ins *switch_ins = NULL;
      ArmMakeInsForBbl(Add, Append, switch_ins, bbl, FALSE, ARM_REG_R15, ARM_REG_R15, index_register, 0, switch_condition);
      ARM_INS_SET_SHIFTTYPE(switch_ins, ARM_SHIFT_TYPE_LSL_IMM);
      ARM_INS_SET_SHIFTLENGTH(switch_ins, 2);
      AFFactoringLogInstruction(switch_ins, "MERGED_DISPATCHER_SWB");
      result.added_ins_info.AddInstruction(T_INS(switch_ins));

      /* register bookkeeping */
      UseRegisterInBbl(bbl, index_register);

      return switch_ins;
    },
    nullptr,
    nullptr);

  return result;
}

DispatcherResult ApplyDistributedTable(F_DispatchGeneratorArguments)
{
  /* We create a chain of BBLs: <factored instructions> --> <offset calculation> --> <switch instruction>
   *
   * First we need to produce the address of the section that contains the base table address.
   *   ADRP rX, <address of table address section>
   * Then we need to load the base address of the table we want to refer to.
   *   LDR rX, [rX]
   * Finally the offset can be loaded from the table.
   *   LDR rY, [rX, rY]
   *
   * The switch instruction itself:
   *   ADD pc, pc, rY
   */
  DispatcherResult result = DispatcherResult();

  VERBOSE(0, (AF "dispatcher: distributed table"));

  /* we need two registers to work with */
  t_reg index_register = ChooseRegister(usable_registers);
  RegsetSetAddReg(result.used_registers, index_register);

  t_reg tmp_register = ChooseRegister(usable_registers);
  RegsetSetAddReg(result.used_registers, tmp_register);

  VERBOSE(0, (AF "      index register r%d, temporary register r%d", index_register, tmp_register));
  dispatch_register = index_register;

  /* create the BBL that will contain the actual switch instruction */
  t_bbl *factored_bbl_next = BblSplitBlock(factored_bbl, BBL_INS_LAST(factored_bbl), FALSE);
  AfterSplit(factored_bbl, factored_bbl_next, FALSE);

  /* load the value of our own TLS variable */
  t_ins *tls_load = TlsLoad(factored_bbl_next, BBL_INS_LAST(factored_bbl_next), tmp_register, TLS_BASEADDRESS_SYMBOL_NAME);
  result.added_ins_info.AddInstruction(tls_load, 2);

  /* load the offset from the base table */
  t_arm_ins *offset_load_instruction;
  ArmMakeInsForBbl(Ldr, Append, offset_load_instruction, factored_bbl_next, FALSE, index_register, tmp_register, index_register, 0, ARM_CONDITION_AL, TRUE, TRUE, FALSE);
  AFFactoringLogInstruction(offset_load_instruction, "MERGED_DISPATCHER_DTBL");
  result.added_ins_info.AddInstruction(T_INS(offset_load_instruction));

  /* the ADD instruction should be put in a separate BBL in order for us to be able to refer to this BBL from relocations */
  factored_bbl_tail = BblSplitBlock(factored_bbl_next, BBL_INS_LAST(factored_bbl_next), FALSE);
  AfterSplit(factored_bbl_next, factored_bbl_tail, FALSE);

  /* use the loaded offset to modify the program counter */
  t_arm_ins *switch_instruction;
  ArmMakeInsForBbl(Add, Append, switch_instruction, factored_bbl_tail, FALSE, ARM_REG_R15, ARM_REG_R15, index_register, 0, ARM_CONDITION_AL);
  ARM_INS_SET_ATTRIB(switch_instruction, ARM_INS_ATTRIB(switch_instruction) | IF_SWITCHJUMP);
  AFFactoringLogInstruction(switch_instruction, "MERGED_DISPATCHER_DTBL");
  result.added_ins_info.AddInstruction(T_INS(switch_instruction));

  RecordDTableDispatcher(factored_bbl_tail);

  BblSet already_modified;

  for (auto& path : factored_paths)
  {
    SwitchcaseData d = {{}, NULL, false, NULL, DummyEdgeData::DispatchType::DistributedTable};

    t_bbl *factored_slice_bbl = CFG_EDGE_TAIL(path.incoming_path.back());
    t_bbl *destination = CFG_EDGE_TAIL(path.outgoing_edge);

    auto table_index = DistributedTableAddEntry(factored_slice_bbl, factored_bbl_tail, destination, d.relocs);
    VERBOSE(DISTTBL_VERBOSITY, ("using table index %d", table_index));

    /* assign an index to every path */
    path.index = switchcase_data.size();

    t_bbl *modify = CFG_EDGE_HEAD(path.incoming_path.back());
    if (already_modified.find(modify) == already_modified.end())
    {
      ASSERT(!RegsetIn(BblRegsLiveAfter(CFG_EDGE_HEAD(path.outgoing_edge)), index_register),
              ("index register r%u is live after slice! @iB", index_register, CFG_EDGE_HEAD(path.outgoing_edge)));
      /* only save the constant if this has not been done already */
      VERBOSE(DISTTBL_VERBOSITY, ("producing constant %d in BBL @eiB", table_index*4, modify));
      t_arm_ins *index_producer = ProduceConstantInBbl(modify, index_register, table_index * 4, false, false);
      InsMarkAfIndexInstruction(T_INS(index_producer));
      result.added_ins_info.AddInstruction(T_INS(index_producer));
      if (index_register == ARM_REG_R13)
        BblSetAFFlag(modify, AF_FLAG_DIRTY_SP);

      RecordModifiedBbl(modify);
      already_modified.insert(modify);
    }

    switchcase_data.push_back(d);
  }

  UseRegisterInBbl(factored_bbl_next, index_register);
  DefineRegisterInBbl(factored_bbl_next, tmp_register);
  DefineRegisterInBbl(factored_bbl_next, index_register);

  UseRegisterInBbl(factored_bbl_tail, index_register);

  return result;
}
