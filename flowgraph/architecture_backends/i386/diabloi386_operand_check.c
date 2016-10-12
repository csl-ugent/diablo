/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloi386.h>

t_bool I386OpCheckNone(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_none)
    return TRUE;
  return FALSE;
}

t_bool I386OpCheckReg(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) != i386_optype_reg)
    return FALSE;
  /* check if we've got the right register */
  switch (bm)
  {
    case bm_AL:
    case bm_AH:
    case bm_AX:
    case bm_EAX:
    case bm_eAX:
      if (I386_OP_BASE(op) != I386_REG_EAX)
	return FALSE;
      break;
    case bm_BL:
    case bm_BH:
    case bm_BX:
    case bm_EBX:
    case bm_eBX:
      if (I386_OP_BASE(op) != I386_REG_EBX)
	return FALSE;
      break;
    case bm_CL:
    case bm_CH:
    case bm_CX:
    case bm_ECX:
    case bm_eCX:
      if (I386_OP_BASE(op) != I386_REG_ECX)
	return FALSE;
      break;
    case bm_DL:
    case bm_DH:
    case bm_DX:
    case bm_EDX:
    case bm_eDX:
      if (I386_OP_BASE(op) != I386_REG_EDX)
	return FALSE;
      break;
    case bm_ESI:
    case bm_eSI:
      if (I386_OP_BASE(op) != I386_REG_ESI)
	return FALSE;
      break;
    case bm_EDI:
    case bm_eDI:
      if (I386_OP_BASE(op) != I386_REG_EDI)
	return FALSE;
      break;
    case bm_ESP:
    case bm_eSP:
      if (I386_OP_BASE(op) != I386_REG_ESP)
	return FALSE;
      break;
    case bm_EBP:
    case bm_eBP:
      if (I386_OP_BASE(op) != I386_REG_EBP)
	return FALSE;
      break;
    case bm_CS:
      if (I386_OP_BASE(op) != I386_REG_CS)
	return FALSE;
      else
	return TRUE;
    case bm_DS:
      if (I386_OP_BASE(op) != I386_REG_DS)
	return FALSE;
      else
	return TRUE;
    case bm_ES:
      if (I386_OP_BASE(op) != I386_REG_ES)
	return FALSE;
      else
	return TRUE;
    case bm_FS:
      if (I386_OP_BASE(op) != I386_REG_FS)
	return FALSE;
      else
	return TRUE;
    case bm_GS:
      if (I386_OP_BASE(op) != I386_REG_GS)
	return FALSE;
      else
	return TRUE;
    case bm_SS:
      if (I386_OP_BASE(op) != I386_REG_SS)
	return FALSE;
      else
	return TRUE;
    default:
      return FALSE;
  }

  /* check if we've got the right byte mode */
  switch (bm)
  {
    case bm_AL:
    case bm_BL:
    case bm_CL:
    case bm_DL:
      if (I386_OP_REGMODE(op) != i386_regmode_lo8)
	return FALSE;
      return TRUE;
    case bm_AH:
    case bm_BH:
    case bm_CH:
    case bm_DH:
      if (I386_OP_REGMODE(op) != i386_regmode_hi8)
	return FALSE;
      return TRUE;
    case bm_AX:
    case bm_BX:
    case bm_CX:
    case bm_DX:
      if (I386_OP_REGMODE(op) != i386_regmode_lo16)
	return FALSE;
      return TRUE;
    case bm_EAX:
    case bm_EBX:
    case bm_ECX:
    case bm_EDX:
    case bm_ESI:
    case bm_EDI:
    case bm_ESP:
    case bm_EBP:
      if (I386_OP_REGMODE(op) != i386_regmode_full32)
	return FALSE;
      return TRUE;
    case bm_eAX:
    case bm_eBX:
    case bm_eCX:
    case bm_eDX:
    case bm_eSI:
    case bm_eDI:
    case bm_eSP:
    case bm_eBP:
      if (I386_OP_REGMODE(op) != i386_regmode_full32 && I386_OP_REGMODE(op) != i386_regmode_lo16)
	return FALSE;
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

t_bool I386OpCheckST(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) != i386_optype_reg)
    return FALSE;

  switch (bm)
  {
    case bm_ST:
    case bm_ST0:
      return I386_OP_BASE(op) == I386_REG_ST0;
    case bm_ST1:
      return I386_OP_BASE(op) == I386_REG_ST1;
    case bm_ST2:
      return I386_OP_BASE(op) == I386_REG_ST2;
    case bm_ST3:
      return I386_OP_BASE(op) == I386_REG_ST3;
    case bm_ST4:
      return I386_OP_BASE(op) == I386_REG_ST4;
    case bm_ST5:
      return I386_OP_BASE(op) == I386_REG_ST5;
    case bm_ST6:
      return I386_OP_BASE(op) == I386_REG_ST6;
    case bm_ST7:
      return I386_OP_BASE(op) == I386_REG_ST7;
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckConst1(t_i386_operand * op, t_i386_bytemode bm)
{
  return (I386_OP_TYPE(op) == i386_optype_imm) && (I386_OP_IMMEDIATE(op) == 1) && (I386_OP_IMMEDSIZE(op) == 1);
}

t_bool I386OpCheckA(t_i386_operand * op, t_i386_bytemode bm)
{
  if (bm == bm_p)
  {
    if (I386_OP_TYPE(op) == i386_optype_farptr)
      return TRUE;
  }
  else
    FATAL(("Unexpected byte mode with A"));
  return FALSE;
}

t_bool I386OpCheckC(t_i386_operand * op, t_i386_bytemode bm)
{
  return (I386_OP_TYPE(op) == i386_optype_reg) && I386IsControlReg(I386_OP_BASE(op));
}

t_bool I386OpCheckD(t_i386_operand * op, t_i386_bytemode bm)
{
  return (I386_OP_TYPE(op) == i386_optype_reg) && I386IsDebugReg(I386_OP_BASE(op));
}

t_bool I386OpCheckE(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    if (!I386IsGeneralPurposeReg(I386_OP_BASE(op))) return FALSE;
    switch (bm)
    {
      case bm_b:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo8) || (I386_OP_REGMODE(op) == i386_regmode_hi8));
      case bm_w:
	return (I386_OP_REGMODE(op) == i386_regmode_lo16);
      case bm_d:
	return (I386_OP_REGMODE(op) == i386_regmode_full32);
      case bm_v:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo16) || (I386_OP_REGMODE(op) == i386_regmode_full32));
      default:
	FATAL(("unexpected byte mode"));
    }
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);

    /* special cases */
    if (I386_OP_BASE(op) == I386_REG_NONE && I386_OP_IMMEDSIZE(op) != 4)
      return FALSE;
    if (I386_OP_BASE(op) == I386_REG_EBP && I386_OP_IMMEDSIZE(op) == 0)
      return FALSE;

    /* check for immediate sizes */
    if (I386_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
      
    switch (bm)
    {
      case bm_b:
	return I386_OP_MEMOPSIZE(op) == 1;
      case bm_w:
	return I386_OP_MEMOPSIZE(op) == 2;
      case bm_d:
	return I386_OP_MEMOPSIZE(op) == 4;
      case bm_v:
	return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
      case bm_p:
	return I386_OP_MEMOPSIZE(op) == 6;
      default:
	FATAL(("unexpected bytemode"));
    }
  }
  return FALSE;
}

t_bool I386OpCheckF(t_i386_operand * op, t_i386_bytemode bm)
{
  /* the flags register is implicit (only used in pushf and popf),
   * so we expect to see an empty operand here */
  return I386_OP_TYPE(op) == i386_optype_none;
}

t_bool I386OpCheckG(t_i386_operand * op, t_i386_bytemode bm)
{
  if ((I386_OP_TYPE(op) == i386_optype_reg) && I386IsGeneralPurposeReg(I386_OP_BASE(op)))
  {
    switch (bm)
    {
      case bm_b:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo8) || (I386_OP_REGMODE(op) == i386_regmode_hi8));
      case bm_w:
	return (I386_OP_REGMODE(op) == i386_regmode_lo16);
      case bm_d:
	return (I386_OP_REGMODE(op) == i386_regmode_full32);
      case bm_v:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo16) || (I386_OP_REGMODE(op) == i386_regmode_full32));
      default:
	FATAL(("unexpected byte mode"));
    }
  }
  return FALSE;
}

/*new functions that dont care as much about the IMMED_SIZE {{{ */
t_bool I386OpChecksI2(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) != i386_optype_imm) return FALSE;
   if (bm == bm_b)
   {
     t_int32 sval = (t_int32) I386_OP_IMMEDIATE(op);
     return (-128 <= sval) && (sval < 128);
   }
   else
     FATAL(("unexpected byte mode"));
   return FALSE;
}

t_bool I386OpCheckI2(t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 imm = I386_OP_IMMEDIATE(op);
  if (I386_OP_TYPE(op) != i386_optype_imm) return FALSE;
  
  switch (bm)
  {
    case bm_b:
      return imm < 256;
    case bm_w:
      return imm < 65536;
    case bm_d:
      return TRUE;
    case bm_v:
      return (I386_OP_IMMEDSIZE(op) == 2 && imm < 65536) || (I386_OP_IMMEDSIZE(op) == 4);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}
/*}}}*/

t_bool I386OpChecksI(t_i386_operand * op, t_i386_bytemode bm)
{
  t_int32 sval;
  
  if (I386_OP_TYPE(op) != i386_optype_imm) return FALSE;
  sval = (t_int32) I386_OP_IMMEDIATE(op);
  switch (bm)
  {
    case bm_b:
      if (I386_OP_IMMEDSIZE(op) == 1)
      {
	return (-128 <= sval) && (sval < 128);
      }
      break;
    case bm_w:
      if (I386_OP_IMMEDSIZE(op) == 2)
      {
	return (-32768 <= sval) && (sval < 32768);
      }
      break;
    case bm_v:
      if (I386_OP_IMMEDSIZE(op) == 2)
      {
	return (-32768 <= sval) && (sval < 32768);
      }
      else if (I386_OP_IMMEDSIZE(op) == 4)
      {
	/* No use to check here, int is 32 bit... */
	return TRUE;
      }
      break;
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckI(t_i386_operand * op, t_i386_bytemode bm)
{
  t_uint32 imm = I386_OP_IMMEDIATE(op);
  if (I386_OP_TYPE(op) != i386_optype_imm) return FALSE;
  
  switch (bm)
  {
    case bm_b:
      return I386_OP_IMMEDSIZE(op) == 1 && imm < 256;
    case bm_w:
      return I386_OP_IMMEDSIZE(op) == 2 && imm < 65536;
    case bm_d:
      return I386_OP_IMMEDSIZE(op) == 4;
    case bm_v:
      return (I386_OP_IMMEDSIZE(op) == 2 && imm < 65536) || (I386_OP_IMMEDSIZE(op) == 4);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckJ(t_i386_operand * op, t_i386_bytemode bm)
{
  t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);

  if (I386_OP_TYPE(op) != i386_optype_imm) return FALSE;
  switch (bm)
  {
    case bm_b:
      return I386_OP_IMMEDSIZE(op) == 1 && simm >= -128 && simm < 128;
    case bm_v:
      return (I386_OP_IMMEDSIZE(op) == 2 && simm >= -32768 && simm < 32768) || (I386_OP_IMMEDSIZE(op) == 4);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckM(t_i386_operand * op, t_i386_bytemode bm)
{
  t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);

  if (I386_OP_TYPE(op) != i386_optype_mem)
    return FALSE;

  /* special cases */
  if (I386_OP_BASE(op) == I386_REG_NONE && I386_OP_IMMEDSIZE(op) != 4)
    return FALSE;
  if (I386_OP_BASE(op) == I386_REG_EBP && I386_OP_IMMEDSIZE(op) == 0)
    return FALSE;
  if (I386_OP_INDEX(op) == I386_REG_ESP)
    return FALSE;

  /* check for immediate sizes */
  if (I386_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
  if (I386_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
  if (I386_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;

  switch (bm)
  {
    case 0:
      return I386_OP_MEMOPSIZE(op) == 0;
    case bm_b:
      return I386_OP_MEMOPSIZE(op) == 1;
    case bm_p:
      return (I386_OP_MEMOPSIZE(op) == 4) || (I386_OP_MEMOPSIZE(op) == 6); 
    case bm_s:
      return I386_OP_MEMOPSIZE(op) == 6; 

    /* the fpu memory operand bytemodes */
    case bm_sr:
      return I386_OP_MEMOPSIZE(op) == 4;
    case bm_dr:
      return I386_OP_MEMOPSIZE(op) == 8;
    case bm_er:
    case bm_bcd:
      return I386_OP_MEMOPSIZE(op) == 10;
    case bm_w:
      return I386_OP_MEMOPSIZE(op) == 2;
    case bm_d:
      return I386_OP_MEMOPSIZE(op) == 4;
    case bm_q:
      return I386_OP_MEMOPSIZE(op) == 8;
    case bm_2byte:
      return I386_OP_MEMOPSIZE(op) == 2;
    case bm_28byte:
      return I386_OP_MEMOPSIZE(op) == 28;
    case bm_108byte:
      return I386_OP_MEMOPSIZE(op) == 108;
    case bm_512byte:
      return I386_OP_MEMOPSIZE(op) == 512;

    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckO(t_i386_operand * op, t_i386_bytemode bm)
{
  /* memory operand, but no base, index or scale can be specified */
  if (I386_OP_TYPE(op) != i386_optype_mem) return FALSE;
  if ((I386_OP_BASE(op) != I386_REG_NONE) || (I386_OP_INDEX(op) != I386_REG_NONE)) return FALSE;
  if (I386_OP_IMMEDSIZE(op) != 2 && I386_OP_IMMEDSIZE(op) != 4) return FALSE;
  switch (bm)
  {
    case bm_b:
      return I386_OP_MEMOPSIZE(op) == 1;
    case bm_v:
      return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool I386OpCheckR(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) != i386_optype_reg) return FALSE;
  return I386OpCheckE(op,bm);
}

t_bool I386OpCheckS(t_i386_operand * op, t_i386_bytemode bm)
{
  return (I386_OP_TYPE(op) == i386_optype_reg) && I386IsSegmentReg(I386_OP_BASE(op));
}

t_bool I386OpCheckX(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    if ((I386_OP_BASE(op) == I386_REG_ESI) && (I386_OP_INDEX(op) == I386_REG_NONE) && (I386_OP_IMMEDIATE(op) == 0))
    {
      switch (bm)
      {
	case bm_b:
	  return I386_OP_MEMOPSIZE(op) == 1;
	case bm_v:
	  return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
	default:
	  FATAL(("unexpected byte mode"));
      }
    }
  }
  return FALSE;
}

t_bool I386OpCheckY(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    if ((I386_OP_BASE(op) == I386_REG_EDI) && (I386_OP_INDEX(op) == I386_REG_NONE) && (I386_OP_IMMEDIATE(op) == 0))
    {
      switch (bm)
      {
	case bm_b:
	  return I386_OP_MEMOPSIZE(op) == 1;
	case bm_v:
	  return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
	default:
	  FATAL(("unexpected byte mode"));
      }
    }
  }
  return FALSE;
}

t_bool I386OpCheckV(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    if (!I386IsGeneralPurposeReg(I386_OP_BASE(op))) return FALSE;
    switch (bm)
    {
      case bm_b:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo8) || (I386_OP_REGMODE(op) == i386_regmode_hi8));
      case bm_w:
	return (I386_OP_REGMODE(op) == i386_regmode_lo16);
      case bm_d:
	return (I386_OP_REGMODE(op) == i386_regmode_full32);
      case bm_v:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo16) || (I386_OP_REGMODE(op) == i386_regmode_full32));
      default:
	FATAL(("unexpected byte mode"));
    }
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);

    /* special cases */
    if (I386_OP_BASE(op) == I386_REG_NONE && I386_OP_IMMEDSIZE(op) != 4)
      return FALSE;
    if (I386_OP_BASE(op) == I386_REG_EBP && I386_OP_IMMEDSIZE(op) == 0)
      return FALSE;

    /* check for immediate sizes */
    if (I386_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
      
    switch (bm)
    {
      case bm_b:
	return I386_OP_MEMOPSIZE(op) == 1;
      case bm_w:
	return I386_OP_MEMOPSIZE(op) == 2;
      case bm_d:
	return I386_OP_MEMOPSIZE(op) == 4;
      case bm_v:
	return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
      case bm_p:
	return I386_OP_MEMOPSIZE(op) == 6;
      default:
	FATAL(("unexpected bytemode"));
    }
  }
  return FALSE;
}

t_bool I386OpCheckW(t_i386_operand * op, t_i386_bytemode bm)
{
  if (I386_OP_TYPE(op) == i386_optype_reg)
  {
    if (!I386IsGeneralPurposeReg(I386_OP_BASE(op))) return FALSE;
    switch (bm)
    {
      case bm_b:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo8) || (I386_OP_REGMODE(op) == i386_regmode_hi8));
      case bm_w:
	return (I386_OP_REGMODE(op) == i386_regmode_lo16);
      case bm_d:
	return (I386_OP_REGMODE(op) == i386_regmode_full32);
      case bm_v:
	return ((I386_OP_REGMODE(op) == i386_regmode_lo16) || (I386_OP_REGMODE(op) == i386_regmode_full32));
      default:
	FATAL(("unexpected byte mode"));
    }
  }
  else if (I386_OP_TYPE(op) == i386_optype_mem)
  {
    t_int32 simm = (t_int32) I386_OP_IMMEDIATE(op);

    /* special cases */
    if (I386_OP_BASE(op) == I386_REG_NONE && I386_OP_IMMEDSIZE(op) != 4)
      return FALSE;
    if (I386_OP_BASE(op) == I386_REG_EBP && I386_OP_IMMEDSIZE(op) == 0)
      return FALSE;

    /* check for immediate sizes */
    if (I386_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
    if (I386_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
      
    switch (bm)
    {
      case bm_b:
	return I386_OP_MEMOPSIZE(op) == 1;
      case bm_w:
	return I386_OP_MEMOPSIZE(op) == 2;
      case bm_d:
	return I386_OP_MEMOPSIZE(op) == 4;
      case bm_v:
	return (I386_OP_MEMOPSIZE(op) == 2) || (I386_OP_MEMOPSIZE(op) == 4);
      case bm_p:
	return I386_OP_MEMOPSIZE(op) == 6;
      default:
	FATAL(("unexpected bytemode"));
    }
  }
  return FALSE;
}

/* vim: set shiftwidth=2 foldmethod=marker: */
