/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloelf.h>

#ifndef DIABLOELF_ARM_DEFINES
#define DIABLOELF_ARM_DEFINES

#define EF_ARM_HASENTRY          0x2
#define EF_ARM_SYMSARESORTED     0x4
#define EF_ARM_DYNSYMSUSESEGIDX  0x8
#define EF_ARM_MAPSYMSFIRST      0x10
#define EF_ARM_EABIMASK          0xff000000
#define EF_ARM_ABI_FLOAT_HARD    0x00000400
#define EF_ARM_ABI_FLOAT_SOFT    0x00000200
#define SHT_ARM_EXIDX 		0x70000001
#define SHT_ARM_PREEMPTMAP	0x70000002
#define SHT_ARM_ATTRIBUTES	0x70000003

#define R_ARM_NONE       0
#define R_ARM_PC24       1
#define R_ARM_ABS32      2
#define R_ARM_REL32      3
#define R_ARM_PC13       4
#define R_ARM_ABS16      5
#define R_ARM_ABS12      6
#define R_ARM_THM_ABS5   7
#define R_ARM_ABS8       8
#define R_ARM_SBREL32    9
#define R_ARM_THM_PC22   10
#define R_ARM_THM_PC8    11
#define R_ARM_AMP_VCALL9 12
#define R_ARM_SWI24      13
#define R_ARM_THM_SWI8   14
#define R_ARM_XPC25      15
#define R_ARM_THM_XPC22  16
#define R_ARM_TLS_TPOFF32 19
#define R_ARM_COPY	 20
#define R_ARM_GLOB_DAT   21
#define R_ARM_JUMP_SLOT  22
#define R_ARM_RELATIVE   23
#define R_ARM_GOTOFF     24
#define R_ARM_GOTPC      25
#define R_ARM_GOT32	 26
#define R_ARM_PLT32	 27
#define R_ARM_CALL	 28
#define R_ARM_JUMP24	 29
#define R_ARM_THM_JUMP24 30
#define R_ARM_TARGET1	 38
#define R_ARM_SBREL31	 39
#define R_ARM_V4BX	 40
#define R_ARM_TARGET2	 41
#define R_ARM_PREL31     42
#define R_ARM_MOVW_ABS_NC 43
#define R_ARM_MOVT_ABS 44
#define R_ARM_MOVW_PREL_NC 45
#define R_ARM_MOVT_PREL 46
#define R_ARM_THM_MOVW_ABS_NC 47
#define R_ARM_THM_MOVT_ABS 48
#define R_ARM_THM_MOVW_PREL_NC 49
#define R_ARM_THM_MOVT_PREL 50
#define R_ARM_GOT_PREL   96
#define R_ARM_THM_JUMP11 102
#define R_ARM_TLS_LDM32  105
#define R_ARM_TLS_LDO32  106
#define R_ARM_TLS_IE32   107
#define R_ARM_TLS_LE32   108
#define R_ARM_ME_TOO	128
#endif

#ifndef DIABLOELF_ARM_FUNCTIONS
#define DIABLOELF_ARM_FUNCTIONS 
void ElfWriteArmSameEndian(FILE * fp, t_object * obj);
void ElfWriteArmSwitchedEndian(FILE * fp, t_object * obj);
void ElfReadArmSameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadArmSwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
t_bool IsElfArmSameEndianOnLsb(FILE * fp);
t_bool IsElfArmSameEndianOnMsb(FILE * fp);
t_bool IsElfArmSwitchedEndianOnLsb(FILE * fp);
t_bool IsElfArmSwitchedEndianOnMsb(FILE * fp);

t_uint64 ElfArmLinkBaseAddress(const t_object *obj, const t_layout_script *script);
t_uint64 ElfArmAlignStartOfRelRO(t_object *obj, long long currpos);
t_uint64 ElfArmAlignGotAfterRelRO(t_object *obj, long long currpos);
t_uint64 ElfArmAlignDataAfterRelRO(t_object *obj, long long currpos);

void * ArmMaybeAddFromArmToThumbStubSpaceAndSymbol(t_ast_successors *args, void *data);
#endif
