#include <diabloanoptarm_advanced_factor.hpp>
using namespace std;

/*
w0: covered and executed archive count
w1: covered and executed object count
w2: covered and executed function count
w3: covered and not executed archive count
w4: covered and not executed object count
w5: covered and not executed function count
w6: covered archive count
w7: covered object count
w8: covered function count
*/

DEFINE_SCORE_CALCULATE_INIT(Exponential)
{
  SET_DEFAULT_WEIGHT(0, 50);
  SET_DEFAULT_WEIGHT(1, 40);
  SET_DEFAULT_WEIGHT(2, 30);
  SET_DEFAULT_WEIGHT(3, 10);
  SET_DEFAULT_WEIGHT(4, 5);
  SET_DEFAULT_WEIGHT(5, 1);
  SET_DEFAULT_WEIGHT(6, 0);
  SET_DEFAULT_WEIGHT(7, 0);
  SET_DEFAULT_WEIGHT(8, 0);
  SET_DEFAULT_WEIGHT(9, 100);

  VERBOSE(0, ("score weight factors for EXPONENTIAL"));
  VERBOSE(0, ("  covered + executed archive count %d", GET_WEIGHT(0)));
  VERBOSE(0, ("  covered + executed object count %d", GET_WEIGHT(1)));
  VERBOSE(0, ("  covered + executed function count %d", GET_WEIGHT(2)));
  VERBOSE(0, ("  covered + !executed archive count %d", GET_WEIGHT(3)));
  VERBOSE(0, ("  covered + !executed object count %d", GET_WEIGHT(4)));
  VERBOSE(0, ("  covered + !executed function count %d", GET_WEIGHT(5)));
  VERBOSE(0, ("  covered archive count %d", GET_WEIGHT(6)));
  VERBOSE(0, ("  covered object count %d", GET_WEIGHT(7)));
  VERBOSE(0, ("  covered function count %d", GET_WEIGHT(8)));
  VERBOSE(0, ("  slice size %d", GET_WEIGHT(9)));
}

DEFINE_SCORE_CALCULATE(Exponential)
{
  auto check_overflow = [&slices, &slice_size, &data] (t_score::LargeScore x, string str) {
    if (x == t_score::LargeScoreHuge()) {
      /* overflow! */
      SliceSetPrint(slices, "overflow", slice_size);
      FATAL(("overflow (%s)! slice size %d, %s", str.c_str(), slice_size, data.Print().c_str()));
    }
  };

  auto factor = [check_overflow] (int weight, size_t value, string str, long double exp_factor) {
    t_score::LargeScore x = pow(2.0L, exp_factor * (weight * value));
    check_overflow(x, str);

    return x;
  };

  t_score result = t_score(t_score::Type::Total);
  result.data = data;
  result.length = slice_size;

  result.total_score = t_score::LargeScoreBase();

  /* executed slices */
  result.total_score += factor(GET_WEIGHT(0), data.source_info.exec_archives.size() , "exec_archives" , 0.5L);
  result.total_score += factor(GET_WEIGHT(1), data.source_info.exec_objects.size()  , "exec_objects"  , 0.5L);
  result.total_score += factor(GET_WEIGHT(2), data.source_info.exec_functions.size(), "exec_functions", 0.5L);

  /* non-executed slices */
  result.total_score += factor(GET_WEIGHT(3), data.source_info.archives.size()  - data.source_info.exec_archives.size() , "not_exec_archives" , 0.5L);
  result.total_score += factor(GET_WEIGHT(4), data.source_info.objects.size()   - data.source_info.exec_objects.size()  , "not_exec_objects"  , 0.5L);
  result.total_score += factor(GET_WEIGHT(5), data.source_info.functions.size() - data.source_info.exec_functions.size(), "not_exec_functions", 0.5L);

  /* total slices */
  result.total_score += factor(GET_WEIGHT(6), data.source_info.archives.size() , "archives" , 0.5L);
  result.total_score += factor(GET_WEIGHT(7), data.source_info.objects.size()  , "objects"  , 0.5L);
  result.total_score += factor(GET_WEIGHT(8), data.source_info.functions.size(), "functions", 0.5L);

  /* slice size */
  if (GET_WEIGHT(9) > 0)
    result.total_score *= slice_size;

  return result;
}
