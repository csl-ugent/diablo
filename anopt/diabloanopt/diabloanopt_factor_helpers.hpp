#ifndef DIABLOANOPT_FACTOR_HELPERS_HPP
#define DIABLOANOPT_FACTOR_HELPERS_HPP

#include <set>
#include <string>

/* for starting new transformation log */
extern LogFile *L_FACTORING;
extern LogFile *L_FACTORING_VAR;

/* needed for debugcounters in AF */
extern t_uint32 nr_total_slices;

extern FunctionUID bbl_factor_function_uid;

struct FactoringSetSourceInformation {
  std::set<FunctionUID> functions, exec_functions;
  std::set<SourceFileUID> objects, exec_objects;
  std::set<SourceArchiveUID> archives, exec_archives;
};

struct FactoringResult {
  int nr_factored_insns;
  AddedInstructionInfo added_ins_info;

  FactoringResult() {
    nr_factored_insns = 0;
    added_ins_info = AddedInstructionInfo();
  }

  void Merge(FactoringResult& b) {
    nr_factored_insns += b.nr_factored_insns;
    added_ins_info.Merge(b.added_ins_info);
  }
};

void FactoringRecordTransformation(BblSet slices, size_t n_ins, FactoringResult insn_stats, bool only_source = false, bool regular_factoring = false);
void FactoringPrintStatistics();
void FactoringLogInstruction(t_ins *ins, std::string comment);

void FactoringLogInit(std::string filename, std::string filename_ins, std::string filename_statistics, bool advanced_factoring = false);
void FactoringLogFini();

FactoringSetSourceInformation FactoringGetSourceInformation(BblSet bbls, int& nr_executed, bool regular_factoring = false);

void FactoringLogComment(std::string x);

#endif /* DIABLOANOPT_FACTOR_HELPERS_HPP */
