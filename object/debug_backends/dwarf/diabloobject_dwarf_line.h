/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_LINE_H
#define DIABLOOBJECT_DWARF_LINE_H

#include "diabloobject_dwarf.h"

#include <vector>
#include <string>

struct DwarfCompilationUnitHeader;

struct DwarfFileEntry {
  std::string *name;

  DwarfDecodedULEB128 last_modified;
  DwarfDecodedULEB128 size;
  DwarfDecodedULEB128 directory_index;

  ~DwarfFileEntry() {
    delete name;
  }
};

struct DwarfLineNumberProgramHeader {
  bool is_64bit;
  t_uint64 unit_length;

  t_uint16 version;
  t_uint64 header_length;
  t_uint8 minimum_instruction_length;
  t_uint8 maximum_operations_per_instruction;
  t_uint8 default_is_stmt;
  t_int8 line_base;
  t_uint8 line_range;
  t_uint8 opcode_base;
  t_uint8 *standard_opcode_lengths;

  std::vector<std::string *> include_directories;
  std::vector<DwarfFileEntry *> file_names;

  ~DwarfLineNumberProgramHeader() {
    for (auto e : include_directories)
      delete e;

    for (auto e : file_names)
      delete e;

    delete[] standard_opcode_lengths;
  }
};

/* Represents one row in the line information matrix. */
struct DwarfLineInfoMatrixRow {
  t_address address;

  t_uint32 file;
  t_uint32 line;
  t_uint32 column;

  t_uint32 discriminator;
};
typedef std::vector<DwarfLineInfoMatrixRow> DwarfLineInfoMatrix;

DwarfLineNumberProgramHeader *
ReadLineNumberProgram(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset);

#endif /* DIABLOOBJECT_DWARF_LINE_H */
