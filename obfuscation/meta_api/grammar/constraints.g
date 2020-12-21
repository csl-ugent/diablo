grammar constraints;

/* common */
options {
  language=C;
}

@parser::header {
#include "meta_api.h"

using namespace std;

// #define DEBUGGING
#ifdef DEBUGGING
#define MY_DEBUG(x) DEBUG(x)
#else
#define MY_DEBUG(x)
#endif
}
@parser::members {
#include <grammar/constraints.cc>
}

@lexer::header {
#include "meta_api.h"
}

/* parser */
parse [MetaAPI_Constraint *current_constraint]
@init {
  _current_constraint = current_constraint;
}
  : constraint
  ;

constraint
  : ident1=IDENTIFIER RELATION ident2=IDENTIFIER
  {
    MY_DEBUG(("add constraint"));
    add_constraint(to_string($ident1.text), to_string($ident2.text), MetaAPI_Relation(to_string($RELATION.text)));
  }
  | ident1=IDENTIFIER RELATION ident2=NUMBER
  {
    MY_DEBUG(("add constraint number"));
    add_constraint_int(to_string($ident1.text), to_string($ident2.text), MetaAPI_Relation(to_string($RELATION.text)));
  }
  | ident1=IDENTIFIER RELATION ident2=STRING
  {
    MY_DEBUG(("add constraint string"));
    string str = to_string($ident2.text);
    str = str.substr(1, str.size()-2);
    add_constraint_string(to_string($ident1.text), str, MetaAPI_Relation(to_string($RELATION.text)));
  };

/* lexer */
fragment DIGIT: ('0'..'9');
fragment DOT: '.';
fragment NEWLINE: ('\r' | '\n');
WHITESPACE: (' ' | '\t' | NEWLINE)+ { $channel = HIDDEN; };
RELATION: ('==' | '<' | '>' | '!=' | '<=' | '>=' | 'INSTANCEOF');
IDENTIFIER: (('$')|('.')|('a'..'z')|('A'..'Z')|('_'))(('$')|('.')|('a'..'z')|('A'..'Z')|('0'..'9')|('_'))*;
NUMBER: (DIGIT)+ (DOT DIGIT+)?;
fragment DQUOTE: '"';
STRING: DQUOTE ('\\' ~(NEWLINE)|~('\\'|DQUOTE|NEWLINE))* DQUOTE;
