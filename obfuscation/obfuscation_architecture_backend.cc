/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include "obfuscation_architecture_backend.h"

using namespace std;

static ObfuscationArchitectureInitializer* global_obfuscation_architecture = 0;

ObfuscationArchitectureInitializer::ObfuscationArchitectureInitializer() {
  ASSERT(!global_obfuscation_architecture, ("Only one obfuscation architecture supported per diablo obfuscation binary"));

  global_obfuscation_architecture = this;
}

ObfuscationArchitectureInitializer* GetObfuscationArchitectureInitializer() {
  ASSERT(global_obfuscation_architecture, ("No obfuscation architecture set!"));
  
  return global_obfuscation_architecture;
}
