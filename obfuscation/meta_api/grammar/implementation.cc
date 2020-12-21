#include <string>
#include <vector>

using namespace std;

enum class ParserState {
  GlobalScope,
  FunctionCall,
  Other
};
ParserState current_state;

vector<vector<MetaAPI_AbstractStmt *> *> stmts;
static bool push_function_call;

static
string to_string(pANTLR3_UINT8 x) {
  return string(reinterpret_cast<char*>(x));
}

static
string to_string(pANTLR3_STRING x) {
  return to_string(x->chars);
}
