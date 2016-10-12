/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_RANGES_H
#define DIABLOOBJECT_DWARF_RANGES_H

#include "diabloobject_dwarf.h"

#include <vector>

struct DwarfCompilationUnitHeader;

struct DwarfRangeListEntry
{
  t_address first;
  t_address second;
};

typedef std::vector<DwarfRangeListEntry *> DwarfRangeList;

DwarfRangeList *
ReadAddressRange(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset);

#endif /* DIABLOOBJECT_DWARF_RANGES_H */
