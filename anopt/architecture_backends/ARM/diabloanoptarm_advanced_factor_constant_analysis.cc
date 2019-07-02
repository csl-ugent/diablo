#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

t_procstate *SliceProcstateBefore(Slice *slice, size_t slice_size)
{
  t_bbl *bbl = slice->Bbl();
  ASSERT(BBL_PROCSTATE_IN(bbl), ("no procstate for slice! %s", slice->Print().c_str()));

  t_procstate *result = ProcStateNewDup(BBL_PROCSTATE_IN(bbl));
  ProcStateDup(result, BBL_PROCSTATE_IN(bbl), CFG_DESCRIPTION(BBL_CFG(bbl)));

  set<t_ins *> all_ins;
  for (auto ins : IterInsIn(slice, slice_size))
    all_ins.insert(ins);
  for (auto ins : IterInsAfter(slice, slice_size))
    all_ins.insert(ins);

  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins) {
    if (all_ins.find(ins) == all_ins.end()) {
      /* instruction is to be put before the slice */
      InsPropagateConstants(result, ins);
    }
  }

  return result;
}

void ProcstateConstantRegisters(t_procstate *procstate, t_regset& constant_registers, t_regset& null_registers, t_regset& tag_registers)
{
  constant_registers = NullRegs;
  null_registers = NullRegs;
  tag_registers = NullRegs;

  for (t_reg reg = ARM_REG_R0; reg < ARM_REG_R15; reg++) {
    t_register_content c;
    auto reg_level = ProcStateGetReg(procstate, reg, &c);
    if (reg_level != CP_BOT
        && reg_level != CP_TOP)
    {
      t_reloc *rel;
      auto tag_level = ProcStateGetTag(procstate, reg, &rel);
      if (tag_level != CP_BOT
          && tag_level != CP_TOP)
        /* this register contains a tag */
        RegsetSetAddReg(tag_registers, reg);
      else {
        /* this register contains a constant */
        if (AddressIsEq(c.i, AddressNew32(0)))
          RegsetSetAddReg(null_registers, reg);

        RegsetSetAddReg(constant_registers, reg);
      }
    }
  }
}

std::string PrintConstants(t_procstate *ps) {
  if (ps == NULL)
    return "";

  stringstream ss;
  for (t_reg reg = ARM_REG_R0; reg < ARM_REG_R15; reg++) {
    t_register_content c;
    auto reg_level = ProcStateGetReg(ps, reg, &c);
    if (reg_level != CP_BOT
        && reg_level != CP_TOP)
    {
      ss << "r" << reg << "=";

      t_reloc *rel;
      auto tag_level = ProcStateGetTag(ps, reg, &rel);
      if (tag_level != CP_BOT
          && tag_level != CP_TOP) {
        /* tag */
        ss << "t";
      }
      else {
        /* constant */
        ss << "c";
      }

      ss << hex << AddressExtractUint32(c.i) << ", ";
    }
  }

  return ss.str();
}

void ProcstateSetReg(t_procstate *ps, t_reg reg, t_uint64 constant, bool bottom) {
  /* reset existing information */
  t_regset modified_registers = RegsetNew();
  RegsetSetAddReg(modified_registers, reg);
  ProcStateSetAllBot(ps, modified_registers);

  if (!bottom) {
    /* set constant information */
    t_register_content reg_content;
    reg_content.i = AddressNew32(constant);
    ProcStateSetReg(ps, reg, reg_content);
  }
}

bool ProcstateGetConstantValue(t_procstate *ps, t_reg reg, t_uint64& constant) {
  t_register_content c;
  auto reg_level = ProcStateGetReg(ps, reg, &c);
  if (reg_level != CP_BOT
      && reg_level != CP_TOP)
  {
    t_reloc *rel;
    auto tag_level = ProcStateGetTag(ps, reg, &rel);
    if (tag_level != CP_BOT
        && tag_level != CP_TOP) {
      /* tag */
    }
    else {
      /* constant */
      constant = G_T_UINT32(c.i);
      return true;
    }
  }

  return false;
}

bool ProcstateGetTag(t_procstate *ps, t_reg reg, t_reloc *& tag) {
  t_register_content c;
  auto reg_level = ProcStateGetReg(ps, reg, &c);
  if (reg_level != CP_BOT
      && reg_level != CP_TOP)
  {
    t_reloc *rel;
    auto tag_level = ProcStateGetTag(ps, reg, &rel);
    if (tag_level != CP_BOT
        && tag_level != CP_TOP) {
      /* tag */
      tag = NULL;
      return true;
    }
    else {
      /* constant */
    }
  }

  return false;
}

void MergeConditionalProcstate(t_cfg_edge *e, t_procstate *new_procstate, t_reg& conditional_register, bool& conditional_register_nonzero) {
  t_bbl *from = CFG_EDGE_HEAD(e);
  t_bbl *tail = CFG_EDGE_TAIL(e);
  t_cfg *cfg = BBL_CFG(from);
  t_architecture_description *desc = CFG_DESCRIPTION(cfg);
  t_cfg_edge *e2;

  /* reset return variables */
  conditional_register = ARM_REG_NONE;
  conditional_register_nonzero = false;

  /* only support conditional branches */
  t_arm_ins *branch_instruction = T_ARM_INS(BBL_INS_LAST(CFG_EDGE_HEAD(e)));
  if (!(branch_instruction
      && ArmIsControlflow(branch_instruction)
      && ArmInsIsConditional(branch_instruction))) {
    return;
  }

#ifdef DEBUG_SPLIT_EDGE
  DEBUG(("doing edge @E", e));
#endif

  /* some precalculations for readability purposes */
  bool edge_is_jump = CFG_EDGE_CAT(e) == ET_JUMP || CFG_EDGE_CAT(e) == ET_IPJUMP;
  bool edge_is_fallthrough = CFG_EDGE_CAT(e) == ET_FALLTHROUGH || CFG_EDGE_CAT(e) == ET_IPFALLTHRU;
  bool condition_is_eq = ARM_INS_CONDITION(branch_instruction) == ARM_CONDITION_EQ;
  bool condition_is_ne = ARM_INS_CONDITION(branch_instruction) == ARM_CONDITION_NE;

  /* try to get the associated procstate with the given edge */
  t_procstate *merging_procstate = CFG_EDGE_CONDITIONAL_PROCSTATE(e);
  bool created_merging_procstate = false;

  if (!merging_procstate) {
    /* if no procstate was found, we try to look for it on the other edge */
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("no default procstate"));
#endif

    /* only support NE and EQ */
    switch (ARM_INS_CONDITION(branch_instruction)) {
    case ARM_CONDITION_LE:
    case ARM_CONDITION_LT:
    case ARM_CONDITION_MI:
    case ARM_CONDITION_GE:
    case ARM_CONDITION_GT:
    case ARM_CONDITION_CS:
    case ARM_CONDITION_HI:
    case ARM_CONDITION_LS:
    case ARM_CONDITION_CC:
    case ARM_CONDITION_PL:
      /* ok to not have conditional procstate */
      break;

    case ARM_CONDITION_NE:
    case ARM_CONDITION_EQ:
      /* look for the other edge (there are only 2, since we're looking at a conditional branch) */
      BBL_FOREACH_SUCC_EDGE(from, e2)
        if (e2 != e)
          merging_procstate = CFG_EDGE_CONDITIONAL_PROCSTATE(e2);
      break;

    default: FATAL(("unhandled conditition @I in @eiB", branch_instruction, from));
    }
  }
  else {
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("using default procstate @C", desc, merging_procstate));
#endif
  }

  if (!merging_procstate
      && (condition_is_eq || condition_is_ne)
      && BblEndsWithConditionalBranchAfterCMP(ARM_INS_BBL(branch_instruction))) {
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("also no default procstate for other edge, manually creating one"));
#endif

    /* construct the valued procstate */
    merging_procstate = ProcStateNew(desc);
    created_merging_procstate = true;
    ProcStateSetAllBot(merging_procstate, desc->all_registers);

    /* find constant register */
    t_arm_ins *cmp = FindCmpThatDeterminesJump(branch_instruction);

    /* set the value inside the procstate */
    t_register_content content;
    content.i = AddressNew32(ARM_INS_IMMEDIATE(cmp));
    ProcStateSetReg(merging_procstate, ARM_INS_REGB(cmp), content);
  }

  if (merging_procstate) {
    bool found_reg = false;

    for (t_reg reg = ARM_REG_R0; reg < ARM_REG_R15; reg++) {
      /* skip this register if the procstate already contains a tag for it */
      t_reloc *rel = NULL;
      if (ProcstateGetTag(new_procstate, reg, rel))
        continue;

      t_uint64 constant = 0;
      if (ProcstateGetConstantValue(merging_procstate, reg, constant)) {
        /* this register contains a constant value */
        ASSERT(!found_reg, ("multiple registers defined? @C", desc, merging_procstate));
        found_reg = true;
        conditional_register = reg;
#ifdef DEBUG_SPLIT_EDGE
        DEBUG(("found register r%d", reg));
#endif

        if ((condition_is_eq && edge_is_jump)
            || (condition_is_ne && edge_is_fallthrough)) {
          /* we're looking at the correct edge */
#ifdef DEBUG_SPLIT_EDGE
          DEBUG(("copying"));
#endif
          ProcstateSetReg(new_procstate, reg, constant, false);
        }
        else {
          /* need to invert */
#ifdef DEBUG_SPLIT_EDGE
          DEBUG(("inverting"));
#endif
          if (constant == 0) {
            conditional_register_nonzero = true;

            /* registered constant */
            t_uint64 registered_constant = 0;
            if (ProcstateGetConstantValue(new_procstate, reg, registered_constant)
                && registered_constant == 0) {
              /* procstate contains 0 */
              ProcStateSetRegBot(new_procstate, reg);
            }
          }
        }
      }
    }
  }
  else {
#ifdef DEBUG_SPLIT_EDGE
    DEBUG(("finally, no procstate was found! @E in @eiB", e, from));
#endif
  }

  if (created_merging_procstate)
    ProcStateFree(merging_procstate);
}
