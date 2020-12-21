/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

/* read in a list of non-contiguous address ranges */
DwarfRangeList *
ReadAddressRange(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset)
{
  DwarfRangeList *ret = new DwarfRangeList();

  /* look up base address for this CU */
  t_address base_address = AddressNew32(0);

  DwarfAbstractParsedAttribute *attr = LookupAttributeForAbbreviationTableEntry(static_cast<DwarfAbbrevTableEntry *>(cu->children[0]), DwarfAttributeCode::DW_AT_low_pc);
  if (attr) {
    /* DW_AT_low_pc contains the base address */
    DwarfAddressAttribute *at_base_address = static_cast<DwarfAddressAttribute *>(attr->decoded);
    base_address = static_cast<t_address>(at_base_address->value);
  }
  else {
    /* first entry in range list should have base address */
    t_address largest_offset = DwarfReadAddress(cu->address_size, sec, offset);
    offset = AddressAddUint32(offset, cu->address_size);
    ASSERT(G_T_UINT32(largest_offset) == 0xffffffff, ("expected max int32 value, got @G", largest_offset));

    base_address = DwarfReadAddress(cu->address_size, sec, offset);
    offset = AddressAddUint32(offset, cu->address_size);
  }

  while (true) {
    t_uint32 sz;
    t_address first;
    t_address second;
    DwarfRangeListEntry *new_entry;

    /* beginning of range */
    sz = 0;
    first = DwarfReadAddress(cu->address_size, sec, offset);
    offset = AddressAddUint32(offset, cu->address_size);

    /* end of range */
    sz = 0;
    second = DwarfReadAddress(cu->address_size, sec, offset);
    offset = AddressAddUint32(offset, cu->address_size);

    /* end of list */
    if (AddressIsEq(first, 0)
        && AddressIsEq(second, 0))
      break;

    /* useful entry, save it */
    new_entry = new DwarfRangeListEntry();
    new_entry->first = AddressAdd(first, base_address);
    new_entry->second = AddressAdd(second, base_address);
    ret->push_back(new_entry);
  }

  return ret;
}
