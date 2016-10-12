#ifndef DIABLOANOPT_AMD64_H
#define DIABLOANOPT_AMD64_H

#include <diabloanopt.h>
#include <diabloamd64.h>
#include "diabloanoptamd64_factor.h"
#include "diabloanoptamd64_stack.h"
#include "diabloanoptamd64_peephole.h"
#include "diabloanoptamd64_inline.h"

#ifndef DIABLOANOPT_AMD64_FUNCTIONS
#ifndef DIABLOANOPT_AMD64_TYPES
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


#ifndef DIABLOANOPT_AMD64_TYPES
#define DIABLOANOPT_AMD64_TYPES
#undef DIABLOANOPT_AMD64_H
#include <diabloanoptamd64.h>
#endif

#ifndef DIABLOANOPT_AMD64_FUNCTIONS
#define DIABLOANOPT_AMD64_FUNCTIONS
#undef DIABLOANOPT_AMD64_H
#include <diabloanoptamd64.h>

void DiabloAnoptAmd64Init(int, char **);
void DiabloAnoptAmd64Fini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
