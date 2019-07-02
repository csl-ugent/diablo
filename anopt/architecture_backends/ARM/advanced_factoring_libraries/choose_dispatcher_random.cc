#include <diabloanoptarm_advanced_factor.hpp>

#include <vector>

DEFINE_CHOOSE_DISPATCHER(Random)
{
  /* construct list of possible dispatchers */
  std::vector<DispatcherType> dispatcher_types;
  if (flags & AF_POSSIBILITY_DISTRIBUTED)
    dispatcher_types.push_back(DispatcherType::DistributedTable);
  if (flags & AF_POSSIBILITY_INDIRECTBRANCH)
    dispatcher_types.push_back(DispatcherType::IndirectBranch);
  if (flags & AF_POSSIBILITY_SWITCHBRANCH)
    dispatcher_types.push_back(DispatcherType::SwitchBranch);
  if (flags & AF_POSSIBILITY_SWITCHOFFSET)
    dispatcher_types.push_back(DispatcherType::SwitchOffset);
  if (flags & AF_POSSIBILITY_INTERNAL_CONDITIONALJUMP)
    dispatcher_types.push_back(DispatcherType::InternalConditionalJump);
  if (flags & AF_POSSIBILITY_CONDITIONALJUMP)
    dispatcher_types.push_back(DispatcherType::ConditionalJump);

  /* pick random dispatcher */
  return dispatcher_types[RNGGenerate(rng) % dispatcher_types.size()];
}
