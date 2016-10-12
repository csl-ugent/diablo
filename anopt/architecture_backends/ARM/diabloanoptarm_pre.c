/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloanoptarm.h>

static t_bool
InsIsFirstInConditionalBundle (t_ins * ins)
{
  t_arm_ins *ains = (t_arm_ins *) ins;
  t_arm_ins *prev_ins = ARM_INS_IPREV(ains);
  t_uint32 cond;


  if (!(INS_ATTRIB(ins) & IF_CONDITIONAL))
    return FALSE;

  cond = ARM_INS_CONDITION(ains);

  while (prev_ins)
  {
    if (ArmInsUpdatesCond (prev_ins))
      return TRUE;
    if (ARM_INS_CONDITION(prev_ins) == cond)
      return FALSE;
    if (ARM_INS_CONDITION(prev_ins) == ArmInvertCondition (cond))
      return FALSE;
    prev_ins = ARM_INS_IPREV(prev_ins);
  }

  return TRUE;
}

static t_uint32
ConditionalBundleSize (t_ins * first_ins)
{
  t_arm_ins *next_ins = T_ARM_INS(first_ins);
  t_uint32 cond;
  int count = 0;

  if (!InsIsFirstInConditionalBundle (first_ins))
    return 0;

  cond = ARM_INS_CONDITION(next_ins);

  while (next_ins)
  {
    if (ARM_INS_CONDITION(next_ins) == cond)
      count++;
    if (ArmInsUpdatesCond (next_ins))
      return count;
    next_ins = ARM_INS_INEXT(next_ins);
  }
  return count;
}

static t_uint32
InverseConditionalBundleSize (t_ins * first_ins)
{
  t_arm_ins *next_ins = T_ARM_INS(first_ins);
  t_uint32 cond;
  int count = 0;

  cond = ARM_INS_CONDITION(next_ins);

  while (next_ins)
  {
    if (ArmIsControlflow (next_ins))
      break;
    if (ARM_INS_CONDITION(next_ins) == ArmInvertCondition (cond))
      count++;
    if (ArmInsUpdatesCond (next_ins))
      return count;
    next_ins = ARM_INS_INEXT(next_ins);
  }
  return count;
}

t_bool BblEndWithConditionalBranch(t_bbl * bbl)
{
  t_cfg_edge * edge;

  t_bool fallthrough = FALSE;
  t_bool jump = FALSE;

  BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if ((CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP) && !BBL_IS_LAST(CFG_EDGE_TAIL(edge)))
	jump = TRUE;
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	fallthrough = TRUE;
    }
  return jump && fallthrough;
}

t_bool BblEndWithConditionalCall(t_bbl * bbl)
{
  t_cfg_edge * edge;

  t_bool fallthrough = FALSE;
  t_bool call = FALSE;

  BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if (CFG_EDGE_CAT(edge)==ET_CALL)
	call = TRUE;
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	fallthrough = TRUE;
    }

  return call && fallthrough;
}

t_bool BblEndWithConditionalReturn(t_bbl * bbl)
{
  t_cfg_edge * edge;

  t_bool fallthrough = FALSE;
  t_bool jump = FALSE;

  BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      if (CFG_EDGE_CAT(edge)==ET_JUMP && BBL_IS_LAST(CFG_EDGE_TAIL(edge)))
	jump = TRUE;
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	fallthrough = TRUE;
    }
  return jump && fallthrough;
}
/*

PartialRedundancyElimination1: if the taken path of a conditional
branch is rarely executed, instructions depending on the same
condition are moved onto that path

*/
void PartialRedundancyElimination1(t_cfg * cfg)
{
  t_bbl * bbl;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BblIsHot(bbl))
	{
	restart:
	  if (BblEndWithConditionalBranch(bbl))
	    {
	      t_cfg_edge * taken = TakenPath(bbl);
	      t_cfg_edge * fall = FallThroughPath(bbl);

	      t_arm_ins * last_ins = (t_arm_ins *) BBL_INS_LAST(bbl);
	      t_regset used_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset cond_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset changed_regs = RegsetNew();
	      t_arm_ins * ins = ARM_INS_IPREV(last_ins);
	      EXEC_CONDITION cond_last_ins = UNKNOWN;
	      EXEC_CONDITION cond = UNKNOWN;
	      
	      if (BBL_IS_HELL(CFG_EDGE_TAIL(taken))) continue;
	      if (!(CFG_EDGE_EXEC_COUNT(taken)<CFG_EDGE_EXEC_COUNT(fall)/3))
		continue;

	      cond_last_ins = ArmInsCondition(last_ins);

	      while (ins)
		{
		  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),cond_regs)))
		    {
		      ins = NULL;
		      break;
		    }

		  cond = ArmInsCondition(ins);

		  if (cond!=UNKNOWN && 
		      cond == cond_last_ins && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),used_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),changed_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_USE(ins),changed_regs))
		      )
		    break;
		  
		  RegsetSetUnion(used_regs,ARM_INS_REGS_USE(ins));
		  RegsetSetUnion(changed_regs,ARM_INS_REGS_DEF(ins));
		  
		  ins = ARM_INS_IPREV(ins);
		}

	      if (ins)
	      {
		t_bbl * new_bbl = BblNew(cfg);
		t_arm_ins * new_ins;
		t_cfg_edge * new_edge;
		t_cfg_edge * new_edge2;
/*#define DEBUG_PRE1*/
#ifdef DEBUG_PRE1
		static int teller = 0;
		t_string x = malloc(100);
		t_string y = malloc(100);

		if (teller >= diablosupport_options.debugcounter) return;
		VERBOSE(0,("PRE1 candidate @ieB\n",bbl));
		printf("%d/%d\n",CFG_EDGE_EXEC_COUNT(taken),INS_EXEC_COUNT(ins));
		sprintf(x,"pre1_pre%d.dot",++teller);
		sprintf(y,"pre1_post%d.dot",teller);
		FunctionDrawGraphWithHotness(BBL_FUNCTION(bbl),x);
#endif

		BblInsertInFunction(new_bbl,BBL_FUNCTION(bbl));
		BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(taken));

		new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(taken),CFG_EDGE_CAT(taken));
		CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(taken));

		if (CFG_EDGE_CORR(taken))
		{
		  CFG_EDGE_SET_CORR(new_edge, CFG_EDGE_CORR(taken));
/*                  CfgEdgeKill(CFG_EDGE_CORR(taken));*/
		  CFG_EDGE_SET_CORR(CFG_EDGE_CORR(new_edge), new_edge);
		  CFG_EDGE_SET_CORR(taken, NULL);
		}
		if(!CfgEdgeIsForwardInterproc(taken))
		  new_edge2 = CfgEdgeCreate(cfg,bbl,new_bbl,CFG_EDGE_CAT(taken));
		else
		  new_edge2 = CfgEdgeCreate(cfg,bbl,new_bbl,ET_JUMP);
		CfgEdgeKill(taken);
		CFG_EDGE_SET_EXEC_COUNT(new_edge2, CFG_EDGE_EXEC_COUNT(new_edge));

		new_ins = ArmInsNewForBbl(new_bbl);
		ArmInsMakeUncondBranch(new_ins);		  
		ArmInsAppendToBbl(new_ins,new_bbl);
		ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));

		new_ins = ArmInsDup(ins);
		ArmInsPrependToBbl(new_ins,new_bbl);
		ArmInsUnconditionalizer(new_ins);
		ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));

		ArmInsKill(ins);

#ifdef DEBUG_PRE1
		VERBOSE(0,("PRE1 after @ieB @ieB\n",bbl,new_bbl));
		FunctionDrawGraphWithHotness(BBL_FUNCTION(bbl),y);
#endif

		goto restart;
	      }
	    }
	}
    }
}

/*

PartialRedundancyElimination1ForReturns: if a conditional return is rarely
executed, and instructions depending on the same condition exist, they
are moved into a separate block, and replaced by a conditional branch

*/

void PartialRedundancyElimination1ForReturns(t_cfg * cfg)
{
  t_bbl * bbl;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);
  CfgEstimateEdgeCounts(cfg);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BblIsHot(bbl))
	{
	  if (BblEndWithConditionalReturn(bbl))
	    {
	      t_cfg_edge * taken = TakenPath(bbl);
	      t_cfg_edge * fall = FallThroughPath(bbl);
	      t_arm_ins * last_ins = (t_arm_ins *) BBL_INS_LAST(bbl);
	      t_regset used_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset cond_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset changed_regs = RegsetNew();
	      t_arm_ins * ins = ARM_INS_IPREV(last_ins);
	      EXEC_CONDITION cond_last_ins = UNKNOWN;
	      EXEC_CONDITION cond = UNKNOWN;
	      


	      if (!(CFG_EDGE_EXEC_COUNT(taken)<CFG_EDGE_EXEC_COUNT(fall)/3))
		continue;

	      cond_last_ins = ArmInsCondition(last_ins);

	      while (ins)
		{
		  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),cond_regs)))
		    {
		      ins = NULL;
		      break;
		    }

		  cond = ArmInsCondition(ins);

		  if (cond!=UNKNOWN && 
		      cond == cond_last_ins && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),used_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),changed_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_USE(ins),changed_regs))
		      )
		    break;
		  
		  RegsetSetUnion(used_regs,ARM_INS_REGS_USE(ins));
		  RegsetSetUnion(changed_regs,ARM_INS_REGS_DEF(ins));
		  
		  ins = ARM_INS_IPREV(ins);
		}

	      if (ins)
		{
		  t_bbl * new_bbl = BblNew(cfg);
		  t_arm_ins * new_ins;
		  t_cfg_edge * new_edge;
		  t_cfg_edge * new_edge2;
		  t_uint32 cond = ARM_INS_CONDITION(ins);

		  VERBOSE(10,("PRE1return  candidate @ieB\n",bbl)); fflush(stdout);

		  BblInsertInFunction(new_bbl,BBL_FUNCTION(bbl));
		  BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(taken));
		  
		  new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(taken),ET_JUMP);
		  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(taken));
		  if (CFG_EDGE_CORR(taken))
		    {
		      CfgEdgeKill(CFG_EDGE_CORR(taken));
		    }
		  new_edge2 = CfgEdgeCreate(cfg,bbl,new_bbl,CFG_EDGE_CAT(taken));
		  CfgEdgeKill(taken);
		  CFG_EDGE_SET_EXEC_COUNT(new_edge2, CFG_EDGE_EXEC_COUNT(new_edge));

		  new_ins = ArmInsDup(T_ARM_INS(BBL_INS_LAST(bbl)));
		  ArmInsPrependToBbl(new_ins,new_bbl);
		  ArmInsUnconditionalizer(new_ins);
		  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));

		  new_ins = ArmInsDup(ins);
		  ArmInsPrependToBbl(new_ins,new_bbl);
		  ArmInsUnconditionalizer(new_ins);
		  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));
		  ArmInsKill(ins);

		  ArmInsMakeCondBranch(T_ARM_INS(BBL_INS_LAST(bbl)),cond);

		  VERBOSE(10,("PRE1return after @ieB @ieB\n",bbl,new_bbl));
		}
	    }
	}
    }
}


/*

PartialRedundancyElimination1ForCalls: if a conditional call is rarely
executed, and instructions depending on the same condition exist, they
are moved into a separate block, and replaced by a conditional branch

*/

void PartialRedundancyElimination1ForCalls(t_cfg * cfg)
{
  t_bbl * bbl;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgEstimateEdgeCounts (cfg);
  CfgComputeHotBblThreshold(cfg,0.99);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BblIsHot(bbl))
	{
	  if (BblEndWithConditionalCall(bbl))
	    {
	      t_cfg_edge * taken = TakenPath(bbl);
	      t_cfg_edge * fall = FallThroughPath(bbl);
	      t_arm_ins * last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
	      t_regset used_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset cond_regs = ARM_INS_REGS_USE(last_ins);
	      t_regset changed_regs = RegsetNew();
	      t_arm_ins * ins = ARM_INS_IPREV(last_ins);
	      EXEC_CONDITION cond_last_ins = UNKNOWN;
	      EXEC_CONDITION cond = UNKNOWN;
	      
	      if (!(CFG_EDGE_EXEC_COUNT(taken)<CFG_EDGE_EXEC_COUNT(fall)/3))
		continue;

	      cond_last_ins = ArmInsCondition(last_ins);

	      while (ins)
		{
		  if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),cond_regs)))
		    {
		      ins = NULL;
		      break;
		    }

		  cond = ArmInsCondition(ins);

		  if (cond!=UNKNOWN && 
		      cond == cond_last_ins && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),used_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_DEF(ins),changed_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_USE(ins),changed_regs))
		      )
		    break;
		  
		  RegsetSetUnion(used_regs,ARM_INS_REGS_USE(ins));
		  RegsetSetUnion(changed_regs,ARM_INS_REGS_DEF(ins));
		  
		  ins = ARM_INS_IPREV(ins);
		}

	      if (ins)
		{
		  t_bbl * new_bbl = BblNew(cfg);
		  t_bbl * new_bbl2 = BblNew(cfg);
		  t_arm_ins * new_ins;
		  t_cfg_edge * new_edge;
		  t_cfg_edge * new_edge3;
		  t_cfg_edge * new_edge4;

		  VERBOSE(10,("potential PRE candidate @ieB\n",bbl));

		  BblInsertInFunction(new_bbl,BBL_FUNCTION(bbl));
		  BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(taken));

		  BblInsertInFunction(new_bbl2,BBL_FUNCTION(bbl));
		  BBL_SET_EXEC_COUNT(new_bbl2, CFG_EDGE_EXEC_COUNT(taken));
		  
		  ArmInsConvertToBranch(last_ins);

		  new_ins = ArmInsNewForBbl(new_bbl);
		  ArmInsMakeCondBranchAndLink(new_ins,ARM_CONDITION_AL);
		  ArmInsAppendToBbl(new_ins,new_bbl);
		  
		  new_ins = ArmInsDup(ins);
		  ArmInsPrependToBbl(new_ins,new_bbl);
		  ArmInsKill(ins);

		  new_ins = ArmInsNewForBbl(new_bbl2);
		  ArmInsMakeUncondBranch(new_ins);
		  ArmInsAppendToBbl(new_ins,new_bbl2);

		  new_edge = CfgEdgeCreate(cfg,bbl,new_bbl,ET_JUMP);
		  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(taken));

		  new_edge3 = CfgEdgeCreate(cfg,new_bbl2,CFG_EDGE_TAIL(CFG_EDGE_CORR(taken)),ET_JUMP);
		  CFG_EDGE_SET_EXEC_COUNT(new_edge3, CFG_EDGE_EXEC_COUNT(taken));
		  
		  new_edge4 = CfgEdgeCreateCall(cfg,new_bbl,CFG_EDGE_TAIL(taken),new_bbl2,FUNCTION_BBL_LAST(BBL_FUNCTION(CFG_EDGE_TAIL(taken))));
		  CFG_EDGE_SET_EXEC_COUNT(new_edge4, CFG_EDGE_EXEC_COUNT(taken));
		  
		  CfgEdgeKill(CFG_EDGE_CORR(taken));
		  CfgEdgeKill(taken);
		  
		  VERBOSE(10,("after PRE @ieB\n@ieB\n@ieB\n",bbl,new_bbl,new_bbl2));
		}
	    }
	}
    }
}

/*#define DEBUG_PRE2*/

/* instructions not live on the fall through path of a conditional
   branch are moved to the taken path if that is seldomly taken
*/

void PartialRedundancyElimination2(t_cfg * cfg)
{
  t_bbl * bbl;
  t_ins * ins;
  t_ins * ins_new;
#ifdef DEBUG_PRE2
  static   int teller = 0;
  t_string x = "candidateXXX.dot                    ";
  t_string y = "afterXXX.dot                        ";
#endif

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BblIsHot(bbl))
	{
	  if (BblEndWithConditionalBranch(bbl))
	    {
	      t_cfg_edge * taken = TakenPath(bbl);
	      t_cfg_edge * fall  = FallThroughPath(bbl);
	      t_bbl * new_target;

	      t_regset taken_live = BBL_INS_FIRST(CFG_EDGE_TAIL(taken))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(taken))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(taken));
	      t_regset fall_live = BBL_INS_FIRST(CFG_EDGE_TAIL(fall))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(fall))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(fall));

	      t_regset changed_regs = RegsetNew();
	      t_regset used_regs = RegsetNew();

	      if (CFG_EDGE_EXEC_COUNT(taken)>=BBL_EXEC_COUNT(bbl)/2) continue;

	      BBL_FOREACH_INS_R(bbl,ins)
		{
		  if (!InsHasSideEffect(ins) && 
		      RegsetIsEmpty(RegsetIntersect(fall_live,INS_REGS_DEF(ins))) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(ins),used_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(ins),changed_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(ins),changed_regs))
		      )
		    {
		      break;
		    }

		  RegsetSetUnion(changed_regs,INS_REGS_DEF(ins));
		  RegsetSetUnion(used_regs,INS_REGS_USE(ins));
		}
	      
	      if (!ins) continue;
#ifdef DEBUG_PRE2
	      if (teller>=global_options.debugcounter) continue;

	      VERBOSE(0,("in BBL @B\n we have a candidate @I\n taken: @E \n fall @E\n",bbl,ins,taken,fall));

	      DiabloPrintArch(stdout,cfg->sec->obj->description,"taken live @A fall live @A\n",taken_live, fall_live);

	      x = sDiabloPrint("candidate%d.dot",++teller);
	      y = sDiabloPrint("after%d.dot",teller);

	      FunctionDrawGraph(BBL_FUNCTION(bbl),x);
#endif
	      new_target = CreateNewTargetBlock(cfg,taken);

	      ins_new = InsDup(ins);
	      InsPrependToBbl(ins_new,new_target);
	      INS_SET_EXEC_COUNT(ins_new,  BBL_EXEC_COUNT(new_target)<INS_EXEC_COUNT(ins)?BBL_EXEC_COUNT(new_target):INS_EXEC_COUNT(ins));
	      InsKill(ins);
	      
	      BBL_SET_REGS_LIVE_OUT(new_target, taken_live);

	      if (!(INS_ATTRIB(ins_new) & IF_CONDITIONAL))
		      BBL_SET_REGS_LIVE_OUT(bbl, RegsetDiff(BBL_REGS_LIVE_OUT(bbl),INS_REGS_DEF(ins_new)));

	      BBL_SET_REGS_LIVE_OUT(bbl, RegsetUnion(BBL_REGS_LIVE_OUT(bbl),INS_REGS_USE(ins_new)));
	      /*	      RegsetSetUnion(BBL_REGS_LIVE_OUT(new_target),fall_live);*/
#ifdef DEBUG_PRE2
	      FunctionDrawGraph(BBL_FUNCTION(bbl),y);
#endif	      
	    }	      
	}
    }
}
/*#define DEBUG_PRE3*/

/* 
   instructions not live on the taken path of a conditional branch are
   moved to the fall through path if that is seldomly followed
*/


void PartialRedundancyElimination3(t_cfg * cfg)
{
  t_bbl * bbl;
  t_ins * ins;
  t_ins * ins_new;
#ifdef DEBUG_PRE3
  static   int teller = 0;
  t_string x = "3candidateXXX.dot                    ";
  t_string y = "3afterXXX.dot                        ";
#endif

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (BblIsHot(bbl))
	{
	  if (BblEndWithConditionalBranch(bbl))
	    {
	      t_cfg_edge * taken = TakenPath(bbl);
	      t_cfg_edge * fall  = FallThroughPath(bbl);
	      t_bbl * new_target;

	      t_regset taken_live = BBL_INS_FIRST(CFG_EDGE_TAIL(taken))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(taken))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(taken));
	      t_regset fall_live = BBL_INS_FIRST(CFG_EDGE_TAIL(fall))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(fall))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(fall));

	      t_regset changed_regs = RegsetNew();
	      t_regset used_regs = RegsetNew();

	      if (CFG_EDGE_EXEC_COUNT(fall)>=BBL_EXEC_COUNT(bbl)/2) continue;

	      BBL_FOREACH_INS_R(bbl,ins)
		{
		  if (!InsHasSideEffect(ins) && 
		      RegsetIsEmpty(RegsetIntersect(taken_live,INS_REGS_DEF(ins))) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(ins),used_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(ins),changed_regs)) && 
		      RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(ins),changed_regs))
		      )
		    {
		      break;
		    }

		  RegsetSetUnion(changed_regs,INS_REGS_DEF(ins));
		  RegsetSetUnion(used_regs,INS_REGS_USE(ins));
		}
	      
	      if (!ins) continue;
#ifdef DEBUG_PRE3
	      if (teller>global_options.debugcounter) continue;

	      VERBOSE(0,("in BBL @B\n we have a candidate @I\n fall: @E \n taken @E\n",bbl,ins,fall,taken));

	      DiabloPrintArch(stdout,cfg->sec->obj->description,"OOPS @A @A\n",BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(taken)),INS_REGS_DEF(ins));
	      DiabloPrintArch(stdout,cfg->sec->obj->description,"fall live @A taken live @A\n",fall_live, taken_live);

	      x = sDiabloPrint("3candidate%d.dot",++teller);
	      y = sDiabloPrint("3after%d.dot",teller);

	      FunctionDrawGraph(BBL_FUNCTION(bbl),x);
#endif
	      new_target = CreateNewTargetBlock(cfg,fall);

	      ins_new = InsDup(ins);
	      InsPrependToBbl(ins_new,new_target);
	      INS_SET_EXEC_COUNT(ins_new,  BBL_EXEC_COUNT(new_target)<INS_EXEC_COUNT(ins)?BBL_EXEC_COUNT(new_target):INS_EXEC_COUNT(ins));
	      InsKill(ins);
	      
	      BBL_SET_REGS_LIVE_OUT(new_target, fall_live);

	      if (!(INS_ATTRIB(ins_new) & IF_CONDITIONAL))
		      BBL_SET_REGS_LIVE_OUT(bbl, RegsetDiff(BBL_REGS_LIVE_OUT(bbl),INS_REGS_DEF(ins_new)));

	     BBL_SET_REGS_LIVE_OUT(bbl,  RegsetUnion(BBL_REGS_LIVE_OUT(bbl),INS_REGS_USE(ins_new)));
	      /*	      RegsetSetUnion(BBL_REGS_LIVE_OUT(new_target),taken_live);*/
#ifdef DEBUG_PRE3
	      FunctionDrawGraph(BBL_FUNCTION(bbl),y);
#endif	      
	    }	      
	}
    }
}

/*#define DEBUG_TRIANGLE*/

void ReplaceTriangleWithConditionalMove(t_cfg * cfg)
{
  t_bbl * upper;
  t_arm_ins * ins_new;

#ifdef DEBUG_TRIANGLE
  static int teller = 0;
  t_string x = malloc(100);
  t_string y = malloc(100);
#endif

  CFG_FOREACH_BBL(cfg,upper)
    {
#ifdef DEBUG_TRIANGLE
      if (!(teller>=global_options.debugcounter))
#endif
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);

	      t_bbl * corner = CFG_EDGE_TAIL(taken);

	      t_arm_ins * one_ins = T_ARM_INS(BBL_INS_FIRST(corner));

	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));

	      if (BBL_IS_HELL(CFG_EDGE_TAIL(taken))) continue;
	      if (BBL_IS_HELL(CFG_EDGE_TAIL(fall))) continue;
	      if (BBL_SUCC_FIRST(corner)!=BBL_SUCC_LAST(corner)) continue;
	      if (BBL_PRED_FIRST(corner)!=BBL_PRED_LAST(corner)) continue;
	      if (CFG_EDGE_TAIL(BBL_SUCC_FIRST(corner))!=CFG_EDGE_TAIL(fall)) continue;
	      if (!one_ins) continue;
	      if (ARM_INS_ATTRIB(one_ins) & IF_CONDITIONAL) continue;

	      if (BBL_NINS(corner)>2) continue;

#ifdef DEBUG_TRIANGLE
	      
	      VERBOSE(0,("TRIANGLE FIRST upper @iB\n corner @iB\n down @iB\n",upper,corner,CFG_EDGE_TAIL(fall)));
	      sprintf(x,"triangle1_pre%d.dot",++teller);
	      sprintf(y,"triangle1_post%d.dot",teller);
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),x);
#endif
	      ins_new = ArmInsDup(one_ins);

	      ARM_INS_SET_CONDITION(ins_new, ARM_INS_CONDITION(branch_ins));
	      ARM_INS_SET_ATTRIB(ins_new,   ARM_INS_ATTRIB(ins_new) | IF_CONDITIONAL);
	      ARM_INS_SET_REGS_USE(ins_new,  ArmUsedRegisters(ins_new));
	      ARM_INS_SET_REGS_DEF(ins_new,  ArmDefinedRegisters(ins_new));

	      ArmInsAppendToBbl(ins_new,upper);
	      CFG_EDGE_SET_EXEC_COUNT(fall,  CFG_EDGE_EXEC_COUNT(fall)+CFG_EDGE_EXEC_COUNT(taken));

	      if (CFG_EDGE_CORR(taken))
		{
		  CfgEdgeKill(CFG_EDGE_CORR(taken));
		}
	      CfgEdgeKill(taken);

	      if(CFG_EDGE_CORR(BBL_SUCC_FIRST(corner)))
		CfgEdgeKill(CFG_EDGE_CORR(BBL_SUCC_FIRST(corner)));
	      CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(corner)));
	     
	      ArmInsKill(branch_ins);
	      ArmInsKill(one_ins);
	      if (BBL_INS_LAST(corner))
		InsKill(BBL_INS_LAST(corner));
	      BblKill(corner);
	      
	      BBL_SET_REGS_LIVE_OUT(upper, BBL_INS_FIRST(CFG_EDGE_TAIL(fall))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(fall))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(fall)));
			
#ifdef DEBUG_TRIANGLE
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),y);
#endif
	    }
	}
    }
  

  CFG_FOREACH_BBL(cfg,upper)
    {
#ifdef DEBUG_TRIANGLE
      if (!(teller>=global_options.debugcounter))
#endif
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);

	      t_bbl * corner = CFG_EDGE_TAIL(fall);
	      t_arm_ins * first_arm_ins = T_ARM_INS(BBL_INS_FIRST(corner));
	      t_arm_ins * last_arm_ins = T_ARM_INS(BBL_INS_LAST(corner));
	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));

	      if (BBL_IS_HELL(CFG_EDGE_TAIL(taken))) continue;
	      if (BBL_IS_HELL(CFG_EDGE_TAIL(fall))) continue;
	      if (BBL_SUCC_FIRST(corner)!=BBL_SUCC_LAST(corner)) continue;
	      if (BBL_PRED_FIRST(corner)!=BBL_PRED_LAST(corner)) continue;
	      if (CFG_EDGE_TAIL(BBL_SUCC_FIRST(corner))!=CFG_EDGE_TAIL(taken)) continue;
	      if (!first_arm_ins) continue;
	      if (ARM_INS_ATTRIB(first_arm_ins) & IF_CONDITIONAL) continue;
	      
	      if (BBL_NINS(corner)>2) continue;
	      if (BBL_NINS(corner)==2 && ARM_INS_TYPE(last_arm_ins)!=IT_BRANCH) continue;
	      
#ifdef DEBUG_TRIANGLE
	      VERBOSE(0,("TRIANGLE SECOND upper @ieB\n corner @ieB\n down @ieB\n",upper,corner,CFG_EDGE_TAIL(taken)));
	      sprintf(x,"triangle2_pre%d.dot",++teller);
	      sprintf(y,"triangle2_post%d.dot",teller);
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),x);
#endif
	      
	      ins_new = ArmInsDup(first_arm_ins);
	      
	      if (ARM_INS_TYPE(first_arm_ins)!=IT_BRANCH)
		{
		  ARM_INS_SET_CONDITION(ins_new,  ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
		  ARM_INS_SET_ATTRIB(ins_new, ARM_INS_ATTRIB(ins_new)|IF_CONDITIONAL);
		  ARM_INS_SET_REGS_USE(ins_new,  ArmUsedRegisters(ins_new));
		  ARM_INS_SET_REGS_DEF(ins_new,  ArmDefinedRegisters(ins_new));
		}

	      ArmInsAppendToBbl(ins_new,upper);		  
	      
	      if (last_arm_ins!=first_arm_ins)
		{
		  ins_new = ArmInsDup(last_arm_ins);
		  ArmInsAppendToBbl(ins_new,upper);		  
		}
	      
	      CFG_EDGE_SET_EXEC_COUNT(taken,  CFG_EDGE_EXEC_COUNT(taken)+CFG_EDGE_EXEC_COUNT(fall));

	      if (CFG_EDGE_CORR(fall))
		{
		  CfgEdgeKill(CFG_EDGE_CORR(fall));
		}
	      CfgEdgeKill(fall);

	      if(CFG_EDGE_CORR(BBL_SUCC_FIRST(corner))) 
		CfgEdgeKill(CFG_EDGE_CORR(BBL_SUCC_FIRST(corner)));

	      CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(corner)));

	      if (ARM_INS_TYPE(last_arm_ins)!=IT_BRANCH)
		{
		  CFG_EDGE_SET_CAT(taken, ET_FALLTHROUGH); 
		}

	      ArmInsKill(branch_ins);
	      if (last_arm_ins!=first_arm_ins)
		ArmInsKill(last_arm_ins);
	      ArmInsKill(first_arm_ins);
	      BblKill(corner);

	      BBL_SET_REGS_LIVE_OUT(upper, BBL_INS_FIRST(CFG_EDGE_TAIL(taken))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(taken))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(taken)));
			

#ifdef DEBUG_TRIANGLE
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),y);
#endif
	    }
	}
    }

  /* for returns on fallthrough */

  CFG_FOREACH_BBL(cfg,upper)
    {
#ifdef DEBUG_TRIANGLE
      if (!(teller>=global_options.debugcounter))
#endif
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);

	      t_bbl * corner = CFG_EDGE_TAIL(fall);
	      t_bbl * lower = CFG_EDGE_TAIL(taken);
	      t_arm_ins * first_arm_ins = T_ARM_INS(BBL_INS_FIRST(corner));
	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));

	      if (BBL_IS_HELL(CFG_EDGE_TAIL(taken))) continue;
	      if (BBL_IS_HELL(CFG_EDGE_TAIL(fall))) continue;
	      if (BBL_SUCC_FIRST(corner)!=BBL_SUCC_LAST(corner)) continue;
	      if (BBL_SUCC_FIRST(lower)!=BBL_SUCC_LAST(lower)) continue;
	      if (CFG_EDGE_CAT(BBL_SUCC_FIRST(lower))!=ET_JUMP) continue;
	      if (BBL_PRED_FIRST(corner)!=BBL_PRED_LAST(corner)) continue;
	      if (BBL_PRED_FIRST(lower)!=BBL_PRED_LAST(lower)) continue;
	      if (CFG_EDGE_TAIL(BBL_SUCC_FIRST(corner))!=FUNCTION_BBL_LAST(BBL_FUNCTION(corner))) continue;
	      if (!first_arm_ins) continue;
	      if (ARM_INS_ATTRIB(first_arm_ins) & IF_CONDITIONAL) continue;
	      if (BBL_NINS(corner)>1) continue;
	      

#ifdef DEBUG_TRIANGLE
	      VERBOSE(0,("TRIANGLE FOURTH upper @ieB\n corner @ieB\n down @ieB\n",upper,corner,CFG_EDGE_TAIL(taken)));
	      sprintf(x,"triangle3_pre%d.dot",++teller);
	      sprintf(y,"triangle3_post%d.dot",teller);	      
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),x);
#endif
	      /*	      VERBOSE(0,("FOURTH upper @ieB\n corner @ieB\n down @ieB\n",upper,corner,CFG_EDGE_TAIL(taken)));*/
	      /*	      FunctionDrawGraph(BBL_FUNCTION(upper),"before.dot");*/
	      ins_new = ArmInsDup(first_arm_ins);
	      
	      ARM_INS_SET_CONDITION(ins_new,  ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
	      ARM_INS_SET_ATTRIB(ins_new, ARM_INS_ATTRIB(ins_new)|IF_CONDITIONAL);
	      ARM_INS_SET_REGS_USE(ins_new,  ArmUsedRegisters(ins_new));
	      ARM_INS_SET_REGS_DEF(ins_new,  ArmDefinedRegisters(ins_new));
	      ArmInsAppendToBbl(ins_new,upper);		  
	      
	      CfgEdgeKill(fall);
	      CfgEdgeKill(T_CFG_EDGE(BBL_SUCC_FIRST(corner)));
	      
	      CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate(cfg,upper,FUNCTION_BBL_LAST(BBL_FUNCTION(upper)),ET_JUMP), BBL_EXEC_COUNT(corner));

	      if (CFG_EDGE_CAT(taken)==ET_JUMP)
		CFG_EDGE_SET_CAT(taken, ET_FALLTHROUGH); 
	      else
		CFG_EDGE_SET_CAT(taken, ET_IPFALLTHRU); 
		
	      
	      ArmInsKill(branch_ins);
	      ArmInsKill(first_arm_ins);
	      BblKill(corner);

	      BBL_SET_REGS_LIVE_OUT(upper, BBL_INS_FIRST(CFG_EDGE_TAIL(taken))?InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(taken))):BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(taken)));
	      /*	      FunctionDrawGraph(BBL_FUNCTION(upper),"after.dot");*/

#ifdef DEBUG_TRIANGLE
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),y);
#endif
	    }
	}
    }



  CFG_FOREACH_BBL(cfg,upper)
    {
#ifdef DEBUG_TRIANGLE
      if (!(teller>=global_options.debugcounter))
#endif
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);

	      if (BBL_IS_HELL(CFG_EDGE_TAIL(taken))) continue;
	      if (BBL_IS_HELL(CFG_EDGE_TAIL(fall))) continue;

	      if (CFG_EDGE_TAIL(taken)==CFG_EDGE_TAIL(fall))
		{

#ifdef DEBUG_TRIANGLE
		  VERBOSE(0,("TRIANGLE THIRD upper @iB\n",upper));
		  sprintf(x,"triangle4_pre%d.dot",++teller);
		  sprintf(y,"triangle4_post%d.dot",teller);	      
		  FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),x);
#endif
		  CFG_EDGE_SET_EXEC_COUNT(fall,  CFG_EDGE_EXEC_COUNT(fall)+CFG_EDGE_EXEC_COUNT(taken));
		  InsKill(BBL_INS_LAST(upper));
		  CfgEdgeKill(taken);
#ifdef DEBUG_TRIANGLE
		  FunctionDrawGraph(BBL_FUNCTION(upper),y);
#endif
		}
	    }
	}
    }

	      
}

void ReplaceRectangleWithConditionalIns(t_cfg * cfg)
{
  t_bbl * upper;
  t_ins * ins;

  CFG_FOREACH_BBL(cfg,upper)
    {
      if (BblEndWithConditionalBranch(upper))
	{
	  t_cfg_edge * taken = TakenPath(upper);
	  t_cfg_edge * fall  = FallThroughPath(upper);
    t_bbl * taken_bbl, * fall_bbl, * lower;
    t_cfg_edge * taken_back, * fall_back;

	  if (!taken || !fall) continue;
	  	  
	  taken_bbl = CFG_EDGE_TAIL(taken);
	  fall_bbl = CFG_EDGE_TAIL(fall);

	  if (!taken_bbl || !fall_bbl) continue;

	  taken_back = BBL_SUCC_FIRST(taken_bbl);
	  fall_back = BBL_SUCC_FIRST(fall_bbl);

	  if (!taken_back || !fall_back) continue;
	  
	  lower = CFG_EDGE_TAIL(taken_back);

	  if (taken_back!=BBL_SUCC_LAST(taken_bbl)) continue;
	  if (BBL_PRED_FIRST(taken_bbl)!=BBL_PRED_LAST(taken_bbl)) continue;
	  
	  if (fall_back!=BBL_SUCC_LAST(fall_bbl)) continue;
	  if (BBL_PRED_FIRST(fall_bbl)!=BBL_PRED_LAST(fall_bbl)) continue;


	  if (CFG_EDGE_CAT(fall_back) != ET_JUMP 
	      && CFG_EDGE_CAT(fall_back) != ET_IPJUMP
	      && CFG_EDGE_CAT(fall_back) != ET_IPFALLTHRU
	      && CFG_EDGE_CAT(fall_back) != ET_FALLTHROUGH
	      )
	    continue;

	  if (CFG_EDGE_CAT(taken_back) != ET_JUMP 
	      && CFG_EDGE_CAT(taken_back) != ET_IPJUMP
	      && CFG_EDGE_CAT(taken_back) != ET_IPFALLTHRU
	      && CFG_EDGE_CAT(taken_back) != ET_FALLTHROUGH
	      )
	    continue;

	  if (CFG_EDGE_TAIL(taken_back)!=CFG_EDGE_TAIL(fall_back)) continue;
	  
	  if (BBL_NINS(taken_bbl)>4) continue;
	  if (BBL_NINS(fall_bbl)>4) continue;

	  BBL_FOREACH_INS(taken_bbl,ins)
	    if (INS_IS_CONDITIONAL(ins)) break;
	  
	  if (ins) continue;

	  BBL_FOREACH_INS(fall_bbl,ins)
	    if (INS_IS_CONDITIONAL(ins)) break;
	  
	  if (ins) continue;

	  VERBOSE(10,("THINK I FOUND QUAD @ieB\n@ieB\n@ieB\n@ieB\n",upper,taken_bbl,fall_bbl,lower));

	}
    }
}


void
ArmCfgBranchSwitch(t_cfg * cfg)
{
  t_bbl * upper;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);
  CfgEstimateEdgeCounts (cfg);
  
  CFG_FOREACH_BBL(cfg,upper)
    {
      if (BblIsHot(upper))
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);
	      
	      t_bbl * taken_bbl = CFG_EDGE_TAIL(taken);
	      t_bbl * fall_bbl = CFG_EDGE_TAIL(fall);
	      
	      t_arm_ins * last_ins_taken = T_ARM_INS(BBL_INS_LAST(taken_bbl));
	      t_arm_ins * last_ins_fall = T_ARM_INS(BBL_INS_LAST(fall_bbl));

	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));

	      if (BBL_PRED_FIRST(fall_bbl)!=BBL_PRED_LAST(fall_bbl)) continue;
	      if (BBL_PRED_FIRST(taken_bbl)!=BBL_PRED_LAST(taken_bbl)) continue;
	      
	      if (BBL_SUCC_FIRST(fall_bbl)!=BBL_SUCC_LAST(fall_bbl)) continue;
	      if (BBL_SUCC_FIRST(taken_bbl)!=BBL_SUCC_LAST(taken_bbl)) continue;
	      
	      if (CFG_EDGE_TAIL(BBL_SUCC_FIRST(fall_bbl))!=CFG_EDGE_TAIL(BBL_SUCC_FIRST(taken_bbl)) && !BBL_IS_LAST(CFG_EDGE_TAIL(BBL_SUCC_FIRST(fall_bbl))) && !BBL_IS_LAST(CFG_EDGE_TAIL(BBL_SUCC_FIRST(taken_bbl)))) continue;
	      
	      if (CFG_EDGE_EXEC_COUNT(taken)>CFG_EDGE_EXEC_COUNT(fall) && !FtPath (taken_bbl, upper, NULL))
		{
		  t_uint32 tmp_type;

		  /*		  FunctionDrawGraph(BBL_FUNCTION(upper),"pre.dot");*/

		  /*		  VERBOSE(0,("1 CAN GAIN ON TEST-BRANCH @ieB\n@ieB\n@ieB\n",upper,fall_bbl,taken_bbl));*/

		  tmp_type = CFG_EDGE_CAT(taken);
		  CFG_EDGE_SET_CAT(taken, CFG_EDGE_CAT(fall));
		  CFG_EDGE_SET_CAT(fall, tmp_type);

		  ARM_INS_SET_CONDITION(branch_ins, ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
		  ARM_INS_SET_EXEC_COUNT(branch_ins, BBL_EXEC_COUNT(ARM_INS_BBL(branch_ins))-ARM_INS_EXEC_COUNT(branch_ins));
		  /*		  FunctionDrawGraph(BBL_FUNCTION(upper),"post.dot");*/

		}
	      
	      if (last_ins_taken && last_ins_fall && ARM_INS_TYPE(last_ins_taken)==IT_BRANCH && ARM_INS_TYPE(last_ins_fall)==IT_BRANCH)
		{
		  /*		  VERBOSE(0,("CAN GAIN ON BRANCH IN ELSE & THEN @ieB\n@ieB\n@ieB\n",upper,fall_bbl,taken_bbl));*/
		  continue;
		}

	      if (BBL_EXEC_COUNT(taken_bbl)>BBL_EXEC_COUNT(fall_bbl) && last_ins_taken && ARM_INS_TYPE(last_ins_taken)==IT_BRANCH && CFG_EDGE_TAIL(BBL_SUCC_FIRST(fall_bbl))==CFG_EDGE_TAIL(BBL_SUCC_FIRST(taken_bbl))) 
		{
		  t_uint32 tmp_type;
		  t_arm_ins * new_ins;
		  /*		  FunctionDrawGraph(BBL_FUNCTION(upper),"pre.dot");*/

		  /*		  VERBOSE(0,("CAN GAIN ON TAKEN @ieB\n@ieB\n@ieB\n",upper,fall_bbl,taken_bbl));*/
		  
		  new_ins = ArmInsDup(last_ins_taken);
		  ArmInsAppendToBbl(new_ins,fall_bbl);
		  ArmInsKill(last_ins_taken);
		  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(fall_bbl));

		  tmp_type = CFG_EDGE_CAT(BBL_SUCC_FIRST(taken_bbl));
		  CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(taken_bbl), CFG_EDGE_CAT(BBL_SUCC_FIRST(fall_bbl)));
		  CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(fall_bbl), tmp_type);

		  /*		  FunctionDrawGraph(BBL_FUNCTION(upper),"post.dot");*/

		}
	      
	      if (BBL_EXEC_COUNT(fall_bbl)>BBL_EXEC_COUNT(taken_bbl) && last_ins_fall && ARM_INS_TYPE(last_ins_fall)==IT_BRANCH && CFG_EDGE_TAIL(BBL_SUCC_FIRST(fall_bbl))==CFG_EDGE_TAIL(BBL_SUCC_FIRST(taken_bbl)))
		{
		  t_uint32 tmp_type;
		  t_arm_ins * new_ins;

		  /*		  VERBOSE(0,("CAN GAIN ON FALL @ieB\n@ieB\n@ieB\n",upper,fall_bbl,taken_bbl));*/


		  new_ins = ArmInsDup(last_ins_fall);
		  ArmInsAppendToBbl(new_ins,taken_bbl);
		  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(taken_bbl));

		  ArmInsKill(last_ins_fall);

		  tmp_type = CFG_EDGE_CAT(BBL_SUCC_FIRST(taken_bbl));
		  CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(taken_bbl), CFG_EDGE_CAT(BBL_SUCC_FIRST(fall_bbl)));
		  CFG_EDGE_SET_CAT(BBL_SUCC_FIRST(fall_bbl), tmp_type);

		  /*		  FunctionDrawGraph(BBL_FUNCTION(upper),"post.dot");*/
		}
	    }
	}
    }	      
}

void
ArmCfgBranchSwitch2(t_cfg * cfg)
{
  t_bbl * upper;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);
  CfgEstimateEdgeCounts (cfg);

  CFG_FOREACH_BBL(cfg,upper)
    {
      if (BblIsHot(upper))
	{
	  if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);
	      t_cfg_edge * edge;
	      t_bool flag_fallthrough = FALSE;
	      t_uint32 tmp_type;
	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));
		
	      if (CFG_EDGE_EXEC_COUNT(taken)/2<=CFG_EDGE_EXEC_COUNT(fall)) continue;
	      if (CfgEdgeIsInterproc(taken)) continue;
	      if (CfgEdgeIsInterproc(fall)) continue;
	      if (FtPath (CFG_EDGE_TAIL(taken), upper, NULL)) continue;
	      if (CFG_EDGE_TAIL(taken)==upper) continue;

	      BBL_FOREACH_PRED_EDGE(CFG_EDGE_TAIL(taken),edge)
		{
		  if (
		      CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || 
		      CFG_EDGE_CAT(edge)==ET_IPFALLTHRU || 
		      CFG_EDGE_CAT(edge)==ET_RETURN || 
		      CFG_EDGE_CAT(edge)==ET_UNKNOWN ||  
		      CFG_EDGE_CAT(edge)==ET_SWITCH)
		    {
		      flag_fallthrough = TRUE;
		      break;
		    }
		}
	      
	      if (flag_fallthrough) continue;

	      /*	      VERBOSE(0,("CAN 2 GAIN ON TEST-BRANCH @B\n",upper));*/
	      
	      /*VERBOSE(0,("CAN 2 GAIN ON TEST-BRANCH @ieB\n@ieB\n@ieB\n",upper,CFG_EDGE_TAIL(fall),CFG_EDGE_TAIL(taken)));
		FunctionDrawGraph(BBL_FUNCTION(upper),"before.dot");*/

	      tmp_type = CFG_EDGE_CAT(taken);
	      CFG_EDGE_SET_CAT(taken, CFG_EDGE_CAT(fall));
	      CFG_EDGE_SET_CAT(fall, tmp_type);
	      
	      ARM_INS_SET_CONDITION(branch_ins, ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
	      ARM_INS_SET_EXEC_COUNT(branch_ins, BBL_EXEC_COUNT(ARM_INS_BBL(branch_ins))-ARM_INS_EXEC_COUNT(branch_ins));

	      /*	      FunctionDrawGraph(BBL_FUNCTION(upper),"after.dot");

	      VERBOSE(0,("RESULT @ieB\n@ieB\n@ieB\n",upper,CFG_EDGE_TAIL(fall),CFG_EDGE_TAIL(taken)));*/

	    }
	}
    }	      
}

void
ArmCfgBranchSwitch3(t_cfg * cfg)
{
  t_bbl * upper;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.99);
  CfgEstimateEdgeCounts (cfg);
  
  CFG_FOREACH_BBL(cfg,upper)
  {
    if (BblIsHot(upper))
    {
      if (BblEndWithConditionalBranch(upper))
	    {
	      t_cfg_edge * taken = TakenPath(upper);
	      t_cfg_edge * fall  = FallThroughPath(upper);
	      t_cfg_edge * edge;
	      t_cfg_edge * safe;
	      t_bool flag_fallthrough = FALSE;
	      t_bbl * taken_succ = CFG_EDGE_TAIL(taken);
	      t_uint32 tmp_type;
	      t_int32 fall_through_edge_count = 0;
	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(upper));
	      t_int32 gain;
	      t_arm_ins * new_ins;
	      t_bbl * taken_succ2 = taken_succ;
	      t_bbl * taken_succ2_next = taken_succ;
        t_bbl * new_bbl;
        t_cfg_edge * edge_new;

	      if (CFG_EDGE_EXEC_COUNT(taken)<CFG_EDGE_EXEC_COUNT(fall)) continue;
	      if (CfgEdgeIsInterproc(taken)) continue;
	      if (CfgEdgeIsInterproc(fall)) continue;
	      if (FtPath (CFG_EDGE_TAIL(taken), upper, NULL)) continue;
	      if (CFG_EDGE_TAIL(taken)==upper) continue;

	      BBL_FOREACH_PRED_EDGE(taken_succ,edge)
        {
          if (CFG_EDGE_CAT(edge)==ET_RETURN 
            || CFG_EDGE_CAT(edge)==ET_UNKNOWN
            || CFG_EDGE_CAT(edge)==ET_SWITCH)
          flag_fallthrough = TRUE;
        }
        
        if (flag_fallthrough) continue;

	      while ((taken_succ2 = taken_succ2_next))
        {
          if (taken_succ2 == upper) break;
          taken_succ2_next = NULL;
          
          BBL_FOREACH_SUCC_EDGE(taken_succ2,edge)
          {
            if ( CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU )
            {
              taken_succ2_next = CFG_EDGE_TAIL(edge);
              break;
            }
            else if ((CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge)==ET_SWI) && CFG_EDGE_CORR(edge))
            {
              taken_succ2_next = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
              break;
            }
          }
          taken_succ2 = NULL;
        }
	      
	      if (taken_succ2) continue;
	      
        BBL_FOREACH_PRED_EDGE(taken_succ,edge)
        {
          if (CFG_EDGE_CAT(edge)==ET_RETURN 
            || CFG_EDGE_CAT(edge)==ET_FALLTHROUGH
            || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU
            || CFG_EDGE_CAT(edge)==ET_UNKNOWN
            || CFG_EDGE_CAT(edge)==ET_SWITCH)
          fall_through_edge_count += CFG_EDGE_EXEC_COUNT(edge);
        }
        
        if (fall_through_edge_count > CFG_EDGE_EXEC_COUNT(taken)) continue;
        
        gain = CFG_EDGE_EXEC_COUNT(taken) - CFG_EDGE_EXEC_COUNT(fall) - fall_through_edge_count;
        
        if (gain<BBL_EXEC_COUNT(upper)/4) continue;

	      /*VERBOSE(0,("CAN GAIN %d ON TEST-BRANCH @ieB\n",gain,upper));*/
	      /*FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),"before.dot");*/

	      tmp_type = CFG_EDGE_CAT(taken);
	      CFG_EDGE_SET_CAT(taken, CFG_EDGE_CAT(fall));
	      CFG_EDGE_SET_CAT(fall, tmp_type);
	      
	      ARM_INS_SET_CONDITION(branch_ins, ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
	      ARM_INS_SET_EXEC_COUNT(branch_ins, BBL_EXEC_COUNT(ARM_INS_BBL(branch_ins))-ARM_INS_EXEC_COUNT(branch_ins));

	      new_bbl = BblNew(cfg);
	      BBL_SET_EXEC_COUNT(new_bbl, fall_through_edge_count);
	      BblInsertInFunction(new_bbl,BBL_FUNCTION(upper));

	      edge_new = CfgEdgeCreate(cfg,new_bbl,taken_succ,ET_JUMP);
	      CFG_EDGE_SET_EXEC_COUNT(edge_new, BBL_EXEC_COUNT(new_bbl));


	      new_ins = ArmInsNewForBbl(new_bbl);
	      ArmInsMakeUncondBranch(new_ins);		  
	      ArmInsAppendToBbl(new_ins,new_bbl);
	      
	      ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));
	      
	      BBL_FOREACH_PRED_EDGE_SAFE(taken_succ,edge,safe)
        {
          if ((CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU) && CFG_EDGE_HEAD(edge) != upper)
          {
            t_cfg_edge * edge_new = CfgEdgeCreate(cfg,CFG_EDGE_HEAD(edge),new_bbl,CFG_EDGE_CAT(edge));
            CFG_EDGE_SET_EXEC_COUNT(edge_new,  CFG_EDGE_EXEC_COUNT(edge));
            /*VERBOSE(0,("MOVING EDGE @E\n TO @E\n",edge,edge_new));*/
            CfgEdgeKill(edge);
          }
        }
	      /*	      VERBOSE(0,("NEW BBL @ieB\n",new_bbl));*/
	      /*	      FunctionDrawGraphWithHotness(BBL_FUNCTION(upper),"after.dot");*/
      }
    }
  }	      
}

void
ArmCfgBranchSwitch4(t_cfg * cfg)
{
  t_bbl * lower;
  t_bbl * max_jmp_head;
  t_cfg_edge * pred,*max_jmp, *edge;
  t_int32 max_jmp_count;
  t_bbl * fallthrough_succ, * fallthrough_succ_next;
  t_cfg_edge * fallthrough_pred_return;
  t_cfg_edge * fallthrough_pred_fall;
  t_int32 fallthrough_pred_count;
  
  if (!diabloflowgraph_options.blockprofilefile)
    return;
  
  CfgComputeHotBblThreshold(cfg,0.97);
  CfgEstimateEdgeCounts (cfg);
  
  CFG_FOREACH_BBL(cfg,lower)
  {
    if(!BBL_FUNCTION(lower) || FunctionGetExitBlock(BBL_FUNCTION(lower)) == lower) continue;
    
    if (BblIsHot(lower))
    {
      max_jmp_count = 0;
      max_jmp = NULL;
      
      fallthrough_pred_count = 0;
      fallthrough_pred_return = NULL;
      fallthrough_pred_fall = NULL;

      BBL_FOREACH_PRED_EDGE(lower,pred)
      {
        if (CFG_EDGE_CAT(pred) == ET_JUMP || CFG_EDGE_CAT(pred)==ET_IPJUMP)
        {
          if (!FallThroughPath(CFG_EDGE_HEAD(pred)) && BBL_PRED_FIRST(CFG_EDGE_HEAD(pred)) && CFG_EDGE_CAT(BBL_PRED_FIRST(CFG_EDGE_HEAD(pred))) != ET_SWITCH)
          {
            if (CFG_EDGE_EXEC_COUNT(pred)>max_jmp_count)
            {
              max_jmp = pred;
              max_jmp_count = CFG_EDGE_EXEC_COUNT(pred);
            }
          }
        }
        else if (CFG_EDGE_CAT(pred) == ET_FALLTHROUGH || CFG_EDGE_CAT(pred) == ET_IPFALLTHRU)
        {
          fallthrough_pred_fall = pred;
          fallthrough_pred_count += CFG_EDGE_EXEC_COUNT(pred);
        }
        else if (CFG_EDGE_CAT(pred) == ET_RETURN)
        {
          fallthrough_pred_return = pred;
          fallthrough_pred_count += CFG_EDGE_EXEC_COUNT(pred);
        }
	      else if (CFG_EDGE_CAT(pred) == ET_SWITCH)
        {
          fallthrough_pred_count += BBL_EXEC_COUNT(lower);
        }   
      }	      
      
      if (max_jmp_count - fallthrough_pred_count < BBL_EXEC_COUNT(lower)/8) continue;
      
      max_jmp_head = CFG_EDGE_HEAD(max_jmp);
      
      fallthrough_succ_next = lower;
      
      while ((fallthrough_succ = fallthrough_succ_next))
      {
        if (fallthrough_succ == max_jmp_head) break;
        fallthrough_succ_next = NULL;
        BBL_FOREACH_SUCC_EDGE(fallthrough_succ,edge)
        {
          if ( CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU )
          {
            fallthrough_succ_next = CFG_EDGE_TAIL(edge);
            break;
          }
          else if ((CFG_EDGE_CAT(edge)==ET_CALL || CFG_EDGE_CAT(edge)==ET_SWI) && CFG_EDGE_CORR(edge))
          {
            fallthrough_succ_next = CFG_EDGE_TAIL(CFG_EDGE_CORR(edge));
            break;
          }
        }
        fallthrough_succ = NULL;
      }
	    
      if (fallthrough_succ) continue;

#if 0
      static int teller = 0;
      
      if (++teller>diablosupport_options.debugcounter)
        return;
#endif
      VERBOSE(10,("CAN %d %d @ieB\n",fallthrough_pred_count,max_jmp_count,lower));
      
      if (fallthrough_pred_return || fallthrough_pred_fall)
      {
        t_bbl * new_bbl = BblNew(cfg);
        t_cfg_edge * edge_new;
        t_arm_ins * new_ins;

        BBL_SET_EXEC_COUNT(new_bbl, fallthrough_pred_count);
        if (fallthrough_pred_fall)
          BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_HEAD(fallthrough_pred_fall)));
        else
          BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_HEAD(CFG_EDGE_CORR(fallthrough_pred_return))));

        if (fallthrough_pred_fall && CFG_EDGE_CAT(fallthrough_pred_fall)==ET_IPFALLTHRU)
        {
          edge_new = CfgEdgeCreate(cfg,new_bbl,lower,ET_IPJUMP);
          CFG_EDGE_SET_EXEC_COUNT(edge_new, BBL_EXEC_COUNT(new_bbl));
        }
        else
        {
          edge_new = CfgEdgeCreate(cfg,new_bbl,lower,ET_JUMP);
          CFG_EDGE_SET_EXEC_COUNT(edge_new, BBL_EXEC_COUNT(new_bbl));
        }
        
        new_ins = ArmInsNewForBbl(new_bbl);
        ArmInsMakeUncondBranch(new_ins);		  
        ArmInsAppendToBbl(new_ins,new_bbl);
        ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));
        
        if (fallthrough_pred_return)
        {
          t_cfg_edge * call_edge;

          if (CFG_EDGE_CAT(CFG_EDGE_CORR(fallthrough_pred_return)) == ET_SWI)
            FATAL(("implement me\n"));
          
          call_edge = CFG_EDGE_CORR(fallthrough_pred_return);
          edge_new = CfgEdgeCreateCall(cfg,CFG_EDGE_HEAD(call_edge),CFG_EDGE_TAIL(call_edge),new_bbl,CFG_EDGE_HEAD(fallthrough_pred_return));
          CFG_EDGE_SET_EXEC_COUNT(edge_new,  CFG_EDGE_EXEC_COUNT(call_edge));
          CFG_EDGE_SET_EXEC_COUNT(CFG_EDGE_CORR(edge_new),  CFG_EDGE_EXEC_COUNT(call_edge));
          CfgEdgeKill(fallthrough_pred_return);
          CfgEdgeKill(call_edge);
		  
          VERBOSE(10,("CAN WITH RETURN\n"));
        }

        if (fallthrough_pred_fall)
        {
          /*VERBOSE(0,("@ieB\n@ieB\n",CFG_EDGE_HEAD(fallthrough_pred_fall),max_jmp_head));*/
          
          if (CFG_EDGE_CORR(fallthrough_pred_fall))
          {
            CFG_EDGE_SET_CORR(edge_new,  CFG_EDGE_CORR(fallthrough_pred_fall));
            CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge_new),  edge_new);
            CFG_EDGE_SET_CORR(fallthrough_pred_fall, NULL);
          }
          
          edge_new = CfgEdgeCreate(cfg,CFG_EDGE_HEAD(fallthrough_pred_fall),new_bbl,ET_FALLTHROUGH);
          CFG_EDGE_SET_EXEC_COUNT(edge_new,  BBL_EXEC_COUNT(new_bbl));
          CfgEdgeKill(fallthrough_pred_fall);
          VERBOSE(10,("CAN WITH OTHER\n"));
        }
      }	      
      else 
        VERBOSE(10,("CAN WITHOUT FALLTHROUGH\n"));
	     
      if (CFG_EDGE_CAT(max_jmp) == ET_JUMP)
        CFG_EDGE_SET_CAT(max_jmp,  ET_FALLTHROUGH);
      else if (CFG_EDGE_CAT(max_jmp) == ET_IPJUMP)
        CFG_EDGE_SET_CAT(max_jmp,  ET_IPFALLTHRU);
      
      VERBOSE(10,("KILLED @I\n",BBL_INS_LAST(max_jmp_head)));
      
      InsKill(BBL_INS_LAST(max_jmp_head));
    }
  }
}

void
ArmCfgHoistConstantProducingCode(t_cfg * cfg)
{
  t_bbl * head, * tail;
  t_cfg_edge * edge;
  t_ins * ins, *jins;
  t_regset used_regs;
  t_bbl * i_bbl;
  t_loop * loop = NULL;
#ifdef DEBUG_HOISTING1
  static int teller = 0;
#endif

  CfgComputeHotBblThreshold(cfg, 0.95);
  CfgEstimateEdgeCounts (cfg);
 
 CFG_FOREACH_EDGE(cfg,edge)
  {
    if (
	CFG_EDGE_CAT(edge)!=ET_JUMP &&
	CFG_EDGE_CAT(edge)!=ET_FALLTHROUGH
       ) continue;
    head = CFG_EDGE_HEAD(edge);
    tail = CFG_EDGE_TAIL(edge);
    if (BBL_NINS(tail)==0) continue;
    if (BBL_EXEC_COUNT(tail)<=CFG_EDGE_EXEC_COUNT(edge)) continue;
    /*      loop = BBL_LOOP(tail); TODO:FIXME, fix all loop interface things because it has changed a lot */
/*    BBL_FOREACH_LOOP(tail,loopref,loop)*/
      CFG_FOREACH_LOOP(cfg,loop) if(LoopContainsBbl(loop,tail))
      if(BBL_FUNCTION(tail) == BBL_FUNCTION(LOOP_HEADER(loop)))
      {
	t_loopiterator * loopiter;
	if (!BblDominates(head,tail)) continue;
	if (LoopContainsBbl(loop,head)) continue;

	ins = BBL_INS_FIRST(tail);
	/* need to test for reloc */
	used_regs = RegsetNew();

	BBL_FOREACH_INS(tail,ins)
	{
	  if (INS_IS_CONSTS(ins))
	  {
	    if (RegsetIsEmpty(RegsetIntersect(used_regs,INS_REGS_DEF(ins))))
	      if (RegsetIsEmpty(INS_REGS_USE(ins)))
		break;
	  }
	  RegsetSetUnion(used_regs,INS_REGS_USE(ins));
	  RegsetSetUnion(used_regs,INS_REGS_DEF(ins));
	}

	if (!ins) continue;


	/*      VERBOSE(0,("header @eiB\n",LOOP_HEADER(loop)));*/
	/*      VERBOSE(0,("back @E\n",LOOP_BACKEDGE(loop)));*/

	LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	  {
	    
	    if (BBL_IS_HELL(i_bbl)) goto end;
	    /*          if (BBL_FUNCTION(i_bbl) != BBL_FUNCTION(LOOP_HEADER)) break;*/
	    /*	  VERBOSE(0,("elem @B\n",LOOPELEM_BBL(elem))); */
	    BBL_FOREACH_INS(i_bbl,jins)
	      if (ins!=jins && !RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(jins),INS_REGS_DEF(ins))))
		goto end;
	    	      
	  }



end:
	Free(loopiter);
	if (i_bbl)
	  continue;

	/*#define DEBUG_HOISTING1*/
#ifdef DEBUG_HOISTING1
	if(teller < global_options.debugcounter)
	{
	  teller++;
#endif
	VERBOSE(10,("important candidate @I\n for code hoisting1  out of @iB\n",ins,tail));

	{
	  t_bbl * new_bbl = BblNew(cfg);
	  t_ins * new_ins;
	  t_cfg_edge * new_edge;
	  t_cfg_edge * new_edge2;

	  BblInsertInFunction(new_bbl,BBL_FUNCTION(tail));
	  BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(edge));

	  new_edge = CfgEdgeCreate(cfg,new_bbl,tail,CFG_EDGE_CAT(edge));
	  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(edge));
	  if (CFG_EDGE_CORR(edge))
	    FATAL(("OOPS @E\n",edge));
	  CfgEdgeKill(edge);
	  new_edge2 = CfgEdgeCreate(cfg,head,new_bbl,CFG_EDGE_CAT(new_edge));
	  CFG_EDGE_SET_EXEC_COUNT(new_edge2, CFG_EDGE_EXEC_COUNT(new_edge));

	  if (CFG_EDGE_CAT(new_edge)==ET_JUMP)
	  {
	    new_ins = InsNewForBbl(new_bbl);
	    ArmInsMakeUncondBranch(T_ARM_INS(new_ins));		  
	    InsAppendToBbl(new_ins,new_bbl);
	  }
	  new_ins = InsDup(ins);
	  InsPrependToBbl(new_ins,new_bbl);
	  InsKill(ins);

	  /*	  VERBOSE(0,("@ieB\n@ieB\n@ieB\n",head,new_bbl,tail));*/
	  

	}
#ifdef DEBUG_HOISTING1
	}
#endif
      }
  }
}

/*#define DEBUG_HOIST3*/

t_bool CfgHoistConstantProducingCode3(t_cfg * cfg)
{
  t_cfg_edge * edge, * safe;
  t_arm_ins * ins;
  t_loop * loop;
  t_regset free_regs, useable_regs;
  static t_bool done = FALSE;
  t_bbl * i_bbl;

#ifdef DEBUG_HOIST3
  static int teller = 0;
  t_string x = malloc(100);
  t_string y = malloc(100);
#endif
  
  if (diabloflowgraph_options.blockprofilefile)
    {
      CfgComputeHotBblThreshold(cfg, 0.95);
      CfgEstimateEdgeCounts (cfg);
    }

  CFG_FOREACH_LOOP(cfg,loop)
    {
      t_bbl * header;
      t_bbl * preheader;
      t_reg free_reg;
      t_arm_ins * new_ins;
      t_bool contains_exit_call = FALSE;
      t_loopiterator * loopiter;
      t_int32 in_count = 0;

      header = T_BBL(LOOP_HEADER(loop));

      if (diabloflowgraph_options.blockprofilefile && !BblIsHot(header)) continue;
      
      LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	{
	  BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	    if (CfgEdgeIsInterproc(edge) || CFG_EDGE_CAT(edge)==ET_SWITCH || BBL_IS_LAST(CFG_EDGE_TAIL(edge))) contains_exit_call = TRUE;
	  BBL_FOREACH_PRED_EDGE(i_bbl,edge)
	    if (CfgEdgeIsInterproc(edge) || CFG_EDGE_CAT(edge)==ET_SWITCH || BBL_IS_LAST(CFG_EDGE_TAIL(edge))) contains_exit_call = TRUE;
	}
      Free(loopiter);


      LOOP_FOREACH_ENTRY_EDGE(loop,edge)
	in_count += CFG_EDGE_EXEC_COUNT(edge);

      if (in_count>BBL_EXEC_COUNT(header)/3) continue;

      LOOP_FOREACH_EXIT_EDGE_SAFE(loop,loopiter,i_bbl,edge,safe)      
	{
	  if (CFG_EDGE_CAT(edge)==ET_CALL) contains_exit_call = TRUE;
	}
      Free(loopiter);

      if (contains_exit_call) continue;

      do 
	{
	  t_loopiterator * loopiter;
	  /*	  t_bool dominates_all_exits = TRUE;*/

	  ins = T_ARM_INS(LoopContainsConstantProducers(cfg,loop));
	  
	  if (!ins) break;
	  if(ARM_INS_IMMEDIATE(ins) == 0) break;
	  free_regs = LoopFreeRegisters(cfg,loop);
	  

	  
	  RegsetSetDup(useable_regs,free_regs);
	  RegsetSetSubReg(useable_regs,ARM_REG_R13);
	  
	  if (RegsetIsEmpty(useable_regs)) break;
	  
	  if (!RegsetIn(free_regs,ARM_REG_R13)) break;
	  
	  /*	  FunctionDrawGraph(BBL_FUNCTION(header),"file.dot");*/

	  //	  VERBOSE(0,("HOIST @I @B\n",ins,INS_BBL(ins)));

	  /*	  LOOP_FOREACH_EXIT_EDGE_SAFE(loop,loopiter,i_bbl,edge,safe)
	    if (!BblDominates(INS_BBL(ins),CFG_EDGE_HEAD(edge)) && !RegsetIsEmpty(RegsetIntersect(BblRegsLiveBefore(CFG_EDGE_TAIL(edge)),INS_REGS_DEF(ins))))
	      {
		VERBOSE(0,("HOIST OOPS\n"));
		FunctionDrawGraphWithHotness(BBL_FUNCTION(INS_BBL(ins)),"not.dot");
		dominates_all_exits = FALSE;
		break;
	      }
	  if (!dominates_all_exits) break;
	  */


#ifdef DEBUG_HOIST3
	  if (teller>=global_options.debugcounter) return FALSE;
	  sprintf(x,"hoist3_pre%d.dot",++teller);
	  sprintf(y,"hoist3_post%d.dot",teller);
	  
	  FunctionDrawGraphWithHotness(BBL_FUNCTION(INS_BBL(ins)),x);
#endif

	  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	    VERBOSE(10,("LOOP BBL @ieB\n",i_bbl));


	  preheader = LoopAddPreheader(cfg,loop);

	  
	  REGSET_FOREACH_REG(useable_regs,free_reg) 
	    break;
	  
	  new_ins = ArmInsDup(ins);
	  ArmInsPrependToBbl(new_ins,preheader);
	  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(preheader));

	  ARM_INS_SET_REGA(new_ins, free_reg);
	  ARM_INS_SET_REGS_DEF(new_ins,  ArmDefinedRegisters(new_ins));
	  
	  ArmInsUnconditionalize(new_ins);

	  new_ins = ArmInsNewForBbl(preheader);
	  ArmInsMakeStr(new_ins,free_reg,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,TRUE,FALSE,TRUE);		  
	  ArmInsPrependToBbl(new_ins,preheader);
	  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(preheader));

	  NodeMarkInit();

	  LOOP_FOREACH_EXIT_EDGE_SAFE(loop,loopiter,i_bbl,edge,safe)
	    {
	      if (!BblIsMarked(CFG_EDGE_TAIL(edge)))
		{
		  t_bbl * new_target;

		  VERBOSE(0,("exit edge from @eiB\n to @eiB\n",CFG_EDGE_HEAD(edge),CFG_EDGE_TAIL(edge)));
		  
		  new_target = CreateNewTargetBlock(cfg,edge);
		  BblMark(new_target);
		  VERBOSE(0,("Created new bbl : @B",new_target));
		  new_ins = ArmInsNewForBbl(new_target);
		  ArmInsMakeLdr(new_ins,free_reg,ARM_REG_R13,ARM_REG_NONE,4,ARM_CONDITION_AL,FALSE,TRUE,FALSE);		  
		  ArmInsPrependToBbl(new_ins,new_target);	      
		  ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_target));
		}
	    }

	  Free(loopiter);
	  
	  ArmInsMakeMov(ins,ARM_INS_REGA(ins),free_reg,0,ARM_INS_CONDITION(ins));
	  ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));
	  ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
	  
	  /*	  FunctionDrawGraph(BBL_FUNCTION(header),"file.dot");*/

#ifdef DEBUG_HOIST3
	  FunctionDrawGraphWithHotness(BBL_FUNCTION(INS_BBL(ins)),y);
#endif
	  
	  done = TRUE;
	  return TRUE;
	} while (TRUE);
    }

  return FALSE;
}

t_regset
LoopFreeRegisters(t_cfg * cfg, t_loop * loop)
{
  t_bbl * bbl;
  t_regset regs = RegsetNew();
  t_loopiterator * loopiter;

  RegsetSetDup(regs,CFG_DESCRIPTION(cfg)->int_registers);
  RegsetSetDiff(regs,CFG_DESCRIPTION(cfg)->always_live);

  LOOP_FOREACH_BBL(loop,loopiter,bbl)
  {
    RegsetSetDiff(regs,BblRegsMaybeDef(bbl));
    RegsetSetDiff(regs,BblRegsUse(bbl));
  }
  Free(loopiter);
  return regs;
}

t_bbl *
LoopAddPreheader(t_cfg * cfg,t_loop * loop)
{
  t_cfg_edge * edge;
  t_bbl * header = LOOP_HEADER(loop);
  t_bbl * fallthrough_preheader;
  t_bbl * new_preheader;
  t_bool is_fallthrough = FALSE;
  t_int32 exec_count = 0;
  t_int32 nr_entry_edges = 0;
  t_cfg_edge * new_edge;

  LOOP_FOREACH_ENTRY_EDGE(loop,edge)
    {
      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH || CFG_EDGE_CAT(edge)==ET_IPFALLTHRU)
	{
	  is_fallthrough = TRUE;
	  fallthrough_preheader = CFG_EDGE_HEAD(edge);
	}
      nr_entry_edges++;
      exec_count += CFG_EDGE_EXEC_COUNT(edge);
    }
  
  is_fallthrough = FALSE;

  new_preheader = BblNew(cfg);
  BblInsertInFunction(new_preheader,BBL_FUNCTION(header));
/*  VERBOSE(0,("New preheader @B",new_preheader));*/
  BBL_SET_EXEC_COUNT(new_preheader, exec_count);
  
  if (!is_fallthrough)
    {
      t_ins * new_ins = InsNewForBbl(new_preheader);
      ArmInsMakeUncondBranch(T_ARM_INS(new_ins));		  
      InsAppendToBbl(new_ins,new_preheader);
    }

  {  
    t_cfg_edge * edge;
    t_cfg_edge * safe;
    
    BBL_FOREACH_PRED_EDGE_SAFE(header,edge,safe)
      {
	t_cfg_edge * copy;
	if (LoopContainsBbl(loop,CFG_EDGE_HEAD(edge))) continue;
	copy = CfgEdgeCreate(cfg,CFG_EDGE_HEAD(edge),new_preheader,CFG_EDGE_CAT(edge));
	CFG_EDGE_SET_EXEC_COUNT(copy,  CFG_EDGE_EXEC_COUNT(edge));
	if (CFG_EDGE_CORR(edge))
	  FATAL(("OOPS EDGE KILL @E\n",edge));
	CfgEdgeKill(edge);
      }
  }
  
  new_edge = CfgEdgeCreate(cfg,new_preheader,header,is_fallthrough?ET_FALLTHROUGH:ET_JUMP);
  CFG_EDGE_SET_EXEC_COUNT(new_edge, exec_count);

  return new_preheader;
}

t_bbl *
CreateNewTargetBlock(t_cfg * cfg, t_cfg_edge * edge)
{
  t_bbl * new_bbl = BblNew(cfg);
  t_cfg_edge * new_edge;
  t_cfg_edge * new_edge2;

  BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));

  BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(edge));

  if (CFG_EDGE_CAT(edge)==ET_JUMP || CFG_EDGE_CAT(edge)==ET_IPJUMP)
    {
      t_ins * new_ins = InsNewForBbl(new_bbl);
      ArmInsMakeUncondBranch(T_ARM_INS(new_ins));		  
      InsAppendToBbl(new_ins,new_bbl);
    }

  new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(edge),CFG_EDGE_CAT(edge));
  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(edge));
  new_edge2 = CfgEdgeCreate(cfg,CFG_EDGE_HEAD(edge),new_bbl,CFG_EDGE_CAT(new_edge));
  CFG_EDGE_SET_EXEC_COUNT(new_edge2, CFG_EDGE_EXEC_COUNT(new_edge));
  if (CFG_EDGE_CORR(edge))
    FATAL(("OOPS EDGE KILL @E\n",edge));
  CfgEdgeKill(edge);
  
  return new_bbl;
}

t_ins * 
LoopContainsConstantProducers(t_cfg * cfg, t_loop * loop)
{
  t_bbl * i_bbl;
  t_ins * ins;
  t_loopiterator * loopiter;

  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    BBL_FOREACH_INS(i_bbl,ins)
      if (INS_IS_CONSTS(ins))
      {
	Free(loopiter);
        return ins;
      }

  Free(loopiter);
  return NULL;
  
}

void
LoopDetectStackSubAdds(t_cfg * cfg, t_loop* loop) 
{
  t_bbl * i_bbl;
  t_bbl *subbbl, * addbbl;
  t_arm_ins * ins, * subins = NULL, *addins = NULL;
  t_cfg_edge * edge;
  t_loopiterator * loopiter;

  
  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      if (BBL_FUNCTION(i_bbl)!=BBL_FUNCTION(LOOP_HEADER(loop))) continue;
      BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	if (CfgEdgeIsInterproc(edge))
	  {
	    if (CFG_EDGE_CAT(edge)==ET_CALL &&  CFG_DESCRIPTION(FUNCTION_CFG(BBL_FUNCTION(i_bbl)))->FunIsGlobal(BBL_FUNCTION(i_bbl)))
	      continue;
	    else
	    {
	      Free(loopiter);
	      return;
	    }
	  }
    }  
  Free(loopiter);
  
  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      if (BBL_FUNCTION(i_bbl)!=BBL_FUNCTION(LOOP_HEADER(loop))) continue;
      BBL_FOREACH_ARM_INS(i_bbl,ins)
	{
	  if (ARM_INS_OPCODE(ins)==ARM_SUB && ARM_INS_REGA(ins)==ARM_REG_R13 && ARM_INS_REGB(ins)==ARM_REG_R13 && ARM_INS_REGC(ins)==ARM_REG_NONE && !ArmInsUpdatesCond(ins) && !ArmInsIsConditional(ins))
	    subins = ins;
	  else if (ARM_INS_OPCODE(ins)==ARM_ADD && ARM_INS_REGA(ins)==ARM_REG_R13 && ARM_INS_REGB(ins)==ARM_REG_R13 && ARM_INS_REGC(ins)==ARM_REG_NONE && !ArmInsUpdatesCond(ins) && !ArmInsIsConditional(ins))
	    addins = ins;
	}
    }  
  Free(loopiter);

  if (!addins || !subins) return;

  if (ARM_INS_IMMEDIATE(addins)!=ARM_INS_IMMEDIATE(subins)) return;

  subbbl = ARM_INS_BBL(subins);
  addbbl = ARM_INS_BBL(addins);

  if (!BblDominates(subbbl,addbbl)) return;
  if (!BblPostdominates(addbbl,subbbl)) return;

  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      if (BBL_FUNCTION(i_bbl)!=BBL_FUNCTION(LOOP_HEADER(loop))) continue;
    }
  Free(loopiter);

  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      if (BBL_FUNCTION(i_bbl)!=BBL_FUNCTION(LOOP_HEADER(loop))) continue;
      if (!BblDominates(subbbl,i_bbl)) continue;
      if (!BblPostdominates(addbbl,i_bbl)) continue;

      if (i_bbl==subbbl) continue;
      if (i_bbl==addbbl) continue;
      
      BBL_FOREACH_ARM_INS(i_bbl,ins)
	{
	  if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R13) || RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13))
	  {
	    Free(loopiter);
	    return;
	  }
	}
    }
  Free(loopiter);

  ins = ARM_INS_INEXT(subins);
  while(ins)
    {
      if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R13)) return;
      if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13)) return;
      ins=ARM_INS_INEXT(ins);
    }

  ins = ARM_INS_IPREV(addins);
  while(ins)
    {
      if (RegsetIn(ARM_INS_REGS_USE(ins),ARM_REG_R13)) return;
      if (RegsetIn(ARM_INS_REGS_DEF(ins),ARM_REG_R13)) return;
      ins=ARM_INS_IPREV(ins);
    }
  
  VERBOSE(10,("found loop with bbls @iB\nand @iB\n",subbbl,addbbl));

  ArmInsKill(subins);
  ArmInsKill(addins);
}

void
DetectLoopStackSubAdds(t_cfg * cfg)
{
  t_loop * loop;
  t_bool inited_self = FALSE;

  if(!BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(dom_marking_number))
  {
    inited_self = TRUE;
    BblInitDomMarkingNumber(cfg);
  }

  CFG_FOREACH_LOOP(cfg,loop)
    if (!diabloflowgraph_options.blockprofilefile || BblIsHot(T_BBL(LOOP_HEADER(loop))))
      LoopDetectStackSubAdds(cfg,loop);

  if(inited_self) BblFiniDomMarkingNumber(cfg);
}

t_bool
LoopHeaderIsHotter(t_loop * loop)
{
  t_int32 count = 0;
  t_cfg_edge * edge;

  LOOP_FOREACH_ENTRY_EDGE(loop,edge)
    count += CFG_EDGE_EXEC_COUNT(edge);

  if (count<BBL_EXEC_COUNT(LOOP_HEADER(loop))/4)
    return TRUE;
  else
    return FALSE;
}

t_cfg_edge *
EdgeDup(t_cfg* cfg,t_cfg_edge * edge, t_bbl * head, t_bbl * tail)
{
  t_cfg_edge * copy = CfgEdgeCreate(cfg,head,tail,CFG_EDGE_CAT(edge));
  CFG_EDGE_SET_EXEC_COUNT(copy, CFG_EDGE_EXEC_COUNT(edge));
  return copy;
}

t_bbl *
BblDupInFun(t_cfg * cfg,t_bbl * bbl)
{
  t_ins * ins;
  t_bbl * copy;

  copy = BblNew(cfg);

  BblInsertInFunction(copy,BBL_FUNCTION(bbl));

    /* duplicate the instructions in the basic block */
  BBL_FOREACH_INS(bbl,ins)
    {
      t_ins * inscopy = InsDup(ins);
      INS_SET_OLD_ADDRESS(inscopy,  INS_OLD_ADDRESS(ins));
      InsAppendToBbl(inscopy, copy);
    }

  BBL_SET_EXEC_COUNT(copy, BBL_EXEC_COUNT(bbl));

  BBL_SET_TMP(bbl,  copy);

  return copy;
}

t_bbl *
CreateNewTargetBlockWithBranch(t_cfg * cfg, t_cfg_edge * edge)
{
  t_bbl * new_bbl = BblNew(cfg);
  t_ins * new_ins;
  t_cfg_edge * new_edge;
  t_cfg_edge * new_edge2;

  BblInsertInFunction(new_bbl,BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
/*  VERBOSE(0,("New targetblock with branch @B",new_bbl));*/

  BBL_SET_EXEC_COUNT(new_bbl, CFG_EDGE_EXEC_COUNT(edge));

  new_ins = InsNewForBbl(new_bbl);
  ArmInsMakeUncondBranch(T_ARM_INS(new_ins));		  
  InsAppendToBbl(new_ins,new_bbl);

  new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
  CFG_EDGE_SET_EXEC_COUNT(new_edge, CFG_EDGE_EXEC_COUNT(edge));
  new_edge2 = CfgEdgeCreate(cfg,CFG_EDGE_HEAD(edge),new_bbl,CFG_EDGE_CAT(edge));
  CFG_EDGE_SET_EXEC_COUNT(new_edge2, CFG_EDGE_EXEC_COUNT(new_edge));
  if (CFG_EDGE_CORR(edge))
    FATAL(("OOPS EDGE KILL @E\n",edge));
  CfgEdgeKill(edge);
  
  return new_bbl;
}

t_bbl *
LoopDuplicate(t_cfg * cfg, t_loop * loop, t_bool share_counts, float ratio)
{
  t_bbl * copy_bbl;
  t_bbl * header = T_BBL(LOOP_HEADER(loop));
  t_bbl * copy_header;
  t_cfg_edge * edge, *copy_edge;
  t_bbl * i_bbl;
  t_loopiterator * loopiter;
  t_ins * i_ins;

  if (ratio>1.0)
    FATAL(("incorrect ratio %f\n",ratio));

  copy_header = BblDupInFun(cfg,header);

  if (share_counts)
    {
      BBL_SET_EXEC_COUNT(copy_header,   BBL_EXEC_COUNT(copy_header)*ratio);
      BBL_SET_EXEC_COUNT(header,  BBL_EXEC_COUNT(header)*(1-ratio));

      BBL_FOREACH_INS(header,i_ins)
	{
	  INS_SET_EXEC_COUNT(i_ins,    INS_EXEC_COUNT(i_ins)* (1-ratio));
	}
      
      BBL_FOREACH_INS(copy_header,i_ins)
	{
	  INS_SET_EXEC_COUNT(i_ins,   INS_EXEC_COUNT(i_ins)* ratio);
	}
    }

  LOOP_FOREACH_BBL_INTRAFUN(loop,loopiter,i_bbl)
    {
      if (i_bbl==header) continue;
      copy_bbl = BblDupInFun(cfg,i_bbl);
      if (share_counts)
	{
	  BBL_SET_EXEC_COUNT(copy_bbl,  BBL_EXEC_COUNT(copy_bbl)*ratio);
	  BBL_SET_EXEC_COUNT(i_bbl,  BBL_EXEC_COUNT(i_bbl)*(1-ratio));

	  BBL_FOREACH_INS(i_bbl,i_ins)
	    {
	      INS_SET_EXEC_COUNT(i_ins,    INS_EXEC_COUNT(i_ins) *( 1-ratio));
	    }
	  
	  BBL_FOREACH_INS(copy_bbl,i_ins)
	    {
	      INS_SET_EXEC_COUNT(i_ins,    INS_EXEC_COUNT(i_ins) * ratio);
	    }
	}
    }

  Free(loopiter);

  LOOP_FOREACH_BBL_INTRAFUN(loop,loopiter,i_bbl)
    {
      BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	{
	  t_bbl * tail = CFG_EDGE_TAIL(edge);
	  if (LoopContainsBbl(loop,tail) && BBL_FUNCTION(tail) == BBL_FUNCTION(LOOP_HEADER(loop)))
	  {
	    copy_edge = EdgeDup(cfg,edge,BBL_TMP(i_bbl),BBL_TMP(tail));
	  }
	  else
	    {
	      copy_edge = EdgeDup(cfg,edge,BBL_TMP(i_bbl),tail);
	      if (CFG_EDGE_CAT(edge)==ET_FALLTHROUGH)
		{
		  t_bbl * target;
		  if (share_counts)
		    {
		      CFG_EDGE_SET_EXEC_COUNT(copy_edge,   CFG_EDGE_EXEC_COUNT(copy_edge)*ratio);
		    }
		  target = CreateNewTargetBlockWithBranch(cfg,copy_edge);
		  copy_edge = T_CFG_EDGE(BBL_PRED_FIRST(target));
		}
	    }
	  if (share_counts)
	    {
	      CFG_EDGE_SET_EXEC_COUNT(copy_edge, CFG_EDGE_EXEC_COUNT(copy_edge)*ratio);
	      CFG_EDGE_SET_EXEC_COUNT(edge,  CFG_EDGE_EXEC_COUNT(edge)*(1-ratio));
	    }
	} 
    }
  Free(loopiter);
  return copy_header;
}

t_bool
InsInBblDependOnEqNe(t_ins * ins)
{
  t_arm_ins * ains=T_ARM_INS(ins);
  t_regset regs = ARM_INS_REGS_DEF(ains);

  ains = ARM_INS_INEXT(ains);

  while (ains)
    {
      if (ARM_INS_ATTRIB(ains) & IF_CONDITIONAL)
	if (ARM_INS_CONDITION(ains)==ARM_CONDITION_NE || ARM_INS_CONDITION(ains)==ARM_CONDITION_EQ)
	  return TRUE;
      /*      if (!RegsetIsEmpty(RegsetIntersect(ARM_INS_REGS_USE(ains),regs)))*/
      RegsetSetDiff(regs,ARM_INS_REGS_DEF(ains));
      ains = ARM_INS_INEXT(ains);
    }
  return FALSE;
}


/*#define DEBUG_INVARIANT_CMP*/

t_bool LoopExtractInvariantCMP(t_cfg * cfg, t_loop * loop, t_ins * ins)
{
  t_bbl * copy_header;
  t_bbl * split_block;
  t_bbl * preheader;
  t_bbl * copy_preheader;
  t_cfg_edge * copy_entry_edge;
  t_cfg_edge * edge;
  t_ins * new_ins;

  float ratio = 0.5;

  t_int32 count;
  t_ins * ins_new , * copy_ins;
  t_ins * i_ins;

#ifdef DEBUG_INVARIANT_CMP
  static   int counter = 0;
  t_string x = (t_string) malloc(100);
  t_string y = (t_string) malloc(100);

  if (counter>=global_options.debugcounter) return FALSE;
#endif

  if (!diabloflowgraph_options.blockprofilefile && !InsInBblDependOnEqNe(ins))
    {
      return FALSE;
    }

  if (!diabloflowgraph_options.blockprofilefile)
    {
      t_bbl * i_bbl;
      t_uint32 count = 0;
      t_loopiterator * loopiter;
      LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	count+=BBL_NINS(i_bbl);
      Free(loopiter);
      if (count>32)
	return FALSE;
    }

#ifdef DEBUG_INVARIANT_CMP
  sprintf(x,"invariant_cmp_pre%d.dot",++counter);
  sprintf(y,"invariant_cmp_post%d.dot",counter);
  
  FunctionDrawGraphWithHotness(BBL_FUNCTION(INS_BBL(ins)),x);

  VERBOSE(0,("LOOP INVARIANT INS @I in loop bbl @iB\n",ins,INS_BBL(ins)));
#endif

  i_ins = INS_INEXT(ins);
  while (i_ins)
    {
      if (ARM_INS_CONDITION(T_ARM_INS(i_ins)) == ARM_CONDITION_EQ)
	{
	  ratio = 1 - ((float)(INS_EXEC_COUNT(i_ins)))/((float)(BBL_EXEC_COUNT(INS_BBL(i_ins))));
	  break;
	}
      else if (ARM_INS_CONDITION(T_ARM_INS(i_ins)) == ARM_CONDITION_NE)
	{
	  ratio = ((float)(INS_EXEC_COUNT(i_ins)))/((float)(BBL_EXEC_COUNT(INS_BBL(i_ins))));
	  break;
	}
      
      if (ArmInsUpdatesCond(T_ARM_INS(i_ins)))
	break;
      i_ins = INS_INEXT(i_ins);
    }
  
  /*  printf("ratio %f\n",ratio);*/

  copy_header = LoopDuplicate(cfg,loop,TRUE, ratio);

  split_block = LoopAddPreheader(cfg,loop);
  preheader = LoopAddPreheader(cfg,loop);

  copy_preheader = BblDupInFun(cfg,preheader);


  copy_entry_edge = EdgeDup(cfg,T_CFG_EDGE(BBL_SUCC_FIRST(preheader)),copy_preheader,copy_header);

  BBL_SET_EXEC_COUNT(copy_preheader,  BBL_EXEC_COUNT(preheader));
  BBL_SET_EXEC_COUNT(preheader, BBL_EXEC_COUNT(preheader)*(1-ratio));
  BBL_SET_EXEC_COUNT(copy_preheader,  BBL_EXEC_COUNT(copy_preheader)*ratio);

  BBL_FOREACH_INS(copy_preheader,i_ins)
    INS_SET_EXEC_COUNT(i_ins, BBL_EXEC_COUNT(copy_preheader));

  BBL_FOREACH_INS(preheader,i_ins)
    INS_SET_EXEC_COUNT(i_ins, BBL_EXEC_COUNT(preheader));


  CFG_EDGE_SET_EXEC_COUNT(BBL_SUCC_FIRST(preheader), BBL_EXEC_COUNT(preheader));
  CFG_EDGE_SET_EXEC_COUNT(BBL_SUCC_FIRST(copy_preheader), BBL_EXEC_COUNT(copy_preheader));

  if (BBL_INS_LAST(preheader))
    InsKill(BBL_INS_LAST(split_block));

  InsAppendToBbl(new_ins = InsDup(ins),split_block);
  INS_SET_EXEC_COUNT(new_ins,  BBL_EXEC_COUNT(split_block));

  if (CFG_EDGE_CORR(BBL_SUCC_FIRST(split_block)))
    FATAL(("OOPS EDGE KILL @E\n",BBL_SUCC_FIRST(split_block)));
  CfgEdgeKill(BBL_SUCC_FIRST(split_block));

  edge = CfgEdgeCreate(cfg,split_block,preheader,ET_JUMP);
  CFG_EDGE_SET_EXEC_COUNT(edge, BBL_EXEC_COUNT(split_block)*(1-ratio));
  edge = CfgEdgeCreate(cfg,split_block,copy_preheader,ET_FALLTHROUGH);
  CFG_EDGE_SET_EXEC_COUNT(edge, BBL_EXEC_COUNT(split_block)*ratio);

  ins_new = InsNewForBbl(split_block);
  ArmInsMakeCondBranch(T_ARM_INS(ins_new), ARM_CONDITION_EQ);
  InsAppendToBbl(ins_new,split_block);
  INS_SET_EXEC_COUNT(ins,  1-ratio);


  count = 0;
  
  BBL_FOREACH_INS(INS_BBL(ins),copy_ins)
    {
      if (copy_ins==ins)
	break;
      count++;
    }
  
  BBL_FOREACH_INS(T_BBL(BBL_TMP(INS_BBL(ins))),copy_ins)
    {
      count--;
      if (count<0)
	break;
    }

  i_ins = INS_INEXT(ins);

  while (i_ins)
    {
      if (ARM_INS_CONDITION(T_ARM_INS(i_ins))==ARM_CONDITION_NE)
	{
	  t_ins * j_ins = INS_INEXT(i_ins);
	  if (ARM_INS_OPCODE(T_ARM_INS(i_ins))==ARM_B)
	    {
	      CFG_EDGE_SET_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins)), CFG_EDGE_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins)))+CFG_EDGE_EXEC_COUNT(TakenPath(INS_BBL(i_ins))));
	      CfgEdgeKill(TakenPath(INS_BBL(i_ins)));
	    }
	  InsKill(i_ins);
	  i_ins = j_ins;
	  continue;
	}
      else if (ARM_INS_CONDITION(T_ARM_INS(i_ins))==ARM_CONDITION_EQ)
	{
	  if (ARM_INS_OPCODE(T_ARM_INS(i_ins))==ARM_B)
	    {
	      CFG_EDGE_SET_EXEC_COUNT(TakenPath(INS_BBL(i_ins)),  CFG_EDGE_EXEC_COUNT(TakenPath(INS_BBL(i_ins)))+CFG_EDGE_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins))));
	      CfgEdgeKill(FallThroughPath(INS_BBL(i_ins)));
	    }
	  ArmInsUnconditionalize(T_ARM_INS(i_ins));
	  INS_SET_REGS_USE(i_ins, ArmUsedRegisters(T_ARM_INS(i_ins)));
	  INS_SET_REGS_DEF(i_ins, ArmDefinedRegisters(T_ARM_INS(i_ins)));
	}
      
      if (ArmInsUpdatesCond(T_ARM_INS(i_ins)))
	break;
    
      i_ins = INS_INEXT(i_ins);

    }

  i_ins = INS_INEXT(copy_ins);

  while (i_ins)
    {
      if (ARM_INS_CONDITION(T_ARM_INS(i_ins))==ARM_CONDITION_EQ)
	{
	  t_ins * j_ins = INS_INEXT(i_ins);
	  if (ARM_INS_OPCODE(T_ARM_INS(i_ins))==ARM_B)
	    {
	      CFG_EDGE_SET_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins)), CFG_EDGE_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins)))+CFG_EDGE_EXEC_COUNT(TakenPath(INS_BBL(i_ins))));
	      CfgEdgeKill(TakenPath(INS_BBL(i_ins)));
	    }
	  InsKill(i_ins);
	  i_ins = j_ins;
	  continue;
	}
      else if (ARM_INS_CONDITION(T_ARM_INS(i_ins))==ARM_CONDITION_NE)
	{
	  if (ARM_INS_OPCODE(T_ARM_INS(i_ins))==ARM_B)
	    {
	      CFG_EDGE_SET_EXEC_COUNT(TakenPath(INS_BBL(i_ins)),  CFG_EDGE_EXEC_COUNT(TakenPath(INS_BBL(i_ins)))+CFG_EDGE_EXEC_COUNT(FallThroughPath(INS_BBL(i_ins))));
	      CfgEdgeKill(FallThroughPath(INS_BBL(i_ins)));
	    }
	  ArmInsUnconditionalize(T_ARM_INS(i_ins));
	  INS_SET_REGS_USE(i_ins, ArmUsedRegisters(T_ARM_INS(i_ins)));
	  INS_SET_REGS_DEF(i_ins, ArmDefinedRegisters(T_ARM_INS(i_ins)));
	}
      
      if (ArmInsUpdatesCond(T_ARM_INS(i_ins)))
	break;
    
      i_ins = INS_INEXT(i_ins);
    }

#ifdef DEBUG_INVARIANT_CMP
  FunctionDrawGraphWithHotness(BBL_FUNCTION(INS_BBL(ins)),y);
#endif	      

  return TRUE;
}

/*#define DEBUG_INVARIANT_ORD*/
t_bool LoopExtractInvariantOrdinary(t_cfg * cfg, t_loop * loop, t_ins *ins)
{
  t_bbl * preheader;
  t_ins * new_ins;
#ifdef DEBUG_INVARIANT_ORD
  static   int counter = 0;
  t_string x = "INVARIANT_ORD_PREXXX.dot                    ";
  t_string y = "INVARIANT_ORD_POSTXXX.dot                   ";

  if (counter>=global_options.debugcounter) return FALSE;

#endif

#ifdef DEBUG_INVARIANT_ORD
  x = sDiabloPrint("INVARIANT_ORD_PRE%d.dot",++counter);
  y = sDiabloPrint("INVARIANT_ORD_POST%d.dot",counter);
  
  FunctionDrawGraph(BBL_FUNCTION(INS_BBL(ins)),x);

  VERBOSE(0,("LOOP INVARIANT INS @I in loop bbl @iB\n",ins,INS_BBL(ins)));
#endif
  preheader = LoopAddPreheader(cfg,loop);
  InsPrependToBbl(new_ins = InsDup(ins),preheader);
  INS_SET_EXEC_COUNT(new_ins, INS_EXEC_COUNT(ins)?BBL_EXEC_COUNT(preheader):0);
  InsKill(ins);

#ifdef DEBUG_INVARIANT_ORD
  FunctionDrawGraph(BBL_FUNCTION(INS_BBL(ins)),y);
#endif	  

  return TRUE;
}

t_bool
OptimizeLoopInvariantIns(t_cfg * cfg,t_loop * loop, t_ins * ins, t_bool is_single_defined)
{
  t_arm_ins * ains=T_ARM_INS(ins);
  t_bbl * header = T_BBL(LOOP_HEADER(loop));
  t_bbl * bbl = ARM_INS_BBL(ains);

  t_bbl * i_bbl;
  t_cfg_edge * exit_edge, *edge;
  t_bbl * head, * tail;
  t_regset live_on_exit;
  t_loopiterator * loopiter;

  /* extra check: must dominate all exit edges, or produced value may not be live on exit edge!!! */

  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
  {

    BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
      if (CfgEdgeIsInterproc(edge) || CFG_EDGE_CAT(edge)==ET_SWITCH || BBL_IS_LAST(CFG_EDGE_TAIL(edge)))
      {
	Free(loopiter);
	return FALSE;
      }

    BBL_FOREACH_PRED_EDGE(i_bbl,edge)
      if (CfgEdgeIsInterproc(edge) || CFG_EDGE_CAT(edge)==ET_SWITCH || BBL_IS_LAST(CFG_EDGE_TAIL(edge)))
      {
	Free(loopiter);
	return FALSE;
      }
  }
  Free(loopiter);

  if (!diabloflowgraph_options.blockprofilefile || (BblIsAlmostHot(bbl) && BblIsAlmostHot(header) && LoopHeaderIsHotter(loop)))
    if (ARM_INS_OPCODE(ains)!=ARM_CMP && !ArmIsControlflow(ains) && is_single_defined)
      {
	t_loopiterator * loopiter;
	LOOP_FOREACH_EXIT_EDGE(loop,loopiter,i_bbl,exit_edge)
	  {
	    head = CFG_EDGE_HEAD(exit_edge);
	    tail = CFG_EDGE_HEAD(exit_edge);

	    if (BblDominates(bbl,head)) continue;
	    
	    live_on_exit = BBL_INS_FIRST(tail)?InsRegsLiveBefore(BBL_INS_FIRST(tail)):BBL_REGS_LIVE_OUT(tail);
	    
	    if (!RegsetIsEmpty(RegsetIntersect(live_on_exit,ARM_INS_REGS_DEF(ains)))) 
	      {
		return FALSE;
	      }
	  }
	Free(loopiter);
      }
  
  if (!diabloflowgraph_options.blockprofilefile || (BblIsAlmostHot(bbl) && BblIsAlmostHot(header) && LoopHeaderIsHotter(loop)))
    {
      if ((ARM_INS_OPCODE(ains)==ARM_CMP || (ARM_INS_OPCODE(ains)==ARM_MOV && (ARM_INS_FLAGS(ains) & FL_S))) && (diabloflowgraph_options.blockprofilefile && BblIsHot(bbl)))
	{
	  if (diabloflowgraph_options.blockprofilefile)
	    {
	      return LoopExtractInvariantCMP(cfg,loop,T_INS(ains));
	    }
	}
      else if (is_single_defined && !ArmIsControlflow(ains))
	{
	  return LoopExtractInvariantOrdinary(cfg,loop,T_INS(ains));
	}
    }
  return FALSE;
}

void
LoopDetectInvariants(t_cfg * cfg, t_loop* loop)      
{
  t_regset changed_regs = RegsetNew();
  t_regset multiple_defined_regs = RegsetNew();
  t_bbl * i_bbl;
  t_ins * ins;
  t_cfg_edge * edge;
  t_loopiterator * loopiter;
  
  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      BBL_FOREACH_SUCC_EDGE(i_bbl,edge)
	if (CfgEdgeIsInterproc(edge) || CFG_EDGE_CAT(edge)==ET_SWITCH || BBL_IS_LAST(CFG_EDGE_TAIL(edge)))
	{
	  Free(loopiter);
	  return;
	}

      BBL_FOREACH_PRED_EDGE(i_bbl,edge)
	if (CFG_EDGE_CAT(edge)==ET_COMPENSATING || CFG_EDGE_CAT(edge)==ET_RETURN || CFG_EDGE_CAT(edge)==ET_SWITCH)
	{
	  Free(loopiter);
	  return;
	}
      
      BBL_FOREACH_INS(i_bbl,ins)
	{
	  if (!RegsetIsEmpty(RegsetIntersect(changed_regs,INS_REGS_DEF(ins))))
	    RegsetSetUnion(multiple_defined_regs,INS_REGS_DEF(ins));
	  RegsetSetUnion(changed_regs,INS_REGS_DEF(ins));
	}
    }
  Free(loopiter);
  
  /* THIS COMPUTATION IS OVERLY SIMPLE SO FAR */
  
  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
    {
      BBL_FOREACH_INS(i_bbl,ins)
	{
	  if ((INS_TYPE(ins)!=IT_LOAD) && (INS_TYPE(ins)!=IT_LOAD_MULTIPLE))
	    if ((INS_TYPE(ins)!=IT_STORE) && (INS_TYPE(ins)!=IT_STORE_MULTIPLE))
	      if ((INS_TYPE(ins)!=IT_FLT_STORE) && (INS_TYPE(ins)!=IT_FLT_LOAD))
		if (RegsetIsEmpty(RegsetIntersect(INS_REGS_USE(ins),changed_regs)))
		  if (OptimizeLoopInvariantIns(cfg,loop,ins,RegsetIsEmpty(RegsetIntersect(INS_REGS_DEF(ins),multiple_defined_regs)))) 
		    {
		      Free(loopiter);
		      return;
		    }
	}
    }
  Free(loopiter);
}

void
DetectLoopInvariants(t_cfg * cfg)
{
  t_loop * loop;
  t_bool inited_self = FALSE;

  if (diabloflowgraph_options.blockprofilefile)
    {  
      CfgComputeHotBblThreshold(cfg, 0.80);
      CfgEstimateEdgeCounts (cfg);
    }

  if(!BBL_DYNAMIC_MEMBER_GLOBAL_ARRAY_INIT(dom_marking_number))
  {
    inited_self = TRUE;
    BblInitDomMarkingNumber(cfg);
  }
  CFG_FOREACH_LOOP(cfg,loop)
    LoopDetectInvariants(cfg,loop);
  if(inited_self) BblFiniDomMarkingNumber(cfg);
}

/*#define DEBUG_LOOP_UNROLLING*/

void
LoopUnrollingSimple(t_cfg * cfg)
{
  t_loop * loop;
  t_uint32 nins,nbbls;
  t_bbl * i_bbl;
  t_bbl * bbl;
  t_uint32 total_nins = 0;
  t_arm_ins * i_ins;
#ifdef DEBUG_LOOP_UNROLLING
  static int teller = 0;
#endif

  if (!diabloflowgraph_options.blockprofilefile)
    return;

  CfgEstimateEdgeCounts(cfg);

  CFG_FOREACH_BBL(cfg,bbl)
    total_nins+=BBL_NINS(bbl);

  CFG_FOREACH_LOOP(cfg,loop)
    {
      t_bbl * header = T_BBL(LOOP_HEADER(loop)); 
      t_loopiterator * loopiter;


      if (!BblIsHot(header)) continue;
      if (!LoopHeaderIsHotter(loop)) continue;
      if (LOOP_COUNT(loop)>4) continue;
      
      nins = nbbls = 0;
      
      LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	{
	  nins+=BBL_NINS(i_bbl);
	  nbbls++;
	}
      Free(loopiter);
      
      if (nins>total_nins/500) continue;
      if (nbbls>3) continue;

#ifdef DEBUG_LOOP_UNROLLING
      if (teller>=global_options.debugcounter) continue;
#endif

      if (nbbls==1)
	{
	  t_bbl * copy = BblDupInFun(cfg,header);
	  t_cfg_edge * fall = FallThroughPath(header);
	  t_cfg_edge * back_edge = TakenPath(header);
	  t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(header));

#ifdef DEBUG_LOOP_UNROLLING
	  teller++;
	  VERBOSE(0,("1 HOT LOOP UNROLLING CANDIDATE WITH %d ins and %d bbls @B\n",nins,nbbls,header));
	  FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"pre.dot");
#endif	 
	  
	  BBL_SET_EXEC_COUNT(copy,   BBL_EXEC_COUNT(copy)/2);
	  BBL_SET_EXEC_COUNT(header,  BBL_EXEC_COUNT(header)/2);
	  
	  CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate(cfg,copy,header,ET_JUMP), CFG_EDGE_EXEC_COUNT(back_edge)/2);
	  CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate(cfg,copy,CFG_EDGE_TAIL(fall),ET_FALLTHROUGH), CFG_EDGE_EXEC_COUNT(fall)/2);

	  BBL_FOREACH_ARM_INS(header,i_ins)
	    ARM_INS_SET_EXEC_COUNT(i_ins, ARM_INS_EXEC_COUNT(i_ins)/2);

	  BBL_FOREACH_ARM_INS(copy,i_ins)
	    ARM_INS_SET_EXEC_COUNT(i_ins, ARM_INS_EXEC_COUNT(i_ins)/2);

	  if (CFG_EDGE_CAT(fall)==ET_FALLTHROUGH)
	    CFG_EDGE_SET_CAT(fall, ET_JUMP);
	  else
	    CFG_EDGE_SET_CAT(fall, ET_IPJUMP);
	  
	  CFG_EDGE_SET_EXEC_COUNT(fall, CFG_EDGE_EXEC_COUNT(fall)/2);

	  CfgEdgeKill(back_edge);

	  CFG_EDGE_SET_EXEC_COUNT(CfgEdgeCreate(cfg,header,copy,ET_FALLTHROUGH), BBL_EXEC_COUNT(header)-CFG_EDGE_EXEC_COUNT(fall));
	  	  
	  ARM_INS_SET_CONDITION(branch_ins,  ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));

	  ARM_INS_SET_EXEC_COUNT(branch_ins,  BBL_EXEC_COUNT(ARM_INS_BBL(branch_ins))-ARM_INS_EXEC_COUNT(branch_ins));

#ifdef DEBUG_LOOP_UNROLLING
	  FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"post.dot");
#endif	 
	}
      else if (nbbls==2)
	{	
	  t_bool only_loop_header_exits = TRUE;
	  t_cfg_edge * exit_edge = NULL;
	  t_bbl * other_bbl = NULL;
	  t_uint32 nr_exit_edges = 0;
	  t_loopiterator * loopiter;

	  LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
	    {
	      if (i_bbl!=header && BBL_SUCC_FIRST(i_bbl)!=BBL_SUCC_LAST(i_bbl))
		{
		  only_loop_header_exits = FALSE;
		  /*		  printf("UNROLLING HOLA\n");*/
		}
	      if (i_bbl!=header)
		other_bbl = i_bbl;
	      /*	      VERBOSE(0,("@ieB\n",i_bbl));*/
	    }
	  Free(loopiter);
	  
	  LOOP_FOREACH_EXIT_EDGE(loop,loopiter,i_bbl,exit_edge)
	    nr_exit_edges++;

	  Free(loopiter);
	    
	  LOOP_FOREACH_EXIT_EDGE(loop,loopiter,i_bbl,exit_edge)
	    break;
	  Free(loopiter);
	  
	  if (only_loop_header_exits && CFG_EDGE_CAT(exit_edge)==ET_FALLTHROUGH)
	    {

#ifdef DEBUG_LOOP_UNROLLING
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"pre.dot");
	      VERBOSE(0,("2 HOT LOOP UNROLLING CANDIDATE WITH %d ins and %d bbls @B\n",nins,nbbls,header));
	      teller++;
#endif	 
	      t_bbl * copy_header = LoopDuplicate(cfg, loop, TRUE, 0.5);

	      t_cfg_edge * taken_edge = TakenPath(header);

	      t_cfg_edge * taken_edge_copy = TakenPath(copy_header);
	      t_cfg_edge * fall_edge_copy = FallThroughPath(copy_header);

	      t_uint32 tmp_type;

	      t_arm_ins * branch_ins = T_ARM_INS(BBL_INS_LAST(copy_header));
	      

	      CfgEdgeCreate(cfg,header,BBL_TMP(CFG_EDGE_TAIL(taken_edge)),ET_JUMP);
	      CfgEdgeKill(taken_edge);

	      tmp_type = CFG_EDGE_CAT(taken_edge_copy);
	      CFG_EDGE_SET_CAT(taken_edge_copy, CFG_EDGE_CAT(fall_edge_copy));
	      CFG_EDGE_SET_CAT(fall_edge_copy, tmp_type);

	      CfgEdgeKill(taken_edge_copy);
	      CfgEdgeCreate(cfg,copy_header,other_bbl,ET_FALLTHROUGH);

	      ARM_INS_SET_CONDITION(branch_ins, ArmInvertCondition(ARM_INS_CONDITION(branch_ins)));
	      ARM_INS_SET_EXEC_COUNT(branch_ins,  BBL_EXEC_COUNT(ARM_INS_BBL(branch_ins))-ARM_INS_EXEC_COUNT(branch_ins));
#ifdef DEBUG_LOOP_UNROLLING
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"post.dot");	      
#endif
	    }
	  else if (only_loop_header_exits && CFG_EDGE_CAT(exit_edge)==ET_JUMP)
	    {
	      t_loopiterator * loopiter;
	      VERBOSE(0,("OEPS 1 HOT LOOP UNROLLING CANDIDATE WITH %d ins and %d bbls @B\n",nins,nbbls,header));	      
	      LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
		{
		  VERBOSE(0,("@ieB\n",i_bbl));
		}
	      Free(loopiter);
	    }
	  else if (nr_exit_edges==1)
	    {
	      t_loopiterator * loopiter;
	      VERBOSE(0,("OEPS 2 HOT LOOP UNROLLING CANDIDATE WITH %d ins and %d bbls @B\n",nins,nbbls,header));	      
	      LOOP_FOREACH_BBL(loop,loopiter,i_bbl)
		{
		  VERBOSE(0,("@ieB\n",i_bbl));
		}    
	      Free(loopiter);
	    }
	  else 	    
	    {
	      t_bbl * copy_header;
	      t_bbl * copy_other_bbl;
	      t_cfg_edge * edge, * tmp_edge;

#ifdef DEBUG_LOOP_UNROLLING
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"pre.dot");
	      VERBOSE(0,("3 HOT LOOP UNROLLING CANDIDATE WITH %d ins and %d bbls and %d exit edges @B\n",nins,nbbls,nr_exit_edges,header));	      
	      teller++;
#endif	 
	      /*
	      LOOP_FOREACH_BBL(loop,i_bbl)
		{
		  VERBOSE(0,("@ieB\n",i_bbl));
		}
	      */
	      
	      copy_header = LoopDuplicate(cfg, loop, TRUE, 0.5);
	      copy_other_bbl = T_BBL(BBL_TMP(other_bbl));

	      BBL_FOREACH_SUCC_EDGE_SAFE(copy_other_bbl,edge,tmp_edge)
		{
		  if (CFG_EDGE_TAIL(edge)==copy_header)
		    {
		      /* Ludo: this code will not work anymore since loop inteface has changed thoroughly */
		      CfgEdgeCreate(cfg,BBL_TMP(other_bbl),header,CFG_EDGE_CAT(LOOP_BACKEDGES(loop)->edge));
		      CfgEdgeKill(edge);
		    }
		}
	      
	      /* Ludo: this code will not work anymore since loop inteface has changed thoroughly */
	      CfgEdgeCreate(cfg,other_bbl,copy_header,CFG_EDGE_CAT(LOOP_BACKEDGES(loop)->edge));
	      CfgEdgeKill(LOOP_BACKEDGES(loop)->edge);

#ifdef DEBUG_LOOP_UNROLLING
	      FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"post.dot");
#endif
	    }
	}
      else
	{
	  t_bbl * copy_header;
	  t_cfg_edge * succ;
	  t_cfg_edge * tmp;
	

	  VERBOSE(0,("LOOP HEADER WITH 3 blocks found @iB",header));

	  copy_header = LoopDuplicate(cfg, loop, TRUE, 0.5);

	  CfgEdgeCreate(cfg,CFG_EDGE_HEAD(LOOP_BACKEDGES(loop)->edge),copy_header,CFG_EDGE_CAT(LOOP_BACKEDGES(loop)->edge));
	  CfgEdgeCreate(cfg,BBL_TMP(CFG_EDGE_HEAD(LOOP_BACKEDGES(loop)->edge)),header,CFG_EDGE_CAT(LOOP_BACKEDGES(loop)->edge));
	  
	  BBL_FOREACH_SUCC_EDGE_SAFE(BBL_TMP(CFG_EDGE_HEAD(LOOP_BACKEDGES(loop)->edge)),succ,tmp)
	    if (CFG_EDGE_TAIL(succ)==copy_header)
	      CfgEdgeKill(succ);
	  CfgEdgeKill(LOOP_BACKEDGES(loop)->edge);
	  
	  FunctionDrawGraphWithHotness(BBL_FUNCTION(header),"post.dot");	      

	  //	  exit(0);
	  
	}
    }
}

t_bool
ExtractConditionalBundle(t_cfg * cfg, t_ins * ins, t_bool inverse)
{
  t_bbl * first_bbl = INS_BBL(ins);
  t_bbl * last_bbl;
  t_bbl * middle_bbl;
  t_bbl * new_bbl;
  t_bbl * return_bbl;
  t_arm_ins * last_ins = NULL;
  t_arm_ins * new_ins;
  t_arm_ins * tmp;
  t_arm_ins * jins;
  t_arm_ins * ains;
  t_arm_ins * first_ins = T_ARM_INS(ins);
  t_cfg_edge * new_edge;
  t_uint32 cond = ARM_INS_CONDITION(first_ins);
#ifdef DEBUG_COND_BUNDLES
  static int teller = 0;
  t_string x = (t_string) malloc(100);
  t_string y = (t_string) malloc(100);
#endif  
  jins = first_ins;

  while (jins)
    {
      if (ARM_INS_CONDITION(jins)==cond || ARM_INS_CONDITION(jins)==ArmInvertCondition(cond))
	last_ins = jins;
      else if (ARM_INS_IS_CONDITIONAL(jins))
	break;
      if (ArmInsUpdatesCond(jins))
	break;
      jins = ARM_INS_INEXT(jins);
    }


#ifdef DEBUG_COND_BUNDLES
  VERBOSE(0,("first ins @I\n",first_ins));
  VERBOSE(0,("last ins @I\n",last_ins));

  sprintf(x,"bundle_pre%d.dot",++teller);
  sprintf(y,"bundle_post%d.dot",teller);
  if (teller>=diablosupport_options.debugcounter) return FALSE;
  VERBOSE(0,("DOING @ieB\n@I\n",first_bbl,ins));
  FunctionDrawGraphWithHotness(BBL_FUNCTION(first_bbl),x);
#endif

  middle_bbl = BblSplitBlock(first_bbl,T_INS(first_ins),TRUE);
  last_bbl = BblSplitBlockNoTestOnBranches(middle_bbl,T_INS(last_ins),FALSE);
  new_bbl = BblDup(middle_bbl);

  BblInsertInFunction(new_bbl,BBL_FUNCTION(first_bbl));

  if (!inverse)
    {
      BBL_SET_EXEC_COUNT(new_bbl,  INS_EXEC_COUNT(BBL_INS_FIRST(middle_bbl)));
      BBL_SET_EXEC_COUNT(middle_bbl,  BBL_EXEC_COUNT(first_bbl) - BBL_EXEC_COUNT(new_bbl));					       

      new_ins = ArmInsNewForBbl(first_bbl);
      ArmInsMakeCondBranch(new_ins,cond);		  
      ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));
      ArmInsAppendToBbl(new_ins,first_bbl);

      CFG_EDGE_SET_EXEC_COUNT(BBL_SUCC_FIRST(first_bbl), BBL_EXEC_COUNT(middle_bbl));
      
      new_edge = CfgEdgeCreate(cfg,first_bbl,new_bbl,ET_JUMP);
      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));

      BBL_FOREACH_ARM_INS_SAFE(new_bbl,ains,tmp)
	{
	  ARM_INS_SET_EXEC_COUNT(ains,  BBL_EXEC_COUNT(new_bbl));
	  if (ARM_INS_CONDITION(ains)==cond)
	    ArmInsUnconditionalizer(ains);
	  else if (ARM_INS_CONDITION(ains)==ArmInvertCondition(cond))
	    ArmInsKill(ains);
	}
      
      BBL_FOREACH_ARM_INS_SAFE(middle_bbl,ains,tmp)
	{
	  ARM_INS_SET_EXEC_COUNT(ains,  BBL_EXEC_COUNT(middle_bbl));
	  if (ARM_INS_CONDITION(ains)==cond)
	    ArmInsKill(ains);
	  else if (ARM_INS_CONDITION(ains)==ArmInvertCondition(cond))
	    ArmInsUnconditionalizer(ains);
	}

      if (!BBL_INS_LAST(new_bbl) || !ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(new_bbl))))
	{
	  new_ins = ArmInsNewForBbl(new_bbl);
	  ArmInsMakeUncondBranch(new_ins);		  
	  ArmInsAppendToBbl(new_ins,new_bbl);
	  ARM_INS_SET_EXEC_COUNT(new_ins,  BBL_EXEC_COUNT(new_bbl));

	  new_edge = CfgEdgeCreate(cfg,new_bbl,last_bbl,ET_JUMP);
	  CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));
	}
      else
	{
	  t_cfg_edge * taken_edge = TakenPath(last_bbl);

	  if (CFG_EDGE_CAT(taken_edge) == ET_CALL)
	    {
	      t_bbl * callee   = CFG_EDGE_TAIL(taken_edge);
	      t_bbl * return_site = CFG_EDGE_CORR(taken_edge)?CFG_EDGE_TAIL(CFG_EDGE_CORR(taken_edge)):NULL;
	      t_bbl * exit_site   = CFG_EDGE_CORR(taken_edge)?CFG_EDGE_HEAD(CFG_EDGE_CORR(taken_edge)):NULL;
	      
	      return_bbl = BblNew(cfg);
	      BBL_SET_EXEC_COUNT(return_bbl,  BBL_EXEC_COUNT(new_bbl));
	      BblInsertInFunction(return_bbl,BBL_FUNCTION(new_bbl));

	      new_ins = ArmInsNewForBbl(return_bbl);
	      ArmInsMakeUncondBranch(new_ins);		  
	      ArmInsAppendToBbl(new_ins,return_bbl);
	      ARM_INS_SET_EXEC_COUNT(new_ins,  BBL_EXEC_COUNT(return_bbl));
	      
	      new_edge = CfgEdgeCreate(cfg,return_bbl,return_site,ET_JUMP);
	      CFG_EDGE_SET_EXEC_COUNT(new_edge,  BBL_EXEC_COUNT(return_bbl));

	      new_edge = CfgEdgeCreateCall(cfg,new_bbl,callee,return_bbl,exit_site);

	      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));

	      if (CFG_EDGE_CORR(new_edge))
		{
		  CFG_EDGE_SET_EXEC_COUNT(CFG_EDGE_CORR(new_edge), BBL_EXEC_COUNT(new_bbl));
		  CfgEdgeKill(CFG_EDGE_CORR(taken_edge));
		}

	      CfgEdgeKill(taken_edge);
	    }
	  else 
	    {
	      new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(taken_edge),CFG_EDGE_CAT(taken_edge));
	      if (CFG_EDGE_CAT(new_edge)==ET_IPJUMP)
		{
		  CFG_EDGE_SET_CORR(new_edge,  CFG_EDGE_CORR(taken_edge));
		  if (CFG_EDGE_CORR(new_edge))
		    CFG_EDGE_SET_CORR(CFG_EDGE_CORR(new_edge),  new_edge);
		  CFG_EDGE_SET_CORR(taken_edge, NULL);
		}
	      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));
	      CfgEdgeKill(taken_edge);
	    }
	}

      if (BBL_INS_LAST(middle_bbl) && ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(middle_bbl))))
	{
	  t_cfg_edge * taken_edge = TakenPath(last_bbl);
	  CfgEdgeKill(BBL_SUCC_FIRST(middle_bbl));
	  if (CFG_EDGE_CAT(taken_edge) == ET_CALL)
	    {
	      FATAL(("IMPLEMENT ME\n"));
	    }
	  else 
	    {
	      new_edge = CfgEdgeCreate(cfg,middle_bbl,CFG_EDGE_TAIL(taken_edge),CFG_EDGE_CAT(taken_edge));
	      if (CFG_EDGE_CAT(new_edge)==ET_IPJUMP)
		{
		  CFG_EDGE_SET_CORR(new_edge,  CFG_EDGE_CORR(taken_edge));
		  if (CFG_EDGE_CORR(new_edge))
		    CFG_EDGE_SET_CORR(CFG_EDGE_CORR(new_edge), new_edge);
		  CFG_EDGE_SET_CORR(taken_edge, NULL);
		}
	      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(middle_bbl));
	      CfgEdgeKill(taken_edge);
	    }
	}
    }
  else
    {
      BBL_SET_EXEC_COUNT(new_bbl,  INS_EXEC_COUNT(BBL_INS_FIRST(middle_bbl)));
      BBL_SET_EXEC_COUNT(middle_bbl,  BBL_EXEC_COUNT(first_bbl) - BBL_EXEC_COUNT(new_bbl));      

      new_ins = ArmInsNewForBbl(first_bbl);
      ArmInsMakeCondBranch(new_ins,ArmInvertCondition(cond));		  
      ARM_INS_SET_EXEC_COUNT(new_ins, BBL_EXEC_COUNT(new_bbl));
      ArmInsAppendToBbl(new_ins,first_bbl);

      CFG_EDGE_SET_EXEC_COUNT(BBL_SUCC_FIRST(first_bbl), BBL_EXEC_COUNT(middle_bbl));
      
      new_edge = CfgEdgeCreate(cfg,first_bbl,new_bbl,ET_JUMP);
      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));

      BBL_FOREACH_ARM_INS_SAFE(new_bbl,ains,tmp)
	{
	  ARM_INS_SET_EXEC_COUNT(ains,  BBL_EXEC_COUNT(new_bbl));
	  if (ARM_INS_CONDITION(ains)==cond)
	    ArmInsKill(ains);
	  else if (ARM_INS_CONDITION(ains)==ArmInvertCondition(cond))
	    ArmInsUnconditionalizer(ains);
	}
      
      BBL_FOREACH_ARM_INS_SAFE(middle_bbl,ains,tmp)
	{
	  ARM_INS_SET_EXEC_COUNT(ains,  BBL_EXEC_COUNT(middle_bbl));
	  if (ARM_INS_CONDITION(ains)==cond)
	    ArmInsUnconditionalizer(ains);
	  else if (ARM_INS_CONDITION(ains)==ArmInvertCondition(cond))
	    ArmInsKill(ains);
	}

      if (!BBL_INS_LAST(new_bbl) || !ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(new_bbl))))
	{
	  new_ins = ArmInsNewForBbl(new_bbl);
	  ArmInsMakeUncondBranch(new_ins);		  
	  ArmInsAppendToBbl(new_ins,new_bbl);
	  ARM_INS_SET_EXEC_COUNT(new_ins,  BBL_EXEC_COUNT(new_bbl));
	  
	  new_edge = CfgEdgeCreate(cfg,new_bbl,last_bbl,ET_JUMP);
	  CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));
	}
      else
	{
	  t_cfg_edge * taken_edge = TakenPath(last_bbl);
	  if (CFG_EDGE_CAT(taken_edge) == ET_CALL)
	    {
	      FATAL(("IMPLEMENT ME\n"));
	    }
	  else 
	    {
	      new_edge = CfgEdgeCreate(cfg,new_bbl,CFG_EDGE_TAIL(taken_edge),CFG_EDGE_CAT(taken_edge));
	      if (CFG_EDGE_CAT(new_edge)==ET_IPJUMP)
		{
		  CFG_EDGE_SET_CORR(new_edge,  CFG_EDGE_CORR(taken_edge));
		  if (CFG_EDGE_CORR(new_edge))
		    CFG_EDGE_SET_CORR(CFG_EDGE_CORR(new_edge), new_edge);
		  CFG_EDGE_SET_CORR(taken_edge, NULL);
		}
	      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(new_bbl));
	      CfgEdgeKill(taken_edge);
	    }
	}

      if (BBL_INS_LAST(middle_bbl) && ArmIsControlflow(T_ARM_INS(BBL_INS_LAST(middle_bbl))))
	{
	  t_cfg_edge * taken_edge = TakenPath(last_bbl);
	  CfgEdgeKill(BBL_SUCC_FIRST(middle_bbl));
	  if (CFG_EDGE_CAT(taken_edge) == ET_CALL)
	    {
	      FATAL(("IMPLEMENT ME\n"));
	    }
	  else 
	    {
	      new_edge = CfgEdgeCreate(cfg,middle_bbl,CFG_EDGE_TAIL(taken_edge),CFG_EDGE_CAT(taken_edge));
	      if (CFG_EDGE_CAT(new_edge)==ET_IPJUMP)
		{
		  CFG_EDGE_SET_CORR(new_edge,  CFG_EDGE_CORR(taken_edge));
		  if (CFG_EDGE_CORR(new_edge))
		    CFG_EDGE_SET_CORR(CFG_EDGE_CORR(new_edge), new_edge);
		  CFG_EDGE_SET_CORR(taken_edge, NULL);
		}
	      CFG_EDGE_SET_EXEC_COUNT(new_edge, BBL_EXEC_COUNT(middle_bbl));
	      CfgEdgeKill(taken_edge);
	    }
	}
    }
  
#ifdef DEBUG_COND_BUNDLES
  FunctionDrawGraphWithHotness(BBL_FUNCTION(first_bbl),y);
  VERBOSE(0,("DID IT @ieB\n@ieB\n@ieB\n@ieB\n",first_bbl,middle_bbl,new_bbl,last_bbl));
#endif  

  return TRUE;
}

void
DetectColdCodeBundles(t_cfg * cfg)
{
  t_bbl * bbl;
  t_ins * ins;

  if (!diabloflowgraph_options.blockprofilefile)
    return;

  CFG_FOREACH_BBL(cfg,bbl)
    {
    restart:
      if (BblIsHot(bbl))
	BBL_FOREACH_INS(bbl,ins)
	  {
	    t_uint32 size;
	    t_uint32 inverse_size;
	    t_int32 gain;
	    t_int32 loss;
	    t_int32 additional_gain;
	    t_int32 net_gain;
	    t_int32 inverse_gain;
	    t_int32 inverse_loss;
	    t_int32 inverse_additional_gain;
	    t_int32 inverse_net_gain;

	    if (ins==BBL_INS_LAST(bbl)) continue;

	    size = ConditionalBundleSize(ins);

	    if (size==0) continue;

	    inverse_size = InverseConditionalBundleSize(ins);
	   
	    gain = size*(BBL_EXEC_COUNT(bbl)-INS_EXEC_COUNT(ins));
	    loss = BBL_EXEC_COUNT(bbl) /* for inserting the branch */ 
	      + INS_EXEC_COUNT(ins) /* for missing the branch */ 
	      + 2 * INS_EXEC_COUNT(ins) /* for adding an unconditional branch and missing it */;
	    additional_gain = inverse_size * (INS_EXEC_COUNT(ins));
	    
	    
	    net_gain = gain-loss+additional_gain;
	    
	    inverse_gain = size*(BBL_EXEC_COUNT(bbl)-INS_EXEC_COUNT(ins));
	    inverse_loss = BBL_EXEC_COUNT(bbl) /* for inserting the branch */ 
	      + (BBL_EXEC_COUNT(bbl)-INS_EXEC_COUNT(ins)) /* for missing the branch */ 
	      + 2 * (BBL_EXEC_COUNT(bbl)-INS_EXEC_COUNT(ins)) /* for adding an unconditional branch and missing it */;
	    inverse_additional_gain = inverse_size * (INS_EXEC_COUNT(ins));
	    
	    inverse_net_gain = inverse_gain-inverse_loss+inverse_additional_gain;
	    
	    if (net_gain > inverse_net_gain && net_gain >= BBL_EXEC_COUNT(bbl)/8)
	      {
		VERBOSE(10,("size %d, %d: %d + %d - %d = %d original + additional bundle @ieB\n",size,inverse_size,gain,additional_gain,loss,net_gain,INS_BBL(ins)));
		if (ExtractConditionalBundle(cfg,ins,FALSE))
		  goto restart;
	      }
	    else if (inverse_net_gain > net_gain && inverse_net_gain >= BBL_EXEC_COUNT(bbl)/8)
	      {
		VERBOSE(10,("size %d, %d: %d + %d - %d = %d reverse bundle @ieB\n",size,inverse_size,inverse_gain,inverse_additional_gain,inverse_loss,inverse_net_gain,INS_BBL(ins)));
		if (ExtractConditionalBundle(cfg,ins,TRUE))
		  goto restart;
	      }

	  }
    }
}

