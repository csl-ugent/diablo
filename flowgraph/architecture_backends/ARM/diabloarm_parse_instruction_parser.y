%{
#include "diabloarm_parse_instruction_parser_extra.h"
#include <diabloarm.h>
extern int yylex (void);
char  * input_instruction;
int input_offset=0;
t_arm_ins * ret;
static t_bool error_code = FALSE;
void yyreset();
#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


%}

/* Datatypes used in the parser */
%union
{
   char * string;
   struct blah
   {
   	int opcode;
   	int is_link;
   	int size;
   	int set;
	int condition;
	int rega;
	int mem;
   } op_and_suf;
   struct 
   {
   	int regb;
	int regc;
	int flags;
	int immed;
	int shifttype;
	int shiftim;
   } mem_op;
   int i;
}

%token ARM_OPCODE
%type <i> ARM_OPCODE 
%token COMMA
%token REGISTER
%type <i> REGISTER
%token CONSTANT
%type <i> CONSTANT
%token CONDITION_SUFFIX
%type <i> CONDITION_SUFFIX
%token MEMORY_SUFFIX
%type <i> MEMORY_SUFFIX
%token SET_SUFFIX
%token REGSET
%type <i> REGSET
%token WRITEBACK
%token SHIFT
%type <i> SHIFT
%token MEMORY
%token BYTE_SUFFIX
%token HALFWORD_SUFFIX
%token SIGNED_BYTE_SUFFIX
%token SIGNED_HALFWORD_SUFFIX
%token LINK_SUFFIX
/* Type rules */

%type <op_and_suf> link;
%type <op_and_suf> size;
%type <op_and_suf> set;
%type <op_and_suf> memory;
%type <op_and_suf> condition;
%type <op_and_suf> suffixlist;
%type <op_and_suf> full_opcode;
%type <op_and_suf> pre;
%type <mem_op> mem;

%{
#include <diabloarm.h>
void yyerror(char * in)
{
  printf("Parse error: %s: %s %d (%s)\n",in,input_instruction,input_offset,&(input_instruction[input_offset]));
  error_code = FALSE;
}


void MakeIns(struct blah i, int rega, int regb, int regc, int regs, int im, int flags,int shift,int len)
{
  ARM_INS_SET_OPCODE(ret, i.opcode);

  switch(i.opcode)
  {
    case ARM_LDM:
      ARM_INS_SET_TYPE(ret,  IT_LOAD_MULTIPLE);
      break;
    case ARM_STM:
      ARM_INS_SET_TYPE(ret,  IT_STORE_MULTIPLE);
      break;
    case ARM_LDR:
    case ARM_LDRB:
    case ARM_LDRH:
    case ARM_LDRSB:
    case ARM_LDRSH:
      ARM_INS_SET_TYPE(ret,  IT_LOAD);
      break;
    case ARM_STR:
    case ARM_STRB:
    case ARM_STRH:
      ARM_INS_SET_TYPE(ret,  IT_STORE);
      break;
    case ARM_ADD:
    case ARM_SUB:
    case ARM_CMP:
    case ARM_RSB:
    case ARM_TST:
    case ARM_BIC:
    case ARM_EOR:
    case ARM_SBC:
    case ARM_RSC:
    case ARM_MOV:
    case ARM_ADC:
    case ARM_TEQ:
    case ARM_MVN:
    case ARM_AND:
    case ARM_CMN:
    case ARM_ORR:
      ARM_INS_SET_TYPE(ret,  IT_DATAPROC);
      break;
    case ARM_MLA:
    case ARM_MUL:
    case ARM_UMULL:
      ARM_INS_SET_TYPE(ret,  IT_MUL);
      break;
    case ARM_SWI:
      ARM_INS_SET_TYPE(ret,  IT_SWI);
      break;
    case ARM_MRS:
    case ARM_MSR:
      ARM_INS_SET_TYPE(ret,  IT_STATUS);
      break;
    case ARM_BL:
    case ARM_B:
      ARM_INS_SET_TYPE(ret,  IT_BRANCH);
      break;
    default:
      VERBOSE(0,("Implement type %d %s",i.opcode,input_instruction));
      yyerror("Unimplemented type, see previous message");
  }
  
  ARM_INS_SET_CONDITION(ret, i.condition);
  ARM_INS_SET_REGA(ret, rega);
  ARM_INS_SET_REGB(ret, regb);
  ARM_INS_SET_REGC(ret, regc);
  ARM_INS_SET_REGS(ret, regs);
  ARM_INS_SET_IMMEDIATE(ret, im);
  ARM_INS_SET_SHIFTTYPE(ret, shift);
  ARM_INS_SET_FLAGS(ret,  flags);
  ARM_INS_SET_SHIFTLENGTH(ret, len);
}


t_bool ArmInsAssembleFromString(t_arm_ins * ins, t_string in)
{
  input_instruction=in;
  input_offset=0;
  error_code = TRUE;
  yyreset();
  ret=ins;
  yyparse();
  return error_code;
}



%}
  %%

armins: /* 4 reg op with 4 registers             */ pre REGISTER COMMA REGISTER COMMA REGISTER       { MakeIns($1,$1.rega,$2,$4,$6,0,($1.set?FL_S:0),ARM_SHIFT_TYPE_NONE,0); }
      | /* 3 reg op with 3 registers             */ pre REGISTER COMMA REGISTER                      { MakeIns($1,$1.rega,$2,$4,ARM_REG_NONE,0,($1.set?FL_S:0),ARM_SHIFT_TYPE_NONE,0); }
      | /* 3 reg op with 3 registers + shift im  */ pre REGISTER COMMA REGISTER COMMA SHIFT CONSTANT { MakeIns($1,$1.rega,$2,$4,ARM_REG_NONE,0,($1.set?FL_S:0),$6,$7); }
      | /* 3 reg op with 3 registers + shift reg */ pre REGISTER COMMA REGISTER COMMA SHIFT REGISTER { MakeIns($1,$1.rega,$2,$4,$7,0,($1.set?FL_S:0),$6+4,0); }
      | /* 3 reg op with immediate               */ pre REGISTER COMMA CONSTANT                      { MakeIns($1,$1.rega,$2,ARM_REG_NONE,ARM_REG_NONE,$4,FL_IMMED|($1.set?FL_S:0),ARM_SHIFT_TYPE_NONE,0); }
      | /* 2 reg op with 2 registers             */ pre REGISTER                                     { MakeIns($1,$1.rega,ARM_REG_NONE,$2,ARM_REG_NONE,0,($1.set?FL_S:0),ARM_SHIFT_TYPE_NONE,0); }
      | /* 2 reg op with 2 registers + shift im  */ pre REGISTER COMMA SHIFT CONSTANT                { MakeIns($1,$1.rega,ARM_REG_NONE,$2,ARM_REG_NONE,0,($1.set?FL_S:0),$4,$5); }
      | /* 2 reg op with 2 registers + shift reg */ pre REGISTER COMMA SHIFT REGISTER                { MakeIns($1,$1.rega,ARM_REG_NONE,$2,$5,0,($1.set?FL_S:0),$4+4,0); }
      | /* 2 reg op with immediate   */             pre CONSTANT                                     { MakeIns($1,$1.rega,ARM_REG_NONE,ARM_REG_NONE,ARM_REG_NONE,$2,FL_IMMED,ARM_SHIFT_TYPE_NONE,0); }
      | /* ld, str */                               pre mem                                          { MakeIns($1,$1.rega,$2.regb,$2.regc,ARM_REG_NONE,$2.immed,($1.set?FL_S:0)|$2.flags,$2.shifttype,$2.shiftim); }
      | pre REGSET                                                                                   { MakeIns($1,ARM_REG_NONE,$1.rega,ARM_REG_NONE,ARM_REG_NONE,$2,($1.set?FL_S:0)|(($1.mem==1||$1.mem==2)?FL_DIRUP:0)|(($1.mem==2||$1.mem==4)?FL_PREINDEX:0),ARM_SHIFT_TYPE_NONE,0);  }
      | full_opcode REGISTER WRITEBACK COMMA REGSET                                                  { MakeIns($1,ARM_REG_NONE,$2,ARM_REG_NONE,ARM_REG_NONE,$5,($1.set?FL_S:0)|(($1.mem==1||$1.mem==2)?FL_DIRUP:0)|(($1.mem==2||$1.mem==4)?FL_PREINDEX:0)|FL_WRITEBACK,ARM_SHIFT_TYPE_NONE,0);  }
      | full_opcode CONSTANT                                                                         { MakeIns($1,ARM_REG_NONE,ARM_REG_NONE,ARM_REG_NONE,ARM_REG_NONE,$2,FL_IMMED|($1.set?FL_S:0),ARM_SHIFT_TYPE_NONE,0);  } 
      ;


mem: /* ld, str: with preindex    */            MEMORY REGISTER COMMA REGISTER MEMORY                      {$$.regb=$2; $$.regc=$4; $$.flags=FL_PREINDEX|FL_DIRUP; $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld, str: with preindex and wb  */       MEMORY REGISTER COMMA REGISTER MEMORY WRITEBACK            {$$.regb=$2; $$.regc=$4; $$.flags=FL_PREINDEX|FL_DIRUP|FL_WRITEBACK; $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld, str: with postindex   */            MEMORY REGISTER MEMORY COMMA REGISTER                      {$$.regb=$2; $$.regc=$5; $$.flags=FL_DIRUP; $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld, str: preindex    */                 MEMORY REGISTER COMMA CONSTANT MEMORY                      {$$.regb=$2; $$.regc=ARM_REG_NONE; $$.flags=FL_PREINDEX|FL_IMMED; if ($4>=0) { $$.flags|=FL_DIRUP; $$.immed=$4;  }else {  $$.immed=-$4;}  $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld, str: preindex    */                 MEMORY REGISTER COMMA CONSTANT MEMORY WRITEBACK            {$$.regb=$2;  $$.regc=ARM_REG_NONE; $$.flags=FL_PREINDEX|FL_IMMED|FL_WRITEBACK; if ($4>=0) { $$.flags|=FL_DIRUP; $$.immed=$4;  }else { $$.immed=-$4;} $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld,str with shift:preindex */           MEMORY REGISTER COMMA REGISTER COMMA SHIFT CONSTANT MEMORY {$$.regb=$2; $$.regc=$4; $$.shifttype=$6; $$.flags=FL_PREINDEX|FL_DIRUP; $$.shiftim=$7; }
   | /* ld, str: postindex    */                MEMORY REGISTER MEMORY COMMA CONSTANT                      {$$.regb=$2; $$.regc=ARM_REG_NONE; $$.flags=FL_IMMED; if ($5>=0) { $$.flags|=FL_DIRUP; $$.immed=$5;  }else { $$.immed=-$5;} $$.shifttype=ARM_SHIFT_TYPE_NONE;}
   | /* ld, str */                              MEMORY REGISTER MEMORY                                     {$$.regb=$2; $$.regc=ARM_REG_NONE; $$.flags=0; $$.immed=0;  $$.shifttype=ARM_SHIFT_TYPE_NONE;}

   

pre: full_opcode REGISTER COMMA { $$=$1; $$.rega=$2; }

full_opcode: ARM_OPCODE suffixlist {  $$=$2;
	                          if (($2.is_link==1)&&($1==ARM_B)) 
	                            $$.opcode=ARM_BL; 
				  else if ($2.is_link)
				    yyerror("Unknown instruction");
				  else if (($2.size==4)&&($1==ARM_LDR))
	                            $$.opcode=ARM_LDRSH; 
				  else if (($2.size==3)&&($1==ARM_LDR))
	                            $$.opcode=ARM_LDRSB; 
				  else if (($2.size==2)&&($1==ARM_LDR))
	                            $$.opcode=ARM_LDRH; 
				  else if (($2.size==1)&&($1==ARM_LDR))
	                            $$.opcode=ARM_LDRB; 
				  else if (($2.size==4)&&($1==ARM_STR))
				    yyerror("Unknown instruction");
				  else if (($2.size==3)&&($1==ARM_STR))
				    yyerror("Unknown instruction");
				  else if (($2.size==2)&&($1==ARM_STR))
	                            $$.opcode=ARM_STRH; 
				  else if (($2.size==1)&&($1==ARM_STR))
	                            $$.opcode=ARM_STRB; 
				  else if ($2.size)
				    yyerror("Unknown instruction");
				  else
				    $$.opcode=$1;
			       }


suffixlist: link size condition memory set { $$.is_link=$1.is_link; $$.size=$2.size; $$.condition=$3.condition; $$.mem=$4.mem; $$.set=$5.set; }
	  ; 
	  
condition:                  { $$.condition=ARM_CONDITION_AL; } 
	 | CONDITION_SUFFIX { $$.condition=$1; } 
	 ;

memory:		      { $$.mem=0; }
      | MEMORY_SUFFIX { $$.mem=$1; }
      ;

set:		   { $$.set=0; }
      | SET_SUFFIX { $$.set=1; }
      ;

size:		               { $$.size=0; }
      | BYTE_SUFFIX            { $$.size=1; }
      | HALFWORD_SUFFIX        { $$.size=2; }
      | SIGNED_BYTE_SUFFIX     { $$.size=3; }
      | SIGNED_HALFWORD_SUFFIX { $$.size=4; }
      ;

link:                   { $$.is_link=0;}
    | LINK_SUFFIX       { $$.is_link=1;}
    ;


%%


