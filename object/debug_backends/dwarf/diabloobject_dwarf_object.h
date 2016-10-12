/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOOBJECT_DWARF_OBJECT_H
#define DIABLOOBJECT_DWARF_OBJECT_H

#include "diabloobject_dwarf.h"

struct DwarfInfo {
  /* As this file is included in C-files, and the DwarfInfo struct is needed therein,
   * instead of defining the AddressRangeTable member as-is, define it as a void-pointer.
   * This is because AddressRangeTable is actually a typedef for a vector, which is of course
   * not defined in C. */
  void *arange_table;
};
typedef struct DwarfInfo t_dwarf_info;

EXTERN
t_dwarf_info
ObjectGetDwarfInfo(t_object * obj, t_dwarf_sections * dwarf_sections);

EXTERN
void
ObjectFreeDebugSections (t_object * object);

#endif /* DIABLOOBJECT_DWARF_OBJECT_H */
