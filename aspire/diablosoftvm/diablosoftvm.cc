/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

extern "C" {
/* For PRIu64 */
#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#include <diabloarm.h>
#define GENERATE_CLASS_CODE
#include <dlfcn.h>
#include <diabloanopt.h>
}

#include <diablosoftvm.h>
#include <diablosoftvm_vmchunk.h>

#include <jansson.h>
#include <fstream>

#include "diablosoftvm_vmchunk.h"
#include "diablosoftvm_json.h"

#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <map>

#include <diabloannotations.h>

#include <code_mobility_cmdline.h>

#define USE_WANDIVM 1
#if USE_WANDIVM
  #include "diablosoftvm_wandivm_interface.h"
#else
  #include "diablosoftvm_softvm_interface.h"
#endif

using namespace std;

#define ENABLE_PROFILE_BASED 1

#define NO_INS_SELECTOR 0
#define SUPPORT_CONST_PROD 1
#define SUPPORT_ADDR_PROD 1
#define SUPPORT_DATA 0
#define DISABLE_MULTI_EXIT 0

#define JANSSON_JSON_FLAGS (JSON_INDENT(2) | JSON_PRESERVE_ORDER)

#define DEBUG_SOFTVM 0
#define ONE_CHUNK 0
#define CHUNK_INDEX 0
#define SOFTVM_DEBUGCOUNTER 0
#define SOFTVM_DEBUGCOUNTER_VALUE diablosupport_options.debugcounter
//#define SOFTVM_DEBUGCOUNTER_VALUE 0
#define DEBUG_EXTRACTOR 0

extern int frontend_id;

static void *isl_handle = NULL;
static Bin2Vm* isl_session = NULL;
static Bin2Vm* phase2_session = NULL;

static t_ptr_array implemented_chunks;
static t_bool implemented_chunks_init = FALSE;

static int chunk_mobile_id = 0;

/* map to relate a dummy instruction to the corresponding chunk */
typedef map<t_arm_ins *, t_vmchunk *> t_ins_to_chunk_map;
typedef pair<t_arm_ins *, t_vmchunk *> t_ins_to_chunk_map_entry;
static t_ins_to_chunk_map dummies_to_chunks;

INS_DYNAMIC_MEMBER(mark, MARK, Mark, t_bool, FALSE);
BBL_DYNAMIC_MEMBER(in_chunk, IN_CHUNK, InChunk, t_bool, FALSE);
BBL_DYNAMIC_MEMBER(softvm_mobile, SOFTVM_MOBILE, SoftVMMobile, t_bool, FALSE);

t_bool
IsInsSupportedByVM(t_arm_ins * ins)
{
  bin2vm_status_code_t b2vStatus = BIN2VM_STATUS_SUCCESS;
  t_bool isSupported = FALSE;
  char assembled_ins[4];

#if !SUPPORT_DATA
  if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_DATA)
    return FALSE;
#endif

  if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_ADDRESS_PRODUCER)
    {
      if (!SUPPORT_ADDR_PROD) return FALSE;
      if (ArmInsIsConditional(ins)) return FALSE;
      if (!(ARM_INS_REFERS_TO(ins))) return FALSE;
      t_reloc * rel = RELOC_REF_RELOC(ARM_INS_REFERS_TO(ins));
      if (StringPatternMatch("*-*",RELOC_CODE(rel)))
	{
	  WARNING(("SKIPPING THE EXTRACTION OF @I BECAUSE OF RELATIVE ADDRESS FROM @R",ins,rel));
	  return FALSE;
	}
      return TRUE;
    }

  if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_CONSTANT_PRODUCER)
  {
	if (!SUPPORT_CONST_PROD) return FALSE;
    return TRUE;
  }

  if (ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_FLOAT_PRODUCER
      || ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_VFPFLOAT_PRODUCER
      || ARM_INS_OPCODE(T_ARM_INS(ins)) == ARM_PSEUDO_CALL)
    return FALSE;

#if NO_INS_SELECTOR
  if (ARM_INS_OPCODE(ins) == ARM_BL
      || ARM_INS_OPCODE(ins) == ARM_BLX
      || ARM_INS_TYPE(ins) == IT_DATA
      || (ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP))
    return FALSE;
  return TRUE;
#else
  ArmAssembleOne(ins, assembled_ins);
  b2vStatus = Bin2VmCheckArmInstruction( isl_session, reinterpret_cast<unsigned char *>(&assembled_ins), ARM_INS_CSIZE(ins), &isSupported );
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not check wether @I is supported or not: %d", ins, b2vStatus));

#if DISABLE_MULTI_EXIT
  if (ARM_INS_TYPE(ins) == IT_BRANCH
      && ArmInsIsConditional(ins))
    return FALSE;
#endif

  return isSupported;
#endif
}

/*
 * Remove the chunks from the binary and replace them with glue code jumps/calls.
 */
void
AspireSoftVMInsertGlueJumps(t_cfg *cfg, t_ptr_array *chunks)
{
  t_bool array_init = FALSE;

  /* loop over the blocks */
  for (int i = 0; i < PtrArrayCount(chunks)
#if SOFTVM_DEBUGCOUNTER
        && i < SOFTVM_DEBUGCOUNTER_VALUE
#endif
        ; i++)
  {
    t_symbol *glue_sym;
    t_arm_ins *tins;
    t_vmchunk *chunk = static_cast<t_vmchunk *>(PtrArrayGet(chunks, i));
    t_function *func = BBL_FUNCTION(CFG_EDGE_TAIL(VMCHUNK_ENTRY(chunk)));
    t_bbl *gluecode_bbl1, *gluecode_bbl2, *gluejump_bbl;
    t_bbl *i_bbl, *tmp_bbl;
    std::string symn;
    char *symn_cstr;
    t_cfg_edge *new_edge;

    if (!array_init)
    {
      PtrArrayInit(&implemented_chunks, FALSE);
      array_init = TRUE;
    }
    PtrArrayAdd(&implemented_chunks, chunk);

    START_LOGGING_TRANSFORMATION_AND_LOG_MORE(L_SOFTVM, "SoftVM,%x,%d", BBL_CADDRESS(CFG_EDGE_TAIL(VMCHUNK_ENTRY(chunk))), PtrArrayCount(VMCHUNK_BBLS(chunk))) {
      for (int i = 0; i < PtrArrayCount(VMCHUNK_BBLS(chunk)); i++)
      {
        t_bbl *bbl = static_cast<t_bbl *>(PtrArrayGet(VMCHUNK_BBLS(chunk), i));
        AddTransformedBblToLog("SoftVM", bbl);
      }

      LogFunctionTransformation("before", func);
    }

    /**********************************************************************************************
     * Look up the vmStartX symbol and its associated glue code BBLs */
    symn = "vmStart" + std::to_string(i);
    symn_cstr = new char [symn.length()+1];
    std::strcpy(symn_cstr, symn.c_str());
    VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Patching stub %s (entry @B)", symn_cstr, CFG_EDGE_TAIL(VMCHUNK_ENTRY(chunk))));

    glue_sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(CFG_OBJECT(cfg)), symn_cstr);
    ASSERT (glue_sym, ("Glue code symbol '%s' not found!", symn_cstr));
    delete[] symn_cstr;

    /* glue code first BBL: entry of stub */
    gluecode_bbl1 = T_BBL(SYMBOL_BASE(glue_sym));
    ASSERT (gluecode_bbl1, ("Glue code '%s' bbl not found!", symn_cstr));

    /* glue code second BBL: exit of stub */
    gluecode_bbl2 = FunctionGetExitBlock(BBL_FUNCTION(gluecode_bbl1));
    ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(gluecode_bbl2)) == NULL, ("expected only one edge going to the exit block, but got multiple @eiB", gluecode_bbl2));
    gluecode_bbl2 = CFG_EDGE_HEAD(BBL_PRED_FIRST(gluecode_bbl2));

    /**********************************************************************************************
     * Create a new BBL, the glue jump BBL, jumping to the glue code */
    gluejump_bbl = BblNew(cfg);
    BblInsertInFunction(gluejump_bbl, func);

    /* branch instruction */
    tins = ArmInsNewForBbl(gluejump_bbl);
    ArmInsMakeUncondBranch(tins);
    ArmInsAppendToBbl(tins, gluejump_bbl);

    /* At this moment in time, the incoming edges of the gluecode are still the same as they were in the beginning,
     * i.e. one incoming HELL edge to keep the stub alive during dead code removal. The edge corresponding to the
     * branch instruction from within the rewritten function to the stub code will be created later on. */
    t_cfg_edge * incoming_hell_edge = NULL;
    t_cfg_edge * tmp_edge = NULL;
    int count = 0;
    BBL_FOREACH_PRED_EDGE(gluecode_bbl1, tmp_edge)
    {
      count++;

      /* look up the CALL-from-HELL edge */
      if (BBL_IS_HELL(CFG_EDGE_HEAD(tmp_edge)))
      {
        if (CFG_EDGE_CAT(tmp_edge) == ET_CALL)
        {
          /* incoming edge created because the vmStartX is made force-reachable */
          ASSERT(incoming_hell_edge == NULL, ("stub should only have one incoming call edge from HELL @eiB", gluecode_bbl1));
          incoming_hell_edge = tmp_edge;
        }
        else if (CFG_EDGE_HEAD(tmp_edge) == CFG_UNIQUE_ENTRY_NODE(cfg))
        {
          /* incoming edge from HELL created because the vmStartX symbol is in the .dynsym section */
          CfgEdgeKill(tmp_edge);
        }
        else
          FATAL(("whoops! @E @iB", tmp_edge, CFG_EDGE_HEAD(tmp_edge)));
      }
    }
    ASSERT(count >= 1, ("stub should at least have one incoming edge @eiB", gluecode_bbl1));

    /* jump edge */ /* TODO: IP */
    new_edge = CfgEdgeCreate(cfg, gluejump_bbl, gluecode_bbl1, (BBL_FUNCTION(gluejump_bbl) == BBL_FUNCTION(gluecode_bbl1)) ? ET_IPJUMP : ET_IPJUMP);
    CfgEdgeCreateCompensating(cfg, new_edge);

    /* This has to be done here as the context-sensitive liveness analysis will FATAL later on.
     * The edges killed here are [(C)all, (R)eturn, (J)ump edges]:
     *    HELL -C-> gluecode_bbl1 -C-> vmExecute -R-> gluecode_bbl2 -J-> <return BBL> -R-> HELL
     *          ^......................................................................^ */
    /* First kill the call/return HELL edge pair */
    CfgEdgeKill(CFG_EDGE_CORR(incoming_hell_edge));
    CfgEdgeKill(incoming_hell_edge);

    BBL_FOREACH_PRED_EDGE(gluecode_bbl1, tmp_edge)
      ASSERT(!BBL_IS_HELL(CFG_EDGE_HEAD(tmp_edge)), ("Can't have any incoming edge from HELL no more! @eiB", gluecode_bbl1));

    /* Now kill the JUMP-edge in the previous figure, jumping to the return BBL */
    ASSERT(CFG_EDGE_SUCC_NEXT(BBL_SUCC_FIRST(gluecode_bbl2)) == NULL, ("stub @F should only have one outgoing edge @eiB", BBL_FUNCTION(gluecode_bbl2), gluecode_bbl2));
    CfgEdgeKill(BBL_SUCC_FIRST(gluecode_bbl2));

    /**********************************************************************************************
     * Modify the entry and exit edges of the chunk */
    /* Have chunk incoming edge point to the glue code jump bbl */
    CfgEdgeChangeTail(VMCHUNK_ENTRY(chunk), gluejump_bbl);

    t_reloc *rel = NULL;
    if ((rel = CFG_EDGE_REL(VMCHUNK_ENTRY(chunk))))
    {
      /* change the corresponding relocation */
#if DEBUG_SOFTVM
      DEBUG(("Changing relocation going to @B", RELOC_TO_RELOCATABLE(rel)[0]));
#endif
      RelocSetToRelocatable(rel, 0, T_RELOCATABLE(gluejump_bbl));
#if DEBUG_SOFTVM
      DEBUG(("                          to @B", RELOC_TO_RELOCATABLE(rel)[0]));
#endif
    }

    /* Have chunk exit edges come from HELL */
    for (int j = 0; j < PtrArrayCount(VMCHUNK_EXITS(chunk)); j++)
    {
      t_cfg_edge *exit_edge = static_cast<t_cfg_edge *>(PtrArrayGet(VMCHUNK_EXITS(chunk), j));
      t_bbl *old_tail = CFG_EDGE_TAIL(exit_edge);

      /* don't kill the existing exit edge because it may be needed in another code patch,
       * possibly taken by entering the chunk at a point other than the entry point. */
      ASSERT(CFG_EDGE_REL(exit_edge) == NULL, ("TODO fix this... add new edge etc...!"));

      t_edge_to_edge_map::iterator it;
      t_edge_to_edge_map *m = static_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));
      t_cfg_edge *bk = NULL;
      for (it = m->begin(); it != m->end(); it++)
      {
        if (it->second == exit_edge)
        {
          bk = it->first;
          m->erase(it);
          break;
        }
      }

      ASSERT(bk != NULL, ("something went wrong..."));

      /* recreate it, but make it come from HELL now */ /* TODO: IP */
      t_cfg_edge *new_exit_edge = CfgEdgeCreate(cfg, gluecode_bbl2, old_tail, (BBL_FUNCTION(gluecode_bbl2) == BBL_FUNCTION(old_tail)) ? ET_IPSWITCH : ET_IPSWITCH);
      if (!FunctionGetExitBlock (BBL_FUNCTION(CFG_EDGE_TAIL(new_exit_edge))))
      {
        /* No exit block was created yet for the tail function. Create it here. */
        t_function *fun = BBL_FUNCTION(CFG_EDGE_TAIL(new_exit_edge));
        FUNCTION_SET_BBL_LAST(fun, BblNew(cfg));
        BblInsertInFunction(FUNCTION_BBL_LAST(fun), fun);
        BBL_SET_ATTRIB(FUNCTION_BBL_LAST(fun), BBL_ATTRIB(FUNCTION_BBL_LAST(fun)) | BBL_IS_EXITBLOCK);
      }
      CfgEdgeCreateCompensating(cfg, new_exit_edge);
      CFG_EDGE_SET_SWITCHVALUE(new_exit_edge, -1);
      BBL_SET_ATTRIB(old_tail, BBL_ATTRIB(old_tail) | BBL_FORCE_REACHABLE);
      m->insert(t_edge_to_edge_map_entry(bk, new_exit_edge));
    }

    /**********************************************************************************************
     * keep the to relocatables referred to by the code in this chunk alive */
    t_ins *dummy_from_ins = BBL_INS_FIRST(gluecode_bbl1);
    ASSERT(!INS_REFERS_TO(dummy_from_ins), ("expected the dummy-from instruction to not have any relocation associated with it! @I", dummy_from_ins));

    t_ins_to_reloc_map *i2r_map = static_cast<t_ins_to_reloc_map *>(VMCHUNK_RELOC_MAP(chunk));
    t_bool moved_reloc_to_dummy = FALSE;

    for (auto it = i2r_map->begin(); it != i2r_map->end(); it++) {
      for (t_reloc *rel : it->second) {
        /* we need the TO relocations referred to by the code in the chunk to be kept alive
         * even after dead code removal. We do this by redirecting the relocation from inside
         * the chunk to a dummy instruction that will be kept for sure. */

        /* change the from of this relocation to the dummy instruction */
        RelocSetFrom(rel, T_RELOCATABLE(dummy_from_ins));
        RELOC_SET_FROM_OFFSET(rel, AddressNew32(0));

        /* add a recognisable label to this relocation */
        if (RELOC_LABEL(rel))
          Free(RELOC_LABEL(rel));
        RELOC_SET_LABEL(rel, StringIo("keep TO relocatable for translated instruction @I", it->first));

        /* split up the relocation code in calculate, write and return parts */
        t_string_array *splitted_reloc_code = StringDivide(RELOC_CODE(rel), "\\", TRUE, TRUE);
        ASSERT(StringArrayNStrings(splitted_reloc_code) == 3, ("expected reloc code \"%s\" to contain 3 parts, but got %d", RELOC_CODE(rel), StringArrayNStrings(splitted_reloc_code)));

        /* add a prefix to the code (so this relocation can be ignored later on), and remove the write part */
        Free(RELOC_CODE(rel));
        RELOC_SET_CODE(rel, StringIo("ideadc0de*%s\\*%s", splitted_reloc_code->first->string, splitted_reloc_code->first->next->next->string));

        StringArrayFree(splitted_reloc_code);

        moved_reloc_to_dummy = TRUE;
      }
    }

    /* only if a relocation has actually been moved: add the dummy instruction to the list of modified instructions */
    if (moved_reloc_to_dummy)
      dummies_to_chunks.insert(t_ins_to_chunk_map_entry(T_ARM_INS(dummy_from_ins), chunk));

    /**********************************************************************************************
     * Clean up the original CFG by removing the translated BBLs */
    /* unreachable BBLs will be cleaned up automatically using unreachable code removal */

    /* embed the chunk in the target function if needed */
    t_bool inline_stub = FALSE;

    if (inline_stub)
    {
      t_cfg_edge *e;

      /* edges incoming to the chunk */
      BBL_FOREACH_PRED_EDGE(gluecode_bbl1, e)
        if (CFG_EDGE_CAT(e) == ET_IPJUMP)
        {
          if (CFG_EDGE_CORR(e))
            CfgEdgeKill(CFG_EDGE_CORR(e));

          CFG_EDGE_SET_CAT(e, ET_JUMP);
        }
        else FATAL(("unsupported edge incoming to chunk! @E @eiB", e, gluecode_bbl1));

      /* edges outgoing from the chunk */
      BBL_FOREACH_SUCC_EDGE(gluecode_bbl2, e)
        if (CFG_EDGE_CAT(e) == ET_IPSWITCH)
        {
          if (CFG_EDGE_CORR(e))
            CfgEdgeKill(CFG_EDGE_CORR(e));

          CFG_EDGE_SET_CAT(e, ET_SWITCH);
        }
        else FATAL(("unsupported edge departing from chunk! @E @eiB", e, gluecode_bbl2));

      t_function *stub_function = BBL_FUNCTION(gluecode_bbl1);
      t_function *to_function = func;

      FUNCTION_FOREACH_BBL_SAFE(stub_function, i_bbl, tmp_bbl)
      {
        if (i_bbl != FunctionGetExitBlock(stub_function))
        {
          BBL_SET_NEXT_IN_FUN(i_bbl, BBL_NEXT_IN_FUN(gluejump_bbl));
          if (BBL_NEXT_IN_FUN(gluejump_bbl))
            BBL_SET_PREV_IN_FUN(BBL_NEXT_IN_FUN(gluejump_bbl), i_bbl);

          /* dependencies between gluejump_bbl and i_bbl */
          BBL_SET_NEXT_IN_FUN(gluejump_bbl, i_bbl);
          BBL_SET_PREV_IN_FUN(i_bbl, gluejump_bbl);

          /* change the function */
          BBL_SET_FUNCTION(i_bbl, to_function);
        }
      }

      /* as we have modified the list of BBLs in the function, BBL_LAST should be updated accordingly... */
      while (BBL_NEXT_IN_FUN(FUNCTION_BBL_LAST(BBL_FUNCTION(gluejump_bbl))))
        FUNCTION_SET_BBL_LAST(BBL_FUNCTION(gluejump_bbl), BBL_NEXT_IN_FUN(FUNCTION_BBL_LAST(BBL_FUNCTION(gluejump_bbl))));

      /* remove the stub function completely now */
      FUNCTION_SET_BBL_FIRST(stub_function, FUNCTION_BBL_LAST(stub_function));
      BBL_SET_PREV_IN_FUN(FUNCTION_BBL_LAST(stub_function), NULL);
      BBL_SET_NEXT_IN_FUN(FUNCTION_BBL_LAST(stub_function), NULL);
    }

    LOG_MORE(L_SOFTVM) { LogFunctionTransformation("after", func); }
    STOP_LOGGING_TRANSFORMATION(L_SOFTVM);

#if 0
    char * bname = NULL;
    asprintf(&bname, "%schunk%d-%s_%s", "./dots/", i, FUNCTION_NAME(func), "debug.dot");
    FunctionDrawGraphWithHotness(func, bname);
#endif

    VMCHUNK_SET_INTEGRATED(chunk, TRUE);
  }

  implemented_chunks_init = TRUE;
}

/*
 */
void
AspireSoftVMInit()
{
  bin2vm_status_code_t b2vStatus;

  ASSERT(diablosoftvm_options.instructionselector_path, ("Please provide the file path and name of the instruction selector library."));

  isl_handle = dlopen(diablosoftvm_options.instructionselector_path, RTLD_LAZY);
  ASSERT(isl_handle, ("could not load instruction selector library (%s)", dlerror()));

  LoadIslSymbols(isl_handle);

  /* open up the phase-2 session */
  b2vStatus = Bin2VmCreate(&phase2_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not create phase-2 session %d", b2vStatus));

  /* open up the instruction selector session */
  b2vStatus = Bin2VmCreateInstructionSelector(&isl_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not create instruction selector session %d", b2vStatus));

  /* select the VM type */
  SetTargetVmType(phase2_session, USE_WANDIVM);
  SetTargetVmType(isl_session, USE_WANDIVM);
}

void
AspireSoftVMPreFini()
{
  PtrArrayFini(&implemented_chunks, FALSE);
  implemented_chunks_init = FALSE;
}

void
AspireSoftVMFini(t_ptr_array *chunks)
{
  /* Close all DL handlers */
  enum bin2vm_status_codes b2vStatus;

  b2vStatus = Bin2VmDestroy(&isl_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not destroy instruction selector session %d", b2vStatus));

  b2vStatus = Bin2VmDestroy(&phase2_session);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("could not destroy phase-2 session %d", b2vStatus));

  dlclose(isl_handle);

  /* Free the array of chunks */
  for (int i=0; i < PtrArrayCount(chunks); ++i)
  {
    t_vmchunk *chunk = static_cast<t_vmchunk *>(PtrArrayGet(chunks, i));
    VmChunkFree(chunk);
  }
}

static void
MarkPossibleSuccEdges(t_bbl *start)
{
  t_cfg_edge *edge;

  BBL_FOREACH_SUCC_EDGE(start, edge)
  {
    t_bbl *tail = CFG_EDGE_TAIL(edge);

    if (CFG_EDGE_CAT(edge) == ET_CALL)
      continue;

    if (CfgEdgeIsInterproc(edge))
      continue;

    /* the tail of this edge can only be added to the chunk if:
     *  - it is translatable;
     *  - it has not been assigned to another chunk yet;
     *  - the corresponding edge is not interprocedural (IP);
     *  - the corresponding edge is not yet put forward as a candidate. */
    if (BblIsMarked(tail)
        && !BblIsMarked2(tail)
        && !CfgEdgeIsMarked(edge))
    {
      /* mark this edge and process the successors of its tail. */
      CfgEdgeMark(edge);
      MarkPossibleSuccEdges(tail);
    }
  }
}

static t_bool
IsMeaningfulChunkInBbl(t_ins *start_ins, t_ins *stop_ins)
{
  if (start_ins == stop_ins
      && ARM_INS_TYPE(T_ARM_INS(start_ins)) == IT_BRANCH)
    return FALSE;

  return TRUE;
}

static t_bool
IsMeaningfulChunk(t_ptr_array *bbls_in_chunk, t_ins *start_ins)
{
  t_bbl *bbl = static_cast<t_bbl *>(PtrArrayGet(bbls_in_chunk, 0));
  ASSERT(bbl == INS_BBL(start_ins), ("expected first BBL in chunk BBL list to contain the start instruction @I, but got @eiB", start_ins, bbl));

  /* if the chunk does not contain any BBLs */
  if (PtrArrayCount(bbls_in_chunk) == 0)
    return FALSE;

  /* if the chunk contains only one BBL,
   * and if the start instructions is a branch instruction,
   * the chunk is meaningless */
  if (PtrArrayCount(bbls_in_chunk) == 1
      && ARM_INS_TYPE(T_ARM_INS(start_ins)) == IT_BRANCH)
    return FALSE;

  return TRUE;
}

static t_bool
BblContainsMarkedIns(t_bbl *bbl)
{
  t_ins *ins;

  BBL_FOREACH_INS(bbl, ins)
    if (INS_MARK(ins))
      return TRUE;

  return FALSE;
}

static t_bool
BblCanBeEntryOfChunk(t_bbl *bbl)
{
  t_cfg_edge *edge;
  t_bool ret = TRUE;

  if (INS_MARK(BBL_INS_FIRST(bbl)))
  {
    t_bool all_predecessors_translatable = TRUE;

    BBL_FOREACH_PRED_EDGE(bbl, edge)
    {
      t_bbl *pred_bbl = CFG_EDGE_HEAD(edge);

      if (!BblIsMarked(pred_bbl))
      {
        all_predecessors_translatable = FALSE;
        break;
      }

      ASSERT(BBL_INS_LAST(pred_bbl), ("Did not expect empty BBL here @eiB", pred_bbl));

      if (!INS_MARK(BBL_INS_LAST(pred_bbl)))
      {
        all_predecessors_translatable = FALSE;
        break;
      }
    }

    if (all_predecessors_translatable)
      ret = FALSE;
  }

  return ret;
}

static void
BblEnsureSingleIncomingEdge(t_bbl *bbl)
{
  if (CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) != NULL)
  {
    /* create a new, empty BBL */
    t_cfg_edge *edge, *edge_tmp;
    t_cfg *cfg = BBL_CFG(bbl);
    t_bbl *new_branch_bbl = BblNew(cfg);
    t_arm_ins *new_ins = ArmInsNewForBbl(new_branch_bbl);

    BblCopyRegions(bbl, new_branch_bbl);
    BblInsertInFunction(new_branch_bbl, BBL_FUNCTION(bbl));

    /* first redirect all incoming edges to the new, empty BBL */
    BBL_FOREACH_PRED_EDGE_SAFE(bbl, edge, edge_tmp)
    {
      if (CFG_EDGE_REL(edge))
      {
        t_reloc *rel = CFG_EDGE_REL(edge);
        ASSERT(RELOC_N_TO_RELOCATABLES(rel) == 1, ("did not expect relocation with more than one TO_RELOCATABLE @R", rel));
        RelocSetToRelocatable(rel, 0, T_RELOCATABLE(new_branch_bbl));
      }
      CfgEdgeChangeTail(edge, new_branch_bbl);
    }

    /* then create a new edge jumping to the chunk entry point */
    ArmInsMakeUncondBranch(new_ins);
    ArmInsAppendToBbl(new_ins, new_branch_bbl);
    /* TODO: IP */
    CfgEdgeCreate(cfg, new_branch_bbl, bbl, (BBL_FUNCTION(new_branch_bbl) == BBL_FUNCTION(bbl)) ? ET_JUMP : ET_JUMP);

    /* sanity check */
    ASSERT(CFG_EDGE_PRED_NEXT(BBL_PRED_FIRST(bbl)) == NULL, ("Something went wrong: @eiB should have only one incoming edge now, coming from @eiB", bbl, new_branch_bbl));
  }
}

static t_bool
ChunkEdgeNeedsFixing(t_cfg_edge *edge, t_ptr_array *chunk_bbls)
{
  if (PtrArrayFind(chunk_bbls, CFG_EDGE_TAIL(edge)) == -1)
  {
    /* this is an external fallthrough edge */
    if (CFG_EDGE_CAT(edge) == ET_FALLTHROUGH
        || CFG_EDGE_CAT(edge) == ET_IPFALLTHRU)
      return TRUE;
  }

  return FALSE;
}

t_bool BblIsFullyTranslatable(t_bbl *bbl)
{
  t_ins *ins;
  BBL_FOREACH_INS(bbl, ins)
    if (!INS_MARK(ins))
      return FALSE;

  return TRUE;
}

static void
CreateChunks(t_cfg *cfg, t_ptr_array *chunks, t_ptr_array *translatable_bbls)
{
  t_bool iterate;

  int chunk_count = 0;
  int trampoline_count = 0;
  int chunk_trampoline_count = 0;

  /* mark BBLs to be added to the chunk */
  BblMarkInit2();

  do
  {
    iterate = FALSE;

#if SOFTVM_DEBUGCOUNTER
#if DEBUG_EXTRACTOR
    if (chunk_count >= SOFTVM_DEBUGCOUNTER_VALUE)
      break;
#endif
#endif

    for (int i = 0; i < PtrArrayCount(translatable_bbls); i++)
    {
      t_bbl *bbl = static_cast<t_bbl *>(PtrArrayGet(translatable_bbls, i));
      t_ins *start_ins, *stop_ins;
      t_cfg_edge *edge;
      t_ptr_array bbls_in_chunk;

#if SOFTVM_DEBUGCOUNTER
#if DEBUG_EXTRACTOR
      if (chunk_count >= SOFTVM_DEBUGCOUNTER_VALUE)
        break;
#endif
#endif

      /* skip non-translatable BBLs
       * or BBLs that have already been put in a chunk */
      if (!BblIsMarked(bbl))
        continue;

      /* Only in case the first instruction in the BBL is translatable:
       * are all predecessors translatable? If so, this BBL will be part of a bigger chunk */
      if (!BblCanBeEntryOfChunk(bbl))
        continue;

      /* mark this BBL as processed */
      BblUnmark(bbl);

      /* find first translatable instruction in BBL */
      start_ins = BBL_INS_FIRST(bbl);
      while (start_ins
             && !INS_MARK(start_ins))
        start_ins = INS_INEXT(start_ins);
      ASSERT(start_ins, ("expected start instruction in @eiB", bbl));

      /* find last translatable instruction in BBL */
      stop_ins = start_ins;
      while (INS_INEXT(stop_ins)
             && INS_MARK(INS_INEXT(stop_ins)))
        stop_ins = INS_INEXT(stop_ins);

      PtrArrayInit(&bbls_in_chunk, FALSE);

      /* if stop_ins has a next instruction, this part of the BBL will form the chunk.
       * In this special case, check whether the part of the BBL is meaningful */
      if (INS_INEXT(stop_ins))
      {
        /* stop_ins is not the last instruction in the BBL */

        /* do we want to create a chunk here? */
        t_bool create_chunk = IsMeaningfulChunkInBbl(start_ins, stop_ins);
        if (!create_chunk)
        {
          /* unmark selected instruction range for translation,
           * as translating them is meaningless */
          VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Meaningless part of BBL (@I -> @I)! @eiB", start_ins, stop_ins, INS_BBL(start_ins)));

          t_ins *tmp = start_ins;
          INS_SET_MARK(tmp, FALSE);
          do
          {
            tmp = INS_INEXT(tmp);
            INS_SET_MARK(tmp, FALSE);
          } while (tmp != stop_ins);

          /* in case this BBL still contains (a) marked instruction(s) */
          if (BblContainsMarkedIns(bbl))
          {
            BblMark(bbl);
            iterate = TRUE;
          }
        }
        else
        {
          /* this is a meaningful chunk -> add the BBL to the list of BBLs to be put in the chunk */
          PtrArrayAdd(&bbls_in_chunk, INS_BBL(start_ins));
          BblMark2(INS_BBL(start_ins));
        }
      }
      else
      {
        /* stop_ins is the last instruction in the BBL.
         * Try to create a list of BBLs as big as possible by iterating over the successors. */
        vector<t_bbl *> bbl_todo;
        bbl_todo.push_back(INS_BBL(start_ins));

        CfgEdgeMarkInit();
        MarkPossibleSuccEdges(bbl);

        while (bbl_todo.size() > 0)
        {
          t_bbl *bbl_it = bbl_todo.back();
          bbl_todo.pop_back();

          ASSERT(BblIsMarked(bbl_it) || (bbl_it == INS_BBL(start_ins)), ("can't add non-translatable BBL to chunk! @eiB @eiB", bbl_it, INS_BBL(start_ins)));

          PtrArrayAdd(&bbls_in_chunk, bbl_it);
          BblMark2(bbl_it);

          BBL_FOREACH_SUCC_EDGE(bbl_it, edge)
          {
            t_bbl *succ_bbl = CFG_EDGE_TAIL(edge);

            /* skip this edge if it is not marked */
            if (!CfgEdgeIsMarked(edge))
              continue;

            /* skip this edge if the tail is already part of a chunk or
             * if the first instruction of the tail is not translatable */
            if (BblIsMarked2(succ_bbl)
                || !INS_MARK(BBL_INS_FIRST((succ_bbl))))
              continue;

            bbl_todo.push_back(succ_bbl);

            /* need to mark the added BBL here too because otherwise it could
             * be added twice to the queue (i.e., if the BBL has a successor edge
             * referring to itself). */
            BblMark2(succ_bbl);
          }
        }
      }

      /* is this a meaningful chunk? */
      if (!IsMeaningfulChunk(&bbls_in_chunk, start_ins))
      {
        /* if not, unmark the BBLs that would have been put in the chunk */
        VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Meaningless chunk!"));

        /* unmark the BBLs in this chunk */
        for (int i = 0; i < PtrArrayCount(&bbls_in_chunk); i++)
        {
          t_bbl *bbl_it = static_cast<t_bbl *>(PtrArrayGet(&bbls_in_chunk, i));

          BblUnmark2(bbl_it);
        }
      }
      else
      {
        t_bool already_counted_chunk = FALSE;

        /* creating a new chunk */
        VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Creating chunk %d containing %d BBLs", PtrArrayCount(chunks), PtrArrayCount(&bbls_in_chunk)));
        t_vmchunk *nchunk = VmChunkNew();
        chunk_count++;

        /* add this newly created chunk to the return array */
        PtrArrayAdd(chunks, nchunk);

/* enable this block to ensure that a selected BBL is always translated;
 * e.g., not preserved because other code paths need this BBL too.
 * Instead of preserving the BBL, split up the big chunk into smaller parts
 * so that every selected BBL eventually gets put in a chunk. */
#if 1
        /* there can only be one entry BBL */
        t_bbl *entry_bbl = INS_BBL(start_ins);

        for (int i = 0; i < PtrArrayCount(&bbls_in_chunk); i++)
        {
          t_bbl *bbl_it = static_cast<t_bbl *>(PtrArrayGet(&bbls_in_chunk, i));

          if (bbl_it == entry_bbl)
            continue;

          /* this BBL has already been unmarked */
          if (!BblIsMarked2(bbl_it))
            continue;

          if (BblIsFullyTranslatable(bbl_it))
            continue;

          t_cfg_edge *e;
          BBL_FOREACH_SUCC_EDGE(bbl_it, e)
          {
            /* if the BBL has a successor edge to somewhere in the chunk, which is not the entry point,
             *  */
            if (BblIsMarked2(CFG_EDGE_TAIL(e)) && CFG_EDGE_TAIL(e) != entry_bbl)
            {
              VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "   removing2 @eB via edge @E", CFG_EDGE_TAIL(e), e));
              BblUnmark2(CFG_EDGE_TAIL(e));
            }
          }
        }

        /* remove BBLs that cause the chunk to have multiple entry points */
        t_bool remove_more = FALSE;
        do
        {
          remove_more = FALSE;

          for (int i = 0; i < PtrArrayCount(&bbls_in_chunk); i++)
          {
            t_bbl *bbl_it = static_cast<t_bbl *>(PtrArrayGet(&bbls_in_chunk, i));

            /* skip the entry BBL */
            if (bbl_it == entry_bbl)
              continue;

            /* skip unmarked BBLs; there are BBLs that were to be put in the chunk,
             * but will not be anymore because it caused multi-entry chunks */
            if (!BblIsMarked2(bbl_it))
              continue;

            t_cfg_edge *e;
            BBL_FOREACH_PRED_EDGE(bbl_it, e)
              /* don't put the current BBL in the chunk
               * if it has a predecessor that can't be put in the chunk */
              if (!BblIsMarked2(CFG_EDGE_HEAD(e)))
              {
                VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "   removing @eB", bbl_it));
                BblUnmark2(bbl_it);
                remove_more = TRUE;
                break;
              }
          }
        } while (remove_more);
#endif

        /* split off all BBLs queued for translation */
        t_function *f = NULL;
        for (int i = 0; i < PtrArrayCount(&bbls_in_chunk); i++)
        {
          t_bbl *bbl_it = static_cast<t_bbl *>(PtrArrayGet(&bbls_in_chunk, i));
          t_ins *ins_it;

          /* skip BBLs that don't need to be put in the chunk anymore */
          if (!BblIsMarked2(bbl_it))
            continue;

          /* this is mainly for debugging purposes: indicate whether a chunk is interprocedural or not */
          if (f)
          {
            /* set the IP property of the chunk if necessary */
            if (BBL_FUNCTION(bbl_it) != f)
              VMCHUNK_SET_IP(nchunk, TRUE);
          }
          else
            /* initialise 'f' */
            f = BBL_FUNCTION(bbl_it);

          /* only the entry BBL can have non-translatable instructions up front */
          if (bbl_it == INS_BBL(start_ins)
              && INS_IPREV(start_ins)
              && !INS_MARK(INS_IPREV(start_ins)))
          {
            /* split off the first part of the BBL */
            BblUnmark2(bbl_it);
            bbl_it = BblSplitBlock(INS_BBL(start_ins), start_ins, TRUE);
            BblMark2(bbl_it);
            bbls_in_chunk.arr[i] = bbl_it;
          }

          /* by now every BBL should be translatable from the beginning */
          ASSERT(INS_MARK(BBL_INS_FIRST(bbl_it)), ("expected first instruction to be translatable @eiB", bbl_it));

          /* look for the last translatable instruction in the BBL */
          ins_it = BBL_INS_FIRST(bbl_it);
          while (INS_INEXT(ins_it)
                 && INS_MARK(INS_INEXT(ins_it)))
            ins_it = INS_INEXT(ins_it);

          /* split off the last part and check if it contains translatable instructions */
          if (INS_INEXT(ins_it))
          {
            /* split off the last part of the BBL */
            t_bbl *last_part = BblSplitBlock(INS_BBL(ins_it), ins_it, FALSE);
            BblUnmark2(last_part);

            if (BblContainsMarkedIns(last_part))
            {
              PtrArrayAdd(translatable_bbls, last_part);
              BblMark(last_part);

              iterate = TRUE;
            }
          }

          if (bbl_it == INS_BBL(start_ins))
          {
            /* In case the chunk entry point has multple incoming edges,
             * create one common jump block in which all these edges converge.
             * This ensures that the chunk only has one incoming edge. */
            BblEnsureSingleIncomingEdge(bbl_it);

            /* set the one and only incoming edge to the entry BBL as the entry edge for the chunk */
            VMCHUNK_SET_ENTRY(nchunk, BBL_PRED_FIRST(INS_BBL(start_ins)));
          }

          /* This BBL is part of the chunk */
          VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "  + @B", bbl_it));
          VmChunkAddBbl(nchunk, bbl_it);

          if (BBL_SOFTVM_MOBILE(bbl_it)
              && VMCHUNK_MOBILE_ID(nchunk) == -1)
            VMCHUNK_SET_MOBILE_ID(nchunk, chunk_mobile_id++);

          ASSERT(!BBL_IN_CHUNK(bbl_it), ("@eiB already in chunk!", bbl_it));
          BBL_SET_IN_CHUNK(bbl_it, TRUE);

          /* fix this BBL: in case it has any fallthrough edge, make it fall through to a branch instead */
          BBL_FOREACH_SUCC_EDGE(bbl_it, edge)
          {
            /* skip BBLs that are part of the chunk */
            if (BblIsMarked2(CFG_EDGE_TAIL(edge))
                && PtrArrayFind(&bbls_in_chunk, CFG_EDGE_TAIL(edge)) != -1) {
              t_bbl *bbl = CFG_EDGE_TAIL(edge);
              continue;
            }

            if (ChunkEdgeNeedsFixing(edge, &bbls_in_chunk) || !BblIsMarked2(CFG_EDGE_TAIL(edge)))
            {
              /* this fallthrough edge should be fixed. Change the fallthrough path
               * so it goes to an unconditional branch INSIDE the chunk that in turn
               * jumps to the final destination */
              t_bbl * new_branch_bbl = BblNew(cfg);
              BblInsertInFunction(new_branch_bbl, BBL_FUNCTION(bbl_it));
              BblCopyRegions(bbl_it, new_branch_bbl);

              t_arm_ins *new_ins;

              /* create a new BBL containing one branch instruction */
              /* TODO: IP */
              t_cfg_edge *new_edge = CfgEdgeCreate(cfg, new_branch_bbl, CFG_EDGE_TAIL(edge), (BBL_FUNCTION(new_branch_bbl) == BBL_FUNCTION(CFG_EDGE_TAIL(edge))) ? ET_JUMP : ET_IPJUMP);
              new_ins = ArmInsNewForBbl(new_branch_bbl);
              ArmInsMakeUncondBranch(new_ins);
              ArmInsAppendToBbl(new_ins, new_branch_bbl);

              /* change the tail of the existing edge, going to the newly created branch BBL */
              CfgEdgeChangeTail(edge, new_branch_bbl);

              /* this is not interprocedural anymore! */
              if (CfgEdgeIsInterproc(edge))
                if (CFG_EDGE_CAT(edge) == ET_IPJUMP)
                {
                  ASSERT(CFG_EDGE_CAT(new_edge) == ET_IPJUMP, ("old edge was of type ET_IPJUMP, but new edge isn't!\nOld: @E\nNew: @E", edge, new_edge));
                  CFG_EDGE_SET_CAT(edge, ET_JUMP);

                  /* adjust corresponding edge data if necessary */
                  if (CFG_EDGE_CORR(edge))
                  {
                    CFG_EDGE_SET_CORR(new_edge, CFG_EDGE_CORR(edge));
                    CFG_EDGE_SET_CORR(CFG_EDGE_CORR(edge), new_edge);

                    CFG_EDGE_SET_CORR(edge, NULL);
                  }
                }
              ASSERT(!CfgEdgeIsInterproc(edge), ("fixme: make this a local (non-interproc) edge! @E", edge));

              /* finally, add the newly created "trampoline" to the chunk */
              VmChunkAddBbl(nchunk, new_branch_bbl);
              BblMark(new_branch_bbl);

              /* do some book keeping */
              trampoline_count++;

              if (!already_counted_chunk) chunk_trampoline_count++;
              already_counted_chunk = TRUE;
            }
          }

          BblUnmark(bbl_it);
        }

        /* sanity check */
        ASSERT(PtrArrayCount(VMCHUNK_BBLS(nchunk)) > 0, ("no BBLs left in chunk!"));

#if DEBUG_SOFTVM
        VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "   chunk contains %d instructions", VmChunkCountInstructions(nchunk)));
#endif
      }

      PtrArrayFini(&bbls_in_chunk, FALSE);

      if (iterate)
        break;
    }
  } while (iterate);

  VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Added %d trampolines in %d chunks to fix invalid exit edges", trampoline_count, chunk_trampoline_count));
  VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Created %d chunks, of which %d are made mobile", chunk_count, chunk_mobile_id));

  if (frontend_id == 4
      && chunk_mobile_id > 0)
    ASSERT(aspire_options.code_mobility, (SVM "%u mobile SoftVM chunks created, but code mobility is disabled (add --code_mobility)!", chunk_mobile_id));
}

static t_bool
CanTranslateBbl(t_bbl *bbl)
{
  t_cfg_edge *edge;

  BBL_FOREACH_SUCC_EDGE(bbl, edge)
    if (CFG_EDGE_CAT(edge) == ET_SWITCH
        || CFG_EDGE_CAT(edge) == ET_IPSWITCH)
      return FALSE;

  return TRUE;
}

static t_bool
ShouldNotTranslateBbl(t_bbl *bbl)
{

  Region *region;
  const SoftVMAnnotationInfo *info;
  t_bool enabled;
  t_bool override_enabled;

  if (BBL_FUNCTION(bbl)
      && StringCmp(FUNCTION_NAME(BBL_FUNCTION(bbl)), "_start") == 0)
    return TRUE;

  /* the fact that this function is executed means that the BBL is marked
   * for translation */
  override_enabled = FALSE;
  BBL_FOREACH_SOFTVM_REGION(bbl, region, info)
  {
    int v = 0;
    if (RegionGetValueForIntOption(region, enable_softvm_option.c_str(), v)
        && v == 1)
    {
      override_enabled = TRUE;
      enabled = TRUE;
    }
    else if (RegionGetValueForIntOption(region, disable_softvm_option.c_str(), v)
              && v == 1)
    {
      override_enabled = TRUE;
      enabled = FALSE;
    }
  }

  if (override_enabled)
    return !enabled;

  return FALSE;
}

static t_bool
RegionIsMobileSoftVM(Region *region)
{
  int v = 0;
  if (RegionGetValueForIntOption(region, mobile_softvm_option.c_str(), v))
    return v == 1;

  return false;
}

t_bool SoftVMSelectBblFromRegionChecker(t_bbl *bbl)
{
  if (ShouldNotTranslateBbl(bbl))
    return FALSE;

  if (BblIsMarked2(bbl))
    return FALSE;

  return TRUE;
}

/*
 * We'll mark the instructions selected for export to the SoftVM and then split
 * into new bbl's that match the blocks of consecutive selected instructions.
 */
int
AspireSoftVMMarkAndSplit(t_cfg *cfg, t_ptr_array *chunks, t_randomnumbergenerator *rng)
{
  Region *region;
  const SoftVMAnnotationInfo *info;

  /* list of BBLs containing translatable instructions */
  t_ptr_array pbbl_list;
  PtrArrayInit(&pbbl_list, FALSE);

  /* translatable instructions */
  InsInitMark(cfg);

  /* translatable BBLs */
  BblMarkInit();

  /* processed BBLs */
  BblMarkInit2();

  BblInitInChunk(cfg);
  BblInitSoftVMMobile(cfg);

  /* look for translatable instructions */
  CFG_FOREACH_SOFTVM_REGION(cfg, region, info)
  {
    vector<t_bbl *> selected_bbls;
    SelectBblsFromRegion(selected_bbls, SoftVMSelectBblFromRegionChecker, region, 100, rng, ENABLE_PROFILE_BASED);

    t_bool is_mobile = RegionIsMobileSoftVM(region);

    for (t_bbl *bbl : selected_bbls)
    {
      t_ins *ins;

      BblMark2(bbl);

      if (is_mobile)
        BBL_SET_SOFTVM_MOBILE(bbl, true);

      /* mark translatable instructions in this BBL */
      BBL_FOREACH_INS(bbl, ins)
      {
        if (IsInsSupportedByVM(T_ARM_INS(ins)))
        {
          if (INS_REFERS_TO(ins)
              && ARM_INS_OPCODE(T_ARM_INS(ins)) != ARM_ADDRESS_PRODUCER) {
            VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Not considering @I because it has one or more TO relocatables", ins));
            continue;
          }

          INS_SET_MARK(ins, TRUE);

          if (!BblIsMarked(bbl))
          {
            PtrArrayAdd(&pbbl_list, bbl);
            BblMark(bbl);
          }
        }
      }

      if (!CanTranslateBbl(bbl))
        INS_SET_MARK(BBL_INS_LAST(bbl), FALSE);

      if (!BblContainsMarkedIns(bbl))
        BblUnmark(bbl);
    }
  }

  /* construct chunks */
  CreateChunks(cfg, chunks, &pbbl_list);

#if ONE_CHUNK
  /* remove all chunks except the selected one */
  VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Removing all chunks except chunk #%d (this is a DEBUG preprocessor definition)", CHUNK_INDEX));

  for (int i=0; i<CHUNK_INDEX; i++)
    PtrArrayRemove(chunks, 0, FALSE);
  while (PtrArrayCount(chunks) > 1)
    PtrArrayRemove(chunks, 1, FALSE);
#endif

  /* compute liveness information prior to refreshing the chunks */
  CfgComputeLiveness (cfg, TRIVIAL);
  CfgComputeLiveness (cfg, CONTEXT_INSENSITIVE);
  CfgComputeLiveness (cfg, CONTEXT_SENSITIVE);

  int nr_mobile_chunks = 0;

  for (int i=0; i<PtrArrayCount(chunks); ++i)
  {
    t_vmchunk *nchunk = static_cast<t_vmchunk *>(PtrArrayGet(chunks, i));

    if (VMCHUNK_MOBILE_ID(nchunk) != -1)
      nr_mobile_chunks++;

    /* Update chunk properties now it's complete */
    VmChunkRefresh(nchunk);

#if DEBUG_SOFTVM
    VERBOSE(SOFTVM_VERBOSITY_LEVEL, (SVM "Printing chunk %d (%s) [%d]", i, VMCHUNK_IP(nchunk) ? "IP" : "regular", PtrArrayCount(VMCHUNK_BBLS(nchunk))));
    VmChunkPrint(nchunk);
#endif

	if (diablosoftvm_options.generate_dots_softvm)
	  CfgDrawFunctionGraphsWithHotness(VMCHUNK_CFG(nchunk), "./dots-newcfg");
  }
  /* }}} */

  PtrArrayFini(&pbbl_list, FALSE);

  InsFiniMark(cfg);
  BblFiniInChunk(cfg);
  BblFiniSoftVMMobile(cfg);

  return nr_mobile_chunks;
}

/*
 * Export the data for the SoftVM in JSON. The input should be an array of chunks,
 * each chunk represented by a list of bbl's.
 */
void
AspireSoftVMExport(t_cfg *cfg, t_ptr_array *chunks)
{
  /* Export */
  ASSERT(diablosoftvm_options.extractor_output_file, ("Please provide the extractor output JSON file path and name."));

  /* generate the array of chunks */
  json_t *json_root = ChunkArray2Json(cfg, chunks, FALSE, FALSE, 0);

  if (json_dump_file(json_root, diablosoftvm_options.extractor_output_file, JANSSON_JSON_FLAGS))
    FATAL(("Could not write JSON output file %s", diablosoftvm_options.extractor_output_file));

  JanssonFreeObject(json_root);
}

void
AspireSoftVMFixups(t_cfg *cfg, t_ptr_array *chunks)
{
  bin2vm_status_code_t b2vStatus = BIN2VM_STATUS_SUCCESS;
  bin2vm_list_vmimages_arm *vmImages = NULL;

  /* calculate the relocations originating from the chunks */
  for (auto it = dummies_to_chunks.begin(); it != dummies_to_chunks.end(); it++)
  {
    t_arm_ins *dummy_ins = it->first;
    t_vmchunk *target_chunk = it->second;

    t_ins_to_reloc_map *i2r_map = static_cast<t_ins_to_reloc_map *>(VMCHUNK_RELOC_MAP(target_chunk));

    /* iterate over all relocations associated with the dummy instruction */
    for (t_reloc_ref *rref = ARM_INS_REFERS_TO(dummy_ins); rref; rref = RELOC_REF_NEXT(rref))
    {
      t_reloc *rel = RELOC_REF_RELOC(rref);

      /* look for the associated instruction in the chunk */
      t_arm_ins *associated_ins = NULL;
      for (auto it = i2r_map->begin(); (it != i2r_map->end()) && (associated_ins == NULL); it++)
        /* first = instruction in the chunk */
        /* second = vector of relocations associated with the instruction */
        for (t_reloc *r : it->second)
          if (r == rel)
          {
            associated_ins = it->first;
            /* no support for instructions with multiple relocations yet */
            ASSERT(it->second.size() == 1, ("uh-oh! no support for instructions having multiple relocations yet @I (%d relocations)", associated_ins, it->second.size()));
            break;
          }

      /* sanity check: we should have found the associated instruction */
      ASSERT(associated_ins, ("could not find associated instruction for @R", rel));

      /* only support for address producers for now */
      ASSERT(ARM_INS_OPCODE(associated_ins) == ARM_ADDRESS_PRODUCER, ("implement support for me! @I", associated_ins));

      /* now execute the stack code, where we only take the calculated result into account */
      ARM_INS_SET_IMMEDIATE (associated_ins, G_T_UINT32 (StackExec (RELOC_CODE(rel), rel, NULL, NULL, FALSE, 0, CFG_OBJECT (cfg))));
    }
  }

  /* assemble all needed data in JSON format and convert it to a C-string */
#if SOFTVM_DEBUGCOUNTER
  json_t *json_root = ChunkArray2Json(cfg, chunks, TRUE, TRUE, SOFTVM_DEBUGCOUNTER_VALUE);
#else
  json_t *json_root = ChunkArray2Json(cfg, chunks, TRUE, FALSE, 0);
#endif
  char *json_string = json_dumps(json_root, JANSSON_JSON_FLAGS);
#if DEBUG_SOFTVM
  json_dump_file(json_root, "phase2.json", JANSSON_JSON_FLAGS);
#endif

  /* set mobile code output directory if necessary */
  if (chunk_mobile_id > 0)
  {
    ASSERT(code_mobility_options.output_dir, ("mobile SoftVM chunks found but the code mobility output directory is not set (--code_mobility_output_dir <string>)!"));
    DirMake(code_mobility_options.output_dir, FALSE);
    Bin2VmSetMobileCodeOutputDir(phase2_session, code_mobility_options.output_dir);
  }

  /* invoke the second phase pass of the X-translator (i.e., the post-linker fixups) */
  b2vStatus = Bin2VmDiabloPhase2(phase2_session, json_string, strlen(json_string),
#if DEBUG_SOFTVM
                                 "vmStart_phase2.s",
#else
                                 NULL,
#endif
                                 NULL, &vmImages);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("Something went wrong in the X-translator during the phase-2 pass (%d)", b2vStatus));
  
  json_decref(json_root);

  /* patch the bytecode already present in the binary with the newly generated, post-linker fixed, bytecode */
  bin2vm_list_vmimages_arm *vmImage = vmImages;
  int imageIdx = 0;
  while (vmImage)
  {
    std::string symn;
    char *symn_cstr;
    t_symbol *bytecode_sym;
    t_bbl *databbl;
    t_uint32 patched_bytes;
    t_vmchunk *chunk = static_cast<t_vmchunk*>(PtrArrayGet(chunks, imageIdx));

    /* look up the original bytecode in the layout */
    symn = "vmImage" + std::to_string(imageIdx);
    symn_cstr = new char [symn.length()+1];
    std::strcpy(symn_cstr, symn.c_str());
    VERBOSE(SOFTVM_VERBOSITY_LEVEL, ("[SoftVM] Patching bytecode %s (size %d)", symn_cstr, vmImage->size));

    bytecode_sym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(CFG_OBJECT(cfg)), symn_cstr);
    ASSERT (bytecode_sym, ("Bytecode symbol '%s' not found!", symn_cstr));

    delete[] symn_cstr;

    /* look up data section */
    t_section * sec = T_SECTION(SYMBOL_BASE(bytecode_sym));

    /* reallocate the data section if necessary */
    /*if (!AddressIsEq(SECTION_CSIZE(sec), AddressNew32(vmImage->size)))
    {
      VERBOSE(SOFTVM_VERBOSITY_LEVEL, ("allocated section @T of size @G is not equal to new image %u (0x%x) for bytecode symbol @S; reallocating the section data!", sec, SECTION_CSIZE(sec), vmImage->size, vmImage->size, bytecode_sym));

      SECTION_SET_CSIZE(sec, AddressNew32(vmImage->size));
      SECTION_SET_DATA(sec, Realloc(SECTION_DATA(sec), vmImage->size));
    }*/
    ASSERT(AddressNew32(vmImage->size) <= G_T_UINT32(SECTION_CSIZE(sec)), ("got bigger VM image in second phase of X-translator!"));

    /* look up the data array pointer */
    char * bytecode_section_data = static_cast<char *>(SECTION_DATA(sec));
    bytecode_section_data += G_T_UINT32(SYMBOL_OFFSET_FROM_START(bytecode_sym));
    ASSERT(AddressIsLe(AddressAddUint32(SYMBOL_OFFSET_FROM_START(bytecode_sym), vmImage->size), SECTION_CSIZE(sec)),
            ("new image of size %u (0x%x) does not fit in section @T of size @G (bytecode symbol @S)", vmImage->size, vmImage->size, sec, SECTION_CSIZE(sec), bytecode_sym));

    /* copy over the new bytecode image */
    if (VMCHUNK_MOBILE_ID(chunk) != -1)
    {
      /* this is a mobile chunk, special section contents needed here! */
      t_uint32 *vmimage_data = static_cast<t_uint32 *>(SECTION_DATA(sec));
      vmimage_data[0] = 0x80000000 | vmImage->size;
      vmimage_data[1] = VMCHUNK_MOBILE_ID(chunk);

      VERBOSE(SOFTVM_VERBOSITY_LEVEL, ("[SoftVM] chunk %u is mobile: size 0x%08x (%u), mobility ID 0x%08x (%u)", imageIdx, vmImage->size, vmImage->size, VMCHUNK_MOBILE_ID(chunk), VMCHUNK_MOBILE_ID(chunk)));
    }
    else
    {
      /* this is a persistent chunk, copy over the new bytecode */
      memcpy(bytecode_section_data, vmImage->data, vmImage->size);

      VERBOSE(SOFTVM_VERBOSITY_LEVEL, ("[SoftVM] chunk %u is persistent: size 0x%08x (%u)", imageIdx, vmImage->size, vmImage->size));
    }

    vmImage = vmImage->next;

#if SOFTVM_DEBUGCOUNTER
    if (imageIdx >= SOFTVM_DEBUGCOUNTER_VALUE)
      break;
#endif
    imageIdx++;
  }

  /* free up used memory */
  b2vStatus = Bin2VmFreeVmImagesArm(&vmImages);
  ASSERT(b2vStatus == BIN2VM_STATUS_SUCCESS, ("Something went wrong in the X-translator when freeing the list of VM images"));

  free(json_string);
}

void
AspireSoftVMReadExtractorOutput(t_cfg * cfg, t_ptr_array *diablo_chunks, t_ptr_array *chunks)
{
  json_t *root;
  json_error_t error;

  ASSERT(diablosoftvm_options.extractor_output_file, ("Please provide the extractor output JSON file path and name."));

  ifstream script_file(diablosoftvm_options.extractor_output_file);
  string script_contents((istreambuf_iterator<char>(script_file)), std::istreambuf_iterator<char>());

  root = json_loads(script_contents.c_str(), 0, &error);
  script_contents = "";

  ASSERT(root, ("Problem parsing JSON file '%s'; error on line %d, '%s'", diablosoftvm_options.extractor_output_file, error.line, error.text));
  ASSERT(json_is_object(root), ("Problem parsing JSON: expected object as root element"));

  /* get array of chunks */
  json_t * chunk_array = json_object_get(root, "chunks");
  ASSERT(json_is_array(chunk_array), ("Problem parsing JSON: expected array as chunk list"));

  size_t chunk_index;
  json_t * chunk;

  BblMarkInit();

  /* iterate over every chunk */
  json_array_foreach(chunk_array, chunk_index, chunk)
  {
    /* iterate over all BBLs */
    ASSERT(json_is_object(chunk), ("Problem parsing JSON: expected object as chunk %d", chunk_index));

    /* for now only one bbl is contained in this list */
    json_t * bbl_list = json_object_get(chunk, "bbls");

    size_t bbl_index;
    json_t *bbl_desc;

    /* for now only one BBL per chunk is supported */
    json_array_foreach(bbl_list, bbl_index, bbl_desc)
    {
      /* retrieve function name of this BBL */
      json_t * function_name_json = json_object_get(bbl_desc, "function name");
      ASSERT(function_name_json, ("No function name given for chunk %d/%d", chunk_index, bbl_index));
      ASSERT(json_is_string(function_name_json), ("Expected string to be given for field \"function name\" in chunk %d/%d", chunk_index, bbl_index));
      t_const_string function_name = json_string_value(function_name_json);

      /* retrieve offset of BBL in the function */
      json_t * function_offset_json = json_object_get(bbl_desc, "function offset");
      ASSERT(json_is_string(function_offset_json), ("Expected integer to be given for field \"function offset\" in chunk %d/%d", chunk_index, bbl_index));
      t_const_string function_offset_str = json_string_value(function_offset_json);
      t_address function_offset = AddressNew32(StringToUint32(function_offset_str, 0));

      /* look up the BBL in the CFG */
      t_function * i_fun;
      t_bool found = FALSE;

      CFG_FOREACH_FUN(cfg, i_fun)
      {
        /* find the function the BBL is in */
        if (!FUNCTION_NAME(i_fun)) continue;

        if(StringPatternMatch(function_name, FUNCTION_NAME(i_fun)))
        {
          t_bbl * i_bbl;
          t_bbl * function_entry = FUNCTION_BBL_FIRST(i_fun);

          FUNCTION_FOREACH_BBL(i_fun, i_bbl)
          {
            t_address offset = AddressSub(BBL_CADDRESS(i_bbl), BBL_CADDRESS(function_entry));

            if (offset == function_offset)
            {
              found = TRUE;

              /* Look up the chunk containing this BBL */
              for (int i=0; i<PtrArrayCount(diablo_chunks); i++)
              {
                t_vmchunk * c = static_cast<t_vmchunk *>(PtrArrayGet(diablo_chunks, i));

                if (BblIsMarked(i_bbl))
                  continue;

                if (VmChunkHasBbl(c, i_bbl) != -1)
                {
                  /* we have found the chunk in which this BBL resides */
#if DEBUG_SOFTVM
                  VERBOSE(SOFTVM_VERBOSITY_LEVEL+1, (SVM "Found BBL @iB for chunk %d/%d in Diablo-chunk %d", i_bbl, chunk_index, bbl_index, i));
#endif
                  PtrArrayAdd(chunks, c);

                  t_ptr_array *chunk_bbls = VMCHUNK_BBLS(c);
                  for (int j = 0; j < PtrArrayCount(chunk_bbls); j++)
                    BblMark(static_cast<t_bbl *>(PtrArrayGet(chunk_bbls, j)));

                  break;
                }
              }
            }

            if (found)
              break;
          }
        }

        if (found)
          break;
      }
    }
  }

  /* the amount of chunks selected by Diablo (diablo_chunks), and the amount of chunks in the sorted extractor output
   * does not have to be equal. This is the case, for example, when additional object files are linked into the input
   * binary for the obfuscator pass. */
   
  /* free up memory */
  json_decref(root);
}

void CfgEdgeKillSoftVM(t_cfg_edge *edge)
{
  if (!implemented_chunks_init)
    return;

  /* check whether this edge is an exit edge of a chunk */
  for (int i = 0; i < PtrArrayCount(&implemented_chunks); i++)
  {
    t_vmchunk *chunk = static_cast<t_vmchunk *>(PtrArrayGet(&implemented_chunks, i));
    t_edge_to_edge_map *e2e_map = static_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));

    for (auto it = e2e_map->begin(); it != e2e_map->end(); it++)
    {
      if (it->second != edge)
        continue;

      //TODO: handle exit edge killing
      FATAL(("Killing an exit edge of chunk %d @E (%p)", i, edge, edge));
    }
  }
}

void BblKillSoftVM(t_bbl *bbl)
{
  if (!implemented_chunks_init)
    return;

  for (int i = 0; i < PtrArrayCount(&implemented_chunks); i++)
  {
    t_vmchunk *chunk = static_cast<t_vmchunk *>(PtrArrayGet(&implemented_chunks, i));
    t_edge_to_edge_map *e2e_map = static_cast<t_edge_to_edge_map *>(VMCHUNK_EXIT_EDGE_MAP(chunk));

    for (auto it = e2e_map->begin(); it != e2e_map->end(); it++)
    {
      t_cfg_edge *exit_edge = it->second;
      if (CFG_EDGE_TAIL(exit_edge) == bbl)
      {
        //TODO: handle exit point killing
        FATAL(("Killing an exit block of chunk %d @eiB", i, bbl));
      }
    }
  }
}
