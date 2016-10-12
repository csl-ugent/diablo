#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS alpha_ins
#define alpha_ins_field_select_prefix ALPHA_INS
#define alpha_ins_function_prefix AlphaIns
#endif

#ifdef ALPHA_INS_NEXT
#undef ALPHA_INS_NEXT
#endif

#ifdef ALPHA_INS_PREV
#undef ALPHA_INS_PREV
#endif

/*! \brief This class is used to represent alpha instructions. 
 *
 * Alpha specific code uses this representation, generic code
 * uses the generic representation */
DIABLO_CLASS_BEGIN
/*! The generic instruction */
EXTENDS(t_ins)
/*! (translated) opcode */ 
/*MEMBER(t_uint32,opcode,OPCODE)*/
/*! operand register 1 */
MEMBER(t_reg,rega,REGA)
/*! operand register 2 */
MEMBER(t_reg,regb,REGB)
/*! destination register */ 
MEMBER(t_reg,regc,REGD)
/*! immediate operand */
MEMBER(t_int32,immediate,IMMEDIATE)
/*! Branch hint, for JSR, JMP etc */
MEMBER(t_int8, branch_hint, BRANCH_HINT)
/*! Alpha PAL code function */
MEMBER(t_int32,pal_function,PAL_FUNCTION)
/*! Instruction format */
MEMBER(t_int8, format, FORMAT)
MEMBER(char *,data,DATA)
/*! retrieve insrtuction opcode in internal format */
MEMBER(t_alpha_opcode,opcode,OPCODE)
/*! instruction flags, not sure whether I need this */
MEMBER(t_instruction_flags,flags,FLAGS)

DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define ALPHA_INS_NEXT(x) ({ FATAL(("Do not use ALPHA_INS_NEXT: Use ALPHA_INS_INEXT instead")); NULL; })
#define ALPHA_INS_PREV(x) ({ FATAL(("Do not use ALPHA_INS_PREV: Use ALPHA_INS_IPREV instead")); NULL; })
