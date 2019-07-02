#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

/* only one should be selected here */
//#define SLICE_IS_ENTIRE_BBL
//#define SLICE_IS_REAL_SLICE
#define SLICE_IS_DEFS

INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(mark);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(slice_id);
INS_DYNAMIC_MEMBER_GLOBAL_ARRAY(slice);

enum class FindStatus {
  StatusSetter,
  EndOfBbl,
  ConflictingDefinition,
  OK
};

t_uint32 SlicePathCount(Slice *slice)
{
  /* number of factored code paths */
  t_uint32 nr_factored_paths = 0;
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(slice->Bbl(), e)
    nr_factored_paths++;

  return nr_factored_paths;
}

void InsInvalidateSlices(t_ins *ins)
{
  if (!af_dynamic_member_init || !INS_SLICES(ins))
    return;

  while (INS_SLICES(ins)->size() > 0)
  {
    auto slice = *(INS_SLICES(ins)->begin());
    slice->Invalidate();
    INS_SLICES(ins)->erase(slice);
  }
}
int Slice::Id() const {
  return INS_SLICE_ID(base_instruction);
}

Slice::~Slice() {
  //DEBUG(("destroying slice %p", this));

  if (af_dynamic_member_init)
    Invalidate();

  if (procstate_before)
    ProcStateFree(procstate_before);
}

/* find a 'condition'al instruction defining reg 'reg', starting from instruction 'start' */
static
FindStatus find_conditional_instruction_defining_reg(t_ins *start, t_reg reg, t_arm_condition_code condition, t_ins*& result)
{
  t_ins *current_ins = start;

  auto fn_ins_is_conditional = BBL_DESCRIPTION(INS_BBL(start))->InsIsConditional;

  result = NULL;

  /* iterate through an entire BBL; stop at the beginning */
  while (current_ins)
  {
        /* if we're looking for an unconditional instruction */
    if (condition == ARM_CONDITION_AL
        /* OR */
        /* if we're looking for a conditional instruction */
        || (condition != ARM_CONDITION_AL
            /* AND the condition codes are equal */
            && fn_ins_is_conditional(current_ins)
            && ARM_INS_CONDITION(T_ARM_INS(current_ins)) == condition))
    {
      /* check whether the register is defined by this instruction */
      t_reg reg_it;
      REGSET_FOREACH_REG(INS_REGS_DEF(current_ins), reg_it)
        if (reg_it == reg)
        {
          result = current_ins;
          return FindStatus::OK;
        }
    }

    /* condition flags are reset */
    if (condition != ARM_CONDITION_AL
        && ARM_INS_FLAGS(T_ARM_INS(current_ins)) & FL_S)
    {
      result = current_ins;
      return FindStatus::StatusSetter;
    }

    /* iterate to the next instruction */
    current_ins = INS_IPREV(current_ins);
  }

  /* if we land here, no instruction was found */
  result = NULL;
  return FindStatus::EndOfBbl;
}

/* find the 'condition'al instruction defining 'reg', starting from 'start' onwards */
static
t_ins *find_ins_defining_reg(t_ins *start, t_reg reg, t_arm_condition_code condition)
{
  t_ins *defining_ins = NULL;
  auto result = find_conditional_instruction_defining_reg(start, reg, condition, defining_ins);

  switch(result)
  {
    case FindStatus::OK:
      return defining_ins;

    case FindStatus::ConflictingDefinition:
      FATAL(("confliciting definition for @I! @I", start, defining_ins));

    case FindStatus::EndOfBbl:
      return NULL;

    case FindStatus::StatusSetter:
      return find_ins_defining_reg(defining_ins, reg, ARM_CONDITION_AL);
  }

  FATAL(("should not come here!"));
}

void Slice::AddInstruction(t_ins *ins) {
  elements.push_back(ins);
  AssociateWithIns(ins);

  if (is_sequence) {
    int order = INS_ORDER(ins);

    if (order < min_order)
      min_order = order;
    if (order > max_order)
      max_order = order;
  }
}

void Slice::FillWithDefs(t_ins *ins)
{
  AddInstruction(ins);
  base_instruction = ins;
  AssociateWithIns(ins);

  if (!INS_IPREV(ins))
    return;

  /* first clear the marks */
  t_ins *ins_it;
  BBL_FOREACH_INS(INS_BBL(ins), ins_it)
    INS_SET_MARK(ins_it, false);

  t_arm_condition_code cond = (BBL_DESCRIPTION(INS_BBL(ins))->InsIsConditional(ins)) ? ARM_INS_CONDITION(T_ARM_INS(ins)) : ARM_CONDITION_AL;

  t_reg reg;
  REGSET_FOREACH_REG(INS_REGS_USE(ins), reg)
  {
    t_ins *x = find_ins_defining_reg(INS_IPREV(ins), reg, cond);

    if (x && !INS_MARK(x))
    {
      INS_SET_MARK(x, true);
      AddInstruction(x);
    }
  }

  Finalize();
}

void Slice::FillWithBbl(t_ins *ins)
{
  t_ins *ins_it = ins;
  while (ins_it)
  {
    AddInstruction(ins_it);
    ins_it = INS_IPREV(ins_it);
  }

  base_instruction = ins;

  Finalize();
}

void Slice::FillWithSequenceAddr(t_ins *ins)
{
  t_ins *ins_it = ins;
  AddInstruction(ins_it);

  if (ARM_INS_OPCODE(T_ARM_INS(ins_it)) == ARM_ADDRESS_PRODUCER) {
    /* don't do anything when starting with an address producer */
  }
  else {
    t_regset uses = INS_REGS_USE(ins_it);
    t_regset defs = INS_REGS_DEF(ins_it);
    t_regset address_before_defs = NullRegs;
    t_regset address_before_uses = NullRegs;

    while (ins_it)
    {
      ins_it = INS_IPREV(ins_it);
      if (!ins_it || Size() >= static_cast<size_t>(diabloanoptarm_options.af_max_block_length))
        break;

      if (ARM_INS_OPCODE(T_ARM_INS(ins_it)) == ARM_ADDRESS_PRODUCER) {
        t_reg r = ARM_INS_REGA(T_ARM_INS(ins_it));

        /* this is an address producer, try to put it after the sequence */
        if (!RegsetIn(uses, r)
            && !RegsetIn(defs, r)) {
          address_after.insert(T_ARM_INS(ins_it));
        }
        /* if that did not succeed, try to put it before the sequence */
        else {
          address_before.insert(T_ARM_INS(ins_it));
          RegsetSetUnion(address_before_defs, INS_REGS_DEF(ins_it));

          /* for conditional address producers preceeded by a CMP or similar */
          RegsetSetUnion(address_before_uses, INS_REGS_USE(ins_it));
        }
      }
      else {
        /* for this instruction to be added to the sequence,
         * we need to be able to move _all_ the 'before' address producers in front of the instruction.
         * Should that not be possible, this instruction can't be added to the sequence! */
        if (!RegsetIsEmpty(RegsetIntersect(address_before_defs, INS_REGS_USE(ins_it)))
            || !RegsetIsEmpty(RegsetIntersect(address_before_defs, INS_REGS_DEF(ins_it)))
            || !RegsetIsEmpty(RegsetIntersect(address_before_uses, INS_REGS_DEF(ins_it))))
          break;

        /* this is not an address producer, just add it to the sequence */
        AddInstruction(ins_it);

        RegsetSetUnion(uses, INS_REGS_USE(ins_it));
        RegsetSetUnion(defs, INS_REGS_DEF(ins_it));
      }
    }
  }

  base_instruction = ins;

  Finalize();
}

void Slice::FillWithSequence(t_ins *ins)
{
  t_ins *ins_it = ins;
  while (ins_it)
  {
    AddInstruction(ins_it);
    ins_it = INS_IPREV(ins_it);

    if (Size() >= static_cast<size_t>(diabloanoptarm_options.af_max_block_length))
      break;
  }

  base_instruction = ins;

  Finalize();
}

void Slice::InvalidateAllRelatedSlices() {
  //DEBUG(("SLICE-INVALIDATE %p (%p)", this, base_instruction));
  //DEBUG(("   found %u elements!", elements.size()));
  DissociateSliceAndBbl(Bbl(), this);
  for (auto ins : elements)
    DissociateFromIns(ins);
}

void Slice::AssociateWithIns(t_ins *ins)
{
  if (invalidated) return;
  ASSERT(!invalidated, ("trying to associate an instruction with an invalidated slice! @I", ins));

  if (!INS_SLICES(ins))
    INS_SET_SLICES(ins, new SliceSet());

  INS_SLICES(ins)->insert(this);
}

void Slice::DissociateFromIns(t_ins *ins)
{
  INS_SLICES(ins)->erase(this);
}

bool Slice::ContainsInstruction(t_ins *ins, size_t slice_size)
{
  if (is_sequence) {
    if (min_order <= INS_ORDER(ins)
        && INS_ORDER(ins) <= max_order) {
      if (slice_size == 0) {
        return true;
      }
      else {
        for (size_t i = 0; i < slice_size; i++)
          if (Get(i, slice_size) == ins)
            return true;
      }
    }
  }
  else {
    if (slice_size == 0) {
      /* no known limit, just iterate over all the instructions */
      for (size_t i = 0; i < elements.size(); i++)
        if (Get(i) == ins)
          return true;
    }
    else {
      for (size_t i = 0; i < slice_size; i++)
        if (Get(i, slice_size) == ins)
          return true;
    }

  }

  return false;
}

int Slice::Compare(Slice *b, size_t& nr_match, int max_len)
{
  //DEBUG(("comparing %u slice %p (%p) with %p (%p)", max_len, this, base_instruction, b, b->base_instruction));
  //DEBUG(("     (base @I)", base_instruction));
  //DEBUG(("     (base @I)", b->base_instruction));
  int result = 0;
  int level = 0;

  size_t a_ridx = 0;
  size_t b_ridx = 0;

  nr_match = 0;

  while (a_ridx < NrInstructions()
         && b_ridx < b->NrInstructions())
  {
    t_ins *a_ins = GetR(a_ridx);
    t_ins *b_ins = b->GetR(b_ridx);

    if (b->is_sequence
        && diabloanoptarm_options.advanced_factoring_identical_sequences) {
      if (!CompareInstructionsLiteral(a_ins, b_ins))
        return SLICECOMPARE_NE;
    }
    else {
      auto result = CompareAbstractInstructions(a_ins, b_ins);
      switch (result) {
      case CompareInstructionsResult::Smaller:
        return SLICECOMPARE_LT;

      case CompareInstructionsResult::Greater:
        return SLICECOMPARE_GT;

      case CompareInstructionsResult::NotEqual:
        return SLICECOMPARE_NE;

      default:
        ;
      }
    }

    nr_match++;

    a_ridx++;
    b_ridx++;

    level++;
    if (max_len > 0
        && level == max_len)
      break;
  }

  return SLICECOMPARE_EQ;
}

size_t Slice::NrInstructions()
{
  size_t result = 0;

  for (auto ins : elements)
      result++;

  return result;
}

void Slice::UpdateInstructions() {
  t_bbl *bbl = Bbl();
  vector<t_ins *> new_elements;

  for (auto ins : elements) {
    DissociateSliceAndBbl(INS_BBL(ins), this);
    DissociateFromIns(ins);
  }

  copy_if(elements.begin(), elements.end(), back_inserter(new_elements), [bbl](t_ins *ins) {
    return INS_BBL(ins) == bbl;
  });

  bool ok = true;
  if (is_sequence) {
    /* this sequence may have been split up.
     * if so, it cannot be transformed anymore */
    map<int, t_ins *> ordered;
    for (auto x : new_elements)
      ordered[INS_ORDER(x)] = x;

    /* check for the sequence: all instructions should have a sequential order */
    bool first = true;
    int prev_order = 0;
    for (auto pair : ordered) {
      int order = pair.first;

      if (first) {
        min_order = order;
        first = false;
      }
      else if (order != prev_order+1) {
        ok = false;
        break;
      }

      prev_order = order;
      max_order = order;
    }
  }

  elements = new_elements;
  for (auto ins : elements)
    AssociateWithIns(ins);
  AssociateSliceAndBbl(Bbl(), this);

  if (NrInstructions() < static_cast<size_t>(diabloanoptarm_options.af_min_block_length)
      || !ok)
    Invalidate();
  else if (parent_slice && !parent_slice->IsInvalidated())
    parent_slice->UpdateInstructions();
}

t_bool Slice::CompareExact(Slice *b, size_t& nr_match, int max_len)
{
  int level = 0;

  size_t a_ridx = 0;
  size_t b_ridx = 0;

  nr_match = 0;

  t_bool (*InsCmp)(t_ins *, t_ins *);
  InsCmp = CFG_DESCRIPTION(BBL_CFG(Bbl()))->InsCmp;

  while (a_ridx < NrInstructions()
          && b_ridx < b->NrInstructions())
  {
    t_ins *a_ins = GetR(a_ridx);
    t_ins *b_ins = b->GetR(b_ridx);

    auto result = InsCmp(a_ins, b_ins);
    if (!result)
      return false;

    nr_match++;

    a_ridx++;
    b_ridx++;

    level++;
    if (max_len > 0
        && level == max_len)
      break;
  }

  return true;
}

string Slice::Print(size_t slice_size)
{
  stringstream ss;
  ss << "Slice(" << uid << ") ";
  if (is_sequence)
    ss << "SEQUENCE(" << min_order << "-" << max_order << ")";
  else
    ss << "SLICE";
  ss << " " << IsInvalidated() << " ";

//#define PRINT_SLICE_POINTER
#ifdef PRINT_SLICE_POINTER
{
  auto str_ptr = StringIo("[%p] ", this);
  ss << str_ptr;
  Free(str_ptr);
}
#endif

//#define PRINT_BBL_ID
#ifdef PRINT_BBL_ID
  auto str_id = StringIo("[id=%u] ",BBL_ID(Bbl()));
  ss << str_id;
  Free(str_id);
#endif

//#define PRINT_BBL_POINTER
#ifdef PRINT_BBL_POINTER
{
  auto str_ptr = StringIo("[%p] ", Bbl());
  ss << str_ptr;
  Free(str_ptr);
}
#endif

  auto str_bbl = StringIo("@B", Bbl());
  ss << str_bbl << " [";
  Free(str_bbl);

  if (IsMaster(this))
    ss << "MASTER";
  else
    ss << "SLAVE";
  ss << "] contains " << Size() << " instructions" << endl;

//#define PRINT_SLICE_REGSETS
#ifdef PRINT_SLICE_REGSETS
  auto str_livebefore = StringIo("@X", CPREGSET(INS_CFG(elements[0]), RegsetIntersect(SliceRegsLiveBefore(this, slice_size), CFG_DESCRIPTION(INS_CFG(elements[0]))->int_registers)));
  ss << " live-before: " << str_livebefore << endl;
  Free(str_livebefore);

  auto str_liveafter = StringIo("@X", CPREGSET(INS_CFG(elements[0]), RegsetIntersect(SliceRegsLiveAfter(this, slice_size), CFG_DESCRIPTION(INS_CFG(elements[0]))->int_registers)));
  ss << " live-after: " << str_liveafter << endl;
  Free(str_liveafter);

  auto str_deadthrough = StringIo("@X", CPREGSET(INS_CFG(elements[0]), SliceRegsDeadThrough(this, slice_size)));
  ss << "  dead-through: " << str_deadthrough << endl;
  Free(str_deadthrough);
#endif

  /* print elements in slice */
  size_t ins_idx = 0;
  if (slice_size > 0)
  {
    /* get index of instruction nr 'slice_size', from the back */
    auto ins = GetR(slice_size - 1);
    for (auto i : elements)
    {
      if (i == ins)
        break;

      ins_idx++;
    }
  }

  for (; ins_idx < elements.size(); ins_idx++)
  {
    auto elem = elements[ins_idx];

    auto str_ins = StringIo("@I", elem);
    ss << str_ins << " [fingerprint: 0x" << hex << INS_FINGERPRINT(elem) << ", order: " << INS_ORDER(elem) << "]" << endl;
    Free(str_ins);
  }

  {
    auto str_bbl = StringIo("@iB", Bbl());
    ss << str_bbl << endl;
    Free(str_bbl);
  }

  return ss.str();
}

bool Slice::Overlaps(Slice *b)
{
  return INS_BBL(base_instruction) == INS_BBL(b->base_instruction);
}

int slice_id(t_ins *ins)
{
  return INS_SLICE_ID(ins);
}

static void set_slice_id(t_ins *ins, int id)
{
  INS_SET_SLICE_ID(ins, id);
}

t_bool Slice::RescheduleBbl()
{
  t_ins *ins_it;
  BBL_FOREACH_INS(Bbl(), ins_it)
    INS_SET_SLICE_ID(ins_it, ~0);

  for (auto ins : elements)
    INS_SET_SLICE_ID(ins, 0);

  t_bool result = false;
  DiabloBrokerCall("RescheduleBblForSlice", Bbl(), slice_id, set_slice_id);

  return result;
}

bool SliceCompare::operator() (const Slice* left, const Slice* right) const
{
	return left->uid < right->uid;
}

vector<FactoredPath> Slice::CalculateFactoredPaths(t_regset live_registers)
{
  vector<FactoredPath> result;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(Bbl(), e)
  {
    bool split_edge = true;
    result.push_back(FactoredPath{{e}, BBL_SUCC_FIRST(Bbl()), -1, split_edge, this});
  }

  return result;
}

t_ins *Slice::Get(int i)
{
  return elements[i];
}

t_ins *Slice::GetR(int i)
{
  return elements[(elements.size()-1)-i];
}

t_ins *Slice::Get(int i, size_t slice_size)
{
  return GetR(slice_size - i - 1);
}

void Slice::FixCombineResults() {
  combine_data_fixed = true;
}

void Slice::UnfixCombineResults() {
  combine_data_fixed = false;
}

void Slice::PrecalculateCombineData(size_t slice_size) {
  /* liveness */
  dead_through = SliceRegsDeadThrough(this, slice_size);
  live_after = SliceRegsLiveAfter(this, slice_size);
  dead_before = SliceRegsLiveBefore(this, slice_size);
  RegsetSetInvers(dead_before);
  RegsetSetIntersect(dead_before, CFG_DESCRIPTION(BBL_CFG(Bbl()))->int_registers);

  /* calculate the overwritten registers */
  overwritten_registers = NullRegs;
  for (size_t i = 0; i < slice_size; i++)
    RegsetSetUnion(overwritten_registers, INS_REGS_DEF(Get(i, slice_size)));

  /* nonzero analysis */
  nonzero_before = SliceRegsNonZeroIn(this, slice_size);

  /* constant analysis */
  if (procstate_before)
    ProcStateFree(procstate_before);
  procstate_before = SliceProcstateBefore(this, slice_size);
  ProcstateConstantRegisters(procstate_before, constant_before, null_before, tag_before);

#if AF_COPY_ANALYSIS
  /* TODO: copy analysis */
#endif

  can_contain_z = GPRegistersEmpty();
  can_contain_nz = GPRegistersEmpty();

  for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
    bool a, m, c;

    if (CanProduceZeroInRegister(r, a, m, c))
      GPRegistersAdd(can_contain_z, r);

    if (CanProduceNonzeroInRegister(r, a, m, c))
      GPRegistersAdd(can_contain_nz, r);
  }
}

bool Slice::CanProduceZeroInRegister(t_reg reg, bool& already, bool& mov, bool& create) {
  bool result = false;
  already = false;
  mov = false;
  create = false;

  if (RegsetIn(null_before, reg)) {
    /* this register already contains 0 */
    already = true;
    result = true;
  }

  if (RegsetIn(dead_before, reg)) {
    if (!RegsetIsEmpty(null_before)) {
      /* another register can be moved in here to create 0 */
      mov = true;
    }
    else {
      /* we need to create a 0 value in this register */
      create = true;
    }

    /* either way, 0 can be produced */
    result = true;
  }

  return result;
}

bool Slice::CanProduceNonzeroInRegister(t_reg reg, bool& already, bool& mov, bool& create) {
  bool result = false;
  already = false;
  mov = false;
  create = false;

  if ((RegsetIn(constant_before, reg) && !RegsetIn(null_before, reg))
      || RegsetIn(tag_before, reg)
      || RegsetIn(nonzero_before, reg)) {
    /* this register already contains !0 */
    already = true;
    result = true;
  }

  if (RegsetIn(dead_before, reg)) {
    if (!RegsetIsEmpty(RegsetDiff(constant_before, null_before))
        || !RegsetIsEmpty(tag_before)
        || !RegsetIsEmpty(nonzero_before)) {
      /* another register can be moved in here to create !0 */
      mov = true;
    }
    else {
      /* we need to create a !0 value in this register */
      create = true;
    }

    result = true;
  }

  return result;
}

Slice *CalculateSliceForInstruction(t_ins *ins, F_SliceIsMaster IsMaster, F_SliceIsSlave IsSlave, bool sequence, bool sequence_without_addrprod)
{
  Slice *slice = NULL;

  INS_SET_ABSTRACT_FORM(ins, ArmInsGetAbstractInstructionForm(ins));
  INS_SET_FINGERPRINT(ins, InstructionFingerprint::GetFingerprint(ins));

  /* skip unsupported instructions */
  if (INS_ABSTRACT_FORM(ins) != AbstractInstructionFormCalculator::Type::Unsupported
      && !(sequence && (ins == BBL_INS_FIRST(INS_BBL(ins)))))
  {
    slice = new Slice(IsMaster, IsSlave, sequence);
    //DEBUG(("created3 slice %p", slice));

    bool del = false;
    if (sequence) {
      /* construct sequence of instructions, not necessarily slice */
      if (sequence_without_addrprod) {
        slice->FillWithSequenceAddr(ins);

        if (slice->address_before.size() == 0
            && slice->address_after.size() == 0)
          del = true;
      } else
        slice->FillWithSequence(ins);

      if (slice->Size() < 2
          || slice->Size() < static_cast<size_t>(diabloanoptarm_options.af_min_block_length)
          || del)
      {
        delete slice;
        slice = NULL;
      }
    }
    else {
#ifdef SLICE_IS_ENTIRE_BBL
      slice->FillWithBbl(ins);
#endif
#ifdef SLICE_IS_DEFS
      slice->FillWithDefs(ins);
#endif

      if (slice->Size() < static_cast<size_t>(diabloanoptarm_options.af_min_block_length))
      {
        delete slice;
        slice = NULL;
      }
    }

    if (slice != NULL)
      AssociateSliceAndBbl(slice->Bbl(), slice);
  }

  return slice;
}

void InsSliceFree(Slice *valp)
{
  if (valp && !(valp->dont_destroy))
  {
    created_slices_global.erase(valp);
    delete valp;
  }
}
