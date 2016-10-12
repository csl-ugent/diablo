/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifdef BIT32ADDRSUPPORT
#include <diabloelf.h>

#define COMPRESSED_SECTION_HEADER_SIZE 24



#define COMPRESSED_SECTION_NODES(data, nnodes, nsyms, nrels)  ((t_compressed_map_node *) (((char *) data) + COMPRESSED_SECTION_HEADER_SIZE))
#define COMPRESSED_SECTION_SYMS(data, nnodes, nsyms, nrels)   ((t_compressed_symbol *)   (((char *) data) + COMPRESSED_SECTION_HEADER_SIZE + nnodes*sizeof(t_compressed_map_node)))
#define COMPRESSED_SECTION_RELS(data, nnodes, nsyms, nrels)   ((t_compressed_reloc *) (((char *) data) + COMPRESSED_SECTION_HEADER_SIZE + nnodes*sizeof(t_compressed_map_node) + nsyms*sizeof(t_compressed_symbol)))
#define COMPRESSED_SECTION_STRTAB(data, nnodes, nsyms, nrels)  (((char *) data) + COMPRESSED_SECTION_HEADER_SIZE + (nnodes*sizeof(t_compressed_map_node)) + (nsyms*sizeof(t_compressed_symbol)) + (nrels*sizeof(t_compressed_reloc)))

t_bool
IsArchCompressedElf32(FILE * fp)
{
  long fpos;
  t_elf_raw32 * raw;
  t_uint32 i;

  if (!IsElf32(fp)) return FALSE;
  
  fpos=ftell(fp);

  raw = ElfRaw32Read(fp);
  
  fseek(fp, fpos, SEEK_SET);

  for (i = 0; i<raw->hdr->e_shnum; i++)
  {
    t_string name = raw->sdatas[raw->hdr->e_shstrndx] + raw->shdrs[i].sh_name;
    if (strcmp(name, ".diablo") == 0) 
    {
      return TRUE;
    }
  }
  
  return FALSE;
}


void 
ArchCompressedElf32Open (FILE * fp, t_archive * ret)
{
  t_uint32 i;
  t_elf_raw32 * raw;
  ret->open_fp = fp;
  raw = ElfRaw32Read(fp);

  for (i = 0; i<raw->hdr->e_shnum; i++)
  {
    t_string name = raw->sdatas[raw->hdr->e_shstrndx] + raw->shdrs[i].sh_name;
    if (strcmp(name, ".diablo") == 0) 
    {
      ret->objects = NULL;
      ret->symbols = NULL;
      ret->data = raw->sdatas[i];
      return;
    }
  }

  FATAL(("Could not find diablo section in archive"));
}

void 
ArchCompressedElf32Close (t_archive * ret)
{
  fclose(ret->open_fp);
}

t_relocatable *
IdxToRelocatable(t_uint32 i, t_object * parent, t_object * obj, t_compressed_map_node * nodes, char * strtab)
{
  t_relocatable * ret;
  if (i == 0)
  {
    ret = T_RELOCATABLE(OBJECT_UNDEF_SECTION(parent));
  }
  else if (i == 1)
  {
    ret=T_RELOCATABLE(OBJECT_ABS_SECTION(parent));
  }
  else
  {
    ret = T_RELOCATABLE(SectionGetFromObjectByName(obj, strtab + nodes[i - 2].sec_name));
  }
  return ret;
}

t_object * 
ArchCompressedElf32GetObject (const t_archive * arch, t_const_string name, t_object * parent, t_bool read_debug)
{
  t_object * ret = ObjectNewCached (StringConcat3(arch->name, ":", name), parent);
  char * cso = arch->data;
  t_uint32 i;
  
  t_uint32 nnodes =  ((t_uint32 * )cso)[0];
  t_uint32 nsyms  =  ((t_uint32 * )cso)[2];
  t_uint32 nrels  =  ((t_uint32 * )cso)[3];
  t_uint32 strtablen =  ((t_uint32 * )cso)[4];

  char * strtab = COMPRESSED_SECTION_STRTAB(cso, nnodes, nsyms, nrels);
  t_compressed_map_node * nodes = COMPRESSED_SECTION_NODES(cso, nnodes, nsyms, nrels);
  t_compressed_symbol * syms = COMPRESSED_SECTION_SYMS(cso, nnodes, nsyms, nrels);
  t_compressed_reloc * rels = COMPRESSED_SECTION_RELS(cso, nnodes, nsyms, nrels);
  
  t_symbol ** symmap = Calloc(sizeof(t_symbol *), nsyms);
  VERBOSE(1, ("Reading file %s", OBJECT_NAME(ret)));

  OBJECT_SET_SYMBOL_TABLE (ret, SymbolTableNew (ret));
  OBJECT_SET_RELOC_TABLE (ret, RelocTableNew (ret));
  OBJECT_SET_OBJECT_HANDLER (ret, OBJECT_OBJECT_HANDLER(parent));

  for (i =0; i<nnodes; i++)
  {
    t_compressed_map_node * cnode = &(nodes[i]);
    t_string object_name = strtab + cnode->obj_name + 2;

    if (strcmp(object_name, name)!=0) continue;
    if (((cnode->type_attribute_size >> 26) & 7) == TypeOther) continue;
    if (cnode->type_attribute_size & (1U << 31U))
    {

      t_section * sec = ObjectAddSectionFromFile (ret, BSS_SECTION, FALSE,
                                arch->open_fp,
                                cnode->offset,
                                AddressNew32(0), AddressNew32(cnode->type_attribute_size & ((1 << 20) - 1 )),
                                (((cnode->type_attribute_size>>20)&0x3f)>0)?AddressNew32(1<<(((cnode->type_attribute_size>>20)&0x3f)-1)):AddressNew32(0),
                                strtab + cnode->sec_name,-1);
      VERBOSE(1, (" A SECTION named %s (size=%d - type =%d)", strtab + cnode->sec_name, cnode->type_attribute_size & ((1 << 20) - 1 ), SECTION_TYPE(sec) ));
    }
    else
    {
      char dtype;

      switch ((cnode->type_attribute_size >> 26) & 7)
      {
        case Code:
          dtype = CODE_SECTION;
          break;
        case Data:
          dtype = DATA_SECTION;
          break;
        case Dbug:
          dtype = DEBUG_SECTION;
          break;
        case TlsData:
          dtype = TLSDATA_SECTION;
          break;
        case TlsBss:
          dtype = TLSBSS_SECTION;
          break;
        default:
          FATAL(("ERROR"));
      }
      
      {
        t_section * sec = ObjectAddSectionFromFile (ret, dtype, (((cnode->type_attribute_size >> 29)&3)) == RO,
                                    arch->open_fp,
                                    cnode->offset,
                                    AddressNew32(0), AddressNew32(cnode->type_attribute_size & ((1 << 20) - 1 )),
                                    (((cnode->type_attribute_size>>20)&0x3f)>0)?AddressNew32(1<<(((cnode->type_attribute_size>>20)&0x3f)-1)):AddressNew32(0),
                                    strtab + cnode->sec_name,-1);
        VERBOSE(1, (" A SECTION named %s (size=%d - type =%d)", strtab + cnode->sec_name, cnode->type_attribute_size & ((1 << 20) - 1 ), SECTION_TYPE(sec) ));
      }
    }
  }

  for (i =0; i<nnodes; i++)
  {
    t_uint32 j;
    t_compressed_map_node * cnode = &(nodes[i]);
    t_string object_name = strtab + cnode->obj_name + 2;
    t_string sec_name = strtab + cnode->sec_name ;

    if (((cnode->type_attribute_size >> 26) & 7) != TypeOther) continue;
    if (strcmp(object_name, name)!=0) continue;
    if (strcmp(sec_name, "SYMBOLS")!=0) continue;

    for (j = cnode->offset; j<=cnode->offset+(cnode->type_attribute_size& ((1 << 20) - 1 )); j++)
    {
      ASSERT(syms[j].name<strtablen, ("Out of strtab read while reading compressed symbol name (name = %x, strtab size =%x)", syms[j].name, strtablen));
      ASSERT(syms[j].code<strtablen, ("Out of strtab read while reading compressed symbol code"));
      ASSERT((syms[j].tentative == 0xffffffff) || (syms[j].tentative<strtablen), ("Out of strtab read while reading compressed tentative name (%x)", syms[j].tentative));

      {
        t_symbol * sym = SymbolTableAddSymbol(OBJECT_SYMBOL_TABLE(ret), strtab + syms[j].name, strtab + syms[j].code, ((t_int32) syms[j].order_dup_search) >> 10, (syms[j].order_dup_search >> 2) &3 , syms[j].order_dup_search &3, IdxToRelocatable(syms[j].base_idx, parent, ret, nodes, strtab), AddressNew32(syms[j].offset_from_start), AddressNew32(syms[j].addend), (syms[j].tentative!=0xffffffff)?strtab + syms[j].tentative:NULL, AddressNew32(syms[j].size), 0);
        SYMBOL_SET_FLAGS(sym, (((t_int32) syms[j].order_dup_search) >> 4) &0xf);
        symmap[j] = sym;
      }
    }
  }

  for (i =0; i<nnodes; i++)
  {
    t_uint32 j;
    t_compressed_map_node * cnode = &(nodes[i]);
    t_string object_name = strtab + cnode->obj_name + 2;
    t_string sec_name = strtab + cnode->sec_name ;

    if (((cnode->type_attribute_size >> 26) & 7) != TypeOther) continue;
    if (strcmp(object_name, name)!=0) continue;
    if (strcmp(sec_name, "RELOCS")!=0) continue;

    for (j = cnode->offset; j<=cnode->offset+(cnode->type_attribute_size& ((1 << 20) - 1 )); j++)
    {
    
      t_relocatable * from = IdxToRelocatable(rels[j].from &0xfffffff, parent, ret, nodes, strtab);

      t_symbol * sym = symmap[rels[j].to];
      t_reloc * rel = RelocTableAddRelocToSymbol (OBJECT_RELOC_TABLE(ret), AddressNew32(rels[j].addend), from, 
                                 AddressNew32(rels[j].from_offset),
                                 sym,
                                 (rels[j].from >>29)&1,
                                 NULL,
                                 NULL,
                                 NULL,
                                 strtab + rels[j].code);

      while ((j+1<nrels)&&(rels[j+1].from == 0xffffffff)) 
      {
        j++;
        RelocAddSymbol(rel, symmap[rels[j].to], AddressNullForObject(parent));
      }
    }
  }

  Free(symmap);

  /* try to find out if the code was handwritten in assembler or created by a compiler */
  {
    t_bool handwritten = TRUE;
    t_symbol *sym = SYMBOL_TABLE_FIRST(OBJECT_SYMBOL_TABLE(ret));
    t_uint32 i;


    while (sym && (!(SYMBOL_FLAGS(sym) & SYMBOL_TYPE_FILE)))
      sym = SYMBOL_NEXT(sym);

    /* file symbol is first absolute symbol */
    if (sym && SYMBOL_NAME(sym))
    {
      char *ext = strrchr (SYMBOL_NAME(sym), '.');

      if (ext)
      {
        ext++;
        if (!strcmp (ext, "c") || !strcmp (ext, "cc")
            || !strcmp (ext, "h") || !strcmp (ext, "C")
            || !strcmp (ext, "cpp") || !strcmp (ext, "hpp"))
          handwritten = FALSE;
      }
    }

    for (i = 0; i < OBJECT_NCODES(ret); i++)
    {
      if (handwritten)
        SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$handwritten", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_CODE(ret)[i]),
                              AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
      else
        SymbolTableAddSymbol (OBJECT_SYMBOL_TABLE(ret), "$compiler", "R00A00+$", -1, TRUE, FALSE, T_RELOCATABLE(OBJECT_CODE(ret)[i]),
                              AddressNullForObject(ret),  AddressNullForObject(ret), NULL, AddressNullForObject(ret), 0);
    }
  }

  DiabloBrokerCall ("AfterObjectRead", ret);
  return ret;
}


static t_string 
PatchName(t_string in)
{
  t_uint32 i;
  t_string_array * sa = StringDivide(in, "@", TRUE, FALSE);
  t_string patched_name = StringArrayJoin(sa, "@@");

  t_string ret;

  StringArrayFree(sa);

  for (i = 0; i< strlen(patched_name); i++)
  {
    if (patched_name[i] == ':') patched_name[i] ='@';
  }

  ret = StringConcat2("@:", patched_name);
  Free(patched_name);
  return ret;
}

static void
AddNode(t_object * obj, void * raw, t_compressed_map_node ** nodes, t_uint32 * nnodes, t_strtab * strtab, char *** object_names, t_uint32 * nobjs, char * objname, char * secname, t_address base_addr, t_address sec_size, t_map_section_type type, t_map_section_attribute attr)
{
  t_uint32 fo;
  t_uint32 before;
  t_string patched_name_full = PatchName(objname);
  t_uint32 i;
  t_uint32 j = 0xffffffff;
  t_object * sobj = ObjectGet(objname, obj, FALSE, NULL);
  t_section * sec = SectionGetFromObjectByName(sobj, secname);

  if ((type == Zero) || (type == TlsBss) || (raw == NULL))
    fo=0xffffffff;
  else
    fo=AddressExtractUint32(OBJECT_OBJECT_HANDLER(obj)->rawaddrtofile(raw, base_addr));

  (*nodes) = Realloc((*nodes), ((*nnodes)+1) * sizeof(t_compressed_map_node));

  (*nodes)[*nnodes].sec_name = StrtabAddString(strtab, secname);
  before = strtab->len;
  (*nodes)[*nnodes].obj_name = StrtabAddString(strtab, patched_name_full);

  if (before != strtab->len)
  {
    ASSERT(object_names, ("AddNode needed to add a new object in a phase where adding new objects is prohibited"));
    (*nobjs)++;
    (*object_names) = Realloc((*object_names), (*nobjs) * sizeof(char *));
    (*object_names)[(*nobjs) - 1] = objname;
  }

  if (sec)
  {

    for (i = 0; i<32; i++)
    {
      if ((1<<i) & AddressExtractUint32(SECTION_ALIGNMENT(sec))) 
      {
        ASSERT(j == 0xffffffff, ("Bad alignment"));
        j = i+1;
      }
    }
    if (j == 0xffffffff) j =0;
  }
  else j = 0;

  /* Bss sections, ... */
  if (fo == 0xffffffff)
  {
    (*nodes)[*nnodes].offset = AddressExtractUint32(base_addr);
    (*nodes)[*nnodes].type_attribute_size = 1U<<31U | (((t_uint32) attr) << 29) | (((t_uint32) type) << 26) | (j <<20) | AddressExtractUint32(sec_size);
  }
  else
  {
    (*nodes)[*nnodes].offset = fo;
    (*nodes)[*nnodes].type_attribute_size = (((t_uint32) attr) << 29) | (((t_uint32) type) << 26) | (j <<20) | AddressExtractUint32(sec_size);
  }
  (*nnodes)++;
  Free(patched_name_full);
}

static t_uint32 
RelocatableToIdx(t_object * obj, t_object * sobj, t_string patched_name_full, t_relocatable * r, t_compressed_map_node ** nodes, t_uint32 * nnodes, t_strtab * strtab)
{
  t_uint32 idx = 0;
  ASSERT(RELOCATABLE_RELOCATABLE_TYPE(r)== RT_SECTION, ("Found symbol, not to section!"));

  if (T_SECTION(r) == OBJECT_UNDEF_SECTION(obj))
  {
    idx = 1;
  }
  else if (T_SECTION(r) == OBJECT_ABS_SECTION(obj))
  {
    idx = 2;
  }
  else
  {
    t_bool found = FALSE;
    t_uint32 x;
    if (SECTION_OBJECT((T_SECTION(r))) != sobj)
      FATAL(("Found a symbol that does not refer to the object in which it is defined"));

    for (x=0; x<*nnodes; x++)
    {
      if (strcmp((strtab->strtab) + (*nodes)[x].obj_name, patched_name_full) ==0)
      {
        if (strcmp((strtab->strtab) + (*nodes)[x].sec_name, SECTION_NAME(T_SECTION(r))) == 0)
        {
          found = TRUE;
          idx = x + 3;
          break;
        }
      }
    }
  }
  return idx;
}

static t_uint32 
AddCReloc(t_object * obj, t_object * sobj, t_string patched_name_full,  t_compressed_map_node ** nodes, t_uint32 *nnodes, t_strtab * strtab, t_reloc * rel, t_compressed_reloc ** rels, t_uint32 * nrels, t_symbol ** map, t_uint32 first_sym, t_uint32 last_sym)
{
  t_uint32 idx = RelocatableToIdx(obj, sobj, patched_name_full, RELOC_FROM(rel), nodes, nnodes, strtab);
 
  t_uint32 i;
  t_uint32 nerels = 0;

  if (idx)
  {
    t_uint32 ntosyms = RELOC_N_TO_SYMBOLS(rel);
    t_uint32 ntorels = RELOC_N_TO_RELOCATABLES(rel);
    t_uint32 cursym = 0;
    (*nrels)++;
    nerels++;

    (*rels) = Realloc(*rels, (*nrels) * sizeof(t_compressed_reloc));

    (*rels)[(*nrels) -1].code = StrtabAddString(strtab, RELOC_CODE(rel));
    (*rels)[(*nrels) -1].from = ((idx -1) | (RELOC_HELL(rel)<<29));
    (*rels)[(*nrels) -1].from_offset = G_T_UINT32(RELOC_FROM_OFFSET(rel));

    if (RELOC_N_ADDENDS(rel)!=1) FATAL(("Implement"));
    if (ntorels!=0) FATAL(("Implement"));

    for (i=first_sym; i<=last_sym; i++)
    {
      if (map[i] == RELOC_TO_SYMBOL(rel)[0])
      {
        break;
      }
    }
    if (i>last_sym) FATAL(("Could not find @S for @R", RELOC_TO_SYMBOL(rel)[0], rel));
    (*rels)[(*nrels) -1].to = i;
    (*rels)[(*nrels) -1].to_offset = G_T_UINT32(RELOC_TO_SYMBOL_OFFSET(rel)[0]);
    (*rels)[(*nrels) -1].addend = G_T_UINT32(RELOC_ADDENDS(rel)[0]);
    


    while (ntosyms>1)
    {
      ntosyms--;
      cursym++;
      (*nrels)++;
      nerels++;
      (*rels) = Realloc(*rels, (*nrels) * sizeof(t_compressed_reloc));
      (*rels)[(*nrels) -1].code = 0xffffffff;
      (*rels)[(*nrels) -1].from = 0xffffffff;
      (*rels)[(*nrels) -1].from_offset = 0xffffffff;

      for (i=first_sym; i<=last_sym; i++)
      {
        if (map[i] == RELOC_TO_SYMBOL(rel)[cursym])
        {
          break;
        }
      }
      if (i>last_sym) FATAL(("Could not find @S", RELOC_TO_SYMBOL(rel)[cursym]));
      (*rels)[(*nrels) -1].to = i;
      (*rels)[(*nrels) -1].to_offset = G_T_UINT32(RELOC_TO_SYMBOL_OFFSET(rel)[cursym]);
      (*rels)[(*nrels) -1].addend = 0xffffffff;
    }
  }
  else
  {
    FATAL(("Could not find %s %s", SECTION_NAME(T_SECTION(RELOC_FROM(rel))), patched_name_full));
  }
  return (*nrels) - nerels;
}

static t_uint32 
AddCSymbol(t_object * obj, t_object * sobj, t_string patched_name_full,  t_compressed_map_node ** nodes, t_uint32 * nnodes, t_strtab * strtab, t_symbol * sym, t_compressed_symbol ** syms, t_uint32 * nsyms, t_symbol *** map)
{

/* t_relocatable *base; 
 * t_address offset_from_start; 
 * t_address addend; 
 * t_address size; 
 * t_string code;
 * t_string tentative; 
 * t_int32 order; 
 * t_tristate dup;
 * t_tristate search; 
 * t_string name; */


  t_uint32 idx = RelocatableToIdx(obj, sobj, patched_name_full, SYMBOL_BASE(sym), nodes, nnodes, strtab);


  if (!idx)
  {
    t_uint32 i;
    t_string copy = StringDup(patched_name_full+2);
    for (i = 0; i< strlen(copy); i++)
      if (copy[i] == '@') copy[i]=':';
    idx = (*nnodes) +3;
    AddNode(obj, NULL, nodes, nnodes, strtab, NULL, NULL, copy, SECTION_NAME(T_SECTION(SYMBOL_BASE(sym))), SECTION_CADDRESS(T_SECTION(SYMBOL_BASE(sym))), SECTION_CSIZE(T_SECTION(SYMBOL_BASE(sym))), Data, RO);
  }

  /* Allocate new symbol */
  (*nsyms)++;
  (*syms) = Realloc((*syms), sizeof(t_compressed_symbol)* (*nsyms));
  (*map) = Realloc((*map), sizeof(t_symbol *)* (*nsyms));


  (*map)[(*nsyms) -1] = sym;
  (*syms)[(*nsyms) - 1].base_idx = idx-1;

  (*syms)[(*nsyms) - 1].offset_from_start = G_T_UINT32(SYMBOL_OFFSET_FROM_START(sym));
  (*syms)[(*nsyms) - 1].addend = G_T_UINT32(SYMBOL_ADDEND(sym));
  (*syms)[(*nsyms) - 1].size = G_T_UINT32(SYMBOL_SIZE(sym));
  (*syms)[(*nsyms) - 1].name = StrtabAddString(strtab, SYMBOL_NAME(sym));

  (*syms)[(*nsyms) - 1].code = StrtabAddString(strtab, SYMBOL_CODE(sym));
  (*syms)[(*nsyms) - 1].order_dup_search = (((t_uint32) SYMBOL_ORDER(sym)) << 10) | (((t_uint32) SYMBOL_FLAGS(sym)) << 4) | (((t_uint32) SYMBOL_DUP(sym)) << 2) | ((t_uint32) SYMBOL_SEARCH(sym));
  
  if (SYMBOL_TENTATIVE(sym)) 
    (*syms)[(*nsyms)-1].tentative = StrtabAddString(strtab, SYMBOL_TENTATIVE(sym));
  else  
    (*syms)[(*nsyms) -1].tentative = 0xffffffffU;

  return (*nsyms)-1;
}


void SaveState(char * inname, char * outname)
{
  t_object * obj;
  /* This array will hold all the map information. It will also be referenced
   * by symbols and reloc */
  t_compressed_map_node * nodes =NULL;
  t_uint32 nnodes = 0;
  t_uint32 nmapnodes = 0;

  t_compressed_symbol * syms = NULL;
  t_uint32 nsyms = 0;

  t_symbol ** map = NULL;

  t_compressed_reloc * rels = NULL;
  t_uint32 nrels = 0;

  t_uint32 nobjs = 0;
  char ** object_names = NULL;

  t_string full_name = FileFind (diabloobject_options.objpath, inname);
  FILE * fpin = fopen(full_name, "r");
  FILE * fpout = fopen(outname,"w");
  void * d;
  char * data;
  t_strtab * strtab = StrtabNew();
  t_uint32 map_handler_name;
  t_uint32 tsize = 0;
  t_uint32 i;
  t_map_node *node, *tmp;

  ASSERT(fpin, ("Could not open %s", inname));

  obj = LinkGetParent (inname, FALSE);

  d=OBJECT_OBJECT_HANDLER(obj)->rawread(fpin);
  LinkObjectsFromMap (obj, FALSE);

  map_handler_name  = StrtabAddString(strtab, OBJECT_MAP(obj)->handler->name);


  DLIST_FOREACH_NODE(OBJECT_MAP(obj), node, tmp)
  {
    t_object * sobj = ObjectGet(node->object, obj, FALSE, NULL);
    t_section * sec = SectionGetFromObjectByName(sobj, node->sec_name);

    if (strcmp(node->sec_name, "COMMON")==0) continue;
    if (sec) 
    {
      switch (SECTION_TYPE(sec))
      {
        case CODE_SECTION:
        case DISASSEMBLING_CODE_SECTION:
        case DISASSEMBLED_CODE_SECTION:
        case FLOWGRAPHING_CODE_SECTION:
        case FLOWGRAPHED_CODE_SECTION:
        case DEFLOWGRAPHING_CODE_SECTION:
        case ASSEMBLING_CODE_SECTION:
          node ->type = Code;
          break;
        case RODATA_SECTION:
        case DATA_SECTION:
          node ->type = Data;
          break;
        case TLSDATA_SECTION:
          node ->type = TlsData;
          node->attr = RW;
          break;
        case BSS_SECTION:
          node ->type = Zero;
          break;
        case TLSBSS_SECTION:
          node -> type = TlsBss;
          node->attr = RW;
          break;
        case NOTE_SECTION:
          continue;
        case DEBUG_SECTION:
        case SPECIAL_SECTION:
        default:
          FATAL(("Implement section type %c", SECTION_TYPE(sec)));
      }
    }
    else if ((StringPatternMatch("*crti.o",node->object)) ||  (StringPatternMatch("*crt1.o",node->object))) continue;
    else FATAL(("Could not find section %s %s", node->sec_name, node->object));

    AddNode(obj, d, &nodes, &nnodes, strtab, &object_names, &nobjs, node->object, node->sec_name, node->base_addr, node->sec_size, node->type, node->attr);
  }

  nmapnodes = nnodes;

  for (i = 0; i<nobjs; i++)
  {
    t_uint32 j;
    t_object * sobj = ObjectGet( object_names[i], obj, FALSE, NULL);
    t_string_array * sa = StringDivide(object_names[i], "@", TRUE, FALSE);
    t_string patched_name = StringArrayJoin(sa, "@@");
    t_string patched_name_full;
    t_reloc * rel;
    t_symbol * sym;
    t_uint32 first =0xffffffff;
    t_uint32 last =0xffffffff;
    t_uint32 first_sym =0xffffffff;
    t_uint32 last_sym =0xffffffff;

    for (j = 0; j< strlen(patched_name); j++)
    {
      if (patched_name[j] == ':') patched_name[j] ='@';
    }

    patched_name_full=StringConcat2("@:", patched_name);
    Free(patched_name);
    StringArrayFree(sa);


    SYMBOL_TABLE_FOREACH_SYMBOL(OBJECT_SYMBOL_TABLE(sobj), sym)
    {
      t_uint32 idx= AddCSymbol(obj, sobj, patched_name_full, &nodes, &nnodes, strtab, sym, &syms, &nsyms, &map);
      if (first_sym == 0xffffffff)
      {
        first_sym =idx;
      }
      last_sym = idx;
    }

    if (first_sym!=0xffffffff) 
      AddNode(obj, d, &nodes, &nnodes, strtab, &object_names, &nobjs, object_names[i], "SYMBOLS", AddressNew32(first_sym), AddressNew32(last_sym-first_sym), TypeOther, RO);

    OBJECT_FOREACH_RELOC(sobj, rel)
    {
      t_uint32 idx= AddCReloc(obj, sobj, patched_name_full, &nodes, &nnodes, strtab, rel, &rels, &nrels, map, first_sym, last_sym);
      if (first == 0xffffffff)
      {
        first =idx;
      }
      last = idx;
    }

    if (first!=0xffffffff) 
      AddNode(obj, d, &nodes, &nnodes, strtab, &object_names, &nobjs, object_names[i], "RELOCS", AddressNew32(first), AddressNew32(last-first), TypeOther, RO); 

    Free(patched_name_full);
  }

  tsize = COMPRESSED_SECTION_HEADER_SIZE+nnodes*sizeof(t_compressed_map_node)+nsyms*sizeof(t_compressed_symbol)+nrels*sizeof(t_compressed_reloc)+strtab->len;
  if (tsize % 1024) tsize = ((tsize/1024)+1) * 1024;

  data = OBJECT_OBJECT_HANDLER(obj)->rawaddsec(d, ".diablo", AddressNew32(tsize));
  ((t_uint32 *) data)[0] = nnodes;
  ((t_uint32 *) data)[1] = nmapnodes;
  ((t_uint32 *) data)[2] = nsyms;
  ((t_uint32 *) data)[3] = nrels;
  ((t_uint32 *) data)[4] = strtab->len;
  ((t_uint32 *) data)[5] = map_handler_name;

  //printf("Size: %d, Last = %d\n", tsize, COMPRESSED_SECTION_STRTAB(data, nnodes, nsyms, nrels) - data + strtab->len);

  memcpy(COMPRESSED_SECTION_NODES (data, nnodes, nsyms, nrels), nodes, nnodes*sizeof(t_compressed_map_node));
  memcpy(COMPRESSED_SECTION_SYMS  (data, nnodes, nsyms, nrels), syms, nsyms*sizeof(t_compressed_symbol));
  memcpy(COMPRESSED_SECTION_RELS  (data, nnodes, nsyms, nrels), rels, nrels*sizeof(t_compressed_reloc));
  memcpy(COMPRESSED_SECTION_STRTAB(data, nnodes, nsyms, nrels), strtab->strtab, strtab->len);
  memset(COMPRESSED_SECTION_STRTAB(data, nnodes, nsyms, nrels)+strtab->len, 0, tsize - (COMPRESSED_SECTION_STRTAB(data, nnodes, nsyms, nrels)+strtab->len - data));
  OBJECT_OBJECT_HANDLER(obj)->rawwrite(d,fpout);
  fclose(fpin);
  fclose(fpout);
  Free(map);
  Free(object_names);
  Free(rels);
  Free(syms);
  Free(nodes);
  StrtabFree(strtab);
}


t_bool
ObjectIsArchCompressedElf32(const t_object * obj)
{
  if (!(OBJECT_COMPRESSED_SUBOBJECTS(obj))) return FALSE;

  if (strcmp(OBJECT_OBJECT_HANDLER(obj)->main_name,"ELF")) return FALSE;

  return TRUE;
}

void 
ArchCompressedElf32ReadMap(const t_object * obj, t_map * map)
{
  FILE * fp = fopen(OBJECT_NAME(obj), "r");
  void * rawobject;
  char * cso = OBJECT_COMPRESSED_SUBOBJECTS(obj);
  t_uint32 i;

  t_uint32 nnodes                  = ((t_uint32 * )cso)[0];
  t_uint32 nmapnodes               = ((t_uint32 * )cso)[1];
  t_uint32 nsyms                   = ((t_uint32 * )cso)[2];
  t_uint32 nrels                   = ((t_uint32 * )cso)[3];
  t_uint32 strtablen               = ((t_uint32 * )cso)[4];
  t_uint32 map_handler_name_offset = ((t_uint32 * )cso)[5];
  
  char * strtab = COMPRESSED_SECTION_STRTAB(cso, nnodes, nsyms, nrels);
  t_compressed_map_node * nodes = COMPRESSED_SECTION_NODES(cso, nnodes, nsyms, nrels);

  ASSERT ((OBJECT_OBJECT_HANDLER(obj)->rawread), ("Map file not found for object %s and included compressed map cannot be decoded because the object handler %s %s does not support raw read", OBJECT_NAME(obj), OBJECT_OBJECT_HANDLER(obj)->main_name, OBJECT_OBJECT_HANDLER(obj)->sub_name));
  ASSERT ((OBJECT_OBJECT_HANDLER(obj)->rawfiletoaddr), ("Map file not found for object %s and included compressed map cannot be decoded because the object handler %s %s does not support raw file pointer to address conversion", OBJECT_NAME(obj), OBJECT_OBJECT_HANDLER(obj)->main_name, OBJECT_OBJECT_HANDLER(obj)->sub_name));

  rawobject = OBJECT_OBJECT_HANDLER(obj)->rawread(fp);

  ASSERT(map_handler_name_offset<strtablen, ("Out of string tab read!"));

  map->handler = MapHandlerGetByName(strtab + map_handler_name_offset);

  ASSERT( map->handler, ("Map handler %s not found!", strtab + map_handler_name_offset));

  for (i =0; i<nmapnodes; i++)
  {
    t_compressed_map_node * cnode = &((nodes)[i]);
    if (((cnode->type_attribute_size >> 26) & 7) != TypeOther)
    {
      t_map_node * node = (t_map_node*) Calloc(1,sizeof(t_map_node));
      if (cnode->type_attribute_size & (1U << 31U))
      {
        ASSERT(cnode->obj_name<strtablen, ("Out of string tab read!"));
        ASSERT(cnode->sec_name<strtablen, ("Out of string tab read!"));
        node->base_addr=AddressNew32(cnode->offset);
      }
      else
      {
        ASSERT(cnode->obj_name<strtablen, ("Out of string tab read!"));
        ASSERT(cnode->sec_name<strtablen, ("Out of string tab read!"));
        node->base_addr=OBJECT_OBJECT_HANDLER(obj)->rawfiletoaddr(rawobject, AddressNew32(cnode->offset));
      }

      node->attr = (cnode->type_attribute_size >> 29) & 3;
      node->type = (cnode->type_attribute_size >> 26) & 7;
      node->object = StringDup(strtab + cnode->obj_name);
      node->sec_name = StringDup(strtab + cnode->sec_name);

      node->sec_size = AddressNew32(cnode->type_attribute_size & ((1 << 20) - 1 ));
      if (strcmp(node->sec_name,".tbss")==0)
      {
        node->mapped_sec_name =StringDup(".tbss");
      }
      else
      {
        node->mapped_sec_name = OBJECT_OBJECT_HANDLER(obj)->rawaddrtosecname(rawobject, node->base_addr);
      }
      ASSERT(node->mapped_sec_name, ("Could not infer the mapped section name @G for %s", node->base_addr, node->sec_name));
      MapInsertNode (map, node);
    }
  }
}
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
