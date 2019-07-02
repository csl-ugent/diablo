#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

// #define DEBUG_REG_EQ
// #define DEBUG_REG_EQ_INPUT
// #define DEBUG_REG_EQ_ACTION

#define NR_REGISTERS 16
typedef array<t_uint8, NR_REGISTERS> t_registers;

#define VREG_UNKNOWN    0x00
#define VREG_INPUT      0x01
#define VREG_INTERNAL   0x02
#define VREG_OUTPUT     0x04
#define VREG_NONE       0xff

enum class RegisterOrigin {
  Input = VREG_INPUT,
  Internal = VREG_INTERNAL,
  Output = VREG_OUTPUT
};

/* Mark the virtual registers as being either:
 *  - slice-input, in case the register is defined from outside the slice, and used within the slice.
 *  - slice-internal, in case the register is defined within the slice, but not live-out.
 *  - slice-output, in case the register is live-out after the slice. */
static
void SliceMarkRegisters(Slice *slice, size_t slice_size)
{
  array<RegisterOrigin, NR_REGISTERS> register_origins;
  register_origins.fill(RegisterOrigin::Input);

  for (size_t i = 0; i < slice_size; i++)
  {
    /* helper lambda */
    auto process_def = [&register_origins, i, slice_size, slice] (t_reg reg) {
      /* need to check whether this register is an output value or not */
      // if (!RegsetIn(slice->regeq_live_out, reg))
      //   /* this register is NOT live-out. Thus it is an internally used register. */
      //   register_origins[reg] = RegisterOrigin::Internal;
      // else
      {
        /* here we know that this register is live-out.
         * However, another instruction in the slice (executed later) may define the final value */
        bool is_defined_further = false;
        for (size_t j = i+1; j < slice_size && !is_defined_further; j++)
        {
          t_ins *j_ins = slice->Get(j, slice_size);

          if (RegsetIn(INS_REGS_DEF(j_ins), reg))
            is_defined_further = true;
        }

        if (!is_defined_further)
          register_origins[reg] = RegisterOrigin::Output;
        else
          register_origins[reg] = RegisterOrigin::Internal;
      }
    };

    t_arm_ins *ins = T_ARM_INS(slice->Get(i, slice_size));

    ARM_INS_SET_VREGA_TYPE(ins, VREG_UNKNOWN);
    ARM_INS_SET_VREGABIS_TYPE(ins, VREG_UNKNOWN);
    ARM_INS_SET_VREGB_TYPE(ins, VREG_UNKNOWN);
    ARM_INS_SET_VREGC_TYPE(ins, VREG_UNKNOWN);
    ARM_INS_SET_VREGS_TYPE(ins, VREG_UNKNOWN);

    if (ARM_INS_USES_REGA(ins)) ARM_INS_SET_VREGA_TYPE(ins, static_cast<int>(register_origins[ARM_INS_REGA(ins)]));
    if (ARM_INS_USES_REGABIS(ins)) ARM_INS_SET_VREGABIS_TYPE(ins, static_cast<int>(register_origins[ARM_INS_REGABIS(ins)]));
    if (ARM_INS_USES_REGB(ins)) ARM_INS_SET_VREGB_TYPE(ins, static_cast<int>(register_origins[ARM_INS_REGB(ins)]));
    if (ARM_INS_USES_REGC(ins)) ARM_INS_SET_VREGC_TYPE(ins, static_cast<int>(register_origins[ARM_INS_REGC(ins)]));
    if (ARM_INS_USES_REGS(ins)) ARM_INS_SET_VREGS_TYPE(ins, static_cast<int>(register_origins[ARM_INS_REGS(ins)]));

    if (ARM_INS_DEFS_REGA(ins))
    {
      process_def(ARM_INS_REGA(ins));
      ARM_INS_SET_VREGA_TYPE(ins, ARM_INS_VREG_TYPE(ARM_INS_VREGA(ins)) | static_cast<int>(register_origins[ARM_INS_REGA(ins)]));
    }
    if (ARM_INS_DEFS_REGABIS(ins))
    {
      process_def(ARM_INS_REGABIS(ins));
      ARM_INS_SET_VREGABIS_TYPE(ins, ARM_INS_VREG_TYPE(ARM_INS_VREGABIS(ins)) | static_cast<int>(register_origins[ARM_INS_REGABIS(ins)]));
    }
    if (ARM_INS_DEFS_REGB(ins))
    {
      process_def(ARM_INS_REGB(ins));
      ARM_INS_SET_VREGB_TYPE(ins, ARM_INS_VREG_TYPE(ARM_INS_VREGB(ins)) | static_cast<int>(register_origins[ARM_INS_REGB(ins)]));
    }
    if (ARM_INS_DEFS_REGC(ins))
    {
      process_def(ARM_INS_REGC(ins));
      ARM_INS_SET_VREGC_TYPE(ins, ARM_INS_VREG_TYPE(ARM_INS_VREGC(ins)) | static_cast<int>(register_origins[ARM_INS_REGC(ins)]));
    }
    if (ARM_INS_DEFS_REGS(ins))
    {
      process_def(ARM_INS_REGS(ins));
      ARM_INS_SET_VREGS_TYPE(ins, ARM_INS_VREG_TYPE(ARM_INS_VREGS(ins)) | static_cast<int>(register_origins[ARM_INS_REGS(ins)]));
    }
  }
}

static
VirtualizationResult SliceVirtualizeRegisters(Slice *slice, size_t slice_size, t_regset& mapped_registers, bool is_debug = false)
{
  VirtualizationResult vresult;

  /* mapping Rx registers to virtual registers */
  t_registers internal_regs;
  t_registers input_regs;
  t_registers output_regs;

  internal_regs.fill(VREG_NONE);
  input_regs.fill(VREG_NONE);
  output_regs.fill(VREG_NONE);

  mapped_registers = NullRegs;

  auto assign_virtual_register = [&internal_regs, &input_regs, &output_regs, &vresult, &slice] (t_reg r, int type, t_arm_ins *ins) {
    t_uint32 vreg_number_i = 0xff;
    t_uint32 vreg_number_I = 0xff;
    t_uint32 vreg_number_O = 0xff;

    if (type & VREG_INTERNAL) {
      if (internal_regs[r] == VREG_NONE)
      {
        internal_regs[r] = vresult.internal_vregs.size();
        vresult.internal_vregs.insert(t_vreg_to_reg_map_item(vresult.internal_vregs.size(), r));
      }
      ASSERT(r < NR_REGISTERS, ("what? @I %s", ins, slice->Print().c_str()));
      vreg_number_i = internal_regs[r];
    }
    if (type & VREG_INPUT) {
      if (input_regs[r] == VREG_NONE)
      {
        input_regs[r] = vresult.input_vregs.size();
        vresult.input_vregs.insert(t_vreg_to_reg_map_item(vresult.input_vregs.size(), r));
      }
      ASSERT(r < NR_REGISTERS, ("what? @I %s", ins, slice->Print().c_str()));
      vreg_number_I = input_regs[r];
    }
    if (type & VREG_OUTPUT) {
      if (output_regs[r] == VREG_NONE)
      {
        output_regs[r] = vresult.output_vregs.size();
        vresult.output_vregs.insert(t_vreg_to_reg_map_item(vresult.output_vregs.size(), r));
      }
      ASSERT(r < NR_REGISTERS, ("what? @I %s", ins, slice->Print().c_str()));
      vreg_number_O = output_regs[r];
    }

    return vreg_number_I | (vreg_number_O << 8) | (vreg_number_i << 16);
  };

  auto add_mapped_register = [&mapped_registers] (t_reg r) {
    if (r == ARM_REG_NONE)
      return;

    RegsetSetAddReg(mapped_registers, r);
  };

  for (size_t i = 0; i < slice_size; i++)
  {
    t_arm_ins *ins = T_ARM_INS(slice->Get(i, slice_size));

    /* virtual registers */
    ARM_INS_SET_VREGA_VALUE(ins, (ARM_INS_REGA(ins) != ARM_REG_NONE) ? assign_virtual_register(ARM_INS_REGA(ins), ARM_INS_VREG_TYPE(ARM_INS_VREGA(ins)), ins) : VREG_NONE);
    ARM_INS_SET_VREGABIS_VALUE(ins, (ARM_INS_REGABIS(ins) != ARM_REG_NONE) ? assign_virtual_register(ARM_INS_REGABIS(ins), ARM_INS_VREG_TYPE(ARM_INS_VREGABIS(ins)), ins) : VREG_NONE);
    ARM_INS_SET_VREGB_VALUE(ins, (ARM_INS_REGB(ins) != ARM_REG_NONE) ? assign_virtual_register(ARM_INS_REGB(ins), ARM_INS_VREG_TYPE(ARM_INS_VREGB(ins)), ins) : VREG_NONE);
    ARM_INS_SET_VREGC_VALUE(ins, (ARM_INS_REGC(ins) != ARM_REG_NONE) ? assign_virtual_register(ARM_INS_REGC(ins), ARM_INS_VREG_TYPE(ARM_INS_VREGC(ins)), ins) : VREG_NONE);
    ARM_INS_SET_VREGS_VALUE(ins, (ARM_INS_REGS(ins) != ARM_REG_NONE) ? assign_virtual_register(ARM_INS_REGS(ins), ARM_INS_VREG_TYPE(ARM_INS_VREGS(ins)), ins) : VREG_NONE);

    /* physical registers */
    add_mapped_register(ARM_INS_REGA(ins));
    add_mapped_register(ARM_INS_REGABIS(ins));
    add_mapped_register(ARM_INS_REGB(ins));
    add_mapped_register(ARM_INS_REGC(ins));
    add_mapped_register(ARM_INS_REGS(ins));
  }

  return vresult;
}

static
bool VirtualRegistersEqual(t_ins *a, t_ins *b)
{
  t_arm_ins *_a = T_ARM_INS(a);
  t_arm_ins *_b = T_ARM_INS(b);

  if (ARM_INS_VREG_VALUE(ARM_INS_VREGA(_a)) != ARM_INS_VREG_VALUE(ARM_INS_VREGA(_b)))
    return false;
  if (ARM_INS_VREG_VALUE(ARM_INS_VREGABIS(_a)) != ARM_INS_VREG_VALUE(ARM_INS_VREGABIS(_b)))
    return false;
  if (ARM_INS_VREG_VALUE(ARM_INS_VREGB(_a)) != ARM_INS_VREG_VALUE(ARM_INS_VREGB(_b)))
    return false;
  if (ARM_INS_VREG_VALUE(ARM_INS_VREGC(_a)) != ARM_INS_VREG_VALUE(ARM_INS_VREGC(_b)))
    return false;
  if (ARM_INS_VREG_VALUE(ARM_INS_VREGS(_a)) != ARM_INS_VREG_VALUE(ARM_INS_VREGS(_b)))
    return false;

  if (ARM_INS_VREG_TYPE(ARM_INS_VREGA(_a)) != ARM_INS_VREG_TYPE(ARM_INS_VREGA(_b)))
    return false;
  if (ARM_INS_VREG_TYPE(ARM_INS_VREGABIS(_a)) != ARM_INS_VREG_TYPE(ARM_INS_VREGABIS(_b)))
    return false;
  if (ARM_INS_VREG_TYPE(ARM_INS_VREGB(_a)) != ARM_INS_VREG_TYPE(ARM_INS_VREGB(_b)))
    return false;
  if (ARM_INS_VREG_TYPE(ARM_INS_VREGC(_a)) != ARM_INS_VREG_TYPE(ARM_INS_VREGC(_b)))
    return false;
  if (ARM_INS_VREG_TYPE(ARM_INS_VREGS(_a)) != ARM_INS_VREG_TYPE(ARM_INS_VREGS(_b)))
    return false;

  return true;
}

bool EqualizeRegistersPrepare(SliceSet slices, size_t slice_size, t_gpregisters preferably_dont_touch, Slice *& ref_slice)
{
  bool result = true;
  t_cfg *cfg = SliceSetCfg(slices);
  t_uint32 worst_required_register_count = 0;

  /* collect a priori known information about the slices */
  for (auto slice : slices)
  {
    slice->regeq_live_out = RegsetIntersect(SliceRegsLiveAfter(slice, slice_size), CFG_DESCRIPTION(cfg)->int_registers);

    SliceMarkRegisters(slice, slice_size);

    t_regset mapped_registers = NullRegs;
    slice->regeq_virtual = SliceVirtualizeRegisters(slice, slice_size, mapped_registers);

    slice->regeq_nr_mapped_regs = RegsetCountRegs(mapped_registers);
#ifdef DEBUG_REG_EQ
    DEBUG(("originally use %u registers", slice->regeq_nr_mapped_regs));
#endif
    worst_required_register_count = (worst_required_register_count < slice->regeq_nr_mapped_regs) ? slice->regeq_nr_mapped_regs : worst_required_register_count;
  }

  /* find the worst-case slices */
  vector<Slice *> worst_case;
  for (auto slice : slices)
    if (slice->regeq_nr_mapped_regs == worst_required_register_count)
      worst_case.push_back(slice);

  /* check that the virtual registers are identical for every instruction in the slices */
  for (size_t ins_id = 0; ins_id < slice_size; ins_id++)
  {
    Slice *reference = nullptr;
    for (auto slice : slices)
    {
      if (reference == nullptr)
      {
        reference = slice;
        continue;
      }

      /* compare to the reference */
      t_ins *ref_ins = reference->Get(ins_id, slice_size);
      t_ins *cmp_ins = slice->Get(ins_id, slice_size);
      bool virtual_equal = VirtualRegistersEqual(ref_ins, cmp_ins);

      if (!virtual_equal)
      {
#ifdef DEBUG_REG_EQ
        //SliceSetPrint(slices, "BOE", slice_size);
        //DEBUG(("virtual registers for @I and @I differ!", ref_ins, cmp_ins));
#endif
        result = false;
      }

      if (!result)
        break;
    }

    if (!result)
      break;
  }

  if (!result)
    return false;

  /* choose a reference slice */

  /* try to find a reference slice which destroys the least amount of preferably untouched registers */
  t_uint32 nr_destroy = 1000;
  for (auto slice : worst_case) {
    t_uint32 my_nr_destroy = GPRegistersCount(GPRegistersIntersectRegset(preferably_dont_touch, slice->overwritten_registers));
    if (my_nr_destroy < nr_destroy) {
      /* this slice overwrites less registers than a previous one */
      ref_slice = slice;
      nr_destroy = my_nr_destroy;
    }
  }

#ifdef DEBUG_REG_EQ
  DEBUG(("reference slice: %s", ref_slice->Print(slice_size).c_str()));
#endif

  return true;
}

bool EqualizeRegisters(SliceSet slices, size_t slice_size, SliceSpecificActionList& action_list, SliceSpecificRegisters& slice_registers, Slice *ref_slice, t_gpregisters& preferably_dont_touch, SliceToGPRegistersMap& overwritten_registers_per_slice, SliceToAllRegisterInformationMap& all_register_information_per_slice)
{
#ifdef DEBUG_REG_EQ
  DEBUG(("=============================="));
#endif
  t_cfg *cfg = SliceSetCfg(slices);

  auto find_register_containing_register = [] (t_registers arr, t_reg reg) {
    t_reg result = reg;

    for (int i = ARM_REG_R0; i < ARM_REG_R15; i++) {
      if (arr[i] == reg) {
        result = i;
        break;
      }
    }

    return result;
  };

  auto find_register_containing_register_reverse = [] (t_registers arr, t_reg reg) {
    return arr[reg];
  };

  auto find_vreg = [] (t_vreg_to_reg_map vreg_map, size_t vreg) {
    auto o = vreg_map.find(vreg);
    ASSERT(o != vreg_map.end(), ("could not find v%d!", vreg));
    return o->second;
  };

  auto find_vreg_index = [] (t_vreg_to_reg_map vreg_map, t_reg reg) {
    size_t index = -1;

    for (auto o : vreg_map) {
      if (o.second == reg) {
        index = o.first;
        break;
      }
    }

    ASSERT(index != static_cast<size_t>(-1), ("could not find r%d!", static_cast<int>(reg)));
    return index;
  };

  auto print_array = [] (string prefix, t_registers arr) {
    stringstream ss;

    for (int i = ARM_REG_R0; i < ARM_REG_R15; i++) {
      if (arr[i] == ARM_REG_NONE)
        continue;

      ss << "r" << i << " = R" << static_cast<int>(arr[i]) << ", ";
    }

#ifdef DEBUG_REG_EQ
    DEBUG(("%s %s", prefix.c_str(), ss.str().c_str()));
#endif
  };

  t_regset common_dead_through = SliceSetCalculateRegsDeadThrough(slices, slice_size);

  for (auto slice : slices)
  {
    t_gpregisters overwritten_registers = GPRegistersEmpty();
    t_reg reg;

    if (slice == ref_slice)
      continue;

    AFActionList actions;

    t_regset used_registers = NullRegs;

    /* register that we'll need to restore */
    t_regset registers_to_be_restored = RegsetDup(slice->regeq_live_out);
    for (size_t vreg_index = 0; vreg_index < slice->regeq_virtual.output_vregs.size(); vreg_index++) {
      auto x = slice->regeq_virtual.output_vregs.find(vreg_index);
      ASSERT(x != slice->regeq_virtual.output_vregs.end(), ("could not find v%d", vreg_index));
      RegsetSetSubReg(registers_to_be_restored, x->second);
    }
#ifdef DEBUG_REG_EQ
    DEBUG(("need to restore @X", CPREGSET(cfg, registers_to_be_restored)));
#endif

    /* keep track where the original registers are moved to */
    /* current register -> actual old register mapping */
    t_registers aliases;
    aliases.fill(ARM_REG_NONE);

    /* propagate the original register values */
    t_registers in_reg_prop;
    in_reg_prop.fill(ARM_REG_NONE);

    t_registers register_lut;
    register_lut.fill(ARM_REG_NONE);

    t_regset registers_still_needed = NullRegs;
    for (size_t vreg_index = 0; vreg_index < ref_slice->regeq_virtual.input_vregs.size(); vreg_index++) {
      auto x = slice->regeq_virtual.input_vregs.find(vreg_index);
      ASSERT(x != slice->regeq_virtual.input_vregs.end(), ("could not find v%d", vreg_index));
      RegsetSetAddReg(registers_still_needed, x->second);
    }

    /* iterate over all the input registers */
    for (size_t vreg_index = 0; vreg_index < ref_slice->regeq_virtual.input_vregs.size(); vreg_index++) {
      t_reg ref_reg = find_vreg(ref_slice->regeq_virtual.input_vregs, vreg_index);
      RegsetSetAddReg(used_registers, ref_reg);

      t_reg reg = find_vreg(slice->regeq_virtual.input_vregs, vreg_index);

      /* these registers are equal */
      if (reg == ref_reg || in_reg_prop[ref_reg] == reg) {
        RegsetSetSubReg(registers_still_needed, reg);
        continue;
      }

      /* find out where the register is stored (in case previous swaps moved this original register) */
      t_reg location = find_register_containing_register(in_reg_prop, reg);

      if (ref_reg == location)
        continue;

      AFAction action;
      action.code = AFActionCode::SwapRegisters;
      action.before = true;
      action.slice = slice;
      action.args[0].reg = ref_reg;
      action.args[1].reg = location;

      aliases[ref_reg] = location;
      register_lut[location] = ref_reg;

      /* 'ref_reg' will be overwritten and thus can't be used as a predicate register */
      GPRegistersSetSubReg(preferably_dont_touch, ref_reg);
      GPRegistersAdd(overwritten_registers, ref_reg);

      /* we need to look up the registers prior to assigning them to the list
       * because this may influence the outcome! */
      t_reg a = (in_reg_prop[location] == ARM_REG_NONE) ? location : in_reg_prop[location];
      t_reg b = (in_reg_prop[ref_reg] == ARM_REG_NONE) ? ref_reg : in_reg_prop[ref_reg];

      /* keep the old value of the overwritten register if:
       *  - the register needs to be restored afterwards (e.g., it is live-out of the slice);
       *  - the register is still needed for input equalisation;
       *  - the register is preferably untouched. */
      if (RegsetIn(registers_to_be_restored, b)
          || RegsetIn(registers_still_needed, b)
          || GPRegistersIn(preferably_dont_touch, b)) {
        aliases[location] = ref_reg;
        register_lut[ref_reg] = location;

        in_reg_prop[location] = b;

        /* 'location' will be overwritten and thus can't be used as a predicate register */
        GPRegistersSetSubReg(preferably_dont_touch, location);
        GPRegistersAdd(overwritten_registers, location);
      }
      else {
#ifdef DEBUG_REG_EQ_INPUT
        DEBUG(("r%d need not be restored! r%d/r%d @X; @X", location, ref_reg, location, CPREGSET(cfg, registers_to_be_restored), CPREGSET(cfg, registers_still_needed)));
#endif

        action.code = AFActionCode::AliasRegisters;
      }

      in_reg_prop[ref_reg] = a;

      actions.push_back(action);
#ifdef DEBUG_REG_EQ_ACTION
      DEBUG(("I%d added action: %s", vreg_index, PrintAction(action).c_str()));
#endif

      /* this input register has been translated */
      RegsetSetSubReg(registers_still_needed, reg);
    }
#ifdef DEBUG_REG_EQ_INPUT
    print_array("prebackup input:", in_reg_prop);
#endif

    /* check that every register that should be restored is safely backed up */
    REGSET_FOREACH_REG(registers_to_be_restored, reg) {
      if (reg >= ARM_REG_R15) break;

      /* if the register has not been overwritten by our compensation instructions,
       * and the register will not be overwritten by the slice, we don't need to take any action... */
      if (in_reg_prop[reg] == ARM_REG_NONE
          && !RegsetIn(ref_slice->overwritten_registers, reg))
        continue;

      /* else, assume that the register is not backed up and look for an existing backup location */
      bool safe = false;
      for (int i = ARM_REG_R0; i < ARM_REG_R15; i++) {
        if (in_reg_prop[i] == reg && !RegsetIn(ref_slice->overwritten_registers, i))
          safe = true;
      }

      /* if this register is safe, don't do anything! */
      if (safe)
        continue;

      /* need to take action! */

      /* look which register this one is currently stored in */
      t_reg src_reg = reg;
      for (int i = ARM_REG_R0; i < ARM_REG_R15; i++) {
        if (in_reg_prop[i] == reg) {
          src_reg = i;
          break;
        }
      }
      if (!((src_reg == reg && in_reg_prop[src_reg] == ARM_REG_NONE) || (src_reg != reg && in_reg_prop[src_reg] == reg))) {
        SliceSetPrint(slices, "BOE", slice_size);
        FATAL(("need to back up r%d, but could not find it!", reg));
      }

      t_reg dst_reg = ARM_REG_NONE;
#define MAX_SEARCH_PHASES 2
      int phase = 0;
      while (phase <= MAX_SEARCH_PHASES) {
        for (int i = ARM_REG_R0; i < ARM_REG_R15; i++) {
          /* if this register will be overwritten, it can't be used as a backup register */
          if (RegsetIn(ref_slice->overwritten_registers, i))
            continue;

          /* also, if another register is already put in this register */
          if (in_reg_prop[i] != ARM_REG_NONE)
            continue;

          /* it does not have any other register aliased to it yet,
           * but maybe this is because the register itself needs to be restored */
          if (RegsetIn(registers_to_be_restored, i))
            continue;

          /* also don't put anything in a register that is used in the slice */
          if (RegsetIn(used_registers, i))
            continue;

          /* During an initial search, we preferably don't want to use a commonly dead register
           * and also not a preferably untouched register. */
          if (phase == 0 && (RegsetIn(common_dead_through, i) || GPRegistersIn(preferably_dont_touch, i)))
            continue;

          /* Since we didn't find any suitable register in phase 0, we should allow
           * for commonly dead registers to be used. However, we still don't want
           * to sacrifice a preferably not touched register. */
          if (phase == 1 && GPRegistersIn(preferably_dont_touch, i))
            continue;

          /* As the previous phases didn't result in a suitable register,
           * allow all sacrifices to be made here. */

          /* we have found a backup register! */
          dst_reg = i;
          break;
        }

        /* try another search in case we weren't able to find a suitable register */
        if (dst_reg == ARM_REG_NONE)
          phase++;
        else
          break;
      }

      if (dst_reg == ARM_REG_NONE) {
#ifdef DEBUG_REG_EQ
        DEBUG(("could not find backup register for r%d!", reg));
#endif
        return false;
      }

      GPRegistersSetSubReg(preferably_dont_touch, dst_reg);
      GPRegistersAdd(overwritten_registers, dst_reg);

      AFAction action;
      action.code = AFActionCode::AliasRegisters;
      action.before = true;
      action.slice = slice;
      action.args[0].reg = dst_reg;
      action.args[1].reg = src_reg;

      actions.push_back(action);
#ifdef DEBUG_REG_EQ_ACTION
      DEBUG(("backing up r%d (in r%d) to r%d: added action: %s", reg, src_reg, dst_reg, PrintAction(action).c_str()));
#endif

      in_reg_prop[dst_reg] = reg;
    }
#ifdef DEBUG_REG_EQ_INPUT
    print_array("final input:", in_reg_prop);
#endif

    /* sanity check */
    for (size_t vreg_index = 0; vreg_index < ref_slice->regeq_virtual.input_vregs.size(); vreg_index++) {
      t_reg ref_reg = find_vreg(ref_slice->regeq_virtual.input_vregs, vreg_index);
      t_reg reg = find_vreg(slice->regeq_virtual.input_vregs, vreg_index);

      ASSERT(((in_reg_prop[ref_reg] == ARM_REG_NONE) ? ref_reg == reg : in_reg_prop[ref_reg] == reg) || (SliceSetPrint(slices, "BOE", slice_size), false),
              ("reference input register (r%d) does not match up (r%d): r%d", ref_reg, reg, in_reg_prop[ref_reg]));
    }

    /* watch out when a register is overwritten that is _conditionally_ defined in the slice! */
    for (size_t i = 0; i < slice_size; i++)
    {
      t_arm_ins *ins = T_ARM_INS(slice->Get(i, slice_size));

      if (!ArmInsIsConditional(ins))
        continue;

      t_reg r;
      REGSET_FOREACH_REG(ARM_INS_REGS_DEF(ins), r)
        if (in_reg_prop[r] != ARM_REG_NONE)
          return false;
    }

    /* patch up the in_reg_prop */
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++)
      if (RegsetIn(ref_slice->overwritten_registers, r))
        in_reg_prop[r] = ARM_REG_NONE;

    t_registers out_reg_prop;
    out_reg_prop.fill(ARM_REG_NONE);

    /* iterate over all the output registers */
    aliases.fill(ARM_REG_NONE);
    t_regset output_registers = NullRegs;
    for (size_t vreg_index = 0; vreg_index < ref_slice->regeq_virtual.output_vregs.size(); vreg_index++) {
      t_reg ref_reg = find_vreg(ref_slice->regeq_virtual.output_vregs, vreg_index);
      t_reg reg = find_vreg(slice->regeq_virtual.output_vregs, vreg_index);

      RegsetSetAddReg(output_registers, reg);

      /* these registers are equal */
      if (reg == ref_reg)
        continue;

#ifdef DEBUG_REG_EQ
      DEBUG(("need to put r%d in r%d", ref_reg, reg));
      print_array("aliases: ", aliases);
#endif

      t_reg location = find_register_containing_register(out_reg_prop, ref_reg);
#ifdef DEBUG_REG_EQ
      DEBUG(("  found r%d in r%d", ref_reg, location));
#endif

      if (reg == location)
        continue;

      AFAction action;
      action.code = AFActionCode::SwapRegisters;
      action.before = false;
      action.slice = slice;
      action.args[0].reg = reg;
      action.args[1].reg = location;

      aliases[reg] = location;

      GPRegistersAdd(overwritten_registers, reg);

      t_reg a = (out_reg_prop[location] == ARM_REG_NONE) ? location : out_reg_prop[location];
      t_reg b = (out_reg_prop[reg] == ARM_REG_NONE) ? reg : out_reg_prop[reg];

      out_reg_prop[reg] = a;

      bool no_backup_needed = false;

#if 0
      if (!RegsetIn(overwritten_registers, location)) {
        t_reg saved = find_register_containing_register_reverse(aliases, location);
        if (saved != ARM_REG_NONE)
          saved = find_register_containing_register_reverse(aliases, saved);

        no_backup_needed = (saved == ARM_REG_NONE);
      }
#endif

      if (no_backup_needed) {
#ifdef DEBUG_REG_EQ
        DEBUG(("output r%d need not be saved!", location));
#endif

        action.code = AFActionCode::AliasRegisters;
      }
      else {
        aliases[location] = reg;
        out_reg_prop[location] = b;
        GPRegistersAdd(overwritten_registers, location);
      }

      actions.push_back(action);
#ifdef DEBUG_REG_EQ_ACTION
      DEBUG(("O%d added action: %s", vreg_index, PrintAction(action).c_str()));
#endif
#ifdef DEBUG_REG_EQ
      print_array("aliases-end: ", aliases);
      print_array("out-reg-prop: ", out_reg_prop);
#endif
    }
#ifdef DEBUG_REG_EQ
    print_array("final output:", out_reg_prop);
#endif

    t_registers final_reg_prop;
    final_reg_prop.fill(ARM_REG_NONE);

    registers_still_needed = RegsetDup(registers_to_be_restored);
    REGSET_FOREACH_REG(registers_to_be_restored, reg) {
      t_reg backup_reg = reg;
      t_reg tmp;

      bool comes_from_input = true;

      /* from IN to OUT */
#ifdef DEBUG_REG_EQ
      stringstream ss;
      ss << "looking for r" << backup_reg;
#endif
      tmp = find_register_containing_register(in_reg_prop, backup_reg);
      backup_reg = tmp;
#ifdef DEBUG_REG_EQ
      ss << ", after input: r" << backup_reg;
#endif
      tmp = find_register_containing_register(out_reg_prop, backup_reg);
      if (tmp != backup_reg) comes_from_input = false;
      backup_reg = tmp;
#ifdef DEBUG_REG_EQ
      ss << ", after output: r" << backup_reg;
#endif
      tmp = find_register_containing_register(final_reg_prop, backup_reg);
      if (tmp != backup_reg) comes_from_input = false;
      backup_reg = tmp;
#ifdef DEBUG_REG_EQ
      ss << ", after restore: r" <<  backup_reg;
#endif

      /* this register has been restored */
      RegsetSetSubReg(registers_still_needed, reg);

      if (comes_from_input)
        RegsetSetAddReg(used_registers, backup_reg);

      /* this register does not need to be restored */
      if (backup_reg == reg)
        continue;

#ifdef DEBUG_REG_EQ
      DEBUG(("%s", ss.str().c_str()));
#endif

      /* restoring register 'reg' */
      AFAction action;
      action.code = AFActionCode::SwapRegisters;
      action.before = false;
      action.slice = slice;
      action.args[0].reg = reg;
      action.args[1].reg = backup_reg;

      GPRegistersAdd(overwritten_registers, reg);
      GPRegistersAdd(overwritten_registers, backup_reg);

      t_reg a = (final_reg_prop[backup_reg] == ARM_REG_NONE) ? backup_reg : final_reg_prop[backup_reg];
      t_reg b = (final_reg_prop[reg] == ARM_REG_NONE) ? reg : final_reg_prop[reg];

      final_reg_prop[reg] = a;
      final_reg_prop[backup_reg] = b;

      actions.push_back(action);
#ifdef DEBUG_REG_EQ_ACTION
      DEBUG(("restore r%d added action: %s", reg, PrintAction(action).c_str()));
#endif
    }
#ifdef DEBUG_REG_EQ
    print_array("final registers:", final_reg_prop);
    DEBUG(("coming from input: @X", CPREGSET(cfg, used_registers)));
#endif

    /* sanity check */
    for (t_reg r = ARM_REG_R0; r < ARM_REG_R15; r++) {
      /* skip dead registers */
      t_uint8 register_origin = REGISTER_ORIGIN_NONE;

      bool live_out = RegsetIn(slice->regeq_live_out, r);

#ifdef DEBUG_REG_EQ
      stringstream ss;
      ss << "r" << static_cast<int>(r);
#endif
      t_reg tmp = r;
      t_reg mapped = tmp;

      if (final_reg_prop[tmp] != ARM_REG_NONE) {
        tmp = final_reg_prop[tmp];
        mapped = tmp;
        register_origin = REGISTER_ORIGIN_INPUT;
#ifdef DEBUG_REG_EQ
        ss << " -> (after restore) r" << static_cast<int>(tmp);
#endif
      }
      if (out_reg_prop[tmp] != ARM_REG_NONE) {
        tmp = out_reg_prop[tmp];
        mapped = tmp;
        register_origin = REGISTER_ORIGIN_INPUT;
#ifdef DEBUG_REG_EQ
        ss << " -> (after output) r" << static_cast<int>(tmp);
#endif
      }

      /* skip if this is an output register */
      if (RegsetIn(output_registers, r)) {
        /* take special care of output registers */
        size_t vreg_index = find_vreg_index(slice->regeq_virtual.output_vregs, r);
        t_reg ref_register = find_vreg(ref_slice->regeq_virtual.output_vregs, vreg_index);

        register_origin = REGISTER_ORIGIN_SLICE;

#ifdef DEBUG_REG_EQ
        ss << " -> (slice) r" << static_cast<int>(ref_register);
#endif

        if (tmp != ref_register && live_out) {
#ifdef DEBUG_REG_EQ
          DEBUG(("mapping: %s", ss.str().c_str()));
#endif
          SliceSetPrint(slices, "OUTPUT-MISMATCH", slice_size);
          FATAL(("output register r%d does not match up!", r));
        }
      }
      else {
        /* from OUT to IN */
        if (in_reg_prop[tmp] != ARM_REG_NONE) {
          tmp = in_reg_prop[tmp];
          mapped = tmp;
          register_origin = REGISTER_ORIGIN_INPUT;
#ifdef DEBUG_REG_EQ
          ss << " -> (after input) r" << static_cast<int>(tmp);
#endif
        }

        /* translated register */
        if (tmp != r && live_out) {
#ifdef DEBUG_REG_EQ
          DEBUG(("mapping: %s", ss.str().c_str()));
#endif
          SliceSetPrint(slices, "", slice_size);
          FATAL(("r%d does not match up!", r));
        }
      }

      t_register_info register_info = ((register_origin == REGISTER_ORIGIN_NONE) || (tmp == r)) ? REGISTER_INFO_UNMODIFIED : ((register_origin << 4) | tmp);
      all_register_information_per_slice[slice][r] = register_info;
    }

    /* associate the actions with this slice */
    if (!actions.empty())
      action_list.insert(SliceSpecificActionListItem{slice, actions});

    slice_registers.insert(SliceSpecificRegistersItem{slice, used_registers});

    GPRegistersSetDiffRegset(overwritten_registers, slice->overwritten_registers);
    GPRegistersSetDiffRegset(overwritten_registers, registers_to_be_restored);
    overwritten_registers_per_slice[slice] = overwritten_registers;
  }

#ifdef DEBUG_REG_EQ
  if (!action_list.empty()) {
    SliceSetPrint(slices, "REGEQ", slice_size);
    /*  SliceSetPrint(slices, "REGEQBOE");
    for (auto slice : slices)
      DEBUG(("@iB", slice->Bbl()));*/
  }
#endif

  return true;
}
