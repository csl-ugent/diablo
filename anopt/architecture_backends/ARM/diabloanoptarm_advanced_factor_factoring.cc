#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

//#define DEBUG_OUTPUT
//#define DUMP_TRANSFORMED_SETS
//#define DUMP_FACTORED_BBL

//#define ALREADY_TRANSFORMED_NR_DOTS 0
//#define ALREADY_TRANSFORMED_NR_DOTS_ID ALREADY_TRANSFORMED_NR_DOTS

AFPhase current_af_phase;
void AFFactoringLogInstruction(t_arm_ins *ins, string x) {
  x += ":" +  to_string(static_cast<t_uint32>(current_af_phase));
  FactoringLogInstruction(T_INS(ins), x);
}

/* main functionality */
static
void FunctionSetOverwrittenRegisters(t_function *function) {
  t_regset result = RegsetNew();

  t_bbl *bbl;
  FUNCTION_FOREACH_BBL(function, bbl) {
    RegsetSetUnion(result, BblRegsMaybeDef(bbl));
  }

  FUNCTION_SET_REGS_OVERWRITTEN(function, result);
}

static
AddedInstructionInfo FixFactoredBblIncomingEdges(t_bbl *bbl)
{
  AddedInstructionInfo result = AddedInstructionInfo();

  t_cfg_edge *keep_ft = NULL;
  bool multiple_ft = false;

  t_cfg_edge *keep_ret = NULL;
  bool multiple_ret = false;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(bbl, e)
  {
    /* fix multiple incoming FALLTHROUGH edges */
    if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
        || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
    {
      if (keep_ft)
        multiple_ft = true;
      else
        keep_ft = e;
    }
    /* fix multiple incoming RETURN edges */
    else if (CFG_EDGE_CAT(e) == ET_RETURN)
    {
      if (keep_ret)
        multiple_ret = true;
      else
        keep_ret = e;
    }
  }

  /* fixing multiple incoming fallthrough edges,
   * and make all incoming fallthrough edges jumps if at least one return edge comes in */
  if (multiple_ft || (keep_ret && keep_ft))
  {
    t_cfg_edge *tmp;

    /* need to use the SAFE iterator here because the edges get modified! */
    BBL_FOREACH_PRED_EDGE_SAFE(bbl, e, tmp)
    {
      /* only consider fallthrough edges */
      if (CFG_EDGE_CAT(e) != ET_FALLTHROUGH
          && CFG_EDGE_CAT(e) != ET_IPFALLTHRU)
        continue;

      /* only possibly skip this edge if no RETURN edge comes in */
      if (!keep_ret)
      {
        if (multiple_ft)
        {
          /* multiple incoming FT edges, which one is to be kept? */
          if (e == keep_ft)
            /* this is the one we want to keep */
            continue;
        }
        else
          /* only one incoming FT edge */
          continue;
      }

      t_bbl *b = AddBranchIndirectionOnEdge(e, false, result);
      BblMark2(b);
    }
  }

  /* fixing multiple incoming return edges */
  if (multiple_ret)
  {
    t_cfg_edge *tmp;
    BBL_FOREACH_PRED_EDGE_SAFE(bbl, e, tmp)
    {
      if (e == keep_ret)
        continue;

      if (CFG_EDGE_CAT(e) != ET_RETURN)
        continue;

      t_bbl *b = AddBranchIndirectionOnEdge(e, true, result);
      BblMark2(b);
    }
  }

  return result;
}

static
t_bbl *DuplicateBblForFactoring(t_bbl *orig)
{
  t_bbl *result = BblDup(orig);

  t_string func_name = StringIo("af-@G", BBL_OLD_ADDRESS(orig));
  LOG_MESSAGE(L_FACTORING, "%s,", func_name);

  t_function *factor = FunctionMake(result, func_name, FT_NORMAL);
  Free(func_name);
  FUNCTION_SET_BEHAVES(factor, FALSE);
  FUNCTION_SET_IS_AF(factor, TRUE);

  UpdateUseDef(result, orig);

  BblSetOriginalFunctionUID(result, af_function_uid);

  return result;
}

static
void AfFunctionUpdateFlags(t_function *fun, AfFlags flags)
{
  FUNCTION_SET_AF_FLAGS(fun, FUNCTION_AF_FLAGS(fun) | flags);
}

static
void AfFunctionUpdateFlagsWhole(t_function *fun)
{
  t_bbl *bbl;
  FUNCTION_FOREACH_BBL(fun, bbl) {
    t_arm_ins *ins;
    BBL_FOREACH_ARM_INS(bbl, ins) {
      if (RegsetIn(ARM_INS_REGS_DEF(ins), ARM_REG_R13))
        AfFunctionUpdateFlags(fun, AF_FLAG_DIRTY_SP);
    }
  }
}

void RedirectRelocations(t_bbl *original, t_bbl *factored)
{
  bool keep_going = true;
  while (keep_going)
  {
    keep_going = false;

    for (auto rr = BBL_REFED_BY(original); rr; rr = RELOC_REF_NEXT(rr))
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);

      for (t_uint32 i = 0; i < RELOC_N_TO_RELOCATABLES(rel); i++)
      {
        if (RELOC_TO_RELOCATABLE(rel)[i] == T_RELOCATABLE(original))
        {
          RelocSetToRelocatable(rel, i, T_RELOCATABLE(factored));
          keep_going = true;
          break;
        }
      }

      if (keep_going)
        break;
    }
  }
}

t_reg ChooseRegister(vector<t_reg>& registers)
{
  if (registers.size() == 0)
    return ARM_REG_NONE;

  t_reg result = registers.back();
  registers.pop_back();

  return result;
}

static
void AtomizeSliceInBbl(Slice *slice, size_t slice_size, size_t slice_nr)
{
  vector<t_ins *> insn_vector;

  t_ins *ins;
  BBL_FOREACH_INS(slice->Bbl(), ins)
    insn_vector.push_back(ins);

  if (slice->is_sequence) {
    /* sort the instructions based on their order ID */
    stable_sort(insn_vector.begin(), insn_vector.end(), [](const t_ins *lhs, const t_ins *rhs) {
      return INS_ORDER(lhs) < INS_ORDER(rhs);
    });
  }
  else {
    /* sort the instructions based on their slice ID, using a stable sort algorithm */
    stable_sort(insn_vector.begin(), insn_vector.end(), [](const t_ins *lhs, const t_ins *rhs) {
      return INS_SLICE_ID(lhs) > INS_SLICE_ID(rhs);
    });
  }

  /* TODO: maybe only set this to TRUE if one or move critical movements have been done */
  slice->cache_invalidated = true;

  /* apply the new schedule; inspired by obfuscation/generic/schedule_instructions.cc */
  t_ins *current_ins = NULL;
  for (size_t idx = 0; idx < insn_vector.size(); idx++)
  {
    t_ins *insn = insn_vector[idx];

    if (!current_ins)
    {
      /* first iteration */
      if (insn != BBL_INS_FIRST(slice->Bbl()))
        BblMoveInstructionBefore(insn, BBL_INS_FIRST(slice->Bbl()));
    }
    else
    {
      /* other iterations */
      if (insn != current_ins)
        BblMoveInstructionAfter(insn, current_ins);
    }

    current_ins = insn;
  }

  /* list the instructions to be moved */
  set<t_ins *> selected_instructions;

  ins = BBL_INS_FIRST(slice->Bbl());
  bool first_seen = false;
  while (ins) {
    if (!first_seen
        && slice->ContainsInstruction(ins, slice_size))
      first_seen = true;

    if (first_seen) {
      selected_instructions.insert(ins);

      if (ins == slice->base_instruction)
        break;
    }

    ins = INS_INEXT(ins);
  }
  ASSERT(ins, ("should not happen %s", slice->Print().c_str()));

  /* move the address producers out of the way */
  string s_before = "ADDR-BEFORE:" + to_string(slice_nr);
  for (auto x : slice->address_before) {
    if (selected_instructions.find(T_INS(x)) == selected_instructions.end())
      continue;

    AFFactoringLogInstruction(x, s_before);
    BblMoveInstructionBefore(T_INS(x), slice->GetR(slice_size - 1));
  }

  string s_after = "ADDR-AFTER:" + to_string(slice_nr);
  for (auto x : slice->address_after) {
    if (selected_instructions.find(T_INS(x)) == selected_instructions.end())
      continue;

    AFFactoringLogInstruction(x, s_after);
    BblMoveInstructionAfter(T_INS(x), slice->base_instruction);
  }
}

static
t_bbl *SliceSplitOff(Slice *slice, size_t slice_size)
{
  auto last = slice->elements.back();
  auto first = BBL_INS_FIRST(INS_BBL(last));
  while (!slice->ContainsInstruction(first, slice_size))
    first = INS_INEXT(first);

  /* first, slice and last BBL: initially no splitting has been done */
  auto first_bbl = slice->Bbl();
  auto slice_bbl = first_bbl;
  auto last_bbl  = first_bbl;

  /* as the BBL will be split, this DAG is not valid anymore! */
  DiabloBrokerCall("FreeDagForBbl", slice_bbl);

  /* split up */
  if (INS_IPREV(first))
  {
    t_bbl *tmp = slice_bbl;
    slice_bbl = BblSplitBlock(slice_bbl, INS_INEXT(INS_IPREV(first)), TRUE);
    AfterSplit(tmp, slice_bbl);
  }
  if (INS_INEXT(last))
  {
    last_bbl = BblSplitBlock(slice_bbl, last, FALSE);
    AfterSplit(slice_bbl, last_bbl);
  }

  // DEBUG(("split first: @iB", first_bbl));
  // DEBUG(("split slice: @iB", slice_bbl));
  // DEBUG(("split last : @iB", last_bbl));

  /* update bookkeeping */
  UpdateAssociatedSlices(first_bbl);

  return slice_bbl;
}

static
AddedInstructionInfo AddBranchToBblIfNeeded(t_bbl *bbl)
{
  AddedInstructionInfo result = AddedInstructionInfo();

  if (BBL_INS_LAST(bbl) && RegsetIn(INS_REGS_DEF(BBL_INS_LAST(bbl)), ARM_REG_R15))
    /* no branch needs to be added here */
    return result;

  bool is_thumb = /* TODO */FALSE;

  /* create an unconditional branch, jumping to the factored BBL */
  t_arm_ins *ins;
  ArmMakeInsForBbl(UncondBranch, Append, ins, bbl, is_thumb);
  result.AddInstruction(T_INS(ins));

  /* make this edge a jump edge */
  ASSERT(BBL_SUCC_FIRST(bbl) == BBL_SUCC_LAST(bbl), ("expected only one outgoing edge! @eiB", bbl));

  t_cfg_edge *e = BBL_SUCC_FIRST(bbl);
  ASSERT(CFG_EDGE_CAT(e) == ET_FALLTHROUGH || CFG_EDGE_CAT(e) == ET_IPFALLTHRU, ("this edge should be fallthrough! @E", e));
  CFG_EDGE_SET_CAT(e, ET_JUMP);

  return result;
}

static
F_DispatchGenerator ChooseDispatchGenerator(FactoringPossibility *poss, bool& using_conditional_jump, bool& added_trampoline, DispatcherType& dispatcher_type){
  F_DispatchGenerator result = nullptr;

  dispatcher_type = DispatcherChooser(poss->flags, af_rng_dispatcher);

  /* look up the generator function */
  auto x = dispatcher_type_to_generator_map.find(dispatcher_type);
  ASSERT(x != dispatcher_type_to_generator_map.end(), ("generator function for dispatcher type %d not found", static_cast<t_uint32>(dispatcher_type)));
  result = dispatcher_type_to_generator_map[dispatcher_type];

  /* bookkeeping */
  auto target = result.target<DispatcherResult (*)(F_DispatchGeneratorArguments)>();

  /* are we using a conditional jump dispatcher of any kind? */
  if (*target == ApplyInternalConditionalJumpDispatcher
      || *target == ApplyConditionalJumpDispatcher)
    using_conditional_jump = true;

  if (*target == ApplySwitchDispatcherBranchTable)
    added_trampoline = true;

  return result;
}

void PropagateBblFlags(t_bbl *start) {
  if (!BblGetAFFlag(start, AF_FLAG_DIRTY_SP))
    return;

  FunctionSet functions;

  /* propagate the flag */
  t_bbl *bbl = start;
  t_cfg_edge *followed_edge = nullptr;
  t_bbl *continuation = nullptr;
  TransformedSliceInformation *slice_information = nullptr;
  while (!FUNCTION_IS_AF(BBL_FUNCTION(bbl))) {
    if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) != NULL)
      __DumpDots(BBL_CFG(start), "boem", 0);
    ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) == NULL, ("expected only one outgoing edge @eiB", bbl));

    BblSetAFFlag(bbl, AF_FLAG_DIRTY_SP);

    followed_edge = BBL_SUCC_FIRST(bbl);
    if (CfgEdgeIsAF(followed_edge)) {
      continuation = CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(followed_edge));
      slice_information = CFG_EDGE_SLICE_INFORMATION(followed_edge);
    }
    bbl = CFG_EDGE_TAIL(followed_edge);
  }

  /* the AF function */
  functions.insert(BBL_FUNCTION(bbl));

  /* after the AF function */
  ASSERT(continuation != nullptr, ("expected continuation point! @eiB", start));
  BblVector worklist = {continuation};

  while (worklist.size() > 0) {
    t_bbl *subject = worklist.back();
    worklist.pop_back();

    bool can_continue = true;
    if (subject == continuation && slice_information->register_info[ARM_REG_R13] != REGISTER_INFO_UNMODIFIED)
      can_continue = false;

    if (can_continue) {
      if (BblGetAFFlag(subject, AF_FLAG_DIRTY_SP | AF_FLAG_SP_ALMOST_OK))
        continue;

      if (RegsetIn(BBL_REGS_DEF(subject), ARM_REG_R13)) {
        if (!BblGetAFFlag(subject, AF_FLAG_DIRTY_SP))
          BblSetAFFlag(subject, AF_FLAG_SP_ALMOST_OK);
        continue;
      }
    }

    BblSetAFFlag(subject, AF_FLAG_DIRTY_SP);

    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(subject, e) {
      if (CfgEdgeIsAF(e))
        worklist.push_back(CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(e)));

      t_bbl *b = CFG_EDGE_TAIL(e);

      if (BBL_IS_HELL(b))
        continue;

      if (FUNCTION_IS_AF(BBL_FUNCTION(b))) {
        /* this is an AF function */
        functions.insert(BBL_FUNCTION(b));
      }
      else
        worklist.push_back(b);
    }
  }

  for (auto fun : functions)
    FUNCTION_FOREACH_BBL(fun, bbl)
      BblSetAFFlag(bbl, AF_FLAG_DIRTY_SP);
}

FactoringResult TransformFactoringPossiblity(FactoringPossibility *poss)
{
  STATUS(START, ("single transformation"));

  int dump_dots = -1;

  static int nr = 0;
  FactoringResult result = FactoringResult();

  auto nr_transformed_slices = nr_total_slices;
  SliceSet slices_to_transform;

  current_af_phase = AFPhase::Unknown;

  /* always first insert the reference slice */
  slices_to_transform.insert(poss->ref_slice);

  for (auto slice : poss->set) {
    if (nr_total_slices + slices_to_transform.size() < static_cast<t_uint32>(DEBUGCOUNTER_VALUE))
      slices_to_transform.insert(slice);
  }

  poss->set = slices_to_transform;

  t_cfg *cfg = SliceSetCfg(poss->set);

  START_LOGGING_TRANSFORMATION_NONEWLINE(L_FACTORING, "AdvancedFactoring,");
  all_af_tf_ids.insert(GetTransformationNumberFromId(GetTransformationId()));

  VERBOSE(0, ("transforming %s set of %u slices of size %d and score %s (did %u/%u slices so far) A:%d(%d) O:%d(%d) F:%d(%d)", GetTransformationIdString().c_str(), poss->set.size(), poss->slice_size, poss->score.to_string().c_str(), nr_total_slices, nr_candidate_slices, poss->score.data.source_info.archives.size(), poss->score.data.source_info.exec_archives.size(), poss->score.data.source_info.objects.size(), poss->score.data.source_info.exec_objects.size(), poss->score.data.source_info.functions.size(), poss->score.data.source_info.exec_functions.size()));
#ifdef DUMP_TRANSFORMED_SETS
  SliceSetPrint(slices_to_transform, "    TF", poss->slice_size);
#endif
  for (auto slice : slices_to_transform)
    VERBOSE(0, (AF "Slice: %c %c @I", (slice->is_sequence ? 'S' : 's'), (slice->IsExecuted() ? '*' : ' '), slice->base_instruction));
  VERBOSE(1, ("  with @X/untouch %s", CPREGSET(cfg, poss->usable_regs), GPRegistersPrint(NULL, poss->preferably_dont_touch).c_str()));

  /* check for overlapping slices */

  /* We don't want to factor slices that have overlapping instructions.
   * However, we should only look at the instructions that will be factored,
   * and not the slices as a whole. To check this, we construct a set of
   * instructions and iteratively add the instructions of the next slices. */
  for (auto slice : slices_to_transform)
  {
    size_t max_insns = (poss->slice_size == 0) ? slice->NrInstructions() : poss->slice_size;
    if (max_insns > slice->NrInstructions())
      CfgDrawFunctionGraphs(cfg, "what");
    ASSERT(max_insns <= slice->NrInstructions(), ("what? %d/%d/%d %p %s", max_insns, poss->slice_size, slice->NrInstructions(), poss, slice->Print().c_str()));

    slice->PrecalculateCombineData(poss->slice_size);
    VERBOSE(1, ("overwritten registers for slice with base %c @I: %s", (slice->IsExecuted() ? '*' : ' '), slice->base_instruction, GPRegistersPrint(NULL, poss->overwritten_registers_per_slice[slice]).c_str()));
  }

  result.nr_factored_insns = poss->slice_size * poss->set.size();

  /* keep a list of the factored code paths */
  vector<FactoredPath> factored_paths;

  t_regset new_live = RegsetNew();

  /* sometimes a register used for internal conditional jump may not be overwritten by the jump itself.
   * Thus, such a register can be used even if it is live. But this register should of course not be considered
   * when looking for equalisation registers. */
  t_gpregisters gpregs = poss->preferably_dont_touch;
  GPRegistersSetIntersectRegset(gpregs, poss->usable_regs);

  vector<t_reg> usable_registers_primary = RegsetToVector(RegsetDiffGPRegisters(poss->usable_regs, gpregs));
  vector<t_reg> usable_registers_secondary = GPRegistersToVector(gpregs);

  vector<t_reg> virtual_to_real;
  for (int i = 0; i < poss->nr_imm_to_reg; i++) {
    t_reg reg = ChooseRegister(usable_registers_primary);
    if (reg == ARM_REG_NONE)
      reg = ChooseRegister(usable_registers_secondary);
    ASSERT(reg != ARM_REG_NONE, ("not enough registers!"));

    virtual_to_real.push_back(reg);
    RegsetSetAddReg(new_live, reg);
    GPRegistersSetSubReg(poss->preferably_dont_touch, reg);
  }

  t_regset still_available = RegsetNew();
  for (t_reg r : usable_registers_primary)
    RegsetSetAddReg(still_available, r);
  for (t_reg r : usable_registers_secondary)
    RegsetSetAddReg(still_available, r);
  RegsetSetDiff(still_available, new_live);

  vector<t_reg> usable_registers = RegsetToVector(still_available);

  /* first of all, we need to create the factored BBL.
   * We do this by looking duplicating a reference slice into a new function. */
  Slice *ref_slice = poss->ref_slice;
  if (ref_slice == nullptr) {
    /* no reference slice has been selected, just take the first one */
    ref_slice = *(slices_to_transform.begin());
  }
  if (slices_to_transform.find(ref_slice) == slices_to_transform.end())
    ref_slice = *slices_to_transform.begin();

  /* need to know whether two slices are in the same BBL or not.
   * if so, we need to know which slice is executed before the other. */
  BblToSliceVectorMap bbl_to_slicevector_map;
  /* SPEEDUP: no need to iterate over the previous variable every time we need to find
   * which BBL is associated with any slice. This uses a little more memory, but saves
   * us a loop during the internal conditional jump dispatcher generation. */
  SliceToBblMap slice_to_bbl_map;

  /* we need to do this here because we want to associate the ORIGINAL
   * slice BBLs with the slices.  */
  for (auto slice : slices_to_transform) {
    bbl_to_slicevector_map[slice->Bbl()].push_back(slice);
    slice_to_bbl_map[slice] = slice->Bbl();
  }

#ifdef ALREADY_TRANSFORMED_NR_DOTS
  if (nr_total_slices == ALREADY_TRANSFORMED_NR_DOTS)
    dump_dots = ALREADY_TRANSFORMED_NR_DOTS_ID;
#endif

  if (dump_dots >= 0)
    DumpDots(cfg, "beforetfsplit", dump_dots);

  current_af_phase = AFPhase::CreateFactoredBlock;

  t_bbl *factored_bbl = NULL;
  BblSet merged_bbls;
  t_uint64 total_exec_count = 0;
  size_t slice_nr = 0;
  for (auto slice : slices_to_transform) {
    /* make sure the slice is one atomic block inside its basic block */
    AtomizeSliceInBbl(slice, poss->slice_size, slice_nr);

    SliceSplitOff(slice, poss->slice_size);

    t_cfg_edge *e;

    /* logging */
    t_arm_ins *ins;
    BBL_FOREACH_ARM_INS(slice->Bbl(), ins) {
      string s = "FACTORED:" + to_string(slice_nr);
      AFFactoringLogInstruction(ins, s);
    }

    /* pre */
    BBL_FOREACH_PRED_EDGE(slice->Bbl(), e) {
      t_cfg_edge *ee = e;
      if (CFG_EDGE_CAT(e) == ET_RETURN) {
        if (CFG_EDGE_CORR(e))
          ee = CFG_EDGE_CORR(e);
        else
          continue;
      }
      else if (CfgEdgeIsInterproc(ee))
        continue;

      t_arm_ins *ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(ee)));
      if (!ins || AddressIsNull(ARM_INS_OLD_ADDRESS(ins)))
        continue;

      string s = "FACTOREDPRE:" + to_string(slice_nr);
      AFFactoringLogInstruction(ins, s);
    }

    /* post */
    BBL_FOREACH_SUCC_EDGE(slice->Bbl(), e) {
      t_cfg_edge* ee = e;
      if (CFG_EDGE_CAT(e) == ET_CALL) {
        if (CFG_EDGE_CORR(e))
          ee = CFG_EDGE_CORR(e);
        else
          continue;
      }
      else if (CfgEdgeIsInterproc(ee))
        continue;

      t_arm_ins *ins = T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(ee)));
      if (!ins || AddressIsNull(ARM_INS_OLD_ADDRESS(ins)))
        continue;

      string s = "FACTOREDPOST:" + to_string(slice_nr);
      AFFactoringLogInstruction(ins, s);
    }

    slice_nr++;

    if (slice == ref_slice) {
      /* create a factored BBL if it does not exist yet */
      factored_bbl = DuplicateBblForFactoring(ref_slice->Bbl());
      VERBOSE(0, (AF "created new function @F", BBL_FUNCTION(factored_bbl)));

      /* apply the necessary actions on the factored BBL */
      AfFlags af_flags = 0;
      auto actions_result = ApplyActionsToBbl(factored_bbl, poss->actions_on_factored, NULL, virtual_to_real, true, af_flags);
      result.added_ins_info.Merge(actions_result.added_ins_info);

      AfFunctionUpdateFlags(BBL_FUNCTION(factored_bbl), af_flags);
      BblSetAFFlag(factored_bbl, af_flags);

      t_arm_ins *ins;
      BBL_FOREACH_ARM_INS(factored_bbl, ins)
        AFFactoringLogInstruction(ins, "MERGED");
    }

    total_exec_count += BBL_EXEC_COUNT(slice->Bbl());
    merged_bbls.insert(slice->Bbl());
  }

  LOG_MESSAGE(L_FACTORING, "%d,", poss->slice_size);

  BBL_SET_EXEC_COUNT(factored_bbl, total_exec_count);
  /* subtract removed instructions (static) */
  result.added_ins_info.nr_added_insns -= (slices_to_transform.size() - 1) * poss->slice_size;

  if (dump_dots >= 0)
    DumpDots(cfg, "beforetf", dump_dots);

  /* sort the slices to which one will be executed first */
  for (auto pair : bbl_to_slicevector_map) {
    /* 'bbl' is the starting BBL that has been split up before */
    t_bbl *bbl = pair.first;
    /* 'slices' is the list of slices to be sorted */
    SliceVector& slices = pair.second;

    /* sort the slice vector */
    if (slices.size() > 1) {
      sort(slices.begin(), slices.end(), [bbl] (Slice *a, Slice *b) {
        /* return TRUE if a should come before b */
        t_bbl *tmp = bbl;

        while (true) {
          if (a->Bbl() == tmp)
            return false;
          if (b->Bbl() == tmp)
            return true;

          ASSERT(BBL_SUCC_FIRST(tmp) == BBL_SUCC_LAST(tmp)
                  && CFG_EDGE_CAT(BBL_SUCC_FIRST(tmp)) == ET_FALLTHROUGH, ("expected only one outgoing edge for @eiB", tmp));
          tmp = CFG_EDGE_TAIL(BBL_SUCC_FIRST(tmp));
        }

        FATAL(("should not come here"));
      });
    }
  }

  /* process the slice BBLs */
  for (auto slice : slices_to_transform)
  {
    current_af_phase = AFPhase::SliceTransform;

    /* split off the slice so it resides in its own BBL */
    t_bbl *bbl = slice->Bbl();

    /* conditional branch-and-link is a special case that we need to take into account */
    t_cfg_edge *return_edge = NULL;
    BBL_FOREACH_PRED_EDGE(bbl, return_edge)
      if (CFG_EDGE_CAT(return_edge) == ET_RETURN)
        break;

    if (return_edge != NULL) {
      t_cfg_edge *call_edge = CFG_EDGE_CORR(return_edge);
      ASSERT(CFG_EDGE_CAT(call_edge) == ET_CALL, ("expected ET_CALL, got @E @eiB", call_edge, bbl));

      t_arm_ins *call_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(call_edge)));
      ASSERT(ARM_INS_OPCODE(call_ins) == ARM_BL, ("expected branch-and-link @I @eiB", call_ins, bbl));

      if (ArmInsIsConditional(call_ins)) {
        /* incoming FT and FT-like edge */
        t_bbl *split_off = BblSplitBlock(bbl, BBL_INS_FIRST(bbl), TRUE);
        AfterSplit(bbl, split_off);
        UpdateAssociatedSlices(bbl);

        bbl = split_off;
      }
    }

#ifdef DEBUG_OUTPUT
    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e)
      DEBUG(("incoming @eiB", CFG_EDGE_HEAD(e)));
    BBL_FOREACH_SUCC_EDGE(bbl, e)
      DEBUG(("outgoing @eiB", CFG_EDGE_TAIL(e)));
    DEBUG(("====="));
#endif

#ifndef AF_DONT_TRANSFORM
    auto slice_factored_paths = slice->CalculateFactoredPaths(poss->usable_regs);

    /* special case when the slice BBL is the return site of a function call */
    if (BblIsReturnSite(bbl))
    {
      /* split off the slice BBL so the return site is kept in-place */
      t_bbl *split_off = BblSplitBlock(bbl, BBL_INS_FIRST(bbl), TRUE);
      AfterSplit(bbl, split_off);
      UpdateAssociatedSlices(bbl);

      /* as an extra edge is added by splitting the basic block,
       * we need to take this into account for the factored paths. */
      for (auto& path : slice_factored_paths)
        if (CFG_EDGE_TAIL(path.incoming_path.back()) == bbl)
          path.incoming_path.insert(path.incoming_path.end(), BBL_SUCC_FIRST(bbl));

      /* update the reference to 'bbl' */
      bbl = split_off;
    }

    /* split off at the end, for inserting restoration instructions after the slice has been executed */
    /* for this, we assume that the slice BBL can't end in a branch instruction */
    t_bbl *restore_bbl = BblSplitBlock(bbl, BBL_INS_LAST(bbl), FALSE);
    AfterSplit(bbl, restore_bbl);
    BBL_SET_ATTRIB(restore_bbl, BBL_ATTRIB(restore_bbl) | BBL_ADVANCED_FACTORING);

    for (auto& path : slice_factored_paths)
      path.outgoing_edge = BBL_SUCC_FIRST(bbl);

    /* add these paths to the global list */
    factored_paths.insert(factored_paths.end(), slice_factored_paths.begin(), slice_factored_paths.end());

    /* at least one outgoing edge expected */
    ASSERT(BBL_SUCC_FIRST(bbl) != NULL, ("what? no outgoing edge? @eiB", bbl));

    /* only one outgoing edge supported for now */
    ASSERT(BBL_SUCC_FIRST(bbl) == BBL_SUCC_LAST(bbl), ("bbl should only have one outgoing edge! @eiB", bbl));

    /* only non-branch instructions can be last in the slice for now */
    ASSERT(CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_FALLTHROUGH
            || CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_IPFALLTHRU, ("only outgoing fallthrough edges supported! @eiB", bbl));

    /* redirect relocations to this slice BBL so they go to the newly factored BBL */
    RedirectRelocations(bbl, factored_bbl);
#endif
  }

#ifndef AF_DONT_TRANSFORM
  /* for logging purposes only */
  set<t_function *> funcs;
  for (auto path : factored_paths)
  {
    funcs.insert(BBL_FUNCTION(CFG_EDGE_HEAD(path.incoming_path.front())));
    funcs.insert(BBL_FUNCTION(CFG_EDGE_TAIL(path.incoming_path.back())));
    funcs.insert(BBL_FUNCTION(CFG_EDGE_TAIL(path.outgoing_edge)));
  }

  for (auto f : funcs)
    LOG_MORE(L_FACTORING)
      LogFunctionTransformation("before", f);
  LOG_MORE(L_FACTORING)
    LogFunctionTransformation("before", BBL_FUNCTION(factored_bbl));

  /* prepare the factored paths */
  map<Slice *, t_bbl *> slices_to_compensation_blocks;
  for (auto& path : factored_paths)
  {
    t_bbl *compensation_bbl = NULL;

    if (slices_to_compensation_blocks.find(path.slice) == slices_to_compensation_blocks.end()) {
      /* create compensation block */
      compensation_bbl = path.slice->Bbl();

      t_bbl *split_off = BblSplitBlock(compensation_bbl, BBL_INS_FIRST(compensation_bbl), TRUE);
      AfterSplit(compensation_bbl, split_off);
      UpdateAssociatedSlices(compensation_bbl);

      BBL_SET_ATTRIB(compensation_bbl, BBL_ATTRIB(compensation_bbl) | BBL_ADVANCED_FACTORING);

      slices_to_compensation_blocks[path.slice] = compensation_bbl;
    }
    else
      compensation_bbl = slices_to_compensation_blocks[path.slice];

    t_bbl *old = CFG_EDGE_TAIL(path.incoming_path.front());
    path.incoming_path.insert(path.incoming_path.end(), BBL_SUCC_FIRST(compensation_bbl));

    //TODO: check if this actually ever does something...
    RedirectRelocations(old, compensation_bbl);

    path.assumption.value = RegisterValue::Unknown;
  }

  /* overwritten registers */
  for (auto pair : poss->overwritten_registers_per_slice) {
    Slice *slice = pair.first;
    t_gpregisters gpregs = pair.second;

    t_function *fun = BBL_FUNCTION(slice->Bbl());
    for (auto reg : GPRegistersToVector(gpregs))
      RecordChangedRegisterInFunction(fun, reg);
  }

  if (dump_dots >= 0)
    DumpDots(cfg, "beforedispatcher", dump_dots);

  /* depending on the chosen strategy, do different things */
  t_bbl *factored_bbl_tail = factored_bbl;
  vector<SwitchcaseData> switchcase_data;

  bool using_conditional_jump = false;
  bool added_trampoline = false;
  DispatcherType dispatcher_type;
  F_DispatchGenerator dispatch_generator = ChooseDispatchGenerator(poss, using_conditional_jump, added_trampoline, dispatcher_type);

  /* generate the dispatch */
  current_af_phase = AFPhase::CreateDispatcher;
  ASSERT(dispatch_generator != nullptr, ("no dispatch generator function defined!"));
  t_reg dispatch_register = ARM_REG_NONE;
  auto dispatch_result = dispatch_generator(usable_registers, factored_paths, factored_bbl, switchcase_data, factored_bbl_tail, poss->preferably_dont_touch, poss->slice_size, bbl_to_slicevector_map, slice_to_bbl_map, dispatch_register, poss->flags);
  result.added_ins_info.Merge(dispatch_result.added_ins_info);

  BblSetAFDispatchType(factored_bbl_tail, dispatcher_type);
  BblSetAFDispatchRegister(factored_bbl_tail, dispatch_register);

  /* factored function calculations:
   * - overwritten registers */
  FunctionSetOverwrittenRegisters(BBL_FUNCTION(factored_bbl));

  /* keep track of used registers */
  RegsetSetUnion(new_live, dispatch_result.used_registers);

  /* apply the necessary actions on the predecessors */
  BblVector distributed_worklist;
  set<t_bbl *> already_modified;
  set<t_bbl *> already_modified_after;
  BblSet predecessors;
  bool need_to_create_jump_edge = true;
  bool need_to_create_fallthrough_edge = true;
  t_uint32 unique_id = 0;

  BblSet landing_sites;

  for (auto path : factored_paths)
  {
    /* INCOMING */
    current_af_phase = AFPhase::EqualizePredecessors;

    t_bbl *modify = CFG_EDGE_HEAD(path.incoming_path.back());
    if (already_modified.find(modify) == already_modified.end())
    {
      ASSERT(BBL_ATTRIB(modify) & BBL_ADVANCED_FACTORING, ("not an AF BBL! @eiB", modify));

      /* create an unconditional branch if needed */
      auto res = AddBranchToBblIfNeeded(modify);
      result.added_ins_info.Merge(res);

      /* only apply the actions if the BBL has not been modified already */
      AfFlags af_flags = 0;
      auto actions_result = ApplyActionsToBbl(modify, poss->actions_on_predecessors, path.slice, virtual_to_real, true, af_flags);
      result.added_ins_info.Merge(actions_result.added_ins_info);
      AfFunctionUpdateFlags(BBL_FUNCTION(factored_bbl), af_flags);
      BblSetAFFlag(modify, af_flags);

      for (auto slice_specific_action_list : poss->slice_actions) {
        if (slice_specific_action_list.first != path.slice) continue;

        AfFlags af_flags = 0;
        auto res = ApplyActionsToBbl(modify, slice_specific_action_list.second, path.slice, virtual_to_real, true, af_flags);
        result.added_ins_info.Merge(res.added_ins_info);

        AfFunctionUpdateFlags(BBL_FUNCTION(factored_bbl), af_flags);
        BblSetAFFlag(modify, af_flags);
      }

      predecessors.insert(modify);

      RecordPropagationSource(modify);
      RecordModifiedBbl(modify);
      already_modified.insert(modify);

      /* propagate used register liveness */
      PropagateLivenessActions(modify);
    }
    else {
#ifdef DEBUG_OUTPUT
      DEBUG(("already modified! @iB", modify));
#endif
    }

    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(modify, e)
    {
      t_reloc *rel = CFG_EDGE_REL(e);

      /* only look at edges that have a relocation associated with */
      if (!rel)
        continue;

      /* as the edges will not end in the factored BBL,
       * they rather will point to the modified BBL ('modify'),
       * so we need to redirect the relocations corresponding to these edges! */
      for (t_uint32 i = 0; i < RELOC_N_TO_RELOCATABLES(rel); i++)
        if (RELOC_TO_RELOCATABLE(rel)[i] == T_RELOCATABLE(factored_bbl))
          RelocSetToRelocatable(rel, i, T_RELOCATABLE(modify));
    }

    /* different incoming paths are supported */
    auto last_edge = path.incoming_path.back();

    /* make the edge a factoring one */
    if (!CfgEdgeIsAF(last_edge))
    {
      CfgEdgeMakeAF(last_edge, BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(last_edge)),
#if AF_COPY_ANALYSIS
        BBL_EQS_IN(CFG_EDGE_TAIL(last_edge))
#else
        NULL
#endif
      );
      CFG_EDGE_SET_AF_CORR(last_edge, path.outgoing_edge);

      /* keep some information needed for constant propagation */
      TransformedSliceInformation *new_slice_information = new TransformedSliceInformation();
      new_slice_information->overwritten_registers = poss->overwritten_registers_per_slice[path.slice];
      new_slice_information->register_info = poss->all_register_information_per_slice[path.slice];
      CFG_EDGE_SET_SLICE_INFORMATION(last_edge, new_slice_information);
    }

    CfgEdgeChangeTail(last_edge, factored_bbl);
    EdgeMakeInterprocedural(last_edge);

    /* OUTGOING */
    current_af_phase = AFPhase::EqualizeSuccessors;

    /* old destination, look through the after-compensation BBL */
    t_bbl *old_destination = CFG_EDGE_TAIL(path.outgoing_edge);

    if (DistributedTableCopyIds(CFG_EDGE_HEAD(path.incoming_path.back()), old_destination))
      distributed_worklist.push_back(old_destination);

    /* set of outgoing edges to be made AF */
    set<t_cfg_edge *> edges_to_be_made_af;

    /* ... but for now, only one outgoing path is supported.
     * Check whether this path has already been transformed. */
    //if (CFG_EDGE_HEAD(path.outgoing_edge) != factored_bbl_tail)
    {
      if (using_conditional_jump)
      {
        /* needs to conform to the code in ApplyConditionalJumpDispatcher */
        if (path.index == CONDITIONAL_JUMP_DISPATCHER_JUMP_EDGE)
        {
          /* jump edge */
          CFG_EDGE_SET_CAT(path.outgoing_edge, ET_JUMP);
          need_to_create_jump_edge = false;
        }
        else /* CONDITIONAL_JUMP_DISPATCHER_FALLTHROUGH_EDGE */
        {
          /* fallthrough edge */
          CFG_EDGE_SET_CAT(path.outgoing_edge, ET_FALLTHROUGH);
          need_to_create_fallthrough_edge = false;
        }
      }
      else
      {
        /* make it a switch edge */
        CFG_EDGE_SET_CAT(path.outgoing_edge, ET_SWITCH);
        CFG_EDGE_SET_SWITCHVALUE(path.outgoing_edge, path.index);
      }

      vector<t_cfg_edge *> outgoing_edges = {path.outgoing_edge};
      edges_to_be_made_af.insert(path.outgoing_edge);//NEW

      t_bbl *slice_landing_site = CFG_EDGE_TAIL(path.outgoing_edge);

      if (path.index != -1)
      {
        ASSERT(!switchcase_data[path.index].is_dummy, ("what? dummy entry for index %d", path.index));

        /* we need to associate edges with the relocations, if any */
        if (switchcase_data[path.index].relocs.size() != 0)
        {
          /* refill the vector */
          outgoing_edges.clear();

          for (auto reloc : switchcase_data[path.index].relocs)
          {
#ifdef DEBUG_OUTPUT
            DEBUG(("doing relocation"));
#endif
            t_cfg_edge *e = path.outgoing_edge;
            if (reloc != *(switchcase_data[path.index].relocs.begin()))
            {
              e = CfgEdgeCreate(cfg, factored_bbl_tail, CFG_EDGE_TAIL(path.outgoing_edge), ET_SWITCH);
              CFG_EDGE_SET_EXEC_COUNT(e, BBL_EXEC_COUNT(CFG_EDGE_TAIL(path.outgoing_edge)));
              CFG_EDGE_SET_SWITCHVALUE(e, path.index);
#ifdef DEBUG_OUTPUT
              DEBUG(("  create edge %d", path.index));
#endif
            }

            CFG_EDGE_SET_REL(e, reloc);
            RELOC_SET_SWITCH_EDGE(reloc, e);

            if (!switchcase_data[path.index].is_dummy) {
              outgoing_edges.push_back(e);
              edges_to_be_made_af.insert(e);//NEW
            }
          }
        }

        if (switchcase_data[path.index].bbl != NULL) {
          CfgEdgeChangeTail(path.outgoing_edge, switchcase_data[path.index].bbl);

          if (added_trampoline)
            slice_landing_site = CFG_EDGE_TAIL(BBL_SUCC_FIRST(switchcase_data[path.index].bbl));
        }
      }

      auto out_procstate = CfgEdgeIsAF(path.outgoing_edge) ? CFG_EDGE_PROCSTATE(path.outgoing_edge) : BBL_PROCSTATE_OUT(CFG_EDGE_HEAD(path.outgoing_edge));
      ASSERT(out_procstate, ("no procstate associated with BBL! @eiB", CFG_EDGE_HEAD(path.outgoing_edge)));

#if AF_COPY_ANALYSIS
      auto equations = BBL_EQS_IN(old_destination);
      if (added_trampoline && !equations) {
        t_bbl *look_at = old_destination;

        /* find the needed equations */
        while (!BBL_EQS_IN(look_at)) {
          ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(look_at)) == NULL, ("expected only one outgoing edge @eiB", look_at));
          look_at = CFG_EDGE_TAIL(BBL_SUCC_FIRST(look_at));
        }
        equations = BBL_EQS_IN(look_at);

        /* copy over the equations properly */
        do {
          look_at = CFG_EDGE_HEAD(BBL_PRED_FIRST(look_at));

          BBL_SET_EQS_IN(look_at, EquationsNew(cfg));
          EquationsCopy(cfg, equations, BBL_EQS_IN(look_at));
        } while (look_at != old_destination);
      }
#endif

      int i = 0;
      for (auto e : outgoing_edges)
      {
        CfgEdgeMakeAF(e, out_procstate,
#if AF_COPY_ANALYSIS
          equations
#else
          NULL
#endif
        );
        CFG_EDGE_SET_AF_CORR(e, last_edge);
        if (CFG_EDGE_CORR(e))
          CfgEdgeKill(CFG_EDGE_CORR(e));
        CfgEdgeChangeHead(e, factored_bbl_tail);
        EdgeMakeInterprocedural(e);

        i++;
      }

      /* create outgoing BBL restoration instructions, but only once! */
      if (BBL_PRED_FIRST(slice_landing_site))
        /* bugfix: apparently on bzip2, using dispcondjp dispatchers landing sites without pred's are created */
        landing_sites.insert(slice_landing_site);

      if (already_modified_after.find(slice_landing_site) == already_modified_after.end()) {
        for (auto slice_specific_action_list : poss->slice_actions) {
          if (slice_specific_action_list.first != path.slice) continue;
          if (!BBL_PRED_FIRST(slice_landing_site)) continue;
          if (CfgEdgeIsAF(BBL_PRED_FIRST(slice_landing_site)) && added_trampoline) continue;

          AfFlags af_flags = 0;
          auto res = ApplyActionsToBbl(slice_landing_site, slice_specific_action_list.second, path.slice, virtual_to_real, false, af_flags);
          result.added_ins_info.Merge(res.added_ins_info);

          BblSetAFFlag(slice_landing_site, af_flags);

          BBL_SET_IS_LANDING_SITE(slice_landing_site, true);
        }

        /* sanity check */
        if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(slice_landing_site))) {
          DumpDots(cfg, "boem", 0);
          FATAL(("unexpected multiple outgoing edges! @eiB", slice_landing_site));
        }

        bool possible_backup_init = false;
        t_regset possible_backup_registers = RegsetNew();

        t_reg reg;
        REGSET_FOREACH_REG(AFBblRegsLiveBeforeTail(BBL_SUCC_FIRST(slice_landing_site)), reg) {
          if (reg >= ARM_REG_R15)
            break;

          if (GPRegistersIn(poss->overwritten_registers_per_slice[path.slice], reg)) {
            VERBOSE(0, ("WARNING needed register r%d", reg));

            /* does the live register contain a known constant?
             * If so, get that value and insert a constant producer!
             * If not, the register needs to be backed up BEFORE the slice is executed, and restored afterwards! */
            t_uint64 constant = 0;
            if (ProcstateGetConstantValue(BBL_PROCSTATE_IN(CFG_EDGE_TAIL(BBL_SUCC_FIRST(slice_landing_site))), reg, constant)) {
              t_arm_ins *ins = ProduceConstantInBbl(slice_landing_site, reg, constant, false, true);
              result.added_ins_info.AddInstruction(T_INS(ins));
              VERBOSE(1, ("produced constant @iB", slice_landing_site));
            }
            else {
              VERBOSE(1, ("queue for backup r%d", reg));
              QueueForBackup(slice_landing_site, reg, CFG_EDGE_SLICE_INFORMATION(last_edge));
          }
        }
        }

        already_modified_after.insert(slice_landing_site);
      }
    }
  }

  for (auto bbl : predecessors)
    PropagateBblFlags(bbl);

  /* Keep a list of BBLs that have been used in calculating live-out regsets.
   * We do this because the live-out registers calculated in the loop below depend
   * on the live-out regsets of their successors. Those successors can possibly be
   * the entry points of the most recently factored BBL. */
  BblSet dbbls;

  t_uint32 dummy_idx = 0;
  for (auto d : switchcase_data)
  {
    if (d.is_dummy) {
      /* this refers to a dummy entry */

      if (d.relocs.size() > 0) {
        for (auto reloc : d.relocs)
        {
          t_cfg_edge *e = CfgEdgeCreate(cfg, factored_bbl_tail, d.bbl, ET_SWITCH);
          CFG_EDGE_SET_SWITCHVALUE(e, dummy_idx);
          EdgeMakeInterprocedural(e);
          CfgEdgeMarkFake(e);

          /* associate the relocation with the selected edge */
          CFG_EDGE_SET_REL(e, reloc);
          RELOC_SET_SWITCH_EDGE(reloc, e);

          DummyEdgeData dummy_data;
          dummy_data.dispatch_type = d.dispatch_type;
          GlobalRedirect(e, dummy_data);
        }
      }
      else {
        t_cfg_edge *e = CfgEdgeCreate(cfg, factored_bbl_tail, d.bbl, ET_SWITCH);
        CFG_EDGE_SET_SWITCHVALUE(e, dummy_idx);
        EdgeMakeInterprocedural(e);
        CfgEdgeMarkFake(e);

        DummyEdgeData dummy_data;
        dummy_data.dispatch_type = d.dispatch_type;
        GlobalRedirect(d.landing_edge ? d.landing_edge : e, dummy_data);
      }
    }
    else {
      if (d.bbl) {
        BBL_SET_REGS_LIVE_OUT(d.bbl, RegsLiveOut(d.bbl));
        dbbls.insert(d.bbl);
      }
    }

    dummy_idx++;
  }

  if (using_conditional_jump) {
    if (need_to_create_jump_edge) {
      /* check whether we need to modify an existing edge */
      if (BBL_SUCC_FIRST(factored_bbl) == BBL_SUCC_LAST(factored_bbl)) {
        t_cfg_edge *e = CfgEdgeCreate(cfg, factored_bbl, switchcase_data[0].bbl, ET_JUMP);
        CFG_EDGE_SET_EXEC_COUNT(e, BBL_EXEC_COUNT(switchcase_data[0].bbl));
      }
    }

    if (need_to_create_fallthrough_edge) {
      /* create a new BBL to contain the branch instruction */
      t_bbl *dummy = BblNew(cfg);
      if (BBL_FUNCTION(factored_bbl))
        BblInsertInFunction(dummy, BBL_FUNCTION(factored_bbl));

      BBL_SET_ATTRIB(dummy, BBL_ATTRIB(dummy) | BBL_ADVANCED_FACTORING);

      /* create new BBL */
      t_arm_ins *branch_ins;
      ArmMakeInsForBbl(UncondBranch, Append, branch_ins, dummy, FALSE);
      result.added_ins_info.AddInstruction(T_INS(branch_ins));

      /* jump edge */
      CfgEdgeCreate(cfg, dummy, dummy, ET_JUMP);

      CfgEdgeCreate(cfg, factored_bbl, dummy, ET_FALLTHROUGH);
    }
  }

  /* edge fixups */
  current_af_phase = AFPhase::Fixups;

  if (dump_dots >= 0)
    DumpDots(cfg, "beforefixincoming", dump_dots);

  auto res = FixFactoredBblIncomingEdges(factored_bbl);
  result.added_ins_info.Merge(res);

#ifdef DEBUG_OUTPUT
  DEBUG(("liveness for @eiB", factored_bbl_tail));
#endif

  /* liveness for each path */
  bool keep_going = true;
  while (keep_going) {
    keep_going = false;

    for (t_bbl *landing_site : landing_sites) {
      if (!CfgEdgeIsAF(BBL_PRED_FIRST(landing_site))) {
        ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(landing_site)) == NULL, ("expected BBL to have only one incoming edge!"));
        landing_site = CFG_EDGE_HEAD(BBL_PRED_FIRST(landing_site));
      }

      ASSERT(CfgEdgeIsAF(BBL_PRED_FIRST(landing_site)), ("expected incoming edge to be AF! @eiB", landing_site));
      t_regset live_before = AFFunctionLiveBefore(landing_site);

      t_cfg_edge *e;
      BBL_FOREACH_PRED_EDGE(factored_bbl, e) {
        t_bbl *other = nullptr;
        if (!CfgEdgeIsAF(e)) {
          FATAL(("boem"));
          other = CFG_EDGE_HEAD(e);
          ASSERT(BblIsMarked2(other), ("this should be a marked2 bbl @iB (factored @eiB, edge @E)", other, factored_bbl, e));
          e = BBL_PRED_FIRST(other);
          ASSERT(CfgEdgeIsAF(e), ("should be AF edge @E @iB @eiB", e, other, factored_bbl));
        }

        if (CFG_EDGE_TAIL(CFG_EDGE_AF_CORR(e)) != landing_site)
          continue;

        t_bbl *tmp = CFG_EDGE_HEAD(e);

        /* set live-out and calculate live-in */
        BBL_SET_REGS_LIVE_OUT(tmp, live_before);

        auto propagate_liveness_to_preds = [&predecessors](t_bbl *tmp) {
          BblVector todo_next;

          /* propagate to predecessors */
          t_cfg_edge *e;
          BBL_FOREACH_PRED_EDGE(tmp, e) {
            t_bbl *next = CFG_EDGE_HEAD(e);
            ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(next)) == NULL, ("expected only one outgoing edge @eiB", next));

            BBL_SET_REGS_LIVE_OUT(next, BblRegsLiveBefore(tmp));

            if (predecessors.find(next) == predecessors.end())
              todo_next.push_back(next);
          }

          return todo_next;
        };
      }
    }

    /* last correction for liveness */
    for (auto dbbl : dbbls) {
      t_regset old = BBL_REGS_LIVE_OUT(dbbl);
      BBL_SET_REGS_LIVE_OUT(dbbl, RegsLiveOut(dbbl));

      keep_going |= !RegsetEquals(old, BBL_REGS_LIVE_OUT(dbbl));
    }
  }

  auto backupresults = BackupRegisters(factored_bbl, new_live, added_trampoline, predecessors);
  result.added_ins_info.Merge(backupresults);

#ifdef DUMP_FACTORED_BBL
  DEBUG(("factored @iB", factored_bbl));
#endif
#ifdef DEBUG_OUTPUT
  if (factored_bbl != factored_bbl_tail)
    DEBUG(("     TAIL @iB", factored_bbl_tail));
#endif

  PropagateTableIds(distributed_worklist);

  for (auto f : funcs)
    LOG_MORE(L_FACTORING)
      LogFunctionTransformation("after", f);
  LOG_MORE(L_FACTORING)
    LogFunctionTransformation("after", BBL_FUNCTION(factored_bbl));
#endif

  /* update nonzero information for compensation BBLs */
  if (dump_dots >= 0)
    DumpDots(cfg, "prodbefore", dump_dots);

  /* assuming the non-zero information was updated correctly when splitting BBLs,
   * we don't want to base the calculation of the non-zero regsets on backward analysis for the newly created BBLs.
   * This is because the non-zero information has not been updated correctly yet after the insertion of compensation instructions. */
  BblSet only_forward_nonzero;
  for (auto bbl : already_modified) {
    only_forward_nonzero.insert(bbl);
    recalculate_nonzero(bbl, false);
  }
  for (auto bbl : already_modified_after) {
    only_forward_nonzero.insert(bbl);
    recalculate_nonzero(bbl, false);
  }

  /* constant producing instructions */
  FastProducerPropagation(cfg, false, only_forward_nonzero);
  /* now that the non-zero information has been updated correctly, we can ignore the previously marked BBLs for the next iterations */
  only_forward_nonzero.clear();
  if (dump_dots >= 0)
    DumpDots(cfg, "prodafter", dump_dots);

  /* assumption checks and fixes, if needed */
  auto fix1result = CheckAndFixAssumptions(cfg, true);
  result.added_ins_info.Merge(fix1result);
  /* again, constant producing propagation, but for the newly inserted instructions (if any) */
  FastProducerPropagation(cfg, false, only_forward_nonzero);
  /* assuption checks, but no fixes this time! */
  auto fix2result = CheckAndFixAssumptions(cfg, false);
  result.added_ins_info.Merge(fix2result);

  /* liveness propagation */
  FastProducerPropagation(cfg, true, only_forward_nonzero);

  if (dump_dots >= 0)
    DumpDots(cfg, "livebefore", dump_dots);

  LivenessForAssumptions(cfg);
  ClearAssumptions();

  CheckRegisterMoves();
  ClearRegisterMoves();

  /* final fixups */
  ClearBeforeAfters();
  for (auto modified : already_modified) {
    if (!CfgEdgeIsInterproc(BBL_SUCC_FIRST(modified))
        && !RegsetEquals(BBL_REGS_LIVE_OUT(modified), BblRegsLiveBefore(CFG_EDGE_TAIL(BBL_SUCC_FIRST(modified))))) {
      RecordProducingInstruction(modified, ARM_REG_NONE, ProducedValue{NULL}, false);
    }
  }

  SetLivePropIncludeStart(true);
  FastProducerPropagation(cfg, true, only_forward_nonzero);
  SetLivePropIncludeStart(false);

  /* obfuscation information */
  AfFunctionUpdateFlagsWhole(BBL_FUNCTION(factored_bbl));

  if (dump_dots >= 0)
    DumpDots(cfg, "transformation", dump_dots);

  nr++;

  STOP_LOGGING_TRANSFORMATION(L_FACTORING);
  STATUS(STOP, ("single transformation"));

  return result;
}

static I2SliceSetMap GroupSlicesByImmediate(SliceSet set, size_t idx)
{
  I2SliceSetMap result;

  for (auto slice : set)
  {
    t_arm_ins *ins = T_ARM_INS(slice->GetR(idx));
    if (!ArmInsHasImmediateOp(ins))
      break;

    result[ARM_INS_IMMEDIATE(ins)].insert(slice);
  }

  return result;
}

size_t GroupImmediatesPerInstruction(vector<ImmSliceSetTuple>* data, SliceSet set, size_t slice_size, bool* consider_immediate_for_removal, bool init)
{
  size_t nr_different_imms = 0;

  for (size_t ins_idx = 0; ins_idx < slice_size; ins_idx++)
  {
    auto imm2setmap = GroupSlicesByImmediate(set, ins_idx);

    /* clear the immediates (of previous calls to this function) */
    data[ins_idx].clear();

    /* store the data */
    for (auto it = imm2setmap.begin(); it != imm2setmap.end(); it++)
      data[ins_idx].push_back(ImmSliceSetTuple{it->first, it->second});

    if (init)
      consider_immediate_for_removal[ins_idx] = imm2setmap.size() > 1;

    if (imm2setmap.size() > 1)
      nr_different_imms++;
  }

  return nr_different_imms;
}

t_possibility_flags DeterminePossibleTransformations(SliceSet& set, size_t slice_size, size_t max_regs_needed, t_regset& dead_through, SliceSpecificRegisters slice_registers, t_gpregisters preferably_dont_touch, int& nr_used_internal_constants /* only for icondjp! */)
{
  t_possibility_flags result = 0;

  /* we don't want to factor sets that have slices originating from the same BBL */
  std::set<t_uint32> ids;
  for (auto slice : set) {
    if (ids.find(BBL_ORIGINAL_ID(slice->Bbl())) != ids.end())
      return result;

    ids.insert(BBL_ORIGINAL_ID(slice->Bbl()));
  }

  /* we're looking for commonly dead registers... */
  dead_through = SliceSetCalculateRegsDeadThrough(set, slice_size);

  /* ... that aren't used by register equalisation instructions */
  for (auto used_registers_pair : slice_registers)
    RegsetSetDiff(dead_through, used_registers_pair.second);

  /* now 'dead_through' contains the registers we can possibly use */
  auto nr_regs_available = RegsetCountRegs(dead_through);

  if (nr_regs_available >= max_regs_needed)
  {
    if (diabloanoptarm_options.af_enable_dispicondjp) {
      /* we have just enough registers available,
       * maybe we can use a compare operation, followed by a conditional branch */
      bool possible = true;

      for (auto slice : set)
      {
        /* check whether the status flags are dead here */
        if (!RegsetIsEmpty(RegsetIntersect(SliceRegsLiveAfter(slice, slice_size), status_registers))) {
          possible = false;
          break;
        }
      }

      vector<t_reg> usable_registers_secondary;
      if (possible) {
        t_gpregisters new_preferably_dont_touch = preferably_dont_touch;
        GPRegistersSetIntersectRegset(new_preferably_dont_touch, dead_through);

        vector<t_reg> usable_registers_primary = RegsetToVector(RegsetDiffGPRegisters(dead_through, new_preferably_dont_touch));
        usable_registers_secondary = GPRegistersToVector(new_preferably_dont_touch);

        for (size_t i = 0; i < max_regs_needed; i++) {
          t_reg reg = ChooseRegister(usable_registers_primary);
          if (reg == ARM_REG_NONE)
            reg = ChooseRegister(usable_registers_secondary);

          if (reg == ARM_REG_NONE) {
            possible = false;
            break;
          }
        }
      }

      if (possible) {
        possible = false;

        preferably_dont_touch = GPRegistersEmpty();
        for (t_reg r : usable_registers_secondary) {
          GPRegistersAdd(preferably_dont_touch, r);
          possible = true;
        }
      }

      Slice *null_slice;
      t_reg subject;
      int p0, p1;
      if (possible)
        possible = ChooseInternalConditionalJumpDispatcherRegister(set, slice_size, preferably_dont_touch, false, null_slice, subject, p0, p1, nr_used_internal_constants);

      if (!GPRegistersIsEmpty(preferably_dont_touch) && possible)
        result |= AF_POSSIBILITY_INTERNAL_CONDITIONALJUMP;
    }
  }

  if (nr_regs_available >= max_regs_needed + 1)
  {
    /* we have an additional register available, so an indirect jump is possible */
    if (diabloanoptarm_options.af_enable_dispib)
      result |= AF_POSSIBILITY_INDIRECTBRANCH;

    /* ... or any switch construct in which the table immediately follows the switch instruction */
    bool switch_extended_check = false;
    if (diabloanoptarm_options.af_enable_dispswb) {
      result |= AF_POSSIBILITY_SWITCHBRANCH;
      switch_extended_check = true;
    }
    if (diabloanoptarm_options.af_enable_dispswo) {
      result |= AF_POSSIBILITY_SWITCHOFFSET;
      switch_extended_check = true;
    }
    if (switch_extended_check) {
      if (diabloanoptarm_options.af_stealthy_switches) {
        /* possible to insert bounds check? */
        bool status_dead = true;
        for (auto slice : set) {
          if (!RegsetIsEmpty(RegsetIntersect(SliceRegsLiveAfter(slice, slice_size), status_registers))) {
            status_dead = false;
            break;
          }
        }

        if (status_dead) {
          /* status flags are dead */
          result |= AF_POSSIBILITY_BOUNDS_CHECK;
        }
        else {
          /* status flags are live */
          if (diabloanoptarm_options.af_stealthy_switches_optional) {
            /* we don't *need* to generate a bounds check */
          }
          else {
            /* we *must* generate a bounds check, so this candidate is impossible */
            result &= ~(AF_POSSIBILITY_SWITCHBRANCH | AF_POSSIBILITY_SWITCHOFFSET);
          }
        }
      }
    }

    /* but maybe also via a conditional jump (an additional register is needed to put a constant in)? */
    if (diabloanoptarm_options.af_enable_dispcondjp && set.size() <= 2 /* TODO maybe support bigger sets with same final destination */) {
      bool conditional_jump_possible = true;
      for (auto slice : set)
      {
        /* check whether the status flags are dead here */
        if (!RegsetIsEmpty(RegsetIntersect(SliceRegsLiveAfter(slice, slice_size), status_registers)))
          conditional_jump_possible = false;

        /* early exit if needed */
        if (!conditional_jump_possible)
          break;
      }

      if (conditional_jump_possible) {
        /* TODO: check whether possible or not */
        result |= AF_POSSIBILITY_CONDITIONALJUMP;
      }
    }
  }

  if (diabloanoptarm_options.af_enable_dispdisttbl) {
    if (nr_regs_available >= max_regs_needed + 2)
    {
      bool tables_ok = true;
      for (auto slice : set)
        if (BblAssociatedTableIdCount(slice->Bbl()) == 0
            || !SliceCanTransformWithDistributedTable(slice))
        {
          tables_ok = false;
          break;
        }

      if (tables_ok)
        result |= AF_POSSIBILITY_DISTRIBUTED;
    }
  }

  return result;
}

t_gpregisters PossibleRegistersCmpZero(Slice *a, Slice *b, size_t slice_size, bool use_cached) {
  if (!use_cached) {
    a->PrecalculateCombineData(slice_size);
    b->PrecalculateCombineData(slice_size);
  }

  t_gpregisters result = a->can_contain_z;
  GPRegistersSetAnd(result, b->can_contain_nz);
  GPRegistersSetDiffRegset(result, a->overwritten_registers);
  GPRegistersSetDiffRegset(result, b->overwritten_registers);

  return result;
}

t_gpregisters CalculatePreferablyDontTouchRegistersCmpZero(SliceSet slices, size_t slice_size, bool use_cached) {
  t_gpregisters result = GPRegistersEmpty();

  /* the status flags need to be overwritten for certain dispatchers.
   * Here we calculate whether or not they are dead after the slices in the set. */
  bool status_flags_dead = SliceSetStatusFlagsDead(slices, slice_size, use_cached);

  /* if this is a pair of slices, maybe we can use a compare */
  if (slices.size() == 2) {
    Slice *a = *(slices.begin());
    Slice *b = *(next(slices.begin()));

    t_gpregisters ab = PossibleRegistersCmpZero(a, b, slice_size, use_cached);
    if (!GPRegistersIsEmpty(ab) && status_flags_dead)
      GPRegistersSetUnion(result, ab);

    t_gpregisters ba = PossibleRegistersCmpZero(b, a, slice_size, use_cached);
    if (!GPRegistersIsEmpty(ba) && status_flags_dead)
      GPRegistersSetUnion(result, ba);
  }

  return result;
}
