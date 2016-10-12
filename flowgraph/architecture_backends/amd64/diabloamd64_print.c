#include <diabloamd64.h>
#include <ctype.h>
#include <string.h>

static t_string conditioncodes[] = {
  "o", "no", "b", "ae", "e", "ne", "be", "a",
  "s", "ns", "p", "np", "l", "ge", "le", "g"
};

static t_string regs64[] = {
  "%rax", "%rbx", "%rcx", "%rdx",
  "%rsi", "%rdi", "%rbp", "%rsp",
  "%r8" , "%r9" , "%r10", "%r11",
  "%r12", "%r13", "%r14", "r15d"
};

static t_string regs32[] = {
  "%eax" , "%ebx" , "%ecx" , "%edx" ,
  "%esi" , "%edi" , "%ebp" , "%esp" ,
  "%r8d" , "%r9d" , "%r10d", "%r11d",
  "%r12d", "%r13d", "%r14d", "%r15d"
};

static t_string regs16[] = {
  "%ax"  , "%bx"   , "%cx"  , "%dx"  ,
  "%si"  , "%di"   , "%bp"  , "%sp"  ,
  "%r8w" , "%r9w"  , "%r10w", "%r11w",
  "%r12w", "%r13w" , "%r14w", "%r15w"
};

static t_string regshi8[] = {
  "%ah"  , "%bh"  , "%ch"   , "%dh"
};

static t_string regslo8[] = {
  "%al"  , "%bl"  , "%cl"   , "%dl"  ,
  "%sil" , "%dil" , "%bpl"  , "%spl"  ,
  "%r8b" , "%r9b" , "%r10b" , "%r11b",
  "%r12b", "%r13b", "%r14b" , "%r15b"
};

static t_string regsseg[] = {
  "%cs", "%ds", "%es", "%fs", "%gs", "%ss"
};

static t_string regsfp[] = {
  "%st", "%st1", "%st2", "%st3",
  "%st4", "%st5", "%st6", "%st7"
};

static t_string regscontrol[] = {
  "%cr0", "%cr1", "%cr2", "%cr3",
  "%cr4", "%cr5", "%cr6", "%cr7"
};

static t_string regsdebug[] = {
  "%dr0", "%dr1", "%dr2", "%dr3",
  "%dr4", "%dr5", "%dr6", "%dr7"
};

static t_string regsxmm[] = {
  "%xmm0" , "%xmm1", "%xmm2", "%xmm3",
  "%xmm4" , "%xmm5", "%xmm6", "%xmm7",
  "%xmm8" , "%xmm9","%xmm10","%xmm11",
  "%xmm12","%xmm13","%xmm14","%xmm15",
};

static void PrintPrefixes(t_amd64_ins * ins, t_string buf)
{
  /* for now, only print repz and repnz prefixes */
  if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REPNZ))
  {
    sprintf(buf,"repnz ");
  }
  else if (AMD64_INS_HAS_PREFIX(ins, AMD64_PREFIX_REP))
  {
    sprintf(buf,"repz ");
  }
  else
    buf[0] = '\0';
}

static void PrintReg(t_amd64_operand * op, t_string buf)
{
  t_reg reg = AMD64_OP_BASE(op);
  int index;

  if (reg >= AMD64_REG_RAX && reg <= AMD64_REG_R15)
  {
    index = reg - AMD64_REG_RAX;
    switch (AMD64_OP_REGMODE(op))
    {
      case amd64_regmode_full64:
        sprintf(buf,"%s",regs64[index]);
        break;
      case amd64_regmode_lo32:
	sprintf(buf,"%s",regs32[index]);
	break;
      case amd64_regmode_lo16:
	sprintf(buf,"%s",regs16[index]);
	break;
      case amd64_regmode_lo8:
	sprintf(buf,"%s",regslo8[index]);
	break;
      case amd64_regmode_hi8:
	sprintf(buf,"%s",regshi8[index]);
	break;
      case amd64_regmode_invalid:
	FATAL(("OOPS"));
    }
  }
  else if (reg >= AMD64_REG_ST0 && reg <= AMD64_REG_ST7)
  {
    index = reg - AMD64_REG_ST0;
    sprintf(buf,"%s",regsfp[index]);
  }
  else if (reg >= AMD64_REG_CS && reg <= AMD64_REG_SS)
  {
    index = reg - AMD64_REG_CS;
    sprintf(buf,"%s",regsseg[index]);
  }
  else if (reg >= AMD64_REG_CR0 && reg <= AMD64_REG_CR4)
  {
    index = reg - AMD64_REG_CR0;
    sprintf(buf,"%s",regscontrol[index]);
  }
  else if (reg >= AMD64_REG_DR0 && reg <= AMD64_REG_DR7)
  {
    index = reg - AMD64_REG_DR0;
    sprintf(buf,"%s",regsdebug[index]);
  }
  else if (reg >= AMD64_REG_XMM0 && reg <= AMD64_REG_XMM15)
  {
      index = reg - AMD64_REG_XMM0;
      sprintf(buf,"%s",regsxmm[index]);
  }
    
  else
    printf("unknown register %d\n",reg);
}

static void PrintMem(t_amd64_operand * op, t_string buf)
{
  t_bool base = (AMD64_OP_BASE(op) != AMD64_REG_NONE);
  t_bool index = (AMD64_OP_INDEX(op) != AMD64_REG_NONE);
  t_bool longform = base || index;
  t_uint32 pos = 0;

  if (AMD64_OP_IMMEDIATE(op) != 0 || !longform)
    pos = sprintf(buf, "0x%llx", AMD64_OP_IMMEDIATE(op));

  if (longform)
  {
    pos += sprintf(buf+pos,"(%s%c",base ? AMD64_OP_BASE(op) == AMD64_REG_RIP ? "%rip"  :regs64[AMD64_OP_BASE(op)] : "", index ? ',' : ')');
    if (index)
      sprintf(buf+pos,"%s,%d)", regs64[AMD64_OP_INDEX(op)], 1 << AMD64_OP_SCALE(op));
  }
}

static t_bool PrintOp(t_amd64_ins * ins, t_amd64_operand * op, t_string buf)
{
  switch (AMD64_OP_TYPE(op))
  {
    case amd64_optype_none:
      return FALSE;

    case amd64_optype_reg:
      if (AMD64_INS_TYPE(ins) == IT_BRANCH)
      {
	/* add a '*' before the operand */
	buf[0] = '*';
	buf++;
      }
      PrintReg(op,buf);
      break;

    case amd64_optype_imm:
      if (AMD64_INS_TYPE(ins) != IT_BRANCH || AMD64_INS_OPCODE(ins) == AMD64_RET)
      {
	if(AMD64_OP_IMMEDSIZE(op)==8){
          t_uint32 temp[2];
	  t_uint64 temp64=AMD64_OP_IMMEDIATE(op);
	  memcpy(&temp,&temp64,2*sizeof(t_uint32));
          sprintf(buf, "$0x%x%x", temp[1],temp[0]);
	}else{
	  sprintf(buf, "$0x%llx", AMD64_OP_IMMEDIATE(op));
	}
      }
      else
      {
	sprintf(buf, "%llx", G_T_UINT64(AddressAdd(AMD64_INS_CADDRESS(ins),AMD64_INS_CSIZE(ins))) + AMD64_OP_IMMEDIATE(op));
      }
      break;

    case amd64_optype_mem:
      if (AMD64_INS_TYPE(ins) == IT_BRANCH)
      {
	/* add a '*' before the operand */
	buf[0] = '*';
	buf++;
      }
      PrintMem(op,buf);
      break;

    case amd64_optype_farptr:
      sprintf(buf, "_FAR_PTR_: TODO");
      break;

    default:
      printf("invalid amd64 instruction\n");
  }
  return TRUE;
}

int Amd64InstructionPrint(t_amd64_ins * ins, char *outputstring) 
{
  char pref[20], opc[20], dest[20], src1[20], src2[20];
  t_uint32 pos;
  t_bool has_dest, has_src1, has_src2;

  /* prefixes */
  PrintPrefixes((t_amd64_ins *) ins,pref);

  /* the opcode */
  sprintf(opc, "%s", amd64_opcode_table[AMD64_INS_OPCODE(ins)].textual);
  if (StringPatternMatch("*CC*",opc))
  {
    /* print condition code */
    for (pos = 0; pos < strlen(opc) - 1; pos++) 
      if (opc[pos] == 'C' && opc[pos+1] == 'C')
	break;
    sprintf(opc+pos, "%s", conditioncodes[AMD64_INS_CONDITION(ins)]);
  }

  /* the operand size suffix */
  /* this is only necessary if the right operand size can not be
   * determined without the suffix, i.e. if there are no register operands */
  if (AMD64_INS_TYPE(ins) != IT_BRANCH)
  {
    t_bool suffix_needed = FALSE, suffix_not_needed = FALSE;
    if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_reg ||
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_reg ||
	AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_reg)
      suffix_not_needed = TRUE;
    
    if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem ||
	AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem ||
	AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
      suffix_needed = TRUE;

    if (suffix_needed && !suffix_not_needed)
    {
      char suffix;
      int opsize = 0;
      if (AMD64_OP_TYPE(AMD64_INS_DEST(ins)) == amd64_optype_mem)
	opsize = AMD64_OP_MEMOPSIZE(AMD64_INS_DEST(ins));
      if (AMD64_OP_TYPE(AMD64_INS_SOURCE1(ins)) == amd64_optype_mem)
	opsize = AMD64_OP_MEMOPSIZE(AMD64_INS_SOURCE1(ins));
      if (AMD64_OP_TYPE(AMD64_INS_SOURCE2(ins)) == amd64_optype_mem)
	opsize = AMD64_OP_MEMOPSIZE(AMD64_INS_SOURCE2(ins));
      
      if (opsize == 1)
	suffix = 'b';
      else if (opsize == 2)
	suffix = 'w';
      else if (opsize == 4)
	suffix = 'l';
      else if (opsize == 8)
        suffix = 'q';
      else
	suffix = ' ';

      opc[strlen(opc)+1] = '\0';
      opc[strlen(opc)] = suffix;
    }
  }

  /* the destination operand */
  has_dest = PrintOp((t_amd64_ins *) ins, AMD64_INS_DEST(ins), dest);

  /* the first source operand */
  has_src1 = PrintOp((t_amd64_ins *) ins, AMD64_INS_SOURCE1(ins), src1);

  /* the second source operand */
  has_src2 = PrintOp((t_amd64_ins *) ins, AMD64_INS_SOURCE2(ins), src2);
 
  /* put it all together */
  if (AMD64_INS_OPCODE(ins) != AMD64_DATA){
    sprintf(outputstring, "%s%-7s%s%s%s%s%s",
      pref, opc,
      has_src2 ? src2 : "", (has_src2 && (has_src1 || has_dest)) ? "," : "",
      has_src1 ? src1 : "", (has_src1 && has_dest) ? "," : "",
      has_dest ? dest : "" );
    return 1;
  }else{
   /* special case: data */
    sprintf(outputstring, "data: %d",   AMD64_INS_DATA(ins));
    return 1;
   // sprintf(outputstring, "%-7s%c %02x", "data", isprint(AMD64_INS_DATA(ins)) ? AMD64_INS_DATA(ins) : '.', (t_uint8) AMD64_INS_DATA(ins));
  } 
}
/* vim: set shiftwidth=2 foldmethod=marker: */
