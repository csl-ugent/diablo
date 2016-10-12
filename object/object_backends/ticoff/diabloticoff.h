#ifndef DIABLOTICOFF_H
#define DIABLOTICOFF_H
#include <diabloobject.h>
#include <diabloticoff_cmdline.h>

/* Section flags */
/* Regular section (allocated, relocated, loaded) */
#define STYP_REG    0x00000000
/* Dummy section (relocated, not allocated, not loaded) */
#define STYP_DSECT  0x00000001 
/* Noload section (allocated, relocated, not loaded) */
#define STYP_NOLOAD 0x00000002 
/* Copy section (relocated, loaded, but not allocated; relocation entries are processed normally) */
#define STYP_COPY   0x00000010 
/* Section contains executable code */
#define STYP_TEXT   0x00000020 
/* Section contains initialized data */
#define STYP_DATA   0x00000040 
/* Section contains uninitialized data */
#define STYP_BSS    0x00000080 
/* Alignment used as a blocking factor */
#define STYP_BLOCK  0x00001000 
/* Section should pass through unchanged */
#define STYP_PASS   0x00002000 
/* Section requires conditional linking */
#define STYP_CLINK  0x00004000 
/* Section contains vector table */
#define STYP_VECTOR 0x00008000 
/* Section has been padded */
#define STYP_PADDED 0x00010000 

/* Symbol storage classes */
/* No storage class */
#define C_NULL 0
/* Reserved */
#define C_AUTO 1
/* External definition */
#define C_EXT 2
/* Static */
#define C_STAT 3
/* Reserved */
#define C_REG 4
/* External reference */
#define C_EXTREF 5
/* Label */
#define C_LABEL 6
/* Undefined label */
#define C_ULABEL 7 
/* Reserved */
#define C_MOS 8
/* Reserved */
#define C_ARG 9
/* Reserved */
#define C_STRTAG 10
/* Reserved */
#define C_MOU 11 
/* Reserved */
#define C_UNTAG 12
/* Reserved */
#define C_TPDEF 13
/* Undefined static */
#define C_USTATIC 14
/* Reserved */
#define C_ENTAG 15
/* Reserved */
#define C_MOE 16 
/* Reserved */
#define C_REGPARM 17
/* Reserved */
#define C_FIELD 18
/* Tentative external definition */
#define C_UEXT 19
/* Static load time label */
#define C_STATLAB 20 
/* External load time label */
#define C_EXTLAB 21 
/* Last declared parameter of a function with variable number of arguments */
#define C_VARARG 27
/* Reserved */
#define C_BLOCK 100
/* Reserved */
#define C_FCN 101 
/* Reserved */
#define C_EOS 102 
/* Reserved */
#define C_FILE 103 
/* Used only by utility programs */
#define C_LINE 104 






#ifndef DIABLOTICOFF_FUNCTIONS
#ifndef DIABLOTICOFF_TYPES
#define TYPEDEFS
/* Placeholder in case we want to introduce classes in the ticoff backend */
#undef TYPEDEFS 
#else
#define TYPES 
/* Placeholder in case we want to introduce classes in the ticoff backend */
#undef TYPES
#endif
#else
#define DEFINES
/* Placeholder in case we want to introduce classes in the ticoff backend */
#undef DEFINES
#define DEFINES2
/* Placeholder in case we want to introduce classes in the ticoff backend */
#undef DEFINES2
#define FUNCTIONS
/* Placeholder in case we want to introduce classes in the ticoff backend */
#undef FUNCTIONS
#endif

#ifndef DIABLOTICOFF_TYPES
#define DIABLOTICOFF_TYPES
#undef DIABLOTICOFF_H
#include <diabloticoff.h>
#endif


#ifndef DIABLOTICOFF_FUNCTIONS
#define DIABLOTICOFF_FUNCTIONS
#undef DIABLOTICOFF_H
#include <diabloticoff.h>
#endif

#ifdef DIABLOTICOFF_FUNCTIONS
void DiabloTiCoffInit(int, char **);
void DiabloTiCoffFini();
void TiCoffReadCommon(FILE * fp, t_object * obj, t_bool read_debug);
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */


