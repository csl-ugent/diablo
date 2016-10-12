/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* Hash table support
 *
 * Interface: TODO
 */

#include <diablosupport.h>
/* Hash Defines {{{ */
#ifndef DIABLOSUPPORT_HASH_DEFINES
#define DIABLOSUPPORT_HASH_DEFINES
#define T_HASH_TABLE_NODE(x) ((t_hash_table_node *) (x))
#define T_HASH_TABLE(x) ((t_hash_table *) (x))
struct _t_hash_table;
typedef t_uint32 (*t_hash_func) (const void *, const struct _t_hash_table *);
typedef t_uint32 (*renamed_t_hash_func) (const void *, const struct _t_hash_table *);
typedef t_int32 (*t_hash_cmp) (const void *, const void *);
typedef t_int32 (*renamed_t_hash_cmp) (const void *, const void *);
typedef void (*t_hash_node_free) (const void *, void *);
typedef void (*renamed_t_hash_node_free) (const void *, void *);
#endif /* }}} Hash Defines */

#ifdef DIABLOSUPPORT_FUNCTIONS
/* Hash Functions {{{ */
#ifndef DIABLOSUPPORT_HASH_FUNCTIONS
#define DIABLOSUPPORT_HASH_FUNCTIONS
t_bool HashTableIsPresent(const t_hash_table* table, const void* vnode);
void HashTableSetKeyForNode (t_hash_table *, void *, void *);
void HashTableFree (const t_hash_table *);
void HashTableWalk (t_hash_table *, void (*)(void *, void *), void *);
void HashTableStats (const t_hash_table *);
#endif /* }}} Hash Functions */
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
