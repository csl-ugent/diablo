#ifndef META_API_LIVENESS_H
#define META_API_LIVENESS_H

/* */
struct _MetaAPI_ActivePredicate {
  MetaAPI_Instance *instance;
  MetaAPI_Predicate *predicate;
  MetaAPI_String *embedded_name_true;
  MetaAPI_String *embedded_name_false;
  t_uint32 uid;

  _MetaAPI_ActivePredicate() {
    static t_uint32 _uid = 0;
    uid = _uid++;
    embedded_name_true = NULL;
    embedded_name_false = NULL;
  }

  struct comparator {
    bool operator()(const MetaAPI_ActivePredicate *lhs, const MetaAPI_ActivePredicate *rhs) const {
      return lhs->uid < rhs->uid;
    }
  };

  std::string Print();
};

// typedef std::set<MetaAPI_ActivePredicate *, MetaAPI_ActivePredicate::comparator> MetaAPI_ActivePredicateSet;
typedef std::map<t_uint32, MetaAPI_ActivePredicate *> MetaAPI_ActivePredicateSet;

#define METAAPI_MAX_PREDICATES 128
typedef std::bitset<METAAPI_MAX_PREDICATES> MetaAPI_LivePredicateBitset;

/* dynamic members */
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(used_predicates, USED_PREDICATES, UsedPredicates, MetaAPI_LivePredicateBitset *, {*valp = NULL;}, {if (*valp) delete *valp;}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(defined_predicates, DEFINED_PREDICATES, DefinedPredicates, MetaAPI_LivePredicateBitset *, {*valp = NULL;}, {if (*valp) delete *valp;}, {});
BBL_DYNAMIC_MEMBER_GLOBAL_BODY(live_predicates, LIVE_PREDICATES, LivePredicates, MetaAPI_LivePredicateBitset *, {*valp = NULL;}, {if (*valp) delete *valp;}, {});

void BblCopyPredicateLiveness(t_bbl *from, t_bbl *to);
bool BblIsPredicateLive(t_bbl *bbl, MetaAPI_ActivePredicate *pred);
MetaAPI_LivePredicateBitset BblDeadPredicatesBefore(t_bbl *bbl);
MetaAPI_LivePredicateBitset BblLivePredicatesBefore(t_bbl *bbl);
MetaAPI_LivePredicateBitset BblDeadPredicatesAfter(t_bbl *bbl);
MetaAPI_LivePredicateBitset BblLivePredicatesAfter(t_bbl *bbl);
t_uint32 SelectRandomPredicateIndex(MetaAPI_LivePredicateBitset predicates);
void MetaAPI_Liveness(MetaAPI_ActivePredicate *pred);
void MetaAPI_RegisterPredicate(MetaAPI_ActivePredicate *pred);
MetaAPI_ActivePredicate *MetaAPI_GetPredicateByUID(t_uint32 uid);
void MetaAPI_ForEachPredicateIn(MetaAPI_LivePredicateBitset *predicate_set, std::function<void(MetaAPI_ActivePredicate *)> helper);
MetaAPI_ActivePredicate *MetaAPI_FirstActivePredicate(MetaAPI_Instance *instance, MetaAPI_Predicate *predicate);

#endif
