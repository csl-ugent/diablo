#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_BOOKKEEPING_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_BOOKKEEPING_H

void AssociateSliceAndBbl(t_bbl *bbl, Slice *slice);
void DissociateSliceAndBbl(t_bbl *bbl, Slice *slice);
void BblInvalidate(t_bbl *bbl);
void UpdateAssociatedSlices(t_bbl *bbl);
void AfterSplit(t_bbl *first, t_bbl *second, bool propagate = TRUE);
void BblPropagateConstantInformation(t_bbl *bbl, t_analysis_complexity complexity);
void BblPropagateCopyInformation(t_bbl *bbl, t_cfg_edge *edge);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_BOOKKEEPING_H */
