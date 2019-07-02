#include "diabloanoptarm_advanced_factor.hpp"

using namespace std;

SliceHashTable::SliceHashTable(size_t slice_size, F_IsMasterBbl p_IsMaster, F_IsSlaveBbl p_IsSlave) {
  /* key size should be >= combined fingerprints size */
  ASSERT(sizeof(SliceKeyType) >= sizeof(InstructionFingerprintType)*N_INS_IN_SLICE_KEY,
         ("incorrect SliceHashTableKey data structure: key size = %d, fingerprint size = %d (%d times, total size %d)",
          sizeof(SliceKeyType), sizeof(InstructionFingerprintType), N_INS_IN_SLICE_KEY, sizeof(InstructionFingerprintType)*N_INS_IN_SLICE_KEY));

  /* intiialise data structures */
  hashtable.clear();
  N = slice_size;

  IsMaster = p_IsMaster;
  IsSlave = p_IsSlave;
}

void SliceHashTable::PrintStatistics() {
  auto it = hashtable.begin();
  int keys = 0;

  while (it != hashtable.end())
  {
    /* count the number of associated values for this key */
    auto range_it = hashtable.equal_range(it->first);
    auto range_size = distance(range_it.first, range_it.second);

    /* go to the next unique key */
    it = hashtable.upper_bound(it->first);

    keys++;
  }
}

vector<SliceKeyType> SliceHashTable::GetSortedKeyVectorByBucketSize() {
  /* first we create a vector in which every entry is a tuple of the
   * key and the number of slices associated with that key. */
  struct KeySizeTuple {
    SliceKeyType key;
    size_t size;
  };
  vector<KeySizeTuple> tmp;

  auto it = hashtable.begin();
  while (it != hashtable.end())
  {
    auto range_it = hashtable.equal_range(it->first);
    size_t count = static_cast<size_t>(distance(range_it.first, range_it.second));

    tmp.push_back(KeySizeTuple{it->first, count});

    it = hashtable.upper_bound(it->first);
  }

  /* then we sort the vector based on the number of associated slices */
  stable_sort(tmp.begin(), tmp.end(), [](KeySizeTuple a, KeySizeTuple b) {
    return a.size > b.size;
  });

  /* now create the result vector, containing only the keys (but sorted) */
  vector<SliceKeyType> result;
  for (auto tuple : tmp)
    result.push_back(tuple.key);

  return result;
}

SliceVector SliceHashTable::GetSortedSliceVectorForKey(SliceKeyType key) {
  SliceVector result;

  auto range_it = hashtable.equal_range(key);
  for (auto it = range_it.first; it != range_it.second; it++)
  {
    if (it->second->IsInvalidated()
        || it->second->NrInstructions() < N)
      continue;

    result.push_back(it->second);
  }

  return result;
}

vector<SliceSet> SliceHashTable::SortSlices(SliceVector all_sorted) {
  vector<SliceSet> result;
  BblMarkInit();

  auto it = all_sorted.begin();
  while (it != all_sorted.end())
  {
    if (!(*it)->IsMarked())
    {
      /* the current slice has not been marked yet, add it to a new vector */
      SliceSet set;

      /* at least add the current slice */
      set.insert(*it);
      auto ref_it = it;

      it++;

      for (auto itt = it; itt != all_sorted.end(); itt++)
      {
        if ((*itt)->IsMarked())
          continue;

        /* this slice has not been marked yet,
         * decide whether to put it in this vector or not */
        size_t nr_match = 0;
        auto r = (*ref_it)->Compare(*itt, nr_match, N);

        if (nr_match >= N)
        {
          /* the number of overlapping instructions is at least 'N' */
          set.insert(*itt);
          (*itt)->Mark();
        }
      }

      /* only add vectors that contain more than one element */
      if (SliceSetConsiderForFactoring(set, N))
        result.push_back(set);
    }
    else
      it++;
  }

  return result;
}

void SliceHashTable::RemoveSingles() {
  auto it = hashtable.begin();
  int removed = 0;

  while (it != hashtable.end())
  {
    /* count the number of associated values for this key */
    auto range_it = hashtable.equal_range(it->first);
    auto range_size = distance(range_it.first, range_it.second);

    /* if this key has only one slice associated with it, remove it */
    if (range_size == 1)
    {
      it = hashtable.erase(it);
      removed++;
    }
    else
      it = hashtable.upper_bound(it->first);
  }
}

void SliceHashTable::RemoveUseless() {
  auto it = hashtable.begin();
  int removed = 0;

  while (it != hashtable.end())
  {
    auto range_it = hashtable.equal_range(it->first);

    if (!RangeIsUseful(range_it.first, range_it.second))
    {
      it = hashtable.erase(range_it.first, range_it.second);
      removed++;
    }
    else
      it = hashtable.upper_bound(it->first);
  }
}

SliceKeyType SliceHashTable::CreateKeyForSlice(Slice *slice, unsigned int start_ins_idx) {
  SliceHashTableKey k;

  for (size_t i = 0U; i < N_INS_IN_SLICE_KEY; i++)
  {
    if (i >= slice->Size() || i >= N)
      k.values[i] = 0;
    else
      k.values[i] = INS_FINGERPRINT(slice->Get(i + start_ins_idx));
  }

  return k.key;
}

bool SliceHashTable::RangeIsUseful(HashTable::iterator first, HashTable::iterator last)
{
  SliceSet set;
  for (auto it = first; it != last; it++)
    set.insert(it->second);

  return SliceSetConsiderForFactoring(set, N);
}

void SliceHashTable::Print()
{
  for (auto& it : hashtable)
  {
    DEBUG(("KEY=%08x", it.first));
    DEBUG(("VALUE=%s", it.second->Print().c_str()));
  }
}

void SliceHashTableManager::Print()
{
  bool no_data = true;
  DEBUG(("Printing contents of hash table manager containing %d hashtables", hashtables.size()));

  for (auto ht : hashtables)
  {
    auto keyvector = ht->GetSortedKeyVectorByBucketSize();

    if (keyvector.size() == 0)
      continue;

    DEBUG(("HT of slice size %u contains %u keys", ht->SliceSize(), keyvector.size()));

    int key_id = 0;
    for (auto key : keyvector)
    {
      DEBUG(("printing key %u/%u", key_id, keyvector.size()));

      auto slices_for_key = ht->GetSortedSliceVectorForKey(key);
      auto slice_sets = ht->SortSlices(slices_for_key);

      int set_id = 0;
      for (auto slice_set : slice_sets)
      {
        if (!SliceSetConsiderForFactoring(slice_set, ht->SliceSize()))
          continue;

        no_data = false;

        DEBUG(("  printing set %u/%u of size %u which contains %u slices", set_id, slice_sets.size(), ht->SliceSize(), slice_set.size()));

        int slice_id = 0;
        for (auto slice : slice_set)
        {
          DEBUG(("    printing slice %u/%u", slice_id, slice_set.size()));
          DEBUG(("%s", slice->Print().c_str()));

          slice_id++;
        }

        set_id++;
      }

      key_id++;
    }
  }

  if (no_data)
    DEBUG(("   no data!"));
}

void AddBblToHashTableManager(SliceHashTableManager& mgr, t_bbl *bbl, F_SliceIsMaster IsMaster, F_SliceIsSlave IsSlave, SliceSet& created)
{
  t_ins *ins;

  if (!BblShouldBeFactored(bbl))
    return;

  if (diabloanoptarm_options.advanced_factoring_enable_sequence) {
    /* add the instruction sequences */
    BBL_FOREACH_INS(bbl, ins)
    {
      {
        Slice *slice = CalculateSliceForInstruction(ins, IsMaster, IsSlave, true, false);
        if (!slice)
          continue;

        slice->ProtectFromDestruction();
        created.insert(slice);

        mgr.Add(slice);
      }

      if (diabloanoptarm_options.advanced_factoring_addressproducers_updown)
      {
        Slice *slice = CalculateSliceForInstruction(ins, IsMaster, IsSlave, true, true);
        if (!slice)
          continue;

        slice->ProtectFromDestruction();
        created.insert(slice);

        mgr.Add(slice);
      }
    }
  }

  if (diabloanoptarm_options.advanced_factoring_enable_slice) {
    /* add the real slices */
    BBL_FOREACH_INS(bbl, ins)
    {
      Slice *slice = CalculateSliceForInstruction(ins, IsMaster, IsSlave, false, false);
      if (!slice)
        continue;

      /* as these slices may be referenced to throughout the whole algorithm,
      * protect them from being destroyed when their first associated instruction is killed.
      * instead, store the slice in a global list that is traversed for destruction later on. */
      slice->ProtectFromDestruction();
      created.insert(slice);

      mgr.Add(slice);

      INS_SET_SLICE(ins, slice);
    }
  }
}
