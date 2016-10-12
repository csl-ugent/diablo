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

/* TODO this is code that does not really have anything to do with flowgraphing,
 * so maybe it should be put somewhere else */

/* Ppc64MarkGlinkAsData {{{ */
void Ppc64MarkGlinkAsData(t_object *obj)
{
  t_symbol *sym;
  t_symbol *last;
  t_address last_address;

  /* mark the glink stub and the glink entries as data, so that we are sure they
   * are not modified. The glink mechanism is complex, and implicitly relies on
   * a certain code ordering (i.e., the entries should all be preserved and
   * placed sequentially right behind the glink stub, and the stub must be
   * exactly 0x40 bytes long). Therefore, it is best to shield it completely
   * from Diablo instead of working around it in each and every optimization */

  if (strcmp("ppc64", OBJECT_OBJECT_HANDLER(obj)->sub_name))
    return;
  
  last = NULL;

  /* find the last __glink_stub:* symbol */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym)
  {
    if (StringPatternMatch("__glink_stub:*",SYMBOL_NAME(sym)))
    {
      t_relocatable *base = SYMBOL_BASE(sym);

      if (RELOCATABLE_RELOCATABLE_TYPE(base) != RT_SUBSECTION)
        FATAL(("expect symbol to point to subsection: @S", sym));
      if (!AddressIsNull(SYMBOL_OFFSET_FROM_START(sym)))
        FATAL(("expect symbol to point to start of subsection: @S", sym));

      if (last)
      {
        t_address a = RELOCATABLE_CADDRESS(SYMBOL_BASE(sym));
        if (AddressIsGt(a, last_address))
        {
          last = sym;
          last_address = a;
        }
      }
      else
      {
        last = sym;
        last_address = RELOCATABLE_CADDRESS(SYMBOL_BASE(sym));
      }
    }
  }

  if (last)
  {
    /* add a new symbol to the end of the last glink entry, so we have an end
     * point for the dataization */
    sym = SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(obj),
                         "ppc64diablo:glink_end", "R00$",
                         SYMBOL_ORDER(last), NO, NO, SYMBOL_BASE(last),
                         AddressNewForObject(obj,8), AddressNullForObject(obj),
                         NULL, AddressNullForObject(obj), 0);
    MarkRegionAsData(obj, "__glink", "ppc64diablo:glink_end");
  }
}
/* }}} */

/* Ppc64FindBBLLeaders {{{ */
#define DEBUG_BBL_LEADERS      0
#define DEBUG_BBL_LEADERS_FULL 0

#define PPC_INS_SET_BBL_LEADER(i)                                             \
  do {                                                                        \
    PPC_INS_SET_ATTRIB ((i), PPC_INS_ATTRIB ((i)) | IF_BBL_LEADER);           \
    nleaders++;                                                               \
  } while (0)

#define PPC_INS_IS_BBL_LEADER(i)                                              \
  (PPC_INS_ATTRIB ((i)) & IF_BBL_LEADER)


/* Ppc64SplitOpd {{{ */
/* !Split .opd section
 *
 * Linux/ppc64 creates one large section with procedure descriptors for most
 * (all?) functions in the object file. Most references to functions, be it
 * for branching or for loading their address, occur via these .opd sections.
 * To enable Diablo to detect which procedure descriptors (and consequently
 * which functions) are actually used and which aren't, split these .opd
 * sections into a separate subsecton for each .opd entry.
 *
 * As a special case, the subsection containing the .opd entry for .start
 * is always marked with SECTION_FLAG_KEEP, since it is not referenced
 * by any code, but only by the entry point field in the ELF header.
 *
 * \param secopd: the .opd section of the object
 * \param entrypointopd: the address of the entry point .opd entry
 * 
 * \return void 
 *
 */
static int __helper_sort_rr(const void *a, const void *b)
{
  t_reloc_ref *rra = *(t_reloc_ref **)a;
  t_reloc_ref *rrb = *(t_reloc_ref **)b;

  if (AddressIsLe(RELOC_FROM_OFFSET(RELOC_REF_RELOC(rra)),
                  RELOC_FROM_OFFSET(RELOC_REF_RELOC(rrb))))
      return -1;
  else return 1;
}
static void sort_reloc_refs(t_section *sec)
{
  t_reloc_ref *rr, **refarr;
  int nrefs = 0, i = 0;
  for (rr = SECTION_REFERS_TO(sec); rr; rr = RELOC_REF_NEXT(rr))
    ++nrefs;
  refarr = Malloc(sizeof(t_reloc_ref *)*nrefs);
  for (rr = SECTION_REFERS_TO(sec); rr; rr = RELOC_REF_NEXT(rr))
    refarr[i++] = rr;
  qsort(refarr, nrefs, sizeof(t_reloc_ref *), __helper_sort_rr);
  for (i = 1; i < nrefs; ++i)
  {
    RELOC_REF_SET_PREV(refarr[i], refarr[i-1]);
    RELOC_REF_SET_NEXT(refarr[i-1], refarr[i]);
  }
  RELOC_REF_SET_PREV(refarr[0], NULL);
  RELOC_REF_SET_NEXT(refarr[nrefs-1], NULL);

  SECTION_SET_REFERS_TO(sec, refarr[0]);
}

void Ppc64SplitOpd(t_section *secopd, t_address entrypointopd)
{
  int nentries = 0;
  int maxentries;
  t_reloc_ref *rr;
  t_section *sub, *tmp;
  t_section **entrysubs;
  t_object *obj = SECTION_OBJECT(secopd);
  t_symbol *sym;

  t_section *firstnew = NULL;

  /* An opd entry is either 2 or 3 64-bit fields.
   * The correct size is typically 24 bytes, but the Cell SDK
   * contains shorter opd entries that only have the first two
   * fields. Anyway, the third field in an opd entry should 
   * always be unrelocated, so we can derive the opd entry format
   * from the presence of a relocation in the third field */

  maxentries = AddressExtractUint32(SECTION_CSIZE(secopd)) / 16;
  entrysubs = Malloc(sizeof(t_section *) * maxentries);
  nentries = 0;
  
  /* {{{ identify and split off the individual entries */
  SECTION_FOREACH_SUBSECTION(secopd,sub)
  {
    if (sub == firstnew) break;

    VERBOSE(1,("processing @T size @G", sub, SECTION_CSIZE(sub)));
    sort_reloc_refs(sub);
    rr = SECTION_REFERS_TO(sub);
    while (rr)
    {
      t_uint32 offset, nextoffset, nextnextoffset;
      t_reloc_ref *next = RELOC_REF_NEXT(rr);
      t_reloc_ref *nextnext;
      t_uint32 size;
      t_section *newsub;

      ASSERT(next, ("opd reloc refs should come in pairs"));
      nextnext = RELOC_REF_NEXT(next);

      offset = AddressExtractUint32(RELOC_FROM_OFFSET(RELOC_REF_RELOC(rr)));
      nextoffset = AddressExtractUint32(RELOC_FROM_OFFSET(RELOC_REF_RELOC(next)));
      if (nextnext)
        nextnextoffset = AddressExtractUint32(RELOC_FROM_OFFSET(RELOC_REF_RELOC(nextnext)));
      else
        nextnextoffset = AddressExtractUint32(SECTION_CSIZE(sub));

      ASSERT(offset+8 == nextoffset, ("opd relocs should come in pairs"));

      size = nextnextoffset - offset;

      VERBOSE(1, ("entry: offset = %d size = %d", offset, size));

      /* create the new subsection */
      newsub =
        SectionCreateForObject(SECTION_OBJECT(sub), SECTION_TYPE(sub), secopd,
                               AddressNew64(size), ".opds");
      SECTION_SET_OLD_ADDRESS(newsub, AddressAddUint32(SECTION_OLD_ADDRESS(sub), offset));
      SECTION_SET_CADDRESS(newsub, AddressAddUint32(SECTION_CADDRESS(sub), offset));
      SECTION_SET_ALIGNMENT(newsub, AddressNew64(8));
      memcpy(SECTION_DATA(newsub),
             ((char *)SECTION_DATA(sub))+offset, size);

      if (AddressIsEq(entrypointopd, SECTION_OLD_ADDRESS(newsub)))
        SECTION_SET_FLAGS(newsub, SECTION_FLAG_KEEP);

      if (!firstnew)
        firstnew = newsub;

      /* move the from_relocs */
      RelocSetFrom (RELOC_REF_RELOC(rr), T_RELOCATABLE(newsub));
      RelocSetFrom (RELOC_REF_RELOC(next), T_RELOCATABLE(newsub));
      RELOC_SET_FROM_OFFSET(RELOC_REF_RELOC(rr), AddressNew64(0));
      RELOC_SET_FROM_OFFSET(RELOC_REF_RELOC(next), AddressNew64(8));

      /* move the to_relocs */
      {
        t_reloc_ref *refby, *safe;
        for (refby = SECTION_REFED_BY(sub), safe = refby ? RELOC_REF_NEXT(refby) : NULL;
             refby;
             refby = safe, safe = refby ? RELOC_REF_NEXT(refby) : NULL)
        {
          t_reloc *rel = RELOC_REF_RELOC(refby);
          t_uint32 to_off;
          
          ASSERT(RELOC_N_TO_RELOCATABLES(rel) == 1, ("implement complex relocs"));
          to_off = AddressExtractUint32(RELOC_TO_RELOCATABLE_OFFSET(rel)[0]);
          if (to_off >= offset && to_off < (offset+size))
          {
            RelocSetToRelocatable(rel, 0, T_RELOCATABLE(newsub));
            RELOC_TO_RELOCATABLE_OFFSET(rel)[0] = AddressSubUint32(RELOC_TO_RELOCATABLE_OFFSET(rel)[0], offset);
            VERBOSE(1,("migrated rel @R", rel));
          }
        }
      }

      entrysubs[nentries++] = newsub;
      rr = nextnext;
    }
  }
  /* }}} */

  /* {{{ move the symbols */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym)
  {
    if (SYMBOL_BASE(sym) &&
        RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION &&
        SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE(sym))) == secopd)
    {
      int i;
      t_address address = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);

      for (i = 0; i < nentries; ++i)
      {
        sub = entrysubs[i];
        if (AddressIsLt(AddressSub(address, SECTION_CADDRESS(sub)), SECTION_CSIZE(sub)))
          break;
      }
      ASSERT(i < nentries,
             ("Could not find subsection for symbol address @G in section .opd: @G->@G",
              address, SECTION_CADDRESS(secopd), AddressAdd(SECTION_CADDRESS(secopd), SECTION_CSIZE(secopd)))); 

      SymbolSetBase(sym, T_RELOCATABLE(sub));
      SYMBOL_SET_OFFSET_FROM_START(sym, AddressSub (address, SECTION_CADDRESS(sub)));
    }
  }
  /* }}} */

  /* {{{ Kill the original subsections */
  SECTION_FOREACH_SUBSECTION_SAFE(secopd, sub, tmp)
  {
    if (!SECTION_REFERS_TO(sub))
      SectionKill(sub);
  }
  /* }}} */

  Free(entrysubs);
}
/* }}} */


/* Ppc64LinuxSetEntryPoint {{{ */
/*! Properly set the entry point
 *
 * On Linux/ppc64, the original entry point address points to the function
 * description entry for _start inside the .opd section. Diablo expects an
 * address to an actual instruction, so get the address from the .opd section
 * and adjust the entry point.
 *
 * \param obj The object we are currently processing
 *
 * \return void 
 */
void
Ppc64LinuxSetEntryPoint(t_object *obj, t_section **secopd, t_address_generic *entryopd)
{
  t_address_generic entry;
  t_section *opd;
  t_symbol * sym;

  STATUS (START, ("Patch the entry point")); 
  sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE (obj), "_start");
  ASSERT (sym && !SYMBOL_EQUAL (sym), ("Couldn't find symbol '_start' for entry point"));

  ASSERT (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (sym)) == RT_SECTION || RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (sym)) == RT_SUBSECTION, ("'_start' is not inside a section!?"));
  opd = T_SECTION (SYMBOL_BASE (sym));
  /* opd is the subsection containing that symbol */
  *secopd=SECTION_PARENT_SECTION(opd);
  ASSERT (!StringCmp (SECTION_NAME (opd), ".opd"), ("'_start' isn't inside section '.opd'!?"));
  *entryopd = OBJECT_ENTRY (obj);
  DEBUG (("Current entry descriptor is @G", *entryopd));
  entry = AddressNew64 (((t_uint64*)(SECTION_DATA (opd) +
                                     AddressExtractUint64 (AddressSub (SECTION_CADDRESS (opd), *entryopd))))[0]);
  OBJECT_SET_ENTRY (obj, entry);
  DEBUG (("Real entry is @G", entry));

  STATUS (STOP, ("Patch the entry point"));
}
/* }}} */


/* Ppc64LinuxProcessOpd {{{ */
/*! Set the real entry point address and split the .opd section
 *
 * See the descriptions of Ppc64LinuxSetEntryPoint and
 * Ppc64SplitOpd for what is done here
 *
 * \param obj The object we are currently processing
 *
 * \return void 
 */
void Ppc64LinuxProcessOpd(t_object *obj)
{
  t_section *opd;
  t_address_generic entryopd;

  if (strcmp("ppc64", OBJECT_OBJECT_HANDLER(obj)->sub_name))
    return;

  Ppc64LinuxSetEntryPoint(obj,&opd,&entryopd);
  Ppc64SplitOpd(opd,entryopd);
}


/*! Basic leader-detection algorithm. 
 *
 * See e.g. the Red Dragon Book ("Compilers: Principles, Techniques and
 * Tools", by Alfred V. Aho, Ravi Sethi, and Jeffrey D. Ullman) for more
 * details. 
 *
 * The aim of this function is to identify all start-addresses of basic
 * blocks (and mark them). There are four reasons to mark an instructions as
 * a basic block leader:
 * 
 * 1. The instruction is the target of a direct or indirect jump. This is not
 * as easy as it seems, especially when the program counter is explicit:
 * every instruction that changes the pc can be considered as a jump
 * instruction (but this is not our case in ppc64).
 *
 * 2. It's the successor of a control-flow altering instruction (if a jump is
 * conditional, than the next instruction will be the first instruction of the
 * fall-through block, else it is either target of an other jump or a dead
 * block. In the latter case, it will be removed by the unreachable block
 * removal routines, so we just mark it as a basic block leader).
 *
 * 3. There is an address produced to this basic block, this is either 
 *
 *    a. done directly, using the program counter (we assume no real
 *    code-address calculations are present (so not address of function + x) 
 *
 *    b. using a relocation (and not necessary detected when scanning the
 *    instructions) 
 *
 * For both cases we need to assume there will be an indirect jump to this
 * address.
 *
 * 4. The start of data blocks in code or instructions following datablocks
 * are considered basic block leaders.
 *
 *
 * This function will find all basic block leaders that are not part of a
 * switch statement. More switch checking will be done in optimize switches.
 * 
 * \param code The section to flowgraph.
 *
 * \return t_uint32 The number of detected BBL leaders.
 */
static t_uint32 Ppc64FindBBLLeaders(t_object *obj)
{
  /* We can still assume that all instructions are stored sequentially in the
   * array Ains. The last instruction stored can be recognised with INSNEXT ==
   * NULL */
  t_uint32 nleaders = 0;
  t_section *code;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, code, i)
  {
    t_address target_address;
    t_ppc_ins *target;
    t_ppc_ins *ins;

    /* Initialisation: The first instruction of the program is always a leader.
     * The reasoning is equal to that of reason 2: Either there will be a jump to
     * this address or it's the entry point or it is dead. */
    PPC_INS_SET_BBL_LEADER (T_PPC_INS(SECTION_DATA (code)));
#if DEBUG_BBL_LEADERS
    DEBUG(("BBLL: first instruction: @I", T_PPC_INS(SECTION_DATA (code))));
#endif

    /* Look for BBL_LEADERs in code {{{*/
    SECTION_FOREACH_PPC_INS (code, ins)
    {
#if DEBUG_BBL_LEADERS_FULL
      DEBUG(("BBL Leader? @I", ins));
#endif

      /* (reason 1) Switch table entry {{{ */
      if (PPC_INS_TYPE (ins) == IT_DATA && (PPC_INS_ATTRIB (ins) & IF_SWITCHTABLE))
      {
        t_ppc_ins *tablebase = ins;
        t_ppc_ins *target;

        /* \TODO: is it necessary to check for pattern on previous instructions?
         *            mtctr rx
         *            bctr
         *            <switch_table>
         */

#if DEBUG_BBL_LEADERS
        DEBUG(("Switch table: @I", ins));
#endif
        while (PPC_INS_TYPE (ins) == IT_DATA && (PPC_INS_ATTRIB (ins) & IF_SWITCHTABLE))
        {
          target = T_PPC_INS(ObjectGetInsByAddress(obj, AddressAdd (PPC_INS_CADDRESS(tablebase), PPC_INS_IMMEDIATE(ins))));
          ASSERT (target, ("Couldn't find the target instruction at @G for a switch table at @G (@G)",
                           AddressAdd (PPC_INS_CADDRESS(tablebase), PPC_INS_IMMEDIATE(ins)),
                           PPC_INS_CADDRESS(ins), PPC_INS_IMMEDIATE(ins)));
          PPC_INS_SET_BBL_LEADER (target);
#if DEBUG_BBL_LEADERS
          DEBUG(("BBLL: switch case: @I (by @I)", target, ins));
#endif
          ins = PPC_INS_INEXT (ins);
        }
        /* This is for sure the first case of the switch table. */
        PPC_INS_SET_BBL_LEADER (ins);
#if DEBUG_BBL_LEADERS
        DEBUG(("BBLL: switch first case: @I", ins));
#endif

        /* step back one instruction, so that the first instruction after the
         * switch table will be processed as well */
        ins = PPC_INS_IPREV(ins);
        continue;
      } /* }}} */
      /* (reason 4) Data intermixed in code {{{ */
      else if (PPC_INS_TYPE (ins) == IT_DATA)
      {
        t_ppc_ins *iprev = NULL;

        /* The start of a block of data is considered a basic block leader */
        PPC_INS_SET_BBL_LEADER (ins);
#if DEBUG_BBL_LEADERS
        DEBUG(("BBLL: data: @I", ins));
#endif

        while (ins && PPC_INS_TYPE (ins) == IT_DATA)
        {
          /* Skip all the other data */
          iprev = ins;
          ins = PPC_INS_INEXT (ins);
        }

        /* The instruction following the data is a basic block leader */
        if (ins) 
        {
          PPC_INS_SET_BBL_LEADER (ins);
#if DEBUG_BBL_LEADERS
          DEBUG(("BBLL: next to data: @I", ins));
#endif
          /* Re-process this instruction */
          ins = iprev;
        }
        else
        {
          break;
        }
      } /* }}} */
      /* (reason 1 & 2) Branches {{{ */
      else if (PPC_INS_TYPE (ins) == IT_BRANCH)
      {
        if (PPC_INS_INEXT(ins) != NULL)
        {
          PPC_INS_SET_BBL_LEADER (PPC_INS_INEXT (ins));
#if DEBUG_BBL_LEADERS
          DEBUG(("BBLL: after a branch: @I (by @I)", PPC_INS_INEXT (ins), ins));
#endif
        }

        /* The target of the branch will also be a basic block leader */
        if (PPC_INS_OPCODE(ins) == PPC_B || PPC_INS_OPCODE(ins) == PPC_BC)
        {
          /* Compute the target address */ 
          if (PpcInsAbsoluteAddress (ins))
          {
            target_address = PPC_INS_IMMEDIATE(ins);
          }
          else
          {
            target_address = AddressAdd (PPC_INS_CADDRESS(ins), PPC_INS_IMMEDIATE(ins));
          }

          /* Get instruction target */
          target = T_PPC_INS(ObjectGetInsByAddress(obj, target_address));

          if (target)
          {
            PPC_INS_SET_BBL_LEADER (target);
#if DEBUG_BBL_LEADERS
            DEBUG(("BBLL: branch: @I (by @I)", target, ins));
#endif
          }
          else
          {
            FATAL(("Unhandled branch flowgraphing: @I -> @G", ins, target_address));
          }
        }
        else if (PPC_INS_OPCODE(ins) != PPC_BCCTR && PPC_INS_OPCODE(ins) != PPC_BCLR)
        {
          FATAL(("Unknown Branch type: %d", PPC_INS_OPCODE(ins)));
        }
      } /*}}}*/
      /* (reason 2) System Calls and Traps {{{*/
      else if (PPC_INS_TYPE (ins) == IT_SWI)
      {
        /* Actually, we could just consider system calls as normal
         * instructions, if we were sure that they are not long-jump-like
         * system calls. Since we do not know that for sure here, we must
         * assume that this will alter control flow => next instruction is a
         * leader */

        if (PPC_INS_INEXT(ins) != NULL) 
        {
          PPC_INS_SET_BBL_LEADER (PPC_INS_INEXT (ins));
#if DEBUG_BBL_LEADERS
          DEBUG(("BBLL: after SWI: @I", PPC_INS_INEXT (ins)));
#endif
        }
      } /*}}}*/
      /* (reason 3) Relocations (no "else if", because can be in combination 
       *  with other case, e.g. an empty procedure which only consists of blr 
       *  -> is a branch, but probably also referred to by a relocation {{{ */
      if (PPC_INS_REFED_BY(ins))
      {
        PPC_INS_SET_BBL_LEADER (ins);
#if DEBUG_BBL_LEADERS
        DEBUG(("BBLL: refed by a relocation: @I", ins));
#endif
      }
      /*}}}*/
#if DEBUG_BBL_LEADERS_FULL
      else
        DEBUG(("Not a BBL Leader: @I", ins));
#endif
    }
    /*}}}*/
  }

  return nleaders;
}
/* }}} */


#define VERBOSE_EDGES          0 //control all the verboses for this function
#if VERBOSE_EDGES
#define VERBOSE_SWITCH         1 //for switch pattern dectection verbose mode
#define VERBOSE_NORMAL         1 //for normal edge verbose mode
#define VERBOSE_PATTERNS       1 //for unknown control flow verbose mode (usefull to detect patterns)
#endif

/* Ppc64IsSwitchJumpBlock {{{ */
/*! Check whether an instruction is the start of a switch table
*
* \param ft_ins  The instruction to be checked
* \param jmp_bbl If a switch table is found, jmp_bbl will return
*                the block which ends with the indirect jump that
*                uses the table
*
* \return Whether or not ft_ins is the start of a switch block
*/
static t_bool
Ppc64IsSwitchTableBlock (t_ppc_ins *ft_ins, t_bbl **jmp_bbl)
{
/* {{{ */
  t_ppc_ins *jtbranch, *prev_controltransfer, *ins;
  t_address_generic switchaddr;

  t_reg jumpreg, condreg;
  t_address_generic bound, tablebase;
  t_ppc_ins *cmpli, *mtspr, *def, *tabledef, *iter;
  t_regset changed; 
  t_bool found_address;
  
  /* switch constructs have the following pattern:
    *   cmpli $cr, %reg, %bound
    *   ...
    *   bc $cr <default_case>
    *   ...
    *   lis  <jumptable_base calculation>
    *   addi <jumptable_base calculation>
    *   ...
    *   rlwinm ... 2,0,29 <offset_in_table calculation>
    *   lwzx <read table value>
    *   ...
    *   add  <jump address calculation> 
    *   ...
    *   mtcrt %reg
    *   bctr 
    * $switchtable:
    *   $target1-$switchtable
    *   $target2-$switchtable
    *   ...
    */

  switchaddr = PPC_INS_CADDRESS(ft_ins);
  /* check whether the bctr precedes the table */
  jtbranch = PPC_INS_IPREV(ft_ins);
  if (!jtbranch ||
      (PPC_INS_OPCODE(jtbranch) != PPC_BCCTR) ||
      PpcInsIsConditional(jtbranch))
    return FALSE;

  /* now look for the BC */
  for (ins = PPC_INS_IPREV(jtbranch); ins && !PpcInsIsControlTransfer(T_INS(ins)); ins = PPC_INS_IPREV(ins)) {
  }
  if (!ins ||
      (PPC_INS_OPCODE(ins) != PPC_BC) ||
      !PPC_IS_CBGT(ins))
    return FALSE;
  prev_controltransfer = ins;

#if VERBOSE_SWITCH
  VERBOSE(0,("Control transfer is conditional"));
  VERBOSE(0,("Previous control transfer is: @I",prev_controltransfer));
  VERBOSE(0,("Previous control transfer is : bgt")); 
  VERBOSE(0,("Candidate found!"));
#endif 

  /* look for the mtspr instruction */
  for (iter = PPC_INS_IPREV(jtbranch); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
  {
    if((PPC_INS_OPCODE(iter)==PPC_MTSPR) &&
       RegsetIn(PPC_INS_REGS_DEF(iter),PPC_REG_CTR) &&
       (PPC_INS_OPCODE(jtbranch)==PPC_BCCTR))
    {
      mtspr = iter;
      break;
    }
  }
  if(iter == prev_controltransfer)
    return FALSE;

#if VERBOSE_SWITCH
  VERBOSE(0,("mtspr found: @I",mtspr));
#endif 

  /* retrieve register used */
  jumpreg = PPC_INS_REGA(mtspr);

#if VERBOSE_SWITCH
  VERBOSE(0,("register used: %s",PpcRegisterName(jumpreg)));
#endif 

  /* one case for calculation:
   *  - offset table: ldx <jumptable_base> + offset_in_table
   add <jumptable_base> + offset loaded from table
   *
   * Look for the <jumptable_base>
   */
  for (iter = PPC_INS_IPREV(jtbranch); iter != prev_controltransfer; iter = PPC_INS_IPREV(iter))
  {
    if(((PPC_INS_OPCODE(iter)==PPC_ADD)||(PPC_INS_OPCODE(iter)==PPC_LDX)) &&
       RegsetIn(PPC_INS_REGS_DEF(iter),jumpreg))
    {
      def = iter;
      break;
    }
  }
  if (iter == prev_controltransfer)
    return FALSE;

#if VERBOSE_SWITCH
  VERBOSE(0,("Definition found: @I",def));
  VERBOSE(0,("table base could be: %s or %s",PpcRegisterName(PPC_INS_REGA(def)),PpcRegisterName(PPC_INS_REGB(def)))); 
#endif 
  /* Both instructions have 2 source registers :
   * REGA or REGB. Both could contain the jumptable
   * base value. Calculate both and choose the one
   * that could be known.
   */
  tabledef = def;

#if VERBOSE_SWITCH
  VERBOSE(0,("Trying to compute: %s",PpcRegisterName(PPC_INS_REGA(def)))); 
#endif 
  tablebase = PpcComputeRelocatedAddressValue(&tabledef,PPC_INS_REGA(def),&found_address);

  if(!found_address)
  {
    tabledef = def;
#if VERBOSE_SWITCH
    VERBOSE(0,("Trying to compute: %s",PpcRegisterName(PPC_INS_REGB(def)))); 
#endif 
    tablebase = PpcComputeRelocatedAddressValue(&tabledef,PPC_INS_REGB(def),&found_address);
  }
  if(!found_address)
    return FALSE;

  /* It should refer to the start of the table
  */
  if (!AddressIsEq(tablebase, switchaddr))
    return FALSE;

  /* Get the condition register checked for the default case
   */
  condreg = PPC_COND_REG(prev_controltransfer);
  
#if VERBOSE_SWITCH
  VERBOSE(0,("Conditional register of jump: %s",PpcRegisterName(condreg))); 
#endif 
  
  /* Look for the compare intruction that defines the bounds  
    * of the table 
    */
  changed = NullRegs;
  for (cmpli = PPC_INS_IPREV(prev_controltransfer); cmpli; cmpli = PPC_INS_IPREV(cmpli))
  {
    
#if VERBOSE_SWITCH
    VERBOSE(0,("Looking Compare instructions: @I",cmpli)); 
#endif 
    
    if ((PPC_INS_OPCODE(cmpli) == PPC_CMPLI) && 
        (PPC_INS_REGT(cmpli)==condreg))	
    { 
      
#if VERBOSE_SWITCH
      VERBOSE(0,("Is cmpli and condred: %s",PpcRegisterName(PPC_INS_REGT(cmpli)))); 
#endif 
      break;
    }
    
    RegsetSetUnion(changed, PPC_INS_REGS_DEF(cmpli));
    
    if (PpcInsIsControlTransfer(T_INS(cmpli)) || RegsetIn(changed,condreg))
    {
#if VERBOSE_SWITCH
      VERBOSE(0,("Or is a control transfer or changes de condition register"));
#endif 
      cmpli = NULL;
      break;
    }
  }
  if (!cmpli)
    return FALSE;
  
#if VERBOSE_SWITCH
  VERBOSE(0,("Compare instruction: @I",cmpli));
#endif 
  
  /* Get jump table bounds from the compare intruction */ 
  bound = PPC_INS_IMMEDIATE(cmpli);
  
#if VERBOSE_SWITCH
  VERBOSE(0,("Switch bounds: @G",bound));
#endif
  /* Check of size of the table */
  if (AddressIsEq (AddressNewForIns(T_INS(cmpli),BBL_NINS(PPC_INS_BBL(ft_ins))-1), bound))
  {
    *jmp_bbl = PPC_INS_BBL(jtbranch);
    return TRUE;
  }
  else
    return FALSE;
/* }}} */
}
/* }}} */

/* Ppc64AddBasicBlockEdges {{{ */
/*! Add the edges to BBLs.
 *
 * Add edges between the newly created basic blocks. Both the linear (array)
 * representation of the instructions and the basic block representation are
 * available: you can access the (copy) basic block by using PPC_INS_BBL of a
 * leader in the linear representation.
 *
 * Although connecting the blocks isn't that hard, it could be difficult to
 * find out if a given jump instructions is a call or just a jump (without
 * return). Getting this completly safe is probably not what you want: if you
 * implement this completly safe, you'll probably end up with one big
 * function, which will degrade the performance of the rest of diablo or hell
 * nodes after each jump which will degrade the performance even further.
 *
 * On the other hand, if you get this wrong (e.g add a jump edge where there
 * should have been a call edge), then chances are that your flowgraph (and
 * thus the produced binary) is wrong (the successor block of the call block
 * will have no incoming (return) edge, and thus this block is dead). 
 *
 * The solution is to look at code that is produced by your compiler(s).
 * It'll probably have only a few possibilties to produce calls. If you
 * handle them as safe as possible, than you're off the hook. 
 *
 * \param section The disassembled section, with basic blocks already created
 *                we want to flowgraph.
 *
 * \return t_uint32 The number of created edges.
 */
static t_uint32 Ppc64AddBasicBlockEdges(t_object *obj)
{
  t_ppc_ins *block_start, *block_end, *ft_ins, *target;
  t_bbl *block, *ft_block;
  t_cfg *cfg = OBJECT_CFG(obj);
  t_reloc_ref *ref;
  t_address_generic target_address;
  t_uint32 nedges = 0;
  t_cfg_edge * edge;
  t_bool is_detected = FALSE;
  t_bbl *switchjmpblock;

  t_section *section;
  int i;

  OBJECT_FOREACH_CODE_SECTION(obj, section, i)
  {
    /* walk through the BBLs and add all possible successors to every node */
    SECTION_FOREACH_PPC_INS(section, block_start)
    {
      if (!PPC_INS_IS_BBL_LEADER (block_start)) continue;

      block = PPC_INS_BBL (block_start);

      /* Look for block end */
      {
        t_ppc_ins *iter;
        for (block_end = block_start, iter = PPC_INS_INEXT (block_start);
             iter && !PPC_INS_IS_BBL_LEADER (iter);
             iter = PPC_INS_INEXT (iter))
        {
          block_end = iter;
        }
      }

#if VERBOSE_EDGES
      DEBUG(("Checking edges for @ieB", block));
#endif

      ft_ins = PPC_INS_INEXT (block_end) ? PPC_INS_INEXT (block_end) : NULL;
      ft_block = ft_ins ? PPC_INS_BBL (ft_ins) : NULL;

#if VERBOSE_EDGES
      if (ft_block)
        DEBUG(("Fallthrough is @ieB", ft_block));
      else
        DEBUG(("Fallthrough is <NULL>"));
#endif

      /* block : the current block
       * block_{start,end} : point to the first/last instruction of the current
       *                     block
       * ft_block : the fallthrough block (if any)
       * ft_ins : the fallthrough instruction (if any)
       * target : will be the target instruction (if any) */

      /* 1. ADD EDGES FOR RELOCATIONS {{{
       * 
       * Each relocation gets its own edge. These edges will be automatically
       * deleted when the relocation is removed. There's no need to add an edge
       * for the obvious relocations (such as the relocations that are present for
       * the calculation of the offsets in intermodular jumps), since they are
       * completly described by the edge itself (the reason for the edge is that
       * there is a jump, not that there is a relocation) 
       */
      for (ref = BBL_REFED_BY (block); ref != NULL; ref = RELOC_REF_NEXT (ref))
      {
        t_bool add_edge = FALSE;

        if (PPC_INS_TYPE (block_start) != IT_DATA)
        {
          if (RELOC_HELL (RELOC_REF_RELOC (ref)))
          {
            add_edge = TRUE;
          }  
        }

        /* Skip the obvious relocations and reloctions to data */
        if (add_edge)
        {
          /* Some extra checks if the relocation is from an instruction */
          if (RELOCATABLE_RELOCATABLE_TYPE (RELOC_FROM (RELOC_REF_RELOC (ref))) == RT_INS)
          {
            if (RELOC_REF_NEXT (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref)))))
            {
              VERBOSE(1, ("1. @R", RELOC_REF_RELOC (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref))))));
              VERBOSE(1, ("2. @R", RELOC_REF_RELOC (RELOC_REF_NEXT (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref)))))));
              FATAL(("Multiple relocs from instruction?"));
            }
            if (RELOC_REF_RELOC (RELOCATABLE_REFERS_TO (RELOC_FROM (RELOC_REF_RELOC (ref)))) != RELOC_REF_RELOC (ref))
            {
              FATAL(("Relocation wrong"));
            }
          }

          t_cfg_edge *found = NULL;
          t_cfg_edge *i_edge;

          /* Find a call edge to this block that hasn't been fulfilled */
          BBL_FOREACH_PRED_EDGE (block, i_edge)
          {
            if (T_BBL (CFG_EDGE_HEAD (i_edge)) == CFG_HELL_NODE (cfg) && CFG_EDGE_CAT (i_edge) == ET_CALL)
            {
              found = i_edge;
              break;
            }
          }

          if (found)
          {
            /* The relocation has an associated edge */
            RELOC_SET_EDGE (RELOC_REF_RELOC (ref), found);
            CFG_EDGE_SET_REFCOUNT (found, CFG_EDGE_REFCOUNT (found) + 1);
            /* also increment the refcount of the corresponding return edge!!! */
            ASSERT (CFG_EDGE_CORR (found), ("Call edge @E does not have a corresponding edge!", found));
            CFG_EDGE_SET_REFCOUNT (CFG_EDGE_CORR(found), CFG_EDGE_REFCOUNT (CFG_EDGE_CORR (found)) + 1);
          }
          else
          {
            /* The relocation has no associated edge */
            RELOC_SET_EDGE(RELOC_REF_RELOC(ref), CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), block, NULL, NULL));
            nedges++;
          }

#if VERBOSE_NORMAL
          VERBOSE(1, ("Adding INS arrow from hell! (relocation @R, @B)", RELOC_REF_RELOC(ref), block));
#endif
        }
      }
      /* }}} */

      /* 2. ADD EDGES FOR "SPECIAL" PATTERNS {{{
       *
       * Here some architecture-dependent instruction patterns are detected and
       * translated into edges. */
      /* 2a. Switch tables {{{ */
      if (ft_ins &&
          (PPC_INS_ATTRIB (ft_ins) & IF_SWITCHTABLE) &&
          PPC_INS_IPREV (ft_ins) &&
          Ppc64IsSwitchTableBlock(ft_ins,&switchjmpblock))
      {
        /* Create the switch edges but keep the original switch table data (to
         * keep the associated symbols) */
        t_ppc_ins *iter, *switchbase;
        int tel;
        t_reloc_ref *ref;
        t_reloc *rel;

        switchbase = ft_ins;

        for (tel = 0, iter = PPC_INS_COPY(switchbase); iter && (PPC_INS_ATTRIB (iter) & IF_SWITCHTABLE); iter = PPC_INS_INEXT (iter), tel++)
        {
          target = T_PPC_INS (ObjectGetInsByAddress(obj, AddressAdd (PPC_INS_CADDRESS (switchbase), PPC_INS_IMMEDIATE (iter))));
          ASSERT (target, ("Couldn't find target of switch (@I + @I)", switchbase, iter));

          edge = CfgEdgeCreate (cfg, switchjmpblock, PPC_INS_BBL (target), ET_SWITCH);
          CFG_EDGE_SET_SWITCHVALUE (edge, tel);

          ref = PPC_INS_REFERS_TO (iter);
          if (ref)
          {
#if VERBOSE_SWITCH
            DEBUG(("Have relocation for switch @E", edge));
#endif
            rel = RELOC_REF_RELOC (ref);
            ASSERT ((RELOC_N_TO_RELOCATABLES (rel) == 2), ("Unexpected relocation in switch table: @R", rel));
#if VERBOSE_SWITCH
            DEBUG(("@I to @R", iter, rel));
#endif
            RELOC_SET_HELL (rel, FALSE);
            CFG_EDGE_SET_REL (edge, rel);
            RELOC_SET_SWITCH_EDGE (rel, edge);
          }
          else
          {
            DEBUG(("No relocation for switch @E", edge));
          }
#if VERBOSE_SWITCH
          VERBOSE(1, ("Adding switch edge @I -> @I (@G - %d) : @E", block_end, target, PPC_INS_IMMEDIATE (iter), tel, edge));
#endif
        }
      }
      /* }}} */
      /* }}} */

      /* 3. ADD NORMAL EDGES {{{
       *
       * Now determine the successors of the block and their types
       */
      switch (PPC_INS_TYPE(block_end))
      {
        case IT_BRANCH: /* {{{ */
          /* If the branch is conditional, add a fall-through edge */
          if (ft_block != NULL && PpcInsIsConditional (block_end))
          {
#if VERBOSE_NORMAL
            VERBOSE(1,("Adding a fallthrough edge from @B to @B",block,ft_block));
#endif
            CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
            nedges++;
          }

          switch (PPC_INS_OPCODE(block_end))
          {
            /* case PPC_B || PPC_BC: {{{ */
            case PPC_BC:
            case PPC_B:
              /* Compute the target address */ 
              if(PpcInsAbsoluteAddress (block_end))
              {
                /* Target address is absolute */
                target_address = PPC_INS_IMMEDIATE(block_end);
              }
              else
              {
                /* Target address is relative */
                target_address = AddressAdd (PPC_INS_CADDRESS(block_end),
                                             PPC_INS_IMMEDIATE(block_end));
              }
              /* Get instruction target */     
              target = T_PPC_INS(ObjectGetInsByAddress(obj, target_address));

              if (target)      
              {
                /* Add a jump edge to the target */

                /* If the instruction updates the link register we assume
                 * that is a function call, else is a jump.
                 */
                if (PpcInsUpdatesLinkRegister (block_end) && ft_block)
                {
#if VERBOSE_NORMAL
                  VERBOSE(1,("Adding call edge from @B to @B wirth fall: @B",block,PPC_INS_BBL(target),ft_block));
#endif
                  CfgEdgeCreateCall (cfg, block, PPC_INS_BBL(target), ft_block, NULL);
                }
                else
                {
#if VERBOSE_NORMAL
                  VERBOSE(1,("Adding jump edge from @B to @B",block,PPC_INS_BBL(target)));
#endif
                  CfgEdgeCreate (cfg, block, PPC_INS_BBL(target), ET_JUMP);
                }
                nedges++;
              }
              else
              {
                /* We don't know where is jumping to. Add and edge to HELL */

                /* Usually a jump to address 0x10000000 means a jump to an undefined
                 * symbol (WEAK symbol). So this path is never reached during
                 * execution */
                if (G_T_UINT64(target_address) != 0x10000000)
                {
                  FATAL(("Direct jump without target!!! Check the address calculation correctness.\
                         Why we get here?.\
                         We don't know where is jumping : address %x Ins:\
                         @I. ",target_address,block_end ));
                }
                nedges++;  
              }
              break; /* }}} */
            case PPC_BCCTR: /* {{{ */
              /* Check first if we have already detected this branch as a switch
               * branch or an indirect call branch */

              is_detected = FALSE;

              BBL_FOREACH_SUCC_EDGE (block, edge)
              {
                if (CFG_EDGE_CAT (edge) == ET_SWITCH || 
                    CFG_EDGE_CAT (edge) == ET_CALL   ||
                    CFG_EDGE_CAT (edge) == ET_JUMP)
                {
#if VERBOSE_NORMAL
                  VERBOSE(1,("Branch to CTR: Is already detected"));
#endif
                  is_detected = TRUE;
                  break;
                }
              }

              if(PpcInsUpdatesLinkRegister (block_end) && !is_detected && ft_block)
              {
#if VERBOSE_NORMAL
                VERBOSE(1,("Branch to CTR: Adding call edge to hell from @B and fallthrough @B", block,ft_block));
#endif
                CfgEdgeCreateCall(cfg, block,CFG_HELL_NODE(cfg) ,ft_block,NULL);
#if VERBOSE_PATTERNS
                VERBOSE(1,("Pattern candidate: @ieB",block));
#endif
                nedges++;
              }
              else if (!is_detected)
              {
#if VERBOSE_NORMAL
                VERBOSE(1,("Branch to CTR: Adding jump edge to hell from @B",block));
#endif
                CfgEdgeCreate (cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
#if VERBOSE_PATTERNS
                VERBOSE(1,("Pattern candidate: @ieB",block));
#endif
                nedges++;
              }
              break; /* }}} */
            case PPC_BCLR: /* {{{ */
              is_detected = FALSE;

              BBL_FOREACH_SUCC_EDGE (block, edge)
              {
                if (CFG_EDGE_CAT (edge) == ET_SWITCH || 
                    CFG_EDGE_CAT (edge) == ET_CALL   ||
                    CFG_EDGE_CAT (edge) == ET_JUMP)
                {
#if VERBOSE_NORMAL
                  VERBOSE(1,("Branch to LR: Is already detected"));
#endif
                  is_detected = TRUE;
                  break;
                }
              }

              if (PpcInsUpdatesLinkRegister (block_end) && !is_detected)
              {
                if (ft_block)
                {
#if VERBOSE_NORMAL
                  VERBOSE(1,("Branch to LR: Adding call edge to hell from @B and fallthrough @B", block, ft_block));
#endif
                  CfgEdgeCreateCall (cfg, block, CFG_HELL_NODE(cfg), ft_block, NULL);
#if VERBOSE_PATTERNS
                  VERBOSE(1,("Pattern candidate: @ieB", block));
#endif
                }
                else
                {
#if VERBOSE_NORMAL
                  VERBOSE(0,("Branch to LR: Adding jump edge to hell!"));
#endif
                  CfgEdgeCreate (cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
#if VERBOSE_PATTERNS
                  VERBOSE(0,("Pattern candidate: @ieB",block));
#endif
                }
              }
              else if (!is_detected)
              {
#if VERBOSE_NORMAL
                VERBOSE(0,("Branch to LR: Adding return edge to hell from @B",block));
#endif
                CfgEdgeCreate (cfg, block, CFG_EXIT_HELL_NODE(cfg), ET_RETURN);
              }
              nedges++;
              break; /* }}} */
            default:
              FATAL((" %s : Unknown Branch type.", __func__ ));
              break;
          }
          break; /* }}} */
        case IT_SWI: /* {{{ */
          /* If the trap is conditional, add a fall-through edge */
          if (ft_block != NULL && PpcInsIsConditional (block_end))
          {
            CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
            nedges++;
#if VERBOSE_NORMAL
            VERBOSE(1,("SWI: Adding fallthrought, ins is conditional from @B to @B",block, ft_block));
#endif
          }
          /* since we don't know where a SWI jumps to, we'll add hell as a successor */
          CfgEdgeCreateSwi (cfg, block, ft_block);
#if VERBOSE_NORMAL
          VERBOSE(1,("SWI : Adding SWI edge from @B", block));
#endif
          nedges++;
          break; /* }}} */
        default:
          /* If there is a next block, add a fall-through edge, except to
	   * data blocks.
	   */
          if (ft_block != NULL &&
              PPC_INS_OPCODE (block_end) != PPC_DATA)
	  {
	    if (!IS_DATABBL (ft_block))
	    {
              CfgEdgeCreate (cfg, block, ft_block, ET_FALLTHROUGH);
#if VERBOSE_NORMAL
              VERBOSE(1,("Adding fallthrough from @B to @B", block, ft_block));
#endif
              nedges++;
	    }
	    /* One exception though: opcode 0 is used as "invalid instruction" by some
	     * glibc code, so if we have a single "0" data entry then assume that it's
	     * a reachable instruction that goes to hell.
             */
	    else if ((BBL_NINS (ft_block) == 1) &&
	             (INS_TYPE (BBL_INS_FIRST (ft_block)) == IT_DATA) &&
		     (AddressExtractUint32 (PPC_INS_IMMEDIATE (T_PPC_INS (BBL_INS_FIRST (ft_block)))) == 0))
	    {
	      CfgEdgeCreate(cfg, block, CFG_HELL_NODE(cfg), ET_JUMP);
	      nedges++;
	    }
          }
          break;
      }
      /* }}} */

#if VERBOSE_EDGES
      DEBUG(("Block is now @eB", block));
#endif
    }
  }

  return nedges;

}
/* }}} */

/* Ppc64Flowgraph {{{ */
/*! Build the flowgraph of a section
 *
 * Probably the most important function in the backend: Create a flowgraph from
 * a list of disassembled ppc instructions. Works in 3 big steps:
 * - Does leader detection on the list of instructions to identify basic blocks
 * - Converts all position-dependent instructions to pseudo instructions
 * - Draws edges between the basic blocks
 *
 */
void Ppc64Flowgraph(t_object *obj)
{
  int tel;
  t_cfg *cfg = OBJECT_CFG(obj);

  /* Find the leaders in the instruction list */
  STATUS(START,("Leader detection"));
  tel = Ppc64FindBBLLeaders(obj);
  VERBOSE(1, ("Leaders detected: %d", tel));
  STATUS(STOP,("Leader detection"));

  /* Create the basic blocks (platform independent) */
  STATUS(START,("Creating Basic Blocks"));
  tel = CfgCreateBasicBlocks(obj);
  VERBOSE(1, ("BBLs detected: %d", tel));
  STATUS(STOP,("Creating Basic Blocks"));

  /* Create the edges between basic blocks */
  STATUS(START,("Creating Basic Block graph"));
  tel= Ppc64AddBasicBlockEdges(obj);
  STATUS(STOP,("Creating Basic Block graph"));

  /* Remove the calls created by weak symbols 
   * \TODO: it's a kind of optimization, should be moved to anopt */
  STATUS(START,("Patch calls to weak symbols")); 
  tel = PpcPatchCallsToWeakSymbols(obj);
  STATUS(STOP,("Patch calls to weak symbols"));

  /* Kill all relocations from the immediate operands of jump and call instructions: {{{
   * they have no use any more because their information is represented by flow
   * graph edges and also mark the operands that are relocated.
   */ 
  {
    t_bbl * bbl;
    t_ppc_ins * ins;
    CFG_FOREACH_BBL(cfg,bbl)
    {
      BBL_FOREACH_PPC_INS(bbl,ins)
      {
        if (PPC_INS_OPCODE(ins) == PPC_B || PPC_INS_OPCODE(ins) == PPC_BC)
        {
          while (PPC_INS_REFERS_TO(ins))
          {
            RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),RELOC_REF_RELOC(PPC_INS_REFERS_TO(ins)));
          }
          PPC_INS_SET_FLAGS(ins,PPC_INS_FLAGS(ins)&~PPC_FL_RELOCATED);
        }
      }
    }
  } /* }}} */
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
