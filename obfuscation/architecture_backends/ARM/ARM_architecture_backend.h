/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_ARCHITECTURE_BACKEND_H
#define ARM_ARCHITECTURE_BACKEND_H

#include <obfuscation/obfuscation_architecture_backend.h>

#include <vector>

struct ARMObfuscationArchitectureInitializer : public ObfuscationArchitectureInitializer {
  ARMObfuscationArchitectureInitializer();
  ~ARMObfuscationArchitectureInitializer();
  virtual void Init(t_uint32 argc, char **argv);

private:
  std::vector<Transformation *> created_obfuscations;
};

#endif /* ARM_ARCHITECTURE_BACKEND_H */
