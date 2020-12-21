#ifndef META_API_HELPERS_H
#define META_API_HELPERS_H

struct _MetaAPI_Relation {
  enum class Type {
    Invalid,
    Eq, Ne,
    Gt, Ge,
    Lt, Le,
    Mod,
    InstanceOf
  };
  Type value;

  _MetaAPI_Relation() {
    value = Type::Invalid;
  }
  _MetaAPI_Relation(std::string str);
  std::string Print();
  static std::string Print(Type t);
  static Type Revert(Type t);
};

struct _MetaAPI_Operand {
  enum class Type {
    Invalid,
    Add, Sub
  };
  Type value;

  _MetaAPI_Operand() {
    value = Type::Invalid;
  }

  _MetaAPI_Operand(std::string str);
  std::string Print();
  static std::string Print(Type t);
};

typedef struct {
  t_bbl *before;
  t_bbl *after;
} PreparedCallSite;

typedef struct {
  t_reg reg_lhs;
  t_reg reg_rhs;
  t_int32 imm_rhs;
  MetaAPI_Relation::Type relation;
} MetaAPI_CompareConfiguration;

PreparedCallSite PrepareForCallInsertion(t_bbl *bbl, bool split_after=false, bool always_new_after=true);
void MetaAPI_AfterBblSplit(t_bbl *from, t_bbl *to);

template<typename S>
auto PickRandomElement(S x, t_randomnumbergenerator *rng) -> decltype(*(x.begin())) {
  auto it = x.begin();
  if (x.size() > 0) {
    auto n = RNGGenerateWithRange(rng, 0, x.size() - 1);
    advance(it, n);
  }
  return *it;
}

template<typename T1, typename T2>
auto PickRandomElement(std::map<T1, T2> x, t_randomnumbergenerator *rng) -> decltype(x.begin()->second) {
  auto it = x.begin();
  if (x.size() > 0) {
    auto n = RNGGenerateWithRange(rng, 0, x.size() - 1);
    advance(it, n);
  }
  return it->second;
}

std::string trim(std::string str);
/* returns TRUE when 'end' is empty */
bool ends_with(std::string str, std::string end);
bool contains(std::string str, std::string x);
FunctionSet MetaAPI_ImplementingFunctionList(t_cfg *cfg);

typedef struct {
  MetaAPI_Function *function;
  size_t configuration;
} MetaAPI_Setter;

/* */
struct FunctionMetaApiData {
  /* 'TRUE'=getter, 'FALSE'=setter */
  bool is_getter;
  /* the affected predicate */
  MetaAPI_ActivePredicate *predicate;
  MetaAPI_Function *getter;
  MetaAPI_Setter setter;

  bool is_verifier;
  t_function *verified_setter_function;

  t_regset saved_registers;
  t_ins *push_location;
  t_ins *pop_location;
  bool is_finalized;
  t_regset overwritten_registers;
  bool from_hell;

  FunctionMetaApiData() {
    is_getter = false;
    predicate = NULL;
    is_verifier = false;

    saved_registers = RegsetNew();
    push_location = NULL;
    pop_location = NULL;
    is_finalized = false;

    from_hell = false;
  }
};

/* dynamic members */
FUNCTION_DYNAMIC_MEMBER_GLOBAL_BODY(metaapi_data, METAAPI_DATA, MetaApiData, FunctionMetaApiData *, {*valp = NULL;}, {if (*valp) delete *valp;}, {});
EDGE_DYNAMIC_MEMBER_GLOBAL(metaapi_corr_backward, METAAPI_CORR_BACKWARD, MetaApiCorrBackward, t_cfg_edge *, NULL);
EDGE_DYNAMIC_MEMBER_GLOBAL(metaapi_corr_foreward, METAAPI_CORR_FOREWARD, MetaApiCorrForeward, t_cfg_edge *, NULL);

bool FunctionIsMeta(t_function *fun);
FunctionMetaApiData *FunctionGetMetaApiData(t_function *fun);
t_cfg_edge *LookThroughMetaFunction(t_cfg_edge *e, bool forward, std::function<bool(t_function *)> stop);
bool MetaAPI_CanTransformFunction(t_function *fun);
std::vector<t_ins*> FindPlacesWhereFlagsAreDead(t_bbl* bbl);
bool BblStatusFlagsAreDeadAfter(t_bbl *bbl);
std::vector<MetaAPI_Setter> MetaAPI_FindTransformers(MetaAPI_Datatype *datatype, MetaAPI_Predicate *predicate, MetaAPI_Effect::Effect desired_value, std::map<MetaAPI_Predicate *, MetaAPI_Effect::Effect> dont_change);
void MetaAPI_LinkEdges(t_cfg_edge *a, t_cfg_edge *b);
bool BblReferedToByExtab(t_bbl *bbl);

extern t_regset condition_registers;
extern t_regset cant_be_live_registers;

#endif
