/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosupport.h>
#include <stdio.h>
#include <unistd.h>


/* TYPEDEFs in diablosupport_ptr_array.h */
#ifndef DIABLOSUPPORT_PTR_ARRAY_TYPEDEFS
#define DIABLOSUPPORT_PTR_ARRAY_TYPEDEFS
typedef struct _t_ptr_array t_ptr_array;
#endif


/* DEFINEs in diablosupport_ptr_array.h */
#ifndef DIABLOSUPPORT_PTR_ARRAY_DEFINES
#define DIABLOSUPPORT_PTR_ARRAY_DEFINES
#endif


#ifdef DIABLOSUPPORT_TYPES
/* types (STRUCTs) in diablosupport_ptr_array.h */
#ifndef DIABLOSUPPORT_PTR_ARRAY_TYPES
#define DIABLOSUPPORT_PTR_ARRAY_TYPES
struct _t_ptr_array
{
  void **arr;
  int length, count;
  t_bool trypreventdup;
};
#endif
#endif


#ifdef DIABLOSUPPORT_FUNCTIONS
/* functions in diablosupport_ptr_array.h */
#ifndef DIABLOSUPPORT_PTR_ARRAY_FUNCTIONS
#define DIABLOSUPPORT_PTR_ARRAY_FUNCTIONS
void PtrArrayInit(t_ptr_array *ea, t_bool trypreventdup);
int PtrArrayCount(const t_ptr_array *ea);
t_bool PtrArrayIsEmpty(const t_ptr_array *ea);
int PtrArrayLength(const t_ptr_array *ea);
void PtrArrayAdd(t_ptr_array *ea, void* edge);
void PtrArrayInsert(t_ptr_array *ea, void* edge, int pos);
void PtrArrayRemove(t_ptr_array *ea, int index, t_bool freeit);
void PtrArrayRemoveLast(t_ptr_array *ea, t_bool freeit);
int PtrArrayFind(const t_ptr_array *ea, const void *item);
void PtrArrayRemoveContent(t_ptr_array *ea, void *item, t_bool freeit);
void* PtrArrayGet(const t_ptr_array *ea, int index);
void* PtrArrayGetFirst(const t_ptr_array *ea);
void* PtrArrayGetLast(const t_ptr_array *ea);
void PtrArrayFini(const t_ptr_array *ea, t_bool free_elements);
#endif
#endif
