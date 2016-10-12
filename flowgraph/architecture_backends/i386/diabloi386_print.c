/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>
#include <ctype.h>
#include <string.h>

static t_string conditioncodes[] = {
  "o", "no", "b", "ae", "e", "ne", "be", "a",
  "s", "ns", "p", "np", "l", "ge", "le", "g"
};

static t_string regs32[] = {
  "%eax", "%ebx", "%ecx", "%edx",
  "%esi", "%edi", "%ebp", "%esp"
};

static t_string regs16[] = {
  "%ax", "%bx", "%cx", "%dx",
  "%si", "%di", "%bp", "%sp"
};

static t_string regs8hi[] = {
  "%ah", "%bh", "%ch", "%dh"
};

static t_string regs8lo[] = {
  "%al", "%bl", "%cl", "%dl"
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
  "%xmm0", "%xmm1", "%xmm2", "%xmm3",
  "%xmm4", "%xmm5", "%xmm6", "%xmm7"
};

static void PrintPrefixes(t_i386_ins * ins, t_string buf)
{
  /* for now, only print repz and repnz prefixes */
  if (I386_INS_HAS_PREFIX(ins, I386_PREFIX_REPNZ))
  {
    sprintf(buf,"repnz ");
  }
  else if (I386_INS_HAS_PREFIX(ins, I386_PREFIX_REP))
  {
    sprintf(buf,"repz ");
  }
  else
    buf[0] = '\0';
}

static void PrintReg(t_i386_operand * op, t_string buf)
{
  t_reg reg = I386_OP_BASE(op);
  int index;

  if (reg >= I386_REG_EAX && reg <= I386_REG_ESP)
  {
    index = reg - I386_REG_EAX;
    switch (I386_OP_REGMODE(op))
    {
      case i386_regmode_full32:
	sprintf(buf,"%s",regs32[index]);
	break;
      case i386_regmode_lo16:
	sprintf(buf,"%s",regs16[index]);
	break;
      case i386_regmode_lo8:
	sprintf(buf,"%s",regs8lo[index]);
	break;
      case i386_regmode_hi8:
	sprintf(buf,"%s",regs8hi[index]);
	break;
      case i386_regmode_invalid:
	FATAL(("OOPS"));
    }
  }
  else if (reg >= I386_REG_ST0 && reg <= I386_REG_ST7)
  {
    index = reg - I386_REG_ST0;
    sprintf(buf,"%s",regsfp[index]);
  }
  else if (reg >= I386_REG_CS && reg <= I386_REG_SS)
  {
    index = reg - I386_REG_CS;
    sprintf(buf,"%s",regsseg[index]);
  }
  else if (reg >= I386_REG_CR0 && reg <= I386_REG_CR4)
  {
    index = reg - I386_REG_CR0;
    sprintf(buf,"%s",regscontrol[index]);
  }
  else if (reg >= I386_REG_DR0 && reg <= I386_REG_DR7)
  {
    index = reg - I386_REG_DR0;
    sprintf(buf,"%s",regsdebug[index]);
  }
  else if (reg >= I386_REG_XMM0 && reg <= I386_REG_XMM7)
  {
      index = reg - I386_REG_XMM0;
      sprintf(buf,"%s",regsxmm[index]);
  }
    
  else
    FATAL(("unknown register %d",reg));
}

void I386PrintReg(t_i386_operand * op, t_string buf)
{
  return PrintReg(op,buf); 
}


static void PrintMem(t_i386_operand * op, t_string buf)
{
  t_bool base = (I386_OP_BASE(op) != I386_REG_NONE);
  t_bool index = (I386_OP_INDEX(op) != I386_REG_NONE);
  t_bool longform = base || index;
  t_uint32 pos = 0;

  if (I386_OP_IMMEDIATE(op) != 0 || !longform)
    pos = sprintf(buf, "%d", I386_OP_IMMEDIATE(op));

  if (longform)
  {
    pos += sprintf(buf+pos,"(%s%c",base ? regs32[I386_OP_BASE(op)] : "", index ? ',' : ')');
    if (index)
      sprintf(buf+pos,"%s,%d)", regs32[I386_OP_INDEX(op)], 1 << I386_OP_SCALE(op));
  }
}

static t_bool PrintOp(t_i386_ins * ins, t_i386_operand * op, t_string buf)
{
  switch (I386_OP_TYPE(op))
  {
    case i386_optype_none:
      return FALSE;

    case i386_optype_reg:
      if (I386_INS_TYPE(ins) == IT_BRANCH)
      {
	/* add a '*' before the operand */
	buf[0] = '*';
	buf++;
      }
      PrintReg(op,buf);
      break;

    case i386_optype_imm:
      if (I386_INS_TYPE(ins) != IT_BRANCH || I386_INS_OPCODE(ins) == I386_RET)
      {
	sprintf(buf, "$%d", I386_OP_IMMEDIATE(op));
      }
      else
      {
	sprintf(buf, "%x", G_T_UINT32(AddressAdd(I386_INS_CADDRESS(ins),I386_INS_CSIZE(ins))) + I386_OP_IMMEDIATE(op));
      }
      break;

    case i386_optype_mem:
      if (I386_INS_TYPE(ins) == IT_BRANCH)
      {
	/* add a '*' before the operand */
	buf[0] = '*';
	buf++;
      }
      PrintMem(op,buf);
      break;

    case i386_optype_farptr:
      sprintf(buf, "_FAR_PTR_: TODO");
      break;

    default:
      FATAL(("invalid i386 instruction"));
  }
  return TRUE;
}

void I386InstructionPrint(t_i386_ins * ins, t_string outputstring) 
{
  char pref[40], opc[40], dest[40], src1[40], src2[40];
  t_uint32 pos;
  t_bool has_dest, has_src1, has_src2;

  if ((!(I386_INS_DEST(ins))) || (!(I386_INS_SOURCE1(ins))) || (!(I386_INS_SOURCE2(ins))))
  {
     sprintf(outputstring, "Ins not yet initialized");
     return;
  }

  /* prefixes */
  PrintPrefixes((t_i386_ins *) ins,pref);

  /* the opcode */
  sprintf(opc, "%s", i386_opcode_table[I386_INS_OPCODE(ins)].textual);
  if (StringPatternMatch("*CC*",opc))
  {
    /* print condition code */
    for (pos = 0; pos < strlen(opc) - 1; pos++) 
      if (opc[pos] == 'C' && opc[pos+1] == 'C')
	break;
    sprintf(opc+pos, "%s", conditioncodes[I386_INS_CONDITION(ins)]);
  }

  /* the operand size suffix */
  /* this is only necessary if the right operand size can not be
   * determined without the suffix, i.e. if there are no register operands */
  if (I386_INS_TYPE(ins) != IT_BRANCH)
  {
    t_bool suffix_needed = FALSE, suffix_not_needed = FALSE;
    if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_reg ||
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_reg ||
	I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_reg)
      suffix_not_needed = TRUE;
    
    if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem ||
	I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem ||
	I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
      suffix_needed = TRUE;

    if (1) //(suffix_needed && !suffix_not_needed)
    {
      char suffix;
      int opsize = 0;
      if (I386_OP_TYPE(I386_INS_DEST(ins)) == i386_optype_mem)
	opsize = I386_OP_MEMOPSIZE(I386_INS_DEST(ins));
      if (I386_OP_TYPE(I386_INS_SOURCE1(ins)) == i386_optype_mem)
	opsize = I386_OP_MEMOPSIZE(I386_INS_SOURCE1(ins));
      if (I386_OP_TYPE(I386_INS_SOURCE2(ins)) == i386_optype_mem)
	opsize = I386_OP_MEMOPSIZE(I386_INS_SOURCE2(ins));

      if (opsize == 0)
      {
	/* no mem ops. try registers */
	t_i386_operand *op;
	op = I386_INS_DEST(ins);
	if (I386_OP_TYPE(op) != i386_optype_reg)
	  op = I386_INS_SOURCE1(ins);
	if (I386_OP_TYPE(op) != i386_optype_reg)
	  op = I386_INS_SOURCE2(ins);
	if (I386_OP_TYPE(op) == i386_optype_reg)
	{
	  switch (I386_OP_REGMODE(op))
	  {
	    case i386_regmode_full32:
	      opsize = 4;
	      break;
	    case i386_regmode_lo16:
	      opsize = 2;
	      break;
	    case i386_regmode_hi8:
	    case i386_regmode_lo8:
	      opsize = 1;
	      break;
	    default:
	      break;
	  }
	}
      }
      
      if (opsize == 1)
	suffix = 'b';
      else if (opsize == 2)
	suffix = 'w';
      else if (opsize == 4)
	suffix = 'l';
      else
	suffix = ' ';

      opc[strlen(opc)+1] = '\0';
      opc[strlen(opc)] = suffix;
    }
  }

  /* the destination operand */
  has_dest = PrintOp((t_i386_ins *) ins, I386_INS_DEST(ins), dest);

  /* the first source operand */
  has_src1 = PrintOp((t_i386_ins *) ins, I386_INS_SOURCE1(ins), src1);

  /* the second source operand */
  has_src2 = PrintOp((t_i386_ins *) ins, I386_INS_SOURCE2(ins), src2);
  

  /* put it all together */
  sprintf(outputstring, "%s%-7s%s%s%s%s%s",
      pref, opc,
      has_src2 ? src2 : "", (has_src2 && (has_src1 || has_dest)) ? "," : "",
      has_src1 ? src1 : "", (has_src1 && has_dest) ? "," : "",
      has_dest ? dest : "" );

  /* special case: data */
  if (I386_INS_OPCODE(ins) == I386_DATA)
    sprintf(outputstring, "%-7s%c %02x", "data", isprint(I386_INS_DATA(ins)) ? I386_INS_DATA(ins) : '.', (t_uint8) I386_INS_DATA(ins));
}
/* vim: set shiftwidth=2 foldmethod=marker: */
