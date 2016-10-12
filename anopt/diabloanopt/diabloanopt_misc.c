/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloanopt.h>
#ifdef MIPS_SUPPORT
#include "../arch/mips/mips_opcodes.h"
#endif

#define BE_VERBOSITY_LEVEL 4

t_bool DefineIsLocal(t_ins * ins)
{
  t_regset defined;
  t_ins * i_ins;

  RegsetSetDup(defined, INS_REGS_DEF(ins));

  i_ins = INS_INEXT(ins);

  while (i_ins)
  {
    if (!INS_IS_CONDITIONAL(i_ins))
      RegsetSetDiff(defined,INS_REGS_DEF(i_ins));
    i_ins = INS_INEXT(i_ins);
  }

  return RegsetIsEmpty(defined);
}

/* merge blocks with only fallthrough out with successors with only fallthrough in */
void MergeBbls(t_cfg * cfg)
{
  t_cfg_edge * i_edge, * j_edge, * tmp;
  t_bbl * head;
  t_bbl * tail;
  t_ins * i_ins;
  int counter = 0;
  /*  static int total_count = 0; */

  STATUS(START, ("Bbl Merging"));
  CFG_FOREACH_EDGE_SAFE(cfg,i_edge,tmp)
  {
    t_bool can_merge = TRUE;
    /* {{{ check for possibilities */
    if (CFG_EDGE_CAT(i_edge)!=ET_FALLTHROUGH) continue; 
    head = CFG_EDGE_HEAD(i_edge);
    tail = CFG_EDGE_TAIL(i_edge);
    if (BBL_SUCC_FIRST(head)!=BBL_SUCC_LAST(head)) continue;
    if (BBL_PRED_FIRST(tail)!=BBL_PRED_LAST(tail)) continue;
    if (!BBL_FUNCTION(tail)) continue; /* Ignore blocks with no fun, they are not connected to the graph anyway */
    if (FunctionGetExitBlock(BBL_FUNCTION(tail))==tail) continue;
    if (BBL_FUNCTION(head)!=BBL_FUNCTION(tail)) continue;
    if (BBL_REFED_BY(tail)) continue;

    t_bool does_broker_allow = TRUE;
    DiabloBrokerCall ("BblMergeAllowed", head, tail, &does_broker_allow);

    if (!does_broker_allow)
      continue;
    /* }}} */

    DiabloBrokerCall ("BblCanMerge", head, tail, &can_merge);
    if (!can_merge) continue;

    DiabloBrokerCall ("BblMergeBefore", head, tail);
    /* {{{ do the merge */
    BBL_FOREACH_INS(tail,i_ins)
      INS_SET_BBL(i_ins, head);

    if (BBL_INS_FIRST(tail))
    {
      INS_SET_IPREV(BBL_INS_FIRST(tail), BBL_INS_LAST(head));
      if (!BBL_INS_FIRST(head))
	BBL_SET_INS_FIRST(head, BBL_INS_FIRST(tail));
    }

    if (BBL_INS_LAST(head))
    {
      INS_SET_INEXT(BBL_INS_LAST(head), BBL_INS_FIRST(tail));
    }

    if (BBL_INS_LAST(tail))
      BBL_SET_INS_LAST(head, BBL_INS_LAST(tail));

    BBL_SET_CSIZE(head,  AddressAdd(BBL_CSIZE(head),BBL_CSIZE(tail)));
    BBL_SET_NINS(head, BBL_NINS(head)+BBL_NINS(tail));
    BBL_SET_NINS(tail, 0);
    BBL_SET_INS_FIRST(tail, NULL);
    BBL_SET_INS_LAST(tail, NULL);

    CfgEdgeKill(i_edge);

    BBL_FOREACH_SUCC_EDGE(tail,j_edge)
    {
      CFG_EDGE_SET_HEAD(j_edge,  head);
    }

    BBL_SET_SUCC_FIRST(head, BBL_SUCC_FIRST(tail));
    BBL_SET_SUCC_LAST(head, BBL_SUCC_LAST(tail));

    BBL_SET_SUCC_FIRST(tail, NULL);
    BBL_SET_SUCC_LAST(tail, NULL);

    /* We have to assure that the bbl following a switch statement is indeed a
     * switch block */
    if ((BBL_NEXT(tail))&&(IS_SWITCH_TABLE(BBL_NEXT(tail))))
    {
      t_bbl * sbbl=BBL_NEXT(tail);

      BBL_SET_NEXT(tail, BBL_NEXT(sbbl));

      if (BBL_NEXT(sbbl))
	BBL_SET_PREV(BBL_NEXT(sbbl), tail);

      /* remove from list of nodes */
      if (sbbl == CFG_NODE_FIRST(cfg))
      {
	FATAL(("Switchtable is first block in program. This cannot happen!"));
      }
      else if (sbbl == CFG_NODE_LAST(cfg))
      {
	FATAL(("Implement corner case where switchtable is last bbl in cfg in MergeBbls!"));
      }


      BBL_SET_NEXT(sbbl, BBL_NEXT(head));

      if (BBL_NEXT(head))
	BBL_SET_PREV(BBL_NEXT(head), sbbl);
      BBL_SET_PREV(sbbl, head);
      BBL_SET_NEXT(head, sbbl);
    }

    BblKill(tail);

    counter++;

    VERBOSE(10,("result @ieB\n",head));

    /* }}} */
    DiabloBrokerCall ("BblMergeAfter", head);
  }

  STATUS(STOP, ("Bbl Merging (%d merges)",counter));
}


void BranchForwarding(t_cfg * cfg)
{

  t_bbl * i_bbl;
  t_cfg_edge * new_edge, * edge;
  t_ins * ins;
  t_bbl * bbl1, *bbl2, *bbl3;
  static int nr_forwarded_branches = 0;

  STATUS(START, ("Branch Forwarding"));
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bool is_return = FALSE;
    if (BBL_NINS(i_bbl)==1)
    {
      t_bool ok=TRUE;
      t_cfg_edge * i_edge=NULL;
      t_cfg_edge * j_edge=NULL;
      BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
      {
	switch	(CFG_EDGE_CAT(i_edge))
	{
	  case ET_FALLTHROUGH:
	  case ET_IPFALLTHRU:
	  
	  case ET_UNKNOWN:   
	  case ET_IPUNKNOWN:   
	  
	  case ET_RETURN:   
	  case ET_CALL:   
	  
	  case ET_SWI: 
	  case ET_COMPENSATING:
	  
	  case ET_SWITCH:
	  case ET_IPSWITCH:
	    ok=FALSE;
	    break;
	    
	  
	  case ET_IPJUMP:
	  case ET_JUMP:
	    if (j_edge) FATAL(("Two jump edges?"));
	    if (!BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))) { ok=FALSE; break;}
	    if (CFG_EDGE_HEAD(i_edge)==CFG_EDGE_TAIL(i_edge)) { ok=FALSE; break;}
	    if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) { ok=FALSE; break;}
	    if (BBL_IS_HELL((CFG_EDGE_TAIL(i_edge)))) { ok=FALSE; break;}
	    /*if ((FunctionGetExitBlock(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))))==CFG_EDGE_TAIL(i_edge)) { ok=FALSE; break;}*/
	    if ((FunctionGetExitBlock(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))))==CFG_EDGE_TAIL(i_edge))
	    { 
	      is_return = TRUE;
	    }

	    j_edge=i_edge;
	    break;

	  default:
	    FATAL(("Edge not handled!"));

	}
	if (!ok) break;
      }

      if (ok)
      {
        t_bool do_forwarding = TRUE;
        t_bool found=FALSE;

        do
        {
          found=FALSE;
          BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
          {
            switch (CFG_EDGE_CAT(i_edge))
            {
              case ET_FALLTHROUGH:
              case ET_IPFALLTHRU:
                break;

              case ET_UNKNOWN:
              case ET_IPUNKNOWN:
                break;

              case ET_RETURN:
                break;
              case ET_CALL:
                VERBOSE(1,("Missing opportunity!\n"));

              case ET_SWI:
              case ET_COMPENSATING:
                break;

              case ET_SWITCH:
              case ET_IPSWITCH:
                break;

              case ET_IPJUMP:
              case ET_JUMP:
                {
                  t_function *from, *to;

                      DiabloBrokerCall ("BranchForwardingDo", i_edge, &do_forwarding);
                      if (!do_forwarding) break;

		  if (CFG_EDGE_HEAD(i_edge)==CFG_EDGE_TAIL(i_edge)) break;
		  if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) { break;}
		  if (BBL_IS_HELL((CFG_EDGE_TAIL(i_edge)))) { break;}

		  /* don't break the single entry point assumption by forwarding an
		   * interprocedural branch to a function entry point to a subsequent bbl of
		   * that function
		   */
		  if (BBL_FUNCTION(i_bbl) &&
		      (FUNCTION_BBL_FIRST(BBL_FUNCTION(i_bbl)) == i_bbl) &&
		      (CFG_EDGE_CAT(i_edge) == ET_IPJUMP))
		  {
		    break;
		  }
      

		  if (!is_return)
		  {
        if (j_edge)
        {
          /* forwarding a regular branch */
          bbl1 = CFG_EDGE_HEAD(i_edge);
          bbl2 =  CFG_EDGE_TAIL(i_edge);
          bbl3 = CFG_EDGE_TAIL(j_edge);

          from = BBL_FUNCTION(bbl1);
          to = BBL_FUNCTION(bbl3);

          nr_forwarded_branches++;
          /* if (nr_forwarded_branches >= diablosupport_options.debugcounter)
             break;
             */

          VERBOSE(BE_VERBOSITY_LEVEL,("BEFORE @ieB\n -> @ieB\n ->@ieB\n",bbl1,bbl2,bbl3));

          if (from == to)
          {
            new_edge = CfgEdgeCreate(cfg,bbl1,bbl3,ET_JUMP);
          }
          else
          {
            new_edge = CfgEdgeCreate(cfg,bbl1,bbl3,ET_IPJUMP);
            if(FunctionGetExitBlock(from) && FunctionGetExitBlock(to))
              CfgEdgeCreateCompensating(cfg,new_edge);
            if (CFG_EDGE_CORR(new_edge))
              CFG_EDGE_SET_EXEC_COUNT(CFG_EDGE_CORR(new_edge),  CFG_EDGE_EXEC_COUNT(i_edge));
          }
          CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(i_edge));
          BBL_SET_EXEC_COUNT(i_bbl, BBL_EXEC_COUNT(i_bbl)-CFG_EDGE_EXEC_COUNT(i_edge));

          BBL_FOREACH_INS(i_bbl,ins)
          {
            if (INS_EXEC_COUNT(ins)-CFG_EDGE_EXEC_COUNT(i_edge) >= 0)
              INS_SET_EXEC_COUNT(ins,   INS_EXEC_COUNT(ins) - CFG_EDGE_EXEC_COUNT(i_edge));
          }

          BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
          {
            if (CFG_EDGE_EXEC_COUNT(edge)-CFG_EDGE_EXEC_COUNT(i_edge) >= 0)
              CFG_EDGE_SET_EXEC_COUNT(edge,     CFG_EDGE_EXEC_COUNT(edge) - CFG_EDGE_EXEC_COUNT(i_edge));
            if (CFG_EDGE_CORR(edge) && 
                CFG_EDGE_EXEC_COUNT(CFG_EDGE_CORR(edge)) >= CFG_EDGE_EXEC_COUNT(i_edge))
              CFG_EDGE_SET_EXEC_COUNT(CFG_EDGE_CORR(edge), CFG_EDGE_EXEC_COUNT(CFG_EDGE_CORR(edge)) -CFG_EDGE_EXEC_COUNT(i_edge));
          }

          if (CFG_EDGE_CORR(i_edge))
            CfgEdgeKill(CFG_EDGE_CORR(i_edge));
          CfgEdgeKill(i_edge);

          VERBOSE(BE_VERBOSITY_LEVEL,("AFTER @ieB\n -> @ieB\n ->@ieB\n",bbl1,bbl2,bbl3));

          found=TRUE;
        }
		  }
		  else
		  {
		    /* forwarding a return statement: move the return statement up */
		    /* do this only for unconditional branches to the return statement,
		     * unless we're on the ARM, where we can create a conditional
		     * return statement */

		    /* check if it's a conditional control transfer */
		    t_cfg_edge *succ;
		    t_ins *new;
		    t_uint32 exec_edge, exec_ins;
                    t_bool do_forwarding = TRUE;

		    bbl1 = CFG_EDGE_HEAD(i_edge);
		    BBL_FOREACH_SUCC_EDGE(bbl1,succ)
		      if (CFG_EDGE_CAT(succ) != ET_JUMP)
			break;
		    if (succ) break;	/* do nothing for conditional jumps to return statements */

        /* do not forward branches that are part of a switch table of branches */
        BBL_FOREACH_PRED_EDGE(bbl1,succ)
          if (CFG_EDGE_CAT(succ) == ET_SWITCH
              || CFG_EDGE_CAT(succ) == ET_IPSWITCH)
            do_forwarding = FALSE;
        if (!do_forwarding) break;

		    if (!FunctionGetExitBlock(BBL_FUNCTION(bbl1))) break; /* too complicated */

                    do_forwarding = TRUE;
                    DiabloBrokerCall ("ForwardToReturnDo",bbl1,&do_forwarding);

                    if (!do_forwarding) break; /* don't risk adding instructions to this block through later optimizations */
                    nr_forwarded_branches++;

                    /*if (nr_forwarded_branches >= diablosupport_options.debugcounter)
                      break;*/

		    VERBOSE(BE_VERBOSITY_LEVEL,("Will do return forwarding for @ieB and @ieB",i_bbl,bbl1));
		    exec_edge = CFG_EDGE_EXEC_COUNT(i_edge);
		    exec_ins = INS_EXEC_COUNT(BBL_INS_LAST(bbl1));
		    if (CFG_EDGE_CORR(i_edge))
		      CfgEdgeKill(CFG_EDGE_CORR(i_edge));
		    CfgEdgeKill(i_edge);
		    InsKill(BBL_INS_LAST(bbl1));

		    new = InsDup(BBL_INS_FIRST(i_bbl));
		    InsAppendToBbl(new,bbl1);
		    i_edge = CfgEdgeCreate(cfg,bbl1,FunctionGetExitBlock(BBL_FUNCTION(bbl1)), ET_JUMP);
		    CFG_EDGE_SET_EXEC_COUNT(i_edge,  exec_edge);
		    INS_SET_EXEC_COUNT(new,  exec_ins);

		    /* adjust exec count of i_bbl */
		    BBL_SET_EXEC_COUNT(i_bbl,   BBL_EXEC_COUNT(i_bbl) -exec_ins);
		    INS_SET_EXEC_COUNT(BBL_INS_FIRST(i_bbl),  INS_EXEC_COUNT(BBL_INS_FIRST(i_bbl)) -exec_ins);
		    CFG_EDGE_SET_EXEC_COUNT(BBL_SUCC_FIRST(i_bbl),   CFG_EDGE_EXEC_COUNT(BBL_SUCC_FIRST(i_bbl)) -  exec_ins);

		    found = TRUE;
		  }
		}
		break;

	      default:
		FATAL(("Edge not handled!"));

	    }
	    if (found)
	    {
	      /* Forward , break the loop*/
	      break;
	    }
	  }
	} while (found);
      }

    }
  }
  STATUS(STOP, ("Branch Forwarding (%u forwarded branches)", nr_forwarded_branches));
}


/* turn jump edges into fallthrough edges */
void BranchElimination(t_cfg * cfg)
{
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;

  static int nr_forwarded_branches = 0;

  STATUS(START, ("Branch elimination"));
  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bool flag_fallthrough = FALSE;
    t_bbl * j_bbl=NULL;
    t_cfg_edge * j_edge=NULL;
    t_uint32 count=0;
    
    if (BBL_IS_HELL(i_bbl)) continue;
    if (IS_DATABBL(i_bbl)) continue;
    if (i_bbl == CFG_UNIQUE_ENTRY_NODE(cfg)) continue;
    /* don't eliminate branches if they're the only instruction in a bbl
     * and the bbl is referred by a relocation, because that would
     * result in that relocation referring the jumped-to block, which is
     * exactly what we try to prevent by inserting such jumps during
     * function factoring
     */
    if ((BBL_NINS(i_bbl) == 1) &&
        BBL_REFED_BY(i_bbl))
      continue;
    
    BBL_FOREACH_SUCC_EDGE(i_bbl,i_edge)
    {
      if (
	  CFG_EDGE_CAT(i_edge)==ET_FALLTHROUGH || 
	  CFG_EDGE_CAT(i_edge)==ET_IPFALLTHRU || 
	  CFG_EDGE_CAT(i_edge)==ET_UNKNOWN ||  
	  CFG_EDGE_CAT(i_edge)==ET_RETURN ||  
	  CFG_EDGE_CAT(i_edge)==ET_CALL ||  
	  CFG_EDGE_CAT(i_edge)==ET_SWI ||
	  CFG_EDGE_CAT(i_edge)==ET_COMPENSATING ||  
	  CFG_EDGE_CAT(i_edge)==ET_SWITCH ||
	  CFG_EDGE_CAT(i_edge)==ET_IPSWITCH)
      {
	flag_fallthrough = TRUE;
	break;
      }
      else if (CFG_EDGE_CAT(i_edge)==ET_JUMP || CFG_EDGE_CAT(i_edge)==ET_IPJUMP)
      {
	if (j_bbl) { flag_fallthrough=TRUE; break;}
	j_bbl=CFG_EDGE_TAIL(i_edge);
	j_edge=i_edge;
      }
      else FATAL(("Implement @E\n",i_edge));
    }

    if (!j_bbl || flag_fallthrough || BBL_IS_HELL(j_bbl))
      continue;

    /* skip switch blocks */
    BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
    {
      t_cfg_edge * j_edge;
      BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(i_edge),j_edge)
      {
	if (CFG_EDGE_CAT(j_edge)==ET_SWITCH || CFG_EDGE_CAT(j_edge)==ET_IPSWITCH)
	{
	  flag_fallthrough = TRUE;
	  break;
	}
      }
    }

    if (flag_fallthrough)
      continue;

    BBL_FOREACH_PRED_EDGE(j_bbl,i_edge)
    {
      if (
	  CFG_EDGE_CAT(i_edge)==ET_FALLTHROUGH || 
	  CFG_EDGE_CAT(i_edge)==ET_IPFALLTHRU || 
	  CFG_EDGE_CAT(i_edge)==ET_RETURN || 
	  CFG_EDGE_CAT(i_edge)==ET_UNKNOWN ||  
	  CFG_EDGE_CAT(i_edge)==ET_SWITCH ||
	  CFG_EDGE_CAT(i_edge)==ET_IPSWITCH ||
          BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))
          )
      {
	flag_fallthrough = TRUE;
	break;
      }
      else 
      {
	count++;
      }
    }

    if (FtPath(j_bbl,i_bbl,NULL)) continue;
    if (j_bbl == i_bbl) continue;

    if (flag_fallthrough)
      continue;

    if (!CFG_DESCRIPTION(cfg)->InsIsUnconditionalJump(BBL_INS_LAST(i_bbl)))
      continue;

    if (BblIsExitBlock(CFG_EDGE_TAIL(j_edge)))
      continue;

    {
      t_bool do_elim = TRUE;
      DiabloBrokerCall ("BranchEliminationDo",
	  i_bbl, CFG_EDGE_TAIL (j_edge), &do_elim);
      if (!do_elim)
	continue;
    }

    VERBOSE (BE_VERBOSITY_LEVEL, ("Forwarding @I in @eiB", BBL_INS_LAST (i_bbl), i_bbl));

    InsKill(BBL_INS_LAST(i_bbl));

    nr_forwarded_branches++;

    if (CFG_EDGE_CAT(j_edge)==ET_JUMP)
      CFG_EDGE_SET_CAT(j_edge, ET_FALLTHROUGH);
    else if(CFG_EDGE_CAT(j_edge)==ET_IPJUMP)
      CFG_EDGE_SET_CAT(j_edge, ET_IPFALLTHRU);
    else
      FATAL(("Oeps!"));
  }     
  STATUS(STOP, ("Branch elimintation (%d eliminated branches)",nr_forwarded_branches));
}

void MarkAllSuccessorsOfEdge(t_cfg * cfg, t_cfg_edge * edge)
{
  t_bbl * bbl;
  t_cfg_edge * i_edge;
  t_bool change = TRUE;
  
  BblMarkInit();

  /* walk upwards through caller chain */
  
  if (CfgEdgeIsForwardInterproc(edge) && CFG_EDGE_CORR(edge))
    {
      /*      VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))));*/
      BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
    }
  else
    {
      /*      VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(edge)));*/
      BblMark(CFG_EDGE_TAIL(edge));
    }

  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked(bbl))
	  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(i_edge));
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_edge)));*/
	      }
	    else if (CFG_EDGE_CORR(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
	      }
	  }
    }
  
  BblMark(CFG_EDGE_TAIL(edge));
  
  change = TRUE;

  /* walk downwards through callee-chain */
    
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked(bbl))
	  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsBackwardInterproc(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		  {
		    change = TRUE;
		    BblMark(CFG_EDGE_TAIL(i_edge));
		    /*		    VERBOSE(0,("marking 2 @B\n",CFG_EDGE_TAIL(i_edge)));*/
		  }
		
		if (CfgEdgeIsForwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		  if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		    {
		      change = TRUE;
		      BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		      /*		      VERBOSE(0,("marking 2 @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
		    }
	      }
	  }
    }

  return;
}

void MarkAllSuccessorsOfEdgeHaltBbl(t_cfg * cfg, t_cfg_edge * edge, t_bbl * halt_bbl)
{
  t_bbl * bbl;
  t_cfg_edge * i_edge;
  t_bool change = TRUE;
  
  BblMarkInit();

  /* walk upwards through caller chain */
  
  if (CfgEdgeIsForwardInterproc(edge) && CFG_EDGE_CORR(edge))
    {
      /*      VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(CFG_EDGE_CORR(edge))));*/
      BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
      if (BblIsMarked(halt_bbl)) return;
    }
  else
    {
      /*      VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(edge)));*/
      BblMark(CFG_EDGE_TAIL(edge));
      if (BblIsMarked(halt_bbl)) return;
    }

  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked(bbl))
	  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(i_edge));
		if (BblIsMarked(halt_bbl)) return;
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_edge)));*/
	      }
	    else if (CFG_EDGE_CORR(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		if (BblIsMarked(halt_bbl)) return;
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
	      }
	  }
    }
  
  BblMark(CFG_EDGE_TAIL(edge));
  if (BblIsMarked(halt_bbl)) return;

  change = TRUE;

  /* walk downwards through callee-chain */
    
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked(bbl))
	  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsBackwardInterproc(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		  {
		    change = TRUE;
		    BblMark(CFG_EDGE_TAIL(i_edge));
		    if (BblIsMarked(halt_bbl)) return;
		    /*		    VERBOSE(0,("marking 2 @B\n",CFG_EDGE_TAIL(i_edge)));*/
		  }
		
		if (CfgEdgeIsForwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		  if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		    {
		      change = TRUE;
		      BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		      if (BblIsMarked(halt_bbl)) return;
		      /*		      VERBOSE(0,("marking 2 @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
		    }
	      }
	  }
    }

  return;
}

void MarkAllSuccessorsOfEdgeNotOverMarkedEdges(t_cfg * cfg, t_cfg_edge * edge)
{
  t_bbl * bbl;
  t_bool change = TRUE;
  t_cfg_edge * i_edge;
  
  BblMarkInit();
  
  if (CfgEdgeIsForwardInterproc(edge) && CFG_EDGE_CORR(edge))
    {
      if (!CfgEdgeIsMarked(CFG_EDGE_CORR(edge))) 
	BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)));
    }
  else
    BblMark(CFG_EDGE_TAIL(edge));
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked(bbl))
	  BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	  {
	    if (CfgEdgeIsMarked(i_edge)) continue;
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(i_edge));
	      }
	    else if (CFG_EDGE_CORR(i_edge) && !CfgEdgeIsMarked(CFG_EDGE_CORR(i_edge)))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
	      }
	  }
    }

  BblMark(CFG_EDGE_TAIL(edge));

  change = TRUE;
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	{
	  if (BblIsMarked(bbl))
	    BBL_FOREACH_SUCC_EDGE(bbl,i_edge)
	    {
	      if (CfgEdgeIsMarked(i_edge)) continue;
	      if (!CfgEdgeIsBackwardInterproc(i_edge))
		{
		  if (!BblIsMarked(CFG_EDGE_TAIL(i_edge)) )
		    change = TRUE;
		  BblMark(CFG_EDGE_TAIL(i_edge));
		  
		  if (CfgEdgeIsForwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		    if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		      {
			change = TRUE;
			BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		      }
		}
	    }
	}
    }

  return;
}

void MarkAllPredecessorOfEdge(t_cfg * cfg, t_cfg_edge * edge)
{
  t_cfg_edge * i_edge;
  t_bbl * bbl;
  t_bool change = TRUE;

  BblMarkInit2();
  
  if (CfgEdgeIsBackwardInterproc(edge) && CFG_EDGE_CORR(edge))
    BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));
  else
    BblMark2(CFG_EDGE_HEAD(edge));
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {	
	    if (!CfgEdgeIsBackwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  change = TRUE;
		BblMark2(CFG_EDGE_HEAD(i_edge));
	      }
	    else if (CFG_EDGE_CORR(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		/*		VERBOSE(0,("marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge))));*/
	      }
	  }
    }
  
  BblMark2(CFG_EDGE_HEAD(edge));
  
  change = TRUE;
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  {
		    change = TRUE;
		    BblMark2(CFG_EDGE_HEAD(i_edge));
		  }
		
		if (CfgEdgeIsBackwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		  {
		    /*		    VERBOSE(0,("@E\n",i_edge));fflush(stdout);fflush(stdout);*/
		    if (!BblIsMarked2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))) )
		    {
		      change = TRUE;
		      BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge)));			  
		    }
		  }
	      }
	  }
    }

  return;
}

void MarkAllPredecessorOfEdgeHatlBbl(t_cfg * cfg, t_cfg_edge * edge, t_bbl * halt_bbl)
{
  t_cfg_edge * i_edge;
  t_bbl * bbl;
  t_bool change = TRUE;

  BblMarkInit2();
  
  if (CfgEdgeIsBackwardInterproc(edge) && CFG_EDGE_CORR(edge))
    BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));
  else
    BblMark2(CFG_EDGE_HEAD(edge));
  
  if (BblIsMarked(halt_bbl)) return;

  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {	
	    if (!CfgEdgeIsBackwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  change = TRUE;
		BblMark2(CFG_EDGE_HEAD(i_edge));
		if (BblIsMarked(halt_bbl)) return;
	      }
	    else if (CFG_EDGE_CORR(i_edge))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		if (BblIsMarked(halt_bbl)) return;
		/*		DiabloPrint(stdout,"marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge)));*/
	      }
	  }
    }
  
  BblMark2(CFG_EDGE_HEAD(edge));

  if (BblIsMarked(halt_bbl)) return;

  change = TRUE;
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  {
		    change = TRUE;
		    BblMark2(CFG_EDGE_HEAD(i_edge));
		    if (BblIsMarked(halt_bbl)) return;
		  }
		
		if (CfgEdgeIsBackwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		  {
		    if (!BblIsMarked2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))) )
		    {
		      change = TRUE;
		      BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge)));
		      if (BblIsMarked(halt_bbl)) return;			  
		    }
		  }
	      }
	  }
    }

  return;
}

void MarkAllPredecessorOfEdgeNotOverMarkedEdges(t_cfg * cfg, t_cfg_edge * edge)
{
  t_cfg_edge * i_edge;
  t_bbl * bbl;
  t_bool change = TRUE;

  BblMarkInit2();

  if (CfgEdgeIsBackwardInterproc(edge) && CFG_EDGE_CORR(edge))
    {
      if (!CfgEdgeIsMarked(CFG_EDGE_CORR(edge)))
	BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)));
    }
  else
    BblMark2(CFG_EDGE_HEAD(edge));
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {
	    if (CfgEdgeIsMarked(i_edge)) continue;
	    if (!CfgEdgeIsBackwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  change = TRUE;
		BblMark2(CFG_EDGE_HEAD(i_edge));
	      }
	    else if (CFG_EDGE_CORR(i_edge) && !CfgEdgeIsMarked(CFG_EDGE_CORR(i_edge)))
	      {
		if (!BblIsMarked(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge))) )
		  change = TRUE;
		BblMark(CFG_EDGE_TAIL(CFG_EDGE_CORR(i_edge)));
		/*		DiabloPrint(stdout,"marking @B\n",CFG_EDGE_TAIL(i_CFG_EDGE_CORR(edge)));*/
	      }
	  }
    }
  
  BblMark2(CFG_EDGE_HEAD(edge));

  change = TRUE;
  
  while (change)
    {
      change = FALSE;
      
      CFG_FOREACH_BBL(cfg,bbl)
	if (BblIsMarked2(bbl))
	  BBL_FOREACH_PRED_EDGE(bbl,i_edge)
	  {
	    if (CfgEdgeIsMarked(i_edge)) continue;
	    if (!CfgEdgeIsForwardInterproc(i_edge))
	      {
		if (!BblIsMarked2(CFG_EDGE_HEAD(i_edge)) )
		  {
		    change = TRUE;
		    BblMark2(CFG_EDGE_HEAD(i_edge));
		  }
		
		if (CfgEdgeIsBackwardInterproc(i_edge) && CFG_EDGE_CORR(i_edge))
		  if (!BblIsMarked2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge))) )
		    {
		      change = TRUE;
		      BblMark2(CFG_EDGE_HEAD(CFG_EDGE_CORR(i_edge)));			  
		    }
	      }
	  }
    }

  return;
}


/* CfgMoveInsDown {{{ */

void CfgMoveInsDown(t_cfg * cfg)
{
  t_bbl * bbl;
  /*static int count=0;*/
  t_regset dead_over_call;
  const t_architecture_description * desc=CFG_DESCRIPTION(cfg);
  
  RegsetSetDup(dead_over_call,desc->dead_over_call);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_bool over_call_edge = FALSE;

    if (BBL_FUNCTION(bbl) && (FUNCTION_BBL_LAST(BBL_FUNCTION(bbl))==bbl)) continue;
    if (BBL_IS_HELL(bbl)) continue;
    if (BBL_REFED_BY(bbl)) continue;
    if (BBL_PRED_FIRST(bbl) && CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)))
    {
      /* at least 2 predecessors */

      t_cfg_edge * edge;
      t_ins * i_ins, * j_ins;
      t_bbl * pred, * first=CFG_EDGE_HEAD(BBL_PRED_FIRST(bbl));
      t_bool ok=TRUE,ins_can_move;

      BBL_FOREACH_PRED_EDGE(bbl,edge)
	{
	  pred=CFG_EDGE_HEAD(edge);
	  
	  if (CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(pred)))
	    ok = FALSE;
	  else if (CFG_EDGE_CAT(edge)==ET_CALL)
	    {
	      if (diabloanopt_options.rely_on_calling_conventions)
		ok = FALSE;
	      over_call_edge = TRUE;
	    }
	}

      if (ok)
      {
	/* all predecessors have only one edge out, or a call and a funlink.
	 * we are at the other end of the call edge (never the funlink edge) */

	do {
	  /* find a moveable instruction and move it */
	  t_bool load_seen=FALSE, store_seen=FALSE, side_effect_seen = FALSE;
	  t_regset use1=RegsetNew(),  def1=RegsetNew(), tmp=RegsetNew();
	  ok=FALSE;
	  BBL_FOREACH_INS_R(first,i_ins)
	  {
	    ins_can_move=TRUE;

	    if (desc->InsIsSystem(i_ins)) /* e.g. cli on i386 */
	      ins_can_move = FALSE;
	    if (desc->InsIsLoad(i_ins))
	      if (store_seen||side_effect_seen) ins_can_move=FALSE;
	    if (desc->InsIsStore(i_ins))
	      if (store_seen||load_seen||side_effect_seen) ins_can_move=FALSE;
            if (desc->InsHasSideEffect(i_ins))
              {
                if (store_seen||load_seen||side_effect_seen) ins_can_move=FALSE;
                side_effect_seen = TRUE;
              }
	    if (desc->InsIsLoad(i_ins))
	      load_seen=TRUE;
	    if (desc->InsIsStore(i_ins))
	      store_seen=TRUE; 
           

            DiabloBrokerCall("InsCanMoveDown", i_ins, &ins_can_move);

	    /* cannot move instructions that define registers that are used or redefined in later instructions */
	    RegsetSetDup(tmp,INS_REGS_DEF(i_ins));
	    RegsetSetIntersect(tmp,use1);
	    if (!RegsetIsEmpty(tmp)) {ins_can_move=FALSE;}
	    /* cannot move instructions that use registers that are redefined in later instructions */
	    RegsetSetDup(tmp,INS_REGS_USE(i_ins));
	    RegsetSetIntersect(tmp,def1);
	    if (!RegsetIsEmpty(tmp)) {ins_can_move=FALSE;}
	    /* cannot move instructions that use registers that are "dead over call" over call edges */
	    if (over_call_edge && diabloanopt_options.rely_on_calling_conventions)
	    {
	      RegsetSetDup(tmp,INS_REGS_USE(i_ins));
	      RegsetSetIntersect(tmp,dead_over_call);
	      if (!RegsetIsEmpty(tmp)) {ins_can_move=FALSE;}
	    }
	    RegsetSetUnion(use1,INS_REGS_USE(i_ins));
	    RegsetSetUnion(def1,INS_REGS_DEF(i_ins));
	    RegsetSetUnion(use1,INS_REGS_DEF(i_ins));

	    if (INS_TYPE(i_ins)==IT_BRANCH) continue;

	    if (!ins_can_move) continue;

	    ok=TRUE;

	    BBL_FOREACH_PRED_EDGE(bbl,edge)
	      {
		if (CFG_EDGE_HEAD(edge)==first) continue;
		j_ins=InsFindMovableDownEqualInsInBbl(i_ins, CFG_EDGE_HEAD(edge));
		if (!j_ins) { ok=FALSE; break; }
	      }

	    if (ok)
	    {


	      /*if (count++ >= diablosupport_options.debugcounter)*/
		/*ok = FALSE;*/
	      /*else*/
	      {
		VERBOSE(1,("Downmoving @I\n",i_ins));
		BBL_FOREACH_PRED_EDGE(bbl,edge)
		{
		  if (CFG_EDGE_HEAD(edge)==first) continue;
		  j_ins=InsFindMovableDownEqualInsInBbl(i_ins, CFG_EDGE_HEAD(edge));
		  VERBOSE(1,("Downmoving @I\n",j_ins));
		  ASSERT(j_ins,("this should never happen!"));
		  INS_SET_EXEC_COUNT(i_ins,   INS_EXEC_COUNT(i_ins) + INS_EXEC_COUNT(j_ins));
		  InsKill(j_ins);
		}

		{
		  t_ins * new_ins = InsDup(i_ins);
		  InsPrependToBbl(new_ins,bbl);
		  VERBOSE(1,("Prepended here...\n@iB",bbl));
		  InsKill(i_ins);

		}
		break;
	      }
	    }
	  }
	} while (ok);
      }
    }
  }
}
/*}}}*/

/* CfgMoveInsUp {{{ */
void CfgMoveInsUp(t_cfg * cfg)
{
  t_bbl * bbl;
  /*static int count=0;*/
  t_regset dead_over_call;
  const t_architecture_description * desc=CFG_DESCRIPTION(cfg);
  
  RegsetSetDup(dead_over_call,desc->dead_over_call);

  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_bool over_call_edge = FALSE;

    if (BBL_FUNCTION(bbl) && (FUNCTION_BBL_LAST(BBL_FUNCTION(bbl))==bbl)) continue;
    if (bbl==CFG_HELL_NODE(cfg)) continue;
    if (bbl==CFG_CALL_HELL_NODE(cfg)) continue;
    if (bbl==CFG_SWI_HELL_NODE(cfg)) continue;

    if (BBL_SUCC_FIRST(bbl) && CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(bbl)))
    {
      /* at least 2 successors */

      t_cfg_edge * edge;
      t_ins * i_ins, * j_ins;
      t_bbl * succ, * first=CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl));
      t_bool ok=TRUE,ins_can_move;
      t_regset branch_def, branch_use;

      branch_def = RegsetDup(INS_REGS_DEF(BBL_INS_LAST(bbl)));
      branch_use = RegsetDup(INS_REGS_USE(BBL_INS_LAST(bbl)));
      BBL_FOREACH_SUCC_EDGE(bbl,edge)
	{
	  succ=T_BBL(CFG_EDGE_TAIL(edge));
	  
	  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(succ)))
	    ok = FALSE;
	  else if (CfgEdgeIsInterproc(edge))
	    {
	      if (diabloanopt_options.rely_on_calling_conventions)
		ok = FALSE;
	      over_call_edge = TRUE;
	    }
	}

      if (ok)
      {
	/* all succcessors have only one edge in, or a return and a funlink.
	 * we are at the other end of the return edge (never the funlink edge) */

	do {
	  /* find a moveable instruction and move it */
	  t_bool load_seen=FALSE, store_seen=FALSE, side_effect_seen = FALSE;
	  t_regset use1=RegsetNew(),  def1=RegsetNew(), tmp=RegsetNew();
	  ok=FALSE;
	  BBL_FOREACH_INS(first,i_ins)
	  {
	    ins_can_move=TRUE;

	    if (desc->InsIsSystem(i_ins)) /* e.g. cli on i386 */
	      ins_can_move = FALSE;
	    if (desc->InsIsLoad(i_ins))
	      if (store_seen||side_effect_seen) ins_can_move=FALSE;
	    if (desc->InsIsStore(i_ins))
	      if (store_seen||load_seen||side_effect_seen) ins_can_move=FALSE;
            if (desc->InsHasSideEffect(i_ins))
              {
                if (store_seen||load_seen||side_effect_seen) ins_can_move=FALSE;
                side_effect_seen = TRUE;
              }
	    if (desc->InsIsLoad(i_ins))
	      load_seen=TRUE;
	    if (desc->InsIsStore(i_ins))
	      store_seen=TRUE; 
	    if (INS_IS_BRANCH(i_ins) || (INS_ATTRIB(i_ins) & IF_PCDEF) || INS_IS_SWI(i_ins) || INS_IS_UNKNOWN(i_ins)) ins_can_move = FALSE;
	    if (RegsetIsEmpty(RegsetIntersect(desc->cond_registers, INS_REGS_DEF(i_ins))))
	      {
		ins_can_move = FALSE;
	      }

            DiabloBrokerCall("InsCanMoveUp", i_ins, &ins_can_move);

	    /* cannot move instructions that define registers that are used or redefined in later instructions */
	    RegsetSetDup(tmp,INS_REGS_DEF(i_ins));
	    RegsetSetIntersect(tmp,use1);
	    if (!RegsetIsEmpty(tmp)) {ins_can_move=FALSE;}
	    /* cannot move instructions that use registers that are redefined in later instructions */
	    RegsetSetDup(tmp,INS_REGS_USE(i_ins));
	    RegsetSetIntersect(tmp,def1);
	    if (!RegsetIsEmpty(tmp)) {ins_can_move=FALSE;}

	    if (!RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(i_ins), branch_def))) ins_can_move = FALSE;
	    if (!RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(i_ins), branch_use))) ins_can_move = FALSE;
	    RegsetSetUnion(use1,INS_REGS_USE(i_ins));
	    RegsetSetUnion(def1,INS_REGS_DEF(i_ins));
	    RegsetSetUnion(use1,INS_REGS_DEF(i_ins));

	    if (INS_TYPE(i_ins)==IT_BRANCH) continue;

	    if (!ins_can_move) continue;

	    ok=TRUE;

	    BBL_FOREACH_SUCC_EDGE(bbl,edge)
	      {
		if (T_BBL(CFG_EDGE_TAIL(edge))==first) continue;
		j_ins=InsFindMovableUpEqualInsInBbl(i_ins, CFG_EDGE_TAIL(edge));
		if (!j_ins) { ok=FALSE; break; }
	      }

	    if (ok)
	    {


	      /*if (count++ >= diablosupport_options.debugcounter)*/
		/*ok = FALSE;*/
	      /*else*/
	      {
		VERBOSE(1,("Upmoving @I\n",i_ins));
		BBL_FOREACH_SUCC_EDGE(bbl,edge)
		{
		  if (T_BBL(CFG_EDGE_TAIL(edge))==first) continue;
		  j_ins=InsFindMovableUpEqualInsInBbl(i_ins, T_BBL(CFG_EDGE_TAIL(edge)));
		  VERBOSE(1,("Upmoving @I\n",j_ins));
		  ASSERT(j_ins,("this should never happen!"));
		  INS_SET_EXEC_COUNT(i_ins,   INS_EXEC_COUNT(i_ins) + INS_EXEC_COUNT(j_ins));
		  InsKill(j_ins);
		}

		{
		  t_ins * new_ins = InsDup(i_ins);
		  InsInsertBefore(new_ins, BBL_INS_LAST(bbl));
		  VERBOSE(1,("Appended here...\n@iB",bbl));
		  InsKill(i_ins);

		}
		break;
	      }
	    }
	  }
	} while (ok);
      }
    }
  }
}
/*}}}*/

/* vim: set shiftwidth=2 foldmethod=marker: */
