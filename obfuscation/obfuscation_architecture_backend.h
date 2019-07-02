/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef OBFUSCATION_ARCHITECTURE_BACKEND_H
#define OBFUSCATION_ARCHITECTURE_BACKEND_H

#include <vector>

extern "C" {
#include <diablosupport.h>
}
#include <diabloflowgraph.hpp>

#include "obfuscation_transformation.h"

struct ObfuscationArchitectureInitializer {
  ObfuscationArchitectureInitializer();
  virtual void Init(t_uint32 argc, char **argv) {}
  virtual ~ObfuscationArchitectureInitializer() {}
};

ObfuscationArchitectureInitializer* GetObfuscationArchitectureInitializer();

void InitArchitecture(void **);
void DestroyArchitecture(void **);

extern bool flatten_always_enabled;

#endif /* OBFUSCATION_ARCHITECTURE_BACKEND_H*/
