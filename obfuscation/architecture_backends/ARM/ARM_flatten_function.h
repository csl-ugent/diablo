/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef ARM_FLATTEN_TRANSFORMATION_H
#define ARM_FLATTEN_TRANSFORMATION_H

#include <obfuscation/generic/flatten_function.h>

class ARMFlattenFunctionTransformation : public FlattenFunctionTransformation {
protected:
  virtual t_bbl*   createSwitchBlock(t_bbl* switch_bbl1, t_section* var_section, register_info* reg_info_);
  virtual void     restoreRegisters(t_bbl* bbl, t_bbl* successor, register_info* reg_info_);
  virtual std::shared_ptr<Successor> redirectSuccessorsCode(t_bbl* bbl, function_info* fun_info);
  virtual t_reloc* writeRelocationInSwitchTable(t_cfg* cfg, t_section* var_section, t_bbl* switch_bbl, t_bbl* target_bbl, int switch_index);

  virtual bool     canRedirectSuccessor(t_bbl* bbl);
  
  virtual void     redirectToSwitchBlock(std::shared_ptr<Successor> successor, t_bbl* switch_bbl);

  
  struct ARMregister_info : public register_info {
    MaybeSaveRegister target_index;
    MaybeSaveRegister index_computation_helper;
    std::vector<MaybeSaveRegister> saves;

    ARMregister_info(const MaybeSaveRegister& target_index, const MaybeSaveRegister& index_computation_helper);
    virtual ~ARMregister_info() {}
  };
  virtual void initRegisterInfo(function_info* info, t_randomnumbergenerator* rng);
  
  struct ARMSuccessor : public Successor {
    t_arm_ins* ins;
    ARMSuccessor(bool code_added, t_cfg* cfg, t_bbl* bbl, t_arm_ins* ins) : Successor(code_added, cfg, bbl), ins(ins) {}
    
    virtual ~ARMSuccessor() {}
  };

public:
  virtual bool     canTransform(const t_function* fun) const;
  ARMFlattenFunctionTransformation(bool multiple_flatten);
};

#endif
