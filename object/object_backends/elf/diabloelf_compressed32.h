/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

#include <diabloelf.h>

#ifdef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_COMPRESSED32_FUNCTIONS
#define DIABLOELF_COMPRESSED32_FUNCTIONS
t_bool IsArchCompressedElf32(FILE * fp);
void ArchCompressedElf32Open (FILE * fp, t_archive * ret);
t_object * ArchCompressedElf32GetObject (const t_archive * arch, t_const_string name, t_object * parent, t_bool read_debug);
t_bool ObjectIsArchCompressedElf32(const t_object *);
void ArchCompressedElf32ReadMap(const t_object * obj, t_map * map);
void ArchCompressedElf32Close(t_archive *);
void SaveState(char * inname, char * outname);
#endif
#endif
