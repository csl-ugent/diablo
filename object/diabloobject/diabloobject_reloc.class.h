#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS reloc
#define reloc_field_select_prefix RELOC
#define reloc_function_prefix Reloc
#endif



/*! A relocation. Relocations describe the relation between data in one
 * relocatable and a referenced relocatable. There are two different
 * representations: a symbolical representation and the normal representation
 * used in diablo. The symbolic representation has a pointer to a symbol, which
 * holds a reference to a relocatable and is used during linking. The normal
 * representation simply holds a reference to a relocatable */
DIABLO_CLASS_BEGIN
MEMBER(t_CLASS *, prev, PREV)
MEMBER(t_CLASS *, next, NEXT)
MEMBER(t_relocatable *, from, FROM)
MEMBER(t_address, from_offset, FROM_OFFSET)
MEMBER(t_reloc_ref *, from_ref, FROM_REF)
MEMBER(t_uint32, n_to_relocatables, N_TO_RELOCATABLES)
MEMBER(t_relocatable **, to_relocatable, TO_RELOCATABLE)
MEMBER(t_address *, to_relocatable_offset, TO_RELOCATABLE_OFFSET)
MEMBER(t_reloc_ref **, to_relocatable_ref, TO_RELOCATABLE_REF)
MEMBER(t_uint32, n_to_symbols, N_TO_SYMBOLS)
MEMBER(t_symbol **, to_symbol, TO_SYMBOL)
MEMBER(t_address *, to_symbol_offset, TO_SYMBOL_OFFSET)
MEMBER(t_reloc_ref **, to_symbol_ref, TO_SYMBOL_REF)
MEMBER(t_uint32, n_addends, N_ADDENDS)
MEMBER(t_address *, addends, ADDENDS)
MEMBER(t_reloc_table *,table, TABLE)
MEMBER(t_string, label, LABEL)
/** Does this relocation lead to a hell edge in the graph? */
MEMBER(t_bool, hell, HELL)
/** The edge that was created because of this relocation */
MEMBER(void *,edge, EDGE)
/** The switch edge that was created because of this relocation */
MEMBER(void *,switch_edge, SWITCH_EDGE)
/** The stack machine program that calculates the relocation */
MEMBER(t_string, code, CODE)
DIABLO_CLASS_END
