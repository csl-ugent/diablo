#include <diabloflowgraph.h>

t_bool I386InlineFunAtCallSite(t_function * fun, t_bbl * callsite);
void I386InlineSmallFunctions(t_cfg * cfg);
void I386InlineFunctionsWithOneCallSite(t_cfg * cfg);
t_bool I386CheckInlinable(t_function * fun);
/* vim: set shiftwidth=2 foldmethod=marker : */
