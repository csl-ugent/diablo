/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopt.h>

CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(bbl_fingerprint);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(bbl_factor);
CFG_DYNAMIC_MEMBER_GLOBAL_ARRAY(bbl_can_be_factored);

static int counter = 0;
//#define DEBUGCOUNTER if(counter++ < diablosupport_options.debugcounter)
#define DEBUGCOUNTER

t_bool CompareTwoBlocks(t_bbl *blocka, t_bbl *blockb)
{
  t_ins *insa, *insb;
  t_bool (*InsCmp)(t_ins *, t_ins *);

  if (BBL_NINS(blocka) != BBL_NINS(blockb)) return FALSE;

  InsCmp = CFG_DESCRIPTION(BBL_CFG(blocka))->InsCmp;

  insa = BBL_INS_FIRST(blocka);
  insb = BBL_INS_FIRST(blockb);
  while (insa)
  {
    if (!InsCmp(insa,insb))
      return FALSE;
    insa = INS_INEXT(insa);
    insb = INS_INEXT(insb);
  }
  return TRUE;
}

/* custom hash table structure for bbl comparison - much easier to set up than the generic hash table */
#define TABLE_SIZE	1024
typedef struct _bbl_hash_entry {
  struct _bbl_hash_entry *next;
  t_bbl *bbl;
} bbl_hash_entry;

#define AddToHolder(holder,block)	\
	do { \
	  if (holder.bbl) holder.bbl = Realloc(holder.bbl,(holder.nbbls+1)*sizeof(t_bbl *)); \
	  else holder.bbl = Malloc(sizeof(t_bbl *)); \
	  holder.bbl[holder.nbbls++] = block; \
	} while (0)


#define CanFactor(bbl)	(CFG_DESCRIPTION(cfg)->CanFactor ? CFG_DESCRIPTION(cfg)->CanFactor(bbl) : TRUE)


t_bool BblFactorInit(t_cfg * cfg)
{
	CfgInitBblFactor(CFG_OBJECT(cfg));
	CfgInitBblFingerprint(CFG_OBJECT(cfg));
	CfgInitBblCanBeFactored(CFG_OBJECT(cfg));
	DiabloBrokerCall("BblFactorInit",cfg);
	if (!CFG_BBL_FACTOR(cfg))
	{
		WARNING(("Architecture does not support basic block factoring (bbl factor not set)"));
		return FALSE;
	}
	else if (!CFG_BBL_FINGERPRINT(cfg))
	{
		 WARNING(("Architecture does not support basic block factoring (bbl fingerprint not set)"));
		return FALSE;
	}
	else if (!CFG_BBL_CAN_BE_FACTORED(cfg))
	{
		 WARNING(("Architecture does not support basic block factoring (bbl can be factored not set)"));
		return FALSE;
	}


	return TRUE;
}

void BblFactorFini(t_cfg * cfg)
{
	CfgFiniBblFactor(CFG_OBJECT(cfg));
	CfgFiniBblFingerprint(CFG_OBJECT(cfg));
	CfgFiniBblCanBeFactored(CFG_OBJECT(cfg));
}
												  
void BblFactoring(t_cfg *cfg, t_randomnumbergenerator *rng)
{
  int i,nins;
  bbl_hash_entry *table[TABLE_SIZE];
  bbl_hash_entry *he, *he2;
  t_bbl *bbl;
  int factorcount = 0;
  static int totalcount = 0;

  STATUS(START, ("BblFactoring"));

  /* make sure we have accurate liveness information */
  CfgComputeSavedChangedRegisters(cfg);
  CfgComputeLiveness(cfg,CONTEXT_SENSITIVE);

  if (diabloflowgraph_options.blockprofilefile)
    CfgComputeHotBblThreshold(cfg, 0.97);
  
  /* {{{ build hash table */
  for (i = 0; i < TABLE_SIZE; i++)
    table[i] = NULL;

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_uint32 index;
    bbl_hash_entry *new;
    
    if (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl)) continue;

    if (IS_DATABBL(bbl)) continue;
    if (!CFG_BBL_CAN_BE_FACTORED(cfg)(bbl)) continue;
    if (!BBL_INS_LAST(bbl)) FATAL(("Canfactor should say NO! @B",bbl));

    /* split off control flow instructions */
    if (CFG_DESCRIPTION(cfg)->InsIsControlflow(BBL_INS_LAST(bbl)))
      BblSplitBlock(bbl,BBL_INS_LAST(bbl),TRUE);

    index = CFG_BBL_FINGERPRINT(cfg)(bbl) % TABLE_SIZE;
    new = Malloc(sizeof(bbl_hash_entry));
    new->next = table[index];
    new->bbl = bbl;
    table[index] = new;
  } /* }}} */

  /* {{{ build equivalence relations and perform factoring */
  BblMarkInit();
  for (i = 0; i < TABLE_SIZE; i++)
  {
    for (he = table[i]; he; he = he->next)
    {
      t_equiv_bbl_holder holder = {0,0};
      if (BblIsMarked(he->bbl)) continue;
      BblMark(he->bbl);
      AddToHolder(holder,he->bbl);

      for (he2 = he->next; he2; he2 = he2->next)
      {
	if (BblIsMarked(he2->bbl)) continue;
	if (CompareTwoBlocks(he->bbl,he2->bbl))
	{
	  AddToHolder(holder,he2->bbl);
	  BblMark(he2->bbl);
	}
      }

      if (holder.nbbls > 1)
      {
	nins = BBL_NINS(he->bbl);
	/*if (totalcount < diablosupport_options.debugcounter)*/
	{
	  int i;
	  VERBOSE(1,("BBL FACTOR: TRY @iB",holder.bbl[0]));
	  for (i = 1; i < holder.nbbls; i++)
	    VERBOSE(1,("slave @B",holder.bbl[i]));

	  if (CFG_BBL_FACTOR(cfg)(&holder,holder.bbl[0]))
	  {
	    VERBOSE(1,("SUCCES"));
	    factorcount += (holder.nbbls - 1) * nins - holder.nbbls;
	    totalcount++;
	  }
	}
      }

      Free(holder.bbl);
    }
  } /* }}} */

  /* {{{ tear down the hash table */
  for (i=0; i<TABLE_SIZE; i++)
  {
    while (table[i])
    {
      bbl_hash_entry *tmp = table[i];
      table[i] = tmp->next;
      Free(tmp);
    }
    table[i] = NULL;
  } /* }}} */

  /* merge the basic blocks with their split-off tails */
  MergeBbls(cfg);

  VERBOSE(0,("approx %d instructions gained (total factorings: %d)",factorcount,totalcount));
  STATUS(STOP, ("BblFactoring"));
}


static void DoEpilogueFactoring(t_equiv_bbl_holder *equivs)
{
  t_bbl *master = equivs->bbl[0];
  int i;

  VERBOSE(1,("XI master @ieB", master));

  for (i = 0; i < equivs->nbbls; i++)
  {
    t_ins *ins;
    t_bbl *bbl = equivs->bbl[i];

    if (bbl == master) continue;

    VERBOSE(1,("--slave @ieB", bbl));

    ASSERT (BBL_SUCC_FIRST(bbl) == BBL_SUCC_LAST(bbl),
	("only unconditional return blocks"));

    DiabloBrokerCall ("EpilogueFactorBefore", bbl, master);

    CfgEdgeKill (BBL_SUCC_FIRST (bbl));

    BBL_SET_EXEC_COUNT(master,  BBL_EXEC_COUNT(master) +BBL_EXEC_COUNT(bbl));

    ins = BBL_INS_FIRST(master);
    
    while (BBL_INS_FIRST(bbl))
      {
	INS_SET_EXEC_COUNT(ins,  INS_EXEC_COUNT(ins) +INS_EXEC_COUNT(BBL_INS_FIRST(bbl)));
	InsKill(BBL_INS_FIRST(bbl));
	ins = INS_INEXT(ins);
      }


    ins = InsNewForBbl(bbl);
    CFG_DESCRIPTION(BBL_CFG(bbl))->InsMakeDirectBranch(ins);
    InsAppendToBbl(ins,bbl);

    if (BBL_FUNCTION(bbl) == BBL_FUNCTION(master))
      CfgEdgeCreate(BBL_CFG(bbl),bbl,master,ET_JUMP);
    else
    {
      t_cfg_edge *edge;
      edge = CfgEdgeCreate(BBL_CFG(bbl),bbl,master,ET_IPJUMP);
      CfgEdgeCreateCompensating(BBL_CFG(bbl),edge);
    }

    DiabloBrokerCall ("EpilogueFactorAfter", bbl, master);
  }
}

void ExternDoEpilogueFactoring(t_equiv_bbl_holder*equivs)
{
  return DoEpilogueFactoring(equivs);
}

void FunctionEpilogueFactoring(t_cfg *cfg)
{
  t_function *fun;
  t_cfg_edge *edge;
  t_bbl *bbl;
  int i;
  static int totalcount = 0;

  bbl_hash_entry *table[TABLE_SIZE];
  bbl_hash_entry *he, *he2;

  if (!CFG_DESCRIPTION(cfg)->InsMakeDirectBranch) return;
  
  STATUS(START, ("Function Epilogue Factoring"));
  if (diabloflowgraph_options.blockprofilefile)
    CfgComputeHotBblThreshold (cfg, 0.97);
  /* {{{ build hash table */
  memset(&(table[0]),0,TABLE_SIZE*sizeof(bbl_hash_entry *));
  CFG_FOREACH_FUN(cfg,fun)
  {
    t_bbl *exitblock = FunctionGetExitBlock(fun);
    if (!exitblock) continue;

    /* only unconditional exit blocks */
    BBL_FOREACH_PRED_EDGE(exitblock,edge)
      if (CFG_EDGE_CAT(edge) == ET_JUMP &&
	  !CFG_EDGE_SUCC_NEXT(edge) && !CFG_EDGE_SUCC_PREV(edge) &&
	  BBL_NINS(CFG_EDGE_HEAD(edge)) > 2)
      {
	/* insert block in hash table */
	t_uint32 index;
        bbl_hash_entry *new;

	if (diabloflowgraph_options.blockprofilefile && BblIsHot(CFG_EDGE_HEAD(edge))) continue;

	new = Malloc(sizeof(bbl_hash_entry));
	
	bbl = CFG_EDGE_HEAD(edge);
	index = CFG_BBL_FINGERPRINT(cfg)(bbl) % TABLE_SIZE;
	new->bbl = bbl;
	new->next = table[index];
	table[index] = new;
      }
  } /* }}} */

  /* {{{ build equivalence relations and perform factoring */
  BblMarkInit();
  for (i = 0; i < TABLE_SIZE; i++)
  {
    for (he = table[i]; he; he = he->next)
    {
      t_equiv_bbl_holder holder = {0,0};
      if (BblIsMarked(he->bbl)) continue;
      BblMark(he->bbl);
      AddToHolder(holder,he->bbl);

      for (he2 = he->next; he2; he2 = he2->next)
      {
	if (BblIsMarked(he2->bbl)) continue;
	if (CompareTwoBlocks(he->bbl,he2->bbl))
	{
	  AddToHolder(holder,he2->bbl);
	  BblMark(he2->bbl);
	}
      }

      if (holder.nbbls > 1)
      {
	/*if (totalcount < diablosupport_options.debugcounter)*/
	{
	  DoEpilogueFactoring(&holder);
	  totalcount++;
	}
      }

      Free(holder.bbl);
    }
  } /* }}} */

  /* {{{ tear down the hash table */
  for (i=0; i<TABLE_SIZE; i++)
  {
    while (table[i])
    {
      bbl_hash_entry *tmp = table[i];
      table[i] = tmp->next;
      Free(tmp);
    }
    table[i] = NULL;
  } /* }}} */

  VERBOSE(0,("%d times",totalcount));
  STATUS(STOP, ("Function Epilogue Factoring"));
}

/** {{{ CompareTwoFunctions */
#define BBL_CORRESPONDING(bbl)	((t_bbl *)BBL_TMP(bbl))
#define BBL_SET_CORRESPONDING(bbl, x)	BBL_SET_TMP(bbl,x)

/* find corresponding blocks in two supposedly identical functions.
 * return FALSE if not all blocks have a correspondence 
 * TODO a more accurate version of this function would create equivalence
 * classes, and look at the outgoing edges to find possible mappings. */
t_bool FindCorrespondingBlocks (t_function *funa, t_function *funb)
{
  t_bbl *bbl, *iter;
  t_uint32 nbbla, nbblb;
  t_cfg_edge *edgea, *edgeb;

  if (!CFG_DESCRIPTION(FUNCTION_CFG(funa))->InsCmp)
    return FALSE;

  /* shortcut: the corresponding block of the entry of funa
   * should be the entry of funb. if these two don't match,
   * there's no use in comparing anything else. */
  if (!FUNCTION_BBL_FIRST(funa) || !FUNCTION_BBL_FIRST(funb) ||
      !CompareTwoBlocks(FUNCTION_BBL_FIRST(funa),FUNCTION_BBL_FIRST(funb)))
    return FALSE;

  /* count the number of blocks in both functions */
  nbbla = nbblb = 0;
  FUNCTION_FOREACH_BBL(funa,bbl)
  {
    nbbla++;
    BBL_SET_CORRESPONDING(bbl, NULL);
  }
  FUNCTION_FOREACH_BBL(funb,bbl)
    nbblb++;
  if (nbbla != nbblb) return FALSE;

  BblMarkInit();

  FUNCTION_FOREACH_BBL(funa,bbl)
  {
    int n_outgoing_edges = 0;

    BBL_FOREACH_SUCC_EDGE(bbl,edgea)
      n_outgoing_edges++;

    /* find corresponding block in funb */
    FUNCTION_FOREACH_BBL(funb,iter)
    {
      int count_outgoing = 0;

      if (BblIsMarked(iter))
	continue;	/* already corresponds to a different block */

      if (!CompareTwoBlocks(bbl,iter)) continue;
      
      /* check the number of outgoing edges, except for the dummy exit block */
      if (bbl == FunctionGetExitBlock(funa))
	break;
      BBL_FOREACH_SUCC_EDGE(iter,edgeb)
	count_outgoing++;
      if (count_outgoing == n_outgoing_edges)
	break;
    }

    if (iter)
    {
      BblMark(iter);
      BBL_SET_CORRESPONDING(bbl,  iter);
    }
    else
      return FALSE;
  }

  /* check if the outgoing edges of corresponding blocks match */
  FUNCTION_FOREACH_BBL(funa,bbl)
  {
    t_bbl *corr = (t_bbl *) BBL_CORRESPONDING(bbl);
    BBL_FOREACH_SUCC_EDGE(bbl,edgea)
    {
      /* skip the outgoing return and compensating edges */
      if (bbl == FunctionGetExitBlock(funa)) continue;

      /* find corresponding outgoing edge for the corresponding block */
      BBL_FOREACH_SUCC_EDGE(corr,edgeb)
      {
	if (CFG_EDGE_CAT(edgea) != CFG_EDGE_CAT(edgeb)) continue;
	if (CFG_EDGE_CAT(edgea) & (ET_SWITCH|ET_IPSWITCH))
	  if (CFG_EDGE_SWITCHVALUE(edgea) != CFG_EDGE_SWITCHVALUE(edgeb))
	    continue;
	break;
      }
      if (!edgeb) return FALSE;
      if (CfgEdgeIsForwardInterproc(edgea))
      {
	if (CFG_EDGE_TAIL(edgea) != CFG_EDGE_TAIL(edgeb)) return FALSE;
      }
      else
      {
	if (BBL_CORRESPONDING(CFG_EDGE_TAIL(edgea)) != CFG_EDGE_TAIL(edgeb))
	  return FALSE;
      }
    }
  }

  return TRUE;
}

t_bool CompareTwoFunctions(t_function *funa, t_function *funb, t_bool transfer_execution_counts)
{
  t_bbl *bbl;
  t_bool has_correspondence = FindCorrespondingBlocks (funa, funb);

  if (!has_correspondence) return FALSE;

  if (transfer_execution_counts)
  {
    FUNCTION_FOREACH_BBL (funa, bbl)
    {
      t_bbl *corr = BBL_CORRESPONDING (bbl);
      t_ins * ins1 = BBL_INS_FIRST(bbl), * ins2 = BBL_INS_FIRST(corr);

      BBL_SET_EXEC_COUNT(bbl, BBL_EXEC_COUNT(bbl) + BBL_EXEC_COUNT(corr));
      DiabloBrokerCall ("RegisterBblFactoring", corr, bbl);

      while (ins1)
      {
	INS_SET_EXEC_COUNT(ins1, INS_EXEC_COUNT(ins1)+INS_EXEC_COUNT(ins2));
	ins1 = INS_INEXT(ins1);
	ins2 = INS_INEXT(ins2);
      }
    }
  }

  return TRUE;
}
#undef BBL_CORRESPONDING
#undef BBL_SET_CORRESPONDING
/* }}} */

/** {{{ WholeFunctionFactoring */
void WholeFunctionFactoring(t_cfg * cfg)
{
  static int factorcount=0;
  t_function *fun1,*fun2;
  t_bbl * bbl;
  t_symbol * sym;

  STATUS(START, ("Whole function factoring"));

  BblMarkInit2();

  /* preparation of precomputed dynamic symbol info */

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_SET_ATTRIB(bbl, BBL_ATTRIB(bbl) | CAN_SKIP);

  if (OBJECT_DYNAMIC_SYMBOL_TABLE(CFG_OBJECT(cfg)))
  {
    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_DYNAMIC_SYMBOL_TABLE(CFG_OBJECT(cfg)),sym)
      if (RELOCATABLE_RELOCATABLE_TYPE(T_RELOCATABLE(SYMBOL_BASE(sym)))==RT_BBL)
        BBL_SET_ATTRIB(T_BBL(SYMBOL_BASE(sym)), BBL_ATTRIB(T_BBL(SYMBOL_BASE(sym))) & ~CAN_SKIP);
  }

  /* end preparation of precomputed dynamic symbol info */

  CFG_FOREACH_FUN(cfg,fun1)
    if (FUNCTION_BBL_FIRST(fun1) && !BblIsMarked2(FUNCTION_BBL_FIRST(fun1)))
    {
      CFG_FOREACH_FUN(cfg,fun2)
	if (fun1!=fun2 
	    && FUNCTION_BBL_FIRST(fun2)
	    && !BblIsMarked2(FUNCTION_BBL_FIRST(fun2))
	    && AddressIsLt(BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun1)),BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun2)))) 
	{
	  if (CompareTwoFunctions(fun1,fun2,FALSE))
	  {
            t_bbl *first;
            /* don't factor functions that contain only a single instruction and
             * that are referred by a relocation (that one instruction is probably
             * a return, and due to the relocation we have to add a branch, and
             * a branch to the return of the other function is less efficient
             * and possibly larger than just the original return)
             */
            first = FUNCTION_BBL_FIRST(fun1);
            if (!(BBL_REFED_BY(first) &&
                  (BBL_NINS(first) == 1) &&
                  (CFG_EDGE_TAIL(BBL_SUCC_FIRST(first)) == FunctionGetExitBlock(fun1)) &&
                  (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(first)) == NULL)))
            {
              ++factorcount;
              VERBOSE(1,("Factoring %s (@G) and %s (@G)",
                    FUNCTION_NAME(fun1),BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun1)),
                    FUNCTION_NAME(fun2),BBL_OLD_ADDRESS(FUNCTION_BBL_FIRST(fun2))
                    ));
              BblMark2(FUNCTION_BBL_FIRST(fun2));
              MoveDirectCallEdgesFromTo(fun2,fun1,TRUE);
              CompareTwoFunctions(fun1,fun2,TRUE);
            }
	  }
	}
    }
  STATUS(STOP, ("Whole function factoring"));
} /* }}} */

/** {{{ Move call edges from one function to another, identical, one.
 *  Returns TRUE if there was at least one edge moved */
/* Some more information:
 *
 * For correctness, hell edges shouldn't be moved. If they are, program 
 * behaviour will change in the following case:
 *
 * int f() { return 0; }
 * int g() { return 0; }
 * int main() {
 * 	if (f == g) printf("equal\n");
 * 	else printf("unequal\n");
 * }
 *
 * However, we can leave the hell edges to g(), and replace the first 
 * block of g() with a branch to f(). This way, the pointers differ 
 * but the code is shared. */
t_bool MoveDirectCallEdgesFromTo(t_function * from, t_function * to, t_bool use_precomputed_symbol_info)
{
  t_cfg_edge * call, * tmp, * edge, * newedge, * e;
  t_bbl * entry = FUNCTION_BBL_FIRST(from);
  t_bbl * caller, * returnsite = NULL;
  t_bbl * bbl;
  t_bbl * new;
  t_reloc_ref * rr, * rr2;
  t_bool any_incoming_moved = FALSE;
  t_bool rels_with_hell_edges = FALSE;
  t_symbol *sym;
  t_address old_address;


  /* don't do hell and call hell functions */
  if (BBL_IS_HELL(entry) || BBL_IS_HELL(FUNCTION_BBL_FIRST(to))) return FALSE;

  /* skip stub functions:
   * if functions have incoming hell edges, after factoring a stub is left
   * that jumps to the factored function. The explanation why this is necessary
   * is at the beginning of MoveDirectCallEdgesFromTo(). However, these
   * stub functions are all identical, so we must avoid factoring them again */
  if (BBL_NINS (entry) == 1 &&
      BBL_SUCC_FIRST (entry) &&
      BBL_SUCC_FIRST (entry) == BBL_SUCC_LAST (entry) &&
      CFG_EDGE_CAT (BBL_SUCC_FIRST (entry)) == ET_IPJUMP)
  {
    /* this conclusively proves that the function is actually just a stub.
     * we refuse to factor it. */
    return FALSE;
  }

  /*static int nfactors = 0;*/
  /*if (nfactors++ >= diablosupport_options.debugcounter) return FALSE;*/

  VERBOSE(1, ("FROMTOING @B -> @B\n", 
	FUNCTION_BBL_FIRST (from), FUNCTION_BBL_FIRST (to)));

  old_address = BBL_CADDRESS(FUNCTION_BBL_FIRST(from));

  DiabloBrokerCall ("FunctionFactorBefore", from, to);

  /* move non-hell call edges */
  BBL_FOREACH_PRED_EDGE_SAFE(entry, call, tmp)
  {
    if (CFG_EDGE_CAT(call) != ET_CALL) continue;
    if (BBL_IS_HELL(CFG_EDGE_HEAD(call))) continue;

    if (CFG_EDGE_CORR(call))
    {
      returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(call));
      CfgEdgeKill(CFG_EDGE_CORR(call));
    }
    else
    {
      returnsite = NULL;	
    }

    caller = CFG_EDGE_HEAD(call);
    CfgEdgeKill(call);
    
    newedge = CfgEdgeCreateCall(
	FUNCTION_CFG(from),
	caller,FUNCTION_BBL_FIRST(to),returnsite,FunctionGetExitBlock(to));
    /* if there is no return site, kill the newly made return edge */
    if (!returnsite)
    {
      CfgEdgeKill (CFG_EDGE_CORR (newedge));
      CFG_EDGE_SET_CORR (newedge, NULL);
    }

    any_incoming_moved = TRUE;
  }

  /* move incoming ipjump and ipfallthrough edges. Don't do this for other
   * incoming interprocedural edges because: 
   * - ipunknown is impossible to move as we don't know what it is 
   * - ipswitch is difficult because of the differences in switch
   * implementations on different architectures.
   */
  FUNCTION_FOREACH_BBL(from,bbl) 
  { 
    BBL_FOREACH_PRED_EDGE_SAFE(bbl,edge,tmp) 
    { 
      t_bbl *corresponding = NULL; 
      t_bbl *iter; 
      t_bbl *head;
      t_cfg_edge *new_edge;

      if (CFG_EDGE_CAT(edge) != ET_IPJUMP && 
	  CFG_EDGE_CAT(edge) != ET_IPFALLTHRU) 
	continue;

      /* find the corresponding block in the other function */
      FUNCTION_FOREACH_BBL(to,iter)
	if (CompareTwoBlocks(bbl,iter))
	{
	  if (!corresponding) corresponding = iter;
	  else goto skip_this_block;
	}
      if (!corresponding)
	FATAL(("Equal functions, but cannot find corresponding block"));

      head = CFG_EDGE_HEAD(edge);
      if (BBL_FUNCTION(head) == to) continue;	/* too complicated */
      if (CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      {
	t_ins *jump;

	head = BblNew(FUNCTION_CFG(from));
	BblInsertInFunction(head,BBL_FUNCTION(CFG_EDGE_HEAD(edge)));
	BBL_SET_EXEC_COUNT(head,  CFG_EDGE_EXEC_COUNT(edge));

	CfgEdgeCreate(FUNCTION_CFG(from),CFG_EDGE_HEAD(edge),head,ET_FALLTHROUGH);
	CFG_EDGE_SET_EXEC_COUNT(BBL_PRED_FIRST(head),  BBL_EXEC_COUNT(head));

	jump = InsNewForBbl(head);
	InsAppendToBbl(jump,head);
	CFG_DESCRIPTION(FUNCTION_CFG(from))->InsMakeDirectBranch(jump);
	INS_SET_EXEC_COUNT(jump,  BBL_EXEC_COUNT(head));

        DiabloBrokerCall ("FunctionFactorAfterJumpCreation", head, CFG_EDGE_HEAD(edge));
      }

      /* add new ipjump edge */
      new_edge = CfgEdgeCreate(FUNCTION_CFG(from),head,corresponding,ET_IPJUMP);
      if (CFG_EDGE_CORR(edge))
	CfgEdgeCreateCompensating(FUNCTION_CFG(from),new_edge);

      /* remove old ipjump edge */
      if (CFG_EDGE_CORR(edge))
	CfgEdgeKill(CFG_EDGE_CORR(edge));
      CfgEdgeKill(edge);
    }
skip_this_block:
    ; /* empty statement to keep the compiler on prozac */
  }

  /* are there any incoming interprocedural edges that will not be moved? */
  if (diablosupport_options.verbose>=1)
  {
    FUNCTION_FOREACH_BBL(from,bbl)
    BBL_FOREACH_PRED_EDGE(bbl,tmp)
    {
      if (CfgEdgeIsForwardInterproc(tmp) && CFG_EDGE_CAT(tmp) != ET_CALL)
	VERBOSE(1,("UNMOVED INTERPROC @E",tmp));
    }
  }
  /* if there are relocations with hell edges to the entry block of the
   * function, do the branch trick */

  for (rr = BBL_REFED_BY(entry); rr; rr = RELOC_REF_NEXT(rr))
    if (RELOC_HELL(RELOC_REF_RELOC(rr)))
      rels_with_hell_edges = TRUE;
  if (rels_with_hell_edges)
  {
    t_ins * ins;
    t_cfg_edge * new_edge;

    /* {{{ add new block */
    new = BblNew(FUNCTION_CFG(from));
    BblInsertInFunction(new,from);
    /* the new block should be the function entry. This means we have to put
     * it at the front of the linked list of function blocks */
    if (BBL_NEXT_IN_FUN(new))
      BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(new),  BBL_PREV_IN_FUN(new));
    BBL_SET_NEXT_IN_FUN(BBL_PREV_IN_FUN(new),  BBL_NEXT_IN_FUN(new));
    BBL_SET_PREV_IN_FUN(new,  NULL);
    BBL_SET_NEXT_IN_FUN(new,  entry);
    FUNCTION_SET_BBL_FIRST(from,  new);
    BBL_SET_PREV_IN_FUN(entry,  new);

    ins = InsNewForBbl(new);
    INS_SET_OLD_ADDRESS(ins,old_address);
    InsAppendToBbl(ins,new);
    CFG_DESCRIPTION(FUNCTION_CFG(from))->InsMakeDirectBranch(ins);

    new_edge = CfgEdgeCreate(FUNCTION_CFG(from),new,FUNCTION_BBL_FIRST(to),ET_IPJUMP);
    if (FunctionGetExitBlock (from) && FunctionGetExitBlock (to))
      CfgEdgeCreateCompensating(FUNCTION_CFG(from),new_edge);

        DiabloBrokerCall ("FunctionFactorAfterJumpCreation", new, FUNCTION_BBL_FIRST(to));
    /* }}} */

    /* {{{ move all relocations with hell edges to this new block */
    for (rr = BBL_REFED_BY(entry), rr2 = rr ? RELOC_REF_NEXT(rr) : NULL;
	 rr;
	 rr = rr2, rr2 = rr ? RELOC_REF_NEXT(rr) : NULL)
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);

      if (RELOC_HELL(rel))
      {
	t_uint32 i;
	t_cfg_edge *edge = RELOC_EDGE(rel);

        for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
        {
          if (RELOC_TO_RELOCATABLE(rel)[i] == T_RELOCATABLE(entry))
          {
            RelocSetToRelocatable(rel,i, T_RELOCATABLE(new));
          }
        }
        
	if (CFG_EDGE_CORR(edge))
	{
	  returnsite = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
	  CfgEdgeKill(CFG_EDGE_CORR(edge));
	}
	else
	  returnsite = NULL;

	caller = CFG_EDGE_HEAD(edge);
	CfgEdgeKill(edge);
	
	BBL_FOREACH_PRED_EDGE(new, e)
	  if (CFG_EDGE_HEAD(e) == caller && CFG_EDGE_CAT(e) == ET_CALL)
	    break;
	if (e)
	{
	  CFG_EDGE_SET_REFCOUNT(e, CFG_EDGE_REFCOUNT(e)+1);
	  edge = e;
	}
	else
	{
	  edge = CfgEdgeCreateCall(FUNCTION_CFG(from),caller,new,returnsite,FunctionGetExitBlock(from));
	}
	RELOC_SET_EDGE(rel, edge);
      }
      /* don't move relocations without hell edge: they belong to ip switches and other
       * weird stuff we'd rather not touch */
    }
    /* }}} */

    any_incoming_moved = TRUE;
  }
  else
  {
    new = FUNCTION_BBL_FIRST(to);
  }

  /* {{{ move all symbols that had the old bbl as base to the new block
   *     (mainly to ensure that dynamic export symbols don't get killed)
   */
  if (OBJECT_DYNAMIC_SYMBOL_TABLE(CFG_OBJECT(FUNCTION_CFG(from))))
  {
    if (!use_precomputed_symbol_info || !(BBL_ATTRIB(entry) & CAN_SKIP))
    {
      SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_DYNAMIC_SYMBOL_TABLE(CFG_OBJECT(FUNCTION_CFG(from))),sym)
      {
        if (SYMBOL_BASE(sym) == T_RELOCATABLE(entry))
        {
          SymbolSetBase(sym,T_RELOCATABLE(new));
          BBL_SET_ATTRIB(new, BBL_ATTRIB(new) & ~CAN_SKIP);
        }
      }
    }
  }
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(CFG_OBJECT(FUNCTION_CFG(from))),sym)
  {
    if (SYMBOL_BASE(sym) == T_RELOCATABLE(entry))
    {
      SymbolSetBase(sym,T_RELOCATABLE(new));
      BBL_SET_ATTRIB(new, BBL_ATTRIB(new) & ~CAN_SKIP);
    }
  }
  /* }}} */

  DiabloBrokerCall ("FunctionFactorAfter", from, to);
  return any_incoming_moved;
} /* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
