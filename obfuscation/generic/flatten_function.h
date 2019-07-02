/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef GEN_FLATTEN_TRANSFORMATION_H
#define GEN_FLATTEN_TRANSFORMATION_H

#include <memory>

class FlattenFunctionTransformation : public FunctionObfuscationTransformation {
  const char* _name;
  static constexpr const char* _name_generic = "flattenfunction";
  static constexpr const char* _name_multiple = "flattenfunction:multiple";
  static constexpr const char* _name_nomultiple = "flattenfunction:nomultiple";
  t_bool flattenFunctionBasicDiversity(t_function * fun, bool more_switch, const std::set<t_bbl*>& bblSet);

  void flattenBasicBlockSet(const std::vector<t_bbl*>& bblSet, t_randomnumbergenerator* rng);

  std::map<t_bbl*, int> bbl_to_index;
  int functions_transformed;
  int bbls_transformed;
  int insts_in_bbls;
  bool multiple_flatten;
protected:
  /* Currently, a function with multiple switch tables will have the same mapping. The problems with the obvious randomization strategies
   * is that they would leak information to an attacker, it seems. TODO */

  /* Contains info on which registers to use for which purpose, whether to spill them, etc */
  struct register_info {
    virtual ~register_info() {}
  };

  struct function_info {
    std::map<t_bbl*, t_uint32> bblToId;
    BblSet transformSuccessorsSet;
    BblSet transformPredecessorsSet;

    register_info* reg_info;
  };
  std::map<t_function*, function_info> function_info_map; // Can be useful when we want 1 switch block for multiple functions
  
  struct Successor {
    Successor(bool code_added, t_cfg* cfg, t_bbl* bbl) : code_added(code_added), cfg(cfg), bbl(bbl) {}
    
    bool code_added; /* Returns true if code was added, false if none was added (ie returns) */
    t_cfg* cfg;
    t_bbl* bbl;

    virtual ~Successor() {}
  };
  
  
  virtual void initRegisterInfo(function_info* info, t_randomnumbergenerator* rng) = 0;
  
  virtual void     fillEntriesInSwitchTable(function_info& info, t_cfg* cfg, t_bbl* switch_bbl, t_section* var_section);

  virtual t_bbl*   createSwitchBlock(t_bbl* switch_bbl1, t_section* var_section, register_info* reg_info_) = 0;
  virtual t_bbl*   createAdditionalSwitchBlock(t_function* function); // For functions where we produce multiple switch blocks. Requires a function_info_map entry!
  virtual void     restoreRegisters(t_bbl* bbl, t_bbl* successor, register_info* reg_info_) = 0;
  virtual std::shared_ptr<Successor> redirectSuccessorsCode(t_bbl* bbl, function_info* fun_info) = 0;
  virtual t_reloc* writeRelocationInSwitchTable(t_cfg* cfg, t_section* var_section, t_bbl* switch_bbl, t_bbl* target_bbl, int switch_index) = 0;

  virtual void     redirectToSwitchBlock(std::shared_ptr<Successor> successor, t_bbl* switch_bbl);

  void             flattenWithCompatibleSwitchBlockSizes(const std::vector<AnnotatedBbl>& bbls, t_randomnumbergenerator* rng);
  virtual bool     canRedirectSuccessor(t_bbl* bbl);
  virtual bool     canRedirectPredecessor(t_bbl* bbl);
  virtual t_int32  countBasicBlocks(t_function * fun);
public:
  FlattenFunctionTransformation(bool multiple_flatten);
  virtual bool canTransform(const t_function* fun) const = 0;
  virtual bool doTransform(t_function* fun, t_randomnumbergenerator * rng);
  virtual void dumpStats(const std::string& prefix);
  virtual const char* name() const { return _name; }

  virtual bool canTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls);
  virtual void doTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls, t_randomnumbergenerator * rng);
};

extern LogFile* L_OBF_FF;

#endif /* FLATTEN_TRANSFORMATION_H */
