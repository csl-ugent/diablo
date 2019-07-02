#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_ANALYSIS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_ANALYSIS_H

void UpdateUseDef(t_bbl *bbl, t_bbl *original = NULL);
void DefineRegisterInBbl(t_bbl *bbl, t_reg reg);
void UseRegisterInBbl(t_bbl *bbl, t_reg reg);
void RemoveUnreachableBbl(t_bbl *bbl);
void AnalyseCertainNonzeroRegisters(t_cfg *cfg);
void CfgEdgeMakeAF(t_cfg_edge *edge, t_procstate *ps, t_equations eqs);

bool CanAddInstructionToBbl(t_bbl *bbl);
bool BblHasRelocationFromIns(t_bbl *bbl);
bool PossiblyFactorBbl(t_bbl *bbl);
bool BblShouldBeFactored(t_bbl *bbl);
bool BblIsReturnSite(t_bbl *bbl);
void ForbidFunction(t_function *fun, bool include_callees);

t_regset AFBblRegsLiveBeforeTail(t_cfg_edge *edge);
t_regset RegsLiveOut(t_bbl *bbl);
t_regset SliceRegsLiveBefore(Slice *slice, size_t slice_size);
t_regset SliceRegsLiveAfter(Slice *slice, size_t slice_size);
t_regset SliceRegsDeadThrough(Slice *slice, size_t slice_size);

std::vector<t_reg> RegsetToVector(t_regset regs);
std::vector<CfgPath> GetIncomingPaths(t_bbl *bbl, bool& success);

t_bbl *SplitEdge(t_cfg_edge *e, bool add_to_tail = true);
t_bbl *AddBranchIndirectionOnEdge(t_cfg_edge *e, bool add_to_tail, AddedInstructionInfo& added_ins_info);

t_regset AFFunctionLiveBefore(t_bbl *landing_site);
t_regset AFFunctionDefines(t_function *function);

static inline
bool CfgEdgeIsAF(t_cfg_edge *edge)
{
  return (CFG_EDGE_FLAGS(edge) & EF_ADVANCED_FACTORING) != 0;
}

bool EdgeFromConditionalBranch(t_cfg_edge *edge, t_arm_condition_code& edge_condition, t_reg& compare_register, t_uint64& compare_value);
BblSet ExitBlockSuccessors(t_bbl *exit_block);
BblSet ExitBlockPredecessors(t_bbl *exit_block);

BblVector EdgeHead(t_cfg_edge *edge);
t_bbl *EdgeTail(t_cfg_edge *e);
void CfgEdgeMoveAF(t_cfg_edge *from, t_cfg_edge *to);

bool RegisterInfoForConditionalBranch(t_arm_ins *branch_ins, t_cfg_edge *taken, t_reg &reg, t_uint64 &value, t_arm_condition_code &condition);

std::vector<t_ins *> IterInsAfter(Slice *slice, size_t slice_size);
std::vector<t_ins *> IterInsIn(Slice *slice, size_t slice_size);

extern t_regset status_registers;

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_ANALYSIS_H */
