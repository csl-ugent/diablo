#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_NONZERO_ANALYSIS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_NONZERO_ANALYSIS_H

BBL_DYNAMIC_MEMBER_GLOBAL_BODY(nonzeroregs_after, NONZEROREGS_AFTER, NonZeroRegsAfter, t_regset *, {*valp = NULL;}, {if (*valp != NULL) delete *valp;}, {*valp = NULL;});

void AnalyseCfgForNonZeroRegisters(t_cfg *cfg);
t_regset BblNonZeroRegistersBefore(t_bbl *bbl, bool use_start_regs /*= false*/, t_regset start_regs /*= NullRegs*/, bool include_procstate = true);
void BblCopyNonZeroRegistersAfter(t_bbl *from, t_bbl *to);
void BblSetNonZeroRegistersAfter(t_bbl *bbl, t_regset regs);
t_regset SliceRegsNonZeroIn(Slice *slice, size_t slice_size);
bool BblClearRegNonZeroAfter(t_bbl *bbl, t_reg reg);
bool BblSetRegNonZeroAfter(t_bbl *bbl, t_reg reg);
bool BblClearRegNonZeroBeforeM(t_bbl *bbl, t_reg reg);
bool BblSetRegNonZeroBeforeM(t_bbl *bbl, t_reg reg);
t_regset BblCalculateNonZeroRegistersAfter(t_bbl *bbl, t_regset start_regs = NullRegs);
void BblSetNonZeroRegistersAfter(t_bbl *bbl, t_regset regs);
void BblSetNonZeroRegistersBeforeM(t_bbl *bbl, t_regset regs);
t_regset BblNonZeroRegistersBeforeM(t_bbl *bbl);

void NonZeroAnalysisInit(t_cfg *cfg);
void NonZeroAnalysisFini(t_cfg *cfg);

t_regset NonzeroIncoming(t_bbl *bbl, BblSet& already_visited);
t_regset NonzeroOutgoing(t_bbl *bbl, BblSet& already_visited);
t_regset EdgeAssociatedNonzeroRegs(t_cfg_edge *edge);

static inline
t_regset BblNonZeroRegistersAfter(t_bbl *bbl)
{
  return (!BBL_NONZEROREGS_AFTER(bbl)) ? NullRegs : *BBL_NONZEROREGS_AFTER(bbl);
}

extern bool nonzero_init;

void SetIsDebugNonzero(bool x);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_NONZERO_ANALYSIS_H */
