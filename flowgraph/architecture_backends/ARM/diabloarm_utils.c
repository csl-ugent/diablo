/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>

t_regset ArmRegsetAddDoubleReg(t_regset mask, t_reg first_register)
{
  RegsetSetAddReg(mask, first_register);

  /* add an extra register if the D-registers are mapped */
  if(first_register <= ARM_REG_S31)
  {
    ASSERT((first_register - ARM_REG_S0) % 2 == 0, ("Lower D-register should be mapped to an even S-register"));

    /* D-registers 0 to 15 are mapped to S-registers 0-31 */
    RegsetSetAddReg(mask, first_register+1);
  }

  return mask;
}

t_regset ArmRegsetAddQuadReg(t_regset mask, t_reg first_register)
{
  /* lower 8 Q-registers are mapped to D-registers 0 to 15,
   * which in turn are mapped to S-registers 0-31
   */
  /* Also assume that the Q-registers are saved as a D-register number */
  RegsetSetAddReg(mask, first_register);
  RegsetSetAddReg(mask, first_register+1);

  if(first_register <= ARM_REG_S31)
  {
    RegsetSetAddReg(mask, first_register+2);
    RegsetSetAddReg(mask, first_register+3);
  }

  return mask;
}

/*!
 * checks whether writeback occurs with this instruction (this can only happen
 * with LDR,STR,LDM,STM and derivatives)
 *
 * \param ins
 *
 * \return int
*/
/* ArmInsWriteBackHappens {{{ */
int ArmInsWriteBackHappens(t_arm_ins * ins) {

  if ( !( (ARM_INS_TYPE(ins) == IT_LOAD) || (ARM_INS_TYPE(ins) == IT_STORE) || (ARM_INS_TYPE(ins) == IT_LOAD_MULTIPLE) || (ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE) || (ARM_INS_TYPE(ins) == IT_FLT_LOAD) || (ARM_INS_TYPE(ins) == IT_FLT_STORE)) || (ARM_INS_TYPE(ins) == IT_UNKNOWN) ) {
    return 0;
  }

  /* LDM and STM treat the P and W flags differently compared to LDR and STR */
  if ((ARM_INS_TYPE(ins) == IT_LOAD_MULTIPLE) || (ARM_INS_TYPE(ins) == IT_STORE_MULTIPLE) )
  {
    if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
      return 1;
    else
      return 0;
  }

  /* LDC and STC treats the W flag the same for pre- and postindexing */
  if (ARM_INS_OPCODE(ins) == ARM_MCR || ARM_INS_OPCODE(ins) == ARM_MRC ||
      ARM_INS_OPCODE(ins) == ARM_MCR2 || ARM_INS_OPCODE(ins) == ARM_MRC2)
  {
    if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
      return 1;
    else
      return 0;
  }


  if (!(ARM_INS_FLAGS(ins) & FL_PREINDEX)) return 1;	/* postindex always includes writeback */

  /* if we reach this point, it's definitely preindex */
  if (ARM_INS_FLAGS(ins) & FL_WRITEBACK) return 1;
  else return 0;

  return 0;
}
/* }}}*/
/*!
 * \todo Document
 *
 * \param ins
 *
 * \return int
*/
/* ArmInsHasShiftedFlexible {{{*/
int ArmInsHasShiftedFlexible(t_arm_ins * ins) {
   if (ARM_INS_REGC(ins) == ARM_REG_NONE) return 0;
   if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE) return 0;
   return 1;
}
/* }}} */
/*!
 * Returns a bitmask specifying which registers are use by this instruction the
 * bottommost sixteen bits represent the usual registers r0 - r15 bit 16 is
 * used to represent the CPSR (i.e. conditional instructions use this register)
 * bit 17 is used to represent the SPSR (possibly referenced in the MRS and MSR
 * instructions
 *
 * \param ins
 *
 * \return t_regset
*/
/* ArmUsedRegisters {{{ */
t_regset ArmUsedRegisters(t_arm_ins * ins)
{

  t_regset mask = NullRegs;

  if (ins == NULL) return NullRegs;
  if (ArmInsIsNOOP(ins)) return NullRegs;

  /* conditional instructions use the CPSR */
  if (ArmInsIsConditional(ins))
  {
    switch(ARM_INS_CONDITION(ins))
	  {
	    case ARM_CONDITION_EQ :
      case ARM_CONDITION_NE :
	      RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
	      break;
      case ARM_CONDITION_CS :
      case ARM_CONDITION_CC :
        RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
        break;
      case ARM_CONDITION_MI :
      case ARM_CONDITION_PL :
        RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
        break;
      case ARM_CONDITION_VS :
      case ARM_CONDITION_VC :
        RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
        break;
      case ARM_CONDITION_HI :
      case ARM_CONDITION_LS :
        RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
        break;
      case ARM_CONDITION_GE :
      case ARM_CONDITION_LT :
        RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
        break;
      case ARM_CONDITION_GT :
      case ARM_CONDITION_LE :
        RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
        break;
      case ARM_CONDITION_AL:
      case ARM_CONDITION_NV :
        break; /* nothing ever happens */
    }
      /* Conditional registers can forward the defined registers */
  }

  switch (ARM_INS_TYPE(ins))
  {
    case IT_BRANCH:
      if ((ARM_INS_OPCODE(ins) == ARM_BX) || (ARM_INS_OPCODE(ins) == ARM_BLX) ||
          (ARM_INS_OPCODE(ins) == ARM_T2CBZ) || (ARM_INS_OPCODE(ins) == ARM_T2CBNZ))
      {
	      if (ARM_INS_REGB(ins) != ARM_REG_NONE)
	      {
	        RegsetSetAddReg(mask,ARM_INS_REGB(ins));
	      }
      }
      if (ARM_INS_REGC(ins) != ARM_REG_NONE)
      {
        RegsetSetAddReg(mask, ARM_INS_REGC(ins));
      }
      break;

    case IT_SWI:
      /* SWI's use _all_ registers */
      {
        t_uint32 r;
        for (r=0; r<24; r++)
          if (r != 15) /* exclude the PC, otherwise we run into trouble during leader detection */
            RegsetSetAddReg(mask,r);

        switch (ARM_INS_OPCODE(ins))
        {
          case ARM_SWI:
          {
            switch (ARM_INS_IMMEDIATE(ins))
            {
              case 0x900005:
              case 0x900004:
              case 0x900003:
                /*	      printf("AHA 2 %x %d\n",ARM_INS_IMMEDIATE(BBL_INS_LAST(head)),ARM_INS_IMMEDIATE(BBL_INS_LAST(head)));	      */
                RegsetSetSubReg(mask,ARM_REG_R3);
                break;
              case 0x900001:
                RegsetSetDup(mask,NullRegs);
                RegsetSetAddReg(mask,ARM_REG_R0);
              default:
                break;
                /*	      printf("VALUE 2 %x %d\n",ARM_INS_IMMEDIATE(BBL_INS_LAST(head)),ARM_INS_IMMEDIATE(BBL_INS_LAST(head)));	      */
            }
          }

          default:
            break;
        }
      }
      break;

    case IT_DATAPROC:
      /* all dataprocessing instructions look the same in a certain way: the only registers */
      /* whose contents may be used in the instruction are ARM_REGB, ARM_REGC and ARM_REGS */
      if (ARM_INS_REGB(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      if (ARM_INS_REGC(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGC(ins));
      if (ARM_INS_REGS(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGS(ins));
      if (ARM_INS_OPCODE(ins) == ARM_ADC) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_SBC) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_RSC) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_RRX) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      /* saturated arithmetic instructions _can_ alter the Q flag, but there's no
       * guarantee they actually will. So even though the Q flag is not used during
       * calculations, these instructions still "use" it as they may have to pass
       * the value through */
      if (ARM_INS_OPCODE(ins) == ARM_QADD) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_QDADD) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_QSUB) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_QDSUB) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_SSAT) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_USAT) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_SSAT16) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      if (ARM_INS_OPCODE(ins) == ARM_USAT16) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);

      if (ARM_INS_OPCODE(ins) == ARM_MOVT) RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      if (ARM_INS_OPCODE(ins) == ARM_BFI) RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      if (ARM_INS_OPCODE(ins) == ARM_BFC) RegsetSetAddReg(mask,ARM_INS_REGA(ins));

      if (ARM_INS_OPCODE(ins) == ARM_SEL) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
      break;

    case IT_MUL:
    case IT_DIV:
      /* the values in ARM_REGB and ARM_REGC are certainly read
       * ARM_REGA is only read in UMLAL, SMLA* and SMLS*
       *
       * ARM_REGS is read with MLA and written with all but MUL and MLA
       */
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      RegsetSetAddReg(mask,ARM_INS_REGC(ins));
      if (ARM_INS_OPCODE(ins) == ARM_MLA ||
          ARM_INS_OPCODE(ins) == ARM_MLS ||
          ARM_INS_OPCODE(ins) == ARM_SMLAL ||
          ARM_INS_OPCODE(ins) == ARM_UMLAL ||
          ARM_INS_OPCODE(ins) == ARM_SMLABB ||
          ARM_INS_OPCODE(ins) == ARM_SMLABT ||
          ARM_INS_OPCODE(ins) == ARM_SMLATB ||
          ARM_INS_OPCODE(ins) == ARM_SMLATT ||
          ARM_INS_OPCODE(ins) == ARM_SMLAWB ||
          ARM_INS_OPCODE(ins) == ARM_SMLAWT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTT ||
          ARM_INS_OPCODE(ins) == ARM_SMLAD ||
          ARM_INS_OPCODE(ins) == ARM_SMLADX ||
          ARM_INS_OPCODE(ins) == ARM_SMLALD ||
          ARM_INS_OPCODE(ins) == ARM_SMLALDX ||
          ARM_INS_OPCODE(ins) == ARM_SMLSD ||
          ARM_INS_OPCODE(ins) == ARM_SMLSDX ||
          ARM_INS_OPCODE(ins) == ARM_SMLSLD ||
          ARM_INS_OPCODE(ins) == ARM_SMLSLDX ||
          ARM_INS_OPCODE(ins) == ARM_SMMLA ||
          ARM_INS_OPCODE(ins) == ARM_SMMLAR ||
          ARM_INS_OPCODE(ins) == ARM_SMMLS ||
          ARM_INS_OPCODE(ins) == ARM_SMMLSR ||
          ARM_INS_OPCODE(ins) == ARM_UMAAL)
            RegsetSetAddReg(mask,ARM_INS_REGS(ins));

      if (ARM_INS_OPCODE(ins) == ARM_SMLAL ||
          ARM_INS_OPCODE(ins) == ARM_UMLAL ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALD ||
          ARM_INS_OPCODE(ins) == ARM_SMLALDX ||
          ARM_INS_OPCODE(ins) == ARM_SMLSLD ||
          ARM_INS_OPCODE(ins) == ARM_SMLSLDX ||
          ARM_INS_OPCODE(ins) == ARM_UMAAL)
            RegsetSetAddReg(mask,ARM_INS_REGA(ins));

      /* saturated arithmetic instructions _can_ alter the Q flag, but there's no
       * guarantee they actually will. So even though the Q flag is not used during
       * calculations, these instructions still "use" it as they may have to pass
       * the value through */
      if (ARM_INS_OPCODE(ins) == ARM_SMLABB ||
          ARM_INS_OPCODE(ins) == ARM_SMLABT ||
          ARM_INS_OPCODE(ins) == ARM_SMLATB ||
          ARM_INS_OPCODE(ins) == ARM_SMLATT ||
          ARM_INS_OPCODE(ins) == ARM_SMLAWB ||
          ARM_INS_OPCODE(ins) == ARM_SMLAWT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
          ARM_INS_OPCODE(ins) == ARM_SMLALTT ||
          ARM_INS_OPCODE(ins) == ARM_SMULBB ||
          ARM_INS_OPCODE(ins) == ARM_SMULBT ||
          ARM_INS_OPCODE(ins) == ARM_SMULTB ||
          ARM_INS_OPCODE(ins) == ARM_SMULTT ||
          ARM_INS_OPCODE(ins) == ARM_SMULWB ||
          ARM_INS_OPCODE(ins) == ARM_SMULWT ||
          ARM_INS_OPCODE(ins) == ARM_SMUAD ||
          ARM_INS_OPCODE(ins) == ARM_SMUADX ||
          ARM_INS_OPCODE(ins) == ARM_SMLAD ||
          ARM_INS_OPCODE(ins) == ARM_SMLADX ||
          ARM_INS_OPCODE(ins) == ARM_SMLSD ||
          ARM_INS_OPCODE(ins) == ARM_SMLSDX )
            RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
      break;

    case IT_LOAD:
      /* ARM_REGB is always read, ARM_REGC if the offset is stored in a register,
       * ARM_REGS if the shift amount is stored in a register */
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      if (ARM_INS_REGC(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGC(ins));
      if (ARM_INS_REGS(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGS(ins));
      if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_RRX) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      break;

    case IT_STORE:
      if (ARM_INS_OPCODE(ins) == ARM_STR)
      {
        if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
        else
          RegsetSetAddReg(mask, ARM_INS_REGA(ins));
      }
      else
      {
        /* ARM_REGA is always used (it's the register that is written to memory!) */
        /* ARM_REGB is always used, ARM_REGC only if the offset is stored in a register,
         * ARM_REGS if the shift amount is stored in a register */
        RegsetSetAddReg(mask,ARM_INS_REGA(ins));
        /* in case of a DoubleWord store, REGA+1 is also read */
        if (ARM_INS_OPCODE(ins) == ARM_STRD || ARM_INS_OPCODE(ins)==ARM_STREXD) RegsetSetAddReg(mask,ARM_INS_REGABIS(ins));
      }

      RegsetSetAddReg(mask,ARM_INS_REGB(ins));

      if((ARM_INS_OPCODE(ins) <= ARM_LDRSTREX_FIRST) || (ARM_LDRSTREX_LAST <= ARM_INS_OPCODE(ins)))
      {
        if (ARM_INS_REGC(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGC(ins));
        if (ARM_INS_REGS(ins) != ARM_REG_NONE) RegsetSetAddReg(mask,ARM_INS_REGS(ins));
        if (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_RRX) RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
      }
      break;

    case IT_FLT_STORE:
      if ((ARM_INS_OPCODE(ins)==ARM_STF) ||
          (ARM_INS_OPCODE(ins)==ARM_FSTS) ||
          (ARM_INS_OPCODE(ins)==ARM_FSTD))
      {
        RegsetSetAddReg(mask,ARM_INS_REGA(ins));
        if (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE)
          RegsetSetAddReg(mask,ARM_INS_REGA(ins)+1);
        RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      } else if (ARM_INS_OPCODE(ins)==ARM_VSTR) {
        if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) {
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
        } else {
          RegsetSetAddReg(mask, ARM_INS_REGA(ins));
        }
        RegsetSetAddReg(mask, ARM_INS_REGB(ins));
      }
      break;

    case IT_FLT_LOAD:
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      break;

    case IT_FLT_ALU:
    case IT_FLT_INT:
    case IT_INT_FLT:
      if ( (ARM_FP_VCVT_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_FP_VCVT_LAST) )
      {
        /* VCVT floating-point instructions are a little different:
         *  - double or single sized registers are not specified by the FL_VFP_DOUBLE-flag
         *  - register size depends on the instruction
         *  - the B-register is always used, the C-register never
         * ... but an extra property we can take into account are the DATATYPE and DATATYPEOP fields
         */

        /* if the VFP_DOUBLE-flag is defined, all operands (destination register also) are double-sized */
        if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGB(ins));
        else
        {
          if(ARM_INS_DATATYPEOP(ins) != DT_NONE)
          {
            if(ARM_INS_DATATYPEOP(ins) == DT_F64)
              mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGB(ins));
            else
              RegsetSetAddReg(mask, ARM_INS_REGB(ins));
          }
          else
            RegsetSetAddReg(mask, ARM_INS_REGB(ins));
        }
      }
      else if ( (ARM_FP_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_FP_LAST) )
      {
        if(ARM_INS_REGB(ins) != ARM_REG_NONE)
        {
          if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
            mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGB(ins));
          else
            RegsetSetAddReg(mask, ARM_INS_REGB(ins));
        }
        if(ARM_INS_REGC(ins) != ARM_REG_NONE)
        {
          if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
            mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGC(ins));
          else
            RegsetSetAddReg(mask, ARM_INS_REGC(ins));
        }
      }
      else
      {
        if ((ARM_INS_REGC(ins)!=ARM_REG_NONE) &&
            (ARM_INS_OPCODE(ins)!=ARM_FMRRD) &&
            (ARM_INS_OPCODE(ins)!=ARM_FMRRS))
        {
          RegsetSetAddReg(mask,ARM_INS_REGC(ins));
          if (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE)
            RegsetSetAddReg(mask,ARM_INS_REGC(ins)+1);
        }
        RegsetSetAddReg(mask,ARM_INS_REGB(ins));
        /* FMRRS also transfers two successive single-precision vfp regs */
        if ((ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE) ||
            (ARM_INS_OPCODE(ins)==ARM_FMRRS))
        {
          switch (ARM_INS_OPCODE(ins))
          {
            case ARM_FCVTDS:
            case ARM_FUITOD:
            case ARM_FSITOD:
            case ARM_FMDRR:
              break;

            default:
              RegsetSetAddReg(mask,ARM_INS_REGB(ins)+1);
              break;
          }
        }
      }
      break;

    case IT_LOAD_MULTIPLE:
    case IT_STORE_MULTIPLE:
      /* ARM_REGB is always used and the other used registers are stored in the immediate field, */
      /* provided that we're dealing with an STM instruction! */
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));

      if(ARM_INS_REGC(ins) != ARM_REG_NONE)
        RegsetSetAddReg(mask, ARM_INS_REGC(ins));

      if((ARM_INS_OPCODE(ins) == ARM_VPUSH) || (ARM_INS_OPCODE(ins) == ARM_VPOP))
        RegsetSetAddReg(mask, ARM_REG_R13);

      if(((ARM_SIMD_FIRSTSTORE <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTSTORE)) ||
          (ARM_INS_OPCODE(ins) == ARM_VPUSH) ||
          (ARM_INS_OPCODE(ins) == ARM_VSTM))
      {
        t_reg dreg;

        /* first, add all registers marked in the multiple field */
        RegsetSetUnion(mask, ARM_INS_MULTIPLE(ins));

        if((ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) ||
          ((ARM_SIMD_FIRSTSTORE <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTSTORE)))
        {
          /* then, for each S-register that was added, add the next S-register to use the D-register */
          for(dreg = ARM_REG_S0; dreg <= ARM_REG_S31; dreg += 2)
            if(RegsetIn(mask, dreg))
              RegsetSetAddReg(mask, dreg+1);
        }
      }
      else
      {
        if (ARM_INS_OPCODE(ins) == ARM_STM) RegsetSetAddMultipleRegs(mask,ARM_INS_IMMEDIATE(ins));
      }

      if ((ARM_SIMD_FIRSTLOAD <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTLOAD))
      {
        if (!(ARM_INS_OPCODE(ins)==ARM_VLD1_ALL || ARM_INS_OPCODE(ins)==ARM_VLD2_ALL || ARM_INS_OPCODE(ins)==ARM_VLD3_ALL || ARM_INS_OPCODE(ins)==ARM_VLD4_ALL))
        RegsetSetUnion(mask,ArmDefinedRegisters(ins));
      }
      break;

    case IT_SWAP:
      RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      RegsetSetAddReg(mask,ARM_INS_REGC(ins));
      break;

    case IT_STATUS:
      /* MRS uses CPSR or SPSR depending on whether FL_SPSR is set */
      /* MSR uses ARM_REGC if the value isn't given as an immediate */
      if (ARM_INS_OPCODE(ins) == ARM_MRS)
      {
        if (ARM_INS_FLAGS(ins) & FL_SPSR)
          RegsetSetAddReg(mask,21);
        else
        {
          RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
          RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
        }
      }
      else if (ARM_INS_REGC(ins) != ARM_REG_NONE)
        RegsetSetAddReg(mask,ARM_INS_REGC(ins));
      break;

    case IT_FLT_STATUS:
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      break;

    case IT_DATA:
      mask = NullRegs;
      break;

    case IT_CONSTS:
      break;

    case IT_CALL:
      RegsetSetAddReg(mask,ARM_REG_R0);
      RegsetSetAddReg(mask,ARM_REG_R1);
      RegsetSetAddReg(mask,ARM_REG_R2);
      RegsetSetAddReg(mask,ARM_REG_R3);
      break;

    case IT_UNKNOWN:
      /* all kinds of special stuff ends up here */
      switch (ARM_INS_OPCODE(ins))
      {
        case ARM_MRC:
        case ARM_MRC2:
          /* no ARM regs used */
          break;

        case ARM_MCR:
        case ARM_MCR2:
          RegsetSetAddReg(mask,ARM_INS_REGC(ins));
          break;

        case ARM_MRRC:
        case ARM_MRRC2:
          /* no ARM regs used */
          break;

        case ARM_MCRR:
        case ARM_MCRR2:
          RegsetSetAddReg(mask,ARM_INS_REGB(ins));
          RegsetSetAddReg(mask,ARM_INS_REGC(ins));
          break;

        case ARM_LDC:
        case ARM_LDC2:
        case ARM_STC:
        case ARM_STC2:
          RegsetSetAddReg(mask,ARM_INS_REGB(ins));
          break;

        case ARM_PLD:
        case ARM_PLDW:
        case ARM_PLI:
          RegsetSetAddReg(mask,ARM_INS_REGB(ins));
          if (ARM_INS_REGC(ins) != ARM_REG_NONE)
            RegsetSetAddReg(mask,ARM_INS_REGC(ins));
          break;

        case ARM_CDP:
        case ARM_CDP2:
          /* no ARM regs used */
          break;

        case ARM_SETEND:
          /* no regs used */
          break;

        case ARM_CPSIE:
        case ARM_CPSID:
        case ARM_CPS:
          /* no regs used */
          break;

        default:
          FATAL(("Implement"));
      }
      break;

    case IT_SIMD:
      if(ARM_INS_OPCODE(ins)==ARM_VMOV_C2SCALAR)
      {
        mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
      }

      if(ARM_INS_REGB(ins) != ARM_REG_NONE)
      {
        if(ARM_INS_NEONFLAGS(ins) & NEONFL_B_QUAD)
          mask = ArmRegsetAddQuadReg(mask, ARM_INS_REGB(ins));
        else if((ARM_INS_NEONFLAGS(ins) & NEONFL_B_DOUBLE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_B_SCALAR))
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGB(ins));
        else if((ARM_INS_NEONFLAGS(ins) & NEONFL_B_CORE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_B_SINGLE))
          RegsetSetAddReg(mask, ARM_INS_REGB(ins));
        else
          FATAL(("Illegal first operand register type"));
      }
      if(ARM_INS_REGC(ins) != ARM_REG_NONE)
      {
        if(ARM_INS_NEONFLAGS(ins) & NEONFL_C_QUAD)
          mask = ArmRegsetAddQuadReg(mask, ARM_INS_REGC(ins));
        else if((ARM_INS_NEONFLAGS(ins) & NEONFL_C_DOUBLE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_C_SCALAR))
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGC(ins));
        else if((ARM_INS_NEONFLAGS(ins) & NEONFL_C_CORE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_C_SINGLE))
          RegsetSetAddReg(mask, ARM_INS_REGC(ins));
        else
          FATAL(("Illegal second operand register type: @I", ins));
      }

      if((ARM_INS_OPCODE(ins) == ARM_VTBL) || (ARM_INS_OPCODE(ins) == ARM_VTBX))
      {
        t_reg dreg;

        /* first, add all registers marked in the multiple field */
        RegsetSetUnion(mask, ARM_INS_MULTIPLE(ins));

        /* then, for each S-registers that was added, add the next S-register */
        for(dreg = ARM_REG_S0; dreg <= ARM_REG_S31; dreg += 2)
          if(RegsetIn(mask, dreg))
            RegsetSetAddReg(mask, dreg+1);
      }

      break;

    case IT_NOP:
      break;

    case IT_SYNC:
      break;

    default:
      FATAL(("Implement type %d!",ARM_INS_TYPE(ins)));
    }

  switch (ARM_INS_OPCODE(ins)) {
  case ARM_SMLALBB:
  case ARM_SMLALBT:
  case ARM_SMLALTB:
  case ARM_SMLALTT:
  case ARM_SMLALD:
  case ARM_SMLSLD:
  case ARM_UMAAL:
  case ARM_UMLAL:
  case ARM_VABA:
  case ARM_VABAL:
  case ARM_VFMA: case ARM_VFMA_F64:
  case ARM_VFMS: case ARM_VFMS_F64:
  case ARM_VFNMA:
  case ARM_VFNMS:
  case ARM_VMLA:
  case ARM_VMLAL:
  case ARM_VMLS:
  case ARM_VMLSL:
  case ARM_VMLA_F: case ARM_VMLA_F64:
  case ARM_VMLS_F: case ARM_VMLS_F64:
  case ARM_VMLA_SCALAR:
  case ARM_VMLS_SCALAR:
  case ARM_VMLAL_SCALAR:
  case ARM_VMLSL_SCALAR:
  case ARM_VNMLA:
  case ARM_VNMLS:
  case ARM_VPADAL:
  case ARM_VQDMLAL:
  case ARM_VQDMLSL:
  case ARM_VQDMLAL_SCALAR:
  case ARM_VQDMLSL_SCALAR:
  case ARM_VRSRA:
  case ARM_VSRA:
  case ARM_VSRI:
  case ARM_VSLI:
  case ARM_VBIT:
  case ARM_VBIC_IMM_0:
  case ARM_VBIC_IMM_1:
  case ARM_VBIF:
  case ARM_VBSL:
  case ARM_VUZP:
  case ARM_VZIP:
  case ARM_VTRN:
  case ARM_VSWP:
  case ARM_VORR_IMM_0:
  case ARM_VORR_IMM_1:
	 RegsetSetUnion(mask,ArmDefinedRegisters(ins));
  default:
	 break;
  }

  return mask;
}
/* }}} */
/*!
 * Returns a bitmask specifying which registers are modified by this
 * instruction the bottommost sixteen bits represent the usual registers r0 -
 * r15 bit 16 is used to represent the CPSR (i.e. instructions having the S
 * flag change this register) bit 17 is used to represent the SPSR (possibly
 * referenced in the MRS and MSR instructions
 *
 * \param ins
 *
 * \return t_regset
*/
/* ArmDefinedRegisters {{{ */
t_regset ArmDefinedRegisters(t_arm_ins * ins)
{

  t_regset mask = NullRegs;

  if (ins == NULL) return NullRegs;
  if (ArmInsIsNOOP(ins)) return NullRegs;

  /* instructions which have the S flag set define the CPSR */
  if (ARM_INS_FLAGS(ins) & FL_S)
  {
    switch (ARM_INS_OPCODE(ins))
  	{
  	case ARM_CONSTANT_PRODUCER:
  	case ARM_ADDRESS_PRODUCER:
  	  break;
  	case ARM_PSEUDO_CALL:
  	  break;
  	case ARM_MUL:
    case ARM_UDIV:
  	case ARM_MLA:
  	  RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
  	  /*mask |= (1<<ARM_REG_Z_CONDITION)|(1<<ARM_REG_N_CONDITION)|(1<<ARM_REG_C_CONDITION);*/
  	  break;
  	case ARM_AND:
  	case ARM_TST:
  	case ARM_BIC:
  	case ARM_EOR:
  	case ARM_ORR:
    case ARM_T2ORN:
  	case ARM_TEQ:
  	case ARM_MOV:
  	case ARM_MVN:
  	  RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
  	  /*mask |= (1<<ARM_REG_Z_CONDITION)|(1<<ARM_REG_N_CONDITION);*/
  	  break;
  	case ARM_CMN:
  	case ARM_CMP:
  	case ARM_ADD:
  	case ARM_ADC:
  	case ARM_SUB:
  	case ARM_SBC:
  	case ARM_RSC:
  	case ARM_RSB:
  	  RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
  	  /*mask |= (1<<ARM_REG_Z_CONDITION)|(1<<ARM_REG_N_CONDITION)|(1<<ARM_REG_C_CONDITION)|(1<<ARM_REG_V_CONDITION);*/
  	  break;
  	case ARM_LDR:
  	case ARM_LDRB:
  	case ARM_STR:
  	case ARM_STRB:
  	case ARM_LDM:
  	  break;

  	default:
  	  RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
  	  RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
  	  /*mask |= (1<<ARM_REG_Z_CONDITION)|(1<<ARM_REG_N_CONDITION)|(1<<ARM_REG_C_CONDITION)|(1<<ARM_REG_V_CONDITION);*/
  	}

    if (ArmInsHasShiftedFlexible(ins))
	    RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
	    /*mask|=1<<ARM_REG_C_CONDITION;*/
  }
  if ((ARM_INS_OPCODE(ins)==ARM_CMP) && (!(ARM_INS_FLAGS(ins) & FL_S))) { DEBUG(("CMP without set (@I)!\n", ins)); }
  if ((ARM_INS_OPCODE(ins)==ARM_CMFE) && (!(ARM_INS_FLAGS(ins) & FL_S))) { DEBUG(("CMFE without set!\n")); }

  switch (ARM_INS_TYPE(ins))
  {
  case IT_BRANCH:
    /* branches define the pc */
    RegsetSetAddReg(mask,15);/*mask |= (1 << 15);*/
    if (ARM_INS_OPCODE(ins)==ARM_BL || ARM_INS_OPCODE(ins)==ARM_BLX)
      RegsetSetAddReg(mask,14);/*mask |= (1<<14);*/
    break;

  case IT_SWI:
    /* SWI's defines _all_ registers */
    {
      t_uint32 r;
      for (r=0; r<5; r++)
        RegsetSetAddReg(mask,r);
      switch (ARM_INS_IMMEDIATE(ins))
      {
        case 0x900001:
          for (r = ARM_REG_R6; r < ARM_REG_R15; r++)
            RegsetSetAddReg(mask,r);
          break;
        default:
          break;
      }
    }
    break;

  case IT_DATAPROC:
    /* all dataprocessing instructions look the same in a certain way: */
    /* only ARM_REGA can be defined */

    if (ARM_INS_REGA(ins) != ARM_REG_NONE)  RegsetSetAddReg(mask,ARM_INS_REGA(ins));/*mask |= (1 << ARM_INS_REGA(ins)); */
    /* saturated arithmetic instructions may also define the Q flag
     * Note this has nothing to do with FL_S: these instructions don't even have a FL_S */
    if (ARM_INS_OPCODE(ins) == ARM_QADD) RegsetSetAddReg(mask, ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_QDADD) RegsetSetAddReg(mask, ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_QSUB) RegsetSetAddReg(mask, ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_QDSUB) RegsetSetAddReg(mask, ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SSAT) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_USAT) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SSAT16) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_USAT16) RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);

    if (ARM_INS_OPCODE(ins) == ARM_UADD8) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_UADD16) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SADD8) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SADD16) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_USUB8) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_USUB16) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SSUB8) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SSUB16) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SADDSUBX) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_SSUBADDX) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_UADDSUBX) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    if (ARM_INS_OPCODE(ins) == ARM_USUBADDX) RegsetSetAddReg(mask,ARM_REG_GE_CONDITION);
    break;

  case IT_MUL:
  case IT_DIV:
    /* the values in ARM_REGB and ARM_REGC are certainly read, ARM_REGA is only read in SMLAL and UMLAL, and always written, */
    RegsetSetAddReg(mask,ARM_INS_REGA(ins));
    /* ARM_REGS is written in all the long (L) variants */
    if (ARM_INS_OPCODE(ins) == ARM_SMLAL ||
        ARM_INS_OPCODE(ins) == ARM_UMLAL ||
        ARM_INS_OPCODE(ins) == ARM_SMULL ||
        ARM_INS_OPCODE(ins) == ARM_UMULL ||
        ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
        ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
        ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
        ARM_INS_OPCODE(ins) == ARM_SMLALTT ||
        ARM_INS_OPCODE(ins) == ARM_SMLALD ||
        ARM_INS_OPCODE(ins) == ARM_SMLALDX ||
        ARM_INS_OPCODE(ins) == ARM_SMLSLD ||
        ARM_INS_OPCODE(ins) == ARM_SMLSLDX ||
        ARM_INS_OPCODE(ins) == ARM_UMAAL)
      RegsetSetAddReg(mask,ARM_INS_REGS(ins));

    /* saturated arithmetic instructions may set the Q flag */
    if (ARM_INS_OPCODE(ins) == ARM_SMLABB ||
        ARM_INS_OPCODE(ins) == ARM_SMLABT ||
        ARM_INS_OPCODE(ins) == ARM_SMLATB ||
        ARM_INS_OPCODE(ins) == ARM_SMLATT ||
        ARM_INS_OPCODE(ins) == ARM_SMLAWB ||
        ARM_INS_OPCODE(ins) == ARM_SMLAWT ||
        ARM_INS_OPCODE(ins) == ARM_SMLALBB ||
        ARM_INS_OPCODE(ins) == ARM_SMLALBT ||
        ARM_INS_OPCODE(ins) == ARM_SMLALTB ||
        ARM_INS_OPCODE(ins) == ARM_SMLALTT ||
        ARM_INS_OPCODE(ins) == ARM_SMULBB ||
        ARM_INS_OPCODE(ins) == ARM_SMULBT ||
        ARM_INS_OPCODE(ins) == ARM_SMULTB ||
        ARM_INS_OPCODE(ins) == ARM_SMULTT ||
        ARM_INS_OPCODE(ins) == ARM_SMULWB ||
        ARM_INS_OPCODE(ins) == ARM_SMULWT ||
        ARM_INS_OPCODE(ins) == ARM_SMUAD ||
        ARM_INS_OPCODE(ins) == ARM_SMUADX ||
        ARM_INS_OPCODE(ins) == ARM_SMLAD ||
        ARM_INS_OPCODE(ins) == ARM_SMLADX ||
        ARM_INS_OPCODE(ins) == ARM_SMLSD ||
        ARM_INS_OPCODE(ins) == ARM_SMLSDX )
      RegsetSetAddReg(mask,ARM_REG_Q_CONDITION);
    break;

  case IT_LOAD:
    /* ARM_REGA is always defined, ARM_REGB is defined if writeback is specified */
    RegsetSetAddReg(mask,ARM_INS_REGA(ins));/*mask |= (1 << ARM_INS_REGA(ins));*/
    /* in case of a DoubleWord load, REGA+1 is also defined */
    if (ARM_INS_OPCODE(ins)==ARM_LDRD || ARM_INS_OPCODE(ins)==ARM_LDREXD) RegsetSetAddReg(mask,ARM_INS_REGABIS(ins));
    if (ArmInsWriteBackHappens(ins)) RegsetSetAddReg(mask,ARM_INS_REGB(ins));/*mask |= (1 << ARM_INS_REGB(ins));*/
    break;

  case IT_STORE:
    /* ARM_REGB is defined if writeback is specified */
    if ((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_LDRSTREX_LAST))
    {
      /* define the result register for the exclusive stores */
      RegsetSetAddReg(mask, ARM_INS_REGC(ins));
    }
    if (ArmInsWriteBackHappens(ins)) RegsetSetAddReg(mask,ARM_INS_REGB(ins));/*mask |= (1 << ARM_INS_REGB(ins));*/
    break;

  case IT_LOAD_MULTIPLE:
  case IT_STORE_MULTIPLE:
    /* ARM_REGB is defined in case of writeback and the other defined registers are stored in the immediate field, */
    /* provided that we're dealing with an LDM instruction! */

    if((ARM_INS_OPCODE(ins) == ARM_VPUSH) || (ARM_INS_OPCODE(ins) == ARM_VPOP))
      RegsetSetAddReg(mask, ARM_REG_R13);

    if(((ARM_SIMD_FIRSTLOAD <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTLOAD)) ||
        (ARM_INS_OPCODE(ins) == ARM_VPOP) ||
        (ARM_INS_OPCODE(ins) == ARM_VLDM))
    {
      t_reg dreg;

      /* first, add all registers marked in the multiple field */
      RegsetSetUnion(mask, ARM_INS_MULTIPLE(ins));

      if((ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) ||
        ((ARM_SIMD_FIRSTLOAD <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_SIMD_LASTLOAD)))
      {
        /* then, for each S-register that was added, add the next S-register to use the D-register */
        for(dreg = ARM_REG_S0; dreg <= ARM_REG_S31; dreg += 2)
          if(RegsetIn(mask, dreg))
            RegsetSetAddReg(mask, dreg+1);
      }
    }
    if (ArmInsWriteBackHappens(ins)) RegsetSetAddReg(mask,ARM_INS_REGB(ins));/*mask |= (1 << ARM_INS_REGB(ins));  */
    if (ARM_INS_OPCODE(ins) == ARM_LDM) RegsetSetAddMultipleRegs(mask,ARM_INS_IMMEDIATE(ins));/*mask |= ARM_INS_IMMEDIATE(ins);*/
    break;

  case IT_SWAP:
    RegsetSetAddReg(mask,ARM_INS_REGA(ins));/*mask |= (1 << ARM_INS_REGA(ins));*/
    break;

  case IT_STATUS:
    /* MRS defines ARM_REGA */
    /* MSR defines CPSR or SPSR depending on the value of the FL_SPSR flag */
    if (ARM_INS_OPCODE(ins) == ARM_MRS) {
      RegsetSetAddReg(mask,ARM_INS_REGA(ins));/*mask |= (1 << ARM_INS_REGA(ins));*/
    } else
      if (ARM_INS_FLAGS(ins) & FL_SPSR)
        RegsetSetAddReg(mask,17);/*mask |= (1 << 17);*/
      else if (ARM_INS_FLAGS(ins) & FL_STATUS)
      {
        RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
      }
    break;

  case IT_FLT_STATUS:
    RegsetSetAddReg(mask,ARM_INS_REGA(ins));
    break;

  case IT_FLT_STORE:
    if (ArmInsWriteBackHappens(ins)) RegsetSetAddReg(mask,ARM_INS_REGB(ins));
    break;

  case IT_FLT_LOAD:
    if (ArmInsWriteBackHappens(ins)) RegsetSetAddReg(mask,ARM_INS_REGB(ins));
    if ((ARM_INS_OPCODE(ins) == ARM_LDF) ||
        (ARM_INS_OPCODE(ins) == ARM_FLDS) ||
        (ARM_INS_OPCODE(ins) == ARM_FLDD))
    {
      RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      if (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE)
        RegsetSetAddReg(mask,ARM_INS_REGA(ins)+1);
    }
    else if(ARM_INS_OPCODE(ins) == ARM_VLDR)
    {
      if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) {
        mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
      } else {
        RegsetSetAddReg(mask, ARM_INS_REGA(ins));
      }
    }
    else
    {
      RegsetSetUnion(mask,ARM_INS_MULTIPLE(ins));
    }
    break;

  case IT_FLT_ALU:
	 if (ARM_INS_OPCODE(ins)==ARM_VCMP || ARM_INS_OPCODE(ins)==ARM_VCMPE)
		RegsetSetAddReg(mask,ARM_REG_FPSCR);

  case IT_FLT_INT:
  case IT_INT_FLT:
    if ( (ARM_FP_VCVT_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_FP_VCVT_LAST) )
    {
      /* if the VFP_DOUBLE-flag is defined, all operands (destination register also) are double-sized */
      if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE) {
        mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
      }
      else
      {
        if(ARM_INS_DATATYPE(ins) != DT_NONE)
        {
          if(ARM_INS_DATATYPE(ins) == DT_F64)
            mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
          else
            RegsetSetAddReg(mask, ARM_INS_REGA(ins));
        }
        else
          RegsetSetAddReg(mask, ARM_INS_REGA(ins));
      }
    }
    else if ( (ARM_FP_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_FP_LAST) )
    {
      if(ARM_INS_REGA(ins) != ARM_REG_NONE)
      {
        if(ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
          mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
        else
          RegsetSetAddReg(mask, ARM_INS_REGA(ins));
      }
    }
    else
    {
      if (ARM_INS_REGA(ins)!=ARM_REG_NONE)
      {
        RegsetSetAddReg(mask,ARM_INS_REGA(ins));
        if (ARM_INS_FLAGS(ins)&FL_VFP_DOUBLE)
        {
          switch (ARM_INS_OPCODE(ins))
          {
          case ARM_FCVTSD:
          case ARM_FTOUID:
          case ARM_FTOUIZD:
          case ARM_FTOSID:
          case ARM_FTOSIZD:
          case ARM_FMRRD:
            break;
          default:
            RegsetSetAddReg(mask,ARM_INS_REGA(ins)+1);
            break;
          }
        }
      }
    }

    /* these two instructions transfer 64 bit floating point state to
     * two integer registers
     */
    if ((ARM_INS_OPCODE(ins)==ARM_FMRRD) ||
        (ARM_INS_OPCODE(ins)==ARM_FMRRS))
      RegsetSetAddReg(mask,ARM_INS_REGC(ins));
    break;

  case IT_DATA:
    mask = NullRegs;
    break;

  case IT_CONSTS:
    if (ARM_INS_FLAGS(ins) & FL_VFP_DOUBLE)
      mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
    else
      RegsetSetAddReg(mask, ARM_INS_REGA(ins));
    break;

  case IT_CALL:
    RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
    RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
    RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
    RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
    RegsetSetAddReg(mask,ARM_REG_R0);
    RegsetSetAddReg(mask,ARM_REG_R1);
    RegsetSetAddReg(mask,ARM_REG_R2);
    RegsetSetAddReg(mask,ARM_REG_R3);
    RegsetSetAddReg(mask,ARM_REG_R10);
    RegsetSetAddReg(mask,ARM_REG_R11);
    RegsetSetAddReg(mask,ARM_REG_R12);
    RegsetSetAddReg(mask,ARM_REG_R14);
    break;

  case IT_UNKNOWN:
    /* all kinds of special stuff ends up here */
    switch (ARM_INS_OPCODE(ins))
    {
    case ARM_MRC:
    case ARM_MRC2:
      if (ARM_INS_FLAGS(ins) & FL_S)
      {
        /* special case */
        ASSERT(ARM_INS_REGA(ins) == ARM_REG_NONE,("internal instruction representation corrupt: @I",ins));
      }
      else
        RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      break;

    case ARM_MCR:
    case ARM_MCR2:
      /* no ARM regs defined */
      break;

    case ARM_LDC:
    case ARM_LDC2:
    case ARM_STC:
    case ARM_STC2:
      if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
        if (ARM_INS_IMMEDIATE(ins) & 0xff00) /* immediate offset != 0 */
          RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      break;

    case ARM_MRRC:
    case ARM_MRRC2:
      RegsetSetAddReg(mask,ARM_INS_REGA(ins));
      RegsetSetAddReg(mask,ARM_INS_REGB(ins));
      break;

    case ARM_MCRR:
    case ARM_MCRR2:
      /* no arm registers are defined here */
      break;

    case ARM_CDP:
    case ARM_CDP2:
      /* no ARM regs used */
      break;

    case ARM_PLD:
    case ARM_PLDW:
    case ARM_PLI:
      /* no registers defined here */
      break;

    case ARM_SETEND:
      /* no action needed */
      break;

    case ARM_CPSIE:
    case ARM_CPSID:
    case ARM_CPS:
      /* no regs defined */
      break;

    default:
      FATAL(("Implement @I",ins));
    }
    break;

  case IT_NOP:
    break;

  case IT_SYNC:
    break;

  case IT_SIMD:
	 if(ARM_INS_REGA(ins) != ARM_REG_NONE)
    {
      if(ARM_INS_REGA(ins) == ARM_REG_CPSR) {
        RegsetSetAddReg(mask,ARM_REG_Z_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_N_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_C_CONDITION);
        RegsetSetAddReg(mask,ARM_REG_V_CONDITION);
      } else if(ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD)
        mask = ArmRegsetAddQuadReg(mask, ARM_INS_REGA(ins));
      else if((ARM_INS_NEONFLAGS(ins) & NEONFL_A_DOUBLE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_A_SCALAR))
        mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGA(ins));
      else if((ARM_INS_NEONFLAGS(ins) & NEONFL_A_CORE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_A_SINGLE))
        RegsetSetAddReg(mask, ARM_INS_REGA(ins));
      else
        FATAL(("Illegal destination register type"));
    } else
      ASSERT(ARM_INS_REGABIS(ins)==ARM_REG_NONE, ("REGA(bis) not equal to ARM_REG_NONE, while REGA equals ARM_REG_NONE"));

    if(ARM_INS_REGABIS(ins) != ARM_REG_NONE)
    {
      if(ARM_INS_NEONFLAGS(ins) & NEONFL_A_QUAD)
        mask = ArmRegsetAddQuadReg(mask, ARM_INS_REGABIS(ins));
      else if((ARM_INS_NEONFLAGS(ins) & NEONFL_A_DOUBLE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_A_SCALAR))
        mask = ArmRegsetAddDoubleReg(mask, ARM_INS_REGABIS(ins));
      else if((ARM_INS_NEONFLAGS(ins) & NEONFL_A_CORE) || (ARM_INS_NEONFLAGS(ins) & NEONFL_A_SINGLE))
        RegsetSetAddReg(mask, ARM_INS_REGABIS(ins));
      else
        FATAL(("Illegal destination register (bis) type: @I", ins));
    }

    switch(ARM_INS_OPCODE(ins))
      {
      case ARM_VUZP:
      case ARM_VZIP:
      case ARM_VTRN:
      case ARM_VSWP:
        RegsetAddReg(mask,ARM_INS_REGB(ins));
      default:
        break;
      }



    break;

  default:
    FATAL(("Implement type %d!",ARM_INS_TYPE(ins)));
  }

  return mask;
}
/* }}} */
/*!
 * \todo Document
 *
 * \param ins
 *
 * \return t_bool
*/
/*!
 * Returns the memory size touched by the instruction
 *
 * \param ins
 *
 * \return int the memory size touched by the instruction; -1 if unknown/unimplemented
*/
 /*ArmInsMemSize{{{*/
t_int32 ArmInsMemSize(t_arm_ins *ins)
{
  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_LDRB:
    case ARM_LDRSB:
    case ARM_STRB:
      return 1;
    case ARM_LDRH:
    case ARM_LDRSH:
    case ARM_STRH:
      return 2;
    case ARM_LDR:
    case ARM_STR:
      return 4;
    case ARM_LDRD:
    case ARM_STRD:
      return 8;
    case ARM_LDM:
    case ARM_STM:
    {
      t_regset mask;
      RegsetSetEmpty(mask);
      RegsetSetAddMultipleRegs(mask,ARM_INS_IMMEDIATE(ins));
      return (RegsetCountRegs(mask)*4);
    }
    default:
      if ((ARM_INS_TYPE(ins)==IT_LOAD) ||
          (ARM_INS_TYPE(ins)==IT_STORE) ||
	  (ARM_INS_TYPE(ins)==IT_FLT_LOAD) ||
	  (ARM_INS_TYPE(ins)==IT_FLT_STORE) ||
	  (ARM_INS_TYPE(ins)==IT_LOAD_MULTIPLE) ||
	  (ARM_INS_TYPE(ins)==IT_STORE_MULTIPLE))
	return -1;
      else
        return 0;
  }
}
/*}}}*/
/*!
 *
*/
/*{{{*/
t_int32 ArmInsMemBaseAdjustment(t_arm_ins *ins)
{
  if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
  {
    int memsize;

    memsize = ArmInsMemSize(ins);
    if (memsize==-1)
      FATAL(("Unsupported memsize for @I",ins));
    if (ARM_INS_FLAGS(ins) & FL_DIRUP)
      return memsize;
    else
      return -memsize;
  }
  return 0;
}
/*}}}*/
/*!
 * Determine the start and end offsets of the memory touched by a
 * memory instruction
 *
 * \param ins the (memory-touching) instruction
 * \para ofs_low valid pointer to t_uint32 in which the starting offset will be stored ((t_uint32)-1 if unknown)
 * \para ofs_high valid pointer to t_uint32 in which the last offset (inclusive) will be stored (undefined if unknown)
 *
 * \return void
*/
/*ArmInsGetMemOffsetRangeTouched{{{*/
void ArmInsGetMemOffsetRangeTouched(t_arm_ins *ins, t_int32 *ofs_low, t_int32 *ofs_high)
{
  switch (ARM_INS_TYPE(ins))
  {
    case IT_STORE_MULTIPLE:
    case IT_STORE:
    {
      t_uint32 ofs;
      int memsize;

      memsize = ArmInsMemSize(ins);
      if (memsize==-1)
      {
        *ofs_low=-1;
	return;
      }

      if (ARM_INS_TYPE(ins)==IT_STORE_MULTIPLE)
        ofs = 0;
      else
        ofs = (t_uint32)ARM_INS_IMMEDIATE(ins);
      if (ARM_INS_FLAGS(ins) & FL_PREINDEX)
      {
        if (ARM_INS_FLAGS(ins) & FL_DIRUP)
	{
	  *ofs_low = ARM_INS_IMMEDIATE(ins) + memsize;
	  *ofs_high = ARM_INS_IMMEDIATE(ins) + 2*memsize-1;
	}
	else
	{
	  *ofs_low = ARM_INS_IMMEDIATE(ins) - memsize;
	  *ofs_high = ARM_INS_IMMEDIATE(ins) - 1;
	}
      }
      else
      {
        *ofs_low = ARM_INS_IMMEDIATE(ins);
	*ofs_high = ARM_INS_IMMEDIATE(ins) + memsize-1;
      }
      return;
    }
    case IT_SWI:
      *ofs_low=-1;
      return;
    default:
      *ofs_low=2;
      *ofs_high=1;
      return;
  }
}
/*}}}*/
/* ArmInsIsCommutative {{{ */
t_bool ArmInsIsCommutative(t_arm_ins * ins)
{
  t_bool thumb16 = (ARM_INS_FLAGS(ins) & FL_THUMB) && (ARM_INS_CSIZE(ins) == 2);

  switch(ARM_INS_OPCODE(ins))
  {
    case ARM_MUL:
    case ARM_UDIV:
    case ARM_MLA:
    case ARM_UMULL:
    case ARM_UMLAL:
    case ARM_SMULL:
    case ARM_SMLAL:
      return FALSE;
      /* this may strike you as odd, but you can't _always_ swap the registers here because mul
       * cant have the first operand the same as the destination register */

    case ARM_ADD:
      if (thumb16 && (ARM_INS_REGB(ins) == ARM_REG_R13))
        return FALSE;
      else if (thumb16)
      {
        /* only problem if using the higher registers (encoding T2) */
        if ((ARM_INS_REGA(ins) >= ARM_REG_R8 || ARM_INS_REGC(ins) >= ARM_REG_R8) &&
            (ARM_INS_REGA(ins) != ARM_INS_REGC(ins)))
          return FALSE;
      }

      return TRUE;

    case ARM_ADC:
    case ARM_AND:
    case ARM_EOR:
    case ARM_ORR:
      return !(thumb16);

    case ARM_TST:
    case ARM_TEQ:
      return TRUE;

    default:
      /* keep compiler happy */
      return FALSE;
  }
  /*return FALSE;*/
}
/* }}} */
/*!
 * return the inverted condition code (eg AL becomes NV)
 *
 * \param condition
 *
 * \return
 */
/* ArmInvertCondition {{{ */
t_arm_condition_code ArmInvertCondition(t_arm_condition_code condition)
{
  switch (condition)
  {
    case ARM_CONDITION_EQ:
      return ARM_CONDITION_NE;
    case ARM_CONDITION_NE:
      return ARM_CONDITION_EQ;
    case ARM_CONDITION_CS:
      return ARM_CONDITION_CC;
    case ARM_CONDITION_CC:
      return ARM_CONDITION_CS;
    case ARM_CONDITION_MI:
      return ARM_CONDITION_PL;
    case ARM_CONDITION_PL:
      return ARM_CONDITION_MI;
    case ARM_CONDITION_VS:
      return ARM_CONDITION_VC;
    case ARM_CONDITION_VC:
      return ARM_CONDITION_VS;
    case ARM_CONDITION_HI:
      return ARM_CONDITION_LS;
    case ARM_CONDITION_LS:
      return ARM_CONDITION_HI;
    case ARM_CONDITION_GE:
      return ARM_CONDITION_LT;
    case ARM_CONDITION_LT:
      return ARM_CONDITION_GE;
    case ARM_CONDITION_GT:
      return ARM_CONDITION_LE;
    case ARM_CONDITION_LE:
      return ARM_CONDITION_GT;
    case ARM_CONDITION_AL:
      return ARM_CONDITION_NV;
    case ARM_CONDITION_NV:
      return ARM_CONDITION_AL;
  }
  /* just keep the compiler happy */
  return ARM_CONDITION_NV;
}
/* }}} */
/*!
 *
 * Checks if a constant can be encoded for a certain opcode
 *
 * \param cons the constant to encode
 *
 * \param opc the opcode
 *
 * \return t_bool TRUE if the value can be encoded else FALSE
*/
/* ArmIsEncodableConstantForOpcode {{{ */
t_bool ArmIsEncodableConstantForOpcode(t_uint32 cons, t_uint16 opc) {
        return ArmInsIsEncodableConstantForOpcode(cons, opc, FALSE);
}

t_bool ArmIsEncodableConstantForOpcodeThumb(t_uint32 cons, t_uint16 opc) {
        return ArmInsIsEncodableConstantForOpcode(cons, opc, TRUE);
}

/* For now, be conservative when checking whether an immediate can be encoded in a
   Thumb or ARM instruction.

   For example, the LDR (immediate) Thumb instruction can in general only encode a
   5-bit immediate, except for the case when REGB == R13 (then an 8-bit immediate
   is possible). Also, a 12-bit immediate is encodable in the 32-bit variant of
   this instruction. However, this 32-bit instruction is not allowed in IT-blocks.
   For simplicity, just do a very conservative check: only return TRUE when the
   immediate is 5-bit encodable. We suspect that there will be no significant loss,
   since this function is only called from the constant propagation code.

   Caveat: using this reasoning scheme, also PC-relative Thumb instructions are affected.
   */
t_bool ArmInsIsEncodableConstantForOpcode(t_uint32 cons, t_uint16 opc, t_bool thumb) {
   int i;

   /* most ARMv6 instructions cannot encode constants in regc */
   switch (opc)
   {
    /* case ARM_RRX: */
     case ARM_BFI:
     case ARM_CLZ:
     case ARM_PKHBT:
     case ARM_PKHTB:
     case ARM_QADD:
     case ARM_QADD16:
     case ARM_QADD8:
     case ARM_QADDSUBX:
     case ARM_QDADD:
     case ARM_QDSUB:
     case ARM_QSUBADDX:
     case ARM_QSUB:
     case ARM_QSUB16:
     case ARM_QSUB8:
     case ARM_RBIT:
     case ARM_REV:
     case ARM_REV16:
     case ARM_REVSH:
     case ARM_SADD16:
     case ARM_SADD8:
     case ARM_SADDSUBX:
     case ARM_SBFX:
     case ARM_SDIV:
     case ARM_SEL:
     case ARM_SHADD16:
     case ARM_SHADD8:
     case ARM_SHADDSUBX:
     case ARM_SHSUBADDX:
     case ARM_SHSUB16:
     case ARM_SHSUB8:
     case ARM_SSUBADDX:
     case ARM_SSUB16:
     case ARM_SSUB8:
     case ARM_SWP:
     case ARM_SWPB:
     case ARM_SXTAB:
     case ARM_SXTAB16:
     case ARM_SXTAH:
     case ARM_SXTB:
     case ARM_SXTB16:
     case ARM_SXTH:
     case ARM_UADD16:
     case ARM_UADD8:
     case ARM_UADDSUBX:
     case ARM_UBFX:
     case ARM_UDIV:
     case ARM_UHADD16:
     case ARM_UHADD8:
     case ARM_UHADDSUBX:
     case ARM_UHSUBADDX:
     case ARM_UHSUB16:
     case ARM_UHSUB8:
     case ARM_UQADD16:
     case ARM_UQADD8:
     case ARM_UQADDSUBX:
     case ARM_UQSUBADDX:
     case ARM_UQSUB16:
     case ARM_UQSUB8:
     case ARM_USAD8:
     case ARM_USADA8:
     case ARM_USAT:
     case ARM_USAT16:
     case ARM_USUBADDX:
     case ARM_USUB16:
     case ARM_USUB8:
     case ARM_UXTAB:
     case ARM_UXTAB16:
     case ARM_UXTAH:
     case ARM_UXTB:
     case ARM_UXTB16:
     case ARM_UXTH:
       return FALSE;

     case ARM_T2ORN:
       return ArmIsThumb2ImmediateEncodable(cons);

     default:
       break;
   }

   /* the vfp load/store multiple instructions use the constant in a special way
    * (it's interlinked with which registers are saved)
    */
  if ((opc == ARM_FLDMS) || (opc == ARM_FLDMD) || (opc == ARM_FLDMX) || (opc == ARM_FSTMS) || (opc == ARM_FSTMD) || (opc == ARM_FSTMX))
    return FALSE;

   if ((opc == ARM_LDR) || (opc == ARM_LDRB) || (opc == ARM_STR) || (opc == ARM_STRB))
   {
      /* twelve-bit constants */
      if (thumb)
        return ((cons & 0x1f) == cons);
      else
        return ((cons & 0xfff) == cons);
   }

   if (opc == ARM_MOVW)
   {
     /* 16-bit constant */
     return ((cons & 0xffff) == cons);
   }

   if ((opc == ARM_FLDS) || (opc == ARM_FLDD) || (opc == ARM_FSTS) || (opc == ARM_FSTD))
   {
     /* ten-bit 4-byte aligned constants */
     return ((cons & 0x3fc) == cons);
   }

   if ((opc == ARM_LDRH) || (opc == ARM_LDRSB) || (opc == ARM_LDRSH) || (opc == ARM_STRH) || (opc == ARM_LDRD) || (opc == ARM_STRD))
   {
     /* eight-bit constants */
     return ((cons & 0xff) == cons);
   }

    if (thumb)
    {
      return ArmIsThumb2ImmediateEncodable(cons) || ((opc == ARM_MOV) && ((cons & 0xff) == cons));
    }
    else
    {
      /* standard case: ARMExpandImm */
      for (i=0; i<32; i+=2) {
        if ((cons & Uint32RotateRight(0xff,i)) == cons)
          return TRUE;
    }
   }
   return FALSE;
}
/* }}} */
/*!
 * Checks if a constant can be produced in two instructions
 *
 * \param val The value to produce
 *
 * \return t_bool TRUE if it can be produced, FALSE if not.
*/
/* ArmConstantIsProducableInTwoInstructions {{{ */
t_bool ArmConstantIsProducableInTwoInstructions(t_uint32 val)
{
  int i;
  t_uint32 a;
  t_uint32 val_invert = ~val;

  /* Try to generate the constant from scratch in two instructions */

  for (i = 0; i < 32; i += 2)
    if (((a = Uint32RotateLeft (val, i)) & 0xff) != 0)
    {
      if (a & 0xff00)
      {
	if (a & ~ 0xffff)
	  continue;
      }
      else if (a & 0xff0000)
      {
	if (a & 0xff000000)
	  continue;
      }
      else
      {
      }

      return TRUE;
    }

  for (i = 0; i < 32; i += 2)
    if (((a = Uint32RotateLeft (val_invert, i)) & 0xff) != 0)
    {
      if (a & 0xff00)
      {
	if (a & ~ 0xffff)
	  continue;
      }
      else if (a & 0xff0000)
      {
	if (a & 0xff000000)
	  continue;
      }
      else
      {
      }

      return TRUE;
    }

  return FALSE;
}

/* }}} */
/*!
 * Encodes a constant in 2 instructions. The constant is produced in the
 * destination register of the instruction that is passed to the function. A
 * second instruction is inserted after this instruction to encode the rest of
 * the value.
 *
 * \param ins The first instruction. Register A will be used as the destination
 * register. The rest of the instruction will be overwritten.
 * \param value The constant value to produce
 *
 * \return void
*/
/* ArmEncodeConstantInTwoInstructions  {{{ */
void ArmEncodeConstantInTwoInstructions(t_arm_ins * ins, t_uint32 value)
{
  t_arm_ins * extra;
  t_uint32 val_invert = ~value;
  int i;
  t_uint32 a;

  extra = ArmInsNewForBbl(ARM_INS_BBL(ins));
  ARM_INS_SET_CSIZE(ins, AddressNew32(4));
  ARM_INS_SET_OLD_SIZE(ins, AddressNew32(0));

  for (i = 0; i < 32; i += 2)
  {
    if (((a = Uint32RotateLeft (value, i)) & 0xff) != 0)
    {
      t_uint32 firstpart,secondpart;

      if (a & 0xff00)
      {
	if (a & ~ 0xffff)
	{
	  continue;
	}
	else
	{
	  /* encode in two succeeding bytes */
	  firstpart = value & Uint32RotateRight(0xff,i);
	  secondpart = value & Uint32RotateRight(0xff00,i);
	}
      }
      else if (a & 0xff0000)
      {
	if (a & 0xff000000)
	{
	  continue;
	}
	else
	{
	  /* encode with one byte interleaved */
	  firstpart = value & Uint32RotateRight(0xff,i);
	  secondpart = value & Uint32RotateRight(0xff0000,i);
	}
      }
      else
      {
	/* encode with two bytes interleaved */
	firstpart = value & Uint32RotateRight(0xff,i);
	secondpart = value & Uint32RotateRight(0xff000000,i);
      }

      /* if we got here, the constant is encodable in two instructions */

      /* first instruction: the low byte */
      ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
      ARM_INS_SET_OPCODE(ins,  ARM_MOV);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(ins,  firstpart);
      ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
      ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
      ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));

      /* second instruction: the high byte */
      ARM_INS_SET_TYPE(extra,  IT_DATAPROC);
      ARM_INS_SET_CONDITION(extra,  ARM_INS_CONDITION(ins));
      ARM_INS_SET_OPCODE(extra,  ARM_ORR);
      ARM_INS_SET_REGA(extra,  ARM_INS_REGA(ins));
      ARM_INS_SET_REGB(extra,  ARM_INS_REGA(ins));
      ARM_INS_SET_REGC(extra,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(extra,  secondpart);
      ARM_INS_SET_REGS(extra,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(extra,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(extra,  0);
      ARM_INS_SET_FLAGS(extra,  0x0 | FL_IMMED);
      ARM_INS_SET_REGS_DEF(extra,  ArmDefinedRegisters(extra));
      ARM_INS_SET_REGS_USE(extra,  ArmUsedRegisters(extra));

      /* insert the extra instruction into the basic block */
      ArmInsInsertAfter(extra,ins);

      return;
    }
  }

  for (i = 0; i < 32; i += 2)
  {
    if (((a = Uint32RotateLeft (val_invert, i)) & 0xff) != 0)
    {
      t_uint32 firstpart,secondpart;

      if (a & 0xff00)
      {
	if (a & ~ 0xffff)
	{
	  continue;
	}
	else
	{
	  /* encode inverted in two succeeding bytes */
	  firstpart = val_invert & Uint32RotateRight(0xff,i);
	  secondpart = val_invert & Uint32RotateRight(0xff00,i);
	}
      }
      else if (a & 0xff0000)
      {
	if (a & 0xff000000)
	{
	  continue;
	}
	else
	{
	  /* encode inverted with one byte interleaved */
	  firstpart = val_invert & Uint32RotateRight(0xff,i);
	  secondpart = val_invert & Uint32RotateRight(0xff0000,i);
	}
      }
      else
      {
	/* encode inverted with two bytes interleaved */
	firstpart = val_invert & Uint32RotateRight(0xff,i);
	secondpart = val_invert & Uint32RotateRight(0xff000000,i);
      }

      /* if we got here, the constant can be encoded in two instructions */

      /* first instruction: the low byte */
      ARM_INS_SET_TYPE(ins,  IT_DATAPROC);
      ARM_INS_SET_OPCODE(ins,  ARM_MVN);
      ARM_INS_SET_REGB(ins,  ARM_REG_NONE);
      ARM_INS_SET_REGC(ins,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(ins,  firstpart);
      ARM_INS_SET_REGS(ins,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(ins,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(ins,  0);
      ARM_INS_SET_FLAGS(ins,  0x0 | FL_IMMED);
      ARM_INS_SET_REGS_DEF(ins,  ArmDefinedRegisters(ins));
      ARM_INS_SET_REGS_USE(ins,  ArmUsedRegisters(ins));

      /* second instruction: the high byte */
      ARM_INS_SET_TYPE(extra,  IT_DATAPROC);
      ARM_INS_SET_CONDITION(extra,  ARM_INS_CONDITION(ins));
      ARM_INS_SET_OPCODE(extra,  ARM_EOR);
      ARM_INS_SET_REGA(extra,  ARM_INS_REGA(ins));
      ARM_INS_SET_REGB(extra,  ARM_INS_REGA(ins));
      ARM_INS_SET_REGC(extra,  ARM_REG_NONE);
      ARM_INS_SET_IMMEDIATE(extra,  secondpart);
      ARM_INS_SET_REGS(extra,  ARM_REG_NONE);
      ARM_INS_SET_SHIFTTYPE(extra,  ARM_SHIFT_TYPE_NONE);
      ARM_INS_SET_SHIFTLENGTH(extra,  0);
      ARM_INS_SET_FLAGS(extra,  0x0 | FL_IMMED);
      ARM_INS_SET_REGS_DEF(extra,  ArmDefinedRegisters(extra));
      ARM_INS_SET_REGS_USE(extra,  ArmUsedRegisters(extra));

      /* insert the extra instruction into the basic block */
      ArmInsInsertAfter(extra,ins);

      return;
    }
  }

  /* if we got here, encoding in two constants was not possible */
  FATAL(("Could not encode constant %x in two instructions!\n",value));
}


/* }}} */

t_arm_ins * ArmGetLastConditionalInsFromBlockWherePropagationSplits(t_bbl * bbl)
{
  t_arm_ins * last = T_ARM_INS(BBL_INS_LAST(bbl));

  if(last)
  {
    if(CFG_DESCRIPTION(BBL_CFG(bbl))->InsIsControlflow(T_INS(last)) && !RegsetIsEmpty(ARM_INS_REGS_DEF(last))
	  && (ARM_INS_ATTRIB(last) & IF_CONDITIONAL) /*&& ARM_INS_TYPE(last)!=IT_BRANCH*/)
    {
      while(ARM_INS_IPREV(last) && (!(ARM_INS_FLAGS(ARM_INS_IPREV(last)) & FL_S)) &&  ARM_INS_CONDITION(ARM_INS_IPREV(last)) == ARM_INS_CONDITION(last))// && !RegsetIsEmpty(ARM_INS_REGS_DEF(last)))
	last = ARM_INS_IPREV(last);
/*      VERBOSE(0,("Splitting propagation at @I",last));*/
      return last;
    }
    else return NULL;
  }
  else return NULL;
}

/** Calculate the registers live after ins, starting from the out
    registers in the block */
t_regset ArmInsRegsLiveAfterConditional(t_arm_ins * ins)
{
  t_arm_ins * i_ins;
  t_arm_ins * last_ins;
  t_cfg_edge * edge, *jump_edge = NULL, *ft_edge = NULL;
  t_bool ignore_condition = TRUE;

  t_bbl * bbl = ARM_INS_BBL(ins);
  t_regset live = RegsetNew();

/*  DiabloPrint(stdout,"Calculating liveness after @I\nin @iB",ins,ARM_INS_BBL(ins));*/

  last_ins = T_ARM_INS(BBL_INS_LAST(bbl));
  /* get outgoing fallthrough and jump edges from the bbl containing the instruction
   * to check
   */
  BBL_FOREACH_SUCC_EDGE(bbl,edge)
  {
    if(CFG_EDGE_CAT(edge) & ET_FT_BLOCK_TRUE) ft_edge = edge;
    else if (CFG_EDGE_CAT(edge) & ET_FT_BLOCK_CALL && CFG_EDGE_CORR(edge)) ft_edge = edge;
    else if(CFG_EDGE_CAT(edge) == ET_JUMP) jump_edge = edge;
    else break;
  }
  /* are the condition flags still the same at the end as at the point where our
   * instruction is?
   */
  i_ins = last_ins;
  while (i_ins != ARM_INS_IPREV(ins))
  {
    if (!RegsetIsMutualExclusive(BBL_DESCRIPTION(bbl)->cond_registers,ARM_INS_REGS_DEF(i_ins)))
    {
      /* they may be different -> be conservative */
      ignore_condition = FALSE;
      break;
    }
    i_ins = ARM_INS_IPREV(ins);
  }

  if(ignore_condition &&
     ARM_INS_CONDITION(last_ins) != ARM_CONDITION_AL &&
     ARM_INS_CONDITION(last_ins) == ARM_INS_CONDITION(ins) &&
     ft_edge && jump_edge)
  {
    t_regset flags;
    /* the last instruction is conditional and has the same condition as the instruction
     * we're checking, and we found a jump and fallthrough edge
     */

    /* if the target bbl is empty it may be an exit bbl, so get the liveout of it
     * (since that may contain function return registers etc), otherwise get the
     * livein of that bbl (registers live before its first instruction)
     */
    if(BBL_NINS(CFG_EDGE_TAIL(jump_edge)))
      RegsetSetDup(live,InsRegsLiveBefore(BBL_INS_FIRST(CFG_EDGE_TAIL(jump_edge))));
    else
      RegsetSetDup(live,BBL_REGS_LIVE_OUT(CFG_EDGE_TAIL(jump_edge)));
/*    DiabloPrintArch(stdout,BBL_OBJECT(bbl)->description,"After @I:\n     using @A \ninstead of @A\n",last_ins,live,BBL_REGS_LIVE_OUT(bbl));*/
  }
  else
  {
    /* our instruction has a different condition as the final instruction and/or
     * we could find all outgoin edges -> be conservative and don't take
     * conditions into account
     */
    ignore_condition = FALSE;
    RegsetSetDup(live,BBL_REGS_LIVE_OUT(bbl));
  }

  last_ins = ArmGetLastConditionalInsFromBlockWherePropagationSplits(bbl);
  if(!last_ins) ignore_condition = FALSE;

#ifdef DEBUG_LIVENESS
  printf("--------------- %d\n",BBL_NINS(bbl));
#endif

  for (i_ins = T_ARM_INS(BBL_INS_LAST(bbl)); ((i_ins!=NULL) && (i_ins != ins)); i_ins = ARM_INS_IPREV(i_ins))
    {
#ifdef DEBUG_LIVENESS
      DiabloPrint(stdout,"@I\n",i_ins);
#endif
      if (!ARM_INS_IS_CONDITIONAL(i_ins) || ignore_condition)
	RegsetSetDiff(live,ARM_INS_REGS_DEF(i_ins));
      RegsetSetUnion(live,ARM_INS_REGS_USE(i_ins));
#ifdef DEBUG_LIVENESS
      DiabloPrintArch(stdout,BBL_OBJECT(bbl)->description,"live: @A\n",live);
#endif
      if(i_ins == last_ins) ignore_condition = FALSE;
    }

  if (!i_ins)
    {
#ifdef DEBUG_LIVENESS
      printf("*************\n");
      InsPrint(ins,0);
      BblPrint(bbl,PRINT_INSTRUCTIONS);
#endif
    }
  if (!i_ins) FATAL(("ins is not in his own basic block!"));

  return live;
}
/*!
 * \todo Document
 *
 * \param i_ins
 * \param copy
 * \param original
 *
 * \return t_bool
 */
/* ArmInsIsCopy {{{ */
t_bool ArmInsIsCopy(t_arm_ins * i_ins, t_reg * copy, t_reg * original)
{
  if (ARM_INS_IS_CONDITIONAL(i_ins)) return FALSE;
  if (ArmInsHasShiftedFlexible(i_ins)) return FALSE;
  if (ARM_INS_OPCODE(i_ins)!=ARM_MOV) return FALSE;
  if (ARM_INS_REGC(i_ins)==ARM_REG_NONE) return FALSE;
  if (ARM_INS_REGC(i_ins)==ARM_REG_R15) return FALSE;
  if (ARM_INS_REGA(i_ins)==ARM_REG_R15) return FALSE;
  if (ARM_INS_REGC(i_ins)==ARM_INS_REGA(i_ins) && (!(ARM_INS_FLAGS(i_ins)&FL_S)))
    {
      ARM_INS_SET_REGA(i_ins, 0);
      ARM_INS_SET_REGC(i_ins, 0);
      ARM_INS_SET_REGS_USE(i_ins, NullRegs);
      ARM_INS_SET_REGS_DEF(i_ins, NullRegs);
      return FALSE;
    }
  *original = ARM_INS_REGC(i_ins); /* Original value */
  *copy = ARM_INS_REGA(i_ins);   /* Copy */
  return TRUE;
}
/* }}} */

/*!
 * \todo Document
 *
 * \param bbl
 *
 * \return t_bool
 */
/* ArmBblInSwitchStatement{{{ */
t_bool
ArmBblInSwitchStatement(t_bbl *bbl, t_bool only_check_default_case)
{
  t_bbl *prev;
  t_ins *last;

  /* the dispatch block of a thumb compact switch table must not be split
   * (it contains multiple pc-relative instructions)
   */

  if (IS_SWITCH_TABLE(bbl))
    return TRUE;

  last = BBL_INS_LAST (bbl);

  if (T_ARM_INS(last) && ((ARM_INS_OPCODE(T_ARM_INS(last)) == ARM_T2TBB) ||
      (ARM_INS_OPCODE(T_ARM_INS(last)) == ARM_T2TBH)))
    return TRUE;

  if (T_ARM_INS(last) &&
      (ARM_INS_FLAGS (T_ARM_INS(last)) & FL_THUMB) &&
      (ARM_INS_ATTRIB (T_ARM_INS(last)) & IF_SWITCHJUMP))
    return TRUE;

  /* now detect both the dispatch of an add-based switch and unconditional
   * branches that may be inside switch blocks of such an add-based switch
   * (e.g. the fallthrough block of a conditional add-based switch)
   */

  /* if this may be a switch block, find the dispatch block, otherwise
   * last is already set correctly to the last instruction of the
   * switch dispatch block
   */
  if (!last ||
      !(ARM_INS_ATTRIB (T_ARM_INS(last)) & IF_SWITCHJUMP))
  {
    t_cfg_edge *switch_edge, *ft_edge;
    /* we only have to ensure that we don't insert pools in add-based jumptables.
     * the variants we currently support all have one instruction per target bbl
     * -> if this bbl has more than instruction it's definitely not part of an
     * add-based switch table. The bbl can also have 0 instructions in case
     * the nop has been killed
     */
    if (BBL_NINS (bbl) > 1)
      return FALSE;
    switch_edge = NULL;
    ft_edge = NULL;
    /* find the incoming switch table edge (or fallthrough from dispatch block) */
    BBL_FOREACH_PRED_EDGE (bbl,switch_edge)
    {
      if ((!only_check_default_case) && CFG_EDGE_CAT (switch_edge) & (ET_SWITCH|ET_IPSWITCH))
        break;
      else if (CFG_EDGE_CAT (switch_edge) & (ET_FALLTHROUGH|ET_IPFALLTHRU))
        ft_edge = switch_edge;
    }
    if (!switch_edge)
      switch_edge = ft_edge;
    if (!switch_edge)
      return FALSE;
    prev = CFG_EDGE_HEAD (switch_edge);
    if (!prev)// || !BBL_NEXT_IN_CHAIN (bbl))
      return FALSE;
    last = BBL_INS_LAST (prev);
  }
  if (!T_ARM_INS(last)) return FALSE;
  if (ARM_INS_OPCODE(T_ARM_INS(last))==ARM_MOV && (ARM_INS_ATTRIB(T_ARM_INS(last)) & IF_SWITCHJUMP) && ARM_INS_REGA(T_ARM_INS(last))==ARM_REG_R15 && ARM_INS_FLAGS(T_ARM_INS(last)) & FL_THUMB)
    return TRUE;
  if (ARM_INS_REGA (T_ARM_INS(last)) != ARM_REG_R15)
    return FALSE;
  if (ARM_INS_OPCODE (T_ARM_INS(last)) != ARM_ADD && ARM_INS_OPCODE (T_ARM_INS(last)) != ARM_LDR)
    return FALSE;
  if (ARM_INS_REGB (T_ARM_INS(last)) != ARM_REG_R15)
    return FALSE;
  return TRUE;
}
/*}}}*/

void ArmForwardToReturnDo(t_bbl * bbl, t_bool * do_forwarding)
{
  *do_forwarding = !ArmBblInSwitchStatement(bbl,TRUE);
}

/*!
 * \todo Document
 *
 * \param bbl
 *
 * \return t_bool
 */
/* ArmBblCanSplitAfter{{{ */
t_bool
ArmBblCanInsertAfter(t_bbl *bbl)
{
  t_cfg_edge *edge;

  BBL_FOREACH_SUCC_EDGE (bbl, edge)
  {
    if (!(CFG_EDGE_CAT (edge) & (ET_JUMP | ET_IPJUMP)) &&
        !BblIsExitBlock (CFG_EDGE_TAIL (edge)) &&
        !(CFG_EDGE_CAT (edge) == ET_CALL && !CFG_EDGE_CORR (edge)))
      break;
  }
  if (!edge && !ArmBblInSwitchStatement(bbl,FALSE))
    return TRUE;
  return FALSE;
}
/*}}}*/
/*!
 * \todo Document
 *
 * \param fun
 *
 * \return t_bool
 */
/* ArmFunIsGlobal {{{ */
t_bool ArmFunIsGlobal(t_function * fun)/*{{{*/
{
  if (FUNCTION_FLAGS(fun) & FF_IS_EXPORTED)
    {
      if(FunctionBehaves(fun))
	return TRUE;
      else return FALSE;
    }
  return FALSE;
}
/*}}}*/


t_uint32
ArmInsModus(t_arm_ins * ins, t_uint32 prev_modus)
{
  if ((ARM_INS_FLAGS(T_ARM_INS(ins)) & FL_THUMB) &&
      (ARM_INS_OPCODE(T_ARM_INS(ins)) != ARM_DATA))
  {
    if (prev_modus == 0) FATAL(("Complicated modus calculation. Implement!"));
    return 1;
  }
  else
  {
    if (prev_modus == 1) FATAL(("Complicated modus calculation. Implement!"));
    return 0;
  }
}



t_address
ArmModus(t_address ina, t_reloc * rel)
{
  t_uint32 i;
  t_uint32 modus = UINT32_MAX;
  t_address ina_even = AddressAnd(ina, ~0x1);

  /* Look for ins or bbl */
  for (i=0; i<RELOC_N_TO_RELOCATABLES(rel); i++)
  {
    if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_BBL)
    {
      t_ins * ins;

      /* Look for the instruction with address insa */
      BBL_FOREACH_INS(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]), ins)
      {
	if ((AddressIsGe(ina_even, INS_CADDRESS(ins)))
	&& (AddressIsLt(ina_even, AddressAdd(INS_CADDRESS(ins), INS_CSIZE(ins)))))
	{
	  modus = ArmInsModus(T_ARM_INS(ins), modus);
	  break;
	}
      }

      if (modus == UINT32_MAX)
      {

	if ((BBL_INS_LAST(T_BBL(RELOC_TO_RELOCATABLE(rel)[i])))&&(AddressIsEq(ina_even, AddressAdd(INS_CADDRESS(BBL_INS_LAST((T_BBL(RELOC_TO_RELOCATABLE(rel)[i])))),INS_CSIZE(BBL_INS_LAST((T_BBL(RELOC_TO_RELOCATABLE(rel)[i]))))))))
	{
	  ins = BBL_INS_LAST((T_BBL(RELOC_TO_RELOCATABLE(rel)[i])));
	  modus = ArmInsModus(T_ARM_INS(ins), modus);
	  break;
	}
	else if ((!BBL_INS_LAST(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]))) && AddressIsEq(ina_even, BBL_CADDRESS(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]))))
	{
          t_cfg_edge * edge;
          t_bbl * bbl = T_BBL(RELOC_TO_RELOCATABLE(rel)[i]);
          t_int32 teller = 0;

          BBL_FOREACH_SUCC_EDGE(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]), edge)
            teller++;
          ASSERT(teller<=1,("NR OF SUCC EDGES OF EMPTY BBL @B IS ASSUMED TO BE AT MOST ONE",bbl));

          while ((edge = BBL_SUCC_FIRST(bbl))!=NULL)
          {
            if (BBL_NINS(CFG_EDGE_TAIL(edge))!=0)
            {
              modus = ArmInsModus(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge))), modus);
              break;
            }
            else
              bbl = CFG_EDGE_TAIL(edge);
          }
          /*
	  t_cfg_edge * edge;
	  BBL_FOREACH_SUCC_EDGE(T_BBL(RELOC_TO_RELOCATABLE(rel)[i]), edge)
	  {
	    if (BBL_NINS(CFG_EDGE_TAIL(edge))!=0)
	    {
	      modus = ArmInsModus(T_ARM_INS(BBL_INS_FIRST(CFG_EDGE_TAIL(edge))), modus);
	      break;
	    }
	  }
          */
 	}
      }


    }
    else if (RELOCATABLE_RELOCATABLE_TYPE(RELOC_TO_RELOCATABLE(rel)[i]) == RT_INS)
    {
      if (AddressIsEq(ina_even, RELOCATABLE_CADDRESS(RELOC_TO_RELOCATABLE(rel)[i])))
        modus = ArmInsModus(T_ARM_INS(RELOC_TO_RELOCATABLE(rel)[i]), modus);
    }
  }


  if (modus == UINT32_MAX)
  {
    VERBOSE(0, ("Modus @R @G: %d", rel, ina, modus));
    FATAL(("Oeps!"));
  }
  return AddressNew32(modus);
}
/*}}}*/

/*!
 * Returns a regset containing the THUMB(1) general purpose registers
 *
 * \return t_regset
 */
/* ArmGetThumbRegset {{{ */
t_regset ArmGetThumbRegset(void)
{
  int i;
  t_regset res;

  res = RegsetNew();
  for (i = ARM_REG_R0; i < ARM_REG_R8; i++)
  {
    RegsetSetAddReg(res, i);
  }
  return res;
}

/*}}}*/
/*
 */
/*ArmAdjustRegReadsBetweenByOffset{{{*/
t_bool ArmAdjustRegReadsBetweenByOffset(t_reg reg, t_arm_ins *begin, t_arm_ins *end, int stack_offset, t_bool perform_modifications)
{
  t_arm_ins *runner = begin;
  do
  {
    /* if the register is written but not read, then everything is fine since
     * the old value no longer matters
     */
    if (RegsetIn(ARM_INS_REGS_DEF(runner),reg) &&
	!RegsetIn(ARM_INS_REGS_USE(runner),reg))
      return TRUE;
    switch (ARM_INS_TYPE(runner))
    {
      case IT_SWI:
	/* don't know what a syscall will use -> fail */
	return FALSE;
      case IT_STORE:
	{
	  /* if the test register is not used at all, it's ok */
	  if (!RegsetIn(ARM_INS_REGS_USE(runner),reg))
	    break;

	  /* if it is used, then check whether its value is stored or
	   * whether there's an index reg (bad),  or whether it is the
	   * base address register for str with offset (possibly good)
	   */
	  if ((ARM_INS_REGA(runner) == reg) ||
	      (ARM_INS_REGC(runner) != ARM_REG_NONE))
	    return FALSE;

	  /* there's a relocation -> fail */
          if (ARM_INS_REFERS_TO(runner))
	    return FALSE;

	  /* try to modify the offset */
	  if (!(ARM_INS_FLAGS(runner) & FL_THUMB))
	  {
	    if ((ARM_INS_IMMEDIATE(runner)+stack_offset) > 4095)
	      return FALSE;
	  }
	  else
	  {
            t_bool ok;

	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)+stack_offset);
	    if (diabloarm_options.fullthumb2)
	      ok = ArmIsThumb2Encodable(runner);
	    else
	      ok = ArmIsThumb1Encodable(runner);
	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)-stack_offset);
	    if (!ok)
	      return FALSE;
	  }

	  /* it is used as base or index for str -> check whether we can adjust the
	   * offset
	   */
	  if ((ARM_INS_IMMEDIATE(runner)+stack_offset) > 4095)
	    return FALSE;

	  /* ok -> modify the offset */
	  if (perform_modifications)
	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)+stack_offset);
          break;
	}
      case IT_LOAD:
	{
	  /* if the test register is not used at all, it's ok */
	  if (!RegsetIn(ARM_INS_REGS_USE(runner),reg))
	    break;

	  /* if test register is used as base and there's no index,
	   * we may be able to modify the offset
	   */
	  if (ARM_INS_REGC(runner) != ARM_REG_NONE)
	    return FALSE;

	  /* the base register should now be the tested register */
          ASSERT(reg == ARM_INS_REGB(runner),("reg %d used by instruction, but not base? @i",reg,runner));

	  /* there's a relocation -> fail */
          if (ARM_INS_REFERS_TO(runner))
	    return FALSE;

	  /* try to modify the offset */
	  if (!(ARM_INS_FLAGS(runner) & FL_THUMB))
	  {
	    if ((ARM_INS_IMMEDIATE(runner)+stack_offset) > 4095)
	      return FALSE;
	  }
	  else
	  {
            t_bool ok;

	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)+stack_offset);
	    if (diabloarm_options.fullthumb2)
	      ok = ArmIsThumb2Encodable(runner);
	    else
	      ok = ArmIsThumb1Encodable(runner);
	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)-stack_offset);
	    if (!ok)
	      return FALSE;
	  }
	  /* ok -> modify the offset */
	  if (perform_modifications)
	    ARM_INS_SET_IMMEDIATE(runner,ARM_INS_IMMEDIATE(runner)+stack_offset);

	  /* if the test register is overwritten, stop (new value unrelated to
	   * old one)
	   */
	  if (ARM_INS_REGA(runner) == reg)
	    return TRUE;
          break;
	}
      default:
	/* the register is read and/or written and not handled specifically
	 * -> fail
	 */
	if (RegsetIn(ARM_INS_REGS_USE(runner),reg))
	  return FALSE;
        break;
    }
    runner = ARM_INS_INEXT(runner);
  } while (runner != ARM_INS_INEXT(end));
  return TRUE;
}
/*}}}*/

t_arm_ins * ArmFindFirstIns(t_bbl *bbl)
{
  t_cfg_edge * edge = NULL;

  if (BBL_INS_FIRST(bbl))
    return T_ARM_INS(BBL_INS_FIRST(bbl));

  BBL_FOREACH_SUCC_EDGE(bbl, edge)
  {
    if (FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))
      return NULL;

    return ArmFindFirstIns(CFG_EDGE_TAIL(edge));
  }

  return NULL;
}

/*!
 * Returns whether a bbl is part of thumb code (even if it is empty)
 *
 */
/*ArmBblIsThumb{{{*/
t_tristate ArmBblIsThumbDirectional(t_bbl *bbl, t_bool forward)
{
  t_cfg_edge *edge;

  if (BBL_INS_FIRST(bbl))
    return (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(bbl))) & FL_THUMB) != 0;

  if (!forward)
  {
    BBL_FOREACH_PRED_EDGE(bbl,edge)
    {
      t_tristate res;

      /* call/return can switch thumb mode */
      if (CFG_EDGE_CAT(edge) & (ET_CALL|ET_RETURN|ET_COMPENSATING))
        continue;
      if (FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_HEAD(edge))))
        continue;
      res = ArmBblIsThumbDirectional(CFG_EDGE_HEAD(edge),FALSE);
      if (res != PERHAPS)
        return res;
    }
  }
  else
  {
    BBL_FOREACH_SUCC_EDGE(bbl,edge)
    {
      t_tristate res;

      /* call/return can switch thumb mode */
      if (CFG_EDGE_CAT(edge) & (ET_CALL|ET_RETURN|ET_COMPENSATING))
        continue;
      if (FUNCTION_IS_HELL(BBL_FUNCTION(CFG_EDGE_TAIL(edge))))
        continue;
      res = ArmBblIsThumbDirectional(CFG_EDGE_TAIL(edge),TRUE);
      if (res != PERHAPS)
        return res;
    }
  }
  return PERHAPS;
}

t_tristate ArmBblIsPerhapsThumb(t_bbl *bbl)
{
  t_tristate res;
  res = ArmBblIsThumbDirectional(bbl,TRUE);
  if (res != PERHAPS)
    return res;
  res = ArmBblIsThumbDirectional(bbl,FALSE);

  return res;
}

t_bool ArmBblIsThumb(t_bbl *bbl)
{
  t_tristate res = ArmBblIsPerhapsThumb(bbl);

  /* be conservative */
  res = (res == PERHAPS) ? FALSE : res;

  return res;
}


/*}}}*/
/*!
 * Returns wheter an immediate can be encoded as an ARM imm12 operand
 *
 * \param val constant to check
 *
 * \return t_bool
 */
/*ArmConstIsImm12Encodable{{{*/
t_bool ArmConstIsImm12Encodable(int val)
{
  int i;

  for (i = 0; i < 32; i += 2)
    if (Uint32RotateLeft (val, i) <= 0xff)
    {
      return TRUE;
    }
  return FALSE;
}
/*}}}*/

/*!
 * Returns wheter an immediate can be encoded as an ARM imm12 operand
 *
 * \param val constant to check
 *
 * \return t_bool
 */
/*ArmConstCanBeImmediateOperand{{{*/
t_bool ArmConstCanBeImmediateOperand(int val, t_bool thumb)
{
  if (thumb)
    return (val >= 0) &&  (val <= 255);
  else
    return ArmConstIsImm12Encodable(val);
}
/*}}}*/
/*!
 * Creates room for a new bbl at the start of an existing bbl in a chain. Afterwards, the argument bbl will contain the branch jumping over the hole, which itself will follow that bbl
 *
 * \param bbl the existing bbl
 *
 * \return void
 */
/*ArmChainCreateHoleBefore{{{*/
void ArmChainCreateHoleBefore(t_bbl *bbl)
{
  t_bbl *rest;
  t_cfg_edge *edge;
  t_arm_ins *branch;
  t_cfg *cfg;
  t_bool has_corr, is_interproc;

  cfg = BBL_CFG(bbl);
  has_corr = FALSE;
  rest = BblSplitBlock (bbl, BBL_INS_FIRST(bbl), TRUE);

  edge = BBL_SUCC_FIRST (bbl);
  is_interproc = CFG_EDGE_CAT (edge) == ET_IPFALLTHRU;
  if (CFG_EDGE_CORR (edge))
  {
    CfgEdgeKill (CFG_EDGE_CORR (edge));
    has_corr = TRUE;
  }
  CfgEdgeKill (edge);
  edge = CfgEdgeCreate (cfg, bbl, rest, is_interproc?ET_IPJUMP:ET_JUMP);
  if (has_corr)
    CfgEdgeCreateCompensating (cfg, edge);
  branch = T_ARM_INS(InsNewForBbl (bbl));
  if (ARM_INS_FLAGS(T_ARM_INS(BBL_INS_FIRST(rest))) & FL_THUMB)
  {
    ARM_INS_SET_FLAGS(branch, ARM_INS_FLAGS(branch) |  FL_THUMB);
    ARM_INS_SET_CSIZE(branch, AddressNew32(2));
  }
  ArmInsAppendToBbl (branch, bbl);
  ArmInsMakeUncondBranch (branch);
}
/*}}}*/
/*!
 * Returns wheter an instruction can be encoded in thumb1
 *
 * \param ins
 *
 * \return t_regset
 */
/* ArmIsThumb1Encodable {{{ */
#undef IS_THUMB_REG
#define IS_THUMB_REG(x) (((x) >= ARM_REG_R0) && ((x) <= ARM_REG_R7))
t_bool
ArmIsThumb1Encodable(t_arm_ins *ins)
{
  return ArmIsThumb1EncodableCheck(ins, TRUE);
}

t_bool
ArmIsThumb1EncodableCheck(t_arm_ins *ins, t_bool check_condition)
{
        return ArmIsThumb1EncodableCheckItNoIt(ins, check_condition, FALSE);
}

t_bool
ArmIsThumb1EncodableCheckItNoIt(t_arm_ins *ins, t_bool check_condition, t_bool it_generated)
{
  t_bool ret;

  t_bool last_in_it, in_it;
  t_arm_ins * owning_it = NULL;

  /* validate the condition code */
  if (check_condition && !ArmInsIsValidThumbConditionCode(ins, &last_in_it, &owning_it))
  {
    VERBOSE(1, ("Instruction @I not encodable in Thumb1 due to an invalid condition code.", ins));
    return FALSE;
  }
  in_it = (owning_it != NULL);

  if ((ARM_INS_CONDITION(ins) != ARM_CONDITION_AL) && !ArmInsIsValidInITBlock(ins)
        && /* conditional branches are allowed */ (ARM_INS_OPCODE(ins)!=ARM_B))
        return FALSE;

  if ((ARM_INS_OPCODE(ins) == TH_BX_R15))
    return TRUE;

  if ((ARM_INS_OPCODE(ins) == ARM_T2CBZ) ||
      (ARM_INS_OPCODE(ins) == ARM_T2CBNZ))
    return TRUE;

  if ((ARM_INS_OPCODE(ins) == ARM_T2NOP) ||
      (ARM_INS_OPCODE(ins) == ARM_T2IT))
    return TRUE;

  /* in thumb1 only MOV supports a limited set of shifters */
  if ((ARM_INS_OPCODE(ins) != ARM_MOV) &&
      (ARM_INS_OPCODE(ins) != ARM_DATA) &&
      (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE))
    return FALSE;

  {
    t_bool does_not_set_flags = ArmInsThumbITDoesNotSetFlags(ins, it_generated);

    switch (ARM_INS_OPCODE(ins))
  {
    //TODO-JENS: MRS/MSR is not supported anymore in Thumb
    /*case ARM_MRS:
    case ARM_MSR:
      ret = !(ARM_INS_FLAGS(ins) & FL_IMMED);
      break;*/
    case ARM_AND:
    case ARM_EOR:
    case ARM_ADC:
    case ARM_SBC:
    case ARM_ORR:
    case ARM_BIC:
    case ARM_MUL:
    case ARM_UDIV:
      /* TODO: for commutative ops, REGC==REGA would also be ok
       * (but then either switch the regs, or adjust the thumb
       *  assembler so it can deal with switched regs)
       */
      ret =
	!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
    ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	(ARM_INS_REGB(ins) == ARM_INS_REGA(ins)) &&
	IS_THUMB_REG(ARM_INS_REGC(ins));
      break;
    case ARM_TST:
    case ARM_CMN:
      ret =
	!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	(ARM_INS_FLAGS(ins) & FL_S) &&
        (ARM_INS_REGA(ins) == ARM_REG_NONE) &&
	IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	IS_THUMB_REG(ARM_INS_REGC(ins));
      break;
    case ARM_RSB:
      ret =
        ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
        (ARM_INS_FLAGS(ins) & FL_IMMED) &&   
        (ARM_INS_IMMEDIATE(ins) == 0) &&
	IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	IS_THUMB_REG(ARM_INS_REGB(ins)) &&
        (ARM_INS_REGC(ins) == ARM_REG_NONE);
      break;
    case ARM_MVN:
      ret =
	!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
    ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	IS_THUMB_REG(ARM_INS_REGA(ins)) &&
        (ARM_INS_REGB(ins) == ARM_REG_NONE) &&
	IS_THUMB_REG(ARM_INS_REGC(ins));
      break;
    case ARM_REV:
    case ARM_REV16:
    case ARM_REVSH:
      ret =
	!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	!(ARM_INS_FLAGS(ins) & FL_S) &&
	IS_THUMB_REG(ARM_INS_REGA(ins)) &&
        (ARM_INS_REGB(ins) == ARM_REG_NONE) &&
	IS_THUMB_REG(ARM_INS_REGC(ins));
      break;
    case ARM_UXTB:
    case ARM_UXTH:
    case ARM_SXTB:
    case ARM_SXTH:
      ret =
        !(ARM_INS_FLAGS(ins) & FL_S) &&
        IS_THUMB_REG(ARM_INS_REGA(ins)) &&
        IS_THUMB_REG(ARM_INS_REGC(ins)) &&
        ARM_INS_SHIFTLENGTH(ins)==0;
      break;
    case ARM_ADD:
    case ARM_SUB:
      /* add/sub trX, trY, trZ */
      if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGC(ins)))
	ret = TRUE;
	  /* add/sub trX, trY, imm3 */
      else if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~7) == 0))
	ret = TRUE;
	  /* add/sub trX, trX, imm8 */
      else if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  (ARM_INS_REGB(ins) == ARM_INS_REGA(ins)) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~255) == 0))
	ret = TRUE;
	  /* add rX, rX, rY */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
      (!(ARM_INS_FLAGS(ins) & FL_S)) &&
	  (ARM_INS_OPCODE(ins) == ARM_ADD) &&
	  (ARM_INS_REGB(ins) == ARM_INS_REGA(ins)))
	ret = TRUE;
	  /* add/sub r13, r13, imm7 */
      else if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
      (!(ARM_INS_FLAGS(ins) & FL_S)) &&
	  (ARM_INS_REGA(ins) == ARM_REG_R13) &&
	  (ARM_INS_REGB(ins) == ARM_REG_R13) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~(127 << 2)) == 0))
	ret = TRUE;
	  /* add trX, r13/r15, imm8:00 */
      else if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
      (!(ARM_INS_FLAGS(ins) & FL_S)) &&
          (ARM_INS_OPCODE(ins) == ARM_ADD) &&
	  (IS_THUMB_REG(ARM_INS_REGA(ins))) &&
	  ((ARM_INS_REGB(ins) == ARM_REG_R13) ||
	   (ARM_INS_REGB(ins) == ARM_REG_R15)) &&
	  ((ARM_INS_IMMEDIATE(ins) & 3) == 0) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~(255 << 2)) == 0))
	ret = TRUE;
      else
        ret = FALSE;
      break;
    case ARM_MOV:
      /* mov trX, trX, LSL/LSR/ASR/ROR reg (updating flags) */
      if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  ((ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_REG) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_REG) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_REG) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ROR_REG)) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGS(ins)) &&
	  (ARM_INS_REGA(ins) == ARM_INS_REGC(ins)) &&
	  (ARM_INS_REGB(ins) == ARM_REG_NONE))
	  ret = TRUE;
      /* mov trX, trY, LSL/LSR/ASR imm31 (updating flags) */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  ((ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSL_IMM) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_LSR_IMM) ||
	   (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_ASR_IMM)) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGC(ins)))
	ret = TRUE;
      /* mov trX, imm8 (updating flags) */
      else if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  (IS_THUMB_REG(ARM_INS_REGA(ins))) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~255) == 0))
         ret = TRUE;
      /* mov trX, trY (updating flags) */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
      ((ARM_INS_FLAGS(ins) & FL_S) || does_not_set_flags) &&
	  (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE) &&
	  IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	  IS_THUMB_REG(ARM_INS_REGB(ins)))
	ret = TRUE;
      /* mov rX, rY (NOT updating flags) */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
      (!(ARM_INS_FLAGS(ins) & FL_S)) &&
	  (ARM_INS_SHIFTTYPE(ins) == ARM_SHIFT_TYPE_NONE)/* &&
	  (!IS_THUMB_REG(ARM_INS_REGA(ins)) ||
	   !IS_THUMB_REG(ARM_INS_REGB(ins)))*/)
	ret = TRUE;
      else
	ret = FALSE;
      break;
    case ARM_CMP:
      /* cmp trX, imm8 (updating flags) */
      if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  (ARM_INS_FLAGS(ins) & FL_S) &&
	  (IS_THUMB_REG(ARM_INS_REGB(ins))) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~255) == 0))
	ret = TRUE;
      /* cmp rX, rY (updating flags) */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  (ARM_INS_FLAGS(ins) & FL_S))
	ret = TRUE;
      else
	ret = FALSE;
      break;
    case ARM_SWI:
    case ARM_BKPT:
      ret =
	((ARM_INS_IMMEDIATE(ins) & ~255) == 0);
      break;
    case ARM_STRB:
    case ARM_STRH:
    case ARM_STR:
    case ARM_LDRB:
    case ARM_LDRSB:
    case ARM_LDRH:
    case ARM_LDRSH:
    case ARM_LDR:
      {
	t_uint32 offset_shift;

        if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
                return FALSE;

	switch (ARM_INS_OPCODE(ins))
	{
	  case ARM_STRB:
	  case ARM_LDRB:
          case ARM_LDRSB:
	    offset_shift = 0;
	    break;
	  case ARM_STRH:
	  case ARM_LDRH:
	  case ARM_LDRSH:
	    offset_shift = 1;
	    break;
	  case ARM_STR:
	  case ARM_LDR:
	    offset_shift = 2;
	    break;
	  default:
	    /* shut up the compiler */
	    offset_shift = -1;
	    break;
	}
  if (((ARM_INS_OPCODE(ins) == ARM_LDRSH) || (ARM_INS_OPCODE(ins) == ARM_LDRSB)) && (ARM_INS_FLAGS(ins) & FL_IMMED))
    return FALSE;

	/* LDR* / STR* trX, [trY, imm5:log2(transfer_size)] */
	if (((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_IMMED)) == (FL_PREINDEX | FL_DIRUP | FL_IMMED)) &&
	    IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	    IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	    ((ARM_INS_IMMEDIATE(ins) & ~(31 << offset_shift)) == 0))
	  ret = TRUE;
	    /* LDR* / STR* trX, [trY, trZ] */
	else if (((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_IMMED)) == (FL_PREINDEX | FL_DIRUP)) &&
	    IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	    IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	    IS_THUMB_REG(ARM_INS_REGC(ins)))
	  ret = TRUE;
	    /* LDR trX, [r13/r15, imm8:00]
	     * STR trX, [r13, imm8:00]
	     */
	else if (((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_IMMED)) == (FL_PREINDEX | FL_DIRUP | FL_IMMED)) &&
	    (((ARM_INS_OPCODE(ins) == ARM_LDR) &&
	      ((ARM_INS_REGB(ins) == ARM_REG_R13) ||
	       (ARM_INS_REGB(ins) == ARM_REG_R15))) ||
	     ((ARM_INS_OPCODE(ins) == ARM_STR) &&
	      (ARM_INS_REGB(ins) == ARM_REG_R13))) &&
	    IS_THUMB_REG(ARM_INS_REGA(ins)) &&
	    ((ARM_INS_IMMEDIATE(ins) & ~(255 << 2)) == 0))
	  ret = TRUE;
	else
	  ret = FALSE;
      }
      break;
    case ARM_LDM:
    case ARM_STM:
      /* STMIA/LDMIA trX,[trY, .., trZ], with writeback except if LDMIA and
       * trX in [trY, .., trZ]
       */
      if (((ARM_INS_FLAGS(ins) & (FL_DIRUP | FL_PREINDEX | FL_IMMED)) == FL_DIRUP) &&
	  IS_THUMB_REG(ARM_INS_REGB(ins)) &&
	  (ARM_INS_REGC(ins) == ARM_REG_NONE) &&
	  ((ARM_INS_IMMEDIATE(ins) & ~0xff) == 0) &&
          (((ARM_INS_FLAGS(ins) & FL_WRITEBACK) != 0) ==
	   !((ARM_INS_OPCODE(ins) == ARM_LDM) &&
             (ARM_INS_IMMEDIATE(ins) & (1 << ARM_INS_REGB(ins))))))
	ret = TRUE;
	  /* push {trX, .., trY, [r14]
	   * pop  {trX, .., trY, [r15]
	   */
      else if (((ARM_INS_FLAGS(ins) & (FL_WRITEBACK | FL_IMMED)) == FL_WRITEBACK) &&
	  (((ARM_INS_OPCODE(ins) == ARM_STM) &&
	    ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) == FL_PREINDEX) &&
	    ((ARM_INS_IMMEDIATE(ins) & ~(0xff | (1 << 14))) == 0)) ||
	   ((ARM_INS_OPCODE(ins) == ARM_LDM) &&
	    ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) == FL_DIRUP) &&
	    ((ARM_INS_IMMEDIATE(ins) & ~(0xff | (1 << 15))) == 0))) &&
	  (ARM_INS_REGB(ins) == ARM_REG_R13) &&
	  (ARM_INS_REGC(ins) == ARM_REG_NONE))
	ret = TRUE;
      else
	ret = FALSE;
      break;
    case ARM_B:
    case ARM_BX:
    case ARM_BL:
       // 16-bit BL(X) (immediate) is not defined anymore in Thumb2
       // TODO-JENS: AppxL-2502
    case ARM_BLX:
      /* B<cond>   simm8:0
       * B/BL/BLX  simm11:0
       */
      if ((ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  ((ARM_INS_OPCODE(ins) == ARM_B)/* ||
	   (ARM_INS_OPCODE(ins) == ARM_BL) ||
	   (ARM_INS_OPCODE(ins) == ARM_BLX)*/) &&
	  ((ARM_INS_IMMEDIATE(ins) & 1) == 0) &&
	  (((ARM_INS_CONDITION(ins) != ARM_CONDITION_AL) &&
	    (ARM_INS_IMMEDIATE(ins) >= (t_int64)-256) &&
	    (ARM_INS_IMMEDIATE(ins) < (t_int64)255)) ||
	   ((ARM_INS_CONDITION(ins) == ARM_CONDITION_AL) &&
            (ARM_INS_OPCODE(ins) == ARM_B) &&
	    (ARM_INS_IMMEDIATE(ins) >= (t_int64)-2048) &&
	    (ARM_INS_IMMEDIATE(ins) < (t_int64)2047)) ||
	   ((ARM_INS_CONDITION(ins) == ARM_CONDITION_AL) &&
            (ARM_INS_OPCODE(ins) != ARM_B) &&
	    (ARM_INS_IMMEDIATE(ins) >= -(1 << 22)) &&
	    (ARM_INS_IMMEDIATE(ins) < (1 << 22)-1))) &&
	  (ARM_INS_REGA(ins) == ARM_REG_NONE) &&
	  (ARM_INS_REGB(ins) == ARM_REG_NONE) &&
	  (ARM_INS_REGC(ins) == ARM_REG_NONE))
	ret = TRUE;
      /* BX/BLX rX */
      else if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
	  ((ARM_INS_OPCODE(ins) == ARM_BX) ||
	   (ARM_INS_OPCODE(ins) == ARM_BLX)) &&
	  (ARM_INS_REGA(ins) == ARM_REG_NONE) &&
	  (ARM_INS_REGB(ins) != ARM_REG_NONE) &&
	  (ARM_INS_REGC(ins) == ARM_REG_NONE))
	ret = TRUE;
      else
	ret = FALSE;
      break;
    case ARM_DATA:
      ret = (AddressExtractUint32(ARM_INS_CSIZE(ins)) == 2);
      break;
    default:
      ret = FALSE;
      break;
  }
  }
  return ret;
  }
#undef IS_THUMB_REG
/*}}}*/

/*!
 * Returns wheter an instruction can be encoded in thumb1 or thumb2
 *
 * \param ins
 *
 * \return t_regset
 */
/* ArmIsThumb2Encodable {{{ */
#undef IS_THUMB_REG
#define IS_THUMB_REG(x) (((x) >= ARM_REG_R0) && ((x) <= ARM_REG_R7))

t_bool ArmIsThumb2ImmediateEncodable(t_uint32 imm)
{
  t_uint32 b = imm & 0x000000ff;
  t_uint32 shift;

  if ((imm & ~0x000000ff) == 0)
    return TRUE;

  if (((imm & ~0x00ff00ff) == 0) &&
      (((imm & 0x00ff0000) >> 16) == (imm & 0x000000ff)))
    return TRUE;

  if (((imm & ~0xff00ff00) == 0) &&
      (((imm & 0xff000000) >> 16) == (imm & 0x0000ff00)))
    return TRUE;

  /* 4 equal bytes */
  if (imm == (b | (b << 8) | (b << 16) | (b << 24)))
    return TRUE;

  for (shift = 0; shift < 24; shift++)
  {
    /* try every possible shift */
    if ((imm & 0x80000000) &&
        ((imm & ~0xff000000) == 0))
      return TRUE;

    /* watch out that we don't shift out set bits! */
    if (!(imm & 0x80000000))
      imm <<= 1;
    else
      break;
  }

  return FALSE;
}

t_bool
ArmIsVectorEncodable(t_arm_ins *ins)
{
  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_FSTMX:
  case ARM_FLDMX:
  case ARM_VLDM:
  case ARM_VSTM:
    return TRUE;
    break;

  default:
    //FATAL(("unchecked vector instruction @I", ins));
    break;
  }

  return TRUE;
}

t_bool
ArmIsThumb2Encodable(t_arm_ins *ins)
{
  return ArmIsThumb2EncodableCheck(ins, TRUE);
}

t_bool
ArmIsThumb2EncodableCheck(t_arm_ins *ins, t_bool check_condition)
{
  t_bool last_in_it, in_it;
  t_arm_ins * owning_it = NULL;

  if (ArmIsThumb1EncodableCheck(ins, check_condition))
    return TRUE;

  /* Thumb2 specific instructions */
  if ((ARM_T2_FIRST <= ARM_INS_OPCODE(ins)) && (ARM_INS_OPCODE(ins) <= ARM_T2_LAST))
    return TRUE;

  /* validate the condition code */
  if (check_condition && !ArmInsIsValidThumbConditionCode(ins, &last_in_it, &owning_it))
  {
    VERBOSE(1, ("Instruction @I not encodable in Thumb2 due to an invalid condition code.", ins));
    return FALSE;
  }
  in_it = (owning_it != NULL);

  /* for now only test for supported instructions; will be generalised later
   */
  switch (ARM_INS_OPCODE(ins))
  {
    case ARM_ADC:
    case ARM_BIC:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
          return FALSE;
      }
      else
      {
        /* register */
        if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15) ||
            (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
          return FALSE;
      }

      return TRUE;
      break;

    case ARM_ADD:
    case ARM_SUB:
    case ARM_RSB:
    case ARM_SBC:
      /* caveat: ADR is translated to ADD/SUB, depending on the address.
                 So, here we should also allow REGB to be 15. */
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */

        /* first try to encode a 12-bit literal imemdiate */
        if (((ARM_INS_IMMEDIATE(ins) & ~0xfff) == 0) &&
            ((ARM_INS_OPCODE(ins) == ARM_ADD) || (ARM_INS_OPCODE(ins) == ARM_SUB)) /*&&
            (ARM_INS_REGB(ins) != ARM_REG_R15)*/)
        {
          if (ARM_INS_REGB(ins) == ARM_REG_R13)
          {
            if ((ARM_INS_REGA(ins) != ARM_REG_R15) && ((ARM_INS_FLAGS(ins) & FL_S) == 0))
              /* encode as ADDW (+ SP) */
              return TRUE;
          }
          else
          {
            if (!((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15)))
              if ((ARM_INS_FLAGS(ins) & FL_S) == 0)
                /* encode as ADDW */
                return TRUE;
          }
        }

        /* try to encode the immediate value */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        if ((ARM_INS_OPCODE(ins) == ARM_RSB) ||
            (ARM_INS_OPCODE(ins) == ARM_SBC))
        {
          if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
              (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
            return FALSE;
        }
        else
        {
          if ((ARM_INS_REGA(ins) == ARM_REG_R15) && ((ARM_INS_FLAGS(ins) & FL_S) == 0))
            return FALSE;

          if (ARM_INS_REGB(ins) != ARM_REG_R13)
            if ((ARM_INS_REGA(ins) == ARM_REG_R13)/* ||
                (ARM_INS_REGB(ins) == ARM_REG_R15)*/)
              return FALSE;
        }

        return TRUE;
      }
      else
      {
        /* register */
        if (ARM_INS_REGB(ins) == ARM_REG_R13)
        {
          /* ADD SP + register */
          if ((ARM_INS_REGA(ins) == ARM_REG_R13) &&
              (((ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_LSL_IMM) && (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)) || (ARM_INS_SHIFTLENGTH(ins) > 3)))
            return FALSE;
        }
        else
        {
          /* ADD register */
          if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
            return FALSE;
        }

        if ((ARM_INS_OPCODE(ins) == ARM_RSB) ||
            (ARM_INS_OPCODE(ins) == ARM_SBC))
        {
          if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
              (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15) ||
              (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
            return FALSE;
        }
        else
        {
          if (((ARM_INS_REGA(ins) == ARM_REG_R15) && ((ARM_INS_FLAGS(ins) & FL_S) == 0)) ||
              (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
            return FALSE;
        }

        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
          return FALSE;

        return TRUE;
      }
      break;

    case ARM_AND:
    case ARM_EOR:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        if ((ARM_INS_REGA(ins) == ARM_REG_R13) ||
            ((ARM_INS_REGA(ins) == ARM_REG_R15) && ((ARM_INS_FLAGS(ins) & FL_S) == 0)) ||
            (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
          return FALSE;
      }
      else
      {
        /* register */
        if ((ARM_INS_REGA(ins) == ARM_REG_R13) ||
            ((ARM_INS_REGA(ins) == ARM_REG_R15) && ((ARM_INS_FLAGS(ins) & FL_S) == 0)) ||
            (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15) ||
            (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
          return FALSE;
      }

      return TRUE;
      break;

    case ARM_BLX:
    case ARM_BL:
      /* only immediate is supported */
      if ((ARM_INS_FLAGS(ins) & FL_IMMED) == 0)
        return FALSE;

    case ARM_B:
    {
      t_uint32 sbit = 25;

      if (ArmInsIsConditional(ins) && (ARM_INS_OPCODE(ins) == ARM_B))
        sbit = 20;

      if (!Uint32CheckSignExtend(ARM_INS_IMMEDIATE(ins), sbit))
        return FALSE;

      return TRUE;
    }
      break;

    case ARM_BFI:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGB(ins) == ARM_REG_R13))
        return FALSE;

      return TRUE;
      break;

    case ARM_MSR:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
        return FALSE;

    case ARM_MRS:
    case ARM_BFC:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    case ARM_CDP:
    case ARM_CDP2:
    case ARM_CLREX:
    case ARM_DBG:
    case ARM_DMB:
    case ARM_DSB:
    case ARM_ISB:
    case ARM_LDC:
    case ARM_LDC2:
    case ARM_MCR:
    case ARM_MCR2:
    case ARM_MCRR:
    case ARM_MCRR2:
    case ARM_MRC:
    case ARM_MRC2:
    case ARM_MRRC:
    case ARM_MRRC2:
    case ARM_NOP:
    case ARM_SEV:
    case ARM_STC:
    case ARM_STC2:
      /* these instructions have a 1-1 mapping */
      return TRUE;
      break;

    case ARM_CLZ:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    case ARM_TEQ:
    case ARM_TST:
      if (ARM_INS_REGB(ins) == ARM_REG_R13)
        return FALSE;

    case ARM_CMN:
    case ARM_CMP:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        if (ARM_INS_REGB(ins) == ARM_REG_R15)
          return FALSE;
      }
      else
      {
        /* register */
        if ((ARM_INS_REGB(ins) == ARM_REG_R15) ||
            (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
          return FALSE;
      }

      return TRUE;
      break;

    case ARM_LDM:
      if (ARM_INS_IMMEDIATE(ins) & 1<<13)
        return FALSE;

      if ((Uint32CountSetBits(ARM_INS_IMMEDIATE(ins)) < 2) ||
          ((ARM_INS_IMMEDIATE(ins) & 0xc000) == 0xc000))
        return FALSE;

      if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_REGB(ins) == ARM_REG_R13) &&
          ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) == FL_DIRUP))
      {
        /* pop multiple */
      }
      else
      {
        if ((ARM_INS_REGB(ins) == ARM_REG_R15))
          return FALSE;

        if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) &&
            (ARM_INS_IMMEDIATE(ins) & 1<<ARM_INS_REGB(ins)))
          return FALSE;

        /* only two types of LDM are supported by Thumb32 */
        if (((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) != FL_DIRUP) &&
            ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) != FL_PREINDEX))
          return FALSE;
      }
      return TRUE;
      break;

    case ARM_STM:
      if ((ARM_INS_IMMEDIATE(ins) & 1<<15) || (ARM_INS_IMMEDIATE(ins) & 1<<13))
        return FALSE;

      if (Uint32CountSetBits(ARM_INS_IMMEDIATE(ins)) < 2)
        return FALSE;

      if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_REGB(ins) == ARM_REG_R13) &&
          ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) == FL_PREINDEX))
      {
        /* push multiple */
      }
      else
      {
        if (ARM_INS_REGB(ins) == ARM_REG_R15)
          return FALSE;

        if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) &&
            (ARM_INS_IMMEDIATE(ins) & 1<<ARM_INS_REGB(ins)))
          return FALSE;

        if (((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) != FL_DIRUP) &&
            ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP)) != FL_PREINDEX))
          return FALSE;
      }

      return TRUE;
      break;

    case ARM_LDR:
    case ARM_LDRB:
    case ARM_LDRH:
    case ARM_LDRSB:
    case ARM_LDRSH:
    case ARM_PLD:
    case ARM_PLDW:
      if ((ARM_INS_REGB(ins) == ARM_REG_R13) &&
          ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_DIRUP | FL_WRITEBACK)) &&
          (ARM_INS_IMMEDIATE(ins) == 4))
      {
        /* POP */
        if (ARM_INS_REGA(ins) == ARM_REG_R13)
          return FALSE;

        return TRUE;
      }
      else if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate or literal */
        if (((ARM_INS_IMMEDIATE(ins) & ~0xff) == 0) && (ARM_INS_REGB(ins) != ARM_REG_R15))
        {
          /* 8-bit immediate */
          if ((ARM_INS_OPCODE(ins) == ARM_PLD) ||
              (ARM_INS_OPCODE(ins) == ARM_PLDW) ||
              (ARM_INS_OPCODE(ins) == ARM_PLI))
          {
            if ((ARM_INS_FLAGS(ins) & FL_DIRUP) == 0)
              return TRUE;
          }
          else
          {
            if (!((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_WRITEBACK)) == 0))
              if (!((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_REGB(ins) == ARM_INS_REGA(ins))))
              {
                if ((ARM_INS_OPCODE(ins) == ARM_LDRB) ||
                    (ARM_INS_OPCODE(ins) == ARM_LDRH) ||
                    (ARM_INS_OPCODE(ins) == ARM_LDRSB) ||
                    (ARM_INS_OPCODE(ins) == ARM_LDRSH))
                {
                  /* extra condition */
                  if (!(
                    (ARM_INS_REGA(ins) == ARM_REG_R13) ||
                   ((ARM_INS_REGA(ins) == ARM_REG_R15) && (ARM_INS_FLAGS(ins) & FL_WRITEBACK))
                    ))
                    return TRUE;
                }
                else
                {
                  return TRUE;
                }
              }
          }
        }

        /* if the previous test failed, try to encode a 12-bit immediate */
        if ((ARM_INS_IMMEDIATE(ins) & ~0xfff) == 0)
        {
          /* 12-bit immediate or literal */
          if (ARM_INS_REGB(ins) == ARM_REG_R15)
          {
            /* literal */
            if ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_WRITEBACK)) != FL_PREINDEX)
              return FALSE;
          }
          else
          {
            if ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) != (FL_PREINDEX | FL_DIRUP))
              return FALSE;
          }

          if ((ARM_INS_OPCODE(ins) == ARM_LDRB) ||
              (ARM_INS_OPCODE(ins) == ARM_LDRH) ||
              (ARM_INS_OPCODE(ins) == ARM_LDRSB) ||
              (ARM_INS_OPCODE(ins) == ARM_LDRSH))
          {
            if (ARM_INS_REGA(ins) == ARM_REG_R13)
              return FALSE;
          }

          return TRUE;
        }

        return FALSE;
      }
      else
      {
        /* register */
        if ((ARM_INS_OPCODE(ins) == ARM_LDRB) ||
            (ARM_INS_OPCODE(ins) == ARM_LDRH) ||
            (ARM_INS_OPCODE(ins) == ARM_LDRSB) ||
            (ARM_INS_OPCODE(ins) == ARM_LDRSH) ||
            (ARM_INS_OPCODE(ins) == ARM_PLD) ||
            (ARM_INS_OPCODE(ins) == ARM_PLDW) ||
            (ARM_INS_OPCODE(ins) == ARM_PLI))
        {
          if ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) != (FL_PREINDEX | FL_DIRUP))
            return FALSE;

          if (((ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_LSL_IMM) && (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)) || (ARM_INS_SHIFTLENGTH(ins) & ~3))
            return FALSE;

          if ((ARM_INS_OPCODE(ins) != ARM_PLD) &&
              (ARM_INS_OPCODE(ins) != ARM_PLDW) &&
              (ARM_INS_OPCODE(ins) != ARM_PLI))
            if (ARM_INS_REGA(ins) == ARM_REG_R13)
              return FALSE;
        }

        if ((ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        return TRUE;
      }

      break;

    case ARM_LDRD:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate or literal */
        if ((ARM_INS_IMMEDIATE(ins) & ~0x3fc) != 0)
          return FALSE;

        if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGABIS(ins) == ARM_REG_R13) || (ARM_INS_REGABIS(ins) == ARM_REG_R15) ||
            (ARM_INS_REGA(ins) == ARM_INS_REGABIS(ins)))
          return FALSE;

        if (ARM_INS_REGB(ins) == ARM_REG_R15)
        {
          /* literal */
          if (ARM_INS_FLAGS(ins) & FL_WRITEBACK)
            return FALSE;
        }
        else
        {
          /* immediate */
          if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) &&
              ((ARM_INS_REGB(ins) == ARM_INS_REGA(ins)) || (ARM_INS_REGB(ins) == ARM_INS_REGABIS(ins))))
            return FALSE;
        }

        return TRUE;
      }
      /* no Thumb2 instruction defined for LDRD (register) */
      break;

    case ARM_LDREX:
    case ARM_STREX:
      if ((ARM_INS_IMMEDIATE(ins) & ~0x3fc) != 0)
        return FALSE;

    case ARM_LDREXB:
    case ARM_LDREXD:
    case ARM_LDREXH:
    case ARM_STREXB:
    case ARM_STREXD:
    case ARM_STREXH:
      if ((ARM_INS_OPCODE(ins) == ARM_STREX) ||
          (ARM_INS_OPCODE(ins) == ARM_STREXB) ||
          (ARM_INS_OPCODE(ins) == ARM_STREXD) ||
          (ARM_INS_OPCODE(ins) == ARM_STREXH))
      {
        if ((ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_REGA(ins) == ARM_INS_REGC(ins))
          return FALSE;

        if (ARM_INS_REGB(ins) == ARM_INS_REGC(ins))
          return FALSE;

        if (ARM_INS_OPCODE(ins) == ARM_STREXD)
          if (ARM_INS_REGABIS(ins) == ARM_INS_REGC(ins))
            return FALSE;
      }

      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGB(ins) == ARM_REG_R15))
        return FALSE;

      if ((ARM_INS_OPCODE(ins) == ARM_LDREXD) ||
          (ARM_INS_OPCODE(ins) == ARM_STREXD))
      {
        /* additional checks */
        if ((ARM_INS_REGABIS(ins) == ARM_REG_R13) || (ARM_INS_REGABIS(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_OPCODE(ins) == ARM_LDREXD)
          if (ARM_INS_REGA(ins) == ARM_INS_REGABIS(ins))
            return FALSE;
      }

      return TRUE;
      break;

    case ARM_SMLAL:
    case ARM_SMLALBB:
    case ARM_SMLALBT:
    case ARM_SMLALTB:
    case ARM_SMLALTT:
    case ARM_SMLALD:
    case ARM_SMLALDX:
    case ARM_SMLSLD:
    case ARM_SMLSLDX:
    case ARM_SMMLS:
    case ARM_SMMLSR:
    case ARM_SMULL:
    case ARM_UMAAL:
    case ARM_UMLAL:
    case ARM_UMULL:
      if ((ARM_INS_REGS(ins) == ARM_REG_R13) || (ARM_INS_REGS(ins) == ARM_REG_R15))
        return FALSE;

      if (ARM_INS_REGA(ins) == ARM_INS_REGS(ins))
        return FALSE;

    case ARM_MLA:
    case ARM_MLS:
    case ARM_SMLABB:
    case ARM_SMLABT:
    case ARM_SMLATB:
    case ARM_SMLATT:
    case ARM_SMLAD:
    case ARM_SMLADX:
    case ARM_SMLAWB:
    case ARM_SMLAWT:
    case ARM_SMLSD:
    case ARM_SMLSDX:
    case ARM_SMMLA:
    case ARM_SMMLAR:
    case ARM_USADA8:
      if ((ARM_INS_OPCODE(ins) == ARM_MLA) ||
          (ARM_INS_OPCODE(ins) == ARM_SMLAL) ||
          (ARM_INS_OPCODE(ins) == ARM_SMULL) ||
          (ARM_INS_OPCODE(ins) == ARM_UMLAL) ||
          (ARM_INS_OPCODE(ins) == ARM_UMULL))
      {
        if (ARM_INS_FLAGS(ins) & FL_S)
          return FALSE;
      }

      if (ARM_INS_OPCODE(ins) == ARM_MLS)
      {
        if (ARM_INS_REGS(ins) == ARM_REG_R15)
          return FALSE;
      }

      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15) ||
          (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15) ||
          (ARM_INS_REGS(ins) == ARM_REG_R13))
        return FALSE;

      return TRUE;
      break;

    case ARM_MOV:
      if (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE)
      {
        /* ASR, LSL, LSR */
        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
        {
          /* shift by register */
          if ((ARM_INS_REGS(ins) == ARM_REG_R13) || (ARM_INS_REGS(ins) == ARM_REG_R15))
            return FALSE;
        }

        if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
          return FALSE;

        return TRUE;
      }
      else if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* MOV immediate, MOVW is a separate case */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15))
          return FALSE;

        return TRUE;
      }
      else
      {
        /* register */
        if ((ARM_INS_FLAGS(ins) & FL_S) &&
            ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
              (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15)))
          return FALSE;

        if (((ARM_INS_FLAGS(ins) & FL_S) == 0) &&
            ((ARM_INS_REGA(ins) == ARM_REG_R15) || (ARM_INS_REGC(ins) == ARM_REG_R15) ||
              ((ARM_INS_REGA(ins) == ARM_REG_R13) && (ARM_INS_REGC(ins) == ARM_REG_R13))))
          return FALSE;

        return TRUE;
      }
      break;

    case ARM_MOVW:
    case ARM_MOVT:
      if (ARM_INS_OPCODE(ins) == ARM_MOVW)
        if (ARM_INS_FLAGS(ins) & FL_S)
          return FALSE;

      if (ARM_INS_IMMEDIATE(ins) & ~0xffff)
        return FALSE;

      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    case ARM_PKHBT:
    case ARM_PKHTB:
    case ARM_MUL:
      if (ARM_INS_FLAGS(ins) & FL_S)
        return FALSE;

    case ARM_QADD:
    case ARM_QADD16:
    case ARM_QADD8:
    case ARM_QADDSUBX:
    case ARM_QDADD:
    case ARM_QDSUB:
    case ARM_QSUBADDX:
    case ARM_QSUB:
    case ARM_QSUB16:
    case ARM_QSUB8:
    case ARM_SADD16:
    case ARM_SADD8:
    case ARM_SADDSUBX:
    case ARM_SDIV:
    case ARM_SEL:
    case ARM_SHADD16:
    case ARM_SHADD8:
    case ARM_SHADDSUBX:
    case ARM_SHSUBADDX:
    case ARM_SHSUB16:
    case ARM_SHSUB8:
    case ARM_SMMUL:
    case ARM_SMMULR:
    case ARM_SMUAD:
    case ARM_SMUADX:
    case ARM_SMULBB:
    case ARM_SMULBT:
    case ARM_SMULTB:
    case ARM_SMULTT:
    case ARM_SMULWB:
    case ARM_SMULWT:
    case ARM_SMUSD:
    case ARM_SMUSDX:
    case ARM_SSUBADDX:
    case ARM_SSUB16:
    case ARM_SSUB8:
    case ARM_UADD16:
    case ARM_UADD8:
    case ARM_UADDSUBX:
    case ARM_UDIV:
    case ARM_UHADD16:
    case ARM_UHADD8:
    case ARM_UHADDSUBX:
    case ARM_UHSUBADDX:
    case ARM_UHSUB16:
    case ARM_UHSUB8:
    case ARM_UQADD16:
    case ARM_UQADD8:
    case ARM_UQADDSUBX:
    case ARM_UQSUBADDX:
    case ARM_UQSUB16:
    case ARM_UQSUB8:
    case ARM_USAD8:
    case ARM_USUBADDX:
    case ARM_USUB16:
    case ARM_USUB8:
      if ((ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
        return FALSE;

    case ARM_RBIT:
    case ARM_REV:
    case ARM_REV16:
    case ARM_REVSH:
    case ARM_SBFX:
    case ARM_UBFX:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGB(ins) == ARM_REG_R13) || (ARM_INS_REGB(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    case ARM_ORR:
      if (ARM_INS_REGB(ins) == ARM_REG_R13)
        return FALSE;

    case ARM_MVN:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15))
        return FALSE;

      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if (!ArmIsThumb2ImmediateEncodable(ARM_INS_IMMEDIATE(ins)))
          return FALSE;

        return TRUE;
      }
      else
      {
        /* register */
        if ((ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if (ARM_INS_REGS(ins) != ARM_REG_NONE)
          return FALSE;

        return TRUE;
      }
      break;

    case ARM_SSAT:
    case ARM_SSAT16:
    case ARM_USAT:
    case ARM_USAT16:
      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    case ARM_STR:
    case ARM_STRB:
    case ARM_STRH:
      if ((ARM_INS_REGB(ins) == ARM_REG_R13) &&
          ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) == (FL_PREINDEX | FL_WRITEBACK)) &&
          (ARM_INS_IMMEDIATE(ins) == 4))
      {
        /* PUSH */
        if ((ARM_INS_REGA(ins) == ARM_REG_R13) ||
            (ARM_INS_REGA(ins) == ARM_REG_R15))
          return FALSE;

        return TRUE;
      }
      else if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if ((ARM_INS_IMMEDIATE(ins) & ~0xff) == 0)
        {
          /* try encoding as 8-bit immediate */
          if (!((ARM_INS_REGB(ins) == ARM_REG_R15) || ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_WRITEBACK)) == 0)))
          {
            if (!((ARM_INS_REGA(ins) == ARM_REG_R15) || ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) && (ARM_INS_REGB(ins) == ARM_INS_REGA(ins)))))
            {
              if ((ARM_INS_OPCODE(ins) == ARM_STRB) ||
                  (ARM_INS_OPCODE(ins) == ARM_STRH))
              {
                if (ARM_INS_REGA(ins) != ARM_REG_R13)
                  return TRUE;
              }
              else
              {
                return TRUE;
              }
            }
          }
        }

        if ((ARM_INS_IMMEDIATE(ins) & ~0xfff) == 0)
        {
          /* try encoding as 12-bit imemdiate */
          if ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) != (FL_PREINDEX | FL_DIRUP))
            return FALSE;

          if (ARM_INS_REGA(ins) == ARM_REG_R15)
            return FALSE;

          if ((ARM_INS_OPCODE(ins) == ARM_STRB) ||
              (ARM_INS_OPCODE(ins) == ARM_STRH))
          {
            if (ARM_INS_REGA(ins) == ARM_REG_R13)
              return FALSE;
          }

          return TRUE;
        }
      }
      else
      {
        /* register */
        if ((ARM_INS_FLAGS(ins) & (FL_PREINDEX | FL_DIRUP | FL_WRITEBACK)) != (FL_PREINDEX | FL_DIRUP))
          return FALSE;

        if (((ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE) && (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_LSL_IMM)) || (ARM_INS_SHIFTLENGTH(ins) > 3))
          return FALSE;

        if ((ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
          return FALSE;

        if ((ARM_INS_OPCODE(ins) == ARM_STRB) ||
            (ARM_INS_OPCODE(ins) == ARM_STRH))
        {
          if (ARM_INS_REGA(ins) == ARM_REG_R13)
            return FALSE;
        }

        return TRUE;
      }
      break;

    case ARM_STRD:
      if (ARM_INS_FLAGS(ins) & FL_IMMED)
      {
        /* immediate */
        if (ARM_INS_IMMEDIATE(ins) & ~0x3fc)
          return FALSE;

        if ((ARM_INS_FLAGS(ins) & FL_WRITEBACK) &&
            ((ARM_INS_REGB(ins) == ARM_INS_REGA(ins)) || (ARM_INS_REGB(ins) == ARM_INS_REGABIS(ins))))
          return FALSE;

        if ((ARM_INS_REGB(ins) == ARM_REG_R15) ||
            (ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
            (ARM_INS_REGABIS(ins) == ARM_REG_R13) || (ARM_INS_REGABIS(ins) == ARM_REG_R15))
          return FALSE;

        return TRUE;
      }
      /* no register version of this instruction defined in Thumb2 */
      break;

    case ARM_SXTAB:
    case ARM_SXTAB16:
    case ARM_SXTAH:
    case ARM_UXTAB:
    case ARM_UXTAB16:
    case ARM_UXTAH:
      if (ARM_INS_REGB(ins) == ARM_REG_R13)
        return FALSE;

    case ARM_SXTB:
    case ARM_SXTB16:
    case ARM_SXTH:
    case ARM_UXTB:
    case ARM_UXTB16:
    case ARM_UXTH:
      if (ARM_INS_SHIFTLENGTH(ins) & ~0x18)
        return FALSE;

      if ((ARM_INS_REGA(ins) == ARM_REG_R13) || (ARM_INS_REGA(ins) == ARM_REG_R15) ||
          (ARM_INS_REGC(ins) == ARM_REG_R13) || (ARM_INS_REGC(ins) == ARM_REG_R15))
        return FALSE;

      return TRUE;
      break;

    default:
      if (ArmIsVectorEncodable(ins))
        return TRUE;

      return FALSE;
  }
  return FALSE;
}
/*}}}*/

t_bool ArmInsMustBeLastInITBlock(t_arm_ins *ins)
{
  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_ADD:
    if (!(ARM_INS_FLAGS(ins) & FL_IMMED) &&
        (ARM_INS_REGA(ins) == ARM_REG_R15) &&
        (ARM_INS_REGA(ins) == ARM_INS_REGB(ins)))
      /* T2 */
      return TRUE;
    break;

  case ARM_B:
  case ARM_BL:
  case ARM_BLX:
  case ARM_BX:
  case ARM_T2TBB:
  case ARM_T2TBH:
    return TRUE;

  case ARM_LDM:
    if (ARM_INS_CSIZE(ins) == 4 &&
        ARM_INS_IMMEDIATE(ins) & 1<<15)
      return TRUE;
    break;

  case ARM_LDR:
    /* the 16-bit variant can't even select the PC */
    if (ARM_INS_CSIZE(ins) == 4 &&
        ARM_INS_REGA(ins) == ARM_REG_R15)
      return TRUE;
    break;

  case ARM_MOV:
    if (ARM_INS_REGA(ins) == ARM_REG_R15)
      return TRUE;
    break;

  default:
    break;
  }

  return FALSE;
}

t_bool ArmInsIsValidInITBlock(t_arm_ins *ins)
{
  /* only Thumb instructions are allowed in IT-blocks */
  if (!(ARM_INS_FLAGS(ins) & FL_THUMB))
    return FALSE;

  /* jumps to instructions in IT-blocks are not allowed
   * By definition, all jump targets are BBL leaders.
   * However, if the only incoming edge to this BBL is a
   * fallthrough edge (maybe a jump instruction existed
   * in the past, but doesn't exist anymore) this leader
   * can be inside an IT-block. */
  /*if (T_ARM_INS(BBL_INS_FIRST(ARM_INS_BBL(ins))) == ins)
  {
    t_cfg_edge * i_edge;

    BBL_FOREACH_PRED_EDGE(ARM_INS_BBL(ins), i_edge)
      if (CFG_EDGE_CAT(i_edge) != ET_FALLTHROUGH)
        return FALSE;
  }*/

  /* only CMP, CMN and TST can set the condition flags */
  if ((ARM_INS_FLAGS(ins) & FL_S) &&
      !(ARM_INS_OPCODE(ins) == ARM_CMP ||
        ARM_INS_OPCODE(ins) == ARM_CMN ||
        ARM_INS_OPCODE(ins) == ARM_TST))
    return FALSE;

  /* these instructions can not be inside an IT-block */
  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_B:
    if (ArmInsIsConditional(ins))
      return FALSE;
    break;

  case ARM_T2CBZ:
  case ARM_T2CBNZ:
  case ARM_T2IT:
  case ARM_SETEND:
    return FALSE;

  default:
    break;
  }

  /* all other instructions can be in an IT-block */
  return TRUE;
}

t_bool ArmInsMustBeInITBlock(t_arm_ins *ins)
{
  if (!ArmInsIsValidInITBlock(ins))
    return FALSE;

  if (ARM_INS_CSIZE(ins) != 2)
    return FALSE;

  switch(ARM_INS_OPCODE(ins))
  {
  case ARM_ADC:
  case ARM_AND:
  case ARM_BIC:
  case ARM_EOR:
  case ARM_MUL:
  case ARM_UDIV:
  case ARM_MVN:
  case ARM_ORR:
  case ARM_SBC:
    if (!(ARM_INS_FLAGS(ins) & FL_S) &&
        !(ARM_INS_FLAGS(ins) & FL_IMMED))
      return TRUE;
    break;

  case ARM_ADD:
    if (!(ARM_INS_FLAGS(ins) & FL_S) &&
        ((ARM_INS_FLAGS(ins) & FL_IMMED) || (ARM_INS_REGA(ins) != ARM_INS_REGB(ins))) &&
        (ARM_INS_REGB(ins) != ARM_REG_R13))
      return TRUE;
    break;

  case ARM_MOV:
    if (!(ARM_INS_FLAGS(ins) & FL_S) &&
        (ARM_INS_SHIFTTYPE(ins) != ARM_SHIFT_TYPE_NONE))
      return TRUE;

    if (!(ARM_INS_FLAGS(ins) & FL_S) &&
        (ARM_INS_FLAGS(ins) & FL_IMMED))
      return TRUE;
    break;

  case ARM_SUB:
    if (ARM_INS_REGA(ins)==ARM_REG_R13
        && ARM_INS_REGB(ins)==ARM_REG_R13
        && (ARM_INS_FLAGS(ins) & FL_IMMED))
      return FALSE;
  case ARM_RSB:
    if (!(ARM_INS_FLAGS(ins) & FL_S))
      return TRUE;
    break;

  default:
    break;
  }

  return FALSE;
}

t_bool ArmInsChangesInstructionSet(t_arm_ins *ins)
{
  t_cfg_edge * i_edge = NULL;
  t_bool is_thumb = (ARM_INS_FLAGS(ins) & FL_THUMB) != 0;

  /* non-branch like instructions can't ever possibly
   * change the instruction set */
  if (ins != T_ARM_INS(BBL_INS_LAST(ARM_INS_BBL(ins))))
    return FALSE;

  /* iterate over every outgoing edge of this BBL */
  BBL_FOREACH_SUCC_EDGE(ARM_INS_BBL(ins), i_edge)
  {
    if (ArmBblIsThumb(CFG_EDGE_TAIL(i_edge)) ^ is_thumb)
      return TRUE;
  }

  return FALSE;
}

t_bool ArmInsIsEncodableItNoIt(t_arm_ins * ins, t_bool it_generated)
{
  if (ARM_INS_FLAGS(ins) & FL_THUMB)
  {
        ASSERT(ARM_INS_FLAGS(ins) & FL_THUMB, ("checking for encodability of Thumb for @I, but this is an ARM instruction!", ins));
        return (ArmIsThumb1EncodableCheckItNoIt(ins, FALSE, it_generated) || ArmIsThumb2EncodableCheck(ins, FALSE));
  }
  else
    return TRUE; /* TODO: check for UNPREDICTABLE ARM instructions */
}

t_bool ArmInsIsEncodable(t_arm_ins * ins)
{
        /* assume IT instructions are not generated yet */
        return ArmInsIsEncodableItNoIt(ins, FALSE);
}

t_bool ArmInsIsSwitchedBL(t_arm_ins * ins)
{
  if (ARM_INS_OPCODE(ins) == ARM_BL
      && ARM_INS_ATTRIB(ins) & IF_SWITCHJUMP)
    return TRUE;

  return FALSE;
}

t_uint32 ArmInsSwitchedBLTableEntrySize(t_arm_ins * ins)
{
  if (!ArmInsIsSwitchedBL(ins))
    return 0;

  t_uint32 sz = ((ARM_INS_FLAGS(ins) & FL_SWITCHEDBL_SZ_MASK) >> FL_SWITCHEDBL_SZ_SHIFT) << 1;
  if (sz == 0) sz++;

  return sz;
}

t_bool ArmInsSwitchedBLIsSignExtend(t_arm_ins * ins)
{
  if (!ArmInsIsSwitchedBL(ins))
    return FALSE;

  t_bool s = ARM_INS_FLAGS(ins) & FL_SWITCHEDBL_SIGNEXT;

  return s;
}

void ArmInstructionIsDirectControlTransfer(t_ins * ins_, t_bool* result) {
  t_arm_ins* ins = T_ARM_INS(ins_);

 switch (ARM_INS_OPCODE(ins)) {
  case ARM_B:
    *result = TRUE;
    return;

  case ARM_BL:
  case ARM_BLX:
  case ARM_BX:
    *result = ARM_INS_REGB(ins) == ARM_REG_NONE;
    return;

  case TH_BX_R15: /* BX R15 */
    *result = TRUE;
    return;

  /* This is a matter of taste: POP {pc} is usually just a return, so a direct control transfer */
  case ARM_LDR:
  case ARM_LDM:
    *result = TRUE;
    return;

  default:
    if (RegsetIn(ARM_INS_REGS_DEF(ins), ARM_REG_R15)) /* regular instructions that write the result of their computation in the program counter, such as ADD PC, ... etc */
      *result = FALSE;
    else
      *result = TRUE;
    return;
 }
}


/* vim: set shiftwidth=2 foldmethod=marker : */
