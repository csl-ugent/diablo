#include <diabloelf.h>

#ifndef DIABLOELF_MIPS_DEFINES
#define DIABLOELF_MIPS_DEFINES

#define SHT_MIPS_LIBLIST	(SHT_LOPROC + 0)
#define SHT_MIPS_CONFLICT	(SHT_LOPROC + 2)
#define SHT_MIPS_GPTAB		(SHT_LOPROC + 3)
#define SHT_MIPS_UCODE		(SHT_LOPROC + 4)
#define SHT_MIPS_DEBUG		(SHT_LOPROC + 5)

#define R_MIPS_NONE	0
#define R_MIPS_16	1
#define R_MIPS_32	2
#define R_MIPS_REL32	3
#define R_MIPS_26	4
#define R_MIPS_HI16	5
#define R_MIPS_LO16	6
#define R_MIPS_GPREL16	7
#define R_MIPS_LITERAL	8
#define R_MIPS_GOT16	9
#define R_MIPS_PC16	10
#define R_MIPS_CALL16	11
#define R_MIPS_GPREL32	12
#define R_MIPS_GOTHI16	21
#define R_MIPS_GOTLO16	22
#define R_MIPS_CALLHI16	30
#define R_MIPS_CALLLO16	31
#define R_MIPS_PC32	248
#endif

#ifndef DIABLOELF_MIPS_FUNCTIONS
#define DIABLOELF_MIPS_FUNCTIONS
void ElfReadMipsSameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWriteMipsSameEndian(FILE * fp, t_object * obj);
void ElfReadMipsSwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
t_bool IsElfMipsSameEndianOnLsb(FILE * fp);
t_bool IsElfMipsSameEndianOnMsb(FILE * fp);
t_bool IsElfMipsSwitchedEndianOnLsb(FILE * fp);
t_bool IsElfMipsSwitchedEndianOnMsb(FILE * fp);
#endif
