#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_DISPATCH_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_DISPATCH_H

#define CONDITIONAL_DISPATCHER_LITERAL 1
#define CONDITIONAL_DISPATCHER_SECTIONS_01 0
#define CONDITIONAL_DISPATCHER_HEAP 0
#define CONDITIONAL_DISPATCHER_CONSTANT_PROPAGATION 0

#define CONDITIONAL_JUMP_DISPATCHER_JUMP_EDGE 1
#define CONDITIONAL_JUMP_DISPATCHER_FALLTHROUGH_EDGE 0

struct DispatcherResult {
  t_regset used_registers;
  AddedInstructionInfo added_ins_info;

  DispatcherResult() {
    used_registers = NullRegs;
    added_ins_info = AddedInstructionInfo();
  }
};

enum class DispatcherType {
  IndirectBranch,
  InternalConditionalJump,
  ConditionalJump,
  SwitchOffset,
  SwitchBranch,
  DistributedTable,

  Invalid = 0xf
};

#define F_DispatchGeneratorArguments std::vector<t_reg>& usable_registers, std::vector<FactoredPath>& factored_paths, t_bbl *factored_bbl, std::vector<SwitchcaseData>& switchcase_data, t_bbl *& factored_bbl_tail, t_gpregisters preferably_dont_touch, size_t slice_size, BblToSliceVectorMap bbl_to_slicevector_map, SliceToBblMap slice_to_bbl_map, t_reg& dispatch_register, t_possibility_flags flags

typedef std::function<DispatcherResult(F_DispatchGeneratorArguments)> F_DispatchGenerator;

bool ChooseInternalConditionalJumpDispatcherRegister(SliceSet slices, size_t slice_size, t_gpregisters preferably_dont_touch, bool use_cached,
                                                      Slice *& null_slice, t_reg& subject, int& selected_zero_index, int& selected_nonzero_index, int& nr_internal_constants);

/* !!! make sure that the switch statement and its data table are chained, in diabloarm_layout.c */
DispatcherResult ApplyConditionalJumpDispatcher(F_DispatchGeneratorArguments);
DispatcherResult ApplyIndirectBranchDispatcher(F_DispatchGeneratorArguments);
DispatcherResult ApplySwitchDispatcherLdrAdd(F_DispatchGeneratorArguments);
DispatcherResult ApplySwitchDispatcherBranchTable(F_DispatchGeneratorArguments);
DispatcherResult ApplyDistributedTable(F_DispatchGeneratorArguments);
DispatcherResult ApplyInternalConditionalJumpDispatcher(F_DispatchGeneratorArguments);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_DISPATCH_H */
