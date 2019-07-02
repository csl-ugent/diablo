/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_BRANCH_FUNCTION_H
#define ARM_BRANCH_FUNCTION_H

#include <obfuscation/generic/branch_function.h>

class AbstractARMRegisterBasedBranchFunctionTransformation : public BranchFunctionTransformation {
  std::map< std::pair<t_bool, t_reg> , t_bbl*> reg_stackbranchfunction_map;
  t_regset possibleRegisters;
protected:
  virtual t_bbl* getStackBranchFunction(t_cfg* cfg, t_bool isThumb, t_reg reg);
  virtual t_bbl* createStackBranchFunctionForRegister(t_cfg* cfg, t_bool isThumb, t_reg reg) = 0;
public:
  AbstractARMRegisterBasedBranchFunctionTransformation();
};

class ARMRegisterBasedBranchFunctionTransformation : public AbstractARMRegisterBasedBranchFunctionTransformation {
  static constexpr const char* _name = "arm_registerbased_branchfunction";
  long possibleSplitPoints; 
  long bblsTransformed;
  long insts_in_bbls;
protected:
  virtual t_bbl* createStackBranchFunctionForRegister(t_cfg* cfg, t_bool isThumb, t_reg reg);
public:
  ARMRegisterBasedBranchFunctionTransformation();
  virtual const char* name() const { return _name; }
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void transformJumpToCall(t_bbl* bbl, t_randomnumbergenerator* rng);
  virtual void dumpStats(const std::string& prefix);
};

struct ARMSplitCallWithBranchFunctionsTransformation : public SplitCallWithBranchFunctionsTransformation {
  ARMSplitCallWithBranchFunctionsTransformation();
};

class ARMCallFunctionTransformation : public CallFunctionTransformation {
  static constexpr const char* _name = "arm_callfunction";
  t_function* getRetFunForReg(t_cfg* cfg, t_bool isThumb, t_reg reg);
  t_cfg_edge* getCalleeEdge(t_bbl* bbl);
  t_bbl* jumpAndPopStubFor(t_cfg* cfg, t_bool isThumb, t_reg reg, t_bbl* target);
  void transformCallIntoRetFunction(t_bbl * bbl_1, t_randomnumbergenerator* rng);
public:
  ARMCallFunctionTransformation();
  virtual const char* name() const { return _name; }
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
};


#endif /* ARM_BRANCH_FUNCTION_H */
