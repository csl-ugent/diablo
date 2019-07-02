#ifndef DIABLOANOPT_ARM_H
#define DIABLOANOPT_ARM_H

#include <diabloanopt.h>
#include "diabloanoptarm_cmdline.h"
#include "diabloanoptarm_constant_propagation.h"
#include "diabloanoptarm_constant_optimizations.h"
#include "diabloanoptarm_copy_propagation.h"
#include "diabloanoptarm_emulation.h"
#include "diabloanoptarm_factor.h"
#include "diabloanoptarm_inline.h"
#include "diabloanoptarm_misc.h"
#include "diabloanoptarm_stack.h"
#include "diabloanoptarm_ls_fwd.h"
#include "diabloanoptarm_pre.h"
#include "diabloanoptarm_advanced_factor.h"

#ifndef DIABLOANOPT_ARM_FUNCTIONS
#ifndef DIABLOANOPT_ARM_TYPES
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


#ifndef DIABLOANOPT_ARM_TYPES
#define DIABLOANOPT_ARM_TYPES
#undef DIABLOANOPT_ARM_H
#include <diabloanoptarm.h>
#endif

#ifndef DIABLOANOPT_ARM_FUNCTIONS
#define DIABLOANOPT_ARM_FUNCTIONS
#undef DIABLOANOPT_ARM_H
#include <diabloanoptarm.h>

void DiabloAnoptArmInit(int, char **);
void DiabloAnoptArmFini();

#ifdef __cplusplus
extern "C"
#endif
void DiabloAnoptArmInitCfgCpp(void * vcfg, void * data);
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
