#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_EQ_REGISTERS_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_EQ_REGISTERS_H

/* Generate a list of actions to be applied to each slice so they are made equal. */
bool EqualizeRegistersPrepare(SliceSet slices, size_t slice_size, t_gpregisters preferably_dont_touch, Slice *& ref_slice);
bool EqualizeRegisters(SliceSet slices, size_t slice_size, SliceSpecificActionList& action_list, SliceSpecificRegisters& slice_registers, Slice *ref_slice, t_gpregisters& preferably_dont_touch, SliceToGPRegistersMap& overwritten_registers_per_slice, SliceToAllRegisterInformationMap& all_register_information_per_slice);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_EQ_REGISTERS_H */
