/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_FLIP_BRANCHES_TRANSFORMATION_H
#define ARM_FLIP_BRANCHES_TRANSFORMATION_H

#include <obfuscation/generic/flip_branches.h>

class I386FlipBranchesTransformation : public FlipBranchesTransformation {
  long bblsTransformed;
public:
  I386FlipBranchesTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
};

#endif /* ARM_FLIP_BRANCHES_TRANSFORMATION_H */
