#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS reloc_table
#define reloc_table_field_select_prefix RELOC_TABLE
#define reloc_table_function_prefix RelocTable
#endif

/*! A reloc table */
DIABLO_CLASS_BEGIN
MEMBER(t_reloc *, first, FIRST)
MEMBER(t_reloc *, last, LAST)
MEMBER(t_object *, object, OBJECT)
MEMBER(t_uint32, nrelocs, NRELOCS)
MEMBER(t_reloc_add_edge_cb,  add_edge_callback, ADD_EDGE_CALLBACK)
MEMBER(t_reloc_del_edge_cb,  del_edge_callback, DEL_EDGE_CALLBACK)
MEMBER(t_reloc_del_switch_edge_cb, del_switch_edge_callback, DEL_SWITCH_EDGE_CALLBACK)
DIABLO_CLASS_END
