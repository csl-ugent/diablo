/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
#include <ctype.h>
/*! \todo Move me and document */
static char *Conditions [] = {"EQ","NE","CS","CC","MI","PL","VS","VC","HI","LS","GE","LT","GT", "LE","","NV"};
/*! \todo Found this definition in 3 different files... Keep one of them */
extern char *Regs[];
extern char *DoubleRegs[];
extern char *QuadRegs[];
extern char *Datatypes[];
/*! \todo Move me and document */
static char *Shifts[] = {"LSL","LSR","ASR","ROR","LSL","LSR","ASR","ROR","RRX"};

char *barierOptions[] = {"invalid", "invalid", "OSHST", "OSH", "invalid", "invalid", "NSHST", "NSH", "invalid", "invalid", "ISHST", "ISH", "invalid", "invalid", "ST", "SY"};

/*!
 * \todo Document
 *
 * \param data
 * \param outputstring
 *
 * \return void
*/
/* ArmInsPrint {{{ */
 void ArmInsPrint(t_ins * data, t_string outputstring)
 {
  /* assume outputstring always has enough room - say at least 80 characters (this is way too much but it's safe */
  t_arm_ins * instruction=(t_arm_ins *) data;

  char opcode[20]="";
  char oper1[10]="", oper2[40]="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", oper3[60]="\0", shift[6]="", shiftarg[10]="";
  char operands[140]="";

  if (ARM_INS_FLAGS(instruction) & FL_THUMB) {
    ThumbInstructionPrint(data, outputstring);
    return;
  }

  /* ======================================================== VERY SPECIAL CASE: data in the code */
  if (ARM_INS_OPCODE(instruction) == ARM_DATA) {
    int imm=ARM_INS_IMMEDIATE(instruction);
    sprintf(opcode,"DATA");
    sprintf(operands, "|%08x|", imm);

    if (isprint(((char *) &imm)[0]))
      sprintf(operands+10, " %c  ", ((char *) &imm)[0]);
    else
      sprintf(operands+10,"\\%2x ",((unsigned char *) &imm)[0]);

    if (isprint(((char *) &imm)[1]))
      sprintf(operands+14, " %c  ", ((char *) &imm)[1]);
    else
      sprintf(operands+14,"\\%2x ",((unsigned char *) &imm)[1]);

    if (isprint(((char *) &imm)[2]))
      sprintf(operands+18, " %c  ", ((char *) &imm)[2]);
    else
      sprintf(operands+18,"\\%2x ",((unsigned char *) &imm)[2]);

    if (isprint(((char *) &imm)[3]))
      sprintf(operands+22, " %c  ", ((char *) &imm)[3]);
    else
      sprintf(operands+22,"\\%2.2x ",((unsigned char *) &imm)[3]);

    sprintf(operands+26,"|");
  }
  else if(ArmInsPrintDiabloSpecific(instruction, opcode, operands)) {}
  else if(ArmInsPrintCoproc(instruction, opcode, operands)) {}
  else if(ArmInsPrintSIMD(instruction, opcode, operands)) {}
  /* ========================================================================= hint instructions */
  else if(ARM_INS_TYPE(instruction) == IT_SYNC)
  {
    sprintf(opcode, "%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);

    if (ARM_INS_OPCODE(instruction) == ARM_DBG)
      sprintf(operands, "%"PRId64"", ARM_INS_IMMEDIATE(instruction) & 0xf);
    else if ((ARM_INS_OPCODE(instruction)==ARM_DMB) || (ARM_INS_OPCODE(instruction)==ARM_DSB) || (ARM_INS_OPCODE(instruction)==ARM_ISB))
      sprintf(operands, "%s", barierOptions[ARM_INS_IMMEDIATE(instruction) & 15]);
  }
  /* ===================================================================== bitfield instructions */
  else if((ARM_BITFIELD_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_BITFIELD_LAST))
  {
    t_uint32 msb = (ARM_INS_IMMEDIATE(instruction) & 0x001f0000) >> 16;
    t_uint32 lsb = ARM_INS_IMMEDIATE(instruction) & 0x0000001f;

    sprintf(opcode, "%s%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
                            Conditions[ARM_INS_CONDITION(instruction)]);

    sprintf(operands, "%s", Regs[ARM_INS_REGA(instruction)]);
    if (ARM_INS_OPCODE(instruction) != ARM_BFC)
      sprintf(operands, "%s, %s", operands, Regs[ARM_INS_REGB(instruction)]);

    if (ARM_INS_OPCODE(instruction) != ARM_RBIT)
      sprintf(operands, "%s, #%d, #%d", operands, lsb, msb-lsb+1);
  }
  /* ====================================================================== load/store exclusive */
  else if((ARM_LDRSTREX_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_LDRSTREX_LAST))
  {
    sprintf(opcode, "%s%s",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              Conditions[ARM_INS_CONDITION(instruction)]);

    if(ARM_INS_OPCODE(instruction)==ARM_STREX || ARM_INS_OPCODE(instruction)==ARM_STREXB ||
       ARM_INS_OPCODE(instruction)==ARM_STREXH || ARM_INS_OPCODE(instruction)==ARM_STREXD)
    {
      sprintf(operands, "%s, ", Regs[ARM_INS_REGC(instruction)]);
    }

    sprintf(operands, "%s%s, ", operands, Regs[ARM_INS_REGA(instruction)]);
    if(ARM_INS_OPCODE(instruction)==ARM_STREXD || ARM_INS_OPCODE(instruction)==ARM_LDREXD)
      sprintf(operands, "%s%s, ", operands, Regs[ARM_INS_REGABIS(instruction)]);

    sprintf(operands, "%s[%s]", operands, Regs[ARM_INS_REGB(instruction)]);
  }
  /* ================================================================ various system instructions */
  else if(ARM_INS_OPCODE(instruction)==ARM_SMC || ARM_INS_OPCODE(instruction)==ARM_SETEND ||
          ARM_INS_OPCODE(instruction)==ARM_CPSIE || ARM_INS_OPCODE(instruction)==ARM_CPSID || ARM_INS_OPCODE(instruction)==ARM_CPS ||
          ARM_INS_OPCODE(instruction)==ARM_NOP)
  {
    sprintf(opcode, "%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);
    if (ARM_INS_OPCODE(instruction)==ARM_NOP)
      sprintf(opcode, "%s%s", opcode, Conditions[ARM_INS_CONDITION(instruction)]);

    if (ARM_INS_OPCODE(instruction)==ARM_SMC)
      sprintf(operands, "%"PRId64"", ARM_INS_IMMEDIATE(instruction) & 0x0000000f);
    else if (ARM_INS_OPCODE(instruction)==ARM_SETEND)
      sprintf(operands, "%s", ARM_INS_IMMEDIATE(instruction) ? "BE" : "LE");
    else if (ARM_INS_OPCODE(instruction)==ARM_CPSIE || ARM_INS_OPCODE(instruction)==ARM_CPSID)
    {
      sprintf(operands, "%s%s%s", (ARM_INS_IMMEDIATE(instruction) & 0x00000100) ? "a":"",
                                  (ARM_INS_IMMEDIATE(instruction) & 0x00000080) ? "i":"",
                                  (ARM_INS_IMMEDIATE(instruction) & 0x00000040) ? "f":""
                                    );
      if(ARM_INS_IMMEDIATE(instruction) & 0x00020000)
        sprintf(operands, "%s, #%"PRId64"", operands, ARM_INS_IMMEDIATE(instruction) & 0x1f);
    }
    else if (ARM_INS_OPCODE(instruction)==ARM_CPS)
      sprintf(operands, "#%"PRId64"", ARM_INS_IMMEDIATE(instruction) & 0x1f);

  }
  else if((ARM_INS_OPCODE(instruction) == ARM_MOV) &&
          (ARM_INS_SHIFTTYPE(instruction) != ARM_SHIFT_TYPE_NONE))
  {
    sprintf(opcode, "%s%s%s", Shifts[ARM_INS_SHIFTTYPE(instruction)],
                            Conditions[ARM_INS_CONDITION(instruction)],
                            (ARM_INS_FLAGS(instruction) & FL_S) ? "S" : "");

    if ((ARM_INS_SHIFTTYPE(instruction) >= ARM_SHIFT_TYPE_LSL_IMM) &&
        (ARM_INS_SHIFTTYPE(instruction) <= ARM_SHIFT_TYPE_ROR_IMM))
      sprintf(operands, "%s, %s, #%d", Regs[ARM_INS_REGA(instruction)], Regs[ARM_INS_REGC(instruction)], ARM_INS_SHIFTLENGTH(instruction));
    else if (ARM_INS_REGS(instruction)==ARM_REG_NONE)
      sprintf(operands, "%s, %s", Regs[ARM_INS_REGA(instruction)], Regs[ARM_INS_REGC(instruction)]);
    else
      sprintf(operands, "%s, %s, %s", Regs[ARM_INS_REGA(instruction)], Regs[ARM_INS_REGC(instruction)], Regs[ARM_INS_REGS(instruction)]);
  }
  /* ================================================================ other instructions, default */
  else
  {
    /* format opcode + condition code + set condition codes flag */
    sprintf(opcode,"%s%s%s%c",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              Conditions[ARM_INS_CONDITION(instruction)],
              (ARM_INS_FLAGS(instruction)&FL_FLT_DOUBLE) ? "D" : ((ARM_INS_FLAGS(instruction)&FL_FLT_DOUBLE_EXTENDED)?"DE":""),
              (ARM_INS_FLAGS(instruction) & FL_S) ? 'S' : ' ');

    if ((ARM_INS_OPCODE(instruction) == ARM_CMP) || (ARM_INS_OPCODE(instruction) == ARM_CMN) ||
        (ARM_INS_OPCODE(instruction) == ARM_TST) || (ARM_INS_OPCODE(instruction) == ARM_TEQ))
	     /* remove trailing S from the opcode (not usually displayed here) */
       opcode[strlen(opcode) - 1] = '\0';

    if (ARM_INS_OPCODE(instruction) == ARM_LDR || ARM_INS_OPCODE(instruction) == ARM_STR ||
        ARM_INS_OPCODE(instruction) == ARM_LDRB || ARM_INS_OPCODE(instruction) == ARM_STRB ||
        ARM_INS_OPCODE(instruction) == ARM_LDRD || ARM_INS_OPCODE(instruction) == ARM_STRD ||
        ARM_INS_OPCODE(instruction) == ARM_STRH || ARM_INS_OPCODE(instruction) == ARM_LDRH ||
        ARM_INS_OPCODE(instruction) == ARM_LDRSB || ARM_INS_OPCODE(instruction) == ARM_LDRSH)
    {
      if ((ARM_INS_FLAGS(instruction) & FL_WRITEBACK) && !(ARM_INS_FLAGS(instruction) & FL_PREINDEX))
      {
        /* the T variant: insert a 'T' right after the opcode */
        char temp_opc[20];
        int opclen = strlen(arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);
        strcpy(temp_opc,opcode);
        sprintf(&(opcode[opclen]),"T%s",&(temp_opc[opclen]));
      }
    }

    if (
      (((ARM_SIMD_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_SIMD_LAST)) ||
      ((ARM_FP_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_FP_LAST))) &&
      (ARM_INS_DATATYPE(instruction) != DT_NONE)
      )
    {
      int opclen = strlen(opcode);
      if(opcode[opclen-1] == ' ')
      {
        /* remove trailing space character */
        opcode[opclen-1] = '\0';
      }

      sprintf(opcode, "%s.%s ", opcode, Datatypes[ARM_INS_DATATYPE(instruction)]);
    }

    /* format operand 1 */
    if (ARM_INS_REGA(instruction) != ARM_REG_NONE)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGA(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGA(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGA(instruction)) && (ARM_INS_REGA(instruction) <= ARM_REG_D31)))
        sprintf(oper1,"%s,",Regs[ARM_INS_REGA(instruction)]);
      else
        sprintf(oper1,"%s,",ArmDoubleRegToString(ARM_INS_REGA(instruction)));
    }

    /* format operand 2 */
    if (ARM_INS_REGB(instruction) != ARM_REG_NONE)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGB(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGB(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGB(instruction)) && (ARM_INS_REGB(instruction) <= ARM_REG_D31)))
        sprintf(oper2,"%s,",Regs[ARM_INS_REGB(instruction)]);
      else
        sprintf(oper2,"%s,",ArmDoubleRegToString(ARM_INS_REGB(instruction)));
    }
    else if(ARM_INS_OPCODE(instruction) == ARM_SSAT ||
            ARM_INS_OPCODE(instruction) == ARM_SSAT16 ||
            ARM_INS_OPCODE(instruction) == ARM_USAT ||
            ARM_INS_OPCODE(instruction) == ARM_USAT16)
    {
      sprintf(oper2, "#%"PRId64",", ARM_INS_IMMEDIATE(instruction));
    }

    /* format operand 3 and shift and shiftarg*/
    if ((ARM_INS_FLAGS(instruction) & (FL_IMMED|FL_IMMEDW)) ||
        (ARM_INS_REGC(instruction) == ARM_REG_NONE && ARM_INS_IMMEDIATE(instruction)))
    {
      if((ARM_INS_OPCODE(instruction) == ARM_VCMP) ||
         (ARM_INS_OPCODE(instruction) == ARM_VCMPE))
      {
        t_uint32 temp = ARM_INS_IMMEDIATE(instruction);
        float tempfloat = *(float*)&temp;

        if(tempfloat == 0.0f)
          sprintf(oper3, "#0.0");
        else
          sprintf(oper3, "#%#f", tempfloat);
      }
      else
      {
  	    sprintf(oper3,"%s%#"PRIx64"",
                  (ARM_INS_TYPE(instruction) == IT_BRANCH) ? "" : "#",
                  ARM_INS_IMMEDIATE(instruction));
      }
	/*	      sprintf(shift, "");
		      sprintf(shiftarg, "");
	*/
      if (ARM_INS_TYPE(instruction) == IT_BRANCH && ARM_INS_BBL(instruction))
        sprintf(oper3+strlen(oper3)," abs: %"PRIx64"",
                  ARM_INS_IMMEDIATE(instruction)+G_T_UINT32(BBL_OLD_ADDRESS(ARM_INS_BBL(instruction)))+BBL_NINS(ARM_INS_BBL(instruction))*4+4);
    }
    else
    {
      if (ARM_INS_REGC(instruction) != ARM_REG_NONE)
      {
        if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
            (ARM_INS_REGC(instruction)<ARM_REG_S0) ||
            (ARM_INS_REGC(instruction)>ARM_REG_S31)) &&
            !((ARM_REG_D16 <= ARM_INS_REGC(instruction)) && (ARM_INS_REGC(instruction) <= ARM_REG_D31)))
          sprintf(oper3,"%s",Regs[ARM_INS_REGC(instruction)]);
        else
          sprintf(oper3,"%s",ArmDoubleRegToString(ARM_INS_REGC(instruction)));
      }
      else
      {
        oper3[0] = '\0';
      }
	/*	      sprintf(shift, "");
		      sprintf(shiftarg, "");
	*/
      if (ARM_INS_SHIFTTYPE(instruction) != ARM_SHIFT_TYPE_NONE)
      {
        sprintf(shift, ",%s ", Shifts[ARM_INS_SHIFTTYPE(instruction)]);

        if (ARM_INS_SHIFTTYPE(instruction) & 0x4)
          sprintf(shiftarg, "%s",Regs[ARM_INS_REGS(instruction)]);
        else
          sprintf(shiftarg, "#%d",ARM_INS_SHIFTLENGTH(instruction));
      }

      if (((ARM_INS_OPCODE(instruction) == ARM_UXTB) ||
            (ARM_INS_OPCODE(instruction) == ARM_UXTH) ||
            (ARM_INS_OPCODE(instruction) == ARM_SXTH))
           && (ARM_INS_SHIFTLENGTH(instruction) == 0))
      {
        shift[0]=0;    // replaces sprintf(shift, ""); to avoid warnings
        shiftarg[0]=0; // sprintf(shiftarg, ""); to avoid warnings
      }

      if (ARM_INS_OPCODE(instruction) == ARM_MLA ||
          ARM_INS_OPCODE(instruction) == ARM_MLS ||
          ARM_INS_OPCODE(instruction) == ARM_SMLABB ||
          ARM_INS_OPCODE(instruction) == ARM_SMLABT ||
          ARM_INS_OPCODE(instruction) == ARM_SMLATB ||
          ARM_INS_OPCODE(instruction) == ARM_SMLATT ||
          ARM_INS_OPCODE(instruction) == ARM_SMLAWB ||
          ARM_INS_OPCODE(instruction) == ARM_SMLAWT ||
          ARM_INS_OPCODE(instruction) == ARM_SMLALBB ||
          ARM_INS_OPCODE(instruction) == ARM_SMLALBT ||
          ARM_INS_OPCODE(instruction) == ARM_SMLALTB ||
          ARM_INS_OPCODE(instruction) == ARM_SMLALTT ||
          ARM_INS_OPCODE(instruction) == ARM_UMAAL ||
          ARM_INS_OPCODE(instruction) == ARM_UMLAL ||
          ARM_INS_OPCODE(instruction) == ARM_UMULL)
        sprintf(oper3+strlen(oper3),",%s",Regs[ARM_INS_REGS(instruction)]);
    }

    /* SPECIAL CASE: MRS and MSR instructions use different registers */
    if (ARM_INS_OPCODE(instruction) == ARM_MRS)
    {
      if (ARM_INS_FLAGS(instruction) & FL_SPSR)
        sprintf(oper2,"SPSR");
      else
        sprintf(oper2,"CPSR");
    }

    if (ARM_INS_OPCODE(instruction) == ARM_MSR)
      sprintf(oper1,"%cPSR_%s%s%s%s,",
                (ARM_INS_FLAGS(instruction) & FL_SPSR) ? 'S' : 'C',
                (ARM_INS_FLAGS(instruction) & FL_CONTROL) ? "c" : "",
                (ARM_INS_FLAGS(instruction) & (1U << 29)) ? "x" : "",
                (ARM_INS_FLAGS(instruction) & FL_STATUS) ? "s" : "",
                (ARM_INS_FLAGS(instruction) & (1U << 31U)) ? "f" : "");

    sprintf(operands,"%s%s%s%s%s",oper1,oper2,oper3,shift,shiftarg);

    /* special case: load and store instructions -> modify operands according to flags */
    if ((ARM_INS_TYPE(instruction) == IT_LOAD) || (ARM_INS_TYPE(instruction) == IT_STORE) ||
        (((ARM_INS_TYPE(instruction) == IT_FLT_LOAD) || (ARM_INS_TYPE(instruction) == IT_FLT_STORE)) ||
        (ARM_INS_TYPE(instruction) == ARM_FSTS) || (ARM_INS_TYPE(instruction) == ARM_FSTD) ||
        (ARM_INS_TYPE(instruction) == ARM_FLDS) || (ARM_INS_TYPE(instruction) == ARM_FLDD)) ||
        (ARM_INS_OPCODE(instruction) == ARM_PLD) || (ARM_INS_OPCODE(instruction) == ARM_PLDW) ||
        (ARM_INS_OPCODE(instruction) == ARM_PLI))
    {
      char oper3temp[20];

      if (!(ARM_INS_FLAGS(instruction) & FL_DIRUP))
      {
        /* put a minus in front of the third argument */
        if (ARM_INS_FLAGS(instruction) & (FL_IMMED|FL_IMMEDW))
          sprintf(oper3temp,"#-%s",oper3+1);
        else
          sprintf(oper3temp,"-%s",oper3);

        sprintf(oper3,"%s", oper3temp);
      }

      if ((ARM_INS_TYPE(instruction) == IT_STORE) &&
          (ARM_INS_REGB(instruction) == ARM_REG_R13) &&
          (ARM_INS_FLAGS(instruction) & FL_PREINDEX) && (ARM_INS_FLAGS(instruction) & FL_WRITEBACK) &&
          (ARM_INS_IMMEDIATE(instruction) == 4))
      {
        /* push */
        sprintf(opcode, "PUSH%s", Conditions[ARM_INS_CONDITION(instruction)]);
        sprintf(operands, "{%s}", Regs[ARM_INS_REGA(instruction)]);
      }
      else if ((ARM_INS_TYPE(instruction) == IT_LOAD) &&
        (ARM_INS_REGB(instruction) == ARM_REG_R13) &&
        !(ARM_INS_FLAGS(instruction) & FL_PREINDEX) &&
        (ARM_INS_IMMEDIATE(instruction) == 4))
      {
        /* pop */
        sprintf(opcode, "POP%s", Conditions[ARM_INS_CONDITION(instruction)]);
        sprintf(operands, "{%s}", Regs[ARM_INS_REGA(instruction)]);
      }
      else
      {
  	    /* strip comma from the end of oper */
        oper2[strlen(oper2)-1] = '\0';

        sprintf(operands,"%s[%s%s%s%s%s%s%c",
                  oper1,
                  oper2,
                  (ARM_INS_FLAGS(instruction) & FL_PREINDEX) ? "," : "],",
                  oper3,
                  shift,
                  shiftarg,
                  (ARM_INS_FLAGS(instruction) & FL_PREINDEX) ? "]" : "",
                  (ARM_INS_FLAGS(instruction) & FL_WRITEBACK) ? '!' : ' ');
      }
    }

    /* SPECIAL CASE: load and store multiple */
    if (((ARM_INS_TYPE(instruction) == IT_LOAD_MULTIPLE) || (ARM_INS_TYPE(instruction) == IT_STORE_MULTIPLE)) &&
      !((ARM_SIMD_LOADSTORE_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_SIMD_LOADSTORE_LAST))
      )
    {
      /* add a suffix to the opcode, indicating the addressing mode */
      char suffix[3];
      char reglist[100] = "{";
      int i, regnum;
      t_uint32 fl = ARM_INS_FLAGS(instruction);

      suffix[0] = (fl & FL_DIRUP) ? 'I' : 'D';
      suffix[1] = (fl & FL_PREINDEX) ? 'B' : 'A';
      suffix[2] = '\0';

      sprintf(oper2, "%s", "");

      if ((ARM_INS_OPCODE(instruction) == ARM_LDM) &&
          (ARM_INS_REGB(instruction) == ARM_REG_R13) &&
          (fl & FL_DIRUP) && !(fl & FL_PREINDEX) && (fl & FL_WRITEBACK))
        sprintf(opcode, "POP%s", Conditions[ARM_INS_CONDITION(instruction)]);

      else if ((ARM_INS_OPCODE(instruction) == ARM_STM) &&
          (ARM_INS_REGB(instruction) == ARM_REG_R13) &&
          !(fl & FL_DIRUP) && (fl & FL_PREINDEX) && (fl & FL_WRITEBACK))
        sprintf(opcode, "PUSH%s", Conditions[ARM_INS_CONDITION(instruction)]);

      else
      {
        sprintf(opcode,"%s%s%s",
                  arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
                  Conditions[ARM_INS_CONDITION(instruction)],
                  suffix);

        /* add a ! to operand 2 if writeback is enabled */
        sprintf(oper2,"%s%s,",Regs[ARM_INS_REGB(instruction)],(fl & FL_WRITEBACK) ? "!" : "");
      }


      /* print out register list in a human-readable form */
      for (i = 0; i < 16; i++)
        if (ARM_INS_IMMEDIATE(instruction) & (1 << i))
        {
          strcat(reglist,Regs[i]);
          strcat(reglist,",");
        }

      reglist[strlen(reglist) - 1] = '}';

      if (fl & FL_USERMODE_REGS)
        strcat(reglist,"^");

      sprintf(operands,"%s%s",oper2,reglist);
    }

    if ((ARM_INS_OPCODE(instruction) == ARM_LFM) || (ARM_INS_OPCODE(instruction) == ARM_SFM))
    {
      char reglist[100] = "{";
      int i;
      t_uint32 fl = ARM_INS_FLAGS(instruction);

      sprintf(oper2,"%s%s",Regs[ARM_INS_REGB(instruction)],(fl & FL_WRITEBACK) ? "!" : "");

      for (i = ARM_REG_F0; i < ARM_REG_F7+1; i++)
        if (RegsetIn(ARM_INS_MULTIPLE(instruction),i))
        {
          strcat(reglist,Regs[i]);
          strcat(reglist,",");
        }

      reglist[strlen(reglist) - 1] = '}';

      if (ARM_INS_IMMEDIATE(instruction))
        sprintf(operands,"[%s %c %"PRIx64"],%s",
                  oper2,
                  (ARM_INS_FLAGS(instruction) & FL_DIRUP)?'+':'-',
                  ARM_INS_IMMEDIATE(instruction),
                  reglist);
      else
        sprintf(operands,"%s,%s",
                  oper2,
                  reglist);
    }

    if ((ARM_INS_OPCODE(instruction) == ARM_FSTMS) || (ARM_INS_OPCODE(instruction) == ARM_FSTMD) || (ARM_INS_OPCODE(instruction) == ARM_FSTMX) ||
        (ARM_INS_OPCODE(instruction) == ARM_FLDMS) || (ARM_INS_OPCODE(instruction) == ARM_FLDMD) || (ARM_INS_OPCODE(instruction) == ARM_FLDMX) ||
        (ARM_INS_OPCODE(instruction) == ARM_VPUSH) || (ARM_INS_OPCODE(instruction) == ARM_VPOP) ||
        (ARM_INS_OPCODE(instruction) == ARM_VSTM) || (ARM_INS_OPCODE(instruction) == ARM_VLDM))
    {
      ArmInsPrintVLoadStore(instruction, opcode, operands);
    }
  }

  if(strlen(operands)>0)
    if(operands[strlen(operands)-1]==',')
      operands[strlen(operands)-1]='\0';

  sprintf(outputstring,"%-10s %-69s",opcode,operands);
  StringTrim(outputstring);
}
/* }}} */


t_bool ArmInsPrintSIMD(t_arm_ins * instruction, t_string opcode, t_string operands)
{
  t_bool handled = FALSE;


  if((ARM_INS_OPCODE(instruction) == ARM_VMOV64_C2S) || (ARM_INS_OPCODE(instruction) == ARM_VMOV64_C2D))
  {
    sprintf(opcode, "%s%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,Conditions[ARM_INS_CONDITION(instruction)]);

    if((ARM_INS_REGA(instruction) >= ARM_REG_R0) && (ARM_INS_REGA(instruction) <= ARM_REG_R15))
    {
      /* to 2 core registers */
      sprintf(operands, "%s, %s", Regs[ARM_INS_REGA(instruction)], Regs[ARM_INS_REGABIS(instruction)]);

      if(ARM_INS_FLAGS(instruction) & FL_VFP_DOUBLE)
        sprintf(operands, "%s, %s", operands, ArmDoubleRegToString(ARM_INS_REGB(instruction)));
      else
        sprintf(operands, "%s, %s, %s", operands, Regs[ARM_INS_REGB(instruction)], Regs[ARM_INS_REGC(instruction)]);
    }
    else
    {
      /* to double or single */
      if(ARM_INS_FLAGS(instruction) & FL_VFP_DOUBLE)
        sprintf(operands, "%s", ArmDoubleRegToString(ARM_INS_REGA(instruction)));
      else
        sprintf(operands, "%s, %s", Regs[ARM_INS_REGA(instruction)], Regs[ARM_INS_REGABIS(instruction)]);

      sprintf(operands, "%s, %s, %s", operands, Regs[ARM_INS_REGB(instruction)], Regs[ARM_INS_REGC(instruction)]);
    }

    handled = TRUE;
  }
  else if((ARM_FP_VCVT_FIRST <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_FP_VCVT_LAST))
  {
    sprintf(opcode, "%s%s.%s.%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc, Conditions[ARM_INS_CONDITION(instruction)], Datatypes[ARM_INS_DATATYPE(instruction)], Datatypes[ARM_INS_DATATYPEOP(instruction)]);

    if(ARM_INS_DATATYPE(instruction) == DT_F64 ||
       (ARM_INS_FLAGS(instruction) & FL_VFP_DOUBLE))
      sprintf(operands, "%s", ArmDoubleRegToString(ARM_INS_REGA(instruction)));
    else
      sprintf(operands, "%s", Regs[ARM_INS_REGA(instruction)]);

    if(ARM_INS_DATATYPEOP(instruction) == DT_F64 ||
       (ARM_INS_FLAGS(instruction) & FL_VFP_DOUBLE))
      sprintf(operands, "%s, %s", operands, ArmDoubleRegToString(ARM_INS_REGB(instruction)));
    else
      sprintf(operands, "%s, %s", operands, Regs[ARM_INS_REGB(instruction)]);

    if(ARM_INS_FLAGS(instruction) & FL_IMMED)
      sprintf(operands, "%s, #%"PRIu64"", operands, ARM_INS_IMMEDIATE(instruction));

    handled = TRUE;
  }
  /* NEON ===================================================================== SIMD instructions */
  else if((ARM_INS_TYPE(instruction) == IT_SIMD) ||
    ((ARM_SIMD_FIRSTSTORE <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_SIMD_LASTSTORE)) ||
    ((ARM_SIMD_FIRSTLOAD  <= ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction) <= ARM_SIMD_LASTLOAD))
    )
  {
    handled = TRUE;

    sprintf(opcode, "%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);

    if(ARM_INS_ATTRIB(instruction) & IF_CONDITIONAL)
      sprintf(opcode, "%s%s", opcode, Conditions[ARM_INS_CONDITION(instruction)]);

    /* destination datatype */
    if(ARM_INS_DATATYPE(instruction) != DT_NONE)
      sprintf(opcode, "%s.%s", opcode, Datatypes[ARM_INS_DATATYPE(instruction)]);

    /* source datatype (operands) */
    if(ARM_INS_DATATYPEOP(instruction) != DT_NONE)
      sprintf(opcode, "%s.%s", opcode, Datatypes[ARM_INS_DATATYPEOP(instruction)]);

    /* -------------------------------------------------------------------- VTBL/VTBX instruction */
    if((ARM_INS_OPCODE(instruction) == ARM_VTBL) || (ARM_INS_OPCODE(instruction) == ARM_VTBX))
    {
      char reglist[100] = "{";
      int i, reg;

      reg = 0;
      for(i = ARM_REG_S0; i <= ARM_REG_D31; i++)
      {
        if(RegsetIn(ARM_INS_MULTIPLE(instruction), i))
          sprintf(reglist, "%s%s,", reglist, DoubleRegs[reg]);

        if(i <= ARM_REG_S31)
          i++;
        if(i == ARM_REG_S31)
          i = ARM_REG_D16-1;

        reg++;
      }
      reglist[strlen(reglist) - 1] = '}';

      sprintf(operands, "%s, %s, %s",
        ArmDoubleRegToString(ARM_INS_REGA(instruction)),
        reglist,
        ArmDoubleRegToString(ARM_INS_REGC(instruction)));

      handled = TRUE;
    }
    /* ------------------------------------------------------------------- VLDx/VSTx instructions */
    else if((ARM_INS_OPCODE(instruction) >= ARM_SIMD_FIRSTSTORE) && (ARM_INS_OPCODE(instruction) <= ARM_SIMD_LASTLOAD))
    {
      char reglist[100] = "{";
      char scalar[10] = "";
      int i, dreg;

      if(ARM_INS_NEONFLAGS(instruction) & NEONFL_MULTI_SCALAR)
      {
        if(ARM_INS_MULTIPLESCALAR(instruction) == MULTIPLESCALAR_ALL)
          sprintf(scalar, "%s", "[]");
        else
          sprintf(scalar, "[%u]", ARM_INS_MULTIPLESCALAR(instruction));
      }

      dreg = 0;
      for(i = ARM_REG_S0; i <= ARM_REG_D31; i++)
      {
        if(RegsetIn(ARM_INS_MULTIPLE(instruction), i))
          sprintf(reglist, "%s%s%s,",
                    reglist,
                    DoubleRegs[dreg],
                    scalar);

        if(i <= ARM_REG_S31)
          i++;
        if(i == ARM_REG_S31)
          i = ARM_REG_D16-1;

        dreg++;
      }
      reglist[strlen(reglist) - 1] = '}';

      sprintf(operands, "%s, [%s", reglist, Regs[ARM_INS_REGB(instruction)]);

      if(ARM_INS_MULTIPLEALIGNMENT(instruction) > 1)
        sprintf(operands, "%s:%u", operands, ARM_INS_MULTIPLEALIGNMENT(instruction));

      sprintf(operands, "%s]%s", operands, (ARM_INS_FLAGS(instruction) & FL_WRITEBACK) ? "!" : "");

      if(ARM_INS_REGC(instruction) != ARM_REG_NONE)
        sprintf(operands, "%s, %s", operands, Regs[ARM_INS_REGC(instruction)]);

      handled = TRUE;
    }
    /* ------------------------------------------------------------------ other SIMD instructions */
    else
    {
      /* destination register */
      if(ARM_INS_REGA(instruction) != ARM_REG_NONE)
      {
        if((ARM_INS_NEONFLAGS(instruction) & NEONFL_A_CORE) || (ARM_INS_NEONFLAGS(instruction) & NEONFL_A_SINGLE))
          sprintf(operands, "%s", Regs[ARM_INS_REGA(instruction)]);

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_A_DOUBLE)
          sprintf(operands, "%s", ArmDoubleRegToString(ARM_INS_REGA(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_A_QUAD)
          sprintf(operands, "%s", ArmQuadRegToString(ARM_INS_REGA(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_A_SCALAR)
          if (ARM_INS_OPCODE(instruction)==ARM_VMOV_C2SCALAR)
            sprintf(operands, "%s[%u]", ArmDoubleRegToString(ARM_INS_REGA(instruction)), ARM_INS_REGASCALAR(instruction));
          else
            sprintf(operands, "%s[]", ArmDoubleRegToString(ARM_INS_REGA(instruction)));

        else
          FATAL(("illegal register type for destination register"));
      }

      /* first operand register */
      if(ARM_INS_REGB(instruction) != ARM_REG_NONE)
      {
        if((ARM_INS_NEONFLAGS(instruction) & NEONFL_B_CORE) || (ARM_INS_NEONFLAGS(instruction) & NEONFL_B_SINGLE))
          sprintf(operands, "%s, %s", operands, Regs[ARM_INS_REGB(instruction)]);

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_B_DOUBLE)
          sprintf(operands, "%s, %s", operands, ArmDoubleRegToString(ARM_INS_REGB(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_B_QUAD)
          sprintf(operands, "%s, %s", operands, ArmQuadRegToString(ARM_INS_REGB(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_B_SCALAR)
          if (ARM_INS_OPCODE(instruction)==ARM_VMOV_SCALAR2C || ARM_INS_OPCODE(instruction)==ARM_VDUP_SCALAR)
            sprintf(operands, "%s, %s[%u]", operands, ArmDoubleRegToString(ARM_INS_REGB(instruction)), ARM_INS_REGBSCALAR(instruction));
          else
            sprintf(operands, "%s, %s[]", operands, ArmDoubleRegToString(ARM_INS_REGB(instruction)));

        else
          FATAL(("illegal register type for first operand register: %s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc));
      }

      /* second operand register */
      if(ARM_INS_REGC(instruction) != ARM_REG_NONE)
      {
        if((ARM_INS_NEONFLAGS(instruction) & NEONFL_C_CORE) || (ARM_INS_NEONFLAGS(instruction) & NEONFL_A_SINGLE))
          sprintf(operands, "%s, %s", operands, Regs[ARM_INS_REGC(instruction)]);

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_C_DOUBLE)
          sprintf(operands, "%s, %s", operands, ArmDoubleRegToString(ARM_INS_REGC(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_C_QUAD)
          sprintf(operands, "%s, %s", operands, ArmQuadRegToString(ARM_INS_REGC(instruction)));

        else if(ARM_INS_NEONFLAGS(instruction) & NEONFL_C_SCALAR)
          if ((ARM_SIMD2REGSSCALAR_FIRST<=ARM_INS_OPCODE(instruction)) && (ARM_INS_OPCODE(instruction)<=ARM_SIMD2REGSSCALAR_LAST))
            sprintf(operands, "%s, %s[%u]", operands, ArmDoubleRegToString(ARM_INS_REGC(instruction)), ARM_INS_REGCSCALAR(instruction));
          else
            sprintf(operands, "%s, %s[]", operands, ArmDoubleRegToString(ARM_INS_REGC(instruction)));

        else
          FATAL(("illegal register type for second operand register: %s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc));
      }

      handled = TRUE;
    }

    /* print immediate value if necessary */
    if(ARM_INS_FLAGS(instruction) & FL_IMMED)
    {
      t_uint64 imm = ARM_INS_IMMEDIATE(instruction);

      t_uint32 temp;

      switch(ARM_INS_DATATYPE(instruction))
      {
        case DT_I8:
        case DT_S8:
        case DT_U8:
        case DT_P8:
          sprintf(operands, "%s, #0x%02x", operands, (t_uint8)(imm & 0xff));
          break;

        case DT_I16:
        case DT_S16:
        case DT_U16:
        case DT_F16:
        case DT_P16:
          sprintf(operands, "%s, #0x%04x", operands, (t_uint16)(imm & 0xffff));
          break;

        case DT_I32:
        case DT_S32:
        case DT_U32:
          sprintf(operands, "%s, #0x%08x", operands, (t_uint32)(imm & 0xffffffff));
          break;

        case DT_F32:
          if(
            (ARM_INS_OPCODE(instruction) == ARM_VMOV_IMM_2)
            )
          {
            temp = (imm & 0xffffffff);
            sprintf(operands, "%s, #%f", operands, *(float*)&temp);
          } else {
            sprintf(operands, "%s, #0x%08x", operands, (t_uint32)(imm & 0xffffffff));
          }
          break;

        default:
          sprintf(operands, "%s, #0x%016"PRIx64"", operands, imm);
      }
    }

  }

  return handled;
}

t_bool ArmInsPrintCoproc(t_arm_ins * instruction, t_string opcode, t_string operands)
{
  t_bool handled = FALSE;

    /* ======================================================================== MRC/MCR instruction */
  if (ARM_INS_OPCODE(instruction) == ARM_MRC || ARM_INS_OPCODE(instruction) == ARM_MCR ||
            ARM_INS_OPCODE(instruction) == ARM_MRC2 || ARM_INS_OPCODE(instruction) == ARM_MCR2)
  {
    handled = TRUE;

    char * Rd;
    if (ARM_INS_OPCODE(instruction) == ARM_MCR || ARM_INS_OPCODE(instruction) == ARM_MCR2)
    {
      if (ARM_INS_REGC(instruction) == ARM_REG_NONE)
        Rd = Regs[15];
      else
        Rd = Regs[ARM_INS_REGC(instruction)];
    }
    else
    {
      Rd = Regs[ARM_INS_REGA(instruction)];
    }

    sprintf(opcode,"%s%s",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              (ARM_INS_OPCODE(instruction)==ARM_MRC || ARM_INS_OPCODE(instruction)==ARM_MCR) ? Conditions[ARM_INS_CONDITION(instruction)] : "");
    sprintf(operands,"%"PRId64",%"PRId64",%s,cr%"PRId64",cr%"PRId64",{%"PRId64"}",
              ARM_INS_IMMEDIATE(instruction) & 0xf,
              (ARM_INS_IMMEDIATE(instruction) >> 4) & 0x7,
              Rd,
              (ARM_INS_IMMEDIATE(instruction) >> 14) & 0xf,
              (ARM_INS_IMMEDIATE(instruction) >> 10) & 0xf,
              (ARM_INS_IMMEDIATE(instruction) >> 7) & 0x7);
  }
  /* ====================================================================== MRRC/MCRR instruction */
  else if (ARM_INS_OPCODE(instruction) == ARM_MRRC || ARM_INS_OPCODE(instruction) == ARM_MCRR ||
            ARM_INS_OPCODE(instruction) == ARM_MRRC2 || ARM_INS_OPCODE(instruction) == ARM_MCRR2)
  {
    handled = TRUE;

    char *Rn, *Rd;
    Rd = Regs[(ARM_INS_OPCODE(instruction) == ARM_MRRC || ARM_INS_OPCODE(instruction) == ARM_MRRC2) ? ARM_INS_REGA(instruction) : ARM_INS_REGB(instruction)];
    Rn = Regs[(ARM_INS_OPCODE(instruction) == ARM_MRRC || ARM_INS_OPCODE(instruction) == ARM_MRRC2) ? ARM_INS_REGB(instruction) : ARM_INS_REGC(instruction)];

    sprintf(opcode,"%s%s",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              (ARM_INS_OPCODE(instruction) == ARM_MRRC || ARM_INS_OPCODE(instruction) == ARM_MCRR) ? Conditions[ARM_INS_CONDITION(instruction)] : "");
    sprintf(operands,"%"PRId64",%"PRId64",%s,%s,cr%"PRId64"",
              ARM_INS_IMMEDIATE(instruction) & 0xf,
              (ARM_INS_IMMEDIATE(instruction) >> 4) & 0xf,
              Rd,
              Rn,
              (ARM_INS_IMMEDIATE(instruction) >> 8) & 0xf);
  }
  /* ======================================================================== LDC/STC instruction */
  else if (ARM_INS_OPCODE(instruction) == ARM_LDC || ARM_INS_OPCODE(instruction) == ARM_STC ||
            ARM_INS_OPCODE(instruction) == ARM_LDC2 || ARM_INS_OPCODE(instruction) == ARM_STC2)
  {
    handled = TRUE;

    char amode[80];

    sprintf(opcode,"%s%s%c",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              (ARM_INS_OPCODE(instruction) == ARM_LDC || ARM_INS_OPCODE(instruction) == ARM_STC) ? Conditions[ARM_INS_CONDITION(instruction)] : "",
              (ARM_INS_FLAGS(instruction) & FL_LONG_TRANSFER)?'L':' ');

    if (ARM_INS_FLAGS(instruction) & FL_PREINDEX)
    {
      sprintf(amode,"[%s,#%s%"PRId64"]%c",
                Regs[ARM_INS_REGB(instruction)],
                (ARM_INS_FLAGS(instruction) & FL_DIRUP) ? "" : "-",
                4*((ARM_INS_IMMEDIATE(instruction) >> 8) & 0xff),
                (ARM_INS_FLAGS(instruction) & FL_WRITEBACK) ? '!' : ' ');
    }
    else
    {
      if (ARM_INS_FLAGS(instruction) & FL_WRITEBACK)
        sprintf(amode,"[%s],#%s%"PRId64"",
                  Regs[ARM_INS_REGB(instruction)],
                  (ARM_INS_FLAGS(instruction) & FL_DIRUP) ? "" : "-",
                  4*((ARM_INS_IMMEDIATE(instruction) >> 8) & 0xff));
      else
        sprintf(amode,"[%s],%"PRId64"",
                  Regs[ARM_INS_REGB(instruction)],
                  (ARM_INS_IMMEDIATE(instruction) >> 8) & 0xff);

   }

   sprintf(operands,"%"PRId64",cr%"PRId64",%s",
           ARM_INS_IMMEDIATE(instruction) & 0xf,
             (ARM_INS_IMMEDIATE(instruction) >> 4) & 0xf,
             amode);
  }
  /* ============================================================================ CDP instruction */
  else if (ARM_INS_OPCODE(instruction) == ARM_CDP || ARM_INS_OPCODE(instruction) == ARM_CDP2)
  {
    handled = TRUE;

    sprintf(opcode, "%s%s",
      arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
      (ARM_INS_OPCODE(instruction) == ARM_CDP) ? Conditions[ARM_INS_CONDITION(instruction)] : "");

    sprintf(operands, "%"PRId64", %"PRId64", cr%"PRId64", cr%"PRId64", cr%"PRId64", %"PRId64"",
      (ARM_INS_IMMEDIATE(instruction) & 0x00000f00) >> 8,
      (ARM_INS_IMMEDIATE(instruction) & 0x00f00000) >> 20,
      (ARM_INS_IMMEDIATE(instruction) & 0x0000f000) >> 12,
      (ARM_INS_IMMEDIATE(instruction) & 0x000f0000) >> 16,
      (ARM_INS_IMMEDIATE(instruction) & 0x0000000f),
      (ARM_INS_IMMEDIATE(instruction) & 0x000000e0) >> 5
      );
  }

  return handled;
}

void ArmInsPrintVLoadStore(t_arm_ins * instruction, t_string opcode, t_string operands)
{
  char oper2[40] = "";
  char reglist[100] = "{";
  int i, regnum;
  int doubles = (ARM_INS_FLAGS(instruction) & FL_VFP_DOUBLE) != 0;
  t_uint32 fl = ARM_INS_FLAGS(instruction);

  if((ARM_INS_OPCODE(instruction) == ARM_VSTM) || (ARM_INS_OPCODE(instruction) == ARM_VLDM))
  {
    char suffix[3];
    suffix[0] = (fl & FL_DIRUP) ? 'I' : 'D';
    suffix[1] = (fl & FL_PREINDEX) ? 'B' : 'A';
    suffix[2] = '\0';

    sprintf(opcode,"%s%s%s",
              arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
              Conditions[ARM_INS_CONDITION(instruction)],
              suffix);
  }

  if(!(ARM_INS_OPCODE(instruction) == ARM_VPUSH || ARM_INS_OPCODE(instruction) == ARM_VPOP))
    sprintf(oper2,"%s%s",Regs[ARM_INS_REGB(instruction)],(fl & FL_WRITEBACK) ? "!" : "");

  regnum = 0;
  for(i = ARM_REG_S0; i <= ARM_REG_D31; i++)
  {
    if(RegsetIn(ARM_INS_MULTIPLE(instruction), i))
      sprintf(reglist, "%s%s,", reglist, (doubles) ? DoubleRegs[regnum] : Regs[regnum+ARM_REG_S0]);

    if(doubles)
    {
      if(i < ARM_REG_S31)
        i++;
      if(i == ARM_REG_S31)
        /* minus 1 because i increments by one at the end of the loop iteration */
        i = ARM_REG_D16-1;
    }
    else if(i == ARM_REG_S31)
      break;

    regnum++;
  }

  reglist[strlen(reglist) - 1] = '}';

  if (ARM_INS_IMMEDIATE(instruction))
    sprintf(operands,"[%s %c %"PRIx64"],%s",
              oper2,
              (ARM_INS_FLAGS(instruction) & FL_DIRUP)?'+':'-',
              ARM_INS_IMMEDIATE(instruction),reglist);
  else if(ARM_INS_OPCODE(instruction) == ARM_VPUSH || ARM_INS_OPCODE(instruction) == ARM_VPOP)
    sprintf(operands,"%s",reglist);
  else
    sprintf(operands,"%s,%s",
              oper2,
              reglist);
}

t_bool ArmInsPrintDiabloSpecific(t_arm_ins * instruction, t_string opcode, t_string operands)
{
  t_bool handled = TRUE;

  /* ================================================================================ Pseudo call */
  if (ARM_INS_OPCODE(instruction) == ARM_PSEUDO_CALL)
  {
    sprintf(opcode,"CALL%s %s",
              Conditions[ARM_INS_CONDITION(instruction)],
              FUNCTION_NAME(((t_function *) ARM_INS_DATA(instruction))));
  }
  /* =========================================================================== Address producer */
  else if (ARM_INS_OPCODE(instruction) == ARM_ADDRESS_PRODUCER)
  {
    sprintf(opcode,"ADR%s",
              Conditions[ARM_INS_CONDITION(instruction)]);
    sprintf(operands,"r%d %"PRIx64"",
              ARM_INS_REGA(instruction), ARM_INS_IMMEDIATE(instruction));

  }
  /* ========================================================================== Constant producer */
  else if (ARM_INS_OPCODE(instruction) == ARM_CONSTANT_PRODUCER)
  {
    sprintf(opcode,"CONST%s",
              Conditions[ARM_INS_CONDITION(instruction)]);
    sprintf(operands,"r%d %"PRIx64" (%"PRId64")",
              ARM_INS_REGA(instruction),
              ARM_INS_IMMEDIATE(instruction),
              ARM_INS_IMMEDIATE(instruction));
  }
  /* ============================================================================= Float producer */
  else if (ARM_INS_OPCODE(instruction) == ARM_FLOAT_PRODUCER)
  {
    sprintf(opcode,"FLOAT%s",
              Conditions[ARM_INS_CONDITION(instruction)]);
    sprintf(operands,"f%d",
              ARM_INS_REGA(instruction) - ARM_REG_F0);
  }
  /* ========================================================================= VFP Float producer */
  else if (ARM_INS_OPCODE(instruction) == ARM_VFPFLOAT_PRODUCER)
  {
    sprintf(opcode,"VFPFLOAT%s",
              Conditions[ARM_INS_CONDITION(instruction)]);

    if (!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE))
      sprintf(operands,"%s,",Regs[ARM_INS_REGA(instruction)]);
    else
      sprintf(operands,"%s,",DoubleRegs[(ARM_INS_REGA(instruction)-ARM_REG_S0)/2]);
  }
  else
    handled = FALSE;

  return handled;
}

/* vim: set shiftwidth=2 foldmethod=marker : */
