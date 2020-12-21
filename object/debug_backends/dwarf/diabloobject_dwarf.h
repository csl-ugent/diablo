/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_H
#define DIABLOOBJECT_DWARF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <diabloobject.h>
#include <diablosupport.h>

#ifdef __cplusplus
}
#endif

#define DWARF_VERBOSITY 0
#define DEBUG_DWARF 0

/* names of the different dwarf sections */
#define ARANGES_SECTION 	".debug_aranges"
#define FRAME_SECTION 		".debug_frame"
#define ABBREV_SECTION 		".debug_abbrev"
#define STR_SECTION 			".debug_str"
#define LOC_SECTION 			".debug_loc"
#define RANGES_SECTION 		".debug_ranges"
#define MACINFO_SECTION 	".debug_macinfo"
#define LINE_SECTION 			".debug_line"
#define PUBNAMES_SECTION 	".debug_pubnames"
#define PUBTYPES_SECTION 	".debug_pubtypes"
#define INFO_SECTION 			".debug_info"
#define TYPES_SECTION 		".debug_types"

struct DwarfSections {
  t_section *aranges_section;
  t_section *frame_section;
  t_section *abbrev_section;
  t_section *str_section;
  t_section *loc_section;
  t_section *ranges_section;
  t_section *macinfo_section;
  t_section *line_section;
  t_section *pubnames_section;
  t_section *pubtypes_section;
  t_section *info_section;
  t_section *types_section;
};

typedef struct DwarfSections t_dwarf_sections;

#ifdef __cplusplus
/* export C++ functions to C */
#define EXTERN extern "C"

class DwarfAbbrevTableEntry;

#include "diabloobject_dwarf_generic.h"
#include "diabloobject_dwarf_helpers.h"
#include "diabloobject_dwarf_units.h"
#include "diabloobject_dwarf_forms.h"
#include "diabloobject_dwarf_tags.h"
#include "diabloobject_dwarf_attributes.h"
#include "diabloobject_dwarf_abbrev_table.h"
#include "diabloobject_dwarf_line.h"
#include "diabloobject_dwarf_ranges.h"
#include "diabloobject_dwarf_aranges.h"

#else
#define EXTERN
#endif

#include "diabloobject_dwarf_object.h"

EXTERN
void
DwarfInit();

EXTERN
void
DwarfSectionsFreeAll (t_dwarf_info * dwarf_info);

#endif /* DIABLOOBJECT_DWARF_H */
