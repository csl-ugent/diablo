#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (integer)] "

enum class GenerateType {
  Any,
  Even,
  Odd
};

static inline
t_uint32 value(AbstractValue::Contents contents) {
  return contents.uint32;
}

static inline
AbstractValue::Contents contents(t_uint32 value) {
  AbstractValue::Contents result;
  result.type = AbstractValue::Type::Integer;
  result.uint32 = value;
  result.compare = [] (const AbstractValue::Contents *a, const AbstractValue::Contents *b) {
    return a->uint32 < b->uint32;
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
    t_uint32 A = value(a->assigned);

    ASSERT(b->have_assigned, (META_API_PREFIX "no value '%s'", b->Print().c_str()));
    t_uint32 B = value(b->assigned);


    if (A < B)
      return AbstractValue::CompareResult::Less;
    else if (A > B)
      return AbstractValue::CompareResult::Greater;

    /* A == B */
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *x) {
    string result = "Integer/";

    if (x->have_assigned)
      result += to_string(value(x->assigned));
    else
      result += "<no value>";

    return result;
  },
  [] (AbstractValue *v, RelationToOtherVariables, Properties) {
    AbstractValue::PossibleValueList result;
    PossibleValueListAdd(result, v->generator_arguments[0]);
    return result;
  }
};

struct MetaAPI_Datatype_Integer : MetaAPI_Datatype {
  MetaAPI_Datatype_Integer(string name) : MetaAPI_Datatype(name) {}

  AbstractValue AbstractValueInstance() {
    AbstractValue x = AbstractValue();
    x.type = AbstractValue::Type::Integer;
    x.interface = default_interface;
    return x;
  }

  AbstractValue False(t_cfg *cfg) {
    AbstractValue x = AbstractValueInstance();
    x.AddGeneratorArgument(contents(0));
    return x;
  }

  AbstractValue True(t_cfg *cfg) {
    AbstractValue x = AbstractValueInstance();
    x.AddGeneratorArgument(contents(1));
    return x;
  }

  AbstractValue FromString(t_cfg *cfg, string str) {
    AbstractValue x = AbstractValueInstance();
    x.AddGeneratorArgument(contents(stoi(str)));
    return x;
  }

  AbstractValue FromExpression(t_cfg *cfg, MetaAPI_Expression expr) {
    AbstractValue result = AbstractValueInstance();

    if (expr.command == "RANGE"
        || expr.command == "EVEN"
        || expr.command == "ODD") {
      auto args = expr.SplitArgumentString();
      result.AddGeneratorArgument(contents(stoi(args[0])));
      result.AddGeneratorArgument(contents(stoi(args[1])));

      GenerateType t = GenerateType::Any;
      if (expr.command == "EVEN")
        t = GenerateType::Even;
      else if (expr.command == "ODD")
        t = GenerateType::Odd;
      result.AddGeneratorArgument(contents(static_cast<t_uint32>(t)));

      result.interface.generator = [] (AbstractValue *v, RelationToOtherVariables relations) {
        /* generator arguments */
        t_uint32 lower = value(v->generator_arguments[0]);
        t_uint32 upper = value(v->generator_arguments[1]);
        GenerateType type = static_cast<GenerateType>(value(v->generator_arguments[2]));

        /* list of possible values */
        // implicit: generate random value in range

        /* pick one */
        // implicit: pick random value

        /* return result */
        t_uint32 value = RNGGenerateWithRange(meta_api_value_rng, lower, upper);

        switch (type) {
        case GenerateType::Even:
          value &= ~0x1;
          if (value < lower) {
            value += 2;
            ASSERT(value <= upper, ("out of range [%d,%d] %d", lower, upper, value));
          }
          break;
        case GenerateType::Odd:
          value |= 0x1;
          if (value > upper) {
            value -= 2;
            ASSERT(value >= lower, ("out of range [%d,%d] %d", lower, upper, value));
          }
          break;
        default:
          /* do nothing */;
        }

        return contents(value);
      };

      result.interface.possible_values = [] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
        t_uint32 lower = value(v->generator_arguments[0]);
        t_uint32 upper = value(v->generator_arguments[1]);
        GenerateType type = static_cast<GenerateType>(value(v->generator_arguments[2]));

        AbstractValue::PossibleValueList result;

        set<t_uint32> exclude;
        bool have_upper_limit = false;
        t_uint32 upper_limit_incl;
        for (auto data : relations) {
          MetaAPI_Relation::Type relation = data.first;
          vector<MetaAPI_Variable *> variables = data.second;

          switch (relation) {
          case MetaAPI_Relation::Type::Ne: {
            /* exclude these values */
            for (auto i : variables) {
              if (i->abstract_value.type != AbstractValue::Type::Integer)
                continue;

              t_uint32 x = value(i->abstract_value.assigned);
              exclude.insert(x);
            }
          } break;

          case MetaAPI_Relation::Type::Lt: {
            for (auto i : variables) {
              if (i->abstract_value.type != AbstractValue::Type::Integer)
                continue;
              
              t_uint32 x = value(i->abstract_value.assigned);
              if (!have_upper_limit) {
                upper_limit_incl = x-1;
              }
              else {
                if (x < upper_limit_incl)
                  upper_limit_incl = x-1;
              }

              have_upper_limit = true;
            }
          } break;

          default:
            FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
          }
        }

        /* iterate over the values within range */
        for (t_uint32 i = lower; i <= upper; i++) {
          if (exclude.find(i) != exclude.end())
            continue;
          if (have_upper_limit && (upper_limit_incl < i))
            continue;
          
          if ((type == GenerateType::Even)
              && (i & 0x1))
            continue;
          if ((type == GenerateType::Odd)
              && !(i & 0x1))
            continue;

          PossibleValueListAdd(result, contents(i));
        }

        return result;
      };
    }
    else if (expr.command == "LIBRARY") {
      /* arguments = "library, function, ARGS" */
      auto first_comma = expr.arguments.find_first_of(",");
      string library_name = trim(expr.arguments.substr(0, first_comma));
      auto second_comma = expr.arguments.find_first_of(",", first_comma+1);
      string function_name = trim(expr.arguments.substr(first_comma+1, (second_comma-1) - first_comma));
      string function_arguments = trim(expr.arguments.substr(second_comma+1));

      t_custom_function func = LoadSharedFunction<t_custom_function>(library_name, function_name);
      func();
      FATAL(("library call: >%s::%s(%s)<", library_name.c_str(), function_name.c_str(), function_arguments.c_str()));
    }
    else {
      FATAL((META_API_PREFIX "unknown expression: %s", expr.Print().c_str()));
    }

    return result;
  }

  string Print(AbstractValue::Contents contents) {
    return to_string(value(contents));
  }
};

DEFINE_REGISTER_DATATYPE_FN(DtInteger) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "implementation dt_integer for '%s'", name.c_str()));
  return new MetaAPI_Datatype_Integer(name);;
}
