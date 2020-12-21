#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (string)] "

static
bool eq(MetaAPI_String *a, MetaAPI_String *b) {
  return (a->Get() == b->Get());
}

static
int cmp(MetaAPI_String *a, MetaAPI_String *b) {
  return strcmp(a->Get().c_str(), b->Get().c_str());
}

static inline
string substring(string str, size_t len) {
  if (len <= 0)
    return str;

  return str.substr(0, (str.size() > len) ? len : str.size());
}

static
bool eq(MetaAPI_String *a, string *b, t_int32 compare_length = -1) {
  if (compare_length <= 0) {
    return a->Get() == *b;
  }

  return substring(a->Get(), compare_length) == substring(*b, compare_length);
}

static
int cmp(MetaAPI_String *a, string *b, t_int32 compare_length = -1) {
  string A = substring(a->Get(), compare_length);
  string B = substring(*b, compare_length);

  return strcmp(A.c_str(), B.c_str());
}

static inline
MetaAPI_String *value(AbstractValue::Contents contents) {
  return AbstractValue::unpack<MetaAPI_String>(contents.generic);
}

static inline
t_int32 GetCompareLengthProperty(Properties properties) {
  auto x = properties.find("compare_length");
  if (x == properties.end())
    return -1;

  return stoi(x->second);
}

static inline
AbstractValue::Contents contents(MetaAPI_String *str) {
  AbstractValue::Contents result;
  result.type = AbstractValue::Type::String;
  result.generic = AbstractValue::pack(str);
  result.compare = [] (const AbstractValue::Contents *a, const AbstractValue::Contents *b) {
    return AddressIsLt(value(*a)->Address(), value(*b)->Address());
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
    ASSERT(b->have_assigned, (META_API_PREFIX "no value '%s'", b->Print().c_str()));

    int r = cmp(value(a->assigned), value(b->assigned));

    if (r < 0)
      return AbstractValue::CompareResult::Less;
    else if (r > 0)
      return AbstractValue::CompareResult::Greater;

    return AbstractValue::CompareResult::Equal;
  },
  [] (AbstractValue *x) {
    string result = "String/";

    if (x->have_assigned)
      result += value(x->assigned)->Print();
    else
      result += "<no value>";

    return result;
  },
  nullptr
};

struct MetaAPI_Datatype_String : MetaAPI_Datatype {
  MetaAPI_Datatype_String(string name) : MetaAPI_Datatype(name) {}

  AbstractValue AbstractValueInstance() {
    AbstractValue x = AbstractValue();
    x.type = AbstractValue::Type::String;
    x.interface = default_interface;
    return x;
  }

  AbstractValue FromString(t_cfg *cfg, string str) {
    AbstractValue x = AbstractValueInstance();
    x.generator_arguments.push_back(contents(MetaAPI_CreateString(CFG_OBJECT(cfg), str)));

    x.interface.possible_values = [] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
      MetaAPI_String *str = value(v->generator_arguments[0]);
      AbstractValue::PossibleValueList result;

      t_int32 max_len = GetCompareLengthProperty(properties);

      set<string> exclude;
      for (auto data : relations) {
        MetaAPI_Relation::Type relation = data.first;
        vector<MetaAPI_Variable *> variables = data.second;

        switch (relation) {
        case MetaAPI_Relation::Type::Ne: {
          for (auto i : variables) {
            if (i->abstract_value.type != AbstractValue::Type::String)
              continue;

            MetaAPI_String *x = value(i->abstract_value.assigned);
            exclude.insert(substring(x->Get(), max_len));
          }
        } break;

        default:
          FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
        }
      }

      if (exclude.find(substring(str->Get(), max_len)) == exclude.end())
        PossibleValueListAdd(result, v->generator_arguments[0]);

      return result;
    };
    return x;
  }

  AbstractValue FromExpression(t_cfg *cfg, MetaAPI_Expression expr) {
    AbstractValue result = AbstractValueInstance();

    auto add_arg = [&result] (string s) {
      AbstractValue::Contents c;
      c.generic = AbstractValue::pack(new string(s));
      result.generator_arguments.push_back(c);
    };

    /* RANDOM_IN_PROGRAM */
    if (expr.command == "RANDOM_IN_PROGRAM") {
      /* optional arguments */
      string str_ends_with = "";
      string str_not_ends_with = "";
      string str_contains = "";
      string str_not_contains = "";

      if (!expr.arguments.empty()) {
        MetaAPI_Expression expr2 = MetaAPI_Expression(expr.arguments);
        if (expr2.command == "ENDS_WITH")
          ASSERT(ExpressionArgumentExtractString(expr2.arguments, str_ends_with), ("string argument expected: %s", expr2.Print().c_str()));
        else if (expr2.command == "!ENDS_WITH")
          ASSERT(ExpressionArgumentExtractString(expr2.arguments, str_not_ends_with), ("string argument expected: %s", expr2.Print().c_str()));
        else if (expr2.command == "CONTAINS")
          ASSERT(ExpressionArgumentExtractString(expr2.arguments, str_contains), ("string argument expected: %s", expr2.Print().c_str()));
        else if (expr2.command == "!CONTAINS")
          ASSERT(ExpressionArgumentExtractString(expr2.arguments, str_not_contains), ("string argument expected: %s", expr2.Print().c_str()));
        else
          FATAL(("unhandled expression: %s", expr2.Print().c_str()));
      }

      add_arg(str_ends_with);
      add_arg(str_not_ends_with);
      add_arg(str_contains);
      add_arg(str_not_contains);

      result.interface.generator = [] (AbstractValue *v, RelationToOtherVariables relations) {
        FATAL(("remove this FATAL in the source code when it triggers. Not sure if it is needed."));

        MetaAPI_String::RelationSet string_relations = MetaAPI_StringRelationSet(
          relations,
          [] (MetaAPI_Variable *v) {return v->abstract_value.type == AbstractValue::Type::String;},
          [] (MetaAPI_Variable *v) {return value(v->abstract_value.assigned);}
        );

        return contents(MetaAPI_ChooseExistingString(string_relations));
      };

      result.interface.possible_values = [] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
        AbstractValue::PossibleValueList result;

        t_int32 compare_length = GetCompareLengthProperty(properties);

        set<string> exclude;
        for (auto data : relations) {
          MetaAPI_Relation::Type relation = data.first;
          vector<MetaAPI_Variable *> variables = data.second;

          switch(relation) {
          case MetaAPI_Relation::Type::Ne: {
            for (auto i : variables) {
              if (i->abstract_value.type != AbstractValue::Type::String)
                continue;

              MetaAPI_String *x = value(i->abstract_value.assigned);
              exclude.insert(substring(x->Get(), compare_length));
            }
          } break;

          default:
            FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
          }
        }

        /* first generator argument is the 'ends with' string */
        string str_ends_with = *AbstractValue::unpack<string>(v->generator_arguments[0].generic);
        bool test_ends_with = !str_ends_with.empty();

        /* second generator argument is the 'not ends with' string */
        string str_not_ends_with = *AbstractValue::unpack<string>(v->generator_arguments[1].generic);
        bool test_not_ends_with = !str_not_ends_with.empty();

        /* third generator argument is the 'contains' string */
        string str_contains = *AbstractValue::unpack<string>(v->generator_arguments[2].generic);
        bool test_contains = !str_contains.empty();

        /* fourth generator argument is the 'not contains' string */
        string str_not_contains = *AbstractValue::unpack<string>(v->generator_arguments[3].generic);
        bool test_not_contains = !str_not_contains.empty();

        MetaAPI_ForEachProgramString([&str_ends_with, &test_ends_with, &str_not_ends_with, &test_not_ends_with, &result, &exclude, compare_length, &str_contains, &test_contains, &str_not_contains, &test_not_contains] (MetaAPI_String *str) {
          if ((exclude.find(substring(str->Get(), compare_length)) == exclude.end())
              && (!test_ends_with || (test_ends_with && ends_with(str->Get(), str_ends_with)))
              && (!test_not_ends_with || (test_not_ends_with && !ends_with(str->Get(), str_not_ends_with)))
              && (!test_contains || (test_contains && contains(str->Get(), str_contains)))
              && (!test_not_contains || (test_not_contains && !contains(str->Get(), str_not_contains)))) {
            PossibleValueListAdd(result, contents(str));
#ifdef ONLY_ONE_POSSIBLE_VALUE
            return true;
#endif
          }

          return false;
        });

        return result;
      };
    }
    /* IN_PROGRAM(xxxxx) */
    else if (expr.command == "IN_PROGRAM") {
      string argument;
      ASSERT(ExpressionArgumentExtractString(expr.arguments, argument), ("expected argument string: %s", expr.Print().c_str()));
      add_arg(argument);

      result.interface.generator = [] (AbstractValue *v, RelationToOtherVariables relations) {
        MetaAPI_String::RelationSet string_relations = MetaAPI_StringRelationSet(
          relations,
          [] (MetaAPI_Variable *v) {return v->abstract_value.type == AbstractValue::Type::String;},
          [] (MetaAPI_Variable *v) {return value(v->abstract_value.assigned);}
        );

        // can't use 'value(...)' here because a string* is encoded, not a MetaAPI_String*
        string *str = AbstractValue::unpack<string>(v->generator_arguments[0].generic);
        return contents(MetaAPI_FindExistingString(*str, string_relations));
      };

      result.interface.possible_values = [&result] (AbstractValue *v, RelationToOtherVariables relations, Properties properties) {
        AbstractValue::PossibleValueList possibles;

        t_int32 compare_length = GetCompareLengthProperty(properties);

        set<string> exclude;
        for (auto data : relations) {
          MetaAPI_Relation::Type relation = data.first;
          vector<MetaAPI_Variable *> variables = data.second;

          switch(relation) {
          case MetaAPI_Relation::Type::Ne: {
            for (auto i : variables) {
              if (i->abstract_value.type != AbstractValue::Type::String)
                continue;

              MetaAPI_String *x = value(i->abstract_value.assigned);
              exclude.insert(substring(x->Get(), compare_length));
            }
          } break;

          default:
            FATAL(("unsupported relation %s", MetaAPI_Relation::Print(relation).c_str()));
          }
        }

        MetaAPI_ForEachProgramString([v, &possibles, &exclude, compare_length] (MetaAPI_String *str) {
          if ((exclude.find(substring(str->Get(), compare_length)) == exclude.end())
              && eq(str, AbstractValue::unpack<string>(v->generator_arguments[0].generic), compare_length)) {
            PossibleValueListAdd(possibles, contents(str));

#ifdef ONLY_ONE_POSSIBLE_VALUE
            return true;
#endif
          }

          return false;
        });

        return possibles;
      };
    }
    /* unknown */
    else {
      FATAL(("unknown expression: ", expr.Print().c_str()));
    }

    return result;
  }

  string Print(AbstractValue::Contents contents) {
    return value(contents)->Print();
  }
};

DEFINE_REGISTER_DATATYPE_FN(DtString) {
  VERBOSE(meta_api_verbosity, (META_API_PREFIX "implementation dt_string for '%s'", name.c_str()));
  return new MetaAPI_Datatype_String(name);
}
