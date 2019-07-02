#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_GPREGISTERS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_GPREGISTERS_H

typedef t_uint16 t_gpregisters;
typedef std::map<Slice *, t_gpregisters> SliceToGPRegistersMap;

#define SET_REG(x, y) ((x) |= 1<<(y))
#define ASSERT_VALID_REG(x) ASSERT(ARM_REG_R0 <= reg && reg <= ARM_REG_R15, ("r%d not a GP register!", reg))

static inline
void GPRegistersAdd(t_gpregisters& regs, t_reg reg) {
  ASSERT_VALID_REG(reg);
  SET_REG(regs, reg);
}

static inline
t_gpregisters GPRegistersEmpty() {
  return 0;
}

static inline
void GPRegistersSetDiffRegset(t_gpregisters& a, t_regset b) {
  a &= ~(register_get_subset(b, 0) & 0xffff);
}

bool GPRegistersIn(t_gpregisters a, t_reg b);
void GPRegistersSetAnd(t_gpregisters& a, t_gpregisters b);
bool GPRegistersIsEmpty(t_gpregisters regs);
void GPRegistersSetUnion(t_gpregisters& a, t_gpregisters b);
void GPRegistersSetDiff(t_gpregisters& a, t_gpregisters b);
std::string GPRegistersPrint(t_cfg *cfg, t_gpregisters regs);

static inline
int GPRegistersCount(t_gpregisters a) {
  return CountSetBits32(a);
}

static inline
t_gpregisters GPRegistersIntersectRegset(t_gpregisters a, t_regset b) {
  return a & (register_get_subset(b, 0) & 0xffff);
}

void GPRegistersSetSubReg(t_gpregisters& a, t_reg b);
t_regset RegsetDiffGPRegisters(t_regset a, t_gpregisters b);
std::vector<t_reg> GPRegistersToVector(t_gpregisters a);
void GPRegistersSetIntersectRegset(t_gpregisters& a, t_regset b);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_GPREGISTERS_H */
