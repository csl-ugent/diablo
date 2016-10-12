/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOI386_TRANSFORMATION_HPP
#define DIABLOI386_TRANSFORMATION_HPP

#include <diabloi386.hpp>

struct I386ArchitectureInfo : public ArchitectureInfoWrapper, ArchitectureInfo {
  I386ArchitectureInfo();
  virtual ~I386ArchitectureInfo() {}

  virtual t_reg getGenericRandomRegister(t_randomnumbergenerator* rng);
  virtual void saveOrRestoreRegisters(const std::vector<MaybeSaveRegister>& regs, t_bbl* bbl, bool push, bool prepend=false);

  virtual void appendUnconditionalBranchInstruction(t_bbl* bbl);

  virtual ArchitectureInfo* getArchitectureInfo(t_bbl*) { return this; }

  virtual bool functionReturnsPC(const t_function* fun);
};

#endif /* DIABLOI386_TRANSFORMATION_HPP */
