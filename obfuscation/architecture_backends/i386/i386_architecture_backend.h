/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef I386_ARCHITECTURE_BACKEND_H
#define I386_ARCHITECTURE_BACKEND_H

struct I386ObfuscationArchitectureInitializer : public ObfuscationArchitectureInitializer {
  I386ObfuscationArchitectureInitializer();
  virtual void Init(t_uint32 argc, char **argv);
};

t_function* GetReturnAddressStubForRegister(t_cfg* cfg, t_reg reg);

#endif /* I386_ARCHITECTURE_BACKEND_H */
