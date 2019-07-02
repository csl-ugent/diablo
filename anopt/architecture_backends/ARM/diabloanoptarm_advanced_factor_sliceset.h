#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_SLICESET_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_SLICESET_H

typedef std::map<Slice *, SliceSet> SliceInterferenceMap;

void SliceSetRemoveInvalids(SliceSet& set, size_t slice_size);
void SliceSetPrint(SliceSet set, std::string prefix, size_t slice_size);
void PrintFullSliceSetInformation(SliceSet& set, size_t slice_size, bool include_cached = false);

bool SliceSetConsiderForFactoring(SliceSet set, size_t slice_size);
bool SliceSetOverlaps(SliceSet set, size_t slice_size);
bool SliceSetStatusFlagsDead(SliceSet& set, size_t slice_size, bool use_cached = false);

t_cfg *SliceSetCfg(SliceSet set);

std::vector<SliceSet> SliceSetCreateNoninterferingSubsets(SliceSet set, size_t slice_size);

t_regset SliceSetCalculateRegsDeadThrough(SliceSet& set, size_t slice_size);

FactoringSetSourceInformation SliceSetGetSourceInformation(SliceSet& slice_set, int& nr_executed);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_SLICESET_H */
