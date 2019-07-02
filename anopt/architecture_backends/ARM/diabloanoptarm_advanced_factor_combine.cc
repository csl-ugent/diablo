#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

static inline
size_t UpperMatrixIndexOf(size_t r, size_t c, size_t dimension)
{
  auto R = min(r, c);
  auto C = max(r, c);

  return (R * dimension) + C;
}

#define DECLARE_BK_MEMBER(T, n) T n; T backup_##n;
#define BK_MEMBER(n) backup_##n = n;
#define RESTORE_MEMBER(n) n = backup_##n;
#define FOR_ALL_INSN(x) for (size_t x = 0; x < slice_size; x++)
struct PseudoSlice
{
  size_t slice_size;
  SliceSet slices;

  /* these members need to be backupped */
  DECLARE_BK_MEMBER(bool *, have_to_equalize_insn);
  DECLARE_BK_MEMBER(t_score, score);
  DECLARE_BK_MEMBER(size_t, min_regs_needed);
  DECLARE_BK_MEMBER(size_t, max_regs_needed);
  DECLARE_BK_MEMBER(t_regset, dead_through);
  DECLARE_BK_MEMBER(t_uint32, nr_regs_available);
  DECLARE_BK_MEMBER(t_uint32, nr_required_regs);
  DECLARE_BK_MEMBER(t_possibility_flags, flags);
  DECLARE_BK_MEMBER(SliceSpecificActionList, slice_actions);
  DECLARE_BK_MEMBER(SliceSpecificRegisters, slice_registers);
  DECLARE_BK_MEMBER(Slice *, ref_slice);
  DECLARE_BK_MEMBER(t_gpregisters, preferably_dont_touch);
  DECLARE_BK_MEMBER(SliceToGPRegistersMap, overwritten_registers_per_slice);
  DECLARE_BK_MEMBER(SliceToAllRegisterInformationMap, all_register_information_per_slice);
  DECLARE_BK_MEMBER(set<FunctionUID>, origin_exec_functions);
  DECLARE_BK_MEMBER(set<SourceFileUID>, origin_exec_objects);
  DECLARE_BK_MEMBER(set<SourceArchiveUID>, origin_exec_archives);
  DECLARE_BK_MEMBER(int, nr_used_constants);

  PseudoSlice(size_t p_slice_size)
  {
    score = t_score();
    slice_size = p_slice_size;
    have_to_equalize_insn = new bool[slice_size];
    backup_have_to_equalize_insn = new bool[slice_size];
    ref_slice = nullptr;
    flags = 0;
    preferably_dont_touch = GPRegistersEmpty();
    overwritten_registers_per_slice.clear();
    all_register_information_per_slice.clear();
    origin_exec_functions.clear();
    origin_exec_objects.clear();
    origin_exec_archives.clear();
    nr_used_constants = 0;
  }

  PseudoSlice(PseudoSlice *p) : PseudoSlice(p->slice_size)
  {
    /* copy over the array */
    FOR_ALL_INSN(i)
      have_to_equalize_insn[i] = p->have_to_equalize_insn[i];

    /* copy over other members */
    slices = p->slices;
    score = p->score;
    min_regs_needed = p->min_regs_needed;
    max_regs_needed = p->max_regs_needed;
    dead_through = p->dead_through;
    nr_regs_available = p->nr_regs_available;
    nr_required_regs = p->nr_required_regs;
    flags = p->flags;
    slice_actions = p->slice_actions;
    slice_registers = p->slice_registers;
    ref_slice = p->ref_slice;
    preferably_dont_touch = p->preferably_dont_touch;
    overwritten_registers_per_slice = p->overwritten_registers_per_slice;
    all_register_information_per_slice = p->all_register_information_per_slice;
    origin_exec_functions = p->origin_exec_functions;
    origin_exec_objects = p->origin_exec_objects;
    origin_exec_archives = p->origin_exec_archives;
    nr_used_constants = p->nr_used_constants;
  }

  ~PseudoSlice()
  {
    delete[] have_to_equalize_insn;
    delete[] backup_have_to_equalize_insn;
  }

  void AddSlice(Slice *s)
  {
    if (s->IsExecuted()) {
      origin_exec_functions.insert(s->origin_function);
      origin_exec_objects.insert(s->origin_object);
      origin_exec_archives.insert(s->origin_archive);
    }
    slices.insert(s);
  }

  void Calculate()
  {
    /* reset */
    slice_actions.clear();
    slice_registers.clear();

    score = t_score();

    if (SliceSetOverlaps(slices, slice_size))
      return;

    /* initial reference slice selection */
    ref_slice = *(slices.begin());

    /* address producer checks */
    for (auto slice : slices) {
      for (size_t ins_idx = 0; ins_idx < slice_size; ins_idx++) {
        t_arm_ins *ins1 = T_ARM_INS(ref_slice->GetR(ins_idx));
        t_arm_ins *ins2 = T_ARM_INS(slice->GetR(ins_idx));

        if (ARM_INS_OPCODE(ins1) != ARM_ADDRESS_PRODUCER)
          continue;

        bool result = CompareAddressProducers(ins1, ins2, true);

        /* special case: slices of size 1 can't have the address producer
         * moved, because it is the slice itself */
        if (slice_size == 1
            && !result) {
          flags = 0;
          return;
        }
      }
    }

    /* there might be some registers that we prefer not to be used.
     * One reason for this could be that, using one or more of these registers,
     * we can implement an additional kind of dispatcher. */
    preferably_dont_touch = CalculatePreferablyDontTouchRegistersCmpZero(slices, slice_size, true);

    /* equalize the immediates */
    nr_required_regs = EqualizeImmediates(slices, slice_size, have_to_equalize_insn);
    flags = DeterminePossibleTransformations(slices, slice_size, nr_required_regs, dead_through, slice_registers, preferably_dont_touch, nr_used_constants);
    if (!Possible())
      return;

    /* imemdiates can be equalized, try to equalize the registers now! */

    /* prepare for register equalization: calculate the information we need to know for
     * possible dispatch generators. This function also tries to be smart when it chooses a reference slice:
     *    try to overwrite as less preferably not touching registers as possible. */
    bool regs_can_be_equalized = EqualizeRegistersPrepare(slices, slice_size, preferably_dont_touch, ref_slice);

    /* eliminate the registers that will be overwritten by the reference slice */
    GPRegistersSetDiffRegset(preferably_dont_touch, ref_slice->overwritten_registers);

    /* _try_ to equalize the registers */
    if (regs_can_be_equalized)
      regs_can_be_equalized = EqualizeRegisters(slices, slice_size, slice_actions, slice_registers, ref_slice, preferably_dont_touch, overwritten_registers_per_slice, all_register_information_per_slice);

    if (regs_can_be_equalized)
    {
      /* based on the previously calculated results, determine the possible transformations */
      flags = DeterminePossibleTransformations(slices, slice_size, nr_required_regs, dead_through, slice_registers, preferably_dont_touch, nr_used_constants);
      nr_regs_available = RegsetCountRegs(dead_through);

      int nr_executed = 0;
      auto source_info = SliceSetGetSourceInformation(slices, nr_executed);
      score = CalculateScoreForSet(slices, slice_size, ScoringData{static_cast<size_t>(nr_required_regs), nr_regs_available, nr_used_constants, nr_executed, flags, source_info, slices.size()});
    }
    else
      flags = 0;
  }

  void Backup()
  {
    BK_MEMBER(score);
    BK_MEMBER(min_regs_needed);
    BK_MEMBER(max_regs_needed);
    BK_MEMBER(dead_through);
    BK_MEMBER(nr_regs_available);
    BK_MEMBER(nr_required_regs);
    BK_MEMBER(flags);
    BK_MEMBER(slice_actions);
    BK_MEMBER(slice_registers);
    BK_MEMBER(ref_slice);
    BK_MEMBER(preferably_dont_touch);
    BK_MEMBER(overwritten_registers_per_slice);
    BK_MEMBER(all_register_information_per_slice);
    BK_MEMBER(origin_exec_functions);
    BK_MEMBER(origin_exec_objects);
    BK_MEMBER(origin_exec_archives);
    BK_MEMBER(nr_used_constants);

    FOR_ALL_INSN(i)
      BK_MEMBER(have_to_equalize_insn[i]);
  }

  void Restore()
  {
    RESTORE_MEMBER(score);
    RESTORE_MEMBER(min_regs_needed);
    RESTORE_MEMBER(max_regs_needed);
    RESTORE_MEMBER(dead_through);
    RESTORE_MEMBER(nr_regs_available);
    RESTORE_MEMBER(nr_required_regs);
    RESTORE_MEMBER(flags);
    RESTORE_MEMBER(slice_actions);
    RESTORE_MEMBER(slice_registers);
    RESTORE_MEMBER(ref_slice);
    RESTORE_MEMBER(preferably_dont_touch);
    RESTORE_MEMBER(overwritten_registers_per_slice);
    RESTORE_MEMBER(all_register_information_per_slice);
    RESTORE_MEMBER(origin_exec_functions);
    RESTORE_MEMBER(origin_exec_objects);
    RESTORE_MEMBER(origin_exec_archives);
    RESTORE_MEMBER(nr_used_constants);

    FOR_ALL_INSN(i)
      RESTORE_MEMBER(have_to_equalize_insn[i]);
  }

  void CopyIn(PseudoSlice *p)
  {
    slices = p->slices;

    score = p->score;
    min_regs_needed = p->min_regs_needed;
    max_regs_needed = p->max_regs_needed;
    dead_through = p->dead_through;
    nr_regs_available = p->nr_regs_available;
    nr_required_regs = p->nr_required_regs;
    flags = p->flags;
    slice_actions = p->slice_actions;
    slice_registers = p->slice_registers;
    ref_slice = p->ref_slice;
    preferably_dont_touch = p->preferably_dont_touch;
    overwritten_registers_per_slice = p->overwritten_registers_per_slice;
    all_register_information_per_slice = p->all_register_information_per_slice;
    origin_exec_functions = p->origin_exec_functions;
    origin_exec_objects = p->origin_exec_objects;
    origin_exec_archives = p->origin_exec_archives;
    nr_used_constants = p->nr_used_constants;

    FOR_ALL_INSN(i)
      have_to_equalize_insn[i] = p->have_to_equalize_insn[i];
  }

  bool Possible()
  {
    if (flags == 0)
      return false;

    return true;
  }
};

static
PseudoSlice *FindBestPair(PseudoSlice **pairs, size_t dimension)
{
  /* look for the best-scoring pair */
  PseudoSlice *best_pair = nullptr;

  for (size_t row = 0; row < dimension; row++)
  {
    for (size_t col = row + 1; col < dimension; col++)
    {
      auto idx = UpperMatrixIndexOf(row, col, dimension);

      /* combination can't be made */
      if (pairs[idx] == nullptr)
        continue;

      /* first valid combination encountered */
      if (best_pair == nullptr
          || (pairs[idx]->score > best_pair->score))
        best_pair = pairs[idx];
    }
  }

  return best_pair;
}

static
bool CanCombineMtx(PseudoSlice **mtx, PseudoSlice *ps, Slice *s, size_t dimension)
{
  for (auto slice : ps->slices)
  {
    auto idx = UpperMatrixIndexOf(slice->my_index, s->my_index, dimension);

    /* is this combination possible? */
    if (mtx[idx] == nullptr)
      return false;
  }

  return true;
}

struct SliceCompareExecuted
{
	bool operator() (const Slice* left, const Slice* right) const {
    /* return TRUE when 'left' is to be put before 'right' */
    if (!(left->IsExecuted()))
	  return left->uid < right->uid;
  }
};

typedef vector<Slice *> SortedSliceVector;

static
PseudoSlice *ExpandSliceSet(PseudoSlice *seed, PseudoSlice **mtx, size_t dimension, vector<Slice *> slices_set, size_t max_set_size)
{
  /* sort slices by executed or not, for quick searches */
  SortedSliceVector slices;

  /* the slices have already been sorted according to their UID,
   * we need to sort this set another time to put the executed slices up front */
  for (auto s : slices_set) {
    if (seed->slices.find(s) == seed->slices.end())
      slices.push_back(s);
  }
  stable_sort(slices.begin(), slices.end(), [] (Slice *x, Slice *y) {
    return x->IsExecuted() && !y->IsExecuted();
  });
  /* now, 'slices' first contains the executed, followed by the non-executed slices */

  /* more slices are available,
   * pick the best slice to add to this combination.
   * By default, the seed set is the best possible combination. */
  PseudoSlice *best = new PseudoSlice(seed);

  /* look for the slice that yields the best score when combined
   * with the existing seed */
  while (slices.size() > 0
          && best->slices.size() < max_set_size)
  {
    Slice *best_slice = nullptr;
    size_t best_position = 0;

    /* look for the best matching slice */
    bool exec_checked = false;
    size_t position = -1;
    for (auto s : slices)
    {
      position++;

      if (diabloanoptarm_options.af_only_executed
          && !s->IsExecuted()) {
        /* we have looked at all the executed slices. Do we already have a valid executed coverage? */

        /* we can exit early when the executed coverage requirements are not met */
        bool ok = (seed->origin_exec_functions.size() >= static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_functions))
                  && (seed->origin_exec_objects.size() >= static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_objects))
                  && (seed->origin_exec_archives.size() >= static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_archives));
        if (!ok) {
          /* exit the FOR loop early */
          break;
        }
      }

      /* check whether this slice can be combined with the rest */
      if (!CanCombineMtx(mtx, seed, s, dimension))
        continue;

      /* try to add this slice to the list */
      seed->Backup();
      seed->AddSlice(s);
      seed->Calculate();

      if (seed->Possible()
          && best->Possible())
      {
        /* did we find a new, better possibility? */
        if (seed->score > best->score)
        {
          /* we have found a new best */
          best->CopyIn(seed);
          best_slice = s;
          best_position = position;
        }
      }

      seed->slices.erase(s);
      seed->Restore();
    }

    /* remove the added slice */
    if (best_slice == nullptr)
      break;
    else
    {
      auto it = slices.begin();
      advance(it, best_position);
      slices.erase(it);
      seed->CopyIn(best);
    }
  }

  return best;
}

static
void MarkAsUsed(PseudoSlice **mtx, size_t dimension, SliceSet set)
{
  for (auto s : set)
  {
    /* only need to iterate over one dimension, because it is a RU matrix */
    for (size_t r = 0; r < dimension; r++)
    {
      auto idx = UpperMatrixIndexOf(r, s->my_index, dimension);

      if (mtx[idx] == nullptr)
        continue;

      delete mtx[idx];
      mtx[idx] = nullptr;
    }
  }
}

/* slice->x is used as a counter to keep track of how many results the slice is in */
static
vector<SliceSetRegsetTuple> LookForSliceCombinationsInternal(vector<Slice *> slices, size_t slice_size)
{
  vector<SliceSetRegsetTuple> result;

  for (auto slice : slices) {
    slice->PrecalculateCombineData(slice_size);
    slice->FixCombineResults();
  }

  auto dimension = slices.size();

  /* initial setup: matrix with pairs of slices.
   * [X][Y] == nullptr -> can't combine X with Y */
  PseudoSlice **pseudoslices_pair = new PseudoSlice *[dimension * dimension];
  for (size_t i = 0; i < dimension; i++)
    for (size_t j = 0; j < dimension; j++)
      pseudoslices_pair[UpperMatrixIndexOf(i, j, dimension)] = nullptr;

  /* construct the initial matrix,
   * to contain the "primitive" pairs of slices */
  size_t row = 0;
  for (auto it = slices.begin(); it != slices.end(); it++, row++)
  {
    /* locally used index for ease of access in the correlation matrix */
    (*it)->my_index = row;

    size_t col = row + 1;
    for (auto itt = next(it); itt != slices.end(); itt++, col++)
    {
      auto idx = UpperMatrixIndexOf(row, col, dimension);

      /* create a combination of two slices */
      pseudoslices_pair[idx] = new PseudoSlice(slice_size);
      pseudoslices_pair[idx]->AddSlice(*it);
      pseudoslices_pair[idx]->AddSlice(*itt);
      pseudoslices_pair[idx]->Calculate();

      if (!pseudoslices_pair[idx]->Possible())
      {
        delete pseudoslices_pair[idx];

        /* these slices can't be combined,
         * store a nullptr at the correct position */
        pseudoslices_pair[idx] = nullptr;
      }
    }
  }

  set<Slice *> all_slices;
  for (auto slice : slices)
    all_slices.insert(slice);

  while (slices.size() > 1)
  {
    /* look for a seed */
    auto seed = FindBestPair(pseudoslices_pair, dimension);
    if (seed == nullptr) break;

    /* try to make the seed as big as possible */
    auto subgroup = ExpandSliceSet(seed, pseudoslices_pair, dimension, slices, diabloanoptarm_options.advanced_factoring_max_set_size);

    /* 'result' contains an OK subgroup of slices */
    MarkAsUsed(pseudoslices_pair, dimension, subgroup->slices);

    /* create a new array so we can free the original data structure later in this function */
    bool *new_have_to_equalize_insn = new bool[slice_size];
    memcpy(new_have_to_equalize_insn, subgroup->have_to_equalize_insn, slice_size * sizeof(bool));

    /* create a new result object */
    result.push_back(SliceSetRegsetTuple{
      subgroup->slices,
      subgroup->dead_through,
      static_cast<int>(subgroup->nr_regs_available),
      new_have_to_equalize_insn,
      subgroup->nr_required_regs,
      subgroup->flags,
      subgroup->slice_actions,
      subgroup->slice_registers,
      subgroup->ref_slice,
      subgroup->preferably_dont_touch,
      subgroup->overwritten_registers_per_slice,
      subgroup->all_register_information_per_slice,
      subgroup->nr_used_constants
    });

    /* remove the extracted slices from the set of possibilities */
    vector<Slice *> smaller;
    for (auto slice : slices) {
      if (subgroup->slices.find(slice) == subgroup->slices.end())
        smaller.push_back(slice);
    }
    slices = smaller;

    delete subgroup;
  }

  for (auto slice : all_slices) {
    slice->UnfixCombineResults();
  }

  for (size_t i = 0; i < dimension; i++)
    for (size_t j = 0; j < dimension; j++) {
      auto data = pseudoslices_pair[UpperMatrixIndexOf(i, j, dimension)];
      if (data != nullptr)
        delete data;
    }
  delete[] pseudoslices_pair;

  return result;
}

vector<SliceSetRegsetTuple> LookForSliceCombinations(SliceSet slices, size_t slice_size)
{
  vector<SliceSetRegsetTuple> result;

  /* randomize the order */
  vector<Slice *> all_slices(slices.begin(), slices.end());

  if (diabloanoptarm_options.advanced_factoring_shuffle)
    shuffle(all_slices.begin(), all_slices.end(), RNGGetGenerator(af_rng_shuffle));

  /* we need to keep the slices in a slice vector, because a set would order them in a way:
   * either by pointer value (default) or by slice UID ('SliceSet' instance). We don't want
   * this to happen, however, as we want the random seed to influence the order in which the
   * 'best' combinations are chosen. */

  if (slices.size() <= static_cast<size_t>(diabloanoptarm_options.af_max_combination_size)) {
    /* look for combinations */
    result = LookForSliceCombinationsInternal(all_slices, slice_size);
  }
  else {
    VERBOSE(0, (AF "processing large set of %d slices", all_slices.size()));
    auto it = all_slices.begin();

    while (it != all_slices.end()) {
      /* select the next max number of elements */
      vector<Slice *> selected;
      while (selected.size() < static_cast<size_t>(diabloanoptarm_options.af_max_combination_size)
              && it != all_slices.end()) {
        selected.push_back(*it);
        it++;
      }

      /* don't consider this subset */
      if (selected.size() < 2)
        continue;

      VERBOSE(0, (AF "  subset of %d slices", selected.size()));
      auto subresult = LookForSliceCombinationsInternal(selected, slice_size);

      result.insert(result.begin(), subresult.begin(), subresult.end());
    }
  }

  return result;
}
