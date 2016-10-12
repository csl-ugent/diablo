#include "diabloflowgraph.hpp"

void DiabloFlowgraphCppInit (int, char **)
{
  InstallGenericNewTargetHandlers();
}

void DiabloFlowgraphCppFini ()
{
  DestroyGenericNewTargetHandlers();
}
