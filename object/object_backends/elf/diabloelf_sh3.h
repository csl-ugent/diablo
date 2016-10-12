#include <diabloelf.h>

#ifndef DIABLOELF_SH3_DEFINES
#define DIABLOELF_SH3_DEFINES

#define EF_SH_MACH_MASK	0x1f
#define EF_SH_UNKNOWN	   0 /* For backwards compatibility.  */
#define EF_SH1		   1
#define EF_SH2		   2
#define EF_SH3		   3
#define EF_SH_HAS_DSP(flags) ((flags) & 4)
#define EF_SH_DSP	   4
#define EF_SH3_DSP	   5
#define EF_SH_HAS_FP(flags) ((flags) & 8)
#define EF_SH3E		   8
#define EF_SH4		   9
#define EF_SH5		  10

#define	R_SH_NONE		0
#define	R_SH_DIR32		1
#define	R_SH_REL32		2
#define	R_SH_DIR8WPN		3
#define	R_SH_IND12W		4
#define	R_SH_DIR8WPL		5
#define	R_SH_DIR8WPZ		6
#define	R_SH_DIR8BP		7
#define	R_SH_DIR8W		8
#define	R_SH_DIR8L		9
#define	R_SH_SWITCH16		25
#define	R_SH_SWITCH32		26
#define	R_SH_USES		27
#define	R_SH_COUNT		28
#define	R_SH_ALIGN		29
#define	R_SH_CODE		30
#define	R_SH_DATA		31
#define	R_SH_LABEL		32
#define	R_SH_SWITCH8		33
#define	R_SH_GNU_VTINHERIT	34
#define	R_SH_GNU_VTENTRY	35
#define	R_SH_GOT32		160
#define	R_SH_PLT32		161
#define	R_SH_COPY		162
#define	R_SH_GLOB_DAT		163
#define	R_SH_JMP_SLOT		164
#define	R_SH_RELATIVE		165
#define	R_SH_GOTOFF		166
#define	R_SH_GOTPC		167
#endif

#ifndef DIABLOELF_SH3_FUNCTIONS
#define DIABLOELF_SH3_FUNCTIONS 
void ElfReadSh3SameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadSh3SwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWriteSh3SameEndian (FILE * fp, t_object * obj);
t_bool IsElfSh3SameEndianOnLsb(FILE * fp);
t_bool IsElfSh3SameEndianOnMsb(FILE * fp);
t_bool IsElfSh3SwitchedEndianOnLsb(FILE * fp);
t_bool IsElfSh3SwitchedEndianOnMsb(FILE * fp);
#endif
