/* This research is supported by the European Union Seventh Framework Programme (FP7/2007-2013), project ASPIRE (Advanced  Software Protection: Integration, Research, and Exploitation), under grant agreement no. 609734; on-line at https://aspire-fp7.eu/. */

/* The development of portions of the code contained in this file was sponsored by Samsung Electronics UK. */

#ifndef DIABLOFLOWGRAPH_H
#define DIABLOFLOWGRAPH_H
#ifdef DIABLOFLOWGRAPH_INTERNAL
#include "diabloflowgraph_config.h"
#endif

#ifdef GENERATE_CLASS_CODE
#undef GENERATE_CLASS_CODE
#include <diabloobject.h>
#define GENERATE_CLASS_CODE
#else
#include <diabloobject.h>
#endif
#include "diabloflowgraph_cmdline.h"
#include "diabloflowgraph_ins.h"
#include "diabloflowgraph_bbl.h"
#include "diabloflowgraph_function.h"
#include "diabloflowgraph_flowgraph.h"
#include "diabloflowgraph_edge.h"
#include "diabloflowgraph_loop.h"
#include "diabloflowgraph_programexit.h"
#include "diabloflowgraph_description.h"
#include "diabloflowgraph_iterators.h"
#include "diabloflowgraph_regset.h"
#include "diabloflowgraph_callgraph.h"
#include "diabloflowgraph_code_layout.h"
#include "diabloflowgraph_cfg_object.h"
#include "diabloflowgraph_profile.h"
#include "diabloflowgraph_dataflow.h"
#include "diabloflowgraph_complexity.h"
#include "diabloflowgraph_logging.h"

#ifndef DIABLOFLOWGRAPH_FUNCTIONS
#ifndef DIABLOFLOWGRAPH_TYPES
#define TYPEDEFS
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#include "diabloflowgraph_ins.class.h"
#undef TYPEDEFS
#else
#define TYPES
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#include "diabloflowgraph_ins.class.h"
#undef TYPES
#endif
#else
#define DEFINES
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#include "diabloflowgraph_ins.class.h"
#undef DEFINES
#define DEFINES2
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#include "diabloflowgraph_ins.class.h"
#undef DEFINES2
#define FUNCTIONS
#include "diabloflowgraph_ins.class.h"
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#undef FUNCTIONS
#define CONSTRUCTORS
#include "diabloflowgraph_ins.class.h"
#include "diabloflowgraph_bbl.class.h"
#include "diabloflowgraph_callgraph.class.h"
#include "diabloflowgraph_callgraph_edge.class.h"
#include "diabloflowgraph_edge.class.h"
#include "diabloflowgraph_flowgraph.class.h"
#include "diabloflowgraph_function.class.h"
#undef CONSTRUCTORS
#endif

#include "diabloflowgraph_dominator.h"

#ifndef DIABLOFLOWGRAPH_TYPES
#define DIABLOFLOWGRAPH_TYPES
#undef DIABLOFLOWGRAPH_H
#include <diabloflowgraph.h>
#endif

#ifndef DIABLOFLOWGRAPH_FUNCTIONS
#define DIABLOFLOWGRAPH_FUNCTIONS
#undef DIABLOFLOWGRAPH_H
#include <diabloflowgraph.h>
void DiabloFlowgraphSetKernelMode();
void DiabloFlowgraphSetKernelMode();
t_bool DiabloFlowgraphInKernelMode();
void DiabloFlowgraphInit (int, char **);
void DiabloFlowgraphFini ();
void DiabloFlowgraphCppInit (int, char **);
void DiabloFlowgraphCppFini ();
#endif

#include "diabloflowgraph_object_tracking.h"

#endif
/* vim: set shiftwidth=4 cinoptions={.5s,g0,p5,t0,(0,^-0.5s,n-2,+0.5s foldmethod=marker tw=78 cindent: */
/* vim: set shiftwidth=2 expandtab cinoptions=p5,t0,(0, foldmethod=marker tw=80 cindent: */
