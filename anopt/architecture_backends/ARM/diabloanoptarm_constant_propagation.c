/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>

/*!
 * Counts the number of defined registers for an instruction
 *
 * \todo This is a weird function, check use, name, ... Its should be in
 * arm_utils it also uses 16 as the number of registers ... 
 *
 * \param ins the instruction for which we want to count the defined registers 
 *
 * \return unsigned int the numer of registers that is defined by this
 * instruction 
 */
/* ArmInsDefinedRegCount {{{ */

void MakeConstProducers2(t_arm_ins * ins, t_procstate *procstate);

t_arm_ins * FindCmpThatDeterminesJump(t_arm_ins * ins)
{
  ins = ARM_INS_IPREV(ins);

  while (ins)
    {
      if (ARM_INS_OPCODE(ins)==ARM_CMP)
	break;
      ins = ARM_INS_IPREV(ins);
    }

  return ins;
}

t_bool BblEndsWithConditionalBranchAfterCMP(t_bbl * bbl)
{
  t_arm_ins * last_arm_ins = T_ARM_INS(BBL_INS_LAST(bbl));
  t_arm_ins * ins;
  t_regset regs;
  t_regset def_regs = RegsetNew();

  if (ARM_INS_OPCODE(last_arm_ins)!=ARM_B) return FALSE;

  if (ARM_INS_CONDITION(last_arm_ins)!=ARM_CONDITION_EQ && ARM_INS_CONDITION(last_arm_ins)!=ARM_CONDITION_NE) return FALSE;

  regs = ARM_INS_REGS_USE(last_arm_ins);
  ins = ARM_INS_IPREV(last_arm_ins);

  while (ins)
    {
      if (ARM_INS_OPCODE(ins)==ARM_CMP)
	break;
      if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),regs)))
	ins = NULL;
      else
	{
	  RegsetSetUnion(def_regs,ARM_INS_REGS_DEF(ins));
	  ins = ARM_INS_IPREV(ins);
	}
    }

  if (ins==NULL)
    return FALSE;

  if (ARM_INS_REGC(ins)!=ARM_REG_NONE) return FALSE;

  if (RegsetIn(def_regs,ARM_INS_REGB(ins))) return FALSE;
  if (ArmInsIsConditional(ins)) return FALSE;
  
  return TRUE;
}

unsigned int ArmInsDefinedRegCount(t_arm_ins * ins) {
  t_reg i;
  int count;
  t_regset mask;

  mask = ArmDefinedRegisters(ins);
  count = 0;

  REGSET_FOREACH_REG(mask,i)
    count++;
  return count;
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetSingleCondition {{{ */
static void ARMTestAndSetSingleCondition(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_reg flag)
{
  t_bool value;
  if (CP_BOT==ProcStateGetCond(state,flag,&value))
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      ProcStateSetCond(*state_true,flag,TRUE);
      ProcStateSetCond(*state_false,flag,FALSE);
    }
  else if (value)
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_false = state;
      *state_true = NULL;
    }
}

static void ARMTestAndSetForCBZ(t_cfg_edge*edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate** state_conditional)
{
  t_arm_ins * last_arm_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  t_reg reg = ARM_INS_REGB(last_arm_ins);
  t_register_content content;

  if (!last_arm_ins || ARM_INS_OPCODE(last_arm_ins)!=ARM_T2CBZ)
  {
    /* needed because CBZ might already be converted to branch or eliminated */
     *state_true = state;
     *state_false = NULL;
     return;
  }

  if (CP_BOT==ProcStateGetReg(state,reg,&content))
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      content.i=AddressNew32(0);
      ProcStateSetReg(*state_true,reg,content);
      
      *state_conditional = ProcStateNew(&arm_description);
      ProcStateSetAllBot(*state_conditional, (&arm_description)->all_registers);
      ProcStateSetReg(*state_conditional, reg, content);
    }
  else if (G_T_UINT32(content.i)==0)
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_false = state;
      *state_true = NULL;
    }
}

static void ARMTestAndSetForCBNZ(t_cfg_edge*edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate** state_conditional)
{
  t_arm_ins * last_arm_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  t_reg reg = ARM_INS_REGB(last_arm_ins);
  t_register_content content;

  if (!last_arm_ins || ARM_INS_OPCODE(last_arm_ins)!=ARM_T2CBNZ)
  {
    /* needed because CBNZ might already be converted to branch or eliminated */
     *state_true = state;
     *state_false = NULL;
     return;
  }

  if (CP_BOT==ProcStateGetReg(state,reg,&content))
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      content.i=AddressNew32(0);
      ProcStateSetReg(*state_false,reg,content);
      
      *state_conditional = ProcStateNew(&arm_description);
      ProcStateSetAllBot(*state_conditional, (&arm_description)->all_registers);
      ProcStateSetReg(*state_conditional, reg, content);
    }
  else if (G_T_UINT32(content.i)!=0)
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_false = state;
      *state_true = NULL;
    }
}

static void ARMTestAndSetSingleConditionExt(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate** state_conditional, t_reg flag)
{
  t_bool value;
  /*#define TEST*/
#ifdef TEST
  static int teller = 0;
#endif
  if (CP_BOT==ProcStateGetCond(state,flag,&value))
    {
      t_arm_ins * last_arm_ins = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
      t_arm_ins * ins = FindCmpThatDeterminesJump(last_arm_ins);
      t_uint32 value = ARM_INS_IMMEDIATE(ins);
      t_reg reg = ARM_INS_REGB(ins);
      t_register_content content;
      content.i=AddressNew32(value);

      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      ProcStateSetCond(*state_true,flag,TRUE);
      ProcStateSetCond(*state_false,flag,FALSE);
      
      *state_conditional = ProcStateNew(&arm_description);
      ProcStateSetAllBot(*state_conditional, (&arm_description)->all_registers);
      ProcStateSetReg(*state_conditional, reg, content);

#ifdef TEST
      if (teller++<diablosupport_options.debugcounter)
#endif
	{
	  ProcStateSetReg(*state_true,reg,content);
	  ProcStateSetTagTop(*state_true,reg);
	  /*	  DiabloPrintArch(stdout,BBL_FUNCTION(CFG_EDGE_HEAD(edge))->cfg->sec->obj->description,"@iB\n @E\n @P\n", CFG_EDGE_HEAD(edge),edge,*state_true); */
	}
    }
  else if (value)
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_false = state;
      *state_true = NULL;
    }
}

static void ARMTestAndSetConditionCN_ALExt(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional) {
  switch (CFG_EDGE_CAT(edge)) {
  case ET_FALLTHROUGH:
  case ET_IPFALLTHRU:
    break;
  
  default: FATAL(("unhandled edge type @E", edge));
  }

  t_arm_ins *last = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  
  if (last
      && ArmIsControlflow(last)
      && ArmInsIsConditional(last)) {
    t_bool process = true;

    switch (ARM_INS_CONDITION(last)) {
    case ARM_CONDITION_EQ:
      process = false;
      break;

    case ARM_CONDITION_NE:
      break;

    default: FATAL(("unhandled condition @I in @eiB", last, CFG_EDGE_HEAD(edge)));
    }
    
    if (process) {
      /* */
      t_arm_ins * ins = FindCmpThatDeterminesJump(last);
      t_uint32 value = ARM_INS_IMMEDIATE(ins);
      t_reg reg = ARM_INS_REGB(ins);
      t_register_content content;
      content.i=AddressNew32(value);
      
      *state_conditional = ProcStateNew(&arm_description);
      ProcStateSetAllBot(*state_conditional, (&arm_description)->all_registers);
      ProcStateSetReg(*state_conditional, reg, content);
    }
  }
  
  /* regular ARMTestAndSetConditionCN_AL functionality */
  *state_true = state;
  *state_false = NULL;
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_EQ {{{ */
void ARMTestAndSetConditionCN_EQ(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_true, state_false, ARM_REG_Z_CONDITION);
}

void ARMTestAndSetConditionCN_EQExt(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate** state_conditional)
{
  ARMTestAndSetSingleConditionExt(edge,state, state_true, state_false, state_conditional, ARM_REG_Z_CONDITION);
}


/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_NE {{{ */
void ARMTestAndSetConditionCN_NE(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_false, state_true, ARM_REG_Z_CONDITION);
}

void ARMTestAndSetConditionCN_NEExt(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate** state_conditional)
{
  ARMTestAndSetSingleConditionExt(edge,state, state_false, state_true, state_conditional, ARM_REG_Z_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_CS {{{ */
void ARMTestAndSetConditionCN_CS(t_cfg_edge * edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_true, state_false, ARM_REG_C_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_CC {{{ */
void ARMTestAndSetConditionCN_CC(t_cfg_edge * edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_false, state_true, ARM_REG_C_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_MI {{{ */
void ARMTestAndSetConditionCN_MI(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge, state, state_true, state_false, ARM_REG_N_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_PL {{{ */
void ARMTestAndSetConditionCN_PL(t_cfg_edge * edge, t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge, state, state_false, state_true, ARM_REG_N_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_VS {{{ */
void ARMTestAndSetConditionCN_VS(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_true, state_false, ARM_REG_V_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_VC {{{ */
void ARMTestAndSetConditionCN_VC(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetSingleCondition(edge,state, state_false, state_true, ARM_REG_V_CONDITION);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_HI {{{ */
void ARMTestAndSetConditionCN_HI(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{ 
  t_bool c_flag;
  t_bool z_flag;
  t_lattice_level c_level = ProcStateGetCond(state,ARM_REG_C_CONDITION,&c_flag);
  t_lattice_level z_level = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&z_flag);

  if (CP_BOT==c_level && CP_BOT==z_level)
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      ProcStateSetCond(*state_true,ARM_REG_C_CONDITION,TRUE);
      ProcStateSetCond(*state_true,ARM_REG_Z_CONDITION,FALSE);
    }
  else if (CP_BOT==c_level)
    {
      if (!z_flag)
	{
	  *state_true  = ProcStateNew(&arm_description);
	  *state_false = state;
	  ProcStateDup(*state_true,state,&arm_description);
	  ProcStateSetCond(*state_true,ARM_REG_C_CONDITION,TRUE);
	  ProcStateSetCond(*state_false,ARM_REG_C_CONDITION,FALSE);
	}
      else
	{
	  *state_true = NULL;
	  *state_false = state;
	}
    }
  else if (CP_BOT==z_level)
    {
      if (c_flag)
	{
	  *state_true  = ProcStateNew(&arm_description);
	  *state_false = state;
	  ProcStateDup(*state_true,state,&arm_description);
	  ProcStateSetCond(*state_true,ARM_REG_Z_CONDITION,FALSE);
	  ProcStateSetCond(*state_false,ARM_REG_Z_CONDITION,TRUE);
	}
      else
	{
	  *state_true = NULL;
	  *state_false = state;
	}
    }
  else if (c_flag && !z_flag)
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_true  = NULL;
      *state_false = state;
    }
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_LS {{{ */
void ARMTestAndSetConditionCN_LS(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetConditionCN_HI(edge,state, state_false, state_true, state_conditional);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_GE {{{ */
void ARMTestAndSetConditionCN_GE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_bool n_flag;
  t_bool v_flag;
  t_lattice_level n_level = ProcStateGetCond(state,ARM_REG_N_CONDITION,&n_flag);
  t_lattice_level v_level = ProcStateGetCond(state,ARM_REG_V_CONDITION,&v_flag);

  if (CP_BOT==n_level && CP_BOT==v_level)
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
    }
  else if (CP_BOT==n_level)
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      if (v_flag)
	{
	  ProcStateSetCond(*state_true,ARM_REG_N_CONDITION,TRUE);
	  ProcStateSetCond(*state_false,ARM_REG_N_CONDITION,FALSE);
	}
      else
	{
	  ProcStateSetCond(*state_true,ARM_REG_N_CONDITION,FALSE);
	  ProcStateSetCond(*state_false,ARM_REG_N_CONDITION,TRUE);
	}
    }
  else if (CP_BOT==v_level)
    {
      *state_true  = ProcStateNew(&arm_description);
      *state_false = state;
      ProcStateDup(*state_true,state,&arm_description);
      if (n_flag)
	{
	  ProcStateSetCond(*state_true,ARM_REG_V_CONDITION,TRUE);
	  ProcStateSetCond(*state_false,ARM_REG_V_CONDITION,FALSE);
	}
      else
	{
	  ProcStateSetCond(*state_true,ARM_REG_V_CONDITION,FALSE);
	  ProcStateSetCond(*state_false,ARM_REG_V_CONDITION,TRUE);
	}
    }
  else if ((n_flag && v_flag)
	   ||
	   (!n_flag && ! v_flag)
	   )
    {
      *state_true = state;
      *state_false = NULL;
    }
  else
    {
      *state_true  = NULL;
      *state_false = state;
    }
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_LT {{{ */
void ARMTestAndSetConditionCN_LT(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetConditionCN_GE(edge,state, state_false, state_true, state_conditional);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_GT {{{ */
void ARMTestAndSetConditionCN_GT(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_bool z_flag;
  t_lattice_level z_level = ProcStateGetCond(state,ARM_REG_Z_CONDITION,&z_flag);

  if (CP_BOT==z_level)
    {
      ARMTestAndSetConditionCN_GE(edge,state, state_true, state_false, state_conditional);

      if (*state_true)
	ProcStateSetCond(*state_true,ARM_REG_Z_CONDITION,FALSE);
    }
  else if (z_flag)
    {
      *state_false = state;
      *state_true = NULL;
    }
  else
    {
      ARMTestAndSetConditionCN_GE(edge,state, state_true, state_false, state_conditional);
    }
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_LE {{{ */
void ARMTestAndSetConditionCN_LE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  ARMTestAndSetConditionCN_GT(edge,state, state_false, state_true, state_conditional);
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_AL {{{ */
void ARMTestAndSetConditionCN_AL(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_true = state;
  *state_false = NULL;
}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionCN_NV {{{ */
void ARMTestAndSetConditionCN_NV(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_false = state;
  *state_true = NULL;
}
/* }}} */
/*!
 *
 * \todo Document
 *
 * This is a function that is attached to system call edges 
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionSetAllToBot {{{ */
void ARMTestAndSetConditionSetAllToBot(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false)
{
  *state_true  = ProcStateNew(&arm_description);
  *state_false = state;
  ProcStateSetAllBot(*state_true,arm_description.all_registers);
}
/* }}} */
/*!
 * This is a function for return and compensating edges: information must be
 * propagated over all outgoing edges of an exit block, at least as far as we
 * are concerned. The constant propagator might add different constraints, such
 * as in a context-sensitive constant propagation. This function is also used
 * for hell edges.
 *
 * In general it is used where more than two successor edges are found in the
 * graph, which model anomalous control flow. They are not taken or not-taken
 * edges, but reflect different execution contexts.  
 *
 * \todo Document
 * 
 * \param edge
 * \param state
 * \param state_true
 * \param state_false
 * \param flag
 *
 * \return void
 */
/* ARMTestAndSetConditionAllPaths {{{ */
void ARMTestAndSetConditionAllPaths(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_true  = state;
  *state_false = state;
}
/* }}} */
/*! \todo Document */
TestAndSetConditionFunction ARMTestAndSetConditionFunctionsArray [] = { &ARMTestAndSetConditionCN_EQ,
									&ARMTestAndSetConditionCN_NE,
									&ARMTestAndSetConditionCN_CS,
									&ARMTestAndSetConditionCN_CC,
									&ARMTestAndSetConditionCN_MI,
									&ARMTestAndSetConditionCN_PL,
									&ARMTestAndSetConditionCN_VS,
									&ARMTestAndSetConditionCN_VC,
									&ARMTestAndSetConditionCN_HI,
									&ARMTestAndSetConditionCN_LS,
									&ARMTestAndSetConditionCN_GE,
									&ARMTestAndSetConditionCN_LT,
									&ARMTestAndSetConditionCN_GT,
									&ARMTestAndSetConditionCN_LE,
									&ARMTestAndSetConditionCN_AL,
									&ARMTestAndSetConditionCN_NV
};


TestAndSetConditionFunction ARMTestAndSetConditionFunctionsArrayExt [] = { &ARMTestAndSetConditionCN_EQExt,
									&ARMTestAndSetConditionCN_NEExt,
									&ARMTestAndSetConditionCN_CS,
									&ARMTestAndSetConditionCN_CC,
									&ARMTestAndSetConditionCN_MI,
									&ARMTestAndSetConditionCN_PL,
									&ARMTestAndSetConditionCN_VS,
									&ARMTestAndSetConditionCN_VC,
									&ARMTestAndSetConditionCN_HI,
									&ARMTestAndSetConditionCN_LS,
									&ARMTestAndSetConditionCN_GE,
									&ARMTestAndSetConditionCN_LT,
									&ARMTestAndSetConditionCN_GT,
									&ARMTestAndSetConditionCN_LE,
									&ARMTestAndSetConditionCN_ALExt,
									&ARMTestAndSetConditionCN_NV
};

/*! 
 * \todo Document
 *
 * \param edge
 * \param last_arm_ins
 *
 * \return void
 */
/* ArmEdgePropagator {{{ */

void ArmEdgePropagator(t_cfg_edge * edge, t_arm_ins * last_arm_ins)
{
  t_uint8  condition = last_arm_ins?ARM_INS_CONDITION(last_arm_ins):ARM_CONDITION_AL;
  t_uint16 edgetype = CFG_EDGE_CAT(edge);

  if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
    {
      CFG_EDGE_SET_TESTCONDITION(edge,  &ARMTestAndSetConditionAllPaths);
      return;
    }

  switch (edgetype)
  {
    case ET_JUMP:
#if 0
      /* Dominique: turned this off, as it seems pretty useless? */
      /* this is a special case: jump tables jumps are unconditional,
	 but we need to propagate to all the blocks in the jump table */

      if (CFG_EDGE_FLAGS(edge) & EF_FROM_SWITCH_TABLE)
      {
	CFG_EDGE_SET_TESTCONDITION(edge,  &ARMTestAndSetConditionAllPaths);
	break;
      }
#endif
    case ET_CALL:
    case ET_IPJUMP:
      ASSERT(last_arm_ins, ("what? @E", edge));
      if (ARM_INS_OPCODE(last_arm_ins)==ARM_T2CBZ)
        {
          CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetForCBZ);
        }
      else if (ARM_INS_OPCODE(last_arm_ins)==ARM_T2CBNZ)
        {
          CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetForCBNZ);
        }
      else if (BblEndsWithConditionalBranchAfterCMP(ARM_INS_BBL(last_arm_ins)))
        {
          CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetConditionFunctionsArrayExt[condition]);
        }
      else
        {
          CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetConditionFunctionsArray[condition]);
        }
      break;

    case ET_FALLTHROUGH:
    case ET_IPFALLTHRU:
    case ET_UNKNOWN:
    case ET_IPUNKNOWN:

      /* if there is a fall-through-like path and PropOverEdge is
	 called for the corresponding edge with some state, then this
	 state needs to be propagated 

	 if this edge comes from a block ending with a conditional
	 branch, PropOverEdge() will have been called for the
	 branch-taken edge before it is called for the fall-through
	 edge, and if the propagated state could be evaluated to
	 "always take the branch", PropOverEdge() will not even be
	 called for the fall-through edges.  */
      if (last_arm_ins
          && BblEndsWithConditionalBranchAfterCMP(ARM_INS_BBL(last_arm_ins))) {
        CFG_EDGE_SET_TESTCONDITION(edge, ARMTestAndSetConditionFunctionsArrayExt[ARM_CONDITION_AL]);
      }
      else {
        CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetConditionFunctionsArray[ARM_CONDITION_AL]);
      }
      break;
    case ET_SWITCH:
    case ET_RETURN:
    case ET_COMPENSATING:
    case ET_IPSWITCH:
      CFG_EDGE_SET_TESTCONDITION(edge,  &ARMTestAndSetConditionAllPaths);
      break;
    case ET_SWI:

      /* these case (and especially link edges) are dealt with in
	 PropOverEdge() */

      /* TODO */

      CFG_EDGE_SET_TESTCONDITION(edge,  ARMTestAndSetConditionFunctionsArray[ARM_CONDITION_AL]);
      break;
    default:
      FATAL(("ARMSetConditionToProp invalid type %lx\n",edgetype));
      break;
  }

}
/* }}} */
/*!
 *
 * \todo Document
 * 
 * \param ins
 * \param prev_state
 * \param next_state
 *
 * \return t_bool
 */
/* ArmInssConstantOptimizer {{{ */
 static t_arm_ins * getInsByAddress(t_cfg * cfg, t_uint32 address)
 {
  static t_ins * tmp = NULL;
  t_bbl * bbl;
  t_ins * ins;

  if (tmp && G_T_UINT32(INS_OLD_ADDRESS(tmp))==address)
    return T_ARM_INS(tmp);

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_INS(bbl,ins)
      if (G_T_UINT32(INS_OLD_ADDRESS(ins))==address)
      {
        tmp = ins;
          return T_ARM_INS(ins);
      }

  return NULL;
 }
static void PrintOneIns(t_cfg * cfg, t_uint32 address, int id)
{

  t_bbl * bbl;
  t_ins * ins;

  if (address==0) return;

  if (address == UINT32_MAX)
    {
      CFG_FOREACH_BBL(cfg,bbl)
        {
          t_bool has_arm = FALSE;
          t_bool has_thumb = FALSE;
          BBL_FOREACH_INS(bbl,ins)
            {
              if (ARM_INS_TYPE(T_ARM_INS(ins))==IT_DATA) continue;
              if (ARM_INS_FLAGS(T_ARM_INS(ins)) & FL_THUMB)
                has_thumb = TRUE;
              else
                has_arm = TRUE;
            }
          if (has_arm && has_thumb)
          {
            if (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_FIRST(bbl))) != TH_BX_R15)
            {
              /* BX r15 does jump to ARM code, so we should not FATAL then.
               * The merge-BBLs pass merges the BBL's containing Thumb and ARM code for this jump.
               */
              FATAL(("LINE %d,ARM AND THUMB IN @ieB",id,bbl));
            }
          }
        }
      return;
    }

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_INS(bbl,ins)
    {
      if (G_T_UINT32(INS_OLD_ADDRESS(ins))==address)
        {
          DEBUG(("LINE %d: @I (rB:r%d, rC:r%d)",id,ins, ARM_INS_REGB(T_ARM_INS(ins)),ARM_INS_REGC(T_ARM_INS(ins))));
          fflush(stdout);
          return;
        }
    }

  DEBUG(("LINE %d: INS NOT FOUND",id));
  fflush(stdout);
}

static void FATALIF(t_cfg * cfg, t_uint32 addr, int id)
{
  t_arm_ins * a = getInsByAddress(cfg, addr);
  ASSERT(a, ("instruction at %x not found", addr));
  ASSERT((ARM_INS_FLAGS(a) & FL_THUMB), ("LINE %d: @I", id, a));
}

t_bool ArmInssConstantOptimizer(t_arm_ins * ins, t_procstate * prev_state, t_procstate * next_state, t_analysis_complexity complexity)
{
  t_bool b = FALSE, c = FALSE; t_uint32 addr = 0x15718;

  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!ArmInsOptKillSBit(ins,prev_state,next_state)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!diabloanopt_options.rely_on_calling_conventions)
    if (!ArmInsOptKillIdempotent(ins,prev_state,next_state)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!ArmInsOptMakeIndirectJumpsDirect(ins,prev_state,next_state, complexity)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!ArmInsOptSwitchOptimizer(ins,prev_state,next_state)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!ArmInsOptEncodeConstantResult(ins,prev_state,next_state)) return FALSE;

  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!diabloanopt_options.rely_on_calling_conventions)
    if (!ArmInsOptProduceConstantFromOtherRegister(ins,prev_state,next_state)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  if (!ArmInsOptEncodeConstantOperands(ins,prev_state,next_state)) return FALSE;


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  ArmOptMoveNullIntoPC(ins);


  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);
  MakeConstProducers2(ins, next_state);

  if (b) PrintOneIns(ARM_INS_CFG(ins), addr, __LINE__);
  if (c) FATALIF(ARM_INS_CFG(ins),addr,__LINE__);

  /* switched off because it makes programs bigger instead of smaller!
   if (!ArmInsOptEncodeConstantResultInTwoInstructions(ins,prev_state,next_state)) return FALSE; */

  /*  ArmInsOptChangeLoadBaseRegister(ins, prev_state, next_state);*/
  return TRUE;
}


void RenameLocalAddressProducers(t_cfg * cfg)
{
  t_arm_ins * i_ins, *j_ins, *k_ins;
  t_regset used_regs = RegsetNew();
  t_bbl * bbl;
  /* static int count = 0; */

  CfgComputeLiveness(cfg,CONTEXT_INSENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);
  CfgComputeSavedChangedRegisters(cfg);

  CFG_FOREACH_BBL(cfg,bbl)
  BBL_FOREACH_ARM_INS(bbl,i_ins)
    {
      if (ARM_INS_OPCODE(i_ins)==ARM_ADDRESS_PRODUCER && !ArmInsIsConditional(i_ins))
	{
	  t_reg def = ARM_INS_REGA(i_ins);
	  t_reg new_reg;
	  t_regset free_regs;
	  
	  /* find an instruction in the same basic block that 
	   * unconditionally overwrites the produced address.
	   * This makes the address producer a local address producer */
	  for (j_ins = ARM_INS_INEXT(i_ins); j_ins; j_ins = ARM_INS_INEXT(j_ins))
	    {
	      if (RegsetIn(ARM_INS_REGS_DEF(j_ins),def) &&
		  RegsetIn(ARM_INS_REGS_USE(j_ins),def) &&
		  ArmInsWriteBackHappens(j_ins))
		{
		  j_ins = NULL;
		  break;
		}
	      
	      if (RegsetIn(ARM_INS_REGS_DEF(j_ins),def) &&
		  !ArmInsIsConditional(j_ins))
		{
		  break;
		}
	      if (ARM_INS_TYPE(j_ins)==IT_STORE_MULTIPLE &&
		  RegsetIn(RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins)),def))
		{
		  /* don't do renaming because this would mess up the order in
		   * which the different registers are stored in memory */
		  j_ins = NULL;
		  break;
		}
	      if (ARM_INS_TYPE(j_ins) == IT_LOAD_MULTIPLE &&
		  RegsetIn(RegsetNewFromUint32(ARM_INS_IMMEDIATE(j_ins)),def) &&
		  ArmInsIsConditional(j_ins))
	      {
		/* this is a partial define that cannot be renamed: renaming it
		 * would change the order in which value are read from memory
		 * in the load multiple */
		j_ins = NULL;
		break;
	      }
	      if ((ARM_INS_OPCODE(j_ins)==ARM_LDRD ||
                   ARM_INS_OPCODE(j_ins)==ARM_STRD) &&
		  (def == ARM_INS_REGA(j_ins) ||
                   def == (ARM_INS_REGA(j_ins)+1)))
		{
                  /* these instructions resp. write/read REGA and REGA+1
                   * we cannot rename just one of them, and REGA must
                   * moreover always be even
                   */
		  j_ins = NULL;
		  break;
		}
	    }
	  
	  if (j_ins==NULL)
	    continue;
	  
	  /* find a free register for the renaming */
	  RegsetSetDup(free_regs,ArmInsRegsLiveAfter(i_ins));
	  RegsetSetInvers(free_regs);
	  
	  RegsetSetSubReg(free_regs,def);
#if 1
	  k_ins = ARM_INS_INEXT(i_ins);
	  while (k_ins)
	    {
	      RegsetSetDiff(free_regs,ARM_INS_REGS_DEF(k_ins));
	      k_ins=ARM_INS_INEXT(k_ins);
	    }
#else
	  for (k_ins=ARM_INS_INEXT(i_ins);k_ins!=j_ins;k_ins=ARM_INS_INEXT(k_ins))
	    RegsetSetDiff(free_regs,ARM_INS_REGS_DEF(k_ins));
#endif  
	  RegsetSetIntersect(free_regs,CFG_DESCRIPTION(BBL_CFG(bbl))->int_registers);
	  RegsetSetDiff(free_regs,BBL_REGS_LIVE_OUT(bbl));
	  RegsetSetDiff(free_regs,used_regs);
	  RegsetSetSubReg(free_regs,ARM_REG_R13);
	  RegsetSetSubReg(free_regs,ARM_REG_R14);
	  RegsetSetSubReg(free_regs,ARM_REG_R15);
	  
	  if (RegsetIsEmpty(free_regs))
	    continue;

	  REGSET_FOREACH_REG_INVERSE(free_regs,new_reg)
	    break;

	  RegsetSetAddReg(used_regs,new_reg);

	  /* do the renaming */
	  /* if (++count > diablosupport_options.debugcounter) continue;
	  VERBOSE(0, ("Doing local rename for @I -> r%d\n", i_ins, new_reg));
	  */
	  ARM_INS_SET_REGA(i_ins, new_reg);

	  ARM_INS_SET_REGS_DEF(i_ins,  ArmDefinedRegisters(i_ins));

	  k_ins = ARM_INS_INEXT(i_ins);

	  while (k_ins)
	    {
	      if (ARM_INS_REGB(k_ins)==def)
		ARM_INS_SET_REGB(k_ins, new_reg);
	      if (ARM_INS_REGC(k_ins)==def)
		ARM_INS_SET_REGC(k_ins, new_reg);
	      if (ARM_INS_REGS(k_ins)==def)
		ARM_INS_SET_REGS(k_ins, new_reg);
	      if (ARM_INS_TYPE(k_ins)==IT_STORE ||
		  (RegsetIn(ARM_INS_REGS_DEF(k_ins),def) &&
		   ARM_INS_CONDITION(k_ins) != ARM_CONDITION_AL))
		if (ARM_INS_REGA(k_ins)==def)
		  ARM_INS_SET_REGA(k_ins, new_reg);

	      ARM_INS_SET_REGS_DEF(k_ins,  ArmDefinedRegisters(k_ins));
	      ARM_INS_SET_REGS_USE(k_ins,  ArmUsedRegisters(k_ins));

	      /* VERBOSE(0, ("And changed @I", k_ins)); */

	      if (RegsetIn(ARM_INS_REGS_DEF(k_ins),def) && !ArmInsIsConditional(k_ins))
		break;
	      k_ins = ARM_INS_INEXT(k_ins);
	    }
	}
    }
}

t_bool ConstProdCanGenerateInOne(t_int32 val, t_bool thumb)
{
  t_uint32 a;
  t_uint32 i;

  if (thumb)
  {
    if (!(val & ~0xff))
      return TRUE; /* MOV (immediate) T1/T3 */

    if (!(val & ~0xffff))
      return TRUE; /* MOVW */

    if (ArmIsThumb2ImmediateEncodable(val))
      return TRUE; /* MOV (immediate) T2 */

    /*if (ArmIsThumb2ImmediateEncodable(~val))
      return TRUE;*/

    return FALSE;
  }

  /* first we try to generate the constant from scratch in one instruction */
  {
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val, i)) <= 0xff)
        return TRUE;
    
    t_int32 val_invert = ~val;
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_invert, i)) <= 0xff)
        return TRUE;

    t_int32 val_neg = -val;
    for (i = 0; i < 32; i += 2)
      if ((a = Uint32RotateLeft (val_neg-1, i)) <= 0xff)
        return TRUE;
  }
  return FALSE;
}

void 
ArmInsEmulatorWrapper(t_ins * ins, t_procstate * state, t_bool update_known_values)
{
  ArmInsEmulator(T_ARM_INS(ins), state, update_known_values);
}

void MakeConstProducers(t_cfg * cfg)
{
  t_bbl * bbl;
  t_procstate * temp_procstate = ProcStateNew(&arm_description);
  t_register_content content;
  t_reloc * reloc;
  t_arm_ins * ins;

  t_ConstantPropagationInsEmul propagator = ArmInsEmulatorWrapper;

  if (diabloflowgraph_options.blockprofilefile)
    CfgComputeHotBblThreshold(cfg,0.95);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      ProcStateSetAllBot(temp_procstate,CFG_DESCRIPTION(cfg)->all_registers);
      BBL_FOREACH_ARM_INS(bbl,ins) 
	{
	  propagator(T_INS(ins),temp_procstate,FALSE);
	  if ((
	       ARM_INS_OPCODE(ins)==ARM_MOV || 
	       ARM_INS_OPCODE(ins)==ARM_MVN || 
	       ARM_INS_OPCODE(ins)==ARM_SUB || 
	       ARM_INS_OPCODE(ins)==ARM_ADD || 
	       ARM_INS_OPCODE(ins)==ARM_LDR 
	       )
	      && ARM_INS_REGA(ins)!=ARM_REG_R15 && !ArmInsIsConditional(ins) && !ArmInsUpdatesCond(ins) && RegsetCountRegs(ARM_INS_REGS_DEF(ins)) == 1)
	    
	    if (CP_VALUE == ProcStateGetReg(temp_procstate,ARM_INS_REGA(ins),&content))
	      if (CP_TOP == ProcStateGetTag(temp_procstate,ARM_INS_REGA(ins),&reloc))
		{
		  if ((diabloflowgraph_options.blockprofilefile && !BblIsHot(bbl)) ||
                      ConstProdCanGenerateInOne(G_T_UINT32(content.i), ARM_INS_FLAGS(ins) & FL_THUMB))
		    ArmMakeConstantProducer(ins,G_T_UINT32(content.i));
		}
	}
    }

  ProcStateFree(temp_procstate);

}

void MakeConstProducers2(t_arm_ins * ins, t_procstate *procstate)
{
  t_register_content content;
  t_reloc * reloc;
  t_uint32 value;

  if (ARM_INS_OPCODE(ins)!=ARM_CONSTANT_PRODUCER && 
      ARM_INS_REGA(ins)!=ARM_REG_R15 && 
      ARM_INS_REGA(ins)!=ARM_REG_NONE && 
      ARM_INS_OPCODE(ins)!=ARM_LDM &&
      ARM_INS_OPCODE(ins)!=ARM_LFM &&
      !ArmInsIsConditional(ins) && 
      !ArmInsUpdatesCond(ins) && 
      !ArmInsHasSideEffect(ins) &&
      RegsetCountRegs(ARM_INS_REGS_DEF(ins)) == 1 &&
      CP_VALUE == ProcStateGetReg(procstate,ARM_INS_REGA(ins),&content) &&
      CP_TOP == ProcStateGetTag(procstate,ARM_INS_REGA(ins),&reloc) && 
      !RegsetIsEmpty(ARM_INS_REGS_USE(ins))
      )
    {
      value = G_T_UINT32(content.i);

      if (ConstProdCanGenerateInOne(value, ARM_INS_FLAGS(ins) & FL_THUMB))
      {
	ArmMakeConstantProducer(ins,value);
      }
    }
}
/* }}} */

void ArmGetFirstInsOfConditionalBranchWithSideEffect(t_bbl * bbl,t_ins ** ins)
{
  t_ins *ret=T_INS(ArmGetLastConditionalInsFromBlockWherePropagationSplits(bbl));
  if (ret && INS_TYPE(ret)!=IT_BRANCH) *ins=ret;
  else *ins=NULL;
}

void 
ArmInssConstantOptimizerWrapper(t_ins * ins, t_procstate * prev_state, t_procstate * next_state, t_analysis_complexity complexity)
{
  ArmInssConstantOptimizer (T_ARM_INS(ins), prev_state, next_state, complexity);
}

void 
ArmEdgePropagatorWrapper(t_cfg_edge * edge, t_ins * last_ins)
{
  ArmEdgePropagator(edge, T_ARM_INS(last_ins));
}

void 
ArmConstantPropagationInit(t_cfg * cfg)
{
	CFG_SET_INSTRUCTION_EMULATOR(cfg,ArmInsEmulatorWrapper);
	CFG_SET_INSTRUCTION_CONSTANT_OPTIMIZER(cfg,ArmInssConstantOptimizerWrapper);
	CFG_SET_EDGE_PROPAGATOR(cfg,ArmEdgePropagatorWrapper);
	CFG_SET_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg,ArmGetFirstInsOfConditionalBranchWithSideEffect);
}

/* vim: set shiftwidth=2 foldmethod=marker : */
