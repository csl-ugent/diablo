#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (function pointer)] "

static inline
t_function *value(AbstractValue::Contents contents) {
  return contents.function;
}

static inline
AbstractValue::Contents contents(t_function *function) {
  AbstractValue::Contents result;
  result.type = AbstractValue::Type::FunctionPointer;
  result.function = function;
  result.compare = [] (const AbstractValue::Contents *a, const AbstractValue::Contents *b) {
    return FUNCTION_ID(a->function) < FUNCTION_ID(b->function);
  };
  return result;
}

static
AbstractValue::HelperFunctions default_interface = {
  [] (AbstractValue *value, RelationToOtherVariables relations) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "default generator implementation"));
    ASSERT(value->generator_arguments.size() > 0, (META_API_PREFIX "must have at least one argument"));

    return value->generator_arguments[0];
  },
  [] (AbstractValue *a, AbstractValue *b) {
    ASSERT(a->have_assigned, (META_API_PREFIX "no value '%s'", a->Print().c_str()));
    t_function *A = value(a->assigned);

    ASSERT(b->have_assigned, (META_API_PREFIX "no value '%s'", b->Print().c_str()));
    t_function *B = value(b->assigned);

    if (A < B)
      return AbstractValue::CompareResult::Less;
    else if (A > B)
      return AbstractValue::CompareResult::Greater;

    /* A == B */
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *x) {
    string result = "FunctionPointer/";

    if (x->have_assigned) {
      t_string str = StringIo("@F", value(x->assigned));
      result += str;
      Free(str);
    }
    else
      result += "<no value>";

    return result;
  }
};

struct MetaAPI_Datatype_FunctionPointer : MetaAPI_Datatype {
  MetaAPI_Datatype_FunctionPointer(string name) : MetaAPI_Datatype(name) {}

  AbstractValue AbstractValueInstance() {
    AbstractValue x = AbstractValue();
    x.type = AbstractValue::Type::FunctionPointer;
    x.interface = default_interface;
    return x;
  }

  AbstractValue FromString(t_cfg *cfg, string str) {
    AbstractValue x = AbstractValueInstance();

    t_function *function = GetFunctionByName(cfg, str.c_str());
    ASSERT(function != NULL, (META_API_PREFIX "function '%s' not found", str.c_str()));

    x.AddGeneratorArgument(contents(function));

    return x;
  }
};

DEFINE_REGISTER_DATATYPE_FN(DtFunctionPointer) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "implementation dt_functionpointer for '%s'", name.c_str()));
  return new MetaAPI_Datatype_FunctionPointer(name);
}
