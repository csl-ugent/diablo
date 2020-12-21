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

#define DEBUG_OBFUSCATIONS diablosupport_options.debugcounter

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(can_transform);
bool initialisation_phase = true;

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
  { "callfunction", { "call_function" } },
  { "meta_api", { "meta_api" } }
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

      int percent_apply = 100;

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
        SelectBblsFromRegion(bbls, AlwaysSelectBblFromRegion, region, percent_apply, rng_obfuscation, FALSE, FALSE);

        for (auto bbl: bbls) {
          all_bbls.push_back(make_pair(bbl, request->options));
        }
      }
    }
  }

  return PartitionAnnotatedBbls(all_bbls, p_compatible);
}

AnnotatedBblPartition SelectBlocks(t_cfg *cfg, AnnotatedBblPartition data, BBLObfuscationTransformation* obfuscator, t_randomnumbergenerator *rng) {
  constexpr int OBF_HOTNESS_MAX = 1000;
  ASSERT(0 <= obfuscation_obfuscation_options.obf_hotness
          && obfuscation_obfuscation_options.obf_hotness <= OBF_HOTNESS_MAX, ("--obf-hotness value %u should be within range [0, %d]", obfuscation_obfuscation_options.obf_hotness, OBF_HOTNESS_MAX));
  ASSERT(0 <= obfuscation_obfuscation_options.obf_hotness_threshold
          && obfuscation_obfuscation_options.obf_hotness_threshold <= 100, ("--obf-hotness-threshold value %u out of range [0, 100]", obfuscation_obfuscation_options.obf_hotness_threshold));
  ASSERT(0 <= obfuscation_obfuscation_options.obf_exec_threshold
          && obfuscation_obfuscation_options.obf_exec_threshold <= 100, ("--obf-exec-threshold value %u out of range [0, 100]", obfuscation_obfuscation_options.obf_exec_threshold));
  ASSERT(0 <= obfuscation_obfuscation_options.obf_considered_minimum,
          ("--obf-considered-minimum should be positive"));

  /* only support one set of BBLs for now */
  ASSERT(data.size() == 1, ("unexpected size %d", data.size()));

  string boutfilename = OutputFilename();
  string all_bbls_filename = boutfilename + ".all_bbls";
  string considered_bbls_filename = boutfilename + ".considered_bbls";
  string selected_bbls_filename = boutfilename + ".obf_bbls";

  struct FunctionData {
    // no need for a float variables here as the fractional part does not matter much anyway
    size_t hotness;
    size_t nr_bbl_hotness;
    size_t mean_hotness;
    size_t consider_hotness_threshold;

    size_t exec;
    size_t nr_bbl_exec;
    size_t mean_exec;
    size_t consider_exec_threshold;

    size_t considered_value;
    vector<t_bbl *> considered_bbls;
    map<int, int> considered_reasons;
    map<int, int> considered_nottf_reasons;

    size_t nr_bbls;

    FunctionData() {
      hotness = 0;
      nr_bbl_hotness = 0;
      mean_hotness = 0;
      consider_hotness_threshold = ~0;

      exec = 0;
      nr_bbl_exec = 0;
      mean_exec = 0;
      consider_exec_threshold = ~0;

      considered_value = 0;

      nr_bbls = 0;
    }

    string toString() {
      static const int BUF_LEN = 25;
      string result = "";

      char buf[BUF_LEN+1];

      snprintf(buf, BUF_LEN, "%11lu ", hotness);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", nr_bbl_hotness);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", mean_hotness);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", consider_hotness_threshold);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", exec);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", nr_bbl_exec);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", mean_exec);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", consider_exec_threshold);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", considered_value);
      result += buf;
      snprintf(buf, BUF_LEN, "%11lu ", considered_bbls.size());
      result += buf;

      return result;
    }
  };
  map<t_function *, FunctionData> function_data;

  /* helper to compute a BBL's hotness value */
  auto compute_bbl_hotness = [] (t_bbl *bbl) {
    return static_cast<size_t>(BBL_NINS(bbl)) * static_cast<size_t>(BBL_EXEC_COUNT(bbl));
  };

  auto not_tfable_string = [obfuscator] (map<int, int> reasons, size_t nr_bbls) {
    string result = "";

    for (auto p : reasons) {
      string sub = obfuscator->statusToString(p.first);
      sub += "=";
      sub += to_string(p.second);
      sub += " (";
      size_t mean = 100.0*p.second/nr_bbls;
      sub += to_string(mean);
      sub += " %), ";

      result += sub;
    }

    return result;
  };

#define REASON_NONE 0
#define REASON_NOT_TRANSFORMABLE 1
#define REASON_THRESHOLD 2
  auto consider_bbl = [&function_data, obfuscator] (t_bbl *bbl, int& reason, int& not_tf_reason) {
    if (BBL_EXEC_COUNT(bbl) < static_cast<t_int64>(function_data[BBL_FUNCTION(bbl)].consider_exec_threshold)) {
      reason = REASON_THRESHOLD;
      return false;
    }

    /* can the bbl be transformed by the selected obfuscator? */
    if (!obfuscator->canTransform(bbl)) {
      reason = REASON_NOT_TRANSFORMABLE;
      not_tf_reason = obfuscator->statusCode();
      return false;
    }

    reason = REASON_NONE;
    return true;
  };

  vector<t_function *> all_functions;

  size_t nr_ice_cold_functions = 0;

  LogFile *L_ALL_BBLS = NULL;
  INIT_LOGGING(L_ALL_BBLS, all_bbls_filename.c_str());

  /* collect profile data */
  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    /* don't conside Diablo-specific functions */
    if (FUNCTION_IS_HELL(fun))
      continue;

    /* don't include runtime functions */
    if (FUNCTION_NAME(fun)
        && (StringPatternMatch("__*", FUNCTION_NAME(fun))
            || StringPatternMatch(".*", FUNCTION_NAME(fun))))
      continue;

    function_data[fun] = FunctionData();
    all_functions.push_back(fun);

    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      /* don't consider empty BBLs */
      if (BBL_NINS(bbl) == 0)
        continue;

      /* function data */
      function_data[fun].hotness += compute_bbl_hotness(bbl);
      function_data[fun].nr_bbl_hotness++;

      if (BBL_EXEC_COUNT(bbl) > 0) {
        function_data[fun].exec += BBL_EXEC_COUNT(bbl);
        function_data[fun].nr_bbl_exec++;
      }

      LOG_MESSAGE(L_ALL_BBLS, "0x%x,%" PRId64 "\n", BBL_OLD_ADDRESS(bbl), BBL_EXEC_COUNT(bbl));
    }

    if (function_data[fun].hotness == 0) {
      nr_ice_cold_functions++;
    }
    else {
      function_data[fun].mean_hotness = function_data[fun].hotness / function_data[fun].nr_bbl_hotness;
      function_data[fun].consider_hotness_threshold = 1.0*obfuscation_obfuscation_options.obf_hotness_threshold/100 * function_data[fun].mean_hotness;

      function_data[fun].mean_exec = function_data[fun].exec / function_data[fun].nr_bbl_exec;
      function_data[fun].consider_exec_threshold = 1.0*obfuscation_obfuscation_options.obf_exec_threshold/100 * function_data[fun].mean_exec;

      VERBOSE(0, ("hotness %11llu/%11u=%11llu (threshold %11llu) exec %11llu/%11u=%11llu (threshold %11llu) @F",
        function_data[fun].hotness, function_data[fun].nr_bbl_hotness, function_data[fun].mean_hotness, function_data[fun].consider_hotness_threshold,
        function_data[fun].exec, function_data[fun].nr_bbl_exec, function_data[fun].mean_exec, function_data[fun].consider_exec_threshold,
        fun));
    }
  }

  FINI_LOGGING(L_ALL_BBLS);

  VERBOSE(0, ("found %d functions in total", all_functions.size()));
  VERBOSE(0, ("  %d functions are ice cold and will not be considered for transformation", nr_ice_cold_functions));
  VERBOSE(0, ("  %d functions remain", all_functions.size()-nr_ice_cold_functions));

  /* explicitely enable blocks that we want to transform */
  vector<t_function *> considered_functions;

  map<int, int> all_reasons;
  map<int, int> all_nottf_reasons;

  size_t total_nr_bbls = 0;

  LogFile *L_CONSIDERED_BBLS = NULL;
  INIT_LOGGING(L_CONSIDERED_BBLS, considered_bbls_filename.c_str());

  LogFile *L_SELECTED_BBLS = NULL;
  INIT_LOGGING(L_SELECTED_BBLS, selected_bbls_filename.c_str());

  for (auto fun : all_functions) {
    /* skip functions that are not executed at all */
    bool consider_function = (function_data[fun].hotness > 0);

    /* for other functions, count the number of possible insertion points */
    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl) {
      if (BBL_NINS(bbl) == 0)
        continue;

      int reason = REASON_NONE;
      int not_tf_reason = 0;
      if (consider_function && consider_bbl(bbl, reason, not_tf_reason)) {
        BBL_SET_CAN_TRANSFORM(bbl, true);

        function_data[fun].considered_value += compute_bbl_hotness(bbl);
        function_data[fun].considered_bbls.push_back(bbl);

        LOG_MESSAGE(L_CONSIDERED_BBLS, "0x%x,%" PRId64 "\n", BBL_OLD_ADDRESS(bbl), BBL_EXEC_COUNT(bbl));
      }

      LOG_MESSAGE(L_SELECTED_BBLS, "0x%x,%" PRId64 ",%d,%d,%d\n", BBL_OLD_ADDRESS(bbl), BBL_EXEC_COUNT(bbl), BBL_CAN_TRANSFORM(bbl), (reason == REASON_THRESHOLD), consider_function);

      if (consider_function) {
        /* record reasons for (non) consideration globally and per function */
        function_data[fun].considered_reasons[reason]++;
        if (reason == REASON_NOT_TRANSFORMABLE) {
          function_data[fun].considered_nottf_reasons[not_tf_reason]++;
          all_nottf_reasons[not_tf_reason]++;
        }
        function_data[fun].nr_bbls++;
        all_reasons[reason]++;

        total_nr_bbls++;
      }
    }

    /* only consider this function if it contains at least the required number of considered BBLs */
    if (function_data[fun].considered_bbls.size() >= static_cast<size_t>(obfuscation_obfuscation_options.obf_considered_minimum))
      considered_functions.push_back(fun);
  }

  FINI_LOGGING(L_CONSIDERED_BBLS);
  FINI_LOGGING(L_SELECTED_BBLS);

  VERBOSE(0, ("  %d functions contain enough considered blocks (>= %d)", considered_functions.size(), obfuscation_obfuscation_options.obf_considered_minimum));
  VERBOSE(0, ("BBL statistics: accepted=%d (%.2f %%) not_transformable=%d (%.2f %%) below_threshold=%d (%.2f %%)",
    all_reasons[REASON_NONE], 100.0*all_reasons[REASON_NONE]/total_nr_bbls,
    all_reasons[REASON_NOT_TRANSFORMABLE], 100.0*all_reasons[REASON_NOT_TRANSFORMABLE]/total_nr_bbls,
    all_reasons[REASON_THRESHOLD], 100.0*all_reasons[REASON_THRESHOLD]/total_nr_bbls));
  VERBOSE(0, ("    %s", not_tfable_string(all_nottf_reasons, total_nr_bbls).c_str()));

  /* sort the considered functions according to the total hotness of the considered blocks */
  stable_sort(considered_functions.begin(), considered_functions.end(), [&function_data] (t_function *a, t_function *b) {
    /* return TRUE when 'a' is to be put before 'b' */
    size_t a_hotness = function_data[a].considered_value;
    size_t b_hotness = function_data[b].considered_value;

    return a_hotness > b_hotness;
  });

  int counter = 0;
  for (auto fun : considered_functions) {
    VERBOSE(0, ("sorted @F", fun));
    VERBOSE(0, ("    %s", function_data[fun].toString().c_str()));
    VERBOSE(0, ("    accepted=%d not_transformable=%d below_threshold=%d", function_data[fun].considered_reasons[REASON_NONE], function_data[fun].considered_reasons[REASON_NOT_TRANSFORMABLE], function_data[fun].considered_reasons[REASON_THRESHOLD]));
    VERBOSE(0, ("    not_transformable reasons: %s", not_tfable_string(function_data[fun].considered_nottf_reasons, function_data[fun].nr_bbls).c_str()));
    counter++;
  }
  
  size_t first_selected_function = static_cast<size_t>(obfuscation_obfuscation_options.obf_first_function);
  ASSERT(0 <= first_selected_function && first_selected_function < considered_functions.size(), ("fist selected function out of range [0, %d]", considered_functions.size()));

  size_t nr_selected_functions = static_cast<size_t>(obfuscation_obfuscation_options.obf_nr_functions);

  /* construct result, sorted with considered BBL's having the highest hotness to BBL's having the lowest hotness value */
  AnnotatedBblPartition result;
  for (size_t x = first_selected_function; (x < (first_selected_function+nr_selected_functions)) && (x < considered_functions.size()); x++) {
    vector<AnnotatedBbl> selected_bbls;

    t_function *fun = considered_functions[x];
    vector<t_bbl *> considered_bbls = function_data[fun].considered_bbls;

    VERBOSE(0, ("queue function @F with %d considered BBLs", fun, considered_bbls.size()));

    shuffle(considered_bbls.begin(), considered_bbls.end(), RNGGetGenerator(rng));

    for (t_bbl *bbl : considered_bbls) {
      AnnotatedBbl bbl_options;
      bbl_options.first = bbl;
      bbl_options.second = AnnotationIntOptions();

      selected_bbls.push_back(bbl_options);
    }

    result.push_back(selected_bbls);
  }

  initialisation_phase = false;
  return result;
}

struct NonDiscriminatoryPartitioning : public CompatibilityPartitioner {
  virtual bool compare(const AnnotatedBbl&, const AnnotatedBbl&) const { return true; }
};

void CfgObfuscateRegions(t_cfg* cfg, bool generate_dots) {
  BblInitCanTransform(cfg);

  t_randomnumbergenerator *rng_obfuscation = RNGCreateChild(RNGGetRootGenerator(), "obfuscations");
  t_randomnumbergenerator *rng_opaquepredicate = RNGCreateChild(rng_obfuscation, "opaquepredicate");
  t_randomnumbergenerator *rng_branchfunction = RNGCreateChild(rng_obfuscation, "branchfunction");
  t_randomnumbergenerator *rng_flattenfunction = RNGCreateChild(rng_obfuscation, "flattenfunction");
  t_randomnumbergenerator *rng_shuffle = RNGCreateChild(rng_obfuscation, "obf_shuffle");

  SetAllObfuscationsEnabled(true);

  VERBOSE(0, ("Applying obfuscations from JSON file"));

  /* Ensure liveness information is correct */
  CfgComputeLiveness(cfg, TRIVIAL);
  CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
  CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);

  if (generate_dots)
  {
    t_string initial_dot_path = "diablo-obfuscator-dots-before-with-liveness";

    t_string callgraph_dot_file = StringConcat2(initial_dot_path, "/callgraph.dot");
    CfgDrawFunctionGraphsWithHotness (cfg, initial_dot_path);
    CgBuild (cfg);
    CgExport (CFG_CG(cfg), callgraph_dot_file);
    Free(callgraph_dot_file);
  }

  set<Transformation *> executed_transformations;

  if (obfuscation_obfuscation_options.meta_api_test_set) {
    /* perform a simple test on the meta API */
    VERBOSE(0, ("testing the meta API with test '%s'", obfuscation_obfuscation_options.meta_api_test));
    DiabloBrokerCall("MetaAPI_Test", cfg, obfuscation_obfuscation_options.meta_api_test);

    executed_transformations.insert(GetRandomTypedTransformationForType<BBLObfuscationTransformation>("meta_api", RNGGetRootGenerator()));
  }
  else {
    /* TODO: verify that if an annotation request comes in for something that we do not know: WARN/FATAL */
    // string obfuscations[] = { "opaquepredicate", "branchfunction", "flattenfunction", "meta_api" };
    string obfuscations[] = { "meta_api" };

#ifdef DEBUG_OBFUSCATIONS
    t_uint32 i = 0;
#endif

    BBLObfuscationTransformation *meta_api_obfuscator = GetRandomTypedTransformationForType<BBLObfuscationTransformation>("meta_api", rng_obfuscation);

    for (auto obfuscation_string: obfuscations) {
      NewDiabloPhase(obfuscation_string.c_str());

      auto obfuscation_partitions = SelectBblsForAllRegionsFor(cfg, obfuscation_string, NonDiscriminatoryPartitioning(), rng_obfuscation);

      /* sort according to hotness */
      obfuscation_partitions = SelectBlocks(cfg, obfuscation_partitions, meta_api_obfuscator, rng_shuffle);

      for (auto partition: obfuscation_partitions) {
#ifdef DEBUG_OBFUSCATIONS
        if (i >= DEBUG_OBFUSCATIONS) {
          break;
        }
#endif

        /* flattening is special: a region-based, non-local transformation */
        if (obfuscation_string == "flattenfunction") {
          FunctionObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<FunctionObfuscationTransformation>("flattenfunction", rng_flattenfunction);
          executed_transformations.insert(obfuscator);
          ASSERT(obfuscator, ("Did not find any obfuscator for type '%s'", obfuscation_string.c_str()));

          if (obfuscator->canTransformRegion(partition)) {
            VERBOSE(1, ("Applying '%s' to a region of BBLs of size %i", obfuscator->name(), (int) partition.size()));
            obfuscator->doTransformRegion(partition, rng_flattenfunction);
          } else {
            VERBOSE(1, ("WARNING: tried applying '%s', but FAILED", obfuscator->name()));
          }
        } else {
          int nr_transformed_in_partition = 0;

          for (auto bbl_options: partition) {
#ifdef DEBUG_OBFUSCATIONS
            if (i >= DEBUG_OBFUSCATIONS) {
              break;
            }
#endif

            if (nr_transformed_in_partition == obfuscation_obfuscation_options.obf_nr_per_partition) {
              VERBOSE(0, ("max number of transformations applied to this partition (%d)", obfuscation_obfuscation_options.obf_nr_per_partition));
              break;
            }

            auto bbl = bbl_options.first;
            t_randomnumbergenerator* rng = (obfuscation_string == "branchfunction") ? rng_branchfunction : rng_opaquepredicate;

            /* TODO: obfuscation_string is probably too broad once/if we get rid of the strict sequencing of transformations */
            BBLObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<BBLObfuscationTransformation>(obfuscation_string.c_str(), rng);
          executed_transformations.insert(obfuscator);
            ASSERT(obfuscator, ("Did not find any obfuscator for type '%s'", obfuscation_string.c_str()));

            if (obfuscator->canTransform(bbl)) {
              VERBOSE(1, ("Applying '%s' to @eiB", obfuscator->name(), bbl));
              t_function *fun = BBL_FUNCTION(bbl);
              t_address address = BBL_OLD_ADDRESS(bbl);

              bool result = obfuscator->doTransform(bbl, rng);

              if (result) {
                nr_transformed_in_partition++;
                VERBOSE(0, ("success(%d) @F @G", nr_transformed_in_partition, fun, address));
              }

#ifdef DEBUG_OBFUSCATIONS
              if (result)
                i++;
#endif
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

      if (generate_dots)
      {
        t_string initial_dot_path = "final-old-liveness";

        t_string callgraph_dot_file = StringConcat2(initial_dot_path, "/callgraph.dot");
        CfgDrawFunctionGraphsWithHotness (cfg, initial_dot_path);
        CgBuild (cfg);
        CgExport (CFG_CG(cfg), callgraph_dot_file);
        Free(callgraph_dot_file);
      }

      CfgComputeLiveness(cfg, TRIVIAL);
      CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
      CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);
    }
  }

  /* finalize the executed transformations */
  for (auto tf : executed_transformations)
    tf->finalizeAll();

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

  BblFiniCanTransform(cfg);
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

  //CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_before");

  if (ConstantPropagationInit(cfg))
    ConstantPropagation(cfg, CONTEXT_SENSITIVE);
  else
    FATAL(("ERR\n"));

  /* TODO: this is also needed, but why isn't the above enough? */
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_BBL_FIRST(fun) && !FUNCTION_IS_HELL(fun) && BBL_PRED_FIRST(FUNCTION_BBL_FIRST(fun))) {
      FunctionUnmarkAllBbls(fun);
      /* JENS: this should not be needed (see AF code in bookkeeping) */
      //FunctionPropagateConstantsAfterIterativeSolution(fun,CONTEXT_SENSITIVE);
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


      if (obfuscator->canTransform(bbl)
#ifdef DEBUG_OBFUSCATIONS
          && i < DEBUG_OBFUSCATIONS
#endif
          ) {
        if (RNGGeneratePercent(rng_apply_chance) <= static_cast<t_uint32>(obfuscator->CmdlineChance())) {
          fun = BBL_FUNCTION(bbl);

          VERBOSE(0, ("Transforming %d a BBL of function %s with obfuscator %s, chance %d", i, FUNCTION_NAME(fun), obfuscator->name(), obfuscator->CmdlineChance()));

#ifdef DEBUG_OBFUSCATIONS
          if (i+1 == DEBUG_OBFUSCATIONS)
            CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_before");
#endif

          //FunctionDrawGraph(fun, "before.dot");
          obfuscator->doTransform(bbl, rng_bbl);
          //FunctionDrawGraph(fun, "after.dot");

#ifdef DEBUG_OBFUSCATIONS
          if (i+1 == DEBUG_OBFUSCATIONS)
            CfgDrawFunctionGraphs(OBJECT_CFG(obj), "./dots_after");
#endif

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

  FreeConstantInformation (cfg);
  ConstantPropagationFini(cfg);
}
