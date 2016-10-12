/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_HELPERS_H
#define DIABLOOBJECT_DWARF_HELPERS_H

#include <string>
#include <vector>

typedef std::vector<t_uint8> DwarfEncodedULEB128;
typedef t_uint64 DwarfDecodedULEB128;

typedef std::vector<t_uint8>	 DwarfEncodedSLEB128;
typedef t_int64  DwarfDecodedSLEB128;

DwarfEncodedULEB128
DwarfReadULEB128FromSection(t_section *section, t_address offset, t_uint32& n_bytes);

DwarfEncodedSLEB128
DwarfReadSLEB128FromSection(t_section *section, t_address offset, t_uint32& n_bytes);

DwarfDecodedULEB128
DwarfDecodeULEB128(DwarfEncodedULEB128 encoded);

DwarfDecodedSLEB128
DwarfDecodeSLEB128(DwarfEncodedSLEB128 encoded);

t_uint64
DwarfReadInitialLength(t_section *section, t_address offset, t_uint32& size);

t_address
DwarfReadAddress(t_uint8 address_size, t_section *section, t_address offset);

std::string
ReadStringFromSection(t_section *section, t_address offset);

#endif
