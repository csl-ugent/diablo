/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diablosmc.h>
#include <diabloi386.h>

#define BBL_CODEBYTE_FIRST(bbl) (BBL_INS_FIRST(bbl)?STATE_CODEBYTE(STATE_REF_STATE(STATELIST_FIRST(INS_STATELIST(BBL_INS_FIRST(bbl))))):NULL)
																	
void SmcBranchForwarding(t_cfg * cfg)/*{{{*/
{
  t_bbl * i_bbl;
  t_cfg_edge * new_edge;
  t_bbl * bbl1, *bbl2, *bbl3;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    if (BBL_NINS(i_bbl)==1 && I386_INS_OPCODE(T_I386_INS(BBL_INS_FIRST(i_bbl)))==I386_JMP)
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
	    if (CFG_EDGE_HEAD(i_edge)==CFG_EDGE_TAIL(i_edge)) { ok=FALSE; break;}
	    if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) { ok=FALSE; break;}
	    if (BBL_IS_HELL((CFG_EDGE_TAIL(i_edge)))) { ok=FALSE; break;}
	    /*if ((FunctionGetExitBlock(BBL_FUNCTION(CFG_EDGE_TAIL(i_edge))))==T_BBL(CFG_EDGE_TAIL(i_edge))) { ok=FALSE; break;}*/
	    j_edge=i_edge;
	    break;

	  default:
	    FATAL(("Edge not handled!"));

	}
	if (!ok) break;
      }
      if (ok)
      {
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
		  if (CFG_EDGE_HEAD(i_edge)==CFG_EDGE_TAIL(i_edge)) break;
		  if (BBL_IS_HELL(CFG_EDGE_HEAD(i_edge))) { break;}
		  if (BBL_IS_HELL((CFG_EDGE_TAIL(i_edge)))) { break;}

		  /* forwarding a regular branch */
		  {
		    bbl1 = CFG_EDGE_HEAD(i_edge);
		    bbl2 =  CFG_EDGE_TAIL(i_edge);
		    bbl3 = CFG_EDGE_TAIL(j_edge);

		    VERBOSE(10,("BEFORE @ieB\n -> @ieB\n ->@ieB\n",bbl1,bbl2,bbl3));

		    if (CFG_EDGE_CAT(i_edge)==ET_JUMP && CFG_EDGE_CAT(j_edge)==ET_JUMP)
		    {
		      new_edge = CfgEdgeCreate(cfg,bbl1,bbl3,ET_JUMP);
		    }
		    else
		    {
		      new_edge = CfgEdgeCreate(cfg,bbl1,bbl3,ET_IPJUMP);
		    }
		    
		    if (CFG_EDGE_CORR(i_edge))
		      CfgEdgeKill(CFG_EDGE_CORR(i_edge));
		    CfgEdgeKill(i_edge);

		    VERBOSE(10,("AFTER @ieB\n -> @ieB\n ->@ieB\n",bbl1,bbl2,bbl3));

		    found=TRUE;
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
  
}/*}}}*/

/* turn jump edges into fallthrough edges */
void SmcBranchElimination(t_cfg * cfg)/*{{{*/
{
  t_bbl * i_bbl;
  t_cfg_edge * i_edge;
  t_cfg_entry_bbl *entry;

  static int nr_forwarded_branches = 0;

  CFG_FOREACH_BBL(cfg,i_bbl)
  {
    t_bool flag_fallthrough = FALSE;
    t_bbl * j_bbl=NULL;
    t_cfg_edge * j_edge=NULL;
    t_uint32 count=0;
    
    if (BBL_IS_HELL(i_bbl)) continue;
    if (IS_DATABBL(i_bbl)) continue;

    BBL_FOREACH_SUCC_EDGE(CFG_UNIQUE_ENTRY_NODE(cfg),i_edge)
      if (i_bbl == CFG_EDGE_TAIL(i_edge)) continue;
    
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

    BBL_FOREACH_PRED_EDGE(i_bbl,i_edge)
    {
      t_cfg_edge * j_edge;
      BBL_FOREACH_SUCC_EDGE(CFG_EDGE_HEAD(i_edge),j_edge)
      {
	if (CFG_EDGE_CAT(j_edge)==ET_SWITCH)
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
	  CFG_EDGE_CAT(i_edge)==ET_SWITCH)
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

    {
      t_bool do_elim = TRUE;
      DiabloBrokerCall ("?BranchEliminationDo",
	  i_bbl, CFG_EDGE_TAIL (j_edge), &do_elim);
      if (!do_elim)
	continue;
    }

    /*if (nr_forwarded_branches >= diablosupport_options.debugcounter)*/
      /*continue;*/
    /*VERBOSE (0, ("Forwarding @I", BBL_INS_LAST (i_bbl)));*/

    InsKill(BBL_INS_LAST(i_bbl));

    nr_forwarded_branches++;

    if (CFG_EDGE_CAT(j_edge)==ET_JUMP)
      CFG_EDGE_SET_CAT(j_edge, ET_FALLTHROUGH);
    else if(CFG_EDGE_CAT(j_edge)==ET_IPJUMP)
      CFG_EDGE_SET_CAT(j_edge, ET_IPFALLTHRU);
    else
      FATAL(("Oeps!"));
  }     
  printf("Branch forwarding : %d times!\n",nr_forwarded_branches);
}/*}}}*/

static t_bool SetPrevAndNextInChainField(t_bbl * bbl1, t_bbl * bbl2) /*beware of common codebytes -> only one next and prev field, can't fill them in twice{{{*/
{
  t_codebyte * codebyte, * codebyte_prev;
  t_ins * ins;
  t_state * state;
  t_bbl * bbl_iter;
  
  CHAIN_FOREACH_BBL(bbl1,bbl_iter)
    BBL_FOREACH_INS(bbl_iter,ins)
    {
      t_state_ref * state_ref;
      INS_FOREACH_STATE(ins,state,state_ref) 
      {
	codebyte = STATE_CODEBYTE(state);
	CODEBYTE_SET_NEXT_IN_CHAIN(codebyte,NULL);
	CODEBYTE_SET_PREV_IN_CHAIN(codebyte,NULL);
      }
    }
  
  CHAIN_FOREACH_BBL(bbl2,bbl_iter)
    BBL_FOREACH_INS(bbl_iter,ins)
    {
      t_state_ref * state_ref;
      INS_FOREACH_STATE(ins,state,state_ref) 
      {
	codebyte = STATE_CODEBYTE(state);
	CODEBYTE_SET_NEXT_IN_CHAIN(codebyte,NULL);
	CODEBYTE_SET_PREV_IN_CHAIN(codebyte,NULL);
      }
    }
  
  codebyte_prev = NULL;
  CHAIN_FOREACH_BBL(bbl1,bbl_iter)
    BBL_FOREACH_INS(bbl_iter,ins)
    {
      t_state_ref * state_ref;
      INS_FOREACH_STATE(ins,state,state_ref) 
      {
	codebyte = STATE_CODEBYTE(state);
	if(codebyte_prev)
	{
	  if(CODEBYTE_NEXT_IN_CHAIN(codebyte_prev))
	    return FALSE;
	  CODEBYTE_SET_NEXT_IN_CHAIN(codebyte_prev,codebyte);
	}
	if(CODEBYTE_PREV_IN_CHAIN(codebyte))
	  return FALSE;
	CODEBYTE_SET_PREV_IN_CHAIN(codebyte,codebyte_prev);
	codebyte_prev = codebyte;
      }
    }
  
  codebyte_prev = NULL;
  CHAIN_FOREACH_BBL(bbl2,bbl_iter)
    BBL_FOREACH_INS(bbl_iter,ins)
    {
      t_state_ref * state_ref;
      INS_FOREACH_STATE(ins,state,state_ref) 
      {
	codebyte = STATE_CODEBYTE(state);
	if(codebyte_prev)
	{
	  if(CODEBYTE_NEXT_IN_CHAIN(codebyte_prev))
	    return FALSE;
	  CODEBYTE_SET_NEXT_IN_CHAIN(codebyte_prev,codebyte);
	}
	if(CODEBYTE_PREV_IN_CHAIN(codebyte))
	  return FALSE;
	CODEBYTE_SET_PREV_IN_CHAIN(codebyte,codebyte_prev);
	codebyte_prev = codebyte;
      }
    }
  
  return TRUE;
}
/*}}}*/

static t_bool ValueOfCodebyteIsConstant(t_codebyte * codebyte)/*{{{*/
{
  t_uint8 value;
  t_state * state;
  t_state_ref * state_ref;
  if(STATELIST_COUNT(CODEBYTE_STATELIST(codebyte)) == 1 )
    return TRUE;
  value = STATE_VALUE(CODEBYTE_STATE_FIRST(codebyte));
  CODEBYTE_FOREACH_STATE(codebyte,state,state_ref)
  {
    if(!STATE_KNOWN(state))
      return FALSE;
    if(STATE_VALUE(state)!=value)
      return FALSE;
  }
  return TRUE;
}/*}}}*/

static t_bool CodebyteEqual(t_codebyte * codebyte1, t_codebyte * codebyte2, t_bool offest_means_different)/*{{{*/
{
  /*if they can be in multiple states, we conservatively assume they are different*/
  if(!ValueOfCodebyteIsConstant(codebyte1) || !ValueOfCodebyteIsConstant(codebyte1))
    return FALSE;

  /*If we do not know, we conservatively assume they are different*/
  if(!STATE_KNOWN(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte1))))||!STATE_KNOWN(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte2))))) 
  {
    /*check to see if they are not identical control transfers*/
    t_state * state1 = STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte1)));
    t_state * state2 = STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte2)));
    t_ins * ins1 = STATE_PARENT_INS(state1);
    t_ins * ins2 = STATE_PARENT_INS(state2);

    if(offest_means_different)
      return FALSE;

    if(InsIsControlTransferInstructionWithImmediateTarget(ins1) && InsIsControlTransferInstructionWithImmediateTarget(ins2))
      if(I386_INS_OPCODE(T_I386_INS(ins1))==I386_INS_OPCODE(T_I386_INS(ins2)) && I386_INS_CONDITION(T_I386_INS(ins1))==I386_INS_CONDITION(T_I386_INS(ins2)))
	if(STATE_PARENT_OFFSET(state1)==STATE_PARENT_OFFSET(state2))
	  if(ControlTransferGetTarget(ins1)==ControlTransferGetTarget(ins2))
	  {
	    return TRUE;
	  }
      
    return FALSE;
  }

  return STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte1)))) == STATE_VALUE(STATE_REF_STATE(STATELIST_FIRST(CODEBYTE_STATELIST(codebyte2))));
}
/*}}}*/

static t_bool ChainDistance0(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2)/*{{{ Returns TRUE if identical, otherwise FALSE*/
{
  while(codebyte_chain1 && codebyte_chain2)
  {
    if(!CodebyteEqual(codebyte_chain1,codebyte_chain2,FALSE))
      return FALSE;

    codebyte_chain1 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2);
  }
  
  /*different size*/
  //if (codebyte_chain1 || codebyte_chain2)
  //  FATAL(("")); 

  return TRUE;
}
/*}}}*/

static void PrintCodebyte(t_codebyte * codebyte)/*{{{*/
{
  t_state * state;
  t_state_ref * state_ref;
  VERBOSE(0,("codebyte at old: @G, new: @G",CODEBYTE_OLD_ADDRESS(codebyte),CODEBYTE_CADDRESS(codebyte)));
  
  CODEBYTE_FOREACH_STATE(codebyte, state, state_ref)
  {
    VERBOSE(0,("\t state: %d %x (@I + %d)", STATE_KNOWN(state),STATE_VALUE(state),STATE_PARENT_INS(state),STATE_PARENT_OFFSET(state)));
  }
}/*}}}*/
  
static void PrintCodebyteChain(t_codebyte * codebyte)/*{{{*/
{
  while(codebyte)
  {
    PrintCodebyte(codebyte);
    codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte);
  }
}/*}}}*/

t_codebyte * SmcChainDistance1(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2, t_uint32 * valuej)/*{{{ Returns -1 if coalescing is possible with one byte modifier */
{
  t_codebyte * codebyte_to_write = NULL;
  t_int32 counter = 0;

  while(codebyte_chain1 && codebyte_chain2)
  {
    if(!CodebyteEqual(codebyte_chain1,codebyte_chain2,FALSE))
    {
      if(codebyte_to_write)
	return NULL;
      
      codebyte_to_write = codebyte_chain1;
      *valuej = STATE_VALUE(CODEBYTE_STATE_FIRST(codebyte_chain2));;
      
      if(!STATE_KNOWN(CODEBYTE_STATE_FIRST(codebyte_chain1))||!STATE_KNOWN(CODEBYTE_STATE_FIRST(codebyte_chain2)))
	return NULL;
    }

    codebyte_chain1 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2);
    counter++;
  }

  if(counter < 5)
    return NULL;

  /*for now forbid longer second chain*/
  if(codebyte_chain1 || codebyte_chain2)
    return NULL;
  
  return codebyte_to_write;  
}
/*}}}*/

static t_bool HasCorrectFormFor8(t_codebyte * codebyte_chain1)/*{{{*/
{
  int i;
  /*goto end of chain*/
  while(CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1))
    codebyte_chain1 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1);

  for(i=0;i<4;i++)
  {
    t_ins * ins1;

    if(!codebyte_chain1)
    {
      if(i==0)
	FATAL((""));
      return FALSE;
    }
    
    ins1 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1));
    if(!InsIsControlTransferInstructionWithImmediateTarget(ins1))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
  }
  
  codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
  
  for(i=0;i<4;i++)
  {
    t_ins * ins1;

    if(!codebyte_chain1)
    {
      if(i==0)
	FATAL((""));
      return FALSE;
    }
    
    ins1 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1));
    if(!InsIsControlTransferInstructionWithImmediateTarget(ins1))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
  }

  return TRUE;
}/*}}}*/

static t_bool ChainDistance8(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2)/*{{{ We are specifically looking for code snippets which are identical appart from two target bbls */
{
  int i;
  
  /*goto end of chain*/
  while(CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1)&&CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2))
  {
    codebyte_chain1 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2);
  }
  /*need to be of same size*/
  if(CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1)||CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2))
    return FALSE;
  
  for(i=0;i<4;i++)
  {
    t_ins * ins1;
    t_ins * ins2;

    if(!codebyte_chain1 || !codebyte_chain2)
      return FALSE;
    
    ins1 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1));
    ins2 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain2));
    if(!InsIsControlTransferInstructionWithImmediateTarget(ins1) || !InsIsControlTransferInstructionWithImmediateTarget(ins2))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  }
  
  if(!codebyte_chain1 || ! codebyte_chain2)
    return FALSE;

  if(!CodebyteEqual(codebyte_chain1,codebyte_chain2,FALSE))
    return FALSE;
  
  codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
  codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  
  for(i=0;i<4;i++)
  {
    t_ins * ins1;
    t_ins * ins2;

    if(!codebyte_chain1 || !codebyte_chain2)
      return FALSE;
    
    ins1 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1));
    ins2 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain2));
    if(!InsIsControlTransferInstructionWithImmediateTarget(ins1) || !InsIsControlTransferInstructionWithImmediateTarget(ins2))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  }
  
  while(codebyte_chain1 && codebyte_chain2)
  {
    if(!CodebyteEqual(codebyte_chain1,codebyte_chain2,FALSE))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  }
  
  return TRUE;  
}

/*}}}*/

t_bool SmcChainDistance4(t_codebyte * codebyte_chain1, t_codebyte * codebyte_chain2)/*{{{ We are specifically looking for code snippets which are identical appart from one target bbl */
{
  int i;
  
  /*goto end of chain*/
  while(CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1)&&CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2))
  {
    codebyte_chain1 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2);
  }
  /*need to be of same size*/
  if(CODEBYTE_NEXT_IN_CHAIN(codebyte_chain1)||CODEBYTE_NEXT_IN_CHAIN(codebyte_chain2))
    return FALSE;
  
  if(!InsIsControlTransferInstructionWithImmediateTarget/*I386InsIsUnconditionalBranch(T_I386_INS*/(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1))))
    return FALSE;

  for(i=0;i<4;i++)
  {
    t_ins * ins1;
    t_ins * ins2;

    if(!codebyte_chain1 || !codebyte_chain2)
      return FALSE;
    
    ins1 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain1));
    ins2 = STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte_chain2));
    if(!InsIsControlTransferInstructionWithImmediateTarget(ins1) || !InsIsControlTransferInstructionWithImmediateTarget(ins2))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  }
  
  while(codebyte_chain1 && codebyte_chain2)
  {
    if(!CodebyteEqual(codebyte_chain1,codebyte_chain2,TRUE))
      return FALSE;

    codebyte_chain1 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain1);
    codebyte_chain2 = CODEBYTE_PREV_IN_CHAIN(codebyte_chain2);
  }

  return TRUE;  
}
/*}}}*/

#define ET_MUSTCHAIN  	ET_FALLTHROUGH | ET_IPFALLTHRU
#define ET_MUSTCHAINMAYBE  	ET_CALL | ET_SWI

static void AdaptForMinimalChains(t_cfg * cfg) /*{{{*/
{
  t_bbl * bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    t_cfg_edge * edge;
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if (CfgEdgeTestCategoryOr(edge,ET_MUSTCHAIN))
      {
	break;
      }
      if (CfgEdgeTestCategoryOr(edge,ET_MUSTCHAINMAYBE) && CFG_EDGE_CORR(edge))
      {
	edge = CFG_EDGE_CORR(edge);
	break;
      }
    }
    if(edge && !BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
    {
      t_bbl * new_bbl = BblNew(cfg);
      t_ins * jump_ins;
      t_cfg_edge * new_edge;

      CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
      
      new_edge = CfgEdgeNew(cfg,CFG_EDGE_HEAD(edge),new_bbl,CFG_EDGE_CAT(edge));
      /* Attention: order of removing and adding edge is important; Kill changes corr too! */ 
      CfgEdgeKill(edge);
      if (CFG_EDGE_CORR(edge))
      {
	CFG_EDGE_SET_CORR(new_edge,CFG_EDGE_CORR(edge));
	CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge),new_edge);
      }
      

      jump_ins = InsNewForBbl(new_bbl);
      I386InstructionMakeJump(T_I386_INS(jump_ins));
      InsAppendToBbl(jump_ins,new_bbl);
    }
  }
}
/*}}}*/

t_bool BblHasCtPath(t_bbl * bbl)/*{{{*/
{
  t_cfg_edge * edge;
  BBL_FOREACH_SUCC_EDGE(bbl,edge) 
  {
    if(!CfgEdgeTestCategoryOr(edge,ET_MUSTCHAIN))
      return TRUE;
  }
  return FALSE;
}/*}}}*/

t_bool BblHasFtPath(t_bbl * bbl)/*{{{*/
{
  t_cfg_edge * edge;
  BBL_FOREACH_SUCC_EDGE(bbl,edge) 
  {
    if(CfgEdgeTestCategoryOr(edge,ET_MUSTCHAIN))
      return TRUE;
  }
  return FALSE;
}/*}}}*/

#undef ET_MUSTCHAIN
#undef ET_MUSTCHAINMAYBE

void OverlapCodebytes(t_bbl * middle1,t_bbl * middle2)/*{{{*/
{
  t_codebyte * codebyte = INS_CODEBYTE_FIRST(BBL_INS_FIRST(middle1));
  t_ins * ins;
  t_state_ref * state_ref_ins;
  t_state * state_ins;

  BBL_FOREACH_INS(middle2,ins)
  {
    INS_FOREACH_STATE(ins,state_ins,state_ref_ins)
    {
      {
	StateRemoveFromStatelist(state_ins,CODEBYTE_STATELIST(STATE_CODEBYTE(state_ins)));
	if(StatelistIsEmpty(CODEBYTE_STATELIST(STATE_CODEBYTE(state_ins))))
	{
	  StatelistKill(CODEBYTE_STATELIST(STATE_CODEBYTE(state_ins)));
	  CodebyteUnlinkFromCfg(STATE_CODEBYTE(state_ins));
	  CodebyteFree(STATE_CODEBYTE(state_ins));
	}
	STATE_SET_CODEBYTE(STATE_REF_STATE(state_ref_ins),codebyte);
	StateAddToStatelist(STATE_REF_STATE(state_ref_ins),CODEBYTE_STATELIST(codebyte));
      }
      codebyte = CODEBYTE_NEXT_IN_CHAIN(codebyte);
    }
  }
}/*}}}*/

void RegisterBblFactoring(t_bbl * orig_bbl, t_bbl * new_bbl);
static t_bbl * SmcMergeBbls(t_bbl * bbl1, t_bbl * bbl2)/*{{{*/
{
  t_ins * ins1, * ins2;
  t_bbl * middle1, * middle2;
  t_bbl * tail1, * tail2;
  
  /*Problem if only one of the snippets consists of two bbls, solution, make sure they both consist of two bbls*/
  if(BblHasFtPath(bbl1)!=BblHasFtPath(bbl2))
  {
    if(BblHasFtPath(bbl1))
      BblSplitBlock(bbl2,BBL_INS_LAST(bbl2),TRUE);
    else 
      BblSplitBlock(bbl1,BBL_INS_LAST(bbl1),TRUE);
  }
  VERBOSE(1,("@ieB @ieB",bbl1,bbl2));
  ins1=BBL_INS_FIRST(bbl1);
  ins2=BBL_INS_FIRST(bbl2);

  while(ins1 && ins2)
  {
    t_state_ref * state1_ref = INS_STATE_REF_FIRST(ins1);
    t_state_ref * state2_ref = INS_STATE_REF_FIRST(ins2);
    
    while(state1_ref && state2_ref)
    {
     if(!CodebyteEqual(STATE_CODEBYTE(STATE_REF_STATE(state1_ref)),STATE_CODEBYTE(STATE_REF_STATE(state2_ref)),FALSE))
      break; 
     state1_ref = STATE_REF_NEXT(state1_ref);
     state2_ref = STATE_REF_NEXT(state2_ref);
    }
    if(state1_ref || state2_ref)
      break;
    

    ins1 = INS_INEXT(ins1);
    ins2 = INS_INEXT(ins2);
  }
  
  if(ins1==NULL || ins2==NULL)
    FATAL((""));
  
  middle1 = BblSplitBlock(bbl1,ins1,TRUE);
  middle2 = BblSplitBlock(bbl2,ins2,TRUE);

  RegisterBblFactoring(bbl2,bbl1);
  
  ins1=BBL_INS_LAST(middle1);
  ins2=BBL_INS_LAST(middle2);

  while(ins1 && ins2)
  {
    t_state_ref * state1_ref = INS_STATE_REF_FIRST(ins1);
    t_state_ref * state2_ref = INS_STATE_REF_FIRST(ins2);
    
    while(state1_ref && state2_ref)
    {
     if(!CodebyteEqual(STATE_CODEBYTE(STATE_REF_STATE(state1_ref)),STATE_CODEBYTE(STATE_REF_STATE(state2_ref)),FALSE))
      break; 
     state1_ref = STATE_REF_NEXT(state1_ref);
     state2_ref = STATE_REF_NEXT(state2_ref);
    }
    if(state1_ref || state2_ref)
      break;
    ins1 = INS_IPREV(ins1);
    ins2 = INS_IPREV(ins2);
  }
  
  if(ins1==NULL||ins2==NULL)
    FATAL((""));
  
  VERBOSE(1,("ToSplit: @iB at @I",middle1,ins1));
  tail1 = BblSplitBlockNoTestOnBranches(middle1,ins1,FALSE);
  VERBOSE(1,("ToSplit: @iB at @I",middle2,ins2));
  tail2 = BblSplitBlockNoTestOnBranches(middle2,ins2,FALSE);

  RegisterBblFactoring(tail2,tail1);
  
  while(BBL_SUCC_FIRST(bbl2))
    CfgEdgeKill(BBL_SUCC_FIRST(bbl2));
  while(BBL_PRED_FIRST(bbl2))
    CfgEdgeKill(BBL_PRED_FIRST(bbl2));
  while(BBL_INS_FIRST(bbl2))
    InsKill(BBL_INS_FIRST(bbl2));
  BblKill(bbl2);
  
  while(BBL_SUCC_FIRST(middle2))
    CfgEdgeKill(BBL_SUCC_FIRST(middle2));
  while(BBL_PRED_FIRST(middle2))
    CfgEdgeKill(BBL_PRED_FIRST(middle2));
  
  while(BBL_SUCC_FIRST(tail2))
    CfgEdgeKill(BBL_SUCC_FIRST(tail2));
  while(BBL_PRED_FIRST(tail2))
    CfgEdgeKill(BBL_PRED_FIRST(tail2));
  while(BBL_INS_FIRST(tail2))
    InsKill(BBL_INS_FIRST(tail2));
  BblKill(tail2);

  CfgEdgeNew(BBL_CFG(bbl1),bbl1,middle2,ET_FALLTHROUGH);
  CfgEdgeNew(BBL_CFG(bbl1),middle2,tail1,ET_FALLTHROUGH);

  OverlapCodebytes(middle1,middle2);
  
  return bbl1;
}/*}}}*/

static t_bbl * SmcMergeBbls2(t_bbl * bbl1, t_bbl * bbl2)/*{{{*/
{
  t_ins * ins1, * ins2;
  t_bbl * middle1 = T_BBL(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl1)));
  t_bbl * middle2;
  t_bbl * tail1 = T_BBL(CFG_EDGE_TAIL(BBL_SUCC_FIRST(middle1)));
  t_bbl * tail2;
  
  ins1=BBL_INS_FIRST(bbl1);
  ins2=BBL_INS_FIRST(bbl2);

  while(ins1 && ins2)
  {
    t_state_ref * state1_ref = INS_STATE_REF_FIRST(ins1);
    t_state_ref * state2_ref = INS_STATE_REF_FIRST(ins2);
    
    while(state1_ref && state2_ref)
    {
     if(!CodebyteEqual(STATE_CODEBYTE(STATE_REF_STATE(state1_ref)),STATE_CODEBYTE(STATE_REF_STATE(state2_ref)),FALSE))
      break; 
     state1_ref = STATE_REF_NEXT(state1_ref);
     state2_ref = STATE_REF_NEXT(state2_ref);
    }
    if(state1_ref || state2_ref)
      break;
    

    ins1 = INS_INEXT(ins1);
    ins2 = INS_INEXT(ins2);
  }
  if(ins1)
    FATAL((""));
  
  middle2 = BblSplitBlock(bbl2,ins2,TRUE);

  RegisterBblFactoring(bbl2,bbl1);
  
  ins1=BBL_INS_LAST(tail1);
  ins2=BBL_INS_LAST(middle2);

  while(ins1 && ins2)
  {
    t_state_ref * state1_ref = INS_STATE_REF_FIRST(ins1);
    t_state_ref * state2_ref = INS_STATE_REF_FIRST(ins2);
    
    while(state1_ref && state2_ref)
    {
     if(!CodebyteEqual(STATE_CODEBYTE(STATE_REF_STATE(state1_ref)),STATE_CODEBYTE(STATE_REF_STATE(state2_ref)),FALSE))
      break; 
     state1_ref = STATE_REF_NEXT(state1_ref);
     state2_ref = STATE_REF_NEXT(state2_ref);
    }
    if(state1_ref || state2_ref)
      break;
    ins1 = INS_IPREV(ins1);
    ins2 = INS_IPREV(ins2);
  }
  
  if(ins1!=NULL)
    FATAL((""));

  tail2 = BblSplitBlockNoTestOnBranches(middle2,ins2,FALSE);

  RegisterBblFactoring(tail2,tail1);
  
  while(BBL_SUCC_FIRST(bbl2))
    CfgEdgeKill(BBL_SUCC_FIRST(bbl2));
  while(BBL_PRED_FIRST(bbl2))
    CfgEdgeKill(BBL_PRED_FIRST(bbl2));
  while(BBL_INS_FIRST(bbl2))
    InsKill(BBL_INS_FIRST(bbl2));
  BblKill(bbl2);
  
  while(BBL_SUCC_FIRST(middle2))
    CfgEdgeKill(BBL_SUCC_FIRST(middle2));
  while(BBL_PRED_FIRST(middle2))
    CfgEdgeKill(BBL_PRED_FIRST(middle2));
  
  while(BBL_SUCC_FIRST(tail2))
    CfgEdgeKill(BBL_SUCC_FIRST(tail2));
  while(BBL_PRED_FIRST(tail2))
    CfgEdgeKill(BBL_PRED_FIRST(tail2));
  while(BBL_INS_FIRST(tail2))
    InsKill(BBL_INS_FIRST(tail2));
  BblKill(tail2);

  CfgEdgeNew(BBL_CFG(bbl1),bbl1,middle2,ET_FALLTHROUGH);
  CfgEdgeNew(BBL_CFG(bbl1),middle2,tail1,ET_FALLTHROUGH);

  OverlapCodebytes(middle1,middle2);
  
  return bbl1;
}/*}}}*/

static t_bool BblSuitableForRetSubstitution(t_bbl * bbl)/*{{{*/
{
  if(BBL_NINS(bbl)<2)
    return FALSE;
  if(!I386InsIsUnconditionalBranch(T_I386_INS(BBL_INS_LAST(bbl))))
    return FALSE;
  if(RegsetIn(BblRegsDef(bbl),I386_REG_ESP))
    return FALSE;
  if(RegsetIn(BblRegsUse(bbl),I386_REG_ESP))
    return FALSE;
  return TRUE;
}/*}}}*/

static t_codebyte * GetCodebyte(t_int32 i, t_codebyte * leader)/*{{{*/
{
  if(i>0)
    FATAL((""));
  while(CODEBYTE_NEXT_IN_CHAIN(leader))
    leader = CODEBYTE_NEXT_IN_CHAIN(leader);
  while(i++<0)
  {
    leader = CODEBYTE_PREV_IN_CHAIN(leader);
  }
  return leader;
}/*}}}*/

void SmcFactorWithFourByteModifier(t_bool i_first, t_uint32 i, t_uint32 j, t_codebyte ** leaders, t_cfg * cfg)/*{{{*/
{
  t_bbl * bbl1_head;
  t_bbl * bbl1_tail;
  t_bbl * bbl2_head;
  t_bbl * bbl2_tail;
  t_ins * ins_to_write;

  bbl1_head = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i])));
  if(!BblHasFtPath(bbl1_head))
    BblSplitBlock(bbl1_head,BBL_INS_LAST(bbl1_head),TRUE);
  bbl2_head = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])));
  if(!BblHasFtPath(bbl2_head))
    BblSplitBlock(bbl2_head,BBL_INS_LAST(bbl2_head),TRUE);
  
  bbl1_tail = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl1_head));//INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(GetCodebyte(-1,leaders[i]))));
  bbl2_tail = CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl2_head));//INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(GetCodebyte(-1,leaders[j]))));

  ins_to_write = BBL_INS_LAST(bbl1_tail);

  /*first time i is adapted*/
  if(i_first)
  {
    t_i386_ins * jump_ins;
    t_i386_ins * modifier;
    t_bbl * target_bbl;
    t_bbl * tail;

    {
      t_cfg_edge * edge = BblGetSuccEdgeOfType(bbl1_tail,ET_JUMP);
      if(!edge)
	edge = BblGetSuccEdgeOfType(bbl1_tail,ET_CALL);
      if(!edge)
	edge = BblGetSuccEdgeOfType(bbl1_tail,ET_IPJUMP);
      if(!edge)
	FATAL((""));
      target_bbl = CFG_EDGE_TAIL(edge);
    }

    jump_ins = I386InsNewForBbl(bbl1_head);
    I386InstructionMakeJump(jump_ins);
    I386InsPrependToBbl(jump_ins,bbl1_head);

    modifier = I386InsNewForBbl(bbl1_head);
    //I386_INS_SET_IS_MODIFIER(modifier,TRUE);
    I386InstructionMakeMovToMem(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0);
    I386InsPrependToBbl(modifier, bbl1_head);

    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(ins_to_write),1,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x6,T_RELOCATABLE(ins_to_write),5,FALSE,NULL,NULL,T_RELOCATABLE(target_bbl),"R01R00-" "\\" WRITE_32);
    I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(modifier))) =I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(modifier))) =I386_OPFLAG_ISRELOCATED;

    tail = BblSplitBlock(bbl1_head,T_INS(jump_ins),FALSE);

    CfgEdgeKill(BBL_SUCC_FIRST(bbl1_head));
    CfgEdgeNew(cfg,bbl1_head,tail,ET_JUMP);

    /*prevent jump forwarding*/
    {
      t_bbl * prevent_forwarding = INS_BBL(ins_to_write);
      t_cfg_edge * edge = BBL_SUCC_FIRST(prevent_forwarding);
      if(EdgeTestCategoryOr(T_EDGE(edge),ET_JUMP | ET_IPJUMP))
      {
	CfgEdgeNew(cfg,prevent_forwarding,prevent_forwarding,EDGE_CAT(T_EDGE(edge)));
	CfgEdgeKill(edge);
      }
    }

    i_first = FALSE;
    bbl1_head = tail;
  }

  {
    t_i386_ins * jump_ins;
    t_i386_ins * modifier;
    t_bbl * target_bbl;
    t_bbl * tail;

    {
      t_cfg_edge * edge = BblGetSuccEdgeOfType(bbl2_tail,ET_JUMP);
      if(!edge)
	edge = BblGetSuccEdgeOfType(bbl2_tail,ET_CALL);
      if(!edge)
	edge = BblGetSuccEdgeOfType(bbl2_tail,ET_IPJUMP);
      if(!edge)
	FATAL(("@ieB @ieB",bbl2_head,bbl2_tail));
      target_bbl = CFG_EDGE_TAIL(edge);
    }

    jump_ins = I386InsNewForBbl(bbl2_head);
    I386InstructionMakeJump(jump_ins);
    I386InsPrependToBbl(jump_ins,bbl2_head);

    modifier = I386InsNewForBbl(bbl2_head);
    //I386_INS_SET_IS_MODIFIER(modifier,TRUE);
    I386InstructionMakeMovToMem(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0);
    I386InsPrependToBbl(modifier, bbl2_head);
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(ins_to_write),1,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x6,T_RELOCATABLE(ins_to_write),5,FALSE,NULL,NULL,T_RELOCATABLE(target_bbl),"R01R00-" "\\" WRITE_32);
    I386_OP_FLAGS(I386_INS_DEST(modifier)) = I386_OPFLAG_ISRELOCATED;
    I386_OP_FLAGS(I386_INS_SOURCE1(modifier)) = I386_OPFLAG_ISRELOCATED;

    tail = BblSplitBlock(bbl2_head,T_INS(jump_ins),FALSE);

    CfgEdgeKill(BBL_SUCC_FIRST(bbl2_head));

    CfgEdgeNew(cfg,bbl2_head,bbl1_head,ET_JUMP);

    RegisterBblFactoring(tail,bbl1_head);

    while(BBL_INS_FIRST(tail))
      InsKill(BBL_INS_FIRST(tail));
    while(BBL_SUCC_FIRST(tail))
      CfgEdgeKill(BBL_SUCC_FIRST(tail));
    while(BBL_PRED_FIRST(tail))
      CfgEdgeKill(BBL_PRED_FIRST(tail));

    CfgEdgeNew(BBL_CFG(bbl1_head),bbl1_head,bbl2_tail,ET_FALLTHROUGH);
    
    OverlapCodebytes(bbl1_tail,bbl2_tail);
    leaders[j]=NULL;
    CODEBYTE_SET_OVERLAP(leaders[i],CODEBYTE_OVERLAP(leaders[i])+1);
  }
  /*decided ok*/
}/*}}}*/

void ModifierRound4(t_cfg * cfg, t_codebyte ** leaders, t_uint32 leader_count)/*{{{*/
{
  t_uint32 i,j;
  
  VERBOSE(0,("START MODIFIERROUND4"));

  SmcBranchForwarding(cfg);

  for(i=0;i<leader_count;i++)
  {
    t_bool i_first = TRUE;
    if(leaders[i]==NULL)
      continue;
    if(CODEBYTE_SCREWED(leaders[i]))
      continue;
    
    for(j=i+1;j<leader_count;j++)
    {
      if(leaders[j]==NULL)
	continue;
      if(CODEBYTE_SCREWED(leaders[j]))
	continue;
      
      if(SmcChainDistance4(leaders[i],leaders[j]))
      {
	VERBOSE(0,("%d %d",i,j));
	SmcFactorWithFourByteModifier(i_first, i, j, leaders, cfg);
	i_first = FALSE;
      }
    }
  }
  
  SmcBranchForwarding(cfg);
  VERBOSE(0,("STOP MODIFIERROUND4"));
}
/*}}}*/

static void FactorRound4(t_cfg * cfg, t_codebyte ** leaders, t_uint32 leader_count)/*{{{*/
{
  t_uint32 i,j;

  VERBOSE(0,("START FACTORROUND4"));
  for(i=0;i<leader_count;i++)
  {
    t_bool i_first = TRUE;
    t_bbl * bbl1;
    if(leaders[i]==NULL)
      continue;
    if(!BblSuitableForRetSubstitution(INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i])))))
      continue;
    
    bbl1 = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i])));
    
    for(j=i+1;j<leader_count;j++)
    {
      if(leaders[j]==NULL)
	continue;
      if(!BblSuitableForRetSubstitution(INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])))))
      continue;
      
      if(SmcChainDistance4(leaders[i],leaders[j]))
      {
	t_bbl * bbl2 = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])));

	if(i_first)
	{
	  t_i386_ins * jump_ins;
	  t_i386_ins * push_ins;
	  t_bbl * target_bbl;
	  t_bbl * tail;
	  
	  {
	    t_cfg_edge * edge = BblGetSuccEdgeOfType(bbl1,ET_JUMP);
	    if(!edge)
	      edge = BblGetSuccEdgeOfType(bbl1,ET_IPJUMP);
	    if(!edge)
	      FATAL((""));
	    target_bbl = CFG_EDGE_TAIL(edge);
	  }
	  
	  jump_ins = I386InsNewForBbl(bbl1);
	  I386InstructionMakeJump(jump_ins);
	  I386InsPrependToBbl(jump_ins,bbl1);

	  push_ins = I386InsNewForBbl(bbl1);
	  I386InstructionMakePush(push_ins,I386_REG_NONE,0);
	  I386InsPrependToBbl(push_ins, bbl1);
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(push_ins),0x1,T_RELOCATABLE(target_bbl),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	  I386_OP_FLAGS(I386_INS_SOURCE1(push_ins)) =I386_OPFLAG_ISRELOCATED;

	  tail = BblSplitBlock(bbl1,T_INS(jump_ins),FALSE);

	  CfgEdgeKill(BBL_SUCC_FIRST(tail));

	  CfgEdgeNew(cfg,bbl1,tail,ET_JUMP);
	  
	  i_first = FALSE;

	  bbl1 = tail;
	}

	{
	  t_i386_ins * jump_ins;
	  t_i386_ins * push_ins;
	  t_bbl * target_bbl;
	  t_bbl * tail;
	  
	  {
	    t_cfg_edge * edge = BblGetSuccEdgeOfType(bbl2,ET_JUMP);
	    if(!edge)
	      edge = BblGetSuccEdgeOfType(bbl2,ET_IPJUMP);
	    if(!edge)
	      FATAL((""));
	    target_bbl = CFG_EDGE_TAIL(edge);
	  }
	  
	  jump_ins = I386InsNewForBbl(bbl2);
	  I386InstructionMakeJump(jump_ins);
	  I386InsPrependToBbl(jump_ins,bbl2);

	  push_ins = I386InsNewForBbl(bbl2);
	  I386InstructionMakePush(push_ins,I386_REG_NONE,0);
	  I386InsPrependToBbl(push_ins, bbl2);
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(push_ins),0x1,T_RELOCATABLE(target_bbl),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	  I386_OP_FLAGS(I386_INS_SOURCE1(push_ins)) =I386_OPFLAG_ISRELOCATED;

	  tail = BblSplitBlock(bbl2,T_INS(jump_ins),FALSE);

	  CfgEdgeKill(BBL_SUCC_FIRST(bbl2));

	  CfgEdgeNew(cfg,bbl2,bbl1,ET_JUMP);
	  
	  while(BBL_INS_FIRST(tail))
	    InsKill(BBL_INS_FIRST(tail));
	  while(BBL_SUCC_FIRST(tail))
	    CfgEdgeKill(BBL_SUCC_FIRST(tail));
	  while(BBL_PRED_FIRST(tail))
	    CfgEdgeKill(BBL_PRED_FIRST(tail));

	  BblKill(tail);

	  leaders[j]=NULL;
	  CODEBYTE_SET_OVERLAP(leaders[i],CODEBYTE_OVERLAP(leaders[i])+1);
	}

	/*decided ok*/
      }
    }
    if(!i_first)
    {
      t_i386_ins * ret_ins;
      
      InsKill(BBL_INS_LAST(bbl1));
      
      ret_ins = I386InsNewForBbl(bbl1);
      I386InstructionMakeReturn(ret_ins);
      I386InsAppendToBbl(ret_ins,bbl1);
      ResetLinksForBbl(bbl1);
    }
  }
  SmcBranchForwarding(cfg);
  VERBOSE(0,("STOP FACTORROUND4"));
}
/*}}}*/

t_bbl *  SmcFactorWithOneByteModifier(t_codebyte * codebyte_to_write, t_bool i_first, t_uint32 i, t_uint32 j, t_codebyte ** leaders, t_cfg * cfg, t_uint32 valuej, t_bbl * head_of_merged_chain)/*{{{*/
{
  if(i_first)
  {
    t_bbl * bbl_original;
    t_uint32 valuei = STATE_VALUE(CODEBYTE_STATE_FIRST(codebyte_to_write));
    {
      t_i386_ins * modifier;

      t_i386_ins * jump_ins;

      t_bbl * bbl = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i])));

      jump_ins =  I386InsNewForBbl(bbl);
      I386InstructionMakeJump(jump_ins);
      I386InsPrependToBbl(jump_ins,bbl);

      modifier = I386InsNewForBbl(bbl);
    //I386_INS_SET_IS_MODIFIER(modifier,TRUE);
      I386InstructionMakeMovToMemLen(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,valuei,1);
      I386InsPrependToBbl(modifier, bbl);
      RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(codebyte_to_write),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
      I386_OP_FLAGS(I386_INS_DEST(modifier)) =I386_OPFLAG_ISRELOCATED;

      bbl_original = BblSplitBlock(bbl,T_INS(jump_ins),FALSE);

      AddLinksForBbl(bbl);

      CfgEdgeKill(BBL_SUCC_FIRST(bbl));

      CfgEdgeNew(cfg,bbl,bbl_original,ET_JUMP);
    }

    {
      t_i386_ins * modifier;

      t_i386_ins * jump_ins;

      t_bbl * bbl_to_eliminate;

      t_bbl * bbl = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])));

      jump_ins =  I386InsNewForBbl(bbl);
      I386InstructionMakeJump(jump_ins);
      I386InsPrependToBbl(jump_ins,bbl);

      modifier = I386InsNewForBbl(bbl);
    //I386_INS_SET_IS_MODIFIER(modifier,TRUE);
      I386InstructionMakeMovToMemLen(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,valuej,1);
      I386InsPrependToBbl(modifier, bbl);
      RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(codebyte_to_write),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
      I386_OP_FLAGS(I386_INS_DEST(modifier)) =I386_OPFLAG_ISRELOCATED;

      bbl_to_eliminate = BblSplitBlock(bbl,T_INS(jump_ins),FALSE);

      CfgEdgeKill(BBL_SUCC_FIRST(bbl));

      CfgEdgeNew(cfg,bbl,bbl_original,ET_JUMP);

      head_of_merged_chain = SmcMergeBbls(bbl_original,bbl_to_eliminate);

      leaders[j]=NULL;

      CODEBYTE_SET_OVERLAP(leaders[i],CODEBYTE_OVERLAP(leaders[i])+1);
    }

    CODEBYTE_SET_SCREWED(leaders[i],TRUE);
  }
  else
  {
    t_i386_ins * modifier;

    t_i386_ins * jump_ins;

    t_bbl * bbl_to_eliminate;

    t_bbl * bbl = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])));

    jump_ins =  I386InsNewForBbl(bbl);
    I386InstructionMakeJump(jump_ins);
    I386InsPrependToBbl(jump_ins,bbl);

    modifier = I386InsNewForBbl(bbl);
    //I386_INS_SET_IS_MODIFIER(modifier,TRUE);
    I386InstructionMakeMovToMemLen(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,valuej,1);
    I386InsPrependToBbl(modifier, bbl);
    RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(codebyte_to_write),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
    I386_OP_FLAGS(I386_INS_DEST(modifier)) =I386_OPFLAG_ISRELOCATED;

    bbl_to_eliminate = BblSplitBlock(bbl,T_INS(jump_ins),FALSE);

    CfgEdgeKill(BBL_SUCC_FIRST(bbl));

    CfgEdgeNew(cfg,bbl,head_of_merged_chain,ET_JUMP);

    SmcMergeBbls2(head_of_merged_chain,bbl_to_eliminate);

    /*
    while(BBL_INS_FIRST(bbl_to_eliminate))
      InsKill(BBL_INS_FIRST(bbl_to_eliminate));
    while(BBL_SUCC_FIRST(bbl_to_eliminate))
      CfgEdgeKill(BBL_SUCC_FIRST(bbl_to_eliminate));
    while(BBL_PRED_FIRST(bbl_to_eliminate))
      CfgEdgeKill(BBL_PRED_FIRST(bbl_to_eliminate));
    
    BblKill(bbl_to_eliminate);*/

    leaders[j]=NULL;
    CODEBYTE_SET_OVERLAP(leaders[i],CODEBYTE_OVERLAP(leaders[i])+1);
  }
  return head_of_merged_chain;
}/*}}}*/


void ModifierRound1(t_cfg * cfg, t_codebyte ** leaders, t_uint32 leader_count)/*{{{*/
{
  t_uint32 i,j;
  
  VERBOSE(0,("START MODIFIERROUND1"));

  for(i=0;i<leader_count;i++)
  {
    t_bool i_first = TRUE;
    t_bbl * head_of_merged_chain = NULL;
    if(leaders[i]==NULL)
      continue;
    
    for(j=i+1;j<leader_count;j++)
    {
      t_codebyte * codebyte_to_write;
      t_uint32 valuej;
      if(leaders[j]==NULL)
	continue;

      codebyte_to_write = SmcChainDistance1(leaders[i],leaders[j],&valuej);
      if(codebyte_to_write)
      {
	VERBOSE(0,("%d %d",i,j));
	head_of_merged_chain = SmcFactorWithOneByteModifier(codebyte_to_write, i_first, i, j, leaders, cfg, valuej, head_of_merged_chain);
	i_first = FALSE;
      }
      
    }
  //  if(!i_first)
  //    CODEBYTE_SET_SCREWED(leaders[i],TRUE);
  }
  SmcBranchForwarding(cfg);
  VERBOSE(0,("STOP MODIFIERROUND1"));
}
/*}}}*/

#if 0
static void Identical(t_cfg * cfg, t_codebyte ** leaders, t_uint32 leader_count)/*{{{*/
{
  t_uint32 teller=0;
  t_uint32 i;

  
  VERBOSE(0,("START IDENTICAL"));

  for(i=0;i<leader_count;i++)
  {
    if(leaders[i]==NULL)
      continue;

    t_ins * leading_ins=STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i]));
    t_ins * ins;
    t_regset dead = RegsetNew();
    t_bbl * orig_bbl=INS_BBL(leading_ins);

    BBL_FOREACH_INS_R(INS_BBL(leading_ins),ins)
    {
      if(ins==BBL_INS_LAST(INS_BBL(leading_ins)))
	continue;
      t_bbl *virtual;
      dead = NullRegs;

      t_bool already_duplicated=FALSE;
      if(I386_OP_FLAGS(I386_INS_SOURCE1(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	already_duplicated=TRUE;
      if(I386_OP_FLAGS(I386_INS_SOURCE2(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	already_duplicated=TRUE;
      if(I386_OP_FLAGS(I386_INS_DEST(T_I386_INS(ins))) & I386_OPFLAG_ISRELOCATED)
	already_duplicated=TRUE;

      if(!already_duplicated)
      {

	virtual=I386InsFindAlternativesTable(T_I386_INS(ins),dead);

	if(virtual)
	{
	  t_i386_ins * insert_ins;
	  t_bbl* split_off;
	  t_bbl* constr_bbl;

	  insert_ins = I386InsNewForBbl(orig_bbl);
	  I386InstructionMakeCondJump(insert_ins,I386_CONDITION_NS);
	  I386InsPrependToBbl (insert_ins,orig_bbl);

	  split_off=BblSplitBlock(orig_bbl,T_INS(insert_ins),FALSE);
	  if(!GenerateConstructionInBbl(virtual, orig_bbl , insert_ins, FALSE))
	    FATAL(("ERR"));

	  constr_bbl=BblSplitBlock(orig_bbl,T_INS(insert_ins),FALSE);
	  
	  CfgEdgeCreate(cfg,orig_bbl,split_off,ET_JUMP);
	    
	  t_i386_ins * i_ins;
	  i_ins = I386InsNewForBbl(constr_bbl);
	  I386InstructionMakeMovToMemLen(i_ins,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0x90,1);
	  I386InsAppendToBbl(i_ins, constr_bbl);
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(i_ins),0x2,T_RELOCATABLE(insert_ins),0,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	  I386_OP_FLAGS(I386_INS_DEST(i_ins)) =I386_OPFLAG_ISRELOCATED;

	  i_ins = I386InsNewForBbl(constr_bbl);
	  I386InstructionMakeMovToMemLen(i_ins,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0xe9,1);
	  I386InsAppendToBbl(i_ins, constr_bbl);
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(i_ins),0x2,T_RELOCATABLE(insert_ins),1,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	  I386_OP_FLAGS(I386_INS_DEST(i_ins)) =I386_OPFLAG_ISRELOCATED;
	  
#if 0
	  t_ins * modifier;
	  modifier = I386InsNewForBbl(constr_bbl);
	  I386InstructionMakeMovToMem(modifier,0,I386_REG_NONE,I386_REG_NONE,I386_SCALE_1,I386_REG_NONE,0);
	  I386InsAppendToBbl(modifier, constr_bbl);
	  
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x2,T_RELOCATABLE(insert_ins),1,FALSE,NULL,NULL,NULL,"R00A00+" "\\" WRITE_32);
	  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(SECTION_OBJECT(CFG_SECTION(cfg))),0x0,T_RELOCATABLE(modifier),0x6,T_RELOCATABLE(insert_ins),5,FALSE,NULL,NULL,target_bbl,"R01R00-" "\\" WRITE_32);
	  I386_OP_FLAGS(I386_INS_DEST(modifier)) =I386_OPFLAG_ISRELOCATED;
	  I386_OP_FLAGS(I386_INS_SOURCE1(modifier)) =I386_OPFLAG_ISRELOCATED;
#endif


	  leaders[i]=NULL;
	  teller++;
	  break;
	}
      }
    } 

  }
  printf("Nr of bbls expanded with identical instructions: %d\n",teller);
  SmcBranchForwarding(cfg);
  VERBOSE(0,("STOP IDENTICAL"));
}
/*}}}*/

#endif
static t_bool FactorRound(t_cfg * cfg, t_codebyte ** leaders, t_uint32 leader_count)/*{{{*/
{
  t_uint32 i,j;
  t_bool change = FALSE;

  VERBOSE(0,("START FACTORROUND"));

  for(i=0;i<leader_count;i++)
  {
    if(leaders[i]==NULL)
      continue;
    
    for(j=i+1;j<leader_count;j++)
    {
      if(leaders[j]==NULL)
	continue;
      
      if(ChainDistance0(leaders[i],leaders[j]))
      {
	t_bbl * bbl = INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[j])));
	t_cfg_edge * edge;
	t_ins * ins;
	t_i386_ins * jump_ins;
	change = TRUE;

	BBL_FOREACH_SUCC_EDGE(bbl,edge)
	{
	  CfgEdgeKill(edge);
	}
	BBL_FOREACH_INS(bbl,ins)
	{
	  InsKill(ins);
	}

	jump_ins = I386InsNewForBbl(bbl);
	I386InstructionMakeJump(jump_ins);
	I386InsAppendToBbl(jump_ins,bbl);
	CfgEdgeNew(cfg,bbl,INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(leaders[i]))),ET_JUMP);

	leaders[j]=NULL;
	
	CODEBYTE_SET_OVERLAP(leaders[i],CODEBYTE_OVERLAP(leaders[i])+1);
      }
    }
  }
  
  SmcBranchForwarding(cfg);
  VERBOSE(0,("STOP FACTORROUND"));
  return change;
}
/*}}}*/

t_bool IsAcceptable(t_codebyte * codebyte)/*{{{*/
{
  //if(BBL_OLD_ADDRESS(INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte))))==0x8081f40)
  //  return FALSE;
  return (!BblIsHot(INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte))))&&!BblIsFrozen(INS_BBL(STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte)))));
}/*}}}*/

void SmcFactorFini(t_cfg * cfg, t_codebyte *** leaders)/*{{{*/
{
  Free(*leaders);
  CodebyteStopPrevInChain(cfg);
  CodebyteStopNextInChain(cfg);
  
  {
    t_bbl * i_bbl, * j_bbl;
    CFG_FOREACH_BBL_SAFE(cfg,i_bbl,j_bbl)
    {
      if (BBL_NINS(i_bbl)==1 && I386_INS_OPCODE(T_I386_INS(BBL_INS_FIRST(i_bbl)))==I386_JMP)
      {
	if(BBL_PRED_FIRST(i_bbl))
	  continue;
	
	while(BBL_SUCC_FIRST(i_bbl))
	  CfgEdgeKill(BBL_SUCC_FIRST(i_bbl));
	while(BBL_INS_FIRST(i_bbl))
	  InsKill(BBL_INS_FIRST(i_bbl));
	BblKill(i_bbl);
      }
    }
  }

  SmcBranchForwarding(cfg);
  SmcBranchElimination(cfg);
}/*}}}*/

t_uint32 SmcFactorInit(t_cfg * cfg, t_codebyte *** leaders) {/*{{{*/
  t_codebyte * codebyte;
  t_codebyte_ref * codebyte_ref;
  t_uint32 leader_count = 0;

  AdaptForMinimalChains(cfg);

  CodebyteStartNextInChain(cfg);
  CodebyteStartPrevInChain(cfg);
  SmcCreateChains(cfg);

  CFG_FOREACH_CODEBYTE(cfg,codebyte)
  {
    if(!CODEBYTE_PREV_IN_CHAIN(codebyte) && IsAcceptable(codebyte))
      leader_count++;
  }

  VERBOSE(0,("NOT HOT NUMBER OF SNIPPETS: %d",leader_count));

  (*leaders) = Calloc(leader_count,sizeof(t_codebyte *));
  leader_count = 0;

  CFG_FOREACH_CODEBYTE(cfg,codebyte)
  {
    if(!CODEBYTE_PREV_IN_CHAIN(codebyte) && IsAcceptable(codebyte))
    {
      (*leaders)[leader_count++]=codebyte;
      CODEBYTE_SET_OVERLAP(codebyte,1);
      CODEBYTE_SET_SCREWED(codebyte,FALSE);
    }
  }
  return leader_count;
}/*}}}*/

void SmcFactor(t_cfg * cfg)/*{{{*/
{
  //t_bbl * bbl;
  t_codebyte ** leaders;
  t_uint32 leader_count=0;
  //t_codebyte * codebyte;
  //t_codebyte_ref * codebyte_ref;

  VERBOSE(0,("START FACTOR"));
  
/*
  CfgComputeHotBblThreshold (cfg, 0.90);
  
  CFG_FOREACH_BBL(cfg,bbl)
  {
    t_ins * ins;
    BBL_FOREACH_INS(bbl,ins)
    {
      SmcInitInstruction(ins);
    }
  }
  CFG_FOREACH_BBL_SAFE(cfg,bbl,tmp_bbl)
  {
    if(BBL_INS_FIRST(bbl))
      if(INS_IS_DATA(BBL_INS_FIRST(bbl)))
      {
	FATAL(("it happens"));
	while(BBL_PRED_FIRST(bbl))
	{
	  t_cfg_edge * edge = BBL_PRED_FIRST(bbl);
	  CfgEdgeNew(cfg,CFG_EDGE_HEAD(edge),CFG_EDGE_HEAD(edge),CFG_EDGE_CAT(edge));
	  CfgEdgeKill(edge);
	}
	if(BBL_SUCC_FIRST(bbl))
	  FATAL((""));
	while(BBL_INS_FIRST(bbl))
	{
	  InsKill(BBL_INS_FIRST(bbl));
	}
	BblKill(bbl);
      }
  }
  */
  /*
  AdaptForMinimalChains(cfg);

  CodebyteStartNextInChain(cfg);
  CodebyteStartPrevInChain(cfg);
  SmcCreateChains(cfg);
  */
  /*
  CFG_FOREACH_CODEBYTE(cfg,codebyte)
  {*/
#if 0 //{{{
    VERBOSE(0,("cb:@I",STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte))));
    t_ins * ii=STATE_PARENT_INS(CODEBYTE_STATE_FIRST(codebyte));
    t_cfg_edge * edge;

    VERBOSE(0,("BBL:@iB \n",INS_BBL(ii)));
    BBL_FOREACH_PRED_EDGE(INS_BBL(ii),edge)
    {
      VERBOSE(0,("Pred:%d BBL:@iB \n",EDGE_CAT(edge),EDGE_HEAD(edge)));
    }
    BBL_FOREACH_SUCC_EDGE(INS_BBL(ii),edge)
    {
      VERBOSE(0,("Succ:%d BBL:@iB \n",EDGE_CAT(edge),EDGE_TAIL(edge)));
    }
#endif //}}}
    /*
    if(!CODEBYTE_PREV_IN_CHAIN(codebyte))
    {
//      VERBOSE(0,("LEADER\n"));
      leader_count++;
    }
  }

  VERBOSE(0,("TOTAL NUMBER OF SNIPPETS: %d",leader_count));
  
  leader_count = 0;
  */

    
  /*
  CFG_FOREACH_CODEBYTE(cfg,codebyte)
  {
    if(!CODEBYTE_PREV_IN_CHAIN(codebyte) && IsAcceptable(codebyte))
      leader_count++;
  }*/
/*  
  VERBOSE(0,("NOT HOT NUMBER OF SNIPPETS: %d",leader_count));
  
  leaders = Calloc(leader_count,sizeof(t_codebyte *));
  leader_count = 0;
  
  CFG_FOREACH_CODEBYTE(cfg,codebyte)
  {
    if(!CODEBYTE_PREV_IN_CHAIN(codebyte) && IsAcceptable(codebyte))
    {
      leaders[leader_count++]=codebyte;
      CODEBYTE_SET_OVERLAP(codebyte,1);
      CODEBYTE_SET_SCREWED(codebyte,FALSE);
    }
  }
  */
  leader_count =  SmcFactorInit(cfg,&leaders);

  /*TODO: ugly hack,make option instead*/
  if (diablosupport_options.debugcounter == 0)
  {
    while(FactorRound(cfg,leaders,leader_count));
    FactorRound4(cfg,leaders,leader_count);
    ModifierRound1(cfg,leaders,leader_count);
    ModifierRound4(cfg,leaders,leader_count);
    //Identical(cfg,leaders,leader_count);
  }

  /*
  {
    t_uint32 i;
    t_uint32 max=1;
    t_uint32 * counters;
    for(i=0;i<leader_count;i++)
    {
      if(leaders[i]==NULL)
	continue;
      if(CODEBYTE_OVERLAP(leaders[i]) > max)
	max = CODEBYTE_OVERLAP(leaders[i]);
    }

    counters = (t_uint32 *)Calloc(max, sizeof(t_uint32));
    
    for(i=0;i<leader_count;i++)
    {
      if(leaders[i]==NULL)
	continue;
      counters[CODEBYTE_OVERLAP(leaders[i])-1]++;
    }

    for(i=0;i<max;i++)
    {
      VERBOSE(0,("OVERLAP: %d %d",i+1,counters[i]*(i+1)));
    }
  }
  */
  SmcFactorFini(cfg,&leaders);
/*
  Free(leaders);
  CodebyteStopPrevInChain(cfg);
  CodebyteStopNextInChain(cfg);
  
  
  {
    t_bbl * i_bbl, * j_bbl;
    CFG_FOREACH_BBL_SAFE(cfg,i_bbl,j_bbl)
    {
      if (BBL_NINS(i_bbl)==1 && I386_INS_OPCODE(T_I386_INS(BBL_INS_FIRST(i_bbl)))==I386_JMP)
      {
	if(BBL_PRED_FIRST(i_bbl))
	  continue;
	
	while(BBL_SUCC_FIRST(i_bbl))
	  CfgEdgeKill(BBL_SUCC_FIRST(i_bbl));
	while(BBL_INS_FIRST(i_bbl))
	  InsKill(BBL_INS_FIRST(i_bbl));
	BblKill(i_bbl);
      }
    }
  }

  SmcBranchForwarding(cfg);
  SmcBranchElimination(cfg);
  VERBOSE(0,("STOP FACTOR"));
  */
}

/*}}}*/

/* vim: set shiftwidth=2 foldmethod=marker:*/
