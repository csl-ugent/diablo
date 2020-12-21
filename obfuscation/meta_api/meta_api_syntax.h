#ifndef META_API_SYNTAX_H
#define META_API_SYNTAX_H

/* actual declarations */
struct _MetaAPI_FunctionParameter {
  MetaAPI_Datatype *type;
  std::string identifier;
  /* set to specific value by XML specification */
  AbstractValue value;
  MetaAPI_Variable *variable;
  MetaAPI_Function *function;

  std::string Print();
};

typedef struct {
  MetaAPI_Datatype *type;
  AbstractValue value;
  Properties properties;
} MetaAPI_Value;

struct _MetaAPI_Predicate {
  enum class Type {
    Default,
    Invariant
  };
  Type type;
  bool enabled;
  MetaAPI_Effect::Effect invariant_value;

  std::string name;
  std::vector<MetaAPI_Function *> getters;

  std::string Print();
};
typedef std::set<MetaAPI_Predicate *> PredicateList;

/* describes one setter configuration (Cx are constraints and Ex are effects):
 * V1==V2, V3!=V4 => P1=T, P2=F
 * (C1)    (C2)      (E1)  (E2) */
typedef struct {
  MetaAPI_Constraint *constraint;
  MetaAPI_Effect *effect;

  std::string Print();
} MetaAPI_SetterConfiguration;

struct _MetaAPI_Function {
  enum class Type {
    None,
    Constructor,
    Transformer,
    Getter,
    Interface,
    InlinedTransformer,
    InlinedGetter,
    InstanceImplementation
  };
  Type type;

  static constexpr int Flag_Implemented = 1<<0;
  int flags;

  MetaAPI_Datatype *on_datatype;

  t_function *function;
  MetaAPI_Datatype *return_type;
  std::string identifier;
  std::string implementation_str;

  MetaAPI_String *embedded_identifier;

  std::vector<MetaAPI_FunctionParameter *> parameters;
  std::vector<MetaAPI_SetterConfiguration> setter_confs;

  ConstraintList constraints;
  std::vector<MetaAPI_AbstractStmt *> implementation;
  std::map<std::string, MetaAPI_Variable *> implementation_locals;
  bool implementation_parsed;

  bool enabled;
  bool inlined;

  bool can_be_external;

  std::string Print();

  _MetaAPI_Function() {
    type = Type::None;
    flags = 0;
    on_datatype = NULL;
    function = NULL;
    return_type = NULL;
    implementation_str = "";
    embedded_identifier = NULL;
    implementation_parsed = false;
    enabled = false;
    inlined = false;
    can_be_external = false;
  }
};

struct _MetaAPI_Test {
  MetaAPI_Datatype *datatype;
  MetaAPI_Predicate *predicate;
  std::string getter_identifier;
  std::string setter_identifier;
  size_t setter_configuration;
  std::string resetter_identifier;
  size_t resetter_configuration;
  MetaAPI_Effect::Effect resetter_value;
  MetaAPI_Effect::Effect value;
  std::string function_name;
};
typedef struct _MetaAPI_Test MetaAPI_Test;

MetaAPI_Datatype *MetaAPI_GetDatatypePtr(std::string name);
MetaAPI_Predicate *MetaAPI_GetPredicateByName(std::string name);
MetaAPI_Variable *MetaAPI_GetGlobalVariableByName(std::string name);
void MetaAPI_parseXML(t_cfg *cfg, t_const_string filename);
MetaAPI_Function *MetaAPI_GetFunctionByName(std::string name);
void MetaAPI_ProcessConstraintsAndEffects();
MetaAPI_Variable *MetaAPI_CreateVariable(t_cfg *cfg, std::string datatype, std::string function, std::string identifier, bool create_section, size_t nr_entries);
MetaAPI_Test MetaAPI_GetTestByName(std::string name);
std::string MetaAPI_GetConfigurationParameter(std::string name, std::string default_value);

extern ConstraintList global_constraints;
extern std::map<std::string, MetaAPI_Function *> meta_api_functions;
extern std::map<std::string, MetaAPI_Datatype *> registered_datatypes;
extern std::map<std::string, MetaAPI_Variable *> global_variables;

#endif
