#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_OBFUSCATION_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_OBFUSCATION_H

class AdvancedFactoringSelector : public NewTargetSelector {
  static constexpr t_const_string _name = "newtargetselector:afselector";
public:
  AdvancedFactoringSelector();
  virtual bool canTransform(const t_bbl*) const;
  virtual t_bbl* doTransform(t_bbl* bbl, t_randomnumbergenerator * rng);
  virtual t_bbl* doTransform(t_cfg *cfg, t_randomnumbergenerator * rng);
  virtual t_const_string name() const { return _name; }

private:
  t_bbl *Common(t_cfg *cfg);
};

void AFObfuscationInit(t_cfg *cfg);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_OBFUSCATION_H */
