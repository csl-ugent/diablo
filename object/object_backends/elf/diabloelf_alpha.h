#include <diabloelf.h>



#ifndef ELF_ALPHA
#define ELF_ALPHA
void ElfReadAlphaSameEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfReadAlphaSwitchedEndian(FILE * fp, t_object * obj, t_bool read_debug);
void ElfWriteAlphaSameEndian(t_object * obj);
t_bool IsElfAlphaSameEndianOnLsb(FILE * fp);
t_bool IsElfAlphaSwitchedEndianOnMsb(FILE * fp);
#endif
