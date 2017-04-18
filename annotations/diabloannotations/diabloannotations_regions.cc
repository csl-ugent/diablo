/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloannotations.h"

extern "C" {
#include <diabloflowgraph.h>
}

#include <algorithm>
#include <map>

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(regions);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(regions);

using namespace std;

typedef std::unordered_set<std::string> FileNameList;

static BblVector GetAllBblsInFile(t_cfg *cfg, const std::string *file_name);

static t_bool init = FALSE;

static
std::string GetFullFilePath(t_ins *ins)
{
  return std::string(INS_SRC_FILE(ins));
}

static
void GetAllFileNames(t_cfg *cfg, FileNameList& list)
{
  t_bbl *bbl_it;

  CFG_FOREACH_BBL(cfg, bbl_it)
  {
    t_ins *ins_it;

    BBL_FOREACH_INS(bbl_it, ins_it)
    {
      /* no file name associated */
      if (!INS_SRC_FILE(ins_it)) continue;

      /* Insert file name */
      list.insert(GetFullFilePath(ins_it));
    }
  }
}

static
void PrintAllFileNames(t_cfg *cfg)
{
  FileNameList list;

  GetAllFileNames(cfg, list);

  for (auto& f : list)
    VERBOSE(0, ("File name: %s", f.c_str()));
}

static
void SplitBblsByAnnotations(t_cfg* cfg, const Annotations& annotations)
{
  /* For every linerange annotation, we find the BBLs that contain instructions in the specified region, and adapt these BBLs
   * so they start and end with instructions from that region (splitting of new BBLs from the start or the end if necessary).
   */
  for (auto annotation : annotations)
  {
    if (annotation->file_name && (annotation->line_begin != UINT32_MAX) && (annotation->line_end != UINT32_MAX))
    {
      BblVector file_bbls = GetAllBblsInFile(cfg, annotation->file_name);
      t_uint32 line_begin = annotation->line_begin;
      t_uint32 line_end = annotation->line_end;

      for (auto bbl : file_bbls)
      {
        /* As one BBL can contain instructions spanning multiple source lines,
         * look at every instruction in the BBL to see if its line number matches. */
        t_ins *ins;
        t_bool found = FALSE;
        BBL_FOREACH_INS_R(bbl, ins)
        {
          /* Is the associated line number within the given range? */
          if (INS_SRC_LINE(ins) >= line_begin && INS_SRC_LINE(ins) <= line_end)
          {
            /* If the last instruction is in the specified region we don't need to split */
            if (ins != BBL_INS_LAST(bbl))
              BblSplitBlock(bbl, ins, FALSE);

            found = TRUE;
            break;
          }
        }

        /* If the BBL turns out to contain no relevant instructions, no forward search is necessary. We also have to handle
         * the exceptional case where the first instructions of a function have as source lines the function definition, placing
         * them before the annotation. These instructions would thus not be part of the region and be split off from the
         * BBL in the upcoming loop.
         */
        if (!found || (annotation->annotation_content->find("from_function_begin") != string::npos))
          continue;

        BBL_FOREACH_INS(bbl, ins)
        {
          /* Is the associated line number within the given range? */
          if (INS_SRC_LINE(ins) >= line_begin && INS_SRC_LINE(ins) <= line_end)
          {
            /* If the first instruction is in the specified region we don't need to split */
            if (ins != BBL_INS_FIRST(bbl))
              BblSplitBlock(bbl, ins, TRUE);

            break;
          }
        }
      }
    }
  }
}

t_bool BblsHaveSameRegions(t_bbl* bbl1, t_bbl* bbl2)
{
  if (!init)
    return TRUE;

  Regions* regions1 = BBL_REGIONS(bbl1);
  Regions* regions2 = BBL_REGIONS(bbl2);

  /* If either BBL is in regions while the other isn't, we won't allow. If both aren't, there is
   * no problem. If both are in regions, we do a check to see whether these are the same regions.
   */
  if ((regions1 == NULL) ^ (regions2 == NULL))
    return FALSE;

  if ((regions1 == NULL) && (regions2 == NULL))
    return TRUE;

  /* If they do both have regions, we'll do some more work to find out if they are the same */
  if (regions1->size() != regions2->size())
    return FALSE;

  return std::is_permutation(regions1->begin(), regions1->end(), regions2->begin());
}

static void BblMergeAllowedForRegions(t_bbl* head, t_bbl* tail, t_bool* allowed)
{
  if (!init)
    return;

  *allowed = BblsHaveSameRegions(head, tail);
}

void RegionsInit(const Annotations& annotations, t_cfg *cfg)
{
  CfgInitRegions(CFG_OBJECT(cfg));
  BblInitRegions(cfg);

  init = TRUE;

  /* Install the bookkeeping callback */
  BblCallbackInstall (cfg, CB_DUP, 1000, BblDupCallbackRegionBookkeeping, NULL);

  /* Install the related broker calls */
  DiabloBrokerCallInstall("BblKill", "t_bbl *", (void *)BblRemoveFromAllRegions, FALSE);
  DiabloBrokerCallInstall("BblSplitAfter", "t_bbl *, t_bbl *", (void *)BblCopyRegions, FALSE);
  DiabloBrokerCallInstall("CopyToNewTargetBBL", "t_bbl *, t_bbl *", (void *)BblCopyRegions, FALSE);
  DiabloBrokerCallInstall("IoModifierBbl", "t_bbl *, t_string_array *", (void *)IoModifierBblRegions, FALSE);
  DiabloBrokerCallInstall("BblMergeAllowed", "t_bbl *, t_bbl *, t_bool *", (void *)BblMergeAllowedForRegions, FALSE);

  /* Do a pre-pass that will split all BBLs so they fit completely into a specified annotation region.
   * This actually only means only the first and last instruction are part of the region, some instructions
   * in the middle might not belong to it.
   */
  SplitBblsByAnnotations(cfg, annotations);

  CreateRegionsFromAnnotationsAndAnnotateBbls(cfg, annotations);
}

static
void RegionDestroy(Region *r)
{
  for (auto request : r->requests)
    delete request;

  delete r;
}

void RegionsFini(t_cfg *cfg)
{
  init = FALSE;

  /* Uninstall the bookkeeping callback */
  BblCallbackUninstall (cfg, CB_DUP, 1000, BblDupCallbackRegionBookkeeping, NULL);

  Region *r;
  CFG_FOREACH_REGION(cfg, r)
    RegionDestroy(r);

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
    if (BBL_REGIONS(bbl))
      delete BBL_REGIONS(bbl);

  delete CFG_REGIONS(cfg);

  CfgFiniRegions(CFG_OBJECT(cfg));
  BblFiniRegions(cfg);
}

static
BblVector GetAllBblsInFunction(t_cfg *cfg, const std::string *function_name)
{
  BblVector result;

  t_function *func_it;
  CFG_FOREACH_FUN(cfg, func_it)
  {
    if (FUNCTION_NAME(func_it)
        && StringPatternMatch(function_name->c_str(), FUNCTION_NAME(func_it)))
    {
      /* function name match */
      t_bbl *bbl_it;
      FUNCTION_FOREACH_BBL(func_it, bbl_it)
      {
        /* skip meaningless BBLs */
        if (BBL_IS_HELL(bbl_it)) continue;
        if (BblIsExitBlock(bbl_it)) continue;

        result.push_back(bbl_it);
      }
    }
  }

  return result;
}

static t_bbl* FallthroughToFirstNonEmpty(t_bbl * bbl)
{
  while (BBL_NINS(bbl) == 0)
  {
    t_cfg_edge* edge;
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
      if (CfgEdgeTestCategoryOr(edge, ET_FALLTHROUGH))
        break;

    if (!edge)
      return NULL;

    bbl = CFG_EDGE_TAIL(edge);
  }
  return bbl;
}

/* Get all BBLs containing instructions associated with the given source file */
static
BblVector GetAllBblsInFile(t_cfg *cfg, const std::string *file_name)
{
  BblVector result;

  t_bbl *bbl_it;
  CFG_FOREACH_BBL(cfg, bbl_it)
  {
    /* skip meaningless BBLs */
    if (BBL_IS_HELL(bbl_it)) continue;
    if (BblIsExitBlock(bbl_it)) continue;

    /* If a BBL is empty, check if it falls through into a BBL from the file */
    t_bbl *bbl_nonempty = FallthroughToFirstNonEmpty(bbl_it);
    if(!bbl_nonempty)
      continue;

    /* as we don't support BBLs containing instructions from multiple files yet, only look at the first instruction */
    t_string bbl_file_name = INS_SRC_FILE(BBL_INS_FIRST(bbl_nonempty));
    if (!bbl_file_name)
      continue;

    std::string full_path = GetFullFilePath(BBL_INS_FIRST(bbl_nonempty));
    if (StringPatternMatch(file_name->c_str(), full_path.c_str()))
    {
      /* add this BBL to our list */
      result.push_back(bbl_it);
    }
  }

  return result;
}

static
t_object *GetSubobjectAndRangeContainingAddress(t_object *obj, t_address addr, t_address& begin, t_address& end)
{
  t_object *subobj, *tmpobj;
  t_section *section;

  OBJECT_FOREACH_SUBOBJECT(obj,subobj,tmpobj)
    if((section = ObjectGetSectionContainingAddress (subobj, addr)) != NULL)
    {
      if (SECTION_TYPE(section) != CODE_SECTION)
        continue;

      begin = SECTION_OLD_ADDRESS(section);
      end = AddressAdd(begin, SECTION_OLD_SIZE(section));

      ASSERT(!AddressIsEq(begin, AddressNullForObject(obj)), ("begin should not be zero! @G (%s) @T", addr, OBJECT_NAME(subobj), section));
      ASSERT(!AddressIsEq(end, AddressNullForObject(obj)), ("end should not be zero! @G (%s) @T", addr, OBJECT_NAME(subobj), section));
      return subobj;
    }

  return NULL;
}

/* Get all BBLs associated with a specific object file (or name pattern) */
static
BblVector GetAllBblsInObjectFile(t_cfg *cfg, const std::string *file_name)
{
  BblVector result;

  t_address begin = AddressNullForObject(CFG_OBJECT(cfg)), end = begin;
  t_object *current_object = NULL;
  bool current_object_match = false;

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (AddressIsLt(BBL_OLD_ADDRESS(bbl), begin)
        || AddressIsGe(BBL_OLD_ADDRESS(bbl), end))
    {
      /* out of current object range */
      current_object = GetSubobjectAndRangeContainingAddress(CFG_OBJECT(cfg), BBL_OLD_ADDRESS(bbl), begin, end);

      if (current_object)
        current_object_match = StringPatternMatch(file_name->c_str(), OBJECT_NAME(current_object));
      else
        current_object_match = false;
    }

    if (current_object_match) {
      result.push_back(bbl);
    }
  }

  return result;
}

/* Get all BBLs containing instructions associated with the given source line range (limits inclusive) */
static
BblVector GetAllBblsInFileLineRange(t_cfg *cfg, const std::string *file_name, t_uint32 line_start, t_uint32 line_end)
{
  BblVector result;

  BblVector file_bbls = GetAllBblsInFile(cfg, file_name);

  for (auto bbl : file_bbls)
  {
    t_ins *ins;

    /* as one BBL can contain instructions spanning multiple source lines,
     * look at every instruction in the BBL to see if its line number matches. */
    BBL_FOREACH_INS(bbl, ins)
    {
      /* is the associated line number within the given range? */
      if (INS_SRC_LINE(ins) >= line_start
          && INS_SRC_LINE(ins) <= line_end)
      {
        /* if the line number of this instruction matches the 'chosen one',
         * add the BBL to our list */
        result.push_back(bbl);

        /* we should only add this BBL once */
        break;
      }
    }
  }

  return result;
}

/* Each region will only contain a single request. This makes it easier to expand (for example: call_depth(100) subrequests of a
 * region's annotation containing multiple annotation requests */
void CreateRegionsFromAnnotationsAndAnnotateBbls(t_cfg *cfg, const Annotations& annotations)
{
  CFG_SET_REGIONS(cfg, new Regions());

  for (auto annotation : annotations)
  {
    BblVector bbls;
    t_uint32 idx = CFG_REGIONS(cfg)->size();

    /* create list of all BBLs in region */
    if (annotation->file_name)
    {
      if (annotation->line_begin != UINT32_MAX
          && annotation->line_end != UINT32_MAX)
      {
        /* range specified */
        VERBOSE(0, ("Creating region %u for file '%s', line range %u-%u", idx, annotation->file_name->c_str(), annotation->line_begin, annotation->line_end));
        bbls = GetAllBblsInFileLineRange(cfg, annotation->file_name, annotation->line_begin, annotation->line_end);
      }
      else
      {
        /* no range specified */
        if (annotation->file_is_object_file)
        {
          VERBOSE(0, ("Creating region %u for object file '%s'", idx, annotation->file_name->c_str()));
          bbls = GetAllBblsInObjectFile(cfg, annotation->file_name);
        }
        else
        {
          VERBOSE(0, ("Creating region %u for file '%s'", idx, annotation->file_name->c_str()));
          bbls = GetAllBblsInFile(cfg, annotation->file_name);
        }
      }
    }
    else if (annotation->function_name)
    {
      /* function name specified, also processes wildcards */
      VERBOSE(0, ("Creating region %u for function '%s' (pattern matching)", idx, annotation->function_name->c_str()));
      bbls = GetAllBblsInFunction(cfg, annotation->function_name);
    }
    else
      FATAL(("no file name nor function name was given for this annotation"));

    VERBOSE(0, ("    this region contains %u BBLs", bbls.size()));

    if (bbls.empty())
    {
      /* no BBLs found */
      WARNING(("No BBLs found for this region. Multiple causes are possible (add '-v' to the commandline of Diablo for possible reasons)"));
      VERBOSE(1, ("  - the source code has not been compiled with debug information (compiler flag '-g')"));

      if (annotation->file_name)
        VERBOSE(1, ("  - the specified file path and name '%s' may be incorrect", annotation->file_name->c_str()));
      if (annotation->function_name)
      {
        VERBOSE(1, ("  - the function '%s' may be inlined", annotation->function_name->c_str()));
        VERBOSE(1, ("  - the function '%s' might have been unreachable, and hence has been removed by Diablo", annotation->function_name->c_str()));
      }

       // TODO output this explicitly in the regions output file?

      continue;
    }

    /* parse annotation content */
    auto requests = ParseAnnotationContent(annotation);

    VERBOSE(0, ("    this region has %i annotation requests, creating a region for each", (int) requests.size()));

    if (requests.size() > 0) {
      for (auto request : requests)
        new Region(cfg, annotation, bbls, request);
    } else {
      /* Add empty region: this is an annotation that Diablo didn't understand, but we might still need metrics */
      VERBOSE(0, ("No Diablo-annotations recognized for this region!!"));
      new Region(cfg, annotation, bbls, nullptr);
    }
  }
}

void IoModifierBblRegions(t_bbl *bbl, t_string_array *array)
{
  if (!init)
    return;

  std::string str;

  /* construct string */
  if (BBL_REGIONS(bbl) != NULL)
  {
    for (auto region : *BBL_REGIONS(bbl))
      str += std::to_string(region->idx) + " ";
  }

  /* Only append to array if a useful string was created */
  if (!str.empty())
    StringArrayAppendString(array, StringConcat3("Regions: ", str.c_str(), "\\l"));
}

void RegionAddBbl(Region *region, t_bbl *bbl)
{
  /* Add the BBL to the region */
  region->bbls.insert(bbl);

  /* add region list to BBL if necessary */
  if (BBL_REGIONS(bbl) == NULL)
    BBL_SET_REGIONS(bbl, new Regions());

  /* Add the region to the BBL */
  BBL_REGIONS(bbl)->insert(region);
}

/* This function iterates over all the BBL's in the region and inserts their function to the unordered_set
 * (no duplicates allowed, this is handled by the insert call).
 */
OrderedFunctionSet RegionGetAllFunctions(const Region* region)
{
  OrderedFunctionSet functions;
  t_bbl* bbl;
  REGION_FOREACH_BBL(region, bbl)
  {
    if (!IS_DATABBL(bbl))
      functions.insert(BBL_FUNCTION(bbl));
  }

  return functions;
}

void BblCopyRegions(t_bbl *from, t_bbl *to)
{
  if (!init) return;

  if (!BBL_REGIONS(from))
    return;

  ASSERT(from != to, ("what?"));
  ASSERT(!BBL_REGIONS(to), ("did not expect @eiB to be already in a region", to));
  BBL_SET_REGIONS(to, new Regions());

  for (auto region : *BBL_REGIONS(from))
    RegionAddBbl(region, to);
}

void BblRemoveFromRegion(Region *region, t_bbl *bbl)
{
  if (!init) return;

  /* Remove the BBL from the region */
  region->bbls.erase(bbl);

  if (!BBL_REGIONS(bbl))
    return;

  /* Remove the region from the BBL */
  BBL_REGIONS(bbl)->erase(region);
}

void BblRemoveFromAllRegions(t_bbl *bbl)
{
  if (!init) return;

  if (!BBL_REGIONS(bbl))
    /* this BBL is not associated with any region */
    return;

  /* Remove the BBL from all regions */
  for (auto region : *BBL_REGIONS(bbl))
      region->bbls.erase(bbl);

  delete BBL_REGIONS(bbl);
  BBL_SET_REGIONS(bbl, NULL);
}

void BblDupCallbackRegionBookkeeping(void *a, void *b)
{
  t_bbl *from = static_cast<t_bbl *>(global_hack_dup_orig);
  t_bbl *to   = static_cast<t_bbl *>(a);

  BblCopyRegions(from, to);
}

void SelectBblsFromRegionWithProfile(BblVector& result, SelectBblFromRegionCallback callback,
                                     const Region *region, t_uint32 select_percent,
                                     t_bool exclude_not_executed, t_bool select_most_executed)
{
  BblVector candidates;

  t_bbl* bbl;
  REGION_FOREACH_BBL(region, bbl)
  {
    if (!callback(bbl))
      continue;

    if (BBL_EXEC_COUNT(bbl) == 0
        && exclude_not_executed)
      continue;

    candidates.push_back(bbl);
  }

  /* Use stable sort, so elements with the same execution count are kept in their original order, so we do not introduce randomness */
  if (select_most_executed)
  {
    struct ExecCountComparator {
      bool operator()(t_bbl* left, t_bbl* right) {
        return BBL_EXEC_COUNT(left) > BBL_EXEC_COUNT(right);
      }
    };

    VERBOSE(1, ("selecting most executed BBLs"));
    stable_sort(candidates.begin(), candidates.end(), ExecCountComparator());
  }
  else
  {
    struct ExecCountComparator {
      bool operator()(t_bbl* left, t_bbl* right) {
        return BBL_EXEC_COUNT(left) < BBL_EXEC_COUNT(right);
      }
    };

    VERBOSE(1, ("selecting least executed BBLs"));
    stable_sort(candidates.begin(), candidates.end(), ExecCountComparator());
  }

  /* For now, just take the least executed BBLs. TODO In future, we might want to randomize this *slightly* */
  candidates.resize((float(candidates.size()) * float(select_percent)) / 100.0);

  /* insert the listed BBLs in the result vector */
  result.insert(result.end(), candidates.begin(), candidates.end());
}

void SelectBblsFromRegionWithoutProfile(BblVector& result, SelectBblFromRegionCallback callback,
                                        const Region *region, t_uint32 select_percent,
                                        t_randomnumbergenerator *rng)
{
  t_randomnumbergenerator *rng_select = RNGCreateChild(rng, "select_noprofile");
  RNGSetRange(rng_select, 0, 100);

  t_bbl* bbl;
  REGION_FOREACH_BBL(region, bbl) {
    if (!callback(bbl))
      continue;

    if (RNGGenerate(rng_select) <= select_percent) {
      result.push_back(bbl);
      VERBOSE(1, ("Accepted BBL"));
    } else {
      VERBOSE(1, ("Rejected BBL"));
    }
  }

  RNGDestroy(rng_select);
}

void SelectBblsFromRegion(BblVector& result, SelectBblFromRegionCallback callback,
                          const Region *region, t_uint32 select_percent,
                          t_randomnumbergenerator *rng,
                          t_bool exclude_not_executed, t_bool select_most_executed)
{
  VERBOSE(1, ("Selecting BBLs from region with chance: %i", select_percent));

  if (!diabloflowgraph_options.blockprofilefile) {
    VERBOSE(1, ("No profile found, randomly selecting BBLs from region"));
    return SelectBblsFromRegionWithoutProfile(result, callback, region, select_percent, rng);
  } else {
    VERBOSE(1, ("Using profile information to select BBLs from region"));
    return SelectBblsFromRegionWithProfile(result, callback, region, select_percent, exclude_not_executed, select_most_executed);
  }
}

t_bool IsBblInRegion(t_bbl *bbl, Region *region)
{
  Region *region_it;
  BBL_FOREACH_REGION(bbl, region_it)
    if (region_it == region)
      return TRUE;

  return FALSE;
}

static t_function* GetIPTarget(t_bbl* bbl)
{
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    switch(CFG_EDGE_CAT(edge)) {
      case ET_IPJUMP:
      case ET_IPFALLTHRU:

        if(!BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
          return BBL_FUNCTION(CFG_EDGE_TAIL(edge));
      default: ;
    }
  }

  return nullptr;
}


static t_function* GetCallee(t_bbl* bbl)
{
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    if (CFG_EDGE_CAT(edge) == ET_CALL)
      if(!BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
        return BBL_FUNCTION(CFG_EDGE_TAIL(edge));
  }

  return nullptr;
}

/* DFS with a slight twist: we can re-visit a node if the current depth_left we would visit it with is higher than its prior depth_left */
static void DFS(Region* region, map<t_function*, int>& processed, t_function* head_node, int depth_left) {
  if (depth_left <= 0)
    return;
  if (!head_node)
      return;

  VERBOSE(1, ("Adding function '%s' to a region at call depth %i", FUNCTION_NAME(head_node), depth_left));

  processed[head_node] = depth_left;

  t_bbl* bbl;
  FUNCTION_FOREACH_BBL(head_node, bbl) {
    RegionAddBbl(region, bbl);

    auto next = GetCallee(bbl);
    int next_depth = depth_left - 1;

    VERBOSE (2, ("Potential callee is: %p, this from @eiB", next, bbl));

    if(!next) {
      /* For a C(++) source developer / annotator, IP jumps/fallthroughs in binary will probably be seen as still belonging to the original function, handle this */
      next = GetIPTarget(bbl);
      if (!next)
        continue;

      VERBOSE(1, ("Considering an IP target (%s) as part of the same function", FUNCTION_NAME(next)));
      next_depth = depth_left;
    }

    auto previous = processed.find(next);
    if ( (previous == processed.end()) || (previous->second < next_depth) ) {
      DFS(region, processed, next, next_depth);
    }
  }
}

void ExpandRegionToCalleesOfDepth(Region* region, int call_depth)
{
  map<t_function*, int> processed;

  VERBOSE(1, ("Expanding region to callees (depth: %i)", call_depth));

  t_bbl* bbl;
  vector<t_function*> callees;

  /* WARNING! We cannot call DFS in a REGION_FOREACH_BBL, because this is not a safe iterator, and RegionAddBbl invalidates the iterator */
  REGION_FOREACH_BBL(region, bbl) {
    if (!bbl)
      continue;

    auto function = GetCallee(bbl);

    if (function) {
      callees.push_back(function);
    }
  }

  for (auto callee: callees) {
    DFS(region, processed, callee, call_depth);
  }
}

/* TODO: take into account regions the fact that a single annotation can get split up internally in multiple (expanded) regions! */
// TODO: merge BBLs of Regions with the same index!! TODO do not remove Regions that Diablo does not understand, so their stats can be computed too
// TODO: more source code factoring/merging here
void LogRegionsStaticComplexity(t_const_string filename, t_cfg* cfg) {
  FILE* f = fopen(filename, "w");

  StaticComplexity c; /* TODO: a bit ugly? */
  c.printHeader(f);

  /* The same annotation can give rise to multiple regions */
  set<t_uint32> seen_index;

  for (auto region: *CFG_REGIONS(cfg)) {
    if (region->annotation)
    {
      if (seen_index.find(region->annotation->index) != seen_index.end())
        continue;

      seen_index.insert(region->annotation->index);
    }


    StaticComplexity complexity = BblsComputeStaticComplexity(region->bbls);
    complexity.printComplexityMetricsLine(f, region->annotation ? region->annotation->index : -2);/* Some regions don't have an annotation */
  }

  fclose(f);
}

void LogRegionsDynamicComplexity(t_const_string filename, t_cfg* cfg) {
  FILE* f = fopen(filename, "w");

  DynamicComplexity c; /* TODO: a bit ugly? */
  c.printHeader(f);

  for (auto region: *CFG_REGIONS(cfg)) {
    DynamicComplexity complexity = BblsComputeDynamicComplexity(region->bbls);
    complexity.printComplexityMetricsLine(f, region->annotation ? region->annotation->index : -2);/* Some regions don't have an annotation */
  }

  fclose(f);
}

Region::Region(t_cfg* cfg, Annotation* annotation, const BblSet& bbls, AbstractAnnotationInfo* request)
  : annotation(annotation), bbls(bbls), idx(CFG_REGIONS(cfg)->size())
{
  if (request)
    requests.push_back(request);
  /* Associate BBLs with regions. We do this without RegionAddBbl. Because bbls has unique members
   * and the region has just been created we know this function to be overkill.
  */
  for (auto bbl : bbls)
  {
    if (BBL_REGIONS(bbl) == NULL)
      BBL_SET_REGIONS(bbl, new Regions());

    BBL_REGIONS(bbl)->insert(this);
  }

  /* Store this instance and do preprocessing */
  CFG_REGIONS(cfg)->insert(this);
  if (request)
    request->preprocessRegion(this); /* This can expand the region (for example, with a call_depth parameter) */
}

bool RegionGetValueForIntOption(const Region *region, t_const_string option_name, int& value) {
  for (auto request : region->requests)
    if (request->GetValueForIntOption(option_name, value))
      return true;

  return false;
}
