/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS arm_ins
#define arm_ins_field_select_prefix ARM_INS
#define arm_ins_function_prefix ArmIns
#endif

#ifdef ARM_INS_NEXT
#undef ARM_INS_NEXT
#endif

#ifdef ARM_INS_PREV
#undef ARM_INS_PREV
#endif

/*! \brief This class is used to represent arm instructions.
 *
 * Arm specific code uses this representation, generic code
 * uses the generic representation */
DIABLO_CLASS_BEGIN
/*! \brief The generic instruction */
EXTENDS(t_ins)
/*! \brief internal representation of the instruction's opcode */
IMEMBER(t_arm_opcode,opcode,OPCODE)
MEMBER(t_thumb_opcode,thumbopcode,THUMBOPCODE)
/*! \brief destination register */
MEMBER(t_reg,rega,REGA)
/*! \brief operand register 1 */
MEMBER(t_reg,regb,REGB)
/*! \brief operand register 2 */
MEMBER(t_reg,regc,REGC)

MEMBER(t_uint32,vrega,VREGA)
MEMBER(t_uint32,vregb,VREGB)
MEMBER(t_uint32,vregc,VREGC)
MEMBER(t_uint32,vregabis,VREGABIS)
MEMBER(t_uint32,vregs,VREGS)
MEMBER(t_uint8,use,USE)
MEMBER(t_uint8,def,DEF)

/*! \brief shift register
 *
 * This field holds the distance over which the third operand has to be
 * shifted, if this distance is given as a register operand. */
MEMBER(t_reg,regs,REGS)
/*! \brief immediate operand
 *
 * This operand has a meaningful value only if the
 * FL_IMMED flag is set in the t_arm_ins::flags field */
MEMBER(t_int64,immediate,IMMEDIATE)
/*! \brief multiple regs operand 
 *
 * This field contains the multiple-registers operand
 * for LDM and STM instructions */
MEMBER(t_regset,multiple,MULTIPLE)
/*! \brief ARM instruction specific flags (set CPSR, ...)
 * \todo Enumerate and document flags */
MEMBER(t_instruction_flags,flags,FLAGS)
/*! \brief Condition code */
IMEMBER(t_arm_condition_code,condition,CONDITION)
/*! \brief The type of shift applied to the second source operand */
MEMBER(t_arm_shift_type,shifttype,SHIFTTYPE)
/*! \brief immediate shift length
 *
 * This field holds the distance over which the third operand has to be
 * shifted, if this distance is given as a constant value. */
MEMBER(t_uint8,shiftlength,SHIFTLENGTH)		
/*! \brief Holds float data for ARM_FLOAT_PRODUCER */
MEMBER(char *,data,DATA)
/*! \brief Holds information about the original instructions that are replaced by an ARM_ADDRESS_PRODUCER pseudo-instruction.
 *
 * \todo This is useful for FIT only: move to a dynamic member */
MEMBER(t_arm_addr_info *,info,INFO)

/*! \brief Used for SIMD instructions to indicate the datatype on which the instruction works ('dt' in the manuals)
 */
MEMBER(t_arm_ins_dt, datatype, DATATYPE)
/*! \brief For the VCVT instructions: datatype of the operand (source)
 */
MEMBER(t_arm_ins_dt, datatypeop, DATATYPEOP)

/* Bis A-register for register transfer between 2 core registers (FP) */
MEMBER(t_reg, regabis, REGABIS)

/*! \brief Used for NEON instructions to indicate the type of register, ...
 */
MEMBER(t_arm_neon_flags, neonflags, NEONFLAGS)

/*! \brief Used to store the scalar index field of the B and C registers.
 */
MEMBER(t_uint8, regascalar, REGASCALAR)
/* !!! assume only one register is scalar-indexed per instruction,
 * We do this in order to save memory...
 */
//MEMBER(t_uint32, regbscalar, REGBSCALAR)
//MEMBER(t_uint32, regcscalar, REGCSCALAR)

/*! \brief Used to store the alignment of the NEON multiple load/store instructions.
 * If this is 0 or 1, this field can be ignored (1-byte alignment).
 */
MEMBER(t_uint8, multiplealignment, MULTIPLEALIGNMENT)
MEMBER(t_uint8, multiplescalar, MULTIPLESCALAR)

DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define ARM_INS_NEXT(x) ({ FATAL(("Do not use ARM_INS_NEXT: Use ARM_INS_INEXT instead")); NULL; })
#define ARM_INS_PREV(x) ({ FATAL(("Do not use ARM_INS_PREV: Use ARM_INS_IPREV instead")); NULL; })

/* assume only one register is scalar-indexed per instruction */
#define ARM_INS_REGBSCALAR(x) ARM_INS_REGASCALAR(x)
#define ARM_INS_SET_REGBSCALAR(x,y) ARM_INS_SET_REGASCALAR(x,y)
#define ARM_INS_REGCSCALAR(x) ARM_INS_REGASCALAR(x)
#define ARM_INS_SET_REGCSCALAR(x,y) ARM_INS_SET_REGASCALAR(x,y)

#define ARM_INS_USEDEF_REGA     0x01
#define ARM_INS_USEDEF_REGABIS  0x02
#define ARM_INS_USEDEF_REGB     0x04
#define ARM_INS_USEDEF_REGC     0x08
#define ARM_INS_USEDEF_REGS     0x10

#define ARM_INS_USES_REGA(ins) (ARM_INS_USE(ins) & ARM_INS_USEDEF_REGA)
#define ARM_INS_USES_REGABIS(ins) (ARM_INS_USE(ins) & ARM_INS_USEDEF_REGABIS)
#define ARM_INS_USES_REGB(ins) (ARM_INS_USE(ins) & ARM_INS_USEDEF_REGB)
#define ARM_INS_USES_REGC(ins) (ARM_INS_USE(ins) & ARM_INS_USEDEF_REGC)
#define ARM_INS_USES_REGS(ins) (ARM_INS_USE(ins) & ARM_INS_USEDEF_REGS)

#define ARM_INS_DEFS_REGA(ins) (ARM_INS_DEF(ins) & ARM_INS_USEDEF_REGA)
#define ARM_INS_DEFS_REGABIS(ins) (ARM_INS_DEF(ins) & ARM_INS_USEDEF_REGABIS)
#define ARM_INS_DEFS_REGB(ins) (ARM_INS_DEF(ins) & ARM_INS_USEDEF_REGB)
#define ARM_INS_DEFS_REGC(ins) (ARM_INS_DEF(ins) & ARM_INS_USEDEF_REGC)
#define ARM_INS_DEFS_REGS(ins) (ARM_INS_DEF(ins) & ARM_INS_USEDEF_REGS)

#define ARM_INS_SET_VREGA_TYPE(ins, t) ARM_INS_SET_VREGA(ins, (ARM_INS_VREGA(ins) & 0x00ffffff) | ((t) << 24))
#define ARM_INS_SET_VREGABIS_TYPE(ins, t) ARM_INS_SET_VREGABIS(ins, (ARM_INS_VREGABIS(ins) & 0x00ffffff) | ((t) << 24))
#define ARM_INS_SET_VREGB_TYPE(ins, t) ARM_INS_SET_VREGB(ins, (ARM_INS_VREGB(ins) & 0x00ffffff) | ((t) << 24))
#define ARM_INS_SET_VREGC_TYPE(ins, t) ARM_INS_SET_VREGC(ins, (ARM_INS_VREGC(ins) & 0x00ffffff) | ((t) << 24))
#define ARM_INS_SET_VREGS_TYPE(ins, t) ARM_INS_SET_VREGS(ins, (ARM_INS_VREGS(ins) & 0x00ffffff) | ((t) << 24))

#define ARM_INS_SET_VREGA_VALUE(ins, v) ARM_INS_SET_VREGA(ins, (ARM_INS_VREGA(ins) & 0xff000000) | (v))
#define ARM_INS_SET_VREGABIS_VALUE(ins, v) ARM_INS_SET_VREGABIS(ins, (ARM_INS_VREGABIS(ins) & 0xff000000) | (v))
#define ARM_INS_SET_VREGB_VALUE(ins, v) ARM_INS_SET_VREGB(ins, (ARM_INS_VREGB(ins) & 0xff000000) | (v))
#define ARM_INS_SET_VREGC_VALUE(ins, v) ARM_INS_SET_VREGC(ins, (ARM_INS_VREGC(ins) & 0xff000000) | (v))
#define ARM_INS_SET_VREGS_VALUE(ins, v) ARM_INS_SET_VREGS(ins, (ARM_INS_VREGS(ins) & 0xff000000) | (v))

#define ARM_INS_VREG_VALUE(x) ((x) & 0x00ffffff)
#define ARM_INS_VREG_TYPE(x) (((x) >> 24) & 0xff)

#define ARM_INS_VREG_VALUE_I(x)  ((x) & 0x000000ff)
#define ARM_INS_VREG_VALUE_O(x) (((x) & 0x0000ff00) >> 8)
#define ARM_INS_VREG_VALUE_i(x) (((x) & 0x00ff0000) >> 16)

#define ARM_INS_CREATE_VREG(value, type) ((value) | ((type) << 24))
