#include <diabloi386.hpp>

void DiabloI386CppInit(int, char **)
{
  SetArchitectureInfoWrapper(new I386ArchitectureInfo());
}

void DiabloI386CppFini()
{
  delete (I386ArchitectureInfo *)GetArchitectureInfoWrapper();
}
