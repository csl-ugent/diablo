/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h>
#include <diabloflowgraph.h>

#define EDGE_IP_STYLE "bold"
#define EDGE_IP_HEAD  "normal"
#define EDGE_STYLE    "solid"
#define EDGE_HEAD     "vee"

#define EDGE_CALLFT_STYLE "dotted"
#define EDGE_CALLFT_COLOR "green"
#define EDGE_CALLFT_HEAD  "onormal"

/* {{{ constructor */
t_function * FunctionMake(t_bbl * entrypoint, t_const_string name,  t_function_type ft) 
{
  t_cfg * cfg=BBL_CFG(entrypoint);
  t_architecture_description * desc = CFG_DESCRIPTION(cfg);
  t_function * ret = FunctionNew(cfg);

  FUNCTION_SET_NAME(ret, name?StringDup(name):NULL);

  FUNCTION_SET_MARKED_BBLS( ret,  NULL);
  FUNCTION_SET_MARKED_EDGES(ret,  NULL);
  FUNCTION_SET_NEXT_MARKED(ret,  NULL);

  /* initialize the entry for this function */
  FUNCTION_SET_BBL_FIRST(ret,  entrypoint);

  /* Insert the entrypoint into the function (must be done first) */
  BblInsertInFunction(entrypoint,ret);

  /* Allocate and link return block */
  FUNCTION_SET_BBL_LAST(ret,  BblNew(cfg));
  BblInsertInFunction(FUNCTION_BBL_LAST(ret),ret);
  BBL_SET_ATTRIB(FUNCTION_BBL_LAST(ret), BBL_ATTRIB(FUNCTION_BBL_LAST(ret))| BBL_IS_EXITBLOCK);

  /* for conservativeness: assume a function changes _all_ registers: this can
   * be refined with an analysis later on */
  FUNCTION_SET_REGS_CHANGED (ret,  desc->all_registers);
  FUNCTION_SET_REGS_SAVED (ret,  RegsetNew());
  FUNCTION_SET_REGS_THROUGH (ret,  RegsetNew());
  FUNCTION_SET_REGS_USED (ret,  desc->all_registers);

  return ret;
}
/* }}} */

/* {{{ destructor */
void FunctionKill(t_function * fun)
{
  t_cfg *fg =FUNCTION_CFG(fun);
#if 0
  while(FUNCTION_LOOPS(fun))
  {
    t_loopref * delete_me = FUNCTION_LOOPS(fun);
    FUNCTION_SET_LOOPS(fun,  FUNCTION_LOOPS(fun)->next);
    Free(delete_me);
  }
#endif
  /* remove from call graph */
  if (NODE_NEXT(T_NODE(fun)) || NODE_PREV(T_NODE(fun)))
  {
    while (NODE_SUCC_FIRST(T_NODE(fun)))
      GraphRemoveEdge(T_GRAPH(CFG_CG(fg)),NODE_SUCC_FIRST(T_NODE(fun)));
    while (NODE_PRED_FIRST(T_NODE(fun)))
      GraphRemoveEdge(T_GRAPH(CFG_CG(fg)),NODE_PRED_FIRST(T_NODE(fun)));
    GraphUnlinkNode(T_GRAPH(CFG_CG(fg)),T_NODE(fun));
  }

  FunctionFree(fun);
}
/* }}} */

void FunctionInsertInCfg (t_cfg* cfg, t_function* fun)
{
  if (CFG_FUNCTION_FIRST(cfg)==NULL)
  {
    FUNCTION_SET_FPREV(fun, NULL);
    CFG_SET_FUNCTION_FIRST(cfg, fun);
    CFG_SET_FUNCTION_LAST(cfg, fun);
  }
  else
  {
    FUNCTION_SET_FPREV(fun, CFG_FUNCTION_LAST(cfg));
    FUNCTION_SET_FNEXT(CFG_FUNCTION_LAST(cfg), fun);
    CFG_SET_FUNCTION_LAST(cfg, fun);
  }
}

/* Unlink the function from the CFG but keep all its edges and BBLs intact */
void FunctionUnlinkFromCfg(t_function* fun)
{
  t_cfg* cfg = FUNCTION_CFG(fun);

  /* Remove from list of functions */
  if (FUNCTION_FPREV(fun))
  {
    FUNCTION_SET_FNEXT(FUNCTION_FPREV(fun), FUNCTION_FNEXT(fun));
  }
  else
  {
    CFG_SET_FUNCTION_FIRST(cfg, FUNCTION_FNEXT(fun));
  }
  if (FUNCTION_FNEXT(fun))
  {
    FUNCTION_SET_FPREV(FUNCTION_FNEXT(fun), FUNCTION_FPREV(fun));
  }
  else
  {
    CFG_SET_FUNCTION_LAST(cfg, FUNCTION_FPREV(fun));
  }

  FUNCTION_SET_CFG(fun, NULL);
  FUNCTION_SET_FPREV(fun, NULL);
  FUNCTION_SET_FNEXT(fun, NULL);
}

/* {{{ Create functions for cfg */

static void FunctionSetConventionFlags(t_function * f, t_bbl * bbl, t_symbol * sym)
{
  FUNCTION_SET_FLAGS(f,  FUNCTION_FLAGS(f) & (~FF_IS_EXPORTED));
  if (sym && (SYMBOL_ORDER(sym)>0))
    {
      FUNCTION_SET_FLAGS (f,  FUNCTION_FLAGS(f) | FF_IS_EXPORTED);
    }
  /* if the function entry is hand written in assembler and all callers
   * are from hand written assembler code, there is no guarantee that the
   * function will indeed obey the calling conventions, even if it is exported */
  if (BBL_ATTRIB(bbl) & BBL_IS_HANDWRITTEN_ASSEMBLY)
    {
      t_cfg_edge * edge;
      t_bool only_from_assembler_code = TRUE;
      BBL_FOREACH_PRED_EDGE(bbl,edge)
        {
          if (CFG_EDGE_CAT(edge) != ET_CALL || BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
            continue;
          if (!(BBL_ATTRIB(CFG_EDGE_HEAD(edge)) & BBL_IS_HANDWRITTEN_ASSEMBLY))
            {
              only_from_assembler_code = FALSE;
              break;
            }
        }
      if (only_from_assembler_code)
        {
          FUNCTION_SET_FLAGS(f,  FUNCTION_FLAGS(f) & (~FF_IS_EXPORTED));
        }
    }
}

static t_bool helper_cfg_find_and_create_function(t_node * current, t_node * parent,void * data) {
  t_function * f;
  t_bbl * bbl=T_BBL(current);
  t_uint32 * nfuncs=(t_uint32 *) data;
  t_cfg_edge * edge;

  if (BBL_IS_HELL(bbl)) return FALSE;
  if (BBL_ATTRIB(bbl) & BBL_IS_FUNCTION_ENTRY) return FALSE;
  
  BBL_FOREACH_PRED_EDGE(bbl,edge)
  {
    /* find the type of the edge that points to this node */
    if (CFG_EDGE_CAT(edge) == ET_CALL)
    {
      (*nfuncs)++;

      BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) | BBL_IS_FUNCTION_ENTRY); 
      break;
    }
  }

  if (BBL_ATTRIB(bbl) & BBL_IS_FUNCTION_ENTRY) 
  {
    t_symbol * sym = NULL;
    t_symbol_ref * sr = NULL;
    
    for (sr=BBL_REFED_BY_SYM(bbl); sr!=NULL; sr=sr->next)
    {
        if (strcmp("$compiler",SYMBOL_NAME(sr->sym)) == 0) continue;
        if (SYMBOL_FLAGS(sr->sym) & SYMBOL_TYPE_MARK_CODE) continue;
        if (strcmp("$handwritten",SYMBOL_NAME(sr->sym)) == 0) continue;
        if (strstr(SYMBOL_NAME(sr->sym),"DIABLO_ARM_GOT32")) continue;
        if (strstr(SYMBOL_NAME(sr->sym),"LPIC")) continue;
	if ((!sym) || (SYMBOL_ORDER(sr->sym)>SYMBOL_ORDER(sym)) || (SYMBOL_FLAGS(sr->sym) & SYMBOL_TYPE_FUNCTION))
	{
	  sym = sr->sym;
	}
    }

    f = FunctionMake(bbl, sym?SYMBOL_NAME(sym):NULL, FT_NORMAL);
    FunctionSetConventionFlags(f,bbl,sym);
  }
  return FALSE;
}

/* The bool indicates whether or not bbls should start new functions even if
   no calls reach them but a function symbol indicates they start a function.

   The standard (old) behavior corresponds to FALSE, but it can be useful
   to enable this in case fine control over some transformations is needed
   that relies on separate functions not being combined accidentally (such
   as when a tail call (by means of a jump) is the only entry path into some
   callee). */

t_uint32 CfgCreateFunctions(t_cfg * fg, t_bool preserve_functions_by_symbol)
{
  t_function * f;
  t_uint32 nfunctions=0;
  t_cfg_edge * i_edge;
  t_bbl * start = CFG_UNIQUE_ENTRY_NODE(fg);
  t_bbl * bbl;
  /* Do function leader detection, and create all function structures */
  GraphDFTraversalWithCheckAndData(T_NODE(start), helper_cfg_find_and_create_function, 0xffff, ET_CALL|ET_SWI, &nfunctions);


  /* Necessary for forced reachable functions in programs that do not jump to
   * hell */
  BBL_FOREACH_SUCC_EDGE(CFG_HELL_NODE(fg),i_edge)
  {
    if (!(BBL_ATTRIB(CFG_EDGE_TAIL(i_edge)) & BBL_IS_FUNCTION_ENTRY))
      GraphDFTraversalWithCheckAndData(T_NODE(CFG_EDGE_TAIL(i_edge)), helper_cfg_find_and_create_function, 0xffff, ET_CALL|ET_SWI, &nfunctions);
  }

  /* We used to combine functions that did a tail call (jump edge) with their callees if 
     those callees only one caller. For correctness and compaction, that was fine.
     For more controlled application of transformations (per function), this is not acceptable
     however. Instead, one function per (reachable) function symbol should be created */
 
  if (preserve_functions_by_symbol)
  {
    CFG_FOREACH_BBL(fg,bbl)
    {
      t_symbol * sym = NULL;
      t_symbol_ref * sr = NULL;

      if ((BBL_ATTRIB(bbl) & BBL_IS_FUNCTION_ENTRY)) continue;

      for (sr=BBL_REFED_BY_SYM(bbl); sr!=NULL; sr=sr->next)
        {
          sym=sr->sym;
          if (!sym) continue;
          if (!(SYMBOL_FLAGS(sym) & (SYMBOL_TYPE_FUNCTION | SYMBOL_TYPE_FUNCTION_SMALLCODE))) continue;
          t_string sn = SYMBOL_NAME(sym);
          if (!sn) continue;

          /* we need to skip symbol names that alias more reasonable, non-diablo-generated symbol names */
          if (sn[0]=='$' || sn[0]=='.') continue; 
          if (strchr(sn,':')) continue; 
          BBL_SET_ATTRIB(bbl,  BBL_ATTRIB(bbl) | BBL_IS_FUNCTION_ENTRY); 
          f = FunctionMake(bbl, sn, FT_NORMAL);
          FunctionSetConventionFlags(f,bbl,sym);
          nfunctions++;
          break; // any block should be added at most once, even if it has multiple func symbols attached to it ...
        }
    }
  }

  return nfunctions;
}
/* }}} */

/* {{{ move return edges to return block */
typedef struct _node_list {
  t_bbl * node;
  struct _node_list * next;
} nodelist;

static nodelist * returnblocks = NULL;

static t_bool function_helper_find_returnblocks(t_node * current, t_node * parent,void * data) {
  /* if one of the successor edges of this node is a return edge */
  /* then add this node to the list */
  nodelist *p;
  t_cfg_edge * edge;

  BBL_FOREACH_SUCC_EDGE(T_BBL(current),edge) 
  {
    if (CFG_EDGE_CAT(edge) == ET_RETURN) 
    {
      /* bingo! */
      p = Malloc(sizeof(nodelist));
      p->node = T_BBL(current);
      p->next = returnblocks;
      returnblocks = p;
      return FALSE;
    }
  }
  return FALSE;
}

t_uint32 FunctionMoveReturnEdgesToReturnBlock(t_function * f)
{
  t_uint32 ret=0;
  nodelist *temp;
  t_cfg * cfg = FUNCTION_CFG(f);

  if (f==CFG_WRAP_FUNCTION(FUNCTION_CFG(f)) || f==CFG_HELL_FUNCTION(FUNCTION_CFG(f))) return 0;

  GraphDFTraversalWithCheckAndData((t_node *) FUNCTION_BBL_FIRST(f), function_helper_find_returnblocks, (t_uint16) ~(ET_INTERPROC), ET_CALL|ET_SWI,NULL);

  /* okay, all the return blocks are in returnblocks */
  /* the dummy return block was already allocated in a previous phase, so we can now link it to the return blocks */

  temp = returnblocks;
  while (temp != NULL) 
  {
    t_cfg_edge * edge, * tmp;
    t_bbl *currnode = temp->node;


    BBL_FOREACH_SUCC_EDGE_SAFE(currnode,edge,tmp)
    {
      if ((CFG_EDGE_TAIL(edge) == CFG_EXIT_HELL_NODE(FUNCTION_CFG(f))) && (CFG_EDGE_CAT(edge) == ET_RETURN) && (CFG_EDGE_CORR(edge) == NULL))
      {
        ret++;
        CfgEdgeKill(edge);
        edge=CfgEdgeNew(cfg, currnode, FUNCTION_BBL_LAST(f), ET_JUMP);	
        CFG_EDGE_SET_CFG(edge, cfg);
        CFG_EDGE_SET_REFCOUNT(edge, 1);
        /* this shouldn't be ET_FALLTHROUGH, because in the case
           of a conditional return there already is a
           fallthrough edge! */
      }
      else if (CFG_EDGE_CAT(edge) == ET_RETURN) 
      {
	t_cfg_edge * retu;
	if (!BBL_IS_HELL(currnode)) FATAL(("How did we get here? @ieB @E",currnode,edge));

        retu=
          /* put the return node between the node and its successor */
           CfgEdgeNew(cfg, FUNCTION_BBL_LAST(f), CFG_EDGE_TAIL(edge), ET_RETURN);
        /* this shouldn't be ET_FALLTHROUGH, because in the case
           of a conditional return there already is a
           fallthrough edge! */
        /* If there is a corresponding edge, patch it! */
        if (CFG_EDGE_CORR(edge)) 
          CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), retu);
        else 
          FATAL(("EDGE Without corresponding!"));
        CfgEdgeKill(edge);
        edge=CfgEdgeNew(cfg, currnode, FUNCTION_BBL_LAST(f), ET_JUMP);	
        CFG_EDGE_SET_CFG(edge, cfg);
        CFG_EDGE_SET_REFCOUNT(edge, 1);
        CFG_EDGE_SET_CFG(retu, cfg);
        CFG_EDGE_SET_REFCOUNT(retu, 1);
        ret++;
        break; /* since there can normally be only one return edge from a node, we can end the for loop here */
      }
      else
      {
        /* With the exception of hell, the only other edge that should come out of a return node is a fallthrough edge(in the case of a conditional return) */
        if ((currnode!=CFG_HELL_NODE(FUNCTION_CFG(f)))&&(CFG_EDGE_CAT(edge)!=ET_FALLTHROUGH)&&(CFG_EDGE_CAT(edge)!=ET_IPFALLTHRU))
        {
          VERBOSE(0,( "entry @B current @ieB %p\n",FUNCTION_BBL_FIRST(f),currnode,currnode));
          FATAL(("found so called return block and outgoing edge is of type %d\n",CFG_EDGE_CAT(edge)));
        }
      }
    }
    temp = temp->next;
  }

  /* clean up returnblocks so that it's ready for the next usage */
  while (returnblocks != NULL)
  {
    temp = returnblocks;
    returnblocks = returnblocks->next;
    Free(temp);
  }

  return ret;
}
/* }}} */

/* {{{ associate blocks to function */
/*! 
 * \param current  a node 
 * \param parent the parent node of the current node
 * \param a function casted to void 
 * \return 1 if we do not need to follow successors of this node, else 0
 * 
 * This (helper) function inserts basic blocks inside a function, is called by a DFS traverse 
 */ 

static t_bool function_helper_associate_blocks(t_node * current, t_node * parent,void * data) 
{
  t_bbl * bbl = (t_bbl *) current;
  t_function * function=(t_function *) data;

  /* Do not assign Hellnodes to a function, and skip all Hell-successors */
  if (BBL_IS_HELL(bbl)) 
  {
    return TRUE;
  }
  /* entry point should already be inserted, but we want to do all successors (so return 0) */
  if (bbl==FUNCTION_BBL_FIRST(function)) 
  {
    return FALSE;
  }
  /* Do not reassign already assigned blocks (and skip all successors of this node) */
  if (BBL_FUNCTION(bbl) != NULL)
  {
    return TRUE; 
  }
  /* If this basicblock is the start of a function, skip it and skip all successors */
  if (BBL_ATTRIB(bbl) & BBL_IS_FUNCTION_ENTRY)
  {
    return TRUE;
  }

  /* otherwise, insert this basic block in the function */

  BblInsertInFunction(bbl,function);

  /* Continue with the successors */
  return FALSE;
}

void FunctionAssociateBbls(t_function * f)
{
  if (FUNCTION_BBL_FIRST(f)) 
    BBL_SET_ATTRIB(FUNCTION_BBL_FIRST(f),   BBL_ATTRIB(FUNCTION_BBL_FIRST(f))  &(~BBL_IS_FUNCTION_ENTRY));
  GraphDFTraversalWithCheckAndData(
      (t_node *) FUNCTION_BBL_FIRST(f), 
      function_helper_associate_blocks, 
      (t_uint16) ~(ET_INTERPROC),
      ET_CALL|ET_SWI,
      (void *) f 
      );
  if (FUNCTION_BBL_FIRST(f)) 
    BBL_SET_ATTRIB(FUNCTION_BBL_FIRST(f),    BBL_ATTRIB(FUNCTION_BBL_FIRST(f))  | BBL_IS_FUNCTION_ENTRY);
}
/* }}} */

/* {{{ Make the return edges of a function point to the right blocks */
t_uint32 FunctionAdjustReturnEdges(t_function * f) 
{
  t_uint32 ret=0;
  t_cfg * flowgraph=FUNCTION_CFG(f);
  t_cfg_edge * edge;
  t_cfg_edge * tmp;
  t_bool hell_to_call_hell = FALSE;

  BBL_FOREACH_PRED_EDGE_SAFE(FUNCTION_BBL_FIRST(f),edge,tmp) 
  {
    if (CFG_EDGE_CAT(edge) != ET_CALL) continue;
    if (CFG_EDGE_HEAD(edge) == CFG_UNIQUE_ENTRY_NODE(flowgraph)) continue;/* TODO: check if this is correct */
    if (f == CFG_CALL_HELL_FUNCTION(flowgraph))
    {
      /* Only one existing call to call hell is allowed at this point, ie the one from hell */
      if(hell_to_call_hell)
        FATAL(("Implement calls to call hell!"));
      if(CFG_EDGE_HEAD(edge) == CFG_HELL_NODE(flowgraph))
	hell_to_call_hell = TRUE;
      else
        FATAL(("Implement calls to call hell!"));
    }
    else if (f == CFG_HELL_FUNCTION(flowgraph))
    {
      if (BBL_CALL_HELL_TYPE(CFG_EDGE_HEAD(edge)))
      	{
	  continue;
	}
      else
	  /* move call from hell -> call_hell and return return from hell -> return_hell 
	   * BUT ONLY IF the call comes from compiler generated code. Otherwise, we can't be
	   * certain that the called function will obey the calling conventions */
	if (!(BBL_ATTRIB(CFG_EDGE_HEAD(edge)) & BBL_IS_HANDWRITTEN_ASSEMBLY))
	{
	  t_cfg_edge * call=CfgEdgeNew(flowgraph, CFG_EDGE_HEAD(edge), CFG_CALL_HELL_NODE(flowgraph) ,ET_CALL);
	  t_cfg_edge * retu=CfgEdgeNew(flowgraph, CFG_EXIT_CALL_HELL_NODE(flowgraph), CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)) ,ET_RETURN);
	  CfgEdgeKill(CFG_EDGE_CORR(edge));
	  CfgEdgeKill(edge);
	  CFG_EDGE_SET_CORR(call, retu);
	  CFG_EDGE_SET_CORR(retu, call);
	  CFG_EDGE_SET_CFG(call, flowgraph);
	  CFG_EDGE_SET_CFG(retu, flowgraph);
	  CFG_EDGE_SET_REFCOUNT(call,1);
	  CFG_EDGE_SET_REFCOUNT(retu,1);
	  ret++;
	}
    }
    else if (CFG_EDGE_HEAD(edge) == CFG_HELL_NODE(flowgraph)) /* Calls from hell */
    {
      t_uint32 i;

      /* call from hell remains unchanged, but return is edge from called function to hell ! */

      t_cfg_edge *retu=CfgEdgeNew(flowgraph,FUNCTION_BBL_LAST(BBL_FUNCTION(CFG_EDGE_TAIL(edge))),CFG_EXIT_HELL_NODE(flowgraph) ,ET_RETURN);

      /* sanity checks */
      ASSERT(CFG_EDGE_CORR(edge),("No corresponding edge for call @E!",edge));
      ASSERT(CFG_EDGE_CFG(CFG_EDGE_CORR(edge)),("No cfg!"));
      ASSERT(CFG_EDGE_REFCOUNT(edge) == CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(edge)),("Refcount of call @E and corresponding return @E do not match\n",edge,CFG_EDGE_CORR(edge)));
      CFG_EDGE_SET_CORR(retu, edge);
      CFG_EDGE_SET_CFG(retu,flowgraph);
      /* the new edge should have the same refcount as the old edge */
      CFG_EDGE_SET_REFCOUNT(retu, CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(edge)));
      ret++;

      /* the old edge should be removed completely, so we must kill it refcount times */
      {
        t_uint32 rc = CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(edge));
        for (i=0; i<rc; i++)
          CfgEdgeKill(CFG_EDGE_CORR(edge));
      }
      CFG_EDGE_SET_CORR(edge, retu);
    }
    else /* normal edges */
    {
      t_cfg_edge * retu=NULL;

      /* sanity checks */
      ASSERT(CFG_EDGE_CORR(edge),("No corresponding edge!\n"));
      ASSERT(CFG_EDGE_REFCOUNT(edge) == 1,("Implement normal calls %x with refcount > 1",edge));
      ASSERT(CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(edge)) == 1,("Implement correspondings of normal calls with refcount > 1"));

      /* get the returnedge from hell, and move it! */
      retu= 
	CfgEdgeNew(flowgraph, FUNCTION_BBL_LAST(BBL_FUNCTION(CFG_EDGE_TAIL(edge))), CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)), ET_RETURN);

      if (CFG_EDGE_CORR(edge) == tmp)	/* this can happen if we encounter a call to the next instruction */
	tmp = CFG_EDGE_PRED_NEXT(tmp);

      CfgEdgeKill(CFG_EDGE_CORR(edge));
      CFG_EDGE_SET_CORR(edge, retu);
      CFG_EDGE_SET_CORR(retu, edge);
      CFG_EDGE_SET_CFG(retu, flowgraph);
      CFG_EDGE_SET_REFCOUNT(retu, 1);
      ret++;
    }
  }
  return ret;
}
/* }}} */

/* {{{ returns the dummy exit block of a function, or NULL if this block has been freed */
t_bbl * FunctionGetExitBlock(t_function * fun)
{
  t_bbl * ret = FUNCTION_BBL_LAST(fun);
  if (BblIsExitBlock(ret))
    return ret;

  return NULL;
}
/* }}} */

/* {{{ Look up the function (by name) in a list of unbehaved functions, return TRUE if found */
t_bool FunctionBehaves(t_function * fun)
{
  
  char * name = fun ? FUNCTION_NAME(fun) : NULL;
  char ** badfuns = fun ? CFG_DESCRIPTION(FUNCTION_CFG(fun))->unbehaved_funs : NULL;
  t_uint32 i = 0;

  if(!fun) return TRUE;
  if(!name) return TRUE;
  if(!badfuns) return TRUE;
  while(strcmp(badfuns[i],""))
  {
    if(!strcmp(name,badfuns[i++])) return FALSE;
  }
  if (!FUNCTION_BEHAVES(fun)) return FALSE;
  return TRUE;
}
/* }}} */
    
t_bool FunctionIsCalledByHell(t_function * fun)
{
  t_bbl * bbl = FUNCTION_BBL_FIRST(fun);
  t_cfg_edge * edge;
  BBL_FOREACH_PRED_EDGE(bbl,edge)
    if (CFG_EDGE_CAT(edge) == ET_CALL)
    {
      if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
	return TRUE;
    }
  return FALSE;
}

/**************************************************************/
/*  function cfg export to .dot                               */
/**************************************************************/

#define ClearBblData(x)		do { \
  if ((x)->fillcolor) Free((x)->fillcolor); \
  if ((x)->label) Free((x)->label); \
  if ((x)->style) Free((x)->style); \
  if ((x)->extra) Free((x)->extra); \
  memset((x), 0, sizeof(t_bbl_draw_data)); \
} while (0)

#define ClearEdgeData(x)	do { \
  if ((x)->color) Free((x)->color); \
  if ((x)->label) Free((x)->label); \
  if ((x)->style) Free((x)->style); \
  if ((x)->extra) Free((x)->extra); \
  memset((x), 0, sizeof(t_edge_draw_data)); \
} while (0)

static t_string GetHellNodeLabel(t_bbl *bbl)
{
  t_cfg *cfg = BBL_CFG(bbl);

  if (bbl == CFG_HELL_NODE(cfg))
    return StringDup("HELL");
  if (bbl == CFG_CALL_HELL_NODE(cfg))
    return StringDup("CALL HELL");
  if (BBL_CALL_HELL_TYPE(bbl))
    return StringConcat2("DYNAMIC CALL: ", FUNCTION_NAME(BBL_FUNCTION(bbl))+16);
  if (bbl == CFG_SWI_HELL_NODE(cfg))
    return StringDup("SWI HELL");
  if (bbl == CFG_LONGJMP_HELL_NODE(cfg))
    return StringDup("LONGJMP HELL");
  if (bbl == CFG_EXIT_HELL_NODE(cfg))
    return StringDup("EXIT HELL");
  if (bbl == CFG_EXIT_CALL_HELL_NODE(cfg))
    return StringDup("EXIT CALL HELL");
  if (FUNCTION_CALL_HELL_TYPE(BBL_FUNCTION(bbl)) &&
      bbl == FUNCTION_BBL_LAST(BBL_FUNCTION(bbl)))
    return StringConcat2("EXIT DYNAMIC CALL: ", FUNCTION_NAME(BBL_FUNCTION(bbl))+16);
  if (bbl == CFG_EXIT_SWI_HELL_NODE(cfg))
    return StringDup("EXIT SWI HELL");
  if (bbl == CFG_EXIT_LONGJMP_HELL_NODE(cfg))
    return StringDup("EXIT LONGJMP HELL");
  if (bbl == CFG_UNIQUE_ENTRY_NODE(cfg))
    return StringDup("UNIQUE ENTRY");
  if (bbl == CFG_UNIQUE_EXIT_NODE(cfg))
    return StringDup("UNIQUE EXIT");

  FATAL(("Unknown hell node @B",bbl));
}

void FunctionDrawGraphAnnotated(
    t_function *fun, 
    t_string filename, 
    void (*bbl_annotator)(t_bbl *, t_bbl_draw_data *), 
    void (*edge_annotator)(t_cfg_edge *, t_edge_draw_data *))
{
  t_ins * ins;
  t_bbl * bbl;
  t_cfg_edge * edge;
  t_bool show_callers = TRUE;
  t_bool draw_link;
  int callers = 0;

  t_bbl_draw_data bbl_data;
  t_edge_draw_data edge_data;

  FILE * out = fopen(filename,"w");
  ASSERT(out,("Could not open %s for writing!",filename));

  memset(&bbl_data, 0, sizeof(t_bbl_draw_data));
  memset(&edge_data, 0, sizeof(t_edge_draw_data));

  /* only show callers if there are less than MAX_SHOW_CALLERS */
#define MAX_SHOW_CALLERS	10
  BBL_FOREACH_PRED_EDGE(FUNCTION_BBL_FIRST(fun),edge)
    if (CFG_EDGE_CAT(edge) == ET_CALL)
      callers++;
  show_callers = (callers < MAX_SHOW_CALLERS);

  /* graph header */
  if (FUNCTION_NAME(fun) && strlen(FUNCTION_NAME(fun)))
    FileIo(out, "digraph \"%s\" {\n\tnode [shape=box];\n",FUNCTION_NAME(fun));
  else
    FileIo(out, "digraph \"noname_@G\" {\n\tnode [shape=box]\n",BBL_CADDRESS(FUNCTION_BBL_FIRST(fun)));

  /* graph body */

  /* {{{ function nodes */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    ClearBblData(&bbl_data);

    if (BBL_IS_HELL(bbl))
      bbl_data.label = GetHellNodeLabel(bbl);
    else if (bbl == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)))
      bbl_data.label = StringDup("Entry");
    else if (bbl == FunctionGetExitBlock(fun))
      bbl_data.label = StringDup("RETURN");
    else
    {
      /* regular block */
      t_string final, tmp1, tmp2;
      final = StringIo("@xB",bbl);
      BBL_FOREACH_INS(bbl,ins)
      {
	tmp1 = final;
	tmp2 = StringIo("@gI\\l",ins);
	final = StringConcat2(tmp1,tmp2);
	Free(tmp1);
	Free(tmp2);
      }

      bbl_data.label = final;
      bbl_data.style = StringDup("filled");
      bbl_data.fillcolor = StringDup("white");
    }

    bbl_annotator(bbl, &bbl_data);

    fprintf(out,"\t\"%p\" [",bbl);
    fprintf(out, "label=\"%s\"", bbl_data.label);
    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);
    fprintf(out,"]\n");
  } /* }}} */

  ClearBblData(&bbl_data);

  /* {{{ successor edges */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      t_bbl * tail = CFG_EDGE_TAIL(edge);

      ClearEdgeData(&edge_data);
      ClearBblData(&bbl_data);
      draw_link = FALSE;

      if (!CfgEdgeTestCategoryOr(edge,ET_INTERPROC))
      {
	/* {{{ intraprocedural edges */
	switch (CFG_EDGE_CAT(edge))
	{
	  case ET_FALLTHROUGH:
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("green");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	    break;
	  case ET_UNKNOWN:
	    edge_data.style = StringDup(EDGE_STYLE);
	    edge_data.color = StringDup("purple");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	    break;
          case ET_SWITCH:
            edge_data.style = StringDup(EDGE_STYLE);
            edge_data.color = StringDup("orange");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
            break;
	  case ET_JUMP:
	    edge_data.style = StringDup(EDGE_STYLE);
	    edge_data.color = StringDup("black");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	    break;
	  default:
            FATAL(("Implement me @E", edge));
	    edge_data.style = StringDup("dashed");
	    edge_data.color = StringDup("blue");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	}

	if (CFG_EDGE_CAT(edge) == ET_SWITCH)
	  edge_data.label = StringIo("%d", CFG_EDGE_SWITCHVALUE(edge));
	/*	else
		edge_data.label = StringIo("O(%1.2f)", ORDER(CFG_EDGE_EXEC_COUNT(edge)));*/

	edge_annotator(edge, &edge_data);

	fprintf(out, "\t\"%p\" -> \"%p\" [", bbl, tail);
	fprintf(out, "style=%s, color=%s", edge_data.style, edge_data.color);
	if (edge_data.label) fprintf(out, ", label=\"%s\"", edge_data.label);
	if (edge_data.extra) fprintf(out, ", %s", edge_data.extra);
	fprintf(out, "]\n");
	/* }}} */
      }
      else
      {
	/* {{{ interprocedural edges */
	char nodename[20];
        t_string clean_name = NULL;
	t_string destfun = BBL_FUNCTION(tail)?(FUNCTION_NAME(BBL_FUNCTION(tail)) ? FUNCTION_NAME(BBL_FUNCTION(tail)) : "noname"):"nofun";

	/* don't print successors of the hell or call hell nodes (there's no way you can get them all to fit nicely in a drawing) */
	if (BBL_IS_HELL(bbl)) continue;
        
        clean_name = StringDup(destfun);

        if (strlen(clean_name)>50)
          sprintf(clean_name+40,"TRUNCATED");

	switch (CFG_EDGE_CAT(edge))
	{
	  case ET_CALL:
	    edge_data.style = StringDup(EDGE_STYLE);
	    edge_data.color = StringDup("red");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	    bbl_data.fillcolor = StringDup("yellow");
	    if (BBL_FUNCTION(tail) == fun)
	    {
	      /* treat recursive calls differently */
	      bbl_data.label = NULL;
	      sprintf(nodename, "%p", tail);
	    }
	    else
	    {
	      sprintf(nodename, "f%p", tail);
	      if (AddressIsEq(BBL_OLD_ADDRESS(tail),BBL_CADDRESS(tail)))
		bbl_data.label = StringIo("%s (@G)",clean_name,BBL_OLD_ADDRESS(tail));
	      else
		bbl_data.label = StringIo("%s (old @G new @G)",clean_name,BBL_OLD_ADDRESS(tail),BBL_CADDRESS(tail));
	      bbl_data.style = StringDup("filled");
	    }
	    if (CFG_EDGE_CORR(edge))
	      draw_link = TRUE; 
	    break;
	  case ET_RETURN:
	    edge_data.style = StringDup(EDGE_STYLE);
	    edge_data.color = StringDup("blue");
            edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	    bbl_data.fillcolor = StringDup("green");
	    sprintf(nodename, "r%p", tail);
	    bbl_data.label = StringIo("@xB in %s",tail,clean_name);
	    break;
	  case ET_COMPENSATING:
	    edge_data.style = StringDup(EDGE_IP_STYLE);
	    edge_data.color = StringDup("blue");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	    bbl_data.fillcolor = StringDup("green");
	    sprintf(nodename, "c%p", tail);
	    bbl_data.label = StringIo("return of %s",clean_name);
	    break;
	  case ET_SWI:
	    edge_data.style = StringDup(EDGE_IP_STYLE);
	    edge_data.color = StringDup("red");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	    bbl_data.fillcolor = StringDup("red");
	    sprintf(nodename,"i%p",tail); /* tail will be the hell node */
	    bbl_data.label = StringDup("HELL");
	    break;
	  case ET_IPUNKNOWN:
	    edge_data.style = StringDup(EDGE_IP_STYLE);
	    edge_data.color = StringDup("purple");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	    bbl_data.fillcolor = StringDup("magenta");
	    bbl_data.style = StringDup("filled");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@xB in %s",tail,clean_name);
	    break;
	  case ET_IPFALLTHRU:
            edge_data.style = StringDup("dashed");
            edge_data.color = StringDup("green");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
            bbl_data.fillcolor = StringDup("black");
            sprintf(nodename, "i%p", tail);
            bbl_data.label = StringIo("@xB in %s",tail,clean_name);
            break;
	  case ET_IPJUMP:
            edge_data.style = StringDup(EDGE_IP_STYLE);
            edge_data.color = StringDup("black");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
            bbl_data.fillcolor = StringDup("black");
            sprintf(nodename, "i%p", tail);
            bbl_data.label = StringIo("@xB in %s",tail,clean_name);
            break;
	  case ET_IPSWITCH:
	    edge_data.style = StringDup(EDGE_IP_STYLE);
	    edge_data.color = StringDup("orange");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	    bbl_data.fillcolor = StringDup("black");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@xB in %s",tail,clean_name);
	    break;
	  default:
	    /* make it stand out as something we haven't seen yet */
            FATAL(("Implement me @E", edge));
	    edge_data.style = StringDup(EDGE_IP_STYLE);
	    edge_data.color = StringDup("magenta");
            edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	    bbl_data.fillcolor = StringDup("blue");
	    sprintf(nodename, "i%p", tail);
	    bbl_data.label = StringIo("@xB in %s",tail,clean_name);
	    bbl_data.style = StringDup("filled");
	    break;
	}

	if (CFG_EDGE_CAT(edge) == ET_IPSWITCH)
	  edge_data.label = StringIo("%d", CFG_EDGE_SWITCHVALUE(edge));

	/* catch hell nodes */
	if (BBL_IS_HELL(tail))
	{
	  sprintf(nodename,"h%p",tail);
	  Free(bbl_data.label);
	  bbl_data.label = GetHellNodeLabel(tail);
	  if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	  bbl_data.fillcolor = StringDup("red");
	  if (bbl_data.style) Free(bbl_data.style);
	  bbl_data.style = StringDup("filled");
	}

	if (show_callers || CFG_EDGE_CAT(edge) != ET_RETURN)
	{
	  /* print edge */
	  edge_annotator(edge, &edge_data);
	  fprintf(out,"\t\"%p\" -> \"%s\" [style=%s,color=%s",bbl,nodename,edge_data.style,edge_data.color);
	  if (edge_data.label) fprintf(out,",label=\"%s\"",edge_data.label);
	  if (edge_data.extra) fprintf(out,", %s",edge_data.extra);
	  fprintf(out,"]\n");

	  /* print extraprocedural block */
	  if (bbl_data.label)
	  {
	    fprintf(out,"\t\"%s\" [", nodename);
	    fprintf(out, "label=\"%s\"", bbl_data.label);
	    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
	    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
	    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);
	    fprintf(out,"]\n");
	  }

	  if (draw_link)
	  {
	    /* draw an invisible link edge, to improve the layout of function call/return edge pairs */
            fprintf(out,"\t\"%p\" -> \"%p\" [style=%s,color=%s,arrowhead=%s]\n",bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)), EDGE_CALLFT_STYLE, EDGE_CALLFT_COLOR, EDGE_CALLFT_HEAD);
	  }
	}

        Free(clean_name);
	/* }}} */
      }
    }
  } /* }}} */

  /* {{{ predecessor edges */
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      t_bbl * head = CFG_EDGE_HEAD(edge);
      t_string destfun = BBL_FUNCTION(head)?(FUNCTION_NAME(BBL_FUNCTION(head)) ? FUNCTION_NAME(BBL_FUNCTION(head)) : "noname"):"nofun";
      char nodename[20];
      t_string clean_name = NULL;
      
      ClearEdgeData(&edge_data);
      ClearBblData(&bbl_data);

      if (!CfgEdgeTestCategoryOr(edge,ET_INTERPROC)) continue; /* all intraprocedural edges have already been handled as predecessors */
      if (BBL_IS_HELL(bbl)) continue;

      clean_name = StringDup(destfun);

      if (strlen(clean_name)>50)
        sprintf(clean_name+40,"TRUNCATED");

      switch (CFG_EDGE_CAT(edge))
      {
	case ET_CALL:
	  edge_data.style = StringDup(EDGE_STYLE);
	  edge_data.color = StringDup("red");
          edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	  bbl_data.fillcolor = StringDup("yellow");
	  sprintf(nodename,"F%p",head);
	  bbl_data.label = StringIo("@xB in %s",head,clean_name);
	  break;
	case ET_IPUNKNOWN:
	  edge_data.style = StringDup(EDGE_IP_STYLE);
	  edge_data.color = StringDup("purple");
          edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	  bbl_data.fillcolor = StringDup("blue");
	  sprintf(nodename,"I%p",head);
	  bbl_data.label = StringIo("@xB in %s",head,clean_name);
	  break;
	case ET_IPFALLTHRU:
          edge_data.style = StringDup("dashed");
          edge_data.color = StringDup("green");
          edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
          bbl_data.fillcolor = StringDup("cyan");
          bbl_data.label = StringIo("@xB in %s",head,clean_name);
          sprintf(nodename,"I%p",head);
          break;
	case ET_IPJUMP:
          edge_data.style = StringDup(EDGE_IP_STYLE);
          edge_data.color = StringDup("black");
          edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
          bbl_data.fillcolor = StringDup("cyan");
          bbl_data.label = StringIo("@xB in %s",head,clean_name);
          sprintf(nodename,"I%p",head);
          break;
	case ET_IPSWITCH:
	  edge_data.style = StringDup(EDGE_IP_STYLE);
	  edge_data.color = StringDup("orange");
          edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	  bbl_data.fillcolor = StringDup("cyan");
	  bbl_data.label = StringIo("@xB in %s",head,clean_name);
	  sprintf(nodename,"I%p",head);
	  break;
	case ET_COMPENSATING:
	  edge_data.style = StringDup(EDGE_IP_STYLE);
          edge_data.color = StringDup("blue");
          edge_data.extra = StringDup("arrowhead=" EDGE_IP_HEAD);
	  bbl_data.fillcolor = StringDup("black");
	  sprintf(nodename,"C%p",head);
	  bbl_data.label = StringIo("return from %s",clean_name);
	  break;
	case ET_RETURN:
	  edge_data.style = StringDup(EDGE_STYLE);
	  edge_data.color = StringDup("blue");
          edge_data.extra = StringDup("arrowhead=" EDGE_HEAD);
	  bbl_data.label = NULL;
	  if (BBL_FUNCTION(head) == fun)
	    sprintf(nodename,"%p", head);
	  else if (BBL_IS_HELL(FUNCTION_BBL_FIRST(BBL_FUNCTION(head))))
	    sprintf(nodename,"h%p", FUNCTION_BBL_FIRST(BBL_FUNCTION(head)));
	  else
	    sprintf(nodename,"f%p", FUNCTION_BBL_FIRST(BBL_FUNCTION(head)));
	  break;
	default:
	  FATAL(("unexpected edge type"));
      }

      if (CFG_EDGE_CAT(edge) == ET_IPSWITCH)
        edge_data.label = StringIo("%d", CFG_EDGE_SWITCHVALUE(edge));

      /* catch hell node and call hell node */
      if (BBL_IS_HELL(head) && CfgEdgeIsForwardInterproc(edge))
      {
	sprintf(nodename,"h%p",head);
	if (bbl_data.label) Free(bbl_data.label);
	bbl_data.label = GetHellNodeLabel(head);
	if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	bbl_data.fillcolor = StringDup("red");
	if (bbl_data.style) Free(bbl_data.style);
	bbl_data.style = StringDup("filled");
      }
      /* catch the entry node */
      if (head == CFG_UNIQUE_ENTRY_NODE(BBL_CFG(bbl)))
      {
	sprintf(nodename,"e%p",head);
	if (bbl_data.label) Free(bbl_data.label);
	bbl_data.label = StringDup("Entry");
	if (bbl_data.fillcolor) Free(bbl_data.fillcolor);
	bbl_data.fillcolor = StringDup("blue");
	if (bbl_data.style) Free(bbl_data.style);
	bbl_data.style = StringDup("filled");
      }

      if (show_callers || CFG_EDGE_CAT(edge) != ET_CALL)
      {
	  /* print edge */
	  edge_annotator(edge, &edge_data);
	  fprintf(out,"\t\"%s\" -> \"%p\" [style=%s,color=%s",nodename,bbl,edge_data.style,edge_data.color);
	  if (edge_data.label) fprintf(out,",label=\"%s\"",edge_data.label);
	  if (edge_data.extra) fprintf(out,", %s",edge_data.extra);
	  fprintf(out,"]\n");

	  /* print extraprocedural block */
	  if (bbl_data.label)
	  {
	    fprintf(out,"\t\"%s\" [", nodename);
	    fprintf(out, "label=\"%s\"", bbl_data.label);
	    if (bbl_data.fillcolor) fprintf(out,", fillcolor=%s", bbl_data.fillcolor);
	    if (bbl_data.style) fprintf(out,", style=%s", bbl_data.style);
	    if (bbl_data.extra) fprintf(out,", %s", bbl_data.extra);
	    fprintf(out,"]\n");
	  }
      }
      
      Free (clean_name);

    }
  } /* }}} */
  
  /* graph footer */
  FileIo(out, "}\n");
  fclose(out);

  /* cleanup */
  ClearBblData(&bbl_data);
  ClearEdgeData(&edge_data);
}

static t_bool print_before_ = TRUE;
static t_bool print_after_ = TRUE;

/* This function will set the options to be used by liveness_bbl_annotator */
void LivenessAnnotatorSetOpt(t_bool print_before, t_bool print_after)
{
  print_before_ = print_before;
  print_after_ = print_after;
}

static void liveness_bbl_annotator(t_bbl *bbl, t_bbl_draw_data *data)
{
  /* Get the parts and combine them */
  t_const_string before = print_before_ ? StringIo("Before: @X\\l", CFG_DESCRIPTION(BBL_CFG(bbl)), BblRegsLiveBefore(bbl)) : NULL;
  t_const_string after = print_after_ ? StringIo("After: @X\\l", CFG_DESCRIPTION(BBL_CFG(bbl)), BblRegsLiveAfter(bbl)) : NULL;
  t_const_string old_label = data->label;
  data->label = StringConcat3(before, old_label, after);

  /* Free everything */
  Free(old_label);
  if(print_before_)
    Free(before);
  if(print_after_)
    Free(after);
}

static void hotness_bbl_annotator(t_bbl *bbl, t_bbl_draw_data *data)
{
  if (!diabloflowgraph_options.blockprofilefile)
    return; 
  if (data->style) Free(data->style);
  if (data->fillcolor) Free(data->fillcolor);
  data->style = StringDup("filled");

  if (BblIsHot(bbl))
    data->fillcolor = StringDup("PeachPuff2");
  else if (BBL_EXEC_COUNT(bbl) > 0)
    data->fillcolor = StringDup("lightyellow");
  else
    data->fillcolor = StringDup("white");
}
static void hotness_edge_annotator(t_cfg_edge *edge, t_edge_draw_data *data)
{
  t_string tmp = NULL;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  if (!EdgeIsHot(edge))
    return;

  if (!strcmp(data->style,"solid"))
    tmp = "bold";
  else if (!strcmp(data->style,"dashed"))
    tmp = "dotted";
  if (tmp)
  {
    Free(data->style);
    data->style = StringDup(tmp);
  }

  if (data->extra) Free(data->extra);
  data->extra = StringIo("weight=%d",CFG_EDGE_EXEC_COUNT(edge));

  if (data->label)
  {
    t_string tmp2 = data->label;
    tmp = StringIo(", %d",CFG_EDGE_EXEC_COUNT(edge));
    data->label = StringConcat2(tmp2,tmp);
    Free(tmp); Free(tmp2);
  }
  else
    data->label = StringIo("%d",CFG_EDGE_EXEC_COUNT(edge));
}

static void null_bbl_annotator(t_bbl *bbl, t_bbl_draw_data *data)
{
}

static void null_edge_annotator(t_cfg_edge *edge, t_edge_draw_data *data)
{
}

void FunctionDrawGraph(t_function *fun, t_string filename)
{
  FunctionDrawGraphAnnotated(fun, filename, null_bbl_annotator, null_edge_annotator);
}

void FunctionDrawGraphWithHotness(t_function *fun, t_string filename)
{
  FunctionDrawGraphAnnotated(fun, filename, hotness_bbl_annotator, hotness_edge_annotator);
}

void FunctionDrawGraphWithLiveness(t_function * fun, t_string filename)
{
  FunctionDrawGraphAnnotated(fun, filename, liveness_bbl_annotator, null_edge_annotator);
}

t_uint32 FunctionGetHeat(t_function * fun)
{
  t_bbl * bbl;
  t_uint32 hotness = 0;
  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    hotness += BBL_NINS(bbl) * BBL_EXEC_COUNT(bbl);
  }
  return hotness;
}


/* {{{ FunctionDuplicate */
#define setcopy(orig,new)	BBL_SET_TMP (orig, new)
#define getcopy(orig)		T_BBL(BBL_TMP(orig))
t_function * FunctionDuplicate (t_function *fun)
{
  t_cfg *cfg = FUNCTION_CFG (fun);
  t_function *new;
  t_bbl *bbl, *copy;
  t_string name = StringConcat2 (FUNCTION_NAME (fun), "__DUP__");

  copy = BblDup (FUNCTION_BBL_FIRST (fun));
  new = FunctionMake (copy, name, FT_NORMAL);
  setcopy (FUNCTION_BBL_FIRST (fun), copy);
  Free (name);

  /* step 1: duplicate all basic blocks */
  FUNCTION_FOREACH_BBL (fun, bbl)
  {
    if (bbl == FunctionGetExitBlock (fun))
      setcopy(bbl, FunctionGetExitBlock (new));
    else if (bbl != FUNCTION_BBL_FIRST (fun))
    {
      copy = BblDup (bbl);
      BblInsertInFunction (copy, new);
      setcopy (bbl, copy);
    }
  }

  /* step 2: duplicate all edges */
  FUNCTION_FOREACH_BBL (fun, bbl)
  {
    t_cfg_edge *edge;
    BBL_FOREACH_SUCC_EDGE (bbl, edge)
    {
      if (bbl == FunctionGetExitBlock (fun)) continue;

      if (!CfgEdgeIsInterproc (edge))
      {
	t_cfg_edge *e = CfgEdgeCreate (cfg,
	    getcopy (bbl), getcopy (CFG_EDGE_TAIL (edge)), CFG_EDGE_CAT (edge));
	if (CFG_EDGE_CAT (edge) == ET_SWITCH)
	  CFG_EDGE_SET_SWITCHVALUE (e, CFG_EDGE_SWITCHVALUE (edge));
      }
      else if (CFG_EDGE_CAT (edge) == ET_CALL)
      {
	t_bbl *returnsite = NULL;
	t_cfg_edge *e;
	if (CFG_EDGE_CORR (edge))
	  returnsite = CFG_EDGE_TAIL (CFG_EDGE_CORR (edge));
	if (returnsite && BBL_FUNCTION (returnsite) != fun)
	  FATAL (("implement interproc call/return pairs"));
	if (CFG_EDGE_TAIL (edge) == FUNCTION_BBL_FIRST (fun))
	{
	  /* recursive call */
	  e = CfgEdgeCreateCall (cfg, getcopy (bbl), getcopy (CFG_EDGE_TAIL (edge)),
	      returnsite ? getcopy (returnsite) : NULL,
	      returnsite ? getcopy (CFG_EDGE_HEAD (CFG_EDGE_CORR (edge))) : NULL);
	}
	else
	{
	  e = CfgEdgeCreateCall (cfg, getcopy (bbl), CFG_EDGE_TAIL (edge),
	      returnsite ? getcopy (returnsite) : NULL,
	      returnsite ? CFG_EDGE_HEAD (CFG_EDGE_CORR (edge)) : NULL);
	}

	if (!returnsite)
	{
	  CfgEdgeKill (CFG_EDGE_CORR (e));
	  CFG_EDGE_SET_CORR (e, NULL);
	}
      }
      else if (CFG_EDGE_CAT (edge) == ET_IPJUMP)
      {
	t_cfg_edge *e = 
	CfgEdgeCreate (cfg, getcopy (bbl), CFG_EDGE_TAIL (edge), CFG_EDGE_CAT (edge));
	if (CFG_EDGE_CORR (edge))
	  CfgEdgeCreateCompensating (cfg, e);
      }
      else if (CFG_EDGE_CAT (edge) == ET_IPFALLTHRU)
      {
	/* break fallthrough path: the original function already has one */
	t_cfg_edge *e;
	t_ins *ins = InsNewForBbl (getcopy (bbl));
	InsAppendToBbl (ins, getcopy (bbl));
	CFG_DESCRIPTION (cfg)->InsMakeDirectBranch (ins);

	e = CfgEdgeCreate (cfg, getcopy (bbl), CFG_EDGE_TAIL (edge), ET_IPJUMP);
	if (CFG_EDGE_CORR (edge))
	  CfgEdgeCreateCompensating (cfg, e);
      }
      else if (CFG_EDGE_CAT (edge) == ET_SWI)
      {
	if (CFG_EDGE_CORR (edge))
	{
	  ASSERT (
	      BBL_FUNCTION (CFG_EDGE_TAIL (CFG_EDGE_CORR (edge))) == BBL_FUNCTION (bbl),
	      ("implement interprocedural swi return"));
	  CfgEdgeCreateSwi (cfg, getcopy (bbl),
                            getcopy (CFG_EDGE_TAIL (CFG_EDGE_CORR (edge))));
	}
	else
	{
	  t_cfg_edge *e = CfgEdgeCreateSwi (cfg, getcopy (bbl), NULL);
	  CfgEdgeKill (CFG_EDGE_CORR (e));
	  CFG_EDGE_SET_CORR (e, NULL);
	}
      }
      else
	FATAL (("implement @E", edge));
    }
  }

  /* step 3: if the original function no longer has an exit block,
   * kill that of the duplicated function as well */
  if (!FunctionGetExitBlock (fun))
    BblKill (FunctionGetExitBlock (new));

  DiabloBrokerCall ("FunctionDuplicateAdditionalDataBlocks", fun, new);
  
  return new;
}
#undef getcopy
#undef setcopy
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
