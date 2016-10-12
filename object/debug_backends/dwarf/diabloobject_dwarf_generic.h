/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_GENERIC_H
#define DIABLOOBJECT_DWARF_GENERIC_H

#include <vector>

enum class DwarfChildEncoding {
  DW_CHILDREN_no = 0x00,
  DW_CHILDREN_yes = 0x01
};

struct DwarfDebugInformationEntry {
  t_address offset;

  std::vector<DwarfDebugInformationEntry *> children;

  virtual ~DwarfDebugInformationEntry() {
    /* children */
    for (auto e : children)
      delete e;
  }
};

#endif /* DIABLOOBJECT_DWARF_GENERIC_H */
