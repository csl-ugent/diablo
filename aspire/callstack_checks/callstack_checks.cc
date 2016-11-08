/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <string>
#include <utility>
#include <set>
#include <vector>

#include "callstack_checks.h"
#include <code_mobility.h>
#include <diablosoftvm.h>

#define CALL_STACK_CHECK_DEBUG 0

#define ARM_REG_SP ARM_REG_R13
#define ARM_REG_LR ARM_REG_R14
#define ARM_REG_PC ARM_REG_R15

using namespace std;

LogFile* L_CALLCHECKS = nullptr;
static t_randomnumbergenerator* rng_csc = nullptr;

// TODO: merge with similar ARM opaque predicates to split at dead places
static bool AreFlagsDeadAfter(t_arm_ins* ins) {
  t_regset used=RegsetNew();

  RegsetSetAddReg(used,ARM_REG_C_CONDITION);
  RegsetSetAddReg(used,ARM_REG_V_CONDITION);
  RegsetSetAddReg(used,ARM_REG_Z_CONDITION);
  RegsetSetAddReg(used,ARM_REG_N_CONDITION);

  t_regset live = RegsetDup(InsRegsLiveAfter(T_INS(ins)));

  if(RegsetIsEmpty(RegsetIntersect(used,live))) {
    /* no flags are live, hooray */
    return true;
  }

  return false;
}

/*
 * We want to add relocations that refer to the .text-section (begin address, and end address = begin address + length. However,
 * at the point of the transformation:
 * 1. All relocations that originally refer to the beginning of the text section (like __text_begin), have been moved to BBLs, and thus will
 *    no longer point to the beginning of the section after layout randomization.
 * 2. The .text section in the produced binary is actually a *new* t_section. So even if we were to be able to get a reference to a section earlier,
 *    it'd be the wrong section anyway.
 * So we use a callback from ObjectDeflowgraph, and:
 * 1. We let all relocations point to a dummy section (this needs to be a DATA_SECTION because CODE_SECTIONS are already merged and killed before the callback
 * 2. In the callback we move all the relocations to the dummy section to the actual, final .text-section that was created by ObjectDeflowgraph.
 */

static t_section* dummy_section = nullptr;

t_section* GetDummySection() {
  ASSERT(dummy_section, ("Forgot to call InitializeDummySection!"));
  return dummy_section;
}

void InitializeDummySection(t_object* object) {
  ASSERT(!dummy_section, ("InitializeDummySection section already called!"));

  dummy_section = SectionCreateForObject(object, DATA_SECTION, NULL /* parent */, AddressNewForSection(sec,1), "dummy");
}

void FixupDummySectionRelocations(t_cfg* cfg) {
  if (!dummy_section) {
    VERBOSE(0, ("No dummy section set, no fixups needed"));
    return;
  }

  t_object* object = CFG_OBJECT(cfg);
  t_section* text_section = SectionGetFromObjectByName (object, ".text");

  VERBOSE(1, ("Dummy: @T", dummy_section));
  VERBOSE(1, ("Text: @T", text_section));

  ASSERT(text_section, ("Expected the final object to have a .text section by now")); // TODO: verify there is only one?

  t_reloc_ref* rr = SECTION_REFED_BY(dummy_section);
  while(rr)
  {
    t_reloc_ref* tmp_rr = RELOC_REF_NEXT(rr);
    t_reloc* rel = RELOC_REF_RELOC(rr);
    t_uint32 relocatable_index = RelocGetToRelocatableIndex(rel, T_RELOCATABLE(dummy_section));

    VERBOSE(1, ("Fixing up relocation @R", rel));

    RelocSetToRelocatable(rel, relocatable_index, T_RELOCATABLE(text_section));

    VERBOSE(1, ("Fixed up relocation @R", rel));

    rr = tmp_rr;
  }

  SectionKill(dummy_section);
  dummy_section = NULL;
}

/*
 * This function selects the possible points to insert the call stack check:
 * * To make it a little bit less conspicuous, it is inserted after the initial PUSH instruction (if it exists).
 *   => the check looks more like 'natural' code. This also frees up some more registers :)
 * * The insertion point is always *before* another instruction
 * * This can only be done *before* the link register is overwritten
 * * The flags must be dead
 *
 * Right now: at least ONE register must be free, it is a TODO to also allow registers to be spilled
 */

vector< pair<t_arm_ins*, t_reg> > GetInsertionPoints(t_bbl* to_check) {
  /* TODO use ARMArchitectureInfo; */
  t_regset possibleRegisters;
  t_reg reg;

  vector< pair<t_arm_ins*, t_reg> > insertionpoints;

  /* insert it after the initial PUSH */
  t_arm_ins* insert_after = T_ARM_INS(BBL_INS_FIRST(to_check));
  if (ARM_INS_OPCODE(insert_after) == ARM_STM || ARM_INS_OPCODE(insert_after) == ARM_STR) {
    VERBOSE(1, ("First instruction of @eiB is a PUSH, skipping after it", to_check));
    insert_after = ARM_INS_INEXT(insert_after);
  }

  /* TODO also allow spilling registers */
  /* Don't insert-after the last instruction of the BBL, that will be the control flow instruction */
  for ( ; insert_after && T_INS(insert_after) != BBL_INS_LAST(to_check); insert_after = ARM_INS_INEXT(insert_after) ) {
    /* Once LR is dead, stop */
    if (RegsetIn(ARM_INS_REGS_DEF(insert_after), ARM_REG_LR)) {
      break;
    }

    RegsetSetEmpty(possibleRegisters);

    for (reg = ARM_REG_R0; reg <= ARM_REG_R12; reg++) {
      RegsetSetAddReg(possibleRegisters, reg);
    }

    t_regset available = RegsetDiff(possibleRegisters, InsRegsLiveAfter(T_INS(insert_after)));
    if (RegsetIsEmpty(available))
      continue;

    if (!AreFlagsDeadAfter(insert_after))
      continue;

    t_reg check_reg;
    REGSET_FOREACH_REG(available, check_reg) {
      break; /* TODO randomize */
    }

    insertionpoints.push_back(make_pair(insert_after, check_reg));
  }

  return insertionpoints;
}

/* If there is only a single callee, we can insert a very simple, fine-grained check:
 * ADR reg, return_address
 * CMP LR, reg
 * BNE <...>
 */
void InsertFineGrainedCheck(t_bbl* to_check, t_bbl* ok_path, t_reg reg, t_bbl* correct_callee_lr_bbl) {
  t_arm_ins* ins;

  ArmMakeInsForBbl(Mov,  Append, ins, to_check, FALSE /* isThumb */, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_reloc* rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(to_check))),
                                                 0x0, /* addend */
                                                 T_RELOCATABLE(ins), /* from */
                                                 0x0, /* from-offset */
                                                 T_RELOCATABLE(correct_callee_lr_bbl), /* to */
                                                 0x0, /* to-offset */
                                                 FALSE, /* hell */
                                                 NULL, /* edge */
                                                 NULL, /* corresp */
                                                 NULL, /* sec */
                                                 "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  ArmMakeInsForBbl(Cmp, Append, ins, to_check, FALSE /* isThumb */, reg, ARM_REG_LR, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, to_check, FALSE /* isThumb */, ARM_CONDITION_NE /* ! */);

  t_bbl* false_path = SelectTargetFor(to_check, rng_csc, true /* need_to_fall_through */);

  CfgEdgeCreate(BBL_CFG(to_check), to_check, false_path, ET_JUMP);
  CfgEdgeCreate(BBL_CFG(to_check), to_check, ok_path, ET_FALLTHROUGH);
}

/* Compare the LR value against the very coarse range of begin-end of the .text section:
 * ADR reg, .text_start
 * CMP LR, reg
 * BLS <...> unsigned comparison! lower or same
 * ADR reg, .text_end TODO: maybe we can add a const here...
 * CMP LR, reg
 * BHI <...> unsigned comparison! higher
 */
void InsertRangeCheck(t_bbl* to_check, t_bbl* ok_path, t_reg reg) {
  t_arm_ins* ins;
  t_bbl* false_path = SelectTargetFor(to_check, rng_csc, true /* need_to_fall_through */); /* TODO + two different ones for first and second compare? */
  t_object* object = CFG_OBJECT(BBL_CFG(to_check));

  t_cfg_edge* edge;
  t_cfg_edge* edge_s;

  /* First check */
  ArmMakeInsForBbl(Mov,  Append, ins, to_check, FALSE /* isThumb */, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_reloc* rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(object),
                                                 0x0, /* addend */
                                                 T_RELOCATABLE(ins), /* from */
                                                 0x0, /* from-offset */
                                                 T_RELOCATABLE(GetDummySection()), /* to */
                                                 0x0, /* to-offset */
                                                 FALSE, /* hell */
                                                 NULL, /* edge */
                                                 NULL, /* corresp */
                                                 NULL, /* sec */
                                                 "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

  ArmMakeInsForBbl(Cmp, Append, ins, to_check, FALSE /* isThumb */, ARM_REG_LR, reg, 0, ARM_CONDITION_AL);
  ArmMakeInsForBbl(CondBranch, Append, ins, to_check, FALSE /* isThumb */, ARM_CONDITION_LS);

  /* Second check */
  ArmMakeInsForBbl(Mov,  Append, ins, to_check, FALSE /* isThumb */, reg, ARM_REG_NONE, 0, ARM_CONDITION_AL);

  rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(object),
                                                 0x0, /* addend */
                                                 T_RELOCATABLE(ins), /* from */
                                                 0x0, /* from-offset */
                                                 T_RELOCATABLE(GetDummySection()), /* to */
                                                 0x0, /* to-offset */
                                                 FALSE, /* hell */
                                                 NULL, /* edge */
                                                 NULL, /* corresp */
                                                 NULL, /* sec */
                                                 "R00Z00+" "\\" WRITE_32); /* base address + size of the section => end of section */

    ArmInsMakeAddressProducer(ins, 0/* immediate */, rel);

    t_bbl* to_check_2 = BblSplitBlock(to_check, T_INS(ins), TRUE /* before */);

    BBL_FOREACH_SUCC_EDGE_SAFE(to_check, edge, edge_s) {
      CfgEdgeKill(edge);
    }

    ArmMakeInsForBbl(Cmp, Append, ins, to_check_2, FALSE /* isThumb */, ARM_REG_LR, reg, 0, ARM_CONDITION_AL);
    ArmMakeInsForBbl(CondBranch, Append, ins, to_check_2, FALSE /* isThumb */, ARM_CONDITION_HI);

    CfgEdgeCreate(BBL_CFG(to_check), to_check, false_path, ET_JUMP);
    CfgEdgeCreate(BBL_CFG(to_check), to_check_2, false_path, ET_JUMP);

    CfgEdgeCreate(BBL_CFG(to_check), to_check, to_check_2, ET_FALLTHROUGH);
    CfgEdgeCreate(BBL_CFG(to_check), to_check_2, ok_path, ET_FALLTHROUGH);
}

bool InsertCheckInBblSinglePredecessor(t_bbl* to_check, t_bbl* correct_callee_lr_bbl, bool rangecheck) {
  VERBOSE(0, ("Inserting check in '%s', range check is: %s", FUNCTION_NAME(BBL_FUNCTION(to_check)), rangecheck ? "true" : "false"));

  auto insertionpoints = GetInsertionPoints(to_check);

  if (insertionpoints.size() == 0) {
    VERBOSE(0, ("No point found to insert a call check in @eiB, bailing!", to_check));
    return false;
  }

  t_function* fun = BBL_FUNCTION(to_check);

  START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_CALLCHECKS, "CallCheck,%x,%s", BBL_CADDRESS(to_check), FUNCTION_NAME(fun)) {
    AddTransformedBblToLog("CallCheck", to_check);
    LogFunctionTransformation("before", fun);
  }

  auto insert_here = insertionpoints.at(RNGGenerateWithRange(rng_csc, 0, insertionpoints.size() - 1));

  t_bbl* ok_path = BblSplitBlock(to_check, T_INS(insert_here.first), FALSE /* before */);

  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  BBL_FOREACH_SUCC_EDGE_SAFE(to_check, edge, edge_s) {
    CfgEdgeKill(edge);
  }

  t_reg reg = insert_here.second;

  if (rangecheck) {
    InsertRangeCheck(to_check, ok_path, reg);
  } else {
    InsertFineGrainedCheck(to_check, ok_path, reg, correct_callee_lr_bbl);
  }

  LOG_MORE(L_CALLCHECKS) { LogFunctionTransformation("after", fun); }
  STOP_LOGGING_TRANSFORMATION(L_CALLCHECKS);

  return true;
}

bool InsertCheckInFunction(t_function* fun) {
  t_bbl* entry = FUNCTION_BBL_FIRST(fun);

  VERBOSE(0, ("Call stack check for '%s'", FUNCTION_NAME(fun)));

  if (!entry)
    return false;

#if CALL_STACK_CHECK_DEBUG
  static int nr = 0;
#endif

  t_cfg_edge* edge;
  t_arm_ins* ins;

  int predecessors = 0;
  t_bbl* predecessor = nullptr;
  t_bbl* correct_callee_lr_bbl = nullptr;
  t_bool predecessor_is_hell = FALSE;

  if (FUNCTION_IS_HELL(fun) || BBL_IS_HELL(entry)) {
    VERBOSE(0, ("Not call-stack-checking hell"));
    return false;
  }

   BBL_FOREACH_PRED_EDGE(entry, edge) {
    t_bbl* head = CFG_EDGE_HEAD(edge);
    ins = T_ARM_INS(BBL_INS_LAST(head));

    
    if (BBL_IS_HELL(head)) {
#if 0
      if (strcmp(FUNCTION_NAME(fun),"fun_that_should_not_be_called")==0)
      {
        /* This case is for ASPIRE demonstration purposes only */
        predecessor_is_hell = TRUE;
        continue;
      }
      else
#endif
      {
        VERBOSE(0, ("Function was called from hell, no call stack checks possible!"));
        return false;
      }
    }
    
    if (!ins) {
      VERBOSE(0, ("No last ins for a BBL of @eiB (predecessor of @B), for inserting a call check, no call stack check inserted!", head, entry));
      return false;
    }

    if (ARM_INS_OPCODE(ins) != ARM_BL && ARM_INS_OPCODE(ins) != ARM_BLX) {
      VERBOSE(0, ("One of the predecessors of @B did not end in a call instruction: @eiB, so we can't really do a call check here, no call stack check inserted!", entry, head));
      return false;
    }

    /* TODO: check if the current edge is ET_CALL? */
    t_cfg_edge* corresponding = CFG_EDGE_CORR(edge);
    if (!corresponding) {
      VERBOSE(0, ("No corresponding edge found for the edge at the end of @eiB, so we can't really do a call check here, no call stack check inserted!", entry));
      return false;
    }

    Region* region = NULL;
    const CodeMobilityAnnotationInfo* info;
    BBL_FOREACH_CODEMOBILITY_REGION(head, region, info)
    {
      VERBOSE(0, ("Function is being called from a region that is slated to be made mobile. No call stack checks inserted!"));
      return false;
    }
#if 0
    const SoftVMAnnotationInfo* info2;
    BBL_FOREACH_SOFTVM_REGION(head, region, info2)
    {
      VERBOSE(0, ("Function is being called from a region that is slated to be made to softvm. No call stack checks inserted! @eiB", head));
      return false;
    }
#endif
    correct_callee_lr_bbl = CFG_EDGE_TAIL(corresponding);

    predecessors++;
    predecessor = head;
  }

  if (!BBL_INS_FIRST(entry)) {
    VERBOSE(0, ("BBL @eiB has no instructions, so we can't insert a call stack check here!", entry));
    return false;
  }

#if CALL_STACK_CHECK_DEBUG
  nr++;
  if (nr >= diablosupport_options.debugcounter)
    return false;

  VERBOSE(0, ("Inserting CALL CHECK in BBL @eiB", entry));
#endif

  if (predecessors == 1 && !predecessor_is_hell) {
    return InsertCheckInBblSinglePredecessor(entry, correct_callee_lr_bbl, false /* range check */);
  } else {
    return InsertCheckInBblSinglePredecessor(entry, correct_callee_lr_bbl, true /* range check */);
  }
}

void CallStackCheckAnnotationInfo::parseAnnotationContent(AnnotationRequests& result, const string& annotation_content) {
  size_t pos = 0;
  result.push_back(this);

  if (annotation_content[pos] != ',')
    return;

  /* After the comma follows a list of 'option1=int:option2=int:...' */
  string s = annotation_content.substr(pos + 1, annotation_content.find(')', pos));
  stringstream ss(s);
  AnnotationIntOptions options = ParseIntOptions(ss);

  auto depth = options.find("call_depth");
  if (depth == options.end())
    return;

  call_depth = depth->second;

  VERBOSE(1, ("Set call depth for annotation '%s' to %i ", annotation_content.c_str(), call_depth));
}

void ApplyCallStackChecks(t_cfg* cfg) {
  VERBOSE(0, ("Applying call stack checks"));

  /* Ensure liveness information is correct */
  CfgComputeLiveness(cfg, TRIVIAL);
  CfgComputeLiveness(cfg, CONTEXT_INSENSITIVE);
  CfgComputeLiveness(cfg, CONTEXT_SENSITIVE);

  /* Initialize the RNG */
  rng_csc = RNGCreateChild(RNGGetRootGenerator(), "call_stack_checks");

  InitializeDummySection(CFG_OBJECT(cfg));

  DiabloBrokerCallInstall("BeforeDeflowgraph", "t_cfg *", (void*)FixupDummySectionRelocations, FALSE);

  set<t_function*> functions_set;
  vector<pair<t_function*, CallStackCheckAnnotationInfo*>> functions;

  Region *region;
  CallStackCheckAnnotationInfo *info;

  CFG_FOREACH_CALLCHECK_REGION(cfg, region, info)
  {
    t_bbl *bbl;

    REGION_FOREACH_BBL(region, bbl)
    {
      if (IS_DATABBL(bbl))
        continue;

      t_function* fun = BBL_FUNCTION(bbl);
      if (functions_set.find(fun) == functions_set.end()) {
        functions_set.insert(fun);
        functions.push_back(make_pair(fun, info));
      }
    }
  }

  for (auto p: functions) {
    t_function* fun = p.first;
    if (InsertCheckInFunction(fun)) {
      p.second->successfully_applied = true;
    }
  }
  RNGDestroy(rng_csc);
}
