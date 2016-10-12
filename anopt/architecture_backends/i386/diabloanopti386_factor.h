#include <diabloanopti386.h>
#ifndef I386_FACTOR_H
#define I386_FACTOR_H
t_uint32 I386BblCmpExtReg(t_bbl * master,t_bbl * slave,t_uint32 * in);
t_uint32 I386BblAbstract(t_bbl * master,t_bbl * slave,t_uint32 * in,t_uint32 * out);
t_bool I386BblCanFactor(t_bbl *bbl);
t_bool I386BblFactor(t_equiv_bbl_holder *equivs, t_bbl *master);
t_bool I386BblFactorConditional(t_equiv_bbl_holder *equivs, t_bbl *master, t_bool limited);
void I386BblFactorInit(t_cfg * cfg);
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
