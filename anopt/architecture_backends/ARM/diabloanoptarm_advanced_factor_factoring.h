#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_FACTORING_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_FACTORING_H

struct SliceSetRegsetTuple {
  SliceSet set;
  t_regset regs;
  int regcount;
  bool *imm_to_reg;
  t_uint32 required_reg_count;
  t_possibility_flags flags;
  SliceSpecificActionList slice_actions;
  SliceSpecificRegisters slice_registers;
  Slice *ref_slice;
  t_gpregisters preferably_dont_touch;
  SliceToGPRegistersMap overwritten_registers_per_slice;
  SliceToAllRegisterInformationMap all_register_information_per_slice;
  int nr_used_constants;
};

struct ImmSliceSetTuple {
  t_uint64 imm;
  SliceSet set;
};

struct SwitchcaseData {
  std::vector<t_reloc *> relocs;
  t_bbl *bbl;
  bool is_dummy;
  t_cfg_edge *landing_edge;
  DummyEdgeData::DispatchType dispatch_type;
};

typedef std::map<t_uint64, SliceSet> I2SliceSetMap;
typedef std::map<t_bbl *, SliceVector> BblToSliceVectorMap;
typedef std::map<Slice *, t_bbl*> SliceToBblMap;

FactoringResult TransformFactoringPossiblity(FactoringPossibility *poss);

t_reg ChooseRegister(std::vector<t_reg>& registers);

size_t GroupImmediatesPerInstruction(std::vector<ImmSliceSetTuple>* data, SliceSet set, size_t slice_size, bool* consider_immediate_for_removal, bool init);

t_possibility_flags DeterminePossibleTransformations(SliceSet& set, size_t slice_size, size_t max_regs_needed, t_regset& dead_through, SliceSpecificRegisters slice_registers, t_gpregisters preferably_dont_touch, int& nr_used_constants);

t_gpregisters PossibleRegistersCmpZero(Slice *a, Slice *b, size_t slice_size, bool use_cached);
t_gpregisters CalculatePreferablyDontTouchRegistersCmpZero(SliceSet slices, size_t slice_size, bool use_cached);

void RedirectRelocations(t_bbl *original, t_bbl *factored);

enum class AFPhase {
  CreateFactoredBlock = 0,
  SliceTransform = 1,
  CreateDispatcher = 2,
  EqualizePredecessors = 3,
  EqualizeSuccessors = 4,
  Fixups = 5,
  Finalization = 6,
  Unknown = -1
};
void AFFactoringLogInstruction(t_arm_ins *ins, std::string type="ADD");
void LogAddedInstructionHeader();

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_FACTORING_H */
