/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef GEN_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H
#define GEN_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H

class SplitWithTwoWayPredicateTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "splittwoway";
  t_bbl* unfoldBblByEdge(t_bbl* bblOrig, t_cfg_edge * edgeToMove);
  long bbls_split;
protected:
  virtual bool canTwowaySplitBasicBlock(const t_bbl* bbl) const;
  virtual bool bblSplitWithTwoWayOpaquePredicate(t_bbl* bbl, t_randomnumbergenerator* rng) = 0; /* Splits randomly and then append to the original bbl, rather than necessarily prepend */
public:
  SplitWithTwoWayPredicateTransformation();
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }

  virtual bool transformationIsAvailableInRandomizedList() { return false; }
};

#endif /* GEN_SPLIT_TWOWAY_PREDICATE_TRANSFORMATION_H */
