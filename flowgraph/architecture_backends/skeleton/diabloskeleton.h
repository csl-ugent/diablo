#ifndef DIABLOSKELETON_H
#define DIABLOSKELETON_H

#include <diabloflowgraph.h>
#include "diabloskeleton_cmdline.h"
#include "diabloskeleton_description.h"
#include "diabloskeleton_registers.h"

#ifndef DIABLOSKELETON_FUNCTIONS
#ifndef DIABLOSKELETON_TYPES
#define TYPEDEFS
#undef TYPEDEFS 
#else
#define TYPES
#undef TYPES
#endif
#else
#define DEFINES
#undef DEFINES
#define DEFINES2
#undef DEFINES2
#define FUNCTIONS
#undef FUNCTIONS
#define CONSTRUCTORS
#undef CONSTRUCTORS
#endif


#ifndef DIABLOSKELETON_TYPES
#define DIABLOSKELETON_TYPES
#undef DIABLOSKELETON_H
#include <diabloskeleton.h>
#endif

#ifndef DIABLOSKELETON_FUNCTIONS
#define DIABLOSKELETON_FUNCTIONS
#undef DIABLOSKELETON_H
#include <diabloskeleton.h>

void DiabloSkeletonInit(int, char **);
void DiabloSkeletonFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
