#include <diabloarm.hpp>

void DiabloArmCppInit(int, char **)
{
  SetArchitectureInfoWrapper(new ARMArchitectureInfoWrapper());
}

void DiabloArmCppFini()
{
  delete (ARMArchitectureInfoWrapper *)GetArchitectureInfoWrapper();
}
