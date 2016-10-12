#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS hash_table_node
#define hash_table_node_field_select_prefix HASH_TABLE_NODE
#define hash_table_node_function_prefix HashTableNode
#endif

/*! \brief A node in a hashtable */
DIABLO_CLASS_BEGIN
MEMBER(t_CLASS *, hash_next, HASH_NEXT)
MEMBER(t_CLASS *, equal, EQUAL)
MEMBER(t_uint32, hash_key, HASH_KEY)
MEMBER(void *, key, KEY)
DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
