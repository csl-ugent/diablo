/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

#ifdef DIABLOARM_FUNCTIONS
#ifndef ARM_ASSEMBLE_FUNCTIONS
#define ARM_ASSEMBLE_FUNCTIONS
void ArmAssembleSection(t_section * sec);
void ArmAssembleOne(t_arm_ins * i_ins, char * data);
t_bool ArmParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before);

t_uint32 ASM_NEON_VD_S(t_uint32 reg);
t_uint32 ASM_NEON_VD_QD(t_uint32 reg);
t_uint32 ASM_NEON_VN_S(t_uint32 reg);
t_uint32 ASM_NEON_VN_QD(t_uint32 reg);
t_uint32 ASM_NEON_VM_S(t_uint32 reg);
t_uint32 ASM_NEON_VM_QD(t_uint32 reg);
t_uint32 ASM_NEON_VM_SCALAR(t_uint32 reg, t_uint32 index, t_arm_ins_dt size);
t_bool ASM_DATATYPE_IS_SIGNED(t_arm_ins * ins);
t_arm_ins_dt ASM_DATATYPE_NORMALIZE(t_arm_ins * ins, t_arm_ins_dt * base_type);
t_reg ASM_MULTIPLE_FIND_DREG_BLOB(t_regset r, t_uint32 * length);
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
