#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_CONSTANT_ANALYSIS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_CONSTANT_ANALYSIS_H

t_procstate *SliceProcstateBefore(Slice *slice, size_t slice_size);
void ProcstateConstantRegisters(t_procstate *procstate, t_regset& constant_registers, t_regset& null_registers, t_regset& tag_registers);
std::string PrintConstants(t_procstate *ps);
void ProcstateSetReg(t_procstate *ps, t_reg reg, t_uint64 constant, bool bottom);
bool ProcstateGetConstantValue(t_procstate *ps, t_reg reg, t_uint64& constant);
bool ProcstateGetTag(t_procstate *ps, t_reg reg, t_reloc *& tag);

void MergeConditionalProcstate(t_cfg_edge *e, t_procstate *new_procstate, t_reg& conditional_register, bool& conditional_register_nonzero);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_CONSTANT_ANALYSIS_H */
