#include "meta_api.h"

using namespace std;

#define DEBUG_THIS

bool _debug_condition = false;
#ifdef DEBUG_THIS
#define _DEBUG(x) if (_debug_condition) DEBUG(x)
#else
#define _DEBUG(x)
#endif

/* 'cloud' functionality */
/* BBL flags are used so the middle-end can draw dot-graphs without broker calls,
 * but we also keep the 'marked' blocks in sets to be able to quickly reset them
 * (so we don't need to iterate over _all_ the basic blocks in the CFG every time). */
BblSet cloud; /* also used for liveness */
static BblSet setters;

static
void CloudClear() {
  for (auto bbl : cloud)
    BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) & ~BBL_AOP_CLOUD);
  cloud.clear();
}

void BblInsertInCloud(t_bbl *bbl) {
  BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_AOP_CLOUD);
  cloud.insert(bbl);
}

bool BblIsInCloud(t_bbl *bbl) {
  return (BBL_ATTRIB(bbl) & BBL_AOP_CLOUD) != 0;
}

void BblRemoveFromCloud(t_bbl *bbl) {
  BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) & ~BBL_AOP_CLOUD);
  cloud.erase(bbl);
}

set<MetaAPI_ActivePredicate *> LivePredicatesInCloud() {
  MetaAPI_LivePredicateBitset x;
  x.reset();
  for (auto bbl : cloud) {
    x |= BblLivePredicatesBefore(bbl);
  }

  set<MetaAPI_ActivePredicate *> result;
  MetaAPI_ForEachPredicateIn(&x, [&result] (MetaAPI_ActivePredicate *pred) {
    result.insert(pred);
  });
  return result;
}

static
void SettersClear() {
  for (auto bbl : setters)
    BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) & ~BBL_AOP_SETTER);
  setters.clear();
}

static
void BblInsertInSetters(t_bbl *bbl) {
  BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_AOP_SETTER);
  setters.insert(bbl);
}

/* helper */
static
bool IsPossibleLocation(BblLocation& loc) {
  bool result = true;
  loc.split_after = false;

  if (FindPlacesWhereFlagsAreDead(loc.bbl).size() > 0) {
    /* nothing needs to be done, BBL ok */
  }
  else if (BblStatusFlagsAreDeadAfter(loc.bbl)) {
    /* BBL needs to be split at the back */
    if (BBL_NINS(loc.bbl) > 0)
      loc.split_after = true;
  }
  else
    result = false;

  /* no support for switch-table related code yet */
  /* check whether this caller BBL is one of the destinations of a switch statement,
   * or, whether this caller BBL is a possible successor of a switch statement.
   * If that is the case, special care should be taken for fallthrough paths associated
   * with switch statements. Here, we insert an unconditional branch instruction on the
   * fallthrough path, which jumps to the BBL from which we'll call the transformation function. */
  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(loc.bbl, e) {
    t_cfg_edge *e2;
    BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(e), e2)
      if (CfgEdgeIsSwitch(e2))
        return false;
  }

  return result;
}

/* Try to find suitable locations to set the 'predicate' when it is evaluated in 'current_bbl'.
 * 'because_of_predicate' will be set to TRUE if
 * 'setter_locations' will contain the set of BBLs that must have a setter inserted.
 */
bool FindSetterLocations(t_bbl *current_bbl, MetaAPI_ActivePredicate *predicate, bool& because_of_predicate, vector<BblLocation>& setter_locations) {
  static int nr = -1; nr++;
  t_cfg *cfg = BBL_CFG(current_bbl);

  /* return values */
  because_of_predicate = false;
  setter_locations.clear();

  /* reset AOP flags, for debugging purposes (dot file generation) */
  CloudClear();
  SettersClear();

  /* no need to construct setter location list for invariant predicates */
  if (predicate /* can be NULL if we're only checking in 'canTransform' */
      && predicate->predicate->type == MetaAPI_Predicate::Type::Invariant)
    return true;

  _DEBUG(("constructing tree %d for getter @eiB", nr, current_bbl));
  // if (force_dots)
  //   DumpDots(cfg, "begin", nr);

  BblSet cloud;
  cloud.insert(current_bbl);

  BblSet farthest_setters;
  ASSERT(!FunctionIsMeta(BBL_FUNCTION(current_bbl)), ("can't be in meta function @eiB", current_bbl));
  farthest_setters.insert(current_bbl);
  BblSet all_blocks;

  /* marking final setter locations */
  BblMarkInit();

  auto check_edge = [predicate] (t_cfg_edge *e, bool& no_propagate, bool& incoming_hell, t_bbl *& head) {
    t_bbl *tail = CFG_EDGE_TAIL(e);
    t_function *head_function = BBL_FUNCTION(head);

    no_propagate = false;
    incoming_hell = false;

    if (CfgEdgeIsInterproc(e)
        && FunctionIsMeta(head_function)) {
      /* don't propagate into setter/getter functions that call this one */
      if (CFG_EDGE_CAT(e) == ET_CALL) {
        no_propagate = true;
        return;
      }

      /* don't try to propagate over instantiation functions */
      if (FUNCTION_NAME(head_function)
          && StringPatternMatch("Meta_Instance*", FUNCTION_NAME(head_function))) {
        no_propagate = true;
        return;
      }

      /* processing AOP function */
      t_cfg_edge *ee = LookThroughMetaFunction(e, false, [predicate] (t_function *f) {
        ASSERT(FunctionIsMeta(f), ("expected meta function @F", f));
        auto d = FunctionGetMetaApiData(f);

        if (!predicate)
          /* no predicate given */
          return false;

        if (d->is_getter) {
          /* doesn't change the value */
          return false;
        }

        if (d->is_verifier)
          d = FunctionGetMetaApiData(d->verified_setter_function);

        if (d->predicate->instance != predicate->instance) {
          /* the meta-function works on another datastructure instance */
          return false;
        }

        /* this is a setter function */
        auto affected_predicates = d->setter.function->setter_confs[d->setter.configuration].effect->affected_predicates;

        if (affected_predicates.find(predicate->predicate) == affected_predicates.end()) {
          /* the setter does not affect the current predicate */
          return false;
        }

        /* the current predicate is (possibly) overwritten by the setter */
        if (affected_predicates[predicate->predicate] == MetaAPI_Effect::Effect::Unchanged) {
          /* the value of the current predicate is unchanged by the setter */
          return false;
        }

        /* the setter negatively affects the current predicate */
        return true;
      });

      if (!ee) {
        no_propagate = true;
        return;
      }

      head = CFG_EDGE_HEAD(ee);
      _DEBUG(("look through meta function @eiB", head));
    }
    else {
      switch (CFG_EDGE_CAT(e)) {
      case ET_FALLTHROUGH:
      case ET_IPFALLTHRU:
      case ET_JUMP:
      case ET_IPJUMP:
      case ET_CALL:
      case ET_SWITCH:
        break;

      case ET_RETURN:
        no_propagate = true;
        break;

      default:
        FATAL(("unhandled edge type @E: @eiB @eiB", e, head, tail));
      }
    }

    if (BBL_IS_HELL(head)) {
      incoming_hell = true;
      return;
    }

    if ((predicate && BblIsPredicateLive(head, predicate))
        || !MetaAPI_CanTransformFunction(BBL_FUNCTION(head))) {
      no_propagate = true;
      return;
    }
  };

  bool restart = true;
  while (restart) {
    restart = false;

    for (auto far : farthest_setters) {
      /* this was a final block */
      if (BblIsMarked(far))
        continue;

      _DEBUG(("looking at predecessors for @eiB", far));
      t_cfg_edge *e;

      /* check all incoming paths */
      BblSet new_farthest;
      bool incoming_hell = false;
      bool no_propagate = false;

      BBL_FOREACH_PRED_EDGE(far, e) {
        t_bbl *head = CFG_EDGE_HEAD(e);
        t_bbl *tail = CFG_EDGE_TAIL(e);

        /* skip loop blocks */
        if (head == far)
          continue;
        
        /* long loops going back to the original bbl */
        if (head == current_bbl) {
          no_propagate = true;
          break;
        }

        bool _incoming_hell, _no_propagate;
        check_edge(e, _no_propagate, _incoming_hell, head);
        incoming_hell |= _incoming_hell;
        no_propagate |= _no_propagate;
        if (incoming_hell || no_propagate)
          break;

        if (cloud.find(head) == cloud.end()) {
          if (FunctionIsMeta(BBL_FUNCTION(head))) {
            CfgDrawFunctionGraphs(BBL_CFG(head), "meta");
            FATAL(("can't be in meta function @eiB", head));
          }

          new_farthest.insert(head);
          all_blocks.insert(head);
        }
      }

      if (incoming_hell || no_propagate) {
        /* not possible to propagate */
        _DEBUG(("impossible to propagate"));

        /* mark this BBL as final */
        BblMark(far);
      }
      else {
        /* possible to propagate */
        _DEBUG(("adding set"));

        /* move this BBL from the farthest setters set to the cloud */
        farthest_setters.erase(far);
        cloud.insert(far);

        /* insert the next level of possible setters */
        for (auto x : new_farthest) {
          ASSERT(!FunctionIsMeta(BBL_FUNCTION(x)), ("can't be in meta function @eiB", x));
          _DEBUG(("    +++ @B", x));
        }
        farthest_setters.insert(new_farthest.begin(), new_farthest.end());
        all_blocks.insert(new_farthest.begin(), new_farthest.end());

        /* need to restart the loop */
        restart = true;
        break;
      }
    }
  }

  for (auto x : farthest_setters)
    _DEBUG(("FAR SETTER @eiB", x));

  BblMarkInit();

  restart = true;
  while (restart) {
    restart = false;

    for (auto setter : farthest_setters) {
      BblLocation tmp = {.bbl = setter};
      if(!IsPossibleLocation(tmp)) {
        /* remove this BBL from the list of farthest setters */
        _DEBUG(("not a suitable location @eiB", setter));
        farthest_setters.erase(setter);

        /* remove the successors from the cloud */
        t_cfg_edge *e;
        BBL_FOREACH_SUCC_EDGE(setter, e) {
          t_cfg_edge *ee = LookThroughMetaFunction(e, true, [] (t_function *) {return false;});
          t_bbl *tail = CFG_EDGE_TAIL(ee);

          if (cloud.find(tail) != cloud.end()) {
            /* remove 'tail' from the cloud and add it to the setters */
            _DEBUG(("remove from cloud @eiB", tail));
            if (tail == current_bbl) {
              _DEBUG(("this is the start block, impossible to find setter locations"));
              return false;
            }
            cloud.erase(tail);
            ASSERT(!FunctionIsMeta(BBL_FUNCTION(tail)), ("can't be in meta function @eiB", tail));
            farthest_setters.insert(tail);
          }
        }

        restart = true;
      }

      if (restart)
        break;
    }
  }

  farthest_setters.erase(current_bbl);
  if (farthest_setters.size() == 0)
    return false;

  /* Another check for success:
   * every predecessor of each cloud bbl should be either another cloud block, or be marked as 'farthest setter' location */
  for (auto bbl : cloud) {
    bool preds_ok = true;

    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e) {
      t_bbl *head = CFG_EDGE_HEAD(e);
      bool _incoming_hell, _no_propagate;
      check_edge(e, _no_propagate, _incoming_hell, head);

      if ((farthest_setters.find(head) == farthest_setters.end())
          && (cloud.find(head) == cloud.end())) {
        /* not ok */
        preds_ok = false;
        break;
      }
    }

    if (!preds_ok)
      return false;
  }

  /* always include the function that will contain the getter */
  FunctionSet functions_to_draw;
  functions_to_draw.insert(BBL_FUNCTION(current_bbl));

  /* construct list of functions to be drawn */
  for (auto bbl : farthest_setters) {
    BblInsertInSetters(bbl);
    functions_to_draw.insert(BBL_FUNCTION(bbl));

    ASSERT(!FunctionIsMeta(BBL_FUNCTION(bbl)), ("can't be in meta function @eiB", bbl));

    /* insert the setter in the result vector */
    BblLocation tmp = BblLocation{.bbl = bbl};
    ASSERT(IsPossibleLocation(tmp), ("not a possible setter location @eiB", bbl));
    setter_locations.push_back(tmp);

    _DEBUG(("setter @eiB", bbl));
  }

  for (auto bbl : cloud) {
    BblInsertInCloud(bbl);
  }

  if (force_dots) {
    FunctionSet functions_to_draw;

    for (auto bbl : all_blocks) {
      BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | BBL_AOP_SELECT);
      functions_to_draw.insert(BBL_FUNCTION(bbl));
    }

    _DEBUG(("dumping to 'setters%d'", nr));
    DumpDotsF(functions_to_draw, "setters", nr);

    for (auto bbl : all_blocks)
      BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) & ~BBL_AOP_SELECT);
  }

  return true;
}
