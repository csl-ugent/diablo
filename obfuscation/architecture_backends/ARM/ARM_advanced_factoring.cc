#include "ARM_obfuscations.h"
#include "ARM_schedule_instructions.h"
using namespace std;

#define T_DAG(x)        (x)
#define T_DAGNODE(x)    (static_cast<t_scheduler_dagnode*>(x))
#define DAGNODE_INS(x)  (T_DAGNODE(x)->ins)
#define DAG_SOURCE(x)   (T_DAG(x)->source)
#define DAG_SINK(x)     (T_DAG(x)->sink)
#define DAGEDGE_DEPENDENCY_TYPE(edge)   (EDGE_CAT(edge))

BBL_DYNAMIC_MEMBER_BODY(dag, DAG, Dag, t_scheduler_dag *, {*valp=NULL;}, {if (*valp) SchedulerDagFree(*valp);}, {*valp=NULL;});
BBL_DYNAMIC_MEMBER(slice_calculated, SLICE_CALCULATED, SliceCalculated, bool, false);

//#define DUMP_DAG
#define SLICE_BASE_EXCLUDE_SUCCESSORS
//#define SLICE_WHOLE_PATH

typedef int (*slice_id_fn)(t_ins *);
typedef void (*set_slice_id_fn)(t_ins *, int);
typedef int (*ins_order_fn)(t_ins *);
typedef t_uint16 (*get_ins_fingerprint_fn)(t_ins *);

static ARMScheduleInstructionsTransformation *tf;

static bool is_dag_init = false;

void FreeDagForBbl(t_bbl *bbl)
{
  if (BBL_DAG(bbl))
    SchedulerDagFree(BBL_DAG(bbl));

  BBL_SET_DAG(bbl, NULL);
}

void CleanupScheduler(t_cfg *cfg)
{
  BblFiniDag(cfg);
  BblFiniSliceCalculated(cfg);
}

bool ShouldFollowEdge(t_scheduler_dag *dag, t_edge *edge)
{
  /* don't follow from-source or to-sink edges */
  if (EDGE_HEAD(edge) == DAG_SOURCE(dag)
      || EDGE_TAIL(edge) == DAG_SINK(dag))
    return false;

#ifdef SLICE_WHOLE_PATH
  return true;

#else
  /* result depends on the type of edge */
  switch (DAGEDGE_DEPENDENCY_TYPE(edge))
  {
    case DEP_RAW:
      return true;

    default:;
  }

  return false;
#endif
}

#define NODE_SHOULD_MARK(node) (slice_id(DAGNODE_INS(node)) == -2)
bool WorklistSliceId(t_scheduler_dag *dag, slice_id_fn slice_id, set_slice_id_fn set_slice_id, int current_slice_id, t_ins *base = NULL)
{
  bool result = false;
  vector<t_node *> worklist;

  /* look for a node to construct a slice for */
  t_node *node;
  GRAPH_FOREACH_NODE(T_GRAPH(dag), node)
    if (node != DAG_SOURCE(dag)
        && node != DAG_SINK(dag)
        && NODE_SHOULD_MARK(node))
    {
      worklist.push_back(node);
      break;
    }

  while (worklist.size() > 0)
  {
    t_edge *edge;

    /* assume the last node in the list should be marked still */
    t_node *node = worklist.back();
    worklist.pop_back();

    /* set slice id for this node */
    set_slice_id(DAGNODE_INS(node), current_slice_id);
    result = true;

    /* process predecessor edges */
    NODE_FOREACH_PRED_EDGE(node, edge)
    {
      t_node *head = EDGE_HEAD(edge);

      if (ShouldFollowEdge(dag, edge)
          && (base || (!base && slice_id(DAGNODE_INS(head)) != -1))
          && slice_id(DAGNODE_INS(head)) != current_slice_id)
      {
        ASSERT(base || (!base && NODE_SHOULD_MARK(head)), ("what? @I", DAGNODE_INS(head)));
        worklist.push_back(head);
      }
    }

#ifdef SLICE_BASE_EXCLUDE_SUCCESSORS
    if (!base)
#endif
    {
      /* process successor edges */
      NODE_FOREACH_SUCC_EDGE(node, edge)
      {
        t_node *tail = EDGE_TAIL(edge);

        if (ShouldFollowEdge(dag, edge)
            && (base || (!base && slice_id(DAGNODE_INS(tail)) != -1))
            && slice_id(DAGNODE_INS(tail)) != current_slice_id)
        {
          ASSERT(base || (!base && NODE_SHOULD_MARK(tail)), ("what? @I", DAGNODE_INS(tail)));
          worklist.push_back(tail);
        }
      }
    }

    /* remove marked nodes at the end of the list */
    while (worklist.size() > 0
            && (!base && !NODE_SHOULD_MARK(worklist.back())))
      worklist.pop_back();
  }

  return result;
}

void helper_create_dag_for_bbl(t_bbl *bbl) {
  static bool dag_init = false;

  /* initialise dynamic member if necessary */
  if (!dag_init)
  {
    tf = static_cast<ARMScheduleInstructionsTransformation *>(GetTransformationsForType("scheduleinstructions")[0]);

    BblInitDag(BBL_CFG(bbl));
    BblInitSliceCalculated(BBL_CFG(bbl));
    dag_init = true;

    DiabloBrokerCallInstall("CleanupAFScheduler", "t_cfg *", (void *)CleanupScheduler, FALSE);
  }

  if (!BBL_DAG(bbl))
  {
    /* construct the DAG */
    BBL_SET_DAG(bbl, tf->SchedulerDagBuildForBbl(bbl, RegsetNew(), true));

#ifdef DUMP_DAG
    /* dump the DAG to a dot file if requested */
    DirMake ("dags", FALSE);
    t_string filename = StringIo("dags/@G.dot", BBL_OLD_ADDRESS(bbl));
    tf->SchedulerDagDump(BBL_DAG(bbl), filename);
    Free(filename);
#endif
  }
}

void MarkSlicesInBbl(t_bbl *bbl, slice_id_fn slice_id, set_slice_id_fn set_slice_id) {
  helper_create_dag_for_bbl(bbl);

  int current_id = 0;
  while (WorklistSliceId(BBL_DAG(bbl), slice_id, set_slice_id, current_id))
    current_id++;
}

void MarkSliceForIns(t_ins *ins, slice_id_fn slice_id, set_slice_id_fn set_slice_id) {
  helper_create_dag_for_bbl(INS_BBL(ins));

  WorklistSliceId(BBL_DAG(INS_BBL(ins)), slice_id, set_slice_id, 0, ins);
}

/* output: given an instruction with slice id x:
 *  instructions with slice id y>x shall be scheduled AFTER the slice,
 *  instructions with slice id y<x shall be scheduled BEFORE the slice. */
void RescheduleBblForSlice(t_bbl *bbl, set_slice_id_fn set_slice_id) {
  /* do we need to rechedule this BBL? */
  t_ins *ins;
  helper_create_dag_for_bbl(bbl);

  if (!BBL_SLICE_CALCULATED(bbl))
  {
    //DEBUG(("looking for slices in @iB", bbl));
    tf->doSliceTransform(bbl, BBL_DAG(bbl), set_slice_id);
    BBL_SET_SLICE_CALCULATED(bbl, true);
  }
}

void RescheduleBblForSequence(t_bbl *bbl, ins_order_fn get_ins_order) {
  /* create sorted map of instructions, sorted by old address */
  map<int, t_ins *> ins_map;

  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins)
    ins_map[get_ins_order(ins)] = ins;

  t_ins *current = NULL;
  for (auto pair : ins_map) {
    t_ins *ins = pair.second;

    if (current)
      BblMoveInstructionAfter(ins, current);

    current = ins;
  }
}

void CanonicalizeBbl(t_bbl *bbl, get_ins_fingerprint_fn get_fingerprint) {
  helper_create_dag_for_bbl(bbl);

  tf->doCanonicalizeTransform(bbl, BBL_DAG(bbl), get_fingerprint);
}
