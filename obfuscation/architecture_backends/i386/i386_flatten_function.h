/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef I386_FLATTEN_TRANSFORMATION_H
#define I386_FLATTEN_TRANSFORMATION_H

#include <obfuscation/generic/flatten_function.h>

class I386FlattenFunctionTransformation : public FlattenFunctionTransformation {
protected:
  virtual t_bbl*   createSwitchBlock(t_bbl* switch_bbl1, t_section* var_section, register_info* reg_info_);
  virtual void     restoreRegisters(t_bbl* bbl, t_bbl* successor, register_info* reg_info_);
  virtual std::shared_ptr<Successor> redirectSuccessorsCode(t_bbl* bbl, function_info* fun_info);
  virtual t_reloc* writeRelocationInSwitchTable(t_cfg* cfg, t_section* var_section, t_bbl* switch_bbl, t_bbl* target_bbl, int switch_index);
  virtual bool     canRedirectSuccessor(t_bbl* bbl);
  virtual void     initRegisterInfo(FlattenFunctionTransformation::function_info* info, t_randomnumbergenerator* rng) { info->reg_info = 0; } // TODO use & initialize reg_info in the x86 backend
  virtual void     redirectToSwitchBlock(std::shared_ptr<Successor> successor, t_bbl* switch_bbl);
  struct I386Successor : public Successor {
    t_i386_ins* ins;
    t_bbl* ft_block;
    I386Successor(bool code_added, t_cfg* cfg, t_bbl* bbl, t_i386_ins* ins, t_bbl* ft_block) : Successor(code_added, cfg, bbl), ins(ins), ft_block(ft_block) {}
    
    virtual ~I386Successor() {}
  };
public:
  virtual bool     canTransform(const t_function* fun) const;
  I386FlattenFunctionTransformation(bool multiple_flatten) : FlattenFunctionTransformation(multiple_flatten) {}
};

#endif
