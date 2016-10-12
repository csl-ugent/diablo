#include <diablosmc.h>
#include <diabloi386.h>

void I386ClearIns(t_i386_ins * ins);
void I386SetGenericInsInfo(t_i386_ins * ins);

static t_i386_opcode_entry * forms[10];   /* this is certainly large enough */
//static t_uint8 buffer[30];

/* ALL LISTS {{{ */
/*lea 0,reg1 == mov 0,reg1 == and 0,reg1 == imul0,reg1,reg1 == sub reg1,reg1 == xor reg1,reg1*/
/*all_regs_live: {lea,move} en {and,imul,sub,xor}*/
/*0*/
t_subs_ins set_0[] =
{
  {I386_LEA,		{SUB_LEA_CONST(0)},		{SUB_NONE},	{SUB_LOC(locmode_loc1)}, 0}, 
  {I386_MOV,		{SUB_CONST(0)},			{SUB_NONE},	{SUB_LOC(locmode_loc1)}, 32}, 
  {I386_AND,		{SUB_CONST(0)},			{SUB_NONE},	{SUB_LOC(locmode_loc1)}, 0},
  {I386_IMULexp2,	{SUB_LOC(locmode_loc1)},	{SUB_CONST(0)},	{SUB_LOC(locmode_loc1)}, 0},
  {I386_SUB, 		{SUB_LOC(locmode_loc1)}, 	{SUB_NONE}, 	{SUB_LOC(locmode_loc1)}, 0},
  {I386_XOR, 		{SUB_LOC(locmode_loc1)}, 	{SUB_NONE}, 	{SUB_LOC(locmode_loc1)}, 68}, 
  {MAX_I386_OPCODE}
};

/*1*/
t_subs_ins set_0a[] =
{
  {I386_AND,		{SUB_CONST(0)},			{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_IMULexp2,	{SUB_LOC(locmode_loc1)},	{SUB_CONST(0)},	{SUB_LOC(locmode_loc1)},0},
  {I386_SUB, 		{SUB_LOC(locmode_loc1)}, 	{SUB_NONE}, 	{SUB_LOC(locmode_loc1)},0},
  {I386_XOR, 		{SUB_LOC(locmode_loc1)}, 	{SUB_NONE}, 	{SUB_LOC(locmode_loc1)},100},
  {MAX_I386_OPCODE}
};

/*2*/
t_subs_ins set_0b[] =
{
  {I386_LEA,		{SUB_LEA_CONST(0)},		{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_MOV,		{SUB_CONST(0)},			{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {MAX_I386_OPCODE}
};

/*mov -1,reg1 == or -1,reg1 == lea -1,reg1*/
/*all_regs_liv: {lea,move} en {or}*/
/*3*/
t_subs_ins set_1[]=
{
  {I386_MOV,	{SUB_CONST(-1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},98},
  {I386_LEA, 	{SUB_LEA_CONST(-1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_OR,	{SUB_CONST(-1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},2},
  {MAX_I386_OPCODE}
};

/*4*/
t_subs_ins set_1a[]=
{
  {I386_MOV,	{SUB_CONST(-1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_LEA, 	{SUB_LEA_CONST(-1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*inc reg1 == lea 1(reg1),reg1 == lea 1(,reg1,1),reg1 == add 1,reg1 == sub -1,reg1*/
/*all_regs_live: {add, inc} {sub} {lea}*/
/*5*/
t_subs_ins set_2[] =
{
  {I386_LEA, {SUB_LEA_CONST_LONG(1,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {I386_LEA, {SUB_LEA_CONST_LONG(1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_INC, {SUB_NONE},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},99},
  {I386_ADD, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_SUB, {SUB_CONST(-1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*6*/
t_subs_ins set_2a[] =
{
  {I386_INC, {SUB_NONE},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_ADD, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_SUB, {SUB_CONST(-1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*7*/
t_subs_ins set_2_a[] =
{
  {I386_INC, {SUB_NONE},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_ADD, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*8*/
t_subs_ins set_2b[] =
{
  {I386_LEA, {SUB_LEA_CONST_LONG(1,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_LEA, {SUB_LEA_CONST_LONG(1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*dec reg1 == lea -1(reg1),reg1 == lea -1(,reg1,1),reg1 == add -1,reg1 == sub 1,reg1*/
/*{sub, dec} {add} {lea}*/
/*9*/
t_subs_ins set_3[] = 
{
  {I386_LEA, {SUB_LEA_CONST_LONG(-1,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_LEA, {SUB_LEA_CONST_LONG(-1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_DEC, {SUB_NONE},     						{SUB_NONE},	{SUB_LOC(locmode_loc1)},99},
  {I386_ADD, {SUB_CONST(-1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_SUB, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {MAX_I386_OPCODE}
};

/*10*/
t_subs_ins set_3a[] = 
{
  {I386_DEC, {SUB_NONE},     						{SUB_NONE},	{SUB_LOC(locmode_loc1)},99},
  {I386_ADD, {SUB_CONST(-1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_SUB, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {MAX_I386_OPCODE}
};

/*11*/
t_subs_ins set_3_a[] = 
{
  {I386_DEC, {SUB_NONE},     						{SUB_NONE},	{SUB_LOC(locmode_loc1)},99},
  {I386_SUB, {SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {MAX_I386_OPCODE}
};

/*12*/
t_subs_ins set_3b[] = 
{
  {I386_LEA, {SUB_LEA_CONST_LONG(-1,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_LEA, {SUB_LEA_CONST_LONG(-1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*neg reg1 == imul -1,reg1,reg1*/
/*{neg} {imul} (CF flag)*/
/*13*/
t_subs_ins set_4[] = 
{
  {I386_NEG,		{SUB_NONE},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_IMULexp2,	{SUB_LOC(locmode_loc1)},{SUB_CONST(-1)},{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*not reg1 == xor -1,reg1*/
/*{not,xor}*/
/*14*/
t_subs_ins set_5[] = 
{
  {I386_NOT,		{SUB_NONE},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_XOR,		{SUB_CONST(-1)},{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*test reg1,reg1 == cmp 0,reg1 == test -1,reg1*/
/*{cmp, test}*/
/*15*/
t_subs_ins set_6[] =
{
  {I386_CMP,	{SUB_LOC(locmode_loc1)},	{SUB_CONST(0)},			{SUB_NONE},2},
  {I386_TEST,	{SUB_LOC(locmode_loc1)},	{SUB_CONST(-1)},		{SUB_NONE},0},
  {I386_TEST, 	{SUB_LOC(locmode_loc1)},	{SUB_LOC(locmode_loc1)},	{SUB_NONE},98},
  {I386_AND,	{SUB_LOC(locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_OR,	{SUB_LOC(locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};


/*add reg1,reg1*/
/*{add, xadd}(als AF dood een verz){shl} {lea}*/
/*16*/
t_subs_ins set_7[] =
{
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {I386_ADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},98},
  {I386_SHL, 	{SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {I386_XADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*17*/
t_subs_ins set_7a[] =
{
  {I386_ADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},99},
  {I386_SHL, 	{SUB_CONST(1)},						{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {I386_XADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*prevents that 8 is found when lea(reg1,reg1,1)*/
/*18*/
t_subs_ins set_7b[] =
{
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {MAX_I386_OPCODE}
};

/*19*/
t_subs_ins set_7_a[] =
{
  {I386_ADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {I386_XADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {MAX_I386_OPCODE}
};

/*add reg1,reg2 == lea (reg1,reg2,1),reg2 == lea (reg2,reg1,1),reg2*/
/*{lea} {add}*/
/*20*/
t_subs_ins set_8[] =
{
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_loc2)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},1},
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc2,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},5},
  {I386_ADD, 	{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc2)},94},
  {MAX_I386_OPCODE}
};

/*21*/
t_subs_ins set_8a[] =
{
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_loc2)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {I386_LEA, 	{SUB_LEA_CONST_LONG(0,locmode_loc2,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {MAX_I386_OPCODE}
};

/*lea imm1(reg1,reg1,1),reg2 == lea imm1(,reg1,2),reg2*/
/*{lea}*/
/*22*/
t_subs_ins set_9[] =
{
  {I386_LEA,{SUB_LEA(immmode_imm1,locmode_loc1,locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {I386_LEA,{SUB_LEA_SCALE(I386_SCALE_2,immmode_imm1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {MAX_I386_OPCODE}
};

/*lea imm1(reg1,reg2,1),reg2 == lea imm1(reg2,reg1,1),reg2*/
/*{lea}*/
/*23*/
t_subs_ins set_10[] =
{
  {I386_LEA, 	{SUB_LEA(immmode_imm1,locmode_loc1,locmode_loc2)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},61},
  {I386_LEA, 	{SUB_LEA(immmode_imm1,locmode_loc2,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},39},
  {MAX_I386_OPCODE}
};

/*mov imm1,reg1 == lea imm1,reg1*/
/*{mov,lea}*/
/*24*/
t_subs_ins set_11[] = 
{
  {I386_LEA,	{SUB_LEA_IMM(immmode_imm1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_MOV,	{SUB_IMM(immmode_imm1)},	{SUB_NONE},	{SUB_LOC(locmode_loc1)},100},
  {MAX_I386_OPCODE}
};

/*mov reg1, reg2 == lea (reg1),reg2 == imul $0x1,reg1,reg2 == lea(,reg1,1),reg2*/
/*{lea,mov} {imul}*/
/*25*/
t_subs_ins set_12[] =
{
  {I386_LEA, 		{SUB_LEA_CONST_LONG(0,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},0},
  {I386_LEA, 		{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},0},
  {I386_MOV,		{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc2)},100},
  {I386_IMULexp2, 	{SUB_LOC(locmode_loc1)},				{SUB_CONST(1)},	{SUB_LOC(locmode_loc2)},0},
  {MAX_I386_OPCODE}
};

/*26*/
t_subs_ins set_12a[] =
{
  {I386_LEA, 		{SUB_LEA_CONST_LONG(0,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},0},
  {I386_LEA, 		{SUB_LEA_CONST_LONG(0,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},0},
  {I386_MOV,		{SUB_LOC(locmode_loc1)},				{SUB_NONE},	{SUB_LOC(locmode_loc2)},100},
  {MAX_I386_OPCODE}
};

/*add imm1,reg1 == sub -imm1,reg1 == lea imm1(reg1),reg1 == lea imm1(,reg1,1),reg1*/
/*{add},{sub},{lea}*/
/*27*/
t_subs_ins set_13[] =
{
  {I386_LEA, {SUB_LEA(immmode_imm1,locmode_loc1,locmode_invalid)}, 	{SUB_NONE},	{SUB_LOC(locmode_loc1)},1},
  {I386_LEA, {SUB_LEA(immmode_imm1,locmode_invalid,locmode_loc1)}, 	{SUB_NONE},	{SUB_LOC(locmode_loc1)},0},
  {I386_ADD, {SUB_IMM(immmode_imm1)},					{SUB_NONE},	{SUB_LOC(locmode_loc1)},50},
  {I386_SUB, {SUB_IMM(immmode_imm1neg)},				{SUB_NONE}, 	{SUB_LOC(locmode_loc1)},50},
  {MAX_I386_OPCODE}
};

/*28*/
t_subs_ins set_13a[] =
{
  {I386_ADD, {SUB_IMM(immmode_imm1)},					{SUB_NONE},	{SUB_LOC(locmode_loc1)},55},
  {I386_SUB, {SUB_IMM(immmode_imm1neg)},				{SUB_NONE}, 	{SUB_LOC(locmode_loc1)},45},
  {MAX_I386_OPCODE}
};

/*29*/
t_subs_ins set_13b[] =
{
  {I386_LEA, {SUB_LEA(immmode_imm1,locmode_loc1,locmode_invalid)}, 	{SUB_NONE},	{SUB_LOC(locmode_loc1)},50},
  {I386_LEA, {SUB_LEA(immmode_imm1,locmode_invalid,locmode_loc1)}, 	{SUB_NONE},	{SUB_LOC(locmode_loc1)},50},
  {MAX_I386_OPCODE}
};

/*lea imm(reg1),reg2 == lea imm(,reg1,1),reg2*/
/*{lea}*/
/*30*/
t_subs_ins set_14[] =
{
  {I386_LEA, 	{SUB_LEA(immmode_imm1,locmode_invalid,locmode_loc1)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {I386_LEA, 	{SUB_LEA(immmode_imm1,locmode_loc1,locmode_invalid)},	{SUB_NONE},	{SUB_LOC(locmode_loc2)},50},
  {MAX_I386_OPCODE}
};

/*cmp reg1,reg2 == cmp reg2,reg1*/
/*enkel als enkel ZF levend is*/
/*t_subs_ins set_15[] = 
{
  {I386_CMP,	{SUB_LOC(locmode_loc1)},	{SUB_LOC(locmode_loc2)},	{SUB_NONE}},
  {I386_CMP,	{SUB_LOC(locmode_loc2)},	{SUB_LOC(locmode_loc1)},	{SUB_NONE}},
  {MAX_I386_OPCODE}
};*/

t_subs_ins * AllSets[]=
{
  set_0,
/*  set_0a,*/
/*  set_0b,*/
  set_1,
/*  set_1a,*/
  set_2,
/*  set_2a,*/
/*  set_2_a,*/
/*  set_2b,*/
  set_3,
/*  set_3a,*/
/*  set_3_a,*/
/*  set_3b,*/
/*  set_4,*/
/*  set_5,*/
  set_6,
  set_7,
/*  set_7a,*/
/*  set_7b,*/
/*  set_7_a,*/
  set_8,
/*  set_8a,*/
/*  set_9,*/
  set_10,
/*  set_11,*/
/*  set_12,*/
/*  set_12a,*/
  set_13,
/*  set_13a,*/
/*  set_13b,*/
/*  set_14,*/
  NULL
};

/*ATTENTION: sequence is relevant*/
t_subs_ins * ACOPSZ_are_dead[]=
{
  set_0,
  set_1,
  set_2,
  set_3,
  set_4,
  set_5,
  set_6,
  set_7,
  set_8,
  set_9,
  set_10,
  set_11,
  /*TODO check why 12 doesnt always work*/
/*  set_12,*/
  set_12a,
  set_13,
  set_14,
/*  set_15,*/
  NULL
};

t_subs_ins * ACOPS_are_dead[]=
{
  set_0a,
  set_0b,
  set_1a,
  set_2a,
  set_2b,
  set_3a,
  set_3b,
  set_4,
  set_6,
  set_7a,
  set_7b,
  set_8a,
  set_9,
  set_10,
  set_11,
  set_12a,
  set_13a,
  set_13b,
  set_14,
/*  set_15,*/
  NULL
};

t_subs_ins * AC_are_dead[]=
{
  set_0a,
  set_0b,
  set_1a,
  set_2a,
  set_2b,
  set_3a,
  set_3b,
  set_4,
  set_6,
  set_7a,
  set_7b,
  set_8a,
  set_9,
  set_10,
  set_11,
  set_12a,
  set_13a,
  set_13b,
  set_14,
  NULL
};

t_subs_ins * A_is_dead[]=
{
  set_0a,
  set_0b,
  set_1a,
  set_2b,
  set_3b,
  set_6,
  set_7a,
  set_7b,
  set_8a,
  set_9,
  set_10,
  set_11,
  set_12a,
  set_13b,
  set_14,
  NULL
};

t_subs_ins * C_is_dead[]=
{
  set_0a,
  set_0b,
  set_1a,
  set_2_a,
  set_2b,
  set_3_a,
  set_3b,
  set_4,
  set_6,
  set_7_a,
  set_7b,
  set_8a,
  set_9,
  set_10,
  set_11,
  set_12a,
  set_13b,
  set_14,
  NULL
};

t_subs_ins * none_are_dead[]=
{
  set_0a,
  set_0b,
  set_1a,
  set_2b,
  set_3b,
  set_6,
  set_7_a,
  set_7b,
  set_8a,
  set_9,
  set_10,
  set_11,
  set_12a,
  set_13b,
  set_14,
  NULL
};
/* }}} */

t_subs_loc loc1;
t_subs_loc loc2;
t_uint32 imm1 = 0;
t_uint32 imm2 = 0;

t_regset all_flags;
t_bool stego_table_initialized=FALSE;
t_section * sec;

t_subs_ins ** ReturnSet(t_regset dead)
/*{{{*/
{
  if(RegsetIn(dead, I386_CONDREG_AF)
      && RegsetIn(dead, I386_CONDREG_CF)
      && RegsetIn(dead, I386_CONDREG_OF)
      && RegsetIn(dead, I386_CONDREG_PF)
      && RegsetIn(dead, I386_CONDREG_SF)
      && RegsetIn(dead, I386_CONDREG_ZF)
    )
    return ACOPSZ_are_dead;
  else if(RegsetIn(dead, I386_CONDREG_AF)
      && RegsetIn(dead, I386_CONDREG_CF)
      && RegsetIn(dead, I386_CONDREG_OF)
      && RegsetIn(dead, I386_CONDREG_PF)
      && RegsetIn(dead, I386_CONDREG_SF)
      )
    return ACOPS_are_dead;
  else if(RegsetIn(dead, I386_CONDREG_AF)
      && RegsetIn(dead, I386_CONDREG_CF)
      )
    return AC_are_dead;
  else if(RegsetIn(dead, I386_CONDREG_AF))
    return A_is_dead;
  else if(RegsetIn(dead, I386_CONDREG_CF))
    return C_is_dead;
  else return none_are_dead;
}
/*}}}*/

t_bool FillInOperand(t_i386_operand * op, t_subs_op * sub)
/*{{{*/
{
  switch(sub->type)
  {
    case optype_none:
      I386_OP_TYPE(op)=i386_optype_none;
      return TRUE;
    case optype_imm:
      I386_OP_TYPE(op)=i386_optype_imm;
      I386_OP_IMMEDSIZE(op)=4;
      
      switch(sub->immediate.mode)
      {
	case immmode_imm1:
	  I386_OP_IMMEDIATE(op)=imm1;
	  break;
	case immmode_imm1neg:
	  I386_OP_IMMEDIATE(op)=-((t_int32)imm1);
	  break;
	case immmode_constant:
	  I386_OP_IMMEDIATE(op)=sub->immediate.constant;
	  break;
	default:
	  FATAL(("unexpected mode"));
	  return FALSE;
      }
      return TRUE;
    case optype_lea:
      I386_OP_TYPE(op)=i386_optype_mem;
      I386_OP_MEMOPSIZE(op)=0;
      I386_OP_SCALE(op)=sub->scale;
      I386_OP_IMMEDSIZE(op)=4;
      
      switch(sub->immediate.mode)
      {
	case immmode_imm1:
	  I386_OP_IMMEDIATE(op)=imm1;
	  break;
	case immmode_imm1neg:
	  I386_OP_IMMEDIATE(op)=-imm1;
	  break;
	case immmode_constant:
	  I386_OP_IMMEDIATE(op)=sub->immediate.constant;
	  break;
	default:
	  FATAL(("unexpected mode"));
	  return FALSE;
      }
      I386_OP_REGMODE(op) = i386_regmode_full32;
      
      if(sub->mode1 == locmode_loc1)
      {
	if(loc1.type != loctype_reg)
	  return FALSE;
	I386_OP_BASE(op) = I386_OP_BASE(&loc1.operand);
      }
      else if(sub->mode1 == locmode_loc2)
      {
	if(loc2.type != loctype_reg)
	  return FALSE;
	I386_OP_BASE(op) = I386_OP_BASE(&loc2.operand);
      }
      else I386_OP_BASE(op) = I386_REG_NONE;
      
      if(sub->mode2 == locmode_loc1)
      {
	if(loc1.type != loctype_reg)
	  return FALSE;
	I386_OP_INDEX(op) = I386_OP_BASE(&loc1.operand);
      }
      else if(sub->mode2 == locmode_loc2)
      {
	if(loc2.type != loctype_reg)
	  return FALSE;
	I386_OP_INDEX(op) = I386_OP_BASE(&loc2.operand);
      }
      else I386_OP_INDEX(op) = I386_REG_NONE;
      
      return TRUE;
      
    case optype_loc:
      if(sub->mode1 == locmode_loc1)
      {
	*op = loc1.operand;
      }
      else if(sub->mode1 == locmode_loc2)
	*op = loc2.operand;
      return TRUE;      

    default:
      FATAL(("shouldnt happen"));
      return FALSE;
  }
}
/*}}}*/

t_i386_ins * InsNewStego()
  /*{{{*/
{
  t_i386_ins * ret = T_I386_INS(Calloc(1,sizeof(t_i386_ins)));
 // t_ins_stego * ins_stego = Calloc(1,sizeof(t_ins_stego));
//  PWord_t JudyValue;

//  JLI(JudyValue, JudyMapStego, ret);
//  *JudyValue = ins_stego;

//  RELOCATABLE_RELOCATABLE_SET_TYPE(ret,RT_INS);
  I386_INS_SET_SECTION(ret,sec);
  I386_INS_SET_CONDITION(ret,I386_CONDITION_NONE);
  return ret;
}
/*}}}*/

static void AddAlternative(t_ins * add_to, t_ins * to_be_added, t_uint8 encoding)
{
  VERBOSE(0,("@I alternative:",add_to));
  VERBOSE(0,("@I\n",to_be_added));
}

t_i386_ins * AddAllAlternatives(t_i386_ins * add_to, t_i386_ins * to_be_added)
  /*{{{Adds to_be_added with different immediate sizes to add_to*/
{
  int orig_size;
//  T_I386_INS(to_be_added)->encoding = 0;

  if(I386_INS_OPCODE(add_to)==I386_INS_OPCODE(to_be_added))
    return NULL;

  if((I386_OPSZPREF(add_to) || I386_ADSZPREF(add_to)))
  {
    printf("if((I386_OPSZPREF(add_to) || I386_ADSZPREF(add_to))\n");
//    if(INS_ALTERNATIVES(add_to)->count==0)
//      AddAlternative(add_to,add_to,0);
    return NULL;
  }

  if(I386_INS_OPCODE(to_be_added)==I386_LEA)
  {
//    orig_size =  I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added));

    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
    else
      FATAL(("ERR"));
#if 0
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=0;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
//      AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=1;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=4;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=orig_size;
#endif
  }
  else if(I386_OP_TYPE(I386_INS_SOURCE1(to_be_added))==i386_optype_imm)
  {
    orig_size = I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added));

      I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=1;
      if(I386GetPossibleEncodings(to_be_added, forms)>0)
	return to_be_added;
//	AddAlternative(add_to, to_be_added,0);

    if(I386_INS_OPCODE(to_be_added)==I386_RET)
    {
      I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=2;
      if(I386GetPossibleEncodings(to_be_added, forms)>0)
	return to_be_added;
	//AddAlternative(add_to, to_be_added,0);
    }

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=4;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(to_be_added))=orig_size;
  }
  else if(I386_OP_TYPE(I386_INS_SOURCE2(to_be_added))==i386_optype_imm)
  {
    orig_size = I386_OP_IMMEDSIZE(I386_INS_SOURCE2(to_be_added));

    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(to_be_added))=1;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(to_be_added))=4;
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(to_be_added))=orig_size;
  }
  else 
    if(I386GetPossibleEncodings(to_be_added, forms)>0)
      return to_be_added;
      //AddAlternative(add_to, to_be_added,0);

  return NULL;
}
/*}}}*/

void FreeCodeByte(t_codebyte * codebyte)
{
  CodebyteUnlinkFromCfg(codebyte);
  StatelistKill(CODEBYTE_STATELIST(codebyte));
  Free(codebyte);
}

/* t_bool GenerateConstructionInBbl(t_bbl* to_construct, t_bbl *bbl ,t_i386_ins * where, t_bool before) {{{ */
t_bool GenerateConstructionInBbl(t_bbl* to_construct, t_bbl *bbl ,t_i386_ins * where, t_bool before)
{
  t_ins * to_ins;
  t_i386_ins * insert_ins;
  t_uint32 pos=0;
  t_cfg * cfg= BBL_CFG(I386_INS_BBL(where));
  t_i386_ins * after=where;
  
  /* Check: No generation of a basic block into "virtual" code*/
  t_codebyte * codebyte;
  t_state_ref * state_ref, * state_ref2;
  t_state * state;
  INS_FOREACH_CODEBYTE(T_INS(where),codebyte,state_ref)
  {
    t_int32 i=0;

    CODEBYTE_FOREACH_STATE(codebyte,state,state_ref2)
      i++;
    if(i!=1)
      return FALSE;
  }

  BBL_FOREACH_INS(to_construct,to_ins)
  {
    INS_FOREACH_STATE(to_ins,state,state_ref)
    {
      insert_ins = I386InsNewForBbl(bbl);
      I386InstructionMakeMovToMemLen(insert_ins,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,(t_uint32)STATE_VALUE(state),1);
      if(before)
      {
	I386InsInsertBefore (insert_ins, where);
      }
      else
      {
	I386InsInsertAfter (insert_ins, after);
	after=insert_ins;
      }
      RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(insert_ins),0x2,T_RELOCATABLE(to_construct),pos,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
      I386_OP_FLAGS(I386_INS_DEST(insert_ins)) =I386_OPFLAG_ISRELOCATED;
      pos++;
    }
  }

  return TRUE;
}
/* }}} */

/* void Split(t_ins * ins1, t_ins * ins2) {{{ */
void Split(t_ins * ins1, t_ins * ins2)
{
  t_cfg * cfg=BBL_CFG(INS_BBL(ins2));

  if(INS_INEXT(ins1)!=ins2)
    FATAL(("ERR\n"));
  {

    //VERBOSE(0,("@I en der achter: @I\n",ins1,ins2));
    VERBOSE(0,("@I en @I\n",ins1,ins2));
    BblSplitBlock(INS_BBL(ins1),ins1,TRUE);
    BblSplitBlock(INS_BBL(ins2),ins2,TRUE);
    BblSplitBlock(INS_BBL(ins2),ins2,FALSE);

    t_cfg_edge * edge,*tmp;
    t_bbl* head=NULL,*tail=NULL;

    BBL_FOREACH_PRED_EDGE(INS_BBL(ins1),edge)
    {
      if(head==NULL)
	head=CFG_EDGE_HEAD(edge);
      else
	FATAL(("ERR\n"));
    }
    
    if(head==NULL)
      FATAL(("ERR\n"));
    
    BBL_FOREACH_SUCC_EDGE(INS_BBL(ins2),edge)
    {
      if(tail==NULL)
	tail=CFG_EDGE_TAIL(edge);
      else
	FATAL(("ERR\n"));
    }

    BBL_FOREACH_PRED_EDGE_SAFE(INS_BBL(ins2),edge,tmp)
    {
      CfgEdgeKill(edge);
    }

    CfgEdgeCreate(cfg,head,INS_BBL(ins2),ET_FALLTHROUGH);
    CfgEdgeCreate(cfg,INS_BBL(ins1),tail,ET_FALLTHROUGH);

    //to:
    t_state_ref * to_state_ref=STATELIST_FIRST(INS_STATELIST(ins1));
    //from:
    t_state_ref * fr_state_ref=STATELIST_FIRST(INS_STATELIST(ins2));
    do
    {
      //to:
      t_codebyte * to_codebyte=STATE_CODEBYTE(STATE_REF_STATE(to_state_ref));

      StateAddToStatelist(STATE_REF_STATE(fr_state_ref),CODEBYTE_STATELIST(to_codebyte));
      FreeCodeByte(STATE_CODEBYTE(STATE_REF_STATE(fr_state_ref)));
      STATE_SET_CODEBYTE(STATE_REF_STATE(fr_state_ref),STATE_CODEBYTE(STATE_REF_STATE(to_state_ref)));

      to_state_ref=to_state_ref->next;
      fr_state_ref=fr_state_ref->next;
      
    }while(fr_state_ref!=NULL && to_state_ref!=NULL);

    if(fr_state_ref!=NULL)
    {
      do{
	t_i386_ins * insert_ins;
	insert_ins = I386InsNewForBbl(INS_BBL(ins1));
	I386InstructionMakeNoop(insert_ins);
	I386InsInsertAfter (insert_ins, T_I386_INS(BBL_INS_LAST(INS_BBL(ins1))));

	to_state_ref=STATELIST_FIRST(INS_STATELIST(T_INS(insert_ins)));
	t_codebyte * to_codebyte=STATE_CODEBYTE(STATE_REF_STATE(to_state_ref));
     
StateAddToStatelist(STATE_REF_STATE(fr_state_ref),CODEBYTE_STATELIST(to_codebyte));
	FreeCodeByte(STATE_CODEBYTE(STATE_REF_STATE(fr_state_ref)));
	STATE_SET_CODEBYTE(STATE_REF_STATE(fr_state_ref),STATE_CODEBYTE(STATE_REF_STATE(to_state_ref)));
	
	fr_state_ref=fr_state_ref->next;
      }while(fr_state_ref);
    }
    else if(to_state_ref!=NULL)
    {
      do{
	t_ins * insert_ins;
	insert_ins = InsNewForBbl(INS_BBL(ins2));
	I386InstructionMakeNoop(T_I386_INS(insert_ins));
	InsInsertAfter (insert_ins, BBL_INS_LAST(INS_BBL(ins2)));

	fr_state_ref=STATELIST_FIRST(INS_STATELIST(insert_ins));
	t_codebyte * to_codebyte=STATE_CODEBYTE(STATE_REF_STATE(to_state_ref));

	StateAddToStatelist(STATE_REF_STATE(fr_state_ref),CODEBYTE_STATELIST(to_codebyte));
	FreeCodeByte(STATE_CODEBYTE(STATE_REF_STATE(fr_state_ref)));
	STATE_SET_CODEBYTE(STATE_REF_STATE(fr_state_ref),STATE_CODEBYTE(STATE_REF_STATE(to_state_ref)));

	to_state_ref=to_state_ref->next;
      }while(to_state_ref);
    }
  }
}
/* }}} */

static t_bbl * GetAlternativesFromSet(t_i386_ins * ins_orig, t_subs_ins * set)
  /* {{{ */
{
  int i=0;
  t_i386_ins * ins_to_add;
  do
  {
    if(I386_INS_OPCODE(ins_orig)==set[i].opcode)
      continue;
    
    ins_to_add = Malloc(sizeof(t_i386_ins));
    I386_INS_SET_CONDITION(ins_to_add, I386_CONDITION_NONE);

    I386_INS_SET_SOURCE1(ins_to_add,Malloc(sizeof(t_i386_operand)));
    I386_INS_SET_SOURCE2(ins_to_add,Malloc(sizeof(t_i386_operand)));
    I386_INS_SET_DEST(ins_to_add,Malloc(sizeof(t_i386_operand)));
    
    I386_INS_SET_OPCODE(ins_to_add,set[i].opcode);
    if (! FillInOperand(I386_INS_SOURCE1(ins_to_add), &set[i].source1) 
	|| !FillInOperand(I386_INS_SOURCE2(ins_to_add), &set[i].source2) 
	|| !FillInOperand(I386_INS_DEST(ins_to_add), &set[i].dest)
	)
    {
      Free(I386_INS_SOURCE1(ins_to_add));
      Free(I386_INS_SOURCE2(ins_to_add));
      Free(I386_INS_DEST(ins_to_add));
      Free(ins_to_add);
      continue; 
    }
    I386InsSetOperandFlags(ins_to_add);
 
    if(I386GetPossibleEncodings(ins_to_add, forms)>0)
    {
      Free(I386_INS_SOURCE1(ins_to_add));
      Free(I386_INS_SOURCE2(ins_to_add));
      Free(I386_INS_DEST(ins_to_add));
      Free(ins_to_add);

      ins_to_add = I386InsNewForBbl(I386_INS_BBL(ins_orig));
      
      I386ClearIns(ins_to_add);
      I386_INS_SET_OPCODE(ins_to_add,set[i].opcode);
      FillInOperand(I386_INS_SOURCE1(ins_to_add), &set[i].source1);
      FillInOperand(I386_INS_SOURCE2(ins_to_add), &set[i].source2);
      FillInOperand(I386_INS_DEST(ins_to_add), &set[i].dest);
      I386SetGenericInsInfo(ins_to_add);

      /* Write out Original instruction */
//      I386InsInsertAfter (ins_to_add,ins_orig);
//      Split(T_INS(ins_orig),T_INS(ins_to_add));
      /* Write out New instruction */
      I386InsInsertBefore (ins_to_add,ins_orig);
      Split(T_INS(ins_to_add),T_INS(ins_orig));

      //VERBOSE(0,("Alternative: @I\n",ins_to_add));
      return I386_INS_BBL(ins_to_add);

      //printf("OK\n");
    }
    else
    {
      Free(I386_INS_SOURCE1(ins_to_add));
      Free(I386_INS_SOURCE2(ins_to_add));
      Free(I386_INS_DEST(ins_to_add));
      Free(ins_to_add);
      //printf("NOK\n");
    }
  }while(set[++i].opcode != MAX_I386_OPCODE);

  //FATAL((""));
  return NULL;
  
}
/* }}} */

static void Initialize(t_i386_ins * ins)
/*{{{*/
{
//  INS_ALTERNATIVES(ins) = Calloc(1,sizeof(t_ins_alternatives));
//  INS_ALTERNATIVES(ins)->count = 0;
  
  if(stego_table_initialized)
    return;
  
  sec = I386_INS_SECTION(ins);
  stego_table_initialized = TRUE;
  all_flags = RegsetNew();
  RegsetSetAddReg(all_flags, I386_CONDREG_OF);
  RegsetSetAddReg(all_flags, I386_CONDREG_SF);
  RegsetSetAddReg(all_flags, I386_CONDREG_ZF);
  RegsetSetAddReg(all_flags, I386_CONDREG_AF);
  RegsetSetAddReg(all_flags, I386_CONDREG_PF);
  RegsetSetAddReg(all_flags, I386_CONDREG_CF);
}
/*}}}*/

static t_bool MatchLoc(t_i386_operand * op, t_subs_op * sub)
/*{{{*/
{
  switch(I386_OP_TYPE(op))
  {
    case i386_optype_mem:
      if(sub->mode1 == locmode_loc1)
      {
	if(loc1.type != loctype_invalid)
	  return FALSE;
	loc1.type = loctype_mem;
	loc1.operand = *op;
      }
      else if(sub->mode1 == locmode_loc2)
      {
	if(loc2.type != loctype_invalid)
	  return FALSE;
	loc2.type = loctype_mem;
	loc2.operand = *op;
      }
      else FATAL((""));
      return TRUE;
    case i386_optype_reg:
      if(sub->mode1 == locmode_loc1)
      {
	if(loc1.type != loctype_invalid)
	{
	  if(!(loc1.type == loctype_reg && I386_OP_BASE(&loc1.operand) == I386_OP_BASE(op) && I386_OP_REGMODE(&loc1.operand) == I386_OP_REGMODE(op)))
	    return FALSE;
	  loc1.operand = *op;
	  return TRUE;
	}
	loc1.type = loctype_reg;
	loc1.operand = *op;
      }
      else if(sub->mode1 == locmode_loc2){
	if(loc2.type != loctype_invalid)
	{
	  if(!(loc2.type == loctype_reg && I386_OP_BASE(&loc2.operand) == I386_OP_BASE(op) && I386_OP_REGMODE(&loc2.operand) == I386_OP_REGMODE(op)))
	    return FALSE;
	  loc2.operand = *op;
	  return TRUE;
	}
	loc2.type = loctype_reg;
	loc2.operand = *op;
      }
      else FATAL((""));
      return TRUE;
    default:
      return FALSE;
  }
}
/*}}}*/

static t_bool MatchReg(t_reg reg, t_i386_regmode regmode, t_locmode mode)
  /*{{{*/
{
  if(mode == locmode_loc1)
  {
    if(reg==I386_REG_NONE)
      return FALSE;
    if(loc1.type != loctype_invalid)
    {
      if(!(loc1.type == loctype_reg && I386_OP_BASE(&loc1.operand) == reg && I386_OP_REGMODE(&loc1.operand) == regmode))
	return FALSE;
      return TRUE;
    }
    loc1.type = loctype_reg;
    I386_OP_TYPE(&loc1.operand) = i386_optype_reg;
    I386_OP_BASE(&loc1.operand) = reg;
    I386_OP_REGMODE(&loc1.operand) = regmode;
    I386_OP_FLAGS(&loc1.operand)&=~I386_OPFLAG_ISRELOCATED;
    return TRUE;
  }
  else if(mode == locmode_loc2)
  {
    if(reg==I386_REG_NONE)
      return FALSE;
    if(loc2.type != loctype_invalid)
    {
      if(!(loc2.type == loctype_reg && I386_OP_BASE(&loc2.operand) == reg && I386_OP_REGMODE(&loc2.operand) == regmode))
	return FALSE;
      return TRUE;
    }
    loc2.type = loctype_reg;
    I386_OP_TYPE(&loc2.operand) = i386_optype_reg;
    I386_OP_BASE(&loc2.operand) = reg;
    I386_OP_REGMODE(&loc2.operand) = regmode;
    I386_OP_FLAGS(&loc2.operand)&=~I386_OPFLAG_ISRELOCATED;
    return TRUE;
  }
  else if(mode == locmode_invalid)
  {
    return reg == I386_REG_NONE;
  }
  return FALSE;
}
/*}}}*/

static t_bool MatchImm(t_uint32 imm, t_subs_imm sub)
/*{{{*/
{
  switch(sub.mode)
  {
    case immmode_constant:
      return imm == sub.constant;
    case immmode_imm1:
      imm1 = imm;
      return TRUE;
    case immmode_imm2:
      imm2 = imm;
      return TRUE;
    case immmode_imm1neg:
      imm1 = -((t_int32)imm);
      return TRUE;
    case immmode_imm2neg:
      imm2 = -((t_int32)imm);
      return TRUE;
    case immmode_invalid:
      return FALSE;
    default:
      FATAL(("shouldnt happen"));
      return FALSE;
  }
}
/*}}}*/

static t_bool MatchOperand(t_i386_operand * op, t_subs_op * sub)
/*{{{*/
{
  switch(sub->type)
  {
    case optype_none:
      return I386_OP_TYPE(op) == i386_optype_none;
      break;
    case optype_lea:
      if(I386_OP_TYPE(op) != i386_optype_mem)
	return FALSE;
      if(I386_OP_MEMOPSIZE(op) != 0)
	return FALSE;
      if(I386_OP_SCALE(op) != sub->scale)
	return FALSE;

      if(I386_OP_REGMODE(op) != i386_regmode_full32)
	return FALSE;

      if(!MatchReg(I386_OP_BASE(op),I386_OP_REGMODE(op),sub->mode1))
	return FALSE;
      if(!MatchReg(I386_OP_INDEX(op),I386_OP_REGMODE(op),sub->mode2))
	return FALSE;

      if(!MatchImm(I386_OP_IMMEDIATE(op),sub->immediate))
	return FALSE;
      return TRUE;
      break;

    case optype_imm:
      if(I386_OP_TYPE(op) != i386_optype_imm)
	return FALSE;
      return MatchImm(I386_OP_IMMEDIATE(op),sub->immediate);
      break;

    case optype_loc:
      return MatchLoc(op,sub);
      break;

    default:
      FATAL(("shouldnt happen"));
      return FALSE;
      break;      
  }
}
/*}}}*/

static t_bool MatchIns(t_i386_ins * ins, t_subs_ins * sub)
/*{{{*/  
{
  loc1.type = loctype_invalid;
  loc2.type = loctype_invalid;

  if(I386_INS_OPCODE(ins)!=sub->opcode)
    return FALSE;
  if(!MatchOperand(I386_INS_SOURCE1(ins), &sub->source1))
    return FALSE;
  if(!MatchOperand(I386_INS_SOURCE2(ins), &sub->source2))
    return FALSE;
  if(!MatchOperand(I386_INS_DEST(ins), &sub->dest))
    return FALSE;

  return TRUE;
}
/*}}}*/

t_bbl * I386InsFindAlternativesTable(t_i386_ins * ins, t_regset dead)
/*{{{*/
{
  t_subs_ins * set_walker;
  int i,j;
  t_subs_ins ** set;
  Initialize(ins);

  set = ReturnSet(dead);

  if(!I386_OPSZPREF(ins) && !I386_ADSZPREF(ins))
    for(i=0, set_walker = set[i]; set_walker!=NULL; set_walker=set[++i])
    {
      for(j=0; set_walker[j].opcode != MAX_I386_OPCODE; j++)
      {
	if(MatchIns(ins, &set_walker[j]))
	{
	  //VERBOSE(0,("@I\n",ins));
	  t_bbl * virtual_bbl = GetAlternativesFromSet(ins, set_walker);
//	  FATAL(("FOUND!\n"));
//	  if(INS_ALTERNATIVES(ins)->count == 0){
//	    FATAL(("shouldnt happen @I",ins));
//	  }
//	  SetRelocateFlags(ins);
	  return virtual_bbl;
	}
      }
    }
//  RevertToConservative(ins);
//  if(INS_ALTERNATIVES(ins)->count == 0){
  //  InsKill(ins);
//  }
//  SetRelocateFlags(ins);
  return NULL;
}
/*}}}*/

/* void SmcFirstTransformation (t_cfg * cfg) {{{ */
t_bool SmcFirstTransformation (t_cfg * cfg)
{
  //printf("SmcFirstTransformation\n");
  t_function * fun;
  //t_uint32 tel=0;
  t_bbl * bbl/*,*split_off1,*split_off2, *split_off3*/;
  t_bbl *virtual;
  t_regset dead = RegsetNew();

  CFG_FOREACH_FUN(cfg,fun)
  {
      FUNCTION_FOREACH_BBL(fun,bbl)
      {
	t_ins * ins;
//	printf("%s\n",FUNCTION_NAME(fun));
	dead = CFG_DESCRIPTION(cfg)->all_registers;
	RegsetSetDiff(dead, BBL_REGS_LIVE_OUT(bbl));
	BBL_FOREACH_INS_R(bbl,ins)
	{
	  t_codebyte * codebyte;
	  t_state_ref * state_ref, * state_ref2;
	  t_state * state;
	  t_bool already_duplicated=FALSE;

	  INS_FOREACH_CODEBYTE(ins,codebyte,state_ref)
	  {
	    t_int32 i=0;

	    CODEBYTE_FOREACH_STATE(codebyte,state,state_ref2)
	      i++;
	    if(i!=1)
	      already_duplicated=TRUE;
	  }

	  if(I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	    already_duplicated=TRUE;
	  if(I386_OP_FLAGS(I386_INS_SOURCE2(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	    already_duplicated=TRUE;
	  if(I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	    already_duplicated=TRUE;

	  if(!already_duplicated)
	  {
	    virtual=I386InsFindAlternativesTable(T_I386_INS(ins),dead);

	    if(virtual)
	    {
	      t_bbl * bbl1;

	      FUNCTION_FOREACH_BBL(fun,bbl1)
	      {
		if(BBL_INS_FIRST(bbl1) && I386_INS_OPCODE(T_I386_INS(BBL_INS_FIRST(bbl1)))==I386_POP)
		{
		//  printf("production of the ins\n");

		  if(GenerateConstructionInBbl(virtual, bbl1 , T_I386_INS(BBL_INS_FIRST(bbl1)), FALSE))
		    return TRUE;
		}
	      }
	      return TRUE;
	    }
	  }
	  RegsetSetDiff(dead, INS_REGS_USE(ins));
	}
    }
  }
  return FALSE;
}
/* }}} */

#if 0
/* Test Function; works for bzip2! {{{ */
void SmcFirstTransformation (t_cfg * cfg)
{
  printf("SmcFirstTransformation\n");
  t_function * fun;
  t_uint32 tel=0;
  t_bbl * bbl,*split_off1,*split_off2, *split_off3;

  CFG_FOREACH_FUN(cfg,fun)
  {
    if(!strcmp(FUNCTION_NAME(fun),"_init"))
    {
      printf("FOUND!\n");
      FUNCTION_FOREACH_BBL(fun,bbl)
      {
//	VERBOSE(0,("bbl_tel:%d @B\n",tel,bbl));
	if(tel==0)
	{
	  t_ins *ins,*target_ins,*new_ins ;
	  t_uint32 tel_ins=0;

	  BBL_FOREACH_INS(bbl,ins)
	  {
	    VERBOSE(0,("@I\n",ins));
	    if(tel_ins==2)
	      target_ins=ins;
	    if(tel_ins==2)
	    {
	      t_ins * insert_ins, * new_ins;
	      insert_ins = InsNewForBbl(bbl);
	      I386InstructionMakeMovToMemLen(insert_ins,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0x90,1);
	      InsInsertAfter (insert_ins, ins);
	      RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(insert_ins),0x2,T_RELOCATABLE(target_ins),0x0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	      I386_OP_FLAGS(I386_INS_DEST(insert_ins)) =I386_OPFLAG_ISRELOCATED;

	      split_off1=BblSplitBlock(INS_BBL(target_ins),target_ins,FALSE);
	      split_off2=BblSplitBlock(INS_BBL(target_ins),target_ins,TRUE);

	      new_ins = InsNewForBbl(bbl);
	      I386InstructionMakeNoop(new_ins);
	      InsInsertAfter (new_ins,target_ins);
	      
	      split_off3=BblSplitBlock(INS_BBL(new_ins),new_ins,TRUE);

	      t_cfg_edge * edge,*tmp;
	      t_bbl* head=NULL,*tail=NULL;
	      BBL_FOREACH_PRED_EDGE(INS_BBL(target_ins),edge)
	      {
		if(head==NULL)
		  head=CFG_EDGE_HEAD(edge);
		else
		  FATAL(("ERR\n"));
	      }
	      if(head==NULL)
		FATAL(("ERR\n"));
	      BBL_FOREACH_SUCC_EDGE(INS_BBL(new_ins),edge)
	      {
		if(tail==NULL)
		  tail=CFG_EDGE_TAIL(edge);
		else
		  FATAL(("ERR\n"));
	      }

	      BBL_FOREACH_PRED_EDGE_SAFE(INS_BBL(new_ins),edge,tmp)
	      {
		CfgEdgeKill(edge);
	      }

	      CfgEdgeCreate(cfg,head,INS_BBL(new_ins),ET_FALLTHROUGH);
	      CfgEdgeCreate(cfg,INS_BBL(target_ins),tail,ET_FALLTHROUGH);

	      StateAddToStatelist(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(new_ins))),CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(target_ins))))));
	     

	     {
	       t_codebyte_ref * codebyte_ref = CODEBYTE_CODEBYTE_REF(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(new_ins)))));
	       if(CODEBYTE_REF_PREV(codebyte_ref))
		 CODEBYTE_REF_SET_NEXT(CODEBYTE_REF_PREV(codebyte_ref),CODEBYTE_REF_NEXT(codebyte_ref));
	       if(CODEBYTE_REF_NEXT(codebyte_ref))
		 CODEBYTE_REF_SET_PREV(CODEBYTE_REF_NEXT(codebyte_ref),CODEBYTE_REF_PREV(codebyte_ref));
	       StatelistKill(CODEBYTE_STATELIST(CODEBYTE_REF_CODEBYTE(codebyte_ref)));
	       Free(CODEBYTE_REF_CODEBYTE(codebyte_ref));
	       Free(codebyte_ref);
	     }
	     
	     STATE_SET_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(new_ins))),STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(target_ins)))));

	     printf("%x\n",STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(new_ins)))));

//	     printf("%x\n",STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(target_ins)))));
//	     printf("%x\n",STATE_VALUE(STATE_REF_STATE(STATE_REF_NEXT(STATELIST_FIRST(INS_STATELIST(target_ins))))));
	     printf("%x\n",STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(target_ins)))))))));
	     printf("%x\n",STATE_VALUE(STATE_REF_STATE(STATE_REF_NEXT(STATELIST_FIRST(CODEBYTE_STATELIST(STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(target_ins))))))))));
	     
	     t_codebyte * codebyte;
	     t_state_ref * state_ref;
	     t_state_ref * state_ref2;
	     t_state * state;

  INS_FOREACH_CODEBYTE(target_ins,codebyte,state_ref)
  {
    printf("\n==========\n%x\n=============\n",CODEBYTE_CADDRESS(codebyte));
    
    CODEBYTE_FOREACH_STATE(codebyte,state,state_ref2)
      printf( "  %x\n",STATE_VALUE(state));
      
  }
  
//   STATE_REF_SET_NEXT(STATELIST_LAST(state_list),state_item);

//  if(INS_STATELIST(new_ins))
//    StatelistKill(INS_STATELIST(new_ins));
//  INS_SET_STATELIST(new_ins,INS_STATELIST(target_ins));
//  codebyte=
 //   t_state * state = NewStateForCodebyte(codebyte);

//  SmcAddCodebytes(buf,length,ins,FALSE);

	      
//	      t_state * 
//	      StateAddToIns(,target_ins)

  break;
	    }
	    tel_ins++;
	  }
	}
	tel++;
      }
    }
  }
}
/* }}} */
#endif

/* vim: set shiftwidth=2 foldmethod=marker:*/
