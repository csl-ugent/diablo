#include <diablosupport_class.h>

#ifndef CLASS
#define CLASS object_hash_node
#define object_hash_node_field_select_prefix OBJECT_HASH_NODE
#define object_hash_node_function_prefix ObjectHashNode
#endif

/*! 
 * \brief This class is used to store objects in a hash table.
 *
 * This class is used to store objects in a hash table. We cannot simply put
 * the objects themselves in the hash table as there is no one on one mapping
 * between object and hash table keys (i.e. names). Objects can have multiple
 * names (the name of the object includes the path, and this path can be
 * relative or absolute) */
DIABLO_CLASS_BEGIN
EXTENDS(t_hash_table_node)
MEMBER(t_object *, obj, OBJ)
MEMBER(t_bool, fake_hash_entry, FAKE_HASH_ENTRY)
DIABLO_CLASS_END
#undef BASECLASS
#define BASECLASS hash_table_node
#include <diablosupport_hash_node.class.h>
#undef BASECLASS
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
