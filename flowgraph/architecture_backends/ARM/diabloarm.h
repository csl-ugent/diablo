#ifndef DIABLOARM_H
#define DIABLOARM_H
#ifdef _MSC_VER
#include <stdlib.h>
#include <io.h>
#endif
#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif
#include "diabloarm_assemble.h"
#include "diabloarm_assemble_one.h"
#include "diabloarm_cmdline.h"
#include "diabloarm_deflowgraph.h"
#include "diabloarm_description.h"
#include "diabloarm_disassemble.h"
#include "diabloarm_disassemble_one.h"
#include "diabloarm_flowgraph.h"
#include "diabloarm_instructions.h"
#include "diabloarm_layout.h"
#include "diabloarm_opcodes.h"
#include "diabloarm_peephole.h"
#include "diabloarm_print.h"
#include "diabloarm_registers.h"
#include "diabloarm_thumb_disassemble_one.h"
#include "diabloarm_thumb_opcodes.h"
#include "diabloarm_thumb_print.h"
#include "diabloarm_thumb_assemble.h"
#include "diabloarm_thumb_assemble_one.h"
#include "diabloarm_utils.h"
#include "diabloarm_tls.h"

#ifndef DIABLOARM_INLINES
#ifndef DIABLOARM_FUNCTIONS
#ifndef DIABLOARM_TYPES
#define TYPEDEFS
#include "diabloarm_arm_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloarm_arm_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloarm_arm_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloarm_arm_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloarm_arm_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloarm_arm_ins.class.h"
#undef CONSTRUCTORS
#endif
#endif


#ifndef DIABLOARM_TYPES
#define DIABLOARM_TYPES
#undef DIABLOARM_H
#include <diabloarm.h>
#endif

#ifndef DIABLOARM_FUNCTIONS
#define DIABLOARM_FUNCTIONS
#undef DIABLOARM_H
#include <diabloarm.h>

void DiabloArmInit(int, char **);
void DiabloArmFini();
void DiabloArmCppInit(int, char **);
void DiabloArmCppFini();
#endif

#ifndef DIABLOARM_INLINES
#define DIABLOARM_INLINES
#undef DIABLOARM_H
#include <diabloarm.h>
#endif

#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
