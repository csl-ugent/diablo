/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <vector>

#include <diabloi386_transformation.hpp>

using namespace std;

I386ArchitectureInfo::I386ArchitectureInfo() {
  RegsetSetEmpty(possibleRegisters);
  for (t_reg reg = I386_REG_EAX; reg <= I386_REG_EBP; reg++) {
    RegsetSetAddReg(possibleRegisters, reg);
  }
}

t_reg I386ArchitectureInfo::getGenericRandomRegister(t_randomnumbergenerator* rng) {
  return RNGGenerateWithRange(rng, I386_REG_EAX, I386_REG_EBP);
}

AddedInstructionInfo I386ArchitectureInfo::saveOrRestoreRegisters(const vector<MaybeSaveRegister>& regs, t_bbl* bbl, bool push, bool prepend) {
  FATAL(("To implement: I386ArchitectureInfo::saveOrRestoreRegisters"));
  return AddedInstructionInfo();
}

void I386ArchitectureInfo::appendUnconditionalBranchInstruction(t_bbl* bbl) {
  t_i386_ins* ins;
  I386MakeInsForBbl(Jump, Append, ins, bbl);
}

bool I386ArchitectureInfo::functionReturnsPC(const t_function* fun) {
  if (ArchitectureInfo::functionReturnsPC(fun))
    return true;

  return I386IsPcThunk(FUNCTION_BBL_FIRST(fun));
}
