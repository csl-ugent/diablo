#ifndef DIABLOSUPPORT_DYNLIB_HPP
#define DIABLOSUPPORT_DYNLIB_HPP
#include <string>

void InitPluginSearchDirectory(std::string dirname);
void CloseAllLibraries();

/* don't use this function directly */
void *LoadSymbolFromLibrary(std::string library, std::string name);

/* but use this one instead */
template<typename T>
T LoadSharedFunction(std::string library, std::string name) {
  return reinterpret_cast<T>(LoadSymbolFromLibrary(library, name));
}

#endif /* DIABLOSUPPORT_DYNLIB_HPP */
