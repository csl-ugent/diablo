/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_UNITS_H
#define DIABLOOBJECT_DWARF_UNITS_H

#include "diabloobject_dwarf.h"

/* for DwarfLineInfoMatrix typedef */
#include "diabloobject_dwarf_line.h"

/* for DwarfAbbrevDeclarationList typedef */
#include "diabloobject_dwarf_abbrev_table.h"

#include "diabloobject_dwarf_generic.h"

#include <vector>
#include <string>

struct DwarfLineNumberProgramHeader;

struct DwarfCompilationUnitHeader
        : public DwarfDebugInformationEntry {
  t_address offset;

  bool is_64bit;
  t_uint64 unit_length;

  t_uint16 version;
  t_address debug_abbrev_offset;
  t_uint8  address_size;
  t_uint32 header_size;

  /* The abbreviation table.
   * Contains the associated declarations and attributes. */
  DwarfAbbrevDeclarationList *abbrev_table;

  /* The line information matrix.
   * one line = one generated instruction
   * addresses can be skipped if file, line, column and discriminator are equal */
  std::vector<DwarfLineInfoMatrix *> line_info_matrices;

  /* 'children' member inherited from DwarfDebugInformationEntry
   * are of type DwarfAbbrevEntry, the first one containing the CU-level attributes. */

  ~DwarfCompilationUnitHeader() {
    for (auto line_info_matrix : line_info_matrices)
      delete line_info_matrix;
  }

  std::string ToString();

  /* attributes */
  DwarfLanguageCode language;
};

struct DwarfTypeUnitHeader {
  bool is_64bit;
  t_uint64 unit_length;

  t_uint16 version;
  t_address debug_abbrev_offset;
  t_uint8  address_size;

  t_uint64 type_signature;
  t_address type_offset;
};

DwarfCompilationUnitHeader *
ReadCompilationUnit(t_section *sec, t_address offset, t_uint32& header_size);

#endif /* DIABLOOBJECT_DWARF_UNITS_H */
