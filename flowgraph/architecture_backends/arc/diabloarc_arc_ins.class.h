#include <diablosupport_class.h> 

#ifndef CLASS
#define CLASS arc_ins
#define arc_ins_field_select_prefix ARC_INS
#define arc_ins_function_prefix ArcIns
#endif

#ifdef ARC_INS_NEXT
#undef ARC_INS_NEXT
#endif

#ifdef ARC_INS_PREV
#undef ARC_INS_PREV
#endif

/*! \brief This class is used to represent arc instructions. 
 *
 * Arc specific code uses this representation, generic code
 * uses the generic representation */
DIABLO_CLASS_BEGIN
/*! \brief The generic instruction */
EXTENDS(t_ins)
DIABLO_CLASS_END

#undef BASECLASS
#define BASECLASS ins
#include  <diabloflowgraph_ins.class.h>
#undef BASECLASS

#define ARC_INS_NEXT(x) ({ FATAL(("Do not use ARC_INS_NEXT: Use ARC_INS_INEXT instead")); NULL; })
#define ARC_INS_PREV(x) ({ FATAL(("Do not use ARC_INS_PREV: Use ARC_INS_IPREV instead")); NULL; })
