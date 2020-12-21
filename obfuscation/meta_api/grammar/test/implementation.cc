#include <string>
#include <vector>

#include "implementationParser.h"
#include "implementationLexer.h"

using namespace std;

pANTLR3_UINT8 to_antlr(string str) {
  char *x = const_cast<char *>(str.c_str());
  return reinterpret_cast<pANTLR3_UINT8>(x);
}

int main(int argc, char** argv) {
  ASSERT(argc == 2, ("expected <file>"));
  string str = string(argv[1]);

  auto input = antlr3FileStreamNew(to_antlr(str), ANTLR3_ENC_8BIT);

  auto lexer = implementationLexerNew(input);
  ASSERT(lexer, ("unable to create lexer '%s'", str.c_str()));
  
  auto tokenizer = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
  ASSERT(tokenizer, ("unable to create tokenizer '%s'", str.c_str()));

  auto parser = implementationParserNew(tokenizer);
  ASSERT(parser, ("unable to create parser '%s'", str.c_str()));

  DEBUG(("parsing implementation '%s'", str.c_str()));
  parser->parse(parser, NULL);

  /* cleanup */
  parser->free(parser);
  tokenizer->free(tokenizer);
  lexer->free(lexer);
}
