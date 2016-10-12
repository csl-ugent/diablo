#include <diabloflowgraph.h>

t_bool Amd64InlineFunAtCallSite(t_function * fun, t_bbl * callsite);
void Amd64InlineSmallFunctions(t_cfg * cfg);
void Amd64InlineFunctionsWithOneCallSite(t_cfg * cfg);
/* vim: set shiftwidth=2 foldmethod=marker : */
