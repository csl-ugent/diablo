#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS skeleton_ins
#define skeleton_ins_field_select_prefix SKELETON_INS
#define skeleton_ins_function_prefix SkeletonIns
#endif

#ifdef SKELETON_INS_NEXT
#undef SKELETON_INS_NEXT
#endif

#ifdef SKELETON_INS_PREV
#undef SKELETON_INS_PREV
#endif


/*! \brief This class is used to represent skeleton instructions. 
 *
 * Skeleton specific code uses this representation, generic code
 * uses the generic representation */
DIABLO_CLASS_BEGIN
/*! The generic instruction */
EXTENDS(t_ins)
/* add MEMBER declarations for architecture-specific fields */
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define SKELETON_INS_NEXT(x) ({ FATAL(("Do not use SKELETON_INS_NEXT: Use SKELETON_INS_INEXT instead")); NULL; })
#define SKELETON_INS_PREV(x) ({ FATAL(("Do not use SKELETON_INS_PREV: Use SKELETON_INS_IPREV instead")); NULL; })
