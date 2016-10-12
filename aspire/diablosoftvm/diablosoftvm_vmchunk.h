/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOSOFTVM_VMCHUNK_H
#define DIABLOSOFTVM_VMCHUNK_H

#include "diablosoftvm.h"

#include <map>
#include <vector>

typedef std::map<t_cfg_edge *, t_cfg_edge *> t_edge_to_edge_map;
typedef std::pair<t_cfg_edge *, t_cfg_edge *> t_edge_to_edge_map_entry;

/* mapping FROM instructions on their TO relocatables */
typedef std::map<t_arm_ins *, std::vector<t_reloc *>> t_ins_to_reloc_map;
typedef std::pair<t_arm_ins *, std::vector<t_reloc *>> t_ins_to_reloc_map_entry;

t_vmchunk * VmChunkNew();
void VmChunkFree(t_vmchunk *chunk);
void VmChunkPrint(t_vmchunk *chunk);
int VmChunkCountInstructions(t_vmchunk *chunk);

#define PTRARRAY_FOREACH(ptrarray, type, ret) \
	for (int i__ = 0; (i__ < PtrArrayCount(ptrarray)) ? (ret = static_cast<type *>(PtrArrayGet(ptrarray, i__)), TRUE) : FALSE; i__++)

#define VMCHUNK_FOREACH_BBL(chunk, bbl) PTRARRAY_FOREACH(VMCHUNK_BBLS(chunk), t_bbl, bbl)
#define VMCHUNK_FOREACH_RELOCATABLE(chunk, r) PTRARRAY_FOREACH(VMCHUNK_RELOCATABLES(chunk), t_relocatable, r)

#endif
