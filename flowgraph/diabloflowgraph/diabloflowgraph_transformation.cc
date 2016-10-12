/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <map>
#include <string>

#include "diabloflowgraph_transformation.hpp"

using namespace std;

static map<string, vector<Transformation*>> reverse_transfo_map;

void RegisterTransformationType(Transformation* transfo, t_const_string type) {
  auto found = reverse_transfo_map.find(type);

  if (found != reverse_transfo_map.end()) {
	(*found).second.push_back(transfo);
	return;
  }

  auto to_insert = vector<Transformation*>();
  to_insert.push_back(transfo);
  reverse_transfo_map.insert(make_pair(type, to_insert));
}

vector<Transformation*> GetTransformationsForType(t_const_string type, bool is_for_randomized_list) {
  auto found = reverse_transfo_map.find(type);

  if (found != reverse_transfo_map.end()) {
    if (!is_for_randomized_list)
      return (*found).second;

    vector<Transformation*> out;
    for (auto it: (*found).second) {
      if (it->transformationIsAvailableInRandomizedList()) {
        out.push_back(it);
      }
    }

  return out;
  }

  return vector<Transformation*>();
}

BBLTransformation::BBLTransformation() {
  RegisterTransformationType(this, name());
}

FunctionTransformation::FunctionTransformation() {
  RegisterTransformationType(this, name());
}

ArchitectureInfo::ArchitectureInfo() {
  possibleRegisters = RegsetNew();
}

static ArchitectureInfoWrapper* wrapper = nullptr;
void SetArchitectureInfoWrapper(ArchitectureInfoWrapper* w) {
  wrapper = w;
}
ArchitectureInfoWrapper *GetArchitectureInfoWrapper() {
  return wrapper;
}

ArchitectureInfo* GetArchitectureInfo(t_bbl* bbl_for) {
  ASSERT(wrapper, ("Expected to have seen a call to SetArchitectureInfoWrapper before calls to GetArchitectureInfo!"));
  return wrapper->getArchitectureInfo(bbl_for);
}


MaybeSaveRegister ArchitectureInfo::getRandomRegisterCustomLiveness(t_regset live, t_regset not_in, t_randomnumbergenerator* rng) {
  t_regset available;

  available = RegsetDiff(possibleRegisters, live);
  available = RegsetDiff(available, not_in);

  if(RegsetIsEmpty(available)) {
    t_reg chosen; // TODO: optimize?
    do {
      chosen = getGenericRandomRegister(rng);
    } while(RegsetIn(not_in, chosen));

    return MaybeSaveRegister(chosen, true);
  }

  // TODO: optimize?
  t_reg reg;
  vector<t_reg> possible;
  REGSET_FOREACH_REG(available, reg) {
    possible.push_back(reg);
  }

  return MaybeSaveRegister(possible.at(RNGGenerateWithRange(rng, 0, possible.size() - 1)), false);
}


MaybeSaveRegister ArchitectureInfo::getRandomRegister(t_ins* before, t_regset not_in, t_randomnumbergenerator* rng) {
  return getRandomRegisterCustomLiveness(InsRegsLiveBefore(before), not_in, rng);
}

t_bbl* ArchitectureInfo::splitBasicBlockWithJump(t_bbl* bbl, t_ins* ins, bool before) {
  t_bbl* split_off = BblSplitBlock(bbl, ins, before);
  t_cfg_edge* edge;
  t_cfg_edge* edge_s;

  appendUnconditionalBranchInstruction(bbl);

  /* TODO: is this really needed? */
  BBL_FOREACH_SUCC_EDGE_SAFE(bbl, edge, edge_s) {
    CfgEdgeKill(edge);
  }

  CfgEdgeCreate(BBL_CFG(bbl), bbl, split_off, ET_JUMP);

  return split_off;
}
