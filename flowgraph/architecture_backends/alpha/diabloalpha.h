#ifndef DIABLOALPHA_H
#define DIABLOALPHA_H
#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif
#include "diabloalpha_disassemble.h"
#include "diabloalpha_assemble.h"
#include "diabloalpha_cmdline.h"
#include "diabloalpha_description.h"
#include "diabloalpha_instructions.h"
#include "diabloalpha_opcodes.h"
#include "diabloalpha_disassemble_one.h"
#include "diabloalpha_registers.h"
#include "diabloalpha_assemble_one.h"
#include "diabloalpha_assemble.h"
#include "diabloalpha_flowgraph.h"
#include "diabloalpha_deflowgraph.h"
//#include "diabloalpha_trace.h"

#ifndef DIABLOALPHA_FUNCTIONS
#ifndef DIABLOALPHA_TYPES
#define TYPEDEFS
#include "diabloalpha_alpha_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloalpha_alpha_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloalpha_alpha_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloalpha_alpha_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloalpha_alpha_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloalpha_alpha_ins.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOALPHA_TYPES
#define DIABLOALPHA_TYPES
#undef DIABLOALPHA_H
#include <diabloalpha.h>
#endif

#ifndef DIABLOALPHA_FUNCTIONS
#define DIABLOALPHA_FUNCTIONS
#undef DIABLOALPHA_H
#include <diabloalpha.h>

void DiabloAlphaInit(int, char **);
void DiabloAlphaFini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
