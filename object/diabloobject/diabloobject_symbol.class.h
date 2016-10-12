/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS symbol
#define symbol_field_select_prefix SYMBOL
#define symbol_function_prefix Symbol
#endif

/*!
 * \brief This class is used to represent symbols in Diablo
 *
 * In general, a symbol is a labeled address expression. They are kept in a
 * structure, called the symbol table. This file holds the typedefs, types,
 * function, and defines needed to create and manipulate all
 * kinds of symbols and build and manipulate the symbol table.
 */
DIABLO_CLASS_BEGIN
EXTENDS(t_hash_table_node)

MEMBER(t_int32, relocrefcount, RELOCREFCOUNT)
MEMBER(t_symbol_table *, symbol_table, SYMBOL_TABLE)
MEMBER(t_relocatable *, base, BASE)
MEMBER(t_symbol_ref *, base_ref, BASE_REF)
MEMBER(t_address, offset_from_start, OFFSET_FROM_START)
MEMBER(t_address, addend, ADDEND)
/*! Size of the symbol. Sometimes used in normal symbols (e.g the size of a
 * function) as debug information, always needed for common symbols. */
MEMBER(t_address, size, SIZE)
/*! The code to calculate the value of a symbol */
MEMBER(t_string, code, CODE)
/*! Stores the linker code needed to create the final form of a tentative
 * symbol. For normal (non-tentative) symbols this is NULL */ 
MEMBER(t_string, tentative, TENTATIVE)

MEMBER(t_int32, order, ORDER)
MEMBER(t_tristate, dup, DUP)
MEMBER(t_tristate, search, SEARCH)

MEMBER(t_string, name, NAME)
MEMBER(t_CLASS *, prev, PREV)
MEMBER(t_CLASS *, next, NEXT)
MEMBER(t_CLASS *, mapped, MAPPED)

MEMBER(t_uint32, flags, FLAGS)

/*! Static initializer, that needs to be called before the first t_symbol is
 * created. It is called from within DiabloObjectInit() */
FUNCTION0 (void, Init)
/*! Static de-initializer, needs to be called once all t_symbol's are freed to
 * avoid memory leaks. It is called from within DiabloObjectFini() */
FUNCTION0 (void, Fini)

DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS hash_table_node
#include <diablosupport_hash_node.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
