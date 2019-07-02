#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_BRANCH_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_BRANCH_H

struct DummyEdgeData {
  enum class DispatchType {
    SwitchOffset,
    SwitchBranch,
    DistributedTable,
    Other
  };
  DispatchType dispatch_type;
  std::set<t_uint32> dtable_set;
};

enum class PossibleBranchTargetType {
  FactoredBbl,
  FactoredEntryBbl
};

void GlobalRedirect(t_cfg_edge *e, DummyEdgeData data);
t_bbl *GetGlobalRedirectRelocTo(t_cfg *cfg);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_BRANCH_H */
