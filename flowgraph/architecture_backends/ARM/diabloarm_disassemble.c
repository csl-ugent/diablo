/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#include <errno.h>

void ArmProcessITBlocks(t_arm_ins * first_ins)
{
  t_uint32 nr_it_blocks = 0;
  t_uint32 nr_it_ins = 0;
  t_uint32 nr_it_ins_total = 0;
  t_arm_ins * current_ins = first_ins;

  VERBOSE(0, ("Processing IT blocks"));

  if(!first_ins)
  {
    VERBOSE(0, ("  first instruction is NULL"));
    return;
  }

  do
  {
    if (ARM_INS_OPCODE(current_ins) == ARM_T2IT)
    {
      /* we have found an IT instruction */
      t_arm_condition_code inversecond, cond;
      t_arm_ins * it_ins, * tmp;
      t_uint32 i;
      t_arm_condition_code insconditions[MAX_INS_IN_IT];
      t_arm_ins_itcond itcond;

      VERBOSE(1, ("  Processing IT block %d @I", nr_it_blocks, current_ins));

      /* construct an array to contain the condition codes for the IT instructions */
      for(i = 0; i < MAX_INS_IN_IT; i++)
        insconditions[i] = ARM_CONDITION_NV;

      /* the two condition codes used in an IT block */
      cond = (ARM_INS_IMMEDIATE(current_ins) & 0x000000f0) >> 4;
      inversecond = cond ^ 1;

      /* the condition code for the first IT-instruction */
      insconditions[0] = cond;

      itcond = ArmInsExtractITCondition(current_ins);
      /* first check that E is not specified when the first condition is AL,
       * only then the inverted condition code is valid.
       */
      switch(itcond)
      {
        case ITCOND_OMIT:
        case ITCOND_T:
        case ITCOND_TT:
        case ITCOND_TTT:
          break;

        default:
          ASSERT(cond != ARM_CONDITION_AL, ("  can't specify ELSE in IT block when first condition is AL"));
      }

      /* now we can safely use the inverted condition code */
      switch(itcond)
      {
        case ITCOND_OMIT:
          break;

        case ITCOND_T:
          insconditions[1] = cond;
          break;

        case ITCOND_E:
          insconditions[1] = inversecond;
          break;

        case ITCOND_TT:
          insconditions[1] = cond;
          insconditions[2] = cond;
          break;

        case ITCOND_ET:
          insconditions[1] = inversecond;
          insconditions[2] = cond;
          break;

        case ITCOND_TE:
          insconditions[1] = cond;
          insconditions[2] = inversecond;
          break;

        case ITCOND_EE:
          insconditions[1] = inversecond;
          insconditions[2] = inversecond;
          break;

        case ITCOND_TTT:
          insconditions[1] = cond;
          insconditions[2] = cond;
          insconditions[3] = cond;
          break;

        case ITCOND_ETT:
          insconditions[1] = inversecond;
          insconditions[2] = cond;
          insconditions[3] = cond;
          break;

        case ITCOND_TET:
          insconditions[1] = cond;
          insconditions[2] = inversecond;
          insconditions[3] = cond;
          break;

        case ITCOND_EET:
          insconditions[1] = inversecond;
          insconditions[2] = inversecond;
          insconditions[3] = cond;
          break;

        case ITCOND_TTE:
          insconditions[1] = cond;
          insconditions[2] = cond;
          insconditions[3] = inversecond;
          break;

        case ITCOND_ETE:
          insconditions[1] = inversecond;
          insconditions[2] = cond;
          insconditions[3] = inversecond;
          break;

        case ITCOND_TEE:
          insconditions[1] = cond;
          insconditions[2] = inversecond;
          insconditions[3] = inversecond;
          break;

        case ITCOND_EEE:
          insconditions[1] = inversecond;
          insconditions[2] = inversecond;
          insconditions[3] = inversecond;
          break;

        default:
          FATAL(("invalid IT condition code %d", itcond));
      }

      /* the condition codes for the IT instructions are stored in insconditions[] */
      tmp = ARM_INS_INEXT(current_ins);

      /* first remove the IT instruction from the instruction listing */
      //      if (global_options.flowgraph)
        ArmInsMakeNoop(current_ins);

      /* then set the appropriate condition codes for every instruction in the IT block */
      nr_it_ins = 0;
      for (i = 0; i < MAX_INS_IN_IT; i++)
      {
        /* an invalid condition code means we have reached the end of the IT block */
        if (insconditions[i] == ARM_CONDITION_NV)
        {
          ASSERT(i != 0, ("can't have IT blocks with 0 instructions"));
          break;
        }

        /* sanity check */
        ASSERT(tmp, ("expected instruction in IT block, but have NULL"));
        ASSERT(ARM_INS_CONDITION(tmp) == ARM_CONDITION_AL, ("can't have conditional instructions in IT blocks"));

        /* set the condition code and the conditional flag */
        ARM_INS_SET_CONDITION(tmp, insconditions[i]);
        if (insconditions[i] != ARM_CONDITION_AL)
          ARM_INS_SET_ATTRIB(tmp, ARM_INS_ATTRIB(tmp)| IF_CONDITIONAL);

        /* recalculate the used/def'ed registers because the status flags are live upon execution */
        ARM_INS_SET_REGS_USE(tmp, ArmUsedRegisters(tmp));
        ARM_INS_SET_REGS_DEF(tmp, ArmDefinedRegisters(tmp));

        /* process the next instruction */
        tmp = ARM_INS_INEXT(tmp);

        nr_it_ins++;
        nr_it_ins_total++;
      }
      VERBOSE(1, ("  IT block %d contained %d instructions", nr_it_blocks, nr_it_ins));

      current_ins = tmp;

      nr_it_blocks++;
    }
    else
    {
      current_ins = ARM_INS_INEXT(current_ins);
    }
  } while(current_ins);

  VERBOSE(0, ("Processed %d IT blocks, containing a total of %d instructions", nr_it_blocks, nr_it_ins_total));
}

/* {{{ Hash table data structures and interface in support of DiabloFlowgraphArmCfgCreated */
typedef struct _t_reloc_he t_reloc_he;

struct _t_reloc_he
{
  t_hash_table_node node;
  t_reloc * reloc; 
};

void * RelocAddressKey(t_address address)
{
  t_address * result = (t_address*) Malloc(sizeof(t_address));
  *result = address;
  return result;
}

static t_uint32
RelocAddressHash(const void * key, const t_hash_table *ht)
{
  t_address a = *(t_address*) key;
  return G_T_UINT32(a) % HASH_TABLE_TSIZE(ht);
}

static t_int32
RelocAddressCmp(const void *addr1, const void *addr2)
{
  t_address a1 = *(t_address*)addr1;
  t_address a2 = *(t_address*)addr2;
  
  /* return 0 if equal */
  return !AddressIsEq(a1,a2);
}

static void 
RelocAddressFree(const void *node, void *null)
{
  t_reloc_he *reloc_node = (t_reloc_he *)node;

  Free(HASH_TABLE_NODE_KEY(&reloc_node->node));
  Free(reloc_node);
}

void FillHashTable(t_hash_table * ht, t_relocatable * relable)
{
  t_address address;
  t_reloc_ref *rr;
  t_reloc *rel;
  t_section *sub;
  t_reloc_he * node;

  if (RELOCATABLE_RELOCATABLE_TYPE(relable)==RT_SECTION)
  {
    SECTION_FOREACH_SUBSECTION(T_SECTION(relable),sub)
      FillHashTable(ht,T_RELOCATABLE(sub));
  }
  else
  {
    rr = RELOCATABLE_REFERS_TO(relable);
    while (rr!=NULL)
    {
      rel = RELOC_REF_RELOC(rr);
      address = AddressAdd (RELOCATABLE_CADDRESS(relable), RELOC_FROM_OFFSET(rel));

      node = (t_reloc_he*) Malloc(sizeof(t_reloc_he));
      node->reloc = rel;
      HASH_TABLE_NODE_SET_KEY(T_HASH_TABLE_NODE(node), RelocAddressKey(address));
      HashTableInsert(ht,node);

      rr=RELOC_REF_NEXT(rr);
    }
  }
}
/*}}}*/

/* {{{ DiabloFlowgraphArmCfgCreated */
void
DiabloFlowgraphArmCfgCreated (t_object *obj, t_cfg *cfg)
{
  t_symbol *sym_it, *syml, *symr;
  t_symbol_table *st;
  t_address sym_it_addr;
  t_section * sec;
  int i;
  /* first create a hash table of relocations per address */

  t_hash_table * ht = HashTableNew(21011,0,RelocAddressHash,RelocAddressCmp,RelocAddressFree);

  OBJECT_FOREACH_SECTION(obj,sec,i)
    FillHashTable(ht,T_RELOCATABLE(sec));

  /* then start the actual work */

  st = OBJECT_SUB_SYMBOL_TABLE (obj);
  ASSERT(st, ("There is no symbol table on the object!"));

  SYMBOL_TABLE_FOREACH_SYMBOL (st, sym_it)
  {
    /* symbol pattern: '$diablo:leb:size:expression' */
    if (StringPatternMatch ("$diablo:*", SYMBOL_NAME (sym_it)))
    {
      t_string expr, symlstr = NULL, symrstr = NULL;
      t_object * secobj = SECTION_OBJECT(T_SECTION(SYMBOL_BASE(sym_it)));
      t_const_string reloc_str;
      t_uint64 data;
      t_reloc *rel;

      /* the .note.ABI-tag section contains manually written local labels ([n]f)
       * which for some reason do not result in a real label in the matching subobject */
      if (StringPatternMatch (".note.ABI*", SECTION_NAME(T_SECTION(SYMBOL_BASE(sym_it)))))
        continue;

      /* TEMP FIX */
      if (StringPatternMatch ("$diablo:*:0", SYMBOL_NAME (sym_it)))
          continue;


      if (StringPatternMatch (".ARM.extab*", SECTION_NAME(T_SECTION(SYMBOL_BASE(sym_it)))))
        {
          t_address symr_addr, syml_addr, orig_offset, new_addend;
          char *str_end = NULL;

          if (!diabloobject_options.keep_exidx) continue; // just don't bother with the extab section


          /* relocatable entries in extab are ULEB128 encoded */

          int length = SymbolDiabloSymbolReadData(sym_it, &data, &expr);

          /* parse expression string */
          t_uint32 value = strtol(expr, &str_end, 0);
          ASSERT(errno != ERANGE, ("out of range error on %s", expr));

          if (str_end == expr)
          {
            /* expression */
            reloc_str = SymbolDiabloSymbolExprToRelocString(expr, &symlstr, &symrstr);
            syml = ObjectGetSymbolByNameAndContext(OBJECT_PARENT(secobj), secobj, symlstr);
            symr = ObjectGetSymbolByNameAndContext(OBJECT_PARENT(secobj), secobj, symrstr);
            symr_addr = StackExecConst(SYMBOL_CODE(symr), NULL, symr, 0, obj);
            syml_addr = StackExecConst(SYMBOL_CODE(syml), NULL, syml, 0, obj);

            char reloc_code[100] = "";

            /* with the following relocation code, we assume that we'll encode the new value in the same
               number of bytes as the original ULEB128 in the original binary. This will fail if 
               binutils was not patched to generate large enough ULEB128 data (i.e., at least 3 bytes), 
               as well as when code would be reordered by Diablo */

            if (StringPatternMatch ("$diablo:u:*",SYMBOL_NAME (sym_it)))
              sprintf(reloc_code, "R00A01+R01A02+-\\e%d\\s0000$", length);
            else 
              sprintf(reloc_code, "R00A01+R01A02+-\\l*w\\s0000$");
              

            new_addend = AddressNew32(0);

            rel = RelocTableAddRelocToRelocatable(
                OBJECT_RELOC_TABLE(obj),
                new_addend,                           // addend 0
                SYMBOL_BASE(sym_it),                  // from
                SYMBOL_OFFSET_FROM_START(sym_it),
                SYMBOL_BASE(syml),                    // to
                SYMBOL_OFFSET_FROM_START(syml),
                TRUE,                                 // HELL
                NULL,
                NULL,
                NULL,                                 // sec
                reloc_code);

            RelocAddRelocatable(rel, SYMBOL_BASE(symr), SYMBOL_OFFSET_FROM_START(symr));

            RelocAddAddend(rel,AddressNew32(0));
            RelocAddAddend(rel,AddressNew32(0));

            t_address address =  StackExecConst(RELOC_CODE(rel), rel, NULL, 0, obj);
            ASSERT(data==G_T_UINT32(address),("OOPS: read uleb data 0x%x does not correspond to computed data 0x@G",data,address));
          }
          else
          {
            /* literal numerical value */
          }

          continue;
        }

      if (StringPatternMatch (".debug*", SECTION_NAME(T_SECTION(SYMBOL_BASE(sym_it)))))
        continue;

      /* If we already had a reloc there is no reason to create one. We create one
       * in order to figure out where a constant originated from. */
      sym_it_addr = StackExecConst(SYMBOL_CODE(sym_it), NULL, sym_it, 0, obj);
      if (HashTableLookup(ht,&sym_it_addr))
        continue;

      SymbolDiabloSymbolReadData(sym_it, &data, &expr);

      /* get our translated expression string and the symbols used in it */
      reloc_str = SymbolDiabloSymbolExprToRelocString(expr, &symlstr, &symrstr);
      syml = ObjectGetSymbolByNameAndContext(OBJECT_PARENT(secobj), secobj, symlstr);
      symr = ObjectGetSymbolByNameAndContext(OBJECT_PARENT(secobj), secobj, symrstr);

      /* we need at least one symbol in the expresson */
      ASSERT (syml != NULL,
          ("Failed to retrieve a symbol (first) found in the $diablo expression: %s",
           expr));

      if (symrstr != NULL)
        ASSERT (symr != NULL,
            ("Failed to retrieve a symbol (second) found in the $diablo expression: %s",
             expr));

      /* really stupid hack, recreating the previous relocation construct for
       * PIC relocations having the whole ldr, add pc, .. construction {{{ */
      if (!((strncmp(reloc_str, "R00A01+R01A02+i00000008+-\\l*w\\s0000$", 36)) 
            && (strncmp(reloc_str, "R00A01+R01A02+i00000004+-\\l*w\\s0000$",36))))
      {
        VERBOSE (4, ("EXCEPT HIT: %s", expr));
        t_address symr_addr, syml_addr, orig_offset, new_addend;
        symr_addr = StackExecConst(SYMBOL_CODE(symr), NULL, symr, 0, obj);
        syml_addr = StackExecConst(SYMBOL_CODE(syml), NULL, syml, 0, obj);

        /* calculate original offset (inverted compared to addend, because the offset
         * is subtracted in the formula above (i00000008/4+-), while the addend  is
         * added in the new formula (A00+))
         */
        orig_offset = AddressSub(AddressSub(syml_addr, symr_addr),AddressNew32(data));
        new_addend = AddressSub(sym_it_addr, AddressAdd(symr_addr, orig_offset));

        rel = RelocTableAddRelocToRelocatable(
            OBJECT_RELOC_TABLE(obj),
            new_addend,                           // addend 0
            SYMBOL_BASE(sym_it),                  // from
            SYMBOL_OFFSET_FROM_START(sym_it),
            SYMBOL_BASE(syml),                    // to
            SYMBOL_OFFSET_FROM_START(syml),
            TRUE,                                 // HELL
            NULL,
            NULL,
            NULL,                                 // sec
            "U00?s0000A00+:R00A01+P-A00+!" "R00A01+M|\\" WRITE_32);

        ASSERT(rel, ("Relocation shouldn't be NULL")); 
        RelocAddAddend(rel,AddressNew32(0));

        VERBOSE(1, ("Handling PIC related $diablo symbol the old fashioned way: %s", expr));
      } else {/*}}}*/
        /* create actual relocation */
        rel = RelocTableAddRelocToRelocatable(
            OBJECT_RELOC_TABLE(obj),
            SYMBOL_ADDEND(syml),                  // addend 01
            SYMBOL_BASE(sym_it),                  // from
            SYMBOL_OFFSET_FROM_START(sym_it),
            SYMBOL_BASE(syml),                    // to
            SYMBOL_OFFSET_FROM_START(syml),
            TRUE,                                 // HELL
            NULL, NULL, NULL,
            reloc_str);

      /* Add second symbol to reloc if available. */
      ASSERT(rel, ("Relocation shouldn't be NULL")); 
      if (symr != NULL)
        RelocAddRelocatable(rel, SYMBOL_BASE(symr), SYMBOL_OFFSET_FROM_START(symr));

      RelocAddAddend(rel,AddressNew32(0));
      RelocAddAddend(rel,AddressNew32(0));
      VERBOSE(1, ("Converting $diablo symbols:\t%s\t --> \t%s", expr, reloc_str));
    }

    /* Free the memory we used TODO get this working */
      Free(reloc_str);
    Free(symlstr);
    Free(symrstr);
    }
  }

  HashTableFree(ht);
}
/*}}}*/

/*!
 * Disassemble an instruction at a given address. Not yet implemented.
 *
 * \param obj
 * \param start
 * \param size_ret
 *
 * \return void *
*/
/* ArmDisassembleOneInstruction {{{ */
void *
ArmDisassembleOneInstruction(t_object * obj, t_address start, int * size_ret)
{
  FATAL(("Implement"));
  return NULL; /* Keep the compiler happy */
}
/* }}} */
/*!
 * Disassemble an instruction at a given offset in a section.
 *
 * \todo parameter size_ret seems unused!
 *
 * \param sec The section in which we want to disassemble something
 * \param offset The offset in the section
 * \param size_ret seems unused!
 *
 * \return void * a void casted t_arm_ins pointer
*/
/* ArmDisassembleOneInstructionForSection {{{ */
void *
ArmDisassembleOneInstructionForSection(t_section * sec, t_uint32 offset, int * size_ret)
{
  t_arm_ins * ret=NULL;
  t_uint32 encoded_instruction;
  t_uint16 i;
  t_address goffset;

  goffset=AddressNew32(offset);
  if (G_T_UINT32(SECTION_CSIZE(sec))>offset)
  {
    ret=Malloc(sizeof(t_arm_ins));
    encoded_instruction=SectionGetData32(sec,goffset);
    /* Find the opcode in the opcode table */
    for (i = 0; (encoded_instruction & arm_opcode_table[i].mask) != arm_opcode_table[i].opcode; i++);
    arm_opcode_table[i].Disassemble(ret,encoded_instruction,i);

    /* Initialize the other fields */
    ARM_INS_SET_IPREV(ret,  NULL);
    ARM_INS_SET_INEXT(ret,  NULL);
    ARM_INS_SET_REGS_USE(ret, ArmUsedRegisters(ret));
    ARM_INS_SET_REGS_DEF(ret, ArmDefinedRegisters(ret));
    ARM_INS_SET_CSIZE(ret, AddressNew32(4));
    ARM_INS_SET_OLD_SIZE(ret, AddressNew32(0));

  }
  return ret;
}
/* }}} */

/** {{{ helper function: determine old address from current address */
static t_address ComputeOldAddress (t_object *obj, t_address current, t_section ** subs, int nsubs)
{
  static int last = 0;
  int i;
  t_bool first;
  t_address offset = AddressNew32 (0), ret;

  if(last>nsubs)
    last = 0;

  /* walk over subsection array, start from where we left off the last time */
  for (i = last, first=TRUE;
      first || (i != last);
      i = (i + 1) % nsubs, first=FALSE)
  {
    offset = AddressSub (current, SECTION_CADDRESS (subs[i]));
    if (AddressIsGe (current, SECTION_CADDRESS (subs[i])) &&
	AddressIsLt (offset, SECTION_CSIZE (subs[i])))
      break;
  }
  if (first || i != last)
  {
    ret = AddressAdd (SECTION_OLD_ADDRESS (subs[i]), offset);
    if (SECTION_FLAGS (subs[i]) & SECTION_FLAG_LINKED_IN)
      ret = AddressNew32 (0xffffffff);
  }
  else
    ret = AddressNew32 (0);
  last = i;
  return ret;
}
/* }}} */
/*!
 * Disassemble an entire section. Creates an array of t_arm_ins's.
 *
 * \param code The section we want to disassemble
 *
 * \return void
*/
/* ArmDisassembleSection {{{ */
void
ArmDisassembleSection(t_section * code)
{
  t_object * obj=SECTION_OBJECT(code);
  int nins = 0;
  t_address offset;
  t_address o_offset;
  t_arm_ins * ins_s=NULL;
  t_arm_ins * prev;
  t_arm_ins * first_ins=NULL;
  t_uint32 data;
  int teller = 0;
  t_uint32 csize = 4;

  t_uint32 nsubs = 0;
  t_section ** subs = NULL;

  {
    char old_type = SECTION_TYPE (code);
    /* hack: trick SectionGetSubsections in returning the subsections,
     * it wouldn't normally do this for DISASSEMBLING sections */
    SECTION_SET_TYPE (code, CODE_SECTION);
    subs = SectionGetSubsections (code, &nsubs);
    SECTION_SET_TYPE (code, old_type);
  }

  offset=AddressNew32(0);
  o_offset=AddressNew32(0);

  prev = NULL;

  while (AddressIsLt (offset, SECTION_CSIZE (code)))
  {
    nins++;
    ins_s = ArmInsNewForSec(code);

    if (first_ins == NULL)
      first_ins = ins_s;

    ARM_INS_SET_CADDRESS(ins_s,  AddressAdd(SECTION_CADDRESS(code),offset));
    ARM_INS_SET_OLD_ADDRESS(ins_s, AddressAdd(SECTION_OLD_ADDRESS(code), offset));

    SECTION_ADDRESS_TO_INS_MAP (code)[teller]=(t_ins*) ins_s;

    if (!ins_s) FATAL(("No instructions!"));

    ArmInsInit(ins_s);

    if (SymbolTableGetCodeType (obj,
	  AddressAdd(SECTION_CADDRESS(code), offset)) == ADDRESS_IS_SMALL_CODE)
    {
      /* {{{ thumb */
      if (SymbolTableGetDataType(obj,
	    AddressAdd(SECTION_CADDRESS(code),offset)) == ADDRESS_IS_DATA)
      {
	/* data in code */
	if ((G_T_UINT32 (AddressAdd (SECTION_CADDRESS(code),offset)) & 0x3) ||
            (SymbolTableGetDataType(obj,
              AddressAddUint32(AddressAdd(SECTION_CADDRESS(code),offset),2)) == ADDRESS_IS_CODE))
	{
	  data=SectionGetData16(code,offset);
	  ThumbDisassembleData(ins_s,(data),TH_DATA);
	  ARM_INS_SET_CSIZE(ins_s, AddressNew32(2));
	  ARM_INS_SET_OLD_SIZE(ins_s, AddressNew32(2));
	}
	else
	{
	  data=SectionGetData32(code,offset);
	  ArmDisassembleData(ins_s,(data),TH_DATA);
	  ARM_INS_SET_CSIZE(ins_s, AddressNew32(4));
	  ARM_INS_SET_OLD_SIZE(ins_s, AddressNew32(4));
	}
      }
      else
      {
        /* Thumb instruction */
        csize = 2;
        data=SectionGetData16(code,offset);

        if (((data & 0xf800) == 0xe800) ||
            ((data & 0xf000) == 0xf000))
        {
          /* this is a 32-bit Thumb instruction */
          data = SectionGetData32By16bit(code, offset);
          csize = 4;
        }

        ArmDisassembleEncoded(T_INS(ins_s), data, TRUE);

        ARM_INS_SET_REGS_USE(ins_s, ArmUsedRegisters(ins_s));
        ARM_INS_SET_REGS_DEF(ins_s, ArmDefinedRegisters(ins_s));
        ARM_INS_SET_CSIZE(ins_s, AddressNew32(csize));
        ARM_INS_SET_OLD_SIZE(ins_s, AddressNew32(csize));

        ARM_INS_SET_FLAGS(ins_s,    ARM_INS_FLAGS(ins_s)| FL_THUMB);
      }
      /* }}} */
    }
    else
    {
      /* {{{ arm code */
      ASSERT (!(G_T_UINT32(AddressAdd(SECTION_CADDRESS(code), offset)) & 0x3),
	  ("Instruction in arm mode not aligned"));

      t_bool ignored = FALSE;
      data=SectionGetData32Ignore(code,offset, &ignored);
      if (ignored) WARNING(("out of section read @T at @G, this has to do with $a, $t and $d symbols!", code, offset));

      if (SymbolTableGetDataType(obj, AddressAdd(SECTION_CADDRESS(code),offset))
	  == ADDRESS_IS_DATA)
      {
	ArmDisassembleData(ins_s,(data),ARM_DATA);
	ARM_INS_SET_CSIZE(ins_s, AddressNew32(4));
	ARM_INS_SET_OLD_SIZE(ins_s, AddressNew32(4));
      }
      else
      {
        ArmDisassembleEncoded(T_INS(ins_s), data, FALSE);
  

  ARM_INS_SET_REGS_USE(ins_s, ArmUsedRegisters(ins_s));
  ARM_INS_SET_REGS_DEF(ins_s, ArmDefinedRegisters(ins_s));
  if (ARM_INS_CONDITION(ins_s)!=ARM_CONDITION_AL)
    ARM_INS_SET_ATTRIB(ins_s,    ARM_INS_ATTRIB(ins_s)| IF_CONDITIONAL);
  if (RegsetIn(ARM_INS_REGS_DEF(ins_s),ARM_REG_R15)) 
    ARM_INS_SET_ATTRIB(ins_s,  ARM_INS_ATTRIB(ins_s)| IF_PCDEF);
  ARM_INS_SET_CSIZE(ins_s, AddressNew32(4));
  ARM_INS_SET_OLD_SIZE(ins_s, AddressNew32(4));

      }
      /* }}} */
    }

    ARM_INS_SET_IPREV(ins_s,  prev);
    if (prev) ARM_INS_SET_INEXT(prev,  ins_s);
    prev=ins_s;

    if (G_T_UINT32(ARM_INS_CSIZE(ins_s)) == 4)
    {
      offset=AddressAddUint32(offset,4);
      o_offset=AddressAddUint32(o_offset,4);
      teller+=4;
    }
    else if (G_T_UINT32(ARM_INS_CSIZE(ins_s)) == 2)
    {
      offset=AddressAddUint32(offset,2);
      o_offset=AddressAddUint32(o_offset,2);
      teller+=2;
    }
    else FATAL(("Impossible CSIZE for instructions!"));
  }
  ARM_INS_SET_INEXT(ins_s,  NULL);

  if (subs) Free (subs);

  VERBOSE(0,("Doing IT-block specific checks"));
  ArmInsSetITSpecifics(first_ins);
  ArmProcessITBlocks(first_ins);

  VERBOSE(0,("Disassembled %d instructions",nins));
}

t_reg NEON_VD_QD(t_uint32 instr)
{
  /* value extraced from instruction equals the number of the D-register targeted,
   * both for Double (D) and Quad (Q) precision registers
   */
  t_uint32 r = ((((instr) & 0x00400000) >> 18) | (((instr) & 0x0000f000) >> 12));

  if(r >= 16)
  {
    /* Only 32 S-registers exist, also, 32 D-registers exist.
     * This means that 16 D-registers (D16-D31) are not mapped to S-registers.
     * As such, they are defined as separate constants in diabloarm_registers.h.
     * We need to take care of this inconvenience...
     */
    r += ARM_REG_D16 - 16;
  }
  else
  {
    /* Convert mapping to S-registers (the even ones); every D-register consists of 2 S-registers. */
    r = ARM_REG_S0 + 2*r;
  }

  return r;
}

t_reg NEON_VN_QD(t_uint32 instr)
{
  t_uint32 r = ((((instr) & 0x00000080) >>  3) | (((instr) & 0x000f0000) >> 16));

  if(r >= 16)
  {
    r += ARM_REG_D16 - 16;
  }
  else
  {
    r = ARM_REG_S0 + 2*r;
  }

  return r;
}

t_reg NEON_VM_QD(t_uint32 instr)
{
  t_uint32 r = ((((instr) & 0x00000020) >>  1) | (((instr) & 0x0000000f)      ));

  if(r >= 16)
  {
    r += ARM_REG_D16 - 16;
  }
  else
  {
    r = ARM_REG_S0 + 2*r;
  }

  return r;
}

void ArmDisassembleEncoded(t_ins * ins_s, t_uint32 data, t_bool is_thumb)
{
  t_arm_ins * ins = (t_arm_ins *)ins_s;
  int i;

  if (is_thumb)
  {
    /* disassemble Thumb instruction */
    for (i = 0; (data & thumb_opcode_table[i].mask) != thumb_opcode_table[i].opcode; i++) {}
    thumb_opcode_table[i].Disassemble(ins, data, i);
  }
  else
  {
    /* disassemble ARM instruction */
    for (i = 0; (data & arm_opcode_table[i].mask) != arm_opcode_table[i].opcode; i++) {}
    arm_opcode_table[i].Disassemble(ins, data, i);
  }
}
/* }}} */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker: */
