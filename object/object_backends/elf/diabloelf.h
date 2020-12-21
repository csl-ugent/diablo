/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOELF_H
#define DIABLOELF_H
#include <diabloobject.h>
#include <diabloelf_main.h>
#include <diabloelf_types.h>
#include <diabloelf_common32.h>
#include <diabloelf_raw32.h>
#include <diabloelf_compressed32.h>
#include <diabloelf_common64.h>
#include <diabloelf_endian.h>
#include <diabloelf_alpha.h>
#include <diabloelf_arm.h>
#include <diabloelf_arc.h>
#include <diabloelf_cmdline.h>
#include <diabloelf_i386.h>
#include <diabloelf_ia64.h>
#include <diabloelf_mips.h>
#include <diabloelf_powerpc.h>
#include <diabloelf_ppc64.h>
#include <diabloelf_sh3.h>
#include <diabloelf_sparc.h>
#include <diabloelf_amd64.h>
#include <diabloelf_spe.h>



#ifndef DIABLOELF_FUNCTIONS
#ifndef DIABLOELF_TYPES
#define TYPEDEFS
/* Placeholder in case we want to introduce classes in the elf backend */
#undef TYPEDEFS 
#else
#define TYPES 
/* Placeholder in case we want to introduce classes in the elf backend */
#undef TYPES
#endif
#else
#define DEFINES
/* Placeholder in case we want to introduce classes in the elf backend */
#undef DEFINES
#define DEFINES2
/* Placeholder in case we want to introduce classes in the elf backend */
#undef DEFINES2
#define FUNCTIONS
/* Placeholder in case we want to introduce classes in the elf backend */
#undef FUNCTIONS
#endif

#ifndef DIABLOELF_TYPES
#define DIABLOELF_TYPES
#undef DIABLOELF_H
#include <diabloelf.h>
#endif


#ifndef DIABLOELF_FUNCTIONS
#define DIABLOELF_FUNCTIONS
#undef DIABLOELF_H
#include <diabloelf.h>
void ElfAddFinalizationRoutine(t_object *obj, t_symbol *fini_routine_sym);
void ElfAddInitializationRoutine(t_object *obj, t_symbol *init_routine_sym, t_bool at_end);
void ElfAddNeededLib (t_object * obj, t_string libname);
void ElfSymbolMaybeAddVersionAlias(t_object *obj, t_symbol_table *symbol_table, t_symbol *sym);
void ElfParseVersioningInformation(t_object *obj);
void ElfGetSymbolDataBroker(t_string name, void *data, t_bool *result);
void ElfGetExportedSymbolDataBroker(t_string name, void *data, t_bool *result);
#endif

#ifdef DIABLOELF_FUNCTIONS
void DiabloElfInit(int, char **);
void DiabloElfFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */

