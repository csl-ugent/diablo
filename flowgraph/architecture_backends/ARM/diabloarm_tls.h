#include <diabloarm.h>

#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_TLS_FUNCTIONS
#define ARM_TLS_FUNCTIONS

void TlsCreate(t_object *obj, t_const_string name);
t_ins *TlsLoad(t_bbl *bbl, t_ins *ins, t_reg dst, t_const_string name);
t_ins *TlsStore(t_bbl *bbl, t_ins *ins, t_reg src, t_reg tmp, t_const_string name);

#endif
#endif
