/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef DIABLOOBJECT_DWARF_TAGS_H
#define DIABLOOBJECT_DWARF_TAGS_H

#include <string>

std::string TagToString(DwarfDecodedULEB128 tag);
void InitTags();

#endif /* DIABLOOBJECT_DWARF_TAGS_H */
