%{
#define YYERROR_VERBOSE
#include "diabloobject_linker_scripts_parser_extra.h"
#include <diabloobject.h>
%}
/* The parser for linker descriptions. See kernel/diablo_linkers.c for more
 * info on linker descriptions */

/* Token definitions */
%token LD_VARIABLE_DEFINITION
%token LD_VARIABLE_STRING
%token LD_RULE_NAME
%token LD_ACTION
%token LD_TRIGGER 
%token LD_LINKER_SCRIPT_SECTION 
%token LD_ADDRESS 
%token LD_SYMBOL
%token LD_NAME
%token LD_FUNCTION_NAME
%token LD_OPEN_B
%token LD_CLOSE_B
%token LD_BODY_STRING
%token LD_EQUAL
%token LD_NOT_EQUAL


/* Associativity information */
%left LD_COMMA
%left LD_AND
%left LD_OR
%left LD_ADD 
%left LD_SUB 
%left LD_DIV
%left LD_EQUAL
%left LD_NOT_EQUAL
%left LD_SHIFT_RIGHT
%left LD_SHIFT_LEFT
%left LD_BITWISE_AND
%nonassoc LD_NOT

/* Datatypes used in the parser */
%union 
{
   int number;
   char * string;
   struct {
     char * string;
     int type;
     void * args;
   } arg_and_type;
   void * args;
   t_ast_node * function;
   void * rule;
}

/* Type rules */
%type <string> LD_VARIABLE_DEFINITION
%type <string> LD_VARIABLE_STRING
%type <string> LD_BODY_STRING
%type <string> LD_RULE_NAME
%type <string> LD_NAME
%type <string> LD_FUNCTION_NAME
%type <arg_and_type> argument
%type <args> non_empty_arguments
%type <args> arguments
%type <function> statement_value
%type <function> comma_separated_statement_value
%type <rule> rule_body 

%{
extern int linker_scripts_lexer_line;
void yyerror(t_const_string in)
{
  FATAL(("At line %d: Parse error: %s",linker_scripts_lexer_line,in));
}
int LinkerScriptParserlex();
%}
%%
linker_file: /* Empty */
		 | linker_file linker_statement { }
		 ;

linker_statement: linker_rule {}
		| linker_definition { /* Definitions are currently ignored */ }
		;

linker_rule: LD_RULE_NAME rule_body { LinkerRuleExecute($2,$1); }
	   ;

rule_body: /* Empty */ 			     		     { $$=LinkerRuleNew();                 }
	 | rule_body LD_ACTION comma_separated_statement_value  { ((t_linker_rule *) $$)->action=$3;  }
	 | rule_body LD_TRIGGER comma_separated_statement_value { ((t_linker_rule *) $$)->trigger=$3; }
	 | rule_body LD_LINKER_SCRIPT_SECTION comma_separated_statement_value { ((t_linker_rule *) $$)->section=$3; }
	 | rule_body LD_ADDRESS comma_separated_statement_value { 
	 	if (emulate_link)
			((t_linker_rule *) $$)->address=$3;
		else
		{
			t_ast_successors *args = NULL;
			args = AstSuccessorsNew();
			AstSuccessorsAddSuccessor(args, StringDup("0x0"), TYPE_OTHER, NULL);
			/*AstSuccessorsAddSuccessor(args, (char *) 0x0, TYPE_NUMERIC, NULL);*/
			((t_linker_rule *) $$)->address = AstNodeFunctionNew (StringDup("ABS"), args);
			AstNodeFree ($3);
		}
	}
	 | rule_body LD_SYMBOL comma_separated_statement_value  { ((t_linker_rule *) $$)->symbol=$3;  }
	 ;

comma_separated_statement_value: comma_separated_statement_value  LD_COMMA statement_value { $$=AstNodeOperatorNew(OPER_COMMA,$1,$3); }
			       | statement_value 					{ $$=$1;                                  }
			       ;

statement_value: statement_value LD_AND statement_value   	{ $$=AstNodeOperatorNew(OPER_AND,$1,$3); }
	       | statement_value LD_OR statement_value    	{ $$=AstNodeOperatorNew(OPER_OR,$1,$3);  }
	       | statement_value LD_ADD statement_value    { $$=AstNodeOperatorNew(OPER_ADD,$1,$3); }
	       | statement_value LD_SUB statement_value    { $$=AstNodeOperatorNew(OPER_SUB,$1,$3); }
	       | statement_value LD_DIV statement_value    { $$=AstNodeOperatorNew(OPER_DIV,$1,$3); }
	       | statement_value LD_EQUAL statement_value    { $$=AstNodeOperatorNew(OPER_EQUAL,$1,$3); }
	       | statement_value LD_NOT_EQUAL statement_value    { $$=AstNodeOperatorNew(OPER_NOT_EQUAL,$1,$3); }
	       | statement_value LD_SHIFT_RIGHT statement_value    { $$=AstNodeOperatorNew(OPER_SHIFT_RIGHT,$1,$3); }
	       | statement_value LD_SHIFT_LEFT statement_value    { $$=AstNodeOperatorNew(OPER_SHIFT_LEFT,$1,$3); }
	       | statement_value LD_BITWISE_AND statement_value    { $$=AstNodeOperatorNew(OPER_BITWISE_AND,$1,$3); }
	       | LD_NOT statement_value 			{ $$=AstNodeOperatorNew(OPER_NOT,$2,NULL); }
	       | LD_OPEN_B statement_value LD_CLOSE_B        	{ $$=$2;                                }
	       | LD_FUNCTION_NAME LD_OPEN_B arguments LD_CLOSE_B { $$=AstNodeFunctionNew($1,$3);         }
	       | LD_NAME	{ $$ = Malloc (sizeof (t_ast_node)); $$->data.name=$1; $$->type=TYPE_OTHER; $$->args=NULL; }
	       ;
	
arguments: non_empty_arguments { $$=$1;} 
	 | /* Empty */ 	       { $$=NULL; }
	 ;

non_empty_arguments: non_empty_arguments LD_COMMA argument { $$=$1; AstSuccessorsAddSuccessor($$,$3.string, $3.type,$3.args); }
		   | argument 				{ $$=AstSuccessorsNew(); AstSuccessorsAddSuccessor($$,$1.string, $1.type, $1.args); }
		   ;

argument: LD_BODY_STRING {$$.string=$1; $$.type=TYPE_STRING; $$.args=NULL; }
	| statement_value { if ($1->type==TYPE_FUNCTION) { $$.string=$1->data.name; $$.type=TYPE_FUNCTION; $$.args=$1->args; Free($1); } else if ($1->type==TYPE_OPER) { $$.type=TYPE_OPER; /* Actually, this is oper, but abuse string field... */ $$.string=$1->data.name;  $$.args=$1->args; Free($1);   } else if ($1->type==TYPE_OTHER) { $$.type=TYPE_OTHER; $$.string=$1->data.name;  $$.args=NULL; Free($1);  } else FATAL(("implement non-function argument astnodes"));  }
	;

linker_definition: LD_VARIABLE_DEFINITION LD_VARIABLE_STRING { Free($1); Free($2); }
		 ;
%%

