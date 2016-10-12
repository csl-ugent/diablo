/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
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

#ifdef __cplusplus
extern "C" {
#endif
#include <diabloanopti386.h>
#include <diablodiversity.h>
#include <math.h>

/* {{{ Includes */
#include <diabloflowgraph.h>
#ifdef __cplusplus
}
#endif
/* }}} */

/*Type and Function definitions{{{*/
typedef struct _t_scheduler_dagedge t_scheduler_dagedge;
typedef struct _t_scheduler_dagnode t_scheduler_dagnode;
typedef struct _t_scheduler_dag t_scheduler_dag;

typedef struct _t_scheduler_list t_scheduler_list;
typedef struct _t_scheduler_listitem t_scheduler_listitem;

typedef enum _t_scheduler_dependency
{
  DEP_RAW = 0,
  DEP_WAW,
  DEP_ORDERED,
  DEP_WAR,
  DEP_WEAKLY_ORDERED,
  DEP_RAR
} t_scheduler_dependency;


struct _t_scheduler_dag
{
  t_graph _hidden_graph;
  
  t_bbl * _hidden_bbl;

  /* Maybe add some specific fields here? */
  t_scheduler_dagnode * _hidden_source;
  t_scheduler_dagnode * _hidden_sink;
 
  /*The schedule under construction*/
  t_scheduler_list * _hidden_schedule;

  /*The nodes that are ready for scheduling*/
  t_scheduler_list * _hidden_ready;

  /*added for counting all possible schedules*/
  t_scheduler_dagnode * _hidden_first;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * _hidden_last;
  /*added for counting all possible schedules*/
  t_uint32 _hidden_nr_scheduled;
};

#define T_DAG(x)	((t_scheduler_dag*)x)
#define DAG_BBL(x)	(T_DAG(x)->_hidden_bbl)
#define DAG_SOURCE(x)	(T_DAG(x)->_hidden_source)
#define DAG_SINK(x)	(T_DAG(x)->_hidden_sink)
#define DAG_SCHEDULE(x)	(T_DAG(x)->_hidden_schedule)
#define DAG_READY(x)	(T_DAG(x)->_hidden_ready)
#define DAG_FIRST(x)	(T_DAG(x)->_hidden_first)
#define DAG_LAST(x)	(T_DAG(x)->_hidden_last)
#define DAG_NR_SCHEDULED(x)	(T_DAG(x)->_hidden_nr_scheduled)

struct _t_scheduler_dagnode
{
  t_node _hidden_node;
  t_ins * _hidden_ins;
  t_uint32 _hidden_dependency_count; /* The node of predecessors that are unscheduled */
  t_uint32 _hidden_flags;
  /* We should add the resources here, in an arch independend manner */
  
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * _hidden_corr;
  /*added for counting all possible schedules*/
  t_scheduler_dagnode * _hidden_next;
  /*added for counting all possible schedules*/
  t_bool _hidden_is_sink;
  /*added for counting all possible schedules*/
  t_bool _hidden_is_source;
};
#define T_DAGNODE(x)	((t_scheduler_dagnode*)x)
#define DAGNODE_INS(x)		(T_DAGNODE(x)->_hidden_ins)
#define DAGNODE_DEPENDENCYCOUNT(x) (T_DAGNODE(x)->_hidden_dependency_count)
#define DAGNODE_FLAGS(x)	(T_DAGNODE(x)->_hidden_flags)
#define DAGNODE_CORR(x)	(T_DAGNODE(x)->_hidden_corr)
#define DAGNODE_NEXT(x)	(T_DAGNODE(x)->_hidden_next)
#define DAGNODE_IS_SINK(x)	(T_DAGNODE(x)->_hidden_is_sink)
#define DAGNODE_IS_SOURCE(x)	(T_DAGNODE(x)->_hidden_is_source)

struct _t_scheduler_dagedge
{
  t_edge _hidden_edge;
};

#define SCHED_SOURCE_DUMMY	0x4	/* A start dummy node, the unique source of the dag */
#define SCHED_SINK_DUMMY	0x8	/* A sink dummy node, the unique sink of the dag */

#define DAGNODE_IS_DUMMY(x)	(T_DAGNODE(x)->flags & (SCHED_SOURCE_DUMMY || SCHED_SINK_DUMMY))

#define T_DAGEDGE(x)	((t_scheduler_dagedge*)x)
#define DAGEDGE_DEPENDENCY_TYPE(edge)	(/*(t_scheduler_dependency)*/EDGE_CAT(edge))
#define DAGEDGE_SET_DEPENDENCY_TYPE(edge, b)	(/*(t_scheduler_dependency)*/EDGE_SET_CAT(edge,b))

#define DE_INFINITY	(0xfffffff0)	/*< An infinite weight for scheduler_dagedges */

struct _t_scheduler_listitem
{
  t_scheduler_dagnode * _hidden_node;
  struct _t_scheduler_listitem *_hidden_next;
  struct _t_scheduler_listitem *_hidden_prev;
};
#define SCHEDULER_LISTITEM_NEXT(x) (x->_hidden_next)
#define SCHEDULER_LISTITEM_PREV(x) (x->_hidden_prev)
#define SCHEDULER_LISTITEM_NODE(x) (x->_hidden_node)

struct _t_scheduler_list
{
  t_scheduler_listitem * _hidden_first;
  t_scheduler_listitem * _hidden_last;
  t_uint32 _hidden_count;
};
#define SCHEDULER_LIST_FIRST(x) (x->_hidden_first)
#define SCHEDULER_LIST_LAST(x) (x->_hidden_last)
#define SCHEDULER_LIST_COUNT(x) (x->_hidden_count)

static t_scheduler_dag * SchedulerDagNew(t_bbl * bbl);
static void SchedulerDagFree(t_scheduler_dag * dag);

static t_scheduler_list * SchedulerListNew(void);
static void SchedulerListFree(t_scheduler_list * list);

static t_scheduler_dagnode * SchedulerDagNodeNew(t_scheduler_dag * dag, t_ins *ins);
static t_scheduler_dagedge * SchedulerDagEdgeNew(t_scheduler_dag * dag, t_ins * head, t_ins * tail, t_scheduler_dependency dependency);
static t_scheduler_dagedge * SchedulerDagEdgeNewToSink(t_scheduler_dag * dag, t_ins * head, t_scheduler_dependency dependency);
static t_scheduler_dagedge * SchedulerDagEdgeNewFromSource(t_scheduler_dag * dag, t_ins * tail, t_scheduler_dependency dependency);

static t_scheduler_dag * SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set);
static void SchedulerApplySchedule(t_scheduler_dag * dag);
static void SchedulerDagScheduleNr(t_scheduler_dag * dag, t_uint32 nr);
static int SteganographyNumberOfPermutations(t_scheduler_dag * dag);

static void SchedulerListInsertNode(t_scheduler_list * list, t_scheduler_dagnode * node_to_schedule);
/*}}}*/

t_scheduler_dag * current_dag = NULL;

double ComputeAvailableChoice(t_cfg * cfg)
{
  t_bbl * bbl;
  double tracker = 0;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_scheduler_dag * dag = SchedulerDagBuildForBbl(bbl,NullRegs);
    int count = SteganographyNumberOfPermutations(dag);
    tracker+=(log(count)/log(2));
    VERBOSE(0,("%lf",tracker));
      
  }
  return tracker;
}

static void SchedulerListPrint(t_scheduler_list * list)
{
  t_scheduler_listitem * listitem = SCHEDULER_LIST_FIRST(list);
  for (t_uint32 i = 0; i < SCHEDULER_LIST_COUNT(list); i++)
  {
    if(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(listitem))!=NULL)
      VERBOSE(0,("@I",DAGNODE_INS(SCHEDULER_LISTITEM_NODE(listitem))));
    else
      VERBOSE(0,("NOT AN INSTRUCTION %d %d",DAGNODE_IS_SINK(SCHEDULER_LISTITEM_NODE(listitem)), DAGNODE_IS_SOURCE(SCHEDULER_LISTITEM_NODE(listitem))));
    listitem = SCHEDULER_LISTITEM_NEXT(listitem);
  }
}
t_diversity_options DiversityScheduleBbls(t_cfg * cfg, t_diversity_choice * choice, t_uint32 phase)/*{{{*/
{
  t_diversity_options ret;
  ret.done = FALSE;
  /*BOOTSTRAP*/
  if(current_dag == NULL)
  {
    /*make sure everything is correct*/
    t_bbl * bbl;
    t_ins * ins;
    CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_FOREACH_INS(bbl,ins)
      {
	I386_INS_SET_REGS_DEF(T_I386_INS(ins), I386InsDefinedRegisters(T_I386_INS(ins)));
	I386_INS_SET_REGS_USE(T_I386_INS(ins), I386InsUsedRegisters(T_I386_INS(ins)));
      }
    }
    CfgComputeLiveness(cfg,TRIVIAL);

    current_dag = SchedulerDagBuildForBbl(T_BBL(CFG_NODE_FIRST(cfg)), NullRegs);
  }
  /*CONTINUED*/
  else
  {
    SchedulerDagScheduleNr(current_dag,choice->choice);
  }

  /*as long as there is no choice, continue*/
  while(SCHEDULER_LIST_COUNT(DAG_READY(current_dag)) == 1 || SCHEDULER_LIST_COUNT(DAG_READY(current_dag)) == 0)
  {
    if(SCHEDULER_LIST_COUNT(DAG_READY(current_dag)) == 1)
    {
      SchedulerDagScheduleNr(current_dag,0);
    }
    else if(SCHEDULER_LIST_COUNT(DAG_READY(current_dag)) == 0)
    {
      t_bbl * next = BBL_NEXT(DAG_BBL(current_dag));
      SchedulerApplySchedule(current_dag);
      SchedulerDagFree(current_dag);
      current_dag = NULL;

      if(next)
      {
	current_dag = SchedulerDagBuildForBbl(next, NullRegs);
      }
      else
      {
	ret.done = TRUE;
	break;
      }
    }
  }
  
  if(!ret.done)
  {
    ret.range = SCHEDULER_LIST_COUNT(DAG_READY(current_dag)) - 1;
    ret.flags = FALSE;
    ret.done = FALSE;
  }
  return ret;
}/*}}}*/

static t_scheduler_listitem * SchedulerListAdd(t_scheduler_list * list, t_scheduler_dagnode * node)/*{{{*/
{
  t_scheduler_listitem * listitem = (t_scheduler_listitem *) Calloc(1,sizeof(t_scheduler_listitem));
  SCHEDULER_LISTITEM_NODE(listitem) = node;
  
  SCHEDULER_LISTITEM_NEXT(listitem) = NULL;
  SCHEDULER_LISTITEM_PREV(listitem) = SCHEDULER_LIST_LAST(list);
  
  SCHEDULER_LIST_LAST(list) = listitem;
  if(SCHEDULER_LISTITEM_PREV(listitem)==NULL)
    SCHEDULER_LIST_FIRST(list) = listitem;
  else
    SCHEDULER_LISTITEM_NEXT(SCHEDULER_LISTITEM_PREV(listitem))=listitem;
  SCHEDULER_LIST_COUNT(list)++;

  return listitem;
}
/*}}}*/

static t_scheduler_list * SchedulerListNew(void)/*{{{*/
{
  t_scheduler_list * list = (t_scheduler_list *) Calloc(1,sizeof(t_scheduler_list));
  return list; 
}/*}}}*/

static void SchedulerApplySchedule(t_scheduler_dag * dag)/*{{{*/
{
  t_scheduler_listitem * item = SCHEDULER_LIST_FIRST(DAG_SCHEDULE(dag));
  t_ins * current_ins = NULL;
  
  while(item)
  {
    if(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item)) != NULL)
    {
      if(current_ins == NULL)
      {
	if(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item))!=BBL_INS_FIRST(DAG_BBL(dag)))
	{
	  BblMoveInstructionBefore(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item)), BBL_INS_FIRST(DAG_BBL(dag)));
	}
      }
      else 
      {
	if(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item))!=current_ins)
	{
	  BblMoveInstructionAfter(DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item)), current_ins);
	}
      }

      current_ins = DAGNODE_INS(SCHEDULER_LISTITEM_NODE(item));
    }
    item = SCHEDULER_LISTITEM_NEXT(item);
  }
}/*}}}*/

static t_scheduler_dag *  SchedulerDagBuildForBbl(t_bbl * bbl, t_regset exclude_set)/*{{{*/
{
  t_ins* ins, *iter_ins;
  t_node * node;
  t_regset currUsed, tmpUsed;
  t_regset currDefs, tmpDefs;
  t_scheduler_dagedge * ret_edge;
  t_bool store_treated;
  t_regset live = BBL_REGS_LIVE_OUT(bbl);
  t_scheduler_dag * dag = SchedulerDagNew(bbl);
  
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

    if(!I386InsIsConditional(T_I386_INS(ins)))     
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
    if(INS_TYPE(ins) == IT_STORE || INS_TYPE(ins) == IT_FLT_STORE || INS_TYPE(ins) == IT_STORE_MULTIPLE
	|| I386InsIsStore(T_I386_INS(ins))
	|| RegsetIn(INS_REGS_DEF(ins),I386_REG_ESP)
	)
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
	if(!store_treated && (INS_TYPE(iter_ins) == IT_STORE || INS_TYPE(iter_ins) == IT_FLT_STORE || INS_TYPE(iter_ins) == IT_STORE_MULTIPLE
	|| I386InsIsStore(T_I386_INS(iter_ins))
	|| RegsetIn(INS_REGS_DEF(iter_ins),I386_REG_ESP)
	      ))
	{
	  ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAW);
	  store_treated = TRUE;
	}
	if(INS_TYPE(iter_ins) == IT_LOAD || INS_TYPE(iter_ins) == IT_FLT_LOAD || INS_TYPE(iter_ins) == IT_LOAD_MULTIPLE
	|| I386InsIsLoad(T_I386_INS(iter_ins))
	    )
	{
	  ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_WAR);
	}
        iter_ins = INS_IPREV(iter_ins);
      }
    }
    if(INS_TYPE(ins) == IT_LOAD || INS_TYPE(ins) == IT_FLT_LOAD || INS_TYPE(ins) == IT_LOAD_MULTIPLE
	|| I386InsIsLoad(T_I386_INS(ins))
	)
    {
      iter_ins = INS_IPREV(ins);
      while(iter_ins)
      {
	if(INS_TYPE(iter_ins) == IT_STORE || INS_TYPE(iter_ins) == IT_FLT_STORE || INS_TYPE(iter_ins) == IT_STORE_MULTIPLE
	|| I386InsIsStore(T_I386_INS(iter_ins))
	|| RegsetIn(INS_REGS_DEF(iter_ins),I386_REG_ESP)
	    )
	{
	  ret_edge = SchedulerDagEdgeNew(dag,iter_ins,ins,DEP_RAW);
	  break;
	}
        iter_ins = INS_IPREV(iter_ins);
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
  
  DAG_READY(dag) = SchedulerListNew();
  DAG_SCHEDULE(dag) = SchedulerListNew();
  SchedulerListAdd(DAG_READY(dag),DAG_SOURCE(dag));
  return dag;
}/*}}}*/

/* Add a new node to the dag containing instruction ins */ /*{{{*/
static t_scheduler_dagnode * SchedulerDagNodeNew(t_scheduler_dag * dag, t_ins *ins)
{
  t_scheduler_dagnode * ret = T_DAGNODE(GraphNewNode(T_GRAPH(dag),0));

  DAGNODE_INS(ret) = ins;
 // DAGNODE_START(ret) = 0;
 // DAGNODE_DEADLINE(ret) = DE_INFINITY;
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
  t_scheduler_listitem * to_schedule = SCHEDULER_LIST_FIRST(DAG_READY(dag));
  t_node * node;
  for(t_uint32 i = 0; i < nr; i++)
  {
    to_schedule = SCHEDULER_LISTITEM_NEXT(to_schedule);
  }
  {
    if(SCHEDULER_LISTITEM_PREV(to_schedule))
      SCHEDULER_LISTITEM_NEXT(SCHEDULER_LISTITEM_PREV(to_schedule)) = SCHEDULER_LISTITEM_NEXT(to_schedule);
    else
      SCHEDULER_LIST_FIRST(DAG_READY(dag))=SCHEDULER_LISTITEM_NEXT(to_schedule);

    if(SCHEDULER_LISTITEM_NEXT(to_schedule))
      SCHEDULER_LISTITEM_PREV(SCHEDULER_LISTITEM_NEXT(to_schedule)) = SCHEDULER_LISTITEM_PREV(to_schedule);
    else
      SCHEDULER_LIST_LAST(DAG_READY(dag))=SCHEDULER_LISTITEM_PREV(to_schedule);
    
    SCHEDULER_LIST_COUNT(DAG_READY(dag))--;
  }

  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    t_edge * edge;
    if(node == (t_node*) SCHEDULER_LISTITEM_NODE(to_schedule))
      NODE_FOREACH_SUCC_EDGE(node, edge)
      {
	DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_TAIL(T_EDGE(edge))))--;
	if(DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(EDGE_TAIL(T_EDGE(edge))))==0)
	{
	  SchedulerListAdd(DAG_READY(dag),T_DAGNODE(EDGE_TAIL(T_EDGE(edge))));
	}
      }
  }
  SchedulerListAdd(DAG_SCHEDULE(dag),SCHEDULER_LISTITEM_NODE(to_schedule));
}/*}}}*/

static t_scheduler_dag * SchedulerDagNew(t_bbl * bbl)/*{{{*/
{
  t_scheduler_dag * ret = (t_scheduler_dag *) Calloc(1,sizeof(t_scheduler_dag));
  
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

  SchedulerListFree(DAG_SCHEDULE(dag));
  SchedulerListFree(DAG_READY(dag));
  Free(dag);
}/*}}}*/

static void SchedulerDagFreeStego(t_scheduler_dag * dag)/*{{{*/
{
  t_node * node, * tmp_node;
  t_edge * edge, * tmp_edge;

  GRAPH_FOREACH_NODE_SAFE(T_GRAPH(dag),node,tmp_node)
    Free(node);
  GRAPH_FOREACH_EDGE_SAFE(T_GRAPH(dag),edge,tmp_edge)
    Free(edge);

  Free(dag);
}/*}}}*/

static void SchedulerListFree(t_scheduler_list * list)/*{{{*/
{
  t_scheduler_listitem * item = SCHEDULER_LIST_FIRST(list);
 
  while(item)
  {
    t_scheduler_listitem * tmp = SCHEDULER_LISTITEM_NEXT(item);
    Free(item);
    item = tmp;
  }
  Free(list);
}/*}}}*/

/*--------------------------------------------------------------------------*/

t_uint32 nr_found_so_far;
#define PERMUTATIONLIMIT 1024

typedef struct _t_stego_listitem t_stego_listitem;

struct _t_stego_listitem{
  t_scheduler_dagnode * node;
  t_stego_listitem * prev;
  t_stego_listitem * next;
};

typedef struct _t_stego_list
{
  t_stego_listitem * first;
  t_stego_listitem * last;
} t_stego_list;

static t_stego_list * stegoListNew(void)/*{{{*/

{
  t_stego_list * list = (t_stego_list *) Calloc(1,sizeof(t_stego_list));

  list->first = NULL;
  list->last = NULL;
  
  return list;
}
/*}}}*/

static t_stego_listitem * stegoListAdd(t_stego_list * list, t_scheduler_dagnode * node)/*{{{*/

{
  t_stego_listitem * listitem = (t_stego_listitem *) Malloc(sizeof(t_stego_listitem));
  listitem->node = node;
  
  listitem->prev = NULL;
  listitem->next = list->first;
  
  list->first = listitem;
  if(listitem->next==NULL){
    list->last = listitem;
  }

  return listitem;
}
/*}}}*/

static void stegoListRemove(t_stego_list * list, t_stego_listitem * listitem)/*{{{*/

{
  if(listitem == list->first)
    list->first = listitem->next;
  else
    listitem->prev->next = listitem->next;
  if(listitem == list->last)
    list->last = listitem->prev;
  else 
    listitem->next->prev = listitem->prev;

  Free(listitem);
}
/*}}}*/

static void stegoListFree(t_stego_list * list)/*{{{*/

{
  t_stego_listitem * item = list->first;
  while(item != NULL){
    stegoListRemove(list, item);
    item = list->first;
  }
  Free(list);
  return;
}
/*}}}*/

static t_stego_list * NewStegoListRemove(t_stego_list * list_orig, t_stego_listitem * item)/*{{{*/
  /*return a new stegolist, with item removed from the original list*/
{
  t_stego_list * ret = stegoListNew();
  t_stego_listitem * iter;
  for(iter = list_orig->first; iter != NULL; iter = iter->next){
    if(iter != item)
      stegoListAdd(ret, iter->node);
  }
  return ret;
}
/*}}}*/

static t_scheduler_dag * NewStegoDagSchedule(t_scheduler_dag * dag_orig, t_stego_list * list_orig, t_scheduler_dagnode * node)/*{{{*/
  /*Returns a new dag with node scheduled */
{
  t_scheduler_dag * ret = SchedulerDagNew/*Stego*/(NULL);
  t_node * iter_node;
  t_edge * iter_edge, * tmp_edge;
  t_stego_listitem * iter_item;
  //PWord_t JudyValue;
  DAG_NR_SCHEDULED(ret) = DAG_NR_SCHEDULED(dag_orig)+1;

  GRAPH_FOREACH_NODE(T_GRAPH(dag_orig), iter_node){
    t_scheduler_dagnode * add = T_DAGNODE(GraphNewNode(T_GRAPH(ret),0));
   /* 
    t_dagnode_stego * node_stego = Calloc(1,sizeof(t_dagnode_stego));
    JLI(JudyValue, JudyMapStego, add);
    *JudyValue = node_stego;
    */
    DAGNODE_NEXT(T_DAGNODE(add))=NULL;
    DAGNODE_INS(add) = DAGNODE_INS(T_DAGNODE(iter_node));
    DAGNODE_DEPENDENCYCOUNT(add) = DAGNODE_DEPENDENCYCOUNT(T_DAGNODE(iter_node));
    if(T_DAGNODE(iter_node) == DAG_SOURCE(dag_orig))
      DAG_SOURCE(ret)=add;
    if(T_DAGNODE(iter_node) == DAG_SINK(dag_orig))
      DAG_SINK(ret)=add;
    DAGNODE_CORR(T_DAGNODE(iter_node)) = add;
  }
    
  DAG_FIRST(ret) = (DAG_FIRST(dag_orig) == NULL)?NULL:DAGNODE_CORR(DAG_FIRST(dag_orig));
  DAG_LAST(ret) = (DAG_LAST(dag_orig) == NULL)?NULL:DAGNODE_CORR(DAG_LAST(dag_orig));
  
  {
    t_scheduler_dagnode * node=T_DAGNODE(DAG_FIRST(dag_orig));
    while(node){
      if(DAGNODE_NEXT(node))
	DAGNODE_NEXT(DAGNODE_CORR(node)) = DAGNODE_CORR(DAGNODE_NEXT(node));
      node = DAGNODE_NEXT(node);
    }
  }
  
  GRAPH_FOREACH_NODE(T_GRAPH(dag_orig), iter_node){
    if(T_DAGNODE(iter_node)==node){
      DAGNODE_NEXT(DAGNODE_CORR(node)) = NULL;
      if(DAG_FIRST(ret) == NULL){
	DAG_FIRST(ret) = DAGNODE_CORR(node);
	DAG_LAST(ret) = DAGNODE_CORR(node);
	DAGNODE_NEXT(DAG_FIRST(ret)) = NULL;
      }
      else{
	DAGNODE_NEXT(DAG_LAST(ret)) = DAGNODE_CORR(node);
	DAG_LAST(ret) = DAGNODE_CORR(node);
	DAGNODE_NEXT(DAG_LAST(ret)) = NULL;
      }
    }
  }
  
  GRAPH_FOREACH_EDGE_SAFE(T_GRAPH(dag_orig), iter_edge, tmp_edge){
    
    if(EDGE_HEAD(iter_edge)==T_NODE(node)){
      DAGNODE_DEPENDENCYCOUNT(DAGNODE_CORR(T_DAGNODE(EDGE_TAIL(iter_edge))))--;
      if(DAGNODE_DEPENDENCYCOUNT(DAGNODE_CORR(T_DAGNODE(EDGE_TAIL(iter_edge))))==0){
	stegoListAdd(list_orig, T_DAGNODE(EDGE_TAIL(iter_edge)));
      }
    }
    else{
      GraphNewEdge(T_GRAPH(ret),T_NODE(DAGNODE_CORR(T_DAGNODE(EDGE_HEAD(iter_edge)))),T_NODE(DAGNODE_CORR(T_DAGNODE(EDGE_TAIL(iter_edge)))),0);
    }
  }
  
  for(iter_item = list_orig->first; iter_item != NULL; iter_item = iter_item->next){
    iter_item->node = DAGNODE_CORR(T_DAGNODE(iter_item->node));
  }

  return ret;
}
/*}}}*/

static void StegoSchedule(t_scheduler_dag * dag_orig, t_stego_list * list_orig, t_stego_listitem * item)/*{{{*/

{
  t_scheduler_dagnode * node;
  t_stego_list * list;
  t_scheduler_dag * dag;
  t_stego_listitem * iter;
  
  node = item->node;
  list = NewStegoListRemove(list_orig, item);
  dag = NewStegoDagSchedule(dag_orig, list, node);
    
  /*Schedule determined*/
  if(list->first == NULL){
    SchedulerDagFreeStego(dag);
    stegoListFree(list);
    nr_found_so_far++;
    return;
  }
  
  for(iter = list->first; iter != NULL; iter = iter->next){
    StegoSchedule(dag, list, iter);
    if(nr_found_so_far == PERMUTATIONLIMIT){
      SchedulerDagFreeStego(dag);
      stegoListFree(list);
      return;
    }
  }
  
  SchedulerDagFreeStego(dag);
  stegoListFree(list);
  return;
}
/*}}}*/

static int SteganographyNumberOfPermutations(t_scheduler_dag * dag)/*{{{*/
{
  t_scheduler_dagnode * source= DAG_SOURCE(dag);
  t_stego_list * list = stegoListNew();
  t_stego_listitem * item;
  nr_found_so_far = 0;
  
  /*schedule source*/
  item = stegoListAdd(list, source);
  DAG_FIRST(dag) = NULL;
  DAG_LAST(dag) = NULL;
  StegoSchedule(dag,list,item);
  stegoListFree(list);
  
  return nr_found_so_far;
}
/*}}}*/

#if 0 /*{{{*/


void SchedulerListInsertNode(t_scheduler_list * list, t_scheduler_dagnode * node_to_schedule)/*{{{*/
{
  t_scheduler_listitem * listitem = (t_scheduler_listitem*) Malloc(sizeof(t_scheduler_listitem));
  listitem->next = NULL;
  listitem->node = node_to_schedule;

  list->nins++;

  /* Insert in the linked list at cycle 'DAGNODE_START(node_to_schedule)' */
  if(list->schedule[DAGNODE_START(node_to_schedule)]) FATAL(("Pretty strange!"));
  listitem->next = list->schedule[DAGNODE_START(node_to_schedule)];
  list->schedule[DAGNODE_START(node_to_schedule)] = listitem;
  
  /*if(list->schedule[DAGNODE_START(node_to_schedule)])
  {
    list->last->next = listitem;
    list->last = listitem;
  }
  else
  {
    list->first = list->last = listitem;
  }*/

}/*}}}*/

/* Update the absolute timing of each node and return true when there is a node ready for scheduling {{{*/
t_bool SchedulerDagUpdateIntervalsAndReadyList(t_scheduler_dag *dag, t_scheduler_dagnode * node, t_uint32 curr_cycle)
{
  t_node * i_node;
  if(node)
  {
    DAGNODE_START(node) = curr_cycle;
    DAGNODE_DEADLINE(node) = curr_cycle;

    SchedulerUpdateAbsoluteTime(node);
  }

  GRAPH_FOREACH_NODE(T_GRAPH(dag),i_node)
    if(DAGNODE_DEPENDENCYCOUNT(i_node) == 0 && !(DAGNODE_FLAGS(i_node) & SCHED_SCHEDULED))
      return TRUE;

  return FALSE;

}/* }}} */

/* Update absolute timing, when a node gets scheduled {{{ */
void SchedulerUpdateAbsoluteTime(t_scheduler_dagnode * node)
{
  t_edge * edge;
  t_bool changes;
  t_uint32 min,max;
  if(DAGNODE_START(node) > DAGNODE_DEADLINE(node))
  {
    if(node->ins)
    FATAL(("Start greater than deadline! @I",node->ins));
    else
      FATAL(("Start comes after deadline"));
  }
  /*if(node->ins) DiabloPrint(stdout,"Doing node @I, %d, %d\n",node->ins,node->start,node->deadline);
  else DiabloPrint(stdout,"Doing node S/E, %d, %d\n",node->start,node->deadline);*/
  NODE_FOREACH_SUCC_EDGE(T_NODE(node),edge)
  {
    changes = FALSE;
    max = (DAGNODE_DEADLINE(node) == DE_INFINITY || DAGEDGE_MAXWEIGHT(edge) == DE_INFINITY)? DE_INFINITY:DAGNODE_DEADLINE(node) + DAGEDGE_MAXWEIGHT(edge);
    min = (DAGNODE_START(node) == DE_INFINITY || DAGEDGE_MINWEIGHT(edge) == DE_INFINITY)? DE_INFINITY:DAGNODE_START(node) + DAGEDGE_MINWEIGHT(edge);
    if(DAGNODE_START(EDGE_TAIL(edge)) < min)
    {
      DAGNODE_START(EDGE_TAIL(edge)) = min;
      changes = TRUE;
    }
    if(DAGNODE_DEADLINE(EDGE_TAIL(edge)) > max)
    {
      DAGNODE_DEADLINE(EDGE_TAIL(edge)) = max;
      changes = TRUE;
    }
    if(changes) SchedulerUpdateAbsoluteTime(T_DAGNODE(EDGE_TAIL(edge)));
  }
  NODE_FOREACH_PRED_EDGE(T_NODE(node),edge)
  {
    changes = FALSE;
    max = (DAGNODE_DEADLINE(node) < DAGEDGE_MINWEIGHT(edge))? 0: (DAGNODE_DEADLINE(node) == DE_INFINITY) ? DE_INFINITY: DAGNODE_DEADLINE(node) - DAGEDGE_MINWEIGHT(edge);
    min = (DAGNODE_START(node) < DAGEDGE_MAXWEIGHT(edge))? 0: DAGNODE_START(node) - DAGEDGE_MAXWEIGHT(edge);
    if(DAGNODE_START(EDGE_HEAD(edge)) < min)
    {
      DAGNODE_START(EDGE_HEAD(edge)) = min;
      changes = TRUE;
    }
    if(DAGNODE_DEADLINE(EDGE_HEAD(edge)) > max)
    {
      DAGNODE_DEADLINE(EDGE_HEAD(edge)) = max;
      changes = TRUE;
    }
    if(changes) SchedulerUpdateAbsoluteTime(T_DAGNODE(EDGE_HEAD(edge)));
  }
  /*if(node->ins) DiabloPrint(stdout,"DONE node @I, %d, %d\n",node->ins,node->start,node->deadline);*/
  /*else DiabloPrint(stdout,"DONE node S/E, %d, %d\n",node->start,node->deadline);*/
}/*}}}*/

/* Write out of dag to dots files */ /*{{{*/
void SchedulerDagPrint(t_scheduler_dag * dag, t_bbl * bbl, t_uint32 troubles)
{
  char * filename, /*buffer[512],*/ labelbuf[10];
  FILE * of;
  t_node * node;
  t_edge * edge;
  t_string color = NULL, style = NULL;

  DirMake("./dots-dag",FALSE);
  if(!troubles)
    filename = StringIo("./dots-dag/@G-%s.dot",BBL_INS_FIRST(bbl)?INS_OLD_ADDRESS(BBL_INS_FIRST(bbl)):0x0,BBL_FUNCTION(bbl)?(FUNCTION_NAME(BBL_FUNCTION(bbl))?FUNCTION_NAME(BBL_FUNCTION(bbl)):"no-name"):"no-name");
  else
    filename = StringIo("./dots-dag/@G-%d.dot",BBL_INS_FIRST(bbl)?INS_OLD_ADDRESS(BBL_INS_FIRST(bbl)):0x0,troubles);
  of = fopen(filename,"w");
  ASSERT(of,("Could not open %s for writing!",filename));

  FileIo(of, "digraph \"@G\" {\n\tnode [shape=box]\n",BBL_OLD_ADDRESS(bbl));
  GRAPH_FOREACH_NODE(T_GRAPH(dag),node)
  {
    if(DAGNODE_INS(node))
    {
      FileIo(of,"\t\"%p\" [label=\"@G:@aI, %d,%d\"];\n",node,INS_OLD_ADDRESS(DAGNODE_INS(node)),DAGNODE_INS(node), DAGNODE_START(node), DAGNODE_DEADLINE(node));
    }
    else if(T_DAGNODE(node) == DAG_SOURCE(dag))
      FileIo(of,"\t\"%p\" [label=\"Source, %d,%d\"];\n",node, DAGNODE_START(node), DAGNODE_DEADLINE(node));
    else if(T_DAGNODE(node) == DAG_SINK(dag))
      FileIo(of,"\t\"%p\" [label=\"Sink, %d,%d\"];\n",node, DAGNODE_START(node), DAGNODE_DEADLINE(node));


    NODE_FOREACH_SUCC_EDGE(node,edge)
    {
      /*
      t_node * current_node = EDGE_TAIL(edge);
      t_node * iter_node;
      */
      t_edge * i_edge,* j_edge;
      t_bool dontprint = FALSE;
      NODE_FOREACH_SUCC_EDGE(node,i_edge)
      {
	if(edge == i_edge) continue;
	NODE_FOREACH_SUCC_EDGE(EDGE_TAIL(i_edge),j_edge)
	{
	  if(EDGE_TAIL(j_edge) == EDGE_TAIL(edge))
	  {
	    dontprint = TRUE;
	    break;
	  }
	}
	if(dontprint) break;
      }

      if(!dontprint)
      {
	(DAGEDGE_MAXWEIGHT(edge) == DE_INFINITY)?sprintf(labelbuf,"INF"):sprintf(labelbuf,"%d",DAGEDGE_MAXWEIGHT(edge));
	switch(DAGEDGE_DEPENDENCY_TYPE(edge))
	{
	  case DEP_RAR:
	    color = "gray";
	    style = "dashed";
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
	    color = "yellow";
	    style = "dashed";
	    break;
	}

	fprintf(of,"\t\"%p\" -> \"%p\" [label=\"%d,%s\",style=%s,color=%s];\n",node,EDGE_TAIL(edge),DAGEDGE_MINWEIGHT(edge),labelbuf,style,color);
      }
    }
  }
  fprintf(of, "}\n");

  Free(filename);
  fclose(of);

}/*}}}*/

/* Print a schedule {{{*/
void SchedulerListPrint(t_scheduler_list * list)
{
  t_uint32 i;
  t_bbl * bbl = list->bbl;
  t_scheduler_listitem * item=NULL;
  if(bbl)
  for(i = 1; i < list->cycles -1; i++)
  {
    VERBOSE(0,("Cycle %d:",i));
    if(list->schedule[i]) item = list->schedule[i];
    while(item)
    {
      if(DAGNODE_INS(item->node))
	VERBOSE(0,("@I\n",DAGNODE_INS(item->node)));
      item = item->next;
    }
    VERBOSE(0,("\n"));
  }
  VERBOSE(0,(">--------------------------------------------<\n"));
}/* }}} */

#define MAX(a,b) (((a)>(b))?(a):(b))
/* Calculate the height in the dag for a node {{{ */
t_uint32 SchedulerDagNodeCalcHeight(t_scheduler_dagnode * node)
{
  t_edge * edge;

  if(DAGNODE_FLAGS(node) & SCHED_HEIGHT)
    return DAGNODE_HEIGHT(node);

  NODE_FOREACH_SUCC_EDGE(T_NODE(node), edge)
  {
    t_uint32 inc = DAGEDGE_MINWEIGHT(edge);

    DAGNODE_HEIGHT(node) = MAX(DAGNODE_HEIGHT(node), SchedulerDagNodeCalcHeight(T_DAGNODE(EDGE_TAIL(edge)))+inc);
  }

  DAGNODE_FLAGS(node) |= SCHED_HEIGHT;
  return DAGNODE_HEIGHT(node);
}/* }}} */

void SchedulerUpdateAbsoluteTime(t_scheduler_dagnode * node);

/* temporary selection function, this should become some kind of arch-specific callback *//*{{{*/
t_scheduler_dagnode * SelectionFunction(t_scheduler_dag * dag, t_scheduler_list * list, t_uint32 cycle, t_bool has_pc_use)
{
#ifdef ARM_SUPPORT
  t_node * node;
  t_node * ret = NULL/* , *alter = NULL */;
  t_edge * i_edge;
  t_int32 weight = 0/* , slack = list->cycles */;
  t_int32 maxweight = -8000;
  t_int32 nedges = 0, max_nedges = 0, pred_edges=0, min_pred_edges = 0;
  t_int32 min_deadline = list->cycles;
  t_bool constrained = FALSE;
  int i;
  t_uint32 deadlines[2048] = {0};
  t_bool must_follow_deadline = FALSE;

  printf("Selecting for cycle %d\n",cycle);
  SchedulerListPrint(list);
#if 1
  /* When the block contains instructions that use the pc, we should do some bookkeeping to see if we are going to
   * create a valid schedule */
  if(has_pc_use)
  {
    GRAPH_FOREACH_NODE(dag,node)
    {
      if(DAGNODE_FLAGS(node) & SCHED_SCHEDULED) continue;

      /* Add 1 to all array elements from deadline to last */
      for (i=DAGNODE_DEADLINE(node)-cycle;i<list->cycles-cycle;i++)
	deadlines[i]++;
    }
    
    /*deadlines[list->cycles-cycle-1] = 0; */
    DiabloPrint(stdout,"@B\n",list->bbl);
    for(i = 0; i < list->cycles-cycle; i++)
      printf("Deadlines[%d] = %d\n",i,deadlines[i]);

    printf("<------------------------------------->\n");
    /* When a array element has a count higher than its position in the array, we MUST follow first deadline or else
     * a valid schedule will not be created */
    for (i=0;i<list->cycles-cycle;i++)
      if (deadlines[i]>i)
      { 
	printf("TRUE %d\n",i);
	must_follow_deadline = TRUE;
	break;
      }
  }
#endif

  GRAPH_FOREACH_NODE(dag,node)
  {
    if(DAGNODE_FLAGS(node) & SCHED_SCHEDULED) continue;
/*    if(DAGNODE_START(node) == DAGNODE_DEADLINE(node)) */

    if(DAGNODE_DEPENDENCYCOUNT(node) == 0)
      /*if(DAGNODE_START(node) <= cycle && DAGNODE_DEADLINE(node) >= cycle)*/
    {
      weight = 0;
      nedges = 0;
      pred_edges = 0;

      if(ret) DiabloPrint(stdout,"Current candidate = @I\n",DAGNODE_INS(ret));
      if(DAGNODE_DEADLINE(node) == cycle)
      {
	if(DAGNODE_INS(node))
	DiabloPrint(stdout,"Must take @I because deadline reached\n",DAGNODE_INS(node));
	return T_DAGNODE(node);
      }
      NODE_FOREACH_SUCC_EDGE(node,i_edge)
      {
	weight += (DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAW)?(INS_TYPE(DAGNODE_INS(node))==IT_LOAD?4:3):(DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAR)?0:1;/*DAGEDGE_MINWEIGHT(i_edge);*/
	if(DAGEDGE_DEPENDENCY_TYPE(i_edge) != DEP_RAR) nedges++;
      }

      NODE_FOREACH_PRED_EDGE(node,i_edge)
      {
	if(DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAW && DAGNODE_START(EDGE_HEAD(i_edge)) == (cycle -1))
	{
	  if (INS_TYPE(DAGNODE_INS(EDGE_HEAD(i_edge)))==IT_LOAD)
	  {
	    printf("ok1\n");
	    weight -=3;
	  }
	  printf("ok2\n");
	  weight -=6;
	}
	else
	  weight -= (DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAR)?0:1;
	if(DAGEDGE_DEPENDENCY_TYPE(i_edge) != DEP_RAR) pred_edges++;
      }
      /*slack = DAGNODE_DEADLINE(node) - DAGNODE_START(node);*/

      if(DAGNODE_DEADLINE(node) != DE_INFINITY)
      {
	if (must_follow_deadline)
	{
	  constrained = TRUE;
	  if(DAGNODE_DEADLINE(node) < min_deadline)
	  {
	    ret = node;
	    min_deadline = DAGNODE_DEADLINE(node);
	    maxweight = weight;
	    max_nedges = nedges;
	    min_pred_edges = pred_edges;
	  }
	  else if(DAGNODE_DEADLINE(node) == min_deadline)
	  {
	    if(pred_edges < min_pred_edges)
	    {
	      ret = node;
	      min_deadline = DAGNODE_DEADLINE(node);
	      maxweight = weight;
	      max_nedges = nedges;
	      min_pred_edges = pred_edges;
	    }
	  }
	}
	else
	{
	  constrained = FALSE;
	  weight-=DAGNODE_DEADLINE(node);
	}
      }
      if(!constrained)
      {
	/*NODE_FOREACH_SUCC_EDGE(node,i_edge)
	  {
	  weight += (DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAW)?3:(DAGEDGE_DEPENDENCY_TYPE(i_edge) == DEP_RAR)?0:1;//DAGEDGE_MINWEIGHT(i_edge);
	  if(DAGEDGE_DEPENDENCY_TYPE(i_edge) != DEP_RAR) nedges++;
	  }*/
	if(weight > maxweight)
	{
	  ret = node;
	  maxweight = weight;
	  max_nedges = nedges;
	  min_pred_edges = pred_edges;
	}
	else if (weight == maxweight && nedges > max_nedges)
	{
	  if(!(nedges == max_nedges && pred_edges > min_pred_edges))
	  {
	    ret = node;
	    max_nedges = nedges;
	    min_pred_edges = pred_edges;
	  }
	}
      }
    }
  }
  return T_DAGNODE(ret);
#endif
  return NULL;
}/*}}}*/

void SchedulerCfgListSchedule(t_cfg * cfg)/*{{{*/
{
  t_function * fun;
  t_bbl * bbl;

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
    if(BBL_NINS(bbl))
      SchedulerBblListSchedule(bbl);
}/*}}}*/

void SchedulerApplySchedule(t_scheduler_list * list);
#endif/*}}}*/
