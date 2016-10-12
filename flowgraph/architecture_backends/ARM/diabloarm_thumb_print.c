/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

/* Includes {{{ */
#include <diabloarm.h>
#include <string.h>
#include <ctype.h>
/* }}} */
static char *Conditions [] = {"EQ","NE","CS","CC","MI","PL","VS","VC","HI","LS","GE","LT","GT", "LE","AL"};
static char *Shifts[] = {"LSL","LSR","ASR","ROR","LSL","LSR","ASR","ROR","RRX"};
static char *ItConditions[] = {"", "T", "E", "TT", "ET", "TE", "EE", "TTT", "ETT", "TET", "EET", "TTE", "ETE", "TEE", "EEE"};
/* defined in diabloarm_print.c */
extern char *barierOptions[];
extern char *Regs[];
extern char *DoubleRegs[];
extern char *QuadRegs[];
extern char *Datatypes[];

/*!
 * \todo Document
 *
 * \param data
 * \param outputstring
 *
 * \return void
 */
/* ThumbInstructionPrint {{{ */
void ThumbInstructionPrint(t_ins * data, t_string outputstring)
{
  t_arm_ins * instruction=(t_arm_ins *) data;
  char opcode[20]="";
  char oper1[10]="", oper2[40]="", oper3[40]="", shift[6]="", shiftarg[10]="";
  char operands[120]="";
  char extra[15]="";
  t_arm_condition_code cond = ARM_INS_CONDITION(instruction);

  if (ARM_INS_TYPE(instruction) != IT_DATA && ARM_INS_OPCODE(instruction)<=ARM_UNDEF)
    sprintf(opcode, "%s%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc,
            (ARM_INS_FLAGS(instruction) & FL_S) ? "S" : "");
  else
    sprintf(opcode, "NO_TABLE_ENTRY");

  if (ARM_INS_TYPE(instruction) == IT_SYNC)
  {
    if (ARM_INS_OPCODE(instruction) == ARM_T2IT)
    {
      sprintf(opcode, "%s%s %s", opcode, ItConditions[ArmInsExtractITCondition(instruction)], Conditions[(ARM_INS_IMMEDIATE(instruction) & 0xf0) >> 4]);
    }
    else if (ARM_INS_FLAGS(instruction) & FL_IMMED)
      sprintf(oper1, "%s", barierOptions[ARM_INS_IMMEDIATE(instruction)]);
  }

  else if (ARM_INS_TYPE(instruction) == IT_NOP)
  {
    /* nothing needs to be done */
  }

  else if ((ARM_INS_OPCODE(instruction) == ARM_FLDMX) || (ARM_INS_OPCODE(instruction) == ARM_FSTMX))
  {
    ArmInsPrintVLoadStore(instruction, opcode, operands);
  }

  else if (ArmInsPrintDiabloSpecific(instruction, opcode, operands)) {}
  else if (ArmInsPrintCoproc(instruction, opcode, operands)) {}
  else if (ArmInsPrintSIMD(instruction, opcode, operands)) {}

  else if ((ARM_INS_TYPE(instruction) == IT_LOAD) || (ARM_INS_TYPE(instruction) == IT_STORE) ||
      (ARM_INS_TYPE(instruction) == IT_FLT_LOAD) || (ARM_INS_TYPE(instruction) == IT_FLT_STORE) ||
      (ARM_INS_OPCODE(instruction) == ARM_PLD) ||
      (ARM_INS_OPCODE(instruction) == ARM_PLDW) ||
      (ARM_INS_OPCODE(instruction) == ARM_PLI))
  {
    if ((ARM_INS_REGB(instruction) == ARM_REG_R13) && (ARM_INS_FLAGS(instruction) & FL_PREINDEX) && (ARM_INS_TYPE(instruction) == IT_STORE) && (ARM_INS_FLAGS(instruction) & FL_WRITEBACK))
    {
      /* PUSH */
      sprintf(opcode, "PUSH");
    }

    /* first operand if not null */
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
    if (ARM_INS_OPCODE(instruction) == ARM_STRD || ARM_INS_OPCODE(instruction) == ARM_LDRD)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGABIS(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGABIS(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGABIS(instruction)) && (ARM_INS_REGABIS(instruction) <= ARM_REG_D31)))
        sprintf(oper1+strlen(oper1),"%s,",Regs[ARM_INS_REGABIS(instruction)]);
      else
        sprintf(oper1+strlen(oper1),"%s,",ArmDoubleRegToString(ARM_INS_REGABIS(instruction)));
    }

    /* second operand between brackets, take pre- and postindexing into account */
    if (ARM_INS_REGB(instruction) != ARM_REG_NONE)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGB(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGB(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGB(instruction)) && (ARM_INS_REGB(instruction) <= ARM_REG_D31)))
        sprintf(oper2,"%s",Regs[ARM_INS_REGB(instruction)]);
      else
        sprintf(oper2,"%s",ArmDoubleRegToString(ARM_INS_REGB(instruction)));
    }

    /* third operand is ... */
    if (ARM_INS_FLAGS(instruction) & FL_IMMED)
    {
      /* ... immediate */
      char * sign = (ARM_INS_FLAGS(instruction) & FL_DIRUP) ? "" : "-";

      if (ARM_INS_IMMEDIATE(instruction) > 9)
        sprintf(oper3, "#%s%#"PRIx64"", sign, ARM_INS_IMMEDIATE(instruction));
      else
        sprintf(oper3, "#%s%"PRIx64"", sign, ARM_INS_IMMEDIATE(instruction));
    }
    else
    {
      /* ... register */
      sprintf(oper3, "%s", Regs[ARM_INS_REGC(instruction)]);
    }

    /* optional shift operand */
    if (ARM_INS_SHIFTTYPE(instruction) != ARM_SHIFT_TYPE_NONE)
    {
      sprintf(shift, ",%s ", Shifts[ARM_INS_SHIFTTYPE(instruction)]);

      if (ARM_INS_SHIFTTYPE(instruction) & 4)
        sprintf(shiftarg, "%s", Regs[ARM_INS_REGS(instruction)]);
      else
        sprintf(shiftarg, "#%d", ARM_INS_SHIFTLENGTH(instruction));
    }

    sprintf(operands, "%s[%s%s%s%s%s%s%s",
            oper1,
            oper2,
            (ARM_INS_FLAGS(instruction) & FL_PREINDEX) ? ", " : "], ",
            oper3,
            shift,
            shiftarg,
            (ARM_INS_FLAGS(instruction) & FL_PREINDEX) ? "]" : "",
            ((ARM_INS_FLAGS(instruction) & FL_WRITEBACK) && (ARM_INS_FLAGS(instruction) & FL_PREINDEX)) ? "!" : "");
  }

  else if (ARM_INS_TYPE(instruction) == IT_SWI)
  {
    sprintf(opcode,"%s",arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);
    sprintf(operands,"#%#"PRIx64"",ARM_INS_IMMEDIATE(instruction));
  }

  else if ((ARM_INS_TYPE(instruction) == IT_BRANCH) && (ARM_INS_OPCODE(instruction) == TH_BX_R15))
  {
    sprintf(opcode, "BX");
    sprintf(operands, "%s", Regs[ARM_REG_R15]);
  }
  else if (ARM_INS_TYPE(instruction) == IT_BRANCH)
  {
    sprintf(opcode, "%s", arm_opcode_table[ARM_INS_OPCODE(instruction)].desc);

    switch (ARM_INS_OPCODE(instruction))
    {
      case ARM_BX:
        sprintf(operands,"%s",Regs[ARM_INS_REGB(instruction)]);
        break;
      case ARM_T2TBB:
      case ARM_T2TBH:
        sprintf(operands, "[%s, %s%s]",
            Regs[ARM_INS_REGB(instruction)],
            Regs[ARM_INS_REGC(instruction)],
            (ARM_INS_OPCODE(instruction) == ARM_T2TBH) ? ", LSL #1" : "");
        break;
      case ARM_T2CBZ:
      case ARM_T2CBNZ:
        sprintf(operands, "%s, %#"PRIx64"",
            Regs[ARM_INS_REGB(instruction)],
            ARM_INS_IMMEDIATE(instruction));
        break;
      default:
        sprintf(operands,"%#"PRIx64"",ARM_INS_IMMEDIATE(instruction));
        break;
    }
/*
    if (ARM_INS_BBL(instruction))
      sprintf(operands+strlen(operands), " old abs: %"PRIx64"", ARM_INS_IMMEDIATE(instruction)+G_T_UINT32(BBL_OLD_ADDRESS(ARM_INS_BBL(instruction)))+BBL_+4);
*/
  }

  else if (ARM_INS_TYPE(instruction) == IT_DATAPROC || ARM_INS_TYPE(instruction) == IT_MUL || ARM_INS_TYPE(instruction) == IT_STATUS || ARM_INS_TYPE(instruction) == IT_FLT_ALU)
  {

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

    /* operand 1 */
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
    /*if (ARM_INS_REGABIS(instruction) != ARM_REG_NONE)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGABIS(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGABIS(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGABIS(instruction)) && (ARM_INS_REGABIS(instruction) <= ARM_REG_D31)))
        sprintf(oper1+strlen(oper1),"%s,",Regs[ARM_INS_REGABIS(instruction)]);
      else
        sprintf(oper1+strlen(oper1),"%s,",ArmDoubleRegToString(ARM_INS_REGABIS(instruction)));
    }*/

    /* operand 2 */
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

    /* operand 3 */
    if (ARM_INS_FLAGS(instruction) & (FL_IMMED | FL_IMMEDW) ||
        ((ARM_INS_REGC(instruction) == ARM_REG_NONE) && ARM_INS_IMMEDIATE(instruction)))
    {
      if (ARM_INS_IMMEDIATE(instruction) > 0x9)
        sprintf(oper3,"#%#"PRIx64"",ARM_INS_IMMEDIATE(instruction));
      else
        sprintf(oper3,"#%"PRIx64"",ARM_INS_IMMEDIATE(instruction));
    }
    else if (ARM_INS_REGC(instruction) != ARM_REG_NONE)
    {
      if ((!(ARM_INS_FLAGS(instruction)&FL_VFP_DOUBLE) ||
          (ARM_INS_REGC(instruction)<ARM_REG_S0) ||
          (ARM_INS_REGC(instruction)>ARM_REG_S31)) &&
          !((ARM_REG_D16 <= ARM_INS_REGC(instruction)) && (ARM_INS_REGC(instruction) <= ARM_REG_D31)))
        sprintf(oper3,"%s,",Regs[ARM_INS_REGC(instruction)]);
      else
        sprintf(oper3,"%s,",ArmDoubleRegToString(ARM_INS_REGC(instruction)));
    }
    else
      oper3[0] = '\0';

    /* shift + arg */
    if (ARM_INS_SHIFTTYPE(instruction) != ARM_SHIFT_TYPE_NONE)
        {
      sprintf(shift, "%s", Shifts[ARM_INS_SHIFTTYPE(instruction)]);

      if (ARM_INS_SHIFTTYPE(instruction) & 4)
        sprintf(shiftarg, " %s", Regs[ARM_INS_REGS(instruction)]);
          else
        sprintf(shiftarg, " #%d", ARM_INS_SHIFTLENGTH(instruction));
        }

    if ((ARM_INS_SHIFTLENGTH(instruction) == 0) &&
        (ARM_INS_OPCODE(instruction) == ARM_UXTH))
    {
      shiftarg[0] = '\0';
      shift[0] = '\0';
    }

    /* special case: MOV instructions are translated to their shift variants */
    if ((ARM_INS_OPCODE(instruction) == ARM_MOV) &&
        (ARM_INS_SHIFTTYPE(instruction) != ARM_SHIFT_TYPE_NONE))
        {
      sprintf(opcode, "%s%s", Shifts[ARM_INS_SHIFTTYPE(instruction)],
                              (ARM_INS_FLAGS(instruction) & FL_S) ? "S" : "");

      shift[0] = '\0';
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
        sprintf(oper3+strlen(oper3),"%s,",Regs[ARM_INS_REGS(instruction)]);

    sprintf(operands,"%s%s%s%s%s",
            oper1,oper2,oper3,shift,shiftarg);

    if ((strlen(shift) == 0) && (strlen(shiftarg) == 0) &&
        (operands[strlen(operands)-1] == ','))
        {
      operands[strlen(operands)-1] = '\0';
    }
  }

  else if ((ARM_INS_TYPE(instruction) == IT_LOAD_MULTIPLE) || (ARM_INS_TYPE(instruction) == IT_STORE_MULTIPLE))
  {

    if ((ARM_INS_OPCODE(instruction) == ARM_FSTMS) || (ARM_INS_OPCODE(instruction) == ARM_FSTMD) || (ARM_INS_OPCODE(instruction) == ARM_FSTMX) ||
        (ARM_INS_OPCODE(instruction) == ARM_FLDMS) || (ARM_INS_OPCODE(instruction) == ARM_FLDMD) || (ARM_INS_OPCODE(instruction) == ARM_FLDMX) ||
        (ARM_INS_OPCODE(instruction) == ARM_VPUSH) || (ARM_INS_OPCODE(instruction) == ARM_VPOP) ||
        (ARM_INS_OPCODE(instruction) == ARM_VSTM) || (ARM_INS_OPCODE(instruction) == ARM_VLDM))
    {
      ArmInsPrintVLoadStore(instruction, opcode, operands);
    }
    else
    {
    char reglist[65] = "{";
    int i, teller;

    for (i = 0, teller = 0; i < 16; i++) {
      if (ARM_INS_IMMEDIATE(instruction) & (1 << i)) {
        if (++teller == 1)
          strcat(reglist,Regs[i]);
      }
      else {
        if (teller > 2)
          strcat(reglist,"-");
        if (teller == 2)
          strcat(reglist,",");
        if (teller > 1)
          strcat(reglist,Regs[i-1]);
        if (teller > 0)
          strcat(reglist,",");
        teller = 0;
      }
    }
    if (teller == 1)
      strcat(reglist,","); /* to avoid removal of 'c' in 'pc' with pop instruction */
    reglist[strlen(reglist) - 1] = '}';

      if ((ARM_INS_REGB(instruction) == ARM_REG_R13) && !(ARM_INS_FLAGS(instruction) & FL_PREFER32) && (ARM_INS_FLAGS(instruction) & FL_WRITEBACK)) {
      (ARM_INS_OPCODE(instruction) == ARM_STM) ? sprintf(opcode,"PUSH") : sprintf(opcode,"POP");
      sprintf(operands,"%s",reglist);
    }
    else {
      (ARM_INS_OPCODE(instruction) == ARM_STM) ? sprintf(opcode,"STMDB") : sprintf(opcode,"LDMIA");
      sprintf(oper2,"%s%s",Regs[ARM_INS_REGB(instruction)], (ARM_INS_FLAGS(instruction) & FL_WRITEBACK) ? "!" : "");
      sprintf(operands,"%s,%s",oper2,reglist);
    }
  }
  }

    else if (ARM_INS_OPCODE(instruction) == ARM_ADDRESS_PRODUCER)
      {
          sprintf(opcode,"ADR");
          sprintf(operands,"%s, 0x%"PRIx64"", Regs[ARM_INS_REGA(instruction)], ARM_INS_IMMEDIATE(instruction));
  }
  else if (ARM_INS_OPCODE(instruction) == ARM_CONSTANT_PRODUCER)
  {
    sprintf(opcode,"CONST");
    sprintf(operands,"%s, 0x%"PRIx64"", Regs[ARM_INS_REGA(instruction)], ARM_INS_IMMEDIATE(instruction));
  }

  else if (ARM_INS_TYPE(instruction) == IT_DATA)
  {
    int data=ARM_INS_IMMEDIATE(instruction);
    sprintf(opcode, "DATA");
    sprintf(operands, "|%04x|", data);
    if (isprint(((char *) & data)[0]))
      sprintf(operands+6, " %c  ", ((char *) & data)[0]);
    else
      sprintf(operands+6, "\\%2x ", ((unsigned char *) & data)[0]);

    if (isprint(((char *) & data)[1]))
      sprintf(operands+10, " %c  ", ((char *) & data)[1]);
    else
      sprintf(operands+10, "\\%2.2x ", ((unsigned char *) & data)[1]);
    sprintf(operands+14, "|");
  }

  if ((ARM_INS_OPCODE(instruction) == ARM_CMP) ||
      (ARM_INS_OPCODE(instruction) == ARM_CMN) ||
      (ARM_INS_OPCODE(instruction) == ARM_TST))
  {
    /* remove trailing S from opcode */
    opcode[strlen(opcode)-1] = '\0';
  }

  if (cond != ARM_CONDITION_AL)
  {
    char tmp[20];
    strcpy(tmp, opcode);

    sprintf(opcode, "%s%s", tmp, Conditions[cond]);
  }

  /*if (ARM_INS_FLAGS(instruction) & FL_PREFER32)
  {
    sprintf(opcode+strlen(opcode), ".W");
  }

  if (ARM_INS_TYPE(instruction) != IT_DATA)
  {
    t_uint32 ins_idx = 0;
    t_arm_ins * owning_it = ArmInsFindOwningIT(instruction, NULL, &ins_idx);
    if(owning_it)
    {
      sprintf(extra, " IT(%x:%d)", ARM_INS_CADDRESS(owning_it), ins_idx);
    }
  }*/
  sprintf(outputstring,"%c%s %-10s %-69s",((ARM_INS_CSIZE(instruction) == 4) ? 'T' : 't'),extra,opcode,operands);

  StringTrim(outputstring);
}
/* }}} */
/* vim: set shiftwidth=2 foldmethod=marker : */
