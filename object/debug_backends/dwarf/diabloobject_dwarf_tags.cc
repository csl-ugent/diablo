/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"
#include "diabloobject_dwarf_tags.h"

#include <map>
#include <string>

using namespace std;

struct DwarfTagStruct;
typedef void (*dwarf_tag_handler_fn)(DwarfTagStruct&);

struct DwarfTagStruct {
  DwarfTagCode code;
  std::string name;

  dwarf_tag_handler_fn handler;
};

static map<DwarfTagCode, DwarfTagStruct> tag_handlers;

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
GetTagInfo(DwarfTagCode idx)
{
  if (tag_handlers.find(idx) == tag_handlers.end())
    FATAL(("undefined tag 0x%x", idx));

  return tag_handlers[idx];
}

/* install one tag */
static inline
void
AddTagHandler(DwarfTagCode idx, string name, dwarf_tag_handler_fn handler)
{
  tag_handlers[idx] = DwarfTagStruct{idx, name, handler};
}

/* get the string representation of the given tag code */
std::string
TagToString(DwarfTagCode tag)
{
  DwarfTagStruct s = GetTagInfo(tag);

  return s.name;
}

/* install all supported tags */
void InitTags()
{
  AddTagHandler(DwarfTagCode::DW_TAG_array_type, "DW_TAG_array_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_class_type, "DW_TAG_class_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_entry_point, "DW_TAG_entry_point", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_enumeration_type, "DW_TAG_enumeration_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_formal_parameter, "DW_TAG_formal_parameter", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_imported_declaration, "DW_TAG_imported_declaration", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_label, "DW_TAG_label", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_lexical_block, "DW_TAG_lexical_block", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_member, "DW_TAG_member", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_pointer_type, "DW_TAG_pointer_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_reference_type, "DW_TAG_reference_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_compile_unit, "DW_TAG_compile_unit", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_string_type, "DW_TAG_string_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_structure_type, "DW_TAG_structure_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_subroutine_type, "DW_TAG_subroutine_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_typedef, "DW_TAG_typedef", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_union_type, "DW_TAG_union_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_unspecified_parameters, "DW_TAG_unspecified_parameters", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_variant, "DW_TAG_variant", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_common_block, "DW_TAG_common_block", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_common_inclusion, "DW_TAG_common_inclusion", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_inheritance, "DW_TAG_inheritance", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_inlined_subroutine, "DW_TAG_inlined_subroutine", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_module, "DW_TAG_module", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_ptr_to_member_type, "DW_TAG_ptr_to_member_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_set_type, "DW_TAG_set_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_subrange_type, "DW_TAG_subrange_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_with_stmt, "DW_TAG_with_stmt", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_access_declaration, "DW_TAG_access_declaration", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_base_type, "DW_TAG_base_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_catch_block, "DW_TAG_catch_block", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_const_type, "DW_TAG_const_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_constant, "DW_TAG_constant", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_enumerator, "DW_TAG_enumerator", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_file_type, "DW_TAG_file_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_friend, "DW_TAG_friend", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_namelist, "DW_TAG_namelist", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_namelist_item, "DW_TAG_namelist_item", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_packed_type, "DW_TAG_packed_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_subprogram, "DW_TAG_subprogram", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_template_type_parameter, "DW_TAG_template_type_parameter", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_template_value_parameter, "DW_TAG_template_value_parameter", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_thrown_type, "DW_TAG_thrown_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_try_block, "DW_TAG_try_block", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_variant_part, "DW_TAG_variant_part", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_variable, "DW_TAG_variable", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_volatile_type, "DW_TAG_volatile_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_dwarf_procedure, "DW_TAG_dwarf_procedure", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_restrict_type, "DW_TAG_restrict_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_interface_type, "DW_TAG_interface_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_namespace, "DW_TAG_namespace", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_imported_module, "DW_TAG_imported_module", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_unspecified_type, "DW_TAG_unspecified_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_partial_unit, "DW_TAG_partial_unit", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_imported_unit, "DW_TAG_imported_unit", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_condition, "DW_TAG_condition", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_shared_type, "DW_TAG_shared_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_type_unit, "DW_TAG_type_unit", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_rvalue_reference_type, "DW_TAG_rvalue_reference_type", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_template_alias, "DW_TAG_template_alias", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_lo_user, "DW_TAG_lo_user", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_hi_user, "DW_TAG_hi_user", DwarfUnhandledTag);

  /* GNU */
  AddTagHandler(DwarfTagCode::DW_TAG_GNU_template_template_param, "DW_TAG_GNU_template_template_param", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_GNU_template_parameter_pack, "DW_TAG_GNU_template_parameter_pack", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_GNU_formal_parameter_pack, "DW_TAG_GNU_formal_parameter_pack", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_GNU_call_site, "DW_TAG_GNU_call_site", DwarfUnhandledTag);
  AddTagHandler(DwarfTagCode::DW_TAG_GNU_call_site_parameter, "DW_TAG_GNU_call_site_parameter", DwarfUnhandledTag);
}
