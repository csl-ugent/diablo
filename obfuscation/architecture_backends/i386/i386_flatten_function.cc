/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include "i386_obfuscations.h"
#include "i386_architecture_backend.h"
#include "i386_flatten_function.h"
using namespace std;

/* TODO: randomize usage of eax, ebx */

bool I386FlattenFunctionTransformation::canTransform(const t_function* fun) const {
  if (!FlattenFunctionTransformation::canTransform(fun))
    return false;

  t_bbl * bbl;

  /* No flattening of functions with a call as last instruction. Some library-functions will not be flattened...
   * TODO: is this basically to prevent a specific subcase of the problem mentioned in canRedirectPredecessor? */
  if(FUNCTION_BBL_LAST(fun) && BBL_INS_LAST(FUNCTION_BBL_LAST(fun)) && I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(FUNCTION_BBL_LAST(fun))))==I386_CALL)
    return false;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    if(bbl && BBL_INS_LAST(bbl) &&
    ( I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_ECXZ ||
     I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOP ||
     I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOPZ ||
     I386_INS_CONDITION(T_I386_INS(BBL_INS_LAST(bbl))) == I386_CONDITION_LOOPNZ))
      return false;
  }
  return true;
}

/* TODO: this is a small hack around functions with basic block structures like
 * exit(bbl1): ....; call exit(bbl2) (when this call is modeled as a *JUMP* edge!)
 * Currently we just do not transform this call, another solution would be to let the call go through to the switch bbl...,
 * or to change the call into an explicit push return addr + jmp
 */
bool I386FlattenFunctionTransformation::canRedirectSuccessor(t_bbl* bbl) {
  if (!FlattenFunctionTransformation::canRedirectSuccessor(bbl)) {
    return false;
  }
 
  if (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl))) != I386_CALL) {
    return true;
  }

  t_cfg_edge * edge;

  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if (CFG_EDGE_CAT(edge)==ET_JUMP) {
      return false;
    }
  }
  
  return true;
}

/* We need to redirect control flow to the head of the switch block; however,
 * the relocation in the switch table needs to refer to it's fallthrough (for PIC). */
static t_bbl* FallThroughBblForSwitchBlock(t_bbl* switch_bbl) {
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(switch_bbl, edge) {
    if (CFG_EDGE_CAT(edge) == ET_CALL) {
      if (!CFG_EDGE_CORR(edge))
        return nullptr;
      return CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
    }
  }

  return nullptr;
}

/* switch_bbl:
 * Used to be jmp *0x<jumptable>(,%eax,4) for statically linked binbinaries.
 * Now that we also require PIC code, we have:
 * 
 * ebx is free by design. eax contains the index in the jumptable
 * TODO for later: these could (should) also be randomized similar to how we do this on ARM
 * 
 * set_ebx_to_return_address: mov ebx, [esp]; ret
 * 
 * call set_ebx_to_return_address
 * ft_block:                                # ebx = T_RELOCATABLE(BBL_INS_FIRST(ft_block))
 * mov eax, offset_to_jumptable[ebx+eax*4]  # ebx+offset_to_jumptable = jumptable; eax*4 = offset in jumptable; afterwards: eax = offset to target_bbl, relative to ft_block, ie, this same instruction
 * lea ebx, eax+ebx                         # we will jmp to (ft_block + target_bbl_offset)
 * jump ebx
 * 
 */
t_bbl* I386FlattenFunctionTransformation::createSwitchBlock(t_bbl* switch_bbl, t_section* var_section, register_info* reg_info_) {
  t_i386_ins* ins;
  t_cfg* cfg = BBL_CFG(switch_bbl);
  t_bbl* head = switch_bbl;

  t_bbl* ft_block = BblNew(cfg);
  BblInsertInFunction(ft_block, BBL_FUNCTION(switch_bbl));

  t_function* eip_stub = GetReturnAddressStubForRegister(cfg, I386_REG_EBX);

  I386MakeInsForBbl(Call, Append, ins, head);
  CfgEdgeCreateCall(cfg, head, FUNCTION_BBL_FIRST(eip_stub), ft_block, FunctionGetExitBlock(eip_stub) /* TODO correct? */);
  
  I386MakeInsForBbl(MovFromMem, Append, ins, ft_block, I386_REG_EAX /* dest */, 0 /* offset, relocated */, I386_REG_EBX /* base */, I386_REG_EAX /* index */, 2 /* scale: 1<<2 = 4 */);
  /* ebx + relocated_offset_to_jumptable = var_section -> relocation = var_section - ft_block */
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                        0,
                                        T_RELOCATABLE(ins), /* from */
                                        0x0, /* from-offset */
                                        T_RELOCATABLE(var_section), /* to */
                                        0x0, /* to-offset */
                                        FALSE, /* hell */
                                        NULL, /* edge */
                                        NULL, /* corresp */
                                        T_RELOCATABLE(ft_block),
                                        "R00R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(ins)) = I386_OPFLAG_ISRELOCATED;

  I386MakeInsForBbl(Lea, Append, ins, ft_block, I386_REG_EBX /* dest */, 0 /* offset */, I386_REG_EAX /* base */, I386_REG_EBX /* index */, 0 /* scale */);
  I386MakeInsForBbl(JumpReg, Append, ins, ft_block, I386_REG_EBX);

  return head;
}

void I386FlattenFunctionTransformation::restoreRegisters(t_bbl* bbl, t_bbl* successor, register_info* reg_info_) {
  t_i386_ins* ins;
  I386MakeInsForBbl(Pop,Append,ins,bbl,I386_REG_EBX);
  I386MakeInsForBbl(Pop,Append,ins,bbl,I386_REG_EAX);
  I386MakeInsForBbl(Jump,Append,ins,bbl);
}


/* Edge handling is done by caller */
/* With PIC, this looks like the following:
 * eax = target
 * ebx = free register
 * 
 * call GetReturnAddressStubEbx
 * ft_block: # ebx now = ft_block
 * lea ebx, [ebx+offset]
 * jmp *ebx
 * 
 * ft_block + offset = switch_bbl
 * Problem: edge handling is done by caller... but this actually calls redirectToSwitchBlock, which we override.
 */
shared_ptr<FlattenFunctionTransformation::Successor> I386FlattenFunctionTransformation::redirectSuccessorsCode(t_bbl* bbl, function_info* fun_info) {
  t_i386_ins* ins;
  t_i386_ins* last_ins;
  t_cfg_edge* edge;
  t_i386_ins* reloc_add_instruction = nullptr;
  t_cfg* cfg = BBL_CFG(bbl);
  register_info* reg_info = fun_info->reg_info;

  /* TODO! In some cases, we presumably could remove these stores / restores later on, using some more intelligent analyses, i.e., chose registers proven to be dead at this point :-) Also, we can play with not storing EBX when all incoming edges come from JUMPs */
  
  /* Save registers.
   * If the last instruction is a return: do nothing!
   * If the last instruction is another control transfer: insert register saving before control transfer, and make the control transfer unconditional, potentially inserting a cond
   *  move to select between multiple successors
   * Otherwise: this bbl only had a fallthrough: Append register saving, add new unconditional control transfer. TODO not for function call edges! */
  
  ASSERT(bbl && BBL_INS_LAST(bbl), ("redirectToSingleSuccessor: invalid arguments: @eiB", bbl));

  last_ins = T_I386_INS(BBL_INS_LAST(bbl));
  if (I386_INS_OPCODE(last_ins)==I386_RET)
    return make_shared<I386Successor>(false, BBL_CFG(bbl), bbl, nullptr, nullptr);
  
  if (I386_INS_OPCODE(last_ins)==I386_Jcc || I386_INS_OPCODE(last_ins) == I386_JMP) {
    /* Store registers before the jump instruction */
    I386MakeInsForIns(Push,Before, ins, last_ins, I386_REG_EAX, 0x0);
    I386MakeInsForIns(Push,Before, ins, last_ins, I386_REG_EBX, 0x0);
  } else {
    /* Should only have a fallthrough path */
    /* TODO write a check here */

    /* Yep, so append the register-saving code to this BBL */
    I386MakeInsForBbl(Push,Append, ins, bbl, I386_REG_EAX, 0x0);
    I386MakeInsForBbl(Push,Append, ins, bbl, I386_REG_EBX, 0x0);
  }

  /* All flows will be made to end the PIC-sequence now, ending in jmp *ebx */
  if (I386_INS_OPCODE(last_ins) == I386_JMP) {
    /* Simple case: a single successor */
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      int target;
      ASSERT(CFG_EDGE_CAT(edge)==ET_JUMP, ("Excpected only a JUMP here @eiB", bbl));

      auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
      ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
      target = id->second;

      I386MakeInsForIns(MovToReg,Before, ins, last_ins, I386_REG_EAX, I386_REG_NONE, target);
    }
    
    I386InsKill(last_ins);
  } else if (I386_INS_OPCODE(last_ins) == I386_Jcc) {
    /* Fallthrough goes to EAX, taken jump goes to EBX */
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      t_reg reg;
      int   target;

      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH) {
        reg = I386_REG_EAX;
      } else if (CFG_EDGE_CAT(edge)==ET_JUMP) {
        reg = I386_REG_EBX;
      } else {
        ASSERT(0, ("Unexpected edge: @eiB", bbl));
      }
      
      auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
      ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
      target = id->second;

      I386MakeInsForIns(MovToReg,Before, ins, last_ins, reg, I386_REG_NONE, target);
    }
    
    I386MakeInsForIns(CondMov,Before,ins, last_ins, I386_REG_EBX, I386_REG_EAX, I386_INS_CONDITION(last_ins));
    I386InsKill(last_ins);
  } else {
    /* Fallthrough or other instruction such as HLT (which is modeled in diablo for non-ring-0 code as an infinite loop with a ET_JUMP edge */
    int count = 0;
    BBL_FOREACH_SUCC_EDGE(bbl,edge) {
      int target;
      ASSERT(count == 0, ("Only one outgoing edge excpected: @eiB", bbl));

      auto id = fun_info->bblToId.find(CFG_EDGE_TAIL(edge));
      ASSERT(id != fun_info->bblToId.end(), ("Expected an id for @eiB", CFG_EDGE_TAIL(edge)));
      target = id->second;

      I386MakeInsForBbl(MovToReg,Append, ins, bbl, I386_REG_EAX, I386_REG_NONE, target);
    }
  }
  
  /* Now append the sequence for PIC code */
  t_bbl* ft_block = BblNew(cfg);
  BblInsertInFunction(ft_block, BBL_FUNCTION(bbl));

  I386MakeInsForBbl(Call, Append, ins, bbl);
  /* The generic code here kills outgoing edges of bbl, so we do the CreateCall in the redirectToSwitchBlock, but this is rather dirty, so, TODO */

  I386MakeInsForBbl(Lea, Append, ins, ft_block, I386_REG_EBX /* dest */, 0 /* offset, relocated */, I386_REG_EBX /* base */, I386_REG_NONE /* index */, 0 /* scale */);
  reloc_add_instruction = ins;
  
  I386MakeInsForBbl(JumpReg, Append, ins, ft_block, I386_REG_EBX);

  return make_shared<I386Successor>(true, BBL_CFG(bbl), bbl, reloc_add_instruction, ft_block);
}

/* See above, we not only need to add an edge, but also add a relocation  */
void I386FlattenFunctionTransformation::redirectToSwitchBlock(shared_ptr<Successor> successor_, t_bbl* switch_bbl) {
  shared_ptr<I386Successor> successor = dynamic_pointer_cast<I386Successor>(successor_);
  ASSERT(successor, ("Expected an I386Successor!"));

  /* The edge starts at the fallthrough BBL, since the original bbl just ends in a call to the eip stub */
  CfgEdgeCreate(successor->cfg, successor->ft_block, switch_bbl, ET_JUMP);
  
  /* We also have to add the call edge from the bbl to the eip stub here, because the outgoing edges of bbl are removed after redirectSuccessorsCode returns */
  t_function* eip_stub = GetReturnAddressStubForRegister(successor->cfg, I386_REG_EBX);
  CfgEdgeCreateCall(successor->cfg, successor->bbl, FUNCTION_BBL_FIRST(eip_stub), successor->ft_block, FunctionGetExitBlock(eip_stub) /* TODO correct? */);
  
  ASSERT(successor->ft_block, ("Did not expect redirectSuccessorsCode without a set ft_block"));
  
  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(successor->cfg)),
                                        0,
                                        T_RELOCATABLE(successor->ins), /* from */
                                        0x0, /* from-offset */
                                        T_RELOCATABLE(switch_bbl), /* to */
                                        0x0, /* to-offset */
                                        FALSE, /* hell */
                                        NULL, /* edge */
                                        NULL, /* corresp */
                                        T_RELOCATABLE(successor->ft_block),
                                        "R00R01-" "\\" WRITE_32);
  I386_OP_FLAGS(I386_INS_SOURCE1(successor->ins)) = I386_OPFLAG_ISRELOCATED;
}

t_reloc* I386FlattenFunctionTransformation::writeRelocationInSwitchTable(t_cfg* cfg, t_section* var_section, t_bbl* switch_bbl, t_bbl* target_bbl, int switch_index) {
  /* switch_entry + ft_block = target_addr */
  /* ft_block = BBL_INS_FIRST(switch_bbl) */
  /* => entry = target_bbl - BBL_INS_FIRST(switch_bbl) */
  t_bbl* ft_block = FallThroughBblForSwitchBlock(switch_bbl);
  ASSERT(ft_block, ("Expected a call fallthrough block for switch block @eiB", switch_bbl));

  return RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                         0x0,
                                         T_RELOCATABLE(var_section),
                                         4*switch_index,
                                         T_RELOCATABLE(target_bbl),
                                         0x0,
                                         FALSE,
                                         NULL,
                                         NULL,
                                         T_RELOCATABLE(BBL_INS_FIRST(ft_block)),
                                         "R00R01-" "\\" WRITE_32);
}
