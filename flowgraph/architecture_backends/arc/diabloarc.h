#ifndef DIABLOARC_H
#define DIABLOARC_H
#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif
#include "diabloarc_cmdline.h"
#include "diabloarc_description.h"
#include "diabloarc_registers.h"

#ifndef DIABLOARC_FUNCTIONS
#ifndef DIABLOARC_TYPES
#define TYPEDEFS
#include "diabloarc_arc_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloarc_arc_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloarc_arc_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloarc_arc_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloarc_arc_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloarc_arc_ins.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOARC_TYPES
#define DIABLOARC_TYPES
#undef DIABLOARC_H
#include <diabloarc.h>
#endif

#ifndef DIABLOARC_FUNCTIONS
#define DIABLOARC_FUNCTIONS
#undef DIABLOARC_H
#include <diabloarc.h>

void DiabloArcInit(int, char **);
void DiabloArcFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
