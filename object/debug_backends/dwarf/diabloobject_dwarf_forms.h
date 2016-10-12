/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_FORMS_H
#define DIABLOOBJECT_DWARF_FORMS_H

#include <string>

enum class DwarfFormCode {
  DW_FORM_addr = 0x01,
  DW_FORM_block2 = 0x03,
  DW_FORM_block4 = 0x04,
  DW_FORM_data2 = 0x05,
  DW_FORM_data4 = 0x06,
  DW_FORM_data8 = 0x07,
  DW_FORM_string = 0x08,
  DW_FORM_block = 0x09,
  DW_FORM_block1 = 0x0a,
  DW_FORM_data1 = 0x0b,
  DW_FORM_flag = 0x0c,
  DW_FORM_sdata = 0x0d,
  DW_FORM_strp = 0x0e,
  DW_FORM_udata = 0x0f,
  DW_FORM_ref_addr = 0x10,
  DW_FORM_ref1 = 0x11,
  DW_FORM_ref2 = 0x12,
  DW_FORM_ref4 = 0x13,
  DW_FORM_ref8 = 0x14,
  DW_FORM_ref_udata = 0x15,
  DW_FORM_indirect = 0x16,
  DW_FORM_sec_offset = 0x17,
  DW_FORM_exprloc = 0x18,
  DW_FORM_flag_present = 0x19,
  DW_FORM_ref_sig8 = 0x20
};

struct DwarfAbstractAttribute;
struct DwarfFormStruct;
struct DwarfAttributeSpec;

typedef DwarfAbstractAttribute* (*dwarf_form_handler_fn)(DwarfFormStruct& form, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz);

struct DwarfFormStruct {
  DwarfFormCode code;
  std::string name;

  dwarf_form_handler_fn handler;
};

void
InitForms();

DwarfAbstractAttribute *
DwarfDecodeAttribute(DwarfAttributeSpec *attr_spec, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz);

void
StringsFreeAllFormCache();

#endif /* DIABLOOBJECT_DWARF_FORMS_H */
