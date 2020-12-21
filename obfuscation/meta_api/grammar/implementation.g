grammar implementation;

/* common */
options {
  language=C;
}

@parser::header {
#ifdef GRAMMAR_TEST
#include "grammar_test.h"
#else
#include "meta_api.h"
#endif

using namespace std;

// #define DEBUGGING
#ifdef DEBUGGING
#define MY_DEBUG(x) DEBUG(x)
#define MY_DEBUG2(x)
#else
#define MY_DEBUG(x)
#define MY_DEBUG2(x)
#endif

#define ADD_STMT_LIST(x) stmts.push_back(x)
#define REMOVE_STMT_LIST(x) stmts.pop_back()
#define ADD_STMT(x) stmts.back()->push_back(x)
}
@parser::members {
#include <grammar/implementation.cc>
}

@lexer::header {
}

/* parser */
parse [vector<MetaAPI_AbstractStmt *>* _stmt_list]
@init {
  ADD_STMT_LIST(_stmt_list);
  current_state = ParserState::GlobalScope;
  push_function_call = true;

  MY_DEBUG(("START PARSING"));
}
@after {
  MY_DEBUG(("STOP PARSING"));

  ASSERT(stmts.size() == 1, ("missing END statement (\%d)", stmts.size()));
  REMOVE_STMT_LIST();
}
  : statement+;

statement
  : ML_COMMENT
  | variable_declaration SEMICOLON
  | function_call SEMICOLON
  | return_stmt SEMICOLON
  | variable_assignment SEMICOLON
  | end_stmt SEMICOLON
  | break_stmt SEMICOLON
  | if_stmt
  | while_stmt
  ;

/* explicit statements */
variable_declaration returns [MetaAPI_VariableDeclarationStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("variable declarations needs to be done at global scope"));

  S = new MetaAPI_VariableDeclarationStmt();
  ADD_STMT(S);

  MY_DEBUG2(("rule:variable_declaration"));
}
@after {
  MY_DEBUG(("declared variable: \%s", S->Print().c_str()));
}
  : datatype=IDENTIFIER (STAR {S->pointer = true;})? name=IDENTIFIER (array_index {S->array_size = $array_index.result;})? (EQUALS (rvalue{S->value = $rvalue.result; MY_DEBUG(("rvalue \%s", S->value.Print().c_str()));}))?
  {
    S->datatype = to_string($datatype.text);
    S->identifier = to_string($name.text);
  };

variable_assignment returns [MetaAPI_VariableAssignmentStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("variable assignment needs to be done at global scope"));

  S = new MetaAPI_VariableAssignmentStmt();
  ADD_STMT(S);

  MY_DEBUG2(("rule:variable_assignment"));
  push_function_call = false;

  t_uint8 modifiers = 0;

  MetaAPI_ImplementationValue to_array_index = MetaAPI_ImplementationValue();
}
@after {
  MY_DEBUG(("assigned variable: \%s", S->Print().c_str()));
  push_function_call = true;
}
  : (STAR {modifiers |= MetaAPI_ImplementationValue::MODIFIER_DEREFERENCE;})? target=meta_api_variable EQUALS (rvalue{S->S = new MetaAPI_RvalueStmt($rvalue.result);})
  {
    S->target = $target.result;
    S->target.modifiers = modifiers;
  }
  | target=meta_api_variable (array_index {to_array_index = $array_index.result;})? EQUALS (function_call{S->S = $function_call.S;})
  {
    S->target = $target.result;
    S->target.array_index = new MetaAPI_ImplementationValue(to_array_index);
  }
  | target=meta_api_variable EQUALS (expression{S->S = $expression.S;})
  {
    S->target = $target.result;
  };

function_call returns [MetaAPI_FunctionCallStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("variable declarations needs to be done at global scope"));
  current_state = ParserState::FunctionCall;

  S = new MetaAPI_FunctionCallStmt();
  if (push_function_call)
    ADD_STMT(S);

  MY_DEBUG2(("rule:function_call"));
}
@after {
  current_state = ParserState::GlobalScope;
  MY_DEBUG(("called function: \%s", S->Print().c_str()));
}
  : (STAR {S->dereference = true;})? (NEW {S->call_new = true;})? name=IDENTIFIER LPAREN (x=rvalue {S->values.push_back($x.result);} (COMMA y=rvalue {S->values.push_back($y.result);})*)? RPAREN
  {
    S->callee = to_string($name.text);
  };

break_stmt returns [MetaAPI_BreakStmt *S]
@init {
  S = new MetaAPI_BreakStmt();
  ADD_STMT(S);

  MY_DEBUG2(("rule:break_stmt"));
}
@after {
  MY_DEBUG(("break"));
}
  : BREAK {};

if_stmt returns [MetaAPI_IfStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("IF statements should be at global scope"));

  S = new MetaAPI_IfStmt();
  ADD_STMT(S);
  ADD_STMT_LIST(&(S->body));

  MY_DEBUG2(("rule:if_stmt"));
  push_function_call = false;
}
@after {
  MY_DEBUG(("got if: \%s", S->Print().c_str()));
  push_function_call = true;
}
  : IF (NOT {S->inverted_condition = true;})? LPAREN (
      function_call {S->S = $function_call.S;}
    | relation {S->S = $relation.S;}
    ) RPAREN
  {};

while_stmt returns [MetaAPI_WhileStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("WHILE statements should be at global scope"));

  S = new MetaAPI_WhileStmt();
  ADD_STMT(S);
  ADD_STMT_LIST(&(S->body));

  MY_DEBUG2(("rule:while_stmt"));
  push_function_call = false;
}
@after {
  MY_DEBUG(("got while: \%s", S->Print().c_str()));
  push_function_call = true;
}
  : WHILE (NOT {S->inverted_condition = true;})? LPAREN (
      function_call {S->S = $function_call.S;}
    | relation {S->S = $relation.S;}
    | rvalue {S->S = new MetaAPI_RvalueStmt($rvalue.result);}
    ) RPAREN
  {};

return_stmt returns [MetaAPI_ReturnStmt *S]
@init {
  ASSERT(current_state == ParserState::GlobalScope, ("return statements should be at global scope"));

  S = new MetaAPI_ReturnStmt();
  ADD_STMT(S);

  MY_DEBUG2(("rule:return_stmt"));
  push_function_call = false;
}
@after {
  MY_DEBUG(("got return: \%s", S->Print().c_str()));
  push_function_call = true;
}
  : RETURN (
      rvalue{S->S = new MetaAPI_RvalueStmt($rvalue.result);}
    | relation{S->S = $relation.S;}
    | function_call{S->S = $function_call.S;}
    )?
  {};

/* implicit statements */
end_stmt
@after {
  stmts.pop_back();
}
  : END {};

meta_api_variable returns [MetaAPI_ImplementationValue result]
@init {
  result = MetaAPI_ImplementationValue();
  MY_DEBUG2(("rule:meta_api_variable"));
}
  : DOLLAR DOLLAR DOLLAR iname=IDENTIFIER {
    result.type = MetaAPI_ImplementationValue::Type::MetaInstanceVariable;
    result.value = to_string($iname.text);
    MY_DEBUG2(("  instance variable \%s", result.value.c_str()));
  }
  | DOLLAR DOLLAR gname=IDENTIFIER {
    result.type = MetaAPI_ImplementationValue::Type::MetaGlobalVariable;
    result.value = to_string($gname.text);
    MY_DEBUG2(("  global variable \%s", result.value.c_str()));
  }
  | DOLLAR lname=IDENTIFIER {
    result.type = MetaAPI_ImplementationValue::Type::MetaLocalVariable;
    result.value = to_string($lname.text);
    MY_DEBUG2(("  local variable \%s", result.value.c_str()));
  };

rvalue returns [MetaAPI_ImplementationValue result]
@init {
  result = MetaAPI_ImplementationValue();
  t_uint8 modifiers = 0;
  MY_DEBUG2(("rule:rvalue"));
}
  : (AMPERSAND { modifiers |= MetaAPI_ImplementationValue::MODIFIER_REFERENCE; })? meta_api_annotation {result = $meta_api_annotation.result;} (DOT IDENTIFIER {modifiers |= MetaAPI_ImplementationValue::MODIFIER_MEMBER; result.value2 = to_string($IDENTIFIER.text);} )? {
    result.modifiers = modifiers;
  }
  | STRING{
    result.type = MetaAPI_ImplementationValue::Type::String;
    result.value = to_string($STRING.text);
  }
  | NUMBER{
    result.type = MetaAPI_ImplementationValue::Type::Number;
    result.value = to_string($NUMBER.text);
  }
  | ((AMPERSAND { modifiers |= MetaAPI_ImplementationValue::MODIFIER_REFERENCE; })? IDENTIFIER){
    result.type = MetaAPI_ImplementationValue::Type::Identifier;
    result.value = to_string($IDENTIFIER.text);
    result.modifiers = modifiers;
  }
  | ((STAR {modifiers |= MetaAPI_ImplementationValue::MODIFIER_DEREFERENCE;})? (AMPERSAND { modifiers |= MetaAPI_ImplementationValue::MODIFIER_REFERENCE; })? meta_api_variable (array_index {$meta_api_variable.result.array_index = new MetaAPI_ImplementationValue($array_index.result);})?){
    result = $meta_api_variable.result;
    result.modifiers = modifiers;
  }
  | (AMPERSAND { modifiers |= MetaAPI_ImplementationValue::MODIFIER_REFERENCE; })? meta_api_variable DOT IDENTIFIER{modifiers |= MetaAPI_ImplementationValue::MODIFIER_MEMBER;} {
    result = $meta_api_variable.result;
    result.value2 = to_string($IDENTIFIER.text);
    result.modifiers = modifiers;
  };

array_index returns [MetaAPI_ImplementationValue result]
@init {
  result = MetaAPI_ImplementationValue();
  MY_DEBUG2(("rule:array_index"));
}
  : LBRACK NUMBER RBRACK
  {
    result.type = MetaAPI_ImplementationValue::Type::Number;
    result.value = to_string($NUMBER.text);
  }
  | LBRACK meta_api_variable RBRACK {
    result = $meta_api_variable.result;
  };

relation returns [MetaAPI_RelationStmt *S]
@init {
  S = new MetaAPI_RelationStmt();
  MY_DEBUG2(("rule:relation"));
}
@after {
  MY_DEBUG(("relation '\%s'", S->Print().c_str()));
}
  : (x=rvalue RELATION y=rvalue) {
    S->op1 = $x.result;
    S->rel = MetaAPI_Relation(to_string($RELATION.text));
    S->op2 = $y.result;
  }
  | (function_call RELATION rvalue) {
    S->S1 = $function_call.S;
    S->rel = MetaAPI_Relation(to_string($RELATION.text));
    S->op2 = $rvalue.result;
  };

expression returns [MetaAPI_ExpressionStmt *S]
@init {
  S = new MetaAPI_ExpressionStmt();
  MY_DEBUG2(("rule:expression"));
}
@after {
  MY_DEBUG(("expression '\%s'", S->Print().c_str()));
}
  : (x=rvalue OPERAND y=rvalue) {
    S->op1 = $x.result;
    S->operand = MetaAPI_Operand(to_string($OPERAND.text));
    S->op2 = $y.result;
  };

meta_api_annotation returns [MetaAPI_ImplementationValue result]
@init {
  result = MetaAPI_ImplementationValue();
  MY_DEBUG2(("rule:meta_api_annotation"));
}
  : DOLLAR LBRACK annotation=IDENTIFIER RBRACK
  {
    result.type = MetaAPI_ImplementationValue::Type::Annotation;
    result.value = to_string($annotation.text->chars);
  };

/* lexer */
ML_COMMENT: '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;};
IF: 'IF';
WHILE: 'WHILE';
RETURN: 'RETURN';
END: 'END';
NEW: 'new';
BREAK: 'BREAK';
RELATION: ('==' | '<' | '>' | '!=' | '<=' | '>=' | '%');
MINUS: '-';
OPERAND: ('+' | MINUS);
NOT: '!';
fragment NEWLINE: ('\r' | '\n');
WHITESPACE: (' ' | '\t' | NEWLINE)+ { $channel = HIDDEN; };
DOLLAR: '$';
EQUALS: '=';
COMMA: ',';
fragment ALPHA: (('a'..'z')|('A'..'Z')|'_');
fragment DIGIT: ('0'..'9');
SEMICOLON: ';';
LBRACK: '[';
RBRACK: ']';
LPAREN: '(';
RPAREN: ')';
fragment DQUOTE: '"';
DOT: '.';
IDENTIFIER: (ALPHA(ALPHA|DIGIT)*);
STRING: DQUOTE ('\\' ~(NEWLINE)|~('\\'|DQUOTE|NEWLINE))* DQUOTE;
NUMBER: MINUS? (DIGIT)+ (DOT DIGIT+)?;
AMPERSAND: '&';
STAR: '*';
