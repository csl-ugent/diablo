#include <diabloanoptarm_advanced_factor.hpp>

/*
  if 1&2 not executed OR
      1&2 executed:
    criteria = [score, length, executed archives, executed object files, executed functions, archives, object files, functions]
    for crit in criteria:
      if crit(1) == crit(2):
        continue
      elif crit(1) > crit(2):
        return 1
      else:
        return 2

    return RANDOM(1, 2)

  else:
    if 1 executed:
      return 1
    else:
      return 2
 */

DEFINE_SCORE_COMPARE(DecisionTree)
{
  /* 1. prioritize sets that are at least executed once over non-executed sets */
  if ((lhs.data.source_info.exec_archives.size() == 0) ^ (rhs.data.source_info.exec_archives.size() == 0)) {
    if (lhs.data.source_info.exec_archives.size() > 0)
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }
  /* no check needed for 'executed objects' and 'executed functions'
   * because they have been covered by the previous check already. */

  /* we get here if:
   * - both sets are not executed
   * - both sets are executed */

  /* 2. prioritize sets with higher scores over those with lower scores */
  else if (lhs.total_score != rhs.total_score) {
    if (lhs.total_score > rhs.total_score)
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* we get here if:
   * - both sets got assigned identical scores */

  /* 3. prioritize sets of longer length over others */
  else if (lhs.length != rhs.length) {
    if (lhs.length > rhs.length)
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* we get here if:
   * - both sets have identical lengths */

  /* 4. prioritize sets covering more executed archives */
  else if (lhs.data.source_info.exec_archives.size() != rhs.data.source_info.exec_archives.size()) {
    if (lhs.data.source_info.exec_archives.size() > rhs.data.source_info.exec_archives.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 5. prioritize sets covering more executed objects */
  else if (diabloanoptarm_options.advanced_factoring_scoring_include_object
            && lhs.data.source_info.exec_objects.size() != rhs.data.source_info.exec_objects.size()) {
    if (lhs.data.source_info.exec_objects.size() > rhs.data.source_info.exec_objects.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 6. prioritize sets covering more executed functions */
  else if (diabloanoptarm_options.advanced_factoring_scoring_include_function
            && lhs.data.source_info.exec_functions.size() != rhs.data.source_info.exec_functions.size()) {
    if (lhs.data.source_info.exec_functions.size() > rhs.data.source_info.exec_functions.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 7. prioritize sets covering more archives */
  else if (lhs.data.source_info.archives.size() != rhs.data.source_info.archives.size()) {
    if (lhs.data.source_info.archives.size() > rhs.data.source_info.archives.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 8. prioritize sets covering more objects */
  else if (diabloanoptarm_options.advanced_factoring_scoring_include_object
            && lhs.data.source_info.objects.size() != rhs.data.source_info.objects.size()) {
    if (lhs.data.source_info.objects.size() > rhs.data.source_info.objects.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 9. prioritize sets covering more functions */
  else if (diabloanoptarm_options.advanced_factoring_scoring_include_function
            && lhs.data.source_info.functions.size() != rhs.data.source_info.functions.size()) {
    if (lhs.data.source_info.functions.size() > rhs.data.source_info.functions.size())
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* 10. factor as much instructions as possible
   *      (don't need to look at length here, because it is assumed to be equal here as tested earlier on) */
  else if (lhs.data.nr_slices != rhs.data.nr_slices) {
    if (lhs.data.nr_slices > rhs.data.nr_slices)
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }

  /* default */
  if (diabloanoptarm_options.advanced_factoring_leftbetter)
    return LHS_BETTER_THAN_RHS;
  else if (diabloanoptarm_options.advanced_factoring_rightbetter)
    return RHS_BETTER_THAN_LHS;
  else {
    if (lhs.random_uid > rhs.random_uid)
      return LHS_BETTER_THAN_RHS;
    else
      return RHS_BETTER_THAN_LHS;
  }
}
