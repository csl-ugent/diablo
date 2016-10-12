#include <diabloelf.h>

#ifndef DIABLOELF_SPARC_DEFINES
#define DIABLOELF_SPARC_DEFINES
#endif

#ifndef DIABLOELF_SPARC_FUNCTIONS
#define DIABLOELF_SPARC_FUNCTIONS
void ElfReadSparcSameEndian (FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadSparcSwitchedEndian(FILE * fp, void * hdr, void * data, t_object * obj);
void ElfWriteSparcSameEndian(t_object * obj);
#endif
