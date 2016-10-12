#include <diabloanoptppc.h>

#ifndef PPC_FACTOR_H
#define PPC_FACTOR_H

t_uint32 ArmBblFingerprint(t_bbl *bbl);
t_bool PpcBblCanFactor(t_bbl *bbl);
t_bool PpcBblFactor(t_equiv_bbl_holder *equivs, t_bbl *master);
void PpcBblFactorInit(t_cfg * cfg);

#endif
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
