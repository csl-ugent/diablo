#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_STATISTICS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_STATISTICS_H

extern t_uint32 nr_candidate_slices;

struct SliceSetRegTuple {
  SliceSet slices;
  t_reg reg;
};

void AddSetToStatistics(SliceSet set, size_t n_ins, FactoringResult insn_stats);
SpecialFunctionTrackResults AdvancedFactoringOriginTracking(t_bbl *bbl);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_STATISTICS_H */
