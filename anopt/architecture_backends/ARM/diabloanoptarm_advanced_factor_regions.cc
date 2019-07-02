#include "diabloanoptarm_advanced_factor.hpp"

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(factoring_region_id);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(factoring_targets);

using namespace std;

vector<FactoringRegion> factoring_regions;

BblVector GetAllBblsInFactoringRegionID(t_cfg *cfg, int id)
{
  vector<t_bbl *> result;

  for (auto bbl : factoring_regions[id].bbls)
    if (BBL_CAN_TRANSFORM(bbl))
      result.push_back(bbl);

  return result;
}

BblVector GetAllBblsNotInFactoringRegionID(t_cfg *cfg, int id)
{
  vector<t_bbl *> result;
  set<t_bbl *> all_regioned_bbls;

  int idx = 0;
  for (auto factoring_region : factoring_regions)
  {
    all_regioned_bbls.insert(factoring_region.bbls.begin(), factoring_region.bbls.end());

    if (id != idx)
      for (auto bbl : factoring_region.bbls)
        result.push_back(bbl);

    idx++;
  }

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
    if (all_regioned_bbls.find(bbl) == all_regioned_bbls.end()
        && PossiblyFactorBbl(bbl) && BBL_CAN_TRANSFORM(bbl))
      result.push_back(bbl);

  return result;
}

BblVector GetSlaveBblsForRegion(t_cfg *cfg, int region_id)
{
  vector<t_bbl *> result;

#if 0
  int target_id = 0;
  if (RegionGetValueForIntOption(region, "target", target_id))
  {
    ASSERT(target_id != region_id, ("can't factor a region with itself (%u/%u)!", region_id, target_id));
    result = GetAllBblsInFactoringRegionID(cfg, target_id);
  }
  else
#endif
    result = GetAllBblsNotInFactoringRegionID(cfg, region_id);

  return result;
}

void ProcessFactoringRegions(t_cfg *cfg)
{
  Region *region;
  FactoringAnnotationInfo *info;

  bool master_found = false;
  int max_region_id = -1;
  CFG_FOREACH_FACTORING_REGION(cfg, region, info)
  {
    int region_id = 0;
    ASSERT(RegionGetValueForIntOption(region, "region_id", region_id), ("no region_id option specified for factoring annotation! In region %s", region->Print().c_str()));
    max_region_id = (max_region_id < region_id) ? region_id : max_region_id;

    if (region_id == diabloanoptarm_options.advanced_factoring_master_region_id)
    {
      VERBOSE(AF_VERBOSITY_LEVEL2, ("using factoring region %u as master!", region_id));
      master_found = true;
    }
  }
  VERBOSE(AF_VERBOSITY_LEVEL2, ("max factoring region ID = %u", max_region_id));
  ASSERT(master_found, ("could not find master region! %d", diabloanoptarm_options.advanced_factoring_master_region_id));

  /* initialise the region vector to the zero-based max region ID */
  ASSERT(max_region_id >= 0, ("not enough factoring regions found! %d", max_region_id));
  factoring_regions.resize(max_region_id + 1);

  CFG_FOREACH_FACTORING_REGION(cfg, region, info)
  {
    int region_id = 0;
    RegionGetValueForIntOption(region, "region_id", region_id);

    t_bbl *bbl;
    REGION_FOREACH_BBL(region, bbl)
    {
      if (!BBL_CAN_TRANSFORM(bbl))
        continue;

      BBL_SET_FACTORING_REGION_ID(bbl, region_id);

      int target_id = 0;
      if (RegionGetValueForIntOption(region, "target", target_id))
      {
        FATAL(("implement me"));
        ASSERT(target_id < 32, ("only 32 targets supported!"));
        BBL_SET_FACTORING_TARGETS(bbl, BBL_FACTORING_TARGETS(bbl) | (1 << target_id));
      }

      factoring_regions[region_id].bbls.insert(bbl);

      if (BBL_EXEC_COUNT(bbl) > 0)
        factoring_regions[region_id].nr_executed++;
    }
  }

  int idx = 0;
  for (auto factoring_region : factoring_regions)
  {
    VERBOSE(AF_VERBOSITY_LEVEL2, ("Factoring region %u contains %u BBLs, of which %u are executed (%f %%)",
                                  idx, factoring_region.bbls.size(), factoring_region.nr_executed, ((float)factoring_region.nr_executed/factoring_region.bbls.size())*100));
    idx++;
  }
}
