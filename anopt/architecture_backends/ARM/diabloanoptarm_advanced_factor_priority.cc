#include "diabloanoptarm_advanced_factor.hpp"

#define NEW_FIX

using namespace std;

static FactoringPossibilitiesList factoring_possibilities;

bool PossibilityCmp::operator() (const FactoringPossibility* lhs, const FactoringPossibility* rhs) const
{
  return lhs->score > rhs->score;
}

/* global possible transformable instruction tracking, for statistics purposes only */
static set<t_address> possible_addresses;
void LogFactoringPossibilities() {
  LogFile *L_ADVANCED_FACTORING_POSSIBILITIES = NULL;

  string filename = OutputFilename();
  filename += ".advanced_factoring.possibilities";
  INIT_LOGGING(L_ADVANCED_FACTORING_POSSIBILITIES, filename.c_str());

  for (auto x : possible_addresses) {
    t_string y = StringIo("@G", x);
    LOG_MESSAGE(L_ADVANCED_FACTORING_POSSIBILITIES, "%s\n", y);
    Free(y);
  }

  FINI_LOGGING(L_ADVANCED_FACTORING_POSSIBILITIES);
}

/* transformation opportunity list management */
void AddPossibilityToGlobalList(FactoringPossibility *poss)
{
  ASSERT(SliceSetConsiderForFactoring(poss->set, poss->slice_size), ("trying to add factoring possibility with invalid set!"));
  factoring_possibilities.insert(poss);
  poss->random_uid = RNGGenerate(af_rng);

  for (auto slice : poss->set) {
    for (size_t i = 0; i < poss->slice_size; i++)
      possible_addresses.insert(INS_OLD_ADDRESS(slice->Get(i, poss->slice_size)));
  }
}

t_score CalculateScoreForPossiblity(FactoringPossibility *poss)
{
  int nr_executed = 0;
  auto source_info = SliceSetGetSourceInformation(poss->set, nr_executed);

  return CalculateScoreForSet(poss->set, poss->slice_size, ScoringData{static_cast<size_t>(poss->nr_imm_to_reg), RegsetCountRegs(poss->usable_regs), poss->nr_used_constants, nr_executed, poss->flags, source_info, poss->set.size()});
}

/* returns TRUE when this possibility is not possible anymore */
static
bool ValidatePossibility(FactoringPossibility *p)
{
  /* */
  for (auto slice : p->set) {
    if (slice->NrInstructions() < p->slice_size)
      return false;
  }

  /* TODO: maybe re-evaluate this using pseudoslice stuff */
  t_regset dead_through = NullRegs;

  GPRegistersSetDiff(p->preferably_dont_touch, p->preferably_dont_touch_but_touched);

  p->flags = DeterminePossibleTransformations(p->set, p->slice_size, p->nr_imm_to_reg, dead_through, p->slice_registers, p->preferably_dont_touch, p->nr_used_constants);
  p->usable_regs = dead_through;

  return p->flags != 0;
}

static
void PrintPossibility(FactoringPossibility *poss, string str = "") {
  DEBUG(("%s    score: %s, size: %d", str.c_str(), poss->score.to_string().c_str(), poss->slice_size));
  int nr_executed = 0;
  FactoringSetSourceInformation source_info = SliceSetGetSourceInformation(poss->set, nr_executed);
  DEBUG(("%d %d %d, %d %d %d",
    source_info.exec_archives.size(), source_info.exec_objects.size(), source_info.exec_functions.size(),
    source_info.archives.size(), source_info.objects.size(), source_info.functions.size()));

  SliceSetPrint(poss->set, "possibility", poss->slice_size);
}

void PrintPossiblities()
{
  for (auto poss : factoring_possibilities)
    PrintPossibility(poss);
}

FactoringPossibility *ChoosePossibility()
{
  FactoringPossibility *poss = NULL;

  /* don't do any more work than needed when debugging! */
  IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
    return NULL;

  auto it = factoring_possibilities.begin();
  while (!poss) {
    if (it == factoring_possibilities.end())
      break;

    poss = *it;

#ifdef NEW_FIX
    if (!poss->possible) {
      poss = NULL;
      it++;
      continue;
    }
#endif

    auto new_set = poss->set;
    SliceSetRemoveInvalids(new_set, poss->slice_size);

    if (new_set.size() == poss->set.size()
        && ValidatePossibility(poss)) {
      /* no invalids removed, this possibility still can be transformed */
      poss->it = it;
    }
    else {
      if (SliceSetConsiderForFactoring(new_set, poss->slice_size)) {
        /* as the set has changed, we need to recalculate this set */
        if (poss->ProcessSubset) {
          poss->ProcessSubset(new_set, poss->slice_size, true);
        }
      }

#ifdef NEW_FIX
      poss->possible = false;
      it++;
#else
      /* the previous operation may insert new element in the list */
      it = factoring_possibilities.erase(it);
      delete poss;
#endif

      poss = NULL;
    }
  }

  return poss;
}

void PrintPriorityListDebugInfo() {
  VERBOSE(0, (AF "priority list contains %d possibilities", factoring_possibilities.size()));
}

size_t PriorityListSize() {
  return factoring_possibilities.size();
}

void RecordFactoredPossibility(FactoringPossibility *poss, FactoringResult insn_stats, bool transformed)
{
  vector<FactoringPossibility *> reprocess;
  FactoringPossibilitiesList new_possibilities;

  nr_candidate_slices += poss->set.size();

  if (transformed) {
    /* keep some statistics */
    AddSetToStatistics(poss->set, poss->slice_size, insn_stats);
  }

  /* invalidate the slices and dissociate them from the possiblity that has been transformed */
  BblSet bbls_to_be_removed;
  for (auto slice : poss->set)
  {
    /* invalidate all relevant slices */
    slice->Invalidate();
    BblInvalidate(slice->Bbl());
    bbls_to_be_removed.insert(slice->Bbl());
  }

  if (transformed) {
    for (auto bbl : bbls_to_be_removed)
      RemoveUnreachableBbl(bbl);
  }

#ifdef NEW_FIX
  poss->possible = false;
#else
  factoring_possibilities.erase(poss->it);
  delete poss;
#endif
}
