#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_ACTION_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_ACTION_H

#define AFACTION_NARGS 5

enum class AFActionCode {
  StoreImmediateInRegister,
  RegisterizeImmediate,
  AliasRegisters,
  SwapRegisters,
};

union AFActionArgument {
  int reg;
  t_uint64 imm;
  size_t ins_id;
};

struct AFAction {
  AFActionCode code;
  Slice *slice;
  AFActionArgument args[AFACTION_NARGS];
  bool before;
};

struct AFActionResult {
  int nr_modified_insns;
  AddedInstructionInfo added_ins_info;

  AFActionResult() {
    nr_modified_insns = 0;
    added_ins_info = AddedInstructionInfo();
  }
};

typedef std::vector<AFAction> AFActionList;

AFActionResult ApplyActionsToBbl(t_bbl *bbl, AFActionList actions, Slice *slice, std::vector<t_reg> virtual_to_real, bool before, AfFlags &af_flags);
std::string PrintAction(AFAction action);

t_arm_ins *ProduceAddressOfBblInBbl(t_bbl *bbl, t_reg reg, t_bbl *destination);
t_arm_ins *ProduceRegisterMoveInBbl(t_bbl *bbl, t_reg dst, t_reg src, bool prepend, bool after);
t_arm_ins *ProduceConstantInBbl(t_bbl *bbl, t_reg reg, t_uint32 constant, bool prepend, bool after);
void CfgConvertPseudoSwaps(t_cfg *cfg);

bool AddInstructionToBbl(t_bbl *bbl, t_arm_ins *& ins, bool prepend = false);
t_arm_ins *AddInstructionToBblI(t_bbl *bbl, t_arm_ins *ins, bool after);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_ACTION_H */
