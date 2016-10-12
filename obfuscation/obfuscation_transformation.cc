/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <map>
#include <string>
#include <set>
#include <algorithm>

extern "C" {
#include <diabloanopt.h>
}

#include <diabloannotations.h>
#include "obfuscation_transformation.h"
#include "obfuscation_json.h"
#include <obfuscation_opt.h>

using namespace std;

BBLObfuscationTransformation::BBLObfuscationTransformation() {
  RegisterTransformationType(this, name());
}

FunctionObfuscationTransformation::FunctionObfuscationTransformation() {
  RegisterTransformationType(this, name());
}

bool flatten_always_enabled = false;

static bool all_obfuscations_enabled = false;
bool AllObfuscationsEnabled() {
  return all_obfuscations_enabled;
}

void SetAllObfuscationsEnabled(bool enable) {
  VERBOSE(0, ("Setting all obfuscations enabled to: %i", (int) enable));
  all_obfuscations_enabled = enable;
}

static bool diversity_transforms = false;
void SetUseDiversityTransforms(bool enable) {
  diversity_transforms = enable;
}
bool DiversityTransformsEnabled() {
  return diversity_transforms;
}

bool FunctionIsBranchFunction(const t_function* fun) {
  t_const_string name = FUNCTION_NAME(fun);
  if (!name)
    return false;

  string s = string(name);
  return (s.find("BranchFunction") != s.npos);
}

/* TODO: augment this with some 'smart' detection of whether PC escapes? (only very basic pattern matching for x86 atm) */
bool DisallowedFunctionToTransform(const t_function* fun) {
  t_const_string name = FUNCTION_NAME(fun);

  if (!name)
    name = ""; /* FunctionReturnsPC will also match some name-less functions! */

  string s = string(name);
  if (   FunctionIsBranchFunction(fun)
      || (s.find("ReturnAddressStub") != s.npos)
      || GetArchitectureInfo(nullptr)->functionReturnsPC(fun) )
    return true;

  return false;
}

void AddRegisterToLiveOut(t_bbl* bbl, t_reg reg) {
  t_regset out = BBL_REGS_LIVE_OUT(bbl);
  RegsetSetAddReg(out, reg);
  BBL_SET_REGS_LIVE_OUT(bbl, out);
}

static t_function* GetFunction(t_cfg* cfg, const string& function_name) {
  t_function* fun;
  t_function* fun_s;
  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_s) {
    if (FUNCTION_NAME(fun) && function_name == FUNCTION_NAME(fun))
      return fun;
  }
  return nullptr;
}

static void DumpStats(t_const_string filename, t_cfg* cfg) {
  t_bbl* bbl;
  t_bbl* bbl_safe;

  t_function* fun;
  t_function* fun_safe;

  /* Stats */
  int tot_functions = 0;
  int tot_bbls = 0;
  int tot_insts = 0;
  int couldTransform = 0;

  string obfuscation_class;
  bool obfuscation_class_set = false;

  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_safe) {
    if(FUNCTION_IS_HELL(fun)) {
      continue;
    }
    tot_functions++;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      tot_bbls++;
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        tot_insts++;
      }
    }
  }

  string prefix = filename;
  prefix = prefix + ",";

  VERBOSE(0, ("%s,All_Stats,functions_transformed,%i", filename, tot_functions));
  VERBOSE(0, ("%s,All_Stats,bbls_transformed,%i", filename, tot_bbls));
  VERBOSE(0, ("%s,All_Stats,insts_in_bbls,%i", filename, tot_insts));

  auto bbl_obfuscators = GetTransformationsForType("bbl_obfuscation");

  if (bbl_obfuscators.size() > 0) {
    for (auto obfuscator: bbl_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No bbl_obfuscation found, skipping"));
  }

  auto function_obfuscators = GetTransformationsForType("function_obfuscation");

  if (function_obfuscators.size() > 0) {
    for (auto obfuscator: function_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No function_obfuscation found, skipping"));
  }
}

/* The obfuscation names specified in D5.01 might not necessarily match the internal names given,
   do a match on that here.
   TODO: deal with the ALL meta-parameter. */
static map<string, set<string>> internal_to_possible_annotation_names = {
  { "opaquepredicate", { "opaque_predicate" } },
  { "flattenfunction", { "flatten_function" } },
  { "branchfunction", { "branch_function" } },
  { "callfunction", { "call_function" } }
};
static bool AnnotationRequestMatches(const string& internal_name, const string annotation_name) {
  VERBOSE(1, ("Trying match for annotation request name '%s' to internal name '%s'...", annotation_name.c_str(), internal_name.c_str()));

  auto it = internal_to_possible_annotation_names.find(internal_name);
  if (it == internal_to_possible_annotation_names.end()) {
    VERBOSE(1, ("No match found!"));
    return false;
  }

  bool b = (*it).second.find(annotation_name) != (*it).second.end();
  VERBOSE(1, ((b ? "Match found!" : "No match found!")));
  return b;
}

t_bool AlwaysSelectBblFromRegion(t_bbl *) {
  return TRUE;
}

int GetAnnotationValueWithDefault(const AnnotatedBbl& abbl, const string& key, int d) {
  if (abbl.second.find(key) == abbl.second.end())
    return d;
  return (*(abbl.second.find(key))).second;
}

AnnotatedBblPartition PartitionAnnotatedBbls(const vector<AnnotatedBbl>& bbls, const CompatibilityPartitioner& p_compatible) {
  /* TODO smarter, faster */
  vector<vector<pair<t_bbl*, AnnotationIntOptions>>> final_partitioning;
  set<t_bbl*> partitioned;

  VERBOSE(0, ("Begin partitioning"));

  for (auto bbl_pair: bbls) {
    vector<pair<t_bbl*, AnnotationIntOptions>> current_partition;

    if (partitioned.find(bbl_pair.first) != partitioned.end())
      continue;

    current_partition.push_back(bbl_pair);
    partitioned.insert(bbl_pair.first);

    for (auto bbl_pair2: bbls) {
      if (partitioned.find(bbl_pair2.first) != partitioned.end())
        continue;

      if (p_compatible.compare(bbl_pair, bbl_pair2)) {
        current_partition.push_back(bbl_pair2);
        partitioned.insert(bbl_pair2.first);
      }
    }

    final_partitioning.push_back(current_partition);
  }

  VERBOSE(0, ("End partitioning"));

  return final_partitioning;
}

AnnotatedBblPartition SelectBblsForAllRegionsFor(t_cfg* cfg, /*const TODO */ string& obfuscation_string, const CompatibilityPartitioner& p_compatible, t_randomnumbergenerator *rng_obfuscation) {
  /* First, we iterate over all regions that match this obfuscation type. We get a total list where for each Region, BBLs are selected with the apply_chance
   * that applies to each region. The choice is totally random if no profile info is given; otherwise, we take the apply_chance lowest execution count BBLs
   * for each region (TODO: this is not ideal for small/split Regions, ideally you group all BBLs together of each different apply_chance, order those, and select the lowest).
   * Next, we partition these selected regions with the Partitioner, this decides if two BBLs can be processed together or not (they might have different or conflicting
   * options, for example). */
  Region *region;
  const ObfuscationAnnotationInfo *info;

  vector<AnnotatedBbl> all_bbls;

  CFG_FOREACH_OBFUSCATION_REGION(cfg, region, info)
  {
    for (auto request : region->requests)
    {
      auto obfuscation_ = dynamic_cast<ObfuscationAnnotationInfo*>(request);
      ASSERT(obfuscation_, ("Internal inconsistency: annotation type is Obfuscation, but did not get ObfuscationAnnotationInfo*"));
      const auto& obfuscation = *obfuscation_; /* TODO a bit ugly */

      ASSERT(obfuscation.name != "ALL", ("TODO: implement support for the 'ALL' obfuscation target"));

      /* If this annotation is not about the obfuscation we currently try to apply: skip it */
      VERBOSE(1, ("Trying annotation request for %s while in phase %s...", obfuscation.name.c_str(), obfuscation_string.c_str()));

      if (!AnnotationRequestMatches(obfuscation_string, obfuscation.name))
        continue;

      VERBOSE(1, ("Annotation request can proceed"));

      int percent_apply = 25;

      VERBOSE(0, (" -> applying obfuscation '%s' (enabled: %i)", obfuscation.name.c_str(), obfuscation.enable));
      for (auto option: obfuscation.options) {
        VERBOSE(0, ("   -> option: '%s' = %i", option.first.c_str(), option.second));

        if (option.first == "percent_apply") {
          percent_apply = option.second;
          ASSERT(percent_apply >= 0 && percent_apply <= 100, ("Invalid percent_apply: %i not a percentage", percent_apply));
        } else if (option.first == "choose_percent_apply_automatically" && option.second == 1) {
          t_randomnumbergenerator *rng = RNGCreateChild(rng_obfuscation, "percent_apply");
          RNGSetRange(rng, 0, 100);
          percent_apply = RNGGenerate(rng);
          RNGDestroy(rng);
          VERBOSE(0, ("Picking a random percentage to apply, chose: %i!", percent_apply));
        }
      }

      if (obfuscation.enable) {
        /* Get a set of basic blocks to transform. For all blocks in the function, we add it with the percent_apply chance. */
        vector<t_bbl*> bbls;
        SelectBblsFromRegion(bbls, AlwaysSelectBblFromRegion, region, percent_apply, rng_obfuscation);

        for (auto bbl: bbls) {
          all_bbls.push_back(make_pair(bbl, request->options));
        }
      }
    }
  }

  return PartitionAnnotatedBbls(all_bbls, p_compatible);
}

struct NonDiscriminatoryPartitioning : public CompatibilityPartitioner {
  virtual bool compare(const AnnotatedBbl&, const AnnotatedBbl&) const { return true; }
};

void CfgObfuscateRegions(t_cfg* cfg) {
  t_randomnumbergenerator *rng_obfuscation = RNGCreateChild(RNGGetRootGenerator(), "obfuscations");
  t_randomnumbergenerator *rng_opaquepredicate = RNGCreateChild(rng_obfuscation, "opaquepredicate");
  t_randomnumbergenerator *rng_branchfunction = RNGCreateChild(rng_obfuscation, "branchfunction");
  t_randomnumbergenerator *rng_flattenfunction = RNGCreateChild(rng_obfuscation, "flattenfunction");

  SetAllObfuscationsEnabled(true);

  VERBOSE(0, ("Applying obfuscations from JSON file"));

  /* Ensure liveness information is correct */
  CfgComputeLiveness(cfg, TRIVIAL);
  CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
  CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);

  /* TODO: verify that if an annotation request comes in for something that we do not know: WARN/FATAL */
  string obfuscations[] = { "opaquepredicate", "branchfunction", "flattenfunction" };
  for (auto obfuscation_string: obfuscations) {
    NewDiabloPhase(obfuscation_string.c_str());

    auto obfuscation_partitions = SelectBblsForAllRegionsFor(cfg, obfuscation_string, NonDiscriminatoryPartitioning(), rng_obfuscation);

    for (auto partition: obfuscation_partitions) {
      /* flattening is special: a region-based, non-local transformation */
      if (obfuscation_string == "flattenfunction") {
        FunctionObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<FunctionObfuscationTransformation>("flattenfunction", rng_flattenfunction);
        ASSERT(obfuscator, ("Did not find any obfuscator for type '%s'", obfuscation_string.c_str()));

        if (obfuscator->canTransformRegion(partition)) {
          VERBOSE(1, ("Applying '%s' to a region of BBLs of size %i", obfuscator->name(), (int) partition.size()));
          obfuscator->doTransformRegion(partition, rng_flattenfunction);
        } else {
          VERBOSE(1, ("WARNING: tried applying '%s', but FAILED", obfuscator->name()));
        }
      } else {
        for (auto bbl_options: partition) {
          auto bbl = bbl_options.first;
          t_randomnumbergenerator* rng = (obfuscation_string == "branchfunction") ? rng_branchfunction : rng_opaquepredicate;

          /* TODO: obfuscation_string is probably too broad once/if we get rid of the strict sequencing of transformations */
          BBLObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<BBLObfuscationTransformation>(obfuscation_string.c_str(), rng);
          ASSERT(obfuscator, ("Did not find any obfuscator for type '%s'", obfuscation_string.c_str()));

          if (obfuscator->canTransform(bbl)) {
            VERBOSE(1, ("Applying '%s' to @eiB", obfuscator->name(), bbl));
            obfuscator->doTransform(bbl, rng);
          } else {
            VERBOSE(1, ("WARNING: tried applying '%s' to @eiB, but FAILED", obfuscator->name(), bbl));
          }
        }
#if 0
        /* Very conservative: re-compute liveness after each obfuscation application. This shouldn't be necessary, but it's always possible that there's some bug
           left. In case of doubt: re-enable this temporarily and check if an observed bug disappears.... */
        CfgComputeLiveness(cfg, TRIVIAL);
        CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
        CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);
#endif
      }
    }

    CfgComputeLiveness(cfg, TRIVIAL);
    CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
    CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);
  }

  CfgComputeLiveness(cfg, TRIVIAL);
  CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
  CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);

  DumpStats("obfuscations", cfg);

  VERBOSE(0, ("DONE Obfuscating with script"));
  VERBOSE(0, ("WARNING TODO: use the pct parameter"));

  RNGDestroy(rng_opaquepredicate);
  RNGDestroy(rng_branchfunction);
  RNGDestroy(rng_flattenfunction);
  RNGDestroy(rng_obfuscation);
}

void ObjectObfuscate(t_const_string filename, t_object* obj)
{
  t_cfg* cfg = OBJECT_CFG(obj);
  t_bbl* bbl;
  t_bbl* bbl_safe;

  /* TODO: use diablo randomness so it is shared among the obfuscation transformations. */
  uint32_t seed_val = 0; /* TODO: initialize with -drs */

  static int i = 0;

  t_function* fun;
  t_function* fun_safe;

  /* Stats */
  int tot_functions = 0;
  int tot_bbls = 0;
  int tot_insts = 0;
  int couldTransform = 0;

  string obfuscation_class;
  bool obfuscation_class_set = false;

  t_randomnumbergenerator *rng_obfuscation = RNGCreateChild(RNGGetRootGenerator(), "obfuscations");
  t_randomnumbergenerator *rng_bbl = RNGCreateChild(rng_obfuscation, "bbl_obfuscation");
  t_randomnumbergenerator *rng_fun = RNGCreateChild(rng_obfuscation, "function_obfuscation");

  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_safe) {
    if(FUNCTION_IS_HELL(fun)) {
      continue;
    }
    tot_functions++;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      tot_bbls++;
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        tot_insts++;
      }
    }
  }

  string prefix = filename;
  prefix = prefix + ",";

  VERBOSE(0, ("%s,All_Stats,functions_transformed,%i", filename, tot_functions));
  VERBOSE(0, ("%s,All_Stats,bbls_transformed,%i", filename, tot_bbls));
  VERBOSE(0, ("%s,All_Stats,insts_in_bbls,%i", filename, tot_insts));

  vector<t_bbl*> bbls;
  vector<t_function*> funs;
  uniform_int_distribution<uint32_t> uniform_selector(0,99);

  /* So we don't double-split BBLs. Right now, don't transform created code, this will change in the future (TODO) */
  bbls.clear();
  CFG_FOREACH_FUNCTION_SAFE(cfg, fun, fun_safe) {
    funs.push_back(fun);

    t_bbl* bbl;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      bbls.push_back(bbl);
    }
  }

  //CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_before");

  /* TODO ensure the liveness is correct through transformations... */
  /* TODO: audit all transformations, check that all BBLs they introduce contain correct liveness, REGS_DEFINED_IN, etc */

  CfgComputeLiveness (cfg, TRIVIAL);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters (cfg);
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

#if 1 /* BBL transformations */

  i = 0;
  couldTransform = 0;

//  CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_before");

  if (ConstantPropagationInit(cfg))
    ConstantPropagation(cfg, CONTEXT_SENSITIVE);
  else
    FATAL(("ERR\n"));

  /* TODO: this is also needed, but why isn't the above enough? */
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(fun))) {
      FunctionUnmarkAllBbls(fun);
      FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);
    }
  }

  auto bbl_obfuscators = GetTransformationsForType("bbl_obfuscation", true);
  if (strcmp(obfuscation_obfuscation_options.obfuscation_bbl_class, "") != 0) {
    /* Because we check whether or not this is really a function_obfuscation later, this is ok */
    bbl_obfuscators = GetTransformationsForType(obfuscation_obfuscation_options.obfuscation_bbl_class);

    VERBOSE(0, ("Only allowing bbl obfuscators of class '%s'", obfuscation_obfuscation_options.obfuscation_bbl_class));
  }


  if (bbl_obfuscators.size() > 0) {
    t_randomnumbergenerator *rng_apply_chance = RNGCreateChild(rng_bbl, "apply_chance");
    RNGSetRange(rng_apply_chance, 0, 100);

    t_bool is_random = TRUE;
    t_randomnumbergenerator *rng_bbl_obfuscator = RNGCreateChild(rng_bbl, "bbl_obfuscator");

    if (bbl_obfuscators.size() > 1) {
      is_random = TRUE;
      RNGSetRange(rng_bbl_obfuscator, 0, bbl_obfuscators.size() - 1);
    }
    else {
      is_random = FALSE;
    }

    for (auto bbl: bbls) {
      auto obfuscator_raw = bbl_obfuscators.at(is_random ? RNGGenerate(rng_bbl_obfuscator) : 0);
      auto obfuscator = dynamic_cast<BBLObfuscationTransformation*>(obfuscator_raw); // TODO cleaner

      ASSERT(obfuscator, ("Expected that the obfuscator '%s' was a bbl_obfuscation, but it wasn't!", obfuscator_raw->name()));

      if (RNGGenerate(rng_apply_chance) < 50) { /* Chance 50% */
        if (obfuscator->canTransform(bbl) /*&& i < diablosupport_options.debugcounter*/) {
          fun = BBL_FUNCTION(bbl);

          VERBOSE(0, ("Transforming a BBL of function %s with obfuscator %s", FUNCTION_NAME(fun), obfuscator->name()));

          //FunctionDrawGraph(fun, "before.dot");
          obfuscator->doTransform(bbl, rng_bbl);
          //FunctionDrawGraph(fun, "after.dot");
          i++;


          /*CfgComputeLiveness (cfg, TRIVIAL);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
          CfgComputeSavedChangedRegisters (cfg);
          CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);*/
        }
      }
    }

    RNGDestroy(rng_bbl_obfuscator);
    RNGDestroy(rng_apply_chance);

    for (auto obfuscator: bbl_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No bbl_obfuscation found, skipping"));
  }

  //CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_after");

#endif



#if 1 /* function transformations */
  auto function_obfuscators = GetTransformationsForType("function_obfuscation", true);

  if (strcmp(obfuscation_obfuscation_options.obfuscation_fun_class, "") != 0) {
    /* Because we check whether or not this is really a function_obfuscation later, this is ok */
    function_obfuscators = GetTransformationsForType(obfuscation_obfuscation_options.obfuscation_fun_class);

    VERBOSE(0, ("Only allowing function obfuscators of class '%s'", obfuscation_obfuscation_options.obfuscation_fun_class));
  }

  if (function_obfuscators.size() > 0) {
    t_bool is_random = TRUE;
    t_randomnumbergenerator *rng_function_obfuscator = RNGCreateChild(rng_fun, "function_obfuscator");

    if (function_obfuscators.size() > 1) {
      is_random = TRUE;
      RNGSetRange(rng_function_obfuscator, 0, function_obfuscators.size() - 1);
    }
    else {
      is_random = FALSE;
    }


    for (auto fun: funs) {
      auto obfuscator_raw = function_obfuscators.at(is_random ? RNGGenerate(rng_function_obfuscator) : 0);
      auto obfuscator = dynamic_cast<FunctionObfuscationTransformation*>(obfuscator_raw); // TODO cleaner

      ASSERT(obfuscator, ("Expected that the obfuscator '%s' was a function_obfuscation, but it wasn't!", obfuscator_raw->name()));
      VERBOSE(0, ("CanTransform? %s %s", FUNCTION_NAME(fun), obfuscator->name()));
      if (obfuscator->canTransform(fun)/* && i < diablosupport_options.debugcounter*/) { // Always apply when possible
        VERBOSE(0, ("Transforming function %s with obfuscator %s", FUNCTION_NAME(fun), obfuscator->name()));

        //FunctionDrawGraph(fun, "before.dot");
        obfuscator->doTransform(fun, rng_fun);
        //FunctionDrawGraph(fun, "after.dot");
        i++;
      }
    }

    RNGDestroy(rng_function_obfuscator);

    for (auto obfuscator: function_obfuscators) {
      obfuscator->dumpStats(string(prefix));
    }
  } else {
    VERBOSE(0, ("No function_obfuscation found, skipping"));
  }
#else

  auto function_obfuscators = GetTransformationsForType("flattenfunction", true);

  if (strcmp(obfuscation_obfuscation_options.obfuscation_fun_class, "") != 0) {
    /* Because we check whether or not this is really a function_obfuscation later, this is ok */
    function_obfuscators = GetTransformationsForType(obfuscation_obfuscation_options.obfuscation_fun_class);

    VERBOSE(0, ("Only allowing function obfuscators of class '%s'", obfuscation_obfuscation_options.obfuscation_fun_class));
  }

  auto obfuscator_raw = function_obfuscators.at(0);
  auto obfuscator = dynamic_cast<FunctionObfuscationTransformation*>(obfuscator_raw); // TODO cleaner
  obfuscator->doTransformRegion(bbls);
  CfgPartitionFunctions (cfg, has_incoming_interproc, always_true);
#endif

  RNGDestroy(rng_bbl);
  RNGDestroy(rng_fun);
  RNGDestroy(rng_obfuscation);
}
