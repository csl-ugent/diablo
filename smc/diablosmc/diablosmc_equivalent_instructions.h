#include <diablosmc.h>
#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloi386.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloi386.h>
#endif
#ifndef SMCEQUIVALENTINSTRUCTIONS_H
#define SMCEQUIVALENTINSTRUCTIONS_H

/* structs en zo {{{ */
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

#define SUB_CONST(imm) 	optype_imm, I386_SCALE_INVALID, {immmode_constant, imm}, locmode_invalid, locmode_invalid
#define SUB_IMM(IMM) 	optype_imm, I386_SCALE_INVALID, {IMM, 0}, locmode_invalid, locmode_invalid
#define SUB_LOC(LOC)	optype_loc, I386_SCALE_INVALID, {immmode_invalid, 0}, LOC, locmode_invalid
#define SUB_NONE	optype_none, I386_SCALE_INVALID, {immmode_invalid, 0}, locmode_invalid, locmode_invalid

#define SUB_LEA_CONST(imm) optype_lea, I386_SCALE_1, {immmode_constant, imm}, locmode_invalid, locmode_invalid
#define SUB_LEA_CONST_LONG(imm,loc1,loc2) optype_lea, I386_SCALE_1, {immmode_constant, imm}, loc1, loc2
#define SUB_LEA(IMM,LOC1,LOC2) optype_lea, I386_SCALE_1, {IMM,0}, LOC1, LOC2
#define SUB_LEA_SCALE(SCALE,IMM,LOC1,LOC2) optype_lea, SCALE, {IMM,0}, LOC1, LOC2
#define SUB_LEA_IMM(IMM) SUB_LEA(IMM,locmode_invalid,locmode_invalid)
/* }}} */
t_bool SmcFirstTransformation (t_cfg * cfg);
t_bbl * I386InsFindAlternativesTable(t_i386_ins * ins, t_regset dead);
t_bool GenerateConstructionInBbl(t_bbl* to_construct, t_bbl *bbl ,t_i386_ins * where, t_bool before);

#endif

/* vim: set shiftwidth=2 foldmethod=marker:*/
