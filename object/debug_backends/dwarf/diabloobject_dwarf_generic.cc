#include "diabloobject_dwarf.h"

#include <string>

using namespace std;

string DwarfInlineCodeToString(DwarfInlineCode c) {
  string result;

  switch(c) {
  case DwarfInlineCode::DW_INL_not_inlined:
    result = "never";
    break;
  case DwarfInlineCode::DW_INL_inlined:
    result = "compile";
    break;
  case DwarfInlineCode::DW_INL_declared_not_inlined:
    result = "declare";
    break;
  case DwarfInlineCode::DW_INL_declared_inlined:
    result = "always";
    break;
  default:
    FATAL(("unsupported code %d", static_cast<t_uint32>(c)));
  }

  return result;
}

string DwarfLanguageCodeToString(DwarfLanguageCode c) {
  string result;

  switch (c) {
  case DwarfLanguageCode::DW_LANG_C_plus_plus:
    result = "C++";
    break;
  case DwarfLanguageCode::DW_LANG_C99:
    result = "C99";
    break;
  case DwarfLanguageCode::DW_LANG_Mips_Assembler:
    result = "MIPS Assembler";
    break;
  default:
    FATAL(("unsupported code 0x%x", static_cast<t_uint32>(c)));
  }

  return result;
}
