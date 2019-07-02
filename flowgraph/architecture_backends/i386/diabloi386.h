#ifndef DIABLOI386_H
#define DIABLOI386_H

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloflowgraph.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloflowgraph.h>
#endif
#include "diabloi386_assemble.h"
#include "diabloi386_assemble_one.h"
#include "diabloi386_cmdline.h"
#include "diabloi386_deflowgraph.h"
#include "diabloi386_description.h"
#include "diabloi386_disassemble.h"
#include "diabloi386_disassemble_one.h"
#include "diabloi386_flowgraph.h"
#include "diabloi386_instructions.h"
#include "diabloi386_opcodes.h"
#include "diabloi386_print.h"
#include "diabloi386_registers.h"
#include "diabloi386_table.h"
#include "diabloi386_operand_iterators.h"
#include "diabloi386_operand_check.h"

#ifndef DIABLOI386_FUNCTIONS
#ifndef DIABLOI386_TYPES
#define TYPEDEFS
#include "diabloi386_i386_ins.class.h"
#undef TYPEDEFS 
#else
#define TYPES
#include "diabloi386_i386_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloi386_i386_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloi386_i386_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloi386_i386_ins.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloi386_i386_ins.class.h"
#undef CONSTRUCTORS
#endif


#ifndef DIABLOI386_TYPES
#define DIABLOI386_TYPES
#undef DIABLOI386_H
#include <diabloi386.h>
#endif

#ifndef DIABLOI386_FUNCTIONS
#define DIABLOI386_FUNCTIONS
#undef DIABLOI386_H
#include <diabloi386.h>

void DiabloI386Init(int, char **);
void DiabloI386Fini();
void DiabloI386CppInit(int, char **);
void DiabloI386CppFini();
#endif

#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
