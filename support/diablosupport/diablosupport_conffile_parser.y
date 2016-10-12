%{
#include <diablosupport.h>
#include "diablosupport_conffile_parser_extra.h"
%}
/* The parser for linker descriptions. See kernel/diablo_linkers.c for more
 * info on linker descriptions */

/* Token definitions */
%token VARIABLE
%token OPER_IS

/* Datatypes used in the parser */
%union 
{
   char * string;
}

/* Type rules */

%type <string> VARIABLE
%{
/*#include <diablosupport.h>*/
extern int confline;
extern int yylex (void);
void yyerror(const char * in)
{
  printf("At line %d: Parse error: %s\n",confline,in);
}
%}
%%

conffile: /* Empty */
	| conffile assignment {}
	;

assignment: VARIABLE OPER_IS VARIABLE { ConfValueSet($1,$3);  }
	  ;

%%

