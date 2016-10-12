#ifndef DIABLOECOFF_H
#define DIABLOECOFF_H
#include <diabloobject.h>
#include <diabloecoff_main.h>
#include <diabloecoff_cmdline.h>



#ifndef DIABLOECOFF_FUNCTIONS
#ifndef DIABLOECOFF_TYPES
#define TYPEDEFS
/* Placeholder in case we want to introduce classes in the ecoff backend */
#undef TYPEDEFS 
#else
#define TYPES 
/* Placeholder in case we want to introduce classes in the ecoff backend */
#undef TYPES
#endif
#else
#define DEFINES
/* Placeholder in case we want to introduce classes in the ecoff backend */
#undef DEFINES
#define DEFINES2
/* Placeholder in case we want to introduce classes in the ecoff backend */
#undef DEFINES2
#define FUNCTIONS
/* Placeholder in case we want to introduce classes in the ecoff backend */
#undef FUNCTIONS
#endif

#ifndef DIABLOECOFF_TYPES
#define DIABLOECOFF_TYPES
#undef DIABLOECOFF_H
#include <diabloecoff.h>
#endif


#ifndef DIABLOECOFF_FUNCTIONS
#define DIABLOECOFF_FUNCTIONS
#undef DIABLOECOFF_H
#include <diabloecoff.h>
#endif

#ifdef DIABLOECOFF_FUNCTIONS
void DiabloEcoffInit(int, char **);
void DiabloEcoffFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */

