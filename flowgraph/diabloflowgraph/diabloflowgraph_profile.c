/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h>
#include <unistd.h>
#ifdef _MSC_VER
#include <stdio.h>
#include <io.h>
#endif
#include <diabloflowgraph.h>

// #define DEBUG_PROFILING

#define BSEARCH 1

typedef struct _t_address_to_bbl_he t_address_to_bbl_he;
typedef struct _t_address_to_ins_he t_address_to_ins_he;
typedef struct _t_ins_he t_ins_he;

struct _t_address_to_bbl_he
{
  t_hash_table_node node;
  t_bbl *bbl;
};

struct _t_address_to_ins_he
{
  t_hash_table_node node;
  t_ins *ins;
};

struct _t_ins_he
{
  t_hash_table_node node;
  t_ins *ins;
  t_ins_he *ident;
};



/* Global variables for the self-profiling code */
static t_symbol* nr_of_bbls_sym  = NULL;
static t_symbol* output_name_sym = NULL;
static t_symbol* ps_sym = NULL;

/* This function initializes self-profiling by linking in the profiling object (and any other necessary objects)
 * and performing some initialization on it.
 */
void SelfProfilingInit (t_object* obj, t_string profiling_object_path)
{
  /* Depending on whether the path is that of an actual object, link it in or
   * assume it is already present.
   */
  if (StringPatternMatch("*.o", profiling_object_path))
  {
    /* If the object doesn't have a PLT, check whether all needed symbols are already present in the binary. If not, link in their objects. */
    if(OBJECT_DYNAMIC_SYMBOL_TABLE(obj) == NULL)
    {
      if(!SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), "fopen"))
        LinkObjectFileNew (obj, "libc.a:fopen.o", "", FALSE, FALSE, NULL);

      if(!SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), "fwrite"))
        LinkObjectFileNew (obj, "libc.a:fwrite.o", "", FALSE, FALSE, NULL);

      if(!SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), "fclose"))
        LinkObjectFileNew (obj, "libc.a:fclose.o", "", FALSE, FALSE, NULL);
    }

    /* Link in the actual profiling object */
    LinkObjectFileNew (obj, profiling_object_path, PREFIX_FOR_LINKED_IN_SP_OBJECT, FALSE, TRUE, NULL);
  }

  /* Initialization: some things that have to be performed on the linked in profiling object before flowgraphing */
  nr_of_bbls_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), SP_IDENTIFIER_PREFIX "nr_of_bbls");
  output_name_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), SP_IDENTIFIER_PREFIX "output_name");
  ps_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), SP_IDENTIFIER_PREFIX "data");
  t_symbol* init_sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), SP_IDENTIFIER_PREFIX "Init");

  /* Check if all symbols were found */
  ASSERT(ps_sym && nr_of_bbls_sym && output_name_sym && init_sym, ("Didn't find all symbols present in the print object! Are you sure this object was linked in?"));

  /* Adapt the program so before start the initialization routine is executed */
  if (SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), FINAL_PREFIX_FOR_LINKED_IN_SP_OBJECT "Init"))
      DiabloBrokerCall ("AddInitializationRoutine", obj, init_sym, false);
}

/* Add self-profiling to an object. This assumes print-object containing the code to print out the profiling information
 * has been linked in. This linked in object also contains some variables that need to be filled in by Diablo.
 */
void CfgAddSelfProfiling (t_object* obj, t_string output_name)
{
  t_bbl* bbl;
  t_uint32 nr_of_bbls = 0;
  t_uint32 size;
  t_cfg* cfg = OBJECT_CFG(obj);
  t_string base_name = FileNameBase(output_name);
  t_string filename = NULL;
  t_bbl** list = Malloc(sizeof(t_bbl*));
  t_bool have_line_number_info = FALSE;
  list[0] = NULL;
  t_string src_infofilename = StringConcat3(output_name,".","src_profile_info");
  t_string complexity_infofilename = StringConcat3(output_name,".","dyn_complexity_info");
  FILE* fp_src_info;
  FILE* fp_complexity_info;
  t_ins * prev_ins, * ins;

  static int nr_blocks = 0;

  CFG_FOREACH_BBL(cfg,bbl)
    BBL_FOREACH_INS(bbl,ins)
    {
      if (INS_SRC_FILE(ins)) have_line_number_info = TRUE;
      break;
    }

  if (have_line_number_info)
    {
      fp_src_info=fopen(src_infofilename,"w");
      ASSERT(fp_src_info, ("could not open '%s' for writing", src_infofilename));
      fprintf(fp_src_info,"#<BBL OLD ADDRESS> <SRC FILE> <SRC LINE NUMBER> <NR. INS> <FUNCTION NAME> <BBL IS ENTRY FUNCTION?>\n");
    }

  fp_complexity_info=fopen(complexity_infofilename,"w");

  Free(complexity_infofilename);
  Free(src_infofilename);

  STATUS(START, ("Adding self-profiling"));

  /* Iterate over all BBLs, add the real ones to a list, and print out their addresses in the original binary */
  CFG_FOREACH_BBL(cfg, bbl)
  {
    /* Make sure it isn't empty, a DATA-BBL, or part of the linked in code */
    if ((BBL_NINS(bbl) != 0) && !IS_DATABBL(bbl) &&
        (!BBL_FUNCTION(bbl) || !FUNCTION_NAME(BBL_FUNCTION(bbl)) || !StringPatternMatch (PREFIX_FOR_LINKED_IN_SP_OBJECT"*", FUNCTION_NAME(BBL_FUNCTION(bbl)))))
    {
      list = Realloc(list, (nr_of_bbls+2)*sizeof(t_bbl*));
      list[nr_of_bbls] = bbl;
      list[nr_of_bbls+1] = NULL;
      nr_of_bbls++;
    }
  }

  /* Set all the variables in the linked in print-object */
  SectionSetData32 (T_SECTION(SYMBOL_BASE(nr_of_bbls_sym)), SYMBOL_OFFSET_FROM_START(nr_of_bbls_sym), nr_of_bbls);

  /* For every BBL to be profiled this section contains its address in the original program (in 64-bit for compatibility),
   * and the number of times it has been executed (also 64-bit value).
   */
  t_section* profiling_sec = T_SECTION(SYMBOL_BASE(ps_sym));
  size = (nr_of_bbls)*2*sizeof (t_uint64);
  SECTION_SET_DATA(profiling_sec, Realloc(SECTION_DATA(profiling_sec), size));
  memset(SECTION_DATA(profiling_sec), 0, size);
  SECTION_SET_CSIZE(profiling_sec, size);

  t_section* sequencing_counter_sec = NULL;
  if (diabloflowgraph_options.sp_sequence) {
    sequencing_counter_sec = SectionCreateForObject(ObjectGetLinkerSubObject(obj), DATA_SECTION, SectionGetFromObjectByName(obj, ".data"), AddressNew32(0), "DIABLO_sequencing_counter");
    SECTION_SET_DATA(sequencing_counter_sec, Realloc(SECTION_DATA(sequencing_counter_sec), 4));
    memset(SECTION_DATA(sequencing_counter_sec), 0, 4);
    SECTION_SET_CSIZE(sequencing_counter_sec, 4);
  }

  t_section* name_sec = T_SECTION(SYMBOL_BASE(output_name_sym));

  if (diabloflowgraph_options.sp_sequence)
    filename = StringConcat2("sequencing_data.", base_name);
  else
    filename = StringConcat2("profiling_data.", base_name);
  Free(base_name);

  size = strlen (filename) + 1;
  SECTION_SET_DATA(name_sec, Realloc(SECTION_DATA(name_sec), size));
  SECTION_SET_CSIZE(name_sec, size);
  memcpy (SECTION_DATA(name_sec), filename, strlen (filename) + 1);
  Free(filename);

  if (diabloflowgraph_options.sp_sequence) {
    /* for sequencing purposes */
    t_section *init_array_section = NULL;
    t_section *sec;
    int tel;
    OBJECT_FOREACH_SECTION(obj, sec, tel) {
      if (!strncmp(SECTION_NAME(sec), ".init_array", strlen(".init_array"))) {
        ASSERT(!init_array_section, ("duplicate section @T", init_array_section));
        init_array_section = sec;
      }
    }
    ASSERT(init_array_section, ("can't find section .init_array"));

    t_section *init_array_vector = NULL;
    SECTION_FOREACH_SUBSECTION(init_array_section, sec) {
      ASSERT(!init_array_vector, ("duplicate vector @T", sec));
      init_array_vector = sec;
    }

    t_reloc *reloc_init_done = NULL;
    t_address max_offset = AddressNew32(0);
    for (t_reloc_ref *rr = SECTION_REFERS_TO(init_array_vector); rr; rr = RELOC_REF_NEXT(rr)) {
      t_reloc *rel = RELOC_REF_RELOC(rr);

      t_relocatable *to = RELOC_TO_RELOCATABLE(rel)[0];
      ASSERT(RELOCATABLE_RELOCATABLE_TYPE(to) == RT_BBL, ("expected relocation to BBL @R", rel));

      t_bbl *bbl = T_BBL(to);
      ASSERT(BBL_FUNCTION(bbl), ("expected to to be in function @eiB @R", bbl, rel));

      if (!strcmp(FUNCTION_NAME(BBL_FUNCTION(bbl)), SP_IDENTIFIER_PREFIX "InitDone")) {
        ASSERT(!reloc_init_done, ("duplicate relocation to %s @R", SP_IDENTIFIER_PREFIX "InitDone", rel));
        reloc_init_done = rel;
      }

      if (AddressIsGt(RELOC_FROM_OFFSET(rel), max_offset))
        max_offset = RELOC_FROM_OFFSET(rel);
    }

    /* we should have found the one */
    ASSERT(reloc_init_done, ("can't find relocation to function '%s'", SP_IDENTIFIER_PREFIX "InitDone"));

    for (t_reloc_ref *rr = SECTION_REFERS_TO(init_array_vector); rr; rr = RELOC_REF_NEXT(rr)) {
      t_reloc *rel = RELOC_REF_RELOC(rr);

      if (AddressIsLt(RELOC_FROM_OFFSET(rel), RELOC_FROM_OFFSET(reloc_init_done))) {
        /* entry before the one, no need to do anything */
      }
      else if (rel == reloc_init_done) {
        /* the one */
        RELOC_SET_FROM_OFFSET(rel, max_offset);
      }
      else {
        /* entry after the one, need to subtract 4 */
        RELOC_SET_FROM_OFFSET(rel, AddressSubUint32(RELOC_FROM_OFFSET(rel), 4));
      }
    }
  }

  /* Iterate over all BBLs again, add instrumentation with relocation to profiling section */
  nr_of_bbls = 0;
  while (list[nr_of_bbls])
  {
    bbl = list[nr_of_bbls];

    /* produce src line info and more in output file */
    if (have_line_number_info)
      {
       prev_ins = NULL;
       t_string fun_name = "noname";
       char entry = '0';
       t_uint32 nr_ins = 0;
       t_uint32 nr_src_operands = 0;
       t_uint32 nr_dst_operands = 0;
       t_bool has_indirect_outgoing_edge = FALSE;
       t_cfg_edge * edge;
       if (BBL_FUNCTION(bbl) && FUNCTION_NAME(BBL_FUNCTION(bbl)))
         {
           fun_name = FUNCTION_NAME(BBL_FUNCTION(bbl));
           if (bbl==FUNCTION_BBL_FIRST(BBL_FUNCTION(bbl)))
             entry = '1';
         }

       BBL_FOREACH_SUCC_EDGE(bbl, edge)
         {
           if (!IsCfgEdgeDirect(edge, TRUE /* are_switches_direct_edges */)) 
             has_indirect_outgoing_edge = TRUE;
         }


       BBL_FOREACH_INS(bbl,ins)
         {
           nr_ins++;
           nr_src_operands += RegsetCountRegs(INS_REGS_USE(ins));
           nr_dst_operands += RegsetCountRegs(INS_REGS_DEF(ins));

           if (INS_SRC_FILE(ins))
             {
               if (prev_ins && INS_SRC_LINE(ins)==INS_SRC_LINE(prev_ins) && !StringCmp(INS_SRC_FILE(ins),INS_SRC_FILE(prev_ins)))
                 continue;
               fprintf(fp_src_info,"0x%x\t%s\t%d\t%d\t%s\t%c\n",BBL_OLD_ADDRESS(bbl),INS_SRC_FILE(ins),INS_SRC_LINE(ins),BBL_NINS(bbl),fun_name,entry);
               prev_ins = ins;
             }
         }

       fprintf(fp_complexity_info,"0x%x\t%d\t%d\t%d\t%d\n",BBL_OLD_ADDRESS(bbl),nr_ins,nr_src_operands,nr_dst_operands,has_indirect_outgoing_edge);
       
       if (prev_ins == NULL)
         {
           if (StringCmp(fun_name,"noname"))
             fprintf(fp_src_info,"0x%x\t-\t-\t%d\t%s\t%c\n",BBL_OLD_ADDRESS(bbl),BBL_NINS(bbl),fun_name,entry);
         }
      }

    /* Add instrumentation at the start of the BBL, it will take care of incrementing the
     * appropriate entry in the profiling section
     */
    t_address offset = nr_of_bbls*2*sizeof (t_uint64);/* Offset in the profiling section */

#ifdef DEBUG_PROFILING
    if (nr_blocks >= diablosupport_options.debugcounter) {
      VERBOSE(0, ("SP-SKIP @iB", bbl));
    }
    else
#endif
    {
      DiabloBrokerCall ("AddInstrumentationToBbl", obj, bbl, profiling_sec, sequencing_counter_sec, offset);
      nr_blocks++;
    }

    /* Write the BBL's address in the old binary to appropriate place in the profiling section */
    SectionSetData64 (profiling_sec, offset, BBL_OLD_ADDRESS(bbl));

    /* Go to next */
    nr_of_bbls++;

    //VERBOSE(0, ("@B\n", bbl));
    //if (nr_of_bbls >= diablosupport_options.debugcounter) break;
  }

 if (have_line_number_info)
    fclose(fp_src_info);

 fclose(fp_complexity_info);

  /* Cleanup and exit */
  Free(list);

  STATUS(STOP, ("Adding self-profiling %d", nr_blocks));
}

void
CfgEstimateEdgeCounts (t_cfg * cfg)
{
  t_cfg_edge *edge;
  t_bool flag, innerflag;
  t_int64 count;
  t_function *ifun;
  t_bbl *bbl;

  CFG_FOREACH_EDGE(cfg, edge)
  {
    t_bbl *head = CFG_EDGE_HEAD(edge);
    t_bbl *tail = CFG_EDGE_TAIL(edge);

    CFG_EDGE_SET_EXEC_COUNT(edge, -1LL);

    if (!BBL_FUNCTION(tail))
      CfgDrawFunctionGraphs(cfg, "tail-not-function");
    ASSERT(BBL_FUNCTION(head), ("head not in function @eiB", head));
    ASSERT(BBL_FUNCTION(tail), ("tail not in function @eiB", tail));
    if ((head != FunctionGetExitBlock (BBL_FUNCTION(head)) && BBL_EXEC_COUNT(head) == 0) || (tail != FunctionGetExitBlock (BBL_FUNCTION(tail)) && BBL_EXEC_COUNT(tail) == 0))
    {
      CFG_EDGE_SET_EXEC_COUNT(edge, 0LL);
      VERBOSE(10, ("ola 1 count know for edge @E\n of @ieB\n", edge, CFG_EDGE_HEAD(edge)));
    }
    else if (BBL_SUCC_FIRST(head) == BBL_SUCC_LAST(head) && head != FunctionGetExitBlock (BBL_FUNCTION(head)))
    {
      CFG_EDGE_SET_EXEC_COUNT(edge, BBL_EXEC_COUNT(head));
      VERBOSE(10, ("ola 2 count know for edge @E\n of @ieB\n", edge, CFG_EDGE_HEAD(edge)));
    }
    else if (BBL_PRED_FIRST(tail) == BBL_PRED_LAST(tail) && tail != FunctionGetExitBlock (BBL_FUNCTION(tail)))
    {
      CFG_EDGE_SET_EXEC_COUNT(edge, BBL_EXEC_COUNT(tail));
      VERBOSE(10, ("ola 3 count know for edge @E\n of @ieB\n", edge, CFG_EDGE_HEAD(edge)));
    }
    else if (BBL_INS_LAST(head) && CFG_DESCRIPTION(cfg)->InsIsControlflow (BBL_INS_LAST(head)) && edge == TakenPath (head) && diabloflowgraph_options.insprofilefile)
    {
      CFG_EDGE_SET_EXEC_COUNT(edge, INS_EXEC_COUNT(BBL_INS_LAST(head)));
      VERBOSE(10, ("aha count know for edge @E\n of @ieB\n", edge, CFG_EDGE_HEAD(edge)));
    }
    else if (BBL_INS_LAST(head) && CFG_DESCRIPTION(cfg)->InsIsControlflow (BBL_INS_LAST(head)) && edge == FallThroughPath (head) && diabloflowgraph_options.insprofilefile)
    {
      CFG_EDGE_SET_EXEC_COUNT(edge, BBL_EXEC_COUNT(head) - INS_EXEC_COUNT(BBL_INS_LAST(head)));
      VERBOSE(10, ("count know for edge @E\n of @ieB\n", edge, CFG_EDGE_HEAD(edge)));
    }
  }

  CFG_FOREACH_EDGE(cfg, edge)
    if (CfgEdgeIsBackwardInterproc (edge))
    {
      if (!CFG_EDGE_CORR(edge)) {
        CfgDrawFunctionGraphs(cfg, "forward");
        FATAL(("no forward edge for @E", edge));
      }
      CFG_EDGE_SET_EXEC_COUNT(edge, CFG_EDGE_EXEC_COUNT(CFG_EDGE_CORR(edge)));
    }

  do
  {
    flag = FALSE;

    CFG_FOREACH_EDGE(cfg, edge)
    {
      t_bbl *hd;
      t_bbl *tl;
      t_cfg_edge *jedg = NULL;

      if (CFG_EDGE_EXEC_COUNT(edge) >= 0)
        continue;

      /*
         see whether we can compute edge count by
         looking at other outgoing edges
         */

      hd = CFG_EDGE_HEAD(edge);
      count = BBL_EXEC_COUNT(hd);

      BBL_FOREACH_SUCC_EDGE(hd, jedg)
      {
        if (jedg == edge)
          continue;
        if (CFG_EDGE_EXEC_COUNT(jedg) < 0LL)
          break;
        else
          count -= CFG_EDGE_EXEC_COUNT(jedg);
      }

      if (!jedg)
      {
        if (count < 0LL)
          count = 0LL;
        CFG_EDGE_SET_EXEC_COUNT(edge, count);
        VERBOSE(10, ("1 count know for edge @E\n", edge));
        flag = TRUE;
        continue;
      }

      /*
         see whether we can compute edge count by
         looking at other outgoing edges
         */

      tl = CFG_EDGE_TAIL(edge);
      count = BBL_EXEC_COUNT(tl);

      BBL_FOREACH_PRED_EDGE(tl, jedg)
      {
        if (jedg == edge)
          continue;
        if (CFG_EDGE_EXEC_COUNT(jedg) < 0)
          break;
        else
          count -= CFG_EDGE_EXEC_COUNT(jedg);
      }

      if (!jedg)
      {
        if (count < 0)
          count = 0;
        CFG_EDGE_SET_EXEC_COUNT(edge, count);
        VERBOSE(10, ("2 count know for edge @E\n", edge));
        flag = TRUE;
        continue;
      }
    }
  }
  while (flag);

  CFG_FOREACH_EDGE(cfg, edge)
  {
    if (CFG_EDGE_EXEC_COUNT(edge) >= 0)
    {
      CFG_EDGE_SET_EXEC_COUNT_MAX(edge, CFG_EDGE_EXEC_COUNT(edge));
      CFG_EDGE_SET_EXEC_COUNT_MIN(edge, CFG_EDGE_EXEC_COUNT(edge));
    }
    else
    {
      t_int64 tl_cnt, hd_cnt;

      hd_cnt = BBL_EXEC_COUNT(CFG_EDGE_HEAD(edge));
      if (hd_cnt < 0LL)
        hd_cnt = 0LL;
      tl_cnt = BBL_EXEC_COUNT(CFG_EDGE_TAIL(edge));
      if (tl_cnt < 0LL)
        tl_cnt = 0LL;
      CFG_EDGE_SET_EXEC_COUNT_MAX(edge, (hd_cnt < tl_cnt) ? hd_cnt : tl_cnt);
      CFG_EDGE_SET_EXEC_COUNT_MIN(edge, 0LL);
    }
  }
#if 0
  CFG_FOREACH_BBL(cfg, bbl)
    if (BBL_EXEC_COUNT(bbl) > 0 && BblEndWithConditionalBranch (bbl))
    {
      t_cfg_edge *pred;
      t_bbl *bbl_pred1, *bbl_pred2;


      t_bbl *succ1 = CFG_EDGE_TAIL(TakenPath (bbl));
      t_bbl *succ2 = CFG_EDGE_TAIL(FallThroughPath (bbl));
      t_int64 count1 = 0, count2 = 0;
      t_int64 countbbl_pred1, countbbl_pred2, countsucc1, countsucc2, min11, min12, min21, min22;

      if (CFG_EDGE_EXEC_COUNT(TakenPath (bbl)) > 0)
        continue;
      if (succ1 == bbl || succ2 == bbl)
        continue;

      BBL_FOREACH_PRED_EDGE(succ1, pred)
      {
        if (CFG_EDGE_EXEC_COUNT(pred) >= 0)
          continue;
        if (CFG_EDGE_HEAD(pred) == bbl)
          continue;
        bbl_pred1 = CFG_EDGE_HEAD(pred);
        count1++;
      }

      BBL_FOREACH_PRED_EDGE(succ2, pred)
      {
        if (CFG_EDGE_EXEC_COUNT(pred) >= 0)
          continue;
        if (CFG_EDGE_HEAD(pred) == bbl)
          continue;
        bbl_pred2 = CFG_EDGE_HEAD(pred);
        count2++;
      }

      if (count1 != 1 || count2 != 1)
        continue;

      if (bbl_pred1 != bbl_pred2)
        continue;
      if (!BblEndWithConditionalBranch (bbl_pred2))
        continue;

      bbl_pred1 = bbl;

      countbbl_pred1 = BBL_EXEC_COUNT(bbl_pred1);
      countbbl_pred2 = BBL_EXEC_COUNT(bbl_pred2);
      countsucc1 = BBL_EXEC_COUNT(succ1);
      countsucc2 = BBL_EXEC_COUNT(succ2);

      BBL_FOREACH_PRED_EDGE(succ1, pred)
      {
        if (CFG_EDGE_EXEC_COUNT(pred) < 0)
          continue;
        countsucc1 -= CFG_EDGE_EXEC_COUNT(pred);
      }

      BBL_FOREACH_PRED_EDGE(succ2, pred)
      {
        if (CFG_EDGE_EXEC_COUNT(pred) < 0)
          continue;
        countsucc2 -= CFG_EDGE_EXEC_COUNT(pred);
      }

      min11 = countbbl_pred1 < countsucc1 ? countbbl_pred1 : countsucc1;
      min12 = countbbl_pred1 < countsucc2 ? countbbl_pred1 : countsucc2;
      min21 = countbbl_pred2 < countsucc1 ? countbbl_pred2 : countsucc1;
      min22 = countbbl_pred2 < countsucc1 ? countbbl_pred2 : countsucc2;

      CFG_EDGE_SET_EXEC_COUNT_MAX(TakenPath (bbl_pred1), (countbbl_pred1 - min12 + min11 + countsucc1 - min21 + min11) / 4);
      CFG_EDGE_SET_EXEC_COUNT_MIN(TakenPath (bbl_pred1), (countbbl_pred1 - min12 + min11 + countsucc1 - min21 + min11) / 4);
      CFG_EDGE_SET_EXEC_COUNT(TakenPath (bbl_pred1), (countbbl_pred1 - min12 + min11 + countsucc1 - min21 + min11) / 4);
      VERBOSE(10, ("3 count know for edge @E\n", TakenPath (bbl_pred1)));
    }
#endif
  CFG_FOREACH_FUN(cfg, ifun)
  {
    do
    {
      flag = FALSE;

      FUNCTION_FOREACH_BBL(ifun, bbl)
      {
        t_int64 min, max;
        t_int64 tmp_min, tmp_max;



        do
        {
          innerflag = FALSE;

          min = max = BBL_EXEC_COUNT(bbl);

          BBL_FOREACH_SUCC_EDGE(bbl, edge)
          {
            max -= CFG_EDGE_EXEC_COUNT_MAX(edge);
            min -= CFG_EDGE_EXEC_COUNT_MIN(edge);
          }

          BBL_FOREACH_SUCC_EDGE(bbl, edge)
          {
            if (CfgEdgeIsInterproc (edge))
              continue;
            if (CFG_EDGE_EXEC_COUNT(edge) >= 0)
              continue;

            tmp_min = max + CFG_EDGE_EXEC_COUNT_MAX(edge);
            tmp_max = min + CFG_EDGE_EXEC_COUNT_MIN(edge);

            if (tmp_min > CFG_EDGE_EXEC_COUNT_MIN(edge) && tmp_min < CFG_EDGE_EXEC_COUNT_MAX(edge))
            {
              CFG_EDGE_SET_EXEC_COUNT_MIN(edge, tmp_min);
              innerflag = flag = TRUE;
            }
            if (tmp_max < CFG_EDGE_EXEC_COUNT_MAX(edge) && tmp_max > CFG_EDGE_EXEC_COUNT_MIN(edge))
            {
              CFG_EDGE_SET_EXEC_COUNT_MAX(edge, tmp_max);
              innerflag = flag = TRUE;
            }
          }
        }
        while (innerflag);

        do
        {
          innerflag = FALSE;

          min = max = BBL_EXEC_COUNT(bbl);

          BBL_FOREACH_PRED_EDGE(bbl, edge)
          {
            max -= CFG_EDGE_EXEC_COUNT_MAX(edge);
            min -= CFG_EDGE_EXEC_COUNT_MIN(edge);
          }

          BBL_FOREACH_PRED_EDGE(bbl, edge)
          {
            if (CfgEdgeIsInterproc (edge))
              continue;
            if (CFG_EDGE_EXEC_COUNT(edge) >= 0)
              continue;
            tmp_min = max + CFG_EDGE_EXEC_COUNT_MAX(edge);
            tmp_max = min + CFG_EDGE_EXEC_COUNT_MIN(edge);
            if (tmp_min > CFG_EDGE_EXEC_COUNT_MIN(edge) && tmp_min < CFG_EDGE_EXEC_COUNT_MAX(edge))
            {
              CFG_EDGE_SET_EXEC_COUNT_MIN(edge, tmp_min);
              innerflag = flag = TRUE;
            }
            if (tmp_max < CFG_EDGE_EXEC_COUNT_MAX(edge) && tmp_max > CFG_EDGE_EXEC_COUNT_MIN(edge))
            {
              CFG_EDGE_SET_EXEC_COUNT_MAX(edge, tmp_max);
              innerflag = flag = TRUE;
            }
          }
        }
        while (innerflag);
      }
    }
    while (flag);

    FUNCTION_FOREACH_BBL(ifun, bbl)
    {
      BBL_FOREACH_SUCC_EDGE(bbl, edge)
      {
        if (CfgEdgeIsInterproc (edge))
          continue;
        if (CFG_EDGE_EXEC_COUNT(edge) >= 0LL)
          continue;

        /* integer divisiion rounds down to the lowest integer */
        if (CFG_EDGE_EXEC_COUNT_MIN(edge) == 0
            && CFG_EDGE_EXEC_COUNT_MAX(edge) == 1)
          CFG_EDGE_SET_EXEC_COUNT(edge, 1);
        else
          CFG_EDGE_SET_EXEC_COUNT(edge, (CFG_EDGE_EXEC_COUNT_MAX(edge) + CFG_EDGE_EXEC_COUNT_MIN(edge)) / 2);

        VERBOSE(10, ("4 count know for edge @E\n", edge));
      }
    }
  }
}

static t_uint32
AddressCmp (const void *key, const void *key2) /*compares fingerprints */
{
  t_uint32 n1 = *((t_uint32 *) key);
  t_uint32 m1 = *((t_uint32 *) key2);


  if (m1 == n1)
    return 0;

  return 1;
}

static t_uint32
BblHashAddress (const void *key, const t_hash_table * table)
{
  return (*((t_uint32 *) key)) % HASH_TABLE_TSIZE(table);
}

static t_uint32
InsHashAddress (const void *key, t_hash_table * table)
{
  return (*((t_uint32 *) key)) % HASH_TABLE_TSIZE(table);
}

static void *
BblAddressKey (t_bbl * bbl)
{
  t_uint32 *result = (t_uint32 *) Malloc (sizeof (t_uint32));

  *result = AddressExtractUint32 (BBL_CADDRESS(bbl));
  /* printf("assigned %x %x %x at %p %p %p\n",*result,*(result+1),*(result+2),result,result+1,result+2); */
  return result;
}

static void *
InsAddressKey (t_ins * ins)
{
  t_uint32 *result = (t_uint32 *) Malloc (sizeof (t_uint32));

  *result = AddressExtractUint32 (INS_OLD_ADDRESS(ins));
  /* printf("assigned %x %x %x at %p %p %p\n",*result,*(result+1),*(result+2),result,result+1,result+2); */
  return result;
}

static void
BblHeAddrKeyFree (const void *he1, void *data)
{
  t_bbl_he *he = (t_bbl_he *) he1;

  Free (HASH_TABLE_NODE_KEY(T_HASH_TABLE_NODE(he)));
  Free (he);
}

static void
InsHeAddrKeyFree (const void *he1, void *data)
{
  t_ins_he *he = (t_ins_he *) he1;

  Free (HASH_TABLE_NODE_KEY(T_HASH_TABLE_NODE(he)));
  Free (he);
}

/* Appears to be broken: Bruno nov 9 2006 */
//#define OPTIMIZED_SEARCH
//#define PROF_TRACE

void
CfgReadInsExecutionCounts (t_cfg * cfg, t_string name)
{
  FILE *fp = fopen (name, "r");


  t_address address;
  t_profile_file_count count;
  t_profile_file_address value;
  t_bbl *bbl;
  t_ins *ins;


  t_hash_table *addresses_to_ins;


  addresses_to_ins = HashTableNew (20033, 0,
                                   (t_hash_func) InsHashAddress,
                                   (t_hash_cmp) AddressCmp,
                                   InsHeAddrKeyFree);

  CFG_FOREACH_BBL(cfg, bbl)
  {
    BBL_FOREACH_INS(bbl, ins)
    {
      t_ins_he *inshe = (t_ins_he *) Malloc (sizeof (t_ins_he));

      inshe->ins = ins;
      HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(inshe), InsAddressKey (ins));
      HashTableInsert (addresses_to_ins, inshe);
      INS_SET_EXEC_COUNT(ins, BBL_EXEC_COUNT(bbl));
    }
  }

  printf ("inserted instructions\n");

  if (fp)
  {
    while (!feof (fp))
    {
      t_uint32 count_read;
#ifdef PROF_TRACE
      t_uint32 start;
      t_uint32 stop;
      
      if (fread(&start, 4, 1, fp)!=1) break;
      if (fread(&stop, 4, 1, fp)!=1) break;
          for (;start<=stop; start+=4)

      printf("%x -> %x\n", start, stop);

#else
      if (fscanf (fp, "%"PRIx64" %d\n", &value, &count_read))
      {
#ifdef OPTIMIZED_SEARCH
        t_uint32 *address_to_look;
        t_address_to_ins_he *node;
#endif
        count = (t_profile_file_count) count_read;

        address = AddressNewForCfg (cfg, value);

#ifdef OPTIMIZED_SEARCH

        address_to_look = Malloc (sizeof (t_uint32));

        *(address_to_look) = AddressExtractUint32 (address);
        node = HashTableLookup (addresses_to_ins, address_to_look);

        if (!node)
        {
          if (count > 0)
            FATAL(("ins not found for profile information\n"));
        }
        else
        {
          ins = node->ins;
          INS_SET_EXEC_COUNT(ins, count);
        }

        Free (address_to_look);
#else
        CFG_FOREACH_BBL(cfg, bbl)
        {
          t_bool exit_loop = FALSE;

          BBL_FOREACH_INS(bbl, ins)
          {
            if (AddressIsEq (address, INS_OLD_ADDRESS(ins)))
            {
              INS_SET_EXEC_COUNT(ins, count);
              exit_loop = TRUE;
              break;
            }
          }
          if (exit_loop)
            break;
        }
#endif
      }
#endif
    }
  }
  else
  {
    FATAL(("oeps, no execution count file with name %s", name));
  }

  HashTableFree (addresses_to_ins);
}


#ifdef BSEARCH
static int __helper_compare_bbls(const void *a, const void *b)
{
  t_bbl *A = *(t_bbl **)a;
  t_bbl *B = *(t_bbl **)b;
  if (AddressIsGt(BBL_OLD_ADDRESS(A),BBL_OLD_ADDRESS(B)))
    return 1;
  if (AddressIsLt(BBL_OLD_ADDRESS(A),BBL_OLD_ADDRESS(B)))
    return -1;
  return 0;
}
#endif

void
CfgReadBlockProfileFile (t_cfg * cfg, t_string name, t_bool execution_profile)
{
  FILE *fp = fopen (name, "r");

  t_address address;
  t_profile_file_count count;
  t_profile_file_address value;
  t_bbl *bbl;
#ifdef BSEARCH
  t_uint32 nbbls;
  t_bbl **bblarr;
#endif
#ifndef OPTIMIZED_SEARCH
  t_bbl *last_found = CFG_NODE_FIRST(cfg);
#endif

#ifdef BSEARCH
  nbbls=0;
  CFG_FOREACH_BBL(cfg,bbl)
    nbbls++;
  bblarr = Calloc(nbbls, sizeof(t_bbl *));
  nbbls=0;
#endif

#ifdef OPTIMIZED_SEARCH
  t_hash_table *addresses_to_blocks;

  addresses_to_blocks = HashTableNew (20033, 0,
                                      (t_uint32 (*)(void *, t_hash_table *)) BblHashAddress,
                                      (t_int32 (*)(void *, void *)) AddressCmp,
                                      BblHeAddrKeyFree);
#endif

  CFG_FOREACH_BBL(cfg, bbl)
  {
#ifdef OPTIMIZED_SEARCH
    t_bbl_he *bblhe = (t_bbl_he *) Malloc (sizeof (t_bbl_he));

    bblhe->bbl = bbl;
    HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(bblhe), BblAddressKey (bbl));
    HashTableInsert (addresses_to_blocks, bblhe);
#endif
#ifdef BSEARCH
    bblarr[nbbls] = bbl;
    nbbls++;
#endif
    if (execution_profile)
      BBL_SET_EXEC_COUNT(bbl, 0LL);
    else
      BBL_SET_SEQUENCE_ID(bbl, 0LL);
  }

#ifdef BSEARCH
  {
    t_uint32 i;
    for (i=0; i < nbbls; i++)
      if (bblarr[i] == NULL)
        FATAL(("null bbl 1"));
  }
  diablo_stable_sort(bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);

  {
    t_uint32 i;
    for (i=0; i < nbbls; i++)
      if (bblarr[i] == NULL)
        FATAL(("null bbl 2"));
  }
#endif


  t_bbl *tempbbl = BblNew(cfg);

  if (fp)
  {
    while (!feof (fp))
    {
#ifdef PROF_TRACE
      t_uint32 start;
      t_uint32 stop;
      
      if (fread(&start, 4, 1, fp)!=1) break;
      if (fread(&stop, 4, 1, fp)!=1) break;
          for (;start<=stop; start+=4)

          {
            t_bbl_he * node = HashTableLookup(addresses_to_blocks, &start);
            if (node)
            {
              VERBOSE(0, ("Found @B", node->bbl));
              if (execution_profile)
                BBL_SET_EXEC_COUNT(node->bbl, BBL_EXEC_COUNT(node->bbl)+1);
            }
            else
            {

            }
          }
#else
      if (diabloflowgraph_options.rawprofiles)
      {
        count=0;
        #ifdef _MSC_VER
        if (!(_read (fileno(fp), &value, sizeof(t_profile_file_address)) && _read (fileno(fp), &count,sizeof(t_profile_file_count))))
        #else
        if (!(read (fileno(fp), &value, sizeof(t_profile_file_address)) && read (fileno(fp), &count,sizeof(t_profile_file_count))))
        #endif
          break;
      }
      else
      {
        t_uint32 count_read;
        if (!(fscanf (fp, "%"PRIx64" %u\n", &value, &count_read)))
          continue;
        count = (t_profile_file_count) count_read;
      }
      {
#ifdef OPTIMIZED_SEARCH
        /* {{{ */
        t_uint32 *address_to_look;
        t_address_to_bbl_he *node;
        /* }}} */
#endif
        address = AddressNewForCfg (cfg, value);

#ifdef OPTIMIZED_SEARCH
        /* {{{ */
        address_to_look = Malloc (sizeof (t_uint32));

        *(address_to_look) = AddressExtractUint32 (address);
        node = HashTableLookup (addresses_to_blocks, address_to_look);

        Free (address_to_look);

        if (!node)
        {
          if (count>0)
            FATAL(("node not found for profile information 0x%x\n",value));
        }
        else
        {
          bbl = node->bbl;
          if (execution_profile)
            BBL_SET_EXEC_COUNT(bbl, count);
          else
            BBL_SET_SEQUENCE_ID(bbl, count);
        }
        /* }}} */
#else
#ifdef BSEARCH
        {
          t_bbl** fbbl;

          BBL_SET_OLD_ADDRESS(tempbbl, address);
          fbbl=(t_bbl**)bsearch(&tempbbl,bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);
          if (fbbl) {
            if (execution_profile)
              BBL_SET_EXEC_COUNT(*fbbl, count);
            else
              BBL_SET_SEQUENCE_ID(*fbbl, count);
          } else
            continue;
        }
#else
        /* start looking from the last found block: in the best case, when the
         * profile file is sorted, this gives linear performance, whereas always
         * looking from the start gives guaranteed O(n^2) performance */
        for (bbl = last_found; bbl; bbl = BBL_NEXT(bbl))
        {
          if (AddressIsEq (address, BBL_OLD_ADDRESS(bbl)))
            break;
        }
        if (!bbl)
        {
          CFG_FOREACH_BBL(cfg, bbl)
          {
            if (bbl == last_found)
            {
              bbl = NULL;
              break;
            }

            if (AddressIsEq (address, BBL_OLD_ADDRESS(bbl)))
              break;
          }
        }
        if (bbl)
        {
          last_found = bbl;

          if (execution_profile)
            BBL_SET_EXEC_COUNT(bbl, count);
          else
            BBL_SET_SEQUENCE_ID(bbl, count);
        }
        else
        {
          continue;
          /*
          if(count!=0 && !AddressIsEq(address,AddressNewForCfg(cfg,-1)))
            FATAL(("error"));
          */
        }
#endif
#endif
      }
#endif
    }

    t_uint64 nr_exec = 0;
    t_uint64 nr_total = 0;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      if (BBL_IS_HELL (bbl))
        continue;
      if (!BBL_FUNCTION(bbl))
        continue;
      if (BBL_IS_LAST(bbl))
        continue;
      
      if (execution_profile) {
        nr_total++;

        if (BBL_EXEC_COUNT(bbl) >= 0LL) {
          if (BBL_EXEC_COUNT(bbl) > 0)
            nr_exec++;
          continue;
        }
        BBL_SET_EXEC_COUNT(bbl, 0LL);
      }
      else {
        if (BBL_SEQUENCE_ID(bbl) >= 0LL)
          continue;
        BBL_SET_SEQUENCE_ID(bbl, 0LL);
      }
    }

    VERBOSE(0, ("PROFILE INFORMATION: %" PRIu64 "/%" PRIu64 "=%.2f", nr_exec, nr_total, 100.0*nr_exec/nr_total));

#ifndef SMC
    if (execution_profile)
      CfgEstimateEdgeCounts (cfg);
#endif

  }
  else
  {
    FATAL(("oeps, no execution count file with name %s", name));
  }

  if (execution_profile)
  {
    t_function  * fun;
    CFG_FOREACH_FUN(cfg,fun)
    {
      if(FUNCTION_BBL_FIRST(fun) && (BBL_EXEC_COUNT(FUNCTION_BBL_FIRST(fun)) != 0) && FunctionGetExitBlock(fun))
      {
        BBL_SET_EXEC_COUNT(FunctionGetExitBlock(fun), BBL_EXEC_COUNT(FUNCTION_BBL_FIRST(fun)));
      }
    }
  }

#ifdef OPTIMIZED_SEARCH
  HashTableFree (addresses_to_blocks);
#endif
#ifdef BSEARCH
  BblKill(tempbbl);
  Free(bblarr);
#endif
}

static int
cmp_count (t_bbl ** a, t_bbl ** b)
{
  t_bbl *bbla = *a;
  t_bbl *bblb = *b;

  if (BBL_EXEC_COUNT(bbla) < BBL_EXEC_COUNT(bblb))
    return 1;
  if (BBL_EXEC_COUNT(bbla) > BBL_EXEC_COUNT(bblb))
    return -1;
  return 0;

}

t_int64
CfgComputeWeight (t_cfg * cfg)
{
  t_bbl *bbl;
  t_int64 total_weight = 0;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_EXEC_COUNT(bbl) > 0)
    {
      total_weight += BBL_EXEC_COUNT(bbl) * BBL_NINS(bbl);
    }
  }
  VERBOSE(1,("total profile-based cfg weight %lld\n", total_weight));
  return total_weight;

}

#if 0
/* this is debugging code for the execution count facility in diablo.
 * enable it by changing the MEMBER definitions of EXEC_COUNT in bbl and edge
 * to MEMBERU */
int CfgEdgeUpdateEXEC_COUNT(t_cfg_edge *e, t_int64 val)
{
  if (val > 0xfffffffLL || val < -30LL)
  {
    VERBOSE(0, ("fucked up %lld for @E", val, e));
    abort();
  }
}
int BblUpdateEXEC_COUNT(t_bbl *b, t_int64 val)
{
  if (val > 0xfffffffLL || val < -30LL)
  {
    VERBOSE(0, ("fucked up %lld for @ieB", val, b));
    abort();
  }
}
#endif

void
CfgComputeHotBblThreshold (t_cfg * cfg, double weight_threshold)
{
  t_bbl *bbl;
  t_int64 total_count = 0LL;
  t_int64 total_weight = 0LL;
  t_int64 hot_weight = 0LL;
  t_int32 n_bbls = 0;
  t_bbl **array;
  int i;

  if (!diabloflowgraph_options.blockprofilefile)
    return;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_EXEC_COUNT(bbl) > 0LL)
    {
      total_count += BBL_EXEC_COUNT(bbl);
      total_weight += BBL_EXEC_COUNT(bbl) * BBL_NINS(bbl);
      n_bbls++;
    }
  }

  array = Malloc (sizeof (t_bbl *) * n_bbls);

  n_bbls = 0;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_EXEC_COUNT(bbl) > 0)
    {
      array[n_bbls++] = bbl;
    }
  }

  diablo_stable_sort (array, n_bbls, sizeof (t_bbl *), (int (*)(const void *, const void *)) cmp_count);

  /* if the threshold is too high, we run off the end of the array.
   * In that case, assume you want 100% of the executed code, so the threshold
   * count should be 1 */
  CFG_SET_HOT_THRESHOLD_COUNT(cfg, 1LL);
  for (i = 0; i < n_bbls; i++)
  {
    bbl = array[i];
    hot_weight += BBL_EXEC_COUNT(bbl) * BBL_NINS(bbl);
    if (((double) hot_weight) > ((double) total_weight) * weight_threshold)
    {
      CFG_SET_HOT_THRESHOLD_COUNT(cfg, BBL_EXEC_COUNT(bbl));
      break;
    }
  }
  VERBOSE(1,("Computed Hot BBL Threshold Value: %lld",CFG_HOT_THRESHOLD_COUNT(cfg)));
  /*
     for (i=0;i<n_bbls;i++)
     {
     bbl = array[i];
     if (BBL_EXEC_COUNT(bbl)>=CFG_HOT_THRESHOLD_COUNT(cfg))
     DiabloPrint(stdout,"HOT @B\n",array[i]);
     }

     printf("THRESHOLD: %d\n",CFG_HOT_THRESHOLD_COUNT(cfg));
     */
  Free (array);
}

t_bool
FunIsFrozen (t_function * fun)
{
  t_cfg *cfg = FUNCTION_CFG(fun);
  t_int64 threshold = CFG_HOT_THRESHOLD_COUNT(cfg);
  t_bbl *bbl;

  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    if(!BblIsFrozen(bbl))
      return FALSE;
  }
  return TRUE;
}

t_bool
FunIsHot (t_function * fun)
{
  t_cfg *cfg = FUNCTION_CFG(fun);
  t_int64 threshold = CFG_HOT_THRESHOLD_COUNT(cfg);
  t_bbl *bbl;

  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;
  FUNCTION_FOREACH_BBL(fun, bbl)
  {
    if (BBL_EXEC_COUNT(bbl) >= threshold)
      return TRUE;
  }
  return FALSE;
}

t_bool
BblIsFrozen (t_bbl * bbl)
{
  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;
  if (!BBL_FUNCTION(bbl))
    return FALSE;
  
  if (BBL_EXEC_COUNT(bbl) == 0)
    return TRUE;
  return FALSE;
}

t_bool
BblIsHot (t_bbl * bbl)
{
  t_cfg *cfg;
  t_int64 threshold;

  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;

  if (!BBL_FUNCTION(bbl))
    return FALSE;
  cfg = BBL_CFG(bbl);
  threshold = CFG_HOT_THRESHOLD_COUNT(cfg);
  if (BBL_EXEC_COUNT(bbl) >= threshold)
    return TRUE;
  return FALSE;
}

t_bool
BblIsAlmostHot (t_bbl * bbl)
{
  t_cfg *cfg;
  t_int64 threshold;

  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;
  if (!BBL_FUNCTION(bbl))
    return FALSE;
  cfg = FUNCTION_CFG(BBL_FUNCTION(bbl));
  threshold = CFG_HOT_THRESHOLD_COUNT(cfg);
  if (BBL_EXEC_COUNT(bbl) >= threshold / 2)
    return TRUE;
  return FALSE;
}

t_bool
EdgeIsHot (t_cfg_edge * edge)
{
  /* this check is needed when using global branch target redirection,
   * and more specifically in the case of a branch to a DATA BBL, which of course does not belong to any function. */
  if (!BBL_FUNCTION(CFG_EDGE_TAIL(edge)))
    return FALSE;

  t_cfg *cfg = FUNCTION_CFG(BBL_FUNCTION(CFG_EDGE_TAIL(edge)));
  t_int64 threshold = CFG_HOT_THRESHOLD_COUNT(cfg);

  if (!diabloflowgraph_options.blockprofilefile)
    return FALSE;
  if (!BblIsHot (CFG_EDGE_HEAD(edge)) && !BblIsHot (CFG_EDGE_TAIL(edge)))
    return FALSE;
  if ((double) CFG_EDGE_EXEC_COUNT(edge) >= ((double) threshold) * 0.90)
    return TRUE;
  return FALSE;
}

#if 0
double
ORDER(t_int32 arg)
{
  if (arg <= 0)
  {
    return 0.0;
  }
  return log10 (((double) arg));
}
#else
double
ORDER(t_int64 arg)
{
  double in = (double) arg;
  double out = 0;

  while (in > 0)
  {
    out += 1;
    in /= 10;
  }
  return out;
}
#endif

static t_uint32 unique_address_counter = 0x7ffffffc;
void CfgAssignUniqueOldAddresses(t_cfg * cfg)
{
  t_bbl * bbl;
  t_address zero = AddressNew32(0);
  
  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (IS_DATABBL(bbl))
        continue;
      if (BBL_FUNCTION(bbl))
        if (FUNCTION_NAME(BBL_FUNCTION(bbl)))
          {
            if (StringPatternMatch("LINKED_IN*",FUNCTION_NAME(BBL_FUNCTION(bbl))))
              continue;
            if (StringPatternMatch("--DYNCALL-HELL--*",FUNCTION_NAME(BBL_FUNCTION(bbl))))
              continue;
          }
      
      if (AddressIsEq(BBL_OLD_ADDRESS(bbl),zero))
        {
          BBL_SET_OLD_ADDRESS(bbl,unique_address_counter);
          unique_address_counter-=4;
        }
    }

  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (IS_DATABBL(bbl))
        continue;
      if (BBL_FUNCTION(bbl))
        if (FUNCTION_NAME(BBL_FUNCTION(bbl)))
          {
            if (!StringPatternMatch("--DYNCALL-HELL--*",FUNCTION_NAME(BBL_FUNCTION(bbl))))
              continue;
          }
      
      if (AddressIsEq(BBL_OLD_ADDRESS(bbl),zero))
        {
          BBL_SET_OLD_ADDRESS(bbl,unique_address_counter);
          unique_address_counter-=4;
        }
    }

}

void CfgResetOldAddresses(t_cfg * cfg)
{
  t_bbl * bbl;
  t_address last_unique_address = AddressNew32(unique_address_counter);
  t_address zero = AddressNew32(0);
  
  CFG_FOREACH_BBL(cfg,bbl)
    {
      if (AddressIsGe(BBL_OLD_ADDRESS(bbl),last_unique_address))
        BBL_SET_OLD_ADDRESS(bbl,zero);
    }
}


/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
