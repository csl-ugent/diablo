#ifndef DIABLOSMC_H
#define DIABLOSMC_H
#ifdef DIABLOSMC_INTERNAL
#include "diablosmc_config.h"
#endif

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif

#include "diablosmc_codebyte.h"
#include "diablosmc_state.h"
#include "diablosmc_cmdline.h"
#include "diablosmc_flowgraph.h"
#include "diablosmc_dynamic_members.h"
#include "diablosmc_statelist.h"
#include "diablosmc_codebytelist.h"
#include "diablosmc_dots.h"
#include "diablosmc_write_out.h"
//#include "diablosmc_equivalent_instructions.h"
#include "diablosmc_factoring.h"
#include "diablosmc_deflowgraph.h"

#ifndef DIABLOSMC_FUNCTIONS
#ifndef DIABLOSMC_TYPES
#define TYPEDEFS
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef DEFINES
#define DEFINES2
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diablosmc_codebyte.class.h"
#include "diablosmc_state.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOSMC_TYPES
#define DIABLOSMC_TYPES
#undef DIABLOSMC_H
#include <diablosmc.h>
#endif

#ifndef DIABLOSMC_FUNCTIONS
#define DIABLOSMC_FUNCTIONS
#undef DIABLOSMC_H
#include <diablosmc.h>

//void DiabloSmcInit (int argc, char **argv);
void DiabloSmcInitAfterwards(t_cfg * cfg);
//void DiabloSmcFini();

extern  t_architecture_description smc_description;
#endif
#endif

/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
