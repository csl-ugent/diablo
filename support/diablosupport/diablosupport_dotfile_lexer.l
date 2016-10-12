%option prefix="DotParser"
%option nounput
%{
#include <diablosupport.h>
#include <diablosupport_dotfile_parser_extra.h>
#include <diablosupport_dotfile_parser.h>

#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT
#endif

static int maxidlen=0;
static char * string_buffer=NULL;

int string_pos=0;
int dot_lexer_line=1;
int props=0;


void LexError(char * err)
{
printf("Lexer error at line %d: %s\n",dot_lexer_line,err);
exit(0);
}

void StringBufferGrow()
{
maxidlen+=100;
string_buffer=Realloc(string_buffer,maxidlen);
}

%}
%x QUOTED_ID
%x PROPS 
%%
"\n"                                  { dot_lexer_line++; }
"\""				      { BEGIN(QUOTED_ID); }
<QUOTED_ID>"\\\""		      { 
	  				if (string_pos>maxidlen-2) StringBufferGrow();
					string_buffer[string_pos++]='"';
				      }
<QUOTED_ID>"\\\n"		      { 
					if (string_pos>maxidlen-2) StringBufferGrow(); 
					dot_lexer_line++; 
				      }
<QUOTED_ID>"\""			      { 
					if (string_pos>maxidlen-1) StringBufferGrow();
					if (props)
					{
					   BEGIN(PROPS);
					}
				        else
					{
					   BEGIN(0); 
					}
                                        string_buffer[string_pos]=0; 
					string_pos=0;
					yylval.string=StringDup(string_buffer);
					return T_ID;
			              }
<QUOTED_ID>.			      { 
					if (string_pos>maxidlen-2) StringBufferGrow();
                                        string_buffer[string_pos++]=yytext[0];
				      }
<QUOTED_ID>"\n"			      { LexError("Un-escaped newline in string");}

[ \t]				      { }
"digraph"[ \t]*"\""		      { BEGIN(QUOTED_ID);return DIGRAPH; }
"digraph"[ \t]*		   	      { return DIGRAPH; }
"{"				      { return OPEN_C; }
"graph"			              { return DOT_PARSER_GRAPH; }
"subgraph"			      { return DOT_PARSER_SUBGRAPH; }
"node"			              { return DOT_PARSER_NODE;}
"edge"			              { return DOT_PARSER_EDGE;}
"}"				      { return CLOSE_C; }
"["				      { BEGIN(PROPS); props++; return OPEN_S; }
";"				      { }
"->"				      { return DOT_PARSER_EDGE;}
[a-zA-Z_][a-zA-Z0-9_]*                { /* Alpha numeric string, not beginning with a number */ yylval.string=StringDup(yytext); return T_ID; }
[0-9][0-9]*                	      { /* A number */ yylval.string=StringDup(yytext); return T_ID; }

<PROPS>"]"		              { props--; if (!props) BEGIN(0); return CLOSE_S; }
<PROPS>"\""		  	      { BEGIN(QUOTED_ID); }
<PROPS>"label"[ \t]*=		      { return LABEL; }
<PROPS>"shape"[ \t]*=		      { return SHAPE;}
<PROPS>"bb"[ \t]*=		      { return BB;}
<PROPS>"pos"[ \t]*=		      { return POS; }
<PROPS>"width"[ \t]*=		      { return WIDTH; }
<PROPS>"height"[ \t]*=		      { return HEIGHT; }
<PROPS>"style"[ \t]*=		      { return STYLE;}
<PROPS>"color"[ \t]*=		      { return COLOR;}
<PROPS>"fillcolor"[ \t]*=	      { return FILLCOLOR;}
<PROPS>"lp"[ \t]*=	      	      { return LP;}
<PROPS>","			      { }
<PROPS>"\n"                           { dot_lexer_line++; }
<PROPS>[ \t]                          { } 
<PROPS>[a-zA-Z0-9_][a-zA-Z0-9_]*      { yylval.string=StringDup(yytext); return T_ID; }
<PROPS>.			      { printf("%s\n",yytext); LexError("Unmatched char in props\n");  }
.				      { printf("=%s= %d\n",yytext,yytext[0]); LexError("Unmatched char\n");  }
%%
int yywrap()
{
  return 1;
}
