/*
 * Copyright (C) 2005 {{{
 *      Ramon Bertran Monfort <rbertran@ac.upc.edu>
 *      Lluis Vilanova <xscript@gmx.net>
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
 * This file is part of the PPC port of Diablo (Diablo is a better
 * link-time optimizer)
 */

/* TODO: check generic t_ins ATTRIB and FLAGS (the latter if necessary) */

#include <diabloppc.h>

/* DiabloFlowgraphPpcCfgCreated {{{ */

/*! Add "special" symbols for diablo on a read object.
*
* \param  obj  The object that has been read from disk
* \return void
*/
void
DiabloFlowgraphPpcCfgCreated (t_object *obj, t_cfg *cfg)
{
  t_symbol *sym;
  t_symbol_table *st;
  
  st = OBJECT_SUB_SYMBOL_TABLE (obj);
  ASSERT(st, ("There is no symbol table on the object!"));
  
  SYMBOL_TABLE_FOREACH_SYMBOL (st, sym)
  {
    if (StringPatternMatch ("$diablo:*@-@*", SYMBOL_NAME (sym)))
    {
      /* These symbols are added whenever the difference between two labels is
      * calculated. Such symbols appear (at least) in both code and rodata
      * sections. In rodata sections, the symbols refer to jump table offsets.
      * We don't touch those, as the code in diabloppc_flowgraph.c expects
      * the original relocations there.
      *
      * In code sections, they appear in at least three cases, all related to
      * code compiled with -mrelocatable. The construct looks like this (in
      * the original assembler code, simplified):
      *
      *      .section        ".got2","aw"
      *  .LCTOC1 = .+32768
      *       ...
      *      .section        .rodata.cst8,"aM",@progbits,8
      *      .align 3
      *      .LC0:
      *      .long   1127219200
      *      .long   -2147483648
      *      .section        ".got2","aw"
      *      .LC1:
      *      .long .LC0
      *      ...
      *      .section        ".text"
      *      .align 2
      *      .globl floatdidf
      *  .LCL0:
      *      .long .LCTOC1-.LCF0
      *      .type   floatdidf, @function
      *  floatdidf:
      *      ...
      *      bcl 20,31,.LCF0
      *  .LCF0:
      *      mflr 30
      *      lwz 0,.LCL0-.LCF0(30)
      *      add 30,0,30
      *      ...
      *      lwz 9,.LC1-.LCTOC1(30)
      *
      *      What happens is that the actual data is (in this case) in the .rodata.cst8
      *      section (at .LC0). To get at it, we first calculate the address of the next
      *      instruction (the bcl 20,31,.LCF0), then we load the address at .LCL0 in
      *      a PIC-safe way, which is the difference between the address of the next
      *      instruction and the start of the .got2 section (well, the middle: +32768),
      *      and finally add the address and that difference together (so the result
      *      is .got2+32768).
      *      
      *      Further loads from the .got2 section happen relative to this calculated
      *      address (stored in r30), e.g. the "lwz 9, ..." at the end.
      *
      *      Once code like the above has been assembled, most of the those label
      *      differences are turned into plain constants: since the distance between
      *      .LCL0 and .LCF0 cannot change (both are in the same section), the
      *      "lwz 0,.LCL0-.LCF0(30)" to load the value at .LCL0 doesn't need a
      *      relocation. The same goes for the labels used in the "lwz 9, ..."
      *      at the end (both lie in the .got2 section).
      *
      *      The only one which does get a relocation is the ".long .LCTOC1-.LCF0",
      *      as it crosses different sections. The relocation produced by the
      *      assembler is an offset relative to .LCTOC1, i.e., relative to the start
      *      of the .got2 section. That offset implicitly takes into account the
      *      distance between .LCL0 and .LCF0.
      *
      *      To avoid problems in case the bcl is moved by Diablo, we throw away
      *      the original relocation and replace it again with one calculating
      *      .LCTOC1-.LCF0.
      *
      *      Further, the assembler alls adds $diablo:lab1@-@lab2 symbols
      *      prior to any instructions containing an "lab1-lab2" expression (i.e.,
      *      the earlier mentioned lwz's without relocations).
      *
      *      Now,
      *
      *      a) the .long mentioned above lies in the middle of a code section. To
      *         detect it, we simply look at "instructions" marked with a "$diablo:..."
      *         label which have a relation pointing into the .got2 section and mark these
      *         as data.
      *      b) there are two subcases for instructions of the form  "lwz rA,Lab1-Lab2(rB)",
      *         whereby Lab1-Lab2 was completely evaluated at assembling time and
      *         turned into a constant (and therefore they have no relocations):
      *        1) Lab1 and Lab2 lie in a .got2 section: no change needed, since we
      *           don't change anything in the .got2 section (it's data)
      *        2) Lab2 and Lab1 lie in a .text section (this is the case for the
      *           "lwz 0,.LCL0-.LCF0(30)" above): we have to add a relocation
      *           to update the offset in case the instructions are moved.
      */

      t_string sname = SYMBOL_NAME(sym)+ 8;   /* Strip of $diablo: */
      t_string sname2 = strstr(sname, "@-@");
      /* Subobject this symbol comes from (at this point we haven't disassembled yet, so it's a section) */
      t_object * obj2 = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym)));
      t_symbol * sym2, * sym3;
      t_reloc * rel, *orgrel;
      t_address symaddr;
      t_section *sec;

      symaddr=StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);

      sec = T_SECTION(SYMBOL_BASE(sym));
       /* DISASSEMBLING_CODE_SECTION? */
      if (SECTION_TYPE(sec)!=CODE_SECTION)
        continue;

      sname2[0]='\0'; /* First @ becomes '\0' = first symbol name */
      sname2+=3; /* sname2 = last symbol name */
      sym2=SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj2), sname); /* Use subobject because of possible duplicates */
      sym3=SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj2), sname2);
      ASSERT(sym2 && sym3, ("Could not expand sym @S to relocation (symbols not found)", sym));

      orgrel=RelocatableGetRelocForAddress(T_RELOCATABLE(sec),symaddr);
      if (orgrel &&
          !strcmp(SECTION_NAME(T_SECTION(SYMBOL_BASE(sym2))),".got2") &&
          (SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym3)))==CODE_SECTION))
      {
        /* case a) above: we add a $got2addr symbol to easily recognise it as data.
         *
         * We also remove the original relocation, as --implicitely--- it's an offset
         * relative to the address of .LCF0. We replace it with one calculating again
         * .LCTOC1-.LCF0 as in the original assembler code, with one twist: we change
         * it into .LCTOC1-(.LCF0-4)+4. The reason is that the address calculation
         * depends on the address of the bcl (which stores &bcl+4 in LR) and not
         * on the address of whatever instruction comes after it (so if the
         * instruction coming after it is moved, the value calculated by the
         * relocation mustn't change; similarly, if the bcl is move, it does have
         * to change).
         */
        t_address oldaddr, newaddr;

        oldaddr = StackExecConst(RELOC_CODE(orgrel), orgrel, NULL, 0, obj);
        RelocTableRemoveReloc(OBJECT_RELOC_TABLE(obj),orgrel);

        SymbolTableAddSymbol (st, "$got2addr", "R00A00+$", -1, TRUE, FALSE,
                              T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                              symaddr,
                              AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);

        rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNewForObject(obj,4),  SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_BASE(sym2), SYMBOL_OFFSET_FROM_START(sym2), TRUE,  NULL, NULL, SYMBOL_BASE(sym3), "R00 R01A00+ - \\ l * w \\ s0000$" );
        RELOC_TO_RELOCATABLE_OFFSET(rel)[1] = AddressSubInt32(SYMBOL_OFFSET_FROM_START(sym3),4);
        RELOC_ADDENDS(rel)[0]=AddressNewForObject(obj,4);

        newaddr = StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj);
        ASSERT(AddressIsEq(oldaddr,newaddr),("Addresses different after converting data relocation! @G != @G",oldaddr,newaddr));

      }
      else if (!orgrel &&
               (SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym2)))==CODE_SECTION) &&
               (SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym3)))==CODE_SECTION))
      {
        /* case b.2) above: we add a relocation to adjust the offset of the instruction
         * when needed. We will check later on whether it actually is a load (we haven't
         * disassembled yet at this point). We also perform the same +4/-4 trick as in
         * case a) for the same reason
         */
        SymbolTableAddSymbol (st, "$got2load", "R00A00+$", -1, TRUE, FALSE,
                              T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
                              symaddr,
                              AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
        /* the +2 to the sym offset is because ppc32 R_PPC_ADDR16_LO relocations always point to the
         * to address of the to be relocated value, rather than to the instruction containing it
         * (so we do the same in our own artificial relocation)
         */
        rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj),  SYMBOL_BASE(sym), AddressAddUint32(SYMBOL_OFFSET_FROM_START(sym),2), SYMBOL_BASE(sym2), SYMBOL_OFFSET_FROM_START(sym2), TRUE,  NULL, NULL, SYMBOL_BASE(sym3), "R00 R01A00+ - \\ = sffff & s0010 < l sffff & | w \\ i00008000+ iffff0000&$" );
        RELOC_TO_RELOCATABLE_OFFSET(rel)[1] = AddressSubInt32(SYMBOL_OFFSET_FROM_START(sym3),4);
        RELOC_ADDENDS(rel)[0]=AddressNewForObject(obj,4);
      }
    }
  }

/* We need them later */
  SymbolTableSortSymbolsByAddress (st, "$got2addr");
  SymbolTableSortSymbolsByAddress (st, "$got2load");
}
/* }}} */


/* SymbolTableCachedSearch {{{ */
typedef struct {
  char *name;
  t_symbol *sym;
  t_address addr;
  t_object *obj;
} t_cached_search;

static t_bool
SymbolTableCachedSearch (t_cached_search *cache, t_address addr)
{
  /* This code is based on the cached version of SymbolTableGetDataType */
  t_bool found = FALSE;
  t_symbol *sym;
  t_symbol_table *st = OBJECT_SUB_SYMBOL_TABLE (cache->obj);
  t_address exec;
  
  /* Reset cache if searching on a previous address. */
  if (cache->sym && AddressIsGe (addr, cache->addr))
  {
    if (AddressIsEq(addr, cache->addr)) return TRUE;
    sym = SYMBOL_EQUAL(cache->sym);
  }
  else
    sym = SymbolTableLookup (st, cache->name);
  
  while (sym)
  {
    exec = StackExec(SYMBOL_CODE(sym), NULL, sym, NULL, FALSE, 0, cache->obj);
    if (AddressIsEq (exec, addr))
    {
      found = TRUE;
      break;
    }
    else if (AddressIsGt (exec, addr))
      break;
    sym = SYMBOL_EQUAL(sym);
  }
  
  if (found)
  {
    cache->sym = sym;
    cache->addr = addr;
  }
  
  return found;
}

static t_cached_search *
SymbolTableCachedSearchInit (char *name, t_object *obj)
{
  t_cached_search *res = Malloc (sizeof (t_cached_search));
  res->name = StringDup (name);
  res->sym = NULL;
  res->addr = AddressNullForObject (obj);
  res->obj = obj;
  return res;
}

static void
SymbolTableCachedSearchFini (t_cached_search *cache)
{
  Free (cache->name);
  Free (cache);
}
/* }}} */


typedef enum {
  PPC_ADDRESS_IS_CODE,
  PPC_ADDRESS_IS_DATA,
  PPC_ADDRESS_IS_GOT2DATA,
  PPC_ADDRESS_IS_GOT2LOAD
} t_data_type_ppc;

static t_data_type_ppc
SymbolTableGetDataType2 (t_object *obj, t_address addr, t_cached_search *got2addr, t_cached_search *got2load)
{
  t_uint32 type = SymbolTableGetDataType (obj, addr);
  switch (type)
  {
    case ADDRESS_IS_CODE:
      if (SymbolTableCachedSearch (got2addr, addr))
        return PPC_ADDRESS_IS_GOT2DATA;
      else if (SymbolTableCachedSearch (got2load, addr))
        return PPC_ADDRESS_IS_GOT2LOAD;
      else
        return PPC_ADDRESS_IS_CODE;
    case ADDRESS_IS_DATA:
      return PPC_ADDRESS_IS_DATA;
    default:
      FATAL(("Unexpected ppc address type"));
  }
}


/*!
 * Disassemble an entire section. Creates an array of t_arm_ins's.
 *
 * \param code The section we want to disassemble
 *
 * \return void 
 */
/* PpcDisassembleSection {{{ */
void
PpcDisassembleSection(t_section * code)
{
  t_object * obj=SECTION_OBJECT(code);
  int nins = 0;
  t_address offset;
  t_address o_offset;
  t_uint16 i;
  t_ppc_ins * ins_s=NULL;
  t_ppc_ins * prev;
  t_uint32 data;
  t_bool previous_set=FALSE;
  t_address previous=AddressNullForObject(obj); /* Just to keep the compiler happy */
  int teller = 0;
  t_cached_search *got2addr = SymbolTableCachedSearchInit ("$got2addr", obj);
  t_cached_search *got2load = SymbolTableCachedSearchInit ("$got2load", obj);

  offset = AddressNullForObject (obj);
  o_offset = AddressNullForObject (obj);

  prev = NULL;

  while(AddressIsLt(offset, SECTION_CSIZE(code)))
  {
    nins++;
    ins_s = (t_ppc_ins *) InsNewForSec(code);


    PpcInsInit(ins_s);
    
    SECTION_ADDRESS_TO_INS_MAP(code)[teller]=(t_ins*) ins_s;

    if (!ins_s) 
    {
      FATAL(("No instructions!"));
    }
    //PPC_INS_SET_REFERS_TO(ins_s, NULL);  /* TODO: Check if this is necessary */
    //PPC_INS_SET_REFED_BY(ins_s, NULL); /* TODO: Check if this is necessary */
    if(AddressExtractUint32(AddressAdd(SECTION_CADDRESS(code),offset)) & 0x3) 
    {
      FATAL(("Not aligned in ppc modus!"));
    }
    data=SectionGetData32(code,offset);

    PPC_INS_SET_CADDRESS(ins_s,  AddressAdd(SECTION_CADDRESS(code),offset));

    for (i = 0; ((data) & ppc_opcode_table[i].mask) !=ppc_opcode_table[i].opcode; i++)
    {
      /* here we are looking for the opcode in the opcode table */
    }
    
    switch (SymbolTableGetDataType2(obj,PPC_INS_CADDRESS(ins_s),got2addr,got2load))
    {
      case PPC_ADDRESS_IS_DATA:
      case PPC_ADDRESS_IS_GOT2DATA:
      {
        PpcInsSet(ins_s, PPC_DATA);
        PpcDisassembleData(ins_s, data, 0);
        break;
      }
      case PPC_ADDRESS_IS_GOT2LOAD:
      {
        PpcInsSet(ins_s,i); 
        ppc_opcode_table[i].Disassemble(ins_s,(data),i);
        switch (PPC_INS_OPCODE(ins_s))
        {
          case PPC_LWZ: case PPC_LHZ: case PPC_LHA: case PPC_LBZ: 
          case PPC_LWZX: case PPC_LHZX: case PPC_LHAX: case PPC_LBZX:
          {
            t_reloc *rel;
            
            rel = RelocatableGetRelocForAddress(T_RELOCATABLE(code), AddressAddUint32(PPC_INS_CADDRESS(ins_s),2));
            ASSERT(rel,("got2load at @G rr has no relocation",PPC_INS_CADDRESS(ins_s)));
            ASSERT(RELOC_N_TO_RELOCATABLES(rel)==2,("got2load at @G relocation does not refer to two relocatables",PPC_INS_CADDRESS(ins_s)));
            ASSERT(AddressIsEq(StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj),PPC_INS_IMMEDIATE(ins_s)),("got2load at @G relocation calculates wrong address",PPC_INS_CADDRESS(ins_s)));
            break;
          }
          default:
            FATAL(("PPC_ADDRESS_IS_GOT2LOAD type for non-load instruction at @G",PPC_INS_CADDRESS(ins_s)));
        }
        break;
      }
      case PPC_ADDRESS_IS_CODE:
      {
        if (data == 0)
        {
          /*TODO: Check why this happens */
          /* Checked: Alingment issues */
          VERBOSE(1,("!!! This is empty instruction introduced by merged code!!!"));
          PpcInsMakeNOOP(ins_s); 
        }
        else if (ppc_opcode_table[i].Disassemble == PpcDisassembleUnknown)
        {
          FATAL(("unknown instruction %x at @G", data, PPC_INS_CADDRESS(ins_s)));
        }
        else
        {
          PpcInsSet(ins_s,i); 
          ppc_opcode_table[i].Disassemble(ins_s,(data),i);
        }
        break;
      }
      default:
      {
        FATAL(("Unhandled ppc address type for instruction @G",PPC_INS_CADDRESS(ins_s)));
      }
    }

    PPC_INS_SET_REGS_USE(ins_s, PpcUsedRegisters(ins_s));
    PPC_INS_SET_REGS_DEF(ins_s, PpcDefinedRegisters(ins_s));
    
    /*TODO: Mark the instruction with FL_RELOCATABLE if a
      relocation point to it.
      PpcCheckRelocated(ins_s); */
    
    PPC_INS_SET_CSIZE(ins_s, AddressNew32(4));
    PPC_INS_SET_OLD_SIZE(ins_s, AddressNew32(4));
    /* Bookkeeping to give the old_address a meaningfull value {{{*/
    if (OBJECT_N_ORIG_CODES(obj))
    {
      t_bool found=FALSE;
      t_uint32 tel;
      for (tel=0;tel<OBJECT_N_ORIG_CODES(obj); tel++)
      {
        if ((AddressIsLe(OBJECT_ORIG_CODES(obj)[tel].base,PPC_INS_CADDRESS(ins_s)))&&(AddressIsGt(AddressAdd(OBJECT_ORIG_CODES(obj)[tel].base,OBJECT_ORIG_CODES(obj)[tel].size),PPC_INS_CADDRESS(ins_s))))
        {
          found = TRUE;

          if (previous_set && (!AddressIsEq(previous,OBJECT_ORIG_CODES(obj)[tel].obase)))
          {
            o_offset=AddressNew32(0);
          }
          previous=OBJECT_ORIG_CODES(obj)[tel].obase;
          previous_set=TRUE;
          if (G_T_UINT32(OBJECT_ORIG_CODES(obj)[tel].obase)==0x0) PPC_INS_SET_OLD_ADDRESS(ins_s, AddressNew32(0xffffffff));
          else  PPC_INS_SET_OLD_ADDRESS(ins_s,  AddressAdd(OBJECT_ORIG_CODES(obj)[tel].obase,o_offset));
          break;
        }
      }
      if (!found) 
      {
        if (!previous_set)
        {
          FATAL(("Could not map merged address (@G -- @I) to original",PPC_INS_CADDRESS(ins_s),ins_s));
        }
        else
        {
          PPC_INS_SET_OLD_ADDRESS(ins_s,  AddressAdd(previous,o_offset));
        }
      }
    }
    else
    {
      /*      FATAL(("HIER\n")); Commented out, because i couldn't find out why this is fatal? */
      PPC_INS_SET_OLD_ADDRESS(ins_s,  AddressAdd(SECTION_CADDRESS(code),offset));
    }/*}}}*/

    PPC_INS_SET_IPREV(ins_s,  prev);
    if (prev) 
    {
      PPC_INS_SET_INEXT(prev,  ins_s);
    }
    if (PPC_INS_INEXT(ins_s)) 
    {
      prev = PPC_INS_INEXT(ins_s); /* make prev point to dummy */
    }
    else
    {
      prev=ins_s;
    }

    if(AddressExtractUint32(PPC_INS_CSIZE(ins_s)) == 4)
    {
      offset=AddressAddUint32(offset,4);
      o_offset=AddressAddUint32(o_offset,4);
      teller+=4;
    }
    else
    {
      FATAL(("Impossible CSIZE for instructions!"));
    }
  }

  PPC_INS_SET_INEXT(ins_s,  NULL);

  /* Verbose information */
  VERBOSE(1,("Disassembled %d instructions",nins));
}
/* }}} */

/* vim:set sw=2 tw=80 foldmethod=marker expandtab cindent cinoptions=p5,t0,(0: */
