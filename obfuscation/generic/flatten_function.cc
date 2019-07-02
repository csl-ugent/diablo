/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* {{{ Copyright
 * Copyright 2001,2002: Bertrand Anckaert
 * 			Bruno De Bus
 *                      Bjorn De Sutter
 *                      Dominique Chanet
 *                      Matias Madou
 *                      Ludo Van Put
 *          2014 Bart Coppens
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
#include <obfuscation/obfuscation_architecture_backend.h>
#include "flatten_function.h"
#include "flatten_function_opt.h"
using namespace std;

LogFile* L_OBF_FF = NULL;

/* TODO: more_switch for double switch blocks! */

static t_bool CanBeFlattenedGeneric(const t_function * fun) /* {{{ */
{
  t_bbl * bbl;

  if (!fun)
    return FALSE;

  if(FUNCTION_IS_HELL(fun)) {
    return FALSE;
  }

  if (DisallowedFunctionToTransform(fun))
    return FALSE;

  return TRUE;
} /* }}} */

t_int32 FlattenFunctionTransformation::countBasicBlocks(t_function * fun)/* {{{ */
{
  t_int32 to_nr_bbl=0;
  t_bbl * bbl;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    if(canRedirectSuccessor(bbl))
    {
      to_nr_bbl++;
    }
  }

  return to_nr_bbl;
}/* }}} */

bool FlattenFunctionTransformation::canRedirectSuccessor(t_bbl* bbl) {
  t_cfg_edge * edge;

  if (! CanBeFlattenedGeneric(BBL_FUNCTION(bbl)))
    return false;

  if (!BBL_INS_FIRST(bbl))
    return false;
  
  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }

  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if(CFG_EDGE_CAT(edge)==ET_CALL ||
      CFG_EDGE_CAT(edge)==ET_RETURN ||
      CFG_EDGE_CAT(edge)==ET_COMPENSATING ||
      CFG_EDGE_CAT(edge)==ET_SWI ||
      CFG_EDGE_CAT(edge)==ET_IPJUMP ||
      CFG_EDGE_CAT(edge)==ET_IPFALLTHRU ||
      CFG_EDGE_CAT(edge)==ET_IPUNKNOWN ||
      CFG_EDGE_CAT(edge)==ET_IPSWITCH ||
      CFG_EDGE_CAT(edge)==ET_SWITCH
    )
      return false;
    if(BBL_IS_HELL(CFG_EDGE_TAIL(edge)))
      return false;
    if(EDGE_CORR((t_edge*)edge)!=NULL)
      return false;
    
    /* If we cannot redirect the successor BBLs of all outgoing edges, we cannot transform this bbl.
       For example, if we JUMP to a BBL that also has an incoming HELL edge. Also, when canRedirectPredecessor
       fails due to FUNCTION_BBL_FIRST/LAST (TODO: verify).
       TODO: we could duplicate the BBL to resolve this
       */
    t_bbl* next = CFG_EDGE_TAIL(edge);
    if(!canRedirectPredecessor(next))
      return false;
  }
  
  return true;
}

bool FlattenFunctionTransformation::canRedirectPredecessor(t_bbl* bbl) {
  t_cfg_edge * edge;

  if (! CanBeFlattenedGeneric(BBL_FUNCTION(bbl)))
    return false;

  /* Since we add a BBL with the possible spill code that redirects to this BBL, rather than extend this BBL itself, we should be able to
   * redirect almost any incoming edge */
  
  if (IS_DATABBL(bbl)) {
    VERBOSE(0, ("WARNING WARNING: Tried to transform a data BBL: @eiB", bbl));
    return false;
  }

  /* TODO this should also be gone? Right now, at least, this doesn't quite work yet because the entry BBL is forcibly inserted as from_switch_list[0], so then
     we can insert the first bbl twice in from_switch_list right now... */
  t_function* fun = BBL_FUNCTION(bbl);
  
  /* test: ander gaat de switch ook naar het return-blok! */
  if(bbl==FUNCTION_BBL_LAST(fun) || bbl==FUNCTION_BBL_FIRST(fun)) {
    return false;
  }

  return true;
}

t_bbl* FlattenFunctionTransformation::createAdditionalSwitchBlock(t_function* fun) {
#if 0
  ASSERT(function_info_map.find(fun) != function_info_map.end(), ("The function info map should have information on the function by now"));
  const auto& info = function_info_map[fun];

  t_cfg* cfg = FUNCTION_CFG(fun);
  
  t_bbl* bbl = BblNew(cfg);  
  BblInsertInFunction(bbl, fun);
  
  t_section* var_section = ObjectNewSubsection(CFG_OBJECT(cfg), info.tot_bbl*sizeof(t_uint32), RODATA_SECTION);
  
  createSwitchBlock(bbl, var_section, info.reg_info, nullptr /* first_bbl_is_flattened: don't split this BBL again to prepend function initializer */);
  
  fillEntriesInSwitchTable(fun, bbl, var_section); /* TODO: the original code by Matias seemed more complicated, I'm not sure why... */
  
  return bbl;
#endif
  FATAL(("Reimplement this"));
  return nullptr;
}

void FlattenFunctionTransformation::fillEntriesInSwitchTable(function_info& info, t_cfg* cfg, t_bbl* switch_bbl, t_section* var_section) {
  for (auto bbl: info.transformPredecessorsSet) {
    t_reloc* rel;
    t_cfg_edge* edge;
    
    t_uint32 id = info.bblToId[bbl];

    /* TODO: right now, this always adds a new BBL ending in a jump to the actual block. In many cases (but not all!), this is not needed, so optimize this! */
    t_bbl* bblX = BblNew(cfg);
    t_function* fun = BBL_FUNCTION(bbl);
    BblInsertInFunction(bblX, fun);

    restoreRegisters(bblX, bbl, info.reg_info);
    CfgEdgeCreate(cfg, bblX, bbl, ET_JUMP);

    /* TODO: this is an over-approximation in case the bbl has predecessors outside both inside and outside the switch table... */
    BBL_SET_EXEC_COUNT(bblX, BBL_EXEC_COUNT(bblX));

    edge = CfgEdgeCreate(cfg, switch_bbl, bblX, ET_IPSWITCH);
    rel  = writeRelocationInSwitchTable(cfg, var_section, switch_bbl, bblX, id);

    CFG_EDGE_SET_REL(edge,rel);
    CFG_EDGE_SET_SWITCHVALUE(edge, id);
    RELOC_SET_SWITCH_EDGE(rel, edge);
  }
}

/* This needs to take a vector, because we will assign (for now) IDs in the order of the bblSet -> a set would have address-sensitive behaviour */
/* Implementational note:
 * BBLs inside the region might need to redirect control flow to BBLs outside the region. Those BBLs are added to the table of contents as well:
 * then we set the id of the next BBL to the id of the region outside the BBL, redirect to the switch block, which redirects to them */
void FlattenFunctionTransformation::flattenBasicBlockSet(const std::vector<t_bbl*>& bblList, t_randomnumbergenerator* rng) {
  if (bblList.size() == 0)
    return;

  t_section* var_section;
  t_ins* ins;
  t_cfg* cfg = BBL_CFG(bblList[0]);
  t_cfg_edge* edge;

  /* TODO: right now, fallthroughs of CALLs are not transformed, but we could try to work around that to obfuscate more... */
  set<t_function*> functions_involved;

  /* Prepare some stats and logging */
  for (auto bbl: bblList) {
    if (canRedirectPredecessor(bbl) || canRedirectSuccessor(bbl)) {
      bbls_transformed++;
      t_ins* ins;
      BBL_FOREACH_INS(bbl, ins) {
        insts_in_bbls++;
      }

      functions_involved.insert(BBL_FUNCTION(bbl));

      if(canRedirectSuccessor(bbl)) {
        LOG_MORE(L_OBF_FF) { AddTransformedBblToLog("FlattenFunction", bbl); }
      }
    }
  }

  /* Logging which functions are affected */
  LOG_MORE(L_OBF_FF) {
    for (auto fun: functions_involved) {
      LogFunctionTransformation("before", fun);
    }
  }

  /* Let's make a table of contents: a bbl -> integer id mapping, and set up the sets of BBLs to transform with predecessors/successors */
  /* Furthermore, when a function entry BBL would be in the set of transformed (to) lists, we split off that BBL. We want to add redirection code from
     the (new) function entry BBL to the id of the split-off BBL.
     The original code put this as 'possible_fallthrough_to_switch', and created the instructions in createSwitchBlock, but we can't do that
     since multiple functions might be involved, so we do that in a second pass */
  map<t_bbl*, t_uint32> bblToId;
  BblSet transformSuccessorsSet;
  BblSet transformPredecessorsSet;

  t_uint32 bblId = 0;

  for (auto bbl: bblList) {
    if ( bblToId.find(bbl) != bblToId.end() ) /* Might be already added previously by iterating over the successors */
      continue;

    if (canRedirectPredecessor(bbl)) {
      //VERBOSE(0, ("A @B -> %i", bbl, bblId));
      if ( bblToId.find(bbl) == bblToId.end() ) {
        bblToId[bbl] = bblId++;
      }
      transformPredecessorsSet.insert(bbl);
    }

    if (canRedirectSuccessor(bbl)) {
      transformSuccessorsSet.insert(bbl);

      BBL_FOREACH_SUCC_EDGE(bbl, edge) {
        t_bbl* succ_bbl = CFG_EDGE_TAIL(edge);
        if ( bblToId.find(succ_bbl) == bblToId.end() ) {
          //VERBOSE(0, ("A2 @B -> %i", bbl, bblId));
          bblToId[succ_bbl] = bblId++;

          transformPredecessorsSet.insert(succ_bbl); /* TODO: but what if we can't? This should have been verified in canRedirectSuccessor, VERIFY */
        }
      }
    }
  }

  t_uint32 maxId = bblId;
  set<t_bbl*> splitOffFunctionEntriesToRedirect;

  /* Function entries */
  for (auto bbl: transformPredecessorsSet) {
    if (bbl != FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl)))
      continue;

    t_bbl* split = BblSplitBlock(bbl ,BBL_INS_FIRST(bbl), TRUE);
    DiabloBrokerCall("AFAfterSplit", bbl, split);

    splitOffFunctionEntriesToRedirect.insert(bbl);
    transformPredecessorsSet.insert(split);

    bblToId[split] = bblToId[bbl];

    if (transformSuccessorsSet.find(bbl) != transformSuccessorsSet.end()) {
      transformSuccessorsSet.erase(bbl);
      transformSuccessorsSet.insert(split);
    }
  }

  /* We do not transform the (split off) function entry head, but the split off part */
  for (auto bbl: splitOffFunctionEntriesToRedirect) {
    transformPredecessorsSet.erase(bbl);
  }

  function_info info;
  info.bblToId = bblToId;
  info.transformSuccessorsSet = transformSuccessorsSet;
  info.transformPredecessorsSet = transformPredecessorsSet;

  initRegisterInfo(&info, rng);

  var_section = ObjectNewSubsection(CFG_OBJECT(cfg), maxId * sizeof(t_uint32), RODATA_SECTION);

  /* Top Switch Production {{{ */

  // The switch block
  t_bbl* switch_bbl = BblNew(cfg);

  static t_uint32 switch_function_nr = 0;
  char nr_str[10];

  sprintf (nr_str, "%d", switch_function_nr++);
  t_string fun_name = StringConcat2("SwitchFunction_", nr_str);

  t_function* switch_block_function = FunctionMake(switch_bbl, fun_name, FT_NORMAL);
  ASSERT(special_function_uid != FunctionUID_INVALID, ("invalid special function uid!"));
  BblSetOriginalFunctionUID(switch_bbl, special_function_uid);

  Free (fun_name);

  BblKill (FunctionGetExitBlock (switch_block_function));

  createSwitchBlock(switch_bbl, var_section, info.reg_info);

  /* The switch block */
  fillEntriesInSwitchTable(info, cfg, switch_bbl, var_section);

  /* We already added pop instructions to all basic blocks' predecessors in fillEntriesInSwitchTable */
  /* So now just redirect the successors */

  for (auto bbl: transformSuccessorsSet) {
    if (!BBL_INS_LAST(bbl))
      continue;

    shared_ptr<Successor> successor = redirectSuccessorsCode(bbl, &info);
    if (!successor->code_added) {
      /* This is a return (or similar): don't screw up edges */
      VERBOSE(1, ("Not removing edges for @eiB", bbl));

      continue;
    }

    t_cfg_edge* h_edge;
    BBL_FOREACH_SUCC_EDGE_SAFE(bbl,edge,h_edge) {
      CfgEdgeKill(edge);
    }

/* TODO: */
#if 0
        if(more_switch)
        {
          switch_bbl = createAdditionalSwitchBlock(fun);
          
          redirectToSwitchBlock(successor, switch_bbl);
        } else { ... }
#endif
    redirectToSwitchBlock(successor, switch_bbl);
  }

  LOG_MORE(L_OBF_FF) {
    for (auto f: functions_involved) {
      LogFunctionTransformation("after", f);
    }
    LogFunctionTransformation("after", switch_block_function);
  }
}
 
void FlattenFunctionTransformation::redirectToSwitchBlock(shared_ptr<Successor> successor, t_bbl* switch_bbl) {
  CfgEdgeCreate(successor->cfg, successor->bbl, switch_bbl, ET_IPJUMP);
}

/* }}} */

FlattenFunctionTransformation::FlattenFunctionTransformation(bool multiple_flatten)
: multiple_flatten(multiple_flatten)
{
  AddOptionsListInitializer(obfuscation_flatten_function_option_list); FlattenFunctionOptInit();
 
  if (multiple_flatten)
    _name = _name_multiple;
  else
    _name = _name_nomultiple;

  RegisterTransformationType(this, _name);
  RegisterTransformationType(this, _name_generic);
  
  functions_transformed = 0;
  bbls_transformed = 0;
  insts_in_bbls = 0;
}

bool FlattenFunctionTransformation::canTransform(const t_function* fun) const {
  if (!obfuscation_flatten_function_options.flatten_function && !AllObfuscationsEnabled())
    return false;
  return CanBeFlattenedGeneric(fun); /* TODO: ick */
}

bool FlattenFunctionTransformation::doTransform(t_function* fun, t_randomnumbergenerator * rng) {
  /* The additional info is the number of basic blocks to merge, chose that many at random */
  /* TODO: in the new framework: randomize again the partial flattening number! */
  /* TODO: optionally chose from the less hot BBLs */
  // TODO: actually chose at random, not just the first n
  //int max_bbl = *(int*)additional_info;
  int max_bbl = countBasicBlocks(fun);
  
  vector<t_bbl*> flattenSet;
  t_bbl* bbl;
  
  /* Function transformation: all BBLs can be transformed */
  FUNCTION_FOREACH_BBL(fun, bbl) {
    flattenSet.push_back(bbl);
  }

  VERBOSE(0, ("Flattening %i BBs out of %i in function '%s'", max_bbl, max_bbl, FUNCTION_NAME(fun)));

  START_LOGGING_TRANSFORMATION(L_OBF_FF, "FlattenFunction,%x,%s", BBL_CADDRESS(FUNCTION_BBL_FIRST(fun)), FUNCTION_NAME(fun));

  flattenBasicBlockSet(flattenSet, rng);

  STOP_LOGGING_TRANSFORMATION(L_OBF_BF);

  return true;
}

bool FlattenFunctionTransformation::canTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls) {
  if (bbls.size() == 0)
    return false;

  /* If we can transform any of the functions of the BBLs mentioned, return yes */
  for (auto bbl_pair: bbls) {
    t_bbl* bbl = bbl_pair.first;
    if (BBL_FUNCTION(bbl) && canTransform(BBL_FUNCTION(bbl)))
      return true;
  }

  return false;
}

AnnotatedBblPartition PartitionOfCompatibleMaxSwitchBlockSizes(const vector<AnnotatedBbl>& bbls) {
  int current_size = 0;
  int current_max = INT_MAX;
  AnnotatedBblPartition result;
  vector<AnnotatedBbl> current_partition;

  for (auto abbl: bbls) {
    int bbl_max = GetAnnotationValueWithDefault(abbl, "max_switch_size", INT_MAX);
    if (bbl_max < current_max) {
      current_max = bbl_max;
    }

    if (current_size + 1 > current_max) {
      result.push_back(current_partition);
      current_partition.clear();
      current_max = bbl_max;
      current_size = 0;
    }

    current_partition.push_back(abbl);
    current_size++;
  }

  if (current_size > 0) {
    result.push_back(current_partition);
  }

  return result;
}

void FlattenFunctionTransformation::flattenWithCompatibleSwitchBlockSizes(const vector<AnnotatedBbl>& bbls, t_randomnumbergenerator* rng) {
  /* It looks too hard (NP-Complete?) to also include a min_switch_size in any meaningful way... TODO */

  AnnotatedBblPartition partitions = PartitionOfCompatibleMaxSwitchBlockSizes(bbls);
  for (auto partition: partitions) {
    VERBOSE(0, ("Flattening a partition of BBLs consisting of %i BBLs", partition.size()));

    vector<t_bbl*> bbls_only;
    set<t_function*> funs;
    stringstream ss;

    for (auto bbl_pair: partition) {
      if (IS_DATABBL(bbl_pair.first))
        continue;

      bbls_only.push_back(bbl_pair.first);

      t_function* fun = BBL_FUNCTION(bbl_pair.first);
      if (funs.find(fun) == funs.end()) {
        funs.insert(fun);
        if (fun)
          ss << "," << FUNCTION_NAME(fun);
        else
          ss << ",(none)";
      }
      VERBOSE(0, ("BBL of %s", FUNCTION_NAME(fun)));
    }
    VERBOSE(0, ("STARTING"));

    START_LOGGING_TRANSFORMATION(L_OBF_FF, "FlattenFunction,%s", ss.str().c_str());

    flattenBasicBlockSet(bbls_only, rng);
    VERBOSE(0, ("Done transforming region"));

    STOP_LOGGING_TRANSFORMATION(L_OBF_FF);
  }
}
/* For now, we do not transform a single multi-function region into one switch block, that would transform all code in the region into single-BBL
 * functions with an incomping IPSWITCH. Might to so later. What we do now: iterate over all BBLs, group them into their functions,
 * and flatten the individual functions (flattening the BBLs that have been marked here as input to flatten) */
void FlattenFunctionTransformation::doTransformRegion(const std::vector<std::pair<t_bbl*, AnnotationIntOptions>>& bbls, t_randomnumbergenerator * rng) {
  if (bbls.size() == 0)
    return;

  flattenWithCompatibleSwitchBlockSizes(bbls, rng);
}

void FlattenFunctionTransformation::dumpStats(const std::string& prefix) {
  VERBOSE(0, ("%sFlatten_Stats,functions_transformed,%i", prefix.c_str(), functions_transformed));
  VERBOSE(0, ("%sFlatten_Stats,bbls_transformed,%i", prefix.c_str(), bbls_transformed));
  VERBOSE(0, ("%sFlatten_Stats,insts_in_bbls,%i", prefix.c_str(), insts_in_bbls));
}
