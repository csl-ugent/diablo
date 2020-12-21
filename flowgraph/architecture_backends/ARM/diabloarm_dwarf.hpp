#ifndef DIABLOARM_DWARF_HPP
#define DIABLOARM_DWARF_HPP

#include <diabloarm.hpp>
#include <flowgraph/debug_backends/dwarf/diabloflowgraph_dwarf.hpp>

void DwarfArmSpecificStuff(t_cfg *cfg);
void DwarfArmFunctionHasInfo(t_function *fun, t_bool *result);
void DwarfArmFunctionFloatArgRetRegsets(DwarfFunctionDefinition *dwarf_function, t_regset *p_args, t_regset *p_rets);

#endif /* DIABLOARM_DWARF_HPP */
