/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>

static t_reg regs32[] = {
  I386_REG_EAX, I386_REG_ECX, I386_REG_EDX, I386_REG_EBX,
  I386_REG_ESP, I386_REG_EBP, I386_REG_ESI, I386_REG_EDI
};

static t_reg regs8[] = {
  I386_REG_EAX, I386_REG_ECX, I386_REG_EDX, I386_REG_EBX,
  I386_REG_EAX, I386_REG_ECX, I386_REG_EDX, I386_REG_EBX
};

static t_reg regs_seg[] = {
  I386_REG_ES, I386_REG_CS, I386_REG_SS, I386_REG_DS,
  I386_REG_FS, I386_REG_GS, I386_REG_NONE, I386_REG_NONE
};

t_uint32 I386OpDisNone(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is the operand disassembly function that goes with NULL_OP: it does
   * nothing */
  return 0;
}

t_uint32 I386OpDisReg(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  I386_OP_TYPE(op) = i386_optype_reg;
  switch (bm)
  {
    case bm_AL: case bm_AH: case bm_AX: case bm_EAX: case bm_eAX:
      I386_OP_BASE(op) = I386_REG_EAX;
      break;
    case bm_BL: case bm_BH: case bm_BX: case bm_EBX: case bm_eBX:
      I386_OP_BASE(op) = I386_REG_EBX;
      break;
    case bm_CL: case bm_CH: case bm_CX: case bm_ECX: case bm_eCX:
      I386_OP_BASE(op) = I386_REG_ECX;
      break;
    case bm_DL: case bm_DH: case bm_DX: case bm_EDX: case bm_eDX:
      I386_OP_BASE(op) = I386_REG_EDX;
      break;
    case bm_eSI: case bm_ESI:
      I386_OP_BASE(op) = I386_REG_ESI;
      break;
    case bm_eDI: case bm_EDI:
      I386_OP_BASE(op) = I386_REG_EDI;
      break;
    case bm_eSP: case bm_ESP:
      I386_OP_BASE(op) = I386_REG_ESP;
      break;
    case bm_eBP: case bm_EBP:
      I386_OP_BASE(op) = I386_REG_EBP;
      break;
    case bm_CS:
      I386_OP_BASE(op) = I386_REG_CS;
      break;
    case bm_DS:
      I386_OP_BASE(op) = I386_REG_DS;
      break;
    case bm_ES:
      I386_OP_BASE(op) = I386_REG_ES;
      break;
    case bm_FS:
      I386_OP_BASE(op) = I386_REG_FS;
      break;
    case bm_GS:
      I386_OP_BASE(op) = I386_REG_GS;
      break;
    case bm_SS:
      I386_OP_BASE(op) = I386_REG_SS;
      break;
    default:
      FATAL(("unknown byte mode"));
  }

  switch (bm)
  {
    case bm_AL: case bm_BL: case bm_CL: case bm_DL:
      I386_OP_REGMODE(op) = i386_regmode_lo8;
      break;
    case bm_AH: case bm_BH: case bm_CH: case bm_DH:
      I386_OP_REGMODE(op) = i386_regmode_hi8;
      break;
    case bm_AX: case bm_BX: case bm_CX: case bm_DX:
    case bm_CS: case bm_DS: case bm_ES: case bm_FS: case bm_GS: case bm_SS:
      I386_OP_REGMODE(op) = i386_regmode_lo16;
      break;
    case bm_EAX: case bm_EBX: case bm_ECX: case bm_EDX:
    case bm_ESI: case bm_EDI: case bm_ESP: case bm_EBP:
      I386_OP_REGMODE(op) = i386_regmode_full32;
      break;
    case bm_eAX: case bm_eBX: case bm_eCX: case bm_eDX:
    case bm_eSI: case bm_eDI: case bm_eSP: case bm_eBP:
      /* these depend on the operand size prefix */
      if (I386_OPSZPREF(ins))
	I386_OP_REGMODE(op) = i386_regmode_lo16;
      else
	I386_OP_REGMODE(op) = i386_regmode_full32;
      break;
    default:
      FATAL(("Invalid bytemode %d", bm));
  }
  return 0;
}

t_uint32 I386OpDisST(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  
  switch (bm)
  {
    case bm_ST:
    case bm_ST0:
      I386_OP_BASE(op) = I386_REG_ST0;
      break;
    case bm_ST1:
      I386_OP_BASE(op) = I386_REG_ST1;
      break;
    case bm_ST2:
      I386_OP_BASE(op) = I386_REG_ST2;
      break;
    case bm_ST3:
      I386_OP_BASE(op) = I386_REG_ST3;
      break;
    case bm_ST4:
      I386_OP_BASE(op) = I386_REG_ST4;
      break;
    case bm_ST5:
      I386_OP_BASE(op) = I386_REG_ST5;
      break;
    case bm_ST6:
      I386_OP_BASE(op) = I386_REG_ST6;
      break;
    case bm_ST7:
      I386_OP_BASE(op) = I386_REG_ST7;
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return 0;
}

t_uint32 I386OpDisConst1(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  I386_OP_TYPE(op) = i386_optype_imm;
  I386_OP_IMMEDIATE(op) = 1;
  I386_OP_IMMEDSIZE(op) = 1;
  return 0;
}

t_uint32 I386OpDisA(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  ASSERT(bm == bm_p,("unknown bytemode"));

  I386_OP_TYPE(op) = i386_optype_farptr;
  if (I386_OPSZPREF(ins))
  {
    I386_OP_IMMEDIATE(op) = (t_uint32) *(t_uint16 *)codep;
    I386_OP_IMMEDSIZE(op) = 2;
    codep += 2;
  }
  else
  {
    I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
    I386_OP_IMMEDSIZE(op) = 4;
    codep += 4;
  }
  I386_OP_SEGSELECTOR(op) = *(t_uint16 *)codep;
  return I386_OPSZPREF(ins) ? 4 : 6;
}

t_uint32 I386OpDisC(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 reg = (modrm >> 3) & 7;
  ASSERT(bm == bm_d, ("unknown bytemode"));
  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_BASE(op) = I386_REG_CR0 + reg;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  return 0;
}

t_uint32 I386OpDisD(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 reg = (modrm >> 3) & 7;
  ASSERT(bm == bm_d, ("unknown bytemode"));
  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_BASE(op) = I386_REG_DR0 + reg;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  return 0;
}

t_uint32 I386OpDisE(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 mod, rm, reg;
  t_uint32 scale = 0, index = 0, base = 0;

  ASSERT(modrm != -1, ("need modrm byte, got none"));
  mod = (modrm >> 6) & 3;
  reg = (modrm >> 3) & 7;
  rm = modrm & 7;

  if (mod == 3)
  {
    /* register */
    I386_OP_TYPE(op) = i386_optype_reg;
    if (bm == bm_b)
    {
      I386_OP_BASE(op) = regs8[rm];
      I386_OP_REGMODE(op) = rm > 3 ? i386_regmode_hi8 : i386_regmode_lo8;
    }
    else
    {
      I386_OP_BASE(op) = regs32[rm];
      if (bm == bm_d || (bm == bm_v && !I386_OPSZPREF(ins)))
	I386_OP_REGMODE(op) = i386_regmode_full32;
      else
	I386_OP_REGMODE(op) = i386_regmode_lo16;
    }
    return 0;
  }
  else
  {
    t_uint32 readbytes = 0;
    t_bool has_sib = FALSE;

    /* memory operand */
    I386_OP_TYPE(op) = i386_optype_mem;

    /* we don't support 16-bit addressing */
    ASSERT(!I386_ADSZPREF(ins), ("16 bit addressing not supported"));

    if (rm == 4)
    {
      has_sib = TRUE;
      ASSERT(sib != -1, ("need SIB byte but none given"));
      scale = (sib >> 6) & 3;
      index = (sib >> 3) & 7;
      base = sib & 7;
    }

    /* base and index regs */
    I386_OP_BASE(op) = regs32[rm];
    I386_OP_REGMODE(op) = i386_regmode_full32;
    I386_OP_INDEX(op) = I386_REG_NONE;

    /* displacement */
    if (mod == 0)
    {
      if (rm == 5)
      {
	/* special case: only disp32, no base register */
	I386_OP_BASE(op) = I386_REG_NONE;
	I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
	I386_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
      else
      {
	I386_OP_IMMEDIATE(op) = 0;
	I386_OP_IMMEDSIZE(op) = 0;
      }
    }
    if (mod == 1)
    {
      I386_OP_IMMEDIATE(op) = (t_int32) *(t_int8 *)codep;
      I386_OP_IMMEDSIZE(op) = 1;
      codep += 1;
      readbytes = 1;
    }
    else if (mod == 2)
    {
      I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
      I386_OP_IMMEDSIZE(op) = 4;
      codep += 4;
      readbytes = 4;
    }

    /* handle sib byte if present */
    if (has_sib)
    {
      I386_OP_SCALE(op) = scale;
      I386_OP_BASE(op) = regs32[base];
      if (index != 4)  /* can't use %esp as an index register */
	I386_OP_INDEX(op) = regs32[index];
      else 
	I386_OP_INDEX(op) = I386_REG_NONE;

      /* special case: base == 5, mod == 0 */
      if (base == 5 && mod == 0)
      {
	/* just scaled index + disp32, no base */
	I386_OP_BASE(op) = I386_REG_NONE;
	I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
	I386_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
    }

    /* determine size of the memory operand */

    switch (bm)
    {
      case bm_b:
	I386_OP_MEMOPSIZE(op) = 1;
	break;
      case bm_w:
	I386_OP_MEMOPSIZE(op) = 2;
	break;
      case bm_d:
	I386_OP_MEMOPSIZE(op) = 4;
	break;
      case bm_v:
	if (I386_OPSZPREF(ins))
	  I386_OP_MEMOPSIZE(op) = 2;
	else
	  I386_OP_MEMOPSIZE(op) = 4;
	break;
      case bm_p:
        I386_OP_MEMOPSIZE(op) = 6;
	break;
      case bm_sd:
      case bm_ss:
      case bm_ps:
      case bm_pd:
      case bm_dq:
        I386_OP_MEMOPSIZE(op) = 16;
	break;
      case bm_q:
        I386_OP_MEMOPSIZE(op) = 8;
	break;
      default:
	FATAL(("unknown bytemode"));
    }
    
    return readbytes;
  }

  /* keep the compiler happy */
  return 0;
}

t_uint32 I386OpDisF(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is only used in pushf and popf instructions, and the flags register
   * is already implicit in these instructions, so we do nothing here */
  return 0;
}

t_uint32 I386OpDisG(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 reg = (modrm >> 3) & 7;

  ASSERT(modrm != -1, ("need modrm byte but got none"));

  I386_OP_TYPE(op) = i386_optype_reg;
  if (bm == bm_b)
  {
    I386_OP_BASE(op) = regs8[reg];
    if (reg > 3)
      I386_OP_REGMODE(op) = i386_regmode_hi8;
    else
      I386_OP_REGMODE(op) = i386_regmode_lo8;
  }
  else
  {
    I386_OP_BASE(op) = regs32[reg];
    if (bm == bm_d || (bm == bm_v && !I386_OPSZPREF(ins)))
      I386_OP_REGMODE(op) = i386_regmode_full32;
    else
      I386_OP_REGMODE(op) = i386_regmode_lo16;
  }
  return 0;
}

t_uint32 I386OpDissI(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  int readbytes = 0;
  /* sign extended byte immediate */
  I386_OP_TYPE(op) = i386_optype_imm;
  switch (bm)
  {
    case bm_b:
      I386_OP_IMMEDIATE(op) = (t_int32) *(t_int8 *)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 1;
      break;
    case bm_w:
      I386_OP_IMMEDIATE(op) = (t_int32) *(t_int16 *)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 2;
      break;
    case bm_d:
      I386_OP_IMMEDIATE(op) = *(t_int32 *)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 4;
      break;
    case bm_v:
      if (!I386_OPSZPREF(ins))
      {
	I386_OP_IMMEDIATE(op) = *(t_int32 *)codep;
	I386_OP_IMMEDSIZE(op) = readbytes = 4;
      }
      else
      {
	I386_OP_IMMEDIATE(op) = (t_int32) *(t_int16*)codep;
	I386_OP_IMMEDSIZE(op) = readbytes = 2;
      }
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return readbytes;
}

t_uint32 I386OpDisI(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 readbytes = 0;

  /* immediate operand, not sign extended */
  I386_OP_TYPE(op) = i386_optype_imm;
  
  switch (bm)
  {
    case bm_b:
      I386_OP_IMMEDIATE(op) = (t_uint32) *(t_uint8*)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 1;
      break;
    case bm_w:
      I386_OP_IMMEDIATE(op) = (t_uint32) *(t_uint16*)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 2;
      break;
    case bm_d:
      I386_OP_IMMEDIATE(op) = *(t_uint32*)codep;
      I386_OP_IMMEDSIZE(op) = readbytes = 4;
      break;
    case bm_v:
      if (!I386_OPSZPREF(ins))
      {
	I386_OP_IMMEDIATE(op) = *(t_uint32*)codep;
	I386_OP_IMMEDSIZE(op) = readbytes = 4;
      }
      else
      {
	I386_OP_IMMEDIATE(op) = (t_uint32) *(t_uint16*)codep;
	I386_OP_IMMEDSIZE(op) = readbytes = 2;
      }
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return readbytes;
}

t_uint32 I386OpDisJ(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  I386_OP_TYPE(op) = i386_optype_imm;

  if (bm == bm_b)
  {
    /* sign-extended byte */
    I386_OP_IMMEDIATE(op) = (t_int32) *(t_int8 *)codep;
    I386_OP_IMMEDSIZE(op) = 1;
    return 1;
  }
  else if (bm == bm_v)
  {
    if (I386_OPSZPREF(ins))
      FATAL(("opsize override prefix with jump instruction... implement this"));
    
    I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
    I386_OP_IMMEDSIZE(op) = 4;
    return 4;
  }
  else
    FATAL(("unknown bytemode"));

  /* keep the compiler happy */
  return 0;
}

t_uint32 I386OpDisM(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is in fact the same as the E_ cases, with the exception that the mod
   * bits should never be 3, so the operand is always a memory operand.
   * the only real difference is in the bytemodes that come with the
   * instruction */
  t_uint32 retval = I386OpDisE(ins, codep, modrm, sib, op, bm_d);

  /* adjust the memopsize, as this is the only part of the operand depending
   * on the byte mode */
  switch (bm)
  {
    case 0:
      /* this happens with the LEA instruction */
      I386_OP_MEMOPSIZE(op) = 0;
      break;
    case bm_b:
      I386_OP_MEMOPSIZE(op) = 1;
      break;
    case bm_a:
      if (I386_OPSZPREF(ins))
	I386_OP_MEMOPSIZE(op) = 4;
      else
	I386_OP_MEMOPSIZE(op) = 8;
      break;
    case bm_p:
      if (I386_OPSZPREF(ins))
	I386_OP_MEMOPSIZE(op) = 4;
      else
	I386_OP_MEMOPSIZE(op) = 6;
      break;
    case bm_s:
      I386_OP_MEMOPSIZE(op) = 6;
      break;

    /* bytemodes for fpu memory operations */
    case bm_sr:
      I386_OP_MEMOPSIZE(op) = 4;
      break;
    case bm_dr:
      I386_OP_MEMOPSIZE(op) = 8;
      break;
    case bm_er:
    case bm_bcd:
      I386_OP_MEMOPSIZE(op) = 10;
      break;
    case bm_w:
      I386_OP_MEMOPSIZE(op) = 2;
      break;
    case bm_d:
      I386_OP_MEMOPSIZE(op) = 4;
      break;
    case bm_q:
      I386_OP_MEMOPSIZE(op) = 8;
      break;
    case bm_dq:
      I386_OP_MEMOPSIZE(op) = 16;
      break;
    case bm_2byte:
      I386_OP_MEMOPSIZE(op) = 2;
      break;
    case bm_28byte:
      I386_OP_MEMOPSIZE(op) = 28;
      break;
    case bm_108byte:
      I386_OP_MEMOPSIZE(op) = 108;
      break;
    case bm_512byte:
      I386_OP_MEMOPSIZE(op) = 512;
      break;

    default:
      FATAL(("unknown bytemode"));
  }
  return retval;
}

t_uint32 I386OpDisO(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 readbytes = 0;

  /* memory operand, but no modrm byte: just offset specified immediately */
  I386_OP_TYPE(op) = i386_optype_mem;
  I386_OP_BASE(op) = I386_REG_NONE;
  I386_OP_INDEX(op) = I386_REG_NONE;

  if (I386_ADSZPREF(ins))
  {
    /* 16-bit offset */
    I386_OP_IMMEDIATE(op) = (t_uint32) *(t_uint16 *)codep;
    I386_OP_IMMEDSIZE(op) = 2;
    readbytes = 2;
  }
  else
  {
    /* 32-bit offset */
    I386_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
    I386_OP_IMMEDSIZE(op) = 4;
    readbytes = 4;
  }

  if (bm == bm_b)
  {
    I386_OP_MEMOPSIZE(op) = 1;
  }
  else if (bm == bm_v && I386_OPSZPREF(ins))
  {
    I386_OP_MEMOPSIZE(op) = 2;
  }
  else if (bm == bm_v && !I386_OPSZPREF(ins))
  {
    I386_OP_MEMOPSIZE(op) = 4;
  }
  else
    FATAL(("unknown bytemode"));
  
  return readbytes;
}

t_uint32 I386OpDisR(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is equal to the E_ case, but the mod field may only be 3, so the
   * operand will always be a register */
  return I386OpDisE(ins,codep,modrm,sib,op,bm);
}

t_uint32 I386OpDisS(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* reg field of the modrm byte selects a segment register */
  t_uint32 reg = (modrm >> 3) & 7;

  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_BASE(op) = regs_seg[reg];
  I386_OP_REGMODE(op) = i386_regmode_lo16;

  return 0;
}

t_uint32 I386OpDisX(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* memory addressed by DS:SI */
  I386_OP_TYPE(op) = i386_optype_mem;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  I386_OP_BASE(op) = I386_REG_ESI;
  I386_OP_INDEX(op) = I386_REG_NONE;
  I386_OP_IMMEDIATE(op) = 0;
  if (bm == bm_b)
    I386_OP_MEMOPSIZE(op) = 1;
  else if (bm == bm_v && I386_OPSZPREF(ins))
    I386_OP_MEMOPSIZE(op) = 2;
  else if (bm == bm_v && !I386_OPSZPREF(ins))
    I386_OP_MEMOPSIZE(op) = 4;
  else 
    FATAL (("unknown bytemode"));
  return 0;
}

t_uint32 I386OpDisY(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* memory addressed by ES:DI */
  I386_OP_TYPE(op) = i386_optype_mem;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  I386_OP_BASE(op) = I386_REG_EDI;
  I386_OP_INDEX(op) = I386_REG_NONE;
  I386_OP_IMMEDIATE(op) = 0;
  if (bm == bm_b)
    I386_OP_MEMOPSIZE(op) = 1;
  else if (bm == bm_v && I386_OPSZPREF(ins))
    I386_OP_MEMOPSIZE(op) = 2;
  else if (bm == bm_v && !I386_OPSZPREF(ins))
    I386_OP_MEMOPSIZE(op) = 4;
  else 
    FATAL (("unknown bytemode"));
  return 0;
}

t_uint32 I386OpDisV(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is equal to the E_ case, but the mod field may only be 3, so the
   * operand will always be a register */
  t_uint32 reg = (modrm >> 3) & 7;
  I386_OP_TYPE(op) = i386_optype_reg;
  I386_OP_BASE(op) = I386_REG_XMM0 + reg;
  I386_OP_REGMODE(op) = i386_regmode_full32;
  return 0;
}

t_uint32 I386OpDisW(t_i386_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_i386_operand * op, t_i386_bytemode bm)
{
  /* this is equal to the E_ case, but the mod field may only be 3, so the
   * operand will always be a register */
  return I386OpDisE(ins,codep,modrm,sib,op,bm);
}

