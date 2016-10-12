/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS hash_table
#define hash_table_field_select_prefix HASH_TABLE
#define hash_table_function_prefix HashTable
#endif

/*! \brief A hash table */
DIABLO_CLASS_BEGIN
MEMBER(t_hash_table_node **, nodes, NODES)
MEMBER(t_uint32, nelements, NELEMENTS)
MEMBER(t_uint32, tsize, TSIZE)
MEMBER(t_uint32, offset, OFFSET)
MEMBER(t_hash_func, hash_func, HASH_FUNC)
MEMBER(t_hash_cmp, hash_cmp, HASH_CMP)
MEMBER(t_hash_node_free, node_free, NODE_FREE)


FUNCTION2(void *, Lookup, t_CLASS const *, void const *)
FUNCTION2(void, Prepend, t_CLASS *, void *)
FUNCTION2(void, Insert, t_CLASS *, void *)
/*! Delete (unlink and free) a node in the hash table
 * 
 * \param p1 the hash table
 * \param p2 he node that has to be deleted (pointer, not the key!) */
FUNCTION2(void, Delete, t_CLASS *, void const *)
/*! Unlink a node from the hash table */
FUNCTION2(void, Unlink, t_CLASS *, void *)

PFUNCTION6(void, Init, t_CLASS *, t_uint32, t_uint32, t_hash_func, t_hash_cmp, t_hash_node_free)
PFUNCTION5(t_CLASS *, New, t_uint32, t_uint32, t_hash_func, t_hash_cmp, t_hash_node_free)
PFUNCTION1(void, Fini, t_CLASS const *)
DIABLO_CLASS_END
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
