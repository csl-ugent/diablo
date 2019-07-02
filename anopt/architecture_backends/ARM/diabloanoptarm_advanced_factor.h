#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_H

/* called from C */
void ConstructBblCompareMatrices(t_cfg *cfg, t_string filename);
t_bool CanModifyJumpEdgeAF(t_cfg_edge *e);
void ArmFlipBranchesFakeEdges(t_cfg *cfg);

#endif
