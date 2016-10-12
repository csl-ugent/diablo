/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"
#include "diabloobject_dwarf_attributes.h"

#include <map>
#include <string>

using namespace std;

/* attribute handler */
struct DwarfAttributeStruct;
typedef DwarfAbstractParsedAttribute *(*dwarf_attribute_handler_fn)(DwarfAttributeStruct&, DwarfCompilationUnitHeader *cu, DwarfSections *, DwarfAbstractAttribute *);

/* keep string representation and handler for every attribute */
struct DwarfAttributeStruct {
  DwarfAttributeCode code;
  std::string name;

  dwarf_attribute_handler_fn handler;
};

static map<DwarfAttributeCode, DwarfAttributeStruct> attribute_handlers;

bool AttributeNeedsParsing(DwarfAbstractAttribute *attr)
{
  switch(attr->attr->name)
  {
  case DwarfAttributeCode::DW_AT_stmt_list:
  case DwarfAttributeCode::DW_AT_ranges:
    return true;

  default:;
  }

  return false;
}

static
DwarfAbstractParsedAttribute *
DwarfSkipParse(DwarfAttributeStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, DwarfAbstractAttribute *decoded_attribute)
{
  return new DwarfAbstractParsedAttribute();
}

/* Default handler: FATAL only if the attribute does not have a default handler and it needs to be parsed */
static
DwarfAbstractParsedAttribute *
DwarfUnhandledAttribute(DwarfAttributeStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, DwarfAbstractAttribute *decoded_attribute)
{
  if (!AttributeNeedsParsing(decoded_attribute))
    return DwarfSkipParse(s, cu, dwarf_sections, decoded_attribute);

  FATAL(("unsupported DWARF attribute: 0x%x (%s); form %x", s.code, s.name.c_str(), decoded_attribute->form));
}

/* parsing a 'statement list', which contains line number information */
static
DwarfAbstractParsedAttribute *
DwarfParseStmtList(DwarfAttributeStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, DwarfAbstractAttribute *decoded_attribute)
{
  DwarfLinePtrAttribute *ret = new DwarfLinePtrAttribute();
  DwarfSecOffsetAttribute *attr = static_cast<DwarfSecOffsetAttribute *>(decoded_attribute);

  ret->line_info = ReadLineNumberProgram(cu, dwarf_sections->line_section, attr->value);

  return ret;
}

/* parsing an 'address range', which contains a list of non-contiguous address ranges */
static
DwarfAbstractParsedAttribute *
DwarfParseRangeListPtr(DwarfAttributeStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, DwarfAbstractAttribute *decoded_attribute)
{
  DwarfRangeListPtrAttribute *ret = new DwarfRangeListPtrAttribute();
  DwarfSecOffsetAttribute *attr = static_cast<DwarfSecOffsetAttribute *>(decoded_attribute);

  ret->range_list = ReadAddressRange(cu, dwarf_sections->ranges_section, attr->value);

  return ret;
}

/* given an attribute code, look up its associated data structure */
static
DwarfAttributeStruct
GetAttributeInfo(DwarfAttributeCode idx)
{
  if (attribute_handlers.find(idx) == attribute_handlers.end())
    return DwarfAttributeStruct{DwarfAttributeCode::DW_AT_lo_user,"",DwarfSkipParse};

  return attribute_handlers[idx];
}

/* handle the given decoded attribute, and parse it */
DwarfAbstractParsedAttribute *
DwarfParseAttribute(DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, DwarfAbstractAttribute *decoded_attribute)
{
  DwarfAttributeStruct s = GetAttributeInfo(decoded_attribute->attr->name);
  DwarfAbstractParsedAttribute *ret = s.handler(s, cu, dwarf_sections, decoded_attribute);

  ret->decoded = decoded_attribute;

  return ret;
}

/* install a new attribute 'idx' handler */
static inline
void
AddAttributeHandler(DwarfAttributeCode idx, string name, dwarf_attribute_handler_fn handler)
{
  attribute_handlers[idx] = DwarfAttributeStruct{idx, name, handler};
}

/* get the string represenation corresponding to the given attribute code */
std::string
AttributeToString(DwarfAttributeCode code)
{
  DwarfAttributeStruct s = GetAttributeInfo(code);

  return s.name;
}

/* install all attribute handlers */
void
InitAttributes()
{
  AddAttributeHandler(DwarfAttributeCode::DW_AT_sibling, "DW_AT_sibling", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_location, "DW_AT_location", DwarfSkipParse);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_name, "DW_AT_name", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_ordering, "DW_AT_ordering", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_byte_size, "DW_AT_byte_size", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_bit_offset, "DW_AT_bit_offset", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_bit_size, "DW_AT_bit_size", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_stmt_list, "DW_AT_stmt_list", DwarfParseStmtList);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_low_pc, "DW_AT_low_pc", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_high_pc, "DW_AT_high_pc", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_language, "DW_AT_language", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_discr, "DW_AT_discr", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_discr_value, "DW_AT_discr_value", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_visibility, "DW_AT_visibility", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_import, "DW_AT_import", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_string_length, "DW_AT_string_length", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_common_reference, "DW_AT_common_reference", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_comp_dir, "DW_AT_comp_dir", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_const_value, "DW_AT_const_value", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_containing_type, "DW_AT_containing_type", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_default_value, "DW_AT_default_value", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_inline, "DW_AT_inline", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_is_optional, "DW_AT_is_optional", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_lower_bound, "DW_AT_lower_bound", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_producer, "DW_AT_producer", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_prototyped, "DW_AT_prototyped", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_return_addr, "DW_AT_return_addr", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_start_scope, "DW_AT_start_scope", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_bit_stride, "DW_AT_bit_stride", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_upper_bound, "DW_AT_upper_bound", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_abstract_origin, "DW_AT_abstract_origin", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_accessibility, "DW_AT_accessibility", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_address_class, "DW_AT_address_class", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_artificial, "DW_AT_artificial", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_base_types, "DW_AT_base_types", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_calling_convention, "DW_AT_calling_convention", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_count, "DW_AT_count", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_data_member_location, "DW_AT_data_member_location", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_decl_column, "DW_AT_decl_column", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_decl_file, "DW_AT_decl_file", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_decl_line, "DW_AT_decl_line", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_declaration, "DW_AT_declaration", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_discr_list, "DW_AT_discr_list", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_encoding, "DW_AT_encoding", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_external, "DW_AT_external", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_frame_base, "DW_AT_frame_base", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_friend, "DW_AT_friend", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_identifier_case, "DW_AT_identifier_case", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_macro_info, "DW_AT_macro_info", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_namelist_item, "DW_AT_namelist_item", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_priority, "DW_AT_priority", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_segment, "DW_AT_segment", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_specification, "DW_AT_specification", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_static_link, "DW_AT_static_link", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_type, "DW_AT_type", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_use_location, "DW_AT_use_location", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_variable_parameter, "DW_AT_variable_parameter", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_virtuality, "DW_AT_virtuality", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_vtable_elem_location, "DW_AT_vtable_elem_location", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_allocated, "DW_AT_allocated", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_associated, "DW_AT_associated", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_data_location, "DW_AT_data_location", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_byte_stride, "DW_AT_byte_stride", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_entry_pc, "DW_AT_entry_pc", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_use_UTF8, "DW_AT_use_UTF8", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_extension, "DW_AT_extension", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_ranges, "DW_AT_ranges", DwarfParseRangeListPtr);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_trampoline, "DW_AT_trampoline", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_call_column, "DW_AT_call_column", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_call_file, "DW_AT_call_file", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_call_line, "DW_AT_call_line", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_description, "DW_AT_description", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_binary_scale, "DW_AT_binary_scale", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_decimal_scale, "DW_AT_decimal_scale", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_small, "DW_AT_small", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_decimal_sign, "DW_AT_decimal_sign", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_digit_count, "DW_AT_digit_count", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_picture_string, "DW_AT_picture_string", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_mutable, "DW_AT_mutable", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_threads_scaled, "DW_AT_threads_scaled", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_explicit, "DW_AT_explicit", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_object_pointer, "DW_AT_object_pointer", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_endianity, "DW_AT_endianity", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_elemental, "DW_AT_elemental", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_pure, "DW_AT_pure", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_recursive, "DW_AT_recursive", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_signature, "DW_AT_signature", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_main_subprogram, "DW_AT_main_subprogram", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_data_bit_offset, "DW_AT_data_bit_offset", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_const_expr, "DW_AT_const_expr", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_enum_class, "DW_AT_enum_class", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_linkage_name, "DW_AT_linkage_name", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_lo_user, "DW_AT_lo_user", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_hi_user, "DW_AT_hi_user", DwarfUnhandledAttribute);

  AddAttributeHandler(DwarfAttributeCode::DW_AT_MIPS_linkage_name, "DW_AT_MIPS_linkage_name", DwarfUnhandledAttribute);

  /* GNU */
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_template_name, "DW_AT_GNU_template_name", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_call_site_value, "DW_AT_GNU_call_site_value", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_tail_call, "DW_AT_GNU_tail_call", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_all_tail_call_sites, "DW_AT_GNU_all_tail_call_sites", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_all_call_sites, "DW_AT_GNU_all_call_sites", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_call_site_target, "DW_AT_GNU_call_site_target", DwarfUnhandledAttribute);
  AddAttributeHandler(DwarfAttributeCode::DW_AT_GNU_call_site_data_value, "DW_AT_GNU_call_site_data_value", DwarfUnhandledAttribute);
}
