/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOSUPPORT_H
#define DIABLOSUPPORT_H

#include <stdlib.h>

#ifdef _MSC_VER
  #include <io.h>
  #include "diablosupport_win32.h"
#endif

#include <diablosupport_sort.h>

#include <diablosupport_config.h>
#include <diablosupport_types.h>
#include <diablosupport_string.h>
#include <diablosupport_malloc.h>
#include <diablosupport_stdio.h>
#include <diablosupport_file.h>
#include <diablosupport_dot.h>
#include <diablosupport_graph.h>
#include <diablosupport_hash.h>
#include <diablosupport_options.h>
#include <diablosupport_cmdline.h>
#include <diablosupport_linkedlist.h>
#include <diablosupport_address.h>
#include <diablosupport_set.h>
#include <diablosupport_class.h>
#include <diablosupport_managed_class.h>
#include <diablosupport_broker.h>
#include <diablosupport_conffile.h>
#include <diablosupport_ptr_array.h>
#include <diablosupport_readline.h>
#include <diablosupport_timing.h>
#include <diablosupport_random.h>
#include <diablosupport_version.h>

#ifndef DIABLOSUPPORT_FUNCTIONS
#ifndef DIABLOSUPPORT_TYPES
#define TYPEDEFS
#include <diablosupport_node.class.h>
#include <diablosupport_graph.class.h>
#include <diablosupport_edge.class.h>
#include <diablosupport_hash_node.class.h>
#include <diablosupport_hash_table.class.h>
#undef TYPEDEFS
#else
#define TYPES
#include <diablosupport_node.class.h>
#include <diablosupport_graph.class.h>
#include <diablosupport_edge.class.h>
#include <diablosupport_hash_node.class.h>
#include <diablosupport_hash_table.class.h>
#undef TYPES
#endif
#else
#define DEFINES
#include <diablosupport_node.class.h>
#include <diablosupport_graph.class.h>
#include <diablosupport_edge.class.h>
#include <diablosupport_hash_node.class.h>
#include <diablosupport_hash_table.class.h>
#undef DEFINES
#define DEFINES2
#include <diablosupport_node.class.h>
#include <diablosupport_graph.class.h>
#include <diablosupport_edge.class.h>
#include <diablosupport_hash_node.class.h>
#include <diablosupport_hash_table.class.h>
#undef DEFINES2
#define FUNCTIONS
#include <diablosupport_node.class.h>
#include <diablosupport_graph.class.h>
#include <diablosupport_edge.class.h>
#include <diablosupport_hash_node.class.h>
#include <diablosupport_hash_table.class.h>
#undef FUNCTIONS
#endif

#ifndef DIABLOSUPPORT_TYPES
#define DIABLOSUPPORT_TYPES
#undef DIABLOSUPPORT_H
#include <diablosupport.h>
#endif

#ifndef DiabloSupportInit
#define DiabloSupportInit DiabloSupportInitCommon
#endif

#ifndef DIABLOSUPPORT_FUNCTIONS
#define DIABLOSUPPORT_FUNCTIONS
#undef DIABLOSUPPORT_H
#include <diablosupport.h>
#endif

#ifdef DIABLOSUPPORT_FUNCTIONS
void DiabloSupportInit32 (int argc, char **argv);
void DiabloSupportInit64 (int argc, char **argv);
void DiabloSupportInitGeneric (int argc, char **argv);
void DiabloSupportInitCommon (int argc, char **argv);
void DiabloSupportFini ();
#endif

/* Needs to be included after defining DIABLOSUPPORT_FUNCTIONS */
#include <diablosupport_logging.h>
#include <diablosupport_utils.h>

#ifdef _MSC_VER
  #define WIN32_DEBUGGING_MACROS
  #include "diablosupport_win32.h"
#endif

#endif
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
