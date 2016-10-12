%option nounput
%{
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#if STAGE>0
#include "opt_gen_parser.h"
#else
#include "opt_gen_stage0_parser.h"
#endif

#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT
#endif

char * buf=NULL;
int buflen=0;
int line=1;
int stringtel=0;
static int yywrap(void) { return 1; }
void yyerror(const char *);
void checkbuf()
{
   if (stringtel>=buflen)
   {
      buflen+=10; 
      buf=(char *) realloc(buf,buflen);
   }
}
%}
%x COMMENT
%x COMMENT_TILL_EOL
%x STRING 

%%
%			{ BEGIN(COMMENT_TILL_EOL); }
#			{ BEGIN(COMMENT_TILL_EOL); }
"//"			{ BEGIN(COMMENT_TILL_EOL); }
"/*"			{ BEGIN(COMMENT); }
"string_option"		{ yylval.option_type=OPTION_TYPE_STRING; return OPTION_TYPE; }
"string_array_option"		{ yylval.option_type=OPTION_TYPE_STRING_ARRAY; return OPTION_TYPE; }
"file_option"		{ yylval.option_type=OPTION_TYPE_FILE; return OPTION_TYPE; }
"bool_option"           { yylval.option_type=OPTION_TYPE_BOOL; return OPTION_TYPE; }
"int_option"            { yylval.option_type=OPTION_TYPE_INT; return OPTION_TYPE; } 
"path_option"           { yylval.option_type=OPTION_TYPE_PATH; return OPTION_TYPE; }
"count_option"          { yylval.option_type=OPTION_TYPE_COUNT; return OPTION_TYPE; }
"usage_option"          { yylval.option_type=OPTION_TYPE_USAGE; return OPTION_TYPE; }
"version_option"        { yylval.option_type=OPTION_TYPE_VERSION; return OPTION_TYPE; }
"ifset"			{ return IFSET; }
"ifnotset"		{ return IFNOTSET; }
"if"			{ return IF; }
"ifnot"		{ return IFNOT; }
"default"               { return OPTION_DEFAULT; }
"short"                 { return OPTION_SHORT; }
"long"                  { return OPTION_LONG; }
"description"           { return OPTION_DESC; }
"desc"                  { return OPTION_DESC; }
"environment"           { return OPTION_ENV; }
"required"              { return OPTION_REQUIRED; }
"forbidden"             { return OPTION_FORBIDDEN; }
"requiredtrue"              { return OPTION_REQUIREDTRUE; }
"forbiddentrue"             { return OPTION_FORBIDDENTRUE; }
"hidden"                { return OPTION_HIDDEN; }
"group"                 { return OPTION_GROUP; }
"env"                   { return OPTION_ENV; }
"on"                    { yylval.boolean=true; return RET_BOOL; }
"off"                   { yylval.boolean=false; return RET_BOOL; }
-0x[0-9a-fA-F]*		{ yylval.number=strtol(yytext,NULL,16); return RET_INT; }
0x[0-9a-fA-F]*		{ yylval.number=strtol(yytext,NULL,16); return RET_INT; }
[0-9]*			{ yylval.number=strtol(yytext,NULL,10); return RET_INT; }
-[0-9]*			{ yylval.number=strtol(yytext,NULL,10); return RET_INT; }
"||"			{ return OR; }
"=="			{ return ISEQ; }
"!="			{ return ISNEQ; }
"&&"			{ return AND; }
"{"			{ return OPEN_C;  }
"}"			{ return CLOSE_C;  }
"("			{ return OPEN_R;  }
")"			{ return CLOSE_R;  }
"="			{ return IS;  }
";"			{ /* Currently ignored */  }
"\""			{ stringtel=1; checkbuf(); buf[0]=0; BEGIN(STRING);  }
[a-zA-Z0-9_]*		{ yylval.string=strdup(yytext); return NAME;  }
[ \t]			{ /* We skip whitespace */ }
\n			{ line++; }
.			{ yyerror("unmatch char"); } 
<STRING>"\\\""		{ stringtel++; checkbuf(); buf[stringtel-2]='"'; buf[stringtel-1]=0;}
<STRING>"\""		{ yylval.string=strdup(buf); BEGIN(0); return RET_STRING; }
<STRING>.		{ stringtel++; checkbuf(); buf[stringtel-2]=yytext[0]; buf[stringtel-1]=0;}
<COMMENT>"*/"           { BEGIN(0); }
<COMMENT>.              { /* Ignore chars in comment block */ }
<COMMENT>\n             { line++; }
<COMMENT_TILL_EOL>\n    { line++; BEGIN(0); }
<COMMENT_TILL_EOL>.     { /* Ignore chars in comment line */ }

%%
