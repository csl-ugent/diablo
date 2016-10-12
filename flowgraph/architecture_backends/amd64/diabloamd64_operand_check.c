#include <diabloamd64.h>


t_bool Amd64OpCheckNone(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_none)
    return TRUE;
  return FALSE;
}

t_bool Amd64OpCheckReg(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) != amd64_optype_reg)
    return FALSE;
  /* check if we've got the right register */
  switch (bm)
  {
    case amd64_bm_AL:
    case amd64_bm_AH:
    case amd64_bm_AX:
    case amd64_bm_EAX:
    case amd64_bm_RAX:
    case amd64_bm_eAX:
    case amd64_bm_rAX:
      if (AMD64_OP_BASE(op) != AMD64_REG_RAX)
	return FALSE;
      break;
    case amd64_bm_BL:
    case amd64_bm_BH:
    case amd64_bm_BX:
    case amd64_bm_EBX:
    case amd64_bm_RBX:
    case amd64_bm_eBX:
    case amd64_bm_rBX:
      if (AMD64_OP_BASE(op) != AMD64_REG_RBX)
	return FALSE;
      break;
    case amd64_bm_CL:
    case amd64_bm_CH:
    case amd64_bm_CX:
    case amd64_bm_ECX:
    case amd64_bm_RCX:
    case amd64_bm_eCX:
    case amd64_bm_rCX:
      if (AMD64_OP_BASE(op) != AMD64_REG_RCX)
	return FALSE;
      break;
    case amd64_bm_DL:
    case amd64_bm_DH:
    case amd64_bm_DX:
    case amd64_bm_EDX:
    case amd64_bm_RDX:
    case amd64_bm_eDX:
    case amd64_bm_rDX:
      if (AMD64_OP_BASE(op) != AMD64_REG_RDX)
	return FALSE;
      break;
    case amd64_bm_SI:
    case amd64_bm_ESI:
    case amd64_bm_RSI:
    case amd64_bm_eSI:
    case amd64_bm_rSI:
      if (AMD64_OP_BASE(op) != AMD64_REG_RSI)
	return FALSE;
      break;
    case amd64_bm_DI:
    case amd64_bm_EDI:
    case amd64_bm_RDI:
    case amd64_bm_eDI:
    case amd64_bm_rDI:
      if (AMD64_OP_BASE(op) != AMD64_REG_RDI)
	return FALSE;
      break;
    case amd64_bm_SP:
    case amd64_bm_ESP:
    case amd64_bm_RSP:
    case amd64_bm_eSP:
    case amd64_bm_rSP:
    if (AMD64_OP_BASE(op) != AMD64_REG_RSP)
	return FALSE;
      break;
    case amd64_bm_BP:
    case amd64_bm_EBP:
    case amd64_bm_RBP:
    case amd64_bm_eBP:
    case amd64_bm_rBP:
      if (AMD64_OP_BASE(op) != AMD64_REG_RBP)
	return FALSE;
      break;
    case amd64_bm_R8B:
    case amd64_bm_R8W:
    case amd64_bm_R8D:
    case amd64_bm_R8:
    case amd64_bm_e8:
    case amd64_bm_r8:
      if (AMD64_OP_BASE(op) != AMD64_REG_R8)
        return FALSE;
      break;
    case amd64_bm_R9B:
    case amd64_bm_R9W:
    case amd64_bm_R9D:
    case amd64_bm_R9:
    case amd64_bm_e9:
    case amd64_bm_r9:
      if (AMD64_OP_BASE(op) != AMD64_REG_R9)
        return FALSE;
      break;
    case amd64_bm_R10B:
    case amd64_bm_R10W:
    case amd64_bm_R10D:
    case amd64_bm_R10:
    case amd64_bm_e10:
    case amd64_bm_r10:
      if (AMD64_OP_BASE(op) != AMD64_REG_R10)
        return FALSE;
      break;
    case amd64_bm_R11B:
    case amd64_bm_R11W:
    case amd64_bm_R11D:
    case amd64_bm_R11:
    case amd64_bm_e11:
    case amd64_bm_r11:
      if (AMD64_OP_BASE(op) != AMD64_REG_R11)
        return FALSE;
      break;
    case amd64_bm_R12B:
    case amd64_bm_R12W:
    case amd64_bm_R12D:
    case amd64_bm_R12:
    case amd64_bm_e12:
    case amd64_bm_r12:
      if (AMD64_OP_BASE(op) != AMD64_REG_R12)
        return FALSE;
      break;
    case amd64_bm_R13B:
    case amd64_bm_R13W:
    case amd64_bm_R13D:
    case amd64_bm_R13:
    case amd64_bm_e13:
    case amd64_bm_r13:
      if (AMD64_OP_BASE(op) != AMD64_REG_R13)
        return FALSE;
      break;
    case amd64_bm_R14B:
    case amd64_bm_R14W:
    case amd64_bm_R14D:
    case amd64_bm_R14:
    case amd64_bm_e14:
    case amd64_bm_r14:
      if (AMD64_OP_BASE(op) != AMD64_REG_R14)
        return FALSE;
      break;
    case amd64_bm_R15B:
    case amd64_bm_R15W:
    case amd64_bm_R15D:
    case amd64_bm_R15:
    case amd64_bm_e15:
    case amd64_bm_r15:
      if (AMD64_OP_BASE(op) != AMD64_REG_R15)
        return FALSE;
      break;
    case amd64_bm_rAXrex:
    case amd64_bm_RAXrex:
    case amd64_bm_ALrex:
        if(AMD64_OP_BASE(op) != AMD64_REG_RAX && AMD64_OP_BASE(op) != AMD64_REG_R8)
	  return FALSE;
      break;
    case amd64_bm_rBXrex:
    case amd64_bm_RBXrex:
    case amd64_bm_BLrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RBX && AMD64_OP_BASE(op) != AMD64_REG_R11)
	return FALSE;
      break;
    case amd64_bm_rCXrex:
    case amd64_bm_RCXrex:
    case amd64_bm_CLrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RCX && AMD64_OP_BASE(op) != AMD64_REG_R9)
	return FALSE;
      break;
    case amd64_bm_rDXrex:
    case amd64_bm_RDXrex:
    case amd64_bm_DLrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RDX && AMD64_OP_BASE(op) != AMD64_REG_R10)
	return FALSE;
      break;
    case amd64_bm_rSIrex:
    case amd64_bm_RSIrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RSI && AMD64_OP_BASE(op) != AMD64_REG_R14)
	return FALSE;
      break;
    case amd64_bm_AHrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RSP && AMD64_OP_BASE(op) != AMD64_REG_R12 && AMD64_OP_BASE(op) != AMD64_REG_RAX)
        return FALSE;
      break;
    case amd64_bm_rDIrex:
    case amd64_bm_RDIrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RDI && AMD64_OP_BASE(op) != AMD64_REG_R15)
	return FALSE;
      break;
    case amd64_bm_BHrex:  
      if(AMD64_OP_BASE(op) != AMD64_REG_RDI && AMD64_OP_BASE(op) != AMD64_REG_R15 && AMD64_OP_BASE(op) != AMD64_REG_RBX)
        return FALSE;
      break;
    case amd64_bm_rSPrex:
    case amd64_bm_RSPrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RSP && AMD64_OP_BASE(op) != AMD64_REG_R12)
	return FALSE;
      break;
    case amd64_bm_CHrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RBP && AMD64_OP_BASE(op) != AMD64_REG_R13 && AMD64_OP_BASE(op) != AMD64_REG_RCX)
	return FALSE;
      break;
    case amd64_bm_rBPrex:
    case amd64_bm_RBPrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RBP && AMD64_OP_BASE(op) != AMD64_REG_R13)
	return FALSE;
      break;
    case amd64_bm_DHrex:
      if(AMD64_OP_BASE(op) != AMD64_REG_RSI && AMD64_OP_BASE(op) != AMD64_REG_R14 && AMD64_OP_BASE(op) != AMD64_REG_RDX)
        return FALSE;
      break;
    case amd64_bm_CS:
      if (AMD64_OP_BASE(op) != AMD64_REG_CS)
	return FALSE;
      else
	return TRUE;
    case amd64_bm_DS:
      if (AMD64_OP_BASE(op) != AMD64_REG_DS)
	return FALSE;
      else
	return TRUE;
    case amd64_bm_ES:
      if (AMD64_OP_BASE(op) != AMD64_REG_ES)
	return FALSE;
      else
	return TRUE;
    case amd64_bm_FS:
      if (AMD64_OP_BASE(op) != AMD64_REG_FS)
	return FALSE;
      else
	return TRUE;
    case amd64_bm_GS:
      if (AMD64_OP_BASE(op) != AMD64_REG_GS)
	return FALSE;
      else
	return TRUE;
    case amd64_bm_SS:
      if (AMD64_OP_BASE(op) != AMD64_REG_SS)
	return FALSE;
      else
	return TRUE;
    default:
      return FALSE;
  }

  /* check if we've got the right byte mode */
  switch (bm)
  {
    case amd64_bm_AL:
    case amd64_bm_BL:
    case amd64_bm_CL:
    case amd64_bm_DL:
    case amd64_bm_R8B:
    case amd64_bm_R9B:
    case amd64_bm_R10B:
    case amd64_bm_R11B:
    case amd64_bm_R12B:
    case amd64_bm_R13B:
    case amd64_bm_R14B:
    case amd64_bm_R15B:
    case amd64_bm_ALrex:
    case amd64_bm_BLrex:
    case amd64_bm_CLrex:
    case amd64_bm_DLrex:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_lo8)
	return FALSE;
      return TRUE;
    case amd64_bm_AHrex:
    case amd64_bm_BHrex:
    case amd64_bm_CHrex:
    case amd64_bm_DHrex:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_lo8 && AMD64_OP_REGMODE(op) != amd64_regmode_hi8)
	return FALSE;
      return TRUE;
    case amd64_bm_AH:
    case amd64_bm_BH:
    case amd64_bm_CH:
    case amd64_bm_DH:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_hi8)
	return FALSE;
      return TRUE;
    case amd64_bm_AX:
    case amd64_bm_BX:
    case amd64_bm_CX:
    case amd64_bm_DX:
    case amd64_bm_SI:
    case amd64_bm_DI:
    case amd64_bm_SP:
    case amd64_bm_BP:
    case amd64_bm_R8W:
    case amd64_bm_R9W:
    case amd64_bm_R10W:
    case amd64_bm_R11W:
    case amd64_bm_R12W:
    case amd64_bm_R13W:
    case amd64_bm_R14W:
    case amd64_bm_R15W:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_lo16)
	return FALSE;
      return TRUE;
    case amd64_bm_EAX:
    case amd64_bm_EBX:
    case amd64_bm_ECX:
    case amd64_bm_EDX:
    case amd64_bm_ESI:
    case amd64_bm_EDI:
    case amd64_bm_ESP:
    case amd64_bm_EBP:
    case amd64_bm_R8D:
    case amd64_bm_R9D:
    case amd64_bm_R10D:
    case amd64_bm_R11D:
    case amd64_bm_R12D:
    case amd64_bm_R13D:
    case amd64_bm_R14D:
    case amd64_bm_R15D:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_lo32)
	return FALSE;
      return TRUE;
    case amd64_bm_RAX:
    case amd64_bm_RBX:
    case amd64_bm_RCX:
    case amd64_bm_RDX:
    case amd64_bm_RSI:
    case amd64_bm_RDI:
    case amd64_bm_RSP:
    case amd64_bm_RBP:
    case amd64_bm_RAXrex:
    case amd64_bm_RBXrex:
    case amd64_bm_RCXrex:
    case amd64_bm_RDXrex:
    case amd64_bm_RSIrex:
    case amd64_bm_RDIrex:
    case amd64_bm_RSPrex:
    case amd64_bm_RBPrex:
    case amd64_bm_R8:
    case amd64_bm_R9:
    case amd64_bm_R10:
    case amd64_bm_R11:
    case amd64_bm_R12:
    case amd64_bm_R13:
    case amd64_bm_R14:
    case amd64_bm_R15:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_full64)
        return FALSE;
      return TRUE;
    case amd64_bm_eAX:
    case amd64_bm_eBX:
    case amd64_bm_eCX:
    case amd64_bm_eDX:
    case amd64_bm_eSI:
    case amd64_bm_eDI:
    case amd64_bm_eSP:
    case amd64_bm_eBP:
    case amd64_bm_e8:
    case amd64_bm_e9:
    case amd64_bm_e10:
    case amd64_bm_e11:
    case amd64_bm_e12:
    case amd64_bm_e13:
    case amd64_bm_e14:
    case amd64_bm_e15:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_lo32 && AMD64_OP_REGMODE(op) != amd64_regmode_lo16)
	return FALSE;
      return TRUE;
    case amd64_bm_rAX:
    case amd64_bm_rBX:
    case amd64_bm_rCX:
    case amd64_bm_rDX:
    case amd64_bm_rSI:
    case amd64_bm_rDI:
    case amd64_bm_rSP:
    case amd64_bm_rBP:
    case amd64_bm_r8:
    case amd64_bm_r9:
    case amd64_bm_r10:
    case amd64_bm_r11:
    case amd64_bm_r12:
    case amd64_bm_r13:
    case amd64_bm_r14:
    case amd64_bm_r15:
    case amd64_bm_rAXrex:
    case amd64_bm_rBXrex:
    case amd64_bm_rCXrex:
    case amd64_bm_rDXrex:
    case amd64_bm_rSIrex:
    case amd64_bm_rDIrex:
    case amd64_bm_rSPrex:
    case amd64_bm_rBPrex:
      if (AMD64_OP_REGMODE(op) != amd64_regmode_full64 && AMD64_OP_REGMODE(op) != amd64_regmode_lo32 && AMD64_OP_REGMODE(op) != amd64_regmode_lo16)
	return FALSE;
      return TRUE;
    default:
      return FALSE;
  }
  return FALSE;
}

t_bool Amd64OpCheckST(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) != amd64_optype_reg)
    return FALSE;

  switch (bm)
  {
    case amd64_bm_ST:
    case amd64_bm_ST0:
      return AMD64_OP_BASE(op) == AMD64_REG_ST0;
    case amd64_bm_ST1:
      return AMD64_OP_BASE(op) == AMD64_REG_ST1;
    case amd64_bm_ST2:
      return AMD64_OP_BASE(op) == AMD64_REG_ST2;
    case amd64_bm_ST3:
      return AMD64_OP_BASE(op) == AMD64_REG_ST3;
    case amd64_bm_ST4:
      return AMD64_OP_BASE(op) == AMD64_REG_ST4;
    case amd64_bm_ST5:
      return AMD64_OP_BASE(op) == AMD64_REG_ST5;
    case amd64_bm_ST6:
      return AMD64_OP_BASE(op) == AMD64_REG_ST6;
    case amd64_bm_ST7:
      return AMD64_OP_BASE(op) == AMD64_REG_ST7;
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool Amd64OpCheckConst1(t_amd64_operand * op, t_amd64_bytemode bm)
{
  return (AMD64_OP_TYPE(op) == amd64_optype_imm) && (AMD64_OP_IMMEDIATE(op) == 1) && (AMD64_OP_IMMEDSIZE(op) == 1);
}

t_bool Amd64OpCheckA(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (bm == amd64_bm_p)
  {
    if (AMD64_OP_TYPE(op) == amd64_optype_farptr)
      return TRUE;
  }
  else
    FATAL(("Unexpected byte mode with A"));
  return FALSE;
}

t_bool Amd64OpCheckC(t_amd64_operand * op, t_amd64_bytemode bm)
{
  return (AMD64_OP_TYPE(op) == amd64_optype_reg) && Amd64IsControlReg(AMD64_OP_BASE(op));
}

t_bool Amd64OpCheckD(t_amd64_operand * op, t_amd64_bytemode bm)
{
  return (AMD64_OP_TYPE(op) == amd64_optype_reg) && Amd64IsDebugReg(AMD64_OP_BASE(op));
}

t_bool Amd64OpCheckE(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_reg)
  {
    if (!Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op))) return FALSE;
    switch (bm)
    {
      case amd64_bm_b:
	return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo8) || (AMD64_OP_REGMODE(op) == amd64_regmode_hi8));
      case amd64_bm_w:
	return (AMD64_OP_REGMODE(op) == amd64_regmode_lo16);
      case amd64_bm_d:
	return (AMD64_OP_REGMODE(op) == amd64_regmode_lo32);
      case amd64_bm_q:
	return (AMD64_OP_REGMODE(op) == amd64_regmode_full64);
      case amd64_bm_z:
	return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo16) || (AMD64_OP_REGMODE(op) == amd64_regmode_lo32));
      case amd64_bm_v8:
        return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo16) || (AMD64_OP_REGMODE(op) == amd64_regmode_full64));
      case amd64_bm_v48:
	return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo32) || (AMD64_OP_REGMODE(op) == amd64_regmode_full64));
      case amd64_bm_v:
	return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo16) || (AMD64_OP_REGMODE(op) == amd64_regmode_lo32) || (AMD64_OP_REGMODE(op) == amd64_regmode_full64));
      default:
	FATAL(("unexpected byte mode"));
    }
  }
  else if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    t_int32 simm = (t_int32) AMD64_OP_IMMEDIATE(op);

    /* special cases */
    if (AMD64_OP_BASE(op) == AMD64_REG_NONE && AMD64_OP_IMMEDSIZE(op) != 4)
      return FALSE;
    if ((AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_R13)&& AMD64_OP_IMMEDSIZE(op)==0)
      return FALSE;
    /* check for immediate sizes */
    if (AMD64_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
    if (AMD64_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
    if (AMD64_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
      
    switch (bm)
    {
      case amd64_bm_b:
	return AMD64_OP_MEMOPSIZE(op) == 1;
      case amd64_bm_w:
	return AMD64_OP_MEMOPSIZE(op) == 2;
      case amd64_bm_d:
	return AMD64_OP_MEMOPSIZE(op) == 4;
      case amd64_bm_q:
        return AMD64_OP_MEMOPSIZE(op) == 8;
      case amd64_bm_z:
	return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4);
      case amd64_bm_v8:
        return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 8);
      case amd64_bm_v:
        return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 8);
      case amd64_bm_p:
	return AMD64_OP_MEMOPSIZE(op) == 6;
      default:
	FATAL(("unexpected bytemode"));
    }
  }
  return FALSE;
}

t_bool Amd64OpCheckF(t_amd64_operand * op, t_amd64_bytemode bm)
{
  /* the flags register is implicit (only used in pushf and popf),
   * so we expect to see an empty operand here */
  return AMD64_OP_TYPE(op) == amd64_optype_none;
}

t_bool Amd64OpCheckG(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if ((AMD64_OP_TYPE(op) == amd64_optype_reg) && Amd64IsGeneralPurposeReg(AMD64_OP_BASE(op)))
  {
    switch (bm)
    {
      case amd64_bm_b:
        return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo8) || (AMD64_OP_REGMODE(op) == amd64_regmode_hi8));
      case amd64_bm_w:
        return (AMD64_OP_REGMODE(op) == amd64_regmode_lo16);
      case amd64_bm_d:
        return (AMD64_OP_REGMODE(op) == amd64_regmode_lo32);
      case amd64_bm_q:
        return (AMD64_OP_REGMODE(op) == amd64_regmode_full64);
      case amd64_bm_z:
        return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo16) || (AMD64_OP_REGMODE(op) == amd64_regmode_lo32));
      case amd64_bm_v48:
	return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo32) || (AMD64_OP_REGMODE(op) == amd64_regmode_full64));
      case amd64_bm_v:
        return ((AMD64_OP_REGMODE(op) == amd64_regmode_lo16) || (AMD64_OP_REGMODE(op) == amd64_regmode_lo32) || (AMD64_OP_REGMODE(op) == amd64_regmode_full64));
      default:
        FATAL(("unexpected byte mode"));
    }
  }
  return FALSE;
}

/*new functions that dont care as much about the IMMED_SIZE {{{ */
t_bool Amd64OpChecksI2(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) != amd64_optype_imm) return FALSE;
   if (bm == amd64_bm_b)
   {
     t_int32 sval = (t_int32) AMD64_OP_IMMEDIATE(op);
     return (-128 <= sval) && (sval < 128);
   }
   else
     FATAL(("unexpected byte mode"));
   return FALSE;
}

t_bool Amd64OpCheckI2(t_amd64_operand * op, t_amd64_bytemode bm)
{
  t_uint32 imm = AMD64_OP_IMMEDIATE(op);
  if (AMD64_OP_TYPE(op) != amd64_optype_imm) return FALSE;
  
  switch (bm)
  {
    case amd64_bm_b:
      return imm < 256;
    case amd64_bm_w:
      return imm < 65536;
    case amd64_bm_d:
      return TRUE;
    case amd64_bm_v:
      return (AMD64_OP_IMMEDSIZE(op) == 2 && imm < 65536) || (AMD64_OP_IMMEDSIZE(op) == 4);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}
/*}}}*/

t_bool Amd64OpChecksI(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) != amd64_optype_imm) return FALSE;
  if (bm == amd64_bm_b)
  {
    if ((AMD64_OP_TYPE(op) == amd64_optype_imm) && (AMD64_OP_IMMEDSIZE(op) == 1))
    {
      t_int32 sval = (t_int32) AMD64_OP_IMMEDIATE(op);
      return (-128 <= sval) && (sval < 128);
    }
  }
  else
    FATAL(("unexpected byte mode"));
  return FALSE;
}

t_bool Amd64OpCheckI(t_amd64_operand * op, t_amd64_bytemode bm)
{
  t_uint64 imm = AMD64_OP_IMMEDIATE(op);
  if (AMD64_OP_TYPE(op) != amd64_optype_imm) return FALSE;
  switch (bm)
  {
    case amd64_bm_b:
      return AMD64_OP_IMMEDSIZE(op) == 1 && imm < 256;
    case amd64_bm_w:
      return AMD64_OP_IMMEDSIZE(op) == 2 && imm < 65536;
    case amd64_bm_d:
      return AMD64_OP_IMMEDSIZE(op) == 4 && imm < 4294967296ULL; 
    case amd64_bm_z:
      return (AMD64_OP_IMMEDSIZE(op) == 2 && imm < 65536) || (AMD64_OP_IMMEDSIZE(op) == 4);
    case amd64_bm_v:
      return (AMD64_OP_IMMEDSIZE(op) == 2 && imm < 65536) || (AMD64_OP_IMMEDSIZE(op) == 4 && imm < 4294967296ULL) || (AMD64_OP_IMMEDSIZE(op) == 8);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool Amd64OpCheckJ(t_amd64_operand * op, t_amd64_bytemode bm)
{
  t_int64 imm = AMD64_OP_IMMEDIATE(op);
  if (AMD64_OP_TYPE(op) != amd64_optype_imm) return FALSE;
  switch (bm)
  {
    case amd64_bm_b:
      return AMD64_OP_IMMEDSIZE(op) == 1 && imm <= 127 && imm >= -128;
    case amd64_bm_w:
      return AMD64_OP_IMMEDSIZE(op) == 2 && imm <= 32767 && imm >= -32768;
    case amd64_bm_d:
      return AMD64_OP_IMMEDSIZE(op) == 4 && imm <= 2147483647LL && imm >= -2147483648LL;
    case amd64_bm_z:
      return (AMD64_OP_IMMEDSIZE(op) == 2 && imm <= 32767 && imm >= -32768) 
	  || (AMD64_OP_IMMEDSIZE(op) == 4 && imm <= 2147483647LL && imm >= -2147483648LL);
    case amd64_bm_v:
      return (AMD64_OP_IMMEDSIZE(op) == 2 && imm <= 32767 && imm >= -32768) 
          || (AMD64_OP_IMMEDSIZE(op) == 4 && imm <= 2147483647LL && imm >= -2147483648LL) 
	  || (AMD64_OP_IMMEDSIZE(op) == 8);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool Amd64OpCheckM(t_amd64_operand * op, t_amd64_bytemode bm)
{
  t_int32 simm = (t_int32) AMD64_OP_IMMEDIATE(op);

  if (AMD64_OP_TYPE(op) != amd64_optype_mem)
    return FALSE;

  /* special cases */
  if (AMD64_OP_BASE(op) == AMD64_REG_NONE && AMD64_OP_IMMEDSIZE(op) != 4)
    return FALSE;
  if ((AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_R13 )&& AMD64_OP_IMMEDSIZE(op) == 0)
    return FALSE;
  if (AMD64_OP_INDEX(op) == AMD64_REG_RSP)
    return FALSE;

  /* check for immediate sizes */
  if (AMD64_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
  if (AMD64_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
  if (AMD64_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
 
  switch (bm)
  {
    case 0:
      return AMD64_OP_MEMOPSIZE(op) == 0;
    case amd64_bm_b:
      return AMD64_OP_MEMOPSIZE(op) == 1;
    case amd64_bm_p:
      return (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 6); 
    case amd64_bm_s:
      return AMD64_OP_MEMOPSIZE(op) == 6; 

    /* the fpu memory operand bytemodes */
    case amd64_bm_sr:
      return AMD64_OP_MEMOPSIZE(op) == 4;
    case amd64_bm_dr:
      return AMD64_OP_MEMOPSIZE(op) == 8;
    case amd64_bm_er:
    case amd64_bm_bcd:
      return AMD64_OP_MEMOPSIZE(op) == 10;
    case amd64_bm_w:
      return AMD64_OP_MEMOPSIZE(op) == 2;
    case amd64_bm_d:
      return AMD64_OP_MEMOPSIZE(op) == 4;
    case amd64_bm_q:
      return AMD64_OP_MEMOPSIZE(op) == 8;
    case amd64_bm_2byte:
      return AMD64_OP_MEMOPSIZE(op) == 2;
    case amd64_bm_28byte:
      return AMD64_OP_MEMOPSIZE(op) == 28;
    case amd64_bm_108byte:
      return AMD64_OP_MEMOPSIZE(op) == 108;
    case amd64_bm_512byte:
      return AMD64_OP_MEMOPSIZE(op) == 512;

    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool Amd64OpCheckO(t_amd64_operand * op, t_amd64_bytemode bm)
{
  /* memory operand, but no base, index or scale can be specified */
  if (AMD64_OP_TYPE(op) != amd64_optype_mem) return FALSE;
  if ((AMD64_OP_BASE(op) != AMD64_REG_NONE) || (AMD64_OP_INDEX(op) != AMD64_REG_NONE)) return FALSE;
  if (AMD64_OP_IMMEDSIZE(op) != 2 && AMD64_OP_IMMEDSIZE(op) != 4) return FALSE;
  switch (bm)
  {
    case amd64_bm_b:
      return AMD64_OP_MEMOPSIZE(op) == 1;
    case amd64_bm_z:
      return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4);
    case amd64_bm_v:
      return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 8);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}

t_bool Amd64OpCheckO8(t_amd64_operand * op, t_amd64_bytemode bm)
{
  /* memory operand, but no base, index or scale can be specified */
  if (AMD64_OP_TYPE(op) != amd64_optype_mem) return FALSE;
  if ((AMD64_OP_BASE(op) != AMD64_REG_NONE) || (AMD64_OP_INDEX(op) != AMD64_REG_NONE)) return FALSE;
  if (AMD64_OP_IMMEDSIZE(op) != 8) return FALSE;
  switch (bm)
  {
    case amd64_bm_b:
      return AMD64_OP_MEMOPSIZE(op) == 1;
    case amd64_bm_z:
      return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4);
    case amd64_bm_v:
      return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 8);
    default:
      FATAL(("unexpected byte mode"));
  }
  return FALSE;
}


t_bool Amd64OpCheckR(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) != amd64_optype_reg) return FALSE;
  return Amd64OpCheckE(op,bm);
}

t_bool Amd64OpCheckS(t_amd64_operand * op, t_amd64_bytemode bm)
{
  return (AMD64_OP_TYPE(op) == amd64_optype_reg) && Amd64IsSegmentReg(AMD64_OP_BASE(op));
}

t_bool Amd64OpCheckX(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    if ((AMD64_OP_BASE(op) == AMD64_REG_RSI) && (AMD64_OP_INDEX(op) == AMD64_REG_NONE) && (AMD64_OP_IMMEDIATE(op) == 0))
    {
      switch (bm)
      {
	case amd64_bm_b:
	  return AMD64_OP_MEMOPSIZE(op) == 1;
        case amd64_bm_z:
          return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4);
        case amd64_bm_v:
	  return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 8);
	default:
	  FATAL(("unexpected byte mode"));
      }
    }
  }
  return FALSE;
}

t_bool Amd64OpCheckY(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    if ((AMD64_OP_BASE(op) == AMD64_REG_RDI) && (AMD64_OP_INDEX(op) == AMD64_REG_NONE) && (AMD64_OP_IMMEDIATE(op) == 0))
    {
      switch (bm){
        case amd64_bm_b:
	  return AMD64_OP_MEMOPSIZE(op) == 1;
        case amd64_bm_v:
  	  return (AMD64_OP_MEMOPSIZE(op) == 2) || (AMD64_OP_MEMOPSIZE(op) == 4) || (AMD64_OP_MEMOPSIZE(op) == 8);
        default:
  	  FATAL(("unexpected byte mode"));
      }
    }
  }
  return FALSE;
}

t_bool Amd64OpCheckV(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_reg)
  {
    if (Amd64IsSSEReg(AMD64_OP_BASE(op))){
      return TRUE;
    }
  }
  return FALSE;
}

t_bool Amd64OpCheckW(t_amd64_operand * op, t_amd64_bytemode bm)
{
  if (AMD64_OP_TYPE(op) == amd64_optype_reg)
  {
    if (Amd64IsSSEReg(AMD64_OP_BASE(op)))
      return TRUE;
  }
  else if (AMD64_OP_TYPE(op) == amd64_optype_mem)
  {
    t_int64 simm = (t_int32) AMD64_OP_IMMEDIATE(op);

    /* special cases */
    if (AMD64_OP_BASE(op) == AMD64_REG_NONE && AMD64_OP_IMMEDSIZE(op) != 4)
      return FALSE;
    if ((AMD64_OP_BASE(op) == AMD64_REG_RBP || AMD64_OP_BASE(op) == AMD64_REG_R13 ) && AMD64_OP_IMMEDSIZE(op) == 0)
      return FALSE;

    /* check for immediate sizes */
      if (AMD64_OP_IMMEDSIZE(op) == 0 && simm != 0) return FALSE;
      if (AMD64_OP_IMMEDSIZE(op) == 1 && (simm < -128 || simm > 127)) return FALSE;
      if (AMD64_OP_IMMEDSIZE(op) == 2 && (simm < -32768 || simm > 32767)) return FALSE;
      if (AMD64_OP_IMMEDSIZE(op) == 4 && (simm < -2147483648LL || simm > 2147483647LL)) return FALSE;
  
      switch (bm)
      {
	case amd64_bm_sd:
	  return AMD64_OP_MEMOPSIZE(op) == 8;
	case amd64_bm_ss:
          return AMD64_OP_MEMOPSIZE(op) == 4;
	case amd64_bm_ps:
	  return AMD64_OP_MEMOPSIZE(op) == 16; 
	case amd64_bm_pd:
	  return AMD64_OP_MEMOPSIZE(op) == 16;
	case amd64_bm_dq:
	  return AMD64_OP_MEMOPSIZE(op) == 16;
	case amd64_bm_q:
	  return AMD64_OP_MEMOPSIZE(op) == 8;
      default:
        FATAL(("unexpected bytemode"));
    }
  }
  return FALSE;
}

/* vim: set shiftwidth=2 foldmethod=marker: */
