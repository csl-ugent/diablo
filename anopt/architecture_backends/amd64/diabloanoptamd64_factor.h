#include <diabloanoptamd64.h>
#ifndef AMD64_FACTOR_H
#define AMD64_FACTOR_H
t_uint32 Amd64BblCmpExtReg(t_bbl * master,t_bbl * slave,t_uint32 * in);
t_uint32 Amd64BblAbstract(t_bbl * master,t_bbl * slave,t_uint32 * in,t_uint32 * out);
t_bool Amd64BblCanFactor(t_bbl *bbl);
t_bool Amd64BblFactor(t_equiv_bbl_holder *equivs, t_bbl *master);
void Amd64BblFactorInit(t_cfg * cfg);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
