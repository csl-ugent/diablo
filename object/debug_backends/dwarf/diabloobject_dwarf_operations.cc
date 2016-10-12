/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloobject_dwarf.h"
#include "diabloobject_dwarf_operations.h"

#include <map>
#include <string>

using namespace std;

struct DwarfOperationStruct;
typedef DwarfAbstractOperation* (*dwarf_operation_handler_fn)(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz);

struct DwarfOperationStruct {
  DwarfOperationCode code;
  std::string name;

  dwarf_operation_handler_fn handler;
};

static map<DwarfOperationCode, DwarfOperationStruct> operation_handlers;

static
DwarfAbstractOperation *
DwarfHandleStackOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfStackOperation *ret = new DwarfStackOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_dup:
  case DwarfOperationCode::DW_OP_drop:

  case DwarfOperationCode::DW_OP_over:
  case DwarfOperationCode::DW_OP_swap:
  case DwarfOperationCode::DW_OP_rot:
  case DwarfOperationCode::DW_OP_deref:

  case DwarfOperationCode::DW_OP_xderef:

  case DwarfOperationCode::DW_OP_push_object_address:
  case DwarfOperationCode::DW_OP_form_tls_address:
  case DwarfOperationCode::DW_OP_call_frame_cfa:
    break;

  case DwarfOperationCode::DW_OP_pick:
  case DwarfOperationCode::DW_OP_deref_size:
  case DwarfOperationCode::DW_OP_xderef_size:
    ret->value = static_cast<t_uint64>(SectionGetData8(section, section_offset));
    sz = 1;
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  return ret;
}

static
DwarfAbstractOperation *
DwarfHandleArithOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfArithOperation *ret = new DwarfArithOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_abs:
  case DwarfOperationCode::DW_OP_and:
  case DwarfOperationCode::DW_OP_div:
  case DwarfOperationCode::DW_OP_minus:
  case DwarfOperationCode::DW_OP_mod:
  case DwarfOperationCode::DW_OP_mul:
  case DwarfOperationCode::DW_OP_neg:
  case DwarfOperationCode::DW_OP_not:
  case DwarfOperationCode::DW_OP_or:
  case DwarfOperationCode::DW_OP_plus:
  case DwarfOperationCode::DW_OP_shl:
  case DwarfOperationCode::DW_OP_shr:
  case DwarfOperationCode::DW_OP_shra:
  case DwarfOperationCode::DW_OP_xor:
    break;

  case DwarfOperationCode::DW_OP_plus_uconst:
    ret->value = static_cast<t_uint64>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(section, section_offset, sz)));
    ASSERT(sz <= sizeof(t_uint64), ("could not decode ULEB128 in t_uint64"));
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  return ret;
}

static
DwarfAbstractOperation *
DwarfHandleBranchOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfBranchOperation *ret = new DwarfBranchOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_le:
  case DwarfOperationCode::DW_OP_ge:
  case DwarfOperationCode::DW_OP_eq:
  case DwarfOperationCode::DW_OP_lt:
  case DwarfOperationCode::DW_OP_gt:
  case DwarfOperationCode::DW_OP_ne:
    break;

  case DwarfOperationCode::DW_OP_skip:
  case DwarfOperationCode::DW_OP_bra:
    ret->value = static_cast<t_address>(SectionGetData16(section, section_offset));
    sz = 2;
    break;

  case DwarfOperationCode::DW_OP_call2:
    ret->value = static_cast<t_address>(SectionGetData16(section, section_offset));
    sz = 2;
    break;

  case DwarfOperationCode::DW_OP_call4:
    ret->value = static_cast<t_address>(SectionGetData32(section, section_offset));
    sz = 4;
    break;

  case DwarfOperationCode::DW_OP_call_ref:
    ret->value = DwarfReadAddress(cu->address_size, section, section_offset);
    sz = cu->address_size;
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  return ret;
}

static
DwarfAbstractOperation *
DwarfHandleAddressOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfAddressOperation *ret = new DwarfAddressOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_addr:
    ret->value = DwarfReadAddress(cu->address_size, section, section_offset);
    sz = cu->address_size;
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  return ret;
}

static
DwarfAbstractOperation *
DwarfHandleRegisterOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfRegisterOffsetOperation *ret = new DwarfRegisterOffsetOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_fbreg:
    ret->reg = DwarfOperationRegister::fbreg;
  case DwarfOperationCode::DW_OP_breg0:
  case DwarfOperationCode::DW_OP_breg1:
  case DwarfOperationCode::DW_OP_breg2:
  case DwarfOperationCode::DW_OP_breg3:
  case DwarfOperationCode::DW_OP_breg4:
  case DwarfOperationCode::DW_OP_breg5:
  case DwarfOperationCode::DW_OP_breg6:
  case DwarfOperationCode::DW_OP_breg7:
  case DwarfOperationCode::DW_OP_breg8:
  case DwarfOperationCode::DW_OP_breg9:
  case DwarfOperationCode::DW_OP_breg10:
  case DwarfOperationCode::DW_OP_breg11:
  case DwarfOperationCode::DW_OP_breg12:
  case DwarfOperationCode::DW_OP_breg13:
  case DwarfOperationCode::DW_OP_breg14:
  case DwarfOperationCode::DW_OP_breg15:
  case DwarfOperationCode::DW_OP_breg16:
  case DwarfOperationCode::DW_OP_breg17:
  case DwarfOperationCode::DW_OP_breg18:
  case DwarfOperationCode::DW_OP_breg19:
  case DwarfOperationCode::DW_OP_breg20:
  case DwarfOperationCode::DW_OP_breg21:
  case DwarfOperationCode::DW_OP_breg22:
  case DwarfOperationCode::DW_OP_breg23:
  case DwarfOperationCode::DW_OP_breg24:
  case DwarfOperationCode::DW_OP_breg25:
  case DwarfOperationCode::DW_OP_breg26:
  case DwarfOperationCode::DW_OP_breg27:
  case DwarfOperationCode::DW_OP_breg28:
  case DwarfOperationCode::DW_OP_breg29:
  case DwarfOperationCode::DW_OP_breg30:
  case DwarfOperationCode::DW_OP_breg31:
    if (s.code != DwarfOperationCode::DW_OP_fbreg)
      ret->reg = static_cast<DwarfOperationRegister>(static_cast<t_uint32>(s.code) - static_cast<t_uint32>(DwarfOperationRegister::breg0));

    ret->value = static_cast<t_uint64>(DwarfDecodeSLEB128(DwarfReadSLEB128FromSection(section, section_offset, sz)));
    ASSERT(sz <= sizeof(t_uint64), ("decoded ULEB128 does not fit in t_uint64"));
    break;

  case DwarfOperationCode::DW_OP_bregx:
  {
    t_uint32 new_sz = 0;

    ret->is_regnum = true;
    ret->regnum = static_cast<t_uint64>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(section, section_offset, sz)));
    new_sz = sz;
    section_offset = AddressAddUint32(section_offset, sz);
    ret->value = static_cast<t_uint64>(DwarfDecodeSLEB128(DwarfReadSLEB128FromSection(section, section_offset, sz)));
    sz += new_sz;
  }
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  return ret;
}

static
DwarfAbstractOperation *
DwarfHandleConstantOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  DwarfConstantOperation *ret = new DwarfConstantOperation();

  switch (s.code)
  {
  case DwarfOperationCode::DW_OP_lit0:
  case DwarfOperationCode::DW_OP_lit1:
  case DwarfOperationCode::DW_OP_lit2:
  case DwarfOperationCode::DW_OP_lit3:
  case DwarfOperationCode::DW_OP_lit4:
  case DwarfOperationCode::DW_OP_lit5:
  case DwarfOperationCode::DW_OP_lit6:
  case DwarfOperationCode::DW_OP_lit7:
  case DwarfOperationCode::DW_OP_lit8:
  case DwarfOperationCode::DW_OP_lit9:
  case DwarfOperationCode::DW_OP_lit10:
  case DwarfOperationCode::DW_OP_lit11:
  case DwarfOperationCode::DW_OP_lit12:
  case DwarfOperationCode::DW_OP_lit13:
  case DwarfOperationCode::DW_OP_lit14:
  case DwarfOperationCode::DW_OP_lit15:
  case DwarfOperationCode::DW_OP_lit16:
  case DwarfOperationCode::DW_OP_lit17:
  case DwarfOperationCode::DW_OP_lit18:
  case DwarfOperationCode::DW_OP_lit19:
  case DwarfOperationCode::DW_OP_lit20:
  case DwarfOperationCode::DW_OP_lit21:
  case DwarfOperationCode::DW_OP_lit22:
  case DwarfOperationCode::DW_OP_lit23:
  case DwarfOperationCode::DW_OP_lit24:
  case DwarfOperationCode::DW_OP_lit25:
  case DwarfOperationCode::DW_OP_lit26:
  case DwarfOperationCode::DW_OP_lit27:
  case DwarfOperationCode::DW_OP_lit28:
  case DwarfOperationCode::DW_OP_lit29:
  case DwarfOperationCode::DW_OP_lit30:
  case DwarfOperationCode::DW_OP_lit31:
    ret->width = 1;
    ret->value = (static_cast<t_uint64>(s.code) - static_cast<t_uint32>(DwarfOperationCode::DW_OP_lit0));
    break;

  case DwarfOperationCode::DW_OP_const1s:
    ret->is_signed = true;
  case DwarfOperationCode::DW_OP_const1u:
    ret->width = 1;
    ret->value = static_cast<t_uint64>(SectionGetData8(section, section_offset));
    sz = 1;
    break;

  case DwarfOperationCode::DW_OP_const2s:
    ret->is_signed = true;
  case DwarfOperationCode::DW_OP_const2u:
    ret->width = 2;
    ret->value = static_cast<t_uint64>(SectionGetData16(section, section_offset));
    sz = 2;
    break;

  case DwarfOperationCode::DW_OP_const4s:
    ret->is_signed = true;
  case DwarfOperationCode::DW_OP_const4u:
    ret->width = 4;
    ret->value = static_cast<t_uint64>(SectionGetData32(section, section_offset));
    sz = 4;
    break;

  case DwarfOperationCode::DW_OP_const8s:
    ret->is_signed = true;
  case DwarfOperationCode::DW_OP_const8u:
    ret->width = 8;
    ret->value = SectionGetData64(section, section_offset);
    sz = 8;
    break;

  case DwarfOperationCode::DW_OP_consts:
    ret->is_signed = true;
    ret->value = static_cast<t_uint64>(DwarfDecodeSLEB128(DwarfReadSLEB128FromSection(section, section_offset, sz)));
    ret->width = sz;
    ASSERT(sz <= sizeof(t_uint64), ("could not decode SLEB128 in 64-bit integer"));
    break;

  case DwarfOperationCode::DW_OP_constu:
    ret->value = static_cast<t_uint64>(DwarfDecodeULEB128(DwarfReadULEB128FromSection(section, section_offset, sz)));
    ret->width = sz;
    ASSERT(sz <= sizeof(t_uint64), ("could not decode ULEB128 in 64-bit integer"));
    break;

  default:
    FATAL(("unsupported operation %x", s.code));
  }

  /* sign extension if needed */
  if (ret->is_signed
      && (ret->value & 1<<(ret->width * 8 - 1))
      && (ret->width < 8))
  {
    DEBUG(("sign extending from %x", ret->value));
    ret->value |= ~(0ULL) << (ret->width * 8);
    DEBUG(("   to %x", ret->value));

    FATAL(("BOEM"));
  }

  return ret;
}

/* default handler */
static
DwarfAbstractOperation *
DwarfUnhandledOperation(DwarfOperationStruct& s, DwarfCompilationUnitHeader *cu, t_section *section, t_address section_offset, t_uint32& sz)
{
  if (s.code == DwarfOperationCode::DW_OP_nop)
    return new DwarfAbstractOperation();

  FATAL(("unsupported DWARF operation: 0x%x (%s)", s.code, s.name.c_str()));
}

/* given an operation code, get the associated data structure */
static
DwarfOperationStruct
GetOperationInfo(DwarfOperationCode idx)
{
  if (operation_handlers.find(idx) == operation_handlers.end())
    FATAL(("undefined operation 0x%x", idx));

  return operation_handlers[idx];
}

/* install one operation */
static inline
void
AddOperationHandler(DwarfOperationCode idx, string name, dwarf_operation_handler_fn handler)
{
  operation_handlers[idx] = DwarfOperationStruct{idx, name, handler};
}

/* install all supported operations */
void InitOperations()
{
  AddOperationHandler(DwarfOperationCode::DW_OP_addr, "DW_OP_addr", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_deref, "DW_OP_deref", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const1u, "DW_OP_const1u", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const1s, "DW_OP_const1s", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const2u, "DW_OP_const2u", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const2s, "DW_OP_const2s", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const4u, "DW_OP_const4u", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const4s, "DW_OP_const4s", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const8u, "DW_OP_const8u", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_const8s, "DW_OP_const8s", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_constu, "DW_OP_constu", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_consts, "DW_OP_consts", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_dup, "DW_OP_dup", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_drop, "DW_OP_drop", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_over, "DW_OP_over", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_pick, "DW_OP_pick", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_swap, "DW_OP_swap", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_rot, "DW_OP_rot", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_xderef, "DW_OP_xderef", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_abs, "DW_OP_abs", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_and, "DW_OP_and", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_div, "DW_OP_div", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_minus, "DW_OP_minus", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_mod, "DW_OP_mod", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_mul, "DW_OP_mul", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_neg, "DW_OP_neg", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_not, "DW_OP_not", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_or, "DW_OP_or", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_plus, "DW_OP_plus", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_plus_uconst, "DW_OP_plus_uconst", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_shl, "DW_OP_shl", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_shr, "DW_OP_shr", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_shra, "DW_OP_shra", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_xor, "DW_OP_xor", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_skip, "DW_OP_skip", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_bra, "DW_OP_bra", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_eq, "DW_OP_eq", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_ge, "DW_OP_ge", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_gt, "DW_OP_gt", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_le, "DW_OP_le", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lt, "DW_OP_lt", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_ne, "DW_OP_ne", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit0, "DW_OP_lit0", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit1, "DW_OP_lit1", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit2, "DW_OP_lit2", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit3, "DW_OP_lit3", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit4, "DW_OP_lit4", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit5, "DW_OP_lit5", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit6, "DW_OP_lit6", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit7, "DW_OP_lit7", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit8, "DW_OP_lit8", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit9, "DW_OP_lit9", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit10, "DW_OP_lit10", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit11, "DW_OP_lit11", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit12, "DW_OP_lit12", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit13, "DW_OP_lit13", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit14, "DW_OP_lit14", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit15, "DW_OP_lit15", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit16, "DW_OP_lit16", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit17, "DW_OP_lit17", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit18, "DW_OP_lit18", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit19, "DW_OP_lit19", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit20, "DW_OP_lit20", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit21, "DW_OP_lit21", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit22, "DW_OP_lit22", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit23, "DW_OP_lit23", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit24, "DW_OP_lit24", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit25, "DW_OP_lit25", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit26, "DW_OP_lit26", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit27, "DW_OP_lit27", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit28, "DW_OP_lit28", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit29, "DW_OP_lit29", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit30, "DW_OP_lit30", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lit31, "DW_OP_lit31", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg0, "DW_OP_reg0", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg1, "DW_OP_reg1", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg2, "DW_OP_reg2", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg3, "DW_OP_reg3", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg4, "DW_OP_reg4", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg5, "DW_OP_reg5", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg6, "DW_OP_reg6", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg7, "DW_OP_reg7", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg8, "DW_OP_reg8", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg9, "DW_OP_reg9", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg10, "DW_OP_reg10", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg11, "DW_OP_reg11", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg12, "DW_OP_reg12", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg13, "DW_OP_reg13", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg14, "DW_OP_reg14", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg15, "DW_OP_reg15", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg16, "DW_OP_reg16", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg17, "DW_OP_reg17", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg18, "DW_OP_reg18", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg19, "DW_OP_reg19", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg20, "DW_OP_reg20", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg21, "DW_OP_reg21", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg22, "DW_OP_reg22", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg23, "DW_OP_reg23", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg24, "DW_OP_reg24", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg25, "DW_OP_reg25", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg26, "DW_OP_reg26", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg27, "DW_OP_reg27", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg28, "DW_OP_reg28", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg29, "DW_OP_reg29", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg30, "DW_OP_reg30", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_reg31, "DW_OP_reg31", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg0, "DW_OP_breg0", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg1, "DW_OP_breg1", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg2, "DW_OP_breg2", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg3, "DW_OP_breg3", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg4, "DW_OP_breg4", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg5, "DW_OP_breg5", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg6, "DW_OP_breg6", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg7, "DW_OP_breg7", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg8, "DW_OP_breg8", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg9, "DW_OP_breg9", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg10, "DW_OP_breg10", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg11, "DW_OP_breg11", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg12, "DW_OP_breg12", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg13, "DW_OP_breg13", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg14, "DW_OP_breg14", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg15, "DW_OP_breg15", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg16, "DW_OP_breg16", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg17, "DW_OP_breg17", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg18, "DW_OP_breg18", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg19, "DW_OP_breg19", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg20, "DW_OP_breg20", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg21, "DW_OP_breg21", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg22, "DW_OP_breg22", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg23, "DW_OP_breg23", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg24, "DW_OP_breg24", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg25, "DW_OP_breg25", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg26, "DW_OP_breg26", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg27, "DW_OP_breg27", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg28, "DW_OP_breg28", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg29, "DW_OP_breg29", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg30, "DW_OP_breg30", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_breg31, "DW_OP_breg31", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_regx, "DW_OP_regx", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_fbreg, "DW_OP_fbreg", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_bregx, "DW_OP_bregx", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_piece, "DW_OP_piece", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_deref_size, "DW_OP_deref_size", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_xderef_size, "DW_OP_xderef_size", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_nop, "DW_OP_nop", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_push_object_address, "DW_OP_push_object_address", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_call2, "DW_OP_call2", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_call4, "DW_OP_call4", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_call_ref, "DW_OP_call_ref", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_form_tls_address, "DW_OP_form_tls_address", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_call_frame_cfa, "DW_OP_call_frame_cfa", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_bit_piece, "DW_OP_bit_piece", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_implicit_value, "DW_OP_implicit_value", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_stack_value, "DW_OP_stack_value", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_lo_user, "DW_OP_lo_user", DwarfUnhandledOperation);
  AddOperationHandler(DwarfOperationCode::DW_OP_hi_user, "DW_OP_hi_user", DwarfUnhandledOperation);
}
