/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOANNOTATIONS_REGIONS_H
#define DIABLOANNOTATIONS_REGIONS_H

extern "C" {
#include <diabloflowgraph.h>
}

#include "diabloannotations.h"

#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <set>

struct Region {
  Annotation *annotation;/* annotation associated with this region */
  BblSet bbls;/* list of BBLs associated with this region */
  t_uint32 idx;/* index */
  AnnotationRequests requests;/* annotation content */

  /* Constructor */
  Region(t_cfg* cfg, Annotation* annotation, const BblSet& bbls, AbstractAnnotationInfo* request);

  /* Delegated constructor */
  Region(t_cfg* cfg, Annotation* annotation, const BblVector& bbls, AbstractAnnotationInfo* request)
    : Region(cfg, annotation, BblSet(bbls.begin(), bbls.end()), request) {}
};

struct regioncmp {
  bool operator() (const Region* lhs, const Region* rhs) const
  {return lhs->idx < rhs->idx;}
};
typedef std::set<Region *, regioncmp> Regions;

/* FUNCTIONS */

void RegionsInit(const Annotations& annotations, t_cfg *cfg);
void RegionsFini(t_cfg *cfg);

t_bool IsBblInRegion(t_bbl *bbl, Region *region);
t_bool BblsHaveSameRegions(t_bbl* bbl1, t_bbl* bbl2);
void CreateRegionsFromAnnotationsAndAnnotateBbls(t_cfg *cfg, const Annotations& annotations);

void ExpandRegionToCalleesOfDepth(Region* region, int call_depth);

void RegionAddBbl(Region *region, t_bbl *bbl);
FunctionSet RegionGetAllFunctions(const Region* region);

void IoModifierBblRegions(t_bbl *bbl, t_string_array* array);
void BblRemoveFromRegion(Region *region, t_bbl *bbl);
void BblRemoveFromAllRegions(t_bbl *bbl);
void BblCopyRegions(t_bbl *from, t_bbl *to);
void BblDupCallbackRegionBookkeeping(void *a, void *b);

typedef t_bool (*SelectBblFromRegionCallback)(t_bbl *);
void SelectBblsFromRegionWithProfile(BblVector& result, SelectBblFromRegionCallback callback,
                                     const Region *region, t_uint32 select_percent,
                                     t_bool exclude_not_executed = TRUE, t_bool select_most_executed = FALSE);
void SelectBblsFromRegionWithoutProfile(BblVector& result, SelectBblFromRegionCallback callback,
                                        const Region *region, t_uint32 select_percent,
                                        t_randomnumbergenerator *rng);
void SelectBblsFromRegion(BblVector& result, SelectBblFromRegionCallback callback,
                          const Region *region, t_uint32 select_percent,
                          t_randomnumbergenerator *rng,
                          t_bool exclude_not_executed = TRUE, t_bool select_most_executed = FALSE);

void LogRegionsStaticComplexity(t_const_string filename, t_cfg* cfg);
void LogRegionsDynamicComplexity(t_const_string filename, t_cfg* cfg);

bool RegionGetValueForIntOption(const Region *region, t_const_string option_name, int& value);

/* PREPROCESSOR DEFINITIONS */

/* Usage:
  Region *region;
  BBL_FOREACH_REGION(bbl, region)
    ...
*/
#define BBL_FOREACH_REGION(bbl, region)\
  if (BBL_REGIONS(bbl) != NULL)\
    for (auto region_it__ = BBL_REGIONS(bbl)->begin(); (region_it__ != BBL_REGIONS(bbl)->end()) ? (region = *region_it__, TRUE) : FALSE; ++region_it__)

/* Usage:
  Region *region;
  CFG_FOREACH_REGION(cfg, region)
    ...
*/
#define CFG_FOREACH_REGION(cfg, region)\
  if (CFG_REGIONS(cfg) != NULL)\
    for (auto region_it__ = CFG_REGIONS(cfg)->begin(); (region_it__ != CFG_REGIONS(cfg)->end()) ? (region = *region_it__, TRUE) : FALSE; ++region_it__)

/* Usage:
  t_bbl *bbl;
  REGION_FOREACH_BBL(region, bbl)
    ...
*/
#define REGION_FOREACH_BBL(region, bbl)\
  for (auto region_it__ = region->bbls.begin(); (region_it__ != region->bbls.end()) ? (bbl = *region_it__, TRUE) : FALSE; ++region_it__)

/* DYNAMIC MEMBERS */

extern "C" {
BBL_DYNAMIC_MEMBER_GLOBAL(regions, REGIONS, Regions, Regions *, NULL);
CFG_DYNAMIC_MEMBER_GLOBAL(regions, REGIONS, Regions, Regions *, NULL);
}

#endif /* DIABLOANNOTATIONS_REGIONS_H */
