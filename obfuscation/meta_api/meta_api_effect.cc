#include "meta_api.h"

using namespace std;

MetaAPI_Effect::Effect MetaAPI_Effect::EffectFromString(std::string str) {
  ASSERT(str.size() == 1, ("unexpected effect string '%s'", str.c_str()));

  if (str[0] == 'T')
    return Effect::True;

  else if (str[0] == 'F')
    return Effect::False;

  else if (str[0] == 'U')
    return Effect::Unchanged;

  else if (str[0] == '?')
    return Effect::Unknown;

  FATAL(("unknown effect '%s'", str.c_str()));
}

string MetaAPI_Effect::EffectToString(MetaAPI_Effect::Effect effect) {
  switch (effect) {
  case Effect::False:
    return "False";
  case Effect::True:
    return "True";
  case Effect::Unchanged:
    return "Unchanged";
  case Effect::Unknown:
    return "Unknown";
  default:
    FATAL(("unsupported effect %d", static_cast<t_uint32>(effect)));
  }
}

bool MetaAPI_Effect::EffectIsBoolean(MetaAPI_Effect::Effect effect) {
  return (effect == Effect::True) || (effect == Effect::False);
}

string MetaAPI_Effect::Print() {
  /* input */
  string result = "Original[" + expression + "]";

  /* parsed */
  for (auto affected : affected_predicates)
    result += "(" + affected.first->Print() + "->" + EffectToString(affected.second) + "), ";

  return result;
}

MetaAPI_Effect::_MetaAPI_Effect(string expr) {
  expression = expr;
}

MetaAPI_Effect::Effect MetaAPI_Effect::RandomTrueFalse() {
  if (RNGGenerateBool(meta_api_effect_rng))
    return Effect::True;

  return Effect::False;
}

MetaAPI_Effect::Effect MetaAPI_Effect::Invert(MetaAPI_Effect::Effect value) {
  Effect result;

  switch (value) {
  case Effect::True:
    result = Effect::False;
    break;

  case Effect::False:
    result = Effect::True;
    break;

  default:
    FATAL(("can't invert effect %s", EffectToString(value).c_str()));
  }

  return result;
}

/* STATUS is defined by one of the included system header files in Antlr.
 * As Diablo also has a STATUS preprocessor definition, we need to undefine Antlr's definition to mitigate a compiler warning. */
#ifdef STATUS
#undef STATUS
#endif

/* effect parsing */
#include "effectsParser.h"
#include "effectsLexer.h"

void MetaAPI_ParseEffectWithGrammar(MetaAPI_Datatype *datatype, MetaAPI_Effect *effect) {
  string str = effect->expression;
  auto input = antlr3StringStreamNew((pANTLR3_UINT8)str.c_str(), ANTLR3_ENC_8BIT, str.size(), (pANTLR3_UINT8)"my-effect");

  peffectsLexer lexer = effectsLexerNew(input);
  ASSERT(lexer, ("unable to create lexer '%s'", str.c_str()));

  auto tokenizer = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  ASSERT(tokenizer, ("unable to create tokenizer '%s'", str.c_str()));

  auto parser = effectsParserNew(tokenizer);
  ASSERT(parser, ("unable to create parser '%s'", str.c_str()));

  parser->parse(parser, datatype, effect);

  /* cleanup */
  parser->free(parser);
  tokenizer->free(tokenizer);
  lexer->free(lexer);
}
