/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#ifndef I386_OBFUSCATION_INLINEFUNCTION_H
#define I386_OBFUSCATION_INLINEFUNCTION_H

class I386InlineFunctionTransformation : public BBLObfuscationTransformation {
  static constexpr const char* _name = "inline_function";
  long inlined;
public:
  I386InlineFunctionTransformation();
  virtual const char* name() const { return _name; }
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
};

#endif /* I386_OBFUSCATION_INLINEFUNCTION_H */
