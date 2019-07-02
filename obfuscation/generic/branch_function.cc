/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */
#include <obfuscation/obfuscation_architecture_backend.h>
#include "branch_function.h"
#include "branch_function_opt.h"
using namespace std;

LogFile* L_OBF_BF = NULL;

/* TODO: factor out into a Policy */

static bool CanTransformOutgoingEdges(const t_bbl* bbl) {
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    t_function* fun = BBL_FUNCTION(CFG_EDGE_TAIL(edge));
    if (!fun)
      continue;

    if (DisallowedFunctionToTransform(fun))
      return false;
  }
  /* Problem: if we COME from a get_pc_thunk, then that pc-register is possibly used here. See,
   * for example, _fini in libm: call get_pc_thunk_ebx ; add const, ebx ; call _do_global_dtors_aux
   * However, in principle, the same could happen with our own code...
   * So for now HACK WORKAROUND: disallow transforms also when the *predecessor* is a get_pc_thunk.
   * Obviously, this is *very* fragile. A better workaround would be a combination of
   * liveness of the pc-containing register, in combination with scanning relocations (TODO) */
  BBL_FOREACH_PRED_EDGE(bbl, edge) {
    t_function* fun = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
    if (!fun)
      continue;

    if (DisallowedFunctionToTransform(fun))
      return false;
  }
  return true;
}

BranchFunctionTransformation::BranchFunctionTransformation() {
  AddOptionsListInitializer(obfuscation_branch_function_option_list); BranchFunctionOptInit();
  
  RegisterTransformationType(this, _name);
}

bool BranchFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_branch_function_options.branch_function && !AllObfuscationsEnabled())
    return false;
  
  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }
  
  t_function* fun = BBL_FUNCTION(bbl);
  if (!fun || FUNCTION_IS_HELL(fun) || !BBL_INS_LAST(bbl))
    return false;
  
  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (CFG_EDGE_CAT(edge) == ET_SWITCH
        || CFG_EDGE_CAT(edge) == ET_IPSWITCH)
      return false;
  }
  
  if (!CanTransformOutgoingEdges(bbl))
    return false;

  // TODO
  /*MEMBER(t_reloc_ref *, refers_to, REFERS_TO)
MEMBER(t_reloc_ref *, refed_by, REFED_BY)
MEMBER(t_symbol_ref *, refed_by_sym, REFED_BY_SYM)
*/

  return BBL_NINS(bbl) > 1;
}

CallFunctionTransformation::CallFunctionTransformation() {
  AddOptionsListInitializer(obfuscation_branch_function_option_list); BranchFunctionOptInit(); /* IEW TODO */

  RegisterTransformationType(this, _name);

  callsTransformed = 0;
}

bool CallFunctionTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_branch_function_options.call_function && !AllObfuscationsEnabled())
    return false;

  if (!bbl)
    return false;

  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }

  t_function* fun = BBL_FUNCTION(bbl);
  if (!fun || BBL_IS_HELL(bbl) || FUNCTION_IS_HELL(fun) || !BBL_INS_LAST(bbl))
    return false;

  t_cfg_edge* edge;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if (CFG_EDGE_CAT(edge) == ET_SWITCH
        || CFG_EDGE_CAT(edge) == ET_IPSWITCH)
      return false;
  }

  if (!CanTransformOutgoingEdges(bbl))
    return false;

  t_cfg_edge* call_edge = nullptr;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      if (CFG_EDGE_CAT(edge) == ET_CALL) {
        call_edge = edge;
      }
    }

  if (!call_edge)
    return false;

  return true;
}

void CallFunctionTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sCallFunction_Stats,calls_transformed,%i", prefix.c_str(), callsTransformed));
}

SplitCallWithBranchFunctionsTransformation::SplitCallWithBranchFunctionsTransformation() {
  AddOptionsListInitializer(obfuscation_branch_function_option_list); BranchFunctionOptInit(); /* IEW TODO */
  
  RegisterTransformationType(this, _name);

  callsTransformed = 0;
}

bool SplitCallWithBranchFunctionsTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_branch_function_options.split_off_calls && !AllObfuscationsEnabled())
    return false;

  if (bbl &&
      BBL_INS_LAST(bbl)
      /* && (I386_INS_OPCODE(T_I386_INS(BBL_INS_LAST(bbl)))==I386_CALL) TODO ?? */) {
    /* Don't transform calls to silly functions */
    t_cfg_edge* edge;
    t_cfg_edge* call_edge = NULL;
    t_function* callee = NULL;

    if (!CanTransformOutgoingEdges(bbl))
      return false;

    BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      if (CFG_EDGE_CAT(edge) == ET_CALL) {
        call_edge = edge;
      }
    }

    if (!call_edge) {
      return false;
    }

    if (!BBL_FUNCTION(CFG_EDGE_TAIL(call_edge))) {
      return false;
    }

    callee = BBL_FUNCTION(CFG_EDGE_TAIL(call_edge));

    if (!callee)
      return false;

    if (FUNCTION_IS_HELL(callee))
      return false;

    return true;
  }
  
  return false;
}


void SplitCallWithBranchFunctionsTransformation::addBranchFunctionCallToBbl(t_bbl* bbl, t_randomnumbergenerator* rng) {
  GetRandomTypedTransformationForType<BranchFunctionTransformation>("branchfunction", rng)->transformJumpToCall(bbl, rng);
}

/*
 Based on DiversityBranchFunctionBeforeAndAfterCalls. This transform changes
 BB1
 call X
 BB2

 into

 BB1
 Branch Function to BB1'
 BB1' := call X
 Branch Function to BB2
 BB2

 (Other jumps with as target BB2 should remain pointing to BB2)

 That is: the call to X is now in a seperate chain from BB1 and BB2 and can now be randomly relayouted.
 
 If we would just create a branch function at the entry of BBL2, ALL edges coming there would be disadvantaged...
 Thus, we make a new basic block whose sole responsability is to jump to basic block 2.
 
 Note that we split basic blocks to end in an unconditional jump. Then, we use the existing branch function transformations
 to replace this unconditional jump with a call to the branch function.
 
 If SPLIT_AFTER is defined, we split both before and after the call, otherwise only before.
 TODO: make this an option
*/

bool SplitCallWithBranchFunctionsTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  VERBOSE(1, ("Splitting off call in @eiB", bbl));

  ASSERT(canTransform(bbl), ("Expected a BBL that can be transformed @eiB", bbl));

  t_bbl* branch_bbl_1;
  t_bbl* bbl_2 = NULL;
  t_bbl* branch_bbl_2;
  t_cfg* cfg = FUNCTION_CFG(BBL_FUNCTION(bbl));
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;
  t_bbl* exit_bbl;
  t_cfg_edge* call_edge = NULL;
  t_bbl* callee;

  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    if (CFG_EDGE_CAT(edge) == ET_CALL) {
      call_edge = edge;
    }
  }

  if (!call_edge) {
    /* This can happen in, for example, exit(), it seems? */
    VERBOSE(0, ("No call edge for bbl!!!"));
    return false;
  }
  
  if (!CFG_EDGE_CORR(call_edge)) {
        VERBOSE(0, ("Only transforming call edges that actually have a fall-through path..."));
        return false;
  }
  
  bbl_2 = CFG_EDGE_TAIL(CFG_EDGE_CORR(call_edge));

  callee = CFG_EDGE_TAIL(call_edge);
  VERBOSE(1, ("ObfuscatingCall to: '%s'", BBL_FUNCTION(callee) ? FUNCTION_NAME(BBL_FUNCTION(callee)) : "Name unknown"));

//#define SPLIT_AFTER
#ifdef SPLIT_AFTER
  /* Make the basic block containing the jump to BBL2. This means that the call will have to RETURN to this newly made BBL */
  branch_bbl_2 = BblNew(cfg);
  if (BBL_FUNCTION(bbl_2))
          BblInsertInFunction(branch_bbl_2, BBL_FUNCTION(bbl_2));

  t_regset live = BblRegsLiveBefore(bbl_2);
  
  GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(branch_bbl_2);
  CfgEdgeCreate(cfg, branch_bbl_2, bbl_2, ET_JUMP);
  
  BBL_SET_REGS_LIVE_OUT(branch_bbl_2, live);
  
  /* Update return edge of the function to this new basic block */
  exit_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(call_edge));

  CfgEdgeKill(CFG_EDGE_CORR(call_edge));
  CfgEdgeKill(call_edge);
  CfgEdgeCreateCall(cfg, bbl, callee, branch_bbl_2, exit_bbl);

  /* Now split the first basic block just before the call */
  branch_bbl_1 = GetArchitectureInfo(bbl)->splitBasicBlockWithJump(bbl, BBL_INS_LAST(bbl), TRUE);

  /* Add the two branch function calls, and we're done! */
  addBranchFunctionCallToBbl(bbl, rng);
  addBranchFunctionCallToBbl(branch_bbl_2, rng);
#else
  /* Now split the first basic block just before the call */
  branch_bbl_1 = GetArchitectureInfo(bbl)->splitBasicBlockWithJump(bbl, BBL_INS_LAST(bbl), TRUE);
  /* Add a single branch function */
  addBranchFunctionCallToBbl(bbl, rng);
#endif
  
  callsTransformed++;
  
  return true;
}

void SplitCallWithBranchFunctionsTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sSplitCall_Stats,calls_transformed,%i", prefix.c_str(), callsTransformed));
}

SplitOffFunctionHeadTransformation::SplitOffFunctionHeadTransformation() {
  AddOptionsListInitializer(obfuscation_branch_function_option_list); BranchFunctionOptInit(); // TODO duplication ...
  
  RegisterTransformationType(this, _name);
  
  functionstransformed = 0;
}
 
bool SplitOffFunctionHeadTransformation::canTransform(const t_function* fun) const {
  if (!obfuscation_branch_function_options.split_off_function_head && !AllObfuscationsEnabled())
    return false;

  if (!fun)
    return false;
  
  auto bbl = FUNCTION_BBL_FIRST(fun);
  
  if (!bbl)
    return false;
  
  if (!BBL_INS_LAST(bbl))
    return false;
  
  std::vector<BranchFunctionTransformation*> obfuscators = GetAllTypedTransformationsForType<BranchFunctionTransformation>("branchfunction");

  for (auto obfuscator : obfuscators)
  {
    /* It is possible that the first BBL is hard to split, in particular if there is some PC/LR-dependent
     * behaviour going on. That is detected automatically by the canTransform... */
    if (obfuscator->canTransform(bbl))
      return true;
  }
  
  return false;
}

bool SplitOffFunctionHeadTransformation::doTransform(t_function* fun, t_randomnumbergenerator * rng) {
  auto bbl = FUNCTION_BBL_FIRST(fun);
  BranchFunctionTransformation* obfuscator = nullptr;

  while (true)
  {
    obfuscator = GetRandomTypedTransformationForType<BranchFunctionTransformation>("branchfunction", rng);
    if (obfuscator->canTransform(bbl))
      break;
  }

  /* Why we don't split at random: if we'd split too early, chances are pattern matching might identify the split-off BBL, and then we gain nothing */
  auto split_off = GetArchitectureInfo(bbl)->splitBasicBlockWithJump(bbl, BBL_INS_LAST(bbl), true /* before */);
  
  obfuscator->transformJumpToCall(bbl, rng);
  
  functionstransformed++;
  
  return true;
}

void SplitOffFunctionHeadTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sSplitFirstBBL_Stats,calls_transformed,%i", prefix.c_str(), functionstransformed));
}
