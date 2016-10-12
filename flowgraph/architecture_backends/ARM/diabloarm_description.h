#include <diabloarm.h>
#ifndef ARM_DESCRIPTION_H
#define ARM_DESCRIPTION_H
/*! A description of different Arm Properties */
extern t_architecture_description arm_description;

#if MAX_REG_ITERATOR > 64
   extern t_regset ARM_ALL_BUT_PC_AND_COND;
#else
   #define ARM_ALL_BUT_PC_AND_COND	0x7fff
#endif
#endif
/* vim: set shiftwidth=2 foldmethod=marker: */
