#include "meta_api.h"

using namespace std;

string MetaAPI_Constraint::Print() {
  string result = "";

  auto get_type = [] (OperandType t) {
    string result = "";

    switch (t) {
    case OperandType::GlobalVariable:
      result = "global";
      break;
    case OperandType::InstanceVariable:
      result = "instance";
      break;
    case OperandType::LocalVariable:
      result = "local";
      break;
    case OperandType::Integer:
      result = "integer";
      break;

    default:
      FATAL(("unhandled"));
    }

    return result;
  };

  result += get_type(v1_type) + "/";

  if (v1_type == OperandType::GlobalVariable)
    result += v1->Print();
  else {
    result += v1_identifier;

    if (v1)
      result += " (" + v1->Print() + ")";
    else
      result += " (unassigned)";
  }

  result += MetaAPI_Relation::Print(relation);

  if (v2_type == OperandType::GlobalVariable)
    result += v2->Print();
  else {
    result += v2_identifier;

    if (v2)
      result += " (" + v2->Print() + ")";
    else
      result += " (unassigned)";
  }

  return result;
}

MetaAPI_Constraint::_MetaAPI_Constraint(string expr) {
  expression = expr;
  v1 = NULL;
  v2 = NULL;
}

/* constraint parsing */
#include "constraintsParser.h"
#include "constraintsLexer.h"

/* STATUS is defined by one of the included system header files in Antlr.
 * As Diablo also has a STATUS preprocessor definition, we need to undefine Antlr's definition to mitigate a compiler warning. */
#ifdef STATUS
#undef STATUS
#endif

void MetaAPI_ParseConstraintWithGrammar(MetaAPI_Constraint *constraint) {
  string str = constraint->expression;
  auto input = antlr3StringStreamNew((pANTLR3_UINT8)str.c_str(), ANTLR3_ENC_8BIT, str.size(), (pANTLR3_UINT8)str.c_str());

  pconstraintsLexer lexer = constraintsLexerNew(input);
  ASSERT(lexer, ("unable to create lexer '%s'", str.c_str()));

  auto tokenizer = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  ASSERT(tokenizer, ("unable to create tokenizer '%s'", str.c_str()));

  auto parser = constraintsParserNew(tokenizer);
  ASSERT(parser, ("unable to create parser '%s'", str.c_str()));

  parser->parse(parser, constraint);

  /* cleanup */
  parser->free(parser);
  tokenizer->free(tokenizer);
  lexer->free(lexer);
}
