#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS ppc_ins
#define ppc_ins_field_select_prefix PPC_INS
#define ppc_ins_function_prefix PpcIns
#endif

#ifdef PPC_INS_NEXT
#undef PPC_INS_NEXT
#endif

#ifdef PPC_INS_PREV
#undef PPC_INS_PREV
#endif

/*! \brief This class is used to represent ppc instructions. 
 *
 * Ppc specific code uses this representation, generic code
 * uses the generic representation */
DIABLO_CLASS_BEGIN
/*! The generic instruction */
EXTENDS(t_ins)
/* add MEMBER declarations for architecture-specific fields */
MEMBER(t_ppc_opcode,opcode,OPCODE)

MEMBER(t_reg,regt,REGT)

MEMBER(t_reg,rega,REGA)

MEMBER(t_reg,regb,REGB)

MEMBER(t_reg,regc,REGC)

MEMBER(t_address,immediate,IMMEDIATE)

MEMBER(t_uint32,flags,FLAGS)

MEMBER(t_ppc_condition_bo,bo,BO)

MEMBER(t_ppc_condition_bit,cb,CB)

MEMBER(t_ppc_condition_trap,ct,CT)

MEMBER(t_uint32,mask,MASK)

MEMBER(t_ppc_branch_hint,bh,BH)

#ifdef TYPES
     t_uint32 _hidden_sregs[PPC_SR_NUM];
     PpcAssembleFunction Assemble;
#else
#ifndef DEFINES2
MEMBER(t_uint32*, sregs, SREGS)
#endif
#endif
     
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define PPC_INS_NEXT(x) ({ FATAL(("Do not use PPC_INS_NEXT: Use PPC_INS_INEXT instead")); NULL; })
#define PPC_INS_PREV(x) ({ FATAL(("Do not use PPC_INS_PREV: Use PPC_INS_IPREV instead")); NULL; })
/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
