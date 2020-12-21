#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (void)] "

static inline
AbstractValue::Contents contents(void *value) {
  AbstractValue::Contents result;
  result.type = AbstractValue::Type::Void;
  ASSERT(value == NULL, ("unsupported"));
  result.generic = value;
  return  result;
}

static
AbstractValue::HelperFunctions default_interface = {
  /* generator */
  [] (AbstractValue *value, RelationToOtherVariables relations) {
    VERBOSE(meta_api_verbosity, (META_API_PREFIX "default generator implementation"));
    ASSERT(value->generator_arguments.size() > 0, (META_API_PREFIX "must have at least one argument"));

    return value->generator_arguments[0];
  },

  /* compare */
  nullptr,

  /* print */
  [] (AbstractValue *value) {
    return "Void";
  },

  /* possible_values */
  nullptr
};

struct MetaAPI_Datatype_Void : MetaAPI_Datatype {
  MetaAPI_Datatype_Void(string name) : MetaAPI_Datatype(name) {}

  AbstractValue AbstractValueInstance() {
    AbstractValue x = AbstractValue();
    x.type = AbstractValue::Type::Void;
    x.interface = default_interface;
    return x;
  }

  AbstractValue FromString(t_cfg *cfg, string str) {
    AbstractValue x = AbstractValueInstance();
    ASSERT(str == "NULL", ("unsupported"));
    x.AddGeneratorArgument(contents(NULL));
    return x;
  }
};

DEFINE_REGISTER_DATATYPE_FN(DtVoid) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "implementation dt_void for '%s'", name.c_str()));
  return new MetaAPI_Datatype_Void(name);
}
