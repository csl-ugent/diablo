/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

#include <vector>

using namespace std;

static
AddressRangeTableEntry *
ReadAddressRangeTableEntry(t_section *sec, t_address offset, t_uint32& size)
{
  AddressRangeTableEntry *ret = new AddressRangeTableEntry();
  t_uint32 sz;

#if DEBUG_DWARF
  VERBOSE(DWARF_VERBOSITY, ("Address range @ offset 0x%x", offset));
#endif

  size = 0;

  ret->unit_length = DwarfReadInitialLength(sec, offset, sz);
  ret->is_64bit = (sz == 12);
  offset = AddressAddUint32(offset, sz);
  size += sz;

  /* version */
  ret->version = SectionGetData16(sec, offset);
  offset = AddressAddUint32(offset, 2);
  size += 2;

  /* section offset */
  if (ret->is_64bit)
  {
    ASSERT(sizeof(t_address) == 8, ("64-bit DWARF information found, but Diablo is compiled for 32-bit!"));

    ret->debug_info_offset = static_cast<t_address>(SectionGetData64(sec, offset));
    offset = AddressAddUint32(offset, 8);
    size += 8;
  }
  else
  {
    ret->debug_info_offset = static_cast<t_address>(SectionGetData32(sec, offset));
    offset = AddressAddUint32(offset, 4);
    size += 4;
  }

  /* address size */
  ret->address_size = SectionGetData8(sec, offset);
  offset = AddressAddUint32(offset, 1);
  size++;

  /* segment size */
  ret->segment_size = SectionGetData8(sec, offset);
  ASSERT(ret->segment_size == 0, ("segmented system memory not tested"));
  offset = AddressAddUint32(offset, 1);
  size++;

#if DEBUG_DWARF
  VERBOSE(DWARF_VERBOSITY, (" Length %d (%d)", ret->unit_length, ret->is_64bit));
  VERBOSE(DWARF_VERBOSITY, (" Version %d", ret->version));
  VERBOSE(DWARF_VERBOSITY, (" Info offset 0x%x", ret->debug_info_offset));
  VERBOSE(DWARF_VERBOSITY, (" Pointer Size %d", ret->address_size));
  VERBOSE(DWARF_VERBOSITY, (" Segment size %d", ret->segment_size));
#endif

  /* alignment */
  t_uint32 struct_size = 2 * ret->address_size + ret->segment_size;
  offset = AddressAddUint32(offset, struct_size - (offset % struct_size));

  /* address range descriptors */
  while (true) {
    t_address start;
    t_address length;
    t_address segment = 0;
    AddressRangeDescriptor *desc;

    /* skip if segment_size is zero */
    if (ret->segment_size > 0)
    {
      segment = DwarfReadAddress(ret->segment_size, sec, offset);
      offset = AddressAddUint32(offset, ret->segment_size);
    }

    start = DwarfReadAddress(ret->address_size, sec, offset);
    offset = AddressAddUint32(offset, ret->address_size);

    length = DwarfReadAddress(ret->address_size, sec, offset);
    offset = AddressAddUint32(offset, ret->address_size);

    /* end-of-list entry */
    if (start == 0
        && length == 0
        && segment == 0)
      break;

    /* useful entry */
    desc = new AddressRangeDescriptor();
    desc->start = start;
    desc->length = length;
    desc->segment = 0;

    ret->descriptors.push_back(desc);
  }

  return ret;
}

/* parse one table entry */
static
void
ParseAddressRangeTableEntry(AddressRangeTableEntry *entry, DwarfSections *dwarf_sections)
{
  t_address offset = entry->debug_info_offset;
  t_uint32 header_size = 0;

  /* read compilation unit */
  entry->cu_header = ReadCompilationUnit(dwarf_sections->info_section, offset, header_size);

  /* read in the associated abbreviation table and parse it */
  entry->cu_header->abbrev_table = ReadAbbreviationDeclarationList(dwarf_sections->abbrev_section, static_cast<t_address>(entry->cu_header->debug_abbrev_offset));
  ParseAbbreviationTable(entry->cu_header, dwarf_sections, AddressAddUint32(offset, header_size));

  ASSERT(entry->cu_header->children.size() == 1, ("unexpected %d", entry->cu_header->children.size()));

  /* collect attribute values */
  auto language_attr = GetDwarfAttribute<DwarfConstantAttribute *>(static_cast<DwarfAbbrevTableEntry *>(entry->cu_header->children[0]), DwarfAttributeCode::DW_AT_language);
  ASSERT(language_attr, ("no language attribute for compilation unit at @G", entry->cu_header->debug_abbrev_offset));
  entry->cu_header->language = static_cast<DwarfLanguageCode>(language_attr->value);
  /* try to convert the read language attribute to a string to see whether or not we recognise it correctly */
  DwarfLanguageCodeToString(entry->cu_header->language);
}

/* Read in an address range table from the .debug_aranges section */
AddressRangeTable *
ReadAddressRangeTable(DwarfSections *dwarf_sections)
{
  AddressRangeTable *ret = new AddressRangeTable();

  /* get the section and an initial offset value for that section */
  t_section *aranges_section = dwarf_sections->aranges_section;

  if (aranges_section)
  {
    t_address offset = AddressNullForSection(aranges_section);

    /* at least some data must be present in the section */
    ASSERT(SECTION_DATA(aranges_section), ("no data found in %s section", SECTION_NAME(aranges_section)));

    /* read in the whole section, consisting of a sequence of table entries */
    while (AddressIsLt(offset, SECTION_CSIZE(aranges_section)))
    {
      t_uint32 sz = 0;
      t_uint64 unit_length = 0;

      /* read in one table entry and parse it */
      AddressRangeTableEntry *new_entry = ReadAddressRangeTableEntry(aranges_section, offset, sz);
      ParseAddressRangeTableEntry(new_entry, dwarf_sections);

      /* offset increment: 'unit_length' does not include the length field itself,
       * so in calculating the offset to the next table entry, this needs to be taken
       * into account. */
      unit_length = new_entry->unit_length;
      unit_length += (new_entry->is_64bit) ? 12 : 4;
      offset = AddressAddUint32(offset, unit_length);

      ret->push_back(new_entry);
    }
  }

  /* possible non-read compilation unit entries,
   * e.g., for the ASPIRE license example compiled with LLVM 3.2 */
  t_address tmp_offset = AddressNew32(0);
  while (tmp_offset < SECTION_CSIZE(dwarf_sections->info_section))
  {
    bool already_done = false;
    AddressRangeTableEntry *entry;

    for (size_t i = 0; i < ret->size(); i++)
    {
      entry = ret->at(i);
      if (AddressIsEq(entry->cu_header->offset, tmp_offset))
      {
        already_done = true;
        break;
      }
    }

    if (already_done)
    {
      tmp_offset = AddressAddUint32(tmp_offset, entry->cu_header->unit_length + 4);
      continue;
    }

    WARNING((".debug_info +@G not read in (because it's not referred to by the .debug_aranges section), creating a dummy .debug_aranges entry for this part", tmp_offset));
    AddressRangeTableEntry *new_entry = new AddressRangeTableEntry();
    new_entry->unit_length = 0;
    new_entry->is_64bit = false;
    new_entry->version = 2;
    new_entry->debug_info_offset = tmp_offset;
    new_entry->address_size = 4;
    new_entry->segment_size = 0;
    ParseAddressRangeTableEntry(new_entry, dwarf_sections);

    ret->push_back(new_entry);
  }

  return ret;
}

void
AddressRangeTableFree(AddressRangeTable *table)
{
  /* free all at once, because every table is created once, but copies may exist */
  AbbreviationTableFreeDeclarationLists();

  for (auto e : *table)
    delete e;

  /* the table itself */
  delete table;
}
