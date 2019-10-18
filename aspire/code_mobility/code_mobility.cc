/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include "code_mobility.h"
#include <obfuscation/obfuscation_transformation.h>

#include <map>

#define CODE_MOBILITY_OBJECT_NAME_PREFIX "mobile_dump_"
#define EF_CODE_MOBILITY_HELL_EDGE (1<<20) /* Flag to signify this edge was created as a result of code mobility tranformations */

using namespace std;

/* Create a stub function that contains the index and uses it to call upon the Resolve function. Return the entrypoint for the stub.
 * The entrypoint of the mobile function is passed as an argument so we can determine which registers are live and thus should be
 * pushed and popped.
 */
t_bbl* CodeMobilityTransformer::CreateGMRTStub (t_bbl* entry_bbl)
{
  t_arm_ins* ins;

  /* We will generate the following code:
   * POP {r0, r1} (calls/jumps coming from places containing no dead registers will land on this)
   * PUSH {r0, lr, pc} (save the register(s) we'll overwrite, together with lr and pc)
   * MOV r0, #index
   * BL Resolve
   * STR r0, [SP, PC_OFFSET] (change the saved PC so we'll return to the address of the loaded mobile code)
   * POP {r0, lr, pc}
   */

  t_uint32 gmrt_index = transform_index - 1;/* Index has already been incremented so we subtract 1 */

  /* Get all the integer registers we'll push */
  t_regset int_to_push = RegsetNew();
  RegsetSetAddReg(int_to_push, ARM_REG_R0);
  RegsetSetAddReg(int_to_push, ARM_REG_R14);
  RegsetSetAddReg(int_to_push, ARM_REG_R15);
  t_uint32 regs = RegsetToUint32(int_to_push);

  /* Make an entrypoint BBL */
  t_bbl* entrypoint = BblNew(cfg);
  ArmMakeInsForBbl(Pop, Append, ins, entrypoint, FALSE, (1 << ARM_REG_R0) | (1 << ARM_REG_R1), ARM_CONDITION_AL, FALSE);
  ArmMakeInsForBbl(Push, Append, ins, entrypoint, FALSE, regs, ARM_CONDITION_AL, FALSE);
  ArmMakeInsForBbl(Mov, Append, ins, entrypoint, FALSE, ARM_REG_R0, ARM_REG_NONE, 0, ARM_CONDITION_AL);
  ArmMakeConstantProducer(ins, gmrt_index);  /* Create an instruction to produce the index into the GMRT */
  ArmMakeInsForBbl(CondBranchAndLink, Append, ins, entrypoint, FALSE, ARM_CONDITION_AL);

  /* Make new function */
  char name[19];
  sprintf(name, "stub_gmrt_%08x", gmrt_index);
  t_function* fun = FunctionMake(entrypoint, name, FT_NORMAL);

  /* Make second (exit) BBL and insert it in the function */
  t_bbl* second = BblNew(cfg);
  BblInsertInFunction(second, fun);

  ArmMakeInsForBbl(Str, Append, ins, second, FALSE, ARM_REG_R0, ARM_REG_R13, ARM_REG_NONE,
    adr_size * (RegsetCountRegs(int_to_push) - 1) /* immediate */, ARM_CONDITION_AL, TRUE /* pre */, TRUE /* up */, FALSE /* wb */);
  ArmMakeInsForBbl(Pop, Append, ins, second, FALSE, regs, ARM_CONDITION_AL, FALSE);

  /* Create an edge to go from the entrypoint BBL to the Resolve function and return to the second BBL */
  CfgEdgeCreateCall (cfg, entrypoint, T_BBL(SYMBOL_BASE(resolve_sym)), second, FunctionGetExitBlock(BBL_FUNCTION(T_BBL(SYMBOL_BASE(resolve_sym)))));

  /* Create an jump edge from the second BBL to the exit block */
  CfgEdgeCreate (cfg, second, FunctionGetExitBlock(fun), ET_JUMP);

  /* Create hell edges to and from the stub */
  CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), entrypoint, CFG_EXIT_HELL_NODE(cfg), FunctionGetExitBlock(fun));

  return entrypoint;
}

CodeMobilityTransformer::CodeMobilityTransformer (t_object* obj, t_const_string output_name)
  : GMRTTransformer(obj, {code_mobility_options.downloader, code_mobility_options.binder}, code_mobility_options.lib_needed, output_name,
      ".diablo.code_mobility.log", TRUE, CODE_MOBILITY_OBJECT_NAME_PREFIX, "CODE_MOBILITY_RELOC_LABEL", EF_CODE_MOBILITY_HELL_EDGE)
{
  /* Get all the symbols we need from the binder object */
  resolve_sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "Resolve");
  ASSERT(resolve_sym, ("Didn't find the symbols present in the binder object! Are you sure it was linked in?"));

  /* Check whether we are dealing with the right binder version */
  t_symbol* version_sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), GMRT_IDENTIFIER_PREFIX "version");
  t_uint32 version = SectionGetData32 (T_SECTION(SYMBOL_BASE(version_sym)), SYMBOL_OFFSET_FROM_START(version_sym));
  ASSERT(version == binder_version, ("The binder version is %d, but Diablo expected version %d.", version, binder_version));

  LOG(L_TRANSFORMS, "START OF CODE MOBILITY LOG\n");
}

CodeMobilityTransformer::~CodeMobilityTransformer()
{
  LOG(L_TRANSFORMS,"END OF CODE MOBILITY LOG\n");
}

void CodeMobilityTransformer::TransformObject ()
{
  STATUS(START, ("Code Mobility"));

  cfg = OBJECT_CFG(obj);
  PrepareCfg(cfg);

  Region* region;
  CodeMobilityAnnotationInfo *info;
  CFG_FOREACH_CODEMOBILITY_REGION(cfg, region, info)
  {
    /* Transform all of these functions if possible */
    for (auto fun : RegionGetAllFunctions(region))
    {
      if (CanTransformFunction(fun))
      {
        t_const_string log_msg = "Making function %s mobile.\n";
        t_const_string fun_name = FUNCTION_NAME(fun) ? FUNCTION_NAME(fun) : "without name";
        VERBOSE(0, (log_msg, fun_name));
        LOG(L_TRANSFORMS, log_msg, fun_name);
        info->successfully_applied = TRUE;/* At least a part of the annotated region will be made mobile */

        /* If we need to make data mobile as well, select those subsections that we can make mobile */
        if (info->transform_data)
          SelectMobileDataForFunction(fun);

        TransformFunction(fun, TRUE);

        /* Add the selected subsections to the newly created mobile object. More specifically we will add the subsections to
         * a subobject (the linker object) of the mobile object, and add it to a new parent section of the mobile object that
         * we create.
         */
        t_object* new_obj = new_objs.back();
        t_object* linker_obj = ObjectGetLinkerSubObject(new_obj);
        t_section* new_parent = SectionCreateForObject(new_obj, RODATA_SECTION, NULL, AddressNullForObject(new_obj), ".rdata");
        for(auto sec : mobile_sections)
        {
          /* Adapt the section name so it includes the name of the subobject it belongs to */
          t_string new_name = StringConcat3(OBJECT_NAME(SECTION_OBJECT(sec)), ":", SECTION_NAME(sec));
          Free(SECTION_NAME(sec));
          SECTION_SET_NAME(sec, new_name);

          /* Move the section */
          SectionReparent(sec, new_parent);
          SectionRemoveFromObject(sec);
          SectionInsertInObject(sec, linker_obj);
        }
        mobile_sections.clear();/* Clear the mobile sections now we won't need it anymore */


        /* Add a GMRT entry pointing at the Resolve function */
        t_uint32 gmrt_offset = (transform_index - 1) * gmrt_entry_size;/* Index already incremented so we subtract 1 */
        t_bbl* stub = CreateGMRTStub(FUNCTION_BBL_FIRST(fun));

        /* Split the first stub BBL so we can add a relocation to the second instruction.
         * We can't create a relocation with the TO-relocatable being an instruction because
         * this is not allowed because of the reasons noted down in the documentation for 'RelocsMigrateToInstructions'. */
        t_bbl *to_bbl = BblSplitBlock(stub, BBL_INS_FIRST(stub), FALSE);

        RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj),
            AddressNullForObject(obj), /* addend */
            T_RELOCATABLE(gmrt_sec), /* from */
            AddressNewForObject(obj, gmrt_offset), /* from-offset */
            T_RELOCATABLE(to_bbl),
            AddressNullForObject(obj),
            FALSE, /* hell */
            NULL, /* edge */
            NULL, /* corresp */
            NULL, /* sec */
            "R00A00+\\l*w\\s0000$");

        /* If we're dealing with PIC code we'll need to add a dynamic relative relocation on this new entry */
        if ((OBJECT_TYPE(obj) == OBJTYP_SHARED_LIBRARY_PIC) || (OBJECT_TYPE(obj) == OBJTYP_EXECUTABLE_PIC))
          DiabloBrokerCall ("AddDynamicRelativeRelocation", obj, gmrt_sec, gmrt_offset);
      }
    }
  }

  /* Set the size of the GMRT */
  SectionSetData32 (T_SECTION(SYMBOL_BASE(gmrt_size_sym)), SYMBOL_OFFSET_FROM_START(gmrt_size_sym), transform_index);

  /* Now we know the size of the GMRT, resize it */
  Free(SECTION_DATA(gmrt_sec));/* Instead of a realloc we do a calloc here, as this table should be zeroed */
  SECTION_SET_DATA(gmrt_sec, Calloc (1, transform_index * gmrt_entry_size));
  SECTION_SET_CSIZE(gmrt_sec, transform_index * gmrt_entry_size);

  STATUS(STOP, ("Code Mobility"));
}

static t_bool cm_split_helper_IsStartOfPartition(t_bbl* bbl)
{
  /* The BBL is part of only one CodeMobility Region. This will get it */
  Region* region = NULL;
  Region* tmp = NULL;
  const CodeMobilityAnnotationInfo* info;
  BBL_FOREACH_CODEMOBILITY_REGION(bbl, tmp, info)
    region = tmp;

  t_cfg_edge* edge;
  BBL_FOREACH_PRED_EDGE(bbl, edge)
  {
    if(CfgEdgeIsForwardInterproc(edge))
      continue;

    /* Determine the head. This depends on whether or not the incoming edge is part of corresponding
     * pair of edges (e.g. call/return).
     */
    t_bbl* head = CFG_EDGE_CORR(edge) ? CFG_EDGE_HEAD(CFG_EDGE_CORR(edge)) : CFG_EDGE_HEAD(edge);

    /* Determine the second region */
    Region* region2 = NULL;
    BBL_FOREACH_CODEMOBILITY_REGION(head, tmp, info)
      region2 = tmp;

    /* If the two regions differ, we must partition */
    if(region != region2)
      return TRUE;
  }

  /* If there are no changes in region, don't partition */
  return FALSE;
}

static t_bool cm_split_helper_CanMerge(t_bbl* bbl1, t_bbl* bbl2)
{
  /* Find the first region */
  Region* region1 = NULL;
  Region* tmp = NULL;
  const CodeMobilityAnnotationInfo* info;
  BBL_FOREACH_CODEMOBILITY_REGION(bbl1, tmp, info)
    region1 = tmp;

  /* Find the second region */
  Region* region2 = NULL;
  BBL_FOREACH_CODEMOBILITY_REGION(bbl2, tmp, info)
    region2 = tmp;

  /* If both partitions are not in the same region, they can't be merged */
  return (region1 == region2)?TRUE:FALSE;
}

/* Do some preparatory (CM-specific) work on the CFG before doing any transformations */
void CodeMobilityTransformer::PrepareCfg (t_cfg* cfg)
{
  /* Check for every BBL whether there is an annotation indicating it shouldn't be transformed. If this is
   * the case, we will remove it from all code mobility regions it is part of.
   */
  t_bbl* bbl;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    /* Gather all code mobility regions and whether they need to be removed */
    vector<Region*> cm_regions;
    bool remove = false;

    Region* region;
    const CodeMobilityAnnotationInfo* info;
    BBL_FOREACH_CODEMOBILITY_REGION(bbl, region, info)
    {
      cm_regions.push_back(region);

      /* If the BBL is part of a non-transform region, we should remove it from all regions */
      if (!info->transform)
        remove = true;
    }

    if (remove)
    {
      for (auto region : cm_regions)
        BblRemoveFromRegion(region, bbl);
    }
  }

  /* Split all functions that have BBLs both in and out of a code mobility region. After this function has
   * been called all code mobility regions will exist out of functions that have no BBLs outside the region.
   */
  CfgPartitionFunctions(cfg, cm_split_helper_IsStartOfPartition, cm_split_helper_CanMerge);

  /* Recompute liveness */
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

  /* Prepare for marking */
  CfgUnmarkAllFun (cfg);
  t_function* fun;
  CFG_FOREACH_FUN(cfg, fun)
  {
    FunctionUnmarkAllBbls (fun);
    FunctionUnmark(fun);
  }

  BblMarkInit2 ();
  CfgEdgeMarkInit ();

  /* Mark all code that is potentially responsible for downloading new blocks, we can't make any of this mobile */
  MarkFrom(cfg, T_BBL(SYMBOL_BASE(init_sym)));
  MarkFrom(cfg, T_BBL(SYMBOL_BASE(resolve_sym)));
}

void CodeMobilityTransformer::FinalizeTransform ()
{
  /* Datastructures for metrics */
  struct Metric
  {
    uint32_t nr_of_mobile_blocks;
    uint32_t nr_of_mobile_bytes;
  };
  map<uint32_t, Metric> metrics;

  /* Create a file to contain metrics and print its header */
  t_const_string fname = StringConcat2(this->output_name, ".code_mobility_metrics");
  FILE* f_metric = fopen(fname, "w+");
  Free(fname);
  fprintf(f_metric, "#region_idx,nr_of_mobile_blocks,nr_of_mobile_bytes\n");

  /* To diversify blocks we use layout randomization. Even if blocks aren't diversified we want to make
   * sure the chains for the mobile blocks are laid oud randomly instead of optimized, this avoids troubles.
   */
  t_randomnumbergenerator *rng_cm_master = nullptr;
  t_randomnumbergenerator *rng_opaquepredicate;
  t_randomnumbergenerator *rng_apply_chance;
  if (code_mobility_options.code_mobility_diversity_seed)
  {
    diabloarm_options.orderseed = code_mobility_options.code_mobility_diversity_seed;
    rng_cm_master = RNGCreateBySeed(code_mobility_options.code_mobility_diversity_seed, "cm_diversity_master");
    rng_opaquepredicate = RNGCreateChild(rng_cm_master, "opaquepredicate");
    rng_apply_chance = RNGCreateChild(rng_cm_master, "apply_chance");
    RNGSetRange(rng_apply_chance, 0, 100);
    SetAllObfuscationsEnabled(true);
  }
  else
    diabloarm_options.orderseed = 10;

  DiabloBrokerCallInstall("AfterChainsOrdered", "t_cfg *, t_chain_holder *" , (void*)AfterChainsOrdered, FALSE);
  for(t_object* new_obj : new_objs)
  {
    /* Get the associated region */
    Region *region = NULL;
    const CodeMobilityAnnotationInfo *info;
    BBL_FOREACH_CODEMOBILITY_REGION(CFG_ENTRY(OBJECT_CFG(new_obj))->entry_bbl, region, info)
      break;
    Metric& metric = metrics[region->idx];

    /* If requested, diversify the mobile block using obfuscations */
    if (code_mobility_options.code_mobility_diversity_seed)
    {
      t_bbl* bbl;
      BblVector bbls;
      t_cfg* cfg = OBJECT_CFG(new_obj);
      CFG_FOREACH_BBL(cfg, bbl)
        bbls.push_back(bbl);

      for (auto bbl : bbls)
      {
        if (RNGGenerate(rng_apply_chance) < 75)
        {
          BBLObfuscationTransformation* obfuscator = GetRandomTypedTransformationForType<BBLObfuscationTransformation>("opaquepredicate", rng_opaquepredicate);
          ASSERT(obfuscator, ("Did not find any obfuscator for type '%s'", "opaquepredicate"));

          if (obfuscator->canTransform(bbl))
          {
            VERBOSE(1, ("Applying '%s' to @eiB", obfuscator->name(), bbl));
            obfuscator->doTransform(bbl, rng_opaquepredicate);
          }
          else
          {
            VERBOSE(1, ("WARNING: tried applying '%s' to @eiB, but FAILED", obfuscator->name(), bbl));
          }
        }
      }
    }

    ObjectDeflowgraph(new_obj);
    ObjectRebuildSectionsFromSubsections (new_obj);
    ObjectAssemble(new_obj);

    /* Determine block size */
    uint32_t block_size = SECTION_CSIZE(OBJECT_CODE(new_obj)[0]);
    for (t_uint32 iii = 0; iii < OBJECT_NRODATAS(new_obj); iii++)
      block_size += SECTION_CSIZE(OBJECT_RODATA(new_obj)[iii]);

    /* Update the metric info */
    metric.nr_of_mobile_blocks++;
    metric.nr_of_mobile_bytes += block_size;
  }

  /* Write out the metrics */
  for (auto pair : metrics)
  {
    fprintf(f_metric, "%u,%u,%u\n", pair.first, pair.second.nr_of_mobile_blocks, pair.second.nr_of_mobile_bytes);
  }
  fclose(f_metric);

  if(separate_reloc_table)
  {
    RelocTableFree(address_producer_table);
    separate_reloc_table = FALSE;
  }

  if (rng_cm_master)
  {
    RNGDestroy(rng_cm_master);
    RNGDestroy(rng_apply_chance);
    RNGDestroy(rng_opaquepredicate);
  }
}

void CodeMobilityTransformer::Output ()
{
  if (strcmp(code_mobility_options.output_dir, ".") != 0)
    DirMake(code_mobility_options.output_dir, FALSE);

  for(t_object* new_obj : new_objs)
  {
    t_const_string path = StringConcat3(code_mobility_options.output_dir, "/", OBJECT_NAME(new_obj));
    FILE* fp = fopen(path, "wb");
    Free(path);

    /* Write out the code section and possible RODATA sections */
    t_section* text_subsec = OBJECT_CODE(new_obj)[0];
    fwrite(SECTION_DATA(text_subsec), 1, SECTION_CSIZE(text_subsec), fp);
    if ((OBJECT_NRODATAS(new_obj) > 0) && (SECTION_SUBSEC_FIRST(OBJECT_RODATA(new_obj)[0]) != NULL))
    {
      /* We also create file containing its metadata (names and offsets of subsections) */
      path = StringConcat4(code_mobility_options.output_dir, "/", OBJECT_NAME(new_obj), ".metadata");
      FILE* fp_md = fopen(path, "wb");
      Free(path);

      for (t_uint32 iii = 0; iii < OBJECT_NRODATAS(new_obj); iii++)
      {
        t_section* sec = OBJECT_RODATA(new_obj)[iii];
        fwrite(SECTION_DATA(sec), 1, SECTION_CSIZE(sec), fp);

        t_section* subsec;
        SECTION_FOREACH_SUBSECTION(sec, subsec)
        {
          /* Decode the name of the subobject and the section, and print out the information */
          t_string subobj = SECTION_NAME(subsec);
          char* colon_pos = strchr(subobj, ':');
          colon_pos[0] = '\0';
          t_string name = colon_pos + 1;
          fprintf(fp_md, "Subobject: %s Subsection: %s Offset: 0x%x\n", subobj, name, AddressExtractUint32(AddressSub(SECTION_CADDRESS(subsec), SECTION_CADDRESS(text_subsec))));
        }
      }
      fclose(fp_md);
    }

    fclose(fp);
  }
}

void CodeMobilityTransformer::AddForceReachables (vector<string>& reachable_vector)
{
  reachable_vector.push_back(GMRT_IDENTIFIER_PREFIX"Init");
  reachable_vector.push_back(GMRT_IDENTIFIER_PREFIX"Resolve");
}

bool CodeMobilityTransformer::IsMobileObject (t_object* obj)
{
  /* We check whether the name matches, and as an extra check whether the object has a linker map */
  return (!strncmp(OBJECT_NAME(obj), CODE_MOBILITY_OBJECT_NAME_PREFIX, strlen(CODE_MOBILITY_OBJECT_NAME_PREFIX)) && (OBJECT_MAP(obj) == NULL));
}

void CodeMobilityTransformer::ReserveEntries(t_uint32 nr)
{
  transform_index += nr;
}
