#include "diabloanoptarm_advanced_factor.hpp"

//#define DEBUG_LIVEPROP

#ifdef DEBUG_LIVEPROP
#define DEBUG_LP(x) DEBUG(x)
#else
#define DEBUG_LP(x)
#endif

using namespace std;

/* defined in _state.cc */
extern RegisterAssumptionInBblMap assumptions;

static inline
t_regset CachedBblRegsLiveBefore(t_bbl *bbl) {
  t_regset result;

  if (BblIsMarked2(bbl)) {
    /* valid cache */
    result = BBL_CACHED_REGS_LIVE_BEFORE(bbl);
  }
  else {
    /* invalid cache */
    if (BBL_IS_HELL(bbl))
      result = BblComputeLiveInFromLiveOut(bbl, false, false);
    else
      result = BblRegsLiveBefore(bbl);

    BBL_SET_CACHED_REGS_LIVE_BEFORE(bbl, result);

    BblMark2(bbl);
  }

  return result;
}

static
BblVector CalculateStartPoints(t_bbl *start, t_reg reg) {
  BblMarkInit();

  BblVector result;

  DEBUG_LP(("assumption in @eiB for r%d", start, reg));

  /* assuming that 'reg' is live-in */
  BblVector worklist;

  auto Push = [&worklist] (t_bbl *bbl) {
    if (BblIsMarked(bbl))
      return;

    BblMark(bbl);
    worklist.push_back(bbl);
  };

  auto Pop = [&worklist] () {
    t_bbl *result = worklist.back();
    worklist.pop_back();

    return result;
  };

  auto Mark = [&result] (t_bbl *bbl) {
    result.push_back(bbl);
  };

  auto Process = [reg, &result, Push, Mark] (t_bbl *bbl, bool look_at_def) {
    if (look_at_def) {
      if (!RegsetIn(BBL_REGS_LIVE_OUT(bbl), reg))
        Mark(bbl);

      if (RegsetIn(BblRegsMaybeDef(bbl), reg))
        return;
    }

    ASSERT(!BBL_IS_HELL(bbl), ("hell no @eiB", bbl));

    DEBUG_LP(("processing @eiB", bbl));

    auto ProcessEdge = [bbl, Push, reg] (t_cfg_edge *e) {
      if (CfgEdgeIsAF(e)) {
        TransformedSliceInformation *slice_information = CFG_EDGE_SLICE_INFORMATION(CFG_EDGE_AF_CORR(e));
        t_gpregisters overwritten_registers = slice_information->overwritten_registers;

        /* this slice overwrites the register */
        if (GPRegistersIn(overwritten_registers, reg))
          return;

        Push(CFG_EDGE_HEAD(CFG_EDGE_AF_CORR(e)));
      }
      else {
        if (CFG_EDGE_CAT(e) == ET_COMPENSATING)
          return;

        if (CFG_EDGE_CAT(e) == ET_RETURN) {
          t_function *callee = BBL_FUNCTION(CFG_EDGE_HEAD(e));

          if (FUNCTION_CALL_HELL_TYPE(callee)) {
            /* call hell */
            if (!RegsetIn(CFG_DESCRIPTION(BBL_CFG(bbl))->callee_may_change, reg))
              Push(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)));
          }
          else {
            if (RegsetIn(FUNCTION_REGS_SAVED(callee), reg)) {
              /* skip callee */
              Push(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)));
            }
            else {
              /* add callee */
              Push(CFG_EDGE_HEAD(e));
            }
          }
        }
        else
          Push(CFG_EDGE_HEAD(e));
      }
    };

    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e)
      ProcessEdge(e);
  };

  Process(start, false);

  while (worklist.size() > 0) {
    t_bbl *bbl = Pop();

    Process(bbl, true);
  }

  return result;
}

static
void DoRegInBbl(t_bbl *bbl, t_reg reg) {
  BblVector bbls = CalculateStartPoints(bbl, reg);
  for (auto x : bbls) {
    if (RegsetIn(BBL_REGS_LIVE_OUT(x), reg))
      continue;

    t_regset new_live_out = BBL_REGS_LIVE_OUT(x);
    RegsetSetAddReg(new_live_out, reg);
    BBL_SET_REGS_LIVE_OUT(x, new_live_out);

    PropagateLiveRegister(x, NULL, reg);
  }
}

void LivenessForAssumptions(t_cfg *cfg) {
  for (auto pair1 : assumptions) {
    t_bbl *bbl = pair1.first;

    for (auto pair2 : pair1.second) {
      t_reg reg = pair2.first;

      DoRegInBbl(bbl, reg);
    }
  }
}

void PropagateLivenessActions(t_bbl *bbl) {
  t_regset regs = NullRegs;

  t_arm_ins *ins;
  BBL_FOREACH_ARM_INS_R(bbl, ins) {
    RegsetSetDiff(regs, ARM_INS_REGS_DEF(ins));
    RegsetSetUnion(regs, ARM_INS_REGS_USE(ins));
  }

  t_reg reg;
  REGSET_FOREACH_REG(regs, reg)
    DoRegInBbl(bbl, reg);
}

/* propagate a live register backward */
bool PropagateLiveRegister(t_bbl *bbl, t_arm_ins *ins, t_reg reg) {
  bool result = false;
  bool need_to_propagate = true;

  t_bbl *start_bbl = bbl;

  if (BBL_FUNCTION(start_bbl) == CFG_WRAP_FUNCTION(BBL_CFG(start_bbl)))
    return result;
  if (BBL_IS_HELL(start_bbl))
    return result;

  DEBUG_LP(("propagating live register r%d in @eiB", reg, bbl));

  if (ins) {
    start_bbl = ARM_INS_BBL(ins);

    t_arm_ins *tmp = ARM_INS_IPREV(ins);
    while (tmp) {
      if (reg != ARM_REG_NONE
          && RegsetIn(ARM_INS_REGS_DEF(tmp), reg)) {
        need_to_propagate = false;
        break;
      }

      tmp = ARM_INS_IPREV(tmp);
    }
    DEBUG_LP(("  from @I in @eiB", ins, start_bbl));
  }

  t_cfg *cfg = BBL_CFG(bbl);

  /* early exit if the register is defined INSIDE the BBL */
  if (!need_to_propagate)
    return result;

  /* if we get here, we need to propagate to the predecessors */
  BblMarkInit();

  auto propagate_liveness_to_predecessor = [start_bbl, reg] (t_bbl *subject, t_cfg_edge *edge) {
    /* 'edge' is successor edge from 'subject' */
    t_regset result;
    t_bbl *tail;

    /* special case for AF edges */
    if (CfgEdgeIsAF(edge)) {
      tail = EdgeTail(edge);
      result = AFFunctionLiveBefore(tail);
    }
    else {
      tail = CFG_EDGE_TAIL(edge);

      if (tail == subject)
        /* take special care when the target is the subject itself,
         * in which case the non-cached value should be used */
        result = BblRegsLiveBefore(tail);

      else {
        switch (CFG_EDGE_CAT(edge)) {
        case ET_CALL: {
          t_function *callee = BBL_FUNCTION(tail);

          /* return site information, if any */
          t_regset return_from_call_site = RegsetNew();
          if (CFG_EDGE_CORR(edge)) {
            t_bbl *return_site = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));

            return_from_call_site = CachedBblRegsLiveBefore(return_site);
            RegsetSetDiff(return_from_call_site, FUNCTION_REGS_CHANGED(callee));
          }

          t_regset used = CachedBblRegsLiveBefore(tail);
          RegsetSetDiff(used, FUNCTION_REGS_SAVED(callee));

          result = RegsetUnion(return_from_call_site, used);
        }
          break;

        case ET_RETURN:
        case ET_JUMP:
        case ET_IPJUMP:
        case ET_FALLTHROUGH:
        case ET_IPFALLTHRU:
        case ET_SWITCH:
        case ET_IPSWITCH:
          result = BblRegsLiveBefore(tail);
          break;

        case ET_COMPENSATING:
          if (FUNCTION_IS_AF(BBL_FUNCTION(subject)))
            result = RegsetNew();
          else
            result = BblRegsLiveBefore(tail);
          break;

        default:
          CfgDrawFunctionGraphs(BBL_CFG(subject), "edge");
          FATAL(("unhandled edge @E for @eiB", edge, subject));
        }
      }
    }

    if (tail == start_bbl
        && reg != ARM_REG_NONE)
      RegsetSetAddReg(result, reg);

    return result;
  };

  auto update_live_out_marked = [&propagate_liveness_to_predecessor] (t_bbl *subject) {
    t_regset result = RegsetDup(BBL_REGS_LIVE_OUT(subject));

    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(subject, e) {
      if (CfgEdgeIsMarked(e))
        RegsetSetUnion(result, propagate_liveness_to_predecessor(subject, e));
    }

    return result;
  };

  auto update_live_out = [] (t_bbl *subject, t_regset new_regs) {
    auto x = RegsetCountRegs(new_regs);
    auto y = RegsetCountRegs(BBL_REGS_LIVE_OUT(subject));

    ASSERT(x >= y, ("less registers @eiB @X", subject, CPREGSET(BBL_CFG(subject), new_regs)));

    if (x > y) {
      BblUnmark2(subject);
      BBL_SET_REGS_LIVE_OUT(subject, new_regs);
      return true;
    }

    return false;
  };

  CfgEdgeSet marked_edges;
  BblVector worklist;

  auto do_return_edge = [reg] (t_cfg_edge *edge) {
    t_function *head_function = BBL_FUNCTION(CFG_EDGE_HEAD(edge));
    bool result = reg != ARM_REG_NONE && (RegsetIn(FUNCTION_REGS_SAVED(head_function), reg) || !RegsetIn(FUNCTION_REGS_CHANGED(head_function), reg));

    if (result) {
      t_cfg *cfg = BBL_CFG(CFG_EDGE_HEAD(edge));
      DEBUG_LP(("need to propagate to incoming! @X @X", CPREGSET(cfg, FUNCTION_REGS_SAVED(head_function)), CPREGSET(cfg, FUNCTION_REGS_CHANGED(head_function))));
    }

    return result;
  };

  /* initial worklist construction */
  BblSet todo;
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(start_bbl, e) {
    CfgEdgeMark(e);
    marked_edges.insert(e);

    if (CFG_EDGE_CAT(e) == ET_RETURN && do_return_edge(e)) {
      todo.insert(CFG_EDGE_HEAD(CFG_EDGE_CORR(e)));

      CfgEdgeMark(CFG_EDGE_CORR(e));
      marked_edges.insert(CFG_EDGE_CORR(e));
    }

    for (auto x : EdgeHead(e))
      todo.insert(x);
  }

  /* only add basic blocks that have a changed regs live out set */
  for (auto x : todo) {
    if (BBL_FUNCTION(x) == CFG_WRAP_FUNCTION(cfg))
      continue;
    if (BBL_IS_HELL(x))
      continue;

    t_regset new_live_out = update_live_out_marked(x);
    if (reg != ARM_REG_NONE)
      RegsetSetAddReg(new_live_out, reg);

    if (update_live_out(x, new_live_out)) {
      DEBUG_LP(("entry change @eiB (new @X)", x, CPREGSET(cfg, new_live_out)));
      BblUnmark(x);
      worklist.push_back(x);
    }
  }

  for (auto x : marked_edges)
    CfgEdgeUnmark(x);
  marked_edges.clear();

  while (worklist.size() > 0) {
    /* get a new block from the queue */
    t_bbl *subject = worklist.back();
    worklist.pop_back();

    if (BblIsMarked(subject))
      continue;

    /* we got a block. This means that the LIVE_OUT regset has changed
     * and thus that this information needs to be propagated */
    ASSERT(!BBL_IS_HELL(subject), ("doing HELL @iB", subject));

    /* mark this block: we have caluclated the live-out set */
    BblMark(subject);
    DEBUG_LP(("doing @eiB", subject));

    /* mark the processed edges and look at the predecessors */
    BblSet preds;

    t_cfg_edge *edge;
    BBL_FOREACH_PRED_EDGE(subject, edge) {
      CfgEdgeMark(edge);
      marked_edges.insert(edge);

      if (CFG_EDGE_CAT(edge) == ET_RETURN && do_return_edge(edge)) {
        preds.insert(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));

        CfgEdgeMark(CFG_EDGE_CORR(edge));
        marked_edges.insert(CFG_EDGE_CORR(edge));
      }

      auto heads = EdgeHead(edge);
      preds.insert(heads.begin(), heads.end());
    }

    /* iterate over the predecessors */
    for (auto predecessor : preds) {
      if (BBL_FUNCTION(predecessor) == CFG_WRAP_FUNCTION(cfg))
        continue;
      if (BBL_IS_HELL(predecessor))
        continue;

      t_regset old_live_out = BBL_REGS_LIVE_OUT(predecessor);
      t_regset new_live_out = update_live_out_marked(predecessor);

      if (update_live_out(predecessor, new_live_out)) {
        DEBUG_LP(("change @eiB (was @X)", predecessor, CPREGSET(cfg, old_live_out)));
        BblUnmark(predecessor);
        worklist.push_back(predecessor);
      }
    }

    for (auto x : marked_edges)
      CfgEdgeUnmark(x);
    marked_edges.clear();
  }

  return result;
}
