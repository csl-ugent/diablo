#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_EQ_IMMEDIATES_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_EQ_IMMEDIATES_H

int *MapImmediatesToRegisters(SliceSet slices, size_t slice_size, bool *imm_to_reg, int& max_map);
int EqualizeImmediates(SliceSet slices, size_t slice_size, bool *& have_to_equalize_insn);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_EQ_IMMEDIATES_H */
