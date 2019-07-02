/* These functions are only usable from C++ files */
#ifndef DIABLO_COMPLEXITY_HPP
#define DIABLO_COMPLEXITY_HPP
#include "diabloflowgraph.hpp"

struct ComplexitySimpleCounts {
  t_uint64 nr_ins = 0;
  t_uint64 nr_src_oper = 0;
  t_uint64 nr_dst_oper = 0;
  t_uint64 nr_edges = 0;
  t_uint64 nr_indirect_edges = 0;
};

struct ComplexityStructureMetrics {
  t_int64 cyclomatic_complexity = 0;
};

struct Complexity {
  virtual ~Complexity() {}
  virtual void printHeader(FILE* f) {}
  /* region_index can be -1 => the entire CFG, but this way you can uniformly parse regions and the regular complexity info in the same way */
  virtual void printComplexityMetricsLine(FILE* f, int region_index) {}
};

struct StaticComplexity : public Complexity {
  ComplexitySimpleCounts counts;
  ComplexityStructureMetrics metrics;

  virtual void printHeader(FILE* f);
  /* region_index can be -1 => the entire CFG, but this way you can uniformly parse regions and the regular complexity info in the same way */
  virtual void printComplexityMetricsLine(FILE* f, int region_index);
};

struct DynamicComplexity : public Complexity {
  ComplexitySimpleCounts dynamic_size;
  ComplexitySimpleCounts dynamic_coverage;
  ComplexityStructureMetrics metrics;

  virtual void printHeader(FILE* f);
  /* region_index can be -1 => the entire CFG, but this way you can uniformly parse regions and the regular complexity info in the same way */
  virtual void printComplexityMetricsLine(FILE* f, int region_index);
};

struct DynamicSizes : public StaticComplexity {
  t_uint64 nr_ins = 0;
  t_uint64 nr_src_oper = 0;
  t_uint64 nr_dst_oper = 0;
  t_uint64 nr_edges = 0;
  t_uint64 nr_indirect_edges = 0;
  t_int64 cyclomatic_complexity = 0;
};


StaticComplexity BblsComputeStaticComplexity(const BblSet& bbls);
/* TODO: maybe this could at the same time also compute static metrics */
DynamicComplexity BblsComputeDynamicComplexity(const BblSet& bbls);

struct StaticComplexityOrigin {
  std::vector<StaticComplexity> archives;
  std::vector<StaticComplexity> objects;
  std::vector<StaticComplexity> functions;
};

struct DynamicComplexityOrigin {
  std::vector<DynamicComplexity> archives;
  std::vector<DynamicComplexity> objects;
  std::vector<DynamicComplexity> functions;
};
void CfgComputeDynamicComplexityOrigin(t_cfg * cfg, size_t nr_archives, size_t nr_objects, size_t nr_functions, std::map<FunctionUID, t_bbl *> function_to_bbl);

#endif
