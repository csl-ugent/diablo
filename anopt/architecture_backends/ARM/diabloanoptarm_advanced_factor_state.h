#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_STATE_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_STATE_H

enum class RegisterValue {
  Unknown,
  Nonzero,
  Zero
};

struct RegisterAssumption {
  RegisterValue value;
  t_uint64 constant;
};

struct FactoredPath
{
  std::vector<t_cfg_edge *> incoming_path;
  t_cfg_edge *outgoing_edge;
  int index;
  bool create_bbl_on_edge;
  Slice *slice;
  t_uint32 edge_type;
  RegisterAssumption assumption;
};

void RecordModifiedBbl(t_bbl *bbl);

struct ProducedValue {
  /* set to NULL if no instruction
   * (analysis will start at beginning of BBL) */
  t_arm_ins *ins;
};

typedef std::map<t_reg, RegisterAssumption> RegisterAssumptionMap;
typedef std::map<t_bbl *, RegisterAssumptionMap> RegisterAssumptionInBblMap;

void RecordProducingInstruction(t_bbl *bbl, t_reg reg, ProducedValue value, bool after);
void RecordRegisterAssumption(t_bbl *bbl, t_reg reg, RegisterAssumption assumption);
void FastProducerPropagation(t_cfg *cfg, bool liveness, BblSet only_forward_nonzero);
AddedInstructionInfo CheckAndFixAssumptions(t_cfg *cfg, bool fix);
void RecordPropagationSource(t_bbl *bbl);
void ClearAssumptions();
void RecordRegisterMoveConstant(t_arm_ins *ins, RegisterValue value);
void CheckRegisterMoves();
void ClearRegisterMoves();
void ClearBeforeAfters();
void SetLivePropIncludeStart(bool b);
bool recalculate_nonzero(t_bbl *bbl, bool also_do_backward);
void RecordChangedRegisterInFunction(t_function *fun, t_reg reg);

void SetIsDebugState(bool x);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_STATE_H */
