#ifndef DIABLOFLOWGRAPH_OBJECT_TRACKING_H
#define DIABLOFLOWGRAPH_OBJECT_TRACKING_H

typedef t_uint32 ObjectSetUID;

#define ORIGIN_INITIAL_DIRECTORY "origin_initial"
#define ORIGIN_FINAL_DIRECTORY "origin_final"

typedef t_uint32 FunctionUID;

#ifdef __cplusplus
extern "C" {
#endif

/* In order for this tracking functionality to work correctly, the following preparations should be made:
 *  - Right after the creation of the CFG, call 'RecordFunctionsAsOriginal(cfg)'.
 *  - Right after the first call set to 'CfgRemoveDeadCodeAndDataBlocks', 'CfgPatchToSingleEntryFunctions' and
 *    'CfgRemoveDeadCodeAndDataBlocks', call 'InitialiseObjectFileTracking(cfg)'.
 */
void InitialiseObjectFileTracking(t_cfg *cfg);
void FinalizeObjectTracking(t_cfg *cfg);
void TrackOriginInformation(t_cfg *cfg, t_string directory);
void DisableOriginTracking();

static inline
void BblInheritSetInformation(t_bbl *new_bbl, t_bbl *origin) {
  BBL_SET_OBJECT_SET(new_bbl, BBL_OBJECT_SET(origin));
}

static inline
void BblCopyExecInformation(t_bbl *from, t_bbl *to) {
  BBL_SET_EXEC_COUNT(to, BBL_EXEC_COUNT(from));
}

static inline
void BblCopyExecInformationToEdge(t_bbl *from, t_cfg_edge *to) {
  CFG_EDGE_SET_EXEC_COUNT(to, BBL_EXEC_COUNT(from));
}

static inline
void BblCopyExecInformationFromEdge(t_cfg_edge *from, t_bbl *to) {
  BBL_SET_EXEC_COUNT(to, CFG_EDGE_EXEC_COUNT(from));
}

static inline
void BblSetOriginalFunctionUID(t_bbl *bbl, FunctionUID uid) {
  BBL_SET_ORIGINAL_FUNCTION(bbl, uid);
}

static inline
FunctionUID BblOriginalFunctionUID(t_bbl *bbl) {
  return BBL_ORIGINAL_FUNCTION(bbl);
}

void UpdateObjectTrackingAfterBblSplit(t_bbl *first, t_bbl *second);
void UpdateObjectTrackingAfterEdgeSplit(t_cfg_edge *split_edge, t_bbl *new_bbl, t_bbl *original_tail);
void UpdateObjectTrackingAfterBblInsertInFunction(t_bbl *bbl);
void UpdateObjectTrackingBeforeKillingEdge(t_cfg_edge *edge);

#ifdef __cplusplus
}
#endif

#endif
