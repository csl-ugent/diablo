/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_ABBREV_TABLE_H
#define DIABLOOBJECT_DWARF_ABBREV_TABLE_H

#include "diabloobject_dwarf.h"

#include "diabloobject_dwarf_attributes.h"
#include "diabloobject_dwarf_tags.h"

#include <map>
#include <string>
#include <vector>

struct DwarfDebugInformationEntry;

struct DwarfAbbrevDeclaration {
  DwarfDecodedULEB128 code;
  DwarfTagCode tag;
  bool has_children;

  std::vector<DwarfAttributeSpec *> attributes;

  ~DwarfAbbrevDeclaration() {
    for (DwarfAttributeSpec *attr : attributes)
      delete attr;
  }
};

/* these are actually added as DwarfDebugInformationEntry instances
 * in the children list of the CU-header. */
struct DwarfAbbrevTableEntry
        : public DwarfDebugInformationEntry {
  DwarfAbbrevDeclaration *declaration;
  std::vector<DwarfAbstractParsedAttribute *> attributes;
  t_address offset;

  ~DwarfAbbrevTableEntry() {
    /* the declaration has already been freed */

    for (DwarfAbstractParsedAttribute *attr : attributes)
      delete attr;
  }

  std::string ToString();
};

typedef std::vector<DwarfAbbrevDeclaration *> DwarfAbbrevDeclarationList;

DwarfAbbrevDeclarationList *
ReadAbbreviationDeclarationList(t_section *sec, t_address offset);

void
ParseAbbreviationTable(DwarfCompilationUnitHeader *cu_header, DwarfSections *dwarf_sections, t_address offset);

void
PrintAbbreviationTable(DwarfCompilationUnitHeader *cu_header);

DwarfAbstractParsedAttribute *
LookupAttributeForAbbreviationTableEntry(DwarfAbbrevTableEntry *entry, DwarfAttributeCode attr_code);

std::vector<DwarfAbstractParsedAttribute *>
LookupAllAttributeForAbbreviationTableEntryArray(DwarfAbbrevTableEntry *entry, DwarfAttributeCode attr_code);

void
AbbreviationTableFreeDeclarationLists();

template<typename T>
T GetDwarfAttribute(DwarfAbbrevTableEntry *entry, DwarfAttributeCode attrib_code) {
  DwarfAbstractParsedAttribute *_attr = LookupAttributeForAbbreviationTableEntry(entry, attrib_code);
  if (!_attr)
    return NULL;

  return static_cast<T>(_attr->decoded);
}

t_address ParseAttributes(DwarfAbbrevTableEntry *entry, t_address offset, std::vector<DwarfAbstractAttribute *>& to_process, DwarfCompilationUnitHeader *cu_header, DwarfSections *dwarf_sections);
void PostProcessParsedAttributes(std::vector<DwarfAbstractAttribute *>& to_process, std::map<t_address, DwarfAbbrevTableEntry *>& table_entries);

#endif /* DIABLOOBJECT_DWARF_ABBREV_TABLE_H */
