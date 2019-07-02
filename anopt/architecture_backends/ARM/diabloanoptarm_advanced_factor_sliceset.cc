#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

void SliceSetRemoveInvalids(SliceSet& set, size_t slice_size)
{
  for (auto it = set.begin(); it != set.end(); )
  {
    auto slice = *it;
    if (slice->IsInvalidated()
        || (slice->NrInstructions() < slice_size)
        || BblHasRelocationFromIns(slice->Bbl()))
      it = set.erase(it);
    else
      it++;
  }
}

FactoringSetSourceInformation SliceSetGetSourceInformation(SliceSet& slice_set, int& nr_executed) {
  BblSet bbls;
  for (auto slice : slice_set) {
    if (slice->IsInvalidated())
      continue;
    bbls.insert(slice->Bbl());
  }

  return FactoringGetSourceInformation(bbls, nr_executed);
}

bool SliceSetConsiderForFactoring(SliceSet slice_set, size_t slice_size)
{
  /* need to have at least two slices to factor */
  if (slice_set.size() < 2)
    return false;

  /* need to meet the required minimal block length */
  if (slice_size < static_cast<size_t>(diabloanoptarm_options.af_min_block_length))
    return false;

	bool only_master = true;
	bool only_slave = true;

  bool at_least_one_executed = false;
  bool at_least_one_exec_master = false;
  bool at_least_one_exec_slave = false;

	int nr_master = 0;
  int nr_exec = 0;

  int nr_executed = 0;
  FactoringSetSourceInformation source_info = SliceSetGetSourceInformation(slice_set, nr_executed);

	for (auto slice : slice_set)
	{
    bool is_exec = slice->IsExecuted();
		bool is_master = slice->IsMaster(slice);

    /* master/slave region tracking */
		if (only_master
				&& !is_master)
			only_master = false;

		if (only_slave
				&& is_master)
			only_slave = false;

    /* executed slice tracking */
    if (is_exec)
    {
      at_least_one_executed = true;
      nr_exec++;

      if (is_master)
        at_least_one_exec_master = true;
      else
        at_least_one_exec_slave = true;
    }

		if (is_master)
			nr_master++;
	}

#ifdef NONCROSSED_SETS_ARE_USELESS
  if (only_master || only_slave)
    return false;
#endif

#ifdef NEED_AT_LEAST_ONE_EXEC_MS
  if (!(at_least_one_exec_master && at_least_one_exec_slave))
    return false;
#endif

#ifdef NEED_AT_LEAST_TWO_EXEC_SLICE
  if (nr_exec < 2)
    return false;
#endif

  bool result = true;

  /* covered */
  if (source_info.functions.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_functions))
    result = false;
  else if (source_info.objects.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_objects))
    result = false;
  else if (source_info.archives.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_archives))
    result = false;

  /* executed */
  else if (source_info.exec_functions.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_functions))
    result = false;
  else if (source_info.exec_objects.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_objects))
    result = false;
  else if (source_info.exec_archives.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_archives))
    result = false;

  return result;
}

void SliceSetPrint(SliceSet set, string prefix, size_t slice_size)
{
  for (auto slice : set) {
    if (slice->IsInvalidated())
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "%s INVALIDATED(%p)", prefix.c_str(), slice));
    else
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "%s %s", prefix.c_str(), slice->Print(slice_size).c_str()));
  }
}

t_cfg *SliceSetCfg(SliceSet set)
{
  if (set.size() == 0)
    return NULL;

  return BBL_CFG((*set.begin())->Bbl());
}

static void AssignSetIdsToSlicesRecursive(SliceInterferenceMap& interferences, int group_id, Slice *slice)
{
  if (slice->group_id != -1)
    return;

  slice->group_id = group_id;

  for (auto s : interferences[slice])
    AssignSetIdsToSlicesRecursive(interferences, group_id, s);
}

static int AssignSetIdsToSlices(SliceInterferenceMap& interferences, int group_id)
{
  for (auto item : interferences)
    if (item.first->group_id == -1)
    {
      AssignSetIdsToSlicesRecursive(interferences, group_id, item.first);
      group_id++;
    }

  return group_id;
}

/* assume partitions[0] is the non-interfering subset */
static vector<SliceSet> FindAllPartitionCombinations(vector<SliceSet>& partitions, size_t slice_size)
{
  vector<SliceSet> result;

  /* TODO: maybe look for N different combinations */
  SliceSet new_set = partitions[0];

  for (size_t i = 1; i < partitions.size(); i++)
  {
    /* choose a slice out of the set with max scoring slices */
    Slice *new_slice = *(partitions[i].begin());

    /* add the chosen one to the new set */
    new_set.insert(new_slice);
  }

  if (SliceSetConsiderForFactoring(new_set, slice_size))
    result.push_back(new_set);

  return result;
}

std::vector<SliceSet> SliceSetCreateNoninterferingSubsets(SliceSet set, size_t slice_size)
{
  vector<SliceSet> result;

  /* key = slice, value = set of interfering slices */
  SliceInterferenceMap interferences;

  /* 1. slices within the same BBL ############################################ */

  /* factorisation of two slices from within the same BBL results in the following problem after the transformation:
   *    insX
   *    insA1
   *    insA2
   *    insB1
   *    insB2
   *    insY
   *
   *    insX --> insA1, insA2 --> insB1, insB2 --> insY
   *  Factorisation: insX --> factored ({insA1, insA2}, {insB1, insB2}) --> insY
   *  in slice A an address producer to slice B should be produced.
   *  This contradicts the factorisation transformation principle. */
  map<t_bbl *, SliceSet> bbl_to_sliceset;
  for (auto slice : set)
  {
    bbl_to_sliceset[slice->Bbl()].insert(slice);

    /* reset the partition id */
    slice->group_id = -1;
  }

  for (auto e : bbl_to_sliceset)
  {
    auto interfering = e.second;

    for (auto slice : interfering)
    {
      interferences[slice].insert(interfering.begin(), interfering.end());
      interferences[slice].erase(slice);
    }
  }
  /* ########################################################################## */

  /* mark the non-interfering slices */
  for (auto e : interferences)
  {
    if (e.second.size() == 0)
      /* group ID = 0 for non-interfering slices */
      (e.first)->group_id = 0;
  }

  /* partition the original sliceset based on the calculated interferences */
  auto nr_groups = AssignSetIdsToSlices(interferences, 1);
  vector<SliceSet> partitions(nr_groups);
  for (auto slice : set)
    partitions[slice->group_id].insert(slice);

  /* combine elements from the different partitions */
  if (nr_groups > 1)
    result = FindAllPartitionCombinations(partitions, slice_size);
  else
    result.push_back(partitions[0]);

  return result;
}

void PrintFullSliceSetInformation(SliceSet& set, size_t slice_size, bool include_cached)
{
  /* first calculate global information */
  vector<t_regset> slice_dead_through;
  vector<t_regset> slice_nonzero_in;
  vector<t_procstate *> slice_procstate_before;

  /* cached information */
  vector<t_regset> slice_tag_before;
  vector<t_regset> slice_null_before;
  vector<t_regset> slice_dead_before;
  vector<t_regset> slice_nonzero_before;
  vector<t_regset> slice_constant_before;
  vector<t_gpregisters> slice_canzero;
  vector<t_gpregisters> slice_cannonzero;

  int i = 0;
  for (auto slice : set)
  {
    VERBOSE(0, ("slice contains %d instructions, base @I", slice_size, slice->base_instruction));

    slice_dead_through.push_back(SliceRegsDeadThrough(slice, slice_size));
    slice_nonzero_in.push_back(SliceRegsNonZeroIn(slice, slice_size));
    slice_procstate_before.push_back(SliceProcstateBefore(slice, slice_size));

    if (include_cached) {
      slice_tag_before.push_back(slice->tag_before);
      slice_null_before.push_back(slice->null_before);
      slice_dead_before.push_back(slice->dead_before);
      slice_nonzero_before.push_back(slice->nonzero_before);
      slice_constant_before.push_back(slice->constant_before);
      slice_canzero.push_back(slice->can_contain_z);
      slice_cannonzero.push_back(slice->can_contain_nz);
    }

    i++;

#if AF_COPY_ANALYSIS
    if (BBL_EQS_IN(slice->Bbl()))
    {
      VERBOSE(0, ("INCOMING EQUATIONS for @iB", slice->Bbl()));
      EquationsPrint(SliceSetCfg(set), BBL_EQS_IN(slice->Bbl()));
    }
#endif
  }

  /* process copy analysis information */

  for (t_reg reg = ARM_REG_R0; reg < ARM_REG_R15; reg++)
  {
    stringstream ss, ss_cst, ss_copy;

    int i = 0;

    for (auto slice : set)
    {
      /* constant information */
      t_procstate *procstate = slice_procstate_before[i];

      t_register_content c;
      auto reg_level = ProcStateGetReg(procstate, reg, &c);
      if (reg_level != CP_BOT
          && reg_level != CP_TOP)
      {
        char s[2];
        s[1] = '\0';

        /* 'c' = constant */
        s[0] = '0';

        t_reloc *rel;
        auto tag_level = ProcStateGetTag(procstate, reg, &rel);
        if (tag_level != CP_BOT
            && tag_level != CP_TOP)
          s[0] = 't';
        else if (!AddressIsEq(c.i, AddressNew32(0)))
          s[0] = 'c';
        ss << s;
        ss_cst << hex << AddressExtractUint32(c.i) << " ";
      }
      else
      {
        ss << ".";
        ss_cst << ". ";
      }

#if AF_COPY_ANALYSIS
      t_cfg *cfg = SliceSetCfg(set);

      /* copy analysis information */
      auto equations = BBL_EQS_IN(slice->Bbl());
      ASSERT(equations, ("no equations for slice %s in @eiB", slice->Print().c_str(), slice->Bbl()));

      /* look through the equations for one which as base register the current register */
      bool ok = false;
      for (t_reg i = 0; i < ARM_REG_R15; i++)
      {
        /* skip invalid equations */
        if (EquationIsTop(equations[i]) || EquationIsBot(equations[i]))
          continue;

        /* normalize this equation */
        auto norm_eq = EquationNormalize(&equations[i], i);
        if (static_cast<t_reg>(norm_eq.rega) == reg)
        {
          if (ok)
            ss_copy << ",";
          ok = true;

          bool printed = false;
          if (norm_eq.regb != CFG_DESCRIPTION(SliceSetCfg(set))->num_int_regs)
          {
            /* regb present */
            ss_copy << "r" << dec << norm_eq.regb;
            printed = true;
          }

          if (norm_eq.constant != 0)
          {
            if (printed)
              ss_copy << '+';
            ss_copy << hex << norm_eq.constant;
            printed = true;
          }

          if (norm_eq.taga || norm_eq.tagb)
          {
            if (printed)
              ss_copy << '+';

            ss_copy << "(";
            if (norm_eq.taga)
              ss_copy << hex << norm_eq.taga;
            if (norm_eq.tagb)
              ss_copy << '-' << hex << norm_eq.tagb;
            ss_copy << ')';
          }
        }
      }

      if (!ok)
        ss_copy << ".";
      ss_copy << "; ";
#endif

      /* dead register information */
      if (RegsetIn(slice_dead_through[i], reg))
        ss << "d";
      else
        ss << ".";

      /* non-zeroness of the register */
      if (RegsetIn(slice_nonzero_in[i], reg))
        ss << "n";
      else
        ss << ".";

      ss << " | ";

      if (include_cached) {
        if (RegsetIn(slice_dead_before[i], reg))
          ss << "d";
        else
          ss << ".";

        if (RegsetIn(slice_null_before[i], reg))
          ss << "0";
        else
          ss << ".";

        if (RegsetIn(slice_nonzero_before[i], reg))
          ss << "n";
        else
          ss << ".";

        if (RegsetIn(slice_tag_before[i], reg))
          ss << "t";
        else
          ss << ".";

        if (RegsetIn(slice_constant_before[i], reg))
          ss << "c";
        else
          ss << ".";

        if (GPRegistersIn(slice_canzero[i], reg))
          ss << "Z";
        else
          ss << ".";

        if (GPRegistersIn(slice_cannonzero[i], reg))
          ss << "N";
        else
          ss << ".";

        ss << " | ";
      }

      i++;
    }

    VERBOSE(0, ("R%d%s: %s # %s # %s",
                reg, (reg < ARM_REG_R10) ? " " : "",
                ss.str().c_str(), ss_cst.str().c_str(), ss_copy.str().c_str()));
  }

  for (auto procstate : slice_procstate_before)
    ProcStateFree(procstate);
}

t_regset SliceSetCalculateRegsDeadThrough(SliceSet& set, size_t slice_size)
{
  t_regset dead_through = SliceRegsDeadThrough(*(set.begin()), slice_size);
  for (auto it = next(set.begin()); it != set.end(); it++)
    RegsetSetIntersect(dead_through, SliceRegsDeadThrough(*it, slice_size));

  return dead_through;
}

bool SliceSetOverlaps(SliceSet slices, size_t slice_size)
{
  bool result = false;

  set<t_ins *> instructions;

  for (auto slice : slices) {
    for (size_t i = 0; i < slice_size; i++) {
      t_ins *ins = slice->GetR(i);
      if (instructions.find(ins) == instructions.end())
        instructions.insert(ins);
      else {
        result = true;
        break;
      }
    }
  }

  return result;
}

bool SliceSetStatusFlagsDead(SliceSet& set, size_t slice_size, bool use_cached) {
  bool result = true;

  for (auto slice : set) {
    t_regset live_after = slice->live_after;
    if (!use_cached)
      live_after = SliceRegsLiveAfter(slice, slice_size);

    if (!RegsetIsEmpty(RegsetIntersect(live_after, status_registers))) {
      result = false;
      break;
    }
  }

  return result;
}
