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

%{
#include <stdio.h>
#include <string.h>
#include <diablospe.h>

extern int yyerror(char *);
extern int yylex (void);

/* TODO: need to check for buffer overflow */
#define BUFLEN 100

char *res;
char buffer[BUFLEN];
t_spe_ins *ins;
int token;
char *idx, *orig;
int len;

%}

/* Token Definitions */
%token VAR
%token TEXT
%token COMMA
%token CONCAT
%token OPEN
%token CLOSE

/* Datatypes used in the parser */
%union
{
  int number;
  char string[100];
}

/* Type rules */
%type <string> VAR TEXT expr leaf var text print 

/* Associativity information */
%left CONCAT
%left COMMA
%left BCOND

%start print

%%

print: text                             { sprintf(res, $1); }

text: expr text                         { sprintf($$, "%s %s", $1, $2); }
    | expr COMMA text                   { sprintf($$, "%s,%s", $1, $3); }
    | expr CONCAT text                  { sprintf($$, "%s%s", $1, $3); }
    | expr                              { sprintf($$, $1); }

expr: leaf                              { sprintf($$, $1); }

leaf: TEXT                              { sprintf($$, $1); }
    | var OPEN var CLOSE                { sprintf($$, "%s(%s)", $1, $3); }
    | var                               { sprintf($$, $1); }

var: VAR                                {
                                          /* Registers are printed like what
                                           * objdump gives: $<regnum> */
                                                 if (!strcmp ($1, "RA")) {
                                            sprintf ($$, "$%d", SPE_INS_REGA (ins));
                                          } else if (!strcmp ($1, "RB")) {
                                            sprintf ($$, "$%d", SPE_INS_REGB (ins));
                                          } else if (!strcmp ($1, "RC")) {
                                            sprintf ($$, "$%d", SPE_INS_REGC (ins));
                                          } else if (!strcmp ($1, "RT")) {
                                            sprintf ($$, "$%d", SPE_INS_REGT (ins));
                                          } else if (!strcmp ($1, "IMM")) {
                                            sprintf ($$, "%d", AddressExtractInt32 (SPE_INS_IMMEDIATE (ins)));
                                          } else if (!strcmp ($1, "IMMx")) {
                                            sprintf ($$, "%x", AddressExtractInt32 (SPE_INS_IMMEDIATE (ins)));
                                          } else if (!strcmp ($1, "ADDR")) {
                                            sprintf ($$, "%d", AddressExtractInt32 (SPE_INS_ADDRESS (ins)));
                                          } else if (!strcmp ($1, "ADDRx")) {
                                            sprintf ($$, "%x", AddressExtractInt32 (SPE_INS_ADDRESS (ins)));
                                          } else if (!strcmp ($1, "OPC")) {
                                            sprintf ($$, "%s", spe_opcode_table[SPE_INS_OPCODE (ins)].name);
                                          } else {
                                             yyerror ($1);
                                          }
   }

%%

int
SpePrintParseerror (char *var)
{
  if (!strcmp(var, "syntax error"))
  {
    FATAL(("Syntax error near token %d on '%s'. Var: %s\n", token, orig, var));
  }
  else
  {
    FATAL(("Unknown symbol %s\n", var));
  }
}

void
SpePrintParse (char *line, t_spe_ins *instr, char *buf)
{
  idx = orig = line;
  len = strlen(line);
  res = buf;
  ins = instr;
  token = 1;
  yyparse();
}



/* vim:set ts=4 sw=4 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
