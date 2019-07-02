#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_REGIONS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_REGIONS_H

/* type definitions */
struct FactoringRegion {
  BblSet bbls;
  size_t nr_executed;
};

/* functions */
void ProcessFactoringRegions(t_cfg *cfg);

BblVector GetAllBblsInFactoringRegionID(t_cfg *cfg, int id);
BblVector GetAllBblsNotInFactoringRegionID(t_cfg *cfg, int id);
BblVector GetSlaveBblsForRegion(t_cfg *cfg, int region_id);

/* dynamic members */
BBL_DYNAMIC_MEMBER_GLOBAL(factoring_region_id, FACTORING_REGION_ID, FactoringRegionId, int, -1);
BBL_DYNAMIC_MEMBER_GLOBAL(factoring_targets, FACTORING_TARGETS, FactoringTargets, int, 0);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_REGIONS_H */
