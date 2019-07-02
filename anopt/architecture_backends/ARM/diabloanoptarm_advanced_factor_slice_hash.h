#ifndef DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_HASH_H
#define DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_HASH_H

#define N_INS_IN_SLICE_KEY 4

typedef t_uint64 SliceKeyType;
typedef std::function<bool(t_bbl *)> F_IsMasterBbl;
typedef std::function<bool(t_bbl *)> F_IsSlaveBbl;

/* TODO: maybe templates here */
struct SliceHashTableKey {
  union {
    SliceKeyType key;
    InstructionFingerprintType values[N_INS_IN_SLICE_KEY];
  };
};

class SliceHashTable {
public:
  typedef std::multimap<SliceKeyType, Slice *> HashTable;
  typedef std::pair<SliceKeyType, Slice *> HashTableEntry;

  SliceHashTable(size_t slice_size, F_IsMasterBbl p_IsMaster, F_IsSlaveBbl p_IsSlave);

  std::pair<HashTable::iterator, HashTable::iterator> GetSameKeyIterators(Slice *slice) {
    return hashtable.equal_range(CreateKeyForSlice(slice));
  }

  void Add(Slice *slice) {
    hashtable.insert(HashTableEntry(CreateKeyForSlice(slice), slice));
  }

  std::vector<SliceKeyType> GetSortedKeyVectorByBucketSize();
  SliceVector GetSortedSliceVectorForKey(SliceKeyType key);
  std::vector<SliceSet> SortSlices(SliceVector all_sorted);

  void RemoveSingles();
  void RemoveUseless();
  void PrintStatistics();

  bool RangeIsUseful(HashTable::iterator first, HashTable::iterator last);

  size_t SliceSize() {
    return N;
  }

  void Print();

private:
  /* calculate the hash table key for this slice
   * based on the template parameter N */
  SliceKeyType CreateKeyForSlice(Slice *slice, unsigned int start_ins_idx = 0);

  size_t N;

  HashTable hashtable;
  F_IsMasterBbl IsMaster;
  F_IsSlaveBbl IsSlave;
};

class SliceHashTableManager {
public:
  SliceHashTableManager(F_IsMasterBbl p_IsMaster, F_IsSlaveBbl p_IsSlave) {
    IsMaster = p_IsMaster;
    IsSlave = p_IsSlave;
  }

  ~SliceHashTableManager() {
    for (auto ht : hashtables)
      delete ht;
  }

  void Add(Slice *slice) {
    size_t limit = slice->Size();
    bool include_singleton = diabloanoptarm_options.af_min_block_length >= 1;
    if (!include_singleton)
      limit--;

    while (hashtables.size() < limit)
      hashtables.push_back(new SliceHashTable(hashtables.size() + 1 + (include_singleton ? 0 : 1), IsMaster, IsSlave));

    for (size_t i = 0; i < limit; i++)
      hashtables[i]->Add(slice);
  }

  std::vector<SliceHashTable *> HashTables() {
    return hashtables;
  }

  void Clean() {
    for (auto ht : hashtables)
    {
      ht->RemoveSingles();
      ht->RemoveUseless();
    }
  }

  void Print();

private:
  std::vector<SliceHashTable *> hashtables;

  F_IsMasterBbl IsMaster;
  F_IsSlaveBbl IsSlave;
};

void AddBblToHashTableManager(SliceHashTableManager& mgr, t_bbl *bbl, F_SliceIsMaster IsMaster, F_SliceIsSlave IsSlave, SliceSet& created);

#endif /* DIABLOANOPTARM_ADVANCED_FACTOR_SLICE_HASH_H */
