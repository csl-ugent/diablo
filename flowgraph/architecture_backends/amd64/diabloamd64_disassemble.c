#include <diabloamd64.h>

/** {{{ Parse instruction prefixes */
static t_uint32 HandlePrefixes(t_amd64_ins * ins, t_uint8 * codep,char * rex)
{
  char * run = ((char *) codep);
  AMD64_INS_SET_PREFIXES(ins,0);
  while (1)
  {
    switch (*run)
    {
      case '\xf0':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_LOCK);
	break;

      case '\xf2':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_REPNZ);
	break;
      case '\xf3':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_REP);
	break;
      case '\x2e':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_CS_OVERRIDE);
	break;
      case '\x36':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_SS_OVERRIDE);
	break;
      case '\x3e':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_DS_OVERRIDE);
	break;
      case '\x26':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_ES_OVERRIDE);
	break;
      case '\x64':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_FS_OVERRIDE);
	break;
      case '\x65':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_GS_OVERRIDE);
	break;
      case '\x66':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_OPSIZE_OVERRIDE);
	break;
      case '\x67':
	AMD64_INS_SET_PREFIXES(ins,AMD64_INS_PREFIXES(ins)|AMD64_PREFIX_ADDRSIZE_OVERRIDE);
	break;

      case '\x40':
      case '\x41':
      case '\x42':
      case '\x43':
      case '\x44':
      case '\x45':
      case '\x46':
      case '\x47':
      case '\x48':
      case '\x49':
      case '\x4a':
      case '\x4b':
      case '\x4c':
      case '\x4d':
      case '\x4e':
      case '\x4f':
	/* rex prefix  */
	*rex=*run;
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
static t_uint32 DoFloat(t_amd64_ins * ins, t_uint8 * codep, t_uint32 * modrm_ret, t_uint32 * sib_ret, t_amd64_opcode_entry ** disentry_ret)
{
  t_uint8 * run = codep;
  t_uint32 modrm, sib;
  t_uint32 ext;
  t_amd64_opcode opc;

  modrm = run[1];
  ext = (modrm >> 3) & 7;

  if ((modrm >> 6) == 3)
  {
    /* register operations */
    opc = disamd64_fpu_reg[*run - 0xd8][ext][modrm & 7].opcode;
    *disentry_ret = &(disamd64_fpu_reg[*run - 0xd8][ext][modrm & 7]);
    sib = -1;
  }
  else
  {
    /* memory operations */
    opc = disamd64_fpu_mem[*run - 0xd8][ext].opcode;
   *disentry_ret = &(disamd64_fpu_mem[*run - 0xd8][ext]);
    if ((modrm & 7) == 4)
      sib = *(run + 2);
    else
      sib = -1;
  }

  AMD64_INS_SET_OPCODE(ins, opc);
  *modrm_ret = modrm;
  *sib_ret = sib;
  
  if (sib == -1)
    return 2;
  else
    return 3;
} /* }}} */

/** {{{ Get opcode, ModR/M and SIB byte for a non-fpu instruction */
static t_uint32 GetOpcodeModRMAndSIB(t_amd64_ins * ins, t_uint8 * codep, char rex, t_uint32 * modrm_ret, t_uint32 * sib_ret, t_amd64_opcode_entry ** disentry_ret)
{
  t_uint8 * run = (t_uint8 *)codep;
  t_amd64_opcode opc = disamd64[*run].opcode;
  t_bool has_modrm = disamd64[*run].has_modrm;
  t_uint32 modrm = -1, sib = -1;
  t_bool sib_was_assigned = FALSE;

  static t_bool seen_ins_with_prefixes;
  static t_bool seen_ins_3dnow;

#ifdef STEGO
  ins->opcode_byte = *codep;
#endif
  
  *disentry_ret = &(disamd64[*run]);

  if((*run==0x90) && (rexb(rex))){
     *disentry_ret = &(nopinstr);
  }
  
  run++;


  /* floating point instructions are somewhat more complex. they are handled
   * in a separate function */
  if (opc == AMD64_FPU_ESC)
    return DoFloat(ins,codep,modrm_ret,sib_ret,disentry_ret);

  if (opc == AMD64_TWOBYTE_ESC)
  {
    opc = disamd64_twobyte[*run].opcode;
    has_modrm = disamd64_twobyte[*run].has_modrm;
    *disentry_ret = &(disamd64_twobyte[*run]);
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

    if (opc >= AMD64_PREFIX_GRP_1)
      {
	/* it is one of the groups based on prefixes */
	if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_SD_OPERANDS))
	  {
	    ext = 0;
	  }
	else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_SS_OPERANDS))
	  {
	    ext = 1;
	  }
	else if (AMD64_INS_HAS_PREFIX(ins,AMD64_PREFIX_PS_OPERANDS))
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
//	    printf(" !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! be careful, ins with prefixes are only supported for allowing disassembly\n");
	  }
	//	printf("%d %d %d\n",opc,opc - AMD64_GRP1a,ext); 

	*disentry_ret = &(disamd64_grps[opc - AMD64_GRP1a][ext]);
	opc = disamd64_grps[opc - AMD64_GRP1a][ext].opcode;

	/*	printf("%d\n",opc); */

      }
    
    else if (opc == AMD64_3DNOW_OPC)
      {
	t_uint32 mod, rm;
	if (disamd64_3dnow[opc].has_modrm)
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
	
	
	*disentry_ret = &(disamd64_3dnow[opc]);
	opc = disamd64_3dnow[opc].opcode;
	has_modrm = FALSE;
	/*	printf("3dnow %x %x %x %x\n",*run,*(run+1),*(run+2),*(run+3));*/
	run++;
	run++;
      }
    else
      {
	/* we haven't determined a precise opcode yet.
	 * we'll have to look at the opcode extension bits in the modrm byte */
	
	if (opc == AMD64_GRP15a && ((modrm & 0xc0) == 0xc0))
	  {
	    opc++; /* SPECIAL CASE: group 15 */
	  }

	ext  = (modrm & 0x38) >> 3;

	/* sanity check: if there is no modrm byte something is definitely wrong */
	ASSERT(has_modrm,("Disassembly tables corrupt!"));
	
	*disentry_ret = &(disamd64_grps[opc - AMD64_GRP1a][ext]);
	opc = disamd64_grps[opc - AMD64_GRP1a][ext].opcode;
      }    
  }

  if (opc < 0)
    goto indirection;

  /* if mod != 11b && rm == 100b && addressing mode is 32-bit, we have an sib
   * byte */
  if (!sib_was_assigned && has_modrm && !AMD64_ADSZPREF(ins) && ((modrm >> 6) != 3) && ((modrm & 7) == 4))
  {
    sib = (t_uint32) *run;
    run++;
  }
  
  if(opc == AMD64_UNSUPPORTED_OPC){
    FATAL(("Unsupported Instruction at %x", AMD64_INS_OLD_ADDRESS(ins)));
  }
	     
  AMD64_INS_SET_OPCODE(ins,opc);

  /*  printf("opcode %d\n",opc); */

  *modrm_ret = modrm;
  *sib_ret = sib;

  /*  printf("modrm %x sib %x\n",modrm,sib); */

  return (run - ((t_uint8 *)codep));
} /* }}} */

/** {{{ set condition field for conditional instructions */
static void SetConditionField(t_amd64_ins * ins,t_uint8 * orig_opcode)
{
  switch (AMD64_INS_OPCODE(ins))
  {
    case AMD64_Jcc:
    case AMD64_SETcc:
    case AMD64_CMOVcc:
      if (orig_opcode[0] == 0x0f)
      {
	/* two-byte opcode: four last bits of second opcode byte */
	AMD64_INS_SET_CONDITION(ins,((t_uint32) orig_opcode[1]) & 0xf);
      }
      else
      {
        /* one-byte opcode: four last bits of the first opcode byte */
	AMD64_INS_SET_CONDITION(ins,((t_uint32) orig_opcode[0]) & 0xf);
      }
      break;

    case AMD64_FCMOVcc:
      {
	t_uint32 selector = ((orig_opcode[0] & 0x1) << 2) + ((orig_opcode[1] & 0x18) >> 3);
	switch (selector)
	{
	  case 0:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_B);
	    break;
	  case 1:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_Z);
	    break;
	  case 2:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_BE);
	    break;
	  case 3:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_P);
	    break;
	  case 4:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_AE);
	    break;
	  case 5:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_NZ);
	    break;
	  case 6:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_A);
	      ;
	    break;
	  case 7:
	    AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_NP);
	    break;
	}
      }
      break;
    case AMD64_LOOPNZ:
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_LOOPNZ);
      break;
    case AMD64_LOOPZ:
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_LOOPZ);
      break;
    case AMD64_LOOP:
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_LOOP);
      break;
    case AMD64_JRCXZ:
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_RCXZ);
      break;
      default:
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_NONE);
  }

  if (AMD64_INS_CONDITION(ins) != AMD64_CONDITION_NONE)
    AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) | IF_CONDITIONAL);

} /* }}} */

/* {{{ set the instruction type field */
static void SetInsType(t_amd64_ins * ins)
{
  AMD64_INS_SET_TYPE(ins,  amd64_opcode_table[AMD64_INS_OPCODE(ins)].type);
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

  qsort(rels,nrels,sizeof(t_reloc *),__rel_cmp);

  *nrels_ret = nrels;
  return rels;
}
/* }}} */

/** {{{ disassemble one  instruction */
#define UpdateCurrentAndRun()   run += len; current = AddressAddUint64(current,len)

#define CheckOpRelocated(op)    if ((curr_rel < max_rel) && AddressIsEq(current,AddressAdd(RELOC_FROM_OFFSET(rels[curr_rel]),RELOCATABLE_CADDRESS(RELOC_FROM(rels[curr_rel])))) && (len > 0)) \
                                { \
				  AMD64_OP_FLAGS(op) |= AMD64_OPFLAG_ISRELOCATED; \
				  curr_rel++; \
				}
#define CheckNoRelocsBefore()   if ((curr_rel < max_rel) && AddressIsGt(current,AddressAdd(rels[curr_rel]->from_offset,REL_FROM(rels[curr_rel])->caddress))) \
                                  FATAL(("Relocs in the non-operand parts of instructions: @R from offset @G base @G current @G",rels[curr_rel], \
					 rels[curr_rel]->from_offset, REL_FROM(rels[curr_rel])->caddress,current));

t_uint32 Amd64DisassembleOne(t_amd64_ins * ins, t_uint8 * codep, t_reloc ** rels, int max_rel, int curr_rel)
{
  t_uint32 len,prefix_len;
  t_uint8 * run;
  t_uint32 modrm = -1, sib = -1;
  t_amd64_opcode_entry * disentry;
  t_address current;
  char rex=0;
  run = codep;
  current = AMD64_INS_CADDRESS(ins);

  /*  DiabloPrint(stdout,"address @G\n",current); */

  /* handle instruction prefixes */  
  
  len = prefix_len = HandlePrefixes(ins,run,&rex);
  UpdateCurrentAndRun();
  
  /* determine opcode bytes + modr/m and sib bytes */
  len = GetOpcodeModRMAndSIB(ins,run,rex,&modrm,&sib,&disentry);
  UpdateCurrentAndRun();

  /* disassemble the instruction operands */
  /*  CheckNoRelocsBefore();*/ /* TODO: terug aanzetten en fixen voor 3DNOW,  BJORN */
  /* 1. the destination operand */
  len = disentry->op1dis(ins,run,modrm,sib,AMD64_INS_DEST(ins),disentry->op1bm,rex);
  CheckOpRelocated(AMD64_INS_DEST(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & DU)
    AMD64_INS_SET_FLAGS(ins,AMD64_INS_FLAGS(ins)|AMD64_IF_DEST_IS_SOURCE);

  /* 2. the first source operand */
  len = disentry->op2dis(ins,run,modrm,sib,AMD64_INS_SOURCE1(ins),disentry->op2bm,rex);
  CheckOpRelocated(AMD64_INS_SOURCE1(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & S1D)
    AMD64_INS_SET_FLAGS(ins,AMD64_INS_FLAGS(ins)|AMD64_IF_SOURCE1_DEF);

  /* 3. the second source operand */
  len = disentry->op3dis(ins,run,modrm,sib,AMD64_INS_SOURCE2(ins),disentry->op3bm,rex);
  CheckOpRelocated(AMD64_INS_SOURCE2(ins));
  UpdateCurrentAndRun();
  if (disentry->usedefpattern & S2D)
    AMD64_INS_SET_FLAGS(ins,AMD64_INS_FLAGS(ins)|AMD64_IF_SOURCE2_DEF);

  if(rex!=0){
    if(AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg 
    && AMD64_OP_REGMODE(AMD64_INS_DEST(ins)) == amd64_regmode_hi8){
      AMD64_OP_REGMODE(AMD64_INS_DEST(ins))=amd64_regmode_lo8;
      switch(AMD64_OP_BASE(AMD64_INS_DEST(ins))){
	case AMD64_REG_RAX:
	  AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RSP;
	  break;
	case AMD64_REG_RBX:
	  AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RDI;
	  break;
	case AMD64_REG_RCX:
	  AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RBP;
	  break;
	case AMD64_REG_RDX:
	  AMD64_OP_BASE(AMD64_INS_DEST(ins))=AMD64_REG_RSI;
	  break;
      }
    }
    if(AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg 
    && AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins)) == amd64_regmode_hi8){
      AMD64_OP_REGMODE(AMD64_INS_SOURCE1(ins))=amd64_regmode_lo8;
      switch(AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))){
	case AMD64_REG_RAX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RSP;
	  break;
	case AMD64_REG_RBX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RDI;
	  break;
	case AMD64_REG_RCX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RBP;
	  break;
	case AMD64_REG_RDX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE1(ins))=AMD64_REG_RSI;
	  break;
      }
    }
    if(AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg 
    && AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins)) == amd64_regmode_hi8){
      AMD64_OP_REGMODE(AMD64_INS_SOURCE2(ins))=amd64_regmode_lo8;
      switch(AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))){
	case AMD64_REG_RAX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RSP;
	  break;
	case AMD64_REG_RBX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RDI;
	  break;
	case AMD64_REG_RCX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RBP;
	  break;
	case AMD64_REG_RDX:
	  AMD64_OP_BASE(AMD64_INS_SOURCE2(ins))=AMD64_REG_RSI;
	  break;
      }
    }
  }

  
  SetConditionField(ins,(t_uint8 *)codep+prefix_len);
  SetInsType(ins);
  AMD64_INS_SET_REGS_USE(ins,  Amd64InsUsedRegisters(ins));
  AMD64_INS_SET_REGS_DEF(ins,  Amd64InsDefinedRegisters(ins));

  /* set the fast_eval flag for constant propagation */
  if (Amd64InsIsConditional(ins)   /* if the status bits are known, but not all source operands, 
					   it's still possible to decide the instruction has no effect */
      || AMD64_INS_OPCODE(ins) == AMD64_XOR	/* for xor %eax,%eax */
      || AMD64_INS_OPCODE(ins) == AMD64_OR	/* for or  $0xffffffff,%eax */
      || AMD64_INS_OPCODE(ins) == AMD64_AND	/* for and $0,%eax */
      || AMD64_INS_OPCODE(ins) == AMD64_SBB	/* for sbb %eax,%eax */
     )
    AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins) & (~IF_FAST_CP_EVAL));
  else
    AMD64_INS_SET_ATTRIB(ins,  AMD64_INS_ATTRIB(ins)| IF_FAST_CP_EVAL);

  return run - codep;
}
#undef UpdateCurrentAndRun
#undef CheckOpRelocated 
#undef CheckNoRelocsBefore
/* }}} */

/** {{{ disassemble a code section */
void Amd64DisassembleSection(t_section * code)
{
  t_object * obj=SECTION_OBJECT(code);
  t_uint8 * buf = SECTION_DATA(code);
  t_uint8 * run = buf;
  t_uint32 inslen;
  t_address old, current, offset;
  t_amd64_ins *ins, *prev;
  t_reloc ** sorted_relocs;
  int nrels, relindex;
  int teller = 0;
		  
  Amd64InitOpcodeTable();
  Amd64CreateOpcodeHashTable();
  sorted_relocs = SortRefersTo(code, &nrels);

  old = SECTION_OLD_ADDRESS(code);
  current = SECTION_CADDRESS(code);

  VERBOSE (0, ("old address @G new address @G", SECTION_OLD_ADDRESS (code), SECTION_CADDRESS (code)));

  prev = NULL;
  offset=AddressNew64(0);
  relindex = 0;

  while (AddressIsLt(offset, SECTION_CSIZE(code)))
  {
    /* let relindex point to the first relocation at or beyond the current address */
    while ((relindex < nrels) && AddressIsLt(AddressAdd(RELOC_FROM_OFFSET(sorted_relocs[relindex]),RELOCATABLE_CADDRESS(RELOC_FROM(sorted_relocs[relindex]))),current))
      relindex++;

    ins = T_AMD64_INS(InsNewForSec(code));

    AMD64_INS_SET_DEST(ins,malloc(sizeof(t_amd64_operand)));
    AMD64_INS_SET_SOURCE1(ins,malloc(sizeof(t_amd64_operand)));
    AMD64_INS_SET_SOURCE2(ins,malloc(sizeof(t_amd64_operand)));
    
    
    {
      t_amd64_operand * operand;
      AMD64_INS_FOREACH_OP(ins,operand)
      {
	AMD64_OP_FLAGS(operand)=0;
	AMD64_OP_BASE(operand)=AMD64_REG_NONE;
	AMD64_OP_INDEX(operand)=AMD64_REG_NONE;
      }
    }

    SECTION_ADDRESS_TO_INS_MAP(code)[teller++]=(t_ins*) ins;

    AMD64_INS_SET_CADDRESS(ins,  current);
    AMD64_INS_SET_OLD_ADDRESS(ins, old);
	
    if (SymbolTableGetDataType(obj, current) == ADDRESS_IS_DATA)
    {
      AMD64_INS_SET_TYPE(ins,  IT_DATA);
      AMD64_INS_SET_ATTRIB(ins,  IF_DATA);
      AMD64_INS_SET_OPCODE(ins,AMD64_DATA);
      AMD64_INS_SET_CONDITION(ins,AMD64_CONDITION_NONE);
              
      AMD64_INS_SET_CSIZE(ins, AddressNew64(1));
      AMD64_INS_SET_OLD_SIZE(ins, AddressNew64(1));
      inslen = 1;
      AMD64_INS_SET_DATA(ins,(t_uint8) (*run));
      DiabloBrokerCall("SmcCreateDataIns",ins,FALSE);
    }
    else
    {
      int i;

      inslen = Amd64DisassembleOne(ins,run,sorted_relocs,nrels,relindex);
      ASSERT(inslen,("Disassembly of instruction at @G (old address) failed!",current));

      AMD64_INS_SET_CSIZE(ins, AddressNew64(inslen));
      AMD64_INS_SET_OLD_SIZE(ins, AddressNew64(inslen));

      for (i = 1; i < inslen; i++)
      {
	teller++;
      }
    }

    run=AddressAddDispl(run, AMD64_INS_CSIZE(ins));
    offset=AddressAdd(offset, AMD64_INS_CSIZE(ins));
    current=AddressAdd(current, AMD64_INS_CSIZE(ins));
    old=AddressAdd(old, AMD64_INS_OLD_SIZE(ins));

    if (prev) 
      AMD64_INS_SET_INEXT(prev, ins);
    AMD64_INS_SET_IPREV(ins, prev);
    prev = ins;
    while ( AMD64_INS_INEXT (prev)) 
      prev = AMD64_INS_INEXT (prev); /* make prev point to the 
						last dummy */  
  }
  Free(sorted_relocs);


#if 0
  FILE *f = fopen("disassemble.list","w");
  ins = prev;
  while (INS_IPREV(ins)) ins = INS_IPREV(ins);
  for (; ins; ins = INS_INEXT(ins))
    FileIo(f,"@I\n",ins);
  fclose(f);
#endif

}
/* }}} */

/* vim: set shiftwidth=2 foldmethod=marker: */
