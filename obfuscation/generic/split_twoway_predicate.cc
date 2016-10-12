/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <iostream>

extern "C" {
#include <diabloanopt.h>
}

#include <obfuscation/obfuscation_architecture_backend.h>
#include "split_twoway_predicate.h"
#include "split_twoway_predicate_opt.h"

SplitWithTwoWayPredicateTransformation::SplitWithTwoWayPredicateTransformation() {
  AddOptionsListInitializer(obfuscation_split_twoway_predicate_option_list); SplitTwoWayPredicateOptInit();
  
  RegisterTransformationType(this, _name);
  
  bbls_split = 0;
}

static t_bool CanUnfoldBbl(const t_bbl * bbl)/*{{{*/
{
  t_cfg_edge * edge;
  if(BBL_NINS(bbl)==0)
    return FALSE;
  /*if(BBL_FACTORED(bbl))
    return FALSE; TODO?*/
  if(BBL_IS_HELL(bbl))
    return FALSE;
  BBL_FOREACH_SUCC_EDGE(bbl, edge) {
    if(CFG_EDGE_CAT(edge) == ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH)
      return FALSE;
    /*
      A Branch Function redirects the control to a location in the binary based on the return address. When we duplicate
      such a basic block, one of those computations will still be correct, but for the other one, since we cannot have
      2 fall-through paths to the same basic block, this doesn't work. It probably could get worked around so that it still
      works (TODO this), but for now this is easier and works too.
    */
    if(DisallowedFunctionToTransform(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))
      return FALSE;
    
    /* TODO: this was not needed on i386? */
    if (BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
      return FALSE;
  }
  
  /* We really do not want to rewrite BBLs coming out of switches This was not needed on x86, but this is needed for ARM and I think it is better to be safe than sorry. */
  BBL_FOREACH_PRED_EDGE(bbl, edge) {
    if(CFG_EDGE_CAT(edge) == ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH)
      return FALSE;
    /* Detect FT edges of switches, TODO: uniformize with ARM's canRedirectSuccessor */
    t_cfg_edge* edge2;
    t_bbl* head = CFG_EDGE_HEAD(edge);
    BBL_FOREACH_SUCC_EDGE(head, edge2) {
      if (CFG_EDGE_CAT(edge2) == ET_SWITCH || CFG_EDGE_CAT(edge2) == ET_IPSWITCH) {
        return FALSE;
      }
    }
  }

  if (BBL_REFED_BY(bbl)) {
    return FALSE;
  }

  return TRUE;
}/*}}}*/

static t_bool CanUnfoldBblByEdge(t_bbl * bbl, t_cfg_edge * edge)/*{{{*/
{
  if(BBL_IS_HELL(CFG_EDGE_HEAD(edge)) ||  CFG_EDGE_CAT(edge)==ET_SWITCH || CFG_EDGE_CAT(edge)==ET_IPSWITCH || CFG_EDGE_CAT(edge) == ET_COMPENSATING)
    return FALSE;
          
  if(!CanUnfoldBbl(bbl))
    return FALSE;
  
  return TRUE;
}/*}}}*/

/* bblOrig is duplicated, edgeToMove is an edge going to the soon to be duplicated bblOrig */
t_bbl* SplitWithTwoWayPredicateTransformation::unfoldBblByEdge(t_bbl * bblOrig, t_cfg_edge * edgeToMove)/*{{{*/
{
  t_bbl * bblCopy;
  t_cfg_edge * edge;
  t_cfg * cfg = BBL_CFG(bblOrig);

  if(!CanUnfoldBblByEdge(bblOrig, edgeToMove))
    FATAL(("@ieB, Not supported!",bblOrig));  

  bblCopy = BblDup(bblOrig);

  /*VERBOSE(0, ("unfolding @eiB using edge FROM @eiB TO @eiB", bblOrig, CFG_EDGE_HEAD(edgeToMove), CFG_EDGE_TAIL(edgeToMove)));
  VERBOSE(0, ("@eiB @eiB", bblCopy, bblOrig));*/
  
  /* Ensure that if we split a hot bbl, the split off bbls is still considered hot! */
  BBL_SET_EXEC_COUNT(bblCopy, BBL_EXEC_COUNT(bblOrig));
  BblInsertInFunction(bblCopy, BBL_FUNCTION(bblOrig));
  
  if(!CFG_EDGE_CORR(edgeToMove))
  {
    CfgUnlinkEdge(cfg, edgeToMove);
    CfgInitEdge(cfg, edgeToMove, CFG_EDGE_HEAD(edgeToMove), bblCopy,CFG_EDGE_CAT(edgeToMove));
  }
  else 
  {
    t_bbl * head_one;
    t_bbl * tail_one;
    t_bbl * head_corr;
    t_bbl * tail_corr;
    t_uint32 cat_one;
    t_uint32 cat_corr;
    if (CFG_EDGE_CAT(edgeToMove) == ET_CALL)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);
      head_corr = CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove));
      tail_corr = CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else if (CFG_EDGE_CAT(edgeToMove) == ET_RETURN)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);

      head_corr = CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove));
      tail_corr = CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else if (CFG_EDGE_CAT(edgeToMove) == ET_IPJUMP || CFG_EDGE_CAT(edgeToMove) == ET_IPFALLTHRU)
    {
      head_one = CFG_EDGE_HEAD(edgeToMove);
      tail_one = bblCopy;
      cat_one = CFG_EDGE_CAT(edgeToMove);

      head_corr = T_BBL(CFG_EDGE_HEAD(CFG_EDGE_CORR(edgeToMove)));
      tail_corr = T_BBL(CFG_EDGE_TAIL(CFG_EDGE_CORR(edgeToMove)));
      cat_corr = CFG_EDGE_CAT(CFG_EDGE_CORR(edgeToMove));
    }
    else
    {
      VERBOSE(0,("@E @ieB",edgeToMove,bblOrig));            
      VERBOSE(0,("%s",FUNCTION_NAME(BBL_FUNCTION(bblOrig))));
      VERBOSE(0,("%d",BBL_IS_HELL(bblOrig)));
      FATAL((""));
    }
    CfgUnlinkEdge(cfg, edgeToMove);
    CfgUnlinkEdge(cfg, CFG_EDGE_CORR(edgeToMove));
    CfgInitEdge(cfg, edgeToMove, head_one, tail_one, cat_one);
    CfgInitEdge(cfg, CFG_EDGE_CORR(edgeToMove), head_corr, tail_corr, cat_corr);
  }
  
  BBL_FOREACH_SUCC_EDGE(bblOrig,edge)
  {
    if(CFG_EDGE_CAT(edge) == ET_FALLTHROUGH || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
    {
      t_bbl * new_bbl = BblNew(cfg);

      t_ins * jump_ins;
      GetArchitectureInfo(bblOrig)->appendUnconditionalBranchInstruction(new_bbl);

      if(CFG_EDGE_CAT(edge) == ET_FALLTHROUGH)
        CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_JUMP);
      else if(CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      {
        t_cfg_edge * new_edge1 = CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(edge),ET_IPJUMP);
        if(CFG_EDGE_CORR(edge))
        {
          t_cfg_edge * new_edge2 =  CfgEdgeNew(cfg,CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)),CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),CFG_EDGE_CAT(CFG_EDGE_CORR(edge)));
          CFG_EDGE_SET_CORR(new_edge1,new_edge2);
          CFG_EDGE_SET_CORR(new_edge2,new_edge1);
        }
      }

      CfgEdgeNew(cfg,bblCopy,new_bbl,ET_FALLTHROUGH);
      if(BBL_FUNCTION(bblOrig))
        BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
    }
    else if (CFG_EDGE_CAT(edge) == ET_CALL)
    {
      t_bbl * new_bbl;
      t_ins * jump_ins;
      
      if(CFG_EDGE_CORR(edge))
      {
        new_bbl = BblNew(cfg); /* TODO: set liveness */

        GetArchitectureInfo(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)))->appendUnconditionalBranchInstruction(new_bbl);

        CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),ET_JUMP);
        CfgEdgeCreateCall(cfg,bblCopy,CFG_EDGE_TAIL(edge),new_bbl,FunctionGetExitBlock(BBL_FUNCTION(bblOrig)));

        if(BBL_FUNCTION(bblOrig))
          BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
      }
      else
      {
        CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),ET_CALL);
      }
    }
    else if (CFG_EDGE_CAT(edge) == ET_SWI) /* TODO: this might cause issues with fallthrough paths on Switch blocks, verify */
    {
      t_bbl * new_bbl;
      t_ins * jump_ins;
      
      if(CFG_EDGE_CORR(edge))
      {
        new_bbl = BblNew(cfg); /* TODO: liveness */
        
        GetArchitectureInfo(CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)))->appendUnconditionalBranchInstruction(new_bbl);

        CfgEdgeNew(cfg,new_bbl,CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),ET_JUMP);

        CfgEdgeCreateSwi(cfg,bblCopy,new_bbl);

        if(BBL_FUNCTION(bblOrig))
          BblInsertInFunction(new_bbl, BBL_FUNCTION(bblOrig));
      }
      else
      {
        CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),ET_SWI);
      }
    }
    else if (CFG_EDGE_CORR(edge))
    {
                t_cfg_edge * new_edge1, * new_edge2;
                if(CFG_EDGE_CAT(CFG_EDGE_CORR(edge))==ET_IPFALLTHRU)
                        continue;
                new_edge1 = CfgEdgeNew(cfg, bblCopy,CFG_EDGE_TAIL(edge),CFG_EDGE_CAT(edge));
                new_edge2 = CfgEdgeNew(cfg, CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)),CFG_EDGE_TAIL(CFG_EDGE_CORR(edge)),CFG_EDGE_CAT(CFG_EDGE_CORR(edge)));
                CFG_EDGE_SET_CORR(new_edge1,new_edge2);
                CFG_EDGE_SET_CORR(new_edge2,new_edge1);
    }
    else
    {
      CfgEdgeNew(cfg,bblCopy,CFG_EDGE_TAIL(edge),CFG_EDGE_CAT(edge));
    }
  }
  
  //VERBOSE(0, ("AFTER:\n@eiB @eiB", bblCopy, bblOrig));
  
  return bblCopy;
}/*}}}*/

/* We really don't want to split BBLs where the instructions are refered to by/refer to relocations.
   They might be used to generate offsets in obfuscations, and if we duplicate these instructions,
   it's possible that the duplicate's offset is incorrectly computed.
   TODO: we could split BEFORE all relocation-relevant instructions
*/
bool SplitWithTwoWayPredicateTransformation::canTwowaySplitBasicBlock(const t_bbl* bbl) const {
  t_ins* ins;
  BBL_FOREACH_INS(bbl, ins) {
    if (INS_REFED_BY(ins) || INS_REFERS_TO(ins)) {
      return false;
    }
  }
  if (BBL_REFED_BY(bbl) || BBL_REFERS_TO(bbl)) {
    return false;
  }
  return true;
}

bool SplitWithTwoWayPredicateTransformation::canTransform(const t_bbl* bbl) const {
  if (!obfuscation_split_twoway_predicate_options.twoway_predicates && !AllObfuscationsEnabled())
    return false;

  if (IS_DATABBL(bbl))
    return false;
  
  if (canTwowaySplitBasicBlock(bbl) && BBL_NINS(bbl)>0 && CanUnfoldBbl(bbl))
    return true;
  
  return false;
}


bool SplitWithTwoWayPredicateTransformation::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  bblSplitWithTwoWayOpaquePredicate(bbl, rng);
  
  unfoldBblByEdge(CFG_EDGE_TAIL(BBL_SUCC_FIRST(bbl)), BBL_SUCC_FIRST(bbl));
  
  bbls_split++;
  
  return true;
}

void SplitWithTwoWayPredicateTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sTwoWaySplit_Stats,bbls_split,%i", prefix.c_str(), bbls_split));
}

/* TODO: add a separate unfold transformation? */
