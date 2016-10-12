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

%{
#include <stdio.h>
#include <string.h>
#include <diabloppc.h>

extern int yyerror(char *);
extern int yylex (void);
/* TODO: need to check for buffer overflow */

#define BUFLEN 100

char *res;
char buffer[BUFLEN];
t_ppc_ins *ins;
int token;
char *idx, *orig;
int len;

void PpcPrintBranchConditional(char *buffer, char *to, t_ppc_ins *ins);
void PpcPrintRotateLeft(char *buffer, t_ppc_ins *ins);
void PpcPrintCondition(char *buffer, t_ppc_ins *ins, t_bool cond);
void PpcPrintConditionReg(int bit, char *buffer);

/* TODO: define this functions correctly on diabloppc_utils.c,
   depending on the instruction the immedate operand is encoded
   in a different way. */

/** Redefined as a regular function to keep the compiler happy */
t_int64 GetImmediate(t_ppc_ins * ins)
{
  t_address res;
  if((PPC_INS_OPCODE(ins)==PPC_BC)||(PPC_INS_OPCODE(ins)==PPC_B))
  {
    if(PPC_INS_FLAGS(ins) & PPC_FL_ABSOLUTE)
    {
      res = PPC_INS_IMMEDIATE(ins);
    }
    else
    {
      res = AddressAdd (PPC_INS_CADDRESS(ins), PPC_INS_IMMEDIATE(ins));
    }
  }
  else
  {
    if (PPC_INS_FLAGS(ins) & PPC_FL_SIGNED)
    {
      res = PPC_INS_IMMEDIATE(ins);
      if(AddressIsGe(res, AddressNullForIns (T_INS(ins))) 
	    && ((PPC_INS_FLAGS(ins) & PPC_FL_IMM_SHIFT16)==0))
      {
        res = AddressAnd(res, AddressNewForIns (T_INS(ins), (0x0000ffff)));
        res = AddressSignExtend(res,15);
      }
    } 
    else
    {
      res = PPC_INS_IMMEDIATE(ins);
    }
    if (PPC_INS_FLAGS(ins) & PPC_FL_IMM_SHIFT16)
    {
      res = AddressShr (res, AddressNewForIns (T_INS(ins), 16));
      res = AddressAnd (res, AddressNewForIns (T_INS(ins), 0x0000ffff));
    }
  }
  
  return AddressExtractInt64SignExtend (res);
}

%}

/* Token Definitions */
%token VAR
%token TEXT
%token INT
%token COMMA
%token CONCAT
%token MINUS
%token ISEQ
%token ISNEQ
%token ISLES
%token ISLESEQ
%token ISGR
%token ISGREQ
%token IFEVAL
%token IFELSE
%token OPEN
%token CLOSE
%token GSTART
%token GEND
%token AND
%token OR

/* Datatypes used in the parser */
%union
{
  int number;
  char string[BUFLEN];
}

/* Type rules */
%type <string> VAR TEXT expr leaf var comp text print 
%type <number> INT bool val

/* Associativity information */
%nonassoc ISEQ ISNEQ ISLES ISLESEQ ISGR ISGREQ
%left CONCAT
%left COMMA
%left BCOND
%left IFEVAL
%left IFELSE
%left AND
%left OR

%start print

%%

print: text                             { sprintf(res, $1); }

text: expr text                         { sprintf($$, "%s %s", $1, $2); }
    | expr COMMA text                   { sprintf($$, "%s,%s", $1, $3); }
    | expr CONCAT text                  { sprintf($$, "%s%s", $1, $3); }
    | expr                              { sprintf($$, $1); }

expr: leaf                              { sprintf($$, $1); }
    | comp                              { sprintf($$, $1); }
    | GSTART text GEND                  { sprintf($$, $2); }

comp: bool IFEVAL text IFELSE text      { if ($1) { sprintf($$, $3); } else { sprintf($$, $5); } }
    | bool IFEVAL text                  { if ($1) { sprintf($$, $3); } else { sprintf($$, "%s", ""); } }

bool: val ISEQ    val                   { $$ = ($1 == $3); }
    | val ISNEQ   val                   { $$ = ($1 != $3); }
    | val ISLES   val                   { $$ = ($1 <  $3); }
    | val ISLESEQ val                   { $$ = ($1 <= $3); }
    | val ISGR    val                   { $$ = ($1 >  $3); }
    | bool AND    bool                  { $$ = ($1 && $3); }
    | bool OR     bool                  { $$ = ($1 || $3); }
    | val                               { $$ = $1; }
    | GSTART bool GEND                  { $$ = $2; }

val: INT                                { $$ = $1; }
   | VAR                                { if (!strcmp($1, "REGA")) {
                                            $$ = PPC_REG2NUM(PPC_INS_REGA(ins));
                                          } else if (!strcmp($1, "REGB")) {
                                            $$ = PPC_REG2NUM(PPC_INS_REGB(ins));
                                          } else if (!strcmp($1, "REGC")) {
                                            $$ = PPC_REG2NUM(PPC_INS_REGC(ins));
                                          } else if (!strcmp($1, "REGT")) {
                                            $$ = PPC_REG2NUM(PPC_INS_REGT(ins));
                                          } else if (!strcmp($1, "REGAi")) {
                                            $$ = PPC_INS_REGA(ins);
                                          } else if (!strcmp($1, "REGBi")) {
                                            $$ = PPC_INS_REGB(ins);
                                          } else if (!strcmp($1, "REGCi")) {
                                            $$ = PPC_INS_REGC(ins);
                                          } else if (!strcmp($1, "REGTi")) {
                                            $$ = PPC_INS_REGT(ins);
                                          } else if (!strcmp($1, "PPC_REG_CTR")) {
                                            $$ = PPC_REG_CTR;
                                          } else if (!strcmp($1, "PPC_REG_LR")) {
                                            $$ = PPC_REG_LR;
                                          } else if (!strcmp($1, "PPC_REG_XER")) {
                                            $$ = PPC_REG_XER;
                                          } else if (!strcmp($1, "IMM")) {
                                            $$ = GetImmediate (ins);
                                          } else if (!strcmp($1, "PPC_FL_LINK")) {
                                            $$ = PPC_INS_FLAGS(ins) & PPC_FL_LINK;
                                          } else if (!strcmp($1, "PPC_FL_ABSOLUTE")) {
                                            $$ = PPC_INS_FLAGS(ins) & PPC_FL_ABSOLUTE;
                                          } else if (!strcmp($1, "PPC_FL_RC")) {
                                            $$ = PPC_INS_FLAGS(ins) & PPC_FL_RC;
                                          } else if (!strcmp($1, "PPC_FL_OE")) {
                                            $$ = PPC_INS_FLAGS(ins) & PPC_FL_OE;
                                          } else if (!strcmp($1, "PPC_FL_L")) {
                                            $$ = PPC_INS_FLAGS(ins) & PPC_FL_L;
                                          } else if (!strcmp($1, "CT")) {
                                            $$ = PPC_INS_CT(ins);
                                          } else {
                                            yyerror($1);
                                          }
                                        }

leaf: TEXT                              { sprintf($$, $1); }
    | var OPEN var CLOSE                { sprintf($$, "%s(%s)", $1, $3); }
    | var                               { sprintf($$, $1); }

var: MINUS val                          { sprintf($$, "%d", -($2)); }
   | VAR                                { if (!strcmp($1, "REGA")) {
                                            sprintf($$, PpcRegisterName (PPC_INS_REGA(ins)));
                                          } else if (!strcmp($1, "REGB")) {
                                            sprintf($$, PpcRegisterName (PPC_INS_REGB(ins)));
                                          } else if (!strcmp($1, "REGC")) {
                                            sprintf($$, PpcRegisterName (PPC_INS_REGC(ins)));
                                          } else if (!strcmp($1, "REGT")) {
                                            sprintf($$, PpcRegisterName (PPC_INS_REGT(ins)));
                                          } else if (!strcmp($1, "BITA")) {
                                            PpcPrintConditionReg(PPC_INS_REGA(ins),buffer);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "BITB")) {
                                            PpcPrintConditionReg(PPC_INS_REGB(ins),buffer);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "BITC")) {
                                            PpcPrintConditionReg(PPC_INS_REGC(ins),buffer);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "BITT")) {
                                            PpcPrintConditionReg(PPC_INS_REGT(ins),buffer);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "IMM")) {
                                            sprintf($$, "%lld", GetImmediate (ins));
                                          } else if (!strcmp($1, "ADDR")) {
                                            sprintf($$, "0x%llx", GetImmediate (ins));
                                          } else if (!strcmp($1, "PPC_FL_L")) {
                                            sprintf($$, "%d",PPC_INS_FLAGS(ins) & PPC_FL_L);
                                          } else if (!strcmp($1, "BO")) {
                                            sprintf($$, "%d", PPC_INS_BO(ins));
                                          } else if (!strcmp($1, "CB")) {
                                            sprintf($$, "%d", PPC_INS_CB(ins));
                                          } else if (!strcmp($1, "BH")) {
                                            sprintf($$, "%d", PPC_INS_CB(ins));
                                          } else if (!strcmp($1, "CT")) {
                                            sprintf($$, "%d", PPC_INS_CT(ins));
                                          } else if (!strcmp($1, "MASK")) {
                                            sprintf($$, "%d", PPC_INS_MASK(ins));
                                          } else if (!strcmp($1, "SH")) {
                                            sprintf($$, "%d",
                                                    PPC_BITFIELD(AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)),
                                                                 0, 5));
                                          } else if (!strcmp($1, "MB")) {
                                            sprintf($$, "%d",
                                                    PPC_BITFIELD(AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)),
                                                                 5, 5));
                                          } else if (!strcmp($1, "ME")) {
                                            sprintf($$, "%d",
                                                    PPC_BITFIELD(AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)),
                                                                 10, 5));
                                          }  else if (!strcmp($1, "BCOND")) {
                                            char buffer[100];
                                            PpcPrintBranchConditional(buffer,"",ins);
                                            sprintf($$, "%s",buffer);
                                          }  else if (!strcmp($1, "BCONDLR")) {
                                            char buffer[100];
                                            PpcPrintBranchConditional(buffer,"lr",ins);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "BCONDCTR")) {
                                            char buffer[100];
                                            PpcPrintBranchConditional(buffer,"ctr",ins);
                                            sprintf($$, "%s",buffer);
                                          } else if (!strcmp($1, "RLEFT")) {
                                            char buffer[100];
                                            PpcPrintRotateLeft(buffer,ins);
                                            sprintf($$, "%s",buffer);
                                          }
                                           else {
                                             yyerror($1);
                                           }
   }

%%

int
yyerror (char *var)
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
PpcPrintParse (char *line, t_ppc_ins *instr, char *buf)
{
  idx = orig = line;
  len = strlen(line);
  res = buf;
  ins = instr;
  token = 1;
  yyparse();
}

void
PpcPrintRotateLeft(char *buffer, t_ppc_ins *ins)
{
  int opcode = PPC_INS_OPCODE(ins);
  char aux[20];
  strcpy(buffer,"");

  switch(opcode)
  {
    case PPC_RLDCR:
      strcat(buffer,"rldcr");
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        strcat(buffer,".");
      }
      strcat(buffer," ");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGB(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_MASK(ins)));
      strcat(buffer,aux);

      break;

    case PPC_RLWIMI:

      strcat(buffer,"rlwimi");
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        strcat(buffer,".");
      }
      strcat(buffer," ");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_IMM_SH(ins));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_IMM_MB(ins));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_IMM_ME(ins));
      strcat(buffer,aux);

      break;

    case PPC_RLWINM:

      if((PPC_IMM_MB(ins)==0)&&(PPC_IMM_ME(ins)==31))
      {
        strcat(buffer,"rotlwi");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_SH(ins));
        strcat(buffer,aux);

      }
      else if((PPC_IMM_SH(ins)==0)&&(PPC_IMM_ME(ins)==31))
      {
        strcat(buffer,"clrlwi");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_MB(ins));
        strcat(buffer,aux);

      }
      else
      {
        strcat(buffer,"rlwinm");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_SH(ins));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_MB(ins));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_ME(ins));
        strcat(buffer,aux);

      }
      break;

    case PPC_RLWNM:

      if((PPC_IMM_MB(ins)==0)&&(PPC_IMM_ME(ins)==31))
      {
        strcat(buffer,"rotlw");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGB(ins)));
        strcat(buffer,aux);

      }
      else
      {
        strcat(buffer,"rlwnm");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGB(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_MB(ins));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_IMM_ME(ins));
        strcat(buffer,aux);

      }
      break;
    case PPC_RLDIC:

      strcat(buffer,"rldic");
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        strcat(buffer,".");
      }
      strcat(buffer," ");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d", AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_INS_MASK(ins));
      strcat(buffer,aux);


      break;

    case PPC_RLDICL:

      if(PPC_INS_MASK(ins)==0)
      {
        strcat(buffer,"rotldi");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d", AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)));
        strcat(buffer,aux);

      }
      else if (AddressIsNull (PPC_INS_IMMEDIATE(ins)))
      {
        strcat(buffer,"clrldi");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_INS_MASK(ins));
        strcat(buffer,aux);

      }
      else
      {
        strcat(buffer,"rldicl");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d", AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_INS_MASK(ins));
        strcat(buffer,aux);

      }

      break;

    case PPC_RLDICR:

      strcat(buffer,"rldicr");
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        strcat(buffer,".");
      }
      strcat(buffer," ");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d", AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_INS_MASK(ins));
      strcat(buffer,aux);

      break;

    case PPC_RLDIMI:

      strcat(buffer,"rldimi");
      if(PPC_INS_FLAGS(ins)&PPC_FL_L)
      {
        strcat(buffer,".");
      }
      strcat(buffer," ");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d", AddressExtractUint32 (PPC_INS_IMMEDIATE(ins)));
      strcat(buffer,aux);
      strcat(buffer,",");

      sprintf(aux,"%d",PPC_INS_MASK(ins));
      strcat(buffer,aux);

      break;

    case PPC_RLDCL:

      if(PPC_INS_MASK(ins)==0)
      {
        strcat(buffer,"rotld");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGB(ins)));
        strcat(buffer,aux);

      }
      else
      {
        strcat(buffer,"rldcl");
        if(PPC_INS_FLAGS(ins)&PPC_FL_L)
        {
          strcat(buffer,".");
        }
        strcat(buffer," ");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGT(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGA(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%s",PpcRegisterName(PPC_INS_REGB(ins)));
        strcat(buffer,aux);
        strcat(buffer,",");

        sprintf(aux,"%d",PPC_INS_MASK(ins));
        strcat(buffer,aux);

      }

    default:
      break;
  }
}

void
PpcPrintBranchConditional(char *buffer, char *to, t_ppc_ins *ins)
{
  int bo = PPC_INS_BO(ins);
  int flags = PPC_INS_FLAGS(ins);
  strcpy(buffer,"b");
  if((bo&PPC_BOU)!=PPC_BOU)
  {
    if(bo==PPC_BOF||
       bo==PPC_BOFP||
       bo==PPC_BOFM4||
       bo==PPC_BOFP4)
    {
      PpcPrintCondition(buffer,ins,FALSE);
    }
    else if(bo==PPC_BOT||
            bo==PPC_BOTP||
            bo==PPC_BOTM4||
            bo==PPC_BOTP4)
    {
      PpcPrintCondition(buffer,ins,TRUE); 
    }
    else
    {
      strcat(buffer,"d");
      if(bo==PPC_BODNZF||
         bo==PPC_BODNZFP||
         bo==PPC_BODNZT||
         bo==PPC_BODNZTP||
         bo==PPC_BODNZ||
         bo==PPC_BODNZP||
         bo==PPC_BODNZM4||
         bo==PPC_BODNZP4)
      {
        strcat(buffer,"nz");
      }
      else if(bo==PPC_BODZF||
              bo==PPC_BODZFP||
              bo==PPC_BODZT||
              bo==PPC_BODZTP||
              bo==PPC_BODZ||
              bo==PPC_BODZP||
              bo==PPC_BODZM4||
              bo==PPC_BODZP4)
      {
        strcat(buffer,"z");
      }

      if(bo==PPC_BODNZF||
         bo==PPC_BODNZFP||
         bo==PPC_BODZF||
         bo==PPC_BODZFP)
      {
        strcat(buffer,"f");
      } 
      else if(bo==PPC_BODNZT||
              bo==PPC_BODNZTP||
              bo==PPC_BODZT||
              bo==PPC_BODZTP)
      {
        strcat(buffer,"t");
      }
    }
  }

  strcat(buffer,to);

  if((flags&PPC_FL_LINK)==PPC_FL_LINK)
  {
    strcat(buffer,"l");
  }

  if((flags&PPC_FL_ABSOLUTE)==PPC_FL_ABSOLUTE)
  {
    strcat(buffer,"a");
  }

  if((bo>>2)==3||(bo>>2)==4)
  {
    if((bo%2)==1)
    {
      strcat(buffer,"+");
    } 
    else
    {
      strcat(buffer,"-");
    }
  }
  strcat(buffer," ");

  if((bo&PPC_BOU)!=PPC_BOU)
  {
    if(bo==PPC_BODNZF||
       bo==PPC_BODNZFP||
       bo==PPC_BODNZT||
       bo==PPC_BODNZTP||
       bo==PPC_BODNZ||
       bo==PPC_BODNZP||
       bo==PPC_BODNZM4||
       bo==PPC_BODNZP4||
       bo==PPC_BODZF||
       bo==PPC_BODZFP||
       bo==PPC_BODZT||
       bo==PPC_BODZTP||
       bo==PPC_BODZ||
       bo==PPC_BODZP||
       bo==PPC_BODZM4||
       bo==PPC_BODZP4)
    {
      if(PPC_COND_REG(ins)!=PPC_REG_CR0)
      {
        if(bo==PPC_BODNZF||
           bo==PPC_BODNZFP||
           bo==PPC_BODZF||
           bo==PPC_BODZFP||
           bo==PPC_BODNZT||
           bo==PPC_BODNZTP||
           bo==PPC_BODZT||
           bo==PPC_BODZTP)
        {  
          strcat(buffer,"4*");
        }
        strcat(buffer,PpcRegisterName(PPC_COND_REG(ins)));
        if(bo==PPC_BODNZF||
           bo==PPC_BODNZFP||
           bo==PPC_BODZF||
           bo==PPC_BODZFP||
           bo==PPC_BODNZT||
           bo==PPC_BODNZTP||
           bo==PPC_BODZT||
           bo==PPC_BODZTP)
        {  
          strcat(buffer,"+");
        }
      }
      if(bo==PPC_BODNZF||
         bo==PPC_BODNZFP||
         bo==PPC_BODZF||
         bo==PPC_BODZFP)
      {
        PpcPrintCondition(buffer,ins,FALSE); 
        strcat(buffer,",\0");
      } 
      else if(bo==PPC_BODNZT||
              bo==PPC_BODNZTP||
              bo==PPC_BODZT||
              bo==PPC_BODZTP)
      {
        PpcPrintCondition(buffer,ins,TRUE); 
        strcat(buffer,",\0");
      }
    }
    else
    {
      if(PPC_COND_REG(ins)!=PPC_REG_CR0)
      {
        strcat(buffer,PpcRegisterName(PPC_COND_REG(ins)));
        strcat(buffer,",\0");
      }
    }
  }

}

void
PpcPrintCondition(char *buffer, t_ppc_ins *ins, t_bool cond)
{
  if(cond)
  {
    if(PPC_IS_CBLT(ins))
    {
      strcat(buffer,"lt");
    }
    else if(PPC_IS_CBGT(ins))
    {
      strcat(buffer,"gt");
    }
    else if(PPC_IS_CBEQ(ins))
    {
      strcat(buffer,"eq");
    }
    else if(PPC_IS_CBSO(ins))
    {
      strcat(buffer,"so");
    }
  }
  else
  {
    if(PPC_IS_CBLT(ins))
    {
      strcat(buffer,"ge");
    }
    else if(PPC_IS_CBGT(ins))
    {
      strcat(buffer,"le");
    }
    else if(PPC_IS_CBEQ(ins))
    {
      strcat(buffer,"ne");
    }
    else if(PPC_IS_CBSO(ins))
    {
      strcat(buffer,"ns");
    }
  }
}

void
PpcPrintConditionReg(int bit, char *buffer)
{
  strcpy(buffer,"");
  if((bit/4)!=0)
  {
    strcat(buffer,"4*");
    strcat(buffer,"cr");
    switch(bit/4)
    {
      case 0:
        strcat(buffer,"0");
        break;
      case 1:
        strcat(buffer,"1");
        break;
      case 2:
        strcat(buffer,"2");
        break;
      case 3:
        strcat(buffer,"3");
        break;
      case 4:
        strcat(buffer,"4");
        break;
      case 5:
        strcat(buffer,"5");
        break;
      case 6:
        strcat(buffer,"6");
        break;
      case 7:
        strcat(buffer,"7");
        break;
      default:
        break;
    }
    strcat(buffer,"+");
  }
  switch(bit%4)
  {
    case 0:
      strcat(buffer,"lt");
      break;
    case 1:
      strcat(buffer,"gt");
      break;
    case 2:
      strcat(buffer,"eq");
      break;
    case 3:
      strcat(buffer,"so");
      break;
    default:
      break;
  }
}

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
