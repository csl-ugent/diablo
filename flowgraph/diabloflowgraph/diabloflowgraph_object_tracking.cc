#include "diabloflowgraph.hpp"

using namespace std;

//#define DEBUG_TRACKING
#define TRACK_VERBOSITY 1

#define TRACKING_PREFIX "Tracking "

#define CACHE_SIZE 4096

BBL_DYNAMIC_MEMBER(assoc, ASSOC, Assoc, TrackingInformation*, NULL);

/* forward declarations */
static bool FunctionIsSpecial(FunctionUID uid);

struct PartitionInfo;

/* to be allocated on a per-function basis */
struct PartitionInfo {
  typedef t_uint32 Index;

  /* shared across all instances */
  static map<Index, PartitionInfo *> all_instances;
  static Index next_uid;

  /* UID for this information */
  Index id;
  /* first BBL of this function */
  t_bbl *bbl;
  /* which object file does this function belong to? */
  ObjectUID object_file;
  /* which SCC does this function belong to? */
  size_t scc_index;
  /* original function UID */
  FunctionUID original_function;

  /* constructor */
  PartitionInfo(t_function *fun) {
    id = next_uid;
    bbl = FUNCTION_BBL_FIRST(fun);

    FunctionUID uid = BBL_ORIGINAL_FUNCTION(FUNCTION_BBL_FIRST(fun));
    if (!FunctionIsSpecial(uid))
      original_function = uid;
    else
      original_function = FunctionUID_INVALID;

    all_instances[next_uid] = this;
    next_uid++;
  }

  static void Reset() {
    for (auto pair : all_instances)
      delete pair.second;

    all_instances.clear();
    next_uid = 0;
  }
};
PartitionInfo::Index PartitionInfo::next_uid = 0;
map<PartitionInfo::Index, PartitionInfo *> PartitionInfo::all_instances;
FUNCTION_DYNAMIC_MEMBER_BODY(partition_info, PARTITION_INFO, PartitionInfo, PartitionInfo *, { *valp = NULL; }, { }, { *valp = NULL; });

struct StronglyConnectedComponent {
  typedef size_t Index;
  typedef vector<StronglyConnectedComponent *> SCCVector;
  typedef set<StronglyConnectedComponent *> SCCSet;

  /* shared across all instances */
  static Index next_uid;
  static SCCVector all_instances;
  static const size_t INVALID_LEVEL = numeric_limits<size_t>::max();

  /* functions contained in this SCC */
  FunctionSet functions;
  /* successor SCCs */
  SCCSet successors;
  /* UID for this SCC */
  Index uid;
  /* assigned level */
  size_t level;
  /* to calculate 'reachable-by' information */
  set<FunctionUID> reachable_by_functions;

  StronglyConnectedComponent() {
    level = INVALID_LEVEL;
    uid = next_uid;

    next_uid++;

    all_instances.push_back(this);
  }

  static void Reset() {
    for (auto scc : all_instances)
      delete scc;

    next_uid = 0;
    all_instances.clear();
  }
};
StronglyConnectedComponent::Index StronglyConnectedComponent::next_uid = 0;
StronglyConnectedComponent::SCCVector StronglyConnectedComponent::all_instances;

struct ObjectSetInformation {
  set<FunctionUID> reachable_by_functions;
  ObjectUID object_index;
};

TrackingInformation::TrackingInformation() {
  functions.clear();
  files.clear();
  archives.clear();
}

void TrackingInformation::Merge(TrackingInformation x) {
  functions.insert(x.functions.begin(), x.functions.end());
  files.insert(x.files.begin(), x.files.end());
  archives.insert(x.archives.begin(), x.archives.end());
}

/* for correctness when printing additional set information */
static t_cfg *init_cfg = NULL;
static bool tracking_activated = false;

/* these datastructures are filled in when origin tracking is initialised */
vector<ObjectAndRange> objects_and_ranges;

vector<string> all_filenames;
static vector<string> all_archives;

vector<t_uint32> nr_instructions_in_file;
vector<t_uint32> nr_instructions_in_archive;
vector<t_uint32> nr_instructions_in_function;
/* ^^^ */

/* used in the SP support code, to disable origin tracking for the SP version */
static bool disable_origin_tracking = false;
static bool only_do_scc_information = false;

/* mapping original function UIDs to their object files */
static map<FunctionUID, ObjectUID> original_functions;

/* special function handlers */
#define SPECIAL_TO_GLOBAL(x) ((FunctionUID_INVALID - 1) - (x))
#define GLOBAL_TO_SPECIAL(x) SPECIAL_TO_GLOBAL(x)
static vector<SpecialFunctionTrackResultsHandler> special_function_track_handlers;
static FunctionUID first_special_uid = FunctionUID_INVALID;

/* Helpers ============================================================================================================ */
FunctionUID RegisterSpecialFunctionType(SpecialFunctionTrackResultsHandler handler) {
  special_function_track_handlers.resize(special_function_track_handlers.size() + 1, handler);

  FunctionUID uid = SPECIAL_TO_GLOBAL(special_function_track_handlers.size() - 1);
  VERBOSE(TRACK_VERBOSITY, (TRACKING_PREFIX "registered handler for special function uid %d (%x): %d", uid, uid, special_function_track_handlers.size() - 1));

  return uid;
}

static
bool FunctionIsSpecial(FunctionUID uid) {
  return SPECIAL_TO_GLOBAL(special_function_track_handlers.size()) <= uid;
}

template <typename T>
static string SetToString(set<T> x) {
  string result;

  for (auto i : x)
    result += to_string(i) + ",";

  return result;
}

/* simple cache implementation */
template<typename T, size_t N>
struct Cache {
  array<T, N> values;
  array<bool, N> valid;
  array<size_t, N> ids;

  size_t hits, misses;

  void Init() {
    hits = 0;
    misses = 0;
    valid.fill(false);
  }

  /* avoid copy */
  /* non-const reference returned here to be able to modify it externally */
  T& Value(size_t index) {
    return values[index % N];
  }

  bool IsValid(size_t index) {
    return valid[index % N] && (ids[index % N] == index);
  }

  void MarkValid(size_t index) {
    valid[index % N] = true;
    ids[index % N] = index;
  }

  void Hit() {
    hits++;
  }

  void Miss() {
    misses++;
  }

  void Report(string name) {
    size_t accesses = hits+misses;
    VERBOSE(0, (TRACKING_PREFIX "Cache statistics for '%s'", name.c_str()));
    VERBOSE(0, (TRACKING_PREFIX "  Hits    : %d (%.4f)", hits, (double)hits/accesses));
    VERBOSE(0, (TRACKING_PREFIX "  Misses  : %d (%.4f)", misses, (double)misses/accesses));
    VERBOSE(0, (TRACKING_PREFIX "  Accesses: %d", accesses));
  }
};

static
void ObjectListToArchivesObjectFiles(set<ObjectUID> objects, set<SourceArchiveUID>& archives, set<SourceFileUID>& files) {
  archives.clear();
  files.clear();

  for (auto obj : objects) {
    archives.insert(objects_and_ranges[obj].archive_id);
    files.insert(objects_and_ranges[obj].filename_id);
  }
}

static
set<ObjectUID> FunctionsToObjects(set<FunctionUID> functions) {
  set<ObjectUID> result;
  for (FunctionUID fun_uid : functions)
  {
    if (FunctionIsSpecial(fun_uid))
      continue;

    ASSERT(original_functions.find(fun_uid) != original_functions.end(), ("could not find UID %d in list of original functions", fun_uid));
    result.insert(original_functions[fun_uid]);
  }

  return result;
}

static
set<FunctionUID> FunctionGetReachableByFunctions(t_function *fun) {
  set<FunctionUID> result;

  if (FUNCTION_PARTITION_INFO(fun))
    result = StronglyConnectedComponent::all_instances[FUNCTION_PARTITION_INFO(fun)->scc_index]->reachable_by_functions;

  return result;
}

static
set<FunctionUID> FunctionGetAssociatedWithFunctions(t_bbl *bbl, map<FunctionUID, t_uint64>& exec_per_function) {
  set<FunctionUID> result;
  exec_per_function.clear();

  t_function *fun = BBL_FUNCTION(bbl);
  FunctionUID uid = BBL_ORIGINAL_FUNCTION(FUNCTION_BBL_FIRST(fun));

  if (FunctionIsSpecial(uid)) {
    /* handle special function */
    auto handler = special_function_track_handlers[GLOBAL_TO_SPECIAL(uid)];
    SpecialFunctionTrackResults x = handler(FUNCTION_BBL_FIRST(fun));

    result.insert(x.associated_with_functions.begin(), x.associated_with_functions.end());
    exec_per_function = x.exec_per_function;
  }
  else {
    result.insert(uid);
    exec_per_function[uid] = BBL_EXEC_COUNT(bbl);
  }

  return result;
}

/* Original function UID bookkeeping ================================================================================== */
void RecordFunctionsAsOriginal(t_cfg *cfg) {
  static bool init = false;
  ASSERT(!init, ("can't record functions as original twice!"));
  init = true;

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    /* placeholder value, so we can count the number of original functions
     * in InitialiseObjectFileTracking */
    original_functions[FUNCTION_ID(fun)] = ObjectUID_INVALID;

    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl)
    BBL_SET_ORIGINAL_FUNCTION(bbl, FUNCTION_ID(fun));
  }
}

static void FixOriginalFunctionInformation(t_cfg *cfg) {
  ASSERT(original_functions.size() > 0, ("no original functions defined, call 'RecordFunctionsAsOriginal' first."));

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (BBL_IS_HELL(bbl) || IS_DATABBL(bbl) || BblIsExitBlock(bbl) || BBL_ATTRIB(bbl) & BBL_DATA_POOL)
      continue;

    /* don't need to do anything */
    if (BBL_ORIGINAL_FUNCTION(bbl) != FunctionUID_INVALID)
      continue;

    FunctionUID original_id = FunctionUID_INVALID;

    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e) {
      t_bbl *from = CFG_EDGE_HEAD(e);

      if (CFG_EDGE_CAT(e) == ET_RETURN) {
        ASSERT(CFG_EDGE_CORR(e), ("@E does not have corr edge! @eiB", e, bbl));
        from = CFG_EDGE_HEAD(CFG_EDGE_CORR(e));
      }

      if (original_id == FunctionUID_INVALID)
        original_id = BBL_ORIGINAL_FUNCTION(from);
      else
        ASSERT(original_id == BBL_ORIGINAL_FUNCTION(from), ("could not determine original function for @eiB", bbl));
    }

    BBL_SET_ORIGINAL_FUNCTION(bbl, original_id);
  }
}

static
ObjectUID ObjectFileForBbl(t_bbl *bbl)
{
  static ObjectUID object_range_idx = ObjectUID_INVALID;

  if (BblIsExitBlock(bbl)
      || BBL_IS_HELL(bbl))
    return ObjectUID_INVALID;

  if (AddressIsEq(BBL_OLD_ADDRESS(bbl), AddressNew32(0)))
  {
    /* this BBL has been created by Diablo */
    return ObjectUID_INVALID;
  }
  else
  {
    /* based on the old address, look up the object file this BBL belongs to */
    if (object_range_idx == ObjectUID_INVALID
        || !(AddressIsGe(BBL_OLD_ADDRESS(bbl), objects_and_ranges[object_range_idx].begin)
              && AddressIsLt(BBL_OLD_ADDRESS(bbl), objects_and_ranges[object_range_idx].end)))
    {
      object_range_idx = ObjectUID_INVALID;

      /* look for the correct object file this BBL belongs to */
      ObjectUID idx = 0;
      for (auto object_range : objects_and_ranges)
      {
        if (AddressIsGe(BBL_OLD_ADDRESS(bbl), object_range.begin)
            && AddressIsLt(BBL_OLD_ADDRESS(bbl), object_range.end))
        {
          object_range_idx = idx;
          break;
        }

        idx++;
      }
    }
  }

  return object_range_idx;
}

static
void IoModifierBblTracking(t_bbl *bbl, t_string_array *array)
{
  if (!tracking_activated)
    return;
  if (BBL_CFG(bbl) != init_cfg)
    return;

  ObjectSetUID set_id = BBL_OBJECT_SET(bbl);

  string set_name = "INVALID_SET";
  if (set_id != ObjectSetUID_INVALID)
    set_name = to_string(set_id);

  string result = "set(" + set_name + ")";

  size_t nr_reachable = 0;
  if (!BBL_FUNCTION(bbl)) {
    result += " NOT_IN_FUNCTION";
  }
  else {
    if (FUNCTION_PARTITION_INFO(BBL_FUNCTION(bbl))
        && FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl)) == bbl) {
      PartitionInfo *pi = FUNCTION_PARTITION_INFO(BBL_FUNCTION(bbl));
      result += " (set " + to_string(pi->id) + "/scc " + to_string(pi->scc_index) +")\\l";
      //result += " [" + SetToString(FunctionGetReachableByFunctions(BBL_FUNCTION(bbl))) + "]";
      nr_reachable = FunctionGetReachableByFunctions(BBL_FUNCTION(bbl)).size();
    }
  }

  FunctionUID fun_uid = FunctionUID_INVALID;
  if (BBL_FUNCTION(bbl)) {
    /* this code is only here for printing ASSERT and FATAL messages for BBLs that don't belong to any function */
    fun_uid = FUNCTION_ID(BBL_FUNCTION(bbl));
  }

  StringArrayAppendString(array, StringIo("Tracking: %s F(%d/%d) R(%d)\\l", result.c_str(), BBL_ORIGINAL_FUNCTION(bbl), fun_uid, nr_reachable));
}

static
void AllocateNewSet(t_function *fun) {
}

void UpdateObjectTrackingAfterBblSplit(t_bbl *first, t_bbl *second) {
  BBL_SET_OBJECT_SET(second, BBL_OBJECT_SET(first));
  BBL_SET_ORIGINAL_FUNCTION(second, BBL_ORIGINAL_FUNCTION(first));
}

void UpdateObjectTrackingAfterBblInsertInFunction(t_bbl *bbl) {
  BBL_SET_ORIGINAL_FUNCTION(bbl, BBL_ORIGINAL_FUNCTION(FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl))));
}

void UpdateObjectTrackingAfterEdgeSplit(t_cfg_edge *split_edge, t_bbl *new_bbl, t_bbl *original_tail) {
  if (CFG_EDGE_CAT(split_edge) == ET_RETURN)
    BBL_SET_OBJECT_SET(new_bbl, BBL_OBJECT_SET(CFG_EDGE_HEAD(CFG_EDGE_CORR(split_edge))));
  else
    BBL_SET_OBJECT_SET(new_bbl, BBL_OBJECT_SET(CFG_EDGE_HEAD(split_edge)));
}

void UpdateObjectTrackingBeforeKillingEdge(t_cfg_edge *edge) {
  /* placeholder */
}

static
void PrintSCC(StronglyConnectedComponent *scc) {
  VERBOSE(0, ("SCC %d L%d contains %d functions", scc->uid, scc->level, scc->functions.size()));
  for (auto fun : scc->functions)
    VERBOSE(0, ("  contains @F", fun));
}

static
void FindStronglyConnectedComponents(t_cfg *cfg, function<void(t_function *, t_uint32)> fn_set_scc_uid, function<t_uint32(t_function *)> fn_get_scc_uid) {
  /* Using Tarjan's algorithm here */
  typedef t_uint32 t_index;
  static const t_index INDEX_INVALID = numeric_limits<t_index>::max();

  /* index getter/setter */
  auto FunctionSetIndex = [](t_function *fun, t_index index) {
    FUNCTION_SET_TMP(fun, reinterpret_cast<void *>(index));
  };
  auto FunctionGetIndex = [](t_function *fun) {
    return static_cast<t_index>(reinterpret_cast<size_t>(FUNCTION_TMP(fun)));
  };

  /* lowlink getter/setter */
  auto FunctionSetLowlink = [](t_function *fun, t_index lowlink) {
    FUNCTION_SET_MIN_DFS(fun, lowlink);
  };
  auto FunctionGetLowlink = [](t_function *fun) {
    return FUNCTION_MIN_DFS(fun);
  };

  /* look up successors of function */
  auto Successors = [](t_function *fun) {
    FunctionSet result;

    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl)
    {
      if (BblIsExitBlock(bbl))
        continue;

      t_cfg_edge *e;
      BBL_FOREACH_SUCC_EDGE(bbl, e)
      {
        if (CfgEdgeIsFake(e))
          continue;

        if (!CfgEdgeIsInterproc(e) || CFG_EDGE_CAT(e) == ET_CALL || CFG_EDGE_CAT(e) == ET_SWI)
          continue;

        t_function *tail_function = BBL_FUNCTION(CFG_EDGE_TAIL(e));
        if (FUNCTION_IS_HELL(tail_function))
          continue;

        result.insert(tail_function);
      }
    }

    return result;
  };

  /* initialisation */
  size_t function_count = 0;
  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    FunctionSetIndex(fun, INDEX_INVALID);
    FunctionSetLowlink(fun, INDEX_INVALID);
    FunctionUnmark(fun);

    function_count++;
  }

  struct Iterator {
    FunctionSet data;
    FunctionSet::iterator iterator;
    t_function *from = NULL;
  };

  /* global variables */
  t_index index = 0;
  vector<t_function *> S;

  /* initial iterator: all functions */
  Iterator *new_iterator = new Iterator();
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    new_iterator->data.insert(fun);
  }
  new_iterator->iterator = new_iterator->data.begin();

  Iterator *current_iterator = new_iterator;
  vector<Iterator *> iterator_stack;

  /* process one node */
  while (true) {
    if (current_iterator->iterator == current_iterator->data.end()) {
      /* end of iterable data */

      /* only for the initial iterator */
      auto v = current_iterator->from;
      if (!v)
        break;

      /* set low */
      for (auto x : current_iterator->data) {
        if (!FunctionIsMarked(x))
          continue;

        FunctionSetLowlink(v, min(FunctionGetLowlink(v), FunctionGetLowlink(x)));
      }

      /* delete the iterator */
      delete current_iterator;

      /* make SCC */
      if (FunctionGetLowlink(v) == FunctionGetIndex(v)) {
        /* start a new strongly connected component */
        StronglyConnectedComponent *scc = new StronglyConnectedComponent();

        t_function *w;
        do {
          w = S.back();
          S.pop_back();

          FunctionUnmark(w);

          scc->functions.insert(w);

          /* original function */
          if (FUNCTION_PARTITION_INFO(w)->original_function != FunctionUID_INVALID)
            scc->reachable_by_functions.insert(FUNCTION_PARTITION_INFO(w)->original_function);

          FUNCTION_PARTITION_INFO(w)->scc_index = scc->uid;
        } while (w != v);
      }

      /* no more iterators left */
      if (iterator_stack.size() == 0)
        break;

      /* go to the next iterator */
      current_iterator = iterator_stack.back();
      iterator_stack.pop_back();
    }
    else {
      auto w = *(current_iterator->iterator);
      current_iterator->iterator++;

      if (FunctionGetIndex(w) == INDEX_INVALID) {
        /* not visited */

        /* initialise unvisited node */
        FunctionSetIndex(w, index);
        FunctionSetLowlink(w, index);
        index++;

        S.push_back(w);
        FunctionMark(w);

        /* create new iterator for this function */
        iterator_stack.push_back(current_iterator);

        current_iterator = new Iterator();
        current_iterator->data = Successors(w);
        current_iterator->iterator = current_iterator->data.begin();
        current_iterator->from = w;
      }
      else {
        if (FunctionIsMarked(w))
          FunctionSetLowlink(current_iterator->from, min(FunctionGetLowlink(current_iterator->from), FunctionGetIndex(w)));
      }
    }
  }

  auto& all_sccs = StronglyConnectedComponent::all_instances;
  VERBOSE(0, (TRACKING_PREFIX "%d functions clustered in %d SCCs", function_count, all_sccs.size()));

  /* finalisation */
  /*   1. assign SCC index to every function */
  for (size_t scc_index = 0; scc_index < all_sccs.size(); scc_index++) {
    /* set the SCC function index */
    for (auto function : all_sccs[scc_index]->functions)
      fn_set_scc_uid(function, scc_index);
  }

  /*   2. successor information */
  for (auto scc : all_sccs) {
    for (auto function : scc->functions) {
      for (auto succ : Successors(function)) {
        /* in same SCC */
        if (fn_get_scc_uid(succ) == fn_get_scc_uid(function))
          continue;

        /* SCC interlink */
        scc->successors.insert(all_sccs[fn_get_scc_uid(succ)]);
      }
    }
  }
}

static
void FunctionSetSccUID(t_function *fun, t_uint32 uid) {
  FUNCTION_SET_MIN_DFS(fun, uid);
}

static
t_uint32 FunctionGetSccUID(t_function *fun) {
  return FUNCTION_MIN_DFS(fun);
}

static
void CalculateSCCsAndReachableBy(t_cfg *cfg, t_string basename) {
  t_function *fun;
  t_cfg_edge *e;

  auto open_and_check = [] (t_string basename, string suffix, ofstream& f) {
    string full_name = string(basename) + suffix;

    f.open(full_name);
    ASSERT(f.is_open(), ("could not open output file %s", full_name.c_str()));
  };

  auto close = [] (ofstream& f) {
    f << "# END OF FILE" << endl;
    f.close();
  };

  /* preparation */
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun)) {
      FUNCTION_SET_PARTITION_INFO(fun, NULL);
    }
    else {
      /* allocate new set to function */
      FUNCTION_SET_PARTITION_INFO(fun, new PartitionInfo(fun));
    }
  }

  /* WARNING The following function uses the TMP and MIN_DFS members of t_function to store temporary data!
   *         The above setter/getter can use MIN_DFS because it is overwritten at the end of the callee. */
  FindStronglyConnectedComponents(cfg, FunctionSetSccUID, FunctionGetSccUID);

  /* look for entry functions and corresponding SCCs */
  StronglyConnectedComponent::SCCSet entry_sccs;
  /* 1. functions reachable from the CFG unique entry point */
  BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg), e) {
    t_function *fun = BBL_FUNCTION(CFG_EDGE_TAIL(e));
    ASSERT(fun, ("entry point not in function @eiB", CFG_EDGE_TAIL(e)));

    if (!FUNCTION_IS_HELL(fun))
      entry_sccs.insert(StronglyConnectedComponent::all_instances[FunctionGetSccUID(fun)]);
  }
  /* 2. functions reachable from HELL */
  BBL_FOREACH_SUCC_EDGE(CFG_HELL_NODE(cfg), e) {
    t_function *fun = BBL_FUNCTION(CFG_EDGE_TAIL(e));
    ASSERT(fun, ("from hell but not in function @eiB", CFG_EDGE_TAIL(e)));

    if (!FUNCTION_IS_HELL(fun))
      entry_sccs.insert(StronglyConnectedComponent::all_instances[FunctionGetSccUID(fun)]);
  }
  /* 3. functions that are only called */
  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    t_bbl *entry = FUNCTION_BBL_FIRST(fun);

    bool only_calls = true;
    BBL_FOREACH_PRED_EDGE(entry, e)
    {
      if (CfgEdgeIsFake(e))
        continue;

      if (CFG_EDGE_CAT(e) != ET_CALL && CfgEdgeIsInterproc(e))
      {
        only_calls = false;
        break;
      }
    }

    if (only_calls)
      entry_sccs.insert(StronglyConnectedComponent::all_instances[FunctionGetSccUID(fun)]);
  }

  vector<StronglyConnectedComponent::SCCSet> all_level_information;

  /* construct first level */
  StronglyConnectedComponent::SCCSet level_0 = entry_sccs;
  for (auto scc : level_0)
    scc->level = 0;
  all_level_information.push_back(level_0);

  /* keep track of all the SCCs that have been assigned a level so far */
  StronglyConnectedComponent::SCCSet all_leveled_sccs = all_level_information[0];

  /* assign levels */
  auto assign_levels = [&all_leveled_sccs] (vector<StronglyConnectedComponent::SCCSet>& all_level_information) {
    size_t current_level = 0;
    while (true) {
      current_level++;

      StronglyConnectedComponent::SCCSet new_level_info;

      for (auto scc : all_level_information[current_level - 1]) {
        for (auto succ : scc->successors) {
          if (all_leveled_sccs.find(succ) == all_leveled_sccs.end()) {
            new_level_info.insert(succ);

            all_leveled_sccs.insert(succ);
            succ->level = current_level;
          }
        }
      }

      if (new_level_info.size() > 0)
        all_level_information.push_back(new_level_info);
      else
        break;
    }
  };

  /* construct deeper levels */
  assign_levels(all_level_information);

  /* maybe some SCCs have not been visited yet */
  StronglyConnectedComponent::SCCVector unassigned_sccs;
  for (auto scc : StronglyConnectedComponent::all_instances) {
    if (scc->level == StronglyConnectedComponent::INVALID_LEVEL) {
      VERBOSE(0, ("no initial level asigned to SCC!"));
      PrintSCC(scc);
      unassigned_sccs.push_back(scc);
    }
  }

  /* make sure we visit all SCCs */
  while (unassigned_sccs.size() > 0) {
    StronglyConnectedComponent *the_chosen_one = unassigned_sccs.back();
    unassigned_sccs.pop_back();

    if (the_chosen_one->level != StronglyConnectedComponent::INVALID_LEVEL)
      /* got a level already, no work needed */
      continue;

    vector<StronglyConnectedComponent::SCCSet> new_all_level_information;

    /* construct first level */
    StronglyConnectedComponent::SCCSet level_0;
    level_0.insert(the_chosen_one);
    for (auto scc : level_0)
      scc->level = 0;
    new_all_level_information.push_back(level_0);

    all_leveled_sccs.insert(level_0.begin(), level_0.end());

    assign_levels(new_all_level_information);

    for (size_t i = 0; i < new_all_level_information.size(); i++) {
      auto new_info = new_all_level_information[i];

      if (i == all_level_information.size())
        /* add new element if needed */
        all_level_information.push_back(new_info);
      else
        /* merge new level information */
        all_level_information[i].insert(new_info.begin(), new_info.end());
    }
  }

  /* propagate information from SCC to SCC, fixpoint */
  bool updated = true;
  while (updated) {
    updated = false;

    for (size_t level = 0; level < all_level_information.size(); level++) {
      for (auto scc : all_level_information[level]) {
        for (auto succ : scc->successors) {
          size_t old_size = succ->reachable_by_functions.size();
          succ->reachable_by_functions.insert(scc->reachable_by_functions.begin(), scc->reachable_by_functions.end());

          updated |= (old_size != succ->reachable_by_functions.size());
        }
      }
    }
  }

  /* emit SCC information */
  ofstream sfile; open_and_check(basename, ".scc-information", sfile);
  sfile << "# SCC_UID:N_FUNCTIONS:N_SUCCESSORS:LEVEL:N_REACHABLE_BY_FUN" << endl;

  for (auto scc : StronglyConnectedComponent::all_instances) {
    sfile << scc->uid;
    sfile << ":NF" << scc->functions.size();
    sfile << ":NS" << scc->successors.size();
    sfile << ":L" << scc->level;
    sfile << ":NR" << scc->reachable_by_functions.size();

    sfile << ":";
    for (auto succ : scc->successors)
      sfile << "s" << succ->uid << ",";

    sfile << ":";
    for (auto fun : scc->functions) {
      sfile << "F(";
      if (!FUNCTION_NAME(fun))
        sfile << "noname";
      else
        sfile << FUNCTION_NAME(fun);
      sfile << "),";
    }

    sfile << ":";
    for (auto fun : scc->reachable_by_functions)
      sfile << "R" << fun << ",";

    sfile << endl;
  }

  close(sfile);
}

static Cache<ObjectSetInformation, CACHE_SIZE> info_cache;
static
ObjectSetInformation& ObjectSetInformationForFunction(t_function *fun) {
  if (!FUNCTION_PARTITION_INFO(fun))
    CfgDrawFunctionGraphs(FUNCTION_CFG(fun), "boem");
  ASSERT(FUNCTION_PARTITION_INFO(fun), ("no partition info for @F", fun));
  auto id = FUNCTION_PARTITION_INFO(fun)->id;
  ObjectSetInformation& result = info_cache.Value(id);

  if (info_cache.IsValid(id)) {
    info_cache.Hit();
    return result;
  }

  info_cache.Miss();


  /* cache is not valid, construct entry */
  result.reachable_by_functions = FunctionGetReachableByFunctions(fun);

  /* object file index */
  ObjectUID object_file = ObjectFileForBbl(FUNCTION_PARTITION_INFO(fun)->bbl);
  if (object_file == ObjectUID_INVALID && result.reachable_by_functions.size() == 1) {
    map <FunctionUID, t_uint64> _;
    auto associated_functions = FunctionGetAssociatedWithFunctions(FUNCTION_BBL_FIRST(fun), _);
    if (associated_functions.size() == 1) {
      FunctionUID function_uid = *(associated_functions.begin());

      /* map to original function UID */
      if (original_functions.find(function_uid) != original_functions.end())
        object_file = original_functions[function_uid];
    }
  }
  result.object_index = object_file;

  /* mark cache entry as valid and return requested data */
  info_cache.MarkValid(id);
  return result;
}

static Cache<TrackingInformation, CACHE_SIZE> tracking_cache;
static
TrackingInformation CalculateReachableBy(t_bbl *bbl) {
  size_t set_id = BBL_OBJECT_SET(bbl);
  TrackingInformation& result = tracking_cache.Value(set_id);

  /* present in the cache? */
  if (tracking_cache.IsValid(set_id)) {
    tracking_cache.Hit();
    return result;
  }

  tracking_cache.Miss();

  result.functions = ObjectSetInformationForFunction(BBL_FUNCTION(bbl)).reachable_by_functions;
  ObjectListToArchivesObjectFiles(FunctionsToObjects(result.functions), result.archives, result.files);

  if (result.files.size() == 0)
    VERBOSE(0, ("@eiB not associated with any object file", bbl));
  if (result.archives.size() == 0)
    VERBOSE(0, ("@eiB not associated with any library", bbl));

  tracking_cache.MarkValid(set_id);

  return result;
}

TrackingInformation CalculateAssociatedWith(t_bbl *bbl) {
  TrackingInformation result;

  result.functions = FunctionGetAssociatedWithFunctions(bbl, result.exec_per_function);
  ObjectListToArchivesObjectFiles(FunctionsToObjects(result.functions), result.archives, result.files);

  return result;
}

static
void CalculateReachableAssociatedNumbers(t_cfg *cfg, t_string directory) {
  /* DATA: reachable-by */
  map<size_t, size_t> nr_data_reachable_by_libraries;
  map<size_t, size_t> nr_data_reachable_by_objects;
  map<size_t, size_t> nr_data_reachable_by_functions;

  /* CODE: reachable-by */
  map<size_t, size_t> nr_insns_reachable_by_libraries;
  map<size_t, size_t> nr_insns_reachable_by_objects;
  map<size_t, size_t> nr_insns_reachable_by_functions;

  /* CODE: associated-with */
  map<size_t, size_t> nr_insns_associated_with_libraries;
  map<size_t, size_t> nr_insns_associated_with_objects;
  map<size_t, size_t> nr_insns_associated_with_functions;

  size_t total_ins_count = 0;
  size_t total_data_count = 0;

  /* collect statistics for each BBL */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (BBL_IS_HELL(bbl)
        || BblIsExitBlock(bbl))
      continue;

    auto nins = BBL_NINS(bbl);

    TrackingInformation reachable_by = TrackingInformation();

    if (IS_DATABBL(bbl)
        || BBL_ATTRIB(bbl) & BBL_DATA_POOL) {
      /* DATA, reachable through global branch redirection */
      total_data_count += nins;

      t_cfg_edge *e;
      BBL_FOREACH_PRED_EDGE(bbl, e) {
        /* _start calls HELL (dynamic linked), which returns to DATA */
        if (CFG_EDGE_CAT(e) == ET_RETURN)
          continue;
        if (CfgEdgeIsFake(e))
          continue;
        if (BBL_IS_HELL(CFG_EDGE_HEAD(e)))
          continue;

        reachable_by.Merge(CalculateReachableBy(CFG_EDGE_HEAD(e)));
      }

      nr_data_reachable_by_functions[reachable_by.functions.size()] += nins;
      nr_data_reachable_by_objects[reachable_by.files.size()] += nins;
      nr_data_reachable_by_libraries[reachable_by.archives.size()] += nins;
    }
    else {
      /* CODE */
      if (BBL_OBJECT_SET(bbl) == ObjectSetUID_INVALID
          /* In AF, default switch case blocks are made with no incoming/outgoing edges.
          * See: diabloarm_layout.c, search for "AdvancedFactoring". */
          && (BBL_PRED_FIRST(bbl) == NULL && BBL_SUCC_FIRST(bbl) == NULL)) {
        continue;
      }

      total_ins_count += nins;

      /* sanity check */
      if (BBL_OBJECT_SET(bbl) == ObjectSetUID_INVALID)
        CfgDrawFunctionGraphs(cfg, "invalid_object_id");
      ASSERT(BBL_OBJECT_SET(bbl) != ObjectSetUID_INVALID, ("invalid object set ID for @eiB", bbl));

      TrackingInformation reachable_by = CalculateReachableBy(bbl);
      nr_insns_reachable_by_libraries[reachable_by.archives.size()] += nins;
      nr_insns_reachable_by_objects[reachable_by.files.size()] += nins;
      nr_insns_reachable_by_functions[reachable_by.functions.size()] += nins;

      TrackingInformation associated_with = CalculateAssociatedWith(bbl);
      nr_insns_associated_with_libraries[associated_with.archives.size()] += nins;
      nr_insns_associated_with_objects[associated_with.files.size()] += nins;
      nr_insns_associated_with_functions[associated_with.functions.size()] += nins;
    }
  }

  /* summary */
  auto print_data = [&directory] (string str, map<size_t, size_t>& data) {
    for (auto p : data)
      VERBOSE(0, (TRACKING_PREFIX "%s_%s:%d:%d", directory, str.c_str(), p.first, p.second));
  };

  /* 1. data */
  VERBOSE(0, (TRACKING_PREFIX "%s_data_reachable:%d", directory, total_data_count));
  print_data("data_reachable_by_library", nr_data_reachable_by_libraries);
  print_data("data_reachable_by_object", nr_data_reachable_by_objects);
  print_data("data_reachable_by_functions", nr_data_reachable_by_functions);

  /* 2. code */
  VERBOSE(0, (TRACKING_PREFIX "%s_insns_reachable:%d", directory, total_ins_count));
  print_data("insns_reachable_by_library", nr_insns_reachable_by_libraries);
  print_data("insns_reachable_by_object", nr_insns_reachable_by_objects);
  print_data("insns_reachable_by_functions", nr_insns_reachable_by_functions);

  print_data("insns_associated_with_library", nr_insns_associated_with_libraries);
  print_data("insns_associated_with_object", nr_insns_associated_with_objects);
  print_data("insns_associated_with_functions", nr_insns_associated_with_functions);
}

static bool already_printed = false;
static inline
void BblExecCountSanityCheck(t_bbl *bbl) {
  if (BBL_EXEC_COUNT(bbl) != 0) {
    t_cfg_edge *e;

    t_uint64 x = 0;
    BBL_FOREACH_PRED_EDGE(bbl, e) {
      if (CfgEdgeIsFake(e))
        continue;

      x += BBL_EXEC_COUNT(CFG_EDGE_HEAD(e));

      if (BBL_IS_HELL(CFG_EDGE_HEAD(e))
          || BblIsExitBlock(CFG_EDGE_HEAD(e)))
        x++;
    }

    if (x == 0) {
      /* */
      if (!already_printed) {
        //CfgDrawFunctionGraphsWithHotness(BBL_CFG(bbl), "origin-exec");
        already_printed = true;
      }
      WARNING(("@eiB should have at least one incoming BBL that is executed", bbl));
    }

    x = 0;
    BBL_FOREACH_SUCC_EDGE(bbl, e) {
      if (CfgEdgeIsFake(e))
        continue;

      x += BBL_EXEC_COUNT(CFG_EDGE_TAIL(e));

      if (BBL_IS_HELL(CFG_EDGE_TAIL(e))
          || BblIsExitBlock(CFG_EDGE_TAIL(e)))
        x++;
    }

    if (x == 0) {
      /* tolerate unexecuted AF functions */
      if (BBL_SUCC_FIRST(bbl) == BBL_SUCC_LAST(bbl)
          && CfgEdgeIsInterproc(BBL_SUCC_FIRST(bbl))
          && FunctionIsSpecial(BBL_ORIGINAL_FUNCTION(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl))))) {
        /* OK */;
      }
      else {
        /* not OK */
        if (!already_printed) {
          //CfgDrawFunctionGraphsWithHotness(BBL_CFG(bbl), "origin-exec");
          already_printed = true;
        }
        WARNING(("@eiB should have at least one outgoing BBL that is executed", bbl));
      }
    }
  }
}

#define ET_CALLFALLTHROUGH -1
static
void EmitOriginTrackingInformation(t_cfg *cfg, t_string basename) {
  /* draw function graphs */
  CfgDrawFunctionGraphsWithHotness(cfg, basename);

  auto open_and_check = [] (t_string basename, string suffix, ofstream& f) {
    string full_name = string(basename) + suffix;

    f.open(full_name);
    ASSERT(f.is_open(), ("could not open output file %s", full_name.c_str()));
  };

  auto close = [] (ofstream& f) {
    f << "# END OF FILE" << endl;
    f.close();
  };

  /* edge listing */
  ofstream efile; open_and_check(basename, ".edges", efile);
  efile << "# EDGE_ID:HEAD_ADDRESS:BRANCH_INSTRUCTION:TAIL_ADDRESS:TO_PLT:EDGE_TYPE:IS_FAKE:FLAGS:AF_CORR_UID" << endl;

  t_cfg_edge *edge;
  size_t edge_uid = 0;
  CFG_FOREACH_EDGE(cfg, edge) {
    /* don't emit these edges */
    if (CFG_EDGE_CAT(edge) == ET_COMPENSATING
        || CFG_EDGE_CAT(edge) == ET_RETURN)
      continue;

    auto check_bbl = [] (t_bbl *bbl) {
      return BBL_FUNCTION(bbl)
              && !FUNCTION_IS_HELL(BBL_FUNCTION(bbl))
              && !IS_DATABBL(bbl)
              && !BblIsExitBlock(bbl)
              && !(BBL_ATTRIB(bbl) & BBL_DATA_POOL)
              && !BBL_IS_HELL(bbl);
    };

    /* check head and tail */
    t_bbl *head = CFG_EDGE_HEAD(edge);
    t_bbl *tail = CFG_EDGE_TAIL(edge);

    bool is_dyncall = BBL_FUNCTION(tail)
                      && FUNCTION_IS_HELL(BBL_FUNCTION(tail))
                      && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(BBL_FUNCTION(tail))) == BBL_CH_DYNCALL;
    bool is_callhell = BBL_FUNCTION(tail)
                        && FUNCTION_IS_HELL(BBL_FUNCTION(tail))
                        && BBL_CALL_HELL_TYPE(FUNCTION_BBL_FIRST(BBL_FUNCTION(tail))) == BBL_CH_NORMAL;

    if (!check_bbl(head) || (!check_bbl(tail) && !(is_dyncall || is_callhell)))
      continue;

    auto print_edge = [&efile, &edge_uid, &edge] (t_bbl *head, t_bbl *tail, t_int32 type, bool is_dyncall, bool is_callhell, bool fake) {

      efile << edge_uid;

      /* head address */
      ASSERT(BBL_INS_FIRST(head), ("no instruction in head @eiB", head));
      auto head_address = StringIo("@G", INS_CADDRESS(BBL_INS_FIRST(head)));
      efile << ":" << head_address;
      Free(head_address);

      /* branch address */
      auto branch_address = StringIo("@G", INS_CADDRESS(BBL_INS_LAST(head)));
      efile << ":" << branch_address;
      Free(branch_address);

      /* tail address */
      ASSERT(BBL_INS_FIRST(tail) || is_dyncall || is_callhell, ("no instruction in tail @eiB", tail));
      auto tail_address = StringIo("@G", (is_dyncall || is_callhell) ? 0 : INS_CADDRESS(BBL_INS_FIRST(tail)));
      efile << ":" << tail_address;
      Free(tail_address);

      /* edges to the PLT are skipped in the above code */
      efile << ":" << is_dyncall;

      /* edge type */
      efile << ":" << type;

      /* fake or not? */
      efile << ":" << fake;

      /* flags */
#define EDGE_FLAG_TO_AF_FUNCTION    1<<0
#define EDGE_FLAG_FROM_AF_FUNCTION  1<<1
#define EDGE_FLAG_EXECUTED          1<<2
      t_uint32 flags = 0;
      bool from_af = false;
      DiabloBrokerCall("FunctionIsAF", BBL_FUNCTION(head), &from_af);
      if (from_af)
        flags |= EDGE_FLAG_FROM_AF_FUNCTION;

      bool to_af = false;
      DiabloBrokerCall("FunctionIsAF", BBL_FUNCTION(tail), &to_af);
      if (to_af)
        flags |= EDGE_FLAG_TO_AF_FUNCTION;

      if (CFG_EDGE_EXEC_COUNT(edge) > 0)
        flags |= EDGE_FLAG_EXECUTED;

      efile << ":" << flags;

      efile << endl;

      edge_uid++;
    };

    auto first_real_bbl = [] (t_bbl *bbl) {
      while (!BBL_INS_FIRST(bbl))
        bbl = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));

      return bbl;
    };

    if (!BBL_INS_FIRST(head))
      continue;

    if (!BBL_INS_FIRST(tail) && !(is_dyncall || is_callhell)) {
      tail = first_real_bbl(tail);
    }

    print_edge(head, tail, CFG_EDGE_CAT(edge), is_dyncall, is_callhell, CfgEdgeIsFake(edge));
    if (CFG_EDGE_CAT(edge) == ET_CALL
        && CFG_EDGE_CORR(edge)) {
      tail = first_real_bbl(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));

      print_edge(head, tail, ET_CALLFALLTHROUGH, false, false, false);
    }
  }

  close(efile);

  /* function listing */
  ofstream ffile; open_and_check(basename, ".functions", ffile);
  ffile << "# FUNCTION_ID:NAME:ORIGINAL_FUNCTION_ID:PARTITION_ID" << endl;
  map<FunctionUID, t_bbl *> function_to_bbl;

  ofstream ffile2; open_and_check(basename, ".functions-reachable", ffile2);
  ffile2 << "# FUNCTION_ID:NAME:ORIGINAL_FUNCTION_ID:NR_REACHABLE" << endl;

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_IS_HELL(fun))
      continue;

    ffile << FUNCTION_ID(fun);
    ffile << ":" << ((FUNCTION_NAME(fun)) ? FUNCTION_NAME(fun) : "(null)");

    ffile2 << FUNCTION_ID(fun);
    ffile2 << ":" << ((FUNCTION_NAME(fun)) ? FUNCTION_NAME(fun) : "(null)");

    ffile << ":";
    ffile2 << ":";
    if (original_functions.find(FUNCTION_ID(fun)) != original_functions.end()) {
      ffile << -1;
      ffile2 << -1;
      function_to_bbl[FUNCTION_ID(fun)] = FUNCTION_BBL_FIRST(fun);
    }
    else {
      t_bbl *bbl = FUNCTION_BBL_FIRST(fun);
      ffile << static_cast<make_signed<FunctionUID>::type>(BBL_ORIGINAL_FUNCTION(bbl));
      ffile2 << static_cast<make_signed<FunctionUID>::type>(BBL_ORIGINAL_FUNCTION(bbl));
      function_to_bbl[BBL_ORIGINAL_FUNCTION(bbl)] = bbl;
    }

    ffile << ":" << FUNCTION_PARTITION_INFO(fun)->id;
    ffile2 << ":" << FunctionGetReachableByFunctions(fun).size();

    ffile << endl;
    ffile2 << endl;
  }

  close(ffile);
  close(ffile2);

  /* instruction and BBL listing */
  ofstream ifile; open_and_check(basename, ".instructions", ifile);
  ifile << "# ADDRESS:BBL_ID:FUNCTION_ID:EXECUTED" << endl;

  ofstream bfile; open_and_check(basename, ".bbls", bfile);
  bfile << "# BBL_ID:ADDRESS:NINS:FUNCTION_ID" << endl;

  already_printed = false;
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_IS_HELL(bbl)
        || BblIsExitBlock(bbl))
      continue;

    if (IS_DATABBL(bbl)
        || BBL_ATTRIB(bbl) & BBL_DATA_POOL)
      continue;

    /* these blocks should have been removed by dead code elimination.
     * However, a block stays in the CFG sometimes without any predecessors. */
    if (!BBL_PRED_FIRST(bbl)
        || !BBL_SUCC_FIRST(bbl))
      continue;

    /* global redirected dummy entries for AF */
    if (BBL_NINS(bbl) == 1
        && CFG_EDGE_CAT(BBL_PRED_FIRST(bbl)) == ET_SWITCH
        && CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) == ET_JUMP
        && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) == NULL
        && CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) == NULL) {
      t_bool has_ipswitch = FALSE;

      t_cfg_edge *e;
      BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(BBL_PRED_FIRST(bbl)), e)
        if (CFG_EDGE_CAT(e) == ET_IPSWITCH) {
          has_ipswitch = TRUE;
          break;
        }

      if (has_ipswitch)
        continue;
    }

    BblExecCountSanityCheck(bbl);

    ASSERT(BBL_OBJECT_SET(bbl) != ObjectSetUID_INVALID, ("bbl does not have a valid object set @eiB", bbl));
    ASSERT(BBL_ORIGINAL_FUNCTION(bbl) != FunctionUID_INVALID, ("bbl does not have a valid original function @eiB", bbl));
    ASSERT(BBL_FUNCTION(bbl), ("bbl not in function @eiB", bbl));

    /* BBL UID */
    bfile << BBL_ID(bbl);
    /* BBL start address */
    bfile << ":" << "0x" << setbase(16) << BBL_CADDRESS(bbl) << setbase(10);
    /* BBL number of instructions */
    bfile << ":" << BBL_NINS(bbl);
    /* function UID */
    bfile << ":" << FUNCTION_ID(BBL_FUNCTION(bbl)) << endl;

    t_ins *ins;
    BBL_FOREACH_INS(bbl, ins)
    {
      /* instruction address */
      auto insnstr = StringIo("@G", INS_CADDRESS(ins));
      ifile << insnstr;
      Free(insnstr);
      /* instruction BBL ID */
      ifile << ":" << BBL_ID(bbl);
      /* instruction function ID, for ease of processing with IDA results must be placed here */
      ifile << ":" << FUNCTION_ID(BBL_FUNCTION(bbl));
      /* is the instruction executed or not? */
      ifile << ":" << (BBL_EXEC_COUNT(bbl) > 0);
      /* is the instruction Thumb or not? */
      ifile << ":" << 0;
      /* is the instruction a NOP or not? */
      ifile << ":" << 0;
      /* is the instruction a macro instruction or not? */
      ifile << ":" << 0;

      ifile << endl;
    }
  }

  close(ifile);
  close(bfile);

  /* archive listing */
  ofstream afile; open_and_check(basename, ".archives", afile);
  afile << "# ARCHIVE_ID:FILENAME" << endl;

  for (size_t i = 0; i < all_archives.size(); i++) {
    /* archive UID */
    afile << i;
    /* archive path and name */
    afile << ":" << all_archives[i] << endl;
  }

  close(afile);

  /* object file listing */
  ofstream ofile; open_and_check(basename, ".objectfiles", ofile);
  ofile << "# OBJECT_ID:FILENAME" << endl;

  for (size_t i = 0; i < all_filenames.size(); i++) {
    /* object file UID */
    ofile << i;
    /* object file path and name */
    ofile << ":" << all_filenames[i] << endl;
  }

  close(ofile);

  /* set listing */
  ofstream sfile; open_and_check(basename, ".partitions", sfile);
  sfile << "# PARTITION_ID:SCC_UID:ASSOCIATED_FUNCTIONS:ASSOCIATED_OBJECTS:ASSOCIATED_LIBRARIES" << endl;

  CFG_FOREACH_FUN(cfg, fun) {
    if (FUNCTION_IS_HELL(fun))
      continue;

    /* set UID */
    sfile << FUNCTION_PARTITION_INFO(fun)->id;

    /* SCC UID */
    sfile << ":" << FunctionGetSccUID(fun);

    /* associated-with */
    TrackingInformation associated_with = CalculateAssociatedWith(FUNCTION_BBL_FIRST(fun));
    sfile << ":" << SetToString(associated_with.functions);
    sfile << ":" << SetToString(associated_with.files);
    sfile << ":" << SetToString(associated_with.archives);

    sfile << endl;
  }

  close(sfile);

  /* SCC: this information is separate because many functions can be grouped together into one SCC */
  ofstream sccfile; open_and_check(basename, ".sccs", sccfile);
  sccfile << "# SCC_ID:REACHABLE_FUNCTIONS:REACHABLE_OBJECTS:REACHABLE_LIBRARIES" << endl;

  for (auto scc_ptr : StronglyConnectedComponent::all_instances) {
    sccfile << scc_ptr->uid;

    /* reachable-by */
    set<SourceArchiveUID> archives;
    set<SourceFileUID> files;
    ObjectListToArchivesObjectFiles(FunctionsToObjects(scc_ptr->reachable_by_functions), archives, files);

    sccfile << ":" << SetToString(scc_ptr->reachable_by_functions);
    sccfile << ":" << SetToString(files);
    sccfile << ":" << SetToString(archives);

    sccfile << endl;
  }

  close(sccfile);

  /* complexity */
  string x = string(basename) + ".stat_complexity_info";
  CfgStaticComplexityOriginInit(x.c_str());

  string y = string(basename) + ".dynamic_complexity_info";
  CfgDynamicComplexityOriginInit(y.c_str());

  CfgComputeStaticComplexityOrigin(cfg, all_archives.size(), all_filenames.size(), original_functions.size());
  CfgComputeDynamicComplexityOrigin(cfg, all_archives.size(), all_filenames.size(), original_functions.size(), function_to_bbl);

  CfgStaticComplexityOriginFini();
  CfgDynamicComplexityOriginFini();

  {
    VERBOSE(0,("NON-ORIGIN PROGRAM COMPLEXITY REPORT"));

    string x = string(basename) + ".stat_complexity_info.non-origin";
    CfgStaticComplexityInit(x.c_str());

    string y = string(basename) + ".dynamic_complexity_info.non-origin";
    CfgDynamicComplexityInit(y.c_str());

    CfgComputeStaticComplexity(cfg);
    CfgComputeDynamicComplexity(cfg);

    CfgStaticComplexityFini();
    CfgDynamicComplexityFini();
  }
}

void TrackOriginInformation(t_cfg *cfg, t_string directory) {
  if (cfg != init_cfg
      || disable_origin_tracking)
    return;

  STATUS(START, ("Tracking origin information"));

  /* reset */
  tracking_activated = false;
  PartitionInfo::Reset();
  StronglyConnectedComponent::Reset();
  tracking_cache.Init();
  info_cache.Init();

  /* repartition */
  tracking_activated = true;
  CalculateSCCsAndReachableBy(cfg, directory);

  /* datastructures */
  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun) {
    if (!FUNCTION_PARTITION_INFO(fun))
      continue;

    /* calculate object file, needed for quick lookups in AF */
    ObjectUID object_file = ObjectFileForBbl(FUNCTION_PARTITION_INFO(fun)->bbl);

    auto reachable_by_functions = FunctionGetReachableByFunctions(fun);
    if (object_file == ObjectUID_INVALID && reachable_by_functions.size() == 1) {
      FunctionUID function_uid = *(reachable_by_functions.begin());

      if (original_functions.find(function_uid) != original_functions.end())
        object_file = original_functions[function_uid];
    }
    FUNCTION_PARTITION_INFO(fun)->object_file = object_file;

    /* set BBL 'set UID' for every BBL inside the function */
    auto id = FUNCTION_PARTITION_INFO(fun)->id;

    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(fun, bbl)
      BBL_SET_OBJECT_SET(bbl, id);
  }

  /* final fixups: empty return sites need to inherit their object set ID from the call site */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (BBL_IS_HELL(bbl) || IS_DATABBL(bbl) || BblIsExitBlock(bbl) || BBL_ATTRIB(bbl) & BBL_DATA_POOL)
      continue;

    if (BBL_OBJECT_SET(bbl) == ObjectSetUID_INVALID) {
      if (BBL_PRED_FIRST(bbl)
          && CFG_EDGE_CAT(BBL_PRED_FIRST(bbl)) == ET_RETURN
          && CFG_EDGE_CORR(BBL_PRED_FIRST(bbl))
          && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) == NULL)
        BBL_SET_OBJECT_SET(bbl, BBL_OBJECT_SET(CFG_EDGE_HEAD(CFG_EDGE_CORR(BBL_PRED_FIRST(bbl)))));
    }
  }

  if (!only_do_scc_information) {
    CalculateReachableAssociatedNumbers(cfg, directory);
    EmitOriginTrackingInformation(cfg, directory);
  }

  info_cache.Report("Object set information");
  tracking_cache.Report("Tracking information");

  STATUS(STOP, ("Tracking origin information"));
}

static
t_object *GetSubobjectAndRangeContainingAddress(t_object *obj, t_address addr, t_address& begin, t_address& end)
{
  t_object *subobj, *tmpobj;
  t_section *section;

  OBJECT_FOREACH_SUBOBJECT(obj,subobj,tmpobj)
    if((section = ObjectGetSectionContainingAddress (subobj, addr)) != NULL)
    {
      if (SECTION_TYPE(section) != CODE_SECTION)
        continue;

      begin = SECTION_OLD_ADDRESS(section);
      end = AddressAdd(begin, SECTION_OLD_SIZE(section));

      return subobj;
    }

  return NULL;
}

void InitialiseObjectFileTracking(t_cfg *cfg)
{
  init_cfg = cfg;

  t_address begin = AddressNullForObject(CFG_OBJECT(cfg));
  t_address end = begin;

  FixOriginalFunctionInformation(cfg);

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_IS_HELL(bbl)
        || BblIsExitBlock(bbl))
      continue;

    t_address bbl_addr = BBL_OLD_ADDRESS(bbl);
    if (objects_and_ranges.size() == 0
        || !(AddressIsGe(bbl_addr, begin) && AddressIsLt(bbl_addr, end)))
    {
      t_object *obj = GetSubobjectAndRangeContainingAddress(CFG_OBJECT(cfg), bbl_addr, begin, end);
      if (AddressIsEq(begin, AddressNew32(0))) {
        VERBOSE(0, ("skipping object file starting at 0: %s", OBJECT_NAME(obj)));
        continue;
      }

      if (obj)
      {
        /* we assume that the object file name is written as:
         * <archive>:<object> */
#ifdef DEBUG_TRACKING
        VERBOSE(0, ("push \"%s\" @G-@G", OBJECT_NAME(obj), begin, end));
#endif

        auto find_or_append = [] (vector<string>& v, string o) {
          size_t index = 0;

          auto found = find(v.begin(), v.end(), o);
          if (found == v.end()) {
            index = v.size();
            v.push_back(o);

#ifdef DEBUG_TRACKING
            VERBOSE(0, ("added %d: %s", index, o.c_str()));
#endif
          }
          else
            index = distance(v.begin(), found);

          return index;
        };

        /* filename, including archive path */
        string filename = OBJECT_NAME(obj);
        ObjectUID filename_index = find_or_append(all_filenames, filename);

        /* archive name */
        string archive = "(null)";
        auto found = filename.find(":");
        if (found != string::npos)
          archive = filename.substr(0, found);
        ObjectUID archive_index = find_or_append(all_archives, archive);

#ifdef DEBUG_TRACKING
        VERBOSE(0, ("OBJECT[%d] %d:%d %s(%s)", objects_and_ranges.size(), archive_index, filename_index, archive.c_str(), filename.c_str()));
#endif
        objects_and_ranges.push_back(ObjectAndRange{archive_index, filename_index, begin, end});
      }
    }
  }

  nr_instructions_in_file.resize(all_filenames.size());
  nr_instructions_in_archive.resize(all_archives.size());

  VERBOSE(0, ("found %d object files in %d archives ", all_filenames.size(), all_archives.size()));

  nr_instructions_in_function.resize(original_functions.size());

  /* count function specific information */
  t_function *f;
  CFG_FOREACH_FUN(cfg, f)
  {
    if (FUNCTION_IS_HELL(f))
      continue;

    /* get the entry BBL for this function and be sure that it is not created by Diablo */
    t_bbl *entry = FUNCTION_BBL_FIRST(f);

    ObjectUID object_idx = ObjectFileForBbl(entry);
    if (original_functions.find(FUNCTION_ID(f)) != original_functions.end())
      original_functions[FUNCTION_ID(f)] = object_idx;

    if (object_idx == ObjectUID_INVALID)
      continue;

    auto object_and_range = objects_and_ranges[object_idx];

    t_bbl *bbl;
    FUNCTION_FOREACH_BBL(f, bbl) {
      nr_instructions_in_file[object_and_range.filename_id] += BBL_NINS(bbl);
      nr_instructions_in_archive[object_and_range.archive_id] += BBL_NINS(bbl);

      FunctionUID function_uid = BBL_ORIGINAL_FUNCTION(bbl);
      if (function_uid != FunctionUID_INVALID)
        nr_instructions_in_function[BBL_ORIGINAL_FUNCTION(bbl)] += BBL_NINS(bbl);
    }
  }

  for (size_t idx = 0; idx < all_archives.size(); idx++)
    VERBOSE(0, ("archive_size:%d:%d", idx, nr_instructions_in_archive[idx]));
  for (size_t idx = 0; idx < all_filenames.size(); idx++)
    VERBOSE(0, ("object_size:%d:%d", idx, nr_instructions_in_file[idx]));
  for (size_t idx = 0; idx < nr_instructions_in_function.size(); idx++)
    VERBOSE(0, ("function_size:%d:%d", idx, nr_instructions_in_function[idx]));

  /* sanity check */
  ASSERT(all_archives.size() <= 8*sizeof(SourceArchiveBitset), ("%d archives do not fit in a bitset of size %d", all_archives.size(), 8*sizeof(SourceArchiveBitset)));

  FunctionInitPartitionInfo(cfg);
  DiabloBrokerCallInstall("IoModifierBblTracking", "t_bbl *, t_string_array *", (void*)IoModifierBblTracking, FALSE);

  TrackOriginInformation(cfg, const_cast<t_string>(ORIGIN_INITIAL_DIRECTORY));

  string s = "ORIGIN_TRACKING:";
  s += ORIGIN_INITIAL_DIRECTORY;
  LogKilledInstructionBarrier(const_cast<t_string>(s.c_str()));
}

void FinalizeObjectTracking(t_cfg *cfg) {
  tracking_activated = false;
  FunctionFiniPartitionInfo(cfg);
}

ObjectUID BblObjectIndex(t_bbl *bbl) {
  auto set_id = BBL_OBJECT_SET(bbl);

  /* sanity check */
  if (set_id == ObjectSetUID_INVALID) {
    CfgDrawFunctionGraphsWithHotness(BBL_CFG(bbl), "invalid_set");
    FATAL(("@eiB does not belong to a valid set!", bbl));
  }

  return PartitionInfo::all_instances[set_id]->object_file;
}

void BblSourceLocation(t_bbl *bbl, FunctionUID& function, SourceFileUID& file, SourceArchiveUID& archive) {
  ObjectUID object_uid = BblObjectIndex(bbl);

  /* associated functions */
  map<FunctionUID, t_uint64> _;
  auto functions = FunctionGetAssociatedWithFunctions(bbl, _);

  function = *(functions.begin());
  file = GetFileUID(object_uid);
  archive = GetArchiveUID(object_uid);
}

SourceArchiveUID GetArchiveUID(ObjectUID idx) {
  return objects_and_ranges[idx].archive_id;
}

std::string GetArchiveName(SourceArchiveUID idx) {
  return all_archives[idx];
}

void DisableOriginTracking() {
  disable_origin_tracking = true;
}

void OnlyDoSccInformation(bool x) {
  only_do_scc_information = x;
}

static vector<TrackingInformation *> created_assoc_structs;
void BblInitAssociatedInfo(t_cfg *cfg) {
  BblInitAssoc(cfg);

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    if (BBL_IS_HELL(bbl) || IS_DATABBL(bbl) || BblIsExitBlock(bbl) || BBL_ATTRIB(bbl) & BBL_DATA_POOL)
      continue;

    TrackingInformation assoc = CalculateAssociatedWith(bbl);

    TrackingInformation *new_assoc = new TrackingInformation();
    new_assoc->archives = assoc.archives;
    new_assoc->files = assoc.files;
    new_assoc->functions = assoc.functions;

    BBL_SET_ASSOC(bbl, new_assoc);
    created_assoc_structs.push_back(new_assoc);
  }
}

void BblFiniAssociatedInfo(t_cfg *cfg) {
  for (auto x : created_assoc_structs)
    delete x;
  created_assoc_structs.clear();

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
    BBL_SET_ASSOC(bbl, NULL);

  BblFiniAssoc(cfg);
}

TrackingInformation *BblAssociatedInfo(t_bbl *bbl) {
  return BBL_ASSOC(bbl);
}

void BblCopyAssociatedInfo(t_bbl *from, t_bbl *to) {
  BBL_SET_ASSOC(to, BBL_ASSOC(from));
}

SourceArchiveBitset SourceArchiveSetToBitset(SourceArchiveSet s) {
  SourceArchiveBitset result = 0;

  for (auto i : s)
    result |= 1<<i;

  return result;
}

