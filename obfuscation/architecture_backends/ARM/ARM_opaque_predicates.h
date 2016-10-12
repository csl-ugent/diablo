/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_OPAQUE_PREDICATES_H
#define ARM_OPAQUE_PREDICATES_H

#include <map>
#include <set>
#include <vector>

#include <obfuscation/generic/opaque_predicate.h>

class ARMOpaquePredicateTransformation : public OpaquePredicateTransformation {
  std::set<t_uint32> programConstants;
  void initConstantTable(t_cfg* cfg);
protected:
  long possibleSplitPoints;
  long bblsTransformed;
  long insts_in_bbls;
  virtual t_bbl* splitBasicBlockWhereFlagsAreDeadBlockWithJump(t_bbl* bbl, t_randomnumbergenerator* rng);
  /* Return a (random) constant <= the le argument; but preferably one that already occurs in the program elsewhere */
  virtual t_uint32 getRandomProgramConstantLE(t_cfg* cfg, t_uint32 le, t_randomnumbergenerator* rng, bool consecutive_mask=false);

  void logBegin(t_bbl* bbl);
  void logEnd(t_bbl* bbl);
public:
  ARMOpaquePredicateTransformation();
  
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual void dumpStats(const std::string& prefix);
};

#define SIMPLE_ARM_PREDICATE(classname, identifier) \
  class classname : public ARMOpaquePredicateTransformation { \
    static constexpr const char* _name = identifier ; \
  public: \
    classname() { RegisterTransformationType(this, _name); } \
    virtual const char* name() const { return _name; } \
    virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng); \
  }

SIMPLE_ARM_PREDICATE(ARMXX2OpaquePredicateTransformation, "arm_opaque_predicate_x+x^2");
SIMPLE_ARM_PREDICATE(ARM7Y2OpaquePredicateTransformation, "arm_opaque_predicate_7y^2-1!=x^2");
SIMPLE_ARM_PREDICATE(ARM2DivXXOpaquePredicateTransformation, "arm_opaque_predicate_2|x+x");
SIMPLE_ARM_PREDICATE(ARMX2GE0OpaquePredicateTransformation, "arm_opaque_predicate_x^2>=0");
SIMPLE_ARM_PREDICATE(ARMX2DivX22OpaquePredicateTransformation, "arm_opaque_predicate_2|x^2div2");
SIMPLE_ARM_PREDICATE(ARMX2DivXv8DivX21OpaquePredicateTransformation, "arm_opaque_predicate_2|x_v_8|(x^2-1)");
SIMPLE_ARM_PREDICATE(ARM3DivX3_3OpaquePredicateTransformation, "arm_opaque_predicate_3|(x^3-x)");
SIMPLE_ARM_PREDICATE(ARMArboitPredicateSumOpaquePredicateTransformation, "arm_opaque_predicate_arboit_sum");

#endif /* ARM_OPAQUE_PREDICATES_H */
