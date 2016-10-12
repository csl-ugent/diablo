#include <diabloamd64.h>

static t_string bmodes[] = {
   "amd64_bm_NONE",
   "amd64_bm_AHrex","amd64_bm_AH","amd64_bm_ALrex","amd64_bm_AL","amd64_bm_AXrex","amd64_bm_AX","amd64_bm_EAX","amd64_bm_RAX","amd64_bm_rAXrex","amd64_bm_eAX","amd64_bm_rAX",
   "amd64_bm_BHrex","amd64_bm_BH","amd64_bm_BLrex","amd64_bm_BL","amd64_bm_BXrex","amd64_bm_BX","amd64_bm_EBX","amd64_bm_RBX","amd64_bm_rBXrex","amd64_bm_eBX","amd64_bm_rBX",
   "amd64_bm_CHrex","amd64_bm_CH","amd64_bm_CLrex","amd64_bm_CL","amd64_bm_CXrex","amd64_bm_CX","amd64_bm_ECX","amd64_bm_RCX","amd64_bm_rCXrex","amd64_bm_eCX","amd64_bm_rCX",
   "amd64_bm_DHrex","amd64_bm_DH","amd64_bm_DLrex","amd64_bm_DL","amd64_bm_DXrex","amd64_bm_DX","amd64_bm_EDX","amd64_bm_RDX","amd64_bm_rDXrex","amd64_bm_eDX","amd64_bm_rDX",
   "amd64_bm_SI" ,"amd64_bm_DI","amd64_bm_SP","amd64_bm_BP",
   "amd64_bm_ESI","amd64_bm_EDI","amd64_bm_ESP","amd64_bm_EBP",
   "amd64_bm_RSI","amd64_bm_RDI","amd64_bm_RSP","amd64_bm_RBP",
   "amd64_bm_rSI","amd64_bm_rDI","amd64_bm_rSP","amd64_bm_rBP",
   "amd64_bm_eSI","amd64_bm_eDI","amd64_bm_eSP","amd64_bm_eBP",
   "amd64_bm_rSIrex","amd64_bm_rDIrex","amd64_bm_rSPrex","amd64_bm_rBPrex",
   "amd64_bm_R8B","amd64_bm_R9B","amd64_bm_R10B","amd64_bm_R11B","amd64_bm_R12B","amd64_bm_R13B","amd64_bm_R14B","amd64_bm_R15B",
   "amd64_bm_R8W","amd64_bm_R9W","amd64_bm_R10W","amd64_bm_R11W","amd64_bm_R12W","amd64_bm_R13W","amd64_bm_R14W","amd64_bm_R15W",
   "amd64_bm_R8D","amd64_bm_R9D","amd64_bm_R10D","amd64_bm_R11D","amd64_bm_R12D","amd64_bm_R13D","amd64_bm_R14D","amd64_bm_R15D",
   "amd64_bm_R8" ,"amd64_bm_R9","amd64_bm_R10","amd64_bm_R11","amd64_bm_R12","amd64_bm_R13","amd64_bm_R14","amd64_bm_R15",
   "amd64_bm_r8","amd64_bm_r9","amd64_bm_r10","amd64_bm_r11","amd64_bm_r12","amd64_bm_r13","amd64_bm_r14","amd64_bm_r15",
   "amd64_bm_e8","amd64_bm_e9","amd64_bm_e10","amd64_bm_e11","amd64_bm_e12","amd64_bm_e13","amd64_bm_e14","amd64_bm_e15",
   "amd64_bm_CS","amd64_bm_DS","amd64_bm_ES","amd64_bm_FS","amd64_bm_GS","amd64_bm_SS",
   "amd64_bm_ST","amd64_bm_ST0","amd64_bm_ST1","amd64_bm_ST2","amd64_bm_ST3","amd64_bm_ST4","amd64_bm_ST5","amd64_bm_ST6","amd64_bm_ST7",
   "amd64_bm_b","amd64_bm_w","amd64_bm_d","amd64_bm_v","amd64_bm_z","amd64_bm_q",
   "amd64_bm_a","amd64_bm_p","amd64_bm_s",
   "amd64_bm_sr","amd64_bm_dr","amd64_bm_er","amd64_bm_bcd",
   "amd64_bm_2byte","amd64_bm_28byte","amd64_bm_108byte","amd64_bm_512byte","amd64_bm_unknown","amd64_bm_sd","amd64_bm_ss","amd64_bm_ps","amd64_bm_pd","amd64_bm_dq"
};

static t_reg regs64[] = {
  AMD64_REG_RAX, AMD64_REG_RCX, AMD64_REG_RDX, AMD64_REG_RBX,  
  AMD64_REG_RSP, AMD64_REG_RBP, AMD64_REG_RSI, AMD64_REG_RDI,
  AMD64_REG_R8 , AMD64_REG_R9 , AMD64_REG_R10, AMD64_REG_R11,
  AMD64_REG_R12, AMD64_REG_R13, AMD64_REG_R14, AMD64_REG_R15
};
static t_reg regs32[] = {
  AMD64_REG_RAX, AMD64_REG_RCX, AMD64_REG_RDX, AMD64_REG_RBX,
  AMD64_REG_RSP, AMD64_REG_RBP, AMD64_REG_RSI, AMD64_REG_RDI,
  AMD64_REG_R8 , AMD64_REG_R9 , AMD64_REG_R10, AMD64_REG_R11,
  AMD64_REG_R12, AMD64_REG_R13, AMD64_REG_R14, AMD64_REG_R15
};

static t_reg regs16[] = {
  AMD64_REG_RAX, AMD64_REG_RCX, AMD64_REG_RDX, AMD64_REG_RBX,
  AMD64_REG_RSP, AMD64_REG_RBP, AMD64_REG_RSI, AMD64_REG_RDI,
  AMD64_REG_R8 , AMD64_REG_R9 , AMD64_REG_R10, AMD64_REG_R11,
  AMD64_REG_R12, AMD64_REG_R13, AMD64_REG_R14, AMD64_REG_R15
};

static t_reg regs8[] = {
  AMD64_REG_RAX, AMD64_REG_RCX, AMD64_REG_RDX, AMD64_REG_RBX,
  AMD64_REG_RAX, AMD64_REG_RCX, AMD64_REG_RDX, AMD64_REG_RBX,
  AMD64_REG_R8 , AMD64_REG_R9 , AMD64_REG_R10, AMD64_REG_R11,
  AMD64_REG_R12, AMD64_REG_R13, AMD64_REG_R14, AMD64_REG_R15
};

static t_reg regs_seg[] = {
  AMD64_REG_ES, AMD64_REG_CS, AMD64_REG_SS, AMD64_REG_DS,
  AMD64_REG_FS, AMD64_REG_GS, AMD64_REG_NONE, AMD64_REG_NONE
};

t_uint32 Amd64OpDisNone(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm,char rex)
{
  /* this is the operand disassembly function that goes with NULL_OP: it does
   * nothing */
  AMD64_OP_TYPE(op) = amd64_optype_none;
  return 0;
}

t_uint32 Amd64OpDisReg(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  switch (bm){
    case amd64_bm_rAXrex:
      bm=(rexb(rex))?amd64_bm_r8:amd64_bm_rAX;
      break;
    case amd64_bm_rCXrex:
      bm=(rexb(rex))?amd64_bm_r9:amd64_bm_rCX;
      break;
    case amd64_bm_rDXrex:
      bm=(rexb(rex))?amd64_bm_r10:amd64_bm_rDX;
      break;
    case amd64_bm_rBXrex:
      bm=(rexb(rex))?amd64_bm_r11:amd64_bm_rBX;
      break;
    case amd64_bm_rSPrex:
      bm=(rexb(rex))?amd64_bm_r12:amd64_bm_rSP;
      break;
   case amd64_bm_rBPrex:
      bm=(rexb(rex))?amd64_bm_r13:amd64_bm_rBP;
      break;
   case amd64_bm_rSIrex:
      bm=(rexb(rex))?amd64_bm_r14:amd64_bm_rSI;
      break;
   case amd64_bm_rDIrex:
      bm=(rexb(rex))?amd64_bm_r15:amd64_bm_rDI;
      break;
    case amd64_bm_RAXrex:
      bm=(rexb(rex))?amd64_bm_R8:amd64_bm_RAX;
      break;
    case amd64_bm_RCXrex:
      bm=(rexb(rex))?amd64_bm_R9:amd64_bm_RCX;
      break;
    case amd64_bm_RDXrex:
      bm=(rexb(rex))?amd64_bm_R10:amd64_bm_RDX;
      break;
    case amd64_bm_RBXrex:
      bm=(rexb(rex))?amd64_bm_R11:amd64_bm_RBX;
      break;
    case amd64_bm_RSPrex:
      bm=(rexb(rex))?amd64_bm_R12:amd64_bm_RSP;
      break;
    case amd64_bm_RBPrex:
      bm=(rexb(rex))?amd64_bm_R13:amd64_bm_RBP;
      break;
    case amd64_bm_RSIrex:
      bm=(rexb(rex))?amd64_bm_R14:amd64_bm_RSI;
      break;
    case amd64_bm_RDIrex:
      bm=(rexb(rex))?amd64_bm_R15:amd64_bm_RDI;
      break;
   case amd64_bm_ALrex:
      bm=(rexb(rex))?amd64_bm_R8B:amd64_bm_AL;
      break;
   case amd64_bm_CLrex:
      bm=(rexb(rex))?amd64_bm_R9B:amd64_bm_CL;
      break;
   case amd64_bm_DLrex:
      bm=(rexb(rex))?amd64_bm_R10B:amd64_bm_DL;
      break;
   case amd64_bm_BLrex:
      bm=(rexb(rex))?amd64_bm_R11B:amd64_bm_BL;
      break;
   case amd64_bm_AHrex:
      bm=(rexb(rex))?amd64_bm_R12B:amd64_bm_AH;
      break;
   case amd64_bm_CHrex:
      bm=(rexb(rex))?amd64_bm_R13B:amd64_bm_CH;
      break;
   case amd64_bm_DHrex:
      bm=(rexb(rex))?amd64_bm_R14B:amd64_bm_DH;
      break;
   case amd64_bm_BHrex:
      bm=(rexb(rex))?amd64_bm_R15B:amd64_bm_BH;
      break;
   default:
      break;
  }
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  switch (bm)
  {
    case amd64_bm_AL: case amd64_bm_AH: case amd64_bm_AX: case amd64_bm_EAX: case amd64_bm_RAX: case amd64_bm_rAX:
      AMD64_OP_BASE(op) = AMD64_REG_RAX;
      break;
    case amd64_bm_BL: case amd64_bm_BH: case amd64_bm_BX: case amd64_bm_EBX: case amd64_bm_RBX:  case amd64_bm_rBX:
      AMD64_OP_BASE(op) = AMD64_REG_RBX;
      break;
    case amd64_bm_CL: case amd64_bm_CH: case amd64_bm_CX: case amd64_bm_ECX: case amd64_bm_RCX:  case amd64_bm_rCX:
      AMD64_OP_BASE(op) = AMD64_REG_RCX;
      break;
    case amd64_bm_DL: case amd64_bm_DH: case amd64_bm_DX: case amd64_bm_EDX: case amd64_bm_RDX:  case amd64_bm_rDX:
      AMD64_OP_BASE(op) = AMD64_REG_RDX;
      break;
    case amd64_bm_rSI: case amd64_bm_ESI: case amd64_bm_RSI: case amd64_bm_SI:
      AMD64_OP_BASE(op) = AMD64_REG_RSI;
      break;
    case amd64_bm_rDI: case amd64_bm_EDI: case amd64_bm_RDI: case amd64_bm_DI:
      AMD64_OP_BASE(op) = AMD64_REG_RDI;
      break;
    case amd64_bm_rSP: case amd64_bm_ESP: case amd64_bm_RSP: case amd64_bm_SP:
      AMD64_OP_BASE(op) = AMD64_REG_RSP;
      break;
    case amd64_bm_rBP: case amd64_bm_EBP: case amd64_bm_RBP: case amd64_bm_BP:
      AMD64_OP_BASE(op) = AMD64_REG_RBP;
      break;    
    case amd64_bm_r8:  case amd64_bm_e8:  case amd64_bm_R8D:  case amd64_bm_R8W:  case amd64_bm_R8B:  case amd64_bm_R8:
      AMD64_OP_BASE(op) = AMD64_REG_R8;
      break;
    case amd64_bm_r9:  case amd64_bm_e9:  case amd64_bm_R9D:  case amd64_bm_R9W:  case amd64_bm_R9B:  case amd64_bm_R9:
      AMD64_OP_BASE(op) = AMD64_REG_R9;
      break;
    case amd64_bm_r10: case amd64_bm_e10: case amd64_bm_R10D: case amd64_bm_R10W: case amd64_bm_R10B: case amd64_bm_R10:
      AMD64_OP_BASE(op) = AMD64_REG_R10;
      break;
    case amd64_bm_r11: case amd64_bm_e11: case amd64_bm_R11D: case amd64_bm_R11W: case amd64_bm_R11B: case amd64_bm_R11:
      AMD64_OP_BASE(op) = AMD64_REG_R11;
      break;
    case amd64_bm_r12: case amd64_bm_e12: case amd64_bm_R12D: case amd64_bm_R12W: case amd64_bm_R12B: case amd64_bm_R12:
      AMD64_OP_BASE(op) = AMD64_REG_R12;
      break;
    case amd64_bm_r13: case amd64_bm_e13: case amd64_bm_R13D: case amd64_bm_R13W: case amd64_bm_R13B: case amd64_bm_R13:
      AMD64_OP_BASE(op) = AMD64_REG_R13;
      break;
    case amd64_bm_r14: case amd64_bm_e14: case amd64_bm_R14D: case amd64_bm_R14W: case amd64_bm_R14B: case amd64_bm_R14:
      AMD64_OP_BASE(op) = AMD64_REG_R14;
      break;
    case amd64_bm_r15: case amd64_bm_e15: case amd64_bm_R15D: case amd64_bm_R15W: case amd64_bm_R15B: case amd64_bm_R15:
      AMD64_OP_BASE(op) = AMD64_REG_R15;
      break;   
    case amd64_bm_CS:
      AMD64_OP_BASE(op) = AMD64_REG_CS;
      break;
    case amd64_bm_DS:
      AMD64_OP_BASE(op) = AMD64_REG_DS;
      break;
    case amd64_bm_ES:
      AMD64_OP_BASE(op) = AMD64_REG_ES;
      break;
    case amd64_bm_FS:
      AMD64_OP_BASE(op) = AMD64_REG_FS;
      break;
    case amd64_bm_GS:
      AMD64_OP_BASE(op) = AMD64_REG_GS;
      break;
    case amd64_bm_SS:
      AMD64_OP_BASE(op) = AMD64_REG_SS;
      break;
    default:
      FATAL(("unknown byte mode"));
  }

  switch (bm)
  {
    case amd64_bm_AL: case amd64_bm_BL: case amd64_bm_CL: case amd64_bm_DL:
    case amd64_bm_R8B:  case amd64_bm_R9B:  case amd64_bm_R10B: case amd64_bm_R11B:
    case amd64_bm_R12B: case amd64_bm_R13B: case amd64_bm_R14B: case amd64_bm_R15B:
      AMD64_OP_REGMODE(op) = amd64_regmode_lo8;
      break;
    case amd64_bm_AH: case amd64_bm_BH: case amd64_bm_CH: case amd64_bm_DH:
      AMD64_OP_REGMODE(op) = amd64_regmode_hi8;
      break;
    case amd64_bm_AX:   case amd64_bm_BX:   case amd64_bm_CX:   case amd64_bm_DX:
    case amd64_bm_SI:   case amd64_bm_DI:   case amd64_bm_SP:   case amd64_bm_BP:
    case amd64_bm_R8W:  case amd64_bm_R9W:  case amd64_bm_R10W: case amd64_bm_R11W:
    case amd64_bm_R12W: case amd64_bm_R13W: case amd64_bm_R14W: case amd64_bm_R15W:
      AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
      break;
    case amd64_bm_EAX:  case amd64_bm_EBX:  case amd64_bm_ECX:  case amd64_bm_EDX:
    case amd64_bm_ESI:  case amd64_bm_EDI:  case amd64_bm_ESP:  case amd64_bm_EBP:
    case amd64_bm_R8D:  case amd64_bm_R9D:  case amd64_bm_R10D: case amd64_bm_R11D:
    case amd64_bm_R12D: case amd64_bm_R13D: case amd64_bm_R14D: case amd64_bm_R15D:
      AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
    break;
    case amd64_bm_RAX: case amd64_bm_RBX: case amd64_bm_RCX: case amd64_bm_RDX:
    case amd64_bm_RSI: case amd64_bm_RDI: case amd64_bm_RSP: case amd64_bm_RBP:
    case amd64_bm_R8:  case amd64_bm_R9:  case amd64_bm_R10: case amd64_bm_R11:
    case amd64_bm_R12: case amd64_bm_R13: case amd64_bm_R14: case amd64_bm_R15:
    case amd64_bm_CS: case amd64_bm_DS: case amd64_bm_ES: case amd64_bm_FS: case amd64_bm_GS: case amd64_bm_SS:
      AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    break;
    case amd64_bm_eAX:  case amd64_bm_eBX:  case amd64_bm_eCX:  case amd64_bm_eDX:
    case amd64_bm_eSI:  case amd64_bm_eDI:  case amd64_bm_eSP:  case amd64_bm_eBP:
    case amd64_bm_e8:   case amd64_bm_e9:   case amd64_bm_e10:  case amd64_bm_e11:
    case amd64_bm_e12:  case amd64_bm_e13:  case amd64_bm_e14:  case amd64_bm_e15:
    /* these depend on the operand size prefix */
    if (AMD64_OPSZPREF(ins))
      AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
    else
      AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
      break;
    case amd64_bm_rAX:  case amd64_bm_rBX:  case amd64_bm_rCX:  case amd64_bm_rDX:
    case amd64_bm_rSI:  case amd64_bm_rDI:  case amd64_bm_rSP:  case amd64_bm_rBP:
    case amd64_bm_r8:   case amd64_bm_r9:   case amd64_bm_r10:  case amd64_bm_r11:
    case amd64_bm_r12:  case amd64_bm_r13:  case amd64_bm_r14:  case amd64_bm_r15:
    /* these depend on the operand size prefix */
    if (rexw(rex)){
      AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    }else{
    if (AMD64_OPSZPREF(ins))
      AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
        else
          AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
    }
    break;
    default:
      FATAL(("Invalid bytemode %d", bm));
  }
  return 0;
}

t_uint32 Amd64OpDisST(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  
  switch (bm)
  {
    case amd64_bm_ST:
    case amd64_bm_ST0:
      AMD64_OP_BASE(op) = AMD64_REG_ST0;
      break;
    case amd64_bm_ST1:
      AMD64_OP_BASE(op) = AMD64_REG_ST1;
      break;
    case amd64_bm_ST2:
      AMD64_OP_BASE(op) = AMD64_REG_ST2;
      break;
    case amd64_bm_ST3:
      AMD64_OP_BASE(op) = AMD64_REG_ST3;
      break;
    case amd64_bm_ST4:
      AMD64_OP_BASE(op) = AMD64_REG_ST4;
      break;
    case amd64_bm_ST5:
      AMD64_OP_BASE(op) = AMD64_REG_ST5;
      break;
    case amd64_bm_ST6:
      AMD64_OP_BASE(op) = AMD64_REG_ST6;
      break;
    case amd64_bm_ST7:
      AMD64_OP_BASE(op) = AMD64_REG_ST7;
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return 0;
}

t_uint32 Amd64OpDisConst1(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  AMD64_OP_TYPE(op) = amd64_optype_imm;
  AMD64_OP_IMMEDIATE(op) = 1;
  AMD64_OP_IMMEDSIZE(op) = 1;
  return 0;
}

t_uint32 Amd64OpDisA(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  ASSERT(bm == amd64_bm_p,("unknown bytemode"));

  AMD64_OP_TYPE(op) = amd64_optype_farptr;
  if (AMD64_OPSZPREF(ins))
  {
    AMD64_OP_IMMEDIATE(op) = (t_uint32) *(t_uint16 *)codep;
    AMD64_OP_IMMEDSIZE(op) = 2;
    codep += 2;
  }
  else
  {
    AMD64_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
    AMD64_OP_IMMEDSIZE(op) = 4;
    codep += 4;
  }
  AMD64_OP_SEGSELECTOR(op) = *(t_uint16 *)codep;
  return AMD64_OPSZPREF(ins) ? 4 : 6;
}

t_uint32 Amd64OpDisC(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 reg = (modrm >> 3) & 7;
  ASSERT(bm == amd64_bm_d, ("unknown bytemode"));
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_BASE(op) = AMD64_REG_CR0 + reg;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  return 0;
}

t_uint32 Amd64OpDisD(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 reg = (modrm >> 3) & 7;
  if(rexr(rex)) reg+=8;
  ASSERT(bm == amd64_bm_d, ("unknown bytemode"));
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_BASE(op) = AMD64_REG_DR0 + reg;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  return 0;
}

t_uint32 Amd64OpDisE(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 mod, rm, reg;
  t_uint32 scale = 0, index = 0, base = 0;

  ASSERT(modrm != -1, ("need modrm byte, got none"));
  mod = (modrm >> 6) & 3;
  reg = (modrm >> 3) & 7;
  rm = modrm & 7;

  if(rexr(rex))
    reg+=8;
  
  if (mod == 3)
  {
    if(rexb(rex))
      rm+=8;
	  
    /* register */
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    
    switch (bm){
      case amd64_bm_b:
        AMD64_OP_BASE(op) = regs8[rm];
        AMD64_OP_REGMODE(op) = ((rm > 3)&&(rm < 8)) ? amd64_regmode_hi8 : amd64_regmode_lo8;
        break;
      case amd64_bm_w:
        AMD64_OP_BASE(op) = regs16[rm];
        AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
	break;
      case amd64_bm_d:
	AMD64_OP_BASE(op) = regs32[rm];
	AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
	break;
      case amd64_bm_q:
        AMD64_OP_BASE(op) = regs64[rm];
        AMD64_OP_REGMODE(op) = amd64_regmode_full64;
        break;
      case amd64_bm_v:
        if(rexw(rex)){
          AMD64_OP_BASE(op) = regs64[rm];
          AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	}else{
          if(AMD64_OPSZPREF(ins)){
            AMD64_OP_BASE(op) = regs16[rm];
            AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
          }else{
            AMD64_OP_BASE(op) = regs32[rm];
            AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
          }
	}
        break;
      case amd64_bm_v48:
	if(rexw(rex)){
	  AMD64_OP_BASE(op) = regs64[rm];
	  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	}else{
	  AMD64_OP_BASE(op) = regs32[rm];
	  AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
	}
	break;
      case amd64_bm_z:
        if(AMD64_OPSZPREF(ins)){
          AMD64_OP_BASE(op) = regs16[rm];
          AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
	}else{
          AMD64_OP_BASE(op) = regs32[rm];
          AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
	}
	break;
      case amd64_bm_v8:
	if(AMD64_OPSZPREF(ins)){
	  AMD64_OP_BASE(op) = regs16[rm];
	  AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
	}else{
	  AMD64_OP_BASE(op) = regs32[rm];
	  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	}
	break;
      default:
      	printf("%s",bmodes[bm]);
	FATAL(("unknown bytemode"));
    }
    return 0;
  }
  else
  {
    t_uint32 readbytes = 0;
    t_bool has_sib = FALSE;

    /* memory operand */
    AMD64_OP_TYPE(op) = amd64_optype_mem;

    /* we don't support 16-bit addressing */
    ASSERT(!AMD64_ADSZPREF(ins), ("16 bit addressing not supported"));

    if (rm == 4)
    {
      has_sib = TRUE;
      ASSERT(sib != -1, ("need SIB byte but none given"));
      scale = (sib >> 6) & 3;
      index = (sib >> 3) & 7;
      base = sib & 7;
      if(rexx(rex))
	index+=8;
      if(rexb(rex))
	base+=8;
    }else{
      if(rexb(rex))
        rm+=8;
    }
    /* base and index regs */
    AMD64_OP_BASE(op) = regs64[rm];
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;

    /* displacement */
    if (mod == 0)
    {
      if (rm == 5)
      {
	/* special case: disp32, besa = %rip */
	AMD64_OP_BASE(op) = AMD64_REG_RIP;
	AMD64_OP_IMMEDIATE(op) = *(t_uint64 *)codep;
	AMD64_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
      else
      {
	AMD64_OP_IMMEDIATE(op) = 0;
	AMD64_OP_IMMEDSIZE(op) = 0;
      }
    }
    if (mod == 1)
    {
      AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int8 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 1;
      codep += 1;
      readbytes = 1;
    }
    else if (mod == 2)
    {
      AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_uint32 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 4;
      codep += 4;
      readbytes = 4;
    }

    /* handle sib byte if present */
    if (has_sib)
    {
      AMD64_OP_SCALE(op) = scale;
      AMD64_OP_BASE(op) = regs64[base];
      if (index != 4 )  /* can't use %esp as an index register */
	AMD64_OP_INDEX(op) = regs64[index];
      else 
	AMD64_OP_INDEX(op) = AMD64_REG_NONE;

      /* special case: base == 5, mod == 0 */
      if (base == 5 && mod == 0)
      {
	/* just scaled index + disp32, no base */
	AMD64_OP_BASE(op) = AMD64_REG_NONE;
	AMD64_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
	AMD64_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
    }

    /* determine size of the memory operand */

    switch (bm)
    {
      case amd64_bm_b:
	AMD64_OP_MEMOPSIZE(op) = 1;
	break;
      case amd64_bm_w:
	AMD64_OP_MEMOPSIZE(op) = 2;
	break;
      case amd64_bm_d:
	AMD64_OP_MEMOPSIZE(op) = 4;
	break;
      case amd64_bm_v:
	if(rexw(rex)){
	  AMD64_OP_MEMOPSIZE(op) = 8;
	}else{
        if (AMD64_OPSZPREF(ins))
	  AMD64_OP_MEMOPSIZE(op) = 2;
	else
	  AMD64_OP_MEMOPSIZE(op) = 4;
	}
	break;
      case amd64_bm_z:
	if (AMD64_OPSZPREF(ins))
	  AMD64_OP_MEMOPSIZE(op) = 2;
	else
	  AMD64_OP_MEMOPSIZE(op) = 4;
	break;
      case amd64_bm_v8:
	if (AMD64_OPSZPREF(ins))
	  AMD64_OP_MEMOPSIZE(op) = 2;
	else
	  AMD64_OP_MEMOPSIZE(op) = 8;
	break;
      case amd64_bm_p:
        AMD64_OP_MEMOPSIZE(op) = 6;
	break;
      case amd64_bm_sd:
      case amd64_bm_ss:
      case amd64_bm_ps:
      case amd64_bm_pd:
      case amd64_bm_dq:
        AMD64_OP_MEMOPSIZE(op) = 16;
	break;
      case amd64_bm_q:
        AMD64_OP_MEMOPSIZE(op) = 8;
	break;
      default:
	FATAL(("unknown bytemode"));
    }
    
    return readbytes;
  }

  /* keep the compiler happy */
  return 0;
}

t_uint32 Amd64OpDisF(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* this is only used in pushf and popf instructions, and the flags register
   * is already implicit in these instructions, so we do nothing here */
  return 0;
}

t_uint32 Amd64OpDisG(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 reg = (modrm >> 3) & 7;

  if(rexr(rex))
    reg+=8;
   
  ASSERT(modrm != -1, ("need modrm byte but got none"));

  AMD64_OP_TYPE(op) = amd64_optype_reg;
    switch (bm){
      case amd64_bm_b:
        AMD64_OP_BASE(op) = regs8[reg];
        AMD64_OP_REGMODE(op) = ((reg > 3)&&(reg < 8)) ? amd64_regmode_hi8 : amd64_regmode_lo8;
        break;
      case amd64_bm_w:
        AMD64_OP_BASE(op) = regs16[reg];
        AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
        break;
      case amd64_bm_d:
        AMD64_OP_BASE(op) = regs32[reg];
        AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
        break;
      case amd64_bm_q:
        AMD64_OP_BASE(op) = regs64[reg];
        AMD64_OP_REGMODE(op) = amd64_regmode_full64;
        break;
      case amd64_bm_v:
        if(rexw(rex)){
          AMD64_OP_BASE(op) = regs64[reg];
          AMD64_OP_REGMODE(op) = amd64_regmode_full64;
        }else{
          if(AMD64_OPSZPREF(ins)){
            AMD64_OP_BASE(op) = regs16[reg];
            AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
          }else{
            AMD64_OP_BASE(op) = regs32[reg];
            AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
          }
        }
        break;
      case amd64_bm_v48:
	if(rexw(rex)){
	  AMD64_OP_BASE(op) = regs64[reg];
	  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
	}else{
	  AMD64_OP_BASE(op) = regs32[reg];
	  AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
	}
	break;
      case amd64_bm_z:
        if(AMD64_OPSZPREF(ins)){
          AMD64_OP_BASE(op) = regs16[reg];
          AMD64_OP_REGMODE(op) = amd64_regmode_lo16;
        }else{
          AMD64_OP_BASE(op) = regs32[reg];
          AMD64_OP_REGMODE(op) = amd64_regmode_lo32;
        }
	break;
      default:
        FATAL(("unknown bytemode"));
    }
    return 0;
}

t_uint32 Amd64OpDissI(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* sign extended byte immediate */
  ASSERT(bm == amd64_bm_b, ("unknown bytemode"));

  AMD64_OP_TYPE(op) = amd64_optype_imm;
  AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int8 *)codep;
  AMD64_OP_IMMEDSIZE(op) = 1;
  return 1;
}

t_uint32 Amd64OpDisI(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 readbytes = 0;

  /* immediate operand, not sign extended */
  AMD64_OP_TYPE(op) = amd64_optype_imm;
  
  switch (bm)
  {
    case amd64_bm_b:
      AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint8*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 1;
      break;
    case amd64_bm_w:
      AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint16*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
      break;
    case amd64_bm_d:
      AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint32*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
      break;
    case amd64_bm_v:
      if(rexw(rex)){
	 AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint64*) codep;
	 AMD64_OP_IMMEDSIZE(op) = readbytes = 8;
      }else{
	if (!AMD64_OPSZPREF(ins)){
	  AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint32*)codep;
	  AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
	}else{
	  AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint16*)codep;
	  AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
	}
      }
    break;
    case amd64_bm_z:
      if (!AMD64_OPSZPREF(ins)){
	AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint32*)codep;
	AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
      }else{
	AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint16*)codep;
	AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
      }
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return readbytes;
}

t_uint32 Amd64OpDisJ(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 readbytes = 0;
  /* immediate operand, not sign extended */
  AMD64_OP_TYPE(op) = amd64_optype_imm;
  
  switch (bm)
  {
    case amd64_bm_b:
      AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int8*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 1;
      break;
    case amd64_bm_w:
      AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int16*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
      break;
    case amd64_bm_d:
      AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int32*)codep;
      AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
      break;
    case amd64_bm_v:
      if(rexw(rex)){
	AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int64*) codep;
	AMD64_OP_IMMEDSIZE(op) = readbytes = 8;
      }else{
	if (!AMD64_OPSZPREF(ins)){
	  AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int32*)codep;
	  AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
	}else{
	  AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int16*)codep;
	  AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
	}
      }
      break;
    case amd64_bm_z:
      if (!AMD64_OPSZPREF(ins)){
	AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int32*)codep;
	AMD64_OP_IMMEDSIZE(op) = readbytes = 4;
      }else{
	AMD64_OP_IMMEDIATE(op) = (t_int64) *(t_int16*)codep;
	AMD64_OP_IMMEDSIZE(op) = readbytes = 2;
      }
    break;
    default:
    FATAL(("unknown bytemode"));
  }
  return readbytes;
}

t_uint32 Amd64OpDisM(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* this is in fact the same as the E_ cases, with the exception that the mod
   * bits should never be 3, so the operand is always a memory operand.
   * the only real difference is in the bytemodes that come with the
   * instruction */
  t_uint32 retval = Amd64OpDisE(ins, codep, modrm, sib, op, amd64_bm_d,rex);

  /* adjust the memopsize, as this is the only part of the operand depending
   * on the byte mode */
  switch (bm)
  {
    case 0:
      /* this happens with the LEA instruction */
      AMD64_OP_MEMOPSIZE(op) = 0;
      break;
    case amd64_bm_b:
      AMD64_OP_MEMOPSIZE(op) = 1;
      break;
    case amd64_bm_a:
      if (AMD64_OPSZPREF(ins))
	AMD64_OP_MEMOPSIZE(op) = 4;
      else
	AMD64_OP_MEMOPSIZE(op) = 8;
      break;
    case amd64_bm_p:
      if (AMD64_OPSZPREF(ins))
	AMD64_OP_MEMOPSIZE(op) = 4;
      else
	AMD64_OP_MEMOPSIZE(op) = 6;
      break;
    case amd64_bm_s:
      AMD64_OP_MEMOPSIZE(op) = 6;
      break;

    /* bytemodes for fpu memory operations */
    case amd64_bm_sr:
      AMD64_OP_MEMOPSIZE(op) = 4;
      break;
    case amd64_bm_dr:
      AMD64_OP_MEMOPSIZE(op) = 8;
      break;
    case amd64_bm_er:
    case amd64_bm_bcd:
      AMD64_OP_MEMOPSIZE(op) = 10;
      break;
    case amd64_bm_w:
      AMD64_OP_MEMOPSIZE(op) = 2;
      break;
    case amd64_bm_d:
      AMD64_OP_MEMOPSIZE(op) = 4;
      break;
    case amd64_bm_q:
      AMD64_OP_MEMOPSIZE(op) = 8;
      break;
    case amd64_bm_dq:
      AMD64_OP_MEMOPSIZE(op) = 16;
      break;
    case amd64_bm_2byte:
      AMD64_OP_MEMOPSIZE(op) = 2;
      break;
    case amd64_bm_28byte:
      AMD64_OP_MEMOPSIZE(op) = 28;
      break;
    case amd64_bm_108byte:
      AMD64_OP_MEMOPSIZE(op) = 108;
      break;
    case amd64_bm_512byte:
      AMD64_OP_MEMOPSIZE(op) = 512;
      break;

    default:
      FATAL(("unknown bytemode"));
  }
  return retval;
}

t_uint32 Amd64OpDisO(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 readbytes = 0;

  /* memory operand, but no modrm byte: just offset specified immediately */
  AMD64_OP_TYPE(op) = amd64_optype_mem;
  AMD64_OP_BASE(op) = AMD64_REG_NONE;
  AMD64_OP_INDEX(op) = AMD64_REG_NONE;

  if (AMD64_ADSZPREF(ins))
  {
    /* 16-bit offset */
    AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint16 *)codep;
    AMD64_OP_IMMEDSIZE(op) = 2;
    readbytes = 2;
  }
  else
  {
    /* 32-bit offset */
    AMD64_OP_IMMEDIATE(op) = *(t_uint64 *)codep;
    AMD64_OP_IMMEDSIZE(op) = 4;
    readbytes = 4;
  }

  switch (bm){
    case amd64_bm_b:
      AMD64_OP_MEMOPSIZE(op) = 1;
      break;
    case amd64_bm_z:
      if(AMD64_OPSZPREF(ins)){
        AMD64_OP_MEMOPSIZE(op) = 2;
      }else{
        AMD64_OP_MEMOPSIZE(op) = 4;
      }
      break;
    case amd64_bm_v:
      if(rexw(rex)){
	AMD64_OP_MEMOPSIZE(op) = 8;
      }else{
        if(AMD64_OPSZPREF(ins)){
          AMD64_OP_MEMOPSIZE(op) = 2;
        }else{
          AMD64_OP_MEMOPSIZE(op) = 4;
        }
      }
      break;
    default:
      FATAL(("unknown bytemode"));
  }
  return readbytes;
}

t_uint32 Amd64OpDisO8(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
    t_uint32 readbytes = 0;

    /* memory operand, but no modrm byte: just offset specified immediately */
    AMD64_OP_TYPE(op) = amd64_optype_mem;
    AMD64_OP_BASE(op) = AMD64_REG_NONE;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;
    
    if (AMD64_ADSZPREF(ins))
    {
      /* 16-bit offset */
      AMD64_OP_IMMEDIATE(op) = (t_uint64) *(t_uint16 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 2;
      readbytes = 2;
    }
    else
    {
      /* 32-bit offset */
      AMD64_OP_IMMEDIATE(op) = *(t_uint64 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 8;
      readbytes = 8;
    }
    
    switch (bm){
      case amd64_bm_b:
	AMD64_OP_MEMOPSIZE(op) = 1;
	break;
      case amd64_bm_z:
	if(AMD64_OPSZPREF(ins)){
	  AMD64_OP_MEMOPSIZE(op) = 2;
	}else{
	  AMD64_OP_MEMOPSIZE(op) = 4;
	}
	break;
      case amd64_bm_v:
	if(rexw(rex)){
	  AMD64_OP_MEMOPSIZE(op) = 8;
	}else{
	  if(AMD64_OPSZPREF(ins)){
	    AMD64_OP_MEMOPSIZE(op) = 2;
	  }else{
	    AMD64_OP_MEMOPSIZE(op) = 4;
	  }
	}
	break;
      default:
	FATAL(("unknown bytemode"));
    }
    return readbytes;
}

t_uint32 Amd64OpDisR(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* this is equal to the E_ case, but the mod field may only be 3, so the
   * operand will always be a register */
  return Amd64OpDisE(ins,codep,modrm,sib,op,bm,rex);
}

t_uint32 Amd64OpDisS(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* reg field of the modrm byte selects a segment register */
  t_uint32 reg = (modrm >> 3) & 7;

  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_BASE(op) = regs_seg[reg];
  AMD64_OP_REGMODE(op) = amd64_regmode_lo16;

  return 0;
}

t_uint32 Amd64OpDisX(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* memory addressed by DS:SI */
  AMD64_OP_TYPE(op) = amd64_optype_mem;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  AMD64_OP_BASE(op) = AMD64_REG_RSI;
  AMD64_OP_INDEX(op) = AMD64_REG_NONE;
  AMD64_OP_IMMEDIATE(op) = 0;
  switch (bm){
    case amd64_bm_b:
      AMD64_OP_MEMOPSIZE(op) = 1;
      break;
    case amd64_bm_z:
      if(AMD64_OPSZPREF(ins)){
        AMD64_OP_MEMOPSIZE(op) = 2;
      }else{
        AMD64_OP_MEMOPSIZE(op) = 4;
      }
    break;
    case amd64_bm_v:
      if(rexw(rex)){
        AMD64_OP_MEMOPSIZE(op) = 8;
      }else{
        if(AMD64_OPSZPREF(ins)){
          AMD64_OP_MEMOPSIZE(op) = 2;
        }else{
          AMD64_OP_MEMOPSIZE(op) = 4;
        }
      }
    break;
    default:
      printf("%s",bmodes[bm]);
      FATAL(("unknown bytemode"));
  }
    return 0;
}

t_uint32 Amd64OpDisY(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* memory addressed by ES:DI */
  AMD64_OP_TYPE(op) = amd64_optype_mem;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  AMD64_OP_BASE(op) = AMD64_REG_RDI;
  AMD64_OP_INDEX(op) = AMD64_REG_NONE;
  AMD64_OP_IMMEDIATE(op) = 0;
  switch (bm){
    case amd64_bm_b:
      AMD64_OP_MEMOPSIZE(op) = 1;
      break;
    case amd64_bm_z:
      if(AMD64_OPSZPREF(ins)){
        AMD64_OP_MEMOPSIZE(op) = 2;
      }else{
        AMD64_OP_MEMOPSIZE(op) = 4;
      }
    break;
    case amd64_bm_v:
      if(rexw(rex)){
        AMD64_OP_MEMOPSIZE(op) = 8;
      }else{
        if(AMD64_OPSZPREF(ins)){
          AMD64_OP_MEMOPSIZE(op) = 2;
        }else{
          AMD64_OP_MEMOPSIZE(op) = 4;
        }
      }
    break;
    default:
      printf("%s",bmodes[bm]);
      FATAL(("unknown bytemode"));
  }
  return 0;
}

t_uint32 Amd64OpDisV(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  /* this is equal to the E_ case, but the mod field may only be 3, so the
   * operand will always be a register */
  t_uint32 reg = (modrm >> 3) & 7;
  if(rexr(rex))
    reg+=8;
  AMD64_OP_TYPE(op) = amd64_optype_reg;
  AMD64_OP_BASE(op) = AMD64_REG_XMM0 + reg;
  AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  return 0;
}

t_uint32 Amd64OpDisW(t_amd64_ins * ins, t_uint8 * codep, t_uint32 modrm, t_uint32 sib, t_amd64_operand * op, t_amd64_bytemode bm, char rex)
{
  t_uint32 mod, rm, reg;
  t_uint32 scale = 0, index = 0, base = 0;
  t_uint32 readbytes=0;
  t_bool has_sib;
   ASSERT(modrm != -1, ("need modrm byte, got none"));
  mod = (modrm >> 6) & 3;
  reg = (modrm >> 3) & 7;
  rm = modrm & 7;
  if(rexr(rex))
    reg+=8;
  if (mod == 3){
    //XMM register
    if(rexb(rex))
      rm+=8;
    AMD64_OP_TYPE(op) = amd64_optype_reg;
    AMD64_OP_BASE(op) = AMD64_REG_XMM0 + rm;
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
  }else{
    //memory operand
    readbytes = 0;
    has_sib = FALSE;

    /* memory operand */
    AMD64_OP_TYPE(op) = amd64_optype_mem;

    /* we don't support 16-bit addressing */
    ASSERT(!AMD64_ADSZPREF(ins), ("16 bit addressing not supported"));

    if (rm == 4){
      has_sib = TRUE;
      ASSERT(sib != -1, ("need SIB byte but none given"));
      scale = (sib >> 6) & 3;
      index = (sib >> 3) & 7;
      base = sib & 7;
      if(rexx(rex))
        index+=8;
      if(rexb(rex))
        base+=8;
    }else{
      if(rexb(rex))
        rm+=8;
    }
    
    /* base and index regs */
    AMD64_OP_BASE(op) = regs32[rm];
    AMD64_OP_REGMODE(op) = amd64_regmode_full64;
    AMD64_OP_INDEX(op) = AMD64_REG_NONE;

    /* displacement */
    if (mod == 0)
    {
      if (rm == 5)
      {
	/* special case: only disp32, no base register */
	AMD64_OP_BASE(op) = AMD64_REG_NONE;
	AMD64_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
	AMD64_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
      else
      {
	AMD64_OP_IMMEDIATE(op) = 0;
	AMD64_OP_IMMEDSIZE(op) = 0;
      }
    }
    if (mod == 1)
    {
      AMD64_OP_IMMEDIATE(op) = (t_int32) *(t_int8 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 1;
      codep += 1;
      readbytes = 1;
    }
    else if (mod == 2)
    {
      AMD64_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
      AMD64_OP_IMMEDSIZE(op) = 4;
      codep += 4;
      readbytes = 4;
    }

    /* handle sib byte if present */
    if (has_sib)
    {
      AMD64_OP_SCALE(op) = scale;
      AMD64_OP_BASE(op) = regs32[base];
      if (index != 4)  /* can't use %esp as an index register */
	AMD64_OP_INDEX(op) = regs32[index];
      else
	AMD64_OP_INDEX(op) = AMD64_REG_NONE;

      /* special case: base == 5, mod == 0 */

      if (base == 5 && mod == 0)
      {
	/* just scaled index + disp32, no base */
	AMD64_OP_BASE(op) = AMD64_REG_NONE;
	AMD64_OP_IMMEDIATE(op) = *(t_uint32 *)codep;
	AMD64_OP_IMMEDSIZE(op) = 4;
	codep += 4;
	readbytes = 4;
      }
    }
    switch (bm)
      {
	case amd64_bm_sd:
  	    AMD64_OP_MEMOPSIZE(op) = 8;
	  break;
	case amd64_bm_ss:
	    AMD64_OP_MEMOPSIZE(op) = 4;
	  break;
 	case amd64_bm_ps:
	    AMD64_OP_MEMOPSIZE(op) = 16;
	  break;
	case amd64_bm_pd:
	    AMD64_OP_MEMOPSIZE(op) = 16;
	  break;
	case amd64_bm_dq:
	    AMD64_OP_MEMOPSIZE(op) = 16;
	  break;
	case amd64_bm_q:
	    AMD64_OP_MEMOPSIZE(op) = 8;
	  break;
 	default:
	  FATAL(("unexpected bytemode"));
      }
  }
  return readbytes;
}

