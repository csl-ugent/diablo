/** \mainpage Diablo objects
 *
 * \section intro Introduction
 *
 * Diablo objects provides all functionality to handle objects. It provides
 * generic interfaces for
 *
 * - archives (diabloobject_archives.h),
 * - object files (diabloobject_object.h),
 * - sections (diabloobject_section.h),
 * - symbols (diabloobject_symbol.h),
 * - relocations (diabloobject_reloc.h),
 * - maps (diabloobject_parsemap.h) and
 * - segments (diabloobject_segment.h)
 *
 * for different architectures. Furthermore it has functionality for
 *
 * - object layout (diabloobject_layout.h) and
 * - object linking (diabloobject_linkers.h and diabloobject_link.h).
 *
 * \section use Using Diablo objects
 *
 * To use diablo objects, include diabloobject.h, and call DiabloObjectInit()
 * at the start of the program.
 */

#ifndef DIABLOOBJECT_H
#define DIABLOOBJECT_H
#ifdef DIABLOOBJECT_INTERNAL
#include "diabloobject_config.h"
#endif
#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diablosupport.h>
#define GENERATE_CLASS_CODE
#else
#include <diablosupport.h>
#endif
#include "diabloobject_archives.h"
#include "diabloobject_iterators.h"
#include "diabloobject_layout.h"
#include "diabloobject_layout_script.h"
#include "diabloobject_linkers.h"
#include "diabloobject_link.h"
#include "diabloobject_object.h"
#include "diabloobject_parsemap.h"
#include "diabloobject_relocatable.h"
#include "diabloobject_reloc.h"
#include "diabloobject_section.h"
#include "diabloobject_segment.h"
#include "diabloobject_stack_machine.h"
#include "diabloobject_symbol.h"
#include "diabloobject_cmdline.h"
#include "diabloobject_exception_handler.h"
#include "diabloobject_relocate.h"
#include "diabloobject_dumps.h"

#ifndef DIABLOOBJECT_FUNCTIONS
#ifndef DIABLOOBJECT_TYPES
#define TYPEDEFS
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef TYPEDEFS
#else
#define TYPES
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef TYPES
#endif
#else
#define DEFINES
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef DEFINES
#define DEFINES2
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef DEFINES2
#define FUNCTIONS
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef FUNCTIONS
#define CONSTRUCTORS
#include <diabloobject_relocatable.class.h>
#include <diabloobject_section.class.h>
#include <diabloobject_segment.class.h>
#include <diabloobject_object_hash_node.class.h>
#include <diabloobject_object.class.h>
#include <diabloobject_symbol.class.h>
#include <diabloobject_symbol_table.class.h>
#include <diabloobject_reloc.class.h>
#include <diabloobject_reloc_table.class.h>
#include <diabloobject_reloc_ref.class.h>
#undef CONSTRUCTORS
#endif

#ifndef DIABLOOBJECT_TYPES
#define DIABLOOBJECT_TYPES
#undef DIABLOOBJECT_H
#include <diabloobject.h>
#endif

#ifndef DIABLOOBJECT_FUNCTIONS
#define DIABLOOBJECT_FUNCTIONS
#undef DIABLOOBJECT_H
#include <diabloobject.h>
#endif

#ifdef DIABLOOBJECT_FUNCTIONS
void DiabloObjectInit (int, char **);
void DiabloObjectFini ();
#endif
#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
