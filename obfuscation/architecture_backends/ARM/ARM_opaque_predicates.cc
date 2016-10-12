/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>
#include <map>

#include "ARM_obfuscations.h"
#include "ARM_opaque_predicates.h"

extern "C" {
#include "diabloarm.h"
#include "diabloanopt.h"
}

using namespace std;

ARMOpaquePredicateTransformation::ARMOpaquePredicateTransformation() {
  insts_in_bbls = 0;
  bblsTransformed = 0;
  possibleSplitPoints = 0;
}

void ARMOpaquePredicateTransformation::initConstantTable(t_cfg* cfg) {
  t_function* fun;
  t_bbl* bbl;
  t_ins* ins_;
  t_arm_ins* ins;

  CFG_FOREACH_FUN(cfg, fun) {
    FUNCTION_FOREACH_BBL(fun, bbl) {
      BBL_FOREACH_INS(bbl, ins_) {
        ins = T_ARM_INS(ins_);
        
        if (   (ARM_INS_FLAGS(ins) & FL_IMMED)
            || (ARM_INS_OPCODE(ins) == ARM_CONSTANT_PRODUCER) ) {
          programConstants.insert(ARM_INS_IMMEDIATE(ins)); // TODO: IMMEDIATE is actually 64 bits, wrap around for now
        }
      }
    }
  }
}

t_uint32 ARMOpaquePredicateTransformation::getRandomProgramConstantLE(t_cfg* cfg, t_uint32 le, t_randomnumbergenerator* rng, bool consecutive_mask) {
  if (programConstants.empty())
    initConstantTable(cfg);
  
  // TODO: actually use the constants found in the program! :-)
  ASSERT(!consecutive_mask, ("TODO: implement consecutive_mask=true"));
  
  return RNGGenerateWithRange(rng, 0, le);
}

void ARMOpaquePredicateTransformation::logBegin(t_bbl* bbl) {
  t_function* fun = BBL_FUNCTION(bbl);

  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_OBF_OOP, "OpaquePredicate,%x,%s,'%s'", BBL_CADDRESS(bbl), FUNCTION_NAME(fun), name()) {
    AddTransformedBblToLog("OpaquePredicate", bbl);
    LogFunctionTransformation("before", fun);
  }
}

void ARMOpaquePredicateTransformation::logEnd(t_bbl* bbl) {
  t_function* fun = BBL_FUNCTION(bbl);

  LOG_MORE(L_OBF_OOP) { LogFunctionTransformation("after", fun); }
  STOP_LOGGING_TRANSFORMATION(L_OBF_OOP);
}

 /* TODO: make generic! (Is made more generic right now in ArchitectureInfo, use that!) */
static t_bbl* SplitBasicBlockWithJump(t_bbl* bbl, t_ins* ins, t_bool before) {
  //VERBOSE(0, ("BEFORE SPLIT @eiB", bbl));
  t_bbl* split_off = BblSplitBlock(bbl, ins, before);
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  t_arm_ins* ins_ = T_ARM_INS(ins);
  
  t_bool isthumb = ArmBblIsThumb(bbl);

  ArmMakeInsForBbl(UncondBranch, Append, ins_, bbl, isthumb);

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, split_off, ET_JUMP);

  //VERBOSE(0, ("AFTER SPLIT @eiB ;;;;; @eiB", bbl, split_off));
  
  return split_off;
}

vector<t_ins*> FindPlacesWhereFlagsAreDead(const t_bbl* bbl) {
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
    if (ins == first) /* Don't split before the first ins... */
      continue;

    live=RegsetDup(InsRegsLiveBefore(ins));

    if(RegsetIsEmpty(RegsetIntersect(used,live))) {
      result.push_back(ins);
    }
  }
  
  return result;
}


t_bbl* ARMOpaquePredicateTransformation::splitBasicBlockWhereFlagsAreDeadBlockWithJump(t_bbl* bbl, t_randomnumbergenerator* rng) {
  auto split_points = FindPlacesWhereFlagsAreDead(bbl);
  auto ins_split    = split_points.at(RNGGenerateWithRange(rng, 0, split_points.size() - 1));
  
  possibleSplitPoints += split_points.size();
  
  VERBOSE(1, ("Splitting BBL @eiB at @I", bbl, ins_split));

  return SplitBasicBlockWithJump(bbl, ins_split, TRUE /* before */);
}

bool ARMOpaquePredicateTransformation::canTransform(const t_bbl* bbl) const {
  if (!OpaquePredicateTransformation::canTransform(bbl))
    return false;

  /* check >= 2 registers available, or pop? TODO */
  
    /* TODO: this is a hack, we should 'just' replace mov lr, pc with an address producer, which is the proper fix. The problem
     that this fixes is when we split the BBL between the mov and the subsequent instruction, the lr will point to our code,
     which is problematic (especially when the kernel returns to the address in lr, which used to be after the kernel call, and now is before.
     This is copied from ARM_branch_function.cc, TODO: merge
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
  
  
  return FindPlacesWhereFlagsAreDead(bbl).size() > 0;
}

void ARMOpaquePredicateTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%s%sOpaque_Stats,bbls_transformed,%i", prefix.c_str(), name(), bblsTransformed));
  VERBOSE(0, ("%s%sOpaque_Stats,split_points,%i", prefix.c_str(), name(), possibleSplitPoints));
  VERBOSE(0, ("%s%sOpaque_Stats,insts_in_bbls,%i", prefix.c_str(), name(), insts_in_bbls));
}


/* TODO make more easy to program! */
bool ARMXX2OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));

  
#if 1
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  
  /* TODO: use constant propagation to do this on registers that look variable */
  t_regset not_in = RegsetNew();
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), not_in, rng);
  
  RegsetSetAddReg(not_in, reg_1.reg);

  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), not_in, rng);

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);
  
  vector<MaybeSaveRegister> all_regs { reg_1, reg_2 }; /* TODO make cleaner by doing it above */
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);

  /* Since this equation is unchanged mod 2^32, no need to prevent overflow */
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_2.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Add, Append, ins, bbl, isthumb, reg_1.reg, reg_1.reg, reg_2.reg, 0 /* immediate */, ARM_CONDITION_AL); /* could also be reg_2 */
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_1.reg, ARM_REG_NONE, 1 /* immediate */, ARM_CONDITION_AL); /* could also be reg_2 */
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_1.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_EQ /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  /* Not needed to add an edge bbl -> split_off, that's already done in SplitBasicBlockWhereFlagsAreDeadBlockWithJump */
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);
#endif
  return true;
}

/* INPUTS: reg_1 (no input sanitization needed)
 * ADD reg_x, reg_1, reg_1
 * AND reg_y, reg_x, 1
 * CMP reg_y, 0
 * always EQUAL
 */
bool ARM2DivXXOpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  
  /* TODO: use constant propagation to do this on registers that look variable */
  t_regset not_in = RegsetNew();
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), not_in, rng);
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), not_in, rng);
  MaybeSaveRegister reg_y = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), not_in, rng);
  vector<MaybeSaveRegister> all_regs { reg_x, reg_y };

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  t_function* fun = BBL_FUNCTION(bbl);
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);

  ArmMakeInsForBbl(Add, Append, ins, bbl, isthumb, reg_x.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_y.reg, reg_x.reg, ARM_REG_NONE, 1 /* immediate */, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_y.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_EQ /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);

  return true;
}

/* INPUTS: reg_x
 * SANITIZE reg_1 <- reg_x: &0xffff (according to Matias/Bertrand)
 * MUL reg_2, reg_1, reg_1
 * CMP reg_2, 0
 * always GREATER OR EQUAL
 */
bool ARMX2GE0OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  vector<MaybeSaveRegister> all_regs { reg_1, reg_2 };

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);

  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng); // TODO: 0xffff
  VERBOSE(1, ("Random x mask: %x", x_mask));
  
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_2.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_2.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_GE /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);

  return true;
}

/* INPUTS: reg_x
 * SANITIZE reg_1 <- reg_x & 0xffff (according to Matias/Bertrand)
 * MUL reg_2, reg_1, reg_1
 * AND reg_3, reg_2, 2   ; (ALTERNATIVE: SAR 1, and AND 1 maybe the SAR can even go in the AND 1?)
 * CMP reg_3, 0 // TODO: combine this and the previous instruction into a TST
 * always EQUAL
 */
bool ARMX2DivX22OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_3 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  vector<MaybeSaveRegister> all_regs { reg_1, reg_2, reg_3 };

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);

  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng); // TODO: 0xffff
  VERBOSE(1, ("Random x mask: %x", x_mask));
  
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_2.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_3.reg, reg_2.reg, ARM_REG_NONE, 2, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_3.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_EQ /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);

  return true;
}

/* INPUTS: reg_x
 * TODO: actually, this mask NEEDS to be either 0xffff'ish, or be placed before the TST reg_x, otherwise it can interfere with the or part of 2| v 8|
 * SANITIZE reg_1 <- reg_x & 0xffff (Matias/Bertrand)
 * TST reg_1, 1
 * If Equal: Ok! (1 AND reg_x sets Zero flag -> divides => jump equal)
 * Fallthrough:
 * MUL reg_2, reg_1, reg_1
 * SUB reg_3, reg_2, 1
 * AND reg_4, reg_3, 7
 * CMP reg_4, 0
 * always EQUAL
 */
bool ARMX2DivXv8DivX21OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_3 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_4 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  vector<MaybeSaveRegister> all_regs { reg_1, reg_2, reg_3, reg_4 };

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng); // TODO: 0xffff
  VERBOSE(1, ("Random x mask: %x", x_mask));
  
  t_bbl* bbl_or = BblNew(BBL_CFG(bbl));
  BblInsertInFunction(bbl_or, fun);

  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);
  
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Tst, Append, ins, bbl, isthumb, reg_1.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_EQ /* ! */);
  
  ArmMakeInsForBbl(Mul, Append, ins, bbl_or, isthumb, reg_2.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Sub, Append, ins, bbl_or, isthumb, reg_3.reg, reg_2.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  ArmMakeInsForBbl(And, Append, ins, bbl_or, isthumb, reg_4.reg, reg_3.reg, ARM_REG_NONE, 7, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl_or, isthumb, reg_4.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  ArmMakeInsForBbl(CondBranch, Append, ins, bbl_or, isthumb, ARM_CONDITION_EQ /* ! */);
  
  t_bbl* restore_bbl = BblNew(BBL_CFG(bbl));
  BblInsertInFunction(restore_bbl, fun);
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, restore_bbl, false /* push */);
  // So that if the above does not restore anything, we don't produce empty BBLs, can be removed with branch elimination
  ArmMakeInsForBbl(CondBranch, Append, ins, restore_bbl, isthumb, ARM_CONDITION_AL /* ! */);


  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    ASSERT(CFG_EDGE_CAT(edge) == ET_JUMP, ("Expected to have a direct jump edge here"));
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, restore_bbl, ET_JUMP);
  CfgEdgeCreate(BBL_CFG(bbl), bbl, bbl_or, ET_FALLTHROUGH);
  CfgEdgeCreate(BBL_CFG(bbl), bbl_or, false_path, ET_FALLTHROUGH);  
  CfgEdgeCreate(BBL_CFG(bbl), bbl_or, restore_bbl, ET_JUMP);  

  CfgEdgeCreate(BBL_CFG(bbl), restore_bbl, split_off, ET_JUMP);

  logEnd(bbl);

  return true;
}

/* INPUTS: reg_x, reg_y
 * SANITIZE x, y (sadly, otherwise x and y could be alive)
 * MUL reg_y_2, reg_y, reg_y
 * MOV reg_8y_2, reg_y_2 LSL 3
 * SUB reg_7y_2, reg_8y_2, reg_y_2
 * SUB reg_rhs, reg_7y_2, 1
 * MUL reg_lhs, reg_x, reg_x
 * CMP reg_rhs, reg_lhs
 * always NOT EQUAL
 */
bool ARM7Y2OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
#if 1
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_y = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng); /* Can be the same as reg_x */

  RegsetSetAddReg(force_live, reg_x.reg);
  MaybeSaveRegister reg_y_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_y_2.reg); /* reg_y_2 is should be kept live for the sub */
  MaybeSaveRegister reg_8y_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetSubReg(force_live, reg_y_2.reg); /* reg_y_2 is dead after this point */
  MaybeSaveRegister reg_7y_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_lhs  = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetSubReg(force_live, reg_x.reg);
  RegsetSetAddReg(force_live, reg_lhs.reg); /* lhs and rhs should be different! */
  MaybeSaveRegister reg_rhs  = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  
  vector<MaybeSaveRegister> all_regs { reg_x, reg_y, reg_y_2, reg_8y_2, reg_7y_2, reg_lhs, reg_rhs }; /* TODO make cleaner by doing it above */
  VERBOSE(1, ("LIVENESS: @X", &arm_description, InsRegsLiveBefore(livepoint)));
  VERBOSE(1, ("Save reg_y_2 (%i): %i", reg_y_2.reg, (int) reg_y_2.save));
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);
  
  InsKill(livepoint); /* Now that we have the liveness info, we can kill the unconditional jump */

  
  /* Sanitize */
  /* TODO: actually, the output of the sanitize can be different from reg_x/reg_y, so they can be alive after all! */
  /* TODO: if we find enough dead registers, use constant producers to put a random large number in those registers here to AND afterwards; otherwise stick with 0xff for efficiency */
  //t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xffff, rng);
  //t_uint32 y_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0x3fff, rng);
  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng);
  t_uint32 y_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng);
  
  VERBOSE(1, ("Random x mask: %x", x_mask));
  VERBOSE(1, ("Random y mask: %x", y_mask));

  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_x.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL); /* TODO: <= 0xffff */
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_y.reg, reg_y.reg, ARM_REG_NONE, y_mask, ARM_CONDITION_AL); /* TODO: <= 0x3fff */
  
  /* Compute LHS */
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_y_2.reg, reg_y.reg, reg_y.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mov, Append, ins, bbl, isthumb, reg_8y_2.reg, reg_y_2.reg, 0, ARM_CONDITION_AL);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_IMM); ARM_INS_SET_SHIFTLENGTH(ins, 3);

  ArmMakeInsForBbl(Sub, Append, ins, bbl, isthumb, reg_7y_2.reg, reg_8y_2.reg, reg_y_2.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Sub, Append, ins, bbl, isthumb, reg_rhs.reg, reg_7y_2.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  
  /* Compute RHS */
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_lhs.reg, reg_x.reg, reg_x.reg, 0, ARM_CONDITION_AL);

  /* Predicate */
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_lhs.reg, reg_rhs.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_NE /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  /* Not needed to add an edge bbl -> split_off, that's already done in SplitBasicBlockWhereFlagsAreDeadBlockWithJump */
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);

  VERBOSE(1, ("INSERTED: pre-BBL @eiB, after-BBL @eiB", bbl, split_off));
#endif
  return true;
}


/* sum of all odd i's in the range i=1 .. 2*x-1 (inclusive) = x^2 */
bool ARMArboitPredicateSumOpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));
  
  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_1.reg);

  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  MaybeSaveRegister reg_i = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_i.reg);
  MaybeSaveRegister reg_bound = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_bound.reg);
  MaybeSaveRegister reg_sum = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_sum.reg);
  RegsetSetSubReg(force_live, reg_i.reg);
  
  MaybeSaveRegister reg_3 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  vector<MaybeSaveRegister> all_regs { reg_1, reg_2, reg_3, reg_i, reg_bound, reg_sum };

  InsKill(BBL_INS_LAST(bbl)); /* We'll be adding a conditional jump, remove the unconditional one that was inserted at the split point */
  
  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng); // This must be low, otherwise the overhead will be too large
  VERBOSE(1, ("Random x mask: %x", x_mask));
  
  t_bbl* bbl_loop = BblNew(BBL_CFG(bbl));
  BblInsertInFunction(bbl_loop, fun);
  t_bbl* bbl_end = BblNew(BBL_CFG(bbl));
  BblInsertInFunction(bbl_end, fun);

  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);
  
  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL);
  // So that we have at least one summand
  ArmMakeInsForBbl(Add, Append, ins, bbl, isthumb, reg_1.reg, reg_1.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);   /* TODO: randomize var? */
  ArmMakeInsForBbl(Add, Append, ins, bbl, isthumb, reg_2.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Sub, Append, ins, bbl, isthumb, reg_bound.reg, reg_2.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mov, Append, ins, bbl, isthumb, reg_sum.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mov, Append, ins, bbl, isthumb, reg_i.reg, ARM_REG_NONE, 3, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_bound.reg, ARM_REG_NONE, 3, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_LT /* ! */); // BELOW

  ArmMakeInsForBbl(Add, Append, ins, bbl_loop, isthumb, reg_sum.reg, reg_sum.reg, reg_i.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Add, Append, ins, bbl_loop, isthumb, reg_i.reg, reg_i.reg, ARM_REG_NONE, 2, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Cmp, Append, ins, bbl_loop, isthumb, reg_i.reg, reg_bound.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl_loop, isthumb, ARM_CONDITION_LE /* ! */); // AE
  
  /* TODO: prepend in reverse order as temporary HACK until saveOrRestoreRegisters always Appends! */
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl_end, false /* push */);
  ArmMakeInsForBbl(Cmp, Prepend, ins, bbl_end, isthumb, reg_3.reg, reg_sum.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mul, Prepend, ins, bbl_end, isthumb, reg_3.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl_end, isthumb, ARM_CONDITION_EQ /* ! */); // Always equal

  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    ASSERT(CFG_EDGE_CAT(edge) == ET_JUMP, ("Expected to have a direct jump edge here"));
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, bbl_end, ET_JUMP);
  CfgEdgeCreate(BBL_CFG(bbl), bbl, bbl_loop, ET_FALLTHROUGH);

  CfgEdgeCreate(BBL_CFG(bbl), bbl_loop, bbl_loop, ET_JUMP);  
  CfgEdgeCreate(BBL_CFG(bbl), bbl_loop, bbl_end, ET_FALLTHROUGH);  

  CfgEdgeCreate(BBL_CFG(bbl), bbl_end, split_off, ET_JUMP);
  CfgEdgeCreate(BBL_CFG(bbl), bbl_end, false_path, ET_FALLTHROUGH);

  logEnd(bbl);

  return true;
}
/*
 * Mask: 1023 0x3ff
 * Dividend Implemented as x*(x^2-1)
 * MUL r_square, r_1, r_1
 * SUB r_2, r_square, 1
 * MUL r_x3x, r_2, r_1
 * 
 * We compute modulo 3 by multiplying with 43690 << 16 + 43691 (discarding lower order bits), which is basically dividing by 3
 * Then subtract that result, multiplied by 3 (shifting that by lsr #1 -> r, add r, r, r, asl 1) from the original target with rsb target, r, multiplication result, resulting
 * in the remainder.
 * MOV r_div, 2863311531
 * UMULL r_mul_lo, r_mul, r_x3x, r_div WARNING: ARM assembly confusingly puts the RdLo left of RdHi
 * MOV r_3, r_mul, lsr #1
 * ADD r_4, r_3, r_3, asl #1
 * RSB r_dest, r_4, r_x3x
 * CMP r_dest, 0
 * ALWAYS equal
 */
bool ARM3DivX3_3OpaquePredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_arm_ins* ins;
  bblsTransformed++;
  insts_in_bbls += BBL_NINS(bbl);

  logBegin(bbl);

  t_bool isthumb = ArmBblIsThumb(bbl);
  if (isthumb) VERBOSE(1, ("Transforming a THUMB bbl!"));

  t_bbl* split_off = splitBasicBlockWhereFlagsAreDeadBlockWithJump(bbl, rng);
  t_function* fun = BBL_FUNCTION(bbl);
  /* TODO make generic on which predicate this depends */
  t_bbl* false_path = SelectTargetFor(bbl, rng, true /* need_to_fall_through */);

  t_ins* livepoint = BBL_INS_LAST(bbl);
  t_regset force_live = RegsetNew();

  /* TODO: use constant propagation to do this on registers that look variable. */
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_1 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_1.reg);

  MaybeSaveRegister reg_square = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_2 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_x3x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_x3x.reg);
  RegsetSetSubReg(force_live, reg_1.reg);
  
  MaybeSaveRegister reg_div = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_div.reg);
  
  MaybeSaveRegister reg_mul = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_mul.reg); /* Register allocation constraint UMULL: RdLo != RdHi */
  MaybeSaveRegister reg_mul_lo = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetSubReg(force_live, reg_mul.reg);

  MaybeSaveRegister reg_3 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  MaybeSaveRegister reg_4 = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetSubReg(force_live, reg_x3x.reg);
  
  MaybeSaveRegister reg_dest = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  vector<MaybeSaveRegister> all_regs { reg_1, reg_square, reg_2, reg_x3x, reg_div, reg_mul, reg_mul_lo, reg_3, reg_4, reg_dest  };

  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, bbl, true /* push */);
  
  InsKill(livepoint); /* Now that we have the liveness info, we can kill the unconditional jump */

  
  /* Sanitize */
  t_uint32 x_mask = getRandomProgramConstantLE(BBL_CFG(bbl), 0xff, rng); // TODO: 0x3ff

  VERBOSE(1, ("Random x mask: %x", x_mask));

  ArmMakeInsForBbl(And, Append, ins, bbl, isthumb, reg_1.reg, reg_x.reg, ARM_REG_NONE, x_mask, ARM_CONDITION_AL);

  /* Compute (x^3-x) */
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_square.reg, reg_1.reg, reg_1.reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Sub, Append, ins, bbl, isthumb, reg_2.reg, reg_square.reg, ARM_REG_NONE, 1, ARM_CONDITION_AL);
  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_x3x.reg, reg_2.reg, reg_1.reg, 0, ARM_CONDITION_AL);

  /* The division */

  ArmMakeInsForBbl(Mov, Append, ins, bbl, isthumb, reg_div.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL); // TODO TODO
  ArmMakeConstantProducer(ins, 2863311531);

  ArmMakeInsForBbl(Mul, Append, ins, bbl, isthumb, reg_mul.reg, reg_x3x.reg, reg_div.reg, 0, ARM_CONDITION_AL);
  ARM_INS_SET_OPCODE(ins, ARM_UMULL);
  ARM_INS_SET_REGS(ins, reg_mul_lo.reg); // TODO: this seems to be correct for encoding A1 combined with the ARM assembler, verify with Thumb2 (and maybe make this more userfriendly)
  
  ArmMakeInsForBbl(Mov, Append, ins, bbl, isthumb, reg_3.reg, reg_mul.reg, 0, ARM_CONDITION_AL);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSR_IMM); ARM_INS_SET_SHIFTLENGTH(ins, 1);

  ArmMakeInsForBbl(Add, Append, ins, bbl, isthumb, reg_4.reg, reg_3.reg, reg_3.reg, 0, ARM_CONDITION_AL);
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_IMM); ARM_INS_SET_SHIFTLENGTH(ins, 1);
  
  ArmMakeInsForBbl(Sub, Append, ins, bbl, isthumb, reg_dest.reg, reg_4.reg, reg_x3x.reg, 0, ARM_CONDITION_AL);
  ARM_INS_SET_OPCODE(ins, ARM_RSB);
  
  /* Predicate */
  ArmMakeInsForBbl(Cmp, Append, ins, bbl, isthumb, reg_dest.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, bbl, isthumb, ARM_CONDITION_EQ /* ! */);
  
  CfgEdgeCreate(BBL_CFG(bbl),bbl, false_path, ET_FALLTHROUGH);
  /* Not needed to add an edge bbl -> split_off, that's already done in SplitBasicBlockWhereFlagsAreDeadBlockWithJump */
  
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(all_regs, split_off, false /* push */);

  logEnd(bbl);

  return true;
}


