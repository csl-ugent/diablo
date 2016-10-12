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

#include <list>
#include <math.h>

extern "C" {
#include <diabloflowgraph.h>
}

#include "schedule_instructions.h"

using namespace std;

/*Type and Function definitions{{{*/
enum t_scheduler_dependency
{
  DEP_RAW = 0,
  DEP_WAW,
  DEP_ORDERED,
  DEP_WAR,
  DEP_WEAKLY_ORDERED,
  DEP_RAR
};

struct t_scheduler_dagnode;
typedef list<t_scheduler_dagnode*> t_scheduler_list;

struct t_scheduler_dag
{
  t_graph graph;
  
  t_bbl * bbl;

  /* Maybe add some specific fields here? */
  t_scheduler_dagnode * source;
  t_scheduler_dagnode * sink;
 
  /*The schedule under construction*/
  t_scheduler_list schedule;

  /*The nodes that are ready for scheduling*/
  t_scheduler_list ready;

  /*added for counting all possible schedules*/
  t_scheduler_dagnode * first;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * last;
  /*added for counting all possible schedules*/
  t_uint32 nr_scheduled;
};

#define T_DAG(x)        (x)
#define DAG_BBL(x)      (T_DAG(x)->bbl)
#define DAG_SOURCE(x)   (T_DAG(x)->source)
#define DAG_SINK(x)     (T_DAG(x)->sink)
#define DAG_SCHEDULE(x) (T_DAG(x)->schedule)
#define DAG_READY(x)    (T_DAG(x)->ready)
#define DAG_FIRST(x)    (T_DAG(x)->first)
#define DAG_LAST(x)     (T_DAG(x)->last)
#define DAG_NR_SCHEDULED(x)     (T_DAG(x)->nr_scheduled)

struct t_scheduler_dagnode : public t_node
{
  t_ins * ins;
  t_uint32 dependency_count; /* The node of predecessors that are unscheduled */
  t_uint32 flags;
  /* We should add the resources here, in an arch independend manner */
  
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * corr;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * next;
  /*added for counting all possible schedules*/
  t_bool is_sink;
  /*added for counting all possible schedules*/
  t_bool is_source;
};
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
static void SchedulerDagFree(t_scheduler_dag * dag);

static t_scheduler_dagnode * SchedulerDagNodeNew(t_scheduler_dag * dag, t_ins *ins);
static t_scheduler_dagedge * SchedulerDagEdgeNew(t_scheduler_dag * dag, t_ins * head, t_ins * tail, t_scheduler_dependency dependency);
static t_scheduler_dagedge * SchedulerDagEdgeNewToSink(t_scheduler_dag * dag, t_ins * head, t_scheduler_dependency dependency);
static t_scheduler_dagedge * SchedulerDagEdgeNewFromSource(t_scheduler_dag * dag, t_ins * tail, t_scheduler_dependency dependency);

static t_scheduler_dag * SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set);
static void SchedulerApplySchedule(t_scheduler_dag * dag);
static void SchedulerDagScheduleNr(t_scheduler_dag * dag, t_uint32 nr);

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

t_scheduler_dag*  ScheduleInstructionsTransformation::SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set)/*{{{*/
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

  BBL_FOREACH_INS(bbl,ins)
  {
    SchedulerDagNodeNew(dag,ins);
  }

  BBL_FOREACH_INS_R(bbl,ins)
  {
    store_treated = FALSE;

    tmpUsed = currUsed = INS_REGS_USE(ins);
    tmpDefs = currDefs = INS_REGS_DEF(ins);
    RegsetSetIntersect(tmpDefs, live);

    if(!desc->InsIsConditional(ins))
      RegsetSetDiff(live,currDefs);
    RegsetSetUnion(live,currUsed);

    /* Make sure that branches stay at the end of the bbl */
    /* TODO: Add edges for memory dependencies 
     */
    if(CFG_DESCRIPTION(BBL_CFG(INS_BBL(ins)))->InsIsControlflow(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAR);
        /* Need an arch call to determine the weights */
        iter_ins = INS_IPREV(iter_ins);
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
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAW);
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
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW);
        /* Need an arch call to determine the weights */
        //RegsetSetDiff(tmpDefs,INS_REGS_DEF(iter_ins));
      }
      if(!RegsetIsMutualExclusive(currDefs,INS_REGS_USE(iter_ins)))
      {
        ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAR);
        /* Need an arch call to determine the weights */
      }

      iter_ins = INS_IPREV(iter_ins);
    }

    /* Loads/Stores cannot be re-ordered with instructions that write the stack pointer (as they modify which addresses are available to read/write) */

    /* If this is a write instruction, add Write-After-* dependency edges */
    if(INS_TYPE(ins) == IT_STORE || INS_TYPE(ins) == IT_FLT_STORE || INS_TYPE(ins) == IT_STORE_MULTIPLE || desc->InsIsStore(ins) || modifiesStackPointer(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        /* Write-After-Write */
        if(!store_treated
          && (    INS_TYPE(iter_ins) == IT_STORE           || INS_TYPE(iter_ins) == IT_FLT_STORE
               || INS_TYPE(iter_ins) == IT_STORE_MULTIPLE  || desc->InsIsStore(iter_ins)
               || modifiesStackPointer(iter_ins) ) )
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW);
          store_treated = TRUE;
        }
        /* Write-After-Read */
        if(INS_TYPE(iter_ins) == IT_LOAD || INS_TYPE(iter_ins) == IT_FLT_LOAD || INS_TYPE(iter_ins) == IT_LOAD_MULTIPLE || desc->InsIsLoad(iter_ins))
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAR);
        }
        iter_ins = INS_IPREV(iter_ins);
      }
    }

    /* If it's a load, add Read-After-Write dependencies if they exist */
    if(INS_TYPE(ins) == IT_LOAD || INS_TYPE(ins) == IT_FLT_LOAD || INS_TYPE(ins) == IT_LOAD_MULTIPLE || desc->InsIsLoad(ins))
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
        if(    INS_TYPE(iter_ins) == IT_STORE          || INS_TYPE(iter_ins) == IT_FLT_STORE
            || INS_TYPE(iter_ins) == IT_STORE_MULTIPLE || desc->InsIsStore(iter_ins)
            || modifiesStackPointer(iter_ins) )
        {
          ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAW);
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
          SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW);
        else
          SchedulerDagEdgeNew(dag,ins,iter_ins,DEP_WAW);
      }
    }
  }

  /* For node with no successor or predecessor, add edge to the source and sink */
  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(!NODE_SUCC_FIRST(node) && T_DAGNODE(node) != DAG_SINK(dag))
    {
      ret_edge = SchedulerDagEdgeNewToSink(dag,DAGNODE_INS(node),DEP_RAR);
    }
    if(!NODE_PRED_FIRST(node) && T_DAGNODE(node) != DAG_SOURCE(dag))
    {
      ret_edge = SchedulerDagEdgeNewFromSource(dag,DAGNODE_INS(node),DEP_RAR);
    }
  }

  DAG_READY(dag).clear();
  DAG_SCHEDULE(dag).clear();

  DAG_READY(dag).push_back(DAG_SOURCE(dag));
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
static t_scheduler_dagedge * SchedulerDagEdgeNew(t_scheduler_dag * dag, t_ins * head, t_ins * tail, t_scheduler_dependency dependency)
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
  DAGNODE_DEPENDENCYCOUNT(tail_node)++;

  return ret;
}/*}}}*/

/* Add a new edge to the dag, from the source to tail, with dependency given as arguments */ /*{{{*/
static t_scheduler_dagedge * SchedulerDagEdgeNewFromSource(t_scheduler_dag * dag, t_ins * tail, t_scheduler_dependency dependency)
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
  DAGNODE_DEPENDENCYCOUNT(tail_node)++;

  return ret;
}/*}}}*/

/* Add a new edge to the dag, from the head to the sink, with dependency given as arguments */ /*{{{*/
static t_scheduler_dagedge * SchedulerDagEdgeNewToSink(t_scheduler_dag * dag, t_ins * head, t_scheduler_dependency dependency)
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

static void SchedulerDagFree(t_scheduler_dag * dag)/*{{{*/
{
  t_node * node, * tmp_node;
  t_edge * edge, * tmp_edge;

  GRAPH_FOREACH_NODE_SAFE(T_GRAPH(dag),node,tmp_node)
    Free(node);
  GRAPH_FOREACH_EDGE_SAFE(T_GRAPH(dag),edge,tmp_edge)
    Free(edge);

  delete dag;
}/*}}}*/


