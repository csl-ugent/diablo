/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>
#ifndef ARM_FACTOR_H
#define ARM_FACTOR_H
t_uint32 ArmBblFingerprint(t_bbl *bbl);
t_bool ArmBblCanFactor(t_bbl *bbl);
t_bool ArmBblFactor(t_equiv_bbl_holder *equivs, t_bbl *master);
void ArmBblFactorInit(t_cfg * cfg);
void ArmEpilogueFactorAfter(t_bbl * bbl, t_bbl * master);
void ArmFunctionFactorAfterJumpCreation(t_bbl * from, t_bbl * to);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
