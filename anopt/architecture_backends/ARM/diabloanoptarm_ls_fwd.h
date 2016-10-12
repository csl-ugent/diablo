/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanoptarm.h>

#ifndef ARM_LS_FWD_TYPEDEFS
#define ARM_LS_FWD_TYPEDEFS
/*! \todo Document */
typedef struct _t_partial_value t_partial_value;
#endif

#ifndef ARM_LS_FWD_H
#define ARM_LS_FWD_H

void ArmLoadStoreFwd(t_cfg* cfg);
void ArmLoadStoreFwdNoDom(t_cfg* cfg);

t_lattice_level ArmPartialEvaluation(t_arm_ins * ins, t_register_content * offset, t_reg reg);

/*! \todo Document */
struct _t_partial_value
{
  t_regset registers;
  t_int32 offset[MAX_REG_ITERATOR];
  t_bool  changed[MAX_REG_ITERATOR];
  t_uint32 value[MAX_REG_ITERATOR];
  t_lattice_level level[MAX_REG_ITERATOR];
  t_reg   corresponding[MAX_REG_ITERATOR];
  t_bool double_registers;
};

#endif
/*@}*/
/* vim: set shiftwidth=2 foldmethod=marker: */
