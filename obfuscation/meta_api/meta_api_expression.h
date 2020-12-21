#ifndef META_API_EXPRESSION_H
#define META_API_EXPRESSION_H

struct _MetaAPI_Expression;
typedef struct _MetaAPI_Expression MetaAPI_Expression;

struct _MetaAPI_Expression {
  std::string command;
  std::string arguments;

  _MetaAPI_Expression(std::string str);
  std::vector<std::string> SplitArgumentString();
  std::string Print();
};

bool ExpressionArgumentExtractString(std::string s, std::string& result);

#endif /* META_API_EXPRESSION_H */
