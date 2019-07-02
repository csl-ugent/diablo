#ifndef DIABLOANOPT_H
#define DIABLOANOPT_H
#ifdef DIABLOANOPT_INTERNAL
#include "diabloanopt_config.h"
#endif

#include <diabloflowgraph.h>
#include "diabloanopt_cmdline.h"
#include "diabloanopt_procstate.h"
#include "diabloanopt_constprop.h"
#include "diabloanopt_liveness.h"
#include "diabloanopt_factor.h"
#include "diabloanopt_extended_copy.h"
#include "diabloanopt_copy_analysis.h"
#include "diabloanopt_misc.h"
#include "diabloanopt_factor_helpers.h"

#define CONSTPROP_HELPERS 0


#ifndef DIABLOANOPT_FUNCTIONS
#ifndef DIABLOANOPT_TYPES
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


#ifndef DIABLOANOPT_TYPES
#define DIABLOANOPT_TYPES
#undef DIABLOANOPT_H
#include <diabloanopt.h>
#endif

#ifndef DIABLOANOPT_FUNCTIONS
#define DIABLOANOPT_FUNCTIONS
#undef DIABLOANOPT_H
#include <diabloanopt.h>

void DiabloAnoptInit(int, char **);
void DiabloAnoptFini();
#endif

#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
