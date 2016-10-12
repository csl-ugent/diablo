/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>
#include <map>
#include <set>
#include <vector>

#include "ARM_obfuscations.h"
#include "ARM_flip_branches.h"

extern "C" {
#include "diabloarm.h"
}

using namespace std;

ArmFlipBranchesTransformation::ArmFlipBranchesTransformation() {
  bblsTransformed = 0;
}


bool ArmFlipBranchesTransformation::canTransform(const t_bbl* bbl) const {
  t_arm_ins* ins = T_ARM_INS(BBL_INS_LAST(bbl));
  
  if (!FlipBranchesTransformation::canTransform(bbl))
    return false;

  if(!ins)
    return false;
  
  if (ARM_INS_OPCODE(ins) != ARM_B)
    return false;

  return ARM_INS_CONDITION(ins) != ARM_CONDITION_AL;
}

/* Based on I386InvertBranchBbl */
bool ArmFlipBranchesTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_bbl * target_bbl = NULL, * split_off;
  t_cfg_edge * edge, * s_edge;
  t_arm_ins * ins;
  t_bool ipjump=FALSE;
  
  VERBOSE(1, ("Flipping branch in @eiB", bbl));

  ASSERT(canTransform(bbl), ("Expected a BBL that can be transformed @eiB", bbl));
  
  ins = T_ARM_INS(BBL_INS_LAST(bbl));

  ARM_INS_SET_CONDITION(ins, ArmInvertCondition(ARM_INS_CONDITION(ins)));

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge,s_edge)
  {
    if(CFG_EDGE_CAT(edge)==ET_FALLTHROUGH)
      CFG_EDGE_SET_CAT(edge,ET_JUMP);
    else if(CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
      CFG_EDGE_SET_CAT(edge,ET_IPJUMP);
    else if(CFG_EDGE_CAT(edge)==ET_JUMP)
    {
      target_bbl=CFG_EDGE_TAIL(edge);
      CfgEdgeKill(edge);
    }
    else if(CFG_EDGE_CAT(edge)==ET_IPJUMP)
    {
      ipjump=TRUE;
      target_bbl=CFG_EDGE_TAIL(edge);
      CfgEdgeKill(edge);
    }
    else
    {
      VERBOSE(0,("@E\n",edge));
      FATAL(("ERROR! No edge (Impossible!)"));
    }
  }

  split_off=BblNew(BBL_CFG(bbl));
  BblInsertInFunction(split_off,BBL_FUNCTION(bbl));

  GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(split_off);
  
  /* TODO: verify that this is correct */
  BBL_SET_REGS_LIVE_OUT(split_off, BBL_REGS_LIVE_OUT(bbl));


  CfgEdgeCreate(BBL_CFG(bbl),bbl,split_off,ET_FALLTHROUGH);

  if(ipjump)
    CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_IPJUMP);
  else
    CfgEdgeCreate(BBL_CFG(bbl),split_off,target_bbl,ET_JUMP);
  
  VERBOSE(1, ("After: @eiB ; @eiB ; @eiB", bbl, split_off, target_bbl));
  
  bblsTransformed++;

  return true;
}

void ArmFlipBranchesTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sBranchFlip_Stats,bbls_transformed,%i", prefix.c_str(), bblsTransformed));
}


