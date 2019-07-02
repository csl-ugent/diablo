#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_PRIORITY_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_PRIORITY_H

enum class FactoringEqualizeStrategy
{
  Identical,
  EqualizeImmediates
};

enum class FactoringBranchStrategy
{
  IndirectBranch,       /* 0 */
  SwitchTableBranches,  /* 1 */
  SwitchTableOffsets,   /* 2 */
  DistributedTable,     /* 3 */

  Invalid
};

typedef std::function<void(SliceSet, size_t, bool)> F_ProcessSubset;

typedef std::map<Slice *, AFActionList> SliceSpecificActionList;
typedef std::pair<Slice *, AFActionList> SliceSpecificActionListItem;

#define REGISTER_ORIGIN_NONE  0x0
#define REGISTER_ORIGIN_INPUT 0x1
#define REGISTER_ORIGIN_SLICE 0x2
#define REGISTER_INFO_UNMODIFIED 0xff
typedef t_uint8 t_register_info;
typedef std::array<t_register_info, 15> t_all_register_info;
typedef std::map<Slice *, t_all_register_info> SliceToAllRegisterInformationMap;

typedef std::map<Slice *, t_regset> SliceSpecificRegisters;
typedef std::pair<Slice *, t_regset> SliceSpecificRegistersItem;

struct PossibilityCmp {
  bool operator() (const FactoringPossibility* lhs, const FactoringPossibility* rhs) const;
};
typedef std::multiset<FactoringPossibility *, PossibilityCmp> FactoringPossibilitiesList;
typedef FactoringPossibilitiesList::iterator FactoringPossibilitiesListIterator;

struct FactoringPossibility
{
  /* function to process any created subset */
  F_ProcessSubset ProcessSubset;

  /* set of slices to be factored */
  SliceSet set;

  /* actions to be done on every predecessor, and on the final factored BBL */
  AFActionList actions_on_predecessors;
  AFActionList actions_on_factored;

  /* the strategy used by this transformation */
  FactoringEqualizeStrategy equalize_strategy;

  /* registers that can be used by the factoring transformation */
  t_regset usable_regs;

  /* score associated with this transformation */
  t_score score;

  /* size of slices */
  size_t slice_size;

  int nr_imm_to_reg;

  /* flags for possible dispatchers */
  t_possibility_flags flags;

  SliceSpecificRegisters slice_registers;
  SliceSpecificActionList slice_actions;

  Slice *ref_slice;

  t_gpregisters preferably_dont_touch;
  t_gpregisters preferably_dont_touch_but_touched;
  SliceToGPRegistersMap overwritten_registers_per_slice;
  SliceToAllRegisterInformationMap all_register_information_per_slice;

  FactoringPossibilitiesListIterator it;

  int nr_used_constants;
  bool possible;

  t_uint32 random_uid;
};

struct TransformedSliceInformation {
  t_gpregisters overwritten_registers;

  /* assume the register index only needs 4 bit */
  t_all_register_info register_info;
};

void LogFactoringPossibilities();
void AddPossibilityToGlobalList(FactoringPossibility *poss);
t_score CalculateScoreForPossiblity(FactoringPossibility *poss);
FactoringPossibility *ChoosePossibility();
void RecordFactoredPossibility(FactoringPossibility *poss, FactoringResult insn_stats, bool transformed);
void PrintPossiblities();
void PrintPriorityListDebugInfo();
size_t PriorityListSize();

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_PRIORITY_H */
