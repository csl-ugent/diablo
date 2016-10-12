%option prefix="ParseMapTiLinker"

%{
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <diablotilinker.h>
#include "diablotilinker_parser_extra.h"
#include "diablotilinker_parser.h"
#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT
#endif
long tilinker_map_lexer_line=1;
static int ParseMapTiLinkerwrap(void) { return 1; }

void yyerror(char *);
%}
Name 		[0-9a-zA-Z_/\-\.()<>@+:$][0-9a-zA-Z_/\-\.()<>@+:$]*

HexNumber	[0-9a-fA-F]+

FltExponent   	[eE][-+]?[0-9]+
FltFract	([0-9]*"."[0-9]+)|([0-9]+".")
FltNumber	(({FltFract}{FltExponent}?)|([0-9]+{FltExponent}))[FfLl]?
Char            '(\\.|[^\\'\n])+'	
Regexp		[^ \n)(]*
%x SYMBOLS
%x MAP
%%
"SECTION"[ ]*"ALLOCATION"[ ]*"MAP"[ ]*  { BEGIN(MAP); }
. 					{ printf("Skip: %s\n",ParseMapTiLinkertext); }
\n					{ tilinker_map_lexer_line++; }
<MAP>\r					{ }
<MAP>^"GLOBAL SYMBOLS:".*		{ BEGIN(SYMBOLS); }
<MAP>^.*("--HOLE--"|"-------").*$       { }
<MAP>^.*output.*attributes.*$           { }
<MAP>^.*section.*page.*origin.*$        { }
<MAP>"UNINITIALIZED"                    { }
<MAP>"[fill".*"]"			{ } 

<MAP>{HexNumber}				{ long long tmp; sscanf(ParseMapTiLinkertext,"%llx",&tmp);  ParseMapTiLinkerlval.ll.num1=tmp&((1LL<<32)-1); ParseMapTiLinkerlval.ll.num2=tmp>>32;  printf("HEX = %lx\n", ParseMapTiLinkerlval.ll.num2); return T_HEX; }
<MAP>" : "					{ return T_LIB; }
<MAP>^{Name}					{ printf("NAME: %s\n",ParseMapTiLinkertext); ParseMapTiLinkerlval.string=StringDup(ParseMapTiLinkertext); return T_LEADING_STRING; }
<MAP>{Name}					{ ParseMapTiLinkerlval.string=StringDup(ParseMapTiLinkertext); printf("NAME: %s\n",ParseMapTiLinkertext); return T_STRING; }
<MAP>[ ]					{ }
<MAP>[\n]					{ tilinker_map_lexer_line++; }
<MAP>^"*"					{ }
<MAP>.						{printf("Unmatched ========\"%d\"===========\n",ParseMapTiLinkertext[0]); yyerror("unmatched character");}
<MAP><<EOF>>					{ BEGIN(0);}

<SYMBOLS>.					{ }
<SYMBOLS>\n					{ tilinker_map_lexer_line++; }
<SYMBOLS><<EOF>>				{ BEGIN(0); }
%%
