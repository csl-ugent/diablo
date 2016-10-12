#ifndef DIABLOANOPT_I386_H
#define DIABLOANOPT_I386_H

#include <diabloanopt.h>
#include <diabloi386.h>
#include "diabloanopti386_cmdline.h"
#include "diabloanopti386_constant_propagation.h"
#include "diabloanopti386_constant_optimizations.h"
#include "diabloanopti386_copy_propagation.h"
#include "diabloanopti386_emulation.h"
#include "diabloanopti386_factor.h"
#include "diabloanopti386_stack.h"
#include "diabloanopti386_inline.h"
#include "diabloanopti386_peephole.h"

#ifndef DIABLOANOPT_I386_FUNCTIONS
#ifndef DIABLOANOPT_I386_TYPES
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


#ifndef DIABLOANOPT_I386_TYPES
#define DIABLOANOPT_I386_TYPES
#undef DIABLOANOPT_I386_H
#include <diabloanopti386.h>
#endif

#ifndef DIABLOANOPT_I386_FUNCTIONS
#define DIABLOANOPT_I386_FUNCTIONS
#undef DIABLOANOPT_I386_H
#include <diabloanopti386.h>

void DiabloAnoptI386Init(int, char **);
void DiabloAnoptI386Fini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
