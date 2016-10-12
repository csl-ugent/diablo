/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */

#ifdef __cplusplus
extern "C" {
#endif
#include <diabloanopti386.h>
#include <diablodiversity.h>
#ifdef __cplusplus
}
#endif

/*{{{ Dynamic Members*/
typedef struct _t_ins_alternatives t_ins_alternatives;

struct _t_ins_alternatives {
  /*number of alternatives*/
  t_uint8 count;
  /*alternatives*/
  t_ins ** ins;
};

t_dynamic_member_info ins_alternatives_array = null_info;

void InsAlternativesInit(t_ins * ins, t_ins_alternatives ** alternatives)
{
  (*alternatives) = (t_ins_alternatives*)Calloc(1,sizeof(t_ins_alternatives));
  (*alternatives)->count = 0;
  (*alternatives)->ins = NULL;
}

void InsAlternativesFini(t_ins * ins, t_ins_alternatives ** alternatives)
{
  int i;
  /*
  for(i=0;i<(*alternatives)->count;i++)
    InsFree((*alternatives)->ins[i]);
  if((*alternatives)->ins)
    Free((*alternatives)->ins);
    */
}

void InsAlternativesDup(t_ins * ins, t_ins_alternatives ** alternatives)
{
  return;
}

DYNAMIC_MEMBER(
    ins,
    t_cfg *,
    ins_alternatives_array,
    t_ins_alternatives *,
    alternatives,
    ALTERNATIVES,
    Alternatives,
    CFG_FOREACH_INS,
    InsAlternativesInit,
    InsAlternativesFini,
    InsAlternativesDup    
)
/*}}}*/

/*{{{ typedefs */
typedef struct _t_subs_ins t_subs_ins;
typedef struct _t_subs_op t_subs_op;
typedef struct _t_subs_imm t_subs_imm;
typedef struct _t_subs_loc t_subs_loc;

typedef enum {
  loctype_reg = 0, 
  loctype_mem,
  loctype_invalid
} t_loctype;

typedef enum {
  immmode_imm1 =0,
  immmode_imm1neg,
  immmode_imm2,
  immmode_imm2neg,
  immmode_constant,
  immmode_invalid
} t_immmode;

struct _t_subs_loc
{
  t_loctype type;
  t_i386_operand operand;
};

struct _t_subs_imm
{
  t_immmode mode;
  t_uint32 constant;
};

typedef enum {
  optype_none = 0, 
  optype_lea,  
  optype_loc,
  optype_imm 
} t_optype;

typedef enum {
  locmode_loc1 = 0,
  locmode_loc2,
  locmode_loc3,
  locmode_invalid
} t_locmode;

struct _t_subs_op
{
  t_optype type;
  t_uint8 scale;
  t_subs_imm immediate;
  t_locmode mode1;
  t_locmode mode2;
};

struct _t_subs_ins
{
  t_i386_opcode opcode;
  
  t_subs_op source1;
  t_subs_op source2;
  t_subs_op dest;

  t_uint8 chance;
};
/*}}}*/

/*Sets of Alternative Instructions {{{*/
#define SUB_CONST(imm) 	optype_imm, I386_SCALE_INVALID, {immmode_constant, (t_uint32)imm}, locmode_invalid, locmode_invalid
#define SUB_IMM(IMM) 	optype_imm, I386_SCALE_INVALID, {IMM, 0}, locmode_invalid, locmode_invalid
#define SUB_LOC(LOC)	optype_loc, I386_SCALE_INVALID, {immmode_invalid, 0}, LOC, locmode_invalid
#define SUB_NONE	optype_none, I386_SCALE_INVALID, {immmode_invalid, 0}, locmode_invalid, locmode_invalid

#define SUB_LEA_CONST(imm) optype_lea, I386_SCALE_1, {immmode_constant, (t_uint32)imm}, locmode_invalid, locmode_invalid
#define SUB_LEA_CONST_LONG(imm,loc1,loc2) optype_lea, I386_SCALE_1, {immmode_constant, (t_uint32)imm}, loc1, loc2
#define SUB_LEA(IMM,LOC1,LOC2) optype_lea, I386_SCALE_1, {IMM,0}, LOC1, LOC2
#define SUB_LEA_SCALE(SCALE,IMM,LOC1,LOC2) optype_lea, SCALE, {IMM,0}, LOC1, LOC2
#define SUB_LEA_IMM(IMM) SUB_LEA(IMM,locmode_invalid,locmode_invalid)

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
/*}}}*/

static t_ins * InsNewNew() /*{{{*/
{
  t_ins * ret = T_INS(Calloc(1,sizeof(t_i386_ins)));
  
  RELOCATABLE_SET_RELOCATABLE_TYPE(T_RELOCATABLE(ret),RT_INS);
  I386ClearIns(T_I386_INS(ret));
  I386_INS_SET_CONDITION(T_I386_INS(ret),I386_CONDITION_NONE);
  return ret;
}
/*}}}*/

static void InvertRelocation(t_ins * from, t_i386_operand * from_relocated_operand, t_ins * to) /*{{{*/
{
  t_reloc * reloc = I386GetRelocForOp(T_I386_INS(from), from_relocated_operand);
  t_string staart;
  t_string nieuw;
  t_string nieuw2;
  
  staart = strchr(RELOC_CODE(reloc), '\\');
  staart[0] = 0;
  staart++;
  nieuw = StringConcat3("s0000",RELOC_CODE(reloc),"-\\");
  nieuw2 = StringConcat2(nieuw,staart);

  Free(nieuw);
  Free(RELOC_CODE(reloc));

  RELOC_SET_CODE(reloc,nieuw2);
}
/*}}}*/

static void InvertRelocationIfNecessary(t_ins * from, t_ins * to) /*{{{*/
{
  t_i386_operand * from_relocated_operand = NULL;
  t_i386_operand * to_relocated_operand = NULL;
  
  if(I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(from)))&I386_OPFLAG_ISRELOCATED)
    from_relocated_operand = I386_INS_SOURCE1(T_I386_INS(from));
  if(I386_OP_FLAGS(I386_INS_SOURCE2(T_I386_INS(from)))&I386_OPFLAG_ISRELOCATED)
    from_relocated_operand = I386_INS_SOURCE2(T_I386_INS(from));
  if(I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(from)))&I386_OPFLAG_ISRELOCATED)
    from_relocated_operand = I386_INS_DEST(T_I386_INS(from));

  if(from_relocated_operand == NULL)
    return;
  
  if(I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(to)))&I386_OPFLAG_ISRELOCATED)
    to_relocated_operand = I386_INS_SOURCE1(T_I386_INS(to));
  if(I386_OP_FLAGS(I386_INS_SOURCE2(T_I386_INS(to)))&I386_OPFLAG_ISRELOCATED)
    to_relocated_operand = I386_INS_SOURCE2(T_I386_INS(to));
  if(I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(to)))&I386_OPFLAG_ISRELOCATED)
    to_relocated_operand = I386_INS_DEST(T_I386_INS(to));
  
  if(to_relocated_operand == NULL){
    VERBOSE(0,("@I,@I",from,to));
    FATAL(("shouldnt happen"));
    return;
  }

  if(I386_OP_IMMEDIATE(from_relocated_operand) == -I386_OP_IMMEDIATE(to_relocated_operand))
    InvertRelocation(from, from_relocated_operand, to);
}
/*}}}*/

void I386MorphInstructionTo(t_ins * from, t_ins * to) /*{{{*/
{
  /*int i=0,j=0;*/
  if(to==NULL)
    FATAL(("shouldnt happen"));

  InvertRelocationIfNecessary(from, to);
  
  I386_INS_SET_OPCODE(T_I386_INS(from),I386_INS_OPCODE(T_I386_INS(T_I386_INS(to))));
  I386_INS_SET_CONDITION(T_I386_INS(from),I386_INS_CONDITION(T_I386_INS(to)));
  I386_INS_SET_FLAGS(T_I386_INS(from),I386_INS_FLAGS(T_I386_INS(to)));
  *I386_INS_SOURCE1(T_I386_INS(from))=*I386_INS_SOURCE1(T_I386_INS(to));
  *I386_INS_SOURCE2(T_I386_INS(from))=*I386_INS_SOURCE2(T_I386_INS(to));
  *I386_INS_DEST(T_I386_INS(from))   =*I386_INS_DEST(T_I386_INS(to));
  //INS_ENCODING(from) = INS_ENCODING(to);
  
  INS_SET_REGS_DEF(from,I386InsDefinedRegisters(T_I386_INS(from)));
  INS_SET_REGS_USE(from,I386InsUsedRegisters(T_I386_INS(from)));
  
  if(I386InsIsConditional(T_I386_INS(from)))
    I386_INS_SET_FLAGS(T_I386_INS(from), I386_INS_FLAGS(T_I386_INS(from))|IF_CONDITIONAL);
  else
    I386_INS_SET_FLAGS(T_I386_INS(from), I386_INS_FLAGS(T_I386_INS(from))&~IF_CONDITIONAL);

  //INS_OPCODE_BYTE(from) = INS_OPCODE_BYTE(to);
  
  I386InsSetOperandFlags(T_I386_INS(from));

  return;
}
/*}}}*/

static void AddFlagTo(t_ins * ins, t_uint32 imm) /*{{{*/
{
  if((I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(ins)))==i386_optype_mem || I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(ins)))==i386_optype_imm)
      && I386_OP_IMMEDIATE(I386_INS_SOURCE1(T_I386_INS(ins)))==imm)
    I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(ins)))|=I386_OPFLAG_ISRELOCATED;
  if((I386_OP_TYPE(I386_INS_SOURCE2(T_I386_INS(ins)))==i386_optype_mem || I386_OP_TYPE(I386_INS_SOURCE2(T_I386_INS(ins)))==i386_optype_imm)
      && I386_OP_IMMEDIATE(I386_INS_SOURCE2(T_I386_INS(ins)))==imm)
    I386_OP_FLAGS(I386_INS_SOURCE2(T_I386_INS(ins)))|=I386_OPFLAG_ISRELOCATED;
  if((I386_OP_TYPE(I386_INS_DEST(T_I386_INS(ins)))==i386_optype_mem || I386_OP_TYPE(I386_INS_DEST(T_I386_INS(ins)))==i386_optype_imm)
      && I386_OP_IMMEDIATE(I386_INS_DEST(T_I386_INS(ins)))==imm)
    I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(ins)))|=I386_OPFLAG_ISRELOCATED;
}
/*}}}*/

static void RestoreRelocateFlag(t_ins * ins, t_i386_operand * op) /*{{{*/
{
  t_uint32 imm;
  int i;
  t_reloc * rel;
  if(!(I386_OP_FLAGS(op) & I386_OPFLAG_ISRELOCATED))
    return;
  if(I386_OP_TYPE(op)!=i386_optype_imm && !(I386_INS_OPCODE(T_I386_INS(ins)) == I386_LEA && I386_OP_TYPE(op)==i386_optype_mem))
    return;
  
  imm = I386_OP_IMMEDIATE(op);
  if(imm == 0){
    rel = I386GetRelocForOp(T_I386_INS(ins),op);
    if(rel != NULL 
	/*RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_SECTION && 
	(
	 RELOC_TO_RELOCATABLE(rel)[0] == T_RELOCATABLE(OBJECT_ABS_SECTION(INS_OBJECT(ins))) ||
	 RELOC_TO_RELOCATABLE(rel)[0] == T_RELOCATABLE(OBJECT_UNDEF_SECTION(INS_OBJECT(ins)))
	)*/
      ){
      I386_OP_FLAGS(op)&=~I386_OPFLAG_ISRELOCATED;
      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(INS_OBJECT(ins)),rel);
      return;
    }
    else
    {
      FATAL(("This shouldn't happen.\nRelocation: @R\nInstruction: @I\n", rel , ins));
    }
  }
  for(i=0;i<INS_ALTERNATIVES(ins)->count;i++){
    AddFlagTo(INS_ALTERNATIVES(ins)->ins[i],imm);
    AddFlagTo(INS_ALTERNATIVES(ins)->ins[i],-imm);
  }

  AddFlagTo(ins,imm);  
}
/*}}}*/

static void SetRelocateFlags(t_ins * ins) /*{{{*/
{
  int i;

  RestoreRelocateFlag(ins,I386_INS_SOURCE1(T_I386_INS(ins)));
  RestoreRelocateFlag(ins,I386_INS_SOURCE2(T_I386_INS(ins)));
  RestoreRelocateFlag(ins,I386_INS_DEST(T_I386_INS(ins)));
  
  for(i=0;i<INS_ALTERNATIVES(ins)->count;i++)
  {
    INS_SET_REGS_DEF(INS_ALTERNATIVES(ins)->ins[i],I386InsDefinedRegisters(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i])));
    INS_SET_REGS_USE(INS_ALTERNATIVES(ins)->ins[i],I386InsUsedRegisters(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i])));

    if(I386InsIsConditional(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i])))
      I386_INS_SET_FLAGS(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i]), I386_INS_FLAGS(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i]))|IF_CONDITIONAL);
    else
      I386_INS_SET_FLAGS(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i]), I386_INS_FLAGS(T_I386_INS(INS_ALTERNATIVES(ins)->ins[i]))&~IF_CONDITIONAL);
  }
}
/*}}}*/

static void AddAlternative(t_ins * add_to, t_ins * to_be_added/*, t_uint8 encoding*/) /*{{{*/
{
//  PWord_t JudyValue;
//  t_ins_stego * ins_stego = Calloc(1,sizeof(t_ins_stego));
  
  if(INS_ALTERNATIVES(add_to)->count % 5 == 0){
    if(INS_ALTERNATIVES(add_to)->count==0)
      INS_ALTERNATIVES(add_to)->ins = (t_ins**) Malloc(5*sizeof(t_i386_ins *));
    else
      INS_ALTERNATIVES(add_to)->ins = (t_ins**) Realloc(INS_ALTERNATIVES(add_to)->ins, (INS_ALTERNATIVES(add_to)->count+5)*sizeof(t_i386_ins *));
  }
  
//  INS_NR(to_be_added) = INS_NR(add_to);
  INS_ALTERNATIVES(add_to)->ins[INS_ALTERNATIVES(add_to)->count]=(t_ins*) Malloc(sizeof(t_i386_ins));
  
//  JLI(JudyValue, JudyMapStego, INS_ALTERNATIVES(add_to)->ins[INS_ALTERNATIVES(add_to)->count]);
//  *JudyValue = ins_stego;
  
  *T_I386_INS(INS_ALTERNATIVES(add_to)->ins[INS_ALTERNATIVES(add_to)->count]) = *T_I386_INS(to_be_added);
//  INS_ENCODING(INS_ALTERNATIVES(add_to)->ins[INS_ALTERNATIVES(add_to)->count]) = encoding;
    
  INS_ALTERNATIVES(add_to)->count++;
  return;
}
/*}}}*/

static t_bool FillInOperand(t_i386_operand * op, t_subs_op * sub, t_subs_loc loc1, t_subs_loc loc2, t_uint32 imm1) /*{{{*/
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

static void AddAlternativeWithShortestImmediates(t_ins * add_to, t_ins * to_be_added) /*{{{Adds to_be_added with different immediate sizes to add_to*/
{
  int orig_size;
  t_i386_opcode_entry * forms[10];   /* this is certainly large enough */
  
  if(I386_INS_OPCODE(T_I386_INS(to_be_added))==I386_LEA){
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=0;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
      
      AddAlternative(add_to, to_be_added);
      return;
    }

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=1;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
      AddAlternative(add_to, to_be_added);
      return;
    }
    
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=4;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
      AddAlternative(add_to, to_be_added);
      return;
    }
  }
  else if(I386_OP_TYPE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))==i386_optype_imm){
    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=1;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
      AddAlternative(add_to, to_be_added);
      return;
    }

    if(I386_INS_OPCODE(T_I386_INS(to_be_added))==I386_RET)
    {
      I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=2;
      if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
	AddAlternative(add_to, to_be_added);
	return;
      }
    }

    I386_OP_IMMEDSIZE(I386_INS_SOURCE1(T_I386_INS(to_be_added)))=4;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0)
    {
      AddAlternative(add_to, to_be_added);
      return;
    }
  }
  else if(I386_OP_TYPE(I386_INS_SOURCE2(T_I386_INS(to_be_added)))==i386_optype_imm){
    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(T_I386_INS(to_be_added)))=1;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0){
      AddAlternative(add_to, to_be_added);
      return;
    }

    I386_OP_IMMEDSIZE(I386_INS_SOURCE2(T_I386_INS(to_be_added)))=4;
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0)
    {
      AddAlternative(add_to, to_be_added);
      return;
    }
  }
  else 
  {
    if(I386GetPossibleEncodings(T_I386_INS(to_be_added), forms)>0)
      AddAlternative(add_to, to_be_added);
  }
}
/*}}}*/

static void GetAlternativesFromSet(t_ins * ins_orig, t_subs_ins * set, t_subs_loc loc1, t_subs_loc loc2, t_uint32 imm1) /*{{{*/
{
  int i=0;
  t_ins * ins_to_add;
  do
  {
    ins_to_add = InsNewNew();
    I386_INS_SET_OPCODE(T_I386_INS(ins_to_add), set[i].opcode);
    if (! FillInOperand(I386_INS_SOURCE1(T_I386_INS(ins_to_add)), &set[i].source1, loc1, loc2, imm1) 
	|| !FillInOperand(I386_INS_SOURCE2(T_I386_INS(ins_to_add)), &set[i].source2, loc1, loc2, imm1) 
	|| !FillInOperand(I386_INS_DEST(T_I386_INS(ins_to_add)), &set[i].dest, loc1, loc2, imm1)
	)
    {
      Free(ins_to_add);
      continue; 
    }
    I386InsSetOperandFlags(T_I386_INS(ins_to_add));
//    I386DoSetSize(ins_to_add,I386_INS_DEST(ins_to_add));
//    I386DoSetSize(ins_to_add,I386_INS_SOURCE1(ins_to_add));
//    I386DoSetSize(ins_to_add,I386_INS_SOURCE2(ins_to_add));
    AddAlternativeWithShortestImmediates(ins_orig,ins_to_add);
    Free(ins_to_add);
  }while(set[++i].opcode != MAX_I386_OPCODE);
  
  {
    int count = INS_ALTERNATIVES(ins_orig)->count;
    int i;
    for(i=0;i<count;i++){
      SetRelocateFlags(ins_orig);
    }
  }
}
/*}}}*/

static t_bool MatchLoc(t_i386_operand * op, t_subs_op * sub, t_subs_loc * loc1, t_subs_loc * loc2) /*{{{*/
{
  switch(I386_OP_TYPE(op))
  {
    case i386_optype_mem:
      if(sub->mode1 == locmode_loc1)
      {
	if(loc1->type != loctype_invalid)
	  return FALSE;
	loc1->type = loctype_mem;
	loc1->operand = *op;
      }
      else if(sub->mode1 == locmode_loc2)
      {
	if(loc2->type != loctype_invalid)
	  return FALSE;
	loc2->type = loctype_mem;
	loc2->operand = *op;
      }
      else FATAL((""));
      return TRUE;
    case i386_optype_reg:
      if(sub->mode1 == locmode_loc1)
      {
	if(loc1->type != loctype_invalid)
	{
	  if(!(loc1->type == loctype_reg && I386_OP_BASE(&loc1->operand) == I386_OP_BASE(op) && I386_OP_REGMODE(&loc1->operand) == I386_OP_REGMODE(op)))
	    return FALSE;
	  loc1->operand = *op;
	  return TRUE;
	}
	loc1->type = loctype_reg;
	loc1->operand = *op;
      }
      else if(sub->mode1 == locmode_loc2){
	if(loc2->type != loctype_invalid)
	{
	  if(!(loc2->type == loctype_reg && I386_OP_BASE(&loc2->operand) == I386_OP_BASE(op) && I386_OP_REGMODE(&loc2->operand) == I386_OP_REGMODE(op)))
	    return FALSE;
	  loc2->operand = *op;
	  return TRUE;
	}
	loc2->type = loctype_reg;
	loc2->operand = *op;
      }
      else FATAL((""));
      return TRUE;
    default:
      return FALSE;
  }
}
/*}}}*/

static t_bool MatchReg(t_reg reg, t_i386_regmode regmode, t_locmode mode, t_subs_loc * loc1, t_subs_loc * loc2) /*{{{*/
{
  if(mode == locmode_loc1)
  {
    if(reg==I386_REG_NONE)
      return FALSE;
    if(loc1->type != loctype_invalid)
    {
      if(!(loc1->type == loctype_reg && I386_OP_BASE(&loc1->operand) == reg && I386_OP_REGMODE(&loc1->operand) == regmode))
	return FALSE;
      return TRUE;
    }
    loc1->type = loctype_reg;
    I386_OP_TYPE(&loc1->operand) = i386_optype_reg;
    I386_OP_BASE(&loc1->operand) = reg;
    I386_OP_REGMODE(&loc1->operand) = regmode;
    I386_OP_FLAGS(&loc1->operand)&=~I386_OPFLAG_ISRELOCATED;
    return TRUE;
  }
  else if(mode == locmode_loc2)
  {
    if(reg==I386_REG_NONE)
      return FALSE;
    if(loc2->type != loctype_invalid)
    {
      if(!(loc2->type == loctype_reg && I386_OP_BASE(&loc2->operand) == reg && I386_OP_REGMODE(&loc2->operand) == regmode))
	return FALSE;
      return TRUE;
    }
    loc2->type = loctype_reg;
    I386_OP_TYPE(&loc2->operand) = i386_optype_reg;
    I386_OP_BASE(&loc2->operand) = reg;
    I386_OP_REGMODE(&loc2->operand) = regmode;
    I386_OP_FLAGS(&loc2->operand)&=~I386_OPFLAG_ISRELOCATED;
    return TRUE;
  }
  else if(mode == locmode_invalid)
  {
    return (reg == I386_REG_NONE);
  }
  return FALSE;
}
/*}}}*/

static t_bool MatchImm(t_uint32 imm, t_subs_imm sub, t_uint32 * imm1) /*{{{*/
{
  switch(sub.mode)
  {
    case immmode_constant:
      return (imm == sub.constant);
    case immmode_imm1:
      (*imm1) = imm;
      return TRUE;
    case immmode_imm2:
      //imm2 = imm;
      return TRUE;
    case immmode_imm1neg:
      (*imm1) = -((t_int32)imm);
      return TRUE;
    case immmode_imm2neg:
      //imm2 = -((t_int32)imm);
      return TRUE;
    case immmode_invalid:
      return FALSE;
    default:
      FATAL(("shouldnt happen"));
      return FALSE;
  }
}
/*}}}*/

static t_bool MatchOperand(t_i386_operand * op, t_subs_op * sub, t_subs_loc * loc1, t_subs_loc * loc2, t_uint32 * imm1) /*{{{*/
{
  switch(sub->type)
  {
    case optype_none:
      return (I386_OP_TYPE(op) == i386_optype_none);
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
      
      if(!MatchReg(I386_OP_BASE(op),I386_OP_REGMODE(op),sub->mode1,loc1,loc2))
	return FALSE;
      if(!MatchReg(I386_OP_INDEX(op),I386_OP_REGMODE(op),sub->mode2,loc1,loc2))
	return FALSE;
      
      if(!MatchImm(I386_OP_IMMEDIATE(op),sub->immediate, imm1))
	return FALSE;
      return TRUE;
      break;
      
    case optype_imm:
      if(I386_OP_TYPE(op) != i386_optype_imm)
	return FALSE;
      return MatchImm(I386_OP_IMMEDIATE(op),sub->immediate, imm1);
      break;
      
    case optype_loc:
      return MatchLoc(op,sub,loc1,loc2);
      break;
      
    default:
      FATAL(("shouldnt happen"));
      return FALSE;
      break;      
  }
}
/*}}}*/

static t_bool MatchIns(t_i386_ins * ins, t_subs_ins * sub, t_subs_loc * loc1, t_subs_loc * loc2, t_uint32 * imm1) /*{{{*/  
{
  loc1->type = loctype_invalid;
  loc2->type = loctype_invalid;
  
  if(I386_INS_OPCODE(ins)!=sub->opcode)
    return FALSE;
  if(!MatchOperand(I386_INS_SOURCE1(ins), &sub->source1, loc1, loc2, imm1))
    return FALSE;
  if(!MatchOperand(I386_INS_SOURCE2(ins), &sub->source2, loc1, loc2, imm1))
    return FALSE;
  if(!MatchOperand(I386_INS_DEST(ins), &sub->dest, loc1, loc2, imm1))
    return FALSE;
  
  return TRUE;
}
/*}}}*/

static t_subs_ins ** ReturnSet(t_regset dead)/*{{{*/
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

static void I386InsFindAlternativesTable(t_ins * ins, t_regset dead)/*{{{*/
{
  t_subs_ins * set_walker;
  int i,j;
  t_subs_ins ** set;

  set = ReturnSet(dead);
  
  t_i386_ins* ins386 = T_I386_INS(ins);
  
  if(!I386_OPSZPREF(ins386) && !I386_ADSZPREF(ins386))
    for(i=0, set_walker = set[i]; set_walker!=NULL; set_walker=set[++i])
    {
      for(j=0; set_walker[j].opcode != MAX_I386_OPCODE; j++)
      {
	t_subs_loc loc1;
	t_subs_loc loc2;
	t_uint32 imm1 = 0;
	if(MatchIns(ins386, &set_walker[j], &loc1, &loc2, &imm1))
	{
	  GetAlternativesFromSet(ins, set_walker, loc1, loc2, imm1);
	  if(INS_ALTERNATIVES(ins)->count == 0){
	    I386DoSetSize(ins386,I386_INS_DEST(ins386));
	    I386DoSetSize(ins386,I386_INS_SOURCE1(ins386));
	    I386DoSetSize(ins386,I386_INS_SOURCE2(ins386));
	    if(MatchIns(ins386, &set_walker[j], &loc1, &loc2, &imm1))
	    {
	      GetAlternativesFromSet(ins, set_walker, loc1, loc2, imm1);
	      if(INS_ALTERNATIVES(ins)->count == 0){
		FATAL(("shouldnt happen @I",ins));
	      }
	    }
	  }
	  //SetRelocateFlags(ins);
	  return;
	}
      }
    }
  return;
}/*}}}*/

t_ins * current_ins = NULL;

static void DiversityISMoveToNextInstruction(t_cfg * cfg)/*{{{*/
{
  if(current_ins && INS_INEXT(current_ins))
    current_ins = INS_INEXT(current_ins);
  else
  {
    t_bbl * bbl;
    /*First Time*/
    if(!current_ins)
    {
      bbl = T_BBL(CFG_NODE_FIRST(cfg));
      while(!BBL_INS_FIRST(bbl))
	bbl = BBL_NEXT(bbl);
      current_ins = BBL_INS_FIRST(bbl);
    }
    /*End of Bbl*/
    else
    {
      bbl = INS_BBL(current_ins);
      do{
	/*All Instructions have been Processed*/
	if(!BBL_NEXT(bbl)) 
	{
	  current_ins = NULL;
	  return;
	}
	bbl = BBL_NEXT(bbl);
      }while(!BBL_INS_FIRST(bbl));

      current_ins = BBL_INS_FIRST(bbl);
    }
  }
}/*}}}*/

t_diversity_options DiversityInstructionSelection(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  
  /*If not first time, apply choice*/
  if(current_ins != NULL)
  {
    if(choice->choice != UINT64_MAX)
      I386MorphInstructionTo(current_ins, INS_ALTERNATIVES(current_ins)->ins[choice->choice]);
  }
  else
  {
    InsInitAlternatives(cfg);
  }

  do
  {
    DiversityISMoveToNextInstruction(cfg);
    /*This is the end*/
    if(current_ins == NULL)
    {
      ret.done = TRUE;
      InsFiniAlternatives(cfg);
    }
    else
    {
      I386InsFindAlternativesTable(current_ins,NullRegs);
      ret.range = (INS_ALTERNATIVES(current_ins)->count==0)?0:INS_ALTERNATIVES(current_ins)->count - 1;
      ret.flags = FALSE;
      ret.done = FALSE;
    }
  }while(!ret.done && ret.range == 0);

  return ret;
}/*}}}*/
/*}}}*/
