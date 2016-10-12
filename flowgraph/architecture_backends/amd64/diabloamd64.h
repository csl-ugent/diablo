#ifndef DIABLOAMD64_H
#define DIABLOAMD64_H

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif
#include "diabloamd64_assemble.h"
#include "diabloamd64_assemble_one.h"
#include "diabloamd64_cmdline.h"
#include "diabloamd64_deflowgraph.h"
#include "diabloamd64_description.h"
#include "diabloamd64_disassemble.h"
#include "diabloamd64_disassemble_one.h"
#include "diabloamd64_flowgraph.h"
#include "diabloamd64_instructions.h"
#include "diabloamd64_opcodes.h"
#include "diabloamd64_print.h"
#include "diabloamd64_registers.h"
#include "diabloamd64_table.h"
#include "diabloamd64_operand_iterators.h"
#include "diabloamd64_operand_check.h"

#ifndef DIABLOAMD64_FUNCTIONS
#ifndef DIABLOAMD64_TYPES
#define TYPEDEFS
#include "diabloamd64_amd64_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloamd64_amd64_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloamd64_amd64_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloamd64_amd64_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloamd64_amd64_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloamd64_amd64_ins.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOAMD64_TYPES
#define DIABLOAMD64_TYPES
#undef DIABLOAMD64_H
#include <diabloamd64.h>
#endif

#ifndef DIABLOAMD64_FUNCTIONS
#define DIABLOAMD64_FUNCTIONS
#undef DIABLOAMD64_H
#include <diabloamd64.h>

void DiabloAmd64Init(int, char **);
void DiabloAmd64Fini();
#endif
#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
