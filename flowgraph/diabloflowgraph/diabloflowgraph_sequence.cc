#include "diabloflowgraph.hpp"

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

static bool
ReadOneSequenceBlock (FILE *fp, t_uint32 nbbls, t_bbl **bblarr)
{
  bool result = true;

  t_address address;
  t_profile_file_count count;
  t_profile_file_address value;
  t_bbl *bbl;

  t_cfg *cfg = BBL_CFG(bblarr[0]);
  t_bbl *tempbbl = BblNew(cfg);

  t_uint32 read_count = 0;
  t_uint32 nbbls_in_file = 0;
  while (!feof (fp))
  {
    if ((read_count > 0) && (read_count == (nbbls_in_file+1)))
      break;

    if (diabloflowgraph_options.rawprofiles)
    {
      count=0;
      if (!(read (fileno(fp), &value, sizeof(t_profile_file_address)) && read (fileno(fp), &count,sizeof(t_profile_file_count)))) {
        result = false;
        break;
      }
    }
    else
    {
      t_uint32 count_read;
      if (!(fscanf (fp, "%" PRIx64 " %u\n", &value, &count_read)))
        continue;
      count = (t_profile_file_count) count_read;
    }

    read_count++;

    if (read_count == 1) {
      nbbls_in_file = G_T_UINT32(value);
      continue;
    }


    address = AddressNewForCfg (cfg, value);

    t_bbl** fbbl;

    BBL_SET_OLD_ADDRESS(tempbbl, address);
    fbbl=(t_bbl**)bsearch(&tempbbl,bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);
    if (fbbl)
      BBL_SET_SEQUENCE_ID(*fbbl, count);
    else
      continue;
  }

  CFG_FOREACH_BBL(cfg, bbl)
  {
    if (BBL_IS_HELL (bbl))
      continue;
    if (!BBL_FUNCTION(bbl))
      continue;
    if (BBL_IS_LAST(bbl))
      continue;

    {
      if (BBL_SEQUENCE_ID(bbl) >= 0LL)
        continue;
      
      FATAL(("boem @eiB", bbl));
      BBL_SET_SEQUENCE_ID(bbl, 0LL);
    }
  }

  BblKill(tempbbl);

  return result;
}

void
ReadSequenceData(t_cfg * cfg, t_string name, t_address before_address, BblSet& before, t_address after_address, BblSet& after) {
  FILE *fp = fopen (name, "r");
  ASSERT(fp, ("can't open sequence file %s", name));

  /* construct sorted list of bbls */
  t_uint32 nbbls=0;
  t_bbl *bbl;
  CFG_FOREACH_BBL(cfg,bbl)
    nbbls++;
  t_bbl **bblarr = static_cast<t_bbl **>(Calloc(nbbls, sizeof(t_bbl *)));

  nbbls = 0;

  CFG_FOREACH_BBL(cfg, bbl)
  {
    bblarr[nbbls] = bbl;
    nbbls++;
    BBL_SET_SEQUENCE_ID(bbl, 0LL);
  }

  /* sort blocks according to address */
  for (t_uint32 i=0; i < nbbls; i++)
    if (bblarr[i] == NULL)
      FATAL(("null bbl 1"));
  diablo_stable_sort(bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);
  for (t_uint32 i=0; i < nbbls; i++)
    if (bblarr[i] == NULL)
      FATAL(("null bbl 2"));

  t_bbl *tempbbl = BblNew(cfg);

  /* look up the before bbl */
  BBL_SET_OLD_ADDRESS(tempbbl, before_address);
  t_bbl *first_bbl = *(t_bbl**)bsearch(&tempbbl,bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);
  ASSERT(first_bbl, ("can't find first bbl with address @G", before_address));

  /* look up the after bbl */
  bool have_after = !AddressIsEq(after_address, AddressNew32(~0));
  t_bbl *last_bbl = NULL;
  if (have_after) {
    BBL_SET_OLD_ADDRESS(tempbbl, after_address);
    last_bbl = *(t_bbl**)bsearch(&tempbbl,bblarr, nbbls, sizeof(t_bbl *), __helper_compare_bbls);
    ASSERT(last_bbl, ("can't find first bbl with address @G", after_address));
  }

  BblKill(tempbbl);

  /* read each data block */
  int block_id = 0;
  while (true) {
    bool ok = ReadOneSequenceBlock(fp, nbbls, bblarr);
    if (!ok)
      break;

    block_id++;

    if (have_after)
      ASSERT(BBL_SEQUENCE_ID(first_bbl) < BBL_SEQUENCE_ID(last_bbl), ("what?\nfirst @eiB\nlast @eiB", first_bbl, last_bbl));

    for (t_uint32 i = 0; i < nbbls; i++) {
      t_bbl *bbl = bblarr[i];

      /* skip non-executed bbls */
      if (BBL_SEQUENCE_ID(bbl) == 0)
        continue;
      
      if (BBL_SEQUENCE_ID(bbl) < BBL_SEQUENCE_ID(first_bbl))
        before.insert(bbl);
      else if (have_after
                && (BBL_SEQUENCE_ID(last_bbl) < BBL_SEQUENCE_ID(bbl)))
        after.insert(bbl);
    }
  }

  Free(bblarr);

  fclose(fp);
}