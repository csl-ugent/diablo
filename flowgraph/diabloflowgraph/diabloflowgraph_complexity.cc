/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "diabloflowgraph_complexity.hpp"

using namespace std;

/* Checks whether or not an edge corresponds to a direct computation, or an indirect one. */
t_bool IsCfgEdgeDirect(t_cfg_edge* edge, t_bool are_switches_direct_edges)
{
  t_bbl* head = CFG_EDGE_HEAD(edge);
  t_bbl* tail = CFG_EDGE_TAIL(edge);

  /* This is already insured by our caller here... */
  if (BBL_IS_HELL(head))
    return FALSE;

  /* If tail is BBL_IS_HELL, this does not necessarily imply that it is an indirect jump, it is usually
   * just a call to a library function (i.e., going through DYNHELL). So our technique of identifying the
   * indirect branches by looking at the instruction at the head is more reliable. */

  switch (CFG_EDGE_CAT(edge)) {
    case ET_FALLTHROUGH:
    case ET_IPFALLTHRU:
      return TRUE;

    case ET_JUMP:
    case ET_CALL:
    case ET_IPJUMP:
      {
        t_bool isdirect = FALSE;
        if (!BBL_INS_LAST(head)) {
          return TRUE; /* If a fallthrough comes from an empty BBL, it will be a direct transfer */
        }
        DiabloBrokerCall("InstructionIsDirectControlTransfer", BBL_INS_LAST(head), &isdirect);
        return isdirect;
      }

    case ET_SWITCH:
    case ET_IPSWITCH:
      return are_switches_direct_edges;

    case ET_SWI:
      return TRUE;

    case ET_COMPENSATING:
    case ET_IPUNKNOWN:
      return TRUE; /* TODO */

    case ET_RETURN:
      return TRUE; /* TODO */
    case ET_UNKNOWN:
    default:
      ASSERT(0, ("Edge type unknown found: @E", edge));
  }
}

/* todo: adapt to situation in which multiple CFGS exist */
/* todo: for example, by increasing the weight of instructions
   lifted from the static binary */

static FILE *stat_compl_file = NULL;
void CfgStaticComplexityInit(t_const_string fname) {
  if (!stat_compl_file)
    stat_compl_file = fopen(fname, "w+");
}

void CfgStaticComplexityFini() {
  if (stat_compl_file)
    fclose(stat_compl_file);

  stat_compl_file = NULL;
}

static FILE *dynamic_compl_file = NULL;
void CfgDynamicComplexityInit(t_const_string fname) {
  if (!dynamic_compl_file)
    dynamic_compl_file = fopen(fname, "w+");
}

void CfgDynamicComplexityFini() {
  if (dynamic_compl_file)
    fclose(dynamic_compl_file);

  dynamic_compl_file = NULL;
}


/* Get all (outgoing) edges originating in the set of BBLs that also end up in this set of BBLs.
   If function calls occur to functions that are not part of the edge set, two behaviors can occur:
   - keep those edges (interesting for cyclomatic_complexity, which otherwise might become a VERY negative number!
   - drop those edges
   */
vector<t_cfg_edge*> EdgesInBbls(const BblSet& bbls, bool keep_leaving_call_edges=true, bool dynamic_coverage=false) {
  vector<t_cfg_edge*> edges_restricted;

  t_cfg_edge* edge;
  for (auto bbl: bbls) {
    BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      if (dynamic_coverage && CFG_EDGE_EXEC_COUNT(edge) == 0 && !BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
        continue;

      if (bbls.find(CFG_EDGE_TAIL(edge)) != bbls.end()) {
        edges_restricted.push_back(edge);
      } else if (keep_leaving_call_edges && CFG_EDGE_CAT(edge) == ET_CALL) {
        edges_restricted.push_back(edge);
        /* Since the call target is not in the region, we'll never see the corresponding edge, add it manually: */
        if (CFG_EDGE_CORR(edge))
          edges_restricted.push_back(CFG_EDGE_CORR(edge));
      }
    }
  }

  return edges_restricted;
}

/* See comment about cyclomatic_complexity in EdgesInBbls: we count such function call targets here */
t_uint64 NrExternalCallTargets(const BblSet& bbls, bool dynamic_coverage) {
  set<t_bbl*> functions_seen;
  t_cfg_edge* edge;

  for (auto bbl: bbls) {
    BBL_FOREACH_SUCC_EDGE(bbl, edge) {
      /* TODO: verify if the edge counts of hell edges (in particular to library functions) are correctly computed */
      if (dynamic_coverage && CFG_EDGE_EXEC_COUNT(edge) == 0 && !BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
        continue;

      if (bbls.find(CFG_EDGE_TAIL(edge)) == bbls.end() && CFG_EDGE_CAT(edge) == ET_CALL) {
        functions_seen.insert(CFG_EDGE_TAIL(edge));
      }
    }
  }

  return functions_seen.size();
}

/* For both static and dynamic */
static void PrintComplexityMetricsHeaderBase(FILE* f, t_const_string prefix)
{
  fprintf(f, "nr_ins%s,nr_src_oper%s,nr_dst_oper%s,halstead_program_size%s,nr_edges%s,nr_indirect_edges_CFIM%s,", prefix, prefix, prefix, prefix, prefix, prefix);
}


void StaticComplexity::printHeader(FILE* f) {
  fprintf(f, "#region_idx,");
  PrintComplexityMetricsHeaderBase(f, "_static");
  fprintf(f, "cyclomatic_complexity_static\n");
}

/* region_index can be -1 => the entire CFG, but this way you can uniformly parse regions and the regular complexity info in the same way */
static void PrintComplexityCounts(FILE* f, const ComplexitySimpleCounts & complexity)
{
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_ins);
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_src_oper);
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_dst_oper);
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_ins + complexity.nr_src_oper + complexity.nr_dst_oper);
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_edges);
  fprintf(f, "%12" PRIu64 ", ", complexity.nr_indirect_edges);
}

void StaticComplexity::printComplexityMetricsLine(FILE* f, int region_index) {
  fprintf(f, "%i, ", region_index);
  PrintComplexityCounts(f, counts);
  fprintf(f, "%12" PRIi64 ", ", metrics.cyclomatic_complexity);
  fprintf(f, "\n");
}

void DynamicComplexity::printHeader(FILE* f) {
  fprintf(f, "#region_idx,");
  /* dynamic size */
  PrintComplexityMetricsHeaderBase(f, "_dynamic_size");
  /* dynamic coverage */
  PrintComplexityMetricsHeaderBase(f, "_dynamic_coverage");
  /* metrics */
  fprintf(f, "cyclomatic_complexity_dynamic_coverage\n");
}

void DynamicComplexity::printComplexityMetricsLine(FILE* f, int region_index) {
  fprintf(f, "%i, ", region_index);
  PrintComplexityCounts(f, dynamic_size);
  PrintComplexityCounts(f, dynamic_coverage);
  fprintf(f, "%12" PRIi64 ", ", metrics.cyclomatic_complexity);
  fprintf(f, "\n");
}

StaticComplexity BblsComputeStaticComplexity(const BblSet& bbls) {
  StaticComplexity complexity;
  t_ins* ins;

  for (auto bbl: bbls) {
    if (BBL_IS_HELL(bbl))
      continue;

    BBL_FOREACH_INS(bbl,ins)
    {
      complexity.counts.nr_ins ++;
      complexity.counts.nr_src_oper += RegsetCountRegs(INS_REGS_USE(ins));
      complexity.counts.nr_dst_oper += RegsetCountRegs(INS_REGS_DEF(ins));
    }
  }

  auto edges_restricted = EdgesInBbls(bbls, true /* keep_leaving_call_edges */, false /* dynamic_coverage */);

  for (auto edge: edges_restricted) {
    /* We definitely don't count edges *from* HELL, we only count the instructions's edges that go to hell */
    if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
      continue;

    complexity.counts.nr_edges++;

    if (!IsCfgEdgeDirect(edge, TRUE /* are_switches_direct_edges */)) {
      complexity.counts.nr_indirect_edges++;

      VERBOSE(1, ("Indirect edge: @E at the end of @eiB", edge, CFG_EDGE_HEAD(edge)));
    }
  }

  complexity.metrics.cyclomatic_complexity = complexity.counts.nr_edges - (bbls.size()+NrExternalCallTargets(bbls, false /* dynamic_coverage */)) + 2;

  return complexity;
}

/* TODO: maybe this could at the same time also compute static metrics, try to merge/factor these as much as possible! */
DynamicComplexity BblsComputeDynamicComplexity(const BblSet& bbls) {
  DynamicComplexity complexity;

  ComplexitySimpleCounts dynamic_size;
  ComplexitySimpleCounts dynamic_coverage;
  ComplexityStructureMetrics metrics;

  for (auto bbl: bbls) {
    if (BBL_IS_HELL(bbl))
      continue;

    t_ins* ins;
    if (BBL_EXEC_COUNT(bbl) >0 ) {
      BBL_FOREACH_INS(bbl, ins)
      {
        complexity.dynamic_size.nr_ins += BBL_EXEC_COUNT(bbl);
        complexity.dynamic_size.nr_src_oper += RegsetCountRegs(INS_REGS_USE(ins)) * BBL_EXEC_COUNT(bbl);
        complexity.dynamic_size.nr_dst_oper += RegsetCountRegs(INS_REGS_DEF(ins)) * BBL_EXEC_COUNT(bbl);

        complexity.dynamic_coverage.nr_ins += 1;
        complexity.dynamic_coverage.nr_src_oper += RegsetCountRegs(INS_REGS_USE(ins));
        complexity.dynamic_coverage.nr_dst_oper += RegsetCountRegs(INS_REGS_DEF(ins));
      }
    }
  }

  auto edges_restricted = EdgesInBbls(bbls, true /* keep_leaving_call_edges */, true /* dynamic_coverage */);

  for (auto edge: edges_restricted) {
    /* We definitely don't count edges *from* HELL, we only count the instructions's edges that go to hell */
    if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
      continue;

    if (CFG_EDGE_EXEC_COUNT(edge) == 0)
      continue;

    complexity.dynamic_size.nr_edges += CFG_EDGE_EXEC_COUNT(edge);
    complexity.dynamic_coverage.nr_edges += 1;

    if (!IsCfgEdgeDirect(edge, TRUE /* are_switches_direct_edges */)) {
      complexity.dynamic_size.nr_indirect_edges += CFG_EDGE_EXEC_COUNT(edge);
      complexity.dynamic_coverage.nr_indirect_edges += 1;

      VERBOSE(1, ("Indirect edge: @E at the end of @eiB", edge, CFG_EDGE_HEAD(edge)));
    }
  }

  complexity.metrics.cyclomatic_complexity = complexity.dynamic_size.nr_edges - (bbls.size()+NrExternalCallTargets(bbls, false /* dynamic_coverage */)) + 2;

  return complexity;
}

static void VerboseComplexityCounts(const ComplexitySimpleCounts& counts) {
  VERBOSE(0,("   NR INS      : %12" PRIu64 "", counts.nr_ins));
  VERBOSE(0,("   NR EDGES    : %12" PRIu64 "", counts.nr_edges));
  VERBOSE(0,("   NR IND.EDGES: %12" PRIu64 "", counts.nr_indirect_edges));
  VERBOSE(0,("   NR SRC OPER : %12" PRIu64 "", counts.nr_src_oper));
  VERBOSE(0,("   NR DST OPER : %12" PRIu64 "", counts.nr_dst_oper));
  VERBOSE(0,("                 --------------"));
  VERBOSE(0,("   TOTAL       : %9" PRIu64 "", counts.nr_ins + counts.nr_src_oper + counts.nr_dst_oper));
  VERBOSE(0,("                 --------------\n"));
}

void CfgComputeStaticComplexity(t_cfg * cfg)
{
  t_bbl * bbl;
  t_ins * ins;
  t_cfg_edge* edge;

  BblSet bbls;
  CFG_FOREACH_BBL(cfg,bbl) {
    bbls.insert(bbl);
  }

  StaticComplexity complexity = BblsComputeStaticComplexity(bbls);

  VERBOSE(0,("STATIC PROGRAM SIZE (SPS): static number of operations and operands in program"));
  VerboseComplexityCounts(complexity.counts);
  VERBOSE(0,("   CYCL COMPLEX: %9" PRIi64 "\n", complexity.metrics.cyclomatic_complexity));


  if (stat_compl_file)
  {
    complexity.printHeader(stat_compl_file);
    complexity.printComplexityMetricsLine(stat_compl_file, -1 /* entire file as region */);
  }
}

void CfgComputeDynamicComplexity(t_cfg * cfg)
{
  BblSet bbls;
  t_bbl* bbl;

  CFG_FOREACH_BBL(cfg,bbl) {
    bbls.insert(bbl);
  }
  DynamicComplexity complexity = BblsComputeDynamicComplexity(bbls);

  VERBOSE(0,("DYNAMIC PROGRAM SIZE (DPS): dynamic number of operations and operands in program"));
  if (complexity.dynamic_size.nr_ins == 0) {
    VERBOSE(0,("   NO PROFILE DATA AVAILABLE"));
  } else {
    VerboseComplexityCounts(complexity.dynamic_size);
  }

  VERBOSE(0,("DYNAMIC COVERAGE (DC): number of operations (and their operands) executed at least once"));
  if (complexity.dynamic_coverage.nr_ins == 0) {
    VERBOSE(0,("   NO PROFILE DATA AVAILABLE"));
  } else {
    VerboseComplexityCounts(complexity.dynamic_coverage);
    VERBOSE(0,("   CYCL COMPLEX: %9" PRIi64 "\n", complexity.metrics.cyclomatic_complexity));
  }

  if (dynamic_compl_file) {
    complexity.printHeader(dynamic_compl_file);
    complexity.printComplexityMetricsLine(dynamic_compl_file, -1 /* entire file as region */);
  }
}
