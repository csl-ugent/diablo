/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <vector>

extern "C" {
#include "diabloflowgraph.h"
#include "diabloflowgraph_new_target_selector_cmdline.h"
}

std::vector<NewTargetSelector *> installed_target_handlers;

using namespace std;

NewTargetSelector::NewTargetSelector() {
  AddOptionsListInitializer(diabloflowgraph_new_target_selector_option_list); DiabloFlowgraphNewTargetSelectorCmdlineInit();

  RegisterTransformationType(this, _name);
}

InfiniteLoopSelector::InfiniteLoopSelector() {
  RegisterTransformationType(this, _name);
}

NewTargetInSameFunctionSelector::NewTargetInSameFunctionSelector() {
  RegisterTransformationType(this, _name);
}

void InstallGenericNewTargetHandlers() {
  installed_target_handlers.clear();

  installed_target_handlers.push_back(new NewTargetInSameFunctionSelector());

  /* Always install this one as backup when no other one is available */
  installed_target_handlers.push_back(new InfiniteLoopSelector());
}

void DestroyGenericNewTargetHandlers() {
  for (auto i : installed_target_handlers)
    delete i;
}


bool InfiniteLoopSelector::canTransform(const t_bbl*) const {
  /* This one can always transform, but only return true if it is enabled. Thus, we can always install
   * this selector, and then it can get randomly chosen if it is explicitly enabled, and chosen as fallback if not. */

  return diabloflowgraph_new_target_selector_options.new_target_infinite_loop;
}

t_bbl* InfiniteLoopSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  t_function* fun = BBL_FUNCTION(bbl);
  t_cfg* cfg = FUNCTION_CFG(fun);

  t_bbl* rubbish = BblNew(cfg);
  DiabloBrokerCall("CopyToNewTargetBBL", bbl, rubbish);
  BblInsertInFunction(rubbish, fun);

  // ArmMakeInsForBbl(Mov,  Append, ins, rubbish, ARM_REG_PC, ARM_REG_NONE, 0, ARM_CONDITION_AL); // Just crash TODO make this possible generically

  GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(rubbish);
  CfgEdgeCreate(cfg, rubbish, rubbish, ET_JUMP);

  return rubbish;
}

static t_int32 countBasicBlocks(t_function * fun)
{
  t_int32 nr_bbl=0;
  t_bbl * bbl;

  FUNCTION_FOREACH_BBL(fun,bbl)
  {
    nr_bbl++;
  }

  return nr_bbl;
}

bool NewTargetInSameFunctionSelector::canTransform(const t_bbl* bbl) const {
  // return countBasicBlocks(BBL_FUNCTION(bbl)) > 1; // TODO, this might make more sense...
  return diabloflowgraph_new_target_selector_options.new_target_in_same_function;
}

/* TODO: this might/will screw liveness analysis!?! */
t_bbl* NewTargetInSameFunctionSelector::doTransform(t_bbl* bbl, t_randomnumbergenerator * rng) {
  /*  */
  vector<t_bbl*> bbls;
  t_function* fun = BBL_FUNCTION(bbl);
  t_bbl* bbl_it;

  FUNCTION_FOREACH_BBL(fun, bbl_it) {
    bbls.push_back(bbl_it);
  }

  /* so this can actually return bbl again... */
  return bbls.at(RNGGenerateWithRange(rng, 0, bbls.size() - 1));
}


t_bbl* SelectTargetFor(t_bbl* bbl, t_randomnumbergenerator* rng, bool need_to_fall_through) {
  NewTargetSelector* selector = 0;
  auto possible = GetTransformationsForType("newtargetselector");

  while (!selector && possible.size() > 0) {
    auto rand = RNGGenerateWithRange(rng, 0, possible.size() - 1);
    auto it   = possible.begin();

    advance(it, rand);
    auto s    = dynamic_cast<NewTargetSelector*>(*it);

    if (s->canTransform(bbl)) {
      selector = s;
    }

    possible.erase(it);
  }

  if (!selector) {
    selector = dynamic_cast<NewTargetSelector*>(GetTransformationsForType("newtargetselector:infiniteloop").at(0));
    VERBOSE(1, ("Falling back to default infiniteloop selector"));
  }

  VERBOSE(1, ("Chose new target selector '%s'", selector->name()));
  
  t_bbl* target = selector->doTransform(bbl, rng);

  /* If our callee needs to fall through to our target bbl, ensure this is possible */
  if (need_to_fall_through) {
    t_cfg_edge* edge;
    bool ft_edges = true; // false;
    /* TODO FIXME: this does not work: it crashes in layouting due to a NULL chain */
#if 0
    BBL_FOREACH_PRED_EDGE(target, edge) {
      if (CFG_EDGE_CAT(edge) != ET_JUMP) // a very simple check, so we also have switch edges, interprocedurals, etc, to be on the safe side
        ft_edges = true;
    }
#endif

    /* Create a new bbl that jumps to our target */
    if (ft_edges) {
      VERBOSE(1, ("Required fallthrough, but one already existed: inserting additional jump-BBL for @eiB", target));

      t_function* fun = BBL_FUNCTION(target);
      t_cfg* cfg      = FUNCTION_CFG(fun);
      t_bbl* jumper   = BblNew(cfg);
      DiabloBrokerCall("CopyToNewTargetBBL", bbl, jumper);

      BblInsertInFunction(jumper, fun);

      GetArchitectureInfo(bbl)->appendUnconditionalBranchInstruction(jumper);
      CfgEdgeCreate(cfg, jumper, target, ET_JUMP);

      return jumper;
    }
  }

  return target;
}
