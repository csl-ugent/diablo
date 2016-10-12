#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS relocatable
#define relocatable_field_select_prefix RELOCATABLE
#define relocatable_function_prefix Relocatable
#endif

/*!  \brief General abstraction used to represent all object file entities that
 * have an address
 *
 * Relocatables (short for relocatable objects) are used to represent all
 * object file entities that have an address, i.e. all entities that play a
 * role in the linking process. Basic blocks, instructions, sections and
 * subsections are instantions of this type */

DIABLO_CLASS_BEGIN
EXTENDS(t_node)
MEMBER(t_relocatable_type, relocatable_type, RELOCATABLE_TYPE)
MEMBER(t_address, caddress, CADDRESS)
MEMBER(t_address, csize, CSIZE)
MEMBER(t_address, old_address, OLD_ADDRESS)
MEMBER(t_address, old_size, OLD_SIZE)
MEMBER(t_address, min_address, MIN_ADDRESS)
MEMBER(t_address, min_size, MIN_SIZE)
MEMBER(t_address, alignment, ALIGNMENT)
MEMBER(t_reloc_ref *, refers_to, REFERS_TO)
MEMBER(t_reloc_ref *, refed_by, REFED_BY)
MEMBER(t_symbol_ref *, refed_by_sym, REFED_BY_SYM)
DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS node
#include <diablosupport_node.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
