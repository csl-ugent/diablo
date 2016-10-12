#include <diabloalpha.h>

#ifndef DIABLO_ALPHA_REGISTERS_H
#define DIABLO_ALPHA_REGISTERS_H

t_string AlphaRegName(t_reg);

#define ALPHA_REG_NONE 255

#define ALPHA_REG_V0   0x00 
#define ALPHA_REG_T0   0x01 
#define ALPHA_REG_T1   0x02
#define ALPHA_REG_T2   0x03
#define ALPHA_REG_T3   0x04 
#define ALPHA_REG_T4   0x05
#define ALPHA_REG_T5   0x06 
#define ALPHA_REG_T6   0x07
#define ALPHA_REG_T7   0x08 

#define ALPHA_REG_S0   0x09 
#define ALPHA_REG_S1   0x0a 
#define ALPHA_REG_S2   0x0b 
#define ALPHA_REG_S3   0x0c 
#define ALPHA_REG_S4   0x0d 
#define ALPHA_REG_S5   0x0e 
#define ALPHA_REG_S6   0x0f 
#define ALPHA_REG_FP   0x0f      /* fp & s6 are the same */

#define ALPHA_REG_A0   0x10 
#define ALPHA_REG_A1   0x11 
#define ALPHA_REG_A2   0x12 
#define ALPHA_REG_A3   0x13 
#define ALPHA_REG_A4   0x14 
#define ALPHA_REG_A5   0x15 
#define ALPHA_REG_T8   0x16 
#define ALPHA_REG_T9   0x17 
#define ALPHA_REG_T10  0x18  
#define ALPHA_REG_T11  0x19 
#define ALPHA_REG_RA   0x1a 
#define ALPHA_REG_PV   0x1b  
#define ALPHA_REG_T12  0x1b 
#define ALPHA_REG_AT   0x1c 
#define ALPHA_REG_GP   0x1d 
#define ALPHA_REG_SP   0x1e 
#define ALPHA_REG_ZERO 0x1f 

#define ALPHA_REG_F0    0x20 
#define ALPHA_REG_F1    0x21 
#define ALPHA_REG_F2    0x22 
#define ALPHA_REG_F3    0x23 
#define ALPHA_REG_F4    0x24 
#define ALPHA_REG_F5    0x25 
#define ALPHA_REG_F6    0x26 
#define ALPHA_REG_F7    0x27 
#define ALPHA_REG_F8    0x28 
#define ALPHA_REG_F9    0x29 
#define ALPHA_REG_F10   0x2a 
#define ALPHA_REG_F11   0x2b 
#define ALPHA_REG_F12   0x2c 
#define ALPHA_REG_F13   0x2d 
#define ALPHA_REG_F14   0x2e 
#define ALPHA_REG_F15   0x2f 
#define ALPHA_REG_F16   0x30 
#define ALPHA_REG_F17   0x31 
#define ALPHA_REG_F18   0x32 
#define ALPHA_REG_F19   0x33 
#define ALPHA_REG_F20   0x34 
#define ALPHA_REG_F21   0x35 
#define ALPHA_REG_F22   0x36 
#define ALPHA_REG_F23   0x37 
#define ALPHA_REG_F24   0x38 
#define ALPHA_REG_F25   0x39 
#define ALPHA_REG_F26   0x3a 
#define ALPHA_REG_F27   0x3b 
#define ALPHA_REG_F28   0x3c 
#define ALPHA_REG_F29   0x3d 
#define ALPHA_REG_F30   0x3e 

#define ALPHA_REG_FZERO 0x3f 


#endif
