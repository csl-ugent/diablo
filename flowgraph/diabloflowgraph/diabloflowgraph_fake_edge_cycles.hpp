#ifndef DIABLOFLOWGRAPH_FAKE_EDGE_CYCLES_HPP
#define DIABLOFLOWGRAPH_FAKE_EDGE_CYCLES_HPP

void FakeEdgeCycles(t_cfg *cfg, t_bbl *hell, TransformationID max_tf_id, t_randomnumbergenerator *rng);
BblSet BblsForTransformationID(TransformationID tf_id, t_randomnumbergenerator *rng, std::set<TransformationID> *to_choose_from = NULL);
void UpdateTransformationMetadataAfterBblSplit(t_bbl *first, t_bbl *second);
bool TransformationMetadataExists(TransformationID);

#endif /* DIABLOFLOWGRAPH_NEW_TARGET_SELECTOR_HPP */
