/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "abstract_transformer.h"

#include <diabloannotations.h>

using namespace std;

AbstractTransformer::AbstractTransformer(t_object* obj, t_const_string output_name, t_const_string log_suffix, t_bool srt,
    t_const_string non, t_const_string trl, t_uint32 edge_flag)
  : obj(obj), output_name(output_name), adr_size(AddressSizeInBytes(OBJECT_ADDRESS_SIZE(obj))), transform_index(0), transformed_hell_edge_flag(edge_flag),
  transformed_reloc_label(trl), L_TRANSFORMS(NULL), new_obj_name(non), address_producer_table(srt ? RelocTableNew(NULL) : OBJECT_RELOC_TABLE(obj)),
  separate_reloc_table(srt)
{
  /* Initialize regset containing all general purpose registers we can actually use */
  t_architecture_description* desc = ObjectGetArchitectureDescription(obj);
  possible = RegsetDiff(desc->int_registers, RegsetUnion(desc->always_live, desc->link_register));

  /* Initialize the log file if necessary */
  if(log_suffix && output_name)
  {
    t_const_string log_filename = StringConcat2 (output_name, log_suffix);
    INIT_LOGGING(L_TRANSFORMS, log_filename);
    Free(log_filename);
  }

  /* Find the first section of the object and use it as binary base */
  t_symbol* binary_base_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), "__text_start");
  ASSERT(SYMBOL_OFFSET_FROM_START(binary_base_sym) == 0, ("This symbol should be at the start of the .text-section!"));
  binary_base_sec = T_SECTION(SYMBOL_BASE(binary_base_sym));
  
  special_function_uid = FunctionUID_INVALID;
}

/* Cleanup of everything created/started in the constructor */
AbstractTransformer::~AbstractTransformer()
{
  if(separate_reloc_table)
    RelocTableFree(address_producer_table);
  FINI_LOGGING(L_TRANSFORMS);
}

/* When dealing with a split off CFG, we want to make sure the chain containing the entry_bbl is actually placed first */
void AbstractTransformer::AfterChainsOrdered (t_cfg* split_cfg, t_chain_holder* ch)
{
  /* Find the head of the chain we want to be first chain */
  t_bbl* chain_head = BBL_FIRST_IN_CHAIN(CFG_ENTRY(split_cfg)->entry_bbl);

  /* Get all chains and the current first chain */
  t_bbl** chains = ch->chains;
  t_bbl* tmp = *chains;

  /* Put the chain we want first and start moving all the rest one further until we reach the original
   * place of the new first chain.
   */
  chains[0] = chain_head;
  for(t_uint32 iii = 1; iii < ch->nchains; iii++)
  {
    if(tmp == chain_head) break;
    t_bbl* tmp2 = chains[iii];
    chains[iii] = tmp;
    tmp = tmp2;
  }
}

/* Create a stub to serve as the target for indirect control flow transfer. This stub will have a direct
 * jump edge to the actual destination, which is also transformed.
 */
t_bbl* AbstractTransformer::CreateIndirectionStub(t_bbl* dest)
{
  t_bbl* entrypoint = BblNew(cfg);
  BBL_SET_REGS_LIVE_OUT(entrypoint, BblRegsLiveBefore(dest));
  BBL_SET_REGS_NEVER_LIVE(entrypoint, BBL_REGS_NEVER_LIVE(dest));
  t_arm_ins* ins;
  ArmMakeInsForBbl(UncondBranch, Append, ins, entrypoint, FALSE);

  /* Make new function */
  char name[23];
  sprintf(name, "stub_indirect_%08x", transform_index);
  t_function* fun = FunctionMake(entrypoint, name, FT_NORMAL);
  ASSERT(special_function_uid != FunctionUID_INVALID, ("invalid special function uid!"));
  BblSetOriginalFunctionUID(entrypoint, special_function_uid);

  /* Model the control flow, start by adding an IPJUMP to the destination and model the control flow in and out of the stub */
  t_cfg_edge* ip1 = CfgEdgeCreate (cfg, entrypoint, dest, ET_IPJUMP);
  t_cfg_edge* ip2 = CfgEdgeCreate (cfg, CFG_HELL_NODE(cfg), entrypoint, ET_IPJUMP);

  /* If the target function has an exit block we should create compensating edges. If it doesn't we should kill the
   * exit block of the stub (and thus can't create compensating edges).
   */
  if (FunctionGetExitBlock(BBL_FUNCTION(dest)))
  {
    CfgEdgeCreateCompensating(cfg, ip1);
    CfgEdgeCreateCompensating(cfg, ip2);
  }
  else
    BblKill(FunctionGetExitBlock(fun));

  /* Immediately transform the edge to the destination we created */
  TransformIncomingEdge(ip1);

  return entrypoint;
}

void AbstractTransformer::TransformIncomingTransformedEdge (t_arm_ins* ins, t_reloc* reloc)
{
  t_bbl* bbl = ARM_INS_BBL(ins);
  t_function* fun_to_log = BBL_FUNCTION(bbl);
  t_const_string fun_name = FUNCTION_NAME(fun_to_log) ? FUNCTION_NAME(fun_to_log) : "no name";
  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformIncomingTransformedEdge,%s", fun_name) {
    LogFunctionTransformation("before", fun_to_log);
  }

  /* Do transformation-specific BBL rewriting */
  TransformIncomingTransformedEdgeImpl(ins, reloc);

  LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", BBL_FUNCTION(bbl)); }
  STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);
}

void AbstractTransformer::TransformOutgoingEdge (t_cfg_edge* edge)
{
  t_bbl* bbl = T_BBL(CFG_EDGE_HEAD(edge));
  t_function* fun_to_log = BBL_FUNCTION(bbl);
  t_const_string fun_name = FUNCTION_NAME(fun_to_log) ? FUNCTION_NAME(fun_to_log) : "no name";
  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformOutgoingEdge,%s", fun_name) {
    LogFunctionTransformation("before", fun_to_log);
  }

  /* This BBL might be the default case for a switch. This is implemented as a branch after
   * a conditional branch. The jumptable for the switch is placed right behind this BBL. Therefore we can't
   * transform it as this would mean an increase in size of the BBL and a larger offset to the jumptable.
   * We'll make a new BBL to contain the default branch and transformed code, and have the old one jump to it.
   */
  t_cfg_edge* tmp_edge;
  t_arm_ins* tmp;
  BBL_FOREACH_PRED_EDGE(bbl, tmp_edge)
    if (CfgEdgeIsFallThrough(tmp_edge))
      break;
  if(tmp_edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(tmp_edge));

    /* If any of the succeeding edges of head is a switch edge */
    BBL_FOREACH_SUCC_EDGE(head, tmp_edge)
      if (CfgEdgeTestCategoryOr(tmp_edge, ET_SWITCH | ET_IPSWITCH))
        break;
    if(tmp_edge)
    {
      t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);
      CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
      ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

      bbl = split;
    }
  }

  /* The BBL might be one of the cases of a switch where each case consists of one instruction and falls
   * through into the next case. This kind of switch is implemented using a simple add instruction. Perhaps
   * switches where each case consists of 2 instructions (or each consists of 3, or...) might also be implemented
   * in this way. I haven't encountered this though and it isn't supported.
   */
  BBL_FOREACH_PRED_EDGE(bbl, tmp_edge)
    if (CfgEdgeTestCategoryOr(tmp_edge, ET_SWITCH | ET_IPSWITCH))
      break;
  if(tmp_edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(tmp_edge));
    t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(head));
    if(ARM_INS_OPCODE(last) == ARM_ADD)
    {
      /* The original case is replaced by a BBL containing only a jump to the code in the original BBL.
       * The original BBL will be instrumented. If it only has a fallthrough edge, this edge is modified
       * to a jump back to the fallthrough case.
       */
      if(CfgEdgeIsFallThrough(BBL_SUCC_FIRST(bbl)) && CfgEdgeIsFallThrough(BBL_SUCC_LAST(bbl)))
      {
        t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(split), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Append, tmp, split, FALSE);
        bbl = split;
      }
      else
      {
        /* In case the cases of the add-switch contain only a jump */
        BBL_FOREACH_SUCC_EDGE(bbl, tmp_edge)
          if (CfgEdgeTestCategoryOr(tmp_edge, ET_JUMP | ET_IPJUMP))
            break;
        if(tmp_edge)
        {
          t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);
          CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
          ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

          bbl = split;
        }
      }
    }
  }

  /* Find the last instruction. This doesn't necessarily exist (in which case we don't do anything)
   * but it might be a B/BL that we'll need to kill depending on its condition code.
   */
  t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(bbl));
  t_relocatable* to = T_RELOCATABLE(CFG_EDGE_TAIL(edge));
  if (last && (CFG_EDGE_CAT(edge) != ET_IPFALLTHRU))
  {
    t_arm_condition_code condition = ARM_INS_CONDITION(last);

    /* If the B or BL is always executed, kill the instruction and append the new instructions
     * to the original BBL.
     */
    if(condition == ARM_CONDITION_AL)
    {
      /* Check if the outgoing edge is going to an imported function, if so we need to adjust the to_relocatable. For modelling purposes
       * the tail of the outgoing edge is a DYNCALL_HELL node instead of the actual to_relocatable. To find the actual to_relocatable we use
       * the relocation. This has to happen now because we need the last instruction for this, which will be removed.
       */
      if(BBL_CALL_HELL_TYPE(T_BBL(CFG_EDGE_TAIL(edge))) == BBL_CH_DYNCALL)
        to = RELOC_TO_RELOCATABLE(RELOC_REF_RELOC(ARM_INS_REFERS_TO(last)))[0];

      InsKill (T_INS(last));
    }
    else /* Create a new BBL in which we will place the new instructions */
    {
      if(BBL_CALL_HELL_TYPE(T_BBL(CFG_EDGE_TAIL(edge))) == BBL_CH_DYNCALL)
        FATAL(("Transforming conditional B/BL to an imported function. Not implemented yet!"));

      /* If we're dealing with conditional control flow, keep the conditional instruction intact and change its
       * target to a newly created BBL that will contain the new instructions created here.
       */
      t_bbl* tmp_bbl = BblNew(new_cfg);
      BblInsertInFunction(tmp_bbl, BBL_FUNCTION(bbl));
      BBL_SET_REGS_LIVE_OUT(tmp_bbl, BblRegsLiveAfter(bbl));
      BBL_SET_REGS_NEVER_LIVE(tmp_bbl, BBL_REGS_NEVER_LIVE(bbl));

      if(ARM_INS_OPCODE(last) == ARM_B)
      {
        CfgEdgeChangeHead(edge, tmp_bbl);
        CfgEdgeCreate(new_cfg, bbl, tmp_bbl, ET_JUMP);
      }
      else if(ARM_INS_OPCODE(last) == ARM_BL)
      {
        /* When dealing with a conditional BL create a new interprocedural edge */
        tmp_edge = CfgEdgeCreate(new_cfg, tmp_bbl, CFG_EDGE_TAIL(edge), ET_IPJUMP);
        CfgEdgeChangeTail(edge, tmp_bbl);
        edge = tmp_edge;
      }

      bbl = tmp_bbl;
    }
  }
  else if(last && (CFG_EDGE_CAT(edge) == ET_IPFALLTHRU))
  {
    /* Create a new BBL in which we will place the new instructions */
    t_bbl* tmp_bbl = BblNew(new_cfg);
    BblInsertInFunction(tmp_bbl, BBL_FUNCTION(bbl));
    BBL_SET_REGS_LIVE_OUT(tmp_bbl, BblRegsLiveAfter(bbl));
    BBL_SET_REGS_NEVER_LIVE(tmp_bbl, BBL_REGS_NEVER_LIVE(bbl));
    CfgEdgeChangeHead(edge, tmp_bbl);
    CfgEdgeCreate(new_cfg, bbl, tmp_bbl, ET_FALLTHROUGH);
    bbl = tmp_bbl;
  }

  /* Do transformation-specific BBL rewriting */
  TransformOutgoingEdgeImpl (bbl, edge, to);

  LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", BBL_FUNCTION(bbl)); }
  STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);
}

/* This will transform the BBL on an incoming edge to a transformed function. */
void AbstractTransformer::TransformIncomingEdge (t_cfg_edge* edge)
{
  t_bbl* bbl = T_BBL(CFG_EDGE_HEAD(edge));
  t_function* fun_to_log = BBL_FUNCTION(bbl);
  t_const_string fun_name = FUNCTION_NAME(fun_to_log) ? FUNCTION_NAME(fun_to_log) : "no name";
  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformIncomingEdge,%s", fun_name) {
    LogFunctionTransformation("before", fun_to_log);
  }

  /* This BBL might be the default case for a switch. This is implemented as a branch after
   * a conditional branch. The jumptable for the switch is placed right behind this BBL. Therefore we can't
   * transform it as this would mean an increase in size of the BBL and a larger offset to the jumptable.
   * We'll make a new BBL to contain the default branch and transformed code, and have the old one jump to it.
   */
  t_cfg_edge* tmp_edge;
  t_arm_ins* tmp;
  BBL_FOREACH_PRED_EDGE(bbl, tmp_edge)
    if (CfgEdgeIsFallThrough(tmp_edge))
      break;
  if(tmp_edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(tmp_edge));

    /* If any of the succeeding edges of head is a switch edge */
    BBL_FOREACH_SUCC_EDGE(head, tmp_edge)
      if (CfgEdgeTestCategoryOr(tmp_edge, ET_SWITCH | ET_IPSWITCH))
        break;
    if(tmp_edge)
    {
      t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);
      CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
      ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

      bbl = split;
    }
  }

  /* The BBL might be one of the cases of a switch where each case consists of one instruction and falls
   * through into the next case. This kind of switch is implemented using a simple add instruction. Perhaps
   * switches where each case consists of 2 instructions (or each consists of 3, or...) might also be implemented
   * in this way. I haven't encountered this though and it isn't supported.
   */
  BBL_FOREACH_PRED_EDGE(bbl, tmp_edge)
    if (CfgEdgeTestCategoryOr(tmp_edge, ET_SWITCH | ET_IPSWITCH))
      break;
  if(tmp_edge)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(tmp_edge));
    t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(head));
    if(ARM_INS_OPCODE(last) == ARM_ADD)
    {
      /* The original case is replaced by a BBL containing only a jump to the code in the original BBL.
       * The original BBL will be instrumented. If it only has a fallthrough edge, this edge is modified
       * to a jump back to the fallthrough case.
       */
      if(CfgEdgeIsFallThrough(BBL_SUCC_FIRST(bbl)) && CfgEdgeIsFallThrough(BBL_SUCC_LAST(bbl)))
      {
        t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

        CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(split), ET_JUMP);
        ArmMakeInsForBbl(UncondBranch, Append, tmp, split, FALSE);
        bbl = split;
      }
      else
      {
        /* In case the cases of the add-switch contain only a jump */
        BBL_FOREACH_SUCC_EDGE(bbl, tmp_edge)
          if (CfgEdgeTestCategoryOr(tmp_edge, ET_JUMP | ET_IPJUMP))
            break;
        if(tmp_edge)
        {
          t_bbl* split = BblSplitBlock (bbl, T_INS(BBL_INS_FIRST(bbl)), TRUE);
          CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(bbl), ET_JUMP);
          ArmMakeInsForBbl(UncondBranch, Prepend, tmp, bbl, FALSE);

          bbl = split;
        }
      }
    }
  }

  /* Find the last instruction. This doesn't necessarily exist (in which case we don't do anything)
   * but it might be a B/BL that we'll need to kill depending on its condition code.
   */
  t_arm_ins* last = T_ARM_INS(BBL_INS_LAST(bbl));
  if (last && (CFG_EDGE_CAT(edge) != ET_IPFALLTHRU))
  {
    t_arm_condition_code condition = ARM_INS_CONDITION(last);

    if (condition == ARM_CONDITION_AL)
      InsKill (T_INS(last));
    else
    {
      /* If we're dealing with conditional control flow, keep the conditional instruction intact and change its
       * target to a newly created BBL that will contain the new instructions created here.
       */
      t_bbl* tmp = BblNew(cfg);
      BblInsertInFunction(tmp, BBL_FUNCTION(bbl));
      BBL_SET_REGS_LIVE_OUT(tmp, BblRegsLiveAfter(bbl));
      BBL_SET_REGS_NEVER_LIVE(tmp, BBL_REGS_NEVER_LIVE(bbl));

      if(ARM_INS_OPCODE(last) == ARM_B)
      {
        CfgEdgeChangeHead(edge, tmp);
        CfgEdgeCreate(cfg, bbl, tmp, ET_JUMP);
      }
      else if(ARM_INS_OPCODE(last) == ARM_BL)
      {
        /* When dealing with a conditional BL create a new interprocedural edge */
        tmp_edge = CfgEdgeCreate(cfg, tmp, CFG_EDGE_TAIL(edge), ET_IPJUMP);
        CfgEdgeCreateCompensating(cfg, tmp_edge);
        CfgEdgeChangeTail(edge, tmp);
        edge = tmp_edge;
      }

      bbl = tmp;
    }
  }
  else if(last && (CFG_EDGE_CAT(edge) == ET_IPFALLTHRU))
  {
    t_bbl* tmp = BblNew(cfg);
    BblInsertInFunction(tmp, BBL_FUNCTION(bbl));
    BBL_SET_REGS_LIVE_OUT(tmp, BblRegsLiveAfter(bbl));
    BBL_SET_REGS_NEVER_LIVE(tmp, BBL_REGS_NEVER_LIVE(bbl));
    CfgEdgeChangeHead(edge, tmp);
    CfgEdgeCreate(cfg, bbl, tmp, ET_FALLTHROUGH);
    bbl = tmp;
  }

  /* Do transformation-specific BBL rewriting */
  TransformIncomingEdgeImpl(bbl, edge);

  LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", BBL_FUNCTION(bbl)); }
  STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);
}

void AbstractTransformer::TransformFunction (t_function* fun, t_bool split_function_from_cfg)
{
  t_const_string fun_name = FUNCTION_NAME(fun) ? FUNCTION_NAME(fun) : "no name";

  /* Find all incoming interprocedural edges and transform it. */
  t_cfg_edge* edge, *tmp;
  t_bbl* entry_bbl = FUNCTION_BBL_FIRST(fun);
  t_bbl* stub = NULL;
  BBL_FOREACH_PRED_EDGE_SAFE(entry_bbl, edge, tmp)
  {
    t_bbl* head = T_BBL(CFG_EDGE_HEAD(edge));

    /* Only transform BBL's that are not a part of this function. */
    if (CfgEdgeIsForwardInterproc(edge))
    {
      /* Edges coming from hell shouldn't be transformed. These edges fit in three cases:
       * 1. The edge is the unique entry edge, for which a stub is created that will be the new CFG entry.
       * 2. The edge is an incoming transformed edge, which is killed (we'll deal with the relocation later on).
       * 3. The edge is a true original hell edge. We create a stub and kill the edge (we'll deal with the relocation later on).
       */
      if(!BBL_IS_HELL(head))
        TransformIncomingEdge(edge);
      else
      {
        if (!stub && ((edge == CFG_ENTRY(cfg)->entry_edge) || !(CFG_EDGE_FLAGS(edge) & transformed_hell_edge_flag)))
          stub = CreateIndirectionStub(entry_bbl);

        /* If we're transforming the function with the unique CFG entrypoint, also move this to the stub */
        if (CFG_ENTRY(cfg)->entry_bbl == entry_bbl)
          CfgReplaceEntry(cfg, stub);
        else
          CfgEdgeKillKeepRel(edge);
      }
    }
    /* Later on we'll add a new BBL that falls through onto the entry BBL. Therefore we must make sure
     * the entry BBL has no fallthrough edges from inside the function arriving. If there are any,
     * we'll turn them into jump edges by adding a new stub BBL.
     */
    else if (CfgEdgeTestCategoryOr(edge, ET_FALLTHROUGH))
    {
      /* Create a new BBL that will serve as a stub */
      t_bbl* stub = BblNew(cfg);
      BblInsertInFunction(stub, fun);
      BBL_SET_REGS_LIVE_OUT(stub, BblRegsLiveAfter(entry_bbl));
      BBL_SET_REGS_NEVER_LIVE(stub, BBL_REGS_NEVER_LIVE(entry_bbl));

      /* Create a branch instruction and change the edges */
      t_arm_ins* tmp_ins;
      ArmMakeInsForBbl(UncondBranch, Prepend, tmp_ins, stub, FALSE);
      CfgEdgeChangeTail(edge, stub);
      CfgEdgeCreate(cfg, stub, entry_bbl, ET_JUMP);
    }
    else if (CfgEdgeTestCategoryOr(edge, ET_RETURN))
    {
      /* If we don't have an indirection stub yet, create one */
      if (!stub)
        stub = CreateIndirectionStub(entry_bbl);

      /* Change the return edge so it returns to the stub */
      CfgEdgeChangeTail (edge, stub);
    }
  }

  t_object* new_obj = NULL;
  if (split_function_from_cfg)
  {
    /* Create new object for the function. Duplicate the needed information from the original object,
     * and initialize the right structures.
     */
    char name[strlen(new_obj_name) + 9];
    sprintf(name, "%s%08x", new_obj_name, transform_index);
    new_obj = ObjectNewCached(name, NULL);
    OBJECT_SET_ADDRESS_SIZE(new_obj, OBJECT_ADDRESS_SIZE(obj));
    OBJECT_SET_SWITCHED_ENDIAN(new_obj, OBJECT_SWITCHED_ENDIAN(obj));
    OBJECT_SET_OBJECT_HANDLER(new_obj, OBJECT_OBJECT_HANDLER(obj));
    /* Use original object to get layout script, as this requires a map (which the new object doesn't have) */
    OBJECT_SET_LAYOUT_SCRIPT(new_obj, ObjectGetAndParseLayoutScript (obj));
    OBJECT_SET_TYPE(new_obj, OBJTYP_SHARED_LIBRARY_PIC);
    OBJECT_SET_SUBOBJECT_CACHE(new_obj, HashTableNew (3001, 0, (t_hash_func) StringHash, (t_hash_cmp) StringCmp, ObjectHashElementFree));
    OBJECT_SET_SUB_SYMBOL_TABLE(new_obj, SymbolTableNew (new_obj));
    OBJECT_SET_RELOC_TABLE(new_obj, RelocTableNew (new_obj));
    SectionCreateForObject(new_obj, CODE_SECTION, NULL, 0, ".text");

    /* Duplicate the _GLOBAL_OFFSET_TABLE_ symbol into the new object's SUB_SYMBOL_TABLE, as this symbol
     * will be used when executing relocations containing the 'g' code.
     */
    t_symbol* got_symbol = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), "_GLOBAL_OFFSET_TABLE_");
    if (got_symbol)
      SymbolTableDupSymbol(OBJECT_SUB_SYMBOL_TABLE(new_obj), got_symbol, "_GLOBAL_OFFSET_TABLE_");

    /* Create a new CFG to move the split-off function to */
    new_cfg = CfgCreate(new_obj);
    OBJECT_SET_CFG(new_obj, new_cfg);
    CfgCreateHellNodesAndEntry (new_obj, new_cfg, AddressNewForCfg(new_cfg, -1), entry_bbl);

    /* This is a temporary hack. The dynamic members for every new CFG must be initialized. The proper way to
     * do this would be initializing them in the anoptarm callback after CfgInit, but for some reason this
     * does not work.
     */
    CfgInitRegions(new_obj);
    BblInitRegions(OBJECT_CFG(new_obj));

    /* Add the new object to vector of new objects */
    new_objs.push_back(new_obj);

    /* Move function to new CFG */
    FunctionUnlinkFromCfg(fun);
    FunctionInsertInCfg(new_cfg, fun);
    FUNCTION_SET_CFG(fun, new_cfg);
  }
  else
    new_cfg = cfg;

  /* Transform the entrypoint now, so any eventual BBLs created during the transformation will end up in the vector of all
   * BBLs to be transformed.
   */
  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformEntrypoint,%s", fun_name) {
    LogFunctionTransformation("before", fun);
  }
  TransformEntrypoint(fun);
  LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", fun); }
  STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);

  /* Build a vector of all BBLs in the function. We need to build a vector because the transformations themselves
   * can create new BBLs, so we need to know which BBLs were already there originally and which we created ourselves.
   */
  t_bbl* bbl;
  vector<t_bbl*> bbls;
  t_bbl* exit_bbl = FunctionGetExitBlock(fun);
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    /* If requested we will move all the function's BBLs, instructions, and edges */
    if (split_function_from_cfg)
    {
      /* Move BBL to new CFG */
      if (BBL_CFG(bbl) != new_cfg)
      {
        DiabloBrokerCall("RemoveBblFromCfg", bbl);
        CfgUnlinkNodeFromGraph(cfg, bbl);
        CfgInsertNodeInGraph(new_cfg, bbl);
        BBL_SET_CFG(bbl, new_cfg);
      }

      /* Move all instructions of the BBL to new CFG */
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins)
        INS_SET_CFG(ins, new_cfg);

      if(bbl == exit_bbl || IS_DATABBL(bbl))
        continue;

      t_bool switch_bbl = FALSE;
      BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, tmp)
      {
        /* Check if the BBL ends in a switch */
        if (CfgEdgeTestCategoryOr(edge, ET_SWITCH | ET_IPSWITCH))
          switch_bbl = TRUE;

        /* Move edge to new CFG */
        if (CFG_EDGE_CFG(edge) != new_cfg)
        {
          CfgUnlinkEdgeFromGraph(cfg, edge);
          CfgInsertEdgeInGraph(new_cfg, edge);
          CFG_EDGE_SET_CFG(edge, new_cfg);
        }

        /* If the edge has a corresponding, move it as well */
        t_cfg_edge* corr = CFG_EDGE_CORR(edge);
        if(corr && CFG_EDGE_CFG(corr) != new_cfg)
        {
          /* Move corr to new CFG */
          CfgUnlinkEdgeFromGraph(cfg, corr);
          CfgInsertEdgeInGraph(new_cfg, corr);
          CFG_EDGE_SET_CFG(corr, new_cfg);
        }
      }

      /* If the BBL ends in a switch we need to move the potential switch BBL to the new CFG as well */
      if (switch_bbl)
      {
        BBL_FOREACH_SUCC_EDGE(bbl, edge)
          if (CfgEdgeTestCategoryOr(edge, ET_SWITCH | ET_IPSWITCH))
            break;

        t_bbl* target = CFG_EDGE_TAIL(edge);

        t_reloc_ref* rr = BBL_REFED_BY(target);
        t_bbl* switchtable = NULL;
        while (rr)
        {
          /* Get reloc and go to next */
          t_reloc* reloc = RELOC_REF_RELOC(rr);
          rr = RELOC_REF_NEXT(rr);

          t_bbl* candidate = INS_BBL(T_INS(RELOC_FROM(reloc)));
          if (IS_DATABBL(candidate))
          {
            switchtable = candidate;
            break;
          }
        }

        /* Move switchtable to new CFG */
        if (switchtable != NULL)
        {
          CfgUnlinkNodeFromGraph(cfg, switchtable);
          CfgInsertNodeInGraph(new_cfg, switchtable);
          BBL_SET_CFG(switchtable, new_cfg);

          /* Move all instructions of the BBL to new CFG */
          t_ins* ins;
          BBL_FOREACH_INS(switchtable, ins)
            INS_SET_CFG(ins, new_cfg);

          /* Move all relocations associated with the table to the new object's reloc table */
          BBL_FOREACH_INS(switchtable, ins)
          {
            t_reloc_ref* rr = INS_REFERS_TO(ins);
            while (rr)
            {
              /* Move reloc and go to next */
              RelocMoveToRelocTable(RELOC_REF_RELOC(rr), OBJECT_RELOC_TABLE(new_obj));
              rr = RELOC_REF_NEXT(rr);
            }
          }
        }
      }
    }

    /* Add BBL to the vector, so we can transform it later */
    bbls.push_back(bbl);
  }

  /* Transform all edges out and the BBLs themselves */
  for (auto bbl: bbls)
  {
    /* Get all the outgoing edges of the BBL before we transform it, as this function may add edges */
    vector<t_cfg_edge*> edges;
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
      edges.push_back(edge);

    /* Transform the contents of the BBL */
    START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformBbl,%s", fun_name) {
      LogFunctionTransformation("before", fun);
    }
    TransformBbl (bbl);
    LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", fun); }
    STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);

    for (auto edge : edges)
    {
      /* We assume that edges going to hell have been created by transformed code (except for edges going to a DYNCALL_HELL node,
       * these must be transformed as they're going to a dynamically linked function). This means they shouldn't be transformed anymore.
       */
      if (CfgEdgeIsForwardInterproc(edge))
      {
        t_bbl* tail = T_BBL(CFG_EDGE_TAIL(edge));
        if (!BBL_IS_HELL(tail) || (BBL_CALL_HELL_TYPE(tail) == BBL_CH_DYNCALL))
          TransformOutgoingEdge (edge);
        else if (split_function_from_cfg)
        {
          /* The edge might go to a hell node from the original CFG, replace this by the hell node of the new CFG */
          t_bbl* orig_tail = CFG_EDGE_TAIL(edge);
          if (orig_tail == CFG_CALL_HELL_NODE(cfg))
            CfgEdgeChangeTail(edge, CFG_CALL_HELL_NODE(new_cfg));
          else if (orig_tail == CFG_HELL_NODE(cfg))
            CfgEdgeChangeTail(edge, CFG_HELL_NODE(new_cfg));
          else {
            t_bbl *result = NULL;
            DiabloBrokerCall("RedirectGlobalTargetHellEdgeToCfg", edge, &result);

            if (result) {
              /* global target hell */
              CfgEdgeChangeTail(edge, result);
            }
          }
        }
      }
      /* Else, check if we're dealing with an edge that exits the function */
      else if (!CfgEdgeIsInterproc(edge) && (CFG_EDGE_TAIL(edge) == exit_bbl))
      {
        START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_TRANSFORMS, "TransformExit,%s", fun_name) {
          LogFunctionTransformation("before", fun);
        }
        TransformExit (edge);
        LOG_MORE(L_TRANSFORMS) { LogFunctionTransformation("after", fun); }
        STOP_LOGGING_TRANSFORMATION(L_TRANSFORMS);
      }
    }
  }

  /* If the entry BBL is still being refered to, this could be for two reasons:
   * 1. It's refered to from an address producer created in a TransformOutgoingEdge of a function
   * already split off that calls this function. We transform these edges in a slightly different manner.
   * 2. The relocation is associated with an original hell edge. We point all these relocations at
   * the stub that was created earlier.
   */
  t_reloc_ref* rr = BBL_REFED_BY(entry_bbl);
  while (rr)
  {
    /* Keep a pointer to the next RR as we'll change the TO_RELOCATABLE of the reloc */
    t_reloc_ref* tmp_rr = RELOC_REF_NEXT(rr);
    t_reloc* reloc = RELOC_REF_RELOC(rr);

      /* If it's an incoming edge that has already been transformed, transform it again */
    if (RELOC_LABEL(reloc) && !strcmp(transformed_reloc_label, RELOC_LABEL(reloc)))
    {
      t_arm_ins* adr = T_ARM_INS(RELOC_FROM(reloc));
      TransformIncomingTransformedEdge (adr, reloc);
    }
    else if (RELOC_HELL(reloc))
    {
      /* Get the index of the entry_bbl as to_relocatable in the reloc, and replace it with the stub */
      ASSERT(stub, ("Trying to replace a relocatable with a stub, but no stub was created!"));
      t_uint32 index = RelocGetToRelocatableIndex(reloc, T_RELOCATABLE(entry_bbl));
      RelocSetToRelocatable(reloc, index, T_RELOCATABLE(stub));
    }

    /* Go to the next reloc_ref */
    rr = tmp_rr;
  }

  if (split_function_from_cfg)
  {
    /* Kill all symbols that refer to the BBL. Or move them to the stub if present (symbol might be exported). */
    while (BBL_REFED_BY_SYM(entry_bbl))
    {
      /* Keep a pointer to the next SR as we'll change the base of the symbol */
      t_symbol_ref* sr = BBL_REFED_BY_SYM(entry_bbl);
      t_symbol* sym = sr->sym;

      if (stub)
        SymbolSetBase(sym, T_RELOCATABLE(stub));
      else
        SymbolTableRemoveSymbol(OBJECT_SUB_SYMBOL_TABLE(obj), sym);
    }

    /* Kill all interprocedural (compensating) edges arriving on the exit block */
    if (exit_bbl)
    {
      BBL_FOREACH_PRED_EDGE_SAFE(exit_bbl, edge, tmp)
      {
        if (CfgEdgeIsBackwardInterproc(edge))
          CfgEdgeKill(edge);
      }
    }
  }

  /* Return the new object, so it can be deflowgraphed and assembled at a later moment */
  transform_index++;
}
