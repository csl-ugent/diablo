/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

/* only execute DwarfInit once */
static bool is_init = false;

/* intiialise maps to map indices on strings and handlers */
void
DwarfInit()
{
  if (is_init)
    return;

  /* diabloobject_dwarf_forms.cc */
  InitForms();

  /* diabloobject_dwarf_tags.cc */
  InitTags();

  /* diabloobject_dwarf_attributes.cc */
  InitAttributes();

  is_init = true;
}

/* clean up allocated memory */
void
DwarfSectionsFreeAll (DwarfInfo * dwarf_info)
{
  AddressRangeTable *arange_table = static_cast<AddressRangeTable *>(dwarf_info->arange_table);

  AddressRangeTableFree(arange_table);
  StringsFreeAllFormCache();
}
