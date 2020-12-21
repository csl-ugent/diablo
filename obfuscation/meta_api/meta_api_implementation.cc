#include "meta_api.h"

using namespace std;

string MetaAPI_ImplementationValue::Print() const {
  string result;
  result += "t=" + to_string(static_cast<t_uint32>(type));

  if (type != Type::Null)
    result += " v=" + value;

  result += " f=" + to_string(modifiers);

  if (modifiers & MODIFIER_MEMBER)
    result += "/member:" + value2;
  if (modifiers & MODIFIER_REFERENCE)
    result += " reference";
  if (modifiers & MODIFIER_DEREFERENCE)
    result += " dereference";

  if (array_index)
    result += " array[" + array_index->Print()+ "]";

  return "Value(" + result + ")";
}

bool MetaAPI_ImplementationValue::GetNumber(t_uint32& n) const {
  if (type != Type::Number)
    return false;
  
  n = stoi(value);
  return true;
}

string MetaAPI_VariableDeclarationStmt::Print() {
  string result = datatype;

  result += " " + identifier;
  if (array_size.type != MetaAPI_ImplementationValue::Type::None)
    result += "[" + array_size.Print() + "]";
  if (value.type != MetaAPI_ImplementationValue::Type::Null)
    result += " = " + value.Print();

  return result;
}

string MetaAPI_BreakStmt::Print() {
  return "break";
}

string MetaAPI_VariableAssignmentStmt::Print() {
  string result = "Assignment/";

  result += target.Print() + " = ";
  if (S)
    result += S->Print();
  else
    result += " (uninitialised)";

  return result;
}

string MetaAPI_FunctionCallStmt::Print() {
  string result = callee;

  result += "(";
  for (auto x : values)
    result += x.Print() + ", ";
  result += ")";

  if (call_new)
    result += " [call-new]";

  return result;
}

string MetaAPI_IfStmt::Print() {
  return "if/" + S->Print();
}

string MetaAPI_WhileStmt::Print() {
  return "while/" + S->Print();
}

string MetaAPI_RelationStmt::Print() {
  return (S1 ? S1->Print() : op1.Print()) + " " + rel.Print() + " " + op2.Print();
}

string MetaAPI_ExpressionStmt::Print() {
  return op1.Print() + " " + operand.Print() + " " + op2.Print();
}

string MetaAPI_RvalueStmt::Print() {
  return value.Print();
}

string MetaAPI_ReturnStmt::Print() {
  if (!S)
    return "return/(null)";

  return "return/" + S->Print();
}

/* implementation parsing */
#include "implementationParser.h"
#include "implementationLexer.h"

/* STATUS is defined by one of the included system header files in Antlr.
 * As Diablo also has a STATUS preprocessor definition, we need to undefine Antlr's definition to mitigate a compiler warning. */
#ifdef STATUS
#undef STATUS
#endif

void MetaAPI_ParseImplementationWithGrammar(string str, vector<MetaAPI_AbstractStmt *>* stmt_list) {
  auto input = antlr3StringStreamNew((pANTLR3_UINT8)str.c_str(), ANTLR3_ENC_8BIT, str.size(), (pANTLR3_UINT8)str.c_str());
  if (str.empty())
    return;

  pimplementationLexer lexer = implementationLexerNew(input);
  ASSERT(lexer, ("unable to create lexer '%s'", str.c_str()));

  auto tokenizer = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  ASSERT(tokenizer, ("unable to create tokenizer '%s'", str.c_str()));

  auto parser = implementationParserNew(tokenizer);
  ASSERT(parser, ("unable to create parser '%s'", str.c_str()));

  parser->parse(parser, stmt_list);

  /* cleanup */
  parser->free(parser);
  tokenizer->free(tokenizer);
  lexer->free(lexer);
}
