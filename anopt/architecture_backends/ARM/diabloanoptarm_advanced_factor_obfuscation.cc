#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

struct AFCategorizedDistributedTableInfo {
  map<t_uint32, BblSet> tableid_to_bblset;
  map<t_uint32, BblSet> singletableid_to_bblset;
};
static AFCategorizedDistributedTableInfo dtable_info;

static BblSet bbls_with_index_instruction;
static BblSet bbls_without_index_instruction;
static BblSet landing_site_bbls;

#define INVALID_ID numeric_limits<size_t>::max()
static size_t max_table_id = INVALID_ID;
static set<size_t> all_tables;

typedef function<t_bbl * (t_bbl *, t_randomnumbergenerator *, NewTargetSelector *)> t_random_bbl_selector;

typedef function<t_arm_ins * (t_arm_ins *, t_randomnumbergenerator *)> t_breakup_function;
vector<t_breakup_function> af_breakup_functions;

static
void AddBblToDtableInfo(t_bbl *bbl) {
  if (!BBL_TABLE_ID_VECTOR(bbl)
      || BBL_TABLE_ID_VECTOR(bbl)->size() == 0) {
    dtable_info.tableid_to_bblset[static_cast<t_uint32>(INVALID_ID)].insert(bbl);
  }
  else {
    for (auto id : *BBL_TABLE_ID_VECTOR(bbl)) {
      dtable_info.tableid_to_bblset[id].insert(bbl);

      if (BBL_TABLE_ID_VECTOR(bbl)->size() == 1)
        dtable_info.singletableid_to_bblset[id].insert(bbl);

      if (max_table_id < id
          || max_table_id == INVALID_ID)
        max_table_id = id;
    }
  }
}

static
void CategorizeDistributedTableBbls(t_cfg *cfg) {
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (!BBL_TABLE_ID_VECTOR(bbl))
      continue;

    if (BblSetsTableId(bbl))
      continue;

    if (BblIsExitBlock(bbl))
      continue;

    if (!BblIsValidFakeTarget(bbl))
      continue;

    AddBblToDtableInfo(bbl);
  }

  /* create master list of table ids */
  all_tables.insert(INVALID_ID);
  if (max_table_id != INVALID_ID)
    for (t_uint32 x = 0; x < max_table_id; x++)
      all_tables.insert(x);
}

static
void AFAfterSplitBroker(t_bbl *first, t_bbl *second)
{
  /* this function will also be called from code mobility,
   * but in that case no bookkeeping should be done anymore. */
  if (BBL_CFG(first) != GetMainCfg())
    return;

  DistributedTableCopyIds(first, second);

  /* keep the categorized dtables in-sync */
  if (BblIsValidFakeTarget(second))
    AddBblToDtableInfo(second);

  BBL_SET_AF_FLAGS(second, BBL_AF_FLAGS(first));
  BBL_SET_OBJECT_SET(second, BBL_OBJECT_SET(first));

  /* index block update */
  t_ins *ins;
  BBL_FOREACH_INS(first, ins)
    if (InsIsAfIndexInstruction(ins))
      break;

  if (!ins) {
    bbls_with_index_instruction.erase(first);

    /* no index instruction found in the first BBL */
    bool added = false;
    BBL_FOREACH_INS(second, ins) {
      if (InsIsAfIndexInstruction(ins)) {
        bbls_with_index_instruction.insert(second);
        break;
      }
    }
  }
}

static
void RemoveBblFromCfgBroker(t_bbl *bbl) {
  bbls_with_index_instruction.erase(bbl);
  landing_site_bbls.erase(bbl);

  for (auto x : dtable_info.tableid_to_bblset)
    dtable_info.tableid_to_bblset[x.first].erase(bbl);
  for (auto x : dtable_info.singletableid_to_bblset)
    dtable_info.singletableid_to_bblset[x.first].erase(bbl);
}

static
void AFCanTransformBbl(t_bbl *bbl, bool *result)
{
  /* we can transform most of the time */
  *result = true;

  /* except for these corner cases! */
  t_function *fun = BBL_FUNCTION(bbl);
  if (FUNCTION_IS_AF(fun)
      && (FUNCTION_AF_FLAGS(fun) & AF_FLAG_DIRTY_SP)) {
    DEBUG(("can't transform because of AF dirty SP @F", fun));
    *result = false;
  }

  if (BblGetAFFlag(bbl, AF_FLAG_DIRTY_SP)) {
    DEBUG(("can't transform because of AF dirty SP @iB", bbl));
    *result = false;
  }
}

#define ADDSUB_MAX_IMM 0xff
static
t_arm_ins *breakupADD(t_arm_ins *index_ins, t_randomnumbergenerator *rng) {
  t_arm_ins *result = NULL;
  if (ARM_INS_IMMEDIATE(index_ins) == 0
      || ARM_INS_OPCODE(index_ins) == ARM_ADDRESS_PRODUCER)
    /* we rely on the fallback functionality in this case */
    return result;

  /* calculate random delta */
  t_uint32 delta = RNGGenerateWithRange(rng, 0, ARM_INS_IMMEDIATE(index_ins) % ADDSUB_MAX_IMM);

  /* compensate existing index instruction and add new ADD instruction */
  ARM_INS_SET_IMMEDIATE(index_ins, ARM_INS_IMMEDIATE(index_ins) - delta);
  ArmMakeInsForIns(Add, After, result, index_ins, false, ARM_INS_REGA(index_ins), ARM_INS_REGA(index_ins), ARM_REG_NONE, delta, ARM_CONDITION_AL);
  ARM_INS_SET_PHASE(result, ARM_INS_PHASE(index_ins));
  ARM_INS_SET_TRANSFORMATION_ID(result, ARM_INS_TRANSFORMATION_ID(index_ins));
  LOG_MESSAGE(L_TARGETSELECTOR, ",add(@I/%d)", result, delta);

  return result;
}

static
t_arm_ins *breakupSUB(t_arm_ins *index_ins, t_randomnumbergenerator *rng) {
  t_arm_ins *result = NULL;
  if (ARM_INS_IMMEDIATE(index_ins) == 0
      || ARM_INS_OPCODE(index_ins) == ARM_ADDRESS_PRODUCER)
    /* we rely on the fallback functionality in this case */
    return result;

  /* calculate random delta */
  t_uint32 delta = RNGGenerateWithRange(rng, 0, ARM_INS_IMMEDIATE(index_ins) % ADDSUB_MAX_IMM);

  /* compensate existing index instruction and add new ADD instruction */
  ARM_INS_SET_IMMEDIATE(index_ins, ARM_INS_IMMEDIATE(index_ins) + delta);
  ArmMakeInsForIns(Sub, After, result, index_ins, false, ARM_INS_REGA(index_ins), ARM_INS_REGA(index_ins), ARM_REG_NONE, delta, ARM_CONDITION_AL);
  ARM_INS_SET_PHASE(result, ARM_INS_PHASE(index_ins));
  ARM_INS_SET_TRANSFORMATION_ID(result, ARM_INS_TRANSFORMATION_ID(index_ins));
  LOG_MESSAGE(L_TARGETSELECTOR, ",sub(@I/%d)", result, delta);

  return result;
}

static
t_arm_ins *breakupFallback(t_arm_ins *index_ins, t_randomnumbergenerator *rng) {
  t_arm_ins *result = NULL;

  /* calculate random delta, starting from '1' here because index is 0 */
  t_uint32 delta = RNGGenerateWithRange(rng, 1, ADDSUB_MAX_IMM);

  /* compensate existing index instruction and add new ADD instruction */
  if (ARM_INS_OPCODE(index_ins) == ARM_ADDRESS_PRODUCER) {
    /* address producer, change TO offset */

    /* ARM instructions have a fixed length of 4 (we don't support Thumb for now) */
    delta &= ~0x3;

    t_reloc *rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(index_ins));
    RELOC_TO_RELOCATABLE_OFFSET(rel)[0] = AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(rel)[0], delta);
  }
  else {
    /* regular instruction, change IMMEDIATE value */
    ARM_INS_SET_IMMEDIATE(index_ins, ARM_INS_IMMEDIATE(index_ins) + delta);
  }
  ArmMakeInsForIns(Sub, After, result, index_ins, false, ARM_INS_REGA(index_ins), ARM_INS_REGA(index_ins), ARM_REG_NONE, delta, ARM_CONDITION_AL);
  ARM_INS_SET_PHASE(result, ARM_INS_PHASE(index_ins));
  ARM_INS_SET_TRANSFORMATION_ID(result, ARM_INS_TRANSFORMATION_ID(index_ins));
  LOG_MESSAGE(L_TARGETSELECTOR, ",fallback(@I/%d)", result, delta);

  return result;
}

static
t_bbl *dtable_random_target(t_bbl *bbl, t_randomnumbergenerator *rng, NewTargetSelector *selector) {
  /* construct list of possible tables */
  set<size_t> possible_tables = all_tables;
  LOG_MESSAGE(L_TARGETSELECTOR, "dtable");

  set<size_t> tables = BblAssociatedTables(bbl);
  if (tables.size() == 0) {
    /* no table associated with this block */
    if (possible_tables.size() > 1)
      possible_tables.erase(INVALID_ID);
  }
  else {
    for (auto x : tables)
      possible_tables.erase(x);

    /* if the final possibility list turns out to be empty,
     * re-add the list of associated tables so we can randomly choose one properly. */
    if (possible_tables.size() == 0)
      possible_tables = tables;
  }

  /* choose random table from list of possibilities
   * and select associated list of target BBLs */
  auto it = possible_tables.begin();
  if (possible_tables.size() > 1)
    advance(it, RNGGenerateWithRange(rng, 0, possible_tables.size() - 1));

  BblSet all_possible_targets = dtable_info.tableid_to_bblset[*it];
  LOG_MESSAGE(L_TARGETSELECTOR, ",table[%d]", *it);

  /* filter out invalid targets */
  BblSet possible_targets;
  for (auto bbl : all_possible_targets)
    if (BblIsValidFakeTarget(bbl))
      possible_targets.insert(bbl);

  /* choose random target BBL */
  auto itt = possible_targets.begin();
  if (possible_targets.size() == 0)
    return NULL;

  if (possible_targets.size() > 1)
    advance(itt, RNGGenerateWithRange(rng, 0, possible_targets.size() - 1));

  return *itt;
}

static
t_bbl *entry_random_target(t_bbl *bbl, t_randomnumbergenerator *rng, NewTargetSelector *selector) {
  t_bbl *result = NULL;
  LOG_MESSAGE(L_TARGETSELECTOR, "af-entry");

  BblSet bbls_with_index_instruction_candidates;
  BblSet bbls_without_index_instruction_candidates;

  TransformationID tf_id = selector->SelectFromTransformationID();

  /* only candidates from specific transformation, if possible */
  BblSet bbls_for_tf_id = BblsForTransformationID(tf_id, rng, &all_af_tf_ids);
  bool fallback_list = false;
  if (bbls_for_tf_id.size() > 0) {
    LOG_MESSAGE(L_TARGETSELECTOR, ",transformation(%d)", tf_id)

    bbls_for_tf_id = selector->FilterTargets(bbl, bbls_for_tf_id, true, tf_id);

    for (auto bbl : bbls_for_tf_id) {
      if (bbls_with_index_instruction.find(bbl) != bbls_with_index_instruction.end())
        bbls_with_index_instruction_candidates.insert(bbl);

      if (bbls_without_index_instruction.find(bbl) != bbls_without_index_instruction.end())
        bbls_without_index_instruction_candidates.insert(bbl);
    }

    if (bbls_with_index_instruction_candidates.size() == 0
        && bbls_without_index_instruction_candidates.size() == 0) {
      bbls_without_index_instruction_candidates = bbls_for_tf_id;
      fallback_list = true;
    }
  }
  else {
    /* all possible candiates */
    bbls_with_index_instruction_candidates = selector->FilterTargets(bbl, bbls_with_index_instruction, true, tf_id);
    bbls_without_index_instruction_candidates = selector->FilterTargets(bbl, bbls_without_index_instruction, true, tf_id);
  }

  /* don't pick a BBL in the same function */
  if ((tf_id != INVALID_TRANSFORMATION_ID)
      && (BblTransformationID(bbl) == tf_id)) {
    /* target in itself */
  }
  else {
    bbls_with_index_instruction_candidates.erase(bbl);
    bbls_without_index_instruction_candidates.erase(bbl);
  }

  auto use_existing = RNGGenerateBool(rng);

  if (bbls_with_index_instruction_candidates.size() > 0
      && (!use_existing
          || bbls_without_index_instruction_candidates.size() == 0)) {
    /* pick random BBL with index instruction */
    auto itt = bbls_with_index_instruction_candidates.begin();
    if (bbls_with_index_instruction_candidates.size() > 1)
      advance(itt, RNGGenerateWithRange(rng, 0, bbls_with_index_instruction_candidates.size() - 1));

    auto it = bbls_with_index_instruction.find(*itt);
    ASSERT(it != bbls_with_index_instruction.end(), ("@eiB not found", *itt));

    LOG_MESSAGE(L_TARGETSELECTOR, ",split");

    t_bbl *index_block = *it;
    BblShouldBePartOfTransformation(index_block, tf_id);

    ASSERT(BblAssociatedInfo(index_block), ("no associated information for @eiB", index_block));

    /* look for index instruction */
    t_ins *index_ins = NULL;
    BBL_FOREACH_INS(index_block, index_ins)
      if (InsIsAfIndexInstruction(index_ins))
        break;
    if (!index_ins)
      CfgDrawFunctionGraphsWithHotness(BBL_CFG(bbl), "index");
    ASSERT(index_ins, ("expected index instruction in @eiB", index_block));

    if (index_ins == BBL_INS_LAST(index_block)) {
      /* need to add an instruction */

      /* we assume that the index instruction is a constant producer */
      t_arm_ins *arm_index_ins = T_ARM_INS(index_ins);
      ASSERT(ARM_INS_OPCODE(arm_index_ins) == ARM_CONSTANT_PRODUCER
              || ARM_INS_OPCODE(arm_index_ins) == ARM_ADDRESS_PRODUCER, ("expected index instruction to be constant/address producer, got @I in @eiB", arm_index_ins, index_block));

      auto fun = af_breakup_functions[RNGGenerateWithRange(rng, 0, af_breakup_functions.size()-1)];
      t_arm_ins *new_ins = fun(arm_index_ins, rng);
      if (!new_ins)
        new_ins = breakupFallback(arm_index_ins, rng);
      /* we need this to count false positives/negatives afterwards */
      InsMarkAfIndexInstruction(T_INS(new_ins));
      LOG_MESSAGE(L_TARGETSELECTOR, ",added(@I)", new_ins);
    }

    /* split the block after the index instruction */
    result = BblSplitBlock(index_block, index_ins, FALSE);
    BblTransformationIDAfterSplit(index_block, result);

    /* add branch instruction after index instruction */
    if (RNGGeneratePercent(rng) <= static_cast<t_uint32>(diabloanoptarm_options.advanced_factoring_branch_after_index)) {
      LOG_MESSAGE(L_TARGETSELECTOR, ",branch");

      t_arm_ins *branch_ins;
      ArmMakeInsForBbl(UncondBranch, Append, branch_ins, index_block, FALSE);

      CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(index_block), ET_JUMP);
    }

    /* bookkeeping */
    bbls_with_index_instruction.erase(it);
    bbls_without_index_instruction.insert(result);

    ASSERT(BblAssociatedInfo(result), ("no associated information for @eiB", result));
  }
  else {
    if (bbls_without_index_instruction_candidates.size() > 0) {
      /* pick random BBL */
      LOG_MESSAGE(L_TARGETSELECTOR, ",existing");

      auto itt = bbls_without_index_instruction_candidates.begin();
      if (bbls_without_index_instruction_candidates.size() > 1)
        advance(itt, RNGGenerateWithRange(rng, 0, bbls_without_index_instruction_candidates.size() - 1));

      auto it = bbls_without_index_instruction.find(*itt);
      ASSERT(it != bbls_without_index_instruction.end() || fallback_list, ("@eiB not found", *itt));

      result = *itt;
      BblShouldBePartOfTransformation(result, tf_id);
    }
    else {
      result = NULL;
    }
  }

  return result;
}

static
t_bbl *landing_site_target(t_bbl *bbl, t_randomnumbergenerator *rng, NewTargetSelector *selector) {
  t_bbl *result = NULL;
  LOG_MESSAGE(L_TARGETSELECTOR, "af-landing");

  BblSet candidates;

  TransformationID tf_id = selector->SelectFromTransformationID();

  /* only candidates from specific transformation, if possible */
  BblSet all_bbls_for_tf_id = BblsForTransformationID(tf_id, rng);

  BblSet bbls_for_tf_id;
  for (auto bbl : all_bbls_for_tf_id)
    if (BBL_IS_LANDING_SITE(bbl))
      bbls_for_tf_id.insert(bbl);

  if (bbls_for_tf_id.size() > 0) {
    LOG_MESSAGE(L_TARGETSELECTOR, ",transformation(%d)", tf_id)

    candidates = selector->FilterTargets(bbl, bbls_for_tf_id, true, tf_id);
  }
  else {
    /* all possible candiates */
    candidates = selector->FilterTargets(bbl, landing_site_bbls, true, tf_id);
  }

  /* don't pick a BBL in the same function */
  if ((tf_id != INVALID_TRANSFORMATION_ID)
      && (BblTransformationID(bbl) == tf_id)) {
    /* target in itself */
  }
  else
    candidates.erase(bbl);

  /* filter out invalid targets */
  BblSet possible_targets;
  for (auto bbl : candidates)
    if (BblIsValidFakeTarget(bbl))
      possible_targets.insert(bbl);
  ASSERT(possible_targets.size() > 0, ("not enough targets in transformation %d", tf_id));

  auto it = possible_targets.begin();
  if (possible_targets.size() > 1)
    advance(it, RNGGenerateWithRange(rng, 0, possible_targets.size() - 1));

  result = *it;
  BblShouldBePartOfTransformation(result, tf_id);

  return result;
}

void AFCheckIndexBlocksBroker(t_cfg *cfg) {
  BblSet new_bbls_with_index_instruction;
  for (auto bbl : bbls_with_index_instruction) {
    t_ins *ins;
    bool added = false;
    BBL_FOREACH_INS(bbl, ins) {
      if (InsIsAfIndexInstruction(ins)) {
        new_bbls_with_index_instruction.insert(bbl);
        added = true;
        break;
      }
    }
  }
  bbls_with_index_instruction = new_bbls_with_index_instruction;
}

void AFObfuscationInit(t_cfg *cfg)
{
  InstallTargetHandler(new AdvancedFactoringSelector());

  DiabloBrokerCallInstall("AFAfterSplit", "t_bbl *, t_bbl *", (void *)AFAfterSplitBroker, FALSE);
  DiabloBrokerCallInstall("AFCanTransformBbl", "t_bbl *, t_bool *", (void *)AFCanTransformBbl, FALSE);
  DiabloBrokerCallInstall("RemoveBblFromCfg", "t_bbl *", (void *)RemoveBblFromCfgBroker, FALSE);
  DiabloBrokerCallInstall("AFCheckIndexBlocks", "t_cfg *", (void *)AFCheckIndexBlocksBroker, FALSE);

  /* record distributed table information */
  CategorizeDistributedTableBbls(cfg);

  /* record dispatcher entry points (BBLs with index instructions) */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    t_ins *ins;
    BBL_FOREACH_INS(bbl, ins) {
      if (InsIsAfIndexInstruction(ins)) {
        bbls_with_index_instruction.insert(bbl);
        break;
      }
    }

    if (BBL_IS_LANDING_SITE(bbl))
      landing_site_bbls.insert(bbl);
  }
  LOG_MESSAGE(L_TARGETSELECTOR, "# %d AF index blocks, %d AF landing sites\n", bbls_with_index_instruction.size(), landing_site_bbls.size());

  /* possible breakup functions */
  af_breakup_functions.push_back(breakupADD);
  af_breakup_functions.push_back(breakupSUB);
}

AdvancedFactoringSelector::AdvancedFactoringSelector() {
  RegisterTransformationType(this, _name);
}

static
bool CanTransformIndex(const t_bbl *bbl, TransformationID tf_id) {
  /* try to list the basic blocks for this transformation */
  if (tf_id >= 0) {
    BblSet bbls_for_tf_id = BblsForTransformationID(tf_id, NULL);
    BblSet with, without;
    if (bbls_for_tf_id.size() > 0) {
      for (auto bbl : bbls_for_tf_id) {
        if (bbls_with_index_instruction.find(bbl) != bbls_with_index_instruction.end())
          with.insert(bbl);

        if (bbls_without_index_instruction.find(bbl) != bbls_without_index_instruction.end())
          without.insert(bbl);
      }
    }
    if ((with.size() == 0) && (without.size() == 0))
      return false;
  }
  else if ((bbls_with_index_instruction.size() == 0) && (bbls_without_index_instruction.size() == 0))
    return false;

  /* no AF transformations applied */
  if (all_af_tf_ids.size() == 0)
    return false;

  return true;
}

static
bool CanTransformTable(const t_bbl *bbl, TransformationID tf_id) {
  if (all_tables.size() <= 1)
    return false;

  /* default case: multiple tables available, assume that this is always possible */
  return true;
}

static
bool CanTransformLanding(const t_bbl *bbl, TransformationID tf_id) {
  if (tf_id >= 0) {
    BblSet bbls_for_tf_id = BblsForTransformationID(tf_id, NULL);

    for (auto bbl : bbls_for_tf_id)
      if (BBL_IS_LANDING_SITE(bbl))
        return true;

    return false;
  }
  else if (landing_site_bbls.size() == 0)
    return false;

  return true;
}

bool AdvancedFactoringSelector::canTransform(const t_bbl* bbl) const {
  /* code mobility */
  if (BBL_CFG(bbl) != GetMainCfg())
    return false;
  
  if (all_af_tf_ids.find(SelectFromTransformationID()) == all_af_tf_ids.end())
    return false;

  if (CanTransformIndex(bbl, SelectFromTransformationID())
        || CanTransformTable(bbl, SelectFromTransformationID())
        || CanTransformLanding(bbl, SelectFromTransformationID()))
    return true;

  return false;
}

t_bbl* AdvancedFactoringSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  vector<t_random_bbl_selector> af_selectors;
  if (CanTransformIndex(bbl, SelectFromTransformationID()))
    af_selectors.push_back(entry_random_target);
  if (CanTransformTable(bbl, SelectFromTransformationID()))
    af_selectors.push_back(dtable_random_target);
  if (CanTransformLanding(bbl, SelectFromTransformationID()))
    af_selectors.push_back(landing_site_target);

  auto it = af_selectors.begin();
  if (af_selectors.size() > 0)
    advance(it, RNGGenerateWithRange(rng, 0, af_selectors.size() - 1));
  auto random_function = *it;

  t_bbl *result = random_function(bbl, rng, this);
  if (!result)
    CfgDrawFunctionGraphs(BBL_CFG(bbl), "not-found");
  ASSERT(result, ("no BBL found @eiB", bbl));

  /* fix for single-entry point functionality */
  if (BblIsReturnSite(result)) {
    if (BBL_NINS(result) == 0)
      result = CFG_EDGE_TAIL(BBL_SUCC_FIRST(result));
    else {
      t_bbl *split_off = BblSplitBlock(result, BBL_INS_FIRST(result), TRUE);
      AFAfterSplitBroker(result, split_off);
      BblTransformationIDAfterSplit(result, split_off);

      result = split_off;
    }
  }

  return result;
}

t_bbl* AdvancedFactoringSelector::doTransform(t_cfg *cfg, t_randomnumbergenerator * rng) {
  FATAL(("implement me"));
}
