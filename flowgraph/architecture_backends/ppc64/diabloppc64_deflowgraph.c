/*
 * Copyright (C) 2006 Lluis Vilanova <xscript@gmx.net> {{{
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * }}}
 */

#include <diabloppc64.h>

static void Ppc64RestoreGotToOriginalSize(t_object *obj)
{
  t_object *linker = ObjectGetLinkerSubObject(obj);
  t_section *got = SectionGetFromObjectByName(obj, ".got");
  t_section *newsub;
  t_section **subs;
  unsigned i, nsubs;
  t_address newsubsize;

  if (!got) return;

  subs = SectionGetSubsections(got, &nsubs);
  ASSERT(nsubs,(".got section has no subsections"));

  newsubsize =
    AddressSub(AddressAdd(SECTION_OLD_ADDRESS(subs[nsubs-1]),
                          SECTION_OLD_SIZE(subs[nsubs-1])),
               SECTION_OLD_ADDRESS(got));
  newsub = SectionCreateForObject(linker, SECTION_TYPE(got), got,
                                  newsubsize, "_fixed_got");
  SECTION_SET_OLD_ADDRESS(newsub, SECTION_OLD_ADDRESS(got));
  SECTION_SET_CADDRESS(newsub, SECTION_CADDRESS(got));

  for (i = 0; i < nsubs; ++i)
  {
    t_reloc_ref *rr;
    t_section *sub = subs[i];
    t_uint32 offset =
      AddressExtractUint32(AddressSub(SECTION_OLD_ADDRESS(sub),
                                      SECTION_OLD_ADDRESS(got)));
    char *dest = ((char *)SECTION_DATA(newsub)) + offset;
    char *src = (char *) SECTION_DATA(sub);
    memcpy(dest, src, AddressExtractUint32(SECTION_CSIZE(sub)));

    while ((rr = SECTION_REFERS_TO(sub)))
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);
      RelocSetFrom(rel, T_RELOCATABLE(newsub));
      RELOC_SET_FROM_OFFSET(rel,
                            AddressAddUint32(RELOC_FROM_OFFSET(rel), offset));
    }
    while ((rr = SECTION_REFED_BY(sub)))
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);
      int j;
      for (j = 0; j < RELOC_N_TO_RELOCATABLES(rel); ++j)
      {
        if (RELOC_TO_RELOCATABLE(rel)[j] == T_RELOCATABLE(sub))
        {
          RelocSetToRelocatable(rel, j, T_RELOCATABLE(newsub));
          RELOC_TO_RELOCATABLE_OFFSET(rel)[j] = 
            AddressAddUint32(RELOC_TO_RELOCATABLE_OFFSET(rel)[j], offset);
        }
      }
    }

    SectionKill(sub);
  }

  Free(subs);
}

/* Ppc64Deflowgraph {{{ */
/*! Build a linear representation of the program.
 *
 * Layouts a flowgraph, and builds a linear list of instructions that can be
 * executed. 
 *
 * \return void 
 */
void Ppc64Deflowgraph(t_object *obj)
{
  t_section *sec;
  int i;

  /* because of problems with shrinking TOCs on multi-TOC 
   * binaries, we restore the TOC to its original size by
   * inserting empty subsections at the locations where
   * previously subsections were deleted.
   * XXX this is a hack! A better long-term solution is needed! */
  Ppc64RestoreGotToOriginalSize(obj);

  PpcDeflowgraph (obj);

  STATUS(START, ("Patch the entry point"));
  for (i = 0; i < OBJECT_NDATAS (obj); i++)
  {
    sec = OBJECT_DATA (obj)[i];
    if (!StringCmp (SECTION_NAME (sec), ".opd"))
    {
      t_address saddr;
      t_symbol *sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE (obj), "_start");
      ASSERT(sym, ("Couldn't find symbol _start"));
      saddr = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);
      ASSERT(AddressIsEq (saddr, SECTION_CADDRESS (sec)),
             ("_start (@G) is not the first descriptor of .opd (@G)",
              saddr, SECTION_CADDRESS (sec)));
      OBJECT_SET_ENTRY (obj, SECTION_CADDRESS (sec));
      VERBOSE(1, ("New entry: @G", OBJECT_ENTRY (obj)));
      break;
    }
  }
  STATUS(STOP,  ("Patch the entry point"));
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
