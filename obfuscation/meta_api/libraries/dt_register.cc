#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (register)] "

static inline
t_reg value(AbstractValue::Contents contents) {
  return contents.uint32;
}

static inline
AbstractValue::Contents contents(t_reg x) {
  AbstractValue::Contents result;
  result.type = AbstractValue::Type::Register;
  result.uint32 = static_cast<t_uint32>(x);
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
    t_reg A = value(a->assigned);

    ASSERT(b->have_assigned, (META_API_PREFIX "no value '%s'", b->Print().c_str()));
    t_reg B = value(b->assigned);

    if (A < B)
      return AbstractValue::CompareResult::Less;
    else if (A > B)
      return AbstractValue::CompareResult::Greater;

    /* A == B */
    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *x) {
    string result = "Register/";

    if (x->have_assigned)
      result += to_string(value(x->assigned));
    else
      result += "<no value>";

    return result;
  },
  nullptr
};

struct MetaAPI_Datatype_Register : MetaAPI_Datatype {
  MetaAPI_Datatype_Register(string name) : MetaAPI_Datatype(name) {}

  AbstractValue AbstractValueInstance() {
    AbstractValue x = AbstractValue();
    x.type = AbstractValue::Type::Register;
    x.interface = default_interface;
    return x;
  }

  AbstractValue FromString(t_cfg *cfg, string str) {
    AbstractValue x = AbstractValueInstance();
    x.AddGeneratorArgument(contents(stoi(str)));

    x.interface.possible_values = [] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
      AbstractValue::PossibleValueList result;

      set<t_reg> exclude;
      for (auto data : relations) {
        MetaAPI_Relation::Type relation = data.first;
        vector<MetaAPI_Variable *> variables = data.second;

        switch (relation) {
        case MetaAPI_Relation::Type::Ne: {
          for (auto i : variables) {
            if (i->abstract_value.type != AbstractValue::Type::Register)
              continue;

            t_reg r = value(i->abstract_value.assigned);
            exclude.insert(r);
          }
        } break;

        default:
          FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
        }
      }

      t_reg r = value(v->generator_arguments[0]);
      if (exclude.find(r) == exclude.end())
        PossibleValueListAdd(result, v->generator_arguments[0]);

      return result;
    };

    return x;
  }

  AbstractValue FromExpression(t_cfg *cfg, MetaAPI_Expression expr) {
    AbstractValue result = AbstractValueInstance();

    if (expr.command == "RANDOM") {
      result.interface.generator = [] (AbstractValue *value, RelationToOtherVariables relations) {
        AbstractValue::Contents result;

        t_regset possible_regs;
        DiabloBrokerCall("MetaAPI_GetRandomRegisters", &possible_regs);

        vector<t_reg> possible_reg_vector;
        t_reg r;
        REGSET_FOREACH_REG(possible_regs, r)
          possible_reg_vector.push_back(r);

        ASSERT(possible_reg_vector.size() > 0, ("need at least one register"));
        auto it = possible_reg_vector.begin();
        if (possible_reg_vector.size() > 1)
          advance(it, RNGGenerateWithRange(meta_api_value_rng, 0, possible_reg_vector.size() - 1));
        result.uint32 = *it;
        return result;
      };

      result.interface.possible_values = [] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
        AbstractValue::PossibleValueList result;

        t_regset possible_regs;
        DiabloBrokerCall("MetaAPI_GetRandomRegisters", &possible_regs);

        set<t_reg> possible_reg_vector;
        t_reg r;
        REGSET_FOREACH_REG(possible_regs, r)
          possible_reg_vector.insert(r);

        for (auto data : relations) {
          MetaAPI_Relation::Type relation = data.first;
          vector<MetaAPI_Variable *> variables = data.second;

          switch (relation) {
          case MetaAPI_Relation::Type::Ne: {
            for (auto i : variables) {
              if (i->abstract_value.type != AbstractValue::Type::Register)
                continue;

              t_reg r = value(i->abstract_value.assigned);
              possible_reg_vector.erase(r);
            }
          } break;

          default:
            FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
          }
        }

        for (t_reg r : possible_reg_vector)
          PossibleValueListAdd(result, contents(r));

        return result;
      };
    }
    else {
      FATAL((META_API_PREFIX "unknown expression: ", expr.Print().c_str()));
    }

    return result;
  }

  string Print(AbstractValue::Contents contents) {
    return to_string(value(contents));
  }
};

DEFINE_REGISTER_DATATYPE_FN(DtRegister)
{
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "implementation dt_register for '%s'", name.c_str()));
  return new MetaAPI_Datatype_Register(name);
}
