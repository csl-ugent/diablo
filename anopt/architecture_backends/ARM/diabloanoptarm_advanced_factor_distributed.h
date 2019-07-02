#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_DISTRIBUTED_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_DISTRIBUTED_H

#define DISTTBL_VERBOSITY 10

struct AFDistributedTable {
  AFDistributedTable(t_cfg *cfg);
  void AllocateSlot(t_uint32 offset, t_bbl *from, t_bbl *to, t_reloc *& reloc);
  t_uint32 NextFreeSlotIndex();
  void Print();
  bool IsSlotFree(t_uint32 index);
  bool Verify();
  void AllocateDummySlot(t_uint32 index);
  void RedirectEmptyEntries();

  t_section *sec;

private:
  void AddSlot();

  std::vector<t_section *> subsections;
  std::vector<bool> subsections_used;
  std::vector<t_reloc *> subsections_relocations;
  std::vector<bool> subsections_dummy;
  t_cfg *cfg;
  int id;
};

void CalculateBaseAddresses(t_cfg *cfg);
std::string ListTableIds(t_bbl *bbl);
t_uint32 DistributedTableAddEntry(t_bbl *source, t_bbl *from, t_bbl *to, std::vector<t_reloc *>& relocs);
bool DistributedTableCopyIds(t_bbl *from, t_bbl *to);
void PropagateTableIds(BblVector &worklist);
void ReportDTableStatistics();
void VerifyTables();
size_t BblAssociatedTableIdCount(t_bbl *bbl);
bool SliceCanTransformWithDistributedTable(Slice *slice);
void RedirectEmptyDTableEntries();
void RecordDTableDispatcher(t_bbl *bbl);
bool BblSetsTableId(t_bbl *bbl);
std::set<size_t> BblAssociatedTables(t_bbl *bbl);
void HandleBblKillDistributedSwitch(t_bbl *bbl);

#define TLS_BASEADDRESS_SYMBOL_NAME "tls_af_baseaddress"

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_DISTRIBUTED_H */
