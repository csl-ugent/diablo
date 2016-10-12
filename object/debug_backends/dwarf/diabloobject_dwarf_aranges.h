/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_ARANGES_H
#define DIABLOOBJECT_DWARF_ARANGES_H

#include "diabloobject_dwarf.h"

#include <vector>

struct DwarfCompilationUnitHeader;

struct AddressRangeDescriptor {
  t_uint32 segment;
  t_address start;
  t_address length;
};

struct AddressRangeTableEntry {
  t_uint64 unit_length;
  bool is_64bit;

  t_uint16 version;
  t_address debug_info_offset;
  t_uint8 address_size;
  t_uint8 segment_size;

  std::vector<AddressRangeDescriptor *> descriptors;

  DwarfCompilationUnitHeader *cu_header;

  ~AddressRangeTableEntry() {
    for (auto d : descriptors)
      delete d;

    delete cu_header;
  }
};

typedef std::vector<AddressRangeTableEntry *> AddressRangeTable;

AddressRangeTable *
ReadAddressRangeTable(DwarfSections *dwarf_sections);

void
AddressRangeTableFree(AddressRangeTable *table);

#endif /* DIABLOOBJECT_DWARF_ARANGES_H */
