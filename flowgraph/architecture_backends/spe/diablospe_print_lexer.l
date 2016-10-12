/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
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
 * This file is part of the SPE port of Diablo (Diablo is a better
 * link-time optimizer)
 */

/* This code has been taken from the print parser of the Ppc32 port. */

%option prefix="SpePrintParse"
%option outfile="lex.yy.c"
%{

#include <stdio.h>

#include "diablospe_print_parser.h"

/* TODO: need to check for buffer overflow */
#define BUFLEN 100

#define yylval  SpePrintParselval
//#define yyerror SpePrintParseerror
//extern int yyerror(char *);

int SpePrintParseerror (char *var);

extern int token;
extern char *idx;
extern int len;

#define YY_INPUT(buf, result, max_size)                 \
        {                                               \
          result = snprintf(buf, max_size, idx);        \
          idx += result;                                \
          len -= result;                                \
        }
%}

%option noyywrap

%%

" "                     { }
$[A-Za-z_]+             { sprintf(yylval.string, yytext + 1); token++; return VAR; }
[A-Za-z0-9]+              { sprintf(yylval.string, yytext); token++; return TEXT; }
","                     { token++; return COMMA; }
"("                     { token++; return OPEN; }
")"                     { token++; return CLOSE; }
"."                     { token++; return CONCAT; }

"$$"                    { sprintf(yylval.string, "."); token++; return TEXT; }

.                       { SpePrintParseerror(yytext); }

%%

/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
