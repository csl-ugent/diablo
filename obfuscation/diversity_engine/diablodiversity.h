/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLODIVERSITY_H
#define DIABLODIVERSITY_H
#ifdef DIABLODIVERSITY_INTERNAL
#include "diablodiversity_config.h"
#endif

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif

#include "diablodiversity_engine.h"
#include "diablodiversity_structs.h"
#include "diablodiversity_engine_iterative.h"
/*#include "diablodiversity_inline.h"
#include "diablodiversity_factor.h"
#include "diablodiversity_scheduling.h"
#include "diablodiversity_instructionselection.h"
#include "diablodiversity_unfold.h"
#include "diablodiversity_flipbranches.h"
#include "diablodiversity_smcfactor.h"
#include "diablodiversity_layout.h"
#include "diablodiversity_flatten.h"
#include "diablodiversity_opaque.h"
#include "diablodiversity_branch_function.h"
#include "diablodiversity_limitinstructionset.h"*/

#ifndef DIABLODIVERSITY_FUNCTIONS
#ifndef DIABLODIVERSITY_TYPES
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


#ifndef DIABLODIVERSITY_TYPES
#define DIABLODIVERSITY_TYPES
#undef DIABLODIVERSITY_H
#include <diablodiversity.h>
#endif

#ifndef DIABLODIVERSITY_FUNCTIONS
#define DIABLODIVERSITY_FUNCTIONS
#undef DIABLODIVERSITY_H
#include <diablodiversity.h>

#endif
#endif

/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
