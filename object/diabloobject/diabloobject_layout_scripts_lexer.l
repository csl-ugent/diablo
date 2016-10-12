%option prefix="lparse"
%{
#include <stdlib.h>
#include <ctype.h>
#include <diablosupport.h>
#include <diabloobject.h>
#include "diabloobject_layout_scripts_parser_extra.h"
#include "diabloobject_layout_scripts_parser.h"

/* #define DEBUG_LEXER */
#ifdef DEBUG_LEXER
#define RNUMBER()	do {							\
				yylval.longlong = StringToUint64(yytext,0);	\
				printf("NUM(%llx)\n",yylval.longlong);		\
				fflush(stdout);					\
				return NUMBER;					\
			} while (0)
#define RIDENT()	do {							\
				yylval.string = StringDup(yytext);		\
				printf("IDENT(%s)\n",yylval.string);		\
				fflush(stdout);					\
				return IDENTIFIER;				\
			} while (0)
#define RTOKEN(tok)	do {							\
				printf("TOKEN(%d)",tok);			\
				if (isprint(tok))				\
					printf(" -> %c\n",tok);			\
				else						\
					printf("\n");				\
				fflush(stdout);					\
				return (tok);					\
			} while (0)
#define RWILDCARD()	do {							\
				yylval.string = ExtractPattern(yytext);		\
				printf("WILDCARD %s\n", yylval.string);		\
				fflush(stdout);					\
				return WILDCARD;				\
			} while (0)
#else
#define RNUMBER()	do {							\
				yylval.longlong = StringToUint64(yytext,0);	\
				return NUMBER;					\
			} while (0)
#define RIDENT()	do {							\
				yylval.string = StringDup(yytext);		\
				return IDENTIFIER;				\
			} while (0)
#define RTOKEN(tok)	do {							\
				return (tok);					\
			} while (0)
#define RWILDCARD()	do {							\
				yylval.string = ExtractPattern(yytext);		\
				return WILDCARD;				\
			} while (0)
#endif

int yywrap(void) { return 1; }

int layoutlineno=1;
static int nestingdepth=0;
static enum {section,subsection} prev_state;
static int in_overlay=0;

t_string ExtractPattern(t_string in);
%}

Identifier	[a-zA-Z_.][a-zA-Z0-9_.$-]*
Decimal		[1-9][0-9]*
Octal		0[0-7]*
Hexadecimal	0(x|X)[0-9a-fA-F]+
LiteralTokens	\+|-|\*|\/|\&|\||\(|\)|;|~|:|=|\"

%s COMMENT
%s SECTION
%s SUBSECTION
%s SUBEXP
%option nounput
%%

<INITIAL>^SECTIONS	{ BEGIN(SECTION); RTOKEN(SECTIONS); }
<INITIAL>.		{ /* discard */ }
<INITIAL>\n		{ layoutlineno++; /* printf("l: %3d\n",layoutlineno); */ }

<SECTION>"/*"		{ BEGIN(COMMENT); prev_state = section; }
<SECTION,COMMENT,SUBSECTION,SUBEXP>\n	{ layoutlineno++; }
<SECTION,SUBSECTION,SUBEXP>[[:blank:]]	{ /* discard */ }

<SECTION>ALIGN		{ RTOKEN(ALIGN); }
<SECTION>PROVIDE	{ RTOKEN(PROVIDE); }
<SECTION>AT		{ RTOKEN(AT); }
<SECTION>ADDR		{ RTOKEN(ADDR); }
<SECTION>\/DISCARD\/	{ RIDENT(); }
<SECTION>SEGMENT_START	{ RTOKEN(SEGMENT_START); }
<SECTION>SEGMENT_END	{ RTOKEN(SEGMENT_END); }
<SECTION>PUT_REMAINING_SECTIONS	{ RTOKEN(PUT_REMAINING_SECTIONS); }
<SECTION>OVERLAY	{ in_overlay = 1; RTOKEN(OVERLAY); }
<SECTION>WILDCARD[[:blank:]]*"("[[:blank:]]*([*]*{Identifier}*)+[[:blank:]]*")" {
	RWILDCARD();
}

<SECTION>{Identifier}	{ RIDENT(); }
<SECTION>{Octal}	{ RNUMBER(); }
<SECTION>{Decimal}	{ RNUMBER(); }
<SECTION>{Hexadecimal}	{ RNUMBER(); }
<SECTION>{LiteralTokens} { RTOKEN(yytext[0]); }
<SECTION>"{"		{ 
				nestingdepth++;
				if (!in_overlay && nestingdepth == 2) BEGIN(SUBSECTION);
				else if (in_overlay && nestingdepth == 3) BEGIN(SUBSECTION);
				RTOKEN('{');
			}
<SECTION>"}"		{
				nestingdepth--;
				if (in_overlay && nestingdepth == 1) in_overlay = 0;
				if (nestingdepth == 0) BEGIN(INITIAL);
				RTOKEN('}');
			}
<SECTION>.		{ ECHO; printf(" is unknown token at line %d\n", layoutlineno); exit(0); }

<SUBSECTION>\.[[:blank:]]*=[^;]*;	{ BEGIN(SUBEXP); yyless(0); }
<SUBSECTION>"/*"	{ BEGIN(COMMENT); prev_state = subsection; }
<SUBSECTION>"}"		{
				nestingdepth--;
				if ((!in_overlay && nestingdepth != 1)
           || (in_overlay && nestingdepth != 2)) {
					FATAL(("unbalanced braces at line %d",layoutlineno));
        }
				BEGIN(SECTION);
				RTOKEN('}');
			}
<SUBSECTION>"{"		{ FATAL(("Nesting too deep at line %d", layoutlineno)); }
<SUBSECTION>.		{ /* discard */ }

<SUBEXP>;		{ BEGIN(SUBSECTION); RTOKEN(';'); }
<SUBEXP>ALIGN		{ RTOKEN(ALIGN); }
<SUBEXP>ADDR		{ RTOKEN(ADDR); }
<SUBEXP>{Identifier}	{ RIDENT(); }
<SUBEXP>{Octal}		{ RNUMBER(); }
<SUBEXP>{Decimal}	{ RNUMBER(); }
<SUBEXP>{Hexadecimal}	{ RNUMBER(); }
<SUBEXP>{LiteralTokens} { RTOKEN(yytext[0]); }

<COMMENT>"*/"		{ 
				if (prev_state == section) 
					BEGIN(SECTION); 
				else if (prev_state == subsection)
					BEGIN(SUBSECTION);
				else
					FATAL(("Unknown return state from comment. line %d",layoutlineno));
			}	
<COMMENT>.		{ /* discard */ }




%%

t_string ExtractPattern(t_string in)
{
	char *begin;
	char *end;
	
	begin = in;
	while (*begin != '(')
		++begin;
	++begin;
	while (isspace(*begin))
		++begin;
	end = &(in[strlen(in)-2]);
	while (isspace(*end))
		--end;
	if (end < begin)
		return NULL;
	++end;
	*end = '\0';
	return StringDup(begin);
}

/* vim: set shiftwidth=2 foldmethod=marker: */
