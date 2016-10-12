/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef GEN_FLIP_BRANCHES_TRANSFORMATION_H
#define GEN_FLIP_BRANCHES_TRANSFORMATION_H

#include <obfuscation/obfuscation_transformation.h>

class FlipBranchesTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "flip_branch";
public:
  FlipBranchesTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
};

#endif /* GEN_FLIP_BRANCHES_TRANSFORMATION_H */
