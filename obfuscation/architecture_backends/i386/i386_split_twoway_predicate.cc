/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>
#include <map>
#include <vector>

#include "i386_split_twoway_predicate.h"

extern "C" {
#include "diabloi386.h"
#include "diabloanopt.h"
}

using namespace std;

/* TODO: before all instructions */
static vector<t_ins*> FindPlacesWhereFlagsAreDead(const t_bbl* bbl) {
  t_regset used=RegsetNew();
  
  RegsetSetAddReg(used, I386_CONDREG_OF);
  RegsetSetAddReg(used, I386_CONDREG_SF);
  RegsetSetAddReg(used, I386_CONDREG_ZF);
  RegsetSetAddReg(used, I386_CONDREG_AF);
  RegsetSetAddReg(used, I386_CONDREG_PF);
  RegsetSetAddReg(used, I386_CONDREG_CF);

  if(RegsetIsEmpty(RegsetIntersect(BblRegsLiveBefore(bbl), used)))
    return { BBL_INS_FIRST(bbl) };
  return {};
}/*}}}*/

bool I386SplitWithTwoWayPredicateTransformation::canTwowaySplitBasicBlock(const t_bbl* bbl) const {
  t_ins* ins_;
  if (!BBL_INS_FIRST(bbl))
    return false;

  if (!SplitWithTwoWayPredicateTransformation::canTwowaySplitBasicBlock(bbl))
    return false;

  auto split_points = FindPlacesWhereFlagsAreDead(bbl);
  
  return split_points.size() > 0;
}

bool I386SplitWithTwoWayPredicateTransformation::bblSplitWithTwoWayOpaquePredicate(t_bbl* bbl, t_randomnumbergenerator* rng) {
  auto split_points = FindPlacesWhereFlagsAreDead(bbl);
  
  possibleSplitPoints += split_points.size();
  
  auto split_list_id = RNGGenerateWithRange(rng, 0, split_points.size() - 1);
  auto split_before = split_points.at(split_list_id);

  auto split_off = GetArchitectureInfo(bbl)->splitBasicBlockWithJump(bbl, split_before, true /* before */);

  t_i386_ins* ins;

  /* TODO: like in ARM, maybe also find instructions overwriting the flags register, and split there */
  I386MakeInsForBbl(Cmp, Append, ins, bbl, GetArchitectureInfo(bbl)->getGenericRandomRegister(rng), I386_REG_NONE, 0);
  addedCmp++;

  /* Make a conditional branch with a random condition code from the non-conditional branch */
  auto condition = t_i386_condition_code(RNGGenerateWithRange(rng, I386_CONDITION_O, I386_CONDITION_G ));
  
  I386MakeInsForBbl(CondJump, Append, ins, bbl, condition);

  /* So we add a SECOND ET_JUMP edge, so that we can unfold... */
  CfgEdgeNew(BBL_CFG(bbl), bbl, split_off, /*ET_JUMP*/ET_FALLTHROUGH);

  return true;
}

I386SplitWithTwoWayPredicateTransformation::I386SplitWithTwoWayPredicateTransformation() {
  possibleSplitPoints = 0;
  possibleFlagIns = 0;
  reusedFlagIns = 0;
  addedCmp = 0;
}

void I386SplitWithTwoWayPredicateTransformation::dumpStats(const std::string& prefix) {
  SplitWithTwoWayPredicateTransformation::dumpStats(prefix);
  VERBOSE(0, ("%sTwoWaySplit_Stats,split_points,%i", prefix.c_str(), possibleSplitPoints));
  VERBOSE(0, ("%sTwoWaySplit_Stats,possible_flag_ins,%i", prefix.c_str(), possibleFlagIns));
  VERBOSE(0, ("%sTwoWaySplit_Stats,reused_flag_ins,%i", prefix.c_str(), reusedFlagIns));
  VERBOSE(0, ("%sTwoWaySplit_Stats,added_cmp,%i", prefix.c_str(), addedCmp));
}

