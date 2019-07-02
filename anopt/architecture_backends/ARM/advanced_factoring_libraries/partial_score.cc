#include <diabloanoptarm_advanced_factor.hpp>

/*
w0: potency weight
w1: resilience weight
w2: stealth weight
*/

DEFINE_SCORE_CALCULATE_INIT(Partial)
{
  SET_DEFAULT_WEIGHT(0, 1);
  SET_DEFAULT_WEIGHT(1, 1);
  SET_DEFAULT_WEIGHT(2, 1);

  VERBOSE(0, ("score weight factors for PARTIAL"));
  VERBOSE(0, ("  potency %d", GET_WEIGHT(0)));
  VERBOSE(0, ("  resilience %d", GET_WEIGHT(1)));
  VERBOSE(0, ("  stealth %d", GET_WEIGHT(2)));
}

DEFINE_SCORE_CALCULATE(Partial)
{
  t_score score = t_score(t_score::Type::Partial);
  score.data = data;
  score.length = slice_size;

  /* POTENCY
   * - more covered archives = higher potency */
  size_t potency = data.source_info.archives.size();

  /* RESILIENCE
   * - more covered executed archives = higher resilience
   * - attacker removes edges showing invariant behavior */

  /* a fragment is either:
   * - not executed (invariant)
   * - executed once (invariant)
   * - executed twice (variant)
   * higher values are irrelevant here because the fragment will be variant anyways.
   * We don't care whether or not the executed slices are part of different functions, object files or archives.
   * We assume that for attackers, the separation between those categories is unclear. */
  size_t nr_exec = data.nr_executed_slices;
  if (nr_exec > 2)
    nr_exec = 2;
  size_t resilience = nr_exec;

  /* STEALTH
   * - using internal constants = higher stealth
   * - depending on dispatcher, more stealthy or not:
   *    switch-based dispatcher is stealthier with bounds check than without */
  size_t stealth = data.nr_used_constants + (data.flags & AF_POSSIBILITY_BOUNDS_CHECK) * 1000;

  score.total_score = GET_WEIGHT(0) * potency + GET_WEIGHT(1) * resilience + GET_WEIGHT(2) * stealth;

  return score;
}
