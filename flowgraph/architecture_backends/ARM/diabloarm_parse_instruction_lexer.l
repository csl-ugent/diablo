%option prefix="ArmParseInstruction"
%{
#define yylval ArmParseInstructionlval
#define yyerror ArmParseInstructionerror
#include <diabloarm.h>
#include "diabloarm_parse_instruction_parser.h"
extern char  * input_instruction;
extern int input_offset;
extern void yyerror(char * in);
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	{ \
		int c = input_instruction[input_offset++]; \
		result = ((c == 0 || c == '\n')) ? YY_NULL : (buf[0] = c, 1); \
	}

	void yyreset()
	{
	BEGIN(0);
	}
%}
%option noyywrap
%x OPCODE_DONE
%x SUFFIXES_DONE
%x REGSET_STATE
%x SHIFT_STATE
%x SHIFT_STATE_FIXUP
%%
[ \t]		     { /* Skip whitespace */ }
[Uu][Mm][Uu][Ll][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_UMULL; return ARM_OPCODE;}
[Uu][Mm][Ll][Aa][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_UMLAL; return ARM_OPCODE;}
[Ss][Mm][Uu][Ll][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_SMULL; return ARM_OPCODE;}
[Ss][Mm][Ll][Aa][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_SMLAL; return ARM_OPCODE;}
[Ss][Ww][Pp][Bb] {BEGIN(OPCODE_DONE); yylval.i=ARM_SWPB; return ARM_OPCODE;}
[Mm][Rr][Rr][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_MRRC; return ARM_OPCODE;}
[Mm][Cc][Rr][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_MCRR; return ARM_OPCODE;}
[Cc][Nn][Ff][Ee] {BEGIN(OPCODE_DONE); yylval.i=ARM_CNFE; return ARM_OPCODE;}
[Cc][Mm][Ff][Ee] {BEGIN(OPCODE_DONE); yylval.i=ARM_CMFE; return ARM_OPCODE;}
[Ww][Ff][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_WFS; return ARM_OPCODE;}
[Ww][Ff][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_WFC; return ARM_OPCODE;}
[Uu][Rr][Dd] {BEGIN(OPCODE_DONE); yylval.i=ARM_URD; return ARM_OPCODE;}
[Tt][Ss][Tt] {BEGIN(OPCODE_DONE); yylval.i=ARM_TST; return ARM_OPCODE;}
[Tt][Ee][Qq] {BEGIN(OPCODE_DONE); yylval.i=ARM_TEQ; return ARM_OPCODE;}
[Tt][Aa][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_TAN; return ARM_OPCODE;}
[Ss][Ww][Pp] {BEGIN(OPCODE_DONE); yylval.i=ARM_SWP; return ARM_OPCODE;}
[Ss][Ww][Ii] {BEGIN(OPCODE_DONE); yylval.i=ARM_SWI; return ARM_OPCODE;}
[Ss][Uu][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_SUF; return ARM_OPCODE;}
[Ss][Uu][Bb] {BEGIN(OPCODE_DONE); yylval.i=ARM_SUB; return ARM_OPCODE;}
[Ss][Tt][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_STR; return ARM_OPCODE;}
[Ss][Tt][Mm] {BEGIN(OPCODE_DONE); yylval.i=ARM_STM; return ARM_OPCODE;}
[Ss][Tt][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_STF; return ARM_OPCODE;}
[Ss][Tt][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_STC; return ARM_OPCODE;}
[Ss][Qq][Tt] {BEGIN(OPCODE_DONE); yylval.i=ARM_SQT; return ARM_OPCODE;}
[Ss][Ii][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_SIN; return ARM_OPCODE;}
[Ss][Ff][Mm] {BEGIN(OPCODE_DONE); yylval.i=ARM_SFM; return ARM_OPCODE;}
[Ss][Bb][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_SBC; return ARM_OPCODE;}
[Rr][Ss][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_RSF; return ARM_OPCODE;}
[Rr][Ss][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_RSC; return ARM_OPCODE;}
[Rr][Ss][Bb] {BEGIN(OPCODE_DONE); yylval.i=ARM_RSB; return ARM_OPCODE;}
[Rr][Pp][Ww] {BEGIN(OPCODE_DONE); yylval.i=ARM_RPW; return ARM_OPCODE;}
[Rr][Nn][Dd] {BEGIN(OPCODE_DONE); yylval.i=ARM_RND; return ARM_OPCODE;}
[Rr][Mm][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_RMF; return ARM_OPCODE;}
[Rr][Ff][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_RFS; return ARM_OPCODE;}
[Rr][Ff][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_RFC; return ARM_OPCODE;}
[Rr][Dd][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_RDF; return ARM_OPCODE;}
[Pp][Oo][Ww] {BEGIN(OPCODE_DONE); yylval.i=ARM_POW; return ARM_OPCODE;}
[Pp][Oo][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_POL; return ARM_OPCODE;}
[Oo][Rr][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_ORR; return ARM_OPCODE;}
[Nn][Rr][Mm] {BEGIN(OPCODE_DONE); yylval.i=ARM_NRM; return ARM_OPCODE;}
[Mm][Vv][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_MVN; return ARM_OPCODE;}
[Mm][Vv][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_MVF; return ARM_OPCODE;}
[Mm][Uu][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_MUL; return ARM_OPCODE;}
[Mm][Uu][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_MUF; return ARM_OPCODE;}
[Mm][Ss][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_MSR; return ARM_OPCODE;}
[Mm][Rr][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_MRS; return ARM_OPCODE;}
[Mm][Rr][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_MRC; return ARM_OPCODE;}
[Mm][Oo][Vv] {BEGIN(OPCODE_DONE); yylval.i=ARM_MOV; return ARM_OPCODE;}
[Mm][Nn][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_MNF; return ARM_OPCODE;}
[Mm][Ll][Aa] {BEGIN(OPCODE_DONE); yylval.i=ARM_MLA; return ARM_OPCODE;}
[Mm][Cc][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_MCR; return ARM_OPCODE;}
[Ll][Oo][Gg] {BEGIN(OPCODE_DONE); yylval.i=ARM_LOG; return ARM_OPCODE;}
[Ll][Gg][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_LGN; return ARM_OPCODE;}
[Ll][Ff][Mm] {BEGIN(OPCODE_DONE); yylval.i=ARM_LFM; return ARM_OPCODE;}
[Ll][Dd][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_LDR; return ARM_OPCODE;}
[Ll][Dd][Mm] {BEGIN(OPCODE_DONE); yylval.i=ARM_LDM; return ARM_OPCODE;}
[Ll][Dd][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_LDF; return ARM_OPCODE;}
[Ll][Dd][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_LDC; return ARM_OPCODE;}
[Ff][Rr][Dd] {BEGIN(OPCODE_DONE); yylval.i=ARM_FRD; return ARM_OPCODE;}
[Ff][Mm][Ll] {BEGIN(OPCODE_DONE); yylval.i=ARM_FML; return ARM_OPCODE;}
[Ff][Ll][Tt] {BEGIN(OPCODE_DONE); yylval.i=ARM_FLT; return ARM_OPCODE;}
[Ff][Ii][Xx] {BEGIN(OPCODE_DONE); yylval.i=ARM_FIX; return ARM_OPCODE;}
[Ff][Dd][Vv] {BEGIN(OPCODE_DONE); yylval.i=ARM_FDV; return ARM_OPCODE;}
[Ee][Xx][Pp] {BEGIN(OPCODE_DONE); yylval.i=ARM_EXP; return ARM_OPCODE;}
[Ee][Oo][Rr] {BEGIN(OPCODE_DONE); yylval.i=ARM_EOR; return ARM_OPCODE;}
[Dd][Vv][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_DVF; return ARM_OPCODE;}
[Cc][Oo][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_COS; return ARM_OPCODE;}
[Cc][Nn][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_CNF; return ARM_OPCODE;}
[Cc][Mm][Pp] {BEGIN(OPCODE_DONE); yylval.i=ARM_CMP; return ARM_OPCODE;}
[Cc][Mm][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_CMN; return ARM_OPCODE;}
[Cc][Mm][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_CMF; return ARM_OPCODE;}
[Cc][Ll][Zz] {BEGIN(OPCODE_DONE); yylval.i=ARM_CLZ; return ARM_OPCODE;}
[Cc][Dd][Pp] {BEGIN(OPCODE_DONE); yylval.i=ARM_CDP; return ARM_OPCODE;}
[Bb][Ll][Xx] {BEGIN(OPCODE_DONE); yylval.i=ARM_BLX; return ARM_OPCODE;}
[Bb][Ii][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_BIC; return ARM_OPCODE;}
[Aa][Tt][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_ATN; return ARM_OPCODE;}
[Aa][Ss][Nn] {BEGIN(OPCODE_DONE); yylval.i=ARM_ASN; return ARM_OPCODE;}
[Aa][Nn][Dd] {BEGIN(OPCODE_DONE); yylval.i=ARM_AND; return ARM_OPCODE;}
[Aa][Dd][Ff] {BEGIN(OPCODE_DONE); yylval.i=ARM_ADF; return ARM_OPCODE;}
[Aa][Dd][Dd] {BEGIN(OPCODE_DONE); yylval.i=ARM_ADD; return ARM_OPCODE;}
[Aa][Dd][Cc] {BEGIN(OPCODE_DONE); yylval.i=ARM_ADC; return ARM_OPCODE;}
[Aa][Cc][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_ACS; return ARM_OPCODE;}
[Aa][Bb][Ss] {BEGIN(OPCODE_DONE); yylval.i=ARM_ABS; return ARM_OPCODE;}
[Bb][Xx]     {BEGIN(OPCODE_DONE); yylval.i=ARM_BX; return ARM_OPCODE;}
[Bb] {BEGIN(OPCODE_DONE);  yylval.i=ARM_B; return ARM_OPCODE;}
.    {yyerror("unmatched character in opcode"); } 

<OPCODE_DONE>[ \t] { BEGIN(SUFFIXES_DONE); }
<OPCODE_DONE>[Ll][eE][qQ]     {/* Branch and Link */ unput('q'); unput('e'); return LINK_SUFFIX; }
<OPCODE_DONE>[Ss][Hh] {return SIGNED_HALFWORD_SUFFIX; }
<OPCODE_DONE>[Ss][Bb] {return SIGNED_BYTE_SUFFIX; }
<OPCODE_DONE>[Cc][Cc] {yylval.i=ARM_CONDITION_CC; return CONDITION_SUFFIX;}
<OPCODE_DONE>[Cc][Ss] {yylval.i=ARM_CONDITION_CS;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Ee][Qq] {yylval.i=ARM_CONDITION_EQ;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Gg][Ee] {yylval.i=ARM_CONDITION_GE;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Gg][Tt] {yylval.i=ARM_CONDITION_GT;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Hh][Ii] {yylval.i=ARM_CONDITION_HI;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Ll][Ee] {yylval.i=ARM_CONDITION_LE;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Ll][Ss] {yylval.i=ARM_CONDITION_LS;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Ll][Tt] {yylval.i=ARM_CONDITION_LT;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Mm][Ii] {yylval.i=ARM_CONDITION_MI;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Nn][Ee] {yylval.i=ARM_CONDITION_NE;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Pp][Ll] {yylval.i=ARM_CONDITION_PL;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Vv][Cc] {yylval.i=ARM_CONDITION_VC;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Vv][Ss] {yylval.i=ARM_CONDITION_VS;return CONDITION_SUFFIX;}
<OPCODE_DONE>[Ii][Aa] {yylval.i=1;return MEMORY_SUFFIX;}
<OPCODE_DONE>[Ii][Bb] {yylval.i=2;return MEMORY_SUFFIX;}
<OPCODE_DONE>[Dd][Aa] {yylval.i=3;return MEMORY_SUFFIX;}
<OPCODE_DONE>[Dd][Bb] {yylval.i=4;return MEMORY_SUFFIX;}
<OPCODE_DONE>[Hh]     {return HALFWORD_SUFFIX; }
<OPCODE_DONE>[Bb]     {return BYTE_SUFFIX; }
<OPCODE_DONE>[Ss]     {return SET_SUFFIX;}
<OPCODE_DONE>[Ll]     {return LINK_SUFFIX; /* Branch and Link */}
<OPCODE_DONE>.	      { yyerror("Unknown char in suffix"); }

<SUFFIXES_DONE>[ \t] { /* Ignore white space */ }
<SUFFIXES_DONE>[lL][sS][lL]		{ yylval.i=0; BEGIN(SHIFT_STATE); return SHIFT;} 
<SUFFIXES_DONE>[lL][sS][rR]		{ yylval.i=1; BEGIN(SHIFT_STATE); return SHIFT;} 
<SUFFIXES_DONE>[aA][sS][rR]		{ yylval.i=2; BEGIN(SHIFT_STATE); return SHIFT;} 
<SUFFIXES_DONE>[rR][oO][rR]		{ yylval.i=3; BEGIN(SHIFT_STATE); return SHIFT;} 
<SUFFIXES_DONE>[rR][rR][xX]		{ yylval.i=8; BEGIN(SHIFT_STATE); /* unput ('#'); unput('0'); */ return SHIFT;} 

<SUFFIXES_DONE>,                 { return COMMA; }
<SUFFIXES_DONE>"!"               { return WRITEBACK; }
<SUFFIXES_DONE>"["               { return MEMORY; }
<SUFFIXES_DONE>"]"               { return MEMORY; }
<SUFFIXES_DONE>[cC][pP][sS][rR]  { return REGISTER; }
<SUFFIXES_DONE>[rR][0-9]*        { yylval.i=atoi(yytext+1); return REGISTER; }
<SUFFIXES_DONE>[pP][cC]          { yylval.i=15; return REGISTER; }
<SUFFIXES_DONE>[sS][pP]          { yylval.i=13; return REGISTER; }
<SUFFIXES_DONE>"#"		 { /* Ignore #, sometimes it is used sometimes it is not */ }	
<SUFFIXES_DONE>-?[0-9]+          { yylval.i=atoi(yytext); return CONSTANT; }
<SUFFIXES_DONE>-?0x[0-9a-f]+     { yylval.i=strtoul(yytext,NULL,16); return CONSTANT; }
<SUFFIXES_DONE>"{"		 { yylval.i=0; BEGIN(REGSET_STATE);}
<SUFFIXES_DONE>.		 { yyerror("unmatched character in operands"); } 

<SHIFT_STATE>[ \t]	         { /* ignore whitespace */ }
<SHIFT_STATE>[rR][0-9]*          { yylval.i=atoi(yytext+1);BEGIN(SUFFIXES_DONE); return REGISTER; }
<SHIFT_STATE>"#"		 { /* Ignore #, sometimes it is used sometimes it is not */ }	
<SHIFT_STATE>-?[0-9]*            { yylval.i=atoi(yytext); BEGIN(SUFFIXES_DONE); return CONSTANT; }
<SHIFT_STATE>.		         { yyerror("unmatched character in shift"); }


<REGSET_STATE>"}"		 { BEGIN(SUFFIXES_DONE); return REGSET; }
<REGSET_STATE>","	 	 { }
<REGSET_STATE>[rR][0-9]*	 { yylval.i|=1<<atoi(yytext+1); }
<REGSET_STATE>[pP][cC]	 	 { yylval.i|=1<<15;}
<REGSET_STATE>.		         { yyerror("unmatched character in regset"); }

%%
