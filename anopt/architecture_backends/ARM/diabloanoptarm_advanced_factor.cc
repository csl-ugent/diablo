#include "diabloanoptarm_advanced_factor.hpp"

extern "C" {
#include <frontends/diablo_options.h>
}

using namespace std;

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(slices);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(slices);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(order);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(incoming_for_slice);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(incoming);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(can_transform);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(table_id_vector);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(original_id);
FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(is_af);
FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(af_flags);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(af_flags);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(prodprop);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(is_landing_site);

EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(af_corr);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(equations);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(slice_information);
FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(explicitely_saved);

#define LAMBDA_BBL_IS_MASTER(region_id) [region_id](t_bbl *bbl) { return BBL_FACTORING_REGION_ID(bbl) == region_id; }
#define LAMBDA_BBL_IS_SLAVE(region_id) [region_id](t_bbl *bbl) { return BBL_FACTORING_REGION_ID(bbl) != region_id; }
#define LAMBDA_SLICE_IS_MASTER(region_id) [region_id](Slice *slice) { return BBL_FACTORING_REGION_ID(slice->Bbl()) == region_id; }
#define LAMBDA_SLICE_IS_SLAVE(region_id) [region_id](Slice *slice) { return BBL_FACTORING_REGION_ID(slice->Bbl()) != region_id; }

SliceSet created_slices_global;

bool af_dynamic_member_init = FALSE;
bool af_dynamic_member_init_once = FALSE;

map<DispatcherType, F_DispatchGenerator> dispatcher_type_to_generator_map;

t_randomnumbergenerator *af_rng = NULL;
t_randomnumbergenerator *af_rng_transformation = NULL;
t_randomnumbergenerator *af_rng_dispatcher = NULL;
t_randomnumbergenerator *af_rng_dummy = NULL;
t_randomnumbergenerator *af_rng_randomswitch = NULL;
t_randomnumbergenerator *af_rng_redirect = NULL;
t_randomnumbergenerator *af_rng_distributed = NULL;
t_randomnumbergenerator *af_rng_shuffle = NULL;
t_randomnumbergenerator *af_rng_compare = NULL;
t_randomnumbergenerator *af_rng_decision = NULL;

FunctionUID af_function_uid = FunctionUID_INVALID;

static t_uint32 cached_transformation_chance = 0;

t_score_for_set CalculateScoreForSet;
t_compare_scores CompareScores;
t_score_init ScoreInit;
t_dispatcher_chooser DispatcherChooser;

extern AFPhase current_af_phase;

std::set<TransformationID> all_af_tf_ids;

bool FunctionIsAF(t_function *fun) {
  if (!af_dynamic_member_init_once)
    return false;

  return FUNCTION_IS_AF(fun);
}

static
void InitAdvancedFactoringRNG()
{
  af_rng = RNGCreateChild(RNGGetRootGenerator(), "af");

  af_rng_transformation = RNGCreateChild(af_rng, "af_transformation");
  af_rng_dispatcher = RNGCreateChild(af_rng, "af_dispatcher");
  af_rng_dummy = RNGCreateChild(af_rng, "af_dummy");
  af_rng_randomswitch = RNGCreateChild(af_rng, "af_randomswitch");
  af_rng_redirect = RNGCreateChild(af_rng, "af_redirect");
  af_rng_distributed = RNGCreateChild(af_rng, "af_distributed");
  af_rng_shuffle = RNGCreateChild(af_rng, "af_shuffle");
  af_rng_compare = RNGCreateChild(af_rng, "af_compare");
  af_rng_dispatcher = RNGCreateChild(af_rng, "af_dispatcher");
}

static
void FiniAdvancedFactoringRNG()
{
  // RNGDestroy(af_rng_transformation);
  // RNGDestroy(af_rng_dispatcher);
  // RNGDestroy(af_rng_dummy);
  // RNGDestroy(af_rng_randomswitch);
  // RNGDestroy(af_rng_redirect);
  // RNGDestroy(af_rng_distributed);
  // RNGDestroy(af_rng_shuffle);
  // RNGDestroy(af_rng_compare);
  // RNGDestroy(af_rng_dispatcher);

  // RNGDestroy(af_rng);
}

static
string PrintRegisters(t_regset regs) {
  stringstream ss;

  t_reg reg;
  REGSET_FOREACH_REG(regs, reg) {
    if (0 <= reg && reg < ARM_REG_R15)
      ss << "r" << reg << ", ";
    else {
      switch (reg) {
      case ARM_REG_C_CONDITION:
        ss << "C, "; break;
      case ARM_REG_N_CONDITION:
        ss << "N, "; break;
      case ARM_REG_Q_CONDITION:
        ss << "Q, "; break;
      case ARM_REG_V_CONDITION:
        ss << "V, "; break;
      case ARM_REG_Z_CONDITION:
        ss << "Z, "; break;
      case ARM_REG_GE_CONDITION:
        ss << "GE, "; break;
      default:
        break;
      }
    }
  }

  return ss.str();
}

static bool af_init = false;
static bool af_obf_init = false;
static bool const_init = false;

static
void IoModifierBblAF(t_bbl *bbl, t_string_array *array)
{
  if (af_init) {
    if (const_init) {
      string strConstantsBefore = PrintConstants(BBL_PROCSTATE_IN(bbl));
      if (!strConstantsBefore.empty())
        StringArrayAppendString(array, StringConcat3("Constant-before: ", strConstantsBefore.c_str(), "\\l"));
    }

    if (const_init) {
      string strConstantsAfter = PrintConstants(BBL_PROCSTATE_OUT(bbl));
      if (!strConstantsAfter.empty())
        StringArrayAppendString(array, StringConcat3("Constant-after: ", strConstantsAfter.c_str(), "\\l"));
    }

    string strTables = ListTableIds(bbl);
    if (!strTables.empty())
      StringArrayAppendString(array, StringConcat2(strTables.c_str(), "\\l"));

    string strLiveBefore = PrintRegisters(BblRegsLiveBefore(bbl));
    if (!strLiveBefore.empty())
      StringArrayAppendString(array, StringConcat3("Live-before: ", strLiveBefore.c_str(), "\\l"));

    string strLiveAfter = PrintRegisters(BblRegsLiveAfter(bbl));
    if (!strLiveAfter.empty())
      StringArrayAppendString(array, StringConcat3("Live-after: ", strLiveAfter.c_str(), "\\l"));

    if (BBL_FUNCTION(bbl))
      StringArrayAppendString(array, StringIo("Function flags: %x\\l", FUNCTION_AF_FLAGS(BBL_FUNCTION(bbl))));

    StringArrayAppendString(array, StringIo("Bbl flags: %x\\l", BBL_AF_FLAGS(bbl)));
  }

  if (af_obf_init) {
    if (BBL_FUNCTION(bbl))
      StringArrayAppendString(array, StringIo("Function flags: %x\\l", FUNCTION_AF_FLAGS(BBL_FUNCTION(bbl))));

    StringArrayAppendString(array, StringIo("Bbl flags: %x\\l", BBL_AF_FLAGS(bbl)));
  }
}

static
void IoModifierEdgeAF(t_cfg_edge *edge, t_string_array *array)
{
  if (!af_init)
    return;
}

static
void IoModifiedInsAF(t_ins *ins, t_string_array *array)
{
  if (!af_init)
    return;

  StringArrayAppendString(array, StringConcat3("(slice-id: ", to_string(INS_SLICE_ID(ins)).c_str(), ")"));
  StringArrayAppendString(array, StringConcat3("(order: ", to_string(INS_ORDER(ins)).c_str(), ")"));
}

static
void AddEqualizedSetToPossibilities(SliceSetRegsetTuple eq, size_t slice_size, F_ProcessSubset fn_process_subset)
{
  auto subsets = SliceSetCreateNoninterferingSubsets(eq.set, slice_size);

  for (auto subset : subsets)
  {
    /* create a new factoring possibility instance */
    auto new_poss = new FactoringPossibility();
    new_poss->set = eq.set;
    new_poss->equalize_strategy = FactoringEqualizeStrategy::EqualizeImmediates;
    new_poss->usable_regs = eq.regs;
    new_poss->slice_size = slice_size;
    new_poss->ProcessSubset = fn_process_subset;
    new_poss->flags = eq.flags;
    new_poss->slice_registers = eq.slice_registers;
    new_poss->slice_actions = eq.slice_actions;
    new_poss->ref_slice = eq.ref_slice;
    new_poss->preferably_dont_touch = eq.preferably_dont_touch;
    new_poss->overwritten_registers_per_slice = eq.overwritten_registers_per_slice;
    new_poss->all_register_information_per_slice = eq.all_register_information_per_slice;
    new_poss->nr_used_constants = eq.nr_used_constants;
    new_poss->possible = true;

    /* map immediates to virtual registers */
    int max_map = -1;
    auto mapping_ids = MapImmediatesToRegisters(eq.set, slice_size, eq.imm_to_reg, max_map);
    new_poss->nr_imm_to_reg = eq.required_reg_count;

    /* construct action list for the factored BBL */
    for (size_t i = 0; i < slice_size; i++)
    {
      if (mapping_ids[i] == -1)
        /* this instruction need not have its immediate mapped */
        continue;

      //DEBUG(("action on factored: registerize %d to v%d", i, mapping_ids[i]));
      AFAction new_action;
      new_action.code = AFActionCode::RegisterizeImmediate;
      new_action.slice = NULL;
      new_action.args[0].ins_id = i;
      new_action.args[1].reg = mapping_ids[i];
      new_action.before = true;

      new_poss->actions_on_factored.push_back(new_action);
    }

    /* construct action list for the predecessors */
    for (auto slice : eq.set)
    {
      int max_done = -1;

      for (size_t i = 0; i < slice_size; i++)
      {
        if (mapping_ids[i] == -1)
          /* this instruction need not have its immediate mapped */
          continue;

        if (mapping_ids[i] <= max_done)
          /* the immediate for this instruction has already been processed */
          continue;

        max_done = mapping_ids[i];

        //DEBUG(("action on predecessor: immediate %llx to register v%d", ARM_INS_IMMEDIATE(T_ARM_INS(slice->GetR(i))), mapping_ids[i]));
        AFAction new_action;
        new_action.code = AFActionCode::StoreImmediateInRegister;
        new_action.slice = slice;
        new_action.args[0].reg = mapping_ids[i];
        new_action.args[1].imm = ARM_INS_IMMEDIATE(T_ARM_INS(slice->GetR(i)));
        new_action.before = true;

        new_poss->actions_on_predecessors.push_back(new_action);
      }
    }

    delete[] mapping_ids;

    new_poss->score = CalculateScoreForPossiblity(new_poss);

    /* finally add the newly created possibility to the global list */
    AddPossibilityToGlobalList(new_poss);
  }

  delete[] eq.imm_to_reg;
}

static
void ProcessFuzzyImmediatesConstructive(SliceSet set, size_t slice_size, bool already_preprocessed = false)
{
  auto subsets = LookForSliceCombinations(set, slice_size);

  for (auto subset : subsets)
  {
    if (!SliceSetConsiderForFactoring(subset.set, slice_size))
      continue;

    AddEqualizedSetToPossibilities(subset, slice_size, ProcessFuzzyImmediatesConstructive);
  }
}

static
void FactorSetOfSlices(SliceSet set, size_t slice_size)
{
  IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
    return;

  /* remove invalid slices from the set */
  SliceSetRemoveInvalids(set, slice_size);
  if (!SliceSetConsiderForFactoring(set, slice_size))
    return;

  ProcessFuzzyImmediatesConstructive(set, slice_size);
}

static
void FactorPossibilities(size_t slice_size = 0)
{
  STATUS(START, ("Transforming all possibilities"));
  PrintPriorityListDebugInfo();

  FactoringPossibility *poss;
  while ((poss = ChoosePossibility()))
  {
    t_cfg *cfg = BBL_CFG((*poss->set.begin())->Bbl());

    FactoringResult insn_stats;
    bool do_transform = RNGGeneratePercent(af_rng_transformation) <= cached_transformation_chance;

    if (do_transform) {
      insn_stats = TransformFactoringPossiblity(poss);

#ifdef AF_VERBOSE
      PrintFullSliceSetInformation(poss->set, slice_size);
#endif
    }

    /* We need to record this possibility, even if no transformation was applied.
     * This way, we 'skip' the transformation of the selected slices. */
    RecordFactoredPossibility(poss, insn_stats, do_transform);

    if (diabloanoptarm_options.advanced_factoring_emit_intermediate_scc) {
      ForkAndWait([cfg] () {
        VERBOSE(AF_VERBOSITY_LEVEL, (AF "Child: creating single entry functions"));
        CfgRemoveDeadCodeAndDataBlocks (cfg);
        CfgPatchToSingleEntryFunctions (cfg);
        CfgRemoveDeadCodeAndDataBlocks (cfg);

        string x = "origin_intermediate-" + to_string(nr_total_slices);
        VERBOSE(AF_VERBOSITY_LEVEL, (AF "Child: tracking origin information"));
        TrackOriginInformation(cfg, const_cast<t_string>(x.c_str()));
      });
    }

    IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER {
      DumpDots(cfg, "debugcounter", 0);
      break;
    }
  }

  STATUS(STOP, ("Transforming all possibilities"));
}

static
void FactorHashTableManager(SliceHashTableManager& mgr)
{
  auto tables = mgr.HashTables();
  reverse(tables.begin(), tables.end());

  for (auto tbl : tables)
  {
    IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
      break;
    IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
      break;

    auto keyvector = tbl->GetSortedKeyVectorByBucketSize();

    for (auto key : keyvector)
    {
      IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
        break;
      IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
        break;

      auto slices_for_key = tbl->GetSortedSliceVectorForKey(key);
      auto slice_sets = tbl->SortSlices(slices_for_key);

      for (auto set : slice_sets)
      {
        IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
          break;
        IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
          break;

        /* remove invalid slices from this set */
        SliceSetRemoveInvalids(set, tbl->SliceSize());

        /* only continue if multiple elements remain in the set */
        if (!SliceSetConsiderForFactoring(set, tbl->SliceSize()))
          continue;

        /* factorize this set of slices */
        FactorSetOfSlices(set, tbl->SliceSize());
      }
    }
  }
}

static
void set_slice_id(t_ins *ins, int id)
{
  INS_SET_SLICE_ID(ins, id);
}

static
int get_ins_order(t_ins *ins)
{
  return INS_ORDER(ins);
}

static
void FactorByHashTables(t_cfg *cfg, int region_id)
{
  STATUS(START, ("Advanced factoring"));
  OnlyDoSccInformation(true);

  if (diabloanoptarm_options.advanced_factoring_emit_intermediate_scc) {
    string x = "origin_intermediate-0";
    TrackOriginInformation(cfg, const_cast<t_string>(x.c_str()));
  }

  /* fixed order, for sequence reconstruction */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    int x = 0;

    t_ins *ins;
    BBL_FOREACH_INS(bbl, ins) {
      INS_SET_ORDER(ins, x);
      x++;
    }
  }

  if (global_options.generate_dots)
    __DumpDots(cfg, "beforeaf", 0);

  /* preparations */
  STATUS(START, ("Hash table construction"));
  SliceHashTableManager mgr(LAMBDA_BBL_IS_MASTER(region_id), LAMBDA_BBL_IS_SLAVE(region_id));

  /* mark BBLs added as 'master' */
  BblMarkInit();

  /* master BBLs */
  size_t master_executed_bbl_count = 0;
  auto bbls_to_factor = GetAllBblsInFactoringRegionID(cfg, region_id);
  VERBOSE(AF_VERBOSITY_LEVEL2, (AF "selected %u master BBLs", bbls_to_factor.size()));
  for (auto bbl : bbls_to_factor)
  {
    BblMark(bbl);

    if (BBL_EXEC_COUNT(bbl) > 0)
      master_executed_bbl_count++;
    AddBblToHashTableManager(mgr, bbl, LAMBDA_SLICE_IS_MASTER(region_id), LAMBDA_SLICE_IS_SLAVE(region_id), created_slices_global);
  }
  VERBOSE(AF_VERBOSITY_LEVEL2, (AF "   of which %u BBLs are executed (master) (%f %%)", master_executed_bbl_count, ((float)master_executed_bbl_count/bbls_to_factor.size())*100));

  /* slave BBLs */
  size_t slave_executed_bbl_count = 0;
  auto bbls_to_factor_with = GetSlaveBblsForRegion(cfg, region_id);
  VERBOSE(AF_VERBOSITY_LEVEL2, (AF "selected %u slave BBLs", bbls_to_factor_with.size()));
  for (auto bbl : bbls_to_factor_with)
  {
    /* don't add BBLs twice! */
    if (BblIsMarked(bbl)) continue;

    if (BBL_EXEC_COUNT(bbl) > 0)
      slave_executed_bbl_count++;
    AddBblToHashTableManager(mgr, bbl, LAMBDA_SLICE_IS_MASTER(region_id), LAMBDA_SLICE_IS_SLAVE(region_id), created_slices_global);
  }
  VERBOSE(AF_VERBOSITY_LEVEL2, (AF "   of which %u BBLs are executed (slave) (%f %%)", slave_executed_bbl_count, ((float)slave_executed_bbl_count/bbls_to_factor_with.size())*100));

  mgr.Clean();

  FLUSH_LOG(L_FACTORING_VAR);

  STATUS(STOP, ("Hash table construction"));

  if (diabloanoptarm_options.af_enable_dispdisttbl) {
    CalculateBaseAddresses(cfg);

    ConstantPropagationFini(cfg);
    ConstantPropagationInit(cfg);
    ConstantPropagation (cfg, CONTEXT_SENSITIVE);
    OptUseConstantInformation(cfg, CONTEXT_SENSITIVE);

    if (global_options.generate_dots)
      __DumpDots(cfg, "rebased", 0);
  }

  /* nonzero registers */
  AnalyseCfgForNonZeroRegisters(cfg);

  STATUS(START, ("Hash table processing"));

  /* The hash table manager stores the slices from small to large by default.
   * Here we first want to process the biggest slices. */
  auto hts = mgr.HashTables();
  reverse(hts.begin(), hts.end());

  for (auto ht : hts)
  {
    IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
      break;
    IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
      break;

    int nr_useful_sets = 0;

    /* get a vector containing a sorted key list,
     * where the first key is associated with the most slices */
    auto keyvector = ht->GetSortedKeyVectorByBucketSize();

    int key_id = 0;
    for (auto key : keyvector)
    {
      IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
        break;
      IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
        break;

      /* get the sorted slice vector */
      auto slicevector = ht->GetSortedSliceVectorForKey(key);

      /* create a vector of slice vectors.
       * Every subvector contains an equivalent set of slices (that is, they have the same N instructions) */
      auto slice_sets = ht->SortSlices(slicevector);

      int set_id = 0;
      for (auto set : slice_sets)
      {
        IF_NR_TOTAL_SLICES_GE_DEBUGCOUNTER
          break;
        IF_PRIORITY_LIST_SIZE_DEBUGCOUNTER
          break;

        /* in the previous loop round, slices may possibly be invalidated.
         * We don't want to include those slices here! */
        SliceSetRemoveInvalids(set, ht->SliceSize());

        /* don't do unnecessary work */
        if (!SliceSetConsiderForFactoring(set, ht->SliceSize()))
          continue;

        /* This is a useful set. Keep some statistics. */
        nr_useful_sets++;

        /* "set" are potentially equivalent slices.
         * In order to sort these slices, we create a new hash table manager that manages the slices in this set. */
        SliceHashTableManager set_mgr(LAMBDA_BBL_IS_MASTER(region_id), LAMBDA_BBL_IS_SLAVE(region_id));
        vector<Slice *> created_slices;

        /* construct a list of unique BBLs */
        for (auto slice : set)
        {
          if (slice->is_sequence) {
            DiabloBrokerCall("RescheduleBblForSequence", slice->Bbl(), get_ins_order);
            /* let's first try to recreate the sequence */
            vector<t_ins *> insns;

            t_ins *current_ins = slice->base_instruction;
            insns.push_back(current_ins);

            current_ins = INS_IPREV(current_ins);
            while (current_ins) {
              if (slice->address_before.find(T_ARM_INS(current_ins)) == slice->address_before.end()
                  && slice->address_after.find(T_ARM_INS(current_ins)) == slice->address_after.end()) {
                /* this instruction is really part of the slice */
                insns.push_back(current_ins);

                if (insns.size() == slice->Size())
                  break;
              }

              current_ins = INS_IPREV(current_ins);
            }

            /* revert the vector to compare it with the original slice */
            reverse(insns.begin(), insns.end());
            bool recreated = insns == slice->elements;

            /* we could not recreate the slice */
            if (!recreated) {
              if (insns.size() != slice->elements.size())
                DEBUG(("could not recreate sequence: %d/%d", insns.size(), slice->elements.size()));
              else {
                /* test if this is a permutation */
                if (is_permutation(slice->elements.begin(), slice->elements.end(), insns.begin()))
                  DEBUG(("could not recreate sequence: permutation"));
                else
                  DEBUG(("could not recreate sequence"));
              }
            }

            if (!recreated)
              continue;

            Slice *current_slice = new Slice (LAMBDA_SLICE_IS_MASTER(region_id), LAMBDA_SLICE_IS_SLAVE(region_id), true);

            current_ins = slice->base_instruction;
            current_slice->SetBaseInstruction(current_ins);

            /* add the instructions, mind you that the 'insns' vector has been reversed! */
            for (auto x = insns.rbegin(); x != insns.rend(); x++)
              current_slice->AddInstruction(*x);

            current_slice->Finalize();
            current_slice->SetParentSlice(slice);

            current_slice->address_before = slice->address_before;
            current_slice->address_after = slice->address_after;

            AssociateSliceAndBbl(current_slice->Bbl(), current_slice);
            created_slices.push_back(current_slice);

            set_mgr.Add(current_slice);
          }
          else
          {
            /* if we land here, this is a real slice, and its parent BBL should be rescheduled */
            t_ins *ins;
            DiabloBrokerCall("RescheduleBblForSlice", slice->Bbl(), set_slice_id);

            /* calculate number of instructions in this slice */
            int nr_ins_in_slice = 0;
            BBL_FOREACH_INS(slice->Bbl(), ins)
              if (INS_SLICE_ID(ins) == INS_SLICE_ID(slice->base_instruction))
                nr_ins_in_slice++;

            /* do we have a new slice here? */
            if (nr_ins_in_slice >= diabloanoptarm_options.af_min_block_length)
            {
              /* this is a valid slice */

              /* the new, accurate slice for the base instruction */
              Slice *current_slice = new Slice(LAMBDA_SLICE_IS_MASTER(region_id), LAMBDA_SLICE_IS_SLAVE(region_id), false);

              t_ins *current_ins = slice->base_instruction;
              current_slice->SetBaseInstruction(current_ins);
              current_slice->AddInstruction(current_ins);

              /* add all the instruction to the current slice which have the same slice ID */
              int looking_for_slice_id = INS_SLICE_ID(current_ins);
              while ((current_ins = INS_IPREV(current_ins)))
                if (INS_SLICE_ID(current_ins) == looking_for_slice_id)
                  current_slice->AddInstruction(current_ins);
              current_slice->Finalize();

              /* this new slice has a parent slice,
               * namely the "primitive" slice. */
              current_slice->SetParentSlice(slice);

              current_slice->address_before = slice->address_before;
              current_slice->address_after = slice->address_after;

              /* bookkeeping */
              AssociateSliceAndBbl(current_slice->Bbl(), current_slice);
              created_slices.push_back(current_slice);

              set_mgr.Add(current_slice);
              //DEBUG(("Added %s", current_slice->Print().c_str()));
            }
          }
        }

        set_mgr.Clean();

        FactorHashTableManager(set_mgr);

        /* free up the allocated memory */
        //   for (auto slice : created_slices)
        //   {
        //     if (!slice->IsInvalidated())
        //     {
        //       DissociateSliceAndBbl(slice->Bbl(), slice);
        //       slice->Invalidate();
        //     }

        //     delete slice;
        //   }
      }

      key_id++;
    }

    if (nr_useful_sets > 0)
      VERBOSE(AF_VERBOSITY_LEVEL, (AF "Found %d useful sets for hash table with slice size %u", nr_useful_sets, ht->SliceSize()));
  }

  STATUS(STOP, ("Hash table processing"));

  FactorPossibilities();

  /* final fixups */
  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_cfg_edge *ft_keep = NULL;
    set<t_cfg_edge *> edges_to_replace;

    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e)
      if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
          || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
      {
        if (ft_keep)
          edges_to_replace.insert(e);
        else
          ft_keep = e;
      }

    for (auto e : edges_to_replace)
    {
      /* kill the existing edge */
      t_bbl *head = CFG_EDGE_HEAD(e);
      t_bbl *tail = CFG_EDGE_TAIL(e);
      t_uint64 old_exec_count = CFG_EDGE_EXEC_COUNT(e);
      CfgEdgeKill(e);

      t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(head));

      bool is_thumb = false;
      if (last)
        is_thumb = ARM_INS_FLAGS(last) & FL_THUMB;

      if (RegsetIn(ARM_INS_REGS_DEF(last), ARM_REG_R15)) {
        ASSERT(ARM_INS_OPCODE(last) == ARM_B
                && ArmInsIsConditional(last), ("expected conditional jump! @eiB", head));
        /* no need to add a branch instruction */
      }
      else {
        /* create the branch instruction */
        t_arm_ins *ins;
        ArmMakeInsForBbl(UncondBranch, Append, ins, head, is_thumb);
      }

      /* create the jump edge */
      t_cfg_edge *new_edge = CfgEdgeCreate(BBL_CFG(bbl), head, tail, ET_JUMP);
      CFG_EDGE_SET_EXEC_COUNT(new_edge, old_exec_count);
      if (BBL_FUNCTION(head) != BBL_FUNCTION(tail))
        EdgeMakeInterprocedural(new_edge);
    }
  }

  VerifyTables();

  /* statistics */
  FactoringPrintStatistics();

  STATUS(STOP, ("Advanced factoring"));
  OnlyDoSccInformation(false);
}

static
void BrokerFunctionIsAF(t_function *fun, t_bool *result) {
  *result = FUNCTION_IS_AF(fun);
}

static
void BrokerEdgeIsAF(t_cfg_edge *edge, t_bool *result) {
  *result = CfgEdgeIsAF(edge);
}

static
void BrokerAFCorrespondingEdge(t_cfg_edge *edge, t_cfg_edge **result) {
  *result = CFG_EDGE_AF_CORR(edge);
}

static
void BrokerAFBblCopyFlags(t_bbl *from, t_bbl *to) {
  if (BblGetAFFlag(from, AF_FLAG_DIRTY_SP))
    BblSetAFFlag(to, AF_FLAG_DIRTY_SP);
}

static
void HandleAdvancedFactoringIsDirectControlTransfer(t_cfg_edge *edge, t_bool *handled, t_bool *result) {
  *handled = false;

  t_function *fun = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
  if (fun
      && FUNCTION_IS_AF(fun)) {
    *handled = true;

    ArmInstructionIsDirectControlTransfer(BBL_INS_LAST(CFG_EDGE_HEAD(edge)), result);
  }
}

t_regset FunctionExplicitelySavedRegs(t_function *fun) {
  return *FUNCTION_EXPLICITELY_SAVED(fun);
}

static
void BblKilled(t_bbl *bbl) {
  HandleBblKillDistributedSwitch(bbl);
}

static
void TransformationIsAF(TransformationID tfid, t_bool *result) {
  *result = (all_af_tf_ids.find(tfid) != all_af_tf_ids.end());
}

void ConstructBblCompareMatrices(t_cfg *cfg, t_string filename)
{
  Region *region;
  FactoringAnnotationInfo *info;

  /* initialize logging */
  string filename_ins = string(filename) + ".instructions";
  string filename_statistics = string(filename) + ".statistics";
  FactoringLogInit(filename, filename_ins, filename_statistics, true);

  /* load plugin */
  string scoring_function_library = diabloanoptarm_options.af_scoring_library;
  string scoring_compare_library = diabloanoptarm_options.af_scoring_compare_library;
  string dispatcher_chooser_library = diabloanoptarm_options.af_choose_dispatcher_library;

  /* scoring */
#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
  ScoreInit = LoadSharedFunction<t_score_init>(scoring_function_library, DYNSYM_SCORE_CALCULATE_INIT);
  CalculateScoreForSet = LoadSharedFunction<t_score_for_set>(scoring_function_library, DYNSYM_SCORE_CALCULATE);
  CompareScores = LoadSharedFunction<t_compare_scores>(scoring_compare_library, DYNSYM_SCORE_COMPARE);
#else
  if (scoring_function_library == SCORE_LIBNAME(Exponential))
    LOAD_STATIC_SCORE(Exponential, Default, ScoreInit, CalculateScoreForSet, CompareScores)
  else if (scoring_function_library == SCORE_LIBNAME(Partial))
    LOAD_STATIC_SCORE(Partial, DecisionTree, ScoreInit, CalculateScoreForSet, CompareScores)
  else
    FATAL(("unhandled scoring function library \"%s\"", scoring_function_library.c_str()));
#endif

  ScoreInit();

  /* dispatcher choosing */
#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
  DispatcherChooser = LoadSharedFunction<t_dispatcher_chooser>(dispatcher_chooser_library, DYNSYM_CHOOSE_DISPATCHER);
#else
  if (dispatcher_chooser_library == DISPATCHER_CHOOSER_LIBNAME(Random))
    LOAD_STATIC_DISPATCHER_CHOOSER(Random, DispatcherChooser)
  else
    FATAL(("unhandled dispatcher chooser library \"%s\"", dispatcher_chooser_library.c_str()));
#endif

  /* dispatcher type to generator */
  dispatcher_type_to_generator_map[DispatcherType::DistributedTable] = ApplyDistributedTable;
  dispatcher_type_to_generator_map[DispatcherType::IndirectBranch] = ApplyIndirectBranchDispatcher;
  dispatcher_type_to_generator_map[DispatcherType::SwitchBranch] = ApplySwitchDispatcherBranchTable;
  dispatcher_type_to_generator_map[DispatcherType::SwitchOffset] = ApplySwitchDispatcherLdrAdd;
  dispatcher_type_to_generator_map[DispatcherType::InternalConditionalJump] = ApplyInternalConditionalJumpDispatcher;
  dispatcher_type_to_generator_map[DispatcherType::ConditionalJump] = ApplyConditionalJumpDispatcher;

  /* SoftVM support could have screwed up this information */
  InitAdvancedFactoringRNG();

  RegsetSetAddReg(status_registers, ARM_REG_Z_CONDITION);
  RegsetSetAddReg(status_registers, ARM_REG_N_CONDITION);
  RegsetSetAddReg(status_registers, ARM_REG_C_CONDITION);
  RegsetSetAddReg(status_registers, ARM_REG_V_CONDITION);

  InsInitAbstractForm(cfg);
  InsInitSlice(cfg);
  InsInitFingerprint(cfg);
  InsInitSliceId(cfg);
  InsInitMark(cfg);
  BblInitFactoringRegionId(cfg);
  BblInitFactoringTargets(cfg);
  BblInitSlices(cfg);
  InsInitSlices(cfg);
  InsInitOrder(cfg);
  BblInitIncomingForSlice(cfg);
  BblInitIncoming(cfg);
  BblInitCanTransform(cfg);
  CfgEdgeInitAfCorr(cfg);
  CfgEdgeInitEquations(cfg);
  CfgEdgeInitSliceInformation(cfg);
  BblInitTableIdVector(cfg);
  BblInitOriginalId(cfg);
  FunctionInitIsAf(cfg);
  FunctionInitAfFlags(cfg);
  BblInitProdProp(cfg);
  FunctionInitExplicitelySaved(cfg);
  BblInitIsLandingSite(cfg);
  NonZeroAnalysisInit(cfg);
  //ArmPrintVirtualRegisters(TRUE);

  af_dynamic_member_init = TRUE;
  af_dynamic_member_init_once = TRUE;

  DiabloBrokerCallInstall("BblKill", "t_bbl *", (void*)BblKilled, FALSE);

  DiabloBrokerCallInstall("IoModifierAF", "t_bbl *, t_string_array *", (void *)IoModifierBblAF, FALSE);
  DiabloBrokerCallInstall("IoModifierInsAF", "t_ins *, t_string_array *", (void *)IoModifiedInsAF, FALSE);
  DiabloBrokerCallInstall("IoModifierEdgeAF", "t_cfg_edge *, t_string_array *", (void *)IoModifierEdgeAF, FALSE);
  DiabloBrokerCallInstall("InsInvalidateSlices", "t_ins *", (void *)InsInvalidateSlices, FALSE);

  DiabloBrokerCallInstall("FunctionIsAF", "t_function *, t_bool *", (void *)BrokerFunctionIsAF, FALSE);
  DiabloBrokerCallInstall("EdgeIsAF", "t_cfg_edge *, t_bool *", (void *)BrokerEdgeIsAF, FALSE);
  DiabloBrokerCallInstall("AFCorrespondingEdge", "t_cfg_edge *, t_cfg_edge **", (void *)BrokerAFCorrespondingEdge, FALSE);

  DiabloBrokerCallInstall("HandleAdvancedFactoringIsDirectControlTransfer", "t_cfg_edge *, t_bool *, t_bool *", (void*)HandleAdvancedFactoringIsDirectControlTransfer, FALSE);

  DiabloBrokerCallInstall("AFBblCopyFlags", "t_bbl *, t_bbl *", (void*)BrokerAFBblCopyFlags, FALSE);

  DiabloBrokerCallInstall("TransformationIsAF", "t_int32 , t_bool *", (void*)TransformationIsAF, FALSE);

  af_init = true;

  /* the whole program */
  {
    size_t nr_bbls = 0;
    size_t nr_exec_bbls = 0;

    t_function *fun;
    CFG_FOREACH_FUN(cfg, fun) {
      if (!FUNCTION_NAME(fun))
        continue;

      /* these functions call the 'setjmp' function.
       * As the behaviour of the callee is horribly ugly to model properly, we don't want to mess with the callers either:
       *  - __libc_start_main
       *  - __libc_start_main_1 (most likely created by Diablo)
       *  - _dl_catch_error
       * These functions were observed in a statically linked program. */
#define __LIBC_START_MAIN "__libc_start_main"
      if (!strncmp(FUNCTION_NAME(fun), __LIBC_START_MAIN, strlen(__LIBC_START_MAIN))
          || !strcmp(FUNCTION_NAME(fun), "_dl_catch_error"))
        ForbidFunction(fun, false);

      /* As some transformations require the TLS sections to have been initialised, we don't want to mess with them before that. 
       * The start of the call chain is this function. */
      else if (!strcmp(FUNCTION_NAME(fun), "__pthread_initialize_minimal"))
        ForbidFunction(fun, true);
    }

    t_bbl *bbl;
    t_uint32 original_id = 0;
    CFG_FOREACH_BBL(cfg, bbl) {
      if (PossiblyFactorBbl(bbl))
      {
        nr_bbls++;

        if (BBL_EXEC_COUNT(bbl) > 0)
          nr_exec_bbls+=0;

        if (CanAddInstructionToBbl(bbl))
        {
          BBL_SET_CAN_TRANSFORM(bbl, true);
          nr_exec_bbls++;
        }
      }

      BBL_SET_ORIGINAL_ID(bbl, original_id);
      original_id++;
    }

    VERBOSE(AF_VERBOSITY_LEVEL2, (AF "WHOLE PROGRAM found %u factorable BBLs of which %u are executed (%f %%)", nr_bbls, nr_exec_bbls, ((float)nr_exec_bbls/nr_bbls)*100));
  }

  /* hotness */
  {
    constexpr int AF_HOTNESS_MAX = 1000;
    ASSERT(0 <= diabloanoptarm_options.af_hotness
            && diabloanoptarm_options.af_hotness <= AF_HOTNESS_MAX, ("--af-hotness value %u should be within range [0, %d]", diabloanoptarm_options.af_hotness, AF_HOTNESS_MAX));

    bool look_at_nins = diabloanoptarm_options.af_hotness_nins;

    /* create a list of all the *factorable* basic blocks */
    size_t trace_length = 0;

    vector<t_bbl *> bbls;
    t_bbl *bbl;
    CFG_FOREACH_BBL(cfg, bbl) {
      if (!BBL_CAN_TRANSFORM(bbl))
        continue;

      trace_length += (look_at_nins ? BBL_NINS(bbl) : 1) * BBL_EXEC_COUNT(bbl);
      bbls.push_back(bbl);
    }

    /* sort the vector according to the trace length of the basic blocks */
    stable_sort(bbls.begin(), bbls.end(), [look_at_nins] (t_bbl * a, t_bbl * b) {
      auto A = (look_at_nins ? BBL_NINS(a) : 1) * BBL_EXEC_COUNT(a);
      auto B = (look_at_nins ? BBL_NINS(b) : 1) * BBL_EXEC_COUNT(b);

      /* if both blocks have an equal trace length,
       * only look at the execution count */
      if (A == B) {
        A = BBL_EXEC_COUNT(a);
        B = BBL_EXEC_COUNT(b);
      }

      /* return TRUE if A is to be put before B */
      return A < B;
    });

    /* emit list of basic blocks, ordered per trace length */
    string full_name = "advanced_factoring.hotness";
    ofstream outfile(full_name);
    ASSERT(outfile.is_open(), ("could not open output file %s", full_name.c_str()));

    outfile << "# hotness=" << diabloanoptarm_options.af_hotness << ", look_at_nins=" << diabloanoptarm_options.af_hotness_nins << endl;

    for (auto bbl : bbls) {
      if (BBL_INS_FIRST(bbl)) {
        auto x = StringIo("@G:%d:%lld", INS_CADDRESS(BBL_INS_FIRST(bbl)), BBL_NINS(bbl), BBL_EXEC_COUNT(bbl));
        outfile << x << endl;
        Free(x);
      }
      else {
        auto x = StringIo("(empty):%d:%d", BBL_NINS(bbl), BBL_EXEC_COUNT(bbl));
        outfile << x << endl;
        Free(x);
      }
    }

    outfile.close();

    /* exclude basic blocks having the top most executed instructions */
    size_t limit_length = trace_length*diabloanoptarm_options.af_hotness/AF_HOTNESS_MAX;/* tot waar we weghalen */
    VERBOSE(AF_VERBOSITY_LEVEL, (AF "selecting basic blocks for %llu/%llu instructions (omitting %d promille hottest)", limit_length, trace_length, diabloanoptarm_options.af_hotness));

    size_t processed_length = 0;
    size_t selected_length = 0;

    size_t nr_accepted_blocks = 0;
    size_t nr_skipped_blocks = 0;
    for (auto it = bbls.rbegin(); it != bbls.rend(); it++) {
      t_bbl *bbl = *it;
      size_t delta = (look_at_nins ? BBL_NINS(bbl) : 1) * BBL_EXEC_COUNT(bbl);

      processed_length += delta;

      if (processed_length < limit_length) {
        nr_skipped_blocks++;
        BBL_SET_CAN_TRANSFORM(bbl, FALSE);
      }
      else {
        selected_length += delta;
        nr_accepted_blocks++;
      }
    }

    VERBOSE(AF_VERBOSITY_LEVEL, (AF "  ultimately selected %llu/%llu instructions (%d promille) in %llu blocks (%llu blocks not accepted)", selected_length, trace_length, selected_length*AF_HOTNESS_MAX/trace_length, nr_accepted_blocks, nr_skipped_blocks));
  }

  /* canonical representation */
  Canonicalize(cfg);

  ConstantPropagationDisableTransformations(TRUE);
  ConstantPropagationAdvancedFactoringPhase(TRUE);
  ASSERT(ConstantPropagationInit(cfg), ("could not init constant propagation"));
  ConstantPropagation (cfg, CONTEXT_SENSITIVE);
  OptUseConstantInformation(cfg, CONTEXT_SENSITIVE);
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  const_init = true;

#if AF_COPY_ANALYSIS
  ASSERT(CopyAnalysisInit(cfg), ("could not init copy propagation"));
  CopyAnalysis(cfg);
#endif

  /* object tracking */
  af_function_uid = RegisterSpecialFunctionType(AdvancedFactoringOriginTracking);
  VERBOSE(0, (AF "created functions will get UID %d (%x)", af_function_uid, af_function_uid));

  /* */
  CfgComputeSavedChangedRegisters(cfg);

  t_regset sp_only_regset = RegsetNew();
  RegsetSetSingleton(sp_only_regset, ARM_REG_R13);

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    FUNCTION_SET_EXPLICITELY_SAVED(fun, new t_regset());

    /* */
    if (FUNCTION_BBL_FIRST(fun)
        && !BBL_IS_HELL(FUNCTION_BBL_FIRST(fun))
        && FunctionGetExitBlock(fun)) {
      t_regset explicitely_saved = RegsetNew();

      bool at_least_one = false;
      bool invalid_exit = false;
      t_bbl *exit_block = FunctionGetExitBlock(fun);
      t_cfg_edge *e;
      BBL_FOREACH_PRED_EDGE(exit_block, e) {
        if (CFG_EDGE_CAT(e) == ET_COMPENSATING)
          continue;

        ASSERT(CFG_EDGE_CAT(e) == ET_JUMP, ("unsupported edge @E", e));

        t_arm_ins *ins;
        BBL_FOREACH_ARM_INS(CFG_EDGE_HEAD(e), ins) {
          if (!ArmInsIsPop(ins))
            continue;

          /* minus the stack pointer */
          t_regset def = ARM_INS_REGS_DEF(ins);
          RegsetSetSubReg(def, ARM_REG_R13);

          if (RegsetIsEmpty(explicitely_saved))
            explicitely_saved = def;

          if (!RegsetEquals(explicitely_saved, def)) {
            /* in this case, fallback to entry point detection */
            at_least_one = false;
            invalid_exit = true;
            break;
          }
        }

        if (invalid_exit)
          break;

        at_least_one = true;
      }

      if (RegsetIn(explicitely_saved, ARM_REG_R15)) {
        RegsetSetSubReg(explicitely_saved, ARM_REG_R15);
        RegsetSetAddReg(explicitely_saved, ARM_REG_R14);
      }

      if (!at_least_one) {
        /* look at the entry block */
        t_bbl *entry_block = FUNCTION_BBL_FIRST(fun);

        t_arm_ins *ins;

        bool contains_push = false;
        BBL_FOREACH_ARM_INS(entry_block, ins) {
          if (ArmInsIsPush(ins)) {
            contains_push = true;
            break;
          }
        }

        if (!contains_push)
          continue;

        BBL_FOREACH_ARM_INS(entry_block, ins) {
          /* stack pointer magic here (SoftVM glue code),
              we need the second PUSH instruction
            0x164e8 SUB   r13,r13,#0x4
            0x164ec PUSH  {r13}
            0x164f0 PUSH  {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14}
            0x164f4 MRS   r0,CPSR
            0x164f8 PUSH  {r0}
            0x164fc MOV   r4,r13
            0x16500 BL    0x844 abs: 16d4c
            */
          if (!ArmInsIsPush(ins)) {
            ASSERT(RegsetIsEmpty(RegsetDiff(ARM_INS_REGS_DEF(ins), CFG_DESCRIPTION(cfg)->callee_may_change))
                    || RegsetEquals(ARM_INS_REGS_DEF(ins), sp_only_regset), ("no can do @I @eiB\n saved: @X\b changed: @X", ins, entry_block, CPREGSET(cfg, FUNCTION_REGS_SAVED(fun)), CPREGSET(cfg, FUNCTION_REGS_CHANGED(fun))));
            continue;
          }

          if (RegsetEquals(ARM_INS_REGS_USE(ins), sp_only_regset))
            continue;

          explicitely_saved = ARM_INS_REGS_USE(ins);
          RegsetSetSubReg(explicitely_saved, ARM_REG_R13);
          break;
        }
      }

      *FUNCTION_EXPLICITELY_SAVED(fun) = explicitely_saved;
    }
  }

  /* parameters */
  VERBOSE(0, (AF "max set size %d", diabloanoptarm_options.advanced_factoring_max_set_size));
  VERBOSE(0, (AF "master region ID %d", diabloanoptarm_options.advanced_factoring_master_region_id));
  VERBOSE(0, (AF "transformation chance %d", diabloanoptarm_options.advanced_factoring_transformation_chance));
  VERBOSE(0, (AF "only transform executed slice sets? %d", diabloanoptarm_options.af_only_executed));
  VERBOSE(0, (AF "slice set must cover at least archives? %d", diabloanopt_options.factor_min_covered_archives));
  VERBOSE(0, (AF "slice set must cover at least object files? %d", diabloanopt_options.factor_min_covered_objects));
  VERBOSE(0, (AF "slice set must cover at least functions? %d", diabloanopt_options.factor_min_covered_functions));
  VERBOSE(0, (AF "slice set must cover at least executed archives? %d", diabloanopt_options.factor_min_covered_exec_archives));
  VERBOSE(0, (AF "slice set must cover at least executed object files? %d", diabloanopt_options.factor_min_covered_exec_objects));
  VERBOSE(0, (AF "slice set must cover at least executed functions? %d", diabloanopt_options.factor_min_covered_exec_functions));
  VERBOSE(0, (AF "chance to add branch after index instruction (percent) %d", diabloanoptarm_options.advanced_factoring_branch_after_index));
  VERBOSE(0, (AF "minimal block length %d", diabloanoptarm_options.af_min_block_length));
  VERBOSE(0, (AF "maximal block length %d", diabloanoptarm_options.af_max_block_length));
  VERBOSE(0, (AF "shuffle? %d", diabloanoptarm_options.advanced_factoring_shuffle));
  VERBOSE(0, (AF "left better? %d", diabloanoptarm_options.advanced_factoring_leftbetter));
  VERBOSE(0, (AF "right better? %d", diabloanoptarm_options.advanced_factoring_rightbetter));

  /* default values */
  cached_transformation_chance = diabloanoptarm_options.advanced_factoring_transformation_chance;

  /* process the annotated factoring regions and associate the BBLs with them */
  ProcessFactoringRegions(cfg);

  /* do the actual work! */
  FactorByHashTables(cfg, diabloanoptarm_options.advanced_factoring_master_region_id);

  current_af_phase = AFPhase::Finalization;
  CfgConvertPseudoSwaps(cfg);

  LogFactoringPossibilities();

  if (diabloanoptarm_options.af_enable_dispdisttbl) {
    RedirectEmptyDTableEntries();
    ReportDTableStatistics();
  }

  if (global_options.generate_dots) {
    /* patch to single-entry functions for smaller dot graphs */
    CfgRemoveDeadCodeAndDataBlocks (cfg);
    CfgPatchToSingleEntryFunctions (cfg);
    CfgRemoveDeadCodeAndDataBlocks (cfg);

    __DumpDots(cfg, "afteraf", 0);
  }

  /* all slices can be destroyed now.
   * Unprotect all the possibly protected slices in the global list.
   * We don't simply destroy the slices here because InsFiniSlice may kill some of them for us.
   * Slices that will not be destroyed will still be in this list, and can be destroyed after InsFiniSlice is called. */
  for (auto slice : created_slices_global)
    slice->UnprotectFromDestruction();

  DiabloBrokerCall("CleanupAFScheduler", cfg);

  af_dynamic_member_init = FALSE;

  /* commented-out fini's are needed for obfuscation */
  InsFiniAbstractForm(cfg);
  InsFiniSlice(cfg);
  InsFiniFingerprint(cfg);
  InsFiniSliceId(cfg);
  BblFiniFactoringRegionId(cfg);
  BblFiniFactoringTargets(cfg);
  BblFiniSlices(cfg);
  InsFiniMark(cfg);
  InsFiniSlices(cfg);
  InsFiniOrder(cfg);
  BblFiniIncomingForSlice(cfg);
  BblFiniIncoming(cfg);
  BblFiniCanTransform(cfg);
  FiniAdvancedFactoringRNG();
  //CfgEdgeFiniAfCorr(cfg);
  CfgEdgeFiniEquations(cfg);
  CfgEdgeFiniSliceInformation(cfg);
  //BblFiniTableIdVector(cfg);
  BblFiniOriginalId(cfg);
  //FunctionFiniIsAf(cfg);
  //FunctionFiniAfFlags(cfg);
  BblFiniProdProp(cfg);
  FunctionFiniExplicitelySaved(cfg);
  NonZeroAnalysisFini(cfg);
  ArmPrintVirtualRegisters(FALSE);

  ConstantPropagationDisableTransformations(FALSE);
  ConstantPropagationAdvancedFactoringPhase(FALSE);

  {
    t_bbl *bbl;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      if (!BBL_PROCSTATE_IN(bbl))
        continue;

      ProcStateFree(BBL_PROCSTATE_IN(bbl));
      BBL_SET_PROCSTATE_IN(bbl, NULL);
    }
  }

  ConstantPropagationFini(cfg);

  af_init = false;

  /* destroy the remaining slices */
  for (auto slice : created_slices_global)
    delete slice;

  /* patch to single-entry functions */
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  CfgPatchToSingleEntryFunctions (cfg);
  CfgRemoveDeadCodeAndDataBlocks (cfg);

  /* initialise interface for obfuscation transformations */
  AFObfuscationInit(cfg);
  af_obf_init = true;

  /* flush all logs */
  FactoringLogFini();
}

void BblSetAFFlag(t_bbl *bbl, int flag) {
  auto flags = BBL_AF_FLAGS(bbl);
  flags |= flag << 8;
  BBL_SET_AF_FLAGS(bbl, flags);
}

bool BblGetAFFlag(t_bbl *bbl, int flag) {
  return ((BBL_AF_FLAGS(bbl) >> 8) & flag) != 0;
}
void BblSetAFDispatchRegister(t_bbl *bbl, t_reg reg) {
  auto flags = BBL_AF_FLAGS(bbl);
  flags |= static_cast<int>(reg) << 4;
  BBL_SET_AF_FLAGS(bbl, flags);
  ASSERT(reg < ARM_REG_R15, ("unsupported register r%d", reg));
}

t_reg BblGetAFDispatchRegister(t_bbl *bbl) {
  return static_cast<t_reg>((BBL_AF_FLAGS(bbl) >> 4) & 0xf);
}

void BblSetAFDispatchType(t_bbl *bbl, DispatcherType dispatcher) {
  auto flags = BBL_AF_FLAGS(bbl);
  flags |= static_cast<int>(dispatcher) & 0xf;
  BBL_SET_AF_FLAGS(bbl, flags);
}

DispatcherType BblGetAFDispatchType(t_bbl *bbl) {
  return static_cast<DispatcherType>(BBL_AF_FLAGS(bbl) & 0xf);
}
