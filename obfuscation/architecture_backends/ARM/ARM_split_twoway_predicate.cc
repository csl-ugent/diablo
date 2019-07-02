/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "ARM_obfuscations.h"
#include "ARM_split_twoway_predicate.h"
using namespace std;

ARMSplitWithTwoWayPredicateTransformation::ARMSplitWithTwoWayPredicateTransformation() {
  possibleSplitPoints = 0;
  possibleFlagIns = 0;
  reusedFlagIns = 0;
  addedCmp = 0;
}

/* TODO: factor out with Opaque Predicates (with bool skip_first_ins or so) */
static vector<t_ins*> FindPlacesWhereFlagsAreDead(const t_bbl* bbl) {
  t_regset used=RegsetNew();

  RegsetSetAddReg(used,ARM_REG_C_CONDITION);
  RegsetSetAddReg(used,ARM_REG_V_CONDITION);
  RegsetSetAddReg(used,ARM_REG_Z_CONDITION);
  RegsetSetAddReg(used,ARM_REG_N_CONDITION);

  t_ins* ins;
  t_regset live = RegsetNew();
  t_ins* first = BBL_INS_FIRST(bbl);
  
  vector<t_ins*> result;

  BBL_FOREACH_INS(bbl, ins) {
    /* As opposed to opaque predicates, we can split before the first ins */

    live=RegsetDup(InsRegsLiveBefore(ins));

    if(RegsetIsEmpty(RegsetIntersect(used,live))) {
      result.push_back(ins);
    }
  }
  
  return result;
}

bool ARMSplitWithTwoWayPredicateTransformation::canTwowaySplitBasicBlock(const t_bbl* bbl) const {
  /* TODO: this is a hack, we should 'just' replace mov lr, pc with an address producer, which is the proper fix. The problem
     that this fixes is when we split the BBL between the mov and the subsequent instruction, the lr will point to our code,
     which is problematic (especially when the kernel returns to the address in lr, which used to be after the kernel call, and now is before.
     This is copied from ARM_branch_function.cc / ARM opaque predicates, TODO: merge
  */
  t_ins* ins_;
  if (!BBL_INS_FIRST(bbl))
    return false;
  
  BBL_FOREACH_INS(bbl, ins_) {
    t_arm_ins* ins = T_ARM_INS(ins_);
    if (ARM_INS_OPCODE(ins) == ARM_MOV) {
      if (ARM_INS_REGA(ins) == ARM_REG_LR && ARM_INS_REGC(ins) == ARM_REG_PC) {
        VERBOSE(0, ("SKIPPED: @I", ins));
        return false;
      }
    }
  }
  
  /* From flattening; TODO: abstract away and reuse */
  switch (ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl)))) {
    case ARM_BX:
      if (ARM_INS_REGB(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_REG_PC)
        return false;
      break;
    case TH_BX_R15: /* BX R15 */
      return false;
    default: break;
  }

  if (!SplitWithTwoWayPredicateTransformation::canTwowaySplitBasicBlock(bbl))
    return false;

  auto split_points = FindPlacesWhereFlagsAreDead(bbl);
  
  return split_points.size() > 0;
}

bool ARMSplitWithTwoWayPredicateTransformation::bblSplitWithTwoWayOpaquePredicate(t_bbl* bbl, t_randomnumbergenerator* rng) {
  auto split_points = FindPlacesWhereFlagsAreDead(bbl);
  
  possibleSplitPoints += split_points.size();
  
  t_bool isthumb = ArmBblIsThumb(bbl);


  /* We have two options:
   * 1) In between the split point and the last place where flags are live, find an instruction that we can set FL_S on (for now heuristic: simple arithmetic insns),
   *    and let that enable the FL_S.
   * 2) Add an explicit CMP random_reg.
   * TODO: for now, we first try the first option, only if that fails, we perform the second strategy, but this could/should be randomizable, steerable, I guess.
   * The jump condition is a random condition out of the set. */
  
  auto split_list_id = RNGGenerateWithRange(rng, 0, split_points.size() - 1);
  auto split_before = split_points.at(split_list_id);
  
  /* Iterate backwards over the list of split_points to find the consecutive list of instructions leading up to the split points, where the flags were dead */
  int current_id = split_list_id - 1;
  auto ins_help = split_before;
  
  vector<t_arm_ins*> possible_ins;
  
  while ( current_id >= 0 ) {
    if (INS_IPREV(ins_help) == split_points.at(current_id)) {      
      t_arm_ins* arm_ins = T_ARM_INS(ins_help);
      
      switch (ARM_INS_OPCODE(arm_ins)) {
        case ARM_AND:
        case ARM_EOR:
        case ARM_SUB:
        case ARM_RSB:
        case ARM_ADD:
        case ARM_ADC:
        case ARM_MOV:
          possible_ins.push_back(arm_ins);
          break;
        default: break; /* Do nothing */
      }

      ins_help = INS_IPREV(ins_help);
    } else {
      break;
    }
    
    current_id--;
  }
  
  auto split_off = GetArchitectureInfo(bbl)->splitBasicBlockWithJump(bbl, split_before, true /* before */);

  t_arm_ins* ins;
  
  /* TODO: it is possible that, for code generation reasons, actually the FL_S is set in instructions after this one, even though flags are dead. It might
   * be beneficial for stealthiness to then remove that FL_S */
  /* TODO: when isthumb, some instructions are not encodable with FL_S; if we would have information about whether or not we can actually encode the FL_S, we can enable the isthumb here again*/
  if (possible_ins.size() > 0 && !isthumb) {
    /* Case 1: Select a random point right before our split point where the flags are still dead, and where we can add FL_S */
    
    /* TODO: actually, the would be an instruction that sets a register that is used in the successor (split off) BBL that is used for indexing */
    ins = possible_ins.at(RNGGenerateWithRange(rng, 0, possible_ins.size() - 1));
    
    VERBOSE(1, ("Setting FL_S for @I in @eiB", ins, bbl));
    
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) | FL_S);
    
    reusedFlagIns++;
    possibleFlagIns += possible_ins.size();
  } else {
    /* Case 2: Add a compare instruction on a random register */
    /* TODO: currently we compare to the value 0, this might be another randomizable value */
    /* TODO: actually, the best random register here would be one that is used in the successor (split off) BBL that is used for indexing */
    
    ArmMakeInsForIns(Cmp, Before, ins, T_ARM_INS(BBL_INS_LAST(bbl)), isthumb, GetArchitectureInfo(bbl)->getGenericRandomRegister(rng), ARM_REG_NONE, 0, ARM_CONDITION_AL);
    
    addedCmp++;
  }

  /* Make a conditional branch with a random condition code from the non-conditional branch */
  /* TODO: ARM_CONDITION_CS ... ARM_CONDITION_HI sound like they're not used much, maybe don't generate them */
  //auto condition = RNGGenerateWithRange(rng, ARM_CONDITION_EQ, ARM_CONDITION_LE );
  auto condition = RNGGenerateWithRange(rng, ARM_CONDITION_EQ, ARM_CONDITION_NE /* ARM_CONDITION_LE TODO, more conditions!: triggers bug in deflowgraph? */ );
  
  ArmInsMakeCondBranch(T_ARM_INS(BBL_INS_LAST(bbl)), condition /* ! */);

  /* So we add a SECOND ET_JUMP edge, so that we can unfold... */
  CfgEdgeNew(BBL_CFG(bbl), bbl, split_off, /*ET_JUMP*/ET_FALLTHROUGH);

  return true;
}

void ARMSplitWithTwoWayPredicateTransformation::dumpStats(const std::string& prefix) {
  SplitWithTwoWayPredicateTransformation::dumpStats(prefix);
  VERBOSE(0, ("%sTwoWaySplit_Stats,split_points,%i", prefix.c_str(), possibleSplitPoints));
  VERBOSE(0, ("%sTwoWaySplit_Stats,possible_flag_ins,%i", prefix.c_str(), possibleFlagIns));
  VERBOSE(0, ("%sTwoWaySplit_Stats,reused_flag_ins,%i", prefix.c_str(), reusedFlagIns));
  VERBOSE(0, ("%sTwoWaySplit_Stats,added_cmp,%i", prefix.c_str(), addedCmp));
}
