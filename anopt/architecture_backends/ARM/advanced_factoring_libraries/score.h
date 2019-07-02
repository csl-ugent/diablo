#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_SCORE_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_SCORE_H
#include <set>
#include <string>
#include <cmath>

#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
#define EXPORT extern "C"
#else
#define EXPORT
#endif

#define SET_DEFAULT_WEIGHT(id, value) do { \
  if (! diabloanoptarm_options.af_weight_ ## id ## _set) \
    diabloanoptarm_options.af_weight_ ## id = value; \
} while (0);

#define GET_WEIGHT(id) diabloanoptarm_options.af_weight_ ## id

struct ScoringData
{
  size_t min_regs_needed;
  size_t nr_regs_available;
  int nr_used_constants;
  int nr_executed_slices;
  t_possibility_flags flags;
  FactoringSetSourceInformation source_info;
  size_t nr_slices;

  std::string Print();
};

struct t_score;
enum class DispatcherType;

#define SCORE_CALCULATE_INIT_ARGS ()
#define SCORE_CALCULATE_ARGS (SliceSet& slices, size_t slice_size, ScoringData data)
#define SCORE_COMPARE_ARGS (const t_score& lhs, const t_score& rhs, t_randomnumbergenerator *rng)
#define CHOOSE_DISPATCHER_ARGS (t_possibility_flags flags, t_randomnumbergenerator *rng)

typedef void (*t_score_init) SCORE_CALCULATE_INIT_ARGS;
typedef t_score (*t_score_for_set) SCORE_CALCULATE_ARGS;
typedef bool (*t_compare_scores) SCORE_COMPARE_ARGS;
typedef DispatcherType (*t_dispatcher_chooser) CHOOSE_DISPATCHER_ARGS;

extern t_score_for_set CalculateScoreForSet;
extern t_compare_scores CompareScores;
extern t_score_init ScoreInit;
extern t_dispatcher_chooser DispatcherChooser;

struct t_score {
  enum class Type {
    Total,
    Partial,
    None
  };
  Type type;
  t_uint32 random_uid;

  /*  */
  using LargeScore = long double;

  static
  constexpr LargeScore LargeScoreMin() {
    return std::numeric_limits<LargeScore>::min();
  }

  static
  constexpr LargeScore LargeScoreBase() {
    return 0.0L;
  }

  static
  constexpr LargeScore LargeScoreHuge() {
    return HUGE_VALL;
  }

  LargeScore total_score;

  /* constructor */
  t_score(Type t) {
    total_score = LargeScoreMin();

    type = t;
    random_uid = RNGGenerate(af_rng);
  }

  t_score() : t_score(Type::None) {
  }

  /* comparator */
  bool operator> (const t_score& rhs) const {
    ASSERT(type == rhs.type, ("can't compare two different score types! %d %d", static_cast<int>(type), static_cast<int>(rhs.type)));
    ASSERT(type != t_score::Type::None, ("no type associated with score"));

    return CompareScores(*this, rhs, af_rng_compare);
  }

  /* to string */
  std::string to_string() {
    return std::to_string(total_score);
  }

  ScoringData data;
  size_t length;
};

#ifdef DIABLOSUPPORT_BUILD_SHARED_LIBS
#define SCORE_CALCULATE_INIT_NAME(x) ScoreCalculateInit
#define SCORE_CALCULATE_NAME(x) ScoreCalculate
#define SCORE_COMPARE_NAME(x) ScoreCompare
#define CHOOSE_DISPATCHER_NAME(x) DispatcherSelector

static const std::string DYNSYM_SCORE_CALCULATE_INIT = "ScoreCalculateInit";
static const std::string DYNSYM_SCORE_CALCULATE = "ScoreCalculate";
static const std::string DYNSYM_SCORE_COMPARE = "ScoreCompare";
static const std::string DYNSYM_CHOOSE_DISPATCHER = "DispatcherSelector";
#else
#define SCORE_CALCULATE_INIT_NAME(x) ScoreCalculateInit ## x
#define SCORE_CALCULATE_NAME(x) ScoreCalculate ## x
#define SCORE_COMPARE_NAME(x) ScoreCompare ## x
#define CHOOSE_DISPATCHER_NAME(x) DispatcherSelector ## x
#endif

#define DEFINE_SCORE_CALCULATE_INIT(x) EXPORT void SCORE_CALCULATE_INIT_NAME(x) SCORE_CALCULATE_INIT_ARGS
#define DEFINE_SCORE_CALCULATE(x) EXPORT t_score SCORE_CALCULATE_NAME(x) SCORE_CALCULATE_ARGS
#define DEFINE_SCORE_COMPARE(x) EXPORT bool SCORE_COMPARE_NAME(x) SCORE_COMPARE_ARGS
#define DEFINE_CHOOSE_DISPATCHER(x) EXPORT DispatcherType CHOOSE_DISPATCHER_NAME(x) CHOOSE_DISPATCHER_ARGS

#define LOAD_STATIC_SCORE(score, compare, fn_init, fn_calculate, fn_compare) {\
  DEFINE_SCORE_CALCULATE_INIT(score);                                         \
  DEFINE_SCORE_CALCULATE(score);                                              \
  DEFINE_SCORE_COMPARE(compare);                                              \
  fn_init = SCORE_CALCULATE_INIT_NAME(score);                                 \
  fn_calculate = SCORE_CALCULATE_NAME(score);                                 \
  fn_compare = SCORE_COMPARE_NAME(compare);                                   \
}

#define LOAD_STATIC_DISPATCHER_CHOOSER(chooser, fn_chooser) { \
  DEFINE_CHOOSE_DISPATCHER(chooser);                          \
  fn_chooser = CHOOSE_DISPATCHER_NAME(chooser);              \
}

static constexpr bool LHS_BETTER_THAN_RHS = true;
static constexpr bool RHS_BETTER_THAN_LHS = false;

#define SCORE_LIBNAME(x) "af" #x "Score"
#define DISPATCHER_CHOOSER_LIBNAME(x) "af" #x "DispatcherChooser"

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_SCORE_H */
