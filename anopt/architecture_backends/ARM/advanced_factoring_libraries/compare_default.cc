#include <diabloanoptarm_advanced_factor.hpp>

DEFINE_SCORE_COMPARE(Default)
{
  if (lhs.total_score > rhs.total_score)
    return LHS_BETTER_THAN_RHS;
  else
    return RHS_BETTER_THAN_LHS;
}
