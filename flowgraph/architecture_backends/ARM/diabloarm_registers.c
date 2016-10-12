/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h>
/*! \todo this definition also in arm description .... */
char *DoubleRegs[] = {"d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","d10","d11","d12","d13","d14","d15","d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
char *QuadRegs[] = {"q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"};
char *Regs[] = {"r0","r1","r2","r3","r4","r5","r6","r7","r8","r9","r10","r11","r12","r13", "r14","r15","cpsr","spsr","q","c","v","z","n","ge","f0","f1","f2","f3","f4","f5","f6","f7","fpsr","s0","s1","s2","s3","s4","s5","s6","s7","s8","s9","s10","s11","s12","s13","s14","s15","s16","s17","s18","s19","s20","s21","s22","s23","s24","s25","s26","s27","s28","s29","s30","s31","fpscr","fpsid","fpexc"};

/* regnames {{{ */
static char * regnames[] = {
  "r0", /* 0 */
  "r1", /* 1 */
  "r2", /* 2 */
  "r3", /* 3 */
  "r4", /* 4 */
  "r5", /* 5 */
  "r6", /* 6 */
  "r7", /* 7 */
  "r8", /* 8 */
  "r9", /* 9 */
  "r10", /* 10 */
  "r11", /* 11 */
  "r12", /* 12 */
  "r13", /* 13 */
  "r14", /* 14 */
  "r15", /* 15 */
  "cpsr", /* 16 */
  "spsr", /* 17 */
  "cQ", /* 18 */
  "cC", /* 19 */
  "cV", /* 20 */
  "cZ", /* 21 */
  "cN", /* 22 */
  "cGE", /* 23 */
  "f0", /* 24 */
  "f1", /* 25 */
  "f2", /* 26 */
  "f3", /* 27 */
  "f4", /* 28 */
  "f5", /* 29 */
  "f6", /* 30 */
  "f7", /* 31 */
  "fpsr", /*32 */
  "s0", /* 33 */
  "s1", /* 34 */
  "s2", /* 35 */
  "s3", /* 36 */
  "s4", /* 37 */
  "s5", /* 38 */
  "s6", /* 39 */
  "s7", /* 40 */
  "s8", /* 41 */
  "s9", /* 42 */
  "s10", /* 43 */
  "s11", /* 44 */
  "s12", /* 45 */
  "s13", /* 46 */
  "s14", /* 47 */
  "s15", /* 48 */
  "s16", /* 49 */
  "s17", /* 50 */
  "s18", /* 51 */
  "s19", /* 52 */
  "s20", /* 53 */
  "s21", /* 54 */
  "s22", /* 55 */
  "s23", /* 56 */
  "s24", /* 57 */
  "s25", /* 58 */
  "s26", /* 59 */
  "s27", /* 60 */
  "s28", /* 61 */
  "s29", /* 62 */
  "s30", /* 63 */
  "s31", /* 64 */
  "fpscr", /* 65 */
  "fpsid", /* 66 */
  "fpexc", /* 67 */
  "d16",  /* 68 */
  "d17",  /* 69 */
  "d18",  /* 70 */
  "d19",  /* 71 */
  "d20",  /* 72 */
  "d21",  /* 73 */
  "d22",  /* 74 */
  "d23",  /* 75 */
  "d24",  /* 76 */
  "d25",  /* 77 */
  "d26",  /* 78 */
  "d27",  /* 79 */
  "d28",  /* 80 */
  "d29",  /* 81 */
  "d30",  /* 82 */
  "d31"  /* 63 */
};
/* }}} */
/*!
 * \todo Document? Maybe remove it?
 *
 * \param reg
 *
 * \return t_string 
*/
/* ArmRegisterName {{{ */
t_string ArmRegisterName(t_reg reg) 
{
  return regnames[reg];
}
/* }}} */

t_bool ArmRegisterIsDouble(t_reg reg)
{
  if(reg>=ARM_REG_D16 && reg<=ARM_REG_D31)
  {
    return TRUE;
  }
  else if(reg>=ARM_REG_S0 && reg<=ARM_REG_S31)
    /* registers D0-D15 are mapped to registers S0-S31, pairwise. */
  {
    if ((reg-ARM_REG_S0)%2 == 0)
    {
      return TRUE;
    }
  }

  return FALSE;
}

int ArmRegisterGetSize(t_reg reg)
{
  if(ArmRegisterIsDouble(reg))
  {
    return 8;
  }

  return 4;
}

/* Convert a register index saved in the instruction class to
 * a double-precision register index.
 * Result should be in the range 0-31.
 */
 t_reg ArmRegToDoubleReg(t_reg r)
 {
  if(r > ARM_REG_S31)
  {
    r += (16 - ARM_REG_D16);
  }
  else
  {
    r -= ARM_REG_S0;
    r /= 2;
  }

  return r;
}

/* TODO: Can this be solved in a cleaner way? */
char *ArmDoubleRegToString(t_reg r)
{
  return DoubleRegs[ArmRegToDoubleReg(r)];
}

char *ArmQuadRegToString(t_reg r)
{
  return QuadRegs[ArmRegToDoubleReg(r)/2];
}

/* vim: set shiftwidth=2 foldmethod=marker : */
