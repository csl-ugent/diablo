/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOARM_TRANSFORMATION_HPP
#define DIABLOARM_TRANSFORMATION_HPP

#include <vector>

#include <diabloarm.hpp>

class ARMArchitectureInfo : public ArchitectureInfo {
  t_bool isThumb;
public:
  ARMArchitectureInfo(t_bool isThumb);
  virtual ~ARMArchitectureInfo() {}

  virtual t_reg getGenericRandomRegister(t_randomnumbergenerator* rng);
  virtual void saveOrRestoreRegisters(const std::vector<MaybeSaveRegister>& regs, t_bbl* bbl, bool push, bool prepend=false);

  /* TODO: this is because of the hacky prepend=true above that needs to be fixed generically */
  virtual void popAppendRegisters(const std::vector<MaybeSaveRegister>& regs, t_bbl* bbl);

  virtual void appendUnconditionalBranchInstruction(t_bbl* bbl);
};

class ARMArchitectureInfoWrapper : public ArchitectureInfoWrapper {
  ARMArchitectureInfo* thumbWrapper;
  ARMArchitectureInfo* armWrapper;
public:
  ARMArchitectureInfoWrapper();
  virtual ~ARMArchitectureInfoWrapper();
  virtual ArchitectureInfo* getArchitectureInfo(t_bbl* bbl_for);
};

#endif /* DIABLOARM_TRANSFORMATION_HPP */
