#include "meta_api.h"

using namespace std;

MetaAPI_Expression::_MetaAPI_Expression(string str) {
  /* arguments present? */
  auto found = str.find_first_of("(");
  if (found != string::npos) {
    /* arguments present */
    command = str.substr(0, found);
    arguments = str.substr(found + 1, str.size() - found - 2);
  }
  else {
    command = str;
    arguments = "";
  }
}

vector<string> MetaAPI_Expression::SplitArgumentString() {
  vector<string> result;

  stringstream ss(arguments);
  string token;
  while (getline(ss, token, ','))
    result.push_back(token);

  return result;
}

string MetaAPI_Expression::Print() {
  string result = "Expression/";
  result += command;
  result += "(" + arguments + ")";

  return result;
}

bool ExpressionArgumentExtractString(string s, string& result) {
  if (s.size() < 2)
    return false;

  if (!((s[0] == '\'') && (s[s.size()-1] == '\'')))
    return false;

  result = s.substr(1, s.size() - 2);
  return result.size() > 0;
}
