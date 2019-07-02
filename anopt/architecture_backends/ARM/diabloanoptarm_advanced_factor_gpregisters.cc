#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

#define CLEAR_REG(x, y) ((x) &= ~(1<<(y)))
#define IS_REG(x, y) (((x) & (1<<(y))) != 0)
#define EMPTY_REGS 0
#define FOREACH_GPREG(y) for (t_reg y = ARM_REG_R0; y <= ARM_REG_R15; y++)

bool GPRegistersIn(t_gpregisters regs, t_reg reg) {
  ASSERT_VALID_REG(reg);
  return IS_REG(regs, reg);
}

void GPRegistersSetAnd(t_gpregisters& a, t_gpregisters b) {
  a &= b;
}

bool GPRegistersIsEmpty(t_gpregisters regs) {
  return regs == EMPTY_REGS;
}

void GPRegistersSetUnion(t_gpregisters& a, t_gpregisters b) {
  a |= b;
}

void GPRegistersSetDiff(t_gpregisters& a, t_gpregisters b) {
  a &= ~b;
}

string GPRegistersPrint(t_cfg *cfg, t_gpregisters regs) {
  stringstream ss;

  FOREACH_GPREG(r) {
    if (!IS_REG(regs, r))
      continue;

    ss << "r" << r << ", ";
  }

  return ss.str();
}

void GPRegistersSetSubReg(t_gpregisters& a, t_reg reg) {
  CLEAR_REG(a, reg);
}

t_regset RegsetDiffGPRegisters(t_regset a, t_gpregisters b) {
  t_regset result = RegsetDup(a);

  FOREACH_GPREG(r)
    if (IS_REG(b, r))
      RegsetSetSubReg(result, r);

  return result;
}

std::vector<t_reg> GPRegistersToVector(t_gpregisters a) {
  vector<t_reg> result;

  FOREACH_GPREG(r)
    if (IS_REG(a, r))
      result.push_back(r);

  return result;
}

void GPRegistersSetIntersectRegset(t_gpregisters& a, t_regset b) {
  t_gpregisters result = GPRegistersEmpty();

  t_reg r;
  REGSET_FOREACH_REG(b, r)
    if (IS_REG(a, r))
      GPRegistersAdd(result, r);

  a = result;
}
