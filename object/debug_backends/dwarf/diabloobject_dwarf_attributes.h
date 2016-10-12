/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_ATTRIBUTES_H
#define DIABLOOBJECT_DWARF_ATTRIBUTES_H

#include "diabloobject_dwarf_ranges.h"
#include "diabloobject_dwarf_forms.h"

#include <string>

enum class DwarfAttributeCode {
  DW_AT_sibling = 0x01,
  DW_AT_location = 0x02,
  DW_AT_name = 0x03,
  DW_AT_ordering = 0x09,
  DW_AT_byte_size = 0x0b,
  DW_AT_bit_offset = 0x0c,
  DW_AT_bit_size = 0x0d,
  DW_AT_stmt_list = 0x10,
  DW_AT_low_pc = 0x11,
  DW_AT_high_pc = 0x12,
  DW_AT_language = 0x13,
  DW_AT_discr = 0x15,
  DW_AT_discr_value = 0x16,
  DW_AT_visibility = 0x17,
  DW_AT_import = 0x18,
  DW_AT_string_length = 0x19,
  DW_AT_common_reference = 0x1a,
  DW_AT_comp_dir = 0x1b,
  DW_AT_const_value = 0x1c,
  DW_AT_containing_type = 0x1d,
  DW_AT_default_value = 0x1e,
  DW_AT_inline = 0x20,
  DW_AT_is_optional = 0x21,
  DW_AT_lower_bound = 0x22,
  DW_AT_producer = 0x25,
  DW_AT_prototyped = 0x27,
  DW_AT_return_addr = 0x2a,
  DW_AT_start_scope = 0x2c,
  DW_AT_bit_stride = 0x2e,
  DW_AT_upper_bound = 0x2f,
  DW_AT_abstract_origin = 0x31,
  DW_AT_accessibility = 0x32,
  DW_AT_address_class = 0x33,
  DW_AT_artificial = 0x34,
  DW_AT_base_types = 0x35,
  DW_AT_calling_convention = 0x36,
  DW_AT_count = 0x37,
  DW_AT_data_member_location = 0x38,
  DW_AT_decl_column = 0x39,
  DW_AT_decl_file = 0x3a,
  DW_AT_decl_line = 0x3b,
  DW_AT_declaration = 0x3c,
  DW_AT_discr_list = 0x3d,
  DW_AT_encoding = 0x3e,
  DW_AT_external = 0x3f,
  DW_AT_frame_base = 0x40,
  DW_AT_friend = 0x41,
  DW_AT_identifier_case = 0x42,
  DW_AT_macro_info = 0x43,
  DW_AT_namelist_item = 0x44,
  DW_AT_priority = 0x45,
  DW_AT_segment = 0x46,
  DW_AT_specification = 0x47,
  DW_AT_static_link = 0x48,
  DW_AT_type = 0x49,
  DW_AT_use_location = 0x4a,
  DW_AT_variable_parameter = 0x4b,
  DW_AT_virtuality = 0x4c,
  DW_AT_vtable_elem_location = 0x4d,
  DW_AT_allocated = 0x4e,
  DW_AT_associated = 0x4f,
  DW_AT_data_location = 0x50,
  DW_AT_byte_stride = 0x51,
  DW_AT_entry_pc = 0x52,
  DW_AT_use_UTF8 = 0x53,
  DW_AT_extension = 0x54,
  DW_AT_ranges = 0x55,
  DW_AT_trampoline = 0x56,
  DW_AT_call_column = 0x57,
  DW_AT_call_file = 0x58,
  DW_AT_call_line = 0x59,
  DW_AT_description = 0x5a,
  DW_AT_binary_scale = 0x5b,
  DW_AT_decimal_scale = 0x5c,
  DW_AT_small = 0x5d,
  DW_AT_decimal_sign = 0x5e,
  DW_AT_digit_count = 0x5f,
  DW_AT_picture_string = 0x60,
  DW_AT_mutable = 0x61,
  DW_AT_threads_scaled = 0x62,
  DW_AT_explicit = 0x63,
  DW_AT_object_pointer = 0x64,
  DW_AT_endianity = 0x65,
  DW_AT_elemental = 0x66,
  DW_AT_pure = 0x67,
  DW_AT_recursive = 0x68,
  DW_AT_signature = 0x69,
  DW_AT_main_subprogram = 0x6a,
  DW_AT_data_bit_offset = 0x6b,
  DW_AT_const_expr = 0x6c,
  DW_AT_enum_class = 0x6d,
  DW_AT_linkage_name = 0x6e,
  DW_AT_lo_user = 0x2000,
  DW_AT_hi_user = 0x3fff,

  DW_AT_MIPS_linkage_name = 0x2007,

  DW_AT_GNU_template_name = 0x2110,
  DW_AT_GNU_call_site_value = 0x2111,
  DW_AT_GNU_call_site_data_value = 0x2112,
  DW_AT_GNU_call_site_target = 0x2113,
  DW_AT_GNU_tail_call = 0x2115,
  DW_AT_GNU_all_tail_call_sites = 0x2116,
  DW_AT_GNU_all_call_sites = 0x2117
};

enum class DwarfAttributeForm {
  Address,
  Block,
  Constant,
  Exprloc,
  Flag,
  SecOffset,
  Reference,
  String
};

/* types of parsed attributes */
enum class DwarfParsedAttributeType {
  /* attribute does not need additional parsing */
  Generic,

  /* attribute of form 'lineptr', pointing to line number information */
  LinePtr,

  /* attribute of form 'rangelistptr', pointing to a non-contiguous address range list */
  RangeListPtr
};

/* attribute as specified in the binary blob */
struct DwarfAttributeSpec {
  DwarfAttributeCode name;
  DwarfFormCode form;
};

/* abstract type for decoded but unparsed attributes */
struct DwarfAbstractAttribute {
  DwarfAttributeForm form;
  DwarfAttributeSpec *attr;

  virtual std::string ToString() { return "(abstract)"; };

  virtual ~DwarfAbstractAttribute() {}
};

/* abstract type for parsed attributes */
struct DwarfAbstractParsedAttribute {
  DwarfAbstractParsedAttribute() { type = DwarfParsedAttributeType::Generic; }

  DwarfAbstractAttribute *decoded;
  DwarfParsedAttributeType type;

  virtual ~DwarfAbstractParsedAttribute() {
    delete decoded;
  }
};

struct DwarfLinePtrAttribute
        : public DwarfAbstractParsedAttribute {
  DwarfLinePtrAttribute() { type = DwarfParsedAttributeType::LinePtr; }

  DwarfLineNumberProgramHeader *line_info;

  ~DwarfLinePtrAttribute() {
    delete line_info;
  }
};

struct DwarfRangeListPtrAttribute
        : public DwarfAbstractParsedAttribute {
  DwarfRangeListPtrAttribute() { type = DwarfParsedAttributeType::RangeListPtr; }

  DwarfRangeList *range_list;

  ~DwarfRangeListPtrAttribute() {
    for (auto e : *range_list)
      delete e;

    delete range_list;
  }
};

struct DwarfAddressAttribute
                : public DwarfAbstractAttribute {
  DwarfAddressAttribute() { form = DwarfAttributeForm::Address; }
  std::string ToString() { return std::to_string(value); }

  t_address value;
};

struct DwarfBlockAttribute
                : public DwarfAbstractAttribute {
  DwarfBlockAttribute() { form = DwarfAttributeForm::Block; }
  std::string ToString() { return std::to_string(length); }

  t_uint64 length;
  t_uint8 *value;

  ~DwarfBlockAttribute() {
    delete[] value;
  }
};

struct DwarfConstantAttribute
                : public DwarfAbstractAttribute {
  DwarfConstantAttribute() { form = DwarfAttributeForm::Constant; }
  std::string ToString() { return std::to_string(value); }

  t_uint64 value;
  bool is_signed;
};

struct DwarfExprlocAttribute
                : public DwarfAbstractAttribute {
  DwarfExprlocAttribute() { form = DwarfAttributeForm::Exprloc; }
  std::string ToString() { return std::to_string(length); }

  DwarfDecodedULEB128 length;
  t_uint8 *value;

  ~DwarfExprlocAttribute() {
    delete[] value;
  }
};

struct DwarfFlagAttribute
                : public DwarfAbstractAttribute {
  DwarfFlagAttribute() { form = DwarfAttributeForm::Flag; }
  std::string ToString() { return std::to_string(value); }

  t_uint8 value;
};

struct DwarfSecOffsetAttribute
                : public DwarfAbstractAttribute {
  DwarfSecOffsetAttribute() { form = DwarfAttributeForm::SecOffset; }
  std::string ToString() { return std::to_string(value); }

  t_address value;
};

struct DwarfReferenceAttribute
                : public DwarfAbstractAttribute {
  DwarfReferenceAttribute() { form = DwarfAttributeForm::Reference; }
  std::string ToString() { return std::to_string(value); }

  t_address value;
};

struct DwarfStringAttribute
                : public DwarfAbstractAttribute {
  DwarfStringAttribute() { form = DwarfAttributeForm::String; }
  std::string ToString() { return std::string(*value); }

  std::string *value;

  /* value should not be deleted here, because it is cached
   * and multiple pointers may point to this instance */
};

void InitAttributes();
std::string AttributeToString(DwarfAttributeCode code);
DwarfAbstractParsedAttribute *DwarfParseAttribute(DwarfCompilationUnitHeader *cu, DwarfSections *, DwarfAbstractAttribute *);
bool AttributeNeedsParsing(DwarfAbstractAttribute *);

#endif /* DIABLOOBJECT_DWARF_ATTRIBUTES_H */
