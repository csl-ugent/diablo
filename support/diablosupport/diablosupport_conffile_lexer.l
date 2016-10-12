%option prefix="ConfParser"
%option nounput
%{
#include <diablosupport.h>
#include <diablosupport_conffile_parser_extra.h>
#include <diablosupport_conffile_parser.h>

extern t_hash_table * conf_hash;
t_string long_string;
int confline=1;

void conferror(char * mesg)
{
  FATAL(("Lexer error at line %d in file: %s",confline,mesg));
}



%}
%x COMMENT
%x STRING
%%

"#"			{ BEGIN(COMMENT); }
<COMMENT>"\n"		{ BEGIN(INITIAL); confline++; }
<COMMENT>.		{ }
"\""			{  long_string=StringDup(""); BEGIN(STRING); }
<STRING>[^\"\n\\]*	{ t_string tmp=long_string; long_string=StringConcat2(tmp,yytext); Free(tmp); }
<STRING>"\\\""		{ t_string tmp=long_string; long_string=StringConcat2(tmp,"\""); Free(tmp); }
<STRING>"\\"		{ t_string tmp=long_string; long_string=StringConcat2(tmp,"\\"); Free(tmp); }
<STRING>"\""		{ ConfParserlval.string=long_string;BEGIN(INITIAL); return VARIABLE; }
<STRING>"\n"		{ conferror("Newline in string variable"); }
" "			{ } 
[^\ "=\n#][^\ "=\n]*	{ ConfParserlval.string=StringDup(yytext); return VARIABLE; }
"="			{ return OPER_IS; }
"\n"			{ confline++; }

%%
int yywrap()
{
  return 1;
}
