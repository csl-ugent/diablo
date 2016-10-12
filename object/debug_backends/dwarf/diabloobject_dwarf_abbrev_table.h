/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_ABBREV_TABLE_H
#define DIABLOOBJECT_DWARF_ABBREV_TABLE_H

#include "diabloobject_dwarf.h"

#include "diabloobject_dwarf_attributes.h"

#include <vector>

struct DwarfDebugInformationEntry;

struct DwarfAbbrevDeclaration {
  DwarfDecodedULEB128 code;
  DwarfDecodedULEB128 tag;
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

  ~DwarfAbbrevTableEntry() {
    /* the declaration has already been freed */

    for (DwarfAbstractParsedAttribute *attr : attributes)
      delete attr;
  }
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

#endif /* DIABLOOBJECT_DWARF_ABBREV_TABLE_H */
