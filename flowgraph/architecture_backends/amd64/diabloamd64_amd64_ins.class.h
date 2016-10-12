#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS amd64_ins
#define amd64_ins_field_select_prefix AMD64_INS
#define amd64_ins_function_prefix Amd64Ins
#endif

#ifdef AMD64_INS_NEXT
#undef AMD64_INS_NEXT
#endif

#ifdef AMD64_INS_PREV
#undef AMD64_INS_PREV
#endif

/*! \brief This class is used to represent amd64/x86_64 instructions. 
 *
 * x86_64 specific code uses this representation, generic code
 * uses the generic representation */

DIABLO_CLASS_BEGIN
/*! \brief The generic instruction */
EXTENDS(t_ins)
/*! \brief instruction opcode (internal diablo representation) */
MEMBER(t_amd64_opcode,opcode,OPCODE)
/*! \brief destination operand
 *
 * \todo document the t_i386_operand structure */
MEMBER(t_amd64_operand *,dest,DEST)
/*! \brief first source operand */
MEMBER(t_amd64_operand *,source1,SOURCE1)
/*! \brief second source operand */
MEMBER(t_amd64_operand *,source2,SOURCE2)
/*! \brief bit field indicating all instruction prefixes */
MEMBER(t_uint16,prefixes,PREFIXES)
/*! \brief amd64-specific instruction flags 
 * 
 * Current flags are:
 * 	- AMD64_IF_DEST_IS_SOURCE: destination operand is also used as a source operand
 * 	- AMD64_IF_SOURCE1_DEF: first source operand is also defined
 * 	- AMD64_IF_SOURCE2_DEF: second source operand is also defined
 * 	- AMD64_IF_JMP_FORCE_4BYTE: force this jmp instruction to have a 4-byte
 * 	  displacement. If you do not specify this flag, the assembler will try to
 * 	  generate a one-byte displacement instead.
 * 	*/
MEMBER(t_uint16,flags,FLAGS)
/*! \brief condition code for conditional instructions */
MEMBER(t_amd64_condition_code,condition,CONDITION)
/*! \brief one byte of data if the instruction represents data in code */
MEMBER(t_uint8,data,DATA)
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define AMD64_INS_NEXT(x) ({ FATAL(("Do not use AMD64_INS_NEXT: Use AMD64_INS_INEXT instead")); NULL; })
#define AMD64_INS_PREV(x) ({ FATAL(("Do not use AMD64_INS_PREV: Use AMD64_INS_IPREV instead")); NULL; })
