#ifndef DIABLOOBJECT_HPP
#define DIABLOOBJECT_HPP

extern "C" {
#include "diabloobject.h"
}

struct DyncallFunctionInfo {
  std::string name;
  std::string library;
  std::string version;
  size_t index;
};

struct ExportedFunctionInfo {
  std::string name;
  std::string version;
  size_t index;
};

#define SYMBOL_VERSION_NODATA -1
#define SYMBOL_VERSION_GLOBAL -2

#endif
