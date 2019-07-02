#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

//#define DEBUG_ACTIONS

void CfgConvertPseudoSwaps(t_cfg *cfg) {
  bool is_thumb = false;

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    vector<t_arm_ins *> pseudo_swaps;

    /* first make a list of pseudo swap instructions, as
     * we will modify the instruction list for the BBL */
    t_arm_ins *ins;
    BBL_FOREACH_ARM_INS(bbl, ins)
      if (ARM_INS_OPCODE(ins) == ARM_PSEUDO_SWAP)
        pseudo_swaps.push_back(ins);

    /* process each pseudo swap instruction */
    for (auto pseudo_swap_insn : pseudo_swaps) {
      t_reg rx = ARM_INS_REGA(pseudo_swap_insn);
      t_reg ry = ARM_INS_REGB(pseudo_swap_insn);

      /* convert the swap instruction to an EOR instruction */
      ArmInsMakeEor(pseudo_swap_insn, rx, rx, ry, 0, ARM_CONDITION_AL);
      AFFactoringLogInstruction(pseudo_swap_insn);

      /* add two more instructions */
      t_arm_ins *add_after;
      t_arm_ins *tmp;

      add_after = pseudo_swap_insn;
      ArmMakeInsForIns(Eor, After, tmp, add_after, is_thumb, ry, rx, ry, 0, ARM_CONDITION_AL);
      ARM_INS_SET_TRANSFORMATION_ID(tmp, ARM_INS_TRANSFORMATION_ID(add_after));
      AFFactoringLogInstruction(tmp);

      add_after = tmp;
      ArmMakeInsForIns(Eor, After, tmp, add_after, is_thumb, rx, rx, ry, 0, ARM_CONDITION_AL);
      ARM_INS_SET_TRANSFORMATION_ID(tmp, ARM_INS_TRANSFORMATION_ID(add_after));
      AFFactoringLogInstruction(tmp);
    }
  }
}

static t_arm_ins *ActionSwapRegisters(t_bbl *bbl, t_reg rx, t_reg ry, bool after, AfFlags& af_flags)
{
#ifdef DEBUG_ACTIONS
  DEBUG(("swapping r%d <-> r%d", rx, ry));
#endif

  /* This implementation uses the XOR-swap algorithm.
   * It comes down to this, mathematically (assuming values x and y need to be swapped):
   *    x = x ^ y
   *    y = x ^ y = (x ^ y) ^ y = x
   *    x = x ^ y = (x ^ y) ^ x = y
   * So, on ARM, two registers Rx and Ry can be swapped using the following instruction sequence:
   *    EOR Rx, Rx, Ry
   *    EOR Ry, Rx, Ry
   *    EOR Rx, Rx, Ry */
  t_arm_ins *added_instruction;
  t_arm_ins *tmp;

  bool is_thumb = false;

  t_ins *last = BBL_INS_LAST(bbl);
  if (last && RegsetIn(INS_REGS_DEF(last), ARM_REG_R15))
    ArmMakeInsForIns(Swap, Before, added_instruction, T_ARM_INS(last), is_thumb, rx, ry);
  else
    ArmMakeInsForBbl(Swap, Append, added_instruction, bbl, is_thumb, rx, ry);
  /* no need to log here, as this will happen when the pseudo swaps are translated to real instructions */

  DefineRegisterInBbl(bbl, rx);
  DefineRegisterInBbl(bbl, ry);
  UseRegisterInBbl(bbl, rx);
  UseRegisterInBbl(bbl, ry);

  ASSERT(rx != ARM_REG_R15 && ry != ARM_REG_R15, ("can't swap to r15! r%d/r%d", rx, ry));

  AFFactoringLogInstruction(added_instruction);

  ProducedValue pval;
  pval.ins = added_instruction;
  //RecordProducingInstruction(bbl, rx, pval, after);

  if (rx == ARM_REG_R13 || ry == ARM_REG_R13)
    af_flags |= AF_FLAG_DIRTY_SP;

  return added_instruction;
}

static t_arm_ins *ActionStoreImmediateInRegister(t_bbl *bbl, t_reg reg, t_uint64 imm, bool after, AfFlags& af_flags)
{
#ifdef DEBUG_ACTIONS
  DEBUG(("store immediate %08llx in r%d", imm, reg));
#endif

  t_arm_ins *constant_producer_ins;
  //DEBUG(("producing a constant in r%u: 0x%llx", reg, imm));

  /* TODO: fixme for Thumb */
  bool is_thumb = false;

  /* create a constant producer */
  t_ins *last = BBL_INS_LAST(bbl);
  if (last && RegsetIn(INS_REGS_DEF(last), ARM_REG_R15))
  {
    /* insert before the last instruction */

    /* This check is permanently disabled to comply with 447.dealII (Linux/ARM, dynamic, Os, GCC 4.8.1).
     * REASON: diabloanoptarm_advanced_factor_factoring.cc:TransformFactoringPossiblity(...)
     *          calls PropagateLivenessActions for INCOMING
     *    This is to propagate changes in register values. However, in the case of 447.dealII,
     *    that code triggers a false positive assertion failure here: two fragments from the same
     *    function are factored, and apparently by prepending an instructions for one fragment
     *    the to-be-generated register is marked live at entry of the second fragment.
     * For now, assume that liveness is correct. This is a dirty hack, but it works! */
#if 0
    if (RegsetIn(InsRegsLiveBefore(last), reg)) {
      CfgDrawFunctionGraphs(BBL_CFG(bbl), "live");
      FATAL(("register r%u not dead before @I in @iB", reg, last, bbl));
    }
#endif

    ArmMakeInsForIns(ConstantProducer, Before, constant_producer_ins, T_ARM_INS(last), is_thumb, reg, static_cast<t_uint32>(imm));
  }
  else
  {
    /* append to the BBL */
    ArmMakeInsForBbl(ConstantProducer, Append, constant_producer_ins, bbl, is_thumb, reg, static_cast<t_uint32>(imm));
  }
  AFFactoringLogInstruction(constant_producer_ins);

  DefineRegisterInBbl(bbl, reg);

  ProducedValue pval;
  pval.ins = constant_producer_ins;
  RecordProducingInstruction(bbl, reg, pval, after);

  ASSERT(reg != ARM_REG_R15, ("can't produce immediate in r15"));

  if (reg == ARM_REG_R13)
    af_flags |= AF_FLAG_DIRTY_SP;

  return constant_producer_ins;
}

static void ActionRegisterizeImmediate(t_bbl *bbl, size_t ins_id, t_reg reg, AfFlags& af_flags)
{
#ifdef DEBUG_ACTIONS
  DEBUG(("registerize immediate for instruction %d in r%d", ins_id, reg));
#endif

  /* get the instruction */
  t_arm_ins *ins;
  size_t counter = 0;
  BBL_FOREACH_ARM_INS_R(bbl, ins)
  {
    if (counter == ins_id)
      break;
    counter++;
  }
#ifdef DEBUG_ACTIONS
  DEBUG(("  instruction: @I", ins));
#endif

  ASSERT(ARM_INS_FLAGS(ins) & FL_IMMED, ("trying to registerize immediate of non-immediate instruction @I in @eiB (index %d)", ins, ARM_INS_BBL(ins), ins_id));

  /* convert the immediate value to a register */
  //DEBUG(("registerizing instruction @I: r%u", ins, reg));
  ArmInsReplaceImmediateWithRegister(ins, reg);
  //DEBUG(("    result: @I", ins));

  UseRegisterInBbl(bbl, reg);

  ASSERT(reg != ARM_REG_R15, ("can't move immediate to r15!"));

  if (reg == ARM_REG_R13)
    af_flags |= AF_FLAG_DIRTY_SP;
}

static t_arm_ins *ActionAliasRegisters(t_bbl *bbl, t_reg reg1, t_reg reg2, bool after, AfFlags& af_flags)
{
#ifdef DEBUG_ACTIONS
  DEBUG(("alias registers r%d - r%d", reg1, reg2));
#endif

  t_arm_ins *move_instruction;
  bool is_thumb = false;

  t_ins *last = BBL_INS_LAST(bbl);
  if (last && RegsetIn(INS_REGS_DEF(last), ARM_REG_R15))
    ArmMakeInsForIns(Mov, Before, move_instruction, T_ARM_INS(last), is_thumb, reg1, reg2, 0, ARM_CONDITION_AL);
  else
    ArmMakeInsForBbl(Mov, Append, move_instruction, bbl, is_thumb, reg1, reg2, 0, ARM_CONDITION_AL);
  AFFactoringLogInstruction(move_instruction);

  DefineRegisterInBbl(bbl, reg1);
  UseRegisterInBbl(bbl, reg2);

  ProducedValue pval;
  pval.ins = move_instruction;
  RecordProducingInstruction(bbl, reg1, pval, after);

  ASSERT(reg1 != ARM_REG_R15 && reg2 != ARM_REG_R15, ("can't alias the PC r%d/r%d", reg1, reg2));

  if (reg1 == ARM_REG_R13)
    af_flags |= AF_FLAG_DIRTY_SP;

  return move_instruction;
}

string PrintAction(AFAction action)
{
  stringstream ss;

  ss << (action.before ? "BEFORE" : "AFTER") << ": ";

  switch (action.code)
  {
  case AFActionCode::StoreImmediateInRegister:
    ss << "v" << action.args[0].reg << " = ";
    ss << action.args[1].imm;
    break;

  case AFActionCode::RegisterizeImmediate:
    ss << "registerize ";
    ss << "instruction #" << action.args[0].ins_id << " -> v" << action.args[1].reg;
    break;

  case AFActionCode::AliasRegisters:
    ss << "alias ";
    ss << "r" << action.args[0].reg << " - r" << action.args[1].reg;
    break;

  case AFActionCode::SwapRegisters:
    ss << "swap ";
    ss << "r" << action.args[0].reg << " <-> r" << action.args[1].reg;
    break;

  default:
    FATAL(("unsupported action %u", static_cast<int>(action.code)));
  }

  return ss.str();
}

AFActionResult ApplyActionsToBbl(t_bbl *bbl, AFActionList actions, Slice *slice, vector<t_reg> virtual_to_regs, bool before, AfFlags& af_flags)
{
  AFActionResult result = AFActionResult();
  t_arm_ins *added;

  for (auto action : actions)
  {
    if (action.slice != slice)
      continue;

    if (action.before != before)
      continue;

    switch (action.code)
    {
    case AFActionCode::StoreImmediateInRegister:
      added = ActionStoreImmediateInRegister(bbl, virtual_to_regs[action.args[0].reg], action.args[1].imm, !before, af_flags);
      result.added_ins_info.AddInstruction(T_INS(added));
      break;

    case AFActionCode::RegisterizeImmediate:
      ActionRegisterizeImmediate(bbl, action.args[0].ins_id, virtual_to_regs[action.args[1].reg], af_flags);
      result.nr_modified_insns++;
      break;

    case AFActionCode::AliasRegisters:
      added = ActionAliasRegisters(bbl, static_cast<t_reg>(action.args[0].reg), static_cast<t_reg>(action.args[1].reg), !before, af_flags);
      result.added_ins_info.AddInstruction(T_INS(added));
      break;

    case AFActionCode::SwapRegisters:
      added = ActionSwapRegisters(bbl, static_cast<t_reg>(action.args[0].reg), static_cast<t_reg>(action.args[1].reg), !before, af_flags);
      result.added_ins_info.AddInstruction(T_INS(added), 3);
      break;

    default:
      FATAL(("unsupported action %u", static_cast<int>(action.code)));
    }
  }

  return result;
}

bool AddInstructionToBbl(t_bbl *bbl, t_arm_ins *& ins, bool prepend)
{
  t_ins *last = BBL_INS_LAST(bbl);
  bool is_thumb = /* TODO */FALSE;
  bool appended_to_bbl = false;

  if (prepend) {
    ArmMakeInsForBbl(Noop, Prepend, ins, bbl, is_thumb);
  }
  else {
    appended_to_bbl = !(last && RegsetIn(INS_REGS_DEF(last), ARM_REG_R15));

    if (appended_to_bbl)
      ArmMakeInsForBbl(Noop, Append, ins, bbl, is_thumb);
    else
      ArmMakeInsForIns(Noop, Before, ins, T_ARM_INS(last), is_thumb);
  }

  return appended_to_bbl;
}

t_arm_ins *AddInstructionToBblI(t_bbl *bbl, t_arm_ins *ins, bool after) {
  t_arm_ins *new_ins;

  bool thumb = false;

  if (ins) {
    if (after)
      ArmMakeInsForIns(Noop, After, new_ins, ins, thumb);
    else
      ArmMakeInsForIns(Noop, Before, new_ins, ins, thumb);
  }
  else {
    if (after) {
      t_ins *last = BBL_INS_LAST(bbl);
      bool append = !(last && RegsetIn(INS_REGS_DEF(last), ARM_REG_R15));

      if (append)
        ArmMakeInsForBbl(Noop, Append, new_ins, bbl, thumb);
      else
        ArmMakeInsForIns(Noop, Before, new_ins, T_ARM_INS(last), thumb);
    }
    else
      ArmMakeInsForBbl(Noop, Prepend, new_ins, bbl, thumb);
  }

  return new_ins;
}

t_arm_ins *ProduceAddressOfBblInBbl(t_bbl *bbl, t_reg reg, t_bbl *destination)
{
  t_arm_ins *address_producer_ins = NULL;
  auto appended_to_bbl = AddInstructionToBbl(bbl, address_producer_ins);

  /* need to check liveness */
  if (!appended_to_bbl)
  {
    if (RegsetIn(InsRegsLiveBefore(BBL_INS_LAST(bbl)), reg))
    {
      __DumpDots(BBL_CFG(bbl), "fatal", 0);
      FATAL(("register r%u not dead before @I anymore! Slicing @eiB", reg, BBL_INS_LAST(bbl), bbl));
    }
  }

  /* address producer step 1: make a "move immediate to register" instruction */
  ArmInsMakeMov(address_producer_ins, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  /* address producer step 2: make the move a real address producer */
  t_reloc* reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(bbl))),
                                                    AddressNew32(0),                    /* addend */
                                                    T_RELOCATABLE(address_producer_ins),/* from */
                                                    AddressNew32(0),                    /* from-offset: next instruction */
                                                    T_RELOCATABLE(destination),         /* to */
                                                    AddressNew32(0),                    /* to-offset */
                                                    FALSE,                              /* hell */
                                                    NULL,                               /* edge*/
                                                    NULL,                               /* corresp */
                                                    NULL,                               /* sec */
                                                    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(address_producer_ins, 0 /* immediate */, reloc);
  AFFactoringLogInstruction(address_producer_ins);

  DefineRegisterInBbl(bbl, reg);

  return address_producer_ins;
}

t_arm_ins *ProduceRegisterMoveInBbl(t_bbl *bbl, t_reg dst, t_reg src, bool prepend, bool after)
{
  ASSERT(ARM_REG_R0 <= src && src < ARM_REG_R15, ("what? r%d", src));
  t_arm_ins *move_instruction;
  auto appended_to_bbl = AddInstructionToBbl(bbl, move_instruction, prepend);

  ArmInsMakeMov(move_instruction, dst, src, 0, ARM_CONDITION_AL);
  AFFactoringLogInstruction(move_instruction);

  DefineRegisterInBbl(bbl, dst);
  UseRegisterInBbl(bbl, src);

  ProducedValue pval;
  pval.ins = move_instruction;
  RecordProducingInstruction(bbl, dst, pval, after);

  return move_instruction;
}

t_arm_ins *ProduceConstantInBbl(t_bbl *bbl, t_reg reg, t_uint32 constant, bool prepend, bool after)
{
  static bool preloaded_symbols = false;
  static vector<t_symbol *> syms;

  t_arm_ins *constant_producer_ins = NULL;
  auto appended_to_bbl = AddInstructionToBbl(bbl, constant_producer_ins, prepend);

  if (!appended_to_bbl)
  {
    if (RegsetIn(InsRegsLiveBefore(BBL_INS_LAST(bbl)), reg))
    {
      __DumpDots(BBL_CFG(bbl), "fatal", 0);
      FATAL(("register r%u not dead before @I anymore! Slicing @eiB", reg, BBL_INS_LAST(bbl), bbl));
    }
  }

#if CONDITIONAL_DISPATCHER_LITERAL
  ArmInsMakeConstantProducer(constant_producer_ins, reg, constant);

#elif CONDITIONAL_DISPATCHER_SECTIONS_01
  if (constant <= 1)
  {
    t_object *obj = CFG_OBJECT(BBL_CFG(bbl));

    if (!preloaded_symbols)
    {
      /* preload the symbols by looking them up */
      t_symbol *s;

      s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), AFSYMBOL_0_NAME);
      ASSERT(s, ("could not find symbol 0"));
      syms.push_back(s);

      s = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), AFSYMBOL_1_NAME);
      ASSERT(s, ("could not find symbol 1"));
      syms.push_back(s);

      preloaded_symbols = true;
    }

    t_arm_ins *address_producer_ins = constant_producer_ins;
    ArmInsMakeMov(address_producer_ins, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);

    t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
                                                      AddressNew32(0),
                                                      T_RELOCATABLE(address_producer_ins),
                                                      AddressNew32(0),
                                                      T_RELOCATABLE(SYMBOL_BASE(syms[constant])),
                                                      AddressNew32(0),
                                                      FALSE,
                                                      NULL,
                                                      NULL,
                                                      NULL,
                                                      "R00A00+" "\\" WRITE_32);
    ArmInsMakeAddressProducer(address_producer_ins, 0, reloc);

    /* make the load instruction */
    t_arm_ins *load_ins = NULL;
    ArmMakeInsForIns(Noop, After, load_ins, address_producer_ins, FALSE);
    ArmInsMakeLdr(load_ins, reg, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL, TRUE, FALSE, FALSE);
  }
  else
    ArmInsMakeConstantProducer(constant_producer_ins, reg, constant);

#elif CONDITIONAL_DISPATCHER_HEAP
  //TODO insert call to malloc
#endif
  AFFactoringLogInstruction(constant_producer_ins);

  DefineRegisterInBbl(bbl, reg);

  ProducedValue pval;
  pval.ins = constant_producer_ins;
  RecordProducingInstruction(bbl, reg, pval, after);

  return constant_producer_ins;
}
