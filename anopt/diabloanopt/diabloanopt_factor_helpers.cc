#include <diabloanopt.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

typedef map<t_uint32, t_uint32> i2imap;

i2imap size_histogram;
i2imap set_size_histogram;
i2imap archives_histogram;
i2imap objects_histogram;
i2imap functions_histogram;

t_uint32 nr_total_slices = 0;
t_uint32 nr_total_sets = 0;
t_uint32 nr_factored_insns = 0;

FactoringResult global_factoring_stats;

vector<SourceArchiveUID> archive_factored_insns;
vector<SourceFileUID> object_factored_insns;
vector<FunctionUID> function_factored_insns;

LogFile *L_FACTORING;
LogFile *L_FACTORING_INS;
LogFile *L_FACTORING_STATISTICS;
LogFile *L_FACTORING_VAR;

FunctionUID bbl_factor_function_uid;

FactoringSetSourceInformation FactoringGetSourceInformation(BblSet bbls, int& nr_executed) {
  FactoringSetSourceInformation result;
  nr_executed = 0;

  for (auto bbl : bbls) {
    /* look up the origin of the slice */
    FunctionUID function;
    SourceFileUID object;
    SourceArchiveUID archive;
    BblSourceLocation(bbl, function, object, archive);

    result.functions.insert(function);
    result.objects.insert(object);
    result.archives.insert(archive);

    if (BBL_EXEC_COUNT(bbl) > 0) {
      result.exec_functions.insert(function);
      result.exec_objects.insert(object);
      result.exec_archives.insert(archive);

      nr_executed++;
    }
  }

  return result;
}

static
FactoringSetSourceInformation BblFactorHolderGetSourceInformation(t_equiv_bbl_holder *equivs, t_bool *can_factor) {
  BblSet bbls;

  for (int i = 0; i < equivs->nbbls; i++) {
    if (can_factor
        && !can_factor[i])
      continue;

    bbls.insert(equivs->bbl[i]);
  }

  int nr_executed = 0;
  return FactoringGetSourceInformation(bbls, nr_executed);
}

extern "C"
t_bool BblFactoringHolderConsiderForFactoring(t_equiv_bbl_holder *equivs, t_bool *can_factor) {
  auto source_info = BblFactorHolderGetSourceInformation(equivs, can_factor);

  /* covered */
  if (source_info.functions.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_functions))
    return FALSE;
  if (source_info.objects.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_objects))
    return FALSE;
  if (source_info.archives.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_archives))
    return FALSE;

  /* executed */
  if (source_info.exec_functions.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_functions))
    return FALSE;
  if (source_info.exec_objects.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_objects))
    return FALSE;
  if (source_info.exec_archives.size() < static_cast<size_t>(diabloanopt_options.factor_min_covered_exec_archives))
    return FALSE;

  return TRUE;
}

void FactoringLogInstruction(t_ins *ins, string x) {
  /* NOTE: if additional fields need to be emitted,
   *        include them colon-separated at the front.
   *        This is because the printed instruction may contain additional colons. */
  t_string insn = StringIo("@I", ins);
  LOG_MESSAGE(L_FACTORING_INS, "%s:%s:%d:%s\n", GetTransformationIdString().c_str(), x.c_str(), (BBL_EXEC_COUNT(INS_BBL(ins)) > 0), insn);
  Free(insn);
}

void FactoringLogComment(string x) {
  LOG_MESSAGE(L_FACTORING_VAR, "%s\n", x.c_str());
}

static
SpecialFunctionTrackResults BblFactoringOriginTracking(t_bbl *bbl) {
  SpecialFunctionTrackResults result;

  /* calculate associated-with count */
  result.associated_with_functions.clear();

  t_bbl *entry = FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl));

  t_int64 total_exec = 0;

  t_cfg_edge *e;
  BBL_FOREACH_PRED_EDGE(entry, e) {
    ASSERT(CFG_EDGE_CAT(e) == ET_CALL, ("unexpected edge @E", e));

    FunctionUID f_uid = BblOriginalFunctionUID(CFG_EDGE_HEAD(e));
    ASSERT(f_uid != FunctionUID_INVALID, ("expected incoming function to be associated with original! @F @eiB", CFG_EDGE_HEAD(e), entry));

    result.associated_with_functions.insert(f_uid);

    result.exec_per_function[f_uid] = BBL_EXEC_COUNT(CFG_EDGE_HEAD(e));
    total_exec += BBL_EXEC_COUNT(CFG_EDGE_HEAD(e));
  }

  if (total_exec != BBL_EXEC_COUNT(entry))
    CfgDrawFunctionGraphs(BBL_CFG(entry), "exec");
  ASSERT(total_exec == BBL_EXEC_COUNT(entry), ("what? @eiB %d/%d", entry, total_exec, BBL_EXEC_COUNT(entry)));

  return result;
}

void FactoringLogInit(string filename, string filename_ins, string filename_statistics, bool advanced_factoring) {
  static bool init = false;
  ASSERT(!init, ("should be called only once"));
  init = true;

  if (!advanced_factoring) {
    /* object tracking */
    bbl_factor_function_uid = RegisterSpecialFunctionType(BblFactoringOriginTracking);
    VERBOSE(0, ("factored BBL functions will get UID %d (%x)", bbl_factor_function_uid, bbl_factor_function_uid));
  }

  INIT_LOGGING(L_FACTORING, filename.c_str());
  LOG_MESSAGE(L_FACTORING, "# tf_uid,tf_uid_hex,tf_type,function_name,block_size,")
  LOG_MESSAGE(L_FACTORING, "nr_blocks,nr_exec_blocks,nr_archives,nr_objects,nr_functions,nr_exec_archives,nr_exec_objects,nr_exec_functions,");
  if (!advanced_factoring)
    LOG_MESSAGE(L_FACTORING, "nr_blocks_can,nr_exec_blocks_can,nr_archives_can,nr_objects_can,nr_functions_can,nr_exec_archives_can,nr_exec_objects_can,nr_exec_functions_can,");
  LOG_MESSAGE(L_FACTORING, "nr_added_ins_static,nr_added_data_static,nr_added_ins_dynamic,nr_factored_ins\n");

  INIT_LOGGING(L_FACTORING_INS, filename_ins.c_str());
  LOG_MESSAGE(L_FACTORING_INS, "# tf_uid:FACTORED:block_id:executed:instruction\n");
  LOG_MESSAGE(L_FACTORING_INS, "# tf_uid:(other reason):executed:instruction\n");

  INIT_LOGGING(L_FACTORING_STATISTICS, filename_statistics.c_str());
  /* header is printed in FactoringPrintStatistics */

  string x = filename_ins + ".comments";
  INIT_LOGGING(L_FACTORING_VAR, x.c_str());
  LOG_MESSAGE(L_FACTORING_VAR, "# various comments for factoring\n");
}

void FactoringLogFini() {
  LOG_MESSAGE(L_FACTORING, "# end");
  FINI_LOGGING(L_FACTORING);

  LOG_MESSAGE(L_FACTORING_INS, "# end");
  FINI_LOGGING(L_FACTORING_INS);

  LOG_MESSAGE(L_FACTORING_STATISTICS, "# end");
  FINI_LOGGING(L_FACTORING_STATISTICS);

  LOG_MESSAGE(L_FACTORING_VAR, "# end");
  FINI_LOGGING(L_FACTORING_VAR);
}

void FactoringRecordTransformation(BblSet slices, size_t n_ins, FactoringResult insn_stats, bool only_source) {
  /* object files and archives */
  int nr_executed = 0;
  FactoringSetSourceInformation source_info = FactoringGetSourceInformation(slices, nr_executed);

  if (!only_source) {
    global_factoring_stats.Merge(insn_stats);

    /* slice size statistics */
    size_histogram[n_ins] += slices.size();

    /* set size statistics */
    set_size_histogram[slices.size()]++;

    /* counters */
    for (auto i : source_info.functions) {
      if (i >= function_factored_insns.size())
        function_factored_insns.resize(i+1);
      function_factored_insns[i] += n_ins;
    }
    for (auto i : source_info.objects) {
      if (i >= object_factored_insns.size())
        object_factored_insns.resize(i+1);
      object_factored_insns[i] += n_ins;
    }
    for (auto i : source_info.archives) {
      if (i >= archive_factored_insns.size())
        archive_factored_insns.resize(i+1);
      archive_factored_insns[i] += n_ins;
    }

    archives_histogram[source_info.archives.size()]++;
    objects_histogram[source_info.objects.size()]++;
    functions_histogram[source_info.functions.size()]++;

    nr_total_sets++;
    nr_total_slices += slices.size();
  }

  /* generic */
  LOG_MESSAGE(L_FACTORING, "%d,%d,", slices.size(), nr_executed);
  /* coverage */
  LOG_MESSAGE(L_FACTORING, "%d,%d,%d,", source_info.archives.size(), source_info.objects.size(), source_info.functions.size());
  /* executed coverage */
  LOG_MESSAGE(L_FACTORING, "%d,%d,%d,", source_info.exec_archives.size(), source_info.exec_objects.size(), source_info.exec_functions.size());

  if (!only_source) {
    /* added instruction data */
    LOG_MESSAGE(L_FACTORING, "%d,%d,%d,", insn_stats.added_ins_info.nr_added_insns, insn_stats.added_ins_info.nr_added_data, insn_stats.added_ins_info.nr_added_insns_dyn);
    /* factoring information */
    LOG_MESSAGE(L_FACTORING, "%d\n", insn_stats.nr_factored_insns);
  }
}

void FactoringPrintStatistics() {
  auto printer = [] (string prefix, i2imap m) {
    for (auto it = m.begin(); it != m.end(); ++it)
      LOG_MESSAGE(L_FACTORING_STATISTICS, "%s:%u:%u\n", prefix.c_str(), it->first, it->second);
  };

  LOG_MESSAGE(L_FACTORING_STATISTICS, "# (histogram) type:x:x_count\n");
  printer("hist_setsize", set_size_histogram);
  printer("hist_slicesize", size_histogram);
  printer("hist_archives", archives_histogram);
  printer("hist_objects", objects_histogram);
  printer("hist_functions", functions_histogram);

  LOG_MESSAGE(L_FACTORING_STATISTICS, "# (numbers) type:x\n");
  LOG_MESSAGE(L_FACTORING_STATISTICS, "total_slices:%d\n", nr_total_slices);
  LOG_MESSAGE(L_FACTORING_STATISTICS, "total_sets:%d\n", nr_total_sets);

  LOG_MESSAGE(L_FACTORING_STATISTICS, "total_insns:%d\n", global_factoring_stats.nr_factored_insns);
  LOG_MESSAGE(L_FACTORING_STATISTICS, "delta_insns:%d\n", global_factoring_stats.added_ins_info.nr_added_insns);
  LOG_MESSAGE(L_FACTORING_STATISTICS, "delta_insns_dyn:%d\n", global_factoring_stats.added_ins_info.nr_added_insns_dyn);
  LOG_MESSAGE(L_FACTORING_STATISTICS, "delta_data:%d\n", global_factoring_stats.added_ins_info.nr_added_data);

  LOG_MESSAGE(L_FACTORING_STATISTICS, "# (origin tracking) type:uid:factored_insn_count:total_insn_count\n");
  for (size_t i = 0; i < archive_factored_insns.size(); i++)
    LOG_MESSAGE(L_FACTORING_STATISTICS, "archive_factored_total_insns:%d:%d:%d\n", i, archive_factored_insns[i], GetArchiveInstructionCount(i));

  for (size_t i = 0; i < object_factored_insns.size(); i++)
    LOG_MESSAGE(L_FACTORING_STATISTICS, "object_factored_total_insns:%d:%d:%d\n", i, object_factored_insns[i], GetFileInstructionCount(i));

  for (size_t i = 0; i < function_factored_insns.size(); i++)
    LOG_MESSAGE(L_FACTORING_STATISTICS, "function_factored_total_insns:%d:%d:%d\n", i, function_factored_insns[i], GetFunctionInstructionCount(i));
}
