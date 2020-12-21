/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <string.h>
#include <diabloflowgraph.h>

/* Architecture handlers {{{ */
struct _t_architecture_handler
{
  struct _t_architecture_handler *next_handler;
  struct _t_architecture_handler *prev_handler;
  t_string name;
  t_architecture_description *description;
  t_address_type address_size;
};

typedef struct _t_architecture_handler t_architecture_handler;

t_architecture_handler *architecture_table = NULL;

t_architecture_description *
ArchitectureGetDescription (t_const_string arch)
{
  t_architecture_handler *iter = architecture_table;
  for (; iter; iter = iter->next_handler)
    if (!strcmp (iter->name, arch))
      return iter->description;
  return NULL;
}

t_address_type
ArchitectureGetAddressSize (t_const_string arch)
{
  t_architecture_handler *iter = architecture_table;
  for (; iter; iter = iter->next_handler)
    if (!strcmp (iter->name, arch))
      return iter->address_size;
  FATAL (("Could not find architecture handler"));
  return ADDRSIZE32;
}

t_architecture_description *
ObjectGetArchitectureDescription (t_object * obj)
{
  return ArchitectureGetDescription (OBJECT_OBJECT_HANDLER (obj)->sub_name);
}

void
ArchitectureHandlerAdd (t_const_string arch_name, t_architecture_description * description, t_address_type addrsize)
{
  t_architecture_handler *t = Malloc (sizeof (t_architecture_handler));

  t->name = StringDup (arch_name);
  t->description = description;
  t->address_size = addrsize;

  t->next_handler = architecture_table;
  if (architecture_table)
    architecture_table->prev_handler = t;
  t->prev_handler = NULL;
  architecture_table = t;
}

void
ArchitectureHandlerRemove (t_const_string arch_name)
{
  t_architecture_handler *prev = NULL;
  t_architecture_handler *next = NULL;
  t_architecture_handler *iter = architecture_table;

  while (iter)
  {
    next = iter->next_handler;
    if (!(strcmp (arch_name, iter->name)))
    {
      if (iter->next_handler)
        iter->next_handler->prev_handler = prev;

      if (prev)
      {
        prev->next_handler = iter->next_handler;
      }
      else
      {
        architecture_table = iter->next_handler;
      }

      Free (iter->name);
      Free (iter);
    }
    prev = iter;
    iter = next;
  }
  return;

}

/* }}} */

static void SymbolMigrateToInstruction(t_object *obj, t_section *code, t_symbol *sym);

void RelocsMigrateToInstructions (t_object * obj, t_section * code);

/* FUN : ObjectDisassemble
 * PAR : An objectfile
 * RET : nothing
 * DESC: Disassembles an object file (called as an action) */

void
ObjectDisassemble (t_object * obj)
{
  t_uint32 tel;

  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);

  ASSERT(desc, ("Cannot disassemble (no architecture description) for architecture %s.\nThis is probably because the architecture backend of this architecture was not initialized in the frontend!", OBJECT_OBJECT_HANDLER(obj)->sub_name));
  ASSERT(desc->DisassembleSec, ("Cannot disassemble (no disassemble in architecture description) for architecture %s", OBJECT_OBJECT_HANDLER(obj)->sub_name));

  DiabloBrokerCall("ObjectDisassembleBefore", obj);

  for (tel = 0; tel < OBJECT_NCODES(obj); tel++)
  {
    t_section *sec = OBJECT_CODE(obj)[tel];

    STATUS(START, ("Disassemble section %s at address 0x%0x",SECTION_NAME(sec),SECTION_CADDRESS(sec)));
    SectionInitDisassembly (sec);
    desc->DisassembleSec (sec);
    SectionFiniDisassembly (obj, sec);
    STATUS(STOP, ("Disassemble"));
  }
  if (OBJECT_DYNAMIC_SYMBOL_TABLE(obj))
  {
    t_symbol *sym;
    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_DYNAMIC_SYMBOL_TABLE(obj),sym)
    {
      t_section *base;
      base = T_SECTION(SYMBOL_BASE(sym));
      if (SECTION_TYPE(base) == DISASSEMBLED_CODE_SECTION)
      {
        SymbolMigrateToInstruction(obj,base,sym);
      }
    }
  }
}

t_uint64
SectionRecalculateSizeAssembling (t_section * sec,
                                  t_relocatable_address_type type)
{
  t_uint64 endaddr = 0;
  t_ins *ins;
  t_uint32 nins = 0;

  if (type == MIN_ADDRESS)
    FATAL(("implement minimal address calculation in this case"));

  for (ins = (t_ins *) SECTION_DATA(sec); ins; ins = INS_INEXT(ins), nins++)
    endaddr += AddressExtractUint64 (INS_CSIZE(ins));
  SECTION_SET_NINS(sec, nins);
  return endaddr;
}

/* FUN : ObjectAssemble
 * PAR : an objectfile
 * RET : nothing
 * DESC: Assemble an objectfile (called as an action) */

void
ObjectAssemble (t_object * obj)
{
  t_uint32 tel;
  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);
  t_cfg *cfg = NULL;

  SymbolTableSetSymbolTypes(OBJECT_SUB_SYMBOL_TABLE(obj));

  for (tel = 0; tel < OBJECT_NCODES(obj); tel++)
  {
    t_section *sec = OBJECT_CODE(obj)[tel];

    if (!cfg) cfg = OBJECT_CFG(SECTION_OBJECT(sec));

    STATUS(START, ("Assemble section %s at address 0x%xT",SECTION_NAME(sec),SECTION_CADDRESS(sec)));
    SectionInitAssembly (obj, sec, 0);
    desc->AssembleSec (sec);
    SECTION_CALLBACKS(sec)->SectionRecalculateSize =
      SectionRecalculateSizeAssembling;
    SectionFiniAssembly (obj, sec);
    STATUS(STOP, ("Assemble section"));
  }

  DiabloBrokerCall ("AfterObjectAssembled", obj);

  /* the cfg is no longer needed if all code is assembled */
  if (cfg)
    CfgFree (cfg);
}

/* {{{ Split off switch jump tables */
static void SplitOffJumpTable(t_section *sec, t_uint32 offset, t_uint32 size)
{
  t_reloc_ref *rr, *rr2;
  t_section *parent = SECTION_PARENT_SECTION(sec);
  t_section *new = SectionCreateForObject(SECTION_OBJECT(sec),
      SECTION_TYPE(sec), parent, AddressNewForSection(sec,size), "jmptable");
  SECTION_SET_CADDRESS(new,
      AddressAddUint32(SECTION_CADDRESS(sec), offset));
  SECTION_SET_OLD_ADDRESS(new,
      AddressAddUint32(SECTION_OLD_ADDRESS(sec), offset));
  SECTION_SET_ALIGNMENT(new, SECTION_ALIGNMENT(sec));

  /* adjust relocations */
  /* {{{ refers to */
  for (rr = SECTION_REFERS_TO(sec), rr2 = rr ? RELOC_REF_NEXT(rr) : NULL;
      rr;
      rr = rr2, rr2 = rr2 ? RELOC_REF_NEXT(rr2) : NULL)
  {
    t_reloc *rel = RELOC_REF_RELOC(rr);
    t_uint32 fromoffset = G_T_UINT32(RELOC_FROM_OFFSET(rel));
    if (fromoffset < offset || fromoffset >= offset+size)
    {
      /* adjust from offset for new section size */
      if (fromoffset >= offset+size)
	RELOC_SET_FROM_OFFSET(rel,
	    AddressSubUint32(RELOC_FROM_OFFSET(rel), size));
    }
    else
    {
      /* move relocation to new section */
      RelocSetFrom(rel, T_RELOCATABLE(new));
      RELOC_SET_FROM_OFFSET(rel,
	  AddressSubUint32(RELOC_FROM_OFFSET(rel), offset));
    }
  } /* }}} */
  /* {{{ refed by */
  for (rr = SECTION_REFED_BY(sec), rr2 = rr ? RELOC_REF_NEXT(rr) : NULL;
      rr;
      rr = rr2, rr2 = rr2 ? RELOC_REF_NEXT(rr2) : NULL)
  {
    t_uint32 i;
    t_reloc *rel = RELOC_REF_RELOC(rr);

    for (i = 0; i < RELOC_N_TO_RELOCATABLES(rel); ++i)
    {
      if (T_RELOCATABLE(sec) == RELOC_TO_RELOCATABLE(rel)[i])
      {
	t_uint32 to_offset = G_T_UINT32(RELOC_TO_RELOCATABLE_OFFSET(rel)[i]);
	if (to_offset >= offset+size)
	  RELOC_TO_RELOCATABLE_OFFSET(rel)[i] =
	    AddressSubUint32(RELOC_TO_RELOCATABLE_OFFSET(rel)[i], size);
	else if (to_offset >= offset)
	{
	  ASSERT(to_offset == offset, ("reference to middle of jump table"));
	  RelocSetToRelocatable(rel, i, T_RELOCATABLE(new));
	  RELOC_TO_RELOCATABLE_OFFSET(rel)[i] = AddressNewForSection(sec, 0);
	}
      }
    }
  } /* }}} */

  /* adjust section data */
  memmove(
      ((char *)SECTION_DATA(sec))+offset,
      ((char *)SECTION_DATA(sec))+offset+size,
      G_T_UINT32(SECTION_CSIZE(sec)) - size - offset);
  SECTION_SET_CSIZE(sec, AddressSubUint32(SECTION_CSIZE(sec), size));
  if (offset == 0)
  {
    SECTION_SET_CADDRESS(sec,
	AddressAddUint32(SECTION_CADDRESS(sec), size));
    SECTION_SET_OLD_ADDRESS(sec,
	AddressAddUint32(SECTION_OLD_ADDRESS(sec), size));
  }
}

static void SeparateJumpTables(t_cfg *cfg)
{
  t_bbl *bbl;
  t_cfg_edge *edge;
  t_bool change = TRUE;
  int rounds = 0;

  /* do this iteratively because we only want to split off
   * jump tables at the start or end of the section and
   * after doing this, a jump table in the middle of a section
   * may as well become first or last */
  while (change)
  {
    printf("------------[round %d]-------------\n", ++rounds);
    change = FALSE;
    CFG_FOREACH_BBL(cfg, bbl)
    {
      t_ins *last;
      t_uint32 bound;
      t_reloc *rel;
      t_relocatable *to;
      t_section *sec;
      t_uint32 secsize;
      t_uint32 offset_in_sec;

      BBL_FOREACH_SUCC_EDGE(bbl, edge)
	if (CFG_EDGE_CAT(edge) == ET_SWITCH ||
	    CFG_EDGE_CAT(edge) == ET_IPSWITCH)
	  break;
      if (!edge) continue;

      /* get switch bound */
      bound = 0;
      BBL_FOREACH_SUCC_EDGE(bbl, edge)
      {
	if (CFG_EDGE_CAT(edge) == ET_SWITCH ||
	    CFG_EDGE_CAT(edge) == ET_IPSWITCH)
	  if (bound < CFG_EDGE_SWITCHVALUE(edge))
	    bound = CFG_EDGE_SWITCHVALUE(edge);
      }

      last = BBL_INS_LAST(bbl);
      if (!INS_REFERS_TO(last)) continue;
      if (RELOC_REF_NEXT(INS_REFERS_TO(last))) continue;
      rel = RELOC_REF_RELOC(INS_REFERS_TO(last));
      if (RELOC_N_TO_RELOCATABLES(rel) != 1) continue;
      to = RELOC_TO_RELOCATABLE(rel)[0];
      if (RELOCATABLE_RELOCATABLE_TYPE(to) != RT_SUBSECTION) continue;
      sec = T_SECTION(to);

      secsize = G_T_UINT32(SECTION_CSIZE(sec));
      offset_in_sec = G_T_UINT32(RELOC_TO_RELOCATABLE_OFFSET(rel)[0]);
      if (secsize <= 4*(bound+1)) continue; /* jump table == complete section */
      if (offset_in_sec != 0 && offset_in_sec + 4*(bound+1) != secsize)
      {
	VERBOSE(1,("Nocando in middle"));
	continue;	/* jump table not first or last in section */
      }

      VERBOSE(1,("Can split off jump table for @iB", bbl));
      VERBOSE(1,("section size %d table size %d offset %d",
	    secsize, 4*(bound+1), offset_in_sec));

      /* split off */
      SplitOffJumpTable(sec, offset_in_sec, 4*(bound+1));
      change = TRUE;
    }
  }
} /* }}} */

void
ObjectConnectDynamicSymbols(t_cfg* cfg)
{
  t_object* obj             = CFG_OBJECT(cfg);
  t_reloc*  reloc           = NULL;
  t_bbl*    bbl             = NULL;
  t_uint32  num_exports     = 0;
  t_uint32  i               = 0;
  t_uint32* export_bbl_idx  = NULL;
  t_reloc** export_relocs   = NULL;

  OBJECT_FOREACH_RELOC(obj, reloc)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(reloc)) == RT_SUBSECTION
      && SECTION_FLAGS(T_SECTION(RELOC_FROM(reloc))) & SECTION_FLAG_EXPORT_SECTION)
    {
      /* look for the .edata => .text reloc */
      for (i = 0; i < RELOC_N_TO_RELOCATABLES(reloc); ++i)
      {
        if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(reloc)[i]) == RT_BBL)
        {
          num_exports++;
          export_relocs = (t_reloc**) Realloc(export_relocs, num_exports * sizeof(t_reloc*));
          export_relocs[num_exports - 1] = reloc;
          export_bbl_idx = (t_uint32*) Realloc(export_bbl_idx, num_exports * sizeof(t_uint32));
          export_bbl_idx[num_exports - 1] = i;
          //VERBOSE(0, ("RELOC FROM EXPORT SYM @R", reloc));
        }
      }
    }
    else
    {
      for (i = 0; i < RELOC_N_TO_RELOCATABLES(reloc); ++i)
      {
        /* TODO: Ask jonas why none of the bbls refers to symbols... */
        if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(reloc)[i]) == RT_SUBSECTION)
        {
          if (SECTION_FLAGS(T_SECTION(RELOC_TO_RELOCATABLE(reloc)[i])) & SECTION_FLAG_IMPORT_SECTION)
          {
            //VERBOSE(0, ("RELOC TO IMPORT SYM @R", reloc));

            /* This would be a good place to replace hell edges by dyncall edges */
          }
        }
      }
    }
  }

  /* make new entrypoints */
  for (i = 0; i < num_exports; ++i)
  {
    t_bbl*      bbl         = T_BBL(RELOC_TO_RELOCATABLE(export_relocs[i])[export_bbl_idx[i]]);

    CfgEdgeCreate(cfg, CFG_UNIQUE_ENTRY_NODE(cfg), bbl, ET_JUMP);
  }

  if (export_bbl_idx)
    Free(export_bbl_idx);
  if (export_relocs)
    Free(export_relocs);
}

/* FUN : ObjectFlowgraph
 * PAR : an object for which we want the flowgraph
 * RET : nothing
 * DESC: Create the flowgraph for for an entire object */

void
ObjectFlowgraph (t_object * obj,
                 t_const_string const * force_leader,
                 t_const_string const * force_reachable,
                 t_bool preserve_functions_by_symbol )
{
  t_uint32 i;
  t_uint32 ret;
  t_section *sec;
  t_function *function;
  t_cfg *cfg = OBJECT_CFG(obj);
  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);

  DiabloBrokerCall ("ObjectFlowgraphBefore", obj);

  /* {{{ mark all force_leader instructions */
  /* {{{ all force_reachable symbols should be force_leader as well */
  if (!force_leader)
    force_leader = force_reachable;
  else if (force_reachable)
  {
    /* add all force_reachable symbols to force_leader */
    int lsize, rsize;
    t_const_string* newleaders;

    for (rsize = 0; force_reachable[rsize]; ++rsize)
    {
    }
    for (lsize = 0; force_leader[lsize]; ++lsize)
    {
    }
    newleaders = Malloc ((lsize + rsize + 1) * sizeof (t_const_string));
    memcpy (newleaders, force_leader, lsize * sizeof (t_const_string));
    memcpy (newleaders + lsize, force_reachable, rsize * sizeof (t_const_string));
    newleaders[lsize + rsize] = NULL;
    force_leader = newleaders;
  } /* }}} */

  if (force_leader)
  {
    for (i = 0; force_leader[i]; ++i)
    {
      t_symbol *sym = SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj), force_leader[i]);

      if (!sym)
      {
        WARNING(("Could not find symbol %s", force_leader[i]));
        continue;
      }
      if (SYMBOL_BASE(sym) && RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_INS)
        INS_SET_ATTRIB(T_INS(SYMBOL_BASE(sym)), INS_ATTRIB(T_INS(SYMBOL_BASE(sym))) | IF_BBL_LEADER);
    }
  }
  /* }}} */

  STATUS(START, ("Flowgraph construction"));
  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    SectionInitFlowgraph(sec);
    SECTION_CALLBACKS(sec)->SectionRecalculateSize =
      SectionRecalculateSizeFlowgraphed;
  }

  ASSERT(desc->Flowgraph, ("TODO port the flowgrapher for this architecture"));
  desc->Flowgraph(obj);

  OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
  {
    SectionFiniFlowgraph(sec);
  }
  STATUS(STOP, ("Flowgraph construction"));

  /* Identify the address producers */
  STATUS(START, ("Detecting address and constant producers"));
  desc->MakeAddressProducers (cfg);
  STATUS(STOP, ("Detecting address and constant producers"));

  /* {{{ add hell edges for force_reachable functions */
  if (force_reachable)
  {
    for (i = 0; force_reachable[i]; ++i)
    {
      t_bbl *block;
      t_ins *ins;
      t_symbol *sym =
        SymbolTableGetSymbolByName (OBJECT_SUB_SYMBOL_TABLE(obj),
                                    force_reachable[i]);

      if (!sym)
      {
        WARNING(("Could not find %s to force it to be reachable", force_reachable[i]));
        continue;
      }

      block = T_BBL(SYMBOL_BASE(sym));
      BBL_FOREACH_INS(block, ins)
      {
        if (AddressIsEq (INS_CADDRESS(ins),
                         AddressAdd (RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)),
                                     SYMBOL_OFFSET_FROM_START(sym))))
        {
          if (ins != BBL_INS_FIRST(block))
          {
            FATAL(("Should not happen: block is forced leader"));
            block = BblSplitBlock (block, ins, TRUE);
          }
          break;
        }
      }

      if (!block || BBL_RELOCATABLE_TYPE(block) != RT_BBL)
        FATAL(("symbol %s does not point to code", force_reachable[i]));
      BBL_SET_ATTRIB(block, BBL_ATTRIB(block) | BBL_FORCE_REACHABLE);
      VERBOSE(1, ("Force reachable @ieB", block));
      if (!IS_DATABBL(block))
      {
        t_bool create_it = TRUE;
        t_cfg_edge *pred;

        BBL_FOREACH_PRED_EDGE(block, pred)
          if (CFG_EDGE_CAT(pred) == ET_CALL
              && CFG_EDGE_HEAD(pred) == CFG_HELL_NODE(cfg))
          {
            create_it = FALSE;
            break;
          }

        if (create_it)
          CfgEdgeCreateCall (cfg, CFG_HELL_NODE(cfg), block, NULL, NULL);
      }
    }
  } /* }}} */

  DiabloBrokerCall("MetaAPI_KeepLive", cfg);

  STATUS(START, ("Processing dynamic symbols"));
  ObjectConnectDynamicSymbols(cfg);
  STATUS(STOP, ("Processing dynamic symbols"));

  /* Find possible functions */
  STATUS(START, ("Function list construction"));
  ret = CfgCreateFunctions (cfg,preserve_functions_by_symbol);
  STATUS(STOP, ("Function list construction"));

  /* Associate basic blocks to a function */
  STATUS(START, ("Association of basic blocks to their functions"));
  CFG_FOREACH_FUN(cfg, function)
    FunctionAssociateBbls (function);
  STATUS(STOP, ("Association of basic blocks to their functions"));

  /* Convert edges to interprocedural version where necessary */
  STATUS(START, ("Patching edges to function representation"));
  ret = CfgPatchInterProcedural (cfg);
  STATUS(STOP, ("Patching edges to function representation"));

  /* Patch the flow graph for setjmp/longjmp anomalies */
  STATUS(START, ("Patching for setjmp/longjmp anomalies"));
  ret = CfgPatchSetJmp (cfg);
  STATUS(STOP, ("Patching for setjmp/longjmp anomalies"));

  STATUS(START, ("Split off jump tables"));
  SeparateJumpTables(cfg);
  STATUS(STOP, ("Split off jump tables"));

  DiabloBrokerCall ("ObjectFlowgraphAfter", obj);
}

static void ComputeCodeSizeGain(t_object * obj)
{/* {{{ compute code size gain */
  double gain, os, ns;
  t_uint32 i;
  t_address osize, nsize;

  osize = nsize = AddressNullForObject(obj);
  for (i = 0; i < OBJECT_NCODES(obj); i++)
  {
    osize = AddressAdd(osize, SECTION_OLD_SIZE(OBJECT_CODE(obj)[i]));
    nsize = AddressAdd(nsize, SECTION_CSIZE(OBJECT_CODE(obj)[i]));
  }
  VERBOSE(0, ("Code section old size @G new size @G", osize, nsize));

  /* Only compute the gain if old size is > 0 (if the section existed) */
  if(osize)
  {
    os = (double) AddressExtractUint64(osize);
    ns = (double) AddressExtractUint64(nsize);
    gain = (os - ns)/os;
    VERBOSE(0, ("GAIN %lf%% in code section", 100.0 * gain));
  }

  for (i = 0; i < OBJECT_NRODATAS(obj); i++)
  {
    osize = AddressAdd(osize, SECTION_OLD_SIZE(OBJECT_RODATA(obj)[i]));
    nsize = AddressAdd(nsize, SECTION_CSIZE(OBJECT_RODATA(obj)[i]));
  }
  for (i = 0; i < OBJECT_NDATAS(obj); i++)
  {
    osize = AddressAdd(osize, SECTION_OLD_SIZE(OBJECT_DATA(obj)[i]));
    nsize = AddressAdd(nsize, SECTION_CSIZE(OBJECT_DATA(obj)[i]));
  }
  for (i = 0; i < OBJECT_NBSSS(obj); i++)
  {
    osize = AddressAdd(osize, SECTION_OLD_SIZE(OBJECT_BSS(obj)[i]));
    nsize = AddressAdd(nsize, SECTION_CSIZE(OBJECT_BSS(obj)[i]));
  }

  /* Only compute the gain if old size is > 0 (if the section existed) */
  if(osize)
  {
    os = (double) AddressExtractUint64(osize);
    ns = (double) AddressExtractUint64(nsize);
    gain = (os - ns)/os;
    VERBOSE(0, ("GAIN %lf%% total", 100.0 * gain));
  }
} /* }}} */

extern int LinkerScriptParserlex_destroy (void );
/* FUN : ObjectDeflowgraph
 * PAR : TODO
 * RET : nothing
 * DESC: TODO */

/* XXX this function is a collection of serious hacks and it actually only
 * works by accident. All kinds of conceptual problems arise when the 
 * deflowgraph algorithm starts creating new sections (as happens with kdiablo).
 * A complete rewrite is necessary, coupled with a rewrite of the section 
 * merging code that is executed before disassembly */
void ObjectDeflowgraph (t_object * obj)
{
  t_uint32 tel;
  const t_architecture_description *desc =
    ObjectGetArchitectureDescription (obj);
  t_section *sec, *unified;
  t_address size;
  t_cfg *cfg = OBJECT_CFG(obj);
  t_bbl *bbl;
  t_ins *ins;

  /* Cleanup: linker scripts are no longer needed so we can free lexer memory */
  LinkerScriptParserlex_destroy();
 
 STATUS(START, ("Deflowgraphing"));


  /* Kill the old code sections, and provide one unified section
   * for the deflowgraphed code. The deflowgraph code itself can add
   * extra sections at its own discretion */
  size = AddressNewForObject(obj, 0);
  
  OBJECT_FOREACH_CODE_SECTION(obj, sec, tel)
  {
    size = AddressAdd(size, SECTION_OLD_SIZE(sec));
  }

  unified =
    SectionCreateForObject(obj, CODE_SECTION, NULL, size, ".text");
  SECTION_SET_TYPE(unified, DEFLOWGRAPHING_CODE_SECTION);
  SECTION_CALLBACKS(unified)->SectionRecalculateSize =
    SectionRecalculateSizeDeflowgraphing;
  SECTION_SET_OLD_SIZE(unified, size);
  SECTION_SET_CADDRESS(unified, SECTION_CADDRESS(OBJECT_CODE(obj)[0]));
  SECTION_SET_ALIGNMENT(unified, SECTION_ALIGNMENT(OBJECT_CODE(obj)[0]));

  OBJECT_FOREACH_CODE_SECTION(obj, sec, tel)
  {
    if (sec == unified)
      continue;

    while (SECTION_SUBSEC_FIRST(sec))
    {
      t_section *sub = SECTION_SUBSEC_FIRST(sec);
      if (SECTION_REFERS_TO(sub))
        FATAL(("relocations coming from @T", sub));
      if (SECTION_REFED_BY(sub))
        FATAL(("relocations pointing to @T", sub));
      SectionKill(sub);
    }

    if (SECTION_REFERS_TO(sec))
      FATAL(("relocations coming from @T", sec));
    while (SECTION_REFED_BY(sec))
    {
      /* migrate to unified section */
      t_uint32 idx;
      t_address offset;
      t_reloc *rel = RELOC_REF_RELOC(SECTION_REFED_BY(sec));
      for (idx = 0; idx < RELOC_N_TO_RELOCATABLES(rel); ++idx)
      {
        if (RELOC_TO_RELOCATABLE(rel)[idx] == T_RELOCATABLE(sec))
          break;
      }

      VERBOSE(1, ("migrating relocation to code section @R", rel));
      RelocSetToRelocatable(rel, idx, T_RELOCATABLE(unified));
      offset = RELOC_TO_RELOCATABLE_OFFSET(rel)[idx];
      offset = AddressAdd(offset,
                          AddressSub(SECTION_CADDRESS(sec),
                                     SECTION_CADDRESS(unified)));
      RELOC_TO_RELOCATABLE_OFFSET(rel)[idx] = offset;
    }

    SectionFree(sec);
  }

  Free(OBJECT_CODE(obj));
  OBJECT_SET_CODE(obj, Malloc(sizeof(t_section *)));
  OBJECT_SET_NCODES(obj, 1);
  OBJECT_CODE(obj)[0] = unified;


  CFG_FOREACH_BBL(cfg, bbl)
  {
    BBL_FOREACH_INS(bbl, ins)
      INS_SET_SECTION(ins, unified);
  }

  DiabloBrokerCall("BeforeDeflowgraph", cfg);
  SetTransformationIdForDeflowgraph();
  DiabloBrokerCall("BeforeDeflowgraphNonTF", cfg);
  desc->Deflowgraph(obj);

  ComputeCodeSizeGain(obj);

  DiabloBrokerCall ("DisableDwarfMemberDuplication");
  OBJECT_FOREACH_CODE_SECTION(obj, sec, tel)
  {
    SectionFiniDeflowgraph (sec);
  }
  DiabloBrokerCall ("AfterDeflowgraph");
  DiabloBrokerCall ("PrintInstructions", cfg);
  
  if (diabloflowgraph_options.origin_tracking
      && diabloflowgraph_options.origin_tracking_final) {
    TrackOriginInformation(cfg, ORIGIN_FINAL_DIRECTORY);
    FinalizeObjectTracking(cfg);
  }

  if (diabloflowgraph_options.final_dots)
    CfgDrawFunctionGraphs(cfg, "final");

  if (cfg)
    CfgFreeData (cfg);

  STATUS(STOP, ("Deflowgraphing"));
}

/* FUN : SectionInitDisassembly
 * PAR :
 * RET : nothing
 * DESC: */

void
SectionInitDisassembly (t_section * section)
{
  t_object *obj = SECTION_OBJECT(section);
  t_cfg *cfg = OBJECT_CFG(obj);
  const t_architecture_description *desc;
  t_address bits_in_section;
  t_uint32 bundlesize;
  t_uint32 max_ins;

  if (!cfg) 
  {
    cfg = CfgCreate (obj);
    DiabloBrokerCall ("CfgCreated", obj, cfg);
  }

  desc = ObjectGetArchitectureDescription (obj);
  bits_in_section = AddressMulUint32 (SECTION_CSIZE(section), 8);
  bundlesize = desc->encoded_instruction_mod_size * desc->bundle_size + desc->template_size;
  max_ins = AddressExtractUint32 (AddressDivUint32 (bits_in_section, bundlesize)) * desc->bundle_size;

  SECTION_SET_TYPE(section, DISASSEMBLING_CODE_SECTION);

  SECTION_SET_ADDRESS_TO_INS_MAP(section, (void **) Calloc (AddressExtractUint32 (SECTION_CSIZE(section)), sizeof (void *)));

  max_ins = MINIMUM(max_ins, 5 * 1024 * 1024);

  VERBOSE(1, ("Max INS: %d", max_ins));

  SECTION_SET_TMP_BUF(section, Calloc (1, desc->decoded_instruction_size * max_ins));
  SECTION_SET_NINS(section, 0);
  SECTION_SET_TMP(section, max_ins);
}

/* FUN : SectionInitFlowgraph
 * PAR :
 * RET :
 * DESC: */

void
SectionInitFlowgraph (t_section * section)
{
  SECTION_SET_TYPE(section, FLOWGRAPHING_CODE_SECTION);
}

void
SectionInitDeflowgraph (t_object * obj, t_section * section)
{
  SECTION_SET_TYPE(section, DEFLOWGRAPHING_CODE_SECTION);
  SECTION_SET_TMP_BUF(section, NULL);
  SECTION_CALLBACKS(section)->SectionRecalculateSize=SectionRecalculateSizeDeflowgraphing;
}

void
SectionFiniDeflowgraph (t_section * section)
{
  /* keep pointer to cfg in tmp_buf */
  SectionToDisassembled (section);
}

/* FUN : SectionFiniFlowgraph
 * PAR :
 * RET :
 * DESC: */

void
SectionFiniFlowgraph (t_section * section)
{
  t_ins *ins;
  t_object *obj = SECTION_OBJECT(section);
  t_architecture_description *desc = ObjectGetArchitectureDescription(obj);

  /* free the linear instruction list */
  SECTION_FOREACH_INS(section, ins)
  {
    InsFreeAllReferedRelocsBase (ins);
    DiabloBrokerCall("FreeStatelistOfIns",ins);
    InsBeforeFree(ins);
    if (desc->InsCleanup) 
      desc->InsCleanup(ins);
  }

  Free (SECTION_DATA(section));
  SECTION_SET_DATA(section, NULL);
  SECTION_SET_TMP_BUF(section, NULL);
  SECTION_SET_TYPE(section, FLOWGRAPHED_CODE_SECTION);
  Free (SECTION_ADDRESS_TO_INS_MAP(section));
  SECTION_SET_ADDRESS_TO_INS_MAP(section, NULL);
}

/* FUN : SectionInitAssembly
 * PAR :
 * RET : nothing
 * DESC: */

#define ceildiv(x,y) ((x)/(y) + ((x)%(y) ? 1 : 0))
#define pad4(x) ((x) % 4 ? (x) + (4 - ((x)%4)) : (x))
void
SectionInitAssembly (t_object * obj, t_section * section, t_uint32 max_ins)
{
  t_uint32 tel;
  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);
  t_uint32 alloc_size =
    pad4 (ceildiv (desc->maximum_encoded_instruction_size, 8) *
          SECTION_NINS(section));

  /* make buffer large enough for any padding that might need to be added */
  alloc_size += 50000;
  /* This happens if we did not flowgraph! */
  if (SECTION_ADDRESS_TO_INS_MAP(section)) 
  {
    Free (SECTION_ADDRESS_TO_INS_MAP(section));
    SECTION_SET_ADDRESS_TO_INS_MAP(section, NULL);
  }
  SECTION_SET_TYPE(section, ASSEMBLING_CODE_SECTION);

  SECTION_SET_TMP_BUF(section, Malloc (alloc_size));
  VERBOSE(0, ("Allocating %d instructions", SECTION_NINS(section)));
  for (tel = 0; tel < alloc_size / 4; tel++)
  {
    *(((int *) (SECTION_TMP_BUF(section))) + tel) = 0xbdb00bdb;
  }
}

/* FUN : SectionFiniDisassembly
 * PAR :
 * RET : nothing
 * DESC: */

void
SectionFiniDisassembly (t_object * obj, t_section * section)
{
  SECTION_SET_TYPE(section, DISASSEMBLED_CODE_SECTION);	/* We're disassembled */
  Free(SECTION_DATA(section));
  SECTION_SET_DATA (section, SECTION_TMP_BUF (section));	/* Copy the instruction data to section data */
  SECTION_SET_TMP_BUF (section, NULL);
  RelocsMigrateToInstructions (obj, section);
}

#undef pad4
#undef ceildiv

/* FUN : SectionFiniAssembly
 * PAR :
 * RET : nothing
 * DESC: */

void
SectionFiniAssembly (t_object * obj, t_section * section)
{
  t_ins *ins;
  t_reloc *rel;
  t_symbol *sym;
  t_section *sub;
  t_string subname = StringConcat2(SECTION_NAME(section), "sub_");
  const t_architecture_description *desc = ObjectGetArchitectureDescription (obj);

  /* remove all old subsections by setting their sizes to 0 */
  SECTION_FOREACH_SUBSECTION(section, sub)
    SECTION_SET_CSIZE(sub, AddressNullForSection(section));

  /* create a subsection corresponding to the code section */
  sub =
    SectionCreateForObject(ObjectGetLinkerSubObject(obj), CODE_SECTION,
                           section, SECTION_CSIZE(section), subname);
  Free(subname);
  memcpy(SECTION_DATA(sub), SECTION_TMP_BUF(section),
         AddressExtractUint32(SECTION_CSIZE(section)));
  SECTION_SET_CADDRESS(sub, SECTION_CADDRESS(section));


  SECTION_SET_TYPE(section, CODE_SECTION); /* We're assembled */

  /*Adapt relocations */
  SECTION_FOREACH_INS(section, ins)
  {
    while (INS_REFERS_TO(ins))
    {
      rel = RELOC_REF_RELOC(INS_REFERS_TO(ins));

      RelocSetFrom (rel, T_RELOCATABLE(sub));
      RELOC_SET_FROM_OFFSET(rel,
        AddressAdd (AddressSub
                    (INS_CADDRESS(ins),
                     SECTION_CADDRESS(INS_SECTION(ins))),
                    RELOC_FROM_OFFSET(rel)));
    }
    while (INS_REFED_BY(ins))
    {
      t_uint32 i;

      rel = RELOC_REF_RELOC(INS_REFED_BY(ins));

      for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
      {
        if (RELOC_TO_RELOCATABLE_REF(rel)[i]==INS_REFED_BY(ins))
        {
          RelocSetToRelocatable (rel, i, T_RELOCATABLE(sub));
          RELOC_TO_RELOCATABLE_OFFSET(rel)[i] =
            AddressAdd (AddressSub
                        (INS_CADDRESS(ins),
                         SECTION_CADDRESS(INS_SECTION(ins))),
                        RELOC_TO_RELOCATABLE_OFFSET(rel)[i]);
        }
      }
    }
    while (INS_REFED_BY_SYM(ins))
    {
      sym = INS_REFED_BY_SYM(ins)->sym;

      SymbolSetBase(sym, T_RELOCATABLE(sub));

      SYMBOL_SET_OFFSET_FROM_START(sym,  AddressAdd (AddressSub (INS_CADDRESS(ins), SECTION_CADDRESS(INS_SECTION(ins))), SYMBOL_OFFSET_FROM_START(sym)));
    }
    
    InsBeforeFree(ins);
    if (desc->InsCleanup) 
      desc->InsCleanup (ins);
  }

  if (SECTION_DATA(section))
    Free (SECTION_DATA(section)); /* Free the instruction data */
  SECTION_SET_DATA(section, SECTION_TMP_BUF(section)); /* Copy the raw data to section data */
  SECTION_SET_TMP_BUF(section, NULL); /* The temporaries are currently not used */
  SECTION_SET_NINS(section, 0);
}

/*}}}*/

/* copy all instructions from the chain of the DEFLOWGRAPHING_SECTION into
 * a linear buffer to create a DISASSEMBLED_SECTION while we're at it, migrate
 * all relocations to the copied instructions
 * {{{ */
#define SECTION_INS(sec,index) ((t_ins*) (((char *)SECTION_TMP_BUF(sec))+(index)*ins_size))
void
SectionToDisassembled (t_section * sec)
{
  t_bbl *chain;
  t_bbl *bbl;
  t_ins *ins;
  const t_architecture_description *desc =
    ObjectGetArchitectureDescription (SECTION_OBJECT(sec));
  size_t ins_size = desc->decoded_instruction_size;
  t_uint32 nins = 0, count;

  if (SECTION_TYPE(sec) != DEFLOWGRAPHING_CODE_SECTION)
    FATAL(("Section %s has type %c", SECTION_NAME(sec), SECTION_TYPE(sec)));

  chain = (t_bbl *)SECTION_TMP_BUF(sec);
  CHAIN_FOREACH_BBL (chain, bbl)
    BBL_FOREACH_INS(bbl, ins)
    nins++;

  SECTION_SET_TMP_BUF(sec, Malloc (ins_size * nins));

  VERBOSE (1, ("To disassembled: %s (size @G address @G)", SECTION_NAME (sec),
               SECTION_CSIZE (sec), SECTION_CADDRESS (sec)));
  count = 0;
  CHAIN_FOREACH_BBL(chain, bbl)
  {
    BBL_FOREACH_INS(bbl, ins)
    {
      t_ins *copy = SECTION_INS(sec, count++);
      /* copy ins */
      INS_SET_COPY(ins, copy);
      InsBeforeDup(ins);
      memcpy (copy, ins, ins_size);
      if (desc->InsDupDynamic) 
      {
        desc->InsDupDynamic (copy, ins);
      }

      /* InsAfterDup might call dynamic member duplicators, that require global_hack_dup_orig to be set */
      global_hack_dup_orig=ins;

      InsAfterDup(ins,copy);
      /* migrate relocations from instruction */
      INS_SET_REFERS_TO(copy, NULL);
      while (INS_REFERS_TO(ins))
      {
        t_reloc *rel = RELOC_REF_RELOC(INS_REFERS_TO(ins));

        RelocSetFrom (rel, (t_relocatable *) copy);
        RELOC_SET_EDGE(rel, NULL);
      }
      INS_SET_REFED_BY(copy, NULL);
      while (INS_REFED_BY(ins))
      {
        t_uint32 i;
        t_reloc *rel = RELOC_REF_RELOC(INS_REFED_BY(ins));
        for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
        {
          if (RELOC_TO_RELOCATABLE_REF(rel)[i]==INS_REFED_BY(ins))
          {
            RelocSetToRelocatable (rel, i, (t_relocatable *) copy);
          }
        }
        RELOC_SET_EDGE(rel, NULL);
      }
      INS_SET_REFED_BY_SYM(copy, NULL);
      while (INS_REFED_BY_SYM(ins))
      {
        t_symbol * sym = INS_REFED_BY_SYM(ins)->sym;

        SymbolSetBase(sym, T_RELOCATABLE(copy));
      }
    }
    /* migrate relocations to bbl */

    if ((BBL_REFED_BY(bbl))||(BBL_REFED_BY_SYM(bbl)))
    {
      if (BBL_INS_FIRST(bbl))
      {
        t_uint32 i;
        t_ins *to = INS_COPY(BBL_INS_FIRST(bbl));

        while (BBL_REFED_BY(bbl))
        {
          t_reloc *rel = RELOC_REF_RELOC(BBL_REFED_BY(bbl));

          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==BBL_REFED_BY(bbl))
            {
	      t_address offset =  AddressSub (INS_CADDRESS(to), BBL_CADDRESS(bbl));
              RelocSetToRelocatable (rel, i, (t_relocatable *) to);
	      if (!AddressIsNull(offset))
	      {
		      FATAL(("Fix me!"));
	      /*
              rel->addend =
                AddressSub (rel->addend,
                            AddressSub (INS_CADDRESS(to),
                                        BBL_CADDRESS(bbl)));
            */
	      }
	    }
          }
          RELOC_SET_EDGE(rel,  NULL);
        }

        while (BBL_REFED_BY_SYM(bbl))
        {
          t_symbol * sym = BBL_REFED_BY_SYM(bbl)->sym;

          SymbolSetBase(sym, T_RELOCATABLE(to));
        }
      }
      else
      {
        /* move relocation to next block with instructions in the chain */
        t_bbl *next = bbl;
        t_bbl *prev = bbl;
        while (!BBL_INS_FIRST(next))
        {
          next = BBL_NEXT_IN_CHAIN(next);
          ASSERT(next, ("should have next in chain"));

          if ((!BBL_SUCC_FIRST(prev)) && (!BBL_REFED_BY(bbl)))
          {
            next = NULL;
            prev = NULL;
            break;
          }
          if (BBL_REFED_BY(bbl))
          ASSERT(BBL_SUCC_FIRST(prev),
                 ("Trying to move relocation @R or symbol pointing to successor of empty bbl @eB, but bbl has no successor", RELOC_REF_RELOC(BBL_REFED_BY(bbl)),bbl));
          else
          ASSERT(BBL_SUCC_FIRST(prev),
                 ("Trying to move relocation or symbol pointing to successor of empty bbl @eB, but bbl has no successor", bbl));
          ASSERT(BBL_SUCC_FIRST(prev) == BBL_SUCC_LAST(prev),
                 ("bbl @eibB should have exactly one successor", bbl));
          if (!(CFG_EDGE_CAT(BBL_SUCC_FIRST(prev)) &
                 (ET_FALLTHROUGH | ET_IPFALLTHRU))) {
            CfgDrawFunctionGraphs(BBL_CFG(prev), "error");
          }
          ASSERT(CFG_EDGE_CAT(BBL_SUCC_FIRST(prev)) &
                 (ET_FALLTHROUGH | ET_IPFALLTHRU),
                 ("should be fallthrough edge: @ieB", prev));
          prev = next;
        }
        while (BBL_REFED_BY(bbl))
        {
          t_uint32 i;
          t_reloc *rel = RELOC_REF_RELOC(BBL_REFED_BY(bbl));
          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==BBL_REFED_BY(bbl))
            {
              RelocSetToRelocatable (rel, i, (t_relocatable *) next);
            }
          }
        }
        while (BBL_REFED_BY_SYM(bbl))
        {
          t_symbol * sym = BBL_REFED_BY_SYM(bbl)->sym;

          if (!next) 
             SymbolTableRemoveSymbol(SYMBOL_SYMBOL_TABLE(sym), sym);
          else
            SymbolSetBase(sym, T_RELOCATABLE(next));
        }
      }
    }
  }
  VERBOSE(0, ("Done: processed %d instructions", count));

  for (count = 0; count < nins; count++)
  {
    INS_SET_INEXT(SECTION_INS(sec, count), SECTION_INS(sec, count + 1));
    INS_SET_IPREV(SECTION_INS(sec, count), SECTION_INS(sec, count - 1));
    INS_SET_BBL(SECTION_INS(sec, count), NULL);
  }
  INS_SET_INEXT(SECTION_INS(sec, nins - 1), NULL);
  INS_SET_IPREV(SECTION_INS(sec, 0), NULL);

  if(SECTION_DATA(sec))
    Free(SECTION_DATA(sec));
  SECTION_SET_DATA(sec, SECTION_TMP_BUF(sec));
  SECTION_SET_TMP_BUF(sec, NULL);
  SECTION_SET_TYPE(sec, DISASSEMBLED_CODE_SECTION);
  SECTION_SET_NINS(sec, nins);
}
#undef SECTION_INS
/* }}} */

/* {{{ SectionCreateDeflowgraphingFromChain */
t_section *
SectionCreateDeflowgraphingFromChain (t_object * obj, t_bbl * chain_head,
                                      t_const_string name)
{
  t_section *sec;
  t_bbl *bbl;
  t_ins *ins;
  t_address size = AddressNullForObject (obj);
  t_uint32 nins;

  sec = SectionCreateForObject (obj, DEFLOWGRAPHING_CODE_SECTION, NULL,
                                size, name);
  SECTION_CALLBACKS (sec)->SectionRecalculateSize = SectionRecalculateSizeDeflowgraphing;
  SECTION_SET_TMP_BUF(sec, chain_head);
  SECTION_SET_DATA (sec, BBL_CFG (chain_head));
  SECTION_SET_CADDRESS (sec, BBL_CADDRESS (chain_head));
  SECTION_SET_OLD_ADDRESS (sec, BBL_OLD_ADDRESS (chain_head));
  
  nins = 0;
  CHAIN_FOREACH_BBL (chain_head, bbl)
  {
    size = AddressAdd (size, BBL_CSIZE(bbl));
    BBL_FOREACH_INS (bbl, ins)
    {
      ++nins;
      INS_SET_SECTION (ins, sec);
    }
  }
  SECTION_SET_OLD_SIZE (sec, size);
  SECTION_SET_CSIZE (sec, size);
  SECTION_SET_NINS (sec, nins);

  return sec;
}

/* }}} */

void
CfgAddEdgeCallback (t_reloc_table * table, t_reloc * rel, void *edge)
{
  t_cfg_edge *cfgedge = (t_cfg_edge *) edge;

  if (CFG_EDGE_CAT(cfgedge) != ET_CALL)
  {
    FATAL(("Implement non calls! @R", rel));
  }
  if (CFG_EDGE_CORR(cfgedge))
    CFG_EDGE_SET_REFCOUNT(CFG_EDGE_CORR(cfgedge), CFG_EDGE_REFCOUNT(CFG_EDGE_CORR(cfgedge)) + 1);
  CFG_EDGE_SET_REFCOUNT(cfgedge, CFG_EDGE_REFCOUNT(cfgedge) + 1);
}

void
CfgDelEdgeCallback (t_reloc_table * table, t_reloc * rel, void *edge)
{
  t_cfg_edge *cfgedge = (t_cfg_edge *) edge;

  if (CFG_EDGE_CORR(cfgedge))
  {
    CfgEdgeKill (CFG_EDGE_CORR(cfgedge));
  }
  CfgEdgeKill (cfgedge);
}

void
CfgDelSwitchEdgeCallback (t_reloc_table * table, t_reloc * rel, void *edge)
{
  t_cfg_edge *cfgedge = (t_cfg_edge *) edge;

  CFG_EDGE_SET_REL(cfgedge, NULL);
}


/* SymbolMigrateToInstruction {{{*/
static void
SymbolMigrateToInstruction(t_object *obj, t_section *code, t_symbol *sym)
{
  t_address generic;
  t_ins *ins_s;
  t_bool migrate;

  migrate=FALSE;
  if (SYMBOL_BASE(sym))
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(SYMBOL_BASE(sym)) == RT_SUBSECTION)
    {
       if (SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE(sym))) == code)
         migrate = TRUE;
    }
    else
    {
      if (T_SECTION(SYMBOL_BASE(sym)) == code)
        migrate = TRUE;
    }
  }

  if (migrate)
  {
    generic = StackExecConst(SYMBOL_CODE(sym), NULL, sym, 0, obj);

    if (AddressIsLt (generic, SECTION_CADDRESS(code))
        || AddressIsGe (generic,
          AddressAdd (SECTION_CADDRESS(code),
            SECTION_CSIZE(code))))
    {
      /* usually, this is a problem, so we should bum out with a
       * FATAL. There is one exception: the symbol might be a debug
       * symbol pointing to the end of the code section. Catch this
       * case and just skip over the symbol */
      if (AddressIsEq
          (generic,
           AddressAdd (SECTION_CADDRESS(code),
             SECTION_CSIZE(code))))
        return;

      FATAL(("trouble in paradise: symbol %s (base name = %s, parent = %s) points out of section SYM(@S):@G=@G+@G SEC:@G+@G=@G OLD ADDRESS @G OLD SIZE @G, OLD SYM @G", SYMBOL_NAME(sym), SECTION_NAME(T_SECTION(SYMBOL_BASE(sym))), SECTION_NAME(code), sym, generic, RELOCATABLE_CADDRESS(SYMBOL_BASE(sym)), SYMBOL_OFFSET_FROM_START(sym), SECTION_CADDRESS(code), SECTION_CSIZE(code), AddressAdd (SECTION_CADDRESS(code), SECTION_CSIZE(code)), SECTION_OLD_ADDRESS(T_SECTION(SYMBOL_BASE(sym))), SECTION_OLD_SIZE(T_SECTION(SYMBOL_BASE(sym))), AddressAdd (SYMBOL_OFFSET_FROM_START(sym), SECTION_OLD_ADDRESS(T_SECTION(SYMBOL_BASE(sym))))));
    }

    ins_s = SecGetInsByAddress (code, generic);

    ASSERT(ins_s, ("Could not find instruction for code address @G in section %s: @G->@G", generic, SECTION_NAME(code), SECTION_CADDRESS(code), AddressAdd(SECTION_CADDRESS(code), SECTION_CSIZE(code))));

    SymbolSetBase(sym, T_RELOCATABLE(ins_s));
    SYMBOL_SET_OFFSET_FROM_START(sym, AddressSub (generic, INS_CADDRESS(ins_s)));
  }
}
/* }}} */

/* RelocsMigrateToInstructions {{{*/
/* We can't have relocations pointing to individual instructions because this would imply
 * that a jump edge (either from HELL or not) goes to the middle of a BBL.
 * Consequently, this makes processing the CFG representation unfeasable because basic blocks
 * are the smallest unit of representation therein. */
void
RelocsMigrateToInstructions (t_object * obj, t_section * code)
{
  t_symbol *sym;
  t_address generic;
  t_ins *ins_s;

  /* migrate all symbols as well, as they can be handy later on for
   * reference purposes (e.g. for instrumentation) */
  SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SUB_SYMBOL_TABLE(obj), sym)
  {
    SymbolMigrateToInstruction(obj,code,sym);
  }

  {
    t_object *sub, *tmp2;
    t_section *sec;
    t_reloc_ref *rr;
    t_reloc *rel;
    t_address address;
    unsigned int i;

    OBJECT_FOREACH_SUBOBJECT(obj, sub, tmp2)
    {
      OBJECT_FOREACH_SECTION(sub, sec, i)
      {
        if (SECTION_PARENT_SECTION(sec) != code)
          continue;

        /* the froms */
        while ((rr = RELOCATABLE_REFERS_TO((t_relocatable *) sec)))
        {
          rel = RELOC_REF_RELOC(rr);
          address = AddressAdd (SECTION_CADDRESS(sec), RELOC_FROM_OFFSET(rel));
          ins_s = SecGetInsByAddress (code, address);
          ASSERT(ins_s, ("Could not find instruction"));
          RelocSetFrom (rel, (t_relocatable *) ins_s);
          RELOC_SET_FROM_OFFSET(rel, AddressSub (address, INS_CADDRESS(ins_s)));
        }

        /* the to's */
        while ((rr = RELOCATABLE_REFED_BY((t_relocatable *) sec)))
        {
	  t_bool found = FALSE;
          t_uint32 i;
          rel = RELOC_REF_RELOC(rr);

          for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
          {
            if (RELOC_TO_RELOCATABLE_REF(rel)[i]==rr)
            {
              address = AddressAdd (SECTION_CADDRESS(sec), RELOC_TO_RELOCATABLE_OFFSET(rel)[i]);
              ins_s = SecGetInsByAddress (code, address);
              if (!ins_s)
              {
                if (AddressIsEq
                    (address,
                     AddressAdd (SECTION_CADDRESS(code),
                                 SECTION_CSIZE(code))))
                {
                  t_ins *ins_iter;
                  /* HACK: try to make it point to the next-to-last instruction,
                   *  the offset will be adjusted so it points right after
                   *  it. This is unsafe in the general case, but since we
                   *  do not split data bbls, it can be done if both source
                   *  and destination lie in the same data bbl.
                   *  This situation occurs in some RVCT binaries with a
                   *  relocation to the __lcnum_c_end symbol in
                   *  c_4.l(lc_numeric_c.o) 
                   */
                  ins_s = SecGetInsByAddress (code, AddressSubInt32(address,1));
                  VERBOSE(1,("During RelocsMigrateToInstructions: Could not find instruction for TO: @T OFFSET @G for reloc @R", RELOC_TO_RELOCATABLE(rel)[i], RELOC_TO_RELOCATABLE_OFFSET(rel)[i],rel));
                  
                  ASSERT(ins_s,("Can't find last instruction of section"));
                  /* Make sure the from and to will be in the same bbl,
                   * because otherwise we cannot guarantee correctness.
                   */
                  ASSERT(RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_INS,("Relocation past end of code section not from an instruction for @R",rel));
		  ASSERT(INS_TYPE(T_INS(RELOC_FROM(rel))) == IT_DATA,("Relocation past end of code section not from an IT_DATA for @R",rel));
                  ASSERT(AddressIsLe(RELOCATABLE_CADDRESS(RELOC_FROM(rel)),INS_CADDRESS(ins_s)),("Relocation past end of code section from instruction after target @R",rel));
		  for (ins_iter = T_INS(RELOC_FROM(rel)); ins_iter && (ins_iter != ins_s); ins_iter = INS_INEXT(ins_iter))
		  {
		    if (INS_TYPE(ins_iter) != IT_DATA)
		      break;
		  }
		  ASSERT(ins_iter == ins_s,("Could not find contiguous data region between from and to of relocation past end of section, so may not end up in a single data bbl @R",rel));
               }
                else
                  FATAL(("During RelocsMigrateToInstructions: Could not find instruction for TO: @T OFFSET @G", RELOC_TO_RELOCATABLE(rel)[i], RELOC_TO_RELOCATABLE_OFFSET(rel)[i]));
              }
              

              RelocSetToRelocatable (rel, i, (t_relocatable *) ins_s);
              RELOC_TO_RELOCATABLE_OFFSET(rel)[i] = AddressSub (address, INS_CADDRESS(ins_s));
	      found = TRUE;
	      break;
            }
          }

	  ASSERT(found, ("Relocations are corrupt!"));
        }
      }
    }
  }

  RelocTableSetCallbacks (OBJECT_RELOC_TABLE(obj), CfgAddEdgeCallback,
                          CfgDelEdgeCallback, CfgDelSwitchEdgeCallback);
}

/*}}} */

void
ObjectRewrite (t_const_string name, int (*func) (t_cfg *), t_const_string oname)
{
  t_object *obj;
  t_cfg *cfg;
  t_const_string objectfilename = name;

  /* Restore a dumped program, it will be loaded by ObjectGet */
  if (diabloobject_options.restore || (diabloobject_options.restore_multi != -1))
  {
    objectfilename = RestoreDumpedProgram ();
  }

  obj = LinkEmulate (objectfilename, FALSE);

  ObjectDisassemble (obj);
  ObjectFlowgraph (obj, NULL, 0, FALSE);
  cfg = OBJECT_CFG(obj);
  func (cfg);

  ObjectDeflowgraph (obj);

  {
    t_uint32 i;
    t_section *sec;
    t_string fname = StringConcat2(oname, ".list");
    FILE *f = fopen(fname, "w");
    Free(fname);
    if (f)
    {
      OBJECT_FOREACH_CODE_SECTION(obj, sec, i)
      {
        t_ins *ins;
        fprintf(f, "========================[%s]========================\n", SECTION_NAME(sec));
        SECTION_FOREACH_INS(sec, ins)
        {
          FileIo(f, "@I\n", ins);
        }
      }
      fclose(f);
    }
  }

  /* rebuild the layout of the data sections
   * so that every subsection sits at it's new address */
  ObjectRebuildSectionsFromSubsections (obj);

  ObjectAssemble (obj);

  {
    t_symbol * symptr;
    for (symptr=SYMBOL_TABLE_FIRST(OBJECT_SUB_SYMBOL_TABLE(obj)); symptr!=NULL; symptr=SYMBOL_NEXT(symptr))
    {
      if (RELOCATABLE_RELOCATABLE_TYPE (SYMBOL_BASE (symptr)) == RT_SUBSECTION)
      {
        SYMBOL_SET_OFFSET_FROM_START(symptr, AddressAdd(SYMBOL_OFFSET_FROM_START(symptr) ,AddressSub (SECTION_CADDRESS(T_SECTION(SYMBOL_BASE (symptr))),SECTION_CADDRESS(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))))));
        SymbolSetBase(symptr, T_RELOCATABLE(SECTION_PARENT_SECTION(T_SECTION(SYMBOL_BASE (symptr)))));
      }
    }
    if (OBJECT_SYMBOL_TABLE(obj)) SymbolTableFree(OBJECT_SYMBOL_TABLE(obj));
    OBJECT_SET_SYMBOL_TABLE(obj, OBJECT_SUB_SYMBOL_TABLE(obj));
    OBJECT_SET_SUB_SYMBOL_TABLE(obj, NULL);
  }



  ObjectWrite (obj, oname);

#ifdef HAVE_STAT
  /* make the file executable */
  chmod (oname,
         S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
         S_IXOTH);
#endif
}

void 
DeflowgraphedModus (t_address *px, t_reloc * rel, t_address * out)
{
  t_address x = *px;
  t_uint32 i;
  t_cfg * cfg = NULL;

  /* Look for ins or bbl */
  for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
  {
     if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_BBL) 
     {
       cfg = BBL_CFG(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]));
     }
     else if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_INS)
     {
       if (INS_BBL(T_INS(RELOC_TO_RELOCATABLE(rel)[i])))
         cfg = BBL_CFG(INS_BBL(T_INS(RELOC_TO_RELOCATABLE(rel)[i])));
       else
         cfg = INS_CFG(T_INS(RELOC_TO_RELOCATABLE(rel)[i]));
     }
  }

  if (!cfg) 
  {
    *out=AddressNullForObject(RELOC_TABLE_OBJECT(RELOC_TABLE(rel)));
  }
  else
  {
    if (CFG_DESCRIPTION(cfg)->Modus)
    {
      *out=CFG_DESCRIPTION(cfg)->Modus(x,rel);
    }
    else
    {
      *out=AddressNullForObject(RELOC_TABLE_OBJECT(RELOC_TABLE(rel)));
    }
  }

}

/* Prints the Diablo listing file showing the original and new instructions. This function
 * is usually called in frontends, after deflowgraphing. The argument output_name here should
 * be the output name of the executable.
 */
void ObjectPrintListing(t_object *obj, t_string output_name)
{
  t_uint32 i;
  t_section *sec;
  t_ins *ins;
  t_string name=StringConcat2(output_name,".list");
  FILE *f = fopen (name, "w");
  ASSERT (f, ("Could not open %s for writing", name));
  int tel;
  t_object * sub_obj, *tmp;
  Free (name);
  OBJECT_FOREACH_CODE_SECTION (obj, sec, i)
  {
    FileIo (f,
        "========================[%s]========================\n",
        SECTION_NAME (sec));
    SECTION_FOREACH_INS (sec, ins)
      FileIo (f, "@pxtfI\n", ins);
  }

  OBJECT_FOREACH_SUBOBJECT(obj, sub_obj, tmp)
    {
      FileIo (f,
              "========================[ object %s ]========================\n",OBJECT_NAME(sub_obj));
      OBJECT_FOREACH_SECTION(sub_obj, sec, tel)
        {
          FileIo (f, "   section %s new 0x%x old 0x%x size %x\n",SECTION_NAME(sec),SECTION_CADDRESS(sec),SECTION_OLD_ADDRESS(sec),SECTION_CSIZE(sec));
        }

    }
  fclose(f);
}
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
