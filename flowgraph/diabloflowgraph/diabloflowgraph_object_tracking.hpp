#ifndef DIABLOFLOWGRAPH_OBJECT_TRACKING_HPP
#define DIABLOFLOWGRAPH_OBJECT_TRACKING_HPP

/*
 * SpecialFunctionTrackResults my_special_function_tracker(t_bbl *bbl) {
 *   SpecialFunctionTrackResults result;
 *
 *   // TODO: fill in 'result' variable
 *
 *   return result;
 * }
 *
 * In the initialisation routine of your transformation:
 * FunctionUID my_special_function_uid = RegisterSpecialFunctionType(my_special_function_tracker);
 *
 * When creating a new function:
 * t_function *fun = FunctionMake(entry, "foo", FT_NORMAL);
 * BblSetOriginalFunctionUID(entry, my_special_function_uid);
 */

typedef size_t ObjectUID;
typedef size_t SourceFileUID;
typedef size_t SourceArchiveUID;

typedef std::set<SourceArchiveUID> SourceArchiveSet;
typedef t_uint64 SourceArchiveBitset;

#define ObjectUID_INVALID std::numeric_limits<ObjectUID>::max()
#define ObjectSetUID_INVALID std::numeric_limits<ObjectSetUID>::max()
#define FunctionUID_INVALID std::numeric_limits<FunctionUID>::max()

/* use dynamic special function UIDs */

struct SpecialFunctionTrackResults {
  std::set<FunctionUID> associated_with_functions;
  std::map<FunctionUID, t_uint64> exec_per_function;
};

typedef std::function<SpecialFunctionTrackResults(t_bbl *)> SpecialFunctionTrackResultsHandler;
FunctionUID RegisterSpecialFunctionType(SpecialFunctionTrackResultsHandler handler);

ObjectUID BblObjectIndex(t_bbl *bbl);

SourceArchiveUID GetArchiveUID(ObjectUID idx);
std::string GetArchiveName(SourceArchiveUID idx);

void RecordFunctionsAsOriginal(t_cfg *cfg);

/* inline functions */
extern "C" std::vector<t_uint32> nr_instructions_in_file;
static inline
size_t GetFileInstructionCount(SourceFileUID idx) {
  return nr_instructions_in_file[idx];
}

extern "C" std::vector<t_uint32> nr_instructions_in_function;
static inline
size_t GetFunctionInstructionCount(FunctionUID idx) {
  return nr_instructions_in_function[idx];
}

extern "C" std::vector<t_uint32> nr_instructions_in_archive;
static inline
size_t GetArchiveInstructionCount(SourceArchiveUID idx) {
  return nr_instructions_in_archive[idx];
}

struct ObjectAndRange
{
  size_t archive_id;
  size_t filename_id;
  t_address begin, end;
};
extern "C" std::vector<ObjectAndRange> objects_and_ranges;
static inline
SourceFileUID GetFileUID(ObjectUID idx) {
  return objects_and_ranges[idx].filename_id;
}

void BblSourceLocation(t_bbl *bbl, FunctionUID& function, SourceFileUID& file, SourceArchiveUID& archive);

struct TrackingInformation {
  std::set<FunctionUID> functions;
  std::set<SourceFileUID> files;
  std::set<SourceArchiveUID> archives;

  std::map<FunctionUID, t_uint64> exec_per_function;

  TrackingInformation();
  void Merge(TrackingInformation x);
};

void BblInitAssociatedInfo(t_cfg *cfg);
void BblFiniAssociatedInfo(t_cfg *cfg);
TrackingInformation *BblAssociatedInfo(t_bbl *bbl);
void BblCopyAssociatedInfo(t_bbl *from, t_bbl *to);
SourceArchiveBitset SourceArchiveSetToBitset(SourceArchiveSet s);
TrackingInformation CalculateAssociatedWith(t_bbl *bbl);
void OnlyDoSccInformation(bool x);

#endif
