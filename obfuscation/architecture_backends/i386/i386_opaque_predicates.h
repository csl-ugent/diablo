/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef I386_OPAQUE_PREDICATES_TRANSFORMATION_H
#define I386_OPAQUE_PREDICATES_TRANSFORMATION_H

#include <obfuscation/generic/opaque_predicate.h>

/* TODO: perhaps split these up like the ARM ones, so we can individually select the predicates we want to insert */
class I386OpaquePredicateTransformation : public OpaquePredicateTransformation {
  long bblsTransformed;
public:
  I386OpaquePredicateTransformation();

  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
};


#endif /* I386_OPAQUE_PREDICATES_TRANSFORMATION_H */
