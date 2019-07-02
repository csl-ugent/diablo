#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

t_uint32 nr_candidate_slices;

void AddSetToStatistics(SliceSet slices, size_t n_ins, FactoringResult insn_stats)
{
  BblSet bbls;
  for (auto slice : slices)
    bbls.insert(slice->Bbl());

  FactoringRecordTransformation(bbls, n_ins, insn_stats);
}

SpecialFunctionTrackResults AdvancedFactoringOriginTracking(t_bbl *bbl) {
  SpecialFunctionTrackResults result;

  /* calculate associated-with count */
  result.associated_with_functions.clear();

  t_bbl *entry = FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl));

  t_int64 total_exec = 0;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(entry, e) {
    /* dummy switch entries can be represented by an infinite loop */
    if (CFG_EDGE_CAT(e) == ET_JUMP
        && BBL_FUNCTION(CFG_EDGE_TAIL(e)) == BBL_FUNCTION(CFG_EDGE_HEAD(e)))
      continue;

    if (CfgEdgeIsFake(e))
      continue;

    if (!CfgEdgeIsInterproc(e))
      CfgDrawFunctionGraphsWithHotness(BBL_CFG(entry), "boem");
    ASSERT(CfgEdgeIsInterproc(e), ("expected IP edge @E in @eiB", e, entry));

    FunctionUID f_uid = BblOriginalFunctionUID(CFG_EDGE_HEAD(e));
    ASSERT(f_uid != FunctionUID_INVALID, ("expected incoming function to be associated with original! @F @eiB", CFG_EDGE_HEAD(e), entry));

    result.associated_with_functions.insert(f_uid);

    result.exec_per_function[f_uid] = BBL_EXEC_COUNT(CFG_EDGE_HEAD(e));
    total_exec += BBL_EXEC_COUNT(CFG_EDGE_HEAD(e));
  }

  if (total_exec != BBL_EXEC_COUNT(entry))
    CfgDrawFunctionGraphs(BBL_CFG(entry), "exec");
  ASSERT(total_exec == BBL_EXEC_COUNT(entry), ("what? @eiB %d/%d", entry, total_exec, BBL_EXEC_COUNT(entry)));

  return result;
}
