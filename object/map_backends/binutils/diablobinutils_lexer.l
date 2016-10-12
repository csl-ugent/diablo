%option prefix="ParseMapBinutils"

%{
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <diablobinutils.h>
#include "diablobinutils_parser_extra.h"
#include "diablobinutils_parser.h"
#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT
#endif
long line=1;
static int ParseMapBinutilswrap(void) { return 1; }
static int opencnt=0; 

void yyerror(char *);
%}
Name 		[0-9a-zA-Z_/\-\.()<>@+:~][0-9a-zA-Z_/\-\.()<>@+:\$~]*
FileName	[0-9a-zA-Z_\./\-]*

HexNumber	"0"[xX][0-9a-fA-F]+
OctNumber	"0"[0-7]*
DecNumber	[1-9][0-9]*

FltExponent   	[eE][-+]?[0-9]+
FltFract	([0-9]*"."[0-9]+)|([0-9]+".")
FltNumber	(({FltFract}{FltExponent}?)|([0-9]+{FltExponent}))[FfLl]?
Char            '(\\.|[^\\'\n])+'	
Regexp		[^ \n)(]*
%x PATTERN 
%x MEMBERS 
%x COMMONS 
%x MEMORY 
%x XREF
%x MAP
%x SKIP
%%
"Archive"[ ]*"member"[ ]*"included"[ ]*"because"[ ]*"of"[ ]*"file"[ ]*"("[ ]*"symbol"[ ]*")" { BEGIN(MEMBERS); }
"Memory"[ ]*"Configuration"[ ]* 		{ BEGIN(MEMORY); }
"Discarded"[ ]*"input"[ ]*"sections"[ ]*	{ BEGIN(MEMBERS); }
<MEMBERS>"Allocating"[ ]*"common"[ ]*"symbols" 	{ BEGIN(COMMONS); }
<MEMBERS>"Memory"[ ]*"Configuration"[ ]* 	{ BEGIN(MEMORY); }
<MEMBERS>.					{ }
<MEMBERS>\n					{ line++; }
<COMMONS>"Memory"[ ]*"Configuration"[ ]* 	{ BEGIN(MEMORY); }
<COMMONS>.					{ }
<COMMONS>\n					{ line++; }
<MEMORY>"Linker"[ ]*"script"[ ]*"and"[ ]*"memory"[ ]*"map"[ ]* { BEGIN(MAP); }
<MEMORY>.					{ }
<MEMORY>\n					{ line++; }
<PATTERN>"("					{ opencnt++; }
<PATTERN>")"					{ if (opencnt==0) { BEGIN(MAP); return T_PATTERN;} else opencnt--; }
<PATTERN>{Regexp}				{ }
<PATTERN>" "					{ } 
<MAP>^"OUTPUT(".*$ { }
<MAP>^"END GROUP"$ { }
<MAP>^"START GROUP"$ { }
<MAP>^"*fill*".*$ { }
<MAP>^" *fill*".*$ { }
<MAP>^.*("LOAD linker stubs"|"CTORS"|"="|"+SIZEOF_HEADERS"|"SIZEOF ("|"PROVIDE ("|"ALIGN ("|".mdebug.abi32"|"__DYNAMIC"|"<code "|"__global"|". +"|"size before relaxing"|"EXCLUDE_FILE("|"Address of section").*$ { }
<MAP>^" "{Name}.*0x00000000[ ]*0x0[ ].*$	{}
<MAP>^"LOAD"					{ return T_LOAD; }
<MAP>^" "{FileName}"*"[^(\n]*"("		{ opencnt=0; BEGIN(PATTERN);}
<SKIP>[\n]					{ BEGIN(MAP); }
<SKIP>.						{ }

<MAP>"Cross"[ ]*"Reference"[ ]*"Table"[ ]* 	{ BEGIN(XREF); }
<MAP>"linker stubs"				{ ParseMapBinutilslval.string=StringDup("Linker"); return T_STRING; }
<MAP>"load address"				{  return T_LOAD_ADDRESS; }
<MAP>{HexNumber}				{ long long tmp; sscanf(ParseMapBinutilstext,"0x%llx",&tmp);  ParseMapBinutilslval.ll.num1=tmp&((1LL<<32)-1); ParseMapBinutilslval.ll.num2=tmp>>32;  return T_HEX; }
<MAP>{OctNumber}				{ }
<MAP>{DecNumber}				{ }
<MAP>^{Name}					{ ParseMapBinutilslval.string=StringDup(ParseMapBinutilstext); return T_LEADING_STRING; }
<MAP>^" "{Name}					{ ParseMapBinutilslval.string=StringDup(ParseMapBinutilstext+1);  return T_SECTION_NAME; }
<MAP>"builtin"					{ return T_BUILTIN; }
<MAP>[0-9a-zA-Z_/\-\.<>@+:][0-9a-zA-Z_/\-\.<>@+:$]*"(".*")"$	{ ParseMapBinutilslval.string=StringDup(ParseMapBinutilstext);  return T_STRING; }
<MAP>[0-9a-zA-Z_/\-\.<>@+:][0-9a-zA-Z_/\-\.<>@+:$]*"<".*">""(".*")"$	{ ParseMapBinutilslval.string=StringDup(ParseMapBinutilstext);  return T_STRING; }
<MAP>{Name}					{ ParseMapBinutilslval.string=StringDup(ParseMapBinutilstext);  return T_STRING; }
<MAP>[ ]					{ }
<MAP>[\n]					{ line++; }
<MAP>"="					{ return T_IS; }
<MAP>","					{ }
<MAP>"&"					{ }
<MAP>.						{printf("Unmatched %s\n",ParseMapBinutilstext); yyerror("unmatched character");}
.						{ }
<MAP><<EOF>>					{ BEGIN(0);}
<XREF>.						{ }
<XREF>\n					{ line++; }
<XREF><<EOF>>					{ BEGIN(0);}
[\n]						{ line++; }
%%
