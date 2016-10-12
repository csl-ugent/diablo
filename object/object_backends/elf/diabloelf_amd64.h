#include <diabloelf.h>

#ifndef DIABLOELF_AMD64_DEFINES
#define DIABLOELF_AMD64_DEFINES

#define R_AMD64_NONE       0
#define R_AMD64_64         1
#define R_AMD64_PC32       2
#define R_AMD64_GOT32      3
#define R_AMD64_PLT32      4
#define R_AMD64_COPY       5
#define R_AMD64_GLOB_DAT   6
#define R_AMD64_JMP_SLOT   7
#define R_AMD64_RELATIVE   8
#define R_AMD64_GOTPCREL   9
#define R_AMD64_32         10
#define R_AMD64_32S        11
#define R_AMD64_16         12
#define R_AMD64_PC16       13
#define R_AMD64_8          14
#define R_AMD64_PC8        15
#define R_AMD64_DPTMOD64   16
#define R_AMD64_DTPOFF64   17
#define R_AMD64_TPOFF64    18
#define R_AMD64_TLSGD      19
#define R_AMD64_TLSLD      20
#define R_AMD64_DTPOFF32   21
#define R_AMD64_GOTTPOFF   22
#define R_AMD64_TPOFF32    23
#define R_AMD64_PC64       24
#define R_AMD64_GOTOFF64   25
#define R_AMD64_GOTPC32    26
#define R_AMD64_GOT64      27
#define R_AMD64_GOTPCREL64 28
#define R_AMD64_GOTPC64    29
#define R_AMD64_GOTPLT64   30
#define R_AMD64_PLTOFF32   31

#endif

#ifndef DIABLOELF_AMD64_FUNCTIONS
#define DIABLOELF_AMD64_FUNCTIONS
void ElfReadAmd64SameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadAmd64SwitchedEndian(FILE * fp, t_object * obj);
void ElfWriteAmd64SameEndian(FILE * fp, t_object * obj);
t_bool IsElfAmd64SameEndianOnLsb(FILE * fp);
t_bool IsElfAmd64SwitchedEndianOnMsb(FILE * fp);
#endif
