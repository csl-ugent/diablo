#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

int *MapImmediatesToRegisters(SliceSet slices, size_t slice_size, bool *imm_to_reg, int& max_map)
{
  max_map = -1;

  /* -1 means 'no change needed' */
  auto result = new int[slice_size];

  /* clear the result vector initially */
  for (size_t i = 0; i < slice_size; i++)
    result[i] = -1;

  vector<t_int8 *> data;

  for (auto slice : slices)
  {
    /* create a temporary mapping for this slice */
    t_int8* mapping = new t_int8[slice_size];

    /* keep a map of immediates mapped to their unique identifier */
    map<t_uint64, int> imms;
    int counter = 0;
    for (size_t ins_idx = 0; ins_idx < slice_size; ins_idx++)
    {
      if (!imm_to_reg[ins_idx])
      {
        mapping[ins_idx] = -1;
        continue;
      }

      t_uint64 imm = ARM_INS_IMMEDIATE(T_ARM_INS(slice->GetR(ins_idx)));
      if (imms.find(imm) == imms.end())
      {
        imms[imm] = counter;
        counter++;
      }

      mapping[ins_idx] = imms[imm];
    }

    data.push_back(mapping);
  }

  int current_reg = 0;
  bool *imm_differs = new bool[slice_size];
  for (size_t i = 0; i < slice_size; i++) {
    imm_differs[i] = false;

    /* shortcut: immediate does not need to be registerized */
    if (!imm_to_reg[i]) {
      result[i] = -1;
      continue;
    }

    /* find out if different virtual registers were assigned */
    int idx = data[0][i];
    for (size_t ii = 1; ii < data.size(); ii++) {
      if (data[ii][i] != idx) {
        imm_differs[i] = true;
        break;
      }
    }

    if (imm_differs[i]) {
      /* different registers have been assigned,
       * we need to create a new virtual register */
      result[i] = current_reg++;
    }
    else {
      /* same register has been assigned,
       * maybe we can reuse a previous register */
      bool assign_new = true;

      for (size_t ii = 0; ii < i; ii++) {
        /* only check the non-differing instructions */
        if (imm_differs[ii])
          continue;

        if (data[0][i] == data[0][ii]) {
          /* match! */
          result[i] = result[ii];
          assign_new = false;
        }
      }

      if (assign_new)
        result[i] = current_reg++;
    }
  }

  for (auto x : data)
    delete[] x;
  delete[] imm_differs;

  max_map = current_reg-1;
  return result;
}

int EqualizeImmediates(SliceSet slices, size_t slice_size, bool *& have_to_equalize_insn)
{
  int nr_required_regs = 0;

  for (size_t i = 0; i < slice_size; i++)
  {
    have_to_equalize_insn[i] = false;

    /* do we see different immediates for this instruction? */
    t_int64 imm = 0;
    bool first = true;

    for (auto slice : slices)
    {
      t_arm_ins *ins = T_ARM_INS(slice->GetR(i));

      /* skip instructions that don't have an immediate */
      if (!ArmInsHasImmediateOp(ins))
        break;

      if (first)
      {
        /* just copy over the immediate value */
        imm = ARM_INS_IMMEDIATE(ins);
        first = false;
      }
      else if (imm != ARM_INS_IMMEDIATE(ins))
      {
        /* this instruction needs to be equalized, and we can early break the for-loop */
        have_to_equalize_insn[i] = true;
        break;
      }
    }
  }

  nr_required_regs = 0;
  auto list = MapImmediatesToRegisters(slices, slice_size, have_to_equalize_insn, nr_required_regs);
  delete[] list;

  /* nr_required_regs is set by the MapImmediatesToRegisters function, and equals the largest index
   * (zero-based) needed to allocate registers. Thus, to get the number of required registers, we
   * need to increment this counter by 1. */
  nr_required_regs++;

  return nr_required_regs;
}
