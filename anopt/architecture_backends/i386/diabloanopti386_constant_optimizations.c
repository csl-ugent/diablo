#include <diabloanopti386.h>

t_bool I386MakeIndirectCallsDirect(t_ins * ins, t_procstate *before, t_procstate *after)
{
#if 0
  t_i386_ins * iins=T_I386_INS(ins);
#endif

  FATAL(("Reimplement"));
#if 0
  if(I386_INS_OPCODE(iins)==I386_CALL)
  {
    t_reloc * rel;

    if(I386_OP_TYPE(I386_INS_SOURCE1(iins))==i386_optype_reg)
    {
      if(ProcStateGetTag(before,I386_OP_BASE(I386_INS_SOURCE1(iins)),&rel)==CP_VALUE)
      {
	VERBOSE(0,("@I\ntag VALUE:@R",iins,rel));
	FATAL(("Mogelijkheid om een indirect call weg te werken!"));
      }
    }
    else if(I386_OP_TYPE(I386_INS_SOURCE1(iins))==i386_optype_mem)
    {
      t_reloc  * rel;

      if(ProcStateGetTag(before,I386_OP_BASE(I386_INS_SOURCE1(iins)),&rel)==CP_VALUE)
      {

	/* Switch-tables */
	if (RELOCATABLE_RELOCATABLE_TYPE(REL_TO_RELOCATABLE(rel)) != RT_SUBSECTION)
	{
	  VERBOSE(0,("@I\ntag VALUE:@R",iins,rel));
	  FATAL(("Mogelijkheid om een indirect call weg te werken!"));
	}
      }
    }
  }
#endif
  return TRUE;
}

  
t_bool I386MakeIndirectJumpsDirect(t_ins * ins, t_procstate *before, t_procstate *after, t_analysis_complexity complexity)
{
  FATAL(("Reimplement"));
#if 0
  t_i386_ins * iins=T_I386_INS(ins);
  if (I386_INS_TYPE(iins) != IT_BRANCH) return TRUE;

  if (I386_OP_TYPE(I386_INS_SOURCE1(iins)) == i386_optype_mem ||
      I386_OP_TYPE(I386_INS_SOURCE1(iins)) == i386_optype_reg)
  {
    t_uint32 value;
    t_reloc *r;
    t_lattice_level vl,rl;

    GetConstValueForOp(iins,I386_INS_SOURCE1(iins),before,FALSE,&vl,&value,&rl,&r);
    if (rl == CP_VALUE && RELOCATABLE_RELOCATABLE_TYPE(REL_TO_RELOCATABLE(r)) == RT_BBL)
    {
      t_cfg_edge *edge;
      t_bbl *to = T_BBL(REL_TO_RELOCATABLE(r));
      t_bbl *from = I386_INS_BBL(iins);

      /* skip indirect jumps to the next instruction as these
       * are used for their side effect: they clean out the 
       * pipeline after processor mode switches */
      if (AddressIsEq(BBL_OLD_ADDRESS(to),
	    AddressAdd(INS_OLD_ADDRESS(T_INS(iins)),INS_OLD_SIZE(T_INS(iins)))))
	return FALSE;

      VERBOSE(0,("DOING IT FOR @I rel = @R",iins,r));

      I386InstructionMakeDirect(iins);
      BBL_FOREACH_SUCC_EDGE(from,edge)
	if (CfgEdgeTestCategoryOr(edge,(ET_JUMP|ET_IPJUMP|ET_CALL|ET_SWITCH|ET_IPSWITCH)))
	  break;
      if (!edge) FATAL(("need edge"));
      if (CfgEdgeTestCategoryOr(edge,(ET_SWITCH|ET_IPSWITCH)))
	FATAL(("IMPLEMENT MAKE DIRECT FOR SWITCHES"));
      if (CFG_EDGE_CAT(edge) == ET_CALL)
      {
	t_bbl *returnblock = NULL;
	if (CFG_EDGE_CORR(edge)) 
	{
	  t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);
	  
	  if (complexity == CONTEXT_INSENSITIVE && corr_edge == BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge)))
	    {
	      
	      if (CFG_EDGE_SUCC_NEXT(corr_edge))
		{
		  CFG_EDGE_SET_PROCSTATE(CFG_EDGE_SUCC_NEXT(corr_edge),  CFG_EDGE_PROCSTATE(corr_edge));
		  CFG_EDGE_SET_PROCSTATE(corr_edge,  NULL);
		}
	    }
	  
	  returnblock = (t_bbl *)CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
	  CfgEdgeKill(CFG_EDGE_CORR(edge));
	}

	if (complexity == CONTEXT_INSENSITIVE && edge == BBL_PRED_FIRST(CFG_EDGE_TAIL(edge)) && CfgEdgeIsForwardInterproc(edge))
	  {
	    if (CFG_EDGE_PRED_NEXT(edge))
	      {
		CFG_EDGE_SET_PROCSTATE(CFG_EDGE_PRED_NEXT(edge),  CFG_EDGE_PROCSTATE(edge));
		CFG_EDGE_SET_PROCSTATE(edge,  NULL);
	      }
	  }

	CfgEdgeKill(edge);
	CfgEdgeCreateCall(BBL_CFG(from),from,to,returnblock,FunctionGetExitBlock(BBL_FUNCTION(to)));
      }
      else
      {
	if (CFG_EDGE_CORR(edge)) 
	  {
	    t_cfg_edge * corr_edge = CFG_EDGE_CORR(edge);
	    
	    if (complexity == CONTEXT_INSENSITIVE && corr_edge == BBL_SUCC_FIRST(CFG_EDGE_HEAD(corr_edge)))
	      {
		
		if (CFG_EDGE_SUCC_NEXT(corr_edge))
		  {
		    CFG_EDGE_SET_PROCSTATE(CFG_EDGE_SUCC_NEXT(corr_edge),  CFG_EDGE_PROCSTATE(corr_edge));
		    CFG_EDGE_SET_PROCSTATE(corr_edge,  NULL);
		  }
	      }
	    CfgEdgeKill(CFG_EDGE_CORR(edge));
	  }

	if (complexity == CONTEXT_INSENSITIVE && edge == BBL_PRED_FIRST(CFG_EDGE_TAIL(edge)) && CfgEdgeIsForwardInterproc(edge))
	  {
	    if (CFG_EDGE_PRED_NEXT(edge))
	      {
		CFG_EDGE_SET_PROCSTATE(CFG_EDGE_PRED_NEXT(edge),  CFG_EDGE_PROCSTATE(edge));
		CFG_EDGE_SET_PROCSTATE(edge,  NULL);
	      }
	  }

	CfgEdgeKill(edge);

	if (BBL_FUNCTION(from) == BBL_FUNCTION(to))
	  CfgEdgeCreate(BBL_CFG(from),from,to,ET_JUMP);
	else
	{
	  edge = CfgEdgeCreate(BBL_CFG(from),from,to,ET_IPJUMP);
	  CFG_EDGE_SET_CORR(edge,  CfgEdgeCreateCompensating(BBL_CFG(from),edge));
	}
      }
    }
  }
#endif
  return TRUE;
}
/* vim: set shiftwidth=2 foldmethod=marker: */
