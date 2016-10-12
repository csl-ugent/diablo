#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS i386_ins
#define i386_ins_field_select_prefix I386_INS
#define i386_ins_function_prefix I386Ins
#endif

#ifdef I386_INS_NEXT
#undef I386_INS_NEXT
#endif

#ifdef I386_INS_PREV
#undef I386_INS_PREV
#endif

/*! \brief This class is used to represent i386/x86 instructions. 
 *
 * x86 specific code uses this representation, generic code
 * uses the generic representation */

DIABLO_CLASS_BEGIN
/*! \brief The generic instruction */
EXTENDS(t_ins)
/*! \brief instruction opcode (internal diablo representation) */
MEMBER(t_i386_opcode,opcode,OPCODE)
/*! \brief destination operand
 *
 * \todo document the t_i386_operand structure */
MEMBER(t_i386_operand *,dest,DEST)
/*! \brief first source operand */
MEMBER(t_i386_operand *,source1,SOURCE1)
/*! \brief second source operand */
MEMBER(t_i386_operand *,source2,SOURCE2)
/*! \brief address producer info - useful for instrumentation 
 *
 * \todo This field is really only useful for FIT -- move it to a dynamic member */
MEMBER(t_CLASS *,ap_original,AP_ORIGINAL)
/*! \brief bit field indicating all instruction prefixes */
MEMBER(t_uint16,prefixes,PREFIXES)
/*! \brief i386-specific instruction flags 
 * 
 * Current flags are:
 * 	- I386_IF_DEST_IS_SOURCE: destination operand is also used as a source operand
 * 	- I386_IF_SOURCE1_DEF: first source operand is also defined
 * 	- I386_IF_SOURCE2_DEF: second source operand is also defined
 * 	- I386_IF_JMP_FORCE_4BYTE: force this jmp instruction to have a 4-byte
 * 	  displacement. If you do not specify this flag, the assembler will try to
 * 	  generate a one-byte displacement instead.
 * 	*/
MEMBER(t_uint16,flags,FLAGS)
/*! \brief condition code for conditional instructions */
MEMBER(t_i386_condition_code,condition,CONDITION)
/*! \brief one byte of data if the instruction represents data in code */
MEMBER(t_uint8,data,DATA)
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS


#define I386_INS_NEXT(x) ({ FATAL(("Do not use I386_INS_NEXT: Use I386_INS_INEXT instead")); NULL; })
#define I386_INS_PREV(x) ({ FATAL(("Do not use I386_INS_PREV: Use I386_INS_IPREV instead")); NULL; })
