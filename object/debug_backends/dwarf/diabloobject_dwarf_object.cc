/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

#include <map>
#include <vector>
#include <iostream>

using namespace std;

/* read in the Dwarf information, starting from the address range table */
DwarfInfo
ObjectGetDwarfInfo(t_object *obj, t_dwarf_sections *dwarf_sections)
{
  DwarfInfo ret;

  dwarf_sections->aranges_section = SectionGetFromObjectByName(obj, ARANGES_SECTION);
  dwarf_sections->frame_section = SectionGetFromObjectByName(obj, FRAME_SECTION);
  dwarf_sections->abbrev_section = SectionGetFromObjectByName(obj, ABBREV_SECTION);
  dwarf_sections->str_section = SectionGetFromObjectByName(obj, STR_SECTION);
  dwarf_sections->loc_section = SectionGetFromObjectByName(obj, LOC_SECTION);
  dwarf_sections->ranges_section = SectionGetFromObjectByName(obj, RANGES_SECTION);
  dwarf_sections->macinfo_section = SectionGetFromObjectByName(obj, MACINFO_SECTION);
  dwarf_sections->line_section = SectionGetFromObjectByName(obj, LINE_SECTION);
  dwarf_sections->pubnames_section = SectionGetFromObjectByName(obj, PUBNAMES_SECTION);
  dwarf_sections->pubtypes_section = SectionGetFromObjectByName(obj, PUBTYPES_SECTION);
  dwarf_sections->info_section = SectionGetFromObjectByName(obj, INFO_SECTION);
  dwarf_sections->types_section = SectionGetFromObjectByName(obj, TYPES_SECTION);

  if(dwarf_sections->info_section)
    VERBOSE(DWARF_VERBOSITY, ("Found %s with size @G", SECTION_NAME(dwarf_sections->info_section), SECTION_CSIZE(dwarf_sections->info_section)));

  DwarfInit();
  ret.arange_table = ReadAddressRangeTable(dwarf_sections);

  return ret;
}

/* As the debug information is no longer correct after the binary has been rewritten,
 * remove the debug sections from the rewritten binary. */
void
ObjectFreeDebugSections (t_object * obj)
{
  t_section * section;
  t_uint32 i;

  if (OBJECT_NDEBUGS(obj))
  {
    OBJECT_FOREACH_DEBUG_SECTION(obj, section, i)
    {
      SectionFree (section);
    }
    OBJECT_SET_NDEBUGS(obj, 0);
    Free (OBJECT_DEBUG(obj));
    OBJECT_SET_DEBUG(obj, NULL);
  }
}
