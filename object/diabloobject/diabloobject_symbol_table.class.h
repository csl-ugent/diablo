#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS symbol_table
#define symbol_table_field_select_prefix SYMBOL_TABLE
#define symbol_table_function_prefix SymbolTable
#endif



/*! A symbol table */
DIABLO_CLASS_BEGIN
EXTENDS(t_hash_table)

MEMBER(t_object *,object, OBJECT)
MEMBER(t_symbol *,first, FIRST)
MEMBER(t_symbol *,last, LAST)
MEMBER(t_uint32, nsyms, NSYMS)

DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS hash_table
#include <diablosupport_hash_table.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
