/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

t_bool is_started = FALSE;
/*!
 * 
 * Dispatches a VFP instruction to the correct assembling subroutine
 *
 * \param i_ins The instruction to assemble 
 *
 * \param data A buffer (must be at least 4 bytes long in which the instruction
 * is assembled 
 *
 * \return void 
*/
/* ArmAssembleVFP {{{ */
static void
ArmAssembleVFP(t_arm_ins * ains, t_uint32 *data)
{
  if (ARM_INS_OPCODE(ains) >= ARM_VFP_DP_FIRST)
  {
    if (ARM_INS_OPCODE(ains) < ARM_VFP_RT_FIRST)
      ArmAssembleVFPDP(ains, data);
    else if (ARM_INS_OPCODE(ains) < ARM_VFP_R2T_FIRST)
      ArmAssembleVFPRT(ains, data);
    else if (ARM_INS_OPCODE(ains) < ARM_VFP_DT_FIRST)
      ArmAssembleVFP2R(ains, data);
    else if (ARM_INS_OPCODE(ains) <= ARM_VFP_LAST)
      ArmAssembleVFPDT(ains, data);
    else
      FATAL(("Not a VFP instruction: @I",ains));
  }
  else
    FATAL(("Not a VFP instruction: @I",ains));
}
/* }}} */

/*!
 * 
 * Dispatches an ARMv6 instruction to the correct assembling subroutine
 *
 * \param i_ins The instruction to assemble 
 *
 * \param data A buffer (must be at least 4 bytes long in which the instruction
 * is assembled 
 *
 * \return void 
*/
/* ArmAssembleV6 {{{ */
static void
ArmAssembleV6(t_arm_ins * ains, t_uint32 *data)
{
  switch (ARM_INS_OPCODE(ains))
  {
    case ARM_MOVW:
    case ARM_MOVT:
      ArmAssembleDataProc(ains, data);
      break;
    case ARM_PKHBT:
    case ARM_PKHTB:
      ArmAssembleV6Pkh(ains, data);
      break;
    case ARM_SXTH:
    case ARM_SXTB16:
    case ARM_SXTB:
    case ARM_UXTH:
    case ARM_UXTB16:
    case ARM_UXTB:
    case ARM_SXTAH:
    case ARM_SXTAB16:
    case ARM_SXTAB:
    case ARM_UXTAH:
    case ARM_UXTAB16:
    case ARM_UXTAB:
      ArmAssembleV6Extract(ains, data);
      break;
    case ARM_SSAT:
    case ARM_SSAT16:
    case ARM_USAT:
    case ARM_USAT16:
      ArmAssembleV6Sat(ains, data);
      break;
    case ARM_SEL:
    default:
      if ((ARM_INS_OPCODE(ains) >= ARM_V6_FIRST) &&
	  (ARM_INS_OPCODE(ains) <= ARM_V6_LAST))
	ArmAssembleV6DataProc(ains, data);
      else
        FATAL(("Not an ARMv6 instruction: @I",ains));
      break;
  }
}
/* }}} */

/*!
 * 
 * Assemble an arm instruction (t_arm_ins to raw arm instruction)
 *
 * \param i_ins The instruction to assemble 
 *
 * \param data A buffer (must be at least 4 bytes long in which the instruction
 * is assembled 
 *
 * \return void 
*/
/* ArmAssembleOne {{{ */
void ArmAssembleOne(t_arm_ins * i_ins, char * data)
{
  t_arm_ins * ains=T_ARM_INS(i_ins);

  if(!is_started)
  {
    is_started = TRUE;
  }

  if (ARM_INS_TYPE(ains) != IT_DATA)
  {
    /* Get the opcode from the opcode table and set it */
    *((t_uint32 *)data) = arm_opcode_table[ARM_INS_OPCODE(ains)].opcode;

    /* Set the other bits (registers, immediates,...) */
    switch (ARM_INS_TYPE(ains))
    {
      case IT_BRANCH   : 
	      ArmAssembleBranch(ains, (t_uint32*)data); 
	      break;

      case IT_DATAPROC :
      	if ((ARM_INS_OPCODE(ains) >= ARM_V6_FIRST) &&
      	    (ARM_INS_OPCODE(ains) <= ARM_V6_LAST) &&
            (ARM_INS_OPCODE(ains)!=ARM_MOVW) && (ARM_INS_OPCODE(ains)!=ARM_MOVT))
      	  ArmAssembleV6(ains, (t_uint32*)data);
        else if ((ARM_BITFIELD_FIRST <= ARM_INS_OPCODE(ains)) && (ARM_INS_OPCODE(ains) <= ARM_BITFIELD_LAST))
          ArmAssembleBitfield(ains, (t_uint32 *) data);
        else if ((ARM_INS_OPCODE(ains)==ARM_SDIV) || (ARM_INS_OPCODE(ains) == ARM_UDIV))
          ArmAssembleDivision(ains, (t_uint32 *)data);
        else
	        ArmAssembleDataProc(ains, (t_uint32*)data);
	      break;
      
      case IT_LOAD     :
        if ((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(ains)) && (ARM_INS_OPCODE(ains) <= ARM_LDRSTREX_LAST))
          ArmAssembleLoadStoreExclusive(ains, (t_uint32 *)data);
        else
      	  ArmAssembleLoad(ains, (t_uint32*)data); 
      	break;

      case IT_STORE    :
        if ((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(ains)) && (ARM_INS_OPCODE(ains) <= ARM_LDRSTREX_LAST))
          ArmAssembleLoadStoreExclusive(ains, (t_uint32 *)data);
        else
      	  ArmAssembleStore(ains, (t_uint32*)data); 
      	break;

      case IT_LOAD_MULTIPLE: 
      case IT_STORE_MULTIPLE:
        if( ((ARM_INS_OPCODE(ains) >= ARM_SIMD_FIRSTSTORE) && (ARM_INS_OPCODE(ains) <= ARM_SIMD_LASTSTORE)) ||
            ((ARM_INS_OPCODE(ains) >= ARM_SIMD_FIRSTLOAD) && (ARM_INS_OPCODE(ains) <= ARM_SIMD_LASTLOAD)) )
        {
          ArmAssembleSIMDLoadStore(ains, (t_uint32 *) data);
        }
        else if( (ARM_SIMD_LOADSTORE_FIRST <= ARM_INS_OPCODE(ains)) && (ARM_INS_OPCODE(ains) <= ARM_SIMD_LOADSTORE_LAST))
        {
          ArmAssembleFLTMEM(ains, (t_uint32 *) data);
        }
        else
        {
	       ArmAssembleMultipleTransfer(ains, (t_uint32 *) data); 
        }
	      break;

      case IT_SWI:
      	switch (ARM_INS_OPCODE(i_ins))
      	{
      	  case ARM_SWI:
      	    ArmAssembleSWI(ains, (t_uint32 *) data);
      	    break;
      	  case ARM_BKPT:
      	    ArmAssembleBKPT(ains, (t_uint32 *) data);
      	    break;
      	  default:
      	    FATAL(("Unknown IT_SWI instruction @I",i_ins));
      	    break;
      	}
      	break;

      case IT_MUL:
      case IT_DIV:
      	ArmAssembleMUL(ains, (t_uint32 *) data); 
      	break;

      case IT_STATUS:
        if ((ARM_INS_OPCODE(ains) < ARM_VFP_DP_FIRST) ||
            (ARM_INS_OPCODE(ains) > ARM_VFP_LAST))
	        ArmAssembleStatus(ains, (t_uint32 *) data);
        else
          ArmAssembleVFP(ains, (t_uint32 *) data);
	      break;

      case IT_SWAP:
      	ArmAssembleSwap(ains, (t_uint32 *) data);
      	break;

      /* Floating point instructions (as used on the XScale) */
      case IT_FLT_STORE:
      case IT_FLT_LOAD:
        if (ARM_INS_OPCODE(ains)==ARM_FSTMX || ARM_INS_OPCODE(ains)==ARM_FLDMX)
          ArmAssembleVFPDT(ains, (t_uint32 *) data);
        else if ((ARM_INS_OPCODE(ains) < ARM_VFP_FIRST) || (ARM_INS_OPCODE(ains) > ARM_VFP_LAST))
          ArmAssembleFLTMEM(ains, (t_uint32 *) data);
        else
          ArmAssembleVFP(ains, (t_uint32 *) data);
	      break;

      case IT_FLT_STATUS:
        if ((ARM_INS_OPCODE(ains) < ARM_VFP_FIRST) ||
            (ARM_INS_OPCODE(ains) > ARM_VFP_LAST))
  	      ArmAssembleFLTStatus(ains, (t_uint32 *) data);
        else
          ArmAssembleVFP(ains, (t_uint32 *) data);
	      break;

      case IT_FLT_ALU:
      case IT_FLT_INT:
      case IT_INT_FLT:
        if ((ARM_FP_FIRST <= ARM_INS_OPCODE(ains)) && (ARM_INS_OPCODE(ains) <= ARM_FP_LAST))
          ArmAssembleVFPDP(ains, (t_uint32 *) data);
        else if ((ARM_INS_OPCODE(ains) < ARM_VFP_FIRST) || (ARM_INS_OPCODE(ains) > ARM_VFP_LAST))
          ArmAssembleFLTCPDO(ains, (t_uint32 *) data);
        else
          ArmAssembleVFP(ains, (t_uint32 *) data);
	      break;

      /* Constant-/Addressproducers */
      case IT_CONSTS:
      	/* Constant-/Addressproducers should already be transformed to normal
      	 * Arm instructions */
      	FATAL(("Attempt to assemble an address producer or a constant producer assembled: @I. This is impossible: these instructions should be converted to normal arm instructions by the layout code.",ains));
      	break;

      case IT_SIMD:
        if((ARM_INS_OPCODE(ains) >= ARM_SIMD_FIRST3SAME) && (ARM_INS_OPCODE(ains) <= ARM_SIMD_LAST3SAME))
        {
          ArmAssembleSIMD3RegsSameLength(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMDIMM_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMDIMM_LAST))
        {
          ArmAssembleSIMDImm(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMDVAR_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMDVAR_LAST))
        {
          ArmAssembleSIMD(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMD2MISC_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMD2MISC_LAST))
        {
          ArmAssembleSIMD2RegsMisc(ains, (t_uint32 *) data);
        }
        else if(ARM_INS_OPCODE(ains) == ARM_VDUP)
        {
          ArmAssembleSIMD(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMDRT_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMDRT_LAST))
        {
          ArmAssembleSIMDTransfer(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMD2REGSSHIFT_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMD2REGSSHIFT_LAST))
        {
          ArmAssembleSIMD2RegsShift(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMD2REGSSCALAR_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMD2REGSSCALAR_LAST))
        {
          ArmAssembleSIMD2RegsScalar(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) >= ARM_SIMD3REGSDIFFLENGTH_FIRST) && (ARM_INS_OPCODE(ains) <= ARM_SIMD3REGSDIFFLENGTH_LAST))
        {
          ArmAssembleSIMD3RegsDifferentLength(ains, (t_uint32 *) data);
        }
        else if((ARM_INS_OPCODE(ains) == ARM_VMOV64_C2S) || (ARM_INS_OPCODE(ains) == ARM_VMOV64_C2D))
        {
          ArmAssembleFP2R(ains, (t_uint32 *) data);
        }
        else
        {
          FATAL(("Attempt to assemble SIMD instruction: @I", ains));
        }
        break;

      case IT_UNKNOWN:
        ArmAssembleMisc(ains, (t_uint32 *) data);
        break;

      case IT_SYNC:
        ArmAssembleHint(ains, (t_uint32 *) data);
        break;

      case IT_NOP:
        ArmAssembleNOP(ains, (t_uint32 *) data);
        break;

      default:
	/* Print the instruction we cannot assemble. Will probably fail,
	 * because it is likely that the print instruction is also not
	 * implemented. */
	FATAL(("Instruction type not implemented! Implement @I\n", ains));
    }
  } 
  else 
  {
    /* The value of a data pseudoinstruction is stored in the immediate field
     * of t_arm_ins */
    *((t_uint32 *) data)=ARM_INS_IMMEDIATE(ains);
  }
}
/* }}} */
/*!
 *  
 * Assemble the entire section. The assembled instructions are written to the
 * temporary buffer of the section (allocated by DiabloAction).
 *
 * \todo Should only have on parameter (section) and the relocation code at the
 * end should probably move to DiabloAction
 *
 * \param obj The object in which the section resides (remove me)
 *
 * \param sec The section to assemble
 *
 * \return void 
*/
/* ArmAssembleSection {{{ */
#define ISARM 0
#define ISTHUMB 1
#define ISDATA 2
void ArmAssembleSection(t_section * sec)
{
  t_uint32 prev = -1;
  t_uint32 address=0;
  int nins = 0;
  t_arm_ins * i_ins;
  char * data = SECTION_TMP_BUF(sec);
  t_cfg * cfg;

  i_ins = SECTION_DATA(sec); 
  cfg=ARM_INS_CFG(i_ins);
  address=G_T_UINT32(SECTION_CADDRESS(sec));

  if (!OBJECT_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec))) 
    OBJECT_SET_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec), SymbolTableNew(SECTION_OBJECT(sec)));

  while(i_ins != NULL)
  {
    /*VERBOSE(0, ("assemble @I", i_ins));*/

    /* add padding if the address where the data is written in the section
       is not the same as the address computed during deflowgraphing */
    while (address!=ARM_INS_CADDRESS(i_ins))
      {
        nins++;
        data += 2; /* per two for Thumb */
        address +=2; 
      } 

    if (G_T_UINT32(AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)))!=(data - ((char *) SECTION_TMP_BUF(sec))))
    {
      VERBOSE(0,("ca @G - @G, data %p - %p",
	    ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec),
	    data, SECTION_TMP_BUF(sec)));
      FATAL(("Alignment error"));
    }
    /* we have to reassemble it */
    /* Use thumb assemble one to assemble thumb instruction */
    if (ARM_INS_FLAGS(i_ins) & FL_THUMB) {
      ThumbAssembleOne(i_ins, data);

      if (ARM_INS_CSIZE(i_ins) == 4)
      {
        /* 32-bit Thumb instructions should have their least significant 16-bit word FIRST */
        t_uint32 *dataptr = (t_uint32 *)data;
        t_uint32 d = *dataptr;
        d = ((d & 0xffff) << 16) | ((d & 0xffff0000) >> 16);
        *dataptr = d;
      }

      if (ARM_INS_TYPE(i_ins) == IT_DATA)
      {
	if (prev != ISDATA)
	{
	  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec)), "$d", "R00$", -1, T_TRISTATE(TRUE), T_TRISTATE(FALSE), T_RELOCATABLE(sec), AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)), AddressNullForObject(SECTION_OBJECT(sec)), NULL, AddressNullForObject(SECTION_OBJECT(sec)), 0);
	  prev = ISDATA;
	}
      }
      else
      {
	if (prev != ISTHUMB)
	{
	  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec)), "$t", "R00$", -1, T_TRISTATE(TRUE), T_TRISTATE(FALSE), T_RELOCATABLE(sec), AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)), AddressNullForObject(SECTION_OBJECT(sec)), NULL, AddressNullForObject(SECTION_OBJECT(sec)), 0);
	  prev = ISTHUMB;
	}
      }
    }
    else
    {
    /* Use arm assemble one to assemble the instruction */
      if ((G_T_UINT32(AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)))& 3) &&
          (ARM_INS_TYPE(i_ins) != IT_DATA))
	  FATAL(("Alignment error: ARM INSTRUCTION: @I", i_ins));
      ArmAssembleOne(i_ins, data);

      if (OBJECT_SWITCHED_ENDIAN(SECTION_OBJECT(sec)))
        (*((t_uint32 *) data)) = Uint32SwapEndian(*((t_uint32 *) data));

      if (ARM_INS_TYPE(i_ins) == IT_DATA)
      {
	if (prev != ISDATA)
	{
	  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec)), "$d", "R00$", -1, T_TRISTATE(TRUE), T_TRISTATE(FALSE), T_RELOCATABLE(sec), AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)), AddressNullForObject(SECTION_OBJECT(sec)), NULL, AddressNullForObject(SECTION_OBJECT(sec)), 0);
	  prev = ISDATA;
	}
      }
      else
      {
	if (prev != ISARM)
	{
	  SymbolTableAddSymbol(OBJECT_SUB_SYMBOL_TABLE(SECTION_OBJECT(sec)), "$a", "R00$", -1, T_TRISTATE(TRUE), T_TRISTATE(FALSE), T_RELOCATABLE(sec), AddressSub(ARM_INS_CADDRESS(i_ins), SECTION_CADDRESS(sec)), AddressNullForObject(SECTION_OBJECT(sec)), NULL, AddressNullForObject(SECTION_OBJECT(sec)), 0);
	  prev = ISARM;
	}
      }
    }

    nins++;
    data += G_T_UINT32(ARM_INS_CSIZE(i_ins));
    address += G_T_UINT32(ARM_INS_CSIZE(i_ins));

    i_ins = ARM_INS_INEXT(i_ins);
  }
}
/* }}} */

t_bool ArmParseFromStringAndInsertAt(t_string ins_text, t_bbl * bbl, t_ins * at_ins, t_bool before)
{
  t_bool asm_succes;
  t_arm_ins * new = ArmInsNewForBbl(bbl);
  if(at_ins)
  {
    if(before == TRUE)
      ArmInsInsertBefore(new, T_ARM_INS(at_ins));
    else
      ArmInsInsertAfter(new, T_ARM_INS(at_ins));
  }
  else ArmInsPrependToBbl(new,bbl);
  
  asm_succes = ArmInsAssembleFromString(new, ins_text);
  if(asm_succes == TRUE)
  {
    ARM_INS_SET_REGS_DEF(new,ArmDefinedRegisters(new));
    ARM_INS_SET_REGS_USE(new,ArmUsedRegisters(new));
    return TRUE;
  }
  else
  {
    ArmInsKill(new);
  }
  return FALSE;
}

int insregToQDregnum(t_reg reg)
{
  int num = (reg - ARM_REG_S0) / 2;

  if(reg >= ARM_REG_D16)
  {
    num = (reg - ARM_REG_D16) + 16;
  }

  return num;
}

t_uint32 ASM_NEON_VD_S(t_uint32 reg)
{
  reg -= ARM_REG_S0;
  return ((reg & 0x01) << 22) | ((reg & 0x1e) << 11);
}
t_uint32 ASM_NEON_VD_QD(t_uint32 reg)
{
  int num = insregToQDregnum(reg);
  return ((num & 0x10) << 18) | ((num & 0x0f) << 12);
}

t_uint32 ASM_NEON_VN_S(t_uint32 reg)
{
  reg -= ARM_REG_S0;
  return ((reg & 0x01) << 7)  | ((reg & 0x1e) << 15);
}
t_uint32 ASM_NEON_VN_QD(t_uint32 reg)
{
  int num = insregToQDregnum(reg);
  return ((num & 0x10) << 3)  | ((num & 0x0f) << 16);
}

t_uint32 ASM_NEON_VM_S(t_uint32 reg)
{
  reg -= ARM_REG_S0;
  return ((reg & 0x01) << 5) | ((reg & 0x1e) >> 1);
}
t_uint32 ASM_NEON_VM_QD(t_uint32 reg)
{
  int num = insregToQDregnum(reg);
  return ((num & 0x10) << 1)  | (num & 0x0f);
}

t_uint32 ASM_NEON_VM_SCALAR(t_uint32 reg, t_uint32 index, t_arm_ins_dt size)
{
  t_uint32 num = (t_uint32)insregToQDregnum(reg);
  if(size == DT_16)
  {
    ASSERT(num <= 7, ("Illegal scalar register"));
    return (num | ((index & 2) << 4) | ((index & 1) << 3));
  }
  else if(size == DT_32)
  {
    ASSERT(num <= 15, ("Illegal scalar register"));
    return (num | (index << 5));
  }
  else
  {
    FATAL(("Illegal size for scalar register: %u.", size));
    return 0;
  }
}

t_bool ASM_DATATYPE_IS_SIGNED(t_arm_ins * ins)
{
  return ((DT_S_START <= ARM_INS_DATATYPE(ins)) && (ARM_INS_DATATYPE(ins) <= DT_S_END));
}

t_arm_ins_dt ASM_DATATYPE_NORMALIZE(t_arm_ins * ins, t_arm_ins_dt * base_type)
{
  t_arm_ins_dt dt = ARM_INS_DATATYPE(ins);

  if((DT_START <= dt) && (dt <= DT_END))
  {
    *base_type = DT_START;
    return dt-DT_START;
  }
  else if((DT_I_START <= dt) && (dt <= DT_I_END))
  {
    *base_type = DT_I_START;
    return dt-DT_I_START;
  }
  else if((DT_S_START <= dt) && (dt <= DT_S_END))
  {
    *base_type = DT_S_START;
    return dt-DT_S_START;
  }
  else if((DT_U_START <= dt) && (dt <= DT_U_END))
  {
    *base_type = DT_U_START;
    return dt-DT_U_START;
  }
  else if((DT_F_START <= dt) && (dt <= DT_F_END))
  {
    *base_type = DT_F_START;
    return dt-DT_F_START;
  }
  else if((DT_P_START <= dt) && (dt <= DT_P_END))
  {
    *base_type = DT_P_START;
    return dt-DT_P_START;
  }
  else
  {
    //FATAL(("Unable to normalize an illegal datatype: %u (instruction: @I)", dt, ins));
    *base_type = DT_NONE;
    return dt;
  }
}

t_reg ASM_MULTIPLE_FIND_DREG_BLOB(t_regset r, t_uint32 * length)
{
  t_reg dreg = 0;
  int i;
  t_bool is_location = FALSE;
  t_uint32 count = 0;
  t_reg location = ARM_REG_NONE;

  for(i = 0; i < 32; i++)
  {
    t_reg diablo_regd_ir = (i < 16) ? (ARM_REG_S0 + (2*i)) : (ARM_REG_D16 + (i-16));

    if(RegsetIn(r, diablo_regd_ir))
    {
      location = (is_location) ? location : diablo_regd_ir;
      is_location = TRUE;
      count++;
    }
    else if(is_location)
    {
      //DEBUG(("Gap after %u reg(s)", count));
      break;
    }
  }

  *length = count;
  return location;
}
/* vim: set shiftwidth=2 foldmethod=marker : */
