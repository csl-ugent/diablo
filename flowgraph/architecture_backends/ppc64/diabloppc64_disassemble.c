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

/* DiabloFlowgraphPpc64CfgCreated {{{ */
#define DEBUG_DATA_SYMBOLS 0

/*! Add "special" symbols for diablo on a read object.
 *
 * \param  obj  The object that has been read from disk
 * \return void
 */
void
DiabloFlowgraphPpc64CfgCreated (t_object *obj, t_cfg *cfg)
{
  t_symbol *sym;
  t_symbol_table *st;

  if (strcmp("ppc64", OBJECT_OBJECT_HANDLER(obj)->sub_name))
    return;

  st = OBJECT_SUB_SYMBOL_TABLE (obj);
  ASSERT(st, ("There is no symbol table on the object!"));

  SYMBOL_TABLE_FOREACH_SYMBOL (st, sym)
  {
    /* There's a .LT_<symname>_name_end symbol at the end of the traceback entry,
     * which may be at the same address as the start address of the next symbol
     * (which can be e.g. a function) -> don't consider this as a traceback entry
     * Since "_name_end" can also be part of the symbol name proper, check whether
     * it really comes at the end.
     */
    if (StringPatternMatch (".LT*", SYMBOL_NAME (sym)) &&
        !StringPatternMatch ("*_name_end", SYMBOL_NAME (sym)))
    {
      /* Traceback table */
      SymbolTableAddSymbol (st, "$traceback", "R00A00+$", -1, TRUE, FALSE,
          T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
          StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj),
          AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
#if DEBUG_DATA_SYMBOLS
      DEBUG(("Traceback mark: @S @@@G", sym, StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj)));
#endif
    }
    else if (StringPatternMatch ("$diablo:*@-@*", SYMBOL_NAME (sym)))
    {
      t_string sname = StringDup(SYMBOL_NAME(sym) + 8);   /* Strip off $diablo: */ 
      t_string sname2 = strstr(sname, "@-@"); /* Second part of a @-@ b */
      /* Subobject this symbol comes from (at this point we haven't disassembled yet, so it's a section) */
      t_object * obj2 = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym)));
      t_symbol * sym2, * sym3;
      t_reloc * rel;

      /* Switch table */
      if (SECTION_TYPE(T_SECTION(SYMBOL_BASE(sym))) == CODE_SECTION)
      {
        SymbolTableAddSymbol (st, "$switchtable", "R00A00+$", -1, TRUE, FALSE,
            T_RELOCATABLE(OBJECT_ABS_SECTION(obj)),
            StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj),
            AddressNullForObject(obj), NULL, AddressNullForObject(obj), 0);
      }

      sname2[0]='\0'; /* First @ becomes '\0' = first symbol name */
      sname2+=3; /* sname2 = last symbol name */
      sym2=SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj2), sname); /* Use subobject because of possible duplicates */
      sym3=SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj2), sname2);

      /* the previous approach won't work for symbols in vectorized sections, as
       * the original symbol and its mapped counterpart are no longer in the
       * same object file. Therefore, we need to take the long way round to find
       * the symbols {{{ */
      
      /* first approach works if the referenced symbols come from the same
       * section as the @-@ symbol */
      if (!sym2)
      {
        sym2 = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), sname);
        while (sym2)
        {
          if (SYMBOL_BASE(sym2) &&
              RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym2)) == RT_SUBSECTION &&
              SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym2))) == obj2)
            break;
          sym2 = SYMBOL_EQUAL(sym2);
        }
      }
      if (!sym3)
      {
        sym3 = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), sname2);
        while (sym3)
        {
          if (SYMBOL_BASE(sym3) &&
              RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym3)) == RT_SUBSECTION &&
              SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym3))) == obj2)
            break;
          sym3 = SYMBOL_EQUAL(sym3);
        }
      }

      /* second approach works if the referenced symbols come from a different,
       * non-vectorized section in the original object */
      if (!sym2)
      {
        sym2 = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), sname);
        while (sym2)
        {
          if (SYMBOL_BASE(sym2) &&
              RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym2)) == RT_SUBSECTION)
          {
            t_object *obj3 = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym2)));
            t_symbol *tsym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj3), SYMBOL_NAME(sym));
            if (tsym && SYMBOL_MAPPED(tsym) == sym)
              break;
          }
          sym2 = SYMBOL_EQUAL(sym2);
        }
      }
      if (!sym3)
      {
        sym3 = SymbolTableGetSymbolByName(OBJECT_SUB_SYMBOL_TABLE(obj), sname2);
        while (sym3)
        {
          if (SYMBOL_BASE(sym3) &&
              RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym3)) == RT_SUBSECTION)
          {
            t_object *obj3 = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym3)));
            t_symbol *tsym = SymbolTableGetSymbolByName(OBJECT_SYMBOL_TABLE(obj3), SYMBOL_NAME(sym));
            if (tsym && SYMBOL_MAPPED(tsym) == sym)
              break;
          }
          sym3 = SYMBOL_EQUAL(sym3);
        }
      }
      /* }}} */

      /*VERBOSE(0,("Looking for %s and %s in %s", sname, sname2, OBJECT_NAME(obj2)));*/
      /*if (sym2) VERBOSE(0, ("sym2: @S", sym2));*/
      /*if (sym3) VERBOSE(0, ("sym3: @S", sym3));*/
      ASSERT(sym2 && sym3, ("Could not expand sym @S to relocation (symbols not found)", sym));

      rel = RelocTableAddRelocToRelocatable(OBJECT_RELOC_TABLE(obj), AddressNullForObject(obj),  SYMBOL_BASE(sym), SYMBOL_OFFSET_FROM_START(sym), SYMBOL_BASE(sym2), SYMBOL_OFFSET_FROM_START(sym2), TRUE,  NULL, NULL, SYMBOL_BASE(sym3), "R00 R01 - \\ l * w \\ s0000$" );
      RELOC_TO_RELOCATABLE_OFFSET(rel)[1] = SYMBOL_OFFSET_FROM_START(sym3);

      Free(sname);

#if DEBUG_DATA_SYMBOLS
      DEBUG(("Switch table mark: @S @@@G", sym, StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj)));
#endif
    }
  }

  /* We need them later */
  SymbolTableSortSymbolsByAddress (st, "$switchtable");
  SymbolTableSortSymbolsByAddress (st, "$traceback");
}
/* }}} */

/* Beware that this must be kept synchronised with ADDRESS_IS_* defines */
typedef enum {
  PPC64_TYPE_IS_CODE = 1,
  PPC64_TYPE_IS_DATA,
  PPC64_TYPE_IS_SMALL_CODE,
  PPC64_TYPE_IS_SWITCHTABLE,
  PPC64_TYPE_IS_TRACEBACK
} t_data_type;

/* SymbolTableCachedSearch {{{ */
typedef struct {
  char *name;
  t_symbol *sym;
  t_address addr;
  t_object *obj;
} t_cached_search;

  t_bool
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

/* SymbolTableGetDataType2 {{{ */
/*! Helper function to discover the type of a piece of data.
 *
 * \param obj Object to search on
 * \param addr Address to check for
 * \param switchtables Cached search for switch tables
 * \param tracebacks Cached search for traceback tables
 *
 * \return Type of the piece of data (t_data_type)
 */
  static t_data_type
SymbolTableGetDataType2 (t_object *obj, t_address addr, t_cached_search *switchtables, t_cached_search *tracebacks)
{
  t_data_type type = SymbolTableGetDataType (obj, addr);
  switch (type)
  {
    case ADDRESS_IS_CODE:
      if (SymbolTableCachedSearch (switchtables, addr))
        return PPC64_TYPE_IS_SWITCHTABLE;
      else if (SymbolTableCachedSearch (tracebacks, addr))
        return PPC64_TYPE_IS_TRACEBACK;
      else
        return PPC64_TYPE_IS_CODE;
    default:
      return type;
  }
}
/* }}} */

/* Ppc64DisassembleSection {{{ */
#define DEBUG_TYPE    0
#define VERBOSE_OTHER 0

/*! Disassemble an entire section. Creates an array of t_ppc_ins's.
 *
 * \param code The section we want to disassemble
 *
 * \return void
 */
  void
Ppc64DisassembleSection (t_section *code)
{
  t_object *obj = SECTION_OBJECT(code);
  int nins = 0;
  t_address offset;
  t_ppc_ins *ins_s = NULL;
  t_ppc_ins *prev;
  t_uint32 data;
  t_address current;
  t_address upto = AddressNullForObject (obj);
  t_string uptoed = NULL;
  int teller = 0;
  int i;

  t_cached_search *switchtables = SymbolTableCachedSearchInit ("$switchtable", obj);
  t_cached_search *tracebacks = SymbolTableCachedSearchInit ("$traceback", obj);

  offset = AddressNullForObject (obj);
  prev = NULL;

  while (AddressIsLt (offset, SECTION_CSIZE (code)))
  {
    nins++;
    ins_s = PpcInsNewForSec (code);
    PpcInsInit(ins_s);
    
    current = AddressAdd (SECTION_CADDRESS (code), offset);

    PPC_INS_SET_CADDRESS (ins_s, current);
    PPC_INS_SET_OLD_ADDRESS (ins_s, AddressAdd (SECTION_OLD_ADDRESS (code), offset));
    if (!AddressIsEq (PPC_INS_CADDRESS (ins_s), PPC_INS_OLD_ADDRESS (ins_s)))
      DEBUG(("Current and old address do not match: @G -- @G",
            PPC_INS_CADDRESS (ins_s), PPC_INS_OLD_ADDRESS (ins_s)));

    PPC_INS_SET_CSIZE (ins_s, AddressNewForObject (obj, 4));
    PPC_INS_SET_OLD_SIZE (ins_s, AddressNewForObject (obj, 4));

    SECTION_ADDRESS_TO_INS_MAP(code)[teller] = T_INS (ins_s);

    if (!ins_s) FATAL (("No instructions!"));

    ASSERT (!(AddressExtractUint32 (current) & 0x3),
        ("Instruction in ppc64 mode not aligned"));

    data = SectionGetData32 (code, offset);

    if (AddressIsNull (upto)) /* Let's see what this bits are... {{{ */
    {
      switch (SymbolTableGetDataType2 (obj, current, switchtables, tracebacks))
      {
        case PPC64_TYPE_IS_DATA: /* {{{ */
#if DEBUG_TYPE
          DEBUG(("Data: @G", PPC_INS_OLD_ADDRESS(ins_s)));
#endif
          PpcDisassembleData (T_PPC_INS (ins_s), data, PPC_DATA);
          break;
          /* }}} */
        case PPC64_TYPE_IS_CODE: /* {{{ */
          if (uptoed)
          {
            Free (uptoed);
            uptoed = NULL;
          }

          /* lookup the opcode in the opcode table */
          for (i = 0; ((data) & ppc_opcode_table[i].mask) != ppc_opcode_table[i].opcode; i++) ;

          if(data != 0)
          {
            PpcInsSet (ins_s, i); 
            ppc_opcode_table[i].Disassemble (T_PPC_INS (ins_s), data, i);
          }
          else
          {
            /* Alignment issues */
#if VERBOSE_OTHER
            VERBOSE(1,("This is empty instruction introduced by merged code (@G)!!!", PPC_INS_CADDRESS(ins_s)));
#endif
            PpcDisassembleData (T_PPC_INS (ins_s), data, PPC_DATA);
            
            /* XXX this is an ugly hack: hardcode a particular data pattern that
             * appears in libc_nonshared.a in the cell SDK 3.0 toolchain. We
             * don't have a version of this file that is compiled with the
             * patched toolchain unfortunately */

            t_uint32 nextdata = SectionGetData32 (code, AddressAddUint32(offset, 4));
            if (nextdata == 0x1)
            {
              /* disassemble the next two instructions as data */
              upto = AddressAddUint32(current, 8);
            }
          }

          PPC_INS_SET_REGS_USE (ins_s, PpcUsedRegisters (ins_s));
          PPC_INS_SET_REGS_DEF (ins_s, PpcDefinedRegisters (ins_s));
#if DEBUG_TYPE
          DEBUG(("Code %#x: @I", data, ins_s));
#endif
          break;
          /* }}} */
        case PPC64_TYPE_IS_SWITCHTABLE: /* {{{ */
          Ppc64DisassembleSwitch (T_PPC_INS (ins_s), data);
#if DEBUG_TYPE
          DEBUG(("Switch table @G", PPC_INS_OLD_ADDRESS(ins_s)));
#endif
          break;
          /* }}} */
        case PPC64_TYPE_IS_TRACEBACK: /* {{{ */
          {
            /* Skip the traceback table as plain and static data 
             * (glibc-2.3.6/sysdeps/powerpc/powerpc64/sysdep.h:175)
             * This has no type of relocation-related information. */
            t_symbol *sym;
            int len, data_len;

            for (sym = SymbolTableGetFirstSymbolByAddress (OBJECT_SUB_SYMBOL_TABLE (obj), current);
                sym != NULL;
                sym = SymbolTableGetNextSymbolByAddress (sym, current))
            {
              if (StringPatternMatch (".LT*", SYMBOL_NAME (sym)))
                break;
            }
            ASSERT(sym, ("Couldn't find traceback symbol in @G", current));

            len = strlen (SYMBOL_NAME (sym)) - 3;
            data_len = 
              4 + /* .long 0 */
              8 + /* .quad TB_DEFAULT */
              4 + /* .long LT_LABEL(name)-BODY_LABEL(name) */
              2 + /* .short  LT_LABELSUFFIX(name,_name_end)-LT_LABELSUFFIX(name,_name_start) */
              len /* .ascii  stringify(name) */
              - 4 /* we have to stop at the final data .long, so the next long will be seen as instruction */
              ;
            data_len = (data_len + 3) & (~3);   /* .align 2 */
            upto = AddressAddUint32 (current, data_len);
            uptoed = StringDup (SYMBOL_NAME (sym) + 3);
            PpcDisassembleData (T_PPC_INS (ins_s), data, PPC_DATA);
#if DEBUG_TYPE
            DEBUG(("Traceback @G upto @G", PPC_INS_OLD_ADDRESS(ins_s), upto));
#endif
          }
          break;
          /* }}} */
        default:
          FATAL(("Unsupported data type while disassembling: %X", SymbolTableGetDataType (obj, current)));
          break;
      }
    } /* }}} */
    else /* Continue reading a bunch of data inside a code section {{{ */
    {
#if DEBUG_TYPE
      DEBUG(("Traceback @G", PPC_INS_OLD_ADDRESS(ins_s)));
#endif
      PpcDisassembleData (T_PPC_INS (ins_s), data, PPC_DATA);
      if (AddressIsEq (current, upto))
        upto = AddressNullForObject (obj);
    } /* }}} */

    PPC_INS_SET_IPREV (ins_s, prev);
    if (prev) PPC_INS_SET_INEXT (prev, ins_s);
    prev = ins_s;

    offset = AddressAddUint32 (offset, 4);
    teller += 4;
  }

  PPC_INS_SET_INEXT(ins_s,  NULL);

  SymbolTableCachedSearchFini (switchtables);
  SymbolTableCachedSearchFini (tracebacks);

  VERBOSE(1, ("Disassembled %d instructions", nins));
}
/* }}} */

/* vim:set ts=4 sw=2 tw=80 foldmethod=marker expandtab: */
