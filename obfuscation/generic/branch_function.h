/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef GEN_OBFUSCATION_TRANSFORMATION_H
#define GEN_OBFUSCATION_TRANSFORMATION_H

#include <obfuscation/obfuscation_transformation.h>

class BranchFunctionTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "branchfunction";
public:
  BranchFunctionTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  
  virtual void transformJumpToCall(t_bbl* bbl, t_randomnumbergenerator* rng) = 0;
};

class CallFunctionTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "callfunction";
protected:
  long callsTransformed;
public:
  CallFunctionTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual void dumpStats(const std::string& prefix);

  virtual bool transformationIsAvailableInRandomizedList() { return false; }
};

class SplitCallWithBranchFunctionsTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "splitcallwithbranchfunction";
  long callsTransformed;
  void addBranchFunctionCallToBbl(t_bbl* bbl, t_randomnumbergenerator* rng);
public:
  SplitCallWithBranchFunctionsTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }

  virtual bool transformationIsAvailableInRandomizedList() { return false; }
};

class SplitOffFunctionHeadTransformation : public FunctionObfuscationTransformation {
  static constexpr const char* _name = "splitofffunctionhead";
  long functionstransformed;
public:
  SplitOffFunctionHeadTransformation();
  virtual bool canTransform(const t_function* fun) const;
  virtual bool doTransform(t_function* fun, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }

  virtual bool transformationIsAvailableInRandomizedList() { return false; }
};

extern FILE* L_OBF_BF;

// TODO: we should not SplitCallWithBranchFunctionsTransformation calls to global_var-using return functions

#endif /* OBFUSCATION_TRANSFORMATION_H */
