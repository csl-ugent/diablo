grammar Predicates;

options {
  language=C;
}

@parser::header {
#include "meta_api.h"
}
@lexer::header {
#include "meta_api.h"
}

@parser::members {
#include <grammar/members.c>
}

parse	returns [struct OP_Datastructure* output]
@init { initDatastructure(); output = currentStructure; }
	:	(semanticRule (WS)*)+{ };

semanticRule
	:	w1=declaration w2=rules { addTransformationToStructure(); };

declaration
	:	functionname WS argumentlist  { setTransformationName( $functionname.text->chars ); splitArgumentListTransformation( $argumentlist.text->chars );};
call	:	functionname WS callArgumentList;

functionname
	:	IDENTIFIER;

argumentlist
	:	variable(',' variable)*;
callArgumentList
	:	(( NUMBER )(',' ( NUMBER) )*)*;

rules 	:	((WS) PIPE rule)*;
rule	:	(condition|allCondition) IMPLIES effects { };

condition
	:	v1 = variable (WS) OPERATION (WS) (v2 = variable | n1 = NUMBER) {addRule($v1.text->chars, $OPERATION.text->chars, $condition.text->chars,0); };
allCondition	:	ALL{addRule(NULL, NULL, NULL,1);};
effects	:	effect ( (WS) '&&' (WS)  effect)*;
effect 	:	(variable) EQUALS EFFECT { addEffect($variable.text->chars, $EFFECT.text->chars); };

variable:	IDENTIFIER;

WS	:	( ' ' | '\t' | '\r' | '\n' | '\r\n');
PIPE	:	'|';
IMPLIES	:	'=>';
OPERATION	:( '<' | '>'|'=='|'>='|'<='|'!=');
EQUALS	:	'=';
EFFECT	:	('T'|'F'|'?'|'U');
NUMBER	:	('0'..'9')+;
ALL	:	'ALL';
IDENTIFIER
	:	( ('a'..'z')|('A'..'Z') |('_')|('-'))  ( ('a'..'z')|('A'..'Z')|('0'..'9') |('_')|('-') )+;
