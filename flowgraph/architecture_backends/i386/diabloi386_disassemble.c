/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>

/** {{{ Parse instruction prefixes */
static t_uint32 HandlePrefixes(t_i386_ins * ins, t_uint8 * codep)
{
  char * run = ((char *) codep);
  I386_INS_SET_PREFIXES(ins, 0);
  while (1)
  {
    switch (*run)
    {
      case '\xf0':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_LOCK);
	break;

      case '\xf2':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_REPNZ);
	break;
      case '\xf3':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_REP);
	break;

      case '\x2e':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_CS_OVERRIDE);
	break;
      case '\x36':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_SS_OVERRIDE);
	break;
      case '\x3e':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_DS_OVERRIDE);
	break;
      case '\x26':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_ES_OVERRIDE);
	break;
      case '\x64':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_FS_OVERRIDE);
	break;
      case '\x65':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_GS_OVERRIDE);
	break;

      case '\x66':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_OPSIZE_OVERRIDE);
	break;
      case '\x67':
	I386_INS_SET_PREFIXES(ins, I386_INS_PREFIXES(ins) | I386_PREFIX_ADDRSIZE_OVERRIDE);
	break;

      default:
	return (run - ((char *) codep));
    }
    run++;
  }

  /* keep the compiler happy */
  return 0;
} /* }}} */

/** {{{ get opcode and ModR/M byte for floating point instructions */
static t_uint32 DoFloat(t_i386_ins * ins, t_uint8 * codep, t_uint32 * modrm_ret, t_uint32 * sib_ret, t_i386_opcode_entry ** disentry_ret)
{
  t_uint8 * run = codep;
  t_uint32 modrm, sib;
  t_uint32 ext;
  t_i386_opcode opc;

  modrm = run[1];
  ext = (modrm >> 3) & 7;

  if ((modrm >> 6) == 3)
  {
    /* register operations */
    opc = dis386_fpu_reg[*run - 0xd8][ext][modrm & 7].opcode;
    *disentry_ret = &(dis386_fpu_reg[*run - 0xd8][ext][modrm & 7]);
    sib = -1;
  }
  else
  {
    /* memory operations */
    opc = dis386_fpu_mem[*run - 0xd8][ext].opcode;
   *disentry_ret = &(dis386_fpu_mem[*run - 0xd8][ext]);
    if ((modrm & 7) == 4)
      sib = *(run + 2);
    else
      sib = -1;
  }

  I386_INS_SET_OPCODE(ins, opc);
  *modrm_ret = modrm;
  *sib_ret = sib;
  
  if (sib == -1)
    return 2;
  else
    return 3;
} /* }}} */

/** {{{ Get opcode, ModR/M and SIB byte for a non-fpu instruction */
static t_uint32 GetOpcodeModRMAndSIB(t_i386_ins * ins, t_uint8 * codep, t_uint32 * modrm_ret, t_uint32 * sib_ret, t_i386_opcode_entry ** disentry_ret)
{
  t_uint8 * run = (t_uint8 *)codep;
  t_i386_opcode opc = dis386[*run].opcode;
  t_bool has_modrm = dis386[*run].has_modrm;
  t_uint32 modrm = -1, sib = -1;
  t_bool sib_was_assigned = FALSE;

  static t_bool seen_ins_with_prefixes;
  static t_bool seen_ins_3dnow;

  *disentry_ret = &(dis386[*run]);

  run++;


  /* floating point instructions are somewhat more complex. they are handled
   * in a separate function */
  if (opc == FPU_ESC)
    return DoFloat(ins,codep,modrm_ret,sib_ret,disentry_ret);

  if (opc == TWOBYTE_ESC)
  {
    opc = dis386_twobyte[*run].opcode;
    has_modrm = dis386_twobyte[*run].has_modrm;
    *disentry_ret = &(dis386_twobyte[*run]);
    run++;
  }

  if (has_modrm)
  {
    modrm = (t_uint32) *run;
    run++;
  }

 indirection:

  if (opc < 0)
  {

    t_uint32 ext;

    if (opc >= PREFIX_GRP_1)
      {
	/* it is one of the groups based on prefixes */
	if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_SD_OPERANDS))
	  {
	    ext = 0;
	  }
	else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_SS_OPERANDS))
	  {
	    ext = 1;
	  }
	else if (I386_INS_HAS_PREFIX(ins,I386_PREFIX_PS_OPERANDS))
	  {
	    ext = 3;
	  }
	else
	  {
	    ext = 2;
	  }

	if (!seen_ins_with_prefixes)
	  {
	    seen_ins_with_prefixes = TRUE;
	    printf(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! be careful, ins with prefixes are only supported for allowing disassembly\n");
	  }
	/*	printf("%d %d %d\n",opc,opc - GRP1a,ext); */

	*disentry_ret = &(dis386_grps[opc - GRP1a][ext]);
	opc = dis386_grps[opc - GRP1a][ext].opcode;

	/*	printf("%d\n",opc); */

      }
    
    else if (opc == I386_3DNOW_OPC)
      {
	t_uint32 mod, rm;
	if (dis386_3dnow[opc].has_modrm)
	  modrm = (t_uint32) *run;
	mod = (modrm >> 6) & 3; 
	rm = modrm & 7;

	if (!seen_ins_3dnow)
	  {
	    printf(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! be careful, 3d now is only supported for disassembling\n");
	    seen_ins_3dnow = TRUE;
	  }

	/*	printf("modrm %x\n", modrm); */
	/*	printf("3dnow %x %x %x %x %x %x %x %x \n",*run,*(run+1),*(run+2),*(run+3),*(run+4),*(run+5),*(run+6),*(run+7)); */
	if (mod == 3) 
	  {
	    /* register */
	    opc = *(run+1);	
	  }
	else if (mod == 2)
	  {
	    if (rm==4)
	      {
		sib_was_assigned = TRUE;
		sib = (t_uint32) *(run+1);
		opc = *(run+6);
		run++;
	      }
	    else
	      opc = *(run+5);	
	  }
	else if (mod == 1)
	  {
	    if (rm==4)
	      {
		sib_was_assigned = TRUE;
		sib = (t_uint32) *(run+1);
		opc = *(run+3);
		run++;
	      }
	    else
	      opc = *(run+2);	
	  }
	else if (mod == 0)
	  {
	    /*	    printf("rm = %x\n",rm); */
	    if (rm==5)
	      opc = *(run+5);
	    else if (rm==4)
	      {
		sib_was_assigned = TRUE;
		sib = (t_uint32) *(run+1);
		opc = *(run+2);
		run++;
	      }
	    else
	      opc = *(run+1);
	  }
	
	
	*disentry_ret = &(dis386_3dnow[opc]);
	opc = dis386_3dnow[opc].opcode;
	has_modrm = FALSE;
	/*	printf("3dnow %x %x %x %x\n",*run,*(run+1),*(run+2),*(run+3));*/
	run++;
	run++;
      }
    else
      {
	/* we haven't determined a precise opcode yet.
	 * we'll have to look at the opcode extension bits in the modrm byte */
	
	if (opc == GRP15a && ((modrm & 0xc0) == 0xc0))
	  {
	    opc++; /* SPECIAL CASE: group 15 */
	  }

	ext  = (modrm & 0x38) >> 3;

	/* sanity check: if there is no modrm byte something is definitely wrong */
	ASSERT(has_modrm,("Disassembly tables corrupt!"));
	
	*disentry_ret = &(dis386_grps[opc - GRP1a][ext]);
	opc = dis386_grps[opc - GRP1a][ext].opcode;
      }    
  }

  if (opc < 0)
    goto indirection;

  /* if mod != 11b && rm == 100b && addressing mode is 32-bit, we have an sib
   * byte */
  if (!sib_was_assigned && has_modrm && !I386_ADSZPREF(ins) && ((modrm >> 6) != 3) && ((modrm & 7) == 4))
  {
    sib = (t_uint32) *run;
    run++;
  }
  
  if(opc == UNSUPPORTED_OPC){
    FATAL(("Unsupported Instruction at %x",I386_INS_OLD_ADDRESS(ins)));
  }
	     
  I386_INS_SET_OPCODE(ins, opc);

  /*  printf("opcode %d\n",opc); */

  *modrm_ret = modrm;
  *sib_ret = sib;

  /*  printf("modrm %x sib %x\n",modrm,sib); */

  return (run - ((t_uint8 *)codep));
} /* }}} */

/** {{{ set condition field for conditional instructions */
static void SetConditionField(t_i386_ins * ins,t_uint8 * orig_opcode)
{
  switch (I386_INS_OPCODE(ins))
  {
    case I386_Jcc:
    case I386_SETcc:
    case I386_CMOVcc:
      if (orig_opcode[0] == 0x0f)
      {
	/* two-byte opcode: four last bits of second opcode byte */
	I386_INS_SET_CONDITION(ins, ((t_uint32) orig_opcode[1]) & 0xf);
      }
      else
      {
         /* one-byte opcode: four last bits of the first opcode byte */
	  I386_INS_SET_CONDITION(ins, ((t_uint32) orig_opcode[0]) & 0xf);
      }
      break;

    case I386_FCMOVcc:
      {
	t_uint32 selector = ((orig_opcode[0] & 0x1) << 2) + ((orig_opcode[1] & 0x18) >> 3);
	switch (selector)
	{
	  case 0:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_B);
	    break;
	  case 1:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_Z);
	    break;
	  case 2:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_BE);
	    break;
	  case 3:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_P);
	    break;
	  case 4:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_AE);
	    break;
	  case 5:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_NZ);
	    break;
	  case 6:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_A);
	      ;
	    break;
	  case 7:
	    I386_INS_SET_CONDITION(ins, I386_CONDITION_NP);
	    break;
	}
      }
      break;
    case I386_LOOPNZ:
      I386_INS_SET_CONDITION(ins, I386_CONDITION_LOOPNZ);
      break;
    case I386_LOOPZ:
      I386_INS_SET_CONDITION(ins, I386_CONDITION_LOOPZ);
      break;
    case I386_LOOP:
      I386_INS_SET_CONDITION(ins, I386_CONDITION_LOOP);
      break;
    case I386_JECXZ:
      I386_INS_SET_CONDITION(ins, I386_CONDITION_ECXZ);
      break;
    default:
      I386_INS_SET_CONDITION(ins, I386_CONDITION_NONE);
  }

  if (I386_INS_CONDITION(ins) != I386_CONDITION_NONE)
    I386_INS_SET_ATTRIB(ins,  I386_INS_ATTRIB(ins) | IF_CONDITIONAL);

} /* }}} */

/* {{{ set the instruction type field */
static void SetInsType(t_i386_ins * ins)
{
  I386_INS_SET_TYPE(ins, i386_opcode_table[I386_INS_OPCODE(ins)].type);
} /* }}} */

/** {{{ helper function: sort the refers_to of a relocatable on increasing offset */
static int __rel_cmp(const void * a, const void * b)
{
  t_reloc * rela = *(t_reloc **)a;
  t_reloc * relb = *(t_reloc **)b;
  t_address addra = AddressAdd(RELOCATABLE_CADDRESS(RELOC_FROM(rela)), RELOC_FROM_OFFSET(rela));
  t_address addrb = AddressAdd(RELOCATABLE_CADDRESS(RELOC_FROM(relb)), RELOC_FROM_OFFSET(relb));
  return AddressExtractInt32(AddressSub(addra,addrb));
}

static t_reloc ** SortRefersTo(t_section * code, int * nrels_ret)
{
  int nrels,i;
  t_reloc ** rels;
  t_reloc * rel;

  nrels = 0;
  OBJECT_FOREACH_RELOC(SECTION_OBJECT(code),rel)
    if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_SUBSECTION) && (SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel))) == code))
      nrels++;
  
  rels = Malloc(nrels*sizeof(t_reloc *));
  i = 0;
  OBJECT_FOREACH_RELOC(SECTION_OBJECT(code),rel)
    if ((RELOCATABLE_RELOCATABLE_TYPE(RELOC_FROM(rel)) == RT_SUBSECTION) && (SECTION_PARENT_SECTION(T_SECTION(RELOC_FROM(rel))) == code))
      rels[i++] = rel;

  diablo_stable_sort(rels,nrels,sizeof(t_reloc *),__rel_cmp);

  *nrels_ret = nrels;
  return rels;
}
/* }}} */

/** {{{ disassemble one  instruction */
#define UpdateCurrentAndRun()   run += len; current = AddressAddUint32(current,len)

#define CheckOpRelocated(op)    if ((curr_rel < max_rel) && AddressIsEq(current,AddressAdd(RELOC_FROM_OFFSET(rels[curr_rel]),RELOCATABLE_CADDRESS(RELOC_FROM(rels[curr_rel])))) && (len > 0)) \
                                { \
				  I386_OP_FLAGS(op) |= I386_OPFLAG_ISRELOCATED; \
				  curr_rel++; \
				}
#define CheckNoRelocsBefore()   if ((curr_rel < max_rel) && AddressIsGt(current,AddressAdd(rels[curr_rel]->from_offset,RELOC_FROM(rels[curr_rel])->caddress))) \
                                  FATAL(("Relocs in the non-operand parts of instructions: @R from offset @G base @G current @G",rels[curr_rel], \
					 rels[curr_rel]->from_offset, RELOC_FROM(rels[curr_rel])->caddress,current));

t_uint32 I386DisassembleOne(t_i386_ins * ins, t_uint8 * codep, t_reloc ** rels, int max_rel, int curr_rel)
{
  t_uint32 len,prefix_len;
  t_uint8 * run;
  t_uint32 modrm = -1, sib = -1;
  t_i386_opcode_entry * disentry;
  t_address current;
  run = codep;
  current = I386_INS_CADDRESS(ins);

  /*  DiabloPrint(stdout,"address @G\n",current); */
  I386_INS_SET_DEST(ins, Calloc(1,sizeof(t_i386_operand)));
  I386_INS_SET_SOURCE1(ins, Calloc(1,sizeof(t_i386_operand)));
  I386_INS_SET_SOURCE2(ins, Calloc(1,sizeof(t_i386_operand)));
  
  /* handle instruction prefixes */
  len = prefix_len = HandlePrefixes(ins,run);
  UpdateCurrentAndRun();
  
  /* determine opcode bytes + modr/m and sib bytes */
  len = GetOpcodeModRMAndSIB(ins,run,&modrm,&sib,&disentry);
  UpdateCurrentAndRun();

  /* disassemble the instruction operands */
  I386_INS_SET_FLAGS(ins,0);
  /*  CheckNoRelocsBefore();*/ /* TODO: terug aanzetten en fixen voor 3DNOW,  BJORN */
  /* 1. the destination operand */
  len = disentry->op1dis(ins,run,modrm,sib,I386_INS_DEST(ins),disentry->op1bm);
  CheckOpRelocated(I386_INS_DEST(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & DU)
    I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_DEST_IS_SOURCE);

  /* 2. the first source operand */
  len = disentry->op2dis(ins,run,modrm,sib,I386_INS_SOURCE1(ins),disentry->op2bm);
  CheckOpRelocated(I386_INS_SOURCE1(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & S1D)
    I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_SOURCE1_DEF);

  /* 3. the second source operand */
  len = disentry->op3dis(ins,run,modrm,sib,I386_INS_SOURCE2(ins),disentry->op3bm);
  CheckOpRelocated(I386_INS_SOURCE2(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & S2D)
    I386_INS_SET_FLAGS(ins, I386_INS_FLAGS(ins) | I386_IF_SOURCE2_DEF);
  
  SetConditionField(ins,(t_uint8 *)codep+prefix_len);
  SetInsType(ins);
  I386_INS_SET_REGS_USE(ins,  I386InsUsedRegisters(ins));
  I386_INS_SET_REGS_DEF(ins,  I386InsDefinedRegisters(ins));

#ifndef EXTENDED_CONSTPROP
  /* set the fast_eval flag for constant propagation */
  if (I386InsIsConditional(ins)   /* if the status bits are known, but not all
				     source operands, it's still possible to
				     decide the instruction has no effect */
      || I386_INS_OPCODE(ins) == I386_XOR	/* for xor %eax,%eax */
      || I386_INS_OPCODE(ins) == I386_OR	/* for or  $0xffffffff,%eax */
      || I386_INS_OPCODE(ins) == I386_AND	/* for and $0,%eax */
      || I386_INS_OPCODE(ins) == I386_SBB	/* for sbb %eax,%eax */
      || I386InsIsLoad(ins)			/* for known constants */
      || I386InsIsStore(ins)			/* for argument definitions */
     )
    I386_INS_SET_ATTRIB(ins, I386_INS_ATTRIB(ins) & (~IF_FAST_CP_EVAL));
  else
    I386_INS_SET_ATTRIB(ins, I386_INS_ATTRIB(ins)| IF_FAST_CP_EVAL);
#else
  INS_SET_ATTRIB(ins,  INS_ATTRIB(ins) & (~IF_FAST_CP_EVAL));
#endif
  
  DiabloBrokerCall("SmcInitInstruction",ins);
  
  return run - codep;
}
#undef UpdateCurrentAndRun
#undef CheckOpRelocated 
#undef CheckNoRelocsBefore
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
      ret = AddressNew32 (0xffffffffU);
  }
  else
    ret = AddressNew32 (0);
  last = i;
  return ret;
}
/* }}} */

/** {{{ disassemble a code section */
void I386DisassembleSection(t_section * code)
{
  t_object * obj=SECTION_OBJECT(code);
  t_uint8 * buf = SECTION_DATA(code);
  t_uint8 * run = buf;
  t_uint32 inslen;
  t_address old, current, offset;
  t_i386_ins *ins, *prev;
  t_reloc ** sorted_relocs;
  int nrels, relindex;
  int teller = 0;

  t_uint32 nsubs = 0;
  t_section ** subs = NULL;

  {
    char old_type = SECTION_TYPE (code);
    /* hack: trick SectionGetSubsections in returning the subsections,
     * it wouldn't normally do this for DISASSEMBLING sections */
    SECTION_SET_TYPE (code, CODE_SECTION);
    subs = SectionGetSubsections (code, &nsubs);
    SECTION_SET_TYPE (code, old_type);
    ASSERT(subs, ("Didn't find any subsections while disassembling section %s in object %s", 
      SECTION_NAME(code), OBJECT_NAME(SECTION_OBJECT(code))));
  }

  sorted_relocs = SortRefersTo(code, &nrels);

  old = SECTION_OLD_ADDRESS(code);
  current = SECTION_CADDRESS(code);

  prev = NULL;
  offset=AddressNew32(0);
  relindex = 0;

  while (AddressIsLt(offset, SECTION_CSIZE(code)))
  {
    /* let relindex point to the first relocation at or beyond the current address */
    while ((relindex < nrels) 
      && AddressIsLt(AddressAdd(RELOC_FROM_OFFSET(sorted_relocs[relindex]),
                        RELOCATABLE_CADDRESS(RELOC_FROM(sorted_relocs[relindex]))),current))
      relindex++;

    ins = T_I386_INS(InsNewForSec(code));

    {
      t_i386_operand * operand;
      I386_INS_FOREACH_OP(ins,operand)
      {
        I386_OP_BASE(operand)=I386_REG_NONE;
        I386_OP_INDEX(operand)=I386_REG_NONE;
      }
    }

    SECTION_ADDRESS_TO_INS_MAP(code)[teller++]=(t_ins*) ins;

    I386_INS_SET_CADDRESS(ins,  current);
    I386_INS_SET_OLD_ADDRESS(ins, old);

    if (SymbolTableGetDataType(obj, current) == ADDRESS_IS_DATA)
    {
      I386_INS_SET_TYPE(ins,  IT_DATA);
      I386_INS_SET_ATTRIB(ins,  IF_DATA);
      I386_INS_SET_OPCODE(ins, I386_DATA);
      I386_INS_SET_CONDITION(ins, I386_CONDITION_NONE);
              
      I386_INS_SET_CSIZE(ins, AddressNew32(1));
      I386_INS_SET_OLD_SIZE(ins, AddressNew32(1));

      I386_INS_SET_DATA(ins, (t_uint8) (*run));
      I386_INS_SET_DEST(ins, Calloc(1,sizeof(t_i386_operand)));
      I386_INS_SET_SOURCE1(ins, Calloc(1,sizeof(t_i386_operand)));
      I386_INS_SET_SOURCE2(ins, Calloc(1,sizeof(t_i386_operand)));
      DiabloBrokerCall("SmcCreateDataIns",ins,FALSE);
    }
    else
    {
      int i;

      inslen = I386DisassembleOne(ins,run,sorted_relocs,nrels,relindex);
      ASSERT(inslen,("Disassembly of instruction at @G (old address) failed!",current));

      I386_INS_SET_CSIZE(ins, AddressNew32(inslen));
      I386_INS_SET_OLD_SIZE(ins, AddressNew32(inslen));

      for (i = 1; i < inslen; i++)
      {
        teller++;
      }
    }

    run=AddressAddDispl(run,I386_INS_CSIZE(ins));
    offset=AddressAdd(offset,I386_INS_CSIZE(ins));
    current=AddressAdd(current,I386_INS_CSIZE(ins));
    old=AddressAdd(old,I386_INS_OLD_SIZE(ins));

    if (prev) 
      I386_INS_SET_INEXT(prev, ins);
    I386_INS_SET_IPREV(ins, prev);
    prev = ins;
    while (I386_INS_INEXT (prev)) 
      prev = I386_INS_INEXT (prev); /* make prev point to the last dummy */
  }
  if (sorted_relocs)
    Free(sorted_relocs);
  Free(subs);

#ifdef _MSC_VER
  {
  FILE *f = fopen("disassemble.list","w");
  ins = prev;
  while (INS_IPREV(ins)) ins = INS_IPREV(ins);
  for (; ins; ins = INS_INEXT(ins))
    FileIo(f,"@I\n",ins);
  fclose(f);
  //exit(0);
  }
#endif  
}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
