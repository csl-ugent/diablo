#include "meta_api.h"

using namespace std;

t_regset condition_registers;
t_regset cant_be_live_registers;

_MetaAPI_Operand::_MetaAPI_Operand(string s) {
  if (s == "+")
    value = Type::Add;
  else if (s == "-")
    value = Type::Sub;
  else
    FATAL(("unsupported operand %s", s.c_str()));
}

string _MetaAPI_Operand::Print() {
  return Print(value);
}

std::string _MetaAPI_Operand::Print(Type t) {
  string result;

  switch (t) {
  case Type::Add:
    result = "+"; break;
  case Type::Sub:
    result = "-"; break;
  default:
    FATAL(("invalid operand %d", t));
  }

  return result;
}

_MetaAPI_Relation::_MetaAPI_Relation(string s) {
  if (s == "==")
    value = Type::Eq;
  else if (s == "!=")
    value = Type::Ne;
  else if (s == "<")
    value = Type::Lt;
  else if (s == "<=")
    value = Type::Le;
  else if (s == ">")
    value = Type::Gt;
  else if (s == ">=")
    value = Type::Ge;
  else if (s == "%")
    value = Type::Mod;
  else if (s == "INSTANCEOF")
    value = Type::InstanceOf;
  else
    FATAL(("unsupported relation %s", s.c_str()));
}

string MetaAPI_Relation::Print() {
  return Print(value);
}

std::string MetaAPI_Relation::Print(Type t) {
  string result;

  switch (t) {
  case Type::Eq:
    result = "=="; break;
  case Type::Ne:
    result = "!="; break;
  case Type::Gt:
    result = ">"; break;
  case Type::Ge:
    result = ">="; break;
  case Type::Lt:
    result = "<"; break;
  case Type::Le:
    result = "<="; break;
  case Type::Mod:
    result = "%"; break;
  case Type::InstanceOf:
    result = "INSTANCEOF"; break;
  default:
    FATAL(("invalid relation %d", t));
  }

  return result;
}

MetaAPI_Relation::Type MetaAPI_Relation::Revert(MetaAPI_Relation::Type t) {
  Type result;

  switch (t) {
  case Type::Eq:
    result = Type::Eq; break;
  case Type::Ne:
    result = Type::Ne; break;
  case Type::Gt:
    result = Type::Le; break;
  case Type::Ge:
    result = Type::Lt; break;
  case Type::Lt:
    result = Type::Ge; break;
  case Type::Le:
    result = Type::Gt; break;
  default:
    FATAL(("invalid relation %d", t));
  }

  return result;
}

vector<t_ins*> FindPlacesWhereFlagsAreDead(t_bbl* bbl) {
  vector<t_ins*> result;

  t_ins* ins;
  BBL_FOREACH_INS_R(bbl, ins) {
    /* keep this instruction if the statusflags are dead here */
    if(RegsetIsEmpty(RegsetIntersect(condition_registers, InsRegsLiveBefore(ins)))
        && RegsetIsEmpty(RegsetIntersect(cant_be_live_registers, InsRegsLiveBefore(ins)))) {
      
      if ((ins == BBL_INS_FIRST(bbl)) && BblReferedToByExtab(bbl)) {
        /* don't add the first instruction if its refered to by an extab relocation */
      }
      else
        result.push_back(ins);
    }
  }

  return result;
}

static
t_ins* getInstructionWithDeadFlags(t_bbl* bbl) {
    vector<t_ins*> instructions = FindPlacesWhereFlagsAreDead(bbl);

    if(instructions.empty()) {
      ASSERT(!BblReferedToByExtab(bbl), ("extab @eiB", bbl));
      return BBL_INS_FIRST(bbl);
    }

    return PickRandomElement(instructions, meta_api_instruction_rng);
}

bool BblStatusFlagsAreDeadAfter(t_bbl *bbl) {
  return RegsetIsEmpty(RegsetIntersect(condition_registers, BblRegsLiveAfter(bbl)))
          && RegsetIsEmpty(RegsetIntersect(cant_be_live_registers, BblRegsLiveAfter(bbl)));
}

void MetaAPI_AfterBblSplit(t_bbl *from, t_bbl *to) {
  /* bookkeeping for cloud */
  if (BblIsInCloud(from))
    BblInsertInCloud(to);

  /* bookkeeping for predicate liveness */
  BblCopyPredicateLiveness(from, to);

  BBL_SET_SEQUENCE_ID(to, BBL_SEQUENCE_ID(from));
}

PreparedCallSite PrepareForCallInsertion(t_bbl *bbl, bool split_after, bool always_new_after) {
  PreparedCallSite result;
  // DEBUG(("prepare for call insertion @eiB", bbl));

  /* find the first instruction in the caller BBL at which the status flags are dead;
   * this will be the point at which the callee will be called */
  if (BBL_INS_FIRST(bbl)) {
    t_bbl *new_bbl;
    if (split_after) {
      ASSERT(!BblReferedToByExtab(bbl), ("from extab! @eiB", bbl));
      new_bbl = BblSplitBlock(bbl, BBL_INS_LAST(bbl), false);
    }
    else {
      t_ins *tmp = getInstructionWithDeadFlags(bbl);
      ASSERT(tmp, ("no instruction found where flags are dead in @eiB", bbl));

      /* split the caller BBL at the instruction after which we want to insert the call */
      if (tmp == BBL_INS_FIRST(bbl))
        ASSERT(!BblReferedToByExtab(bbl), ("from extab! @eiB", bbl));
      new_bbl = BblSplitBlock(bbl, tmp, TRUE);
    }

    MetaAPI_AfterBblSplit(bbl, new_bbl);

    result.before = bbl;
    result.after = new_bbl;
  }
  else {
    /* The caller BBL is empty */
    bool add_branch = false;

    /* check whether this caller BBL is one of the destinations of a switch statement,
     * or, whether this caller BBL is a possible successor of a switch statement.
     * If that is the case, special care should be taken for fallthrough paths associated
     * with switch statements. Here, we insert an unconditional branch instruction on the
     * fallthrough path, which jumps to the BBL from which we'll call the transformation function. */
    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e) {
      t_cfg_edge *e2;
      BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(e), e2)
        if (CfgEdgeIsSwitch(e2)) {
          add_branch = true;
          break;
        }

      if (add_branch)
        break;
    }

    result.before = bbl;

    e = BBL_SUCC_FIRST(bbl);
    if (!e) {
      CfgDrawFunctionGraphs(BBL_CFG(bbl), "error");
      FATAL(("no successor @eiB", bbl));
    }

    if (CfgEdgeIsInterproc(e)) {
      if (CFG_EDGE_CAT(e) != ET_IPFALLTHRU)
        CfgDrawFunctionGraphs(BBL_CFG(bbl), "ipfallthru");
      ASSERT(CFG_EDGE_CAT(e) == ET_IPFALLTHRU, ("expected ET_IPFALLTHU @E", e));
      ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
      result.after = BblSplitBlock(result.before, NULL, FALSE);
      MetaAPI_AfterBblSplit(result.before, result.after);
    }
    else {
      ASSERT(CFG_EDGE_CAT(e) == ET_FALLTHROUGH, ("expected ET_FALLTHROUGH @E", e));
      result.after = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
    }

    if (add_branch) {
      /* if necessary, we add an unconditional branch,
       * jumping to the BBL from which the function will be called */
      ASSERT(!BblReferedToByExtab(bbl), ("from extab! @eiB", bbl));
      t_bbl *new_bbl = BblSplitBlock(bbl, BBL_INS_FIRST(bbl), FALSE);
      MetaAPI_AfterBblSplit(bbl, new_bbl);

      t_ins *ins = CFG_DESCRIPTION(BBL_CFG(bbl))->BblAddJumpInstruction(bbl);
      InsAppendToBbl(ins, bbl);

      CfgEdgeKill(BBL_SUCC_FIRST(bbl));
      CfgEdgeCreate(BBL_CFG(bbl), bbl, new_bbl, ET_JUMP);

      result.before = new_bbl;
    }
  }

  if (!always_new_after
      && BBL_INS_FIRST(result.before)
      && BBL_NINS(result.after) == 0
      && BBL_NINS(CFG_EDGE_TAIL(BBL_SUCC_FIRST(result.after))) == 0) {
    result.before = result.after;
    result.after = CFG_EDGE_TAIL(BBL_SUCC_FIRST(result.before));
  }

  /* for our convenience, make sure that the pre and post blocks are empty */
  if (BBL_INS_FIRST(result.before)) {
    t_bbl *tmp = result.before;
    /* don't need to sanity check refed by extab as we split after the block */
    result.before = BblSplitBlock(result.before, BBL_INS_LAST(result.before), false);
    MetaAPI_AfterBblSplit(tmp, result.before);
  }

  if (BBL_INS_FIRST(result.after)) {
    ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
    result.after = BblSplitBlock(result.before, BBL_INS_LAST(result.before), false);
    MetaAPI_AfterBblSplit(result.before, result.after);
  }

  /* Split again if multiple edges incoming to first block,
   * as our current code only supports one edge going into a meta-API function (CFG_EDGE_METAAPI_CORR). */
  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(result.before))) {
    t_bbl *tmp = result.before;
    ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
    result.before = BblSplitBlock(result.before, BBL_INS_LAST(result.before), false);
    MetaAPI_AfterBblSplit(tmp, result.before);
  }

  /* other checks */
  t_cfg_edge *e;
  bool need_split_after = false;
  BBL_FOREACH_PRED_EDGE(result.before, e) {
    if (CFG_EDGE_CAT(e) == ET_RETURN) {
      /* split off another block as this is the return site of a call.
       * This is purely for aethetics in the dot graphs. */
      need_split_after = true;
      break;
    }

    if (CFG_EDGE_METAAPI_CORR_BACKWARD(e)) {
      need_split_after = true;
      break;
    }
  }
  if (need_split_after) {
    t_bbl *tmp = result.before;
    ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
    result.before = BblSplitBlock(result.before, BBL_INS_LAST(result.before), false);
    MetaAPI_AfterBblSplit(tmp, result.before);
  }

  /* second block can't have multiple incoming edges */
  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(result.after))) {
    // DEBUG(("SPLIT! @eiB", result.after));

    /* the 'before' block is empty */
    ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
    result.after = BblSplitBlock(result.before, NULL, false);
    MetaAPI_AfterBblSplit(result.before, result.after);

    // DEBUG(("@eiB", result.after));
  }

  need_split_after = false;
  BBL_FOREACH_SUCC_EDGE(result.after, e) {
    if (CFG_EDGE_METAAPI_CORR_FOREWARD(e)) {
      need_split_after = true;
      break;
    }
  }

  if (BblIsExitBlock(result.after))
    need_split_after = true;

  if (need_split_after) {
    ASSERT(!BblReferedToByExtab(result.before), ("from extab! @eiB", result.before));
    result.after = BblSplitBlock(result.before, BBL_INS_LAST(result.before), false);
    MetaAPI_AfterBblSplit(result.before, result.after);
  }

  return result;
}

/* */
FunctionSet MetaAPI_ImplementingFunctionList(t_cfg *cfg) {
  FunctionSet result;

  /* construct the call graph */
  CgBuild(cfg);

  CfgUnmarkAllFun(cfg);

  vector<t_function *> worklist;

  /* collect all the functions implementing some functionality needed by the meta-API */
  for (auto f : meta_api_functions)
    worklist.push_back(f.second->function);

  while (true)
  {
    /* get the last unmarked function in the work list */
    t_function *subject = nullptr;
    while (subject == nullptr || FunctionIsMarked(subject)) {
      if (worklist.size() == 0) {
        subject = nullptr;
        break;
      }

      subject = worklist.back();
      worklist.pop_back();
    }
    if (subject == nullptr)
      break;

    FunctionMark(subject);
    result.insert(subject);

    t_cg_edge *e;
    t_function *callee;
    FUNCTION_FOREACH_CALLEE(subject, e, callee)
    {
      if (FUNCTION_IS_HELL(callee)) {
        if (FUNCTION_NAME(callee)
            && StringPatternMatch("--DYNCALL-HELL--*", FUNCTION_NAME(callee))) {
          /* callee refers to PLT function.
           * we need to check whether the referee is also in this binary */
          string fun_name = string(FUNCTION_NAME(callee)).substr(strlen("--DYNCALL-HELL--"));

          t_function *fun = GetFunctionByName(cfg, fun_name.c_str());
          if (fun) {
            /* this function is implemented in the binary itself */
            worklist.push_back(fun);
          }
        }
      }
      else {
        /* regular function */
        worklist.push_back(callee);
      }
    }
  }

  VERBOSE(0, ("meta-API uses %d functions", result.size()));

  return result;
}

#define WHITESPACE " \t"
string trim(string str) {
  str.erase(0, str.find_first_not_of(WHITESPACE));
  str.erase(str.find_last_not_of(WHITESPACE) + 1);

  return str;
}

bool ends_with(string s, string e) {
  if (e.size() == 0)
    return true;

  size_t pos = s.rfind(e);

  /* not found */
  if (pos == string::npos)
    return false;

  /* found, but need to check position */
  return (pos == (s.size() - e.size()));
}

bool contains(string str, string x) {
  if (x.size() == 0)
    return true;

  return str.find(x) != string::npos;
}

bool FunctionIsMeta(t_function *fun) {
  return fun && (FUNCTION_METAAPI_DATA(fun) != NULL);
}

bool MetaAPI_CanTransformFunction(t_function *fun) {
  return implementing_functions.find(fun) == implementing_functions.end();
}

FunctionMetaApiData *FunctionGetMetaApiData(t_function *fun) {
  return FUNCTION_METAAPI_DATA(fun);
}

static
t_cfg_edge *LookThroughMetaFunctionRecursive(t_cfg_edge *e, bool forward, function<bool(t_function *)> stop) {
  t_cfg_edge *ee = e;

  /* special case: head nor tail is in meta-API function */
  if (!FunctionIsMeta(BBL_FUNCTION(CFG_EDGE_HEAD(ee)))
      && !FunctionIsMeta(BBL_FUNCTION(CFG_EDGE_TAIL(ee))))
    return ee;

  if (forward
      && !FunctionIsMeta(BBL_FUNCTION(CFG_EDGE_TAIL(ee))))
    return ee;

  if (!forward
      && !FunctionIsMeta(BBL_FUNCTION(CFG_EDGE_HEAD(ee))))
    return ee;

  ASSERT(CfgEdgeIsInterproc(ee), ("expected interproc edge @E", ee));
  // DEBUG(("look through meta function %d @E", forward, e));

  if (forward) {
    /* look forwards */
    t_function *callee = BBL_FUNCTION(CFG_EDGE_TAIL(e));
    if (stop(callee))
      return NULL;

    FunctionMetaApiData *d = FunctionGetMetaApiData(callee);
    ASSERT(d, ("no meta-API data for function @F", callee));
    ASSERT(!d->is_verifier, ("unexpected analysis of verifier @F", callee));

    auto check_edge = [] (t_cfg_edge *e) {
      t_cfg_edge *ee = CFG_EDGE_METAAPI_CORR_FOREWARD(e);
      ASSERT(ee, ("expected corresponding meta-API forward edge @E", e));

      if (CFG_EDGE_METAAPI_CORR_BACKWARD(ee))
        ASSERT(CFG_EDGE_METAAPI_CORR_BACKWARD(ee) == e, ("@E @E should be bidirectionally linked @E", e, ee, CFG_EDGE_METAAPI_CORR_BACKWARD(ee)));
      else
        ASSERT(CFG_EDGE_METAAPI_CORR_BACKWARD(ee) == e, ("@E @E should be bidirectionally linked (null)", e, ee));

      return ee;
    };

    ee = check_edge(e);
  }
  else {
    /* look backwards */
    t_function *caller = BBL_FUNCTION(CFG_EDGE_HEAD(e));
    if (stop(caller))
      return NULL;

    FunctionMetaApiData *d = FunctionGetMetaApiData(caller);
    ASSERT(d, ("no meta-API data for function @F", caller));

    auto check_edge = [] (t_cfg_edge *e) {
      if (CfgEdgeIsFake(e)) {
        /* debugging */
        t_cfg_edge *non_fake = NULL;

        t_cfg_edge *it;
        BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(e), it) {
          if (CfgEdgeIsFake(it)) {
            ASSERT(it == e, ("unexpected @E @E", it, e));
            continue;
          }

          ASSERT(!non_fake, ("multiple non-fake edges @eiB", CFG_EDGE_HEAD(e)));
          non_fake = it;
        }

        ASSERT(non_fake, ("can't find real edge for @E", e));
        e = non_fake;
      }

      t_cfg_edge *ee = CFG_EDGE_METAAPI_CORR_BACKWARD(e);
      ASSERT_WITH_DOTS((CFG_EDGE_CFG(e), "error"), ee, ("expected corresponding meta-API backward edge @E", e));

      if (CFG_EDGE_METAAPI_CORR_FOREWARD(ee))
        ASSERT(CFG_EDGE_METAAPI_CORR_FOREWARD(ee) == e, ("@E @E should be bidirectionally linked @E", e, ee, CFG_EDGE_METAAPI_CORR_FOREWARD(ee)));
      else
        ASSERT(CFG_EDGE_METAAPI_CORR_FOREWARD(ee) == e, ("@E @E should be bidirectionally linked (null)", e, ee));

      return ee;
    };

    if (d->is_verifier) {
      /* look through immediate predecessor */
      t_cfg_edge *eee = check_edge(e);
      t_bbl *head = CFG_EDGE_HEAD(eee);
      ASSERT(BBL_NINS(head) == 0, ("expected empty block, got @eiB", head));
      ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(head)) == NULL, ("expected only one incoming edge @eiB", head));

      e = BBL_PRED_FIRST(head);
    }

    ee = check_edge(e);
  }

  if (ee != e)
    return LookThroughMetaFunctionRecursive(ee, forward, stop);

  return e;
}

t_cfg_edge *LookThroughMetaFunction(t_cfg_edge *e, bool forward, function<bool(t_function *)> stop) {
  return LookThroughMetaFunctionRecursive(e, forward, stop);
}

void MetaAPI_LinkEdges(t_cfg_edge *a, t_cfg_edge *b){
  if (CFG_EDGE_METAAPI_CORR_FOREWARD(a)) {
    CfgDrawFunctionGraphs(CFG_EDGE_CFG(a), "corr-a");
    FATAL(("already corresponding edge a @E", a));
  }

  if (CFG_EDGE_METAAPI_CORR_BACKWARD(b)) {
    CfgDrawFunctionGraphs(CFG_EDGE_CFG(b), "corr-b");
    FATAL(("already corresponding edge b @E", b));
  }

  CFG_EDGE_SET_METAAPI_CORR_FOREWARD(a, b);
  CFG_EDGE_SET_METAAPI_CORR_BACKWARD(b, a);

  // DEBUG(("linking A=@E  B=@E", a, b));
}

/* find a suitable transformer, given the boundary conditions */
/* TODO: recode this function so multiple predicates can be set to a specific value, maybe? */
vector<MetaAPI_Setter> MetaAPI_FindTransformers(MetaAPI_Datatype *datatype, MetaAPI_Predicate *predicate, MetaAPI_Effect::Effect desired_value, map<MetaAPI_Predicate *, MetaAPI_Effect::Effect> dont_change) {
  vector<MetaAPI_Setter> result;
  VERBOSE(meta_api_verbosity, ("searching transformer for '%s'", datatype->Print().c_str()));
  VERBOSE(meta_api_verbosity, ("  %s -> %s", predicate->Print().c_str(), MetaAPI_Effect::EffectToString(desired_value).c_str()));

  /* look through the possible transformers... */
  for (auto transformer : datatype->transformers) {
    /* skip it if disabled */
    if (!transformer->enabled)
      continue;

    /* ... and through the possible effects for that transformer */
    size_t configuration = 0;
    for (auto setter_conf : transformer->setter_confs) {
      bool ok = true;

      if (setter_conf.effect->affected_predicates.find(predicate) == setter_conf.effect->affected_predicates.end()) {
        /* doesn't change the predicate value */
        ok = false;
      }
      else {
        for (auto affected : setter_conf.effect->affected_predicates) {
          MetaAPI_Predicate *affected_predicate = affected.first;
          MetaAPI_Effect::Effect affected_value = affected.second;

          if (affected_predicate == predicate) {
            /* the predicate we want to change should get the desired value */
            if (desired_value != affected_value) {
              ok = false;
              break;
            }
          }
          else {
            /* other predicates */
            if (dont_change.find(affected_predicate) != dont_change.end()) {
              /* this predicate should not be changed */
              MetaAPI_Effect::Effect fixed_value = dont_change[affected_predicate];

              if (((fixed_value != affected_value) || (fixed_value == MetaAPI_Effect::Effect::Unchanged))
                  && (affected_value != MetaAPI_Effect::Effect::Unchanged)) {
                /* the new value does not equal the predefined value */
                ok = false;
                break;
              }
            }
          }

          if (!ok)
            break;
        }
      }

      if (ok) {
        VERBOSE(meta_api_verbosity, ("found good transformer '%s': '%s'", transformer->Print().c_str(), setter_conf.Print().c_str()));
        result.push_back(MetaAPI_Setter{.function = transformer, .configuration = configuration});
      }

      configuration++;
    }
  }

  return result;
}

bool BblReferedToByExtab(t_bbl *bbl) {
  for (t_reloc_ref *rr = BBL_REFED_BY(bbl); rr; rr = RELOC_REF_NEXT(rr)) {
    t_reloc *rel = RELOC_REF_RELOC(rr);
    
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) != RT_SUBSECTION)
      continue;
    
    t_section *from = T_SECTION(RELOC_FROM(rel));
    if (!strcmp(SECTION_NAME(from), ".ARM.extab")) {
      DEBUG(("RELOC @R", rel));
      return true;
    }
  }

  return false;
}