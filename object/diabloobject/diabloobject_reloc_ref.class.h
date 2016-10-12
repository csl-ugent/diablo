#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS reloc_ref
#define reloc_ref_field_select_prefix RELOC_REF
#define reloc_ref_function_prefix RelocRef
#endif

/*! Struct to build a list of relocation references, as used in t_ins and t_bbl
 * */
DIABLO_CLASS_BEGIN
MEMBER(t_CLASS *, next, NEXT)
MEMBER(t_CLASS *, prev, PREV)
MEMBER(t_reloc *, reloc, RELOC)
DIABLO_CLASS_END
