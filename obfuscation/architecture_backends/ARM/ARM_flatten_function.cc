/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "ARM_obfuscations.h"
#include "ARM_flatten_function.h"
using namespace std;

/* TODO: randomize usage of registers.
   TODO: if the branch ends in ARM_BX or ARM_BLX, redirect control flow differently, I *suppose* this shouldn't be too hard, probably this should 'just' involve
   a different stub and a different switch block for Thumb/non-Thumb code, but it's a bit of extra work */

ARMFlattenFunctionTransformation::ARMregister_info::ARMregister_info(const MaybeSaveRegister& target_index, const MaybeSaveRegister& index_computation_helper)
  : target_index(target_index), index_computation_helper(index_computation_helper),
    saves(vector<MaybeSaveRegister> { target_index, index_computation_helper })
{
}

void ARMFlattenFunctionTransformation::initRegisterInfo(function_info* info, t_randomnumbergenerator* rng) {
  MaybeSaveRegister r0(ARM_REG_R0, true);
  MaybeSaveRegister r1(ARM_REG_R1, true);

  /* TODO Well, they could be really random here... verify */
  if (info->transformSuccessorsSet.size() == 0 && info->transformPredecessorsSet.size() == 0) {
    info->reg_info = new ARMregister_info(r1, r0);
    return;
  }

  t_regset live_anywhere = RegsetNew();

  /* These randomized registers need to be dead at the beginning of each from_bbl, and at the end of each to_bbl (or save them). */
  for (auto bbl: info->transformPredecessorsSet) {
    live_anywhere = RegsetUnion(live_anywhere, BblRegsLiveBefore(bbl));
  }
  for (auto bbl: info->transformSuccessorsSet) {
    live_anywhere = RegsetUnion(live_anywhere, BblRegsLiveAfter(bbl));
  }

  t_regset not_in = RegsetNew();
  r0 = GetArchitectureInfo(0)->getRandomRegisterCustomLiveness(live_anywhere, not_in, rng);  // TODO: 0 argument for getArchitectureInfo
  RegsetSetAddReg(not_in, r0.reg);
  r1 = GetArchitectureInfo(0)->getRandomRegisterCustomLiveness(live_anywhere, not_in, rng);  // TODO: 0 argument for getArchitectureInfo

  VERBOSE(1, ("Custom flattening R0: R%i (spilled?: %i)", r0.reg, (int)r0.save));
  VERBOSE(1, ("Custom flattening R1: R%i (spilled?: %i)", r1.reg, (int)r1.save));

  /* TODO: maybe we should have a 50-50 pct chance to swap, so both register *uses* have equal chance of being spilled */

  info->reg_info = new ARMregister_info(r1, r0);
}

bool ARMFlattenFunctionTransformation::canTransform(const t_function* fun) const {
  if (!FlattenFunctionTransformation::canTransform(fun))
    return false;

  t_bbl * bbl;
  t_arm_ins* last_ins = T_ARM_INS(BBL_INS_LAST(FUNCTION_BBL_LAST(fun)));

  /* No flattening of functions with a call as last instruction. Some library-functions will not be flattened... */
  if(FUNCTION_BBL_LAST(fun) && BBL_INS_LAST(FUNCTION_BBL_LAST(fun))
    && last_ins && (ARM_INS_OPCODE(last_ins)==ARM_BL || ARM_INS_OPCODE(last_ins)==ARM_BLX)) {
    return false;
  }

  return true;
}

bool ARMFlattenFunctionTransformation::canRedirectSuccessor(t_bbl* bbl) {
 if (!FlattenFunctionTransformation::canRedirectSuccessor(bbl)) {
   return false;
 }

 if (!BBL_INS_LAST(bbl))
   return false;

 t_arm_opcode opc = ARM_INS_OPCODE(T_ARM_INS(BBL_INS_LAST(bbl)));

 int nr_edges = 0;
 t_cfg_edge* edge;

 /* There is a nasty situation on ARM with switch blocks: (Happens, for example, in _wordcopy_fwd_aligned at O2 static gcc Thumb2 for libquantum)
    CMP r3, #0x6
    LDRLS r15,[r15,r3,LSL #2]
    B fallthrough_case
    ; Switch table of the LDRLS comes here
  * If we transform the B fallthrough_case BBL in any way, the switch table can't be layouted correctly any more, and the switch statement reads from instructions rather than data!
  * TODO: 1. this ought to be added to all transformations that might add code to a BBL... 2. This ought to fatal in layouting I'd say?
  */

 BBL_FOREACH_PRED_EDGE(bbl,edge) {
   t_cfg_edge* edge2;
   t_bbl* head = CFG_EDGE_HEAD(edge);
   BBL_FOREACH_SUCC_EDGE(head, edge2) {
     if (CFG_EDGE_CAT(edge2) == ET_SWITCH || CFG_EDGE_CAT(edge2) == ET_IPSWITCH) {
       return false;
     }
   }
 }

 switch (opc) {
  case ARM_BL:
  case ARM_BLX:
    return false;
  case ARM_B:
    return true;
  case ARM_BX:
    if (ARM_INS_REGB(T_ARM_INS(BBL_INS_LAST(bbl))) == ARM_REG_PC)
      return false;
    return true; // TODO: perhaps we should just have the opposite check of the default case here, with ET_JUMP
  case TH_BX_R15: /* BX R15 */
    return false;
  case ARM_T2CBNZ:
  case ARM_T2CBZ:
    return false;
  case ARM_LDR:
  case ARM_LDM:
    /* If this is a pop of the PC, we cannot flatten that. */
    if (RegsetIn(ARM_INS_REGS_DEF(T_ARM_INS(BBL_INS_LAST(bbl))), ARM_REG_PC))
      return false;
    /* fallthrough to default case: check that all edges are of the correct kind (FT) */
  default:
    /* Only a single fallthrough allowed: */
    nr_edges = 0;
    /* TODO: also ensure the last instruction does not modify PC... */
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      nr_edges++;

      ASSERT(CFG_EDGE_CAT(edge)==ET_FALLTHROUGH, ("Unknown last instruction: @I in @eiB", BBL_INS_LAST(bbl), bbl));
     }
    return nr_edges == 1;
  }

 return false;
}

/*
 * Initially, I tried
 * r1 <- address of jump table
 * ldr pc, r1, r0, lsl 2 -> pc = jumptable[r0*4] # contents of jump table are absolute addresses of target instructions/bbls
 * However, Diablo does not seem to like the lack of relocation on the ldr. Hence, currently, it works as follows:
 * r0 <- address of jump table
 * ldr r1, r0, r1, lsl 2 -> r1 = jumptable[r0*4]
 * add pc, pc, r1 # contents of jump table are pc-relative!
 * 
 * Ok, now for Thumb2, since we cannot just add to them to PC to switch from ARM to Thumb.:
 * index_computation_helper <- address of jump table
 * ldr target_index, target_index, index_computation_helper, lsl 2 -> target_index = ABSOLUTE ADDRESS (|1 for Thumb2).
 * bx target_index
 * 
 * TODO: now the tables contain the unencoded target addresses, whereas before we had PC-relative addresses. I dare not use PC here right now, maybe later (although it should be fine)
 */
t_bbl* ARMFlattenFunctionTransformation::createSwitchBlock(t_bbl* switch_bbl, t_section* var_section, register_info* reg_info_) {
  t_arm_ins* ins;
  t_cfg* cfg = BBL_CFG(switch_bbl);
  t_reloc* rel;
  t_bbl* head = switch_bbl;
  ARMregister_info* reg_info = dynamic_cast<ARMregister_info*>(reg_info_);

  /* If we flattened the first basic block, we add a fallthrough from that block to our switch block; hence they have to be in the same mode.
     However, we could just as well do a BX to switch to ARM mode, for example... For now, just let them be the same mode as the switch_bbl.
     What CAN happen is that switch_bbl is just a newly inserted BBL in the function, disconnected from anything. In that case, we can use the
     default, conservative, FALSE return value of ArmBblIsThumb.
     TODO: perhaps as a post-pass, verify at least that we don't have BBLs of mixed Thumb and ARM, and no fallthroughs, although that could
     interact badly with the possible new target selector that switches modes on purpose after opaque predicates... */
  t_tristate isThumbTristate = ArmBblIsPerhapsThumb(switch_bbl);
  t_bool isThumb;
  switch(isThumbTristate) {
    case NO:
    case PERHAPS:
      isThumb = FALSE;
      break;
    case YES:
      isThumb = TRUE;
      break;
    default:
      ASSERT(FALSE, ("Unknown tristate value %i", (int) isThumbTristate));
  }

  VERBOSE(1, ("Is Thumb? %i, @eiB", (int)isThumb, switch_bbl));

  ArmMakeInsForBbl(Mov,  Append, ins, head, isThumb, reg_info->index_computation_helper.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */
  ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S); /* TODO: I have to find out why this is suddenly necessary for Thumb2 */

  rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                        0x0, /* addend */
                                        T_RELOCATABLE(ins), /* from */
                                        0x0, /* from-offset */
                                        T_RELOCATABLE(var_section), /* to */
                                        0x0, /* to-offset */
                                        FALSE, /* hell */
                                        NULL, /* edge */
                                        NULL, /* corresp */
                                        NULL, /* sec */
                                        "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  /* TODO: actually, we could use another register here randomly now that we automate the choice anyway */
  ArmMakeInsForBbl(Ldr, Append, ins, head, isThumb,
                                           reg_info->target_index.reg,
                                           reg_info->index_computation_helper.reg,
                                           reg_info->target_index.reg, 0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);
  /* the register is shifted left over two positions over two positions */
  ARM_INS_SET_SHIFTTYPE(ins, ARM_SHIFT_TYPE_LSL_IMM);
  ARM_INS_SET_SHIFTLENGTH(ins, 2);

  /* target = PC + target, to make it PIC. WARNING! This means that the T_RELOCATABLE in the relocation should point to THIS (i.e. the second-to-last) instruction */
  ArmMakeInsForBbl(Add, Append, ins, head, isThumb, reg_info->target_index.reg, reg_info->target_index.reg, ARM_REG_PC, 0, ARM_CONDITION_AL);

  ArmMakeInsForBbl(UncondBranchExchange, Append, ins, head, isThumb, reg_info->target_index.reg);

  VERBOSE(1, ("Switch block created: @eiB", head));

  return head;
}

void ARMFlattenFunctionTransformation::restoreRegisters(t_bbl* bbl, t_bbl* successor, register_info* reg_info_) {
  t_arm_ins* ins;
  ARMregister_info* reg_info = dynamic_cast<ARMregister_info*>(reg_info_);

  /* Currently, the successor is disconnected from the function's control flow, and thus ArmBblIsThumb would return FALSE ... */
  t_bool isThumb = ArmBblIsThumb(successor);

  GetArchitectureInfo(successor)->saveOrRestoreRegisters(reg_info->saves, bbl, false); /* Same comment as above, but for the getArchitectureInfo's ArmBblIsThumb call */
  ArmMakeInsForBbl(UncondBranch, Append, ins, bbl, isThumb);
}


/* Edge handling is done by caller */
shared_ptr<FlattenFunctionTransformation::Successor> ARMFlattenFunctionTransformation::redirectSuccessorsCode(t_bbl* bbl, function_info* fun_info) {
  t_arm_ins* ins;
  t_arm_ins* last_ins;
  t_cfg_edge* edge;
  ARMregister_info* reg_info = dynamic_cast<ARMregister_info*>(fun_info->reg_info);

  t_bool isThumb = ArmBblIsThumb(bbl);

  auto successor = make_shared<ARMSuccessor>(false, BBL_CFG(bbl), bbl, nullptr);

  /* TODO! In some cases, we presumably could remove these stores / restores later on, using some more intelligent analyses, i.e., chose registers proven to be dead at this point :-) */

  /* Save registers.
   * If the last instruction is a return: do nothing!
   * If the last instruction is another control transfer: insert register saving before control transfer, and make the control transfer unconditional, potentially inserting a cond
   *  move to select between multiple successors
   * Otherwise: this bbl only had a fallthrough: Append register saving, add new unconditional control transfer. TODO not for function call edges! */

  ASSERT(bbl && BBL_INS_LAST(bbl), ("redirectToSingleSuccessor: invalid arguments: @eiB", bbl));

  last_ins = T_ARM_INS(BBL_INS_LAST(bbl));

  /* If this is a return block, don't modify it! (Actually, maybe we should just not add these blocks in the caller of this function... TODO) */
  ASSERT(BBL_FUNCTION(bbl), ("This BBL is not in a function! @eiB"));
  t_bbl* exit_bbl = FunctionGetExitBlock (BBL_FUNCTION(bbl));
  if (exit_bbl) { /* Some functions don't have an exit block, such as __libc_fatal */
    BBL_FOREACH_PRED_EDGE(exit_bbl, edge) {
      if (CFG_EDGE_HEAD(edge) == bbl)
        return successor;
    }
  }

  successor->code_added = true;

  if (ARM_INS_OPCODE(last_ins)==ARM_B
      || ARM_INS_OPCODE(last_ins)==ARM_BX) {
    /* Store registers before the jump instruction */
    /* HACK for now: we want to insert the push before the existing ARM_B, but after the other code in the BBL; currently our
     * generic saveOrRestoreRegisters has an extra parameter for this... */
    GetArchitectureInfo(bbl)->saveOrRestoreRegisters(reg_info->saves, bbl, true, true /* TODO: hack */);
  } else {
    /* Should only have a fallthrough path */
    /* TODO write a check here */

    /* Yep, so append the register-saving code to this BBL */
    GetArchitectureInfo(bbl)->saveOrRestoreRegisters(reg_info->saves, bbl, true);
  }

  if (ARM_INS_OPCODE(last_ins) == ARM_B
        || ARM_INS_OPCODE(last_ins) == ARM_BX) {
    /* Two cases: conditional and unconditional */
    if (ARM_INS_CONDITION(last_ins) == ARM_CONDITION_AL) {
      /* Simple case: a single successor */
      BBL_FOREACH_SUCC_EDGE(bbl,edge) {
        int target;
        ASSERT(CFG_EDGE_CAT(edge)==ET_JUMP, ("Excpected only a JUMP here @eiB", bbl));

        auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
        ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
        target = id->second;

        /* TODO make cleaner: at this point, the constant might not be Thumb2-encodable, while the ArmMakeInsForIns tries to enforce this... */
        ArmMakeInsForIns(Mov, Before, ins, last_ins, isThumb, reg_info->target_index.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
        ARM_INS_SET_IMMEDIATE(ins, target);
        ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
        ArmMakeConstantProducer(ins, target);
      }
      /* Go to the dispatcher BBL: Load its address */
      ArmMakeInsForBbl(Mov, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S); /* TODO: I have to find out why this is suddenly necessary for Thumb2 */
      successor->ins = ins; /* We will fill out the relocation/address producer later, when we have the switch block's BBL (TODO, make cleaner design here, perhaps) */
    
      /* TODO: maybe put this instruction in successor */
      ArmMakeInsForBbl(Add, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, reg_info->index_computation_helper.reg, ARM_REG_PC, 0, ARM_CONDITION_AL);
    
      /* Unconditionalize last ins, and make a BX to ourselves here */
      InsKill(T_INS(last_ins));
      ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg);
    } else {
      /* Fallthrough goes to R1, taken jump goes to R0 */
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      t_reg reg;
      int   target;

      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH) {
        reg = reg_info->target_index.reg;
      } else if (CFG_EDGE_CAT(edge)==ET_JUMP) {
        reg = reg_info->index_computation_helper.reg; // TODO: this could be another random free register!
      } else {
        ASSERT(0, ("Unexpected edge: @eiB", bbl));
      }
      
      auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
      ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
      target = id->second;

      ArmMakeInsForIns(Mov, Before, ins, last_ins, isThumb, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
      ARM_INS_SET_IMMEDIATE(ins, target);
      ArmMakeConstantProducer(ins, target);
    }
    
    ArmMakeInsForIns(Mov, Before, ins, last_ins, isThumb, reg_info->target_index.reg, reg_info->index_computation_helper.reg, 0, ARM_INS_CONDITION(last_ins));
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
    
    /* Go to the dispatcher BBL: Load its address */
    ArmMakeInsForBbl(Mov, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
    ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);  /* TODO: I have to find out why this is suddenly necessary for Thumb2 */
    successor->ins = ins; /* We will fill out the relocation/address producer later, when we have the switch block's BBL (TODO, make cleaner design here, perhaps) */
    
    /* TODO: maybe put this instruction in successor */
    ArmMakeInsForBbl(Add, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, reg_info->index_computation_helper.reg, ARM_REG_PC, 0, ARM_CONDITION_AL);
    
    /* Unconditionalize last ins, and make a BX to ourselves here */
    InsKill(T_INS(last_ins));
    ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg);
    }
  } else {
    /* Fallthrough */
    int verify = 0;
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      int target;
      //FunctionDrawGraph(BBL_FUNCTION(bbl), "fatal.dot");
      ASSERT(CFG_EDGE_CAT(edge)==ET_FALLTHROUGH, ("Excpected only a fallthrough edge here @eiB", bbl));

      auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
      ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
      target = id->second;

      ArmMakeInsForBbl(Mov, Append, ins, bbl, isThumb, reg_info->target_index.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
      ARM_INS_SET_IMMEDIATE(ins, target);
      ArmMakeConstantProducer(ins, target);
      
      /* Go to the dispatcher BBL: Load its address */
      ArmMakeInsForBbl(Mov, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
      ARM_INS_SET_FLAGS(ins, ARM_INS_FLAGS(ins) & ~FL_S);
      successor->ins = ins; /* We will fill out the relocation/address producer later, when we have the switch block's BBL (TODO, make cleaner design here, perhaps) */
      
      /* TODO: maybe put this instruction in successor */
      ArmMakeInsForBbl(Add, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg, reg_info->index_computation_helper.reg, ARM_REG_PC, 0, ARM_CONDITION_AL);
      
      /* make a BX to ourselves here */
      ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, isThumb, reg_info->index_computation_helper.reg);
      verify++;
    }
    ASSERT(verify > 0, ("Expected at least one fallthrough edge @eiB", bbl));
  }
  
  ASSERT(successor->ins, ("Expected to have a correctly initialized ins for this BBL @eiB", bbl));
  
  return successor;
}

void ARMFlattenFunctionTransformation::redirectToSwitchBlock(shared_ptr<Successor> successor_, t_bbl* switch_bbl) {
  shared_ptr<ARMSuccessor> successor = dynamic_pointer_cast<ARMSuccessor>(successor_);
  ASSERT(successor, ("Expected an ARMSuccessor when redirecting control flow to @eiB here", switch_bbl));

  /* the BX at the end of the flattened BBL should redirect control flow to the switch_bbl. The PC-relative PIC add-computation happens in the instruction BEFORE the BX */
  /* Note that the PC will be 4/8 (depending on Thumb2 modus of the current bbl) 4 or 8 ahead, we need to subtract that again */
  t_const_string relocation = "R00A00|R01-A01-" "\\" WRITE_32;

  t_reloc* rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(successor->cfg)),
                                        ArmBblIsThumb(switch_bbl) ? AddressNew32(1) : AddressNew32(0), /* use addend to set modus of next bbl */
                                        T_RELOCATABLE(successor->ins), /* from */
                                        0x0, /* from-offset */
                                        T_RELOCATABLE(switch_bbl), /* to */
                                        0x0, /* to-offset */
                                        FALSE, /* hell */
                                        NULL, /* edge */
                                        NULL, /* corresp */
                                        T_RELOCATABLE(ARM_INS_INEXT(successor->ins)), /* the PIC PC-relative computation is done in the second-to-last instruction! */                                        
                                        relocation);
  if (ArmBblIsThumb(successor->bbl))
    RelocAddAddend(rel, AddressNew32(4));
  else
    RelocAddAddend(rel, AddressNew32(8));
  /* Make address producer of the instruction that generates the address for the BX */
  ArmInsMakeAddressProducer(successor->ins, 0/* immediate */, rel);
  
  FlattenFunctionTransformation::redirectToSwitchBlock(successor, switch_bbl); // Add the edge
}

t_reloc* ARMFlattenFunctionTransformation::writeRelocationInSwitchTable(t_cfg* cfg, t_section* var_section, t_bbl* switch_bbl, t_bbl* target_bbl, int switch_index) {
  /* target pc in switch table = PC_switch_table + target, to make it PIC */
  //t_const_string relocation = "R00R00M|R01-" "\\" WRITE_32;
  /* Note that the PC will be 4/8 (depending on Thumb2 modus of the current bbl) 4 or 8 ahead, we need to subtract that again */
  t_const_string relocation = "R00A00|R01-A01-" "\\" WRITE_32;

  t_reloc* rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                         ArmBblIsThumb(target_bbl) ? AddressNew32(1) : AddressNew32(0), /* use addend to set modus of next bbl */
                                         T_RELOCATABLE(var_section), /* from */
                                         AddressNew32(4*switch_index),  /* from-offset in section */
                                         T_RELOCATABLE(BBL_INS_FIRST(target_bbl)), /* to */
                                         AddressNew32(0), /* to-offset */
                                         FALSE, /* hell */
                                         NULL, /* edge*/
                                         NULL, /* corresp */
                                         T_RELOCATABLE(INS_IPREV(BBL_INS_LAST(switch_bbl))), /* the PIC PC-relative computation is done in the second-to-last instruction! */
                                         relocation);
  if (ArmBblIsThumb(switch_bbl))
    RelocAddAddend(rel, AddressNew32(4));
  else
    RelocAddAddend(rel, AddressNew32(8));
  
  return rel;
}

ARMFlattenFunctionTransformation::ARMFlattenFunctionTransformation(bool multiple_flatten)
  : FlattenFunctionTransformation(multiple_flatten)
{
}
