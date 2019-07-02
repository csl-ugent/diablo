#include <diabloanoptarm_advanced_factor.hpp>

/*
w0: incoming edge count
w1: gained instruction count
w2: required register count
w3: covered function count
w4: available register count
*/

DEFINE_SCORE_CALCULATE_INIT(Default)
{
  SET_DEFAULT_WEIGHT(0, 100);
  SET_DEFAULT_WEIGHT(1, 100);
  SET_DEFAULT_WEIGHT(2, 100);
  SET_DEFAULT_WEIGHT(3, 100);
  SET_DEFAULT_WEIGHT(4, 100);

  VERBOSE(0, ("score weight factors for DEFAULT"));
  VERBOSE(0, ("  factored path count %d", GET_WEIGHT(0)));
  VERBOSE(0, ("  gained instruction count %d", GET_WEIGHT(1)));
  VERBOSE(0, ("  required register count %d", GET_WEIGHT(2)));
  VERBOSE(0, ("  covered function count %d", GET_WEIGHT(3)));
  VERBOSE(0, ("  available register count %d", GET_WEIGHT(4)));
}

DEFINE_SCORE_CALCULATE(Default)
{
  t_score score = t_score(t_score::Type::Total);
  score.data = data;
  score.length = slice_size;

  score.total_score = t_score::LargeScoreBase();

  /* other statistics of the whole set */
  for (auto slice : slices)
  {
    score.total_score += SlicePathCount(slice) * (GET_WEIGHT(0) / 100);
    score.total_score += slice_size * (GET_WEIGHT(1) / 100);
  }

  /* 2. needed registers; this is a BAD factor! */
  score.total_score -= data.min_regs_needed * (GET_WEIGHT(2) / 100);

  /* 3. number of different functions */
  score.total_score += data.source_info.functions.size() * (GET_WEIGHT(3) / 100);

  /* 4. number of dead registers */
  score.total_score += data.nr_regs_available * (GET_WEIGHT(4) / 100);

  return score;
}
