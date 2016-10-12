/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#include <diabloelf.h>

#ifndef DIABLOELF_I386_DEFINES
#define DIABLOELF_I386_DEFINES
#define R_I386_NONE      0
#define R_I386_32        1
#define R_I386_PC32      2
#define R_I386_GOT32     3
#define R_I386_PLT32     4
#define R_I386_COPY      5
#define R_I386_GLOB_DAT  6
#define R_I386_JMP_SLOT  7
#define R_I386_RELATIVE  8
#define R_I386_GOTOFF    9
#define R_I386_GOTPC     10
#define R_I386_TLS_TPOFF 14
#define R_I386_TLS_IE    15 
#define R_I386_TLS_GOTIE 16
#define R_I386_TLS_LE    17
#define R_I386_TLS_GD    18
#define R_I386_16        20
#endif

#ifndef DIABLOELF_I386_FUNCTIONS
#define DIABLOELF_I386_FUNCTIONS
void ElfReadI386SameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadI386SwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWriteI386SameEndian(FILE * fp, t_object * obj);
t_bool IsElfI386SameEndianOnLsb(FILE * fp);
t_bool IsElfI386SwitchedEndianOnMsb(FILE * fp);
t_uint64 ElfI386AlignStartOfRelRO(t_object *obj, long long currpos);
t_uint64 ElfI386AlignGotAfterRelRO(t_object *obj, long long currpos);
t_uint64 ElfI386AlignDataAfterRelRO(t_object *obj, long long currpos);
#endif
