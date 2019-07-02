/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 *                      Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *
 *
 * This file is part of Diablo (Diablo is a better link-time optimizer)
 *
 * Diablo is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Diablo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Diablo; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* }}} */
#include <obfuscation/obfuscation_architecture_backend.h>
#include "schedule_instructions.h"
using namespace std;

#define T_DAG(x)        (x)
#define DAG_BBL(x)      (T_DAG(x)->bbl)
#define DAG_SOURCE(x)   (T_DAG(x)->source)
#define DAG_SINK(x)     (T_DAG(x)->sink)
#define DAG_SCHEDULE(x) (T_DAG(x)->schedule)
#define DAG_READY(x)    (T_DAG(x)->ready)
#define DAG_FIRST(x)    (T_DAG(x)->first)
#define DAG_LAST(x)     (T_DAG(x)->last)
#define DAG_NR_SCHEDULED(x)     (T_DAG(x)->nr_scheduled)

#define T_DAGNODE(x)    (static_cast<t_scheduler_dagnode*>(x))
#define DAGNODE_INS(x)          (T_DAGNODE(x)->ins)
#define DAGNODE_DEPENDENCYCOUNT(x) (T_DAGNODE(x)->dependency_count)
#define DAGNODE_FLAGS(x)        (T_DAGNODE(x)->flags)
#define DAGNODE_CORR(x) (T_DAGNODE(x)->corr)
#define DAGNODE_NEXT(x) (T_DAGNODE(x)->next)
#define DAGNODE_IS_SINK(x)      (T_DAGNODE(x)->is_sink)
#define DAGNODE_IS_SOURCE(x)    (T_DAGNODE(x)->is_source)

struct t_scheduler_dagedge
{
  t_edge edge;
};

#define SCHED_SOURCE_DUMMY      0x4     /* A start dummy node, the unique source of the dag */
#define SCHED_SINK_DUMMY        0x8     /* A sink dummy node, the unique sink of the dag */

#define DAGNODE_IS_DUMMY(x)     (T_DAGNODE(x)->flags & (SCHED_SOURCE_DUMMY || SCHED_SINK_DUMMY))

#define T_DAGEDGE(x)    ((t_scheduler_dagedge*)x)
#define DAGEDGE_DEPENDENCY_TYPE(edge)   (/*(t_scheduler_dependency)*/EDGE_CAT(edge))
#define DAGEDGE_SET_DEPENDENCY_TYPE(edge, b)    (/*(t_scheduler_dependency)*/EDGE_SET_CAT(edge,b))

#define DE_INFINITY     (0xfffffff0)    /*< An infinite weight for scheduler_dagedges */

static t_scheduler_dag * SchedulerDagNew(t_bbl * bbl);

static t_scheduler_dagnode * SchedulerDagNodeNew(t_scheduler_dag * dag, t_ins *ins);
static t_scheduler_dagedge * SchedulerDagEdgeNew(t_scheduler_dag * dag, t_ins * head, t_ins * tail, t_scheduler_dependency dependency, bool reverse=false);
static t_scheduler_dagedge * SchedulerDagEdgeNewToSink(t_scheduler_dag * dag, t_ins * head, t_scheduler_dependency dependency, bool reverse=false);
static t_scheduler_dagedge * SchedulerDagEdgeNewFromSource(t_scheduler_dag * dag, t_ins * tail, t_scheduler_dependency dependency, bool reverse=false);

static t_scheduler_dag * SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set);
static void SchedulerApplySchedule(t_scheduler_dag * dag);
static void SchedulerDagScheduleNr(t_scheduler_dag * dag, t_uint32 nr);
static void SchedulerDagScheduleNrReverse(t_scheduler_dag * dag, t_uint32 nr);
static void SchedulerReverseSchedule(t_scheduler_dag * dag);

/*}}}*/

/* TODO: The original code explicitly set used / defined registers here before computing a trivial liveness. Why? Seems not needed... */
/* TODO Reasonably assumption: correct liveness information :-) */

ScheduleInstructionsTransformation::ScheduleInstructionsTransformation() {
  RegisterTransformationType(this, _name);
}

void ScheduleInstructionsTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("NO STATS YET FOR ScheduleInstructionsTransformation"));
}

bool ScheduleInstructionsTransformation::canTransform(const t_bbl* bbl) const {
  if (BBL_IS_HELL(bbl))
    return false;
  if (DisallowedFunctionToTransform(BBL_FUNCTION(bbl)))
    return false;

  return true;
}

/* DAG_READY contains nodes that can at this point be scheduled. If there is a choice, we can randomly pick one. These are then moved to the DAG_SCHEDULE
 * list, which contains the actual schedule we picked. Once the DAG_READY is empty, all nodes will have been transfered to the DAG_SCHEDULE, and we
 * can apply that (randomized) schedule to the BBL. */
bool ScheduleInstructionsTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  VERBOSE(1, ("BBL before scheduling: @eiB", bbl));

  t_scheduler_dag* current_dag = SchedulerDagBuildForBbl(bbl, NullRegs);

  while (!DAG_READY(current_dag).empty()) {
    /* If there is no choice, just apply this node in the schedule as is. The SchedulerDagScheduleNr updates the DAG_READY list. */
    if (DAG_READY(current_dag).size() == 1) {
      SchedulerDagScheduleNr(current_dag,0);
    } else {
      /* There is some choice, pick something at random :-) */
      int choice = RNGGenerateWithRange(rng, 0, DAG_READY(current_dag).size() - 1);
      SchedulerDagScheduleNr(current_dag, choice);
    }
  }

  /* Our schedule is completed, apply it! */
  SchedulerApplySchedule(current_dag);

  SchedulerDagFree(current_dag);

  VERBOSE(1, ("BBL after scheduling: @eiB", bbl));
  return true;
}

static t_ins *GetScheduleNr(t_scheduler_dag * dag, t_uint32 nr)
{
  auto it = DAG_READY(dag).begin();

  /* Take element number 'nr' from the list */
  for(t_uint32 i = 0; i < nr; i++, ++it)
    ;

  return DAGNODE_INS(*it);
}

static inline bool InsIsInSlice(t_ins *ins, int (*slice_id)(t_ins *))
{
  return slice_id(ins) == 0;
}

static t_ins *GetFirstReadyInSlice(t_scheduler_dag *dag, int (*slice_id)(t_ins *), int& nr)
{
  nr = 0;
  for (auto it = DAG_READY(dag).begin(); it != DAG_READY(dag).end(); it++, nr++)
    if (InsIsInSlice(DAGNODE_INS(*it), slice_id))
      return DAGNODE_INS(*it);

  nr = -1;
  return NULL;
}

static t_ins *GetFirstReadyNotInSlice(t_scheduler_dag *dag, int (*slice_id)(t_ins *), int& nr)
{
  nr = 0;
  for (auto it = DAG_READY(dag).begin(); it != DAG_READY(dag).end(); it++, nr++)
    if (!InsIsInSlice(DAGNODE_INS(*it), slice_id))
      return DAGNODE_INS(*it);

  nr = -1;
  return NULL;
}

//#define DEBUG_CANONICALIZE
void ScheduleInstructionsTransformation::doCanonicalizeTransform(t_bbl *bbl, t_scheduler_dag *dag, get_ins_fingerprint_fn fn)
{
  /* construct the DAG for the input BBL */
  t_scheduler_dag *current_dag = SchedulerDagBuildForBbl(bbl, NullRegs, true);

  while (!DAG_READY(current_dag).empty())
  {
    int selected_nr = 0;
    t_ins *selected_ins = nullptr;

    if (DAG_READY(current_dag).size() > 1)
    {
      /* multiple possibilities available, look for the smallest fingerprint */
      int nr = 0;
      for (auto it = DAG_READY(current_dag).begin(); it != DAG_READY(current_dag).end(); it++, nr++) {
        auto current_ins = DAGNODE_INS(*it);

        /* source/sink check */
        if (!current_ins)
          continue;

        /* first instruction */
        if (selected_ins == nullptr) {
          selected_nr = nr;
          selected_ins = current_ins;
          continue;
        }

        /* priority function: fingerprint a > fingerprint b */
        if (fn(current_ins) > fn(selected_ins)) {
          selected_nr = nr;
          selected_ins = current_ins;
        }
      }
    }

    /* we've scheduled in reverse */
    SchedulerDagScheduleNrReverse(current_dag, selected_nr);
  }

  SchedulerReverseSchedule(current_dag);

#ifdef DEBUG_CANONICALIZE
  vector<t_ins *> original_instructions;
  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins)
    original_instructions.push_back(ins);
#endif
  SchedulerApplySchedule(current_dag);
#ifdef DEBUG_CANONICALIZE
  size_t pos = 0;
  BBL_FOREACH_INS(bbl, ins) {
    if (original_instructions[pos] != ins) {
      DEBUG(("different! @iB", bbl));
      break;
    }
    pos++;
  }
#endif

  SchedulerDagFree(current_dag);
}

#define SLICETF_VERBOSITY_LEVEL 0
#define SLICETF_VERBOSITY_LEVEL2 SLICETF_VERBOSITY_LEVEL+1
void ScheduleInstructionsTransformation::doSliceTransform(t_bbl *bbl, t_scheduler_dag *dag, set_slice_id_fn set_slice_id)
{
  int global_slice_id = -1;

  /* construct the DAG for the input BBL */
  t_scheduler_dag *current_dag = SchedulerDagBuildForBbl(bbl, NullRegs, true);

  /* we aren't looking for any registers, initially */
  t_regset looking_for_regs = NullRegs;

  while (!DAG_READY(current_dag).empty())
  {
    int nr = 0;
    t_ins *ins = NULL;
    bool ins_in_slice = false;

    if (DAG_READY(current_dag).size() == 1)
    {
      /* only one possibility to schedule now */
      nr = 0;
      ins = GetScheduleNr(current_dag, nr);

      /* only check when not a source/sink node */
      if (ins)
      {
        /* check whether this instruction is part of the slice */
        t_regset regs = INS_REGS_USE(ins);
        RegsetSetIntersect(regs, looking_for_regs);

        t_reg r;
        REGSET_FOREACH_REG(regs, r)
        {
          ins_in_slice = true;
          break;
        }
      }
    }
    else
    {
      if (!RegsetEquals(looking_for_regs, NullRegs))
      {
        /* multiple possibilities available */
        for (auto it = DAG_READY(current_dag).begin(); it != DAG_READY(current_dag).end(); it++, nr++)
        {
          /* does this instruction define one of the registers we're looking for? */
          auto current_ins = DAGNODE_INS(*it);

          /* source/sink check */
          if (!current_ins)
            continue;

          t_regset regs = INS_REGS_DEF(current_ins);
          RegsetSetIntersect(regs, looking_for_regs);

          /* check whether at least one register defined by this instruction is also one we're looking for */
          t_reg r;
          REGSET_FOREACH_REG(regs, r)
          {
            ins = current_ins;
            ins_in_slice = true;
            break;
          }

          /* don't look further if we already found an instruction */
          if (ins) break;
        }
      }

      /* choose a random instruction in case none has been found already */
      if (!ins)
      {
        /* choose another instruction which is out of the current slice */
        nr = 0;
        ins = GetScheduleNr(current_dag, nr);
      }
    }

    if (ins_in_slice)
    {
      set_slice_id(ins, global_slice_id);
      RegsetSetDiff(looking_for_regs, INS_REGS_DEF(ins));
      RegsetSetUnion(looking_for_regs, INS_REGS_USE(ins));
    }
    else
    {
      if (ins)
      {
        global_slice_id++;
        set_slice_id(ins, global_slice_id);
        looking_for_regs = INS_REGS_USE(ins);
      }
    }

    //if (ins) DEBUG(("     scheduling @I", ins));

    /* we've scheduled in reverse */
    SchedulerDagScheduleNrReverse(current_dag, nr);
  }

  /*SchedulerReverseSchedule(current_dag);
  SchedulerApplySchedule(current_dag);*/
  SchedulerDagFree(current_dag);
}

static void SchedulerReverseSchedule(t_scheduler_dag * dag)
{
  reverse(DAG_SCHEDULE(dag).begin(), DAG_SCHEDULE(dag).end());
}
/* This function actually applies the selected schedule of this dag to the BBL */
static void SchedulerApplySchedule(t_scheduler_dag * dag)/*{{{*/
{
  t_ins * current_ins = NULL;

  for (t_scheduler_dagnode* item: DAG_SCHEDULE(dag)) {
    if(DAGNODE_INS(item) != NULL)
    {
      if(current_ins == NULL)
      {
        if(DAGNODE_INS(item)!=BBL_INS_FIRST(DAG_BBL(dag)))
        {
          BblMoveInstructionBefore(DAGNODE_INS(item), BBL_INS_FIRST(DAG_BBL(dag)));
        }
      }
      else
      {
        if(DAGNODE_INS(item)!=current_ins)
        {
          BblMoveInstructionAfter(DAGNODE_INS(item), current_ins);
        }
      }

      current_ins = DAGNODE_INS(item);
    }
  }
}/*}}}*/

void ScheduleInstructionsTransformation::SchedulerDagDump(t_scheduler_dag *dag, string filename)
{
  ofstream fout(filename);
  ASSERT(fout.is_open(), ("could not open file %s to dump the scheduler DAG in!", filename.c_str()));

  fout << "digraph \"" << DAG_BBL(dag) << "\" {" << endl;
  fout << "\tnode [shape=box];" << endl;

  t_node *node;
  GRAPH_FOREACH_NODE(T_GRAPH(dag), node)
  {
    /* 1. print the node itself */
    fout << "\t\"" << node << "\" [";

    /* construct the label for this node */
    t_string label = NULL;
    if (DAGNODE_INS(node))
      label = StringIo("@I", DAGNODE_INS(node));
    else if (T_DAGNODE(node) == DAG_SOURCE(dag))
      label = StringIo("source\n@B", DAG_BBL(dag));
    else if (T_DAGNODE(node) == DAG_SINK(dag))
      label = StringDup("sink");
    else
      FATAL(("what?"));
    fout << "label=\"" << label << "\"";
    Free(label);

    fout << "];" << endl;

    /* 2. print the successor edges */
    t_edge *edge;
    NODE_FOREACH_SUCC_EDGE(node, edge)
    {
      /* Decide whether or not the edge needs to be printed.
       * Take for example the following situation:
       *
       * XXX ---------+
       *  | (i_edge)  |
       * YYY          | (edge)
       *  | (j_edge)  |
       * ZZZ ---------+
       *
       * ... where the dependency flow is from top to bottom.
       * In this case, the right edge ('edge') should not be drawn,
       * as a dependency is already defined by the middle block via 'i_edge' followed by 'j_edge'. */
      bool dont_print = FALSE;

      t_edge *i_edge;
      NODE_FOREACH_SUCC_EDGE(node, i_edge)
      {
        if (edge == i_edge) continue;

        t_edge *j_edge;
        NODE_FOREACH_SUCC_EDGE(EDGE_TAIL(i_edge), j_edge)
        {
          if (EDGE_TAIL(j_edge) == EDGE_TAIL(edge))
          {
            dont_print = TRUE;
            break;
          }
        }

        if (dont_print) break;
      }

      /*if (dont_print)
        continue;*/

      /* determine the color and style for this edge,
       * depending on the dependency type */
      string color, style;
      switch(DAGEDGE_DEPENDENCY_TYPE(edge))
      {
      case DEP_RAR:
        color = "gray";
        style = "solid";
        break;

      case DEP_RAW:
        color = "blue";
        style = "solid";
        break;

      case DEP_WAR:
        color = "green";
        style = "solid";
        break;

      case DEP_WAW:
        color = "red";
        style = "solid";
        break;

      default:
        FATAL(("unknown edge type %d", DAGEDGE_DEPENDENCY_TYPE(edge)));
      }

      fout << "\t\"" << node << "\" -> \"" << EDGE_TAIL(edge) << "\" [";
      fout << "color=" << color << ",";
      fout << "style=" << style;
      fout << "];" << endl;
    }
  }

  fout << "}" << endl;

  fout.close();
}

#define WARNING_LEVEL 10
t_scheduler_dag*  ScheduleInstructionsTransformation::SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set, bool reverse)/*{{{*/
{
  t_ins* ins, *iter_ins;
  t_node * node;
  t_regset currUsed, tmpUsed;
  t_regset currDefs, tmpDefs;
  t_scheduler_dagedge * ret_edge;
  t_bool store_treated;
  t_regset live = BBL_REGS_LIVE_OUT(bbl);
  t_scheduler_dag * dag = SchedulerDagNew(bbl);
  t_architecture_description* desc = CFG_DESCRIPTION(BBL_CFG(bbl));

  /* create the dummy nodes */
  DAG_SOURCE(dag) = SchedulerDagNodeNew(dag,NULL);
  DAG_SINK(dag) = SchedulerDagNodeNew(dag,NULL);
  DAG_BBL(dag) = bbl;

  /* create a node in the DAG for every instruction in the BBL.
   * As this node is automagically added to the internal graph datastructure,
   * the return value of this function call need not be used. */
  BBL_FOREACH_INS(bbl,ins)
    SchedulerDagNodeNew(dag,ins);

  BBL_FOREACH_INS_R(bbl,ins)
  {
    store_treated = FALSE;

    tmpUsed = currUsed = INS_REGS_USE(ins);
    tmpDefs = currDefs = INS_REGS_DEF(ins);

    /* of the calculated defined registers, extract only
     * the registers that are actually used (live) further on. */
    RegsetSetIntersect(tmpDefs, live);

    if(!desc->InsIsConditional(ins))
      RegsetSetDiff(live,currDefs);

    /* backwards liveness analysis: live = live U ins_used */
    RegsetSetUnion(live,currUsed);

    /* Make sure that branches stay at the end of the bbl */
    /* TODO: Add edges for memory dependencies
     */
    if(CFG_DESCRIPTION(BBL_CFG(INS_BBL(ins)))->InsIsControlflow(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAR, reverse);
        /* Need an arch call to determine the weights */
        iter_ins = INS_IPREV(iter_ins);
      }

      iter_ins = INS_IPREV(ins);
      if (iter_ins) {
        /* on ARM, e.g.:
         * MOV r14, r15
         * SUB r15, r0, #0x1f */
        bool result = false;
        DiabloBrokerCall("ShouldKeepInsCombination", ins, iter_ins, &result);

        if (result) {
          /* yes, also the previous instruction should be kept in-order! */
          t_ins *dst = iter_ins;
          iter_ins = INS_IPREV(iter_ins);

          while (iter_ins)
          {
            ret_edge = SchedulerDagEdgeNew(dag, iter_ins, dst, DEP_RAR, reverse);
            iter_ins = INS_IPREV(iter_ins);
          }
        }
      }
    }

    iter_ins = INS_IPREV(ins);
    RegsetSetDiff(tmpUsed,exclude_set);
    while(iter_ins)
    {
      if(!RegsetIsMutualExclusive(tmpUsed,INS_REGS_DEF(iter_ins)))
      {
        /* Insert a RAW dependency, remove the defined regs from tmpSet, because we only have to
         * insert an edge once for a RAW*/
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAW, reverse);
        /* Need an arch call to determine the weights */
        RegsetSetDiff(tmpUsed,INS_REGS_DEF(iter_ins));
      }
      iter_ins = INS_IPREV(iter_ins);
      if(RegsetIsEmpty(tmpUsed)) break;
    }

    iter_ins = INS_IPREV(ins);
    RegsetSetDiff(tmpDefs,exclude_set);
    while(iter_ins)
    {
      if(!RegsetIsMutualExclusive(tmpDefs,INS_REGS_DEF(iter_ins)))
      {
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW, reverse);
        /* Need an arch call to determine the weights */
        //RegsetSetDiff(tmpDefs,INS_REGS_DEF(iter_ins));
      }
      if(!RegsetIsMutualExclusive(currDefs,INS_REGS_USE(iter_ins)))
      {
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAR, reverse);
        /* Need an arch call to determine the weights */
      }

      iter_ins = INS_IPREV(iter_ins);
    }

    /* Loads/Stores cannot be re-ordered with instructions that write the stack pointer (as they modify which addresses are available to read/write) */

    /* If this is a write instruction, add Write-After-* dependency edges */

    /* Here we need to take special care when the stack pointer is written to. For example, on ARM:
     *  PUSH  {...}
     *  ADD   r11, r13, #32
     *  SUB   r13, r13, #156
     *  ...
     *  STR   r1, [r11, #-40]
     *  ...
     * In theory, the instruction scheduler can schedule the SUB-instruction after the STR-instruction (data dependencies are respected).
     * However, when the following schedule is achieved:
     *  PUSH  {...}
     *  ADD   r11, r13, #32
     *  ...
     *  STR   r1, [r11, #-40]
     *  ...
     *  SUB   r13, r13, #156
     *  ...
     * And an obfuscation transformation (e.g., insertion of an opaque predicate) is applied between the STR and the SUB,
     * which assumes that the stack pointer is _always_ valid, the result will be that the value written by the STR instruction
     * can possibly be overwritten by the opaque predicate instructions (e.g., when a PUSH of two registers is inserted).
     *
     * The solution for this problem is to treat non-load/store instructions that modify the stack pointer as "allocation" instructions. */
    if(desc->InsIsStore(ins) || modifiesStackPointer(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        /* Write-After-Write */
        if(!store_treated
          && (desc->InsIsStore(iter_ins)
               || modifiesStackPointer(iter_ins) ) )
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW, reverse);
          store_treated = TRUE;
        }

        /* Write-After-Read */
        if(desc->InsIsLoad(iter_ins))
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAR, reverse);
        }
        iter_ins = INS_IPREV(iter_ins);
      }
    }

    /* If it's a load, add Read-After-Write dependencies if they exist */
    if(desc->InsIsLoad(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        if(desc->InsIsStore(iter_ins)
            || modifiesStackPointer(iter_ins) )
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAW, reverse);
          break;
        }
        iter_ins = INS_IPREV(iter_ins);
      }
    }

    /* If this instruction has side-effects, add dependencies to all instructions before AND after, because these instructions really should not be re-ordered,
     * as they can include memory barriers, endianness changes, etc. */
    if (hasSideEffects(ins)) {
      bool before = true;

      BBL_FOREACH_INS(bbl, iter_ins) {
        if (iter_ins == ins) {
          before = false;
          continue;
        }
        if (before)
          SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW, reverse);
        else
          SchedulerDagEdgeNew(dag,ins,iter_ins,DEP_WAW, reverse);
      }
    }
  }

  /* For node with no successor or predecessor, add edge to the source and sink */
  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(!NODE_SUCC_FIRST(node) && T_DAGNODE(node) != DAG_SINK(dag))
    {
      ret_edge = SchedulerDagEdgeNewToSink(dag,DAGNODE_INS(node),DEP_RAR, reverse);
    }
    if(!NODE_PRED_FIRST(node) && T_DAGNODE(node) != DAG_SOURCE(dag))
    {
      ret_edge = SchedulerDagEdgeNewFromSource(dag,DAGNODE_INS(node),DEP_RAR, reverse);
    }
  }

  DAG_READY(dag).clear();
  DAG_SCHEDULE(dag).clear();

  DAG_READY(dag).push_back((reverse) ? DAG_SINK(dag) : DAG_SOURCE(dag));
  return dag;
}/*}}}*/

/* Add a new node to the dag containing instruction ins */ /*{{{*/
static t_scheduler_dagnode * SchedulerDagNodeNew(t_scheduler_dag * dag, t_ins *ins)
{
  t_scheduler_dagnode * ret = T_DAGNODE(GraphNewNode(T_GRAPH(dag),0));

  DAGNODE_INS(ret) = ins;
  DAGNODE_FLAGS(ret) = 0;
  DAGNODE_DEPENDENCYCOUNT(ret) = 0;

  return ret;
}/*}}}*/

/* Add a new edge to the dag, from head to tail, with dependency given as arguments */ /*{{{*/
static t_scheduler_dagedge * SchedulerDagEdgeNew(t_scheduler_dag * dag, t_ins * head, t_ins * tail, t_scheduler_dependency dependency, bool reverse)
{
  t_scheduler_dagedge * ret;
  t_node * node;
  t_node * head_node = NULL, * tail_node = NULL;
  t_edge * i_edge;

  /* First check if the node are already in the dag */
  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(DAGNODE_INS(node) == head) head_node = node;
    if(DAGNODE_INS(node) == tail) tail_node = node;

    if(tail_node && head_node) break;
  }
  if(!head_node && !tail_node) FATAL(("Head or tail not yet in dag!"));

  NODE_FOREACH_SUCC_EDGE(head_node,i_edge)
    if(EDGE_TAIL(i_edge) == tail_node)
    {
      if(DAGEDGE_DEPENDENCY_TYPE(i_edge) > dependency)
        DAGEDGE_SET_DEPENDENCY_TYPE(i_edge, dependency) ;
      return T_DAGEDGE(i_edge);
    }

  /* Create the new edge */
  ret = T_DAGEDGE(GraphNewEdge(T_GRAPH(dag),head_node,tail_node,dependency));

  if (reverse)
    DAGNODE_DEPENDENCYCOUNT(head_node)++;
  else
    DAGNODE_DEPENDENCYCOUNT(tail_node)++;

  return ret;
}/*}}}*/

/* Add a new edge to the dag, from the source to tail, with dependency given as arguments */ /*{{{*/
static t_scheduler_dagedge * SchedulerDagEdgeNewFromSource(t_scheduler_dag * dag, t_ins * tail, t_scheduler_dependency dependency, bool reverse)
{
  t_scheduler_dagedge * ret;
  t_node * node;
  t_node * tail_node = NULL;
  t_edge * i_edge;

  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(DAGNODE_INS(node) == tail)
    {
      tail_node = node;
      break;
    }
  }
  if(!tail_node) FATAL(("Tail not yet in dag!"));
  if(!DAG_SOURCE(dag)) FATAL(("No source in dag yet!"));

  NODE_FOREACH_SUCC_EDGE(T_NODE(DAG_SOURCE(dag)),i_edge)
    if(EDGE_TAIL(i_edge) == tail_node)
    {
      if(DAGEDGE_DEPENDENCY_TYPE(i_edge) > dependency) DAGEDGE_SET_DEPENDENCY_TYPE(i_edge, dependency);
      return T_DAGEDGE(i_edge);
    }

  /* Create the new edge */
  ret = T_DAGEDGE(GraphNewEdge(T_GRAPH(dag),T_NODE(DAG_SOURCE(dag)),tail_node,dependency));

  if (reverse)
    DAGNODE_DEPENDENCYCOUNT(DAG_SOURCE(dag))++;
  else
    DAGNODE_DEPENDENCYCOUNT(tail_node)++;

  return ret;
}/*}}}*/

/* Add a new edge to the dag, from the head to the sink, with dependency given as arguments */ /*{{{*/
static t_scheduler_dagedge * SchedulerDagEdgeNewToSink(t_scheduler_dag * dag, t_ins * head, t_scheduler_dependency dependency, bool reverse)
{
  t_scheduler_dagedge * ret;
  t_node * node;
  t_node * head_node = NULL;
  t_edge * i_edge;

  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(DAGNODE_INS(node) == head)
    {
      head_node = node;
      break;
    }
  }
  if(!head_node) FATAL(("Tail not yet in dag!"));
  if(!DAG_SINK(dag)) FATAL(("No source in dag yet!"));

  NODE_FOREACH_SUCC_EDGE(head_node,i_edge)
    if(T_DAGNODE(EDGE_TAIL(i_edge)) == DAG_SINK(dag))
    {
      if(DAGEDGE_DEPENDENCY_TYPE(i_edge) > dependency) DAGEDGE_SET_DEPENDENCY_TYPE(i_edge, dependency);
      return T_DAGEDGE(i_edge);
    }

  /* Create the new edge */
  ret = T_DAGEDGE(GraphNewEdge(T_GRAPH(dag),head_node,T_NODE(DAG_SINK(dag)),dependency));

  if (reverse)
    DAGNODE_DEPENDENCYCOUNT(head_node)++;
  else
    DAGNODE_DEPENDENCYCOUNT(DAG_SINK(dag))++;

  return ret;
}/*}}}*/

static void SchedulerDagScheduleNr(t_scheduler_dag * dag, t_uint32 nr)/*{{{*/
{
  auto it = DAG_READY(dag).begin();
  t_node * node;

  /* Take element number 'nr' from the list */
  for(t_uint32 i = 0; i < nr; i++, ++it)
    ;

  t_scheduler_dagnode* to_schedule = *it;
  DAG_READY(dag).erase(it);

  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    t_edge * edge;
    if(node == to_schedule) {
      NODE_FOREACH_SUCC_EDGE(node, edge)
      {
        DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_TAIL(T_EDGE(edge))))--;
        if(DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_TAIL(T_EDGE(edge))))==0)
        {
          DAG_READY(dag).push_back(T_DAGNODE(EDGE_TAIL(T_EDGE(edge))));
        }
      }
    }
  }
  DAG_SCHEDULE(dag).push_back(to_schedule);
}/*}}}*/

static void SchedulerDagScheduleNrReverse(t_scheduler_dag * dag, t_uint32 nr)
{
  auto it = DAG_READY(dag).begin();
  t_node * node;

  for (t_uint32 i = 0; i < nr; i++, ++it)
    ;

  t_scheduler_dagnode* to_schedule = *it;
  DAG_READY(dag).erase(it);

  GRAPH_FOREACH_NODE(T_GRAPH(dag), node)
  {
    t_edge *edge;
    if (node == to_schedule) {
      NODE_FOREACH_PRED_EDGE(node, edge)
      {
        DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_HEAD(T_EDGE(edge))))--;
        if (DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_HEAD(T_EDGE(edge)))) == 0)
        {
          DAG_READY(dag).push_back(T_DAGNODE(EDGE_HEAD(T_EDGE(edge))));
        }
      }
    }
  }
  DAG_SCHEDULE(dag).push_back(to_schedule);
}

static t_scheduler_dag * SchedulerDagNew(t_bbl * bbl)/*{{{*/
{
  t_scheduler_dag * ret = new t_scheduler_dag();

  GRAPH_SET_NODE_SIZE(T_GRAPH(ret), sizeof(t_scheduler_dagnode));
  GRAPH_SET_EDGE_SIZE(T_GRAPH(ret), sizeof(t_scheduler_dagedge));

  DAG_BBL(ret) = bbl;
  DAG_FIRST(ret) = NULL;
  DAG_LAST(ret) = NULL;
  DAG_NR_SCHEDULED(ret) = 0;

  return ret;
}/*}}}*/

void SchedulerDagFree(t_scheduler_dag * dag)/*{{{*/
{
  t_node * node, * tmp_node;
  t_edge * edge, * tmp_edge;

  GRAPH_FOREACH_NODE_SAFE(T_GRAPH(dag),node,tmp_node)
    Free(node);
  GRAPH_FOREACH_EDGE_SAFE(T_GRAPH(dag),edge,tmp_edge)
    Free(edge);

  delete dag;
}/*}}}*/
