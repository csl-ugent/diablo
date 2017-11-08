/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

#include "ARM_obfuscations.h"
#include "ARM_branch_function.h"

extern "C" {
#include "diabloarm.h"
}

/* TODO: these branch functions do not just replace branches; they SPLIT the BBL, insert unconditional jump, and THEN replace that jump. Make that clearer / more generic! */

using namespace std;

AbstractARMRegisterBasedBranchFunctionTransformation::AbstractARMRegisterBasedBranchFunctionTransformation() {
}

 /* TODO: make generic! */
static t_bbl* SplitBasicBlockWithJump(t_bbl* bbl, t_ins* ins, t_bool before) {
  //VERBOSE(0, ("BEFORE SPLIT @eiB", bbl));
  t_bbl* split_off = BblSplitBlock(bbl, ins, before);
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  t_arm_ins* ins_ = T_ARM_INS(ins);
  
  t_bool isThumb = ArmBblIsThumb(bbl);

  ArmMakeInsForBbl(UncondBranch, Append, ins_, bbl, isThumb);

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, split_off, ET_JUMP);

  //VERBOSE(0, ("AFTER SPLIT @eiB ;;;;; @eiB", bbl, split_off));
  
  return split_off;
}

static t_bbl* RandomSplitBasicBlockWithJump(t_bbl* bbl, t_randomnumbergenerator* rng) {
  t_ins* ins;
  int i = 0;
  int splitbefore_nr = RNGGenerateWithRange(rng, 1, BBL_NINS(bbl) - 1);
  
  t_bool isThumb = ArmBblIsThumb(bbl);
  
  VERBOSE(1, ("Splitting BBL at %i out of %i", splitbefore_nr, BBL_NINS(bbl)));

  BBL_FOREACH_INS(bbl, ins) {
    if (i == splitbefore_nr) {
      return SplitBasicBlockWithJump(bbl, ins, TRUE /* before */);
    }
    i++;
  }
  
  ASSERT(0, ("Split before nr %i failed after %i in @eiB", splitbefore_nr, i, bbl));
  return 0;
}

t_bbl* AbstractARMRegisterBasedBranchFunctionTransformation::getStackBranchFunction(t_cfg* cfg, t_bool isThumb, t_reg reg) {
  auto found = reg_stackbranchfunction_map.find( make_pair(isThumb, reg) );
  
  if (found != reg_stackbranchfunction_map.end())
        return (*found).second;
  
 t_bbl* bf = createStackBranchFunctionForRegister(cfg, isThumb, reg);
 reg_stackbranchfunction_map.insert(make_pair( make_pair(isThumb, reg) , bf));
 
 return bf;
}

void ARMRegisterBasedBranchFunctionTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sBranch_Stats,bbls_transformed,%i", prefix.c_str(), bblsTransformed));
  VERBOSE(0, ("%sBranch_Stats,split_points,%i", prefix.c_str(), possibleSplitPoints));
  VERBOSE(0, ("%sBranch_Stats,insts_in_bbls,%i", prefix.c_str(), insts_in_bbls));
}

/* Register-based branch function */
/* at the end of the BBL that contains the jump: 
 stmfd	sp!, {r0,lr} @ also not stealthy...
 mov		r0, #4 @ .BF_FT - .BF_FAKE_FT, this is the 'argument'
 bl		branchfun_r0 <- depends on the register!
 bf_fake_ft:
 
.BF_FT1:
	ldmfd	sp!, {r0,lr} @ Not stealthy!
orig target:
	actual code continues


branchfun_r0:	add	pc, lr, r0 */

t_bbl* ARMRegisterBasedBranchFunctionTransformation::createStackBranchFunctionForRegister(t_cfg* cfg, t_bool isThumb, t_reg reg) {
  /*
    ARM mode: add	pc, lr, r0
    Thumb2 mode:
      add pc, lr, r0
      bx  lr
  */

  t_arm_ins* ins;
  t_function* fun;
  t_bbl* bbl =BblNew(cfg);
  t_bbl* retBlock = BblNew(cfg);

  fun=FunctionMake(bbl, StringIo("BranchFunctionStackR%i_%s", (int) reg, isThumb ? "Thumb2" : "ARM"), FT_NORMAL);

  t_regset regs;
  
  if (isThumb) {
    ArmMakeInsForBbl(Add,  Append, ins, bbl, isThumb, ARM_REG_LR, ARM_REG_LR, reg, 0 /* immediate */, ARM_CONDITION_AL);
    ArmMakeInsForBbl(UncondBranch, Append, ins, bbl, isThumb);
    ARM_INS_SET_OPCODE(ins, ARM_BX); ARM_INS_SET_REGB(ins, ARM_REG_LR); ARM_INS_SET_FLAGS(ins, FL_THUMB); /* Ensure FL_IMMED is off */
  } else {
    ArmMakeInsForBbl(Add,  Append, ins, bbl, isThumb, ARM_REG_PC, ARM_REG_LR, reg, 0 /* immediate */, ARM_CONDITION_AL);
  }

  VERBOSE(1, ("Added branch function %i %i @eiB", (int) isThumb, (int) reg, bbl));

  return bbl;
}
bool ARMRegisterBasedBranchFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!BranchFunctionTransformation::canTransform(bbl)) {
	return false;
  }

  /* TODO: this is a hack, we should 'just' replace mov lr, pc with an address producer, which is the proper fix. The problem
     that this fixes is when we split the BBL between the mov and the subsequent instruction, the lr will point to our code,
     which is problematic (especially when the kernel returns to the address in lr, which used to be after the kernel call, and now is before.
  */
  t_ins* ins_;
  if (!BBL_INS_FIRST(bbl)) {
    return false;
  }
  
  BBL_FOREACH_INS(bbl, ins_) {
    t_arm_ins* ins = T_ARM_INS(ins_);
    if (ARM_INS_OPCODE(ins) == ARM_MOV) {
      if (ARM_INS_REGA(ins) == ARM_REG_LR && ARM_INS_REGC(ins) == ARM_REG_PC) {
        VERBOSE(0, ("SKIPPED: @I", ins));
        return false;
      }
    }
  }

  return true;
}

bool ARMRegisterBasedBranchFunctionTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  possibleSplitPoints += BBL_NINS(bbl) - 1; bblsTransformed++; insts_in_bbls += BBL_NINS(bbl);

  t_function* fun = BBL_FUNCTION(bbl);

  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_OBF_BF, "BranchFunction,%x,%s", BBL_CADDRESS(bbl), FUNCTION_NAME(fun)) {
    AddTransformedBblToLog("BranchFunction", bbl);
    LogFunctionTransformation("before", fun);
  }

  t_bbl* split_off = RandomSplitBasicBlockWithJump(bbl, rng);
  transformJumpToCall(bbl, rng);

  LOG_MORE(L_OBF_BF) { LogFunctionTransformation("after", fun); }
  STOP_LOGGING_TRANSFORMATION(L_OBF_BF);

  return true;
}
  
void ARMRegisterBasedBranchFunctionTransformation::transformJumpToCall(t_bbl* bbl, t_randomnumbergenerator* rng) {
  t_cfg* cfg = BBL_CFG(bbl);
  t_bool isThumb = ArmBblIsThumb(bbl);
  
  t_bbl* successor = NULL;
  t_cfg_edge* e;
  t_cfg_edge* successor_edge = NULL;
  BBL_FOREACH_SUCC_EDGE(bbl, e) {
    ASSERT(!successor, ("BBL expected only a single successor here! @eiB", bbl));
    successor = CFG_EDGE_TAIL(e);
    successor_edge = e;
  }

  //VERBOSE(0, ("BBL live @X", &arm_description, InsRegsLiveAfter(BBL_INS_LAST(bbl))));

  MaybeSaveRegister save_lr(ARM_REG_LR, false /* save */);

  if (RegsetIn(InsRegsLiveBefore(BBL_INS_LAST(bbl)), ARM_REG_LR)) {
    save_lr.save = true;
    VERBOSE(1, ("Saving LR"));
  } else {
    VERBOSE(1, ("Not saving LR"));
  }

  t_regset force_live = RegsetNew();
  MaybeSaveRegister random_reg = GetArchitectureInfo(bbl)->getRandomRegister(BBL_INS_LAST(bbl), force_live, rng);

  vector<MaybeSaveRegister> spilled_registers = { save_lr, random_reg };

  t_bbl* bf = getStackBranchFunction(BBL_CFG(bbl), isThumb, random_reg.reg);

  t_bbl* split_off = BblSplitBlock(bbl, BBL_INS_LAST(bbl), TRUE /* before */);

  InsKill(BBL_INS_LAST(split_off)); // Get rid of the jump. It was useful before for the liveness
  CfgEdgeKill(successor_edge);

  /* 
  stmfd	sp!, {r0,lr} @ also not stealthy...
  mov		r0, #4 @ .BF_FT - .BF_FAKE_FT, this is the 'argument'
  fallthrough to:
  bl		branchfun_r0 <- depends on the register!
  The last BL is in a separate BBL, so that we can put a relocation to the start of the BBL + it's size, which will end up at
  the LR for the BL. Furthermore, the relocation+edge to an untransformable function should mean that it doesn't get transformed (duplicated/split), if it does, that is a bug...
  This has the advantage that we can have an IPJUMP from the BL to the branch function, and need no explicit fallthrough edge. So Diablo can just layout a random
  chain after the BL.
  */
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(spilled_registers, bbl, true /* push */);

  AddRegisterToLiveOut(bbl, ARM_REG_LR);
  AddRegisterToLiveOut(bbl, random_reg.reg);
  AddRegisterToLiveOut(split_off, random_reg.reg);

  t_arm_ins* ins;
  ArmMakeInsForBbl(Mov,  Append, ins, bbl, isThumb, random_reg.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */

  /* The relocation can be the same for ARM and Thumb2 mode, since BL will automatically set the last bit of LR to 1 if we
   * are in Thumb mode. Thus, the end result of LR+ComputedConstant will automatically also have its 1 bit set correctly
   * in both ARM and Thumb mode. */
  t_reloc* reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset: next instruction*/
      T_RELOCATABLE(T_RELOCATABLE(successor)), /* to */
      AddressNew32(0), /* to-offset: begin globale variable  */
      FALSE, /* hell */
      NULL, /* edge*/
      NULL, /* corresp */
      T_RELOCATABLE(split_off), /* sec */
	  "R00R01Z01+-" "\\" WRITE_32);

  ArmInsMakeAddressProducer(ins, 0 /* immediate */, reloc);

  ArmMakeInsForBbl(CondBranchAndLink, Append, ins, split_off, isThumb, ARM_CONDITION_AL);

  /* For now, model the call/return to/from the branch function as IPJUMPs (with compensating edges), this ensures that liveness is propagated correctly. */
  e = CfgEdgeCreate(BBL_CFG(bbl), split_off, bf, ET_IPJUMP);
  if (FunctionGetExitBlock(BBL_FUNCTION(split_off)))
    CfgEdgeCreateCompensating(BBL_CFG(bbl), e);
  e = CfgEdgeCreate(BBL_CFG(bbl), bf, successor, ET_IPJUMP);
  if (FunctionGetExitBlock(BBL_FUNCTION(successor)))
    CfgEdgeCreateCompensating(BBL_CFG(bbl), e);

  VERBOSE(1, ("Transformed BF: @eiB", bbl));

  /* Prepend fallthrough (i.e. split off here!) with restoring instruction ldmfd	sp!, {r0,lr} */
  GetArchitectureInfo(bbl)->saveOrRestoreRegisters(spilled_registers, successor, false /* push */);

  VERBOSE(1, ("Transformed BF: @eiB", successor));
}



ARMSplitCallWithBranchFunctionsTransformation::ARMSplitCallWithBranchFunctionsTransformation() {
}







/* Call functions / Return functions */
static t_function* branch_fun_for_with_ret = NULL;
static t_section* global_jump_var = NULL;

static t_section* GetGlobalJumpVar(t_cfg* cfg) {
  if (global_jump_var)
    return global_jump_var;

  global_jump_var = ObjectNewSubsection(CFG_OBJECT(cfg),sizeof(t_uint32),BSS_SECTION);
  return global_jump_var;
}

/* The call functions, inspired by our original x86 call functions, store (a modification of: + 4)
 * the target function in a global variable. We do so here as well.
 *
 * Thus:
 * bl(x) fun
 * => (in mode M)
 * push {rx, ry}
 * ADR rx, global_var
 * CONST ry, offset_to_fun_stub - 4
 * STR ry, [rx]
 * pop {rx, ry}
 * bl callfun_M
 *
 * callfun_M:
 * push {rx}
 * adr rx, global_var
 * ldr rx, [rx]
 * sub rx, rx, 4
 * bx rx
 *
 * fun_stub:
 * pop {rx}
 * b fun
 *
 * TODO optimizations:
 * we can avoid push {rx} and pop {rx} in the callfun/fun_stub pair in some places, depending on liveness.
 */
static map<pair<t_reg, bool>, t_function*> reg_to_callfun;
t_function* ARMCallFunctionTransformation::getRetFunForReg(t_cfg* cfg, t_bool isThumb, t_reg reg) {
  auto idx = make_pair(reg, isThumb);

  if (reg_to_callfun.find(idx) != reg_to_callfun.end())
    return reg_to_callfun[idx];

  t_arm_ins * ins;
  t_function * fun;
  t_bbl * bbl=BblNew(cfg);
  t_bbl * retBlock = BblNew(cfg);
  t_reloc* global_reloc;

  fun=FunctionMake(bbl, "BranchFunctionWithReturn", FT_NORMAL);

  /* Insert Bbl in Branch Function:
    * push {rx}
    * adr rx, global_var
    * ldr rx, [rx]
    * sub rx, rx, 4
    * bx rx
   **/
  MaybeSaveRegister reg_1(reg, true /* always save, for now */);
  ARMArchitectureInfo info(isThumb);
  info.saveOrRestoreRegisters({reg_1}, bbl, true /* push */);

  /* ADR */
  ArmMakeInsForBbl(Mov,  Append, ins, bbl, isThumb, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S); /* TODO: I have to find out why this is suddenly necessary for Thumb2 */
  global_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE(  GetGlobalJumpVar(cfg)  ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      FALSE,
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0/* immediate */, global_reloc);

  /* LDR */
  ArmMakeInsForBbl(Ldr, Append, ins, bbl, isThumb, reg, reg, ARM_REG_NONE, 0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  /* SUB */
  ArmMakeInsForBbl(Sub, Append, ins, bbl, isThumb, reg, reg, ARM_REG_NONE, 4, ARM_CONDITION_AL);

  /* BX */
  ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, isThumb, reg);

  BblInsertInFunction(retBlock,fun);

  CfgEdgeNew(cfg, bbl, CFG_HELL_NODE(cfg), ET_IPJUMP);
  CfgEdgeNew(cfg, retBlock, CFG_HELL_NODE(cfg), ET_RETURN);

  reg_to_callfun[idx] = fun;
  return fun;
}

t_cfg_edge* ARMCallFunctionTransformation::getCalleeEdge(t_bbl* bbl) {
  t_cfg_edge* edge;
  t_cfg_edge* found = NULL;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
        if (CFG_EDGE_CAT(edge) == ET_CALL) {
          if (found) {
                FATAL(("Double call found in @eiB",bbl));
          }
          found = edge;
        } else {
          // This can happen with branch functions (disassembly thwarting): the call gets a fallthrough to the garbage. Just don't transform those (yet? TODO)
          return NULL;
        }
  }
  ASSERT(found,("No outgoing call edge in @eiB",bbl));
  ASSERT(BBL_FUNCTION(CFG_EDGE_TAIL(found)),("bbl not in function @eiB",CFG_EDGE_TAIL(found)));
  return found;
}

t_bbl* ARMCallFunctionTransformation::jumpAndPopStubFor(t_cfg* cfg, t_bool isThumb, t_reg reg, t_bbl* target) {
  t_bbl* bbl = BblNew(cfg);
  t_bbl* retBlock = BblNew(cfg);
  t_arm_ins* ins;
  t_function* fun;

  fun = FunctionMake(bbl, "ReturnAddressStub", FT_NORMAL);

  MaybeSaveRegister reg_1(reg, true /* always save, for now */);
  GetArchitectureInfo(target)->saveOrRestoreRegisters({reg_1}, bbl, false /* push */);

  ArmMakeInsForBbl(UncondBranch, Append, ins, bbl, isThumb);

  CfgEdgeNew(cfg, bbl, target, ET_IPJUMP);

  /* TODO retblock */
  return bbl;
}


void ARMCallFunctionTransformation::transformCallIntoRetFunction(t_bbl * bbl, t_randomnumbergenerator* rng)/* {{{ */
{
  t_cfg_edge* callee_edge;
  t_function* callee;

  t_arm_ins* ins;
  t_reloc* callee_reloc;
  t_cfg* cfg = BBL_CFG(bbl);

  t_bbl* exit_bbl;
  t_bbl* orig_fallthrough;

  t_reloc *immrel;

  callee_edge = getCalleeEdge(bbl);
  if (!callee_edge)
        return;

  t_bbl* callee_bbl = CFG_EDGE_TAIL(callee_edge);
  callee = BBL_FUNCTION(callee_bbl);

  /* call to exit, etc */
  if (!CFG_EDGE_CORR(callee_edge))
        return;

  /* Transform call 'callee' to:
   * push {rx, ry}
   * ADR rx, global_var
   * CONST ry, offset_to_fun_stub - 4
   * ADD ry, pc, ry
   * STR ry, [rx]
   * pop {rx, ry}
   * bl callfun_M */

  /* Update return edge of the function to this new basic block */
  exit_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(callee_edge));
  orig_fallthrough = CFG_EDGE_TAIL(CFG_EDGE_CORR(callee_edge));

  CfgEdgeKill(CFG_EDGE_CORR(callee_edge));
  CfgEdgeKill(callee_edge);
  callee_edge = nullptr;

  t_bool isThumb = ArmBblIsThumb(bbl);
  t_bool isTargetThumb = ArmBblIsThumb(callee_bbl);

  t_ins* livepoint = BBL_INS_LAST(bbl);

  t_reg stub_reg = ARM_REG_R1;  /* TODO reg randomization */
  VERBOSE(0, ("UH1....... %i %i @eiB", ArmBblIsThumb(callee_bbl), isTargetThumb, callee_bbl));
  t_bbl* jumpstub_bbl = jumpAndPopStubFor(cfg, isTargetThumb, stub_reg, callee_bbl);

  t_regset force_live = RegsetNew();
  MaybeSaveRegister reg_x = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);
  RegsetSetAddReg(force_live, reg_x.reg);
  MaybeSaveRegister reg_y = GetArchitectureInfo(bbl)->getRandomRegister(livepoint, force_live, rng);

  GetArchitectureInfo(bbl)->saveOrRestoreRegisters({reg_x, reg_y}, bbl, true /* push */);

  ArmMakeInsForBbl(Mov,  Append, ins, bbl, isThumb, reg_x.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S); /* TODO: I have to find out why this is suddenly necessary for Thumb2 */
  t_reloc* global_reloc = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(0), /* addend */
      T_RELOCATABLE(ins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE(  GetGlobalJumpVar(cfg)  ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      FALSE,
      NULL, /* edge*/
      NULL, /* corresp */
      NULL, /* sec */
      "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0/* immediate */, global_reloc);

  /* voeg relocatie toe voor constante die moet worden geschreven door mov */
  ArmMakeInsForBbl(Mov,  Append, ins, bbl, isThumb, reg_y.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_arm_ins* constins = ins; /* The CONST above needs to refer to the PC of the ADD */

  ArmMakeInsForBbl(Add, Append, ins, bbl, isThumb, reg_y.reg, reg_y.reg, ARM_REG_PC, 0, ARM_CONDITION_AL);

  int addend = 4; /* We'll substract this 4 in the RetFun */
  if (isTargetThumb)
    addend |= 1; /* But this mode switching '1' will remain */
  addend -= isThumb ? 4 : 8; /* Compensate for PC offset in the current mode */
  
  immrel = RelocTableAddRelocToRelocatable
     (OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
      AddressNew32(addend), /* addend */ // TODO with sec now pointing to ins + ?4:8
      T_RELOCATABLE(constins), /* from */
      AddressNew32(0),  /* from-offset */
      T_RELOCATABLE( jumpstub_bbl ), /* to */
      AddressNew32(0), /* to-offset: begin callee */
      FALSE, /* hell: functie is nu via hell reachable */
      NULL, /* edge*/
      NULL, /* corresp */
      T_RELOCATABLE(ins), /* sec */
      "R00A00+R01-" "\\" WRITE_32);

  ArmInsMakeAddressProducer(constins, 0/* immediate */, immrel);

  /* STR */
  ArmMakeInsForBbl(Str, Append, ins, bbl, isThumb, reg_y.reg, reg_x.reg, ARM_REG_NONE, 0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  ((ARMArchitectureInfo*)GetArchitectureInfo(bbl))->popAppendRegisters({reg_x, reg_y}, bbl);

  /* BLX */
  ArmMakeInsForBbl(CondBranchAndLink, Append, ins, bbl, isThumb, ARM_CONDITION_AL);
  
  InsKill(livepoint);

  CfgEdgeCreateCall(cfg, bbl, FUNCTION_BBL_FIRST(getRetFunForReg(cfg, isThumb, stub_reg)), orig_fallthrough, exit_bbl);
}

bool ARMCallFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!CallFunctionTransformation::canTransform(bbl))
    return false;

  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (BBL_IS_HELL(CFG_EDGE_TAIL(edge))) {
      return false;
    }
  }

  t_arm_ins* ins = T_ARM_INS(BBL_INS_LAST(bbl));

  if (!ins)
    return false;

  if (ARM_INS_OPCODE(ins)==ARM_BL)
    return true;

  if (ARM_INS_OPCODE(ins)!=ARM_BLX)
    return false;

  /* Don't allow BLX reg */
  if (ARM_INS_REGB(ins) != ARM_REG_NONE)
    return false;
  return true;
}

bool ARMCallFunctionTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  transformCallIntoRetFunction(bbl, rng);
  callsTransformed++;

  return true;
}
