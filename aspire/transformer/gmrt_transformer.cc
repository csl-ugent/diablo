/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "gmrt_transformer.h"

using namespace std;

t_const_string GMRTTransformer::transform_label = "$Transformed$";

GMRTTransformer::GMRTTransformer (t_object* obj, initializer_list<t_const_string> objs_needed, t_string lib_needed, t_const_string output_name, t_const_string log_suffix, t_bool srt, t_const_string non, t_const_string trl, t_uint32 edge_flag)
: AbstractTransformer(obj, output_name, log_suffix, srt, non, trl, edge_flag)
{
  t_const_string *needed_libs = NULL;

  /* A colon separated string array has been passed containing the names of all the shared objects the downloader
   * object depends on. We will tokenize this string (replacing all colons with NULL chars) and pass an array of
   * t_const_string's to LinkObjectFileNew.
   */
  if (lib_needed)
  {
    t_uint32 iii = 0;
    needed_libs = (t_const_string*) Malloc(2 * sizeof(t_const_string));

    needed_libs[iii] = strtok (lib_needed, PATH_DELIMITER);
    while (needed_libs[iii])
    {
      iii++;
      needed_libs = (t_const_string*) Realloc(needed_libs, (iii + 1) * sizeof(t_const_string));
      needed_libs[iii] = strtok (NULL, PATH_DELIMITER);
    }
  }

  /* Link in the objects that are still needed */
  t_bool libs_linked = FALSE;
  for (auto on : objs_needed)
  {
    if (on)
    {
     if(!libs_linked)
     {
      LinkObjectFileNew (obj, on, PREFIX_FOR_LINKED_IN_GMRT_OBJECT, FALSE, TRUE, needed_libs);
      libs_linked = TRUE;
     }
     else
      LinkObjectFileNew (obj, on, PREFIX_FOR_LINKED_IN_GMRT_OBJECT, FALSE, TRUE, NULL);
    }
  }

  if (lib_needed)
    Free(needed_libs);

  t_symbol* binary_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "binary");
  t_symbol* gmrt_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "global_mobile_redirection_table");
  gmrt_size_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "GMRT_size");
  init_sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "Init");

  ASSERT(binary_sym && gmrt_sym && gmrt_size_sym && init_sym, ("Didn't find the symbols associated with the GMRT! Are you sure the right object was linked in?"));

  /* Get the GMRT from the symbol */
  gmrt_sec = T_SECTION(SYMBOL_BASE(gmrt_sym));

  /* The size of a GMRT entry has been set as the first element of the GMRT */
  gmrt_entry_size = SectionGetData32 (T_SECTION(SYMBOL_BASE(gmrt_sym)), SYMBOL_OFFSET_FROM_START(gmrt_sym));

  /* Set the binary base symbol to the binary base of this object. The old base
   * is now a section that should be removed. Replace all references to it with
   * references to the new base and kill the old base.
   */
  t_section* old_binary_sec = T_SECTION(SYMBOL_BASE(binary_sym));
  SymbolSetBase(binary_sym, T_RELOCATABLE(binary_base_sec));

  t_reloc_ref* rr = SECTION_REFED_BY(old_binary_sec);
  while(rr)
  {
    t_reloc_ref* tmp_rr = RELOC_REF_NEXT(rr);
    t_reloc* rel = RELOC_REF_RELOC(rr);
    t_uint32 relocatable_index = RelocGetToRelocatableIndex(rel, T_RELOCATABLE(old_binary_sec));
    RelocSetToRelocatable(rel, relocatable_index, T_RELOCATABLE(binary_base_sec));

    rr = tmp_rr;
  }
  SectionKill(old_binary_sec);

  /* Install the initialization routine */
  if (SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), FINAL_PREFIX_FOR_LINKED_IN_GMRT_OBJECT "Init"))
    DiabloBrokerCall ("AddInitializationRoutine", obj, init_sym, false);

  DiabloBrokerCallInstall("RelocIsRelative", "t_reloc *, t_bool *", (void*)GMRTTransformer::RelocIsTransformed, FALSE);
}

t_bool GMRTTransformer::CanTransformFunction (t_function* fun) const
{
  t_const_string log_msg = "Can't transform function %s: %s.\n";
  t_const_string fun_name = FUNCTION_NAME(fun);

  /* Small inner lambda function to do all logging and return FALSE */
  auto log_and_return_false = [=](t_const_string reason){
    VERBOSE(0, (log_msg, fun_name, reason));
    LOG(L_TRANSFORMS, log_msg, fun_name, reason);
    return FALSE;
  };

  if (FUNCTION_IS_HELL(fun))
    return log_and_return_false("it's a hell function");

  if (FUNCTION_CFG(fun) != cfg)
    return log_and_return_false("it has already been transformed");

  /* If the entry BBL is marked, this means it is reachable from the BBL that resolves invocations of transformed code */
  if (BblIsMarked2(FUNCTION_BBL_FIRST(fun)))
    return log_and_return_false("this function plays a part in accessing the transformed code and thus can't be transformed itself");

  if (FUNCTION_NAME(fun))
  {
    t_bbl* bbl;
    FUNCTION_FOREACH_BBL(fun, bbl)
    {
      t_cfg_edge* edge;
      BBL_FOREACH_SUCC_EDGE(bbl, edge)
        if (CfgEdgeTestCategoryOr(edge, ET_IPSWITCH))
          return log_and_return_false("the function has an outgoing IPSWITCH edge, which isn't supported yet");

      BBL_FOREACH_PRED_EDGE(bbl, edge)
        if (CfgEdgeTestCategoryOr(edge, ET_IPSWITCH))
          return log_and_return_false("the function has an incoming IPSWITCH edge, which isn't supported yet");
    }
  }

  return TRUE;
}

void GMRTTransformer::TransformAddressProducer (t_arm_ins* ins)
{
  t_reg dest = ARM_INS_REGA(ins);
  t_reg helper;

  /* When adding new instructions, we want them to be executed on the same condition as the original instuction they replace */
  t_arm_condition_code condition = ARM_INS_CONDITION(ins);

  /* We will generate the code below by appending every new instruction to the last new instruction
   * (or to the original ins, as in the first case).
   */
  t_arm_ins* last_new = ins;
  t_arm_ins* tmp;

  /* We will generate the following code:
   * [OPTIONAL] PUSH {helper}
   * ADR dest, binary_base_addr
   * [OPTIONAL] LDR dest, [dest] (if the relocation is not relative)
   * ADR helper, $offset
   * ADD dest, dest, helper
   * [OPTIONAL] POP {helper}
   */

  /* Find out how many registers there are available, this time we only need 1 */
  t_regset available = RegsetDiff(possible, InsRegsLiveAfter(T_INS(ins)));
  t_uint32 nr_of_dead_regs = RegsetCountRegs(available);

  if(nr_of_dead_regs == 0)
  {
    helper = (dest == ARM_REG_R0)?ARM_REG_R1:ARM_REG_R0;
    ArmMakeInsForIns(Push, After, tmp, last_new, FALSE, (1 << helper), condition, FALSE);
    last_new = tmp;
  }
  else
  {
    REGSET_FOREACH_REG(available, helper)
      break;
  }

  /* The relocation for the address producer, this will produce the address of the data BBL containing the binary base */
  ArmMakeInsForIns(Mov, After, tmp, last_new, FALSE, dest, ARM_REG_NONE, 0, condition); /* Just get us an instruction with a correct regA */
  last_new = tmp;
  t_reloc* tmp_rel = RelocTableAddRelocToRelocatable(address_producer_table,
    AddressNullForObject(obj), /* addend */
    T_RELOCATABLE(tmp), /* from */
    AddressNullForObject(obj), /* from-offset */
    T_RELOCATABLE(binary_base_bbl), /* to */
    AddressNullForObject(obj), /* to-offset */
    FALSE, /* hell */
    NULL, /* edge */
    NULL, /* corresp */
    NULL, /* sec */
    "R00A00+\\l*w\\s0000$");
  ArmInsMakeAddressProducer(tmp, 0/* immediate */, tmp_rel);

  /* We get the original relocation that is to be transformed. Depending on whether or not the relocation
   * is relative, we use either the base of the binary, or the base of the mobile block to calculate the
   * offset from.
   */
  t_relocatable* offset_base;
  t_reloc* orig_rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins));
  if (!RelocIsRelative(orig_rel))
  {
    /* Load the actual binary base */
    ArmMakeInsForIns(Ldr, After, tmp, last_new, FALSE, dest, dest, ARM_REG_NONE,
        0 /* immediate */, condition, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);
    last_new = tmp;
    offset_base = T_RELOCATABLE(binary_base_sec);
  }
  else
    /* Simply use the mobile block as base */
    offset_base = T_RELOCATABLE(binary_base_bbl);

  /* We create the new address producer, duplicating the original relocation */
  ArmMakeInsForIns(Mov, After, tmp, last_new, FALSE, helper, ARM_REG_NONE, 0, condition); /* Just get us an instruction with a correct regA */
  last_new = tmp;
  RELOC_SET_EDGE(orig_rel, NULL);/* Slight hack: if this isn't NULL we'll get a use after free on the dup */
  t_reloc* offset_rel = RelocTableDupReloc(address_producer_table, orig_rel);
  ArmInsMakeAddressProducer(tmp, 0/* immediate */, offset_rel);

  /* We need to adjust the relocation somewhat so that it is offset toward the "base". The hardest part is rewriting the relocation code. */
  RelocSetFrom(offset_rel, T_RELOCATABLE(tmp));
  t_uint32 nreloc = RELOC_N_TO_RELOCATABLES(offset_rel);
  char new_piece[6];
  sprintf(new_piece, "R%02d-\\", nreloc);
  RelocAddRelocatable(offset_rel, offset_base, AddressNullForObject(obj));
  t_string old = RELOC_CODE(offset_rel);
  char* loc = strstr(old, "\\");
  *loc = '\0';
  t_string code = StringConcat3(old, new_piece, (char*)((t_uint64)loc + 1));
  Free(old);
  RELOC_SET_CODE(offset_rel, code);
  t_string label = RELOC_LABEL(offset_rel);
  if(label)
  {
    RELOC_SET_LABEL(offset_rel, StringConcat2(transform_label, label));
    Free(label);
  }
  else RELOC_SET_LABEL(offset_rel, StringDup(transform_label));

  ArmMakeInsForIns(Add, After, tmp, last_new, FALSE, dest, dest, helper, 0, condition);
  last_new = tmp;

  if(nr_of_dead_regs == 0)
    ArmMakeInsForIns(Pop, After, tmp, last_new, FALSE, (1 << helper), condition, FALSE);

  /* Remove the old address producer */
  InsKill(T_INS(ins));
}

void GMRTTransformer::TransformIncomingTransformedEdgeImpl (t_arm_ins* ins, t_reloc* reloc)
{
  t_bbl* bbl = ARM_INS_BBL(ins);

  /* We will have to rewrite the following code:
   * [OPTIONAL] SUB SP, SP, #4 (only when there no available registers)
   * [OPTIONAL] PUSH {adr, offset} (or PUSH {offset})
   * ADR adr, $binary_base_addr
   * LDR adr, [adr]
   * ADR offset, $offset
   * ADD adr, adr, offset
   * [OPTIONAL] STR adr, [SP, 8]
   * [OPTIONAL] MOV LR, PC
   * BLX adr (or BX adr or POP {adr, offset, PC})
   *
   * Into:
   * [OPTIONAL] PUSH {adr, offset} (when there's 0 or 1 dead registers)
   * ADR adr, $binary_base_addr
   * LDR adr, [adr]
   * ADR offset, $gmrt_offset_to_entry
   * LDR adr, [adr, offset]
   * [OPTIONAL] SUB adr, adr, 4
   * BLX adr or BX adr, depending on which type of edge we're dealing with.
   */

   /* We will detect if there weren't enough dead registers for the code inserted previously, and handle accordingly. */
   t_uint32 nr_of_dead_regs = 2;

  /* We start by transforming the address producer so it produces the offset of the GMRT entry to the binary base instead of
   * producing the address of the target BBL.
   */
  RelocSetToRelocatable(reloc, 0, T_RELOCATABLE(gmrt_sec));
  RELOC_TO_RELOCATABLE_OFFSET(reloc)[0] = AddressNewForObject(obj, gmrt_entry_size * transform_index);

  /* Find the last instruction (a BLX or BX) and the register it uses */
  t_reg offset = ARM_INS_REGA(ins);
  t_reg adr;
  t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(bbl));
  t_arm_ins* tmp = ARM_INS_IPREV(last);

  /* If there are no dead registers the we use the POP-trick to emulate a BLX or BX. In the process of rewriting we
   * can change this into a proper BLX or BX.
   */
  if(!((ARM_INS_OPCODE(last) == ARM_BLX) || (ARM_INS_OPCODE(last) == ARM_BX)))
  {
    nr_of_dead_regs = 0;
    adr = ARM_REG_R0;
    InsKill(T_INS(last));

    /* If the previous instruction is a MOV we're dealing with a CALL edge, else an IPJUMP or such alike */
    if(ARM_INS_OPCODE(tmp) == ARM_MOV)
      ArmMakeInsForBbl(CondBranchLinkAndExchange, Append, last, bbl, FALSE, ARM_CONDITION_AL, adr);
    else
      ArmMakeInsForBbl(UncondBranchExchange, Append, last, bbl, FALSE, adr);
  }
  adr = ARM_INS_REGB(last);

  /* Remove all possible instructions that were added right before the last instruction to handle a shortage in dead registers */
  while(ARM_INS_OPCODE(tmp) != ARM_ADD)
  {
    t_arm_ins* prev = ARM_INS_IPREV(tmp);
    InsKill(T_INS(tmp));

    /* If nr_of_dead_regs hasn't been set to 0, but the instruction before the last instruction isn't an ADD, there was one dead register */
    if(nr_of_dead_regs == 2) nr_of_dead_regs = 1;
    tmp = prev;
  }

  /* Delete the add */
  t_arm_ins* prev = ARM_INS_IPREV(tmp);
  InsKill(T_INS(tmp));
  tmp = prev;

  if(nr_of_dead_regs == 0)
    ARM_INS_SET_REGA(ARM_INS_INEXT(ins), adr);

  /* After this load, 'adr' should contain the address of the mobile function (or stub) */
  ArmMakeInsForIns(Ldr, Before, tmp, last, FALSE, adr, adr, offset,
    0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  /* Some extra things that have to happen when there aren't enough dead registers available. */
  if(nr_of_dead_regs <= 1)
  {
    /* If we have to pop the r0 and r1 registers before continuing execution of the mobile code, subtract 4 from the
     * calculated address. This will land us on a pop {r0, r1} situated right before the actual code.
     */
    ArmMakeInsForIns(Sub, Before, tmp, last, FALSE, adr, adr, ARM_REG_NONE, 4, ARM_CONDITION_AL);

    /* Find the address producer that creates the destination address */
    t_arm_ins* first_non_optional = ARM_INS_IPREV(ARM_INS_IPREV(ins));

    /* If there were no dead registers we'll keep the PUSH, but remove the SUB in front of it */
    if(nr_of_dead_regs == 0)
      InsKill(T_INS(ARM_INS_IPREV(ARM_INS_IPREV(first_non_optional))));
    else
    {
      /* If there was one dead register we change the PUSH of one register into a PUSH of two */
      InsKill(T_INS(ARM_INS_IPREV(first_non_optional)));
      ArmMakeInsForIns(Push, Before, tmp, first_non_optional, FALSE, (1 << ARM_REG_R0) | (1 << ARM_REG_R1), ARM_CONDITION_AL, FALSE);
    }
  }
}

void GMRTTransformer::TransformOutgoingEdgeImpl (t_bbl* bbl, t_cfg_edge* edge, t_relocatable* to)
{
  /* Find out how many registers there are available */
  t_regset available = RegsetDiff(possible, BblRegsLiveAfter(bbl));

  /* We will generate the following code:
   * [OPTIONAL] SUB SP, SP, #4 (only when there no available registers)
   * [OPTIONAL] PUSH {adr, offset} (or PUSH {offset})
   * ADR adr, $binary_base_addr
   * LDR adr, [adr]
   * ADR offset, $offset
   * ADD adr, adr, offset
   * [OPTIONAL] STR adr, [SP, 8]
   * [OPTIONAL] MOV LR, PC
   * BLX adr (or BX adr or POP {adr, offset, PC})
   */

  /* The code for calling out of transformed code uses two registers. We can either use two dead registers (if available),
   * use one dead register and a live register that needs to be pushed/popped, or two live registers
   * that need to be pushed/popped.
   */
  t_reg adr, offset;
  t_arm_ins* ins;
  t_uint32 nr_of_dead_regs = RegsetCountRegs(available);
  if(nr_of_dead_regs == 0)
  {
    adr = ARM_REG_R0;
    offset = ARM_REG_R1;

    /* We subtract from the SP register first to reserve a place on the stack */
    ArmMakeInsForBbl(Sub, Append, ins, bbl, FALSE, ARM_REG_R13, ARM_REG_R13, ARM_REG_NONE, adr_size, ARM_CONDITION_AL);

    ArmMakeInsForBbl(Push, Append, ins, bbl, FALSE, (1 << adr) | (1 << offset), ARM_CONDITION_AL, FALSE);
  }
  else if(nr_of_dead_regs == 1)
  {
    REGSET_FOREACH_REG(available, adr)
      break;
    offset = (adr == ARM_REG_R0)?ARM_REG_R1:ARM_REG_R0;
    ArmMakeInsForBbl(Push, Append, ins, bbl, FALSE, (1 << offset), ARM_CONDITION_AL, FALSE);
  }
  else
  {
    REGSET_FOREACH_REG(available, adr)
      break;
    REGSET_FOREACH_REG(available, offset)
      if(adr != offset) break;
  }

  /* The relocation for the address producer, this will produce the address of the data BBL containing the address of the binary base */
  ArmMakeInsForBbl(Mov, Append, ins, bbl, FALSE, adr, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */
  t_reloc* rel = RelocTableAddRelocToRelocatable(address_producer_table,
    AddressNullForObject(obj), /* addend */
    T_RELOCATABLE(ins), /* from */
    AddressNullForObject(obj), /* from-offset */
    T_RELOCATABLE(binary_base_bbl), /* to */
    AddressNullForObject(obj), /* to-offset */
    FALSE, /* hell */
    NULL, /* edge */
    NULL, /* corresp */
    NULL, /* sec */
    "R00A00+\\l*w\\s0000$");
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  ArmMakeInsForBbl(Ldr, Append, ins, bbl, FALSE, adr, adr, ARM_REG_NONE,
    0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  /* The relocation for the address producer, this will produce the offset of the target BBL in the binary */
  ArmMakeInsForBbl(Mov, Append, ins, bbl, FALSE, offset, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */
  rel = RelocTableAddRelocToRelocatable(address_producer_table,
    AddressNullForObject(obj), /* addend */
    T_RELOCATABLE(ins), /* from */
    AddressNullForObject(obj), /* from-offset */
    to, /* to */
    AddressNullForObject(obj), /* to-offset */
    FALSE, /* hell */
    NULL, /* edge */
    NULL, /* corresp */
    T_RELOCATABLE(binary_base_sec), /* sec */
    "R00R01-\\l*w\\s0000$");
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);
  RELOC_SET_LABEL(rel, StringDup(transformed_reloc_label));

  ArmMakeInsForBbl(Add, Append, ins, bbl, FALSE, adr, adr, offset, 0, ARM_CONDITION_AL);

  /* Store the target address in the place on the stack previously reserved. */
  if(nr_of_dead_regs == 0)
    ArmMakeInsForBbl(Str, Append, ins, bbl, FALSE, adr, ARM_REG_R13, ARM_REG_NONE,
      8 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  t_cfg* target_cfg = BBL_CFG(CFG_EDGE_TAIL(edge));
  switch (CFG_EDGE_CAT(edge))
  {
    case ET_CALL:
    {
      /* Create new hell edge for target and adjust existing edge to go to hell */
      t_cfg_edge* call_edge = CfgEdgeCreateCall (target_cfg, CFG_HELL_NODE(target_cfg), T_BBL(CFG_EDGE_TAIL(edge)), CFG_EXIT_HELL_NODE(target_cfg), FunctionGetExitBlock(BBL_FUNCTION(CFG_EDGE_TAIL(edge))));
      CFG_EDGE_SET_FLAGS(call_edge, CFG_EDGE_FLAGS(call_edge) | transformed_hell_edge_flag);
      CfgEdgeChangeTail (edge, CFG_CALL_HELL_NODE(new_cfg));
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);

      /* We can either generate a BLX if the 'adr'-register was dead on beforehand, or else emulate a BLX */
      if(nr_of_dead_regs == 0)
      {
        /* We need to emulate a BLX instruction by manually putting the return address into LR and popping the address
         * we want to call to into the PC (together with the registers we saved). We start by moving the PC into LR.
         */
        ArmMakeInsForBbl(Mov, Append, ins, bbl, FALSE, ARM_REG_R14, ARM_REG_R15, 0, ARM_CONDITION_AL);
        ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << ARM_REG_R15) | (1 << adr) | (1 << offset), ARM_CONDITION_AL, FALSE);
      }
      else
      {
        if(nr_of_dead_regs == 1)
          ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << offset), ARM_CONDITION_AL, FALSE);

        ArmMakeInsForBbl(CondBranchLinkAndExchange, Append, ins, bbl, FALSE, ARM_CONDITION_AL, adr);
      }
      break;
    }

    case ET_IPJUMP:
    {
      /* Create new hell edge for target and adjust existing edge to go to hell */
      t_cfg_edge* ip = CfgEdgeCreate (target_cfg, CFG_HELL_NODE(target_cfg), T_BBL(CFG_EDGE_TAIL(edge)), ET_IPJUMP);
      if (FunctionGetExitBlock(BBL_FUNCTION(T_BBL(CFG_EDGE_TAIL(edge)))))/* Only create a compensating if possible */
          CfgEdgeCreateCompensating(target_cfg, ip);
      CFG_EDGE_SET_FLAGS(ip, CFG_EDGE_FLAGS(ip) | transformed_hell_edge_flag);
      CfgEdgeChangeTail (edge, CFG_HELL_NODE(new_cfg));
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);

      /* We can either generate a BX if the 'adr'-register was dead on beforehand, or else emulate a BX */
      if(nr_of_dead_regs == 0)
      {
        /* We need to emulate a BX instruction by manually popping the address we want to call to into the PC
         * (together with the registers we saved).
         */
        ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << ARM_REG_R15) | (1 << adr) | (1 << offset), ARM_CONDITION_AL, FALSE);
      }
      else
      {
        if(nr_of_dead_regs == 1)
          ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << offset), ARM_CONDITION_AL, FALSE);

        ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, FALSE, adr);
      }
      break;
    }

    case ET_IPSWITCH:
    {
      FATAL(("Unsupported for now!"));
    }

    case ET_IPFALLTHRU:
    {
      /* Create new hell edge for target and adjust existing edge to go to hell */
      t_cfg_edge* ip = CfgEdgeCreate (target_cfg, CFG_HELL_NODE(target_cfg), T_BBL(CFG_EDGE_TAIL(edge)), ET_IPJUMP);
      if (FunctionGetExitBlock(BBL_FUNCTION(T_BBL(CFG_EDGE_TAIL(edge)))))/* Only create a compensating if possible */
        CfgEdgeCreateCompensating(target_cfg, ip);
      CFG_EDGE_SET_FLAGS(ip, CFG_EDGE_FLAGS(ip) | transformed_hell_edge_flag);
      CfgEdgeChangeTail (edge, CFG_HELL_NODE(new_cfg));
      CFG_EDGE_SET_CAT(edge, ET_IPJUMP);
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);

      /* We can either generate a BX if the 'adr'-register was dead on beforehand, or else emulate a BX */
      if(nr_of_dead_regs == 0)
      {
        /* We need to emulate a BX instruction by manually popping the address we want to call to into the PC
         * (together with the registers we saved).
         */
        ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << ARM_REG_R15) | (1 << adr) | (1 << offset), ARM_CONDITION_AL, FALSE);
      }
      else
      {
        if(nr_of_dead_regs == 1)
          ArmMakeInsForBbl(Pop, Append, ins, bbl, FALSE, (1 << offset), ARM_CONDITION_AL, FALSE);

        ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, FALSE, adr);
      }
      break;
    }

    default:
      FATAL(("Case not implemented!"));
  }
}

/* The address of the GMRT and the index of the function are loaded, and subsequently used to jump to the entry
 * in the GMRT. To do this we some registers, we'll use dead ones if available but if necessary push/pop.
 */
void GMRTTransformer::TransformIncomingEdgeImpl(t_bbl* bbl, t_cfg_edge* edge)
{
  /* Find out how many registers there are available */
  t_regset available = RegsetDiff(possible, BblRegsLiveAfter(bbl));

  /* We will generate the following code:
   * [OPTIONAL] PUSH {adr, empty} (always r0 and r1)
   * ADR adr, gmrt_entry (gmrt_sec + 4 * index)
   * LDR adr, [adr]
   * [OPTIONAL] SUB adr, adr, 4
   * BLX adr or BX adr, depending on which type of edge we're dealing with.
   */

  /* The code for calling transformed code uses two registers. We can either use two dead registers (if available),
   * use one dead register and a live register that needs to be pushed/popped, or two live registers
   * that need to be pushed/popped.
   */
  t_reg adr;
  t_arm_ins* ins;
  t_uint32 nr_of_dead_regs = RegsetCountRegs(available);
  if(nr_of_dead_regs == 0)
  {
    adr = ARM_REG_R0;
    ArmMakeInsForBbl(Push, Append, ins, bbl, FALSE, (1 << adr) | (1 << ARM_REG_R1), ARM_CONDITION_AL, FALSE);
  }
  else
  {
    REGSET_FOREACH_REG(available, adr)
      break;
  }

  /* The relocation for the address producer, this will produce the destination address */
  ArmMakeInsForBbl(Mov, Append, ins, bbl, FALSE, adr, ARM_REG_NONE, 0, ARM_CONDITION_AL); /* Just get us an instruction with a correct regA */
  t_reloc* rel = RelocTableAddRelocToRelocatable(address_producer_table,
    AddressNullForObject(obj), /* addend */
    T_RELOCATABLE(ins), /* from */
    AddressNullForObject(obj), /* from-offset */
    T_RELOCATABLE(gmrt_sec), /* to */
    transform_index * gmrt_entry_size, /* to-offset */
    FALSE, /* hell */
    NULL, /* edge */
    NULL, /* corresp */
    NULL, /* sec */
    "R00A00+\\l*w\\s0000$");
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  /* After this load, 'adr' should contain the address of the split-off function (or stub) */
  ArmMakeInsForBbl(Ldr, Append, ins, bbl, FALSE, adr, adr, ARM_REG_NONE,
    0 /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);

  /* If we have to pop the r0 and r1 registers before continuing execution of the transformed code, subtract 4 from the
   * calculated address. This will land us on a pop {r0, r1} situated right before the actual code.
   */
  if(nr_of_dead_regs == 0)
    ArmMakeInsForBbl(Sub, Append, ins, bbl, FALSE, adr, adr, ARM_REG_NONE, 4, ARM_CONDITION_AL);

  switch (CFG_EDGE_CAT(edge))
  {
    case ET_CALL:
    {
      /* Adjust edge and generate new indirect BLX */
      CfgEdgeChangeTail(edge, CFG_CALL_HELL_NODE(cfg));
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);
      ArmMakeInsForBbl(CondBranchLinkAndExchange, Append, ins, bbl, FALSE, ARM_CONDITION_AL, adr);
      break;
    }

    case ET_IPJUMP:
    {
      /* Adjust edge and generate new indirect BX */
      CfgEdgeChangeTail(edge, CFG_HELL_NODE(cfg));
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);
      ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, FALSE, adr);
      break;
    }

    case ET_IPSWITCH:
    {
      FATAL(("Unsupported for now!"));
    }

    case ET_IPFALLTHRU:
    {
      /* Adjust edge and generate new indirect BX */
      CfgEdgeChangeTail(edge, CFG_HELL_NODE(cfg));
      CFG_EDGE_SET_CAT(edge, ET_IPJUMP);
      CFG_EDGE_SET_FLAGS(edge, CFG_EDGE_FLAGS(edge) | transformed_hell_edge_flag);
      ArmMakeInsForBbl(UncondBranchExchange, Append, ins, bbl, FALSE, adr);
      break;
    }

    default:
      FATAL(("Case not implemented!"));
  }
}

void GMRTTransformer::TransformBbl (t_bbl* bbl)
{
  t_ins* ins, *tmp_ins;
  BBL_FOREACH_INS_SAFE(bbl, ins, tmp_ins)
  {
    t_arm_ins* arm_ins = T_ARM_INS(ins);

    if(ARM_INS_OPCODE(arm_ins) == ARM_ADDRESS_PRODUCER)
    {
      t_reloc* rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(arm_ins));

      /* We differentiate between internal and external addresses. An internal address is one located in:
       * - a mobile section
       * - a switch table that is part of the transformed function
       */
      if ((RELOC_N_TO_RELOCATABLES(rel) == 1) && (
            /* Mobile section */
            ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_SUBSECTION)
             && (mobile_sections.find(T_SECTION(RELOC_TO_RELOCATABLE(rel)[0])) != mobile_sections.end()))
            /* Switch table */
            || ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL) && (BBL_CFG(T_BBL(RELOC_TO_RELOCATABLE(rel)[0])) == new_cfg))))
      {
        /* Normal internal address producers don't have to be transformed, we only have to move their relocation
         * to the relocation table of the transformed object. If the value they produce is an offset however,
         * things get more complicated.
         */
        if (RelocIsRelative(rel))
          TransformAddressProducer(arm_ins);
        else
          RelocMoveToRelocTable(rel, OBJECT_RELOC_TABLE(new_objs.back()));
      }
      else
      {
        /* If the the address being produced is an offset to the GOT we're dealing with something more alike to a
         * constant producer as a real address producer, and thus this shouldn't be transformed. We will move the
         * relocation to the address producer table however, as it will be an inter-object relocation.
         */
        if (RelocIsRelative(rel))
          RelocMoveToRelocTable(rel, address_producer_table);
        else
          TransformAddressProducer(arm_ins);
      }
    }
  }
}

void GMRTTransformer::TransformEntrypoint(t_function* fun)
{
  /* Add a BBL that will contain the optional pop instruction */
  t_arm_ins* ins;
  t_bbl* pop_bbl = BblNew(new_cfg);
  BblInsertInFunction(pop_bbl, fun);
  ArmMakeInsForBbl(Pop, Append, ins, pop_bbl, FALSE, (1 << ARM_REG_R0) | (1 << ARM_REG_R1), ARM_CONDITION_AL, FALSE);
  CfgEdgeCreate(new_cfg, pop_bbl, FUNCTION_BBL_FIRST(fun), ET_FALLTHROUGH);

  /* Add a DATA_BBL that will contain the binary base */
  binary_base_bbl = BblNew(new_cfg);
  BblInsertInFunction(binary_base_bbl, fun);
  BBL_SET_ATTRIB(binary_base_bbl, BBL_ATTRIB(binary_base_bbl)| BBL_IS_DATA);
  ArmMakeInsForBbl(Data, Append, ins, binary_base_bbl, FALSE, 0);
  CfgEdgeCreate(new_cfg, binary_base_bbl, pop_bbl, ET_FALLTHROUGH);
}

void GMRTTransformer::RelocIsTransformed(t_reloc* rel, t_bool* is_relative)
{
  /* Code mobility related relative relocations */
  if (RELOC_LABEL(rel) && (strncmp(RELOC_LABEL(rel), transform_label, strlen(transform_label)) == 0))
    *is_relative = TRUE;
}

/* This function finds all relocatables referred to from the function that could be made mobile,
 * for a list of requirements these relocatables should meet, see below.
 */
void GMRTTransformer::SelectMobileDataForFunction(const t_function* fun)
{
  t_bbl* bbl;
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    t_ins* ins;
    BBL_FOREACH_INS(bbl, ins)
    {
      t_reloc_ref* rr = INS_REFERS_TO(ins);
      while (rr)
      {
        t_reloc* reloc = RELOC_REF_RELOC(rr);
        rr = RELOC_REF_NEXT(rr);

        /* For every to relocatable we will examine whether it meets the requirements:
         * - it's referred to using a relocation with only 1 to relocatable
         * - it's a RODATA subsection
         * - it does not refer to any relocatables itself
         * - it's only referred to from instructions within this function
         * If these requirements are met we will add the section to the set. These requirements can still be expanded
         * later on (for example allowing sections to refer to instructions inside the function) but the limitations
         * are broad enough for the currently foreseen usecases.
         */
        if (RELOC_N_TO_RELOCATABLES(reloc) != 1)
          continue;

        t_relocatable* to = RELOC_TO_RELOCATABLE(reloc)[0];
        if (RELOCATABLE_RELOCATABLE_TYPE(to) != RT_SUBSECTION)
          continue;

        t_section* sec = T_SECTION(to);
        if (SECTION_TYPE(sec) != RODATA_SECTION)
          continue;

        if (RELOCATABLE_REFERS_TO(to))
          continue;

        t_reloc_ref* rr2 = RELOCATABLE_REFED_BY(to);
        bool referred_from_outside_fun = false;
        while (rr2)
        {
          t_reloc* reloc2 = RELOC_REF_RELOC(rr2);
          t_relocatable* from = RELOC_FROM(reloc2);

          if ((RELOCATABLE_RELOCATABLE_TYPE(from) != RT_INS) || (BBL_FUNCTION(INS_BBL(T_INS(from))) != fun))
          {
            referred_from_outside_fun = true;
            break;
          }

          rr2 = RELOC_REF_NEXT(rr2);
        }

        if (referred_from_outside_fun)
          continue;

        VERBOSE(2, ("Making section @T mobile.", T_RELOCATABLE(sec)));
        mobile_sections.insert(sec);
      }
    }
  }
}
