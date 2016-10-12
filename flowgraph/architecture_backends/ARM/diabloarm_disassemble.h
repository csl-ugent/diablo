/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloobject.h>

#ifndef ARM_DISASSEMBLE_H
#define ARM_DISASSEMBLE_H
/* Includes {{{ */
#include <diablosupport.h>
/* }}} */
void * ArmDisassembleOneInstruction(t_object * obj, t_address start, int * size_ret);
void * ArmDisassembleOneInstructionForSection(t_section * sec, t_uint32 offset, int * size_ret);
void ArmDisassembleSection(t_section * code);
void DiabloFlowgraphArmCfgCreated (t_object *obj, t_cfg *cfg);
void ArmDisassembleEncoded(t_ins * ins, t_uint32 instr, t_bool is_thumb);

t_reg NEON_VD_QD(t_uint32 instr);
t_reg NEON_VN_QD(t_uint32 instr);
t_reg NEON_VM_QD(t_uint32 instr);

#define NEON_VN_S(x) (ARM_REG_S0 + ((((x) & 0x000f0000) >> 15) | (((x) >>  7) & 1)))
#define NEON_VD_S(x) (ARM_REG_S0 + ((((x) & 0x0000f000) >> 11) | (((x) >> 22) & 1)))
#define NEON_VM_S(x) (ARM_REG_S0 + ((((x) & 0x0000000f) <<  1) | (((x) >>  5) & 1)))

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
