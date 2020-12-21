#ifndef META_API_EFFECT_H
#define META_API_EFFECT_H

struct _MetaAPI_Effect;
typedef struct _MetaAPI_Effect MetaAPI_Effect;

struct _MetaAPI_Effect {
  enum class Effect {
    True,
    False,
    Unknown,
    Unchanged
  };
  static Effect EffectFromString(std::string str);
  static std::string EffectToString(Effect effect);
  static Effect RandomTrueFalse();
  static Effect Invert(Effect effect);
  static bool EffectIsBoolean(Effect effect);

  std::string expression;
  std::map<MetaAPI_Predicate *, Effect> affected_predicates;

  std::string Print();

  _MetaAPI_Effect(std::string expr);
};

void MetaAPI_ParseEffectWithGrammar(MetaAPI_Datatype *datatype, MetaAPI_Effect *effect);
void MetaAPI_RandomTrueFalseEffect();

#endif /* META_API_EFFECT_H */
