/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"

#include <map>
#include <string>

#include <iostream>

using namespace std;

static map<DwarfFormCode, DwarfFormStruct> form_handlers;
static map<t_address, string *> dwarf_strings_cache;
static map<string, string*> dwarf_strings_cache2;

#define DEBUG_READ 0

/* default handler */
static
DwarfAbstractAttribute *
DwarfUnhandledForm(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  FATAL(("unsupported DWARF form: 0x%x (%s)", s.code, s.name.c_str()));
}

/* parse fixed-length constants */
static
DwarfAbstractAttribute *
DwarfParseFixedConstantForm(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfConstantAttribute *ret = new DwarfConstantAttribute();

  switch (s.code)
  {
  case DwarfFormCode::DW_FORM_data1:
    sz = 1;
    ret->value = static_cast<t_uint64>(SectionGetData8(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_data2:
    sz = 2;
    ret->value = static_cast<t_uint64>(SectionGetData16(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_data4:
    sz = 4;
    ret->value = static_cast<t_uint64>(SectionGetData32(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_data8:
    sz = 8;
    ret->value = SectionGetData64(dwarf_sections->info_section, info_section_offset);
    break;

  default:
    FATAL(("unsupported code %x", s.code));
  }

  ret->is_signed = false;

#if DEBUG_READ
  DEBUG(("read constant %llx", ret->value));
#endif
  return ret;
}

/* parse variable-length constants */
static
DwarfAbstractAttribute *
DwarfParseVariableConstantForm(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfConstantAttribute *ret = new DwarfConstantAttribute();

  switch (s.code)
  {
  case DwarfFormCode::DW_FORM_sdata:
    ret->value = static_cast<t_uint64>(DwarfDecodeSLEB128(DwarfReadSLEB128FromSection(dwarf_sections->info_section, info_section_offset, sz)));
    ASSERT(sz <= sizeof(t_uint64), ("decoded SLEB128 does not fit in 64-bit"));
    ret->is_signed = true;
    break;

  case DwarfFormCode::DW_FORM_udata:
    ret->value = static_cast<t_uint64>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(dwarf_sections->info_section, info_section_offset, sz)));
    ASSERT(sz <= sizeof(t_uint64), ("decoded ULEB128 does not fit in 64-bit"));
    ret->is_signed = false;
    break;

  default:
    FATAL(("unsupported code %x", s.code));
  }

#if DEBUG_READ
  DEBUG(("read variable length constant of size %d 0x%x", sz, ret->value));
#endif
  return ret;
}

/* parse addresses */
static
DwarfAbstractAttribute *
DwarfParseAddress(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfAddressAttribute *ret = new DwarfAddressAttribute();

  ret->value = DwarfReadAddress(cu->address_size, dwarf_sections->info_section, info_section_offset);
  sz = cu->address_size;

#if DEBUG_READ
  DEBUG(("read address %llx", ret->value));
#endif
  return ret;
}

/* parse strings in the same section */
static
DwarfAbstractAttribute *
DwarfParseString(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfStringAttribute *ret = new DwarfStringAttribute();
  string str = ReadStringFromSection(dwarf_sections->info_section, info_section_offset);

  if (dwarf_strings_cache2.find(str) == dwarf_strings_cache2.end())
    dwarf_strings_cache2[str] = new string(str);

  ret->value = dwarf_strings_cache2[str];
  sz = static_cast<t_uint32>(ret->value->length()) + 1;

#if DEBUG_READ
  DEBUG(("read string %s", ret->value->c_str()));
#endif
  return ret;
}

/* parse a string pointer, pointing to another section */
static
DwarfAbstractAttribute *
DwarfParseStrp(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfStringAttribute *ret = new DwarfStringAttribute();
  t_address offset;

  if (cu->is_64bit)
  {
    ASSERT(sizeof(t_address) == 8, ("64-bit DWARF data found, but Diablo is compiled for 32-bit!"));

    sz = 8;
    offset = SectionGetData64(dwarf_sections->info_section, info_section_offset);
  }
  else
  {
    sz = 4;
    offset = static_cast<t_address>(SectionGetData32(dwarf_sections->info_section, info_section_offset));
  }

  /* cache lookup */
  if (dwarf_strings_cache.find(offset) != dwarf_strings_cache.end())
    ret->value = dwarf_strings_cache[offset];
  else
  {
    ret->value = new string(ReadStringFromSection(dwarf_sections->str_section, offset));
    dwarf_strings_cache[offset] = ret->value;
  }

#if DEBUG_READ
  DEBUG(("read string from +@G %s", offset, ret->value->c_str()));
#endif
  return ret;
}

/* parse section offsets */
static
DwarfAbstractAttribute *
DwarfParseSecOffset(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfSecOffsetAttribute *ret = new DwarfSecOffsetAttribute();

  if (cu->is_64bit)
  {
    ASSERT(sizeof(t_address) == 8, ("64-bit DWARF section offset found, but Diablo is compiled for 32-bit!"));

    sz = 8;
    ret->value = static_cast<t_address>(SectionGetData64(dwarf_sections->info_section, info_section_offset));
  }
  else
  {
    sz = 4;
    ret->value = static_cast<t_address>(SectionGetData32(dwarf_sections->info_section, info_section_offset));
  }

#if DEBUG_READ
  DEBUG(("read section offset %llx", ret->value));
#endif
  return ret;
}

/* parse references */
static
DwarfAbstractAttribute *
DwarfParseReference(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfReferenceAttribute *ret = new DwarfReferenceAttribute();

  switch (s.code)
  {
  case DwarfFormCode::DW_FORM_ref1:
    sz = 1;
    ret->value = static_cast<t_address>(SectionGetData8(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_ref2:
    sz = 2;
    ret->value = static_cast<t_address>(SectionGetData16(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_ref4:
    sz = 4;
    ret->value = static_cast<t_address>(SectionGetData32(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_ref8:
    ASSERT(sizeof(t_address) == 8, ("64-bit DWARF data found, but Diablo is compiled for 32-bit!"));

    sz = 8;
    ret->value = static_cast<t_address>(SectionGetData64(dwarf_sections->info_section, info_section_offset));
    break;

  case DwarfFormCode::DW_FORM_ref_udata:
    ret->value = static_cast<t_address>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(dwarf_sections->info_section, info_section_offset, sz)));
    ASSERT(sz <= sizeof(t_address), ("length of decoded ULEB128 integer is bigger than supported length"));
    break;

  case DwarfFormCode::DW_FORM_ref_addr:
    sz = cu->address_size;
    ret->value = DwarfReadAddress(cu->address_size, dwarf_sections->info_section, info_section_offset);
    break;

  case DwarfFormCode::DW_FORM_ref_sig8:
    FATAL(("implement me"));
    break;

  default:
    FATAL(("unsupported code %x", s.code));
  }

#if DEBUG_READ
  DEBUG(("read reference %llx", ret->value));
#endif
  return ret;
}

/* parse flags */
static
DwarfAbstractAttribute *
DwarfParseFlag(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfFlagAttribute *ret = new DwarfFlagAttribute();

  switch (s.code)
  {
  case DwarfFormCode::DW_FORM_flag_present:
    sz = 0;
    ret->value = 0xff;
    break;

  case DwarfFormCode::DW_FORM_flag:
    sz = 1;
    ret->value = SectionGetData8(dwarf_sections->info_section, info_section_offset);
    break;

  default:
    FATAL(("unsupported code %x", s.code));
  }

#if DEBUG_READ
  DEBUG(("read flag %x", ret->value));
#endif
  return ret;
}

/* parse expression locations */
static
DwarfAbstractAttribute *
DwarfParseExprloc(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfExprlocAttribute *ret = new DwarfExprlocAttribute();
  t_uint32 n_bytes = 0;
  t_uint32 i;

  ret->length = DwarfDecodeULEB128(DwarfReadULEB128FromSection(dwarf_sections->info_section, info_section_offset, n_bytes));
  sz = n_bytes + static_cast<t_uint32>(ret->length);

  ret->value = new t_uint8[ret->length];
  for (i = 0; i < ret->length; i++)
    ret->value[i] = SectionGetData8(dwarf_sections->info_section, AddressAddUint32(info_section_offset, n_bytes + i));

#if DEBUG_READ
  DEBUG(("read expression of length %d (%d)", ret->length, n_bytes));
#endif
  return ret;
}

/* parse blocks */
static
DwarfAbstractAttribute *
DwarfParseBlock(DwarfFormStruct& s, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfBlockAttribute *ret = new DwarfBlockAttribute();

  t_uint32 i;

  switch (s.code)
  {
  case DwarfFormCode::DW_FORM_block1:
    ret->length = static_cast<t_uint64>(SectionGetData8(dwarf_sections->info_section, info_section_offset));
    sz = 1;
    break;

  case DwarfFormCode::DW_FORM_block2:
    ret->length = static_cast<t_uint64>(SectionGetData16(dwarf_sections->info_section, info_section_offset));
    sz = 2;
    break;

  case DwarfFormCode::DW_FORM_block4:
    ret->length = static_cast<t_uint64>(SectionGetData32(dwarf_sections->info_section, info_section_offset));
    sz = 4;
    break;

  case DwarfFormCode::DW_FORM_block:
    ret->length = static_cast<t_uint64>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(dwarf_sections->info_section, info_section_offset, sz)));
    ASSERT(sz <= sizeof(t_uint64), ("decoded ULEB128 block length does not fit in t_uint64"));
    break;

  default:
    FATAL(("unsupported code %x", s.code));
  }

  ret->value = new t_uint8[ret->length];
  for (i = 0; i < ret->length; i++)
    ret->value[i] = SectionGetData8(dwarf_sections->info_section, AddressAddUint32(info_section_offset, sz + i));

  sz += static_cast<t_uint32>(ret->length);
  ASSERT(ret->length < static_cast<t_uint32>(~0), ("block length does not fit in 32 bits"));

#if DEBUG_READ
  DEBUG(("read block of length %d", ret->length));
#endif
  return ret;
}

/* given a form code, get the internal data structure */
static
DwarfFormStruct
GetFormInfo(DwarfFormCode idx)
{
  if (form_handlers.find(idx) == form_handlers.end())
    FATAL(("undefined form 0x%x", idx));

  return form_handlers[idx];
}

/* decode an attribute, retrieving the associated data */
DwarfAbstractAttribute *
DwarfDecodeAttribute(DwarfAttributeSpec *attr_spec, DwarfCompilationUnitHeader *cu, DwarfSections *dwarf_sections, t_address info_section_offset, t_uint32& sz)
{
  DwarfFormStruct form = GetFormInfo(attr_spec->form);
  DwarfAbstractAttribute *ret = form.handler(form, cu, dwarf_sections, info_section_offset, sz);

  ret->attr = attr_spec;

  return ret;
}

/* install a new attribute */
static inline
void
AddFormHandler(DwarfFormCode idx, string name, dwarf_form_handler_fn handler)
{
  form_handlers[idx] = DwarfFormStruct{idx, name, handler};
}

/* install all supported attributes */
void
InitForms()
{
  AddFormHandler(DwarfFormCode::DW_FORM_addr, "DW_FORM_addr", DwarfParseAddress);
  AddFormHandler(DwarfFormCode::DW_FORM_block2, "DW_FORM_block2", DwarfParseBlock);
  AddFormHandler(DwarfFormCode::DW_FORM_block4, "DW_FORM_block4", DwarfParseBlock);
  AddFormHandler(DwarfFormCode::DW_FORM_data2, "DW_FORM_data2", DwarfParseFixedConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_data4, "DW_FORM_data4", DwarfParseFixedConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_data8, "DW_FORM_data8", DwarfParseFixedConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_string, "DW_FORM_string", DwarfParseString);
  AddFormHandler(DwarfFormCode::DW_FORM_block, "DW_FORM_block", DwarfParseBlock);
  AddFormHandler(DwarfFormCode::DW_FORM_block1, "DW_FORM_block1", DwarfParseBlock);
  AddFormHandler(DwarfFormCode::DW_FORM_data1, "DW_FORM_data1", DwarfParseFixedConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_flag, "DW_FORM_flag", DwarfParseFlag);
  AddFormHandler(DwarfFormCode::DW_FORM_sdata, "DW_FORM_sdata", DwarfParseVariableConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_strp, "DW_FORM_strp", DwarfParseStrp);
  AddFormHandler(DwarfFormCode::DW_FORM_udata, "DW_FORM_udata", DwarfParseVariableConstantForm);
  AddFormHandler(DwarfFormCode::DW_FORM_ref_addr, "DW_FORM_ref_addr", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_ref1, "DW_FORM_ref1", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_ref2, "DW_FORM_ref2", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_ref4, "DW_FORM_ref4", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_ref8, "DW_FORM_ref8", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_ref_udata, "DW_FORM_ref_udata", DwarfParseReference);
  AddFormHandler(DwarfFormCode::DW_FORM_indirect, "DW_FORM_indirect", DwarfUnhandledForm);
  AddFormHandler(DwarfFormCode::DW_FORM_sec_offset, "DW_FORM_sec_offset", DwarfParseSecOffset);
  AddFormHandler(DwarfFormCode::DW_FORM_exprloc, "DW_FORM_exprloc", DwarfParseExprloc);
  AddFormHandler(DwarfFormCode::DW_FORM_flag_present, "DW_FORM_flag_present", DwarfParseFlag);
  AddFormHandler(DwarfFormCode::DW_FORM_ref_sig8, "DW_FORM_ref_sig8", DwarfParseReference);
}

void
StringsFreeAllFormCache()
{
  for (auto e : dwarf_strings_cache)
    delete e.second;

  for (auto e : dwarf_strings_cache2)
    delete e.second;
}
