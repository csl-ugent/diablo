#include "meta_api.h"
#include <rapidxml-1.13/rapidxml.hpp>
#include <rapidxml-1.13/rapidxml_print.hpp>
#include <rapidxml-1.13/rapidxml_utils.hpp>

using namespace std;

map<string, MetaAPI_Function *> meta_api_functions;
map<string, MetaAPI_Datatype *> registered_datatypes;
static map<string, MetaAPI_Predicate *> predicate_by_name;
static map<string, MetaAPI_Test> tests_by_name;
static map<string, string> configuration;

map<string, MetaAPI_Variable *> global_variables;
ConstraintList global_constraints;

static
string xml_to_string(rapidxml::xml_node<> *node) {
  string all;
  rapidxml::print(back_inserter(all), *node, 0);
  return all;
}

static
bool string_to_bool(string s) {
  if (s == "true")
    return true;
  else if (s == "false")
    return false;
  else
    FATAL(("invalid boolean string '%s'", s.c_str()));
}

string MetaAPI_FunctionParameter::Print() {
  stringstream result;

  result << type->Print();
  result << " " << identifier;
  result << "=" << value.Print();

  return result.str().c_str();
}

string MetaAPI_Function::Print() {
  stringstream result;

  result << return_type->Print();
  result << " " << identifier;
  result << "(";
  for (auto p : parameters)
    result << p->Print() << ", ";
  result << ")";

  return result.str();
}

string MetaAPI_Datatype::Print() {
  stringstream result;

  result << name << "[class? " << is_class << "]";
  if (! aliases_to.empty())
    result << "=" << aliases_to;

  return result.str();
}

MetaAPI_Datatype *MetaAPI_GetDatatypePtr(string name) {
  auto it = registered_datatypes.find(name);

  /* pointer */
  if (it == registered_datatypes.end()) {
    if (!name.compare(name.size() - 1, 1, "*")
        || !name.compare(name.size() - 1, 1, "&"))
      return MetaAPI_GetDatatypePtr(trim(name.substr(0, name.size() - 1)));

    if (!name.compare(0, strlen("const"), "const"))
      return MetaAPI_GetDatatypePtr(trim(name.substr(strlen("const"))));
  }

  ASSERT(it != registered_datatypes.end(), ("error: datatype '%s' not defined", name.c_str()));

  /* alias */
  MetaAPI_Datatype *result = (*it).second;
  if (! result->aliases_to.empty()) {
    /* an alias exists! */
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "'%s' aliases to '%s'", result->name.c_str(), result->aliases_to.c_str()));
    return MetaAPI_GetDatatypePtr(result->aliases_to);
  }

  return result;
}

static inline
string get_attr(rapidxml::xml_node<> *node, string name, bool& exists) {
  auto x = node->first_attribute(name.c_str());
  if (x) {
    exists = true;
    return x->value();
  }

  exists = false;
  return "";
}

static inline
string get_attr(rapidxml::xml_node<> *node, string name, string default_value) {
  bool exists;
  string x = get_attr(node, name, exists);
  if (!exists)
    x = default_value;
  return x;
}

static inline
string get_attr(rapidxml::xml_node<> *node, string name) {
  bool exists = false;
  string result = get_attr(node, name, exists);
  ASSERT(exists, ("required attribute '%s' not found in node '%s'", name.c_str(), xml_to_string(node).c_str()));
  return result;
}

void parse_implemented_parameter(MetaAPI_Datatype *datatype, string identifier) {
  MetaAPI_FunctionParameter *parameter = new MetaAPI_FunctionParameter();

  parameter->type = datatype;
  parameter->identifier = identifier;
}

static
void parse_parameter(t_cfg *cfg, MetaAPI_Function *function, rapidxml::xml_node<> *def) {
  MetaAPI_FunctionParameter *parameter = new MetaAPI_FunctionParameter();

  /* datatype */
  parameter->type = MetaAPI_GetDatatypePtr(get_attr(def, "datatype"));

  /* name */
  parameter->identifier = get_attr(def, "name");

  /* fixed value */
  bool valueX;
  string value = get_attr(def, "value", valueX);
  if (valueX) {
    if (value == "[instance]") {
      /* pointer to current instance of datastructure */
      parameter->value = AbstractValue::CreateInstance();
    }
    else {
      parameter->value = parameter->type->FromString(cfg, value);
    }
  }
  else {
    /* value not set */
    parameter->value = AbstractValue::CreateNull();
  }

  parameter->variable = NULL;

  parameter->function = function;
  function->parameters.push_back(parameter);
}

static
MetaAPI_Constraint *parse_constraint(rapidxml::xml_node<> *def) {
  VERBOSE(meta_api_verbosity, ("parsing constraint '%s'", xml_to_string(def).c_str()));
  MetaAPI_Constraint *result = new MetaAPI_Constraint(def->value());
  return result;
}

static
void parse_effect(MetaAPI_Function *function, rapidxml::xml_node<> *def) {
  MetaAPI_SetterConfiguration setter_conf;

  auto expression_node = def->first_node("expression");
  ASSERT(expression_node, ("expected expression node"));
  setter_conf.effect = new MetaAPI_Effect(expression_node->value());

  auto constraints_node = def->first_node("constraint");
  if (constraints_node)
    setter_conf.constraint = parse_constraint(constraints_node);
  else
    setter_conf.constraint = NULL;

  function->setter_confs.push_back(setter_conf);
}

static
MetaAPI_Function *parse_function(t_cfg *cfg, rapidxml::xml_node<> *def, MetaAPI_Datatype *datatype) {
  MetaAPI_Function *function = new MetaAPI_Function();
  function->on_datatype = datatype;

  /* list of functions to add this instance to */
  string type = get_attr(def, "type");

  function->enabled = string_to_bool(get_attr(def, "enabled", "true"));
  function->inlined = string_to_bool(get_attr(def, "inline", "false"));

  bool external_present = false;
  get_attr(def, "can_be_external", external_present);
  function->can_be_external = external_present;

  if (type == "constructor")
    function->type = MetaAPI_Function::Type::Constructor;
  else if (type == "implementedconstructor") {
    function->type = MetaAPI_Function::Type::Constructor;
    function->flags |= MetaAPI_Function::Flag_Implemented;
  }
  else if (type == "transformer")
    function->type = MetaAPI_Function::Type::Transformer;
  else if (type == "inlinedtransformer")
    function->type = MetaAPI_Function::Type::InlinedTransformer;
  else if (type == "getter")
    function->type = MetaAPI_Function::Type::Getter;
  else if (type == "inlinedgetter")
    function->type = MetaAPI_Function::Type::InlinedGetter;
  else if (type == "interface")
    function->type = MetaAPI_Function::Type::Interface;
  else
    FATAL(("error: unsupported function type '%s'", type.c_str()));

  /* return type */
  string returns = get_attr(def, "returntype", "void");
  ASSERT(!returns.empty(), ("can't define empty return datatype %s", xml_to_string(def).c_str()));
  function->return_type = MetaAPI_GetDatatypePtr(returns);

  /* identifier */
  function->identifier = get_attr(def, "identifier");
  function->embedded_identifier = NULL;

  /* Diablo function instance */
  function->function = GetFunctionByName(cfg, function->identifier.c_str());

  if (function->type == MetaAPI_Function::Type::Transformer) {
    /* the function must exist, as a regular call will be injected */
    ASSERT(function->function, ("can't find function '%s' needed by '%s'", function->identifier.c_str(), xml_to_string(def).c_str()));
  }
  else if ((function->type == MetaAPI_Function::Type::InlinedTransformer)
            || (function->type == MetaAPI_Function::Type::InlinedGetter)) {
    /* the function should not exist */
    ASSERT(!function->function, ("function '%s' must not exist", function->identifier.c_str()));
  }
  else if (function->flags & MetaAPI_Function::Flag_Implemented) {
    ASSERT(function->function, ("can't find implemented function '%s' needed by '%s'", function->identifier.c_str(), xml_to_string(def).c_str()));
  }

  /* add a special 'this' parameter if this function is a class method
   * and if it is defined as a member method */
  bool member_present = false;
  string _is_member = get_attr(def, "is_member", member_present);
  bool is_member = member_present ? string_to_bool(_is_member) : datatype->is_class;
  if (is_member) {
    /* add implicit 'this' pointer */
    MetaAPI_FunctionParameter *this_parameter = new MetaAPI_FunctionParameter();
    this_parameter->type = datatype;
    this_parameter->identifier = "this";
    this_parameter->value = AbstractValue::CreateInstance();
    this_parameter->variable = NULL;
    this_parameter->function = function;

    function->parameters.push_back(this_parameter);
  }

  /* parameter list */
  for (auto node = def->first_node("parameter"); node; node = node->next_sibling("parameter"))
    parse_parameter(cfg, function, node);

  /* effects, but only for transformers */
  auto effects_node = def->first_node("effects");
  if (effects_node) {
    ASSERT((function->type == MetaAPI_Function::Type::Transformer)
            || (function->type == MetaAPI_Function::Type::InlinedTransformer), ("effects should only be defined for transformer functions"));

    for (auto node = effects_node->first_node("effect"); node; node = node->next_sibling("effect"))
      parse_effect(function, node);
  }

  /* implementation, but only for inlinedtransformers */
  auto implementation_node = def->first_node("implementation");
  if (implementation_node) {
    ASSERT((function->type == MetaAPI_Function::Type::InlinedTransformer)
            || (function->type == MetaAPI_Function::Type::InlinedGetter), ("implementation should only be defined for inlinedtransformer functions"));

    function->implementation_str = implementation_node->value();
  }
  function->implementation_parsed = false;

  ASSERT(meta_api_functions.find(function->identifier) == meta_api_functions.end(), ("function '%s' already defined?", function->identifier.c_str()));
  meta_api_functions[function->identifier] = function;

  return function;
}

static
void parse_value(t_cfg *cfg, MetaAPI_Datatype *datatype, rapidxml::xml_node<> *def) {
  MetaAPI_Value new_value;

  string type = get_attr(def, "type");
  new_value.type = MetaAPI_GetDatatypePtr(type);

  bool valueX;
  string value = get_attr(def, "value", valueX);

  bool expressionX;
  string expression = get_attr(def, "expression", expressionX);

  /* sanity check */
  ASSERT(!(valueX && expressionX), ("can't have value and expression '%s'", xml_to_string(def).c_str()));

  if (valueX) {
    /* value */
    new_value.value = new_value.type->FromString(cfg, value);
  }
  else if (expressionX) {
    /* expression */
    new_value.value = new_value.type->FromExpression(cfg, expression);
  }
  else {
    new_value.value = AbstractValue::CreateProxy();
  }

  for (auto attr = def->first_attribute(); attr; attr = attr->next_attribute()) {
    string name = string(attr->name());
    if (name != "type"
        && name != "value"
        && name != "expression") {
      new_value.properties[name] = string(attr->value());
    }
  }

  datatype->values.push_back(new_value);
}

static
void parse_predicate(t_cfg *cfg, MetaAPI_Datatype *datatype, rapidxml::xml_node<> *def) {
  MetaAPI_Predicate *predicate = new MetaAPI_Predicate();

  predicate->name = get_attr(def, "name");

  string type = get_attr(def, "type", "default");
  if (type == "default")
    predicate->type = MetaAPI_Predicate::Type::Default;
  else if (type == "invariant")
    predicate->type = MetaAPI_Predicate::Type::Invariant;
  else
    FATAL(("error: unsupported predicate type '%s'", type.c_str()));

  predicate->enabled = string_to_bool(get_attr(def, "enabled", "true"));

  if (predicate->type == MetaAPI_Predicate::Type::Invariant) {
    string value = get_attr(def, "value");
    predicate->invariant_value = MetaAPI_Effect::EffectFromString(value);
    ASSERT(MetaAPI_Effect::EffectIsBoolean(predicate->invariant_value), ("expected boolean value for predicate %s", predicate->name.c_str()));
  }

  for (auto node = def->first_node("function"); node; node = node->next_sibling("function")) {
    MetaAPI_Function *function = parse_function(cfg, node, datatype);

    /* add the function to the correct list */
    switch(function->type) {
    case MetaAPI_Function::Type::Getter:
    case MetaAPI_Function::Type::InlinedGetter:
      if (function->enabled)
        predicate->getters.push_back(function);
      break;
    default:
      FATAL(("unsupported function type '%s'", function->Print().c_str()));
    }
  }

  datatype->predicates.push_back(predicate);

  string global_predicate_name = datatype->name + "_" + predicate->name;
  ASSERT(predicate_by_name.find(global_predicate_name) == predicate_by_name.end(), ("duplicate predicate '%s'", predicate->name.c_str()));
  predicate_by_name[global_predicate_name] = predicate;
}

static
void process_variable_declarations(string impl, function<void(MetaAPI_VariableDeclarationStmt *S)> processor) {
  /* parse the pseudo-code */
  vector<MetaAPI_AbstractStmt *> implementation;
  MetaAPI_ParseImplementationWithGrammar(impl, &implementation);

  /* declare global variables */
  for (auto _S : implementation) {
    switch(_S->type) {
    case MetaAPI_AbstractStmt::Type::VariableDeclaration: {
      /* declare a global variable */
      MetaAPI_VariableDeclarationStmt *S = dynamic_cast<MetaAPI_VariableDeclarationStmt *>(_S);
      processor(S);
    } break;

    default:
      FATAL(("global variables can only declare variables '%s'", impl.c_str()));
    }
  }
}

static
void parse_datatype(t_cfg *cfg, rapidxml::xml_node<> *def) {
  /* look up target data type */
  string name = get_attr(def, "name");
  MetaAPI_Datatype *datatype = MetaAPI_GetDatatypePtr(name);

  datatype->enabled = string_to_bool(get_attr(def, "enabled", "true"));

  datatype->is_class = false;
  get_attr(def, "class", datatype->is_class);

  bool have_size = false;
  string _size = get_attr(def, "size", have_size);
  datatype->size = have_size ? stoi(_size) : 0;
  if (datatype->is_class)
    ASSERT(datatype->size != 0, ("need to have positive 'size' attribute for class-based datatype '%s' in definition %s", name.c_str(), xml_to_string(def).c_str()));

  bool have_vtable = false;
  string _vtable = get_attr(def, "vtable", have_vtable);
  if (have_vtable) {
    datatype->vtable_symbol = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(CFG_OBJECT(cfg)), _vtable.c_str());

    if (datatype->enabled)
      ASSERT(datatype->vtable_symbol, ("can't find vtable '%s'", _vtable.c_str()));
  }
  else
    datatype->vtable_symbol = NULL;

  /* members */
  auto members_node = def->first_node("members");
  if (members_node) {
    for (auto node = members_node->first_node("member"); node; node = node->next_sibling("member")) {
      MetaAPI_Datatype::Member m;

      m.name = get_attr(node, "name");
      m.offset = stoi(get_attr(node, "offset"));

      datatype->members.push_back(m);
    }
  }

  /* functions */
  auto functions_node = def->first_node("functions");
  if (functions_node) {
    for (auto node = functions_node->first_node("function"); node; node = node->next_sibling("function")) {
      MetaAPI_Function *function = parse_function(cfg, node, datatype);

      /* add the function to the correct list */
      switch (function->type) {
      case MetaAPI_Function::Type::Constructor:
        datatype->constructors.push_back(function);
        break;
      case MetaAPI_Function::Type::Transformer:
      case MetaAPI_Function::Type::InlinedTransformer:
        datatype->transformers.push_back(function);
        break;
      case MetaAPI_Function::Type::Interface:
        datatype->interface.push_back(function);
        break;
      default:
        FATAL(("unsupported function type '%s'", function->Print().c_str()));
      }
    }
  }

  /* value */
  auto values_node = def->first_node("values");
  if (values_node) {
    for (auto node = values_node->first_node("value"); node; node = node->next_sibling("value")) {
      parse_value(cfg, datatype, node);
    }
  }

  /* predicates */
  auto predicates_node = def->first_node("predicates");
  if (predicates_node) {
    for (auto node = predicates_node->first_node("predicate"); node; node = node->next_sibling("predicate")) {
      parse_predicate(cfg, datatype, node);
    }
  }

  /* instance-specific variables and implementation */
  auto instance_node = def->first_node("instance");
  if (instance_node) {
    MetaAPI_Function *f = new MetaAPI_Function();
    f->type = MetaAPI_Function::Type::InstanceImplementation;
    f->on_datatype = datatype;
    f->return_type = MetaAPI_GetDatatypePtr("void");
    f->identifier = name + "_InstanceImplementation";

    auto implementation_node = instance_node->first_node("implementation");
    ASSERT(implementation_node, ("no implementation node"));
    f->implementation_str = implementation_node->value();
    f->implementation_parsed = false;

    auto instance_variables_node = instance_node->first_node("variables");
    if (instance_variables_node) {
      process_variable_declarations(instance_variables_node->value(), [cfg, datatype] (MetaAPI_VariableDeclarationStmt *S) {
        /* get number of entries in array, if any */
        t_uint32 nr_entries = 1;
        if (S->array_size.type != MetaAPI_ImplementationValue::Type::None)
          ASSERT(S->array_size.GetNumber(nr_entries), ("expected number"));

        MetaAPI_Variable *var = MetaAPI_CreateVariable(cfg, S->datatype, "__INSTANCE__", S->identifier, false, nr_entries);
        ASSERT(datatype->instance_implementation_locals.find(S->identifier) == datatype->instance_implementation_locals.end(), ("duplicate instance variable %s", S->identifier.c_str()));
        datatype->instance_implementation_locals[S->identifier] = var;
      });
    }

    auto instance_constraints_node = instance_node->first_node("constraints");
    if (instance_constraints_node) {
      for (auto node = instance_constraints_node->first_node("constraint"); node; node = node->next_sibling("constraint"))
        f->constraints.push_back(parse_constraint(node));
    }

    datatype->instance_implementation = f;
  }
}

MetaAPI_Variable *MetaAPI_CreateVariable(t_cfg *cfg, string datatype, string function, string identifier, bool create_section, size_t nr_entries) {
  MetaAPI_Variable *result;

  /* construct section name */
  if (create_section) {
    string section_name = ".data.MetaAPIvar";
    section_name += "$" + datatype;
    section_name += "$" + function;
    section_name += "$" + identifier;

    t_object *obj = CFG_OBJECT(cfg);
    t_section *section = SectionCreateForObject(ObjectGetLinkerSubObject(obj), DATA_SECTION, SectionGetFromObjectByName(obj, ".data"), AddressNew32(4 * nr_entries), section_name.c_str());
    SECTION_SET_ALIGNMENT(section, 4);

    result = new MetaAPI_Variable(section);
  }
  else {
    result = new MetaAPI_Variable(MetaAPI_GetDatatypePtr(datatype), identifier);
  }

  result->array_size = nr_entries;

  return result;
}

static
void parse_variable(t_cfg *cfg, rapidxml::xml_node<> *def) {
  string identifier = get_attr(def, "identifier");
  MetaAPI_Variable *variable = MetaAPI_CreateVariable(cfg, get_attr(def, "datatype"), "__GLOBAL__", identifier, true, 1);
  global_variables[identifier] = variable;
}

void MetaAPI_parseXML(t_cfg *cfg, t_const_string filename) {
  /* sanity check and open the XML file */
  ASSERT(filename, ("use -aop_xml to specify an XML file"));
  VERBOSE(0, (META_API_PREFIX "parsing XML file '%s'", filename));
  rapidxml::file<> xml_file(filename);

  /* parse the file contents with the default RapidXML configuration ('0') */
  rapidxml::xml_document<> doc;
  doc.parse<0>(xml_file.data());

  /* the root node */
  auto *root_node = doc.first_node();

  /* the obfuscator configuration parameters */
  auto configuration_node = root_node->first_node("configuration");
  if (configuration_node) {
    for (auto node = configuration_node->first_node("parameter"); node; node = node->next_sibling("parameter")) {
      string name = get_attr(node, "name");
      string value = get_attr(node, "value");

      configuration[name] = value;
    }
  }

  /* the defined data types */
  auto datatypes_node = root_node->first_node("datatypes");
  ASSERT(datatypes_node, ("no datatypes defined in '%s'", filename));
  for (auto node = datatypes_node->first_node("datatype"); node; node = node->next_sibling("datatype")) {
    string name = get_attr(node, "name");

    /* implementation */
    bool libraryX;
    string library = get_attr(node, "library", libraryX);

    MetaAPI_Datatype *x;
    if (libraryX) {
      /* load the library and register the datatype */
      VERBOSE(meta_api_verbosity, (META_API_PREFIX "loading datatype implementation for '%s' from external library '%s'", name.c_str(), library.c_str()));

      t_register_datatype_fn fnRegister;
#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
      fnRegister = LoadSharedFunction<t_register_datatype_fn>(library, DYNSYM_REGISTER_DATATYPE);
#else

#define load(x) \
if (library == "maDatatype" #x) { \
  DEFINE_REGISTER_DATATYPE_FN(Dt ## x); \
  fnRegister = REGISTER_DATATYPE_NAME(Dt ## x); \
}
      load(Integer)
      else load(String)
      else load(Register)
      else load(FunctionPointer)
      else load(Void)
      else FATAL(("unsupported library %s", library.c_str()));
#endif
      x = fnRegister(name);
    }
    else {
      x = new MetaAPI_Datatype(name);
    }

    bool aliasX;
    string alias = get_attr(node, "alias", aliasX);
    if (aliasX)
      x->aliases_to = alias;

    DEBUG(("registering datatype '%s'", name.c_str()));
    registered_datatypes[name] = x;
  }

  /* the actual details */
  for (auto node = datatypes_node->first_node("datatype"); node; node = node->next_sibling("datatype"))
    parse_datatype(cfg, node);

  /* global variables */
  auto global_variables_node = root_node->first_node("variables");
  if (global_variables_node) {
    process_variable_declarations(global_variables_node->value(), [cfg] (MetaAPI_VariableDeclarationStmt *S) {
      MetaAPI_Variable *var = MetaAPI_CreateVariable(cfg, S->datatype, "__GLOBAL__", S->identifier, true, 1);
      ASSERT(global_variables.find(S->identifier) == global_variables.end(), ("duplicate global variable %s", S->identifier.c_str()));
      global_variables[S->identifier] = var;
    });
  }

  /* global constraints */
  auto global_constraints_node = root_node->first_node("constraints");
  if (global_constraints_node) {
    for (auto node = global_constraints_node->first_node("constraint"); node; node = node->next_sibling("constraint"))
      global_constraints.push_back(parse_constraint(node));
  }

  auto tests_node = root_node->first_node("tests");
  if (tests_node) {
    for (auto node = tests_node->first_node("test"); node; node = node->next_sibling("test")) {
      string name = get_attr(node, "name");

      MetaAPI_Test test_configuration = MetaAPI_Test();
      test_configuration.datatype = MetaAPI_GetDatatypePtr(get_attr(node, "datatype"));

      string predicate_name = test_configuration.datatype->name + "_" + get_attr(node, "predicate");
      test_configuration.predicate = MetaAPI_GetPredicateByName(predicate_name);
      test_configuration.value = MetaAPI_Effect::EffectFromString(get_attr(node, "value"));
      test_configuration.setter_identifier = get_attr(node, "setter");
      test_configuration.setter_configuration = stoi(get_attr(node, "setter_constraint", "0"));
      test_configuration.getter_identifier = get_attr(node, "getter");
      test_configuration.resetter_identifier = get_attr(node, "resetter", "");
      test_configuration.resetter_configuration = stoi(get_attr(node, "resetter_constraint", "0"));
      bool have_resetter_value = false;
      string resetter_value_string = get_attr(node, "resetter_value", have_resetter_value);
      if (have_resetter_value)
        test_configuration.resetter_value = MetaAPI_Effect::EffectFromString(resetter_value_string);
      else
        test_configuration.resetter_value = MetaAPI_Effect::Invert(test_configuration.value);
      test_configuration.function_name = get_attr(node, "function", "main");

      ASSERT(tests_by_name.find(name) == tests_by_name.end(), ("multiple tests with same name! %s", name.c_str()));
      DEBUG(("registering test with name '%s'", name.c_str()));
      tests_by_name[name] = test_configuration;
    }
  }
}

void MetaAPI_ProcessConstraintsAndEffects() {
  /* iterate the effects per function */
  for (auto it : meta_api_functions) {
    MetaAPI_Function *function = it.second;

    for (auto& setter_conf : function->setter_confs) {
      /* the effect itself */
      MetaAPI_ParseEffectWithGrammar(function->on_datatype, setter_conf.effect);

      /* the constraint, if any */
      if (setter_conf.constraint)
        MetaAPI_ParseConstraintWithGrammar(setter_conf.constraint);
    }

    /* iterate over implementations */
    if ((function->type == MetaAPI_Function::Type::InlinedTransformer)
        || (function->type == MetaAPI_Function::Type::InlinedGetter))
      MetaAPI_ParseImplementationWithGrammar(function->implementation_str, &(function->implementation));
  }

  /* iterate over the (global) variable constraints */
  for (auto constraint : global_constraints)
    MetaAPI_ParseConstraintWithGrammar(constraint);

  for (auto name_datatype : registered_datatypes) {
    MetaAPI_Datatype *datatype = name_datatype.second;

    /* only if there is some implementation defined */
    if (datatype->instance_implementation) {
      for (auto constraint : datatype->instance_implementation->constraints)
        MetaAPI_ParseConstraintWithGrammar(constraint);

      MetaAPI_ParseImplementationWithGrammar(datatype->instance_implementation->implementation_str, &(datatype->instance_implementation->implementation));
    }
  }
}

MetaAPI_Function *MetaAPI_GetFunctionByName(std::string name) {
  ASSERT(meta_api_functions.find(name) != meta_api_functions.end(), ("can't find function '%s' in meta-API specification", name.c_str()));

  return meta_api_functions[name];
}

MetaAPI_Predicate *MetaAPI_GetPredicateByName(std::string name) {
  ASSERT(predicate_by_name.find(name) != predicate_by_name.end(), ("can't find predicate '%s'", name.c_str()));
  return predicate_by_name[name];
}

MetaAPI_Variable *MetaAPI_GetGlobalVariableByName(std::string name) {
  ASSERT(global_variables.find(name) != global_variables.end(), ("can't find variable '%s'", name.c_str()));
  return global_variables[name];
}

MetaAPI_Test MetaAPI_GetTestByName(std::string name) {
  ASSERT(tests_by_name.find(name) != tests_by_name.end(), ("can't find test '%s'", name.c_str()));
  return tests_by_name[name];
}

string MetaAPI_Predicate::Print() {
  return name;
}

string MetaAPI_SetterConfiguration::Print() {
  string result = "";

  if (constraint)
    result += constraint->Print();
  result += "=>";
  if (effect)
    result += effect->Print();

  return result;
}

string MetaAPI_GetConfigurationParameter(string name, string default_value) {
  if (configuration.find(name) == configuration.end())
    return default_value;

  return configuration[name];
}
