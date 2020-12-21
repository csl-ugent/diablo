#include "meta_api.h"

using namespace std;

static t_uint32 max_uid = 0;

static MetaAPI_ActivePredicateSet all_predicates;
static MetaAPI_LivePredicateBitset all_predicates_set;

extern BblSet cloud;

void MetaAPI_ForEachPredicateIn(MetaAPI_LivePredicateBitset *predicate_set, function<void(MetaAPI_ActivePredicate *)> helper) {
  for (t_uint32 i = 0; i <= max_uid; i++) {
    if (predicate_set->test(i))
      helper(MetaAPI_GetPredicateByUID(i));
  }
}

static
void BblAddLivePredicate(t_bbl *bbl, MetaAPI_ActivePredicate *pred) {
  if (!BBL_LIVE_PREDICATES(bbl))
    BBL_SET_LIVE_PREDICATES(bbl, new MetaAPI_LivePredicateBitset());

  BBL_LIVE_PREDICATES(bbl)->set(pred->uid);
}

void BblCopyPredicateLiveness(t_bbl *from, t_bbl *to) {
  if (!BBL_LIVE_PREDICATES(from))
    return;

  ASSERT(!BBL_LIVE_PREDICATES(to), ("already live predicates in @eiB", to));

  MetaAPI_ForEachPredicateIn(BBL_LIVE_PREDICATES(from), [to] (MetaAPI_ActivePredicate *p) {
    BblAddLivePredicate(to, p);
  });
}

bool BblIsPredicateLive(t_bbl *bbl, MetaAPI_ActivePredicate *pred) {
  if (!BBL_LIVE_PREDICATES(bbl))
    return false;

  return BBL_LIVE_PREDICATES(bbl)->test(pred->uid);
}

string MetaAPI_ActivePredicate::Print() {
  return "P[" + to_string(uid) + "] " + predicate->Print() + "@" + instance->datatype->Print();
}

MetaAPI_LivePredicateBitset BblLivePredicatesBefore(t_bbl *bbl) {
  MetaAPI_LivePredicateBitset result;

  if (BBL_LIVE_PREDICATES(bbl))
    result |= *BBL_LIVE_PREDICATES(bbl);
  if (BBL_USED_PREDICATES(bbl))
    result |= *BBL_USED_PREDICATES(bbl);

  result &= all_predicates_set;

  return result;
}

MetaAPI_LivePredicateBitset BblLivePredicatesAfter(t_bbl *bbl) {
  MetaAPI_LivePredicateBitset result;

  if (BBL_LIVE_PREDICATES(bbl))
    result |= *BBL_LIVE_PREDICATES(bbl);
  if (BBL_DEFINED_PREDICATES(bbl))
    result |= *BBL_DEFINED_PREDICATES(bbl);

  result &= all_predicates_set;

  return result;
}

MetaAPI_LivePredicateBitset BblDeadPredicatesBefore(t_bbl *bbl) {
  return BblLivePredicatesBefore(bbl).flip() & all_predicates_set;
}

MetaAPI_LivePredicateBitset BblDeadPredicatesAfter(t_bbl *bbl) {
  return BblLivePredicatesAfter(bbl).flip() & all_predicates_set;
}

t_uint32 SelectRandomPredicateIndex(MetaAPI_LivePredicateBitset predicates) {
  t_uint32 offset = RNGGenerateWithRange(meta_api_liveness_rng, 0, max_uid);
  for (t_uint32 i = 0; i <= max_uid; i++) {
    size_t index = (offset + i) % (max_uid+1);
    if (predicates.test(index))
      return index;
  }

  FATAL(("implement me %s", predicates.to_string().c_str()));
}

void MetaAPI_Liveness(MetaAPI_ActivePredicate *pred) {
  for (auto bbl : cloud)
    BblAddLivePredicate(bbl, pred);
}

void MetaAPI_RegisterPredicate(MetaAPI_ActivePredicate *pred) {
  if (all_predicates.size() == 0) {
    /* this is the first time this function is called */
    all_predicates_set.reset();
  }

  ASSERT(all_predicates.find(pred->uid) == all_predicates.end(), ("already predicate with uid %d", pred->uid));
  all_predicates[pred->uid] = pred;

  ASSERT(pred->uid < METAAPI_MAX_PREDICATES, ("out of range"));
  if (pred->uid > max_uid)
    max_uid = pred->uid;

  all_predicates_set.set(pred->uid);
}

MetaAPI_ActivePredicate *MetaAPI_FirstActivePredicate(MetaAPI_Instance *instance, MetaAPI_Predicate *predicate) {
  for (auto i : all_predicates) {
    if (i.second->instance != instance)
      continue;
    if (i.second->predicate != predicate)
      continue;

    return i.second;
  }

  return NULL;
}

MetaAPI_ActivePredicate *MetaAPI_GetPredicateByUID(t_uint32 uid) {
  auto x = all_predicates.find(uid);
  ASSERT(x != all_predicates.end(), ("can't find predicate with uid %d", uid));
  return x->second;
}