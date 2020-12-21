#include "meta_api.h"

#include <obfuscation/obfuscation_opt.h>

using namespace std;

/* dynamic members */
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(used_predicates);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(defined_predicates);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(live_predicates);

FUNCTION_DYNAMIC_MEMBER_GLOBAL_ARRAY(metaapi_data);

EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(metaapi_corr_backward);
EDGE_DYNAMIC_MEMBER_GLOBAL_ARRAY(metaapi_corr_foreward);

/* random number generators */
t_randomnumbergenerator *meta_api_rng;
t_randomnumbergenerator *meta_api_getter_rng;
t_randomnumbergenerator *meta_api_setter_rng;
t_randomnumbergenerator *meta_api_effect_rng;
t_randomnumbergenerator *meta_api_string_rng;
t_randomnumbergenerator *meta_api_value_rng;
t_randomnumbergenerator *meta_api_instruction_rng;
t_randomnumbergenerator *meta_api_liveness_rng;

FunctionSet implementing_functions;

static bool is_init = false;
t_uint32 meta_api_verbosity;
bool meta_api_debug;
t_int32 meta_api_max_setter_count;
bool meta_api_max_setter_set;
bool meta_api_verify_setters;
bool meta_api_enable_two_way;
bool meta_api_restrict_push_pop;
int meta_api_instance_count;
int meta_api_setters_per_getter;
bool meta_api_impl_inst;
bool meta_api_impl_set;
bool meta_api_impl_reset;
bool meta_api_impl_get;
string meta_api_project_name;

bool meta_api_measuring;
string meta_api_measure_init_address;
string meta_api_measure_loc1_address;
string meta_api_measure_loc2_address;
string meta_api_measure_loc3_address;
t_function *meta_api_init_function = NULL;

static
string BblPredicateInformation(t_bbl *bbl) {
  string ret = "";

  if (BBL_LIVE_PREDICATES(bbl)) {
    auto x = BBL_LIVE_PREDICATES(bbl);
    if (x->any()) {
      ret += "LIVE(";

      MetaAPI_ForEachPredicateIn(x, [&ret] (MetaAPI_ActivePredicate *pred) {
        ret += "P" + to_string(pred->uid) + ",";
      });

      ret += ") ";
    }
  }

  if (BBL_USED_PREDICATES(bbl)) {
    auto x = BBL_USED_PREDICATES(bbl);
    if (x->any()) {
      ret += "USE(";

      MetaAPI_ForEachPredicateIn(x, [&ret] (MetaAPI_ActivePredicate *pred) {
        ret += "P" + to_string(pred->uid) + ",";
      });

      ret += ") ";
    }
  }

  if (BBL_DEFINED_PREDICATES(bbl)) {
    auto x = BBL_DEFINED_PREDICATES(bbl);
    if (x->any()) {
      ret += "DEF(";

      MetaAPI_ForEachPredicateIn(x, [&ret] (MetaAPI_ActivePredicate *pred) {
        ret += "P" + to_string(pred->uid) + ",";
      });

      ret += ") ";
    }
  }

  if (BblIsInCloud(bbl))
    ret += " cloud";

  return ret;
}

static
void IoBblModifierBroker(t_bbl *bbl, t_string_array *array) {
  if (!is_init)
    return;

  string s = BblPredicateInformation(bbl);

  if (!s.empty())
    StringArrayAppendString(array, StringConcat2(s.c_str(), "\\l"));
}

static
void FunctionDrawGraphSpecialFunctionCall(t_cfg_edge *e, void **target) {
  /* look at the call edge */
  if (CFG_EDGE_CAT(e) == ET_RETURN)
    e = CFG_EDGE_CORR(e);

  *target = reinterpret_cast<void *>(CFG_EDGE_TAIL(e));
}

void MetaAPI_Init(t_cfg *cfg, t_randomnumbergenerator *rng, t_const_string xml_file) {
  ASSERT(!is_init, (META_API_PREFIX "already initialised!"));
  is_init = true;

  DiabloBrokerCall("MetaAPI_GetCompareRegisters", &condition_registers);
  DiabloBrokerCall("MetaAPI_CantBeLiveRegisters", &cant_be_live_registers);

  /* instantiate RNGs */
  meta_api_rng = RNGCreateChild(rng, "meta_api");
  meta_api_getter_rng = RNGCreateChild(meta_api_rng, "getter");
  meta_api_setter_rng = RNGCreateChild(meta_api_rng, "setter");
  meta_api_effect_rng = RNGCreateChild(meta_api_rng, "effect");
  meta_api_string_rng = RNGCreateChild(meta_api_rng, "string");
  meta_api_value_rng = RNGCreateChild(meta_api_rng, "value");
  meta_api_instruction_rng = RNGCreateChild(meta_api_rng, "instruction");
  meta_api_liveness_rng = RNGCreateChild(meta_api_rng, "liveness");

  /* parse input files */
  MetaAPI_parseXML(cfg, xml_file);

  /* install broker calls */
  DiabloBrokerCallInstall("AOPAfterSplit", "t_bbl *, t_bbl *", (void *)MetaAPI_AfterBblSplit, FALSE);
  DiabloBrokerCallInstall("AOPSpecialFunctionAddress", "t_cfg_edge *e, void **target", (void *)FunctionDrawGraphSpecialFunctionCall, FALSE);
  DiabloBrokerCallInstall("IoModifierBblMetaAPI", "t_bbl *, t_string_array *", (void *)IoBblModifierBroker, FALSE);

  /* dynamic members */
  BblInitUsedPredicates(cfg);
  BblInitDefinedPredicates(cfg);
  BblInitLivePredicates(cfg);

  FunctionInitMetaApiData(cfg);

  CfgEdgeInitMetaApiCorrBackward(cfg);
  CfgEdgeInitMetaApiCorrForeward(cfg);

  /* initialise additional data */
  MetaAPI_FindProgramStrings(cfg);
  vector<MetaAPI_Variable *> variables = MetaAPI_FindVariables(cfg);

  /* now that we know each variable, parse the constraints and effects */
  MetaAPI_ProcessConstraintsAndEffects();

  MetaAPI_LoadVariableConstraints(cfg, global_constraints);
  MetaAPI_AssignVariableValues(cfg, variables, true);

  /* We can't transform these functions because they implement (part of) the functionsality we need
   * for the data structures and predicates to work. */
  implementing_functions = MetaAPI_ImplementingFunctionList(cfg);

  meta_api_verbosity = obfuscation_obfuscation_options.meta_api_verbosity;
  meta_api_debug = obfuscation_obfuscation_options.meta_api_debug;
  meta_api_max_setter_count = obfuscation_obfuscation_options.meta_api_setter_count;
  meta_api_max_setter_set = obfuscation_obfuscation_options.meta_api_setter_count_set;
  meta_api_verify_setters = obfuscation_obfuscation_options.meta_api_verify_setters;
  meta_api_enable_two_way = obfuscation_obfuscation_options.meta_api_enable_twoway;
  meta_api_restrict_push_pop = obfuscation_obfuscation_options.meta_api_restrict_pushpop;
  meta_api_instance_count = obfuscation_obfuscation_options.meta_api_instance_count;
  meta_api_setters_per_getter = obfuscation_obfuscation_options.meta_api_max_setters_per_getter;
  meta_api_impl_inst = obfuscation_obfuscation_options.meta_api_implement_instance;
  meta_api_impl_set = obfuscation_obfuscation_options.meta_api_implement_set;
  meta_api_impl_reset = obfuscation_obfuscation_options.meta_api_implement_reset;
  meta_api_impl_get = obfuscation_obfuscation_options.meta_api_implement_get;
  meta_api_measuring = obfuscation_obfuscation_options.meta_api_measure_loc1_address_set || obfuscation_obfuscation_options.meta_api_measure_loc2_address_set || obfuscation_obfuscation_options.meta_api_measure_loc3_address_set;
  if (meta_api_measuring) {
    meta_api_measure_loc1_address = obfuscation_obfuscation_options.meta_api_measure_loc1_address;
    meta_api_measure_loc2_address = obfuscation_obfuscation_options.meta_api_measure_loc2_address;
    meta_api_measure_loc3_address = obfuscation_obfuscation_options.meta_api_measure_loc3_address;
  }
  ASSERT(meta_api_instance_count >= 1, ("--meta-api-number-instances should at least be 1, but got %d", meta_api_instance_count));

  /* sanity check: the initialisation function, if any, should be the very last entry in the .init_array section */
  t_section *init_array_section = NULL;
  t_section *sec;
  int tel;
  OBJECT_FOREACH_SECTION(CFG_OBJECT(cfg), sec, tel) {
    if (!strncmp(SECTION_NAME(sec), ".init_array", strlen(".init_array"))) {
      ASSERT(!init_array_section, ("duplicate section @T", init_array_section));
      init_array_section = sec;
    }
  }
  ASSERT(init_array_section, ("can't find section .init_array"));

  t_section *init_array_vector = NULL;
  SECTION_FOREACH_SUBSECTION(init_array_section, sec) {
    ASSERT(!init_array_vector, ("duplicate vector @T", sec));
    init_array_vector = sec;
  }

  string meta_api_init_function_name = META_API_LINKIN_FUNCTION_NAME(init);
  string conf_meta_api_init_function_name = MetaAPI_GetConfigurationParameter("init_function", "");
  if (!conf_meta_api_init_function_name.empty()) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "overriding default global initialisation function name"));
    meta_api_init_function_name = conf_meta_api_init_function_name;
  }

  meta_api_project_name = "(unknown project)";
  string conf_meta_api_project_name = MetaAPI_GetConfigurationParameter("project_name", "");
  if (!conf_meta_api_project_name.empty()) {
    meta_api_project_name = conf_meta_api_project_name;
  }

  for (t_reloc_ref *rr = SECTION_REFERS_TO(init_array_vector); rr; rr = RELOC_REF_NEXT(rr)) {
    t_reloc *rel = RELOC_REF_RELOC(rr);

    t_relocatable *to = RELOC_TO_RELOCATABLE(rel)[0];
    ASSERT(RELOCATABLE_RELOCATABLE_TYPE(to) == RT_BBL, ("expected relocation to BBL @R", rel));

    t_bbl *bbl = T_BBL(to);
    ASSERT(BBL_FUNCTION(bbl), ("expected to to be in function @eiB @R", bbl, rel));

    if (!strcmp(FUNCTION_NAME(BBL_FUNCTION(bbl)), meta_api_init_function_name.c_str())) {
      t_address last_offset = AddressSubUint32(SECTION_CSIZE(init_array_vector), 4);
      /* disable this fatal for now, need to check in the future whether this should be enabled or not */
      // ASSERT(AddressIsEq(RELOC_FROM_OFFSET(rel), last_offset), ("initialisation function @F (@R) should be the very last entry in @T", BBL_FUNCTION(bbl), rel, init_array_vector));
      meta_api_init_function = BBL_FUNCTION(bbl);
    }
  }

  if (!meta_api_init_function)
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "can't find global initialisation function '%s'", meta_api_init_function_name.c_str()));
  else
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "found global initialisation function '%s'", meta_api_init_function_name.c_str()));
}

void MetaAPI_KeepLive(t_cfg *cfg) {
  t_symbol *sym;
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(CFG_OBJECT(cfg)), sym) {
    ASSERT(SYMBOL_NAME(sym), ("expected symbol name @S", sym));

    /* need the asterisk in front also because of possible C++ name mangling
     * (in which case a '_Z' prefix is always prepended). */
    if (StringPatternMatch("*" META_API_LINKIN_PREFIX_STRING "*", SYMBOL_NAME(sym))) {
      if (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) != RT_BBL)
        continue;

      t_bbl *bbl = T_BBL(SYMBOL_BASE(sym));
      ASSERT(!IS_DATABBL(bbl), ("unexpected @eiB", bbl));

      VERBOSE(meta_api_verbosity, (META_API_PREFIX "force-reachable @S @eiB", sym, bbl));

      BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_FORCE_REACHABLE);

      bool hell_edge = false;
      if (!BBL_PRED_FIRST(bbl))
        hell_edge = true;
      else if (!CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl))
                && (CFG_EDGE_CAT(BBL_PRED_FIRST(bbl)) == ET_RETURN))
        hell_edge = true;

      if (hell_edge)
        CfgEdgeCreateCall(cfg, CFG_HELL_NODE(cfg), bbl, NULL, NULL);
    }
  }
}
