/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

/* read in a list of non-contiguous address ranges */
DwarfRangeList *
ReadAddressRange(DwarfCompilationUnitHeader *cu, t_section *sec, t_address offset)
{
  DwarfRangeList *ret = new DwarfRangeList();

  /* look up base address for this CU */
  DwarfAbstractParsedAttribute *attr = LookupAttributeForAbbreviationTableEntry(static_cast<DwarfAbbrevTableEntry *>(cu->children[0]), DwarfAttributeCode::DW_AT_low_pc);
  ASSERT(attr, ("could not find DW_AT_low_pc for cu"));

  DwarfAddressAttribute *at_base_address = static_cast<DwarfAddressAttribute *>(attr->decoded);
  t_address base_address = static_cast<t_address>(at_base_address->value);

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
