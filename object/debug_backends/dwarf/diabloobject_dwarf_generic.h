/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_GENERIC_H
#define DIABLOOBJECT_DWARF_GENERIC_H

#include <vector>
#include <string>

enum class DwarfChildEncoding {
  DW_CHILDREN_no = 0x00,
  DW_CHILDREN_yes = 0x01
};

enum class DwarfInlineCode {
  /* declaration: no inline
   * compiler   : no inline */
  DW_INL_not_inlined = 0x00,

  /* declaration: no inline
   * compiler   : inline */
  DW_INL_inlined = 0x01,

  /* declaration: inline
   * compiler   : no inline */
  DW_INL_declared_not_inlined = 0x02,

  /* declaration: inline
   * compiler   : inline */
  DW_INL_declared_inlined = 0x03
};
std::string DwarfInlineCodeToString(DwarfInlineCode c);

enum class DwarfLanguageCode {
  DW_LANG_C_plus_plus = 0x0004,
  DW_LANG_C99 = 0x000c,
  DW_LANG_Mips_Assembler = 0x8001,
};
std::string DwarfLanguageCodeToString(DwarfLanguageCode c);

struct DwarfDebugInformationEntry {
  t_address offset;

  std::vector<DwarfDebugInformationEntry *> children;

  virtual ~DwarfDebugInformationEntry() {
    /* children */
    for (auto e : children)
      delete e;
  }
};

#endif /* DIABLOOBJECT_DWARF_GENERIC_H */
