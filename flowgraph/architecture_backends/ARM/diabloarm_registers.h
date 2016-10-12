/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloarm.h> 

#ifndef ARM_REGISTERS_H
#define ARM_REGISTERS_H
t_string ArmRegisterName(t_reg reg);
/* The arm registers */
/* \todo document */
#define ARM_REG_NONE       255
/* \todo document */
#define ARM_REG_R0           0
/* \todo document */
#define ARM_REG_R1           1
/* \todo document */
#define ARM_REG_R2           2
/* \todo document */
#define ARM_REG_R3           3
/* \todo document */
#define ARM_REG_R4           4
/* \todo document */
#define ARM_REG_R5           5
/* \todo document */
#define ARM_REG_R6           6
/* \todo document */
#define ARM_REG_R7           7
/* \todo document */
#define ARM_REG_R8           8
/* \todo document */
#define ARM_REG_R9           9
/* \todo document */
#define ARM_REG_R10         10
/* \todo document */
#define ARM_REG_R11         11
/* \todo document */
#define ARM_REG_R12         12
/* \todo document */
#define ARM_REG_R13         13
/* \todo document */
#define ARM_REG_R14         14
/* \todo document */
#define ARM_REG_R15         15
/* \todo document */
#define ARM_REG_CPSR        16
/* \todo document */
#define ARM_REG_SPSR        17
/* \todo document */
#define ARM_REG_Q_CONDITION 18
/* \todo document */
#define ARM_REG_C_CONDITION 19
/* \todo document */
#define ARM_REG_V_CONDITION 20 
/* \todo document */
#define ARM_REG_Z_CONDITION 21 
/* \todo document */
#define ARM_REG_N_CONDITION 22 
/* \todo document */
#define ARM_REG_GE_CONDITION 23 
/* FPA registers */
/* \todo document */
#define ARM_REG_F0          24
/* \todo document */
#define ARM_REG_F1          25
/* \todo document */
#define ARM_REG_F2          26
/* \todo document */
#define ARM_REG_F3          27
/* \todo document */
#define ARM_REG_F4          28
/* \todo document */
#define ARM_REG_F5          29
/* \todo document */
#define ARM_REG_F6          30
/* \todo document */
#define ARM_REG_F7          31
/* \todo document */
#define ARM_REG_FPSR        32
/* VFP registers */
/* \todo document */
#define ARM_REG_S0          33
/* \todo document */
#define ARM_REG_S1          34
/* \todo document */
#define ARM_REG_S2          35
/* \todo document */
#define ARM_REG_S3          36
/* \todo document */
#define ARM_REG_S4          37
/* \todo document */
#define ARM_REG_S5          38
/* \todo document */
#define ARM_REG_S6          39
/* \todo document */
#define ARM_REG_S7          40
/* \todo document */
#define ARM_REG_S8          41
/* \todo document */
#define ARM_REG_S9          42
/* \todo document */
#define ARM_REG_S10         43
/* \todo document */
#define ARM_REG_S11         44
/* \todo document */
#define ARM_REG_S12         45
/* \todo document */
#define ARM_REG_S13         46
/* \todo document */
#define ARM_REG_S14         47
/* \todo document */
#define ARM_REG_S15         48
/* \todo document */
#define ARM_REG_S16         49
/* \todo document */
#define ARM_REG_S17         50
/* \todo document */
#define ARM_REG_S18         51
/* \todo document */
#define ARM_REG_S19         52
/* \todo document */
#define ARM_REG_S20         53
/* \todo document */
#define ARM_REG_S21         54
/* \todo document */
#define ARM_REG_S22         55
/* \todo document */
#define ARM_REG_S23         56
/* \todo document */
#define ARM_REG_S24         57
/* \todo document */
#define ARM_REG_S25         58
/* \todo document */
#define ARM_REG_S26         59
/* \todo document */
#define ARM_REG_S27         60
/* \todo document */
#define ARM_REG_S28         61
/* \todo document */
#define ARM_REG_S29         62
/* \todo document */
#define ARM_REG_S30         63
/* \todo document */
#define ARM_REG_S31         64

/* \todo document */
#define ARM_REG_FPSCR       65
/* \todo document */
#define ARM_REG_FPSID       66
/* \todo document */
#define ARM_REG_FPEXC       67

/*
 * VFPv3-D32, VFPv4-D32, Advanced SIMD registers
 * Registers D0-D15 are mapped to doublets of the ARM_REG_S* registers.
 * Ref: ARM-DDI0406C.b p.A2-57
 */
#define ARM_REG_D16     68
#define ARM_REG_D17     69
#define ARM_REG_D18     70
#define ARM_REG_D19     71
#define ARM_REG_D20     72
#define ARM_REG_D21     73
#define ARM_REG_D22     74
#define ARM_REG_D23     75
#define ARM_REG_D24     76
#define ARM_REG_D25     77
#define ARM_REG_D26     78
#define ARM_REG_D27     79
#define ARM_REG_D28     80
#define ARM_REG_D29     81
#define ARM_REG_D30     82
#define ARM_REG_D31     83

#define IS_THUMB_REG(x) (((x) >= ARM_REG_R0) && ((x) <= ARM_REG_R7))
#define ArmRegisterGetType(r) RegisterGetType(arm_description, (r))

t_bool ArmRegisterIsDouble(t_reg reg);
int ArmRegisterGetSize(t_reg reg);
char *ArmDoubleRegToString(t_reg r);
char *ArmQuadRegToString(t_reg r);

#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
