/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Includes {{{ */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <diabloobject.h>
/*}}}*/

 /*#define VERBOSE_PLACEMENT  */
 /*#define VERBOSE_PLACEMENT2 */

t_segment *gseg = NULL;
static struct {
  t_bool in_overlay;
  t_uint64 start;
  t_uint64 max_size;
} overlay;

/* {{{ Helper functions */
static int
_sec_compare_caddresses (const t_section * const* s1, const t_section * const* s2)
{
  if (AddressIsLt (SECTION_CADDRESS(*s1), SECTION_CADDRESS(*s2)))
    return -1;
  if (AddressIsGt (SECTION_CADDRESS(*s1), SECTION_CADDRESS(*s2)))
    return 1;
  return 0;
}

static int
_sec_compare_old_addresses (const t_section * const* s1, const t_section * const* s2)
{
  ASSERT(!AddressIsNull(SECTION_OLD_ADDRESS(*s1)), ("old address is null! @T", *s1));
  ASSERT(!AddressIsNull(SECTION_OLD_ADDRESS(*s2)), ("old address is null! @T", *s2));

  if (AddressIsLt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
    return -1;
  if (AddressIsGt (SECTION_OLD_ADDRESS(*s1), SECTION_OLD_ADDRESS(*s2)))
    return 1;
  return 0;
}

static int
_sec_compare_align (const t_section * const* s1, t_section * const* s2)
{
  if (AddressIsLt (SECTION_ALIGNMENT(*s1), SECTION_ALIGNMENT(*s2)))
    return 1;
  if (AddressIsGt (SECTION_ALIGNMENT(*s1), SECTION_ALIGNMENT(*s2)))
    return -1;
  return 0;
}

/* sort sections by old address, but put all code sections up front */
static int
_sec_reorder (const t_section ** a, const t_section ** b)
{
  const t_section *seca = *a;
  const t_section *secb = *b;

  /* Assure that .interp is the first section */
  if (strcmp(SECTION_NAME(seca), ".interp") == 0)
    return -1;
  /* Assure that .interp is the first section */
  else if (strcmp(SECTION_NAME(secb), ".interp") == 0)
    return 1;
  else if (SECTION_TYPE(seca) == SECTION_TYPE(secb))
    return AddressIsLt (SECTION_CADDRESS(seca),
                        SECTION_CADDRESS(secb)) ? -1 : 1;
  else if (SECTION_TYPE(seca) == CODE_SECTION)
    return -1;
  else if (SECTION_TYPE(secb) == CODE_SECTION)
    return 1;
  else
    return AddressIsLt (SECTION_CADDRESS(seca),
                        SECTION_CADDRESS(secb)) ? -1 : 1;

  /* keep the compiler happy */
  return 0;
}

/* sort the array of code sections in the OBJECT_CODE array */
static int
_object_code_reorder (const t_section ** a, const t_section ** b)
{
  const t_section *seca = *a;
  const t_section *secb = *b;

  return AddressIsLt (SECTION_CADDRESS(seca),
                        SECTION_CADDRESS(secb)) ? -1 : 1;
}
/* }}} */
/* {{{ ObjectPlaceSectionsContiguously: place a given set of parent sections contiguously in memory,
 * adjusting address and size for all of them (and their subsecs)
 * PARAMS:
 * - secs: the array of parent sections
 * - nsecs: the number of sections in the array
 * - start: address in memory where placing starts
 * - end_align: enlarge the last section so that
 * the first free address is aligned to end_align
 * - pad_sections: indicates whether the parent sections
 * should be resized to fill the gaps created by
 * alignment. We want to do this on the final section
 * layout, but not while reordering sections before code
 * section merging (screws up $switch symbols for padding
 * in code sections...)
 * RETURNS:
 * the address of the first free byte of memory
 */
static t_address
ObjectPlaceSectionsContiguously (t_section ** secs, t_uint32 nsecs,
                                 t_address start, t_address end_align,
                                 t_bool pad_sections)
{
  t_uint32 i, j;
  t_bool entry_set = FALSE;

  t_section *subsec, *sec, *prev_sec;
  t_address first_free, tmp_addr, prev_base;

  t_uint32 nsubsecs = 0;
  t_section **subsecarr = NULL;

  /* the start address should be at the right alignment */
  ASSERT((AddressIsEq (start, AddressAlign (SECTION_ALIGNMENT(secs[0]), start))),
         ("Start address @G not properly aligned", start));

  first_free = start;
  for (i = 0; i < nsecs; i++)
  {
    sec = secs[i];
    prev_sec = i ? secs[i - 1] : NULL;

    subsecarr = SectionGetSubsections (sec, &nsubsecs);

    /* Align the section. if padding must be added, enlarge the previous
     * section accordingly */
    tmp_addr = AddressAlign (SECTION_ALIGNMENT(sec), first_free);
    if (pad_sections && prev_sec)
      SECTION_SET_CSIZE(prev_sec, AddressAdd (SECTION_CSIZE(prev_sec), AddressSub (tmp_addr, first_free)));
    first_free = tmp_addr;
#ifdef VERBOSE_PLACEMENT
    t_address old_address = SECTION_CADDRESS(sec);
    t_address new_address = first_free;

    VERBOSE(0, ("SECTION %s before @G after @G align @G size @G", SECTION_NAME(sec),
                old_address, new_address, SECTION_ALIGNMENT(sec), SECTION_CSIZE(sec)));
#endif
    /* set the new address of the parent section */
    SECTION_SET_CADDRESS(sec, first_free);

    /* set the new address of the subsections */
    /* preserve relative distance between subsections, even if alignment constraints
     * allow for smaller distances (this can happen due to some alignment weirdness
     * with expanded map files) */
    prev_base = AddressNullForSection (sec);
    for (j = 0; j < nsubsecs; j++)
    {
      subsec = subsecarr[j];
      first_free = AddressAlign (SECTION_ALIGNMENT(subsec), first_free);
      if (!AddressIsNull (prev_base))
      {
        /* preserve the distance to the previous subsection */
        //t_address old = first_free;
        first_free = AddressAdd (SECTION_CADDRESS(subsecarr[j - 1]),
                                 AddressSub (SECTION_CADDRESS(subsec), prev_base));
      }
      if (!AddressIsNull (SECTION_CSIZE(subsec)))
        prev_base = SECTION_CADDRESS(subsec);


      if ((AddressIsGe(OBJECT_ENTRY(SECTION_OBJECT(sec)), SECTION_CADDRESS(subsec)))
      && (AddressIsLt(OBJECT_ENTRY(SECTION_OBJECT(sec)), AddressAdd(SECTION_CADDRESS(subsec), SECTION_CSIZE(subsec)))) && (entry_set == FALSE))
      {
        entry_set = TRUE;
        OBJECT_SET_ENTRY(SECTION_OBJECT(sec), AddressAdd(first_free, AddressSub(OBJECT_ENTRY(SECTION_OBJECT(sec)), SECTION_CADDRESS(subsec))));
      }


      SECTION_SET_CADDRESS(subsec, first_free);
      first_free = AddressAdd (first_free, SECTION_CSIZE(subsec));
#ifdef VERBOSE_PLACEMENT
      VERBOSE(0, ("\tsub %s (size @G) before @G after @G(offsets @G <-> @G)",
                  SECTION_NAME(subsec), SECTION_CSIZE(subsec), prev_base, SECTION_CADDRESS(subsec),
                  AddressSub (prev_base, old_address),
                  AddressSub (SECTION_CADDRESS(subsec), new_address)));
      VERBOSE(0, ("\t\tOLD @G", SECTION_OLD_ADDRESS(subsec)));
      VERBOSE(0, ("\t\tfrom %s", OBJECT_NAME (SECTION_OBJECT (subsec))));
#endif
    }

    /* set the size of the parent section */

    /* This is a hack.... TODO: Fix me */
    SECTION_SET_CSIZE(sec, AddressSub (first_free, SECTION_CADDRESS(sec)));

    if (subsecarr)
      Free (subsecarr);
  }

  /* change the size of the last section so that the first free address is
   * properly aligned */
  tmp_addr = AddressAlign (end_align, first_free);
  SECTION_SET_CSIZE(secs[nsecs - 1], AddressAdd (SECTION_CSIZE(secs[nsecs - 1]), AddressSub (tmp_addr, first_free)));
  first_free = tmp_addr;

  return first_free;
} /* }}} */

/* SectionPutAtAddress {{{ */
static t_uint64
SectionPutAtAddress (t_section * sec, t_uint64 start, const t_layout_rule * r,
                     t_bool min, t_bool reorder_for_min_alignment_losses, t_bool place_subsecs)
{
  t_object *obj = SECTION_OBJECT(sec);
  t_uint64 secaddr, endaddr, tmpaddr, padding;
  int i, j;
  t_uint32 nsubs, align;
  t_section **subs;

  /* check if the script specifies a start address */
  if (r && r->u.secspec->address)
    secaddr = LayoutScriptCalcExp (r->u.secspec->address, start, obj);
  else
    secaddr = start;
  ASSERT(secaddr >= start,
         ("Section %s: requested address < first free address", SECTION_NAME(sec)));

  /* make sure the section is aligned properly */
  align = AddressExtractUint32 (SECTION_ALIGNMENT(sec));
  if (align)
    secaddr = (secaddr + (align - 1)) & ~((t_uint64) (align - 1));
  start = secaddr;

#ifdef VERBOSE_PLACEMENT
  VERBOSE(0, ("section %s address 0x%llx", SECTION_NAME(sec), secaddr));
#endif

  if (min)
    SECTION_SET_MIN_ADDRESS(sec, AddressNewForObject (obj, secaddr));
  else
    SECTION_SET_CADDRESS(sec, AddressNewForObject (obj, secaddr));
  subs = SectionGetSubsections (sec, &nsubs);

  t_bool is_overridden = FALSE;
  DiabloBrokerCall("CustomSubsectionPlacement", sec, subs, nsubs, &is_overridden);

  if (!is_overridden)
  {
    if (reorder_for_min_alignment_losses)
    {
      if (SECTION_TYPE(sec) == DATA_SECTION ||
          SECTION_TYPE(sec) == RODATA_SECTION ||
          SECTION_TYPE(sec) == BSS_SECTION ||
          SECTION_TYPE(sec) == TLSDATA_SECTION ||
          SECTION_TYPE(sec) == TLSBSS_SECTION)
      {
        /* don't change the order of the TLS sections */
        if (SECTION_TYPE(sec) != TLSDATA_SECTION &&
            SECTION_TYPE(sec) != TLSBSS_SECTION) {
          if (!diabloobject_options.data_order_seed)
          {
            diablo_stable_sort(subs, nsubs, sizeof (t_section *),
                  (int (*)(const void *, const void *)) _sec_compare_align);
          }
          else
          {
            /* randomise order using a standard shuffle */
            t_section *tmpsec;

            srand(diabloobject_options.data_order_seed);
            for (i = nsubs - 1; i >= 0; --i)
            {
              j = random() % (i + 1);
              tmpsec = subs[j];
              subs[j] = subs[i];
              subs[i] = tmpsec;
            }
          }
        }
        tmpaddr = secaddr;
        for (i = 0; i < nsubs; i++)
        {
          t_section *sub = subs[i];
          align = AddressExtractUint64 (SECTION_ALIGNMENT(sub));
          padding =
            align ? ((tmpaddr + (align - 1)) & ~((t_uint64) (align - 1))) -
            tmpaddr : 0;
          if (padding)
          {
            int bestj = 0;
            int minpad = padding;

            for (j = nsubs - 1; j > i; j--)
            {
              t_section *sub2 = subs[j];

              if (AddressExtractUint32 (SECTION_ALIGNMENT(sub2)) > 1)
                break;
              if (AddressExtractUint64 (SECTION_CSIZE(sub2)) == 0)
                continue;
              if ((AddressExtractUint64 (SECTION_CSIZE(sub2)) % align) ==
                  0)
                continue;
              if (((tmpaddr +
                    AddressExtractUint64 (SECTION_CSIZE(sub2)) + (align -
                                                                  1)) &
                   ~((t_uint64) (align - 1))) - tmpaddr -
                  AddressExtractUint64 (SECTION_CSIZE(sub2)) >= minpad)
                continue;
              minpad =
                (((tmpaddr + AddressExtractUint64 (SECTION_CSIZE(sub2)) +
                   (align - 1)) & ~((t_uint64) (align - 1))) - tmpaddr -
                 AddressExtractUint64 (SECTION_CSIZE(sub2)));
              bestj = j;
              if (minpad == 0)
                break;
            }

            if (minpad != padding)
            {
              t_section *newi = subs[bestj];

              for (j = bestj; j > i; j--)
              {
                subs[j] = subs[j - 1];
              }
              subs[i] = newi;
              i--;
              continue;

            }
          }
          if (align)
            tmpaddr = (tmpaddr + (align - 1)) & ~((t_uint64) (align - 1));
          tmpaddr += AddressExtractUint64 (SECTION_CSIZE(sub));
        }
      }
    }
  }

  /* When linking in new objects after linker emulation we will place all sections, but we don't want
   * the addresses of the original subsections to change relative to eachother. Therefore we will only
   * place the subsections for sections that were linked in, or sections for which new subsections have
   * been generated (this can happen for sections containing dynamic symbol information for example).
   * TODO: Clean up the below code and the associated code in _sec_compare_old_addresses. It is ELF-specific
   * and should be placed somewhere else.
   */
  if(place_subsecs || (SECTION_FLAGS(sec) & SECTION_FLAG_LINKED_IN) || !strcmp(".got", SECTION_NAME(sec))
    || !strcmp(".dynamic", SECTION_NAME(sec)) || ((nsubs != 0) && AddressIsNull(SECTION_OLD_ADDRESS(subs[nsubs - 1]))))
  {
    for (i = 0; i < nsubs; i++)
    {
      t_section *sub = subs[i];

      align = AddressExtractUint32 (SECTION_ALIGNMENT(sub));
      if (align)
        secaddr = (secaddr + (align - 1)) & ~((t_uint64) (align - 1));
      if (min)
        SECTION_SET_MIN_ADDRESS(sub, AddressNewForObject (obj, secaddr));
      else
        SECTION_SET_CADDRESS(sub, AddressNewForObject (obj, secaddr));
      #ifdef VERBOSE_PLACEMENT2
      VERBOSE(0, ("subsection %s address 0x%llx old @G", SECTION_NAME(sub), secaddr, SECTION_OLD_ADDRESS(sub)));
      #endif
      secaddr += AddressExtractUint64 (SECTION_CSIZE(sub));
    }
  }
  else
  {
    for (i = 0; i < nsubs; i++)
    {
      t_section *sub = subs[i];

      SECTION_SET_CADDRESS(sub, AddressAdd(secaddr, AddressSub(SECTION_OLD_ADDRESS(sub), SECTION_OLD_ADDRESS(sec))));

      #ifdef VERBOSE_PLACEMENT2
      VERBOSE(0, ("subsection %s address 0x%llx old @G", SECTION_NAME(sub), secaddr, SECTION_OLD_ADDRESS(sub)));
      #endif
    }

    if (SECTION_TYPE(sec) != ATTRIB_SECTION)
      secaddr += AddressExtractUint64 (SECTION_CSIZE(sec));
  }

  /* If endaddr is equal to start we have a special type of section that has no
   * subsections, e.g., a deflowgraphed section. We use callbacks in that case.
   */

  if (secaddr == start)
  {
    if ((SECTION_TYPE(sec) != CODE_SECTION)
        && (SECTION_TYPE(sec) != DATA_SECTION)
        && (SECTION_TYPE(sec) != TLSDATA_SECTION)
        && (SECTION_TYPE(sec) != RODATA_SECTION)
        && (SECTION_TYPE(sec) != BSS_SECTION)
        && (SECTION_TYPE(sec) != TLSBSS_SECTION)
        && (SECTION_TYPE(sec) != NOTE_SECTION)
        && (SECTION_TYPE(sec) != DEBUG_SECTION)
        && (SECTION_TYPE(sec) != ATTRIB_SECTION))
    {
      if ((!SECTION_CALLBACKS(sec)->SectionRecalculateSize))
      {
        FATAL(("Could not recalculate size of section %s type %c", SECTION_NAME(sec), SECTION_TYPE (sec)));
      }
      if (min)
        secaddr += SECTION_CALLBACKS(sec)->SectionRecalculateSize (sec, MIN_ADDRESS);
      else
        secaddr += SECTION_CALLBACKS(sec)->SectionRecalculateSize (sec, NEW_ADDRESS);
    }

  }

  /* for ATTRIB_SECTIONS, we don't add the subsections (somehow, the linker script does not 
   * automatically make them skipped ... so instead we have to set its new size to its old one
   */
  if (SECTION_TYPE(sec)==ATTRIB_SECTION)
    secaddr += SECTION_CSIZE(sec);

  /* determine the end address of the section */
  if (r && r->u.secspec->internal_rule
      && !strcmp (".", r->u.secspec->internal_rule->lhs))
    endaddr =
      LayoutScriptCalcExp (r->u.secspec->internal_rule->rhs, secaddr, obj);
  else
    endaddr = secaddr;
  ASSERT(secaddr <= endaddr,
         ("section %s: specified end address < first free address",
          SECTION_NAME(sec)));

  if (min)
    SECTION_SET_MIN_SIZE(sec, AddressNewForObject (obj, endaddr - start));
  else
    SECTION_SET_CSIZE(sec, AddressNewForObject (obj, endaddr - start));

#ifdef VERBOSE_PLACEMENT
  VERBOSE(0,
          ("-> 0x%llx size 0x%x", endaddr,
           AddressExtractUint32 (SECTION_CSIZE(sec))));
#endif

  if (subs)
    Free (subs);

  if (overlay.in_overlay)
    SECTION_SET_FLAGS(sec, SECTION_FLAGS(sec) | SECTION_FLAG_IN_OVERLAY);
  else
    SECTION_SET_FLAGS(sec, SECTION_FLAGS(sec) & ~SECTION_FLAG_IN_OVERLAY);

  /* TLSBSS sections are special: they don't take up any virtual address
   * space in PT_LOAD sections (because they're dynamically allocated
   * for each thread)
   */
  if (SECTION_TYPE(sec) != TLSBSS_SECTION)
    return endaddr;
  else
    return start;
}

/* }}} */
/* SectionGetGeneralType {{{ */
/* return general section type (ie the normal section
 * type for non-code sections and CODE_SECTION
 * for all variants of the code section (disassembled,
 * flowgraphed, etc.) */
char
SectionGetGeneralType (const t_section * sec)
{
  switch (SECTION_TYPE(sec))
  {
    case RODATA_SECTION:
    case DATA_SECTION:
    case BSS_SECTION:
    case NOTE_SECTION:
    case TLSBSS_SECTION:
    case TLSDATA_SECTION:
    case ATTRIB_SECTION:
      return SECTION_TYPE(sec);
  }
  return CODE_SECTION;
}

/* }}} */
/* ExecLayoutRule {{{ */
static t_uint64
ExecLayoutRule (const t_layout_rule * r, t_uint64 currpos, t_object * obj,
                t_bool min, t_bool reorder_subsecs, t_bool place_subsecs)
{
  /* skip symbol assignments */
  if (r->kind == ASSIGN && strcmp (r->u.assign->lhs, "."))
    return currpos;

  if (r->kind == ASSIGN)
  {
    if (overlay.in_overlay)
      FATAL(("position assignments not allowed in overlay"));

    currpos = LayoutScriptCalcExp (r->u.assign->rhs, currpos, obj);
#ifdef VERBOSE_PLACEMENT
    VERBOSE(0, (". = %llx", currpos));
#endif
  }
  else if (r->kind == SECTION_SPEC && !r->u.secspec->wildcard)
  {
    /* place section rule */
    t_string secname = r->u.secspec->name;
    t_section *shn = HashTableLookup (OBJECT_SECTION_TABLE(obj), secname);

    if (shn)
      currpos = SectionPutAtAddress (shn, currpos, r, min, reorder_subsecs, place_subsecs);
    if (overlay.in_overlay)
      currpos = overlay.start;
  }
  else if (r->kind == SECTION_SPEC && r->u.secspec->wildcard)
  {
    t_string pattern = r->u.secspec->name;
    t_section *sec;
    int i;
    OBJECT_FOREACH_SECTION (obj, sec, i)
    {
      if (!StringPatternMatch (pattern, SECTION_NAME (sec))) continue;
      currpos = SectionPutAtAddress (sec, currpos, r, min, reorder_subsecs, place_subsecs);
      if (overlay.in_overlay)
        currpos = overlay.start;
    }
  }
  else if (r->kind == SEG_START)
  {
    t_segment *seg = ObjectGetSegmentByName (obj, r->u.segment_ident);

    if (seg)
    {
      SEGMENT_SET_CADDRESS(seg, AddressNewForObject (obj, currpos));
      SEGMENT_SET_HOIST_HEADERS(seg, FALSE);
    }
    else
    {
      seg = SegmentNew (r->u.segment_ident);
      SEGMENT_SET_SEGMENT_NEXT(seg, NULL);
      SEGMENT_SET_HOIST_HEADERS(seg, FALSE);
      if (OBJECT_SEGMENT_FIRST(obj))
      {
        SEGMENT_SET_SEGMENT_NEXT(OBJECT_SEGMENT_LAST(obj), seg);
        SEGMENT_SET_SEGMENT_PREV(seg, OBJECT_SEGMENT_LAST(obj));
        OBJECT_SET_SEGMENT_LAST(obj, seg);
      }
      else
      {
        SEGMENT_SET_SEGMENT_PREV(seg, NULL);
        OBJECT_SET_SEGMENT_FIRST(obj, seg);
        OBJECT_SET_SEGMENT_LAST(obj, seg);
      }
      SEGMENT_SET_CADDRESS(seg, AddressNewForObject (obj, currpos));
    }
    gseg = seg;
  }
  else if (r->kind == SEG_END)
  {
    t_segment *seg = ObjectGetSegmentByName (obj, r->u.segment_ident);
    t_address tmp;

    if (seg)
    {
      tmp = AddressNewForObject (obj, currpos);
      SEGMENT_SET_CSIZE(seg, AddressSub (tmp, SEGMENT_CADDRESS(seg)));
    }
    gseg = NULL;
  }
  else if (r->kind == PUT_SECTIONS)
  {
    t_section *sec;
    t_uint32 i;

    OBJECT_FOREACH_SECTION(obj, sec, i)
    {
      if (SectionGetGeneralType (sec) == r->u.sectype)
        if (!(SECTION_FLAGS(sec) & SF_MARKED))
          currpos =
            SectionPutAtAddress (sec, currpos, NULL, min, reorder_subsecs, place_subsecs);
    }
  }
  else if (r->kind == OVERLAY_START)
  {
    overlay.in_overlay = TRUE;
    overlay.start = currpos;
    overlay.max_size = 0LL;
  }
  else if (r->kind == OVERLAY_END)
  {
    currpos = overlay.start + overlay.max_size;
    overlay.in_overlay = FALSE;
  }
  else
    FATAL(("unknown type of layout rule"));

  return currpos;
}

/* }}} */

/**
 * \todo Document
 */

void
ObjectPlaceSections (t_object * obj, t_bool min, t_bool reorder_subsecs_for_minimal_alignment_loss, t_bool place_subsecs)
{
  t_layout_rule *script;
  t_layout_rule *r;
  t_uint64 currpos;
  int i;
  t_section *sec;

#ifdef VERBOSE_PLACEMENT
  VERBOSE(0, ("Layout started!"));
#endif
  /* {{{ initialization */
  /* turn off the marked and placed flags for all sections */
  OBJECT_FOREACH_SECTION(obj, sec, i)
    SECTION_SET_FLAGS(sec, SECTION_FLAGS(sec) & ~(SF_MARKED));
  /* reset the overlay information structure */
  overlay.in_overlay = FALSE;
  /* }}} */
 
  if (!OBJECT_LAYOUT_SCRIPT(obj))
    OBJECT_SET_LAYOUT_SCRIPT(obj, ObjectGetAndParseLayoutScript (obj));
  script = OBJECT_LAYOUT_SCRIPT(obj)->first;

  /* {{{ first step: mark all sections that are mentioned in the layout script
   * for those that are unmarked after this step, we will have to decide
   * ourselves where to place them */
  for (r = script; r; r = r->next)
  {

    if (r->kind != SECTION_SPEC)
      continue;
    if (!r->u.secspec->wildcard)
    {
      t_section *sec =
        HashTableLookup (OBJECT_SECTION_TABLE(obj), r->u.secspec->name);
      if (!sec)
        continue; /* section removed by diablo */
      SECTION_SET_FLAGS(sec, SECTION_FLAGS(sec) | (SF_MARKED));
    }
    else
    {
      t_section *sec;
      int i;
      OBJECT_FOREACH_SECTION (obj, sec, i)
      {
        if (StringPatternMatch (r->u.secspec->name, SECTION_NAME (sec)))
          SECTION_SET_FLAGS (sec, SECTION_FLAGS (sec) | SF_MARKED);
      }
    }
  } /* }}} */

  /* second step: execute layout script */
  currpos = 0LL;
  for (r = script; r; r = r->next)
  {
    currpos = ExecLayoutRule (r, currpos, obj, min,
                              reorder_subsecs_for_minimal_alignment_loss, place_subsecs);
  }

  /* {{{ third step: reorder the sections in the OBJECT_CODE(obj)[], OBJECT_DATA(obj)[], ... arrays
   * this is necessary as the section order may have changed during the placing of the
   * sections, and there is code in diablo that expects the order to be correct */
  /* NOTE: we need to keep the TLS sections (.tdata, .tbss) in-order according to the old addresses */
  diablo_stable_sort (OBJECT_CODE(obj), OBJECT_NCODES(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  diablo_stable_sort (OBJECT_RODATA(obj), OBJECT_NRODATAS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  diablo_stable_sort (OBJECT_DATA(obj), OBJECT_NDATAS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  diablo_stable_sort (OBJECT_TLSDATA(obj), OBJECT_NTLSDATAS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_old_addresses);
  diablo_stable_sort (OBJECT_BSS(obj), OBJECT_NBSSS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  diablo_stable_sort (OBJECT_TLSBSS(obj), OBJECT_NTLSBSSS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_old_addresses);
  diablo_stable_sort (OBJECT_NOTE(obj), OBJECT_NNOTES(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  /*  diablo_stable_sort (OBJECT_DEBUG(obj), OBJECT_NDEBUGS(obj), sizeof (t_section *),
      (int (*)(const void *, const void *)) _sec_compare_caddresses);*/
  diablo_stable_sort (OBJECT_ATTRIB(obj), OBJECT_NATTRIBS(obj), sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_compare_caddresses);
  /* }}} */

#ifdef VERBOSE_PLACEMENT
  VERBOSE(0, ("Layout finished!"));
#endif

  /* after the layout has been determined, reserve more space for ULEB128 relocations
     in .ARM.extab section if that needs to be kept 
     if that is done, relayout all over again because addresses will change again */

  if (diabloobject_options.keep_exidx)
    if (ObjectAdaptSpaceForLEBRelocs(obj))
      ObjectPlaceSections (obj, min, reorder_subsecs_for_minimal_alignment_loss, place_subsecs);
}

/* }}} */
/* ObjectOrderCodeSectionsContiguously {{{ */
void
ObjectOrderCodeSectionsContiguously (t_object * obj)
{
  t_uint32 i;
  t_section **secarr;
  t_uint32 nsecs;
  t_address tmp_align;
  t_address tlsbssaddr;

  /* we only do this for parent objects */
  ASSERT(OBJECT_SUBOBJECT_CACHE(obj), ("Can't order code sections contiguously for sub objects"));

  /* Note. TLSBSS is intentionally *not* included here, because it's
   * not part of the PT_LOAD segment
   */
  nsecs = OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NDATAS(obj) + OBJECT_NTLSDATAS(obj) + OBJECT_NBSSS(obj) + OBJECT_NNOTES(obj) /*+ OBJECT_NDEBUGS(obj)*/ + OBJECT_NATTRIBS(obj);
  secarr = (t_section **) Calloc (nsecs, sizeof (t_section *));

  memcpy (secarr, OBJECT_CODE(obj), sizeof (t_section *) * OBJECT_NCODES(obj));
  memcpy (secarr + OBJECT_NCODES(obj), OBJECT_RODATA(obj),
          sizeof (t_section *) * OBJECT_NRODATAS(obj));
  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj),  OBJECT_TLSDATA(obj),
          sizeof (t_section *) * OBJECT_NTLSDATAS(obj));
  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NTLSDATAS(obj), OBJECT_DATA(obj),
          sizeof (t_section *) * OBJECT_NDATAS(obj));
  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NTLSDATAS(obj) + OBJECT_NDATAS(obj), OBJECT_BSS(obj),
          sizeof (t_section *) * OBJECT_NBSSS(obj));
  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NTLSDATAS(obj) + OBJECT_NDATAS(obj) + OBJECT_NBSSS(obj),
          OBJECT_NOTE(obj), sizeof (t_section *) * OBJECT_NNOTES(obj));
  /*  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NTLSDATAS(obj) + OBJECT_NDATAS(obj) + OBJECT_NBSSS(obj) + 
      OBJECT_NNOTES(obj), OBJECT_DEBUG(obj), sizeof (t_section *) * OBJECT_NDEBUGS(obj));*/
  memcpy (secarr + OBJECT_NCODES(obj) + OBJECT_NRODATAS(obj) + OBJECT_NTLSDATAS(obj) + OBJECT_NDATAS(obj) + OBJECT_NBSSS(obj) + 
          OBJECT_NNOTES(obj) /*+ OBJECT_NDEBUGS(obj)*/, OBJECT_ATTRIB(obj), sizeof (t_section *) * OBJECT_NATTRIBS(obj));

  /* reorder the sections: code sections will come first */
  diablo_stable_sort (secarr, nsecs, sizeof (t_section *),
         (int (*)(const void *, const void *)) _sec_reorder);

  /* this is a guess: we don't know anything about the end alignment so we take 8 byte,
   * as this is a safe value for the heap begin on 32-bit and 64-bit architectures */
  tmp_align = AddressNewForObject (obj, 8);

  ObjectPlaceSectionsContiguously (secarr, nsecs,
                                   SECTION_CADDRESS(secarr[0]), tmp_align,
                                   FALSE);

  /* now set the addresses for the TLSBSS sections, if any: they follow right
   * after TLSDATA (if any), hence they're at the same address as the start of
   * DATA
   */
  /* sort the TLS sections (.tdata, .tbss) according to the original addresses */
  diablo_stable_sort(OBJECT_TLSBSS(obj), OBJECT_NTLSBSSS(obj), sizeof(t_section *), (int (*)(const void *, const void *)) _sec_compare_old_addresses);
  diablo_stable_sort(OBJECT_TLSDATA(obj), OBJECT_NTLSDATAS(obj), sizeof(t_section *), (int (*)(const void *, const void *)) _sec_compare_old_addresses);

  tlsbssaddr = SECTION_CADDRESS(OBJECT_DATA(obj)[0]);
  for (i = 0; i < OBJECT_NTLSBSSS(obj); i++)
  {
    SECTION_SET_CADDRESS(OBJECT_TLSBSS(obj)[i],tlsbssaddr);
    tlsbssaddr = AddressAdd(tlsbssaddr,SECTION_CSIZE(OBJECT_TLSBSS(obj)[i]));
  }
  
  /* Make sure the addresses in object code are ordered */
  diablo_stable_sort(OBJECT_CODE(obj),OBJECT_NCODES(obj),  sizeof (t_section *), (int (*)(const void *, const void *)) _object_code_reorder);

  Free(secarr);
}

/* }}} */

void
LayoutPrint (const t_object * obj)
{
  t_object *sub, *tmp;

  t_section *sec;
  t_uint32 i;

  OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp)
  {
    OBJECT_FOREACH_SECTION(sub, sec, i)
    {
      if (SECTION_TYPE(sec) == CODE_SECTION)
        continue;
      VERBOSE(0,
              ("%s %s old: @G new: @G\n", OBJECT_NAME(sub), SECTION_NAME(sec),
               SECTION_OLD_ADDRESS(sec), SECTION_CADDRESS(sec)));
    }
  }
}

/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
