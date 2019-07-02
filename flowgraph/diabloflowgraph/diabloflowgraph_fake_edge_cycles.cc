#include "diabloflowgraph.hpp"

#include <list>
#include <sstream>

using namespace std;

#define MAX_ITERATIONS 10

typedef t_uint32 FakeEdgeCycleUID;
static const FakeEdgeCycleUID INVALID_FAKE_EDGE_CYCLE_UID = -1;

struct TransformationMetadata {
  TransformationID tf_id;
  SourceArchiveBitset covered_archives;
  bool is_final;
  BblSet bbls;
  CfgEdgeSet edges_to_redirect;
  FakeEdgeCycleUID fake_edge_cycle_uid;
  TransformationID next_tf_id;

  TransformationMetadata() {
    tf_id = INVALID_TRANSFORMATION_ID;
    covered_archives = SourceArchiveBitset();
    is_final = true;
    bbls = BblSet();
    edges_to_redirect = CfgEdgeSet();
    fake_edge_cycle_uid = INVALID_FAKE_EDGE_CYCLE_UID;
    next_tf_id = INVALID_TRANSFORMATION_ID;
  }
};

struct EdgeCycleData {
  vector<TransformationID> transformations;
  FakeEdgeCycleUID uid;
};

static size_t max_cycle_size = 0;
static vector<TransformationMetadata *> transformation_metadata;
static vector<EdgeCycleData *> edge_cycle_data;

static
void CollectTransformationInformation(t_cfg *cfg, t_bbl *hell) {
  STATUS(START, ("collect transformation information"));

  /* first we construct a _set_ of transformation IDs
   * because we want to have each ID only once in the final list. */
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl) {
    /* transformation ID for this BBL */
    auto tf_id = BblTransformationID(bbl);
    if (tf_id == INVALID_TRANSFORMATION_ID)
      continue;

    if (transformation_metadata[tf_id] == nullptr) {
      transformation_metadata[tf_id] = new TransformationMetadata();
      transformation_metadata[tf_id]->tf_id = tf_id;
    }

    /* collect bbls per transformation */
    transformation_metadata[tf_id]->bbls.insert(bbl);

    /* collect fake edges per transformation */
    t_cfg_edge *edge;
    BBL_FOREACH_SUCC_EDGE(bbl, edge)
      if (CFG_EDGE_TAIL(edge) == hell)
        transformation_metadata[tf_id]->edges_to_redirect.insert(edge);

    if (IS_DATABBL(bbl))
      continue;

    auto assoc = BblAssociatedInfo(bbl);
    ASSERT(assoc != NULL, ("no associated info for @eiB", bbl));
    transformation_metadata[tf_id]->covered_archives |= SourceArchiveSetToBitset(assoc->archives);
  }

  size_t nr_final = 0;
  size_t x = 0;
  for (auto i : transformation_metadata) {
    /* every transformation should have been set */
    if (i == nullptr)
      continue;

    if (i->edges_to_redirect.size() > 0)
      i->is_final = false;
    else
      nr_final++;

    x++;
  }

  VERBOSE(0, ("fake edge clusters: %d transformations are final", nr_final));
  STATUS(STOP, ("collect transformation information"));
}

static
void CreateCycles(list<TransformationMetadata *>& transformations) {
  STATUS(START, ("create cycles"));

  /* helper function */
  auto loop_iterator_next = [&transformations] (list<TransformationMetadata *>::iterator it) {
    auto result = it;
    advance(result, 1);

    /* go back to the beginning if the end is reached */
    if (result == transformations.end())
      result = transformations.begin();

    return result;
  };

  /* create cycles by uid */
  FakeEdgeCycleUID next_edge_cycle_uid = 0;

  for (auto it = transformations.begin(); it != transformations.end(); it++) {
    if ((*it)->fake_edge_cycle_uid == INVALID_FAKE_EDGE_CYCLE_UID) {
      (*it)->fake_edge_cycle_uid = next_edge_cycle_uid;

      if (next_edge_cycle_uid >= edge_cycle_data.size()) {
        ASSERT(next_edge_cycle_uid == edge_cycle_data.back()->uid + 1, ("what?"));
        edge_cycle_data.push_back(nullptr);
      }

      ASSERT(edge_cycle_data[next_edge_cycle_uid] == nullptr, ("what?"));
      edge_cycle_data[next_edge_cycle_uid] = new EdgeCycleData();
      edge_cycle_data[next_edge_cycle_uid]->transformations.push_back((*it)->tf_id);
      edge_cycle_data[next_edge_cycle_uid]->uid = next_edge_cycle_uid;

      next_edge_cycle_uid++;
    }
    else {
      /* check if the cycle has reached its max size */
      if (edge_cycle_data[(*it)->fake_edge_cycle_uid]->transformations.size() == max_cycle_size)
        continue;
    }

    if (max_cycle_size > 1) {
      /* look for successor, starting with the next element */
      auto itt = loop_iterator_next(it);

      /* take the 'best' candidate out of the next 10 items in the list */
      auto best = it;
      size_t best_result = 0;

      int iteration_nr = 0;
      while (iteration_nr < MAX_ITERATIONS) {
        /* an item that has already been put in a cycle should be skipped
        * without incrementing the iteration counter */
        if ((*itt)->fake_edge_cycle_uid == INVALID_FAKE_EDGE_CYCLE_UID) {
          /* count the number of overlapping archives */
          size_t x = CountSetBits64((*it)->covered_archives & (*itt)->covered_archives);
          if ((best == it) || (x < best_result)) {
            best = itt;
            best_result = x;
          }

          /* early exit in case no overlaps are found,
          * as it can't get any better! */
          if (x == 0)
            break;

          iteration_nr++;
        }

        itt = loop_iterator_next(itt);

        /* in case we exhausted the entire list */
        if (itt == it)
          break;
      }

      /* in case we found a good candidate */
      if (best != it) {
        /* add it to the cycle */
        (*best)->fake_edge_cycle_uid = (*it)->fake_edge_cycle_uid;

        edge_cycle_data[(*it)->fake_edge_cycle_uid]->transformations.push_back((*best)->tf_id);
      }
    }
  }

  /* statistics */
  t_string output_basename = OutputFilename();
  string filename = string(output_basename) + ".fake-cycles-groups";
  Free(output_basename);

  LogFile *L_FAKE_CLUSTER_GROUPS = NULL;
  INIT_LOGGING(L_FAKE_CLUSTER_GROUPS, filename.c_str());

  size_t total_cycle_size = 0;
  size_t max_size = 0;
  size_t min_size = 99999;
  for (auto x : edge_cycle_data) {
    if (x == nullptr)
      break;

    size_t s = x->transformations.size();
    total_cycle_size += s;

    if (max_size < s)
      max_size = s;
    if (s < min_size)
      min_size = s;

    /* */
    LOG_MESSAGE(L_FAKE_CLUSTER_GROUPS, "%d", x->uid);
    stringstream ss;
    for (auto tf_nr : x->transformations) {
      LOG_MESSAGE(L_FAKE_CLUSTER_GROUPS, ",%d", tf_nr);
      ss << tf_nr << ",";
    }
    LOG_MESSAGE(L_FAKE_CLUSTER_GROUPS, "\n");
  }
  VERBOSE(0, ("created %d edge cycles with average size of %.3f (min %d, max %d)", next_edge_cycle_uid, static_cast<double>(total_cycle_size)/next_edge_cycle_uid, min_size, max_size));

  FINI_LOGGING(L_FAKE_CLUSTER_GROUPS);

  STATUS(STOP, ("create cycles"));
}

static
void RedirectCycles(t_randomnumbergenerator *rng) {
  LogFile *L_LOG = NULL;
  t_string output_basename = OutputFilename();
  INIT_LOGGING(L_LOG, (string(output_basename) + ".fake-cycles-first").c_str());
  Free(output_basename);

  STATUS(START, ("redirecting to cycles"));

  for (auto cycle : edge_cycle_data) {
    if (cycle == nullptr)
      break;

    LOG_MESSAGE(L_LOG, "%d", cycle->uid);

    if (RNGGeneratePercent(rng) <= static_cast<t_uint32>(diabloflowgraph_new_target_selector_options.fake_edge_cluster_chance)) {
      for (auto it = cycle->transformations.begin(); it != cycle->transformations.end(); it++) {
        /* source */
        TransformationID tf_id_from = *it;

        /* destination */
        auto itt = it;
        advance(itt, 1);
        if (itt == cycle->transformations.end()) {
          /* creating a back edge */
          if (RNGGeneratePercent(rng) > static_cast<t_uint32>(diabloflowgraph_new_target_selector_options.fake_cluster_backedge_chance)) {
            LOG_MESSAGE(L_LOG, ",noback");
            break;
          }

          itt = cycle->transformations.begin();
        }
        TransformationID tf_id_to = *itt;

        /* pick a random fake edge to redirect */
        auto fake_edge_it = transformation_metadata[tf_id_from]->edges_to_redirect.begin();
        advance(fake_edge_it, RNGGenerateWithRange(rng, 0, transformation_metadata[tf_id_from]->edges_to_redirect.size() - 1));
        t_cfg_edge *fake_edge = *fake_edge_it;

        /* bookkeeping */
        transformation_metadata[tf_id_from]->edges_to_redirect.erase(fake_edge_it);

        /* pick a random target and redirect the fake edge */
        LOG_MESSAGE(L_LOG, ",%d-%d", tf_id_from, tf_id_to);
        redirect_fake_edge(fake_edge, rng, tf_id_to);
      }
    }
    else {
      /* don't do this cycle */
      LOG_MESSAGE(L_LOG, ",skip");
    }

    LOG_MESSAGE(L_LOG, "\n");
  }

  STATUS(STOP, ("redirecting to cycles"));
}

BblSet BblsForTransformationID(TransformationID tf_id, t_randomnumbergenerator *rng, std::set<TransformationID> *to_choose_from) {
  BblSet all = BblSet();

  if (transformation_metadata.size() > 0) {
    if (tf_id == -1) {
      ASSERT(rng != NULL, ("didn't get a RNG"));

      if (to_choose_from) {
        /* use the provided list */
        ASSERT(to_choose_from->size() > 0, ("no options given to choose from!"));

        auto it = to_choose_from->begin();
        advance(it, RNGGenerateWithRange(rng, 0, to_choose_from->size() - 1));

        tf_id = *it;
      }
      else {
        tf_id = RNGGenerateWithRange(rng, 0, transformation_metadata.size() - 1);

        if (transformation_metadata[tf_id] == nullptr) {
          vector<TransformationID> possible;
          for (auto i : transformation_metadata)
            if (i != nullptr)
              possible.push_back(i->tf_id);

          auto index = RNGGenerateWithRange(rng, 0, possible.size() - 1);
          tf_id = possible[index];
        }
      }
    }

    ASSERT(transformation_metadata[tf_id] != nullptr, ("what? %d", tf_id));
    all = transformation_metadata[tf_id]->bbls;
  }

  BblSet result;
  for (auto x : all)
    if (BblIsValidFakeTarget(x))
      result.insert(x);

  return result;
}

bool TransformationMetadataExists(TransformationID tf_id) {
  return transformation_metadata.size() > 0;
}

void UpdateTransformationMetadataAfterBblSplit(t_bbl *first, t_bbl *second) {
  /* no administration needs to be done */
  if (transformation_metadata.size() == 0)
    return;

  auto first_tf_id = BblTransformationID(first);
  auto second_tf_id = BblTransformationID(second);

  auto tf_id = (first_tf_id != INVALID_TRANSFORMATION_ID) ? first_tf_id : second_tf_id;
  if (tf_id == INVALID_TRANSFORMATION_ID)
    return;

  if (first_tf_id == INVALID_TRANSFORMATION_ID)
    transformation_metadata[tf_id]->bbls.erase(first);

  if (second_tf_id != INVALID_TRANSFORMATION_ID)
    transformation_metadata[tf_id]->bbls.insert(second);
}

void FakeEdgeCycles(t_cfg *cfg, t_bbl *hell, TransformationID max_tf_id, t_randomnumbergenerator *rng) {
  /* caching of command-line parameters */
  max_cycle_size = static_cast<size_t>(diabloflowgraph_new_target_selector_options.fake_edge_cluster_size);

  /* pre-allocate to avoid multiple realloc calls */
  transformation_metadata = vector<TransformationMetadata *>(max_tf_id + 1, nullptr);

  /* collect information */
  CollectTransformationInformation(cfg, hell);

  if (max_cycle_size > 0) {
    /* CFG cleanup and transformation count */
    VERBOSE(0, ("found %d transformations, will partition in approx. %d cycles", max_tf_id + 1, (max_tf_id + 1)/max_cycle_size));
    edge_cycle_data = vector<EdgeCycleData *>(((max_tf_id + 1) / max_cycle_size) + 1, nullptr);

    /* shuffle the vector */
    vector<TransformationMetadata *> shuffled = transformation_metadata;
    VERBOSE(0, ("fake edge cycles: random shuffle of %d transformations", shuffled.size()));
    shuffle(shuffled.begin(), shuffled.end(), RNGGetGenerator(rng));

    /* create a doubly linked list */
    list<TransformationMetadata *> transformations;
    for (auto i : shuffled) {
      if (i == nullptr)
        continue;

      if (i->is_final)
        continue;

      t_bool result = FALSE;
      DiabloBrokerCall("TransformationIsAF", i->tf_id, &result);
      if (result)
        continue;

      transformations.emplace_back(i);
    }

    /* create edge cycles */
    CreateCycles(transformations);

    /* redirect edges */
    RedirectCycles(rng);
  }
}
