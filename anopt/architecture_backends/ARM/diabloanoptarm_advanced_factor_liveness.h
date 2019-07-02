#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_LIVENESS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_LIVENESS_H

bool PropagateLiveRegister(t_bbl *bbl, t_arm_ins *ins, t_reg reg);
void LivenessForAssumptions(t_cfg *cfg);
void PropagateLivenessActions(t_bbl *bbl);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_LIVENESS_H */
