#include "meta_api.h"

using namespace std;

#undef META_API_PREFIX
#define META_API_PREFIX "[meta-api (datatype)] "

bool MetaAPI_Datatype_cmp::operator() (MetaAPI_Datatype *a, MetaAPI_Datatype *b) const {
  return a->uid < b->uid;
}

AbstractValue MetaAPI_Datatype::FromString(t_cfg *cfg, string str) {
  FATAL(("override FromString for '%s'", name.c_str()));
}

static
Properties merge_properties(Properties a, Properties overrides) {
  for (auto x : overrides) {
    a[x.first] = x.second;
  }

  return a;
}

PossibleValueSet MetaAPI_Datatype::CollectPossibleValues(RelationToOtherVariables relations, Properties properties) {
  PossibleValueSet result;

  auto add_to_result = [&result] (MetaAPI_Datatype *type, AbstractValue::PossibleValueList data) {
    if (data.size() == 0)
      return;

    if (result.find(type) == result.end())
      result[type] = set<AbstractValue::Contents>();
    result[type].insert(data.begin(), data.end());
  };

  auto add_to_result_2 = [&result] (MetaAPI_Datatype *type, set<AbstractValue::Contents> data) {
    if (data.size() == 0)
      return;

    if (result.find(type) == result.end())
      result[type] = set<AbstractValue::Contents>();
    result[type].insert(data.begin(), data.end());
  };

  ASSERT(values.size() > 0, (META_API_PREFIX "expected at least one value for %s", Print().c_str()));
  for (auto i : values) {
    if (i.value.type == AbstractValue::Type::MetaAPI_Proxy) {
      PossibleValueSet proxy_result = i.type->CollectPossibleValues(relations, merge_properties(properties, i.properties));
      for (auto ii : proxy_result)
        add_to_result_2(ii.first, ii.second);
    }
    else {
      AbstractValue::PossibleValueList possibles = i.value.PossibleValues(relations, merge_properties(properties, i.properties));
      add_to_result(i.type, possibles);
    }
  }

  VERBOSE(meta_api_verbosity, ("found %d possible values for '%s'", result.size(), Print().c_str()));

  return result;
}

AbstractValue MetaAPI_Datatype::GenerateRandomValue(RelationToOtherVariables relations, t_randomnumbergenerator *rng) {
  VERBOSE(meta_api_verbosity, ("GenerateRandomValue for '%s'", Print().c_str()));

  Properties no_properties;
  PossibleValueSet possibles = CollectPossibleValues(relations, no_properties);

  for (auto i : possibles) {
    MetaAPI_Datatype *datatype = i.first;
    set<AbstractValue::Contents> values = i.second;

    VERBOSE(meta_api_verbosity, ("found %d values of type '%s'", values.size(), datatype->Print().c_str()));
  }

  ASSERT(possibles.size() > 0, ("no possible values for %s", Print().c_str()));

  auto A_it = possibles.begin();
  if (possibles.size() > 1) {
    /* random iterator */
    advance(A_it, RNGGenerateWithRange(rng, 0, possibles.size() - 1));
  }

  MetaAPI_Datatype *datatype = A_it->first;
  VERBOSE(meta_api_verbosity, ("chose values of type %s", datatype->Print().c_str()));

  auto values = A_it->second;
  ASSERT(values.size() > 0, ("no values"));
  auto B_it = values.begin();
  if (values.size() > 1) {
    /* random iterator */
    advance(B_it, RNGGenerateWithRange(rng, 0, values.size() - 1));
  }

  AbstractValue::Contents contents = *B_it;
  VERBOSE(meta_api_verbosity, ("picked value %s", datatype->Print(contents).c_str()));

  return datatype->FromContents(contents);
}

vector<MetaAPI_Datatype *> DatatypesWithPredicates() {
  vector<MetaAPI_Datatype *> result;

  for (auto i : registered_datatypes) {
    if (i.second->predicates.size() > 0)
      result.push_back(i.second);
  }

  return result;
}

t_int32 MetaAPI_Datatype::MemberOffset(string name) {
  for (auto m : members)
    if (m.name == name)
      return m.offset;
  
  return -1;
}
