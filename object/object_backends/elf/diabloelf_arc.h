#include <diabloelf.h>

#ifndef DIABLOELF_ARC_DEFINES
#define DIABLOELF_ARC_DEFINES

#define R_ARC_32         0x04
#define R_ARC_S25H_PCREL 0x10
#define R_ARC_S25W_PCREL 0x11
#define R_ARC_SDA_LDST2  0x15
#define R_ARC_32_ME      0x1b
#define R_ARC_SDA32_ME   0x1e

#endif

#ifndef DIABLOELF_ARC_FUNCTIONS
#define DIABLOELF_ARC_FUNCTIONS 
void ElfWriteArcSameEndian(FILE * fp, t_object * obj);
void ElfWriteArcSwitchedEndian(FILE * fp, t_object * obj);
void ElfReadArcSameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadArcSwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
t_bool IsElfArcSameEndianOnLsb(FILE * fp);
t_bool IsElfArcSameEndianOnMsb(FILE * fp);
t_bool IsElfArcSwitchedEndianOnLsb(FILE * fp);
t_bool IsElfArcSwitchedEndianOnMsb(FILE * fp);
#endif
