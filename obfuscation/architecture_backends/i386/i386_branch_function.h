/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_OBFUSCATION_TRANSFORMATION_H
#define ARM_OBFUSCATION_TRANSFORMATION_H

#include <map>

#include <obfuscation/generic/branch_function.h>

class I386BranchFunctionTransformation : public BranchFunctionTransformation {
  static constexpr const char* _name = "i386_stackbased_branchfunction";
public:
  I386BranchFunctionTransformation() { RegisterTransformationType(this, _name); }
  virtual const char* name() const { return _name; }
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void transformJumpToCall(t_bbl* bbl, t_randomnumbergenerator* rng);
};

class I386CallFunctionTransformation : public CallFunctionTransformation {
  static constexpr const char* _name = "i386_callfunction";
public:
  I386CallFunctionTransformation() { RegisterTransformationType(this, _name); }
  virtual const char* name() const { return _name; }
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
};


#endif /* ARM_OBFUSCATION_TRANSFORMATION_H */
