/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <vector>

#include <diabloarm_transformation.hpp>

using namespace std;

ARMArchitectureInfo::ARMArchitectureInfo(t_bool isThumb)
  : isThumb(isThumb)
{
  RegsetSetEmpty(possibleRegisters);
  for (t_reg reg = ARM_REG_R0; reg <= ARM_REG_R12; reg++) {
    RegsetSetAddReg(possibleRegisters, reg);
  }
}

t_reg ARMArchitectureInfo::getGenericRandomRegister(t_randomnumbergenerator* rng) {
  return RNGGenerateWithRange(rng, ARM_REG_R0, ARM_REG_R12);
}


void ARMArchitectureInfo::saveOrRestoreRegisters(const vector<MaybeSaveRegister>& regs, t_bbl* bbl, bool push, bool prepend) {
  t_arm_ins* ins;

  int mask = 0;

  /* TODO: merge the pushes/pops if needed */
  for (auto reg: regs) {
    if (!reg.save)
      continue;

    mask |= 1 << reg.reg;
  }

  if (!mask)
    return;

  if (prepend) { // TODO: hack for function flattening, remove the need for this!
    t_arm_ins* last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
    if (push)
      ArmMakeInsForIns(Push, Before, ins, last_ins, isThumb, mask, ARM_CONDITION_AL, isThumb /* thumb */);
    else
      ArmMakeInsForIns(Pop,  Before, ins, last_ins, isThumb, mask, ARM_CONDITION_AL, isThumb /* thumb */);
  } else {
    if (push)
      ArmMakeInsForBbl(Push, Append, ins, bbl, isThumb, mask, ARM_CONDITION_AL, isThumb /* thumb */);
    else
      ArmMakeInsForBbl(Pop, Prepend, ins, bbl, isThumb, mask, ARM_CONDITION_AL, isThumb /* thumb */); // TODO: uniformize, make it also Append
  }
}

/* TODO merge with above */
void ARMArchitectureInfo::popAppendRegisters(const vector<MaybeSaveRegister>& regs, t_bbl* bbl) {
  t_arm_ins* ins;

  int mask = 0;

  /* TODO: merge the pushes/pops if needed */
  for (auto reg: regs) {
    if (!reg.save)
      continue;

    mask |= 1 << reg.reg;
  }

  if (!mask)
    return;

  ArmMakeInsForBbl(Pop, Append, ins, bbl, isThumb, mask, ARM_CONDITION_AL, isThumb /* thumb */);
}

void ARMArchitectureInfo::appendUnconditionalBranchInstruction(t_bbl* bbl) {
  t_arm_ins* ins;
  ArmMakeInsForBbl(UncondBranch, Append, ins, bbl, isThumb);
}


ARMArchitectureInfoWrapper::ARMArchitectureInfoWrapper() {
  thumbWrapper = new ARMArchitectureInfo(TRUE);
  armWrapper = new ARMArchitectureInfo(FALSE);
}

ARMArchitectureInfoWrapper::~ARMArchitectureInfoWrapper() {
  delete thumbWrapper;
  delete armWrapper;
}

ArchitectureInfo* ARMArchitectureInfoWrapper::getArchitectureInfo(t_bbl* bbl_for) {
  if (!bbl_for) {
    // TODO: replace this with a generic wrapper, perhaps, that only works on functions that are the same for ARM and Thumb2. Atm this is used for register-based functions...
    VERBOSE(1, ("ARMArchitectureInfoWrapper::getArchitectureInfo called with NULL, using ARM"));
    return armWrapper;
  }
  if (ArmBblIsThumb(bbl_for))
    return thumbWrapper;
  return armWrapper;
}
