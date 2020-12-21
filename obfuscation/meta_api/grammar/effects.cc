#include <string>

using namespace std;

static MetaAPI_Datatype *_current_datatype;
static MetaAPI_Effect *_current_effect;

void add_effect(pANTLR3_UINT8 _predicate_name, pANTLR3_UINT8 _effect) {
  string predicate_name = string(reinterpret_cast<char *>(_predicate_name));
  string effect = string(reinterpret_cast<char *>(_effect));

  string full_predicate_name = _current_datatype->name + "_" + predicate_name;
  MetaAPI_Predicate *predicate = MetaAPI_GetPredicateByName(full_predicate_name);
  ASSERT(_current_effect->affected_predicates.find(predicate) == _current_effect->affected_predicates.end(),
    ("duplicate effect for predicate '%s'", full_predicate_name.c_str()));

  _current_effect->affected_predicates[predicate] = MetaAPI_Effect::EffectFromString(effect);
}
