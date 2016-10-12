/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

/* read in one compilation unit header */
DwarfCompilationUnitHeader *
ReadCompilationUnit(t_section *sec, t_address offset, t_uint32& header_size)
{
  DwarfCompilationUnitHeader *ret = new DwarfCompilationUnitHeader();
  t_uint32 sz = 0;

#if DEBUG_DWARF
  VERBOSE(DWARF_VERBOSITY, ("Compilation unit @ offset 0x%x", offset));
#endif

  header_size = 0;

  ret->offset = offset;

  /* unit length */
  ret->unit_length = DwarfReadInitialLength(sec, offset, sz);
  ret->is_64bit = (sz == 12);
  offset = AddressAddUint32(offset, sz);
  header_size += sz;

  /* version */
  ret->version = SectionGetData16(sec, offset);
  offset = AddressAddUint32(offset, 2);
  header_size += 2;

  /* section offset */
  if (ret->is_64bit)
  {
    ASSERT(sizeof(t_address) == 8, ("64-bit DWARF information found, but Diablo is compiled for 32-bit!"));

    ret->debug_abbrev_offset = static_cast<t_address>(SectionGetData64(sec, offset));
    offset = AddressAddUint32(offset, 8);
    header_size += 8;
  }
  else
  {
    ret->debug_abbrev_offset = static_cast<t_address>(SectionGetData32(sec, offset));
    offset = AddressAddUint32(offset, 4);
    header_size += 4;
  }

  /* address size */
  ret->address_size = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);
  header_size++;

  ret->header_size = header_size;

#if DEBUG_DWARF
  VERBOSE(DWARF_VERBOSITY, (" Length 0x%x (%d)", ret->unit_length, ret->is_64bit));
  VERBOSE(DWARF_VERBOSITY, (" Version %d", ret->version));
  VERBOSE(DWARF_VERBOSITY, (" Abbrev Offset 0x%x", ret->debug_abbrev_offset));
  VERBOSE(DWARF_VERBOSITY, (" Pointer Size %d", ret->address_size));
#endif

  return ret;
}
