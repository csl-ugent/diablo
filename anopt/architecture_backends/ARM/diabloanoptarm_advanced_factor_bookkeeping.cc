#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

//#define DEBUG_NONZERO

#ifdef DEBUG_NONZERO
#define DEBUG_NZ(x) DEBUG(x)
#else
#define DEBUG_NZ(x)
#endif

void AssociateSliceAndBbl(t_bbl *bbl, Slice *slice)
{
  /* don't associate invalidated slices */
  if (slice->IsInvalidated())
    return;

  /* create the datastructure if necessary */
  if (!BBL_SLICES(bbl))
    BBL_SET_SLICES(bbl, new SliceSet());

  //DEBUG(("Associating\n   %s\n   WITH @iB", slice->Print().c_str(), bbl));
  BBL_SLICES(bbl)->insert(slice);
}

void DissociateSliceAndBbl(t_bbl *bbl, Slice *slice)
{
  if (!BBL_SLICES(bbl))
    return;

  //DEBUG(("Dissociating\n   %s\n   WITH @iB", slice->Print().c_str(), bbl));
  BBL_SLICES(bbl)->erase(slice);
}

void BblInvalidate(t_bbl *bbl)
{
  if (!BBL_SLICES(bbl))
    return;

  /* first we put all the associated slices in a vector because BBL_SLICES will
   * be modified by the invalidation function. */
  vector<Slice *> slices;
  for (auto slice : *BBL_SLICES(bbl))
    slices.push_back(slice);

  for (auto slice : slices)
    slice->Invalidate();
}

void UpdateAssociatedSlices(t_bbl *bbl)
{
  if (!BBL_SLICES(bbl))
    return;

  vector<Slice *> slices;
  for (auto it = BBL_SLICES(bbl)->begin(); it != BBL_SLICES(bbl)->end(); it++)
    slices.push_back(*it);
  for (auto slice : slices) {
    slice->UpdateInstructions();
    DissociateSliceAndBbl(bbl, slice);
    AssociateSliceAndBbl(slice->Bbl(), slice);
  }
}

void BblPropagateConstantInformation(t_bbl *bbl, t_analysis_complexity complexity)
{
  ASSERT(BBL_PROCSTATE_IN(bbl), ("no incoming procstate for @eiB", bbl));
  BblPropagateConstantInformation(bbl, complexity, NULL, NULL);
}

void BblPropagateCopyInformation(t_bbl *bbl, t_cfg_edge *edge)
{
  /* apparently this is needed...
   * TODO: find out why! */
  FunctionUnmarkAllBbls(BBL_FUNCTION(bbl));

  ASSERT(BBL_EQS_IN(bbl), ("no incoming equations for @eiB", bbl));
  BblCopyAnalysis(bbl, edge, FALSE);
}

void AfterSplit(t_bbl *first, t_bbl *second, bool propagate)
{
  BBL_SET_ORIGINAL_ID(second, BBL_ORIGINAL_ID(first));
  BBL_SET_AF_FLAGS(second, BBL_AF_FLAGS(first));

  BBL_SET_FACTORING_REGION_ID(second, BBL_FACTORING_REGION_ID(first));

  /* use/def regsets need to be recalculated */
  UpdateUseDef(first);
  UpdateUseDef(second);

  /* if we can't add an instruction to the first BBL */
  if (CanAddInstructionToBbl(first))
  {
    CfgPath p = {BBL_SUCC_FIRST(first)};
    auto new_incoming = new vector<CfgPath>();
    new_incoming->push_back(p);
    BBL_SET_INCOMING(second, new_incoming);
  }
  else if (BBL_INCOMING(first))
  {
    BBL_SET_INCOMING(second, new vector<CfgPath>(*BBL_INCOMING(first)));
  }

  if (propagate)
  {
    DistributedTableCopyIds(first, second);

  /* copy analysis information */
#if AF_COPY_ANALYSIS
    BBL_SET_EQS_IN(second, EquationsNew(BBL_CFG(second)));
    BblPropagateCopyInformation(first, BBL_SUCC_FIRST(first));
#endif

    DEBUG_NZ(("propagate @eiB @eiB", first, second));

    if (nonzero_init) {
      /* nonzero register information */
      BblCopyNonZeroRegistersAfter(first, second);

      t_regset second_before = BblNonZeroRegistersBefore(second, false, NullRegs);
      DEBUG_NZ(("  second_before @X", CPREGSET(BBL_CFG(first), second_before)));

      t_regset first_beforem = BblNonZeroRegistersBeforeM(first);
      DEBUG_NZ(("  first_beforem @X", CPREGSET(BBL_CFG(first), first_beforem)));

      t_regset first_after = BblCalculateNonZeroRegistersAfter(first, first_beforem);
      DEBUG_NZ(("  first_after @X", CPREGSET(BBL_CFG(first), first_after)));

      BblSetNonZeroRegistersAfter(first, RegsetUnion(second_before, first_after));

      t_regset first_after2 = BblNonZeroRegistersAfter(first);
      DEBUG_NZ(("  first_after2 @X", CPREGSET(BBL_CFG(first), first_after2)));

      BblSetNonZeroRegistersBeforeM(second, first_after2);
    }

    /* constant propagation information */

    /* first we propagate the constant information again in the first BBL,
     * so the output constant information is known */
    BblPropagateConstantInformation(first, CONTEXT_SENSITIVE);

    /* then we set the input constant information of the second BBL to the
     * constant information after the first BBL and finally calculate the
     * output constant information for the second BBL */
    ASSERT(BBL_PROCSTATE_OUT(first), ("no procstate out for @eiB to @eiB", first, second));
    BBL_SET_PROCSTATE_IN(second, ProcStateNewDup(BBL_PROCSTATE_OUT(first)));
    BblPropagateConstantInformation(second, CONTEXT_SENSITIVE);

    /* registers that are propagated over the edge */
    CFG_EDGE_SET_PROP_REGS(BBL_SUCC_FIRST(first), CFG_DESCRIPTION(BBL_CFG(first))->all_registers);
  }
}
