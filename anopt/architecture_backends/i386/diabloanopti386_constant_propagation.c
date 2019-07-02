#include <diabloanopti386.h>

/* TODO ugly hack! */
extern t_argstate *current_argstate;

void I386TestAndSetConditionAllPaths(t_cfg_edge* edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_true  = ProcStateNew(&i386_description);
  *state_false = state;
  ProcStateDup(*state_true,state,&i386_description);
}

void I386TestAndSetConditionAlways(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_true  = state;
  *state_false = NULL;
}

void I386TestAndSetConditionNever(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false)
{
  *state_false  = state;
  *state_true = NULL;
}

void I386TestAndSetConditionSetAllToBot(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  *state_true  = ProcStateNew(&i386_description);
  *state_false = state;
  ProcStateSetAllBot(*state_true,i386_description.all_registers);
}

#define SetTrueAndFalse(_state,_true_state,_false_state,_maybe_true,_maybe_false)\
do {                                                                   \
  if (_maybe_true == YES)                                              \
  {                                                                    \
    *_true_state = _state;                                             \
    *_false_state = NULL;                                              \
  }                                                                    \
  else if (_maybe_false == YES)                                        \
  {                                                                    \
    *_true_state = NULL;                                               \
    *_false_state = _state;                                            \
  }                                                                    \
  else if (_maybe_true == PERHAPS && _maybe_false == PERHAPS)          \
  {                                                                    \
    *_true_state = _state;                                             \
    *_false_state = ProcStateNew(&i386_description);                                    \
    ProcStateDup(*_false_state,_state,&i386_description);                                \
  }                                                                    \
  else                                                                 \
    FATAL(("should not happen"));                                      \
} while (0)

static void I386TestAndSetConditionJO(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_O,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_NO,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
    ProcStateSetCond(*state_true,I386_CONDREG_OF,TRUE);

  if (*state_false)
    ProcStateSetCond(*state_false,I386_CONDREG_OF,FALSE);
}

static void I386TestAndSetConditionJNO(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJO(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJB(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_B,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_AE,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
    ProcStateSetCond(*state_true,I386_CONDREG_CF,TRUE);

  if (*state_false)
    ProcStateSetCond(*state_false,I386_CONDREG_CF,FALSE);
}

static void I386TestAndSetConditionJAE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJB(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJZ(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_Z,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_NZ,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
    ProcStateSetCond(*state_true,I386_CONDREG_ZF,TRUE);

  if (*state_false)
    ProcStateSetCond(*state_false,I386_CONDREG_ZF,FALSE);
}

static void I386TestAndSetConditionJNZ(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJZ(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJBE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_BE,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_A,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_false)
  {
    ProcStateSetCond(*state_false,I386_CONDREG_ZF,FALSE);
    ProcStateSetCond(*state_false,I386_CONDREG_CF,FALSE);
  }

  if (*state_true)
  {
    t_bool flag;
    if ((ProcStateGetCond(*state_true,I386_CONDREG_CF,&flag) == CP_VALUE) && !flag)
      ProcStateSetCond(*state_true,I386_CONDREG_ZF,TRUE);
    if ((ProcStateGetCond(*state_true,I386_CONDREG_ZF,&flag) == CP_VALUE) && !flag)
      ProcStateSetCond(*state_true,I386_CONDREG_CF,TRUE);
  }
}

static void I386TestAndSetConditionJA(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJBE(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJS(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_S,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_NS,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
    ProcStateSetCond(*state_true,I386_CONDREG_SF,TRUE);

  if (*state_false)
    ProcStateSetCond(*state_false,I386_CONDREG_SF,FALSE);
}

static void I386TestAndSetConditionJNS(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJS(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJP(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_P,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_NP,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
    ProcStateSetCond(*state_true,I386_CONDREG_PF,TRUE);

  if (*state_false)
    ProcStateSetCond(*state_false,I386_CONDREG_PF,FALSE);
}

static void I386TestAndSetConditionJNP(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJP(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJL(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_L,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_GE,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
  {
    t_bool flag;
    if ((ProcStateGetCond(*state_true,I386_CONDREG_SF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_true,I386_CONDREG_OF,!flag);
    if ((ProcStateGetCond(*state_true,I386_CONDREG_OF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_true,I386_CONDREG_SF,!flag);
  }

  if (*state_false)
  {
    t_bool flag;
    if ((ProcStateGetCond(*state_false,I386_CONDREG_SF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_false,I386_CONDREG_OF,flag);
    if ((ProcStateGetCond(*state_false,I386_CONDREG_OF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_false,I386_CONDREG_SF,flag);
  }
}

static void I386TestAndSetConditionJGE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJL(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJLE(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_LE,state);
  t_tristate maybe_false = I386ConditionHolds(I386_CONDITION_G,state);
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (*state_true)
  {
    /* zf == 0 => sf != of
     * sf == of => zf == 1 */
    t_bool kzf, ksf, kof;
    t_bool zf, sf, of;
    kzf = ProcStateGetCond(*state_true,I386_CONDREG_ZF,&zf) == CP_VALUE;
    ksf = ProcStateGetCond(*state_true,I386_CONDREG_SF,&sf) == CP_VALUE;
    kof = ProcStateGetCond(*state_true,I386_CONDREG_OF,&of) == CP_VALUE;

    if (kzf && !zf)
    {
      if (ksf) ProcStateSetCond(*state_true,I386_CONDREG_OF,!sf);
      if (kof) ProcStateSetCond(*state_true,I386_CONDREG_SF,!of);
    }

    if (ksf && kof && sf == of)
      ProcStateSetCond(*state_true,I386_CONDREG_ZF,TRUE);
  }

  if (*state_false)
  {
    /* zf == 0 && sf == of */
    t_bool flag;
    if ((ProcStateGetCond(*state_false,I386_CONDREG_SF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_false,I386_CONDREG_OF,flag);
    if ((ProcStateGetCond(*state_false,I386_CONDREG_OF,&flag) == CP_VALUE))
      ProcStateSetCond(*state_false,I386_CONDREG_SF,flag);

    ProcStateSetCond(*state_false,I386_CONDREG_ZF,FALSE);
  }
}

static void I386TestAndSetConditionJG(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJLE(edge,state,state_false,state_true, state_conditional);
}

static void I386TestAndSetConditionJECXZ(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_register_content ecx;

  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_ECXZ,state);
  t_tristate maybe_false = (maybe_true == PERHAPS ? PERHAPS : (maybe_true == YES ? NO : YES));
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);

  if (maybe_true)
  {
    ecx.i=AddressNew32(0);
    ProcStateSetReg(*state_true,I386_REG_ECX,ecx);
  }
}

static void I386TestAndSetConditionLOOP(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  I386TestAndSetConditionJECXZ(edge,state,state_true,state_false, state_conditional);
}

static void I386TestAndSetConditionLOOPZ(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_LOOPZ,state);
  t_tristate maybe_false = (maybe_true == PERHAPS ? PERHAPS : (maybe_true == YES ? NO : YES));
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);
}

static void I386TestAndSetConditionLOOPNZ(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_tristate maybe_true = I386ConditionHolds(I386_CONDITION_LOOPNZ,state);
  t_tristate maybe_false = (maybe_true == PERHAPS ? PERHAPS : (maybe_true == YES ? NO : YES));
  SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);
}

static void I386TestAndSetConditionSwitch(t_cfg_edge * edge,t_procstate* state, t_procstate **state_true, t_procstate** state_false, t_procstate **state_conditional)
{
  t_i386_ins *ins = T_I386_INS(BBL_INS_LAST(CFG_EDGE_HEAD(edge)));
  if (ins
      && I386_INS_OPCODE(ins) == I386_JMP
      && I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem
      && I386_OP_INDEX(I386_INS_SOURCE1(ins)) != I386_REG_NONE)
  {
    t_register_content rc;
    t_lattice_level level = ProcStateGetReg(state,I386_OP_INDEX(I386_INS_SOURCE1(ins)),&rc);
    t_tristate maybe_true,maybe_false;

    if (level == CP_VALUE && G_T_UINT32(rc.i) == CFG_EDGE_SWITCHVALUE(edge))
    {
      maybe_true = YES;
      maybe_false = NO;
    }
    else if (level == CP_VALUE && G_T_UINT32(rc.i) != CFG_EDGE_SWITCHVALUE(edge))
    {
      maybe_true = NO;
      maybe_false = YES;
    }
    else
      maybe_true = maybe_false = PERHAPS;

    SetTrueAndFalse(state,state_true,state_false,maybe_true,maybe_false);
    if (*state_true)
    {
      rc.i=AddressNew32(CFG_EDGE_SWITCHVALUE(edge));
      ProcStateSetReg(state,I386_OP_INDEX(I386_INS_SOURCE1(ins)),rc);
    }
  }
  else
  {
    /* don't recognize this kind of switch. be conservative */
    I386TestAndSetConditionAllPaths(edge,state,state_true,state_false, state_conditional);
  }
}

TestAndSetConditionFunction I386TestAndSetConditionArray[] = {
  /* I386_CONDITION_O  */ I386TestAndSetConditionJO, 
  /* I386_CONDITION_NO */ I386TestAndSetConditionJNO,
  /* I386_CONDITION_B  */ I386TestAndSetConditionJB, 
  /* I386_CONDITION_AE */ I386TestAndSetConditionJAE,
  /* I386_CONDITION_Z  */ I386TestAndSetConditionJZ, 
  /* I386_CONDITION_NZ */ I386TestAndSetConditionJNZ,
  /* I386_CONDITION_BE */ I386TestAndSetConditionJBE,
  /* I386_CONDITION_A  */ I386TestAndSetConditionJA, 
  /* I386_CONDITION_S  */ I386TestAndSetConditionJS, 
  /* I386_CONDITION_NS */ I386TestAndSetConditionJNS,
  /* I386_CONDITION_P  */ I386TestAndSetConditionJP, 
  /* I386_CONDITION_NP */ I386TestAndSetConditionJNP,
  /* I386_CONDITION_L  */ I386TestAndSetConditionJL, 
  /* I386_CONDITION_GE */ I386TestAndSetConditionJGE,
  /* I386_CONDITION_LE */ I386TestAndSetConditionJLE,
  /* I386_CONDITION_G  */ I386TestAndSetConditionJG
};

static TestAndSetConditionFunction JumpEdgePropagator(t_i386_ins * ins)
{
  switch (I386_INS_OPCODE(ins))
  {
    case I386_Jcc:
      return I386TestAndSetConditionArray[I386_INS_CONDITION(ins)];
    case I386_JECXZ:
      return I386TestAndSetConditionJECXZ;
    case I386_LOOP:
      return I386TestAndSetConditionLOOP;
    case I386_LOOPZ:
      return I386TestAndSetConditionLOOPZ;
    case I386_LOOPNZ:
      return I386TestAndSetConditionLOOPNZ;
    default:
      return &I386TestAndSetConditionAllPaths;
  }
  /* keep the compiler happy */
  return NULL;
}

void I386EdgePropagator(t_cfg_edge * edge, t_i386_ins * ins)
{
  t_uint16 edgetype  = CFG_EDGE_CAT(edge);

  if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
  {
    CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionAllPaths);
    return;
  }

  switch (edgetype)
  {
    case ET_IPJUMP:
    case ET_JUMP:
      if (!ins || (CFG_EDGE_FLAGS(edge) & EF_FROM_SWITCH_TABLE))
	CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionAllPaths);
      else
	CFG_EDGE_SET_TESTCONDITION(edge,  JumpEdgePropagator(ins));
      break;

    case ET_CALL:
      CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionAllPaths);
      break;

    case ET_COMPENSATING:
    case ET_RETURN:
      CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionAllPaths);
      break;

    case ET_SWITCH:
    case ET_IPSWITCH:
      CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionSwitch);
      break;

    case ET_IPFALLTHRU:
    case ET_FALLTHROUGH:
    case ET_UNKNOWN:
    case ET_IPUNKNOWN:
      CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionAlways);
      break;

    case ET_SWI:
      CFG_EDGE_SET_TESTCONDITION(edge,  &I386TestAndSetConditionSetAllToBot);
      break;

    default:
      FATAL(("invalid edge type @E",edge));
  }
}
/*
{
  if (BBL_IS_HELL(EDGE_HEAD(edge)))
  {
    CFG_EDGE_SET_TESTCONDITION(edge,  &i386TestAndSetConditionAllPaths);
    return;
  }
  CFG_EDGE_SET_TESTCONDITION(edge,  &i386TestAndSetConditionAllPaths);
  
}*/



t_bool I386InstructionsConstantOptimizer(t_i386_ins * ins, t_procstate * prev_state, t_procstate * next_state, t_analysis_complexity complexity)
{
#if 0
  /* uitgeschakeld wegens een bug */
  if (!I386MakeIndirectJumpsDirect(ins,prev_state,next_state, t_analysis_complexity complexity)) return FALSE;

  if (!I386MakeIndirectCallsDirect(ins,prev_state,next_state)) 
    return FALSE; 
#endif

  return TRUE;
}

void 
I386GetFirstInsOfConditionalBranchWithSideEffect(t_bbl * bbl,t_i386_ins ** ins)
{
  t_i386_ins *ret=T_I386_INS(BBL_INS_LAST(bbl));
  if (ret &&  (CFG_DESCRIPTION(BBL_CFG(bbl))->InsIsControlflow(T_INS(ret))) && (!RegsetIsEmpty(I386_INS_REGS_DEF(ret))) && (I386_INS_ATTRIB(ret) & IF_CONDITIONAL) && (I386_INS_TYPE(ret)!=IT_BRANCH)) *ins=ret;
  else *ins=NULL;
}

static void 
I386InstructionEmulatorWrapper(t_ins * ins, t_procstate * state, t_bool update_known_values)
{
  I386InstructionEmulator(T_I386_INS(ins), state, update_known_values);
}

static void 
I386InstructionsConstantOptimizerWrapper(t_ins * ins, t_procstate * prev_state, t_procstate * next_state, t_analysis_complexity complexity)
{
  I386InstructionsConstantOptimizer(T_I386_INS(ins), prev_state, next_state, complexity);
}

static void 
I386EdgePropagatorWrapper(t_cfg_edge * edge, t_ins * ins)
{
  I386EdgePropagator(edge, T_I386_INS(ins));
}

static void 
I386GetFirstInsOfConditionalBranchWithSideEffectWrapper(t_bbl * bbl, t_ins ** ins)
{
  I386GetFirstInsOfConditionalBranchWithSideEffect(bbl, (t_i386_ins **) ins);
}

void 
I386ConstantPropagationInit(t_cfg * cfg)
{
	CFG_SET_INSTRUCTION_EMULATOR(cfg, I386InstructionEmulatorWrapper);
	CFG_SET_INSTRUCTION_CONSTANT_OPTIMIZER(cfg, I386InstructionsConstantOptimizerWrapper);
	CFG_SET_EDGE_PROPAGATOR(cfg, I386EdgePropagatorWrapper);
	CFG_SET_GET_FIRST_INS_OF_CONDITIONAL_BRANCH_WITH_SIDE_EFFECT(cfg, I386GetFirstInsOfConditionalBranchWithSideEffectWrapper);
	I386InitArgumentForwarding(cfg);
}

void
I386ConstantPropagationFini(t_cfg *cfg)
{
}
/* vim: set shiftwidth=2 foldmethod=marker: */
