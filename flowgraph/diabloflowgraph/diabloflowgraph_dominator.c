/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/*#define DEBUG_DOMINATOR*/
#include <diabloflowgraph.h>
#include <time.h>

/* Local stack implementation {{{*/
#define MAX_STACK_HEIGHT 100000
#define VSTACK_NOT_EMPTY(stack) ((stack)->top)

typedef struct _t_void_stack
{
  signed int top;
  void * stack[MAX_STACK_HEIGHT];
} void_stack;

#define StackPrint(stack) do \
{ \
  t_uint32 i = 0;\
  while(i < stack->top)\
  {\
    VERBOSE(0,("%d: %d",i,BBL_DFS_NUMBER(stack->stack[i])));\
    i++;\
  }\
} while (0)


#define Push(stack2,data) do { \
  ((void_stack*)stack2)->stack[((void_stack*)stack2)->top] = data;\
  ((void_stack*)stack2)->top++;\
  /*if(((void_stack*)stack2)->top == MAX_STACK_HEIGHT) FATAL(("Increase stack height"));*/\
} while (0)

#define Pop(stack2) ((void*)(stack2)->stack[--((stack2)->top)])/*}}}*/

BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(next_in_dfs);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(dom_marking_number);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(nr_dominated);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(largest_pred_dfs);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(smallest_pred_dfs);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(min_dfs);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(stack_edge);
BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY(dominates_array);

/*clock_t dom_ticks;*/
/*clock_t dom_ticks2;*/
t_bool dominator_info_correct = FALSE;
int max_in_degree = 0;
long unsigned int bevolking = 0;

#define DomNext(x) ((t_bbl_list *)((x)->next))

#define FAST_DOM
#define WITH_REDO
#define WITH_SKIP
/*#define PRINT_PROGRESSION*/
/*#define STATS*/
/*#define PRINT_DFS*/
#define PRINT_PDFS
/*#define PRINT_COMPUTATION*/
/*#define PRINT_INTERSECTION*/
/*#define OPT_DOMINATES2*/
/*#define ENABLE_STATISTICS*/
t_bbl all_bbls;
t_bbl * const ALL_BBLS = &all_bbls;


t_uint32 dominator_node_marking_number=1;

typedef struct _t_bbl_queue
{
  t_bbl_list * start;
  t_bbl_list * end;
} t_bbl_queue;

t_uint32 mask_postdominator = ET_FALLTHROUGH | ET_CALL | ET_RETURN | ET_JUMP /*| ET_SWI*/ | ET_UNKNOWN | ET_IPUNKNOWN | ET_IPFALLTHRU | ET_IPJUMP | ET_COMPENSATING | ET_SWITCH | ET_POSTDOM;

/* {{{ Forward declarations */
void BblMarkInitDominator(void);
#define BblIsMarkedDominator(node) (BBL_DOM_MARKING_NUMBER(node)==dominator_node_marking_number)
#define BblMarkDominator(node)     (BBL_SET_DOM_MARKING_NUMBER(node,dominator_node_marking_number))
#define BblUnmarkDominator(node)   (BBL_SET_DOM_MARKING_NUMBER(node,0))

static t_bool MarkDominatorsUpwardsUntilStop(t_cfg_edge * edge, t_bool init, t_uint32 stop_number, t_cfg_edge * link_edge);
static void Removetransitive(t_bbl * ibbl);
static void RemovetransitiveNew(t_bbl * ibbl);
static void MarkDominatorsDownwardsForSkippingUnnecessaryIntersections(t_bbl * bbl, t_uint32 stop_number);
void BblSortAndCountPredAndSuccEdgesUsingDfs(t_bbl * bbl);
static void RemoveBblFromDominatesOf(t_bbl * bbl, t_bbl * from);
static void MarkRemovedDominatorsUpWardsAndNumber(t_bbl * bbl, t_bbl * assign_bbl);
static void MarkDominatorsUpwardsForSkippingUnnecessaryIntersections(t_bbl * bbl, t_uint32 stop_number);
static void MarkDominatorsUpwardsForTransitiveRemoval(t_bbl * bbl, t_uint32 stop_number);
static void MarkDominatorsDownwardsForTransitiveRemoval(t_bbl * bbl);
void DominatorDFSNumbering(t_cfg *cfg);
#ifdef POST_DOMINATORS_IMPLEMENTED
void DominatorPDFSNumbering(t_cfg *cfg);
void BblSortAndCountPredAndSuccEdgesUsingPDfs(t_bbl * bbl);
#endif
static void MarkDominatorsUpwards(t_bbl * bbl);
static void ComputeCallsAlongPathsAndChangeEdgeOrder(t_cfg * cfg);
void CfgDetectPseudoLeaves(t_cfg * cfg);
static void RemovetransitiveDown(t_bbl * ibbl);
#ifdef ENABLE_STATISTICS
static void GetStatistics(t_cfg *cfg);
#endif
/* }}} */

void DominatorPrint(t_bbl_list * dom)/*{{{*/
{
  while(dom)
    {
      if(dom->bbl != ALL_BBLS)
	VERBOSE(0,("@G(@G)  ",BBL_OLD_ADDRESS(dom->bbl),BBL_FUNCTION(dom->bbl)?BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(BBL_FUNCTION(dom->bbl))):AddressNullForBbl(dom->bbl)));
      else
	VERBOSE(0,("ALL_BBLS  "));
      dom = DomNext(dom);
    }
  printf("\n");
}/*}}}*/

/*!
 * \param t_cfg
 *
 * Calculate the dominators for each (reachable) basic block in the graph, using
 * a fast and efficient algorithm (described in paper).
 *
 * Also postdominators are being calculated, but first loop detection is done,
 * to connect infinite loops with the unique exit node. While doing loop
 * detection, for each basic block, its loop_edge field gets filled in with
 * the backedge defining a loop. */

void ComDominators(t_cfg * cfg)
{
  t_bbl * ibbl;
  t_function * ifun;
  t_loopref * looplist = NULL;
  t_bool do_it_again = TRUE;
#ifdef PRINT_PROGRESSION
  t_uint32 iterations = 0;
  t_uint32 aangepast = 0;
  t_uint32 total_aangepast = 0;
  t_uint32 total_done = 0;
#endif
  t_function * fun;
  t_cfg_edge *i_edge;
  t_cfg_edge *j_edge;
  int teller_edges = 0;

  
  if (dominator_info_correct)
  {
    VERBOSE(0, ("SKIPPED unnecessary dominator calculation"));
    return;
  }
  
  STATUS(START,("Dominator calculation"));

  BblInitNextInDfs(cfg);
  BblInitStackEdge(cfg);
  BblInitDomMarkingNumber(cfg);
  BblInitNrDominated(cfg);
  BblInitLargestPredDfs(cfg);
  BblInitSmallestPredDfs(cfg);
  BblInitMinDfs(cfg);
  BblInitDominatesArray(cfg);
  
  CgBuild(cfg);
  CfgMarkEdgesForWhichInIsReachableFromOut(cfg);
  CfgMarkEdgesForWhichAllBblsAreReachableFromOut(cfg);
  CfgMarkEdgesThatCoverWholeFun(cfg);

    /* first some initializations, setting all required fields to some value {{{*/
  /* All blocks are initially dominated by all blocks, except for the entry */
  
#ifdef ENABLE_MEM_TRACING
  FATAL(("Reimplement")); /*ResetMemoryTracing(); */
#endif

  CFG_FOREACH_FUN(cfg, ifun)
    {
      FUNCTION_FOREACH_BBL(ifun,ibbl)
	{
	  BBL_SET_DFS_NUMBER(ibbl,  0);
	  BBL_SET_NEXT_IN_DFS(ibbl,  NULL);

	  BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl) & (~BBL_FIXED_DOMINATORS));
	  BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl) | NEED_TO_REDO);
	  BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl) & (~HAS_INCOMING_RETURN_EDGE));

	  DominatorFree(BBL_DOMINATED_BY(ibbl));
	  BBL_SET_DOMINATED_BY(ibbl, NULL);
/*          DominatorFree(BBL_P_DOMINATED_BY(ibbl));*/
/*          BBL_SET_P_DOMINATED_BY(ibbl, NULL);*/
	  BBL_SET_DOMINATED_BY(ibbl,  NULL);

	  BBL_SET_STACK_EDGE(ibbl,  NULL);
	  BBL_SET_NR_DOMINATED(ibbl,  0xffffffff);
	}
    }

/*  dom_ticks = clock();*/

  ComputeCallsAlongPathsAndChangeEdgeOrder(cfg);

  /*}}}*/

  DominatorDFSNumbering(cfg);
  
  CfgDetectPseudoLeaves(cfg);

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
      BblSortAndCountPredAndSuccEdgesUsingDfs(ibbl);

  /* Initialise with dfs predecessor and call site if necessary *//*{{{*/

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
    {
      t_bbl * ret_bbl = NULL, *call_bbl = NULL, *other_bbl = NULL;
      t_bbl_list * insert;
      t_uint32 npreds = 0;

      if (BBL_DFS_NUMBER(ibbl) <= 1 )
	{
	  /*        VERBOSE(0,("@B is not numbered!",ibbl));*/
	  continue;
	}
      else if (ibbl==CFG_EXIT_SWI_HELL_NODE(cfg))
	{
	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=CFG_SWI_HELL_NODE(cfg);
	  insert->next=BBL_DOMINATED_BY(ibbl);
	  BBL_SET_DOMINATED_BY(ibbl, insert);

	  SetAddElemBlindly(BBL_DOMINATES_ARRAY(CFG_SWI_HELL_NODE(cfg)),ibbl);
	  BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl)  | BBL_FIXED_DOMINATORS);
	  continue;
	}
#ifdef PRINT_DFS
      else VERBOSE(0,("\nDFSNR: %d ",BBL_DFS_NUMBER(ibbl)));
#endif
      BBL_FOREACH_PRED_EDGE(ibbl,i_edge)
	{
	  if(CFG_EDGE_CAT(i_edge) == ET_RETURN || CFG_EDGE_CAT(i_edge) == ET_COMPENSATING)
	    {
	      BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl) | HAS_INCOMING_RETURN_EDGE);
	      if(BBL_DFS_NUMBER(CFG_EDGE_HEAD(i_edge)) < BBL_DFS_NUMBER(ibbl))
	      {
		if(!call_bbl || BBL_DFS_NUMBER(call_bbl) > BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))))
		  call_bbl = CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge));
		ret_bbl = CFG_EDGE_HEAD(i_edge);
	      }
	    }
	  else if (!other_bbl) 
	    other_bbl = CFG_EDGE_HEAD(i_edge);
	  /* TODO: check < or > */
	  else if (BBL_DFS_NUMBER(other_bbl) > BBL_DFS_NUMBER(CFG_EDGE_HEAD(i_edge))) 
	    other_bbl = CFG_EDGE_HEAD(i_edge);
	  npreds++;
	}

      /* If there are only incoming return or compensating edges */
      if(!other_bbl)
	{
	  if(!ret_bbl) FATAL(("Trubles! @ieB",ibbl));

	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=ret_bbl;
	  insert->next=BBL_DOMINATED_BY(ibbl);
	  BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
	  VERBOSE(0,("1-> %d ",BBL_DFS_NUMBER(ret_bbl)));
#endif
	  SetAddElemBlindly(BBL_DOMINATES_ARRAY(ret_bbl),ibbl);

	  if (BBL_ATTRIB(ret_bbl) & BBL_HAS_MORE_THAN_ONE_OUTGOING_DFS_EDGE || (BBL_ATTRIB(ret_bbl) & IS_EXIT_OF_LEAF)) 
	    {

	      /* do store the call-site because this function has more
		 than one callsite */

	      insert = Malloc(sizeof(t_bbl_list));
	      insert->bbl=call_bbl;
	      insert->next=BBL_DOMINATED_BY(ibbl);
	      BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
	      VERBOSE(0,("2-> %d ",BBL_DFS_NUMBER(call_bbl)));
#endif
	      SetAddElemBlindly(BBL_DOMINATES_ARRAY(call_bbl),ibbl);
	    }
	}
      else if (!ret_bbl)
	{
	  if(!other_bbl) FATAL(("Trubles!"));


	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=other_bbl;
	  insert->next=BBL_DOMINATED_BY(ibbl);
	  BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
	  VERBOSE(0,("3-> %d ",BBL_DFS_NUMBER(other_bbl)));
#endif
	  SetAddElemBlindly(BBL_DOMINATES_ARRAY(other_bbl),ibbl);
	}
      else
	{
	  if(BBL_DFS_NUMBER(call_bbl)<BBL_DFS_NUMBER(other_bbl))
	    {
	      insert = Malloc(sizeof(t_bbl_list));
	      insert->bbl=ret_bbl;
	      insert->next=BBL_DOMINATED_BY(ibbl);
	      BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
	      VERBOSE(0,("4-> %d ",BBL_DFS_NUMBER(insert->data)));
#endif
	      SetAddElemBlindly(BBL_DOMINATES_ARRAY(ret_bbl),ibbl);

	      if (BBL_ATTRIB(ret_bbl) & BBL_HAS_MORE_THAN_ONE_OUTGOING_DFS_EDGE || (BBL_ATTRIB(ret_bbl) & IS_EXIT_OF_LEAF))
		{
		  insert = Malloc(sizeof(t_bbl_list));
		  insert->bbl=call_bbl;
		  insert->next=BBL_DOMINATED_BY(ibbl);
		  BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
		  VERBOSE(0,("5-> %d ",BBL_DFS_NUMBER(insert->data)));
#endif

		  SetAddElemBlindly(BBL_DOMINATES_ARRAY(call_bbl),ibbl);
		}
	    }
	  else
	    {
	      insert = Malloc(sizeof(t_bbl_list));
	      insert->bbl=other_bbl;
	      insert->next=BBL_DOMINATED_BY(ibbl);
	      BBL_SET_DOMINATED_BY(ibbl, insert);
#ifdef PRINT_DFS
	      VERBOSE(0,("6-> %d ",BBL_DFS_NUMBER(insert->data)));
#endif
	      SetAddElemBlindly(BBL_DOMINATES_ARRAY(other_bbl),ibbl);
	    }
	}

      if (npreds == 1) 
	{
	  BBL_SET_ATTRIB(ibbl,    BBL_ATTRIB(ibbl) | BBL_FIXED_DOMINATORS);
#ifdef PRINT_DFS
	  printf(" FIXED");
#endif
	}
    }
  /*}}}*/

#ifdef PRINT_PROGRESSION
  printf("real start of iterative computations after %.2f seconds\n",((float)(clock()-dom_ticks))/CLOCKS_PER_SEC);fflush(stdout);
#endif		      

  while(do_it_again)
    {
     

#ifdef PRINT_PROGRESSION
      int done = 0;
#endif
      /*  VERBOSE(0,("Dominator calculation lasted %d ticks",(clock()-dom_ticks)));*/

      do_it_again = FALSE;
#ifdef PRINT_PROGRESSION
      iterations++;
#endif


      ibbl = CFG_UNIQUE_ENTRY_NODE(cfg);
#ifdef PRINT_PROGRESSION
      aangepast = 0;
#endif
      /* Now proceed in dfs preorder, by following next_in_dfs */

      while(ibbl)
	{
	  t_uint32 npreds = 0;
	  t_cfg_edge  *return_edge = NULL, *other_edge = NULL;
	  void_stack stack, stack2;
	  void_stack * working_stack[2] = {&stack,&stack2}; 
	  t_uint8 x=0,y=1;
	  t_bbl_list * prev_dominators;
	  t_cfg_edge * stack_edge = NULL;
	  t_bool removed_dfs_head = FALSE;
	  t_bool new_dfs_head = FALSE;
	  t_cfg_edge * first_mark_edge;
	  /*	  int local_can_skip = 0;
	  	  int local_cannot_skip = 0;*/

	  if (BBL_ATTRIB(ibbl) & BBL_FIXED_DOMINATORS)
	    {
	      ibbl = BBL_NEXT_IN_DFS(ibbl);
	      continue;
	    }

#ifdef WITH_REDO
	  if (!(BBL_ATTRIB(ibbl) & NEED_TO_REDO))
	    {
	      ibbl = BBL_NEXT_IN_DFS(ibbl);
	      continue;
	    }
#endif
	  /*	  VERBOSE(0,("%d doing\n",BBL_DFS_NUMBER(ibbl)));*/

	  stack.top=0;
	  stack2.top=0;

#ifdef PRINT_PROGRESSION
	  done++;

	  /* We set the stack_edge field of the bbl if this was not set before. This field avoids the recomputation 
	   * of the best suited stack_edge, since this does not change during one dominator calculation. We choose
	   * the lowest numbered predecessor as stack_edge, but in case of an incoming return edge, we choose only
	   * from the set of heads of incoming return (or compensating) edges {{{*/

#endif
	  if (BBL_ATTRIB(ibbl) & HAS_INCOMING_RETURN_EDGE)
	    {
	      if(!(BBL_STACK_EDGE(ibbl)))
		{
		  BBL_FOREACH_PRED_EDGE(ibbl,i_edge)
		    {
		      CFG_EDGE_SET_FLAGS(i_edge,    CFG_EDGE_FLAGS(i_edge)  & (~EF_CAN_SKIP));
		      
		      if(CFG_EDGE_CAT(i_edge) == ET_RETURN || CFG_EDGE_CAT(i_edge) == ET_COMPENSATING)
			{
			  if(!return_edge || BBL_DFS_NUMBER(CFG_EDGE_HEAD(return_edge)) > BBL_DFS_NUMBER(CFG_EDGE_HEAD(i_edge)))
			    return_edge = i_edge;
			}
		    }
		  BBL_SET_STACK_EDGE(ibbl,  return_edge);
		}
	      
	      stack_edge = BBL_STACK_EDGE(ibbl);
	      
#ifdef PRINT_INTERSECTION
	      printf("pushing %d %d\n",BBL_DFS_NUMBER(CFG_EDGE_HEAD(return_edge)),BBL_DFS_NUMBER(CFG_EDGE_HEAD(EDGE_CORR(return_edge))));
#endif

	      Push(working_stack[x],CFG_EDGE_HEAD(stack_edge));
	      Push(working_stack[x],CFG_EDGE_HEAD(CFG_EDGE_CORR(stack_edge)));
	    }
	  else
	    {
#ifdef WITH_SKIP
	      /* We will try to avoid unnecessary (costly) intersections. In case a bbl has a lot of
	       * incoming edges, it is quite probable that some of the predecessors dominate other
	       * predecessors. It can be cheaper to identify these cases and exclude the corresponding
	       * intersection that are needed later on, than to do intersection with all predecessors.
	       * This optimization is however based on a heuristic and is faster on average if a good
	       * value is chosen to determine 'a lot of incoming edges'. */

	      BblMarkInit2();
	      
	      if (BBL_ATTRIB(ibbl) & HAS_A_LOT_OF_INCOMING_EDGES)
		{
		  /*		  printf("upmarking\n");*/

		  BBL_FOREACH_PRED_EDGE(ibbl,first_mark_edge)
		    {
		      t_bbl * pred = CFG_EDGE_HEAD(first_mark_edge);
		      
		      MarkDominatorsUpwardsForSkippingUnnecessaryIntersections(pred, BBL_SMALLEST_PRED_DFS(ibbl));
		    }
		}
	      
	      BblMarkInit();
	      
	      if (BBL_ATTRIB(ibbl) & HAS_A_LOT_OF_INCOMING_EDGES)
		{

		  /*		  printf("downmarking\n");*/

		  BBL_FOREACH_PRED_EDGE(ibbl,first_mark_edge)
		    {
		      t_bbl * pred = CFG_EDGE_HEAD(first_mark_edge);
		      if (BblIsMarked2(pred))
			MarkDominatorsDownwardsForSkippingUnnecessaryIntersections(pred, BBL_LARGEST_PRED_DFS(ibbl));
		    }
		}
#endif
#ifndef WITH_SKIP
	      if(!(BBL_STACK_EDGE(ibbl)))
		{
#endif	     
		  BBL_FOREACH_PRED_EDGE(ibbl,i_edge)
		    {
#ifdef WITH_SKIP
		      if (BblIsMarked(CFG_EDGE_HEAD(i_edge)))
			{
			  /*			  local_can_skip++; */
			  CFG_EDGE_SET_FLAGS(i_edge,    CFG_EDGE_FLAGS(i_edge)  | EF_CAN_SKIP);
			  continue;
			}
		      
		      /*		      local_cannot_skip++;*/
		      CFG_EDGE_SET_FLAGS(i_edge,   CFG_EDGE_FLAGS(i_edge)  & (~EF_CAN_SKIP));
		      npreds++;
#endif
		      if (!other_edge || BBL_DFS_NUMBER(CFG_EDGE_HEAD(other_edge)) > BBL_DFS_NUMBER(CFG_EDGE_HEAD(i_edge))) 
			other_edge = i_edge;
		    }
		  ASSERT(other_edge || (BBL_FUNCTION(ibbl) == CFG_WRAP_FUNCTION(cfg)),("no stack edge found for @eiB",ibbl));
		  BBL_SET_STACK_EDGE(ibbl,  other_edge);
#ifndef WITH_SKIP
		}
#endif
	      /*	      if (local_cannot_skip>local_can_skip)
			      BBL_ATTRIB(ibbl) &= ~HAS_A_LOT_OF_INCOMING_EDGES; */

	      /* Only one real edge in: we found the dominators that we should remember, possibly we have two of them in case of a return edge */
	  
#ifdef WITH_SKIP
	      /* It's possible that there is only one predecessor left, because all the rest is skipped due 
	       * to the fact that the remaining predecessor dominates all the others. */
	      if (npreds==1)
		{
		  if (CFG_EDGE_HEAD(other_edge)!=T_BBL(BBL_DOMINATED_BY(ibbl)->bbl))
		    {
		      BBL_DOMINATED_BY(ibbl)->bbl=CFG_EDGE_HEAD(other_edge);
		      do_it_again = TRUE;
		    }
		  
#ifdef NEED_TO_REDO
		  BBL_SET_ATTRIB(ibbl,      BBL_ATTRIB(ibbl)  & (~NEED_TO_REDO));
#endif

		  ibbl = BBL_NEXT_IN_DFS(ibbl);
		  continue;
		}
#endif
	      /* the wrapper function doesn't have predecessors */
	      if (BBL_FUNCTION(ibbl) != CFG_WRAP_FUNCTION(cfg))
	      {
		stack_edge = BBL_STACK_EDGE(ibbl);
		Push(working_stack[x],CFG_EDGE_HEAD(stack_edge));
	      }
	    }
	  /*}}}*/
	  
#ifdef PRINT_INTERSECTION
	  VERBOSE(0,("\nStackedge = @E",stack_edge));
#endif

#ifdef NEED_TO_REDO
	  BBL_SET_ATTRIB(ibbl,   BBL_ATTRIB(ibbl)  & (~NEED_TO_REDO));
#endif

	  /* Stack edge is selected and it's head is pushed on working_stack[x]. Now
	   * we start the real intersection by iterating over all other predecessors. */
	  
	  BBL_FOREACH_PRED_EDGE(ibbl,first_mark_edge)
	    {
	      t_uint32 i;
	      t_uint32 stop_number = 0xfffffff;
	      t_bool reached_domtree_root;

	      if (first_mark_edge == stack_edge) continue;

#ifdef WITH_SKIP
	      if (CFG_EDGE_FLAGS(first_mark_edge) & EF_CAN_SKIP) continue;
#endif

	      BblMarkInit();
	      BblMarkInit2();

	      for(i=0;i<working_stack[x]->top;i++)
		{
		  if (BBL_DFS_NUMBER(working_stack[x]->stack[i])<stop_number)
		    stop_number = BBL_DFS_NUMBER(working_stack[x]->stack[i]);
		  BblMark2(working_stack[x]->stack[i]);
		}
	      
	      /*VERBOSE(0,("init %d %d\n",BBL_DFS_NUMBER(ibbl),stop_number));*/
	      
#ifdef PRINT_INTERSECTION
	      VERBOSE(0,("Markedge = @E",first_mark_edge));
#endif

	      /* Start marking upwards in the intermediate dominator tree but stop marking when the DFS numbers of
	       * the node fall below the stop_number. It makes no sense to continue marking in that case, because
	       * we will never mark a node that is currently on working_stack[x]. */
	      if(CFG_EDGE_CAT(first_mark_edge) == ET_RETURN || CFG_EDGE_CAT(first_mark_edge) == ET_COMPENSATING)
		reached_domtree_root = MarkDominatorsUpwardsUntilStop(first_mark_edge,TRUE,stop_number,CFG_EDGE_CORR(first_mark_edge));
	      else
		reached_domtree_root = MarkDominatorsUpwardsUntilStop(first_mark_edge,TRUE,stop_number,NULL);
	      
	      while(VSTACK_NOT_EMPTY(working_stack[x]))
		{
		  /*                          StackPrint(working_stack[x]);*/
		  
		  t_bbl * curr_bbl = Pop(working_stack[x]);
		 
#ifdef PRINT_INTERSECTION
		  VERBOSE(0,("Popped number %d",BBL_DFS_NUMBER(curr_bbl)));
#endif	      
 
		black_label:
		  
		  /* If we reached a common point in the dom-graph, we can stop */

		  /* If the stop_number decreases, it makes sense again to do upwards marking */
		  if (!reached_domtree_root 
		      && !BblIsMarkedDominator(curr_bbl) 
		      && BBL_DFS_NUMBER(curr_bbl)<stop_number
		      && BBL_DOMINATES_ARRAY(curr_bbl)->nr_elements>1
		      )
		    {
		      stop_number = BBL_DFS_NUMBER(curr_bbl);

#ifdef PRINT_INTERSECTION
		      VERBOSE(0,("adapt %d %d\n",BBL_DFS_NUMBER(ibbl),stop_number));
#endif
		      reached_domtree_root = MarkDominatorsUpwardsUntilStop(first_mark_edge,FALSE,stop_number,NULL);
		    }
		  
		  /* If we reached a common point in the dom-graph, we can stop */

		  if(BblIsMarkedDominator(curr_bbl))
		    {
		      if(!BblIsMarked(curr_bbl))
			{
			  BblMark(curr_bbl);
			  Push(working_stack[y],curr_bbl);
#ifdef PRINT_INTERSECTION
			  VERBOSE(0,("Pushed number %d on stack y",BBL_DFS_NUMBER(curr_bbl)));
#endif			    
			}
		    }
		  else
		    {
		      t_bbl_list * iter_dom = BBL_DOMINATED_BY(curr_bbl);
		      
		      t_bbl * this_bbl;
		      curr_bbl = NULL;
		      
		      while(iter_dom)
			{
			  this_bbl = iter_dom->bbl;
			  iter_dom = DomNext(iter_dom);
			  
			  if(!BblIsMarked2(this_bbl))
			    { 
			      if (BBL_MIN_DFS(this_bbl)<BBL_DFS_NUMBER(ibbl))
				{
				  curr_bbl = this_bbl;
				  BblMark2(this_bbl);
				  break;
				}
			    }
			}
		      
		      
		      while(iter_dom)
			{
			  this_bbl = iter_dom->bbl;
			  iter_dom = DomNext(iter_dom);
			  
			  if(!BblIsMarked2(this_bbl))
			    {
			      if (BBL_MIN_DFS(this_bbl)<BBL_DFS_NUMBER(ibbl))
				{
				  Push(working_stack[x],this_bbl);
				  BblMark2(this_bbl);
#ifdef PRINT_INTERSECTION
				  VERBOSE(0,("Pushed number %d on stack x",BBL_DFS_NUMBER(this_bbl)));
#endif
				}
			    }
			}
		      
		      if (curr_bbl)
			{
#ifdef PRINT_INTERSECTION
			  VERBOSE(0,("goto %d \n",BBL_DFS_NUMBER(curr_bbl)));
#endif
			  goto black_label;
			}
		    }
		}
	      
	      x= 1-x;
	      y= 1-y;
	      /*          VERBOSE(0,("Switch x=%d y=%d",x,y));*/
	    }
	      
	      /*        VERBOSE(0,("We found some dominators for @B",ibbl));*/
	      
	      /* Now compare them with the existing dominators to see if anything has changed */
	  
	      /* Keep the previous dominators in a list */
	  prev_dominators = BBL_DOMINATED_BY(ibbl);
	  BBL_SET_DOMINATED_BY(ibbl,  NULL);
      
#ifdef PRINT_COMPUTATION
	  printf("%d -----------------------\n", BBL_DFS_NUMBER(ibbl));
#endif	      

	  while(VSTACK_NOT_EMPTY(working_stack[x]))
	    {
	      t_bbl_list * iter_prev_dom = prev_dominators;
	      t_bbl_list * prev_iter_prev_dom = NULL;
	      t_bool can_stay = FALSE;
	      
	      t_bbl * dom = Pop(working_stack[x]);
	      /* Start looking for dom in the list and move it to the new dominator list of ibbl */
	      /*          VERBOSE(0,("\t@B",dom));*/
	      
	      if (dom==NULL) continue;
	      
	      while(iter_prev_dom)
		{
		  if(iter_prev_dom->bbl == dom)
		    {
		      can_stay = TRUE;
		      break;
		    }
		  prev_iter_prev_dom = iter_prev_dom;
		  iter_prev_dom = DomNext(iter_prev_dom);
		}
	      
	      if(can_stay)
		{
		  if (prev_iter_prev_dom) prev_iter_prev_dom->next=iter_prev_dom->next;
		  else prev_dominators=iter_prev_dom->next;
#ifdef PRINT_COMPUTATION
		  printf("%d vorig: %d \n",BBL_DFS_NUMBER(ibbl),BBL_DFS_NUMBER(dom));
#endif

		  iter_prev_dom->next=BBL_DOMINATED_BY(ibbl);
		  BBL_SET_DOMINATED_BY(ibbl, iter_prev_dom);
		}
	      else
		{
		  /* Create a new list item */
		  iter_prev_dom = (t_bbl_list *) Calloc(1,sizeof(t_bbl_list));
		  iter_prev_dom->bbl=dom;
		  iter_prev_dom->next=BBL_DOMINATED_BY(ibbl);
		  BBL_SET_DOMINATED_BY(ibbl, iter_prev_dom);
		  SetAddElemBlindly(BBL_DOMINATES_ARRAY(dom),ibbl);

#ifdef PRINT_COMPUTATION
		  printf("%d nieuw: %d \n",BBL_DFS_NUMBER(ibbl),BBL_DFS_NUMBER(dom));
#endif
		  

		  new_dfs_head = TRUE;
		}
	    }

	  if(prev_dominators)
	    {
	      t_bbl_list * freeme;
	      
#ifdef WITH_REDO
	      t_bbl_list * iter = BBL_DOMINATED_BY(ibbl);
	      	      
	      BblMarkInitDominator();

	      while (iter)
		{
		  BblMarkDominator(iter->bbl);
		  MarkDominatorsUpwards(iter->bbl);
		  iter = DomNext(iter);
		}
#endif

	      while(prev_dominators)
		{
		  removed_dfs_head = TRUE;

#ifdef PRINT_COMPUTATION
		  printf("%d weggelaten: %d \n",BBL_DFS_NUMBER(ibbl),BBL_DFS_NUMBER(prev_dominators->data));
#endif
	
		  RemoveBblFromDominatesOf(ibbl,prev_dominators->bbl);

		  freeme = prev_dominators;
		  
#ifdef WITH_REDO
		  MarkRemovedDominatorsUpWardsAndNumber(prev_dominators->bbl,ibbl);
#endif

		  prev_dominators = DomNext(prev_dominators);
		  Free(freeme);
		}
	    }
	 
 
	  if (removed_dfs_head || new_dfs_head)
	    {
	      do_it_again = TRUE;
#ifdef PRINT_PROGRESSION
	      aangepast++;
#endif
	    }

	  ibbl = BBL_NEXT_IN_DFS(ibbl);
	}
      /* temp berekening van te veranderen knopen */

#ifdef PRINT_PROGRESSION
      printf("iteration: %d aangepast: %d (%d total) done %d (%d total) after %.2f seconds\n",iterations,aangepast,total_aangepast+=aangepast,done,total_done+=done,((float)(clock()-dom_ticks))/CLOCKS_PER_SEC);
#endif
      if (do_it_again)
	{
	  
#ifdef WITH_REDO
	  ibbl = CFG_UNIQUE_ENTRY_NODE(cfg);
	  
	  while (ibbl)
	    {
	      if (BBL_NR_DOMINATED(ibbl)!=0xffffffff)
		{
		  int count , i;
		  t_set * dominated_blocks = BBL_DOMINATES_ARRAY(ibbl);

		  for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
		    {
		      t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
		      if (dominated == NULL) continue;
		      if (!(BBL_ATTRIB(dominated) & BBL_FIXED_DOMINATORS))
			if (BBL_NR_DOMINATED(ibbl)<=BBL_LARGEST_PRED_DFS(dominated))
			  {
#ifdef COMPUTATION
			    printf("need to redo %d because %d was removed from %d\n",BBL_DFS_NUMBER(dominated),BBL_DFS_NUMBER(ibbl),BBL_NR_DOMINATED(ibbl));
#endif
			    BBL_SET_ATTRIB(dominated,      BBL_ATTRIB(dominated)| NEED_TO_REDO);
			  }
		      count++;
		    }
		}

	      BBL_SET_NR_DOMINATED(ibbl,  0xffffffff);
	      ibbl = BBL_NEXT_IN_DFS(ibbl);
	    }
#endif	  
	}
    }

  /* now remove redundant entries in graph */

  ibbl = CFG_UNIQUE_ENTRY_NODE(cfg);
  while (ibbl && BBL_DFS_NUMBER(ibbl)<1000)
    {
      if (BBL_DOMINATES_ARRAY(ibbl)->nr_elements>100)
	RemovetransitiveDown(ibbl);
      ibbl = BBL_NEXT_IN_DFS(ibbl);      
    }

  ibbl = CFG_UNIQUE_ENTRY_NODE(cfg);
  
  while (ibbl)
    {
      RemovetransitiveNew(ibbl);
      ibbl = BBL_NEXT_IN_DFS(ibbl);
    }
    
  /*{{{*/

#ifdef OPT_DOMINATES2
  CFG_FOREACH_BBL(cfg,ibbl)
    BBL_ATTRIB(ibbl) &= ~BBL_CAN_BE_DOMINATOR;
  
  CFG_FOREACH_BBL(cfg,ibbl)
    {
      data_ll_elem * iter = BBL_DOMINATED_BY(ibbl);
      while (iter)
	{
	  BBL_ATTRIB(iter->data) |= BBL_CAN_BE_DOMINATOR;
	  iter = DomNext(iter);
	}
    }
#endif
  /*}}}*/

/*  dom_ticks2 = (clock()-dom_ticks);*/

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
    {
      t_set * set = BBL_DOMINATES_ARRAY(ibbl);
      if (set)
      {
	teller_edges+=set->nr_elements;
      }
    }

/*  VERBOSE(0,("Seconds, blocks, edges, ticks/block, ticks/edge, edges in dom graph, per block, edges per block"));*/
/*  VERBOSE(0,("GREPME %.2f, %d, %d, %d, %d, %d, %1.2f, %1.2f",((float)dom_ticks2)/CLOCKS_PER_SEC,CFG_NBBLS(cfg),CFG_NEDGES(cfg),dom_ticks2/CFG_NBBLS(cfg),dom_ticks2/CFG_NEDGES(cfg),teller_edges,((float)teller_edges)/(float)CFG_NBBLS(cfg) , ((float)CFG_NEDGES(cfg))/((float)CFG_NBBLS(cfg))));*/

#ifdef ENABLE_STATISTICS
  GetStatistics(cfg);
#endif
/*    exit(0);*/
/*  }*/

  BblFiniDominatesArray(cfg);
  BblFiniNextInDfs(cfg);
  BblFiniStackEdge(cfg);
  BblFiniNrDominated(cfg);
  BblFiniLargestPredDfs(cfg);
  BblFiniSmallestPredDfs(cfg);
  BblFiniMinDfs(cfg);

  STATUS(START,("Simple Loop detection"));
  /*{{{ Special edges are drawn between nodes without successors and the unique exit node, and between the headers of an infinite loop
   * and the unique exit node. TODO: implement fast postdom and investigate if this piece of code can disappear. */
  if (CfgDetectSimpleInfiniteLoops(cfg,&looplist))
    {
      t_loopref * inf_iter = NULL;
      t_cfg_edge * new_edge = NULL;
      t_uint32 inf_loops = 0;
      while(looplist)
	{
	  inf_iter = looplist;
	  new_edge = CfgEdgeCreate(cfg,CFG_EDGE_TAIL(inf_iter->loop->backedges->edge),CFG_UNIQUE_EXIT_NODE(cfg),ET_POSTDOM);
	 /* LoopPrint(looplist->loop,0); */
	  looplist = looplist->next;
	  Free(inf_iter);
	  inf_loops++;
	}
      VERBOSE(0,("%d infinite loops",inf_loops));
    }

  BblFiniDomMarkingNumber(cfg);
#if 0
  CFG_FOREACH_FUN(cfg,ifun)
    FUNCTION_FOREACH_BBL(ifun,ibbl)
    {
      if(ibbl == CFG_UNIQUE_EXIT_NODE(cfg)) continue;
      if(!BBL_SUCC_FIRST(ibbl))
	{
	  /* This bbl has no edges out, insert a special edge to the unique exit */
	  VERBOSE(0,("Connecting @B with unique exit node",ibbl));
	  CfgEdgeCreate(cfg,ibbl,CFG_UNIQUE_EXIT_NODE(cfg),ET_POSTDOM);
	}
    }
#endif
  /*}}}*/
  STATUS(STOP,("Simple Loop detection"));
/*  VERBOSE(0,("Loop calculation lasted %.2f seconds",((float)(clock()-dom_ticks2-dom_ticks)/CLOCKS_PER_SEC)));*/

#ifdef POST_DOMINATORS_IMPLEMENTED /* FIXME: Temporarily disabled because it currently is not adjusted to the new dominator calculation yet */
  STATUS(START,("Post-dominator information"));
  /*{{{ iterate until fixpoint, postdominators */

#ifdef DEBUG_DOMINATOR
  VERBOSE(0,("%x is de unique exit\n",CFG_UNIQUE_EXIT_NODE(cfg)));
  VERBOSE(0,("%x is ALL_BBLS\n",ALL_BBLS));
#endif

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
    {
      BBL_ATTRIB(ibbl) &= ~BBL_FIXED_DOMINATORS;
      BBL_ATTRIB(ibbl) &= ~IS_EXIT_OF_LEAF;
      BBL_ATTRIB(ibbl) &= ~HAS_INCOMING_RETURN_EDGE;
      BBL_SET_PDOMINATES_ARRAY(ibbl,  SetNew(2));

    }

  DominatorPDFSNumbering(cfg);

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
      BblSortAndCountPredAndSuccEdgesUsingPDfs(ibbl);

  /* Initialisation with pdfs predecessor and return site if necessary {{{*/
  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,ibbl)
    {
      t_bbl * ret_bbl = NULL, *call_bbl = NULL, *other_bbl = NULL;
      t_bbl_list * insert;
      t_uint32 npreds = 0;

      if (BBL_PDFS_NUMBER(ibbl) <= 1 )
	{
	  VERBOSE(0,("@B is not numbered!",ibbl));
	  continue;
	}
      else if (ibbl==CFG_SWI_HELL_NODE(cfg))
	{
	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=CFG_EXIT_SWI_HELL_NODE(cfg);
	  insert->next=BBL_PDOMINATED_BY(ibbl);
	  BBL_SET_PDOMINATED_BY(ibbl, insert);

	  SetAddElemBlindly(BBL_PDOMINATES_ARRAY(CFG_EXIT_SWI_HELL_NODE(cfg)),ibbl);
	  BBL_ATTRIB(ibbl) |= BBL_FIXED_DOMINATORS;
	  continue;
	}
#ifdef PRINT_PDFS
      else VERBOSE(0,("\nPDFSNR: %d ",BBL_PDFS_NUMBER(ibbl)));
#endif
      BBL_FOREACH_SUCC_EDGE(ibbl,i_edge)
	{
	  if(EDGE_CAT(i_edge) == ET_CALL || EDGE_CAT(i_edge) == ET_IPJUMP)
	    {
	      BBL_ATTRIB(ibbl) |= HAS_OUTGOING_CALL_EDGE;
	      if(BBL_PDFS_NUMBER(CFG_EDGE_TAIL(i_edge)) < BBL_PDFS_NUMBER(ibbl))
	      {
		if(EDGE_CORR(i_edge))
		{
		  if(!ret_bbl || BBL_PDFS_NUMBER(ret_bbl) > BBL_PDFS_NUMBER(CFG_EDGE_TAIL(EDGE_CORR(i_edge))))
		    ret_bbl = CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge));
		}
		call_bbl = CFG_EDGE_TAIL(i_edge);
	      }
	    }
	  else if (!other_bbl) 
	    other_bbl = CFG_EDGE_TAIL(i_edge);
	  /* TODO: check < or > */
	  else if (BBL_PDFS_NUMBER(other_bbl) > BBL_PDFS_NUMBER(CFG_EDGE_TAIL(i_edge))) 
	    other_bbl = CFG_EDGE_TAIL(i_edge);
	  npreds++;
	}

      /* If there are only outgoing call or ipjump edges */
      if(!other_bbl)
      {
	if(!call_bbl) FATAL(("Trubles! @ieB",ibbl));

	insert = Malloc(sizeof(t_bbl_list));
	insert->bbl=call_bbl;
	insert->next=BBL_PDOMINATED_BY(ibbl);
	BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	VERBOSE(0,("1-> %d ",BBL_PDFS_NUMBER(call_bbl)));
#endif
	SetAddElemBlindly(BBL_PDOMINATES_ARRAY(call_bbl),ibbl);

	if (ret_bbl && (BBL_ATTRIB(call_bbl) & BBL_HAS_MORE_THAN_ONE_INCOMING_PDFS_EDGE || (BBL_ATTRIB(call_bbl) & IS_EXIT_OF_LEAF)))
	{
	  /* store the return-site because this function has more than one callsite */
          insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=ret_bbl;
	  insert->next=BBL_PDOMINATED_BY(ibbl);
	  BBL_SET_PDOMINATED_BY(ibbl, insert);
	  
#ifdef PRINT_PDFS
	  VERBOSE(0,("2-> %d ",BBL_PDFS_NUMBER(ret_bbl)));
#endif
	  SetAddElemBlindly(BBL_PDOMINATES_ARRAY(ret_bbl),ibbl);
	}
      }
      else if (!call_bbl)
      {
	if(!other_bbl) FATAL(("Trubles!"));


	insert = Malloc(sizeof(t_bbl_list));
	insert->bbl=other_bbl;
	insert->next=BBL_PDOMINATED_BY(ibbl);
	BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	VERBOSE(0,("3-> %d ",BBL_PDFS_NUMBER(other_bbl)));
#endif
	SetAddElemBlindly(BBL_PDOMINATES_ARRAY(other_bbl),ibbl);
      }
      else
      {
	
	if(ret_bbl && BBL_PDFS_NUMBER(ret_bbl)<BBL_PDFS_NUMBER(other_bbl))
	{
	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=call_bbl;
	  insert->next=BBL_PDOMINATED_BY(ibbl);
	  BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	  VERBOSE(0,("4-> %d ",BBL_PDFS_NUMBER(insert->bbl)));
#endif
	  SetAddElemBlindly(BBL_PDOMINATES_ARRAY(call_bbl),ibbl);

	  if (BBL_ATTRIB(call_bbl) & BBL_HAS_MORE_THAN_ONE_INCOMING_PDFS_EDGE || (BBL_ATTRIB(ret_bbl) & IS_EXIT_OF_LEAF))
	  {
	    insert = Malloc(sizeof(t_bbl_list));
	    insert->bbl=ret_bbl;
	    insert->next=BBL_PDOMINATED_BY(ibbl);
	    BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	    VERBOSE(0,("5-> %d ",BBL_PDFS_NUMBER(insert->bbl)));
#endif

	    SetAddElemBlindly(BBL_PDOMINATES_ARRAY(ret_bbl),ibbl);
	  }
	}
	/* It can occur that there is no ret_bbl, then we should only test whether call < other */
	else if(!ret_bbl && BBL_PDFS_NUMBER(call_bbl)<BBL_PDFS_NUMBER(other_bbl))
	{
	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=call_bbl;
	  insert->next=BBL_PDOMINATED_BY(ibbl);
	  BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	  VERBOSE(0,("7-> %d ",BBL_PDFS_NUMBER(insert->bbl)));
#endif
	  SetAddElemBlindly(BBL_PDOMINATES_ARRAY(call_bbl),ibbl);
	}
	else
	{
	  insert = Malloc(sizeof(t_bbl_list));
	  insert->bbl=other_bbl;
	  insert->next=BBL_PDOMINATED_BY(ibbl);
	  BBL_SET_PDOMINATED_BY(ibbl, insert);
#ifdef PRINT_PDFS
	  VERBOSE(0,("6-> %d ",BBL_PDFS_NUMBER(insert->bbl)));
#endif
	  SetAddElemBlindly(BBL_PDOMINATES_ARRAY(other_bbl),ibbl);
	}
      }
      if(npreds == 1)
      {
	BBL_ATTRIB(ibbl) |= BBL_FIXED_DOMINATORS;
#ifdef PRINT_PDFS
	  printf(" FIXED");
#endif
      }
    }
  /*}}}*/

  /* }}}*/
  STATUS(STOP,("Post-dominator information"));
  exit(0);
#endif

  /* remove the edges to the unique exit again, because they could interfere
   * with other analysis and optimizations */
  BBL_FOREACH_PRED_EDGE_SAFE(CFG_UNIQUE_EXIT_NODE(cfg),i_edge,j_edge)
    if(CFG_EDGE_CAT(i_edge) == ET_POSTDOM)
      CfgEdgeKill(i_edge);

  STATUS(STOP,("Dominator calculation"));

  dominator_info_correct = TRUE;

  return;
}


static void RemoveBblFromDominatesOf(t_bbl * bbl, t_bbl * from)
{
  SetRemoveElem(BBL_DOMINATES_ARRAY(from),bbl);
}


/*! \param Two bbl's
 *  \ret t_bool
 *
 *  Return TRUE if dominator dominates dominated, otherwise FALSE */
t_bool BblDominates(t_bbl * dominator, t_bbl * dominated)
{
  t_bbl_queue dom_queue = {NULL,NULL};
  t_bbl * entry_node = CFG_UNIQUE_ENTRY_NODE(BBL_CFG(dominated));
  t_bbl * orig_dominated = dominated;
  t_bbl_list * elem, * tmp_dll;
  static t_bbl * prev_or = NULL, *prev_ed = NULL;
  static t_bool prev_result = FALSE;
  t_bbl_list * iter;

  t_bbl_list * iter2, *iter3;
  int teller;

  if (dominator == prev_or && dominated == prev_ed) 
    {
      return prev_result;
    }
  
  if (BBL_DFS_NUMBER(dominator)>BBL_DFS_NUMBER(dominated))
    {
      prev_or = dominator;
      prev_ed = dominated;
      prev_result = FALSE;
      return prev_result;
    }


#ifdef OPT_DOMINATES2
  if (!(BBL_ATTRIB(dominator) & BBL_CAN_BE_DOMINATOR))
    if (dominator!=dominated) 
      {
	return FALSE;
      }
#endif
  
  if(dominator == dominated)
    {
      prev_or = dominator;
      prev_ed = dominated;
      prev_result = TRUE;
      return prev_result;
    }

  iter = BBL_DOMINATED_BY(dominated);

 retry_first:
  iter2=iter;
  teller = 0;

  while(iter)
    {
      if(iter->bbl == dominator)
	{
	  prev_or = dominator;
	  prev_ed = orig_dominated;
	  prev_result = TRUE;
	  return prev_result;
	}

      teller++;

      iter = DomNext(iter);
    }

  if (teller==1 && iter2->bbl != entry_node)
    {
      dominated = iter2->bbl;
      iter = BBL_DOMINATED_BY(dominated);
      goto retry_first;
    }

  BblMarkInitDominator();

  iter = BBL_DOMINATED_BY(dominated);

  while(iter)
    {
      if(iter->bbl != entry_node)
	{
	  BblMarkDominator(iter->bbl);
	  tmp_dll = (t_bbl_list *)Calloc(1,sizeof(t_bbl_list));
	  tmp_dll->bbl = iter->bbl;
	  tmp_dll->next=dom_queue.start;
	  dom_queue.start=tmp_dll;
	}
      iter = DomNext(iter);
    }

  while(dom_queue.start)
    {
      t_bbl * prev_bbl ;
      elem = (t_bbl_list*)dom_queue.start;
      dom_queue.start=dom_queue.start->next;
      iter = BBL_DOMINATED_BY(elem->bbl);
      prev_bbl = elem->bbl;
    retry:
    
      iter3 = iter;
      teller = 0;
      
      while(iter)
	{
	  if (!BblIsMarkedDominator(iter->bbl))
	    {
	      teller++;
	      if(iter->bbl == dominator)
		{
		  if (elem!=NULL)
		    {
		      Free(elem);
		      elem=NULL;
		    }
		  goto cleanup_true;
		}
	      iter2 = iter;
	    }
	  iter = DomNext(iter);
	}
      
      if (teller==1 && iter2->bbl != entry_node)
	{
	  BblMarkDominator(iter2->bbl);
	  prev_bbl = iter2->bbl;
	  iter = BBL_DOMINATED_BY(iter2->bbl);
	  if (elem!=NULL)
	    {
	      Free(elem);
	      elem = NULL;
	    }
	  goto retry;
	}
      iter = iter3;

      while(iter)
	{
	  if(!BblIsMarkedDominator(iter->bbl) && T_BBL(iter->bbl) != entry_node)
	    {
	      BblMarkDominator(iter->bbl);
	      tmp_dll = (t_bbl_list *)Calloc(1,sizeof(t_bbl_list));
	      tmp_dll->bbl = iter->bbl;
	      tmp_dll->next=dom_queue.start;
	      dom_queue.start=tmp_dll;
	    }
	  iter = DomNext(iter);
	}
      if (elem!=NULL)
	{
	  Free(elem);
	  elem = NULL;
	}
    }
  
  prev_or = dominator;
  prev_ed = orig_dominated;
  prev_result = FALSE;
  return prev_result;

 cleanup_true:
  while(dom_queue.start)
    {
      elem = dom_queue.start;
      dom_queue.start=dom_queue.start->next;
      Free(elem);
    }
  prev_or = dominator;
  prev_ed = orig_dominated;
  prev_result = TRUE;
  return prev_result;

}

/*! \param Two bbl's
 *  \ret t_bool
 *
 *  Return TRUE if postdominator postdominates postdominated, otherwise FALSE */
t_bool BblPostdominates(t_bbl * postdominator, t_bbl * postdominated)
{
  /* TODO implement */
  return FALSE;
}


/*{{{ Graph export functions*/
static t_bool interproc = FALSE;
 
static t_bool node_writeout_dominator(t_node * current, t_node * parent,void * data) {
  /* print out dominator of current node */
  /* used for exporting the flowgraph */
  FILE * out=data;
  /*  data_ll_elem * iter;*/
  /*  t_bbl * i_bbl;*/

  /* Write out node name of current{{{*/
  FileIo(out, "\t\"%d\" [label=\"",BBL_DFS_NUMBER(T_BBL(current)));
  if (T_BBL(current) ==  CFG_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out,"HELL");
    } 
  else if (T_BBL(current) == CFG_CALL_HELL_NODE(BBL_CFG(T_BBL(current)))) 
    {
      FileIo(out, "CALL HELL");
    } 
  else if (BBL_CALL_HELL_TYPE(T_BBL(current)))
  {
    FileIo(out, "DYNAMIC CALL: %s", FUNCTION_NAME(BBL_FUNCTION(T_BBL(current)))+16);
  }
  else if (T_BBL(current) == CFG_EXIT_CALL_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "EXIT CALL HELL");
    } 
  else if (FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(T_BBL(current))) &&
           current == T_NODE(FUNCTION_BBL_LAST(BBL_FUNCTION(T_BBL(current)))))
  {
    FileIo(out, "EXIT DYNAMIC CALL: %s", FUNCTION_NAME(BBL_FUNCTION(T_BBL(current)))+16);
  }
  else if (T_BBL(current) == CFG_EXIT_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "EXIT HELL");
    } 
  else if (T_BBL(current) == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "UNIQUE ENTRY");
    } 
  else if (T_BBL(current) == CFG_UNIQUE_EXIT_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "UNIQUE EXIT");
    }
  else if (BBL_IS_LAST(T_BBL(current)) && !BBL_NINS(T_BBL(current)))
    { /* return block */
      FileIo(out, "%d: RETURN FROM %s", BBL_DFS_NUMBER(T_BBL(current)),FUNCTION_NAME(BBL_FUNCTION(T_BBL(current))));
    } 
  else 
    {
      FileIo(out, "%d: @G\\n", BBL_DFS_NUMBER(T_BBL(current)),BBL_OLD_ADDRESS(T_BBL(current)));
    }

  FileIo(out,"\"]\n");/*}}}*/
 
  if (T_BBL(current) != CFG_UNIQUE_ENTRY_NODE(BBL_CFG(T_BBL(current))))
    {
      t_bbl_list * iter = BBL_DOMINATED_BY(T_BBL(current));
      while(iter)
	{
	  FileIo(out,"\t\"%d\" -> \"%d\" [style=%s,color=%s];\n",BBL_DFS_NUMBER(iter->bbl),BBL_DFS_NUMBER(T_BBL(current)),"solid",(BBL_FUNCTION(iter->bbl) == BBL_FUNCTION(T_BBL(current)))?"black":"red");
	  /*      if(BBL_FUNCTION(iter->data) != BBL_FUNCTION(current) && iter->data != ALL_BBLS)*/
	  /*        FileIo(out,"\t\"%d\" [label=\"%d: %s\",style=filled, color=yellow]\n",BBL_DFS_NUMBER(iter->data),BBL_DFS_NUMBER(iter->data),FUNCTION_NAME(BBL_FUNCTION(iter->data)));*/
	  iter = DomNext(iter);
	}
    }

  return FALSE;
}

#if 0
static t_bool node_writeout_postdominator(t_node * current, t_node * parent,void * data) {
  /* print out postdominator of current node */
  /* used for exportating the flowgraph */
  FILE * out=data;

  /* Write out node name of current{{{*/
  FileIo(out, "\t\"%p\" [label=\"",current);
  if (T_BBL(current) ==  CFG_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out,"HELL");
    } 
  else if (T_BBL(current) == CFG_CALL_HELL_NODE(BBL_CFG(T_BBL(current)))) 
    {
      FileIo(out, "CALL HELL");
    } 
  else if (T_BBL(current) == CFG_EXIT_CALL_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "EXIT CALL HELL");
    } 
  else if (T_BBL(current) == CFG_EXIT_HELL_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "EXIT HELL");
    } 
  else if (T_BBL(current) == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "UNIQUE ENTRY");
    } 
  else if (T_BBL(current) == CFG_UNIQUE_EXIT_NODE(BBL_CFG(T_BBL(current))))
    {
      FileIo(out, "UNIQUE EXIT");
    }
  else if (BBL_IS_LAST(T_BBL(current)) && !BBL_NINS(T_BBL(current)))
    { /* return block */
      FileIo(out, "RETURN FROM F%p", BBL_FUNCTION(T_BBL(current)));
    } 
  else 
    {
      FileIo(out, "@G\\n", BBL_OLD_ADDRESS(T_BBL(current)));
    }

  FileIo(out,"\"]\n");/*}}}*/
 
  if (T_BBL(current) != CFG_UNIQUE_EXIT_NODE(BBL_CFG(T_BBL(current))))
    {
      t_bbl_list * iter = BBL_P_DOMINATED_BY(T_BBL(current));
      while(iter)
	{
	  FileIo(out,"\t\"%p\" -> \"%p\" [style=%s,color=%s];\n",iter->bbl,current,"solid",(BBL_FUNCTION(iter->bbl) == BBL_FUNCTION(T_BBL(current)))?"black":"red");
	  if(BBL_FUNCTION(iter->bbl) != BBL_FUNCTION(T_BBL(current)) && iter->bbl != ALL_BBLS)
	    FileIo(out,"\t\"%p\" [label=\"@G: %s\",style=filled, color=yellow]\n",iter->bbl,BBL_OLD_ADDRESS(iter->bbl),FUNCTION_NAME(BBL_FUNCTION(iter->bbl)));
	  iter = DomNext(iter);
	}
#if 0
      if(BBL_PDOMINATED_BY(current)[0])
	{
	  FileIo(out,"\t\"%p\" -> \"%p\" [style=%s,color=%s];\n",BBL_PDOMINATED_BY(current)[0],current,
		      "solid",
		      (BBL_FUNCTION(BBL_PDOMINATED_BY(current)[0]) == BBL_FUNCTION(current))?"black":"red");

	  if (BBL_FUNCTION(BBL_PDOMINATED_BY(current)[0]) != BBL_FUNCTION(current) && /*!interproc &&*/ BBL_PDOMINATED_BY(current)[0] != ALL_BBLS)
	    FileIo(out,"\t\"%p\" [label=\"@G: %s\",style=filled, color=yellow]\n",
			BBL_PDOMINATED_BY(current)[0], BBL_OLD_ADDRESS(BBL_PDOMINATED_BY(current)[0]),
			FUNCTION_NAME(BBL_FUNCTION(BBL_PDOMINATED_BY(current)[0])));
	}
      if(BBL_PDOMINATED_BY(current)[1])
	{
	  FileIo(out,"\t\"%p\" -> \"%p\" [style=%s,color=%s];\n",BBL_PDOMINATED_BY(current)[1],current,
		      "solid",
		      (BBL_FUNCTION(BBL_PDOMINATED_BY(current)[1]) == BBL_FUNCTION(current))?"black":"red");

	  if (BBL_FUNCTION(BBL_PDOMINATED_BY(current)[1]) != BBL_FUNCTION(current) && /*!interproc &&*/ BBL_PDOMINATED_BY(current)[1] != ALL_BBLS)
	    FileIo(out,"\t\"%p\" [label=\"@G: %s\",style=filled, color=yellow]\n",
			BBL_PDOMINATED_BY(current)[1], BBL_OLD_ADDRESS(BBL_PDOMINATED_BY(current)[1]),
			FUNCTION_NAME(BBL_FUNCTION(BBL_PDOMINATED_BY(current)[1])));
	}
#endif
      /*BBL_FOREACH_PDOMINATES(T_BBL(current),iter,i_bbl)
	{
      
	if(BBL_FUNCTION(i_bbl) && BBL_FUNCTION(i_bbl) == BBL_FUNCTION(T_BBL(current)) && (T_BBL(current) != i_bbl))
	FileIo(out,"\t\"%p\" -> \"%p\" [style=%s,color=%s];\n",i_bbl,current,
	"solid",
	"black");

	}*/
    }

  return FALSE;
}
#endif

void Export_FunctionDominator(t_bbl * start, t_uint32 mask, const char * filename)
{
  FILE * out = fopen(filename,"w");
  t_cfg * cfg = BBL_CFG(start);
  t_cfg_edge * edge = CfgEdgeCreate(cfg,CFG_SWI_HELL_NODE(cfg),CFG_EXIT_SWI_HELL_NODE(cfg),ET_JUMP);

  if (mask & ET_INTERPROC) interproc = TRUE;
  else interproc = FALSE;
  ASSERT(out,("Could not open %s for writing!",filename));
  FileIo(out, "digraph \"@G\" {\n\tnode [shape=ellipse]\n",BBL_OLD_ADDRESS(start));
  GraphDFTraversalWithCheckAndData((t_node *) start,node_writeout_dominator, mask, ET_CALL|ET_SWI,out);
  if(interproc) FileIo(out, "\t\"%p\" [label=\"ALL BBLS\"]\n",ALL_BBLS);
  FileIo(out, "}\n");
  CfgEdgeKill(edge);
  fclose(out);
}

#ifdef POST_DOMINATORS_IMPLEMENTED
void Export_FunctionPostDominator(t_bbl * start, t_uint32 mask, const char * filename)
{
  FILE * out = fopen(filename,"w");
  if (mask & ET_INTERPROC) interproc = TRUE;
  else interproc = FALSE;
  ASSERT(out,("Could not open %s for writing!",filename));
  FileIo(out, "digraph \"@G\" {\n\tnode [shape=ellipse]\n",BBL_OLD_ADDRESS(start));
  GraphDFTraversalWithCheckAndData((t_node *) start,node_writeout_postdominator, mask,ET_CALL|ET_SWI,out);
  if(interproc) FileIo(out, "\t\"%p\" [label=\"ALL BBLS\"]\n",ALL_BBLS);
  FileIo(out, "}\n");
  fclose(out);
}
#endif
/*}}}*/


void DominatorFree(t_bbl_list * list)
{
  t_bbl_list * free;

  while(list)
    {
      free = list;
      list = DomNext(list);
      Free(free);
    }
}

void BblMarkInitDominator(void)
{
  if (++dominator_node_marking_number==0x7fffffff)
    {
      FATAL(("Marking number overflow\n"));
    }
}

void DominatorDFSNumbering(t_cfg *cfg)
{
  t_bbl * ibbl, *prev_bbl = NULL;
  t_cfg_edge * iedge;
  t_uint32 dfsnumber = 0;

  void_stack dfsstack;

  dfsstack.top =0;

  Push(&dfsstack,CFG_UNIQUE_ENTRY_NODE(cfg));

  while(VSTACK_NOT_EMPTY(&dfsstack))
    {
      ibbl = T_BBL(Pop(&dfsstack));

      /*      VERBOSE(0,("bbl @eB\n",ibbl)); */

      if(BBL_DFS_NUMBER(ibbl)) continue;
      dfsnumber++;
      BBL_SET_DFS_NUMBER(ibbl,  dfsnumber);
      if(prev_bbl)
	{
	  BBL_SET_NEXT_IN_DFS(prev_bbl,  ibbl);
	  /*      VERBOSE(0,("Next of @B is\n   -> @B",prev_bbl,ibbl));*/
	}

      if (ibbl==CFG_SWI_HELL_NODE(cfg))
	Push(&dfsstack, CFG_EXIT_SWI_HELL_NODE(cfg));
	

      BBL_FOREACH_SUCC_EDGE(ibbl,iedge)
	{
	  /* In case of a call, start numbering inside called function if it is not done yet,
	   * else put tail of corresponding on the stack */
	  if(CfgEdgeIsForwardInterproc(iedge))
	    {
	      if (BBL_DFS_NUMBER(CFG_EDGE_TAIL(iedge)))
		{
		  /* Only start numbering at the function return if the exit block of the 
		   * called function is already numbered. Important for recursive functions! */
		  if(CFG_EDGE_CORR(iedge) && BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge))))
		    {
		      if(!BBL_DFS_NUMBER(CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge))))
			{
			  Push(&dfsstack,CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge)));
			}
		    }
		}
	      else
		{
		  Push(&dfsstack,CFG_EDGE_TAIL(iedge));
		}
	    }
	  else if(CFG_EDGE_CAT(iedge) == ET_RETURN || CFG_EDGE_CAT(iedge) == ET_COMPENSATING)
	    {
	      /* Only push the blocks whose corresponding block has been treated already */
	      if(BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge))))
		{
		  if((!BBL_DFS_NUMBER(CFG_EDGE_TAIL(iedge))))
		    {
		      Push(&dfsstack,CFG_EDGE_TAIL(iedge));
		    }
		}
	    }
	  else if (!BBL_DFS_NUMBER(CFG_EDGE_TAIL(iedge)))
	    {
	      Push(&dfsstack,CFG_EDGE_TAIL(iedge));
	    }
	}
      prev_bbl = ibbl;
    }

  CFG_FOREACH_BBL(cfg,ibbl)
    {
      t_uint32 max = 0;
      t_uint32 min = 0xffffffff;

      BBL_FOREACH_PRED_EDGE(ibbl,iedge)
	{
	  if (CfgEdgeIsBackwardInterproc(iedge))
	    {
	      if (BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)))>max)
		max = BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)));
	      if (BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)))<min)
		min = BBL_DFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)));
	    }
	  if (BBL_DFS_NUMBER(CFG_EDGE_HEAD(iedge))>max)
	    max = BBL_DFS_NUMBER(CFG_EDGE_HEAD(iedge));
	  if (BBL_DFS_NUMBER(CFG_EDGE_HEAD(iedge))<min)
	    min = BBL_DFS_NUMBER(CFG_EDGE_HEAD(iedge));
	}
      
      BBL_SET_LARGEST_PRED_DFS(ibbl,  max);
      BBL_SET_SMALLEST_PRED_DFS(ibbl,  min);
    }
}

#ifdef POST_DOMINATORS_IMPLEMENTED /* FIXME: Temporarily disabled because it currently is not adjusted to the new dominator calculation yet */
void DominatorPDFSNumbering(t_cfg *cfg)
{
  t_bbl * ibbl, *prev_bbl = NULL;
  t_cfg_edge * iedge;
  t_uint32 dfsnumber = 0;

  void_stack dfsstack;

  dfsstack.top =0;

  Push(&dfsstack,CFG_UNIQUE_EXIT_NODE(cfg));

  while(VSTACK_NOT_EMPTY(&dfsstack))
    {
      ibbl = T_BBL(Pop(&dfsstack));

/*            VERBOSE(0,("POP @B\n",ibbl)); */

      if(BBL_PDFS_NUMBER(ibbl)) continue;
      dfsnumber++;
      BBL_SET_PDFS_NUMBER(ibbl,  dfsnumber);
      VERBOSE(0,("pdfs %d: @B",dfsnumber,ibbl));
      if(prev_bbl)
	{
	  BBL_SET_NEXT_IN_PDFS(prev_bbl,  ibbl);
/*                VERBOSE(0,("Next of @B is\n   -> @B",prev_bbl,ibbl));*/
	}

      if (ibbl==CFG_EXIT_SWI_HELL_NODE(cfg))
      {
	Push(&dfsstack, CFG_SWI_HELL_NODE(cfg));
/*        VERBOSE(0,("XPushed @B",CFG_SWI_HELL_NODE(cfg)));*/
      }
	

      BBL_FOREACH_PRED_EDGE(ibbl,iedge)
	{
	  if(CFG_EDGE_CAT(iedge) == ET_RETURN || CFG_EDGE_CAT(iedge) == ET_COMPENSATING)
	    {
	      if (BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge)))
		{
		  if(BBL_PDFS_NUMBER(CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge))))
		    {
		      if(!BBL_PDFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge))))
			{
			  Push(&dfsstack,CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)));
/*                          VERBOSE(0,("1Pushed @B",CFG_EDGE_HEAD(EDGE_CORR(iedge))));*/
			}
		    }
		}
	      else
		{
		  Push(&dfsstack,CFG_EDGE_HEAD(iedge));
/*                  VERBOSE(0,("2Pushed @B",CFG_EDGE_HEAD(iedge)));*/
		}
	    }
	  else if(CFG_EDGE_IS_FORWARD_INTERPROC(iedge))
	    {
	      /* Only push the blocks whose corresponding block has been treated already */
	      /* or the blocks that have no corresponding edge */
	      if(CFG_EDGE_CORR(iedge) == NULL || BBL_PDFS_NUMBER(CFG_EDGE_TAIL(CFG_EDGE_CORR(iedge))))
		{
		  if((!BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge))))
		    {
		      Push(&dfsstack,CFG_EDGE_HEAD(iedge));
/*                      VERBOSE(0,("3Pushed @B",CFG_EDGE_HEAD(iedge)));*/
		    }
		}
	    }
	  else if (!BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge)))
	    {
	      Push(&dfsstack,CFG_EDGE_HEAD(iedge));
/*              VERBOSE(0,("4Pushed @B",CFG_EDGE_HEAD(iedge)));*/
	    }
	}
      prev_bbl = ibbl;
    }

  CFG_FOREACH_BBL(cfg,ibbl)
    {
      t_uint32 max = 0;
      t_uint32 min = 0xffffffff;

      BBL_FOREACH_SUCC_EDGE(ibbl,iedge)
	{
	  if (CfgEdgeIsForwardInterproc(iedge) && CFG_EDGE_CORR(iedge) != NULL)
	    {
	      if (BBL_PDFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)))>max)
		max = BBL_PDFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)));
	      if (BBL_PDFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)))<min)
		min = BBL_PDFS_NUMBER(CFG_EDGE_HEAD(CFG_EDGE_CORR(iedge)));
	    }
	  if (BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge))>max)
	    max = BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge));
	  if (BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge))<min)
	    min = BBL_PDFS_NUMBER(CFG_EDGE_HEAD(iedge));
	}
      
      BBL_SET_LARGEST_PRED_DFS(ibbl,  max);
      BBL_SET_SMALLEST_PRED_DFS(ibbl,  min);
    }
}
#endif

void MarkRemovedDominatorsUpWardsAndNumber(t_bbl * bbl, t_bbl * assign_bbl)
{
  t_bbl_list * iter_dom;
  
  static void_stack stacky;
  
  static void_stack *stack = &stacky;
  
  t_uint32 assign_number = BBL_DFS_NUMBER(assign_bbl);

  t_bbl *this_bbl;

  stacky.top=0;


  if (!BblIsMarkedDominator(bbl))
    Push(stack,bbl);
  else return;

  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);

      if(BblIsMarkedDominator(bbl)) continue;
    red_label:
    
      BblMarkDominator(bbl);

      if (BBL_NR_DOMINATED(bbl)>assign_number)
	{
#ifdef PRINT_COMPUTATION
	  printf("removed node %d from dominator set of %d\n",BBL_DFS_NUMBER(bbl),assign_number);
#endif
	  BBL_SET_NR_DOMINATED(bbl, assign_number);
	}

      iter_dom = BBL_DOMINATED_BY(bbl);
   
      bbl = NULL;
    
      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = iter_dom->next;
	  if (!BblIsMarkedDominator(this_bbl))
	    {
	      bbl = this_bbl;
	      break;
	    }
	}

      while(iter_dom)
	{

	  this_bbl = iter_dom->bbl;
	  iter_dom = iter_dom->next;
	  if (!BblIsMarkedDominator(this_bbl))
	    {
	      Push(stack,this_bbl);
	    }
	}

      if (bbl)
	goto red_label;
    }
}

static t_bool MarkDominatorsUpwardsUntilStop(t_cfg_edge * edge, t_bool init, t_uint32 stop_number, t_cfg_edge * link_edge)
{
  t_bbl * bbl = CFG_EDGE_HEAD(edge);
  t_bbl * bbl2;

  t_bbl * dom_bbl = CFG_EDGE_TAIL(edge);
  t_uint32 dom_bbl_nr = BBL_DFS_NUMBER(dom_bbl);

  t_bbl_list * iter_dom;
  
  static void_stack stacky = {0};

  static void_stack *stack = &stacky;

  static void_stack stackz = {0};

  static void_stack *stack2 = &stackz;

  void_stack *tmp_stack;

  t_bbl *this_bbl;

  if (init)
    {
      stacky.top = 0;
      stackz.top = 0;

      BblMarkInitDominator();

      if (BBL_MIN_DFS(bbl)<dom_bbl_nr)
	{
	  Push(stack,bbl);
	}

      if (link_edge)
	{
	  bbl2 = CFG_EDGE_HEAD(link_edge);
	  Push(stack,bbl2);
	}
    }
  else
    {
      int i;

      for (i=0;i<stack->top;i++)
	BblUnmarkDominator(stack->stack[i]);
    }


  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if(BblIsMarkedDominator(bbl)) continue;


    red_label:
    
      BblMarkDominator(bbl);

#ifdef PRINT_INTERSECTION
      VERBOSE(0,("Marked number %d",BBL_DFS_NUMBER(bbl),bbl));
#endif
      if (BBL_DFS_NUMBER(bbl) < stop_number) 
	{
	  Push(stack2,bbl);
	  continue;
	}

      iter_dom = BBL_DOMINATED_BY(bbl);
   
      bbl = NULL;
    
      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = iter_dom->next;
	  if (!BblIsMarkedDominator(this_bbl) && BBL_MIN_DFS(this_bbl)<dom_bbl_nr)
	    {
	      bbl = this_bbl;
	      break;
	    }
	}

      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = iter_dom->next;
	  if (!BblIsMarkedDominator(this_bbl) && BBL_MIN_DFS(this_bbl)<dom_bbl_nr)
	    {
	      Push(stack,this_bbl);
	    }
	}

      if (bbl)
	goto red_label;
    }

  tmp_stack = stack;
  stack = stack2;
  stack2 = tmp_stack;

  if (stack->top==1 && BBL_DFS_NUMBER(stack->stack[0])==1)
    return TRUE;

  return FALSE;


}

static void Removetransitive(t_bbl * ibbl)
{
  t_bbl_list * iter_dom = BBL_DOMINATED_BY(ibbl);

  iter_dom = BBL_DOMINATED_BY(ibbl);

  while (iter_dom)
    {
      t_bbl * dom = iter_dom->bbl;

      t_bbl_list * iter_dom2 = BBL_DOMINATED_BY(ibbl);
      t_bbl_list * prev_iter = NULL;
      t_bbl_list * next_iter;

      while (iter_dom2)
	{
	  t_bbl * dom2 = iter_dom2->bbl;
	  next_iter = iter_dom2->next;

	  if (dom!=dom2 && BblDominates(dom2,dom))
	    {
	      if (prev_iter) prev_iter->next=iter_dom2->next;
	      else BBL_SET_DOMINATED_BY(ibbl, iter_dom2->next);
	      Free(iter_dom2);
	      RemoveBblFromDominatesOf(ibbl,dom2);
	    }
	  else
	    {
	      prev_iter = iter_dom2;
	    }

	  iter_dom2 = next_iter;
	}
      iter_dom = iter_dom->next;
    }
}

static void RemovetransitiveNew(t_bbl * ibbl)
{
  t_uint32 min_dom = 0xffffffff;
  t_uint32 max_dom = 0x0;

  int teller = 0;

  int removed = 0;

  t_bbl_list * iter_dom = BBL_DOMINATED_BY(ibbl);
  t_bbl_list * prev_iter = NULL;
  t_bbl_list * next_iter;

  while (iter_dom)
    {
      t_bbl * dom = iter_dom->bbl;
      teller++;
      if (BBL_DFS_NUMBER(dom)<min_dom)
	min_dom = BBL_DFS_NUMBER(dom);
      if (BBL_DFS_NUMBER(dom)>max_dom)
	max_dom = BBL_DFS_NUMBER(dom);
      iter_dom = iter_dom->next;
    }

  iter_dom = BBL_DOMINATED_BY(ibbl);
  

  if (teller<=1) return;

  if (teller<3) 
    {
      
      Removetransitive(ibbl);
      return;
    }

  BblMarkInit2();

  iter_dom = BBL_DOMINATED_BY(ibbl);
  
  while (iter_dom)
    {
      t_bbl * dom = iter_dom->bbl;
      if (!BblIsMarked2(dom))
	MarkDominatorsUpwardsForTransitiveRemoval(dom, min_dom-1);
      iter_dom = iter_dom->next;
    }
  
  iter_dom = BBL_DOMINATED_BY(ibbl);

  while (iter_dom)
    {
      t_bbl * dom = iter_dom->bbl;
      next_iter = iter_dom->next;
      
      if (BblIsMarked2(dom))
	{
	  if (prev_iter) prev_iter->next=iter_dom->next;
	  else BBL_SET_DOMINATED_BY(ibbl, iter_dom->next);
	  Free(iter_dom);
	  RemoveBblFromDominatesOf(ibbl,dom);
	  removed++;
	}
      else
	{
	  prev_iter = iter_dom;
	}
      
      iter_dom = next_iter;
    }
}

static void RemovetransitiveDown(t_bbl * ibbl)
{
  int i,count;
  int removed = 0;

  t_set * dominated_blocks = BBL_DOMINATES_ARRAY(ibbl);
  
  BblMarkInit();
  
  for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
    {
      t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
      if (dominated == NULL) continue;
      if (!BblIsMarked(dominated))
	MarkDominatorsDownwardsForTransitiveRemoval(dominated);
      count++;
    }

  for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
    {
      t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
      if (dominated == NULL) continue;
      if (BblIsMarked(dominated))
	{
	  t_bbl_list * iter_dom,* prev_iter;
	  
	  dominated_blocks->data[i]=NULL;
	  removed++;
	  
	  iter_dom = BBL_DOMINATED_BY(dominated);
	  prev_iter = NULL;
	  
	  while (iter_dom)
	    {
	      t_bbl * dom = iter_dom->bbl;
	      
	      if (dom==ibbl)
		{
		  if (prev_iter) prev_iter->next=iter_dom->next;
		  else BBL_SET_DOMINATED_BY(dominated, iter_dom->next);
		  Free(iter_dom);
		  break;
		}
	      else
		{
		  prev_iter = iter_dom;
		}
	      iter_dom = iter_dom->next;
	    }
	}
      count++;
    }

  dominated_blocks->nr_elements -= removed;
}

static void MarkDominatorsUpwardsForSkippingUnnecessaryIntersections(t_bbl * bbl, t_uint32 stop_number)
{
  t_bbl_list * iter_dom;

  static void_stack stacky = {0};

  static void_stack *stack = &stacky;

  t_bbl *this_bbl;

  BblMark2(bbl);

  iter_dom = BBL_DOMINATED_BY(bbl);

  while(iter_dom)
    {
      if((BBL_DFS_NUMBER(iter_dom->bbl) >= stop_number) && !BblIsMarked2(iter_dom->bbl))
	Push(stack,iter_dom->bbl);
      iter_dom = DomNext(iter_dom);
    }
  
  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if (BblIsMarked2(bbl)) continue;
    red_label:
      
      BblMark2(bbl);

#ifdef PRINT_INTERSECTION
      VERBOSE(0,("UpMarked number %d",BBL_DFS_NUMBER(bbl)));
#endif
      iter_dom = BBL_DOMINATED_BY(bbl);
      
      bbl = NULL;
    
      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarked2(this_bbl) && BBL_DFS_NUMBER(this_bbl) >= stop_number)
	    {
	      bbl = this_bbl;
	      break;
	    }
	}

      while(iter_dom)
	{

	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarked2(this_bbl) && BBL_DFS_NUMBER(this_bbl) >= stop_number)
	    {
	      Push(stack,this_bbl);
	    }
	}

      if (bbl)
	goto red_label;
    }
}

static void MarkDominatorsUpwardsForTransitiveRemoval(t_bbl * bbl, t_uint32 stop_number)
{
  t_bbl_list * iter_dom;

  static void_stack stacky = {0};

  static void_stack *stack = &stacky;

  t_bbl *this_bbl;

  iter_dom = BBL_DOMINATED_BY(bbl);

  while(iter_dom)
    {
      if((BBL_DFS_NUMBER(iter_dom->bbl) >= stop_number) && !BblIsMarked2(iter_dom->bbl))
	Push(stack,iter_dom->bbl);
      iter_dom = DomNext(iter_dom);
    }
  
  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if (BblIsMarked2(bbl)) continue;
    red_label:
      
      BblMark2(bbl);

#ifdef PRINT_INTERSECTION
      VERBOSE(0,("UpMarked number %d",BBL_DFS_NUMBER(bbl)));
#endif
      iter_dom = BBL_DOMINATED_BY(bbl);
      
      bbl = NULL;
    
      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarked2(this_bbl) && BBL_DFS_NUMBER(this_bbl) >= stop_number)
	    {
	      bbl = this_bbl;
	      break;
	    }
	}

      while(iter_dom)
	{

	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarked2(this_bbl) && BBL_DFS_NUMBER(this_bbl) >= stop_number)
	    {
	      Push(stack,this_bbl);
	    }
	}

      if (bbl)
	goto red_label;
    }
}

static void MarkDominatorsUpwards(t_bbl * bbl)
{
  t_bbl_list * iter_dom;

  static void_stack stacky = {0};

  static void_stack *stack = &stacky;

  t_bbl *this_bbl;

  iter_dom = BBL_DOMINATED_BY(bbl);

  while(iter_dom)
    {
      if(!BblIsMarkedDominator(iter_dom->bbl))
	Push(stack,iter_dom->bbl);
      iter_dom = DomNext(iter_dom);
    }
  
  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if (BblIsMarkedDominator(bbl)) continue;
    red_label:
      
      BblMarkDominator(bbl);

#ifdef PRINT_INTERSECTION
      VERBOSE(0,("3 UpMarked number %d",BBL_DFS_NUMBER(bbl)));
#endif

      iter_dom = BBL_DOMINATED_BY(bbl);
      
      bbl = NULL;
    
      while(iter_dom)
	{
	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarkedDominator(this_bbl))
	    {
	      bbl = this_bbl;
	      break;
	    }
	}

      while(iter_dom)
	{

	  this_bbl = iter_dom->bbl;
	  iter_dom = DomNext(iter_dom);
	  if (!BblIsMarkedDominator(this_bbl))
	    {
	      Push(stack,this_bbl);
	    }
	}

      if (bbl)
	goto red_label;
    }
}


static void MarkDominatorsDownwardsForSkippingUnnecessaryIntersections(t_bbl * bbl, t_uint32 stop_number)
{

  static void_stack stacky = {0};

  static void_stack *stack = &stacky;

  t_bbl *this_bbl;
  int count , i;

  t_set * dominated_blocks = BBL_DOMINATES_ARRAY(bbl);
  
  for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
    {
      t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
      if (dominated == NULL) continue;
      if((BBL_DFS_NUMBER(dominated) <= stop_number) && !BblIsMarked(dominated) 
	 && BblIsMarked2(dominated)
	 )
	Push(stack,dominated);
      count++;
    }
    
  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if(BblIsMarked(bbl)) continue;

    red_label:
      
      BblMark(bbl);
      
#ifdef PRINT_INTERSECTION
      VERBOSE(0,("DownMarked number %d",BBL_DFS_NUMBER(bbl)));
#endif
      
      dominated_blocks = BBL_DOMINATES_ARRAY(bbl);
      
      bbl = NULL;

      for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
	{
	  t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
	  if (dominated == NULL) continue;
	  this_bbl = dominated;
	  if (!BblIsMarked(this_bbl) && 
	      BblIsMarked2(this_bbl)
	      )
	    {
	      bbl = this_bbl;
	      i++;
	      count++;
	      break;
	    }
	  count++;
	}

      
      for (;count<dominated_blocks->nr_elements;i++)
	{
	  t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
	  if (dominated == NULL) continue;
	  this_bbl = dominated;
	  
	  if (!BblIsMarked(this_bbl) && 
	      BblIsMarked2(this_bbl)
	      )
	    {
	      Push(stack,this_bbl);
	    }
	  count++;
	}
      
      if (bbl)
	goto red_label;
    }
}

static void MarkDominatorsDownwardsForTransitiveRemoval(t_bbl * bbl)
{
  static void_stack stacky = {0};
  
  static void_stack *stack = &stacky;
  
  t_bbl *this_bbl;
  int count , i;
  
  t_set * dominated_blocks = BBL_DOMINATES_ARRAY(bbl);
  
  for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
    {
      t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
      if (dominated == NULL) continue;
      if (!BblIsMarked(dominated))
	Push(stack,dominated);
      count++;
    }
  
  while(VSTACK_NOT_EMPTY(stack))
    {
      bbl = Pop(stack);
      
      if(BblIsMarked(bbl)) continue;
      
    red_label:
      
      BblMark(bbl);
      
#ifdef PRINT_INTERSECTION
      VERBOSE(0,("DownMarked number %d",BBL_DFS_NUMBER(bbl)));
#endif
      
      dominated_blocks = BBL_DOMINATES_ARRAY(bbl);
      
      bbl = NULL;
      
      for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
	{
	  t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
	  if (dominated == NULL) continue;
	  this_bbl = dominated;
	  if (!BblIsMarked(this_bbl))
	    {
	      bbl = this_bbl;
	      i++;
	      count++;
	      break;
	    }
	  count++;
	}
      
      for (;count<dominated_blocks->nr_elements;i++)
	{
	  t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
	  if (dominated == NULL) continue;
	  this_bbl = dominated;
	  
	  if (BblIsMarked(this_bbl))
	    Push(stack,this_bbl);
	  count++;
	}
      
      if (bbl)
	goto red_label;
    }
}




void BblSortAndCountPredAndSuccEdgesUsingDfs(t_bbl * bbl)
{
  t_bool change = TRUE;
  t_cfg_edge * pred;
  t_cfg_edge * prev_pred;
  int teller = 0;

  while (change)
    {
      teller = 0;
      change = FALSE;
      pred = BBL_PRED_FIRST(bbl);
      prev_pred = 0;

      while (pred)
	{
	  if (BBL_DFS_NUMBER(CFG_EDGE_HEAD(pred)))
	    teller++;
	  
	  /*	  if (prev_pred && BBL_DFS_NUMBER(CFG_EDGE_HEAD(pred)) < BBL_DFS_NUMBER(CFG_EDGE_HEAD(prev_pred)))
	    {
	      EDGE_SET_PRED_NEXT(prev_pred, EDGE_PRED_NEXT(pred));
	      EDGE_SET_PRED_PREV(pred, EDGE_PRED_PREV(prev_pred));
	      
	      if (prev_pred==BBL_PRED_FIRST(bbl))
		{
		  BBL_SET_PRED_FIRST(bbl, pred);
		}
	      else
		{
		  EDGE_SET_PRED_NEXT(EDGE_PRED_PREV(prev_pred), pred);

		}

	      if (pred==BBL_PRED_LAST(bbl))
		{
		  BBL_SET_PRED_LAST(bbl, prev_pred);
		}
	      else
		{
		  EDGE_SET_PRED_PREV(EDGE_PRED_NEXT(pred), prev_pred);
		}

	      EDGE_SET_PRED_PREV(prev_pred, pred);
	      EDGE_SET_PRED_NEXT(pred, prev_pred);

	      pred = prev_pred;

	      change = TRUE;
	    }
	  */

	  prev_pred = pred;
	  pred = CFG_EDGE_PRED_NEXT(pred);
	}
    }

  if (teller>1)
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) | BBL_HAS_MORE_THAN_ONE_INCOMING_DFS_EDGE);
  else
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) & (~BBL_HAS_MORE_THAN_ONE_INCOMING_DFS_EDGE));
  
  if (teller>10)
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) | HAS_A_LOT_OF_INCOMING_EDGES);
  else
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) & (~HAS_A_LOT_OF_INCOMING_EDGES));

  teller = 0;
  t_cfg_edge * succ;
  BBL_FOREACH_SUCC_EDGE(bbl,succ)
  {
    if (BBL_DFS_NUMBER(CFG_EDGE_TAIL(succ)))
      teller++;
  }

  if (teller>1)
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) | BBL_HAS_MORE_THAN_ONE_OUTGOING_DFS_EDGE);
  else
    BBL_SET_ATTRIB(bbl,   BBL_ATTRIB(bbl) & (~BBL_HAS_MORE_THAN_ONE_OUTGOING_DFS_EDGE));
}

#ifdef POST_DOMINATORS_IMPLEMENTED
void BblSortAndCountPredAndSuccEdgesUsingPDfs(t_bbl * bbl)
{
  t_bool change = TRUE;
  t_cfg_edge * pred;
  int teller = 0;

  while (change)
    {
      teller = 0;
      change = FALSE;
      pred = BBL_SUCC_FIRST(bbl);

      while (pred)
	{
	  if (BBL_PDFS_NUMBER(CFG_EDGE_TAIL(pred)))
	    teller++;

	  pred = CFG_EDGE_SUCC_NEXT(pred);
	}
    }

  if (teller>1)
    BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) | BBL_HAS_MORE_THAN_ONE_OUTGOING_PDFS_EDGE);
  else
    BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) & (~BBL_HAS_MORE_THAN_ONE_OUTGOING_PDFS_EDGE));
  
  if (teller>10)
    BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) | HAS_A_LOT_OF_OUTGOING_EDGES);
  else
    BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) & (~HAS_A_LOT_OF_OUTGOING_EDGES));

    {
      int teller = 0;

      BBL_FOREACH_PRED_EDGE(bbl,pred)
	{
	  if (BBL_DFS_NUMBER(CFG_EDGE_TAIL(pred)))
	    teller++;
	}

      if (teller>1)
	BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl)  | BBL_HAS_MORE_THAN_ONE_INCOMING_PDFS_EDGE);
      else
	BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl)  & (~BBL_HAS_MORE_THAN_ONE_INCOMING_PDFS_EDGE));
  
    }
}
#endif



t_bbl * DfsToBbl(t_cfg * cfg,t_uint32 nr)
{
  t_bbl * result;
  CFG_FOREACH_BBL(cfg,result)
    if (BBL_DFS_NUMBER(result)==nr)
      return result;
  return NULL;
}


static void ComputeCallsAlongPathsAndChangeEdgeOrder(t_cfg * cfg)
{
  t_function * fun;
  t_cfg_edge * edge;
  t_bbl * bbl;
  int nr;

  CFG_FOREACH_EDGE(cfg,edge)
    CFG_EDGE_SET_FOLLOWING_CALLS(edge, 0);
  
  CFG_FOREACH_FUN(cfg,fun)
    {
      t_bbl * exit_bbl = FunctionGetExitBlock(fun);
      t_bbl * entry_bbl = FUNCTION_BBL_FIRST(fun);
      
      FUNCTION_FOREACH_BBL_R(fun,bbl)
	{
	  if (bbl==exit_bbl) continue;
	  if (bbl==entry_bbl) continue;
	  
	  nr = 0x0;
	  
	  BBL_FOREACH_SUCC_EDGE(bbl,edge)
	    {
	      if (CFG_EDGE_CORR(edge) && CFG_EDGE_CAT(edge)==ET_CALL)
		nr += 1 + CFG_EDGE_FOLLOWING_CALLS(CFG_EDGE_CORR(edge));
	      else if (!CfgEdgeIsInterproc(edge) && AddressIsLt(BBL_OLD_ADDRESS(bbl),BBL_OLD_ADDRESS(CFG_EDGE_TAIL(edge))) && !AddressIsNull(BBL_OLD_ADDRESS(bbl)) )
		nr += CFG_EDGE_FOLLOWING_CALLS(edge);
	    }
	  
	  BBL_FOREACH_PRED_EDGE(bbl,edge)
	    {
	      if (nr != CFG_EDGE_FOLLOWING_CALLS(edge))
		{
		  CFG_EDGE_SET_FOLLOWING_CALLS(edge, nr);
		}
	    }
	}
    }

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      /*      int prev_value = 0x0; */
      int prev_value = 0xfffff;
      
      if (!BBL_SUCC_LAST(bbl)) continue;
      if (!BBL_SUCC_FIRST(bbl)) continue;

      if (BBL_SUCC_FIRST(bbl) == BBL_SUCC_LAST(bbl)) continue;
      if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)) != BBL_SUCC_LAST(bbl)) continue;

      if (CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl)) != ET_FALLTHROUGH && CFG_EDGE_CAT(BBL_SUCC_FIRST(bbl))!=ET_JUMP) continue;

      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	{
	  if (CFG_EDGE_CAT(edge) != ET_FALLTHROUGH && CFG_EDGE_CAT(edge)!=ET_JUMP) continue;

	  if (prev_value<CFG_EDGE_FOLLOWING_CALLS(edge))
	    {
	      /* switch edges */
	      t_cfg_edge * last = BBL_SUCC_FIRST(bbl);
	      t_cfg_edge * first = BBL_SUCC_LAST(bbl);
	      
	      CFG_EDGE_SET_SUCC_NEXT(last,  NULL);
	      CFG_EDGE_SET_SUCC_PREV(first,  NULL);
	      CFG_EDGE_SET_SUCC_NEXT(first,  last);
	      CFG_EDGE_SET_SUCC_PREV(last,  first);
	      BBL_SET_SUCC_LAST(bbl,  last);
	      BBL_SET_SUCC_FIRST(bbl,  first);
	      break;

	    }
	  prev_value = CFG_EDGE_FOLLOWING_CALLS(edge);
	}
    }

 VERBOSE(0,("end sorting"));
}

void CfgDetectPseudoLeaves(t_cfg * cfg)
{
  t_function * fun;
  t_bbl * bbl;
  t_cfg_edge * edge, * corr_edge;
  t_uint32 additional = 0;
  t_bbl * exit_block;
  int marked = 0, unmarked = 0;
  t_bool change = TRUE;
  
  CFG_FOREACH_FUN(cfg,fun)
    {
      FUNCTION_SET_FLAGS(fun,  FUNCTION_FLAGS(fun)  &~FF_IS_MARKED);
      FunctionUnmarkAllBbls(fun);
    }

  CFG_FOREACH_EDGE(cfg,edge)
    {
      if (CfgEdgeIsForwardInterproc(edge))
	FUNCTION_SET_FLAGS(BBL_FUNCTION(CFG_EDGE_HEAD(edge)), FUNCTION_FLAGS(BBL_FUNCTION(CFG_EDGE_HEAD(edge)))|FF_IS_MARKED);
    }


  CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) & (~IS_EXIT_OF_LEAF));
      BBL_SET_MIN_DFS(bbl, 0);
    }

  /* mark the exit bbls of leaf functions as IS_EXIT_OF_LEAF */ 
  CFG_FOREACH_FUN(cfg,fun)
    {
      if ((exit_block = FunctionGetExitBlock(fun)))
	{
	  if (FUNCTION_FLAGS(fun) & FF_IS_MARKED)
	    {
	      marked++;
	      BBL_SET_ATTRIB(exit_block, BBL_ATTRIB(exit_block) &~IS_EXIT_OF_LEAF);
	    }
	  else
	    {
	      unmarked++;
	      BBL_SET_ATTRIB(exit_block,   BBL_ATTRIB(exit_block) |IS_EXIT_OF_LEAF);
	      BBL_SET_MIN_DFS(exit_block,  BBL_DFS_NUMBER(FUNCTION_BBL_FIRST(fun)));
	    }
	}
    }

  /* mark the exit bbls as IS_EXIT_OF_LEAF, if their function has at
   * least one path from their entry to their exit block that 
   *   a) does not perform any calls, or
   *   b) only calls leaf functions (or functions complying with these points
   *     a or b) that are dominated by the current function   */
  while (change)
    {
      change = FALSE;

      BblMarkInit();
      
      CFG_FOREACH_FUN(cfg,fun)
	{
	  t_bbl * exit_block = FunctionGetExitBlock(fun);
	  if (!exit_block) continue;
	  if (BBL_IS_HELL(FUNCTION_BBL_FIRST(fun))) continue;
	  if (BBL_ATTRIB(exit_block) & IS_EXIT_OF_LEAF) continue;
	  
	  FunctionMarkBbl(fun,FUNCTION_BBL_FIRST(fun));
	  BblMark(FUNCTION_BBL_FIRST(fun));
	  
	  while (FunctionUnmarkBbl(fun,&bbl))
	    {
	      if (BblIsMarked(bbl))
		{
		  BBL_FOREACH_SUCC_EDGE(bbl,edge)
		    {
		      if (!CfgEdgeIsInterproc(edge))
			{		      
			  if (!BblIsMarked(CFG_EDGE_TAIL(edge)))
			    {
			      FunctionMarkBbl(fun,CFG_EDGE_TAIL(edge));
			      BblMark(CFG_EDGE_TAIL(edge));
			    }
			}
		      else if (
			       (CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge)==ET_IPJUMP) 
			       && (corr_edge = CFG_EDGE_CORR(edge)) 
			       && (BBL_ATTRIB(CFG_EDGE_HEAD(corr_edge)) & IS_EXIT_OF_LEAF) 
			       && BBL_MIN_DFS(CFG_EDGE_HEAD(corr_edge)) > BBL_DFS_NUMBER(FUNCTION_BBL_FIRST(fun))
			       )
			{
			  if (!BblIsMarked(CFG_EDGE_TAIL(corr_edge)))
			    {
			      FunctionMarkBbl(fun,CFG_EDGE_TAIL(corr_edge));
			      BblMark(CFG_EDGE_TAIL(corr_edge));
			    }			  
			}
		    }
		}   
	    }
	  
	  if (BblIsMarked(exit_block))
	    {
	      unmarked++;
	      marked--;
	      change = TRUE;
	      BBL_SET_ATTRIB(exit_block,   BBL_ATTRIB(exit_block)  |IS_EXIT_OF_LEAF);
	      BBL_SET_MIN_DFS(exit_block,  BBL_DFS_NUMBER(FUNCTION_BBL_FIRST(fun)));
	      FUNCTION_SET_MIN_DFS(fun,  BBL_DFS_NUMBER(FUNCTION_BBL_FIRST(fun)));
	    }
	}
    }

  /*  printf("%d non-leaf functions, %d leaf functions\n", marked, unmarked);*/
  
  change = TRUE;
  
  while(change)
    {
      t_function * cg_node;
      t_cg_edge * cg_edge;
      
      change = FALSE;
      
      CG_FOREACH_NODE(CFG_CG(cfg),cg_node)
	{
	  t_function * fun = CG_NODE_FUN(cg_node);
	  t_uint32 min_callees = BBL_DFS_NUMBER(FUNCTION_BBL_FIRST(fun));
	  
	  if (FUNCTION_MIN_DFS(fun)) continue;
	  if (!FunctionGetExitBlock(fun)) continue;

	  FUNCTION_FOREACH_SUCC_EDGE(cg_node,cg_edge)
	    min_callees = MINIMUM(min_callees, FUNCTION_MIN_DFS(CG_NODE_FUN(CG_EDGE_TAIL(cg_edge))));

	  if (min_callees>0)
	    {
	      additional++;
	      FUNCTION_SET_MIN_DFS(fun,  min_callees);
	      BBL_SET_MIN_DFS(FunctionGetExitBlock(fun),  min_callees);
	      BBL_SET_ATTRIB(FunctionGetExitBlock(fun),  BBL_ATTRIB(FunctionGetExitBlock(fun)) |IS_EXIT_OF_LEAF);
	      
	      /*	      VERBOSE(0,("changed @B to %d\n",FUNCTION_BBL_FIRST(fun),min_callees));*/
	      
	      change = TRUE;
	    }
	}
    }
  
 
  VERBOSE(0, ("%d non-leaf functions, %d leaf functions, %d additional", marked, unmarked, additional));
  
  return;
}

#ifdef ENABLE_STATISTICS
#define MAX_IN 64
static void GetStatistics(t_cfg *cfg)
{
  t_function *fun;
  t_bbl *bbl;
  t_cfg_edge *iedge;
  t_uint32 nbbls=0;
  t_uint32 nedges=0;
  t_uint32 in_degree[MAX_IN]={0};
  t_uint32 max_in = 0;
  t_uint32 nr_dominators;
  t_uint32 i;
  t_bbl_list *iter_dom;
  t_uint32 average_max_depth = 0;
  t_uint32 average_min_depth = 0;
  t_uint32 average_nr_preds = 0;
  t_uint32 nr_bbls = 0;
  t_uint32 array[10000] = {0};


  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      nbbls++;
      
      BBL_FOREACH_SUCC_EDGE(bbl,iedge)
      {
	nedges++;
      }

      nr_dominators=0;
      iter_dom=BBL_DOMINATED_BY(bbl);

      while(iter_dom)
      {
	nr_dominators++;
	iter_dom = DomNext(iter_dom);
      }
      
      if(nr_dominators<MAX_IN)
      {
	in_degree[nr_dominators] += 1;
	if(nr_dominators > max_in) max_in = nr_dominators;
      }
      else
	FATAL(("In degree > %d",MAX_IN));
#if 0
      if(nr_dominators>9) 
      {
	VERBOSE(0,("Indegree %d for @B",nr_dominators,bbl));
	iter_dom=BBL_DOMINATED_BY(bbl);
	while(iter_dom)
	{
	  VERBOSE(0,("@B",iter_dom->bbl));
	  iter_dom = DomNext(iter_dom);
	}
      }
#endif    
    }

  printf("In-degree, count\n");
  for(i=1;i<=max_in;i++) 
  {
    printf("%d, %d\n",i,in_degree[i]);
  }

  CFG_FOREACH_FUN(cfg,fun)
    FUNCTION_FOREACH_BBL(fun,bbl)
    {
      T_NODE(bbl)->marked_number = 0xfffffff;
      T_NODE(bbl)->marked_number2 = 0x0;
      BBL_DOM_MARKING_NUMBER(bbl) = 0x0;
    }
  
  bbl = CFG_UNIQUE_ENTRY_NODE(cfg);

  T_NODE(bbl)->marked_number = 0x0;
  T_NODE(bbl)->marked_number2 = 0x0;
  BBL_DOM_MARKING_NUMBER(bbl) = 0x0;

 
  while (bbl)
    {
      t_set * dominated_blocks = BBL_DOMINATES_ARRAY(bbl);
      
      int i,count;
      for (i=0,count=0;count<dominated_blocks->nr_elements;i++)
	{
	  t_bbl * dominated = T_BBL(dominated_blocks->data[i]);
	  if (dominated == NULL) continue;
	  T_NODE(dominated)->marked_number   = MINIMUM(T_NODE(dominated)->marked_number ,T_NODE(bbl)->marked_number +1);
	  T_NODE(dominated)->marked_number2  = MAXIMUM(T_NODE(dominated)->marked_number2,T_NODE(bbl)->marked_number2+1);
	  BBL_DOM_MARKING_NUMBER(dominated)     += BBL_DOM_MARKING_NUMBER(bbl)+1;
	  count++;
	}

      nr_bbls++;

      average_min_depth += T_NODE(bbl)->marked_number;
      average_max_depth += T_NODE(bbl)->marked_number2;
      average_nr_preds  += BBL_DOM_MARKING_NUMBER(bbl);

      array[dominated_blocks->nr_elements]++;

      bbl = BBL_NEXT_IN_DFS(bbl);
    }

  printf("depth info: min = %.2f, max = %.2f nr_pred %.2f\n",((float)average_min_depth)/nr_bbls,((float)average_max_depth)/nr_bbls,((float)average_nr_preds)/nr_bbls);

  /*
    for (i=0;i<10000;i++)
    if (array[i]) 
    printf("%d: %d, ",i,array[i]);

    printf("\n");
  */


}
#endif

void DominatorCleanup(t_cfg * cfg)
{
  t_bbl * ibbl;
  t_function * ifun;

  if (dominator_info_correct)
    return;

  CFG_FOREACH_FUN(cfg, ifun)
    {
      FUNCTION_FOREACH_BBL(ifun,ibbl)
	{
	  DominatorFree(BBL_DOMINATED_BY(ibbl));
	  BBL_SET_DOMINATED_BY(ibbl, NULL);
/*          DominatorFree(BBL_P_DOMINATED_BY(ibbl));*/
/*          BBL_SET_P_DOMINATED_BY(ibbl, NULL);*/
	}
    }
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
