#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

static vector<AFDistributedTable *> tables;
static t_randomnumbergenerator *base_index_rng;

BblSet rebased_bbls;
BblSet dtable_dispatchers;

/* keep track of the number of rebases done per function */
map<t_function *, int> fun_rebase_count;

void RecordDTableDispatcher(t_bbl *bbl) {
  dtable_dispatchers.insert(bbl);
}

void HandleBblKillDistributedSwitch(t_bbl *bbl) {
  if (rebased_bbls.find(bbl) != rebased_bbls.end())
    rebased_bbls.erase(bbl);
}

AFDistributedTable::AFDistributedTable(t_cfg *p_cfg)
{
  static int global_id = 0;

  cfg = p_cfg;
  id = global_id;

  t_string name = StringIo("$af_dtable%d", id);
  sec = SectionCreateForObject(ObjectGetLinkerSubObject(CFG_OBJECT(cfg)), DATA_SECTION, SectionGetFromObjectByName(CFG_OBJECT(cfg), ".data"), AddressNew32(0), name);

  subsections_used.clear();
  subsections.clear();
  subsections_relocations.clear();
  subsections_dummy.clear();

  global_id++;
}

void AFDistributedTable::Print()
{
  stringstream ss;
  ss << "T" << id;
  if (id < 10) ss << " ";
  ss << ": ";
  for (size_t i = 0; i < subsections.size(); i++) {
    bool is_used = subsections_used[i];
    bool is_dummy = subsections_dummy[i];

    if (is_dummy)
      ss << "?";
    else if (is_used)
      ss << "x";
    else
      ss << ".";
  }
  ss << " (" << subsections.size() << ")";
  DEBUG(("%s", ss.str().c_str()));
}

static
t_bbl *GetRandomDTableDispatcher() {
  auto it = dtable_dispatchers.begin();
  advance(it, RNGGenerateWithRange(af_rng_redirect, 0, dtable_dispatchers.size() - 1));

  return *it;
}

void AFDistributedTable::AllocateDummySlot(t_uint32 index) {
  ASSERT(!subsections_used[index], ("what? %d/%d", id, index));
  subsections_dummy[index] = true;

  VERBOSE(DISTTBL_VERBOSITY, ("allocating dummy slot %d/%d", id, index));

  t_reloc *reloc;
  AllocateSlot(index, GetRandomDTableDispatcher(), GetGlobalRedirectRelocTo(cfg), reloc);
}

void AFDistributedTable::RedirectEmptyEntries() {
  for (size_t index = 0; index < subsections.size(); index++) {
    if (subsections_used[index])
      continue;

    AllocateDummySlot(index);
  }
}

void AFDistributedTable::AllocateSlot(t_uint32 index, t_bbl *from, t_bbl *to, t_reloc *& reloc)
{
  /* add spacers if needed */
  while (subsections.size() <= index)
    AddSlot();

  ASSERT(!subsections_used[index], ("what? %d/%d", id, index));

  VERBOSE(DISTTBL_VERBOSITY, ("allocating slot in %d/%d at offset %d", id, index, index*4));
  VERBOSE(DISTTBL_VERBOSITY, ("   from @eiB to @eiB", from, to));

  /* we need to calculate a PC-relative offset between two BBLs */
  reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)), AddressNew32(8),
                                          T_RELOCATABLE(sec), AddressNew32(index * 4),
                                          T_RELOCATABLE(to), AddressNew32(0),
                                          FALSE, NULL, NULL, NULL,
                                          "R00R01-A00-" "\\" WRITE_32);
  RelocAddRelocatable(reloc, T_RELOCATABLE(from), AddressNew32(0));

  RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(cfg)),
                                  AddressNullForObject(CFG_OBJECT(cfg)),
                                  T_RELOCATABLE(BBL_INS_FIRST(from)), AddressNew32(0),
                                  T_RELOCATABLE(sec), AddressNew32(index * 4),
                                  FALSE, NULL, NULL, NULL,
                                  "R00A00+\\*\\s0000$");
  /* TODO: keep sparse subsections alive by adding relocations to them too! */

  subsections_used[index] = true;
  subsections_relocations[index] = reloc;
}

t_uint32 AFDistributedTable::NextFreeSlotIndex()
{
  t_uint32 result = 0;

  while (result < subsections_used.size()
          && subsections_used[result] == true)
    result++;

  return result;
}

bool AFDistributedTable::IsSlotFree(t_uint32 index)
{
  bool result = false;

  if (index >= subsections_used.size())
    result = true;
  else
    result = !subsections_used[index];

  if (result && RNGGeneratePercent(af_rng_dummy) <= static_cast<t_uint32>(diabloanoptarm_options.af_dummy_entry_chance))
      result = false;

  return result;
}

void AFDistributedTable::AddSlot()
{
  SECTION_SET_CSIZE(sec, SECTION_CSIZE(sec) + 4);
  SECTION_SET_DATA(sec, Realloc(SECTION_DATA(sec), SECTION_CSIZE(sec)));

  subsections.push_back(NULL);
  subsections_used.push_back(false);
  subsections_relocations.push_back(NULL);
  subsections_dummy.push_back(false);
}

bool AFDistributedTable::Verify()
{
  bool result = true;

  DEBUG(("verifying table %d", id));

  for (size_t i = 0; i < subsections_used.size(); i++)
  {
    if (!subsections_used[i])
      continue;
    if (subsections_dummy[i])
      continue;

    t_reloc *rel = subsections_relocations[i];

    ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[0]) == RT_BBL, ("what? @R", rel));
    ASSERT(RELOC_SWITCH_EDGE(rel), ("no edge associated with relocation %d! @R", i, rel));
    t_bbl *dst = CFG_EDGE_TAIL(T_CFG_EDGE(RELOC_SWITCH_EDGE(rel)));
    bool ok = T_BBL(RELOC_TO_RELOCATABLE(rel)[0]) == dst;

    if (!ok)
    {
      DEBUG(("   fixup entry %d (offset %d)!", i, i*4));
      RelocSetToRelocatable(rel, 0, T_RELOCATABLE(dst));
      result = false;
    }
  }

  return result;
}

void VerifyTables()
{
  for (auto table : tables)
    table->Verify();
}

t_uint32 DistributedTableAddEntry(t_bbl *source, t_bbl *from, t_bbl *to, vector<t_reloc *>& relocs)
{
  /* assume that this code is only executed when a list of tables is available */
  ASSERT(BBL_TABLE_ID_VECTOR(source), ("no table list available! @eiB", source));

  /* look for the next available slot in the tables */
  for (auto table_id : *BBL_TABLE_ID_VECTOR(source))
    tables[table_id]->Print();

  t_uint32 common_index = 0;
#define MAX_INDEX 1000000
  while (common_index < MAX_INDEX)
  {
    bool all_ok = true;

    for (auto table_id : *BBL_TABLE_ID_VECTOR(source))
      if (!tables[table_id]->IsSlotFree(common_index))
      {
        all_ok = false;
        break;
      }

    if (all_ok) break;

    common_index++;
  }
  ASSERT(common_index < MAX_INDEX, ("max supported table entry count is %d, got %d", MAX_INDEX, common_index));
  VERBOSE(DISTTBL_VERBOSITY, ("smallest common index: %d", common_index));

  /* create an entry in each of the tables at the calculated offset */
  for (auto table_id : *BBL_TABLE_ID_VECTOR(source))
  {
    t_reloc *reloc;
    tables[table_id]->AllocateSlot(common_index, from, to, reloc);
    ASSERT(reloc, ("what? no relocation!"));
    relocs.push_back(reloc);
  }

  return common_index;
}

static
bool BblAssociateWithTableId(t_bbl *bbl, size_t id)
{
  if (BBL_TABLE_ID_VECTOR(bbl) == NULL)
    BBL_SET_TABLE_ID_VECTOR(bbl, new set<size_t>());

  auto old_size = BBL_TABLE_ID_VECTOR(bbl)->size();
  BBL_TABLE_ID_VECTOR(bbl)->insert(id);

  return old_size != BBL_TABLE_ID_VECTOR(bbl)->size();
}

bool DistributedTableCopyIds(t_bbl *from, t_bbl *to)
{
  bool changed = false;

  if (!BBL_TABLE_ID_VECTOR(from))
    return changed;

  for (auto table_id : *BBL_TABLE_ID_VECTOR(from))
    changed |= BblAssociateWithTableId(to, table_id);

  return changed;
}

size_t BblAssociatedTableIdCount(t_bbl *bbl)
{
  if (!BBL_TABLE_ID_VECTOR(bbl))
    return 0;

  return BBL_TABLE_ID_VECTOR(bbl)->size();
}

bool BblSetsTableId(t_bbl *bbl)
{
  return rebased_bbls.find(bbl) != rebased_bbls.end();
}

std::string ListTableIds(t_bbl *bbl)
{
  stringstream ss;

  if (BblSetsTableId(bbl))
    ss << "*";

  if (BBL_TABLE_ID_VECTOR(bbl))
    for (auto id : *BBL_TABLE_ID_VECTOR(bbl))
      ss << id << ",";

  return ss.str();
}

static
bool BblInheritTableIds(t_bbl *from, t_bbl *to)
{
  /* skip self-referencing edges */
  if (to == from)
    return false;

  /* don't need to propagate to determined BBLs */
  if (BBL_IS_HELL(to))
    return false;
  if (BblSetsTableId(to))
    return false;

  /* watch out! */
  t_cfg_edge *edge;
  BBL_FOREACH_PRED_EDGE(to, edge)
    if (BBL_IS_HELL(CFG_EDGE_HEAD(edge)))
      return false;

  size_t old_count = BblAssociatedTableIdCount(to);

  ASSERT(BBL_TABLE_ID_VECTOR(from), ("no table ID list for @eiB", from));
  ASSERT(!BBL_IS_HELL(to), ("destination BBL is hell @eiB", to));

  for (auto it : *BBL_TABLE_ID_VECTOR(from))
    BblAssociateWithTableId(to, it);

  return (old_count != BblAssociatedTableIdCount(to));
}

static
bool BblClearTableIds(t_bbl *bbl) {
  if (BblAssociatedTableIdCount(bbl) == 0)
    return false;

  if (BblSetsTableId(bbl))
    return false;

  BBL_TABLE_ID_VECTOR(bbl)->clear();
  return true;
}

static
t_bbl *NextBbl(t_cfg_edge *e)
{
  t_bbl *result = CFG_EDGE_TAIL(e);

  /* correctly handle interprocedural edges */
  if (CfgEdgeIsInterproc(e))
  {
    switch (CFG_EDGE_CAT(e))
    {
    case ET_IPSWITCH:
    case ET_IPFALLTHRU:
    case ET_IPJUMP:
    case ET_CALL:
    case ET_SWI:
    case ET_RETURN:
    case ET_COMPENSATING:
      /* Propagate the information directly to the return site,
       * but only if the destination is HELL! */
      if (BBL_IS_HELL(result))
      {
        ASSERT(CFG_EDGE_CORR(e), ("no corresponding edge for call @E", e));
        result = CFG_EDGE_TAIL(CFG_EDGE_CORR(e));
      }
      break;

    default: FATAL(("unsupported edge @E @eiB", e, result));
    }
  }

  return result;
}

void PropagateTableIds(BblVector &worklist)
{
  t_cfg *cfg = NULL;
  if (worklist.size() > 0)
    cfg = BBL_CFG(worklist[0]);

  /* worklist contains a list of BBLs that are the roots of
   * the propagation process */
  while (worklist.size() > 0)
  {
    t_bbl *subject = worklist.back();
    worklist.pop_back();

    BblMarkInit();
    BblVector new_worklist = {subject};

    /* new_worklist contains a list of BBLs that still need to be updated,
     * and thus that need to have their successors updated */
    while (new_worklist.size() > 0)
    {
      /* get the next unmarked element from the worklist */
      t_bbl *from = NULL;
      while (!from || BblIsMarked(from))
      {
        if (new_worklist.size() == 0) {
          from = NULL;
          break;
        }
        from = new_worklist.back();
        new_worklist.pop_back();
      }
      if (!from)
        continue;

      BblMark(from);

      /* propagate the table id information */
      t_cfg_edge *e;
      BBL_FOREACH_SUCC_EDGE(from, e)
      {
        t_bbl *to = NextBbl(e);

        if (BblInheritTableIds(from, to))
        {
          BblUnmark(to);
          new_worklist.push_back(to);
        }
      }
    }
  }

  if (!cfg)
    return;

  /* WARNING
   * This is a VERY ugly fix. Apparently the previous algorithm sometimes propagates table IDs to BBLs
   * for which not all predecessors actually have a table ID set. This is of course wrong, and BBLs that
   * don't have a table ID set in each and every predecessor should not have table IDs propagated to them.
   * But it Just Works (tm). */
  ASSERT(worklist.size() == 0, ("worklist not empty"));

  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg, bbl)
    if (BblAssociatedTableIdCount(bbl) == 0)
      worklist.push_back(bbl);

  while (worklist.size() > 0) {
    t_bbl *subject = worklist.back();
    worklist.pop_back();

    BblMarkInit();
    BblVector new_worklist = {subject};

    while (new_worklist.size() > 0) {
      t_bbl *from = NULL;
      while (!from || BblIsMarked(from)) {
        if (new_worklist.size() == 0) {
          from = NULL;
          break;
        }
        from = new_worklist.back();
        new_worklist.pop_back();
      }
      if (!from) continue;

      BblMark(from);

      t_cfg_edge *e;
      BBL_FOREACH_SUCC_EDGE(from, e) {
        t_bbl *to = NextBbl(e);

        if (BblClearTableIds(to)) {
          BblUnmark(to);
          new_worklist.push_back(to);
        }
      }
    }
  }
}

static
void RefreshBaseAddress(t_bbl *bbl)
{
  /* we want to prepend the base address calculation to the BBL */
  t_regset dead = BblRegsLiveBefore(bbl);
  RegsetSetInvers(dead);
  RegsetSetIntersect(dead, CFG_DESCRIPTION(BBL_CFG(bbl))->int_registers);
  RegsetSetSubReg(dead, ARM_REG_R15);

  /* calculate random base index */
  size_t random_base_index = RNGGenerate(base_index_rng);
  ASSERT(random_base_index < tables.size(), ("what? %d %d", random_base_index, tables.size()));

  /* regs[0] is the table address register
   * regs[1] is the section address register */
  t_reg regs[2];
  int nr = 0;

  t_reg r;
  REGSET_FOREACH_REG(dead, r)
  {
    regs[nr] = r;
    nr++;

    /* we've found 2 registers */
    if (nr == 2) break;
  }

  if (BBL_NINS(bbl) > 0)
  {
    t_bbl *last = BblSplitBlock(bbl, BBL_INS_FIRST(bbl), TRUE);
    AfterSplit(bbl, last);

    /* redirect successor edges to the BBL properly */
    t_cfg_edge *e;
    BBL_FOREACH_SUCC_EDGE(last, e)
      if (CFG_EDGE_TAIL(e) == bbl)
        CfgEdgeChangeTail(e, last);
  }

  t_regset candidates = RegsetDiff(CFG_DESCRIPTION(BBL_CFG(bbl))->int_registers, dead);
  t_regset saved_registers = NullRegs;
  int nr_saved = 0;
  REGSET_FOREACH_REG(candidates, r)
  {
    if (nr == 2) break;

    RegsetSetAddReg(saved_registers, r);
    regs[nr] = r;
    nr++;
    nr_saved++;
  }

  VERBOSE(DISTTBL_VERBOSITY, ("rebasing r%d/r%d %d @B", regs[0], regs[1], random_base_index, bbl));

  /* create the address producer instruction */
  t_arm_ins *new_ins;
  ArmMakeInsForBbl(Mov, Prepend, new_ins, bbl, FALSE, regs[0], ARM_REG_NONE, 0, ARM_CONDITION_AL);
  t_reloc *reloc = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(CFG_OBJECT(BBL_CFG(bbl))), AddressNew32(0),
                                                    T_RELOCATABLE(new_ins), AddressNew32(0),
                                                    T_RELOCATABLE(tables[random_base_index]->sec), AddressNew32(0),
                                                    FALSE, NULL, NULL, NULL,
                                                    "R00A00+" "\\" WRITE_32);
  ArmInsMakeAddressProducer(new_ins, random_base_index, reloc);
  InsMarkAfIndexInstruction(T_INS(new_ins));
  AFFactoringLogInstruction(new_ins);

  TlsStore(bbl, T_INS(new_ins), regs[0], regs[1], TLS_BASEADDRESS_SYMBOL_NAME);

  /* save and restore the used live registers */
  if (nr_saved > 0)
  {
    t_reg r;
    t_uint32 regs = 0;
    REGSET_FOREACH_REG(saved_registers, r)
      regs |= 1<<r;

    t_arm_ins *new_ins;
    ArmMakeInsForBbl(Push, Prepend, new_ins, bbl, FALSE, regs, ARM_CONDITION_AL, FALSE);
    AFFactoringLogInstruction(new_ins);
    ArmMakeInsForBbl(Pop, Append, new_ins, bbl, FALSE, regs, ARM_CONDITION_AL, FALSE);
    AFFactoringLogInstruction(new_ins);
  }

  /* keep track of the rebased BBLs */
  BblAssociateWithTableId(bbl, random_base_index);
  rebased_bbls.insert(bbl);

  fun_rebase_count[BBL_FUNCTION(bbl)]++;
}

bool CanRefreshInBbl(t_bbl *bbl)
{
  if (BBL_IS_HELL(bbl) || BblIsExitBlock(bbl) || IS_DATABBL(bbl))
    return false;

  if (!BBL_CAN_TRANSFORM(bbl))
    return false;

  /* only do rebases within functions */
  if (!BBL_FUNCTION(bbl))
    return false;

  /* we should be able to add instructions to the BBL */
  if (!CanAddInstructionToBbl(bbl))
    return false;

  if (BblSetsTableId(bbl))
    return false;

  return true;
}

set<t_function *> forbidden_functions;
void CalculateBaseAddresses(t_cfg *cfg)
{
  /* certain functions can't be touched by this transformation */
  forbidden_functions = CfgDontTouchTheseFunctions(cfg);

  /* first we need to create our own TLS symbol (thread-safe) */
  TlsCreate(CFG_OBJECT(cfg), TLS_BASEADDRESS_SYMBOL_NAME);

  /* how many different bases do we want to use? */
  size_t tablecount = 0;

  t_function *fun;
  CFG_FOREACH_FUN(cfg, fun)
  {
    if (FUNCTION_IS_HELL(fun))
      continue;

    tablecount++;
    fun_rebase_count[fun] = 0;
  }

  /* upper bound */
  if (tablecount > static_cast<size_t>(diabloanoptarm_options.af_dtable_max_tables))
    tablecount = static_cast<size_t>(diabloanoptarm_options.af_dtable_max_tables);

  VERBOSE(0, ("creating %d tables", tablecount));

  /* create instances of the base tables */
  for (size_t i = 0; i < tablecount; i++)
    tables.push_back(new AFDistributedTable(cfg));

  base_index_rng = RNGCreateChild(af_rng_distributed, "af_distributed_base_index");
  RNGSetRange(base_index_rng, 0, tables.size()-1);

  t_randomnumbergenerator *rebase_chance = RNGCreateChild(af_rng_distributed, "af_distributed_rebase");

  size_t nr_rebased = 0;

  t_bbl *bbl;

  /* first make sure all the BBLs that should set a new value do so */
  BblSet bbl_set;
  CFG_FOREACH_BBL(cfg, bbl)
  {
    /*if (nr_rebased >= diablosupport_options.debugcounter)
      break;*/

    if (!CanRefreshInBbl(bbl)) continue;

    if (forbidden_functions.find(BBL_FUNCTION(bbl)) != forbidden_functions.end())
      continue;

    /* should this BBL be considered? */
    bool should_refresh = false;
    t_cfg_edge *e;
    BBL_FOREACH_PRED_EDGE(bbl, e)
    {
      /* this should also include return-from-HELL, as e.g.,
       * blx <hell> instructions can call a function that sets the table ID */
      if (BBL_IS_HELL(CFG_EDGE_HEAD(e))/* && CFG_EDGE_CAT(e) != ET_RETURN*/)
      {
        should_refresh = true;
        break;
      }

      /* returning from __aeabi function? */
#define AEABI_FUNCTION_PREFIX "__aeabi_"
      t_function *fun = BBL_FUNCTION(CFG_EDGE_HEAD(e));
      if (CFG_EDGE_CAT(e) == ET_RETURN
          && fun && FUNCTION_NAME(fun)
          && !strncmp(FUNCTION_NAME(fun), AEABI_FUNCTION_PREFIX, strlen(AEABI_FUNCTION_PREFIX)))
      {
        should_refresh = true;
        break;
      }
    }

    if (!should_refresh)
    {
      if (RNGGeneratePercent(rebase_chance) <= static_cast<t_uint32>(diabloanoptarm_options.af_dtable_rebase_chance))
        should_refresh = true;
    }

    /* only consider the must-do BBLs here */
    if (!should_refresh)
      continue;

    bbl_set.insert(bbl);
    nr_rebased++;
  }

  for (auto bbl : bbl_set)
    RefreshBaseAddress(bbl);

  VERBOSE(0, ("rebased %d", nr_rebased));

  BblVector worklist;
  for (auto x : rebased_bbls)
    worklist.push_back(x);
  PropagateTableIds(worklist);

  RNGDestroy(base_index_rng);
  RNGDestroy(rebase_chance);
}

void ReportDTableStatistics()
{
  VERBOSE(0, ("DTABLE STATISTICS"));
  for (auto table : tables)
    table->Print();
}

bool SliceCanTransformWithDistributedTable(Slice *slice)
{
  ASSERT(BBL_FUNCTION(slice->Bbl()), ("not in function! @eiB", slice->Bbl()));

  if (forbidden_functions.find(BBL_FUNCTION(slice->Bbl())) == forbidden_functions.end())
    return true;

  return false;
}

void RedirectEmptyDTableEntries() {
  for (auto table : tables)
    table->RedirectEmptyEntries();
}

std::set<size_t> BblAssociatedTables(t_bbl *bbl) {
  set<size_t> result;

  if (BBL_TABLE_ID_VECTOR(bbl)) {
    for (auto x : *BBL_TABLE_ID_VECTOR(bbl))
      result.insert(x);
  }

  return result;
}