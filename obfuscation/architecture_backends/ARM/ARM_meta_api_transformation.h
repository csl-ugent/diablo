#ifndef ARM_META_API_TRANSFORMATION_H
#define ARM_META_API_TRANSFORMATION_H

class ARMMetaApiTransformation : public BBLObfuscationTransformation {
private:
  static constexpr const char* _name = "meta_api";

  t_int64 min_sequence_id;
  void initialise(t_cfg *cfg);
  bool canTransformBbl(t_bbl* bbl) const;

  std::vector<t_function *> functions_to_finalize;

public:
  /* constructor/destructor */
  ARMMetaApiTransformation();
  ~ARMMetaApiTransformation();

  /* obligatory overrides */
  virtual bool canTransform(const t_bbl* bbl) const;
  virtual bool doTransform(t_bbl* bbl, t_randomnumbergenerator *rng);
  virtual const char* name() const { return _name; }

  virtual void finalizeAll() const;

  virtual std::string statusToString(int status_code);
  virtual int statusCode();

  bool testing;
  MetaAPI_Test test_configuration;
  std::string test_name;
  t_cfg *test_cfg;
};

void MetaAPI_PerformTest(t_cfg *cfg, t_string test_name);

extern bool meta_api_testing;

#endif /* ARM_META_API_TRANSFORMATION_H */
