/*
 * Copyright (C) 2007 Lluis Vilanova <vilanova@ac.upc.edu> {{{
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
 *
 * This file is part of the Cell port of Diablo (Diablo is a better link-time
 * optimizer)
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <diabloppc64.h>
#include <diablospe.h>
#include "diablo_cell_options.h"

/* {{{ the main program structure description data structures
 * TODO these should probably be moved to a generally available header file */
typedef struct {
  t_string name;
  t_string handle;

  t_object *obj;
  t_cfg *cfg;
  t_symbol *handlesym;
} t_spu_binary;

typedef struct _t_program_description {
  t_string ppu_name;
  int nspu;
  t_spu_binary *spu;

  t_object *obj;
  t_cfg *cfg;
} t_program_description;
/* }}} */

/* {{{ ParseProgramDescriptionFile */
#define line_starts_with(line, pref) (!strncmp(line, pref, strlen(pref)))
static t_program_description *ParseProgramDescriptionFile(t_string fname)
{
  FILE *f = fopen(fname, "r");
  t_string line = NULL;
  int len = 0;

  t_program_description *pd = Calloc(1, sizeof(t_program_description));

  ASSERT(f, ("Could not open %s for reading", fname));
  while (getline(&line, &len, f) != -1)
  {
    if (line[0] == '#')
      continue;

    if (line_starts_with(line, "ppu:"))
    {
      /* ppu: <name_of_binary> */
      int nspaces = strspn(line+4, " \t");
      t_string arg = line + 4 + nspaces;
      t_string end = strpbrk(arg, " \t\n\0");
      if (end) *end = '\0';
      ASSERT(!pd->ppu_name, ("Cannot have 2 ppu programs defined"));
      pd->ppu_name = StringDup(arg);
    }
    else if (line_starts_with(line, "spu:"))
    {
      /* spu: <name_of_binary> <handle_symbol> */
      int nspaces = strspn(line+4, " \t");
      t_string arg1 = line + 4 + nspaces;
      t_string end = strpbrk(arg1, " \t\n\0");
      if (!end || *end == '\n' || *end == '\0')
        FATAL(("spu line needs two parameters"));
      *end = '\0';
      
      nspaces = strspn(end+1, " \t");
      t_string arg2 = end + 1 + nspaces;
      end = strpbrk(arg2, " \t\n\0");
      if (end) *end = '\0';

      if (!pd->nspu)
        pd->spu = Malloc(sizeof(t_spu_binary));
      else
        pd->spu = Realloc(pd->spu, sizeof(t_spu_binary) * (pd->nspu + 1));

      pd->spu[pd->nspu].name = StringDup(arg1);
      pd->spu[pd->nspu].handle = StringDup(arg2);
      pd->nspu++;
    }
    else
      FATAL(("unrecognized format"));
  }

  ASSERT(pd->ppu_name, ("We need at least a ppu wrapper program"));
  ASSERT(pd->nspu, ("There should be at least one embedded spu binary"));
  return pd;
} /* }}} */

static void GetSpuBinary(t_spu_binary *sd, t_object *ppu_obj)
{
  t_object *obj;
  t_cfg *cfg;

  VERBOSE(0, ("+++++++++++++++++++++ [spu object %s] +++++++++++++++++++++"));

  /* use standard spu linker script
   * if you want to override this, do so via the description file
   * (to be implemented) */
  diabloobject_options.linkerscript_set = FALSE;

  obj = sd->obj = LinkEmulate(sd->name, FALSE);
  sd->handlesym =
    SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(ppu_obj), sd->handle);
  ASSERT(sd->handlesym, ("Could not find handle %s", sd->handle));

  ObjectDisassemble (obj);
  ObjectFlowgraph (obj, NULL, 0);
  cfg = sd->cfg = OBJECT_CFG(obj);

  CfgRemoveDeadCodeAndDataBlocks (cfg);
  CfgPatchToSingleEntryFunctions (cfg);
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  CfgRemoveUselessConditionalJumps (cfg);
  CfgRemoveEmptyBlocks (cfg);

  VERBOSE(0, ("object: %s", sd->name));
  VERBOSE(0, ("handle: %s", sd->handle));
  VERBOSE(0, ("> ready for rewriting"));
}

#define TEMP_SPU_NAME   "./diablo-temp-spu"
#define SYMBOL_PREFIX   "diablo-reembed-"
static void PutSpuBinary(t_spu_binary *sd, t_object *ppu_obj)
{
  static int invokecount = 0;
  t_cfg * cfg = sd->cfg;
  t_object *obj = sd->obj;
  t_section *sec;

  /* write spu binary to disk {{{ */
  ObjectDeflowgraph (obj);
  ObjectRebuildSectionsFromSubsections (obj);
  ObjectAssemble (obj);

  /* move symbols to parent sections */
  {
    t_symbol * symptr;
    for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj));
        symptr!=NULL;
        symptr=SYMBOL_NEXT(symptr))
    {
      if (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (symptr)) == RT_SUBSECTION)
      {
        SYMBOL_SET_OFFSET_FROM_START(symptr,
            AddressAdd(SYMBOL_OFFSET_FROM_START(symptr),
              AddressSub (
                SECTION_CADDRESS(T_SECTION(SYMBOL_BASE (symptr))),
                SECTION_CADDRESS(SECTION_PARENT_SECTION(
                    T_SECTION(SYMBOL_BASE (symptr)))))));
        SymbolSetBase(symptr,
            T_RELOCATABLE(SECTION_PARENT_SECTION(
                T_SECTION(SYMBOL_BASE(symptr)))));
      }
    }
    if (OBJECT_SYMBOL_TABLE(obj)) SymbolTableFree(OBJECT_SYMBOL_TABLE(obj));
    OBJECT_SET_SYMBOL_TABLE(obj, OBJECT_SUB_SYMBOL_TABLE(obj));
    OBJECT_SET_SUB_SYMBOL_TABLE(obj, NULL);
  }

  ObjectWrite(obj, TEMP_SPU_NAME);
  /* }}} */

  /* {{{ run ppu-embedspu */
  /* TODO make all of this crap configurable:
   *    - the actual name and location of ppu-embedspu
   *    - ppu-embedspu arguments (e.g. -m64)
   */
  char command[400];
  char embedname[100];
  sprintf(embedname, "%s-%d-embed64.o", TEMP_SPU_NAME, invokecount++);
  sprintf(command, "ppu-embedspu -m64 %s %s %s",
      sd->handle, TEMP_SPU_NAME, embedname);
  int sysret = system(command);
  ASSERT(sysret == 0, ("Failed to run ppu-embedspu"));
  /* }}} */

  LinkObjectFileNew(ppu_obj, embedname, SYMBOL_PREFIX, FALSE, FALSE, NULL);

  t_string newhandle = StringConcat2(SYMBOL_PREFIX, sd->handle);
  t_string newhsym = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(ppu_obj), newhandle);
  t_object *embed = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(newhsym)));
  t_section *origsec = T_SECTION(SYMBOL_BASE(sd->handlesym));
  t_object *origobj = SECTION_OBJECT(origsec);
  t_reloc_ref *rr;
  int i;

  while ((rr = SECTION_REFED_BY(origsec)))
  {
    t_reloc *rel = RELOC_REF_RELOC(rr);
    if (strcmp(RELOC_LABEL(rel), sd->handle))
      FATAL(("unexpected reloc @R", rel));
    if (RELOC_N_TO_RELOCATABLES(rel) > 1)
      FATAL(("unexpected reloc @R", rel));
    if (!AddressIsNull(RELOC_TO_RELOCATABLE_OFFSET(rel)[0]))
      FATAL(("unexpected reloc @R", rel));

    RelocSetToRelocatable(rel, 0, SYMBOL_BASE(newhsym));
  }

  OBJECT_FOREACH_SECTION(origobj, sec, i)
  {
    for (rr = SECTION_REFED_BY(sec); rr; rr = RELOC_REF_NEXT(rr))
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);
      t_relocatable *from = RELOC_FROM(rel);
      if (RELOCATABLE_RELOCATABLE_TYPE(from) != RT_SUBSECTION)
        FATAL(("unexpected relocation @R", rel));
      if (SECTION_OBJECT(T_SECTION(from)) != origobj)
        FATAL(("unexpected relocation @R", rel));
    }
    for (rr = SECTION_REFED_BY(sec); rr; rr = RELOC_REF_NEXT(rr))
    {
      t_reloc *rel = RELOC_REF_RELOC(rr);
      int j;
      for (j = 0; j < RELOC_N_TO_RELOCATABLES(rel); ++j)
      {
        t_relocatable *to = RELOC_TO_RELOCATABLE(rel)[j];
        if (RELOCATABLE_RELOCATABLE_TYPE(to) != RT_SUBSECTION)
          FATAL(("unexpected rel @R", rel));
        if (SECTION_OBJECT(T_SECTION(to)) != origobj)
          FATAL(("unexpected rel @R", rel));
      }
    }
    while ((rr = SECTION_REFERS_TO(sec)))
      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(ppu_obj), RELOC_REF_RELOC(rr));
    while ((rr = SECTION_REFED_BY(sec)))
      RelocTableRemoveReloc(OBJECT_RELOC_TABLE(ppu_obj), RELOC_REF_RELOC(rr));

    SECTION_SET_CSIZE(sec, AddressNewForObject(ppu_obj, 0));
  }
  
#if 0
  t_object *embed = ObjectGet(TEMP_SPU_NAME "-embed64.o", NULL, FALSE);
  t_section *sec;
  int i;
  OBJECT_FOREACH_SECTION(embed, sec, i)
    VERBOSE(0,("embed -> @T", sec));
  t_reloc *rel;
  OBJECT_FOREACH_RELOC(embed, rel)
    VERBOSE(0,("embed -> @R", rel));

  t_section *origsec = SYMBOL_BASE(sd->handlesym);
  t_object *origobj = SECTION_OBJECT(origsec);

  OBJECT_FOREACH_SECTION(origobj, sec, i)
  {
    VERBOSE(0, (">> @T", sec));
    t_reloc_ref *rr;
    for (rr = SECTION_REFERS_TO(sec); rr; rr = RELOC_REF_NEXT(rr))
      VERBOSE(0, (">>>REFTO @R", RELOC_REF_RELOC(rr)));
    for (rr = SECTION_REFED_BY(sec); rr; rr = RELOC_REF_NEXT(rr))
      VERBOSE(0, (">>>REFBY @R", RELOC_REF_RELOC(rr)));
  }
  exit(0);
#endif
}

int main (int argc, char **argv)
{
  t_object *obj, *tmp;
  t_cfg *cfg;
  int i;

  DiabloFlowgraphInit (argc, argv);
  DiabloPpc64Init (argc, argv);
  DiabloSpeInit (argc, argv);

  DiabloCellOptionsInit ();
  OptionParseCommandLine (diablo_cell_option_list, argc, argv, FALSE);
  OptionGetEnvironment (diablo_cell_option_list);
  DiabloCellOptionsVerify ();
  OptionDefaults (diablo_cell_option_list);

  t_program_description *desc = ParseProgramDescriptionFile(diablo_cell_options.program_description);

  /* set the ppu linker script 
   * if you want to override the linker script for the ppu or spu binaries,
   * do so via the description file (to be implemented) */
  diabloobject_options.linkerscript =
    StringDup(DATADIR "/linker_descriptions/ELF-cell-ppu-BINUTILS_LD.ld");
  diabloobject_options.linkerscript_set = TRUE;

  obj = desc->obj = LinkEmulate(desc->ppu_name, FALSE);
  RelocTableRemoveConstantRelocs(OBJECT_RELOC_TABLE(obj));
  ObjectDisassemble(obj);

  ObjectFlowgraph(obj, NULL, NULL);
  cfg = desc->cfg = OBJECT_CFG(obj);

  for (i = 0; i < desc->nspu; ++i)
    GetSpuBinary(&desc->spu[i], obj);

  /* prepare ppu binary for rewriting */
  CfgRemoveDeadCodeAndDataBlocks (cfg);
  CfgPatchToSingleEntryFunctions (cfg);
  CfgRemoveDeadCodeAndDataBlocks (cfg);

  /* TODO
   *   - connect
   *   - rewrite
   *   - write spu binaries
   *   - call ppu-embedspu
   *   - replace embedded spu binaries
   */

  /* TODO insert global rewriting code here */

  /* write ppc64 binary */
  ObjectDeflowgraph(obj);
  ObjectDumpDisassembledCode(obj, "./b.out.list");
  ObjectAssemble(obj);

  /* re-embed spu binaries */
  for (i = 0; i < desc->nspu; ++i)
    PutSpuBinary(&desc->spu[i], obj);

  ObjectPlaceSections(obj, FALSE, TRUE, TRUE);

  RelocateCore(obj, FALSE);

  ObjectRebuildSectionsFromSubsections(obj);


  {
    t_symbol * symptr;

    OBJECT_FOREACH_SYMBOL_SAFE(obj, symptr, tmp)
    {
        if (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (symptr)) == RT_SUBSECTION)
        {
            SYMBOL_SET_OFFSET_FROM_START(symptr, AddressAdd(SYMBOL_OFFSET_FROM_START(symptr),
                        AddressSub (SECTION_CADDRESS(T_SECTION(SYMBOL_BASE (symptr))),
                        SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))))));
            SymbolSetBase(symptr, T_RELOCATABLE(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))));
        }

        /* Removing symbols we added in Diablo for internal purposes. */
        if ((SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_CODE)
                || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_DATA)
                || (SYMBOL_FLAGS(sym) & SYMBOL_TYPE_MARK_THUMB)
                || strcmp("$compiler", SYMBOL_NAME(sym)) == 0
                || strcmp("$switch", SYMBOL_NAME(sym)) == 0)
            SymbolTableRemoveSymbol (OBJECT_SYMBOL_TABLE(obj), sym);
    }

    printf("+++++++++++++++++++++++++++++++++++++\n");
    if (OBJECT_SYMBOL_TABLE(obj)) SymbolTableFree(OBJECT_SYMBOL_TABLE(obj));
    OBJECT_SET_SYMBOL_TABLE(obj, OBJECT_SUB_SYMBOL_TABLE(obj));
    OBJECT_SET_SUB_SYMBOL_TABLE(obj, NULL);
  }

  /* set entry point to first entry of the .opd section.
   * In practice, this works, but in theory this is not 
   * guaranteed to be correct. XXX be aware of this when
   * you encounter problems with weirdly misplaced entry
   * points! */
  {
    t_section *sec = SectionGetFromObjectByName(obj, ".opd");
    ASSERT(sec, ("Could not find the .opd section"));

    OBJECT_SET_ENTRY(obj, SECTION_CADDRESS(sec));
  }

  ObjectWrite (obj, "./b.out");
  chmod ("./b.out",
      S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
      S_IXOTH);

  Free(desc);

  DiabloSpeFini ();
  DiabloPpc64Fini ();
  DiabloFlowgraphFini ();

  PrintRemainingBlocks ();

  return 0;
}

/* vim:set ts=2 sw=2 tw=80 foldmethod=marker expandtab: */
