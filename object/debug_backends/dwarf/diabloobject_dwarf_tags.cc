/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"
#include "diabloobject_dwarf_tags.h"

#include <map>
#include <string>

using namespace std;

struct DwarfTagStruct;
typedef void (*dwarf_tag_handler_fn)(DwarfTagStruct&);

struct DwarfTagStruct {
  DwarfDecodedULEB128 code;
  std::string name;

  dwarf_tag_handler_fn handler;
};

static map<DwarfDecodedULEB128, DwarfTagStruct> tag_handlers;

/* default handler */
static
void
DwarfUnhandledTag(DwarfTagStruct& s)
{
  FATAL(("unsupported DWARF tag: 0x%x (%s)", s.code, s.name.c_str()));
}

/* given a tag code, get its associated data structure */
static
DwarfTagStruct
GetTagInfo(DwarfDecodedULEB128 idx)
{
  if (tag_handlers.find(idx) == tag_handlers.end())
    FATAL(("undefined tag 0x%x", idx));

  return tag_handlers[idx];
}

/* install one tag */
static inline
void
AddTagHandler(int idx, string name, dwarf_tag_handler_fn handler)
{
  tag_handlers[static_cast<DwarfDecodedULEB128>(idx)] = DwarfTagStruct{static_cast<DwarfDecodedULEB128>(idx), name, handler};
}

/* get the string representation of the given tag code */
std::string
TagToString(DwarfDecodedULEB128 tag)
{
  DwarfTagStruct s = GetTagInfo(tag);

  return s.name;
}

/* install all supported tags */
void InitTags()
{
  AddTagHandler(0x01, "DW_TAG_array_type", DwarfUnhandledTag);
  AddTagHandler(0x02, "DW_TAG_class_type", DwarfUnhandledTag);
  AddTagHandler(0x03, "DW_TAG_entry_point", DwarfUnhandledTag);
  AddTagHandler(0x04, "DW_TAG_enumeration_type", DwarfUnhandledTag);
  AddTagHandler(0x05, "DW_TAG_formal_parameter", DwarfUnhandledTag);
  AddTagHandler(0x08, "DW_TAG_imported_declaration", DwarfUnhandledTag);
  AddTagHandler(0x0a, "DW_TAG_label", DwarfUnhandledTag);
  AddTagHandler(0x0b, "DW_TAG_lexical_block", DwarfUnhandledTag);
  AddTagHandler(0x0d, "DW_TAG_member", DwarfUnhandledTag);
  AddTagHandler(0x0f, "DW_TAG_pointer_type", DwarfUnhandledTag);
  AddTagHandler(0x10, "DW_TAG_reference_type", DwarfUnhandledTag);
  AddTagHandler(0x11, "DW_TAG_compile_unit", DwarfUnhandledTag);
  AddTagHandler(0x12, "DW_TAG_string_type", DwarfUnhandledTag);
  AddTagHandler(0x13, "DW_TAG_structure_type", DwarfUnhandledTag);
  AddTagHandler(0x15, "DW_TAG_subroutine_type", DwarfUnhandledTag);
  AddTagHandler(0x16, "DW_TAG_typedef", DwarfUnhandledTag);
  AddTagHandler(0x17, "DW_TAG_union_type", DwarfUnhandledTag);
  AddTagHandler(0x18, "DW_TAG_unspecified_parameters", DwarfUnhandledTag);
  AddTagHandler(0x19, "DW_TAG_variant", DwarfUnhandledTag);
  AddTagHandler(0x1a, "DW_TAG_common_block", DwarfUnhandledTag);
  AddTagHandler(0x1b, "DW_TAG_common_inclusion", DwarfUnhandledTag);
  AddTagHandler(0x1c, "DW_TAG_inheritance", DwarfUnhandledTag);
  AddTagHandler(0x1d, "DW_TAG_inlined_subroutine", DwarfUnhandledTag);
  AddTagHandler(0x1e, "DW_TAG_module", DwarfUnhandledTag);
  AddTagHandler(0x1f, "DW_TAG_ptr_to_member_type", DwarfUnhandledTag);
  AddTagHandler(0x20, "DW_TAG_set_type", DwarfUnhandledTag);
  AddTagHandler(0x21, "DW_TAG_subrange_type", DwarfUnhandledTag);
  AddTagHandler(0x22, "DW_TAG_with_stmt", DwarfUnhandledTag);
  AddTagHandler(0x23, "DW_TAG_access_declaration", DwarfUnhandledTag);
  AddTagHandler(0x24, "DW_TAG_base_type", DwarfUnhandledTag);
  AddTagHandler(0x25, "DW_TAG_catch_block", DwarfUnhandledTag);
  AddTagHandler(0x26, "DW_TAG_const_type", DwarfUnhandledTag);
  AddTagHandler(0x27, "DW_TAG_constant", DwarfUnhandledTag);
  AddTagHandler(0x28, "DW_TAG_enumerator", DwarfUnhandledTag);
  AddTagHandler(0x29, "DW_TAG_file_type", DwarfUnhandledTag);
  AddTagHandler(0x2a, "DW_TAG_friend", DwarfUnhandledTag);
  AddTagHandler(0x2b, "DW_TAG_namelist", DwarfUnhandledTag);
  AddTagHandler(0x2c, "DW_TAG_namelist_item", DwarfUnhandledTag);
  AddTagHandler(0x2d, "DW_TAG_packed_type", DwarfUnhandledTag);
  AddTagHandler(0x2e, "DW_TAG_subprogram", DwarfUnhandledTag);
  AddTagHandler(0x2f, "DW_TAG_template_type_parameter", DwarfUnhandledTag);
  AddTagHandler(0x30, "DW_TAG_template_value_parameter", DwarfUnhandledTag);
  AddTagHandler(0x31, "DW_TAG_thrown_type", DwarfUnhandledTag);
  AddTagHandler(0x32, "DW_TAG_try_block", DwarfUnhandledTag);
  AddTagHandler(0x33, "DW_TAG_variant_part", DwarfUnhandledTag);
  AddTagHandler(0x34, "DW_TAG_variable", DwarfUnhandledTag);
  AddTagHandler(0x35, "DW_TAG_volatile_type", DwarfUnhandledTag);
  AddTagHandler(0x36, "DW_TAG_dwarf_procedure", DwarfUnhandledTag);
  AddTagHandler(0x37, "DW_TAG_restrict_type", DwarfUnhandledTag);
  AddTagHandler(0x38, "DW_TAG_interface_type", DwarfUnhandledTag);
  AddTagHandler(0x39, "DW_TAG_namespace", DwarfUnhandledTag);
  AddTagHandler(0x3a, "DW_TAG_imported_module", DwarfUnhandledTag);
  AddTagHandler(0x3b, "DW_TAG_unspecified_type", DwarfUnhandledTag);
  AddTagHandler(0x3c, "DW_TAG_partial_unit", DwarfUnhandledTag);
  AddTagHandler(0x3d, "DW_TAG_imported_unit", DwarfUnhandledTag);
  AddTagHandler(0x3f, "DW_TAG_condition", DwarfUnhandledTag);
  AddTagHandler(0x40, "DW_TAG_shared_type", DwarfUnhandledTag);
  AddTagHandler(0x41, "DW_TAG_type_unit", DwarfUnhandledTag);
  AddTagHandler(0x42, "DW_TAG_rvalue_reference_type", DwarfUnhandledTag);
  AddTagHandler(0x43, "DW_TAG_template_alias", DwarfUnhandledTag);
  AddTagHandler(0x4080, "DW_TAG_lo_user", DwarfUnhandledTag);
  AddTagHandler(0xffff, "DW_TAG_hi_user", DwarfUnhandledTag);

  /* GNU */
  AddTagHandler(0x4106, "DW_TAG_GNU_template_template_param", DwarfUnhandledTag);
  AddTagHandler(0x4107, "DW_TAG_GNU_template_parameter_pack", DwarfUnhandledTag);
  AddTagHandler(0x4108, "DW_TAG_GNU_formal_parameter_pack", DwarfUnhandledTag);
  AddTagHandler(0x4109, "DW_TAG_GNU_call_site", DwarfUnhandledTag);
  AddTagHandler(0x410a, "DW_TAG_GNU_call_site_parameter", DwarfUnhandledTag);
}
