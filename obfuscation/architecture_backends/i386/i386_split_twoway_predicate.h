/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H
#define ARM_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H

#include <obfuscation/generic/split_twoway_predicate.h>

class I386SplitWithTwoWayPredicateTransformation : public SplitWithTwoWayPredicateTransformation {
  long possibleSplitPoints, possibleFlagIns, reusedFlagIns, addedCmp;
protected:
  virtual bool canTwowaySplitBasicBlock(const t_bbl* bbl) const;
  virtual bool bblSplitWithTwoWayOpaquePredicate(t_bbl* bbl, t_randomnumbergenerator* rng);  /* Splits randomly and then append to the original bbl, rather than necessarily prepend */
public:
  I386SplitWithTwoWayPredicateTransformation();
  
  virtual void dumpStats(const std::string& prefix);
};

#endif /* ARM_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H */
