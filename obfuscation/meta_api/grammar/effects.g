grammar effects;

/* common */
options {
  language=C;
}

@parser::header {
#include "meta_api.h"
}
@parser::members {
#include <grammar/effects.cc>
}

@lexer::header {
#include "meta_api.h"
}

/* parser */
parse [MetaAPI_Datatype *current_datatype, MetaAPI_Effect *current_effect]
@init {
  _current_datatype = current_datatype;
  _current_effect = current_effect;
}
  : effect (COMMA effect)*
  ;

effect
  : IDENTIFIER EQUALS EFFECT
  {
    add_effect($IDENTIFIER.text->chars, $EFFECT.text->chars);
  }
  ;

/* lexer */
fragment NEWLINE: ('\r' | '\n');
WHITESPACE: (' ' | '\t' | NEWLINE)+ { $channel = HIDDEN; };
EQUALS    : ('=');
COMMA     : (',');
EFFECT    : ('T' | 'F' | 'U' | '?');
IDENTIFIER: (('a'..'z')|('A'..'Z')|('_'))(('a'..'z')|('A'..'Z')|('0'..'9')|('_'))*;

NUMBER    : ('0'..'9')+;
