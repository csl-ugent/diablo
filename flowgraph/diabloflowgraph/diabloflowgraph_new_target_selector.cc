/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "diabloflowgraph.hpp"
#include <sstream>

//#define DEBUG_TARGETS
#define DEBUG_TARGETS_DEBUGCOUNTER diablosupport_options.debugcounter
static t_uint32 nr_selected_targets = 0;

using namespace std;

vector<NewTargetSelector *> installed_target_handlers;

static bool cfg_to_postpass_callback_init = false;
static map<t_cfg*, t_bbl*> cfg_to_hell;

static ObfuscationSplitSelector *obfuscation_split_selector = NULL;
static GlobalPostPassTargetSelector *global_target_selector = NULL;

/* Given a BBL for which a fake target should be chosen:
 * false: the target can have one or more overlapping covered libraries
 * true : the target can't have any overlapping covered libraries */
#define STRICT_INTERLIB_TARGET true

LogFile* L_TARGETSELECTOR = NULL;

template<typename T>
static
set<T> intersect(set<T>& x, set<T>& y) {
  set<T> result;
  set_intersection(x.begin(), x.end(), y.begin(), y.end(), inserter(result, result.begin()));
  return result;
}

template<typename T>
static
string setToString(set<T>& x) {
  stringstream ss;
  for (auto y : x)
    ss << y << ",";
  return ss.str();
}

bool BblIsValidFakeTarget(t_bbl *bbl) {
  if (BBL_NINS(bbl) == 1
      && CFG_DESCRIPTION(BBL_CFG(bbl))->InsIsUnconditionalJump(BBL_INS_FIRST(bbl))) {
    t_cfg_edge *p;
    BBL_FOREACH_PRED_EDGE(bbl, p) {
      t_cfg_edge *s;
      BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(p), s)
        if (CFG_EDGE_CAT(s) == ET_SWITCH
            || CFG_EDGE_CAT(s) == ET_IPSWITCH)
          return false;
    }
  }

  if (BBL_NINS(bbl) > 0
      && INS_IS_DATA(BBL_INS_FIRST(bbl)))
    return false;

  if (BBL_NINS(bbl) == 0)
    return false;

  return true;
}

/* return TRUE when the target can be considered an option */
static
bool TargetFilter(t_bbl *from, t_bbl *target, TransformationID tf_id) {
  /* only select other libraries */
  auto from_assoc = BblAssociatedInfo(from);
  ASSERT(from_assoc != NULL, ("no associated information for @eiB", from));

  /* due to code mobility */
  if (BBL_CFG(from) != BBL_CFG(target))
    return false;

  if (BBL_CFG(from) != GetMainCfg()) {
    /* from code mobility CFG */
    if (IS_DATABBL(target))
      return false;
  }
  else {
    /* from original CFG */
    auto to_assoc = BblAssociatedInfo(target);
    if (!to_assoc && IS_DATABBL(target))
      return false;
    ASSERT(to_assoc != NULL, ("no associated information for @eiB", target));

    if (STRICT_INTERLIB_TARGET) {
      /* strict criterion */
      if (intersect(from_assoc->archives, to_assoc->archives).size() != 0)
        return false;
    }
  }

  return true;
}

NewTargetSelector::NewTargetSelector() {
  AddOptionsListInitializer(diabloflowgraph_new_target_selector_option_list); DiabloFlowgraphNewTargetSelectorCmdlineInit();

  RegisterTransformationType(this, _name);
  _tf_id = INVALID_TRANSFORMATION_ID;
}

BblSet NewTargetSelector::FilterTargets(t_bbl *bbl, BblSet targets, bool fallback_if_empty, TransformationID tf_id) const {
  if (_target_filter == nullptr)
    return targets;

  BblSet result;
  for (auto x : targets) {
    if (!BblIsValidFakeTarget(x))
      continue;

    if (_target_filter(bbl, x, tf_id))
      result.insert(x);
  }

  if (fallback_if_empty
      && result.size() == 0) {
    VERBOSE(0, ("no more targets for @iB (targets in transformation %d)", bbl, tf_id));

    if (BBL_CFG(bbl) != GetMainCfg()) {
      t_bbl *x;
      CFG_FOREACH_BBL(BBL_CFG(bbl), x) {
        if (!BBL_IS_HELL(x))
          continue;

        if (BBL_NINS(x) > 0
            && INS_IS_DATA(BBL_INS_FIRST(x)))
          continue;

        result.insert(x);
      }
    }
    else
      result = targets;
  }

  return result;
}

InfiniteLoopSelector::InfiniteLoopSelector() {
  RegisterTransformationType(this, _name);
}

NewTargetInSameFunctionSelector::NewTargetInSameFunctionSelector() {
  RegisterTransformationType(this, _name);
}

ObfuscationSplitSelector::ObfuscationSplitSelector() {
  RegisterTransformationType(this, _name);
}

void InstallGenericNewTargetHandlers() {
  installed_target_handlers.clear();

  installed_target_handlers.push_back(new NewTargetInSameFunctionSelector());

  global_target_selector = new GlobalPostPassTargetSelector();
  installed_target_handlers.push_back(global_target_selector);

  obfuscation_split_selector = new ObfuscationSplitSelector();
  installed_target_handlers.push_back(obfuscation_split_selector);

  /* Always install this one as backup when no other one is available */
  installed_target_handlers.push_back(new InfiniteLoopSelector());
}

void InitTargetSelectorLogging(string filename) {
  INIT_LOGGING(L_TARGETSELECTOR, filename.c_str());
}

void InstallTargetHandler(NewTargetSelector *nts) {
  installed_target_handlers.push_back(nts);
}

void DestroyGenericNewTargetHandlers() {
  for (auto i : installed_target_handlers)
    delete i;

  FINI_LOGGING(L_TARGETSELECTOR);
}

static
void RemoveGenericNewTargetHandler(t_const_string name) {
  Transformation *transfo = nullptr;

  /* can't use range-based for here because we need the iterator instance */
  for (auto it = installed_target_handlers.begin(); it != installed_target_handlers.end(); it++) {
    if (!strcmp(name, (*it)->name())) {
      transfo = *it;
      installed_target_handlers.erase(it);
      break;
    }
  }

  if (transfo == nullptr)
    return;

  UnregisterTransformationType(transfo, name);
  UnregisterTransformationType(transfo, "newtargetselector");
  delete transfo;
}

bool ObfuscationSplitSelector::canTransform(const t_bbl* bbl) const {
  if (BBL_CFG(bbl) != GetMainCfg())
    return false;

  return possible_targets.size();
}

t_bbl *ObfuscationSplitSelector::doTransform(t_bbl *bbl, t_randomnumbergenerator *rng) {
  BblSet options;

  auto tf_id = SelectFromTransformationID();
  BblSet bbls_for_tf_id = BblsForTransformationID(tf_id, rng);

  if (bbls_for_tf_id.size() == 0) {
    /* all possible options */
    options = possible_targets;
  }
  else {
    /* only options are from selected transformation */
    options = bbls_for_tf_id;
  }

  options = FilterTargets(bbl, options, true, tf_id);

  options.erase(bbl);

  if (options.size() == 0)
    options.insert(bbl);

  /* choose random BBL */
  auto it = options.begin();
  if (options.size() > 1)
    advance(it, RNGGenerateWithRange(rng, 0, options.size() - 1));

  auto it_chosen = options.find(*it);
  t_bbl *chosen = *it_chosen;

  BblShouldBePartOfTransformation(chosen, tf_id);

  auto BblGetNthInstruction = [](t_bbl *bbl, int n) {
    t_ins *result = BBL_INS_FIRST(bbl);

    while (n > 0) {
      result = INS_INEXT(result);
      n--;
    }

    return result;
  };

  if (BBL_NINS(chosen) > 1
      /*&& RNGGenerateBool(rng)*/) {
    t_ins *tmp;

    /* split the block */
    LOG_MESSAGE(L_TARGETSELECTOR, "split");

    t_ins *ins = NULL;
    int split_after = 0;
    bool do_split = TRUE;
    if (tf_id == INVALID_TRANSFORMATION_ID) {
      if (BBL_NINS(chosen) > 2)
        split_after = RNGGenerateWithRange(rng, 0, BBL_NINS(chosen)-2);
      
      ins = BblGetNthInstruction(chosen, split_after);
    }
    else {
      bool last_ins_is_tf = (GetTransformationNumberFromId(INS_TRANSFORMATION_ID(BBL_INS_LAST(chosen))) == tf_id);

      /* count total number of transformation instructions */
      int nr_total_tf_insns = 0;
      BBL_FOREACH_INS(chosen, tmp) {
        if (GetTransformationNumberFromId(INS_TRANSFORMATION_ID(tmp)) == tf_id)
          nr_total_tf_insns++;
      }
      ASSERT(nr_total_tf_insns > 0, ("no instructions for TF%d found in @eiB", tf_id, chosen));

      /* count the number of non-final transformation instructions */
      int nr_nonfinal_tf_insns = nr_total_tf_insns;
      if (last_ins_is_tf)
        nr_nonfinal_tf_insns--;

      if (nr_total_tf_insns == 1) {
        /* only one TF instruction in the block, no smart decision can be made here */
        if (BBL_NINS(chosen) > 2)
          split_after = RNGGenerateWithRange(rng, 0, BBL_NINS(chosen)-2);
        ins = BblGetNthInstruction(chosen, split_after);

        if (GetTransformationNumberFromId(INS_TRANSFORMATION_ID(BBL_INS_FIRST(chosen))) == tf_id)
          do_split = FALSE;
        else
          WARNING(("WARNING only has one TF%d instruction, splitting after @I: @eiB", tf_id, ins, chosen));
      }
      else {
        /* multiple TF instructions */
        int max_index = nr_nonfinal_tf_insns - 1;
        if (!last_ins_is_tf)
          max_index--;
        
        split_after = max_index;
        if (max_index > 0)
          split_after = RNGGenerateWithRange(rng, 0, max_index);
        
        int nr_seen_tf_ins = 0;
        BBL_FOREACH_INS(chosen, tmp) {
          if (GetTransformationNumberFromId(INS_TRANSFORMATION_ID(tmp)) == tf_id) {
            if (nr_seen_tf_ins == split_after) {
              ins = tmp;
              break;
            }

            nr_seen_tf_ins++;
          }
        }
      }
    }
    ASSERT(ins, ("what? no instruction found at %d @eiB", split_after, chosen));
    ASSERT(ins != BBL_INS_LAST(chosen), ("what? chose last instruction @I in @eiB", ins, chosen));
    LOG_MESSAGE(L_TARGETSELECTOR, ",@I", ins);

    t_bbl *split_off = chosen;
    if (do_split) {
      split_off = BblSplitBlock(chosen, ins, FALSE);
      DiabloBrokerCall("AFAfterSplit", chosen, split_off);
      BblTransformationIDAfterSplit(chosen, split_off);
    }

    possible_targets.insert(split_off);
    chosen = split_off;
    LOG_MESSAGE(L_TARGETSELECTOR, ",%p", chosen);
  }

  return chosen;
}

t_bbl *ObfuscationSplitSelector::doTransform(t_cfg *cfg, t_randomnumbergenerator *rng) {
  FATAL(("implement me"));
}

void ObfuscationSplitSelector::registerTarget(t_bbl *bbl) {
  possible_targets.insert(bbl);
  nr_total++;
}

void ObfuscationSplitSelectorRegisterTarget(t_bbl *bbl) {
  ASSERT(obfuscation_split_selector != NULL, ("obfuscation split selector instance not created"));
  obfuscation_split_selector->registerTarget(bbl);
}

bool InfiniteLoopSelector::canTransform(const t_bbl*) const {
  /* This one can always transform, but only return true if it is enabled. Thus, we can always install
   * this selector, and then it can get randomly chosen if it is explicitly enabled, and chosen as fallback if not. */
  return diabloflowgraph_new_target_selector_options.new_target_infinite_loop;
}

t_bbl* InfiniteLoopSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_function* fun = BBL_FUNCTION(bbl);
  t_cfg* cfg = FUNCTION_CFG(fun);

  t_bbl* rubbish = BblNew(cfg);
  DiabloBrokerCall("CopyToNewTargetBBL", bbl, rubbish);
  BblInsertInFunction(rubbish, fun);
  BblCopyAssociatedInfo(bbl, rubbish);

  // ArmMakeInsForBbl(Mov,  Append, ins, rubbish, ARM_REG_PC, ARM_REG_NONE, 0, ARM_CONDITION_AL); // Just crash TODO make this possible generically

  GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(rubbish);
  CfgEdgeCreate(cfg, rubbish, rubbish, ET_JUMP);

  return rubbish;
}

t_bbl* InfiniteLoopSelector::doTransform(t_cfg *cfg, t_randomnumbergenerator * rng) {
  FATAL(("implement me"));
}

static t_int32 countBasicBlocks(t_function * fun)
{
  t_int32 nr_bbl=0;
  t_bbl * bbl;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    nr_bbl++;
  }

  return nr_bbl;
}

bool NewTargetInSameFunctionSelector::canTransform(const t_bbl* bbl) const {
  // return countBasicBlocks(BBL_FUNCTION(bbl)) > 1; // TODO, this might make more sense...
  return diabloflowgraph_new_target_selector_options.new_target_in_same_function;
}

/* TODO: this might/will screw liveness analysis!?! */
t_bbl* NewTargetInSameFunctionSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  /*  */
  vector<t_bbl*> bbls;
  t_function* fun = BBL_FUNCTION(bbl);
  t_bbl* bbl_it;

  FUNCTION_FOREACH_BBL(fun, bbl_it) {
    /* don't redirect to Diablo-specific RETURN blocks */
    if (BblIsExitBlock(bbl_it))
      continue;

    bbls.push_back(bbl_it);
  }

  /* so this can actually return bbl again... */
  return bbls.at(RNGGenerateWithRange(rng, 0, bbls.size() - 1));
}

t_bbl* NewTargetInSameFunctionSelector::doTransform(t_cfg *cfg, t_randomnumbergenerator * rng) {
  FATAL(("implement me"));
}

TransformationID BblTransformationID(t_bbl *bbl) {
  TransformationID bbl_tf_id = INVALID_TRANSFORMATION_ID;

  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins) {
    TransformationID tf_id = GetTransformationNumberFromId(INS_TRANSFORMATION_ID(ins));
    if (tf_id < 0)
      continue;

    if (!((bbl_tf_id == tf_id) || (bbl_tf_id == INVALID_TRANSFORMATION_ID) || CFG_DESCRIPTION(BBL_CFG(bbl))->InsIsUnconditionalJump(ins)))
      WARNING(("multiple transformation IDs in one block @iB", bbl));

    bbl_tf_id = tf_id;
  }

  return bbl_tf_id;
}

t_bbl* SelectTargetFor(t_bbl* bbl, t_randomnumbergenerator* rng, bool need_to_fall_through, TransformationID tf_id, PossibleTargetSelectorTargetFilter target_filter) {
  if (tf_id >= 0)
    ASSERT(TransformationMetadataExists(tf_id), ("SelectTargetFor called with transformation ID %d, but this transformation does not exist @eiB", tf_id, bbl));

  NewTargetSelector* selector = 0;
  auto possible = GetTransformationsForType("newtargetselector");

  string source_function_name = "(noname)";
  if (BBL_FUNCTION(bbl)
      && FUNCTION_NAME(BBL_FUNCTION(bbl)))
    source_function_name = FUNCTION_NAME(BBL_FUNCTION(bbl));
  START_LOGGING_TRANSFORMATION_NONEWLINE(L_TARGETSELECTOR, "NewTargetSelector,%x(%x),%s,%d,%d", BBL_CADDRESS(bbl), BBL_OLD_ADDRESS(bbl), source_function_name.c_str(), need_to_fall_through, BblTransformationID(bbl))

  if (diabloflowgraph_new_target_selector_options.new_target_all_global)
    selector = global_target_selector;
  else {
    while (!selector && possible.size() > 0) {
      auto rand = RNGGenerateWithRange(rng, 0, possible.size() - 1);
      auto it   = possible.begin();

      advance(it, rand);
      auto s    = dynamic_cast<NewTargetSelector*>(*it);
      s->SetSelectFromTransformationID(tf_id);
      s->SetPossibleTargetSelectorTargetFilterFunction(target_filter);

      if (s->canTransform(bbl))
        selector = s;

      possible.erase(it);
    }

    if (!selector
        || diabloflowgraph_new_target_selector_options.new_target_disable
#ifdef DEBUG_TARGETS
        || nr_selected_targets >= DEBUG_TARGETS_DEBUGCOUNTER
#endif
        ) {
      selector = dynamic_cast<NewTargetSelector*>(GetTransformationsForType("newtargetselector:infiniteloop").at(0));
      VERBOSE(0, ("Falling back to default infiniteloop selector"));
    }
  }

  nr_selected_targets++;

  VERBOSE(0, ("Chose new target selector '%s': TF%d -> TF%d", selector->name(), BblTransformationID(bbl), tf_id));
  LOG_MESSAGE(L_TARGETSELECTOR, ",%s", selector->name())

  LOG_MESSAGE(L_TARGETSELECTOR, ",[");
  t_bbl* target = selector->doTransform(bbl, rng);
  LOG_MESSAGE(L_TARGETSELECTOR, "]");

  string target_function_name = "(noname)";
  if (BBL_FUNCTION(target)
      && FUNCTION_NAME(BBL_FUNCTION(target)))
    target_function_name = FUNCTION_NAME(BBL_FUNCTION(target));
  LOG_MESSAGE(L_TARGETSELECTOR, ",%x(%x),%s", BBL_CADDRESS(target), BBL_OLD_ADDRESS(target), target_function_name.c_str());

  /* If our callee needs to fall through to our target bbl, ensure this is possible */
  if (need_to_fall_through) {
    t_cfg_edge* edge;
    bool ft_edges = false;

    /* TODO FIXME: this does not work: it crashes in layouting due to a NULL chain */
    BBL_FOREACH_PRED_EDGE(target, edge) {
      if (CFG_EDGE_CAT(edge) != ET_JUMP) {
        /* a very simple check, so we also have switch edges, interprocedurals, etc, to be on the safe side */
        ft_edges = true;
        break;
      }
    }

    /* Create a new bbl that jumps to our target */
    if (ft_edges) {
      VERBOSE(1, ("Required fallthrough, but one already existed: inserting additional jump-BBL for @eiB", target));
      LOG_MESSAGE(L_TARGETSELECTOR, ",jump");

      t_function* fun = BBL_FUNCTION(bbl);
      t_cfg* cfg      = FUNCTION_CFG(fun);
      t_bbl* jumper   = BblNew(cfg);
      DiabloBrokerCall("CopyToNewTargetBBL", bbl, jumper);

      BblInsertInFunction(jumper, fun);

      GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(jumper);
      t_cfg_edge *e = CfgEdgeCreate(cfg, jumper, target, ET_IPJUMP);
      EdgeMakeIntraProcedural(e);
      EdgeMakeInterprocedural(e);
      CfgEdgeMarkFake(e);

      target = jumper;
    }
    else {
      LOG_MESSAGE(L_TARGETSELECTOR, ",nojump");
    }
  }

  LOG_MESSAGE(L_TARGETSELECTOR, "\n");
  STOP_LOGGING_TRANSFORMATION(L_TARGETSELECTOR);

  ASSERT(BBL_CFG(bbl) == BBL_CFG(target), ("@eiB and @eiB not in same CFG", bbl, target));
  return target;
}

void BblShouldBePartOfTransformation(t_bbl *bbl, TransformationID tf_id) {
  if (tf_id >= 0
      && BblTransformationID(bbl) != tf_id) {
    CfgDrawFunctionGraphsWithHotness(BBL_CFG(bbl), "tfid");
    FATAL(("target @eiB not in transformation %d (0x%x)", bbl, tf_id, tf_id));
  }
}

void BblTransformationIDAfterSplit(t_bbl *first, t_bbl *second) {
  BblCopyAssociatedInfo(first, second);
  UpdateTransformationMetadataAfterBblSplit(first, second);
}

t_bbl *redirect_fake_edge(t_cfg_edge *edge, t_randomnumbergenerator *rng, TransformationID to_tf_id) {
  t_bbl* target = SelectTargetFor(CFG_EDGE_HEAD(edge), rng, false, to_tf_id, TargetFilter);
  if (!BblIsValidFakeTarget(target))
    CfgDrawFunctionGraphs(BBL_CFG(target), "invalid");
  ASSERT(BblIsValidFakeTarget(target), ("invalid fake target for @eiB: @eiB", CFG_EDGE_HEAD(edge), target));

  if (!BblAssociatedInfo(target)) {
    if (diablosupport_options.verbose >= 1) {
      if (intersect(BblAssociatedInfo(CFG_EDGE_HEAD(edge))->archives, BblAssociatedInfo(target)->archives).size() == 0)
        VERBOSE(1, ("interlib @B @B", CFG_EDGE_HEAD(edge), target));
      else
        VERBOSE(1, ("intralib @B @B", CFG_EDGE_HEAD(edge), target));

      VERBOSE(1, ("Moving edge from @B\n ==> to target @B", CFG_EDGE_HEAD(edge), target));
    }
  }

  /* create exit block if none exists already */
  if (!FunctionGetExitBlock(BBL_FUNCTION(target)))
    FunctionCreateExitBlock(BBL_FUNCTION(target));

  CfgEdgeChangeTail(edge, target);
  EdgeMakeIntraProcedural(edge);
  EdgeMakeInterprocedural(edge);

  return target;
}

static
TransformationID OneTransformationPerBbl(t_cfg *cfg) {
  TransformationID max_tf_id = INVALID_TRANSFORMATION_ID;

  /* Make sure that only one transformation is covered in each function.
   * We need this so that we can order the BBLs per transformation. */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    TransformationID bbl_tf_id = INVALID_TRANSFORMATION_ID;

    bool keep_going = true;
    while (keep_going) {
      keep_going = false;

      t_ins *ins;
      BBL_FOREACH_INS(bbl, ins) {
        TransformationID tf_id = GetTransformationNumberFromId(INS_TRANSFORMATION_ID(ins));
        if (tf_id < 0)
          continue;

        if ((bbl_tf_id == tf_id) || (bbl_tf_id == INVALID_TRANSFORMATION_ID)) {
          /* OK */
          bbl_tf_id = tf_id;
        }
        else {
          /* not OK, split */
          t_bbl *split_off = BblSplitBlock(bbl, ins, TRUE);
          DiabloBrokerCall("AFAfterSplit", bbl, split_off);
          BblCopyAssociatedInfo(bbl, split_off);
          keep_going = true;
          break;
        }
      }
    }

    if ((max_tf_id == INVALID_TRANSFORMATION_ID)
        || (bbl_tf_id > max_tf_id))
      max_tf_id = bbl_tf_id;
  }

  return max_tf_id;
}

static void GlobalTargetPostPass(t_cfg* cfg) {
  if (cfg_to_hell.find(cfg) == cfg_to_hell.end()) {
    VERBOSE(1, ("No HELL target for this cfg!"));
    return;
  }
  t_bbl* hell = cfg_to_hell[cfg];

  STATUS(START, ("Redirecting edges globally"));
  LOG_MESSAGE(L_TARGETSELECTOR, "# obfuscation split selector total %d blocks, left %d blocks\n", obfuscation_split_selector->total(), obfuscation_split_selector->available());

  BblInitAssociatedInfo(cfg);

  /* bookkeeping */
  DiabloBrokerCall("AFCheckIndexBlocks", cfg);

  RemoveGenericNewTargetHandler("newtargetselector:globalpostpass");
  diabloflowgraph_new_target_selector_options.new_target_all_global = FALSE;

  t_randomnumbergenerator* rng = RNGCreateChild(RNGGetRootGenerator(), "GlobalTargetPostPass");

  TransformationID max_tf_id = OneTransformationPerBbl(cfg);

  if (cfg == GetMainCfg())
    FakeEdgeCycles(cfg, hell, max_tf_id, rng);
  else
    VERBOSE(0, ("not creating fake edge cycles in sub CFG"));

  map<TransformationID, map<TransformationID, t_uint32>> fake_links;

  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  BBL_FOREACH_PRED_EDGE_SAFE(hell, edge, edge_s) {
    t_bbl *target = redirect_fake_edge(edge, rng);

    /* record this fake link */
    TransformationID from_tf_id = BblTransformationID(CFG_EDGE_HEAD(edge));
    TransformationID to_tf_id = BblTransformationID(target);
    if (fake_links.find(from_tf_id) == fake_links.end())
      fake_links[from_tf_id] = map<TransformationID, t_uint32>();
    fake_links[from_tf_id][to_tf_id]++;
  }

  vector<t_bbl*> possible_targets;
  t_bbl* bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (BBL_IS_HELL(bbl))
      continue;
    if (BBL_NINS(bbl) == 0)
      continue;
    possible_targets.push_back(bbl);
  }

  while (BBL_REFED_BY(hell)) {
    t_reloc *reloc = RELOC_REF_RELOC(BBL_REFED_BY(hell));

    for (t_uint32 i = 0; i < RELOC_N_TO_RELOCATABLES(reloc); i++) {
      if (RELOC_TO_RELOCATABLE(reloc)[i] == T_RELOCATABLE(hell)) {
        t_bbl *target = NULL;
        if (RELOC_SWITCH_EDGE(reloc))
          target = CFG_EDGE_TAIL(T_CFG_EDGE(RELOC_SWITCH_EDGE(reloc)));
        else
          target = possible_targets.at(RNGGenerateWithRange(rng, 0, possible_targets.size() - 1));

        VERBOSE(1, ("Moving reloc (to %d) @R ==> @B", i, reloc, target));

        RelocSetToRelocatable(reloc, i, T_RELOCATABLE(target));
      }
    }
  }

  while (BBL_REFERS_TO(hell))
    FATAL(("implement me"));

  LogFile *L_LOG = NULL;
  t_string output_basename = OutputFilename();
  INIT_LOGGING(L_LOG, (string(output_basename) + ".fake-cycles-all").c_str());
  Free(output_basename);

  for (auto p1 : fake_links) {
    auto from_tf_id = p1.first;
    LOG_MESSAGE(L_LOG, "%d", from_tf_id);

    for (auto p2 : p1.second) {
      auto to_tf_id = p2.first;
      auto counter = p2.second;

      LOG_MESSAGE(L_LOG, ",%d:%d", to_tf_id, counter);
    }

    LOG_MESSAGE(L_LOG, "\n");
  }

  FINI_LOGGING(L_LOG);

  if (BBL_PRED_FIRST(hell))
    FATAL(("Hell @eiB still has incoming edge! @E", hell, BBL_PRED_FIRST(hell)));

  t_function *to_kill = BBL_FUNCTION(hell);
  t_bbl *tmp;
  FUNCTION_FOREACH_BBL_SAFE(to_kill, bbl, tmp)
    BblKill(bbl);
  FunctionKill(to_kill);

  BblFiniAssociatedInfo(cfg);

  RNGDestroy(rng);

  STATUS(STOP, ("Redirecting edges globally"));
}

GlobalPostPassTargetSelector::GlobalPostPassTargetSelector() {
  RegisterTransformationType(this, _name);
}

bool GlobalPostPassTargetSelector::canTransform(const t_bbl* bbl) const {
  return diabloflowgraph_new_target_selector_options.new_target_global;
}

static
void RedirectGlobalTargetHellEdgeToCfg(t_cfg_edge *edge, t_bbl **result) {
  *result = NULL;

  /* check whether the edge goes to the global target hell of the CFG it points to */
  if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))
      && cfg_to_hell.find(BBL_CFG(CFG_EDGE_TAIL(edge))) != cfg_to_hell.end()
      && CFG_EDGE_TAIL(edge) == cfg_to_hell[BBL_CFG(CFG_EDGE_TAIL(edge))]) {
    /* going to a global target hell block in the original CFG */

    /* create a new block for the origin CFG */
    *result = global_target_selector->doTransform(BBL_CFG(CFG_EDGE_HEAD(edge)), NULL);
  }
}

t_bbl *GlobalPostPassTargetSelector::Common(t_cfg *cfg) {
  if (!cfg_to_postpass_callback_init) {
    DiabloBrokerCallInstall("BeforeDeflowgraph", "t_cfg *", (void*)GlobalTargetPostPass, FALSE);
    DiabloBrokerCallInstall("RedirectGlobalTargetHellEdgeToCfg", "t_cfg_edge *, t_bbl **", (void*)RedirectGlobalTargetHellEdgeToCfg, FALSE);
    cfg_to_postpass_callback_init = true;
  }

  if (cfg_to_hell.find(cfg) == cfg_to_hell.end()) {
    t_bbl* hell = BblNew(cfg);
    BBL_SET_IS_HELL(hell, TRUE);
    t_function *fun = FunctionMake(hell, "--GLOBALTARGETHELL--", FT_HELL);

    t_function *it = CFG_HELL_FUNCTIONS(cfg);
    while (FUNCTION_NEXT_HELL(it) != NULL)
      it = FUNCTION_NEXT_HELL(it);
    FUNCTION_SET_NEXT_HELL(it, fun);
    FUNCTION_SET_NEXT_HELL(fun, NULL);

    cfg_to_hell[cfg] = hell;

    return hell;
  }

  return cfg_to_hell[cfg];
}

t_bbl* GlobalPostPassTargetSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  return Common(BBL_CFG(bbl));
}

t_bbl* GlobalPostPassTargetSelector::doTransform(t_cfg *cfg, t_randomnumbergenerator * rng) {
  return Common(cfg);
}
