#include "ARM_meta_api.h"

using namespace std;

#define AOP_VERBOSITY 0
#define AOP_VERBOSITY_INC 1

#define DEBUG_COUNTER diablosupport_options.debugcounter

#define DEBUG_ARG_INVALID_SETTER 0xff
#define DEBUG_ARG_INVALID_CHECK 0x01

#define GENERATE_DOTS 0
// #define FORCE_DOTS 1

t_bbl *measure_loc1_bbl = NULL;
t_bbl *measure_loc2_bbl = NULL;
t_bbl *measure_loc3_bbl = NULL;

bool meta_api_testing = false;

enum StatusCode {
  STATUS_NO_BBL,
  STATUS_NOT_BBL_CAN_TRANSFORM,
  STATUS_NO_SET_POINTS,
  STATUS_OK,
  STATUS_NOT_IN_FUNCTION,
  STATUS_GLOBAL_CONSTRUCTOR,
  STATUS_DIABLO_FUNCTION,
  STATUS_CALL_WEAK_FN,
  STATUS_META_FUNCTION,
  STATUS_SEQUENCE,
  STATUS_UNTOUCHABLE,
  STATUS_HELL_FUNCTION,
  STATUS_STATUS_FLAGS,
  STATUS_NO_SUCC,
  STATUS_DATA,
  STATUS_SUCC_DATA,
  STATUS_SWITCH_RELATED,
  STATUS_SET_POINT_OVERHEAD
};
static int status_code;

struct Statistics {
  int bbl_not_in_function;
  int bbl_in_implementation;
  int bbl_hell;
  int bbl_no_dead_flags;
  int bbl_no_successor;
  int bbl_data;
  int bbl_data_successor;
  int total_tests;
  int before_sequence;
  int switch_related;
  int not_all_paths;
};

static Statistics global_stats;
static bool is_initialised = false;

static int nr_transformed = 0;
static t_uint32 transformation_uid = 0;
static t_uint32 setter_uid = 0;
static t_uint32 getter_uid = 0;

int ARMMetaApiTransformation::statusCode() {
  return status_code;
}

string ARMMetaApiTransformation::statusToString(int status_code) {
  string str = "";

  switch (status_code) {
  case STATUS_NO_BBL:
    str = "NO_BBL"; break;
  case STATUS_NOT_BBL_CAN_TRANSFORM:
    str = "NOT_BBL_CAN_TRANSFORM"; break;
  case STATUS_NO_SET_POINTS:
    str = "NO_SET_POINTS"; break;
  case STATUS_OK:
    str = "OK"; break;
  case STATUS_NOT_IN_FUNCTION:
    str = "NOT_IN_FUNCTION"; break;
  case STATUS_GLOBAL_CONSTRUCTOR:
    str = "GLOBAL_CONSTRUCTOR"; break;
  case STATUS_DIABLO_FUNCTION:
    str = "DIABLO_FUNCTION"; break;
  case STATUS_CALL_WEAK_FN:
    str = "CALL_WEAK_FN"; break;
  case STATUS_META_FUNCTION:
    str = "META_FUNCTION"; break;
  case STATUS_SEQUENCE:
    str = "SEQUENCE"; break;
  case STATUS_UNTOUCHABLE:
    str = "UNTOUCHABLE"; break;
  case STATUS_HELL_FUNCTION:
    str = "HELL_FUNCTION"; break;
  case STATUS_STATUS_FLAGS:
    str = "STATUS_FLAGS"; break;
  case STATUS_NO_SUCC:
    str = "NO_SUCC"; break;
  case STATUS_DATA:
    str = "DATA"; break;
  case STATUS_SUCC_DATA:
    str = "SUCC_DATA"; break;
  case STATUS_SWITCH_RELATED:
    str = "SWITCH_RELATED"; break;
  case STATUS_SET_POINT_OVERHEAD:
    str = "SET_POINT_OVERHEAD"; break;

  default:
    FATAL(("unsupported status code"));
  }

  return str;
}

ARMMetaApiTransformation::ARMMetaApiTransformation() {
  AddOptionsListInitializer(obfuscation_opaque_predicate_option_list);
  OpaquePredicateOptInit();

  RegisterTransformationType(this, _name);

  testing = false;
}

ARMMetaApiTransformation::~ARMMetaApiTransformation() {
}

/* Initialise the defined datastructures.
 * Parameters:
 *  bbl   The BBL in which calls to the initialisation functions should be inserted. */
static
BblVector createInstances(t_cfg *cfg, vector<t_function *>& functions_to_finalize, t_function *target_function, bool testing) {
  BblVector result;

  auto possible = DatatypesWithPredicates();
  t_uint32 uid = 0;
  for (auto datatype : possible) {
    if (!testing && !datatype->enabled)
      continue;

    for (int i = 0; i < meta_api_instance_count; i++) {
      VERBOSE(AOP_VERBOSITY, ("instantiating(%d) '%s'", uid, datatype->Print().c_str()));

      string fn_name = "Meta_Instance" + to_string(uid);

      bool create_in_init_function = false;
      if (meta_api_init_function)
        create_in_init_function = true;

      t_bbl *return_site;
      t_function *fun = NULL;
      PreparedCallSite call_site;
      MetaAPI_Instance *instance = MetaAPI_InstantiateDatastructure(cfg, datatype, NULL, target_function ? FUNCTION_BBL_FIRST(target_function) : NULL, fn_name, return_site, fun, false, nullptr, call_site, create_in_init_function, meta_api_impl_inst);
      functions_to_finalize.push_back(fun);
      datatype->instances.push_back(instance);
      result.push_back(return_site);

      /* activate the predicates associated with this instance */
      for (auto predicate : datatype->predicates) {
        /* skip disabled predicates */
        if (! predicate->enabled)
          continue;

        MetaAPI_ActivePredicate *x = new MetaAPI_ActivePredicate();
        x->instance = instance;
        x->predicate = predicate;
        MetaAPI_RegisterPredicate(x);

        VERBOSE(AOP_VERBOSITY, ("  added predicate '%s'", x->Print().c_str()));
      }

      uid++;
    }
  }

  /* by default, the value of each predicate is set to 'unknown' */
  return result;
}

/* 'canTransform' is _always_ called before 'doTransform' */
bool ARMMetaApiTransformation::canTransform(const t_bbl* bbl_) const {
  /* this is a bit of a dirty hack:
   * the 'canTransform' function is declared 'const', so semantically it can't modify its members.
   * However, it is only now that we can do the initialisation stuff... */
  if (!is_initialised) {
    ARMMetaApiTransformation* thisObject = const_cast<ARMMetaApiTransformation*>(this);
    thisObject->initialise(bbl_ ? BBL_CFG(bbl_) : test_cfg);
  }

  if (!bbl_) {
    status_code = STATUS_NO_BBL;
    return false;
  }

  t_bbl *bbl = const_cast<t_bbl*>(bbl_);
  t_cfg *cfg = BBL_CFG(bbl);

  if (!initialisation_phase) {
    if (!BBL_CAN_TRANSFORM(bbl)) {
      status_code = STATUS_NOT_BBL_CAN_TRANSFORM;
      return false;
    }
  }

  /* keep track of the number of blocks we tested */
  global_stats.total_tests++;

  /* basic checks, sets status_code */
  if (!canTransformBbl(bbl))
    return false;

  /* extended check */
  bool _temp1;
  vector<BblLocation> setter_locations;
  if (!FindSetterLocations(bbl, NULL, _temp1, setter_locations)) {
    /* not (enough) setter locations found */
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: not all incoming paths have a location where flags are dead", bbl));
    global_stats.not_all_paths++;
    status_code = STATUS_NO_SET_POINTS;
    return false;
  }

  /* setter locations found, but need to consider introduced overhead */
  t_uint64 getter_exec_count = BBL_EXEC_COUNT(bbl);
  t_uint64 setter_exec_count = 0;
  for (BblLocation loc : setter_locations)
    setter_exec_count += BBL_EXEC_COUNT(loc.bbl);

  if (getter_exec_count > 0) {
    t_uint64 value = setter_exec_count/getter_exec_count;
    if (value > (t_uint64)meta_api_setters_per_getter) {
      VERBOSE(/*AOP_VERBOSITY+AOP_VERBOSITY_INC*/0, ("can't transform @B: too much overhead, getter %" PRIu64 ", needs %u setters executed %" PRIu64 " times: %" PRIu64, bbl, getter_exec_count, setter_locations.size(), setter_exec_count, value));
      status_code = STATUS_SET_POINT_OVERHEAD;
      return false;
    }
  }

  status_code = STATUS_OK;
  return true;
}

static
t_bbl *CreateCrashingBlock(t_function *fun, t_uint32 value) {
  t_bbl *block = BblNew(FUNCTION_CFG(fun));
  BblInsertInFunction(block, fun);

  t_arm_ins *ins;
  ArmMakeInsForBbl(ConstantProducer, Append, ins, block, false, ARM_REG_R0, 0);
  ArmMakeInsForBbl(Ldr, Append, ins, block, false, ARM_REG_R0, ARM_REG_R0, ARM_REG_NONE, value, ARM_CONDITION_AL, true, true, false);
  ArmMakeInsForBbl(UncondBranch, Append, ins, block, false);
  CfgEdgeCreate(FUNCTION_CFG(fun), block, block, ET_JUMP);

  return block;
}

static
t_bbl *CreateEmptyBblOnEdge(t_cfg_edge *edge, bool head_function, bool jump) {
  ASSERT(CFG_EDGE_CAT(edge) != ET_CALL
         && CFG_EDGE_CAT(edge) != ET_RETURN, ("oops @E", edge));

  t_cfg *cfg = CFG_EDGE_CFG(edge);
  t_bbl *old_tail = CFG_EDGE_TAIL(edge);

  t_bbl *new_bbl = BblNew(cfg);
  BblInsertInFunction(new_bbl, BBL_FUNCTION(head_function ? CFG_EDGE_HEAD(edge) : CFG_EDGE_TAIL(edge)));
  CfgEdgeChangeTail(edge, new_bbl);

  if (head_function)
    EdgeMakeIntraProcedural(edge);

  if (jump) {
    t_arm_ins *ins;
    ArmMakeInsForBbl(UncondBranch, Append, ins, new_bbl, false);
  }

  int cat = jump ? ET_JUMP : ET_FALLTHROUGH;
  t_cfg_edge *new_edge = CfgEdgeCreate(cfg, new_bbl, old_tail, cat);
  if (BBL_FUNCTION(new_bbl) != BBL_FUNCTION(old_tail))
    EdgeMakeInterprocedural(new_edge);
  
  BBL_SET_REGS_LIVE_OUT(new_bbl, BblRegsLiveBefore(old_tail));

  return new_bbl;
}

t_cfg_edge *decide_control_flow(MetaAPI_QueryResult query_result, bool count_max_setters, MetaAPI_Effect::Effect new_value, t_uint32 val) {
  /* create a bogus block
  * TODO: for ease of debugging, do a load of address '0', this should ideally be replaced by either:
  *  - a fake edge, to be redirected by the fake edge specialised code;
  *  - another valid destination, in case two-way predicates are used. */
  t_bbl *bogus_block = count_max_setters ? query_result.next_bbl : CreateCrashingBlock(BBL_FUNCTION(query_result.decision_bbl), val);
  t_cfg *cfg = BBL_CFG(bogus_block);

  /* for now, assume that the branch is TAKEN when the result is FALSE */
  t_bbl *false_block;
  t_bbl *true_block;
  if (new_value == MetaAPI_Effect::Effect::True) {
    /* predicate value is TRUE */
    true_block = query_result.next_bbl;
    false_block = bogus_block;
  }
  else {
    /* predicate value is FALSE */
    true_block = bogus_block;
    false_block = query_result.next_bbl;
  }

  t_cfg_edge *false_edge = CfgEdgeCreate(cfg, query_result.decision_bbl, false_block, ET_JUMP);
  EdgeMakeInterprocedural(false_edge);

  t_cfg_edge *true_edge = CfgEdgeCreate(cfg, query_result.decision_bbl, true_block, ET_FALLTHROUGH);
  EdgeMakeInterprocedural(true_edge);

  if (false_block == bogus_block) {
    CfgEdgeMarkFake(false_edge);
    ASSERT_WITH_DOTS((BBL_CFG(bogus_block), "interproc"), CfgEdgeIsInterproc(true_edge), ("expected interprocedural true edge, got @E", true_edge));
    MetaAPI_LinkEdges(query_result.first_edge, true_edge);
  }
  else {
    CfgEdgeMarkFake(true_edge);
    ASSERT_WITH_DOTS((BBL_CFG(bogus_block), "interproc"), CfgEdgeIsInterproc(false_edge), ("expected interprocedural false edge, got @E", false_edge));
    MetaAPI_LinkEdges(query_result.first_edge, false_edge);
  }

  return (false_block == bogus_block) ? true_edge : false_edge;
}

static
void break_fallthrough_chains(t_bbl *start) {
  t_bbl *current = start;

  t_cfg_edge *last_notip_ft_edge = NULL;
  t_cfg_edge *ft_edge = NULL;
  do {
    t_bbl *new_current = NULL;

    /* look up fallthrough edge of this block */
    ft_edge = NULL;

    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(current, e) {
      if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
          || CFG_EDGE_CAT(e) == ET_IPFALLTHRU) {
        ft_edge = e;
        new_current = CFG_EDGE_TAIL(e);
        break;
      }
      else if (CFG_EDGE_CAT(e) == ET_CALL
          && CFG_EDGE_CORR(e)) {
        new_current = CFG_EDGE_TAIL(CFG_EDGE_CORR(e));
        break;
      }
    }

    /* early exit if no fallthrough edge */
    if (!new_current)
      return;
    
    if (ft_edge
        && !CfgEdgeIsInterproc(ft_edge))
      last_notip_ft_edge = ft_edge;
    
    current = new_current;
  } while (current != start);

  /* we only get here when 'current' == 'start',
   * in other words, when a fallthrough chain is found */
  ASSERT(last_notip_ft_edge, ("expected non-IP fallthrough edge, but found none @eiB", start));

  t_bbl *tail = CFG_EDGE_TAIL(last_notip_ft_edge);
  t_bbl *jump_bbl = CreateEmptyBblOnEdge(last_notip_ft_edge, false, true);
  MetaAPI_AfterBblSplit(tail, jump_bbl);
}

void ARMMetaApiTransformation::finalizeAll() const {
  for (t_function *fun : functions_to_finalize) {
    FunctionMetaApiData *d = FunctionGetMetaApiData(fun);
    if (! d->is_finalized) {
      MetaAPI_ActuallySaveRestoreRegisters(fun);
      d->is_finalized = true;
    }
  }
}

bool ARMMetaApiTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator *rng) {
  static bool first_time = true;

  t_cfg *cfg = testing ? test_cfg : BBL_CFG(bbl);
  bool success = false;

  transformation_uid = nr_transformed;

  t_uint64 getter_exec_count = 0;
  t_uint64 setter_exec_count = 0;

  if (meta_api_measuring)
    ASSERT(testing, ("can only measure in testing mode"));

  if (testing) {
    VERBOSE(0, ("TRANSFORM_%d meta_api on (to be determined)", transformation_uid));
    getter_exec_count = 1;

    meta_api_testing = true;
  }
  else {
    VERBOSE(0, ("TRANSFORM_%d meta_api on @iB", transformation_uid, bbl));
    getter_exec_count = BBL_EXEC_COUNT(bbl);

    meta_api_testing = false;
  }

  ASSERT(canTransform(bbl) || testing, ("should be able to transform @eiB", bbl));

  t_function *target_function = GetFunctionByName(cfg, "main");
  if (testing) {
    test_configuration = MetaAPI_GetTestByName(test_name);

    VERBOSE(0, ("insert test in function '%s'", test_configuration.function_name.c_str()));

    target_function = GetFunctionByName(cfg, test_configuration.function_name.c_str());
    ASSERT(target_function, ("can't find target function '%s'", test_configuration.function_name.c_str()));

    bbl = FUNCTION_BBL_FIRST(target_function);

    if (meta_api_measuring) {
      // find and split off BBLs in which to inject our code
      t_arm_ins *measure_loc1_ins = NULL;
      t_arm_ins *measure_loc2_ins = NULL;
      t_arm_ins *measure_loc3_ins = NULL;

      {

        auto find_and_split_instruction = [] (t_function *fun, t_uint32 address) {
          t_arm_ins *result = NULL;

          t_bbl *bbl;
          FUNCTION_FOREACH_BBL(fun, bbl) {
            t_arm_ins *ins;
            BBL_FOREACH_ARM_INS(bbl, ins) {
              if (ARM_INS_OLD_ADDRESS(ins) == address) {
                result = ins;

                // split before
                if (ins != T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(ins))))
                  BblSplitBlock(ARM_INS_BBL(ins), T_INS(ins), true);

                // split after
                if (ins != T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(ins))))
                  BblSplitBlock(ARM_INS_BBL(ins), T_INS(ins), false);

                break;
              }
            }
          }

          return result;
        };

        measure_loc1_ins = find_and_split_instruction(target_function, std::stoi(meta_api_measure_loc1_address, nullptr, 0));
        measure_loc2_ins = find_and_split_instruction(target_function, std::stoi(meta_api_measure_loc2_address, nullptr, 0));
        measure_loc3_ins = find_and_split_instruction(target_function, std::stoi(meta_api_measure_loc3_address, nullptr, 0));
      }

      measure_loc1_bbl = ARM_INS_BBL(measure_loc1_ins);
      measure_loc2_bbl = ARM_INS_BBL(measure_loc2_ins);
      measure_loc3_bbl = ARM_INS_BBL(measure_loc3_ins);

      ASSERT(measure_loc1_bbl != NULL, ("loc1 address '%s' not found", meta_api_measure_loc1_address.c_str()));
      DEBUG(("loc1 in @eiB", measure_loc1_bbl));
      ASSERT(measure_loc2_bbl != NULL, ("loc2 address '%s' not found", meta_api_measure_loc2_address.c_str()));
      DEBUG(("loc2 in @eiB", measure_loc2_bbl));
      ASSERT(measure_loc3_bbl != NULL, ("loc3 address '%s' not found", meta_api_measure_loc3_address.c_str()));
      DEBUG(("loc3 in @eiB", measure_loc3_bbl));
    }
  }

  if (first_time) {
    /* initialise datastructures */
    VERBOSE(meta_api_verbosity, ("instantiating data structures"));
    BblVector return_sites = createInstances(BBL_CFG(bbl), functions_to_finalize, target_function, testing);

    if (testing) {
      if (meta_api_measuring) {
        bbl = measure_loc2_bbl;
      } else {
        if (!meta_api_init_function) {
          t_bbl *next = return_sites.front();
          bbl = next ? next : bbl;

        }
        else if(meta_api_project_name == "radare2") {
          bbl = FUNCTION_BBL_LAST(target_function);

          t_cfg_edge *e;
          BBL_FOREACH_PRED_EDGE(bbl, e)
            bbl = CFG_EDGE_HEAD(e);
        }
      }

      DEBUG(("TEST MODE -- overriding bbl with @eiB", bbl));
    }

    first_time = false;
  }

#if GENERATE_DOTS
  vector<t_function *> functions_to_plot;
  functions_to_plot.push_back(BBL_FUNCTION(bbl));
#endif

  /* debugging */
  force_dots = false;
  t_int32 dots_uid = -1;

  bool count_max_setters = false;

#ifdef DEBUG_COUNTER
  if (nr_transformed == (DEBUG_COUNTER - 1)) {
    VERBOSE(meta_api_verbosity, ("DBG forcing dot file generation for %d", nr_transformed));

#ifdef FORCE_DOTS
    force_dots = true;
    dots_uid = transformation_uid;
#endif

    count_max_setters = meta_api_max_setter_set;
    if (count_max_setters)
      VERBOSE(meta_api_verbosity, (META_API_PREFIX "max number of setters %d", meta_api_max_setter_count));
  }
#endif

  /* dump state BEFORE transforming */
  if (dots_uid >= 0) {
    VERBOSE(meta_api_verbosity, ("DBG dumping beforetf%d", dots_uid));
    DumpDots(cfg, "beforetf", dots_uid);
  }

  /* collect dead predicates (= usable) */
  auto dead_predicates_before = BblDeadPredicatesBefore(bbl);
  auto dead_predicates_after = BblDeadPredicatesAfter(bbl);

  MetaAPI_ActivePredicate *selected_predicate = NULL;

  if (dead_predicates_before.none()
      && dead_predicates_after.none()) {
    /* no dead predicates found */
  }
  else {
    /* dead predicate before or after the block */
    auto dead_predicates = dead_predicates_before.none() ? &dead_predicates_after : &dead_predicates_before;

    /* TODO: comment me */
    vector<BblLocation> setter_locations;
    success = true;
    bool loop = false;

    if (testing) {
      t_arm_ins *ins;

      /* look for the requested predicate */
      ASSERT(test_configuration.datatype->instances.size() > 0, ("no instances for datatype '%s'", test_configuration.datatype->name.c_str()));
      selected_predicate = MetaAPI_FirstActivePredicate(test_configuration.datatype->instances[0], test_configuration.predicate);

      /* create blocks and fill them with a single NOP instruction.
         This prevents the meta-API code from erroneously moving one block to another function */
      ASSERT(!BblReferedToByExtab(bbl), ("from extab! @eiB", bbl));
      t_bbl *tmp = BblSplitBlock(bbl, BBL_INS_FIRST(bbl), true);
      ArmMakeInsForBbl(Noop, Prepend, ins, bbl, ArmBblIsThumb(bbl));

      if (selected_predicate->predicate->type != MetaAPI_Predicate::Type::Invariant)
        setter_locations.push_back(BblLocation{.bbl = bbl});

      bbl = tmp;
      ArmMakeInsForBbl(Noop, Prepend, ins, bbl, ArmBblIsThumb(bbl));
    }
    else {
      do {
        /* no predicates left? */
        if (dead_predicates->none()) {
          VERBOSE(meta_api_verbosity, ("no dead predicate found"));
          success = false;
          break;
        }

        /* one or more predicates available,
        * select random predicate */
        auto random_uid = SelectRandomPredicateIndex(*dead_predicates);
        VERBOSE(meta_api_verbosity, ("chose random uid %d", random_uid));
        selected_predicate = MetaAPI_GetPredicateByUID(random_uid);
        VERBOSE(meta_api_verbosity, ("chose predicate %s", selected_predicate->Print().c_str()));

        /* will be set to 'TRUE' when another predicate should be tried */
        loop = false;

        /* try to find suitable locations to insert setters */
        bool because_of_predicate = false;
        setter_locations.clear();
        success = FindSetterLocations(bbl, selected_predicate, because_of_predicate, setter_locations);

        /* can't do */
        if (!success) {
          if (because_of_predicate) {
            VERBOSE(meta_api_verbosity, ("  ERROR not dead, choosing another one..."));

            dead_predicates->reset(random_uid);

            loop = true;
          }
          else {
            /* If we come here, then no suitable setter locations can be found for this predicate.
            * This will also be the case for every other predicate, so don't even try to find another one.
            * This block simply can't be transformed. */
            break;
          }
        }
      } while (loop);
    }

    if (success) {
      bool pred_is_invariant = selected_predicate->predicate->type == MetaAPI_Predicate::Type::Invariant;

      /* select the value to set the predicate to */
      MetaAPI_Effect::Effect new_value = MetaAPI_Effect::Effect::Unknown;

      if (pred_is_invariant) {
        VERBOSE(meta_api_verbosity, ("don't try to change value of '%s' because it is invariant", selected_predicate->Print().c_str()));
      }
      else {
        VERBOSE(meta_api_verbosity, ("try to change value of '%s'", selected_predicate->Print().c_str()));

        if (testing) {
          /* fixed value from the test configuration */
          new_value = test_configuration.value;
        }
        else if (meta_api_enable_two_way) {
          /* prefer to reuse an existing one-way getter
          * This function call may return Effect::Unknown when no getter can be reused.
          * In that case, we fall back to the generation of a pseudo-random value below. */
          new_value = MetaAPI_PreferToReuseGetter(selected_predicate, meta_api_effect_rng);
        }

        if (new_value == MetaAPI_Effect::Effect::Unknown)
          new_value = MetaAPI_Effect::RandomTrueFalse();

        VERBOSE(meta_api_verbosity, ("  --> %s", MetaAPI_Effect::EffectToString(new_value).c_str()));
      }

      MetaAPI_Function *getter_func = testing ? MetaAPI_GetFunctionByName(test_configuration.getter_identifier) : PickRandomElement(selected_predicate->predicate->getters, meta_api_getter_rng);
      string getter_func_name = "MetaTF" + to_string(transformation_uid) + "_" + selected_predicate->predicate->Print() + "_UID" + to_string(selected_predicate->uid) + "_Getter_" + MetaAPI_Effect::EffectToString(new_value);

      /* */
      set<MetaAPI_ActivePredicate *> dont_change_preds = LivePredicatesInCloud();
      for (auto setter_location : setter_locations) {
        setter_exec_count += BBL_EXEC_COUNT(setter_location.bbl);
        auto live_in = BblLivePredicatesBefore(setter_location.bbl);
        MetaAPI_ForEachPredicateIn(&live_in, [&dont_change_preds] (MetaAPI_ActivePredicate *p) {
          dont_change_preds.insert(p);
        });
      }

      for (auto i : dont_change_preds)
        VERBOSE(meta_api_verbosity, ("dont change %s", i->Print().c_str()));
      map<MetaAPI_Predicate *, MetaAPI_Effect::Effect> dont_change;
      for (auto x : dont_change_preds)
        dont_change[x->predicate] = MetaAPI_Effect::Effect::Unchanged;

      map<t_bbl *, MetaAPI_Setter, bblcmp> setters_per_block;

      vector<t_bbl *> possible_multiple_incoming_fallthrough;

      auto nr_incoming_fallthrough = [] (t_bbl *bbl) {
        int nr_fallthrough = 0;

        t_cfg_edge *e;
        BBL_FOREACH_PRED_EDGE(bbl, e) {
          if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
              || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
            nr_fallthrough++;
        }

        return nr_fallthrough;
      };

      for (auto setter_location : setter_locations) {
        vector<t_bbl *> targets;

        if (meta_api_measuring) {
          targets.push_back(measure_loc1_bbl);
        } else {
          if (setter_location.split_after) {
            t_cfg_edge *e;
            BBL_FOREACH_SUCC_EDGE(setter_location.bbl, e) {
              switch (CFG_EDGE_CAT(e)) {
              case ET_FALLTHROUGH:
              case ET_JUMP: {
                if (!BblIsInCloud(CFG_EDGE_TAIL(e)))
                  continue;

                /* this block is in the cloud and should get a setter */
                t_bbl *bbl = CreateEmptyBblOnEdge(e, true, false);
                MetaAPI_AfterBblSplit(setter_location.bbl, bbl);
                targets.push_back(bbl);

                if (nr_incoming_fallthrough(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl))) > 1) {
                  t_bbl *new_bbl = CreateEmptyBblOnEdge(BBL_SUCC_FIRST(bbl), true, true);
                  MetaAPI_AfterBblSplit(bbl, new_bbl);
                }
              } break;

              case ET_IPFALLTHRU:
              case ET_IPJUMP: {
                t_bbl *tail = CFG_EDGE_TAIL(e);
                t_function *fun = BBL_FUNCTION(tail);
                ASSERT(FunctionIsMeta(fun), ("expected meta-api function @eiB @E @F", tail, e, fun));

                t_bbl *next = CFG_EDGE_TAIL(LookThroughMetaFunction(e, true, [] (t_function *) {return false;}));

                if (!BblIsInCloud(next))
                  continue;

                t_bbl *bbl = CreateEmptyBblOnEdge(e, true, false);
                MetaAPI_AfterBblSplit(setter_location.bbl, bbl);
                ASSERT(CFG_EDGE_METAAPI_CORR_FOREWARD(e), ("expected forward meta api edge @E", e));
                ASSERT(!CFG_EDGE_METAAPI_CORR_BACKWARD(e), ("unexpected backward meta api edge @E", e));

                t_cfg_edge *e_far = CFG_EDGE_METAAPI_CORR_FOREWARD(e);

                CFG_EDGE_SET_METAAPI_CORR_FOREWARD(e, NULL);
                CFG_EDGE_SET_METAAPI_CORR_BACKWARD(e_far, NULL);
                MetaAPI_LinkEdges(BBL_SUCC_FIRST(bbl), e_far);

                if (nr_incoming_fallthrough(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl))) > 1) {
                  t_bbl *new_bbl = CreateEmptyBblOnEdge(BBL_SUCC_FIRST(bbl), true, true);
                  MetaAPI_AfterBblSplit(bbl, new_bbl);
                }
              } break;

              default:
                CfgDrawFunctionGraphs(cfg, "unhandled");
                FATAL(("unhandled @E", e));
              }
            }
          }
          else {
            targets.push_back(setter_location.bbl);
          }
        }

        for (t_bbl *target : targets) {
          ASSERT(setters_per_block.find(target) == setters_per_block.end(), ("unexpected @eiB", target));

          BblRemoveFromCloud(target);

          vector<MetaAPI_Setter> setters;
          if (testing) {
            MetaAPI_Setter setter;
            setter.function = MetaAPI_GetFunctionByName(test_configuration.setter_identifier);
            setter.configuration = test_configuration.setter_configuration;
            setters.push_back(setter);
          }
          else {
            /* find a setter function */
            setters = MetaAPI_FindTransformers(selected_predicate->instance->datatype, selected_predicate->predicate, new_value, dont_change);
            if (setters.size() == 0) {
              success = false;
              break;
            }
          }

          setters_per_block[target] = PickRandomElement(setters, meta_api_setter_rng);
        }
        if (!success)
          break;
      }

      if (success) {
        if (pred_is_invariant) {
          VERBOSE(meta_api_verbosity, ("not changing value of '%s' because it is invariant", selected_predicate->Print().c_str()));
        } else {
          VERBOSE(meta_api_verbosity, ("changing value of '%s'", selected_predicate->Print().c_str()));
          int setter_uid = 0;

          if (dots_uid >= 0) {
            VERBOSE(meta_api_verbosity, ("DBG dumping before-setters%d", dots_uid));
            DumpDots(cfg, "before-setters", dots_uid);
          }

          for (auto p : setters_per_block) {
            t_bbl *target = p.first;
            MetaAPI_Setter setter = p.second;

            /* call the setter function */
            string setter_func_name = "MetaTF" + to_string(transformation_uid) + "_" + selected_predicate->predicate->Print() + "_UID" + to_string(selected_predicate->uid) + "_Setter" + to_string(setter_uid) + "_" + MetaAPI_Effect::EffectToString(new_value);

            /* debug functionality */
            if (count_max_setters
                && (setter_uid >= meta_api_max_setter_count)) {
              VERBOSE(meta_api_verbosity, ("SKIP %s", setter_func_name.c_str()));
              setter_uid++;
              continue;
            }

            SetterVerificationInfo setter_verify;
            setter_verify.expected = new_value;
            setter_verify.predicate = selected_predicate;

            if (!(testing && (meta_api_project_name == "radare2")))
              ASSERT_WITH_DOTS((BBL_CFG(target), "meta"), !FunctionIsMeta(BBL_FUNCTION(target)), ("can't transform in meta function! @eiB", target));
            MetaAPI_TransformResult transform_result = MetaAPI_TransformDatastructure(selected_predicate->instance, setter.function, target, setter.configuration, setter_func_name, setter_verify, meta_api_impl_set);
            functions_to_finalize.push_back(transform_result.function);
            FUNCTION_METAAPI_DATA(transform_result.function)->predicate = selected_predicate;
#if GENERATE_DOTS
            functions_to_plot.push_back(BBL_FUNCTION(target));
            functions_to_plot.push_back(transform_result.function);
#endif

            t_bbl *cont_point = CFG_EDGE_TAIL(BBL_SUCC_FIRST(transform_result.last_block));
            BblInsertInCloud(cont_point);

            /* verify the set value */
            if (meta_api_verify_setters) {
              string setter_verify_name = setter_func_name + "_verify";

              t_bbl *getter_target = CreateEmptyBblOnEdge(BBL_SUCC_FIRST(transform_result.last_block), false, false);
              MetaAPI_AfterBblSplit(CFG_EDGE_TAIL(BBL_SUCC_FIRST(transform_result.last_block)), getter_target);
              MetaAPI_QueryResult query_result = MetaAPI_QueryDatastructure(selected_predicate, getter_func, getter_target, setter_verify_name, new_value, false /* this should _always_ be 'false' for verifiers! */, meta_api_impl_get);
              functions_to_finalize.push_back(BBL_FUNCTION(query_result.decision_bbl));
#if GENERATE_DOTS
              functions_to_plot.push_back(BBL_FUNCTION(query_result.decision_bbl));
#endif

              decide_control_flow(query_result, false, new_value, DEBUG_ARG_INVALID_SETTER);

              /* dummy information */
              FUNCTION_METAAPI_DATA(BBL_FUNCTION(query_result.decision_bbl))->is_verifier = true;
              FUNCTION_METAAPI_DATA(BBL_FUNCTION(query_result.decision_bbl))->verified_setter_function = transform_result.function;

              ASSERT(BBL_NINS(CFG_EDGE_HEAD(query_result.first_edge)) == 0, ("unexpected @eiB", CFG_EDGE_HEAD(query_result.first_edge)));
            }

            /* create branches when needed (one BBL may not have multiple incoming FT edges) */
            vector<t_cfg_edge *> ft_edges;

            /* collect fallthrough edges */
            t_cfg_edge *e;
            BBL_FOREACH_PRED_EDGE(cont_point, e) {
              if (CFG_EDGE_CAT(e) == ET_FALLTHROUGH
                  || CFG_EDGE_CAT(e) == ET_IPFALLTHRU)
                ft_edges.push_back(e);
            }

            /* apply fix when multiple incoming fallthrough */
            if (ft_edges.size() > 1) {
              for (size_t i = 1; i < ft_edges.size(); i++) {
                t_cfg_edge *e = ft_edges[i];
                t_bbl *jump_bbl = CreateEmptyBblOnEdge(e, false, true);
                MetaAPI_AfterBblSplit(cont_point, jump_bbl);

                if (e == BBL_SUCC_FIRST(transform_result.last_block))
                  BblInsertInCloud(jump_bbl);
              }
            }

            setter_uid++;
          }
        }

        if (dots_uid >= 0) {
          VERBOSE(meta_api_verbosity, ("DBG dumping before-getter%d", dots_uid));
          DumpDots(cfg, "before-getter", dots_uid);
        }

        /* call the getter */
        if (testing && (meta_api_project_name == "radare2")) {
          BblSplitBlock(bbl, BBL_INS_FIRST(bbl), TRUE);
        }

        VERBOSE(meta_api_verbosity, ("insert getter in @B weight %" PRIu64, bbl, setter_exec_count/getter_exec_count));
        BblRemoveFromCloud(bbl);

        if (!(testing && (meta_api_project_name == "radare2")))
          ASSERT_WITH_DOTS((BBL_CFG(bbl), "query"), !FunctionIsMeta(BBL_FUNCTION(bbl)), ("can't query in meta function! @eiB", bbl));
        MetaAPI_QueryResult query_result = MetaAPI_QueryDatastructure(selected_predicate, getter_func, bbl, getter_func_name, new_value, meta_api_enable_two_way, meta_api_impl_get);
        functions_to_finalize.push_back(BBL_FUNCTION(query_result.decision_bbl));
        BblInsertInCloud(bbl);

        FUNCTION_METAAPI_DATA(BBL_FUNCTION(query_result.decision_bbl))->is_getter = true;
        FUNCTION_METAAPI_DATA(BBL_FUNCTION(query_result.decision_bbl))->predicate = selected_predicate;

#if GENERATE_DOTS
        functions_to_plot.push_back(BBL_FUNCTION(query_result.decision_bbl));
#endif

        t_cfg_edge *result_edge = NULL;
        if (meta_api_impl_get) {
          if (!query_result.two_way)
            result_edge = decide_control_flow(query_result, count_max_setters, new_value, DEBUG_ARG_INVALID_CHECK);
        }
        else {
          result_edge = BBL_SUCC_FIRST(query_result.decision_bbl);
        }

        if (!meta_api_measuring) {
          if (testing) {
            t_bbl *new_bbl = CreateEmptyBblOnEdge(result_edge, false, false);

            if ((meta_api_project_name == "ninja")
                  || (meta_api_project_name == "python")
                  || (meta_api_project_name == "radare2")) {
              /* add BL to exit */
              t_arm_ins *ins;
              ArmMakeInsForBbl(UncondBranchAndLink, Append, ins, new_bbl, false);

              t_bbl *next = CFG_EDGE_TAIL(BBL_SUCC_FIRST(new_bbl));
              CfgEdgeKill(BBL_SUCC_FIRST(new_bbl));

              CallDynamicSymbol(cfg, "exit", T_INS(ins), new_bbl, next);
            }
            else if (meta_api_project_name == "keepassxc") {
              t_arm_ins *ins;
              ArmMakeInsForBbl(Add, Append, ins, new_bbl, false, ARM_REG_R13, ARM_REG_R13, ARM_REG_NONE, 0x8c, ARM_CONDITION_AL);
              ArmMakeInsForBbl(Pop, Append, ins, new_bbl, false, 0x8ff0, ARM_CONDITION_AL, false);
            }
          }
        }

        /* break fallthrough chains */
        break_fallthrough_chains(query_result.decision_bbl);

        if (meta_api_measuring) {
          if (test_configuration.resetter_identifier != "") {
            /* a resetter function was defined */
            MetaAPI_Setter resetter;
            resetter.function = MetaAPI_GetFunctionByName(test_configuration.resetter_identifier);
            resetter.configuration = test_configuration.resetter_configuration;

            string resetter_func_name = "MetaTF" + to_string(transformation_uid) + "_" + selected_predicate->predicate->Print() + "_UID" + to_string(selected_predicate->uid) + "_Resetter" + "_" + MetaAPI_Effect::EffectToString(test_configuration.resetter_value);

            SetterVerificationInfo resetter_verify;
            resetter_verify.expected = test_configuration.resetter_value;
            resetter_verify.predicate = selected_predicate;

            MetaAPI_TransformDatastructure(selected_predicate->instance, resetter.function, measure_loc3_bbl, resetter.configuration, resetter_func_name, resetter_verify, meta_api_impl_reset);
          }
        }

        if (dots_uid >= 0) {
          VERBOSE(meta_api_verbosity, ("DBG dumping aftertf%d", dots_uid));
          DumpDots(cfg, "aftertf", dots_uid);
        }

        if (success)
          MetaAPI_Liveness(selected_predicate);

        if (dots_uid >= 0) {
          VERBOSE(meta_api_verbosity, ("DBG dumping liveness%d", dots_uid));
          DumpDots(cfg, "liveness", dots_uid);
        }

        /* transformation succeeded, update counters */
        nr_transformed++;
      }
    }
  }

#if GENERATE_DOTS
  for (t_function *f : functions_to_plot) {
    t_string x = StringIo("%s.dot", FUNCTION_NAME(f));
    FunctionDrawGraph(f, x);
    Free(x);
  }
#endif

  return success;
}

void ARMMetaApiTransformation::initialise(t_cfg *cfg) {
  /* read in the meta-API information */
  MetaAPI_Init(cfg, RNGGetRootGenerator(), obfuscation_opaque_predicate_options.advanced_opaque_predicates_xml);

  /* don't transform anything before 'main' */
  if (!testing) {
    t_address before_address = AddressNew32(0);
    t_address after_address = AddressNew32(~0);

    bool have_before = false;
    bool have_after = false;

    string not_before_function_name = MetaAPI_GetConfigurationParameter("not_before_function", "");
    if (!not_before_function_name.empty()) {
      t_function *f = GetFunctionByName(cfg, not_before_function_name.c_str());
      ASSERT(f, ("can't find function specified by meta-API configuration parameter 'not_before_function': %s", not_before_function_name.c_str()));

      before_address = BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(f));
      have_before = true;
    }

    string not_after_latest_in_function_name = MetaAPI_GetConfigurationParameter("not_after_latest_in_function", "");
    if (!not_after_latest_in_function_name.empty()) {
      t_function *f = GetFunctionByName(cfg, not_after_latest_in_function_name.c_str());
      ASSERT(f, ("can't find function specified by meta-API configuration parameter 'not_after_function': %s", not_after_latest_in_function_name.c_str()));

      t_bbl *selected = NULL;

      t_bbl *bbl;
      FUNCTION_FOREACH_BBL(f, bbl) {
        if (!selected
            || (BBL_SEQUENCE_ID(bbl) > BBL_SEQUENCE_ID(selected)))
          selected = bbl;
      }

      ASSERT(selected, ("can't find bbl in @F", f));
      after_address = BBL_OLD_ADDRESS(selected);
      have_after = true;
    }
    else {
      string not_after_function_name = MetaAPI_GetConfigurationParameter("not_after_function", "");
      if (!not_after_function_name.empty()) {
        t_function *f = GetFunctionByName(cfg, not_after_function_name.c_str());
        ASSERT(f, ("can't find function specified by meta-API configuration parameter 'not_after_function': %s", not_after_function_name.c_str()));

        /* last block is somewhat harder */
        t_cfg_edge *i_edge;

        vector<t_bbl *> exit_blocks;
        BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_LAST(f),i_edge)
        {
          /* skip compensating edges */
          if (CFG_EDGE_CAT(i_edge) == ET_COMPENSATING)
            continue;
          
          if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge)))
            continue;

          exit_blocks.push_back(CFG_EDGE_HEAD(i_edge));
        }

        if (exit_blocks.size() == 1)
          after_address = BBL_OLD_ADDRESS(exit_blocks[0]);
        else
          ASSERT_WITH_DOTS((cfg, "exits"), false, ("can't find unique exit point for function @F (before @G), got %d blocks", f, before_address, exit_blocks.size()));
        
        have_after = true;
      }
    }

    VERBOSE(AOP_VERBOSITY, ("sequenced before @G, after @G", before_address, after_address));

    BblSet before, after;
    if (have_before || have_after)
      ReadSequenceData(cfg, diabloflowgraph_options.blocksequencefile, before_address, before, after_address, after);

    /* default: all blocks can be transformed */
    t_bbl *bbl;
    CFG_FOREACH_BBL(cfg, bbl)
      BBL_SET_SEQUENCE_ID(bbl, 1ULL);
    
    /* before */
    for (auto bbl : before)
      BBL_SET_SEQUENCE_ID(bbl, 0ULL);
    /* after */
    for (auto bbl : after)
      BBL_SET_SEQUENCE_ID(bbl, 2ULL);
  }

  VERBOSE(AOP_VERBOSITY, ("These are the functions that can't be transformed with advanced opaque predicates:"));
  for(auto f : implementing_functions) {
    FunctionUID bbl_function;
    SourceFileUID bbl_object;
    SourceArchiveUID bbl_archive;
    BblSourceLocation(FUNCTION_BBL_FIRST(f), bbl_function, bbl_object, bbl_archive);

    string filename = "(null)";
    if (bbl_object != SourceFileUID_INVALID)
      filename = GetSourceFileName(bbl_object);
    VERBOSE(AOP_VERBOSITY, ("  - @F\n    %s", f, filename.c_str()));
  }

  /* don't execute this function twice */
  is_initialised = true;
}

/* Check whether a single BBL can be transformed or not.
 * No checks beyond the BBL are done here.
 * Parameters:
 *  bbl The BBL that should be checked.
 * Returns:
 *  TRUE if the BBL can be transformed, FALSE otherwise. */
bool ARMMetaApiTransformation::canTransformBbl(t_bbl* bbl) const {
  /* possibly can, but won't transform BBLs that are not part of any function */
  t_function *fun = BBL_FUNCTION(bbl);

  if (!fun) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: not part of any function", bbl));
    global_stats.bbl_not_in_function++;
    status_code = STATUS_NOT_IN_FUNCTION;
    return false;
  }

#define N "_GLOBAL__sub_I"
  if (FUNCTION_NAME(fun) && !strncmp(FUNCTION_NAME(fun), N, strlen(N))) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: part of global constructor", bbl));
    status_code = STATUS_GLOBAL_CONSTRUCTOR;
    return false;
  }
#undef N

#define N "DIABLO_"
  if (FUNCTION_NAME(fun) && !strncmp(FUNCTION_NAME(fun), N, strlen(N))) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: part of Diablo functionality", bbl));
    status_code = STATUS_DIABLO_FUNCTION;
    return false;
  }
#undef N

#define N "call_weak_fn"
  if (FUNCTION_NAME(fun) && !strcmp(FUNCTION_NAME(fun), N)) {
    status_code = STATUS_CALL_WEAK_FN;
    return false;
  }
#undef N

  if (FunctionIsMeta(fun)) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: inside meta function", bbl));
    status_code = STATUS_META_FUNCTION;
    return false;
  }

  /* */
  if (BBL_SEQUENCE_ID(bbl) != 1) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: not allowed by sequence", bbl));
    global_stats.before_sequence++;
    status_code = STATUS_SEQUENCE;
    return false;
  }

  /* can't transform parts of untouchable functions */
  if(!MetaAPI_CanTransformFunction(fun)) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: part of untouchable function", bbl));
    global_stats.bbl_in_implementation++;
    status_code = STATUS_UNTOUCHABLE;
    return false;
  }

  /* can't transform HELL functions */
  if(FUNCTION_IS_HELL(fun)) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: part of HELL function", bbl));
    global_stats.bbl_hell++;
    status_code = STATUS_HELL_FUNCTION;
    return false;
  }

  /* at least one instruction should be present where the statusflags are dead */
  vector<t_ins*> goodPlaces = FindPlacesWhereFlagsAreDead( bbl );
  if( goodPlaces.size() == 0 ) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: no places where statusflags are dead", bbl));
    global_stats.bbl_no_dead_flags++;
    status_code = STATUS_STATUS_FLAGS;
    return false;
  }

  /* special case for _start etc */
  if (!BBL_SUCC_FIRST(bbl)) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: no successor BBL", bbl));
    global_stats.bbl_no_successor++;
    status_code = STATUS_NO_SUCC;
    return false;
  }

  /* DATA blocks can't be transformed */
  if (BBL_ATTRIB(bbl) & BBL_IS_DATA) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: data BBL", bbl));
    global_stats.bbl_data++;
    status_code = STATUS_DATA;
    return false;
  }

  /* TODO: fixme */
  if (CFG_EDGE_NEXT(BBL_SUCC_FIRST(bbl)) == NULL
      && BBL_ATTRIB(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl))) & BBL_IS_DATA) {
    VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: successor BBL is data BBL", bbl));
    global_stats.bbl_data_successor++;
    status_code = STATUS_SUCC_DATA;
    return false;
  }

  /* for now: can't transform BBLs that are part of a switch statement */
  /* TODO: could be fixed (probably?) */
  t_cfg_edge *edge;
  BBL_FOREACH_PRED_EDGE(bbl, edge) {
    t_cfg_edge* edge2;
    t_bbl* head = CFG_EDGE_HEAD(edge);
    BBL_FOREACH_SUCC_EDGE(head, edge2) {
      if (CfgEdgeIsSwitch(edge2)) {
        VERBOSE(AOP_VERBOSITY+AOP_VERBOSITY_INC, ("can't transform @B: related to switch statement", bbl));
        global_stats.switch_related++;
        status_code = STATUS_SWITCH_RELATED;
        return false;
      }
    }
  }

  status_code = STATUS_OK;
  return true;
}

void MetaAPI_PerformTest(t_cfg *cfg, t_string test_name) {
  BBLObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<BBLObfuscationTransformation>("meta_api", RNGGetRootGenerator());

  ARMMetaApiTransformation *meta_api_transformation = dynamic_cast<ARMMetaApiTransformation *>(obfuscator);
  meta_api_transformation->testing = true;
  meta_api_transformation->test_name = test_name;
  meta_api_transformation->test_cfg = cfg;
  meta_api_transformation->doTransform(NULL, meta_api_rng);
}
