#ifndef DIABLOFLOWGRAPH_DWARF_PARSE_H
#define DIABLOFLOWGRAPH_DWARF_PARSE_H

void FindFunctionDeclarations(DwarfAbbrevTableEntry *entry);
std::map<t_address, DwarfFunctionDefinition *> DwarfParseInformation(t_cfg *cfg);
bool FunctionHasNoDwarf(t_function *fun);

#endif
