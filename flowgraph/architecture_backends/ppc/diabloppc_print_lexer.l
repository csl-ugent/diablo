/*
 * Copyright (C) 2005, 2006 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * }}}
 * 
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

/*%option prefix="PrintParse"*/
%option outfile="lex.yy.c"
%{
#include <stdio.h>
#ifdef _MSC_VER
#include <stdlib.h>
#include <io.h>
#include "diablosupport_win32.h"
#endif

/* TODO: need to check for buffer overflow */
#define BUFLEN 100

#define yylval PrintParselval
#define yyerror PrintParseerror

#include "diabloppc_print_parser.h"

extern int token;
extern char *idx;
extern int len;
int yyerror (char *);

#define YY_INPUT(buf, result, max_size)                 \
        {                                               \
          result = snprintf(buf, max_size, idx);        \
          idx += result;                                \
          len -= result;                                \
        }
%}

%option noyywrap

%%

AND                   { token++; return AND; }
OR                    { token++; return OR; }
" "                     { }
$[A-Za-z_]+             { sprintf(yylval.string, yytext + 1); token++; return VAR; }
[0-9]+                  { yylval.number = strtol(yytext, NULL, 10); token++; return INT; }
-[0-9]+                 { yylval.number = strtol(yytext, NULL, 10); token++; return INT; }
[A-Za-z0-9]+              { sprintf(yylval.string, yytext); token++; return TEXT; }
","                     { token++; return COMMA; }
"("                     { token++; return OPEN; }
")"                     { token++; return CLOSE; }
"-"                     { token++; return MINUS; }
"."                     { token++; return CONCAT; }
"{"                     { token++; return GSTART; }
"}"                     { token++; return GEND; }

"$$"                    { sprintf(yylval.string, "."); token++; return TEXT; }

"=="                    { token++; return ISEQ; }
"!="                    { token++; return ISNEQ; }
"<"                     { token++; return ISLES; }
"<="                    { token++; return ISLESEQ; }
">"                     { token++; return ISGR; }
">="                    { token++; return ISGREQ; }

"?"                     { token++; return IFEVAL; }
":"                     { token++; return IFELSE; }

.                       { yyerror(yytext); }

%%

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
